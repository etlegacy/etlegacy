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
 * @file tvg_cmds.c
 */

#include "tvg_local.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

const char *aTeams[TEAM_NUM_TEAMS] = { "FFA", "^1Axis^7", "^$Allies^7", "^2Spectators^7" };
team_info  teamInfo[TEAM_NUM_TEAMS];

/**
 * @brief G_MatchOnePlayer
 * @param[in] plist
 * @param[out] err
 * @param[in] len
 * @return
 */
static qboolean G_MatchOnePlayer(int *plist, char *err, size_t len)
{
	err[0] = '\0';

	if (plist[0] == -1)
	{
		Q_strcat(err, len, "no connected player by that name or slot #");
		return qfalse;
	}

	if (plist[1] != -1)
	{
		char      line[MAX_NAME_LENGTH + 10];
		gclient_t *cl;
		int       *p;

		line[0] = '\0';

		Q_strcat(err, len, "more than one player name matches be more specific or use the slot #:\n");
		for (p = plist; *p != -1; p++)
		{
			cl = &level.clients[*p];
			if (cl->pers.connected == CON_CONNECTED)
			{
				Com_sprintf(line, MAX_NAME_LENGTH + 10, "%2i - %s^7\n", *p, cl->pers.netname);
				if (strlen(err) + strlen(line) > len)
				{
					break;
				}
				Q_strcat(err, len, line);
			}
		}
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Sets plist to an array of integers that represent client numbers that have
 * names that are a partial match for s. List is terminated by a -1.
 *
 * @param[in] s
 * @param[out] plist
 * @return Number of matching clientids.
 */
int TVG_ClientNumbersFromString(char *s, int *plist)
{
	gclient_t *p;
	int       i, found = 0;
	char      s2[MAX_STRING_CHARS];
	char      n2[MAX_STRING_CHARS];
	char      *m;

	*plist = -1;

	// if a number is provided, it might be a slot #
	if (Q_isanumber(s))
	{
		i = Q_atoi(s);

		if (i >= 0 && i < level.maxclients)
		{
			p = &level.clients[i];
			if (p->pers.connected == CON_CONNECTED || p->pers.connected == CON_CONNECTING)
			{
				*plist++ = i;
				*plist   = -1;
				return 1;
			}
		}
	}

	// now look for name matches
	Q_strncpyz(s2, s, sizeof(s2));
	Q_CleanStr(s2);
	Q_strlwr(s2);

	if (strlen(s2) < 1)
	{
		return 0;
	}

	for (i = 0; i < level.maxclients; ++i)
	{
		p = &level.clients[i];
		if (p->pers.connected != CON_CONNECTED && p->pers.connected != CON_CONNECTING)
		{
			continue;
		}

		Q_strncpyz(n2, p->pers.netname, sizeof(n2));
		Q_CleanStr(n2);
		Q_strlwr(n2);

		m = strstr(n2, s2);

		if (m != NULL)
		{
			*plist++ = i;
			found++;
		}
	}
	*plist = -1;
	return found;
}

/**
 * @brief Find player slot by matching the slot number or complete/partial player name but unique
 * @param[in] to
 * @param[in] s
 * @return A player number for either a number or name string, -1 if invalid / not found
 */
int TVG_ClientNumberFromString(gclient_t *to, char *s)
{
	int pids[MAX_CLIENTS];

	// no match or more than 1 player matchs, out error
	if (TVG_ClientNumbersFromString(s, pids) != 1)
	{
		char err[MAX_STRING_CHARS];

		G_MatchOnePlayer(pids, err, sizeof(err));

		if (to)
		{
			CPx(to - level.clients, va("print \"[lon]Bad client slot: [lof]%s\n\"", err));
		}
		else
		{
			G_Printf("Bad client slot: %s", err);
		}

		return -1;
	}

	return pids[0];
}

/*
 * @brief G_TeamDamageStats
 * @param[in] ent
 *
 * @note Unused
void G_TeamDamageStats(gentity_t *ent)
{
    if (!ent->client) return;

    {
        float teamHitPct =
            ent->client->sess.hits ?
            (ent->client->sess.team_hits / ent->client->sess.hits)*(100):
            0;

        CPx(ent-g_entities,
            va("print \"Team Hits: %.2f Total Hits: %.2f "
                "Pct: %.2f Limit: %d\n\"",
            ent->client->sess.team_hits,
            ent->client->sess.hits,
            teamHitPct,
            g_teamDamageRestriction.integer
            ));
    }
    return;
}
*/

/**
 * @brief G_PlaySound_Cmd
 */
void G_PlaySound_Cmd(void)
{
	char sound[MAX_QPATH], name[MAX_NAME_LENGTH], cmd[32] = { "playsound" };

	if (trap_Argc() < 2)
	{
		G_Printf("usage: playsound [name|slot#] sound\n");
		return;
	}

	if (trap_Argc() > 2)
	{
		trap_Argv(0, cmd, sizeof(cmd));
		trap_Argv(1, name, sizeof(name));
		trap_Argv(2, sound, sizeof(sound));
	}
	else
	{
		trap_Argv(1, sound, sizeof(sound));
		name[0] = '\0';
	}

	if (name[0])
	{
		int       cnum;
		gentity_t *victim;

		cnum = TVG_ClientNumberFromString(NULL, name);

		if (cnum == -1)
		{
			return;
		}

		victim = &level.gentities[cnum];

		if (!Q_stricmp(cmd, "playsound_env"))
		{
			G_AddEvent(victim, EV_GENERAL_SOUND, G_SoundIndex(sound));
		}
		else
		{
			G_ClientSound(victim, G_SoundIndex(sound));
		}
	}
	else
	{
		G_globalSound(sound);
	}
}

/**
 * @brief Add score with clientNum at index i of level.sortedClients[] to the string buf.
 *
 * @param ent - unused
 * @param[in] i
 * @param[out] buf
 * @param[in] bufsize
 *
 * @return qtrue if the score was appended to buf, qfalse otherwise.
 *
 * @todo FIXME: FEATURE_MULTIVIEW might be buggy -> powerups var is used to store player class/type (differs from GPL Code)
 *       playerClass is no longer sent!
 */
qboolean G_SendScore_Add(gentity_t *ent, int i, char *buf, int bufsize)
{
	gclient_t *cl = &level.clients[level.sortedClients[i]];
	int       ping, respawnsLeft = cl->ps.persistant[PERS_RESPAWNS_LEFT]; // number of respawns left
	char      entry[128];
	int       totalXP   = 0;
	int       miscFlags = 0; // 1 - ready 2 - is bot

	entry[0] = '\0';

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		if (g_entities[level.sortedClients[i]].health <= 0)
		{
			respawnsLeft = -2;
		}
	}
	else if (respawnsLeft == 0 && ((cl->ps.pm_flags & PMF_LIMBO) || (level.intermissiontime && g_entities[level.sortedClients[i]].health <= 0)))
	{
		respawnsLeft = -2;
	}

	if (cl->pers.connected == CON_CONNECTING)
	{
		ping = -1;
	}
	else
	{
		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
	}

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		totalXP = cl->ps.persistant[PERS_SCORE];
	}
	else
	{
		int j;

		if ((g_gametype.integer == GT_WOLF_CAMPAIGN && g_xpSaver.integer) ||
		    (g_gametype.integer == GT_WOLF_CAMPAIGN && (g_campaigns[level.currentCampaign].current != 0 && !level.newCampaign)) ||
		    (g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer != 0))
		{
			for (j = SK_BATTLE_SENSE; j < SK_NUM_SKILLS; j++)
			{
				totalXP += cl->sess.skillpoints[j];
			}
		}
		else
		{
			for (j = SK_BATTLE_SENSE; j < SK_NUM_SKILLS; j++)
			{
				// current map XPs only
				totalXP += cl->sess.skillpoints[j] - cl->sess.startskillpoints[j];
			}
		}
	}

	if (cl->ps.eFlags & EF_READY)
	{
		miscFlags |= 1;
	}

	if (g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT)
	{
		miscFlags |= 2;
	}

	Com_sprintf(entry,
	            sizeof(entry),
	            " %i %i %i %i %i %i %i",
	            level.sortedClients[i],
	            totalXP,
	            ping,
	            (level.intermissiontime - cl->pers.enterTime) / 60000,
	            g_entities[level.sortedClients[i]].s.powerups,
	            miscFlags,
	            respawnsLeft
	            );

	if ((strlen(buf) + strlen(entry) + 1) > bufsize)
	{
		return qfalse;
	}
	Q_strcat(buf, bufsize, entry);

	return qtrue;
}

/**
 * @brief Sends current scoreboard information
 * @param[out] ent
 * @param dwCommand - unused
 * @param value - unused
 */
void TVG_Cmd_Score_f(gclient_t *client, unsigned int dwCommand, int value)
{
	trap_SendServerCommand(client - level.clients, level.cmds.score[0]);
	if (level.cmds.scoreHasTwoParts)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.score[1]);
	}
}

/**
 * @brief CheatsOk
 * @param[in] ent
 * @return
 */
qboolean CheatsOk(gentity_t *ent)
{
	if (!g_cheats.integer)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	if (ent->health <= 0)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"You must be alive to use this command.\n\""));
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief Concatenates console arguments
 * @param[in] start concatenate from this argument
 */
char *ConcatArgs(int start)
{
	int          i, c;
	unsigned int tlen, len = 0;
	static char  line[MAX_STRING_CHARS];
	char         arg[MAX_STRING_CHARS];

	c = trap_Argc();
	for (i = start ; i < c ; i++)
	{
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1)
		{
			break;
		}
		Com_Memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1)
		{
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/**
 * @brief GetSkillPointUntilLevelUp
 * @param[in] ent
 * @param[in] skill
 * @return
 */
float GetSkillPointUntilLevelUp(gentity_t *ent, skillType_t skill)
{
	if (ent->client->sess.skill[skill] < NUM_SKILL_LEVELS - 1)
	{
		int i = ent->client->sess.skill[skill] + 1;
		int x = 1;

		for (; i < NUM_SKILL_LEVELS; i++, x++)
		{
			if (GetSkillTableData(skill)->skillLevels[ent->client->sess.skill[skill] + x] >= 0)
			{
				return GetSkillTableData(skill)->skillLevels[ent->client->sess.skill[skill] + x] - ent->client->sess.skillpoints[skill];
			}
		}
	}
	return -1;
}

/**
 * @brief Cmd_Noclip_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 *
 * @note argv(0) noclip
 */
void Cmd_Noclip_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char *msg;
	char *name;

	name = ConcatArgs(1);

	if (!CheatsOk(ent))
	{
		return;
	}

	if (!Q_stricmp(name, "on") || Q_atoi(name))
	{
		ent->client->noclip = qtrue;
	}
	else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
	{
		ent->client->noclip = qfalse;
	}
	else
	{
		ent->client->noclip = !ent->client->noclip;
	}

	if (ent->client->noclip)
	{
		msg = "noclip ON\n";
	}
	else
	{
		msg = "noclip OFF\n";
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/**
 * @brief If the client being followed leaves the game, or you just want to drop
 * to free floating spectator mode.
 *
 * @param[in,out] client
 */
void StopFollowing(gclient_t *client)
{
	// divert behaviour if TEAM_SPECTATOR, moved the code from SpectatorThink to put back into free fly correctly
	// (I am not sure this can be called in non-TEAM_SPECTATOR situation, better be safe)
	//if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		// drop to free floating, somewhere above the current position (that's the client you were following)
		vec3_t    pos, angle;

		// FIXME: to avoid having a spectator with a gun..
		//if (client->wbuttons & WBUTTON_ATTACK2 || client->buttons & BUTTON_ATTACK)
		//{
		//	return;
		//}

		client->sess.spectatorState  = SPECTATOR_FREE;
		client->sess.spectatorClient = 0;

		VectorCopy(client->ps.origin, pos);
		//pos[2] += 16; // removing for now
		VectorCopy(client->ps.viewangles, angle);

		ClientBegin(client - level.clients);

		VectorCopy(pos, client->ps.origin);
		TVG_SetClientViewAngle(client, angle);
	}
	//else
	//{
	//	// legacy code, FIXME: useless?
	//	// no this is for limbo i'd guess
	//	client->sess.spectatorState = SPECTATOR_FREE;
	//	client->ps.clientNum        = ent - g_entities;
	//}
}

/*
 * @brief G_NumPlayersWithWeapon
 * @param[in] weap
 * @param[in] team
 * @return
 *
 * @note Unused
int G_NumPlayersWithWeapon(weapon_t weap, team_t team)
{
    int i, j, cnt = 0;

    for (i = 0; i < level.numConnectedClients; i++)
    {
        j = level.sortedClients[i];

        if (level.clients[j].sess.playerType != PC_SOLDIER)
        {
            continue;
        }

        if (level.clients[j].sess.sessionTeam != team)
        {
            continue;
        }

        if (level.clients[j].sess.latchPlayerWeapon != weap && level.clients[j].sess.playerWeapon != weap)
        {
            continue;
        }

        cnt++;
    }

    return cnt;
}
*/

/**
 * @brief G_NumPlayersOnTeam
 * @param[in] team
 * @return
 */
int G_NumPlayersOnTeam(team_t team)
{
	int i, j, cnt = 0;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (level.clients[j].sess.sessionTeam != team)
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

/**
 * @brief G_TeamCount
 * @param[in] ent
 * @param[in] weap weapon or -1
 * @return
 */
int G_TeamCount(gentity_t *ent, int weap)
{
	int i, j, cnt;

	if (weap == -1)     // we aint checking for a weapon, so always include ourselves
	{
		cnt = 1;
	}
	else     // we ARE checking for a weapon, so ignore ourselves
	{
		cnt = 0;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (j == ent - g_entities)
		{
			continue;
		}

		if (level.clients[j].sess.sessionTeam != ent->client->sess.sessionTeam)
		{
			continue;
		}

		if (weap != -1)
		{
			if (level.clients[j].sess.playerWeapon != weap && level.clients[j].sess.latchPlayerWeapon != weap)
			{
				continue;
			}
		}

		cnt++;
	}

	return cnt;
}

/**
 * @brief G_ClassCount
 * @param[in] ent
 * @param[in] playerType
 * @param[in] team
 * @return
 */
int G_ClassCount(gentity_t *ent, int playerType, team_t team)
{
	int i, j, cnt = 0;

	if (playerType < PC_SOLDIER || playerType > PC_COVERTOPS)
	{
		return 0;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (ent && j == ent - g_entities)
		{
			continue;
		}

		if (level.clients[j].sess.sessionTeam != team)
		{
			continue;
		}

		if (level.clients[j].sess.playerType != playerType &&
		    level.clients[j].sess.latchPlayerType != playerType)
		{
			continue;
		}
		cnt++;
	}
	return cnt;
}

/**
 * @brief "Topshots" accuracy rankings
 * @param ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_WeaponStatsLeaders_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_weaponStatsLeaders_cmd(ent, qtrue, qtrue);
	return;
}

/**
 * @brief Cmd_wStats_f
 * @param ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_wStats_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_statsPrint(ent, 1);
	return;
}

/**
 * @brief Player game stats
 * @param ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_sgStats_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	G_statsPrint(ent, 2);
	return;
}

/**
 * @brief Cmd_ResetSetup_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_ResetSetup_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	qboolean changed = qfalse;

	if (!ent || !ent->client)
	{
		return;
	}

	ent->client->sess.latchPlayerType = ent->client->sess.playerType;

	if (ent->client->sess.latchPlayerWeapon != ent->client->sess.playerWeapon)
	{
		ent->client->sess.latchPlayerWeapon = ent->client->sess.playerWeapon;
		changed                             = qtrue;
	}

	if (ent->client->sess.latchPlayerWeapon2 != ent->client->sess.playerWeapon2)
	{
		ent->client->sess.latchPlayerWeapon2 = ent->client->sess.playerWeapon2;
		changed                              = qtrue;
	}

	if (changed)
	{
		ClientUserinfoChanged(ent - g_entities);
	}
}

/**
 * @brief Cmd_Follow_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value - unused
 */
void Cmd_Follow_f(gclient_t *client, unsigned int dwCommand, int value)
{
	int  cnum;
	char arg[MAX_TOKEN_CHARS];

	//if (trap_Argc() != 2)
	//{
	//	if (client->sess.spectatorState == SPECTATOR_FOLLOW)
	//	{
	//		StopFollowing(client);
	//	}
	//	return;
	//}

	//trap_Argv(1, arg, sizeof(arg));

	//if (!Q_stricmp(arg, "allies") || !Q_stricmp(arg, "axis"))
	//{
	//	team_t team;
	//	team = (!Q_stricmp(arg, "allies") ? TEAM_ALLIES : TEAM_AXIS);

	//	if ((client->sess.sessionTeam == TEAM_AXIS ||
	//	     client->sess.sessionTeam == TEAM_ALLIES) &&
	//	    client->sess.sessionTeam != team)
	//	{
	//		CP("print \"Can't follow a player on an enemy team!\n\"");
	//		return;
	//	}

	//	if (!TeamCount(client - level.clients, team))
	//	{
	//		CP(va("print \"The %s team %s empty!  Follow command ignored.\n\"", aTeams[team],
	//		      ((client->sess.sessionTeam != team) ? "is" : "would be")));
	//		return;
	//	}

	//	// Allow for simple toggle
	//	if (client->sess.spec_team != team)
	//	{
	//		if (teamInfo[team].spec_lock && !(client->sess.spec_invite & team))
	//		{
	//			CP(va("print \"Sorry, the %s team is locked from spectators.\n\"", aTeams[team]));
	//		}
	//		else
	//		{
	//			client->sess.spec_team = team;
	//			CP(va("print \"Spectator follow is now locked on the %s team.\n\"", aTeams[team]));
	//			Cmd_FollowCycle_f(client, 1, qfalse);
	//		}
	//	}
	//	else
	//	{
	//		client->sess.spec_team = 0;
	//		CP(va("print \"%s team spectating is now disabled.\n\"", aTeams[team]));
	//	}

	//	return;
	//}

	//cnum = TVG_ClientNumberFromString(client, arg);

	//if (cnum == -1)
	//{
	//	return;
	//}

	//// Can't follow enemy players if not a spectator
	//if ((client->sess.sessionTeam == TEAM_AXIS ||
	//     client->sess.sessionTeam == TEAM_ALLIES) &&
	//    client->sess.sessionTeam != level.clients[cnum].sess.sessionTeam)
	//{
	//	CP("print \"Can't follow a player on an enemy team!\n\"");
	//	return;
	//}

	//// can't follow self
	//if (&level.clients[cnum] == client)
	//{
	//	return;
	//}

	//// can't follow another spectator, but shoutcasters can follow other shoutcasters
	//if (level.clients[cnum].sess.sessionTeam == TEAM_SPECTATOR && (!level.clients[cnum].sess.shoutcaster || !client->sess.shoutcaster))
	//{
	//	return;
	//}

	//if (level.clients[cnum].ps.pm_flags & PMF_LIMBO)
	//{
	//	return;
	//}

	//// can't follow a player on a speclocked team, unless allowed
	//if (!G_allowFollow(client, level.clients[cnum].sess.sessionTeam))
	//{
	//	CP(va("print \"Sorry, the %s team is locked from spectators.\n\"", aTeams[level.clients[cnum].sess.sessionTeam]));
	//	return;
	//}

	//client->sess.spectatorState  = SPECTATOR_FOLLOW;
	//client->sess.spectatorClient = cnum;
}

/**
 * @brief Cmd_FollowCycle_f
 * @param[in,out] ent
 * @param[in] dir
 * @param[in] skipBots
 */
void Cmd_FollowCycle_f(gclient_t *client, int dir, qboolean skipBots)
{
	int clientnum;

	if (dir != 1 && dir != -1)
	{
		G_Error("Cmd_FollowCycle_f: bad dir %i\n", dir);
	}

	clientnum = client->sess.spectatorClient;
	do
	{
		clientnum += dir;
		if (clientnum >= MAX_CLIENTS)
		{
			clientnum = 0;
		}
		if (clientnum < 0)
		{
			clientnum = MAX_CLIENTS - 1;
		}

		// can only follow valid clients
		if (!level.ettvMasterClients[clientnum].valid)
		{
			continue;
		}

		// can't follow another spectator
		//if (level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR)
		//{
		//	continue;
		//}

		if (level.ettvMasterClients[clientnum].ps.pm_flags & PMF_LIMBO)
		{
			continue;
		}

		/*if (!G_desiredFollow(ent, level.clients[clientnum].sess.sessionTeam))
		{
			continue;
		}*/

		// cycle through humans only?..
		if (skipBots && (g_entities[clientnum].r.svFlags & SVF_BOT))
		{
			continue;
		}

		// this is good, we can use it
		client->sess.spectatorClient = clientnum;
		client->sess.spectatorState  = SPECTATOR_FOLLOW;
		return;
	}
	while (clientnum != client->sess.spectatorClient);

	// leave it where it was
}

/**
 * @brief Cmd_FollowNext_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_FollowNext_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	Cmd_FollowCycle_f(ent, 1, qfalse);
}

/**
 * @brief Cmd_FollowPrevious_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_FollowPrevious_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	Cmd_FollowCycle_f(ent, -1, qfalse);
}

/**
 * @brief Try to follow the same client as last time (before getting killed)
 * @param[in] ent
 */
qboolean G_FollowSame(gentity_t *ent)
{
	//if (ent->client->sess.spectatorClient < 0 || ent->client->sess.spectatorClient >= level.maxclients)
	//{
	//	return qfalse;
	//}

	//// can only follow connected clients
	//if (level.clients[ent->client->sess.spectatorClient].pers.connected != CON_CONNECTED)
	//{
	//	return qfalse;
	//}

	//// can't follow another spectator
	//if (level.clients[ent->client->sess.spectatorClient].sess.sessionTeam == TEAM_SPECTATOR)
	//{
	//	return qfalse;
	//}

	//// couple extra checks for limbo mode
	//if (ent->client->ps.pm_flags & PMF_LIMBO)
	//{
	//	if (level.clients[ent->client->sess.spectatorClient].sess.sessionTeam != ent->client->sess.sessionTeam)
	//	{
	//		return qfalse;
	//	}
	//}

	//if (level.clients[ent->client->sess.spectatorClient].ps.pm_flags & PMF_LIMBO)
	//{
	//	return qfalse;
	//}

	//if (!G_desiredFollow(ent, level.clients[ent->client->sess.spectatorClient].sess.sessionTeam))
	//{
	//	return qfalse;
	//}

	//return qtrue;
}

/**
 * @brief Plays a sound (wav file or sound script) on this entity
 * @param[in] ent entity to play the sound on
 * @param[in] soundId sound file name or sound script ID
 * @param[in] volume sound volume, only applies to sound file name call
 *
 * @note Unused. Keep this.
 *
 * @note Calling G_AddEvent(..., EV_GENERAL_SOUND, ...) has the danger of
 * the event never getting through to the client because the entity might not
 * be visible (unless it has the SVF_BROADCAST flag), so if you want to make sure
 * the sound is heard, call this function instead.
 */
void G_EntitySound(gentity_t *ent, const char *soundId, int volume)
{
	//   for sound script, volume is currently always 127.
	trap_SendServerCommand(-1, va("entitySound %d %s %d %i %i %i normal", ent->s.number, soundId, volume,
	                              (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2]));
}

/**
 * @brief Similar to G_EntitySound, but do not cut this sound off
 * @param[in] ent entity to play the sound on
 * @param[in] soundId sound file name or sound script ID
 * @param[in] volume sound volume, only applies to sound file name call
 *
 * @note Unused. See G_EntitySound()
 */
void G_EntitySoundNoCut(gentity_t *ent, const char *soundId, int volume)
{
	//   for sound script, volume is currently always 127.
	trap_SendServerCommand(-1, va("entitySound %d %s %d %i %i %i noCut", ent->s.number, soundId, volume,
	                              (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2]));
}

/**
 * @brief G_HQSay
 * @param[in] other
 * @param[in] color
 * @param[in] name
 * @param[in] message
 */
void G_HQSay(gentity_t *other, int color, const char *name, const char *message)
{
	if (!other || !other->inuse || !other->client)
	{
		return;
	}

	trap_SendServerCommand(other - g_entities, va("gamechat \"%s%c%c%s\" 1", name, Q_COLOR_ESCAPE, color, message));
}

/**
 * @brief TVG_SayTo
 * @param[in] ent
 * @param[in] other
 * @param[in] mode
 * @param[in] color
 * @param[in] name
 * @param[in] message
 * @param[in] localize
 */
void TVG_SayTo(gclient_t *client, gclient_t *other, int mode, int color, const char *name, const char *message, qboolean localize)
{
	char cmd[6];

	if (!other)
	{
		return;
	}
	//if ((mode == SAY_TEAM || mode == SAY_TEAMNL) && !OnSameTeam(ent, other))
	//{
	//	return;
	//}

	// if spectator, no chatting to players
	//if (match_mutespecs.integer > 0 && client->sess.referee == 0 &&
	//    ((client->sess.sessionTeam == TEAM_FREE && other->sess.sessionTeam != TEAM_FREE) ||
	//     (client->sess.sessionTeam == TEAM_SPECTATOR && other->sess.sessionTeam != TEAM_SPECTATOR)))
	//{
	//	return;
	//}
	//else
	//{
	//	if (mode == SAY_BUDDY)      // send only to people who have the sender on their buddy list
	//	{
	//		if (ent->s.clientNum != other->s.clientNum)
	//		{
	//			fireteamData_t *ft1, *ft2;
	//			if (!G_IsOnFireteam(other - g_entities, &ft1))
	//			{
	//				return;
	//			}
	//			if (!G_IsOnFireteam(ent - g_entities, &ft2))
	//			{
	//				return;
	//			}
	//			if (ft1 != ft2)
	//			{
	//				return;
	//			}
	//		}
	//	}

	//	if (COM_BitCheck(other->client->sess.ignoreClients, (ent - g_entities)))
	//	{
	//		//Q_strncpyz(cmd, "print", sizeof(cmd));
	//	}
	//	else if (mode == SAY_TEAM || mode == SAY_BUDDY)
	//	{
	//		Q_strncpyz(cmd, "tchat", sizeof(cmd));

	//		trap_SendServerCommand((int)(other - g_entities),
	//		                       va("%s \"%c%c%s%s\" %i %i %i %i %i",
	//		                          cmd,
	//		                          Q_COLOR_ESCAPE, color, message,
	//		                          (!Q_stricmp(cmd, "print")) ? "\n" : "",
	//		                          (int)(ent - g_entities), localize,
	//		                          (int)ent->s.pos.trBase[0],
	//		                          (int)ent->s.pos.trBase[1],
	//		                          (int)ent->s.pos.trBase[2]));
	//	}
	//	else
	//	{
	//		Q_strncpyz(cmd, "chat", sizeof(cmd));

	//		trap_SendServerCommand((int)(other - g_entities),
	//		                       va("%s \"%s%c%c%s%s\" %i %i",
	//		                          cmd, name, Q_COLOR_ESCAPE, color,
	//		                          message,
	//		                          (!Q_stricmp(cmd, "print")) ? "\n" : "",
	//		                          (int)(ent - g_entities), localize));
	//	}
	//}
}

/**
 * @brief TVG_Say
 * @param[in] client
 * @param[in] target
 * @param[in] mode
 * @param[in] chatText
 */
void TVG_Say(gclient_t *client, gclient_t *target, int mode, const char *chatText)
{
	int       j;
	gclient_t *other;
	int       color;
	char      name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];

	switch (mode)
	{
	default:
	case SAY_ALL:
		G_LogPrintf("say: ^7%s^7: ^2%s\n", client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%c%c%s%c%c: %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_GREEN);
		color = COLOR_GREEN;
		break;
	case SAY_BUDDY:
		G_LogPrintf("saybuddy: ^7%s^7: ^3%s\n", client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "[lof]%c%c(%s%c%c): %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_YELLOW);
		color = COLOR_YELLOW;
		break;
	case SAY_TEAM:
		G_LogPrintf("sayteam: ^7%s^7: ^5%s\n", client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "[lof]%c%c(%s%c%c): %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_CYAN);
		color = COLOR_CYAN;
		break;
	case SAY_TEAMNL:
		G_LogPrintf("sayteamnl: ^7%s^7: ^2%s\n", client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%c%c(%s%c%c): %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_CYAN);
		color = COLOR_CYAN;
		break;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	if (target)
	{
		if (!COM_BitCheck(target->sess.ignoreClients, client - level.clients))
		{
			TVG_SayTo(client, target, mode, color, name, text, qfalse);
		}
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer)
	{
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.numConnectedClients; j++)
	{
		other = &level.clients[level.sortedClients[j]];
		if (!COM_BitCheck(other->sess.ignoreClients, client - level.clients))
		{
			TVG_SayTo(client, other, mode, color, name, text, qfalse);
		}
	}
}

/**
 * @brief TVG_Say_f
 * @param[in] client
 * @param[in] mode
 * @param[in] arg0
 */
void TVG_Say_f(gclient_t *client, int mode /*, qboolean arg0*/)
{
	if (client->sess.muted)
	{
		trap_SendServerCommand(client - level.clients, "print \"Can't chat - you are muted\n\"");
		return;
	}

	if (trap_Argc() < 2 /*&& !arg0*/)
	{
		return;
	}

	TVG_Say(client, NULL, mode, ConcatArgs((/*(arg0) ? 0 :*/ 1)));
}

/**
 * @brief G_VoiceTo
 * @param[in] ent
 * @param[in] other
 * @param[in] mode
 * @param[in] id
 * @param[in] voiceonly
 * @param[in] randomNum
 */
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly, float randomNum, int vsayNum, const char *customChat)
{
	int  color;
	char *cmd;

	//if (!other)
	//{
	//	return;
	//}
	//if (!other->inuse)
	//{
	//	return;
	//}
	//if (!other->client)
	//{
	//	return;
	//}

	//if (mode == SAY_TEAM && !OnSameTeam(ent, other))
	//{
	//	return;
	//}

	//// spec vchat rules follow the same as normal chatting rules
	//if (match_mutespecs.integer > 0 && ent->client->sess.referee == 0 &&
	//    ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR)
	//{
	//	return;
	//}

	//if (mode == SAY_TEAM)
	//{
	//	cmd = "vtchat";
	//}
	//else if (mode == SAY_BUDDY)
	//{
	//	cmd = "vbchat";
	//}
	//else
	//{
	//	cmd = "vchat";
	//}

	//if (mode == SAY_TEAM || mode == SAY_BUDDY)
	//{
	//	CPx(other - g_entities, va("%s %d %d %s %i %i %i %f %i %s", cmd, voiceonly, (int)(ent - g_entities), id, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2], (double)randomNum, vsayNum, customChat));
	//}
	//else
	//{
	//	CPx(other - g_entities, va("%s %d %d %s %f %i %s", cmd, voiceonly, (int)(ent - g_entities), id, (double)randomNum, vsayNum, customChat));
	//}
}

/**
 * @brief G_Voice
 * @param[in,out] ent
 * @param[in] target
 * @param[in] mode
 * @param[in] id
 * @param[in] voiceonly
 */
void G_Voice(gentity_t *ent, gentity_t *target, int mode, const char *id, const char *customChat, qboolean voiceonly, int vsayNum)
{
	int       j;
	gentity_t *victim;
	float     randomNum = random();

	// Don't allow excessive spamming of voice chats
	ent->voiceChatSquelch     -= (level.time - ent->voiceChatPreviousTime);
	ent->voiceChatPreviousTime = level.time;

	if (ent->voiceChatSquelch < 0)
	{
		ent->voiceChatSquelch = 0;
	}

	// spam check
	if (ent->voiceChatSquelch >= 30000)
	{
		trap_SendServerCommand(ent - g_entities, "cp \"^1Spam Protection^7: VoiceChat ignored\"");
		return;
	}

	if (g_voiceChatsAllowed.integer)
	{
		ent->voiceChatSquelch += (30000 / g_voiceChatsAllowed.integer);
	}
	else
	{
		return;
	}

	if (target)
	{
		G_VoiceTo(ent, target, mode, id, voiceonly, randomNum, vsayNum, customChat);
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer)
	{
		G_Printf("voice: ^7%s^7 %s\n", ent->client->pers.netname, id);
	}

	if (mode == SAY_BUDDY)
	{
		char     buffer[32];
		int      cls, i, cnt, num;
		qboolean allowclients[MAX_CLIENTS];

		Com_Memset(allowclients, 0, sizeof(allowclients));

		trap_Argv(1, buffer, 32);

		cls = Q_atoi(buffer);

		trap_Argv(2, buffer, 32);
		cnt = Q_atoi(buffer);
		if (cnt > MAX_CLIENTS)
		{
			cnt = MAX_CLIENTS;
		}

		for (i = 0; i < cnt; i++)
		{
			trap_Argv(3 + i, buffer, 32);

			num = Q_atoi(buffer);
			if (num < 0)
			{
				continue;
			}
			if (num >= MAX_CLIENTS)
			{
				continue;
			}

			allowclients[num] = qtrue;
		}

		for (j = 0; j < level.numConnectedClients; j++)
		{
			victim = &g_entities[level.sortedClients[j]];

			if (level.sortedClients[j] != ent->s.clientNum)
			{
				if (cls != -1 && cls != level.clients[level.sortedClients[j]].sess.playerType)
				{
					continue;
				}
			}

			if (cnt)
			{
				if (!allowclients[level.sortedClients[j]])
				{
					continue;
				}
			}

			if (COM_BitCheck(victim->client->sess.ignoreClients, (ent - g_entities)))
			{
				continue;
			}

			G_VoiceTo(ent, victim, mode, id, voiceonly, randomNum, vsayNum, customChat);
		}
	}
	else
	{
		// send it to all the apropriate clients
		for (j = 0; j < level.numConnectedClients; j++)
		{
			victim = &g_entities[level.sortedClients[j]];
			if (COM_BitCheck(victim->client->sess.ignoreClients, (ent - g_entities)))
			{
				continue;
			}
			G_VoiceTo(ent, victim, mode, id, voiceonly, randomNum, vsayNum, customChat);
		}
	}
}

/**
 * @brief G_Voice_f
 * @param[in] ent
 * @param[in] mode
 * @param[in] arg0
 * @param[in] voiceonly
 */
void G_Voice_f(gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly)
{
	char bufferIndexCustom[32];
	int  vsayNum;

	if (ent->client->sess.muted)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Can't chat - you are muted\n\"");
		return;
	}

	if (mode != SAY_BUDDY)
	{
		if (trap_Argc() < 2 && !arg0)
		{
			return;
		}

		trap_Argv((arg0) ? 0 : 1, bufferIndexCustom, sizeof(bufferIndexCustom));

		if (isdigit(bufferIndexCustom[0]))
		{
			vsayNum = Q_atoi(bufferIndexCustom);
			trap_Argv((arg0) ? 1 : 2, bufferIndexCustom, sizeof(bufferIndexCustom));
			G_Voice(ent, NULL, mode, bufferIndexCustom, ConcatArgs(((arg0) ? 2 : 3)), voiceonly, vsayNum);
			return;
		}
		G_Voice(ent, NULL, mode, bufferIndexCustom, ConcatArgs(((arg0) ? 1 : 2)), voiceonly, -1);
	}
	else
	{
		char bufferIndex[32];
		int  index;

		trap_Argv(2, bufferIndex, sizeof(bufferIndex));
		index = Q_atoi(bufferIndex);
		if (index < 0)
		{
			index = 0;
		}

		if (trap_Argc() < 3 + index && !arg0)
		{
			return;
		}

		trap_Argv((arg0) ? 2 + index : 3 + index, bufferIndexCustom, sizeof(bufferIndexCustom));

		if (isdigit(bufferIndexCustom[0]))
		{
			vsayNum = Q_atoi(bufferIndexCustom);
			trap_Argv((arg0) ? 3 + index : 4 + index, bufferIndexCustom, sizeof(bufferIndexCustom));
			G_Voice(ent, NULL, mode, bufferIndexCustom, ConcatArgs(((arg0) ? 4 + index : 5 + index)), voiceonly, vsayNum);
			return;
		}
		G_Voice(ent, NULL, mode, bufferIndexCustom, ConcatArgs(((arg0) ? 3 + index : 4 + index)), voiceonly, -1);
	}
}

/**
 * @brief Cmd_Where_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_Where_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin)));
}

qboolean StringToFilter(const char *s, ipFilter_t *f);

/**
 * @brief G_FindFreeComplainIP
 * @param[in,out] cl
 * @param[in] ip
 * @return
 */
qboolean G_FindFreeComplainIP(gclient_t *cl, ipFilter_t *ip)
{
	int i = 0;

	if (!g_ipcomplaintlimit.integer)
	{
		return qtrue;
	}

	for (i = 0; i < MAX_COMPLAINTIPS && i < g_ipcomplaintlimit.integer; i++)
	{
		if (!cl->pers.complaintips[i].compare && !cl->pers.complaintips[i].mask)
		{
			cl->pers.complaintips[i].compare = ip->compare;
			cl->pers.complaintips[i].mask    = ip->mask;
			return qtrue;
		}
		if ((cl->pers.complaintips[i].compare & cl->pers.complaintips[i].mask) == (ip->compare & ip->mask))
		{
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief TVG_Cmd_SetViewpos_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void TVG_Cmd_SetViewpos_f(gclient_t *client, unsigned int dwCommand, int value)
{
	vec3_t origin, angles;
	char   buffer[MAX_TOKEN_CHARS];
	int    i;

	if (!g_cheats.integer && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_SendServerCommand(client - level.clients, va("print \"Only spectators can use the setviewpos command.\n\""));
		return;
	}

	if (trap_Argc() == 5)
	{
		VectorClear(angles);
		for (i = 0; i < 3; i++)
		{
			trap_Argv(i + 1, buffer, sizeof(buffer));
			origin[i] = Q_atof(buffer);
		}

		trap_Argv(4, buffer, sizeof(buffer));
		angles[YAW] = Q_atof(buffer);
	}
	else if (trap_Argc() == 8)
	{
		for (i = 0; i < 3; i++)
		{
			trap_Argv(i + 1, buffer, sizeof(buffer));
			origin[i] = Q_atof(buffer);
		}

		for (i = 0; i < 3; i++)
		{
			trap_Argv(i + 4, buffer, sizeof(buffer));
			angles[i] = Q_atof(buffer);
		}

		trap_Argv(7, buffer, sizeof(buffer));
		qboolean useViewHeight = Q_atof(buffer);

		if (useViewHeight)
		{
			origin[2] -= (client->ps.viewheight + 1);  // + 1 to account for teleport event origin shift
		}
	}
	else
	{
		trap_SendServerCommand(client - level.clients, va("print \"usage: setviewpos x y z yaw\n       setviewpos x y z pitch yaw roll useViewHeight(1/0)\n\""));
		return;
	}

	TVG_TeleportPlayer(client, origin, angles);
}

extern vec3_t playerMins;
extern vec3_t playerMaxs;

/**
 * @brief Cmd_Activate_f
 * @param[in,out] ent
 */
void Cmd_Activate_f(gentity_t *ent)
{
	
}

/**
 * @brief Cmd_Activate2_f
 * @param[in] ent
 */
void Cmd_Activate2_f(gentity_t *ent)
{

}

/**
 * @brief Cmd_WeaponStat_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_WeaponStat_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char buffer[16];
	extWeaponStats_t stat;

	if (!ent || !ent->client)
	{
		return;
	}

	if (trap_Argc() != 2)
	{
		return;
	}
	trap_Argv(1, buffer, 16);
	stat = (extWeaponStats_t)(atoi(buffer));

	if (stat < WS_KNIFE || stat >= WS_MAX)
	{
		return;
	}

	trap_SendServerCommand(ent - g_entities, va("rws %i %i", ent->client->sess.aWeaponStats[stat].atts, ent->client->sess.aWeaponStats[stat].hits));
}

/**
 * @brief Cmd_IntermissionWeaponStats_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_IntermissionWeaponStats_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char buffer[1024];
	int i, clientNum;

	if (!ent || !ent->client)
	{
		return;
	}

	trap_Argv(1, buffer, sizeof(buffer));

	clientNum = Q_atoi(buffer);
	if (clientNum < 0 || clientNum > g_maxclients.integer)
	{
		return;
	}

	Q_strncpyz(buffer, "imws ", sizeof(buffer));

	// hit regions
	Q_strcat(buffer, sizeof(buffer), va("%i %i %i %i ",
	                                    level.clients[clientNum].pers.playerStats.hitRegions[HR_HEAD],
	                                    level.clients[clientNum].pers.playerStats.hitRegions[HR_ARMS],
	                                    level.clients[clientNum].pers.playerStats.hitRegions[HR_BODY],
	                                    level.clients[clientNum].pers.playerStats.hitRegions[HR_LEGS]));

	for (i = 0; i < WS_MAX; i++)
	{
		Q_strcat(buffer, sizeof(buffer), va("%i %i %i ", level.clients[clientNum].sess.aWeaponStats[i].atts, level.clients[clientNum].sess.aWeaponStats[i].hits, level.clients[clientNum].sess.aWeaponStats[i].kills));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

/**
 * @brief Cmd_IntermissionPlayerKillsDeaths_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_IntermissionPlayerKillsDeaths_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char buffer[MAX_STRING_CHARS];
	int i;

	if (!ent || !ent->client)
	{
		return;
	}

	Q_strncpyz(buffer, "impkd0 ", sizeof(buffer));
	for (i = 0; i < g_maxclients.integer; i++)
	{
		if (i == g_maxclients.integer / 2)
		{
			trap_SendServerCommand(ent - g_entities, buffer);
			Q_strncpyz(buffer, "impkd1 ", sizeof(buffer));
		}

		if (g_entities[i].inuse)
		{
			Q_strcat(buffer, sizeof(buffer), va("%i %i %i %i %i %i ", level.clients[i].sess.kills, level.clients[i].sess.deaths, level.clients[i].sess.gibs, level.clients[i].sess.self_kills, level.clients[i].sess.team_kills, level.clients[i].sess.team_gibs));
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), "0 0 0 0 0 0 ");
		}
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

/**
 * @brief Cmd_IntermissionPlayerTime_f
 * @param[in] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_IntermissionPlayerTime_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char buffer[1024];
	int i;

	if (!ent || !ent->client)
	{
		return;
	}

	Q_strncpyz(buffer, "impt ", sizeof(buffer));
	for (i = 0; i < g_maxclients.integer; i++)
	{
		if (g_entities[i].inuse)
		{
			Q_strcat(buffer, sizeof(buffer), va("%i %i %i ", level.clients[i].sess.time_axis, level.clients[i].sess.time_allies, level.clients[i].sess.time_played));
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), "0 0 0 ");
		}
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}
/**
 * @brief G_CalcClientAccuracies
 */
void G_CalcClientAccuracies(void)
{
	int i, j;
	int shots, hits, headshots;

	for (i = 0; i < g_maxclients.integer; i++)
	{
		shots     = 0;
		hits      = 0;
		headshots = 0;

		if (g_entities[i].inuse)
		{
			for (j = 0; j < WS_MAX; j++)
			{
				// don't take into account weapon that can't do headshot
				if (!aWeaponInfo[j].fHasHeadShots)
				{
					continue;
				}

				shots     += level.clients[i].sess.aWeaponStats[j].atts;
				hits      += level.clients[i].sess.aWeaponStats[j].hits;
				headshots += level.clients[i].sess.aWeaponStats[j].headshots;
			}

			level.clients[i].acc   = shots ? 100 * hits / (float)shots : 0.f;
			level.clients[i].hspct = hits ? 100 * headshots / (float)hits : 0.f;
		}
		else
		{
			level.clients[i].acc   = 0.f;
			level.clients[i].hspct = 0.f;
		}
	}
}

/**
 * @brief Cmd_IntermissionWeaponAccuracies_f
 * @param[in] ent
 */
void Cmd_IntermissionWeaponAccuracies_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char buffer[1024];
	int i;

	if (!ent || !ent->client)
	{
		return;
	}

	G_CalcClientAccuracies();

	Q_strncpyz(buffer, "imwa ", sizeof(buffer));
	for (i = 0; i < g_maxclients.integer; i++)
	{
		if (g_entities[i].inuse)
		{
			Q_strcat(buffer, sizeof(buffer), va("%.1f %.1f ", level.clients[i].acc, level.clients[i].hspct));
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), "0 0 ");
		}
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

/**
 * @brief Cmd_SelectedObjective_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_SelectedObjective_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	int i, val;
	char buffer[16];
	vec_t dist, neardist = 0;
	int nearest = -1;

	if (!ent || !ent->client)
	{
		return;
	}

	if (trap_Argc() != 2)
	{
		return;
	}
	trap_Argv(1, buffer, 16);
	val = Q_atoi(buffer) + 1;

	for (i = 0; i < level.numLimboCams; i++)
	{
		if (!level.limboCams[i].spawn && level.limboCams[i].info == val)
		{
			if (!level.limboCams[i].hasEnt)
			{
				VectorCopy(level.limboCams[i].origin, ent->s.origin2);
				ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
				trap_SendServerCommand(ent - g_entities, va("portalcampos %i %i %i %i %i %i %i %i", val - 1, (int)level.limboCams[i].origin[0], (int)level.limboCams[i].origin[1], (int)level.limboCams[i].origin[2], (int)level.limboCams[i].angles[0], (int)level.limboCams[i].angles[1], (int)level.limboCams[i].angles[2], level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));

				break;
			}
			else
			{
				dist = VectorDistanceSquared(level.limboCams[i].origin, g_entities[level.limboCams[i].targetEnt].r.currentOrigin);
				if (nearest == -1 || dist < neardist)
				{
					nearest  = i;
					neardist = dist;
				}
			}
		}
	}

	if (nearest != -1)
	{
		i = nearest;

		VectorCopy(level.limboCams[i].origin, ent->s.origin2);
		ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
		trap_SendServerCommand(ent - g_entities, va("portalcampos %i %i %i %i %i %i %i %i", val - 1, (int)level.limboCams[i].origin[0], (int)level.limboCams[i].origin[1], (int)level.limboCams[i].origin[2], (int)level.limboCams[i].angles[0], (int)level.limboCams[i].angles[1], (int)level.limboCams[i].angles[2], level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
	}
}

/**
 * @brief Cmd_Ignore_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_Ignore_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char cmd[MAX_TOKEN_CHARS], name[MAX_NAME_LENGTH];
	int cnum;

	trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(1, name, sizeof(cmd));

	if (!*name)
	{
		trap_SendServerCommand(ent - g_entities, "print \"usage: Ignore <clientname>.\n\"");
		return;
	}

	cnum = TVG_ClientNumberFromString(ent, name);

	if (cnum == -1)
	{
		return;
	}

	COM_BitSet(ent->client->sess.ignoreClients, cnum);
	trap_SendServerCommand(ent - g_entities, va("print \"[lon]You are ignoring [lof]%s[lon]^7.\n\"", level.clients[cnum].pers.netname));
}

/**
 * @brief Cmd_UnIgnore_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param value    - unused
 */
void Cmd_UnIgnore_f(gentity_t *ent, unsigned int dwCommand, int value)
{
	char cmd[MAX_TOKEN_CHARS], name[MAX_NAME_LENGTH];
	int cnum;

	trap_Argv(0, cmd, sizeof(cmd));
	trap_Argv(1, name, sizeof(cmd));

	if (!*name)
	{
		trap_SendServerCommand(ent - g_entities, "print \"usage: Unignore <clientname>.\n\"");
		return;
	}

	cnum = TVG_ClientNumberFromString(ent, name);

	if (cnum == -1)
	{
		return;
	}

	COM_BitClear(ent->client->sess.ignoreClients, cnum);
	trap_SendServerCommand(ent - g_entities, va("print \"[lof]%s[lon]^7 is no longer ignored.\n\"", level.clients[cnum].pers.netname));
}

#define ENTNFO_HASH         78985
#define CS_HASH             25581
#define TINFO_HASH          65811
#define SC0_HASH            31373
#define SC1_HASH            31494
#define WEAPONSTATS_HASH    149968
#define SC_HASH             25565
#define CPM_HASH            38410
#define CP_HASH             25221
#define PRINT_HASH          67401
#define CHAT_HASH           50150
#define VCHAT_HASH          64608

/**
* @brief TVG_ClientCommandPassThrough This handles server commands (server responses to client commands)
* @details ETTV Client connected to master changes messages to clientNum = -2
           if we detect that incoming cmd is a broadcast message we should immediately sent it (clientNum = -1).
		   ETTV Server can send ClientCommands to master server too, for example requesting scores, stats etc.
		   the response will come with clientNum -2 again
* @param[in] cmd
*/
static void TVG_ClientCommandPassThrough(char *cmd)
{
	//char parsecmd[MAX_TOKEN_CHARS];
	char passcmd[MAX_TOKEN_CHARS];
	char *token;
	//int passcmdLength = 0;
	//int cmdLength     = 0;
	int i = 0;

	/**parsecmd = cmd;

	while (Q_stricmp(parsecmd, ""))
	{
		cmdLength = strlen(parsecmd);
		if (passcmdLength + cmdLength > sizeof(passcmd) - 2)
		{
			G_Printf("client command passthrough exceeds limit\n");
			return;
		}

		strcpy(passcmdLength + passcmdLength, parsecmd);
		passcmdLength += cmdLength;

		trap_Argv(i, parsecmd, sizeof(parsecmd));
	}*/

	if (!cmd[0])
	{
		return;
	}

	Q_strncpyz(passcmd, cmd, sizeof(passcmd));

	token = strtok(passcmd, " ");

	Com_Printf("pass: %s\n", cmd);

	switch (BG_StringHashValue(token))
	{
	case ENTNFO_HASH:                     // "entnfo"
		trap_SendServerCommand(-1, cmd);
		return;
	case CS_HASH:                         // "cs"
		//trap_SendServerCommand(-1, cmd);
		return;
	case TINFO_HASH:                      // "tinfo"
		trap_SendServerCommand(-1, cmd);
		return;
	case SC0_HASH:                        // "sc0"
		level.cmds.scoreHasTwoParts = qfalse;
		Q_strncpyz(level.cmds.score[0], cmd, sizeof(level.cmds.score[0]));
		return;
	case SC1_HASH:                        // "sc1"
		level.cmds.scoreHasTwoParts = qtrue;
		Q_strncpyz(level.cmds.score[1], cmd, sizeof(level.cmds.score[0]));
		return;
	case WEAPONSTATS_HASH:                // "WeaponStats"
		return;
	case SC_HASH:                         // "sc"
		if (level.cmds.scoresTime != level.time || level.cmds.scoresEndIndex > 99)
		{
			level.cmds.scoresEndIndex = 0;
		}
		Q_strncpyz(level.cmds.scores[level.cmds.scoresEndIndex++], cmd, sizeof(level.cmds.scores[0]));
		level.cmds.scoresTime = level.time;
		return;
	case CPM_HASH:                        // "cpm"
		trap_SendServerCommand(-1, cmd);
		return;
	case CP_HASH:                         // "cp"
		trap_SendServerCommand(-1, cmd);
		return;
	case PRINT_HASH:                      // "print"
		trap_SendServerCommand(-1, cmd);
		return;
	case CHAT_HASH:                       // "chat"
		trap_SendServerCommand(-1, cmd);
		return;
	case VCHAT_HASH:                      // "chat"
		trap_SendServerCommand(-1, cmd);
		return;
	default:
		G_Printf("Unknown client game command: %s [%lu]\n", cmd, BG_StringHashValue(token));
		break;
	}

}

/**
 * @brief ClientCommand
 * @param[in] clientNum
 */
void ClientCommand(int clientNum)
{
	char cmd[MAX_TOKEN_CHARS];

	trap_Argv(0, cmd, sizeof(cmd));

	if (clientNum == -2)
	{
		TVG_ClientCommandPassThrough(cmd);
		return;
	}

	gclient_t *client = level.clients + clientNum;

	if (!client)
	{
		return;     // not fully in game yet
	}

	if (TVG_commandCheck(client, cmd))
	{
		return;
	}
}

/**
 * @brief Replaces all occurances of "\\\\n" with '\\n'
 *
 * @param[in] s
 */
char *Q_AddCR(char *s)
{
	char *copy, *place, *start;

	if (!*s)
	{
		return s;
	}
	start = s;
	while (*s)
	{
		if (*s == '\\')
		{
			copy  = s;
			place = s;
			s++;
			if (*s == 'n')
			{
				*copy = '\n';
				while (*++s)
				{
					*++copy = *s;
				}
				*++copy = '\0';
				s       = place;
				continue;
			}
		}
		s++;
	}
	return start;
}
