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

/**
 * Initialize the Json library memory functions
 */
void Q_JSONInit(void);

cJSON *Q_FSReadJsonFrom(const char *path);

qboolean Q_FSWriteJSONTo(cJSON *object, const char *path);

/**
 * Serialise a JSON object and write it to the specified file
 * @param object cJson object which to print out
 * @param handle file handle to the already open file
 */
qboolean Q_FSWriteJSON(cJSON *object, fileHandle_t handle);

#endif
