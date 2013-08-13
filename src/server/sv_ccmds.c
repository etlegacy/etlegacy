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
 * @file sv_ccmds.c
 * @brief OPERATOR CONSOLE ONLY COMMANDS
 *
 * These commands can only be entered from stdin or by a remote operator datagram
 */

#include "server.h"

/*
==================
SV_GetPlayerByName

Returns the player with name from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByName(void)
{
	client_t *cl;
	int      i;
	char     *s;
	char     cleanName[64];

	// make sure server is running
	if (!com_sv_running->integer)
	{
		return NULL;
	}

	if (Cmd_Argc() < 2)
	{
		Com_Printf("No player specified.\n");
		return NULL;
	}

	s = Cmd_Argv(1);

	// check for a name match
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (cl->state <= CS_ZOMBIE)
		{
			continue;
		}
		if (!Q_stricmp(cl->name, s))
		{
			return cl;
		}

		Q_strncpyz(cleanName, cl->name, sizeof(cleanName));
		Q_CleanStr(cleanName);
		if (!Q_stricmp(cleanName, s))
		{
			return cl;
		}
	}

	Com_Printf("Player %s is not on the server\n", s);

	return NULL;
}

//=========================================================

/**
 * @brief Restart the server on a different map
 */
static void SV_Map_f(void)
{
	char     *cmd;
	char     *map;
	char     mapname[MAX_QPATH];
	qboolean cheat;
	char     expanded[MAX_QPATH];

	cmd = Cmd_Argv(0);
	map = Cmd_Argv(1);
	if (!map || !map[0])
	{
		Com_Printf("Usage: %s <map name>\n", cmd);
		return;
	}

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	Com_sprintf(expanded, sizeof(expanded), "maps/%s.bsp", map);
	if (FS_ReadFile(expanded, NULL) == -1)
	{
		Com_Printf("Can't find map %s\n", expanded);
		return;
	}

	Cvar_Set("gamestate", va("%i", GS_INITIALIZE)); // reset gamestate on map/devmap
	Cvar_Set("g_currentRound", "0");                // reset the current round
	Cvar_Set("g_nextTimeLimit", "0");               // reset the next time limit

	if (!Q_stricmp(cmd, "devmap"))
	{
		cheat = qtrue;
	}
	else
	{
		cheat = qfalse;
	}

	// save the map name here cause on a map restart we reload the etconfig.cfg
	// and thus nuke the arguments of the map command
	Q_strncpyz(mapname, map, sizeof(mapname));

	// start up the map
	SV_SpawnServer(mapname);

	// set the cheat value
	// if the level was started with "map <mapname>", then
	// cheats will not be allowed.
	// If started with "devmap <mapname>"
	// then cheats will be allowed
	if (cheat)
	{
		Cvar_Set("sv_cheats", "1");
	}
	else
	{
		Cvar_Set("sv_cheats", "0");
	}
}

/*
================
SV_CheckTransitionGameState
================
*/
static qboolean SV_CheckTransitionGameState(gamestate_t new_gs, gamestate_t old_gs)
{
	if (old_gs == new_gs && new_gs != GS_PLAYING)
	{
		return qfalse;
	}

	if (old_gs == GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP)
	{
		return qfalse;
	}

	if (old_gs == GS_INTERMISSION && new_gs != GS_WARMUP)
	{
		return qfalse;
	}

	if (old_gs == GS_RESET && (new_gs != GS_WAITING_FOR_PLAYERS && new_gs != GS_WARMUP))
	{
		return qfalse;
	}

	return qtrue;
}

/*
================
SV_TransitionGameState
================
*/
static qboolean SV_TransitionGameState(gamestate_t new_gs, gamestate_t old_gs, int delay)
{
	// we always do a warmup before starting match
	if (old_gs == GS_INTERMISSION && new_gs == GS_PLAYING)
	{
		new_gs = GS_WARMUP;
	}

	// check if its a valid state transition
	if (!SV_CheckTransitionGameState(new_gs, old_gs))
	{
		return qfalse;
	}

	if (new_gs == GS_RESET)
	{
		new_gs = GS_WARMUP;
	}

	Cvar_Set("gamestate", va("%i", new_gs));

	return qtrue;
}

void MSG_PrioritiseEntitystateFields(void);
void MSG_PrioritisePlayerStateFields(void);

static void SV_FieldInfo_f(void)
{
	MSG_PrioritiseEntitystateFields();
	MSG_PrioritisePlayerStateFields();
}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f(void)
{
	int         i;
	client_t    *client;
	char        *denied;
	qboolean    isBot;
	int         delay = 0;
	gamestate_t new_gs, old_gs;

	// make sure we aren't restarting twice in the same frame
	if (com_frameTime == sv.serverId)
	{
		return;
	}

	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() > 1)
	{
		delay = atoi(Cmd_Argv(1));
	}

	if (delay)
	{
		sv.restartTime = svs.time + delay * 1000;
		SV_SetConfigstring(CS_WARMUP, va("%i", sv.restartTime));
		return;
	}

	// read in gamestate or just default to GS_PLAYING
	old_gs = atoi(Cvar_VariableString("gamestate"));

	if (Cmd_Argc() > 2)
	{
		new_gs = atoi(Cmd_Argv(2));
	}
	else
	{
		new_gs = GS_PLAYING;
	}

	if (!SV_TransitionGameState(new_gs, old_gs, delay))
	{
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if (sv_maxclients->modified)
	{
		char mapname[MAX_QPATH];

		Com_Printf("sv_maxclients variable change -- restarting.\n");
		// restart the map the slow way
		Q_strncpyz(mapname, Cvar_VariableString("mapname"), sizeof(mapname));

		SV_SpawnServer(mapname);
		return;
	}

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	// don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	sv.serverId = com_frameTime;
	Cvar_Set("sv_serverid", va("%i", sv.serverId));

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state      = SS_LOADING;
	sv.restarting = qtrue;

	SV_RestartGameProgs();

	// run a few frames to allow everything to settle
	for (i = 0; i < GAME_INIT_FRAMES; i++)
	{
		VM_Call(gvm, GAME_RUN_FRAME, svs.time);
		svs.time += FRAMETIME;
	}

	sv.state      = SS_GAME;
	sv.restarting = qfalse;

	// connect and begin all the clients
	for (i = 0 ; i < sv_maxclients->integer ; i++)
	{
		client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if (client->state < CS_CONNECTED)
		{
			continue;
		}

		if (client->netchan.remoteAddress.type == NA_BOT)
		{
			isBot = qtrue;
		}
		else
		{
			isBot = qfalse;
		}

		// add the map_restart command
		SV_AddServerCommand(client, "map_restart\n");

		// connect the client again, without the firstTime flag
		denied = VM_ExplicitArgPtr(gvm, VM_Call(gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot));
		if (denied)
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient(client, denied);
			if (!isBot)
			{
				Com_Printf("SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i);   // bk010125
			}
			continue;
		}

		if (client->state == CS_ACTIVE)
		{
			SV_ClientEnterWorld(client, &client->lastUsercmd);
		}
		else
		{
			// If we don't reset client->lastUsercmd and are restarting during map load,
			// the client will hang because we'll use the last Usercmd from the previous map,
			// which is wrong obviously.
			SV_ClientEnterWorld(client, NULL);
		}
	}

	// run another frame to allow things to look at all the players
	VM_Call(gvm, GAME_RUN_FRAME, svs.time);
	svs.time += FRAMETIME;
}

//===============================================================

/*
==================
SV_TempBanNetAddress
==================
*/
void SV_TempBanNetAddress(netadr_t address, int length)
{
	int i;
	int oldesttime = 0;
	int oldest     = -1;

	for (i = 0; i < MAX_TEMPBAN_ADDRESSES; i++)
	{
		if (!svs.tempBanAddresses[i].endtime || svs.tempBanAddresses[i].endtime < svs.time)
		{
			// found a free slot
			svs.tempBanAddresses[i].adr     = address;
			svs.tempBanAddresses[i].endtime = svs.time + (length * 1000);

			return;
		}
		else
		{
			if (oldest == -1 || oldesttime > svs.tempBanAddresses[i].endtime)
			{
				oldesttime = svs.tempBanAddresses[i].endtime;
				oldest     = i;
			}
		}
	}

	svs.tempBanAddresses[oldest].adr     = address;
	svs.tempBanAddresses[oldest].endtime = svs.time + length;
}

qboolean SV_TempBanIsBanned(netadr_t address)
{
	int i;

	for (i = 0; i < MAX_TEMPBAN_ADDRESSES; i++)
	{
		if (svs.tempBanAddresses[i].endtime && svs.tempBanAddresses[i].endtime > svs.time)
		{
			if (NET_CompareAdr(address, svs.tempBanAddresses[i].adr))
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f(void)
{
	int           i, j, l;
	client_t      *cl;
	playerState_t *ps;
	const char    *s;
	int           ping;

	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	Com_Printf("cpu server utilization: %i %%\n"
	           "avg response time: %i ms\n"
	           "map: %s\n"
	           "num score ping name            lastmsg address               qport rate\n"
	           "--- ----- ---- --------------- ------- --------------------- ----- -----\n",
	           ( int ) svs.stats.cpu,
	           ( int ) svs.stats.avg,
	           sv_mapname->string);

	// FIXME: extend player name lenght (>16 chars) ? - they are printed!
	// FIXME: do a Com_Printf per line! ... create the row at first
	for (i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		Com_Printf("%3i ", i);
		ps = SV_GameClientNum(i);
		Com_Printf("%5i ", ps->persistant[PERS_SCORE]);

		if (cl->state == CS_CONNECTED)
		{
			Com_Printf("CNCT ");
		}
		else if (cl->state == CS_ZOMBIE)
		{
			Com_Printf("ZMBI ");
		}
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf("%4i ", ping);
		}

		Com_Printf("%s", rc(cl->name));
		l = 16 - strlen(cl->name);
		for (j = 0 ; j < l ; j++)
		{
			Com_Printf(" ");
		}

		Com_Printf("%7i ", svs.time - cl->lastPacketTime);

		s = NET_AdrToString(cl->netchan.remoteAddress);
		Com_Printf("%s", s);

		l = 22 - strlen(s);
		for (j = 0 ; j < l ; j++)
		{
			Com_Printf(" ");
		}

		Com_Printf("%5i", cl->netchan.qport);

		Com_Printf(" %5i\n", cl->rate);

	}
	Com_Printf("\n");
}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f(void)
{
	char *p;
	char text[1024];

	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() < 2)
	{
		return;
	}

	strcpy(text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p) - 1] = 0;
	}

	strcat(text, p);

	SV_SendServerCommand(NULL, "chat \"%s\"", text);
}

/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f(void)
{
	svs.nextHeartbeatTime = -9999999;
}

/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f(void)
{
	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	Com_Printf("Server info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
}

/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Systeminfo_f(void)
{
	Com_Printf("System info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE));
}

/*
===========
SV_DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
static void SV_DumpUser_f(void)
{
	client_t *cl;

	// make sure server is running
	if (!com_sv_running->integer)
	{
		Com_Printf("Server is not running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: dumpuser <userid>\n");
		return;
	}

	cl = SV_GetPlayerByName();
	if (!cl)
	{
		Com_Printf("No player found.\n");
		return;
	}

	Com_Printf("userinfo\n--------\n");
	Info_Print(cl->userinfo);
}

/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f(void)
{
	SV_Shutdown("killserver");
}

/*
==================
SV_CompleteMapName
==================
*/
static void SV_CompleteMapName(char *args, int argNum)
{
	if (argNum == 2)
	{
		Field_CompleteFilename("maps", "bsp", qtrue, qfalse);
	}
}

/*
=================
SV_GameCompleteStatus_f
=================
*/
void SV_GameCompleteStatus_f(void)
{
	SV_MasterGameCompleteStatus();
}

//===========================================================

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands(void)
{
	static qboolean initialized;

	if (initialized)
	{
		return;
	}
	initialized = qtrue;

	Cmd_AddCommand("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand("status", SV_Status_f);
	Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand("systeminfo", SV_Systeminfo_f);
	Cmd_AddCommand("dumpuser", SV_DumpUser_f);
	Cmd_AddCommand("map_restart", SV_MapRestart_f);
	Cmd_AddCommand("fieldinfo", SV_FieldInfo_f);
	Cmd_AddCommand("sectorlist", SV_SectorList_f);
	Cmd_AddCommand("gameCompleteStatus", SV_GameCompleteStatus_f);

	Cmd_AddCommand("map", SV_Map_f);
	Cmd_SetCommandCompletionFunc("map", SV_CompleteMapName);
	Cmd_AddCommand("devmap", SV_Map_f);
	Cmd_SetCommandCompletionFunc("devmap", SV_CompleteMapName);

	Cmd_AddCommand("killserver", SV_KillServer_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", SV_ConSay_f);
	}
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands(void)
{
#if 0
	// removing these won't let the server start again
	Cmd_RemoveCommand("heartbeat");
	Cmd_RemoveCommand("kick");
	Cmd_RemoveCommand("banUser");
	Cmd_RemoveCommand("banClient");
	Cmd_RemoveCommand("status");
	Cmd_RemoveCommand("serverinfo");
	Cmd_RemoveCommand("systeminfo");
	Cmd_RemoveCommand("dumpuser");
	Cmd_RemoveCommand("map_restart");
	Cmd_RemoveCommand("sectorlist");
	Cmd_RemoveCommand("say");
#endif
}
