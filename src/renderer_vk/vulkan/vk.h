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
 * @file vulkan/vk.h
 * @brief Vulkan plumbing: instance/device/surface/swapchain/
 * render pass/framebuffers/command buffers/sync objects, enough to clear
 * the screen and present a frame. Real geometry submission (vertex/index
 * buffers, pipelines, descriptor sets) is deferred for now;
 * see tr_shade.c.
 */

#ifndef VK_H
#define VK_H

#include "../../qcommon/q_shared.h"

/**
 * @brief Creates the VkInstance, picks a physical device, creates the
 * logical device + queues, the window surface (via ri.VKimp_CreateSurface)
 * and the initial swapchain/render pass/framebuffers/command pool/sync
 * objects for the given window size. Fills in glConfig's vendor/renderer/
 * version strings and basic limits from the chosen physical device.
 * @return qfalse on unrecoverable failure (caller should fall back / abort)
 */
qboolean VK_Init(int width, int height);

/**
 * @brief Tears down the swapchain/render pass/framebuffers/sync objects,
 * the logical device, the surface and the instance, in that order. Safe to
 * call even if VK_Init() only partially succeeded.
 */
void VK_Shutdown(void);

/**
 * @brief Recreates the swapchain/framebuffers for a new window size
 * (window resize, display mode change, ...).
 */
qboolean VK_ResizeSwapchain(int width, int height);

/**
 * @brief Acquires the next swapchain image and begins recording the
 * primary command buffer for this frame, starting the render pass with a
 * clear-to-black (or r_clear colored) attachment load.
 */
void VK_BeginFrame(void);

/**
 * @brief Ends the render pass / command buffer, submits it, and presents
 * the acquired swapchain image.
 */
void VK_EndFrame(void);

struct image_s;
struct shaderCommands_s;

/**
 * @brief uploads a 32-bit RGBA image (one mip level only) to
 * a new device-local Vulkan texture + descriptor set, stored on the given
 * image_t. Returns qfalse on failure (caller should treat the image as
 * untextured / fall back to tr.whiteImage).
 */
qboolean VK_CreateImageTexture(struct image_s *image, const byte *data, int width, int height, qboolean mipmap, int wrapClampMode);

/**
 * @brief Destroys the Vulkan resources created by VK_CreateImageTexture
 * for the given image_t. Safe to call on an image that was never uploaded.
 */
void VK_DeleteImageTexture(struct image_s *image);

/**
 * @brief submits one already-tessellated 2D/UI shader batch
 * (as populated by RB_StretchPic/RB_Draw2dPolys/etc. into tess) using the
 * fixed UI pipeline, textured with the given image (tr.whiteImage if NULL).
 */
void VK_Draw2D(struct shaderCommands_s *input, struct image_s *image);

#endif // VK_H
