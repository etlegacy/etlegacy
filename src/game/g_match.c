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
 * @file g_match.c
 * @brief Match handling
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

/**
 * @brief G_initMatch
 */
void G_initMatch(void)
{
	int i;

	for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
	{
		G_teamReset(i, qfalse);
	}
}

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
	if (ent != NULL)
	{
		CP(va("print \"%s\n\"", str));
		CP(va("cp \"%s\n\"", str));
	}
	else
	{
		AP(va("print \"%s\n\"", str));
		AP(va("cp \"%s\n\"", str));
	}
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
#ifdef FEATURE_MULTIVIEW
	case DP_MVSPAWN:
	{
		int       i;
		gentity_t *ent;

		for (i = 0; i < level.numConnectedClients; i++)
		{
			ent = g_entities + level.sortedClients[i];

			if (ent->client->pers.mvReferenceList == 0)
			{
				continue;
			}
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				continue;
			}
			G_smvRegenerateClients(ent, ent->client->pers.mvReferenceList);
		}
	}
	break;
#endif
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
			int x;

			x = attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].atts--;

			if (x < 1)
			{
				attacker->client->sess.aWeaponStats[GetMODTableData(mod)->indexWeaponStat].atts = 1;
			}

			if (targ->health <= FORCE_LIMBO_HEALTH)
			{
				if (targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
				{
					attacker->client->sess.gibs++;
				}
				else
				{
					attacker->client->sess.team_gibs++;
				}
			}
		}

		return;
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

/**
 * @brief Generates weapon stat info for given ent
 * @param[in] refEnt
 * @return
 */
char *G_createStats(gentity_t *ent)
{
	unsigned int i, dwWeaponMask = 0, dwSkillPointMask = 0;
	char         strWeapInfo[MAX_STRING_CHARS]  = { 0 };
	char         strSkillInfo[MAX_STRING_CHARS] = { 0 };
	gclient_t    *cl;

	if (!ent || !ent->client)
	{
		return NULL;
	}

	cl = ent->client;

	if (cl->pers.connected != CON_CONNECTED)
	{
		return NULL;
	}

	// Add weapon stats as necessary
	// The client also expects stats when kills are above 0
	for (i = WS_KNIFE; i < WS_MAX; i++)
	{
		if (ent->client->sess.aWeaponStats[i].atts || ent->client->sess.aWeaponStats[i].hits ||
		    ent->client->sess.aWeaponStats[i].deaths || ent->client->sess.aWeaponStats[i].kills)
		{
			dwWeaponMask |= (1 << i);
			Q_strcat(strWeapInfo, sizeof(strWeapInfo), va(" %d %d %d %d %d",
			                                              ent->client->sess.aWeaponStats[i].hits, ent->client->sess.aWeaponStats[i].atts,
			                                              ent->client->sess.aWeaponStats[i].kills, ent->client->sess.aWeaponStats[i].deaths,
			                                              ent->client->sess.aWeaponStats[i].headshots));
		}
	}

	// Additional info
	// Only send these when there are some weaponstats. This is what the client expects.
	if (dwWeaponMask != 0)
	{
		Q_strcat(strWeapInfo, sizeof(strWeapInfo), va(" %d %d %d %d %d %d %d %d %.1f",
		                                              ent->client->sess.damage_given,
		                                              ent->client->sess.damage_received,
		                                              ent->client->sess.team_damage_given,
		                                              ent->client->sess.team_damage_received,
		                                              ent->client->sess.gibs,
		                                              ent->client->sess.self_kills,
		                                              ent->client->sess.team_kills,
		                                              ent->client->sess.team_gibs,
		                                              (ent->client->sess.time_axis + ent->client->sess.time_allies) == 0 ? 0 : 100.0 * ent->client->sess.time_played / (ent->client->sess.time_axis + ent->client->sess.time_allies)
		                                              ));
	}

	// Add skillpoints as necessary
	if ((g_gametype.integer == GT_WOLF_CAMPAIGN && g_xpSaver.integer) ||
		(g_gametype.integer == GT_WOLF_CAMPAIGN && (g_campaigns[level.currentCampaign].current != 0 && !level.newCampaign)) ||
	    (g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer != 0))
	{
		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			if (ent->client->sess.skillpoints[i] != 0.f) // Skillpoints can be negative
			{
				dwSkillPointMask |= (1 << i);
				Q_strcat(strSkillInfo, sizeof(strSkillInfo), va(" %d", (int)ent->client->sess.skillpoints[i]));
			}
		}
	}
#ifdef FEATURE_PRESTIGE
	else if (g_prestige.integer && g_gametype.integer != GT_WOLF_CAMPAIGN && g_gametype.integer != GT_WOLF_STOPWATCH && g_gametype.integer != GT_WOLF_LMS)
	{
		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			if (ent->client->sess.skillpoints[i] != 0.f) // Skillpoints can be negative
			{
				dwSkillPointMask |= (1 << i);
				Q_strcat(strSkillInfo, sizeof(strSkillInfo), va(" %d %d", (int)ent->client->sess.skillpoints[i], (int)(ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i])));
			}
		}
	}
#endif
	else
	{
		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			// current map XPs only
			if ((ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i]) != 0.f) // Skillpoints can be negative
			{
				dwSkillPointMask |= (1 << i);
				Q_strcat(strSkillInfo, sizeof(strSkillInfo), va(" %d", (int)(ent->client->sess.skillpoints[i] - ent->client->sess.startskillpoints[i])));
			}
		}
	}

	// workaround to always hide previous map stats in warmup
	// Stats will be cleared correctly when the match actually starts
	if ((g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN) &&
	    !(g_gametype.integer == GT_WOLF_STOPWATCH && g_currentRound.integer == 1))
	{
		dwWeaponMask     = 0;
		strWeapInfo[0]   = '\0';
		dwSkillPointMask = 0;
		strSkillInfo[0]  = '\0';
	}

#if defined(FEATURE_RATING) && defined (FEATURE_PRESTIGE)
	return(va("%d %d %d%s %d%s %.2f %.2f %d",
#elif defined(FEATURE_RATING)
	return(va("%d %d %d%s %d%s %.2f %.2f",
#elif defined (FEATURE_PRESTIGE)
	return(va("%d %d %d%s %d%s %d",
#else
	return (va("%d %d %d%s %d%s",
#endif
	          (int)(ent - g_entities),
	          ent->client->sess.rounds,
	          dwWeaponMask,
	          strWeapInfo,
	          dwSkillPointMask,
	          strSkillInfo
#ifdef FEATURE_RATING
	          ,
	          ent->client->sess.mu - 3 * ent->client->sess.sigma,
	          ent->client->sess.mu - 3 * ent->client->sess.sigma - (ent->client->sess.oldmu - 3 * ent->client->sess.oldsigma)
#endif
#ifdef FEATURE_PRESTIGE
	          ,
	          ent->client->sess.prestige
#endif
	          ));
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

#ifdef FEATURE_RATING
	// skill rating
	cl->sess.mu       = MU;
	cl->sess.sigma    = SIGMA;
	cl->sess.oldmu    = cl->sess.mu;
	cl->sess.oldsigma = cl->sess.sigma;
#endif
#ifdef FEATURE_PRESTIGE
	cl->sess.prestige = 0;
#endif
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
 * @param pszStatsInfo
 */
void G_parseStats(const char *pszStatsInfo)
{
	gclient_t  *cl;
	const char *tmp = pszStatsInfo;
	int        i, dwWeaponMask, dwClientID = Q_atoi(pszStatsInfo);

	if (dwClientID > MAX_CLIENTS)
	{
		return;
	}

	cl = &level.clients[dwClientID];

#define GETVAL(x) if ((tmp = strchr(tmp, ' ')) == NULL) { return; } x = Q_atoi(++tmp);

	GETVAL(cl->sess.rounds);
	GETVAL(dwWeaponMask);
	for (i = WS_KNIFE; i < WS_MAX; i++)
	{
		if (dwWeaponMask & (1 << i))
		{
			GETVAL(cl->sess.aWeaponStats[i].hits);
			GETVAL(cl->sess.aWeaponStats[i].atts);
			GETVAL(cl->sess.aWeaponStats[i].kills);
			GETVAL(cl->sess.aWeaponStats[i].deaths);
			GETVAL(cl->sess.aWeaponStats[i].headshots);
		}
	}

	// These only gets generated when there are some weaponstats.
	// This is what the client expects.
	if (dwWeaponMask != 0)
	{
		GETVAL(cl->sess.damage_given);
		GETVAL(cl->sess.damage_received);
		GETVAL(cl->sess.team_damage_given);
		GETVAL(cl->sess.team_damage_received);
	}
}

/**
 * @brief Prints current player match info.
 * @param[in] ent
 *
 * @todo FIXME: put the pretty print on the client
 */
void G_printMatchInfo(gentity_t *ent)
{
	int       i, j, cnt = 0, eff, time_eff;
	int       tot_timex, tot_timel, tot_timep, tot_kills, tot_deaths, tot_gibs, tot_sk, tot_tk, tot_tg, tot_dg, tot_dr, tot_tdg, tot_tdr, tot_xp;
	gclient_t *cl;
	gentity_t *cl_ent;
	char      *ref;
	char      guid[MAX_GUID_LENGTH + 1];
	char      n2[MAX_STRING_CHARS];

	for (i = TEAM_AXIS; i <= TEAM_SPECTATOR; i++)
	{
		if (TeamCount(-1, i) == 0)
		{
			continue;
		}

		tot_timex  = 0;
		tot_timel  = 0;
		tot_timep  = 0;
		tot_kills  = 0;
		tot_deaths = 0;
		tot_gibs   = 0;
		tot_sk     = 0;
		tot_tk     = 0;
		tot_tg     = 0;
		tot_dg     = 0;
		tot_dr     = 0;
		tot_tdg    = 0;
		tot_tdr    = 0;
		tot_xp     = 0;

		CP("sc \"\n\"");
#ifdef FEATURE_RATING
		CP("sc \"^7GUID      TEAM       Player         ^1 TmX^$ TmL^7 TmP^7 Kll Dth Gib  SK  TK  TG^7 Eff^2    DG^1    DR^6  TDG^$  TDR^3  Score^8  Rating^5  Delta\n\"");
		CP("sc \"^7------------------------------------------------------------------------------------------------------------------------\n\"");
#else
		CP("sc \"^7GUID      TEAM       Player         ^1 TmX^$ TmL^7 TmP^7 Kll Dth Gib  SK  TK  TG^7 Eff^2    DG^1    DR^6  TDG^$  TDR^3  Score\n\"");
		CP("sc \"^7---------------------------------------------------------------------------------------------------------\n\"");
#endif

		for (j = 0; j < level.numConnectedClients; j++)
		{
			cl     = level.clients + level.sortedClients[j];
			cl_ent = g_entities + level.sortedClients[j];

			if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam != i)
			{
				continue;
			}

			SanitizeString(cl->pers.cl_guid, guid, qfalse);
			if (cl_ent->r.svFlags & SVF_BOT)
			{
				guid[9] = '\0';
			}
			else
			{
				guid[8] = '\0';
				strcat(guid, "*");
			}

			SanitizeString(cl->pers.netname, n2, qfalse);
			n2[15] = 0;

			ref         = "^7";
			tot_timex  += cl->sess.time_axis;
			tot_timel  += cl->sess.time_allies;
			tot_timep  += cl->sess.time_played;
			tot_kills  += cl->sess.kills;
			tot_deaths += cl->sess.deaths;
			tot_gibs   += cl->sess.gibs;
			tot_sk     += cl->sess.self_kills;
			tot_tk     += cl->sess.team_kills;
			tot_tg     += cl->sess.team_gibs;
			tot_dg     += cl->sess.damage_given;
			tot_dr     += cl->sess.damage_received;
			tot_tdg    += cl->sess.team_damage_given;
			tot_tdr    += cl->sess.team_damage_received;
			tot_xp     += (g_gametype.integer == GT_WOLF_LMS) ? cl->ps.persistant[PERS_SCORE] : cl->ps.stats[STAT_XP];

			eff = (cl->sess.deaths + cl->sess.kills == 0) ? 0 : 100 * cl->sess.kills / (cl->sess.deaths + cl->sess.kills);
			if (eff < 0)
			{
				eff = 0;
			}

			time_eff = (cl->sess.time_axis + cl->sess.time_allies == 0) ? 0 : 100 * cl->sess.time_played / (cl->sess.time_axis + cl->sess.time_allies);

			if (ent->client == cl ||
				(ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
				 ent->client->sess.spectatorState == SPECTATOR_FOLLOW &&
				 ent->client->sess.spectatorClient == level.sortedClients[j]))
			{
				ref = "^3";
			}

			cnt++;
#ifdef FEATURE_RATING
			trap_SendServerCommand(ent - g_entities, va("sc \"%-9s %-14s %s%-15s^1%4d^$%4d^7%s%4d^3%4d%4d%4d%4d%4d%4d%s%4d^2%6d^1%6d^6%5d^$%5d^3%7d^8%8.2f^5%+7.2f\n\"",
#else
			trap_SendServerCommand(ent - g_entities, va("sc \"%-9s %-14s %s%-15s^1%4d^$%4d^7%s%4d^3%4d%4d%4d%4d%4d%4d%s%4d^2%6d^1%6d^6%5d^$%5d^3%7d\n\"",
#endif
			                                            guid,
			                                            aTeams[i],
			                                            ref,
			                                            n2,
			                                            cl->sess.time_axis / 60000,
			                                            cl->sess.time_allies / 60000,
			                                            ref,
			                                            time_eff,
			                                            cl->sess.kills,
			                                            cl->sess.deaths,
			                                            cl->sess.gibs,
			                                            cl->sess.self_kills,
			                                            cl->sess.team_kills,
			                                            cl->sess.team_gibs,
			                                            ref,
			                                            eff,
			                                            cl->sess.damage_given,
			                                            cl->sess.damage_received,
			                                            cl->sess.team_damage_given,
			                                            cl->sess.team_damage_received,
			                                            (g_gametype.integer == GT_WOLF_LMS) ? cl->ps.persistant[PERS_SCORE] : cl->ps.stats[STAT_XP]
#ifdef FEATURE_RATING
			                                            ,
			                                            cl->sess.mu - 3 * cl->sess.sigma,
			                                            (cl->sess.mu - 3 * cl->sess.sigma) - (cl->sess.oldmu - 3 * cl->sess.oldsigma)
#endif
			                                            ));
		}

		eff = (tot_kills + tot_deaths == 0) ? 0 : 100 * tot_kills / (tot_kills + tot_deaths);
		if (eff < 0)
		{
			eff = 0;
		}

		time_eff = (tot_timex + tot_timel == 0) ? 0 : 100 * tot_timep / (tot_timex + tot_timel);

#ifdef FEATURE_RATING
		CP("sc \"^7------------------------------------------------------------------------------------------------------------------------\n\"");
#else
		CP("sc \"^7---------------------------------------------------------------------------------------------------------\n\"");
#endif
		trap_SendServerCommand(ent - g_entities, va("sc \"%-9s %-14s ^5%-15s^1%4d^$%4d^5%4d%4d%4d%4d%4d%4d%4d^5%4d^2%6d^1%6d^6%5d^$%5d^3%7d\n\"",
		                                            "",
		                                            aTeams[i],
		                                            "Totals",
		                                            tot_timex / 60000,
		                                            tot_timel / 60000,
		                                            time_eff,
		                                            tot_kills,
		                                            tot_deaths,
		                                            tot_gibs,
		                                            tot_sk,
		                                            tot_tk,
		                                            tot_tg,
		                                            eff,
		                                            tot_dg,
		                                            tot_dr,
		                                            tot_tdg,
		                                            tot_tdr,
		                                            tot_xp
		                                            ));
	}

#ifdef FEATURE_RATING
	// display map bias
	if (g_skillRating.integer && g_gametype.integer != GT_WOLF_STOPWATCH && g_gametype.integer != GT_WOLF_LMS)
	{
		if (g_skillRating.integer > 1)
		{
			CP(va("sc \"\n^2Map bias: ^1%+.1f^7/^$%+.1f^7 pct\n^2Win prob: ^1%+.1f^7/^$%+.1f^7 pct\n\" 0",
			      100.f * (level.mapProb - 0.5f), 100.f * (0.5f - level.mapProb), 100.f * level.axisProb, 100.f * level.alliesProb));
		}
		else
		{
			CP(va("sc \"\n^2Win prob: ^1%+.1f^7/^$%+.1f^7 pct\n\" 0", 100.f * level.axisProb, 100.f * level.alliesProb));
		}
	}
#endif

	CP(va("sc \"%s\n\n\" 0", ((!cnt) ? "^3\nNo scores to report." : "")));
}

/**
 * @brief Dumps end-of-match info
 * @param dwDumpType
 */
void G_matchInfoDump(unsigned int dwDumpType)
{
	int       i, ref;
	gentity_t *ent;
	gclient_t *cl;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		ref = level.sortedClients[i];
		ent = &g_entities[ref];
		cl  = ent->client;

		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}

		if (dwDumpType == EOM_WEAPONSTATS)
		{
			// If client wants to write stats to a file, don't auto send this stuff
			if (!(cl->pers.clientFlags & CGF_STATSDUMP))
			{
				if ((cl->pers.autoaction & AA_STATSALL)
#ifdef FEATURE_MULTIVIEW
					|| cl->pers.mvCount > 0
#endif
					)
				{
					G_statsall_cmd(ent, 0, qfalse);
				}
				else if (cl->sess.sessionTeam != TEAM_SPECTATOR)
				{
					if (cl->pers.autoaction & AA_STATSTEAM)
					{
						G_statsall_cmd(ent, cl->sess.sessionTeam, qfalse);                // Currently broken.. need to support the overloading of dwCommandID
					}
					else
					{
						CP(va("ws %s\n", G_createStats(ent)));
					}
				}
				else if (cl->sess.spectatorState != SPECTATOR_FREE)
				{
					int pid = cl->sess.spectatorClient;

					if ((cl->pers.autoaction & AA_STATSTEAM))
					{
						G_statsall_cmd(ent, level.clients[pid].sess.sessionTeam, qfalse); // Currently broken.. need to support the overloading of dwCommandID
					}
					else
					{
						CP(va("ws %s\n", G_createStats(g_entities + pid)));
					}
				}
			}

			// Log it
			if (cl->sess.sessionTeam != TEAM_SPECTATOR)
			{
				G_LogPrintf("WeaponStats: %s\n", G_createStats(ent));
			}

		}
		else if (dwDumpType == EOM_MATCHINFO)
		{
			if (!(cl->pers.clientFlags & CGF_STATSDUMP))
			{
				G_printMatchInfo(ent);
			}
			if (g_gametype.integer == GT_WOLF_STOPWATCH)
			{
				if (g_currentRound.integer == 1)       // We've already missed the switch
				{
					CP(va("print \">>> ^3Clock set to: %d:%02d\n\n\n\"",
						  g_nextTimeLimit.integer,
						  (int)(60.0f * (g_nextTimeLimit.value - g_nextTimeLimit.integer))));
				}
				else
				{
					float val = (float)((level.timeCurrent - (level.startTime + level.time - level.intermissiontime)) / 60000.0);

					if (val < g_timelimit.value)
					{
						CP(va("print \">>> ^3Objective reached at %d:%02d (original: %d:%02d)\n\n\n\"",
							  (int)val,
							  (int)(60.0f * (val - (int)val)),
							  g_timelimit.integer,
							  (int)(60.0f * (g_timelimit.value - g_timelimit.integer))));
					}
					else
					{
						CP(va("print \">>> ^3Objective NOT reached in time (%d:%02d)\n\n\n\"",
							  g_timelimit.integer,
							  (int)(60.0f * (g_timelimit.value - g_timelimit.integer))));
					}
				}
			}
		}
	}
}

/**
 * @brief Update configstring for vote info
 * @param[in] cv
 * @return
 */
int G_checkServerToggle(vmCvar_t *cv)
{
	int nFlag;

	if (cv == &match_mutespecs)
	{
		nFlag = CV_SVS_MUTESPECS;
	}
	else if (cv == &g_friendlyFire)
	{
		nFlag = CV_SVS_FRIENDLYFIRE;
	}
	else if (cv == &g_antilag)
	{
		nFlag = CV_SVS_ANTILAG;
	}
	else if (cv == &g_balancedteams)
	{
		nFlag = CV_SVS_BALANCEDTEAMS;
	}
	// special case for 2 bits
	else if (cv == &match_warmupDamage)
	{
		if (cv->integer > 0)
		{
			level.server_settings &= ~CV_SVS_WARMUPDMG;
			nFlag                  = (cv->integer > 2) ? 2 : cv->integer;
			nFlag                  = nFlag << 2;
		}
		else
		{
			nFlag = CV_SVS_WARMUPDMG;
		}
	}
	else if (cv == &g_nextmap && g_gametype.integer != GT_WOLF_CAMPAIGN)
	{
		if (*cv->string)
		{
			level.server_settings |= CV_SVS_NEXTMAP;
		}
		else
		{
			level.server_settings &= ~CV_SVS_NEXTMAP;
		}
		return qtrue;
	}
	else if (cv == &g_nextcampaign && g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		if (*cv->string)
		{
			level.server_settings |= CV_SVS_NEXTMAP;
		}
		else
		{
			level.server_settings &= ~CV_SVS_NEXTMAP;
		}
		return qtrue;
	}
	else
	{
		return qfalse;
	}

	if (cv->integer > 0)
	{
		level.server_settings |= nFlag;
	}
	else
	{
		level.server_settings &= ~nFlag;
	}

	return qtrue;
}

/**
 * @brief Sends a player's stats to the requesting client.
 * @param[in] ent
 * @param[in] nType
 */
void G_statsPrint(gentity_t *ent, int nType)
{
	const char *cmd;

	if (!ent || (ent->r.svFlags & SVF_BOT))
	{
		return;
	}

	cmd = (nType == 0) ? "ws" : ((nType == 1) ? "wws" : "gstats");         // Yes, not the cleanest

	// If requesting stats for self, its easy
	if (trap_Argc() < 2)
	{
		// Always send to everybody at end of match
		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR || level.intermissiontime)
		{
			CP(va("%s %s\n", cmd, G_createStats(ent)));
			// Specs default to players they are chasing
		}
		else if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			CP(va("%s %s\n", cmd, G_createStats(g_entities + ent->client->sess.spectatorClient)));
		}
		else
		{
			CP(va("%s %s\n", cmd, G_createStats(ent)));
			CP("print \"\nType ^3\\weaponstats <player_id>^7 to see stats on an active player.\n\"");
		}
	}
	else
	{
		int  pid;
                char arg[MAX_TOKEN_CHARS];

		// Find the player to poll stats.
		trap_Argv(1, arg, sizeof(arg));
		if ((pid = ClientNumberFromString(ent, arg)) == -1)
		{
			return;
		}

		CP(va("%s %s\n", cmd, G_createStats(g_entities + pid)));
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
