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
 * @file sv_cl_main.c
 */

#include "server.h"

#ifdef DEDICATED

/**
 * @brief SV_CL_Connect_f
 */
void SV_CL_Connect_f(void)
{
	char         *server;
	const char   *ip_port;
	int          argc   = Cmd_Argc();
	netadrtype_t family = NA_UNSPEC;

	if (argc != 4 && argc != 5)
	{
		Com_Printf("usage: tv connect [-4|-6] server masterpassword privatepassword\n");
		return;
	}

	//if (!strcmp(Cmd_Argv(2), "-4"))
	//{
	//	family = NA_IP;
	//}
	//else if (!strcmp(Cmd_Argv(2), "-6"))
	//{
	//	family = NA_IP6;
	//}
	//else
	//{
	//	Com_Printf(S_COLOR_YELLOW "WARNING: only -4 or -6 as address type understood\n");
	//}

	server = Cmd_Argv(2);

	FS_ClearPureServerPacks();

	// clear any previous "server full" type messages
	svclc.serverMessage[0] = 0;

	if (com_sv_running->integer)
	{
		// if running a local server, kill it
		SV_Shutdown("Server quit");
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");
	SV_Frame(0);

	SV_CL_Disconnect();

	Q_strncpyz(svcls.servername, server, sizeof(svcls.servername));

	if (!NET_StringToAdr(svcls.servername, &svclc.serverAddress, family))
	{
		Com_Printf("Bad server address\n");
		svcls.state = CA_DISCONNECTED;
		return;
	}
	if (svclc.serverAddress.port == 0)
	{
		svclc.serverAddress.port = BigShort(PORT_SERVER);
	}

	Q_strncpyz(svclc.serverMasterPassword, Cmd_Argv(3), sizeof(svclc.serverMasterPassword));

	if (argc == 5)
	{
		Q_strncpyz(svclc.serverPassword, Cmd_Argv(4), sizeof(svclc.serverPassword));
	}

	ip_port = NET_AdrToString(&svclc.serverAddress);

	Com_Printf("%s resolved to %s\n", svcls.servername, ip_port);

	// if we aren't playing on a lan, we need to request a challenge
	if (NET_IsLocalAddress(&svclc.serverAddress))
	{
		svcls.state          = CA_CHALLENGING;
		svcls.challengeState = CA_CHALLENGING_INFO;
	}
	else
	{
		svcls.state = CA_CONNECTING;
	}

	// prepare to catch a connection process that would turn bad
	Cvar_Set("com_errorDiagnoseIP", NET_AdrToString(&svclc.serverAddress));

	// we need to setup a correct default for this, otherwise the first val we set might reappear
	Cvar_Set("com_errorMessage", "");

	svclc.connectTime        = -99999; // SV_CL_CheckForResend() will fire immediately
	svclc.connectPacketCount = 0;
}

/**
* @brief SV_CL_Commands_f
*/
void SV_CL_Commands_f(void)
{
	char *cmd;
	int  argc = Cmd_Argc();

	if (argc < 2)
	{
		Com_Printf("usage: tv <connect|disconnect>\n");
		return;
	}

	cmd = Cmd_Argv(1);

	if (!Q_stricmp(cmd, "connect"))
	{
		SV_CL_Connect_f();
	}
	else if (!Q_stricmp(cmd, "disconnect"))
	{
		SV_Shutdown("Server disconnected");
	}
}
#else
/**
* @brief SV_CL_Commands_f
*/
void SV_CL_Commands_f(void)
{
	char *cmd;
	int  argc = Cmd_Argc();

	if (argc < 2)
	{
		Com_Printf("usage: tv <demo|ff>\n");
		return;
	}

	cmd = Cmd_Argv(1);

	if (!Q_stricmp(cmd, "demo"))
	{
		SV_CL_PlayDemo_f();
	}
	else if (!Q_stricmp(cmd, "ff"))
	{
		SV_CL_FastForward_f();
	}
}

#endif // DEDICATED

#define RETRANSMIT_TIMEOUT  3000

/**
 * @brief Resend a connect message if the last one has timed out
 */
void SV_CL_CheckForResend(void)
{
	char buffer[64];
	// don't send anything if playing back a demo
	if (svclc.demo.playing)
	{
		return;
	}

	// resend if we haven't gotten a reply yet
	if (svcls.state != CA_CONNECTING && svcls.state != CA_CHALLENGING)
	{
		return;
	}

	if (svcls.realtime - svclc.connectTime < RETRANSMIT_TIMEOUT)
	{
		return;
	}

	svclc.connectTime = svcls.realtime; // for retransmit requests
	svclc.connectPacketCount++;

	switch (svcls.state)
	{
	case CA_CONNECTING:
	{
		Q_strncpyz(buffer, "getchallenge", sizeof(buffer));
		NET_OutOfBandPrint(NS_CLIENT, &svclc.serverAddress, buffer);
	}
	break;
	case CA_CHALLENGING:
	{
		// first get the server information
		if (svcls.challengeState == CA_CHALLENGING_INFO)
		{
			Com_sprintf(buffer, sizeof(buffer), "getinfo %i", svclc.challenge);
			NET_OutOfBandPrint(NS_CLIENT, &svclc.serverAddress, buffer);
		}
		// then attempt to connect
		else
		{
			int  port;
			char info[MAX_INFO_STRING];
			char data[MAX_INFO_STRING + 10];

			// received and confirmed the challenge, now responding with a 'connect' packet
			port = (int)(Cvar_VariableValue("net_qport"));

			Q_strncpyz(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));

			// make sure nothing restricted can slip through
			if (!Com_IsCompatible(&svclc.agent, 0x1))
			{
				Q_SafeNetString(info, MAX_INFO_STRING, qtrue);
			}

			Info_SetValueForKey(info, "protocol", va("%i", ETTV_PROTOCOL_VERSION));
			Info_SetValueForKey(info, "qport", va("%i", port));
			Info_SetValueForKey(info, "challenge", va("%i", svclc.challenge));
			Info_SetValueForKey(info, "masterpassword", svclc.serverMasterPassword);
			Info_SetValueForKey(info, "password", svclc.serverPassword);

			Info_SetValueForKey(info, "name", sv_etltv_clientname->string);
			Info_SetValueForKey(info, "rate", "90000");
			Info_SetValueForKey(info, "snaps", "20");
			Info_SetValueForKey(info, "cl_maxpackets", va("%i", SV_CL_MAXPACKETS));
			Info_SetValueForKey(info, "cg_uinfo", "0 0 40");

			Com_sprintf(data, sizeof(data), "connect \"%s\"", info);
			NET_OutOfBandData(NS_CLIENT, &svclc.serverAddress, (const char *)data, strlen(data));

			// the most current userinfo has been sent, so watch for any
			// newer changes to userinfo variables
			cvar_modifiedFlags &= ~CVAR_USERINFO;
		}
	}
	break;
	default:
		Com_Error(ERR_FATAL, "SV_CL_CheckForResend: bad svcls.state");
	}
}

/**
* @brief SV_CL_Disconnect
*/
void SV_CL_Disconnect(void)
{
	if (svclc.demo.recording)
	{
		SV_CL_StopRecord_f();
	}

	SV_CL_DemoCleanUp();

	// send a disconnect message to the server
	// send it a few times in case one is dropped
	if (svcls.state >= CA_CONNECTED && !svclc.demo.playing)
	{
		SV_CL_AddReliableCommand("disconnect");

		SV_CL_WritePacket();
		SV_CL_WritePacket();
		SV_CL_WritePacket();
	}

	FS_ClearPureServerPacks();

	SV_CL_ClearState();

	// wipe the client connection
	Com_Memset(&svclc, 0, sizeof(svclc));

	svcls.state               = CA_DISCONNECTED;
	svcls.lastRunFrameTime    = 0;
	svcls.lastRunFrameSysTime = Sys_Milliseconds();
	svcls.isGamestateParsed   = qfalse;
	svcls.isDelayed           = sv_etltv_delay->integer != 0;
}

/**
 * @brief The given command will be transmitted to the server, and is guaranteed to
 * not have future usercmd_t executed before it is executed.
 *
 * @param cmd
 */
void SV_CL_AddReliableCommand(const char *cmd)
{
	int index;

	if (svclc.demo.playing)
	{
		return;
	}

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if (svclc.reliableSequence - svclc.reliableAcknowledge > MAX_RELIABLE_COMMANDS)
	{
		Com_Error(ERR_DROP, "Server Client command overflow");
	}
	svclc.reliableSequence++;
	index = svclc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
	Q_strncpyz(svclc.reliableCommands[index], cmd, sizeof(svclc.reliableCommands[index]));
}

/**
 * @brief SV_CL_ReadyToSendPacket
 *
 * @details Returns qfalse if we are over the maxpackets limit
 * and should choke back the bandwidth a bit by not sending
 * a packet this frame.  All the commands will still get
 * delivered in the next packet, but saving a header and
 * getting more delta compression will reduce total bandwidth.
 *
 * @return
 */
qboolean SV_CL_ReadyToSendPacket(void)
{
	int oldPacketNum;
	int delta;

	// don't send anything if playing back a demo
	if (svclc.demo.playing)
	{
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	//if (*svcls.download.downloadTempName &&
	//	svcls.realtime - svclc.lastPacketSentTime < 50)
	//{
	//	return qfalse;
	//}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if (svcls.state != CA_ACTIVE &&
	    svcls.state != CA_PRIMED &&
	    /*!*cls.download.downloadTempName &&*/
	    svcls.realtime - svclc.lastPacketSentTime < 1000)
	{
		return qfalse;
	}

	// send every frame for loopbacks
	if (svclc.netchan.remoteAddress.type == NA_LOOPBACK)
	{
		return qtrue;
	}

	// send every frame for LAN
	if (Sys_IsLANAddress(&svclc.netchan.remoteAddress))
	{
		return qtrue;
	}

	oldPacketNum = (svclc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta        = svcls.realtime - svcl.outPackets[oldPacketNum].p_realtime;
	if (delta < 1000 / SV_CL_MAXPACKETS)
	{
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Create and send the command packet to the server
 * Including both the reliable commands and the usercmds
 *
 * @details During normal gameplay, a client packet will contain something like:
 *
 * 4   sequence number
 * 2   qport
 * 4   serverid
 * 4   acknowledged sequence number
 * 4   clc.serverCommandSequence
 * \<optional reliable commands\>
 * 1   clc_move or clc_moveNoDelta
 * 1   command count
 * \<count * usercmds\>
 */
void SV_CL_WritePacket(void)
{
#ifdef DEDICATED
	msg_t     buf;
	byte      data[MAX_MSGLEN];
	int       i;
	usercmd_t *cmd;
	usercmd_t nullcmd;
	int       packetNum;
	int       key;

	// don't send anything if playing back a demo
	if (svclc.demo.playing)
	{
		return;
	}

	Com_Memset(&nullcmd, 0, sizeof(nullcmd));

	MSG_Init(&buf, data, sizeof(data));

	MSG_Bitstream(&buf);
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong(&buf, svclc.serverIdLatest);

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong(&buf, svclc.serverMessageSequenceLatest);

	// write the last reliable message we received
	MSG_WriteLong(&buf, svclc.serverCommandSequenceLatest);

	// write any unacknowledged clientCommands
	// NOTE: if you verbose this, you will see that there are quite a few duplicates
	// typically several unacknowledged cp or userinfo commands stacked up
	for (i = svclc.reliableAcknowledge + 1; i <= svclc.reliableSequence; i++)
	{
		MSG_WriteByte(&buf, clc_clientCommand);
		MSG_WriteLong(&buf, i);
		MSG_WriteString(&buf, svclc.reliableCommands[i & (MAX_RELIABLE_COMMANDS - 1)]);
	}

	svcl.cmdNumber++;

	// FIXME: there has to be a way to simplify this
	// begin a client move command
	if ((!svcls.isGamestateParsed && (!svcl.snap.valid || svclc.demo.waiting || svclc.serverMessageSequence != svcl.snap.messageNum)) // live game
	    || (svcls.isGamestateParsed && (!svclc.moveDelta || svclc.demo.waiting || !svcls.queueDemoWaiting)))                         // delayed game
	{
		MSG_WriteByte(&buf, clc_moveNoDelta);
	}
	else
	{
		MSG_WriteByte(&buf, clc_move);
	}

	// write the command count
	MSG_WriteByte(&buf, 1);

	// use the checksum feed in the key
	key = svclc.checksumFeedLatest;
	// also use the message acknowledge
	key ^= svclc.serverMessageSequenceLatest;
	// also use the last acknowledged server command in the key
	key ^= MSG_HashKey(svclc.serverCommandsLatest[svclc.serverCommandSequenceLatest & (MAX_RELIABLE_COMMANDS - 1)], 32, 0);

	// write all the commands, including the predicted command
	cmd             = &svcl.cmds[svcl.cmdNumber & CMD_MASK];
	cmd->serverTime = svcl.serverTimeLatest;
	MSG_WriteDeltaUsercmdKey(&buf, key, &nullcmd, cmd);

	// deliver the message
	packetNum                              = svclc.netchan.outgoingSequence & PACKET_MASK;
	svcl.outPackets[packetNum].p_realtime  = svcls.realtime;
	svcl.outPackets[packetNum].p_cmdNumber = svcl.cmdNumber;
	svclc.lastPacketSentTime               = svcls.realtime;

	SV_CL_Netchan_Transmit(&svclc.netchan, &buf);

	// tv clients never really should have messages large enough
	while (svclc.netchan.unsentFragments)
	{
		SV_CL_Netchan_TransmitNextFragment(&svclc.netchan);
	}
#endif
}

/**
 * @brief SV_CL_InitTVGame
 */
void SV_CL_InitTVGame(void)
{
	const char *info;
	const char *mapname;
	int        t1, t2;

	t1 = Sys_Milliseconds();

	// find the current mapname
	info    = svcl.gameState.stringData + svcl.gameState.stringOffsets[CS_SERVERINFO];
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(svcl.mapname, sizeof(svcl.mapname), "maps/%s.bsp", mapname);

	svcls.state = CA_LOADING;

	SV_SpawnServer(mapname);

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	svcls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf("SV_CL_InitTVGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// make sure everything is paged in
	if (!Sys_LowPhysicalMemory())
	{
		Com_TouchMemory();
	}
}

/**
 * @brief SV_CL_ConfigstringModified
 */
void SV_CL_ConfigstringModified(void)
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

	old = svcl.gameState.stringData + svcl.gameState.stringOffsets[index];
	if (!strcmp(old, s))
	{
		return;     // unchanged
	}

	// build the new gameState_t
	oldGs = svcl.gameState;

	Com_Memset(&svcl.gameState, 0, sizeof(svcl.gameState));

	// leave the first 0 for uninitialized strings
	svcl.gameState.dataCount = 1;

	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
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

		if (len + 1 + svcl.gameState.dataCount > MAX_GAMESTATE_CHARS)
		{
			Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
		}

		// append it to the gameState string buffer
		svcl.gameState.stringOffsets[i] = svcl.gameState.dataCount;
		Com_Memcpy(svcl.gameState.stringData + svcl.gameState.dataCount, dup, len + 1);
		svcl.gameState.dataCount += len + 1;
	}

	if (index == CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		SV_CL_SystemInfoChanged();
	}
	else
	{
		SV_SetConfigstring(index, s);
	}
}

/**
 * @brief SV_CL_Cvar_InfoString returns updated configstring
 * @param[in] cs
 * @param[in] index
 * @return
 */
char *SV_CL_Cvar_InfoString(char *cs, int index)
{
	static char newcs[BIG_INFO_STRING];

	Q_strncpyz(newcs, cs, sizeof(newcs));

	if (svcls.state != CA_DISCONNECTED)
	{
		if (index == CS_SERVERINFO)
		{
			// FIXME: sv_maxclients can't go over 64 for legacy cgame
			// and possibly other mods, which would be good to support too
			if (sv_maxclients->integer > MAX_CLIENTS)
			{
				Info_SetValueForKey_Big(newcs, "sv_maxclients", "64");
			}
			else
			{
				Info_SetValueForKey_Big(newcs, "sv_maxclients", sv_maxclients->string);
			}

			Info_SetValueForKey_Big(newcs, "sv_privateClients", sv_privateClients->string);
			Info_SetValueForKey_Big(newcs, "sv_maxPing", sv_maxPing->string);
			Info_SetValueForKey_Big(newcs, "sv_minPing", sv_minPing->string);
			Info_SetValueForKey_Big(newcs, "sv_maxRate", sv_maxRate->string);
			Info_SetValueForKey_Big(newcs, "sv_dlRate", sv_dlRate->string);
			Info_SetValueForKey_Big(newcs, "sv_hostname", sv_hostname->string);
			Info_SetValueForKey_Big(newcs, "version", Cvar_VariableString("version"));
		}
		else if (index == CS_SYSTEMINFO)
		{
			Info_SetValueForKey_Big(newcs, "sv_serverid", va("%d", sv.serverId));
			Info_SetValueForKey_Big(newcs, "sv_pure", sv_pure->string);
		}
	}

	return newcs;
}

/**
 * @brief Set up argc/argv for the given command
 * @param[in] serverCommandNumber
 * @return
 */
qboolean SV_CL_GetServerCommand(int serverCommandNumber)
{
	char        *s;
	char        *cmd;
	static char bigConfigString[BIG_INFO_STRING];

	// if we have irretrievably lost a reliable command, drop the connection
	if (serverCommandNumber <= svclc.serverCommandSequence - MAX_RELIABLE_COMMANDS)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if (svclc.demo.playing)
		{
			return qfalse;
		}
		Com_Error(ERR_DROP, "SV_CL_GetServerCommand: a reliable command was cycled out");
		return qfalse;
	}

	if (serverCommandNumber > svclc.serverCommandSequence)
	{
		Com_Error(ERR_DROP, "SV_CL_GetServerCommand: requested a command not received");
		return qfalse;
	}

	s                               = svclc.serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS - 1)];
	svclc.lastExecutedServerCommand = serverCommandNumber;

	//if (cl_showServerCommands->integer)
	//{
	//	Com_DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);
	//}

rescan:
	Cmd_TokenizeString(s);
	cmd = Cmd_Argv(0);

	if (!strcmp(cmd, "disconnect"))
	{
		// this is a bit hacky solution (same code invoked twice) to properly
		// propagate disconnect message to chained tv server(s) and their client(s)
		if (Cmd_Argc() >= 2)
		{
			SV_Shutdown(va("Server Disconnected - %s", Cmd_Argv(1)));
		}
		else
		{
			SV_Shutdown("Server disconnected");
		}

		Com_Error(ERR_SERVERDISCONNECT, "Server disconnected");
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
		SV_CL_ConfigstringModified();
		// reparse the string, because SV_CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return qtrue;
	}

	if (!strcmp(cmd, "map_restart"))
	{
		// clear outgoing commands before passing
		Com_Memset(svcl.cmds, 0, sizeof(svcl.cmds));
		Cbuf_AddText("map_restart 0\n");
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
	//if (!strcmp(cmd, "clientLevelShot"))
	//{
	//	// don't do it if we aren't running the server locally,
	//	// otherwise malicious remote servers could overwrite
	//	// the existing thumbnails
	//	if (!com_sv_running->integer)
	//	{
	//		return qfalse;
	//	}
	//	// close the console
	//	Con_Close();
	//	// take a special screenshot next frame
	//	Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");
	//	return qtrue;
	//}

	Cmd_SingleTokenString(s);
	return qtrue;
}

/**
 * @brief SV_CL_SendPureChecksums
 */
void SV_CL_SendPureChecksums(int serverId)
{
	//const char *pChecksums;
	char cMsg[MAX_INFO_VALUE];

	// if we are pure we need to send back a command with our referenced pk3 checksums
	//pChecksums = FS_ReferencedPakPureChecksums();

	//Com_sprintf(cMsg, sizeof(cMsg), "cp %d %s", svcl.serverId, pChecksums);
	Com_sprintf(cMsg, sizeof(cMsg), "cp %d SLAVE", serverId);

	SV_CL_AddReliableCommand(cMsg);
}

/**
 * @brief SV_CL_DownloadsComplete
 */
void SV_CL_DownloadsComplete(void)
{
	// let the client game init and load data
	svcls.state = CA_LOADING;

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
	if (svcls.state != CA_LOADING)
	{
		return;
	}

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
	SV_CL_FlushMemory();

	SV_CL_InitTVGame();

	if (!svcls.isGamestateParsed)
	{
		SV_CL_SendPureChecksums(svcl.serverId);
	}

	SV_CL_WritePacket();
	SV_CL_WritePacket();
	SV_CL_WritePacket();

	svcls.state = CA_ACTIVE;
	sv.time     = 0;
}

/**
 * @brief SV_CL_ServerInfoPacketCheck
 * @param[in] from
 * @param[in] msg
 */
void SV_CL_ServerInfoPacketCheck(const netadr_t *from, msg_t *msg)
{
	int  prot;
	char *infoString;
	char *gameName;

	infoString = MSG_ReadString(msg);

	// if this isn't the correct protocol version, ignore it
	prot = Q_atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != PROTOCOL_VERSION)
	{
		Com_DPrintf("Different protocol info packet: %s\n", infoString);
		Com_Error(ERR_FATAL, "Game server uses unsupported protocol: %i, expected %i (%s)", prot, PROTOCOL_VERSION, GAMENAME_STRING);
		return;
	}

	// if this isn't the correct game, ignore it
	gameName = Info_ValueForKey(infoString, "gamename");
	if (!gameName[0] || Q_stricmp(gameName, GAMENAME_STRING))
	{
		Com_DPrintf("Different game info packet: %s\n", infoString);
		Com_Error(ERR_FATAL, "Unsupported game server: %s", gameName);
		return;
	}

	// upon challenging we request server info to obtain user agent information
	// btw, old clients dont store version in getinfoResponse body, however, all etl clients do
	if (svcls.state == CA_CHALLENGING && svcls.challengeState == CA_CHALLENGING_INFO)
	{
		Com_ParseUA(&svclc.agent, Info_ValueForKey(infoString, "version"));
	}
}

/**
 * @brief SV_CL_ServerInfoPacket
 * @param[in] from
 * @param[in] msg
 */
void SV_CL_ServerInfoPacket(const netadr_t *from, msg_t *msg)
{
	//int  i, type;
	//char info[MAX_INFO_STRING];
	char *infoString;
	int  prot;
	char *gameName;

	infoString = MSG_ReadString(msg);

	// if this isn't the correct protocol version, ignore it
	prot = Q_atoi(Info_ValueForKey(infoString, "protocol"));
	if (prot != PROTOCOL_VERSION)
	{
		Com_DPrintf("Different protocol info packet: %s\n", infoString);
		return;
	}

	// if this isn't the correct game, ignore it
	gameName = Info_ValueForKey(infoString, "gamename");
	if (!gameName[0] || Q_stricmp(gameName, GAMENAME_STRING))
	{
		Com_DPrintf("Different game info packet: %s\n", infoString);
		return;
	}

	//// iterate servers waiting for ping response
	//for (i = 0; i < MAX_PINGREQUESTS; i++)
	//{
	//	if (cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr(from, cl_pinglist[i].adr))
	//	{
	//		// calc ping time
	//		cl_pinglist[i].time = svcls.realtime - cl_pinglist[i].start + 1;

	//		if (com_developer->integer)
	//		{
	//			Com_Printf("ping time %dms from %s\n", cl_pinglist[i].time, NET_AdrToString(from));
	//		}

	//		// save of info
	//		Q_strncpyz(cl_pinglist[i].info, infoString, sizeof(cl_pinglist[i].info));

	//		// tack on the net type
	//		// NOTE: make sure these types are in sync with the netnames strings in the UI
	//		switch (from.type)
	//		{
	//		case NA_BROADCAST:
	//		case NA_IP:
	//			type = 1;
	//			break;
	//		case NA_IP6:
	//			type = 2;
	//			break;
	//		default:
	//			type = 0;
	//			break;
	//		}
	//		Info_SetValueForKey(cl_pinglist[i].info, "nettype", va("%d", type));
	//		CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

	//		return;
	//	}
	//}

	//// if not just sent a local broadcast or pinging local servers
	//if (svcls.pingUpdateSource != AS_LOCAL)
	//{
	//	return;
	//}

	//for (i = 0; i < MAX_OTHER_SERVERS; i++)
	//{
	//	// empty slot
	//	if (cls.localServers[i].adr.port == 0)
	//	{
	//		break;
	//	}

	//	// avoid duplicate
	//	if (NET_CompareAdr(from, cls.localServers[i].adr))
	//	{
	//		return;
	//	}
	//}

	//if (svcls.numlocalservers == MAX_OTHER_SERVERS)
	//{
	//	Com_DPrintf("MAX_OTHER_SERVERS hit, dropping infoResponse\n");
	//	return;
	//}

	//// add this to the list
	//svcls.numlocalservers = i + 1;
	//CL_InitServerInfo(&cls.localServers[i], &from);

	//Q_strncpyz(info, MSG_ReadString(msg), MAX_INFO_STRING);
	//if (strlen(info))
	//{
	//	if (info[strlen(info) - 1] != '\n')
	//	{
	//		Q_strcat(info, sizeof(info), "\n");
	//	}
	//	Com_Printf("%s: %s", NET_AdrToString(from), info);
	//}
}

/**
 * @brief SV_CL_DisconnectPacket
 *
 * @details Sometimes the server can drop the client and the netchan based
 * disconnect can be lost.  If the client continues to send packets
 * to the server, the server will send out of band disconnect packets
 * to the client so it doesn't have to wait for the full timeout period.
 *
 * @param[in] from
 */
void SV_CL_DisconnectPacket(const netadr_t *from)
{
	if (svcls.state < CA_AUTHORIZING)
	{
		return;
	}

	// if not from our server, ignore it
	if (!NET_CompareAdr(from, &svclc.netchan.remoteAddress))
	{
		return;
	}

	// if we have received packets within three seconds, ignore (it might be a malicious spoof)
	// NOTE: there used to be a  clc.lastPacketTime = cls.realtime; line in CL_PacketEvent before calling CL_ConnectionLessPacket
	// therefore .. packets never got through this check, clients never disconnected
	// switched the clc.lastPacketTime = cls.realtime to happen after the connectionless packets have been processed
	// you still can't spoof disconnects, cause legal netchan packets will maintain realtime - lastPacketTime below the threshold
	if (svcls.realtime - svclc.lastPacketTime < 3000)
	{
		return;
	}

	// if we are doing a disconnected download, leave the 'connecting' screen on with the progress information
	//if (!svcls.download.bWWWDlDisconnected)
	//{
	//	// drop the connection
	//	message = "Server disconnected for unknown reason";
	//	Com_Printf("%s\n", message);
	//	Cvar_Set("com_errorMessage", message);
	//	SV_CL_Disconnect();
	//}
	//else
	{
		SV_CL_Disconnect();
	}
}

/**
 * @brief an OOB message from server, with potential markups.
 * Print OOB are the only messages we handle markups in.
 *
 * [err_dialog]: Used to indicate that the connection should be aborted. No
 *               further information, just do an error diagnostic screen afterwards.
 * [err_prot]:   This is a protocol error. The client uses a custom protocol error
 *               message (client sided) in the diagnostic window.
 * ET://         Redirects client to another server.
 * The space for the error message on the connection screen is limited to 256 chars.
 */
void SV_CL_PrintPacket(msg_t *msg)
{
	char     *s;
	qboolean disconnect = qtrue;

	s = MSG_ReadBigString(msg);
	if (!Q_stricmpn(s, "[err_dialog]", 12))
	{
		Q_strncpyz(svclc.serverMessage, s + 12, sizeof(svclc.serverMessage));
	}
	else if (!Q_stricmpn(s, "[err_prot]", 10))
	{
		Q_strncpyz(svclc.serverMessage, s + 10, sizeof(svclc.serverMessage));
	}
	else if (!Q_stricmpn(s, "[err_update]", 12))
	{
		Q_strncpyz(svclc.serverMessage, s + 12, sizeof(svclc.serverMessage));
	}
	else if (!Q_stricmpn(s, "et://", 5))
	{
		Q_strncpyz(svclc.serverMessage, s, sizeof(svclc.serverMessage));
	}
	else
	{
		Q_strncpyz(svclc.serverMessage, s, sizeof(svclc.serverMessage));
		disconnect = qfalse;
	}

	Com_Printf("%s", svclc.serverMessage);

	if (disconnect)
	{
		SV_CL_Disconnect();
	}
}

/**
 * @brief Responses to broadcasts, etc
 *
 * Compare first n chars so it doesn’t bail if token is parsed incorrectly.
 *
 * @param[in] from
 * @param[in] msg
 */
void SV_CL_ConnectionlessPacket(const netadr_t *from, msg_t *msg)
{
	char *s;
	char *c;

	MSG_BeginReadingOOB(msg);
	MSG_ReadLong(msg);      // skip the -1

	s = MSG_ReadStringLine(msg);

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);

	if (com_developer->integer)
	{
		Com_Printf("SV_CL packet %s: %s\n", NET_AdrToString(from), c);
	}

	// challenge from the server we are connecting to
	if (!Q_stricmp(c, "challengeResponse"))
	{
		if (svcls.state != CA_CONNECTING)
		{
			Com_Printf("Unwanted challenge response received, '%s' ignored\n", c);
		}
		else
		{
			// start sending challenge response instead of challenge request packets
			svclc.challenge = Q_atoi(Cmd_Argv(1));
			if (Cmd_Argc() > 2)
			{
				svclc.onlyVisibleClients = Q_atoi(Cmd_Argv(2));
			}
			else
			{
				svclc.onlyVisibleClients = 0;
			}
			svcls.state              = CA_CHALLENGING;
			svcls.challengeState     = CA_CHALLENGING_INFO;
			svclc.connectPacketCount = 0;
			svclc.connectTime        = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			svclc.serverAddress = *from;
			Com_DPrintf("challenge: %d\n", svclc.challenge);
		}
		return;
	}

	// server connection
	if (!Q_stricmp(c, "connectResponse"))
	{
		if (svcls.state >= CA_CONNECTED)
		{
			Com_Printf("Dup connect received.  Ignored.\n");
			return;
		}
		if (svcls.state != CA_CHALLENGING)
		{
			Com_Printf("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if (!NET_CompareAdr(from, &svclc.serverAddress))
		{
			Com_Printf("connectResponse from a different address.  Ignored.\n");
			Com_Printf("%s should have been %s\n", NET_AdrToString(from),
			           NET_AdrToString(&svclc.serverAddress));
			return;
		}

		Com_CheckUpdateStarted();

		Netchan_Setup(NS_CLIENT, &svclc.netchan, from, Cvar_VariableValue("net_qport"));
		svcls.state              = CA_CONNECTED;
		svclc.lastPacketSentTime = -9999;     // send first packet immediately
		return;
	}

	// server responding to an info broadcast
	if (!Q_stricmp(c, "infoResponse"))
	{
		if (svcls.state == CA_CHALLENGING && svcls.challengeState == CA_CHALLENGING_INFO)
		{
			SV_CL_ServerInfoPacketCheck(from, msg);
			svcls.challengeState = CA_CHALLENGING_REQUEST;
			svclc.connectTime    = -99999; // CL_CheckForResend() will fire immediately
			return;
		}
		SV_CL_ServerInfoPacket(from, msg);
		return;
	}

	// server responding to a get playerlist
	if (!Q_stricmp(c, "statusResponse"))
	{
		//CL_ServerStatusResponse(from, msg);
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect"))
	{
		SV_CL_DisconnectPacket(from);
		return;
	}

	// echo request from server
	if (!Q_stricmp(c, "echo"))
	{
		// FIXME: || NET_CompareAdr(from, clc.authorizeServer)
		//if (NET_CompareAdr(from, svclc.serverAddress) || NET_CompareAdr(from, rcon_address) || NET_CompareAdr(from, autoupdate.autoupdateServer))
		//{
		//	NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
		//}
		return;
	}

	// cd check
	if (!Q_stricmp(c, "keyAuthorize"))
	{
		// we don't use these now, so dump them on the floor
		return;
	}

	// global MOTD
	if (!Q_stricmp(c, "motd"))
	{
		//CL_MotdPacket(from);
		return;
	}

	// echo request from server
	if (!Q_stricmp(c, "print"))
	{
		// FIXME: || NET_CompareAdr(from, clc.authorizeServer)
		if (NET_CompareAdr(from, &svclc.serverAddress) /* || NET_CompareAdr(from, rcon_address) || NET_CompareAdr(from, autoupdate.autoupdateServer)*/)
		{
			SV_CL_PrintPacket(msg);
		}
		return;
	}

	// Update server response message
	if (!Q_stricmp(c, "updateResponse"))
	{
		//Com_UpdateInfoPacket(from);
		return;
	}

	// list of servers sent back by a master server
	if (!Q_strncmp(c, "getserversResponse", 18))
	{
		//CL_ServersResponsePacket(&from, msg, qfalse);
		return;
	}

	// list of servers sent back by a master server (extended)
	if (!Q_strncmp(c, "getserversExtResponse", 21))
	{
		//CL_ServersResponsePacket(&from, msg, qtrue);
		return;
	}

	// list of blocked servers sent back by a master server
	if (!Q_strncmp(c, "getBlockedServersResponse", 25))
	{
		//CL_BlockedServersResponsePacket(&from, msg);
		return;
	}

	Com_DPrintf("Unknown connectionless packet command.\n");
}

#ifdef  DEDICATED

/**
 * @brief A packet has arrived from the main event loop
 *
 * @param[in] from
 * @param[in] msg
 */
static void SV_CL_PacketEvent(const netadr_t *from, msg_t *msg)
{
	int headerBytes;

	if (!NET_CompareAdr(from, &svclc.serverAddress))
	{
		if (svcls.isTVGame)
		{
			SV_PacketEvent(from, msg);
		}

		return;
	}

	//if (Com_UpdatePacketEvent(from))
	//{
	//	return;
	//}

	if (msg->cursize >= 4 && *(int *)msg->data == -1)
	{
		SV_CL_ConnectionlessPacket(from, msg);
		return;
	}

	svclc.lastPacketTime = svcls.realtime;

	if (svcls.state < CA_CONNECTED)
	{
		return;     // can't be a valid sequenced packet
	}

	if (msg->cursize < 4)
	{
		Com_Printf("%s: Runt packet\n", NET_AdrToString(from));
		return;
	}

	// packet from server
	if (!NET_CompareAdr(from, &svclc.netchan.remoteAddress))
	{
		if (com_developer->integer)
		{
			Com_Printf("%s:sequenced packet without connection\n", NET_AdrToString(from));
		}
		// client isn't connected - don't send disconnect
		return;
	}

	if (!SV_CL_Netchan_Process(&svclc.netchan, msg))
	{
		return;     // out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	svclc.serverMessageSequenceLatest = LittleLong(*(int *)msg->data);
	svclc.lastPacketTime              = svcls.realtime;

	SV_CL_ParseServerMessage_Ext(msg, headerBytes);
}

/**
 * @brief A packet has arrived from the main event loop
 *
 * @param[in] from
 * @param[in] msg
 */
void CL_PacketEvent(const netadr_t *from, msg_t *msg)
{
	if (svcls.state != CA_DISCONNECTED)
	{
		SV_CL_PacketEvent(from, msg);
	}
}

#endif //  DEDICATED

/**
 * @brief SV_CL_FlushMemory
 */
void SV_CL_FlushMemory(void)
{
	if (svclc.demo.recording)
	{
		SV_CL_StopRecord_f();
	}

	// if not running a server clear the whole hunk
	if (!com_sv_running->integer)
	{
		// clear the whole hunk
		Hunk_Clear();
		// clear collision map data
		CM_ClearMap();
	}
	else
	{
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}
}

/**
 * @brief Called before parsing a gamestate
 */
void SV_CL_ClearState(void)
{
	Com_Memset(&svcl, 0, sizeof(svcl));
}

/**
 * @brief SV_CL_GetPlayerstate
 * @param[in] clientNum
 * @param[in,out] ps
 * @return qtrue if playerstate is valid
 */
int SV_CL_GetPlayerstate(int clientNum, playerState_t *ps)
{
	if (!svcls.isTVGame)
	{
		Com_Error(ERR_FATAL, "SV_CL_GetPlayerstate: TVGame not loaded");
	}

	if (clientNum >= MAX_CLIENTS)
	{
		Com_Error(ERR_FATAL, "SV_CL_GetPlayerstate: invalid clientNum %d", clientNum);
	}

	if (ps == NULL)
	{
		Com_Error(ERR_FATAL, "SV_CL_GetPlayerstate: clientNum %d NULL ps", clientNum);
	}

	if (clientNum == -1)
	{
		*ps = svcl.snap.ps;

		return svcl.snap.valid;
	}
	else
	{
		if (!svcl.snap.playerstates[clientNum].valid)
		{
			return qfalse;
		}

		*ps = svcl.snap.playerstates[clientNum].ps;
	}

	return qtrue;
}

/**
 * @brief SV_CL_RunFrame
 */
void SV_CL_RunFrame(void)
{
	int frameMsec = 1000 / sv_fps->integer;
	int systime   = Sys_Milliseconds();
	int frameDelta, queueMs;

	if (svcls.state <= CA_DISCONNECTED)
	{
		return;
	}

	if (svcls.isDelayed)
	{
		int queueTime = SV_CL_GetQueueTime() - sv_etltv_delay->integer * 1000;

		if (queueTime < 0)
		{
			queueTime = queueTime - frameMsec + 1;
		}

		svcl.serverTime = (queueTime / frameMsec) * frameMsec;

		if (svMsgQueueHead)
		{
			queueMs = svMsgQueueHead->serverTime - svcl.serverTime;

			if (sv_etltv_queue_ms->integer != queueMs)
			{
				Cvar_Set("ettv_queue_ms", va("%i", queueMs));
			}
		}
	}
	else
	{
		if (sv_etltv_queue_ms->integer != -1)
		{
			Cvar_Set("ettv_queue_ms", "-1");
		}
	}

	if (svcl.serverTime < svcls.lastRunFrameTime)
	{
		if (svcls.firstSnap)
		{
			Com_DPrintf("Server went backwards in time: %d > %d\n", svcls.lastRunFrameTime, svcl.serverTime);
		}
		else
		{
			Com_Printf("Rewinding server time\n");

			sv.time                   = svcl.serverTime;
			svcls.lastRunFrameTime    = svcl.serverTime;
			svcls.lastRunFrameSysTime = systime;
		}
	}
	else
	{
		if (!svclc.demo.playing)
		{
			frameDelta = (((systime - svcls.lastRunFrameSysTime) + svcls.lastRunFrameTime) / frameMsec) * frameMsec;

			if (systime - svcls.lastRunFrameSysTime > 1500 / sv_fps->integer && frameDelta > svcl.serverTime)
			{
				if (sv.time != 0)
				{
					Com_Printf("Dropped server frame: systimeDelta [%d] sv.time [%d] svcl.serverTime [%d] frameDelta [%d]\n",
					           systime - svcls.lastRunFrameSysTime, sv.time, svcl.serverTime, frameDelta);
				}

				svcl.serverTime = frameDelta;
			}
		}
	}

	Cbuf_Execute();

	if (sv.time < svcl.serverTime && (svcls.firstSnap || svcls.isDelayed))
	{
		if (!svclc.demo.playing || svcl.serverTime - sv.time <= frameMsec)
		{
			svs.time                 += frameMsec;
			sv.time                   = svcl.serverTime;
			svcls.lastRunFrameTime    = sv.time;
			svcls.lastRunFrameSysTime = systime;

			VM_Call(gvm, GAME_RUN_FRAME, sv.time);

			SV_SendClientMessages();
			return;
		}

		if (sv.time != 0)
		{
			Com_Printf("Dropped frame: sv.time [%d] svcl.serverTime [%d] frameMsec [%d]\n", sv.time, svcl.serverTime, frameMsec);
		}

		svs.time                 += frameMsec;
		sv.time                  += frameMsec;
		svcls.lastRunFrameTime    = sv.time;
		svcls.lastRunFrameSysTime = systime;

		VM_Call(gvm, GAME_RUN_FRAME, sv.time);
	}
}

/**
 * @brief SV_CL_Frame
 */
void SV_CL_Frame(int frameMsec)
{
	int startTime;

	if (cvar_modifiedFlags & CVAR_SERVERINFO)
	{
		SV_SetConfigstring(CS_SERVERINFO, SV_CL_Cvar_InfoString(sv.configstrings[CS_SERVERINFO], CS_SERVERINFO));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if (cvar_modifiedFlags & CVAR_SERVERINFO_NOUPDATE)
	{
		SV_SetConfigstringNoUpdate(CS_SERVERINFO, SV_CL_Cvar_InfoString(sv.configstrings[CS_SERVERINFO], CS_SERVERINFO));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO_NOUPDATE;
	}
	if (cvar_modifiedFlags & CVAR_SYSTEMINFO)
	{
		SV_SetConfigstring(CS_SYSTEMINFO, SV_CL_Cvar_InfoString(sv.configstrings[CS_SYSTEMINFO], CS_SYSTEMINFO));
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}
	if (cvar_modifiedFlags & CVAR_WOLFINFO)
	{
		SV_SetConfigstring(CS_WOLFINFO, SV_CL_Cvar_InfoString(sv.configstrings[CS_WOLFINFO], CS_WOLFINFO));
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}

	if (com_speeds->integer)
	{
		startTime = Sys_Milliseconds();
	}
	else
	{
		startTime = 0;  // quite a compiler warning
	}

	SV_CL_ParseMessageQueue();

	if (svclc.demo.fastForwardTime <= sv.time)
	{
		// run the game simulation in chunks
		while (sv.timeResidual >= frameMsec)
		{
			sv.timeResidual -= frameMsec;
			svs.time        += frameMsec;

			if (svclc.demo.playing && svcl.serverTime <= sv.time)
			{
				SV_CL_ReadDemoMessage();
			}

			SV_CL_RunFrame();
		}
	}
	else if (svclc.demo.playing && svcl.serverTime <= sv.time)
	{
		SV_CL_ReadDemoMessage();
	}
	else
	{
		SV_CL_RunFrame();
	}

	if (com_speeds->integer)
	{
		time_game = Sys_Milliseconds() - startTime;
	}

	if (SV_CL_ReadyToSendPacket())
	{
		SV_CL_WritePacket();
	}

	// check timeouts
	SV_CheckTimeouts();

	// check user info buffer thingy
	SV_CheckClientUserinfoTimer();
}
