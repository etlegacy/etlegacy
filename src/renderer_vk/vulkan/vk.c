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

// Generated (GLSL -> SPIR-V -> C header) by cmake/ETLBin2H.cmake at build
// time; see the FEATURE_RENDERER_VULKAN block in cmake/ETLBuildRenderer.cmake.
#include "ui.vert.glsl.h"
#include "ui.frag.glsl.h"
#include "tr3d.vert.glsl.h"
#include "tr3d.frag.glsl.h"
#include "tr3d_lm.vert.glsl.h"
#include "tr3d_lm.frag.glsl.h"
#include "gamma.vert.glsl.h"
#include "gamma.frag.glsl.h"

#define VK_MAX_SWAPCHAIN_IMAGES 8

// A world can have many distinct shader stateBits/cullType combinations;
// pipelines are created lazily and cached for the life of the renderer.
#define VK_MAX_SCENE_PIPELINES 128

// non-GLS pipeline states used by stencil shadow volumes
// (renderer1's RB_ShadowTessEnd()/RB_ShadowFinish()).
#define VK_SPECIAL_NONE           0
#define VK_SPECIAL_SHADOW_INCR    1 // volume faces: no color, stencil ALWAYS, pass -> increment
#define VK_SPECIAL_SHADOW_DECR    2 // volume faces: no color, stencil ALWAYS, pass -> decrement
#define VK_SPECIAL_SHADOW_DARKEN  3 // full-screen darken where stencil != 0

// One lazily-created pipeline variant for a given (stateBits, cullType,
// polygonOffset) combination used by the 3D scene pipelines.
typedef struct
{
	unsigned long stateBits;
	int cullType;
	qboolean polygonOffset;
	int special; // VK_SPECIAL_* (stencil shadow pipelines), 0 = normal
	VkPipeline pipeline;
} vkScenePipelineEntry_t;

// One lazily-created pipeline variant for a given stateBits value used by
// the 2D/UI pipeline (see VK_GetUIPipeline()). UI shader stages (e.g. the
// automap circular-mask trick in cg_commandmap.c, which relies on
// alphafunc + depthFunc equal) need their real stateBits honored instead
// of always forcing the same fixed blend-only state.
typedef struct
{
	unsigned long stateBits;
	VkPipeline pipeline;
} vkUIPipelineEntry_t;

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
	VkFramebuffer swapchainFramebuffers[VK_MAX_SWAPCHAIN_IMAGES]; // presentRenderPass targets

	// the main render pass (vk.renderPass) draws the whole
	// frame into this offscreen color image; VK_EndFrame() then copies it
	// to the swapchain through the gamma pass (renderer1's mainFbo +
	// R_ScreenGamma()).
	VkImage sceneColorImage;
	VkDeviceMemory sceneColorMemory;
	VkImageView sceneColorView;
	VkFramebuffer sceneFramebuffer;
	VkRenderPass presentRenderPass;
	VkDescriptorSet sceneColorDescriptor;
	VkShaderModule gammaVertModule;
	VkShaderModule gammaFragModule;
	VkPipelineLayout gammaPipelineLayout;
	VkPipeline gammaPipeline;
	float gammaValue;
	float overbrightScale;

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	// depth buffer, recreated alongside the swapchain.
	VkFormat depthFormat;
	qboolean depthHasStencil;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	// UI/2D rendering pipeline.
	VkDescriptorSetLayout uiDescriptorSetLayout;
	VkPipelineLayout uiPipelineLayout;
	vkUIPipelineEntry_t uiPipelines[VK_MAX_SCENE_PIPELINES];
	int numUIPipelines;
	VkDescriptorPool uiDescriptorPool;
	VkSampler uiSamplerRepeat; // non-mipmapped images (UI, lightmaps, ...): linear, level 0 only
	VkSampler uiSamplerClamp;

	// samplers for mipmapped images, built from r_textureMode
	// (GL_TextureMode) and r_ext_texture_filter_anisotropic.
	VkSampler mipSamplerRepeat;
	VkSampler mipSamplerClamp;
	int texFilterMin;            // GL enum, e.g. GL_LINEAR_MIPMAP_NEAREST
	int texFilterMag;            // GL enum, GL_NEAREST or GL_LINEAR
	float maxSamplerAnisotropy;  // 0 when samplerAnisotropy is unsupported

	// Single-frame-in-flight "arena" vertex/index buffers for 2D draws,
	// persistently mapped and reset at the start of every frame. Safe
	// because there is only one command buffer/fence in flight (see
	// VK_BeginFrame/VK_EndFrame) so the CPU never writes into memory the
	// GPU might still be reading.
	VkBuffer vertexArenaBuffer;
	VkDeviceMemory vertexArenaMemory;
	void *vertexArenaMapped;
	VkDeviceSize vertexArenaSize;
	VkDeviceSize vertexArenaOffset;

	VkBuffer indexArenaBuffer;
	VkDeviceMemory indexArenaMemory;
	void *indexArenaMapped;
	VkDeviceSize indexArenaSize;
	VkDeviceSize indexArenaOffset;

	// static 3D geometry pipelines + resources.
	VkPipelineLayout scenePipelineLayout;   // 1 descriptor set (base texture)
	VkPipelineLayout sceneLMPipelineLayout; // 2 descriptor sets (base + lightmap)

	VkShaderModule sceneVertModule;
	VkShaderModule sceneFragModule;
	VkShaderModule sceneLMVertModule;
	VkShaderModule sceneLMFragModule;

	VkShaderModule uiVertModule;
	VkShaderModule uiFragModule;

	vkScenePipelineEntry_t scenePipelines[VK_MAX_SCENE_PIPELINES];
	int numScenePipelines;
	vkScenePipelineEntry_t sceneLMPipelines[VK_MAX_SCENE_PIPELINES];
	int numSceneLMPipelines;

	// Shared across UI and scene draws: tracks whichever pipeline (UI or
	// one of the scene variants) was bound most recently in this command
	// buffer, so a redundant vkCmdBindPipeline can be skipped safely even
	// when UI and 3D draws are interleaved within the same frame (e.g. the
	// limbo menu's 2D background/buttons around its small 3D model preview).
	VkPipeline lastBoundPipeline;

	// GL-convention (column-major) projection/modelview matrices captured
	// from the no-op'd glLoadMatrixf() call sites in tr_backend.c (see
	// VK_SetProjectionMatrix/VK_SetModelViewMatrix). projMatrix already has
	// the GL->Vulkan clip-space correction (Y-flip + depth 0..1) baked in.
	float projMatrix[16];
	float modelMatrix[16];
	float mvpMatrix[16];

	// current fog state, mirroring renderer1's glfogsettings
	// (R_Fog()/R_FogOff() in tr_main.c call VK_SetFog()/VK_SetFogOff()
	// instead of the no-op'd glFog*() calls). Linear fog only (GL_LINEAR
	// is the mode R_Fog() defaults to when a fog volume doesn't specify
	// one, which covers the vast majority of ET fog usage).
	qboolean fogEnabled;
	qboolean fogParamsValid; // VK_SetFog() has run at least once
	float clearColor[4];     // last glClearColor() value
	float fogColor[4];
	float fogStart;
	float fogEnd;

	// last scene viewport rect (post Y-flip), stashed so
	// VK_SetSkyDepthRange() can reissue vkCmdSetViewport with the same
	// rect but a different depth range (sky is drawn pinned to the far
	// depth plane, same as renderer1's glDepthRange(1,1) trick).
	int lastViewportX;
	int lastViewportFlippedY;
	int lastViewportWidth;
	int lastViewportHeight;

	// portal/mirror eye-space clip plane, mirroring
	// renderer1's glClipPlane(GL_CLIP_PLANE0, ...)/glEnable(GL_CLIP_PLANE0)
	// (RB_BeginDrawingView in tr_backend.c, no-op'd here). Vulkan has no
	// fixed-function clip-plane equivalent that's portable without extra
	// device features, so this is applied as a per-fragment discard in
	// the scene shaders instead (see tr3d*.frag.glsl).
	qboolean clipPlaneEnabled;
	float clipPlane[4];

	VkBuffer sceneVertexArenaBuffer;
	VkDeviceMemory sceneVertexArenaMemory;
	void *sceneVertexArenaMapped;
	VkDeviceSize sceneVertexArenaSize;
	VkDeviceSize sceneVertexArenaOffset;

	VkBuffer sceneIndexArenaBuffer;
	VkDeviceMemory sceneIndexArenaMemory;
	void *sceneIndexArenaMapped;
	VkDeviceSize sceneIndexArenaSize;
	VkDeviceSize sceneIndexArenaOffset;

	qboolean initialized;
	uint32_t currentImageIndex;
} vkContext_t;

static vkContext_t vk;

// vertex format fed into the UI pipeline: screen-space pixel
// position, texture UV, and final (already colorGen/alphaGen-baked) RGBA8
// vertex color.
typedef struct
{
	float pos[2];
	float uv[2];
	byte color[4];
} vkUIVertex_t;

// Sized generously: a single 2D shader batch can be up to SHADER_MAX_VERTEXES
// (10000) vertices, and a busy UI frame (console/loading text, HUD, menus)
// can issue dozens of such batches.
#define VK_UI_VERTEX_ARENA_SIZE (16 * 1024 * 1024)
#define VK_UI_INDEX_ARENA_SIZE  (4 * 1024 * 1024)

// vertex formats fed into the 3D scene pipelines.
typedef struct
{
	float pos[3];
	float uv[2];
	byte color[4];
} vk3DVertex_t;

typedef struct
{
	float pos[3];
	float uv0[2];
	float uv1[2];
	byte color[4];
} vk3DVertexLM_t;

#define VK_SCENE_VERTEX_ARENA_SIZE (32 * 1024 * 1024)
#define VK_SCENE_INDEX_ARENA_SIZE  (8 * 1024 * 1024)
#define VK_UI_MAX_DESCRIPTOR_SETS 2048

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

	// anisotropic filtering (r_ext_texture_filter_anisotropic)
	{
		VkPhysicalDeviceFeatures   supported;
		VkPhysicalDeviceProperties props;

		vkGetPhysicalDeviceFeatures(vk.physicalDevice, &supported);
		vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);

		vk.maxSamplerAnisotropy = 0.0f;
		if (supported.samplerAnisotropy)
		{
			deviceFeatures.samplerAnisotropy = VK_TRUE;
			vk.maxSamplerAnisotropy          = props.limits.maxSamplerAnisotropy;
		}
	}

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

/**
 * @brief Finds a device memory type index matching both the given
 * memory-type bitmask (from a VkMemoryRequirements) and the requested
 * property flags (host-visible, device-local, ...).
 */
static uint32_t VK_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
static VkShaderModule VK_CreateShaderModule(const unsigned char *code, unsigned int size);

/**
 * @brief creates the depth buffer (image + memory + view)
 * sized to match the current swapchain extent. Recreated alongside the
 * swapchain on resize (see VK_ResizeSwapchain).
 */
static qboolean VK_CreateDepthResources(void)
{
	VkImageCreateInfo     imageInfo;
	VkMemoryRequirements  memRequirements;
	VkMemoryAllocateInfo  allocInfo;
	VkImageViewCreateInfo viewInfo;

	// prefer a depth format with stencil (needed by stencil
	// shadow volumes, r_shadows 2). Apple GPUs expose D32_SFLOAT_S8_UINT
	// only; some others D24_UNORM_S8_UINT. Fall back to depth-only.
	{
		static const VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		VkFormatProperties    props;
		int                   i;

		vk.depthFormat     = VK_FORMAT_D32_SFLOAT;
		vk.depthHasStencil = qfalse;

		for (i = 0; i < (int)ARRAY_LEN(candidates); i++)
		{
			vkGetPhysicalDeviceFormatProperties(vk.physicalDevice, candidates[i], &props);
			if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				vk.depthFormat     = candidates[i];
				vk.depthHasStencil = qtrue;
				break;
			}
		}
	}

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = vk.swapchainExtent.width;
	imageInfo.extent.height = vk.swapchainExtent.height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = vk.depthFormat;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vk.depthImage), "vkCreateImage(depth)"))
	{
		return qfalse;
	}

	vkGetImageMemoryRequirements(vk.device, vk.depthImage, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vk.depthImageMemory), "vkAllocateMemory(depth)"))
	{
		return qfalse;
	}

	vkBindImageMemory(vk.device, vk.depthImage, vk.depthImageMemory, 0);

	Com_Memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                       = vk.depthImage;
	viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                      = vk.depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (vk.depthHasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.depthImageView), "vkCreateImageView(depth)"))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief offscreen color image the main render pass draws
 * into (same format/size as the swapchain so all existing pipelines stay
 * render-pass compatible). Sampled by the gamma pass; TRANSFER_SRC for
 * future screenshot readback.
 */
static qboolean VK_CreateSceneColorResources(void)
{
	VkImageCreateInfo     imageInfo;
	VkMemoryRequirements  memRequirements;
	VkMemoryAllocateInfo  allocInfo;
	VkImageViewCreateInfo viewInfo;

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = vk.swapchainExtent.width;
	imageInfo.extent.height = vk.swapchainExtent.height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = vk.swapchainFormat;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vk.sceneColorImage), "vkCreateImage(scene color)"))
	{
		return qfalse;
	}

	vkGetImageMemoryRequirements(vk.device, vk.sceneColorImage, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vk.sceneColorMemory), "vkAllocateMemory(scene color)"))
	{
		return qfalse;
	}

	vkBindImageMemory(vk.device, vk.sceneColorImage, vk.sceneColorMemory, 0);

	Com_Memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                       = vk.sceneColorImage;
	viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                      = vk.swapchainFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	return VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.sceneColorView), "vkCreateImageView(scene color)");
}

static qboolean VK_CreateRenderPassAndFramebuffers(void)
{
	VkAttachmentDescription colorAttachment;
	VkAttachmentReference   colorAttachmentRef;
	VkAttachmentDescription depthAttachment;
	VkAttachmentReference   depthAttachmentRef;
	VkAttachmentDescription attachments[2];
	VkSubpassDescription    subpass;
	VkSubpassDependency     dependencies[2];
	VkRenderPassCreateInfo  renderPassInfo;
	VkFramebufferCreateInfo framebufferInfo;
	VkImageView             fbAttachments[2];
	uint32_t                i;

	if (!VK_CreateDepthResources() || !VK_CreateSceneColorResources())
	{
		return qfalse;
	}

	// --- main (scene) render pass: offscreen color + depth/stencil
	Com_Memset(&colorAttachment, 0, sizeof(colorAttachment));
	colorAttachment.format         = vk.swapchainFormat;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&colorAttachmentRef, 0, sizeof(colorAttachmentRef));
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Com_Memset(&depthAttachment, 0, sizeof(depthAttachment));
	depthAttachment.format         = vk.depthFormat;
	depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = vk.depthHasStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Com_Memset(&depthAttachmentRef, 0, sizeof(depthAttachmentRef));
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[0] = colorAttachment;
	attachments[1] = depthAttachment;

	Com_Memset(&subpass, 0, sizeof(subpass));
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	Com_Memset(dependencies, 0, sizeof(dependencies));
	// previous frame's gamma pass read the scene image -> this frame writes it
	dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass    = 0;
	dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// scene color writes -> gamma pass samples it
	dependencies[1].srcSubpass    = 0;
	dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	Com_Memset(&renderPassInfo, 0, sizeof(renderPassInfo));
	renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments    = attachments;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies   = dependencies;

	if (!VK_CheckResult(vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.renderPass), "vkCreateRenderPass"))
	{
		return qfalse;
	}

	fbAttachments[0] = vk.sceneColorView;
	fbAttachments[1] = vk.depthImageView;

	Com_Memset(&framebufferInfo, 0, sizeof(framebufferInfo));
	framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass      = vk.renderPass;
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.pAttachments    = fbAttachments;
	framebufferInfo.width           = vk.swapchainExtent.width;
	framebufferInfo.height          = vk.swapchainExtent.height;
	framebufferInfo.layers          = 1;

	if (!VK_CheckResult(vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.sceneFramebuffer), "vkCreateFramebuffer(scene)"))
	{
		return qfalse;
	}

	// --- present render pass: swapchain color only, fully overwritten by
	// the gamma pass
	colorAttachment.loadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	subpass.pDepthStencilAttachment = NULL;

	Com_Memset(dependencies, 0, sizeof(dependencies));
	dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass    = 0;
	dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments    = &colorAttachment;
	renderPassInfo.dependencyCount = 1;

	if (!VK_CheckResult(vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.presentRenderPass), "vkCreateRenderPass(present)"))
	{
		return qfalse;
	}

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		fbAttachments[0] = vk.swapchainImageViews[i];

		framebufferInfo.renderPass      = vk.presentRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments    = fbAttachments;

		if (!VK_CheckResult(vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.swapchainFramebuffers[i]), "vkCreateFramebuffer"))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief gamma pass shader modules + pipeline layout (one
 * combined image sampler, gamma/overbright push constants). Created once.
 */
static qboolean VK_CreateGammaPipelineLayout(void)
{
	VkPushConstantRange        pushRange;
	VkPipelineLayoutCreateInfo layoutInfo;

	vk.gammaVertModule = VK_CreateShaderModule(gamma_vert_spv, gamma_vert_spv_SIZE);
	vk.gammaFragModule = VK_CreateShaderModule(gamma_frag_spv, gamma_frag_spv_SIZE);
	if (vk.gammaVertModule == VK_NULL_HANDLE || vk.gammaFragModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	Com_Memset(&pushRange, 0, sizeof(pushRange));
	pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.size       = 2 * sizeof(float);

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &vk.uiDescriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushRange;

	vk.gammaValue      = 1.0f;
	vk.overbrightScale = 1.0f;

	return VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.gammaPipelineLayout), "vkCreatePipelineLayout(gamma)");
}

/**
 * @brief per-swapchain present resources - the gamma pipeline
 * (bound to presentRenderPass) and the descriptor set sampling the scene
 * color image. Recreated with the swapchain.
 */
static qboolean VK_CreatePresentResources(void)
{
	VkPipelineShaderStageCreateInfo        stages[2];
	VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo      viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo   multisampling;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlending;
	VkDynamicState                         dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo       dynamicState;
	VkGraphicsPipelineCreateInfo           pipelineInfo;
	VkDescriptorSetAllocateInfo            dsAllocInfo;
	VkDescriptorImageInfo                  imageDescInfo;
	VkWriteDescriptorSet                   writeSet;

	Com_Memset(stages, 0, sizeof(stages));
	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vk.gammaVertModule;
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = vk.gammaFragModule;
	stages[1].pName  = "main";

	Com_Memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	Com_Memset(&inputAssembly, 0, sizeof(inputAssembly));
	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	Com_Memset(&viewportState, 0, sizeof(viewportState));
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	Com_Memset(&rasterizer, 0, sizeof(rasterizer));
	rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode    = VK_CULL_MODE_NONE;
	rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth   = 1.0f;

	Com_Memset(&multisampling, 0, sizeof(multisampling));
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	Com_Memset(&colorBlendAttachment, 0, sizeof(colorBlendAttachment));
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	Com_Memset(&colorBlending, 0, sizeof(colorBlending));
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	Com_Memset(&dynamicState, 0, sizeof(dynamicState));
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = vk.gammaPipelineLayout;
	pipelineInfo.renderPass          = vk.presentRenderPass;
	pipelineInfo.subpass             = 0;

	if (!VK_CheckResult(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vk.gammaPipeline), "vkCreateGraphicsPipelines(gamma)"))
	{
		return qfalse;
	}

	Com_Memset(&dsAllocInfo, 0, sizeof(dsAllocInfo));
	dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo.descriptorPool     = vk.uiDescriptorPool;
	dsAllocInfo.descriptorSetCount = 1;
	dsAllocInfo.pSetLayouts        = &vk.uiDescriptorSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &dsAllocInfo, &vk.sceneColorDescriptor), "vkAllocateDescriptorSets(scene color)"))
	{
		return qfalse;
	}

	Com_Memset(&imageDescInfo, 0, sizeof(imageDescInfo));
	imageDescInfo.sampler     = vk.uiSamplerClamp;
	imageDescInfo.imageView   = vk.sceneColorView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&writeSet, 0, sizeof(writeSet));
	writeSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.dstSet          = vk.sceneColorDescriptor;
	writeSet.dstBinding      = 0;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeSet.pImageInfo      = &imageDescInfo;

	vkUpdateDescriptorSets(vk.device, 1, &writeSet, 0, NULL);

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

/**
 * @brief Finds a device memory type index matching both the given
 * memory-type bitmask (from a VkMemoryRequirements) and the requested
 * property flags (host-visible, device-local, ...).
 */
static uint32_t VK_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	uint32_t                         i;

	vkGetPhysicalDeviceMemoryProperties(vk.physicalDevice, &memProperties);

	for (i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	Ren_Print("Vulkan: failed to find a suitable memory type\n");
	return 0;
}

/**
 * @brief Allocates a VkBuffer + backing VkDeviceMemory pair with the given
 * usage/property flags.
 */
static qboolean VK_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory)
{
	VkBufferCreateInfo   bufferInfo;
	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo allocInfo;

	Com_Memset(&bufferInfo, 0, sizeof(bufferInfo));
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateBuffer(vk.device, &bufferInfo, NULL, buffer), "vkCreateBuffer"))
	{
		return qfalse;
	}

	vkGetBufferMemoryRequirements(vk.device, *buffer, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, memory), "vkAllocateMemory"))
	{
		vkDestroyBuffer(vk.device, *buffer, NULL);
		*buffer = VK_NULL_HANDLE;
		return qfalse;
	}

	vkBindBufferMemory(vk.device, *buffer, *memory, 0);

	return qtrue;
}

/**
 * @brief Begins a single-use primary command buffer for one-shot transfer
 * operations (texture upload layout transitions/copies). The caller must
 * pair this with VK_EndOneShotCommands().
 */
static VkCommandBuffer VK_BeginOneShotCommands(void)
{
	VkCommandBufferAllocateInfo allocInfo;
	VkCommandBufferBeginInfo    beginInfo;
	VkCommandBuffer             commandBuffer;

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = vk.commandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(vk.device, &allocInfo, &commandBuffer);

	Com_Memset(&beginInfo, 0, sizeof(beginInfo));
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

/**
 * @brief Ends and submits a one-shot command buffer created with
 * VK_BeginOneShotCommands(), waiting (vkQueueWaitIdle) for it to complete
 * before freeing it. Synchronous and only meant for load-time texture
 * uploads, not per-frame work.
 */
static void VK_EndOneShotCommands(VkCommandBuffer commandBuffer)
{
	VkSubmitInfo submitInfo;

	vkEndCommandBuffer(commandBuffer);

	Com_Memset(&submitInfo, 0, sizeof(submitInfo));
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk.graphicsQueue);

	vkFreeCommandBuffers(vk.device, vk.commandPool, 1, &commandBuffer);
}

/**
 * @brief Creates the two samplers used by UI textures (one per
 * wrap mode); shaders always sample with LINEAR filtering since real
 * mipmaps aren't generated yet.
 */
/**
 * @brief creates one sampler for mipmapped images from the
 * current GL-style texture mode (renderer1's GL_TextureMode() /
 * glTexParameter MIN/MAG filter + GL_TEXTURE_MAX_ANISOTROPY_EXT).
 * Non-mipmap min filters (GL_NEAREST/GL_LINEAR) clamp to level 0 via
 * maxLod = 0.25, the spec-recommended GL emulation.
 */
static VkSampler VK_CreateMipSampler(VkSamplerAddressMode addressMode)
{
	VkSamplerCreateInfo samplerInfo;
	VkSampler           sampler = VK_NULL_HANDLE;
	qboolean            useMips = qtrue;

	Com_Memset(&samplerInfo, 0, sizeof(samplerInfo));
	samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = (vk.texFilterMag == GL_NEAREST) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	switch (vk.texFilterMin)
	{
	case GL_NEAREST:
		samplerInfo.minFilter  = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		useMips                = qfalse;
		break;
	case GL_LINEAR:
		samplerInfo.minFilter  = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		useMips                = qfalse;
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		samplerInfo.minFilter  = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		samplerInfo.minFilter  = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		samplerInfo.minFilter  = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
	default:
		samplerInfo.minFilter  = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	}

	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = useMips ? VK_LOD_CLAMP_NONE : 0.25f;

	if (useMips && vk.maxSamplerAnisotropy > 1.0f && r_extTextureFilterAnisotropic->value > 1.0f)
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy    = Com_Clamp(1.0f, vk.maxSamplerAnisotropy, r_extTextureFilterAnisotropic->value);
	}

	if (!VK_CheckResult(vkCreateSampler(vk.device, &samplerInfo, NULL, &sampler), "vkCreateSampler(mip)"))
	{
		return VK_NULL_HANDLE;
	}

	return sampler;
}

static qboolean VK_CreateMipSamplers(void)
{
	vk.mipSamplerRepeat = VK_CreateMipSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
	vk.mipSamplerClamp  = VK_CreateMipSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	return (vk.mipSamplerRepeat != VK_NULL_HANDLE && vk.mipSamplerClamp != VK_NULL_HANDLE) ? qtrue : qfalse;
}

static VkSampler VK_SamplerForImage(qboolean mipmap, int wrapClampMode)
{
	if (mipmap)
	{
		return (wrapClampMode == GL_REPEAT) ? vk.mipSamplerRepeat : vk.mipSamplerClamp;
	}

	return (wrapClampMode == GL_REPEAT) ? vk.uiSamplerRepeat : vk.uiSamplerClamp;
}

static qboolean VK_CreateUISamplers(void)
{
	VkSamplerCreateInfo samplerInfo;

	Com_Memset(&samplerInfo, 0, sizeof(samplerInfo));
	samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = VK_FILTER_LINEAR;
	samplerInfo.minFilter    = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.maxLod       = 0.25f;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	if (!VK_CheckResult(vkCreateSampler(vk.device, &samplerInfo, NULL, &vk.uiSamplerRepeat), "vkCreateSampler(repeat)"))
	{
		return qfalse;
	}

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	if (!VK_CheckResult(vkCreateSampler(vk.device, &samplerInfo, NULL, &vk.uiSamplerClamp), "vkCreateSampler(clamp)"))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Creates the descriptor pool that every 2D image's descriptor set
 * (one combined-image-sampler each) is allocated from.
 */
static qboolean VK_CreateUIDescriptorPool(void)
{
	VkDescriptorPoolSize       poolSize;
	VkDescriptorPoolCreateInfo poolInfo;

	Com_Memset(&poolSize, 0, sizeof(poolSize));
	poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = VK_UI_MAX_DESCRIPTOR_SETS;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	poolInfo.maxSets       = VK_UI_MAX_DESCRIPTOR_SETS;

	return VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.uiDescriptorPool), "vkCreateDescriptorPool");
}

/**
 * @brief Creates a VkShaderModule from an embedded SPIR-V byte array.
 */
static VkShaderModule VK_CreateShaderModule(const unsigned char *code, unsigned int size)
{
	VkShaderModuleCreateInfo createInfo;
	VkShaderModule           module = VK_NULL_HANDLE;

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode    = (const uint32_t *)code;

	VK_CheckResult(vkCreateShaderModule(vk.device, &createInfo, NULL, &module), "vkCreateShaderModule");

	return module;
}

/**
 * @brief Maps the engine's GLS_SRCBLEND_* / GLS_DSTBLEND_* state bits to the
 * matching VkBlendFactor, mirroring GL_State()'s switch in tr_backend.c.
 */
static VkBlendFactor VK_SrcBlendFactor(unsigned long stateBits)
{
	switch (stateBits & GLS_SRCBLEND_BITS)
	{
	case GLS_SRCBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
	case GLS_SRCBLEND_ONE: return VK_BLEND_FACTOR_ONE;
	case GLS_SRCBLEND_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
	case GLS_SRCBLEND_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case GLS_SRCBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
	case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case GLS_SRCBLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
	case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case GLS_SRCBLEND_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	default: return VK_BLEND_FACTOR_ONE;
	}
}

static VkBlendFactor VK_DstBlendFactor(unsigned long stateBits)
{
	switch (stateBits & GLS_DSTBLEND_BITS)
	{
	case GLS_DSTBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
	case GLS_DSTBLEND_ONE: return VK_BLEND_FACTOR_ONE;
	case GLS_DSTBLEND_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
	case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case GLS_DSTBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
	case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case GLS_DSTBLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
	case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	default: return VK_BLEND_FACTOR_ZERO;
	}
}

/**
 * @brief Creates the descriptor set layout (single combined-image-sampler),
 * push-constant (scale/offset + alphaTest) pipeline layout, and compiles
 * the shader modules shared by every lazily-created UI pipeline variant
 * (see VK_GetUIPipeline()). Variants now honor each
 * shader stage's real stateBits (alpha test, depth test/func/write)
 * instead of always forcing RB_SetGL2D's fixed blend-only state - this is
 * required for tricks like the automap's circular-mask (alphafunc GE128 +
 * depthFunc equal) in cg_commandmap.c to render correctly.
 */
static qboolean VK_CreateUIPipeline(void)
{
	VkDescriptorSetLayoutBinding    binding;
	VkDescriptorSetLayoutCreateInfo layoutInfo;
	VkPushConstantRange             pushConstantRange;
	VkPipelineLayoutCreateInfo      pipelineLayoutInfo;

	Com_Memset(&binding, 0, sizeof(binding));
	binding.binding         = 0;
	binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &binding;

	if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &layoutInfo, NULL, &vk.uiDescriptorSetLayout), "vkCreateDescriptorSetLayout"))
	{
		return qfalse;
	}

	Com_Memset(&pushConstantRange, 0, sizeof(pushConstantRange));
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(float) * 8; // vec2 scale + vec2 offset + vec4 alphaTest

	Com_Memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = 1;
	pipelineLayoutInfo.pSetLayouts            = &vk.uiDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, NULL, &vk.uiPipelineLayout), "vkCreatePipelineLayout"))
	{
		return qfalse;
	}

	vk.uiVertModule = VK_CreateShaderModule(ui_vert_spv, ui_vert_spv_SIZE);
	vk.uiFragModule = VK_CreateShaderModule(ui_frag_spv, ui_frag_spv_SIZE);

	if (vk.uiVertModule == VK_NULL_HANDLE || vk.uiFragModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Creates one 2D/UI pipeline variant for the given stateBits,
 * honoring depth test/write/func and blend state exactly like the
 * scene pipelines (see VK_CreateScenePipelineVariant()).
 */
static VkPipeline VK_CreateUIPipelineVariant(unsigned long stateBits)
{
	VkPipelineShaderStageCreateInfo        stages[2];
	VkVertexInputBindingDescription        vertexBinding;
	VkVertexInputAttributeDescription      vertexAttributes[3];
	VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo      viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo   multisampling;
	VkPipelineDepthStencilStateCreateInfo  depthStencil;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlending;
	VkDynamicState                         dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo       dynamicState;
	VkGraphicsPipelineCreateInfo           pipelineInfo;
	VkPipeline                             pipeline = VK_NULL_HANDLE;

	Com_Memset(stages, 0, sizeof(stages));
	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vk.uiVertModule;
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = vk.uiFragModule;
	stages[1].pName  = "main";

	Com_Memset(&vertexBinding, 0, sizeof(vertexBinding));
	vertexBinding.binding   = 0;
	vertexBinding.stride    = sizeof(vkUIVertex_t);
	vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	Com_Memset(vertexAttributes, 0, sizeof(vertexAttributes));
	vertexAttributes[0].location = 0;
	vertexAttributes[0].binding  = 0;
	vertexAttributes[0].format   = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[0].offset   = offsetof(vkUIVertex_t, pos);
	vertexAttributes[1].location = 1;
	vertexAttributes[1].binding  = 0;
	vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[1].offset   = offsetof(vkUIVertex_t, uv);
	vertexAttributes[2].location = 2;
	vertexAttributes[2].binding  = 0;
	vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
	vertexAttributes[2].offset   = offsetof(vkUIVertex_t, color);

	Com_Memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
	vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount   = 1;
	vertexInputInfo.pVertexBindingDescriptions      = &vertexBinding;
	vertexInputInfo.vertexAttributeDescriptionCount = 3;
	vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributes;

	Com_Memset(&inputAssembly, 0, sizeof(inputAssembly));
	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	Com_Memset(&viewportState, 0, sizeof(viewportState));
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	Com_Memset(&rasterizer, 0, sizeof(rasterizer));
	rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode    = VK_CULL_MODE_NONE; // RB_SetGL2D: glDisable(GL_CULL_FACE)
	rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth   = 1.0f;

	Com_Memset(&multisampling, 0, sizeof(multisampling));
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	Com_Memset(&depthStencil, 0, sizeof(depthStencil));
	depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable  = (stateBits & GLS_DEPTHTEST_DISABLE) ? VK_FALSE : VK_TRUE;
	depthStencil.depthWriteEnable = (stateBits & GLS_DEPTHMASK_TRUE) ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp   = (stateBits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;

	Com_Memset(&colorBlendAttachment, 0, sizeof(colorBlendAttachment));
	colorBlendAttachment.blendEnable         = (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstColorBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstAlphaBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	Com_Memset(&colorBlending, 0, sizeof(colorBlending));
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	Com_Memset(&dynamicState, 0, sizeof(dynamicState));
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = vk.uiPipelineLayout;
	pipelineInfo.renderPass          = vk.renderPass;
	pipelineInfo.subpass             = 0;

	if (!VK_CheckResult(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline), "vkCreateGraphicsPipelines(ui)"))
	{
		return VK_NULL_HANDLE;
	}

	return pipeline;
}

/**
 * @brief Returns the cached 2D/UI pipeline for this stateBits value,
 * creating and caching it on first use. Returns VK_NULL_HANDLE if the
 * cache is full or creation failed (caller should skip the draw).
 */
static VkPipeline VK_GetUIPipeline(unsigned long stateBits)
{
	int        i;
	VkPipeline pipeline;

	for (i = 0; i < vk.numUIPipelines; i++)
	{
		if (vk.uiPipelines[i].stateBits == stateBits)
		{
			return vk.uiPipelines[i].pipeline;
		}
	}

	if (vk.numUIPipelines >= VK_MAX_SCENE_PIPELINES)
	{
		Ren_Print("Vulkan: UI pipeline cache full, dropping draw\n");
		return VK_NULL_HANDLE;
	}

	pipeline = VK_CreateUIPipelineVariant(stateBits);
	if (pipeline == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	vk.uiPipelines[vk.numUIPipelines].stateBits = stateBits;
	vk.uiPipelines[vk.numUIPipelines].pipeline  = pipeline;
	vk.numUIPipelines++;

	return pipeline;
}

/**
 * @brief Creates the persistently-mapped vertex/index arena buffers used
 * by VK_Draw2D() for every 2D draw this frame.
 */
static qboolean VK_CreateUIArenaBuffers(void)
{
	vk.vertexArenaSize = VK_UI_VERTEX_ARENA_SIZE;
	vk.indexArenaSize  = VK_UI_INDEX_ARENA_SIZE;

	if (!VK_CreateBuffer(vk.vertexArenaSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.vertexArenaBuffer, &vk.vertexArenaMemory))
	{
		return qfalse;
	}
	if (!VK_CreateBuffer(vk.indexArenaSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.indexArenaBuffer, &vk.indexArenaMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, vk.vertexArenaMemory, 0, vk.vertexArenaSize, 0, &vk.vertexArenaMapped);
	vkMapMemory(vk.device, vk.indexArenaMemory, 0, vk.indexArenaSize, 0, &vk.indexArenaMapped);

	return qtrue;
}

/**
 * @brief persistently-mapped vertex/index arena buffers for
 * 3D scene draws, analogous to the UI arena buffers but with their own
 * (larger) backing storage and vertex formats.
 */
static qboolean VK_CreateSceneArenaBuffers(void)
{
	vk.sceneVertexArenaSize = VK_SCENE_VERTEX_ARENA_SIZE;
	vk.sceneIndexArenaSize  = VK_SCENE_INDEX_ARENA_SIZE;

	if (!VK_CreateBuffer(vk.sceneVertexArenaSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.sceneVertexArenaBuffer, &vk.sceneVertexArenaMemory))
	{
		return qfalse;
	}
	if (!VK_CreateBuffer(vk.sceneIndexArenaSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.sceneIndexArenaBuffer, &vk.sceneIndexArenaMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, vk.sceneVertexArenaMemory, 0, vk.sceneVertexArenaSize, 0, &vk.sceneVertexArenaMapped);
	vkMapMemory(vk.device, vk.sceneIndexArenaMemory, 0, vk.sceneIndexArenaSize, 0, &vk.sceneIndexArenaMapped);

	return qtrue;
}

/**
 * @brief creates the two pipeline layouts used by static 3D
 * geometry draws - one with a single descriptor set (base texture only,
 * used by RB_StageIteratorGeneric/VertexLitTexture) and one with two
 * descriptor sets (base + lightmap, used by RB_StageIteratorLightmappedMultitexture
 * and 2-stage shaders in the generic path). Both reuse
 * vk.uiDescriptorSetLayout (single combined-image-sampler) since every
 * image already gets one such descriptor set from VK_CreateImageTexture.
 * Also pre-compiles the shader modules shared by every lazily-created
 * pipeline variant.
 */
static qboolean VK_CreateScenePipelineLayouts(void)
{
	VkPushConstantRange        pushConstantRange;
	VkPipelineLayoutCreateInfo layoutInfo;
	VkDescriptorSetLayout      twoSetLayouts[2];

	Com_Memset(&pushConstantRange, 0, sizeof(pushConstantRange));
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(float) * 48; // mat4 mvp + mat4 modelView + vec4 alphaTest + vec4 fogColor + vec4 fogParams + vec4 clipPlane

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &vk.uiDescriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushConstantRange;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.scenePipelineLayout), "vkCreatePipelineLayout(scene)"))
	{
		return qfalse;
	}

	twoSetLayouts[0] = vk.uiDescriptorSetLayout;
	twoSetLayouts[1] = vk.uiDescriptorSetLayout;

	layoutInfo.setLayoutCount = 2;
	layoutInfo.pSetLayouts    = twoSetLayouts;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.sceneLMPipelineLayout), "vkCreatePipelineLayout(sceneLM)"))
	{
		return qfalse;
	}

	vk.sceneVertModule   = VK_CreateShaderModule(tr3d_vert_spv, tr3d_vert_spv_SIZE);
	vk.sceneFragModule   = VK_CreateShaderModule(tr3d_frag_spv, tr3d_frag_spv_SIZE);
	vk.sceneLMVertModule = VK_CreateShaderModule(tr3d_lm_vert_spv, tr3d_lm_vert_spv_SIZE);
	vk.sceneLMFragModule = VK_CreateShaderModule(tr3d_lm_frag_spv, tr3d_lm_frag_spv_SIZE);

	if (vk.sceneVertModule == VK_NULL_HANDLE || vk.sceneFragModule == VK_NULL_HANDLE ||
	    vk.sceneLMVertModule == VK_NULL_HANDLE || vk.sceneLMFragModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Creates one 3D scene pipeline variant for the given (stateBits,
 * cullType) pair. `lightmapped` selects the vertex format/shaders/pipeline
 * layout (vk3DVertex_t + tr3d.* vs. vk3DVertexLM_t + tr3d_lm.*).
 */
static VkPipeline VK_CreateScenePipelineVariant(unsigned long stateBits, int cullType, qboolean lightmapped, qboolean polygonOffset, int special)
{
	VkPipelineShaderStageCreateInfo        stages[2];
	VkVertexInputBindingDescription        vertexBinding;
	VkVertexInputAttributeDescription      vertexAttributes[4];
	uint32_t                               attributeCount;
	VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo      viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo   multisampling;
	VkPipelineDepthStencilStateCreateInfo  depthStencil;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlending;
	VkDynamicState                         dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo       dynamicState;
	VkGraphicsPipelineCreateInfo           pipelineInfo;
	VkPipeline                             pipeline = VK_NULL_HANDLE;
	qboolean                               cullFront;
	uint32_t                               ai;

	Com_Memset(stages, 0, sizeof(stages));
	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = lightmapped ? vk.sceneLMVertModule : vk.sceneVertModule;
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = lightmapped ? vk.sceneLMFragModule : vk.sceneFragModule;
	stages[1].pName  = "main";

	Com_Memset(&vertexBinding, 0, sizeof(vertexBinding));
	vertexBinding.binding   = 0;
	vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	Com_Memset(vertexAttributes, 0, sizeof(vertexAttributes));
	if (lightmapped)
	{
		vertexBinding.stride = sizeof(vk3DVertexLM_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertexLM_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertexLM_t, uv0);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[2].offset   = offsetof(vk3DVertexLM_t, uv1);
		vertexAttributes[3].location = 3;
		vertexAttributes[3].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[3].offset   = offsetof(vk3DVertexLM_t, color);
		attributeCount               = 4;
	}
	else
	{
		vertexBinding.stride = sizeof(vk3DVertex_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertex_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertex_t, uv);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[2].offset   = offsetof(vk3DVertex_t, color);
		attributeCount               = 3;
	}
	for (ai = 0; ai < attributeCount; ai++)
	{
		vertexAttributes[ai].binding = 0;
	}

	Com_Memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
	vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount   = 1;
	vertexInputInfo.pVertexBindingDescriptions      = &vertexBinding;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
	vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributes;

	Com_Memset(&inputAssembly, 0, sizeof(inputAssembly));
	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	Com_Memset(&viewportState, 0, sizeof(viewportState));
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	// GL_Cull: cullFront = (cullType == CT_FRONT_SIDED); glCullFace(cullFront ? GL_FRONT : GL_BACK).
	// The Vulkan clip-space Y-flip baked into VK_SetProjectionMatrix cancels
	// out against Vulkan's inherently Y-down viewport mapping, so winding
	// order as seen by the rasterizer matches GL's - no frontFace flip needed.
	cullFront = (cullType == CT_FRONT_SIDED);

	Com_Memset(&rasterizer, 0, sizeof(rasterizer));
	rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode    = cullType == CT_TWO_SIDED ? VK_CULL_MODE_NONE : (cullFront ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT);
	rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth   = 1.0f;

	// shader->polygonOffset (decals, marks): renderer1 enables
	// GL_POLYGON_OFFSET_FILL with glPolygonOffset(r_offsetFactor,
	// r_offsetUnits). Baked into the pipeline; both cvars are CVAR_CHEAT
	// and effectively never change at runtime.
	if (polygonOffset)
	{
		rasterizer.depthBiasEnable         = VK_TRUE;
		rasterizer.depthBiasSlopeFactor    = r_offsetFactor->value;
		rasterizer.depthBiasConstantFactor = r_offsetUnits->value;
	}

	Com_Memset(&multisampling, 0, sizeof(multisampling));
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	Com_Memset(&depthStencil, 0, sizeof(depthStencil));
	depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable  = (stateBits & GLS_DEPTHTEST_DISABLE) ? VK_FALSE : VK_TRUE;
	depthStencil.depthWriteEnable = (stateBits & GLS_DEPTHMASK_TRUE) ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp   = (stateBits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;

	Com_Memset(&colorBlendAttachment, 0, sizeof(colorBlendAttachment));
	colorBlendAttachment.blendEnable         = (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstColorBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstAlphaBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	Com_Memset(&colorBlending, 0, sizeof(colorBlending));
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	// stencil shadow volume states (see VK_SPECIAL_*).
	if (special == VK_SPECIAL_SHADOW_INCR || special == VK_SPECIAL_SHADOW_DECR)
	{
		VkStencilOpState op;

		Com_Memset(&op, 0, sizeof(op));
		op.failOp      = VK_STENCIL_OP_KEEP;
		op.depthFailOp = VK_STENCIL_OP_KEEP;
		op.passOp      = (special == VK_SPECIAL_SHADOW_INCR) ? VK_STENCIL_OP_INCREMENT_AND_CLAMP : VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		op.compareOp   = VK_COMPARE_OP_ALWAYS;
		op.compareMask = 255;
		op.writeMask   = 255;
		op.reference   = 1;

		depthStencil.stencilTestEnable = VK_TRUE;
		depthStencil.front             = op;
		depthStencil.back              = op;

		colorBlendAttachment.blendEnable    = VK_FALSE;
		colorBlendAttachment.colorWriteMask = 0;
	}
	else if (special == VK_SPECIAL_SHADOW_DARKEN)
	{
		VkStencilOpState op;

		Com_Memset(&op, 0, sizeof(op));
		op.failOp      = VK_STENCIL_OP_KEEP;
		op.depthFailOp = VK_STENCIL_OP_KEEP;
		op.passOp      = VK_STENCIL_OP_KEEP;
		op.compareOp   = VK_COMPARE_OP_NOT_EQUAL;
		op.compareMask = 255;
		op.writeMask   = 0;
		op.reference   = 0;

		depthStencil.stencilTestEnable = VK_TRUE;
		depthStencil.front             = op;
		depthStencil.back              = op;
	}

	Com_Memset(&dynamicState, 0, sizeof(dynamicState));
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = lightmapped ? vk.sceneLMPipelineLayout : vk.scenePipelineLayout;
	pipelineInfo.renderPass          = vk.renderPass;
	pipelineInfo.subpass             = 0;

	if (!VK_CheckResult(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline), "vkCreateGraphicsPipelines(scene)"))
	{
		return VK_NULL_HANDLE;
	}

	return pipeline;
}

/**
 * @brief Returns the cached pipeline for this (stateBits, cullType) pair,
 * creating and caching it on first use. Returns VK_NULL_HANDLE if the
 * cache is full or creation failed (caller should skip the draw).
 */
/**
 * @brief Mirrors GL_Cull()'s backEnd.viewParms.isMirror front/back flip
 * (tr_backend.c): mirrored views have inverted winding, so front/back
 * culling must be swapped to match what the real GL renderer does via
 * glCullFace(). Two-sided shaders are unaffected. Flipping cullType here
 * (rather than baking isMirror into the pipeline cache key) keeps the
 * cache correct automatically, since a flipped cullType is itself a
 * distinct, correctly-cached pipeline variant.
 */
static int VK_MirrorCullType(int cullType)
{
	if (!backEnd.viewParms.isMirror)
	{
		return cullType;
	}

	if (cullType == CT_FRONT_SIDED)
	{
		return CT_BACK_SIDED;
	}
	if (cullType == CT_BACK_SIDED)
	{
		return CT_FRONT_SIDED;
	}

	return cullType;
}

static VkPipeline VK_GetScenePipeline(unsigned long stateBits, int cullType, qboolean lightmapped, qboolean polygonOffset, int special)
{
	vkScenePipelineEntry_t *entries = lightmapped ? vk.sceneLMPipelines : vk.scenePipelines;
	int                    *count   = lightmapped ? &vk.numSceneLMPipelines : &vk.numScenePipelines;
	int                    i;
	VkPipeline             pipeline;

	for (i = 0; i < *count; i++)
	{
		if (entries[i].stateBits == stateBits && entries[i].cullType == cullType && entries[i].polygonOffset == polygonOffset &&
		    entries[i].special == special)
		{
			return entries[i].pipeline;
		}
	}

	if (*count >= VK_MAX_SCENE_PIPELINES)
	{
		Ren_Print("Vulkan: scene pipeline cache full, dropping draw\n");
		return VK_NULL_HANDLE;
	}

	pipeline = VK_CreateScenePipelineVariant(stateBits, cullType, lightmapped, polygonOffset, special);
	if (pipeline == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	entries[*count].stateBits     = stateBits;
	entries[*count].cullType      = cullType;
	entries[*count].polygonOffset = polygonOffset;
	entries[*count].special       = special;
	entries[*count].pipeline      = pipeline;
	(*count)++;

	return pipeline;
}

/**
 * @brief Multiplies two column-major 4x4 matrices (GL convention: index =
 * col*4+row), out = a * b. `out` must not alias `a` or `b`.
 */
static void VK_Mat4Mul(float *out, const float *a, const float *b)
{
	int c, r, k;

	for (c = 0; c < 4; c++)
	{
		for (r = 0; r < 4; r++)
		{
			float sum = 0.0f;

			for (k = 0; k < 4; k++)
			{
				sum += a[k * 4 + r] * b[c * 4 + k];
			}
			out[c * 4 + r] = sum;
		}
	}
}

/**
 * @brief Captures the engine's GL-convention projection matrix (called
 * from the no-op'd glLoadMatrixf(backEnd.viewParms.projectionMatrix) call
 * site in SetViewportAndScissor()) and bakes in the GL -> Vulkan
 * clip-space correction: negate the Y row (Vulkan NDC Y maps to the
 * viewport opposite of GL's, so this keeps the final image right-side up)
 * and remap Z from GL's [-1,1] range to Vulkan's [0,1] range.
 */
void VK_SetProjectionMatrix(const float *m)
{
	int c;

	for (c = 0; c < 4; c++)
	{
		vk.projMatrix[c * 4 + 0] = m[c * 4 + 0];
		vk.projMatrix[c * 4 + 1] = -m[c * 4 + 1];
		vk.projMatrix[c * 4 + 2] = 0.5f * m[c * 4 + 2] + 0.5f * m[c * 4 + 3];
		vk.projMatrix[c * 4 + 3] = m[c * 4 + 3];
	}

	VK_Mat4Mul(vk.mvpMatrix, vk.projMatrix, vk.modelMatrix);
}

/**
 * @brief copies out the already-Vulkan-converted projection
 * matrix (used by flare rendering to save/restore the world projection
 * around its own screen-space ortho projection, mirroring renderer1's
 * glPushMatrix/glPopMatrix around glOrtho()).
 */
void VK_GetProjectionMatrix(float *m)
{
	Com_Memcpy(m, vk.projMatrix, sizeof(vk.projMatrix));
}

/**
 * @brief sets the Vulkan-space projection matrix directly,
 * bypassing the GL->Vulkan conversion VK_SetProjectionMatrix() applies.
 * Used to restore a matrix previously captured with
 * VK_GetProjectionMatrix(), which is already in Vulkan convention.
 */
void VK_SetRawProjectionMatrix(const float *m)
{
	Com_Memcpy(vk.projMatrix, m, sizeof(vk.projMatrix));
	VK_Mat4Mul(vk.mvpMatrix, vk.projMatrix, vk.modelMatrix);
}

/**
 * @brief Captures the engine's GL-convention modelview matrix (called from
 * the no-op'd glLoadMatrixf(backEnd.orientation.modelMatrix)/
 * glLoadMatrixf(backEnd.viewParms.world.modelMatrix) call sites in
 * tr_backend.c) and recomputes the cached MVP used by VK_Draw3D*().
 */
void VK_SetModelViewMatrix(const float *m)
{
	Com_Memcpy(vk.modelMatrix, m, sizeof(vk.modelMatrix));
	VK_Mat4Mul(vk.mvpMatrix, vk.projMatrix, vk.modelMatrix);
}

/**
 * @brief copies out the currently-set modelview matrix, so sky
 * rendering can temporarily zero its translation component (the classic
 * "infinite skybox stays centered on the camera" trick, matching
 * renderer1's glTranslatef(viewerOrigin) applied on top of an
 * already-loaded view matrix) and restore the original afterwards.
 */
void VK_GetModelViewMatrix(float *m)
{
	Com_Memcpy(m, vk.modelMatrix, sizeof(vk.modelMatrix));
}

/**
 * @brief records the current linear-fog parameters, called
 * from R_Fog() (tr_main.c) in place of the no-op'd glFog*() calls.
 * Picked up per-draw by VK_Draw3D()/VK_Draw3DLightmapped() and applied in
 * the fragment shader based on each vertex's eye-space depth.
 */
void VK_SetFog(const float *color, float start, float end)
{
	vk.fogEnabled  = qtrue;
	vk.fogColor[0] = color[0];
	vk.fogColor[1] = color[1];
	vk.fogColor[2] = color[2];
	vk.fogColor[3] = color[3];
	vk.fogStart    = start;
	vk.fogEnd      = end;

	vk.fogParamsValid = qtrue;
}

void VK_SetFogOff(void)
{
	vk.fogEnabled = qfalse;
}

/**
 * @brief Re-enables fog with the parameters from the last VK_SetFog(),
 * mirroring glEnable(GL_FOG) in R_FogOn() (renderer1 keeps the fog
 * parameters across glDisable/glEnable, so R_FogOff()/R_FogOn() pairs,
 * e.g. around dlight passes or per-stage noFog, must restore fog).
 */
void VK_SetFogOn(void)
{
	if (vk.fogParamsValid)
	{
		vk.fogEnabled = qtrue;
	}
}

/**
 * @brief records an eye-space portal/mirror clip-plane
 * equation (ax+by+cz+dw, same convention as GL_CLIP_PLANE0: fragments
 * with dot(eyePos, plane.xyz) + plane.w < 0 are clipped), called from
 * RB_BeginDrawingView() (tr_backend.c) in place of the no-op'd
 * glClipPlane()/glEnable(GL_CLIP_PLANE0) calls. Applied as a per-fragment
 * discard in the scene shaders since Vulkan has no portable
 * fixed-function clip-plane equivalent.
 */
void VK_SetPortalClip(const float *planeEq)
{
	vk.clipPlaneEnabled = qtrue;
	vk.clipPlane[0]     = planeEq[0];
	vk.clipPlane[1]     = planeEq[1];
	vk.clipPlane[2]     = planeEq[2];
	vk.clipPlane[3]     = planeEq[3];
}

void VK_ClearPortalClip(void)
{
	vk.clipPlaneEnabled = qfalse;
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

	// offscreen scene target + present pass resources
	if (vk.sceneColorDescriptor != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(vk.device, vk.uiDescriptorPool, 1, &vk.sceneColorDescriptor);
		vk.sceneColorDescriptor = VK_NULL_HANDLE;
	}
	if (vk.gammaPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(vk.device, vk.gammaPipeline, NULL);
		vk.gammaPipeline = VK_NULL_HANDLE;
	}
	if (vk.sceneFramebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(vk.device, vk.sceneFramebuffer, NULL);
		vk.sceneFramebuffer = VK_NULL_HANDLE;
	}
	if (vk.presentRenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(vk.device, vk.presentRenderPass, NULL);
		vk.presentRenderPass = VK_NULL_HANDLE;
	}
	if (vk.sceneColorView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(vk.device, vk.sceneColorView, NULL);
		vk.sceneColorView = VK_NULL_HANDLE;
	}
	if (vk.sceneColorImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk.device, vk.sceneColorImage, NULL);
		vk.sceneColorImage = VK_NULL_HANDLE;
	}
	if (vk.sceneColorMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, vk.sceneColorMemory, NULL);
		vk.sceneColorMemory = VK_NULL_HANDLE;
	}

	if (vk.depthImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(vk.device, vk.depthImageView, NULL);
		vk.depthImageView = VK_NULL_HANDLE;
	}
	if (vk.depthImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk.device, vk.depthImage, NULL);
		vk.depthImage = VK_NULL_HANDLE;
	}
	if (vk.depthImageMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, vk.depthImageMemory, NULL);
		vk.depthImageMemory = VK_NULL_HANDLE;
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

	// on first init this runs before the UI descriptor pool
	// and gamma layout exist; VK_Init creates the present resources then.
	if (vk.gammaPipelineLayout != VK_NULL_HANDLE && !VK_CreatePresentResources())
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

	// mipmapped-image samplers follow the current texture
	// mode (gl_filter_min/max persist across vid_restart, while
	// GL_TextureMode() only reruns when r_textureMode is modified).
	vk.texFilterMin = gl_filter_min;
	vk.texFilterMag = gl_filter_max;

	// UI/2D rendering pipeline + resources.
	if (!VK_CreateUISamplers() ||
	    !VK_CreateMipSamplers() ||
	    !VK_CreateUIDescriptorPool() ||
	    !VK_CreateUIPipeline() ||
	    !VK_CreateUIArenaBuffers())
	{
		return qfalse;
	}

	// static 3D geometry pipeline layouts + arena buffers.
	// Actual pipelines are created lazily per (stateBits, cullType) the
	// first time they're needed (see VK_GetScenePipeline/VK_GetSceneLMPipeline).
	if (!VK_CreateScenePipelineLayouts() ||
	    !VK_CreateSceneArenaBuffers())
	{
		return qfalse;
	}

	// gamma/present pass
	if (!VK_CreateGammaPipelineLayout() ||
	    !VK_CreatePresentResources())
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

	// UI/2D rendering resources.
	if (vk.device != VK_NULL_HANDLE)
	{
		if (vk.vertexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.vertexArenaMemory);
		}
		if (vk.indexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.indexArenaMemory);
		}
		if (vk.vertexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.vertexArenaBuffer, NULL);
		}
		if (vk.vertexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.vertexArenaMemory, NULL);
		}
		if (vk.indexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.indexArenaBuffer, NULL);
		}
		if (vk.indexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.indexArenaMemory, NULL);
		}
		{
			int pi;

			for (pi = 0; pi < vk.numUIPipelines; pi++)
			{
				vkDestroyPipeline(vk.device, vk.uiPipelines[pi].pipeline, NULL);
			}
		}
		if (vk.gammaPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.gammaPipelineLayout, NULL);
		}
		if (vk.gammaVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.gammaVertModule, NULL);
		}
		if (vk.gammaFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.gammaFragModule, NULL);
		}
		if (vk.uiPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.uiPipelineLayout, NULL);
		}
		if (vk.uiVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.uiVertModule, NULL);
		}
		if (vk.uiFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.uiFragModule, NULL);
		}
		if (vk.uiDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.uiDescriptorSetLayout, NULL);
		}
		if (vk.uiDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.uiDescriptorPool, NULL);
		}
		if (vk.uiSamplerRepeat != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.uiSamplerRepeat, NULL);
		}
		if (vk.mipSamplerRepeat != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerRepeat, NULL);
		}
		if (vk.mipSamplerClamp != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerClamp, NULL);
		}
		if (vk.uiSamplerClamp != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.uiSamplerClamp, NULL);
		}

		// static 3D geometry resources.
		if (vk.sceneVertexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.sceneVertexArenaMemory);
		}
		if (vk.sceneIndexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.sceneIndexArenaMemory);
		}
		if (vk.sceneVertexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.sceneVertexArenaBuffer, NULL);
		}
		if (vk.sceneVertexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.sceneVertexArenaMemory, NULL);
		}
		if (vk.sceneIndexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.sceneIndexArenaBuffer, NULL);
		}
		if (vk.sceneIndexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.sceneIndexArenaMemory, NULL);
		}
		{
			int pi;

			for (pi = 0; pi < vk.numScenePipelines; pi++)
			{
				vkDestroyPipeline(vk.device, vk.scenePipelines[pi].pipeline, NULL);
			}
			for (pi = 0; pi < vk.numSceneLMPipelines; pi++)
			{
				vkDestroyPipeline(vk.device, vk.sceneLMPipelines[pi].pipeline, NULL);
			}
		}
		if (vk.scenePipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.scenePipelineLayout, NULL);
		}
		if (vk.sceneLMPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.sceneLMPipelineLayout, NULL);
		}
		if (vk.sceneVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneVertModule, NULL);
		}
		if (vk.sceneFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneFragModule, NULL);
		}
		if (vk.sceneLMVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMVertModule, NULL);
		}
		if (vk.sceneLMFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMFragModule, NULL);
		}
	}

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
	VkClearValue             clearValues[2];

	if (!vk.initialized)
	{
		return;
	}

	// reset the per-frame vertex/index arenas.
	vk.vertexArenaOffset = 0;
	vk.indexArenaOffset  = 0;

	// same for the 3D scene arenas. lastBoundPipeline is shared
	// across UI and scene draws (viewport/scissor are set per-view, not
	// cached per-frame - see VK_SetSceneViewport(), called from
	// tr_backend.c's SetViewportAndScissor()).
	vk.sceneVertexArenaOffset = 0;
	vk.sceneIndexArenaOffset  = 0;
	vk.lastBoundPipeline      = VK_NULL_HANDLE;

	vkWaitForFences(vk.device, 1, &vk.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(vk.device, 1, &vk.inFlightFence);

	vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore, VK_NULL_HANDLE, &vk.currentImageIndex);

	vkResetCommandBuffer(vk.commandBuffer, 0);

	Com_Memset(&beginInfo, 0, sizeof(beginInfo));
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);

	Com_Memset(clearValues, 0, sizeof(clearValues));
	clearValues[0].color.float32[0]     = 0.0f;
	clearValues[0].color.float32[1]     = 0.0f;
	clearValues[0].color.float32[2]     = 0.0f;
	clearValues[0].color.float32[3]     = 1.0f;
	clearValues[1].depthStencil.depth   = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	Com_Memset(&renderPassInfo, 0, sizeof(renderPassInfo));
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = vk.renderPass;
	renderPassInfo.framebuffer       = vk.sceneFramebuffer; // offscreen scene target
	renderPassInfo.renderArea.extent = vk.swapchainExtent;
	renderPassInfo.clearValueCount   = 2;
	renderPassInfo.pClearValues      = clearValues;

	vkCmdBeginRenderPass(vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

/**
 * @brief TEMP DEBUG: dumps the current swapchain image to a PPM file for
 * offline visual inspection (reads our own rendered image data directly
 * from the GPU, not the user's display).
 */
static void VK_DebugDumpSwapchainImage(const char *path)
{
	VkCommandBufferAllocateInfo allocCmdInfo;
	VkCommandBuffer             cmd;
	VkCommandBufferBeginInfo    beginInfo;
	VkImageMemoryBarrier        barrier;
	VkBufferCreateInfo          bufInfo;
	VkBuffer                    readbackBuffer;
	VkMemoryRequirements        memReq;
	VkMemoryAllocateInfo        memAllocInfo;
	VkDeviceMemory              readbackMemory;
	VkBufferImageCopy           region;
	VkSubmitInfo                submitInfo;
	VkDeviceSize                bufSize = (VkDeviceSize)vk.swapchainExtent.width * vk.swapchainExtent.height * 4;
	void                        *mapped;
	FILE                        *f;
	uint32_t                    x, y;

	vkDeviceWaitIdle(vk.device);

	Com_Memset(&bufInfo, 0, sizeof(bufInfo));
	bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.size        = bufSize;
	bufInfo.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkCreateBuffer(vk.device, &bufInfo, NULL, &readbackBuffer);

	vkGetBufferMemoryRequirements(vk.device, readbackBuffer, &memReq);
	Com_Memset(&memAllocInfo, 0, sizeof(memAllocInfo));
	memAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize  = memReq.size;
	memAllocInfo.memoryTypeIndex = VK_FindMemoryType(memReq.memoryTypeBits,
	                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(vk.device, &memAllocInfo, NULL, &readbackMemory);
	vkBindBufferMemory(vk.device, readbackBuffer, readbackMemory, 0);

	Com_Memset(&allocCmdInfo, 0, sizeof(allocCmdInfo));
	allocCmdInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocCmdInfo.commandPool        = vk.commandPool;
	allocCmdInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocCmdInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(vk.device, &allocCmdInfo, &cmd);

	Com_Memset(&beginInfo, 0, sizeof(beginInfo));
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	Com_Memset(&barrier, 0, sizeof(barrier));
	barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                       = vk.swapchainImages[vk.currentImageIndex];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask               = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask               = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	Com_Memset(&region, 0, sizeof(region));
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width           = vk.swapchainExtent.width;
	region.imageExtent.height          = vk.swapchainExtent.height;
	region.imageExtent.depth           = 1;
	vkCmdCopyImageToBuffer(cmd, vk.swapchainImages[vk.currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer, 1, &region);

	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	vkEndCommandBuffer(cmd);

	Com_Memset(&submitInfo, 0, sizeof(submitInfo));
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &cmd;
	vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk.graphicsQueue);

	vkMapMemory(vk.device, readbackMemory, 0, bufSize, 0, &mapped);

	f = fopen(path, "wb");
	if (f)
	{
		byte *px = (byte *)mapped;

		fprintf(f, "P6\n%u %u\n255\n", vk.swapchainExtent.width, vk.swapchainExtent.height);
		for (y = 0; y < vk.swapchainExtent.height; y++)
		{
			for (x = 0; x < vk.swapchainExtent.width; x++)
			{
				byte *p = px + (y * vk.swapchainExtent.width + x) * 4;

				if (vk.swapchainFormat == VK_FORMAT_B8G8R8A8_UNORM)
				{
					fputc(p[2], f);
					fputc(p[1], f);
					fputc(p[0], f);
				}
				else
				{
					fputc(p[0], f);
					fputc(p[1], f);
					fputc(p[2], f);
				}
			}
		}
		fclose(f);
		Ren_Print("VK_DebugDumpSwapchainImage: wrote %s\n", path);
	}

	vkUnmapMemory(vk.device, readbackMemory);
	vkFreeCommandBuffers(vk.device, vk.commandPool, 1, &cmd);
	vkDestroyBuffer(vk.device, readbackBuffer, NULL);
	vkFreeMemory(vk.device, readbackMemory, NULL);
}

void VK_EndFrame(void)
{
	VkSubmitInfo         submitInfo;
	VkPresentInfoKHR     presentInfo;
	VkPipelineStageFlags waitStage       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	static int           debugFrameCount = 0;

	if (!vk.initialized)
	{
		return;
	}

	vkCmdEndRenderPass(vk.commandBuffer);

	// renderer1's R_ScreenGamma() - copy the offscreen scene
	// to the swapchain image applying r_gamma and the overbright scale.
	{
		VkRenderPassBeginInfo presentInfoRP;
		VkViewport            viewport;
		VkRect2D              scissor;
		float                 push[2];

		Com_Memset(&presentInfoRP, 0, sizeof(presentInfoRP));
		presentInfoRP.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		presentInfoRP.renderPass        = vk.presentRenderPass;
		presentInfoRP.framebuffer       = vk.swapchainFramebuffers[vk.currentImageIndex];
		presentInfoRP.renderArea.extent = vk.swapchainExtent;

		vkCmdBeginRenderPass(vk.commandBuffer, &presentInfoRP, VK_SUBPASS_CONTENTS_INLINE);

		Com_Memset(&viewport, 0, sizeof(viewport));
		viewport.width    = (float)vk.swapchainExtent.width;
		viewport.height   = (float)vk.swapchainExtent.height;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);

		Com_Memset(&scissor, 0, sizeof(scissor));
		scissor.extent = vk.swapchainExtent;
		vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gammaPipeline);
		vk.lastBoundPipeline = vk.gammaPipeline;
		vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gammaPipelineLayout, 0, 1, &vk.sceneColorDescriptor, 0, NULL);

		push[0] = vk.gammaValue;
		push[1] = vk.overbrightScale;
		vkCmdPushConstants(vk.commandBuffer, vk.gammaPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), push);

		vkCmdDraw(vk.commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(vk.commandBuffer);
	}

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

	debugFrameCount++;
	if (debugFrameCount == 200)
	{
		VK_DebugDumpSwapchainImage("/tmp/vk_capture.ppm");
	}

	// temporary on-demand debug capture: `touch /tmp/vk_capture_trigger`
	// while the game is running to dump the next frame.
	{
		FILE *trigger = fopen("/tmp/vk_capture_trigger", "r");

		if (trigger)
		{
			fclose(trigger);
			remove("/tmp/vk_capture_trigger");
			VK_DebugDumpSwapchainImage("/tmp/vk_capture_manual.ppm");
		}
	}
}

/**
 * @brief uploads a 32-bit RGBA image to a new device-local
 * VkImage (single mip level) via a host-visible staging buffer and a
 * synchronous one-shot transfer command buffer, then creates its view
 * and descriptor set (sampler picked by wrapClampMode). Stores the new
 * handles on the image_t itself.
 *
 * @note scope reductions: always uploads as
 * VK_FORMAT_R8G8B8A8_UNORM regardless of the legacy GL internalFormat
 * selection. Uploads the mipLevels packed levels in data
 * (built CPU-side by Upload32()).
 */
qboolean VK_CreateImageTexture(struct image_s *image, const byte *data, int width, int height, int mipLevels, qboolean mipmap, int wrapClampMode)
{
	VkDeviceSize                imageSize     = 0;
	VkBuffer                    stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory              stagingMemory = VK_NULL_HANDLE;
	void                        *mapped;
	VkImageCreateInfo           imageInfo;
	VkMemoryRequirements        memRequirements;
	VkMemoryAllocateInfo        allocInfo;
	VkImage                     vkImage       = VK_NULL_HANDLE;
	VkDeviceMemory              vkImageMemory = VK_NULL_HANDLE;
	VkCommandBuffer             cmd;
	VkImageMemoryBarrier        barrier;
	VkImageViewCreateInfo       viewInfo;
	VkImageView                 vkImageView = VK_NULL_HANDLE;
	VkDescriptorSetAllocateInfo dsAllocInfo;
	VkDescriptorSet             descriptorSet = VK_NULL_HANDLE;
	VkDescriptorImageInfo       imageDescInfo;
	VkWriteDescriptorSet        writeSet;
	VkSampler                   sampler;
	VkBufferImageCopy           regions[16];
	int                         level, levelW, levelH;

	if (!vk.initialized || !data || width <= 0 || height <= 0)
	{
		return qfalse;
	}

	// data holds mipLevels tightly packed RGBA levels, each
	// half the size of the previous one (clamped to 1), as built by
	// Upload32() with renderer1's R_MipMap().
	if (mipLevels < 1)
	{
		mipLevels = 1;
	}
	if (mipLevels > (int)ARRAY_LEN(regions))
	{
		mipLevels = (int)ARRAY_LEN(regions);
	}

	Com_Memset(regions, 0, sizeof(regions));
	levelW = width;
	levelH = height;
	for (level = 0; level < mipLevels; level++)
	{
		regions[level].bufferOffset                = imageSize;
		regions[level].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[level].imageSubresource.mipLevel   = (uint32_t)level;
		regions[level].imageSubresource.layerCount = 1;
		regions[level].imageExtent.width           = (uint32_t)levelW;
		regions[level].imageExtent.height          = (uint32_t)levelH;
		regions[level].imageExtent.depth           = 1;

		imageSize += (VkDeviceSize)levelW * (VkDeviceSize)levelH * 4;

		levelW = (levelW > 1) ? levelW >> 1 : 1;
		levelH = (levelH > 1) ? levelH >> 1 : 1;
	}

	if (!VK_CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &stagingBuffer, &stagingMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, stagingMemory, 0, imageSize, 0, &mapped);
	Com_Memcpy(mapped, data, (size_t)imageSize);
	vkUnmapMemory(vk.device, stagingMemory);

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = (uint32_t)width;
	imageInfo.extent.height = (uint32_t)height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = (uint32_t)mipLevels;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vkImage), "vkCreateImage"))
	{
		vkDestroyBuffer(vk.device, stagingBuffer, NULL);
		vkFreeMemory(vk.device, stagingMemory, NULL);
		return qfalse;
	}

	vkGetImageMemoryRequirements(vk.device, vkImage, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vkImageMemory), "vkAllocateMemory(image)"))
	{
		vkDestroyImage(vk.device, vkImage, NULL);
		vkDestroyBuffer(vk.device, stagingBuffer, NULL);
		vkFreeMemory(vk.device, stagingMemory, NULL);
		return qfalse;
	}

	vkBindImageMemory(vk.device, vkImage, vkImageMemory, 0);

	cmd = VK_BeginOneShotCommands();

	Com_Memset(&barrier, 0, sizeof(barrier));
	barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                       = vkImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = (uint32_t)mipLevels;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask               = 0;
	barrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	vkCmdCopyBufferToImage(cmd, stagingBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)mipLevels, regions);

	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	VK_EndOneShotCommands(cmd);

	vkDestroyBuffer(vk.device, stagingBuffer, NULL);
	vkFreeMemory(vk.device, stagingMemory, NULL);

	Com_Memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                       = vkImage;
	viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                      = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = (uint32_t)mipLevels;
	viewInfo.subresourceRange.layerCount = 1;

	if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vkImageView), "vkCreateImageView"))
	{
		vkDestroyImage(vk.device, vkImage, NULL);
		vkFreeMemory(vk.device, vkImageMemory, NULL);
		return qfalse;
	}

	sampler = VK_SamplerForImage(mipmap, wrapClampMode);

	Com_Memset(&dsAllocInfo, 0, sizeof(dsAllocInfo));
	dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo.descriptorPool     = vk.uiDescriptorPool;
	dsAllocInfo.descriptorSetCount = 1;
	dsAllocInfo.pSetLayouts        = &vk.uiDescriptorSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &dsAllocInfo, &descriptorSet), "vkAllocateDescriptorSets"))
	{
		vkDestroyImageView(vk.device, vkImageView, NULL);
		vkDestroyImage(vk.device, vkImage, NULL);
		vkFreeMemory(vk.device, vkImageMemory, NULL);
		return qfalse;
	}

	Com_Memset(&imageDescInfo, 0, sizeof(imageDescInfo));
	imageDescInfo.sampler     = sampler;
	imageDescInfo.imageView   = vkImageView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&writeSet, 0, sizeof(writeSet));
	writeSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.dstSet          = descriptorSet;
	writeSet.dstBinding      = 0;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeSet.pImageInfo      = &imageDescInfo;

	vkUpdateDescriptorSets(vk.device, 1, &writeSet, 0, NULL);

	image->vkImage         = (void *)vkImage;
	image->vkImageMemory   = (void *)vkImageMemory;
	image->vkImageView     = (void *)vkImageView;
	image->vkSampler       = (void *)sampler; // not owned here: one of the shared samplers
	image->vkDescriptorSet = (void *)descriptorSet;

	return qtrue;
}

/**
 * @brief Tears down the Vulkan resources owned by an image_t (created by
 * VK_CreateImageTexture); safe to call on an image that was never uploaded.
 * Synchronous (vkDeviceWaitIdle) since texture purges happen outside the
 * per-frame hot path.
 */
void VK_DeleteImageTexture(struct image_s *image)
{
	if (!image || !vk.initialized || vk.device == VK_NULL_HANDLE)
	{
		return;
	}

	if (!image->vkImage && !image->vkImageView && !image->vkDescriptorSet && !image->vkImageMemory)
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	if (image->vkDescriptorSet)
	{
		VkDescriptorSet ds = (VkDescriptorSet)image->vkDescriptorSet;
		vkFreeDescriptorSets(vk.device, vk.uiDescriptorPool, 1, &ds);
		image->vkDescriptorSet = NULL;
	}
	if (image->vkImageView)
	{
		vkDestroyImageView(vk.device, (VkImageView)image->vkImageView, NULL);
		image->vkImageView = NULL;
	}
	if (image->vkImage)
	{
		vkDestroyImage(vk.device, (VkImage)image->vkImage, NULL);
		image->vkImage = NULL;
	}
	if (image->vkImageMemory)
	{
		vkFreeMemory(vk.device, (VkDeviceMemory)image->vkImageMemory, NULL);
		image->vkImageMemory = NULL;
	}
	image->vkSampler = NULL; // shared sampler, not owned by this image
}

/**
 * @brief Converts the engine's GLS_ATEST_* state bits into the alphaTest.x
 * mode value consumed by tr3d.frag.glsl / ui.frag.glsl (0=off, 1=GT_0,
 * 2=LT_80, 3=GE_80).
 */
static float VK_AlphaTestMode(unsigned long stateBits)
{
	switch (stateBits & GLS_ATEST_BITS)
	{
	case GLS_ATEST_GT_0: return 1.0f;
	case GLS_ATEST_LT_80: return 2.0f;
	case GLS_ATEST_GE_80: return 3.0f;
	default: return 0.0f;
	}
}

/**
 * @brief Appends one shader batch's worth of already-tessellated 2D/UI
 * geometry (tess.xyz/texCoords[0]/vertexColors/indexes) into this frame's
 * vertex/index arenas and issues one vkCmdDrawIndexed with the UI pipeline
 * variant matching `stateBits` bound to the given image's descriptor set.
 * Honoring the real stateBits (rather than always forcing a fixed
 * blend-only state) is required for UI tricks that rely on depth test/
 * func/write or alpha test, e.g. the automap's circular-mask trick in
 * cg_commandmap.c (alphafunc GE128 + depthFunc equal against the 3D
 * scene's still-resident depth buffer, since 2D draws share the same
 * render pass/depth attachment as the 3D scene).
 *
 * @note scope reductions: only bundle 0 / image frame 0 is
 * considered (no multitexture, no animated-image frame selection yet).
 */
void VK_Draw2D(shaderCommands_t *input, struct image_s *image, unsigned long stateBits)
{
	int             numVertexes, numIndexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vkUIVertex_t    *dstVerts;
	uint32_t        *dstIndexes;
	VkViewport      viewport;
	VkRect2D        scissor;
	float           pushConstants[8];
	VkDescriptorSet descriptorSet;
	VkBuffer        vertexBuffers[1];
	VkDeviceSize    offsets[1];
	VkPipeline      pipeline;


	if (!vk.initialized || !input || input->numVertexes == 0 || input->numIndexes == 0)
	{
		return;
	}

	if (!image)
	{
		image = tr.whiteImage;
	}
	if (!image || !image->vkDescriptorSet)
	{
		// texture failed to upload (or hasn't been uploaded yet); skip this
		// batch rather than binding a garbage/null descriptor set.
		return;
	}

	numVertexes = input->numVertexes;
	numIndexes  = input->numIndexes;

	vertexBytes = (VkDeviceSize)numVertexes * sizeof(vkUIVertex_t);
	indexBytes  = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.vertexArenaOffset + vertexBytes > vk.vertexArenaSize ||
	    vk.indexArenaOffset + indexBytes > vk.indexArenaSize)
	{
		// Rate-limited: logging this on every dropped draw would itself grow
		// the on-screen console text, causing more 2D draws and more drops -
		// a feedback loop that never recovers once the arena fills up once.
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw2D - UI vertex/index arena exhausted this frame, dropping remaining 2D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vkUIVertex_t *)((byte *)vk.vertexArenaMapped + vk.vertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].uv[0]    = input->texCoords[i][0][0];
		dstVerts[i].uv[1]    = input->texCoords[i][0][1];
		dstVerts[i].color[0] = input->vertexColors[i][0];
		dstVerts[i].color[1] = input->vertexColors[i][1];
		dstVerts[i].color[2] = input->vertexColors[i][2];
		dstVerts[i].color[3] = input->vertexColors[i][3];
	}

	dstIndexes = (uint32_t *)((byte *)vk.indexArenaMapped + vk.indexArenaOffset);
	Com_Memcpy(dstIndexes, input->indexes, (size_t)indexBytes);

	pipeline = VK_GetUIPipeline(stateBits);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	if (vk.lastBoundPipeline != pipeline)
	{
		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vk.lastBoundPipeline = pipeline;
	}

	// Always set the full-window viewport/scissor for 2D/UI draws: viewport/
	// scissor are dynamic command-buffer state that persists across pipeline
	// binds, so if a 3D sub-view (e.g. the limbo menu's small player-model
	// preview) sets a smaller viewport earlier in this same frame, caching
	// "set once per frame" here would leave later UI draws incorrectly
	// clipped to that stale, smaller rect.
	Com_Memset(&viewport, 0, sizeof(viewport));
	viewport.width    = (float)glConfig.vidWidth;
	viewport.height   = (float)glConfig.vidHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);

	Com_Memset(&scissor, 0, sizeof(scissor));
	scissor.extent.width  = (uint32_t)glConfig.vidWidth;
	scissor.extent.height = (uint32_t)glConfig.vidHeight;
	vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);

	pushConstants[0] = 2.0f / (float)glConfig.vidWidth;
	pushConstants[1] = 2.0f / (float)glConfig.vidHeight;
	pushConstants[2] = -1.0f;
	pushConstants[3] = -1.0f;
	pushConstants[4] = VK_AlphaTestMode(stateBits);
	pushConstants[5] = 0.0f;
	pushConstants[6] = 0.0f;
	pushConstants[7] = 0.0f;
	vkCmdPushConstants(vk.commandBuffer, vk.uiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);

	descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.uiPipelineLayout, 0, 1, &descriptorSet, 0, NULL);

	vertexBuffers[0] = vk.vertexArenaBuffer;
	offsets[0]       = vk.vertexArenaOffset;
	vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(vk.commandBuffer, vk.indexArenaBuffer, vk.indexArenaOffset, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, 0, 0, 0);

	vk.vertexArenaOffset += vertexBytes;
	vk.indexArenaOffset  += indexBytes;
}

/**
 * @brief Sets the dynamic viewport/scissor for 3D scene draws. Must be
 * called once per view (from SetViewportAndScissor() in tr_backend.c,
 * alongside VK_SetProjectionMatrix) rather than cached per-frame: a single
 * frame may contain multiple 3D views with different viewport rects (e.g.
 * the limbo menu's small player-model preview rendered alongside the main
 * world view), so a "set once per frame" cache would leave later views
 * incorrectly using an earlier view's viewport/scissor.
 */
void VK_SetSceneViewport(int x, int y, int width, int height)
{
	VkViewport viewport;
	VkRect2D   scissor;
	int        flippedY;

	if (!vk.initialized)
	{
		return;
	}

	// backEnd.viewParms.viewportX/Y use GL's bottom-left-origin convention
	// (y is measured up from the bottom of the window), but Vulkan's
	// viewport/scissor use a top-left-origin convention. This cancels out
	// for full-height views (y=0, height=screenHeight) but must be
	// converted explicitly for partial-height views such as the limbo
	// menu's small player-model preview, otherwise the scissor rect ends
	// up in the wrong place and silently clips the view's geometry away.
	flippedY = (int)vk.swapchainExtent.height - (y + height);

	// stashed so VK_SetSkyDepthRange() can reissue the viewport with the
	// same rect but a different depth range.
	vk.lastViewportX        = x;
	vk.lastViewportFlippedY = flippedY;
	vk.lastViewportWidth    = width;
	vk.lastViewportHeight   = height;

	Com_Memset(&viewport, 0, sizeof(viewport));
	viewport.x        = (float)x;
	viewport.y        = (float)flippedY;
	viewport.width    = (float)width;
	viewport.height   = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);

	Com_Memset(&scissor, 0, sizeof(scissor));
	scissor.offset.x      = x;
	scissor.offset.y      = flippedY;
	scissor.extent.width  = (uint32_t)width;
	scissor.extent.height = (uint32_t)height;
	vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);
}

/**
 * @brief reissues the last scene viewport with a different
 * depth range, pinning sky geometry to the far depth plane (mirroring
 * renderer1's RB_StageIteratorSky glDepthRange(1,1) trick: drawing sky
 * first at maximum depth lets normally-depth-tested opaque surfaces
 * drawn afterwards cover it everywhere they're present).
 */
void VK_SetSkyDepthRange(float minDepth, float maxDepth)
{
	VkViewport viewport;

	if (!vk.initialized)
	{
		return;
	}

	Com_Memset(&viewport, 0, sizeof(viewport));
	viewport.x        = (float)vk.lastViewportX;
	viewport.y        = (float)vk.lastViewportFlippedY;
	viewport.width    = (float)vk.lastViewportWidth;
	viewport.height   = (float)vk.lastViewportHeight;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);
}

/**
 * @brief glClearColor() capture point (see vk_gl_compat.h).
 */
void VK_SetClearColor(float r, float g, float b, float a)
{
	vk.clearColor[0] = r;
	vk.clearColor[1] = g;
	vk.clearColor[2] = b;
	vk.clearColor[3] = a;
}

/**
 * @brief renderer1's glClear(GL_COLOR_BUFFER_BIT) with the
 * last glClearColor(). Clears the last scene viewport rect (GL's scissor
 * box during RB_BeginDrawingView/RB_Hyperspace), or the whole framebuffer
 * when fullFramebuffer is set (RB_DrawBuffer's r_clear, before any view).
 */
void VK_ClearSceneColor(qboolean fullFramebuffer)
{
	VkClearAttachment attachment;
	VkClearRect       rect;

	if (!vk.initialized)
	{
		return;
	}

	Com_Memset(&attachment, 0, sizeof(attachment));
	attachment.aspectMask                  = VK_IMAGE_ASPECT_COLOR_BIT;
	attachment.colorAttachment             = 0;
	attachment.clearValue.color.float32[0] = vk.clearColor[0];
	attachment.clearValue.color.float32[1] = vk.clearColor[1];
	attachment.clearValue.color.float32[2] = vk.clearColor[2];
	attachment.clearValue.color.float32[3] = vk.clearColor[3];

	Com_Memset(&rect, 0, sizeof(rect));
	if (fullFramebuffer || vk.lastViewportWidth <= 0 || vk.lastViewportHeight <= 0)
	{
		rect.rect.extent = vk.swapchainExtent;
	}
	else
	{
		rect.rect.offset.x      = vk.lastViewportX;
		rect.rect.offset.y      = vk.lastViewportFlippedY;
		rect.rect.extent.width  = (uint32_t)vk.lastViewportWidth;
		rect.rect.extent.height = (uint32_t)vk.lastViewportHeight;
	}
	rect.baseArrayLayer = 0;
	rect.layerCount     = 1;

	vkCmdClearAttachments(vk.commandBuffer, 1, &attachment, 1, &rect);
}

/**
 * @brief clears depth to 1.0 inside the last scene viewport
 * rect, replicating renderer1's per-view glClear(GL_DEPTH_BUFFER_BIT) in
 * RB_BeginDrawingView(). The render pass's loadOp=CLEAR only runs once
 * per frame, so without this a portal/mirror subview's depth survives
 * into the main view: main-view surfaces behind the mirror quad (and
 * elsewhere the subview drew nearer geometry) then depth-test against
 * the subview's values instead of a fresh buffer.
 */
void VK_ClearSceneDepth(void)
{
	VkClearAttachment attachment;
	VkClearRect       rect;

	if (!vk.initialized || vk.lastViewportWidth <= 0 || vk.lastViewportHeight <= 0)
	{
		return;
	}

	Com_Memset(&attachment, 0, sizeof(attachment));
	attachment.aspectMask                      = VK_IMAGE_ASPECT_DEPTH_BIT | (vk.depthHasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	attachment.clearValue.depthStencil.depth   = 1.0f;
	attachment.clearValue.depthStencil.stencil = 0;

	Com_Memset(&rect, 0, sizeof(rect));
	rect.rect.offset.x      = vk.lastViewportX;
	rect.rect.offset.y      = vk.lastViewportFlippedY;
	rect.rect.extent.width  = (uint32_t)vk.lastViewportWidth;
	rect.rect.extent.height = (uint32_t)vk.lastViewportHeight;
	rect.baseArrayLayer     = 0;
	rect.layerCount         = 1;

	vkCmdClearAttachments(vk.commandBuffer, 1, &attachment, 1, &rect);
}

/**
 * @brief True when the batch's shader requests polygon offset (decals,
 * marks); selects a depth-biased pipeline variant.
 */
static qboolean VK_ShaderPolygonOffset(const shaderCommands_t *input)
{
	return (input->shader && input->shader->polygonOffset) ? qtrue : qfalse;
}

/**
 * @brief Maps the shader's collapsed-multitexture env (how texture unit 1
 * combines with unit 0 in renderer1's DrawMultitextured()) to the mode
 * index tr3d_lm.frag.glsl expects: 0=modulate, 1=add, 2=replace, 3=decal.
 * r_lightMap forces GL_REPLACE there too, so the lightmap shows alone.
 */
static float VK_MultitextureEnvMode(const shaderCommands_t *input)
{
	if (r_lightMap->integer)
	{
		return 2.0f;
	}

	switch (input->shader ? input->shader->multitextureEnv : 0)
	{
	case GL_ADD: return 1.0f;
	case GL_REPLACE: return 2.0f;
	case GL_DECAL: return 3.0f;
	default: return 0.0f;
	}
}

/**
 * @brief submits one already-tessellated 3D shader stage
 * batch (single texture) using the lazily-created pipeline variant that
 * matches stateBits/cullType. Vertex data is read from input->xyz,
 * input->svars.texcoords[0] and input->svars.colors (populated by
 * ComputeColors()/ComputeTexCoords() in tr_shade.c).
 */
void VK_Draw3D(shaderCommands_t *input, struct image_s *image, unsigned long stateBits, int cullType)
{
	VK_Draw3DIndexes(input, input->indexes, input->numIndexes, image, stateBits, cullType);
}

/**
 * @brief VK_Draw3D() drawing only the given subset of indexes (e.g. the
 * triangles a dynamic light actually touches) while still reading every
 * vertex from input.
 */
void VK_Draw3DIndexes(shaderCommands_t *input, const glIndex_t *indexes, int numIndexes, struct image_s *image, unsigned long stateBits, int cullType)
{
	int             numVertexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertex_t    *dstVerts;
	uint32_t        *dstIndexes;
	float           pushConstants[48];
	VkPipeline      pipeline;
	VkDescriptorSet descriptorSet;
	VkBuffer        vertexBuffers[1];
	VkDeviceSize    offsets[1];

	if (!vk.initialized || !input || input->numVertexes == 0 || numIndexes == 0)
	{
		return;
	}

	if (!image)
	{
		image = tr.whiteImage;
	}
	if (!image || !image->vkDescriptorSet)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	numVertexes = input->numVertexes;

	vertexBytes = (VkDeviceSize)numVertexes * sizeof(vk3DVertex_t);
	indexBytes  = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3D - scene vertex/index arena exhausted this frame, dropping remaining 3D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vk3DVertex_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].pos[2]   = input->xyz[i][2];
		dstVerts[i].uv[0]    = input->svars.texcoords[0][i][0];
		dstVerts[i].uv[1]    = input->svars.texcoords[0][i][1];
		dstVerts[i].color[0] = input->svars.colors[i][0];
		dstVerts[i].color[1] = input->svars.colors[i][1];
		dstVerts[i].color[2] = input->svars.colors[i][2];
		dstVerts[i].color[3] = input->svars.colors[i][3];
	}

	dstIndexes = (uint32_t *)((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset);
	Com_Memcpy(dstIndexes, indexes, (size_t)indexBytes);

	if (vk.lastBoundPipeline != pipeline)
	{
		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vk.lastBoundPipeline = pipeline;
	}

	Com_Memcpy(pushConstants, vk.mvpMatrix, sizeof(vk.mvpMatrix));
	Com_Memcpy(pushConstants + 16, vk.modelMatrix, sizeof(vk.modelMatrix));
	pushConstants[32] = VK_AlphaTestMode(stateBits);
	pushConstants[33] = 0.0f;
	pushConstants[34] = 0.0f;
	pushConstants[35] = 0.0f;
	pushConstants[36] = vk.fogColor[0];
	pushConstants[37] = vk.fogColor[1];
	pushConstants[38] = vk.fogColor[2];
	pushConstants[39] = vk.fogColor[3];
	pushConstants[40] = vk.fogEnabled ? 1.0f : 0.0f;
	pushConstants[41] = vk.fogStart;
	pushConstants[42] = vk.fogEnd;
	pushConstants[43] = vk.clipPlaneEnabled ? 1.0f : 0.0f;
	pushConstants[44] = vk.clipPlane[0];
	pushConstants[45] = vk.clipPlane[1];
	pushConstants[46] = vk.clipPlane[2];
	pushConstants[47] = vk.clipPlane[3];
	vkCmdPushConstants(vk.commandBuffer, vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);

	descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.scenePipelineLayout, 0, 1, &descriptorSet, 0, NULL);

	vertexBuffers[0] = vk.sceneVertexArenaBuffer;
	offsets[0]       = vk.sceneVertexArenaOffset;
	vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(vk.commandBuffer, vk.sceneIndexArenaBuffer, vk.sceneIndexArenaOffset, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, 0, 0, 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief submits one already-tessellated 3D shader stage
 * batch modulating two textures (base * lightmap), used by
 * RB_StageIteratorLightmappedMultitexture and 2-stage (base+lightmap)
 * shaders handled by RB_StageIteratorGeneric's 3D path. UVs are read from
 * input->svars.texcoords[0]/[1], color from input->svars.colors.
 */
void VK_Draw3DLightmapped(shaderCommands_t *input, struct image_s *base, struct image_s *lightmap, unsigned long stateBits, int cullType)
{
	int             numVertexes, numIndexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertexLM_t  *dstVerts;
	uint32_t        *dstIndexes;
	float           pushConstants[48];
	VkPipeline      pipeline;
	VkDescriptorSet descriptorSets[2];
	VkBuffer        vertexBuffers[1];
	VkDeviceSize    offsets[1];

	if (!vk.initialized || !input || input->numVertexes == 0 || input->numIndexes == 0)
	{
		return;
	}

	if (!base)
	{
		base = tr.whiteImage;
	}
	if (!lightmap)
	{
		lightmap = tr.whiteImage;
	}
	if (!base->vkDescriptorSet || !lightmap->vkDescriptorSet)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(stateBits, VK_MirrorCullType(cullType), qtrue, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	numVertexes = input->numVertexes;
	numIndexes  = input->numIndexes;

	vertexBytes = (VkDeviceSize)numVertexes * sizeof(vk3DVertexLM_t);
	indexBytes  = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3DLightmapped - scene vertex/index arena exhausted this frame, dropping remaining 3D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vk3DVertexLM_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].pos[2]   = input->xyz[i][2];
		dstVerts[i].uv0[0]   = input->svars.texcoords[0][i][0];
		dstVerts[i].uv0[1]   = input->svars.texcoords[0][i][1];
		dstVerts[i].uv1[0]   = input->svars.texcoords[1][i][0];
		dstVerts[i].uv1[1]   = input->svars.texcoords[1][i][1];
		dstVerts[i].color[0] = input->svars.colors[i][0];
		dstVerts[i].color[1] = input->svars.colors[i][1];
		dstVerts[i].color[2] = input->svars.colors[i][2];
		dstVerts[i].color[3] = input->svars.colors[i][3];
	}

	dstIndexes = (uint32_t *)((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset);
	Com_Memcpy(dstIndexes, input->indexes, (size_t)indexBytes);

	if (vk.lastBoundPipeline != pipeline)
	{
		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vk.lastBoundPipeline = pipeline;
	}

	Com_Memcpy(pushConstants, vk.mvpMatrix, sizeof(vk.mvpMatrix));
	Com_Memcpy(pushConstants + 16, vk.modelMatrix, sizeof(vk.modelMatrix));
	pushConstants[32] = VK_AlphaTestMode(stateBits);
	pushConstants[33] = VK_MultitextureEnvMode(input);
	pushConstants[34] = 0.0f;
	pushConstants[35] = 0.0f;
	pushConstants[36] = vk.fogColor[0];
	pushConstants[37] = vk.fogColor[1];
	pushConstants[38] = vk.fogColor[2];
	pushConstants[39] = vk.fogColor[3];
	pushConstants[40] = vk.fogEnabled ? 1.0f : 0.0f;
	pushConstants[41] = vk.fogStart;
	pushConstants[42] = vk.fogEnd;
	pushConstants[43] = vk.clipPlaneEnabled ? 1.0f : 0.0f;
	pushConstants[44] = vk.clipPlane[0];
	pushConstants[45] = vk.clipPlane[1];
	pushConstants[46] = vk.clipPlane[2];
	pushConstants[47] = vk.clipPlane[3];
	vkCmdPushConstants(vk.commandBuffer, vk.sceneLMPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);

	descriptorSets[0] = (VkDescriptorSet)base->vkDescriptorSet;
	descriptorSets[1] = (VkDescriptorSet)lightmap->vkDescriptorSet;
	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.sceneLMPipelineLayout, 0, 2, descriptorSets, 0, NULL);

	vertexBuffers[0] = vk.sceneVertexArenaBuffer;
	offsets[0]       = vk.sceneVertexArenaOffset;
	vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(vk.commandBuffer, vk.sceneIndexArenaBuffer, vk.sceneIndexArenaOffset, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, 0, 0, 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief number of stencil bits of the depth attachment
 * (reported as glConfig.stencilBits; renderer1 code gates stencil
 * shadows on >= 4).
 */
int VK_StencilBits(void)
{
	return vk.depthHasStencil ? 8 : 0;
}

/**
 * @brief uploads raw positions with one constant color and
 * draws them with the given (special) scene pipeline, white texture.
 */
static void VK_DrawSceneRaw(VkPipeline pipeline, const vec4_t *xyz, int numVertexes, const byte *color,
                            const glIndex_t *indexes, int numIndexes, const float *mvp, const float *modelView, qboolean useClip)
{
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertex_t    *dstVerts;
	float           pushConstants[48];
	VkDescriptorSet descriptorSet;
	VkBuffer        vertexBuffers[1];
	VkDeviceSize    offsets[1];
	int             i;

	if (pipeline == VK_NULL_HANDLE || numVertexes == 0 || numIndexes == 0 || !tr.whiteImage || !tr.whiteImage->vkDescriptorSet)
	{
		return;
	}

	vertexBytes = (VkDeviceSize)numVertexes * sizeof(vk3DVertex_t);
	indexBytes  = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		return;
	}

	dstVerts = (vk3DVertex_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = xyz[i][0];
		dstVerts[i].pos[1]   = xyz[i][1];
		dstVerts[i].pos[2]   = xyz[i][2];
		dstVerts[i].uv[0]    = 0.0f;
		dstVerts[i].uv[1]    = 0.0f;
		dstVerts[i].color[0] = color[0];
		dstVerts[i].color[1] = color[1];
		dstVerts[i].color[2] = color[2];
		dstVerts[i].color[3] = color[3];
	}
	Com_Memcpy((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset, indexes, (size_t)indexBytes);

	if (vk.lastBoundPipeline != pipeline)
	{
		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vk.lastBoundPipeline = pipeline;
	}

	Com_Memset(pushConstants, 0, sizeof(pushConstants));
	Com_Memcpy(pushConstants, mvp, 16 * sizeof(float));
	Com_Memcpy(pushConstants + 16, modelView, 16 * sizeof(float));
	if (useClip && vk.clipPlaneEnabled)
	{
		pushConstants[43] = 1.0f;
		pushConstants[44] = vk.clipPlane[0];
		pushConstants[45] = vk.clipPlane[1];
		pushConstants[46] = vk.clipPlane[2];
		pushConstants[47] = vk.clipPlane[3];
	}
	vkCmdPushConstants(vk.commandBuffer, vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);

	descriptorSet = (VkDescriptorSet)tr.whiteImage->vkDescriptorSet;
	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.scenePipelineLayout, 0, 1, &descriptorSet, 0, NULL);

	vertexBuffers[0] = vk.sceneVertexArenaBuffer;
	offsets[0]       = vk.sceneVertexArenaOffset;
	vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(vk.commandBuffer, vk.sceneIndexArenaBuffer, vk.sceneIndexArenaOffset, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, 0, 0, 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief one stencil pass of a shadow volume (renderer1's
 * R_RenderShadowEdges() under glStencilOp(KEEP, KEEP, INCR/DECR) with
 * color writes off). cullType uses the engine's CT_* convention, already
 * resolved for mirrors by the caller.
 */
void VK_DrawShadowVolume(const vec4_t *xyz, int numVertexes, const glIndex_t *indexes, int numIndexes, int cullType, qboolean increment)
{
	static const byte color[4] = { 51, 51, 51, 255 };
	VkPipeline        pipeline;

	if (!vk.initialized || !vk.depthHasStencil)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO, cullType, qfalse, qfalse,
	                               increment ? VK_SPECIAL_SHADOW_INCR : VK_SPECIAL_SHADOW_DECR);

	VK_DrawSceneRaw(pipeline, xyz, numVertexes, color, indexes, numIndexes, vk.mvpMatrix, vk.modelMatrix, qtrue);
}

/**
 * @brief renderer1's RB_ShadowFinish() darken pass: multiply
 * the current view by 0.6 wherever the stencil is non-zero. Drawn as a
 * clip-space quad covering the view (identity matrices) without depth
 * test/write; renderer1 instead draws a near eye-space quad with depth
 * writes on, which has the same visible result.
 */
void VK_DrawShadowFinish(void)
{
	static const byte      color[4]     = { 153, 153, 153, 255 };
	static const vec4_t    quad[4]      = { { -1, -1, 0.5f, 1 }, { 1, -1, 0.5f, 1 }, { 1, 1, 0.5f, 1 }, { -1, 1, 0.5f, 1 } };
	static const glIndex_t idx[6]       = { 0, 1, 2, 0, 2, 3 };
	static const float     identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	VkPipeline             pipeline;

	if (!vk.initialized || !vk.depthHasStencil)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO, CT_TWO_SIDED, qfalse, qfalse,
	                               VK_SPECIAL_SHADOW_DARKEN);

	VK_DrawSceneRaw(pipeline, quad, 4, color, idx, 6, identity, identity, qfalse);
}

/**
 * @brief device anisotropy limit (0 if unsupported), reported
 * through renderer1's textureFilterAnisotropic/maxAnisotropy globals.
 */
float VK_GetMaxAnisotropy(void)
{
	return vk.maxSamplerAnisotropy;
}

/**
 * @brief Vulkan side of renderer1's GL_TextureMode(): rebuild
 * the mipmapped-image samplers for the new min/mag filter and rebind every
 * mipmapped image's descriptor to them (the sampler is baked into each
 * combined-image-sampler descriptor). Runs from RE_BeginFrame between
 * frames; waits for the GPU so no in-flight frame still uses the old ones.
 */
void VK_SetTextureMode(int minFilter, int magFilter)
{
	VkSampler oldRepeat, oldClamp;
	int       i;

	if (!vk.initialized || (minFilter == vk.texFilterMin && magFilter == vk.texFilterMag))
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	oldRepeat       = vk.mipSamplerRepeat;
	oldClamp        = vk.mipSamplerClamp;
	vk.texFilterMin = minFilter;
	vk.texFilterMag = magFilter;

	if (!VK_CreateMipSamplers())
	{
		Ren_Print("VK_SetTextureMode: sampler creation failed, keeping previous filter\n");
		if (vk.mipSamplerRepeat != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerRepeat, NULL);
		}
		if (vk.mipSamplerClamp != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerClamp, NULL);
		}
		vk.mipSamplerRepeat = oldRepeat;
		vk.mipSamplerClamp  = oldClamp;
		return;
	}

	for (i = 0; i < tr.numImages; i++)
	{
		image_t               *image = tr.images[i];
		VkDescriptorImageInfo imageDescInfo;
		VkWriteDescriptorSet  writeSet;
		VkSampler             sampler;

		if (!image || !image->mipmap || !image->vkDescriptorSet || !image->vkImageView)
		{
			continue;
		}

		sampler = VK_SamplerForImage(qtrue, image->wrapClampMode);

		Com_Memset(&imageDescInfo, 0, sizeof(imageDescInfo));
		imageDescInfo.sampler     = sampler;
		imageDescInfo.imageView   = (VkImageView)image->vkImageView;
		imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Com_Memset(&writeSet, 0, sizeof(writeSet));
		writeSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet          = (VkDescriptorSet)image->vkDescriptorSet;
		writeSet.dstBinding      = 0;
		writeSet.descriptorCount = 1;
		writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet.pImageInfo      = &imageDescInfo;

		vkUpdateDescriptorSets(vk.device, 1, &writeSet, 0, NULL);
		image->vkSampler = (void *)sampler;
	}

	vkDestroySampler(vk.device, oldRepeat, NULL);
	vkDestroySampler(vk.device, oldClamp, NULL);
}

/**
 * @brief values for the final gamma pass (set every frame by
 * R_ScreenGamma(); renderer1's u_gamma / u_overBrightBits uniforms).
 */
void VK_SetGammaParams(float gamma, float overbrightScale)
{
	vk.gammaValue      = (gamma > 0.0f) ? gamma : 1.0f;
	vk.overbrightScale = overbrightScale;
}
