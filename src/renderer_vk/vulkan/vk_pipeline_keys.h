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
 * @file vulkan/vk_pipeline_keys.h
 * @brief Pipeline keys: what identifies a pipeline of the Vulkan renderer, and
 * the built-in list of keys created at startup (vk_pipeline_keys.c).
 */

#ifndef INCLUDE_VK_PIPELINE_KEYS_H
#define INCLUDE_VK_PIPELINE_KEYS_H

#include <stdint.h>

/**
 * @struct vkPipelineKey_t
 * @brief The arguments of VK_GetUIPipeline() / VK_GetScenePipelineEx(). The
 * same on every GPU, so a list of keys (vkpipelinekeys.bin, and the built-in
 * list) stays valid across GPUs and driver updates.
 */
typedef struct
{
	uint32_t type;                  ///< VK_PIPELINE_KEY_*
	uint32_t stateBits;             ///< GLS_* bits
	int32_t cullType;               ///< CT_*
	int32_t polygonOffset;          ///< 0 or 1
	int32_t special;                ///< VK_SPECIAL_*
	int32_t vertexSource;           ///< VK_VERTS_*
} vkPipelineKey_t;

#define VK_PIPELINE_KEY_UI       0
#define VK_PIPELINE_KEY_SCENE    1
#define VK_PIPELINE_KEY_SCENE_LM 2

/// The pipelines of the stock maps, created at startup together with the
/// player's own list, so that a first run doesn't compile them mid-frame.
extern const vkPipelineKey_t vkDefaultPipelineKeys[];
extern const int             vkNumDefaultPipelineKeys;

#endif // #ifndef INCLUDE_VK_PIPELINE_KEYS_H
