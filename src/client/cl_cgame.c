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
 * @file cl_cgame.c
 * @brief Client system interaction with client game
 */

#include "client.h"
#include "sun_include.h"
#include "etrewind_version.h"
#include "../sys/sys_local.h"
#include "../botlib/botlib.h"

#define TRAP_EXTENSIONS_LIST cg_extensionTraps
#include "../qcommon/vm_ext.h"

static ext_trap_keys_t cg_extensionTraps[] =
{
	{ "trap_SysFlashWindow_Legacy",  CG_SYS_FLASH_WINDOW, qfalse },
	{ "trap_CommandComplete_Legacy", CG_COMMAND_COMPLETE, qfalse },
	{ "trap_CmdBackup_Ext_Legacy",   CG_CMDBACKUP_EXT,    qfalse },
	{ "trap_MatchPaused_Legacy",     CG_MATCHPAUSED,      qfalse },
	{ NULL,                          -1,                  qfalse }
};

extern botlib_export_t *botlib_export;

void Key_GetBindingBuf(int keynum, char *buf, int buflen);
void Key_KeynumToStringBuf(int keynum, char *buf, int buflen);

/**
 * @brief CL_GetGameState
 * @param[out] gs
 */
void CL_GetGameState(gameState_t *gs)
{
	*gs = cl.gameState;
}

/**
 * @brief CL_GetGlconfig
 * @param[out] config
 */
void CL_GetGlconfig(glconfig_t *config)
{
	// make sure we are only copying over vanilla data
	memcpy(config, &cls.glconfig, offsetof(glconfig_t, smpActive) + sizeof(qboolean));
}

/**
 * @brief CL_GetUserCmd
 * @param[in] cmdNumber
 * @param[out] ucmd
 * @return
 */
qboolean CL_GetUserCmd(int cmdNumber, usercmd_t *ucmd)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if (cmdNumber > cl.cmdNumber)
	{
		Com_Error(ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if (cmdNumber <= cl.cmdNumber - cl.cmdBackup)
	{
		return qfalse;
	}

	*ucmd = cl.cmds[cmdNumber & cl.cmdMask];

	return qtrue;
}

/**
 * @brief CL_GetCurrentCmdNumber
 * @return
 */
int CL_GetCurrentCmdNumber(void)
{
	return cl.cmdNumber;
}

/*
 * @brief CL_GetParseEntityState
 * @param[in] parseEntityNumber
 * @param[out] state
 * @return
 *
 * @note Unused
qboolean CL_GetParseEntityState(int parseEntityNumber, entityState_t *state)
{
    // can't return anything that hasn't been parsed yet
    if (parseEntityNumber >= cl.parseEntitiesNum)
    {
        Com_Error(ERR_DROP, "CL_GetParseEntityState: %i >= %i",
                  parseEntityNumber, cl.parseEntitiesNum);
    }

    // can't return anything that has been overwritten in the circular buffer
    if (parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES)
    {
        return qfalse;
    }

    *state = cl.parseEntities[parseEntityNumber & (MAX_PARSE_ENTITIES - 1)];
    return qtrue;
}
*/

/**
 * @brief CL_GetCurrentSnapshotNumber
 * @param[out] snapshotNumber
 * @param[out] serverTime
 */
void CL_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime)
{
	*snapshotNumber = cl.snap.messageNum;
	*serverTime     = cl.snap.serverTime;
}

/**
 * @brief CL_GetSnapshot
 * @param[in] snapshotNumber
 * @param[out] snapshot
 * @return
 */
qboolean CL_GetSnapshot(int snapshotNumber, snapshot_t *snapshot)
{
	clSnapshot_t *clSnap;
	int          i, count;

	if (snapshotNumber > cl.snap.messageNum)
	{
		Com_Error(ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if (cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP)
	{
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if (!clSnap->valid)
	{
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if (cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES)
	{
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags             = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping                  = clSnap->ping;
	snapshot->serverTime            = clSnap->serverTime;
	Com_Memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
	snapshot->ps = clSnap->ps;
	count        = clSnap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT)
	{
		Com_Printf("CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT);
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for (i = 0 ; i < count ; i++)
	{
		snapshot->entities[i] =
			cl.parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

extern cvar_t *sv_fps;

/**
 * @brief CL_InterpolationCheckRange
 */
static void CL_InterpolationCheckRange(void)
{
	int snaps, updateRate, buffer;

	if (!cl_interpolation->integer)
	{
		return;
	}

	snaps      = Cvar_VariableValue("snaps");
	updateRate = snaps < sv_fps->integer ? 1000 / snaps : 1000 / sv_fps->integer;
	buffer     = (FRAMETIME / 2) / updateRate - 1;

	if (cl_interpolation->integer > buffer)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' is over allowed buffer (%d > %d), setting to %d\n", cl_interpolation->name, cl_interpolation->integer, buffer, buffer);
		Cvar_SetValue(cl_interpolation->name, buffer);
	}
}

/**
 * @brief CL_SetSnapshotLerp
 */
void CL_SetSnapshotLerp(void)
{
	int i = 0, snapshotNumber = cl.snap.messageNum;

	CL_InterpolationCheckRange();

	while (i < cl_interpolation->integer)
	{
		if (cl.snapshots[--snapshotNumber & PACKET_MASK].valid)
		{
			i++;
		}

		// wrapped around
		if (cl.snapshots[snapshotNumber & PACKET_MASK].messageNum == cl.snap.messageNum)
		{
			break;
		}
	}

	cl.snapLerp = cl.snapshots[snapshotNumber & PACKET_MASK];
}

/**
 * @brief CL_SetUserCmdValue
 * @param[in] userCmdValue
 * @param[in] flags
 * @param[in] mask
 * @param[in] sensitivityScale
 * @param[in] mpIdentClient
 */
void CL_SetUserCmdValue(int userCmdValue, int flags, int mask, float sensitivityScale, int mpIdentClient)
{
	cl.cgameUserCmdValue  = userCmdValue;
	cl.cgameFlags         = (cl.cgameFlags & ~mask) | (flags & mask);
	cl.cgameSensitivity   = sensitivityScale;
	cl.cgameMpIdentClient = mpIdentClient;
}

/**
 * @brief CL_SetClientLerpOrigin
 * @param[in] x
 * @param[in] y
 * @param[in] z
 */
void CL_SetClientLerpOrigin(float x, float y, float z)
{
	cl.cgameClientLerpOrigin[0] = x;
	cl.cgameClientLerpOrigin[1] = y;
	cl.cgameClientLerpOrigin[2] = z;
}

/**
 * @brief CL_CGameCheckKeyExec
 * @param[in] key
 * @return
 */
qboolean CL_CGameCheckKeyExec(int key)
{
	if (cgvm)
	{
		return (qboolean)(VM_Call(cgvm, CG_CHECKEXECKEY, key));
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief CL_ConfigstringModified
 */
void CL_ConfigstringModified(void)
{
	char        *old, *s;
	int         i, index;
	char        *dup;
	gameState_t oldGs;
	int         len;

	index = Q_atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "configstring < 0 or configstring >= MAX_CONFIGSTRINGS");
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
	if (!strcmp(old, s))
	{
		return;     // unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	Com_Memset(&cl.gameState, 0, sizeof(cl.gameState));

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for (i = 0 ; i < MAX_CONFIGSTRINGS ; i++)
	{
		if (i == index)
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if (!dup[0])
		{
			continue;       // leave with the default empty string
		}

		len = strlen(dup);

		if (len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS)
		{
			Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
		Com_Memcpy(cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1);
		cl.gameState.dataCount += len + 1;
	}

	if (index == CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}
}

/**
 * @brief Set up argc/argv for the given command
 * @param[in] serverCommandNumber
 * @return
 */
qboolean CL_GetServerCommand(int serverCommandNumber)
{
	char        *s;
	char        *cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int         argc;
	qboolean    commentCommand = qfalse;

	// if we have irretrievably lost a reliable command, drop the connection
	if (serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if (clc.demo.playing)
		{
			return qfalse;
		}
		Com_Error(ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out");
		return qfalse;
	}

	if (serverCommandNumber > clc.serverCommandSequence)
	{
		Com_Error(ERR_DROP, "CL_GetServerCommand: requested a command not received");
		return qfalse;
	}

	s                             = clc.serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS - 1)];
	clc.lastExecutedServerCommand = serverCommandNumber;

	if (cl_showServerCommands->integer)
	{
		Com_DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);
	}

rescan:
	if (s && s[0] == '/' && s[1] == '/')
	{
		commentCommand = qtrue;
		Cmd_TokenizeString(s + 2);
	}
	else
	{
		Cmd_TokenizeString(s);
	}
	cmd  = Cmd_Argv(0);
	argc = Cmd_Argc();

	if (!strcmp(cmd, "disconnect"))
	{
		// allow server to indicate why they were disconnected
		if (argc >= 2)
		{
			Com_Error(ERR_SERVERDISCONNECT, "Server Disconnected - %s", Cmd_Argv(1));
		}
		else
		{
			Com_Error(ERR_SERVERDISCONNECT, "Server disconnected");
		}
	}

	if (commentCommand)
	{
		if (!strcmp(cmd, "auth-srv"))
		{
			if (clc.demo.playing)
			{
				return qfalse;
			}
#ifdef LEGACY_AUTH
			Auth_Server_Command_f();
#endif
		}

		return qfalse;
	}

	if (!strcmp(cmd, "bcs0"))
	{
		Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return qfalse;
	}

	if (!strcmp(cmd, "bcs1"))
	{
		s = Cmd_Argv(2);
		if (strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		Q_strcat(bigConfigString, sizeof(bigConfigString), s);
		return qfalse;
	}

	if (!strcmp(cmd, "bcs2"))
	{
		s = Cmd_Argv(2);
		if (strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		Q_strcat(bigConfigString, sizeof(bigConfigString), s);
		Q_strcat(bigConfigString, sizeof(bigConfigString), "\"");
		s = bigConfigString;
		goto rescan;
	}

	if (!strcmp(cmd, "cs"))
	{
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return qtrue;
	}

	if (!strcmp(cmd, "map_restart"))
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		// clear outgoing commands before passing
		Com_Memset(cl.cmds, 0, sizeof(cl.cmds));
		return qtrue;
	}

	if (!strcmp(cmd, "popup"))       // direct server to client popup request, bypassing cgame
	{
		return qfalse;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if (!strcmp(cmd, "clientLevelShot"))
	{
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if (!com_sv_running->integer)
		{
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}

/**
 * @brief Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
 *
 * @param[in] mapname
 *
 * @see SV_SetExpectedHunkUsage (Copied from server to here)
 */
void CL_SetExpectedHunkUsage(const char *mapname)
{
	int        handle;
	const char *memlistfile = "hunkusage.dat";
	int        len;

	len = FS_FOpenFileByMode(memlistfile, &handle, FS_READ);
	if (len >= 0)     // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
	{
		char *token;
		char *buftrav;
		char *buf = (char *)Z_Malloc(len + 1);

		Com_Memset(buf, 0, len + 1);

		FS_Read((void *)buf, len, handle);
		FS_FCloseFile(handle);

		// now parse the file, filtering out the current map
		buftrav = buf;

		COM_BeginParseSession("CL_SetExpectedHunkUsage");
		while ((token = COM_Parse(&buftrav)) != NULL && token[0])
		{
			if (!Q_stricmp(token, mapname))
			{
				// found a match
				token = COM_Parse(&buftrav);    // read the size
				if (token && token[0])
				{
					// this is the usage
					com_expectedhunkusage = Q_atoi(token);
					Z_Free(buf);
					return;
				}
			}
		}

		Z_Free(buf);
	}
	// just set it to a negative number,so the cgame knows not to draw the percent bar
	com_expectedhunkusage = -1;
}

/**
 * @brief CL_SendBinaryMessage
 * @param[in] buf
 * @param[in] buflen
 * @return
 */
static int CL_SendBinaryMessage(const char *buf, int buflen)
{
	if (buflen < 0 || buflen > MAX_BINARY_MESSAGE)
	{
		Com_Printf("CL_SendBinaryMessage: bad length %i", buflen);
		clc.binaryMessageLength = 0;
		return 0;
	}

	clc.binaryMessageLength = buflen;
	Com_Memcpy(clc.binaryMessage, buf, buflen);
	return 1;
}

/**
 * @brief CL_BinaryMessageStatus
 * @return
 */
static int CL_BinaryMessageStatus(void)
{
	if (clc.binaryMessageLength == 0)
	{
		return MESSAGE_EMPTY;
	}

	if (clc.binaryMessageOverflowed)
	{
		return MESSAGE_WAITING_OVERFLOW;
	}

	return MESSAGE_WAITING;
}

/**
 * @brief CL_CGameBinaryMessageReceived
 * @param[in] buf
 * @param[in] buflen
 * @param[in] serverTime
 */
void CL_CGameBinaryMessageReceived(const byte *buf, int buflen, int serverTime)
{
	// This should not happen, but someone may spice up the server to cause peoples clients to shutdown.
	// It happens because binary messages are just a bolted in functionality which is read after the main msg data is parser
	// aka packages longer than normal come here and if we don't have cgame loaded all hell brakes loose.
	if (!cgvm)
	{
		Com_Error(ERR_DROP, "Invalid server message received. Server is possibly doing something malicious.");
	}
	else
	{
		VM_Call(cgvm, CG_MESSAGERECEIVED, buf, buflen, serverTime);
	}
}

/**
 * @brief Just adds default parameters that cgame doesn't need to know about
 * @param[in] mapname
 */
void CL_CM_LoadMap(const char *mapname)
{
	unsigned int checksum;

	// If we are not running the server, then set expected usage here
	if (!com_sv_running->integer)
	{
		CL_SetExpectedHunkUsage(mapname);
	}
	else
	{
		// catch here when a local server is started to avoid outdated com_errorDiagnoseIP
		Cvar_Set("com_errorDiagnoseIP", "");
	}

	Con_ScrollBottom();

	CM_LoadMap(mapname, qtrue, &checksum);
}

/**
 * @brief CL_ShutdownCGame
 */
void CL_ShutdownCGame(void)
{
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_CGAME);
	cls.cgameStarted = qfalse;
	if (!cgvm)
	{
		return;
	}
	VM_Call(cgvm, CG_SHUTDOWN);
	VM_Free(cgvm);
	cgvm = NULL;
}

/**
 * @brief The cgame module is making a system call
 * @param[in] args
 * @return
 */
intptr_t CL_CgameSystemCalls(intptr_t *args)
{
	switch (args[0])
	{
	case CG_PRINT:
		Com_Printf("%s", (char *)VMA(1));
		return 0;
	case CG_ERROR:
		Com_Error(ERR_DROP, "%s", (char *)VMA(1));
	case CG_MILLISECONDS:
		return Sys_Milliseconds();
	case CG_CVAR_REGISTER:
		Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
		return 0;
	case CG_CVAR_UPDATE:
		Cvar_Update(VMA(1));
		return 0;
	case CG_CVAR_SET:
		Cvar_SetSafe(VMA(1), VMA(2));
		return 0;
	case CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
		return 0;
	case CG_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		Cvar_LatchedVariableStringBuffer(VMA(1), VMA(2), args[3]);
		return 0;
	case CG_ARGC:
		return Cmd_Argc();
	case CG_ARGV:
		Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
		return 0;
	case CG_ARGS:
		Cmd_ArgsBuffer(VMA(1), args[2]);
		return 0;
	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
	case CG_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case CG_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case CG_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;
	case CG_FS_GETFILELIST:
		return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
	case CG_FS_DELETEFILE:
		return FS_Delete(VMA(1));
	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText(VMA(1));
		return 0;
	case CG_ADDCOMMAND:
		Cmd_AddCommand(VMA(1));
		return 0;
	case CG_REMOVECOMMAND:
		Cmd_RemoveCommandSafe(VMA(1));
		return 0;
	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand(VMA(1));
		return 0;
	case CG_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
	case CG_CM_LOADMAP:
		CL_CM_LoadMap(VMA(1));
		return 0;
	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case CG_CM_INLINEMODEL:
		return CM_InlineModel(args[1]);
	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel(VMA(1), VMA(2), qfalse);
	case CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel(VMA(1), VMA(2), qtrue);
	case CG_CM_POINTCONTENTS:
		return CM_PointContents(VMA(1), args[2]);
	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContents(VMA(1), args[2], VMA(3), VMA(4));
	case CG_CM_BOXTRACE:
		CM_BoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qfalse);
		return 0;
	case CG_CM_CAPSULETRACE:
		CM_BoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qtrue);
		return 0;
	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule*/ qfalse);
		return 0;
	case CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule*/ qtrue);
		return 0;
	case CG_CM_MARKFRAGMENTS:
		return re.MarkFragments(args[1], VMA(2), VMA(3), args[4], VMA(5), args[6], VMA(7));

	case CG_R_PROJECTDECAL:
		re.ProjectDecal(args[1], args[2], VMA(3), VMA(4), VMA(5), args[6], args[7]);
		return 0;
	case CG_R_CLEARDECALS:
		re.ClearDecals();
		return 0;

	case CG_S_STARTSOUND:
		S_StartSound(VMA(1), args[2], args[3], args[4], args[5]);
		return 0;
	case CG_S_STARTSOUNDEX:
		S_StartSoundEx(VMA(1), args[2], args[3], args[4], args[5], args[6]);
		return 0;
	case CG_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], args[3]);
		return 0;
	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds();
		return 0;
	case CG_S_CLEARSOUNDS:
		if (args[1] == 0)
		{
			S_ClearSounds(qtrue, qfalse);
		}
		else if (args[1] == 1)
		{
			S_ClearSounds(qtrue, qtrue);
		}
		return 0;
	case CG_S_ADDLOOPINGSOUND:
		// FIXME handling of looping sounds changed
		S_AddLoopingSound(VMA(1), VMA(2), args[3], args[4], args[5], args[6]);
		return 0;
	case CG_S_ADDREALLOOPINGSOUND:
		S_AddRealLoopingSound(VMA(1), VMA(2), args[3], args[4], args[5], args[6]);
		return 0;
	case CG_S_STOPSTREAMINGSOUND:
		S_StopEntStreamingSound(args[1]);
		return 0;
	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition(args[1], VMA(2));
		return 0;
	// talking animations
	case CG_S_GETVOICEAMPLITUDE:
		return S_GetVoiceAmplitude(args[1]);

	case CG_S_GETSOUNDLENGTH:
		return S_GetSoundLength(args[1]);

	// for looped sound starts
	case CG_S_GETCURRENTSOUNDTIME:
		return S_GetCurrentSoundTime();

	case CG_S_RESPATIALIZE:
		S_Respatialize(args[1], VMA(2), VMA(3), args[4]);
		return 0;
	case CG_S_REGISTERSOUND:
		return S_RegisterSound(VMA(1), args[2]);
	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack(VMA(1), VMA(2), args[3]);    // added fadeup time
		return 0;
	case CG_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);     // added music/all-streaming options
		return 0;
	case CG_S_STARTSTREAMINGSOUND:
		return S_StartStreamingSound(VMA(1), VMA(2), args[3], args[4], args[5]);
	case CG_R_LOADWORLDMAP:
		re.LoadWorld(VMA(1));
		return 0;
	case CG_R_REGISTERMODEL:
		return re.RegisterModel(VMA(1));
	case CG_R_REGISTERSKIN:
		return re.RegisterSkin(VMA(1));
	case CG_R_GETSKINMODEL:
		return re.GetSkinModel(args[1], VMA(2), VMA(3));
	case CG_R_GETMODELSHADER:
		return re.GetShaderFromModel(args[1], args[2], args[3]);
	case CG_R_REGISTERSHADER:
		return re.RegisterShader(VMA(1));
	case CG_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip(VMA(1));
	case CG_R_REGISTERFONT:
		re.RegisterFont(VMA(1), args[2], VMA(3), (args[4] == qtrue));
		return 0;
	case CG_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	case CG_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene(VMA(1));
		return 0;
	case CG_R_ADDPOLYTOSCENE:
		re.AddPolyToScene(args[1], args[2], VMA(3));
		return 0;
	case CG_R_ADDPOLYSTOSCENE:
		re.AddPolysToScene(args[1], args[2], VMA(3), args[4]);
		return 0;
	case CG_R_ADDPOLYBUFFERTOSCENE:
		re.AddPolyBufferToScene(VMA(1));
		return 0;
	case CG_R_ADDLIGHTTOSCENE:
		re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
		return 0;
	case CG_R_ADDCORONATOSCENE:
		re.AddCoronaToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;
	case CG_R_SETFOG:
		re.SetFog(args[1], args[2], args[3], VMF(4), VMF(5), VMF(6), VMF(7));
		return 0;
	case CG_R_SETGLOBALFOG:
		re.SetGlobalFog(args[1], args[2], VMF(3), VMF(4), VMF(5), VMF(6));
		return 0;
	case CG_R_RENDERSCENE:
		re.RenderScene(VMA(1));
		return 0;
	case CG_R_SAVEVIEWPARMS:
		return 0;
	case CG_R_RESTOREVIEWPARMS:
		return 0;
	case CG_R_SETCOLOR:
		re.SetColor(VMA(1));
		return 0;
	case CG_R_DRAWSTRETCHPIC:
		re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;
	case CG_R_DRAWROTATEDPIC:
		re.DrawRotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
		return 0;
	case CG_R_DRAWSTRETCHPIC_GRADIENT:
		re.DrawStretchPicGradient(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMA(10), args[11]);
		return 0;
	case CG_R_DRAW2DPOLYS:
		re.Add2dPolys(VMA(1), args[2], args[3]);
		return 0;
	case CG_R_MODELBOUNDS:
		re.ModelBounds(args[1], VMA(2), VMA(3));
		return 0;
	case CG_R_LERPTAG:
		return re.LerpTag(VMA(1), VMA(2), VMA(3), args[4]);
	case CG_GETGLCONFIG:
		CL_GetGlconfig(VMA(1));
		return 0;
	case CG_GETGAMESTATE:
		CL_GetGameState(VMA(1));
		return 0;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber(VMA(1), VMA(2));
		return 0;
	case CG_GETSNAPSHOT:
		return CL_GetSnapshot(args[1], VMA(2));
	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand(args[1]);
	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case CG_GETUSERCMD:
		return CL_GetUserCmd(args[1], VMA(2));
	case CG_SETUSERCMDVALUE:
		CL_SetUserCmdValue(args[1], args[2], MASK_CGAMEFLAGS_SHOWGAMEVIEW, VMF(3), args[4]);
		return 0;
	case CG_SETCLIENTLERPORIGIN:
		CL_SetClientLerpOrigin(VMF(1), VMF(2), VMF(3));
		return 0;
	case CG_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();
	case CG_KEY_ISDOWN:
		return Key_IsDown(args[1]);
	case CG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case CG_KEY_SETCATCHER:
		Key_SetCatcherVM(args[1]);
		return 0;
	case CG_KEY_GETKEY:
		return Key_GetKey(VMA(1));

	case CG_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();
	case CG_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case CG_MEMSET:
		return (intptr_t)Com_Memset(VMA(1), args[2], args[3]);
	case CG_MEMCPY:
		return (intptr_t)Com_Memcpy(VMA(1), VMA(2), args[3]);
	case CG_STRNCPY:
		return (intptr_t)strncpy(VMA(1), VMA(2), args[3]);
	case CG_SIN:
		return Q_FloatAsInt(sin(VMF(1)));
	case CG_COS:
		return Q_FloatAsInt(cos(VMF(1)));
	case CG_ATAN2:
		return Q_FloatAsInt(atan2(VMF(1), VMF(2)));
	case CG_SQRT:
		return Q_FloatAsInt(sqrt(VMF(1)));
	case CG_FLOOR:
		return Q_FloatAsInt(floor(VMF(1)));
	case CG_CEIL:
		return Q_FloatAsInt(ceil(VMF(1)));
	case CG_ACOS:
		return Q_FloatAsInt(Q_acos(VMF(1)));

	case CG_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine(VMA(1));
	case CG_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle(VMA(1));
	case CG_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle(args[1]);
	case CG_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
	case CG_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
	case CG_PC_UNREAD_TOKEN:
		botlib_export->PC_UnreadLastTokenHandle(args[1]);
		return 0;

	case CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case CG_REAL_TIME:
		return Com_RealTime(VMA(1));
	case CG_SNAPVECTOR:
		Sys_SnapVector(VMA(1));
		return 0;

	case CG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case CG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case CG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case CG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case CG_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case CG_R_REMAP_SHADER:
		re.RemapShader(VMA(1), VMA(2), VMA(3));
		return 0;

	case CG_TESTPRINTINT:
		Com_Printf("%s%li\n", (char *)VMA(1), (long)args[2]);
		return 0;
	case CG_TESTPRINTFLOAT:
		Com_Printf("%s%f\n", (char *)VMA(1), VMF(2));
		return 0;

	case CG_GET_ENTITY_TOKEN:
		return re.GetEntityToken(VMA(1), args[2]);

	case CG_INGAME_POPUP:
		if (cls.state == CA_ACTIVE && !clc.demo.playing)
		{
			if (uivm)     // can be called as the system is shutting down
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, args[1]);
			}
		}
		return 0;

	case CG_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], VMA(2), args[3]);
		return 0;

	case CG_KEY_SETBINDING:
		Key_SetBinding(args[1], VMA(2));
		return 0;

	case CG_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
		return 0;

	case CG_KEY_BINDINGTOKEYS:
		Key_GetBindingByString(VMA(1), VMA(2), VMA(3));
		return 0;

	case CG_TRANSLATE_STRING:
		CL_TranslateStringMod(VMA(1), VMA(2));
		return 0;

	case CG_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], args[3]);
		return 0;

	case CG_R_INPVS:
		return re.inPVS(VMA(1), VMA(2));

	case CG_GETHUNKDATA:
		Com_GetHunkInfo(VMA(1), VMA(2));
		return 0;

	// binary channel
	case CG_SENDMESSAGE:
		return CL_SendBinaryMessage(VMA(1), args[2]);
	case CG_MESSAGESTATUS:
		return CL_BinaryMessageStatus();
	case CG_R_LOADDYNAMICSHADER:
		return re.LoadDynamicShader(VMA(1), VMA(2));
	case CG_R_RENDERTOTEXTURE:
		re.RenderToTexture(args[1], args[2], args[3], args[4], args[5]);
		return 0;
	case CG_R_GETTEXTUREID:
		return re.GetTextureId(VMA(1));
	// flush gl rendering buffers
	case CG_R_FINISH:
		re.Finish();
		return 0;

	case CG_LOADCAMERA:
	case CG_STARTCAMERA:
	case CG_STOPCAMERA:
	case CG_GETCAMERAINFO:
	case CG_PUMPEVENTLOOP:
	case CG_INGAME_CLOSEPOPUP:
	case CG_R_LIGHTFORPOINT: // re-added to avoid a crash when called - still in enum of cgameImport_t
		return 0;

	///< extensions
	case CG_TRAP_GETVALUE:
		return VM_Ext_GetValue(VMA(1), args[2], VMA(3));

	case CG_SYS_FLASH_WINDOW:
		GLimp_FlashWindow(args[1]);
		return 0;

	case CG_COMMAND_COMPLETE:
		Field_CompleteModSuggestion(VMA(1));
		return 0;

	case CG_CMDBACKUP_EXT:
		cl.cmdBackup = CMD_BACKUP_ETL;
		cl.cmdMask   = CMD_MASK_ETL;
		return 0;
	case CG_MATCHPAUSED:
		S_PauseSounds(args[1]);
		return 0;

	default:
		Com_Error(ERR_DROP, "Bad cgame system trap: %ld", (long int) args[0]);
		break;
	}
	return 0;
}

/**
 * @brief This updates the "hunkusage.dat" file with the current map and it's hunk usage count
 *
 * This is used for level loading, so we can show a percentage bar dependant on the amount
 * of hunk memory allocated so far
 *
 * This will be slightly inaccurate if some settings like sound quality are changed, but these
 * things should only account for a small variation (hopefully)
 */
void CL_UpdateLevelHunkUsage(void)
{
	int        handle;
	const char *memlistfile = "hunkusage.dat";
	char       outstr[256];
	int        len, memusage;

	memusage = Cvar_VariableIntegerValue("com_hunkused");

	len = FS_FOpenFileByMode(memlistfile, &handle, FS_READ);
	if (len >= 0)     // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value
	{
		char *buftrav, *outbuftrav;
		char *outbuf;
		char *token;
		char *buf;

		buf = (char *)Z_Malloc(len + 1);
		Com_Memset(buf, 0, len + 1);
		outbuf = (char *)Z_Malloc(len + 1);
		Com_Memset(outbuf, 0, len + 1);

		(void) FS_Read((void *)buf, len, handle);
		FS_FCloseFile(handle);

		// now parse the file, filtering out the current map
		buftrav       = buf;
		outbuftrav    = outbuf;
		outbuftrav[0] = '\0';

		COM_BeginParseSession("CL_UpdateLevelHunkUsage");
		while ((token = COM_Parse(&buftrav)) != NULL && token[0])
		{
			if (!Q_stricmp(token, cl.mapname))
			{
				// found a match
				token = COM_Parse(&buftrav); // read the size
				if (token && token[0])
				{
					if (Q_atoi(token) == memusage) // if it is the same, abort this process
					{
						Z_Free(buf);
						Z_Free(outbuf);
						return;
					}
				}
			}
			else // send it to the outbuf
			{
				Q_strcat(outbuftrav, len + 1, token);
				Q_strcat(outbuftrav, len + 1, " ");
				token = COM_Parse(&buftrav);    // read the size
				if (token && token[0])
				{
					Q_strcat(outbuftrav, len + 1, token);
					Q_strcat(outbuftrav, len + 1, "\n");
				}
				else
				{
					//Com_Error does memory clean up
					//Z_Free(buf);
					//Z_Free(outbuf);
					Com_Error(ERR_DROP, "hunkusage.dat file is corrupt");
				}
			}
		}

		handle = FS_FOpenFileWrite(memlistfile);
		if (handle < 0)
		{
			Com_Error(ERR_DROP, "cannot create %s", memlistfile);
		}
		// input file is parsed, now output to the new file
		len = strlen(outbuf);
		if (FS_Write((void *)outbuf, len, handle) != len)
		{
			Com_Error(ERR_DROP, "cannot write to %s", memlistfile);
		}
		FS_FCloseFile(handle);

		Z_Free(buf);
		Z_Free(outbuf);
	}
	// now append the current map to the current file
	(void) FS_FOpenFileByMode(memlistfile, &handle, FS_APPEND);
	if (handle < 0)
	{
		Com_Error(ERR_DROP, "cannot write to hunkusage.dat, check disk full");
	}
	Com_sprintf(outstr, sizeof(outstr), "%s %i\n", cl.mapname, memusage);
	(void) FS_Write(outstr, strlen(outstr), handle);
	FS_FCloseFile(handle);

	// now just open it and close it, so it gets copied to the pak dir
	len = FS_FOpenFileByMode(memlistfile, &handle, FS_READ);
	if (len >= 0)
	{
		FS_FCloseFile(handle);
	}
}

/**
 * @brief Should only be called by CL_StartHunkUsers
 */
void CL_InitCGame(void)
{
	const char *info;
	const char *mapname;
	int        t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info    = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cl.mapname, sizeof(cl.mapname), "maps/%s.bsp", mapname);

	// mark all extensions as inactive
	VM_Ext_ResetActive();

	// load the dll
	cgvm = VM_Create("cgame", qtrue, CL_CgameSystemCalls, VMI_NATIVE);
	if (!cgvm)
	{
		VM_Error(ERR_DROP, "cgame", Sys_GetDLLName("cgame"));
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	// bani - added clc.playing, since some mods need this at init time, and drawactiveframe is too late for them
	VM_Call(cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum, clc.demo.playing, qtrue, (clc.demo.playing ? &dpi : 0), ETLEGACY_VERSION_INT);

	// reset any CVAR_CHEAT cvars registered by cgame
	if (!clc.demo.playing && !cl_connectedToCheatServer)
	{
		Cvar_SetCheatState();
	}

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf("CL_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if (!Sys_LowPhysicalMemory())
	{
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify();

	// update the memory usage file
	CL_UpdateLevelHunkUsage();
}

/**
 * @brief See if the current console command is claimed by the cgame
 * @return
 */
qboolean CL_GameCommand(void)
{
	if (!cgvm)
	{
		return qfalse;
	}

	return (qboolean)(VM_Call(cgvm, CG_CONSOLE_COMMAND));
}

qboolean CL_GameCompleteCommand(void)
{
	if (!cgvm)
	{
		return qfalse;
	}

	if (!VM_Ext_IsActive(CG_COMMAND_COMPLETE))
	{
		return qfalse;
	}

	return (qboolean)(VM_Call(cgvm, CG_CONSOLE_COMPLETE_ARGUMENT));
}

/**
 * @brief CL_CGameRendering
 */
void CL_CGameRendering(void)
{
	VM_Call(cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, 0, clc.demo.playing);
	VM_Debug(0);
}

int threshold = -1;
int svFrameTime;
int clFrameTime;

/**
 * @brief CL_FindIncrementThreshold
 * @details Calculates the threshold used when deciding whether to roll time forward.
 * 			The threshold ensures the client won't encroach on the margin specified by
 * 			cl_extrapolationMargin because of non-synchronous client/server frame rates.
 * @return
 */
int CL_FindIncrementThreshold(void)
{
	int LCM;
	int min;
	int clTime;

	clFrameTime = cls.frametime;

	// handles zero duration between frames (often happens on map change)
	if (clFrameTime == 0 || svFrameTime == 0)
	{
		return 0;
	}

	// calculates the least common multiple of clFrameTime and svFrameTime
	// the LCM represents how long until the time over-run pattern repeats
	LCM = svFrameTime > clFrameTime ? svFrameTime : clFrameTime;
	while (1)
	{
		if (LCM % clFrameTime == 0 && LCM % svFrameTime == 0)
		{
			break;
		}
		++LCM;
	}

	min    = 0;
	clTime = 0;
	// finds the worst amount of client over-run assuming no initial spare time
	while (clTime <= LCM)
	{
		int svTime    = (clTime / svFrameTime) * svFrameTime;
		int spareTime = svTime - clTime;
		if (spareTime < min)
		{
			min = spareTime;
		}

		clTime += clFrameTime;
	}

	return abs(min);
}

#define RESET_TIME  500
#define HALVE_TIME  100

/**
 * @brief Adjust the clients view of server time.
 *
 * @details We attempt to have cl.serverTime exactly equal the server's view
 * of time plus the timeNudge, but with variable latencies over
 * the internet it will often need to drift a bit to match conditions.
 *
 * Our ideal time would be to have the adjusted time approach, but not pass,
 * the very latest snapshot.
 *
 * Adjustments are only made when a new snapshot arrives with a rational
 * latency, which keeps the adjustment process framerate independent and
 * prevents massive overadjustment during times of significant packet loss
 * or bursted delayed packets.
 *
 */
void CL_AdjustTimeDelta(void)
{
	int  newDelta;
	int  deltaDelta;
	char *deltaMessage;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if (clc.demo.playing)
	{
		return;
	}

	newDelta   = cl.snapLerp.serverTime - cls.realtime;
	deltaDelta = abs(newDelta - cl.serverTimeDelta);

	if (deltaDelta > RESET_TIME)
	{
		cl.baselineDelta = cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snapLerp.serverTime;   // FIXME: is this a problem for cgame?
		cl.serverTime    = cl.snapLerp.serverTime;

		if (cl_showTimeDelta->integer)
		{
			deltaMessage = "^1(reset";
		}
	}
	else if (deltaDelta > HALVE_TIME)
	{
		// fast adjust, cut the difference in half
		cl.serverTimeDelta = (cl.serverTimeDelta + newDelta) >> 1;

		if (cl_showTimeDelta->integer)
		{
			deltaMessage = "^1(halve";
		}
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if (com_timescale->value == 0.f || com_timescale->value == 1.f)
		{
			if (cl.extrapolatedSnapshot)
			{
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta     -= 2;
				// set a cmd packet flag so server is aware of delta decrease
				cl.cgameFlags |= MASK_CGAMEFLAGS_SERVERTIMEDELTA_BACKWARD;

				if (cl_showTimeDelta->integer)
				{
					deltaMessage = "^6(-2 ms";
				}
			}
			else
			{
				int svOldFrameTime = svFrameTime;
				int spareTime;

				if (com_sv_running->integer)
				{
					svFrameTime = 1000 / sv_fps->integer;
				}
				else
				{
					// must be done this way to avoid incorrect svFrameTime when on a slow client
					svFrameTime = (cl.snapshots[(cl.snapLerp.messageNum - 0) & PACKET_MASK].serverTime)
					              - (cl.snapshots[(cl.snapLerp.messageNum - 1) & PACKET_MASK].serverTime);
				}

				// find the new threshold if not set or client/server frametime has changed
				if (threshold == -1 || svFrameTime != svOldFrameTime || clFrameTime != cls.frametime)
				{
					threshold = CL_FindIncrementThreshold();
				}

				spareTime =
					cl.snapLerp.serverTime                // server time
					- (cls.realtime + cl.serverTimeDelta) // client time
					- cl_extrapolationMargin->integer;    // margin time

				if (spareTime > threshold)
				{
					// move our sense of time forward to minimize total latency
					cl.serverTimeDelta++;
					// set a cmd packet flag so server is aware of delta increment
					cl.cgameFlags |= MASK_CGAMEFLAGS_SERVERTIMEDELTA_FORWARD;

					if (cl_showTimeDelta->integer)
					{
						deltaMessage = "^5(+1 ms";
					}
				}
				else
				{
					if (cl_showTimeDelta->integer)
					{
						deltaMessage = "^o(none";
					}
				}
			}
		}
		else
		{
			if (cl_showTimeDelta->integer)
			{
				deltaMessage = "^9(disabled";
			}
		}
	}

	if (cl_showTimeDelta->integer)
	{
		int  drift      = cl.serverTimeDelta - cl.baselineDelta; // some negative drift is expected
		char terminator = (cl_showTimeDelta->integer & 4) ? '\n' : ' ';

		Com_Printf("%s | %i %i %i)%c", deltaMessage, cl.serverTimeDelta, deltaDelta, drift, terminator);
	}
}

/**
 * @brief CL_FirstSnapshot
 */
void CL_FirstSnapshot(void)
{
	// ignore snapshots that don't have entities
	if (cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE)
	{
		return;
	}
	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.baselineDelta               = cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime               = cl.snap.serverTime;
	clc.demo.timedemo.timeBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if (cl_activeAction->string[0])
	{
		Cbuf_AddText(cl_activeAction->string);
		Cbuf_AddText("\n");
		Cvar_Set("activeAction", "");
	}

#ifdef FEATURE_DBMS
	if (!clc.demo.playing)
	{
		DB_UpdateFavorite(cl_profile->string, cls.servername);
	}
#endif

	if (cl_showTimeDelta->integer)
	{
		Com_Printf("^2(first snapshot | serverTimeDelta = %i)\n", cl.serverTimeDelta);
	}
}

/**
 * @brief CL_SetCGameTime
 */
void CL_SetCGameTime(void)
{
	// getting a valid frame message ends the connection process
	if (cls.state != CA_ACTIVE)
	{
		if (cls.state != CA_PRIMED)
		{
			return;
		}
		if (clc.demo.playing)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if (!clc.demo.firstFrameSkipped)
			{
				clc.demo.firstFrameSkipped = qtrue;
				return;
			}
\n		// SunLight - demo seek
		if(clc.demoplaying) Check_DemoSeek();

			CL_ReadDemoMessage();
		}
		if (cl.newSnapshots)
		{
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if (cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if (!cl.snap.valid)
	{
		Com_Error(ERR_DROP, "CL_SetCGameTime: !cl.snap.valid");
	}

	// allow pause in single player
	if (sv_paused->integer && cl_paused->integer && com_sv_running->integer)
	{
		// paused
		return;
	}

	if (cl.snap.serverTime < cl.oldFrameServerTime)
	{
		// if this is a localhost, then we are probably loading a savegame
		if (!Q_stricmp(cls.servername, "localhost"))
		{
			// do nothing?
			CL_FirstSnapshot();
		}
		else
		{
			Com_Error(ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime");
		}
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if (clc.demo.playing && cl_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances
	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn = cl_timeNudge->integer;
		int spareTime;

		if (tn < -30)
		{
			tn = -30;
		}
		else if (tn > 30)
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if (cl.serverTime < cl.oldServerTime)
		{
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives

		spareTime =
			cl.snapLerp.serverTime                 //server
			- (cls.realtime + cl.serverTimeDelta); //client

		if (spareTime <= cl_extrapolationMargin->integer)
		{
			cl.extrapolatedSnapshot = qtrue;
		}

		if (cl_showTimeDelta->integer)
		{
			char colorCode;

			if (spareTime > cl_extrapolationMargin->integer)
			{
				colorCode = '7'; // excess time to spare (white)
			}
			else if (spareTime == cl_extrapolationMargin->integer)
			{
				colorCode = '2'; // exactly on target (green)
			}
			else if (spareTime >= 0)
			{
				colorCode = '3'; // margin in use (yellow)
			}
			else
			{
				colorCode = '1'; // margin exhausted (red)
			}

			if (cl_showTimeDelta->integer & 2 || cl.newSnapshots)
			{
				Com_Printf("^%c%+03i ", colorCode, spareTime);
			}
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if (cl.newSnapshots)
	{
		CL_AdjustTimeDelta();
	}

	if (clc.demo.playing)
	{
		CL_DemoRun();
	}
}
+//###################################################
+// SunLight - mod and server infos
+
+int ModIs(const char *name)
+{
+	const char *info;
+	const char *modname;
+
+	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SYSTEMINFO];
+	modname = Info_ValueForKey( info, "fs_game" );
+
+	return (!strcmp(modname, name));
+}
+
+//###################################################
+// player names - SunLight
+void VectorRotateY(float * source, float angle, float * dest)
+{
+	float rad, s, c;
+
+	rad = (float) (angle * M_PI/180);
+	s = (float) sin(rad);
+	c = (float) cos(rad);
+
+	dest[0] = s*source[2] + c*source[0];
+	dest[1] = source[1];
+	dest[2] = c*source[2] - (s*source[0]);
+}
+
+void VectorRotateZ(float * source, float angle, float * dest)
+{
+	float rad, s, c;
+
+	rad = (float) (angle * M_PI/180);
+	s = (float) sin(rad);
+	c = (float) cos(rad);
+
+	dest[0] = c*source[0] - s*source[1];
+	dest[1] = s*source[0] + c*source[1];
+	dest[2] = source[2];
+}
+
+void Normalize_Angle_2(float * ang)
+{
+	while (*ang < -360.0f) *ang += 360.0f;
+	while (*ang > 360.0f)	*ang -= 360.0f;
+	if (*ang > 180.0f)	*ang -= 360.0f;
+	if (*ang < -180.0f)	*ang += 360.0f;
+}
+
+// World to screen: returns 1 if visible
+int WorldToScreenCoords(vec3_t origin, vec3_t axis, vec3_t worldCoord, int *x, int *y, refdef_t *rd)
+{
+	float tan_fovx, tan_fovy;
+	float anglex, angley;
+	float sx, sy;
+	vec3_t point_direct;
+	vec3_t angles_1;
+	vec3_t angles_2;
+	vec3_t vector_2;
+	vec3_t vector_3;
+
+	tan_fovx = tan( DEG2RAD( rd->fov_x ) * 0.5 );
+	tan_fovy = tan( DEG2RAD( rd->fov_y ) * 0.5 );
+
+	VectorSubtract(worldCoord, origin, point_direct);
+	VectorNormalize(axis);
+	vectoangles(axis, angles_1);
+
+	VectorRotateZ(point_direct, -angles_1[YAW], vector_2);
+	VectorRotateY(vector_2, -angles_1[PITCH], vector_3);
+
+	vectoangles(vector_3, angles_2);
+	anglex = angles_2[YAW];
+	Normalize_Angle_2(&anglex);
+
+	vector_3[1] = 0;
+	vectoangles(vector_3, angles_2);
+	angley = angles_2[PITCH];
+	Normalize_Angle_2(&angley);
+
+	if(anglex >= rd->fov_x/2) return 0;
+	if(anglex <= -rd->fov_x/2) return 0;
+	if(angley >= rd->fov_y/2) return 0;
+	if(angley <= -rd->fov_y/2) return 0;
+
+	anglex = tan(DEG2RAD(anglex));
+	sx = (float)(rd->width/2)-((float)(rd->width/2) * (anglex / tan_fovx));
+	if(sx < 0 || sx > rd->width) return 0;
+	*x = (int)sx;
+
+	angley = tan(DEG2RAD(angley));
+	sy = (float)(rd->height/2)+((float)(rd->height/2) * (angley / tan_fovy));
+	if(sy < 0 || sy > rd->height) return 0;
+	*y = (int)sy;
+
+	return 1;
+}
+
+void SCR_DrawChar2( int x, int y, float size, int ch ) {
+	int row, col;
+	float frow, fcol;
+	float ax, ay, aw, ah;
+
+	ch &= 255;
+
+	if ( ch == ' ' ) return;
+	if ( y < -size ) return;
+
+	ax = x;
+	ay = y;
+	aw = size;
+	ah = size;
+	SCR_AdjustFrom640( &ax, &ay, &aw, &ah );
+
+	row = ch >> 4;
+	col = ch & 15;
+
+	frow = row * 0.0625;
+	fcol = col * 0.0625;
+	size = 0.0625;
+
+	re.DrawStretchPic( ax, ay, aw, ah,
+					   fcol, frow,
+					   fcol + size, frow + size,
+					   cls.charSetShader );
+}
+
+void SCR_DrawStringExtNoShadow( int x, int y, float size, const char *string, float *setColor, qboolean forceColor )
+{
+	vec4_t color;
+	const char  *s;
+	int xx;
+
+	color[0] = color[1] = color[2] = 0;
+	color[3] = setColor[3];
+
+	s = string;
+	xx = x;
+	re.SetColor( setColor );
+	while ( *s ) {
+		if ( Q_IsColorString( s ) ) {
+			if ( !forceColor ) {
+				if ( *( s + 1 ) == COLOR_NULL ) {
+					memcpy( color, setColor, sizeof( color ) );
+				} else {
+					memcpy( color, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( color ) );
+					color[3] = setColor[3];
+				}
+				color[3] = setColor[3];
+				re.SetColor( color );
+			}
+			s += 2;
+			continue;
+		}
+		SCR_DrawChar2( xx, y, size, *s );
+		xx += size;
+		s++;
+	}
+	re.SetColor( NULL );
+}
+
+qboolean CL_GetTag( int clientNum, char *tagname, orientation_t *or );
+void DrawPlayerName(refdef_t *rd, int clientnum, vec4_t color, qboolean force)
+{
+	int pos_x, pos_y;
+	vec3_t pos;
+	vec3_t temp;
+	vec3_t temp2;
+	char name[40];
+	orientation_t or;
+	float size;
+	float xscale = rd->width / 640.f;
+	float yscale = rd->height / 480.f;
+
+	if(CL_GetTag(clientnum, "tag_head", &or))
+	{
+		VectorCopy(rd->vieworg, temp);
+		VectorCopy(rd->viewaxis[0], temp2);
+		VectorCopy(or.origin, pos);
+		pos[2] += 20;
+
+		size = VectorDistance(temp, pos);
+		size = size / 700.f;
+		if(size > 1) size = 1;
+		size = ((1.0f-size) + 0.3) * 14;
+
+		if(WorldToScreenCoords(temp, temp2, pos, &pos_x, &pos_y, rd))
+		{
+			GetPlayerName(clientnum, name, 39);
+			SCR_DrawStringExtNoShadow((pos_x/xscale),
+				(pos_y/yscale)-(size/2),
+				size, name, color, force);
+		}
+	}
+}
+
+void DrawPlayerNames(refdef_t *rd)
+{
+	float color[4];
+	int i, team, team2;
+
+	color[0] = color[1] = color[2] = 1.0;
+	color[3] = 1.0;
+
+	if (demo_playernames > 1) team = get_player_team(last_stored_snap.ps.clientNum);
+
+	for (i = 0; i < MAX_CLIENTS; i++)
+	{
+		if (FindPlayerEntityInSnap(&last_stored_snap, i) == NULL) continue;
+		if (demo_playernames > 1)
+		{
+			team2 = get_player_team(i);
+			if((demo_playernames == 2) && (team2 == team)) continue;
+			if((demo_playernames == 3) && (team2 != team)) continue;
+		}
+
+		DrawPlayerName(rd, i, color, qfalse);
+	}
+}
+
+//###################################################
+// PLAYERPOV - SunLight
+void SunPStoENT(playerState_t *ps, entityState_t *s)
+{
+	int i;
+	qboolean noevents;
+
+	noevents = qfalse;
+	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR )
+	{
+		s->eType = ET_INVISIBLE;
+		noevents = qtrue;
+	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH )
+	{
+		s->eType = ET_INVISIBLE;
+		noevents = qtrue;
+	} else s->eType = ET_PLAYER;
+
+	s->number = ps->clientNum;
+
+	s->pos.trType = TR_INTERPOLATE;
+	VectorCopy( ps->origin, s->pos.trBase );
+	VectorCopy( ps->velocity, s->pos.trDelta);
+
+	s->apos.trType = TR_INTERPOLATE;
+	VectorCopy( ps->viewangles, s->apos.trBase );
+
+	if ( ps->movementDir > 128 ) {
+		s->angles2[YAW] = (float)ps->movementDir - 256;
+	} else {
+		s->angles2[YAW] = ps->movementDir;
+	}
+
+	s->legsAnim     = ps->legsAnim;
+	s->torsoAnim    = ps->torsoAnim;
+	s->clientNum    = ps->clientNum;
+
+	s->eFlags = ps->eFlags;
+
+	if (ps->eFlags & EF_DEAD) noevents = qtrue;
+	if ( ps->stats[STAT_HEALTH] <= 0 ) {
+		noevents = qtrue;
+		s->eFlags |= EF_DEAD;
+	} else {
+		s->eFlags &= ~EF_DEAD;
+	}
+
+	if ( ps->externalEvent ) {
+		s->event = ps->externalEvent;
+		s->eventParm = ps->externalEventParm;
+	} else if ( ps->entityEventSequence < ps->eventSequence ) {
+		int seq;
+
+		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
+			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
+		}
+		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
+		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
+		s->eventParm = ps->eventParms[ seq ];
+		ps->entityEventSequence++;
+	}
+
+	for (i = 0; i < MAX_EVENTS; i++)
+	{
+		s->events[i] = EV_NONE;
+		s->eventParms[i] = 0;
+	}
+
+	if(noevents == qfalse)
+	{
+		for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
+			s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
+			s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
+			s->eventSequence++;
+		}
+	} else {
+		s->event = EV_NONE;
+	}
+
+	s->weapon = ps->weapon;
+	s->groundEntityNum = ps->groundEntityNum;
+
+	s->powerups = 0;
+	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
+		if ( ps->powerups[ i ] ) {
+			s->powerups |= 1 << i;
+		}
+	}
+
+	s->nextWeapon = ps->nextWeapon;
+//	s->loopSound = ps->loopSound;
+	s->teamNum = ps->teamNum;
+	s->aiState = ps->aiState;
+}
+
+void SunENTtoPS(entityState_t *s, playerState_t *ps)
+{
+	int i, val;
+	playerState_t ps2;
+	vec3_t playerMins = {-18, -18, -24};
+	vec3_t playerMaxs = {18, 18, 48};
+
+	memset(&ps2, 0, sizeof(playerState_t));
+
+	ps2.commandTime = cl.snap.serverTime;
+	ps2.pm_type = PM_NORMAL;
+	ps2.pm_time = ps->pm_time;
+	ps2.gravity = ps->gravity;
+	ps2.aiState = s->aiState;
+
+	for(i = 0;i < MAX_WEAPONS;i++) {
+		ps2.ammo[i] = 999;
+		ps2.ammoclip[i] = 999;
+		COM_BitSet(ps2.weapons, i); // like a 'give all', or silent will crash on EV_NOAMMO
+	}
+
+	ps2.clientNum = s->number;
+	ps2.eFlags = s->eFlags;
+	ps2.stats[STAT_HEALTH] = 100;
+
+	VectorCopy(playerMaxs, ps2.maxs);
+	VectorCopy(playerMins, ps2.mins);
+
+	ps2.crouchMaxZ = ps->crouchMaxZ;
+	ps2.crouchViewHeight = ps->crouchViewHeight;
+	ps2.standViewHeight = ps->standViewHeight;
+	ps2.deadViewHeight = ps->deadViewHeight;
+
+	ps2.viewheight = ps->standViewHeight;
+
+	if(s->eFlags & EF_CROUCHING) {
+		ps2.pm_flags |= PMF_DUCKED;
+		ps2.maxs[2] = ps->crouchMaxZ;
+		ps2.viewheight = ps->crouchViewHeight;
+	} else if((s->eFlags & EF_PRONE) || (s->eFlags & EF_PRONE_MOVING)) {
+		ps2.pm_flags |= PMF_DUCKED;
+		ps2.viewheight = PRONE_VIEWHEIGHT;
+	}
+
+	ps2.runSpeedScale = ps->runSpeedScale;
+	ps2.sprintSpeedScale = ps->sprintSpeedScale;
+	ps2.crouchSpeedScale = ps->crouchSpeedScale;
+
+	ps2.weaponstate = WEAPON_READY;
+	ps2.friction = 1.0;
+
+	VectorCopy(s->pos.trBase,ps2.origin);
+	VectorCopy(s->pos.trDelta,ps2.velocity);
+	VectorCopy(s->apos.trBase,ps2.viewangles);
+
+	ps2.teamNum = s->teamNum;
+	if(ModIs("jaymod")) ps2.persistant[1] = s->teamNum;
+	else
+	{
+		ps2.persistant[PERS_TEAM] = s->teamNum;
+		ps2.persistant[PERS_RESPAWNS_LEFT] = -1;
+	}
+
+	ps2.groundEntityNum = s->groundEntityNum;
+
+	for(i = 0;i < MAX_POWERUPS;i++) {
+		val = (s->powerups >> i) & 1;
+		ps2.powerups[i] = val;
+	}
+
+	ps2.weapon = s->weapon;
+	ps2.nextWeapon = s->nextWeapon;
+
+	ps2.externalEvent = s->event;
+	ps2.externalEventParm = s->eventParm;
+	ps2.externalEventTime = cl.snap.serverTime;
+
+	for(i = 0; i < MAX_EVENTS;i++)
+		ps2.events[i] = s->events[i];
+	ps2.eventSequence = s->eventSequence;
+	ps2.entityEventSequence = s->eventSequence;
+
+	ps2.pm_flags |= PMF_FOLLOW;
+
+	memcpy(ps, &ps2, sizeof(playerState_t));
+	return;
+}
+
+entityState_t * FindPlayerEntityInSnap(snapshot_t *snap, int clientnum)
+{
+	entityState_t *s;
+	int i;
+
+	s = NULL;
+	if((clientnum >= 0) && (clientnum < MAX_CLIENTS))
+	{
+		for (i = 0; i < snap->numEntities; i++)
+		{
+			s = &snap->entities[i];
+			if(s->eType!= ET_PLAYER) continue;
+			if(s->number == clientnum) break;
+		}
+
+		if(i == snap->numEntities) return NULL;
+		if(s->eFlags & EF_DEAD) return NULL;
+	}
+	return s;
+}
+
+int demo_follow_validview = 0;
+
+void PlayerPOV(snapshot_t *snap, int clientnum)
+{
+	playerState_t * ps;
+	playerState_t backup;
+	entityState_t *s;
+
+	s = NULL;
+	demo_follow_validview = 0;
+
+	ps = &snap->ps;
+	demo_follow_original_player = ps->clientNum;
+
+	s = FindPlayerEntityInSnap(snap, clientnum);
+	if(s == NULL) return;
+
+	memcpy(&backup, ps, sizeof(playerState_t));
+	SunENTtoPS(s, ps);
+
+	// checks if the original player is dead.
+	// if yes, the entity should be a corpse or removed altogether.
+	if (!( (backup.eFlags & EF_DEAD) || (backup.stats[STAT_HEALTH] <= 0) ))
+		SunPStoENT(&backup, s);
+
+	demo_follow_validview = 1;
+}
+
+
+//###################################################
+// DEMO 'WARPOMETER' - SunLight
+float warp_vel_ideal[WARPOMETER_BACKUP];
+float warp_vel_real[WARPOMETER_BACKUP];
+unsigned char warp_ground[WARPOMETER_BACKUP];
+int warp_graph_index = 0;
+
+void Set_Warpometer(snapshot_t *snap, snapshot_t *previous)
+{
+	float vel, idealvel;
+	vec3_t pos1;
+	vec3_t pos2;
+	entityState_t *ent1;
+	entityState_t *ent2;
+
+	if(snap->ps.clientNum != previous->ps.clientNum) return;
+
+	if(demo_warpometer_clientnum == previous->ps.clientNum)
+	{
+		VectorCopy(previous->ps.origin, pos1);
+		VectorCopy(snap->ps.origin, pos2);
+
+		pos1[2] = 0;
+		pos2[2] = 0;
+
+		vel = VectorDistance(pos1, pos2);
+
+		idealvel = sqrt(previous->ps.velocity[0]*previous->ps.velocity[0]+
+					previous->ps.velocity[1]*previous->ps.velocity[1]);
+
+		warp_vel_ideal[warp_graph_index&WARPOMETER_MASK] = idealvel/20;
+		warp_vel_real[warp_graph_index&WARPOMETER_MASK] = vel;
+		if(previous->ps.groundEntityNum != -1) warp_ground[warp_graph_index&WARPOMETER_MASK] = 1;
+		else warp_ground[warp_graph_index&WARPOMETER_MASK] = 0;
+
+		warp_graph_index ++;
+	} else
+	{
+		ent1 = FindPlayerEntityInSnap(previous, demo_warpometer_clientnum);
+		if(ent1 == NULL) return;
+		ent2 = FindPlayerEntityInSnap(snap, demo_warpometer_clientnum);
+		if(ent2 == NULL) return;
+
+		VectorCopy(ent1->pos.trBase, pos1);
+		VectorCopy(ent2->pos.trBase, pos2);
+
+		pos1[2] = 0;
+		pos2[2] = 0;
+
+		vel = VectorDistance(pos1, pos2);
+		idealvel = sqrt(ent1->pos.trDelta[0]*ent1->pos.trDelta[0]+
+					ent1->pos.trDelta[1]*ent1->pos.trDelta[1]);
+
+		warp_vel_ideal[warp_graph_index&WARPOMETER_MASK] = idealvel/20;
+		warp_vel_real[warp_graph_index&WARPOMETER_MASK] = vel;
+		if(ent1->groundEntityNum != -1) warp_ground[warp_graph_index&WARPOMETER_MASK] = 1;
+		else warp_ground[warp_graph_index&WARPOMETER_MASK] = 0;
+
+		warp_graph_index ++;
+	}
+}
+
+void Draw_Warpometer(void)
+{
+	float color[4];
+	char text[200];
+	char temp[33];
+	int i, pos;
+	float val;
+	float diff_total;
+	int samples;
+
+	float ground_diff;
+	int samples_ground;
+
+
+	color[0] = color[1] = color[2] = 0;
+	color[3] = 0.5f;
+	SCR_FillRect( SCREEN_WIDTH-256, 100, 256, SCREEN_HEIGHT-200, color );
+
+	color[0] = color[1] = 0;
+	color[2] = 1.0f;
+	color[3] = 1.0f;
+
+	for(i = 0;i < WARPOMETER_MASK;i++)
+	{
+		pos = warp_graph_index + WARPOMETER_MASK - i;
+		val = warp_vel_ideal[pos&WARPOMETER_MASK] * 2;
+		if (val > SCREEN_HEIGHT-200 - 30) val = SCREEN_HEIGHT-200 - 30;
+
+		SCR_FillRect( SCREEN_WIDTH-i-1, SCREEN_HEIGHT-val-105, 1, val, color );
+	}
+
+	color[0] = color[1] = 1.0f;
+	color[2] = 0;
+	color[3] = 0.75f;
+	for(i = 0;i < WARPOMETER_MASK;i++)
+	{
+		pos = warp_graph_index + WARPOMETER_MASK - i;
+		val = warp_vel_real[pos&WARPOMETER_MASK] * 2;
+		if (val > SCREEN_HEIGHT-200 - 30) val = SCREEN_HEIGHT-200 - 30;
+
+		SCR_FillRect( SCREEN_WIDTH-i-1, SCREEN_HEIGHT-val-105, 1, val, color );
+	}
+
+	diff_total = 0;
+	samples = 0;
+	ground_diff = 0;
+	samples_ground = 0;
+
+	for(i = 0;i < WARPOMETER_MASK;i++)
+	{
+		if(warp_vel_ideal[i] > 1)
+		{
+			val = (float)(warp_vel_real[i]-warp_vel_ideal[i]);
+			val = fabs(val) / (float)warp_vel_ideal[i];
+			diff_total += val;
+			samples++;
+		}
+
+		if(warp_ground[i] == 1)
+		{
+			if(warp_vel_real[i] >= warp_vel_ideal[i])
+			{
+				ground_diff += (warp_vel_real[i]-warp_vel_ideal[i]);
+				samples_ground++;
+			}
+		}
+	}
+
+	color[0] = color[1] = color[2] = 1.0f;
+	color[3] = 1.0f;
+
+	strcpy(text, "^3MOVEGRAPH:^7 ");
+	GetPlayerName(demo_warpometer_clientnum, temp, 32);
+	strcat(text, temp);
+	SCR_DrawStringExtNoShadow(SCREEN_WIDTH-256+3, 100+3, 8, text, color, qfalse);
+
+	if(samples != 0)
+	{
+		sprintf(text, "avg. diff:   %.2f%%", (diff_total*100)/(float)samples);
+		SCR_DrawStringExtNoShadow(SCREEN_WIDTH-256+3, 100+3+10, 8, text, color, qfalse);
+	}
+	if(samples_ground != 0)
+	{
+		sprintf(text, "ground diff: %.1f", ground_diff/(float)samples_ground);
+		SCR_DrawStringExtNoShadow(SCREEN_WIDTH-256+3, 100+3+20, 8, text, color, qfalse);
+	}
+}
+
+//###################################################
+// DEMO UNLAGGING - SunLight
+void UnlagLerpPos(vec3_t start, vec3_t end, float frac, vec3_t result)
+{
+	int i;
+	for(i=0;i<3;i++) result[i] = start[i] + frac * (end[i] - start[i]);
+}
+
+#define UNLAG_SNAPSHOT_BACKUP (0x0f+1)
+#define UNLAG_SNAPSHOT_MASK 0x0f
+snapshot_t unlag_snaps[UNLAG_SNAPSHOT_BACKUP];
+int snaps_stored = 0;
+snapshot_t last_stored_snap;
+
+void DemoUnlag(snapshot_t *snap)
+{
+	int old_snapnum;
+	int i,j, num_ent;
+	snapshot_t *old_snap;
+
+	if ( !clc.demoplaying ) return;
+	if (snap == NULL) return;
+
+	if(no_damage_kick)
+	{
+		snap->ps.damageEvent = 0;
+		snap->ps.damageCount = 0;
+	}
+
+	// changes pov before storing, this way it's possible to unlag that player as well
+	if(demo_follow_enabled)
+	{
+		if(demo_follow_attacker) {
+			if(ModIs("jaymod")) demo_follow_clientnum = snap->ps.persistant[3];
+			else demo_follow_clientnum = snap->ps.persistant[PERS_ATTACKER];
+
+			if(FindPlayerEntityInSnap(snap, demo_follow_clientnum) != NULL)
+				PlayerPOV(snap, demo_follow_clientnum);
+			else demo_follow_clientnum = -1;
+		} else	{
+			PlayerPOV(snap, demo_follow_clientnum);
+		}
+	}
+
+	memcpy( &unlag_snaps[snaps_stored & UNLAG_SNAPSHOT_MASK], snap, sizeof(snapshot_t) );
+	snaps_stored++;
+
+	// checks player movement before changing anything
+	if(demo_warpometer_enabled && (snaps_stored > 2))
+	{
+		snapshot_t * previous;
+
+		previous = &unlag_snaps[(snaps_stored-2) & UNLAG_SNAPSHOT_MASK];
+		Set_Warpometer(snap, previous);
+	}
+
+	if(demo_unlag_value == -1) // auto unlag from playerstate ping
+	{
+		int target_time, snap_num;
+		float frac;
+		snapshot_t *old_snap1, *old_snap2;
+		int time1, time2;
+		int ent_index1, ent_index2, k;
+
+		if(demo_follow_enabled) goto skip_unlag; // can't tell the ping of a player not in ps
+		target_time = snap->ps.commandTime-35; // sets to 35ms for best visuals, 50 is too much ahead
+
+		if(snaps_stored <= UNLAG_SNAPSHOT_BACKUP) goto skip_unlag;
+
+		for(snap_num = snaps_stored-1; // last stored
+			snap_num > snaps_stored-1-UNLAG_SNAPSHOT_BACKUP;
+			snap_num--)
+		{
+			if(unlag_snaps[snap_num & UNLAG_SNAPSHOT_MASK].serverTime <= target_time)
+				break;
+		}
+
+		if((snap_num <= snaps_stored-2) && (snap_num > snaps_stored-1-UNLAG_SNAPSHOT_BACKUP))
+		{
+			time1 = unlag_snaps[snap_num & UNLAG_SNAPSHOT_MASK].serverTime;
+			time2 = unlag_snaps[(snap_num+1) & UNLAG_SNAPSHOT_MASK].serverTime;
+			frac = (float)(target_time-time1)/(float)(time2-time1);
+
+			old_snap1 = &unlag_snaps[snap_num & UNLAG_SNAPSHOT_MASK];
+			old_snap2 = &unlag_snaps[(snap_num+1) & UNLAG_SNAPSHOT_MASK];
+
+			for(i=0;i<MAX_ENTITIES_IN_SNAPSHOT;i++)
+			{
+				if(snap->entities[i].eType != ET_PLAYER) continue;
+
+				num_ent = snap->entities[i].number;
+				if(num_ent == snap->ps.clientNum) continue;
+
+				ent_index1 = -1;
+				ent_index2 = -1;
+
+				// let's see if this player is present in both snaps
+				for(j=0;j<MAX_ENTITIES_IN_SNAPSHOT;j++)	{
+					if((old_snap1->entities[j].number == num_ent) &&
+						(old_snap1->entities[j].eType == snap->entities[i].eType))
+					{
+						ent_index1 = j;
+						break;
+					}
+				}
+
+				for(j=0;j<MAX_ENTITIES_IN_SNAPSHOT;j++)	{
+					if((old_snap2->entities[j].number == num_ent) &&
+						(old_snap2->entities[j].eType == snap->entities[i].eType))
+					{
+						ent_index2 = j;
+						break;
+					}
+				}
+
+				if((ent_index1 != -1) && (ent_index2 != -1))
+				{
+					snapshot_t *best;
+					int ent_index;
+
+					if (frac <= 0.5) {
+						best = old_snap1;
+						ent_index = ent_index1;
+					} else {
+						best = old_snap2;
+						ent_index = ent_index2;
+					}
+
+					VectorCopy(best->entities[ent_index].origin, snap->entities[i].origin);
+					VectorCopy(best->entities[ent_index].origin2, snap->entities[i].origin2);
+
+					VectorCopy(best->entities[ent_index].angles, snap->entities[i].angles);
+					VectorCopy(best->entities[ent_index].angles2, snap->entities[i].angles2);
+
+					snap->entities[i].groundEntityNum = best->entities[ent_index].groundEntityNum;
+					snap->entities[i].frame = best->entities[ent_index].frame;
+					snap->entities[i].solid = best->entities[ent_index].solid;
+
+					snap->entities[i].legsAnim = best->entities[ent_index].legsAnim;
+					snap->entities[i].torsoAnim = best->entities[ent_index].torsoAnim;
+					snap->entities[i].animMovetype = best->entities[ent_index].animMovetype;
+
+					if (frac <= 0.5) {
+						memcpy(&snap->entities[i],
+							&old_snap1->entities[ent_index1],
+							sizeof(entityState_t));
+					} else {
+						memcpy(&snap->entities[i],
+							&old_snap2->entities[ent_index2],
+							sizeof(entityState_t));
+					}
+
+					// and then lerps the position and angles
+					UnlagLerpPos(old_snap1->entities[ent_index1].pos.trBase,
+						old_snap2->entities[ent_index2].pos.trBase,
+						frac, snap->entities[i].pos.trBase);
+
+					for(k=0;k<3;k++)
+					{
+						snap->entities[i].apos.trBase[k] = LerpAngle(
+							old_snap1->entities[ent_index1].apos.trBase[k],
+							old_snap2->entities[ent_index2].apos.trBase[k],
+							frac);
+					}
+				}
+			} // for(i=0;i<MAX_ENTITIES_IN_SNAPSHOT;i++)
+		} // if(snap_num <= snaps_stored-2)
+	}
+	else
+	{
+		if(demo_unlag_value)
+		{
+			if((demo_unlag_value/50) + 2 > UNLAG_SNAPSHOT_BACKUP)
+				demo_unlag_value = (UNLAG_SNAPSHOT_BACKUP - 2) * 50;
+		}
+
+		if(demo_unlag_value && (snaps_stored > (demo_unlag_value/50) + 2))
+		{
+			old_snapnum = (snaps_stored - 2 - (demo_unlag_value/50)) &
+				UNLAG_SNAPSHOT_MASK;
+			old_snap = &unlag_snaps[old_snapnum];
+
+			// copies players position
+			for(i=0;i<MAX_ENTITIES_IN_SNAPSHOT;i++)
+			{
+				if((snap->entities[i].eType != ET_PLAYER) &&
+					(snap->entities[i].eType != ET_CORPSE))
+					continue;
+
+				num_ent = snap->entities[i].number;
+				if(num_ent == snap->ps.clientNum) continue;
+
+				// now finds the matching ent in the old snap, and if found, copies it
+				for(j=0;j<MAX_ENTITIES_IN_SNAPSHOT;j++)
+				{
+					if(old_snap->entities[j].number != num_ent) continue;
+					if(old_snap->entities[j].eType == snap->entities[i].eType)
+					{
+						memcpy(&snap->entities[i], &old_snap->entities[j],
+							sizeof(entityState_t));
+						break;
+					}
+				}
+			}
+		}
+	}
+skip_unlag:
+	snap->ping = (snap->serverTime - snap->ps.commandTime) - 50;
+	memcpy( &last_stored_snap, snap, sizeof(snapshot_t) );
+}
+
 /*
 ====================
 CL_GetSnapshot
 ====================
 */
+
+extern int last_requested_snapshot; // SunLight - to keep track of how many snapshots ahead we are
+
 qboolean    CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
 	clSnapshot_t    *clSnap;
 	int i, count;
 
+	// SunLight - don't send old snapshots for seeking
+	last_requested_snapshot = snapshotNumber;
+
+	if(clc.demoplaying)
+	{
+		int difference;
+
+		if(snapshotNumber < cl.snap.messageNum) // Avoids giving a snap to cgame
+		{
+			return qfalse;
+		}
+
+		difference = cl.snap.serverTime - (cl.serverTimeDelta+cls.realtime);
+		if(difference > 100)
+		{
+			if(demo_is_seeking) cls.realtime += difference; // only when seeking, otherwise laggy demos will skip
+		}
+	}
+
 	if ( snapshotNumber > cl.snap.messageNum ) {
 		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
 	}
 
 	// FIXME: configstring changes and server commands!!!
 
+	// SunLight
+	DemoUnlag(snapshot);
+
 	return qtrue;
 }
 
 		// when a demo record was started after the client got a whole bunch of
 		// reliable commands then the client never got those first reliable commands
 		if ( clc.demoplaying ) {
+			if(serverCommandNumber >= first_serverCommandSequence)
+			{
+				Com_Error( ERR_DROP,
+					"CL_GetServerCommand:\nA reliable command was cycled out! %d <= %d - %d, first=%d\n",
+					 serverCommandNumber, clc.serverCommandSequence, MAX_RELIABLE_COMMANDS,
+					first_serverCommandSequence);
+			}
 			return qfalse;
 		}
 		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
 		re.SetGlobalFog( args[1], args[2], VMF( 3 ), VMF( 4 ), VMF( 5 ), VMF( 6 ) );
 		return 0;
 	case CG_R_RENDERSCENE:
-		re.RenderScene( VMA( 1 ) );
+		{
+			// SunLight
+			refdef_t *rd;
+			rd = VMA(1);
+
+			if(clc.demoplaying && abs(cls.glconfig.vidWidth - rd->width) < 100)
+			{
+				if(demo_force_zoom_enabled)	{
+					float ratio = rd->fov_y / rd->fov_x;
+					rd->fov_x = demo_force_zoom_x;
+					rd->fov_y = (float)demo_force_zoom_y * ratio;
+				}
+				if(demo_force_zoom_shift) {
+					float ratio = rd->fov_y / rd->fov_x;
+					rd->fov_x = (float)demo_shift_fov;
+					rd->fov_y = (float)demo_shift_fov*ratio;
+				}
+			}
+
+			re.RenderScene(rd);
+
+			if(clc.demoplaying && demo_playernames &&
+				(abs(cls.glconfig.vidWidth - rd->width) < 100))
+				DrawPlayerNames(rd);
+			else if(demo_follow_validview) {
+				float color[4];
+				color[0] = color[1] = color[2] = 1.0;
+				color[3] = 1.0;
+				DrawPlayerName(rd, demo_follow_original_player, color, qfalse);
+			}
+
 		return 0;
+		}
 	case CG_R_SAVEVIEWPARMS:
 		re.SaveViewParms();
 		return 0;
 CL_CGameRendering
 =====================
 */
-void CL_CGameRendering( stereoFrame_t stereo ) {
-/*	static int x = 0;
-	if(!((++x) % 20)) {
-		Com_Printf( "numtraces: %i\n", numtraces / 20 );
-		numtraces = 0;
-	} else {
-	}*/
+
+void CL_CGameRendering( stereoFrame_t stereo )
+{
+	float xscale;
+	float yscale;
 
 	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
+
+	// SunLight -- extra hud info
+	if(!demo_is_seeking)
+	{
+		xscale = cls.glconfig.vidWidth / 640.0;
+		yscale = cls.glconfig.vidHeight / 480.0;
+
+		if (clc.demoplaying)
+		{
+			float color[4];
+			char str[256];
+			char str2[256];
+			float w2, h2;
+			int textwidth;
+			int pos2;
+
+			// these are in 640 coords, while SMALLCHAR_WIDTH, etc. is in real res.
+			w2 = (SMALLCHAR_WIDTH+1) / xscale;
+			h2 = (SMALLCHAR_HEIGHT + 15) / yscale;
+
+			Q_strncpyz(str, etr_ver, 256);
+			Q_strcat(str, 256, " V");
+			Q_strcat(str, 256, ETREWIND_VERSION);
+
+			pos2 = (strlen(str) + 3);
+
+			if(cls.state == CA_ACTIVE)
+			{
+				if(demo_follow_enabled && demo_follow_validview &&
+					(demo_follow_clientnum != -1)) {
+					color[0] =  1.0f;
+					color[1] = color[2] = 0;
+					color[3] = 0.75f;
+				} else {
+					color[0] = color[1] = color[2] = 0;
+					color[3] = 0.5f;
+				}
+				SCR_FillRect( 0, 0, SCREEN_WIDTH, h2, color );
+
+				sprintf(str2, " - normals: %d - realview: %d", (int)Cvar_VariableValue("r_shownormals"), no_damage_kick);
+				Q_strcat(str, 256, str2);
+
+				if(demo_unlag_value == 0)
+				{
+					Q_strncpyz(str2, " - unlag:off", 256);
+				} else if(demo_unlag_value == -1) {
+					Q_strncpyz(str2, " - unlag:auto", 256);
+				} else
+				{
+					if((demo_unlag_value/50) + 2 > UNLAG_SNAPSHOT_BACKUP)
+						demo_unlag_value = (UNLAG_SNAPSHOT_BACKUP - 2) * 50;
+
+					if(snaps_stored > (demo_unlag_value/50) + 2)
+						sprintf(str2, " - unlag:%d", demo_unlag_value);
+					else
+						sprintf(str2, " - unlag:%d (buffering...)", demo_unlag_value);
+				}
+				Q_strcat(str, 256, str2);
+
+				sprintf(str2, " - svtime: %d", cl.serverTime);
+				Q_strcat(str, 256, str2);
+
+				if(demo_follow_enabled && demo_follow_validview && (demo_follow_clientnum != -1))
+					sprintf(str2, " ping:???");
+				else
+					sprintf(str2, " ping:%03d", last_stored_snap.ping);
+				Q_strcat(str, 256, str2);
+
+				if(demo_follow_enabled)
+				{
+					if(!demo_follow_attacker) {
+						if(demo_follow_validview) sprintf(str2, " - ^3follow player#%d^7", demo_follow_clientnum);
+						else sprintf(str2, " - follow #%d (n/a)", demo_follow_clientnum);
+					} else	{
+						if(demo_follow_clientnum != -1)	{
+							if(demo_follow_validview) sprintf(str2, " - followall: #%d", demo_follow_clientnum);
+							else Q_strncpyz(str2, " - followall: n/a", 256);
+						} else {
+							Q_strncpyz(str2, " - followall: n/a", 256);
+						}
+					}
+					Q_strcat(str, 256, str2);
+				}
+			} else
+			{
+				float x, y, w, h;
+				int width;
+
+				width = SMALLCHAR_WIDTH;
+				Q_strncpyz(str2, str, 256);
+				Q_CleanStr(str2);
+				width *= (strlen(str2) + 2);
+
+				color[0] = color[1] = color[2] = 0;
+				color[3] = 0.5;
+
+				x = 0;
+				y = 12+1 - SMALLCHAR_HEIGHT*0.5;
+				w = width;
+				h = SMALLCHAR_HEIGHT*2;
+
+				re.SetColor( color );
+				re.DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
+				re.SetColor( NULL );
+			}
+
+			//----------------- draws infotext
+			Q_strncpyz(str2, str, 256);
+			Q_CleanStr(str2);
+			textwidth = SMALLCHAR_WIDTH * (strlen(str2) + 2);
+			if(textwidth >= cls.glconfig.vidWidth) Q_strncpyz(str, &str[pos2], 256);
+
+			//----------------- demoname and map name
+			if(cls.state == CA_ACTIVE)
+			{
+				int width;
+				float perc;
+				const char * info;
+
+				info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
+
+				Q_strncpyz(last_demo_played, clc.demoName, 256);
+				Q_strncpyz(last_mapname, Info_ValueForKey(info, "mapname"), 256);
+				Q_strncpyz(last_servername, Info_ValueForKey(info, "sv_hostname"), 256);
+				Q_strcat(last_servername, 256, "^7, ");
+				Q_strcat(last_servername, 256, Info_ValueForKey(info, "gamename"));
+				Q_strcat(last_servername, 256, " ");
+				Q_strcat(last_servername, 256, Info_ValueForKey(info, "mod_version"));
+
+				if(demo_demooffset > demo_total_length) demo_total_length = FS_filelength(clc.demofile);
+				perc = ((float)demo_demooffset*100) / (float)demo_total_length;
+				sprintf(str2, "    demo:^3%s^7(%d%%) map:^3%s",
+					last_demo_played, (int)perc, last_mapname);
+				width = SMALLCHAR_WIDTH * (strlen(str2) + 1);
+
+				if(width + textwidth >= cls.glconfig.vidWidth)
+				{
+					float x, y, w, h;
+
+					w = width / xscale;
+					h = (SMALLCHAR_HEIGHT*1.5) / yscale;
+					x = SCREEN_WIDTH-w;
+					y = (SMALLCHAR_HEIGHT + 15) / yscale;
+
+					color[0] = color[1] = color[2] = 0;
+					color[3] = 0.5;
+					SCR_FillRect( x, y, w, h, color );
+
+					w = width - SMALLCHAR_WIDTH;
+					x = SCREEN_WIDTH * xscale-w;
+					y = (SMALLCHAR_HEIGHT + 15);
+
+					color[0] = color[1] = color[2] = 0;
+					color[3] = 0.8;
+					SCR_DrawSmallStringExt(x+2,y+2,str2,color,qtrue);
+
+					color[0] = color[1] = color[2] = 1.0;
+					color[3] = 1.0;
+					SCR_DrawSmallStringExt(x,y,str2,color,qfalse);
+				} else {
+					Q_strcat(str, 256, str2);
+				}
+			}
+
+			color[0] = color[1] = color[2] = 0;
+			color[3] = 0.8;
+			SCR_DrawSmallStringExt(SMALLCHAR_WIDTH+3,12+3,str,color,qtrue);
+
+			color[0] = color[1] = color[2] = 1.0;
+			color[3] = 1.0;
+			SCR_DrawSmallStringExt(SMALLCHAR_WIDTH+1,12+1,str,color,qfalse);
+		}
+
+		if((cls.state == CA_ACTIVE) && demo_warpometer_enabled && (snaps_stored > 2))
+		{
+			Draw_Warpometer();
+		}
+	}
+	//----------------------
+
 	VM_Debug( 0 );
 }
 
 CL_SetCGameTime
 ==================
 */
-void CL_SetCGameTime( void ) {
+
+void CL_SetCGameTime( void )
+{
 	// getting a valid frame message ends the connection process
 	if ( cls.state != CA_ACTIVE ) {
 		if ( cls.state != CA_PRIMED ) {
 				clc.firstDemoFrameSkipped = qtrue;
 				return;
 			}
+
+			// SunLight - demo seek
+			if(clc.demoplaying) Check_DemoSeek();
\n		// SunLight - demo seek
		if(clc.demoplaying) Check_DemoSeek();

 			CL_ReadDemoMessage();
 		}
 		if ( cl.newSnapshots ) {
 			// do nothing?
 			CL_FirstSnapshot();
 		} else {
-			Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
+			Com_Printf ("cl.snap.serverTime < cl.oldFrameServerTime\n" ); // SunLight
 		}
 	}
 	cl.oldFrameServerTime = cl.snap.serverTime;
 	while ( cl.serverTime >= cl.snap.serverTime ) {
 		// feed another messag, which should change
 		// the contents of cl.snap
+
+		// SunLight - demo seek
+		if(clc.demoplaying) Check_DemoSeek();
+
\n		// SunLight - demo seek
		if(clc.demoplaying) Check_DemoSeek();

 		CL_ReadDemoMessage();
+
 		if ( cls.state != CA_ACTIVE ) {
 			Cvar_Set( "timescale", "1" );
 			return;     // end of demo
 // console.c
 
 #include "client.h"
-
+#include "sun_include.h"
+#include "etrewind_version.h"
 
 int g_console_field_width = 78;
 
 
 	re.SetColor( g_color_table[ColorIndex( COLNSOLE_COLOR )] );
 
-	i = strlen( Q3_VERSION );
+	{
+		char str[256];
 
-	for ( x = 0 ; x < i ; x++ ) {
+		strcpy(str, etr_ver);
+		Q_CleanStr(str);
+		strcat(str, " V");
+		strcat(str, ETREWIND_VERSION);
+		strcat(str, ", ");
+		strcat(str, Q3_VERSION);
 
-		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH,
+		i = strlen( str);
 
-						   ( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2 ) ), Q3_VERSION[x] );
+		for ( x = 0 ; x < i ; x++ )
+		{
+			SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH,
+				( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2 ) ), str[x] );
 
 	}
-
+	}
 
 	// draw the text
 	con.vislines = lines;
 void CL_Vid_Restart_f( void );
 void CL_Snd_Restart_f( void );
 void CL_NextDemo( void );
-void CL_ReadDemoMessage( void );
+int CL_ReadDemoMessage( void ); // SunLight - was void
 
 void CL_InitDownloads( void );
 void CL_NextDownload( void );
 */
 
 #include "client.h"
+#include "sun_include.h"
 
 /*
 
 // fretn
 qboolean consoleButtonWasPressed = qfalse;
 
+//SunLight
+int old_r_shownormals = 5;
+
 void CL_KeyEvent( int key, qboolean down, unsigned time ) {
 	char    *kb;
 	char cmd[1024];
 	if ( !key ) {
 		return;
 	}
+	if (key < 0 || key >= MAX_KEYS)
+	{
+		return; // -- SunLight fixes bug.
+	}
 
 	switch ( key ) {
 	case K_KP_PGUP:
 		}
 	}
 
-#ifdef __linux__
 	if ( key == K_ENTER ) {
 		if ( down ) {
 			if ( keys[K_ALT].down ) {
 			}
 		}
 	}
-#endif
+
+	// SunLight -- toggle shownormals, realview and other stuff
+	if ( clc.demoplaying )
+	{
+		if ( !( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) ) )
+		{
+			if ( key == 's')
+			{
+				if ( down )
+				{
+					int current_normals;
+
+					current_normals = Cvar_VariableValue("r_shownormals");
+
+					if(current_normals) Cvar_Set( "r_shownormals", "0" );
+					else Cvar_Set( "r_shownormals", va( "%d", old_r_shownormals) );
+
+					old_r_shownormals = current_normals;
+				}
+			}
+			else if ( key == 'r')
+			{
+				if ( down ) no_damage_kick ^= 1;
+			}
+			else if ( key == 'n')
+			{
+				if ( down ) demo_playernames = (demo_playernames >= 1) ? 0 : 1;
+			} else if ( key == K_SHIFT)
+			{
+				if ( down ) demo_force_zoom_shift = 1;
+				else  demo_force_zoom_shift = 0;
+			}  else if ( key == K_CTRL)
+			{
+				if ( down ) {
+					float val = Cvar_VariableValue("timescale");
+					if (val != demo_force_timescale) demo_old_timescale = val;
+					Cvar_Set("timescale", va( "%f", demo_force_timescale ));
+				} else	{
+					Cvar_Set("timescale", va( "%f", demo_old_timescale ));
+				}
+			}  else if ( key == K_ALT)
+			{
+				if ( down )	{
+					float val = Cvar_VariableValue("timescale");
+					if (val != demo_force_timescale_alt) demo_old_timescale_alt = val;
+					Cvar_Set("timescale", va( "%f", demo_force_timescale_alt ));
+				} else {
+					Cvar_Set("timescale", va( "%f", demo_old_timescale_alt ));
+				}
+			} else if ( key == 'p')
+			{
+				if ( down ) {
+					float val = Cvar_VariableValue("timescale");
+					if (val == 0) {// unpause
+						if (demo_old_timescale != 0)
+							Cvar_Set("timescale", va( "%f", demo_old_timescale ));
+						else
+							Cvar_Set("timescale", "1");
+					} else { // pause
+						demo_old_timescale = val;
+						Cvar_Set("timescale", "0");
+						VM_Call( cgvm, CG_EVENT_HANDLING, CGAME_EVENT_DEMO );
+					}
+				}
+			} else if ( key == 'a')
+			{ // Automatic unlag, realview and shownormals
+				if ( down ) {
+					int team;
+
+					demo_unlag_value = -1; // auto unlag
+					no_damage_kick = 1;
+
+					// auto shownormals
+					old_r_shownormals = Cvar_VariableValue("r_shownormals");
+					team = get_player_team(last_stored_snap.ps.clientNum);
+					if(team == TEAM_AXIS) Cvar_Set( "r_shownormals", "7" );
+					else if(team == TEAM_ALLIES) Cvar_Set( "r_shownormals", "6" );
+				}
+			}
+
+
+		}
+	}
 
 	// console key is hardcoded, so the user can never unbind it
 	if ( key == '`' || key == '~' ) {
 // cl_main.c  -- client main loop
 
 #include "client.h"
+#include "sun_include.h"
+#include "etrewind_version.h"
 #include <limits.h>
 
 #include "snd_local.h" // fretn
 
 }
 
+
+//-----------------------
+// SunLight -- extra stuff
+
+char etr_ver[] = "ET Demoview by SunLight";
+char last_demo_played[256];
+char last_mapname[256];
+char last_servername[256];
+
+void demo_show_lastdemo(void)
+{
+	Com_Printf(etr_ver);
+	Com_Printf("\n");
+	Com_Printf("Last demo played: %s\n", last_demo_played);
+	Com_Printf("map: %s\n", last_mapname);
+	Com_Printf("server: %s\n", last_servername);
+}
+
+int demo_playernames = 0;
+
+void demo_toggle_playernames(void)
+{
+	if ( Cmd_Argc() != 2 ) {
+		Com_Printf(etr_ver);
+		Com_Printf("\n");
+		Com_Printf("^7usage: demo_playernames [0 - 3]\n");
+		Com_Printf("^70 = off, 1 = on, 2 = enemies, 3 = team mates\n");
+		return;
+	}
+	if (!clc.demoplaying)
+	{
+		Com_Printf("^1you are not playing a demo...\n");
+		return;
+	}
+
+	demo_playernames = atoi(Cmd_Argv(1));
+}
+
+// timescale
+float demo_force_timescale = 0.5;
+float demo_old_timescale = 1;
+
+void demo_ctrl_timescale(void)
+{
+	if ( Cmd_Argc() < 2 ) {
+		Com_Printf(etr_ver);
+		Com_Printf("\n\n");
+		Com_Printf("^7usage: demo_ctrl_timescale [value]\n");
+		Com_Printf("\n^7current value: %.1f\n", demo_force_timescale);
+		return;
+	}
+
+	demo_force_timescale = atof(Cmd_Argv(1));
+	if (demo_force_timescale < 0.1) demo_force_timescale = 0.1;
+	else if (demo_force_timescale > 20) demo_force_timescale = 20;
+
+	Com_Printf("\n^7hold ctrl to change timescale to %.1f\n", demo_force_timescale);
+}
+
+float demo_force_timescale_alt = 5;
+float demo_old_timescale_alt = 1;
+
+void demo_alt_timescale(void)
+{
+	if ( Cmd_Argc() < 2 ) {
+		Com_Printf(etr_ver);
+		Com_Printf("\n\n");
+		Com_Printf("^7usage: demo_alt_timescale [value]\n");
+		Com_Printf("\n^7current value: %.1f\n", demo_force_timescale_alt);
+		return;
+	}
+
+	demo_force_timescale_alt = atof(Cmd_Argv(1));
+	if (demo_force_timescale_alt < 0.1) demo_force_timescale_alt = 0.1;
+	else if (demo_force_timescale_alt > 20) demo_force_timescale_alt = 20;
+
+	Com_Printf("\n^7hold alt to change timescale to %.1f\n", demo_force_timescale_alt);
+}
+
+// zoom
+int demo_shift_fov = 40;
+
+void demo_shift_zoomfov(void)
+{
+	if ( Cmd_Argc() < 2 ) {
+		Com_Printf(etr_ver);
+		Com_Printf("\n\n");
+		Com_Printf("^7usage: demo_shift_zoomfov [fov]\n");
+		Com_Printf("^7fov is in degrees like cg_fov, the smaller the fov the higher the zoom\n");
+		Com_Printf("\n^7current value: %d\n", demo_shift_fov);
+		return;
+	}
+
+	demo_shift_fov = atoi(Cmd_Argv(1));

/*
 * @brief CL_GetTag
 * @param[in] clientNum
 * @param[in] tagname
 * @param[in] orientation
 * @return
 *
 */
qboolean CL_GetTag(int clientNum, char *tagname, orientation_t *orientation)
{
	if (!cgvm)
	{
		return qfalse;
	}

	return VM_Call(cgvm, CG_GET_TAG, clientNum, tagname, orientation);
}
