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
 * @file sys_win32_con.c
 * @brief Contains windows-specific code for console.
 */

#ifdef USE_WINDOWS_CONSOLE

#include "../client/client.h"
#include "../qcommon/q_unicode.h"
#include "win_resource.h"
#include "sys_win32.h"

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <windows.h>

#define SYSCON_DEFAULT_WIDTH    540
#define SYSCON_DEFAULT_HEIGHT   450

#define COPY_ID         1
#define QUIT_ID         2
#define CLEAR_ID        3

#define ERRORBOX_ID     10
#define ERRORTEXT_ID    11

#define EDIT_ID         100
#define INPUT_ID        101

//Old value: 16384
#define CONSOLE_BUFFER_SIZE     32768

#define COLOR_TEXT_NORMAL RGB(192, 192, 192)
#define COLOR_TEXT_EDIT   RGB(255, 255, 255)
#define COLOR_TEXT_ERROR1 RGB(255, 0, 0)
#define COLOR_TEXT_ERROR2 COLOR_TEXT_NORMAL //RGB(0, 0, 0)
#define COLOR_BCK_NORMAL  RGB(28, 47, 54)
#define COLOR_BCK_ERROR   RGB(28, 47, 54)

#define COMPONENT_ZERO 0, 0, 0, 0

typedef struct
{
	HWND hWnd;
	HWND hwndBuffer;

	HWND hwndButtonClear;
	HWND hwndButtonCopy;
	HWND hwndButtonQuit;

	HWND hwndErrorBox;
	HWND hwndErrorText; // unused

	HBITMAP hbmLogo;
	HBITMAP hbmClearBitmap;

	HBRUSH hbrEditBackground;
	HBRUSH hbrErrorBackground;

	HFONT hfBufferFont;
	HFONT hfButtonFont;

	HWND hwndInputLine;

	char errorString[128];

	char consoleText[512], returnedText[512]; // returnedText is unused
	int visLevel;
	qboolean quitOnClose;
	int windowWidth, windowHeight;

	WNDPROC SysInputLineWndProc;

} WinConData;

static WinConData s_wcd;

/**
 * @brief CON_ResizeWindowsCon
 * @param[in] cx
 * @param[in] cy
 */
static void CON_ResizeWindowsCon(int cx, int cy)
{
	float sx, /*sy,*/ x, y, w, h;

	if (cx < SYSCON_DEFAULT_WIDTH)
	{
		cx = SYSCON_DEFAULT_WIDTH;
	}
	if (cy < SYSCON_DEFAULT_HEIGHT)
	{
		cy = SYSCON_DEFAULT_HEIGHT;
	}

	sx = (float)cx / SYSCON_DEFAULT_WIDTH;
	// sy = (float)cy / SYSCON_DEFAULT_HEIGHT; // FIXME: never read !

	x = 5;
	w = cx - 15;
	if (s_wcd.hwndErrorBox)
	{
		SetWindowPos(s_wcd.hwndErrorBox, NULL, x, 5, w, 30, 0);

		y = 40;
		h = cy - 72;
	}
	else
	{
		y = 5;
		h = cy - 65;
	}
	SetWindowPos(s_wcd.hwndBuffer, NULL, x, y, w, h, 0);

	if (s_wcd.hwndInputLine)
	{
		y = y + h + 8;
		h = 20;
		SetWindowPos(s_wcd.hwndInputLine, NULL, x, y, w, h, 0);
	}

	y = y + h + 4;
	w = 72 * sx;
	h = 24;
	SetWindowPos(s_wcd.hwndButtonCopy, NULL, x, y, w, h, 0);

	x = x + w + 2;
	if (s_wcd.hwndButtonClear)
	{
		SetWindowPos(s_wcd.hwndButtonClear, NULL, x, y, w, h, 0);
	}

	x = cx - 15 - w;
	SetWindowPos(s_wcd.hwndButtonQuit, NULL, x, y, w, h, 0);

	s_wcd.windowWidth  = cx;
	s_wcd.windowHeight = cy;
}

/**
 * @brief ConWndProc
 * @param[in] hWnd
 * @param[in] uMsg
 * @param[in] wParam
 * @param[in] lParam
 * @return
 */
static LONG WINAPI ConWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char            *cmdString;
	static qboolean s_timePolarity;

	switch (uMsg)
	{
	case WM_SIZE:
		CON_ResizeWindowsCon(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE)
		{
			SetFocus(s_wcd.hwndInputLine);
		}

		if (com_viewlog && (com_dedicated && !com_dedicated->integer))
		{
			// if the viewlog is open, check to see if it's being minimized
			if (com_viewlog->integer == 1)
			{
				if (HIWORD(wParam))         // minimized flag
				{
					Cvar_Set("viewlog", "2");
				}
			}
			else if (com_viewlog->integer == 2)
			{
				if (!HIWORD(wParam))            // minimized flag
				{
					Cvar_Set("viewlog", "1");
				}
			}
		}
		break;

	case WM_CLOSE:
		if ((com_dedicated && com_dedicated->integer))
		{
			cmdString = CopyString("quit");
			Com_QueueEvent(0, SE_CONSOLE, 0, 0, strlen(cmdString) + 1, cmdString);
		}
		else if (s_wcd.quitOnClose)
		{
			PostQuitMessage(0);
		}
		else
		{
			Sys_ShowConsole(0, qfalse);
			Cvar_Set("viewlog", "0");
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == s_wcd.hwndBuffer)
		{
			SetBkColor((HDC)wParam, COLOR_BCK_NORMAL);
			SetTextColor((HDC)wParam, COLOR_TEXT_NORMAL);
			return (long)s_wcd.hbrEditBackground;
		}
		else if ((HWND)lParam == s_wcd.hwndErrorBox)
		{
			SetBkColor((HDC)wParam, COLOR_BCK_ERROR);
			if (s_timePolarity & 1)
			{
				SetTextColor((HDC)wParam, COLOR_TEXT_ERROR1);
			}
			else
			{
				SetTextColor((HDC)wParam, COLOR_TEXT_ERROR2);
			}

			return (long)s_wcd.hbrErrorBackground;
		}
		break;
	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == s_wcd.hwndInputLine)
		{
			SetBkColor((HDC)wParam, COLOR_BCK_NORMAL);
			SetTextColor((HDC)wParam, COLOR_TEXT_EDIT);

			return (long)s_wcd.hbrEditBackground;
		}
	case WM_COMMAND:
		if (wParam == COPY_ID)
		{
			SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
			SendMessage(s_wcd.hwndBuffer, WM_COPY, 0, 0);
		}
		else if (wParam == QUIT_ID)
		{
			if (s_wcd.quitOnClose)
			{
				PostQuitMessage(0);
			}
			else
			{
				cmdString = CopyString("quit");
				Com_QueueEvent(0, SE_CONSOLE, 0, 0, strlen(cmdString) + 1, cmdString);
			}
		}
		else if (wParam == CLEAR_ID)
		{
			SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
			SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM ) L"");
			UpdateWindow(s_wcd.hwndBuffer);
		}
		break;
	case WM_CREATE:
		s_wcd.hbmLogo            = LoadBitmap(g_wv.hInstance, MAKEINTRESOURCE(IDB_BITMAP1));
		s_wcd.hbmClearBitmap     = LoadBitmap(g_wv.hInstance, MAKEINTRESOURCE(IDB_BITMAP2));
		s_wcd.hbrEditBackground  = CreateSolidBrush(COLOR_BCK_NORMAL);
		s_wcd.hbrErrorBackground = CreateSolidBrush(COLOR_BCK_ERROR);
		SetTimer(hWnd, 1, 1000, NULL);
		break;
	case WM_ERASEBKGND:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_TIMER:
		if (wParam == 1)
		{
			s_timePolarity = (qboolean) !s_timePolarity;
			if (s_wcd.hwndErrorBox)
			{
				InvalidateRect(s_wcd.hwndErrorBox, NULL, FALSE);
			}
		}

		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
win32 console commandline completion
*/

#define WIN_COMMAND_HISTORY     64

static field_t win_consoleField;
static int     win_acLength;
static char    win_completionString[MAX_TOKEN_CHARS];
static char    win_currentMatch[MAX_TOKEN_CHARS];
static int     win_matchCount;
static int     win_matchIndex;
static int     win_findMatchIndex;
static field_t win_historyEditLines[WIN_COMMAND_HISTORY];
static int     win_nextHistoryLine = 0;
static int     win_historyLine     = 0;

/**
 * @brief Win_FindIndexMatch
 * @param[in] s
 */
static void Win_FindIndexMatch(const char *s)
{
	if (Q_stricmpn(s, win_completionString, strlen(win_completionString)))
	{
		return;
	}
	if (win_findMatchIndex == win_matchIndex)
	{
		Q_strncpyz(win_currentMatch, s, sizeof(win_currentMatch));
	}

	win_findMatchIndex++;
}

/**
 * @brief Win_FindMatches
 * @param[in] s
 */
static void Win_FindMatches(const char *s)
{
	int i;

	if (Q_stricmpn(s, win_completionString, strlen(win_completionString)))
	{
		return;
	}
	win_matchCount++;
	if (win_matchCount == 1)
	{
		Q_strncpyz(win_currentMatch, s, sizeof(win_currentMatch));
		return;
	}

	// cut currentMatch to the amount common with s
	for (i = 0; s[i]; i++)
	{
		if (tolower(win_currentMatch[i]) != tolower(s[i]))
		{
			win_currentMatch[i] = 0;
		}
	}

	win_currentMatch[i] = 0;
}

/**
 * @brief Win_KeyConcatArgs
 */
static void Win_KeyConcatArgs(void)
{
	int  i;
	char *arg;

	for (i = 1; i < Cmd_Argc(); i++)
	{
		Q_strcat(win_consoleField.buffer, sizeof(win_consoleField.buffer), " ");
		arg = Cmd_Argv(i);
		while (*arg)
		{
			if (*arg == ' ')
			{
				Q_strcat(win_consoleField.buffer, sizeof(win_consoleField.buffer), "\"");
				break;
			}

			arg++;
		}
		Q_strcat(win_consoleField.buffer, sizeof(win_consoleField.buffer), Cmd_Argv(i));
		if (*arg == ' ')
		{
			Q_strcat(win_consoleField.buffer, sizeof(win_consoleField.buffer), "\"");
		}
	}
}

/**
 * @brief Win_ConcatRemaining
 * @param[in] src
 * @param[in] start
 */
static void Win_ConcatRemaining(const char *src, const char *start)
{
	const char *str;

	str = strstr(src, start);
	if (!str)
	{
		Win_KeyConcatArgs();
		return;
	}

	str += strlen(start);
	Q_strcat(win_consoleField.buffer, sizeof(win_consoleField.buffer), str);
}

/**
 * @brief Win_PrintMatches
 * @param[in] s
 */
static void Win_PrintMatches(const char *s)
{
	if (!Q_stricmpn(s, win_currentMatch, win_acLength))
	{
		Sys_Print(va("  ^9%s^0\n", s));
	}
}

/**
 * @brief ydnar: to display cvar values
 * @param s
 */
static void Win_PrintCvarMatches(const char *s)
{
	if (!Q_stricmpn(s, win_currentMatch, win_acLength))
	{
		Sys_Print(va("  ^9%s = ^5%s^0\n", s, Cvar_VariableString(s)));
	}
}

/**
 * @brief Win_CompleteCommand
 * @param[in] showMatches
 */
static void Win_CompleteCommand(qboolean showMatches)
{
	field_t *edit = &win_consoleField;
	field_t temp;

	if (win_acLength == 0)
	{
		// only look at the first token for completion purposes
		Cmd_TokenizeString(edit->buffer);

		Q_strncpyz(win_completionString, Cmd_Argv(0), sizeof(win_completionString));
		if (win_completionString[0] == '\\' || win_completionString[0] == '/')
		{
			Q_strncpyz(win_completionString, win_completionString + 1, sizeof(win_completionString));
		}

		win_matchCount      = 0;
		win_matchIndex      = 0;
		win_currentMatch[0] = 0;

		if (strlen(win_completionString) == 0)
		{
			return;
		}

		Cmd_CommandCompletion(Win_FindMatches);
		Cvar_CommandCompletion(Win_FindMatches);

		if (win_matchCount == 0)
		{
			return; // no matches
		}

		Com_Memcpy(&temp, edit, sizeof(field_t));

		if (win_matchCount == 1)
		{
			Com_sprintf(edit->buffer, sizeof(edit->buffer), "%s", win_currentMatch);
			if (Cmd_Argc() == 1)
			{
				Q_strcat(win_consoleField.buffer, sizeof(win_consoleField.buffer), " ");
			}
			else
			{
				Win_ConcatRemaining(temp.buffer, win_completionString);
			}

			edit->cursor = Q_UTF8_Strlen(edit->buffer);
		}
		else
		{
			// multiple matches, complete to shortest
			Com_sprintf(edit->buffer, sizeof(edit->buffer), "%s", win_currentMatch);
			win_acLength = edit->cursor = Q_UTF8_Strlen(edit->buffer);
			Win_ConcatRemaining(temp.buffer, win_completionString);
			showMatches = qtrue;
		}
	}
	else if (win_matchCount != 1)
	{
		// get the next match and show instead
		char lastMatch[MAX_TOKEN_CHARS];

		Q_strncpyz(lastMatch, win_currentMatch, sizeof(lastMatch));

		win_matchIndex++;
		if (win_matchIndex == win_matchCount)
		{
			win_matchIndex = 0;
		}

		win_findMatchIndex = 0;
		Cmd_CommandCompletion(Win_FindIndexMatch);
		Cvar_CommandCompletion(Win_FindIndexMatch);

		Com_Memcpy(&temp, edit, sizeof(field_t));

		// and print it
		Com_sprintf(edit->buffer, sizeof(edit->buffer), "%s", win_currentMatch);
		edit->cursor = Q_UTF8_Strlen(edit->buffer);
		Win_ConcatRemaining(temp.buffer, lastMatch);
	}

	// hijack it
	if (win_matchCount == 1)
	{
		win_acLength = Q_UTF8_Strlen(win_currentMatch);
	}

	// run through again, printing matches
	if (showMatches && win_matchCount > 0)
	{
		Com_Memcpy(&temp, edit, sizeof(*edit));
		temp.buffer[win_acLength] = '\0';
		Sys_Print(va("] %s\n", temp.buffer));
		Cmd_CommandCompletion(Win_PrintMatches);
		Cvar_CommandCompletion(Win_PrintCvarMatches);
	}
}

/**
 * @brief InputLineWndProc
 * @param[in] hWnd
 * @param[in] uMsg
 * @param[in] wParam
 * @param[in] lParam
 * @return
 */
LONG WINAPI InputLineWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t w_buffer[MAX_EDIT_LINE];

	switch (uMsg)
	{
	case WM_KILLFOCUS:
		if ((HWND)wParam == s_wcd.hWnd || (HWND)wParam == s_wcd.hwndErrorBox)
		{
			SetFocus(hWnd);
			return 0;
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_UP:
			// previous history item
			if ((win_nextHistoryLine - win_historyLine < WIN_COMMAND_HISTORY) && win_historyLine > 0)
			{
				win_historyLine--;
			}
			win_consoleField = win_historyEditLines[win_historyLine % WIN_COMMAND_HISTORY];
			Sys_StringToWideCharArray(win_consoleField.buffer, w_buffer, sizeof(w_buffer));

			SetWindowTextW(s_wcd.hwndInputLine, w_buffer);
			SendMessage(s_wcd.hwndInputLine, EM_SETSEL, win_consoleField.cursor, win_consoleField.cursor);
			win_acLength = 0;
			return 0;

		case VK_DOWN:
			// next history item
			if (win_historyLine < win_nextHistoryLine)
			{
				win_historyLine++;
				win_consoleField = win_historyEditLines[win_historyLine % WIN_COMMAND_HISTORY];
				Sys_StringToWideCharArray(win_consoleField.buffer, w_buffer, sizeof(w_buffer));

				SetWindowTextW(s_wcd.hwndInputLine, w_buffer);
				SendMessage(s_wcd.hwndInputLine, EM_SETSEL, win_consoleField.cursor, win_consoleField.cursor);
			}

			win_acLength = 0;
			return 0;
		}
		break;

	case WM_CHAR:
		//GetWindowText( s_wcd.hwndInputLine, inputBuffer, sizeof( inputBuffer ) );
		GetWindowTextW(s_wcd.hwndInputLine, w_buffer, ARRAY_LEN(w_buffer));

		Sys_WideCharArrayToString(w_buffer, win_consoleField.buffer, ARRAY_LEN(win_consoleField.buffer));

		SendMessage(s_wcd.hwndInputLine, EM_GETSEL, (WPARAM) NULL, (LPARAM) &win_consoleField.cursor);
		win_consoleField.widthInChars = Q_UTF8_Strlen(win_consoleField.buffer);
		win_consoleField.scroll       = 0;

		// handle enter key
		if (wParam == 13)
		{
			strncat(s_wcd.consoleText, win_consoleField.buffer, sizeof(s_wcd.consoleText) - strlen(s_wcd.consoleText) - 5);
			strcat(s_wcd.consoleText, "\n");
			SetWindowTextW(s_wcd.hwndInputLine, L"");

			Sys_Print(va("]%s\n", win_consoleField.buffer));

			// clear autocomplete length
			win_acLength = 0;

			// copy line to history buffer
			if (win_consoleField.buffer[0] != '\0')
			{
				win_historyEditLines[win_nextHistoryLine % WIN_COMMAND_HISTORY] = win_consoleField;
				win_nextHistoryLine++;
				win_historyLine = win_nextHistoryLine;
			}

			return 0;
		}
		// handle tab key (commandline completion)
		else if (wParam == 9)
		{
			// enable this code for tab double-tap show matching
#if 0
			static int win_tabTime = 0;
			{
				int tabTime = Sys_Milliseconds();

				if ((tabTime - win_tabTime) < 100)
				{
					Win_CompleteCommand(qtrue);
					win_tabTime = 0;
				}
				else
				{
					Win_CompleteCommand(qfalse);
					win_tabTime = tabTime;
				}
			}
#else
			Win_CompleteCommand(qfalse);
#endif

			SetWindowText(s_wcd.hwndInputLine, win_consoleField.buffer);
			win_consoleField.widthInChars = Q_UTF8_Strlen(win_consoleField.buffer);
			SendMessage(s_wcd.hwndInputLine, EM_SETSEL, win_consoleField.cursor, win_consoleField.cursor);

			return 0;
		}
		// handle everything else
		else
		{
			win_acLength = 0;
		}

		break;
	}

	return CallWindowProc(s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam);
}

#define IDI_ICON1 1

/**
 * @brief Sys_CreateConsole
 */
void Sys_CreateConsole(void)
{
	HDC      hDC;
	WNDCLASS wc;
	RECT     rect;

	const char *DEDCLASS = "ET: Legacy WinConsole";
#if defined (UPDATE_SERVER)
	const char *WINDOWNAME = "ET: Legacy Update Server";
#elif defined(DEDICATED)
	const char *WINDOWNAME = "ET: Legacy Dedicated Server";
#else
	const char *WINDOWNAME = "ET: Legacy Console";
#endif

	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX;

	Com_Memset(&wc, 0, sizeof(wc));

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC)ConWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_wv.hInstance;
	wc.hIcon         = LoadIcon(g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = DEDCLASS;

	if (!RegisterClass(&wc))
	{
		return;
	}

	rect.left   = 0;
	rect.right  = SYSCON_DEFAULT_WIDTH;
	rect.top    = 0;
	rect.bottom = SYSCON_DEFAULT_HEIGHT;
	AdjustWindowRectEx(&rect, DEDSTYLE, FALSE, 0);

	hDC     = GetDC(GetDesktopWindow());
	swidth  = GetDeviceCaps(hDC, HORZRES);
	sheight = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(GetDesktopWindow(), hDC);

	s_wcd.windowWidth  = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	s_wcd.hWnd = CreateWindowEx(0,
	                            DEDCLASS,
	                            WINDOWNAME,
	                            DEDSTYLE,
	                            (swidth - 600) / 2, (sheight - 450) / 2, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
	                            NULL,
	                            NULL,
	                            g_wv.hInstance,
	                            NULL);

	if (s_wcd.hWnd == NULL)
	{
		return;
	}

	// create fonts
	hDC     = GetDC(s_wcd.hWnd);
	nHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(s_wcd.hWnd, hDC);

	s_wcd.hfBufferFont = CreateFont(nHeight, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
	s_wcd.hfButtonFont = CreateFont(nHeight, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Microsoft Sans Serif");

	// create the input line
	s_wcd.hwndInputLine = CreateWindowExW(WS_EX_CLIENTEDGE,
	                                     L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
	                                     0, 0, 0, 0,
	                                     s_wcd.hWnd,
	                                     ( HMENU ) INPUT_ID,    // child window ID
	                                     g_wv.hInstance, NULL);

	// create the buttons
	s_wcd.hwndButtonCopy = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
	                                    0, 0, 0, 0,
	                                    s_wcd.hWnd,
	                                    ( HMENU ) COPY_ID,          // child window ID
	                                    g_wv.hInstance, NULL);
	SendMessage(s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM ) "Copy");

	s_wcd.hwndButtonClear = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
	                                     0, 0, 0, 0,
	                                     s_wcd.hWnd,
	                                     ( HMENU ) CLEAR_ID,        // child window ID
	                                     g_wv.hInstance, NULL);
	SendMessage(s_wcd.hwndButtonClear, WM_SETTEXT, 0, ( LPARAM ) "Clear");

	s_wcd.hwndButtonQuit = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
	                                    0, 0, 0, 0,
	                                    s_wcd.hWnd,
	                                    ( HMENU ) QUIT_ID,          // child window ID
	                                    g_wv.hInstance, NULL);
	SendMessage(s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM ) "Quit");

	// create the scroll buffer
	s_wcd.hwndBuffer = CreateWindowExW(WS_EX_CLIENTEDGE,
	                                  L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL /* | WS_BORDER*/ |
	                                  ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
	                                  0, 0, 0, 0,
	                                  s_wcd.hWnd,
	                                  ( HMENU ) EDIT_ID,    // child window ID
	                                  g_wv.hInstance, NULL);

#if defined (_WIN64)
	s_wcd.SysInputLineWndProc = ( WNDPROC ) SetWindowLongPtr(s_wcd.hwndInputLine, GWLP_WNDPROC, ( LONG_PTR ) InputLineWndProc);
#else
	s_wcd.SysInputLineWndProc = ( WNDPROC ) SetWindowLong(s_wcd.hwndInputLine, GWL_WNDPROC, ( long ) InputLineWndProc);
#endif

	CON_ResizeWindowsCon(s_wcd.windowWidth, s_wcd.windowHeight);

	SendMessage(s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, FALSE);
	SendMessage(s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, FALSE);

	SendMessage(s_wcd.hwndButtonCopy, WM_SETFONT, (WPARAM)s_wcd.hfButtonFont, FALSE);
	SendMessage(s_wcd.hwndButtonClear, WM_SETFONT, (WPARAM)s_wcd.hfButtonFont, FALSE);
	SendMessage(s_wcd.hwndButtonQuit, WM_SETFONT, (WPARAM)s_wcd.hfButtonFont, FALSE);

	/*
	ShowWindow(s_wcd.hWnd, SW_SHOWDEFAULT);
	UpdateWindow(s_wcd.hWnd);
	SetForegroundWindow(s_wcd.hWnd);
	SetFocus(s_wcd.hwndInputLine);

	s_wcd.visLevel = 1;
	*/

	s_wcd.visLevel = 0;
}

/**
 * @brief Sys_DestroyConsole
 */
void Sys_DestroyConsole(void)
{
	if (s_wcd.hfBufferFont)
	{
		DeleteObject(s_wcd.hfBufferFont);
	}
	if (s_wcd.hfButtonFont)
	{
		DeleteObject(s_wcd.hfButtonFont);
	}

	if (s_wcd.hWnd)
	{
		ShowWindow(s_wcd.hWnd, SW_HIDE);
		CloseWindow(s_wcd.hWnd);
		DestroyWindow(s_wcd.hWnd);
		s_wcd.hWnd = 0;
	}
}

/**
 * @brief Sys_ShowConsole
 * @param[in] visLevel
 * @param[in] quitOnClose
 */
void Sys_ShowConsole(int visLevel, qboolean quitOnClose)
{
	if (quitOnClose)
	{
		timeEndPeriod(1);
	}

	s_wcd.quitOnClose = quitOnClose;

	if (visLevel == s_wcd.visLevel)
	{
		return;
	}

	s_wcd.visLevel = visLevel;

	if (!s_wcd.hWnd)
	{
		return;
	}

	ShowWindow(s_wcd.hWnd, SW_SHOWNORMAL);
	SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);

	switch (visLevel)
	{
	case 0:
		ShowWindow(s_wcd.hWnd, SW_HIDE);
		break;
	case 1:
		ShowWindow(s_wcd.hWnd, SW_SHOWNORMAL);
		UpdateWindow(s_wcd.hWnd);
		SetForegroundWindow(s_wcd.hWnd);
		SetFocus(s_wcd.hwndInputLine);
		SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);
		break;
	case 2:
		ShowWindow(s_wcd.hWnd, SW_MINIMIZE);
		break;
	default:
		Sys_Error("Invalid visLevel %d sent to Sys_ShowConsole", visLevel);
		break;
	}
}

#if defined (_WIN32)
/**
 * @brief Sys_ConsoleInput
 * @return
 */
char *Sys_ConsoleInput(void)
{
	if (s_wcd.consoleText[0] == 0)
	{
		return NULL;
	}

	strcpy(s_wcd.returnedText, s_wcd.consoleText);
	s_wcd.consoleText[0] = 0;

	return s_wcd.returnedText;
}
#endif

/**
 * @brief Conbuf_AppendText
 * @param[in] pMsg
 */
void Conbuf_AppendText(const char *pMsg)
{
	char                 buffer[CONSOLE_BUFFER_SIZE * 2], *b = buffer;
	wchar_t              w_buffer[CONSOLE_BUFFER_SIZE * 2];
	const char           *msg;
	int                  bufLen, i = 0;
	static unsigned long s_totalChars;

	// if the message is REALLY long, use just the last portion of it
	if (strlen(pMsg) > CONSOLE_BUFFER_SIZE - 1)
	{
		msg = pMsg + strlen(pMsg) - CONSOLE_BUFFER_SIZE + 1;
	}
	else
	{
		msg = pMsg;
	}

	// copy into an intermediate buffer
	while (msg[i] && ((b - buffer) < sizeof(buffer) - 1))
	{
		if (msg[i] == '\n' && msg[i + 1] == '\r')
		{
			b[0] = '\r';
			b[1] = '\n';
			b   += 2;
			i++;
		}
		else if (msg[i] == '\r')
		{
			b[0] = '\r';
			b[1] = '\n';
			b   += 2;
		}
		else if (msg[i] == '\n')
		{
			b[0] = '\r';
			b[1] = '\n';
			b   += 2;
		}
		else if (Q_IsColorString(&msg[i]))
		{
			i++;
		}
		else
		{
			*b = msg[i];
			b++;
		}

		i++;
	}
	*b     = 0;

	bufLen = Sys_StringToWideCharArray(buffer, w_buffer, CONSOLE_BUFFER_SIZE * 2);
	s_totalChars += bufLen;

	// replace selection instead of appending if we're overflowing
	if (s_totalChars > CONSOLE_BUFFER_SIZE)
	{
		SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
		s_totalChars = bufLen;
	}
	else
	{
		// always append at the bottom of the textbox
		SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0xFFFF, 0xFFFF);
	}

	// put this text into the windows console
	SendMessage(s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff);
	SendMessage(s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0);
	SendMessageW(s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM) w_buffer);
}

/**
 * @brief Sys_SetErrorText
 * @param[in] buf
 */
void Sys_SetErrorText(const char *buf)
{
	Q_strncpyz(s_wcd.errorString, buf, sizeof(s_wcd.errorString));

	if (!s_wcd.hwndErrorBox)
	{
		//RECT windowRect;
		//GetWindowRect(s_wcd.hWnd, &windowRect);

		s_wcd.hwndErrorBox = CreateWindow("static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
		                                  0, 0, 0, 0,
		                                  s_wcd.hWnd,
		                                  (HMENU)ERRORBOX_ID,            // child window ID
		                                  g_wv.hInstance, NULL);
		DestroyWindow(s_wcd.hwndInputLine);
		s_wcd.hwndInputLine = NULL;

		DestroyWindow(s_wcd.hwndButtonClear);
		s_wcd.hwndButtonClear = NULL;

		CON_ResizeWindowsCon(s_wcd.windowWidth, s_wcd.windowHeight);
		SendMessage(s_wcd.hwndErrorBox, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);
		SetWindowText(s_wcd.hwndErrorBox, s_wcd.errorString);
	}
}

/**
 * @brief Sys_ClearViewlog_f
 */
void Sys_ClearViewlog_f(void)
{
	SendMessageW(s_wcd.hwndBuffer, WM_SETTEXT, 0, (LPARAM) L"");
}

/**
 * @brief Sys_PumpConsoleEvents
 */
void Sys_PumpConsoleEvents(void)
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_wv.sysMsgTime = msg.time;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
#endif
