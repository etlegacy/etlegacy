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
 * @file botlib_stub.c
 * @brief this is a stub botlib so that we can compile without changes to the
 * rest of the engine. This way, we can drop in the real botlib later if we get
 * access to it.
 *
 * Notes:
 * + The l_* files are pilfered from extractfuncs.  They work, but
 *   I believe they're a bit out-of-date versus the real versions..
 * + I don't yet return real handles for the PC functions--instead, I
 *   pass around the real pointer to the source_t struct.  This is
 *   probably a bad thing, and it should be fixed down the road.
 */

#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"
#include "l_script.h"
#include "l_precomp.h"

void botlib_stub(void);
int PC_LoadSourceHandle(const char *);
int PC_FreeSourceHandle(int);
int PC_ReadTokenHandle(int, pc_token_t *);
int PC_SourceFileAndLine(int, char *, int *);
void PC_UnreadLastTokenHandle(int);

botlib_import_t botimport;

botlib_export_t *GetBotLibAPI(int version, botlib_import_t *imports)
{
	static botlib_export_t botlib_export;

	botimport = *imports;

	botlib_export.ea.EA_Say          = (void *)botlib_stub;
	botlib_export.ea.EA_SayTeam      = (void *)botlib_stub;
	botlib_export.ea.EA_UseItem      = (void *)botlib_stub;
	botlib_export.ea.EA_DropItem     = (void *)botlib_stub;
	botlib_export.ea.EA_UseInv       = (void *)botlib_stub;
	botlib_export.ea.EA_DropInv      = (void *)botlib_stub;
	botlib_export.ea.EA_Gesture      = (void *)botlib_stub;
	botlib_export.ea.EA_Command      = (void *)botlib_stub;
	botlib_export.ea.EA_SelectWeapon = (void *)botlib_stub;
	botlib_export.ea.EA_Talk         = (void *)botlib_stub;
	botlib_export.ea.EA_Attack       = (void *)botlib_stub;
	botlib_export.ea.EA_Reload       = (void *)botlib_stub;
	botlib_export.ea.EA_Use          = (void *)botlib_stub;
	botlib_export.ea.EA_Respawn      = (void *)botlib_stub;
	botlib_export.ea.EA_Jump         = (void *)botlib_stub;
	botlib_export.ea.EA_DelayedJump  = (void *)botlib_stub;
	botlib_export.ea.EA_Crouch       = (void *)botlib_stub;
	botlib_export.ea.EA_Walk         = (void *)botlib_stub;
	botlib_export.ea.EA_MoveUp       = (void *)botlib_stub;
	botlib_export.ea.EA_MoveDown     = (void *)botlib_stub;
	botlib_export.ea.EA_MoveForward  = (void *)botlib_stub;
	botlib_export.ea.EA_MoveBack     = (void *)botlib_stub;
	botlib_export.ea.EA_MoveLeft     = (void *)botlib_stub;
	botlib_export.ea.EA_MoveRight    = (void *)botlib_stub;
	botlib_export.ea.EA_Move         = (void *)botlib_stub;
	botlib_export.ea.EA_View         = (void *)botlib_stub;
	botlib_export.ea.EA_Prone        = (void *)botlib_stub;
	botlib_export.ea.EA_EndRegular   = (void *)botlib_stub;
	botlib_export.ea.EA_GetInput     = (void *)botlib_stub;
	botlib_export.ea.EA_ResetInput   = (void *)botlib_stub;


	botlib_export.BotLibSetup        = (void *)botlib_stub;
	botlib_export.BotLibShutdown     = (void *)botlib_stub;
	botlib_export.BotLibVarSet       = (void *)botlib_stub;
	botlib_export.BotLibVarGet       = (void *)botlib_stub;
	botlib_export.BotLibStartFrame   = (void *)botlib_stub;
	botlib_export.BotLibLoadMap      = (void *)botlib_stub;
	botlib_export.BotLibUpdateEntity = (void *)botlib_stub;
	botlib_export.Test               = (void *)botlib_stub;

	botlib_export.PC_AddGlobalDefine        = PC_AddGlobalDefine;
	botlib_export.PC_RemoveAllGlobalDefines = PC_RemoveAllGlobalDefines;
	botlib_export.PC_LoadSourceHandle       = PC_LoadSourceHandle;
	botlib_export.PC_FreeSourceHandle       = PC_FreeSourceHandle;
	botlib_export.PC_ReadTokenHandle        = PC_ReadTokenHandle;
	botlib_export.PC_SourceFileAndLine      = PC_SourceFileAndLine;
	botlib_export.PC_UnreadLastTokenHandle  = PC_UnreadLastTokenHandle;


	return &botlib_export;
}

void botlib_stub(void)
{
	botimport.Print(PRT_WARNING, "WARNING: botlib stub!\n");
}

int PC_LoadSourceHandle(const char *filename)
{
	// rain - FIXME - LoadSourceFile should take a const filename
	return (int)LoadSourceFile(filename);
}

int PC_FreeSourceHandle(int handle)
{
	FreeSource((source_t *)handle);
	return 0;
}

int PC_ReadTokenHandle(int handle, pc_token_t *token)
{
	token_t t;
	int     ret;

	ret = PC_ReadToken((source_t *)handle, &t);

	token->type       = t.type;
	token->subtype    = t.subtype;
	token->intvalue   = t.intvalue;
	token->floatvalue = t.floatvalue;
	Q_strncpyz(token->string, t.string, MAX_TOKENLENGTH);
	token->line         = t.line;
	token->linescrossed = t.linescrossed;

	// gamecode doesn't want the quotes on the string
	if (token->type == TT_STRING)
	{
		StripDoubleQuotes(token->string);
	}

	return ret;
}

int PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	source_t *source = (source_t *)handle;

	Q_strncpyz(filename, source->filename, 128);
	// ikkyo - i'm pretty sure token.line is the line of the last token
	//         parsed, not the line of the token currently being parsed...
	*line = source->token.line;

	return 0;
}

void PC_UnreadLastTokenHandle(int handle)
{
	PC_UnreadLastToken((source_t *)handle);
}
