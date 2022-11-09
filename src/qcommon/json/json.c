/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2022 ET:Legacy team <mail@etlegacy.com>
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
 * @file json.c
 * @brief cJSON helper functions
*/

#include "json.h"

#if MODLIB
#if CGAMEDLL
#include "../../cgame/cg_local.h"
#elif UIDLL
#include "../../ui/ui_local.h"
#elif GAMEDLL
#include "../../game/g_local.h"
#endif
#else
#include "../qcommoh.h"
#endif

void Q_JSONInit(void)
{
	static qboolean initDone = qfalse;

	if (initDone)
	{
		return;
	}

	// This is mostly pointless now, but we might want to use custom allocators or a buffer at some point.
	cJSON_Hooks hooks =
	{
		Com_Allocate,
		Com_Dealloc
	};
	cJSON_InitHooks(&hooks);
	initDone = qtrue;
}

void Q_FSWriteJSON(cJSON *object, fileHandle_t handle)
{
	char *serialised = NULL;

	serialised = cJSON_Print(object);

#if MODLIB
	trap_FS_Write(serialised, (int) strlen(serialised), handle);
	trap_FS_FCloseFile(handle);
#else
	FS_Write(serialised, (int) strlen(serialised), handle);
	FS_FCloseFile(handle);
#endif

	Com_Dealloc(serialised);
	cJSON_Delete(object);
}
