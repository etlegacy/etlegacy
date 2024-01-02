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
 * @file g_demo_legacy.c
 */

#include "g_local.h"

/**
 * @brief G_DemoStateChanged
 * @param[in] demoState
 * @param[in] demoClientsNum
 */
void G_DemoStateChanged(demoState_t demoState, int demoClientsNum)
{
	char userinfo[MAX_INFO_STRING] = { 0 };
	char *reason;
	int  num;

	level.demoState      = demoState;
	level.demoClientsNum = demoClientsNum;

	switch (demoState)
	{

	case DS_PLAYBACK:
	case DS_WAITINGPLAYBACK:
		trap_Cvar_Set("g_customConfig", "");
		trap_Cvar_Update(&g_customConfig);
		break;

	case DS_RECORDING:
#ifdef FEATURE_OMNIBOT
		num = trap_BotAllocateClient(g_maxclients.integer - 1);

		if (num < 0)
		{
			Com_Printf("Could not add ETL DEMO STATS BOT\n");
			return;
		}

		Info_SetValueForKey(userinfo, "name", "ETL DEMO STATS BOT");
		Info_SetValueForKey(userinfo, "rate", "25000");
		Info_SetValueForKey(userinfo, "snaps", "20");
		Info_SetValueForKey(userinfo, "ip", "localhost");
		Info_SetValueForKey(userinfo, "cl_guid", "ETL-DEMO-STATS-BOT");
		Info_SetValueForKey(userinfo, "tv", "1"); // value 1 - atm only used so omnibot module will not handle this bot

		trap_SetUserinfo(num, userinfo);

		if ((reason = ClientConnect(num, qtrue, qtrue)) != 0)
		{
			Com_Printf("Could not connect ETL DEMO STATS BOT: %s\n", reason);
			return;
		}

		SetTeam(&g_entities[num], "spectator", qtrue, WP_NONE, WP_NONE, qfalse);
#endif
		break;

	// called when demo record stops
	case DS_NONE:
		if (level.demoClientBotNum)
		{
			trap_DropClient(level.demoClientBotNum, "disconnected", 0);
			level.demoClientBotNum = 0;
		}
		break;
	default:
		break;
	}
}

#ifdef FEATURE_OMNIBOT
/**
* @brief G_DemoRequestStats
*
* Managing bot that collects stats during recording
*
* FIXME: There is probably no point requesting continuously during intermission/warmup
*/
static void G_DemoRequestStats(void)
{
	gentity_t  *ent;
	gclient_t  *cl;
	char       buf[MAX_STRING_CHARS] = { 0 };
	static int lastRequestTime       = 0;
	int        i;

	if (!level.demoClientBotNum)
	{
		return;
	}

	ent = &g_entities[level.demoClientBotNum];

	if (lastRequestTime + 5000 <= level.time)
	{
		ent->r.svFlags &= ~SVF_BOT;

		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = level.clients + level.sortedClients[i];

			if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR)
			{
				continue;
			}

			// request weapon stats
			trap_EA_Command(level.demoClientBotNum, va("sgstats %d", level.sortedClients[i]));

			if (g_gamestate.integer == GS_INTERMISSION)
			{
				trap_EA_Command(level.demoClientBotNum, va("imws %d", level.sortedClients[i]));
			}
		}

		ent->r.svFlags |= SVF_BOT;

		// request scoreboard stats (FEATURE_MULTIVIEW request (every 5s) in spectator endframe too)
		G_SendScore(ent);

		lastRequestTime = level.time;
	}

	// imitating real client (this updates server client lastPacketTime so it doesn't get timeouted)
	while (trap_BotGetServerCommand(level.demoClientBotNum, buf, sizeof(buf)))
	{
	}
}
#endif

/**
* @brief G_DemoIntermission
*
* Handles moving clients that are watching server demo to intermission point
*/
static void G_DemoIntermission(void)
{
	gentity_t  *ent;
	static int oldGamestate = -1;
	int        i;

	if (g_gamestate.integer == GS_INTERMISSION && oldGamestate != GS_INTERMISSION)
	{
		level.intermissiontime = level.time;

		FindIntermissionPoint();

		// move all clients to the intermission point
		for (i = level.demoClientsNum; i < level.maxclients; i++)
		{
			ent = g_entities + i;
			if (!ent->inuse)
			{
				continue;
			}
			MoveClientToIntermission(ent, qfalse);
		}
	}

	if (oldGamestate == GS_INTERMISSION && g_gamestate.integer != GS_INTERMISSION)
	{
		level.intermissiontime = 0;
	}

	oldGamestate = g_gamestate.integer;
}

/**
* @brief G_DemoRunFrame
*/
qboolean G_DemoRunFrame(void)
{
	gentity_t *ent;
	int       i;

	if (level.demoState == DS_RECORDING)
	{
#ifdef FEATURE_OMNIBOT
		G_DemoRequestStats();
#endif
		return qfalse;
	}

	if (level.demoState != DS_PLAYBACK && level.demoState != DS_WAITINGPLAYBACK)
	{
		return qfalse;
	}

	trap_Cvar_Set("g_guidCheck", "0");
	trap_Cvar_Set("g_allowVote", "0");
	trap_Cvar_Set("voteFlags", "0");

	G_DemoIntermission();

	// let entities initialize
	if (level.framenum < 7)
	{
		for (i = 0; i < level.num_entities; i++)
		{
			g_entities[i].runthisframe = qfalse;
		}

		// go through all allocated objects
		for (i = 0; i < level.num_entities; i++)
		{
			G_RunEntity(&g_entities[i], level.frameTime);
		}
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		ent = &g_entities[level.sortedClients[i]];

		if (level.sortedClients[i] < level.demoClientsNum)
		{
			ent->health = ent->client->ps.stats[STAT_HEALTH];
			continue;
		}

		ClientEndFrame(ent);
	}

	CheckTeamStatus();

	G_UpdateTeamMapData();

	return qtrue;
}
