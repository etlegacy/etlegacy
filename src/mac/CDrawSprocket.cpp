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

// ======================================================================
// CDrawSprocket.cp
// ======================================================================

// ======================================================================
// includes
// ======================================================================

Q3DEF_BEGIN
#include "../client/client.h"
#include "mac_local.h"
Q3DEF_END

#define MacWarning Sys_Warning
#define MacFatalError Sys_Error
#define MacAssert

#if _DEBUG
#define MAC_DEVELOPMENT 1
#define MAC_DEBUG 1
#endif

#include "CDrawSprocket.h"

#if TARGET_API_MAC_CARBON
#include "PickMonitor/pickmonitor.h"
#endif

// ======================================================================
// globals
// ======================================================================
CDrawSprocket *  gDrawSprocket = NULL;

RGBColor sBlack = { 0x0000, 0x0000, 0x0000 };
//extern CentPrefs macPrefs; // TODO: FindBestContext needs to return DisplayID

// ======================================================================
// functions
// ======================================================================

//------------------------------------------------------------------------------------
//	MyFindBestContextOnDisplayID
//
//	Calls DSpFindBestContextOnDisplayID() if available, otherwise mimics the
//	functionality of that call. - jb, 3/24/01
//------------------------------------------------------------------------------------

static OSStatus MyFindBestContextOnDisplayID( DSpContextAttributesPtr inDesiredAttrib,
											  DSpContextReference *outContext, DisplayIDType inDisplayID ) {
	OSStatus status;
	DSpContextReference context = NULL;
	int bestScore = -1;
	DSpContextReference bestContext = NULL;
	DSpContextAttributes bestAttrib;
	enum {
		kScoreWidthOK = 10,
		kScoreWidthExact = 11,
		kScoreHeightOK = 10,
		kScoreHeightExact = 11,
		kScoreDepthOK = 10,
		kScoreDepthExact = 15,
		kScoreFrequencyMatch = 2
	};

	if ( outContext == NULL || inDesiredAttrib == NULL || inDisplayID == 0 ) {
		return paramErr;
	}

	// On my machine, the real DrawSprocket function picked a wrong resolution for
	// the 800x600 AltGame context on my Voodoo3 monitor and this code didn't, so
	// I disabled this for now... - jb, 3/25/01
#if 1
	// call the real DSp function if possible
	if ( (void *)DSpFindBestContextOnDisplayID != (void *)kUnresolvedCFragSymbolAddress ) {
		return ::DSpFindBestContextOnDisplayID( inDesiredAttrib, outContext, inDisplayID );
	}
#endif

	// iterate over all available contexts and keep track of best one
	status = ::DSpGetFirstContext( inDisplayID, &context );

	while ( status == noErr )
	{
		DSpContextAttributes attrib;

		status = ::DSpContext_GetAttributes( context, &attrib );

		if ( status == noErr ) {
			int score = 0;

			if ( attrib.displayWidth >= inDesiredAttrib->displayWidth ) {
				score = score + ( attrib.displayWidth == inDesiredAttrib->displayWidth ?
								  kScoreWidthExact : kScoreWidthOK );
			}
			if ( attrib.displayHeight >= inDesiredAttrib->displayHeight ) {
				score = score + ( attrib.displayHeight == inDesiredAttrib->displayHeight ?
								  kScoreHeightExact : kScoreHeightOK );
			}

			if ( inDesiredAttrib->pageCount == 1 ) {
				if ( attrib.displayDepthMask & inDesiredAttrib->displayDepthMask ) {
					score = score +
							( ( attrib.displayBestDepth & inDesiredAttrib->displayBestDepth )
							  ? kScoreDepthExact : kScoreDepthOK );
				}
			} else
			{
				if ( ( attrib.backBufferDepthMask & inDesiredAttrib->backBufferDepthMask ) &&
					 ( attrib.displayDepthMask & inDesiredAttrib->displayDepthMask ) ) {
					score = score +
							( ( ( attrib.backBufferBestDepth & inDesiredAttrib->backBufferDepthMask )
								&& ( attrib.displayBestDepth & inDesiredAttrib->displayBestDepth ) )
							  ? kScoreDepthExact : kScoreDepthOK );
				}
			}

			if ( inDesiredAttrib->frequency ) {
				score = score + ( attrib.frequency == inDesiredAttrib->frequency ?
								  kScoreFrequencyMatch : 0 );
			}

#if 0
			if ( inDesiredAttrib->colorNeeds == kDSpColorNeeds_Require &&
				 attrib.colorNeeds != kDSpColorNeeds_Require ) {
				score = -1;
			}
#endif

			if ( score > bestScore ) {
				bestScore = score;
				bestContext = context;
				bestAttrib = attrib;
			}

			status = ::DSpGetNextContext( context, &context );
		}
	}

	// if best context has at least the minimum score, return it
	if ( bestScore >= ( kScoreWidthOK + kScoreHeightOK + kScoreDepthExact ) ) {
		*outContext = bestContext;
		status = noErr;
	}

	return status;
}


#pragma mark -


//------------------------------------------------------------------------------------
//		Startup DrawSprocket
//------------------------------------------------------------------------------------
CDrawSprocket::CDrawSprocket( DisplayIDType inPreferredMonitor ) {
	MacAssert( gDrawSprocket == NULL );       // Only one of these at a time!!!
	gDrawSprocket = this;

	// Zero-out internal data
	mContext = mGameContext = mAltGameContext = mMovieContext = 0;
	memset( &mAttributes, 0, sizeof( mAttributes ) );
	memset( &mGameAttributes, 0, sizeof( mGameAttributes ) );
	memset( &mAltGameAttributes, 0, sizeof( mAltGameAttributes ) );
	memset( &mMovieAttributes, 0, sizeof( mMovieAttributes ) );
	mUseDefaultContext = true;
	mDisplayID = inPreferredMonitor;
	mGammaFadedOut = false;
	mXOffset = 0;
	mYOffset = 0;
	mWRefCon = 0;
	mDoubleBuffered = false;
	mHasQueue = false;
	mPickedMonitor = false;
	mInWindow = false;

	// Startup DrawSprocket
// LBO - FIXME
//	if ((Ptr) ::DSpStartup == (Ptr) kUnresolvedCFragSymbolAddress)
//		ErrorDie(kErrDrawSprocketNotInstalled);

// LBO - FIXME
	DSpStartup();
//	if (::DSpStartup() != noErr)
//		ErrorDie(kErrFatalErrorNotCaught);

	// Find out if we can use the coolio DSp 1.7 fast switching code
	if ( ( Ptr ) ::DSpContext_Queue != (Ptr) kUnresolvedCFragSymbolAddress ) {
		// My Voodoo monitor (secondary) crashes hard when a 32-bit, single-buffered,
		// 1024x768 game context is reserved and activated, followed by a 16-bit,
		// single-buffered 640x480 movie context being queued and switched. The actual
		// crash is in PaintRect() in SwitchToContext(). Not using the queueing fixes
		// the crash. No problem with the Ati Rage 128 monitor. There also was no problem
		// when the game context was 16-bit. I suspect a 3dfx driver bug. I added an
		// option to Centipede to disable fast res switching and turn mHasQueue back off.
		// - jb, 3/27/01
		mHasQueue = true;
	}

#if TARGET_API_MAC_CARBON
	// See what version is cookin'
	if ( (Ptr) kUnresolvedCFragSymbolAddress != (Ptr) DSpGetVersion ) {
		NumVersion versionDSp;
		versionDSp = DSpGetVersion();

		if ( ( versionDSp.majorRev == 0x01 ) && ( versionDSp.minorAndBugRev < 0x99 ) ) {
			UInt32 response;

			if ( ( Gestalt( gestaltSystemVersion, (SInt32 *) &response ) == noErr ) && ( response >= 0x01000 ) ) {
				// this version of DrawSprocket is not completely functional on Mac OS X
				MacWarning( "This version of DrawSprocket is unstable under MacOS X. Proceed with caution." );
			}
		}
	}
#endif

	OSStatus status = noErr;

	if ( inPreferredMonitor == 0 ) {          // If inPreferredMonitor is a -1...
		mUseDefaultContext = true;          // Use the default display
	} else
	{
		mPickedMonitor = true;
	}

	// Check to make sure nothing went wrong
// LBO - FIXME
//	if (status != noErr)
//		ErrorDie(kErrDrawSprocketCantSwitch);
}


//------------------------------------------------------------------------------------
//		Shutdown DrawSprocket
//------------------------------------------------------------------------------------
CDrawSprocket::~CDrawSprocket() {
	OSStatus status;

	// If mHasQueue == false, only one context will ever be reserved, so we
	// needn't worry about all of our special game contexts. If mHasQueue is
	// true, some or all of the special contexts may be queued or reserved. These
	// _must_ be released to prevent crashes and corruption in the Finder after
	// quitting. To be safe (I don't know for sure if it's necessary), try to
	// queue or reserve each one before releasing it. - jb, 3/25/01
	if ( mHasQueue ) {
		// avoid double-releases
		if ( mMovieContext == mContext ) {
			mMovieContext = NULL;
		}
		if ( mGameContext == mContext ) {
			mGameContext = NULL;
		}
		if ( mAltGameContext == mContext ) {
			mAltGameContext = NULL;
		}

		// make sure each context is queued or reserved before releasing it
		if ( mMovieContext ) {
			if ( mContext ) {
				::DSpContext_Queue( mContext, mMovieContext, &mMovieAttributes );
			} else {
				::DSpContext_Reserve( mMovieContext, &mMovieAttributes );
			}
			::DSpContext_Release( mMovieContext );
			mMovieContext = NULL;
		}

		if ( mGameContext ) {
			if ( mContext ) {
				::DSpContext_Queue( mContext, mGameContext, &mGameAttributes );
			} else {
				::DSpContext_Reserve( mGameContext, &mGameAttributes );
			}
			::DSpContext_Release( mGameContext );
			mGameContext = NULL;
		}

		if ( mAltGameContext ) {
			if ( mContext ) {
				::DSpContext_Queue( mContext, mAltGameContext, &mAltGameAttributes );
			} else {
				::DSpContext_Reserve( mAltGameContext, &mAltGameAttributes );
			}
			::DSpContext_Release( mAltGameContext );
			mAltGameContext = NULL;
		}
	}

	// Make sure the current graphics device is set to something sensible.
	SetGDevice( GetMainDevice() );

	// First, release any existing context
	if ( mContext ) {
		GrafPtr curPort;

		GetPort( &curPort );
#if !TARGET_API_MAC_CARBON
		if ( curPort == GetFrontBuffer() || curPort == GetBackBuffer() ) {
			GrafPtr screenPort;
			GetWMgrPort( &screenPort );
			SetPort( screenPort );
		}
#endif
		if ( GetState() != kDSpContextState_Inactive ) {
			status = ::DSpContext_SetState( mContext, kDSpContextState_Inactive );
		}
		FadeGammaUp( true );
		status = ::DSpContext_Release( mContext );
		mContext = NULL;
	}

	// Shutdown sprockets
	::DSpShutdown();

	if ( this == gDrawSprocket ) {    // which is should always be
		gDrawSprocket = NULL;
	}
}


//------------------------------------------------------------------------------------
//		Make sure that when you hit a breakpoint, you don't have a blank screen
//------------------------------------------------------------------------------------
void CDrawSprocket::EnableDebugging( Boolean inDebugMode ) {
	::DSpSetDebugMode( inDebugMode );
}


//------------------------------------------------------------------------------------
//		IsValidResolution indicates whether or not the currently selected
//		display has a valid context available for the desired width, height, and depth.
//		Frequency info is optional.  When we iterate through the available contexts,
//		we assume that DrawSprocket will return them in order.  Since this should be
//		the case, we can look for the first context whose width & height is EQUAL to
//		or GREATER than the width and height we're requesting.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::IsValidResolution( short inWidth, short inHeight, short inDepth, Fixed inFrequency ) {
	// Grab the first context for the selected display
	DSpContextReference context;
	OSStatus status = ::DSpGetFirstContext( mDisplayID, &context );

	// Iterate through each successive context looking for the first one which matches
	// our search criteria the best
	while ( status == noErr )
	{
		DSpContextAttributes theAttributes;
		status = ::DSpContext_GetAttributes( context, &theAttributes );

		// Check to see how close this context is to what we want
		if ( status == noErr                             &&
			 theAttributes.displayWidth >= inWidth       &&
			 theAttributes.displayHeight >= inHeight     &&
			 theAttributes.displayBestDepth == inDepth ) {
			// Check frequency requirement (if there is one) before returning
			if ( inFrequency ) {
				if ( theAttributes.frequency == inFrequency ) {
					return true;
				}
			} else {
				return true;
			}
		}

		status = ::DSpGetNextContext( context, &context );
	}

	// Resolution not found
	return false;
}


//------------------------------------------------------------------------------------
//
//      Returns the context that best matches the specified attributes. Restricts
//		the context to the monitor specified by mDisplayID, unless the user is unable
//		or unwilling to choose a monitor.
//
//		Returns in outAttributes attributes that describe the requested attributes.
//
//		Returns true for success.
//
//		Rewritten - jb, 3/24/01
//
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FindBestContext( short inWidth, short inHeight, short inDepth,
										Fixed inFrequency, short inPageCount, DSpContextReference *outContext,
										DSpContextAttributes *outAttributes ) {
	DSpContextAttributes desiredAttrib;
	DSpContextReference context = NULL;
	OSStatus status;

	MacAssert( inPageCount == 1 || inPageCount == 2 );
	MacAssert( outContext && outAttributes );
	if ( outContext == NULL || outAttributes == NULL ) {
		return false;
	}

	memset( &desiredAttrib, 0, sizeof( desiredAttrib ) );
	desiredAttrib.pageCount = inPageCount;
	desiredAttrib.displayWidth = inWidth;
	desiredAttrib.displayHeight = inHeight;
	desiredAttrib.backBufferBestDepth = inDepth;
	desiredAttrib.displayBestDepth = inDepth;
	desiredAttrib.backBufferDepthMask = inDepth;
	desiredAttrib.displayDepthMask = inDepth;
	desiredAttrib.colorNeeds = kDSpColorNeeds_Require;
	desiredAttrib.frequency = inFrequency;

	// we want a display ID -- try to get one from the user if we haven't already
	if ( !mDisplayID ) {
		mPickedMonitor = false;
	}

	if ( !mPickedMonitor ) {
#if TARGET_API_MAC_CARBON
		if ( CanUserSelectMonitor() ) {
			status = PickMonitorDialog( &mDisplayID );
		}

		if ( status == noErr && mDisplayID != 0 ) {
//			macPrefs.displayID = mDisplayID;
			mPickedMonitor = true;
		}
#else
		Boolean result;

		status = ::DSpCanUserSelectContext( &desiredAttrib, &result );
		if ( status == noErr && result == true ) {
			DSpContextReference tempContext;
#if MAC_CENT
			ChangeMouseState( false );
#endif
			status = ::DSpUserSelectContext( &desiredAttrib, 0L, NULL, &tempContext );
#if MAC_CENT
			ChangeMouseState( true );
#endif
			if ( status == noErr ) {
				status = ::DSpContext_GetDisplayID( tempContext, &mDisplayID );
			}

			if ( status == noErr && mDisplayID != 0 ) {
//				macPrefs.displayID = mDisplayID;
				mPickedMonitor = true;
			}
		}
#endif
	}

	if ( mPickedMonitor && mDisplayID ) {
		// Try to find a valid context on the monitor we've chosen
		status = MyFindBestContextOnDisplayID( &desiredAttrib, &context, mDisplayID );
//		if (status != noErr)
//			context = NULL;
	}

	// if we couldn't get a context for the specified display or no display was specifed,
	// try to get one for any display
	if ( context == NULL ) {
		status = ::DSpFindBestContext( &desiredAttrib, &context );
//		if (status != noErr)
//			context = NULL;
//		else
		{
			status = ::DSpContext_GetDisplayID( context, &mDisplayID );
			if ( status == noErr ) {
				mPickedMonitor = true;
			}
		}
	}

	// found a suitable context
	if ( context ) {
		*outContext = context;
		*outAttributes = desiredAttrib;
		return true;
	}

	// Resolution not found
	return false;
}


//------------------------------------------------------------------------------------
//		Returns info about the first Draw Sprocket context.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FindFirstContext( DSpContextReference *outContext, DSpContextAttributes *outAttributes ) {
	OSStatus status;

	// Grab the first context for this display
	status = ::DSpGetFirstContext( mDisplayID, outContext );
	if ( status == noErr ) {
		status = ::DSpContext_GetAttributes( *outContext, outAttributes );
	}

	return status == noErr;
}

//------------------------------------------------------------------------------------
//		Returns info about an additional draw spocket context.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FindNextContext( DSpContextReference *ioContext, DSpContextAttributes *outAttributes ) {
	OSStatus status;

	// Grab the first context for this display
	status = ::DSpGetNextContext( *ioContext, ioContext );
	if ( status == noErr ) {
		status = ::DSpContext_GetAttributes( *ioContext, outAttributes );
	}

	return status == noErr;
}

//------------------------------------------------------------------------------------
// ¥ ConfigureMovieContext
//------------------------------------------------------------------------------------
// Sets up a DrawSprocket context, must be called before any DrawSprocket context
// has been made active. If we're using DSp 1.7 or later, this will also queue the
// context so that we can switch to it quickly.
//
// If this is called and a movie context has already been set, it returns false and
// does nothing further.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::ConfigureMovieContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount ) {
	// Toss the game context if one exists.
	if ( mMovieContext ) {
		if ( mHasQueue ) {
			if ( mContext ) {
				::DSpContext_Queue( mContext, mMovieContext, &mMovieAttributes );
			} else {
				::DSpContext_Reserve( mGameContext, &mMovieAttributes );
			}
			::DSpContext_Release( mMovieContext );
		}
		mMovieContext = NULL;
	}

	// Find the desired context
	DSpContextAttributes desiredAttributes;
	DSpContextReference newContext;

	if ( !FindBestContext( inWidth, inHeight, inDepth, inFrequency, inPageCount, &newContext, &desiredAttributes ) ) {
		// For some reason, we've failed miserably.  Gotta punt...
//		FadeGammaUp(true);
//		char text[64];
//		sprintf(text, "[%dx%d, %d bit, %d buffer(s)]", mMovieWidth, mMovieHeight, mMovieDepth, inPageCount);
		return false;
	} else
	{
		// Set this flag so that updates aren't synched with the vertical retrace
		desiredAttributes.contextOptions |= kDSpContextOption_DontSyncVBL;

		// We've found a new context to switch to.  Save off the desiredAttributes
		mMovieAttributes = desiredAttributes;
		mMovieContext = newContext;
	}
	return true;
}

//------------------------------------------------------------------------------------
// ¥ ConfigureGameContext
//------------------------------------------------------------------------------------
// Sets up a DrawSprocket context, must be called before any DrawSprocket context
// has been made active. If we're using DSp 1.7 or later, this will also queue the
// context so that we can switch to it quickly.
//
// If this is called and a game context has already been set, it returns false and
// does nothing further.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::ConfigureGameContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount ) {
	// Toss the game context if one exists.
	if ( mGameContext ) {
		if ( mGameContext == mContext ) {
			if ( GetState() != kDSpContextState_Inactive ) {
				::DSpContext_SetState( mContext, kDSpContextState_Inactive );
			}
			mContext = NULL;
		}

#if 0
		if ( mHasQueue ) {
			if ( !mContext ) {
				::DSpContext_Reserve( mGameContext, &mGameAttributes );
			} else if ( mContext != mGameContext ) {
				::DSpContext_Queue( mContext, mGameContext, &mGameAttributes );
			}
			::DSpContext_Release( mGameContext );
		}
#else
		::DSpContext_Release( mGameContext );
#endif
		mGameContext = NULL;
	}

	if ( !mGameContext ) {
		// Find the desired context
		DSpContextAttributes desiredAttributes;
		DSpContextReference newContext;

		if ( !FindBestContext( inWidth, inHeight, inDepth, inFrequency, inPageCount, &newContext, &desiredAttributes ) ) {
			// For some reason, we've failed miserably.  Gotta punt...
//			FadeGammaUp(true);
//			char text[64];
//			sprintf(text, "[%dx%d, %d bit, %d buffer(s)]", mMovieWidth, mMovieHeight, mMovieDepth, inPageCount);
			return false;
		} else
		{
			// Set this flag so that updates aren't synched with the vertical retrace
			desiredAttributes.contextOptions |= kDSpContextOption_DontSyncVBL;

			// We've found a new context to switch to.  Save off the desiredAttributes
			mGameAttributes = desiredAttributes;
			mGameContext = newContext;
		}
	}

	return true;
}

//------------------------------------------------------------------------------------
// ¥ ConfigureAltGameContext
//------------------------------------------------------------------------------------
// Sets up a DrawSprocket context, must be called before any DrawSprocket context
// has been made active. If we're using DSp 1.7 or later, this will also queue the
// context so that we can switch to it quickly.
//
// If this is called and a game context has already been set, it returns false and
// does nothing further.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::ConfigureAltGameContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount ) {
	// Toss the alt game context if one exists.
	if ( mAltGameContext ) {
		if ( mHasQueue ) {
			if ( mContext ) {
				::DSpContext_Queue( mContext, mAltGameContext, &mAltGameAttributes );
			} else {
				::DSpContext_Reserve( mAltGameContext, &mAltGameAttributes );
			}
			::DSpContext_Release( mAltGameContext );
		}
		mAltGameContext = NULL;
	}

	if ( !mAltGameContext ) {
		// Find the desired context
		DSpContextAttributes desiredAttributes;
		DSpContextReference newContext;

		if ( !FindBestContext( inWidth, inHeight, inDepth, inFrequency, inPageCount, &newContext, &desiredAttributes ) ) {
			// For some reason, we've failed miserably.  Gotta punt...
//			FadeGammaUp(true);
//			char text[64];
//			sprintf(text, "[%dx%d, %d bit, %d buffer(s)]", mMovieWidth, mMovieHeight, mMovieDepth, inPageCount);
			return false;
		} else
		{
			// Set this flag so that updates aren't synched with the vertical retrace
			desiredAttributes.contextOptions |= kDSpContextOption_DontSyncVBL;

			// We've found a new context to switch to.  Save off the desiredAttributes
			mAltGameAttributes = desiredAttributes;
			mAltGameContext = newContext;
		}
	} else {
		return false;
	}

	return true;
}


//------------------------------------------------------------------------------------
// ¥ SwitchToContext
//------------------------------------------------------------------------------------
//
//	Switch to the specified context, using Queue/Switch if available. - jb, 3/24/01
//
//	Some things about DrawSprocket that are apparently undocumented but seem to be true:
//
//	- Each time you call DSpFindBestContextOnDisplayID(), DSpGetFirstContext(),
//		DSpGetNextContext(), or DSpFindBestContext(), you get a different context
//		reference, i.e. if you call DSpGetFirstContext() three times in a row you
//		get three different references. (Is it leaking memory each time? I wouldn't
//		doubt it.) If you ever call DSpContext_Release() on a context reference, it is
//		invalid forever after that and can't be reserved again.
//
//	- Only one context can ever be reserved at a time. Would have been nice if that
//		was mentioned in the docs.
//
//	- There must be a reserved context when you call the gamma fade functions or
//		your Mac will crash hard.
//
//	- If you have queued or reserved any contexts, you must release them before
//		exiting your application to prevent hard crashes and/or corruption on quit.
//
//	- If you intend to open dialog boxes or the like while a context is active, you
//		should ensure that your application reserves and activates its largest
//		context before any other context. Otherwise the visible region can be too
//		small and interfere with the drawing of the dialog box.
//
//	- If you reserve a context but don't activate it, and then activate a different
//		context, you can be pretty sure desktop icons will get moved all over the
//		place.
//
//	YMMV. - jb, 3/25/01
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SwitchToContext( DSpContextReference inContext,
										DSpContextAttributes *inAttrib, Boolean inDoFade ) {
	DSpContextState curState;
	OSStatus status = noErr;

	MacAssert( inContext );
	if ( !inContext ) {
		return false;
	}

	// are we already in the specified context?
	if ( mContext == inContext ) {
		if ( GetState() != kDSpContextState_Active ) {
			SetState( kDSpContextState_Active );
		}
		CGrafPtr port = GetDrawingBuffer();
		MacAssert( port );
		SetPort( port );
		return true;
	}

	// kill any active context (or queue/switch if available)
	if ( mContext ) {
		// Fade to black before switching, if requested.
		if ( inDoFade ) {
#if MAC_DEVELOPMENT || MAC_DEBUG
			FadeGammaDown( false );       // Don't fade all monitors if we're debugging
#else
			FadeGammaDown( true );
#endif
		}

#if 1
		// Resetting the current graphics device prevents a problem in Centipede
		// after the sequence mMovieContext->mGameContext->aglSuspendGameContext()->
		// mAltGameContext->mGameContext->aglResumeGameContext() when my non-main
		// Voodoo3 monitor is selected. It also prevents a problem with the InputSprocket
		// configure dialog appearing partially offscreen on the 3dfx monitor.
		// The reason is probably complicated... - jb, 3/25/01
		SetGDevice( GetMainDevice() );
#endif

		if ( mHasQueue ) {
			// Queue the new context
			status = ::DSpContext_Queue( mContext, inContext, inAttrib );
			if ( status == kDSpConfirmSwitchWarning ) {
				status = noErr;
			}
			if ( status != noErr ) {
				goto bail;
			}

			// Switch to the queued context
			status = ::DSpContext_Switch( mContext, inContext );
			if ( status == kDSpConfirmSwitchWarning ) {
				status = noErr;
			}
			if ( status != noErr ) {
				goto bail;
			}

			mContext = inContext;
			mAttributes = *inAttrib;
		} else
		{
			if ( GetState() != kDSpContextState_Inactive ) {
				SetState( kDSpContextState_Inactive );
			}
			status = ::DSpContext_Release( mContext );

			// see if we killed any special-case contexts
			DSpContextReference *killedContext = NULL;
			DSpContextAttributes *killedAttrib = NULL;

			if ( mContext == mMovieContext ) {
				killedContext = &mMovieContext;
				killedAttrib = &mMovieAttributes;
			} else if ( mContext == mGameContext )     {
				killedContext = &mGameContext;
				killedAttrib = &mGameAttributes;
			} else if ( mContext == mAltGameContext )     {
				killedContext = &mAltGameContext;
				killedAttrib = &mAltGameAttributes;
			}
			mContext = NULL;
			if ( killedContext ) {
				// regenerate killed context while no contexts are active
				*killedContext = NULL;
				DSpContextAttributes dontCareAttrib;

				Boolean result = FindBestContext( killedAttrib->displayWidth,
												  killedAttrib->displayHeight, killedAttrib->displayBestDepth,
												  killedAttrib->frequency, killedAttrib->pageCount, killedContext,
												  &dontCareAttrib );
			}
		}
	}

	if ( mContext != inContext ) {
		// reserve specified context
		status = ::DSpContext_Reserve( inContext, inAttrib );
		if ( status == kDSpConfirmSwitchWarning ) {
			status = noErr;
		}
		if ( status == noErr ) {
			mContext = inContext;
			mAttributes = *inAttrib;
		}

		// activate specified context
		if ( mContext ) {
			// Fade to black before switching, if requested.
			if ( inDoFade ) {
#if MAC_DEVELOPMENT || MAC_DEBUG
				FadeGammaDown( false );       // Don't fade all monitors if we're debugging
#else
				FadeGammaDown( true );
#endif
			}
			if ( GetState() != kDSpContextState_Active ) {
				SetState( kDSpContextState_Active );
			}
		}
	}

	// bail if we failed
	if ( mContext != inContext ) {
		goto bail;
	}

	// Record whether or not our new context is double bufferred
	if ( mAttributes.pageCount == 2 ) {
		mDoubleBuffered = true;
	} else {
		mDoubleBuffered = false;
	}

	{ // TTimo - variables and labels playing nasty
	 // Immediately set the port to that of the DSp drawing buffer - phs, 10/24/99
	 //Ê(I'm pretty sure that not only is this needed, but it's correct.  We want to
	 // set the port to the one that we're going to draw to) - phs, 11/8/99
		CGrafPtr port = GetDrawingBuffer();
		MacAssert( port );
		SetPort( port );
		Rect portRect;

#if 1
		// Paranoia compels me to set the right device before drawing to the newly-
		// activated context. - jb, 3/27/01
		GDHandle savedDevice = GetGDevice();
		DisplayIDType displayID;
		GDHandle device = NULL;

		status = ::DSpContext_GetDisplayID( mContext, &displayID );
		if ( status == noErr ) {
			DMGetGDeviceByDisplayID( displayID, &device, false );
		}
		if ( device ) {
			SetGDevice( device );
		}
#endif

		// LBO - without the RGBForeColor call, ATi cards will clear the color
		// to white! Using qd.white in that case will produce the desired
		// effect, but break on Voodoo3 cards.
		RGBForeColor( &sBlack );
		PaintRect( GetPortBounds( port, &portRect ) );
		SwapBuffers();

#if 1 // restore device - jb, 3/27/01
		SetGDevice( savedDevice );
#endif
	}

	// Figure out if we need to keep track of any internal offsets
	mXOffset = 0;
	mYOffset = 0;

	DSpContextAttributes tempAttrib;

	if ( noErr == ::DSpContext_GetAttributes( mContext, &tempAttrib ) ) {
		mXOffset = ( tempAttrib.displayWidth - mAttributes.displayWidth ) / 2;
		mYOffset = ( tempAttrib.displayHeight - mAttributes.displayHeight ) / 2;

		MacAssert( mXOffset >= 0 );
		MacAssert( mYOffset >= 0 );

		if ( mXOffset || mYOffset ) {
			// Change the size of our port to exactly what we want
			PortSize( mAttributes.displayWidth, mAttributes.displayHeight );

			// Center the port on the display
			MovePortTo( mXOffset, mYOffset );
		}
	}

	// Fade back in
	if ( inDoFade ) {
#if MAC_DEVELOPMENT || MAC_DEBUG
		FadeGammaUp( false );     // Don't fade all monitors if we're debugging
#else
		FadeGammaUp( true );
#endif
	}

	return true;

bail:

	// Fade back in
	if ( inDoFade && mContext ) {
#if MAC_DEVELOPMENT || MAC_DEBUG
		FadeGammaUp( false );     // Don't fade all monitors if we're debugging
#else
		FadeGammaUp( true );
#endif
	}
	SysBeep( 0 );
	return false;
}


//------------------------------------------------------------------------------------
// ¥ SwitchToMovieContext
//------------------------------------------------------------------------------------
// Switches to the preconfigured movie context. If we're already there, does nothing.
// If the movie context is NULL, returns failure. This code assumes that we're already
// looking at the game context, and that the movie context has already been reserved.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SwitchToMovieContext( Boolean inDoFade ) {
	// No movie context? punt
	if ( mMovieContext == NULL ) {
		goto bail;
	}

	return SwitchToContext( mMovieContext, &mMovieAttributes, inDoFade );

bail:
	SysBeep( 0 );
	return false;
}

//------------------------------------------------------------------------------------
// ¥ SwitchToGameContext
//------------------------------------------------------------------------------------
// Switches to the preconfigured game context. If we're already there, does nothing.
// If the game context is NULL, returns failure. The game context must already be
// reserved.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SwitchToGameContext( Boolean inDoFade ) {
	// No game context? punt
	if ( mGameContext == NULL ) {
		goto bail;
	}

	return SwitchToContext( mGameContext, &mGameAttributes, inDoFade );

bail:
	SysBeep( 0 );
	return false;
}

//------------------------------------------------------------------------------------
// ¥ SwitchToAltGameContext
//------------------------------------------------------------------------------------
// Switches to the preconfigured alt game context. If we're already there, does nothing.
// If the game context is NULL, returns failure. The alt game context must already be
// reserved. Assumes that the regular game context is also reserved. movie context is
// optional.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SwitchToAltGameContext( Boolean inDoFade ) {
	// No alt game context? punt
	if ( mAltGameContext == NULL ) {
		goto bail;
	}

	return SwitchToContext( mAltGameContext, &mAltGameAttributes, inDoFade );

bail:
	SysBeep( 0 );
	return false;
}

//------------------------------------------------------------------------------------
//		DrawSprocket resolution switching routine.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SwitchToResolution( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount, Boolean inDoFade ) {
//	MacWarning("CDrawSprocket::SwitchToResolution not implemented");
//	return false;

	// If we've previously switched resolutions, we should have saved off our context attributes.  Check the
	// parameters sent in against what's been saved to see if we've already got the correct context
	if ( mContext                                &&
		 mAttributes.displayWidth == inWidth     &&
		 mAttributes.displayHeight == inHeight   &&
		 mAttributes.displayBestDepth == inDepth &&
		 mAttributes.frequency == inFrequency    &&
		 mAttributes.pageCount == inPageCount ) {
		return true;
	}

	// First, release any existing context.  We can't get ANY context info from a display with an
	// active context (this is a stupid limitation of DrawSprocket)
	OSStatus status;
	if ( mContext ) {
		status = ::DSpContext_SetState( mContext, kDSpContextState_Inactive );
		if ( status == kDSpConfirmSwitchWarning ) {
			status = noErr;
		}

		if ( status == noErr ) {
			status = ::DSpContext_Release( mContext );
			mContext = NULL;
		}
	}

	MacAssert( mContext == NULL );

	// Find the desired context
	DSpContextAttributes desiredAttributes;
	DSpContextReference newContext;
	if ( !FindBestContext( inWidth, inHeight, inDepth, inFrequency, inPageCount, &newContext, &desiredAttributes ) ) {
		// For some reason, we've failed miserably.  Gotta punt...
		FadeGammaUp( true );
		char text[64];
		sprintf( text, "[%dx%d, %d bit, %d buffer(s)]", inWidth, inHeight, inDepth, inPageCount );
// LBO - FIXME
//		ErrorDie(kErrDrawSprocketCantSwitch, text);
	} else
	{
		// Set this flag so that updates aren't synched with the vertical retrace
		desiredAttributes.contextOptions |= kDSpContextOption_DontSyncVBL;

		// We've found a new context to switch to.  Save off the desiredAttributes
		mAttributes = desiredAttributes;

		// Reserve the found context
		mContext = newContext;
		status = ::DSpContext_Reserve( mContext, &mAttributes );

		// Fade to black before switching, if requested.
		if ( inDoFade ) {
#if MAC_DEVELOPMENT
			FadeGammaDown( false );       // Don't fade all monitors if we're debugging
#else
			FadeGammaDown( true );
#endif
		}

		// Switch to new resolution
		status = ::DSpContext_SetState( mContext, kDSpContextState_Active );

		if ( status == kDSpConfirmSwitchWarning ) {
			// Get this error if sprockets thinks it had trouble making the switch.
			// You're supposed to confirm the switch with the user by displaying
			// an alert. We're ignoring this because many times you get this error
			// even when the switch is to a valid resolution. Evidentally, the
			// Display Manager is over zealous about wanting to warn the user of
			// potential problems.
			status = noErr;
		}

		// Figure out if we need to keep track of any internal offsets
		mXOffset = 0;
		mYOffset = 0;

		if ( status == noErr ) {
#if MAC_DEVELOPMENT
/*
			// Recheck the attributes. MAK 06/09/00
			::DSpContext_GetAttributes(mContext, &desiredAttributes);
			if (desiredAttributes.pageCount == 3)
				desiredAttributes.pageCount = 2;
*/
#endif
			// Record whether or not our new context is double bufferred
			if ( desiredAttributes.pageCount == 2 ) {
				mDoubleBuffered = true;
			} else {
				mDoubleBuffered = false;
			}

			// Immediately set the port to that of the DSp drawing buffer - phs, 10/24/99
			//Ê(I'm pretty sure that not only is this needed, but it's correct.  We want to
			// set the port to the one that we're going to draw to) - phs, 11/8/99
//			SetDrawingPort ();
			CGrafPtr port = GetDrawingBuffer();
			MacAssert( port );
			SetPort( port );
		}

		if ( ( status == noErr ) && ( !mUseDefaultContext ) ) {
			// If we didn't call DSpFindBestContext, and instead iterated through all available contexts to find
			// one that was appropriate, we may have found a context whose width or height is too big.  Therefore,
			// we need to compensate by calling PortSize() & MovePortTo()
			CGrafPtr port = GetDrawingBuffer();
			if ( port ) {
				mXOffset = ( mAttributes.displayWidth - inWidth ) / 2;
				mYOffset = ( mAttributes.displayHeight - inHeight ) / 2;

				MacAssert( mXOffset >= 0 );
				MacAssert( mYOffset >= 0 );

				if ( mXOffset || mYOffset ) {
//					SetPort((GrafPtr) port);
//						SetOrigin(-mXOffset, -mYOffset);

					// Change the size of our port to exactly what we want
					PortSize( inWidth, inHeight );

					// Center the port on the display
					MovePortTo( mXOffset, mYOffset );
//						mXOffset = 0;
//						mYOffset = 0;
				}
			}
		}

#if MAC_DEVELOPMENT
		// Make sure we didn't get any errors besides kDSpConfirmSwitchWarning - phs, 11/9/99
		MacAssert( status == noErr );
#endif
		return ( status == noErr );
	}

	return false;
}


//------------------------------------------------------------------------------------
//		Return some info about the currently active context
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::GetResolution( short *outWidth, short *outHeight, short *outDepth, Fixed *outFrequency, short *outPageCount ) {
	if ( mContext ) {
		DSpContextAttributes attributes;
		if ( ::DSpContext_GetAttributes( mContext, &attributes ) == noErr ) {
			*outWidth = attributes.displayWidth;
			*outHeight = attributes.displayHeight;
			*outDepth = attributes.displayBestDepth;
			*outFrequency = attributes.frequency;
			*outPageCount = attributes.pageCount;

			return true;
		}
	}

	*outWidth = 0;
	*outHeight = 0;
	*outDepth = 0;
	*outFrequency = 0;
	*outPageCount = 0;

	return false;
}

//------------------------------------------------------------------------------------
//	Return some info about the currently chosen game context. Note that the
//  context may not yet be active. This routine is used by AGLUtils to see what
//  we got when we asked DrawSprocket to find a GL game context for us.
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::GetGameResolution( short *outWidth, short *outHeight, short *outDepth, Fixed *outFrequency, short *outPageCount ) {
	if ( mGameContext ) {
		*outWidth = mGameAttributes.displayWidth;
		*outHeight = mGameAttributes.displayHeight;
		*outDepth = mGameAttributes.displayBestDepth;
		*outFrequency = mGameAttributes.frequency;
		*outPageCount = mGameAttributes.pageCount;

		return true;
	}

	*outWidth = 0;
	*outHeight = 0;
	*outDepth = 0;
	*outFrequency = 0;
	*outPageCount = 0;

	return false;
}


//------------------------------------------------------------------------------------
//		Set the state of the current context
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SetState( DSpContextState inState ) {
	if ( mContext ) {
		return ::DSpContext_SetState( mContext, inState ) == noErr;
	} else {
		return false;
	}
}


//------------------------------------------------------------------------------------
//		Get the state of the current context
//------------------------------------------------------------------------------------
DSpContextState CDrawSprocket::GetState() {
	DSpContextState result;
	if ( mContext && ( ::DSpContext_GetState( mContext, &result ) == noErr ) ) {
		return result;
	}

	return kDSpContextState_Inactive;
}

//------------------------------------------------------------------------------------
//		Get the display ID for the current context
//------------------------------------------------------------------------------------
DisplayIDType CDrawSprocket::GetDisplayID( void ) {
//	DisplayIDType displayID;
//	if (mContext && (::DSpContext_GetDisplayID(mContext, &displayID) == noErr))
//		return displayID;

//	return NULL;
	return mDisplayID;
}

//------------------------------------------------------------------------------------
//		Get the gdevice for the current context
//------------------------------------------------------------------------------------
GDHandle CDrawSprocket::GetGDevice( void ) {
	DisplayIDType displayID;
	GDHandle device = NULL;
	if ( mContext && ( ::DSpContext_GetDisplayID( mContext, &displayID ) == noErr ) ) {
		DMGetGDeviceByDisplayID( displayID, &device, false );
		if ( device == NULL ) {
			device = GetMainDevice();
		}
	}

	return device;
}

//------------------------------------------------------------------------------------
// ¥ÊGetMouse
//------------------------------------------------------------------------------------
// Read the global mouse position and convert it to local coordinates for the
// current context
//------------------------------------------------------------------------------------
void CDrawSprocket::GetMouse( Point *outPoint ) {
	OSStatus status;

	if ( !mContext ) {
		outPoint->h = outPoint->v = 0;
		return;
	}
	status = ::DSpGetMouse( outPoint );
	status = ::DSpContext_GlobalToLocal( mContext, outPoint );
}


//------------------------------------------------------------------------------------
//		Set the CLUT of the on-screen draw context
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SetCLUTEntries( ColorSpec *inColorSpec, unsigned short inStartingEntry, unsigned short inEntryCount ) {
	if ( mContext ) {
		OSErr err = DSpContext_SetCLUTEntries( mContext, inColorSpec, inStartingEntry, inEntryCount );
		if ( err == noErr ) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------------
//		Set one CLUT entry of the on-screen draw context, 16-bit RGB values
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SetRGBIndex( UInt16 red, UInt16 green, UInt16 blue, unsigned short inIndex ) {
	if ( mContext ) {
		ColorSpec colorSpec;

		colorSpec.value = inIndex;
		colorSpec.rgb.red = red;
		colorSpec.rgb.blue = blue;
		colorSpec.rgb.green = green;

		OSErr err = DSpContext_SetCLUTEntries( mContext, &colorSpec, inIndex, 1 );
		if ( err == noErr ) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------------
//		Set a range of CLUT entries of the on-screen draw context, 8-bit RGB values
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::SetRGBEntries_8( UInt8 const *rgbArray, unsigned short inStartingEntry, unsigned short inEntryCount ) {
	if ( mContext ) {
		ColorSpec *colorSpec;
		int i;

		// Allocate storage for a temporary colorSpec array
		colorSpec = (ColorSpec *) malloc( sizeof( ColorSpec ) * inEntryCount );
		if ( colorSpec == NULL ) {
			return false;
		}

		// Convert our 8-bit RGB triplets into 16-bit colorspec entries
		for ( i = 0; i < inEntryCount; i++ )
		{
			colorSpec[i].value = i + inStartingEntry;
			colorSpec[i].rgb.red = ( *rgbArray++ ) << 8;
			colorSpec[i].rgb.green = ( *rgbArray++ ) << 8;
			colorSpec[i].rgb.blue = ( *rgbArray++ ) << 8;
		}

		// Set the color table
		// LBO - Debugging DSp views inEntryCount as inLastEntry?!? So we subtract one for the 256 case...
		OSErr err = DSpContext_SetCLUTEntries( mContext, colorSpec, inStartingEntry, inEntryCount - 1 );

		free( colorSpec );
		if ( err == noErr ) {
			return true;
		}
	}

	return false;
}


//------------------------------------------------------------------------------------
//		Do a slow fade up of the gamma
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FadeGammaUp( Boolean inFadeAllScreens ) {
#if MAC_DEVELOPMENT
	return true;
#endif
	bool result = false;

	// prevent infinite recursion - jb, 3/25/01
	static int alreadyHere = 0;
	if ( alreadyHere ) {
		return false;
	}
	alreadyHere = 1;

	if ( mGammaFadedOut ) {
		MacAssert( mContext );
		if ( mContext ) {
			::DSpContext_FadeGammaIn( inFadeAllScreens ? kDSpEveryContext : mContext, NULL );
			mGammaFadedOut = false;
			result = true;
		}
	}

	alreadyHere = 0;
	return result;
}


//------------------------------------------------------------------------------------
//		Do a slow fade down of the gamma
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FadeGammaDown( Boolean inFadeAllScreens ) {
#if MAC_DEVELOPMENT
	return true;
#endif
	bool result = false;

	// prevent infinite recursion - jb, 3/25/01
	static int alreadyHere = 0;
	if ( alreadyHere ) {
		return false;
	}
	alreadyHere = 1;

	if ( !mGammaFadedOut ) {
		MacAssert( mContext );
		if ( mContext ) {
			::DSpContext_FadeGammaOut( inFadeAllScreens ? kDSpEveryContext : mContext, NULL );
			mGammaFadedOut = true;
			alreadyHere = 0;
			result = true;
		}
	}

	alreadyHere = 0;
	return result;
}


//------------------------------------------------------------------------------------
//		Do an immediate fade up of the gamma
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FadeGammaUpImmediate( Boolean inFadeAllScreens ) {
#if MAC_DEVELOPMENT
	return true;
#endif
	bool result = false;

	// prevent infinite recursion - jb, 3/25/01
	static int alreadyHere = 0;
	if ( alreadyHere ) {
		return false;
	}
	alreadyHere = 1;

	if ( mGammaFadedOut ) {
		if ( mContext ) {
			MacAssert( mContext );
			::DSpContext_FadeGamma( inFadeAllScreens ? kDSpEveryContext : mContext, 100, NULL );
			mGammaFadedOut = false;
			result = true;
		}
	}

	alreadyHere = 0;
	return result;
}


//------------------------------------------------------------------------------------
//		Do an immediate fade of the gamma (either up or down)
//------------------------------------------------------------------------------------
Boolean CDrawSprocket::FadeGammaDownImmediate( Boolean inFadeAllScreens ) {
#if MAC_DEVELOPMENT
	return true;
#endif
	bool result = false;

	// prevent infinite recursion - jb, 3/25/01
	static int alreadyHere = 0;
	if ( alreadyHere ) {
		return false;
	}
	alreadyHere = 1;

	if ( !mGammaFadedOut ) {
		if ( mContext ) {
			MacAssert( mContext );
			::DSpContext_FadeGamma( inFadeAllScreens ? kDSpEveryContext : mContext, 0, NULL );
			mGammaFadedOut = true;
			result = true;
		}
	}

	alreadyHere = 0;
	return result;
}


//------------------------------------------------------------------------------------
//		Grab the GrafPtr of the currently active context's front buffer
//------------------------------------------------------------------------------------
CGrafPtr CDrawSprocket::GetFrontBuffer( void ) {
	if ( mContext ) {
		CGrafPtr port = NULL;
		if ( ::DSpContext_GetFrontBuffer( mContext, &port ) == noErr ) {
			return port;
		}
	}

	return NULL;
}


//------------------------------------------------------------------------------------
//		Grab the GrafPtr of the currently active context's back buffer
//------------------------------------------------------------------------------------
CGrafPtr CDrawSprocket::GetBackBuffer( void ) {
	if ( mContext && mDoubleBuffered ) {
		CGrafPtr port = NULL;
		if ( ::DSpContext_GetBackBuffer( mContext, kDSpBufferKind_Normal, &port ) == noErr ) {
			return port;
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------------
//		Return the CGrafPtr to draw into.  Normally this is the back buffer, but when
//		drawing directly to the screen, it will be the front buffer.
//------------------------------------------------------------------------------------
CGrafPtr CDrawSprocket::GetDrawingBuffer() {
	if ( mDoubleBuffered ) {
		return( GetBackBuffer() );
	} else {
		return( GetFrontBuffer() );
	}
}

//------------------------------------------------------------------------------------
//		Sets QuickDraw to point to the drawing buffer of our context
//------------------------------------------------------------------------------------
void CDrawSprocket::SetDrawingPort( void ) {
	OSErr err = noErr;
	DisplayIDType displayID;
	GDHandle dstDevice;
	GrafPtr dstPort = (GrafPtr)GetDrawingBuffer();
	MacAssert( dstPort );

	DMGetGDeviceByDisplayID( mDisplayID, &dstDevice, false );
	SetGWorld( dstPort, dstDevice );
}

//------------------------------------------------------------------------------------
//		Sets QuickDraw to point to the front buffer of our context
//------------------------------------------------------------------------------------
void CDrawSprocket::SetFrontPort( void ) {
	OSErr err = noErr;
	DisplayIDType displayID;
	GDHandle dstDevice;
	GrafPtr dstPort = (GrafPtr)GetFrontBuffer();
	MacAssert( dstPort );

	DMGetGDeviceByDisplayID( mDisplayID, &dstDevice, false );
	SetGWorld( dstPort, dstDevice );
}

//------------------------------------------------------------------------------------
//		Sets QuickDraw to point to the front buffer of our context
//------------------------------------------------------------------------------------
void CDrawSprocket::SetBackPort( void ) {
	OSErr err = noErr;
	DisplayIDType displayID;
	GDHandle dstDevice;
	GrafPtr dstPort = (GrafPtr)GetBackBuffer();
	MacAssert( dstPort );

	DMGetGDeviceByDisplayID( mDisplayID, &dstDevice, false );
	SetGWorld( dstPort, dstDevice );
}

//------------------------------------------------------------------------------------
//		Get the rowBytes (pitch) of the currently active context's back buffer
//------------------------------------------------------------------------------------
long CDrawSprocket::GetRowBytes( void ) {
	long pitch = 0;

	if ( mContext ) {
#if TARGET_API_MAC_CARBON
		// LBO - this call is 8.5/6 or later
		pitch = GetPixRowBytes( GetPortPixMap( gDrawSprocket->GetBackBuffer() ) );
#else
		pitch = ( *( GetPortPixMap( gDrawSprocket->GetBackBuffer() ) ) )->rowBytes & 0x3fff;
#endif
	}

	return pitch;
}

//------------------------------------------------------------------------------------
//		Copies a gworld to the current drawing buffer. Makes 2 assumptions:
//		1. the gworld pixmap is locked, 2. the buffers are identical in dimension
//------------------------------------------------------------------------------------
void CDrawSprocket::CopyGWorld( GWorldPtr theGWorld, Rect *blitRect ) {
	PixMapHandle pixmap = GetGWorldPixMap( theGWorld );
	CGrafPtr dstPort = GetDrawingBuffer();
	GDHandle saveDevice;
	GrafPtr savePort;
	MacAssert( dstPort );

	// Save current port
	GetGWorld( &savePort, &saveDevice );
	SetDrawingPort();
	ForeColor( blackColor );
	BackColor( whiteColor );

	CopyBits( ( BitMapPtr ) * pixmap, GetPortBitMapForCopyBits( dstPort ),
			  blitRect, blitRect, srcCopy, NULL );

	SetGWorld( savePort, saveDevice );
}

//------------------------------------------------------------------------------------
//		Blit to the screen.
//------------------------------------------------------------------------------------
void CDrawSprocket::SwapBuffers( void ) {
	if ( mDoubleBuffered ) {
//		if (fast)
		{
#if TARGET_API_MAC_CARBON
			CGrafPtr srcPort = GetBackBuffer();
			CGrafPtr dstPort = GetFrontBuffer();
			MacAssert( srcPort );
			MacAssert( dstPort );

			// Save current port
			GrafPtr savePort;
			GDHandle saveDevice;
			Rect srcBounds, dstBounds;

			(void)GetPortBounds( srcPort, &srcBounds );
			(void)GetPortBounds( dstPort, &dstBounds );

			GetGWorld( &savePort, &saveDevice );
//			SetFrontPort ();
			SetPort( dstPort );
			ForeColor( blackColor );
			BackColor( whiteColor );

			CopyBits( GetPortBitMapForCopyBits( srcPort ),
					  GetPortBitMapForCopyBits( dstPort ),
					  &srcBounds, &dstBounds, srcCopy, NULL );

			SetGWorld( savePort, saveDevice );
#else
			DSpBlitInfo blitInfo;
			memset( &blitInfo, 0, sizeof( blitInfo ) );

			blitInfo.srcBuffer = GetBackBuffer();
			// opaque GrafPorts - jb, 5/5/00
			(void)GetPortBounds( blitInfo.srcBuffer, &blitInfo.srcRect );

			blitInfo.dstBuffer = GetFrontBuffer();
			// opaque GrafPorts - jb, 5/5/00
			(void)GetPortBounds( blitInfo.dstBuffer, &blitInfo.dstRect );

			OSStatus err = DSpBlit_Fastest( &blitInfo, false );
#endif
		}
	}
}


//------------------------------------------------------------------------------------
//		Erase the back buffer.
//------------------------------------------------------------------------------------
void CDrawSprocket::ClearBackBuffer( RGBColor *inColor ) {
	if ( mDoubleBuffered ) {
		CGrafPtr backBuffer = GetBackBuffer();
		if ( backBuffer ) {
			// Save current port
			GrafPtr savePort;
			GDHandle saveDevice;
			GetGWorld( &savePort, &saveDevice );

			// Paint back buffer the specified color
			SetBackPort();
			Rect r;
			(void)GetPortBounds( backBuffer, &r );

			RGBForeColor( inColor );
			PaintRect( &r );
			ForeColor( blackColor );

			// Restore old port
			SetGWorld( savePort, saveDevice );
		}
	} else
	{
		CGrafPtr backBuffer = GetFrontBuffer();
		if ( backBuffer ) {
			// Save current port
			GrafPtr savePort;
			GDHandle saveDevice;
			GetGWorld( &savePort, &saveDevice );

			// Paint back buffer the specified color
			SetFrontPort();
			Rect r;
			(void)GetPortBounds( backBuffer, &r );

			RGBForeColor( inColor );
			PaintRect( &r );
			ForeColor( blackColor );

			// Restore old port
			SetGWorld( savePort, saveDevice );
		}
	}
}


//------------------------------------------------------------------------------------
//		Invalidate a portion of the BackBuffer a faster DSp_Blit
//------------------------------------------------------------------------------------
void CDrawSprocket::InvalBackBufferRect( Rect *inRect ) {
#if !TARGET_API_MAC_CARBON
	if ( mDoubleBuffered ) {
		DSpContext_InvalBackBufferRect( mContext, inRect );
	}
#endif
}

//------------------------------------------------------------------------------------
//		Whether to use new DSp 1.7 queue/switch functions.
//------------------------------------------------------------------------------------
void CDrawSprocket::UseQueueAndSwitch( bool useIt ) {
	// make sure you only do this early on, before activating any contexts
	MacAssert( mContext == NULL );

	if ( useIt && ( Ptr ) ::DSpContext_Queue != (Ptr) kUnresolvedCFragSymbolAddress ) {
		mHasQueue = true;
	} else {
		mHasQueue = false;
	}
}

//------------------------------------------------------------------------------------
//		True if we're to draw in a window instead of fullscreen
//------------------------------------------------------------------------------------
void CDrawSprocket::UseWindow( bool useWindow ) {
	// if no context, just set the flag and go
	if ( mContext == NULL ) {
		goto bail;
	}

	if ( mInWindow != useWindow ) {
	}

bail:
	mInWindow = useWindow;
}
