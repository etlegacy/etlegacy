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

#ifndef VM_EXT_H
#define VM_EXT_H

#include "q_shared.h"

typedef struct
{
	char *name;
	int trapKey;
	qboolean active;
} ext_trap_keys_t;

/**
 * @brief Get engine value
 * @param[out] value buffer
 * @param[in] valueSize buffer size
 * @param[in] key to query
 * @return true if value for key is found
 */
static ID_INLINE qboolean VM_Ext_GetValue(ext_trap_keys_t *list, char *value, int valueSize, const char *key)
{
	int i;

	if (!list)
	{
		return qfalse;
	}

	for (i = 0; list[i].name; i++)
	{
		if (!Q_stricmp(key, list[i].name))
		{
			Com_sprintf(value, valueSize, "%i", list[i].trapKey);
			list[i].active = qtrue;
			return qtrue;
		}
	}

	return qfalse;
}

static ID_INLINE void VM_Ext_ResetActive(ext_trap_keys_t *list)
{
	int i;

	if (!list)
	{
		return;
	}

	// mark all extensions as inactive
	for (i = 0; list[i].name; i++)
	{
		list[i].active = qfalse;
	}
}

#ifndef TRAP_EXTENSIONS_LIST
#error "Missing TRAP_EXTENSIONS_LIST definition"
#endif

#define TRAP_EXT_INDEX(x) ((x) - (COM_TRAP_GETVALUE + 1))
#define VM_Ext_IsActive(x) TRAP_EXTENSIONS_LIST[TRAP_EXT_INDEX((x))].active
#define VM_Ext_GetValue(value, size, key) VM_Ext_GetValue(TRAP_EXTENSIONS_LIST, (value), (size), (key))
#define VM_Ext_ResetActive() VM_Ext_ResetActive(TRAP_EXTENSIONS_LIST)

#endif
