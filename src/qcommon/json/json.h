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
 * @file json.h
 * @brief handles the cJSON include and some small helpers
*/

#ifndef INCLUDE_JSON_H
#define INCLUDE_JSON_H

#include "../q_shared.h"

#ifdef BUNDLED_CJSON
#include "cJSON.h"
#else
#include <cjson/cJSON.h>
#endif

#define Q_JsonError(...) Com_Printf(S_COLOR_RED "[JSON-ERROR] " __VA_ARGS__)

/**
 * Initialize the Json library memory functions
 */
void Q_JSONInit(void);

/*
 * Parses the file to a json object, the calling code must call cJSON_delete on the returned object (if not null)
 */
cJSON *Q_FSReadJsonFrom(const char *path);

/**
 * Serialise a JSON object and write it to the specified file
 * @param object cJson object which to print out
 * @param path output path
 * @note closes the file handle
 * @return true if write process succeeded
 */
qboolean Q_FSWriteJSONTo(cJSON *object, const char *path);

/**
 * Serialise a JSON object and write it to the specified file
 * @param object cJson object which to print out
 * @param handle file handle to the already open file
 * @note closes the file handle
 * @return true if write process succeeded
 */
qboolean Q_FSWriteJSON(cJSON *object, fileHandle_t handle);

#define Q_ReadIntValueJson(x, y) (int) Q_ReadNumberValueJson(x, y)
#define Q_ReadFloatValueJson(x, y) (float) Q_ReadNumberValueJson(x, y)

/**
 * Get field by name and return the number value is available (defaults to 0)
 * @param object json object which to fetch the field from
 * @param name field name
 * @return double value of the field
 */
static ID_INLINE double Q_ReadNumberValueJson(cJSON *object, const char *name)
{
	cJSON *tmp = cJSON_GetObjectItem(object, name);

	if (tmp && cJSON_IsNumber(tmp))
	{
		return cJSON_GetNumberValue(tmp);
	}

	Q_JsonError("Missing field: %s\n", name);

	return 0;
}

static ID_INLINE char *Q_ReadStringValueJson(cJSON *object, const char *name)
{
	cJSON *tmp = cJSON_GetObjectItem(object, name);

	if (tmp && cJSON_IsString(tmp))
	{
		return cJSON_GetStringValue(tmp);
	}

	Q_JsonError("Missing field: %s\n", name);

	return 0;
}

static ID_INLINE qboolean Q_ReadBoolValueJson(cJSON *object, const char *name)
{
	cJSON *tmp = cJSON_GetObjectItem(object, name);

	if (tmp && cJSON_IsBool(tmp))
	{
		return cJSON_IsTrue(tmp) ? qtrue : qfalse;
	}

	Q_JsonError("Missing field: %s\n", name);

	return 0;
}

#endif
