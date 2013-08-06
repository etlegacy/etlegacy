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
 * @file cmd.c
 * @brief Quake script command processing module
 */

#include "q_shared.h"
#include "qcommon.h"

#ifndef DEDICATED
#include "../client/client.h"
#endif

#define MAX_CMD_BUFFER  131072
#define MAX_CMD_LINE    1024

typedef struct
{
	byte *data;
	int maxsize;
	int cursize;
} cmd_t;

int   cmd_wait;
cmd_t cmd_text;
byte  cmd_text_buf[MAX_CMD_BUFFER];

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
void Cmd_Wait_f(void)
{
	if (Cmd_Argc() == 2)
	{
		cmd_wait = atoi(Cmd_Argv(1));
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

/*
=============================================================================
                        COMMAND BUFFER
=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init(void)
{
	cmd_text.data    = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void Cbuf_AddText(const char *text)
{
	int l;

	l = strlen(text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;
}

/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText(const char *text)
{
	int len;
	int i;

	len = strlen(text) + 1;
	if (len + cmd_text.cursize > cmd_text.maxsize)
	{
		Com_Printf("Cbuf_InsertText overflowed\n");
		return;
	}

	// move the existing command text
	for (i = cmd_text.cursize - 1 ; i >= 0 ; i--)
	{
		cmd_text.data[i + len] = cmd_text.data[i];
	}

	// copy the new text in
	Com_Memcpy(cmd_text.data, text, len - 1);

	// add a \n
	cmd_text.data[len - 1] = '\n';

	cmd_text.cursize += len;
}

/*
============
Cbuf_ExecuteText
============
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

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute(void)
{
	int  i;
	char *text;
	char line[MAX_CMD_LINE];
	int  quotes;
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

/*
==============================================================================
                        SCRIPT COMMANDS
==============================================================================
*/

/*
===============
Cmd_Exec_f
===============
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
	FS_ReadFile(filename, &f.v);
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

/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
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
 * @brief Prints the rest of the line to the console
 * and shows a notification if connected to a server.
 */
void Cmd_Echo_f(void)
{
#ifndef DEDICATED
	// "cpm" is a cgame command, so just print the text if disconnected
	if (cls.state != CA_CONNECTED && cls.state != CA_ACTIVE)
	{
		Com_Printf("%s\n", Cmd_Args());
		return;
	}
#endif

	Cbuf_AddText(va("cpm \"%s\"\n", Cmd_Args()));
}

/*
=============================================================================
                    COMMAND EXECUTION
=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s *next;
	char *name;
	xcommand_t function;
	completionFunc_t complete;
} cmd_function_t;

static int  cmd_argc;
static char *cmd_argv[MAX_STRING_TOKENS];               // points into cmd_tokenized
static char cmd_tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS];         // will have 0 bytes inserted
static char cmd_cmd[BIG_INFO_STRING];         // the original command we received (no token processing)

static cmd_function_t *cmd_functions;       // possible commands to execute

/*
============
Cmd_Argc
============
*/
int Cmd_Argc(void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char *Cmd_Argv(int arg)
{
	if ((unsigned)arg >= cmd_argc)
	{
		return "";
	}
	return cmd_argv[arg];
}

/*
============
Cmd_ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void Cmd_ArgvBuffer(int arg, char *buffer, int bufferLength)
{
	Q_strncpyz(buffer, Cmd_Argv(arg), bufferLength);
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char *Cmd_Args(void)
{
	static char cmd_args[MAX_STRING_CHARS];
	int         i;

	cmd_args[0] = 0;
	for (i = 1 ; i < cmd_argc ; i++)
	{
		strcat(cmd_args, cmd_argv[i]);
		if (i != cmd_argc - 1)
		{
			strcat(cmd_args, " ");
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
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
		strcat(cmd_args, cmd_argv[i]);
		if (i != cmd_argc - 1)
		{
			strcat(cmd_args, " ");
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(max-1)
============
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
		strcat(cmd_args, cmd_argv[i]);
		if (i != max - 1)
		{
			strcat(cmd_args, " ");
		}
	}

	return cmd_args;
}

/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void Cmd_ArgsBuffer(char *buffer, int bufferLength)
{
	Q_strncpyz(buffer, Cmd_Args(), bufferLength);
}

/*
============
Cmd_Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
============
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

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
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
		while (*text > ' ')
		{
			if (!ignoreQuotes && text[0] == '"')
			{
				break;
			}

			if (text[0] == '/' && text[1] == '/')
			{
				// lets us put 'http://' in commandlines
				if (text == text_in || (text > text_in && text[-1] != ':'))
				{
					break;
				}
			}

			// skip /* */ comments
			if (text[0] == '/' && text[1] == '*')
			{
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if (!*text)
		{
			return;     // all tokens parsed
		}
	}
}

void Cmd_TokenizeString(const char *text_in)
{
	Cmd_TokenizeString2(text_in, qfalse);
}

void Cmd_TokenizeStringIgnoreQuotes(const char *text_in)
{
	Cmd_TokenizeString2(text_in, qtrue);
}

/*
============
Cmd_FindCommand
============
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

/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand(const char *cmd_name, xcommand_t function)
{
	cmd_function_t *cmd;

	// fail if the command already exists
	if (Cmd_FindCommand(cmd_name))
	{
		// allow completion-only commands to be silently doubled
		if (function != NULL)
		{
			Com_Printf("Cmd_AddCommand: %s already defined\n", cmd_name);
		}
		return;
	}

	// use a small malloc to avoid zone fragmentation
	cmd           = S_Malloc(sizeof(cmd_function_t));
	cmd->name     = CopyString(cmd_name);
	cmd->function = function;
	cmd->complete = NULL;
	cmd->next     = cmd_functions;
	cmd_functions = cmd;
}

/*
============
Cmd_SetCommandCompletionFunc
============
*/
void Cmd_SetCommandCompletionFunc(const char *command, completionFunc_t complete)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(command, cmd->name))
		{
			cmd->complete = complete;
		}
	}
}

/*
============
Cmd_RemoveCommand
============
*/
void Cmd_RemoveCommand(const char *cmd_name)
{
	cmd_function_t *cmd, **back = &cmd_functions;

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
			if (cmd->name)
			{
				Z_Free(cmd->name);
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
 */
void Cmd_RemoveCommandSafe(const char *cmd_name)
{
	cmd_function_t *cmd;

	if (!cmd_name[0])
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

/*
============
Cmd_CommandCompletion
============
*/
void Cmd_CommandCompletion(void (*callback)(const char *s))
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions ; cmd ; cmd = cmd->next)
	{
		callback(cmd->name);
	}
}

/*
============
Cmd_CompleteArgument
============
*/
void Cmd_CompleteArgument(const char *command, char *args, int argNum)
{
	cmd_function_t *cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp(command, cmd->name) && cmd->complete)
		{
			cmd->complete(args, argNum);
		}
	}
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
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

/*
============
Cmd_List_f
============
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

		Com_Printf("%s\n", cmd->name);
		i++;
	}
	Com_Printf("%i commands\n", i);
}

/*
==================
Cmd_CompleteCfgName
==================
*/
void Cmd_CompleteCfgName(char *args, int argNum)
{
	if (argNum == 2)
	{
		Field_CompleteFilename("", "cfg", qfalse, qtrue);
	}
}

/**
 * @brief Recursively removes files matching a given pattern from homepath.
 * Useful for removing incomplete downloads and other garbage.
 * Files listed in the com_cleanWhitelist cvar are protected from deletion.
 */
void Cmd_CleanHomepath_f(void)
{
	int      i, j, numFiles = 0, delFiles = 0, totalNumFiles = 0;
	char     **pFiles = NULL, *tokens;
	char     path[MAX_OSPATH], whitelist[MAX_OSPATH];
	qboolean whitelisted;

	if (Cmd_Argc() < 3)
	{
		// basically files are downloaded again when required - but better print a warning for inexperienced users
		Com_Printf("usage: clean <mod> <pattern[1]> <pattern[n]>\nexample: clean all *tmp */zzz* etmain/etkey\nwarning: This command deletes files in fs_homepath. If you are not sure how to use this command do not play with fire!");
		return;
	}

	// avoid unreferenced pk3 runtime issues (not on HD but still referenced in game)
#ifndef DEDICATED
	if (cls.state != CA_DISCONNECTED)
	{
		Com_Printf("You are connected to a server - '/disconnect' to run '/clean'.\n");
		return;
	}
#else
	if (com_sv_running && com_sv_running->integer)
	{
		Com_Printf("Server is running - '/killserver'  to run '/clean'.\n");
		return;
	}
#endif // DEDICATED

	Cvar_VariableStringBuffer("fs_homepath", path, sizeof(path));

	// If the first argument is "all" or "*", search the whole homepath
	// FIXME: add more options ? see #53
	if (Q_stricmp(Cmd_Argv(1), "all") && Q_stricmp(Cmd_Argv(1), "*"))
	{
		Q_strcat(path, sizeof(path), va("%c%s", PATH_SEP, Cmd_Argv(1)));
	}

	for (i = 2; i < Cmd_Argc(); i++)
	{
		pFiles = Sys_ListFiles(path, NULL, Cmd_Argv(i), &numFiles, qtrue);

		Com_Printf("Found %i files matching the pattern \"%s\" under %s\n", numFiles, Cmd_Argv(i), path);

		// debug
		//for (j = 0; j < numFiles; j++)
		//{
		//	Com_Printf("FILE[%i]: %s - pattern: %s\n", j + 1, pFiles[j], Cmd_Argv(i));
		//}

		for (j = 0; j < numFiles; j++)
		{
			whitelisted = qfalse;
			totalNumFiles++;

			// FIXME: - don't let admins force this! - move to dat file?
			//        - optimize - don't do this each loop! -> strtok modifies the input string, which is undefined behaviour on a literal char[], at least in C99
			//          & print the whitelist files on top again

			Cvar_VariableStringBuffer("com_cleanwhitelist", whitelist, sizeof(whitelist));
			// Prevent clumsy users from deleting important files - keep leading space!
			Q_strcat(whitelist, sizeof(whitelist), " .txt .cfg .dat .gm .way"); // no need to add *.so or *.dll, FS_Remove denies that per default

			//Com_DPrintf("Whitelist files/patterns: %s\n", whitelist);

			// Check if this file is in the whitelist
			tokens = strtok(whitelist, " ,;");

			while (tokens != NULL)
			{
				if (strstr(pFiles[j], tokens))
				{
					Com_Printf("- skipping file[%i]: %s%c%s - pattern: %s\n", j + 1, path, PATH_SEP, pFiles[j], tokens);
					whitelisted = qtrue;
					break;
				}
				tokens = strtok(NULL, " ,;");
			}

			if (whitelisted)
			{
				continue;
			}

			Com_Printf("- removing file[%i]: %s%c%s\n", j + 1, path, PATH_SEP, pFiles[j]);
			//remove(va("%s%c%s", path, PATH_SEP, pFiles[j])); // enable *.so & *.dll lib deletion
			FS_Remove(va("%s%c%s", path, PATH_SEP, pFiles[j]));
			delFiles++;
		}

		Sys_FreeFileList(pFiles);
		numFiles = 0;
	}
	Com_Printf("Path of fs_homepath cleaned - %i matches - %i files skipped - %i files deleted.\n", totalNumFiles, totalNumFiles - delFiles, delFiles);
}

/*
============
Cmd_Init
============
*/
void Cmd_Init(void)
{
	Cmd_AddCommand("cmdlist", Cmd_List_f);
	Cmd_AddCommand("exec", Cmd_Exec_f);
	Cmd_AddCommand("execq", Cmd_Exec_f);
	Cmd_SetCommandCompletionFunc("exec", Cmd_CompleteCfgName);
	Cmd_SetCommandCompletionFunc("execq", Cmd_CompleteCfgName);
	Cmd_AddCommand("vstr", Cmd_Vstr_f);
	Cmd_SetCommandCompletionFunc("vstr", Cvar_CompleteCvarName);
	Cmd_AddCommand("echo", Cmd_Echo_f);
	Cmd_AddCommand("wait", Cmd_Wait_f);
	Cmd_AddCommand("clean", Cmd_CleanHomepath_f);
}
