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
// Defines specifc to this game
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
#define kGameName           "Return to Castle Wolfenstein: Enemy Territory"
#define kGameNameP          "\pReturn to Castle Wolfenstein: Enemy Territory"
#define kCDName             "\pWolfenstein ET"

#define kGameFolderName     "Wolfenstein ET"
#define kGameCreator        'WlfE'
#define kGameBinaryType     'WlfB'

// This is used by the dedicated server to find the .bundle files in the main game app. It must
// match the full app package name.
#if MAC_DEBUG
	#define kMacExeName     "./Wolf ET Debug.app/"
#elif MAC_NOCD
	#define kMacExeName     "./Wolf ET (NoCD).app/"
#else
	#define kMacExeName     "./Wolfenstein ET.app/"
#endif

#if DEMO
#define kMacGameDir     "Demo"
#else
#define kMacGameDir     "etmain"
#endif

#if MAC_DEBUG
#define kMacCGameLib    "CGameLib Debug"
#define kMacGameLib     "QAGameLib Debug"
#define kMacUILib       "UILib Debug"
#else
#define kMacCGameLib    "CGameLib"
#define kMacGameLib     "QAGameLib"
#define kMacUILib       "UILib"
#endif

#define BOTLIB          1

#define MAC_WOLF        1
#define MAC_WOLF2_MP    1
#define MAC_WOLF_ET     1

#define MAC_Q3_MP       1

// This uses the older Q3 core APIs from our Mac code
#define MAC_Q3_OLDSTUFF 1

// Set this to 1 if the Q3 engine uses C linkage
#define MAC_Q3_C        1

#if MAC_Q3_C
#define Q3DEF extern "C"
#define Q3DEF_BEGIN extern "C" {
#define Q3DEF_END }
#else
#define Q3DEF
#define Q3DEF_BEGIN
#define Q3DEF_END
#endif
