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
 * @file sv_cl_parse.c
 */

#include "server.h"

const char *sv_cl_strings[32] =
{
	"svc_bad",

	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot",
	"svc_EOF",
	"svc_ettv_playerstates",
	"svc_ettv_currentstate"
};

/**
 * @brief SV_CL_SHOWNET
 * @param[in] msg
 * @param[in] s
 */
void SV_CL_SHOWNET(msg_t *msg, const char *s)
{
	if (sv_etltv_shownet->integer >= 2)
	{
		Com_Printf("%3i:%s\n", msg->readcount - 1, s);
	}
}

/**
  * @brief SV_CL_SetPurePaks
 * @param[in] referencedOnly
 */
static void SV_CL_SetPurePaks(qboolean referencedOnly)
{
	const char *s, *t;
	char       *systemInfo = svcl.gameState.stringData + svcl.gameState.stringOffsets[CS_SYSTEMINFO];

	if (!referencedOnly)
	{
		// check pure server string
		s = Info_ValueForKey(systemInfo, "sv_paks");
		t = Info_ValueForKey(systemInfo, "sv_pakNames");
		FS_PureServerSetLoadedPaks(s, t);
	}
	else
	{
		FS_PureServerSetLoadedPaks("", "");
	}

	s = Info_ValueForKey(systemInfo, "sv_referencedPaks");
	t = Info_ValueForKey(systemInfo, "sv_referencedPakNames");
	FS_PureServerSetReferencedPaks(s, t);
}

/**
  * @brief The systeminfo configstring has been changed, so parse new information out of it.
  * This will happen at every gamestate, and possibly during gameplay.
  */
void SV_CL_SystemInfoChanged(void)
{
	char       *systemInfo = svcl.gameState.stringData + svcl.gameState.stringOffsets[CS_SYSTEMINFO];
	const char *s;

	// NOTE: when the serverId changes, any further messages we send to the server will use this new serverId
	// in some cases, outdated cp commands might get sent with this news serverId
	svcl.serverId = Q_atoi(Info_ValueForKey(systemInfo, "sv_serverid"));

	// don't set any vars when playing a demo
	if (svclc.demo.playing)
	{
		// allow running demo in pure mode to simulate server environment,
		// but still setup the referenced packages for the container system to work
		//SV_CL_SetPurePaks(!svclc.demo.pure);
		return;
	}

	s = Info_ValueForKey(systemInfo, "sv_cheats");

	if (!Q_atoi(s))
	{
		Cvar_SetCheatState();
	}

	SV_CL_SetPurePaks(!sv_pure->integer);
}

/**
 * @brief SV_CL_ConfigstringInfoChanged update cvars based on configstring from master
 */
void SV_CL_ConfigstringInfoChanged(int csnum)
{
	char       *cs;
	const char *s;
	char       key[BIG_INFO_KEY];
	char       value[BIG_INFO_VALUE];

	cs = svcl.gameState.stringData + svcl.gameState.stringOffsets[csnum];

	s = cs;
	while (s)
	{
		cvarFlags_t cvar_flags;

		Info_NextPair(&s, key, value);
		if (!key[0])
		{
			break;
		}

		if (csnum == CS_SERVERINFO)
		{
			if (!Q_strncmp(key, "sv_", 3) || !Q_stricmp(key, "g_needpass") || !Q_stricmp(key, "g_password"))
			{
				continue;
			}
		}

		if ((cvar_flags = Cvar_Flags(key)) & CVAR_NONEXISTENT)
		{
			if (csnum == CS_SERVERINFO)
			{
				Cvar_Get(key, value, CVAR_SERVER_CREATED | CVAR_ROM | CVAR_SERVERINFO);
			}
			else
			{
				Cvar_Get(key, value, CVAR_SERVER_CREATED | CVAR_ROM | CVAR_WOLFINFO);
			}
		}
		else
		{
			Cvar_SetSafe(key, value);
		}
	}
}

/**
 * @brief SV_CL_ParseGamestate
 * @param[in] msg
 */
void SV_CL_ParseGamestate(msg_t *msg)
{
	int            i;
	entityState_t  *es;
	entityShared_t *entShared;
	int            newnum;
	entityState_t  nullstate;
	entityShared_t nullstateShared;
	int            cmd;
	char           *s;
	char           missingFiles[MAX_TOKEN_CHARS] = { '\0' };

	svclc.connectPacketCount = 0;

	// wipe local client state
	SV_CL_ClearState();

	// a gamestate always marks a server command sequence
	svclc.serverCommandSequence = MSG_ReadLong(msg);

	// parse all the configstrings and baselines
	svcl.gameState.dataCount = 1; // leave a 0 at the beginning for uninitialized configstrings
	while (1)
	{
		cmd = MSG_ReadByte(msg);

		if (cmd == svc_EOF)
		{
			break;
		}

		if (cmd == svc_configstring)
		{
			int len;

			i = MSG_ReadShort(msg);
			if (i < 0 || i >= MAX_CONFIGSTRINGS)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseGamestate: configstring < 0 or configstring >= MAX_CONFIGSTRINGS");
			}
			s   = MSG_ReadBigString(msg);
			len = strlen(s);

			if (len + 1 + svcl.gameState.dataCount > MAX_GAMESTATE_CHARS)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseGamestate: MAX_GAMESTATE_CHARS exceeded");
			}

			// append it to the gameState string buffer
			svcl.gameState.stringOffsets[i] = svcl.gameState.dataCount;
			Com_Memcpy(svcl.gameState.stringData + svcl.gameState.dataCount, s, len + 1);
			svcl.gameState.dataCount += len + 1;
		}
		else if (cmd == svc_baseline)
		{
			newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);
			if (newnum < 0 || newnum >= MAX_GENTITIES)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseGamestate: Baseline number out of range: %i", newnum);
			}
			Com_Memset(&nullstate, 0, sizeof(nullstate));
			Com_Memset(&nullstateShared, 0, sizeof(nullstateShared));
			es        = &svcl.entityBaselines[newnum];
			entShared = &svcl.entityBaselinesShared[newnum];
			MSG_ReadDeltaEntity(msg, &nullstate, es, newnum);
			MSG_ETTV_ReadDeltaEntityShared(msg, &nullstateShared, entShared);
		}
		else if (cmd == svc_ettv_currentstate)
		{
			newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);

			if (newnum < 0 || newnum >= MAX_GENTITIES)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseGamestate: Currentstate number out of range : %i", newnum);
			}

			MSG_ReadDeltaEntity(msg, &svcl.entityBaselines[newnum], &svcl.currentStateEntities[newnum], newnum);
			MSG_ETTV_ReadDeltaEntityShared(msg, &svcl.entityBaselinesShared[newnum], &svcl.currentStateEntitiesShared[newnum]);
			svcl.currentStateEntitiesShared[newnum].linked = qtrue;
		}
		else
		{
			Com_Error(ERR_DROP, "SV_CL_ParseGamestate: bad command byte");
		}
	}

	svclc.clientNum    = MSG_ReadLong(msg);
	svclc.checksumFeed = MSG_ReadLong(msg);

	// parse serverId and other cvars
	SV_CL_SystemInfoChanged();
	SV_CL_ConfigstringInfoChanged(CS_SERVERINFO);
	SV_CL_ConfigstringInfoChanged(CS_WOLFINFO);

	// happens always if the game is live
	// otherwise only on first server connection
	if (!svcls.isGamestateParsed)
	{
		svclc.serverIdLatest     = svcl.serverId;
		svclc.checksumFeedLatest = svclc.checksumFeed;
	}

	// Verify if we have all official pakfiles. As we won't
	// be downloading them, we should be kicked for not having them.
	if (sv_pure->integer && !FS_VerifyOfficialPaks())
	{
		Com_Error(ERR_DROP, "Couldn't load an official pak file; verify your installation and make sure it has been updated to the latest version.");
	}

	// reinitialize the filesystem if the game directory has changed
	FS_ConditionalRestart(svclc.checksumFeed);

	// Verify if we have all non-official pakfiles. As we won't
	// be downloading them, we should be kicked for not having them.
	if (FS_ComparePaks(missingFiles, sizeof(missingFiles), qfalse))
	{
		Com_Printf("Missing paks: %s\n", missingFiles);
		SV_CL_Disconnect();
		Cvar_Set("cl_paused", "0");
		sv.time = 0;
		return;
	}

	svclc.moveDelta = svcls.isGamestateParsed;

	SV_CL_DownloadsComplete();

	// make sure the game starts
	Cvar_Set("cl_paused", "0");

	svcls.fixHitch          = svcls.isGamestateParsed; // only true for delayed games on map change
	svcls.isGamestateParsed = svcls.isDelayed;
	svcls.firstSnap         = qfalse;
	svcls.queueDemoWaiting  = !sv_etltv_autorecord->integer;
}

/**
 * @brief SV_CL_ParseGamestateQueue parse gamestate message to read checksum, serverId and
 * send checksum and moveNoDelta usercmd, everything else is gonna get parsed properly later
 * @param[in] msg
 */
void SV_CL_ParseGamestateQueue(msg_t *msg)
{
	int            i;
	entityState_t  nullstate;
	entityShared_t nullstateShared;
	int            cmd;
	char           *s;

	// a gamestate always marks a server command sequence
	svclc.serverCommandSequenceLatest = MSG_ReadLong(msg);

	while (1)
	{
		cmd = MSG_ReadByte(msg);

		if (cmd == svc_EOF)
		{
			break;
		}

		if (cmd == svc_configstring)
		{
			i = MSG_ReadShort(msg);

			if (i < 0 || i >= MAX_CONFIGSTRINGS)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseGamestateQueue: configstring < 0 or configstring >= MAX_CONFIGSTRINGS");
			}

			s = MSG_ReadBigString(msg);

			if (i == CS_SYSTEMINFO)
			{
				svclc.serverIdLatest = Q_atoi(Info_ValueForKey(s, "sv_serverid"));
			}
		}
		else if (cmd == svc_baseline || cmd == svc_ettv_currentstate)
		{
			MSG_ReadBits(msg, GENTITYNUM_BITS);
			MSG_ReadDeltaEntity(msg, &nullstate, &nullstate, 0);
			MSG_ETTV_ReadDeltaEntityShared(msg, &nullstateShared, &nullstateShared);
		}
		else
		{
			Com_Error(ERR_DROP, "SV_CL_ParseGamestateQueue: bad command byte");
		}
	}

	MSG_ReadLong(msg); // clientNum
	svclc.checksumFeedLatest = MSG_ReadLong(msg);

	SV_CL_SendPureChecksums(svclc.serverIdLatest);

	svclc.moveDelta = qfalse;

	SV_CL_WritePacket();
	SV_CL_WritePacket();
	SV_CL_WritePacket();

	svcls.queueDemoWaiting = !sv_etltv_autorecord->integer;
}

/**
 * @brief Command strings are saved off and executed right away
 * @param[in] msg
 */
void SV_CL_ParseCommandString(msg_t *msg)
{
	char *s;
	int  seq;
	int  index;

	seq = MSG_ReadLong(msg);
	s   = MSG_ReadString(msg);

	// see if we have already executed stored it off
	if (svclc.serverCommandSequence >= seq)
	{
		return;
	}

	svclc.serverCommandSequence       = seq;
	svclc.serverCommandSequenceLatest = seq;
	index                             = seq & (MAX_RELIABLE_COMMANDS - 1);

	Q_strncpyz(svclc.serverCommands[index], s, sizeof(svclc.serverCommands[index]));
	Q_strncpyz(svclc.serverCommandsLatest[index], s, sizeof(svclc.serverCommands[index]));

	if (!gvm)
	{
		return;
	}

	while (svclc.lastExecutedServerCommand < svclc.serverCommandSequence)
	{
		if (SV_CL_GetServerCommand(++svclc.lastExecutedServerCommand))
		{
			VM_CallFunc(gvm, GAME_CLIENT_COMMAND, -2);
		}
	}
}

/**
 * @brief SV_CL_ParseBinaryMessage
 * @param[in] msg
 */
void SV_CL_ParseBinaryMessage(msg_t *msg)
{
	int size;

	MSG_BeginReadingUncompressed(msg);

	size = msg->cursize - msg->readcount;
	if (size <= 0 || size > MAX_BINARY_MESSAGE)
	{
		return;
	}

	SV_GameBinaryMessageReceived(-1, (char *)&msg->data[msg->readcount], size, svcl.snap.serverTime);
}

/**
 * @brief Parses deltas from the given base and adds the resulting entity to the current frame and
 * determines sv.num_entities for current server frame
 * @param[in] msg
 * @param[in,out] frame
 * @param[in] newnum
 * @param[in,out] old
 * @param[in,out] oldShared
 * @param[in] unchanged
 */
void SV_CL_DeltaEntity(msg_t *msg, svclSnapshot_t *frame, int newnum, entityState_t *old, entityShared_t *oldShared, qboolean unchanged)
{
	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	entityState_t  *state       = &svcl.parseEntities[svcl.parseEntitiesNum & (MAX_PARSE_ENTITIES - 1)];
	entityShared_t *stateShared = &svcl.parseEntitiesShared[svcl.parseEntitiesNum & (MAX_PARSE_ENTITIES - 1)];
	sharedEntity_t *gEnt;

	if (unchanged)
	{
		*state       = *old;
		*stateShared = *oldShared;
	}
	else
	{
		MSG_ReadDeltaEntity(msg, old, state, newnum);
		MSG_ETTV_ReadDeltaEntityShared(msg, oldShared, stateShared);
	}

	if (svcls.isTVGame && sv.gentities)
	{
		gEnt = SV_GentityNum(newnum);

		if (state->number == (MAX_GENTITIES - 1))
		{
			SV_UnlinkEntity(gEnt);

			if (sv_etltv_shownet->integer == 4)
			{
				Com_Printf("%d- ", newnum);
			}

			return;     // entity was delta removed
		}

		gEnt->s = *state;
		gEnt->r = *stateShared;

		if (state->number < MAX_CLIENTS)
		{
			gEnt->r.contents = CONTENTS_BODY;
			gEnt->r.ownerNum = ENTITYNUM_NONE;
		}

		gEnt->s.number = newnum;
		SV_LinkEntity(gEnt);

		sv.num_entities = newnum + 1;

		if (sv_etltv_shownet->integer == 4)
		{
			Com_Printf("%d+ ", newnum);
		}
	}

	if (state->number == (MAX_GENTITIES - 1))
	{
		return;     // entity was delta removed
	}

	svcl.parseEntitiesNum++;
	frame->numEntities++;
}

/**
 * @brief SV_CL_ParsePacketEntities
 * @param[in] msg
 * @param[in] oldframe
 * @param[out] newframe
 *
 * @note oldnum is set to MAX_GENTITIES to ensure newnum
 * will never be greater than oldnum in case of invalid oldframe or oldindex
 */
void SV_CL_ParsePacketEntities(msg_t *msg, svclSnapshot_t *oldframe, svclSnapshot_t *newframe)
{
	entityState_t  *oldstate;
	entityShared_t *oldstateShared;
	int            oldindex, newnum, oldnum;

	sv.num_entities = 0;

	oldstate       = NULL;
	oldstateShared = NULL;
	oldindex       = 0;

	newframe->parseEntitiesNum = svcl.parseEntitiesNum;
	newframe->numEntities      = 0;

	// delta from the entities present in oldframe
	if (!oldframe)
	{
		oldnum = MAX_GENTITIES;
	}
	else
	{
		if (oldindex >= oldframe->numEntities)
		{
			oldnum = MAX_GENTITIES;
		}
		else
		{
			oldstate = &svcl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldstateShared = &svcl.parseEntitiesShared[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		// read the entity index number
		newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);

		if (newnum >= (MAX_GENTITIES - 1))
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP, "SV_CL_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (sv_etltv_shownet->integer == 3)
			{
				Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			SV_CL_DeltaEntity(msg, newframe, oldnum, oldstate, oldstateShared, qtrue);

			oldindex++;

			if (!oldframe || oldindex >= oldframe->numEntities)
			{
				oldnum = MAX_GENTITIES;
			}
			else
			{
				oldstate = &svcl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldstateShared = &svcl.parseEntitiesShared[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}
		}

		if (oldnum == newnum)
		{
			// delta from previous state
			if (sv_etltv_shownet->integer == 3)
			{
				Com_Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			SV_CL_DeltaEntity(msg, newframe, newnum, oldstate, oldstateShared, qfalse);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = MAX_GENTITIES;
			}
			else
			{
				oldstate = &svcl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldstateShared = &svcl.parseEntitiesShared[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}
		}
		else if (oldnum > newnum)
		{
			// delta from baseline
			if (sv_etltv_shownet->integer == 3)
			{
				Com_Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			SV_CL_DeltaEntity(msg, newframe, newnum, &svcl.entityBaselines[newnum], &svcl.entityBaselinesShared[newnum], qfalse);
		}
	}

	// any remaining entities in the old frame are copied over
	while (oldnum != MAX_GENTITIES)
	{
		// one or more entities from the old packet are unchanged
		if (sv_etltv_shownet->integer == 3)
		{
			Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		SV_CL_DeltaEntity(msg, newframe, oldnum, oldstate, oldstateShared, qtrue);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = MAX_GENTITIES;
		}
		else
		{
			oldstate       = &svcl.parseEntities[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldstateShared = &svcl.parseEntitiesShared[(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldnum         = oldstate->number;
		}
	}

	//if (cl_shownuments->integer)
	{
		//Com_Printf("Entities in packet: %i\n", newframe->numEntities);
	}
}

/**
 * @brief If the snapshot is parsed properly, it will be copied to
 * cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
 * for any reason, no changes to the state will be made at all.
 *
 * @param[in] msg
 */
void SV_CL_ParseSnapshot(msg_t *msg)
{
	int            len;
	svclSnapshot_t *old;
	svclSnapshot_t newSnap;
	int            deltaNum;
	int            oldMessageNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.reliableAcknowledge = MSG_ReadLong( msg );

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset(&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to svc_snapshot
	newSnap.serverCommandNum = svclc.serverCommandSequence;

	newSnap.serverTime = MSG_ReadLong(msg);
	svcl.serverTime    = newSnap.serverTime;

	newSnap.messageNum = svclc.serverMessageSequence;

	deltaNum = MSG_ReadByte(msg);
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte(msg);

	svclc.moveDelta = qfalse;

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		svclc.moveDelta = qtrue;
		svcls.firstSnap = qtrue;
		newSnap.valid   = qtrue;      // uncompressed frame
		old             = NULL;

		if (svclc.demo.recording)
		{
			svclc.demo.waiting = qfalse;   // we can start recording now
		}
		else if (sv_etltv_autorecord->integer && !svclc.demo.playing)
		{
			Cbuf_ExecuteText(EXEC_APPEND, "record\n");
		}
	}
	else
	{
		old = &svcl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if (!old->valid)
		{
			// should never happen
			Com_Printf("SV_CL_ParseSnapshot: Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_DPrintf("SV_CL_ParseSnapshot: Delta frame too old.\n");
		}
		else if (svcl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - 128)
		{
			Com_Error(ERR_DROP, "SV_CL_ParseSnapshot: Delta parseEntitiesNum too old.\n");
		}
		else
		{
			if (!svclc.demo.recording && sv_etltv_autorecord->integer && !svclc.demo.playing)
			{
				Cbuf_ExecuteText(EXEC_APPEND, "record\n");
			}

			svclc.moveDelta = qtrue;
			newSnap.valid   = qtrue;  // valid delta parse
			svcls.firstSnap = qtrue;
		}
	}

	// read areamask
	len = MSG_ReadByte(msg);

	if (len < 0 || len > sizeof(newSnap.areamask))
	{
		Com_Error(ERR_DROP, "SV_CL_ParseSnapshot: Invalid size %d for areamask.", len);
		return;
	}

	MSG_ReadData(msg, &newSnap.areamask, len);

	// read playerinfo
	SV_CL_SHOWNET(msg, "playerstate");
	if (old)
	{
		MSG_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	SV_CL_SHOWNET(msg, "packet entities");
	SV_CL_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = svcl.snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		svcl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	svcl.snap = newSnap;

	// save the frame off in the backup array for later delta comparisons
	svcl.snapshots[svcl.snap.messageNum & PACKET_MASK] = svcl.snap;

	if (sv_etltv_shownet->integer == 3)
	{
		Com_Printf("   snapshot:%i  delta:%i\n", svcl.snap.messageNum,
		           svcl.snap.deltaNum);
	}

	svcl.newSnapshots = qtrue;
}

/**
 * @brief SV_CL_ParsePlayerstates
 * @param[in] msg
 */
void SV_CL_ParsePlayerstates(msg_t *msg)
{
	svclSnapshot_t *frame, *oldframe = NULL;
	int            i, clientNum, oldMessageNum;

	if (svcl.snap.deltaNum != -1)
	{
		oldframe = &svcl.snapshots[svcl.snap.deltaNum & PACKET_MASK];
	}

	frame = &svcl.snapshots[svcl.snap.messageNum & PACKET_MASK];

	while (1)
	{
		// read the clientNum
		clientNum = MSG_ReadByte(msg);

		if (clientNum == 255)
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP, "SV_CL_ParsePlayerstates: end of message");
		}

		if (!oldframe || !oldframe->playerstates[clientNum].valid)
		{
			MSG_ReadDeltaPlayerstate(msg, NULL, &frame->playerstates[clientNum].ps);

			if (sv_etltv_shownet->integer >= 2)
			{
				Com_Printf("%3i:playerstate baseline (client %d)\n", msg->readcount - 1, clientNum);
			}
		}
		else
		{
			MSG_ReadDeltaPlayerstate(msg, &oldframe->playerstates[clientNum].ps, &frame->playerstates[clientNum].ps);

			if (sv_etltv_shownet->integer >= 2)
			{
				Com_Printf("%3i:playerstate delta (client %d)\n", msg->readcount - 1, clientNum);
			}
		}

		frame->playerstates[clientNum].valid = qtrue;
	}

	svcl.snap = *frame;

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = svcl.snap.messageNum + 1;

	if (svcl.snap.messageNum - oldMessageNum >= PACKET_BACKUP)
	{
		oldMessageNum = svcl.snap.messageNum - (PACKET_BACKUP - 1);
	}
	for (; oldMessageNum < svcl.snap.messageNum; oldMessageNum++)
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			svcl.snapshots[oldMessageNum & PACKET_MASK].playerstates[i].valid = qfalse;
		}
	}

	if (sv_etltv_shownet->integer >= 2)
	{
		Com_Printf("%3i:end of playerstates\n", msg->readcount - 1);
	}
}

/**
 * @brief SV_CL_GetQueueTime
 * @return
 */
int SV_CL_GetQueueTime(void)
{
	static int prevTime = 0;

	if (svMsgQueueHead)
	{
		// map change hitch fix
		// when map changes it takes about 500-1000ms to load it, during that time packets are not parsed but still arrive
		// (master server doesn't know we delayed loading next map) which results in delayed "lag"
		// this is meant to change the times when the packets in the queue will be read
		// as if the packets were parsed into queue at the time they arrived (assuming no network issues)
		if (svcls.fixHitch && prevTime && svMsgQueueHead->serverTime &&
		    svMsgQueueHead->systime - prevTime > 100)
		{
			serverMessageQueue_t *cur      = svMsgQueueHead;
			int                  frameMsec = 1000 / sv_fps->integer;

			svcls.fixHitch = qfalse;

			do
			{
				cur->systime = prevTime + frameMsec;
				prevTime     = cur->systime;
				cur          = cur->next;
			}
			while (cur && cur->systime - prevTime > 100);
		}

		prevTime = svMsgQueueHead->systime;

		return Sys_Milliseconds() - svMsgQueueHead->systime + svMsgQueueHead->serverTime;
	}

	return 0;
}

/**
 * @brief SV_CL_ParseMessageQueue
 */
void SV_CL_ParseMessageQueue(void)
{
	int i, index;

	if (!svcls.isGamestateParsed)
	{
		return;
	}

	while (1)
	{
		if (!svMsgQueueHead || (svMsgQueueHead->serverTime && SV_CL_GetQueueTime() - sv_etltv_delay->integer * 1000 < svMsgQueueHead->serverTime))
		{
			break;
		}

		svclc.serverMessageSequence = svMsgQueueHead->serverMessageSequence;

		if (svMsgQueueHead->serverTime)
		{
			svcls.isDelayed = qfalse;
		}
		else
		{
			svcls.isDelayed = qtrue;
		}

		for (i = 0; i < svMsgQueueHead->numServerCommand; i++)
		{
			index = (i + svMsgQueueHead->serverCommandSequence) & (MAX_RELIABLE_COMMANDS - 1);
			Q_strncpyz(svclc.serverCommands[index], svMsgQueueHead->serverCommands[i], sizeof(svclc.serverCommands[index]));
			free(svMsgQueueHead->serverCommands[i]);
		}

		svclc.serverCommandSequence = svMsgQueueHead->numServerCommand + svMsgQueueHead->serverCommandSequence - 1;

		while (svclc.lastExecutedServerCommand < svclc.serverCommandSequence)
		{
			if (SV_CL_GetServerCommand(++svclc.lastExecutedServerCommand))
			{
				VM_CallFunc(gvm, GAME_CLIENT_COMMAND, -2);
			}
		}

		SV_CL_ParseServerMessage(&svMsgQueueHead->msg, svMsgQueueHead->headerBytes);

		if (svMsgQueueHead->next)
		{
			svMsgQueueHead = svMsgQueueHead->next;
			free(svMsgQueueHead->prev->msg.data);
			free(svMsgQueueHead->prev);
			svMsgQueueHead->prev = NULL;
		}
		else
		{
			free(svMsgQueueHead->msg.data);
			free(svMsgQueueHead);
			svMsgQueueHead = NULL;
		}
	}
}

/**
 * @brief SV_CL_ParseServerMessage_Ext
 * @param[in] msg
 * @param[in] headerBytes
 */
void SV_CL_ParseServerMessage_Ext(msg_t *msg, int headerBytes)
{
	MSG_Bitstream(msg);

	if (sv_etltv_shownet->integer == 1)
	{
		Com_Printf("%i ", msg->cursize);
	}
	else if (sv_etltv_shownet->integer >= 2)
	{
		Com_Printf("------------------\n");
	}

	// get the reliable sequence acknowledge number
	svclc.reliableAcknowledge = MSG_ReadLong(msg);

	if (svclc.reliableAcknowledge < svclc.reliableSequence - MAX_RELIABLE_COMMANDS)
	{
		svclc.reliableAcknowledge = svclc.reliableSequence;
	}

	// this statement is always true for live game
	// and only once on first server connection for delayed game
	// that way server will be up and running and clients can connect
	// even if snapshots with entities and players will still be delayed and not sent to clients
	if (!svcls.isGamestateParsed)
	{
		svclc.serverMessageSequence = svclc.serverMessageSequenceLatest;
		svclc.serverCommandSequence = svclc.serverCommandSequenceLatest;
		SV_CL_ParseServerMessage(msg, headerBytes);
		svcl.serverTimeLatest = svcl.serverTime;
		svclc.serverIdLatest  = svcl.serverId;
	}
	else
	{
		SV_CL_ParseServerMessageIntoQueue(msg, headerBytes);
		SV_CL_ParseMessageQueue();
	}
}

/**
 * @brief SV_CL_ParseServerMessage
 * @param[in] msg
 * @param[in] headerBytes
 */
void SV_CL_ParseServerMessage(msg_t *msg, int headerBytes)
{
	int cmd;

	// parse the message
	while (1)
	{
		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP, "SV_CL_ParseServerMessage: read past end of server message");
		}

		cmd = MSG_ReadByte(msg);

		if (cmd == svc_EOF)
		{
			SV_CL_SHOWNET(msg, "END OF MESSAGE");
			break;
		}

		if (sv_etltv_shownet->integer >= 2)
		{
			if (cmd < 0 || cmd > svc_ettv_currentstate) // MSG_ReadByte might return -1 and we can't access our sv_cl_strings array ...
			{
				Com_Printf("%3i:BAD BYTE %i\n", msg->readcount - 1, cmd); // -> ERR_DROP
			}
			else
			{
				if (!sv_cl_strings[cmd])
				{
					Com_Printf("%3i:BAD CMD %i\n", msg->readcount - 1, cmd);
				}
				else
				{
					SV_CL_SHOWNET(msg, sv_cl_strings[cmd]);
				}
			}
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP, "SV_CL_ParseServerMessage: Illegible server message %d", cmd);
		case svc_nop:
			break;
		case svc_serverCommand:
			SV_CL_ParseCommandString(msg);
			break;
		case svc_gamestate:
			SV_CL_ParseGamestate(msg);
			break;
		case svc_snapshot:
			SV_CL_ParseSnapshot(msg);
			break;
		case svc_ettv_playerstates:
			SV_CL_ParsePlayerstates(msg);
			break;
		case svc_download:
			//CL_ParseDownload(msg);
			break;
		}
	}

	SV_CL_ParseBinaryMessage(msg);

	if (svcls.isTVGame)
	{
		SV_CL_RunFrame();
	}

	if (svclc.demo.recording && !svclc.demo.waiting)
	{
		SV_CL_WriteDemoMessage(msg, headerBytes);
	}
}

/**
 * @brief SV_CL_Allocate
 */
static void *SV_CL_Allocate(int size)
{
	void *data;

	if (size > MAX_MSGLEN)
	{
		Com_Error(ERR_FATAL, "SV_CL_Allocate: Oversized allocation of [%d].", size);
	}

	data = Com_Allocate(size);

	if (!data)
	{
		Com_Error(ERR_FATAL, "SV_CL_Allocate: Couldn't allocate size [%d].", size);
	}

	return data;
}

/**
 * @brief SV_CL_NewMessage
 * @return
 */
static serverMessageQueue_t *SV_CL_NewMessage(void)
{
	serverMessageQueue_t *newMessage = Com_Allocate(sizeof(serverMessageQueue_t));

	if (!newMessage)
	{
		Com_Error(ERR_FATAL, "SV_CL_NewMessage: Couldn't allocate new message.");
	}

	Com_Memset(newMessage, 0, sizeof(serverMessageQueue_t));

	if (svMsgQueueHead == NULL)
	{
		svMsgQueueHead = svMsgQueueTail = newMessage;
	}
	else
	{
		svMsgQueueTail->next       = newMessage;
		svMsgQueueTail->next->prev = svMsgQueueTail;
		svMsgQueueTail             = newMessage;
	}

	return newMessage;
}

/**
 * @brief SV_CL_CheckNewQueuedCommand for change in serverId
 * @param[in] cmd
 */
static void SV_CL_CheckNewQueuedCommand(char *queuedCmd)
{
	char        *s                               = queuedCmd;
	char        cmd[MAX_STRING_TOKENS]           = { 0 };
	static char bigConfigString[BIG_INFO_STRING] = { 0 };

rescan:
	Cmd_TokenizeString(s);
	Q_strncpyz(cmd, Cmd_Argv(0), sizeof(cmd));

	if (!strcmp(cmd, "bcs0"))
	{
		Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return;
	}

	if (!strcmp(cmd, "bcs1"))
	{
		Q_strncpyz(cmd, Cmd_Argv(2), sizeof(cmd));
		if (strlen(bigConfigString) + strlen(cmd) >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		Q_strcat(bigConfigString, sizeof(bigConfigString), cmd);
		return;
	}

	if (!strcmp(cmd, "bcs2"))
	{
		Q_strncpyz(cmd, Cmd_Argv(2), sizeof(cmd));
		if (strlen(bigConfigString) + strlen(cmd) + 1 >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		Q_strcat(bigConfigString, sizeof(bigConfigString), cmd);
		Q_strcat(bigConfigString, sizeof(bigConfigString), "\"");
		s = bigConfigString;
		goto rescan;
	}

	if (!strcmp(cmd, "cs"))
	{
		if (Q_atoi(Cmd_Argv(1)) == CS_SYSTEMINFO)
		{
			s                    = Cmd_ArgsFrom(2);
			svclc.serverIdLatest = Q_atoi(Info_ValueForKey(s, "sv_serverid"));
		}
	}
}

/**
 * @brief SV_CL_CopyMsg
 * @param[in,out] dest
 * @param[in,out] src
 * @param[in] readCount
 * @param[in] bit
 */
void SV_CL_CopyMsg(msg_t *dest, msg_t *src, int readCount, int bit)
{
	byte *data;

	src->readcount = readCount;
	src->bit       = bit;
	data           = SV_CL_Allocate(src->cursize);
	MSG_Copy(dest, data, src->cursize, src);
}

/**
 * @brief SV_CL_ParseServerMessageIntoQueue
 * svc_serverCommand(s) are saved off and are not parsed again (saved msg skips it)
 * svc_gamestate is parsed fully (most read data is ignored) and will be parsed fully again
 * svc_snapshot is parsed only partly and will be parsed fully later
 * @param[in] msg
 */
void SV_CL_ParseServerMessageIntoQueue(msg_t *msg, int headerBytes)
{
	serverMessageQueue_t *currentMessage;
	char                 *s;
	int                  cmd, lastReadBit, lastReadCount, index, seq, size;

	currentMessage                        = SV_CL_NewMessage();
	currentMessage->headerBytes           = headerBytes;
	currentMessage->serverMessageSequence = svclc.serverMessageSequenceLatest;
	currentMessage->serverCommandSequence = svclc.serverCommandSequenceLatest;
	currentMessage->deltaNum              = -1;

	lastReadCount = msg->readcount;

	// parse the message
	while (1)
	{
		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP, "SV_CL_ParseServerMessageIntoQueue: read past end of server message");
		}

		lastReadBit = msg->bit;
		cmd         = MSG_ReadByte(msg);

		if (cmd == svc_EOF)
		{
			break;
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP, "SV_CL_ParseServerMessageIntoQueue: Illegible server message %d", cmd);
		case svc_nop:
			break;
		case svc_serverCommand:
			if (currentMessage->numServerCommand >= MAX_RELIABLE_COMMANDS)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseServerMessageIntoQueue: too many commands");
			}

			seq = MSG_ReadLong(msg);
			s   = MSG_ReadString(msg);

			lastReadCount = msg->readcount;

			if (!currentMessage->numServerCommand)
			{
				currentMessage->serverCommandSequence = seq;
			}
			else if (currentMessage->numServerCommand + currentMessage->serverCommandSequence != seq)
			{
				Com_Error(ERR_DROP, "SV_CL_ParseServerMessageIntoQueue: command out of order");
			}

			// see if we have already executed stored it off
			if (svclc.serverCommandSequenceLatest >= seq)
			{
				break;
			}

			svclc.serverCommandSequenceLatest = seq;
			index                             = seq & (MAX_RELIABLE_COMMANDS - 1);
			Q_strncpyz(svclc.serverCommandsLatest[index], s, sizeof(svclc.serverCommandsLatest[index]));

			size                                                             = strlen(s) + 1;
			currentMessage->serverCommands[currentMessage->numServerCommand] = SV_CL_Allocate(size);
			Q_strncpyz(currentMessage->serverCommands[currentMessage->numServerCommand], s, size);

			// check for new serverId
			SV_CL_CheckNewQueuedCommand(currentMessage->serverCommands[currentMessage->numServerCommand]);

			currentMessage->numServerCommand++;
			break;
		case svc_gamestate:
			SV_CL_ParseGamestateQueue(msg);
			SV_CL_CopyMsg(&currentMessage->msg, msg, lastReadCount, lastReadBit);
			return;
		case svc_snapshot:
			svcl.serverTimeLatest      = MSG_ReadLong(msg);
			currentMessage->serverTime = svcl.serverTimeLatest;
			currentMessage->systime    = Sys_Milliseconds();
			currentMessage->deltaNum   = MSG_ReadByte(msg);

			// try and predict if we should send moveNoDelta command
			// reduces the amount of unnecessary moveNoDelta commands caused by delay
			if (!currentMessage->deltaNum || svclc.serverMessageSequenceLatest == currentMessage->deltaNum ||
			    svclc.serverMessageSequenceLatest - currentMessage->deltaNum <= 0)
			{
				if (sv_etltv_autorecord->integer && !svcls.queueDemoWaiting &&
				    currentMessage->prev && !currentMessage->prev->deltaNum)
				{
					svcls.queueDemoWaiting = qtrue;
				}
				else
				{
					svclc.moveDelta = qtrue;
				}
			}

			SV_CL_CopyMsg(&currentMessage->msg, msg, lastReadCount, lastReadBit);
			return;
		}
	}

	SV_CL_CopyMsg(&currentMessage->msg, msg, lastReadCount, lastReadBit);
}
