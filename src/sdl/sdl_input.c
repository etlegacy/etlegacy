/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file sdl_input.c
 */

#ifdef BUNDLED_SDL
	#include "SDL.h"
#else
	#include <SDL/SDL.h>
#endif

#ifdef _WIN32 // USE_RAW_INPUT_MOUSE is not defined yet
#ifdef BUNDLED_SDL
#   include "SDL_syswm.h"
#else
#   include <SDL/SDL_syswm.h>
#endif
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../client/client.h"
#include "../sys/sys_local.h"

#ifdef __APPLE__
// Mouse acceleration needs to be disabled
#define MACOS_X_ACCELERATION_HACK
// Cursor needs hack to hide
#define MACOS_X_CURSOR_HACK
#endif // __APPLE__

#ifdef MACOS_X_ACCELERATION_HACK
#include <IOKit/IOTypes.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>
#endif

static cvar_t *in_nograb;
static cvar_t *in_keyboardDebug = NULL;

static cvar_t   *in_mouse        = NULL;
static qboolean mouseAvailable   = qfalse;
static qboolean mouseActive      = qfalse;
static qboolean keyRepeatEnabled = qfalse;

#ifdef MACOS_X_ACCELERATION_HACK
static cvar_t *in_disablemacosxmouseaccel = NULL;
static double originalMouseSpeed          = -1.0;
#endif

static SDL_Joystick *stick                = NULL;
static cvar_t       *in_joystick          = NULL;
static cvar_t       *in_joystickDebug     = NULL;
static cvar_t       *in_joystickThreshold = NULL;
static cvar_t       *in_joystickNo        = NULL;
static cvar_t       *in_joystickUseAnalog = NULL;

static int vidRestartTime = 0;

#define CTRL(a) ((a) - 'a' + 1)

#ifdef USE_RAW_INPUT_MOUSE

static WNDPROC  SDLWindowProc = NULL;
static qboolean mouseRaw;

/*
Doing that because I couldn't get to RAWINPUT through SDL_SYSWMEVENT //run
http://lists.libsdl.org/pipermail/sdl-libsdl.org/2005-February/048704.html
*/

int rawMouseButtonMap[5] = { K_MOUSE1, K_MOUSE2, K_MOUSE3, K_MOUSE4, K_MOUSE5 };

static LONG WINAPI RawWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INPUT:
		if (mouseRaw)
		{
			RAWINPUT     ri;
			unsigned int i = sizeof(ri);

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &ri, &i, sizeof(RAWINPUTHEADER));
			if (mouseActive)
			{
				for (i = 0; i < 5; i++)
				{
					if (ri.data.mouse.ulButtons & (1 << (i * 2)))
					{
						Com_QueueEvent(0, SE_KEY, rawMouseButtonMap[i], qtrue, 0, NULL);
					}
					else if (ri.data.mouse.ulButtons & (1 << (i * 2 + 1)))
					{
						Com_QueueEvent(0, SE_KEY, rawMouseButtonMap[i], qfalse, 0, NULL);
					}
				}
				if (ri.data.mouse.lLastX || ri.data.mouse.lLastY)
				{
					Com_QueueEvent(0, SE_MOUSE, ri.data.mouse.lLastX, ri.data.mouse.lLastY, 0, NULL);
				}
			}
			if (ri.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
			{
				for (i = 0; i < abs((signed short)ri.data.mouse.usButtonData) / WHEEL_DELTA; i++)
				{
					Com_QueueEvent(0, SE_KEY, (signed short)ri.data.mouse.usButtonData < 0 ? K_MWHEELDOWN : K_MWHEELUP, qtrue, 0, NULL);
					Com_QueueEvent(0, SE_KEY, (signed short)ri.data.mouse.usButtonData < 0 ? K_MWHEELDOWN : K_MWHEELUP, qfalse, 0, NULL);
				}
			}
		}
		break;
	default:
		break;
	}
	return CallWindowProc(SDLWindowProc, hWnd, uMsg, wParam, lParam);
}

/*
============================================================
RAW INPUT MOUSE
(Cgg)
============================================================
*/

qboolean IN_InitRawMouse(void)
{
	// http://www.usb.org/developers/devclass_docs/Hut1_12.pdf
	if (!SDLWindowProc)
	{
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWMInfo(&wmInfo);
		SDLWindowProc = (WNDPROC)GetWindowLongPtr(wmInfo.window, GWLP_WNDPROC);
		SetWindowLongPtr(wmInfo.window, GWLP_WNDPROC, (LONG_PTR)RawWndProc);
	}

	{
		RAWINPUTDEVICE dev =
		{
			1,  // usUsagePage - generic desktop controls
			2,  // usUsage - mouse
			0,  // dwFlags
			0   // hwndTarget
		};
		if (!RegisterRawInputDevices(&dev, 1, sizeof(dev)))
		{
			Com_Printf("Raw input registration failed. (0x%lx)\n", GetLastError());
			return qfalse;
		}
	}
	Com_DPrintf("Registered for raw input.\n");
	mouseRaw = qtrue;
	return qtrue;
}

qboolean IN_ShutdownRawMouse(void)
{
	RAWINPUTDEVICE dev =
	{
		1,  // usUsagePage - generic desktop controls
		2,  // usUsage - mouse
		RIDEV_REMOVE,   // dwFlags
		NULL    // hwndTarget
	};
	if (!RegisterRawInputDevices(&dev, 1, sizeof(dev)))
	{
		Com_Printf("Mouse release failed. (0x%lx)\n", GetLastError());
		return qfalse;
	}
	mouseRaw = qfalse;
	Com_DPrintf("Released raw input mouse.\n");
	if (SDLWindowProc)
	{
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWMInfo(&wmInfo);
		SetWindowLongPtr(wmInfo.window, GWLP_WNDPROC, (LONG_PTR)SDLWindowProc);
		SDLWindowProc = NULL;
	}
	return qtrue;
}

#endif

/**
 * @brief Prints keyboard identifiers in the console
 */
static void IN_PrintKey(const SDL_keysym *keysym, keyNum_t key, qboolean down)
{
	if (down)
	{
		Com_Printf("+ ");
	}
	else
	{
		Com_Printf("  ");
	}

	Com_Printf("0x%02x \"%s\"", keysym->scancode,
	           SDL_GetKeyName(keysym->sym));

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
	if (keysym->mod & KMOD_LMETA)
	{
		Com_Printf(" KMOD_LMETA");
	}
	if (keysym->mod & KMOD_RMETA)
	{
		Com_Printf(" KMOD_RMETA");
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

	Com_Printf(" Q:0x%02x(%s)", key, Key_KeynumToString(key));

	if (keysym->unicode)
	{
		Com_Printf(" U:0x%02x", keysym->unicode);

		if (keysym->unicode > ' ' && keysym->unicode < '~')
		{
			Com_Printf("(%c)", (char)keysym->unicode);
		}
	}

	Com_Printf("\n");
}

#define MAX_CONSOLE_KEYS 16

static qboolean IN_IsConsoleKey(keyNum_t key, const unsigned char character)
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
			unsigned char character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[MAX_CONSOLE_KEYS];
	static int          numConsoleKeys = 0;
	int                 i;

	// Only parse the variable when it changes
	if (cl_consoleKeys->modified)
	{
		consoleKey_t *c;
		char         *text_p, *token;
		int          charCode;

		cl_consoleKeys->modified = qfalse;
		text_p                   = cl_consoleKeys->string;
		numConsoleKeys           = 0;

		while (numConsoleKeys < MAX_CONSOLE_KEYS)
		{
			c        = &consoleKeys[numConsoleKeys];
			charCode = 0;

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
				c->u.character = (unsigned char)charCode;
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
 * @brief translates SDL keyboard identifier to its Q3 counterpart
 */
static const char *IN_TranslateSDLToQ3Key(SDL_keysym *keysym,
                                          keyNum_t *key, qboolean down)
{
	static unsigned char buf[2] = { '\0', '\0' };

	*buf = '\0';
	*key = 0;

	if (keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE)
	{
		// These happen to match the ASCII chars
		*key = (int)keysym->sym;
	}
	else
	{
		switch (keysym->sym)
		{
		case SDLK_PAGEUP:       *key = K_PGUP;          break;
		case SDLK_KP9:          *key = K_KP_PGUP;       break;
		case SDLK_PAGEDOWN:     *key = K_PGDN;          break;
		case SDLK_KP3:          *key = K_KP_PGDN;       break;
		case SDLK_KP7:          *key = K_KP_HOME;       break;
		case SDLK_HOME:         *key = K_HOME;          break;
		case SDLK_KP1:          *key = K_KP_END;        break;
		case SDLK_END:          *key = K_END;           break;
		case SDLK_KP4:          *key = K_KP_LEFTARROW;  break;
		case SDLK_LEFT:         *key = K_LEFTARROW;     break;
		case SDLK_KP6:          *key = K_KP_RIGHTARROW; break;
		case SDLK_RIGHT:        *key = K_RIGHTARROW;    break;
		case SDLK_KP2:          *key = K_KP_DOWNARROW;  break;
		case SDLK_DOWN:         *key = K_DOWNARROW;     break;
		case SDLK_KP8:          *key = K_KP_UPARROW;    break;
		case SDLK_UP:           *key = K_UPARROW;       break;
		case SDLK_ESCAPE:       *key = K_ESCAPE;        break;
		case SDLK_KP_ENTER:     *key = K_KP_ENTER;      break;
		case SDLK_RETURN:       *key = K_ENTER;         break;
		case SDLK_TAB:          *key = K_TAB;           break;
		case SDLK_F1:           *key = K_F1;            break;
		case SDLK_F2:           *key = K_F2;            break;
		case SDLK_F3:           *key = K_F3;            break;
		case SDLK_F4:           *key = K_F4;            break;
		case SDLK_F5:           *key = K_F5;            break;
		case SDLK_F6:           *key = K_F6;            break;
		case SDLK_F7:           *key = K_F7;            break;
		case SDLK_F8:           *key = K_F8;            break;
		case SDLK_F9:           *key = K_F9;            break;
		case SDLK_F10:          *key = K_F10;           break;
		case SDLK_F11:          *key = K_F11;           break;
		case SDLK_F12:          *key = K_F12;           break;
		case SDLK_F13:          *key = K_F13;           break;
		case SDLK_F14:          *key = K_F14;           break;
		case SDLK_F15:          *key = K_F15;           break;

		case SDLK_BACKSPACE:    *key = K_BACKSPACE;     break;
		case SDLK_KP_PERIOD:    *key = K_KP_DEL;        break;
		case SDLK_DELETE:       *key = K_DEL;           break;
		case SDLK_PAUSE:        *key = K_PAUSE;         break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:       *key = K_SHIFT;         break;

		case SDLK_LCTRL:
		case SDLK_RCTRL:        *key = K_CTRL;          break;

		case SDLK_RMETA:
		case SDLK_LMETA:        *key = K_COMMAND;       break;

		case SDLK_RALT:
		case SDLK_LALT:         *key = K_ALT;           break;

		case SDLK_LSUPER:
		case SDLK_RSUPER:       *key = K_SUPER;         break;

		case SDLK_KP5:          *key = K_KP_5;          break;
		case SDLK_INSERT:       *key = K_INS;           break;
		case SDLK_KP0:          *key = K_KP_INS;        break;
		case SDLK_KP_MULTIPLY:  *key = K_KP_STAR;       break;
		case SDLK_KP_PLUS:      *key = K_KP_PLUS;       break;
		case SDLK_KP_MINUS:     *key = K_KP_MINUS;      break;
		case SDLK_KP_DIVIDE:    *key = K_KP_SLASH;      break;

		case SDLK_MODE:         *key = K_MODE;          break;
		case SDLK_COMPOSE:      *key = K_COMPOSE;       break;
		case SDLK_HELP:         *key = K_HELP;          break;
		case SDLK_PRINT:        *key = K_PRINT;         break;
		case SDLK_SYSREQ:       *key = K_SYSREQ;        break;
		case SDLK_BREAK:        *key = K_BREAK;         break;
		case SDLK_MENU:         *key = K_MENU;          break;
		case SDLK_POWER:        *key = K_POWER;         break;
		case SDLK_EURO:         *key = K_EURO;          break;
		case SDLK_UNDO:         *key = K_UNDO;          break;
		case SDLK_SCROLLOCK:    *key = K_SCROLLOCK;     break;
		case SDLK_NUMLOCK:      *key = K_KP_NUMLOCK;    break;
		case SDLK_CAPSLOCK:     *key = K_CAPSLOCK;      break;

		default:
			if (keysym->sym >= SDLK_WORLD_0 && keysym->sym <= SDLK_WORLD_95)
			{
				*key = (keysym->sym - SDLK_WORLD_0) + K_WORLD_0;
			}
			break;
		}
	}

	if (down && keysym->unicode && !(keysym->unicode & 0xFF00))
	{
		unsigned char ch = (unsigned char)keysym->unicode & 0xFF;

		switch (ch)
		{
		case 127:     // ASCII delete
			if (*key != K_DEL)
			{
				// ctrl-h
				*buf = CTRL('h');
				break;
			}
		// fallthrough

		default:
			*buf = ch;
			break;
		}
	}

	if (in_keyboardDebug->integer)
	{
		IN_PrintKey(keysym, *key, down);
	}

	// Keys that have ASCII names but produce no character are probably
	// dead keys -- ignore them
	if (down && strlen(Key_KeynumToString(*key)) == 1 &&
	    keysym->unicode == 0)
	{
		if (in_keyboardDebug->integer)
		{
			Com_Printf("  Ignored dead key '%c'\n", *key);
		}

		*key = 0;
	}

	if (IN_IsConsoleKey(*key, *buf))
	{
		// Console keys can't be bound or generate characters
		*key = K_CONSOLE;
		*buf = '\0';
	}

	// Don't allow extended ASCII to generate characters
	if (*buf & 0x80)
	{
		*buf = '\0';
	}

	return (char *)buf;
}

#ifdef MACOS_X_ACCELERATION_HACK

static io_connect_t IN_GetIOHandle(void) // mac os x mouse accel hack
{
	io_connect_t  iohandle = MACH_PORT_NULL;
	kern_return_t status;
	io_service_t  iohidsystem = MACH_PORT_NULL;
	mach_port_t   masterport;

	status = IOMasterPort(MACH_PORT_NULL, &masterport);
	if (status != KERN_SUCCESS)
	{
		return 0;
	}

	iohidsystem = IORegistryEntryFromPath(masterport, kIOServicePlane ":/IOResources/IOHIDSystem");
	if (!iohidsystem)
	{
		return 0;
	}

	status = IOServiceOpen(iohidsystem, mach_task_self(), kIOHIDParamConnectType, &iohandle);
	IOObjectRelease(iohidsystem);

	return iohandle;
}
#endif

static void IN_GobbleMotionEvents(void)
{
	SDL_Event dummy[1];

	// Gobble any mouse motion events
	SDL_PumpEvents();
	while (SDL_PeepEvents(dummy, 1, SDL_GETEVENT,
	                      SDL_EVENTMASK(SDL_MOUSEMOTION)))
	{
	}
}

static void IN_ActivateMouse(void)
{
	if (!mouseAvailable || !SDL_WasInit(SDL_INIT_VIDEO))
	{
		return;
	}

#ifdef MACOS_X_ACCELERATION_HACK
	if (!mouseActive) // mac os x mouse accel hack
	{
		// Save the status of mouse acceleration
		originalMouseSpeed = -1.0; // in case of error
		if (in_disablemacosxmouseaccel->integer)
		{
			io_connect_t mouseDev = IN_GetIOHandle();
			if (mouseDev != 0)
			{
				if (IOHIDGetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), &originalMouseSpeed) == kIOReturnSuccess)
				{
					Com_Printf("previous mouse acceleration: %f\n", originalMouseSpeed);
					if (IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), -1.0) != kIOReturnSuccess)
					{
						Com_Printf("Could not disable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
						Cvar_Set("in_disablemacosxmouseaccel", 0);
					}
				}
				else
				{
					Com_Printf("Could not disable mouse acceleration (failed at IOHIDGetAccelerationWithKey).\n");
					Cvar_Set("in_disablemacosxmouseaccel", 0);
				}
				IOServiceClose(mouseDev);
			}
			else
			{
				Com_Printf("Could not disable mouse acceleration (failed at IO_GetIOHandle).\n");
				Cvar_Set("in_disablemacosxmouseaccel", 0);
			}
		}
	}
#endif

	if (!mouseActive)
	{
		SDL_ShowCursor(0);
#ifdef MACOS_X_CURSOR_HACK
		// This is a bug in the current SDL/macosx...have to toggle it a few
		//  times to get the cursor to hide.
		SDL_ShowCursor(1);
		SDL_ShowCursor(0);
#endif
		SDL_WM_GrabInput(SDL_GRAB_ON);

		IN_GobbleMotionEvents();
	}

	// in_nograb makes no sense in fullscreen mode
	if (!Cvar_VariableIntegerValue("r_fullscreen"))
	{
		if (in_nograb->modified || !mouseActive)
		{
			if (in_nograb->integer)
			{
				SDL_WM_GrabInput(SDL_GRAB_OFF);
			}
			else
			{
				SDL_WM_GrabInput(SDL_GRAB_ON);
			}

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

static void IN_DeactivateMouse(void)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		return;
	}

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if (!Cvar_VariableIntegerValue("r_fullscreen"))
	{
#if 0
		if ((Key_GetCatcher() == KEYCATCH_UI) &&
		    (SDL_GetAppState() & SDL_APPMOUSEFOCUS))
		{
			SDL_ShowCursor(0);
		}
		else
#endif
		{
			SDL_ShowCursor(1);
		}
	}

	if (!mouseAvailable)
	{
		return;
	}

#ifdef MACOS_X_ACCELERATION_HACK
	if (mouseActive) // mac os x mouse accel hack
	{
		if (originalMouseSpeed != -1.0)
		{
			io_connect_t mouseDev = IN_GetIOHandle();
			if (mouseDev != 0)
			{
				Com_Printf("restoring mouse acceleration to: %f\n", originalMouseSpeed);
				if (IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), originalMouseSpeed) != kIOReturnSuccess)
				{
					Com_Printf("Could not re-enable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
				}
				IOServiceClose(mouseDev);
			}
			else
			{
				Com_Printf("Could not re-enable mouse acceleration (failed at IO_GetIOHandle).\n");
			}
		}
	}
#endif

	if (mouseActive)
	{
		IN_GobbleMotionEvents();

		SDL_WM_GrabInput(SDL_GRAB_OFF);

		// Don't warp the mouse unless the cursor is within the window
		if (SDL_GetAppState() & SDL_APPMOUSEFOCUS)
		{
			SDL_WarpMouse(cls.glconfig.vidWidth / 2, cls.glconfig.vidHeight / 2);
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

static void IN_InitJoystick(void)
{
	int  i          = 0;
	int  total      = 0;
	char buf[16384] = "";

	if (stick != NULL)
	{
		SDL_JoystickClose(stick);
	}

	stick = NULL;
	memset(&stick_state, '\0', sizeof(stick_state));

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		Com_Printf("Initializing joystick devices\n");
		if (SDL_Init(SDL_INIT_JOYSTICK) == -1)
		{
			Com_Printf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		Com_Printf("...joysticks initialized\n");
	}

	total = SDL_NumJoysticks();
	Com_Printf("...available joysticks: %d\n", total);

	// Print list and build cvar to allow ui to select joystick.
	for (i = 0; i < total; i++)
	{
		Q_strcat(buf, sizeof(buf), SDL_JoystickName(i));
		Q_strcat(buf, sizeof(buf), "\n");
	}

	Cvar_Get("in_availableJoysticks", buf, CVAR_ROM);

	if (!in_joystick->integer)
	{
		Com_Printf("...no active joystick set\n");
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		return;
	}

	in_joystickNo = Cvar_Get("in_joystickNo", "0", CVAR_ARCHIVE);
	if (in_joystickNo->integer < 0 || in_joystickNo->integer >= total)
	{
		Cvar_Set("in_joystickNo", "0");
	}

	in_joystickUseAnalog = Cvar_Get("in_joystickUseAnalog", "0", CVAR_ARCHIVE);

	stick = SDL_JoystickOpen(in_joystickNo->integer);

	if (stick == NULL)
	{
		Com_Printf("No joystick opened.\n");
		return;
	}

	Com_DPrintf("Joystick %d opened\n", in_joystickNo->integer);
	Com_DPrintf("Name:       %s\n", SDL_JoystickName(in_joystickNo->integer));
	Com_DPrintf("Axes:       %d\n", SDL_JoystickNumAxes(stick));
	Com_DPrintf("Hats:       %d\n", SDL_JoystickNumHats(stick));
	Com_DPrintf("Buttons:    %d\n", SDL_JoystickNumButtons(stick));
	Com_DPrintf("Balls:      %d\n", SDL_JoystickNumBalls(stick));
	Com_DPrintf("Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No");

	SDL_JoystickEventState(SDL_QUERY);
}

static void IN_ShutdownJoystick(void)
{
	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

static void IN_JoyMove(void)
{
	qboolean     joy_pressed[ARRAY_LEN(joy_keys)];
	unsigned int axes  = 0;
	unsigned int hats  = 0;
	int          total = 0;
	int          i     = 0;

	if (!stick)
	{
		return;
	}

	SDL_JoystickUpdate();

	memset(joy_pressed, '\0', sizeof(joy_pressed));

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
			Com_QueueEvent(0, SE_MOUSE, balldx, balldy, 0, NULL);
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
				Com_QueueEvent(0, SE_KEY, K_JOY1 + i, pressed, 0, NULL);
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
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 0], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHT:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_DOWN:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 2], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFT:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 3], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHTUP:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 0], qfalse, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_RIGHTDOWN:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 2], qfalse, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 1], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFTUP:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 0], qfalse, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 3], qfalse, 0, NULL);
					break;
				case SDL_HAT_LEFTDOWN:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 2], qfalse, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 3], qfalse, 0, NULL);
					break;
				default:
					break;
				}
				// press event
				switch (((Uint8 *)&hats)[i])
				{
				case SDL_HAT_UP:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 0], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHT:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_DOWN:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 2], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFT:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 3], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHTUP:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 0], qtrue, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_RIGHTDOWN:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 2], qtrue, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 1], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFTUP:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 0], qtrue, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 3], qtrue, 0, NULL);
					break;
				case SDL_HAT_LEFTDOWN:
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 2], qtrue, 0, NULL);
					Com_QueueEvent(0, SE_KEY, hat_keys[4 * i + 3], qtrue, 0, NULL);
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
				float f = ((float) abs(axis)) / 32767.0f;

				if (f < in_joystickThreshold->value)
				{
					axis = 0;
				}

				if (axis != stick_state.oldaaxes[i])
				{
					Com_QueueEvent(0, SE_JOYSTICK_AXIS, i, axis, 0, NULL);
					stick_state.oldaaxes[i] = axis;
				}
			}
			else
			{
				float f = ((float) axis) / 32767.0f;

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
				Com_QueueEvent(0, SE_KEY, joy_keys[i], qtrue, 0, NULL);
			}

			if (!(axes & (1 << i)) && (stick_state.oldaxes & (1 << i)))
			{
				Com_QueueEvent(0, SE_KEY, joy_keys[i], qfalse, 0, NULL);
			}
		}
	}

	// Save for future generations.
	stick_state.oldaxes = axes;
}

static void IN_ProcessEvents(void)
{
	SDL_Event  e;
	const char *character = NULL;
	keyNum_t   key        = 0;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		return;
	}

	if (Key_GetCatcher() == 0 && keyRepeatEnabled)
	{
		SDL_EnableKeyRepeat(0, 0);
		keyRepeatEnabled = qfalse;
	}
	else if (!keyRepeatEnabled)
	{
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
		                    SDL_DEFAULT_REPEAT_INTERVAL);
		keyRepeatEnabled = qtrue;
	}

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_KEYDOWN:
			character = IN_TranslateSDLToQ3Key(&e.key.keysym, &key, qtrue);
			if (key)
			{
				Com_QueueEvent(0, SE_KEY, key, qtrue, 0, NULL);
			}

			if (character)
			{
				Com_QueueEvent(0, SE_CHAR, *character, 0, 0, NULL);
			}
			break;

		case SDL_KEYUP:
			IN_TranslateSDLToQ3Key(&e.key.keysym, &key, qfalse);

			if (key)
			{
				Com_QueueEvent(0, SE_KEY, key, qfalse, 0, NULL);
			}
			break;

		case SDL_MOUSEMOTION:
			if (mouseActive)
			{
				Com_QueueEvent(0, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL);
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			int b;

			switch (e.button.button)
			{
			case SDL_BUTTON_LEFT:      b = K_MOUSE1;     break;
			case SDL_BUTTON_MIDDLE:    b = K_MOUSE3;     break;
			case SDL_BUTTON_RIGHT:     b = K_MOUSE2;     break;
			case SDL_BUTTON_WHEELUP:   b = K_MWHEELUP;   break;
			case SDL_BUTTON_WHEELDOWN: b = K_MWHEELDOWN; break;
			case SDL_BUTTON_X1:        b = K_MOUSE4;     break;
			case SDL_BUTTON_X2:        b = K_MOUSE5;     break;
			default:                   b = K_AUX1 + (e.button.button - SDL_BUTTON_X2 + 1) % 16; break;
			}
			Com_QueueEvent(0, SE_KEY, b, (e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse), 0, NULL);
		}
		break;

		case SDL_QUIT:
			Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
			break;

		case SDL_VIDEORESIZE:
		{
			char width[32], height[32];

			Com_sprintf(width, sizeof(width), "%d", e.resize.w);
			Com_sprintf(height, sizeof(height), "%d", e.resize.h);
			Cvar_Set("r_customwidth", width);
			Cvar_Set("r_customheight", height);
			Cvar_Set("r_mode", "-1");
			// wait until user stops dragging for 1 second, so
			// we aren't constantly recreating the GL context while
			// he tries to drag...
			vidRestartTime = Sys_Milliseconds() + 1000;
		}
		break;
		case SDL_ACTIVEEVENT:
			if (e.active.state & SDL_APPINPUTFOCUS)
			{
				Cvar_SetValue("com_unfocused", !e.active.gain);
#ifdef USE_RAW_INPUT_MOUSE
				if (e.active.gain && in_mouse->integer == 3)    //raw input stops working on winxp after losing focus. (why?)
				{
					IN_ShutdownRawMouse();
					IN_InitRawMouse();
				}
#endif
			}
			if (e.active.state & SDL_APPACTIVE)
			{
				Cvar_SetValue("com_minimized", !e.active.gain);
				//if ( e.active.gain && Cvar_VariableIntegerValue("r_fullscreen") )
				//  Cbuf_ExecuteText( EXEC_APPEND, "vid_restart\n" );
			}
			break;

		default:
			break;
		}
	}
}

void IN_Frame(void)
{
	qboolean loading;
	qboolean fullscreen;

	IN_JoyMove();
	IN_ProcessEvents();

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading    = (cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE);
	fullscreen = Cvar_VariableIntegerValue("r_fullscreen");

	if (!fullscreen && (Key_GetCatcher() & KEYCATCH_CONSOLE))
	{
		// Console is down in windowed mode
		IN_DeactivateMouse();
	}
	else if (!fullscreen && loading)
	{
		// Loading in windowed mode
		IN_DeactivateMouse();
	}
	else if (!(SDL_GetAppState() & SDL_APPINPUTFOCUS))
	{
		// Window not got focus
		IN_DeactivateMouse();
	}
	else
	{
		IN_ActivateMouse();
	}

	// in case we had to delay actual restart of video system...
	if ((vidRestartTime != 0) && (vidRestartTime < Sys_Milliseconds()))
	{
		vidRestartTime = 0;
		Cbuf_AddText("vid_restart\n");
	}
}

void IN_InitKeyLockStates(void)
{
	unsigned char *keystate = SDL_GetKeyState(NULL);

	keys[K_SCROLLOCK].down  = keystate[SDLK_SCROLLOCK];
	keys[K_KP_NUMLOCK].down = keystate[SDLK_NUMLOCK];
	keys[K_CAPSLOCK].down   = keystate[SDLK_CAPSLOCK];
}

void IN_Init(void)
{
	int appState;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		Com_Error(ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )");
		return;
	}

	Com_DPrintf("\n------- Input Initialization -------\n");

	in_keyboardDebug = Cvar_Get("in_keyboardDebug", "0", CVAR_ARCHIVE);

	// mouse variables
	in_mouse  = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);
	in_nograb = Cvar_Get("in_nograb", "0", CVAR_ARCHIVE);

	in_joystick          = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH);
	in_joystickDebug     = Cvar_Get("in_joystickDebug", "0", CVAR_TEMP);
	in_joystickThreshold = Cvar_Get("joy_threshold", "0.15", CVAR_ARCHIVE);

#ifdef MACOS_X_ACCELERATION_HACK
	in_disablemacosxmouseaccel = Cvar_Get("in_disablemacosxmouseaccel", "1", CVAR_ARCHIVE);
#endif

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	keyRepeatEnabled = qtrue;

#ifdef USE_RAW_INPUT_MOUSE
	if (mouseRaw)
	{
		IN_ShutdownRawMouse();
	}
	if (in_mouse->integer == 3 && IN_InitRawMouse())
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
		SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
		SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	}
	else
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
		SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
		SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
	}
#endif

	mouseAvailable = (in_mouse->value != 0);
	IN_DeactivateMouse();

	appState = SDL_GetAppState();
	Cvar_SetValue("com_unfocused", !(appState & SDL_APPINPUTFOCUS));
	Cvar_SetValue("com_minimized", !(appState & SDL_APPACTIVE));

	IN_InitKeyLockStates();

#ifndef __APPLE__ // FIXME: Joystick initialization crashes some Mac OS X clients
	IN_InitJoystick();
#endif
	Com_DPrintf("------------------------------------\n");
}

void IN_Shutdown(void)
{
	Com_Printf("SDL input devices shut down.\n");
#ifdef USE_RAW_INPUT_MOUSE
	IN_ShutdownRawMouse();
#endif
	IN_DeactivateMouse();
	mouseAvailable = qfalse;

	IN_ShutdownJoystick();
}

void IN_Restart(void)
{
	IN_ShutdownJoystick();
	IN_Init();
}
