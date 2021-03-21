/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 * @file q_platform.h
 * @brief Platform specific definitions
 */

#ifndef INCLUDE_Q_PLATFORM_H
#define INCLUDE_Q_PLATFORM_H

// this is for determining if we have an asm version of a C function
#define idx64 0

#ifdef Q3_VM

#define id386 0
#define idppc 0
#define idppc_altivec 0
#define idsparc 0

#else

#if (defined _M_IX86 || defined __i386__) && !defined(C_ONLY)
#define id386 1
#else
#define id386 0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || \
    defined(__ppc) || defined(__ppc__) || defined(__PPC__) || \
    defined(__POWERPC__)) && !defined(C_ONLY)
#define idppc 1
#if defined(__VEC__) || defined(__ALTIVEC__) || defined(__APPLE_ALTIVEC__)
#define idppc_altivec 1
#ifdef __APPLE__  // Apple's GCC does this differently than the FSF.
#define VECCONST_UINT8(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
	(vector unsigned char) (a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)
#else
#define VECCONST_UINT8(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
	(vector unsigned char) { a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p }
#endif
#else
#define idppc_altivec 0
#endif
#else
#define idppc 0
#define idppc_altivec 0
#endif

#if defined(__sparc__) && !defined(C_ONLY)
#define idsparc 1
#else
#define idsparc 0
#endif

#endif

#ifndef __ASM_I386__ // don't include the C bits if included from qasm.h

// for windows fastcall option
#define QDECL
#define QCALL

//================================================================= WIN64/32 ===

#if defined(_WIN64) || defined(__WIN64__)

#undef idx64
#define idx64 1

#undef QDECL
#define QDECL __cdecl

#undef QCALL
#define QCALL __stdcall

#if defined(_MSC_VER)
#define OS_STRING "win_msvc64"
#elif defined __MINGW64__
#define OS_STRING "win_mingw64"
#endif

#define ID_INLINE __inline
#define PATH_SEP '\\'

#if defined(_WIN64)
#define ARCH_STRING "x64"
#elif defined _M_ALPHA
#define ARCH_STRING "AXP"
#endif

#define Q3_LITTLE_ENDIAN

#define DLL_EXT ".dll"

#elif defined(_WIN32) || defined(__WIN32__)

#undef QDECL
#define QDECL __cdecl

#undef QCALL
#define QCALL __stdcall

#if defined(_MSC_VER)
#define OS_STRING "win_msvc"
#elif defined __MINGW32__
#define OS_STRING "win_mingw"
#endif

#define ID_INLINE __inline
#define PATH_SEP '\\'

#if defined(_M_IX86) || defined(__i386__)
#define ARCH_STRING "x86"
#elif defined _M_ALPHA
#define ARCH_STRING "AXP"
#endif

#define Q3_LITTLE_ENDIAN

#define DLL_EXT ".dll"

#endif


//================================================================= MAC OS ===

#if defined(__APPLE__) || defined(__APPLE_CC__)

#define OS_STRING "macos"
#define ID_INLINE __inline
#define PATH_SEP '/'

#ifdef __ppc__
#define ARCH_STRING "ppc"
#define Q3_BIG_ENDIAN
#elif defined __i386__
#define ARCH_STRING "i386"
#define Q3_LITTLE_ENDIAN
#elif defined __x86_64__
#undef idx64
#define idx64 1
#define ARCH_STRING "x86_64"
#define Q3_LITTLE_ENDIAN
#elif defined __arm64__
#undef idx64
#define idx64 1
#define ARCH_STRING "arm64"
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_EXT "_mac"

#endif

//================================================================= LINUX ===

#if defined(__linux__) || defined(__FreeBSD_kernel__)

#include <endian.h>

#if defined(__linux__)
#define OS_STRING "linux"
#else
#define OS_STRING "kFreeBSD"
#endif

#define ID_INLINE inline
#define PATH_SEP '/'

#if defined __i386__
#define ARCH_STRING "i386"
#elif defined __x86_64__
#undef idx64
#define idx64 1
#define ARCH_STRING "x86_64"
#elif defined __powerpc64__
#define ARCH_STRING "ppc64"
#elif defined __powerpc__
#define ARCH_STRING "ppc"
#elif defined __s390__
#define ARCH_STRING "s390"
#elif defined __s390x__
#define ARCH_STRING "s390x"
#elif defined __ia64__
#define ARCH_STRING "ia64"
#elif defined __alpha__
#define ARCH_STRING "alpha"
#elif defined __sparc__
#define ARCH_STRING "sparc"
#elif defined __arm__
#define ARCH_STRING "arm" // __ARM_ARCH_'V'__ FIXME: add ARM version to the ARCH_STRING
#elif defined __cris__
#define ARCH_STRING "cris"
#elif defined __hppa__
#define ARCH_STRING "hppa"
#elif defined __mips__
#define ARCH_STRING "mips"
#elif defined __sh__
#define ARCH_STRING "sh"
#endif

#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_EXT ".so"

#endif

//=================================================================== BSD ===

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#include <sys/types.h>
#include <machine/endian.h>

#ifndef __BSD__
  #define __BSD__
#endif

#if defined(__FreeBSD__)
#define OS_STRING "freebsd"
#elif defined(__OpenBSD__)
#define OS_STRING "openbsd"
#elif defined(__NetBSD__)
#define OS_STRING "netbsd"
#endif

#define ID_INLINE inline
#define PATH_SEP '/'

#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __amd64__
#undef idx64
#define idx64 1
#define ARCH_STRING "x86_64"
#elif defined __axp__
#define ARCH_STRING "alpha"
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_EXT ".so"

#endif

//================================================================= SUNOS ===

#ifdef __sun

#include <stdint.h>
#include <sys/byteorder.h>

#define OS_STRING "solaris"
#define ID_INLINE inline
#define PATH_SEP '/'

#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __sparc
#define ARCH_STRING "sparc"
#endif

#if defined(_BIG_ENDIAN)
#define Q3_BIG_ENDIAN
#elif defined(_LITTLE_ENDIAN)
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_EXT ".so"

#endif

//================================================================== IRIX ===

#ifdef __sgi

#define OS_STRING "irix"
#define ID_INLINE __inline
#define PATH_SEP '/'

#define ARCH_STRING "mips"

#define Q3_BIG_ENDIAN // SGI's MIPS are always big endian

#define DLL_EXT ".so"

#endif

//================================================================== Q3VM ===

#ifdef Q3_VM

#define OS_STRING "q3vm"
#define ID_INLINE
#define PATH_SEP '/'

#define ARCH_STRING "bytecode"

#define DLL_EXT ".qvm"

#endif

//=============================================================== ANDROID ===

#ifdef __ANDROID__

#ifndef OS_STRING
  #define OS_STRING "android"
#endif
#define ID_INLINE inline
#define PATH_SEP '/'

#if defined __arm__
      #undef ARCH_STRING
      #define ARCH_STRING "armeabi-v7a"
#elif defined __aarch64__
      #define ARCH_STRING "arm64-v8a"
#endif

#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
#define Q3_BIG_ENDIAN
#else
#define Q3_LITTLE_ENDIAN
#endif

#define DLL_EXT ".so"

#endif

//===========================================================================

//catch missing defines in above blocks
#if !defined(OS_STRING)
#error "Operating system not supported"
#endif

#if !defined(ARCH_STRING)
#error "Architecture not supported"
#endif

#ifndef ID_INLINE
#error "ID_INLINE not defined"
#endif

#ifndef PATH_SEP
#error "PATH_SEP not defined"
#endif

#ifndef DLL_EXT
#error "DLL_EXT not defined"
#endif


//endianness
short ShortSwap(short l);
int LongSwap(int l);
float FloatSwap(const float *f);

#if defined(Q3_BIG_ENDIAN) && defined(Q3_LITTLE_ENDIAN)
#error "Endianness defined as both big and little"
#elif defined(Q3_BIG_ENDIAN)

#define LittleShort(x) ShortSwap(x)
#define LittleLong(x) LongSwap(x)
#define LittleFloat(x) FloatSwap(&x)
#define BigShort
#define BigLong
#define BigFloat

#elif defined(Q3_LITTLE_ENDIAN)

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort(x) ShortSwap(x)
#define BigLong(x) LongSwap(x)
#define BigFloat(x) FloatSwap(&x)

#elif defined(Q3_VM)

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort
#define BigLong
#define BigFloat

#else
#error "Endianness not defined"
#endif


//platform string
#ifndef ETLEGACY_DEBUG
#define PLATFORM_STRING OS_STRING "-" ARCH_STRING
#else
#define PLATFORM_STRING OS_STRING "-" ARCH_STRING "-debug"
#endif

#endif

#define LL(x) x = LittleLong(x)
#define LF(x) x = LittleFloat(x)

#endif // #ifndef INCLUDE_Q_PLATFORM_H
