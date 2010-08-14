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

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// CarbonMouse.c
//
// © 2001-5 Aspyr Media.
//
// Some Carbon routines to handle reading the mouse state.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Includes
//

#include <Carbon/Carbon.h>
#include "CarbonMouse.h"
#include "MacPrefs.h"

#if MAC_Q3
#ifdef __cplusplus
extern "C" {
#endif
Boolean ConsoleWindowIsFrontmost( void );
#ifdef __cplusplus
}
#endif
#endif

extern long gSystemVersion;

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Defines
//

#define MAX_BUTTONS 32      // Carbon events supports up to 65536 buttons,
							// but we're a little more pragmatic

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Local variables
//

static EventHandlerUPP sMouseEventHandlerUPP;

static Point sMouseDelta;                       // current delta value
static SInt32 sMouseWheelDelta;                 // current mouse wheel delta
static Boolean sMouseButtons[MAX_BUTTONS];      // current button state
static EventTime sMouseTime[MAX_BUTTONS];       // timestamp of last button event
static EventTime sMouseDeltaTime;               // timestamp of last delta event
static EventTime sMouseWheelDeltaTime;          // timestamp of last delta event

static Boolean sMouseEnabled;

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Implementation
//

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// appMouseEventHandler
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// This event handler is installed by Carbon_InitMouse (below) to respond to
// Carbon events that affect the application as a whole. It does all the work
// of keeping track of mouse deltas and button states.
//
// Caveats: mouse deltas are only supported under X so far. Also, under Carbon 9
// pressing any mouse button returns a button value of 1, e.g. the main mouse button
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
static pascal OSStatus appMouseEventHandler( EventHandlerCallRef myHandler, EventRef event, void* userData ) {
#pragma unused (myHandler, userData)
	OSStatus result = eventNotHandledErr;
	EventMouseButton theButton;
	Point mouseDelta;
	SInt32 mouseWheelDelta;
	UInt32 eventKind;

	if ( !sMouseEnabled ) {
		return eventNotHandledErr;
	}

#if MAC_Q3
	// MLTE handles some mouse events, so we pass them along if the console is frontmost
	if ( ConsoleWindowIsFrontmost() ) {
		return eventNotHandledErr;
	}
#endif

	eventKind = GetEventKind( event );

	switch ( eventKind )
	{
	case kEventMouseMoved:
	case kEventMouseDragged:
	{
		EventTime evtTime = GetEventTime( event );

		result = GetEventParameter( event, kEventParamMouseDelta, typeQDPoint, NULL,
									sizeof( mouseDelta ), NULL, &mouseDelta );

		if ( evtTime == sMouseDeltaTime ) {
			return eventNotHandledErr;
		}

		// Clamp any overflow
		if ( (SInt32) sMouseDelta.h + (SInt32) mouseDelta.h > 32767 ) {
			sMouseDelta.h = 32767;
		} else if ( (SInt32) sMouseDelta.h + (SInt32) mouseDelta.h < -32767 ) {
			sMouseDelta.h = -32767;
		} else {
			sMouseDelta.h += mouseDelta.h;
		}

		if ( (SInt32) sMouseDelta.v + (SInt32) mouseDelta.v > 32767 ) {
			sMouseDelta.h = 32767;
		} else if ( (SInt32) sMouseDelta.v + (SInt32) mouseDelta.v < -32767 ) {
			sMouseDelta.v = -32767;
		} else {
			sMouseDelta.v += mouseDelta.v;
		}

		sMouseDeltaTime = evtTime;

		// We handled the event, eat it
		result = noErr;
		break;
	}
	case kEventMouseDown:
	{
		(void)GetEventParameter( event, kEventParamMouseButton, typeMouseButton, NULL,
								 sizeof( theButton ), NULL, &theButton );

		// the button # is 1-based, our array is 0-based
		theButton -= 1;

		if ( theButton < MAX_BUTTONS ) {
			EventTime evtTime = GetEventTime( event );

			if ( evtTime != sMouseTime[theButton] ) {
				sMouseButtons[theButton] = 1;
				sMouseTime[theButton] = evtTime;

				// We handled the event, eat it
				result = noErr;
			}
		}
		break;
	}
	case kEventMouseUp:
	{
		(void)GetEventParameter( event, kEventParamMouseButton, typeMouseButton, NULL,
								 sizeof( theButton ), NULL, &theButton );

		// the button # is 1-based, our array is 0-based
		theButton -= 1;

		if ( theButton < MAX_BUTTONS ) {
			EventTime evtTime = GetEventTime( event );

			if ( evtTime != sMouseTime[theButton] ) {
				sMouseButtons[theButton] = 0;
				sMouseTime[theButton] = evtTime;

				// We handled the event, eat it
				result = noErr;
			}
		}
		break;
	}
	case kEventMouseWheelMoved:
	{
		EventTime evtTime = GetEventTime( event );

		result = GetEventParameter( event, kEventParamMouseWheelDelta, typeSInt32, NULL,
									sizeof( mouseWheelDelta ), NULL, &mouseWheelDelta );

		if ( evtTime == sMouseWheelDeltaTime ) {
			return eventNotHandledErr;
		}

		// Clamp any overflow. Note that the delta is already a SInt32, so we're
		// clamping very prematurely. It should never be an issue though.
		if ( sMouseWheelDelta + mouseWheelDelta > 32767 ) {
			sMouseWheelDelta = 32767;
		} else {
			sMouseWheelDelta += mouseWheelDelta;
		}

		sMouseWheelDeltaTime = evtTime;

		// We handled the event, eat it
		result = noErr;
		break;
	}
	}

	return result;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Carbon_InitMouse
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Call this to install CarbonEvent handlers to read the mouse states.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
Boolean Carbon_InitMouse( void ) {
	EventHandlerRef ref;
	OSStatus status;
	EventTypeSpec list[] = {  {kEventClassMouse, kEventMouseDown },
							  {kEventClassMouse, kEventMouseUp },
							  {kEventClassMouse, kEventMouseMoved },            // deltas while mouse button is up
							  {kEventClassMouse, kEventMouseDragged },          // deltas while mouse button is down
							  {kEventClassMouse, kEventMouseWheelMoved } };

	// If we don't support Carbon events, bail
	if ( (Ptr) InstallEventHandler == (Ptr) kUnresolvedCFragSymbolAddress ) {
		goto bail;
	}

	// Install an application event handler
	sMouseEventHandlerUPP = NewEventHandlerUPP( appMouseEventHandler );
	status = InstallApplicationEventHandler( sMouseEventHandlerUPP, 5, list, 0, &ref );

#if TARGET_RT_MAC_CFM // ¥¥¥¥¥¥
	if ( status != noErr ) {
		return false;
	}
#endif

	// Disable the mouse initially so that any pre-game dialogs will work
	sMouseEnabled = false;

	// indicate success
	return true;

bail:
	return false;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Carbon_ReadMouseDeltas
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Effectively polls the mouse delta values as set by the Carbon event handlers.
// It also resets them so that the delta states accurately represent the delta
// since the last time this routine was called.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void Carbon_ReadMouseDeltas( SInt32 *deltaX, SInt32 *deltaY ) {
	*deltaX = sMouseDelta.h;
	*deltaY = sMouseDelta.v;

	// Reset them for the next time through
	sMouseDelta.h = sMouseDelta.v = 0;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Carbon_ReadMouseWheelDelta
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Effectively polls the mouse delta values as set by the Carbon event handlers.
// It also resets them so that the delta states accurately represent the delta
// since the last time this routine was called.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void Carbon_ReadMouseWheelDelta( SInt32 *delta ) {
	*delta = sMouseWheelDelta;

	// Reset them for the next time through
	sMouseWheelDelta = 0;
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Carbon_ReadMouseButton
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Effectively polls the mouse button values based on the current
// states as set by the Carbon event handlers. First button is 0.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
Boolean Carbon_ReadMouseButton( int inButtonNum ) {
	if ( inButtonNum >= MAX_BUTTONS ) {
		return false;
	}

	return sMouseButtons[inButtonNum];
}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Carbon_EnableMouse
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// Controls whether our Carbon event handler will handle mouse events or
// ignore them totally. Used around dialogs, similar to InputSprocket.
// If mouse events are enabled, we eat all the events we can handle, which
// means that they don't get passed back to the system.
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void Carbon_EnableMouse( Boolean inState ) {
	int i;

	// set the state
	sMouseEnabled = inState;

	if ( inState ) {
		CGPoint newPoint;
		CGRect bounds;
		CGDirectDisplayID displayID;

		displayID = (CGDirectDisplayID) macPrefs.displayID;
		if ( displayID == 0 ) {
			displayID = kCGDirectMainDisplay;
		}

		bounds = CGDisplayBounds( displayID );

		// FIXME: this is hosed when you're running in a window .. that stuff is working with the display

		// If we're capturing the mouse, pin cursor to center of selected display.
		newPoint.x = bounds.origin.x + ( bounds.size.width / 2 );
		newPoint.y = bounds.origin.y + ( bounds.size.height / 2 );

		CGSetLocalEventsSuppressionInterval( 0.0 );
		CGWarpMouseCursorPosition( newPoint );
	}

	CGAssociateMouseAndMouseCursorPosition( !inState );

	// reset our internal states to default values
	sMouseDelta.h = sMouseDelta.v = 0;
	sMouseWheelDelta = 0;
	for ( i = 0; i < MAX_BUTTONS; i++ )
		sMouseButtons[i] = 0;
}

