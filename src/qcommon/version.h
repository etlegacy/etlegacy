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
 * @file version.h
 * @brief Build version symbols exported from version.c.
 */

#ifndef INCLUDE_VERSION_H
#define INCLUDE_VERSION_H

extern const char etlegacy_version[];
extern const char etlegacy_version_short[];
extern const int  etlegacy_version_int;
extern const char etlegacy_q3_version[];
extern const char etlegacy_et_version[];
extern const char etlegacy_product_version_str[];
extern const char etlegacy_product_build_time[];
extern const char etlegacy_product_build_features[];

#define ETLEGACY_VERSION                ((char *)etlegacy_version)
#define ETLEGACY_VERSION_SHORT          ((char *)etlegacy_version_short)
#define ETLEGACY_VERSION_INT            etlegacy_version_int
#define Q3_VERSION                      ((char *)etlegacy_q3_version)
#define ET_VERSION                      ((char *)etlegacy_et_version)
#define PRODUCT_VERSION_STR             ((char *)etlegacy_product_version_str)
#define PRODUCT_BUILD_TIME              ((char *)etlegacy_product_build_time)
#define PRODUCT_BUILD_FEATURES          ((char *)etlegacy_product_build_features)
#define ETLEGACY_VERSION_IS_DEVELOPMENT_BUILD (ETLEGACY_VERSION_INT % 10000 > 0 /* the last 4 digits ought to be 0 on release builds */)

#endif
