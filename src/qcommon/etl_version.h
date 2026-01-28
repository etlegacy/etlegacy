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

#include "q_platform_os.h"

#ifndef INCLUDE_ETL_VERSION_H
#define INCLUDE_ETL_VERSION_H

#define PRODUCT_NAME            "etlegacy"
#define PRODUCT_LABEL           "ET Legacy"
#define PRODUCT_URL             "https://www.etlegacy.com"
#define CLIENT_WINDOW_TITLE     PRODUCT_LABEL
#define CLIENT_WINDOW_MIN_TITLE PRODUCT_LABEL

#define ETL_VERSION(major, minor, patch, commit) ((unsigned int)((((major) & 255) << 24) ^ (((minor) & 255) << 16) ^ (((patch) & 255) << 8) ^ ((commit) & 255)))

extern const char etlegacy_modname[];
extern const char etlegacy_modurl[];
extern const char etlegacy_omnibot_modname[];

extern const char etlegacy_version[];
extern const char etlegacy_version_short[];

/* Integral version (as produced by etlegacy_version_int). */
extern const unsigned long etlegacy_version_int;

/* Windows RC-style version tuple: { major, minor, patch, build }. */
extern const int  etlegacy_product_version[4];
extern const char etlegacy_product_version_str[];

extern const char etlegacy_product_build_time[];
extern const char etlegacy_product_build_features[];

extern const char q3_version[];
extern const char et_version[];

#endif // #ifndef INCLUDE_ETL_VERSION_H
