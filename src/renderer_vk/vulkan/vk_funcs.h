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
 * @file vulkan/vk_funcs.h
 * @brief X-macro lists of every Vulkan entry point the renderer calls.
 *
 * The renderer is built with VK_NO_PROTOTYPES and never links against a
 * Vulkan library: vk.c defines one function pointer per entry below and
 * resolves them at runtime through vkGetInstanceProcAddr (obtained via
 * ri.VKimp_GetInstanceProcAddr from the library SDL opened: the system Vulkan
 * loader, or on macOS the MoltenVK in the app bundle) and vkGetDeviceProcAddr.
 *
 * Calling a Vulkan function that is not listed here fails to compile when
 * VK_NO_PROTOTYPES is defined - add it to the matching list.
 */

#ifndef VK_FUNCS_H
#define VK_FUNCS_H

/// loader-level functions, resolved with a NULL instance
#define VK_GLOBAL_FUNCS(X) \
		X(vkCreateInstance) \
		X(vkEnumerateInstanceExtensionProperties)

/// instance-level functions, resolved after vkCreateInstance
#define VK_INSTANCE_FUNCS(X) \
		X(vkCreateDevice) \
		X(vkDestroyInstance) \
		X(vkDestroySurfaceKHR) \
		X(vkEnumerateDeviceExtensionProperties) \
		X(vkEnumeratePhysicalDevices) \
		X(vkGetDeviceProcAddr) \
		X(vkGetPhysicalDeviceFeatures) \
		X(vkGetPhysicalDeviceFormatProperties) \
		X(vkGetPhysicalDeviceMemoryProperties) \
		X(vkGetPhysicalDeviceProperties) \
		X(vkGetPhysicalDeviceQueueFamilyProperties) \
		X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
		X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
		X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
		X(vkGetPhysicalDeviceSurfaceSupportKHR)

/// device-level functions, resolved after vkCreateDevice
#define VK_DEVICE_FUNCS(X) \
		X(vkAcquireNextImageKHR) \
		X(vkAllocateCommandBuffers) \
		X(vkAllocateDescriptorSets) \
		X(vkAllocateMemory) \
		X(vkBeginCommandBuffer) \
		X(vkBindBufferMemory) \
		X(vkBindImageMemory) \
		X(vkCmdBeginRenderPass) \
		X(vkCmdBindDescriptorSets) \
		X(vkCmdBindIndexBuffer) \
		X(vkCmdBindPipeline) \
		X(vkCmdBindVertexBuffers) \
		X(vkCmdClearAttachments) \
		X(vkCmdClearColorImage) \
		X(vkCmdCopyBufferToImage) \
		X(vkCmdCopyImageToBuffer) \
		X(vkCmdDraw) \
		X(vkCmdDrawIndexed) \
		X(vkCmdDrawIndexedIndirect) \
		X(vkCmdDispatch) \
		X(vkCmdEndRenderPass) \
		X(vkCmdPipelineBarrier) \
		X(vkCmdPushConstants) \
		X(vkCmdSetScissor) \
		X(vkCmdSetViewport) \
		X(vkCreateBuffer) \
		X(vkCreateCommandPool) \
		X(vkCreateDescriptorPool) \
		X(vkCreateDescriptorSetLayout) \
		X(vkCreateFence) \
		X(vkCreateFramebuffer) \
		X(vkCreateGraphicsPipelines) \
		X(vkCreateComputePipelines) \
		X(vkCreateImage) \
		X(vkCreateImageView) \
		X(vkCreatePipelineLayout) \
		X(vkCreateRenderPass) \
		X(vkCreateSampler) \
		X(vkCreateSemaphore) \
		X(vkCreateShaderModule) \
		X(vkCreateSwapchainKHR) \
		X(vkDestroyBuffer) \
		X(vkDestroyCommandPool) \
		X(vkDestroyDescriptorPool) \
		X(vkDestroyDescriptorSetLayout) \
		X(vkDestroyDevice) \
		X(vkDestroyFence) \
		X(vkDestroyFramebuffer) \
		X(vkDestroyImage) \
		X(vkDestroyImageView) \
		X(vkDestroyPipeline) \
		X(vkDestroyPipelineLayout) \
		X(vkDestroyRenderPass) \
		X(vkDestroySampler) \
		X(vkDestroySemaphore) \
		X(vkDestroyShaderModule) \
		X(vkDestroySwapchainKHR) \
		X(vkDeviceWaitIdle) \
		X(vkEndCommandBuffer) \
		X(vkFreeCommandBuffers) \
		X(vkFreeDescriptorSets) \
		X(vkFreeMemory) \
		X(vkGetBufferMemoryRequirements) \
		X(vkGetDeviceQueue) \
		X(vkGetImageMemoryRequirements) \
		X(vkGetSwapchainImagesKHR) \
		X(vkMapMemory) \
		X(vkQueuePresentKHR) \
		X(vkQueueSubmit) \
		X(vkQueueWaitIdle) \
		X(vkResetCommandBuffer) \
		X(vkResetFences) \
		X(vkUnmapMemory) \
		X(vkUpdateDescriptorSets) \
		X(vkWaitForFences) \
		X(vkAntiLagUpdateAMD)

// fallback definitions for Vulkan SDK headers < 1.3.291
#ifndef VK_AMD_anti_lag
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ANTI_LAG_FEATURES_AMD ((VkStructureType)1000476000)
#define VK_STRUCTURE_TYPE_ANTI_LAG_DATA_AMD ((VkStructureType)1000476001)
#define VK_STRUCTURE_TYPE_ANTI_LAG_PRESENTATION_INFO_AMD ((VkStructureType)1000476002)

typedef enum VkAntiLagModeAMD
{
	VK_ANTI_LAG_MODE_DRIVER_CONTROL_AMD = 0,
	VK_ANTI_LAG_MODE_ON_AMD             = 1,
	VK_ANTI_LAG_MODE_OFF_AMD            = 2
} VkAntiLagModeAMD;

typedef enum VkAntiLagStageAMD
{
	VK_ANTI_LAG_STAGE_INPUT_AMD   = 0,
	VK_ANTI_LAG_STAGE_PRESENT_AMD = 1
} VkAntiLagStageAMD;

typedef struct VkPhysicalDeviceAntiLagFeaturesAMD
{
	VkStructureType sType;
	void            *pNext;
	VkBool32        antiLag;
} VkPhysicalDeviceAntiLagFeaturesAMD;

typedef struct VkAntiLagPresentationInfoAMD
{
	VkStructureType   sType;
	void              *pNext;
	VkAntiLagStageAMD stage;
	uint64_t          frameIndex;
} VkAntiLagPresentationInfoAMD;

typedef struct VkAntiLagDataAMD
{
	VkStructureType                    sType;
	const void                         *pNext;
	VkAntiLagModeAMD                   mode;
	uint32_t                           maxFPS;
	const VkAntiLagPresentationInfoAMD *pPresentationInfo;
} VkAntiLagDataAMD;

typedef void (VKAPI_PTR *PFN_vkAntiLagUpdateAMD)(VkDevice device, const VkAntiLagDataAMD *pData);
#endif

#endif // VK_FUNCS_H
