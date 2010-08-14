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

// ======================================================================
// CDrawSprocket.h
// ======================================================================
#pragma once

// ======================================================================
// includes
// ======================================================================

#include <DrawSprocket/DrawSprocket.h>

// ======================================================================
// structures
// ======================================================================

class CDrawSprocket
{
private:
DSpContextReference mContext;                       // Currently active context
DSpContextReference mGameContext;                   // Context we'll use for the game
DSpContextReference mAltGameContext;                // Context we'll use for the game
DSpContextReference mMovieContext;                  // Context we'll use for movies
DSpContextAttributes mAttributes;                   // Attributes of current context
DSpContextAttributes mGameAttributes;               // Attributes of current context
DSpContextAttributes mAltGameAttributes;            // Attributes of current context
DSpContextAttributes mMovieAttributes;              // Attributes of current context
Boolean mUseDefaultContext;                         // When true, we let Draw Sprocket pick a monitor
DisplayIDType mDisplayID;                           // Try to use this screen ID when switching contexts

Boolean mGammaFadedOut;
Boolean mPickedMonitor;                             // true if the user has chosen a display already

short mXOffset;                                     // Used if we have to center the screen output manually in a context
short mYOffset;                                     // that's too large

long mWRefCon;                                      //�Similar to a MacOS Window's RefCon - phs, 9/30/99

bool mDoubleBuffered;                               // Set this to "true" if you want a single
													// DSp Context (as opposed to swapping buffers
													// between two contexts)

bool mHasQueue;                                     // True if we have the DSp 1.7 Queue/Switch calls available
bool mInWindow;                                     // True if we're drawing into a window instead of a fullscreen context

public:
CDrawSprocket( DisplayIDType inPreferredMonitor = 0 );
virtual ~CDrawSprocket();

virtual void            EnableDebugging( Boolean inDebugMode );
virtual Boolean         IsValidResolution( short inWidth, short inHeight, short inDepth, Fixed inFrequency = 0 );

virtual Boolean         FindBestContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount, DSpContextReference *outContext, DSpContextAttributes *outAttributes );
virtual Boolean         FindFirstContext( DSpContextReference *outContext, DSpContextAttributes *outAttributes );
virtual Boolean         FindNextContext( DSpContextReference *outContext, DSpContextAttributes *outAttributes );
DSpContextReference     GetCurrentContext()             { return mContext; }
DSpContextReference     GetGameContext()                { return mGameContext; }
DSpContextReference     GetAltGameContext()             { return mAltGameContext; }
DSpContextReference     GetMovieContext()               { return mMovieContext; }

virtual Boolean         ConfigureMovieContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount );
virtual Boolean         ConfigureGameContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount );
virtual Boolean         ConfigureAltGameContext( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount );

virtual Boolean         SwitchToContext( DSpContextReference inContext, DSpContextAttributes *inAttrib, Boolean inDoFade );

virtual Boolean         SwitchToMovieContext( Boolean inDoFade = true );
virtual Boolean         SwitchToGameContext( Boolean inDoFade = true );
virtual Boolean         SwitchToAltGameContext( Boolean inDoFade = true );

virtual Boolean         SwitchToResolution( short inWidth, short inHeight, short inDepth, Fixed inFrequency, short inPageCount, Boolean inDoFade );
virtual Boolean         GetResolution( short *outWidth, short *outHeight, short *outDepth, Fixed *outFrequency, short *outPageCount );
virtual Boolean         GetGameResolution( short *outWidth, short *outHeight, short *outDepth, Fixed *outFrequency, short *outPageCount );
virtual Boolean         SetState( DSpContextState inState );
virtual void            GetMouse( Point *outPoint );
virtual DisplayIDType   GetDisplayID( void );
virtual GDHandle        GetGDevice( void );

virtual DSpContextState GetState();

virtual Boolean         SetCLUTEntries( ColorSpec *inColorSpec, unsigned short inStartingEntry, unsigned short inEntryCount );
virtual Boolean         SetRGBIndex( UInt16 red, UInt16 green, UInt16 blue, unsigned short inIndex );
virtual Boolean         SetRGBEntries_8( UInt8 const *rgbArray, unsigned short inStartingEntry, unsigned short inEntryCount );

virtual Boolean         FadeGammaUp( Boolean inFadeAllScreens );
virtual Boolean         FadeGammaDown( Boolean inFadeAllScreens );
virtual Boolean         FadeGammaUpImmediate( Boolean inFadeAllScreens );
virtual Boolean         FadeGammaDownImmediate( Boolean inFadeAllScreens );

bool                    IsDoubleBuffered()              { return mDoubleBuffered; }
virtual CGrafPtr        GetFrontBuffer();
virtual CGrafPtr        GetBackBuffer();
void                    SetDrawingPort( void );
void                    SetFrontPort( void );
void                    SetBackPort( void );
long                    GetRowBytes( void );

CGrafPtr                GetDrawingBuffer();                         // Returns the buffer to draw into (back normally) - KLC.
void                    CopyGWorld( GWorldPtr theGWorld, Rect *blitRect );          // Copies a gworld to the current drawing buffer
void                    SwapBuffers();                              // Blits entire screen - KLC.
void                    ClearBackBuffer( RGBColor *inColor );       // Sets the back buffer to specified color - phs, 8/19/99
void                    InvalBackBufferRect( Rect *inRect );

void                    SetWRefCon( long inWRefCon )    { mWRefCon = inWRefCon; }
long                    GetWRefCon( void )              { return mWRefCon; }

void                    UseQueueAndSwitch( bool useIt );
void                    UseWindow( bool useWindow );
};

// Single global instance of the CDrawSprocket class.
extern CDrawSprocket *gDrawSprocket;
