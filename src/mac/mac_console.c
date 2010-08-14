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

#include "../client/client.h"
#include "mac_local.h"

#if MAC_Q3_MP
	#define NO_CONSOLE_IN_SP    0
#else
// Set this to 1 to keep the console out of single-player games
	#define NO_CONSOLE_IN_SP    1
#endif

typedef struct
{
	WindowRef hWnd;

	TXNObject textObject;
	TXNObject editTextObject;

	char errorString[80];

	char consoleText[512], returnedText[512];
	int visLevel;
	qboolean quitOnClose;
	int windowWidth, windowHeight;
} MacConData;

static MacConData s_wcd;

extern void CompleteCommand( void ) ;

extern unsigned int _ftype, _fcreator;

FILE *sConsoleFile;

#define kConsole_WindowMargin 16
#define kConsole_Width 620
#define kConsole_Height 420

// Our pseudo-object numbers for the MLTE fields in the window
#define kMLTEStatic 1
#define kMLTEEdit   2

#if MAC_MOHAAB
// LBO 3/23/04 - RGB = 226 194 135 (sandy brown, matches game progress bar)
static RGBColor sTextDefaultColor =     { 0xe200, 0xc200, 0x8700 };
#else
static RGBColor sTextDefaultColor =     { 0x8000, 0xffff, 0x8000 };
#endif
static RGBColor sConsoleBkgndColor =    { 0x0000, 0x0000, 0x0000 };

static RGBColor sRGBBlack =     { 0x0000, 0x0000, 0x0000 };
static RGBColor sRGBWhite =     { 0xffff, 0xffff, 0xffff };
static RGBColor sRGBRed =       { 0xffff, 0x0000, 0x0000 };
static RGBColor sRGBGreen =     { 0x0000, 0xffff, 0x0000 };
static RGBColor sRGBBlue =      { 0x0000, 0x0000, 0xffff };
static RGBColor sRGBYellow =    { 0xffff, 0xffff, 0x0000 };
static RGBColor sRGBCyan =      { 0x0000, 0xffff, 0xffff };
static RGBColor sRGBMagenta =   { 0xffff, 0x0000, 0xffff };

// MLTE item that has the current focus
static int sCurrentFocus;
static Boolean sMLTEActive;

OSStatus InitMLTE( void ) {
	TXNMacOSPreferredFontDescription defaults;  // fontID, pointSize, encoding, and fontStyle
	TXNInitOptions options;
	OSStatus status;

	// If MLTE isn't present, we can't set up a Mac console
	if ( TXNVersionInformation == (void*)kUnresolvedCFragSymbolAddress ) {
		return paramErr;
	}

	defaults.fontID = (UInt32)kTXNDefaultFontName;
	defaults.pointSize = kTXNDefaultFontSize;   // Note that this is a Fixed value
	defaults.encoding = CreateTextEncoding( kTextEncodingMacRoman, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat );
	defaults.fontStyle  = kTXNDefaultFontStyle;

	options = 0L; // kTXNWantMoviesMask | kTXNWantSoundMask | kTXNWantGraphicsMask;

	status = TXNInitTextension( &defaults, 1, options );

	return status;
}

void GetMLTEBoundsFromWindow( WindowRef inWindow, Rect *outBounds ) {
	Rect windowRect;

	if ( inWindow == NULL ) {
		return;
	}

	GetWindowPortBounds( inWindow, &windowRect );

	outBounds->top = windowRect.top + 10;
	outBounds->bottom = windowRect.bottom - ( kConsole_WindowMargin + 16 + 12 );
	outBounds->left = windowRect.left + kConsole_WindowMargin;
	outBounds->right = windowRect.right - kConsole_WindowMargin;
}

void GetEditableMLTEBoundsFromWindow( WindowRef inWindow, Rect *outBounds ) {
	Rect windowRect;

	if ( inWindow == NULL ) {
		return;
	}

	GetWindowPortBounds( inWindow, &windowRect );

	outBounds->top = windowRect.bottom - ( kConsole_WindowMargin + 16 );
	outBounds->bottom = windowRect.bottom - kConsole_WindowMargin;
	outBounds->left = windowRect.left + kConsole_WindowMargin;
	outBounds->right = windowRect.right - kConsole_WindowMargin;
}

OSStatus AttachMLTEToWindow( WindowRef inWindow, TXNObject *outObject ) {
	TXNFrameID frameID = 0;
	TXNFrameOptions frameOptions;
	OSStatus status;
	Rect textRect;

	frameOptions = kTXNWantVScrollBarMask |
				   kTXNNoKeyboardSyncMask |
				   kTXNAlwaysWrapAtViewEdgeMask;

	GetMLTEBoundsFromWindow( inWindow, &textRect );

	status = TXNNewObject(  NULL,
							inWindow,
							&textRect,
							frameOptions,
							kTXNTextEditStyleFrameType,
							kTXNTextFile,
							kTXNMacOSEncoding,  // No unicode, please
							&( *outObject ),
							&frameID,
							0 );

	if ( status == noErr ) {
		if ( *outObject != NULL ) {
			TXNControlTag controlTags[] = { kTXNNoUserIOTag };
			TXNControlData controlData[1] = { kTXNReadOnly };

			// sets the state of the scrollbars so they are drwan correctly
			status = TXNActivate( *outObject, frameID, kScrollBarsSyncWithFocus );
			if ( status != noErr ) {
				goto bail;
			}

			// Disable user IO for this MLTE object
			status = TXNSetTXNObjectControls(
				*outObject,
				false,
				sizeof( controlTags ) / sizeof( TXNControlTag ),
				controlTags,
				controlData );
		}
	}

bail:
	return status;
}

void SetMLTECarbonEvents( WindowRef inWindow, TXNObject inTextObject ) {
	// Declare the variables
	OSStatus status;
	TXNControlTag controlTags[] = { kTXNUseCarbonEvents };
	TXNControlData controlData[1];
	TXNCarbonEventInfo carbonEventInfo;
	CFStringRef keys[] =
	{
		kTXNTextHandlerKey,
		kTXNWindowEventHandlerKey,
		kTXNCommandTargetKey,
//		kTXNCommandUpdateKey
	};
	EventTargetRef eventTargets[] =
	{
		GetWindowEventTarget( inWindow ),
		GetWindowEventTarget( inWindow ),
		GetWindowEventTarget( inWindow ),
//		GetWindowEventTarget( inWindow )
	};


	// Initialize the TXNCarbonEventInfo data structure for handling
	// Carbon Text Input events
	carbonEventInfo.useCarbonEvents = true;
	carbonEventInfo.filler = 0;
	carbonEventInfo.flags = kTXNNoAppleEventHandlersMask;

	carbonEventInfo.fDictionary =
		CFDictionaryCreate( kCFAllocatorDefault,
							(const void **) &keys,
							(const void **) &eventTargets,
							sizeof( keys ) / sizeof( CFStringRef ),
							&kCFCopyStringDictionaryKeyCallBacks,
							NULL );

	controlData[0].uValue = ( UInt32 ) & carbonEventInfo;

	// Tell MLTE to install its Carbon handlers
	status = TXNSetTXNObjectControls(
		inTextObject,
		false,
		sizeof( controlTags ) / sizeof( TXNControlTag ),
		controlTags,
		controlData );

	// Release the dictionary
	CFRelease( carbonEventInfo.fDictionary );
}

OSStatus AttachEditableMLTEToWindow( WindowRef inWindow, TXNObject *outObject ) {
	TXNFrameID frameID = 0;
	TXNFrameOptions frameOptions;
	OSStatus status;
	Rect textRect;

	frameOptions = kTXNSingleLineOnlyMask |
				   kTXNNoKeyboardSyncMask |
				   kTXNAlwaysWrapAtViewEdgeMask;

	GetEditableMLTEBoundsFromWindow( inWindow, &textRect );

	status = TXNNewObject(  NULL,
							inWindow,
							&textRect,
							frameOptions,
							kTXNTextEditStyleFrameType,
							kTXNTextFile,
							kTXNMacOSEncoding,  // No unicode, please
							&( *outObject ),
							&frameID,
							0 );

	if ( status == noErr ) {
		if ( *outObject != NULL ) {
			status = TXNActivate( *outObject, frameID, kScrollBarsSyncWithFocus );
			if ( status != noErr ) {
				goto bail;
			}
		}
	}

bail:
	return status;
}

void SetMLTEFontInfo( TXNObject inTextObject, Str255 inFontName, short inFontSize ) {
	TXNTypeAttributes attrib[2];
	short fontID;

	if ( inTextObject == NULL ) {
		return;
	}

	GetFNum( inFontName, &fontID );
	attrib[0].tag = kTXNQDFontFamilyIDAttribute;
	attrib[0].size = kTXNQDFontFamilyIDAttributeSize;
	attrib[0].data.dataValue = fontID;

	attrib[1].tag = kTXNQDFontSizeAttribute;
	attrib[1].size = kTXNFontSizeAttributeSize;
	// Convert from short to Fixed
	attrib[1].data.dataValue = inFontSize << 16;

	(void)TXNSetTypeAttributes( inTextObject, 2, attrib, 0, 0 );
}

void SetMLTEBackgroundColor( TXNObject inTextObject, RGBColor *inColor ) {
	TXNBackground bg;

	bg.bgType = kTXNBackgroundTypeRGB;
	memcpy( &bg.bg.color, inColor, sizeof( RGBColor ) );

	(void)TXNSetBackground( inTextObject, &bg );
}

void SetMLTEFontColor( TXNObject inTextObject, RGBColor *inColor ) {
	TXNTypeAttributes attrib[2];

	if ( inTextObject == NULL ) {
		return;
	}

	attrib[0].tag = kTXNQDFontColorAttribute;
	attrib[0].size = kTXNQDFontColorAttributeSize;
	attrib[0].data.dataPtr = &( *inColor );

	(void)TXNSetTypeAttributes( inTextObject, 2, attrib, 0, 0 );
}

void GetEditableMLTEText( TXNObject inTextObject, char *outText, int inBufferSize ) {
	ByteCount textLength = TXNDataSize( inTextObject ) / 2;
	Handle textHandle;
	OSStatus status;

	// have MLTE make us a copy of all the text
	status = TXNGetDataEncoded(
		inTextObject,           // TXNObject iTXNObject,
		kTXNStartOffset,        // TXNOffset iStartOffset,
		kTXNEndOffset,          // TXNOffset iEndOffset,
		&textHandle,
		kTXNTextData );

	if ( status ) {
		goto bail;
	}

	if ( textLength > inBufferSize ) {
		textLength = inBufferSize;
	}

	HLockHi( textHandle );
	memcpy( outText, *textHandle, textLength );
	// MLTE bug? LBO - When MLTE translates from utxt to TEXT, it sometimes
	// adds a garbage character to the end. We manually truncate it to prevent this.
	outText[textLength] = 0;
	HUnlock( textHandle );
	DisposeHandle( textHandle );

bail:
	return;
}

void SetEditableMLTEText( TXNObject inTextObject, char *inText ) {
	OSStatus status;

	// Blast it with our new text
	status = TXNSetData(
		inTextObject,
		kTXNTextData,
		(void *)inText,
		strlen( inText ),
		kTXNStartOffset,
		kTXNEndOffset );

bail:
	return;
}

void UpdateMLTEFocus( Boolean forceUpdate ) {
	Rect textRect1, textRect2;
	GrafPtr savedPort;
	static Boolean lastActive = true;
	static int lastFocus = -1;
	Boolean willDraw = true;

	// If nothing has changed, skip drawing the focus bar
	if ( ( sMLTEActive == lastActive ) && ( lastFocus == sCurrentFocus ) && !forceUpdate ) {
		willDraw = false;
	}

	GetPort( &savedPort );
	SetPortWindowPort( s_wcd.hWnd );

	TXNGetViewRect( s_wcd.textObject, &textRect1 );
	TXNGetViewRect( s_wcd.editTextObject, &textRect2 );

	// MLTE draws scrollbars differently on 9 and 10. Namely, on 10 it draws in places it should not.
	if ( gSystemVersion >= 0x1000 ) {
		InsetRect( &textRect1, -1, -1 );
	}

	InsetRect( &textRect2, -1, -1 );

	if ( sCurrentFocus == kMLTEStatic ) {
		if ( sMLTEActive ) {
			TXNFocus( s_wcd.textObject, true );
			if ( willDraw ) {
				DrawThemeFocusRect( &textRect1, true );
			}
		} else
		{
			TXNFocus( s_wcd.textObject, false );
			if ( willDraw ) {
				DrawThemeFocusRect( &textRect1, false );
			}
		}

		// Always disable focus for the other text field
		TXNFocus( s_wcd.editTextObject, false );
		if ( willDraw ) {
			DrawThemeFocusRect( &textRect2, false );
		}
	} else
	{
		if ( sMLTEActive ) {
			TXNFocus( s_wcd.editTextObject, true );
			if ( willDraw ) {
				DrawThemeFocusRect( &textRect2, true );
			}
		} else
		{
			TXNFocus( s_wcd.editTextObject, false );
			if ( willDraw ) {
				DrawThemeFocusRect( &textRect2, false );
			}
		}

		// Always disable focus for the other text field
		TXNFocus( s_wcd.textObject, false );
		if ( willDraw ) {
			DrawThemeFocusRect( &textRect1, false );
		}
	}

	SetPort( savedPort );

	// Set these for the next time through
	lastActive = sMLTEActive;
	lastFocus = sCurrentFocus;
}

void UpdateConsoleWindow( void ) {
	ThemeDrawingState themeState;
	Rect portRect, textRect;

	(void)GetWindowPortBounds( s_wcd.hWnd, &portRect );
	GetThemeDrawingState( &themeState );
	SetThemeBackground( kThemeBrushDialogBackgroundActive, 32, true );
	ClipRect( &portRect );
	EraseRect( &portRect );

	TXNDraw( s_wcd.textObject, NULL );

	TXNGetViewRect( s_wcd.textObject, &textRect );
	// MLTE draws scrollbars differently on 9 and 10. Namely, on 10 it draws in places it should not.
	if ( gSystemVersion >= 0x1000 ) {
		InsetRect( &textRect, -1, -1 );
	}
	DrawThemeEditTextFrame( &textRect, sMLTEActive ? kThemeStateActive : kThemeStateInactive );

	TXNDraw( s_wcd.editTextObject, NULL );

	TXNGetViewRect( s_wcd.editTextObject, &textRect );
	InsetRect( &textRect, -1, -1 );
	DrawThemeEditTextFrame( &textRect, sMLTEActive ? kThemeStateActive : kThemeStateInactive );

	UpdateMLTEFocus( true );
}

Boolean ConsoleWindowIsFrontmost( void ) {
#if NO_CONSOLE_IN_SP
	return false;
#else
	if ( s_wcd.hWnd == NULL ) {
		return false;
	}

	if ( FrontWindow() == s_wcd.hWnd ) {
		return true;
	} else {
		return false;
	}
#endif
}

static pascal OSStatus consoleEventHandler( EventHandlerCallRef handlerChain, EventRef event, void *userData ) {
	UInt32 event_kind;
	UInt32 event_class;
	OSStatus err = eventNotHandledErr;
	Rect textRect;

	event_kind = GetEventKind( event );
	event_class = GetEventClass( event );

	if ( event_class == kEventClassKeyboard ) {
		char charCode;

		// If the console window is *not* in front, pass any keyboard events on.
		if ( !ConsoleWindowIsFrontmost() ) {
			return eventNotHandledErr;
		}

		(void)GetEventParameter( event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof( charCode ), NULL, &charCode );

		switch ( event_kind )
		{
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		{
			char inputBuffer[1024];

			// We only handle RETURN and TAB
			if ( charCode == 0x0d ) {
				GetEditableMLTEText( s_wcd.editTextObject, inputBuffer, sizeof( inputBuffer ) );
				strncat( s_wcd.consoleText, inputBuffer, sizeof( s_wcd.consoleText ) - strlen( s_wcd.consoleText ) - 5 );
				strcat( s_wcd.consoleText, "\n" );

				SetEditableMLTEText( s_wcd.editTextObject, "" );

				Sys_Print( va( "]%s\n", inputBuffer ) );

#if MAC_JK2 // ¥¥¥ LBO - add case for non-JK2
				strcpy( kg.g_consoleField.buffer, inputBuffer );
				kg.historyEditLines[kg.nextHistoryLine % COMMAND_HISTORY] = kg.g_consoleField;
				kg.nextHistoryLine++;
				kg.historyLine = kg.nextHistoryLine;
#endif

				err = noErr;
			}
#if MAC_JK2
			else if ( charCode == 0x09 ) {
				GetEditableMLTEText( s_wcd.editTextObject, inputBuffer, sizeof( inputBuffer ) );
				strcpy( kg.g_consoleField.buffer, inputBuffer );
				CompleteCommand();
				SetEditableMLTEText( s_wcd.editTextObject, kg.g_consoleField.buffer );
				err = noErr;
			}
#endif
			break;
		}
		}
	} else if ( event_class == kEventClassWindow )     {
		switch ( event_kind )
		{
		case kEventWindowDrawContent:
			UpdateConsoleWindow();
			err = noErr;
			break;
		case kEventWindowActivated:
			sMLTEActive = true;
			(void)TXNActivate( s_wcd.textObject, 0, kScrollBarsAlwaysActive );
			(void)TXNActivate( s_wcd.editTextObject, 0, kScrollBarsAlwaysActive );
			UpdateMLTEFocus( false );
			err = noErr;
			break;
		case kEventWindowDeactivated:
			sMLTEActive = false;
			(void)TXNActivate( s_wcd.textObject, 0, kScrollBarsAlwaysActive );
			(void)TXNActivate( s_wcd.editTextObject, 0, kScrollBarsAlwaysActive );
			UpdateMLTEFocus( false );
			err = noErr;
			break;
		case kEventWindowResizeCompleted:
			GetWindowPortBounds( s_wcd.hWnd, &textRect );
			InvalWindowRect( s_wcd.hWnd, &textRect );
			GetMLTEBoundsFromWindow( s_wcd.hWnd, &textRect );
			TXNSetFrameBounds( s_wcd.textObject, textRect.top, textRect.left, textRect.bottom, textRect.right, 0 );
			GetEditableMLTEBoundsFromWindow( s_wcd.hWnd, &textRect );
			TXNSetFrameBounds( s_wcd.editTextObject, textRect.top, textRect.left, textRect.bottom, textRect.right, 0 );
			err = noErr;
			break;
		}
	} else if ( event_class == kEventClassMouse )     {
		// If the console window is *not* in front, pass any mouse events on.
		if ( !ConsoleWindowIsFrontmost() ) {
			return eventNotHandledErr;
		}

		if ( event_kind == kEventMouseMoved ) {
			// Track the mouse and adjust to the i-beam cursor if appropriate
			TXNAdjustCursor( s_wcd.editTextObject, NULL );
			err = noErr;
		} else if ( event_kind == kEventMouseDown )     {
			Point mouseLoc;
			Rect textRect;

			// Change which MLTE object has focus based on a mouse down in the window
			(void)GetEventParameter( event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof( mouseLoc ), NULL, &mouseLoc );
			GlobalToLocal( &mouseLoc );

			TXNGetRectBounds( s_wcd.textObject, &textRect, NULL, NULL );
			if ( PtInRect( mouseLoc, &textRect ) ) {
				if ( sCurrentFocus != kMLTEStatic ) {
					sCurrentFocus = kMLTEStatic;
					UpdateMLTEFocus( false );
				}
			} else
			{
				TXNGetRectBounds( s_wcd.editTextObject, &textRect, NULL, NULL );
				if ( PtInRect( mouseLoc, &textRect ) ) {
					if ( sCurrentFocus != kMLTEEdit ) {
						sCurrentFocus = kMLTEEdit;
						UpdateMLTEFocus( false );
					}
				}
			}
		}
	} else if ( event_class == kEventClassCommand )     {
		HICommand cmd;

		// If the console window is *not* in front, pass any mouse events on.
		if ( !ConsoleWindowIsFrontmost() ) {
			return eventNotHandledErr;
		}

		(void)GetEventParameter( event, kEventParamDirectObject, typeHICommand, NULL, sizeof( cmd ), NULL, &cmd );

		switch ( event_kind )
		{
		case kEventCommandProcess:
			if ( ( sCurrentFocus == kMLTEStatic ) && ( cmd.commandID == kHICommandCopy ) ) {
				TXNCopy( s_wcd.textObject );
				err = noErr;
			} else if ( sCurrentFocus == kMLTEEdit )     {
				switch ( cmd.commandID )
				{
				case kHICommandCopy:
					TXNCopy( s_wcd.editTextObject );
					err = noErr;
					break;
				case kHICommandCut:
					TXNCut( s_wcd.editTextObject );
					err = noErr;
					break;
				case kHICommandPaste:
					TXNPaste( s_wcd.editTextObject );
					err = noErr;
					break;
				case kHICommandClear:
					TXNClear( s_wcd.editTextObject );
					err = noErr;
					break;
				}
			}
			break;
		}
	}
	return err;
}
#pragma mark -

/*
==================
Sys_CreateConsole
==================
*/
void    Sys_CreateConsole( void ) {
	Rect windowBounds;
	OSStatus status;
	GDHandle device;
	Str255 windowTitle;

	EventTypeSpec consoleEventList[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassCommand, kEventCommandProcess },
		{ kEventClassWindow, kEventWindowDrawContent },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowResizeCompleted },
	};

	memset( &s_wcd, 0, sizeof( s_wcd ) );

	// Console only available in single-player app
#if NO_CONSOLE_IN_SP
	return;
#else
	if ( InitMLTE() != noErr ) {
		return;
	}

	// Center the window
	device = GetMainDevice();
	windowBounds.left = ( ( ( *device )->gdRect.right - ( *device )->gdRect.left ) - kConsole_Width ) / 2;
	windowBounds.top = ( ( ( *device )->gdRect.bottom - ( *device )->gdRect.top ) - kConsole_Height ) / 2;
	windowBounds.right = windowBounds.left + kConsole_Width;
	windowBounds.bottom = windowBounds.top + kConsole_Height;

	status = CreateNewWindow( kDocumentWindowClass, kWindowResizableAttribute, &windowBounds, &s_wcd.hWnd );
	if ( status != noErr ) {
		return;
	}

	PLstrcpy( windowTitle, kGameNameP );
	PLstrcat( windowTitle, "\p Console" );
	SetWTitle( s_wcd.hWnd, windowTitle );

	InstallStandardEventHandler( GetWindowEventTarget( s_wcd.hWnd ) );
	InstallWindowEventHandler( s_wcd.hWnd, NewEventHandlerUPP( consoleEventHandler ), sizeof( consoleEventList ) / sizeof( EventTypeSpec ), consoleEventList, 0, NULL );

	// Create a default MLTE object and attach it to our window
	status = AttachMLTEToWindow( s_wcd.hWnd, &s_wcd.textObject );
	if ( status != noErr ) {
		return;
	}

	// Set the background color
	SetMLTEBackgroundColor( s_wcd.textObject, &sConsoleBkgndColor );

	// Create a default MLTE object and attach it to our window
	status = AttachEditableMLTEToWindow( s_wcd.hWnd, &s_wcd.editTextObject );
	if ( status != noErr ) {
		return;
	}

	// MLTE bug: LBO - we set all the MLTE object paramters after both have been created
	// because MLTE on 9 will use the parameters for the first one in the creation
	// of the second if we set them individually above.

	// Set the default text font and size for the MLTE object
	SetMLTEFontInfo( s_wcd.textObject, "\pCourier", 14 );

	// Set the font color
	SetMLTEFontColor( s_wcd.textObject, &sTextDefaultColor );

	SetMLTECarbonEvents( s_wcd.hWnd, s_wcd.textObject );

	// Set the font color
	SetMLTEFontColor( s_wcd.editTextObject, &sRGBBlack );

	SetMLTECarbonEvents( s_wcd.hWnd, s_wcd.editTextObject );

	sCurrentFocus = kMLTEEdit;
	sMLTEActive = true;

	Sys_ShowConsole( 1, qfalse );
#endif
}

/*
==================
Sys_ShowConsole
==================
*/
void    Sys_ShowConsole( int visLevel, qboolean quitOnClose ) {
	s_wcd.quitOnClose = quitOnClose;

	if ( visLevel == s_wcd.visLevel ) {
		return;
	}

	s_wcd.visLevel = visLevel;

	if ( !s_wcd.hWnd ) {
		return;
	}

	switch ( visLevel )
	{
	case 0:     // hide
	case 2:     // minimize
		HideWindow( s_wcd.hWnd );
		break;
	case 1:
		ShowWindow( s_wcd.hWnd );
//			SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		break;
	default:
		Sys_Error( "Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel );
		break;
	}
}


/*
================
Sys_Print

This is called for all console output, even if the game is running
full screen and the dedicated console window is hidden.
================
*/
void    Sys_Print( const char *text ) {
	if ( s_wcd.hWnd ) {
		GrafPtr savedPort;
		GrafPtr port = GetWindowPort( s_wcd.hWnd );
		char *curText, *startText;
		char c;
		int runLength;
		TXNOffset startOffset, endOffset;

		GetPort( &savedPort );
		SetPort( port );

		// Save the current selection
		TXNGetSelection( s_wcd.textObject, &startOffset, &endOffset );

		SetMLTEFontColor( s_wcd.textObject, &sTextDefaultColor );
		curText = startText = (char *) text;
		runLength = 0;

		while ( ( c = *curText ) != 0 )
		{
			if ( Q_IsColorString( curText ) ) {
				int color;

				// Flush the existing run of text before switching colors
				if ( runLength ) {
					TXNSetData( s_wcd.textObject, kTXNTextData, startText, runLength, kTXNEndOffset, kTXNEndOffset );
				}

				// Skip past the "^"
				curText++;

				// Set the font color for the text that follows
				color = ColorIndex( *curText++ );

				switch ( color )
				{
#if MAC_MOHAA
// LBO - our console background text is black, so we print black text in the default console text color.
				case 0: SetMLTEFontColor( s_wcd.textObject, &sTextDefaultColor ); break;
#else
				case 0: SetMLTEFontColor( s_wcd.textObject, &sRGBBlack ); break;
#endif
				case 1: SetMLTEFontColor( s_wcd.textObject, &sRGBRed ); break;
				case 2: SetMLTEFontColor( s_wcd.textObject, &sRGBGreen ); break;
				case 3: SetMLTEFontColor( s_wcd.textObject, &sRGBYellow ); break;
				case 4: SetMLTEFontColor( s_wcd.textObject, &sRGBBlue ); break;
				case 5: SetMLTEFontColor( s_wcd.textObject, &sRGBCyan ); break;
				case 6: SetMLTEFontColor( s_wcd.textObject, &sRGBMagenta ); break;
				case 7: SetMLTEFontColor( s_wcd.textObject, &sRGBWhite ); break;
				default: SetMLTEFontColor( s_wcd.textObject, &sTextDefaultColor ); break;
				}

				// new color means new run of text
				runLength = 0;

				// LBO - weirdness. Sometimes there's a 0x19 after a color change byte pair.
				// MLTE draws it as a box, so we just skip it now.
				if ( *curText == 0x19 ) {
					curText++;
				}
				startText = curText;
			} else
			{
				runLength++;
				curText++;
			}
		}

		TXNSetData( s_wcd.textObject, kTXNTextData, startText, runLength, kTXNEndOffset, kTXNEndOffset );

		// Reset the selection
		TXNSetSelection( s_wcd.textObject, startOffset, endOffset );

		if ( s_wcd.visLevel == 1 ) {
			TXNDraw( s_wcd.textObject, NULL );

			// Because we can tweak the selection text programmatically in an
			// unfocused MLTE object, the hilight state of the unfocused selection will suddenly
			// render as if it were active. By resetting the focus after such an action,
			// the hilight state does not change.
			UpdateMLTEFocus( false );

			// We need to flush it on OSX so that updates are reflected quickly
			QDFlushPortBuffer( port, NULL );
		}
		SetPort( savedPort );
	}

#if MAC_DEBUG
	if ( !sConsoleFile ) {
//		cvar_t	*basedir;
		char buffer[1024];

		_fcreator = 'CWIE';
		_ftype = 'TEXT';

//		basedir = Cvar_Get( "fs_basepath", "", 0 );
//		Com_sprintf( buffer, sizeof(buffer), "%s:console.log", basedir->string );
		if ( AmIBundled() ) {
			sConsoleFile = fopen( "../../../console.log", "w" );
		} else {
			sConsoleFile = fopen( "console.log", "w" );
		}
	}
	if ( sConsoleFile ) {
		fprintf( sConsoleFile, text );
		fflush( sConsoleFile );
	}
#endif

	if ( ( com_developer && com_developer->integer ) ||
		 ( com_dedicated && com_dedicated->integer ) ) {
		fprintf( stdout, "%s", text );
	}
}

void Sys_DebugPrint( const char *msg ) {
//¥¥¥	OutputDebugString( msg );
}


/*
==================
Sys_ConsoleEvent
==================
*/
qboolean Sys_ConsoleEvent( EventRecord *event, RgnHandle cursRgn ) {
	qboolean handled = qfalse;
	WindowRef window;
	short part;

	part = FindWindow( event->where, &window );

	if ( window != s_wcd.hWnd ) {
		return qfalse;
	}

//	TXNAdjustCursor (s_wcd.textObject, cursRgn);
	TXNAdjustCursor( s_wcd.editTextObject, cursRgn );

	switch ( event->what )
	{
	case updateEvt:
	{
		GrafPtr savePort;

		BeginUpdate( s_wcd.hWnd );
		GetPort( &savePort );
		SetPortWindowPort( s_wcd.hWnd );
		UpdateConsoleWindow();
		SetPort( savePort );

		EndUpdate( s_wcd.hWnd );
		handled = qtrue;
		break;
	}
	case mouseDown:
	{
		if ( part == inContent ) {
			TXNClick( s_wcd.textObject, event );
			TXNClick( s_wcd.editTextObject, event );
			handled = qtrue;
		}
		break;
	}
	}
	return handled;
}


/*
================
Sys_ConsoleInput

Checks for a complete line of text typed in at the console.
Return NULL if a complete line is not ready.
================
*/
char *Sys_ConsoleInput( void ) {
	if ( s_wcd.consoleText[0] == 0 ) {
		return NULL;
	}

	strcpy( s_wcd.returnedText, s_wcd.consoleText );
	s_wcd.consoleText[0] = 0;

	return s_wcd.returnedText;
}

void Sys_SendStringToConsole( const char *inString ) {
	char *command;
#if MAC_Q3_OLDSTUFF
	command = (char *)Z_Malloc( strlen( inString ) + 1 );
#else
	command = (char *)Z_Malloc( strlen( inString ) + 1, TAG_EVENT, qfalse );
#endif
	if ( !command ) {
		goto bail;
	}

	strcpy( command, inString );

	Sys_QueEvent( 0, SE_CONSOLE, 0, 0, strlen( command ) + 1, (void*) command );
bail:
	return;
}
