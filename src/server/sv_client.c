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
 * @file sv_client.c
 * @brief Server code for dealing with clients
 */

#include "server.h"

static void SV_CloseDownload(client_t *cl);

/**
 * @brief A "getchallenge" OOB command has been received
 *
 * @param[in] from
 *
 * @returns challenge number that can be used in a subsequent
 *          connectResponse command.
 *
 * We do this to prevent denial of service attacks that flood
 * the server with invalid connection IPs. With a challenge,
 * they must give a valid IP address.
 */
void SV_GetChallenge(netadr_t from)
{
	int         i;
	int         oldest;
	int         oldestTime;
	challenge_t *challenge;

	if (SV_TempBanIsBanned(from))
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string);
		return;
	}

	if (sv_protect->integer & SVP_IOQ3)
	{
		// Prevent using getchallenge as an amplifier
		if (SVC_RateLimitAddress(from, 10, 1000))
		{
			SV_WriteAttackLog(va("SV_GetChallenge: rate limit from %s exceeded, dropping request\n",
			                     NET_AdrToString(from)));
			return;
		}

		// Allow getchallenge to be DoSed relatively easily, but prevent
		// excess outbound bandwidth usage when being flooded inbound
		if (SVC_RateLimit(&outboundLeakyBucket, 10, 100))
		{
			SV_WriteAttackLog("SV_GetChallenge: rate limit exceeded, dropping request\n");
			return;
		}
	}

	oldest     = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	challenge = &svs.challenges[0];
	for (i = 0 ; i < MAX_CHALLENGES ; i++, challenge++)
	{
		if (!challenge->connected && NET_CompareAdr(from, challenge->adr))
		{
			break;
		}
		if (challenge->time < oldestTime)
		{
			oldestTime = challenge->time;
			oldest     = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// this is the first time this client has asked for a challenge
		challenge = &svs.challenges[oldest];

		challenge->challenge = (((unsigned int) rand() << 16) ^ (unsigned int)rand()) ^ svs.time;
		challenge->adr       = from;
		challenge->firstTime = svs.time;
		challenge->firstPing = 0;
		challenge->time      = svs.time;
		challenge->connected = qfalse;
	}

	// FIXME: deal with restricted filesystem - done with sv_pure check ?

	challenge->pingTime = svs.time;
	if (sv_onlyVisibleClients->integer)
	{
		NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i %i", challenge->challenge, sv_onlyVisibleClients->integer);
	}
	else
	{
		NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i", challenge->challenge);
	}

	return;
}

/**
 * @brief Determines if a client is allowed to connect when sv_ipMaxClients is set
 * @param[in] from
 * @return    qtrue for valid clients
 *
 * @note maybe we want to have more control over localhost clients in future.
 * So we let localhost connect since bots don't connect from SV_DirectConnect
 * where SV_isClientIPValidToConnect is done.
 */
static qboolean SV_isClientIPValidToConnect(netadr_t from)
{
	client_t   *clientTmp;
	int        count = 1;     // we count as the first one
	int        max   = sv_ipMaxClients->integer;
	int        i;
	const char *theirIP;
	const char *clientIP;

	// disabled
	if (max <= 0)
	{
		return qtrue;
	}

	clientIP = NET_AdrToString(from);

	// let localhost connect
	// FIXME: see above note: We might use a free flag of sv_protect cvar to include local addresses
	if (NET_IsLocalAddress(from))
	{
		return qtrue;
	}

	// Iterate over each connected client and check the IP, keeping a count of connections
	// from the same IP as this connecting client
	for (i = 0; i < sv_maxclients->integer; ++i)
	{
		clientTmp = &svs.clients[i];

		if (clientTmp->state < CS_CONNECTED) // only active clients
		{
			continue;
		}

		theirIP = NET_AdrToString(clientTmp->netchan.remoteAddress);

		// Don't compare the port - just the IP
		if (CompareIPNoPort(clientIP, theirIP))
		{
			++count;
			if (count > max)
			{
				// no dev print - let admins see this
				Com_Printf("SV_isClientIPValidToConnect: too many connections from %s\n", clientIP);

				return qfalse;
			}
		}
	}

	// Ok let them connect
	return qtrue;
}

/**
 * @brief Screening of userinfo keys and values
 *
 * @param[in] from
 * @param[in] userinfo
 */
static qboolean SV_IsValidUserinfo(netadr_t from, const char *userinfo)
{
	// FIXME: add some logging in for admins? we only do developer prints when a client is filtered
	int version;

	// NOTE: but we might need to store the protocol around for potential non http/ftp clients
	version = Q_atoi(Info_ValueForKey(userinfo, "protocol"));
	if (version != PROTOCOL_VERSION)
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_update]" PROTOCOL_MISMATCH_ERROR_LONG);
		Com_DPrintf("    rejected connect from version %i\n", version);

		return qfalse;
	}

	// validate userinfo to filter out the people blindly using hack code
	// FIXME: check existence of all keys and do some sanity checks
	//'\g_password\none\cl_guid\5XXXB6723XXXB\cl_wwwDownload\1\name\ETL player\rate\25000\snaps\20\protocol\84\qport\26708\challenge\1579001347'
	if (Info_ValueForKey(userinfo, "rate")[0] == 0)
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string); // Invalid connection! ... using temp ban msg :)
		Com_DPrintf("    rejected connect with wrong rate\n");

		return qfalse;
	}
	//else
	//{
	// FIXME: check rate in between 0 and sv_maxrate?
	//}

	// FIXME:
	//if (Info_ValueForKey(userinfo, "cl_wwwDownload")[0] == 0)
	// varify this value with server setting
	// we can send a proper message to clients when server has set up wwwdDownload and client
	// uses poor server download. Worst case: clients run into missing file issues later on and can't connect

	// FIXME: auth service?
	//if (Info_ValueForKey(userinfo, "g_password")[0] == 0)
	//if (Info_ValueForKey(userinfo, "name")[0] == 0)

	return qtrue;
}

/**
 * @brief Screening of user before 'real connection' and the client enters the world (using a slot)
 *
 * @param[in] from
 * @param[in] userinfo
 */
static qboolean SV_isValidClient(netadr_t from, const char *userinfo)
{
	// FIXME: add some logging in for admins? we only do developer prints when a client is filtered

	// inspect what we get as userinfo
	if (!SV_IsValidUserinfo(from, userinfo))
	{
		return qfalse;
	}

	// server bots are always valid (with valid userinfo)
	if (from.type == NA_BOT)
	{
		return qtrue;
	}

	if (SV_TempBanIsBanned(from))
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", sv_tempbanmessage->string);
		Com_DPrintf("    rejected connect with banned client\n");

		return qfalse;
	}

	// Too many connections from the same IP - don't permit the connection (if set)
	if (!SV_isClientIPValidToConnect(from))
	{
		if (sv_ipMaxClients->integer  == 1)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Only 1 connection per IP is allowed on this server!\n");
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Only %i connections per IP are allowed on this server!\n", sv_ipMaxClients->integer);
		}

		if (com_developer->integer)
		{
			Com_DPrintf("    rejected connect with max IP connection limit reached [%s]\n", NET_AdrToString(from));
		}

		return qfalse;
	}

	return qtrue;
}

/**
 * @brief SV_isValidGUID
 * @param[in] from
 * @param[in] userinfo
 * @return
 */
static qboolean SV_isValidGUID(netadr_t from, const char *userinfo)
{
	int  i;
	char guid[MAX_GUID_LENGTH + 1] = { 0 };

	Q_strncpyz(guid, Info_ValueForKey(userinfo, "cl_guid"), sizeof(guid));

	// don't allow empty, unknown or 'NO_GUID' guid
	if (strlen(guid) < MAX_GUID_LENGTH)
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Bad GUID: Invalid etkey. Please use the ET: Legacy client or add an etkey and set pb_cl_enable 1.\n");
		Com_DPrintf("Client rejected for bad sized etkey\n");
		return qfalse;
	}

	// check guid format
	for (i = 0; i < MAX_GUID_LENGTH; i++)
	{
		if (guid[i] < 48 || (guid[i] > 57 && guid[i] < 65) || guid[i] > 70)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Bad GUID: Invalid etkey.\n");
			Com_DPrintf("Client rejected for bad etkey\n");
			return qfalse;
		}
	}

	// don't check duplicate guid in developer mod
	if (!sv_cheats->integer)
	{
		client_t *cl;

		// check duplicate guid with validated clients
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer ; i++, cl++)
		{
			// don't check for bots GUID (empty) and player which are not fully connected
			// otherwise it could check for the reserved client slot which already contain
			// same client information trying to connect and who encounter latency / entering
			// password at the same time
			if (cl->state <= CS_PRIMED || cl->netchan.remoteAddress.type == NA_BOT)
			{
				continue;
			}

			if (!Q_strncmp(guid, cl->guid, MAX_GUID_LENGTH))
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Bad GUID: Duplicate etkey.\n");
				Com_DPrintf("Client rejected for duplicate etkey\n");
				return qfalse;
			}
		}
	}

	return qtrue;
}

/**
 * @brief A "connect" OOB command has been received
 *
 * @param[in] from
 */
void SV_DirectConnect(netadr_t from)
{
	char     userinfo[MAX_INFO_STRING];
	int      i, count = 0;
	client_t *cl, *newcl;
	client_t temp;
	int      clientNum;
	int      qport;
	int      challenge;
	char     *password;
	int      startIndex;
	char     *denied;

	Com_DPrintf("SVC_DirectConnect ()\n");

	// Prevent using connect as an amplifier
	if (sv_protect->integer & SVP_IOQ3)
	{
		if (SVC_RateLimitAddress(from, 10, 1000))
		{
			SV_WriteAttackLog(va("Bad direct connect - rate limit from %s exceeded, dropping request\n",
			                     NET_AdrToString(from)));
			return;
		}
	}

	Q_strncpyz(userinfo, Cmd_Argv(1), sizeof(userinfo));

	// sort out clients we don't want to have in game/does temp ban!
	if (!SV_isValidClient(from, userinfo))
	{
		return;
	}

	challenge = Q_atoi(Info_ValueForKey(userinfo, "challenge"));
	qport     = Q_atoi(Info_ValueForKey(userinfo, "qport"));

	// we don't need these keys after connection, release some space in userinfo
	Info_RemoveKey(userinfo, "challenge");
	Info_RemoveKey(userinfo, "qport");
	Info_RemoveKey(userinfo, "protocol");

	// quick reject
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		// This check was allowing clients to reconnect after zombietime(2 secs)
		//if ( cl->state == CS_FREE ) {
		//continue;
		//}

		if (NET_CompareBaseAdr(from, cl->netchan.remoteAddress)
		    && (cl->netchan.qport == qport || from.port == cl->netchan.remoteAddress.port))
		{
			if ((svs.time - cl->lastConnectTime) < sv_reconnectlimit->integer * 1000)
			{
				if (com_developer->integer)
				{
					Com_Printf("%s:reconnect rejected : too soon\n", NET_AdrToString(from));
				}
				return;
			}
			break;
		}
	}

	// see if the challenge is valid (local clients don't need to challenge)
	if (!NET_IsLocalAddress(from))
	{
		int ping;

		for (i = 0 ; i < MAX_CHALLENGES ; i++)
		{
			if (NET_CompareAdr(from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
				{
					break;      // good
				}
			}
		}
		if (i == MAX_CHALLENGES)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]No or bad challenge for address.\n");
			return;
		}
		// force the IP key/value pair so the game can filter based on ip
		Info_SetValueForKey(userinfo, "ip", NET_AdrToString(from));

		if (svs.challenges[i].firstPing == 0)
		{
			ping                        = svs.time - svs.challenges[i].pingTime;
			svs.challenges[i].firstPing = ping;
		}
		else
		{
			ping = svs.challenges[i].firstPing;
		}

		Com_Printf("Client %i connecting with %i challenge ping\n", i, ping);
		svs.challenges[i].connected = qtrue;

		// never reject a LAN client based on ping
		if (!Sys_IsLANAddress(from))
		{
			if (sv_minPing->value != 0.f && ping < sv_minPing->value)
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Server is for high pings only\n");
				Com_DPrintf("Client %i rejected on a too low ping\n", i);
				return;
			}
			if (sv_maxPing->value != 0.f && ping > sv_maxPing->value)
			{
				NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]Server is for low pings only\n");
				Com_DPrintf("Client %i rejected on a too high ping: %i\n", i, ping);
				return;
			}
		}
	}
	else
	{
		// force the "ip" info key to "localhost"
		Info_SetValueForKey(userinfo, "ip", "localhost");
	}

	newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (NET_CompareBaseAdr(from, cl->netchan.remoteAddress)
		    && (cl->netchan.qport == qport
		        || from.port == cl->netchan.remoteAddress.port))
		{
			Com_Printf("%s:reconnect\n", NET_AdrToString(from));
			newcl = cl;

			// this doesn't work because it nukes the players userinfo

			// disconnect the client from the game first so any flags the
			// player might have are dropped
			//VM_Call( gvm, GAME_CLIENT_DISCONNECT, newcl - svs.clients );

			goto gotnewcl;
		}
	}

	// check guid after we ensure client doesn't already use a slot
	if (sv_guidCheck->integer && !SV_isValidGUID(from, userinfo))
	{
		return;
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey(userinfo, "password");
	if (*password && !strcmp(password, sv_privatePassword->string))
	{
		startIndex = sv_democlients->integer;
	}
	else
	{
		// skip past the reserved slots
		startIndex = sv_privateClients->integer + sv_democlients->integer;
	}

	newcl = NULL;
	// check ALL slots for CS_FREE first
	for (i = startIndex; i < sv_maxclients->integer ; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == CS_FREE)
		{
			newcl = cl;
			break;
		}
	}

	// if there is no free slot avalable we prefer human players over bots
	// do a 2nd run and kick bots above startIndex slot
	if (!newcl)
	{
		for (i = startIndex; i < sv_maxclients->integer ; i++)
		{
			cl = &svs.clients[i];
			if (cl->netchan.remoteAddress.type == NA_BOT)
			{
				SV_DropClient(&svs.clients[i], "humans over robots!");
				newcl = &svs.clients[i];
				break;
			}
		}
	}

	// note: keep consistency - at this point bots might be still connected to private slots when a human player w/o password connects
	// but if we use a private slot for players w/o password the slot is no longer available for reserved players (see startIndex)
	if (!newcl)
	{
		if (NET_IsLocalAddress(from))
		{
			Com_Error(ERR_FATAL, "server is full on local connect"); // clarify: why do we have to abort here?
		}
		else
		{
			NET_OutOfBandPrint(NS_SERVER, from, va("print\n%s\n", sv_fullmsg->string));
			Com_DPrintf("Rejected a connection.\n");
		}

		return;
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->reliableAcknowledge = 0;
	cl->reliableSequence    = 0;

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is EVER initialized
	*newcl         = temp;
	clientNum      = newcl - svs.clients;
	newcl->gentity = SV_GentityNum(clientNum);

	newcl->gentity->r.svFlags = 0; // clear client flags on new connection.
	newcl->challenge          = challenge; // save the challenge
	Q_strncpyz(newcl->guid, Info_ValueForKey(userinfo, "cl_guid"), sizeof(newcl->guid)); // save guid

	// save the address
	Netchan_Setup(NS_SERVER, &newcl->netchan, from, qport);

	// init the netchan queue
	SV_Netchan_ClearQueue(newcl);

	// save the userinfo
	Q_strncpyz(newcl->userinfo, userinfo, sizeof(newcl->userinfo));

	// get the game a chance to reject this connection or modify the userinfo
	denied = (char *)(VM_Call(gvm, GAME_CLIENT_CONNECT, clientNum, qtrue, qfalse)); // firstTime = qtrue
	if (denied)
	{
		// we can't just use VM_ArgPtr, because that is only valid inside a VM_Call
		denied = VM_ExplicitArgPtr(gvm, (intptr_t)denied);

		NET_OutOfBandPrint(NS_SERVER, from, "print\n[err_dialog]%s\n", denied);
		Com_DPrintf("Game rejected a connection: %s\n", denied);
		return;
	}

	SV_UserinfoChanged(newcl);

	// Clear out firstPing now that client is connected
	svs.challenges[i].firstPing = 0;

	// send the connect packet to the client
	NET_OutOfBandPrint(NS_SERVER, from, "connectResponse");

	Com_DPrintf("Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name);

	newcl->state              = CS_CONNECTED;
	newcl->lastSnapshotTime   = 0;
	newcl->lastValidGamestate = 0;
	newcl->lastPacketTime     = svs.time;
	newcl->lastConnectTime    = svs.time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
		}
	}
	if (count == 1 || count == sv_maxclients->integer)
	{
		SV_Heartbeat_f();
	}

	// newcl->protocol = PROTOCOL_VERSION;
	newcl->protocol = Q_atoi(Info_ValueForKey(userinfo, "protocol"));

	// check client's engine version
	Com_ParseUA(&newcl->agent, Info_ValueForKey(userinfo, "etVersion"));
}

/**
 * @brief Check if the message has overflowed and if it has then drop the client.
 *
 * @param client[in,out] client to whon the message would be sent
 * @param msg[in,out] the message data which would be sent
 * @return did the message buffer overflow
 */
qboolean SV_CheckForMsgOverflow(client_t *client, msg_t *msg)
{
	if (!msg->overflowed)
	{
		return qfalse;
	}

	Com_Printf("WARNING: msg overflowed for %s\n", client->name);
	MSG_Clear(msg);

	SV_DropClient(client, "Msg overflowed");
	return qtrue;
}

/**
 * @brief Called when the player is totally leaving the server, either willingly
 * or unwillingly.  This is NOT called if the entire server is quiting
 * or crashing -- SV_FinalCommand() will handle that
 *
 * @param[in,out] drop
 * @param[in] reason
 *
 */
void SV_DropClient(client_t *drop, const char *reason)
{
	int         i;
	challenge_t *challenge;
	qboolean    isBot = qfalse;

	if (drop->state == CS_ZOMBIE)
	{
		return;     // already dropped
	}

	if (drop->gentity && (drop->gentity->r.svFlags & SVF_BOT))
	{
		isBot = qtrue;
	}
	else
	{
		if (drop->netchan.remoteAddress.type == NA_BOT)
		{
			isBot = qtrue;
		}
	}

	// Don't drop bots nor democlients (will make the server crash since there's no network connection to manage with these clients!)
	if (!isBot && !drop->demoClient)
	{
		// see if we already have a challenge for this ip
		challenge = &svs.challenges[0];

		for (i = 0 ; i < MAX_CHALLENGES ; i++, challenge++)
		{
			if (NET_CompareAdr(drop->netchan.remoteAddress, challenge->adr))
			{
				challenge->connected = qfalse;
				break;
			}
		}

		SV_Netchan_ClearQueue(drop);
	}

	if (!isBot)
	{
		// tell everyone why they got dropped
		// we want this displayed elsewhere now
		SV_SendServerCommand(NULL, "cpm \"%s" S_COLOR_WHITE " %s\"\n", rc(drop->name), reason);
	}

	Com_DPrintf("Going to CS_ZOMBIE for %s\n", drop->name);
	drop->state = CS_ZOMBIE;        // become free in a few seconds

	// Kill any download
	SV_CloseDownload(drop);

	// call the prog function for removing a client
	// this will remove the body, among other things
	VM_Call(gvm, GAME_CLIENT_DISCONNECT, drop - svs.clients);

	// add the disconnect command
	SV_SendServerCommand(drop, "disconnect \"%s\"\n", reason);

	if (drop->netchan.remoteAddress.type == NA_BOT)
	{
		SV_BotFreeClient(drop - svs.clients);
	}

	// nuke user info
	SV_SetUserinfo(drop - svs.clients, "");

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for (i = 0 ; i < sv_maxclients->integer ; i++)
	{
		// we check real players slots: if real players slots are all empty (not counting democlients), we send an heartbeat to update
		if (svs.clients[i].state >= CS_CONNECTED && !svs.clients[i].demoClient)
		{
			break;
		}
	}
	if (i == sv_maxclients->integer)
	{
		SV_Heartbeat_f();
	}
}

/**
 * @brief Sends the first message from the server to a connected client.
 * This will be sent on the initial connection and upon each new map load.
 * It will be resent if the client acknowledges a later message but has
 * the wrong gamestate.
 *
 * @param[in,out] client
 */
void SV_SendClientGameState(client_t *client)
{
	int           start;
	entityState_t *base, nullstate;
	msg_t         msg;
	byte          msgBuffer[MAX_MSGLEN];

	Com_DPrintf("SV_SendClientGameState() for %s\n", client->name);

	if (client->state != CS_PRIMED)
	{
		Com_DPrintf("Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name);
	}

	client->state         = CS_PRIMED;
	client->pureAuthentic = 0;
	client->gotCP         = qfalse;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	MSG_Init(&msg, msgBuffer, sizeof(msgBuffer));

	// NOTE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong(&msg, client->lastClientCommand);

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient(client, &msg);

	// send the gamestate
	MSG_WriteByte(&msg, svc_gamestate);
	MSG_WriteLong(&msg, client->reliableSequence);

	// write the configstrings
	for (start = 0 ; start < MAX_CONFIGSTRINGS ; start++)
	{
		if (sv.configstrings[start][0])
		{
			MSG_WriteByte(&msg, svc_configstring);
			MSG_WriteShort(&msg, start);
			MSG_WriteBigString(&msg, sv.configstrings[start]);
		}
	}

	// write the baselines
	Com_Memset(&nullstate, 0, sizeof(nullstate));
	for (start = 0 ; start < MAX_GENTITIES; start++)
	{
		base = &sv.svEntities[start].baseline;
		if (!base->number)
		{
			continue;
		}

		MSG_WriteByte(&msg, svc_baseline);
		MSG_WriteDeltaEntity(&msg, &nullstate, base, qtrue);
	}

	MSG_WriteByte(&msg, svc_EOF);

	MSG_WriteLong(&msg, client - svs.clients);

	// write the checksum feed
	MSG_WriteLong(&msg, sv.checksumFeed);

	if (SV_CheckForMsgOverflow(client, &msg))
	{
		return;
	}

	// debug info
	Com_DPrintf("Sending %i bytes in gamestate to client: %i\n", msg.cursize, (int) (client - svs.clients));

	// deliver this to the client
	SV_SendMessageToClient(&msg, client);
}

/**
 * @brief SV_ClientEnterWorld
 * @param[in,out] client
 * @param[in] cmd
 */
void SV_ClientEnterWorld(client_t *client, usercmd_t *cmd)
{
	int            clientNum = client - svs.clients;
	sharedEntity_t *ent;

	Com_DPrintf("Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name);
	client->state = CS_ACTIVE;

	// don't timeout when client enters the world (after bigger download times > sv_timeout->integer and SV_Netchan_Process inactivity)
	// actually this resets sv_timeout/_dl vars on map changes
	client->lastPacketTime = svs.time;

	// server-side demo playback: prevent players from joining the game when a demo is replaying (particularly if the gametype is non-team based, by default the gamecode force players to join in)
	if (sv.demoState == DS_PLAYBACK &&
	    ((client - svs.clients) >= sv_democlients->integer) && ((client - svs.clients) < sv_maxclients->integer))        // check that it's a real player
	{
		SV_ExecuteClientCommand(client, "team spectator", qtrue, qfalse);
	}

	// set up the entity for the client
	ent             = SV_GentityNum(clientNum);
	ent->s.number   = clientNum;
	client->gentity = ent;

	client->deltaMessage     = -1;
	client->lastSnapshotTime = 0;   // generate a snapshot immediately

	if (cmd)
	{
		Com_Memcpy(&client->lastUsercmd, cmd, sizeof(client->lastUsercmd));
	}
	else
	{
		Com_Memset(&client->lastUsercmd, '\0', sizeof(client->lastUsercmd));
	}

	// call the game begin function
	VM_Call(gvm, GAME_CLIENT_BEGIN, client - svs.clients);
}

/*
============================================================
CLIENT COMMAND EXECUTION
============================================================
*/

/**
 * @brief Clear/free any download vars
 * @param[in,out] cl
 */
static void SV_CloseDownload(client_t *cl)
{
	int i;

	// EOF
	if (cl->download)
	{
		FS_FCloseFile(cl->download);
	}
	cl->download      = 0;
	*cl->downloadName = 0;

	// don't timeout after download for valid clients
	// so SV_CheckTimeouts doesn't drop when we switch related timeout cvar
	if (cl->state > CS_ZOMBIE)
	{
		cl->lastPacketTime = svs.time;
	}

	// Free the temporary buffer space
	for (i = 0; i < MAX_DOWNLOAD_WINDOW; i++)
	{
		if (cl->downloadBlocks[i])
		{
			Z_Free(cl->downloadBlocks[i]);
			cl->downloadBlocks[i] = NULL;
		}
	}
}

/**
 * @brief Abort a download if in progress
 * @param[in] cl
 */
static void SV_StopDownload_f(client_t *cl)
{
	if (*cl->downloadName)
	{
		Com_Printf("clientDownload: %d : file \"%s\" aborted\n", (int) (cl - svs.clients), cl->downloadName);
	}

	SV_CloseDownload(cl);
}

/**
 * @brief Downloads are finished
 * @param[in] cl
 */
static void SV_DoneDownload_f(client_t *cl)
{
	if (cl->state == CS_ACTIVE)
	{
		Com_Printf("Warning: SV_DoneDownload_f called for '%s' and client state is already active\n", rc(cl->name));
		return;
	}

	// don't timeout after download for valid clients
	// so SV_CheckTimeouts doesn't drop when we switch related timeout cvar
	if (cl->state > CS_ZOMBIE)
	{
		cl->lastPacketTime = svs.time;
	}

	// resend the game state to update any clients that entered during the download
	SV_SendClientGameState(cl);
}

/**
 * @brief The argument will be the last acknowledged block from the client, it should be
 * the same as cl->downloadClientBlock
 * @param[in,out] cl
 */
void SV_NextDownload_f(client_t *cl)
{
	int block = Q_atoi(Cmd_Argv(1));

	if (block == cl->downloadClientBlock)
	{
		Com_DPrintf("clientDownload: %d : client acknowledge of block %d\n", (int) (cl - svs.clients), block);

		// Find out if we are done.  A zero-length block indicates EOF
		if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0)
		{
			Com_Printf("clientDownload: %d : file \"%s\" completed\n", (int) (cl - svs.clients), cl->downloadName);
			SV_CloseDownload(cl);
			return;
		}

		cl->downloadAckTime = svs.time;
		cl->downloadClientBlock++;
		return;
	}

	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//          because the cgame isn't loaded yet
	SV_DropClient(cl, "broken download");
}

/**
 * @brief SV_BeginDownload_f
 * @param[in,out] cl
 */
void SV_BeginDownload_f(client_t *cl)
{
	// Kill any existing download
	SV_CloseDownload(cl);

	// stop us from printing dupe messages
	if (strcmp(cl->downloadName, Cmd_Argv(1)))
	{
		cl->downloadnotify = DLNOTIFY_ALL;
	}

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	Q_strncpyz(cl->downloadName, Cmd_Argv(1), sizeof(cl->downloadName));
}

/**
 * @brief SV_WWWDownload_f
 * @param[in,out] cl
 */
void SV_WWWDownload_f(client_t *cl)
{
	char *subcmd = Cmd_Argv(1);

	// only accept wwwdl commands for clients which we first flagged as wwwdl ourselves
	if (!cl->bWWWDl)
	{
		Com_Printf("SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, rc(cl->name));
		SV_DropClient(cl, va("SV_WWWDownload: unexpected wwwdl %s", subcmd));
		return;
	}

	if (!Q_stricmp(subcmd, "ack"))
	{
		if (cl->bWWWing)
		{
			Com_Printf("WARNING: dupe wwwdl ack from client '%s'\n", rc(cl->name));
		}

		cl->bWWWing = qtrue;
		return;
	}
	else if (!Q_stricmp(subcmd, "bbl8r"))
	{
		SV_DropClient(cl, "acking disconnected download mode");
		return;
	}

	// below for messages that only happen during/after download
	if (!cl->bWWWing)
	{
		Com_Printf("SV_WWWDownload: unexpected wwwdl '%s' for client '%s'\n", subcmd, rc(cl->name));
		SV_DropClient(cl, va("SV_WWWDownload: unexpected wwwdl %s", subcmd));
		return;
	}

	if (!Q_stricmp(subcmd, "done"))
	{
		SV_CloseDownload(cl);

		cl->bWWWing = qfalse;
		return;
	}
	else if (!Q_stricmp(subcmd, "fail"))
	{
		Com_Printf("Warning: Client dowbnload failed.\n");
		SV_CloseDownload(cl);
		cl->bWWWing   = qfalse;
		cl->bFallback = qtrue;
		// send a reconnect
		SV_SendClientGameState(cl);
		return;
	}
	else if (!Q_stricmp(subcmd, "chkfail"))
	{
		Com_Printf("WARNING: client '%s' reports that the redirect download for '%s' had wrong checksum.\n", rc(cl->name), cl->downloadName);
		Com_Printf("         you should check your download redirect configuration.\n");
		SV_CloseDownload(cl);
		cl->bWWWing   = qfalse;
		cl->bFallback = qtrue;
		// send a reconnect
		SV_SendClientGameState(cl);
		return;
	}

	Com_Printf("SV_WWWDownload: unknown wwwdl subcommand '%s' for client '%s'\n", subcmd, rc(cl->name));
	SV_DropClient(cl, va("SV_WWWDownload: unknown wwwdl subcommand '%s'", subcmd));
}

/**
 * @brief Abort an attempted download
 * @param[in] cl
 * @param[in] msg
 */
void SV_BadDownload(client_t *cl, msg_t *msg)
{
	MSG_WriteByte(msg, svc_download);
	MSG_WriteShort(msg, 0);   // client is expecting block zero
	MSG_WriteLong(msg, -1);   // illegal file size

	SV_CloseDownload(cl);
}

/**
 * @brief sv_wwwFallbackURL can be used to redirect clients to a web URL in case direct ftp/http didn't work (or is disabled on client's end)
 *
 * @param[in] cl
 * @param[in] msg
 *
 * @return return true when a redirect URL message was filled up
 * when the cvar is set to something, the download server will effectively never use a legacy download strategy
 */
static qboolean SV_CheckFallbackURL(client_t *cl, msg_t *msg)
{
	if (!sv_wwwFallbackURL->string || strlen(sv_wwwFallbackURL->string) == 0)
	{
		return qfalse;
	}

	Com_Printf("clientDownload: sending client '%s' to fallback URL '%s'\n", rc(cl->name), sv_wwwFallbackURL->string);

	MSG_WriteByte(msg, svc_download);
	MSG_WriteShort(msg, DLTYPE_WWW);
	MSG_WriteString(msg, sv_wwwFallbackURL->string);
	MSG_WriteLong(msg, 0);
	MSG_WriteLong(msg, 2);   // DL_FLAG_URL

	return qtrue;
}

/**
 * @brief Check if we are able to share the file
 * @param[in] cl
 * @param[in] msg
 * @return
 */
static qboolean SV_CheckDownloadAllowed(client_t *cl, msg_t *msg)
{
	char errorMessage[1024];
	int  idPack = FS_idPak(cl->downloadName, BASEGAME);

	// sv_allowDownload and idPack checks
	if (!sv_allowDownload->integer || idPack)
	{
		// cannot auto-download file
		if (idPack)
		{
			Com_Printf("clientDownload: %d : \"%s\" cannot download id pk3 files\n", (int)(cl - svs.clients), cl->downloadName);
			Com_sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload official pk3 file \"%s\"", cl->downloadName);
		}
		else
		{
			Com_Printf("clientDownload: %d : \"%s\" download disabled", (int)(cl - svs.clients), cl->downloadName);
			if (sv_pure->integer)
			{
				Com_sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
				                                                "You will need to get this file elsewhere before you "
				                                                "can connect to this pure server.\n", cl->downloadName);
			}
			else
			{
				Com_sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
				                                                "Set autodownload to No in your settings and you might be "
				                                                "able to connect even if you don't have the file.\n", cl->downloadName);
			}
		}

		SV_BadDownload(cl, msg);
		MSG_WriteString(msg, errorMessage);   // (could SV_DropClient isntead?)

		return qfalse;
	}

	return qtrue;
}

/**
 * @brief We open the file here
 * @param[in,out] cl
 * @param[in] msg
 * @return
 */
static qboolean SV_SetupDownloadFile(client_t *cl, msg_t *msg)
{
	int          download_flag;
	fileHandle_t downloadFileHandle = 0;
	int          downloadSize       = 0;

	// prevent duplicate download notifications
	if (cl->downloadnotify & DLNOTIFY_BEGIN)
	{
		cl->downloadnotify &= ~DLNOTIFY_BEGIN;
		Com_Printf("clientDownload: %d : beginning \"%s\"\n", (int)(cl - svs.clients), cl->downloadName);
	}

	// Check if we allow downloading of the package
	if (!SV_CheckDownloadAllowed(cl, msg))
	{
		// return true so the error message gets sent
		return qtrue;
	}

	downloadSize = FS_SV_FOpenFileRead(cl->downloadName, &downloadFileHandle);
	if (downloadSize <= 0)
	{
		Com_Printf("clientDownload: %d : \"%s\" file not found on server\n", (int)(cl - svs.clients), cl->downloadName);
		SV_BadDownload(cl, msg);
		SV_DropClient(cl, va("File \"%s\" not found on server for autodownloading.\n", cl->downloadName));
		return qtrue;
	}

	// www download redirect protocol
	// NOTE: this is called repeatedly while a client connects. Maybe we should sort of cache the message or something
	// FIXME: we need to abstract this to an independant module for maximum configuration/usability by server admins
	// FIXME: I could rework that, it's crappy
	if (sv_wwwDownload->integer)
	{
		if (cl->bDlOK)
		{
			if (!cl->bFallback)
			{
				FS_FCloseFile(downloadFileHandle);   // don't keep open, we only care about the size

				Q_strncpyz(cl->downloadURL, va("%s/%s", sv_wwwBaseURL->string, cl->downloadName), sizeof(cl->downloadURL));

				// prevent multiple download notifications
				if (cl->downloadnotify & DLNOTIFY_REDIRECT)
				{
					cl->downloadnotify &= ~DLNOTIFY_REDIRECT;
					Com_Printf("Redirecting client '%s' to %s\n", rc(cl->name), cl->downloadURL);
				}
				// once cl->downloadName is set (and possibly we have our listening socket), let the client know
				cl->bWWWDl = qtrue;
				MSG_WriteByte(msg, svc_download);
				MSG_WriteShort(msg, DLTYPE_WWW);
				// compatible with legacy svc_download protocol: [size] [size bytes]
				// download URL, size of the download file, download flags
				MSG_WriteString(msg, cl->downloadURL);
				MSG_WriteLong(msg, downloadSize);
				download_flag = 0;
				if (sv_wwwDlDisconnected->integer)
				{
					download_flag |= (1 << DL_FLAG_DISCON);
				}

				MSG_WriteLong(msg, download_flag);   // flags

				// set the download ack time for http downloads
				cl->downloadAckTime = svs.time;

				return qtrue;
			}
			else
			{
				cl->bFallback = qfalse;
				if (SV_CheckFallbackURL(cl, msg))
				{
					FS_FCloseFile(downloadFileHandle);
					return qtrue;
				}

				Com_Printf("Client '%s': falling back to regular downloading for failed file %s\n", rc(cl->name), cl->downloadName);
			}
		}
		else
		{
			if (SV_CheckFallbackURL(cl, msg))
			{
				FS_FCloseFile(downloadFileHandle);
				return qtrue;
			}

			Com_Printf("Client '%s' is not configured for www download\n", rc(cl->name));
		}
	}

	cl->bWWWDl       = qfalse;
	cl->download     = downloadFileHandle;
	cl->downloadSize = downloadSize;

	// is valid source, init
	cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
	cl->downloadCount        = 0;
	cl->downloadEOF          = qfalse;

	// We reset the ack time to current when we start
	cl->downloadAckTime = svs.time;

	return qfalse;
}

/**
 * @brief Check to see if the client wants a file, open it if needed and start pumping the client
 * @param[in,out] cl
 * @param[in] msg
 * @return
 */
static qboolean SV_WriteDownloadToClient(client_t *cl, msg_t *msg)
{
	int curindex;

	if (!*cl->downloadName)
	{
		return qfalse; // Nothing being downloaded
	}
	if (cl->bWWWing)
	{
		return qfalse; // The client acked and is downloading with ftp/http
	}

	// CVE-2006-2082: validate the download against the list of pak files
	if (!FS_VerifyPak(cl->downloadName))
	{
		// will drop the client and leave it hanging on the other side. good for him
		SV_DropClient(cl, "illegal download request");
		return qfalse;
	}

	// set up the file to be downloaded
	if (!cl->download && SV_SetupDownloadFile(cl, msg))
	{
		return qtrue;
	}

	// Perform any reads that we need to
	while (cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW && cl->downloadSize != cl->downloadCount)
	{
		curindex = (cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW);

		if (!cl->downloadBlocks[curindex])
		{
			cl->downloadBlocks[curindex] = Z_Malloc(MAX_DOWNLOAD_BLKSIZE);
		}

		cl->downloadBlockSize[curindex] = FS_Read(cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download);

		if (cl->downloadBlockSize[curindex] < 0)
		{
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[curindex];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if (cl->downloadCount == cl->downloadSize && !cl->downloadEOF &&
	    cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW)
	{

		cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = qtrue;  // We have added the EOF block
	}

	if (cl->downloadClientBlock == cl->downloadCurrentBlock)
	{
		return qfalse; // Nothing to transmit
	}

	// Write out the next section of the file, if we have already reached our window,
	// automatically start retransmitting
	if (cl->downloadXmitBlock == cl->downloadCurrentBlock)
	{
		// We have transmitted the complete window, should we start resending?
		if (svs.time - cl->downloadSendTime > 1000)
		{
			cl->downloadXmitBlock = cl->downloadClientBlock;
		}
		else
		{
			return qfalse;
		}
	}

	// Send current block
	curindex = (cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW);

	MSG_WriteByte(msg, svc_download);
	MSG_WriteShort(msg, cl->downloadXmitBlock);

	// block zero is special, contains file size
	if (cl->downloadXmitBlock == 0)
	{
		MSG_WriteLong(msg, cl->downloadSize);
	}

	MSG_WriteShort(msg, cl->downloadBlockSize[curindex]);

	// Write the block
	if (cl->downloadBlockSize[curindex])
	{
		MSG_WriteData(msg, cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex]);
	}

	Com_DPrintf("clientDownload: %d : writing block %d\n", (int)(cl - svs.clients), cl->downloadXmitBlock);

	// Move on to the next block
	// It will get sent with next snap shot.  The rate will keep us in line.
	cl->downloadXmitBlock++;
	cl->downloadSendTime = svs.time;

	return qtrue;
}

/**
 * @brief Send one round of fragments, or queued messages to all clients that have data pending.
 * @return The shortest time interval for sending next packet to client
 */
int SV_SendQueuedMessages(void)
{
	int      i, retval = -1, nextFragT;
	client_t *cl;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];

		if (cl->state)
		{
			nextFragT = SV_RateMsec(cl);

			if (!nextFragT)
			{
				nextFragT = SV_Netchan_TransmitNextFragment(cl);
			}

			if (nextFragT >= 0 && (retval == -1 || retval > nextFragT))
			{
				retval = nextFragT;
			}
		}
	}

	return retval;
}

/**
 * @brief Send one round of download messages to all clients
 * @return
 */
int SV_SendDownloadMessages(void)
{
	int      i, numDLs = 0;
	client_t *cl;
	msg_t    msg;
	byte     msgBuffer[MAX_MSGLEN];

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];

		if (cl->state && *cl->downloadName)
		{
			Com_Memset(&msgBuffer, 0, MAX_MSGLEN);

			MSG_Init(&msg, msgBuffer, sizeof(msgBuffer));
			MSG_WriteLong(&msg, cl->lastClientCommand);

			if (SV_WriteDownloadToClient(cl, &msg))
			{
				SV_Netchan_Transmit(cl, &msg);
				numDLs++;
			}
		}
	}

	return numDLs;
}

/**
 * @brief The client is going to disconnect, so remove the connection immediately
 * @param[in] cl
 *
 * @todo FIXME: move to game?
 */
static void SV_Disconnect_f(client_t *cl)
{
	SV_DropClient(cl, "disconnected");
}

/**
 * @brief SV_VerifyPaks_f
 * @details
 * If we are pure, disconnect the client if they do no meet the following conditions:
 *
 * 1. the first two checksums match our view of cgame and ui DLLs
 * Wolf specific: the checksum is the checksum of the pk3 we found the DLL in
 * 2. there are no any additional checksums that we do not have
 *
 * This routine would be a bit simpler with a goto but i abstained
 *
 * @param[in,out] cl
 */
static void SV_VerifyPaks_f(client_t *cl)
{
	int        nChkSum1, nChkSum2;
	int        nClientChkSum[1024];
	int        nServerChkSum[1024];
	const char *pPaks, *pArg;

	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	if (sv_pure->integer != 0)
	{
		int      nClientPaks, nServerPaks, i, j, nCurArg;
		qboolean bGood;

		nChkSum1 = nChkSum2 = 0;

		bGood = (FS_FileIsInPAK(Sys_GetDLLName("cgame"), &nChkSum1) == 1);
		if (bGood)
		{
			bGood = (FS_FileIsInPAK(Sys_GetDLLName("ui"), &nChkSum2) == 1);
		}

		nClientPaks = Cmd_Argc();

		// start at arg 2 ( skip serverId cl_paks )
		nCurArg = 1;

		pArg = Cmd_Argv(nCurArg++);

		if (!pArg)
		{
			bGood = qfalse;
		}
		else
		{
			// we may get incoming cp sequences from a previous checksumFeed, which we need to ignore
			// since serverId is a frame count, it always goes up
			if (atoi(pArg) < sv.checksumFeedServerId)
			{
				Com_DPrintf("ignoring outdated cp command from client %s\n", rc(cl->name));
				return;
			}
		}

		// we basically use this while loop to avoid using 'goto' :)
		while (bGood)
		{
			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if (nClientPaks < 6)
			{
				bGood = qfalse;
				break;
			}
			// verify first to be the cgame checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || Q_atoi(pArg) != nChkSum1)
			{
				Com_Printf("nChkSum1 %d == %d\n", Q_atoi(pArg), nChkSum1);
				bGood = qfalse;
				break;
			}
			// verify the second to be the ui checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || Q_atoi(pArg) != nChkSum2)
			{
				Com_Printf("nChkSum2 %d == %d\n", Q_atoi(pArg), nChkSum2);
				bGood = qfalse;
				break;
			}
			// should be sitting at the delimeter now
			pArg = Cmd_Argv(nCurArg++);
			if (*pArg != '@')
			{
				bGood = qfalse;
				break;
			}
			// store checksums since tokenization is not re-entrant
			for (i = 0; nCurArg < nClientPaks; i++)
			{
				nClientChkSum[i] = Q_atoi(Cmd_Argv(nCurArg++));
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for (i = 0; i < nClientPaks; i++)
			{
				for (j = 0; j < nClientPaks; j++)
				{
					if (i == j)
					{
						continue;
					}
					if (nClientChkSum[i] == nClientChkSum[j])
					{
						bGood = qfalse;
						break;
					}
				}
				if (bGood == qfalse)
				{
					break;
				}
			}
			if (bGood == qfalse)
			{
				break;
			}

			// get the pure checksums of the pk3 files loaded by the server
			pPaks = FS_LoadedPakPureChecksums();
			Cmd_TokenizeString(pPaks);
			nServerPaks = Cmd_Argc();
			if (nServerPaks > 1024)
			{
				nServerPaks = 1024;
			}

			for (i = 0; i < nServerPaks; i++)
			{
				nServerChkSum[i] = Q_atoi(Cmd_Argv(i));
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for (i = 0; i < nClientPaks; i++)
			{
				for (j = 0; j < nServerPaks; j++)
				{
					if (nClientChkSum[i] == nServerChkSum[j])
					{
						break;
					}
				}
				if (j > nServerPaks)
				{
					bGood = qfalse;
					break;
				}
			}
			if (bGood == qfalse)
			{
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = sv.checksumFeed;
			for (i = 0; i < nClientPaks; i++)
			{
				nChkSum1 ^= nClientChkSum[i];
			}
			nChkSum1 ^= nClientPaks;
			if (nChkSum1 != nClientChkSum[nClientPaks])
			{
				bGood = qfalse;
				break;
			}

			// break out
			break;
		}

		cl->gotCP = qtrue;

		if (bGood)
		{
			cl->pureAuthentic = 1;
		}
		else
		{
			cl->pureAuthentic    = 0;
			cl->lastSnapshotTime = 0;
			cl->state            = CS_ACTIVE;
			SV_SendClientSnapshot(cl);
			SV_DropClient(cl, "Unpure client detected. Invalid .PK3 files referenced!");
		}
	}
}

/**
 * @brief SV_ResetPureClient_f
 * @param[out] cl
 */
static void SV_ResetPureClient_f(client_t *cl)
{
	cl->pureAuthentic = 0;
	cl->gotCP         = qfalse;
}

/**
 * @brief Pull specific info from a newly changed userinfo string into a more C friendly form.
 * @param[in,out] cl
 */
void SV_UserinfoChanged(client_t *cl)
{
	char *val;
	int  i;

	// name for C code
	Q_strncpyz(cl->name, Info_ValueForKey(cl->userinfo, "name"), sizeof(cl->name));

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// internet public server, assume they don't need a rate choke
	if (Sys_IsLANAddress(cl->netchan.remoteAddress) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1)
	{
		cl->rate = 99999;   // lans should not rate limit
	}
	else
	{
		val = Info_ValueForKey(cl->userinfo, "rate");
		if (strlen(val))
		{
			i        = Q_atoi(val);
			cl->rate = i;
			if (cl->rate < 1000)
			{
				cl->rate = 1000;
			}
			else if (cl->rate > 90000)
			{
				cl->rate = 90000;
			}
		}
		else
		{
			cl->rate = 5000;
		}
	}

	/*
	val = Info_ValueForKey(cl->userinfo, "handicap");
	if (strlen(val))
	{
	    i = Q_atoi(val);
	    if (i <= -100 || i > 100 || strlen(val) > 4)
	    {
	        Info_SetValueForKey(cl->userinfo, "handicap", "0");
	    }
	}
	*/

	// snaps command
	val = Info_ValueForKey(cl->userinfo, "snaps");
	if (strlen(val))
	{
		i = Q_atoi(val);
		if (i < 1)
		{
			i = 1;
		}
		else if (i > sv_fps->integer)
		{
			i = sv_fps->integer;
		}

		i = 1000 / i;
	}
	else
	{
		i = 50; // 1000 / sv_fps->integer
	}

	if (i != cl->snapshotMsec)
	{
		// Reset last sent snapshot so we avoid desync between server frame time and snapshot send time
		cl->lastSnapshotTime = 0;
		cl->snapshotMsec     = i;
	}

	// maintain the IP information
	// this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
	// the banning code relies on this being consistently present
	// - modified to always keep this consistent, instead of only
	// when "ip" is 0-length, so users can't supply their own IP
	//Com_DPrintf("Maintain IP in userinfo for '%s'\n", cl->name);
	if (!NET_IsLocalAddress(cl->netchan.remoteAddress))
	{
		Info_SetValueForKey(cl->userinfo, "ip", NET_AdrToString(cl->netchan.remoteAddress));
	}
	else
	{
		// force the "ip" info key to "localhost" for local clients
		Info_SetValueForKey(cl->userinfo, "ip", "localhost");
	}

	// download prefs of the client
	val       = Info_ValueForKey(cl->userinfo, "cl_wwwDownload");
	cl->bDlOK = qfalse;
	if (strlen(val))
	{
		i = Q_atoi(val);
		if (i != 0)
		{
			cl->bDlOK = qtrue;
		}
	}

	// no version was set on connect, check cgame version as a fallback
	if (cl->agent.string[0] == 0 && (val = Info_ValueForKey(cl->userinfo, "cg_etVersion"))[0])
	{
		Com_ParseUA(&cl->agent, val);
	}
}

/**
 * @brief SV_UpdateUserinfo_f
 * @param[in] cl
 */
void SV_UpdateUserinfo_f(client_t *cl)
{
	char *arg = Cmd_Argv(1);
	// Stop random empty /userinfo calls without hurting anything
	if (!arg || !*arg)
	{
		return;
	}

	if ((sv_floodProtect->integer) && (cl->state >= CS_ACTIVE) && (svs.time < cl->nextReliableUserTime))
	{
		Q_strncpyz(cl->userinfobuffer, arg, sizeof(cl->userinfobuffer));
		SV_SendServerCommand(cl, "print \"^7Command ^1delayed^7 due to sv_floodprotect.\n\"");
		return;
	}
	cl->userinfobuffer[0]    = 0;
	cl->nextReliableUserTime = svs.time + 5000;

	// Save userinfo changes to demo (also in SV_SetUserinfo() in sv_init.c)
	if (sv.demoState == DS_RECORDING)
	{
		SV_DemoWriteClientUserinfo(cl, arg);
	}

	Q_strncpyz(cl->userinfo, arg, sizeof(cl->userinfo));

	SV_UserinfoChanged(cl);
	// call prog code to allow overrides
	VM_Call(gvm, GAME_CLIENT_USERINFO_CHANGED, cl - svs.clients);
}

typedef struct
{
	char *name;
	void (*func)(client_t *cl);
	qboolean allowedpostmapchange;
} ucmd_t;

static ucmd_t ucmds[] =
{
	{ "userinfo",   SV_UpdateUserinfo_f,  qfalse },
	{ "disconnect", SV_Disconnect_f,      qtrue  },
	{ "cp",         SV_VerifyPaks_f,      qfalse },
	{ "vdr",        SV_ResetPureClient_f, qfalse },
	{ "download",   SV_BeginDownload_f,   qfalse },
	{ "nextdl",     SV_NextDownload_f,    qfalse },
	{ "stopdl",     SV_StopDownload_f,    qfalse },
	{ "donedl",     SV_DoneDownload_f,    qfalse },
	{ "wwwdl",      SV_WWWDownload_f,     qfalse },
	{ NULL,         NULL,                 qfalse }
};

/**
 * @brief Also called by bot code
 * @param[in] cl
 * @param[in] s
 * @param[in] clientOK
 * @param[in] premaprestart
 */
void SV_ExecuteClientCommand(client_t *cl, const char *s, qboolean clientOK, qboolean premaprestart)
{
	ucmd_t   *u;
	qboolean bProcessed = qfalse;

	Cmd_TokenizeString(s);

	// see if it is a server level command
	for (u = ucmds ; u->name ; u++)
	{
		if (!strcmp(Cmd_Argv(0), u->name))
		{
			if (premaprestart && !u->allowedpostmapchange)
			{
				continue;
			}

			u->func(cl);
			bProcessed = qtrue;
			break;
		}
	}

	if (clientOK)
	{
		// pass unknown strings to the game
		// accept democlients, else you won't be able to make democlients join teams nor say messages!
		if (!u->name && sv.state == SS_GAME && (cl->state == CS_ACTIVE || cl->state == CS_PRIMED || cl->demoClient))
		{
			if (!SV_DemoClientCommandCapture(cl, s))
			{
				return;
			}

			if (cl->state < CS_ACTIVE && !Q_strncmp(Cmd_Argv(0), "say", 3))
			{
				// Do not allow spamming of messages if the client is not active
				Com_DPrintf("client spam ignored for %s\n", rc(cl->name));
				return;
			}

			Cmd_Args_Sanitize();
			VM_Call(gvm, GAME_CLIENT_COMMAND, cl - svs.clients);
		}
	}
	else if (!bProcessed)
	{
		Com_DPrintf("client text ignored for %s: %s\n", rc(cl->name), Cmd_Argv(0));
	}
}

/**
 * @brief SV_ClientCommand
 * @param[in,out] cl
 * @param[in] msg
 * @param[in] premaprestart
 * @return
 */
static qboolean SV_ClientCommand(client_t *cl, msg_t *msg, qboolean premaprestart)
{
	int        seq;
	const char *s;
	qboolean   clientOk     = qtrue;
	qboolean   floodprotect = qtrue;

	seq = MSG_ReadLong(msg);
	s   = MSG_ReadString(msg);

	// see if we have already executed it
	if (cl->lastClientCommand >= seq)
	{
		return qtrue;
	}

	Com_DPrintf("clientCommand: %s : %i : %s\n", rc(cl->name), seq, s);

	// drop the connection if there are issues with reading messages
	if (seq < 0 || s[0] == 0) // invalid MSG_Read
	{
		Com_Printf("Client %s dropped for invalid client command message\n", rc(cl->name));
		SV_DropClient(cl, "Invalid client command message");
		return qfalse;
	}

	// drop the connection if we have somehow lost commands
	if (seq > cl->lastClientCommand + 1)
	{
		Com_Printf("Client %s lost %i clientCommands\n", rc(cl->name), seq - cl->lastClientCommand + 1);
		SV_DropClient(cl, "Lost reliable commands");
		return qfalse;
	}

	// AHA! Need to steal this for some other stuff BOOKMARK
	// - some server game-only commands we cannot have flood protect
	if (!Q_strncmp("team", s, 4) || !Q_strncmp("setspawnpt", s, 10) || !Q_strncmp("score", s, 5) || !Q_stricmp("forcetapout", s))
	{
		//Com_DPrintf( "Skipping flood protection for: %s\n", s );
		floodprotect = qfalse;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	if (!com_cl_running->integer &&
	    cl->state >= CS_ACTIVE &&          // this was commented out in Wolf.  Did we do that?
	    sv_floodProtect->integer &&
	    svs.time < cl->nextReliableTime &&
	    floodprotect)
	{
		// ignore any other text messages from this client but let them keep playing
		// moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = qfalse;
	}

	// don't allow another command for 800 msec
	if (floodprotect &&
	    svs.time >= cl->nextReliableTime)
	{
		cl->nextReliableTime = svs.time + 800;
	}

	SV_ExecuteClientCommand(cl, s, clientOk, premaprestart);

	cl->lastClientCommand = seq;
	Com_sprintf(cl->lastClientCommandString, sizeof(cl->lastClientCommandString), "%s", s);

	return qtrue;       // continue procesing
}

//==================================================================================

/**
 * @brief  Also called by bot code
 * @param[in,out] cl
 * @param[in] cmd
 */
void SV_ClientThink(client_t *cl, usercmd_t *cmd)
{
	cl->lastUsercmd = *cmd;

	if (cl->state != CS_ACTIVE)
	{
		return;     // may have been kicked during the last usercmd
	}

	VM_Call(gvm, GAME_CLIENT_THINK, cl - svs.clients);
}

/**
 * @brief SV_UserMove
 *
 * @details The message usually contains all the movement commands
 * that were in the last three packets, so that the information
 * in dropped packets can be recovered.
 *
 * On very fast clients, there may be multiple usercmd packed into
 * each of the backup packets.
 *
 * @param cl
 * @param msg
 * @param delta
 */
static void SV_UserMove(client_t *cl, msg_t *msg, qboolean delta)
{
	int       i, key;
	int       cmdCount;
	usercmd_t nullcmd;
	usercmd_t cmds[MAX_PACKET_USERCMDS];
	usercmd_t *cmd, *oldcmd;

	if (delta)
	{
		cl->deltaMessage = cl->messageAcknowledge;
	}
	else
	{
		cl->deltaMessage = -1;
	}

	cmdCount = MSG_ReadByte(msg);

	if (cmdCount < 1)
	{
		Com_Printf("cmdCount < 1\n");
		return;
	}

	if (cmdCount > MAX_PACKET_USERCMDS)
	{
		Com_Printf("cmdCount > MAX_PACKET_USERCMDS\n");
		return;
	}

	// use the checksum feed in the key
	key = sv.checksumFeed;
	// also use the message acknowledge
	key ^= cl->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= MSG_HashKey(cl->reliableCommands[cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)], 32, !Com_IsCompatible(&cl->agent, 0x1));

	Com_Memset(&nullcmd, 0, sizeof(nullcmd));
	oldcmd = &nullcmd;
	for (i = 0 ; i < cmdCount ; i++)
	{
		cmd = &cmds[i];
		MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
		oldcmd = cmd;
	}

	// save the data to the server-side demo if recording
	/*
	if (sv.demoState == DS_RECORDING)
	{
	    SV_DemoWriteClientUsercmd(cl, delta, cmdCount, cmds, key);
	}
	*/

	// save time for ping calculation
	cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = svs.time;

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if (cl->state == CS_PRIMED)
	{
		if (sv_pure->integer != 0 && !cl->gotCP)
		{
			// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
			Com_DPrintf("%s: didn't get cp command, resending gamestate\n", cl->name);
			SV_SendClientGameState(cl);
			return;
		}
		SV_ClientEnterWorld(cl, &cmds[0]);
		// the moves can be processed normaly
	}

	// a bad cp command was sent, drop the client
	if (sv_pure->integer != 0 && cl->pureAuthentic == 0)
	{
		SV_DropClient(cl, "Cannot validate pure client!");
		return;
	}

	if (cl->state != CS_ACTIVE)
	{
		cl->deltaMessage = -1;
		return;
	}

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for (i = 0 ; i < cmdCount ; i++)
	{
		// if this is a cmd from before a map_restart ignore it
		if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime)
		{
			continue;
		}
		// extremely lagged or cmd from before a map_restart
		//if ( cmds[i].serverTime > svs.time + 3000 ) {
		//  continue;
		//}

		// don't execute if this is an old cmd which is already executed
		// these old cmds are included when cl_packetdup > 0
		if (cmds[i].serverTime <= cl->lastUsercmd.serverTime)       // Q3_MISSIONPACK
		{ //if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
			continue;   // from just before a map_restart
		}

		SV_ClientThink(cl, &cmds[i]);
	}
}

/**
 * @brief SV_ParseBinaryMessage
 * @param[in] cl
 * @param[in] msg
 */
static void SV_ParseBinaryMessage(client_t *cl, msg_t *msg)
{
	int size;

	MSG_BeginReadingUncompressed(msg);

	size = msg->cursize - msg->readcount;
	if (size <= 0 || size > MAX_BINARY_MESSAGE)
	{
		return;
	}

	SV_GameBinaryMessageReceived(cl - svs.clients, (char *)&msg->data[msg->readcount], size, cl->lastUsercmd.serverTime);
}

/*
===========================================================================
USER CMD EXECUTION
===========================================================================
*/

/**
 * @brief SV_ExecuteClientMessage
 * @param[in,out] cl
 * @param[in] msg
 */
void SV_ExecuteClientMessage(client_t *cl, msg_t *msg)
{
	int c;
	int serverId;

	MSG_Bitstream(msg);

	serverId               = MSG_ReadLong(msg);
	cl->messageAcknowledge = MSG_ReadLong(msg);

	if (cl->messageAcknowledge < 0 || serverId < 0)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifdef ETLEGACY_DEBUG
		SV_DropClient(cl, "DEBUG: illegible client message or invalid server id");
#endif
		return;
	}

	cl->reliableAcknowledge = MSG_ReadLong(msg);

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if (cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS)
	{
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifdef ETLEGACY_DEBUG
		SV_DropClient(cl, "DEBUG: illegible client message");
#endif
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state

	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if (serverId != sv.serverId && !*cl->downloadName && !strstr(cl->lastClientCommandString, "nextdl"))
	{
		if (serverId >= sv.restartedServerId && serverId < sv.serverId)     // TTimo - use a comparison here to catch multiple map_restart
		{   // they just haven't caught the map_restart yet
			Com_DPrintf("%s: ignoring pre map_restart / outdated client message status: %d\n", rc(cl->name), cl->state);
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if (cl->state != CS_ACTIVE && cl->messageAcknowledge > cl->gamestateMessageNum)
		{
			Com_DPrintf("%s: dropped gamestate, resending\n", rc(cl->name));
			SV_SendClientGameState(cl);
		}

		// read optional clientCommand strings
		do
		{
			c = MSG_ReadByte(msg);
			if (c < 0) // invalid MSG_Read
			{
				return;
			}
			if (c == clc_EOF)
			{
				break;
			}
			if (c != clc_clientCommand)
			{
				break;
			}
			if (!SV_ClientCommand(cl, msg, qtrue))
			{
				return; // we couldn't execute it because of the flood protection
			}
			if (cl->state == CS_ZOMBIE)
			{
				return; // disconnect command
			}
		}
		while (1);

		return;
	}

	// read optional clientCommand strings
	do
	{
		c = MSG_ReadByte(msg);
		if (c < 0) // invalid MSG_Read
		{
			return;
		}
		if (c == clc_EOF)
		{
			break;
		}
		if (c != clc_clientCommand)
		{
			break;
		}
		if (!SV_ClientCommand(cl, msg, qfalse))
		{
			return; // we couldn't execute it because of the flood protection
		}
		if (cl->state == CS_ZOMBIE)
		{
			return; // disconnect command
		}
	}
	while (1);

	// read the usercmd_t
	if (c == clc_move)
	{
		SV_UserMove(cl, msg, qtrue);
		c = MSG_ReadByte(msg);
	}
	else if (c == clc_moveNoDelta)
	{
		SV_UserMove(cl, msg, qfalse);
		c = MSG_ReadByte(msg);
	}

	if (c != clc_EOF)
	{
		Com_Printf("WARNING: bad command byte for client %i\n", (int) (cl - svs.clients));
	}

	SV_ParseBinaryMessage(cl, msg);

	//if ( msg->readcount != msg->cursize ) {
	//    Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
	//}
}
