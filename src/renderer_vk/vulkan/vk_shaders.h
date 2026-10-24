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
 * @file vulkan/vk_shaders.h
 * @brief The Vulkan renderer's shaders: the GLSL sources in glsl/, compiled
 * to SPIR-V and embedded into one generated source (vk_shaders.c, written by
 * cmake/ETLEmbedVkShaders.cmake; the shader list is in cmake/ETLVkShaders.cmake).
 */

#ifndef VK_SHADERS_H
#define VK_SHADERS_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
	const char *name;      ///< GLSL file name without ".glsl", e.g. "tr3d_lm.frag"
	const uint32_t *code;  ///< SPIR-V words
	size_t size;           ///< in bytes
} vkShaderCode_t;

/**
 * @brief The embedded shader with this name, or NULL if there is none.
 */
const vkShaderCode_t *VK_FindShaderCode(const char *name);

#endif // VK_SHADERS_H
