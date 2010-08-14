/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

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

#ifndef macprefs_h
#define macprefs_h

#include <Carbon/Carbon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	UInt32 monitorFrequency;    // Menu selection # for the monitor refresh rate

	DisplayIDType displayID;
	Boolean button2fake;        // True if we turn ctrl-clicks into second mouse button clicks

	char serial[21];

} PrefsRec;

extern PrefsRec macPrefs;

#define kPrefsResource  'pref'
#define rPrefsVersion   1112    // IMPORTANT - increment this each time the PrefsRec changes

// also defined in mac.h
#define kPrefsFileType      'pref'
#define kPrefsStringType    'STR ' // resource type for our .ini strings

void LoadPrefs( PrefsRec *thePrefs );
void SavePrefs( PrefsRec *thePrefs );
void WritePrefsString( const char *name, const char *value );
Boolean ReadPrefsString( const char *name, char *value );

#ifdef __cplusplus
}
#endif

#endif