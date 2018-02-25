/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 * @file cg_polybus.c
 */

#include "cg_local.h"

#define MAX_PB_BUFFERS  128

polyBuffer_t cg_polyBuffers[MAX_PB_BUFFERS];
qboolean     cg_polyBuffersInuse[MAX_PB_BUFFERS];

/**
 * @brief CG_PB_FindFreePolyBuffer
 * @param[in] shader
 * @param[in] numVerts
 * @param[in] numIndicies
 * @return 
 */
polyBuffer_t *CG_PB_FindFreePolyBuffer(qhandle_t shader, int numVerts, int numIndicies)
{
	int i;
	int firstFree = -1;

	// first find one with the same shader if possible
	for (i = 0; i < MAX_PB_BUFFERS; ++i)
	{
		if (!cg_polyBuffersInuse[i])
		{
			if (firstFree == -1)
			{
				firstFree = i;
			}
			continue;
		}

		if (cg_polyBuffers[i].shader != shader)
		{
			continue;
		}

		if (cg_polyBuffers[i].numIndicies + numIndicies >= MAX_PB_INDICIES)
		{
			continue;
		}

		if (cg_polyBuffers[i].numVerts + numVerts >= MAX_PB_VERTS)
		{
			continue;
		}

		cg_polyBuffersInuse[i]   = qtrue;
		cg_polyBuffers[i].shader = shader;

		return &cg_polyBuffers[i];
	}

	// return the free pB we have already found above
	if (firstFree != -1)
	{
		cg_polyBuffersInuse[firstFree]        = qtrue;
		cg_polyBuffers[firstFree].shader      = shader;
		cg_polyBuffers[firstFree].numIndicies = 0;
		cg_polyBuffers[firstFree].numVerts    = 0;
		return &cg_polyBuffers[firstFree];
	}

	// or return NULL if no free buffer was found..
	return NULL;
}

/**
 * @brief CG_PB_ClearPolyBuffers
 */
void CG_PB_ClearPolyBuffers(void)
{
	// changed numIndicies and numVerts to be reset in CG_PB_FindFreePolyBuffer, not here (should save the cache misses we were prolly getting)
	Com_Memset(cg_polyBuffersInuse, 0, sizeof(cg_polyBuffersInuse));
}

/**
 * @brief CG_PB_RenderPolyBuffers
 */
void CG_PB_RenderPolyBuffers(void)
{
	int i;

	for (i = 0; i < MAX_PB_BUFFERS; i++)
	{
		if (cg_polyBuffersInuse[i])
		{
			trap_R_AddPolyBufferToScene(&cg_polyBuffers[i]);
		}
	}
}
