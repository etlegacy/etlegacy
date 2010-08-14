/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Â“Wolf ET Source CodeÂ”).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Defines general to all Quake 3 engine games
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
#ifndef __MACOS__
#define __MACOS__   1       // needed for MrC
#endif
#define MAC_PORT    1

#if !TARGET_RT_MAC_MACHO
#define TARGET_RT_MAC_CFM   1
#endif

#define MAC_Q3      1

// Set this to 1 to make qgl* calls actual function links rather than #defines
#define MAC_QGL_LINKED  0

#ifdef __cplusplus
extern "C" {
#endif
extern int stricmp( const char *s1, const char *s2 );
extern int strnicmp( const char *s1, const char *s2, unsigned long n );
#define strcmpi stricmp
extern char * strupr( char * );
extern char * strlwr( char * );
extern char * itoa( int val, char *str, int radix );
#ifdef __cplusplus
}
#endif

//void OutputDebugString(const char * s);

#define _isnan      isnan
#define _snprintf   snprintf
#define _strnicmp   strnicmp
#define _stricmp    stricmp

#define __cdecl
#define __stdcall
#define __fastcall
#define _inline     inline
#define __forceinline inline
#define __max( a,b )  ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#define __min( a,b )  ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )

#include <limits.h> // LBO - for PATH_MAX, below
#include <stdio.h>

#ifdef __cplusplus
extern "C"
#endif
FILE * mac_fopen( const char *filename, const char * open_mode );

// LBO - This looks confusing and circular, so let's sort it out in English.

// Under Mach-O, in the Apple headers, PATH_MAX is defined, _MAX_PATH is not
// Under Mach-O, in the MSL headers, PATH_MAX is defined, _MAX_PATH is
// Under CFM, PATH_MAX isn't defined, _MAX_PATH is

#if __MWERKS__ // LBO - for _MAX_PATH
	#include <stdlib.h>
#endif

#ifndef _MAX_PATH
	#define _MAX_PATH       PATH_MAX
#else
	#ifndef PATH_MAX
		#define PATH_MAX    _MAX_PATH // LBO 12/28/03
	#endif

	#ifndef MAX_PATH
		#define MAX_PATH    _MAX_PATH
	#endif
#endif

#define fopen   mac_fopen

typedef const char      *LPCSTR, *PCSTR;
typedef LPCSTR PCTSTR, LPCTSTR;
typedef unsigned int UINT;
typedef long LONG;
typedef long INT32;
typedef unsigned short USHORT;
typedef unsigned long DWORD;
typedef int*            HANDLE;
typedef unsigned char BYTE;
typedef DWORD COLORREF;
typedef long long __int64;
typedef struct tagPOINT
{
	LONG x;
	LONG y;
} POINT;

#ifdef __MWERKS__
#pragma cpp_extensions on
#pragma gcc_extensions on
#endif

#include "MacPrefix_GameSpecific.h"
