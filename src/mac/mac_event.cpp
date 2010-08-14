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
	#include "../client/client.h"
	#include "mac_local.h"
Q3DEF_END

#if MAC_ALICE
extern void MUSIC_CheckLooping( void );
#endif

void DoDrag( WindowRef myWindow,Point mouseloc );
void DoGoAwayBox( WindowRef myWindow, Point mouseloc );
void DoCloseWindow( WindowRef myWindow );
void DoKeyDown( EventRecord *event );
static void DoDiskEvent( EventRecord *event );
static void DoOSEvent( EventRecord *event );
static void DoUpdate( WindowRef myWindow );
static void DoActivate( WindowRef myWindow, int myModifiers );
static void DoAboutBox( void );
void DoMenuCommand( long menuAndItem );
void DoMouseDown( EventRecord *event );
void DoMouseUp( EventRecord *event );
void DoMenuAdjust( void );
void DoKeyUp( EventRecord *event );

static EventLoopTimerRef sTimerRef;
static EventLoopTimerUPP sTimerUPP;

extern cvar_t   *r_fullscreen;

/*
================
Sys_MsecForMacEvent

Q3 event records take time in msec,
so convert the mac event record when
(60ths) to msec.  The base values
are updated ever frame, so this
is guaranteed to not drift.
=================
*/
int Sys_MsecForMacEvent( void ) {
	int tics;

	tics = sys_lastEventTic - sys_ticBase;
	return sys_msecBase + tics * 16;
}




void DoMouseDown( EventRecord *event ) {
	int myPart;
	WindowRef myWindow;
	Point point;

	myPart = FindWindow( event->where, &myWindow );

	switch ( myPart )
	{
	case inMenuBar:
		DrawMenuBar();
		DoMenuCommand( MenuSelect( event->where ) );
		break;
	case inDrag:
		DoDrag( myWindow, event->where );

		// update the vid_xpos / vid_ypos cvars
		point.h = 0;
		point.v = 0;
		LocalToGlobal( &point );
		Cvar_SetValue( "vid_xpos", point.h );
		Cvar_SetValue( "vid_ypos", point.v );
		return;
		break;
	case inGoAway:
		DoGoAwayBox( myWindow, event->where );
		break;

	case inContent:
		if ( myWindow != FrontWindow() ) {
			SelectWindow( myWindow );
		}
		break;
	}
}

void DoMouseUp( EventRecord *event ) {
}

void DoDrag( WindowRef myWindow, Point mouseloc ) {
#if !DEDICATED
	Rect dragBounds;

	GetRegionBounds( GetGrayRgn(), &dragBounds );
	DragWindow( myWindow,mouseloc,&dragBounds );

	aglUpdateContext( aglGetCurrentContext() );
#endif
}


void DoGoAwayBox( WindowRef myWindow, Point mouseloc ) {
	if ( TrackGoAway( myWindow,mouseloc ) ) {
		DoCloseWindow( myWindow );
	}
}

void DoCloseWindow( WindowRef myWindow ) {
}

void DoMenuAdjust( void ) {
}

#if MAC_MOHAA
#define K_SHIFT K_LSHIFT
#define K_ALT   K_LALT
#define K_CTRL  K_LCTRL
#endif

#if MAC_Q3_OLDSTUFF
int vkeyToQuakeKey[256] = {
/*0x00*/ 'a', 's', 'd', 'f', 'h', 'g', 'z', 'x',
/*0x08*/ 'c', 'v', '?', 'b', 'q', 'w', 'e', 'r',
/*0x10*/ 'y', 't', '1', '2', '3', '4', '6', '5',
/*0x18*/ '=', '9', '7', '-', '8', '0', ']', 'o',
/*0x20*/ 'u', '[', 'i', 'p', K_ENTER, 'l', 'j', '\'',
/*0x28*/ 'k', ';', '\\', ',', '/', 'n', 'm', '.',
/*0x30*/ K_TAB, K_SPACE, '`', K_BACKSPACE, '?', K_ESCAPE, '?', K_COMMAND,
/*0x38*/ K_SHIFT, K_CAPSLOCK, K_ALT, K_CTRL, '?', '?', '?', '?',
/*0x40*/ '?', K_KP_DEL, '?', K_KP_STAR, '?', K_KP_PLUS, '?', K_KP_NUMLOCK,
/*0x48*/ '?', '?', '?', K_KP_SLASH, K_KP_ENTER, '?', K_KP_MINUS, '?',
/*0x50*/ '?', K_KP_EQUALS, K_KP_INS, K_KP_END, K_KP_DOWNARROW, K_KP_PGDN, K_KP_LEFTARROW, K_KP_5,
/*0x58*/ K_KP_RIGHTARROW, K_KP_HOME, '?', K_KP_UPARROW, K_KP_PGUP, '?', '?', '?',
/*0x60*/ K_F5, K_F6, K_F7, K_F3, K_F8, K_F9, '?', K_F11,
/*0x68*/ '?', K_F13, '?', K_F14, '?', K_F10, '?', K_F12,
/*0x70*/ '?', K_F15, K_INS, K_HOME, K_PGUP, K_DEL, K_F4, K_END,
/*0x78*/ K_F2, K_PGDN, K_F1, K_LEFTARROW, K_RIGHTARROW, K_DOWNARROW, K_UPARROW, K_POWER
};

// A -> Q, Q -> A
// Z -> W, W -> Z
// - -> )
// ] -> $
// [ -> ^
// ' -> `
// ; -> M
// \ -> '
// , -> ;
// / -> -
// M -> ,
// . -> :
int vkeyToQuakeKey_French[256] = {
/*0x00*/ 'q', 's', 'd', 'f', 'h', 'g', 'w', 'x',
/*0x08*/ 'c', 'v', '?', 'b', 'a', 'z', 'e', 'r',
/*0x10*/ 'y', 't', '1', '2', '3', '4', '6', '5',
/*0x18*/ '=', '9', '7', ')', '8', '0', '$', 'o',
/*0x20*/ 'u', '^', 'i', 'p', K_ENTER, 'l', 'j', '`',
/*0x28*/ 'k', 'M', '\'', ';', '-', 'n', ',', ':',
/*0x30*/ K_TAB, K_SPACE, '`', K_BACKSPACE, '?', K_ESCAPE, '?', K_COMMAND,
/*0x38*/ K_SHIFT, K_CAPSLOCK, K_ALT, K_CTRL, '?', '?', '?', '?',
/*0x40*/ '?', K_KP_DEL, '?', K_KP_STAR, '?', K_KP_PLUS, '?', K_KP_NUMLOCK,
/*0x48*/ '?', '?', '?', K_KP_SLASH, K_KP_ENTER, '?', K_KP_MINUS, '?',
/*0x50*/ '?', K_KP_EQUALS, K_KP_INS, K_KP_END, K_KP_DOWNARROW, K_KP_PGDN, K_KP_LEFTARROW, K_KP_5,
/*0x58*/ K_KP_RIGHTARROW, K_KP_HOME, '?', K_KP_UPARROW, K_KP_PGUP, '?', '?', '?',
/*0x60*/ K_F5, K_F6, K_F7, K_F3, K_F8, K_F9, '?', K_F11,
/*0x68*/ '?', K_F13, '?', K_F14, '?', K_F10, '?', K_F12,
/*0x70*/ '?', K_F15, K_INS, K_HOME, K_PGUP, K_DEL, K_F4, K_END,
/*0x78*/ K_F2, K_PGDN, K_F1, K_LEFTARROW, K_RIGHTARROW, K_DOWNARROW, K_UPARROW, K_POWER
};

// Z -> Y, Y -> Z
// = -> '
// - -> §
// ] -> +
// [ -> Ÿ
// ' -> Š
// ; -> š
// \ -> #
// / -> -
int vkeyToQuakeKey_German[256] = {
/*0x00*/ 'a', 's', 'd', 'f', 'h', 'g', 'y', 'x',
/*0x08*/ 'c', 'v', '?', 'b', 'q', 'w', 'e', 'r',
/*0x10*/ 'z', 't', '1', '2', '3', '4', '6', '5',
/*0x18*/ '\'', '9', '7', '§', '8', '0', '+', 'o',
/*0x20*/ 'u', 'Ÿ', 'i', 'p', K_ENTER, 'l', 'j', 'Š',
/*0x28*/ 'k', 'š', '#', ',', '-', 'n', 'm', '.',
/*0x30*/ K_TAB, K_SPACE, '`', K_BACKSPACE, '?', K_ESCAPE, '?', K_COMMAND,
/*0x38*/ K_SHIFT, K_CAPSLOCK, K_ALT, K_CTRL, '?', '?', '?', '?',
/*0x40*/ '?', K_KP_DEL, '?', K_KP_STAR, '?', K_KP_PLUS, '?', K_KP_NUMLOCK,
/*0x48*/ '?', '?', '?', K_KP_SLASH, K_KP_ENTER, '?', K_KP_MINUS, '?',
/*0x50*/ '?', K_KP_EQUALS, K_KP_INS, K_KP_END, K_KP_DOWNARROW, K_KP_PGDN, K_KP_LEFTARROW, K_KP_5,
/*0x58*/ K_KP_RIGHTARROW, K_KP_HOME, '?', K_KP_UPARROW, K_KP_PGUP, '?', '?', '?',
/*0x60*/ K_F5, K_F6, K_F7, K_F3, K_F8, K_F9, '?', K_F11,
/*0x68*/ '?', K_F13, '?', K_F14, '?', K_F10, '?', K_F12,
/*0x70*/ '?', K_F15, K_INS, K_HOME, K_PGUP, K_DEL, K_F4, K_END,
/*0x78*/ K_F2, K_PGDN, K_F1, K_LEFTARROW, K_RIGHTARROW, K_DOWNARROW, K_UPARROW, K_POWER
};

#else
int vkeyToQuakeKey[256] = {
//			0 or 8		1 or 9			2 or A		3 or B			4 or C			5 or D			6 or E			7 or F
/*0x00*/ A_CAP_A,    A_CAP_S,        A_CAP_D,    A_CAP_F,        A_CAP_H,        A_CAP_G,        A_CAP_Z,        A_CAP_X,
/*0x08*/ A_CAP_C,    A_CAP_V,        A_DIVIDE,   A_CAP_B,        A_CAP_Q,        A_CAP_W,        A_CAP_E,        A_CAP_R,
/*0x10*/ A_CAP_Y,    A_CAP_T,        A_1,        A_2,            A_3,            A_4,            A_6,            A_5,
/*0x18*/ '=',     A_9,            A_7,        '-',         A_8,            A_0,            ']',         A_CAP_O,
/*0x20*/ A_CAP_U,    '[',         A_CAP_I,    A_CAP_P,        A_ENTER,        A_CAP_L,        A_CAP_J,        '\'',
/*0x28*/ A_CAP_K,    ';',         '\\',     ',',         '/',         A_CAP_N,        A_CAP_M,        '.',
/*0x30*/ A_TAB,      A_SPACE,        A_CONSOLE,  A_BACKSPACE,    '?',         A_ESCAPE,       '?',         A_COMMAND,
/*0x38*/ A_SHIFT,    A_CAPSLOCK,     A_ALT,      A_CTRL,         '?',         '?',         '?',         '?',
/*0x40*/ '?',     A_KP_PERIOD,    '?',     A_MULTIPLY,     '?',         A_KP_PLUS,      '?',         A_NUMLOCK,
/*0x48*/ '?',     '?',         '?',     A_DIVIDE,       A_KP_ENTER,     '?',         A_KP_MINUS,     '?',
/*0x50*/ '?',     A_KP_EQUALS,    A_KP_0,     A_KP_1,         A_KP_2,         A_KP_3,         A_KP_4,         A_KP_5,
/*0x58*/ A_KP_6,     A_KP_7,         '?',     A_KP_8,         A_KP_9,         '?',         '?',         '?',
/*0x60*/ A_F5,       A_F6,           A_F7,       A_F3,           A_F8,           A_F9,           '?',         A_F11,
/*0x68*/ '?',     A_PRINTSCREEN,  '?',     A_SCROLLLOCK,   '?',         A_F10,          '?',         A_F12,
/*0x70*/ '?',     A_PAUSE,        A_INSERT,   A_HOME,         A_PAGE_UP,      A_DELETE,       A_F4,           A_END,
/*0x78*/ A_F2,       A_PAGE_DOWN,    A_F1,       A_CURSOR_LEFT,  A_CURSOR_RIGHT, A_CURSOR_DOWN,  A_CURSOR_UP,    '?'
};

// A -> Q, Q -> A
// Z -> W, W -> Z
// - -> )
// ] -> $
// [ -> ^
// ' -> `
// ; -> M
// \ -> '
// , -> ;
// / -> -
// M -> ,
// . -> :
int vkeyToQuakeKey_French[256] = {
//			0 or 8		1 or 9			2 or A		3 or B			4 or C			5 or D			6 or E			7 or F
/*0x00*/ A_CAP_Q,    A_CAP_S,        A_CAP_D,    A_CAP_F,        A_CAP_H,        A_CAP_G,        A_CAP_W,        A_CAP_X,
/*0x08*/ A_CAP_C,    A_CAP_V,        A_DIVIDE,   A_CAP_B,        A_CAP_A,        A_CAP_Z,        A_CAP_E,        A_CAP_R,
/*0x10*/ A_CAP_Y,    A_CAP_T,        A_1,        A_2,            A_3,            A_4,            A_6,            A_5,
/*0x18*/ '=',     A_9,            A_7,        ')',         A_8,            A_0,            '$',         A_CAP_O,
/*0x20*/ A_CAP_U,    A_CARET,        A_CAP_I,    A_CAP_P,        A_ENTER,        A_CAP_L,        A_CAP_J,        A_CAP_UGRAVE,
/*0x28*/ A_CAP_K,    A_CAP_M,        A_LEFT_SINGLE_QUOTE, ';',    '-',         A_CAP_N,        ',',         ':',
/*0x30*/ A_TAB,      A_SPACE,        A_CONSOLE,  A_BACKSPACE,    '?',         A_ESCAPE,       '?',         A_COMMAND,
/*0x38*/ A_SHIFT,    A_CAPSLOCK,     A_ALT,      A_CTRL,         '?',         '?',         '?',         '?',
/*0x40*/ '?',     A_KP_PERIOD,    '?',     A_MULTIPLY,     '?',         A_KP_PLUS,      '?',         A_NUMLOCK,
/*0x48*/ '?',     '?',         '?',     A_DIVIDE,       A_KP_ENTER,     '?',         A_KP_MINUS,     '?',
/*0x50*/ '?',     A_KP_EQUALS,    A_KP_0,     A_KP_1,         A_KP_2,         A_KP_3,         A_KP_4,         A_KP_5,
/*0x58*/ A_KP_6,     A_KP_7,         '?',     A_KP_8,         A_KP_9,         '?',         '?',         '?',
/*0x60*/ A_F5,       A_F6,           A_F7,       A_F3,           A_F8,           A_F9,           '?',         A_F11,
/*0x68*/ '?',     A_PRINTSCREEN,  '?',     A_SCROLLLOCK,   '?',         A_F10,          '?',         A_F12,
/*0x70*/ '?',     A_PAUSE,        A_INSERT,   A_HOME,         A_PAGE_UP,      A_DELETE,       A_F4,           A_END,
/*0x78*/ A_F2,       A_PAGE_DOWN,    A_F1,       A_CURSOR_LEFT,  A_CURSOR_RIGHT, A_CURSOR_DOWN,  A_CURSOR_UP,    '?'
};

// Z -> Y, Y -> Z
// = -> '
// - -> §
// ] -> +
// [ -> Ÿ
// ' -> Š
// ; -> š
// \ -> #
// / -> -
int vkeyToQuakeKey_German[256] = {
//			0 or 8		1 or 9			2 or A		3 or B			4 or C			5 or D			6 or E			7 or F
/*0x00*/ A_CAP_A,    A_CAP_S,        A_CAP_D,    A_CAP_F,        A_CAP_H,        A_CAP_G,        A_CAP_Y,        A_CAP_X,
/*0x08*/ A_CAP_C,    A_CAP_V,        A_DIVIDE,   A_CAP_B,        A_CAP_Q,        A_CAP_W,        A_CAP_E,        A_CAP_R,
/*0x10*/ A_CAP_Z,    A_CAP_T,        A_1,        A_2,            A_3,            A_4,            A_6,            A_5,
/*0x18*/ '\'',     A_9,            A_7,        A_GERMANDBLS,   A_8,            A_0,            '+',         A_CAP_O,
/*0x20*/ A_CAP_U,    A_CAP_UDIERESIS,A_CAP_I,    A_CAP_P,        A_ENTER,        A_CAP_L,        A_CAP_J,        A_CAP_ADIERESIS,
/*0x28*/ A_CAP_K,    A_CAP_ODIERESIS,'#',     ',',         '-',         A_CAP_N,        A_CAP_M,        '.',
/*0x30*/ A_TAB,      A_SPACE,        A_CONSOLE,  A_BACKSPACE,    '?',         A_ESCAPE,       '?',         A_COMMAND,
/*0x38*/ A_SHIFT,    A_CAPSLOCK,     A_ALT,      A_CTRL,         '?',         '?',         '?',         '?',
/*0x40*/ '?',     A_KP_PERIOD,    '?',     A_MULTIPLY,     '?',         A_KP_PLUS,      '?',         A_NUMLOCK,
/*0x48*/ '?',     '?',         '?',     A_DIVIDE,       A_KP_ENTER,     '?',         A_KP_MINUS,     '?',
/*0x50*/ '?',     A_KP_EQUALS,    A_KP_0,     A_KP_1,         A_KP_2,         A_KP_3,         A_KP_4,         A_KP_5,
/*0x58*/ A_KP_6,     A_KP_7,         '?',     A_KP_8,         A_KP_9,         '?',         '?',         '?',
/*0x60*/ A_F5,       A_F6,           A_F7,       A_F3,           A_F8,           A_F9,           '?',         A_F11,
/*0x68*/ '?',     A_PRINTSCREEN,  '?',     A_SCROLLLOCK,   '?',         A_F10,          '?',         A_F12,
/*0x70*/ '?',     A_PAUSE,        A_INSERT,   A_HOME,         A_PAGE_UP,      A_DELETE,       A_F4,           A_END,
/*0x78*/ A_F2,       A_PAGE_DOWN,    A_F1,       A_CURSOR_LEFT,  A_CURSOR_RIGHT, A_CURSOR_DOWN,  A_CURSOR_UP,    '?'
};
#endif

#if 0
int asciiToQuakeKey[256] = {
//			0 or 8			1 or 9			2 or A			3 or B			4 or C			5 or D			6 or E			7 or F
/*0x00*/ -1,             -1,             -1,             -1,             -1,             -1,             -1,             -1,
/*0x08*/ -1,             -1,             -1,             -1,             -1,             -1,             -1,             -1,
/*0x10*/ -1,             -1,             -1,             -1,             -1,             -1,             -1,             -1,
/*0x18*/ -1,             -1,             -1,             -1,             -1,             -1,             -1,             -1,
/*0x20*/ A_SPACE,        A_PLING,        A_DOUBLE_QUOTE, A_HASH,         A_STRING,       A_PERCENT,      A_AND,          A_SINGLE_QUOTE,
/*0x28*/ A_OPEN_BRACKET, A_CLOSE_BRACKET, A_STAR,        A_PLUS,         A_COMMA,        A_MINUS,        A_PERIOD,       A_FORWARD_SLASH,
/*0x30*/ A_0,            A_1,            A_2,            A_3,            A_4,            A_5,            A_6,            A_7,
/*0x38*/ A_8,            A_9,            A_COLON,        A_SEMICOLON,    A_LESSTHAN,     A_EQUALS,       A_GREATERTHAN,  A_QUESTION,
/*0x40*/ A_AT,           A_CAP_A,        A_CAP_B,        A_CAP_C,        A_CAP_D,        A_CAP_E,        A_CAP_F,        A_CAP_G,
/*0x48*/ A_CAP_H,        A_CAP_I,        A_CAP_J,        A_CAP_K,        A_CAP_L,        A_CAP_M,        A_CAP_N,        A_CAP_O,
/*0x50*/ A_CAP_P,        A_CAP_Q,        A_CAP_R,        A_CAP_S,        A_CAP_T,        A_CAP_U,        A_CAP_V,        A_CAP_W,
/*0x58*/ A_CAP_X,        A_CAP_Y,        A_CAP_Z,        A_OPEN_SQUARE,  A_BACKSLASH,    A_CLOSE_SQUARE, A_CARET,        A_UNDERSCORE,
/*0x60*/ A_LEFT_SINGLE_QUOTE,A_CAP_A,    A_CAP_B,        A_CAP_C,        A_CAP_D,        A_CAP_E,        A_CAP_F,        A_CAP_G,
/*0x68*/ A_CAP_H,        A_CAP_I,        A_CAP_J,        A_CAP_K,        A_CAP_L,        A_CAP_M,        A_CAP_N,        A_CAP_O,
/*0x70*/ A_CAP_P,        A_CAP_Q,        A_CAP_R,        A_CAP_S,        A_CAP_T,        A_CAP_U,        A_CAP_V,        A_CAP_W,
/*0x78*/ A_CAP_X,        A_CAP_Y,        A_CAP_Z,        A_OPEN_BRACE,   A_BAR,          A_CLOSE_BRACE,  A_TILDE,        A_DELETE,

/*0x80*/ A_CAP_ADIERESIS,    A_CAP_ARING,        A_CAP_CCEDILLA,     A_CAP_EACUTE,   A_CAP_NTILDE,   A_CAP_ODIERESIS,    A_CAP_UDIERESIS,    A_LOW_AACUTE,
/*0x88*/ A_LOW_AGRAVE,       A_LOW_ACIRCUMFLEX,  A_LOW_ADIERESIS,    A_LOW_ATILDE,   A_LOW_ARING,    A_LOW_CCEDILLA,     A_LOW_EACUTE,       A_LOW_EGRAVE,
/*0x90*/ A_LOW_ECIRCUMFLEX,  A_LOW_EDIERESIS,    A_LOW_IACUTE,       A_LOW_IGRAVE,   A_LOW_ICIRCUMFLEX,A_LOW_IDIERESIS,  A_LOW_NTILDE,       A_LOW_OACUTE,
/*0x98*/ A_LOW_OGRAVE,       A_LOW_OCIRCUMFLEX,  A_LOW_ODIERESIS,    A_LOW_OTILDE,   A_LOW_UACUTE,   A_LOW_UGRAVE,       A_LOW_UCIRCUMFLEX,  A_LOW_UDIERESIS,
/*0xa0*/ -1,                 -1,                 -1,                 -1,             -1,             -1,                 -1,                 -1,
/*0xa8*/ -1,                 -1,                 -1,                 -1,             -1,             -1,                 -1,                 -1,
/*0xb0*/ -1,                 -1,                 -1,                 -1,             -1,             -1,                 -1,                 -1,
/*0xc8*/ -1,                 -1,                 -1,                 -1,             -1,             -1,                 -1,                 -1,
};
#endif

Boolean GenerateCharEvent( int inModifiers, int inKeyCode ) {
	// This routine returns true if the modifier/key combo should generate an additional
	// SE_CHAR event for the Q3 core. We use it to block combos of keys that would
	// otherwise print an extraneous character, like Cmd-V (it would print a 'v').

	if ( inModifiers & cmdKey ) {
		// Cmd-V (Paste)
		if ( inKeyCode == 0x09 ) {
			return false;
		}
	}
	return true;
}

void Mac_GenerateKeyEvent( int inTime, int inModifiers, int inCharCode, int inKeyCode, Boolean inIsKeyDown ) {
	int vKey;

#if 1
	#if MAC_GERMAN
	vKey = vkeyToQuakeKey_German[inKeyCode];
	#elif MAC_FRENCH
	vKey = vkeyToQuakeKey_French[inKeyCode];
	#else
	vKey = vkeyToQuakeKey[inKeyCode];
	#endif
#else
	if ( mac_keyboardScript == verGermany ) {
		vKey = vkeyToQuakeKey_German[inKeyCode];
	} else if ( mac_keyboardScript == verFrance ) {
		vKey = vkeyToQuakeKey_French[inKeyCode];
	} else {
		vKey = vkeyToQuakeKey[inKeyCode];
	}
#endif

	if ( inIsKeyDown ) {
		Sys_QueEvent( inTime, SE_KEY, vKey, 1, 0, NULL );
#if MAC_Q3_OLDSTUFF
		if ( GenerateCharEvent( inModifiers, inKeyCode ) && ( vKey != '`' ) )
#else
		if ( GenerateCharEvent( inModifiers, inKeyCode ) && ( vKey != A_CONSOLE ) )
#endif
		{
			Sys_QueEvent( inTime, SE_CHAR, inCharCode, 0, 0, NULL );
		}
	} else
	{
		Sys_QueEvent( inTime, SE_KEY, vKey, 0, 0, NULL );
	}
}

void DoKeyDown( EventRecord *event ) {
	int charCode;
	int keyCode;

	if ( event->modifiers & cmdKey ) {
		UInt32 menuCmd = MenuEvent( event );
		if ( HiWord( menuCmd ) != 0 ) {
			DoMenuCommand( menuCmd );
			return;
		}
	}

	charCode = BitAnd( event->message,charCodeMask );
	keyCode = ( event->message & keyCodeMask ) >> 8;

	Mac_GenerateKeyEvent( Sys_MsecForMacEvent(), event->modifiers, charCode, keyCode, true );
}

void DoKeyUp( EventRecord *event ) {
	int charCode;
	int keyCode;

	charCode = BitAnd( event->message,charCodeMask );
	keyCode = ( event->message & keyCodeMask ) >> 8;

	Mac_GenerateKeyEvent( Sys_MsecForMacEvent(), event->modifiers, charCode, keyCode, false );
}

/*
==================
Sys_ModifierEvents
==================
*/
static void Sys_ModifierEvents( int modifiers ) {
	static int oldModifiers = 0;
	static int prevPause = 0;
	static qboolean prevFullscreen = qtrue;
	int changed;
	int i;
	qboolean fullscreen = qtrue;

	typedef struct
	{
		int bit;
		int keyCode;
	} modifierKey_t;

	static modifierKey_t keys[] =
	{
#if MAC_Q3_OLDSTUFF
		{ 128, K_MOUSE1 },
		{ 256, K_COMMAND },
		{ 512, K_SHIFT },
		{1024, K_CAPSLOCK },
		{2048, K_ALT },
		{4096, K_CTRL },
#else
		{ 128, A_MOUSE1 },
		{ 256, A_COMMAND },
		{ 512, A_SHIFT },
		{1024, A_CAPSLOCK },
		{2048, A_ALT },
		{4096, A_CTRL },
#endif
		{-1, -1 }
	};

#if MAC_Q3_MP
	// LBO 12/29/03 - 1.11a patch. We need to ignore key/mouse events when running
	// as a dedicated server.
	if ( com_dedicated->integer ) {
		return;
	}
#endif

#if MAC_STVEF
	if ( cls.uiStarted && ue.UI_GetActiveMenu ) {
		ue.UI_GetActiveMenu( NULL, &fullscreen );
	}
#endif

#if !MAC_Q3_OLDSTUFF
	if ( ( cl_paused->integer == 0 && prevPause == 1 ) || ( fullscreen == qfalse && prevFullscreen == qtrue ) ) {
		changed = modifiers;
	} else
#endif
	{
		changed = modifiers ^ oldModifiers;
	}

	for ( i = 0 ; keys[i].bit != -1 ; i++ )
	{
#if !DEDICATED
		// if we have input sprockets running, ignore mouse events we
		// get from the debug passthrough driver
#if MAC_Q3_OLDSTUFF
		if ( inputActive && keys[i].keyCode == K_MOUSE1 )
#else
		if ( inputActive && keys[i].keyCode == A_MOUSE1 )
#endif
		{
			continue;
		}
#endif

		if ( changed & keys[i].bit ) {
			Sys_QueEvent( Sys_MsecForMacEvent(),  SE_KEY, keys[i].keyCode, !!( modifiers & keys[i].bit ), 0, NULL );
		}
	}

	oldModifiers = modifiers;
#if !MAC_Q3_OLDSTUFF
	prevPause = cl_paused->integer;
	prevFullscreen = fullscreen;
#endif
}


static void DoDiskEvent( EventRecord *event ) {

}

static void DoOSEvent( EventRecord   *event ) {

}

static void DoUpdate( WindowRef myWindow ) {
#if !DEDICATED
	GrafPtr origPort;

	GetPort( &origPort );
	SetPortWindowPort( myWindow );

	BeginUpdate( myWindow );
	EndUpdate( myWindow );

	aglUpdateContext( aglGetCurrentContext() );

	SetPort( origPort );
#endif
}

static void DoActivate( WindowRef myWindow, int myModifiers ) {

}

static void OptionsMenu_ToggleMouse( MenuRef inMenu, MenuItemIndex inItem ) {
	inputMouseInWindow = !inputMouseInWindow;
	CheckMenuItem( inMenu, inItem, inputMouseInWindow );
}

static void OptionsMenu_ToggleWindow( MenuRef inMenu, MenuItemIndex inItem ) {
	if ( ( cls.state == CA_ACTIVE ) ||
		 ( cls.state == CA_CONNECTED ) ||
		 ( cls.state == CA_DISCONNECTED ) ) {
		if ( r_fullscreen->integer == 1 ) {
			Cbuf_AddText( "seta r_fullscreen \"0\"\n" );
		} else {
			Cbuf_AddText( "seta r_fullscreen \"1\"\n" );
		}
		Cbuf_AddText( "vid_restart\n" );
	}
}

void DoMenuCommand( long menuAndItem ) {
	MenuID myMenuNum;
	MenuItemIndex myItemNum;
	int myResult;
	Str255 myDAName;
	MenuRef menu;

	myMenuNum   = HiWord( menuAndItem );
	myItemNum   = LoWord( menuAndItem );
	menu = GetMenuHandle( myMenuNum );

	switch ( myMenuNum )
	{
	case mApple:
		switch ( myItemNum )
		{
		case iAbout:
//					DoAboutBox ();
			break;
		}
		break;
	case mFile:
		switch ( myItemNum )
		{
		case iQuit:
			Com_Quit_f();
			break;
		}
		break;
	case mOptions:
		switch ( myItemNum )
		{
		case iMouse:
			OptionsMenu_ToggleMouse( menu, myItemNum );
			break;
		case iHideWindow:
			OptionsMenu_ToggleWindow( menu, myItemNum );
			break;
		}
		break;
	}

	HiliteMenu( 0 );
}

void TestTime( EventRecord *ev ) {
	int msec;
	int tics;
	static int startTics, startMsec;

	msec = Sys_Milliseconds();
	tics = ev->when;

	if ( !startTics || ev->what == mouseDown ) {
		startTics = tics;
		startMsec = msec;
	}

	msec -= startMsec;
	tics -= startTics;

	if ( !tics ) {
		return;
	}
	Com_Printf( "%i msec to tic\n", msec / tics );
}

static MPCriticalRegionID ECritID = NULL;
/*
==================
Sys_SendKeyEvents
==================
*/
void Sys_SendKeyEvents( void ) {
	Boolean gotEvent;
	EventRecord event;
	static RgnHandle cursRgn;

	// Under 10.0 and up, we use Carbon events to do all this
	if ( gUseCarbonEvents ) {
		return;
	}

	if ( cursRgn == NULL ) {
		cursRgn = NewRgn();
	}

#if MAC_ALICE && !MAC_MILES
	{
		// MoviesTask can be expensive. Only call Update() every 20 ticks at most. - jb, 6/29/00
		static UInt32 lastMovieTime;
		UInt32 nowTime = TickCount();

		if ( lastMovieTime + 30 < nowTime ) {
			lastMovieTime = nowTime;
			MUSIC_CheckLooping();
			MoviesTask( NULL, 0 );
		}
	}
#endif

	if ( ECritID == NULL ) {
		MPCreateCriticalRegion( &ECritID );
	}
	if ( ECritID ) {
		MPEnterCriticalRegion( ECritID, kDurationForever );   //DAJ

	}
	SetEventMask( -1 );

//Com_Printf ("Sys_SendKeyEvents\n");

	if ( !glConfig.isFullscreen ) {
		// this call involves 68k code and task switching.
		// do it on the desktop, or if they explicitly ask for
		// it when fullscreen
		gotEvent = WaitNextEvent( everyEvent, &event, 0, cursRgn );
	}

	{
		gotEvent = GetNextEvent( everyEvent, &event );
	}
	if ( ECritID ) {
		MPExitCriticalRegion( ECritID );
	}

	// generate faked events from modifer changes
	Sys_ModifierEvents( event.modifiers );

	sys_lastEventTic = event.when;

	if ( !gotEvent ) {
		return;
	}
//Com_Printf ("got event\n");

	if ( Sys_ConsoleEvent( &event, cursRgn ) ) {
		return;
	}

	#if _DEBUG
//	Com_Printf ("dispatching event: %d\n", event.what);
	#endif

	switch ( event.what )
	{
	case mouseDown:
		DoMouseDown( &event );
		break;
	case mouseUp:
		DoMouseUp( &event );
		break;
	case keyDown:
		DoKeyDown( &event );
		break;
	case keyUp:
		DoKeyUp( &event );
		break;
	case autoKey:
		DoKeyDown( &event );
		break;
	case updateEvt:
		DoUpdate( (WindowRef) event.message );
		break;
	case diskEvt:
		DoDiskEvent( &event );
		break;
	case activateEvt:
		DoActivate( (WindowRef) event.message, event.modifiers );
		break;
	case osEvt:
		DoOSEvent( &event );
		break;
	default:
		break;
	}

	#if _DEBUG
//	Com_Printf ("done dispatching\n");
	#endif
}

#if TARGET_API_MAC_CARBON
static pascal OSStatus appEventHandler( EventHandlerCallRef handlerChain, EventRef event, void *userData ) {
	UInt32 event_kind;
	UInt32 event_class;
	OSStatus err = eventNotHandledErr;
	UInt32 modifiers;
//	WindowRef eventWindow;

	event_kind = GetEventKind( event );
	event_class = GetEventClass( event );

//	(void)GetEventParameter (event, kEventParamWindowRef, typeWindowRef, NULL, sizeof (WindowRef), NULL, &eventWindow);

	if ( event_class == kEventClassKeyboard ) {
		char charCode;
		UInt32 keyCode;
		EventTime eventTimeInMsecs;

#if !DEDICATED
		// Raw keyboard events will take priority over MLTE text input events.
		// Therefore, if the console window is in front, pass any keyboard events on.
		if ( ConsoleWindowIsFrontmost() ) {
			return eventNotHandledErr;
		}
#endif

		(void)GetEventParameter( event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof( charCode ), NULL, &charCode );
		(void)GetEventParameter( event, kEventParamKeyCode, typeUInt32, NULL, sizeof( keyCode ), NULL, &keyCode );
		(void)GetEventParameter( event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof( modifiers ), NULL, &modifiers );

		// Get the event time in milliseconds
		eventTimeInMsecs = GetEventTime( event ) * 1000.0;

		switch ( event_kind )
		{
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
			Mac_GenerateKeyEvent( eventTimeInMsecs, modifiers, charCode, keyCode, true );
			Sys_ModifierEvents( modifiers );
			err = noErr;
			break;

		case kEventRawKeyUp:
			Mac_GenerateKeyEvent( eventTimeInMsecs, modifiers, charCode, keyCode, false );
			Sys_ModifierEvents( modifiers );
			err = noErr;
			break;

		case kEventRawKeyModifiersChanged:
			Sys_ModifierEvents( modifiers );
			err = noErr;
			break;
		}
	} else if ( event_class == kEventClassApplication )     {
		switch ( event_kind )
		{
		case kEventAppActivated:
			err = noErr;
			break;
		case kEventAppDeactivated:
			err = noErr;
			break;
		}
	}
#if !DEDICATED
	else if ( event_class == kEventClassCommand ) {
		HICommand cmd;

		(void)GetEventParameter( event, kEventParamDirectObject, typeHICommand, NULL, sizeof( cmd ), NULL, &cmd );
		switch ( event_kind )
		{
		case kEventCommandProcess:
			switch ( cmd.commandID )
			{
			case 'Qmou':
				OptionsMenu_ToggleMouse( cmd.menu.menuRef, cmd.menu.menuItemIndex );
				err = noErr;
				break;
			case 'Qwin':
				OptionsMenu_ToggleWindow( cmd.menu.menuRef, cmd.menu.menuItemIndex );
				err = noErr;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
#endif

	return err;
}

static pascal OSStatus serviceEventHandler( EventHandlerCallRef handlerChain, EventRef inEvent, void *userData ) {
	OSStatus err = eventNotHandledErr;

	CFStringRef message;
	ScrapRef specificScrap;
	Size byteCount;
	OSStatus status;
	char *command;
	char *buffer;

	GetEventParameter( inEvent, kEventParamServiceMessageName, typeCFStringRef, NULL, sizeof( CFStringRef ), NULL, &message );
	GetEventParameter( inEvent, kEventParamScrapRef, typeScrapRef, NULL, sizeof( ScrapRef ), NULL, &specificScrap );

	status = GetScrapFlavorSize( specificScrap, 'TEXT', &byteCount );
	if ( status == noErr ) {
		// Allocate room for the string plus the trailing NULL and set it all to zero
		buffer = (char *) calloc( byteCount + 1, 1 );
		if ( !buffer ) {
			goto bail;
		}

		// Get the 'TEXT' clipping from the service
		err = GetScrapFlavorData( specificScrap, 'TEXT', &byteCount, (void *)buffer );
	}

	if ( CFEqual( message, CFSTR( "connectToServer" ) ) ) {
		command = (char *)calloc( byteCount + 9, 1 );
		if ( !command ) {
			goto bail;
		}

		sprintf( command, "connect %s", buffer );
	} else
	{
		command = (char *)calloc( byteCount + 1, 1 );
		if ( !command ) {
			goto bail;
		}

		strcpy( command, buffer );
	}

#if !DEDICATED // ¥¥¥
	Sys_SendStringToConsole( command );

	Com_Printf( "]%s (via OSX Service)\n", command );

#if !MAC_Q3_OLDSTUFF
	strcpy( kg.g_consoleField.buffer, command );
	kg.historyEditLines[kg.nextHistoryLine % COMMAND_HISTORY] = kg.g_consoleField;
	kg.nextHistoryLine++;
	kg.historyLine = kg.nextHistoryLine;
#endif
#endif

	free( command );

	// We handled the event, mark it as such
	err = noErr;

bail:
	return err;
}

extern cvar_t *sys_profile;
extern Boolean gMoviePlaying;
pascal void carbonTimerCallback( EventLoopTimerRef timer, void *userData ) {
	static UInt16 count;
	GrafPtr savePort;

	// Timers are called at funky times, like when a menu is down.
	// We save the port (per the notes in UH 3.4) to keep things sane.
	GetPort( &savePort );

	count++;

#if MAC_ALICE && !MAC_MILES
	// Give QuickTime some time every 250  milliseconds (4 times a second)
	if ( count >= 250 ) {
		MUSIC_CheckLooping();
		MoviesTask( NULL, 0 );
		count = 0;
	}

	if ( gMoviePlaying ) {
		goto bail;
	}
#endif

	// run the frame
	Com_Frame();

#if __profile__
	if ( sys_profile->modified ) {
		sys_profile->modified = qfalse;
		if ( sys_profile->integer ) {
			Com_Printf( "Beginning profile.\n" );
			Sys_BeginProfiling();
		} else
		{
			Com_Printf( "Ending profile.\n" );
			Sys_EndProfiling();
		}
	}
#endif

bail:
	SetPort( savePort );
//	if (!alive)
//		QuitApplicationEventLoop();
}

void Carbon_InstallTimer( void ) {
	if ( sTimerRef ) {
		return;
	}

	if ( !sTimerUPP ) {
		sTimerUPP = NewEventLoopTimerUPP( carbonTimerCallback );
	}

	// We want to fire off every millisecond
	InstallEventLoopTimer( GetCurrentEventLoop(), NULL, kEventDurationMillisecond * 1,
						   sTimerUPP, NULL, &sTimerRef );
}

void Carbon_RemoveTimer( void ) {
	OSStatus status;

	if ( sTimerRef ) {
		status = RemoveEventLoopTimer( sTimerRef );
		sTimerRef = NULL;
	}
	if ( sTimerUPP ) {
		DisposeEventLoopTimerUPP( sTimerUPP );
		sTimerUPP = NULL;
	}
}

void Carbon_InstallServiceEvents( void ) {
	EventTypeSpec serviceEventList[] =
	{
		{ kEventClassService, kEventServicePerform }
	};

	InstallApplicationEventHandler( NewEventHandlerUPP( serviceEventHandler ), 1, serviceEventList, 0, NULL );
}

void Carbon_InstallEvents( void ) {
	EventTypeSpec appEventList[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		{ kEventClassCommand, kEventCommandProcess }
	};

	InstallApplicationEventHandler( NewEventHandlerUPP( appEventHandler ), sizeof( appEventList ) / sizeof( EventTypeSpec ), appEventList, 0, NULL );
#if MAC_Q3_MP
	Carbon_InstallServiceEvents();
#endif
}

#endif // TARGET_API_MAC_CARBON
