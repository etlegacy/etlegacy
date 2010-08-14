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

#define text_type       'TEXT'
#define text_creator    'ttxt'

#undef fopen

#include <stdio.h>
unsigned int _ftype;
unsigned int _fcreator;
#include <unistd.h>
#include <sys/stat.h>
#define USE_POSIX_PATH  1

#if USE_POSIX_PATH
const char *kPathSep = "/";
#else
const char *kPathSep = ":";
#endif

#include <math.h>
Q3DEF_BEGIN
	#if MAC_MOHAA && GAME_DLL
		#include "../fgame/g_local.h"
	#else
		#include "../client/client.h"
	#endif
	#include "mac_local.h"
Q3DEF_END

#if __cplusplus
extern "C" {
#endif

void GetHFSPathFromPosix( const char *inPath, char *outPath ) {
	CFStringRef theFileName = NULL;
	CFStringRef newPathString = NULL;
	CFURLRef theFileURL = NULL;

	theFileName = CFStringCreateWithCString( CFAllocatorGetDefault(), inPath, kCFStringEncodingMacRoman );
	if ( !theFileName ) {
		goto bail;
	}
	theFileURL = CFURLCreateWithFileSystemPath( CFAllocatorGetDefault(), theFileName, kCFURLPOSIXPathStyle, false );
	if ( !theFileURL ) {
		goto bail;
	}
	newPathString = CFURLCopyFileSystemPath( theFileURL, kCFURLHFSPathStyle );
	if ( !newPathString ) {
		goto bail;
	}

	CFStringGetCString( newPathString, outPath, MAX_OSPATH, kCFStringEncodingMacRoman );

	CFRelease( theFileURL );
	CFRelease( theFileName );
	CFRelease( newPathString );

	return;
bail:
	if ( theFileURL ) {
		CFRelease( theFileURL );
	}
	if ( theFileName ) {
		CFRelease( theFileName );
	}
	if ( newPathString ) {
		CFRelease( newPathString );
	}

	*outPath = 0;
}

void GetPosixPathFromHFS( const char *inPath, char *outPath ) {
	CFStringRef theFileName = NULL;
	CFStringRef newPathString = NULL;
	CFURLRef theFileURL = NULL;

	theFileName = CFStringCreateWithCString( CFAllocatorGetDefault(), inPath, kCFStringEncodingMacRoman );
	if ( !theFileName ) {
		goto bail;
	}
	theFileURL = CFURLCreateWithFileSystemPath( CFAllocatorGetDefault(), theFileName, kCFURLHFSPathStyle, false );
	if ( !theFileURL ) {
		goto bail;
	}
	newPathString = CFURLCopyFileSystemPath( theFileURL, kCFURLPOSIXPathStyle );
	if ( !newPathString ) {
		goto bail;
	}

	CFStringGetCString( newPathString, outPath, MAX_OSPATH, kCFStringEncodingMacRoman );

	CFRelease( theFileURL );
	CFRelease( theFileName );
	CFRelease( newPathString );

	return;
bail:
	if ( theFileURL ) {
		CFRelease( theFileURL );
	}
	if ( theFileName ) {
		CFRelease( theFileName );
	}
	if ( newPathString ) {
		CFRelease( newPathString );
	}

	*outPath = 0;
}

void FILE_SetCreator( const char *inPath, OSType inCreator, OSType inType ) {
	CFStringRef theFileName = NULL;
	FSRef fileRef;
	FSSpec fileSpec;
	CFURLRef theFileURL = NULL;
	Boolean success;
	OSErr err;

	theFileName = CFStringCreateWithCString( CFAllocatorGetDefault(), inPath, kCFStringEncodingMacRoman );
	if ( !theFileName ) {
		goto bail;
	}
#if USE_POSIX_PATH
	theFileURL = CFURLCreateWithFileSystemPath( CFAllocatorGetDefault(), theFileName, kCFURLPOSIXPathStyle, false );
#else
	theFileURL = CFURLCreateWithFileSystemPath( CFAllocatorGetDefault(), theFileName, kCFURLHFSPathStyle, false );
#endif
	if ( !theFileURL ) {
		goto bail;
	}
	success = CFURLGetFSRef( theFileURL, &fileRef );
	if ( !success ) {
		goto bail;
	}

	err = FSGetCatalogInfo( &fileRef, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL );

	FInfo finderInfo;

	FSpGetFInfo( &fileSpec, &finderInfo );
	finderInfo.fdCreator = inCreator;
	finderInfo.fdType = inType;
	FSpSetFInfo( &fileSpec, &finderInfo );

bail:
	if ( theFileURL ) {
		CFRelease( theFileURL );
	}
	if ( theFileName ) {
		CFRelease( theFileName );
	}

	return;
}

FILE * mac_fopen( const char *filename, const char * open_mode ) {
	if ( strstr( filename, ".cfg" ) ||
		 strstr( filename, ".log" ) ||
		 strstr( filename, ".txt" ) ) {
		_ftype = text_type;
		_fcreator = text_creator;
	} else if ( strstr( filename, ".tga" ) )       {
		_ftype = 'TPIC';
		_fcreator = 'prvw';
	} else if ( strstr( filename, ".jpg" ) )       {
		_ftype = 'JPEG';
		_fcreator = 'prvw';
	} else
	{
		_ftype = kGameBinaryType;
		_fcreator = kGameCreator;
	}
//	char tempPath[MAX_OSPATH];
	FILE *reply = NULL;
	int create = strchr( open_mode, 'w' ) || strchr( open_mode, 'a' );

//	GetPosixPathFromHFS (filename, tempPath);
	reply = fopen( filename, open_mode );
	if ( reply && create ) {
		FILE_SetCreator( filename, _fcreator, _ftype );
	}
	return reply;
}

Boolean AmIBundled( void ) {
	FSRef processRef;
	FSCatalogInfo processInfo;
	int isBundled;
	ProcessSerialNumber psn = {0, kCurrentProcess};

	GetProcessBundleLocation( &psn, &processRef );
	FSGetCatalogInfo( &processRef, kFSCatInfoNodeFlags, &processInfo, NULL, NULL, NULL );
	isBundled = processInfo.nodeFlags & kFSNodeIsDirectoryMask;

	return( isBundled );
}

OSErr GetFullPathFromSpec( const FSSpec *spec, char *fullpath ) {
	OSErr err;
	FSRef theRef;
	FSSpec tempSpec;
	Boolean isFolder, wasAliased;

	// Make a copy of the spec and attempt to resolive it if it's an alias
	tempSpec = *spec;
	err = ResolveAliasFile( &tempSpec, true, &isFolder, &wasAliased );

	// Conver the FSSpec to an FSRef and attempt to get a full path from it.
	err = FSpMakeFSRef( &tempSpec, &theRef );
	if ( err == noErr ) {
		FSRefMakePath( &theRef, (UInt8 *) fullpath, MAX_OSPATH );
		strcat( fullpath, kPathSep );
	}

	// It's not possible to have an FSRef for files that don't exist yet. In this
	// case, we create an FSRef for the parent directory, convert that to a full path
	// and append the filename to that.
	if ( err == fnfErr ) {
		FSRefParam paramBlock;
		CFURLRef theParentURL;
		CFStringRef theFileName;
		CFURLRef theFileURL;
		Boolean successful;

		paramBlock.ioVRefNum = spec->vRefNum;
		paramBlock.ioNamePtr = NULL;
		paramBlock.ioDirID = spec->parID;
		paramBlock.newRef = &theRef;
		err = PBMakeFSRefSync( &paramBlock );
		if ( err == noErr ) {
			theParentURL = CFURLCreateFromFSRef( CFAllocatorGetDefault(), paramBlock.newRef );
			theFileName = CFStringCreateWithBytes( CFAllocatorGetDefault(), spec->name + 1,
												   spec->name[0], kCFStringEncodingMacRoman,
												   false );
			theFileURL = CFURLCreateWithFileSystemPathRelativeToBase( CFAllocatorGetDefault(),
																	  theFileName,
#if USE_POSIX_PATH
																	  kCFURLPOSIXPathStyle, false,
#else
																	  kCFURLHFSPathStyle, false,
#endif
																	  theParentURL );
			successful = CFURLGetFileSystemRepresentation( theFileURL, true, (UInt8*)fullpath, MAX_OSPATH );
			CFRelease( theFileURL );
			CFRelease( theFileName );
			CFRelease( theParentURL );
		}
	}

	return err;
}

FILE * FSp_fopen( const FSSpec *spec, const char *open_mode ) {
	char path[MAX_OSPATH];
	OSErr err;

	err = GetFullPathFromSpec( spec, path );
	if ( err == noErr ) {
		return fopen( path, open_mode );
	} else {return NULL;}
}

OSErr GetApplicationFSSpec( FSSpecPtr theFSSpecPtr ) {
	ProcessSerialNumber serial;
	ProcessInfoRec info;
	OSErr err;

	serial.highLongOfPSN = 0;
	serial.lowLongOfPSN = kCurrentProcess;

	info.processInfoLength = sizeof( ProcessInfoRec );
	info.processName = NULL;
	info.processAppSpec = theFSSpecPtr;

	err = GetProcessInformation( &serial, &info );

	return err;
}

OSErr GetApplicationPackageFSSpecFromBundle( FSSpecPtr theFSSpecPtr ) {
	FSRef myBundleRef;
	CFBundleRef refMainBundle = NULL;
	CFURLRef refMainBundleURL = NULL;
	Boolean ok;

	// get app bundle
	refMainBundle = CFBundleGetMainBundle();
	if ( !refMainBundle ) {
		return paramErr;
	}

	// create a URL to the app bundle
	refMainBundleURL = CFBundleCopyBundleURL( refMainBundle );
	if ( !refMainBundleURL ) {
		return paramErr;
	}

	ok = CFURLGetFSRef( refMainBundleURL, &myBundleRef );
	CFRelease( refMainBundleURL );
	if ( !ok ) {
		return fnfErr;
	}

	return FSGetCatalogInfo( &myBundleRef, kFSCatInfoNone, NULL, NULL, theFSSpecPtr, NULL );
}

#if __cplusplus
}
#endif


#pragma mark -
void Sys_Mkdir( const char *inPath ) {
	// LBO 1/30/05. Modified to create any intermediate folders. If built
	// against MSL, mkdir tends to return EMACOSERR for any kind of
	// failure instead of, e.g., EEXIST or ENOENT.

	if ( inPath == NULL ) {
		return;
	}

	char macPath[MAX_OSPATH];
	strcpy( macPath, inPath );

	// Don't bother with "" or "/", just return success.
	if ( macPath[0] == 0 || ( macPath[0] == kPathSep[0] && macPath[1] == 0 ) ) {
		return;
	}

	// Don't bother if we've got an app-relative path. Those are pointing
	// to the wrong place anyway.
	if ( !strchr( &macPath[1], kPathSep[0] ) ) {
		return;
	}

	int result = mkdir( macPath, 0777 );

	// Don't fail if the folder already exists.
	if ( result != 0 && ( errno == EEXIST ) ) {
		result = 0;
	}

	if ( result != 0 /*&& errno == ENOENT*/ ) {
		// ENOENT: A component of the path prefix does not exist
		// Parse path and create each folder in path.

		int len = strlen( macPath );

		// Remove trailing delimiter if it exists, we'll use null terminator instead.
		if ( macPath[len - 1] == kPathSep[0] ) {
			macPath[--len] = 0;
		}

		// Only try this if there is a delimiter in the path.
		if ( strchr( &macPath[1], kPathSep[0] ) ) {
			result = 0;
			int pos = strchr( &macPath[1], kPathSep[0] ) - &macPath[0] + 1;
			while ( /*result == 0 &&*/ pos <= len )
			{
				int c = macPath[pos];
				if ( ( c == kPathSep[0] || c == 0 ) && ( pos != 0 ) ) {
					// Temporarily truncate the path and create that folder.
					macPath[pos] = 0;
					result = mkdir( macPath, 0777 );
					macPath[pos] = c;

					// Don't fail if the folder already exists.
					if ( result != 0 && errno == EEXIST ) {
						result = 0;
					}
				}
				pos++;
			}
		}
	}
}

char *Sys_Cwd( void ) {
	static char dir[MAX_OSPATH];
	int l;

	if ( AmIBundled() ) {
		FSSpec bundleSpec;
		OSErr err;

		GetApplicationPackageFSSpecFromBundle( &bundleSpec );
		bundleSpec.name[0] = 0;
		err = GetFullPathFromSpec( &bundleSpec, dir );
	} else
	{
		FSSpec bundleSpec;
		OSErr err;

		GetApplicationFSSpec( &bundleSpec );
		bundleSpec.name[0] = 0;
		err = GetFullPathFromSpec( &bundleSpec, dir );
	}

	// strip off the last delimiter
	l = strlen( dir );
	if ( l > 0 ) {
#if USE_POSIX_PATH
		if ( dir[l - 1] == '/' )
#else
		if ( dir[l - 1] == ':' )
#endif
		{ dir[l - 1] = 0;}
	}
	return dir;
}

char *Sys_DefaultCDPath( void ) {
	// LBO - We treat the game directory as un-writable to work better
	// with multiple users. Therefore, the "CD Path" will point to the
	// install path for the application.
	return Sys_Cwd();
}

char *Sys_DefaultInstallPath( void ) {
	// The base path of the installation. If the Q3 engine supports this call,
	// it means that it differentiates between the install path and a specific
	// home directory path for the user. In this case, Sys_DefaultBasePath()
	// is not used.
	return Sys_Cwd();
}

char *Sys_DefaultHomePath( void ) {
	static char fullpath[MAX_OSPATH];
	OSErr err;
	long dirID;
	short refNum;
	FSSpec spec;

	if ( 1 ) { //(gSystemVersion >= 0x1000)
			  // The "base path" is a writable location. We want to play
			  // nicely with multiple users, so we point the base path to the
			  // Application Support-><GameName> folder.
		err = FindFolder( kUserDomain, kApplicationSupportFolderType, kCreateFolder, &refNum, &dirID );

		spec.parID = dirID;
		spec.vRefNum = refNum;
		spec.name[0] = 0;

		err = GetFullPathFromSpec( &spec, fullpath );
		strcat( fullpath, kGameFolderName );
		Sys_Mkdir( fullpath );
		return fullpath;
	} else {
		return Sys_Cwd();
	}
}

char *Sys_DefaultBasePath( void ) {
	// In older versions of the Q3 engine, it doesn't differentiate between the install path
	// and the user's home directory. Instead, it calls the installation path the "base path".
	// In this case, we point the install to the home directory and fake up the CD path to point
	// to the install path, since we never use the CD path anyway.
	return Sys_DefaultHomePath();
}


/*
 =================================================================================

 FILE FINDING

 =================================================================================
*/

#define MAX_FOUND_FILES 0x1000

#if MAC_JK2_MP || MAC_MOHAA || MAC_WOLF
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles ) {
	char search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char filename[MAX_OSPATH];
	int VRefNum;
	int DrDirId;
	FSSpec fsspec;
	int index;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	// LBO 6/1/04 - bugfix. "basedir" should already have a path delimiter on it.
	if ( strlen( subdirs ) ) {
		Com_sprintf( search, sizeof( search ), "%s%s:", basedir, subdirs );
	} else
	{
		Com_sprintf( search, sizeof( search ), "%s", basedir );
	}

	// get the volume and directory numbers
	// there has to be a better way than this...
	{
		CInfoPBRec paramBlock;

		Q_strncpyz( search, search, sizeof( search ) );
		c2pstrcpy( (StringPtr) search, search );
		FSMakeFSSpec( 0, 0, (unsigned char *)search, &fsspec );

		VRefNum = fsspec.vRefNum;

		memset( &paramBlock, 0, sizeof( paramBlock ) );
		paramBlock.hFileInfo.ioNamePtr = (unsigned char *)search;
		PBGetCatInfoSync( &paramBlock );

		DrDirId = paramBlock.hFileInfo.ioDirID;
	}


	for ( index = 1 ; ; index++ )
	{
		CInfoPBRec paramBlock;
		char macFileName[MAX_OSPATH];
		OSErr err;

		memset( &paramBlock, 0, sizeof( paramBlock ) );
		paramBlock.hFileInfo.ioNamePtr = (unsigned char *)macFileName;
		paramBlock.hFileInfo.ioVRefNum = VRefNum;
		paramBlock.hFileInfo.ioFDirIndex = index;
		paramBlock.hFileInfo.ioDirID = DrDirId;

		err = PBGetCatInfoSync( &paramBlock );

		if ( err != noErr ) {
			break;
		}

		p2cstrcpy( macFileName, (const StringPtr)macFileName );

		if ( paramBlock.hFileInfo.ioFlAttrib & 16 ) {
			if ( strlen( subdirs ) ) {
				Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s:%s", subdirs, macFileName );
			} else
			{
				Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s", macFileName );
			}
			Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}

		// LBO 6/1/04 - bugfix. If no subdirectory string, just use "macFileName".
		if ( strlen( subdirs ) ) {
			Com_sprintf( filename, sizeof( filename ), "%s:%s", subdirs, macFileName );
		} else {
			Com_sprintf( filename, sizeof( filename ), "%s", macFileName );
		}
		if ( !Com_FilterPath( filter, filename, qfalse ) ) {
			continue;
		}
		list[ *numfiles ] = CopyString( filename );
		( *numfiles )++;
	}
}
#endif

#pragma mark Sys_ListFiles
#if MAC_MOHAA
Q3DEF char **Sys_ListFiles( const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs  )
#elif MAC_WOLF || MAC_JK2_MP
Q3DEF char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs  )
#else
Q3DEF char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs  )
#endif
{
	int nfiles;
	char        **listCopy;
	char pdirectory[MAX_OSPATH];
	char        *list[MAX_FOUND_FILES];
	int directoryFlag;
	int i;
	int extensionLength;
	int VRefNum;
	int DrDirId;
	int index;
	FSSpec fsspec;

#if MAC_JK2_MP || MAC_MOHAA || MAC_WOLF
	if ( filter ) {
		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", (char *)filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if ( !nfiles ) {
			return NULL;
		}

#if MAC_Q3_OLDSTUFF
		listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
#else
		listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS, qfalse );
#endif
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}
#endif

	// get the volume and directory numbers
	// there has to be a better way than this...
	{
		CInfoPBRec paramBlock;

#if USE_POSIX_PATH
		GetHFSPathFromPosix( directory, pdirectory );
//		Q_strncpyz( pdirectory, directory, sizeof(pdirectory) );
#else
		Q_strncpyz( pdirectory, directory, sizeof( pdirectory ) );
#endif
		c2pstrcpy( (StringPtr) pdirectory, pdirectory );
		FSMakeFSSpec( 0, 0, (unsigned char *)pdirectory, &fsspec );

		VRefNum = fsspec.vRefNum;

		memset( &paramBlock, 0, sizeof( paramBlock ) );
		paramBlock.hFileInfo.ioNamePtr = (unsigned char *)pdirectory;
		PBGetCatInfoSync( &paramBlock );

		DrDirId = paramBlock.hFileInfo.ioDirID;
	}

	if ( !extension ) {
		extension = "";
	}
	extensionLength = strlen( extension );

	if ( wantsubs || ( extension[0] == '/' && extension[1] == 0 ) ) {
		directoryFlag = 16;
	} else {
		directoryFlag = 0;
	}

	nfiles = 0;

	for ( index = 1 ; ; index++ ) {
		CInfoPBRec paramBlock;
		char fileName[MAX_OSPATH];
		int length;
		OSErr err;

		memset( &paramBlock, 0, sizeof( paramBlock ) );
		paramBlock.hFileInfo.ioNamePtr = (unsigned char *)fileName;
		paramBlock.hFileInfo.ioVRefNum = VRefNum;
		paramBlock.hFileInfo.ioFDirIndex = index;
		paramBlock.hFileInfo.ioDirID = DrDirId;

		err = PBGetCatInfoSync( &paramBlock );

		if ( err != noErr ) {
			break;
		}
		if ( directoryFlag ^ ( paramBlock.hFileInfo.ioFlAttrib & 16 ) ) {
			continue;
		}

		// convert filename to C string
		length = fileName[0];
		p2cstrcpy( fileName, (const StringPtr) fileName );

		// check the extension
		if ( !directoryFlag ) {
			if ( length < extensionLength ) {
				continue;
			}
			if ( Q_stricmp( fileName + length - extensionLength, extension ) ) {
				continue;
			}
		}

		// add this file
		if ( nfiles == MAX_FOUND_FILES - 1 ) {
			break;
		}
		list[ nfiles ] = CopyString( fileName );
		nfiles++;
	}

	list[ nfiles ] = 0;


	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

#if MAC_Q3_OLDSTUFF
	listCopy = (char **) Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
#else
	listCopy = (char **) Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_LISTFILES, qfalse );
#endif
	for ( i = 0 ; i < nfiles ; i++ )
	{
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void Sys_FreeFileList( char **list ) {
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ )
	{
		Z_Free( list[i] );
	}

	Z_Free( list );
}

void Mac_GetOSPath( short inDomain, OSType inFolderType, char *outPath ) {
	OSStatus err = noErr;
	char temp[PATH_MAX];
	FSRef foundRef;

	if ( !outPath ) {
		return;
	}

	err = FSFindFolder( inDomain, inFolderType, true, &foundRef );
	if ( err ) {
		return;
	}

	err = FSRefMakePath( &foundRef, (UInt8 *)outPath, PATH_MAX );
}

#if MAC_JKJA
qboolean Sys_FileOutOfDate( LPCSTR psFinalFileName /* dest */, LPCSTR psDataFileName /* src */ ) {
	return qfalse;
}

qboolean Sys_CopyFile( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, qboolean bOverwrite ) {
	return qfalse;
}
#endif