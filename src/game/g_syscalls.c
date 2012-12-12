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
 * @file g_syscalls.c
 * @brief this file is only included when building a dll, g_syscalls.asm is
 * included instead when building a qvm
 */

#include "g_local.h"

static intptr_t (QDECL *syscall)(intptr_t arg, ...) = (intptr_t (QDECL *)(intptr_t, ...)) - 1;

Q_EXPORT void dllEntry(intptr_t (QDECL *syscallptr)(intptr_t arg, ...))
{
	syscall = syscallptr;
}

int PASSFLOAT(float x)
{
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void trap_Printf(const char *fmt)
{
	syscall(G_PRINT, fmt);
}

void trap_Error(const char *fmt)
{
	syscall(G_ERROR, fmt);
	// shut up GCC warning about returning functions, because we know better
	exit(1);
}

int trap_Milliseconds(void)
{
	return syscall(G_MILLISECONDS);
}
int trap_Argc(void)
{
	return syscall(G_ARGC);
}

void trap_Argv(int n, char *buffer, int bufferLength)
{
	syscall(G_ARGV, n, buffer, bufferLength);
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	return syscall(G_FS_FOPEN_FILE, qpath, f, mode);
}

void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
	syscall(G_FS_READ, buffer, len, f);
}

int trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
	return syscall(G_FS_WRITE, buffer, len, f);
}

int trap_FS_Rename(const char *from, const char *to)
{
	return syscall(G_FS_RENAME, from, to);
}

void trap_FS_FCloseFile(fileHandle_t f)
{
	syscall(G_FS_FCLOSE_FILE, f);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return syscall(G_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

void trap_SendConsoleCommand(int exec_when, const char *text)
{
	syscall(G_SEND_CONSOLE_COMMAND, exec_when, text);
}

void trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags)
{
	syscall(G_CVAR_REGISTER, cvar, var_name, value, flags);
}

void trap_Cvar_Update(vmCvar_t *cvar)
{
	syscall(G_CVAR_UPDATE, cvar);
}

void trap_Cvar_Set(const char *var_name, const char *value)
{
	syscall(G_CVAR_SET, var_name, value);
}

int trap_Cvar_VariableIntegerValue(const char *var_name)
{
	return syscall(G_CVAR_VARIABLE_INTEGER_VALUE, var_name);
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	syscall(G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}

void trap_Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
	syscall(G_CVAR_LATCHEDVARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}

void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                         playerState_t *clients, int sizeofGClient)
{
	syscall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient);
}

void trap_DropClient(int clientNum, const char *reason, int length)
{
	syscall(G_DROP_CLIENT, clientNum, reason, length);
}

void trap_SendServerCommand(int clientNum, const char *text)
{
	// commands over 1022 chars will crash the client engine upon receipt,
	// so ignore them
	if (strlen(text) > 1022)
	{
		G_LogPrintf("%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAMEVERSION, clientNum);
		G_LogPrintf("%s: text [%s.950s]... truncated\n", GAMEVERSION, text);
		return;
	}
	syscall(G_SEND_SERVER_COMMAND, clientNum, text);
}

void trap_SetConfigstring(int num, const char *string)
{
	syscall(G_SET_CONFIGSTRING, num, string);
}

void trap_GetConfigstring(int num, char *buffer, int bufferSize)
{
	syscall(G_GET_CONFIGSTRING, num, buffer, bufferSize);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize)
{
	syscall(G_GET_USERINFO, num, buffer, bufferSize);
}

void trap_SetUserinfo(int num, const char *buffer)
{
	syscall(G_SET_USERINFO, num, buffer);
}

void trap_GetServerinfo(char *buffer, int bufferSize)
{
	syscall(G_GET_SERVERINFO, buffer, bufferSize);
}

void trap_SetBrushModel(gentity_t *ent, const char *name)
{
	syscall(G_SET_BRUSH_MODEL, ent, name);
}

void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

void trap_TraceNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACE, results, start, mins, maxs, end, -2, contentmask);
}

void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask)
{
	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, -2, contentmask);
}

int trap_PointContents(const vec3_t point, int passEntityNum)
{
	return syscall(G_POINT_CONTENTS, point, passEntityNum);
}


qboolean trap_InPVS(const vec3_t p1, const vec3_t p2)
{
	return syscall(G_IN_PVS, p1, p2);
}

qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	return syscall(G_IN_PVS_IGNORE_PORTALS, p1, p2);
}

void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open)
{
	syscall(G_ADJUST_AREA_PORTAL_STATE, ent, open);
}

qboolean trap_AreasConnected(int area1, int area2)
{
	return syscall(G_AREAS_CONNECTED, area1, area2);
}

void trap_LinkEntity(gentity_t *ent)
{
	syscall(G_LINKENTITY, ent);
}

void trap_UnlinkEntity(gentity_t *ent)
{
	syscall(G_UNLINKENTITY, ent);
}


int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount)
{
	return syscall(G_ENTITIES_IN_BOX, mins, maxs, list, maxcount);
}

qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return syscall(G_ENTITY_CONTACT, mins, maxs, ent);
}

qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent)
{
	return syscall(G_ENTITY_CONTACTCAPSULE, mins, maxs, ent);
}

// #ifdef FEATURE_OMNIBOT FIXME: precompiler macros for engine ?
int trap_BotAllocateClient(int clientNum)
{
	return syscall(G_BOT_ALLOCATE_CLIENT, clientNum);
}

int trap_BotGetServerCommand(int clientNum, char *message, int size)
{
	return syscall(BOTLIB_GET_CONSOLE_MESSAGE, clientNum, message, size);
}

void trap_BotUserCommand(int clientNum, usercmd_t *ucmd)
{
	syscall(BOTLIB_USER_COMMAND, clientNum, ucmd);
}

void trap_EA_Command(int client, char *command)
{
	syscall(BOTLIB_EA_COMMAND, client, command);
}
// #endif

int trap_GetSoundLength(sfxHandle_t sfxHandle)
{
	return syscall(G_GET_SOUND_LENGTH, sfxHandle);
}

sfxHandle_t trap_RegisterSound(const char *sample, qboolean compressed)
{
	return syscall(G_REGISTERSOUND, sample, compressed);
}

#ifdef DEBUG
//#define FAKELAG
#ifdef FAKELAG
#define MAX_USERCMD_BACKUP  256
#define MAX_USERCMD_MASK    (MAX_USERCMD_BACKUP - 1)

static usercmd_t cmds[MAX_CLIENTS][MAX_USERCMD_BACKUP];
static int       cmdNumber[MAX_CLIENTS];
#endif // FAKELAG
#endif // DEBUG

void trap_GetUsercmd(int clientNum, usercmd_t *cmd)
{
	syscall(G_GET_USERCMD, clientNum, cmd);

#ifdef FAKELAG
	{
		char s[MAX_STRING_CHARS];
		int  fakeLag;

		trap_Cvar_VariableStringBuffer("g_fakelag", s, sizeof(s));
		fakeLag = atoi(s);
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
			memcpy(&cmds[clientNum][cmdNumber[clientNum] & MAX_USERCMD_MASK], cmd, sizeof(usercmd_t));

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

qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
	return syscall(G_GET_ENTITY_TOKEN, buffer, bufferSize);
}

int trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points)
{
	return syscall(G_DEBUG_POLYGON_CREATE, color, numPoints, points);
}

void trap_DebugPolygonDelete(int id)
{
	syscall(G_DEBUG_POLYGON_DELETE, id);
}

int trap_RealTime(qtime_t *qtime)
{
	return syscall(G_REAL_TIME, qtime);
}

void trap_SnapVector(float *v)
{
	syscall(G_SNAPVECTOR, v);
	return;
}

qboolean trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t *or)
{
	return syscall(G_GETTAG, clientNum, tagFileNumber, tagName, or);
}

qboolean trap_LoadTag(const char *filename)
{
	return syscall(G_REGISTERTAG, filename);
}

int trap_PC_AddGlobalDefine(char *define)
{
	return 0;
}

int trap_PC_LoadSource(const char *filename)
{
	return syscall(BOTLIB_PC_LOAD_SOURCE, filename);
}

int trap_PC_FreeSource(int handle)
{
	return syscall(BOTLIB_PC_FREE_SOURCE, handle);
}

int trap_PC_ReadToken(int handle, pc_token_t *pc_token)
{
	return syscall(BOTLIB_PC_READ_TOKEN, handle, pc_token);
}

int trap_PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	return syscall(BOTLIB_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

int trap_PC_UnReadToken(int handle)
{
	return syscall(BOTLIB_PC_UNREAD_TOKEN, handle);
}

void trap_SendMessage(int clientNum, char *buf, int buflen)
{
	syscall(G_SENDMESSAGE, clientNum, buf, buflen);
}

messageStatus_t trap_MessageStatus(int clientNum)
{
	return syscall(G_MESSAGESTATUS, clientNum);
}
