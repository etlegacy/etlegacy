/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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
 * @file tvg_match.c
 * @brief Match handling
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"
#include "json.h"

/**
 * @brief Plays specified sound globally.
 * @param[in] sound
 */
void G_globalSound(const char *sound)
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_GLOBAL_SOUND);

	te->s.eventParm = G_SoundIndex(sound);
	te->r.svFlags  |= SVF_BROADCAST;
}

/**
 * @brief Dumps end-of-match info
 * @param dwDumpType
 */
void G_matchInfoDump(unsigned int dwDumpType)
{
	//int       i, ref;
	//gentity_t *ent;
	//gclient_t *cl;

//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		ref = level.sortedClients[i];
//		ent = &g_entities[ref];
//		cl = ent->client;
//
//		if (cl->pers.connected != CON_CONNECTED)
//		{
//			continue;
//		}
//
//		if (dwDumpType == EOM_WEAPONSTATS)
//		{
//			// If client wants to write stats to a file, don't auto send this stuff
//			if (!(cl->pers.clientFlags & CGF_STATSDUMP))
//			{
//				if ((cl->pers.autoaction & AA_STATSALL)
//#ifdef FEATURE_MULTIVIEW
//				    || cl->pers.mvCount > 0
//#endif
//				    )
//				{
//					G_statsall_cmd(ent, 0, qfalse);
//				}
//				else if (cl->sess.sessionTeam != TEAM_SPECTATOR)
//				{
//					if (cl->pers.autoaction & AA_STATSTEAM)
//					{
//						G_statsall_cmd(ent, cl->sess.sessionTeam, qfalse);                // Currently broken.. need to support the overloading of dwCommandID
//					}
//					else
//					{
//						CP(va("ws %s\n", G_createStats(ent)));
//					}
//				}
//				else if (cl->sess.spectatorState != SPECTATOR_FREE)
//				{
//					int pid = cl->sess.spectatorClient;
//
//					if ((cl->pers.autoaction & AA_STATSTEAM))
//					{
//						G_statsall_cmd(ent, level.clients[pid].sess.sessionTeam, qfalse); // Currently broken.. need to support the overloading of dwCommandID
//					}
//					else
//					{
//						CP(va("ws %s\n", G_createStats(g_entities + pid)));
//					}
//				}
//			}
//
//			// Log it
//			if (cl->sess.sessionTeam != TEAM_SPECTATOR)
//			{
//				G_LogPrintf("WeaponStats: %s\n", G_createStats(ent));
//			}
//
//		}
//		else if (dwDumpType == EOM_MATCHINFO)
//		{
//			if (!(cl->pers.clientFlags & CGF_STATSDUMP))
//			{
//				G_printMatchInfo(ent);
//			}
//			if (g_gametype.integer == GT_WOLF_STOPWATCH)
//			{
//				if (g_currentRound.integer == 1)       // We've already missed the switch
//				{
//					CP(va("print \"^3>>> Clock set to: ^7%d:%02d\n\n\n\"",
//					      g_nextTimeLimit.integer,
//					      (int)(60.0f * (g_nextTimeLimit.value - g_nextTimeLimit.integer))));
//				}
//				else
//				{
//					float val = (float)((level.timeCurrent - (level.startTime + level.time - level.intermissiontime)) / 60000.0);
//
//					if (val < g_timelimit.value)
//					{
//						CP(va("print \"^3>>> Objective reached at ^7%d:%02d^3 (original: ^7%d:%02d^3)\n\n\n\"",
//						      (int)val,
//						      (int)(60.0f * (val - (int)val)),
//						      g_timelimit.integer,
//						      (int)(60.0f * (g_timelimit.value - g_timelimit.integer))));
//					}
//					else
//					{
//						CP(va("print \"^3>>> Objective NOT reached in time (^7%d:%02d^3)\n\n\n\"",
//						      g_timelimit.integer,
//						      (int)(60.0f * (g_timelimit.value - g_timelimit.integer))));
//					}
//				}
//			}
//		}
//	}
}

/**
 * @brief Sends a player's stats to the requesting client.
 * @param[in] client
 * @param[in] nType
 */
void TVG_statsPrint(gclient_t *client, int nType, int cooldown)
{
	int  pid;
	char arg[MAX_TOKEN_CHARS];
	const char *cmd = (nType == 0) ? "weaponstats" : ((nType == 1) ? "wstats" : "sgstats");

	// If requesting stats for self, it's easy
	if (trap_Argc() < 2)
	{
		if (client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			pid = client->sess.spectatorClient;
		}
		else
		{
			return;
		}
	}
	else
	{
		// find the player to poll stats
		trap_Argv(1, arg, sizeof(arg));
		if ((pid = TVG_MasterClientNumberFromString(client, arg)) == -1)
		{
			return;
		}
	}

	client->wantsInfoStats[nType].requested          = qtrue;
	client->wantsInfoStats[nType].requestedClientNum = pid;

	// request new stats
	if (level.cmds.lastInfoStatsUpdate + cooldown <= level.time)
	{
		level.cmds.infoStats[nType].valid[pid] = qfalse;
		level.cmds.lastInfoStatsUpdate         = level.time;

		trap_SendServerCommand(-2, va("%s %d\n", cmd, pid));
	}
}
