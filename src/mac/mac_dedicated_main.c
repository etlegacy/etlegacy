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

#if MAC_ALICE || MAC_STVEF_HM
	#ifdef __cplusplus
extern "C" {
	#endif
#endif

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "mac_local.h"
#include "g_public.h"
#include "unzip.h"

#if MAC_ALICE || MAC_STVEF_HM
	#ifdef __cplusplus
}
	#endif
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define bzero( a,b ) memset( a,0,b )

cvar_t          *pagememory;
long gSystemVersion;
qboolean sys_copy   = qfalse;

static char sys_cmdline[MAX_STRING_CHARS];
clientStatic_t cls;



void *Sys_GetBotAIAPI( void *parms ) {
	return NULL;
}

void Conbuf_AppendText( const char *pMsg ) {
	char msg[4096];
	strcpy( msg, pMsg );
	printf( Q_CleanStr( msg ) );
	printf( "\n" );
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory( void ) {
	return qfalse;
}


/*
===============
PrintMatches

===============
*/
static char g_consoleField1[256];
static char g_consoleField2[256];

static void PrintMatches( const char *s ) {
	if ( !Q_stricmpn( s, g_consoleField1, strlen( g_consoleField1 ) ) ) {
		printf( "    %s\n", s );
	}
}

qboolean stdin_active = qtrue;
char *Sys_ConsoleInput( void ) {
#if 1
	static char text[256];
	int len;
	fd_set fdset;
	struct timeval timeout;

#ifndef DEDICATED
	if ( !com_dedicated || !com_dedicated->value ) {
		return NULL;
	}
#endif

	if ( !stdin_active ) {
		return NULL;
	}

	FD_ZERO( &fdset );
	FD_SET( 0, &fdset ); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if ( select( 1, &fdset, NULL, NULL, &timeout ) == -1 || !FD_ISSET( 0, &fdset ) ) {
		return NULL;
	}

	len = read( 0, text, sizeof( text ) );
	if ( len == 0 ) { // eof!
		stdin_active = qfalse;
		return NULL;
	}

	if ( len < 1 ) {
		return NULL;
	}
	text[len - 1] = 0;    // rip off the /n and terminate

	return text;
#else
	const char ClearLine[] = "\r                                                                               \r";

	static int len = 0;
	static bool bPendingExtended = false;

	if ( !kbhit() ) {
		return NULL;
	}

	if ( len == 0 ) {
		memset( g_consoleField1,0,sizeof( g_consoleField1 ) );
	}

	g_consoleField1[len] = getch();

	if ( bPendingExtended ) {
		switch ( g_consoleField1[len] )
		{
		case 'H':       //up
			strcpy( g_consoleField1, g_consoleField2 );
			printf( ClearLine );
			printf( "%s",g_consoleField1 );
			len = strlen( g_consoleField1 );
			break;

		case 'K':       //left
			break;

		case 'M':       //right
			break;

		case 'P':       //down
			break;
		}
		g_consoleField1[len] = 0;   //erase last key hit
		bPendingExtended = false;
	} else {
		switch ( (unsigned char) g_consoleField1[len] )
		{
		case 0x00: //fkey is next
		case 0xe0: //extended = arrow keys
			g_consoleField1[len] = 0; //erase last key hit
			bPendingExtended = true;
			break;
		case 8: // backspace
			printf( "%c %c",g_consoleField1[len],g_consoleField1[len] );
			g_consoleField1[len] = 0;
			if ( len > 0 ) {
				len--;
			}
			g_consoleField1[len] = 0;
			break;
		case 9: //Tab
			if ( len ) {
				g_consoleField1[len] = 0; //erase last key hit
				printf( "\n" );
				// run through again, printing matches
				Cmd_CommandCompletion( PrintMatches );
				Cvar_CommandCompletion( PrintMatches );
				printf( "\n%s", g_consoleField1 );
			}
			break;
		case 27: // esc
			// clear the line
			printf( ClearLine );
			len = 0;
			break;
		case '\r': //enter
			g_consoleField1[len] = 0; //erase last key hit
			printf( "\n" );
			if ( len ) {
				len = 0;
				strcpy( g_consoleField2, g_consoleField1 );
				return g_consoleField1;
			}
			break;
		case 'v' - 'a' + 1: // ctrl-v is paste
			g_consoleField1[len] = 0; //erase last key hit
			char *cbd;
			cbd = Sys_GetClipboardData();
			if ( cbd ) {
				strncpy( &g_consoleField1[len], cbd, sizeof( g_consoleField1 ) );
				printf( "%s",cbd );
				len += strlen( cbd );
				Z_Free( cbd );
				if ( len == sizeof( g_consoleField1 ) ) {
					len = 0;
					return g_consoleField1;
				}
			}
			break;
		default:
			printf( "%c",g_consoleField1[len] );
			len++;
			if ( len == sizeof( g_consoleField1 ) ) {
				len = 0;
				return g_consoleField1;
			}
			break;
		}
	}

	return NULL;
#endif
}

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose ) {
}

/*
=================
Sys_Shutdown
=================
*/
void Sys_Shutdown( void ) {
#if MAC_Q3_MP
	Sys_ShutdownNetworking();
#endif
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error( const char *error, ... ) {
	va_list argptr;
	char text[4096];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

//	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	Sys_Shutdown();

	// LBO - we do an ExitToShell here because exit can call some destructors that
	// are in rough shape if the app hasn't started up properly.
	ExitToShell();
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	Sys_Shutdown();

	ExitToShell();
}


/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	printf( "%s", msg );
}

void Sys_DebugPrint( const char *msg ) {
//¥¥¥	OutputDebugString( msg );
}


/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/
qboolean    Sys_CheckCD( void ) {
#ifdef FINAL_BUILD
//	return Sys_ScanForCD();
#else
	return qtrue;
#endif
}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	ScrapRef currentScrap;
	OSStatus status;
	char *buffer;
	Size byteCount;

	GetCurrentScrap( &currentScrap );

	status = GetScrapFlavorSize( currentScrap, 'TEXT', &byteCount );
	if ( ( status == noErr ) && ( byteCount < 1023 ) ) {
		buffer = (char *)Z_Malloc( byteCount + 1 );
		if ( !buffer ) {
			return NULL;
		}

		// Get the 'TEXT' clipping from the clipboard
		status = GetScrapFlavorData( currentScrap, 'TEXT', &byteCount, (void *)buffer );
	}
	return buffer;
}

#if MAC_JKJA && MAC_Q3_MP
inline int Old_Sys_Milliseconds( void ) {
	AbsoluteTime t;
	Nanoseconds nano;
	double doub;

#define kTwoPower32 ( 4294967296.0 )      /* 2^32 */

	t = UpTime();
	nano = AbsoluteToNanoseconds( t );
	doub = ( ( (double) nano.hi ) * kTwoPower32 ) + nano.lo;

	return doub * 0.000001;
}

int Sys_Milliseconds( bool baseTime ) {
	static int sys_timeBase = Old_Sys_Milliseconds();
	int sys_curtime;

	sys_curtime = Old_Sys_Milliseconds();
	if ( !baseTime ) {
		sys_curtime -= sys_timeBase;
	}

	return sys_curtime;
}
#else
int Sys_Milliseconds( void ) {
	AbsoluteTime t;
	Nanoseconds nano;
	double doub;

#define kTwoPower32 ( 4294967296.0 )      /* 2^32 */

	t = UpTime();
	nano = AbsoluteToNanoseconds( t );
	doub = ( ( (double) nano.hi ) * kTwoPower32 ) + nano.lo;

	return doub * 0.000001;
}
#endif

void Sys_SnapVector( float *v ) {
	v[0] = rint( v[0] );
	v[1] = rint( v[1] );
	v[2] = rint( v[2] );
}

#if MAC_WOLF_ET
float Sys_GetCPUSpeed( void ) {
	float cpuSpeed;

	OSErr err;
	long response;

	err = Gestalt( gestaltProcClkSpeed, &response );
	if ( !err ) {
		cpuSpeed = (float)response / 1000.0f;
	} else {
		cpuSpeed = 500.0f;
	}

	return cpuSpeed;
}
#endif

#pragma mark -
void DisableAutodial() {
	//NOT IMPLMENTED
}

void RestoreAutodial() {
	//NOT IMPLMENTED
}

void RecoverLostAutodialData() {
	//NOT IMPLMENTED
}

void SetNormalThreadPriority() {
	//NOT IMPLMENTED
}

void SetBelowNormalThreadPriority() {
	//NOT IMPLMENTED
}

qboolean LoadRegistryInfo( qboolean user, const char *pszName, void *pvBuf, long *plSize ) {
	//NOT IMPLMENTED
	return qfalse;
}

qboolean SaveRegistryInfo( qboolean user, const char *pszName, void *pvBuf, long lSize, qboolean bString ) {
	//NOT IMPLMENTED
	return qfalse;
}

void Sys_CloseMutex( void ) {
}

/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

#pragma mark -

CFragConnectionID game_connID;
CFBundleRef gameBundleRef = NULL;

typedef void ( *dllEntryPtr )( int ( *syscallptr )( int, ... ) );
typedef int ( *systemCallsPtr )( int, ... );
typedef int ( *vmMainPtr )( int, ... );
typedef void *( *getGameAPIPtr )( void * );

typedef struct
{
	void *procAddr;
	void *TOCAddr;
} TVector;

typedef struct
{
	// For CFM shared libraries
	CFragConnectionID connID;

	// For Mach-O bundles
	CFBundleRef bundleRef;
	CFURLRef bundleURL;
	systemCallsPtr cfmGlue;
	dllEntryPtr cfmGlue2;
	vmMainPtr cfmGlue3;
	TVector tvector;
} dll_t;

//
//	This function allocates a block of CFM glue code which contains the instructions to call CFM routines
//
UInt32 sGlueCode[6] = {0x3D800000, 0x618C0000, 0x800C0000, 0x804C0004, 0x7C0903A6, 0x4E800420};

void *MachOFunctionPointerForCFMFunctionPointer( void *cfmfp ) {
	UInt32  *mfp = (UInt32*) NewPtr( sizeof( sGlueCode ) );       //	Must later dispose of allocated memory
	mfp[0] = sGlueCode[0] | ( (UInt32)cfmfp >> 16 );
	mfp[1] = sGlueCode[1] | ( (UInt32)cfmfp & 0xFFFF );
	mfp[2] = sGlueCode[2];
	mfp[3] = sGlueCode[3];
	mfp[4] = sGlueCode[4];
	mfp[5] = sGlueCode[5];
	MakeDataExecutable( mfp, sizeof( sGlueCode ) );
	return( mfp );
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// ¥ Sys_UnpackDLL
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Tries to find a named DLL in the pk3 hierarchy. If found, it copies it, and then
// attempts to treat the copied file as a zip file, unzipping it to the temp directory.
// This should only be called when looking for a Mach-O .bundle DLL.

Boolean Sys_UnpackDLL( const char *name ) {
	void *data;
	FILE *macFile;
	int len = FS_ReadFile( name, &data );
	int ck;
	char zipPath[PATH_MAX];
	char macFolderPath[PATH_MAX];

	if ( len < 1 ) { //failed to read the file (out of the pk3 if pure)
		return false;
	}

	if ( FS_FileIsInPAK( name, &ck ) == -1 ) {
		FS_FreeFile( data );
		return false;
	}

	// We create our temp copy of the DLL zip file outside the Quake3 folder hierarchy
	// so that it's not found and used the next time through (which might mark it as unpure).
	Mac_GetOSPath( kUserDomain, kTemporaryFolderType, macFolderPath );
	sprintf( zipPath, "%s%s%s", macFolderPath, kPathSep, kMacGameDir );
	Sys_Mkdir( zipPath );
	sprintf( zipPath, "%s%s%s%s%s", macFolderPath, kPathSep, kMacGameDir, kPathSep, name );

	macFile = fopen( zipPath, "wb" );
	if ( !macFile ) {
		//can't open for writing? Might be in use.
		//This is possibly a malicious user attempt to circumvent dll
		//replacement so we won't allow it.
		FS_FreeFile( data );
		return false;
	}

	if ( fwrite( data, 1, len, macFile ) < len ) {
		//Failed to write the full length. Full disk maybe?
		FS_FreeFile( data );
		fclose( macFile );
		return false;
	}

	fclose( macFile );

	// LBO - now try and unzip the file
	{
		unzFile zipFile;
		unz_global_info zipInfo;
		int err;
		int i;
		void *buf = NULL;

		zipFile = unzOpen( zipPath );
		if ( !zipFile ) {
			return false;
		}

		err = unzGetGlobalInfo( zipFile, &zipInfo );
		if ( err != UNZ_OK ) {
			return false;
		}

		err = unzGoToFirstFile( zipFile );

		for ( i = 0; i < zipInfo.number_entry; i++ )
		{
			unz_file_info file_info;
			char newFileName[256];
			char newFilePath[PATH_MAX];
			FILE *newFile;
			int j;

			err = unzGetCurrentFileInfo( zipFile, &file_info, newFileName, sizeof( newFileName ), NULL, 0, NULL, 0 );

			for ( j = 0; j < strlen( newFileName ); j++ )
			{
				if ( newFileName[j] == '/' ) {
					newFileName[j] = kPathSep[0];
				}
			}

			sprintf( newFilePath, "%s%s%s%s%s", macFolderPath, kPathSep, kMacGameDir, kPathSep, newFileName );

			if ( newFilePath[strlen( newFilePath ) - 1] == kPathSep[0] ) {
				newFilePath[strlen( newFilePath ) - 1] = 0;
				Sys_Mkdir( newFilePath );
			} else
			{
				newFile = fopen( newFilePath,"wb" );
				if ( !newFile ) {
					break;
				}

				if ( buf ) {
					free( buf );
				}
				buf = malloc( file_info.uncompressed_size );
				if ( !buf ) {
					break;
				}

				err = unzOpenCurrentFile( zipFile );
				if ( err ) {
					break;
				}

				err = unzReadCurrentFile( zipFile, buf, file_info.uncompressed_size );
				if ( ( err < 0 ) || ( err != file_info.uncompressed_size ) ) {
					break;
				}

				err = fwrite( buf, file_info.uncompressed_size, 1, newFile );
				if ( err != 1 ) {
					break;
				}

				fclose( newFile );

				err = unzCloseCurrentFile( zipFile );
			}

			err = unzGoToNextFile( zipFile );
			if ( err ) {
				break;
			}
		}

		if ( buf ) {
			free( buf );
		}
	}

	FS_FreeFile( data );

	remove( zipPath );

	return true;
}

#if MAC_WOLF2_MP
char* Sys_GetDLLName( const char *name ) {
#if MAC_DEBUG
	return va( "%s_d_mac", name );
#else
	return va( "%s_mac", name );
#endif
}
#endif

#if MAC_WOLF2_MP
#pragma mark Sys_LoadDll
void *Sys_LoadDll( const char *name, char *fqpath, int( **entryPoint ) ( int, ... ), int ( *systemCalls )( int, ... ) )
#else
void *Sys_LoadDll( const char *name, int( **entryPoint ) ( int, ... ), int ( *systemCalls )( int, ... ) )
#endif
{
	OSErr err = noErr;
	FSSpec SLSpec;
	char name2[255];
	dllEntryPtr dllEntry = NULL;
	dll_t *dllSpec;

	dllSpec = (dll_t *) calloc( sizeof( dll_t ), 1 );
	if ( dllSpec == NULL ) {
		goto cantLoad;
	}

	err = noErr;

	Com_sprintf( name2, sizeof( name2 ), "%s", Sys_GetDLLName( name ) );

	Boolean didLoad;
	// Try Mach-O first.
tryMachO:
	{
		char macFolderPath[PATH_MAX];
		char temp[PATH_MAX];

		if ( !Sys_UnpackDLL( name2 ) ) {
			goto tryAppPackage;
		}

		Mac_GetOSPath( kUserDomain, kTemporaryFolderType, macFolderPath );

		// create a URL to the bundle
		sprintf( temp, "%s%s%s%s%s.bundle", macFolderPath, kPathSep, kMacGameDir, kPathSep, name2 );

		dllSpec->bundleURL = CFURLCreateWithBytes( kCFAllocatorDefault, (UInt8 *)temp, strlen( temp ), kCFStringEncodingUTF8, NULL );

		// did we actaully get a bundle URL
		if ( !dllSpec->bundleURL ) {
			goto tryAppPackage;
		}

		// get the actual bundle for the library
		dllSpec->bundleRef = CFBundleCreate( kCFAllocatorDefault, dllSpec->bundleURL );
		if ( !dllSpec->bundleRef ) {
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto tryAppPackage;
		}

		// Load the bundle code
		didLoad = CFBundleLoadExecutable( dllSpec->bundleRef );
		if ( !didLoad ) {
			if ( dllSpec->bundleRef ) {
				CFRelease( dllSpec->bundleRef );
			}
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto tryAppPackage;
		}

		dllEntry = (dllEntryPtr)CFBundleGetFunctionPointerForName( dllSpec->bundleRef, CFSTR( "dllEntry" ) );
		if ( dllEntry == NULL ) {
			if ( dllSpec->bundleRef ) {
				CFRelease( dllSpec->bundleRef );
			}
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto tryAppPackage;
		}

		dllEntry( systemCalls );

		// Bind "vmMain" so we can call into it later as well
		*entryPoint = (vmMainPtr)CFBundleGetFunctionPointerForName( dllSpec->bundleRef, CFSTR( "vmMain" ) );
		if ( *entryPoint == NULL ) {
			if ( dllSpec->bundleRef ) {
				CFRelease( dllSpec->bundleRef );
			}
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto tryAppPackage;
		}

		Com_Printf( "Mach-O bundle %s loaded (pk3)\n", name );
		return (void *)dllSpec;
	}

tryAppPackage:
	Com_sprintf( name2, sizeof( name2 ), "%s", Sys_GetDLLName( name ) );

	strcat( name2, ".bundle" );

	// Look for the bundle inside the app package
	{
		FSRef myBundleRef;
		CFBundleRef refMainBundle = NULL;
		CFStringRef libString;
		Boolean ok;

		// get app bundle
#if DEDICATED
		char path[PATH_MAX];
		CFURLRef bundleURL;

		strcpy( path, kMacExeName );
		bundleURL = CFURLCreateFromFileSystemRepresentation( NULL, path, strlen( path ), true );
		refMainBundle = CFBundleCreate( NULL, bundleURL );
		CFRelease( bundleURL );
#else
		refMainBundle = CFBundleGetMainBundle();
#endif
		if ( !refMainBundle ) {
			err = fnfErr;
			goto cantLoad;
		}

		libString = CFStringCreateWithCString( kCFAllocatorDefault, name2, kCFStringEncodingUTF8 );

		// get a URL to our shared library from the "Resources" directory of the bundle
		dllSpec->bundleURL = CFBundleCopyResourceURL( refMainBundle, libString, NULL, NULL );
		if ( !dllSpec->bundleURL ) {
			err = fnfErr;
			goto cantLoad;
		}

		// get the actual bundle for the library
		dllSpec->bundleRef = CFBundleCreate( kCFAllocatorDefault, dllSpec->bundleURL );
		if ( !dllSpec->bundleRef ) {
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto cantLoad;
		}

		// Load the bundle code
		didLoad = CFBundleLoadExecutable( dllSpec->bundleRef );
		if ( !didLoad ) {
			if ( dllSpec->bundleRef ) {
				CFRelease( dllSpec->bundleRef );
			}
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto cantLoad;
		}

		dllEntry = (dllEntryPtr)CFBundleGetFunctionPointerForName( dllSpec->bundleRef, CFSTR( "dllEntry" ) );
		if ( dllEntry == NULL ) {
			if ( dllSpec->bundleRef ) {
				CFRelease( dllSpec->bundleRef );
			}
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto cantLoad;
		}

		dllEntry( systemCalls );

		// Bind "vmMain" so we can call into it later as well
		*entryPoint = (vmMainPtr)CFBundleGetFunctionPointerForName( dllSpec->bundleRef, CFSTR( "vmMain" ) );
		if ( *entryPoint == NULL ) {
			if ( dllSpec->bundleRef ) {
				CFRelease( dllSpec->bundleRef );
			}
			if ( dllSpec->bundleURL ) {
				CFRelease( dllSpec->bundleURL );
			}
			goto cantLoad;
		}

		Com_Printf( "Mach-O bundle %s loaded\n", name );
		return (void *)dllSpec;
	}


cantLoad:
	return NULL;
}

void Sys_UnloadDll( void *dllHandle ) {
	OSErr err = noErr;
	dll_t *dllSpec = (dll_t *) dllHandle;

	if ( dllSpec->connID ) {
		err = CloseConnection( &( dllSpec->connID ) );
	} else
	{
		if ( dllSpec->bundleRef ) {
			CFRelease( dllSpec->bundleRef );
		}
		if ( dllSpec->bundleURL ) {
			CFRelease( dllSpec->bundleURL );
		}
		if ( dllSpec->cfmGlue ) {
			DisposePtr( (Ptr)dllSpec->cfmGlue );
		}
		if ( dllSpec->cfmGlue2 ) {
			DisposePtr( (Ptr)dllSpec->cfmGlue2 );
		}
		if ( dllSpec->cfmGlue3 ) {
			DisposePtr( (Ptr)dllSpec->cfmGlue3 );
		}
	}

	free( dllSpec );
}

void    *Sys_GetGameAPI( void *parms ) {
	OSErr err = noErr;
	CFBundleRef refMainBundle = NULL;
	CFURLRef refMainBundleURL = NULL;
	CFStringRef fileName;
	Boolean didLoad;
	getGameAPIPtr GetGameAPI = NULL;
	char name2[255];

	// get app bundle (works even for a non-bundled app!)
#if DEDICATED
	char path[PATH_MAX];
	CFURLRef bundleURL;

	strcpy( path, kMacExeName );
	bundleURL = CFURLCreateFromFileSystemRepresentation( NULL, path, strlen( path ), true );
	refMainBundle = CFBundleCreate( NULL, bundleURL );
	CFRelease( bundleURL );
#else
	refMainBundle = CFBundleGetMainBundle();
#endif
	if ( !refMainBundle ) {
		goto cantLoad;
	}

	// get a URL to our shared library from the "Resources" directory of the bundle
	refMainBundleURL = CFBundleCopyResourceURL( refMainBundle, CFSTR( kMacGameLib ".bundle" ), NULL, NULL );
	if ( !refMainBundleURL ) {
		err = fnfErr;
		goto cantLoad;
	}

#if MAC_DEBUG
	CFShow( refMainBundleURL );
#endif

	// get the actual bundle for the library
	gameBundleRef = CFBundleCreate( kCFAllocatorDefault, refMainBundleURL );
	if ( !gameBundleRef ) {
		goto cantLoad;
	}
	CFRelease( refMainBundleURL );

	// Load the bundle code
	didLoad = CFBundleLoadExecutable( gameBundleRef );
	if ( !didLoad ) {
		goto cantLoad;
	}

	GetGameAPI = (getGameAPIPtr)CFBundleGetFunctionPointerForName( gameBundleRef, CFSTR( "GetGameAPI" ) );
	if ( GetGameAPI == NULL ) {
		goto cantLoad;
	}

	return GetGameAPI( parms );

cantLoad:
	if ( err == cfragNoClientMemErr ) {
		Sys_Error( "Can't load %s. Give the game additional memory and try again.", kMacGameLib );
	} else {
		Sys_Error( "Can't load %s, error %d", kMacGameLib, err );
	}

	// Keep the compiler happy
	return NULL;
}

void    Sys_UnloadGame( void ) {
	OSErr err = noErr;

	if ( game_connID ) {
		err = CloseConnection( &game_connID );
	}
	game_connID = 0;
	if ( gameBundleRef ) {
		CFRelease( gameBundleRef );
		gameBundleRef = NULL;
	}
}


/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
	FS_Seek( f, offset, origin );
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define MAX_QUED_EVENTS     256
#define MASK_QUED_EVENTS    ( MAX_QUED_EVENTS - 1 )

sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead = 0;
static int eventTail = 0;
byte sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t  *ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf( "Sys_QueEvent: overflow\n" );
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
	//   MSG			msg;
	sysEvent_t ev;
	char        *s;
	msg_t netmsg;
	netadr_t adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

#if 0 // ¥¥¥
	  // pump the message loop
	while ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
		if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
//		g_wv.sysMsgTime = msg.time;

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
#endif

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char    *b;
		int len;

		len = strlen( s ) + 1;
		b = (char *)Z_Malloc( len );
		Q_strncpyz( b, s, len );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket( &adr, &netmsg ) ) {
		netadr_t        *buf;
		int len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t *)Z_Malloc( len );
		*buf = adr;
		memcpy( buf + 1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
#if 0 // ¥¥¥
	IN_Shutdown();
	IN_Init();
#endif
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
#if 0 // ¥¥¥
void Sys_Net_Restart_f( void ) {
	NET_Restart();
}
#endif

enum {
	CPUID_PPC_601 = 1,
	CPUID_PPC_603,
	CPUID_PPC_604,
	CPUID_PPC_G3,
	CPUID_PPC_G4,
	CPUID_PPC_UNK
};


int Sys_GetProcessorId( void ) {
	OSErr err;
	long response;

	err = Gestalt( gestaltNativeCPUfamily, &response );
	if ( err == noErr ) {
		switch ( response )
		{
		case gestaltCPU601:
			return CPUID_PPC_601;
		case gestaltCPU603:
			return CPUID_PPC_603;
		case gestaltCPU604:
			return CPUID_PPC_604;
		case gestaltCPU750:
			return CPUID_PPC_G3;
		case gestaltCPUG4:
			return CPUID_PPC_G4;
		default:
			return CPUID_PPC_UNK;
		}
	} else {
		return CPUID_GENERIC;
	}
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/

void Sys_Init( void ) {
	long response;
	OSErr err;
	int cpuid;
	int numCPU = 1;


#if MAC_Q3_MP
	Sys_InitNetworking();
#endif

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
///	timeBeginPeriod( 1 );

	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
//¥¥¥	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	//
	// figure out our CPU
	//
	Cvar_Get( "sys_cpustring", "detect", 0 );
	if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "detect" ) ) {
		Com_Printf( "...detecting CPU, found " );

		cpuid = Sys_GetProcessorId();

		switch ( cpuid )
		{
		case CPUID_PPC_601:
			Cvar_Set( "sys_cpustring", "PowerPC 601" );
			break;
		case CPUID_PPC_603:
			Cvar_Set( "sys_cpustring", "PowerPC 603" );
			break;
		case CPUID_PPC_604:
			Cvar_Set( "sys_cpustring", "PowerPC 604" );
			break;
		case CPUID_PPC_G3:
			Cvar_Set( "sys_cpustring", "PowerPC G3" );
			break;
		case CPUID_PPC_G4:
			if ( numCPU == 2 ) {
				Cvar_Set( "sys_cpustring", "PowerPC G4 x 2" );
			} else if ( numCPU == 1 ) {
				Cvar_Set( "sys_cpustring", "PowerPC G4" );
			} else {
				Cvar_Set( "sys_cpustring", "PowerPC G4 (multiple)" );
			}
			break;
		case CPUID_PPC_UNK:
			if ( numCPU == 2 ) {
				Cvar_Set( "sys_cpustring", "PowerPC (unknown) x 2" );
			} else if ( numCPU == 1 ) {
				Cvar_Set( "sys_cpustring", "PowerPC (unknown)" );
			} else {
				Cvar_Set( "sys_cpustring", "PowerPC (unknown - multiple)" );
			}
			break;
		}
	} else
	{
		Com_Printf( "...forcing CPU type to " );
		if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "generic" ) ) {
			cpuid = CPUID_GENERIC;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "x87" ) )     {
			cpuid = CPUID_INTEL_PENTIUM;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "mmx" ) )     {
			cpuid = CPUID_INTEL_MMX;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "3dnow" ) )     {
			cpuid = CPUID_AMD_3DNOW;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIII" ) )     {
			cpuid = CPUID_INTEL_KATMAI;
		}
#if 0
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIV" ) ) {
			cpuid = CPUID_INTEL_WILLIAMETTE;
		}
#endif
		else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "axp" ) ) {
			cpuid = CPUID_AXP;
		} else
		{
			Com_Printf( "WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString( "sys_cpustring" ) );
			cpuid = CPUID_GENERIC;
		}
	}

	Cvar_SetValue( "sys_cpuid", cpuid );
	Com_Printf( "%s\n", Cvar_VariableString( "sys_cpustring" ) );

	err = Gestalt( gestaltProcClkSpeed, &response );
	Cvar_SetValue( "sys_cpuspeed", response );

	err = Gestalt( gestaltPhysicalRAMSize, &response );
	Cvar_SetValue( "sys_memory", response );

///	Cvar_Set( "username", Sys_GetCurrentUser() );

#if 0 // ¥¥¥ LBO
	IN_Init();      // FIXME: not in dedicated?
#endif
}

//===============================================================================
//	SetDefaultDirectory
//
//	Under native OS X, the default directory for apps is some bizarro Unix
//  location. Here we redefine it to the classic MacOS location, which is the
//  same folder that the app is running from.
//===============================================================================

FSSpec app_spec;
void SetDefaultDirectory( void ) {
	ProcessSerialNumber serial;
	ProcessInfoRec info;
	WDPBRec wpb;
	OSErr err;

	serial.highLongOfPSN = 0;
	serial.lowLongOfPSN = kCurrentProcess;


	info.processInfoLength = sizeof( ProcessInfoRec );
	info.processName = NULL;
	info.processAppSpec = &app_spec;

	err = GetProcessInformation( &serial, &info );

	wpb.ioVRefNum = app_spec.vRefNum;
	wpb.ioWDDirID = app_spec.parID;
	wpb.ioNamePtr = NULL;

	err = PBHSetVolSync( &wpb );
}

//=======================================================================
//int	totalMsec, countMsec;

/*
==================
WinMain

==================
*/
//int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int main( int argc, char **argv ) {
	char cwd[MAX_OSPATH];
	char *cmdline;
	int i,len;
	OSErr err;
//	int			startTime, endTime;

	// should never get a previous instance in Win32
//    if ( hPrevInstance ) {
//        return 0;
//	}

	// merge the command line, this is kinda silly
	for ( len = 1, i = 1; i < argc; i++ )
		len += strlen( argv[i] ) + 1;
	cmdline = (char *)malloc( len );
	*cmdline = 0;
	for ( i = 1; i < argc; i++ ) {
		if ( i > 1 ) {
			strcat( cmdline, " " );
		}
		strcat( cmdline, argv[i] );
	}

	SetDefaultDirectory();

	// Are we on 10?
	err = Gestalt( gestaltSystemVersion, &gSystemVersion );

//	g_wv.hInstance = hInstance;
//	Q_strncpyz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );


	// done before Com/Sys_Init since we need this for error output
//	Sys_CreateConsole();

	// no abort/retry/fail errors
///	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif


//¥¥¥	Sys_InitStreamThread();

	Com_Init( cmdline );

#if MAC_MOHAA
	Sys_InitLocalization();
#endif

//¥¥¥	NET_Init();

	Com_Printf( "Working directory: %s\n", Sys_Cwd() );

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

	// main game loop
	while ( 1 ) {
		// if not running as a game client, sleep a bit
//		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
//¥¥¥			Sleep( 5 );
//		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );

//		startTime = Sys_Milliseconds();

#if 0 // ¥¥¥
	  // make sure mouse and joystick are only called once a frame
		IN_Frame();
#endif

		// run the game
		Com_Frame();

//		endTime = Sys_Milliseconds();
//		totalMsec += endTime - startTime;
//		countMsec++;
	}

	// never gets here
	return 0;
}


