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


/*
 *	Apple Universal Headers 3.1
 *
 *	Uncomment any additional #includes you want to add to your MacHeaders.
 */
//#include <MacHeaders.h>

#ifdef verify
#undef verify
#endif

#include <Carbon/Carbon.h>

#if !DEDICATED
#include <OpenGL/gl.h>
#include <AGL/agl.h>
#endif
#include <math.h>
#include <time.h>
#include <errno.h>

/* Menus: */
	#define rMenuBar    128
	#define rMenuBarX   129
/* Apple menu: */
		#define mApple  128
		#define iAbout  1
/* File menu: */
		#define mFile   129
		#define iQuit   1
/* Edit menu: */
		#define mEdit   130
		#define iUndo   1
		#define iCut    3
		#define iCopy   4
		#define iPaste  5
		#define iClear  6
/* Options menu: */
		#define mOptions 131
		#define iMouse  1
		#define iHideWindow 2
/* Windows: */
	#define kMainWindow     128
	#define kFullScreenWindow       129
/* Dilogs: */
	#define kAboutDialog    128

// Set this to use Carbon timers on MacOS 8/9
#define CARBON_TIMERS_ON_9 0

enum {
	rErrStrings = 128,
	rMiscStrings = 129,

	kErrStringError = 1,
	kErrStringWarning,
	kErrStringDrawSprocket,
	kErrStringQuickTime,
	kErrStringWrongOS,
	kErrStringWrongOSX,
	kErrStringMisc,
	kErrStringCDKey,
	kErrStringCarbonLib
};

// Fixed-point refresh frequency values
#define kFrequency60 0x003c0000 // 60Hz, fixed point
#define kFrequency67 0x00430000 // 67Hz, fixed point
#define kFrequency70 0x00460000 // 70Hz, fixed point
#define kFrequency72 0x00480000 // 72Hz, fixed point
#define kFrequency75 0x004b0000 // 75Hz, fixed point
#define kFrequency80 0x00500000 // 80Hz, fixed point
#define kFrequency85 0x00550000 // 85Hz, fixed point
#define kFrequency90 0x005a0000 // 90Hz, fixed point
#define kFrequency99 0x00630000 // 99Hz, fixed point
#define kFrequency100 0x00640000 // 100Hz, fixed point
#define kFrequency120 0x00780000 // 120Hz, fixed point
#define kFrequency124 0x007c0000 // 124Hz, fixed point

enum {
	// Menu items - refresh rate menu
	kMenuRefreshAuto = 1,
	//---
	kMenuRefresh60 = 3,
	kMenuRefresh67,
	kMenuRefresh70,
	kMenuRefresh75,
	kMenuRefresh80,
	kMenuRefresh85,
	kMenuRefresh90,
	kMenuRefresh99,
	kMenuRefresh100,
	kMenuRefresh120,
	kMenuRefresh124
};

extern Fixed gRefreshRate;

// mac_main.c
extern int sys_ticBase;
extern int sys_msecBase;
extern int sys_lastEventTic;
extern int sys_CDinDrive;
extern long gSystemVersion;
extern Boolean gUseCarbonEvents;

// GetKeys character codes
#define kCommandKey 0x37
#define kShiftKey 0x38
#define kOptionKey 0x3a
#define kControlKey 0x3b

#include "../qcommon/qcommon.h"

#ifdef __cplusplus
extern "C" {
#endif

void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
void Sys_Error( const char *format, ... );
void Sys_Warning( const char *error, ... );

#ifdef __cplusplus
}
#endif

void OutputDebugString( const char *s );
//qboolean VIDEO_Open(char *psPathlessBaseName, qboolean qbInGame, qboolean qbTestOpenOnly, int iLanguageNumber);

// mac_glimp.c
extern glconfig_t glConfig;

#if MAC_Q3_MP
//�LBO - put this in tr_types.h for single-player apps
typedef struct
{
	qboolean clampToEdgeAvailable;
	qboolean textureFilterAnisotropicAvailable;
	qboolean multisampleAvailable;
	qboolean hasClientStorage;
} glconfig_mac_t;

extern glconfig_mac_t glConfig_Mac;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// mac_files.c
Boolean AmIBundled( void );
OSErr GetApplicationPackageFSSpecFromBundle( FSSpecPtr theFSSpecPtr );
void Mac_GetOSPath( short inDomain, OSType inFolderType, char *outPath );
void GetPosixPathFromHFS( const char *inPath, char *outPath );
extern const char *kPathSep;

// mac_glimp.c
void GLimp_SetGameGamma( void );
#if !MAC_JKJA || MAC_Q3_MP
void VID_Printf( int print_level, const char *fmt, ... );
#endif

// mac_event.c
extern int vkeyToQuakeKey[256];
void Sys_SendKeyEvents( void );
void Carbon_InstallTimer( void );
void Carbon_RemoveTimer( void );
void Carbon_InstallEvents( void );

// mac_net.c
void Sys_InitNetworking( void );
void Sys_ShutdownNetworking( void );
qboolean Sys_GetPacket( netadr_t *net_from, msg_t *net_message );

// mac_input.c
void Sys_InitInput( void );
void Sys_ShutdownInput( void );
void Sys_SuspendInput( void );
void Sys_ResumeInput( void );
void Sys_Input( void );

void IN_Init( void );
void IN_Shutdown( void );
void IN_Frame( void );

extern Boolean inputActive;
extern Boolean inputMouseInWindow;

// mac_console.c
Boolean ConsoleWindowIsFrontmost( void );

void    Sys_CreateConsole( void );
void    Sys_ShowConsole( int level, qboolean quitOnClose );
void    Sys_Print( const char *text );
char    *Sys_ConsoleInput( void );
qboolean Sys_ConsoleEvent( EventRecord *event, RgnHandle cursRgn );

void Sys_SendStringToConsole( const char *inString );

#ifdef __cplusplus
}
#endif

