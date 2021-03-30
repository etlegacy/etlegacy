/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file sdl_input.c
 */

#include "sdl_defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __ANDROID__
#include <jni.h>
#endif

#include "../client/client.h"
#include "../sys/sys_local.h"

static cvar_t *in_keyboardDebug = NULL;

static cvar_t *in_mouse = NULL;
static cvar_t *in_nograb;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive    = qfalse;

static SDL_Joystick *stick                = NULL;
static cvar_t       *in_joystick          = NULL;
static cvar_t       *in_joystickThreshold = NULL;
static cvar_t       *in_joystickNo        = NULL;
static cvar_t       *in_joystickUseAnalog = NULL;

static int vidRestartTime = 0;
SDL_Window *mainScreen    = NULL;

// Used for giving the engine better (= unlagged) input timestamps
// We give the engine the time we last polled inputs as the timestamp to the current inputs.
// As long as the input backend doesn't have reliable timestamps, this is the right thing to do!
// The engine simulates *the period from the last frame's beginning to the current frame's beginning* when it simulates the current frame.
// It's not simulating "the next 1/60th or 1/125th" of a second. If you give it "now" as input timestamps, your inputs do not occur until the *next* simulated chunk of time.
// Try it. com_maxfps 1, press "forward", have fun.
static int lasttime = 0; // if 0, Com_QueueEvent will use the current time. This is for the first frame.

#define CTRL(a) ((a) - 'a' + 1)

/**
 * @brief IN_GetClipboardData
 * @return
 */
char *IN_GetClipboardData(void)
{
	if (SDL_HasClipboardText())
	{
		char   *data;
		char   *temp;
		size_t len = 0;

		temp = SDL_GetClipboardText();
		if (!temp || !temp[0])
		{
			return NULL;
		}

		//len = u8_strlen(temp) + 1;
		len = strlen(temp) + 1;

		data = Z_Malloc(len);

		//u8_escape(data, len, temp, qfalse);
		Q_strncpyz(data, temp, len);

		strtok(data, "\n\r\b");
		SDL_free(temp);

		return data;
	}
	else
	{
		return NULL;
	}
}

void IN_SetClipboardData(const char *text)
{
	SDL_SetClipboardText(text);
}

/**
 * @brief Check if Num key is lock down
 * @return qtrue if Num key is lock down, qfalse if not
 */
qboolean IN_IsNumLockDown(void)
{
#ifdef _WIN32
	if (GetKeyState(VK_NUMLOCK) & 0x01)
	{
		return qtrue;
	}
#else
	if (SDL_GetModState() & KMOD_NUM)
	{
		return qtrue;
	}
#endif

	return qfalse;
}

/**
 * @brief Prints keyboard identifiers in the console
 * @param[in] keysym
 * @param[in] key
 * @param[in] down
 */
static void IN_PrintKey(const SDL_Keysym *keysym, keyNum_t key, qboolean down)
{
	if (down)
	{
		Com_Printf("+ ");
	}
	else
	{
		Com_Printf("  ");
	}

	Com_Printf("Scancode: 0x%02x(%s) Sym: 0x%02x(%s)",
	           keysym->scancode, SDL_GetScancodeName(keysym->scancode),
	           keysym->sym, SDL_GetKeyName(keysym->sym));

	if (keysym->mod & KMOD_LSHIFT)
	{
		Com_Printf(" KMOD_LSHIFT");
	}
	if (keysym->mod & KMOD_RSHIFT)
	{
		Com_Printf(" KMOD_RSHIFT");
	}
	if (keysym->mod & KMOD_LCTRL)
	{
		Com_Printf(" KMOD_LCTRL");
	}
	if (keysym->mod & KMOD_RCTRL)
	{
		Com_Printf(" KMOD_RCTRL");
	}
	if (keysym->mod & KMOD_LALT)
	{
		Com_Printf(" KMOD_LALT");
	}
	if (keysym->mod & KMOD_RALT)
	{
		Com_Printf(" KMOD_RALT");
	}
	if (keysym->mod & KMOD_LGUI)
	{
		Com_Printf(" KMOD_LGUI");
	}
	if (keysym->mod & KMOD_RGUI)
	{
		Com_Printf(" KMOD_RGUI");
	}
	if (keysym->mod & KMOD_NUM)
	{
		Com_Printf(" KMOD_NUM");
	}
	if (keysym->mod & KMOD_CAPS)
	{
		Com_Printf(" KMOD_CAPS");
	}
	if (keysym->mod & KMOD_MODE)
	{
		Com_Printf(" KMOD_MODE");
	}
	if (keysym->mod & KMOD_RESERVED)
	{
		Com_Printf(" KMOD_RESERVED");
	}

	Com_Printf(" Q:0x%02x(%s)\n", key, Key_KeynumToString(key));
}

#define MAX_CONSOLE_KEYS 16

/**
 * @brief IN_IsConsoleKey
 * @param[in] key
 * @param[in] character
 * @return
 */
static qboolean IN_IsConsoleKey(keyNum_t key, int character)
{
	typedef struct consoleKey_s
	{
		enum
		{
			KEY,
			CHARACTER
		} type;

		union
		{
			keyNum_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[MAX_CONSOLE_KEYS];
	static int          numConsoleKeys = 0;
	int                 i;

	if (key == K_GRAVE
#ifdef __APPLE__
	    || key == 60 // Same as console key
#endif
	    )
	{
		return qtrue;
	}

	// Only parse the variable when it changes
	if (cl_consoleKeys->modified)
	{
		char *text_p, *token;

		cl_consoleKeys->modified = qfalse;
		text_p                   = cl_consoleKeys->string;
		numConsoleKeys           = 0;

		while (numConsoleKeys < MAX_CONSOLE_KEYS)
		{
			consoleKey_t *c       = &consoleKeys[numConsoleKeys];
			int          charCode = 0;

			token = COM_Parse(&text_p);
			if (!token[0])
			{
				break;
			}

			if (strlen(token) == 4)
			{
				charCode = Com_HexStrToInt(token);
			}

			if (charCode > 0)
			{
				c->type        = CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type  = KEY;
				c->u.key = Key_StringToKeynum(token);

				// 0 isn't a key
				if (c->u.key <= 0)
				{
					continue;
				}
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if (key == character)
	{
		key = 0;
	}

	for (i = 0; i < numConsoleKeys; i++)
	{
		consoleKey_t *c = &consoleKeys[i];

		switch (c->type)
		{
		case KEY:
			if (key && c->u.key == key)
			{
				return qtrue;
			}
			break;
		case CHARACTER:
			if (c->u.character == character)
			{
				return qtrue;
			}
			break;
		}
	}

	return qfalse;
}

/**

 */
/**
 * @brief Translates SDL keyboard identifier to its Q3 counterpart
 * @param[in] keysym
 * @param[in] down
 * @return
 */
static keyNum_t IN_TranslateSDLToQ3Key(SDL_Keysym *keysym, qboolean down)
{
	keyNum_t key = 0;

	if (keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE)
	{
		// These happen to match the ASCII chars
		key = (keyNum_t)keysym->sym;
	}
	else
	{
		switch (keysym->sym)
		{
		// keypad
		case SDLK_KP_0:
			key = K_KP_INS;
			break;
		case SDLK_KP_1:
			key = K_KP_END;
			break;
		case SDLK_KP_2:
			key = K_KP_DOWNARROW;
			break;
		case SDLK_KP_3:
			key = K_KP_PGDN;
			break;
		case SDLK_KP_4:
			key = K_KP_LEFTARROW;
			break;
		case SDLK_KP_5:
			key = K_KP_5;
			break;
		case SDLK_KP_6:
			key = K_KP_RIGHTARROW;
			break;
		case SDLK_KP_7:
			key = K_KP_HOME;
			break;
		case SDLK_KP_8:
			key = K_KP_UPARROW;
			break;
		case SDLK_KP_9:
			key = K_KP_PGUP;
			break;
		case SDLK_KP_ENTER:     key = K_KP_ENTER;
			break;
		case SDLK_KP_PERIOD:    key = K_KP_DEL;
			break;
		case SDLK_KP_MULTIPLY:  key = K_KP_STAR;
			break;
		case SDLK_KP_PLUS:      key = K_KP_PLUS;
			break;
		case SDLK_KP_MINUS:     key = K_KP_MINUS;
			break;
		case SDLK_KP_DIVIDE:    key = K_KP_SLASH;
			break;

		case SDLK_PAGEUP:       key = K_PGUP;
			break;
		case SDLK_PAGEDOWN:     key = K_PGDN;
			break;
		case SDLK_HOME:         key = K_HOME;
			break;
		case SDLK_END:          key = K_END;
			break;
		case SDLK_LEFT:         key = K_LEFTARROW;
			break;
		case SDLK_RIGHT:        key = K_RIGHTARROW;
			break;
		case SDLK_DOWN:         key = K_DOWNARROW;
			break;
		case SDLK_UP:           key = K_UPARROW;
			break;
		case SDLK_ESCAPE:       key = K_ESCAPE;
			break;
		case SDLK_RETURN:       key = K_ENTER;
			break;
		case SDLK_TAB:          key = K_TAB;
			break;
		case SDLK_F1:           key = K_F1;
			break;
		case SDLK_F2:           key = K_F2;
			break;
		case SDLK_F3:           key = K_F3;
			break;
		case SDLK_F4:           key = K_F4;
			break;
		case SDLK_F5:           key = K_F5;
			break;
		case SDLK_F6:           key = K_F6;
			break;
		case SDLK_F7:           key = K_F7;
			break;
		case SDLK_F8:           key = K_F8;
			break;
		case SDLK_F9:           key = K_F9;
			break;
		case SDLK_F10:          key = K_F10;
			break;
		case SDLK_F11:          key = K_F11;
			break;
		case SDLK_F12:          key = K_F12;
			break;
		case SDLK_F13:          key = K_F13;
			break;
		case SDLK_F14:          key = K_F14;
			break;
		case SDLK_F15:          key = K_F15;
			break;
		case SDLK_BACKSPACE:    key = K_BACKSPACE;
			break;

		case SDLK_DELETE:       key = K_DEL;
			break;
		case SDLK_PAUSE:        key = K_PAUSE;
			break; // FIXME: SDL 2.0 maps PAUSE to PAUSE as well as BREAK
		// (key = K_BREAK;         break;)
		case SDLK_LSHIFT:       key = K_LSHIFT;
			break;
		case SDLK_RSHIFT:       key = K_RSHIFT;
			break;
		case SDLK_LCTRL:        key = K_LCTRL;
			break;
		case SDLK_RCTRL:        key = K_RCTRL;
			break;

#ifdef __APPLE__
		case SDLK_RGUI:
		case SDLK_LGUI:         key = K_COMMAND;
			break;
#else
		case SDLK_RGUI:
		case SDLK_LGUI:         key = K_SUPER;
			break;
#endif

		case SDLK_RALT:         key = K_RALT;
			break;
		case SDLK_LALT:         key = K_LALT;
			break;

		case SDLK_INSERT:       key = K_INS;
			break;

		case SDLK_MODE:         key = K_MODE;
			break;
		case SDLK_APPLICATION:  key = K_COMPOSE;     // map to K_MENU ?
			break;
		case SDLK_HELP:         key = K_HELP;
			break;
		case SDLK_PRINTSCREEN:  key = K_PRINT;
			break;
		case SDLK_SYSREQ:       key = K_SYSREQ;
			break;
		case SDLK_MENU:         key = K_MENU;
			break;
		case SDLK_POWER:        key = K_POWER;
			break;
		case SDLK_CURRENCYUNIT: key = K_EURO;
			break;
		case SDLK_UNDO:         key = K_UNDO;
			break;
		case SDLK_SCROLLLOCK:   key = K_SCROLLOCK;
			break;
		case SDLK_NUMLOCKCLEAR: key = K_KP_NUMLOCK;
			break;
		case SDLK_CAPSLOCK:     key = K_CAPSLOCK;
			break;

		default:
			// physical key mapped to a non-ascii character
			switch (keysym->scancode)
			{
			case SDL_SCANCODE_1: key = K_1;
				break;
			case SDL_SCANCODE_2: key = K_2;
				break;
			case SDL_SCANCODE_3: key = K_3;
				break;
			case SDL_SCANCODE_4: key = K_4;
				break;
			case SDL_SCANCODE_5: key = K_5;
				break;
			case SDL_SCANCODE_6: key = K_6;
				break;
			case SDL_SCANCODE_7: key = K_7;
				break;
			case SDL_SCANCODE_8: key = K_8;
				break;
			case SDL_SCANCODE_9: key = K_9;
				break;
			case SDL_SCANCODE_0: key = K_0;
				break;

			case SDL_SCANCODE_MINUS: key = K_MINUS;
				break;
			case SDL_SCANCODE_EQUALS: key = K_EQUALS;
				break;
			case SDL_SCANCODE_LEFTBRACKET: key = K_LEFTBRACKET;
				break;
			case SDL_SCANCODE_RIGHTBRACKET: key = K_RIGHTBRACKET;
				break;
			case SDL_SCANCODE_BACKSLASH: key = K_BACKSLASH;
				break;
			case SDL_SCANCODE_SEMICOLON: key = K_SEMICOLON;
				break;
			case SDL_SCANCODE_APOSTROPHE: key = K_APOSTROPHE;
				break;
			case SDL_SCANCODE_GRAVE: key = K_GRAVE;              // NOTE: this is console toggle key!
				break;
			case SDL_SCANCODE_COMMA: key = K_COMMA;
				break;
			case SDL_SCANCODE_PERIOD: key = K_PERIOD;
				break;
			case SDL_SCANCODE_SLASH: key = K_SLASH;
				break;
			case SDL_SCANCODE_NONUSBACKSLASH: key = K_NONUSBACKSLASH;
				break;
#ifdef __ANDROID__
			case SDL_SCANCODE_AC_BACK: key = K_ESCAPE;
				break;
#endif

			default:
				break;
			}
			break;
		}
	}

	if (in_keyboardDebug->integer)
	{
		IN_PrintKey(keysym, key, down);
	}

	if (IN_IsConsoleKey(key, 0))
	{
		// Console keys can't be bound or generate characters
		key = CONSOLE_KEY;
	}

	return key;
}

/**
 * @brief IN_GobbleMotionEvents
 */
static void IN_GobbleMotionEvents(void)
{
	SDL_Event dummy[1];

	// Gobble any mouse motion events
	SDL_PumpEvents();
	while (SDL_PeepEvents(dummy, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION) > 0)
		;
}

/**
 * @brief IN_GrabMouse
 * @param[in] grab
 * @param[in] relative
 */
static void IN_GrabMouse(qboolean grab, qboolean relative)
{
	static qboolean mouse_grabbed   = qfalse, mouse_relative = qfalse;
#if !defined(__ANDROID__) || __ANDROID_API__ > 23
	int             relative_result = 0;
#endif

	if (relative == !mouse_relative)
	{
		SDL_ShowCursor(!relative);
#if !defined(__ANDROID__) || __ANDROID_API__ > 23
		// On Android Phones with API <= 23 this is causing App to close since it could not
		// set relative mouse location (Mouse location is always at top left side of screen)
		if ((relative_result = SDL_SetRelativeMouseMode((SDL_bool)relative)) != 0)
		{
			// FIXME: this happens on some systems (IR4)
			if (relative_result == -1)
			{
				Com_Error(ERR_FATAL, "Setting relative mouse location fails (system not supported)\n");
			}
			else
			{
				Com_Error(ERR_FATAL, "Setting relative mouse location fails: %s\n", SDL_GetError());
			}
		}
#endif
		mouse_relative = relative;
	}

	if (grab == !mouse_grabbed)
	{
		SDL_SetWindowGrab(mainScreen, (SDL_bool)grab);
		mouse_grabbed = grab;
	}
}

/**
 * @brief IN_ActivateMouse
 */
static void IN_ActivateMouse(void)
{
	if (!mouseAvailable || !SDL_WasInit(SDL_INIT_VIDEO))
	{
		return;
	}

	if (!mouseActive)
	{
		IN_GrabMouse(qtrue, qtrue);

		IN_GobbleMotionEvents();
	}

	// in_nograb makes no sense in fullscreen mode
	if (!cls.glconfig.isFullscreen)
	{
		if (in_nograb->modified || !mouseActive)
		{
			if (in_nograb->integer)
			{
				IN_GrabMouse(qfalse, qtrue);
			}
			else
			{
				IN_GrabMouse(qtrue, qtrue);
			}

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

/**
 * @brief IN_DeactivateMouse
 */
static void IN_DeactivateMouse(void)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		return;
	}

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if (!cls.glconfig.isFullscreen)
	{
#if 0
		if ((Key_GetCatcher() == KEYCATCH_UI) &&
		    screen == SDL_GetMouseFocus())
		{
			// TODO: (SDL_GetAppState() & SDL_APPMOUSEFOCUS))
			IN_GrabMouse(qtrue, qtrue);
		}
		else
#endif
		{
			IN_GrabMouse(qfalse, qfalse);
		}
	}

	if (!mouseAvailable)
	{
		return;
	}

	if (mouseActive)
	{
		IN_GobbleMotionEvents();
		IN_GrabMouse(qfalse, qfalse);

		// Don't warp the mouse unless the cursor is within the window
		if (SDL_GetWindowFlags(mainScreen) & SDL_WINDOW_MOUSE_FOCUS)
		{
			SDL_WarpMouseInWindow(mainScreen, cls.glconfig.vidWidth / 2, cls.glconfig.vidHeight / 2);
		}

		mouseActive = qfalse;
	}
}

// We translate axes movement into keypresses
static int joy_keys[16] =
{
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW,   K_DOWNARROW,
	K_JOY16,     K_JOY17,
	K_JOY18,     K_JOY19,
	K_JOY20,     K_JOY21,
	K_JOY22,     K_JOY23,

	K_JOY24,     K_JOY25,
	K_JOY26,     K_JOY27
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static int hat_keys[16] =
{
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};

struct
{
	qboolean buttons[16];  // !!! FIXME: these might be too many.
	unsigned int oldaxes;
	int oldaaxes[16];
	unsigned int oldhats;
} stick_state;

/**
 * @brief Prints joystick info to console
 */
void IN_PrintJoystickInfo_f(void)
{
	if (!in_joystick->integer)
	{
		Com_Printf("Joysticks disabled by cvar setting.\n");
		return;
	}

	if (SDL_NumJoysticks() > 0)
	{
		Com_Printf("Joystick [%d] '%s' opened - %i devices available\n", in_joystickNo->integer, SDL_JoystickNameForIndex(in_joystickNo->integer), SDL_NumJoysticks());
		Com_Printf("Axes:       %d\n", SDL_JoystickNumAxes(stick));
		Com_Printf("Hats:       %d\n", SDL_JoystickNumHats(stick));
		Com_Printf("Buttons:    %d\n", SDL_JoystickNumButtons(stick));
		Com_Printf("Balls:      %d\n", SDL_JoystickNumBalls(stick));
		Com_Printf("Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No");
	}
	else
	{
		Com_Printf("No joystick available.\n");
	}
}

/**
 * @brief Inits game controller input devices
 * @note This doesn't deal with SDL_INIT_GAMECONTROLLER
 */
static void IN_InitJoystick(void)
{
	//int  i     = 0;
	int  total = 0;
	//char buf[MAX_CVAR_VALUE_STRING] = "";

	Cmd_AddCommand("joystickInfo", IN_PrintJoystickInfo_f, "Prints joystick info."); // command is valid when in_joystick 0 is set

	if (!in_joystick->integer)
	{
		return;
	}

	if (stick != NULL)
	{
		SDL_JoystickClose(stick);
	}

	stick = NULL;
	Com_Memset(&stick_state, '\0', sizeof(stick_state));

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
		{
			Com_Printf(S_COLOR_RED "Joystick initialization failed: %s\n", SDL_GetError());
			return;
		}
	}

	total = SDL_NumJoysticks();

	// create list and build cvar to allow ui to select joystick
	// FIXME: cvar availableJoysticks isn't used in our menus - add it?
	//for (i = 0; i < total; i++)
	//{
	//	Q_strcat(buf, sizeof(buf), SDL_JoystickNameForIndex(i));
	//	Q_strcat(buf, sizeof(buf), " ");
	//}
	//Cvar_Get("in_availableJoysticks", buf, CVAR_ROM);

	in_joystickNo = Cvar_Get("in_joystickNo", "0", CVAR_ARCHIVE);
	if (in_joystickNo->integer < 0 || in_joystickNo->integer >= total)
	{
		Cvar_Set("in_joystickNo", "0");
	}

	in_joystickUseAnalog = Cvar_Get("in_joystickUseAnalog", "0", CVAR_ARCHIVE);

	stick = SDL_JoystickOpen(in_joystickNo->integer);

	if (stick == NULL)
	{
		Com_Printf(S_COLOR_RED "Joystick initialization failed: no device available.\n");
		return;
	}

	SDL_JoystickEventState(SDL_QUERY);
}

/**
 * @brief IN_ShutdownJoystick
 */
static void IN_ShutdownJoystick(void)
{
	Cmd_RemoveCommand("joystickInfo");

	// in_joystick cvar is latched
	if (!SDL_WasInit(SDL_INIT_JOYSTICK) || !in_joystick->integer)
	{
		return;
	}

	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/**
 * @brief IN_JoyMove
 */
static void IN_JoyMove(void)
{
	unsigned int axes  = 0;
	unsigned int hats  = 0;
	int          total = 0;
	int          i     = 0;

	// in_joystick cvar is latched
	if (!in_joystick->integer || !stick)
	{
		return;
	}

	SDL_JoystickUpdate();

	// update the ball state.
	total = SDL_JoystickNumBalls(stick);
	if (total > 0)
	{
		int balldx = 0;
		int balldy = 0;
		int dx;
		int dy;

		for (i = 0; i < total; i++)
		{
			dx = 0;
			dy = 0;

			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx += dx;
			balldy += dy;
		}
		if (balldx || balldy)
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if (abs(balldx) > 1)
			{
				balldx *= 2;
			}
			if (abs(balldy) > 1)
			{
				balldy *= 2;
			}
			Com_QueueEvent(lasttime, SE_MOUSE, balldx, balldy, 0, NULL);
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons(stick);
	if (total > 0)
	{
		if (total > ARRAY_LEN(stick_state.buttons))
		{
			total = ARRAY_LEN(stick_state.buttons);
		}
		for (i = 0; i < total; i++)
		{
			qboolean pressed = (SDL_JoystickGetButton(stick, i) != 0);

			if (pressed != stick_state.buttons[i])
			{
				Com_QueueEvent(lasttime, SE_KEY, K_JOY1 + i, pressed, 0, NULL);
				stick_state.buttons[i] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats(stick);
	if (total > 0)
	{
		if (total > 4)
		{
			total = 4;
		}
		for (i = 0; i < total; i++)
		{
			((Uint8 *)&hats)[i] = SDL_JoystickGetHat(stick, i);
		}
	}

	// update hat state
	if (hats != stick_state.oldhats)
	{
		for (i = 0; i < 4; i++)
		{
			if (((Uint8 *)&hats)[i] != ((Uint8 *)&stick_state.oldhats)[i])
			{
				// release event
				switch (((Uint8 *)&stick_state.oldhats)[i])
				{
				case SDL_HAT_UP:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 0], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHT:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_DOWN:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 2], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFT:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 3], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHTUP:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 0], qfalse, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHTDOWN:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 2], qfalse, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFTUP:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 0], qfalse, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 3], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFTDOWN:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 2], qfalse, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 3], qfalse, 0, NULL);
					break;
				default:
					break;
				}
				// press event
				switch (((Uint8 *)&hats)[i])
				{
				case SDL_HAT_UP:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 0], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHT:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_DOWN:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 2], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFT:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 3], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHTUP:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 0], qtrue, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHTDOWN:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 2], qtrue, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFTUP:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 0], qtrue, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 3], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFTDOWN:
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 2], qtrue, 0, NULL);
					Com_QueueEvent(lasttime, SE_KEY, hat_keys[4 * i + 3], qtrue, 0, NULL);
					break;
				default:
					break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes(stick);
	if (total > 0)
	{
		if (total > 16)
		{
			total = 16;
		}
		for (i = 0; i < total; i++)
		{
			Sint16 axis = SDL_JoystickGetAxis(stick, i);

			if (in_joystickUseAnalog->integer)
			{
				float f = abs(axis) / 32767.0f;

				if (f < in_joystickThreshold->value)
				{
					axis = 0;
				}

				if (axis != stick_state.oldaaxes[i])
				{
					Com_QueueEvent(lasttime, SE_JOYSTICK_AXIS, i, axis, 0, NULL);
					stick_state.oldaaxes[i] = axis;
				}
			}
			else
			{
				float f = axis / 32767.0f;

				if (f < -in_joystickThreshold->value)
				{
					axes |= (1 << (i * 2));
				}
				else if (f > in_joystickThreshold->value)
				{
					axes |= (1 << ((i * 2) + 1));
				}
			}
		}
	}

	// Time to update axes state based on old vs. new.
	if (axes != stick_state.oldaxes)
	{
		for (i = 0; i < 16; i++)
		{
			if ((axes & (1 << i)) && !(stick_state.oldaxes & (1 << i)))
			{
				Com_QueueEvent(lasttime, SE_KEY, joy_keys[i], qtrue, 0, NULL);
			}

			if (!(axes & (1 << i)) && (stick_state.oldaxes & (1 << i)))
			{
				Com_QueueEvent(lasttime, SE_KEY, joy_keys[i], qfalse, 0, NULL);
			}
		}
	}

	// Save for future generations.
	stick_state.oldaxes = axes;
}

/**
 * @brief IN_WindowResize
 * @param[in] e
 */
static void IN_WindowResize(SDL_Event *e)
{
	// Only do a vid_restart if the size of the window actually changed. On OS X at least, it's possible
	// to receive a resize event when the window simply moves, or even when Dock shows/hides. No need to
	// do a vid_restart then.
	if (!cls.glconfig.isFullscreen && (cls.glconfig.vidWidth != e->window.data1 || cls.glconfig.vidHeight != e->window.data2))
	{
		char width[32], height[32];

		Com_sprintf(width, sizeof(width), "%d", e->window.data1);
		Com_sprintf(height, sizeof(height), "%d", e->window.data2);
		Cvar_Set("r_customwidth", width);
		Cvar_Set("r_customheight", height);
		Cvar_Set("r_mode", "-1");
		// wait until user stops dragging for 1 second, so
		// we aren't constantly recreating the GL context while
		// he tries to drag...
		vidRestartTime = Sys_Milliseconds() + 1000;
	}
}

static void IN_WindowMoved(SDL_Event *e)
{
    int displayIndex = 0;
    displayIndex = SDL_GetWindowDisplayIndex(GLimp_MainWindow());
    if (displayIndex >= 0)
    {
        Cvar_Set("r_windowLocation", va("%d,%d,%d", displayIndex, e->window.data1, e->window.data2));
    }
}

/*
 * @brief IN_WindowFocusLost
 * @note checks if the renderer is actually running and if we are fullscreen
 */
static void IN_WindowFocusLost()
{
    if (cls.rendererStarted && cls.glconfig.isFullscreen)
    {
    	Com_DPrintf("Trying to minimize. Checking SDL flags.\n");
    	// If according to the game flags we should minimize,
    	// then lets actually make sure and ask the windowing system for its opinion.
		Uint32 flags = SDL_GetWindowFlags(GLimp_MainWindow());
		if (flags & SDL_WINDOW_FULLSCREEN && flags & SDL_WINDOW_SHOWN && !(flags & SDL_WINDOW_MINIMIZED) && flags & SDL_WINDOW_INPUT_GRABBED)
		{
			Com_DPrintf("SDL says we are good to go and call minimize.\n");
			Cbuf_ExecuteText(EXEC_NOW, "minimize");
		}
		else
		{
			Com_DPrintf("SDL did not think we should minimize, trashing event.\n");
			SDL_RaiseWindow(mainScreen);
			IN_ActivateMouse();
		}
    }
}

/**
 * @brief IN_ProcessEvents
 */
static void IN_ProcessEvents(void)
{
	SDL_Event       e;
	keyNum_t        key         = 0;
	static keyNum_t lastKeyDown = 0;
	qboolean 		skipLost	= qfalse;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		return;
	}

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_KEYDOWN:
			if (e.key.repeat && Key_GetCatcher() == 0)
			{
				break;
			}

			if ((key = IN_TranslateSDLToQ3Key(&e.key.keysym, qtrue)))
			{
				Com_QueueEvent(lasttime, SE_KEY, key, qtrue, 0, NULL);
				if (key == K_BACKSPACE)
				{
					// This was added to keep mod comp, mods do not check K_BACKSPACE but instead use char 8 which is backspace in ascii
					// 8 == CTRL('h') == BS aka Backspace from ascii table
					Com_QueueEvent(lasttime, SE_CHAR, CTRL('h'), 0, 0, NULL);
				}
				else if ((keys[K_LCTRL].down || keys[K_RCTRL].down) && key >= 'a' && key <= 'z')
				{
					Com_QueueEvent(lasttime, SE_CHAR, CTRL(key), 0, 0, NULL);
				}
			}
			lastKeyDown = key;
			break;
		case SDL_KEYUP:
			if ((key = IN_TranslateSDLToQ3Key(&e.key.keysym, qfalse)))
			{
				Com_QueueEvent(lasttime, SE_KEY, key, qfalse, 0, NULL);
			}
			lastKeyDown = 0;
			break;
		case SDL_TEXTINPUT:
			if (lastKeyDown != CONSOLE_KEY)
			{
				char *c = e.text.text;

				// Quick and dirty UTF-8 to UTF-32 conversion
				while (*c)
				{
					int utf32 = 0;

					if ((*c & 0x80) == 0)
					{
						utf32 = *c++;
					}
					else if ((*c & 0xE0) == 0xC0)    // 110x xxxx
					{
						utf32 |= (*c++ & 0x1F) << 6;
						utf32 |= (*c++ & 0x3F);
					}
					else if ((*c & 0xF0) == 0xE0)    // 1110 xxxx
					{
						utf32 |= (*c++ & 0x0F) << 12;
						utf32 |= (*c++ & 0x3F) << 6;
						utf32 |= (*c++ & 0x3F);
					}
					else if ((*c & 0xF8) == 0xF0)    // 1111 0xxx
					{
						utf32 |= (*c++ & 0x07) << 18;
						utf32 |= (*c++ & 0x3F) << 12;
						utf32 |= (*c++ & 0x3F) << 6;
						utf32 |= (*c++ & 0x3F);
					}
					else
					{
						Com_DPrintf("Unrecognised UTF-8 lead byte: 0x%x\n", (unsigned int)*c);
						c++;
					}

					if (utf32 != 0)
					{
						if (IN_IsConsoleKey(0, utf32))
						{
							Com_QueueEvent(lasttime, SE_KEY, CONSOLE_KEY, qtrue, 0, NULL);
							Com_QueueEvent(lasttime, SE_KEY, CONSOLE_KEY, qfalse, 0, NULL);
						}
						else
						{
							Com_QueueEvent(lasttime, SE_CHAR, utf32, 0, 0, NULL);
						}
					}
				}
			}
			break;
		case SDL_MOUSEMOTION:
			if (mouseActive)
			{
				if (!e.motion.xrel && !e.motion.yrel)
				{
					break;
				}
				Com_QueueEvent(lasttime, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			int b;

			switch (e.button.button)
			{
			case SDL_BUTTON_LEFT:      b = K_MOUSE1;
				break;
			case SDL_BUTTON_MIDDLE:    b = K_MOUSE3;
				break;
			case SDL_BUTTON_RIGHT:     b = K_MOUSE2;
				break;
			case SDL_BUTTON_X1:        b = K_MOUSE4;
				break;
			case SDL_BUTTON_X2:        b = K_MOUSE5;
				break;
			default:                   b = K_AUX1 + (e.button.button - SDL_BUTTON_X2 + 1) % 16;
				break;
			}
			Com_QueueEvent(lasttime, SE_KEY, b, (e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse), 0, NULL);
		}
		break;
		case SDL_MOUSEWHEEL:
			if (e.wheel.y > 0)
			{
				Com_QueueEvent(lasttime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL);
				Com_QueueEvent(lasttime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL);
			}
			else if (e.wheel.y < 0)
			{
				Com_QueueEvent(lasttime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL);
				Com_QueueEvent(lasttime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL);
			}

			break;
		case SDL_DROPFILE:
			if (!Q_strncmp(e.drop.file, "et://", 5))
			{
				Cbuf_AddText(va("connect \"%s\"", e.drop.file));
			}
			else if (FS_IsDemoExt(e.drop.file, -1))
			{
				Cbuf_AddText(va("demo \"%s\"", e.drop.file));
			}
			SDL_free(e.drop.file);
			break;
		case SDL_QUIT:
			Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
			break;
		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
			case SDL_WINDOWEVENT_RESIZED:
				IN_WindowResize(&e);
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				Cvar_SetValue("com_minimized", 1);
				break;
			case SDL_WINDOWEVENT_SHOWN:
			case SDL_WINDOWEVENT_RESTORED:
			case SDL_WINDOWEVENT_MAXIMIZED:
				Cvar_SetValue("com_minimized", 0);
				skipLost = qtrue;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				// When we are restoring a fullscreen window that is not the desktop resolution
				// the desktop will resize and SDL will get restores & focus gained & focus lost in order.
				// So.. to fix minimizing just after user re-selects the game, we will skip focus lost events
				// for just a SINGLE frame!
				if (skipLost)
				{
					break;
				}
				IN_WindowFocusLost();
			case SDL_WINDOWEVENT_LEAVE:
				Key_ClearStates();
				Cvar_SetValue("com_unfocused", 1);
				break;
			case SDL_WINDOWEVENT_ENTER:
			{
				Uint32 flags = SDL_GetWindowFlags(mainScreen);
				// We might not have focus, just the user moving his mouse over things
				if (!(flags & SDL_WINDOW_INPUT_FOCUS))
				{
					break;
				}
			}
			case SDL_WINDOWEVENT_FOCUS_GAINED:
			{
				Cvar_SetValue("com_unfocused", 0);
				if (com_minimized->integer)
				{
					SDL_RestoreWindow(mainScreen);
				}
				SDL_RaiseWindow(mainScreen);
			}
			break;
			case SDL_WINDOWEVENT_MOVED:
			    IN_WindowMoved(&e);
			    break;
			}
			break;
		default:
			break;
		}
	}
}

/**
 * @brief IN_Frame
 */
void IN_Frame(void)
{
	// If not DISCONNECTED (main menu), ACTIVE (in game) or CINEMATIC (playing video), we're loading
	qboolean loading   = (cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE);
	qboolean cinematic = (cls.state == CA_CINEMATIC);

	// Get the timestamp to give the next frame's input events (not the ones we're gathering right now, though)
	int start = Sys_Milliseconds();

#ifdef __ANDROID__

	JNIEnv *env = (JNIEnv*) SDL_AndroidGetJNIEnv();

	if (env == NULL)
		return;

	jobject activity = (jobject)SDL_AndroidGetActivity();

	if (activity == NULL)
		return;

	jclass clazz = (*env)->GetObjectClass(env, activity);

	if (clazz == NULL)
		return;

	jfieldID f_id = (*env)->GetStaticFieldID(env, clazz, "UiMenu", "Z");
	qboolean f_boolean = (*env)->GetStaticBooleanField(env, clazz, f_id);

	if (cls.state == CA_ACTIVE)
	{
		if (f_boolean != qtrue)
			(*env)->SetStaticBooleanField(env, clazz, f_id, qtrue);
	}
	else
	{
		if (f_boolean != qfalse)
			(*env)->SetStaticBooleanField(env, clazz, f_id, qfalse);
	}

	(*env)->DeleteLocalRef(env, clazz);
	(*env)->DeleteLocalRef(env, activity);

#endif // __ANDROID__

	if (!cls.glconfig.isFullscreen && (Key_GetCatcher() & KEYCATCH_CONSOLE))
	{
		// Console is down in windowed mode
		IN_DeactivateMouse();
	}
	else if (!cls.glconfig.isFullscreen && loading && !cinematic)
	{
		// Loading in windowed mode
		IN_DeactivateMouse();
	}
	else if (com_minimized->integer || com_unfocused->integer)
	{
		// Window not got focus
		IN_DeactivateMouse();
	}
	else
	{
		if (loading)
		{
			// Just eat up all the mouse events that are not used anyway
			IN_GobbleMotionEvents();
		}

		IN_ActivateMouse();
		IN_JoyMove();
	}

	// in case we had to delay actual restart of video system...
	if ((vidRestartTime != 0) && (vidRestartTime < Sys_Milliseconds()))
	{
		vidRestartTime = 0;
		Cbuf_AddText("vid_restart\n");
	}

	IN_ProcessEvents();

	// Store the timestamp for the next frame's input events
	lasttime = start;
}

/**
 * @brief IN_InitKeyLockStates
 */
static void IN_InitKeyLockStates(void)
{
	unsigned const char *keystate = SDL_GetKeyboardState(NULL);

	keys[K_SCROLLOCK].down  = keystate[SDL_SCANCODE_SCROLLLOCK];
	keys[K_KP_NUMLOCK].down = keystate[SDL_SCANCODE_NUMLOCKCLEAR];
	keys[K_CAPSLOCK].down   = keystate[SDL_SCANCODE_CAPSLOCK];
}

/**
 * @brief IN_Init
 */
void IN_Init(void)
{
	int appState;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		Com_Error(ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )");
	}

	mainScreen = (SDL_Window *)GLimp_MainWindow();

	//Com_Printf("\n------- Input Initialization -------\n");


#ifdef __ANDROID__
	// This set mouse input and touch to be handled separately
	// SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
	// This has been removed and replaced with those bellow in SDL 2.0.10
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
	SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
#endif

	in_keyboardDebug = Cvar_Get("in_keyboardDebug", "0", CVAR_TEMP);

	// mouse variables
	in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);

	if (in_mouse->integer == 2)
	{
		if (!SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE))
		{
			Com_Printf(S_COLOR_RED "Emulating non raw mouse input failed!\n");
		}
	}
	else
	{
		if (!SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0", SDL_HINT_OVERRIDE))
		{
			Com_Printf(S_COLOR_RED "Raw mouse input failed!\n");
		}
	}

	in_nograb = Cvar_Get("in_nograb", "0", CVAR_ARCHIVE);

	in_joystick          = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH);
	in_joystickThreshold = Cvar_Get("in_joystickThreshold", "0.15", CVAR_ARCHIVE);

	SDL_StartTextInput();

	mouseAvailable = (in_mouse->value != 0.f);
	IN_DeactivateMouse();

	appState = SDL_GetWindowFlags(mainScreen);
	Cvar_SetValue("com_unfocused", !((appState & SDL_WINDOW_INPUT_FOCUS) && (appState & SDL_WINDOW_MOUSE_FOCUS)));
	Cvar_SetValue("com_minimized", appState & SDL_WINDOW_MINIMIZED);

	IN_InitKeyLockStates();

	IN_InitJoystick();

	//Com_Printf("------------------------------------\n");
}

/**
 * @brief IN_Shutdown
 */
void IN_Shutdown(void)
{
	SDL_StopTextInput();
	Com_Printf("SDL input devices shut down.\n");

	IN_DeactivateMouse();
	mouseAvailable = qfalse;

	IN_ShutdownJoystick();

	mainScreen = NULL;
}

/**
 * @brief IN_Restart
 */
void IN_Restart(void)
{
	IN_ShutdownJoystick();
	IN_Init();
}
