/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 * Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>
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
 * @file sys_win32.h
 * @brief Contains windows-specific code for console.
 */

#ifndef INCLUDE_SYS_WIN_H
#define INCLUDE_SYS_WIN_H

void Sys_CreateConsoleWindow(void);

#if defined (_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(disable : 4201)
#pragma warning( push )
#endif
#include <windows.h>
#if defined (_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning( pop )
#endif

typedef struct
{
	HINSTANCE reflib_library;           // Handle to refresh DLL
	qboolean reflib_active;
	HWND hWnd;
	HINSTANCE hInstance;
	qboolean activeApp;
	OSVERSIONINFO osversion;
	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned sysMsgTime;
} WinVars_t;

extern WinVars_t g_wv;

size_t Sys_WideCharArrayToString(wchar_t *array, char *buffer, size_t len);
size_t Sys_StringToWideCharArray(const char* string, wchar_t *output, size_t len);

#ifdef USE_WINDOWS_CONSOLE
void    Sys_CreateConsole(void);
void    Sys_ShowConsole(int visLevel, qboolean quitOnClose);
int     Game_Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void    Com_FrameExt(void);
void    WinSetExceptionWnd(HWND wnd);
void    Sys_SetErrorText(const char *text);
void    Sys_PumpConsoleEvents(void);
#define Sys_ShowConsoleWindow(x, y) Sys_ShowConsole(x, y)
#else
#define Sys_ShowConsoleWindow(x, y) if (x) { Sys_CreateConsoleWindow(); }
#endif

#endif // #ifndef INCLUDE_SYS_WIN_H
