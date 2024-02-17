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
 * @file q_oss.h
 * @brief oss specific items
*/

#ifndef ETLEGACY_Q_OSS_H
#define ETLEGACY_Q_OSS_H

#include "q_shared.h"

typedef enum
{
	OSS_DEFAULT         = 0,        ///< 0 - vanilla/unknown/ET:L auto setup
	OSS_WIN_X86         = BIT(0),   ///< 1 - Windows x86
	OSS_LNX_X86         = BIT(1),   ///< 2 - Linux x86
	OSS_LNX_X86_64      = BIT(2),   ///< 4 - Linux x86_64
	OSS_MACOS_x86_64    = BIT(3),   ///< 8 - macOs (lets just assume for the sake of clarity that this is x86_64)
	OSS_ANDROID_AARCH64 = BIT(4),   ///< 16 - Android aarch64
	OSS_LNX_ARMV7       = BIT(5),   ///< 32 - Raspberry Pi arm
	OSS_LNX_ARMV8_64    = BIT(6),   ///< 64 - Raspberry Pi aarch 64
	OSS_MACOS_AARCH64   = BIT(7),   ///< 128 - macOS m1
	OSS_WIN_X86_64      = BIT(8),   ///< 256 - Windows x86_64
	OSS_ANDROID_X86     = BIT(9),   ///< 512 - Android x86
	OSS_ANDROID_X86_64  = BIT(10),  ///< 1024 - Android x86_64

	OSS_END                         ///< Moving "known platforms" index
} oss_t;

#define OSS_DEFAULT_SUPPORTED (OSS_WIN_X86 | OSS_LNX_X86)

#if defined(_WIN64) || defined(__WIN64__)
#define OSS_CURRENT_PLATFORM OSS_WIN_X86_64
#elif defined(_WIN32) || defined(__WIN32__)
#define OSS_CURRENT_PLATFORM OSS_WIN_X86
#elif defined(__APPLE__) || defined(__APPLE_CC__)
#if defined __arm64__
#define OSS_CURRENT_PLATFORM OSS_MACOS_AARCH64
#elif defined __x86_64__
#define OSS_CURRENT_PLATFORM OSS_MACOS_x86_64
#else
#define OSS_CURRENT_PLATFORM OSS_DEFAULT
#endif
#elif defined(__linux__) || defined(__FreeBSD_kernel__)
#if defined __i386__
#define OSS_CURRENT_PLATFORM OSS_LNX_X86
#elif defined __x86_64__
#define OSS_CURRENT_PLATFORM OSS_LNX_X86_64
#elif defined __arm__
#define OSS_CURRENT_PLATFORM OSS_LNX_ARMV7
#elif defined __aarch64__
#define OSS_CURRENT_PLATFORM OSS_LNX_ARMV8_64
#else
#define OSS_CURRENT_PLATFORM OSS_DEFAULT
#endif
#elif defined(__ANDROID__)
#if defined __i386__
#define OSS_CURRENT_PLATFORM OSS_ANDROID_X86
#elif defined __x86_64__
#define OSS_CURRENT_PLATFORM OSS_ANDROID_X86_64
#elif defined __aarch64__
#define OSS_CURRENT_PLATFORM OSS_ANDROID_AARCH64
#else
#define OSS_CURRENT_PLATFORM OSS_DEFAULT
#endif
#else
#define OSS_CURRENT_PLATFORM OSS_DEFAULT
#endif

#ifdef Q_OSS_STR_INC
// This must be kept in sync with the oss_t enum above
const char *oss_str[] = {
	"win_x86",         "lnx_x86",     "lnx_x86_64",   "macos_x86_64",
	"android_aarch64", "lnx_armv7",   "lnx_armv8_64", "macos_aarch64",
	"win_x86_64",      "android_x86", "android_x86_64"
};

#define OSS_KNOWN_COUNT ARRAY_LEN(oss_str)
#endif

#endif
