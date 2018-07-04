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
 * @file g_mem.c
 */

#include "g_local.h"

#define POOLSIZE    (16 * 1024 * 1024) // up to 32 if required

static char memoryPool[POOLSIZE];
static unsigned int  allocPoint;

/**
 * @brief G_Alloc
 * @param[in] size
 * @return
 */
void *G_Alloc(unsigned int size)
{
	char *p;

	if (g_debugAlloc.integer)
	{
		G_Printf("G_Alloc of %i bytes (%i bytes left)\n", size, POOLSIZE - allocPoint - ((size + 31) & ~31u));
	}

	if (allocPoint + size > POOLSIZE)
	{
		G_Error("G_Alloc: failed on allocation of %u bytes\n", size);
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += (size + 31) & ~31u;

	return p;
}

/**
 * @brief G_InitMemory
 */
void G_InitMemory(void)
{
	allocPoint = 0;
}

/**
 * @brief Svcmd_GameMem_f
 */
void Svcmd_GameMem_f(void)
{
	G_Printf("Game memory status: %i out of %i bytes allocated - %i bytes free\n", allocPoint, POOLSIZE, POOLSIZE - allocPoint);
}
