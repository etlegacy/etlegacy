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
 * @file g_syscalls.c
 */

#include "g_local.h"

static intptr_t(QDECL * syscall)(intptr_t arg, ...) = (intptr_t(QDECL *)(intptr_t, ...)) - 1;

/**
 * @brief dllEntry
 */
Q_EXPORT void dllEntry(intptr_t(QDECL * syscallptr)(intptr_t arg, ...))
{
	syscall = syscallptr;
}

/**
 * @brief PASSFLOAT
 * @param[in] x
 * @return
 */
int PASSFLOAT(float x)
{
	floatint_t fi;

	fi.f = x;
	return fi.i;
}

/**
 * @brief trap_Printf
 * @param[in] fmt
 */
void trap_Printf(const char *fmt)
{
	SystemCall(G_PRINT, fmt);
}

/**
 * @brief trap_Error
 * @param[in] fmt
 */
void trap_Error(const char *fmt)
{
	SystemCall(G_ERROR, fmt);
	// shut up GCC warning about returning functions, because we know better
	exit(1);
}

/**
 * @brief trap_Milliseconds
 * @return
 */
int trap_Milliseconds(void)
{
	return SystemCall(G_MILLISECONDS);
}

/**
 * @brief trap_Argc
 * @return
 */
int trap_Argc(void)
{
	return SystemCall(G_ARGC);
}

/**
 * @brief trap_Argv
 * @param[in] n
 * @param[out] buffer
 * @param[in] bufferLength
 */
void trap_Argv(int n, char *buffer, int bufferLength)
{
	SystemCall(G_ARGV, n, buffer, bufferLength);
}

/**
 * @brief trap_FS_FOpenFile
 * @param[in] qpath
 * @param[in] f
 * @param[in] mode
 * @return
 */
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	return SystemCall(G_FS_FOPEN_FILE, qpath, f, mode);
}

/**
 * @brief trap_FS_Read
 * @param[out] buffer
 * @param[in] len
 * @param[in] f
 */
void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	SystemCall(G_FS_READ, buffer, len, f);
}

/**
 * @brief trap_FS_Write
 * @param[in] buffer
 * @param[in] len
 * @param[in] f
 * @return
 */
int trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	return SystemCall(G_FS_WRITE, buffer, len, f);
}

/**
 * @brief trap_FS_Rename
 * @param[in] from
 * @param[in] to
 * @return
 */
int trap_FS_Rename(const char *from, const char *to)
{
	return SystemCall(G_FS_RENAME, from, to);
}

/**
 * @brief trap_FS_FCloseFile
 * @param[in] f
 */
void trap_FS_FCloseFile(fileHandle_t f)
{
	SystemCall(G_FS_FCLOSE_FILE, f);
}

/**
 * @brief trap_FS_GetFileList
 * @param[in] path
 * @param[in] extension
 * @param[out] listbuf
 * @param[in] bufsize
 * @return
 */
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return SystemCall(G_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

/**
 * @brief trap_SendConsoleCommand
 * @param[in] exec_when
 * @param[in] text
 */
void trap_SendConsoleCommand(int exec_when, const char *text)
{
	SystemCall(G_SEND_CONSOLE_COMMAND, exec_when, text);
}

/**
 * @brief trap_Cvar_Register
 * @param[in] vmCvar
 * @param[in] varName
 * @param[in] defaultValue
 * @param[in] flags
 */
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags)
{
	SystemCall(G_CVAR_REGISTER, vmCvar, varName, defaultValue, flags);
}

/**
 * @brief trap_Cvar_Update
 * @param[in] vmCvar
 */
void trap_Cvar_Update(vmCvar_t *vmCvar)
{
	SystemCall(G_CVAR_UPDATE, vmCvar);
}

/**
 * @brief trap_Cvar_Set
 * @param[in] varName
 * @param[in] value
 */
void trap_Cvar_Set(const char *varName, const char *value)
{
	SystemCall(G_CVAR_SET, varName, value);
}

/**
 * @brief trap_Cvar_VariableIntegerValue
 * @param[in] varName
 * @return
 */
int trap_Cvar_VariableIntegerValue(const char *varName)
{
	return SystemCall(G_CVAR_VARIABLE_INTEGER_VALUE, varName);
}

/**
 * @brief trap_Cvar_VariableStringBuffer
 * @param[in] varName
 * @param[out] buffer
 * @param[in] bufsize
 */
void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize)
{
	SystemCall(G_CVAR_VARIABLE_STRING_BUFFER, varName, buffer, bufsize);
}

/**
 * @brief trap_Cvar_LatchedVariableStringBuffer
 * @param[in] varName
 * @param[out] buffer
 * @param[in] bufsize
 */
void trap_Cvar_LatchedVariableStringBuffer(const char *varName, char *buffer, int bufsize)
{
	SystemCall(G_CVAR_LATCHEDVARIABLESTRINGBUFFER, varName, buffer, bufsize);
}

/**
 * @brief trap_LocateGameData
 * @param[in] gEnts
 * @param[in] numGEntities
 * @param[in] sizeofGEntity_t
 * @param[in] gameClients
 * @param[in] sizeofGClient
 */
void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                         playerState_t *gameClients, int sizeofGClient)
{
	SystemCall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, gameClients, sizeofGClient);
}

/**
 * @brief trap_DropClient
 * @param[in] clientNum
 * @param[in] reason
 * @param[in] length
 */
void trap_DropClient(int clientNum, const char *reason, int length)
{
	SystemCall(G_DROP_CLIENT, clientNum, reason, length);
}

/**
 * @brief trap_SendServerCommand
 * @param[in] clientNum
 * @param[in] text
 */
void trap_SendServerCommand(int clientNum, const char *text)
{
	// commands over 1022 chars will crash the client engine upon receipt,
	// so ignore them
	if (strlen(text) > 1022)
	{
		G_LogPrintf("%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", MODNAME, clientNum);
		G_LogPrintf("%s: text [%s.950s]... truncated\n", MODNAME, text);
		return;
	}
	SystemCall(G_SEND_SERVER_COMMAND, clientNum, text);
}

/**
 * @brief trap_SetConfigstring
 * @param[in] num
 * @param[in] string
 */
void trap_SetConfigstring(int num, const char *string)
{
	SystemCall(G_SET_CONFIGSTRING, num, string);
}

/**
 * @brief trap_GetConfigstring
 * @param[in] num
 * @param[out] buffer
 * @param[in] bufferSize
 */
void trap_GetConfigstring(int num, char *buffer, int bufferSize)
{
	SystemCall(G_GET_CONFIGSTRING, num, buffer, bufferSize);
}

/**
 * @brief trap_GetUserinfo
 * @param[in] num
 * @param[out] buffer
 * @param[in] bufferSize
 */
void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
	SystemCall(G_GET_USERINFO, num, buffer, bufferSize);
}

/**
 * @brief trap_SetUserinfo
 * @param[in] num
 * @param[in] buffer
 */
void trap_SetUserinfo(int num, const char *buffer)
{
	SystemCall(G_SET_USERINFO, num, buffer);
}

/**
 * @brief trap_GetServerinfo
 * @param[out] buffer
 * @param[in] bufferSize
 */
void trap_GetServerinfo(char *buffer, int bufferSize)
{
	SystemCall(G_GET_SERVERINFO, buffer, bufferSize);
}

/**
 * @brief trap_SetBrushModel
 * @param[in] ent
 * @param[in] name
 */
void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	SystemCall(G_SET_BRUSH_MODEL, ent, name);
}

/**
 * @brief trap_Trace
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param[in] passEntityNum
 * @param[in] contentmask
 */
void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	SystemCall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

/**
 * @brief trap_TraceNoEnts
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param passEntityNum - unused
 * @param[in] contentmask
 */
void trap_TraceNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	SystemCall(G_TRACE, results, start, mins, maxs, end, -2, contentmask);
}

/**
 * @brief trap_TraceCapsule
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param[in] passEntityNum
 * @param[in] contentmask
 */
void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	SystemCall(G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

/**
 * @brief trap_TraceCapsuleNoEnts
 * @param[out] results
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param passEntityNum - unused
 * @param[in] contentmask
 */
void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	SystemCall(G_TRACECAPSULE, results, start, mins, maxs, end, -2, contentmask);
}

/**
 * @brief trap_PointContents
 * @param[in] point
 * @param[in] passEntityNum
 * @return
 */
int trap_PointContents(const vec3_t point, int passEntityNum)
{
	return SystemCall(G_POINT_CONTENTS, point, passEntityNum);
}

/**
 * @brief trap_InPVS
 * @param[in] p1
 * @param[in] p2
 * @return
 */
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	return (qboolean)(SystemCall(G_IN_PVS, p1, p2));
}

/**
 * @brief trap_InPVSIgnorePortals
 * @param[in] p1
 * @param[in] p2
 * @return
 */
qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	return (qboolean)(SystemCall(G_IN_PVS_IGNORE_PORTALS, p1, p2));
}

/**
 * @brief trap_AdjustAreaPortalState
 * @param[in] ent
 * @param[in] open
 */
void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open)
{
	SystemCall(G_ADJUST_AREA_PORTAL_STATE, ent, open);
}

/**
 * @brief trap_AreasConnected
 * @param[in] area1
 * @param[in] area2
 * @return
 */
qboolean trap_AreasConnected(int area1, int area2)
{
	return (qboolean)(SystemCall(G_AREAS_CONNECTED, area1, area2));
}

/**
 * @brief trap_LinkEntity
 * @param[in] ent
 */
void trap_LinkEntity(gentity_t *ent)
{
	SystemCall(G_LINKENTITY, ent);
}

/**
 * @brief trap_UnlinkEntity
 * @param[in] ent
 */
void trap_UnlinkEntity(gentity_t *ent)
{
	SystemCall(G_UNLINKENTITY, ent);
}

/**
 * @brief trap_EntitiesInBox
 * @param[in] mins
 * @param[in] maxs
 * @param[out] list
 * @param[in] maxCount
 * @return
 */
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxCount)
{
	return SystemCall(G_ENTITIES_IN_BOX, mins, maxs, list, maxCount);
}

/**
 * @brief trap_EntityContact
 * @param[in] mins
 * @param[in] maxs
 * @param[in] ent
 * @return
 */
qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return (qboolean)(SystemCall(G_ENTITY_CONTACT, mins, maxs, ent));
}

qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return (qboolean)(SystemCall(G_ENTITY_CONTACTCAPSULE, mins, maxs, ent));
}

//#ifdef FEATURE_OMNIBOT FIXME: precompiler macros for engine ?

/**
 * @brief trap_BotAllocateClient
 * @param[in] clientNum
 * @return
 */
int trap_BotAllocateClient(int clientNum)
{
	return SystemCall(G_BOT_ALLOCATE_CLIENT, clientNum);
}

/**
 * @brief trap_BotGetServerCommand
 * @param[in] clientNum
 * @param[out] message
 * @param[in] size
 * @return
 */
int trap_BotGetServerCommand(int clientNum, char *message, int size)
{
	return SystemCall(BOTLIB_GET_CONSOLE_MESSAGE, clientNum, message, size);
}

/**
 * @brief trap_BotUserCommand
 * @param[in] clientNum
 * @param[in] ucmd
 */
void trap_BotUserCommand(int clientNum, usercmd_t *ucmd)
{
	SystemCall(BOTLIB_USER_COMMAND, clientNum, ucmd);
}

/**
 * @brief trap_EA_Command
 * @param[in] clientNum
 * @param[in] command
 */
void trap_EA_Command(int clientNum, char *command)
{
	SystemCall(BOTLIB_EA_COMMAND, clientNum, command);
}
// #endif

/**
 * @brief trap_GetSoundLength
 * @param[in] sfxHandle
 * @return
 */
int trap_GetSoundLength(sfxHandle_t sfxHandle)
{
	return SystemCall(G_GET_SOUND_LENGTH, sfxHandle);
}

/**
 * @brief trap_RegisterSound
 * @param[in] sample
 * @param[in] compressed
 * @return
 */
sfxHandle_t trap_RegisterSound(const char *sample, qboolean compressed)
{
	return SystemCall(G_REGISTERSOUND, sample, compressed);
}

#ifdef ETLEGACY_DEBUG
//#define FAKELAG
#ifdef FAKELAG
#define MAX_USERCMD_BACKUP  256
#define MAX_USERCMD_MASK    (MAX_USERCMD_BACKUP - 1)

static usercmd_t cmds[MAX_CLIENTS][MAX_USERCMD_BACKUP];
static int       cmdNumber[MAX_CLIENTS];
#endif // FAKELAG
#endif // DEBUG

/**
 * @brief trap_GetUsercmd
 * @param[in] clientNum
 * @param[in] cmd
 */
void trap_GetUsercmd(int clientNum, usercmd_t *cmd)
{
	SystemCall(G_GET_USERCMD, clientNum, cmd);

#ifdef FAKELAG
	{
		char s[MAX_STRING_CHARS];
		int  fakeLag;

		trap_Cvar_VariableStringBuffer("g_fakelag", s, sizeof(s));
		fakeLag = Q_atoi(s);
		if (fakeLag < 0)
		{
			fakeLag = 0;
		}

		if (fakeLag)
		{
			int i;
			int realcmdtime, thiscmdtime;

			// store our newest usercmd
			cmdNumber[clientNum]++;
			Com_Memcpy(&cmds[clientNum][cmdNumber[clientNum] & MAX_USERCMD_MASK], cmd, sizeof(usercmd_t));

			// find a usercmd that is fakeLag msec behind
			i           = cmdNumber[clientNum] & MAX_USERCMD_MASK;
			realcmdtime = cmds[clientNum][i].serverTime;
			i--;
			do
			{
				thiscmdtime = cmds[clientNum][i & MAX_USERCMD_MASK].serverTime;

				if (realcmdtime - thiscmdtime > fakeLag)
				{
					// found the right one
					cmd = &cmds[clientNum][i & MAX_USERCMD_MASK];
					return;
				}

				i--;
			}
			while ((i & MAX_USERCMD_MASK) != (cmdNumber[clientNum] & MAX_USERCMD_MASK));

			// didn't find a proper one, just use the oldest one we have
			cmd = &cmds[clientNum][(cmdNumber[clientNum] - 1) & MAX_USERCMD_MASK];
			return;
		}
	}
#endif // FAKELAG
}

/**
 * @brief trap_GetEntityToken
 * @param[out] buffer
 * @param[in] bufferSize
 * @return
 */
qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	return (qboolean)(SystemCall(G_GET_ENTITY_TOKEN, buffer, bufferSize));
}

/**
 * @brief trap_DebugPolygonCreate
 * @param[in] color
 * @param[in] numPoints
 * @param[in] points
 * @return
 */
int trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points)
{
	return SystemCall(G_DEBUG_POLYGON_CREATE, color, numPoints, points);
}

/**
 * @brief trap_DebugPolygonDelete
 * @param[in] id
 */
void trap_DebugPolygonDelete(int id)
{
	SystemCall(G_DEBUG_POLYGON_DELETE, id);
}

/**
 * @brief trap_RealTime
 * @param[in] qtime
 * @return
 */
int trap_RealTime(qtime_t *qtime)
{
	return SystemCall(G_REAL_TIME, qtime);
}

/**
 * @brief trap_SnapVector
 * @param[in] v
 */
void trap_SnapVector(float *v)
{
	SystemCall(G_SNAPVECTOR, v);
	return;
}

/**
 * @brief trap_GetTag
 *
 * @param[in] clientNum
 * @param[in] tagFileNumber
 * @param[out] tagName
 * @param[out] orientation
 */
qboolean trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t *orientation)
{
	return (qboolean)(SystemCall(G_GETTAG, clientNum, tagFileNumber, tagName, orientation));
}

/**
 * @brief trap_LoadTag
 * @param[in] filename
 * @return
 */
qboolean trap_LoadTag(const char *filename)
{
	return (qboolean)(SystemCall(G_REGISTERTAG, filename));
}

/**
 * @brief trap_PC_AddGlobalDefine
 * @param define - unused
 * @return
 *
 * @note Empty function return 0
 */
int trap_PC_AddGlobalDefine(char *define)
{
	return 0;
}

/**
 * @brief trap_PC_LoadSource
 * @param[in] filename
 * @return
 */
int trap_PC_LoadSource(const char *filename)
{
	return SystemCall(BOTLIB_PC_LOAD_SOURCE, filename);
}

/**
 * @brief trap_PC_FreeSource
 * @param[in] handle
 * @return
 */
int trap_PC_FreeSource(int handle)
{
	return SystemCall(BOTLIB_PC_FREE_SOURCE, handle);
}

/**
 * @brief trap_PC_ReadToken
 * @param[in] handle
 * @param[out] pc_token
 * @return
 */
int trap_PC_ReadToken(int handle, pc_token_t *pc_token)
{
	return SystemCall(BOTLIB_PC_READ_TOKEN, handle, pc_token);
}

/**
 * @brief trap_PC_SourceFileAndLine
 * @param[in] handle
 * @param[in] filename
 * @param[in] line
 * @return
 */
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	return SystemCall(BOTLIB_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

/**
 * @brief trap_PC_UnReadToken
 * @param[in] handle
 * @return
 */
int trap_PC_UnReadToken(int handle)
{
	return SystemCall(BOTLIB_PC_UNREAD_TOKEN, handle);
}

/**
 * @brief Sends binary messages. Since ETL 2.76 this is no longer void.
 *        See new return values
 *
 * @return 1 if message is in queue (will be sent) - 0 not sent
 *
 */
qboolean trap_SendMessage(int clientNum, char *buf, int buflen)
{
	return (qboolean)(SystemCall(G_SENDMESSAGE, clientNum, buf, buflen));
}

/**
 * @brief trap_MessageStatus
 * @param[in] clientNum
 * @return
 */
messageStatus_t trap_MessageStatus(int clientNum)
{
	return (messageStatus_t)(SystemCall(G_MESSAGESTATUS, clientNum));
}
