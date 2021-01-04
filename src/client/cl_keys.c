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
 * @file cl_keys.c
 */

#include "client.h"
#include "../qcommon/q_unicode.h"
#include "../sdl/sdl_defs.h"

/*
key up events are sent even if in console mode
*/

field_t historyEditLines[COMMAND_HISTORY];

int nextHistoryLine;        // the last line in the history buffer, not masked
int historyLine;            // the line being displayed from history buffer
                            // will be <= nextHistoryLine

field_t g_consoleField;

qboolean key_overstrikeMode;

int    anykeydown;
qkey_t keys[MAX_KEYS];

typedef struct
{
	char *name;
	int keynum;
} keyname_t;

qboolean UI_checkKeyExec(int key);
qboolean CL_CGameCheckKeyExec(int key);

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
keyname_t keynames[] =
{
	{ "TAB",             K_TAB            },
	{ "ENTER",           K_ENTER          },
	{ "ESCAPE",          K_ESCAPE         },
	{ "SPACE",           K_SPACE          },
	{ "BACKSPACE",       K_BACKSPACE      },
	{ "UPARROW",         K_UPARROW        },
	{ "DOWNARROW",       K_DOWNARROW      },
	{ "LEFTARROW",       K_LEFTARROW      },
	{ "RIGHTARROW",      K_RIGHTARROW     },

	{ "RIGHTALT",        K_RALT           },
	{ "LEFTALT",         K_LALT           },
	{ "ALT",             K_LALT           },

	{ "RIGHTCTRL",       K_RCTRL          },
	{ "LEFTCTRL",        K_LCTRL          },
	{ "CTRL",            K_LCTRL          },

	{ "RIGHTSHIFT",      K_RSHIFT         },
	{ "LEFTSHIFT",       K_LSHIFT         },
	{ "SHIFT",           K_LSHIFT         },

	{ "COMMAND",         K_COMMAND        },

	{ "CAPSLOCK",        K_CAPSLOCK       },

	{ "F1",              K_F1             },
	{ "F2",              K_F2             },
	{ "F3",              K_F3             },
	{ "F4",              K_F4             },
	{ "F5",              K_F5             },
	{ "F6",              K_F6             },
	{ "F7",              K_F7             },
	{ "F8",              K_F8             },
	{ "F9",              K_F9             },
	{ "F10",             K_F10            },
	{ "F11",             K_F11            },
	{ "F12",             K_F12            },
	{ "F13",             K_F13            },
	{ "F14",             K_F14            },
	{ "F15",             K_F15            },

	{ "INS",             K_INS            },
	{ "DEL",             K_DEL            },
	{ "PGDN",            K_PGDN           },
	{ "PGUP",            K_PGUP           },
	{ "HOME",            K_HOME           },
	{ "END",             K_END            },

	{ "MOUSE1",          K_MOUSE1         },
	{ "MOUSE2",          K_MOUSE2         },
	{ "MOUSE3",          K_MOUSE3         },
	{ "MOUSE4",          K_MOUSE4         },
	{ "MOUSE5",          K_MOUSE5         },

	{ "MWHEELUP",        K_MWHEELUP       },
	{ "MWHEELDOWN",      K_MWHEELDOWN     },

	{ "JOY1",            K_JOY1           },
	{ "JOY2",            K_JOY2           },
	{ "JOY3",            K_JOY3           },
	{ "JOY4",            K_JOY4           },
	{ "JOY5",            K_JOY5           },
	{ "JOY6",            K_JOY6           },
	{ "JOY7",            K_JOY7           },
	{ "JOY8",            K_JOY8           },
	{ "JOY9",            K_JOY9           },
	{ "JOY10",           K_JOY10          },
	{ "JOY11",           K_JOY11          },
	{ "JOY12",           K_JOY12          },
	{ "JOY13",           K_JOY13          },
	{ "JOY14",           K_JOY14          },
	{ "JOY15",           K_JOY15          },
	{ "JOY16",           K_JOY16          },
	{ "JOY17",           K_JOY17          },
	{ "JOY18",           K_JOY18          },
	{ "JOY19",           K_JOY19          },
	{ "JOY20",           K_JOY20          },
	{ "JOY21",           K_JOY21          },
	{ "JOY22",           K_JOY22          },
	{ "JOY23",           K_JOY23          },
	{ "JOY24",           K_JOY24          },
	{ "JOY25",           K_JOY25          },
	{ "JOY26",           K_JOY26          },
	{ "JOY27",           K_JOY27          },
	{ "JOY28",           K_JOY28          },
	{ "JOY29",           K_JOY29          },
	{ "JOY30",           K_JOY30          },
	{ "JOY31",           K_JOY31          },
	{ "JOY32",           K_JOY32          },

	{ "AUX1",            K_AUX1           },
	{ "AUX2",            K_AUX2           },
	{ "AUX3",            K_AUX3           },
	{ "AUX4",            K_AUX4           },
	{ "AUX5",            K_AUX5           },
	{ "AUX6",            K_AUX6           },
	{ "AUX7",            K_AUX7           },
	{ "AUX8",            K_AUX8           },
	{ "AUX9",            K_AUX9           },
	{ "AUX10",           K_AUX10          },
	{ "AUX11",           K_AUX11          },
	{ "AUX12",           K_AUX12          },
	{ "AUX13",           K_AUX13          },
	{ "AUX14",           K_AUX14          },
	{ "AUX15",           K_AUX15          },
	{ "AUX16",           K_AUX16          },

	{ "KP_HOME",         K_KP_HOME        },
	{ "KP_UPARROW",      K_KP_UPARROW     },
	{ "KP_PGUP",         K_KP_PGUP        },
	{ "KP_LEFTARROW",    K_KP_LEFTARROW   },
	{ "KP_5",            K_KP_5           },
	{ "KP_RIGHTARROW",   K_KP_RIGHTARROW  },
	{ "KP_END",          K_KP_END         },
	{ "KP_DOWNARROW",    K_KP_DOWNARROW   },
	{ "KP_PGDN",         K_KP_PGDN        },
	{ "KP_ENTER",        K_KP_ENTER       },
	{ "KP_INS",          K_KP_INS         },
	{ "KP_DEL",          K_KP_DEL         },
	{ "KP_SLASH",        K_KP_SLASH       },
	{ "KP_MINUS",        K_KP_MINUS       },
	{ "KP_PLUS",         K_KP_PLUS        },
	{ "KP_NUMLOCK",      K_KP_NUMLOCK     },
	{ "KP_STAR",         K_KP_STAR        },
	{ "KP_EQUALS",       K_KP_EQUALS      },

	{ "PAUSE",           K_PAUSE          },

	{ "SEMICOLON",       ';'              }, // because a raw semicolon seperates commands

	{ "US_0",            K_0              },
	{ "US_1",            K_1              },
	{ "US_2",            K_2              },
	{ "US_3",            K_3              },
	{ "US_4",            K_4              },
	{ "US_5",            K_5              },
	{ "US_6",            K_6              },
	{ "US_7",            K_7              },
	{ "US_8",            K_8              },
	{ "US_9",            K_9              },

	{ "US_MINUS",        K_MINUS          },
	{ "US_EQUALS",       K_EQUALS         },
	{ "US_LEFTBRACKET",  K_LEFTBRACKET    },
	{ "US_RIGHTBRACKET", K_RIGHTBRACKET   },
	{ "US_BACKSLASH",    K_BACKSLASH      },
	{ "US_SEMICOLON",    K_SEMICOLON      },
	{ "US_APOSTROPHE",   K_APOSTROPHE     },
	{ "US_GRAVE",        K_GRAVE          },
	{ "US_COMMA",        K_COMMA          },
	{ "US_PERIOD",       K_PERIOD         },
	{ "US_SLASH",        K_SLASH          },
	{ "US_BACKSLASH",    K_NONUSBACKSLASH },

	{ "WINDOWS",         K_SUPER          },
	{ "COMPOSE",         K_COMPOSE        },
	{ "MODE",            K_MODE           },
	{ "HELP",            K_HELP           },
	{ "PRINT",           K_PRINT          },
	{ "SYSREQ",          K_SYSREQ         },
	{ "SCROLLOCK",       K_SCROLLOCK      },
	{ "BREAK",           K_BREAK          },
	{ "MENU",            K_MENU           },
	{ "POWER",           K_POWER          },
	{ "EURO",            K_EURO           },
	{ "UNDO",            K_UNDO           },

	{ NULL,              0                }
};

/*
=============================================================================
EDIT FIELDS
=============================================================================
*/

/**
 * @brief Handles horizontal scrolling and cursor blinking
 * x, y, and width are in pixels
 * @param[in,out] edit
 * @param[in] x
 * @param[in] y
 * @param[in] width - unused
 * @param[in] size
 * @param[in] showCursor
 * @param[in] noColorEscape
 */
void Field_VariableSizeDraw(field_t *edit, int x, int y, int width, int size, qboolean showCursor, qboolean noColorEscape)
{
	int  len     = strlen(edit->buffer);
	int  drawLen = edit->widthInChars;
	int  prestep;
	char str[MAX_STRING_CHARS];

	// guarantee that cursor will be visible
	if (len <= drawLen)
	{
		prestep = 0;
	}
	else
	{
		if (edit->scroll + drawLen > len)
		{
			edit->scroll = len - drawLen;
			if (edit->scroll < 0)
			{
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if (prestep + drawLen > len)
	{
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if (drawLen >= MAX_STRING_CHARS)
	{
		Com_Error(ERR_DROP, "drawLen >= MAX_STRING_CHARS");
		return;
	}

	Com_Memcpy(str, edit->buffer + prestep, drawLen);
	str[drawLen] = 0;

	// draw it
	SCR_DrawSmallString(x, y, str, colorWhite, qfalse, noColorEscape);

	// draw the cursor
	if (showCursor)
	{
		int cursorChar, i;

		if ((int)(cls.realtime >> 8) & 1)
		{
			return;     // off blink
		}

		if (key_overstrikeMode)
		{
			cursorChar = 11;
		}
		else
		{
			cursorChar = 10;
		}

		if (!noColorEscape)
		{
			Q_CleanStr(str);
		}
		i = drawLen - strlen(str);

		SCR_DrawSmallChar(x + (edit->cursor - prestep - i) * size, y, cursorChar);
	}
}

/**
 * @brief Field_Draw
 * @param[in] edit
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] showCursor
 * @param[in] noColorEscape
 */
void Field_Draw(field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape)
{
	Field_VariableSizeDraw(edit, x, y, width, SMALLCHAR_WIDTH, showCursor, noColorEscape);
}

/**
 * @brief Field_Paste
 * @param[in] edit
 */
void Field_Paste(field_t *edit)
{
	char         *cbd;
	size_t       pasteLen, i;

	cbd = IN_GetClipboardData();

	if (!cbd)
	{
		return;
	}

	// send as if typed, so insert / overstrike works properly
	pasteLen = Q_UTF8_Strlen(cbd);
	uint32_t *chars = Com_Allocate(sizeof(uint32_t) * pasteLen);
	Com_Memset(chars, 0, sizeof(uint32_t) * pasteLen);
	Q_UTF8_ToUTF32(cbd, chars, &pasteLen);

	for (i = 0 ; i < pasteLen ; i++)
	{
		Field_CharEvent(edit, chars[i]);
	}
	Com_Dealloc(chars);

	Z_Free(cbd);
}

/**
 * @brief Performs the basic line editing functions for the console,
 * in-game talk, and menu fields
 *
 * Key events are used for non-printable characters, others are gotten from char events.
 *
 * @param[in,out] edit
 * @param[in] key
 */
void Field_KeyDownEvent(field_t *edit, int key)
{
	size_t len, stringLen;

	// shift-insert is paste
	if (((key == K_INS) || (key == K_KP_INS)) && (keys[K_RSHIFT].down || keys[K_LSHIFT].down))
	{
		Field_Paste(edit);
		return;
	}

	key = tolower(key);
	len = strlen(edit->buffer);
	stringLen = Q_UTF8_Strlen(edit->buffer);

	switch (key)
	{
	case K_DEL:
	case K_KP_DEL:
		if (edit->cursor < stringLen)
		{
			int offset = Q_UTF8_ByteOffset(edit->buffer, edit->cursor);
			char *current = Q_UTF8_CharAt(edit->buffer, edit->cursor);
			int charWidth = Q_UTF8_Width(current);

			memmove(edit->buffer + offset,
			        edit->buffer + offset + charWidth, len - offset);
		}
		break;
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
		if (edit->cursor < stringLen)
		{
			edit->cursor++;
		}
		break;
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		if (edit->cursor > 0)
		{
			edit->cursor--;
		}
		break;
	case K_HOME:
	case K_KP_HOME:
		edit->cursor = 0;
		break;
	case 'a':
		if (keys[K_LCTRL].down || keys[K_RCTRL].down)
		{
			edit->cursor = 0;
		}
		break;
	case K_END:
	case K_KP_END:
		edit->cursor = stringLen;
		break;
	case 'e':
		if (keys[K_LCTRL].down || keys[K_RCTRL].down)
		{
			edit->cursor = stringLen;
		}
		break;
	case K_INS:
	case K_KP_INS:
		key_overstrikeMode = !key_overstrikeMode;
		break;
	default:
		break;
	}

	// Change scroll if cursor is no longer visible
	if (edit->cursor < edit->scroll)
	{
		edit->scroll = edit->cursor;
	}
	else if (edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len)
	{
		edit->scroll = edit->cursor - edit->widthInChars + 1;
	}
}

/**
 * @brief Field_CharEvent
 * @param[in,out] edit
 * @param[in] ch
 */
void Field_CharEvent(field_t *edit, int ch)
{
	int len, stringLen;
	//int charWidth;
	//char *value = NULL;

	//charWidth = Q_UTF8_WidthCP(ch);
	//value     = Q_UTF8_Encode(ch);

	if (ch == 'v' - 'a' + 1)      // ctrl-v is paste
	{
		Field_Paste(edit);
		return;
	}

	if (ch == 'c' - 'a' + 1)      // ctrl-c clears the field
	{
		Field_Clear(edit);
		return;
	}

	len       = strlen(edit->buffer);
	stringLen = Q_UTF8_Strlen(edit->buffer);

	if (ch == 'h' - 'a' + 1)          // ctrl-h is backspace
	{
		if (edit->cursor > 0)
		{
			int offset = Q_UTF8_ByteOffset(edit->buffer, edit->cursor);
			char *prev = Q_UTF8_CharAt(edit->buffer, edit->cursor - 1);
			int charWidth = Q_UTF8_Width(prev);
			memmove(edit->buffer + offset - charWidth, edit->buffer + offset, len + 1 - offset);
			edit->cursor--;
			if (edit->cursor < edit->scroll)
			{
				edit->scroll--;
			}
		}
		return;
	}

	if (ch == 'a' - 'a' + 1)      // ctrl-a is home
	{
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if (ch == 'e' - 'a' + 1)      // ctrl-e is end
	{
		edit->cursor = stringLen;
		edit->scroll = edit->cursor - edit->widthInChars;
		return;
	}

	// ignore any other non printable chars
	if (ch < 32)
	{
		return;
	}

	// - 2 to leave room for the leading slash and trailing \0
	if (len == MAX_EDIT_LINE - 2)
	{
		return; // all full
	}

	Q_UTF8_Insert(edit->buffer, stringLen, edit->cursor, ch, key_overstrikeMode);
	edit->cursor++;


	if (edit->cursor >= edit->widthInChars)
	{
		edit->scroll++;
	}

	if (edit->cursor == stringLen + 1)
	{
		edit->buffer[Q_UTF8_ByteOffset(edit->buffer, edit->cursor)] = 0;
	}
}

/*
=============================================================================
CONSOLE LINE EDITING
==============================================================================
*/

/**
 * @brief Handles history and console scrollback
 * @param[in] key
 */
void Console_Key(int key)
{
	// ctrl-L clears screen
	if (key == 'l' && (keys[K_LCTRL].down || keys[K_RCTRL].down))
	{
		Cbuf_AddText("clear\n");
		return;
	}

	// enter finishes the line
	if (key == K_ENTER || key == K_KP_ENTER)
	{
		con.highlightOffset = 0;

#if SLASH_COMMAND
		// if not in the game explicitly prepend a slash if needed
		if (cls.state != CA_ACTIVE && g_consoleField.buffer[0] != '\\'
		    && g_consoleField.buffer[0] != '/')
		{
			char temp[MAX_STRING_CHARS];

			Q_strncpyz(temp, g_consoleField.buffer, sizeof(temp));
			Com_sprintf(g_consoleField.buffer, sizeof(g_consoleField.buffer), "\\%s", temp);
			g_consoleField.cursor++;
		}
#endif

		Com_Printf("]%s\n", g_consoleField.buffer);

#if SLASH_COMMAND
		// leading slash is an explicit command
		if (g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/')
		{
			Cbuf_AddText(g_consoleField.buffer + 1);      // valid command
			Cbuf_AddText("\n");
		}
		else
		{
			// other text will be chat messages
			if (!g_consoleField.buffer[0])
			{
				return; // empty lines just scroll the console without adding to history
			}
			else
			{
				char escapedChatMessage[MAX_EDIT_LINE * 2];
				Q_EscapeUnicode(g_consoleField.buffer, escapedChatMessage, MAX_EDIT_LINE * 2);

				Cbuf_AddText("cmd say ");
				Cbuf_AddText(escapedChatMessage);
				Cbuf_AddText("\n");
			}
		}
#else
		Cbuf_AddText(g_consoleField.buffer);      // valid command
		Cbuf_AddText("\n");

		if (!g_consoleField.buffer[0])
		{
			return; // empty lines just scroll the console without adding to history
		}
#endif

		// copy line to history buffer
		historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
		nextHistoryLine++;
		historyLine = nextHistoryLine;

		Field_Clear(&g_consoleField);

		g_consoleField.widthInChars = g_console_field_width;

		if (cls.state == CA_DISCONNECTED)
		{
			SCR_UpdateScreen();     // force an update, because the command
		}                           // may take some time
		return;
	}

	// command completion

	if (key == K_TAB)
	{
		Console_AutoComplete(&g_consoleField, &con.highlightOffset);
		return;
	}

	// clear autocompletion buffer on normal key input
	if ((key >= K_SPACE && key <= K_BACKSPACE) || (key == K_LEFTARROW) || (key == K_RIGHTARROW) ||
	    (key >= K_KP_LEFTARROW && key <= K_KP_RIGHTARROW) ||
	    (key >= K_KP_SLASH && key <= K_KP_PLUS) || (key >= K_KP_STAR && key <= K_KP_EQUALS))
	{
		if (key == K_BACKSPACE && con.highlightOffset)
		{
			Console_RemoveHighlighted(&g_consoleField, &con.highlightOffset);
		}
		else
		{
			con.highlightOffset = 0;
		}
	}

	// command history (ctrl-p ctrl-n for unix style)

	// added some mousewheel functionality to the console
	if ((key == K_MWHEELUP && (keys[K_RSHIFT].down || keys[K_LSHIFT].down)) || (key == K_UPARROW) || (key == K_KP_UPARROW) ||
	    ((tolower(key) == 'p') && (keys[K_LCTRL].down || keys[K_RCTRL].down)))
	{
		if (nextHistoryLine - historyLine < COMMAND_HISTORY
		    && historyLine > 0)
		{
			historyLine--;
		}
		g_consoleField      = historyEditLines[historyLine % COMMAND_HISTORY];
		con.highlightOffset = 0;
		return;
	}

	// added some mousewheel functionality to the console

	if ((key == K_MWHEELDOWN && (keys[K_RSHIFT].down || keys[K_LSHIFT].down)) || (key == K_DOWNARROW) || (key == K_KP_DOWNARROW) ||
	    ((tolower(key) == 'n') && (keys[K_LCTRL].down || keys[K_RCTRL].down)))
	{
		if (historyLine == nextHistoryLine)
		{
			return;
		}
		historyLine++;
		g_consoleField      = historyEditLines[historyLine % COMMAND_HISTORY];
		con.highlightOffset = 0;
		return;
	}

	// console scrolling
	if (key == K_PGUP || key == K_KP_PGUP)
	{
		Con_PageUp();
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN)
	{
		Con_PageDown();
		return;
	}

	if (key == K_MWHEELUP)           // added some mousewheel functionality to the console
	{
		if (keys[K_LCTRL].down || keys[K_RCTRL].down) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollUp(con.visibleLines);
		}
		else
		{
			Con_ScrollUp(con.visibleLines / 4);
		}
		return;
	}

	if (key == K_MWHEELDOWN)         // added some mousewheel functionality to the console
	{
		if (keys[K_LCTRL].down || keys[K_RCTRL].down) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollDown(con.visibleLines);
		}
		else
		{
			Con_ScrollDown(con.visibleLines / 4);
		}
		return;
	}

	// ctrl-home = top of console
	if ((key == K_HOME || key == K_KP_HOME) && (keys[K_LCTRL].down || keys[K_RCTRL].down))
	{
		Con_ScrollTop();
		return;
	}

	// ctrl-end = bottom of console
	if ((key == K_END || key == K_KP_END) && (keys[K_LCTRL].down || keys[K_RCTRL].down))
	{
		Con_ScrollBottom();
		return;
	}

	// pass to the normal editline routine
	Field_KeyDownEvent(&g_consoleField, key);
}

//============================================================================

/**
 * @brief Key_GetOverstrikeMode
 * @return
 */
qboolean Key_GetOverstrikeMode(void)
{
	return key_overstrikeMode;
}

/**
 * @brief Key_SetOverstrikeMode
 * @param[in] state
 */
void Key_SetOverstrikeMode(qboolean state)
{
	key_overstrikeMode = state;
}

/**
 * @brief Key_IsDown
 * @param[in] keynum
 * @return
 */
qboolean Key_IsDown(int keynum)
{
	if (keynum < 0 || keynum >= MAX_KEYS)
	{
		return qfalse;
	}

	return keys[keynum].down;
}

/**
 * @brief Returns a key number to be used to index keys[] by looking at
 * the given string.  Single ascii characters return themselves, while
 * the K_* names are matched up.
 *
 * 0x11 will be interpreted as raw hex, which will allow new controlers
 * to be configured even if they don't have defined names.
 *
 * @param[in] str
 * @return
 */
int Key_StringToKeynum(const char *str)
{
	keyname_t *kn;

	if (!str || !str[0])
	{
		return -1;
	}
	if (!str[1])
	{
		return tolower(str[0]);
	}

	// check for hex code
	if (strlen(str) == 4)
	{
		int n = Com_HexStrToInt(str);

		if (n >= 0)
		{
			return n;
		}
	}

	// scan for a text match
	for (kn = keynames ; kn->name ; kn++)
	{
		if (!Q_stricmp(str, kn->name))
		{
			return kn->keynum;
		}
	}

	return -1;
}

/**
 * @brief Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
 * given keynum.
 *
 * @param[in] keynum
 * @return
 */
char *Key_KeynumToString(int keynum)
{
	keyname_t   *kn;
	static char tinystr[5];
	int         i, j;

	if (keynum == -1)
	{
		return "<KEY NOT FOUND>";
	}

	if (keynum < 0 || keynum >= MAX_KEYS)
	{
		return "<OUT OF RANGE>";
	}

	// check for printable ascii (don't use quote)
	if (keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';')
	{
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	// check for a key string
	for (kn = keynames ; kn->name ; kn++)
	{
		if (keynum == kn->keynum)
		{
			return kn->name;
		}
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}

#define BIND_HASH_SIZE 1024
#define generateHashValue(fname) Q_GenerateHashValue(fname, BIND_HASH_SIZE, qtrue, qfalse)

/**
 * @brief Key_SetBinding
 * @param[in] keynum
 * @param[in] binding
 */
void Key_SetBinding(int keynum, const char *binding)
{
	char *lcbinding;    // make a copy of our binding lowercase
	                    // so name toggle scripts work again: bind x name BzZIfretn?
	                    // resulted into bzzifretn?

	if (keynum == -1)
	{
		return;
	}

	// free old bindings
	if (keys[keynum].binding)
	{
		Z_Free(keys[keynum].binding);
	}

	// allocate memory for new binding
	keys[keynum].binding = CopyString(binding);
	lcbinding            = CopyString(binding);
	Q_strlwr(lcbinding);   // saves doing it on all the generateHashValues in Key_GetBindingByString

	keys[keynum].hash = generateHashValue(lcbinding);

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/**
 * @brief Key_GetBinding
 * @param[in] keynum
 * @return
 */
char *Key_GetBinding(int keynum)
{
	if (keynum == -1)
	{
		return "";
	}

	return keys[keynum].binding;
}

/**
 * @brief Key_GetBindingByString
 * @param[in] binding
 * @param[out] key1
 * @param[out] key2
 *
 * @note Binding MUST be lower case
 */
void Key_GetBindingByString(const char *binding, int *key1, int *key2)
{
	int i;
	int hash = generateHashValue(binding);

	*key1 = -1;
	*key2 = -1;

	for (i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].hash == hash && !Q_stricmp(binding, keys[i].binding))
		{
			if (*key1 == -1)
			{
				*key1 = i;
			}
			else if (*key2 == -1)
			{
				*key2 = i;
				return;
			}
		}
	}
}

/**
 * @brief Key_GetKey
 * @param[in] binding
 * @return
 */
int Key_GetKey(const char *binding)
{
	if (binding)
	{
		int i;

		for (i = 0 ; i < MAX_KEYS ; i++)
		{
			if (keys[i].binding && Q_stricmp(binding, keys[i].binding) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

/**
 * @brief Key_Unbind_f
 */
void Key_Unbind_f(void)
{
	int b;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(b, "");
}

/**
 * @brief Key_Unbindall_f
 */
void Key_Unbindall_f(void)
{
	int i;

	for (i = 0 ; i < MAX_KEYS ; i++)
	{
		if (keys[i].binding)
		{
			Key_SetBinding(i, "");
		}
	}
}

/**
 * @brief Key_Bind_f
 */
void Key_Bind_f(void)
{
	int  i, c, b;
	char cmd[1024];

	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		// FIXME: keyboard translations
		if (keys[b].binding && keys[b].binding[0])
		{
			Com_Printf("\"%s\" = \"%s\"\n", Key_KeynumToString(b), keys[b].binding);
		}
		else
		{
			Com_Printf("\"%s\" is not bound\n", Key_KeynumToString(b));
		}
		return;
	}

	// copy the rest of the command line
	cmd[0] = 0;     // start out with a null string
	for (i = 2 ; i < c ; i++)
	{
		Q_strcat(cmd, sizeof(cmd), Cmd_Argv(i));
		if (i != (c - 1))
		{
			strcat(cmd, " ");
		}
	}

	Key_SetBinding(b, cmd);
}

/**
 * @brief Writes lines containing "bind key value"
 * @param[in] f
 */
void Key_WriteBindings(fileHandle_t f)
{
	int i;

	FS_Printf(f, "unbindall\n");

	for (i = 0 ; i < MAX_KEYS ; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);
		}
	}
}

/**
 * @brief Key_Bindlist_f
 */
void Key_Bindlist_f(void)
{
	int i;
	int freeKeys = 0;

	Com_Printf("key             bind\n");

	Com_Printf("-----------------------------------\n");

	for (i = 0 ; i < MAX_KEYS ; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			if (Cmd_Argc() != 2)
			{
				Com_Printf("%-15s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);
			}
		}
		else
		{
			++freeKeys;

			if (Cmd_Argc() == 2)
			{
				Com_Printf("%-15s *free to bind*\n", Key_KeynumToString(i));
			}
		}
	}

	Com_Printf("-----------------------------------\n");
	Com_Printf("%i free keys available.\n", freeKeys);
	Com_Printf("Enter /bindlist -f to see free keys.\n"); // or any other param ... :)
}

/**
 * @brief Key_KeynameCompletion
 */
void Key_KeynameCompletion(void (*callback)(const char *s))
{
	int i;

	for (i = 0; keynames[i].name != NULL; i++)
	{
		callback(keynames[i].name);
	}
}

/**
 * @brief Key_CompleteUnbind
 * @param[in] args
 * @param[in] argNum
 */
static void Key_CompleteUnbind(char *args, int argNum)
{
	if (argNum == 2)
	{
		// Skip "unbind "
		char *p = Com_SkipTokens(args, 1, " ");

		if (p > args)
		{
			Field_CompleteKeyname();
		}
	}
}

/**
 * @brief Key_CompleteBind
 * @param[in] args
 * @param[in] argNum
 */
static void Key_CompleteBind(char *args, int argNum)
{
	char *p;

	if (argNum == 2)
	{
		// Skip "bind "
		p = Com_SkipTokens(args, 1, " ");

		if (p > args)
		{
			Field_CompleteKeyname();
		}
	}
	else if (argNum >= 3)
	{
		// Skip "bind <key> "
		p = Com_SkipTokens(args, 2, " ");

		if (p > args)
		{
			Field_CompleteCommand(p, qtrue, qtrue);
		}
	}
}

/**
 * @brief CL_InitKeyCommands
 */
void CL_InitKeyCommands(void)
{
	// register our functions
	Cmd_AddCommand("bind", Key_Bind_f, "Binds a key.");
	Cmd_SetCommandCompletionFunc("bind", Key_CompleteBind);
	Cmd_AddCommand("unbind", Key_Unbind_f, "Unbinds a key.");
	Cmd_SetCommandCompletionFunc("unbind", Key_CompleteUnbind);
	Cmd_AddCommand("unbindall", Key_Unbindall_f, "Unbinds all keys.");
	Cmd_AddCommand("bindlist", Key_Bindlist_f, "Prints a list of all key bindings.");
}

qboolean consoleButtonWasPressed = qfalse;

qboolean CL_NumPadEvent(int key)
{
	switch (key)
	{
	case K_KP_INS:        // 0
	case K_KP_END:        // 1
	case K_KP_DOWNARROW:  // 2
	case K_KP_PGDN:       // 3
	case K_KP_LEFTARROW:  // 4
	case K_KP_5:          // 5
	case K_KP_RIGHTARROW: // 6
	case K_KP_HOME:       // 7
	case K_KP_UPARROW:    // 8
	case K_KP_PGUP:       // 9
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Called by the system for both key up and key down events
 * @param[in] key
 * @param[in] down
 * @param[in] time
 */
void CL_KeyEvent(int key, qboolean down, unsigned time)
{
	char     *kb;
	char     cmd[1024];
	qboolean bypassMenu = qfalse;
	qboolean onlybinds  = qfalse;

	if (!key)
	{
		return;
	}

	// special handling for numbers of numeric keypad & NUM pressed
	if (IN_IsNumLockDown() && down)
	{
		onlybinds = CL_NumPadEvent(key);
	}

	// update auto-repeat status and BUTTON_ANY status
	keys[key].down = down;

	if (down)
	{
		keys[key].repeats++;
		if (keys[key].repeats == 1)
		{
			anykeydown++;
		}
	}
	else
	{
		keys[key].repeats = 0;
		anykeydown--;
		if (anykeydown < 0)
		{
			anykeydown = 0;
		}
	}


	// Switch from / to fullscreen on Alt+Enter.
	if (key == K_ENTER)
	{
		if (down)
		{
			if (keys[K_LALT].down || keys[K_RALT].down)
			{
				Key_ClearStates();
				if (Cvar_VariableIntegerValue("r_fullscreen"))
				{
					Com_Printf("Switching to windowed rendering\n");
					Cvar_Set("r_fullscreen", "0");
				}
				else
				{
					Com_Printf("Switching to fullscreen rendering\n");
					Cvar_Set("r_fullscreen", "1");
				}
				// some desktops might freeze without restarting the video subsystem
				if (Q_stristr(cls.glconfig.renderer_string, "mesa") ||
				    Q_stristr(cls.glconfig.renderer_string, "gallium") ||
				    Q_stristr(cls.glconfig.vendor_string, "nouveau") ||
				    Q_stristr(cls.glconfig.vendor_string, "mesa"))
				{
					Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
				}

				return;
			}
		}
	}

#ifdef __linux__
	// Minimize the game on Alt+Tab press.
	if (key == K_TAB)
	{
		if (down)
		{
			if (keys[K_LALT].down || keys[K_RALT].down)
			{
				Key_ClearStates();
				Cbuf_ExecuteText(EXEC_NOW, "minimize");
				return;
			}
		}
	}
#endif

	// console key is hardcoded, so the user can never unbind it
	if (key == CONSOLE_KEY || ((keys[K_RSHIFT].down || keys[K_LSHIFT].down) && key == K_ESCAPE))
	{
		if (!down)
		{
			return;
		}

		Con_ToggleConsole_f();

		if (!keys[K_LCTRL].down && !keys[K_RCTRL].down && !keys[K_LALT].down && !keys[K_RALT].down)
		{
			Key_ClearStates();
		}

		// the console key should never be used as a char
		consoleButtonWasPressed = qtrue;
		return;
	}
	else
	{
		consoleButtonWasPressed = qfalse;
	}

	// most keys during demo playback will bring up the menu, but non-ascii
	// keys can still be used for bound actions
	if (down && (key < 128 || key == K_MOUSE1)
	    && (clc.demoplaying || cls.state == CA_CINEMATIC) && !cls.keyCatchers)
	{

		Cvar_Set("nextdemo", "");
		key = K_ESCAPE;
	}

	// escape is always handled special
	if (key == K_ESCAPE && down)
	{
		if (cls.keyCatchers & KEYCATCH_CONSOLE)
		{
			// First time you press escape it will clear the written command (reset the line)
			// if the commandline is empty then we close the console
			if (strlen(g_consoleField.buffer))
			{
				con.highlightOffset   = 0;
				g_consoleField.cursor = 0;
				Com_Memset(g_consoleField.buffer, 0, sizeof(char) * MAX_EDIT_LINE);
			}
			else
			{
				Con_ToggleConsole_f();

				if (!keys[K_LCTRL].down && !keys[K_RCTRL].down && !keys[K_LALT].down && !keys[K_RALT].down)
				{
					Key_ClearStates();
				}
			}

			return;
		}

		// escape always gets out of CGAME stuff
		if (cls.keyCatchers & KEYCATCH_CGAME)
		{
			cls.keyCatchers &= ~KEYCATCH_CGAME;
			VM_Call(cgvm, CG_EVENT_HANDLING, CGAME_EVENT_NONE);

			if (clc.demoplaying)
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			}

			return;
		}

		if (!(cls.keyCatchers & KEYCATCH_UI))
		{
			if (cls.state == CA_ACTIVE && !clc.demoplaying)
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME);
			}
			else
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			}
			return;
		}

		VM_Call(uivm, UI_KEY_EVENT, key, down);
		return;
	}

	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.

	if (!down)
	{
		kb = keys[key].binding;
		if (kb && kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			Com_sprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb + 1, key, time);
			Cbuf_AddText(cmd);
		}

		if ((cls.keyCatchers & KEYCATCH_UI) && uivm)
		{
			if (!onlybinds || VM_Call(uivm, UI_WANTSBINDKEYS))
			{
				VM_Call(uivm, UI_KEY_EVENT, key, down);
			}
		}
		else if ((cls.keyCatchers & KEYCATCH_CGAME) && cgvm)
		{
			if (!onlybinds || VM_Call(cgvm, CG_WANTSBINDKEYS))
			{
				VM_Call(cgvm, CG_KEY_EVENT, key, down);
			}
		}

		return;
	}

	// if we just want to pass it along to game
	if (cl_bypassMouseInput && cl_bypassMouseInput->integer)
	{
		if ((key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 || key == K_MOUSE4 || key == K_MOUSE5))
		{
			if (cl_bypassMouseInput->integer == 1)
			{
				bypassMenu = qtrue;
			}
		}
		else if (((cls.keyCatchers & KEYCATCH_UI) && !UI_checkKeyExec(key)) || ((cls.keyCatchers & KEYCATCH_CGAME) && !CL_CGameCheckKeyExec(key)))
		{
			bypassMenu = qtrue;
		}
	}

	// distribute the key down event to the apropriate handler
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		if (!onlybinds)
		{
			Console_Key(key);
		}
	}
	else if ((cls.keyCatchers & KEYCATCH_UI) && !bypassMenu)
	{
		if (!onlybinds || VM_Call(uivm, UI_WANTSBINDKEYS))
		{
			VM_Call(uivm, UI_KEY_EVENT, key, down);
		}
	}
	else if ((cls.keyCatchers & KEYCATCH_CGAME) && !bypassMenu)
	{
		if (cgvm)
		{
			if (!onlybinds || VM_Call(cgvm, CG_WANTSBINDKEYS))
			{
				VM_Call(cgvm, CG_KEY_EVENT, key, down);
			}
		}
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		if (!onlybinds)
		{
			Console_Key(key);
		}
	}
	else
	{
		// send the bound action
		kb = keys[key].binding;
		if (!kb)
		{
			if (key >= 200)
			{
				Com_Printf("%s (key %i) is unbound, use controls menu to set.\n"
				           , Key_KeynumToString(key), key);
			}
		}
		else if (kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			Com_sprintf(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
			Cbuf_AddText(cmd);
		}
		else
		{
			// down-only command
			Cbuf_AddText(kb);
			Cbuf_AddText("\n");
		}
	}
}

/**
 * @brief Normal keyboard characters, already shifted / capslocked / etc
 * @param[in] key
 */
void CL_CharEvent(int key)
{
	// the console key should never be used as a char
	// - added uk equivalent of shift+`
	// the RIGHT way to do this would be to have certain keys disable the equivalent SE_CHAR event

	// this should be fixed in Com_EventLoop but I can't be arsed to leave this as is

	if (key == (unsigned char) '`' || key == (unsigned char) '~' || key == (unsigned char) '\xAC')
	{
		return;
	}

	// distribute the key down event to the apropriate handler
	if (cls.keyCatchers & KEYCATCH_CONSOLE)
	{
		Field_CharEvent(&g_consoleField, key);
	}
	else if (cls.keyCatchers & KEYCATCH_UI)
	{
		VM_Call(uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, qtrue);
	}
	else if (cls.keyCatchers & KEYCATCH_CGAME)
	{
		VM_Call(cgvm, CG_KEY_EVENT, key | K_CHAR_FLAG, qtrue);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Field_CharEvent(&g_consoleField, key);
	}
}

/**
 * @brief Key_ClearStates
 */
void Key_ClearStates(void)
{
	int i;

	anykeydown = 0;

	for (i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].down)
		{
			CL_KeyEvent(i, qfalse, 0);
		}
		keys[i].down    = 0;
		keys[i].repeats = 0;
	}
}
