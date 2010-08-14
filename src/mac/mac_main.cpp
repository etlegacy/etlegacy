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

#include <Security/AuthSession.h>

#include <math.h>

Q3DEF_BEGIN
	#include "../renderer/tr_local.h"
	#include "../client/client.h"
	#include "mac_local.h"
	#include "../game/g_public.h"
	#include "../qcommon/unzip.h"
Q3DEF_END

#include <unistd.h>
//#include "HID Manager/HID_Utilities_CFM.h"
#include "AGLUtils.h"
#include "MacPrefs.h"
#include "CDrawSprocket.h"
#if MAC_Q3_QUICKTIME
#include "CQuickTimePlayer.h"
#endif
#include "PickMonitor/pickmonitor.h"

#if MAC_Q3_MP
#define kOMEventClass FOUR_CHAR_CODE( 'OM2!' )
#define kOMExecuteCommands FOUR_CHAR_CODE( 'exec' )

//#include "GameRanger SDK/GameRanger.h"
qboolean    Sys_GetPacket( netadr_t *net_from, msg_t *net_message );

#endif

void GetDialogItemTextAppearance( DialogRef inDialog, DialogItemIndex inItem, StringPtr outText );
char *ReadCommandLineParms( void );


static cvar_t   *gamedll;
static cvar_t   *cgamedll;
cvar_t          *pagememory;

int sys_ticBase;
int sys_msecBase;
int sys_lastEventTic;
int sys_CDinDrive = false;
cvar_t  *sys_profile;

static char *sCommandLine;
static char *sCommandLine_temp;
static Boolean sGameRangerHosting;

FSSpec app_spec;
long gSystemVersion;

Boolean gUseCarbonEvents;
Boolean gMoviePlaying;
Fixed gRefreshRate;

SInt32 mac_keyboardScript;

Boolean macSessionGraphics = false;

Boolean OptionDialog( void );

Q3DEF_BEGIN

extern void GLimp_pause( void );
extern void GLimp_resume( void );

//===========================================================================

qboolean Sys_CheckCD( void ) {
	return qtrue;
}

#pragma mark -

#if MAC_Q3_MP
// Call this from SV_SpawnServer() in sv_init.c(pp), right after the call to SV_InitGameProgs();
void Mac_GameRanger_HostReady( void ) {
	// Were we started by GameRanger?
	if ( sGameRangerHosting ) {
		// Yes, tell the clients we're ready to host so that they can join
		//GRHostReady ();

		// Clear the flag so we don't call GR inappropriately
		sGameRangerHosting = false;
	}
}
#endif

//===========================================================================
#pragma mark -

CFragConnectionID game_connID;
CFragConnectionID cgame_connID;
CFragConnectionID ui_connID;

typedef void ( *dllEntryPtr )( int ( *syscallptr )( int, ... ) );
typedef int ( *systemCallsPtr )( int, ... );
typedef int ( *vmMainPtr )( int, ... );

typedef struct
{
	// For CFM shared libraries
	CFragConnectionID connID;

	// For Mach-O bundles
	CFBundleRef bundleRef;
	CFURLRef bundleURL;
	systemCallsPtr cfmGlue;
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

/*
===============
Sys_UnpackDLL
Tries to find a named DLL in the pk3 hierarchy. If found, it copies it, and then
attempts to treat the copied file as a zip file, unzipping it to the temp directory.
This should only be called when looking for a Mach-O .bundle DLL.
===============
*/
extern qboolean legacy_mp_bin;
Boolean Sys_UnpackDLL( const char *name ) {
	void *data;
	FILE *macFile;
	int len = FS_ReadFile( name, &data );
	int ck;
	char zipPath[PATH_MAX];
	char macFolderPath[PATH_MAX];

	if ( len < 1 ) {
		// failed to read the file (if pure, may exist out of the pk3s)
		return false;
	}

	// qagame can be loaded outside of pk3 hierarchy
	// for everything else, pure rules apply
	if ( strcmp( name, "qagame_mac" ) ) {

		if ( FS_FileIsInPAK( name, &ck ) == -1 ) {
			FS_FreeFile( data );
			return false;
		}

	}

	if ( legacy_mp_bin ) {
		// don't load from legacy 2.60 mp_bin.pk3, get our updated version
		return false;
	}

	// We create our temp copy of the DLL zip file outside the folder hierarchy
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
	/*
  return va("%s.mp.osx.so", name);
	*/
}
#endif


#pragma mark Sys_LoadDll
void *Sys_LoadDll( const char *name, char *fqpath, int( **entryPoint ) ( int, ... ), int ( *systemCalls )( int, ... ) ) {
	OSErr err = noErr;
	FSSpec SLSpec;
	char name2[255];
	dllEntryPtr dllEntry = NULL;
	dll_t *dllSpec;

	legacy_mp_bin = qfalse;

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

	// TTimo - never load out of tree if we are in pure mode
	// (I suppose all the Mac ports of Q3-tech Aspyr has made are flawed that way)
	// when we found in the legacy_mp_bin, keep going. We'll make sure the checksum matches the hardcoded value
	if ( !legacy_mp_bin && FS_IsPure() ) {
		goto cantLoad;
	}

	Com_sprintf( name2, sizeof( name2 ), "%s", Sys_GetDLLName( name ) );

	strcat( name2, ".bundle" );

	// Look for the bundle inside the app package
	{
		FSRef myBundleRef;
		CFBundleRef refMainBundle = NULL;
		CFStringRef libString;
		CFStringRef cfpath;
		char path[ PATH_MAX ];
		Boolean ok;
		unsigned int checksum;

		// get app bundle
		refMainBundle = CFBundleGetMainBundle();
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

		if ( legacy_mp_bin ) {
			// make sure the override is what we expect
			cfpath = CFURLCopyFileSystemPath( dllSpec->bundleURL, kCFURLPOSIXPathStyle );
			if ( !CFStringGetCString( cfpath, path, sizeof( path ), kCFStringEncodingASCII ) ) {
				Com_Printf( "failed to get the OS path to the bundle\n" );
				goto cantLoad;
			}
			strncat( path, va( "/Contents/MacOS/%s_mac", name ), sizeof( path ) - strlen( path ) - 64 );
			checksum = FS_ChecksumOSPath( path );
			//Com_Printf( "%s %X\n", path, checksum );
			// it doesn't hurt to always do the load and checksum, makes the setup easier
			// we only enforce the check when pureness is required however
			if ( FS_IsPure() ) {
				if ( checksum != 0x50A800B3 && checksum != 0x81A6FB10 ) {
					goto cantLoad;
				}
			}
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
			Com_Printf( "dllEntry not found\n" );
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
			Com_Printf( "vmMain not found\n" );
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
	}

	free( dllSpec );
}


void    *Sys_GetGameAPI( void *parms ) {
	void    *( *GetGameAPI )(void *);
	OSErr err = noErr;
	FSSpec SLSpec;
	Str255 symName;
	CFragSymbolClass symClass;

	// If the library is still hanging around for some freakish reason, dump it
	if ( game_connID ) {
		Sys_UnloadGame();
	}

#if MAC_TIME_LIMIT
	CheckTimeLimit();
#endif
	c2pstrcpy( symName, kMacGameLib );

	if ( AmIBundled() ) {
		FSRef myBundleRef;
		CFBundleRef refMainBundle = NULL;
		CFURLRef refMainBundleURL = NULL;
		Boolean ok;

		// get app bundle
		refMainBundle = CFBundleGetMainBundle();
		if ( !refMainBundle ) {
			err = fnfErr;
			goto cantLoad;
		}

		// get a URL to our shared library from the "Resources" directory of the bundle
		refMainBundleURL = CFBundleCopyResourceURL( refMainBundle, CFSTR( kMacGameLib ), NULL, NULL );
		if ( !refMainBundleURL ) {
			err = fnfErr;
			goto cantLoad;
		}

		// Turn that URL into an FSRef
		ok = CFURLGetFSRef( refMainBundleURL, &myBundleRef );
		CFRelease( refMainBundleURL );
		if ( !ok ) {
			err = fnfErr;
			goto cantLoad;
		}

		// Now get an FSSpec for it
		err = FSGetCatalogInfo( &myBundleRef, kFSCatInfoNone, NULL, NULL, &SLSpec, NULL );
	} else
	{
		char gameDir2[1024];

		err = FSMakeFSSpec( 0, 0L, symName, &SLSpec );
		if ( err == fnfErr ) {
			sprintf( gameDir2, ":%s:%s", kMacGameDir, kMacGameLib );
			c2pstrcpy( symName, gameDir2 );

			err = FSMakeFSSpec( 0, 0L, symName, &SLSpec );
		}
	}

	if ( err ) {
		goto cantLoad;
	}

	err = GetDiskFragment( &SLSpec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, &game_connID, (Ptr*)NULL, symName );

	// If no memory, try again in the system heap
	if ( err == cfragNoClientMemErr ) {
		THz savedZone = LMGetApplZone();

		LMSetApplZone( LMGetSysZone() );
		err = GetDiskFragment( &SLSpec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, &game_connID, (Ptr*)NULL, symName );
		LMSetApplZone( savedZone );
	}

	if ( err ) {
		goto cantLoad;
	}

	strcpy( (char*) symName, (char*)"\pGetGameAPI" );

	err =  FindSymbol( game_connID, symName, (Ptr *)&GetGameAPI, &symClass );
	if ( err ) {
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
}

void    *Sys_GetUIAPI( void ) {
	void    *( *GetUIAPI )(void);
	OSErr err = noErr;
	FSSpec SLSpec;
	Str255 symName;
	CFragSymbolClass symClass;

	// If the library is still hanging around for some freakish reason, dump it
	if ( ui_connID ) {
		Sys_UnloadCGame();
	}

#if MAC_TIME_LIMIT
	CheckTimeLimit();
#endif
	c2pstrcpy( symName, kMacUILib );

	if ( AmIBundled() ) {
		FSRef myBundleRef;
		CFBundleRef refMainBundle = NULL;
		CFURLRef refMainBundleURL = NULL;
		Boolean ok;

		// get app bundle
		refMainBundle = CFBundleGetMainBundle();
		if ( !refMainBundle ) {
			err = fnfErr;
			goto cantLoad;
		}

		// get a URL to our shared library from the "Resources" directory of the bundle
		refMainBundleURL = CFBundleCopyResourceURL( refMainBundle, CFSTR( kMacUILib ), NULL, NULL );
		if ( !refMainBundleURL ) {
			err = fnfErr;
			goto cantLoad;
		}

		// Turn that URL into an FSRef
		ok = CFURLGetFSRef( refMainBundleURL, &myBundleRef );
		CFRelease( refMainBundleURL );
		if ( !ok ) {
			err = fnfErr;
			goto cantLoad;
		}

		// Now get an FSSpec for it
		err = FSGetCatalogInfo( &myBundleRef, kFSCatInfoNone, NULL, NULL, &SLSpec, NULL );
	} else
	{
		char gameDir2[1024];

		err = FSMakeFSSpec( 0, 0L, symName, &SLSpec );
		if ( err == fnfErr ) {
			sprintf( gameDir2, ":%s:%s", kMacGameDir, kMacUILib );
			c2pstrcpy( symName, gameDir2 );

			err = FSMakeFSSpec( 0, 0L, symName, &SLSpec );
		}
		if ( err == fnfErr ) {
			Sys_Error( "Can't find %s at location %s", kMacUILib, gameDir2 );
		}
	}

	if ( err ) {
		goto cantLoad;
	}

	err = GetDiskFragment( &SLSpec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, &ui_connID, (Ptr*)NULL, symName );

	// If no memory, try again in the system heap
	if ( err == cfragNoClientMemErr ) {
		THz savedZone = LMGetApplZone();

		LMSetApplZone( LMGetSysZone() );
		err = GetDiskFragment( &SLSpec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, &ui_connID, (Ptr*)NULL, symName );
		LMSetApplZone( savedZone );
	}

	if ( err ) {
		goto cantLoad;
	}

	strcpy( (char*) symName, (char*)"\pGetUIAPI" );

	err =  FindSymbol( ui_connID, symName, (Ptr *)&GetUIAPI, &symClass );
	if ( err ) {
		goto cantLoad;
	}

	return GetUIAPI();

cantLoad:
	if ( err == cfragNoClientMemErr ) {
		Sys_Error( "Can't load %s. Give the game additional memory and try again.", kMacUILib );
	} else {
		Sys_Error( "Can't load %s, error %d", kMacUILib, err );
	}

	// Keep the compiler happy
	return NULL;
}

void    Sys_UnloadUI( void ) {
	OSErr err = noErr;

	if ( ui_connID ) {
		err = CloseConnection( &ui_connID );
		ui_connID = 0;
	}
}

void    *Sys_GetCGameAPI( void ) {
	void    *( *GetCGameAPI )(void);
	OSErr err = noErr;
	FSSpec SLSpec;
	Str255 symName;
	CFragSymbolClass symClass;

	// If the library is still hanging around for some freakish reason, dump it
	if ( cgame_connID ) {
		Sys_UnloadCGame();
	}

#if MAC_TIME_LIMIT
	CheckTimeLimit();
#endif
	c2pstrcpy( symName, kMacCGameLib );

	if ( AmIBundled() ) {
		FSRef myBundleRef;
		CFBundleRef refMainBundle = NULL;
		CFURLRef refMainBundleURL = NULL;
		Boolean ok;

		// get app bundle
		refMainBundle = CFBundleGetMainBundle();
		if ( !refMainBundle ) {
			err = fnfErr;
			goto cantLoad;
		}

		// get a URL to our shared library from the "Resources" directory of the bundle
		refMainBundleURL = CFBundleCopyResourceURL( refMainBundle, CFSTR( kMacCGameLib ), NULL, NULL );
		if ( !refMainBundleURL ) {
			err = fnfErr;
			goto cantLoad;
		}

		// Turn that URL into an FSRef
		ok = CFURLGetFSRef( refMainBundleURL, &myBundleRef );
		CFRelease( refMainBundleURL );
		if ( !ok ) {
			err = fnfErr;
			goto cantLoad;
		}

		// Now get an FSSpec for it
		err = FSGetCatalogInfo( &myBundleRef, kFSCatInfoNone, NULL, NULL, &SLSpec, NULL );
	} else
	{
		char gameDir2[1024];

		err = FSMakeFSSpec( 0, 0L, symName, &SLSpec );
		if ( err == fnfErr ) {
			sprintf( gameDir2, ":%s:%s", kMacGameDir, kMacCGameLib );
			c2pstrcpy( symName, gameDir2 );

			err = FSMakeFSSpec( 0, 0L, symName, &SLSpec );
		}
		if ( err == fnfErr ) {
			Sys_Error( "Can't find %s at location %s", kMacCGameLib, gameDir2 );
		}
	}

	if ( err ) {
		goto cantLoad;
	}

	err = GetDiskFragment( &SLSpec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, &cgame_connID, (Ptr*)NULL, symName );

	// If no memory, try again in the system heap
	if ( err == cfragNoClientMemErr ) {
		THz savedZone = LMGetApplZone();

		LMSetApplZone( LMGetSysZone() );
		err = GetDiskFragment( &SLSpec, 0, kCFragGoesToEOF, NULL, kPrivateCFragCopy, &cgame_connID, (Ptr*)NULL, symName );
		LMSetApplZone( savedZone );
	}

	if ( err ) {
		goto cantLoad;
	}

	strcpy( (char*) symName, (char*)"\pGetCGameAPI" );

	err =  FindSymbol( cgame_connID, symName, (Ptr *)&GetCGameAPI, &symClass );
	if ( err ) {
		goto cantLoad;
	}

	return GetCGameAPI();

cantLoad:
	if ( err == cfragNoClientMemErr ) {
		Sys_Error( "Can't load %s. Give the game additional memory and try again.", kMacCGameLib );
	} else {
		Sys_Error( "Can't load %s, error %d", kMacCGameLib, err );
	}

	// Keep the compiler happy
	return NULL;
}

void    Sys_UnloadCGame( void ) {
	OSErr err = noErr;

	if ( cgame_connID ) {
		err = CloseConnection( &cgame_connID );
		cgame_connID = 0;
	}
}


//===========================================================================

char *Sys_GetClipboardData( void ) {
	ScrapRef currentScrap;
	OSStatus status;
	char *buffer;
	Size byteCount;

	GetCurrentScrap( &currentScrap );

	status = GetScrapFlavorSize( currentScrap, 'TEXT', &byteCount );
	if ( ( status == noErr ) && ( byteCount < 1023 ) ) {
#if MAC_Q3_OLDSTUFF
		buffer = (char *)Z_Malloc( byteCount + 1 );
#else
		buffer = (char *)Z_Malloc( byteCount + 1, TAG_GENERAL, qfalse );
#endif
		if ( !buffer ) {
			return NULL;
		}

		// Get the 'TEXT' clipping from the clipboard
		status = GetScrapFlavorData( currentScrap, 'TEXT', &byteCount, (void *)buffer );
	}
	return buffer;
}

char *Sys_GetWholeClipboard( void ) {
	// LBO - not sure what this does differently from Sys_GetClipboardData
	return Sys_GetClipboardData();
}

void Sys_SetClipboard( const char *contents ) {
	ScrapRef currentScrap;
	OSStatus status;
	Size byteCount;

	ClearCurrentScrap();
	GetCurrentScrap( &currentScrap );

	byteCount = strlen( contents );

	// Put the 'TEXT' clipping on the clipboard
	status = PutScrapFlavor( currentScrap, 'TEXT', kScrapFlavorMaskNone, byteCount, (void *)contents );
}

enum {
	CPUID_PPC_601 = 1,
	CPUID_PPC_603,
	CPUID_PPC_604,
	CPUID_PPC_G3,
	CPUID_PPC_G4,
	CPUID_PPC_UNK,
	CPUID_INTEL
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
			err = Gestalt( gestaltSysArchitecture, &response );
			if ( err == noErr ) {
				if ( response == gestaltPowerPC ) {
					return CPUID_PPC_UNK;
				} else {
					return CPUID_INTEL;
				}
			} else {
				return CPUID_GENERIC;
			}
		}
	} else {
		return CPUID_GENERIC;
	}
}

void *Sys_InitializeCriticalSection() {
	return (void *)-1;
}

void Sys_EnterCriticalSection( void *ptr ) {
}

void Sys_LeaveCriticalSection( void *ptr ) {
}

int Sys_GetHighQualityCPU() {
	// FIXME TTimo see win_shared.c
	return 0;
}

void Sys_StartProcess( char *exeName, qboolean doexit ) {
	OSErr launchErr;
	FSSpec spec;
	Str255 macName;
	LaunchParamBlockRec myLaunchParams;
	ProcessSerialNumber launchedProcessSN;


	launchErr = FSMakeFSSpec( 0, 0L, "\pMedal of Honor", &spec );
	if ( launchErr != noErr ) {
		Com_Error( ERR_DROP, "Unable to find Medal of Honor" );
	}

	myLaunchParams.launchBlockID = extendedBlock;
	myLaunchParams.launchEPBLength = extendedBlockLen;
	myLaunchParams.launchFileFlags = 0;
	if ( !doexit ) {
		myLaunchParams.launchControlFlags = launchContinue + launchNoFileFlags;
	} else {
		myLaunchParams.launchControlFlags = launchNoFileFlags;
	}
	myLaunchParams.launchAppSpec = &spec;
	myLaunchParams.launchAppParameters = nil;

	launchErr = LaunchApplication( &myLaunchParams );

	if ( launchErr != noErr ) {
		Com_Error( ERR_DROP, "Unable to launch Medal of Honor" );
	}


	// this is only used for WolfSP / WolfMP spawning for now
	if ( doexit ) {
		exit( 0 );
	}
}

/*
==================
Sys_OpenURL
==================
*/
void Sys_OpenURL( const char *url, qboolean doexit ) {
	OSStatus err;
	ICInstance inst;
	long startSel;
	long endSel;

#ifdef WOLF_SP_DEMO
	strcpy( url, "www.aspyr.com/mini-sites/rtcw" );    //DAJ HACK
#endif
	err = ICStart( &inst, 'WlfS' );
	if ( err == noErr ) {
#if !TARGET_API_MAC_CARBON
		err = ICFindConfigFile( inst, 0, NULL );
		if ( err == noErr )
#endif
		{
			startSel = 0;
			endSel = strlen( url );
			err = ICLaunchURL( inst, "\p", (char*)url, strlen( url ), &startSel, &endSel );
		}
		ICStop( inst );
	}

	if ( doexit ) {
		ExitToShell();
	}

}

void Sys_SnapVector( float *v ) {
	v[0] = rint( v[0] );
	v[1] = rint( v[1] );
	v[2] = rint( v[2] );
}

/*
==================
Sys_CloseMutex

==================
*/

void Sys_CloseMutex( void ) {

}

//===================================================================


/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
	Sys_ShutdownInput();
	Sys_InitInput();
}

/*
================
Sys_Init

The cvar and file system has been setup, so configurations are loaded
================
*/
void Sys_Init( void ) {
	long response;
	OSErr err;
	int cpuid;
	int numCPU;

#if MAC_Q3_MP
	Sys_InitNetworking();
#endif
	Sys_InitInput();

	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
#if MAC_Q3_MP
//	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);
#endif

	numCPU = 1;
	if ( MPLibraryIsLoaded() ) {
		numCPU = MPProcessors();
	}

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
		case CPUID_INTEL:
			Cvar_Set( "sys_cpustring", "Intel" );
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
#if !MAC_Q3_OLDSTUFF
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
}

/*
=================
Sys_Shutdown
=================
*/
void Sys_Shutdown( void ) {
	Sys_EndProfiling();
	Sys_ShutdownInput();
#if MAC_Q3_MP
	Sys_ShutdownNetworking();
#endif

#if TARGET_API_MAC_CARBON
	if ( gUseCarbonEvents ) {
		QuitApplicationEventLoop();
	}
#endif
#if MAC_Q3_QUICKTIME
	ExitMovies();
#endif
	SavePrefs( &macPrefs );
}


/*
==================
Sys_PumpMessageLoop

==================
*/
void Sys_PumpMessageLoop( void ) {

}

/*
=================
Sys_BeginProfiling
=================
*/
static qboolean sys_profiling;
void Sys_BeginProfiling( void ) {
#if __profile__
	if ( !sys_profile->integer ) {
		return;
	}
	ProfilerInit( collectDetailed, bestTimeBase, 16384, 64 );
	sys_profiling = qtrue;
#endif
}

/*
=================
Sys_EndProfiling
=================
*/
void Sys_EndProfiling( void ) {
#if __profile__
	unsigned char pstring[1024];

	if ( !sys_profiling ) {
		return;
	}
	sys_profiling = qfalse;

	sprintf( (char *)pstring + 1, "%s:profile.txt", Cvar_VariableString( "fs_basepath" ) );
	pstring[0] = strlen( (char *)pstring + 1 );
	ProfilerDump( pstring );
//DAJ	ProfilerTerm();
#endif
}

qboolean Sys_LowPhysicalMemory( void ) {
	return qfalse;
}

//================================================================================


/*
================
Sys_Milliseconds
================
*/
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

/*
================
Sys_Error
================
*/
void Sys_Error( const char *format, ... ) {
	va_list argptr;
	char errString[1024];
	AlertStdAlertParamRec param;
	short itemHit;
	Str255 briefMsg;
	int length;

	Sys_Shutdown();

	// If we're using DSp on OSX, we need to pause the context so we can see dialogs
	if ( r_fullscreen ) {
		GLimp_pause();
		if ( ( r_fullscreen->integer ) && ( gSystemVersion >= 0x01000 ) ) {
			if ( gDrawSprocket->GetState() == kDSpContextState_Active ) {
				gDrawSprocket->SetState( kDSpContextState_Paused );
			}
		}
	}

// aglSetFullscreen on 10 will not pause correctly, so the dialog
// will not be seen. We just quit the app instead.
//if (gSystemVersion >= 0x1000) goto bail;

	va_start( argptr, format );
	length = vsprintf( errString + 1, format, argptr );
	va_end( argptr );
	errString[0] = length;

	// set the dialog box strings
	param.movable       = 0;
	param.filterProc    = NULL;
	param.defaultText = "\pOK";
	param.cancelText    = NULL;
	param.otherText     = NULL;
	param.helpButton    = false;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton = 0;
	param.position    = kWindowDefaultPosition;

	GetIndString( briefMsg, rErrStrings, kErrStringError );
	StandardAlert( kAlertStopAlert, briefMsg, (StringPtr) errString, &param, &itemHit );

bail:
	// LBO - we do an ExitToShell here because exit can call some destructors that
	// are in rough shape if the app hasn't started up properly.
	ExitToShell();
}

/*
================
Sys_ErrorString
================
*/
void Sys_ErrorString( int inStringNum ) {
	va_list argptr;
	Str255 errString;
	AlertStdAlertParamRec param;
	short itemHit;
	Str255 briefMsg;
	int length;

	Sys_Shutdown();

	// If we're using DSp on OSX, we need to pause the context so we can see dialogs
	if ( r_fullscreen ) {
		GLimp_pause();
		if ( ( r_fullscreen->integer ) && ( gSystemVersion >= 0x01000 ) ) {
			if ( gDrawSprocket->GetState() == kDSpContextState_Active ) {
				gDrawSprocket->SetState( kDSpContextState_Paused );
			}
		}
	}

// aglSetFullscreen on 10 will not pause correctly, so the dialog
// will not be seen. We just quit the app instead.
//if (gSystemVersion >= 0x1000) goto bail;

	GetIndString( errString, rErrStrings, inStringNum );

	// set the dialog box strings
	param.movable       = 0;
	param.filterProc    = NULL;
	param.defaultText = "\pOK";
	param.cancelText    = NULL;
	param.otherText     = NULL;
	param.helpButton    = false;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton = 0;
	param.position    = kWindowDefaultPosition;

	GetIndString( briefMsg, rErrStrings, kErrStringError );
	StandardAlert( kAlertStopAlert, briefMsg, (StringPtr) errString, &param, &itemHit );

bail:
	// LBO - we do an ExitToShell here because exit can call some destructors that
	// are in rough shape if the app hasn't started up properly.
	ExitToShell();
//	exit(0);
}

void Sys_ErrorLite( int inStringNum ) {
	va_list argptr;
	Str255 errString;
	AlertStdAlertParamRec param;
	short itemHit;
	Str255 briefMsg;
	int length;

	GetIndString( errString, rErrStrings, inStringNum );

	// set the dialog box strings
	param.movable       = 0;
	param.filterProc    = NULL;
	param.defaultText = "\pOK";
	param.cancelText    = NULL;
	param.otherText     = NULL;
	param.helpButton    = false;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton = 0;
	param.position    = kWindowDefaultPosition;

	GetIndString( briefMsg, rErrStrings, kErrStringError );
	StandardAlert( kAlertStopAlert, briefMsg, (StringPtr) errString, &param, &itemHit );

bail:
	// LBO - we do an ExitToShell here because exit can call some destructors that
	// are in rough shape if the app hasn't started up properly.
	ExitToShell();
//	exit(0);
}


/*
================
Sys_ErrorString
================
*/
void Sys_ErrorMisc( OSStatus inErr ) {
	char errString[1024];
	AlertStdAlertParamRec param;
	short itemHit;
	Str255 briefMsg;
	int length;

	Sys_Shutdown();

	// If we're using DSp on OSX, we need to pause the context so we can see dialogs
	if ( r_fullscreen ) {
		GLimp_pause();
		if ( ( r_fullscreen->integer ) && ( gSystemVersion >= 0x01000 ) ) {
			if ( gDrawSprocket->GetState() == kDSpContextState_Active ) {
				gDrawSprocket->SetState( kDSpContextState_Paused );
			}
		}
	}

// aglSetFullscreen on 10 will not pause correctly, so the dialog
// will not be seen. We just quit the app instead.
//if (gSystemVersion >= 0x1000) goto bail;

	GetIndString( (StringPtr)errString, rErrStrings, kErrStringMisc );

	// set the dialog box strings
	param.movable       = 0;
	param.filterProc    = NULL;
	param.defaultText = "\pOK";
	param.cancelText    = NULL;
	param.otherText     = NULL;
	param.helpButton    = false;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton = 0;
	param.position    = kWindowDefaultPosition;

	GetIndString( briefMsg, rErrStrings, kErrStringError );
	StandardAlert( kAlertStopAlert, briefMsg, (StringPtr)errString, &param, &itemHit );

	// LBO - we do an ExitToShell here because exit can call some destructors that
	// are in rough shape if the app hasn't started up properly.
	ExitToShell();
}

/*
================
Sys_Warning
================
*/
void Sys_Warning( const char *format, ... ) {
	va_list argptr;
	char errString[1024];
	AlertStdAlertParamRec param;
	short itemHit;
	Str255 briefMsg;
	int length;

	GLimp_pause();
	// If we're using DSp on OSX, we need to pause the context so we can see dialogs
	if ( ( r_fullscreen->integer ) && ( gSystemVersion >= 0x01000 ) ) {
		if ( gDrawSprocket->GetState() == kDSpContextState_Active ) {
			gDrawSprocket->SetState( kDSpContextState_Paused );
		}
	}


	va_start( argptr, format );
	length = vsprintf( errString + 1, format, argptr );
	va_end( argptr );
	errString[0] = length;

	// set the dialog box strings
	param.movable       = 0;
	param.filterProc    = NULL;
	param.defaultText = "\pOK";
	param.cancelText    = NULL;
	param.otherText     = NULL;
	param.helpButton    = false;
	param.defaultButton = kAlertStdAlertOKButton;
	param.cancelButton = 0;
	param.position    = kWindowDefaultPosition;

	GetIndString( briefMsg, rErrStrings, kErrStringWarning );
	StandardAlert( kAlertStopAlert, briefMsg, (StringPtr) errString, &param, &itemHit );

	// If we're using DSp on OSX, we need to pause the context so we can see dialogs
	if ( ( r_fullscreen->integer ) && ( gSystemVersion >= 0x01000 ) ) {
		if ( gDrawSprocket->GetState() == kDSpContextState_Paused ) {
			gDrawSprocket->SetState( kDSpContextState_Active );
		}
	}

	GLimp_resume();
	GLimp_SetGameGamma();
}


/*
================
Sys_Quit
================
*/
void Sys_Quit( void ) {
	Sys_Shutdown();

	GLimp_pause();
	ExitToShell();
}


//===================================================================

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

//=================================================================================


/*
========================================================================

EVENT LOOP

========================================================================
*/

#define MAX_QUED_EVENTS     256
#define MASK_QUED_EVENTS    ( MAX_QUED_EVENTS - 1 )

sysEvent_t eventQue[MAX_QUED_EVENTS];
int eventHead, eventTail;
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
	sysEvent_t   *ev;

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
=================
Sys_PumpEvents
=================
*/
void Sys_PumpEvents( void ) {
	char        *s;
	msg_t netmsg;
	netadr_t adr;

//Com_Printf ("Sys_PumpEvents\n");

	// pump the message loop
	Sys_SendKeyEvents();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char *b;
		int len;

		len = strlen( s ) + 1;
		b = (char *)Z_Malloc( len );
		if ( !b ) {
			Com_Error( ERR_FATAL, "malloc failed in Sys_PumpEvents" );
		}
		strcpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for other input devices
	Sys_Input();

#if MAC_Q3_MP
	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket( &adr, &netmsg ) ) {
		netadr_t     *buf;
		int len;

		// copy out to a seperate buffer for qeueing
		len = sizeof( netadr_t ) + netmsg.cursize;
		buf = (netadr_t *)Z_Malloc( len );
		if ( !buf ) {
			Com_Error( ERR_FATAL, "malloc failed in Sys_PumpEvents" );
		}
		*buf = adr;
		memcpy( buf + 1, netmsg.data, netmsg.cursize );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}
#endif
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t ev;

//Com_Printf ("Sys_GetEvent\n");

#if MAC_TIME_LIMIT
	CheckTimeLimit();
#endif
	if ( eventHead == eventTail ) {
		Sys_PumpEvents();
	}
	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	// track the mac event "when" to milliseconds rate
	sys_ticBase = sys_lastEventTic;
	sys_msecBase = ev.evTime;

	return ev;
}

#if MAC_WOLF_ET
qboolean Sys_IsNumLockDown( void ) {
	return qfalse;
}

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


Q3DEF_END

//------------------------------------------------------------------------------------
// ¥ IsPressed
//------------------------------------------------------------------------------------
short IsPressed( unsigned short k ) {
// FIXME: don't think that ever worked
#if 0
	unsigned char km[16];
	KeyMap *keymap = (KeyMap*) &km;
#if TARGET_API_MAC_CARBON
	GetKeys( (SInt32 *) *keymap );
#else
	GetKeys( (UInt32 *) *keymap );
#endif
	return ( ( km[k >> 3] >> ( k & 7 ) ) & 1 );
#else
	return 0;
#endif
}

#pragma mark -
extern "C"
{
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
}
#pragma mark -

#if TARGET_API_MAC_CARBON
//===============================================================================
//	SetDefaultDirectory
//
//	Under native OS X, the default directory for apps is some bizarro Unix
//  location. Here we redefine it to the classic MacOS location, which is the
//  same folder that the app is running from.
//===============================================================================

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
#endif

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// GetButtonValue
//
// A helper routine to read the state of a dialog control without all the crap
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
short GetButtonValue( DialogRef theDialog, short item ) {
	short theType;
	Handle theHndl;
	Rect theRect;

	GetDialogItem( theDialog, item, &theType, &theHndl, &theRect );
	return ( GetControlValue( (ControlRef) theHndl ) );
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// SetButtonValue
//
// A helper routine to set the state of a dialog control without all the crap
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void SetButtonValue( DialogRef theDialog, short item, short value ) {
	short theType;
	Handle theHndl;
	Rect theRect;

	GetDialogItem( theDialog, item, &theType, &theHndl, &theRect );
	SetControlValue( (ControlRef) theHndl, value );
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// DisableDialogControl
//
// A helper routine to disable a dialog button without all the crap
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void DisableDialogControl( DialogRef theDialog, short item ) {
	short theType;
	Handle theHndl;
	Rect theRect;

	GetDialogItem( theDialog, item, &theType, &theHndl, &theRect );
	HiliteControl( (ControlRef) theHndl, kControlInactivePart );
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// EnableDialogControl
//
// A helper routine to disable a dialog button without all the crap
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void EnableDialogControl( DialogRef theDialog, short item ) {
	short theType;
	Handle theHndl;
	Rect theRect;

	GetDialogItem( theDialog, item, &theType, &theHndl, &theRect );
	HiliteControl( (ControlRef) theHndl, kControlNoPart );
}

//===============================================================================
//	GetDialogItemType
//
//	Returns the type of a dialog item.
//===============================================================================

DialogItemType GetDialogItemType( DialogRef inDialog, DialogItemIndex inItem ) {
	DialogItemType itemType;
	Handle itemHandle;
	Rect itemBounds;

	GetDialogItem( inDialog, inItem, &itemType, &itemHandle, &itemBounds );
	return itemType;
}

//===============================================================================
//	GetDialogControlValue
//
//	Returns the value of the control attached to a dialog item
//===============================================================================

SInt16 GetDialogControlValue( DialogRef inDialog, DialogItemIndex inItem ) {
	ControlRef controlRef;

	// fetch the control handle and return the value
	if ( GetDialogItemAsControl( inDialog, inItem, &controlRef ) == noErr ) {
		return GetControlValue( controlRef );
	} else {
		return 0;
	}
}

//===============================================================================
//	SetDialogControlValue
//
//	Sets the value of the control attached to a dialog item
//===============================================================================

void SetDialogControlValue( DialogRef inDialog, DialogItemIndex inItem, SInt16 inValue ) {
	ControlRef controlRef;

	// fetch the control handle and set the value
	if ( GetDialogItemAsControl( inDialog, inItem, &controlRef ) == noErr ) {
		if ( GetControlValue( controlRef ) != inValue ) {
			SetControlValue( controlRef, inValue );
		}
	}
}


//===============================================================================
//	GetDialogItemTextAppearance
//
//	Returns the text of a static/edit text item assuming the Appearance Manager.
//===============================================================================

void GetDialogItemTextAppearance( DialogRef inDialog, DialogItemIndex inItem, StringPtr outText ) {
	ControlRef controlRef;
	OSStatus status = noErr;

	// fetch the control handle and set the value
	status = GetDialogItemAsControl( inDialog, inItem, &controlRef );
	if ( status == noErr ) {
		GetDialogItemText( (Handle)controlRef, outText );
	}
}

#pragma mark -
#pragma mark ¥ Apple Event Handlers / Only Mortal Events

static char *GetCommandLineParameters( const AppleEvent *appleEvt, AEKeyword keyWord ) {
	OSErr theError;
	DescType typeCode;
	Size dataSize;
	Size actualSize;
	char *theCommandLine = NULL;

	if ( ( theError = AESizeOfParam( appleEvt, keyWord, &typeCode, &dataSize ) ) == noErr && dataSize > 0 ) {
		if ( ( theCommandLine = (char*)malloc( dataSize + 1 ) ) == NULL ) {
			return NULL;
		}

		if ( ( theError = AEGetParamPtr( appleEvt, keyWord, typeChar, &typeCode, theCommandLine, dataSize, &actualSize ) ) != noErr || dataSize != actualSize ) {
			free( (void*)theCommandLine );
			theCommandLine = NULL;
			return NULL;
		}

		theCommandLine[ (int)actualSize ] = '\0';
	}

	return theCommandLine;
}

static pascal OSErr RunAppleEventHandler( const AppleEvent *appleEvt, AppleEvent *reply, long refcon ) {
#pragma unused( reply, refcon )
	// Parameters pass in with an 'orun' event. 'CLin' seems to be the defacto standard, as seen in Unreal Tournament, so we use it too.
	// The normal behavior of 'orun' is to open an empty document, but that doesn't make much sense here!

	if ( ( sCommandLine = GetCommandLineParameters( appleEvt, 'CLin' ) ) != NULL ) {
//		char theDebugMessage[ 1024 ];
	}

	return noErr;
}

static pascal OSErr QuitAppleEventHandler( const AppleEvent *appleEvt, AppleEvent* reply, long refcon ) {
#pragma unused (appleEvt, reply, refcon)
	if ( gUseCarbonEvents ) {
		QuitApplicationEventLoop();
	} else {
		Com_Quit_f();
	}

	return noErr;
//#pragma noreturn (QuitAppleEventHandler)
}

#if MAC_Q3_MP
static pascal OSErr OMExecuteAppleEventHandler( const AppleEvent *appleEvt, AppleEvent *reply, long refcon ) {
#pragma unused( reply, refcon )
	/* Ensure we dump any previous command line string */

	if ( sCommandLine != NULL ) {
		free( (void*)sCommandLine );
	}
	sCommandLine = NULL;

#if !DEDICATED // ¥¥¥
	if ( ( sCommandLine = GetCommandLineParameters( appleEvt, 'CLin' ) ) != NULL ) {
		Sys_SendStringToConsole( sCommandLine );
		Com_Printf( "]%s (via AppleEvent)\n", sCommandLine );
	}
#endif

	return noErr;
}
#endif

static OSErr InitAppleEvents( void ) {
	OSErr err = noErr;

	err = AEInstallEventHandler( kCoreEventClass, kAEOpenApplication, NewAEEventHandlerUPP( RunAppleEventHandler ), 0, false );
	if ( err != noErr ) {
		goto bail;
	}

	err = AEInstallEventHandler( kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP( QuitAppleEventHandler ), 0, false );
	if ( err != noErr ) {
		goto bail;
	}

#if MAC_Q3_MP
	err = AEInstallEventHandler( kOMEventClass, kOMExecuteCommands, NewAEEventHandlerUPP( OMExecuteAppleEventHandler ), 0, false );
	if ( err != noErr ) {
		goto bail;
	}
#endif

bail:
	return err;
}

#pragma mark -

/*
=============
InitMacStuff
=============
*/
void InitMacStuff( void ) {
	Handle menuBar;
	char dir[MAX_OSPATH];
	OSErr err;
	long qtVer;
	long response;
	Boolean bail = false;

	OSStatus error;
	SecuritySessionId mySession;
	SessionAttributeBits sessionInfo;

	error = SessionGetInfo( callerSecuritySession, &mySession, &sessionInfo );
	if ( sessionInfo & sessionHasGraphicAccess ) {
		macSessionGraphics = true;
	}

	MoreMasterPointers( 64UL * 10 );

	if ( macSessionGraphics ) {
		InitCursor();
	}

	// Install some Apple Events handlers
	err = InitAppleEvents();
	if ( err != noErr ) {
		ExitToShell();
	}

	// We do this early because it relies on grabbing the first apple event from the Finder.
	sCommandLine_temp = ReadCommandLineParms();

	// Preflight some stuff

#if MAC_Q3_QUICKTIME
	// Check for QuickTime 4.0 so we can play .mp3 files
	err = Gestalt( gestaltQuickTime, (long *) &qtVer );
	if ( err ) {
		qtVer = 0L;
	}

	// If QT4 isn't present, tell user he won't have mp3 audio
	if ( qtVer < 0x04000000 ) {
		Sys_ErrorLite( kErrStringQuickTime );
	}
#endif

	// Are we on 10?
	err = Gestalt( gestaltSystemVersion, &gSystemVersion );

	// We bail on < MacOS 10.2.8
	if ( gSystemVersion < 0x1028 ) {
		Sys_ErrorLite( kErrStringWrongOSX );
	}

	// No DrawSprocket? Then freak out! We could run in a window, but...eh.
	if ( (Ptr) DSpStartup == (Ptr) kUnresolvedCFragSymbolAddress ) {
		Sys_ErrorLite( kErrStringDrawSprocket );
	}

	if ( !MPLibraryIsLoaded() ) {
		Sys_ErrorLite( kErrStringMisc );
	}

#if MAC_TIME_LIMIT
	CheckTimeLimit();
#endif

#if TARGET_API_MAC_CARBON
	// Tell OSX to set the local directory to the one the app lives in
	SetDefaultDirectory();
#endif

	// see if we should modify quit in accordance with the Aqua HI guidelines
	err = Gestalt( gestaltMenuMgrAttr, &response );
	if ( ( err == noErr ) && ( response & gestaltMenuMgrAquaLayoutMask ) ) {
		menuBar = GetNewMBar( rMenuBarX );
	} else
	{
		menuBar = GetNewMBar( rMenuBar );
	}

	// init menu
	if ( !menuBar ) {
		Com_Error( ERR_FATAL, "MenuBar not found." );
	}

	SetMenuBar( menuBar );
	DisposeHandle( menuBar );

	DrawMenuBar();

#if MAC_Q3_QUICKTIME
	EnterMovies();
#endif

	SetEventMask( -1 );

}

//==================================================================================

// OptionDialog
//
// Configures Mac-specific options at startup. Returns true if we're to quit
Boolean OptionDialog( void ) {
	short itemHit;
	DialogRef theDialog;
	GWorldPtr savedPort;
	GDHandle savedDevice;
	Boolean done;
	Cursor arrow;
	DisplayIDType theMonitor = 0;
	OSStatus status = noErr;

// Dialog item constants
	enum {
		rOptionsDialog = 130,

		dMenuFrequency = 3,
		dButtonQuit,
		dButtonMonitors,
		dButtonConfigJoysticks
	};

	status = DSpStartup();
	if ( status != noErr ) {
		Sys_ErrorMisc( status );
	}

	// attempt to fetch the alert dialog
	theDialog = GetNewDialog( rOptionsDialog, NULL, ( WindowRef ) - 1 );
	if ( theDialog == NULL ) {
		return false;
	}

	// point to the port
	GetGWorld( &savedPort, &savedDevice );
	SetPortDialogPort( theDialog );

	// set the default items
	SetDialogDefaultItem( theDialog, ok );
	SetDialogCancelItem( theDialog, cancel );

	// Set up the controls
	SetButtonValue( theDialog, dMenuFrequency, macPrefs.monitorFrequency );
	theMonitor = macPrefs.displayID;

	// Disable the "choose monitor" button if we've only got one to pick from
	{
		GDHandle currentMonitor;
		UInt32 totalMonitors = 0;

		// Walk the device list
		for ( currentMonitor = GetDeviceList(); currentMonitor != NULL; currentMonitor = GetNextDevice( currentMonitor ) )
		{
			totalMonitors++;
		}

		// Only 1 monitor found
		if ( totalMonitors == 1 ) {
			DisableDialogControl( theDialog, dButtonMonitors );
			theMonitor = 0;
		}
	}

	// Disable the joystick config on OS 9
	if ( gSystemVersion < 0x1000 ) {
		DisableDialogControl( theDialog, dButtonConfigJoysticks );
	}

	// display the alert and make sure the cursor is an arrow
	ShowWindow( GetDialogWindow( theDialog ) );
	SetCursor( GetQDGlobalsArrow( &arrow ) );

	// loop on ModalDialog until we're done
	done = false;
	while ( !done )
	{
		ModalDialog( NULL, &itemHit );
		switch ( itemHit )
		{
		case ok:
		case cancel:
		case dButtonQuit:
			done = true;
			break;
		case dButtonConfigJoysticks:
			//ConfigJoystick_HID();
			break;
		case dButtonMonitors:
		{
			status = PickMonitorDialog( &theMonitor );
			break;
		}
		}
	}

	// Read back the controls
	if ( itemHit == ok ) {
		macPrefs.monitorFrequency = GetButtonValue( theDialog, dMenuFrequency );
		macPrefs.displayID = theMonitor;
	}

bail:
	// tear it down
	DisposeDialog( theDialog );
	SetGWorld( savedPort, savedDevice );

	// If the user wants to quit, return true
	if ( itemHit == dButtonQuit ) {
		return true;
	} else {
		return false;
	}
}

/*
=============
ReadCommandLineParms

Read startup options from a 'CLin' apple event, a text file or dialog box
=============
*/
char *ReadCommandLineParms( void ) {
	FILE    *f;
	int len;
	char    *buf;
	EventRecord event;
	UInt32 timeoutTicks;

	sGameRangerHosting = false;

	memset( &event, 0, sizeof( EventRecord ) );

	// wait for the first AppleEvent to come through, but time out after 1/2 second
	timeoutTicks = TickCount() + 30;
	while ( !WaitNextEvent( highLevelEventMask, &event, 0x7fffffff, NULL ) && TickCount() < timeoutTicks ) {};

	// Attempt to handle the first AppleEvent (usually oapp or odoc)
	AEProcessAppleEvent( &event );

	// If the 'oapp' event set up the command line, return it
	if ( sCommandLine != NULL ) {
		return sCommandLine;
	}

	/*
#if MAC_Q3_MP
	// Finally, use GameRanger to fill out the command line if nothing else
	if (GRCheckFileForCmd())
	{
		GRGetWaitingCmd();
		if (GRHasProperty( 'Exec' ))
		{
			char *grName = GRGetPropertyStr( 'Exec' );
			// otherwise check for a parms file
			f = fopen( grName, "r" );
			if ( !f )
			{
				return "";
			}
			fseek( f, 0, SEEK_END );
			len = ftell( f );
			fseek( f, 0, SEEK_SET );
			buf = (char *) malloc( len + 1 );
			if ( !buf )
			{
				Sys_Error ("Could not launch via GameRanger, %s length %d", grName, len);
			}
			buf[len] = 0;
			fread( buf, len, 1, f );
			fclose( f );

			sGameRangerHosting = true;

			return buf;
		}
	}
#endif // MAC_Q3_MP
	*/

	return NULL;
}

/*
=============
main
=============
*/
int main( int argc, const char *argv[] ) {
	char    *commandLine;
	char realCommandLine[1024];
	int i;
	Boolean bail = false;

	InitMacStuff();

	LoadPrefs( &macPrefs );

	// If option or command is held down, throw up a config dialog
	if ( IsPressed( kOptionKey ) || IsPressed( kCommandKey ) ) {
		bail = OptionDialog();
	}

	if ( bail ) {
		ExitToShell();
	}

	switch ( macPrefs.monitorFrequency )
	{
	case kMenuRefreshAuto: gRefreshRate = 0; break;
	case kMenuRefresh60:   gRefreshRate = kFrequency60; break;
	case kMenuRefresh67:   gRefreshRate = kFrequency67; break;
	case kMenuRefresh70:   gRefreshRate = kFrequency70; break;
	case kMenuRefresh75:   gRefreshRate = kFrequency75; break;
	case kMenuRefresh80:   gRefreshRate = kFrequency80; break;
	case kMenuRefresh85:   gRefreshRate = kFrequency85; break;
	case kMenuRefresh90:   gRefreshRate = kFrequency90; break;
	case kMenuRefresh99:   gRefreshRate = kFrequency99; break;
	case kMenuRefresh100:  gRefreshRate = kFrequency100; break;
	case kMenuRefresh120:  gRefreshRate = kFrequency120; break;
	case kMenuRefresh124:  gRefreshRate = kFrequency124; break;
	default: gRefreshRate = 0; break;
	}

	// Write the changes back out now, in case we bail before the app quits properly
	SavePrefs( &macPrefs );

	// Default to 1 for carbon events if we're on 10
	// Otherwise, default to old-school events on 8/9
	if ( gSystemVersion >= 0x1000 ) {
		gUseCarbonEvents = true;
	} else {
		gUseCarbonEvents = false;
	}

	if ( gUseCarbonEvents ) {
		InstallStandardEventHandler( GetApplicationEventTarget() );
	}

	Sys_CreateConsole();

	// get the initial time base
	Sys_Milliseconds();

	if ( sCommandLine_temp ) {
		strcpy( realCommandLine, sCommandLine_temp );
	} else {
		realCommandLine[ 0 ] = '\0';
		for ( i = 1; i < argc; i++ ) {
			if ( i > 1 ) {
				strcat( realCommandLine, " " );
			}
			strcat( realCommandLine, argv[ i ] );
		}
	}

	Com_Init( realCommandLine );

	sys_profile = Cvar_Get( "sys_profile", "0", 0 );
	sys_profile->modified = qfalse;

#ifndef DEDICATED
	// LBO - 1.11a patch. Use carbon events when running as a dedicated server. This prevents all the
	// hitching warnings when resizing the windows, etc.
	if ( com_dedicated->integer && gSystemVersion >= 0x1000 ) {
		gUseCarbonEvents = true;
	}
#endif

#if 0 && MAC_DEBUG
	Com_Printf( "SYS_DLLNAME_QAGAME + %d: '%s'\n", SYS_DLLNAME_QAGAME_SHIFT, FS_ShiftStr( "qagame_mac", SYS_DLLNAME_QAGAME_SHIFT ) );
	Com_Printf( "SYS_DLLNAME_CGAME + %d: '%s'\n", SYS_DLLNAME_CGAME_SHIFT, FS_ShiftStr( "cgame_mac", SYS_DLLNAME_CGAME_SHIFT ) );
	Com_Printf( "SYS_DLLNAME_UI + %d: '%s'\n", SYS_DLLNAME_UI_SHIFT, FS_ShiftStr( "ui_mac", SYS_DLLNAME_UI_SHIFT ) );
#endif

	if ( gUseCarbonEvents ) {
		Carbon_InstallTimer();
		Carbon_InstallEvents();
		RunApplicationEventLoop();
		Com_Quit_f();
	} else
	{
		while ( 1 )
		{
			// run the frame
			Com_Frame();
		}
	}

	return 0;
}
