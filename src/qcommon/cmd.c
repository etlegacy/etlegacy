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
 * @file cmd.c
 * @brief Quake script command processing module
 */

#include "q_shared.h"
#include "qcommon.h"
#include "q_unicode.h"

#ifndef DEDICATED
#include "../client/client.h"
#endif

#define MAX_CMD_BUFFER  131072
#define MAX_CMD_LINE    1024

typedef struct
{
	byte *data;
	unsigned int maxsize;
	unsigned int cursize;
} cmd_t;

int   cmd_wait;
cmd_t cmd_text;
byte  cmd_text_buf[MAX_CMD_BUFFER];

//=============================================================================

/**
 * @brief Causes execution of the remainder of the command buffer to be delayed until
 * next frame.
 *
 * This allows commands like:
 * bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
 */
void Cmd_Wait_f(void)
{
	if (Cmd_Argc() == 2)
	{
		cmd_wait = Q_atoi(Cmd_Argv(1));
		if (cmd_wait < 0)
		{
			cmd_wait = 1; // ignore the argument
		}
	}
	else
	{
		cmd_wait = 1; // ignore the argument
	}
}

/**
=============================================================================
                        COMMAND BUFFER
=============================================================================
*/

/**
 * @brief Cbuf_Init
 */
void Cbuf_Init(void)
{
	cmd_text.data    = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/**
 * @brief Adds command text at the end of the buffer, does NOT add a final \\n
 * @param text
 */
void Cbuf_AddText(const char *text)
{
	size_t l;

	l = strlen(text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;
}

/**
 * @brief Adds command text immediately after the current command
 * Adds a \\n to the text
 *
 * @param[in] text
 */
void Cbuf_InsertText(const char *text)
{
	size_t       len;
	unsigned int i;

	len = strlen(text) + 1;
	if (len + cmd_text.cursize > cmd_text.maxsize)
	{
		Com_Printf("Cbuf_InsertText overflowed\n");
		return;
	}

	// move the existing command text
	for (i = cmd_text.cursize - 1 ; i != UINT_MAX; i--)
	{
		cmd_text.data[i + len] = cmd_text.data[i];
	}

	// copy the new text in
	Com_Memcpy(cmd_text.data, text, len - 1);

	// add a \n
	cmd_text.data[len - 1] = '\n';

	cmd_text.cursize += len;
}

/**
 * @brief Cbuf_ExecuteText
 * @param[in] exec_when
 * @param[in] text
 */
void Cbuf_ExecuteText(int exec_when, const char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		if (text && strlen(text) > 0)
		{
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", text);
			Cmd_ExecuteString(text);
		}
		else
		{
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", cmd_text.data);
			Cbuf_Execute();
		}
		break;
	case EXEC_INSERT:
		Cbuf_InsertText(text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText(text);
		break;
	default:
		Com_Error(ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/**
 * @brief Cbuf_Execute
 */
void Cbuf_Execute(void)
{
	unsigned int i;
	char         *text;
	char         line[MAX_CMD_LINE];
	int          quotes;
	// This will keep // style comments all on one line by not breaking on
	// a semicolon.  It will keep /* ... */ style comments all on one line by not
	// breaking it for semicolon or newline.
	qboolean in_star_comment  = qfalse;
	qboolean in_slash_comment = qfalse;

	while (cmd_text.cursize)
	{
		if (cmd_wait > 0)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break or comment: // or /* */
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i = 0 ; i < cmd_text.cursize ; i++)
		{
			// FIXME: ignore quoted text

			if (text[i] == '"')
			{
				quotes++;
			}

			if (!(quotes & 1))
			{
				if (i < cmd_text.cursize - 1)
				{
					if (!in_star_comment && text[i] == '/' && text[i + 1] == '/')
					{
						in_slash_comment = qtrue;
					}
					else if (!in_slash_comment && text[i] == '/' && text[i + 1] == '*')
					{
						in_star_comment = qtrue;
					}
					else if (in_star_comment && text[i] == '*' && text[i + 1] == '/')
					{
						in_star_comment = qfalse;
						// If we are in a star comment, then the part after it is valid
						// Note: This will cause it to NUL out the terminating '/'
						// but ExecuteString doesn't require it anyway.
						i++;
						break;
					}
				}
				if (!in_slash_comment && !in_star_comment && text[i] == ';')
				{
					break;
				}
			}
			if (!in_star_comment && (text[i] == '\n' || text[i] == '\r'))
			{
				in_slash_comment = qfalse;
				break;
			}
		}

		if (i >= (MAX_CMD_LINE - 1))
		{
			i = MAX_CMD_LINE - 1;
		}

		Com_Memcpy(line, text, i);
		line[i] = 0;

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec) can insert data at the
		// beginning of the text buffer

		if (i == cmd_text.cursize)
		{
			cmd_text.cursize = 0;
		}
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove(text, text + i, cmd_text.cursize);
		}

		// execute the command line
		Cmd_ExecuteString(line);
	}
}

/**
==============================================================================
                        SCRIPT COMMANDS
==============================================================================
*/

/**
 * @brief Executes a script file
 */
void Cmd_Exec_f(void)
{
	char filename[MAX_QPATH];
	union
	{
		char *c;
		void *v;
	} f;
	qboolean quiet;

	quiet = !Q_stricmp(Cmd_Argv(0), "execq");

	if (Cmd_Argc() != 2)
	{
		Com_Printf("exec%s <filename> : execute a script file%s\n",
		           quiet ? "q" : "", quiet ? " without notification" : "");
		return;
	}

	Q_strncpyz(filename, Cmd_Argv(1), sizeof(filename));
	COM_DefaultExtension(filename, sizeof(filename), ".cfg");
	(void) FS_ReadFile(filename, &f.v);
	if (!f.c)
	{
		Com_Printf("couldn't exec %s\n", filename);
		return;
	}
	if (!quiet)
	{
		Com_Printf("execing %s\n", filename);
	}

	Cbuf_InsertText(f.c);

	FS_FreeFile(f.v);
}

/**
 * @brief Inserts the current value of a variable as command text
 */
void Cmd_Vstr_f(void)
{
	char *v;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("vstr <variablename> : execute a variable command\n");
		return;
	}

	v = Cvar_VariableString(Cmd_Argv(1));
	Cbuf_InsertText(va("%s\n", v));
}

/**
 * @brief Prints quoted text to the console
 * and shows a notification if connected to a server.
 *
 * Example: echo "Hello " vstr name "!"
 */
void Cmd_Echo_f(void)
{
	int      i                  = 1;
	char     text[MAX_CMD_LINE] = "";
	qboolean vstr               = qfalse;

	// Print cvar value following vstr
	while (i < Cmd_Argc())
	{
		if (!Q_stricmp(Cmd_Argv(i), "vstr"))
		{
			Q_strcat(text, sizeof(text), Cvar_VariableString(Cmd_Argv(i + 1)));
			vstr = qtrue;
			i++;
		}
		else
		{
			Q_strcat(text, sizeof(text), Cmd_Argv(i));
		}
		i++;
	}

#ifndef DEDICATED
	// "cpm" is a cgame command, so just print the text if disconnected
	if (cls.state != CA_CONNECTED && cls.state != CA_ACTIVE)
#endif
	{
		Com_Printf("%s\n", vstr ? text : Cmd_Args());
		return;
	}

	Cbuf_AddText(va("cpm \"%s\"\n", vstr ? text : Cmd_Args()));
}

/**
=============================================================================
                    COMMAND EXECUTION
=============================================================================
*/

/**
 * @struct cmd_function_s
 */
typedef struct cmd_function_s
{
	struct cmd_function_s *next;
	char *name;
	char *description;
	xcommand_t function;
	completionFunc_t complete;
} cmd_function_t;

static int  cmd_argc;
static char *cmd_argv[MAX_STRING_TOKENS];                               ///< points into cmd_tokenized
static char cmd_tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS];         ///< will have 0 bytes inserted
static char cmd_cmd[BIG_INFO_STRING];                                   ///< the original command we received (no token processing)

static cmd_function_t *cmd_functions;                                   ///< possible commands to execute

/**
 * @brief Cmd_Argc
 * @return
 */
int Cmd_Argc(void)
{
	return cmd_argc;
}

/**
 * @brief Cmd_Argv
 * @param arg
 * @return
 */
char *Cmd_Argv(int arg)
{
	if (arg >= cmd_argc)
	{
		return "";
	}
	return cmd_argv[arg];
}

/**
 * @brief The interpreted versions use this because they can't have pointers returned to them
 * @param arg
 * @param buffer
 * @param bufferLength
 */
void Cmd_ArgvBuffer(int arg, char *buffer, size_t bufferLength)
{
	Q_strncpyz(buffer, Cmd_Argv(arg), bufferLength);
}

/**
 * @brief Cmd_Args
 * @return A single string containing argv(1) to argv(argc()-1)
 */
char *Cmd_Args(void)
{
	static char cmd_args[MAX_STRING_CHARS];
	int         i;

	cmd_args[0] = 0;
	for (i = 1 ; i < cmd_argc ; i++)
	{
		Q_strcat(cmd_args, MAX_STRING_CHARS, cmd_argv[i]);
		if (i != cmd_argc - 1)
		{
			strcat(cmd_args, " ");
		}
	}

	return cmd_args;
}

/**
 * @brief Cmd_ArgsFrom
 * @param arg
 * @return A single string containing argv(arg) to argv(argc()-1)
 */
char *Cmd_ArgsFrom(int arg)
{
	static char cmd_args[BIG_INFO_STRING];
	int         i;

	cmd_args[0] = 0;
	if (arg < 0)
	{
		arg = 0;
	}
	for (i = arg ; i < cmd_argc ; i++)
	{
		Q_strcat(cmd_args, BIG_INFO_STRING, cmd_argv[i]);
		if (i != cmd_argc - 1)
		{
			strcat(cmd_args, " ");
		}
	}

	return cmd_args;
}

/**
 * @brief Cmd_ArgsFromTo
 * @param arg
 * @param max
 * @return A single string containing argv(arg) to argv(max-1)
 */
char *Cmd_ArgsFromTo(int arg, int max)
{
	static char cmd_args[BIG_INFO_STRING];
	int         i;

	cmd_args[0] = 0;
	if (arg < 0)
	{
		arg = 0;
	}

	//FIXME what should these be
	if (max > cmd_argc)
	{
		max = cmd_argc;
	}
	if (max < arg)
	{
		max = cmd_argc;
	}

	for (i = arg ; i < max ; i++)
	{
		Q_strcat(cmd_args, BIG_INFO_STRING, cmd_argv[i]);
		if (i != max - 1)
		{
			strcat(cmd_args, " ");
		}
	}

	return cmd_args;
}

/**
 * @brief The interpreted versions use this because they can't have pointers returned to them
 * @param[out] buffer
 * @param[in] bufferLength
 */
void Cmd_ArgsBuffer(char *buffer, size_t bufferLength)
{
	Q_strncpyz(buffer, Cmd_Args(), bufferLength);
}

/**
 * @brief Retrieve the unmodified command string.
 * For rcon use when you want to transmit without altering quoting
 * @return
 */
char *Cmd_Cmd(void)
{
	return cmd_cmd;
}

/**
 * @brief Replaces command separators with space to prevent interpretation
 *
 * This prevents the infamous callvote hack.
 */
void Cmd_Args_Sanitize(void)
{
	int i;

	for (i = 1; i < cmd_argc; i++)
	{
		char *c = cmd_argv[i];

		if (strlen(c) > MAX_CVAR_VALUE_STRING - 1)
		{
			c[MAX_CVAR_VALUE_STRING - 1] = '\0';
		}

		while ((c = strpbrk(c, "\n\r;")))
		{
			*c = ' ';
			++c;
		}
	}
}

/**
 * @brief Parses the given string into command line tokens.
 *
 * @details The text is copied to a separate buffer and 0 characters
 * are inserted in the appropriate place, The argv array
 * will point into this temporary buffer.
 *
 * @param[in] text_in
 * @param[in] ignoreQuotes
 */
static void Cmd_TokenizeString2(const char *text_in, qboolean ignoreQuotes)
{
	const char *text;
	char       *textOut;

	// clear previous args
	cmd_argc = 0;

	if (!text_in)
	{
		return;
	}

	Q_strncpyz(cmd_cmd, text_in, sizeof(cmd_cmd));

	text    = text_in;
	textOut = cmd_tokenized;

	while (1)
	{
		if (cmd_argc == MAX_STRING_TOKENS)
		{
			return;         // this is usually something malicious
		}

		while (1)
		{
			// skip whitespace
			while (*text && *text <= ' ')
			{
				text++;
			}
			if (!*text)
			{
				return;         // all tokens parsed
			}

			// skip // comments
			if (text[0] == '/' && text[1] == '/')
			{
				// lets us put 'http://' in commandlines
				if (text == text_in || (text > text_in && text[-1] != ':'))
				{
					return;         // all tokens parsed
				}
			}

			// skip /* */ comments
			if (text[0] == '/' && text[1] == '*')
			{
				while (*text && (text[0] != '*' || text[1] != '/'))
				{
					text++;
				}
				if (!*text)
				{
					return;     // all tokens parsed
				}
				text += 2;
			}
			else
			{
				break;          // we are ready to parse a token
			}
		}

		// handle quoted strings
		// NOTE: this doesn't handle \" escaping
		if (!ignoreQuotes && *text == '"')
		{
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while (*text && *text != '"')
			{
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if (!*text)
			{
				return;     // all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		// skip until whitespace, quote, or command
		while (qtrue)
		{
			uint32_t point1 = Q_UTF8_CodePoint(text);
			int width = Q_UTF8_Width(text);

			if(point1 <= ' ')
			{
				break;
			}

			if (!ignoreQuotes && point1 == '"')
			{
				break;
			}

			if (point1 == '/' && text[1] == '/')
			{
				// lets us put 'http://' in commandlines
				if (text == text_in || (text > text_in && text[-1] != ':'))
				{
					break;
				}
			}

			// skip /* */ comments
			if (point1 == '/' && text[1] == '*')
			{
				break;
			}

			while (width)
			{
				*textOut++ = *text++;
				width--;
			}
		}

		*textOut++ = 0;

		if (!*text)
		{
			return;     // all tokens parsed
		}
	}
}

/**
 * @brief Cmd_TokenizeString
 * @param[in] text
 */
void Cmd_TokenizeString(const char *text)
{
	Cmd_TokenizeString2(text, qfalse);
}

/**
 * @brief Cmd_TokenizeStringIgnoreQuotes
 * @param[in] text_in
 */
void Cmd_TokenizeStringIgnoreQuotes(const char *text_in)
{
	Cmd_TokenizeString2(text_in, qtrue);
}

/**
 * @brief Cmd_FindCommand
 * @param[in] cmd_name
 * @return
 */
cmd_function_t *Cmd_FindCommand(const char *cmd_name)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(cmd_name, cmd->name))
		{
			return cmd;
		}
	}
	return NULL;
}

/**
 * @brief Cmd_AddSystemCommand
 * @param[in] cmd_name
 * @param[in] function
 * @param[in] description
 * @param[in] complete
 */
void Cmd_AddSystemCommand(const char *cmd_name, xcommand_t function, const char *description, completionFunc_t complete)
{
	cmd_function_t *cmd;

	if (!cmd_name || !cmd_name[0])
	{
		Com_Printf(S_COLOR_RED "Cmd_AddSystemCommand can't add NULL or empty command name\n");
		return;
	}

	// fail if the command already exists
	if (Cmd_FindCommand(cmd_name))
	{
		// allow completion-only commands to be silently doubled
		if (function != NULL)
		{
			Com_Printf("Cmd_AddCommandExtended: %s already defined\n", cmd_name);
		}
		return;
	}

	// use a small Com_Allocate to avoid zone fragmentation
	cmd           = S_Malloc(sizeof(cmd_function_t));
	cmd->name     = CopyString(cmd_name);
	cmd->function = function;
	cmd->complete = complete;
	cmd->next     = cmd_functions;
	cmd_functions = cmd;

	if (description && description[0])
	{
		cmd->description = CopyString(description);
	}
	else
	{
		cmd->description = NULL;
	}
}

/**
 * @brief Cmd_SetCommandCompletionFunc
 * @param[in] command
 * @param[in] complete
 */
void Cmd_SetCommandCompletionFunc(const char *command, completionFunc_t complete)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(command, cmd->name))
		{
			cmd->complete = complete;
			return;
		}
	}
}

/**
 * @brief Cmd_SetCommandDescription
 * @param[in] command
 * @param[in] description
 *
 * @note Unused
 */
void Cmd_SetCommandDescription(const char *command, const char *description)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(command, cmd->name))
		{
			cmd->description = CopyString(description);
		}
	}
}

/**
 * @brief Cmd_RemoveCommand
 * @param[in] cmd_name
 */
void Cmd_RemoveCommand(const char *cmd_name)
{
	cmd_function_t *cmd, **back = &cmd_functions;

	if (!cmd_name || !cmd_name[0])
	{
		Com_Printf(S_COLOR_RED "Cmd_RemoveCommand called with an empty command name\n");
		return;
	}

	while (1)
	{
		cmd = *back;
		if (!cmd)
		{
			// command wasn't active
			return;
		}
		if (!strcmp(cmd_name, cmd->name))
		{
			*back = cmd->next;

			Z_Free(cmd->name);

			if (cmd->description)
			{
				Z_Free(cmd->description);
			}
			Z_Free(cmd);
			return;
		}
		back = &cmd->next;
	}
}

/**
 * @brief Only remove commands with no associated function
 * Already removed in ETL: +button4, -button4
 * Allowed to remove:      +lookup, +lookdown, -lookup, +lookup, configstrings
 * @param[in] cmd_name
 */
void Cmd_RemoveCommandSafe(const char *cmd_name)
{
	cmd_function_t *cmd;

	if (!cmd_name || !cmd_name[0])
	{
		Com_Printf(S_COLOR_RED "Cmd_RemoveCommandSafe called with an empty command name\n");
		return;
	}

	// silent return for obsolete genuine ET cmds which have been removed in ETL
	if (!strcmp(cmd_name, "+button4") || !strcmp(cmd_name, "-button4"))
	{
		return;
	}

	cmd = Cmd_FindCommand(cmd_name);

	if (!cmd)
	{
		// don't nag for commands which might have been removed
		if (!(!strcmp(cmd_name, "+lookup") || !strcmp(cmd_name, "+lookdown")
		      || !strcmp(cmd_name, "-lookup") || !strcmp(cmd_name, "-lookdown")
		      || !strcmp(cmd_name, "configstrings")))
		{
			Com_Printf(S_COLOR_RED "Cmd_RemoveCommandSafe called for an unknown command \"%s\"\n", cmd_name);
		}
		return;
	}

	// don't remove commands in general ...

	// this ensures commands like vid_restart, quit etc won't be removed from the engine by mod code
	if (cmd->function &&
	    // several mods are removing some system commands to avoid abuse - let's allow these
	    !(!strcmp(cmd_name, "+lookup") || !strcmp(cmd_name, "+lookdown")
	      || !strcmp(cmd_name, "-lookup") || !strcmp(cmd_name, "-lookdown")
	      || !strcmp(cmd_name, "configstrings")))
	{
		Com_Printf(S_COLOR_RED "Restricted source tried to remove system command \"%s\"\n", cmd_name);
		return;
	}

	Com_DPrintf(S_COLOR_YELLOW "Cmd_RemoveCommandSafe command \"%s\" removed\n", cmd_name);

	Cmd_RemoveCommand(cmd_name);
}

/**
 * @brief Cmd_CommandCompletion
 */
void Cmd_CommandCompletion(void (*callback)(const char *s))
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions ; cmd ; cmd = cmd->next)
	{
		callback(cmd->name);
	}
}

/**
 * @brief Cmd_CompleteArgument
 * @param[in] command
 * @param[in] args
 * @param[in] argNum
 */
void Cmd_CompleteArgument(const char *command, char *args, int argNum)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(command, cmd->name))
		{
			if (cmd->complete)
			{
				cmd->complete(args, argNum);
			}
			return;
		}
	}
}

/**
 * @brief A complete command line has been parsed, so try to execute it
 * @param text
 */
void Cmd_ExecuteString(const char *text)
{
	cmd_function_t *cmd, **prev;

	// execute the command line
	Cmd_TokenizeString(text);
	if (!Cmd_Argc())
	{
		return;     // no tokens
	}

	// check registered command functions
	for (prev = &cmd_functions ; *prev ; prev = &cmd->next)
	{
		cmd = *prev;
		if (!Q_stricmp(cmd_argv[0], cmd->name))
		{
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev         = cmd->next;
			cmd->next     = cmd_functions;
			cmd_functions = cmd;

			// perform the action
			if (!cmd->function)
			{
				// let the cgame or game handle it
				break;
			}
			else
			{
				cmd->function();
			}
			return;
		}
	}

	// check cvars
	if (Cvar_Command())
	{
		return;
	}

	// check client game commands
	if (com_cl_running && com_cl_running->integer && CL_GameCommand())
	{
		return;
	}

	// check server game commands
	if (com_sv_running && com_sv_running->integer && SV_GameCommand())
	{
		return;
	}

	// check ui commands
	if (com_cl_running && com_cl_running->integer && UI_GameCommand())
	{
		return;
	}

	// send it as a server command if we are connected
	CL_ForwardCommandToServer(text);
}

/**
 * @brief List available commands
 */
void Cmd_List_f(void)
{
	cmd_function_t *cmd;
	int            i = 0;
	char           *match;

	if (Cmd_Argc() > 1)
	{
		match = Cmd_Argv(1);
	}
	else
	{
		match = NULL;
	}

	for (cmd = cmd_functions ; cmd ; cmd = cmd->next)
	{
		if (match && !Com_Filter(match, cmd->name, qfalse))
		{
			continue;
		}

		if (cmd->description)
		{
			Com_Printf("%-18s : %s\n", cmd->name, cmd->description);
		}
		else
		{
			Com_Printf("%-18s\n", cmd->name);
		}
		i++;
	}
	Com_Printf("%i commands\n", i);
}

/**
 * @brief Cmd_CompleteCfgName
 * @param args - unused
 * @param[in] argNum
 */
void Cmd_CompleteCfgName(char *args, int argNum)
{
	if (argNum == 2)
	{
		Field_CompleteFilename("", "cfg", qfalse, qtrue);
	}
}

/**
 * @brief Cmd_Init
 */
void Cmd_Init(void)
{
	// 'cmdlist' should have alias commands like 'help' or '?' but these are already used in mods :/
	Cmd_AddCommand("cmdlist", Cmd_List_f, "Prints a list of all available commands.");
	Cmd_AddCommand("exec", Cmd_Exec_f , "Executes a script file.", Cmd_CompleteCfgName);
	Cmd_AddCommand("execq", Cmd_Exec_f, "Executes a script file quietly.", Cmd_CompleteCfgName);
	Cmd_AddCommand("vstr", Cmd_Vstr_f, "Inserts the current value of a variable as command text.", Cvar_CompleteCvarName);
	Cmd_AddCommand("echo", Cmd_Echo_f, "Prints quoted text to the console and shows a notification if connected to a server.");
	Cmd_AddCommand("wait", Cmd_Wait_f, "Causes execution of the remainder of the command buffer to be delayed until next frame.");
}
