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
 * @file sv_snapshot.c
 */

#include "server.h"

/*
=============================================================================

Delta encode a client frame onto the network channel

A normal server packet will look like:

4   sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1   svc_snapshot
4   last client reliable command
4   serverTime
1   lastframe for delta compression
1   snapFlags
1   areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/**
 * @brief Writes a delta update of an entityState_t list to the message.
 * @param[in] from
 * @param[in] to
 * @param[in] msg
 */
static void SV_EmitPacketEntities(clientSnapshot_t *from, clientSnapshot_t *to, msg_t *msg)
{
	entityState_t *oldent = NULL, *newent = NULL;
	int           oldindex = 0, newindex = 0;
	int           oldnum, newnum;
	int           from_num_entities;

	// generate the delta update
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.snapshotEntities[(to->first_entity + newindex) % svs.numSnapshotEntities];
			newnum = newent->number;
		}

		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.snapshotEntities[(from->first_entity + oldindex) % svs.numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is qfalse, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSG_WriteDeltaEntity(msg, oldent, newent, qfalse);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			if (newnum >= MAX_GENTITIES)
			{
				Com_Error(ERR_FATAL, "SV_EmitPacketEntities: MAX_GENTITIES exceeded");
			}

			// this is a new entity, send it from the baseline
			MSG_WriteDeltaEntity(msg, &sv.svEntities[newnum].baseline, newent, qtrue);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSG_WriteDeltaEntity(msg, oldent, NULL, qtrue);
			oldindex++;
			continue;
		}
	}

	MSG_WriteBits(msg, (MAX_GENTITIES - 1), GENTITYNUM_BITS);       // end of packetentities
}

/**
 * @brief SV_WriteSnapshotToClient
 * @param[in] client
 * @param[in] msg
 */
static void SV_WriteSnapshotToClient(client_t *client, msg_t *msg)
{
	clientSnapshot_t *frame, *oldframe;
	int              lastframe;
	int              snapFlags;

	// this is the snapshot we are creating
	frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

	// try to use a previous frame as the source for delta compressing the snapshot
	if (client->deltaMessage <= 0 || client->state != CS_ACTIVE)
	{
		// client is asking for a retransmit
		oldframe  = NULL;
		lastframe = 0;
	}
	else if (client->netchan.outgoingSequence - client->deltaMessage >= (PACKET_BACKUP - 3))
	{
		// client hasn't gotten a good message through in a long time
		Com_DPrintf("%s: Delta request from out of date packet.\n", client->name);
		oldframe  = NULL;
		lastframe = 0;
	}
	else
	{
		// we have a valid snapshot to delta from
		oldframe  = &client->frames[client->deltaMessage & PACKET_MASK];
		lastframe = client->netchan.outgoingSequence - client->deltaMessage;

		// the snapshot's entities may still have rolled off the buffer, though
		if (oldframe->first_entity <= svs.nextSnapshotEntities - svs.numSnapshotEntities)
		{
			Com_DPrintf("%s: Delta request from out of date entities.\n", client->name);
			oldframe  = NULL;
			lastframe = 0;
		}
	}

	MSG_WriteByte(msg, svc_snapshot);

	// NOTE, MRE: now sent at the start of every message from server to client
	// let the client know which reliable clientCommands we have received
	//MSG_WriteLong( msg, client->lastClientCommand );

	// send over the current server time so the client can drift
	// its view of time to try to match
	MSG_WriteLong(msg, svs.time);

	// what we are delta'ing from
	MSG_WriteByte(msg, lastframe);

	snapFlags = svs.snapFlagServerBit;
	if (client->rateDelayed)
	{
		snapFlags |= SNAPFLAG_RATE_DELAYED;
	}
	if (client->state != CS_ACTIVE)
	{
		snapFlags |= SNAPFLAG_NOT_ACTIVE;
	}

	MSG_WriteByte(msg, snapFlags);

	// send over the areabits
	MSG_WriteByte(msg, frame->areabytes);
	MSG_WriteData(msg, frame->areabits, frame->areabytes);

	//{
	//int sz = msg->cursize;
	//int usz = msg->uncompsize;

	// delta encode the playerstate
	if (oldframe)
	{
		MSG_WriteDeltaPlayerstate(msg, &oldframe->ps, &frame->ps);
	}
	else
	{
		MSG_WriteDeltaPlayerstate(msg, NULL, &frame->ps);
	}

	//Com_Printf( "Playerstate delta size: %f\n", ((msg->cursize - sz) * sv_fps->integer) / 8.f );
	//}

	// delta encode the entities
	SV_EmitPacketEntities(oldframe, frame, msg);

	// padding for rate debugging
	if (sv_padPackets->integer)
	{
		int i;

		for (i = 0 ; i < sv_padPackets->integer ; i++)
		{
			MSG_WriteByte(msg, svc_nop);
		}
	}
}

/**
 * @brief (re)send all server commands the client hasn't acknowledged yet
 * @param[in] client
 * @param[in] msg
 */
void SV_UpdateServerCommandsToClient(client_t *client, msg_t *msg)
{
	int i;

	// write any unacknowledged serverCommands
	for (i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++)
	{
		MSG_WriteByte(msg, svc_serverCommand);
		MSG_WriteLong(msg, i);
		MSG_WriteString(msg, client->reliableCommands[i & (MAX_RELIABLE_COMMANDS - 1)]);
	}

	client->reliableSent = client->reliableSequence;
}

/*
=============================================================================
Build a client snapshot structure
=============================================================================
*/

//#define   MAX_SNAPSHOT_ENTITIES   1024 // q3 uses this
#define MAX_SNAPSHOT_ENTITIES   2048

typedef struct
{
	int numSnapshotEntities;
	int snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;

/**
 * @brief SV_QsortEntityNumbers
 * @param[in] a
 * @param[in] b
 * @return
 */
static int QDECL SV_QsortEntityNumbers(const void *a, const void *b)
{
	const int *ea, *eb;

	ea = (const int *)a;
	eb = (const int *)b;

	if (*ea == *eb)
	{
		Com_Error(ERR_DROP, "SV_QsortEntityStates: duplicated entity");
	}

	if (*ea < *eb)
	{
		return -1;
	}

	return 1;
}

/**
 * @brief SV_AddEntToSnapshot
 * @param[in] clientEnt
 * @param[in,out] svEnt
 * @param[in] gEnt
 * @param[in,out] eNums
 */
static void SV_AddEntToSnapshot(sharedEntity_t *clientEnt, svEntity_t *svEnt, sharedEntity_t *gEnt, snapshotEntityNumbers_t *eNums)
{
	// if we have already added this entity to this snapshot, don't add again
	if (svEnt->snapshotCounter == sv.snapshotCounter)
	{
		return;
	}
	svEnt->snapshotCounter = sv.snapshotCounter;

	// if we are full, silently discard entities
	if (eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES)
	{
		Com_Printf("Warning: MAX_SNAPSHOT_ENTITIES reached. Ignoring ent.\n");
		return;
	}

	if (gEnt->r.snapshotCallback)
	{
		if (!(qboolean)(VM_Call(gvm, GAME_SNAPSHOT_CALLBACK, gEnt->s.number, clientEnt->s.number)))
		{
			return;
		}
	}

	eNums->snapshotEntities[eNums->numSnapshotEntities] = gEnt->s.number;
	eNums->numSnapshotEntities++;
}

#ifdef FEATURE_ANTICHEAT
/**
 * @brief SV_AddEntitiesVisibleFromPoint
 * @param[in] origin
 * @param[in,out] frame
 * @param[in] eNums
 * @param[in] portal
 */
static void SV_AddEntitiesVisibleFromPoint(vec3_t origin, clientSnapshot_t *frame, snapshotEntityNumbers_t *eNums, qboolean portal)
#else
/**
 * @brief SV_AddEntitiesVisibleFromPoint
 * @param[in] origin
 * @param[in,out] frame
 * @param[in] eNums
 */
static void SV_AddEntitiesVisibleFromPoint(vec3_t origin, clientSnapshot_t *frame, snapshotEntityNumbers_t *eNums)
#endif
{
	int            e, i;
	sharedEntity_t *ent, *playerEnt, *ment;
#ifdef FEATURE_ANTICHEAT
	sharedEntity_t *client;
#endif
	svEntity_t *svEnt;
	int        l;
	int        clientarea, clientcluster;
	int        leafnum;
	byte       *clientpvs;
	byte       *bitvector;

	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specfically check for it
	if (!sv.state)
	{
		return;
	}

	leafnum       = CM_PointLeafnum(origin);
	clientarea    = CM_LeafArea(leafnum);
	clientcluster = CM_LeafCluster(leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits(frame->areabits, clientarea);

	clientpvs = CM_ClusterPVS(clientcluster);

	playerEnt = SV_GentityNum(frame->ps.clientNum);
	if (playerEnt->r.svFlags & SVF_SELF_PORTAL)
	{
#ifdef FEATURE_ANTICHEAT
		SV_AddEntitiesVisibleFromPoint(playerEnt->s.origin2, frame, eNums, qtrue); // FIXME: portal qtrue?!
#else
		SV_AddEntitiesVisibleFromPoint(playerEnt->s.origin2, frame, eNums);
#endif
	}

	for (e = 0 ; e < sv.num_entities ; e++)
	{
		ent = SV_GentityNum(e);

		// never send entities that aren't linked in
		if (!ent->r.linked)
		{
			continue;
		}

		if (ent->s.number != e)
		{
			Com_DPrintf("FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}

		// entities can be flagged to explicitly not be sent to the client
		if (ent->r.svFlags & SVF_NOCLIENT)
		{
			continue;
		}

		// entities can be flagged to be sent to only one client
		if (ent->r.svFlags & SVF_SINGLECLIENT)
		{
			if (ent->r.singleClient != frame->ps.clientNum)
			{
				continue;
			}
		}
		// entities can be flagged to be sent to everyone but one client
		if (ent->r.svFlags & SVF_NOTSINGLECLIENT)
		{
			if (ent->r.singleClient == frame->ps.clientNum)
			{
				continue;
			}
		}

		svEnt = SV_SvEntityForGentity(ent);

		// don't double add an entity through portals
		if (svEnt->snapshotCounter == sv.snapshotCounter)
		{
			continue;
		}

		// broadcast entities are always sent
		if (ent->r.svFlags & SVF_BROADCAST)
		{
			SV_AddEntToSnapshot(playerEnt, svEnt, ent, eNums);
			continue;
		}

		bitvector = clientpvs;

		// just check origin for being in pvs, ignore bmodel extents
		if (ent->r.svFlags & SVF_IGNOREBMODELEXTENTS)
		{
			if (bitvector[svEnt->originCluster >> 3] & (1 << (svEnt->originCluster & 7)))
			{
				SV_AddEntToSnapshot(playerEnt, svEnt, ent, eNums);
			}

			continue;
		}

		// ignore if not touching a PV leaf
		// check area
		if (!CM_AreasConnected(clientarea, svEnt->areanum))
		{
			// doors can legally straddle two areas, so
			// we may need to check another one
			if (!CM_AreasConnected(clientarea, svEnt->areanum2))
			{
				continue;
			}
		}

		// check individual leafs
		if (!svEnt->numClusters)
		{
			continue;
		}
		l = 0;
		for (i = 0 ; i < svEnt->numClusters ; i++)
		{
			l = svEnt->clusternums[i];
			if (bitvector[l >> 3] & (1 << (l & 7)))
			{
				break;
			}
		}

		// if we haven't found it to be visible,
		// check overflow clusters that coudln't be stored
		if (i == svEnt->numClusters)
		{
			if (svEnt->lastCluster)
			{
				for ( ; l <= svEnt->lastCluster ; l++)
				{
					if (bitvector[l >> 3] & (1 << (l & 7)))
					{
						break;
					}
				}
				if (l == svEnt->lastCluster)
				{
					continue; // not visible
				}
			}
			else
			{
				continue;
			}
		}

		// added "visibility dummies"
		if (ent->r.svFlags & SVF_VISDUMMY)
		{
			// find master;
			ment = SV_GentityNum(ent->s.otherEntityNum);

			if (ment)
			{
				svEntity_t *master = 0;
				master = SV_SvEntityForGentity(ment);

				if (master->snapshotCounter == sv.snapshotCounter || !ment->r.linked)
				{
					continue;
				}

				SV_AddEntToSnapshot(playerEnt, master, ment, eNums);
			}

			continue;   // master needs to be added, but not this dummy ent
		}
		else if (ent->r.svFlags & SVF_VISDUMMY_MULTIPLE)
		{
			int        h;
			svEntity_t *master = 0;

			for (h = 0; h < sv.num_entities; h++)
			{
				ment = SV_GentityNum(h);

				if (ment == ent)
				{
					continue;
				}

				if (ment)
				{
					master = SV_SvEntityForGentity(ment);
				}
				else
				{
					continue;
				}

				if (!(ment->r.linked))
				{
					continue;
				}

				if (ment->s.number != h)
				{
					Com_DPrintf("FIXING vis dummy multiple ment->S.NUMBER!!!\n");
					ment->s.number = h;
				}

				if (ment->r.svFlags & SVF_NOCLIENT)
				{
					continue;
				}

				if (master->snapshotCounter == sv.snapshotCounter)
				{
					continue;
				}

				if (ment->s.otherEntityNum == ent->s.number)
				{
					SV_AddEntToSnapshot(playerEnt, master, ment, eNums);
				}
			}

			continue;
		}

#ifdef FEATURE_ANTICHEAT
		if (sv_wh_active->integer > 0 && e < sv_maxclients->integer)     // client
		{
			// note: !r.linked is already exclused - see above

			if (e == frame->ps.clientNum)
			{
				continue;
			}

			client = SV_GentityNum(frame->ps.clientNum);

			// exclude bots and free flying specs
			if (!portal && !(client->r.svFlags & SVF_BOT) && (frame->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) && !(frame->ps.pm_flags & PMF_FOLLOW))
			{
				if (!SV_CanSee(frame->ps.clientNum, e))
				{
					SV_RandomizePos(frame->ps.clientNum, e);
					SV_AddEntToSnapshot(client, svEnt, ent, eNums);
					continue;
				}
			}
		}
#endif

		// add it
		SV_AddEntToSnapshot(playerEnt, svEnt, ent, eNums);

		// if its a portal entity, add everything visible from its camera position
		if (ent->r.svFlags & SVF_PORTAL)
		{
#ifdef FEATURE_ANTICHEAT
			SV_AddEntitiesVisibleFromPoint(ent->s.origin2, frame, eNums, qtrue /*localClient*/);
#else
			SV_AddEntitiesVisibleFromPoint(ent->s.origin2, frame, eNums /*, qtrue, localClient*/);
#endif
		}

		continue;
	}
}

/**
 * @brief Decides which entities are going to be visible to the client, and
 * copies off the playerstate and areabits.
 *
 * This properly handles multiple recursive portals, but the render
 * currently doesn't.
 *
 * For viewing through other player's eyes, clent can be something other than client->gentity
 *
 * @param[in,out] client
 */
static void SV_BuildClientSnapshot(client_t *client)
{
	vec3_t                  org;
	clientSnapshot_t        *frame;
	snapshotEntityNumbers_t entityNumbers;
	int                     i;
	sharedEntity_t          *ent;
	entityState_t           *state;
	svEntity_t              *svEnt;
	sharedEntity_t          *clent;
	int                     clientNum;
	playerState_t           *ps;

	// bump the counter used to prevent double adding
	sv.snapshotCounter++;

	// this is the frame we are creating
	frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

	// clear everything in this snapshot
	entityNumbers.numSnapshotEntities = 0;
	Com_Memset(frame->areabits, 0, sizeof(frame->areabits));

	frame->num_entities = 0;

	clent = client->gentity;
	if (!clent || client->state == CS_ZOMBIE)
	{
		return;
	}

	// grab the current playerState_t
	ps        = SV_GameClientNum(client - svs.clients);
	frame->ps = *ps;

	// never send client's own entity, because it can
	// be regenerated from the playerstate
	clientNum = frame->ps.clientNum;
	if (clientNum < 0 || clientNum >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_BuildClientSnapshot: bad gEnt");
	}
	svEnt = &sv.svEntities[clientNum];

	svEnt->snapshotCounter = sv.snapshotCounter;

	if (clent->r.svFlags & SVF_SELF_PORTAL_EXCLUSIVE)
	{
		// find the client's viewpoint
		VectorCopy(clent->s.origin2, org);
	}
	else
	{
		VectorCopy(ps->origin, org);
	}
	org[2] += ps->viewheight;

	// added for 'lean'
	// need to account for lean, so areaportal doors draw properly
	if (frame->ps.leanf != 0.f)
	{
		vec3_t right, v3ViewAngles;
		VectorCopy(ps->viewangles, v3ViewAngles);
		v3ViewAngles[2] += frame->ps.leanf / 2.0f;
		angles_vectors(v3ViewAngles, NULL, right, NULL);
		VectorMA(org, frame->ps.leanf, right, org);
	}

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
#ifdef FEATURE_ANTICHEAT
	SV_AddEntitiesVisibleFromPoint(org, frame, &entityNumbers, qfalse /*client->netchan.remoteAddress.type == NA_LOOPBACK*/);
#else
	SV_AddEntitiesVisibleFromPoint(org, frame, &entityNumbers /*, qfalse, client->netchan.remoteAddress.type == NA_LOOPBACK*/);
#endif

	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort(entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
	      sizeof(entityNumbers.snapshotEntities[0]), SV_QsortEntityNumbers);

	// now that all viewpoint's areabits have been OR'd together, invert
	// all of them to make it a mask vector, which is what the renderer wants
	for (i = 0 ; i < MAX_MAP_AREA_BYTES / 4 ; i++)
	{
		((int *)frame->areabits)[i] = ((int *)frame->areabits)[i] ^ -1;
	}

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.nextSnapshotEntities;
	for (i = 0 ; i < entityNumbers.numSnapshotEntities ; i++)
	{
		ent    = SV_GentityNum(entityNumbers.snapshotEntities[i]);
		state  = &svs.snapshotEntities[svs.nextSnapshotEntities % svs.numSnapshotEntities];
		*state = ent->s;

#ifdef FEATURE_ANTICHEAT
		if (sv_wh_active->integer && entityNumbers.snapshotEntities[i] < sv_maxclients->integer)
		{
			if (SV_PositionChanged(entityNumbers.snapshotEntities[i]))
			{
				SV_RestorePos(entityNumbers.snapshotEntities[i]);
			}
		}
#endif

		svs.nextSnapshotEntities++;
		// this should never hit, map should always be restarted first in SV_Frame
		if (svs.nextSnapshotEntities >= 0x7FFFFFFE)
		{
			Com_Error(ERR_FATAL, "SV_BuildClientSnapshot: svs.nextSnapshotEntities wrapped");
		}

		frame->num_entities++;
	}
}

#define UDPIP_HEADER_SIZE 28
#define UDPIP6_HEADER_SIZE 48

/**
 * @brief Return the number of msec until another message can be sent to
 * a client based on its rate settings
 *
 * @param[in] client
 *
 * @return The number of msec
 */
int SV_RateMsec(client_t *client)
{
	int rate, rateMsec;
	int messageSize;

	messageSize = client->netchan.lastSentSize;
	rate        = client->rate;

	if (sv_maxRate->integer)
	{
		if (sv_maxRate->integer < 1000)
		{
			Cvar_Set("sv_MaxRate", "1000");
		}
		if (sv_maxRate->integer < rate)
		{
			rate = sv_maxRate->integer;
		}
	}

	if (sv_minRate->integer)
	{
		if (sv_minRate->integer < 1000)
		{
			Cvar_Set("sv_minRate", "1000");
		}
		if (sv_minRate->integer > rate)
		{
			rate = sv_minRate->integer;
		}
	}

	if (client->netchan.remoteAddress.type == NA_IP6)
	{
		messageSize += UDPIP6_HEADER_SIZE;
	}
	else
	{
		messageSize += UDPIP_HEADER_SIZE;
	}

	if (com_timescale->value > 0.f)
	{
		rateMsec = messageSize * 1000 / ((int)(rate * com_timescale->value));
	}
	else
	{
		rateMsec = messageSize * 1000 / rate;
	}
	rate = Sys_Milliseconds() - client->netchan.lastSentTime;

	if (rate > rateMsec)
	{
		return 0;
	}
	else
	{
		return rateMsec - rate;
	}
}

/**
 * @brief Called by SV_SendClientSnapshot and SV_SendClientGameState
 * @param[in] msg
 * @param[in,out] client
 */
void SV_SendMessageToClient(msg_t *msg, client_t *client)
{
	// record information about the message
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize  = msg->cursize;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent  = svs.time;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

	// send the datagram
	SV_Netchan_Transmit(client, msg);
}

/**
 * @brief There is no need to send full snapshots to clients who are loading a map.
 * So we send them "idle" packets with the bare minimum required to keep them on the server.
 *
 * @param[in] client
 */
void SV_SendClientIdle(client_t *client)
{
	byte  msg_buf[MAX_MSGLEN];
	msg_t msg;

	MSG_Init(&msg, msg_buf, sizeof(msg_buf));
	msg.allowoverflow = qtrue;

	if (!Com_IsCompatible(&client->agent, 0x1))
	{
		MSG_EnableCharStrip(&msg);
	}

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong(&msg, client->lastClientCommand);

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant entityState_t
	// and the playerState_t
	//  SV_WriteSnapshotToClient( client, &msg );

	// Add any download data if the client is downloading
	//SV_WriteDownloadToClient(client, &msg);

    if (SV_CheckForMsgOverflow(client, &msg))
    {
        return;
    }

	SV_SendMessageToClient(&msg, client);

	sv.bpsTotalBytes  += msg.cursize;           // net debugging
	sv.ubpsTotalBytes += msg.uncompsize / 8;    // net debugging
}

/**
 * @brief SV_SendClientSnapshot
 *
 * @param[in] client
 *
 * @note Also called by SV_FinalCommand
 */
void SV_SendClientSnapshot(client_t *client)
{
	byte  msg_buf[MAX_MSGLEN];
	msg_t msg;

	if (client->state < CS_ACTIVE)
	{
		// zombie clients need full snaps so they can still process reliable commands
		// (eg so they can pick up the disconnect reason)
		if (client->state != CS_ZOMBIE)
		{
			SV_SendClientIdle(client);
			return;
		}
	}

	// build the snapshot
	SV_BuildClientSnapshot(client);

	// bots need to have their snapshots build, but
	// the query them directly without needing to be sent
	if (client->gentity && (client->gentity->r.svFlags & SVF_BOT))
	{
		return;
	}

	MSG_Init(&msg, msg_buf, sizeof(msg_buf));
	msg.allowoverflow = qtrue;

	if (!Com_IsCompatible(&client->agent, 0x1))
	{
		MSG_EnableCharStrip(&msg);
	}

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong(&msg, client->lastClientCommand);

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant entityState_t
	// and the playerState_t
	SV_WriteSnapshotToClient(client, &msg);

    if (SV_CheckForMsgOverflow(client, &msg))
    {
        return;
    }

	SV_SendMessageToClient(&msg, client);

	sv.bpsTotalBytes  += msg.cursize;           // net debugging
	sv.ubpsTotalBytes += msg.uncompsize / 8;    // net debugging
}

/**
 * @brief SV_SendClientMessages
 */
void SV_SendClientMessages(void)
{
	int      i;
	client_t *c;
	int      numclients = 0;    // net debugging

	sv.bpsTotalBytes  = 0;      // net debugging
	sv.ubpsTotalBytes = 0;      // net debugging

	// update any changed configstrings from this frame
	SV_UpdateConfigStrings();

	// send a message to each connected client
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		c = &svs.clients[i];

		// changed <= CS_ZOMBIE to < CS_ZOMBIE so that the
		// disconnect reason is properly sent in the network stream
		// do not send a packet to a democlient, this will cause the engine to crash
		if (c->state < CS_ZOMBIE || c->demoClient)
		{
			continue;       // not connected
		}

		// needed to insert this otherwise bots would cause error drops in sv_net_chan.c:
		// --> "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment\n"
		if (c->gentity && (c->gentity->r.svFlags & SVF_BOT))
		{
			continue;
		}

		if (svs.time - c->lastSnapshotTime < c->snapshotMsec * com_timescale->value)
		{
			continue;       // It's not time yet
		}

		if (*c->downloadName)
		{
			// If the client is downloading via netchan and has not acknowledged a package in 4secs drop it
			if (c->download && (svs.time - c->downloadAckTime) > 4000)
			{
				SV_DropClient(c, "Download failed");
			}
			c->lastValidGamestate = svs.time;
			continue;       // Client is downloading, don't send snapshots
		}
		else if (c->state == CS_ACTIVE)
		{
			c->lastValidGamestate = svs.time;
		}

		if (c->netchan.unsentFragments || c->netchan_start_queue)
		{
			c->rateDelayed = qtrue;
			continue;       // Drop this snapshot if the packet queue is still full or delta compression will break
		}

		if (!(c->netchan.remoteAddress.type == NA_LOOPBACK ||
		      (sv_lanForceRate->integer && Sys_IsLANAddress(c->netchan.remoteAddress))))
		{
			// rate control for clients not on LAN
			if (SV_RateMsec(c) > 0)
			{
				// Not enough time since last packet passed through the line
				c->rateDelayed = qtrue;
				continue;
			}
		}

		numclients++; // net debugging

		// generate and send a new message
		SV_SendClientSnapshot(c);
		c->lastSnapshotTime = svs.time;
		c->rateDelayed      = qfalse;
	}

	// net debugging
	if (sv_showAverageBPS->integer && numclients > 0)
	{
		float ave = 0, uave = 0;

		for (i = 0; i < MAX_BPS_WINDOW - 1; i++)
		{
			sv.bpsWindow[i] = sv.bpsWindow[i + 1];
			ave            += sv.bpsWindow[i];

			sv.ubpsWindow[i] = sv.ubpsWindow[i + 1];
			uave            += sv.ubpsWindow[i];
		}

		sv.bpsWindow[MAX_BPS_WINDOW - 1] = sv.bpsTotalBytes;
		ave                             += sv.bpsTotalBytes;

		sv.ubpsWindow[MAX_BPS_WINDOW - 1] = sv.ubpsTotalBytes;
		uave                             += sv.ubpsTotalBytes;

		if (sv.bpsTotalBytes >= sv.bpsMaxBytes)
		{
			sv.bpsMaxBytes = sv.bpsTotalBytes;
		}

		if (sv.ubpsTotalBytes >= sv.ubpsMaxBytes)
		{
			sv.ubpsMaxBytes = sv.ubpsTotalBytes;
		}

		sv.bpsWindowSteps++;

		if (sv.bpsWindowSteps >= MAX_BPS_WINDOW)
		{
			float comp_ratio;

			sv.bpsWindowSteps = 0;

			ave  = (ave / (float)MAX_BPS_WINDOW);
			uave = (uave / (float)MAX_BPS_WINDOW);

			comp_ratio   = (1 - ave / uave) * 100.f;
			sv.ucompAve += comp_ratio;
			sv.ucompNum++;

			Com_DPrintf("bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
			            (double)(ave / (float)numclients), (double)ave, sv.bpsMaxBytes, (double)uave, sv.ubpsMaxBytes, (double)comp_ratio, (double)(sv.ucompAve / (float)sv.ucompNum));
		}
	}
}

/**
 * @brief SV_CheckClientUserinfoTimer
 */
void SV_CheckClientUserinfoTimer(void)
{
	int      i;
	client_t *cl;
	char     bigbuffer[MAX_INFO_STRING * 2];

	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (!cl->state)
		{
			continue; // not connected
		}
		if ((sv_floodProtect->integer) && (svs.time >= cl->nextReliableUserTime) && (cl->state >= CS_ACTIVE) && (cl->userinfobuffer[0] != 0))
		{
			// We have something in the buffer and it's time to process it
			Com_sprintf(bigbuffer, sizeof(bigbuffer), "userinfo \"%s\"", cl->userinfobuffer);

			Cmd_TokenizeString(bigbuffer);
			SV_UpdateUserinfo_f(cl);
		}
	}
}
