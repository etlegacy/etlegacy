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
 * @file console.c
 * @brief Ingame console
 * Must hold SHIFT + ~ to get console. CTRL + SHIFT + '~' opens a small console.
 */

#include "client.h"

int g_console_field_width = 78;
#define DEFAULT_CONSOLE_WIDTH   78

#define CONSOLE_COLOR  COLOR_WHITE

console_t con;

cvar_t *con_conspeed;
cvar_t *con_notifytime;
cvar_t *con_autoclear;


vec4_t console_color = { 1.0, 1.0, 1.0, 1.0 };
vec4_t console_highlightcolor = { 0.5, 0.5, 0.2, 0.45 };

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	// persistent console input is more useful (added cvar)
	if (con_autoclear->integer)
	{
		Field_Clear(&g_consoleField);
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();

	// multiple console size support
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		cls.keyCatchers &= ~KEYCATCH_CONSOLE;
		con.desiredFrac  = 0.0f;
	}
	else
	{
		cls.keyCatchers |= KEYCATCH_CONSOLE;

		// short console
		if (keys[K_CTRL].down)
		{
			con.desiredFrac = (5.0 * SMALLCHAR_HEIGHT) / cls.glconfig.vidHeight;
		}
		// full console
		else if (keys[K_ALT].down)
		{
			con.desiredFrac = 1.0;
		}
		// normal half-screen console
		else
		{
			con.desiredFrac = 0.5;
		}
	}
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
	chat_team = qfalse;

	Field_Clear(&chatField);
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	chat_team = qtrue;

	Field_Clear(&chatField);
	chatField.widthInChars = 25;
	cls.keyCatchers       ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f(void)
{
	chat_team  = qfalse;
	chat_buddy = qtrue;

	Field_Clear(&chatField);
	chatField.widthInChars = 26;
	cls.keyCatchers       ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f(void)
{
	int i;

	for (i = 0 ; i < CON_TEXTSIZE ; i++)
	{
		con.text[i] = (ColorIndex(CONSOLE_COLOR) << 8) | ' ';
	}

	Con_Bottom();       // go to end
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f(void)
{
	int          l, x, i;
	short        *line;
	fileHandle_t f;
	int          bufferlen;
	char         *buffer;
	char         filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: condump <filename>\n");
		return;
	}

	Q_strncpyz(filename, Cmd_Argv(1), sizeof(filename));
	COM_DefaultExtension(filename, sizeof(filename), ".txt");

	Com_Printf("Dumped console text to %s.\n", filename);

	f = FS_FOpenFileWrite(filename);
	if (!f)
	{
		Com_Printf("ERROR: couldn't open %s.\n", filename);
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l % con.totallines) * con.linewidth;
		for (x = 0 ; x < con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
			{
				break;
			}
		if (x != con.linewidth)
		{
			break;
		}
	}

#ifdef _WIN32
	bufferlen = con.linewidth + 3 * sizeof(char);
#else
	bufferlen = con.linewidth + 2 * sizeof(char);
#endif

	buffer = Hunk_AllocateTempMemory(bufferlen);

	// write the remaining lines
	buffer[bufferlen - 1] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l % con.totallines) * con.linewidth;
		for (i = 0; i < con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x = con.linewidth - 1 ; x >= 0 ; x--)
		{
			if (buffer[x] == ' ')
			{
				buffer[x] = 0;
			}
			else
			{
				break;
			}
		}
#ifdef _WIN32
		Q_strcat(buffer, bufferlen, "\r\n");
#else
		Q_strcat(buffer, bufferlen, "\n");
#endif
		FS_Write(buffer, strlen(buffer), f);
	}

	Hunk_FreeTempMemory(buffer);
	FS_FCloseFile(f);
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify(void)
{
	int i;

	for (i = 0 ; i < NUM_CON_TIMES ; i++)
	{
		con.times[i] = 0;
	}
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize(void)
{
	int              i, width;
	MAC_STATIC short tbuf[CON_TEXTSIZE];

	// wasn't allowing for larger consoles
	// width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
	width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
	{
		return;
	}

	if (width < 1)            // video hasn't been initialized yet
	{
		width          = DEFAULT_CONSOLE_WIDTH;
		con.linewidth  = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for (i = 0; i < CON_TEXTSIZE; i++)
		{
			con.text[i] = (ColorIndex(CONSOLE_COLOR) << 8) | ' ';
		}
	}
	else
	{
		int j;
		int oldtotallines, numlines, numchars;
		int oldwidth = con.linewidth;

		con.linewidth  = width;
		oldtotallines  = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines       = oldtotallines;

		if (con.totallines < numlines)
		{
			numlines = con.totallines;
		}

		numchars = oldwidth;

		if (con.linewidth < numchars)
		{
			numchars = con.linewidth;
		}

		memcpy(tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for (i = 0; i < CON_TEXTSIZE; i++)
		{
			con.text[i] = (ColorIndex(CONSOLE_COLOR) << 8) | ' ';
		}

		for (i = 0 ; i < numlines ; i++)
		{
			for (j = 0 ; j < numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
				    tbuf[((con.current - i + oldtotallines) %
				          oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}

/*
==================
Cmd_CompleteTxtName
==================
*/
void Cmd_CompleteTxtName(char *args, int argNum)
{
	if (argNum == 2)
	{
		Field_CompleteFilename("", "txt", qfalse, qtrue);
	}
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	int i;

	con_notifytime = Cvar_Get("con_notifytime", "7", 0);   // increased per id req for obits
	con_conspeed   = Cvar_Get("scr_conspeed", "3", 0);
	con_autoclear  = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE);

	Field_Clear(&g_consoleField);
	g_consoleField.widthInChars = g_console_field_width;
	for (i = 0 ; i < COMMAND_HISTORY ; i++)
	{
		Field_Clear(&historyEditLines[i]);
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	// clMessageMode: deprecated in favor of cgame/ui based version
	Cmd_AddCommand("clMessageMode", Con_MessageMode_f);
	Cmd_AddCommand("clMessageMode2", Con_MessageMode2_f);
	Cmd_AddCommand("clMessageMode3", Con_MessageMode3_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);
	Cmd_SetCommandCompletionFunc("condump", Cmd_CompleteTxtName);
}

/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed(qboolean skipnotify)
{
	int i;

	// mark time for transparent overlay
	if (con.current >= 0)
	{
		if (skipnotify)
		{
			con.times[con.current % NUM_CON_TIMES] = 0;
		}
		else
		{
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}

	con.x = 0;
	if (con.display == con.current)
	{
		con.display++;
	}
	con.current++;
	for (i = 0; i < con.linewidth; i++)
		con.text[(con.current % con.totallines) * con.linewidth + i] = (ColorIndex(CONSOLE_COLOR) << 8) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
#if defined(_WIN32) && defined(NDEBUG)
#pragma optimize( "g", off ) // SMF - msvc totally screws this function up with optimize on
#endif

void CL_ConsolePrint(char *txt)
{
	int      y;
	int      c, l;
	int      color;
	qboolean skipnotify = qfalse;
	int      prev;

	// work around for text that shows up in console but not in notify
	if (!Q_strncmp(txt, "[skipnotify]", 12))
	{
		skipnotify = qtrue;
		txt       += 12;
	}

	// for some demos we don't want to ever show anything on the console
	if (cl_noprint && cl_noprint->integer)
	{
		return;
	}

	if (!con.initialized)
	{
		con.color[0]             =
		    con.color[1]         =
		        con.color[2]     =
		            con.color[3] = 1.0f;
		con.linewidth            = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}

	color = ColorIndex(CONSOLE_COLOR);

	while ((c = *txt) != 0)
	{
		if (Q_IsColorString(txt))
		{
			if (*(txt + 1) == COLOR_NULL)
			{
				color = ColorIndex(CONSOLE_COLOR);
			}
			else
			{
				color = ColorIndex(*(txt + 1));
			}
			txt += 2;
			continue;
		}

		// count word length
		for (l = 0 ; l < con.linewidth ; l++)
		{
			if (txt[l] <= ' ')
			{
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth))
		{
			Con_Linefeed(skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed(skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:    // display character and advance
			y = con.current % con.totallines;
			// rain - sign extension caused the character to carry over
			// into the color info for high ascii chars; casting c to unsigned
			con.text[y * con.linewidth + con.x] = (color << 8) | (unsigned char)c;
			con.x++;
			if (con.x >= con.linewidth)
			{

				Con_Linefeed(skipnotify);
				con.x = 0;
			}
			break;
		}
	}

	// mark time for transparent overlay
	if (con.current >= 0)
	{
		if (skipnotify)
		{
			prev = con.current % NUM_CON_TIMES - 1;
			if (prev < 0)
			{
				prev = NUM_CON_TIMES - 1;
			}
			con.times[prev] = 0;
		}
		else
		{
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}
	}
}

#if defined(_WIN32) && defined(NDEBUG)
#pragma optimize( "g", on ) // SMF - re-enabled optimization
#endif

/*
==============================================================================
DRAWING
==============================================================================
*/

/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput(void)
{
	int y;

	if (cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE))
	{
		return;
	}

	y = con.vislines - (SMALLCHAR_HEIGHT * 2);

	// hightlight the current autocompleted part
	if (con.acLength)
	{
		Cmd_TokenizeString(g_consoleField.buffer);

		if (strlen(Cmd_Argv(0)) - con.acLength > 0)
		{
			re.SetColor(console_highlightcolor);
			re.DrawStretchPic(con.xadjust + (2 + con.acLength) * SMALLCHAR_WIDTH,
			                  y + 2,
			                  (strlen(Cmd_Argv(0)) - con.acLength) * SMALLCHAR_WIDTH,
			                  SMALLCHAR_HEIGHT - 2, 0, 0, 0, 0, cls.whiteShader);
		}
	}

	re.SetColor(con.color);

	SCR_DrawSmallChar(con.xadjust + 1 * SMALLCHAR_WIDTH, y, ']');

	Field_Draw(&g_consoleField, con.xadjust + 2 * SMALLCHAR_WIDTH, y,
	           SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue, qtrue);
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify(void)
{
	int   x, v = 0;
	short *text;
	int   i;
	int   time;
	int   currentColor = 7;

	re.SetColor(g_color_table[currentColor]);

	for (i = con.current - NUM_CON_TIMES + 1 ; i <= con.current ; i++)
	{
		if (i < 0)
		{
			continue;
		}
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
		{
			continue;
		}
		time = cls.realtime - time;
		if (time > con_notifytime->value * 1000)
		{
			continue;
		}
		text = con.text + (i % con.totallines) * con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)))
		{
			continue;
		}

		for (x = 0 ; x < con.linewidth ; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}
			if (((text[x] >> 8) & COLOR_BITS) != currentColor)
			{
				currentColor = (text[x] >> 8) & COLOR_BITS;
				re.SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar(cl_conXOffset->integer + con.xadjust + (x + 1) * SMALLCHAR_WIDTH, v, text[x] & 0xff);
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor(NULL);

	if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))
	{
		return;
	}

	// draw the chat line
	if (cls.keyCatchers & KEYCATCH_MESSAGE)
	{
		int skip;

		if (chat_team)
		{
			char buf[128];
			CL_TranslateString("say_team:", buf);
			SCR_DrawBigString(8, v, buf, 1.0f, qfalse);
			skip = strlen(buf) + 2;
		}
		else if (chat_buddy)
		{
			char buf[128];
			CL_TranslateString("say_fireteam:", buf);
			SCR_DrawBigString(8, v, buf, 1.0f, qfalse);
			skip = strlen(buf) + 2;
		}
		else
		{
			char buf[128];
			CL_TranslateString("say:", buf);
			SCR_DrawBigString(8, v, buf, 1.0f, qfalse);
			skip = strlen(buf) + 1;
		}

		Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v,
		              SCREEN_WIDTH - (skip + 1) * BIGCHAR_WIDTH, qtrue, qtrue);

		v += BIGCHAR_HEIGHT;
	}
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole(float frac)
{
	int    i, x, y;
	int    rows;
	short  *text;
	int    row;
	int    lines = cls.glconfig.vidHeight * frac;
	int    currentColor;
	vec4_t color;
	char   version[256] = ET_VERSION;

	if (lines <= 0)
	{
		return;
	}

	if (lines > cls.glconfig.vidHeight)
	{
		lines = cls.glconfig.vidHeight;
	}

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640(&con.xadjust, NULL, NULL, NULL);

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if (y < 1)
	{
		y = 0;
	}
	else
	{
		SCR_DrawPic(0, 0, SCREEN_WIDTH, y, cls.consoleShader);

		// merged from WolfSP
		if (frac >= 0.5f)
		{
			color[0] = color[1] = color[2] = frac * 2.0f;
			color[3] = 1.0f;
			re.SetColor(color);

			// draw the logo
			SCR_DrawPic(192, 70, 256, 128, cls.consoleShader2);
			re.SetColor(NULL);
		}
	}

	// matching light text
	color[0] = 0.75f;
	color[1] = 0.75f;
	color[2] = 0.75f;
	color[3] = 1.0f;
	if (frac < 1)
	{
		SCR_FillRect(0, y, SCREEN_WIDTH, 1.25f, color);
	}

	// draw the version number

	re.SetColor(g_color_table[ColorIndex(CONSOLE_COLOR)]);

	if (Cvar_VariableIntegerValue("com_updateavailable"))
	{
		Com_sprintf(version, sizeof(version), _("%s (UPDATE AVAILABLE)"), ET_VERSION);
	}

	i = strlen(version);

	for (x = 0 ; x < i ; x++)
	{
		if (x > strlen(ET_VERSION))
		{
			re.SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
		}
		SCR_DrawSmallChar(cls.glconfig.vidWidth - (i - x) * SMALLCHAR_WIDTH,
		                  (lines - (SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2)), version[x]);
	}

	// draw the text
	con.vislines = lines;
	rows         = (lines - SMALLCHAR_WIDTH) / SMALLCHAR_WIDTH; // rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT * 3);

	// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		re.SetColor(g_color_table[ColorIndex(COLOR_WHITE)]);
		for (x = 0 ; x < con.linewidth ; x += 4)
			SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, '^');
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = con.display;

	if (con.x == 0)
	{
		row--;
	}

	currentColor = 7;
	re.SetColor(g_color_table[currentColor]);

	for (i = 0 ; i < rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
		{
			break;
		}
		if (con.current - row >= con.totallines)
		{
			// past scrollback wrap point
			continue;
		}

		text = con.text + (row % con.totallines) * con.linewidth;

		for (x = 0 ; x < con.linewidth ; x++)
		{
			if ((text[x] & 0xff) == ' ')
			{
				continue;
			}

			if (((text[x] >> 8) & COLOR_BITS) != currentColor)
			{
				currentColor = (text[x] >> 8) & COLOR_BITS;
				re.SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar(con.xadjust + (x + 1) * SMALLCHAR_WIDTH, y, text[x] & 0xff);
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();

	re.SetColor(NULL);
}

extern cvar_t *con_drawnotify;

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole(void)
{
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// if disconnected, render console full screen
	if (cls.state == CA_DISCONNECTED)
	{
		if (!(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)))
		{
			Con_DrawSolidConsole(1.0f);
			return;
		}
	}

	if (con.displayFrac)
	{
		Con_DrawSolidConsole(con.displayFrac);
	}
	else
	{
		// draw notify lines
		if (cls.state == CA_ACTIVE && con_drawnotify->integer)
		{
			Con_DrawNotify();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole(void)
{
	// decide on the destination height of the console
	// short console support via shift+~
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		con.finalFrac = con.desiredFrac;
	}
	else
	{
		con.finalFrac = 0;  // none visible

	}
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value * cls.realFrametime * 0.001;
		if (con.finalFrac > con.displayFrac)
		{
			con.displayFrac = con.finalFrac;
		}

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value * cls.realFrametime * 0.001;
		if (con.finalFrac < con.displayFrac)
		{
			con.displayFrac = con.finalFrac;
		}
	}
}

void Con_PageUp(void)
{
	con.display -= 2;
	if (con.current - con.display >= con.totallines)
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown(void)
{
	con.display += 2;
	if (con.display > con.current)
	{
		con.display = con.current;
	}
}

void Con_Top(void)
{
	con.display = con.totallines;
	if (con.current - con.display >= con.totallines)
	{
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom(void)
{
	con.display = con.current;
}

void Con_Close(void)
{
	if (!com_cl_running->integer)
	{
		return;
	}
	Field_Clear(&g_consoleField);
	Con_ClearNotify();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac    = 0;           // none visible
	con.displayFrac  = 0;
}
