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
 * @file con_tty.c
 */

#ifdef _WIN32


#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"
#include "windows.h"

#define QCONSOLE_HISTORY 32

static WORD qconsole_attrib;

// saved console status
static DWORD               qconsole_orig_mode;
static CONSOLE_CURSOR_INFO qconsole_orig_cursorinfo;

// cmd history
static char qconsole_history[QCONSOLE_HISTORY][MAX_EDIT_LINE];
static int  qconsole_history_pos    = -1;
static int  qconsole_history_oldest = 0;

// current edit buffer
static char qconsole_line[MAX_EDIT_LINE];
static int  qconsole_linelen = 0;

static HANDLE qconsole_hout;
static HANDLE qconsole_hin;

/*
==================
CON_CtrlHandler

The Windows Console doesn't use signals for terminating the application
with Ctrl-C, logging off, window closing, etc.  Instead it uses a special
handler routine.  Fortunately, the values for Ctrl signals don't seem to
overlap with true signal codes that Windows provides, so calling
Sys_SigHandler() with those numbers should be safe for generating unique
shutdown messages.
==================
*/
static BOOL WINAPI CON_CtrlHandler(DWORD sig)
{
	Sys_SigHandler(sig);
	return TRUE;
}

/*
==================
CON_HistAdd
==================
*/
static void CON_HistAdd(void)
{
	Q_strncpyz(qconsole_history[qconsole_history_oldest], qconsole_line,
	           sizeof(qconsole_history[qconsole_history_oldest]));

	if (qconsole_history_oldest >= QCONSOLE_HISTORY - 1)
	{
		qconsole_history_oldest = 0;
	}
	else
	{
		qconsole_history_oldest++;
	}

	qconsole_history_pos = qconsole_history_oldest;
}

/*
==================
CON_HistPrev
==================
*/
static void CON_HistPrev(void)
{
	int pos;

	pos = (qconsole_history_pos < 1) ?
	      (QCONSOLE_HISTORY - 1) : (qconsole_history_pos - 1);

	// don' t allow looping through history
	if (pos == qconsole_history_oldest)
	{
		return;
	}

	qconsole_history_pos = pos;
	Q_strncpyz(qconsole_line, qconsole_history[qconsole_history_pos],
	           sizeof(qconsole_line));
	qconsole_linelen = strlen(qconsole_line);
}

/*
==================
CON_HistNext
==================
*/
static void CON_HistNext(void)
{
	int pos;

	pos = (qconsole_history_pos >= QCONSOLE_HISTORY - 1) ?
	      0 : (qconsole_history_pos + 1);

	// clear the edit buffer if they try to advance to a future command
	if (pos == qconsole_history_oldest)
	{
		qconsole_line[0] = '\0';
		qconsole_linelen = 0;
		return;
	}

	qconsole_history_pos = pos;
	Q_strncpyz(qconsole_line, qconsole_history[qconsole_history_pos],
	           sizeof(qconsole_line));
	qconsole_linelen = strlen(qconsole_line);
}


/*
==================
CON_Show
==================
*/
static void CON_Show(void)
{
	CONSOLE_SCREEN_BUFFER_INFO binfo;
	COORD                      writeSize = { MAX_EDIT_LINE, 1 };
	COORD                      writePos  = { 0, 0 };
	SMALL_RECT                 writeArea = { 0, 0, 0, 0 };
	int                        i;
	CHAR_INFO                  line[MAX_EDIT_LINE];

	GetConsoleScreenBufferInfo(qconsole_hout, &binfo);

	// if we're in the middle of printf, don't bother writing the buffer
	if (binfo.dwCursorPosition.X != 0)
	{
		return;
	}

	writeArea.Left   = 0;
	writeArea.Top    = binfo.dwCursorPosition.Y;
	writeArea.Bottom = binfo.dwCursorPosition.Y;
	writeArea.Right  = MAX_EDIT_LINE;

	// build a space-padded CHAR_INFO array
	for (i = 0; i < MAX_EDIT_LINE; i++)
	{
		if (i < qconsole_linelen)
		{
			line[i].Char.AsciiChar = qconsole_line[i];
		}
		else
		{
			line[i].Char.AsciiChar = ' ';
		}

		line[i].Attributes = qconsole_attrib;
	}

	if (qconsole_linelen > binfo.srWindow.Right)
	{
		WriteConsoleOutput(qconsole_hout,
		                   line + (qconsole_linelen - binfo.srWindow.Right),
		                   writeSize, writePos, &writeArea);
	}
	else
	{
		WriteConsoleOutput(qconsole_hout, line, writeSize,
		                   writePos, &writeArea);
	}
}

/*
==================
CON_Shutdown
==================
*/
void CON_Shutdown(void)
{
	SetConsoleMode(qconsole_hin, qconsole_orig_mode);
	SetConsoleCursorInfo(qconsole_hout, &qconsole_orig_cursorinfo);
	CloseHandle(qconsole_hout);
	CloseHandle(qconsole_hin);
}

/*
==================
CON_Init
==================
*/
void CON_Init(void)
{
	CONSOLE_CURSOR_INFO        curs;
	CONSOLE_SCREEN_BUFFER_INFO info;
	int                        i;

	// handle Ctrl-C or other console termination
	SetConsoleCtrlHandler(CON_CtrlHandler, TRUE);

	qconsole_hin = GetStdHandle(STD_INPUT_HANDLE);
	if (qconsole_hin == INVALID_HANDLE_VALUE)
	{
		return;
	}

	qconsole_hout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (qconsole_hout == INVALID_HANDLE_VALUE)
	{
		return;
	}

	GetConsoleMode(qconsole_hin, &qconsole_orig_mode);

	// allow mouse wheel scrolling
	SetConsoleMode(qconsole_hin,
	               qconsole_orig_mode & ~ENABLE_MOUSE_INPUT);

	FlushConsoleInputBuffer(qconsole_hin);

	GetConsoleScreenBufferInfo(qconsole_hout, &info);
	qconsole_attrib = info.wAttributes;

	SetConsoleTitle("ioquake3 Dedicated Server Console");

	// make cursor invisible
	GetConsoleCursorInfo(qconsole_hout, &qconsole_orig_cursorinfo);
	curs.dwSize   = 1;
	curs.bVisible = FALSE;
	SetConsoleCursorInfo(qconsole_hout, &curs);

	// initialize history
	for (i = 0; i < QCONSOLE_HISTORY; i++)
		qconsole_history[i][0] = '\0';
}

/*
==================
CON_Input
==================
*/
char *CON_Input(void)
{
	INPUT_RECORD buff[MAX_EDIT_LINE];
	DWORD        count = 0, events = 0;
	WORD         key   = 0;
	int          i;
	int          newlinepos = -1;

	if (!GetNumberOfConsoleInputEvents(qconsole_hin, &events))
	{
		return NULL;
	}

	if (events < 1)
	{
		return NULL;
	}

	// if we have overflowed, start dropping oldest input events
	if (events >= MAX_EDIT_LINE)
	{
		ReadConsoleInput(qconsole_hin, buff, 1, &events);
		return NULL;
	}

	if (!ReadConsoleInput(qconsole_hin, buff, events, &count))
	{
		return NULL;
	}

	FlushConsoleInputBuffer(qconsole_hin);

	for (i = 0; i < count; i++)
	{
		if (buff[i].EventType != KEY_EVENT)
		{
			continue;
		}
		if (!buff[i].Event.KeyEvent.bKeyDown)
		{
			continue;
		}

		key = buff[i].Event.KeyEvent.wVirtualKeyCode;

		if (key == VK_RETURN)
		{
			newlinepos = i;
			break;
		}
		else if (key == VK_UP)
		{
			CON_HistPrev();
			break;
		}
		else if (key == VK_DOWN)
		{
			CON_HistNext();
			break;
		}
		else if (key == VK_TAB)
		{
			field_t f;

			Q_strncpyz(f.buffer, qconsole_line,
			           sizeof(f.buffer));
			Field_AutoComplete(&f);
			Q_strncpyz(qconsole_line, f.buffer,
			           sizeof(qconsole_line));
			qconsole_linelen = strlen(qconsole_line);
			break;
		}

		if (qconsole_linelen < sizeof(qconsole_line) - 1)
		{
			char c = buff[i].Event.KeyEvent.uChar.AsciiChar;

			if (key == VK_BACK)
			{
				int pos = (qconsole_linelen > 0) ?
				          qconsole_linelen - 1 : 0;

				qconsole_line[pos] = '\0';
				qconsole_linelen   = pos;
			}
			else if (c)
			{
				qconsole_line[qconsole_linelen++] = c;
				qconsole_line[qconsole_linelen]   = '\0';
			}
		}
	}

	CON_Show();

	if (newlinepos < 0)
	{
		return NULL;
	}

	if (!qconsole_linelen)
	{
		Com_Printf("\n");
		return NULL;
	}

	CON_HistAdd();
	Com_Printf("%s\n", qconsole_line);

	qconsole_linelen = 0;

	return qconsole_line;
}

/*
==================
CON_Print
==================
*/
void CON_Print(const char *msg)
{
	fputs(msg, stderr);

	CON_Show();
}

#else

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#ifndef DEDICATED
#include "../client/client.h"
#endif

#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

/*
=============================================================
tty console routines

NOTE: if the user is editing a line when something gets printed to the early
console then it won't look good so we provide CON_Hide and CON_Show to be
called before and after a stdout or stderr output
=============================================================
*/

extern qboolean stdinIsATTY;
static qboolean stdin_active;
// general flag to tell about tty console mode
static qboolean ttycon_on           = qfalse;
static int      ttycon_hide         = 0;
static int      ttycon_show_overdue = 0;

// some key codes that the terminal may be using, initialised on start up
static int TTY_erase;
static int TTY_eof;

static struct termios TTY_tc;

static field_t TTY_con;

// This is somewhat of aduplicate of the graphical console history
// but it's safer more modular to have our own here
#define CON_HISTORY 32
static field_t ttyEditLines[CON_HISTORY];
static int     hist_current = -1, hist_count = 0;

#ifndef DEDICATED
// Don't use "]" as it would be the same as in-game console,
//   this makes it clear where input came from.
#define TTY_CONSOLE_PROMPT "tty]"
#else
#define TTY_CONSOLE_PROMPT "]"
#endif

/*
==================
CON_FlushIn

Flush stdin, I suspect some terminals are sending a LOT of shit
FIXME relevant?
==================
*/
static void CON_FlushIn(void)
{
	char key;
	while (read(STDIN_FILENO, &key, 1) != -1)
		;
}

/*
==================
CON_Back

Output a backspace

NOTE: it seems on some terminals just sending '\b' is not enough so instead we
send "\b \b"
(FIXME there may be a way to find out if '\b' alone would work though)
==================
*/
static void CON_Back(void)
{
	char              key;
	size_t UNUSED_VAR size;

	key  = '\b';
	size = write(STDOUT_FILENO, &key, 1);
	key  = ' ';
	size = write(STDOUT_FILENO, &key, 1);
	key  = '\b';
	size = write(STDOUT_FILENO, &key, 1);
}

/*
==================
CON_Hide

Clear the display of the line currently edited
bring cursor back to beginning of line
==================
*/
static void CON_Hide(void)
{
	if (ttycon_on)
	{
		int i;
		if (ttycon_hide)
		{
			ttycon_hide++;
			return;
		}
		if (TTY_con.cursor > 0)
		{
			for (i = 0; i < TTY_con.cursor; i++)
			{
				CON_Back();
			}
		}
		// Delete prompt
		for (i = strlen(TTY_CONSOLE_PROMPT); i > 0; i--)
		{
			CON_Back();
		}
		ttycon_hide++;
	}
}

/*
==================
CON_Show

Show the current line
FIXME need to position the cursor if needed?
==================
*/
static void CON_Show(void)
{
	if (ttycon_on)
	{
		int i;

		assert(ttycon_hide > 0);
		ttycon_hide--;
		if (ttycon_hide == 0)
		{
			size_t UNUSED_VAR size;
			size = write(STDOUT_FILENO, TTY_CONSOLE_PROMPT, strlen(TTY_CONSOLE_PROMPT));
			if (TTY_con.cursor)
			{
				for (i = 0; i < TTY_con.cursor; i++)
				{
					size = write(STDOUT_FILENO, TTY_con.buffer + i, 1);
				}
			}
		}
	}
}

/*
==================
CON_Shutdown

Never exit without calling this, or your terminal will be left in a pretty bad state
==================
*/
void CON_Shutdown(void)
{
	if (ttycon_on)
	{
		CON_Hide();
		tcsetattr(STDIN_FILENO, TCSADRAIN, &TTY_tc);
	}

	// Restore blocking to stdin reads
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) & ~O_NONBLOCK);
}

/*
==================
Hist_Add
==================
*/
void Hist_Add(field_t *field)
{
	// Don't save blank lines in history.
	if (!field->cursor)
	{
		return;
	}

	int i;
	assert(hist_count <= CON_HISTORY);
	assert(hist_count >= 0);
	assert(hist_current >= -1);
	assert(hist_current <= hist_count);
	// make some room
	for (i = CON_HISTORY - 1; i > 0; i--)
	{
		ttyEditLines[i] = ttyEditLines[i - 1];
	}
	ttyEditLines[0] = *field;
	if (hist_count < CON_HISTORY)
	{
		hist_count++;
	}
	hist_current = -1; // re-init
}

/*
==================
Hist_Prev
==================
*/
field_t *Hist_Prev(void)
{
	int hist_prev;
	assert(hist_count <= CON_HISTORY);
	assert(hist_count >= 0);
	assert(hist_current >= -1);
	assert(hist_current <= hist_count);
	hist_prev = hist_current + 1;
	if (hist_prev >= hist_count)
	{
		return NULL;
	}
	hist_current++;
	return &(ttyEditLines[hist_current]);
}

/*
==================
Hist_Next
==================
*/
field_t *Hist_Next(void)
{
	assert(hist_count <= CON_HISTORY);
	assert(hist_count >= 0);
	assert(hist_current >= -1);
	assert(hist_current <= hist_count);
	if (hist_current >= 0)
	{
		hist_current--;
	}
	if (hist_current == -1)
	{
		return NULL;
	}
	return &(ttyEditLines[hist_current]);
}

/*
==================
CON_SigCont
Reinitialize console input after receiving SIGCONT, as on Linux the terminal seems to lose all
set attributes if user did CTRL+Z and then does fg again.
==================
*/

void CON_SigCont(int signum)
{
	CON_Init();
}

/*
==================
CON_Init

Initialize the console input (tty mode if possible)
==================
*/
void CON_Init(void)
{
	struct termios tc;

	// If the process is backgrounded (running non interactively)
	// then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	// If SIGCONT is received, reinitialize console
	signal(SIGCONT, CON_SigCont);

	// Make stdin reads non-blocking
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

	if (!stdinIsATTY)
	{
		Com_Printf("tty console mode disabled\n");
		ttycon_on    = qfalse;
		stdin_active = qtrue;
		return;
	}

	Field_Clear(&TTY_con);
	tcgetattr(STDIN_FILENO, &TTY_tc);
	TTY_erase = TTY_tc.c_cc[VERASE];
	TTY_eof   = TTY_tc.c_cc[VEOF];
	tc        = TTY_tc;

	/*
	ECHO: don't echo input characters
	ICANON: enable canonical mode.  This  enables  the  special
	characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	STATUS, and WERASE, and buffers by lines.
	ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	DSUSP are received, generate the corresponding sig
	nal
	*/
	tc.c_lflag &= ~(ECHO | ICANON);

	/*
	ISTRIP strip off bit 8
	INPCK enable input parity checking
	*/
	tc.c_iflag    &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN]  = 1;
	tc.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSADRAIN, &tc);
	ttycon_on   = qtrue;
	ttycon_hide = 1; // Mark as hidden, so prompt is shown in CON_Show
	CON_Show();
}

/*
==================
CON_Input
==================
*/
char *CON_Input(void)
{
	// we use this when sending back commands
	static char       text[MAX_EDIT_LINE];
	int               avail;
	char              key;
	field_t           *history;
	size_t UNUSED_VAR size;

	if (ttycon_on)
	{
		avail = read(STDIN_FILENO, &key, 1);
		if (avail != -1)
		{
			// we have something
			// backspace?
			// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
			if ((key == TTY_erase) || (key == 127) || (key == 8))
			{
				if (TTY_con.cursor > 0)
				{
					TTY_con.cursor--;
					TTY_con.buffer[TTY_con.cursor] = '\0';
					CON_Back();
				}
				return NULL;
			}
			// check if this is a control char
			if ((key) && (key) < ' ')
			{
				if (key == '\n')
				{
#ifndef DEDICATED
					// if not in the game explicitly prepend a slash if needed
					if (clc.state != CA_ACTIVE && TTY_con.cursor &&
					    TTY_con.buffer[0] != '/' && TTY_con.buffer[0] != '\\')
					{
						memmove(TTY_con.buffer + 1, TTY_con.buffer, sizeof(TTY_con.buffer) - 1);
						TTY_con.buffer[0] = '\\';
						TTY_con.cursor++;
					}

					if (TTY_con.buffer[0] == '/' || TTY_con.buffer[0] == '\\')
					{
						Q_strncpyz(text, TTY_con.buffer + 1, sizeof(text));
					}
					else if (TTY_con.cursor)
					{
						Com_sprintf(text, sizeof(text), "cmd say %s", TTY_con.buffer);
					}
					else
					{
						text[0] = '\0';
					}

					// push it in history
					Hist_Add(&TTY_con);
					CON_Hide();
					Com_Printf("%s%s\n", TTY_CONSOLE_PROMPT, TTY_con.buffer);
					Field_Clear(&TTY_con);
					CON_Show();
#else
					// push it in history
					Hist_Add(&TTY_con);
					Q_strncpyz(text, TTY_con.buffer, sizeof(text));
					Field_Clear(&TTY_con);
					key  = '\n';
					size = write(STDOUT_FILENO, &key, 1);
					size = write(STDOUT_FILENO, TTY_CONSOLE_PROMPT, strlen(TTY_CONSOLE_PROMPT));
#endif

					return text;
				}
				if (key == '\t')
				{
					CON_Hide();
					Field_AutoComplete(&TTY_con);
					CON_Show();
					return NULL;
				}
				avail = read(STDIN_FILENO, &key, 1);
				if (avail != -1)
				{
					// VT 100 keys
					if (key == '[' || key == 'O')
					{
						avail = read(STDIN_FILENO, &key, 1);
						if (avail != -1)
						{
							switch (key)
							{
							case 'A':
								history = Hist_Prev();
								if (history)
								{
									CON_Hide();
									TTY_con = *history;
									CON_Show();
								}
								CON_FlushIn();
								return NULL;
								break;
							case 'B':
								history = Hist_Next();
								CON_Hide();
								if (history)
								{
									TTY_con = *history;
								}
								else
								{
									Field_Clear(&TTY_con);
								}
								CON_Show();
								CON_FlushIn();
								return NULL;
								break;
							case 'C':
								return NULL;
							case 'D':
								return NULL;
							}
						}
					}
				}
				Com_DPrintf("droping ISCTL sequence: %d, TTY_erase: %d\n", key, TTY_erase);
				CON_FlushIn();
				return NULL;
			}
			if (TTY_con.cursor >= sizeof(text) - 1)
			{
				return NULL;
			}
			// push regular character
			TTY_con.buffer[TTY_con.cursor] = key;
			TTY_con.cursor++; // next char will always be '\0'
			// print the current line (this is differential)
			size = write(STDOUT_FILENO, &key, 1);
		}

		return NULL;
	}
	else if (stdin_active)
	{
		int            len;
		fd_set         fdset;
		struct timeval timeout;

		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset); // stdin
		timeout.tv_sec  = 0;
		timeout.tv_usec = 0;
		if (select(STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(STDIN_FILENO, &fdset))
		{
			return NULL;
		}

		len = read(STDIN_FILENO, text, sizeof(text));
		if (len == 0) // eof!
		{
			stdin_active = qfalse;
			return NULL;
		}

		if (len < 1)
		{
			return NULL;
		}
		text[len - 1] = 0;    // rip off the /n and terminate

		return text;
	}
	return NULL;
}

/*
==================
CON_Print
==================
*/
void CON_Print(const char *msg)
{
	if (!msg[0])
	{
		return;
	}

	CON_Hide();

	if (com_ansiColor && com_ansiColor->integer)
	{
		Sys_AnsiColorPrint(msg);
	}
	else
	{
		fputs(msg, stderr);
	}

	if (!ttycon_on)
	{
		// CON_Hide didn't do anything.
		return;
	}

	// Only print prompt when msg ends with a newline, otherwise the console
	//   might get garbled when output does not fit on one line.
	if (msg[strlen(msg) - 1] == '\n')
	{
		CON_Show();

		// Run CON_Show the number of times it was deferred.
		while (ttycon_show_overdue > 0)
		{
			CON_Show();
			ttycon_show_overdue--;
		}
	}
	else
	{
		// Defer calling CON_Show
		ttycon_show_overdue++;
	}
}

#endif
