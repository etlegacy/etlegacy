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

#include <math.h>

Q3DEF_BEGIN
#include "../client/client.h"
#include "mac_local.h"
Q3DEF_END

#include "MacPrefs.h"
#include "CarbonMouse.h"
//#include "HID Manager/HID_Utilities_CFM.h"

extern "C" FILE * FSp_fopen( const FSSpec *spec, const char *open_mode );

Boolean inputActive = false;
Boolean inputSuspended = true;
Boolean inputCarbon = false;
Boolean inputHID = false;
Boolean inputMouseInWindow = true;

//HIDEntry HIDAnalog[kQ3_AnalogAxes];
//HIDEntry HIDDigital[kQ3_DigitalDirections];
//HIDEntry HIDButtons[kQ3_DigitalButtons];

#define MAX_DEVICES     16

#if MAC_Q3_OLDSTUFF
#define MAX_MOUSE_BUTTONS   17
#else
#define MAX_MOUSE_BUTTONS   24
#endif
#define MAX_ELEMENTS    32
#define MAX_DELTAS  5
UInt32 numDevices;
UInt32 numElements[MAX_DEVICES];

//HIDEntry			hidElements[MAX_DEVICES][MAX_ELEMENTS];

int prevMove[MAX_DEVICES][MAX_ELEMENTS];
float mouseFactor;

cvar_t              *in_mouse;
cvar_t              *in_joystick;
cvar_t              *in_macMouseDivider;
cvar_t              *in_joyThreshold;
cvar_t              *joy_xbutton;
cvar_t              *joy_ybutton;

#if MAC_Q3_OLDSTUFF
#define A_MWHEELDOWN    K_MWHEELDOWN
#define A_MWHEELUP      K_MWHEELUP
#define A_CURSOR_UP     K_UPARROW
#define A_CURSOR_DOWN   K_DOWNARROW
#define A_CURSOR_LEFT   K_LEFTARROW
#define A_CURSOR_RIGHT  K_RIGHTARROW

#define A_MOUSE1    K_MOUSE1
#define A_MOUSE2    K_MOUSE2
#define A_MOUSE3    K_MOUSE3
#define A_MOUSE4    K_MOUSE4
#define A_MOUSE5    K_MOUSE5

#define A_AUX1      K_AUX1
#define A_AUX2      K_AUX2
#define A_AUX3      K_AUX3
#define A_AUX4      K_AUX4

#define A_AUX5      K_AUX5
#define A_AUX6      K_AUX6
#define A_AUX7      K_AUX7
#define A_AUX8      K_AUX8
#define A_AUX9      K_AUX9
#define A_AUX10     K_AUX10
#define A_AUX11     K_AUX11
#define A_AUX12     K_AUX12
#define A_AUX13     K_AUX13
#define A_AUX14     K_AUX14
#define A_AUX15     K_AUX15
#define A_AUX16     K_AUX16

#define A_JOY0      K_JOY1
#define A_JOY1      K_JOY2
#endif

static Boolean sLastButton[MAX_MOUSE_BUTTONS];

extern short IsPressed( unsigned short k );

void Input_Init( void );
void Input_GetState( void );


/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput( void ) {
	NumVersion ver;
	int i, j;
	OSStatus err;
	UInt32 temp;

	inputCarbon = false;
	inputHID = false;
	inputActive = false;

#if MAC_Q3_MP
	// no input with dedicated servers
	if ( com_dedicated->integer ) {
		return;
	}
#endif

	Com_Printf( "------- Input Initialization -------\n" );
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE );
	in_macMouseDivider = Cvar_Get( "in_macMouseDivider", "163", CVAR_ARCHIVE );   // FIXME: why this constant
	in_joyThreshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE );
	joy_xbutton = Cvar_Get( "joy_xbutton", "1", CVAR_ARCHIVE );
	joy_ybutton = Cvar_Get( "joy_ybutton", "1", CVAR_ARCHIVE );

	mouseFactor = 1.0 / in_macMouseDivider->value;

	{
		inputHID = true;

		if ( Carbon_InitMouse() ) {
			inputCarbon = true;
			if ( in_mouse->integer ) {
				inputActive = true;
			}

			Com_Printf( "------------------------------------\n" );
			return;
		} else
		{
			inputActive = false;
			return;
		}
	}
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput( void ) {
	if ( inputHID ) {
		inputHID = false;
	}

	if ( inputCarbon ) {
		Carbon_EnableMouse( false );
	}

	if ( !inputActive ) {
		return;
	}
	CGDirectDisplayID displayID;
	displayID = (CGDirectDisplayID)macPrefs.displayID;
	if ( displayID == 0 ) {
		displayID = kCGDirectMainDisplay;
	}
	CGDisplayShowCursor( displayID );

	inputActive = qfalse;
	inputSuspended = true;
}

void Sys_SuspendInput( void ) {
	if ( inputSuspended ) {
		return;
	}
	inputSuspended = true;
	if ( inputCarbon ) {
		Carbon_EnableMouse( false );
	}
	InitCursor();
}

void Sys_ResumeInput( void ) {
	if ( !inputSuspended ) {
		return;
	}
	inputSuspended = false;
	CGDirectDisplayID displayID;
	displayID = (CGDirectDisplayID)macPrefs.displayID;
	if ( displayID == 0 ) {
		displayID = kCGDirectMainDisplay;
	}
	CGDisplayHideCursor( displayID );
	memset( prevMove, 0, sizeof( prevMove ) );

	if ( inputCarbon ) {
		int i;

		// Reset our button tracking states
		for ( i = 0; i < MAX_MOUSE_BUTTONS; i++ )
			sLastButton[i] = 0;
		Carbon_EnableMouse( true );
	}
}

// Note that A_MOUSE1 through A_MOUSE5 are not consecutive constants
// in the Q3 core, thus the lookup table
static int sMouseButtonLookup[MAX_MOUSE_BUTTONS] =
{
	A_MOUSE1, A_MOUSE2, A_MOUSE3, A_MOUSE4, A_MOUSE5, A_AUX5, A_AUX6, A_AUX7,
	A_AUX8, A_AUX9, A_AUX10, A_AUX11, A_AUX12, A_AUX13, A_AUX14, A_AUX15, A_AUX16
#if !MAC_Q3_OLDSTUFF
	,A_AUX17, A_AUX18, A_AUX19, A_AUX20, A_AUX21, A_AUX22, A_AUX23
#endif
};

void Sys_Input_Carbon( void ) {
	SInt32 move[MAX_DELTAS];
	int i;

	if ( in_mouse->integer != 0 ) {
		// Read the mouse deltas
		Carbon_ReadMouseDeltas( &move[0], &move[1] );

		if ( move[0] || move[1] ) {
			Sys_QueEvent( 0, SE_MOUSE, move[0], move[1], 0, NULL );
		}

		// Read the button states
		for ( i = 0; i < MAX_MOUSE_BUTTONS; i++ )
		{
			Boolean isPressed;

			isPressed = Carbon_ReadMouseButton( i );

			// If the button state differs from what we last saw, generate an event
			if ( isPressed != sLastButton[i] ) {
				int q3_button;
				q3_button = sMouseButtonLookup[i];
				// 1.04.1 patch
				if ( ( q3_button == A_MOUSE1 ) && ( macPrefs.button2fake ) && IsPressed( kControlKey ) ) {
					q3_button = A_MOUSE2;
				}
				Sys_QueEvent( 0, SE_KEY, q3_button, isPressed, 0, NULL );
				sLastButton[i] = isPressed;
			}
		}

		// Read the mouse wheel delta
		Carbon_ReadMouseWheelDelta( &move[2] );

		if ( move[2] < 0 ) {
			Sys_QueEvent( 0, SE_KEY, A_MWHEELDOWN, 1, 0, NULL );
		} else if ( move[2] > 0 ) {
			Sys_QueEvent( 0, SE_KEY, A_MWHEELUP, 1, 0, NULL );
		}
	}
}


// LBO - note that changes are needed to cl_input.cpp in the routine
// CL_JoystickMove(). It's surprisingly broken - I guess no one tries to
// play Quake3 games with analog sticks. :-)

#define kJoystickAnalogLimit 512

int joyDirection[4] = {
	A_CURSOR_UP, A_CURSOR_DOWN,
	A_CURSOR_LEFT, A_CURSOR_RIGHT
};

/*
=================
Sys_Input
=================
*/
void Sys_Input( void ) {

	if ( !inputActive ) {
		return;
	}

#if MAC_Q3_MP
	// always suspend for dedicated
	if ( com_dedicated->integer ) {
		Sys_SuspendInput();
		return;
	}
#endif

	if ( ( !glConfig.isFullscreen ) && ( !inputMouseInWindow ) ) {
		Sys_SuspendInput();
		return;
	}

#if !MAC_MOHAA && !MAC_ALICE
	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		// temporarily deactivate if not in the game and
		// running on the desktop
		if ( Cvar_VariableValue( "r_fullscreen" ) == 0 ) {
			Sys_SuspendInput();
			return;
		}
	}
#endif

	Sys_ResumeInput();

	// If we're using OSX, send mouse events via CarbonEvents
	if ( inputCarbon ) {
		Sys_Input_Carbon();
	}
}

extern "C" void IN_StartupJoystick( void ) {
}

extern "C" void IN_MouseCancel( void ) {
}
