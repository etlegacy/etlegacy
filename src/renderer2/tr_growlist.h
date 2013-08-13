/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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

#ifndef TR_GROWLIST_H
#define TR_GROWLIST_H
#include "tr_local.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	qboolean frameMemory;
	int currentElements;
	int maxElements;                // will reallocate and move when exceeded
	void **elements;
} growList_t;

// you don't need to init the growlist if you don't mind it growing and moving
// the list as it expands
void            Com_InitGrowList(growList_t *list, int maxElements);
void            Com_DestroyGrowList(growList_t *list);
int             Com_AddToGrowList(growList_t *list, void *data);
void *Com_GrowListElement(const growList_t *list, int index);
int             Com_IndexForGrowListElement(const growList_t *list, const void *element);

#ifdef __cplusplus
}
#endif

#endif