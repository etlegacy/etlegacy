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
#ifndef INCLUDE_Q_PLATFORM_OS_H
#define INCLUDE_Q_PLATFORM_OS_H

//======================= WIN32 DEFINES =================================

#ifdef _WIN32

#undef QDECL
#define QDECL   __cdecl

/**
 * @def CPUSTRING
 * @brief Platform and architecture string incorporated into the version string.
 *
 * To maintain compatibility with ET 2.60b mods these values must be set:
 *      windows: win-x86
 *      linux:   linux-i386
 *      mac:     ?
 *
 * See FAKE_VERSION
 */
#ifdef _WIN64
#define CPUSTRING   "win-x64"
#else
#define CPUSTRING   "win-x86"
#endif

#define PATH_SEP '\\'

#endif // _WIN32

//======================= MAC OS DEFINES =================================

#if defined(__APPLE__)

#define CPUSTRING   "MacOS" // TODO: check if some mods depend on the old Mac CPUSTRING
// #define CPUSTRING   "OSX-universal" // old

#define PATH_SEP    '/'

/*

// Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
// runs *much* faster than calling sqrt(). We'll use two Newton-Raphson
// refinement steps to get bunch more precision in the 1/sqrt() value for very little cost.
// We'll then multiply 1/sqrt times the original value to get the sqrt.
// This is about 12.4 times faster than sqrt() and according to my testing (not exhaustive)
// it returns fairly accurate results (error below 1.0e-5 up to 100000.0 in 0.1 increments).

// Note ET:L uses BSD Library functions for this

static ID_INLINE float idSqrt(float x)
{
    const float half = 0.5;
    const float one  = 1.0;
    float       B, y0, y1;

    // This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
    if (Q_fabs(x) == 0.0)
    {
        return x;
    }
    B = x;

#ifdef __GNUC__
    asm ("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
    y0 = __frsqrte(B);
#endif
    // First refinement step

    y1 = y0 + half * y0 * (one - B * y0 * y0);

    // Second refinement step -- copy the output of the last step to the input of this step

    y0 = y1;
    y1 = y0 + half * y0 * (one - B * y0 * y0);

    // Get sqrt(x) from x * 1/sqrt(x)
    return x * y1;
}
#define sqrt idSqrt
*/

#endif // defined(__APPLE__)

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __x86_64__
#define CPUSTRING   "linux-x86_64"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#elif defined __arm__
#define CPUSTRING   "linux-arm"
#elif defined __aarch64__
#define CPUSTRING   "linux-aarch64"
#else
#define CPUSTRING   "linux-other"
#endif

#define PATH_SEP '/'

#endif // __linux__

//======================= OPENBSD DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __OpenBSD__

#ifdef __i386__
#define CPUSTRING   "openbsd-i386"
#elif defined __x86_64__
#define CPUSTRING   "openbsd-x86_64"
#else
#define CPUSTRING   "openbsd-other"
#endif

#define PATH_SEP '/'

#endif // __OpenBSD__

//======================= FREEBSD DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __FreeBSD__

#ifdef __i386__
#define CPUSTRING   "freebsd-i386"
#elif defined __x86_64__
#define CPUSTRING   "freebsd-x86_64"
#else
#define CPUSTRING   "freebsd-other"
#endif

#define PATH_SEP '/'

#endif // __FreeBSD__

//======================= NETBSD DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __NetBSD__

#ifdef __i386__
#define CPUSTRING   "netbsd-i386"
#elif defined __x86_64__
#define CPUSTRING   "netbsd-x86_64"
#else
#define CPUSTRING   "netbsd-other"
#endif

#define PATH_SEP '/'

#endif //  __NetBSD__

//======================= ANDROID DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __ANDROID__

#ifdef __arm__
#undef CPUSTRING
#define CPUSTRING   "android-armeabi-v7a"
#elif defined __aarch64__
#undef CPUSTRING
#define CPUSTRING   "android-arm64-v8a"
#endif

#define PATH_SEP '/'

#endif // __ANDROID__


#endif /* end of include guard: INCLUDE_Q_PLATFORM_OS_H */
