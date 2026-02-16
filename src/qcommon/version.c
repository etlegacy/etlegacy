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
 * @file version.c
 * @brief Defines build/version symbols from generated build metadata.
 */

#include "q_shared.h"
#include <version_generated.h>

const char etlegacy_version[]             = ETL_BUILD_VERSION;
const char etlegacy_version_short[]       = ETL_BUILD_VERSION_SHORT;
const int  etlegacy_version_int           = ETL_BUILD_VERSION_INT;
const char etlegacy_q3_version[]          = PRODUCT_LABEL " " ETL_BUILD_VERSION;
const char etlegacy_product_version_str[] = ETL_BUILD_PRODUCT_VERSION_STR;

#ifdef ETLEGACY_DEBUG
const char etlegacy_et_version[] = PRODUCT_LABEL " " ETL_BUILD_VERSION " " CPUSTRING " " __DATE__ " DEBUG";
#else
const char etlegacy_et_version[] = PRODUCT_LABEL " " ETL_BUILD_VERSION " " CPUSTRING " " __DATE__;
#endif

const char etlegacy_product_build_time[]     = ETL_BUILD_TIME;
const char etlegacy_product_build_features[] = ETL_BUILD_FEATURES;
