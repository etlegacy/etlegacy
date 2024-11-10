/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file cl_console.c
 * @brief Ingame console
 * Must hold SHIFT + ~ to get console. CTRL + SHIFT + '~' opens a small console.
 */

#include "client.h"
#include "../qcommon/q_unicode.h"

#define CONSOLE_COLOR  COLOR_WHITE
#define DEFAULT_CONSOLE_WIDTH   158

int smallCharWidth = SMALLCHAR_WIDTH;
int smallCharHeight = SMALLCHAR_HEIGHT;

static int versionStringLen = 0;

console_t con;

cvar_t *con_notifytime;
cvar_t *con_openspeed;
cvar_t *con_autoclear;
cvar_t *con_background;
cvar_t *con_defaultHeight;
cvar_t *con_scale;

vec4_t console_highlightcolor = { 0.5f, 0.5f, 0.2f, 0.45f };

int Con_ConsoleFieldWidth(void)
{
	CL_SetConsoleScale(con_scale->value);

	if (!cls.glconfig.vidWidth)
	{
		return DEFAULT_CONSOLE_WIDTH;
	}

	// discard version string length if we're not drawing it
	if (versionStringLen > (cls.glconfig.vidWidth / smallCharWidth - 2) * 0.66f)
	{
		return (cls.glconfig.vidWidth / smallCharWidth - 3);
	}

	return (cls.glconfig.vidWidth / smallCharWidth - 2) - (versionStringLen ? versionStringLen + 3 : 0);
}

/**
 * @brief Toggle console
 */
void Con_ToggleConsole_f(void)
{
	qboolean ctrl = keys[K_LCTRL].down || keys[K_RCTRL].down;
	qboolean alt  = keys[K_LALT].down || keys[K_RALT].down;
	float    shortConsole;
	float    normalConsole;
	float    fullConsole;

	con.highlightOffset = 0;

	// persistent console input is more useful (added cvar)
	if (con_autoclear->integer)
	{
		Field_Clear(&g_consoleField);
	}

	g_consoleField.widthInChars = Con_ConsoleFieldWidth();

	shortConsole  = (4.0f * smallCharHeight) / cls.glconfig.vidHeight;
	normalConsole = Com_Clamp((4.0f * smallCharHeight) / cls.glconfig.vidHeight, 1.0f, con_defaultHeight->value);
	fullConsole   = 1.0f;

	Con_ClearNotify();

	// We clear the keys here to the +/- actions can finnish before modifying the key catcher
	// No, this causes things like +sprint, +forward, even vstr scripts like
	// crouch/sprint toggle to break whenever console is toggled
	// TODO: maybe there's a workaround for this that could be applied?
	// Key_ClearKeys();

	// multiple console size support
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		// check if the user wants to resize instead of close

		// short console
		if (ctrl && con.desiredFrac != shortConsole)
		{
			con.desiredFrac = shortConsole;
		}
		// full console
		else if (alt && con.desiredFrac != fullConsole)
		{
			con.desiredFrac = fullConsole;
		}
		else
		{
			cls.keyCatchers &= ~KEYCATCH_CONSOLE;
			con.desiredFrac  = 0.0f;
		}
	}
	else
	{
		cls.keyCatchers |= KEYCATCH_CONSOLE;

		// short console
		if (ctrl)
		{
			con.desiredFrac = shortConsole;
		}
		// full console
		else if (alt)
		{
			con.desiredFrac = fullConsole;
		}
		// normal half-screen console
		else
		{
			con.desiredFrac = normalConsole;
		}
	}
}

/**
 * @brief Clear console
 */
void Con_Clear_f(void)
{
	int i;

	for (i = 0; i < CON_TEXTSIZE; i++)
	{
		con.text[i]      = ' ';
		con.textColor[i] = ColorIndex(CONSOLE_COLOR);
	}

	con.totalLines = 0;

	Con_ScrollBottom();
}

/**
 * @brief Save content to a file
 */
void Con_Dump_f(void)
{
	int          l, x, i;
	unsigned int *line;
	fileHandle_t f;
	size_t       bufferlen;
	char         *buffer;
	char         filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: condump <filename>\n");
		return;
	}

	Q_strncpyz(filename, Cmd_Argv(1), sizeof(filename));
	COM_DefaultExtension(filename, sizeof(filename), ".txt");

	if (!COM_CompareExtension(filename, ".txt"))
	{
		Com_Printf("Con_Dump_f: Only the '.txt' extension is supported by this command!\n");
		return;
	}

	f = FS_FOpenFileWrite(filename);
	if (!f)
	{
		Com_Printf("ERROR: couldn't open %s.\n", filename);
		return;
	}

	Com_Printf("Dumped console text to %s.\n", filename);

	// skip empty lines
	for (l = con.current - con.maxTotalLines + 1; l <= con.current; l++)
	{
		line = con.text + (l % con.maxTotalLines) * con.linewidth;
		for (x = 0; x < con.linewidth; x++)
			if (line[x] != ' ')
			{
				break;
			}
		if (x != con.linewidth)
		{
			break;
		}
	}

#ifdef _WIN32
	bufferlen = con.linewidth + 3 * (int)sizeof(char);
#else
	bufferlen = con.linewidth + 2 * sizeof(char);
#endif

	buffer = Hunk_AllocateTempMemory(bufferlen);

	// write the remaining lines
	buffer[bufferlen - 1] = 0;
	for (; l <= con.current; l++)
	{
		line = con.text + (l % con.maxTotalLines) * con.linewidth;
		for (i = 0; i < con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x = con.linewidth - 1; x >= 0; x--)
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
		(void) FS_Write(buffer, strlen(buffer), f);
	}

	Hunk_FreeTempMemory(buffer);
	FS_FCloseFile(f);
}

/**
 * @brief Con_ClearNotify
 */
void Con_ClearNotify(void)
{
	int i;

	for (i = 0; i < NUM_CON_TIMES; i++)
	{

		con.times[i] = 0;

	}
}

/**
 * @brief Reformat the buffer if the line width has changed
 */
void Con_CheckResize(void)
{
	int   i, width, dateLen, archLen;
	int   tbuf[CON_TEXTSIZE];
	byte  tbuff[CON_TEXTSIZE];
	float scale = con_scale->value;

	CL_SetConsoleScale(scale);

	dateLen = Q_PrintStrlen(con.date) + 1;
	archLen = Q_PrintStrlen(con.arch) + 1;

	width  = (cls.glconfig.vidWidth / smallCharWidth) - 2;
	width -= dateLen >= archLen ? dateLen : archLen;

	if (width == con.linewidth && !con_scale->modified)
	{
		return;
	}

	if (width < 1)            // video hasn't been initialized yet
	{
		con.linewidth     = DEFAULT_CONSOLE_WIDTH * scale;
		con.maxTotalLines = CON_TEXTSIZE / con.linewidth;
		for (i = 0; i < CON_TEXTSIZE; i++)
		{
			con.text[i]      = ' ';
			con.textColor[i] = ColorIndex(CONSOLE_COLOR);
		}
		con.totalLines = 0;
	}
	else
	{
		int j;
		int oldtotalLines, numlines, numchars;
		int oldwidth = con.linewidth;

		con.linewidth     = width;
		oldtotalLines     = con.maxTotalLines;
		con.maxTotalLines = CON_TEXTSIZE / con.linewidth;
		numlines          = oldtotalLines;

		if (con.maxTotalLines < numlines)
		{
			numlines = con.maxTotalLines;
		}

		numchars = oldwidth;

		// FIXME: this does not correctly reformat the buffer when decreasing con_scale,
		//  only when increasing. If using != here instead to allow it to reformat on decrease,
		//  the output is totally messed up as linebreaks seem to not retain
		if (con.linewidth < numchars)
		{
			numchars = con.linewidth;
		}

		Com_Memcpy(tbuf, con.text, CON_TEXTSIZE * sizeof(int));
		Com_Memcpy(tbuff, con.textColor, CON_TEXTSIZE * sizeof(byte));
		for (i = 0; i < CON_TEXTSIZE; i++)
		{
			con.text[i]      = ' ';
			con.textColor[i] = ColorIndex(CONSOLE_COLOR);
		}

		for (i = 0; i < numlines; i++)
		{
			for (j = 0; j < numchars; j++)
			{
				con.text[(con.maxTotalLines - 1 - i) * con.linewidth + j] =
					tbuf[((con.current - i + oldtotalLines) %
					      oldtotalLines) * oldwidth + j];
				con.textColor[(con.maxTotalLines - 1 - i) * con.linewidth + j] =
					tbuff[((con.current - i + oldtotalLines) %
					       oldtotalLines) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	g_consoleField.widthInChars = Con_ConsoleFieldWidth();
	con.current                 = con.maxTotalLines - 1;
	con.bottomDisplayedLine     = con.current;
	con.scrollIndex             = con.current;
	con_scale->modified         = qfalse;
}

/**
 * @brief Complete file text name
 * @param args - unused
 * @param[in] argNum
 */
void Cmd_CompleteTxtName(char *args, int argNum)
{
	if (argNum == 2)
	{
		Field_CompleteFilename("", "txt", qfalse, qtrue);
	}
}

#define ETL_CONSOLE_HISTORY_FILE "etl-history"
#define ETL_CONSOLE_HISTORY_BUFFER_MAX 2048

/**
 * @brief Load console history from a local file
 */
static void Con_LoadConsoleHistory(void)
{
	fileHandle_t f;
	long         size;
	int          read, i, width = 0;
	char         *buffer;

	size = FS_SV_FOpenFileRead(ETL_CONSOLE_HISTORY_FILE, &f);
	if (!f)
	{
		Com_Printf(S_COLOR_YELLOW "Couldn't read %s.\n", ETL_CONSOLE_HISTORY_FILE);
		return;
	}

	if (size > ETL_CONSOLE_HISTORY_BUFFER_MAX * sizeof(char))
	{
		size = ETL_CONSOLE_HISTORY_BUFFER_MAX * sizeof(char);
	}
	else
	{
		size++;
	}

	buffer = Hunk_AllocateTempMemory(size);
	Com_Memset(buffer, 0, size);

	read = FS_Read(buffer, (int)size - 1, f);

	for (i = 0, historyLine = 0; i < read && historyLine < (COMMAND_HISTORY - 1);)
	{
		if (!buffer[i])
		{
			break;
		}

		if (buffer[i] == '\n')
		{
			// only increment the history line if the buffer is not empty
			if (historyEditLines[historyLine].buffer[0])
			{
				historyLine++;
			}
			i++;
			width = 5;
			continue;
		}

		width = Q_UTF8_Width(&buffer[i]);
		if (!width)
		{
			break;
		}

		Field_InsertChar(&historyEditLines[historyLine], (int) Q_UTF8_CodePoint(&buffer[i]), qfalse);
		i += width;
	}

	// if the history file did not end with a newline
	if (width < 5 && historyEditLines[historyLine].buffer[0])
	{
		historyLine++;
	}
	nextHistoryLine = historyLine;

	FS_FCloseFile(f);
	Hunk_FreeTempMemory(buffer);
}

/**
 * @brief Persist console history to a local file
 */
void Con_SaveConsoleHistory(void)
{
	int          i, start, current, len, lastLen;
	const char   *last = NULL;
	fileHandle_t f;

	f = FS_SV_FOpenFileWrite(ETL_CONSOLE_HISTORY_FILE);
	if (!f)
	{
		Com_Printf("Couldn't write %s.\n", ETL_CONSOLE_HISTORY_FILE);
		return;
	}

	start = (nextHistoryLine % COMMAND_HISTORY);
	for (i = (start + 1); i < (COMMAND_HISTORY + start); i++)
	{
		current = i % COMMAND_HISTORY;
		if (!*historyEditLines[current].buffer)
		{
			continue;
		}

		// trim trailing spaces
		len = (int)strlen(historyEditLines[current].buffer);
		while (len > 0 && historyEditLines[current].buffer[len - 1] == ' ')
		{
			len--;
		}

		// Do not write the same line twice
		if (last && !Q_stricmpn(last, historyEditLines[current].buffer, (len > lastLen ? len : lastLen)))
		{
			continue;
		}

		if (!FS_Write(historyEditLines[current].buffer, len, f))
		{
			Com_Printf("Couldn't write %s.\n", ETL_CONSOLE_HISTORY_FILE);
		}

		if (!FS_Write("\n", 1, f))
		{
			Com_Printf("Couldn't write %s.\n", ETL_CONSOLE_HISTORY_FILE);
		}
		last    = historyEditLines[current].buffer;
		lastLen = len;
	}

	FS_FCloseFile(f);
}

/**
 * @brief Initialize console
 */
void Con_Init(void)
{
	int i;

	con_notifytime = Cvar_Get("con_notifytime", "7", 0);    // increased per id req for obits
	con_openspeed  = Cvar_Get("con_openspeed", "5", 0);
	con_autoclear  = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE_ND);
	Cvar_Get("con_fontName", "JetBrainsMono-SemiBold", CVAR_LATCH | CVAR_ARCHIVE_ND);

	con_background    = Cvar_GetAndDescribe("con_background", "", CVAR_ARCHIVE, "Console background color in normalized RGBA format, eg. \"0.2 0.2 0.2 0.8\".");
	con_defaultHeight = Cvar_GetAndDescribe("con_defaultHeight", "0.5", CVAR_ARCHIVE_ND, "Default console height without key modifiers.");

	con_scale = Cvar_GetAndDescribe("con_scale", "1.0", CVAR_ARCHIVE_ND, "Scaling factor for console font size.");
	Cvar_CheckRange(con_scale, 0.5f, 4.0f, qfalse);

	Field_Clear(&g_consoleField);
	g_consoleField.widthInChars = Con_ConsoleFieldWidth();
	for (i = 0; i < COMMAND_HISTORY; i++)
	{
		Field_Clear(&historyEditLines[i]);
		historyEditLines[i].widthInChars = g_consoleField.widthInChars;
	}

	Con_LoadConsoleHistory();

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f, "Toggles the console.");
	Cmd_AddCommand("clear", Con_Clear_f, "Clears console content.");
	Cmd_AddCommand("condump", Con_Dump_f, "Dumps console content to disk.", Cmd_CompleteTxtName);
}

/**
 * @brief Free client console commands before shutdown
 */
void Con_Shutdown(void)
{
	Cmd_RemoveCommand("toggleconsole");
	Cmd_RemoveCommand("clear");
	Cmd_RemoveCommand("condump");
}

/**
 * @brief Line feed
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

	if (con.scrollIndex >= con.current)
	{
		con.scrollIndex++;
	}

	con.current++;

	if (con.totalLines < con.maxTotalLines)
	{
		con.totalLines++;
	}

	for (i = 0; i < con.linewidth; i++)
	{
		con.text[(con.current % con.maxTotalLines) * con.linewidth + i]      = ' ';
		con.textColor[(con.current % con.maxTotalLines) * con.linewidth + i] = ColorIndex(CONSOLE_COLOR);
	}
}


#if defined(_WIN32) && !defined(ETLEGACY_DEBUG)
#pragma optimize( "g", off ) // msvc totally screws this function up with optimize on
#endif

/**
 * @brief Handles cursor positioning, line wrapping, etc
 *
 * @param[in] txt
 */
void CL_ConsolePrint(char *txt)
{
	int      y;
	int      c, l;
	int      color;
	qboolean skipnotify = qfalse;
	int      prev;

	if (cls.clipboard.buffer)
	{
		Q_strcat(cls.clipboard.buffer, cls.clipboard.bufferSize, txt);
	}

	// for some demos we don't want to ever show anything on the console
	if (cl_noprint && cl_noprint->integer)
	{
		return;
	}

	if (!con.initialized)
	{
		static cvar_t null_cvar = { 0 };
		con.color[0]        = con.color[1] = con.color[2] = con.color[3] = 1.0f;
		con.linewidth       = -1;
		con.scale           = 1.0f;
		con_scale           = &null_cvar;
		con_scale->value    = 1.0f;
		con_scale->modified = qtrue;
		Con_CheckResize();
		con.initialized = qtrue;
	}

	// work around for text that shows up in console but not in notify
	if (!Q_strncmp(txt, "[skipnotify]", 12))
	{
		skipnotify = qtrue;
		txt       += 12;
	}

	color = ColorIndex(CONSOLE_COLOR);

	while (*txt != 0)
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
		for (l = 0; l < con.linewidth; l++)
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

		c = Q_UTF8_CodePoint(txt);

		switch (c)
		{
		case '\n':
			Con_Linefeed(skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:
			// display character and advance
			y = con.current % con.maxTotalLines;

			// sign extension caused the character to carry over
			// into the color info for high ascii chars; casting c to unsigned
			con.text[y * con.linewidth + con.x]      = c;
			con.textColor[y * con.linewidth + con.x] = color;
			con.x++;
			if (con.x >= con.linewidth)
			{
				Con_Linefeed(skipnotify);
				con.x = 0;
			}
			break;
		}

		txt += Q_UTF8_Width(txt);
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

#if defined(_WIN32) && !defined(ETLEGACY_DEBUG)
#pragma optimize( "g", on ) // re-enabled optimization
#endif

/**
 * @brief Draw version text
 */
void Con_DrawVersion(void)
{
	int               x, y, versionLen, envoiOffset = 1;
	static const char *devBuild = NULL;
	Q_strncpyz(con.version, Q3_VERSION, sizeof(con.version));
	Q_strncpyz(con.date, __DATE__, sizeof(con.date));
	Q_strncpyz(con.arch, CPUSTRING, sizeof(con.arch));

	if (com_updateavailable->integer)
	{
		Q_strcat(con.version, sizeof(con.version), " ^2(UPDATE AVAILABLE)");
	}

	// Add DEVELOPMENTAL BUILD banner to console
	if (devBuild == NULL && ETLEGACY_VERSION_IS_DEVELOPMENT_BUILD)
	{
		devBuild = "^8DEVELOPMENT BUILD";
	}
	if (devBuild != NULL)
	{
		envoiOffset = 2;
	}

#ifdef ETLEGACY_DEBUG
	Q_strcat(con.arch, sizeof(con.arch), " ^3DEBUG");
#endif

	// poor man's widescreen correction, use % of screen width instead of absolute values
	// to retain position with different aspect ratios
	x = cls.glconfig.vidWidth - (cls.glconfig.vidWidth * 0.0075f);
	y = con.scanLines;

	versionLen = Q_PrintStrlen(con.version) + 1;

	// if version string would take up over 2/3 of the entire line due to huge con_scale,
	// just drop the version drawing completely so there's some space to write
	if ((cls.glconfig.vidWidth / smallCharWidth - 2) * 0.66f > versionLen)
	{
		SCR_DrawSmallString(x - (versionLen * smallCharWidth), y - (1.25f * smallCharHeight * envoiOffset), con.version, colorWhite, qfalse, qfalse);
	}

	SCR_DrawSmallString(x - (ARRAY_LEN(__DATE__) * smallCharWidth), y - (1.25f * (smallCharHeight * (envoiOffset + 1))), __DATE__, colorWhite, qfalse, qfalse);
	SCR_DrawSmallString(x - ((Q_PrintStrlen(con.arch) + 1) * smallCharWidth), y - (1.25f * (smallCharHeight * (envoiOffset + 2))), con.arch, colorWhite, qfalse, qfalse);

	if (devBuild != NULL)
	{
		int devBuildLen = Q_PrintStrlen(devBuild) + 1;
		SCR_DrawSmallString(x - (devBuildLen * smallCharWidth), y - (1.25f * (smallCharHeight * (1))), devBuild, colorWhite, qfalse, qfalse);
	}
}

/**
 * @brief Draw system clock
 */
void Con_DrawClock(void)
{
	int       x, y;
	char      clock[6];
	time_t    longTime;
	struct tm *localTime;

	// get date from system
	time(&longTime);
	localTime = localtime(&longTime);

	// poor man's widescreen correction, use % of screen width instead of absolute values
	// to retain position with different aspect ratios
	x = cls.glconfig.vidWidth - (cls.glconfig.vidWidth * 0.0075f);
	y = SMALLCHAR_HEIGHT / 2;

	Com_sprintf(clock, sizeof(clock), "%02d%c%02d", localTime->tm_hour, localTime->tm_sec & 1 ? ':' : ' ', localTime->tm_min);
	SCR_DrawSmallString(x - (ARRAY_LEN(clock) * smallCharWidth), y, clock, colorMdGrey, qfalse, qfalse);
}

/**
 * @brief Draw the editline after a ] prompt
 */
void Con_DrawInput(void)
{
	int y;

	if (cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE))
	{
		return;
	}

	y = con.scanLines - 1.25f * smallCharHeight;

	// highlight the current autocompleted part
	if (con.highlightOffset && g_consoleField.buffer[0])
	{
		re.SetColor(console_highlightcolor);
		re.DrawStretchPic((2 + con.highlightOffset) * smallCharWidth,
		                  y + 2,
		                  (strlen(g_consoleField.buffer) - con.highlightOffset) * smallCharWidth,
		                  smallCharHeight - 2, 0, 0, 0, 0, cls.whiteShader);
	}

	re.SetColor(con.color);

	SCR_DrawSmallChar(smallCharWidth, y, ']');

	Field_Draw(&g_consoleField, 2 * smallCharWidth, y,
	           smallCharWidth - 3 * smallCharWidth, qtrue, qtrue);
}

extern cvar_t *con_numNotifies;
#define clamp(min, max, value) ((value < min) ? min : (value > max) ? max : value)

/**
 * @brief Draws the last few lines of output transparently over the game top
 */
void Con_DrawNotify(void)
{
	int          x, v = 0;
	unsigned int *text;
	byte         *textColor;
	int          i;
	int          time;
	int          currentColor = 7;
	int          maxNotifies;

	maxNotifies = clamp(0, NUM_CON_TIMES, con_numNotifies->integer);
	if (maxNotifies == 0)
	{
		return;
	}

	re.SetColor(g_color_table[currentColor]);

	for (i = con.current - maxNotifies + 1; i <= con.current; i++)
	{
		if (i < 0)
		{
			continue;
		}

		time = con.times[i % maxNotifies];

		if (time == 0)
		{
			continue;
		}

		time = cls.realtime - time;

		if (time > con_notifytime->value * 1000)
		{
			continue;
		}

		text      = con.text + (i % con.maxTotalLines) * con.linewidth;
		textColor = con.textColor + (i % con.maxTotalLines) * con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)))
		{
			continue;
		}

		for (x = 0; x < con.linewidth; x++)
		{
			if ((text[x]) == ' ')
			{
				continue;
			}

			if (textColor[x] != currentColor)
			{
				currentColor = textColor[x];
				re.SetColor(g_color_table[currentColor]);
			}

			SCR_DrawSmallChar(cl_conXOffset->integer + (x + 1) * smallCharWidth, v, text[x]);
		}

		v += smallCharHeight;
	}

	re.SetColor(NULL);
}

#undef clamp

/**
 * @brief Draw scrollbar
 * @param[in] length
 * @param[in] x
 * @param[in] y
 */
void Con_DrawScrollbar(int length, float x, float y)
{
	vec4_t      color          = { 0.2f, 0.2f, 0.2f, 0.75f };
	const float width          = 1.0f;
	const float handleLength   = con.totalLines ? length * MIN(1.0f, (float) con.visibleLines / con.totalLines) : 0;
	const float lengthPerLine  = (length - handleLength) / (con.totalLines - con.visibleLines);
	const float relativeScroll = con.current - con.totalLines + MIN(con.visibleLines, con.totalLines);
	const float handlePosition = lengthPerLine * (con.bottomDisplayedLine - relativeScroll);

	SCR_FillRect(x, y, width, length, color);

	color[0] = 0.5f;
	color[1] = 0.5f;
	color[2] = 0.5f;
	color[3] = 1.0f;

	// draw the handle
	if (handlePosition >= 0 && handleLength > 0)
	{
		SCR_FillRect(x, y + handlePosition, width, handleLength, color);
	}
	// this happens when line appending gets us over the top position in a roll-lock situation (scrolling itself won't do that)
	else if (con.totalLines)
	{
		SCR_FillRect(x, y, width, handleLength, color);
	}
}

/**
 * @brief Draws the console with the solid background
 * @param[in] frac
 */
void Con_DrawSolidConsole(float frac)
{
	int          i, x, y;
	unsigned int *text;
	byte         *textColor;
	int          row;
	int          currentColor;
	vec4_t       color;

	if (frac <= 0)
	{
		return;
	}

	if (frac > 1)
	{
		frac = 1;
	}

	con.scanLines = frac * cls.glconfig.vidHeight;

	// draw the background
	y = frac * SCREEN_HEIGHT;

	if (y < 1)
	{
		y = 0;
	}
	else
	{
		static vec4_t   consoleParsedBackgroundColor = { 0.f, 0.f, 0.f, 0.f };
		static qboolean isParsed                     = qfalse;

		if (con_background->modified)
		{
			con_background->modified = qfalse;
			isParsed                 = Q_ParseColor(con_background->string, consoleParsedBackgroundColor);
		}

		if (isParsed)
		{
			SCR_FillRect(0, 0, SCREEN_WIDTH, y, consoleParsedBackgroundColor);
		}
		else
		{
			SCR_DrawPic(0, 0, SCREEN_WIDTH, y, cls.consoleShader);
		}
		/*
		// draw the logo
		if (frac >= 0.5f)
		{
		    color[0] = color[1] = color[2] = frac * 2.0f;
		    color[3] = 1.0f;
		    re.SetColor(color);

		    SCR_DrawPic(192, 70, 256, 128, cls.consoleShader2);
		    re.SetColor(NULL);
		}
		*/
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

	re.SetColor(g_color_table[ColorIndex(CONSOLE_COLOR)]);

	// draw the input prompt, user text, and cursor
	Con_DrawInput();
	// draw scrollbar
	Con_DrawScrollbar(y - 6, SCREEN_WIDTH - 5, 3);
	// draw the version number
	Con_DrawVersion();
	// draw system clock
	Con_DrawClock();

	// draw text
	con.visibleLines = (con.scanLines - smallCharHeight) / smallCharHeight - 1;  // rows of text to draw

	y = con.scanLines - 3 * smallCharHeight;

	// draw from the bottom up
	if (con.scrollIndex < con.current - 1)
	{
		// draw arrows to show the buffer is backscrolled
		re.SetColor(g_color_table[ColorIndex(COLOR_WHITE)]);

		color[3] = 0.50f;
		re.SetColor(color);

		for (x = 0; x < con.linewidth; x += 4)
		{
			SCR_DrawSmallChar((x + 1) * smallCharWidth, y + 0.75f * smallCharHeight, '^');
		}

		re.SetColor(NULL);
	}

	row = con.bottomDisplayedLine;

	if (con.x == 0)
	{
		row--;
	}

	currentColor = 7;
	re.SetColor(g_color_table[currentColor]);

	for (i = 0; i < con.visibleLines; i++, y -= smallCharHeight, row--)
	{
		if (row < 0)
		{
			break;
		}

		if (con.current - row >= con.maxTotalLines)
		{
			// past scrollback wrap point
			continue;
		}

		text      = con.text + (row % con.maxTotalLines) * con.linewidth;
		textColor = con.textColor + (row % con.maxTotalLines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
		{
			if (text[x] == ' ')
			{
				continue;
			}

			if (textColor[x] != currentColor)
			{
				currentColor = textColor[x];
				re.SetColor(g_color_table[currentColor]);
			}
			SCR_DrawSmallChar((x + 1) * smallCharWidth, y, text[x]);
		}
	}

	re.SetColor(NULL);
}

extern cvar_t *con_drawnotify;

/**
 * @brief Draw console
 */
void Con_DrawConsole(void)
{
	if (!con.displayFrac)
	{
		// draw notify lines
		if (cls.state == CA_ACTIVE && con_drawnotify->integer)
		{
			Con_DrawNotify();
		}

		// render console only if opened but also if disconnected
		if (!(cls.state == CA_DISCONNECTED && !(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))))
		{
			return;
		}
	}

	// check for console width changes from a vid mode change
	Con_CheckResize();

	Con_DrawSolidConsole(con.displayFrac);
}

/**
 * @brief Count and update the version string length for the console input field
 */
static void Con_UpdateVersionStringLen()
{
	const int len = Q_PrintStrlen(con.version);

	if (len != versionStringLen)
	{
		versionStringLen            = len;
		g_consoleField.widthInChars = Con_ConsoleFieldWidth();
	}
}

/**
 * @brief Scroll console up or down
 */
void Con_RunConsole(void)
{
	if (com_updateavailable->modified || versionStringLen == 0)
	{
		com_updateavailable->modified = qfalse;
		Con_UpdateVersionStringLen();
	}

	// decide on the destination height of the console
	// short console support via shift+~
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		con.finalFrac = con.desiredFrac;
	}
	else
	{
		con.finalFrac = 0.0f;  // none visible
	}

	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_openspeed->value * cls.realFrametime * 0.001f;

		if (con.finalFrac > con.displayFrac)
		{
			con.displayFrac = con.finalFrac;
		}
	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_openspeed->value * cls.realFrametime * 0.001f;

		if (con.finalFrac < con.displayFrac)
		{
			con.displayFrac = con.finalFrac;
		}
	}

	// animated scroll
	if (con.displayFrac > 0)
	{
		const float scrolldiff   = MAX(0.5f, abs(con.bottomDisplayedLine - con.scrollIndex));
		int         nudgingValue = con_openspeed->value * cls.realFrametime * 0.005f * scrolldiff;

		// nudge might turn out to be 0 so just bump it to 1 so we actually move towards our goal
		if (nudgingValue <= 0)
		{
			nudgingValue = 1;
		}

		if (con.bottomDisplayedLine < con.scrollIndex)
		{
			con.bottomDisplayedLine += nudgingValue;

			if (con.bottomDisplayedLine > con.scrollIndex)
			{
				con.bottomDisplayedLine = con.scrollIndex;
			}
		}
		else if (con.bottomDisplayedLine > con.scrollIndex)
		{
			con.bottomDisplayedLine -= nudgingValue;

			if (con.bottomDisplayedLine < con.scrollIndex)
			{
				con.bottomDisplayedLine = con.scrollIndex;
			}
		}
	}
	else
	{
		con.bottomDisplayedLine = con.scrollIndex;
	}
}

/**
 * @brief Check that the console does not go over the buffer limits
 */
static void Con_CheckLimits(void)
{
	// going up
	if (con.scrollIndex < con.current - con.totalLines + con.visibleLines)
	{
		con.scrollIndex = con.current - con.totalLines + con.visibleLines;
	}

	// going down
	if (con.scrollIndex > con.current)
	{
		con.scrollIndex = con.current;
	}
}

/**
 * @brief Page up
 */
void Con_PageUp(void)
{
	Con_ScrollUp(con.visibleLines / 2);
}

/**
 * @brief Page down
 */
void Con_PageDown(void)
{
	Con_ScrollDown(con.visibleLines / 2);
}

/**
 * @brief Scroll up
 */
void Con_ScrollUp(int lines)
{
	con.scrollIndex -= lines;
	Con_CheckLimits();
}

/**
 * @brief Scroll down
 */
void Con_ScrollDown(int lines)
{
	con.scrollIndex += lines;
	Con_CheckLimits();
}

/**
 * @brief Scroll to top
 */
void Con_ScrollTop(void)
{
	con.scrollIndex = con.current - con.totalLines + con.visibleLines;
	Con_CheckLimits();
}

/**
 * @brief Scroll to bottom
 */
void Con_ScrollBottom(void)
{
	con.scrollIndex = con.current;
	Con_CheckLimits();
}

/**
 * @brief Close console
 */
void Con_Close(void)
{
	if (!com_cl_running->integer)
	{
		return;
	}

	if (com_developer->integer > 1)
	{
		return;
	}

	Field_Clear(&g_consoleField);
	Con_ClearNotify();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac    = 0;
	con.displayFrac  = 0;
}
