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
#include "../qcommon.h"
#endif

void Q_JSONInitWith(void *(*malloc_fn)(size_t sz), void (*free_fn)(void *ptr))
{
	static qboolean initDone = qfalse;
	cJSON_Hooks     hooks;

	if (initDone)
	{
		return;
	}

	// This is mostly pointless now, but we might want to use custom allocators or a buffer at some point.
	hooks = (cJSON_Hooks)
	{
		malloc_fn,
		free_fn
	};
	cJSON_InitHooks(&hooks);
	initDone = qtrue;
}

void Q_JSONInit(void)
{
	Q_JSONInitWith(Com_Allocate, Com_Dealloc);
}

cJSON *Q_FSReadJsonFrom(const char *path)
{
	fileHandle_t fileHandle;
	int          len;
	char         *buffer = NULL;
	cJSON        *object = NULL;

#if MODLIB
	len = trap_FS_FOpenFile(path, &fileHandle, FS_READ);
#else
	len = FS_FOpenFileByMode(path, &fileHandle, FS_READ);
#endif

	// if there is no file, or if the size is over 5 megs (which is a bit sus)
	if (!fileHandle || !len || len > 5242880)
	{
#if MODLIB
		trap_FS_FCloseFile(fileHandle);
#else
		FS_FCloseFile(fileHandle);
#endif
		return NULL;
	}

	buffer = (char *)cJSON_malloc(len + 1);
	if (!buffer)
	{
		return NULL;
	}

#if MODLIB
	trap_FS_Read(buffer, len, fileHandle);
	trap_FS_FCloseFile(fileHandle);
#else
	FS_Read(buffer, len, fileHandle);
	FS_FCloseFile(fileHandle);
#endif
	buffer[len] = '\0';

	// read buffer
	object = cJSON_Parse(buffer);
	cJSON_free(buffer);

	return object;
}

qboolean Q_FSWriteJSONTo(cJSON *object, const char *path)
{
	int          len;
	fileHandle_t fileHandle;
#if MODLIB
	len = trap_FS_FOpenFile(path, &fileHandle, FS_WRITE);
#else
	len = FS_FOpenFileByMode(path, &fileHandle, FS_WRITE);
#endif

	// file handle failed
	if (len < 0)
	{
		return qfalse;
	}

	return Q_FSWriteJSON(object, fileHandle);
}

qboolean Q_FSWriteJSON(cJSON *object, fileHandle_t handle)
{
	int  len, outLen;
	char *serialised = NULL;

	serialised = cJSON_Print(object);
	len        = (int) strlen(serialised);

#if MODLIB
	outLen = trap_FS_Write(serialised, len, handle);
	trap_FS_FCloseFile(handle);
#else
	outLen = FS_Write(serialised, len, handle);
	FS_FCloseFile(handle);
#endif

	if (len != outLen)
	{
		return qfalse;
	}

	cJSON_free(serialised);
	cJSON_Delete(object);

	return qtrue;
}
