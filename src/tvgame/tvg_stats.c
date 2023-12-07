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
 * @file tvg_stats.c
 */

#include "tvg_local.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

/**
 * @brief G_PrintAccuracyLog
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void G_PrintAccuracyLog(gentity_t *ent, unsigned int dwCommand, int value)
{
	int  i;
	char buffer[2048];

	Q_strncpyz(buffer, "WeaponStats", 2048);

	for (i = WP_KNIFE; i < WP_NUM_WEAPONS; i++)
	{
		if (GetWeaponTableData(i)->indexWeaponStat == WS_MAX)
		{
			continue;
		}

		Q_strcat(buffer, 2048, va(" %i %i %i",
		                          ent->client->pers.playerStats.weaponStats[i].kills,
		                          ent->client->pers.playerStats.weaponStats[i].killedby,
		                          ent->client->pers.playerStats.weaponStats[i].teamkills));
	}

	Q_strcat(buffer, 2048, va(" %i", ent->client->pers.playerStats.selfkills));

	for (i = 0; i < HR_NUM_HITREGIONS; i++)
	{
		Q_strcat(buffer, 2048, va(" %i", ent->client->pers.playerStats.hitRegions[i]));
	}

	Q_strcat(buffer, 2048, va(" %i", 6 /*level.numOidTriggers*/));

	for (i = 0; i < 6 /*level.numOidTriggers*/; i++)
	{
		Q_strcat(buffer, 2048, va(" %i", ent->client->pers.playerStats.objectiveStats[i]));
		Q_strcat(buffer, 2048, va(" %i", ent->client->sess.sessionTeam == TEAM_AXIS ? level.objectiveStatsAxis[i] : level.objectiveStatsAllies[i]));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

/**
 * @brief - send name, team and value when there is a winner - else empty string
 * and TEAM_FREE = 0 (client structure is only used for awards!)
 * - connectedClients have a team but keep the check for TEAM_FREE
 * ... we'll never know for sure, connectedClients are determined in CalculateRanks
 */
void G_BuildEndgameStats(void)
{
	char      buffer[1024];
	int       i, j;
	gclient_t *best         = NULL;
	int       bestClientNum = -1;
	float     mapXP, bestMapXP = 0.f;

//	G_CalcClientAccuracies();
//
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		level.clients[i].hasaward = qfalse;
//	}
//
//	*buffer = '\0';
//
//#ifdef FEATURE_PRESTIGE
//	// most prestigious player - check prestige > 0, then XPs
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		// no reward for non players
//		if (!cl->sess.time_axis && !cl->sess.time_allies)
//		{
//			continue;
//		}
//
//		if (cl->sess.prestige <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.prestige > best->sess.prestige)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.prestige == best->sess.prestige && cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.prestige, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//#endif
//
//	// highest ranking officer - check rank, then medals and XP
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.rank > best->sess.rank)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.rank == best->sess.rank && cl->medals > best->medals)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.rank == best->sess.rank && cl->medals == best->medals && cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i 0 %i ", bestClientNum, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//#ifdef FEATURE_RATING
//	// highest rated player - check skill rating
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		// no reward for non players
//		if (!cl->sess.time_axis && !cl->sess.time_allies)
//		{
//			continue;
//		}
//
//		if (cl->sess.mu - 3 * cl->sess.sigma <= 0)
//		{
//			continue;
//		}
//
//		if (!best || (cl->sess.mu - 3 * cl->sess.sigma) > (best->sess.mu - 3 * best->sess.sigma))
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %.2f %i ", bestClientNum, MIN(Com_RoundFloatWithNDecimal(best->sess.mu - 3 * best->sess.sigma, 2), 50.f), best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//#endif
//
//	// highest experience points - check XP (total in campaign, otherwise this map only then total XP)
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->ps.persistant[PERS_SCORE] <= 0)
//		{
//			continue;
//		}
//
//		if (g_gametype.integer == GT_WOLF_CAMPAIGN)
//		{
//			if (!best || cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
//			{
//				best          = cl;
//				bestClientNum = level.sortedClients[i];
//			}
//		}
//		else
//		{
//			mapXP = 0.f;
//
//			for (j = 0; j < SK_NUM_SKILLS; j++)
//			{
//				mapXP += (cl->sess.skillpoints[j] - cl->sess.startskillpoints[j]);
//			}
//
//			if (!best || mapXP > bestMapXP)
//			{
//				best          = cl;
//				bestMapXP     = mapXP;
//				bestClientNum = level.sortedClients[i];
//			}
//			else if (mapXP == bestMapXP && cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
//			{
//				best          = cl;
//				bestMapXP     = mapXP;
//				bestClientNum = level.sortedClients[i];
//			}
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//
//		if (g_gametype.integer == GT_WOLF_CAMPAIGN)
//		{
//			Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->ps.persistant[PERS_SCORE], best->sess.sessionTeam));
//		}
//		else
//		{
//			Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, (int)bestMapXP, best->sess.sessionTeam));
//		}
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// most highly decorated - check medals then XP
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->medals <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->medals > best->medals)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->medals == best->medals && cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->medals, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	// highest fragger - check kills, then damage given
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->sess.kills <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.kills > best->sess.kills)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.kills == best->sess.kills && cl->sess.damage_given > best->sess.damage_given)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.kills, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// highest skills - check skills points (this map only, min lvl 1)
//	for (i = 0; i < SK_NUM_SKILLS; i++)
//	{
//		best = NULL;
//
//		for (j = 0; j < level.numConnectedClients; j++)
//		{
//			gclient_t *cl = &level.clients[level.sortedClients[j]];
//
//			if (cl->sess.sessionTeam == TEAM_FREE)
//			{
//				continue;
//			}
//
//			if ((cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) <= 0)
//			{
//				continue;
//			}
//
//			if (cl->sess.skill[i] < 1)
//			{
//				continue;
//			}
//
//			if (!best || (cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) > (best->sess.skillpoints[i] - best->sess.startskillpoints[i]))
//			{
//				best          = cl;
//				bestClientNum = level.sortedClients[j];
//			}
//		}
//
//		if (best)
//		{
//			best->hasaward = qtrue;
//			Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, (int)(best->sess.skillpoints[i] - best->sess.startskillpoints[i]), best->sess.sessionTeam));
//		}
//		else
//		{
//			Q_strcat(buffer, 1024, "-1 0 0 ");
//		}
//	}
//
//	best = NULL;
//
//	// highest accuracy
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->acc <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->acc > best->acc)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %.2f %i ", bestClientNum, (double)best->acc, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// highest HS percentage
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->hspct <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->hspct > best->hspct)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %.2f %i ", bestClientNum, (double)best->hspct, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// best survivor - check time played percentage (min 50% of map duration)
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if ((level.time - cl->pers.enterTime) / (float)(level.time - level.intermissiontime) < 0.5f)
//		{
//			continue;
//		}
//
//		if (!best || (cl->sess.time_played / (float)(level.time - cl->pers.enterTime)) > (best->sess.time_played / (float)(level.time - best->pers.enterTime)))
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %.2f %i ", bestClientNum, MIN(100. * best->sess.time_played / (double)(level.time - best->pers.enterTime), 100.), best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// most damage given - check damage given, then damage received
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->sess.damage_given <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.damage_given > best->sess.damage_given)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.damage_given == best->sess.damage_given && cl->sess.damage_received > best->sess.damage_received)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.damage_given, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// most gibs - check gibs, then damage given
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->sess.gibs <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.gibs > best->sess.gibs)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.gibs == best->sess.gibs && cl->sess.damage_given > best->sess.damage_given)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.gibs, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// most selfkill - check selfkills, then deaths
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->sess.self_kills <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.self_kills > best->sess.self_kills)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.self_kills == best->sess.self_kills && cl->sess.deaths > best->sess.deaths)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.self_kills, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// most deaths - check deaths, then damage received
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (cl->sess.deaths <= 0)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.deaths > best->sess.deaths)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.deaths == best->sess.deaths && cl->sess.damage_received > best->sess.damage_received)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.deaths, best->sess.sessionTeam));
//	}
//	else
//	{
//		Q_strcat(buffer, 1024, "-1 0 0 ");
//	}
//
//	best = NULL;
//
//	// I ain't got no friends award - check team kills, then team damage given (min 5 tks)
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (!best || cl->sess.team_kills > best->sess.team_kills)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//		else if (cl->sess.team_kills == best->sess.team_kills && cl->sess.team_damage_given > best->sess.team_damage_given)
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		best->hasaward = qtrue;
//	}
//	Q_strcat(buffer, 1024, va("%i %i %i ", best && best->sess.team_kills >= 5 ? bestClientNum : -1, best ? best->sess.team_kills : 0, best && best->sess.team_kills >= 5 ? best->sess.sessionTeam : TEAM_FREE));
//
//	best = NULL;
//
//	// welcome newbie! award - don't get this if any other award given or > 100 xp (this map)
//	for (i = 0; i < level.numConnectedClients; i++)
//	{
//		gclient_t *cl = &level.clients[level.sortedClients[i]];
//
//		if (cl->sess.sessionTeam == TEAM_FREE)
//		{
//			continue;
//		}
//
//		if (!best || ((cl->ps.persistant[PERS_SCORE] - cl->sess.startxptotal) / (float)(level.time - cl->pers.enterTime)) > (best->ps.persistant[PERS_SCORE] - best->sess.startxptotal) / (float)(level.time - best->pers.enterTime))
//		{
//			best          = cl;
//			bestClientNum = level.sortedClients[i];
//		}
//	}
//
//	if (best)
//	{
//		if ((best->ps.persistant[PERS_SCORE] - best->sess.startxptotal) >= 100.f || best->medals || best->hasaward)
//		{
//			best = NULL;
//		}
//	}
//	Q_strcat(buffer, 1024, va("%i %i %i ", best ? bestClientNum : -1, best ? (int)(best->ps.persistant[PERS_SCORE] - best->sess.startxptotal) : 0, best ? best->sess.sessionTeam : TEAM_FREE));
//
//	trap_SetConfigstring(CS_ENDGAME_STATS, buffer);
}
