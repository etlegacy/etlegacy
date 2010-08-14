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

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// MacPrefs.c
//
// Contains routines to deal with the Preferences file
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Q3DEF_BEGIN
#include "../client/client.h"
#include "mac_local.h"
Q3DEF_END

#include "MacPrefs.h"

//extern OSStatus SaveConfig_HID (void);
//extern OSStatus LoadConfig_HID (void);

PrefsRec macPrefs;

// CreateDefaultPrefs
//
// This routine sets up the default prefs, which are used either if this is the first
// time being run, or for some reason the prefs cannot be loaded. It is called by
// LoadPrefs and is internal to this file.

static void CreateDefaultPrefs( PrefsRec *thePrefs ) {
	thePrefs->button2fake = false;
	thePrefs->monitorFrequency = kMenuRefreshAuto;

	thePrefs->displayID = NULL;
	memset( thePrefs->serial, 0, sizeof( thePrefs->serial ) );
}

static OSErr CreatePrefsFile( FSSpecPtr prefsFile ) {
	OSErr err = noErr;

	FSpCreateResFile( prefsFile, kGameCreator, kPrefsFileType, 0 );
	return ( err = ResError() );
}

static SInt16 OpenPrefsFile( void ) {
	OSErr err = noErr;
	SInt16 prefsRefNum;
	FSSpec prefsFile;

	// Look for the Preferences folder, bail if not found
	if ( ( err = FindFolder( kOnSystemDisk, kPreferencesFolderType, -1,
							 &prefsFile.vRefNum, &prefsFile.parID ) ) != noErr ) {
		return ( -1 );
	}

	// Found it, now try to open our Prefs file
	GetIndString( prefsFile.name, 129, 1 );
	prefsRefNum = FSpOpenResFile( &prefsFile, fsRdWrPerm );

	if ( ( err = ResError() ) != noErr ) {
		// If there was a problem, try to delete the file
		if ( ( err != fnfErr ) && ( ( err = FSpDelete( &prefsFile ) ) != noErr ) ) {
			return( -1 );
		}

		// If deleting worked (or it didn't exist), create a new one and open it
		if ( ( err = CreatePrefsFile( &prefsFile ) ) != noErr ) {
			return( -1 );
		}
		prefsRefNum = FSpOpenResFile( &prefsFile, fsRdWrPerm );
	}
	if ( ResError() == fnfErr ) {
		return( -1 );
	}

	// Specify the prefs file as the current resource file
	UseResFile( prefsRefNum );
	return ( prefsRefNum );
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// LoadPrefs
//
// Loads the prefs from disk into the structure passed into the routine. If the prefs
// cannot be loaded or they are being created from scratch, it sets them to defaults.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void LoadPrefs( PrefsRec *thePrefs ) {
	SInt16 prefsRefNum, saveRefNum;
	Handle rsrc;
	UInt32 type = kPrefsResource;
	UInt16 version = rPrefsVersion;

	//	LoadConfig_HID();

	if ( !thePrefs ) {
		return;
	}

	// Set up the default settings before we try loading anything
	CreateDefaultPrefs( thePrefs );

	// Save the current resource file since we'll be changing it later
	saveRefNum = CurResFile();

	// Try to open our prefs file, bail if something failed
	if ( ( prefsRefNum = OpenPrefsFile() ) == -1 ) {
		return;
	}

	// Try to read in the resource into our handle
	rsrc = (Handle) Get1Resource( type, version );

	// If we got data, copy it to our PrefsRec
	if ( rsrc ) {
		// 1112
		thePrefs->button2fake = ( **( (PrefsRec **) rsrc ) ).button2fake;
		thePrefs->monitorFrequency = ( **( (PrefsRec **) rsrc ) ).monitorFrequency;
		thePrefs->displayID = ( **( (PrefsRec **) rsrc ) ).displayID;

		memcpy( thePrefs->serial, ( **( (PrefsRec **) rsrc ) ).serial, sizeof( thePrefs->serial ) );

		goto bail;
	}

bail:
	if ( rsrc ) {
		ReleaseResource( rsrc );
	}
	CloseResFile( prefsRefNum );
	UseResFile( saveRefNum );
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// SavePrefs
//
// Writes the prefs to disk from the structure passed into the routine. If there is
// a failure, nothing gets written and no warning is given or returned.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void SavePrefs( PrefsRec *thePrefs ) {
	SInt16 prefsRefNum, saveRefNum;
	Handle rsrc;
	UInt32 type = kPrefsResource;
	UInt16 version = rPrefsVersion;

	//SaveConfig_HID();

	if ( !thePrefs ) {
		return;
	}

	// Open the Prefs file
	saveRefNum = CurResFile();
	if ( ( prefsRefNum = OpenPrefsFile() ) == -1 ) {
		return;
	}

	// Get rid of the old prefs resource before writing the new one
	if ( ( rsrc = (Handle) Get1Resource( type, version ) ) != 0 ) {
		RemoveResource( rsrc );  // Only removes it from the rsrc map
		DisposeHandle( rsrc );   // Now we actually free up the memory
	}

	// Create a new handle to hold our prefs data for writing
	rsrc = NewHandle( sizeof( PrefsRec ) );
	if ( rsrc ) {
		// copy the data to our new handle
		( **( (PrefsRec **) rsrc ) ).button2fake = thePrefs->button2fake;
		( **( (PrefsRec **) rsrc ) ).monitorFrequency = thePrefs->monitorFrequency;
		( **( (PrefsRec **) rsrc ) ).displayID = thePrefs->displayID;

		memcpy( ( **( (PrefsRec **) rsrc ) ).serial, thePrefs->serial, sizeof( thePrefs->serial ) );

		// write the handle to the resource file
		AddResource( rsrc, type, version, "\p" );
		if ( ResError() ) {
			DisposeHandle( rsrc );
		} else {
			WriteResource( rsrc );
		}
	}

	CloseResFile( prefsRefNum );
	UseResFile( saveRefNum );
}

void WritePrefsString( const char *name, const char *value ) {
	SInt16 prefsRefNum, saveRefNum;
	Handle rsrc;
	Str255 theName;

	c2pstrcpy( theName, name );

	// Open the Prefs file
	saveRefNum = CurResFile();
	if ( ( prefsRefNum = OpenPrefsFile() ) == -1 ) {
		return;
	}

	// Get rid of the old prefs resource before writing the new one
	if ( ( rsrc = (Handle) Get1NamedResource( kPrefsStringType, theName ) ) != 0 ) {
		RemoveResource( rsrc );  // Only removes it from the rsrc map
		DisposeHandle( rsrc );   // Now we actually free up the memory
	}

	// Create a new handle to hold our prefs data for writing
	rsrc = NewHandleClear( strlen( value ) + 1 );
	HLockHi( rsrc );
	if ( rsrc ) {
		c2pstrcpy( (unsigned char *) *rsrc, value );
		// write the handle to the resource file
		AddResource( rsrc, kPrefsStringType, Unique1ID( kPrefsStringType ), theName );
		if ( ResError() ) {
			DisposeHandle( rsrc );
		} else {
			WriteResource( rsrc );
		}
	}


bail:
	CloseResFile( prefsRefNum );
	UseResFile( saveRefNum );
}

Boolean ReadPrefsString( const char *name, char *value ) {
	Str255 theName;
	StringHandle theString;
	SInt16 prefsRefNum, saveRefNum;
	Boolean retVal = false;

	// default to an empty return value
	value[0] = 0x00;

	c2pstrcpy( theName, name );

	// Open the Prefs file
	saveRefNum = CurResFile();
	if ( ( prefsRefNum = OpenPrefsFile() ) == -1 ) {
		return false;
	}

	theString = (unsigned char **) Get1NamedResource( kPrefsStringType, theName );
	if ( ResError() != noErr ) {
		goto bail;
	} else
	{
		p2cstrcpy( value, *theString );
		retVal = true;
	}

bail:
	CloseResFile( prefsRefNum );
	UseResFile( saveRefNum );
	return retVal;
}

