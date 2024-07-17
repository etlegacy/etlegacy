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
* @file q_primitives.h
* @brief Primitives
*/

#ifndef INCLUDE_Q_PRIMITIVES_H
#define INCLUDE_Q_PRIMITIVES_H

#ifdef Q3_VM

#include "bg_lib.h"

typedef int intptr_t;

#else

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef __aarch64__ // ARM definition seems to not work here

#include <stddef.h>

#endif

#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#include <float.h>

#if defined (_MSC_VER) && (_MSC_VER >= 1600)
#include <stdint.h>

// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
int Q_vsnprintf(char *str, size_t size, const char *format, va_list args);
#elif defined (_MSC_VER)
#include <io.h>

typedef signed __int64 int64_t;
typedef signed __int32 int32_t;
typedef signed __int16 int16_t;
typedef signed __int8 int8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
int Q_vsnprintf(char *str, size_t size, const char *format, va_list args);
#else // not using MSVC

#include <stdint.h>

#define Q_vsnprintf vsnprintf
#endif // defined (_MSC_VER) && (_MSC_VER >= 1600)
#endif // Q3_VM


typedef unsigned char byte;

/**
 * @enum qboolean
 * @brief Boolean definition
 */
typedef enum
{
	qfalse, qtrue
} qboolean;

/**
 * @union floatint_t
 * @brief
 */
typedef union
{
	float f;
	int32_t i;
	uint32_t ui;
} floatint_t;

#endif // #ifndef INCLUDE_Q_PRIMITIVES_H
