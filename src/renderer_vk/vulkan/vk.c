/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file vulkan/vk.c
 * @brief Vulkan implementation: instance/device/surface/
 * swapchain/render pass/framebuffers/command buffers/sync objects and a
 * clear+present frame loop. No geometry is submitted yet.
 */

#include <vulkan/vulkan.h>
#include <string.h>
#include <stdlib.h>

#include "vk.h"
#include "../../renderercommon/tr_common.h"
#include "../tr_local.h"

#define VK_MAX_SWAPCHAIN_IMAGES 8

typedef struct
{
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	uint32_t graphicsQueueFamily;
	uint32_t presentQueueFamily;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
	VkImage swapchainImages[VK_MAX_SWAPCHAIN_IMAGES];
	VkImageView swapchainImageViews[VK_MAX_SWAPCHAIN_IMAGES];
	VkFramebuffer swapchainFramebuffers[VK_MAX_SWAPCHAIN_IMAGES];

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	qboolean initialized;
	uint32_t currentImageIndex;
} vkContext_t;

static vkContext_t vk;

/**
 * @brief Minimal error helper: prints the VkResult and returns qfalse so
 * callers can bail out cleanly instead of crashing on an invalid handle.
 */
static qboolean VK_CheckResult(VkResult result, const char *what)
{
	if (result != VK_SUCCESS)
	{
		Ren_Print("Vulkan: %s failed (VkResult %d)\n", what, (int)result);
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief Returns qtrue if the named instance extension is present in the
 * loader's enumeration (used to conditionally opt in to
 * VK_KHR_portability_enumeration, required by MoltenVK-based loaders on
 * macOS since Vulkan SDK 1.3.216 to avoid VK_ERROR_INCOMPATIBLE_DRIVER).
 */
static qboolean VK_HasInstanceExtension(const char *name)
{
	uint32_t              count = 0, i;
	VkExtensionProperties *props;

	if (vkEnumerateInstanceExtensionProperties(NULL, &count, NULL) != VK_SUCCESS || count == 0)
	{
		return qfalse;
	}

	props = malloc(count * sizeof(VkExtensionProperties));
	if (!props)
	{
		return qfalse;
	}

	if (vkEnumerateInstanceExtensionProperties(NULL, &count, props) != VK_SUCCESS)
	{
		free(props);
		return qfalse;
	}

	for (i = 0; i < count; i++)
	{
		if (!strcmp(props[i].extensionName, name))
		{
			free(props);
			return qtrue;
		}
	}

	free(props);
	return qfalse;
}

static qboolean VK_CreateInstance(void)
{
	unsigned int         extCount = 0;
	const char           *extNames[32];
	qboolean             usePortability;
	VkApplicationInfo    appInfo;
	VkInstanceCreateInfo createInfo;

	if (!ri.VKimp_GetInstanceExtensions(&extCount, NULL))
	{
		return qfalse;
	}
	// Reserve one extra slot for VK_KHR_portability_enumeration.
	if (extCount >= (sizeof(extNames) / sizeof(extNames[0])) - 1)
	{
		Ren_Print("Vulkan: too many required instance extensions (%u)\n", extCount);
		return qfalse;
	}
	if (!ri.VKimp_GetInstanceExtensions(&extCount, extNames))
	{
		return qfalse;
	}

	// macOS/MoltenVK loaders expose the ICD as a "portability subset"
	// driver; without requesting this extension and flag, vkCreateInstance
	// fails with VK_ERROR_INCOMPATIBLE_DRIVER on recent SDKs.
	usePortability = VK_HasInstanceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	if (usePortability)
	{
		extNames[extCount++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	}

	Com_Memset(&appInfo, 0, sizeof(appInfo));
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "ET: Legacy";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "ET: Legacy renderer_vk";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = extCount;
	createInfo.ppEnabledExtensionNames = extNames;
	// NOTE: VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR is a
	// VkInstanceCreateFlagBits enumerator, not a preprocessor macro, so it
	// cannot be probed with #ifdef - it's unconditionally available once
	// vulkan_core.h declares VK_KHR_portability_enumeration (SDK >= 1.3.216).
	if (usePortability)
	{
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}

	return VK_CheckResult(vkCreateInstance(&createInfo, NULL, &vk.instance), "vkCreateInstance");
}

static qboolean VK_PickPhysicalDeviceAndQueues(void)
{
	uint32_t         deviceCount = 0, i, q;
	VkPhysicalDevice devices[16];
	VkBool32         presentSupport;

	vkEnumeratePhysicalDevices(vk.instance, &deviceCount, NULL);
	if (deviceCount == 0)
	{
		Ren_Print("Vulkan: no physical devices found\n");
		return qfalse;
	}
	if (deviceCount > 16)
	{
		deviceCount = 16;
	}
	vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices);

	// pick the first device that has both a graphics and a present queue
	for (i = 0; i < deviceCount; i++)
	{
		uint32_t                queueFamilyCount = 0;
		VkQueueFamilyProperties queueFamilies[32];
		int                     graphicsFamily = -1, presentFamily = -1;

		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL);
		if (queueFamilyCount > 32)
		{
			queueFamilyCount = 32;
		}
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

		for (q = 0; q < queueFamilyCount; q++)
		{
			if (queueFamilies[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsFamily = (int)q;
			}

			presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], q, vk.surface, &presentSupport);
			if (presentSupport)
			{
				presentFamily = (int)q;
			}
		}

		if (graphicsFamily >= 0 && presentFamily >= 0)
		{
			vk.physicalDevice      = devices[i];
			vk.graphicsQueueFamily = (uint32_t)graphicsFamily;
			vk.presentQueueFamily  = (uint32_t)presentFamily;
			return qtrue;
		}
	}

	Ren_Print("Vulkan: no suitable physical device (graphics+present queue) found\n");
	return qfalse;
}

static qboolean VK_CreateLogicalDevice(void)
{
	float                    queuePriority = 1.0f;
	VkDeviceQueueCreateInfo  queueCreateInfos[2];
	uint32_t                 queueCreateInfoCount = 0;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkDeviceCreateInfo       createInfo;
	const char               *deviceExtensions[2] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, NULL };
	uint32_t                 deviceExtensionCount = 1;
	uint32_t                 availableExtCount = 0, i;
	VkExtensionProperties    *availableExt;

	Com_Memset(queueCreateInfos, 0, sizeof(queueCreateInfos));

	queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = vk.graphicsQueueFamily;
	queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
	queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
	queueCreateInfoCount++;

	if (vk.presentQueueFamily != vk.graphicsQueueFamily)
	{
		queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = vk.presentQueueFamily;
		queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
		queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
		queueCreateInfoCount++;
	}

	Com_Memset(&deviceFeatures, 0, sizeof(deviceFeatures));

	// MoltenVK-backed physical devices report VK_KHR_portability_subset as
	// a supported device extension; the Vulkan spec requires enabling it
	// at device-creation time whenever the physical device supports it.
	if (vkEnumerateDeviceExtensionProperties(vk.physicalDevice, NULL, &availableExtCount, NULL) == VK_SUCCESS &&
	    availableExtCount > 0)
	{
		availableExt = malloc(availableExtCount * sizeof(VkExtensionProperties));
		if (availableExt && vkEnumerateDeviceExtensionProperties(vk.physicalDevice, NULL, &availableExtCount, availableExt) == VK_SUCCESS)
		{
			for (i = 0; i < availableExtCount; i++)
			{
				if (!strcmp(availableExt[i].extensionName, "VK_KHR_portability_subset"))
				{
					deviceExtensions[deviceExtensionCount++] = "VK_KHR_portability_subset";
					break;
				}
			}
		}
		if (availableExt)
		{
			free(availableExt);
		}
	}

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos       = queueCreateInfos;
	createInfo.queueCreateInfoCount    = queueCreateInfoCount;
	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = deviceExtensionCount;
	createInfo.ppEnabledExtensionNames = deviceExtensions;

	if (!VK_CheckResult(vkCreateDevice(vk.physicalDevice, &createInfo, NULL, &vk.device), "vkCreateDevice"))
	{
		return qfalse;
	}

	vkGetDeviceQueue(vk.device, vk.graphicsQueueFamily, 0, &vk.graphicsQueue);
	vkGetDeviceQueue(vk.device, vk.presentQueueFamily, 0, &vk.presentQueue);

	return qtrue;
}

static qboolean VK_CreateSwapchainAndViews(int width, int height)
{
	VkSurfaceCapabilitiesKHR caps;
	VkSurfaceFormatKHR       formats[32];
	uint32_t                 formatCount = 32;
	VkPresentModeKHR         presentMode = VK_PRESENT_MODE_FIFO_KHR; // always supported, vsynced
	VkExtent2D               extent;
	VkSwapchainCreateInfoKHR createInfo;
	uint32_t                 i;
	uint32_t                 queueFamilyIndices[2];

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &caps);
	vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, formats);
	if (formatCount == 0)
	{
		Ren_Print("Vulkan: surface has no formats\n");
		return qfalse;
	}

	vk.swapchainFormat = formats[0].format;
	for (i = 0; i < formatCount; i++)
	{
		if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
		{
			vk.swapchainFormat = formats[i].format;
			break;
		}
	}

	if (caps.currentExtent.width != 0xFFFFFFFFu)
	{
		extent = caps.currentExtent;
	}
	else
	{
		extent.width  = (uint32_t)Com_Clamp(caps.minImageExtent.width, caps.maxImageExtent.width, (float)width);
		extent.height = (uint32_t)Com_Clamp(caps.minImageExtent.height, caps.maxImageExtent.height, (float)height);
	}
	vk.swapchainExtent = extent;

	vk.swapchainImageCount = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && vk.swapchainImageCount > caps.maxImageCount)
	{
		vk.swapchainImageCount = caps.maxImageCount;
	}
	if (vk.swapchainImageCount > VK_MAX_SWAPCHAIN_IMAGES)
	{
		vk.swapchainImageCount = VK_MAX_SWAPCHAIN_IMAGES;
	}

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface          = vk.surface;
	createInfo.minImageCount    = vk.swapchainImageCount;
	createInfo.imageFormat      = vk.swapchainFormat;
	createInfo.imageColorSpace  = formats[0].colorSpace;
	createInfo.imageExtent      = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	queueFamilyIndices[0] = vk.graphicsQueueFamily;
	queueFamilyIndices[1] = vk.presentQueueFamily;
	if (vk.graphicsQueueFamily != vk.presentQueueFamily)
	{
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform   = caps.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode    = presentMode;
	createInfo.clipped        = VK_TRUE;

	if (!VK_CheckResult(vkCreateSwapchainKHR(vk.device, &createInfo, NULL, &vk.swapchain), "vkCreateSwapchainKHR"))
	{
		return qfalse;
	}

	vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.swapchainImageCount, NULL);
	if (vk.swapchainImageCount > VK_MAX_SWAPCHAIN_IMAGES)
	{
		vk.swapchainImageCount = VK_MAX_SWAPCHAIN_IMAGES;
	}
	vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.swapchainImageCount, vk.swapchainImages);

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		VkImageViewCreateInfo viewInfo;

		Com_Memset(&viewInfo, 0, sizeof(viewInfo));
		viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                       = vk.swapchainImages[i];
		viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                      = vk.swapchainFormat;
		viewInfo.components.r                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.swapchainImageViews[i]), "vkCreateImageView"))
		{
			return qfalse;
		}
	}

	return qtrue;
}

static qboolean VK_CreateRenderPassAndFramebuffers(void)
{
	VkAttachmentDescription colorAttachment;
	VkAttachmentReference   colorAttachmentRef;
	VkSubpassDescription    subpass;
	VkSubpassDependency     dependency;
	VkRenderPassCreateInfo  renderPassInfo;
	uint32_t                i;

	Com_Memset(&colorAttachment, 0, sizeof(colorAttachment));
	colorAttachment.format         = vk.swapchainFormat;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	Com_Memset(&colorAttachmentRef, 0, sizeof(colorAttachmentRef));
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Com_Memset(&subpass, 0, sizeof(subpass));
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments    = &colorAttachmentRef;

	Com_Memset(&dependency, 0, sizeof(dependency));
	dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass    = 0;
	dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	Com_Memset(&renderPassInfo, 0, sizeof(renderPassInfo));
	renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments    = &colorAttachment;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies   = &dependency;

	if (!VK_CheckResult(vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.renderPass), "vkCreateRenderPass"))
	{
		return qfalse;
	}

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		VkFramebufferCreateInfo framebufferInfo;

		Com_Memset(&framebufferInfo, 0, sizeof(framebufferInfo));
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass      = vk.renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments    = &vk.swapchainImageViews[i];
		framebufferInfo.width           = vk.swapchainExtent.width;
		framebufferInfo.height          = vk.swapchainExtent.height;
		framebufferInfo.layers          = 1;

		if (!VK_CheckResult(vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.swapchainFramebuffers[i]), "vkCreateFramebuffer"))
		{
			return qfalse;
		}
	}

	return qtrue;
}

static qboolean VK_CreateCommandObjects(void)
{
	VkCommandPoolCreateInfo     poolInfo;
	VkCommandBufferAllocateInfo allocInfo;
	VkSemaphoreCreateInfo       semaphoreInfo;
	VkFenceCreateInfo           fenceInfo;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = vk.graphicsQueueFamily;

	if (!VK_CheckResult(vkCreateCommandPool(vk.device, &poolInfo, NULL, &vk.commandPool), "vkCreateCommandPool"))
	{
		return qfalse;
	}

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = vk.commandPool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (!VK_CheckResult(vkAllocateCommandBuffers(vk.device, &allocInfo, &vk.commandBuffer), "vkAllocateCommandBuffers"))
	{
		return qfalse;
	}

	Com_Memset(&semaphoreInfo, 0, sizeof(semaphoreInfo));
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	Com_Memset(&fenceInfo, 0, sizeof(fenceInfo));
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (!VK_CheckResult(vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.imageAvailableSemaphore), "vkCreateSemaphore(imageAvailable)"))
	{
		return qfalse;
	}
	if (!VK_CheckResult(vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.renderFinishedSemaphore), "vkCreateSemaphore(renderFinished)"))
	{
		return qfalse;
	}
	if (!VK_CheckResult(vkCreateFence(vk.device, &fenceInfo, NULL, &vk.inFlightFence), "vkCreateFence"))
	{
		return qfalse;
	}

	return qtrue;
}

static void VK_DestroySwapchainObjects(void)
{
	uint32_t i;

	if (vk.device == VK_NULL_HANDLE)
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		if (vk.swapchainFramebuffers[i] != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(vk.device, vk.swapchainFramebuffers[i], NULL);
			vk.swapchainFramebuffers[i] = VK_NULL_HANDLE;
		}
		if (vk.swapchainImageViews[i] != VK_NULL_HANDLE)
		{
			vkDestroyImageView(vk.device, vk.swapchainImageViews[i], NULL);
			vk.swapchainImageViews[i] = VK_NULL_HANDLE;
		}
	}

	if (vk.renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(vk.device, vk.renderPass, NULL);
		vk.renderPass = VK_NULL_HANDLE;
	}

	if (vk.swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);
		vk.swapchain = VK_NULL_HANDLE;
	}
}

qboolean VK_ResizeSwapchain(int width, int height)
{
	VK_DestroySwapchainObjects();

	if (!VK_CreateSwapchainAndViews(width, height))
	{
		return qfalse;
	}
	if (!VK_CreateRenderPassAndFramebuffers())
	{
		return qfalse;
	}

	return qtrue;
}

qboolean VK_Init(int width, int height)
{
	VkPhysicalDeviceProperties props;

	Com_Memset(&vk, 0, sizeof(vk));

	if (!ri.VKimp_GetInstanceExtensions || !ri.VKimp_CreateSurface)
	{
		Ren_Print("Vulkan: engine does not expose VKimp_* window-system hooks\n");
		return qfalse;
	}

	if (!VK_CreateInstance())
	{
		return qfalse;
	}

	if (!ri.VKimp_CreateSurface(vk.instance, (void **)&vk.surface))
	{
		Ren_Print("Vulkan: failed to create window surface\n");
		return qfalse;
	}

	if (!VK_PickPhysicalDeviceAndQueues())
	{
		return qfalse;
	}

	if (!VK_CreateLogicalDevice())
	{
		return qfalse;
	}

	if (!VK_ResizeSwapchain(width, height))
	{
		return qfalse;
	}

	if (!VK_CreateCommandObjects())
	{
		return qfalse;
	}

	vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);
	Q_strncpyz(glConfig.vendor_string, "Vulkan", sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string, props.deviceName, sizeof(glConfig.renderer_string));
	Com_sprintf(glConfig.version_string, sizeof(glConfig.version_string), "%d.%d.%d",
	            VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
	glConfig.maxTextureSize = (int)props.limits.maxImageDimension2D;

	vk.initialized = qtrue;

	return qtrue;
}

void VK_Shutdown(void)
{
	if (vk.device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(vk.device);
	}

	VK_DestroySwapchainObjects();

	if (vk.imageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(vk.device, vk.imageAvailableSemaphore, NULL);
	}
	if (vk.renderFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(vk.device, vk.renderFinishedSemaphore, NULL);
	}
	if (vk.inFlightFence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vk.device, vk.inFlightFence, NULL);
	}
	if (vk.commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(vk.device, vk.commandPool, NULL);
	}
	if (vk.device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(vk.device, NULL);
	}
	if (vk.surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
	}
	if (vk.instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(vk.instance, NULL);
	}

	Com_Memset(&vk, 0, sizeof(vk));
}

void VK_BeginFrame(void)
{
	VkCommandBufferBeginInfo beginInfo;
	VkRenderPassBeginInfo    renderPassInfo;
	VkClearValue             clearColor;

	if (!vk.initialized)
	{
		return;
	}

	vkWaitForFences(vk.device, 1, &vk.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(vk.device, 1, &vk.inFlightFence);

	vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore, VK_NULL_HANDLE, &vk.currentImageIndex);

	vkResetCommandBuffer(vk.commandBuffer, 0);

	Com_Memset(&beginInfo, 0, sizeof(beginInfo));
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);

	clearColor.color.float32[0] = 0.0f;
	clearColor.color.float32[1] = 0.0f;
	clearColor.color.float32[2] = 0.0f;
	clearColor.color.float32[3] = 1.0f;

	Com_Memset(&renderPassInfo, 0, sizeof(renderPassInfo));
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = vk.renderPass;
	renderPassInfo.framebuffer       = vk.swapchainFramebuffers[vk.currentImageIndex];
	renderPassInfo.renderArea.extent = vk.swapchainExtent;
	renderPassInfo.clearValueCount   = 1;
	renderPassInfo.pClearValues      = &clearColor;

	vkCmdBeginRenderPass(vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VK_EndFrame(void)
{
	VkSubmitInfo         submitInfo;
	VkPresentInfoKHR     presentInfo;
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	if (!vk.initialized)
	{
		return;
	}

	vkCmdEndRenderPass(vk.commandBuffer);
	vkEndCommandBuffer(vk.commandBuffer);

	Com_Memset(&submitInfo, 0, sizeof(submitInfo));
	submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.pWaitSemaphores      = &vk.imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask    = &waitStage;
	submitInfo.commandBufferCount   = 1;
	submitInfo.pCommandBuffers      = &vk.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = &vk.renderFinishedSemaphore;

	vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.inFlightFence);

	Com_Memset(&presentInfo, 0, sizeof(presentInfo));
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = &vk.renderFinishedSemaphore;
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = &vk.swapchain;
	presentInfo.pImageIndices      = &vk.currentImageIndex;

	vkQueuePresentKHR(vk.presentQueue, &presentInfo);
}
