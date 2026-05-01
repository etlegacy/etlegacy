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
 * @file renderer_vk/tr_shader_program.c
 * @brief placeholder: renderer1's GLSL program objects have no
 * Vulkan equivalent (SPIR-V modules + pipeline objects do). Reports no
 * shader programs available so callers (R_InitGamma, etc.) take their
 * "unavailable" fallback path; a real implementation arrives alongside the
 * Vulkan pipeline work.
 */

#include "tr_local.h"

void R_UseShaderProgram(shaderProgram_t *program)
{
}

GLint R_GetShaderProgramUniform(shaderProgram_t *program, const char *name)
{
	return -1;
}

shaderProgram_t *R_CreateShaderProgram(const char *vert, const char *frag)
{
	return NULL;
}

void R_DestroyShaderProgram(shaderProgram_t *program)
{
}

qboolean R_ShaderProgramsAvailable(void)
{
	return qfalse;
}

void R_InitShaderPrograms(void)
{
}

void R_ShutdownShaderPrograms(void)
{
}
