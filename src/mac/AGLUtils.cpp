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

#include <math.h>

Q3DEF_BEGIN
	#include "../renderer/tr_local.h"
	#include "../client/client.h"
	#include "mac_local.h"
Q3DEF_END

#define MacWarning Sys_Warning
#define MacFatalError Sys_Error

#ifndef AGL_SAMPLES_ARB
#define AGL_SAMPLE_BUFFERS_ARB    55  /* number of multi sample buffers               */
#define AGL_SAMPLES_ARB           56  /* number of samples per multi sample buffer    */
#endif

#define USE_AGL_FULLSCREEN      0

//=====================================================================
// Includes

#include "AGLUtils.h"
#include "CDrawSprocket.h"

//=====================================================================
// Global Variables

Fixed aglFrequency;                     // Monitor frequency, set in main() from the prefs
AGLContext gAGLContext;                 // The currently active drawing context, either primary or secondary
AGLContext gAGLContext_Primary;         // Primary context, used in non-SMP situations or in the main thread
AGLContext gAGLContext_Secondary;       // Secondary context, used mainly for an SMP thread
AGLPixelFormat gPixelFormat;

//=====================================================================
// Local Variables

static WindowRef sBlankingWindow;       // Used only for AGL_FULLSCREEN cases, to blank out the desktop
static WindowRef sGLWindow;             // In non-AGL_FULLSCREEN cases, contains the GL drawable window
static Rect sGLWindowRect;
static RGBColor sRGBWhite = { 0xFFFF, 0xFFFF, 0xFFFF };
static RGBColor sRGBBlack = { 0x0000, 0x0000, 0x0000 };
static Boolean sAGLActive = false;      // True if a drawable is active
static long sSystemVersion;
static CGrafPtr sCurrentAGLPort;

static DisplayIDType glMonitor;
/*static*/ short glWidth, glHeight;
static int glFrequency;                 // Same as aglFrequency, only in int form

static EventHandlerUPP sWindowEventHandlerUPP;          // carbon window constrain event handler
static EventHandlerRef sEventHandlerRef;                // event handler reference

//=====================================================================
// Implementation

OSStatus MySetWindowContentColor( WindowRef inWindow, RGBColor const *inRGB ) {
	OSStatus status = noErr;

	// LBO - the following routine requires 8.5 in non-Carbon builds (8.1 in Carbon)
	status = SetWindowContentColor( inWindow, inRGB );

	return status;
}

//===============================================================================
// appWindowEventHandler
//
// This event handler is installed by InitCarbonEvents (below) to prevent
// Carbon from moving our windows when they're offscreen. This is most noticable
// when we've set up a blanking window that covers the entire gray region.
//===============================================================================
static pascal OSStatus appWindowEventHandler( EventHandlerCallRef myHandler, EventRef event, void* userData ) {
#pragma unused (myHandler, userData)
	OSStatus result = eventNotHandledErr;
	UInt32 eventKind;

	eventKind = GetEventKind( event );

	switch ( eventKind )
	{
	case kEventWindowConstrain:
	{
		// We handled the event, eat it
		result = noErr;
		break;
	}
	}
	return result;
}

void InstallCarbonWindowEvents( WindowRef inWindow ) {
	OSStatus status;
	EventTypeSpec list[] = { {kEventClassWindow, kEventWindowConstrain } };

	// If we don't support Carbon events, bail
	if ( (Ptr) InstallEventHandler == (Ptr) kUnresolvedCFragSymbolAddress ) {
		return;
	}

	// Install an application event handler
	if ( sWindowEventHandlerUPP == NULL ) {
		sWindowEventHandlerUPP = NewEventHandlerUPP( appWindowEventHandler );
		if ( !sWindowEventHandlerUPP ) {
			SysBeep( 0 ); return;
		}
	}

	status = InstallWindowEventHandler( inWindow, sWindowEventHandlerUPP, 1, list, 0, &sEventHandlerRef );
	if ( status != noErr ) {
		SysBeep( 0 );
	}
}

void DisposeCarbonWindowEvents( void ) {
	if ( sEventHandlerRef == NULL ) {
		return;
	}

	(void)RemoveEventHandler( sEventHandlerRef );
	sEventHandlerRef = NULL;

	if ( sWindowEventHandlerUPP != NULL ) {
		DisposeEventHandlerUPP( sWindowEventHandlerUPP );
	}
	sWindowEventHandlerUPP = NULL;
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// _aglSetGameContext
//
// Given an already chosen AGL context and monitor, this routine will take care of the bs
// involved with choosing a properly-sized window (on non-fullscreen devices) and it
// will activate a fullscreen context if desired for those devices that support that option.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
GLboolean _aglSetGameContext( short inWidth, short inHeight, short inDepth, short inAGLDepth, short inStencilDepth,
							  Fixed inFreq, DisplayIDType inDevice, Boolean inWindow, int inFSAASamples ) {
	GLint result;
	GLboolean ok;
	GDHandle device;

	GLint attrib[] = { AGL_RGBA,
					   AGL_DOUBLEBUFFER,
					   AGL_NO_RECOVERY,
					   AGL_FULLSCREEN,
					   AGL_ACCELERATED,
					   AGL_RED_SIZE, 8,
					   AGL_GREEN_SIZE, 8,
					   AGL_BLUE_SIZE, 8,
					   AGL_ALPHA_SIZE, 0,
					   AGL_DEPTH_SIZE, 16,
					   AGL_STENCIL_SIZE, 0,
#if TEST_SMP
					   AGL_MP_SAFE,
#endif
#if TEST_FSAA
					   AGL_SAMPLES_ARB, 4,
					   AGL_SAMPLE_BUFFERS_ARB, 1,
#endif
					   AGL_NONE };
	short depth, pageCount;
	Fixed frequency;
	OSErr err;
	WindowAttributes winAttrib;
	GLenum glErr;

	// Are we on 10?
	err = Gestalt( gestaltSystemVersion, &sSystemVersion );

#if MAC_DEBUG && SINGLE_BUFFER
	attrib[1] = AGL_NONE;
#endif

	if ( !gDrawSprocket ) {
		return GL_FALSE;
	}
	glFrequency = Fix2X( inFreq );
	glMonitor = inDevice;
	glWidth = inWidth;
	glHeight = inHeight;

	sCurrentAGLPort = NULL;

#if !USE_AGL_FULLSCREEN
	attrib[3] = AGL_ACCELERATED;
#endif

	// AGL_RED/GREEN/BLUE_SIZE
	if ( inDepth <= 16 ) {
		// 15/16 default to 555 RGB
		attrib[6] = attrib[8] = attrib[10] = 5;
	}

	// AGL_DEPTH_SIZE
	if ( inAGLDepth ) {
		attrib[14] = inAGLDepth;
	} else {
		attrib[14] = inDepth;
	}

	// AGL_STENCIL_SIZE
	if ( inStencilDepth ) {
		attrib[16] = inStencilDepth;
	}

	DMGetGDeviceByDisplayID( inDevice, &device, false );
	if ( device == NULL ) {
		device = GetMainDevice();
	}

	if ( gAGLContext ) {
		_aglDisposeGameContext();
	}

#if TEST_SMP
	// There is no AGL_MP_SAFE renderer on MacOS 8/9
	if ( sSystemVersion < 0x01000 ) {
		attrib[17] = AGL_NONE;
	}
#endif

	// If our monitor is set to 8-bit under MacOS 8 or 9, we have to change that for OpenGL to
	// find a valid context.
	if ( sSystemVersion < 0x01000 ) {
		int curDepth = ( **( ( **device ).gdPMap ) ).pixelSize;
		if ( curDepth == 8 ) {
			if ( err = HasDepth( device, 8, 1, 1 ) ) {
				if ( inDepth == 16 ) {
					err = SetDepth( device, 16, 1, 1 );
				} else {
					err = SetDepth( device, 32, 1, 1 );
				}
			}
			if ( err ) {
				MacWarning( "This game requires a color monitor that supports thousands or millions of colors" );
			}
		}
	}

#if TEST_FSAA
	// If < 10.2 or no samples, don't ask for them
	if ( ( sSystemVersion < 0x01020 ) || ( inFSAASamples == 0 ) ) {
		attrib[18] = AGL_NONE;
		attrib[19] = AGL_NONE;
		attrib[20] = AGL_NONE;
		attrib[21] = AGL_NONE;
	} else
	{
		attrib[19] = inFSAASamples;
	}
#endif

#if 1 // !USE_AGL_FULLSCREEN
//	attrib[5] = AGL_ACCELERATED;
	gPixelFormat = aglChoosePixelFormat( &device, 1, attrib );
#else
	if ( sSystemVersion < 0x01000 ) {
		// Under MacOS 8/9, the 3dfx driver is the only card to support agl
		// fullscreen. It's deprecated and buggy, so we must set it up the "wrong" way
		// to get agl fullscreen to work, otherwise the system will freak out and crash
		// badly in the aglChoosePixelFormat call below.
		attrib[3] = AGL_ACCELERATED;
		gPixelFormat = aglChoosePixelFormat( &device, 1, attrib );
	} else
	{
		// Since we're trying for AGL_FULLSCREEN, we must set the first 2 params to 0
		gPixelFormat = aglChoosePixelFormat( NULL, 0, attrib );

		// If it failed, try again without AGL_FULLSCREEN
		if ( gPixelFormat == NULL ) {
			attrib[3] = AGL_ACCELERATED;
			gPixelFormat = aglChoosePixelFormat( &device, 1, attrib );
		}
	}
#endif

	if ( gPixelFormat == NULL ) {
		MacWarning( "aglChoosePixelFormat failed" );
		return GL_FALSE;
	}

	// Create an AGL context
	gAGLContext_Primary = aglCreateContext( gPixelFormat, NULL );
	if ( !gAGLContext_Primary ) {
		MacWarning( "aglCreateContext failed" );
		return GL_FALSE;
	}

#if TEST_SMP
	gAGLContext_Secondary = aglCreateContext( gPixelFormat, gAGLContext_Primary );
	if ( !gAGLContext_Secondary ) {
		MacWarning( "aglCreateContext (alt) failed" );
	}
#endif

	// Make the primary context the current context
	_aglUsePrimaryContext();

	// If we found a fullscreen device, see if the renderer supports it.
	// This is pretty dumb, but on OSX it can be mutually exclusive due to a bug in OpenGL
	if ( attrib[3] == AGL_FULLSCREEN ) {
		AGLRendererInfo info;
		info = aglQueryRendererInfo( &device, 1 );
		if ( !info ) {
			MacWarning( "aglQueryRendererInfo failed" );
			return GL_FALSE;
		}

		// is this a fullscreen device?
		ok = aglDescribeRenderer( info, AGL_FULLSCREEN, &result );

		aglDestroyRendererInfo( info );
	} else {
		result = 0;
	}

	if ( gUseCarbonEvents ) {
		winAttrib = kWindowStandardHandlerAttribute;
	} else {
		winAttrib = kWindowNoAttributes;
	}

	// Burp the agl error handler. We could have encountered errors above and worked around them
	// so we need to toss any errors currently pending.
	glErr = aglGetError();

	// If it is a fullscreen renderer and we want to run fullscreen, set up a fullscreen context.
	// We don't use DrawSprocket for fullscreen contexts (e.g. 3dfx) because
	// a) it's not strictly necessary and b) the aglSetFullScreen call can
	// confuse DrawSprocket and cause the Finder to mangle the desktop icons
	// The downside of this is that there are no gamma fades around the res switch
	// when we're running on 3dfx hardware - yet.
	if ( result && !inWindow ) {
		OSStatus status;
		Rect desktopRect;
		CGrafPtr oldPort;

		// Get a rect that covers the entire desktop
		GetRegionBounds( GetGrayRgn(), &desktopRect );

		// Create a blanking window to hide everything
#if TARGET_API_MAC_CARBON
		{
			status = CreateNewWindow( kDocumentWindowClass, winAttrib, &desktopRect, &sBlankingWindow );
			if ( status ) {
				MacWarning( "CreateNewWindow failed" );
				return GL_FALSE;
			}
			glMonitor = 0;
		}
#else
		sBlankingWindow = NewCWindow( NULL, &desktopRect, "\p", false, noGrowDocProc, ( WindowRef ) - 1, false, 0 );
		if ( !sBlankingWindow ) {
			return GL_FALSE;
		}
#endif
		GetPort( &oldPort );
		// Clear the window to black, then show it
		SetPortWindowPort( sBlankingWindow );
		RGBForeColor( &sRGBBlack );
		RGBBackColor( &sRGBWhite );

		status = MySetWindowContentColor( sBlankingWindow, &sRGBBlack );
		ShowWindow( sBlankingWindow );
		SetPort( oldPort );

		sCurrentAGLPort = ( CGrafPtr ) - 1;
		ok = aglSetFullScreen( gAGLContext, glWidth, glHeight, glFrequency, glMonitor );

		// That tanked, try a different frequency
		if ( !ok ) {
			ok = aglSetFullScreen( gAGLContext, glWidth, glHeight, 60.0, glMonitor );
		}
		if ( !ok ) {
			ok = aglSetFullScreen( gAGLContext, glWidth, glHeight, 75.0, glMonitor );
		}

#if 0
		// If that stuff failed, let's try leaving out the monitor
		if ( !ok ) {
			ok = aglSetFullScreen( gAGLContext, glWidth, glHeight, 60.0, 0 );
		}
		if ( !ok ) {
			ok = aglSetFullScreen( gAGLContext, glWidth, glHeight, 75.0, 0 );
		}
#endif

		if ( !ok ) {
			MacFatalError( "aglSetFullScreen failed. Try specifying a different refresh rate." );
			return GL_FALSE;
		}

#if MAC_Q3
	#if MAC_JKJA
		VID_Printf( PRINT_ALL, "...AGL_FULLSCREEN, %dx%d at %dHz\n", glWidth, glHeight, glFrequency );
	#else
		ri.Printf( PRINT_ALL, "...AGL_FULLSCREEN, %dx%d at %dHz\n", glWidth, glHeight, glFrequency );
	#endif
#endif
		sAGLActive = true;
		sGLWindowRect.top = sGLWindowRect.left = 0;
		sGLWindowRect.right = glWidth;
		sGLWindowRect.bottom = glHeight;
	} else
	{
		short windowType;
		Str32 winTitle;
		OSStatus status;

		if ( !inWindow ) {
			GDHandle newDevice;
			short outWidth, outHeight, outDepth, outPageCount;
			Fixed outFreq;

//			if (!gDrawSprocket->SwitchToResolution (inWidth, inHeight, inDepth, inFreq, 1, true)) return GL_FALSE;
			gDrawSprocket->ConfigureGameContext( inWidth, inHeight, inDepth, inFreq, 1 );
			gDrawSprocket->SwitchToGameContext( true );

			newDevice = gDrawSprocket->GetGDevice();
			if ( newDevice ) {
				sGLWindowRect.top = ( *newDevice )->gdRect.top;
				sGLWindowRect.left = ( *newDevice )->gdRect.left;
			} else
			{
#if MAC_Q3
	#if MAC_JKJA
				Cvar_Set( "r_mode", "3" );
	#else
				ri.Cvar_Set( "r_mode", "3" );
	#endif
				GLimp_Shutdown();
				GLimp_Init();
#endif
			}

			gDrawSprocket->GetGameResolution( &outWidth, &outHeight, &outDepth, &outFreq, &outPageCount );
#if MAC_Q3
	#if MAC_JKJA
			VID_Printf( PRINT_ALL, "...DSp context, %dx%d, %d bit, %d pages\n", outWidth, outHeight, outDepth, outPageCount );
	#else
			ri.Printf( PRINT_ALL, "...DSp context, %dx%d, %d bit, %d pages\n", outWidth, outHeight, outDepth, outPageCount );
	#endif
#endif

			windowType = plainDBox;
		} else
		{
			// Center the window
			sGLWindowRect.left = ( ( ( *device )->gdRect.right - ( *device )->gdRect.left ) - glWidth ) / 2;
			sGLWindowRect.top = ( ( ( *device )->gdRect.bottom - ( *device )->gdRect.top ) - glHeight ) / 2;

			windowType = noGrowDocProc;
		}

		// create a window to draw into

		if ( !inWindow ) {
			DSpContextReference gameContext;

			gameContext = gDrawSprocket->GetCurrentContext();
//			DSpContext_LocalToGlobal (gameContext, (Point *)&sGLWindowRect);
			gDrawSprocket->FadeGammaUp( true );
		} else
		{
			sGLWindowRect.left += ( **device ).gdRect.left;
			sGLWindowRect.top += ( **device ).gdRect.top;
		}
		sGLWindowRect.right = sGLWindowRect.left + glWidth;
		sGLWindowRect.bottom = sGLWindowRect.top + glHeight;

		sprintf( (char *)winTitle, "%s", kGameName );
		c2pstrcpy( winTitle, (const char *)winTitle );

#if TARGET_API_MAC_CARBON
		{
			// On 10, we can attach a drawable directly to a DSp CGrafPtr
			// On < 10, we have to use a blanking window instead.
			if ( inWindow ) {
				CreateNewWindow( kDocumentWindowClass, winAttrib, &sGLWindowRect, &sGLWindow );
			} else if ( sSystemVersion < 0x01000 ) {
//				CreateNewWindow (kPlainWindowClass, winAttrib, &sGLWindowRect, &sGLWindow);
//				CreateNewWindow (kDocumentWindowClass, winAttrib, &sGLWindowRect, &sGLWindow);
				// Always create a window on classic
				sGLWindow = NewCWindow( NULL, &sGLWindowRect, winTitle, 0, windowType, ( WindowRef ) - 1, 0, 0 );
				if ( sGLWindow == NULL ) {
					return GL_FALSE;
				}
//				InstallCarbonWindowEvents (sGLWindow);
			} else {
				sGLWindow = NULL;
			}
		}
#else
		// Always create a window on classic
		sGLWindow = NewCWindow( NULL, &sGLWindowRect, winTitle, 0, windowType, ( WindowRef ) - 1, 0, 0 );
		if ( sGLWindow == NULL ) {
			return GL_FALSE;
		}
#endif

		// Attach the agl context
		if ( sGLWindow ) {
			CGrafPtr oldPort;

			GetPort( &oldPort );
			SetPortWindowPort( sGLWindow );
			RGBForeColor( &sRGBBlack );
			RGBBackColor( &sRGBWhite );
			SetPort( oldPort );

			SetWTitle( sGLWindow, kGameNameP );
//			status = MySetWindowContentColor (sGLWindow, &sRGBBlack);
			ShowWindow( sGLWindow );
			BringToFront( sGLWindow );

			sCurrentAGLPort = GetWindowPort( sGLWindow );
			ok = aglSetDrawable( gAGLContext, sCurrentAGLPort );
#if 0 //TEST_SMP
			ok = aglSetDrawable( gAltAGLContext, sCurrentAGLPort );
#endif
		} else
		{
			// use DSp's front buffer on Mac OS X
			sCurrentAGLPort = gDrawSprocket->GetFrontBuffer();
			if ( sCurrentAGLPort == NULL ) {
//				ReportError ("DSpContext_GetFrontBuffer() had an error.");
				return NULL;
			}

			// there is a problem in Mac OS X GM CoreGraphics that may not size the port pixmap correctly
			// this will check the vertical sizes and offset if required to fix the problem
			// this will not center ports that are smaller then a particular resolution
			{
				long deltaV, deltaH;
				Rect portBounds;
				PixMapHandle hPix = GetPortPixMap( sCurrentAGLPort );
				Rect pixBounds = ( **hPix ).bounds;
				GetPortBounds( sCurrentAGLPort, &portBounds );
				deltaV = ( portBounds.bottom - portBounds.top ) - ( pixBounds.bottom - pixBounds.top ) +
						 ( portBounds.bottom - portBounds.top - glHeight ) / 2;
				deltaH = -( portBounds.right - portBounds.left - glWidth ) / 2;
				if ( deltaV || deltaH ) {
					GrafPtr pPortSave;
					GetPort( &pPortSave );
					SetPort( (GrafPtr)sCurrentAGLPort );
					// set origin to account for CG offset and if requested drawable smaller than screen rez
					SetOrigin( deltaH, deltaV );
					SetPort( pPortSave );
				}
			}

			ok = aglSetDrawable( gAGLContext, sCurrentAGLPort );
#if 0 //TEST_SMP
			ok = aglSetDrawable( gAltAGLContext, sCurrentAGLPort );
#endif
		}
		if ( !ok ) {
			MacWarning( "aglSetDrawable failed" );
			return GL_FALSE;
		}

		sAGLActive = true;
	}

	return GL_TRUE;
}

void _aglDisposeGameContext( void ) {
	if ( gAGLContext_Primary ) {
		aglSetCurrentContext( NULL );
		aglSetDrawable( gAGLContext_Primary, NULL );
		aglDestroyContext( gAGLContext_Primary );
		gAGLContext_Primary = NULL;
	}

#if TEST_SMP
	if ( gAGLContext_Secondary ) {
		aglSetDrawable( gAGLContext_Secondary, NULL );
		aglDestroyContext( gAGLContext_Secondary );
		gAGLContext_Secondary = NULL;
	}
#endif

	gAGLContext = NULL;

	sAGLActive = false;

	if ( sGLWindow ) {
		DisposeWindow( sGLWindow );
		sGLWindow = NULL;
	}
	if ( sBlankingWindow ) {
		DisposeWindow( sBlankingWindow );
		sBlankingWindow = NULL;
	}

	if ( gPixelFormat ) {
		// Pixel format is no longer needed
		aglDestroyPixelFormat( gPixelFormat );
		gPixelFormat = NULL;
	}

}

GLboolean _aglSuspendGameContext( void ) {
	GLboolean result;

	if ( sAGLActive == false ) {
		return GL_TRUE;
	}
	if ( !gAGLContext ) {
		return GL_TRUE;
	}

//	gDrawSprocket->FadeGammaDownImmediate (true);
	result = aglSetDrawable( gAGLContext, NULL );
#if TEST_SMP
//	(void)aglSetDrawable (gAltAGLContext, NULL);
#endif

#if 0
	if ( r_fullscreen->integer ) {
		// If we're using DSp on OSX, we need to pause the context so we can see dialogs
		if ( ( sSystemVersion >= 0x01000 ) && !sGLWindow && !sBlankingWindow ) {
			if ( gDrawSprocket->GetState() == kDSpContextState_Active ) {
				gDrawSprocket->SetState( kDSpContextState_Paused );
			}
		}
		gDrawSprocket->FadeGammaUpImmediate( true );
	}
#endif

#if 0
	// Are we using AGL_FULLSCREEN?
	if ( sBlankingWindow ) {
		// Yes, give some time for the monitor to adjust to the new res.
		UInt32 tickCount;
		Delay( 60, &tickCount );
	}
#endif

	// InitCursor ();
	sAGLActive = false;
	return result;
}

CGrafPtr _aglGetCurrentPort( void ) {
	if ( sGLWindow ) {
		// Running in a window
		return GetWindowPort( sGLWindow );
	} else if ( sBlankingWindow )     {
		// Using aglSetFullscreen
		return ( CGrafPtr ) - 1;
	} else
	{
		// Using DrawSprocket fullscreen
		return gDrawSprocket->GetFrontBuffer();
	}
}

GLboolean _aglResumeGameContext( void ) {
	GLboolean result = GL_FALSE;

	if ( !gAGLContext ) {
		return GL_TRUE;
	}

	if ( r_fullscreen->integer ) {
		/*
		CGDirectDisplayID displayID;
		displayID = (CGDirectDisplayID)sys_gl.displayID;
		if ( displayID == 0 ) {
			displayID = kCGDirectMainDisplay;
		}
		CGDisplayHideCursor( displayID );
		//		HideCursor();
		*/
		gDrawSprocket->FadeGammaDownImmediate( true );
	}

	if ( sGLWindow ) {
		// the blanking window can get shoved offscreen when the movie context
		// becomes active, put it back where it belongs - jb, 3/25/01
		#undef MoveWindow
		MoveWindow( sGLWindow, sGLWindowRect.left, sGLWindowRect.top, false );

		// The blanking window can get moved in priority, bump it to the front
		BringToFront( sGLWindow );

		result = aglSetDrawable( gAGLContext, GetWindowPort( sGLWindow ) );
	} else if ( sBlankingWindow )     {
		// The blanking window can get moved in priority, bump it to the front
		BringToFront( sBlankingWindow );

		// Set the agl context for fullscreen instead of a window
		result = aglSetFullScreen( gAGLContext, glWidth, glHeight, glFrequency, glMonitor );

		// That tanked, try a different frequency
		if ( !result ) {
			result = aglSetFullScreen( gAGLContext, glWidth, glHeight, 60.0, glMonitor );
		}
		if ( !result ) {
			result = aglSetFullScreen( gAGLContext, glWidth, glHeight, 75.0, glMonitor );
		}
	}
#if TARGET_API_MAC_CARBON
	else
	{
#if 0
		// If we're using DSp on OSX, we need to reactivate the context
		if ( ( sSystemVersion >= 0x01000 ) && ( r_fullscreen->integer ) ) {
			if ( gDrawSprocket->GetState() == kDSpContextState_Paused ) {
				gDrawSprocket->SetState( kDSpContextState_Active );
			}
		}
#endif
		{
			CGrafPtr pPort;
			// use DSp's front buffer on Mac OS X
			pPort = gDrawSprocket->GetFrontBuffer();
			if ( pPort == NULL ) {
//				ReportError ("DSpContext_GetFrontBuffer() had an error.");
				return result;
			}

			// there is a problem in Mac OS X GM CoreGraphics that may not size the port pixmap correctly
			// this will check the vertical sizes and offset if required to fix the problem
			// this will not center ports that are smaller then a particular resolution
			{
				long deltaV, deltaH;
				Rect portBounds;
				PixMapHandle hPix = GetPortPixMap( pPort );
				Rect pixBounds = ( **hPix ).bounds;
				GetPortBounds( pPort, &portBounds );
				deltaV = ( portBounds.bottom - portBounds.top ) - ( pixBounds.bottom - pixBounds.top ) +
						 ( portBounds.bottom - portBounds.top - glHeight ) / 2;
				deltaH = -( portBounds.right - portBounds.left - glWidth ) / 2;
				if ( deltaV || deltaH ) {
					GrafPtr pPortSave;
					GetPort( &pPortSave );
					SetPort( (GrafPtr)pPort );
					// set origin to account for CG offset and if requested drawable smaller than screen rez
					SetOrigin( deltaH, deltaV );
					SetPort( pPortSave );
				}
			}

			result = aglSetDrawable( gAGLContext, pPort );
		}
	}
#endif

	if ( r_fullscreen->integer ) {
		gDrawSprocket->FadeGammaUpImmediate( true );
	}

	sAGLActive = true;
	return result;
}

void _aglUsePrimaryContext( void ) {
	GLboolean ok;

	if ( gAGLContext_Primary == NULL ) {
#if MAC_DEBUG && TEST_SMP
		Debugger();
#endif
		return;
	}
	gAGLContext = gAGLContext_Primary;

	ok = aglSetDrawable( gAGLContext_Secondary, NULL );

	ok = aglSetCurrentContext( gAGLContext );
	if ( !ok ) {
		MacWarning( "aglSetCurrentContext failed" );
	}
}

void _aglUseSecondaryContext( void ) {
	GLboolean ok;

	if ( gAGLContext_Secondary == NULL ) {
#if MAC_DEBUG && TEST_SMP
		Debugger();
#endif
		return;
	}
	gAGLContext = gAGLContext_Secondary;

	ok = aglSetDrawable( gAGLContext_Primary, NULL );

	ok = aglSetCurrentContext( gAGLContext );
	if ( !ok ) {
		MacWarning( "aglSetCurrentContext failed" );
	}

	ok = aglSetDrawable( gAGLContext, _aglGetCurrentPort() );
}

void _aglSwapBuffers( void ) {
	if ( sAGLActive ) {
		aglSwapBuffers( gAGLContext );
	} else {
		gDrawSprocket->SwapBuffers();
	}
}

WindowRef _aglGetGLWindow( void ) {
	return sGLWindow;
}

AGLDrawable _aglGetDrawable( void ) {
	return aglGetDrawable( gAGLContext );
}

Boolean _aglUsingFullscreen( void ) {
	if ( sBlankingWindow ) {
		return true;
	} else {
		return false;
	}
}
