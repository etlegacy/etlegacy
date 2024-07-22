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

#ifdef ETLEGACY_DEBUG
#define Q_JsonError(...) Com_Printf(S_COLOR_RED "[JSON-ERROR] " __VA_ARGS__)
#else
#define Q_JsonError(...)
#endif

/**
 * Initialize the Json library memory functions
 */
void Q_JSONInitWith(void *(*malloc_fn)(size_t sz), void (*free_fn)(void *ptr));

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

#define Q_ReadIntValueJson(object, name) (int) Q_ReadNumberValueJsonEx((object), (name), 0)
#define Q_ReadIntValueJsonEx(object, name, def) (int) Q_ReadNumberValueJsonEx((object), (name), (double) (def))

#define Q_ReadFloatValueJson(object, name) (float) Q_ReadFloatValueJsonEx((object), (name), 0.0f)
#define Q_ReadFloatValueJsonEx(object, name, def) (float) Q_ReadNumberValueJsonEx((object), (name), (double) (def))

#define Q_ReadNumberValueJson(object, name) (float) Q_ReadNumberValueJsonEx((object), (name), 0.0)

/**
 * Get field by name and return the number value if available (defaults to 0)
 * @param object json object which to fetch the field from
 * @param name field name
 * @return double value of the field
 */
static ID_INLINE double Q_ReadNumberValueJsonEx(cJSON *object, const char *name, double defaultValue)
{
	cJSON *tmp = cJSON_GetObjectItem(object, name);

	if (tmp && cJSON_IsNumber(tmp))
	{
		return cJSON_GetNumberValue(tmp);
	}

	return defaultValue;
}

#define Q_ReadStringValueJson(object, name) Q_ReadStringValueJsonEx((object), (name), NULL)

/**
 * Get field by name and return the string value if available otherwise returns the given default value.
 * @param object json object which to fetch the field from
 * @param name field name
 * @param defaultVal default value if field is not set
 * @return string value found
 */
static ID_INLINE char *Q_ReadStringValueJsonEx(cJSON *object, const char *name, char *defaultVal)
{
	cJSON *tmp = cJSON_GetObjectItem(object, name);

	if (tmp && cJSON_IsString(tmp))
	{
		return cJSON_GetStringValue(tmp);
	}

	return defaultVal;
}

#define Q_ReadBoolValueJson(object, name) Q_ReadBoolValueJsonEx((object), (name), qfalse)

/**
 * Get field by name and return the boolean value if available otherwise returns the given default value.
 * @param object json object which to fetch the field from
 * @param name field name
 * @param defaultVal default value if field is not set
 * @return boolean value found
 */
static ID_INLINE qboolean Q_ReadBoolValueJsonEx(cJSON *object, const char *name, qboolean defaultVal)
{
	cJSON *tmp = cJSON_GetObjectItem(object, name);

	if (tmp && cJSON_IsBool(tmp))
	{
		return cJSON_IsTrue(tmp) ? qtrue : qfalse;
	}

	return defaultVal;
}

#endif
