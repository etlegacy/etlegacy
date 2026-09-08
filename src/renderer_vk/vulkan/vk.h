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
 * @brief uploads a 32-bit RGBA image (data
 * holds mipLevels packed levels, see Upload32()) to a new device-local
 * Vulkan texture + descriptor set, stored on the given
 * image_t. Returns qfalse on failure (caller should treat the image as
 * untextured / fall back to tr.whiteImage).
 */
qboolean VK_CreateImageTexture(struct image_s *image, const byte *data, int width, int height, int mipLevels, qboolean mipmap, int wrapClampMode);

/**
 * @brief rebuilds the mipmapped-image samplers for a new
 * GL-style min/mag filter and rebinds existing images (GL_TextureMode).
 */
void VK_SetTextureMode(int minFilter, int magFilter);

/**
 * @brief device max sampler anisotropy, 0 if unsupported.
 */
float VK_GetMaxAnisotropy(void);

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
void VK_Draw2D(struct shaderCommands_s *input, struct image_s *image, unsigned long stateBits);

/**
 * @brief captures the GL-convention projection matrix (see
 * tr_backend.c's SetViewportAndScissor()) and bakes in the GL -> Vulkan
 * clip-space correction (Y-flip + depth range remap).
 */
void VK_SetProjectionMatrix(const float *m);
void VK_GetProjectionMatrix(float *m);
void VK_SetRawProjectionMatrix(const float *m);

/**
 * @brief sets the dynamic viewport/scissor for 3D scene
 * draws. Must be called once per view (a single frame may contain
 * multiple 3D views with different viewport rects, e.g. the limbo menu's
 * small player-model preview alongside the main world view).
 */
void VK_SetSceneViewport(int x, int y, int width, int height);

/**
 * @brief reissues the last scene viewport with a different
 * depth range (used by sky rendering to pin geometry to the far depth
 * plane, matching renderer1's glDepthRange(1,1) trick).
 */
void VK_SetSkyDepthRange(float minDepth, float maxDepth);

/**
 * @brief clears the depth buffer within the last scene
 * viewport rect (renderer1's per-view glClear(GL_DEPTH_BUFFER_BIT)).
 */
void VK_ClearSceneDepth(void);

/**
 * @brief color clear with the last glClearColor() value, over
 * the last scene viewport (or the whole framebuffer).
 */
void VK_ClearSceneColor(qboolean fullFramebuffer);

/**
 * @brief gamma and overbright multiplier applied by the final
 * offscreen -> swapchain pass in VK_EndFrame().
 */
void VK_SetGammaParams(float gamma, float overbrightScale);

/**
 * @brief stencil bits of the depth attachment (0 or 8).
 */
int VK_StencilBits(void);

/**
 * @brief stencil shadow volumes (renderer1's
 * RB_ShadowTessEnd()/RB_ShadowFinish()). xyz is the engine's padded
 * vec4 position array; cullType uses the CT_* convention.
 */
void VK_DrawShadowVolume(const float (*xyz)[4], int numVertexes, const unsigned int *indexes, int numIndexes, int cullType, qboolean increment);
void VK_DrawShadowFinish(void);

/**
 * @brief captures the GL-convention modelview matrix (world
 * or per-entity, see tr_backend.c's RB_RenderDrawSurfList()) and
 * recomputes the cached MVP used by VK_Draw3D()/VK_Draw3DLightmapped().
 */
void VK_SetModelViewMatrix(const float *m);
void VK_GetModelViewMatrix(float *m);

/**
 * @brief records linear-fog parameters (color + start/end
 * distance) applied in the 3D fragment shaders based on eye-space depth.
 * Called from R_Fog() (tr_main.c) in place of the no-op'd glFog*() calls.
 */
void VK_SetFog(const float *color, float start, float end);

/**
 * @brief disables fog for subsequent 3D draws. Called from
 * R_FogOff() (tr_main.c) in place of the no-op'd glDisable(GL_FOG).
 */
void VK_SetFogOff(void);
void VK_SetFogOn(void);

/**
 * @brief records an eye-space portal/mirror clip-plane
 * equation, applied as a per-fragment discard in the 3D fragment shaders.
 * Called from RB_BeginDrawingView() (tr_backend.c) in place of the
 * no-op'd glClipPlane()/glEnable(GL_CLIP_PLANE0) calls.
 */
void VK_SetPortalClip(const float *planeEq);

/**
 * @brief disables the portal/mirror clip plane for
 * subsequent 3D draws. Called in place of the no-op'd
 * glDisable(GL_CLIP_PLANE0).
 */
void VK_ClearPortalClip(void);

/**
 * @brief submits one already-tessellated 3D shader stage
 * batch (single texture) using a pipeline variant matching stateBits/
 * cullType, transformed by the current projection * modelview matrix.
 */
void VK_Draw3D(struct shaderCommands_s *input, struct image_s *image, unsigned long stateBits, int cullType);

/**
 * @brief VK_Draw3D() restricted to a subset of indexes (all
 * vertices still uploaded), used by the dynamic light pass.
 */
void VK_Draw3DIndexes(struct shaderCommands_s *input, const unsigned int *indexes, int numIndexes, struct image_s *image, unsigned long stateBits, int cullType);

/**
 * @brief submits one already-tessellated 3D shader stage
 * batch modulating two textures (base * lightmap).
 */
void VK_Draw3DLightmapped(struct shaderCommands_s *input, struct image_s *base, struct image_s *lightmap, unsigned long stateBits, int cullType);

#endif // VK_H
