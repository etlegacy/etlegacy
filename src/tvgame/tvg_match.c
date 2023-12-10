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
 * @brief Setting initialization
 */
void G_loadMatchGame(void)
{
	int  i, dwBlueOffset, dwRedOffset;
	int  aRandomValues[MAX_REINFSEEDS];
	char strReinfSeeds[MAX_STRING_CHARS];

	G_Printf("Setting MOTD...\n");
	trap_SetConfigstring(CS_CUSTMOTD + 0, server_motd0.string);
	trap_SetConfigstring(CS_CUSTMOTD + 1, server_motd1.string);
	trap_SetConfigstring(CS_CUSTMOTD + 2, server_motd2.string);
	trap_SetConfigstring(CS_CUSTMOTD + 3, server_motd3.string);
	trap_SetConfigstring(CS_CUSTMOTD + 4, server_motd4.string);
	trap_SetConfigstring(CS_CUSTMOTD + 5, server_motd5.string);

	// Voting flags
	G_voteFlags();

	// Set up the random reinforcement seeds for both teams and send to clients
	dwBlueOffset = rand() % MAX_REINFSEEDS;
	dwRedOffset  = rand() % MAX_REINFSEEDS;
	Q_strncpyz(strReinfSeeds, va("%d %d", (dwBlueOffset << REINF_BLUEDELT) + (rand() % (1 << REINF_BLUEDELT)),
	                             (dwRedOffset << REINF_REDDELT)  + (rand() % (1 << REINF_REDDELT))),
	           MAX_STRING_CHARS);

	for (i = 0; i < MAX_REINFSEEDS; i++)
	{
		aRandomValues[i] = (rand() % REINF_RANGE) * aReinfSeeds[i];
		Q_strcat(strReinfSeeds, MAX_STRING_CHARS, va(" %d", aRandomValues[i]));
	}

	level.dwBlueReinfOffset = 1000 * aRandomValues[dwBlueOffset] / aReinfSeeds[dwBlueOffset];
	level.dwRedReinfOffset  = 1000 * aRandomValues[dwRedOffset] / aReinfSeeds[dwRedOffset];

	trap_SetConfigstring(CS_REINFSEEDS, strReinfSeeds);
}

/**
 * @brief Simple alias for sure-fire print :)
 * @param[in] str
 * @param[in] ent
 */
void G_printFull(const char *str, gentity_t *ent)
{
	//if (ent != NULL)
	//{
	//	CP(va("print \"%s\n\"", str));
	//	CP(va("cp \"%s\n\"", str));
	//}
	//else
	//{
	//	AP(va("print \"%s\n\"", str));
	//	AP(va("cp \"%s\n\"", str));
	//}
}

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
 * @brief G_globalSoundEnum
 * @param[in] sound
 */
void G_globalSoundEnum(int sound)
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_GLOBAL_SOUND);

	te->s.eventParm = sound;
	te->r.svFlags  |= SVF_BROADCAST;
}

/**
 * @brief G_delayPrint
 * @param[in,out] dpent
 */
void G_delayPrint(gentity_t *dpent)
{
	int      think_next = 0;
	qboolean fFree      = qtrue;

	switch (dpent->spawnflags)
	{
	case DP_PAUSEINFO:
		if (level.match_pause > PAUSE_UNPAUSING)
		{
			int cSeconds = match_timeoutlength.integer * 1000 - (level.time - dpent->timestamp);

			if (cSeconds > 1000)
			{
				AP(va("cp \"^3Match resuming in ^1%d^3 seconds!\n\"", cSeconds / 1000));
				think_next = level.time + 15000;
				fFree      = qfalse;
			}
			else
			{
				level.match_pause = PAUSE_UNPAUSING;
				AP("print \"^3Match resuming in 10 seconds!\n\"");
				G_globalSound("sound/osp/prepare.wav");
				G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
			}
		}
		break;
	case DP_UNPAUSING:
		if (level.match_pause == PAUSE_UNPAUSING)
		{
			int cSeconds = 11 * 1000 - (level.time - dpent->timestamp);

			if (cSeconds > 1000)
			{
				AP(va("cp \"^3Match resuming in ^1%d^3 seconds!\n\"", cSeconds / 1000));
				think_next = level.time + 1000;
				fFree      = qfalse;
			}
			else
			{
				level.match_pause = PAUSE_NONE;
				G_globalSound("sound/osp/fight.wav");
				G_printFull("^1FIGHT!", NULL);
				trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime + level.timeDelta));
				level.server_settings &= ~CV_SVS_PAUSE;
				trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
			}
		}
		break;
	default:
		break;
	}

	dpent->nextthink = think_next;
	if (fFree)
	{
		dpent->think = 0;
		G_FreeEntity(dpent);
	}
}

static char *pszDPInfo[] =
{
	"DPRINTF_PAUSEINFO",
	"DPRINTF_UNPAUSING",
	"DPRINTF_CONNECTINFO",
	"DPRINTF_MVSPAWN",
	"DPRINTF_UNK1",
	"DPRINTF_UNK2",
	"DPRINTF_UNK3",
	"DPRINTF_UNK4",
	"DPRINTF_UNK5"
};

/**
 * @brief G_spawnPrintf
 * @param[in] print_type
 * @param[in] print_time
 * @param[in] owner
 */
void G_spawnPrintf(int print_type, int print_time, gentity_t *owner)
{
	gentity_t *ent;

	ent = G_Spawn();

	ent->classname  = pszDPInfo[print_type];
	ent->clipmask   = 0;
	ent->parent     = owner;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags  |= EF_NODRAW;
	ent->s.eType    = ET_ITEM;

	ent->spawnflags = print_type;       // Tunnel in DP enum
	ent->timestamp  = level.time;       // Time entity was created

	ent->nextthink = print_time;
	ent->think     = G_delayPrint;
}

/**
 * @brief G_addStats
 * @param[in,out] targ
 * @param[in,out] attacker
 * @param[in] damage
 * @param[in] mod
 */
void G_addStats(gentity_t *targ, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	if (!targ || !targ->client)
	{
		return;
	}

	// Keep track of only active player-to-player interactions in a real game
	if (
#ifndef DEBUG_STATS
		g_gamestate.integer != GS_PLAYING ||
#endif
		mod == MOD_SWITCHTEAM || (targ->client->ps.pm_flags & PMF_LIMBO))
	{
		return;
	}

	// Special hack for intentional gibbage
	if (targ->health <= 0 && targ->client->ps.pm_type == PM_DEAD)
	{
		if (attacker && attacker->client)
		{
			weapon_t weap = GetMODTableData(mod)->weaponIcon;

			// don't count hits/shots for hitscan weapons
			if (!GetWeaponTableData(weap)->splashDamage)
			{
				int x;

				x = attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].atts--;

				if (x < 1)
				{
					attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].atts = 1;
				}
			}

			if (targ->health <= FORCE_LIMBO_HEALTH)
			{
				if (targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
				{
					attacker->client->sess.gibs++;
				}
				else if (targ != attacker)
				{
					attacker->client->sess.team_gibs++;
				}
			}
		}

		return;
	}

	// gibs from explosions when player was still alive
	if (targ->health <= GIB_HEALTH && attacker && attacker->client)
	{
		if (targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		{
			attacker->client->sess.gibs++;
		}
		else if (targ != attacker)
		{
			attacker->client->sess.team_gibs++;
		}
	}

	//  G_Printf("mod: %d, Index: %d, dmg: %d\n", mod, G_weapStatIndex_MOD(mod), dmg_ref);

	// Selfkills only affect the player specifically
	if (targ == attacker || !attacker || !attacker->client || mod == MOD_SUICIDE) // FIXME: inspect world kills count as selfkill?
	{
		if (targ->health <= 0)
		{
			targ->client->sess.self_kills++;
		}
#ifdef DEBUG_STATS
		if (!attacker || !attacker->client)
#endif
		return;
	}

	// Player team stats
	if (targ->client->sess.sessionTeam == attacker->client->sess.sessionTeam)
	{
		attacker->client->sess.team_damage_given += damage;
		targ->client->sess.team_damage_received  += damage;

		if (targ->health <= 0)
		{
			attacker->client->sess.team_kills++;
		}
#ifndef DEBUG_STATS
		return;
#endif
	}

	// General player stats
	if (mod != MOD_SYRINGE)
	{
		attacker->client->sess.damage_given += damage;
		targ->client->sess.damage_received  += damage;

		if (targ->health <= 0)
		{
			attacker->client->sess.kills++;
			targ->client->sess.deaths++;
		}
	}

	// Player weapon stats
	if (damage > 0)
	{
		attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].hits++;
	}
	if (targ->health <= 0)
	{
		attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].kills++;
		targ->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].deaths++;
	}
}

/**
 * @brief Records weapon headshots
 * @param[in,out] attacker
 * @param[in] mod
 */
void G_addStatsHeadShot(gentity_t *attacker, meansOfDeath_t mod)
{
#ifndef DEBUG_STATS
	if (g_gamestate.integer != GS_PLAYING)
	{
		return;
	}
#endif
	if (!attacker || !attacker->client)
	{
		return;
	}

	attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].headshots++;
}

void G_createStatsJson(gentity_t *ent, void *target)
{
	unsigned int i;
	gclient_t    *cl;
	cJSON        *tmp, *tmp2;
	qboolean     weapons = qfalse;

	if (!ent || !ent->client)
	{
		return;
	}

	cl = ent->client;

	if (cl->pers.connected != CON_CONNECTED)
	{
		return;
	}

	cJSON_AddNumberToObject(target, "ent", (int)(ent - g_entities));
	cJSON_AddNumberToObject(target, "rounds", ent->client->sess.rounds);

	// workaround to always hide previous map stats in warmup
	// Stats will be cleared correctly when the match actually starts
	if ((g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN) &&
	    !(g_gametype.integer == GT_WOLF_STOPWATCH))
	{
		return;
	}

	// Add weapon stats as necessary
	// The client also expects stats when kills are above 0
	tmp = cJSON_AddObjectToObject(target, "weapons");
	for (i = WS_KNIFE; i < WS_MAX; i++)
	{
		if (ent->client->sess.aWeaponStats[i].atts || ent->client->sess.aWeaponStats[i].hits ||
		    ent->client->sess.aWeaponStats[i].deaths || ent->client->sess.aWeaponStats[i].kills)
		{
			weapons = qtrue;
			tmp2    = cJSON_AddObjectToObject(tmp, aWeaponInfo[i].pszCode);
			cJSON_AddNumberToObject(tmp2, "hits", ent->client->sess.aWeaponStats[i].hits);
			cJSON_AddNumberToObject(tmp2, "atts", ent->client->sess.aWeaponStats[i].atts);
			cJSON_AddNumberToObject(tmp2, "kills", ent->client->sess.aWeaponStats[i].kills);
			cJSON_AddNumberToObject(tmp2, "deaths", ent->client->sess.aWeaponStats[i].deaths);
			cJSON_AddNumberToObject(tmp2, "headshots", ent->client->sess.aWeaponStats[i].headshots);
		}
	}

	// Additional info
	if (weapons)
	{
		tmp2 = cJSON_AddObjectToObject(tmp, "_shared");
		cJSON_AddNumberToObject(tmp2, "damage_given", ent->client->sess.damage_given);
		cJSON_AddNumberToObject(tmp2, "damage_received", ent->client->sess.damage_received);
		cJSON_AddNumberToObject(tmp2, "team_damage_given", ent->client->sess.team_damage_given);
		cJSON_AddNumberToObject(tmp2, "team_damage_received", ent->client->sess.team_damage_received);
		cJSON_AddNumberToObject(tmp2, "gibs", ent->client->sess.gibs);
		cJSON_AddNumberToObject(tmp2, "self_kills", ent->client->sess.self_kills);
		cJSON_AddNumberToObject(tmp2, "team_kills", ent->client->sess.team_kills);
		cJSON_AddNumberToObject(tmp2, "team_gibs", ent->client->sess.team_gibs);
		cJSON_AddNumberToObject(tmp2, "play_time", (ent->client->sess.time_axis + ent->client->sess.time_allies) == 0 ? 0 : 100.0 * ent->client->sess.time_played / (ent->client->sess.time_axis + ent->client->sess.time_allies));
	}


	tmp = cJSON_AddObjectToObject(target, "skills");
	// Add skill points as necessary
	if ((g_gametype.integer == GT_WOLF_CAMPAIGN && g_xpSaver.integer) ||
	    (g_gametype.integer == GT_WOLF_CAMPAIGN && (g_campaigns[level.currentCampaign].current != 0 && !level.newCampaign)) ||
	    (g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer != 0))
	{
		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			if (ent->client->sess.skillpoints[i] != 0.f) // Skillpoints can be negative
			{
				cJSON_AddNumberToObject(tmp, skillTable[i].skillNames, (int)ent->client->sess.skillpoints[i]);
			}
		}
	}
	else
	{
		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			// current map XPs only
			if ((ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i]) != 0.f) // Skillpoints can be negative
			{
				cJSON_AddNumberToObject(tmp, skillTable[i].skillNames, (int)(ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i]));
			}
		}
	}
}

/**
 * @brief Generates weapon stat info for given ent
 * @param[in] refEnt
 * @return
 */
char *G_createStats(gentity_t *ent)
{
//	unsigned int i, dwWeaponMask = 0, dwSkillPointMask = 0;
//	char         strWeapInfo[MAX_STRING_CHARS]  = { 0 };
//	char         strSkillInfo[MAX_STRING_CHARS] = { 0 };
//	gclient_t    *cl;
//
//	if (!ent || !ent->client)
//	{
//		return NULL;
//	}
//
//	cl = ent->client;
//
//	if (cl->pers.connected != CON_CONNECTED)
//	{
//		return NULL;
//	}
//
//	// Add weapon stats as necessary
//	// The client also expects stats when kills are above 0
//	for (i = WS_KNIFE; i < WS_MAX; i++)
//	{
//		if (ent->client->sess.aWeaponStats[i].atts || ent->client->sess.aWeaponStats[i].hits ||
//		    ent->client->sess.aWeaponStats[i].deaths || ent->client->sess.aWeaponStats[i].kills)
//		{
//			dwWeaponMask |= (1 << i);
//			Q_strcat(strWeapInfo, sizeof(strWeapInfo), va(" %d %d %d %d %d",
//			                                              ent->client->sess.aWeaponStats[i].hits, ent->client->sess.aWeaponStats[i].atts,
//			                                              ent->client->sess.aWeaponStats[i].kills, ent->client->sess.aWeaponStats[i].deaths,
//			                                              ent->client->sess.aWeaponStats[i].headshots));
//		}
//	}
//
//	// Additional info
//	// Only send these when there are some weaponstats. This is what the client expects.
//	if (dwWeaponMask != 0)
//	{
//		Q_strcat(strWeapInfo, sizeof(strWeapInfo), va(" %d %d %d %d %d %d %d %d %.1f",
//		                                              ent->client->sess.damage_given,
//		                                              ent->client->sess.damage_received,
//		                                              ent->client->sess.team_damage_given,
//		                                              ent->client->sess.team_damage_received,
//		                                              ent->client->sess.gibs,
//		                                              ent->client->sess.self_kills,
//		                                              ent->client->sess.team_kills,
//		                                              ent->client->sess.team_gibs,
//		                                              (ent->client->sess.time_axis + ent->client->sess.time_allies) == 0 ? 0 : 100.0 * ent->client->sess.time_played / (ent->client->sess.time_axis + ent->client->sess.time_allies)
//		                                              ));
//	}
//
//	// Add skillpoints as necessary
//	if ((g_gametype.integer == GT_WOLF_CAMPAIGN && g_xpSaver.integer) ||
//	    (g_gametype.integer == GT_WOLF_CAMPAIGN && (g_campaigns[level.currentCampaign].current != 0 && !level.newCampaign)) ||
//	    (g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer != 0))
//	{
//		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
//		{
//			if (ent->client->sess.skillpoints[i] != 0.f) // Skillpoints can be negative
//			{
//				dwSkillPointMask |= (1 << i);
//				Q_strcat(strSkillInfo, sizeof(strSkillInfo), va(" %d", (int)ent->client->sess.skillpoints[i]));
//			}
//		}
//	}
//	else
//	{
//		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
//		{
//			// current map XPs only
//			if ((ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i]) != 0.f) // Skillpoints can be negative
//			{
//				dwSkillPointMask |= (1 << i);
//				Q_strcat(strSkillInfo, sizeof(strSkillInfo), va(" %d", (int)(ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i])));
//			}
//		}
//	}
//
//	// workaround to always hide previous map stats in warmup
//	// Stats will be cleared correctly when the match actually starts
//	if ((g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN) &&
//	    !(g_gametype.integer == GT_WOLF_STOPWATCH))
//	{
//		dwWeaponMask     = 0;
//		strWeapInfo[0]   = '\0';
//		dwSkillPointMask = 0;
//		strSkillInfo[0]  = '\0';
//	}
//
//#if defined(FEATURE_RATING) && defined (FEATURE_PRESTIGE)
//	return(va("%d %d %d%s %d%s %.2f %.2f %d",
//#elif defined(FEATURE_RATING)
//	return(va("%d %d %d%s %d%s %.2f %.2f",
//#elif defined (FEATURE_PRESTIGE)
//	return(va("%d %d %d%s %d%s %d",
//#else
//	return (va("%d %d %d%s %d%s",
//#endif
//	          (int)(ent - g_entities),
//	          ent->client->sess.rounds,
//	          dwWeaponMask,
//	          strWeapInfo,
//	          dwSkillPointMask,
//	          strSkillInfo
//#ifdef FEATURE_RATING
//	          ,
//	          ent->client->sess.mu - 3 * ent->client->sess.sigma,
//	          ent->client->sess.mu - 3 * ent->client->sess.sigma - (ent->client->sess.oldmu - 3 * ent->client->sess.oldsigma)
//#endif
//#ifdef FEATURE_PRESTIGE
//	          ,
//	          ent->client->sess.prestige
//#endif
//	          ));
return NULL;
}

/**
 * @brief Resets player's current stats (session related only)
 * @param[in] nClient
 */
void G_deleteStats(int nClient)
{
	gclient_t *cl = &level.clients[nClient];

	cl->sess.damage_given         = 0;
	cl->sess.damage_received      = 0;
	cl->sess.deaths               = 0;
	cl->sess.rounds               = 0;
	cl->sess.kills                = 0;
	cl->sess.gibs                 = 0;
	cl->sess.self_kills           = 0;
	cl->sess.team_kills           = 0;
	cl->sess.team_gibs            = 0;
	cl->sess.team_damage_given    = 0;
	cl->sess.team_damage_received = 0;
	cl->sess.time_axis            = 0;
	cl->sess.time_allies          = 0;
	cl->sess.time_played          = 0;

	cl->sess.startskillpoints[SK_BATTLE_SENSE]                             = 0;
	cl->sess.startskillpoints[SK_EXPLOSIVES_AND_CONSTRUCTION]              = 0;
	cl->sess.startskillpoints[SK_FIRST_AID]                                = 0;
	cl->sess.startskillpoints[SK_SIGNALS]                                  = 0;
	cl->sess.startskillpoints[SK_LIGHT_WEAPONS]                            = 0;
	cl->sess.startskillpoints[SK_HEAVY_WEAPONS]                            = 0;
	cl->sess.startskillpoints[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] = 0;

	Com_Memset(&cl->sess.aWeaponStats, 0, sizeof(cl->sess.aWeaponStats));
	trap_Cvar_Set(va("wstats%i", nClient), va("%d", nClient));
}

/**
 * @brief Parses weapon stat info for given ent
 * The given string must be space delimited and contain only integers
 *
 * @param object json object that holds the data
 */
void G_parseStatsJson(void *object)
{
	gclient_t *cl;
	cJSON     *tmp, *weapons;
	int       i, dwClientID;
	qboolean  weaponsFound = qfalse;

	dwClientID = Q_ReadIntValueJson(object, "ent");

	if (dwClientID > MAX_CLIENTS)
	{
		return;
	}

	cl = &level.clients[dwClientID];

	cl->sess.rounds = Q_ReadIntValueJson(object, "rounds");

	weapons = cJSON_GetObjectItem(object, "weapons");
	for (i = WS_KNIFE; i < WS_MAX; i++)
	{
		tmp = cJSON_GetObjectItem(weapons, aWeaponInfo[i].pszCode);

		if (tmp)
		{
			weaponsFound = qtrue;
			cl->sess.aWeaponStats[i].hits = Q_ReadIntValueJson(tmp, "hits");
			cl->sess.aWeaponStats[i].atts = Q_ReadIntValueJson(tmp, "atts");
			cl->sess.aWeaponStats[i].kills = Q_ReadIntValueJson(tmp, "kills");
			cl->sess.aWeaponStats[i].deaths = Q_ReadIntValueJson(tmp, "deaths");
			cl->sess.aWeaponStats[i].headshots = Q_ReadIntValueJson(tmp, "headshots");
		}
	}

	if (weaponsFound)
	{
		tmp = cJSON_GetObjectItem(weapons, "_shared");

		if (tmp)
		{
			cl->sess.damage_given = Q_ReadIntValueJson(tmp, "damage_given");
			cl->sess.damage_received = Q_ReadIntValueJson(tmp, "damage_received");
			cl->sess.team_damage_given = Q_ReadIntValueJson(tmp, "team_damage_given");
			cl->sess.team_damage_received = Q_ReadIntValueJson(tmp, "team_damage_received");
		}
		else
		{
			Q_JsonError("Missing _shared object\n");
		}
	}
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

/**
 * @brief G_resetRoundState
 */
void G_resetRoundState(void)
{
	if (g_gametype.integer == GT_WOLF_STOPWATCH)
	{
		trap_Cvar_Set("g_currentRound", "0");
	}
	else if (g_gametype.integer == GT_WOLF_LMS)
	{
		trap_Cvar_Set("g_currentRound", "0");
		trap_Cvar_Set("g_lms_currentMatch", "0");
	}
}

/**
 * @brief G_resetModeState
 */
void G_resetModeState(void)
{
	if (g_gametype.integer == GT_WOLF_STOPWATCH)
	{
		trap_Cvar_Set("g_nextTimeLimit", "0");
	}
	else if (g_gametype.integer == GT_WOLF_LMS)
	{
		trap_Cvar_Set("g_axiswins", "0");
		trap_Cvar_Set("g_alliedwins", "0");
	}
}
