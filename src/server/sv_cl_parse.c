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

	SV_CL_SetPurePaks(qfalse);
}

/**
 * @brief SV_CL_ConfigstringInfoChanged update cvars based on configstring from master
 */
static void SV_CL_ConfigstringInfoChanged(int csnum)
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
			if (!Q_strncmp(key, "sv_", 3) || !Q_stricmp(key, "protocol") || !Q_stricmp(key, "version") ||
			    !Q_stricmp(key, "gamename") || !Q_stricmp(key, "g_needpass") || !Q_stricmp(key, "g_password"))
			{
				continue;
			}
		}

		if ((cvar_flags = Cvar_Flags(key)) & CVAR_NONEXISTENT)
		{
			Cvar_Get(key, value, CVAR_SERVER_CREATED | CVAR_ROM);
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
				Com_Error(ERR_DROP, "configstring < 0 or configstring >= MAX_CONFIGSTRINGS");
			}
			s   = MSG_ReadBigString(msg);
			len = strlen(s);

			if (len + 1 + svcl.gameState.dataCount > MAX_GAMESTATE_CHARS)
			{
				Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
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
				Com_Error(ERR_DROP, "Baseline number out of range: %i", newnum);
			}
			Com_Memset(&nullstate, 0, sizeof(nullstate));
			Com_Memset(&nullstateShared, 0, sizeof(nullstateShared));
			es        = &svcl.entityBaselines[newnum];
			entShared = &svcl.entitySharedBaselines[newnum];
			MSG_ReadDeltaEntity(msg, &nullstate, es, newnum);
			MSG_ETTV_ReadDeltaEntityShared(msg, &nullstateShared, entShared);
		}
		else if (cmd == svc_ettv_currentstate)
		{
			newnum = MSG_ReadBits(msg, GENTITYNUM_BITS);

			if (newnum < 0 || newnum >= MAX_GENTITIES)
			{
				Com_Error(ERR_DROP, "Currentstate number out of range : %i", newnum);
			}

			MSG_ReadDeltaEntity(msg, &svcl.entityBaselines[newnum], &svcl.currentStateEntities[newnum], newnum);
			MSG_ETTV_ReadDeltaEntityShared(msg, &svcl.entitySharedBaselines[newnum], &svcl.currentStateEntitiesShared[newnum]);
			svcl.currentStateEntitiesShared[newnum].linked = qtrue;
		}
		else
		{
			Com_Error(ERR_DROP, "CL_ParseGamestate: bad command byte");
		}
	}

	svclc.clientNum = MSG_ReadLong(msg);
	// read the checksum feed
	svclc.checksumFeed = MSG_ReadLong(msg);

	// parse serverId and other cvars
	SV_CL_SystemInfoChanged();
	SV_CL_ConfigstringInfoChanged(CS_SERVERINFO);
	SV_CL_ConfigstringInfoChanged(CS_WOLFINFO);

	// Verify if we have all official pakfiles. As we won't
	// be downloading them, we should be kicked for not having them.
	if (sv_pure->integer && !FS_VerifyOfficialPaks())
	{
		Com_Error(ERR_DROP, "Couldn't load an official pak file; verify your installation and make sure it has been updated to the latest version.");
	}

	// reinitialize the filesystem if the game directory has changed
	FS_ConditionalRestart(svclc.checksumFeed);

	if (FS_ComparePaks(missingFiles, sizeof(missingFiles), qfalse))
	{
		Com_Printf("Missing paks: %s\n", missingFiles);
		SV_CL_Disconnect();
		Cvar_Set("cl_paused", "0");
		sv.time = 0;
		return;
	}

	SV_CL_DownloadsComplete();

	// make sure the game starts
	Cvar_Set("cl_paused", "0");
}

/**
 * @brief Command strings are just saved off until cgame asks for them
 * when it transitions a snapshot
 *
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

	svclc.serverCommandSequence = seq;
	index                       = seq & (MAX_RELIABLE_COMMANDS - 1);

	Q_strncpyz(svclc.serverCommands[index], s, sizeof(svclc.serverCommands[index]));

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
 * @brief Parses deltas from the given base and adds the resulting entity to the current frame
 *
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
			return;     // entity was delta removed
		}

		gEnt->s = *state;
		gEnt->r = *stateShared;

		if (state->number < MAX_CLIENTS)
		{
			gEnt->r.contents = CONTENTS_SOLID;
		}

		gEnt->s.number = newnum;
		SV_LinkEntity(gEnt);
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
			Com_Error(ERR_DROP, "CL_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			//if (cl_shownet->integer == 3)
			//{
			//	Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			//}
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
			//if (cl_shownet->integer == 3)
			//{
			//	Com_Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			//}
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
			//if (cl_shownet->integer == 3)
			//{
			//	Com_Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			//}
			SV_CL_DeltaEntity(msg, newframe, newnum, &svcl.entityBaselines[newnum], &svcl.entitySharedBaselines[newnum], qfalse);
		}
	}

	// any remaining entities in the old frame are copied over
	while (oldnum != MAX_GENTITIES)
	{
		// one or more entities from the old packet are unchanged
		//if (cl_shownet->integer == 3)
		//{
		//	Com_Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		//}
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
	int            i, packetNum;

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

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = qtrue;      // uncompressed frame
		old           = NULL;
		if (svclc.demo.recording)
		{
			svclc.demo.waiting = qfalse;   // we can start recording now
			//if(cl_autorecord->integer) {
			//  Cvar_Set( "g_synchronousClients", "0" );
			//}
		}
		//else
		//{
		//	if (cl_autorecord->integer /*&& Cvar_VariableValue( "g_synchronousClients")*/)
		//	{
		//		char    name[256];
		//		char    mapname[MAX_QPATH];
		//		char    *period;
		//		qtime_t time;

		//		Com_RealTime(&time);

		//		Q_strncpyz(mapname, cl.mapname, MAX_QPATH);
		//		for (period = mapname; *period; period++)
		//		{
		//			if (*period == '.')
		//			{
		//				*period = '\0';
		//				break;
		//			}
		//		}

		//		for (period = mapname; *period; period++)
		//		{
		//			if (*period == '/')
		//			{
		//				break;
		//			}
		//		}
		//		if (*period)
		//		{
		//			period++;
		//		}

		//		Com_sprintf(name, sizeof(name), "demos/%d-%02d-%02d-%02d%02d%02d-%s.dm_%d", 1900 + time.tm_year,
		//			time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, period, PROTOCOL_VERSION);

		//		CL_Record(name);
		//	}
		//}
	}
	else
	{
		old = &svcl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if (!old->valid)
		{
			// should never happen
			Com_Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_DPrintf("Delta frame too old.\n");
		}
		else if (svcl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - 128)
		{
			Com_DPrintf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = qtrue;  // valid delta parse
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
	//SHOWNET(msg, "playerstate");
	if (old)
	{
		MSG_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSG_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	//SHOWNET(msg, "packet entities");
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
	svcl.snap      = newSnap;
	svcl.snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP; i++)
	{
		packetNum = (svclc.netchan.outgoingSequence - 1 - i) & PACKET_MASK;
		if (svcl.snap.ps.commandTime >= svcl.outPackets[packetNum].p_serverTime)
		{
			svcl.snap.ping = svcls.realtime - svcl.outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	svcl.snapshots[svcl.snap.messageNum & PACKET_MASK] = svcl.snap;

	//if (cl_shownet->integer == 3)
	//{
	//	Com_Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum,
	//		cl.snap.deltaNum, cl.snap.ping);
	//}

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

		if (!oldframe || !oldframe->playerstates[clientNum].valid)
		{
			MSG_ReadDeltaPlayerstate(msg, NULL, &frame->playerstates[clientNum].ps);
		}
		else
		{
			MSG_ReadDeltaPlayerstate(msg, &oldframe->playerstates[clientNum].ps, &frame->playerstates[clientNum].ps);
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
}

/**
 * @brief SV_CL_ParseServerMessage
 * @param[in] msg
 */
void SV_CL_ParseServerMessage(msg_t *msg)
{
	int cmd;

	//if (cl_shownet->integer == 1)
	//{
	//	Com_Printf("%i ", msg->cursize);
	//}
	//else if (cl_shownet->integer >= 2)
	//{
	//	Com_Printf("------------------\n");
	//}

	MSG_Bitstream(msg);

	// get the reliable sequence acknowledge number
	svclc.reliableAcknowledge = MSG_ReadLong(msg);

	if (svclc.reliableAcknowledge < svclc.reliableSequence - MAX_RELIABLE_COMMANDS)
	{
		svclc.reliableAcknowledge = svclc.reliableSequence;
	}

	// parse the message
	while (1)
	{
		if (msg->readcount > msg->cursize)
		{
			Com_Error(ERR_DROP, "CL_ParseServerMessage: read past end of server message");
		}

		cmd = MSG_ReadByte(msg);

		if (cmd == svc_EOF)
		{
			//SHOWNET(msg, "END OF MESSAGE");
			break;
		}

		//if (cl_shownet->integer >= 2)
		//{
		//	if (cmd < 0 || cmd > svc_EOF) // MSG_ReadByte might return -1 and we can't access our svc_strings array ...
		//	{
		//		Com_Printf("%3i:BAD BYTE %i\n", msg->readcount - 1, cmd); // -> ERR_DROP
		//	}
		//	else
		//	{
		//		if (!svc_strings[cmd])
		//		{
		//			Com_Printf("%3i:BAD CMD %i\n", msg->readcount - 1, cmd);
		//		}
		//		else
		//		{
		//			SHOWNET(msg, svc_strings[cmd]);
		//		}
		//	}
		//}

		// other commands
		switch (cmd)
		{
		default:
			Com_Error(ERR_DROP, "CL_ParseServerMessage: Illegible server message %d", cmd);
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
}
