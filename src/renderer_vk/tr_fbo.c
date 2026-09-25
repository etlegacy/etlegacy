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
 * @file renderer_vk/tr_fbo.c
 * @brief Placeholder: renderer1's FBO render targets are not used. tr.useFBO
 * stays false, so the reused frontend/backend code paths that gate on it
 * never touch these FBOs. Offscreen targets (scene color, MSAA resolve)
 * are handled directly by the Vulkan backend in vulkan/vk.c.
 */

#include "tr_local.h"

frameBuffer_t *mainFbo   = NULL;
frameBuffer_t *msMainFbo = NULL;

void R_FBOSetViewport(frameBuffer_t *from, frameBuffer_t *to)
{
}

void R_BindFBO(frameBuffer_t *fb)
{
}

frameBuffer_t *R_CurrentFBO(void)
{
	return NULL;
}

byte *R_FBOReadPixels(frameBuffer_t *fb, size_t *offset, int *padlen)
{
	if (offset)
	{
		*offset = 0;
	}
	if (padlen)
	{
		*padlen = 0;
	}
	return NULL;
}

void R_FboCopyToTex(frameBuffer_t *from, image_t *to)
{
}

void R_FboBlit(frameBuffer_t *from, frameBuffer_t *to)
{
}

void R_FboRenderTo(frameBuffer_t *from, frameBuffer_t *to)
{
}

void R_InitFBO(void)
{
	tr.useFBO = qfalse;
}

void R_ShutdownFBO(void)
{
}
