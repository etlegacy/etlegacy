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
 * @file sv_game.c
 * @brief Interface to the game dll
 */

#include "server.h"
#include "../botlib/botlib.h"

botlib_export_t *botlib_export;

/**
* @todo TODO: These functions must be used instead of pointer arithmetic, because
* the game allocates gentities with private information after the server shared part
*/

/**
 * @brief SV_NumForGentity
 * @param[in] ent
 * @return
 *
 * @note Unused
 */
int SV_NumForGentity(sharedEntity_t *ent)
{
	int num = ((byte *)ent - (byte *)sv.gentities) / sv.gentitySize;

	return num;
}

/**
 * @brief SV_GentityNum
 * @param[in] num
 * @return
 */
sharedEntity_t *SV_GentityNum(int num)
{
	sharedEntity_t *ent = ( sharedEntity_t * )((byte *)sv.gentities + sv.gentitySize * (num));

	return ent;
}

/**
 * @brief SV_GameClientNum
 * @param[in] num
 * @return
 */
playerState_t *SV_GameClientNum(int num)
{
	playerState_t *ps = ( playerState_t * )((byte *)sv.gameClients + sv.gameClientSize * (num));

	return ps;
}

/**
 * @brief SV_SvEntityForGentity
 * @param[in] gEnt
 * @return
 */
svEntity_t *SV_SvEntityForGentity(sharedEntity_t *gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
	}
	return &sv.svEntities[gEnt->s.number];
}

/**
 * @brief SV_GEntityForSvEntity
 * @param[in] svEnt
 * @return
 */
sharedEntity_t *SV_GEntityForSvEntity(svEntity_t *svEnt)
{
	int num = svEnt - sv.svEntities;

	return SV_GentityNum(num);
}

/**
 * @brief Sends a command string to a client
 * @param[in] clientNum
 * @param[in] text
 */
void SV_GameSendServerCommand(int clientNum, const char *text)
{
	// record the game server commands in demos
	if (sv.demoState == DS_RECORDING)
	{
		SV_DemoWriteGameCommand(clientNum, text);
	}
	else if (sv.demoState == DS_PLAYBACK)
	{
		SV_CheckLastCmd(text, qtrue);   // store the new game command, so when replaying a demo message, we can check for duplicates: maybe this message was already submitted (because of the events simulation, an event may trigger a message), and so we want to avoid those duplicates: if an event already triggered a message, no need to issue the one stored in the demo
	}

	if (clientNum == -1)
	{
		SV_SendServerCommand(NULL, "%s", text);
	}
	else
	{
		if (clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}

		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}

/**
 * @brief Disconnects the client with a message
 * @param[in] clientNum
 * @param[in] reason
 * @param[in] length
 */
void SV_GameDropClient(int clientNum, const char *reason, int length)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
	if (length)
	{
		SV_TempBanNetAddress(svs.clients[clientNum].netchan.remoteAddress, length);
	}
}

/**
 * @brief Sets mins and maxs for inline bmodels
 * @param[in,out] ent
 * @param[in] name
 */
void SV_SetBrushModel(sharedEntity_t *ent, const char *name)
{
	clipHandle_t h;
	vec3_t       mins, maxs;

	if (!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL for #%i", ent->s.number);
	}

	if (name[0] != '*')
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s of #%i isn't a brush model", name, ent->s.number);
	}

	ent->s.modelindex = Q_atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;       // we don't know exactly what is in the brushes

	SV_LinkEntity(ent);         // FIXME: remove
}

/**
 * @brief Also checks portalareas so that doors block sight
 * @param[in] p1
 * @param[in] p2
 * @return
 */
qboolean SV_inPVS(const vec3_t p1, const vec3_t p2)
{
	int  leafnum;
	int  cluster;
	int  area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1   = CM_LeafArea(leafnum);
	mask    = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2   = CM_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return qfalse;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		return qfalse;      // a door blocks sight
	}

	return qtrue;
}

/**
 * @brief Does NOT check portalareas
 * @param[in] p1
 * @param[in] p2
 * @return
 */
qboolean SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int  leafnum;
	int  cluster;
	byte *mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	mask    = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief SV_AdjustAreaPortalState
 * @param[in] ent
 * @param[in] open
 */
void SV_AdjustAreaPortalState(sharedEntity_t *ent, qboolean open)
{
	svEntity_t *svEnt = SV_SvEntityForGentity(ent);

	if (svEnt->areanum2 == -1)
	{
		return;
	}

	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}

/**
 * @brief SV_EntityContact
 * @param[in] mins
 * @param[in] maxs
 * @param[in] gEnt
 * @param[in] capsule
 * @return
 */
qboolean SV_EntityContact(const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, const qboolean capsule)
{
	const float  *origin = gEnt->r.currentOrigin;
	const float  *angles = gEnt->r.currentAngles;
	clipHandle_t ch;
	trace_t      trace;

	// check for exact collision
	ch = SV_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs,
	                       ch, -1, origin, angles, capsule);

	return trace.startsolid;
}

/**
 * @brief SV_GetServerinfo
 * @param[out] buffer
 * @param[in] bufferSize
 */
void SV_GetServerinfo(char *buffer, unsigned int bufferSize)
{
	if (bufferSize < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %u", bufferSize);
	}
	Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), bufferSize);
}

/**
 * @brief SV_LocateGameData
 * @param[in] gEnts
 * @param[in] numGEntities
 * @param[in] sizeofGEntity_t
 * @param[in] clients
 * @param[in] sizeofGameClient
 */
void SV_LocateGameData(sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                       playerState_t *clients, int sizeofGameClient)
{
	sv.gentities    = gEnts;
	sv.gentitySize  = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients    = clients;
	sv.gameClientSize = sizeofGameClient;
}

/**
 * @brief SV_GetUsercmd
 * @param[in] clientNum
 * @param[out] cmd
 */
void SV_GetUsercmd(int clientNum, usercmd_t *cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

/**
 * @brief SV_SendBinaryMessage
 * @param[in] cno
 * @param[out] buf
 * @param[in] buflen
 * @return 1 if message is in queue - 0 not sent (since 2.76)
 */
static int SV_SendBinaryMessage(int cno, char *buf, int buflen)
{
	if (cno < 0 || cno >= sv_maxclients->integer)
	{
		Com_Printf("SV_SendBinaryMessage: bad client %i - message not sent\n", cno);
		svs.clients[cno].binaryMessageLength = 0;
		return 0;
	}

	if (buflen < 0 || buflen > MAX_BINARY_MESSAGE)
	{
		Com_Printf("SV_SendBinaryMessage: bad buffer length %i - message not sent to client #%i\n", buflen, cno);
		svs.clients[cno].binaryMessageLength = 0;
		return 0;
	}

	svs.clients[cno].binaryMessageLength = buflen;
	Com_Memcpy(svs.clients[cno].binaryMessage, buf, buflen);
	return 1;
}

/**
 * @brief SV_BinaryMessageStatus
 * @param[in] cno
 * @return
 */
static int SV_BinaryMessageStatus(int cno)
{
	if (cno < 0 || cno >= sv_maxclients->integer)
	{
		return qfalse;
	}

	if (svs.clients[cno].binaryMessageLength == 0)
	{
		return MESSAGE_EMPTY;
	}

	if (svs.clients[cno].binaryMessageOverflowed)
	{
		return MESSAGE_WAITING_OVERFLOW;
	}

	return MESSAGE_WAITING;
}

/**
 * @brief SV_GameBinaryMessageReceived
 * @param[in] cno
 * @param[in] buf
 * @param[in] buflen
 * @param[in] commandTime
 */
void SV_GameBinaryMessageReceived(int cno, const char *buf, int buflen, int commandTime)
{
	VM_Call(gvm, GAME_MESSAGERECEIVED, cno, buf, buflen, commandTime);
}

//==============================================

/**
 * @brief FloatAsInt
 * @param[in] f
 * @return
 */
static int FloatAsInt(float f)
{
	floatint_t fi;

	fi.f = f;
	return fi.i;
}

/**
 * @brief Get engine value
 * @param[out] value buffer
 * @param[in] valueSize buffer size
 * @param[in] key to query
 * @return true if value for key is found
 */
static qboolean SV_GetValue(char *value, int valueSize, const char *key)
{
	return qfalse;
}

extern int S_RegisterSound(const char *name, qboolean compressed);
extern int S_GetSoundLength(sfxHandle_t sfxHandle);

/**
 * @brief The module is making a system call
 * @param[in] args
 * @return
 */
intptr_t SV_GameSystemCalls(intptr_t *args)
{
	switch (args[0])
	{
	case G_PRINT:
		Com_Printf("%s", (char *)VMA(1));
		return 0;
	case G_ERROR:
		Com_Error(ERR_DROP, "%s", (char *)VMA(1));
		return 0;
	case G_MILLISECONDS:
		return Sys_Milliseconds();
	case G_CVAR_REGISTER:
		Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
		return 0;
	case G_CVAR_UPDATE:
		Cvar_Update(VMA(1));
		return 0;
	case G_CVAR_SET:
		Cvar_SetSafe((const char *)VMA(1), (const char *)VMA(2));
		return 0;
	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue((const char *)VMA(1));
	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
		return 0;
	case G_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		Cvar_LatchedVariableStringBuffer(VMA(1), VMA(2), args[3]);
		return 0;
	case G_ARGC:
		return Cmd_Argc();
	case G_ARGV:
		Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
		return 0;
	case G_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText(args[1], VMA(2));
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
	case G_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case G_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case G_FS_RENAME:
		FS_Rename(VMA(1), VMA(2));
		return 0;
	case G_FS_FCLOSE_FILE:
		FS_FCloseFile(args[1]);
		return 0;
	case G_FS_GETFILELIST:
		return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData(VMA(1), args[2], args[3], VMA(4), args[5]);
		return 0;
	case G_DROP_CLIENT:
		SV_GameDropClient(args[1], VMA(2), args[3]);
		return 0;
	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], VMA(2));
		return 0;
	case G_LINKENTITY:
		SV_LinkEntity(VMA(1));
		return 0;
	case G_UNLINKENTITY:
		SV_UnlinkEntity(VMA(1));
		return 0;
	case G_ENTITIES_IN_BOX:
		return SV_AreaEntities(VMA(1), VMA(2), VMA(3), args[4]);
	case G_ENTITY_CONTACT:
		return SV_EntityContact(VMA(1), VMA(2), VMA(3), /* int capsule */ qfalse);
	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact(VMA(1), VMA(2), VMA(3), /* int capsule */ qtrue);
	case G_TRACE:
		SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /* int capsule */ qfalse);
		return 0;
	case G_TRACECAPSULE:
		SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /* int capsule */ qtrue);
		return 0;
	case G_POINT_CONTENTS:
		return SV_PointContents(VMA(1), args[2]);
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel(VMA(1), VMA(2));
		return 0;
	case G_IN_PVS:
		return SV_inPVS(VMA(1), VMA(2));
	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals(VMA(1), VMA(2));

	case G_SET_CONFIGSTRING:
		// Don't allow the game to overwrite demo configstrings (unless it modifies the normal spectator clients configstrings, this exception allows for player connecting during a demo playback to be correctly rendered, else they will get an empty configstring so no icon, no name, nothing...)
		// ATTENTION: sv.demoState check must be placed LAST! Else, it will short-circuit and prevent normal players configstrings from being set!
		if ((sv_democlients->integer > 0 && args[1] >= CS_PLAYERS + sv_democlients->integer && args[1] < CS_PLAYERS + sv_maxclients->integer) || sv.demoState != DS_PLAYBACK)
		{
			SV_SetConfigstring(args[1], VMA(2));
		}
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], VMA(2), args[3]);
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo(args[1], VMA(2));
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo(args[1], VMA(2), args[3]);
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo(VMA(1), args[2]);
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState(VMA(1), args[2]);
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected(args[1], args[2]);

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient(args[1]);

	case G_GET_USERCMD:
		SV_GetUsercmd(args[1], VMA(2));
		return 0;
	case G_GET_ENTITY_TOKEN:
	{
		const char *s;

		s = COM_Parse(&sv.entityParsePoint);
		Q_strncpyz(VMA(1), s, args[2]);
		if (!sv.entityParsePoint && !s[0])
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}

	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate(args[1], args[2], VMA(3));
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete(args[1]);
		return 0;
	case G_REAL_TIME:
		return Com_RealTime(VMA(1));
	case G_SNAPVECTOR:
		Sys_SnapVector(VMA(1));
		return 0;
	case G_GETTAG:
		return SV_GetTag(args[1], args[2], VMA(3), VMA(4));

	case G_REGISTERTAG:
		return SV_LoadTag(VMA(1));

	case G_REGISTERSOUND:
		return S_RegisterSound(VMA(1), args[2]);
	case G_GET_SOUND_LENGTH:
		return S_GetSoundLength(args[1]);

	//====================================

	case BOTLIB_SETUP:
		return 0;
	case BOTLIB_SHUTDOWN:
		return -1;
	case BOTLIB_LIBVAR_SET:
		return 0;
	case BOTLIB_LIBVAR_GET:
		return 0;

	case BOTLIB_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle(VMA(1));
	case BOTLIB_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle(args[1]);
	case BOTLIB_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
	case BOTLIB_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
	case BOTLIB_PC_UNREAD_TOKEN:
		botlib_export->PC_UnreadLastTokenHandle(args[1]);
		return 0;

	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage(args[1], VMA(2), args[3]);
	case BOTLIB_USER_COMMAND:
		{
			unsigned clientNum = args[1];

			if ( clientNum < sv_maxclients->integer )
			{
				SV_ClientThink(&svs.clients[clientNum], VMA(2));
			}
		}
		return 0;

	case BOTLIB_EA_COMMAND:
		{
			unsigned clientNum = args[1];

			if ( clientNum < sv_maxclients->integer )
			{
				SV_ExecuteClientCommand(&svs.clients[clientNum], VMA(2), qtrue, qfalse);
			}
		}

		return 0;

	case TRAP_MEMSET:
		Com_Memset(VMA(1), args[2], args[3]);
		return 0;

	case TRAP_MEMCPY:
		Com_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;

	case TRAP_STRNCPY:
		return (intptr_t)strncpy(VMA(1), VMA(2), args[3]);

	case TRAP_SIN:
		return FloatAsInt(sin(VMF(1)));

	case TRAP_COS:
		return FloatAsInt(cos(VMF(1)));

	case TRAP_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case TRAP_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case TRAP_MATRIXMULTIPLY: // never called for real
		_MatrixMultiply(VMA(1), VMA(2), VMA(3));
		return 0;

	case TRAP_ANGLEVECTORS:
		angles_vectors(VMA(1), VMA(2), VMA(3), VMA(4));
		return 0;

	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector(VMA(1), VMA(2));
		return 0;

	case TRAP_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case TRAP_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case PB_STAT_REPORT:
		return 0;

	case G_SENDMESSAGE:
		return SV_SendBinaryMessage(args[1], VMA(2), args[3]);
	case G_MESSAGESTATUS:
		return SV_BinaryMessageStatus(args[1]);

	case G_TRAP_GETVALUE:
		return SV_GetValue(VMA(1), args[2], VMA(3));

	default:
		Com_Error(ERR_DROP, "Bad game system trap: %ld", (long int) args[0]);
		break;
	}

	return -1;
}

/**
 * @brief Called every time a map changes
 */
void SV_ShutdownGameProgs(void)
{
	if (!gvm)
	{
		return;
	}

	// stop any demos
	SV_DemoStopAll();

	// shutdown game
	VM_Call(gvm, GAME_SHUTDOWN, qfalse);
	VM_Free(gvm);
	gvm = NULL;
}

/**
 * @brief Called for both a full init and a restart
 * @param[in] restart
 */
static void SV_InitGameVM(qboolean restart)
{
	int i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for (i = 0 ; i < sv_maxclients->integer ; i++)
	{
		svs.clients[i].gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call(gvm, GAME_INIT, svs.time, Com_Milliseconds(), restart, qtrue, ETLEGACY_VERSION_INT);

	// start recording a demo
	if (sv_autoDemo->integer)
	{
		// stop any demos
		SV_DemoStopAll();
		SV_DemoAutoDemoRecord();
	}
}

/**
 * @brief Called on a map_restart, but not on a normal map change
 */
void SV_RestartGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, GAME_SHUTDOWN, qtrue);

	// do a restart instead of a free
	gvm = VM_Restart(gvm);
	if (!gvm)
	{
		Com_Error(ERR_FATAL, "VM_Restart on game failed");
	}

	SV_InitGameVM(qtrue);
}

/**
 * @brief Called on a normal map change, not on a map_restart
 */
void SV_InitGameProgs(void)
{
	sv.num_tagheaders = 0;
	sv.num_tags       = 0;

	// load the dll
	gvm = VM_Create("qagame", qfalse, SV_GameSystemCalls, VMI_NATIVE);
	if (!gvm)
	{
		Com_Error(ERR_FATAL, "VM_Create on game failed");
	}

	SV_InitGameVM(qfalse);
}

/**
 * @brief See if the current console command is claimed by the game
 * @return
 */
qboolean SV_GameCommand(void)
{
	if (sv.state != SS_GAME)
	{
		return qfalse;
	}

	return VM_Call(gvm, GAME_CONSOLE_COMMAND);
}

/**
 * @brief SV_GetTag
 * @param[in] clientNum - unused
 * @param[in] tagFileNumber
 * @param[out] tagname
 * @param[out] orientation
 * @return qfalse if unable to retrieve tag information for this client
 */
qboolean SV_GetTag(int clientNum, int tagFileNumber, char *tagname, orientation_t *orientation)
{
	if (tagFileNumber > 0 && tagFileNumber <= sv.num_tagheaders)
	{
		int i;

		for (i = sv.tagHeadersExt[tagFileNumber - 1].start; i < sv.tagHeadersExt[tagFileNumber - 1].start + sv.tagHeadersExt[tagFileNumber - 1].count; i++)
		{
			if (!Q_stricmp(sv.tags[i].name, tagname))
			{
				VectorCopy(sv.tags[i].origin, orientation->origin);
				VectorCopy(sv.tags[i].axis[0], orientation->axis[0]);
				VectorCopy(sv.tags[i].axis[1], orientation->axis[1]);
				VectorCopy(sv.tags[i].axis[2], orientation->axis[2]);
				return qtrue;
			}
		}
	}

	return qfalse;
}
