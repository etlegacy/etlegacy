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
 * @file g_cmds.c
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

qboolean G_IsOnFireteam(int entityNum, fireteamData_t **teamNum);

/**
 * @brief G_MatchOnePlayer
 * @param[in] plist
 * @param[out] err
 * @param[in] len
 * @return
 */
qboolean G_MatchOnePlayer(int *plist, char *err, size_t len)
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
int ClientNumbersFromString(char *s, int *plist)
{
	gclient_t *p;
	int       i, found = 0;
	char      s2[MAX_STRING_CHARS];
	char      n2[MAX_STRING_CHARS];
	char      *m;
	char      *end = NULL;

	*plist = -1;

	// if a number is provided, it might be a slot #
	// check the whole string is a number and only if so assume it is a slot number
	//          still fails for players with a name like "23" and there are 24 slots used
	i = (int) strtol(s, &end, 10); // end will be "" when s contains only digits

	if ((!end || !*end) && i >= 0)
	{
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
	SanitizeString(s, s2, qtrue);
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
		SanitizeString(p->pers.netname, n2, qtrue);
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
		int       pids[MAX_CLIENTS];
		char      err[MAX_STRING_CHARS];
		gentity_t *victim;

		if (ClientNumbersFromString(name, pids) != 1)
		{
			G_MatchOnePlayer(pids, err, sizeof(err));
			G_Printf("playsound: %s\n", err);
			return;
		}
		victim = &level.gentities[pids[0]];

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

#ifdef FEATURE_RATING
/**
 * @brief G_SendSkillRating
 * @param[in] ent
 */
static void G_SendSkillRating(gentity_t *ent)
{
	char      buffer[1024];
	int       i, clientNum;
	gclient_t *cl;

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

	Q_strncpyz(buffer, "sra ", sizeof(buffer));

	// win probability
	Q_strcat(buffer, sizeof(buffer), va("%.1f ", level.axisProb * 100.f));
	Q_strcat(buffer, sizeof(buffer), va("%.1f ", level.alliesProb * 100.f));

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];
		Q_strcat(buffer, sizeof(buffer), va("%.3f ", cl->sess.mu - 3 * cl->sess.sigma));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}
#endif

#ifdef FEATURE_PRESTIGE
/**
 * @brief G_SendPrestige
 * @param[in] ent
 */
static void G_SendPrestige(gentity_t *ent)
{
	char      buffer[1024];
	int       i, clientNum;
	gclient_t *cl;

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

	Q_strncpyz(buffer, "pr ", sizeof(buffer));

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];
		Q_strcat(buffer, sizeof(buffer), va("%i ", cl->sess.prestige));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}
#endif

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
 * @param[in] ent
 */
void G_SendScore(gentity_t *ent)
{
	int i         = 0;
	int numSorted = level.numConnectedClients; // send the latest information on all clients
	int count     = 0;
	// commands over 1022 will crash the client so they're
	// pruned in trap_SendServerCommand()
	// 1022 -32 for the startbuffer -3 for the clientNum
	char buffer[987];
	char startbuffer[32];

	*buffer      = '\0';
	*startbuffer = '\0';

#ifdef FEATURE_RATING
	if (g_skillRating.integer)
	{
		G_SendSkillRating(ent);
	}
#endif

#ifdef FEATURE_PRESTIGE
	if (g_prestige.integer)
	{
		G_SendPrestige(ent);
	}
#endif

	Q_strncpyz(startbuffer, va(
				   "sc0 %i %i",
				   level.teamScores[TEAM_AXIS],
				   level.teamScores[TEAM_ALLIES]),
	           sizeof(startbuffer));

	// keep adding scores to the sc0 command until we fill
	// up the buffer.  Any scores that are left will be
	// added on to the sc1 command.
	for (; i < numSorted; ++i)
	{
		// the old version of SendScore() did this.  I removed it
		// originally because it seemed like an unneccessary hack.
		// perhaps it is necessary for compat with CG_Argv()?
		if (count == 33)
		{
			break;
		}
		if (!G_SendScore_Add(ent, i, buffer, sizeof(buffer)))
		{
			break;
		}
		count++;
	}
	trap_SendServerCommand(ent - g_entities, va("%s %i%s", startbuffer, count, buffer));

	if (i == numSorted)
	{
		return;
	}

	count        = 0;
	*buffer      = '\0';
	*startbuffer = '\0';
	Q_strncpyz(startbuffer, "sc1", sizeof(startbuffer));
	for (; i < numSorted; ++i)
	{
		if (!G_SendScore_Add(ent, i, buffer, sizeof(buffer)))
		{
			G_Printf("ERROR: G_SendScore() buffer overflow\n");
			break;
		}
		count++;
	}
	if (!count)
	{
		return;
	}

	trap_SendServerCommand(ent - g_entities, va("%s %i%s", startbuffer, count, buffer));
}

/**
 * @brief Request current scoreboard information
 * @param[out] ent
 */
void Cmd_Score_f(gentity_t *ent)
{
	ent->client->wantsscore = qtrue;
	//G_SendScore( ent );
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
 * @brief Remove case and control characters
 * @param[in] in
 * @param[out] out
 * @param[in] fToLower
 */
void SanitizeString(char *in, char *out, qboolean fToLower)
{
	while (*in)
	{
		if (*in == 27 || *in == '^')
		{
			in++;       // skip color code
			if (*in)
			{
				in++;
			}
			continue;
		}

		if (*in < 32)
		{
			in++;
			continue;
		}

		*out++ = (fToLower) ? tolower(*in++) : *in++;
	}

	*out = 0;
}

/**
 * @brief ClientNumberFromString
 * @param[in] to
 * @param[in] s
 * @return A player number for either a number or name string, -1 if invalid
 */
int ClientNumberFromString(gentity_t *to, char *s)
{
	gclient_t *cl;
	int       idnum;
	char      s2[MAX_STRING_CHARS];
	char      n2[MAX_STRING_CHARS];
	qboolean  fIsNumber = qtrue;

	// See if its a number or string
	for (idnum = 0; idnum < (int)strlen(s) && s[idnum] != 0; idnum++)
	{
		if (s[idnum] < '0' || s[idnum] > '9')
		{
			fIsNumber = qfalse;
			break;
		}
	}

	// check for a name match
	SanitizeString(s, s2, qtrue);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++)
	{
		if (cl->pers.connected != CON_CONNECTED)
		{
			continue;
		}

		SanitizeString(cl->pers.netname, n2, qtrue);
		if (!strcmp(n2, s2))
		{
			return(idnum);
		}
	}

	// numeric values are just slot numbers
	if (fIsNumber)
	{
		idnum = Q_atoi(s);
		if (idnum < 0 || idnum >= level.maxclients)
		{
			CPx(to - g_entities, va("print \"Bad client slot: [lof]%i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED)
		{
			CPx(to - g_entities, va("print \"Client[lof] %i [lon]is not active\n\"", idnum));
			return -1;
		}
		return(idnum);
	}

	CPx(to - g_entities, va("print \"User [lof]%s [lon]is not on the server\n\"", s));
	return -1;
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
 * @brief Give items to a client
 * @param[in,out] ent
 */
void Cmd_Give_f(gentity_t *ent)
{
	char *name, *amt;
	//gitem_t     *it;
	weapon_t weapon;
	qboolean give_all;
	//gentity_t       *it_ent;
	//trace_t     trace;
	int      amount;
	qboolean hasAmount = qfalse;

	if (!ent || !ent->client)
	{
		return;
	}

	if (!CheatsOk(ent))
	{
		return;
	}

	// check for an amount (like "give health 30")
	amt = ConcatArgs(2);
	if (*amt)
	{
		hasAmount = qtrue;
	}
	amount = Q_atoi(amt);

	name = ConcatArgs(1);

	if (Q_stricmp(name, "all") == 0)
	{
		give_all = qtrue;
	}
	else
	{
		give_all = qfalse;
	}

	if (Q_stricmpn(name, "skill", 5) == 0)
	{
		skillType_t skill;
		float       points;

		if ((ent->client->sess.sessionTeam != TEAM_ALLIES && ent->client->sess.sessionTeam != TEAM_AXIS) || g_gamestate.integer != GS_PLAYING)
		{
			trap_SendServerCommand(ent - g_entities, va("print \"give skill: Command not available - player is spectator or game isn't started.\n\""));
			return;
		}

		if (hasAmount) // skill number given
		{
			skill = (skillType_t)amount; // Change amount to skill, so that we can use amount properly

			if (skill >= SK_BATTLE_SENSE && skill < SK_NUM_SKILLS)
			{
				// Detecting the correct amount to move to the next skill level
				points = GetSkillPointUntilLevelUp(ent, skill);
				if (points < 0)
				{
					points = 20;
				}

				G_AddSkillPoints(ent, skill, points);
				G_DebugAddSkillPoints(ent, skill, points, "give skill");

				// ceil the given points to keep consistency with the displayed XP value in HUD
				trap_SendServerCommand(ent - g_entities, va("print \"give skill: Skill %i '%s' increased (+%.0fXP).\n\"", skill, GetSkillTableData(skill)->skillNames, ceil(points)));

			}
			else
			{
				trap_SendServerCommand(ent - g_entities, va("print \"give skill <skill_no>: No valid skill '%i' (0-6).\n\"", skill));
			}
		}
		else
		{
			// bumps all skills with 1 level
			for (skill = SK_BATTLE_SENSE; skill < SK_NUM_SKILLS; skill++)
			{
				// Detecting the correct amount to move to the next skill level
				points = GetSkillPointUntilLevelUp(ent, skill);
				if (points < 0)
				{
					points = 20;
				}

				G_AddSkillPoints(ent, skill, points);
				G_DebugAddSkillPoints(ent, skill, points, "give skill");
			}
			trap_SendServerCommand(ent - g_entities, va("print \"give skill: All skills increased by 1 level.\n\""));
		}
		return;
	}

	if (Q_stricmpn(name, "medal", 5) == 0)
	{
		int i;

		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			if (!ent->client->sess.medals[i])
			{
				ent->client->sess.medals[i] = 1;
				break; // add 1 medal, not all at once..
			}
		}
		ClientUserinfoChanged(ent - g_entities);
		return;
	}

	if (give_all || Q_stricmpn(name, "health", 6) == 0)
	{
		// modified
		if (amount)
		{
			if (amount > 0)
			{
				ent->health += amount;
			}
			else
			{
				// health amount could be negative and deal damage
				G_Damage(ent, ent, ent, NULL, NULL, -amount, DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
			}
		}
		else
		{
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if (!give_all)
		{
			return;
		}
	}

	/*if ( Q_stricmpn( name, "damage", 6) == 0)
	{
	    if(amount) {
	        name = ConcatArgs( 3 );

	        if( *name ) {
	            int client = ClientNumberFromString( ent, name );
	            if( client >= 0 ) {
	                G_Damage( &g_entities[client], ent, ent, NULL, NULL, amount, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
	            }
	        } else {
	            G_Damage( ent, ent, ent, NULL, NULL, amount, DAMAGE_NO_PROTECTION, MOD_UNKNOWN );
	        }
	    }

	    return;
	}*/

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (weapon = 0; weapon < WP_NUM_WEAPONS; weapon++)
		{
			COM_BitSet(ent->client->ps.weapons, weapon);
		}

		if (!give_all)
		{
			return;
		}
	}

	if (give_all || Q_stricmpn(name, "ammo", 4) == 0)
	{
		if (amount)
		{
			if (ent->client->ps.weapon
			    && ent->client->ps.weapon != WP_SATCHEL && ent->client->ps.weapon != WP_SATCHEL_DET
			    )
			{
				Add_Ammo(ent, (weapon_t)ent->client->ps.weapon, amount, qtrue);
			}
		}
		else
		{
			for (weapon = WP_KNIFE ; weapon < WP_NUM_WEAPONS ; weapon++)
			{
				if (COM_BitCheck(ent->client->ps.weapons, weapon) && weapon != WP_SATCHEL && weapon != WP_SATCHEL_DET)
				{
					Add_Ammo(ent, weapon, 9999, qtrue);
				}
			}
		}

		if (!give_all)
		{
			return;
		}
	}

	// "give allammo <n>" allows you to give a specific amount of ammo to /all/ weapons while
	// allowing "give ammo <n>" to only give to the selected weap.
	if (Q_stricmpn(name, "allammo", 7) == 0 && amount)
	{
		for (weapon = WP_KNIFE ; weapon < WP_NUM_WEAPONS; weapon++)
			Add_Ammo(ent, weapon, amount, qtrue);

		if (!give_all)
		{
			return;
		}
	}

	// Wolf keys
	if (give_all || Q_stricmp(name, "keys") == 0)
	{
		ent->client->ps.stats[STAT_KEYS] = (1 << KEY_NUM_KEYS) - 2;
		if (!give_all)
		{
			return;
		}
	}

	// spawn a specific item right on the player
	/*if ( !give_all ) {
	    it = BG_FindItem (name);
	    if (!it) {
	        return;
	    }

	    it_ent = G_Spawn();
	    VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
	    it_ent->classname = it->classname;
	    G_SpawnItem (it_ent, it);
	    FinishSpawningItem(it_ent );
	    Com_Memset( &trace, 0, sizeof( trace ) );
	    it_ent->active = qtrue;
	    Touch_Item (it_ent, ent, &trace);
	    it_ent->active = qfalse;
	    if (it_ent->inuse) {
	        G_FreeEntity( it_ent );
	    }
	}*/
}

/**
 * @brief Sets client to godmode
 * @param[in,out] ent
 *
 * @note argv(0) god
 */
void Cmd_God_f(gentity_t *ent)
{
	char     *msg;
	char     *name;
	qboolean godAll = qfalse;

	if (!CheatsOk(ent))
	{
		return;
	}

	name = ConcatArgs(1);

	// are we supposed to make all our teammates gods too?
	if (Q_stricmp(name, "all") == 0)
	{
		godAll = qtrue;
	}

	// can only use this cheat in single player
	if (godAll && g_gametype.integer == GT_SINGLE_PLAYER)
	{
		int       j;
		qboolean  settingFlag = qtrue;
		gentity_t *other;

		// are we turning it on or off?
		if (ent->flags & FL_GODMODE)
		{
			settingFlag = qfalse;
		}

		// loop through all players
		for (j = 0; j < level.maxclients; j++)
		{
			other = &g_entities[j];
			// if they're on the same team
			if (OnSameTeam(other, ent))
			{
				// set or clear the flag
				if (settingFlag)
				{
					other->flags |= FL_GODMODE;
				}
				else
				{
					other->flags &= ~FL_GODMODE;
				}
			}
		}
		if (settingFlag)
		{
			msg = "godmode all ON\n";
		}
		else
		{
			msg = "godmode all OFF\n";
		}
	}
	else
	{
		if (!Q_stricmp(name, "on") || Q_atoi(name))
		{
			ent->flags |= FL_GODMODE;
		}
		else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
		{
			ent->flags &= ~FL_GODMODE;
		}
		else
		{
			ent->flags ^= FL_GODMODE;
		}
		if (!(ent->flags & FL_GODMODE))
		{
			msg = "godmode OFF\n";
		}
		else
		{
			msg = "godmode ON\n";
		}
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/**
 * @brief Sets client to nofatigue
 * @param[in,out] ent
 *
 * @note argv(0) nofatigue
 */
void Cmd_Nofatigue_f(gentity_t *ent)
{
	char *msg;
	char *name = ConcatArgs(1);

	if (!CheatsOk(ent))
	{
		return;
	}

	if (!Q_stricmp(name, "on") || Q_atoi(name))
	{
		ent->flags |= FL_NOFATIGUE;
	}
	else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
	{
		ent->flags &= ~FL_NOFATIGUE;
	}
	else
	{
		ent->flags ^= FL_NOFATIGUE;
	}

	if (!(ent->flags & FL_NOFATIGUE))
	{
		msg = "nofatigue OFF\n";
	}
	else
	{
		msg = "nofatigue ON\n";
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/**
 * @brief Sets client to notarget
 * @param[in,out] ent
 *
 * @note argv(0) notarget
 */
void Cmd_Notarget_f(gentity_t *ent)
{
	char *msg;

	if (!CheatsOk(ent))
	{
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
	{
		msg = "notarget OFF\n";
	}
	else
	{
		msg = "notarget ON\n";
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/**
 * @brief Cmd_Noclip_f
 * @param[in,out] ent
 *
 * @note argv(0) noclip
 */
void Cmd_Noclip_f(gentity_t *ent)
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
 * @brief Sets client to nostamina
 * @param[in,out] ent
 *
 * @note argv(0) nostamina
 */
void Cmd_Nostamina_f(gentity_t *ent)
{
	char *msg;
	char *name = ConcatArgs(1);

	if (!CheatsOk(ent))
	{
		return;
	}

	if (!Q_stricmp(name, "on") || Q_atoi(name))
	{
		ent->flags |= FL_NOSTAMINA;
	}
	else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
	{
		ent->flags &= ~FL_NOSTAMINA;
	}
	else
	{
		ent->flags ^= FL_NOSTAMINA;
	}

	if (!(ent->flags & FL_NOSTAMINA))
	{
		msg = "nostamina OFF\n";
	}
	else
	{
		msg = "nostamina ON\n";
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/**
 * @brief Cmd_Kill_f
 * @param[in,out] ent
 */
void Cmd_Kill_f(gentity_t *ent)
{
	if (ent->health <= 0)
	{
#ifdef FEATURE_OMNIBOT
		// cs: bots have to go to limbo when issuing /kill otherwise it's trouble
		if (ent->r.svFlags & SVF_BOT)
		{
			limbo(ent, qtrue);
			return;
		}
#endif
		limbo(ent, qtrue);
		return;
	}

	if (ent->client->freezed)
	{
		trap_SendServerCommand(ent - g_entities, "cp \"You are frozen - ^3/kill^7 is disabled.\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
	    (ent->client->ps.pm_flags & PMF_LIMBO) ||
	    ent->health <= 0 || level.match_pause != PAUSE_NONE)
	{
		return;
	}

	ent->flags                                  &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH]           = ent->health = 0;
	ent->client->ps.persistant[PERS_HWEAPON_USE] = 0; // if using /kill while at MG42

	player_die(ent, ent, ent, (g_gamestate.integer == GS_PLAYING) ? 100000 : 135, MOD_SUICIDE);
}

/**
 * @brief Cmd_DropObjective_f
 * @param[in,out] ent
 */
void Cmd_DropObjective_f(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (ent->health <= 0)
	{
		return;
	}

	if (!ent->client->ps.powerups[PW_REDFLAG] && !ent->client->ps.powerups[PW_BLUEFLAG])
	{
		return;
	}

	if (level.time - ent->client->pickObjectiveTime < 10000)
	{
		CP("cp \"You can't drop objective right after picking it up.\"");
		return;
	}

	G_DropItems(ent);
}

/**
 * @brief G_TeamDataForString
 * @param[in] teamstr
 * @param[in] clientNum
 * @param[out] team
 * @param[out] sState
 * @param[in,out] specClient
 */
void G_TeamDataForString(const char *teamstr, int clientNum, team_t *team, spectatorState_t *sState)
{
	*sState = SPECTATOR_NOT;

	if (!Q_stricmp(teamstr, "spectator") || !Q_stricmp(teamstr, "s"))
	{
		*team   = TEAM_SPECTATOR;
		*sState = SPECTATOR_FREE;
	}
	else if (!Q_stricmp(teamstr, "red") || !Q_stricmp(teamstr, "r") || !Q_stricmp(teamstr, "axis"))
	{
		*team = TEAM_AXIS;
	}
	else if (!Q_stricmp(teamstr, "blue") || !Q_stricmp(teamstr, "b") || !Q_stricmp(teamstr, "allies"))
	{
		*team = TEAM_ALLIES;
	}
	else
	{
		*team = PickTeam(clientNum);
		if (!G_teamJoinCheck(*team, &g_entities[clientNum]))
		{
			*team = ((TEAM_AXIS | TEAM_ALLIES) & ~*team);
		}
	}
}

/**
 * @brief Drops items. Currently only red-/blueflag.
 * @param[in,out] self
 */
void G_DropItems(gentity_t *self)
{
	gitem_t *item = NULL;

	// drop flag regardless
	if (self->client->ps.powerups[PW_REDFLAG])
	{
		item                                  = BG_GetItem(ITEM_RED_FLAG);
		self->client->ps.powerups[PW_REDFLAG] = 0;

		// update objective indicator
		level.redFlagCounter -= 1;
		G_globalFlagIndicator();
	}
	if (self->client->ps.powerups[PW_BLUEFLAG])
	{
		item                                   = BG_GetItem(ITEM_BLUE_FLAG);
		self->client->ps.powerups[PW_BLUEFLAG] = 0;

		// update objective indicator
		level.blueFlagCounter -= 1;
		G_globalFlagIndicator();
	}

	if (item)
	{
		vec3_t    launchvel = { 0, 0, 0 };
		vec3_t    forward;
		vec3_t    origin;
		vec3_t    angles;
		gentity_t *flag;
		vec3_t    mins;
		vec3_t    maxs;
		vec3_t    viewpos;
		trace_t   tr;

		VectorCopy(self->client->ps.origin, origin);
		// if the player hasn't died, then assume he's throwing objective
		if (self->health > 0)
		{
			VectorCopy(self->client->ps.viewangles, angles);
			if (angles[PITCH] > 0)
			{
				angles[PITCH] = 0;
			}
			AngleVectors(angles, forward, NULL, NULL);
			VectorMA(self->client->ps.velocity, 96, forward, launchvel);
			VectorMA(origin, 36.0f, forward, origin);
			origin[2] += self->client->ps.viewheight;

			// prevent stuck item in solid
			VectorCopy(self->client->ps.origin, viewpos);
			VectorSet(mins, -(ITEM_RADIUS + 8), -(ITEM_RADIUS + 8), 0);
			VectorSet(maxs, (ITEM_RADIUS + 8), (ITEM_RADIUS + 8), 2 * (ITEM_RADIUS + 8));

			trap_EngineerTrace(self, &tr, viewpos, mins, maxs, origin, self->s.number, MASK_MISSILESHOT);
			if (tr.startsolid)
			{
				VectorCopy(forward, viewpos);
				VectorNormalizeFast(viewpos);
				VectorMA(self->r.currentOrigin, -24.f, viewpos, viewpos);

				trap_EngineerTrace(self, &tr, viewpos, mins, maxs, origin, self->s.number, MASK_MISSILESHOT);

				VectorCopy(tr.endpos, origin);
			}
			else if (tr.fraction < 1)       // oops, bad launch spot
			{
				VectorCopy(tr.endpos, origin);
				SnapVectorTowards(origin, viewpos);
			}

			// set timer
			self->client->dropObjectiveTime = level.time;
		}

		flag = LaunchItem(item, origin, launchvel, self->s.number);

		flag->s.modelindex2 = self->s.otherEntityNum2; // FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
		flag->message       = self->message; // also restore item name

#ifdef OMNIBOTS
		Bot_Util_SendTrigger(flag, NULL, va("%s dropped.", flag->message), "dropped");
#endif
		// Clear out player's temp copies
		self->s.otherEntityNum2 = 0;
		self->message           = NULL;
	}
}

/**
 * @brief SetTeam
 * @param[in,out] ent
 * @param[in] s
 * @param[in] force
 * @param[in] w1
 * @param[in] w2
 * @param[in] setweapons
 * @return
 */
qboolean SetTeam(gentity_t *ent, const char *s, qboolean force, weapon_t w1, weapon_t w2, qboolean setweapons)
{
	team_t           team, oldTeam;
	gclient_t        *client   = ent->client;
	int              clientNum = client - level.clients;
	spectatorState_t specState;
	int              respawnsLeft = client->ps.persistant[PERS_RESPAWNS_LEFT]; // preserve respawn count

	// see what change is requested

	G_TeamDataForString(s, client - level.clients, &team, &specState);

	if (ent->client->freezed)
	{
		trap_SendServerCommand(clientNum, "cp \"You are frozen!\n\"");
		return qfalse;
	}

	if (team != TEAM_SPECTATOR)
	{
		// Ensure the player can join
		if (!G_teamJoinCheck(team, ent))
		{
			// Leave them where they were before the command was issued
			return qfalse;
		}

		if (g_noTeamSwitching.integer && (team != ent->client->sess.sessionTeam && ent->client->sess.sessionTeam != TEAM_SPECTATOR) && g_gamestate.integer == GS_PLAYING && !force)
		{
			trap_SendServerCommand(clientNum, "cp \"You cannot switch during a match, please wait until the round ends.\"");
			return qfalse;  // ignore the request
		}

		if (((g_gametype.integer == GT_WOLF_LMS && g_lms_teamForceBalance.integer) || g_teamForceBalance.integer) && !force)
		{
			int counts[TEAM_NUM_TEAMS];

			counts[TEAM_ALLIES] = TeamCount(ent - g_entities, TEAM_ALLIES);
			counts[TEAM_AXIS]   = TeamCount(ent - g_entities, TEAM_AXIS);

			// We allow a spread of one
			if (team == TEAM_AXIS && counts[TEAM_AXIS] - counts[TEAM_ALLIES] >= 1)
			{
				CP("cp \"The Axis has too many players.\n\"");
				return qfalse; // ignore the request
			}
			if (team == TEAM_ALLIES && counts[TEAM_ALLIES] - counts[TEAM_AXIS] >= 1)
			{
				CP("cp \"The Allies have too many players.\n\"");
				return qfalse; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}
	}

	if (g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer)
	{
		team = TEAM_SPECTATOR;
	}

	// decide if we will allow the change
	oldTeam = client->sess.sessionTeam;
	if (team == oldTeam && team != TEAM_SPECTATOR)
	{
		return qfalse;
	}

	// prevent players from switching to regain deployments
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if ((g_maxlives.integer > 0 ||
		     (g_alliedmaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_ALLIES) ||
		     (g_axismaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_AXIS)) &&
		    ent->client->ps.persistant[PERS_RESPAWNS_LEFT] == 0 && oldTeam != TEAM_SPECTATOR)
		{
			if (g_gamestate.integer == GS_PLAYING) // but allow changing team in warmup
			{
				CP("cp \"You can't switch teams because you are out of lives.\n\" 3");
				return qfalse;  // ignore the request
			}
		}
	}

	// execute the team change
	if (team != TEAM_SPECTATOR)
	{
		client->pers.initialSpawn = qfalse;
#ifdef FEATURE_MULTIVIEW
		// no MV in-game
		if (client->pers.mvCount > 0)
		{
			G_smvRemoveInvalidClients(ent, TEAM_AXIS);
			G_smvRemoveInvalidClients(ent, TEAM_ALLIES);
		}
#endif
	}

	if (oldTeam != TEAM_SPECTATOR)
	{
		if (!(ent->client->ps.pm_flags & PMF_LIMBO))
		{
			// Kill him (makes sure he loses flags, etc)
			ent->flags                        &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die(ent, ent, ent, 100000, MOD_SWITCHTEAM);
		}
	}
	// they go to the end of the line for tournements
	if (team == TEAM_SPECTATOR)
	{
		client->sess.spectatorTime = level.time;
		if (!client->sess.referee)
		{
			client->pers.invite = 0;
		}
#ifdef FEATURE_MULTIVIEW
		if (team != oldTeam)
		{
			G_smvAllRemoveSingleClient(ent - g_entities);
		}
#endif
	}

	G_LeaveTank(ent, qfalse);
	G_RemoveClientFromFireteams(clientNum, qtrue, qfalse);
	if (g_landminetimeout.integer)
	{
		G_ExplodeMines(ent);
	}
	G_FadeItems(ent, MOD_SATCHEL);

	// remove ourself from teamlists
	{
		int                  i;
		mapEntityData_t      *mEnt;
		mapEntityData_Team_t *teamList;

		for (i = 0; i < 2; i++)
		{
			teamList = &mapEntityData[i];

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(teamList, mEnt);
			}

			mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

			while (mEnt)
			{
				mapEntityData_t *mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);

				G_FreeMapEntityData(teamList, mEntFree);
			}
		}
	}
	client->sess.spec_team       = 0;
	client->sess.sessionTeam     = team;
	client->sess.spectatorState  = specState;
	client->sess.spectatorClient = 0;
	client->pers.ready           = qfalse;

	// During team switching you can sometime spawn immediately
	client->pers.lastReinforceTime = 0;

	// (l)users will spam spec messages... honest!
	if (team != oldTeam)
	{
		gentity_t *tent = G_PopupMessage(PM_TEAM);

		tent->s.effect2Time = team;
		tent->s.effect3Time = clientNum;
		tent->s.density     = 0;
	}

	if (setweapons)
	{
		G_SetClientWeapons(ent, w1, w2, qfalse);
	}

	// get and distribute relevent paramters
	G_UpdateCharacter(client);              // FIXME : doesn't ClientBegin take care of this already?
	ClientUserinfoChanged(clientNum);

	ClientBegin(clientNum);

	// restore old respawn count (players cannot jump from team to team to regain lives)
	if (respawnsLeft >= 0 && oldTeam != TEAM_SPECTATOR)
	{
		client->ps.persistant[PERS_RESPAWNS_LEFT] = respawnsLeft;
	}

	G_verifyMatchState(oldTeam);

	// Reset stats when changing teams
	//if (team != oldTeam)
	//{
	//	G_deleteStats(clientNum);
	//}

	if (g_gamestate.integer == GS_PLAYING && (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES))
	{
		if (g_gametype.integer == GT_WOLF_LMS && level.numTeamClients[0] > 0 && level.numTeamClients[1] > 0)
		{
			trap_SendServerCommand(clientNum, "cp \"Will spawn next round, please wait.\n\"");
			limbo(ent, qfalse);
			return qfalse;
		}
		else
		{
			int i;
			int x = client->sess.sessionTeam - TEAM_AXIS;

			for (i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; i++)
			{
				if (level.commanderSounds[x][i].index)
				{
					gentity_t *tent = G_TempEntityNotLinked(EV_GLOBAL_CLIENT_SOUND);

					tent->s.eventParm    = level.commanderSounds[x][i].index - 1;
					tent->s.teamNum      = clientNum;
					tent->r.singleClient = clientNum;
					tent->r.svFlags      = SVF_SINGLECLIENT | SVF_BROADCAST;
				}
			}
		}
	}

	ent->client->pers.autofireteamCreateEndTime = 0;
	ent->client->pers.autofireteamJoinEndTime   = 0;

	if (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES)
	{
		if (g_autoFireteams.integer)
		{
			fireteamData_t *ft = G_FindFreePublicFireteam(client->sess.sessionTeam);

			if (ft)
			{
				trap_SendServerCommand(ent - g_entities, "aftj -1");
				ent->client->pers.autofireteamJoinEndTime = level.time + 20500;
			}
			else
			{
				trap_SendServerCommand(ent - g_entities, "aftc -1");
				ent->client->pers.autofireteamCreateEndTime = level.time + 20500;
			}
		}
	}

	if (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES)
	{
		ent->client->inactivityTime        = level.time + (g_inactivity.integer ? g_inactivity.integer : 60) * 1000;
		ent->client->inactivitySecondsLeft = (g_inactivity.integer) ? g_inactivity.integer : 60;
	}
	else
	{
		ent->client->inactivityTime        = level.time + (g_spectatorInactivity.integer ? g_spectatorInactivity.integer : 60) * 1000;
		ent->client->inactivitySecondsLeft = (g_spectatorInactivity.integer) ? g_spectatorInactivity.integer : 60;
	}

#ifdef FEATURE_RATING
	if (g_skillRating.integer)
	{
		level.axisProb   = G_CalculateWinProbability(TEAM_AXIS);
		level.alliesProb = 1.0f - level.axisProb;
	}
#endif
	return qtrue;
}

/**
 * @brief If the client being followed leaves the game, or you just want to drop
 * to free floating spectator mode.
 *
 * @param[in,out] ent
 */
void StopFollowing(gentity_t *ent)
{
	// divert behaviour if TEAM_SPECTATOR, moved the code from SpectatorThink to put back into free fly correctly
	// (I am not sure this can be called in non-TEAM_SPECTATOR situation, better be safe)
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		// drop to free floating, somewhere above the current position (that's the client you were following)
		vec3_t    pos, angle;
		gclient_t *client = ent->client;

		// FIXME: to avoid having a spectator with a gun..
		//if (client->wbuttons & WBUTTON_ATTACK2 || client->buttons & BUTTON_ATTACK)
		//{
		//	return;
		//}

		VectorCopy(client->ps.origin, pos);
		//pos[2] += 16; // removing for now
		VectorCopy(client->ps.viewangles, angle);
		// Need this as it gets spec mode reset properly
		SetTeam(ent, "s", qtrue, WP_NONE, WP_NONE, qfalse);
		VectorCopy(pos, client->ps.origin);
		SetClientViewAngle(ent, angle);
	}
	else
	{
		// legacy code, FIXME: useless?
		// no this is for limbo i'd guess
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->ps.clientNum        = ent - g_entities;
	}
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
 * @brief Checks for heavy and rifle weapons restriction
 *
 * @param[in] ent
 * @param[in] weapon
 * @return
 *
 * @todo This function needs some rework: count picked up opposite team weapons too
 *       see CG_LimboPanel_RealWeaponIsDisabled
 */
qboolean G_IsWeaponDisabled(gentity_t *ent, weapon_t weapon)
{
	int        playerCount, weaponCount, maxCount = -1;
	const char *weaponString = "";

	// allow selecting weapons as spectator for bots (to avoid endless loops in pfnChangeTeam())
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR && !(ent->r.svFlags & SVF_BOT))
	{
		return qtrue;
	}

	if (!(GetWeaponTableData(weapon)->skillBased == SK_HEAVY_WEAPONS || (GetWeaponTableData(weapon)->type & WEAPON_TYPE_RIFLENADE)
	      || (GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->type & WEAPON_TYPE_RIFLENADE)))
	{
		return qfalse;
	}

	playerCount = G_TeamCount(ent, -1);
	weaponCount = G_TeamCount(ent, weapon);

	// total percentage restriction
	if (GetWeaponTableData(weapon)->skillBased == SK_HEAVY_WEAPONS && weaponCount >= ceil(playerCount * g_heavyWeaponRestriction.integer * 0.01))
	{
		return qtrue;
	}

	// single weapon restrictions
	if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_PANZER)
	{
		maxCount     = team_maxRockets.integer;
		weaponString = team_maxRockets.string;
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MG)
	{
		maxCount     = team_maxMachineguns.integer;
		weaponString = team_maxMachineguns.string;
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MORTAR)
	{
		maxCount     = team_maxMortars.integer;
		weaponString = team_maxMortars.string;
	}
	else if ((GetWeaponTableData(weapon)->type & WEAPON_TYPE_RIFLENADE)
	         || (GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->type & WEAPON_TYPE_RIFLENADE))
	{
		maxCount     = team_maxRiflegrenades.integer;
		weaponString = team_maxRiflegrenades.string;
	}
	else if (weapon == WP_FLAMETHROWER)
	{
		maxCount     = team_maxFlamers.integer;
		weaponString = team_maxFlamers.string;
	}

	if (maxCount == -1)
	{
		return qfalse;
	}

	if (strstr(weaponString, "%-"))
	{
		maxCount = floor(maxCount * playerCount * 0.01);
	}
	else if (strstr(weaponString, "%"))
	{
		maxCount = ceil(maxCount * playerCount * 0.01);
	}

	if (GetWeaponTableData(weapon)->weapAlts)
	{
		// add basic weapons
		weaponCount += G_TeamCount(ent, GetWeaponTableData(weapon)->weapAlts);
	}

	if (weaponCount >= maxCount)
	{
		if (ent->client->ps.pm_flags & PMF_LIMBO)
		{
			CP(va("cp \"^1*^3 %s not available!^1 *\" 1", GetWeaponTableData(weapon)->desc));
		}
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief G_SetClientWeapons
 * @param[in,out] ent
 * @param[in] w1
 * @param[in] w2
 * @param[in] updateclient
 */
void G_SetClientWeapons(gentity_t *ent, weapon_t w1, weapon_t w2, qboolean updateclient)
{
	qboolean changed = qfalse;

	if (ent->client->sess.latchPlayerWeapon2 != w2)
	{
		ent->client->sess.latchPlayerWeapon2 = w2;
		changed                              = qtrue;
	}

	if (!G_IsWeaponDisabled(ent, w1))
	{
		if (ent->client->sess.latchPlayerWeapon != w1)
		{
			ent->client->sess.latchPlayerWeapon = w1;
			changed                             = qtrue;
		}
	}
	else
	{
		if (ent->client->sess.latchPlayerWeapon)
		{
			ent->client->sess.latchPlayerWeapon = WP_NONE;
			changed                             = qtrue;
		}
	}

	if (updateclient && changed)
	{
		ClientUserinfoChanged(ent - g_entities);
	}
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
 * @brief G_IsClassFull
 * @param[in] ent
 * @param[in] playerType
 * @param[in] team
 * @return
 */
qboolean G_IsClassFull(gentity_t *ent, int playerType, team_t team)
{
	int maxCount, count, tcount;

	if (playerType < PC_SOLDIER || playerType > PC_COVERTOPS || team == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	count  = G_ClassCount(ent, playerType, team);
	tcount = G_NumPlayersOnTeam(team);
	if (ent->client->sess.sessionTeam != team)
	{
		tcount++;
	}

	switch (playerType)
	{
	case PC_SOLDIER:
		if (team_maxSoldiers.integer == -1)
		{
			break;
		}
		maxCount = team_maxSoldiers.integer;
		if (strstr(team_maxSoldiers.string, "%-"))
		{
			maxCount = floor(team_maxSoldiers.integer * tcount * 0.01);
		}
		else if (strstr(team_maxSoldiers.string, "%"))
		{
			maxCount = ceil(team_maxSoldiers.integer * tcount * 0.01);
		}

		if (count >= maxCount)
		{
			CP("cp \"^1Soldier^7 is not available! Choose another class!\n\"");
			return qtrue;
		}
		break;
	case PC_MEDIC:
		if (team_maxMedics.integer == -1)
		{
			break;
		}
		maxCount = team_maxMedics.integer;
		if (strstr(team_maxMedics.string, "%-"))
		{
			maxCount = floor(team_maxMedics.integer * tcount * 0.01);
		}
		else if (strstr(team_maxMedics.string, "%"))
		{
			maxCount = ceil(team_maxMedics.integer * tcount * 0.01);
		}
		if (count >= maxCount)
		{
			CP("cp \"^1Medic^7 is not available! Choose another class!\n\"");
			return qtrue;
		}
		break;
	case PC_ENGINEER:
		if (team_maxEngineers.integer == -1)
		{
			break;
		}
		maxCount = team_maxEngineers.integer;
		if (strstr(team_maxEngineers.string, "%-"))
		{
			maxCount = floor(team_maxEngineers.integer * tcount * 0.01);
		}
		else if (strstr(team_maxEngineers.string, "%"))
		{
			maxCount = ceil(team_maxEngineers.integer * tcount * 0.01);
		}
		if (count >= maxCount)
		{
			CP("cp \"^1Engineer^7 is not available! Choose another class!\n\"");
			return qtrue;
		}
		break;
	case PC_FIELDOPS:
		if (team_maxFieldops.integer == -1)
		{
			break;
		}
		maxCount = team_maxFieldops.integer;
		if (strstr(team_maxFieldops.string, "%-"))
		{
			maxCount = floor(team_maxFieldops.integer * tcount * 0.01);
		}
		else if (strstr(team_maxFieldops.string, "%"))
		{
			maxCount = ceil(team_maxFieldops.integer * tcount * 0.01);
		}
		if (count >= maxCount)
		{
			CP("cp \"^1Field Ops^7 is not available! Choose another class!\n\"");
			return qtrue;
		}
		break;
	case PC_COVERTOPS:
		if (team_maxCovertops.integer == -1)
		{
			break;
		}
		maxCount = team_maxCovertops.integer;
		if (strstr(team_maxCovertops.string, "%-"))
		{
			maxCount = floor(team_maxCovertops.integer * tcount * 0.01);
		}
		else if (strstr(team_maxCovertops.string, "%"))
		{
			maxCount = ceil(team_maxCovertops.integer * tcount * 0.01);
		}
		if (count >= maxCount)
		{
			CP("cp \"^1Covert Ops^7 is not available! Choose another class!\n\"");
			return qtrue;
		}
		break;
	default:
		break;
	}
	return qfalse;
}

/**
 * @brief Cmd_Team_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param fValue - unused
 */
void Cmd_Team_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	char             s[MAX_TOKEN_CHARS];
	char             ptype[4];
	char             weap[4], weap2[4];
	weapon_t         w, w2;
	int              playerType;
	team_t           team;
	spectatorState_t specState;
	qboolean         classChange;

	if (trap_Argc() < 2)
	{
		char *pszTeamName;

		switch (ent->client->sess.sessionTeam)
		{
		case TEAM_ALLIES:
			pszTeamName = "Allies";
			break;
		case TEAM_AXIS:
			pszTeamName = "Axis";
			break;
		case TEAM_SPECTATOR:
			pszTeamName = "Spectator";
			break;
		case TEAM_FREE:
		default:
			pszTeamName = "Free";
			break;
		}

		CP(va("print \"%s team\n\"", pszTeamName));
		return;
	}

	trap_Argv(1, s, sizeof(s));
	trap_Argv(2, ptype, sizeof(ptype));
	trap_Argv(3, weap, sizeof(weap));
	trap_Argv(4, weap2, sizeof(weap2));

	w  = Q_atoi(weap);
	w2 = Q_atoi(weap2);

	G_TeamDataForString(s, ent->s.clientNum, &team, &specState);

	// don't allow shoutcasters to join teams
	if (ent->client->sess.shoutcaster && (team == TEAM_ALLIES || team == TEAM_AXIS))
	{
		CP("print \"team: shoutcasters may not join a team\n\"");
		CP("cp \"Shoutcasters may not join a team.\n\"");
		return;
	}

	if (*ptype)
	{
		playerType = Q_atoi(ptype);
	}
	else
	{
		playerType = ent->client->sess.playerType;
	}

	if (playerType < PC_SOLDIER || playerType > PC_COVERTOPS)
	{
		playerType = PC_SOLDIER;
	}

	if (G_IsClassFull(ent, playerType, team))
	{
		CP("print \"team: class is not available\n\"");
		return;
	}

	if (ent->client->sess.playerType != playerType || ent->client->sess.latchPlayerType != playerType)
	{
		classChange = qtrue;

		// default primary weapon for class and team
		if (!IS_VALID_WEAPON(w))
		{
			w = GetPlayerClassesData(team, playerType)->classPrimaryWeapons[0].weapon;
		}

		// default secondary weapon for class and team
		if (!IS_VALID_WEAPON(w2))
		{
			w2 = GetPlayerClassesData(team, playerType)->classSecondaryWeapons[0].weapon;
		}
	}
	else
	{
		classChange = qfalse;

		// "swap" team, try to keep the previous selected weapons or equivalent if no one were selected
		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			// primary weapon
			if (!IS_VALID_WEAPON(w))
			{
				w = ent->client->sess.playerWeapon;

				if (GetWeaponTableData(ent->client->sess.playerWeapon)->weapEquiv)
				{
					w = GetWeaponTableData(ent->client->sess.playerWeapon)->weapEquiv;
				}
			}

			// secondary weapon
			if (!IS_VALID_WEAPON(w2))
			{
				w2 = ent->client->sess.playerWeapon2;

				if (GetWeaponTableData(ent->client->sess.playerWeapon2)->weapEquiv)
				{
					w2 = GetWeaponTableData(ent->client->sess.playerWeapon2)->weapEquiv;
				}
			}
		}
	}

	ent->client->sess.latchPlayerType = playerType;

	if (!SetTeam(ent, s, qfalse, w, w2, qtrue))
	{
		if (classChange)
		{
			G_SetClientWeapons(ent, w, w2, qfalse);
			ClientUserinfoChanged(ent - g_entities);
		}
		else
		{
			G_SetClientWeapons(ent, w, w2, qtrue);
		}
	}
}

/**
 * @brief Cmd_ResetSetup_f
 * @param[in,out] ent
 */
void Cmd_ResetSetup_f(gentity_t *ent)
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
 * @param fValue - unused
 */
void Cmd_Follow_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue)
{
	int  i;
	char arg[MAX_TOKEN_CHARS];
	int  pids[MAX_CLIENTS];

	if (trap_Argc() != 2)
	{
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			StopFollowing(ent);
		}
		return;
	}

	if ((ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES) && !(ent->client->ps.pm_flags & PMF_LIMBO))
	{
		CP("print \"Can't follow while not in limbo if on a team!\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	// Let /follow match partial names
	// Use > instead of != since could be a team name
	if (ClientNumbersFromString(arg, pids) > 1)
	{
		CP("print \"Partial Name Matches more than 1 Player.\n\"");
		return;
	}
	i = pids[0];

	if (i == -1)
	{
		if (!Q_stricmp(arg, "allies"))
		{
			i = TEAM_ALLIES;
		}
		else if (!Q_stricmp(arg, "axis"))
		{
			i = TEAM_AXIS;
		}
		else
		{
			return;
		}
		if ((ent->client->sess.sessionTeam == TEAM_AXIS ||
		     ent->client->sess.sessionTeam == TEAM_ALLIES) &&
		    ent->client->sess.sessionTeam != i)
		{
			CP("print \"Can't follow a player on an enemy team!\n\"");
			return;
		}

		if (!TeamCount(ent - g_entities, i))
		{
			CP(va("print \"The %s team %s empty!  Follow command ignored.\n\"", aTeams[i],
			      ((ent->client->sess.sessionTeam != i) ? "is" : "would be")));
			return;
		}

		// Allow for simple toggle
		if (ent->client->sess.spec_team != i)
		{
			if (teamInfo[i].spec_lock && !(ent->client->sess.spec_invite & i))
			{
				CP(va("print \"Sorry, the %s team is locked from spectators.\n\"", aTeams[i]));
			}
			else
			{
				ent->client->sess.spec_team = i;
				CP(va("print \"Spectator follow is now locked on the %s team.\n\"", aTeams[i]));
				Cmd_FollowCycle_f(ent, 1, qfalse);
			}
		}
		else
		{
			ent->client->sess.spec_team = 0;
			CP(va("print \"%s team spectating is now disabled.\n\"", aTeams[i]));
		}

		return;
	}

	// Can't follow enemy players if not a spectator
	if ((ent->client->sess.sessionTeam == TEAM_AXIS ||
	     ent->client->sess.sessionTeam == TEAM_ALLIES) &&
	    ent->client->sess.sessionTeam != level.clients[i].sess.sessionTeam)
	{
		CP("print \"Can't follow a player on an enemy team!\n\"");
		return;
	}

	// can't follow self
	if (&level.clients[i] == ent->client)
	{
		return;
	}

	// can't follow another spectator, but shoutcasters can follow other shoutcasters
	if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR && (!level.clients[i].sess.shoutcaster || !ent->client->sess.shoutcaster))
	{
		return;
	}

	if (level.clients[i].ps.pm_flags & PMF_LIMBO)
	{
		return;
	}

	// can't follow a player on a speclocked team, unless allowed
	if (!G_allowFollow(ent, level.clients[i].sess.sessionTeam))
	{
		CP(va("print \"Sorry, the %s team is locked from spectators.\n\"", aTeams[level.clients[i].sess.sessionTeam]));
		return;
	}

	ent->client->sess.spectatorState  = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/**
 * @brief Cmd_FollowCycle_f
 * @param[in,out] ent
 * @param[in] dir
 * @param[in] skipBots
 */
void Cmd_FollowCycle_f(gentity_t *ent, int dir, qboolean skipBots)
{
	int clientnum;

	// first set them to spectator
	if ((ent->client->sess.spectatorState == SPECTATOR_NOT) && (!(ent->client->ps.pm_flags & PMF_LIMBO))) // for limbo state
	{
		SetTeam(ent, "s", qfalse, WP_NONE, WP_NONE, qfalse);
	}

	if (dir != 1 && dir != -1)
	{
		G_Error("Cmd_FollowCycle_f: bad dir %i\n", dir);
	}

	clientnum = ent->client->sess.spectatorClient;
	do
	{
		clientnum += dir;
		if (clientnum >= level.maxclients)
		{
			clientnum = 0;
		}
		if (clientnum < 0)
		{
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if (level.clients[clientnum].pers.connected != CON_CONNECTED)
		{
			continue;
		}

		// can't follow another spectator
		if (level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		// couple extra checks for limbo mode
		if (ent->client->ps.pm_flags & PMF_LIMBO)
		{
			if (level.clients[clientnum].ps.pm_flags & PMF_LIMBO)
			{
				continue;
			}
			if (level.clients[clientnum].sess.sessionTeam != ent->client->sess.sessionTeam)
			{
				continue;
			}
		}

		if (level.clients[clientnum].ps.pm_flags & PMF_LIMBO)
		{
			continue;
		}

		if (!G_desiredFollow(ent, level.clients[clientnum].sess.sessionTeam))
		{
			continue;
		}

		// cycle through humans only?..
		if (skipBots && (g_entities[clientnum].r.svFlags & SVF_BOT))
		{
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState  = SPECTATOR_FOLLOW;
		return;
	}
	while (clientnum != ent->client->sess.spectatorClient);

	// leave it where it was
}

/**
 * @brief Try to follow the same client as last time (before getting killed)
 * @param[in] ent
 */
qboolean G_FollowSame(gentity_t *ent)
{
	if (ent->client->sess.spectatorClient < 0 || ent->client->sess.spectatorClient >= level.maxclients)
	{
		return qfalse;
	}

	// can only follow connected clients
	if (level.clients[ent->client->sess.spectatorClient].pers.connected != CON_CONNECTED)
	{
		return qfalse;
	}

	// can't follow another spectator
	if (level.clients[ent->client->sess.spectatorClient].sess.sessionTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	// couple extra checks for limbo mode
	if (ent->client->ps.pm_flags & PMF_LIMBO)
	{
		if (level.clients[ent->client->sess.spectatorClient].sess.sessionTeam != ent->client->sess.sessionTeam)
		{
			return qfalse;
		}
	}

	if (level.clients[ent->client->sess.spectatorClient].ps.pm_flags & PMF_LIMBO)
	{
		return qfalse;
	}

	if (!G_desiredFollow(ent, level.clients[ent->client->sess.spectatorClient].sess.sessionTeam))
	{
		return qfalse;
	}

	return qtrue;
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
 * @brief G_SayTo
 * @param[in] ent
 * @param[in] other
 * @param[in] mode
 * @param[in] color
 * @param[in] name
 * @param[in] message
 * @param[in] localize
 */
void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, qboolean localize)
{
	char cmd[6];

	if (!other || !other->inuse || !other->client)
	{
		return;
	}
	if ((mode == SAY_TEAM || mode == SAY_TEAMNL) && !OnSameTeam(ent, other))
	{
		return;
	}

	// if spectator, no chatting to players
	if (match_mutespecs.integer > 0 && ent->client->sess.referee == 0 &&
	    ((ent->client->sess.sessionTeam == TEAM_FREE && other->client->sess.sessionTeam != TEAM_FREE) ||
	     (ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR)))
	{
		return;
	}
	else
	{
		if (mode == SAY_BUDDY)      // send only to people who have the sender on their buddy list
		{
			if (ent->s.clientNum != other->s.clientNum)
			{
				fireteamData_t *ft1, *ft2;
				if (!G_IsOnFireteam(other - g_entities, &ft1))
				{
					return;
				}
				if (!G_IsOnFireteam(ent - g_entities, &ft2))
				{
					return;
				}
				if (ft1 != ft2)
				{
					return;
				}
			}
		}

		if (COM_BitCheck(other->client->sess.ignoreClients, (ent - g_entities)))
		{
			//Q_strncpyz(cmd, "print", sizeof(cmd));
		}
		else if (mode == SAY_TEAM || mode == SAY_BUDDY)
		{
			Q_strncpyz(cmd, "tchat", sizeof(cmd));

			trap_SendServerCommand((int)(other - g_entities),
			                       va("%s \"%c%c%s%s\" %i %i %i %i %i",
			                          cmd,
			                          Q_COLOR_ESCAPE, color, message,
			                          (!Q_stricmp(cmd, "print")) ? "\n" : "",
			                          (int)(ent - g_entities), localize,
			                          (int)ent->s.pos.trBase[0],
			                          (int)ent->s.pos.trBase[1],
			                          (int)ent->s.pos.trBase[2]));
		}
		else
		{
			Q_strncpyz(cmd, "chat", sizeof(cmd));

			trap_SendServerCommand((int)(other - g_entities),
			                       va("%s \"%s%c%c%s%s\" %i %i",
			                          cmd, name, Q_COLOR_ESCAPE, color,
			                          message,
			                          (!Q_stricmp(cmd, "print")) ? "\n" : "",
			                          (int)(ent - g_entities), localize));
		}

#ifdef FEATURE_OMNIBOT
		// Omni-bot: Tell the bot about the chat message
		Bot_Event_ChatMessage(other - g_entities, ent, mode, message);
#endif
	}
}

/**
 * @brief G_Say
 * @param[in] ent
 * @param[in] target
 * @param[in] mode
 * @param[in] chatText
 */
void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText)
{
	int       j;
	gentity_t *other;
	int       color;
	char      name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];

	switch (mode)
	{
	default:
	case SAY_ALL:
		G_LogPrintf("say: ^7%s^7: ^2%s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%c%c%s%c%c: %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_GREEN);
		color = COLOR_GREEN;
		break;
	case SAY_BUDDY:
		G_LogPrintf("saybuddy: ^7%s^7: ^3%s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "[lof]%c%c(%s%c%c): %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_YELLOW);
		color = COLOR_YELLOW;
		break;
	case SAY_TEAM:
		G_LogPrintf("sayteam: ^7%s^7: ^5%s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "[lof]%c%c(%s%c%c): %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_CYAN);
		color = COLOR_CYAN;
		break;
	case SAY_TEAMNL:
		G_LogPrintf("sayteamnl: ^7%s^7: ^2%s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%c%c(%s%c%c): %c%c", Q_COLOR_ESCAPE, COLOR_WHITE, ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, Q_COLOR_ESCAPE, COLOR_CYAN);
		color = COLOR_CYAN;
		break;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	if (target)
	{
		if (!COM_BitCheck(target->client->sess.ignoreClients, ent - g_entities))
		{
			G_SayTo(ent, target, mode, color, name, text, qfalse);
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
		other = &g_entities[level.sortedClients[j]];
		if (!COM_BitCheck(other->client->sess.ignoreClients, ent - g_entities))
		{
			G_SayTo(ent, other, mode, color, name, text, qfalse);
		}
	}
}

/**
 * @brief Cmd_Say_f
 * @param[in] ent
 * @param[in] mode
 * @param[in] arg0
 */
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0)
{
	if (trap_Argc() < 2 && !arg0)
	{
		return;
	}
	G_Say(ent, NULL, mode, ConcatArgs(((arg0) ? 0 : 1)));
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
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly, float randomNum)
{
	int  color;
	char *cmd;

	if (!other)
	{
		return;
	}
	if (!other->inuse)
	{
		return;
	}
	if (!other->client)
	{
		return;
	}

	if (mode == SAY_TEAM && !OnSameTeam(ent, other))
	{
		return;
	}

	// spec vchat rules follow the same as normal chatting rules
	if (match_mutespecs.integer > 0 && ent->client->sess.referee == 0 &&
	    ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		return;
	}

	// send only to people who have the sender on their buddy list
	if (mode == SAY_BUDDY)
	{
		if (ent->s.clientNum != other->s.clientNum)
		{
			fireteamData_t *ft1, *ft2;

			if (!G_IsOnFireteam(other - g_entities, &ft1))
			{
				return;
			}
			if (!G_IsOnFireteam(ent - g_entities, &ft2))
			{
				return;
			}
			if (ft1 != ft2)
			{
				return;
			}
		}
	}

	if (mode == SAY_TEAM)
	{
		color = COLOR_CYAN;
		cmd   = "vtchat";
	}
	else if (mode == SAY_BUDDY)
	{
		color = COLOR_YELLOW;
		cmd   = "vbchat";
	}
	else
	{
		color = COLOR_GREEN;
		cmd   = "vchat";
	}

	// if bots we don't send voices (no matter if omnibot or not)
	if (other->r.svFlags & SVF_BOT)
	{
#ifdef FEATURE_OMNIBOT
		// Omni-bot Send this voice macro to the bot as an event.
		Bot_Event_VoiceMacro(other - g_entities, ent, mode, id);
#endif
		return;
	}

	if (mode == SAY_TEAM || mode == SAY_BUDDY)
	{
		CPx(other - g_entities, va("%s %d %d %d %s %i %i %i %f", cmd, voiceonly, (int)(ent - g_entities), color, id, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2], (double)randomNum));
	}
	else
	{
		CPx(other - g_entities, va("%s %d %d %d %s %f", cmd, voiceonly, (int)(ent - g_entities), color, id, (double)randomNum));
	}
}

/**
 * @brief G_Voice
 * @param[in,out] ent
 * @param[in] target
 * @param[in] mode
 * @param[in] id
 * @param[in] voiceonly
 */
void G_Voice(gentity_t *ent, gentity_t *target, int mode, const char *id, const char *customChat, qboolean voiceonly)
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
		//if (g_customChat.integer && strlen(customChat) > 1 )
		if (strlen(customChat) > 1) // custom chat cvar control?
		{
			G_Say(ent, target, mode, customChat);
			G_VoiceTo(ent, target, mode, id, qtrue, randomNum);
		}
		else
		{
			G_VoiceTo(ent, target, mode, id, voiceonly, randomNum);
		}
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

		//if (g_customChat.integer && strlen(customChat) > 1)
		if (strlen(customChat) > 1) // custom chat cvar control?
		{
			G_Say(ent, NULL, mode, customChat);
			voiceonly = qtrue;
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

			G_VoiceTo(ent, victim, mode, id, voiceonly, randomNum);
		}
	}
	else
	{
		//if (g_customChat.integer && strlen(customChat) > 1)
		if (strlen(customChat) > 1) // custom chat cvar control?
		{
			G_Say(ent, NULL, mode, customChat);
			voiceonly = qtrue;
		}

		// send it to all the apropriate clients
		for (j = 0; j < level.numConnectedClients; j++)
		{
			victim = &g_entities[level.sortedClients[j]];
			if (COM_BitCheck(victim->client->sess.ignoreClients, (ent - g_entities)))
			{
				continue;
			}
			G_VoiceTo(ent, victim, mode, id, voiceonly, randomNum);
		}
	}
}

/**
 * @brief Cmd_Voice_f
 * @param[in] ent
 * @param[in] mode
 * @param[in] arg0
 * @param[in] voiceonly
 */
static void Cmd_Voice_f(gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly)
{
	char bufferIndexCustom[32];

	if (mode != SAY_BUDDY)
	{
		if (trap_Argc() < 2 && !arg0)
		{
			return;
		}

		trap_Argv((arg0) ? 0 : 1, bufferIndexCustom, sizeof(bufferIndexCustom));

		G_Voice(ent, NULL, mode, bufferIndexCustom, ConcatArgs(((arg0) ? 1 : 2)), voiceonly);
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

		G_Voice(ent, NULL, mode, bufferIndexCustom, ConcatArgs(((arg0) ? 3 + index : 4 + index)), voiceonly);
	}
}

/**
 * @brief Cmd_Where_f
 * @param[in] ent
 */
void Cmd_Where_f(gentity_t *ent)
{
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin)));
}

/**
 * @brief Cmd_CallVote_f
 * @param[in,out] ent
 * @param dwCommand - unused
 * @param[in] fRefCommand
 * @return
 */
qboolean Cmd_CallVote_f(gentity_t *ent, unsigned int dwCommand, qboolean fRefCommand)
{
	int  i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
	char voteDesc[VOTE_MAXSTRING];
	char *voteStringFormat;

	// Normal checks, if its not being issued as a referee command
	if (!fRefCommand)
	{
		// added mute check
		if (ent->client->sess.muted)
		{
			CP("cp \"You cannot call a vote while muted.\"");
			return qfalse;
		}
		else if (level.voteInfo.voteTime)
		{
			CP("cp \"A vote is already in progress.\"");
			return qfalse;
		}
		else if (level.intermissiontime)
		{
			CP("cp \"You cannot call a vote during intermission.\"");
			return qfalse;
		}
		else if (!ent->client->sess.referee)
		{
			if (voteFlags.integer == VOTING_DISABLED)
			{
				CP("cp \"Voting is disabled on this server.\"");
				return qfalse;
			}
			else if (vote_limit.integer > 0 && ent->client->pers.voteCount >= vote_limit.integer)
			{
				CP(va("cp \"You have already called the maximum number of votes (%d).\"", vote_limit.integer));
				return qfalse;
			}
			else if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			{
				CP("cp \"You cannot call a vote as a spectator.\"");
				return qfalse;
			}
		}
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));

	// quake3 engine callvote bug fix from Luigi Auriemma and/or /dev/humancontroller
	// http://bugzilla.icculus.org/show_bug.cgi?id=3593
	// also see http://aluigi.freeforums.org/quake3-engine-callvote-bug-t686-30.html
	if (strchr(arg1, ';') || strchr(arg2, ';') ||
	    strchr(arg1, '\r') || strchr(arg2, '\r') ||
	    strchr(arg1, '\n') || strchr(arg2, '\n'))
	{
		char *strCmdBase = (!fRefCommand) ? "vote" : "ref command";

		G_refPrintf(ent, "Invalid %s string", strCmdBase);
		return qfalse;
	}

	if (trap_Argc() > 1 && (i = G_voteCmdCheck(ent, arg1, arg2, fRefCommand)) != G_NOTFOUND)
	{
		if (i != G_OK) // only G_OK continues, G_INVALID & other are dropped here
		{
			// no output here
			return qfalse;                  // Command error
		}
	}
	else
	{
		if (!fRefCommand)
		{
			CP(va("print \"^3>>> Unknown vote command: ^7%s %s\n\"", arg1, arg2));
			G_voteHelp(ent, qtrue);
		}
		return qfalse;
	}

	voteStringFormat = arg2[0] ? "%s %s" : "%s";
	Com_sprintf(level.voteInfo.voteString, sizeof(level.voteInfo.voteString), voteStringFormat, arg1, arg2);

	// start the voting, the caller automatically votes yes
	// If a referee, vote automatically passes.
	if (fRefCommand)
	{
		// Don't announce some votes, as in comp mode, it is generally a ref
		// who is policing people who shouldn't be joining and players don't want
		// this sort of spam in the console
		if (level.voteInfo.vote_fn != G_Kick_v && level.voteInfo.vote_fn != G_Mute_v)
		{
			AP("cp \"^1** Referee Server Setting Change **\n\"");
		}

		// just call the stupid thing.... don't bother with the voting faff
		level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);

		G_globalSoundEnum(GAMESOUND_MISC_REFEREE);
	}
	else
	{
		// do not automatically vote yes in polls
		if (level.voteInfo.vote_fn != G_Poll_v)
		{
			level.voteInfo.voteYes = 1;
		}
		else
		{
			level.voteInfo.voteYes = 0;
		}
		AP(va("print \"[lof]%s^7 [lon]called a vote.[lof] Voting for: %s\n\"", ent->client->pers.netname, level.voteInfo.voteString));

		G_LogPrintf("callvote: %i %s\n", (int)(ent - g_entities), level.voteInfo.voteString);

		level.voteInfo.voteCaller = ent->s.number;
		level.voteInfo.voteTeam   = ent->client->sess.sessionTeam;
		AP(va("cp \"[lof]%s\n^7[lon]called a vote.\n\"", ent->client->pers.netname));
		G_globalSoundEnum(GAMESOUND_MISC_VOTE);
	}

	// shorter the vote timeout when warmup countdown or map time are going to end
	if (g_gamestate.integer == GS_WARMUP_COUNTDOWN && (level.warmupTime - level.time) < VOTE_TIME)
	{
		level.voteInfo.voteTime = level.warmupTime - VOTE_TIME;
	}
	else if (g_gamestate.integer == GS_PLAYING && (level.startTime + (g_timelimit.integer * 60000) - level.time < VOTE_TIME))
	{
		level.voteInfo.voteTime = level.startTime + (g_timelimit.integer * 60000) - VOTE_TIME;
	}
	else
	{
		level.voteInfo.voteTime = level.time;
	}
	level.voteInfo.voteNo = 0;

	// Don't send the vote info if a ref initiates (as it will automatically pass)
	if (!fRefCommand)
	{
		for (i = 0; i < level.numConnectedClients; i++)
		{
			level.clients[level.sortedClients[i]].ps.eFlags &= ~EF_VOTED;
		}

		ent->client->pers.voteCount++;

		// allow to vote...
		if (level.voteInfo.vote_fn != G_Poll_v)
		{
			ent->client->ps.eFlags |= EF_VOTED;
		}

		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
		Q_strncpyz(voteDesc, level.voteInfo.voteString, sizeof(voteDesc));
		if ((g_voting.integer & VOTEF_DISP_CALLER))
		{
			Q_strcat(voteDesc, sizeof(voteDesc), " (called by ");
			Q_strcat(voteDesc, sizeof(voteDesc), ent->client->pers.netname);
			Q_strcat(voteDesc, sizeof(voteDesc), ")");
		}
		trap_SetConfigstring(CS_VOTE_STRING, voteDesc);
		trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteInfo.voteTime));
	}

	return qtrue;
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
 * @brief Cmd_Vote_f
 * @param[in,out] ent
 */
void Cmd_Vote_f(gentity_t *ent)
{
	char msg[64];

	// Complaints supercede voting (and share command)
	if (ent->client->pers.complaintEndTime > level.time && g_gamestate.integer == GS_PLAYING && g_complaintlimit.integer)
	{
		gentity_t *other = &g_entities[ent->client->pers.complaintClient];
		gclient_t *cl    = other->client;

		if (!cl)
		{
			return;
		}
		if (cl->pers.connected != CON_CONNECTED)
		{
			return;
		}
		if (cl->pers.localClient)
		{
			trap_SendServerCommand(ent - g_entities, "complaint -3");
			return;
		}

		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			int num;

			// Increase their complaint counter
			cl->pers.complaints++;

			num = g_complaintlimit.integer - cl->pers.complaints;

			if (!cl->pers.localClient)
			{
				const char *value = level.clients[ent - g_entities].pers.client_ip;

				ipFilter_t ip;

				StringToFilter(value, &ip);

				if (num <= 0 || !G_FindFreeComplainIP(cl, &ip))
				{
					trap_DropClient(cl - level.clients, "kicked after too many complaints.", cl->sess.referee ? 0 : 300);
					trap_SendServerCommand(ent - g_entities, "complaint -1");
					return;
				}
			}

			trap_SendServerCommand(ent->client->pers.complaintClient, va("cpm \"^1Warning^7: Complaint filed against you by %s^7. You have lost XP.\n\"", ent->client->pers.netname));    // ^*
			trap_SendServerCommand(ent - g_entities, "complaint -1");

			G_LoseKillSkillPoints(other, ent->sound2to1, ent->sound1to2, ent->sound2to3 ? qtrue : qfalse);
		}
		else
		{
			trap_SendServerCommand(ent->client->pers.complaintClient, "cpm \"No complaint filed against you.\n\"");
			trap_SendServerCommand(ent - g_entities, "complaint -2");
		}

		// Reset this ent's complainEndTime so they can't send multiple complaints
		ent->client->pers.complaintEndTime = -1;
		ent->client->pers.complaintClient  = -1;

		return;
	}

	if (ent->client->pers.applicationEndTime > level.time)
	{
		gclient_t *cl = g_entities[ent->client->pers.applicationClient].client;

		if (!cl)
		{
			return;
		}
		if (cl->pers.connected != CON_CONNECTED)
		{
			return;
		}

		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "application -4");
			trap_SendServerCommand(ent->client->pers.applicationClient, "application -3");

			G_AddClientToFireteam(ent->client->pers.applicationClient, ent - g_entities);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "application -4");
			trap_SendServerCommand(ent->client->pers.applicationClient, "application -2");
		}

		ent->client->pers.applicationEndTime = 0;
		ent->client->pers.applicationClient  = -1;

		return;
	}

	ent->client->pers.applicationEndTime = 0;
	ent->client->pers.applicationClient  = -1;

	if (ent->client->pers.invitationEndTime > level.time)
	{
		gclient_t *cl = g_entities[ent->client->pers.invitationClient].client;

		if (!cl)
		{
			return;
		}
		if (cl->pers.connected != CON_CONNECTED)
		{
			return;
		}

		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "invitation -4");
			trap_SendServerCommand(ent->client->pers.invitationClient, "invitation -3");

			G_AddClientToFireteam(ent - g_entities, ent->client->pers.invitationClient);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "invitation -4");
			trap_SendServerCommand(ent->client->pers.invitationClient, "invitation -2");
		}

		ent->client->pers.invitationEndTime = 0;
		ent->client->pers.invitationClient  = -1;

		return;
	}

	ent->client->pers.invitationEndTime = 0;
	ent->client->pers.invitationClient  = -1;

	if (ent->client->pers.propositionEndTime > level.time)
	{
		gclient_t *cl = g_entities[ent->client->pers.propositionClient].client;

		if (!cl)
		{
			return;
		}
		if (cl->pers.connected != CON_CONNECTED)
		{
			return;
		}

		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "proposition -4");
			trap_SendServerCommand(ent->client->pers.propositionClient2, "proposition -3");

			G_InviteToFireTeam(ent - g_entities, ent->client->pers.propositionClient);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "proposition -4");
			trap_SendServerCommand(ent->client->pers.propositionClient2, "proposition -2");
		}

		ent->client->pers.propositionEndTime = 0;
		ent->client->pers.propositionClient  = -1;
		ent->client->pers.propositionClient2 = -1;

		return;
	}

	if (ent->client->pers.autofireteamEndTime > level.time)
	{
		fireteamData_t *ft;

		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "aft -2");

			if (G_IsFireteamLeader(ent - g_entities, &ft))
			{
				ft->priv = qtrue;
				G_UpdateFireteamConfigString(ft);
			}
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "aft -2");
		}

		ent->client->pers.autofireteamEndTime = 0;

		return;
	}

	if (ent->client->pers.autofireteamCreateEndTime > level.time)
	{
		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			trap_SendServerCommand(ent - g_entities, "aftc -2");

			G_RegisterFireteam(ent - g_entities);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "aftc -2");
		}

		ent->client->pers.autofireteamCreateEndTime = 0;

		return;
	}

	if (ent->client->pers.autofireteamJoinEndTime > level.time)
	{
		trap_Argv(1, msg, sizeof(msg));

		if (tolower(msg[0]) == 'y' || msg[0] == '1')
		{
			fireteamData_t *ft;

			trap_SendServerCommand(ent - g_entities, "aftj -2");


			ft = G_FindFreePublicFireteam(ent->client->sess.sessionTeam);
			if (ft)
			{
				G_AddClientToFireteam(ent - g_entities, ft->joinOrder[0]);
			}
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "aftj -2");
		}

		ent->client->pers.autofireteamCreateEndTime = 0;

		return;
	}

	ent->client->pers.propositionEndTime = 0;
	ent->client->pers.propositionClient  = -1;
	ent->client->pers.propositionClient2 = -1;

	// Reset this ent's complainEndTime so they can't send multiple complaints
	ent->client->pers.complaintEndTime = -1;
	ent->client->pers.complaintClient  = -1;

	if (!level.voteInfo.voteTime)
	{
		trap_SendServerCommand(ent - g_entities, "print \"No vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_VOTED)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Vote already cast.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	if (ent->client->sess.muted)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to vote when muted.\n\"");
		return;
	}

	if (level.voteInfo.vote_fn == G_Kick_v)
	{
		int pid = Q_atoi(level.voteInfo.vote_value);

		if (!g_entities[pid].client)
		{
			return;
		}

		if (g_entities[pid].client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->sess.sessionTeam != g_entities[pid].client->sess.sessionTeam)
		{
			trap_SendServerCommand(ent - g_entities, "print \"Cannot vote to kick player on opposing team.\n\"");
			return;
		}
	}
	else if (level.voteInfo.vote_fn == G_Surrender_v)
	{
		if (ent->client->sess.sessionTeam != level.voteInfo.voteTeam)
		{
			CP("cp \"You cannot vote on the other team's surrender.\"");
			return;
		}
	}

	trap_SendServerCommand(ent - g_entities, "print \"Vote cast.\n\"");

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv(1, msg, sizeof(msg));

	if (tolower(msg[0]) == 'y' || msg[0] == '1')
	{
		level.voteInfo.voteYes++;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
	}
	else
	{
		level.voteInfo.voteNo++;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
	}
	// a majority will be determined in G_CheckVote,
	// which will also account for players entering or leaving
}

/**
 * @brief Cmd_SetViewpos_f
 * @param[in] ent
 */
void Cmd_SetViewpos_f(gentity_t *ent)
{
	vec3_t origin, angles;
	char   buffer[MAX_TOKEN_CHARS];
	int    i;

	if (!g_cheats.integer)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}

	if (trap_Argc() == 5)
	{
		VectorClear(angles);
		for (i = 0; i < 3; i++)
		{
			trap_Argv(i + 1, buffer, sizeof(buffer));
			origin[i] = atof(buffer);
		}

		trap_Argv(4, buffer, sizeof(buffer));
		angles[YAW] = atof(buffer);
	}
	else if (trap_Argc() == 8)
	{
		for (i = 0; i < 3; i++)
		{
			trap_Argv(i + 1, buffer, sizeof(buffer));
			origin[i] = atof(buffer);
		}

		for (i = 0; i < 3; i++)
		{
			trap_Argv(i + 4, buffer, sizeof(buffer));
			angles[i] = atof(buffer);
		}

		trap_Argv(7, buffer, sizeof(buffer));
		qboolean useViewHeight = atof(buffer);
		origin[2] = useViewHeight ? origin[2] -= (ent->client->ps.viewheight + 1) : origin[2];  // + 1 to account for teleport event origin shift
	}
	else
	{
		trap_SendServerCommand(ent - g_entities, va("print \"usage: setviewpos x y z yaw\n       setviewpos x y z pitch yaw roll useViewHeight(1/0)\n\""));
		return;
	}

	TeleportPlayer(ent, origin, angles);
}

extern vec3_t playerMins;
extern vec3_t playerMaxs;

/**
 * @brief G_TankIsOccupied
 * @param[in] ent
 * @return
 */
qboolean G_TankIsOccupied(gentity_t *ent)
{
	if (!ent->tankLink)
	{
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief G_TankIsMountable
 * @param[in] ent
 * @param[in] other
 * @return
 */
qboolean G_TankIsMountable(gentity_t *ent, gentity_t *other)
{
	if (!(ent->spawnflags & 128))
	{
		return qfalse;
	}

	if (level.disableTankEnter)
	{
		return qfalse;
	}

	if (G_TankIsOccupied(ent))
	{
		return qfalse;
	}

	if (ent->health <= 0)
	{
		return qfalse;
	}

	if (other->client->ps.weaponDelay)
	{
		return qfalse;
	}

	if (GetWeaponTableData(other->client->ps.weapon)->type & WEAPON_TYPE_SET)
	{
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief Do_Activate2_f
 * @param[in,out] ent
 * @param[in,out] traceEnt
 * @return
 */
qboolean Do_Activate2_f(gentity_t *ent, gentity_t *traceEnt)
{
	// Check the class and health state of the player trying to steal the uniform.
	if (ent->client->sess.playerType == PC_COVERTOPS && ent->health > 0)
	{
		if (!ent->client->ps.powerups[PW_BLUEFLAG] && !ent->client->ps.powerups[PW_REDFLAG])
		{
			if (traceEnt->s.eType == ET_CORPSE)
			{
				if (BODY_TEAM(traceEnt) < 4 && BODY_TEAM(traceEnt) != ent->client->sess.sessionTeam)
				{
					if (BODY_VALUE(traceEnt) >= 250)
					{
						traceEnt->nextthink = traceEnt->timestamp + BODY_TIME;

						//BG_AnimScriptEvent( &ent->client->ps, ent->client->pers.character->animModelInfo, ANIM_ET_PICKUPGRENADE, qfalse, qtrue );
						//ent->client->ps.pm_flags |= PMF_TIME_LOCKPLAYER;
						//ent->client->ps.pm_time = 2100;

						ent->client->ps.powerups[PW_OPS_DISGUISED] = 1;
						ent->client->ps.powerups[PW_OPS_CLASS_1]   = BODY_CLASS(traceEnt) & 1;
						ent->client->ps.powerups[PW_OPS_CLASS_2]   = BODY_CLASS(traceEnt) & 2;
						ent->client->ps.powerups[PW_OPS_CLASS_3]   = BODY_CLASS(traceEnt) & 4;

						BODY_TEAM(traceEnt) += 4;
						traceEnt->activator  = ent;
						traceEnt->s.eFlags  |= EF_HEADSHOT; // remove hat

						traceEnt->s.time2 = 1;

						// sound effect
						G_AddEvent(ent, EV_DISGUISE_SOUND, 0);

						G_AddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f);
						G_DebugAddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f, "stealing uniform");

						ent->client->disguiseClientNum = traceEnt->s.clientNum;

						CPx(ent->s.number, va("cp \"Uniform of %s^7 has been stolen\" 1", g_entities[traceEnt->s.clientNum].client->pers.netname));

						ClientUserinfoChanged(ent->s.clientNum);
					}
					else
					{
						BODY_VALUE(traceEnt) += 5;
					}
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/**
 * @brief Extracted out the functionality of Cmd_Activate_f from finding the object to use
 * so we can force bots to use items, without worrying that they are looking EXACTLY at the target
 *
 * @param[in,out] ent
 * @param[in,out] traceEnt
 * @return
 */
qboolean Do_Activate_f(gentity_t *ent, gentity_t *traceEnt)
{
	// invisible entities can't be used
	if (traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION)
	{
		return qfalse;
	}

	if (!traceEnt->classname)
	{
		return qfalse;
	}

	traceEnt->flags &= ~FL_SOFTACTIVATE;    // FL_SOFTACTIVATE will be set if the user is holding 'walk' key

	if (traceEnt->s.eType == ET_ALARMBOX)
	{
		trace_t trace;

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			return qfalse;
		}

		Com_Memset(&trace, 0, sizeof(trace));

		if (traceEnt->use)
		{
			G_UseEntity(traceEnt, ent, 0);
		}
	}
	else if (traceEnt->s.eType == ET_ITEM)
	{
		trace_t trace;

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			return qfalse;
		}

		Com_Memset(&trace, 0, sizeof(trace));

		if (traceEnt->touch)
		{
			if (ent->client->pers.autoActivate == PICKUP_ACTIVATE)
			{
				ent->client->pers.autoActivate = PICKUP_FORCE;      // force pickup
			}
			traceEnt->active = qtrue;
			traceEnt->touch(traceEnt, ent, &trace);
		}
	}
	else if (traceEnt->s.eType == ET_CABINET_A || traceEnt->s.eType == ET_CABINET_H)
	{
		trace_t trace;

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			return qfalse;
		}

		Com_Memset(&trace, 0, sizeof(trace));

		// parent trigger hold touch
		if (traceEnt->parent && traceEnt->parent->touch)
		{
			if (ent->client->pers.autoActivate == PICKUP_ACTIVATE)
			{
				ent->client->pers.autoActivate = PICKUP_FORCE;          // force pickup
			}

			traceEnt->parent->touch(traceEnt->parent, ent, &trace);
		}
	}
	else if (traceEnt->s.eType == ET_MOVER && G_TankIsMountable(traceEnt, ent))
	{
		G_Script_ScriptEvent(traceEnt, "mg42", "mount");
		ent->tagParent = traceEnt->nextTrain;
		Q_strncpyz(ent->tagName, "tag_player", MAX_QPATH);
		ent->backupWeaponTime                   = ent->client->ps.weaponTime;
		ent->client->ps.weaponTime              = traceEnt->backupWeaponTime;
		ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

		ent->tankLink      = traceEnt;
		traceEnt->tankLink = ent;

		G_ProcessTagConnect(ent, qtrue);
	}
	else if (G_EmplacedGunIsMountable(traceEnt, ent))
	{
		vec3_t    point;
		vec3_t    forward;      //, offset, end;
		gclient_t *cl = &level.clients[ent->s.clientNum];

		AngleVectors(traceEnt->s.apos.trBase, forward, NULL, NULL);
		VectorMA(traceEnt->r.currentOrigin, -36, forward, point);
		point[2] = ent->r.currentOrigin[2];

		// Save initial position
		VectorCopy(point, ent->TargetAngles);

		// Zero out velocity
		VectorCopy(vec3_origin, ent->client->ps.velocity);
		VectorCopy(vec3_origin, ent->s.pos.trDelta);

		traceEnt->active     = qtrue;
		ent->active          = qtrue;
		traceEnt->r.ownerNum = ent->s.number;
		VectorCopy(traceEnt->s.angles, traceEnt->TargetAngles);
		traceEnt->s.otherEntityNum = ent->s.number;

		cl->pmext.harc = traceEnt->harc;
		cl->pmext.varc = traceEnt->varc;
		VectorCopy(traceEnt->s.angles, cl->pmext.centerangles);
		cl->pmext.centerangles[PITCH] = AngleNormalize180(cl->pmext.centerangles[PITCH]);
		cl->pmext.centerangles[YAW]   = AngleNormalize180(cl->pmext.centerangles[YAW]);
		cl->pmext.centerangles[ROLL]  = AngleNormalize180(cl->pmext.centerangles[ROLL]);

		ent->backupWeaponTime                   = ent->client->ps.weaponTime;
		ent->client->ps.weaponTime              = traceEnt->backupWeaponTime;
		ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

		G_UseTargets(traceEnt, ent);     // added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)
	}
	else if (((Q_stricmp(traceEnt->classname, "func_door") == 0) || (Q_stricmp(traceEnt->classname, "func_door_rotating") == 0)))
	{
		if ((ent->client->pers.cmd.buttons & BUTTON_WALKING) || (ent->client->ps.pm_flags & PMF_DUCKED))
		{
			traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
		}
		G_TryDoor(traceEnt, ent, ent);        // (door,other,activator)
	}
	else if ((Q_stricmp(traceEnt->classname, "team_WOLF_checkpoint") == 0))
	{
		if (traceEnt->count != ent->client->sess.sessionTeam)
		{
			traceEnt->health++;
		}
	}
	else if ((Q_stricmp(traceEnt->classname, "func_button") == 0) && (traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY) && traceEnt->active == qfalse)
	{
		Use_BinaryMover(traceEnt, ent, ent);
		traceEnt->active = qtrue;
	}
	else if (!Q_stricmp(traceEnt->classname, "func_invisible_user"))
	{
		if ((ent->client->pers.cmd.buttons & BUTTON_WALKING) || (ent->client->ps.pm_flags & PMF_DUCKED))
		{
			traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
		}
		G_UseEntity(traceEnt, ent, ent);
	}
	else if (!Q_stricmp(traceEnt->classname, "props_footlocker"))
	{
		G_UseEntity(traceEnt, ent, ent);
	}
	else
	{
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief G_LeaveTank
 * @param[in,out] ent
 * @param[in] position
 */
void G_LeaveTank(gentity_t *ent, qboolean position)
{
	gentity_t *tank = ent->tankLink;
	// found our tank (or whatever)

	if (!tank)
	{
		return;
	}

	if (position)
	{
		trace_t tr;
		vec3_t  axis[3];
		vec3_t  pos;

		AnglesToAxis(tank->s.angles, axis);

		VectorMA(ent->client->ps.origin, 128, axis[1], pos);
		trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

		if (tr.startsolid)
		{
			// try right
			VectorMA(ent->client->ps.origin, -128, axis[1], pos);
			trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

			if (tr.startsolid)
			{
				// try back
				VectorMA(ent->client->ps.origin, -224, axis[0], pos);
				trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

				if (tr.startsolid)
				{
					// try front
					VectorMA(ent->client->ps.origin, 224, axis[0], pos);
					trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

					if (tr.startsolid)
					{
						// give up
						return;
					}
				}
			}
		}

		VectorClear(ent->client->ps.velocity);   // dont want them to fly away ;D
		TeleportPlayer(ent, pos, ent->client->ps.viewangles);
	}

	tank->mg42weapHeat         = ent->client->ps.weapHeat[WP_DUMMY_MG42];
	tank->backupWeaponTime     = ent->client->ps.weaponTime;
	ent->client->ps.weaponTime = ent->backupWeaponTime;

	// Prevent player always mounting the last gun used, on tank maps
	G_RemoveConfigstringIndex(va("%i %i %s", ent->s.number, ent->tagParent->s.number, ent->tagName), CS_TAGCONNECTS, MAX_TAGCONNECTS);

	G_Script_ScriptEvent(tank, "mg42", "unmount");
	ent->tagParent          = NULL;
	*ent->tagName           = '\0';
	ent->s.eFlags          &= ~EF_MOUNTEDTANK;
	ent->client->ps.eFlags &= ~EF_MOUNTEDTANK;
	tank->s.powerups        = -1;

	tank->tankLink = NULL;
	ent->tankLink  = NULL;
}

/**
 * @brief Cmd_Activate_f
 * @param[in,out] ent
 */
void Cmd_Activate_f(gentity_t *ent)
{
	trace_t  tr;
	vec3_t   end;
	vec3_t   forward, right, up, offset;
	qboolean pass2 = qfalse;

	if (ent->health <= 0)
	{
		return;
	}

	if (GetWeaponTableData(ent->s.weapon)->type & WEAPON_TYPE_SET)
	{
		return;
	}

	if (ent->active)
	{
		if (ent->client->ps.persistant[PERS_HWEAPON_USE])
		{
			int i;

			// Restore original position if current position is bad
			trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, MASK_PLAYERSOLID);
			if (tr.startsolid)
			{
				VectorCopy(ent->TargetAngles, ent->client->ps.origin);
				VectorCopy(ent->TargetAngles, ent->r.currentOrigin);
				ent->r.contents = CONTENTS_CORPSE;      // this will correct itself in ClientEndFrame
			}

			ent->client->ps.eFlags &= ~EF_MG42_ACTIVE;          // unset flag
			ent->client->ps.eFlags &= ~EF_AAGUN_ACTIVE;

			ent->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			ent->active                                  = qfalse;

			for (i = 0; i < level.num_entities; i++)
			{
				if (g_entities[i].s.eType == ET_MG42_BARREL && g_entities[i].r.ownerNum == ent->s.number)
				{
					g_entities[i].mg42weapHeat     = ent->client->ps.weapHeat[WP_DUMMY_MG42];
					g_entities[i].backupWeaponTime = ent->client->ps.weaponTime;
					break;
				}
			}
			ent->client->ps.weaponTime = ent->backupWeaponTime;
		}
		else
		{
			ent->active = qfalse;
		}
		return;
	}

	if ((ent->client->ps.eFlags & EF_MOUNTEDTANK) && (ent->s.eFlags & EF_MOUNTEDTANK) && !level.disableTankExit)
	{
		G_LeaveTank(ent, qtrue);
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	VectorCopy(ent->client->ps.origin, offset);
	offset[2] += ent->client->ps.viewheight;

	// lean
	if (ent->client->ps.leanf != 0.f)
	{
		VectorMA(offset, ent->client->ps.leanf, right, offset);
	}

	VectorMA(offset, CH_MAX_DIST, forward, end);

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP));

	if ((tr.surfaceFlags & SURF_NOIMPACT) || tr.entityNum == ENTITYNUM_WORLD)
	{
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_TRIGGER));
		pass2 = qtrue;
	}

	while (!(tr.surfaceFlags & SURF_NOIMPACT) && !(tr.entityNum == ENTITYNUM_WORLD))
	{
		qboolean found = qfalse;

		if (VectorDistance(offset, tr.endpos) <= CH_ACTIVATE_DIST)
		{
			found = Do_Activate_f(ent, &g_entities[tr.entityNum]);
		}

		if (found || pass2)
		{
			break;
		}

		pass2 = qtrue;
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_TRIGGER));
	}
}

/**
 * @brief G_PushPlayer
 * @param[in,out] ent
 * @param[in,out] victim
 * @return
 */
qboolean G_PushPlayer(gentity_t *ent, gentity_t *victim)
{
	vec3_t dir, push;

	if (!g_shove.integer)
	{
		return qfalse;
	}

	// Both players need to be up and running like little bunnies they are.
	if (ent->health <= 0 || victim->health <= 0)
	{
		return qfalse;
	}

	if ((level.time - ent->client->pmext.shoveTime) < 500)
	{
		return qfalse;
	}

	// Prevent possible cheating, as well as annoying push after revive and spawning
	if (ent->client->ps.powerups[PW_INVULNERABLE])
	{
		return qfalse;
	}

	// if a player cannot move at this moment, don't allow him to get pushed..
	if (victim->client->ps.pm_flags & PMF_TIME_LOCKPLAYER)
	{
		return qfalse;
	}

	// Don't allow pushing when player is using mg
	if (victim->client->ps.persistant[PERS_HWEAPON_USE])
	{
		return qfalse;
	}

	ent->client->pmext.shoveTime = level.time;

	// push is our forward vector
	AngleVectors(ent->client->ps.viewangles, dir, NULL, NULL);
	VectorNormalizeFast(dir);

	// etpro velocity
	VectorScale(dir, g_shove.integer * 5, push);

	// no longer try to shove into ground
	if ((push[2] > Q_fabs(push[0])) &&
	    (push[2] > Q_fabs(push[1])))
	{
		// player is being boosted
		if (g_misc.integer & G_MISC_SHOVE_Z)
		{
			// like in etpro, shoving up gives a bit more than JUMP_VELOCITY
			push[2] = dir[2] * g_shove.integer * 5;
		}
		else
		{
			push[2] = 64;
		}
	}
	else
	{
		// give them a little hop
		push[2] = 64;
	}

	VectorAdd(victim->s.pos.trDelta, push, victim->s.pos.trDelta);
	VectorAdd(victim->client->ps.velocity, push,
	          victim->client->ps.velocity);

	// are we pushed?
	victim->client->pmext.shoved = qtrue;
	victim->client->pmext.pusher = ent - g_entities;

	G_AddEvent(victim, EV_SHOVE_SOUND, 0);

	victim->client->ps.pm_time   = 100;
	victim->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	return qtrue;
}

/**
 * @brief Cmd_Activate2_f
 * @param[in] ent
 */
void Cmd_Activate2_f(gentity_t *ent)
{
	trace_t  tr;
	vec3_t   end;
	vec3_t   forward, right, up, offset;
	qboolean pass2 = qfalse;

	if (ent->health <= 0)
	{
		return;
	}

	if (GetWeaponTableData(ent->s.weapon)->type & WEAPON_TYPE_SET)
	{
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePointForActivate(ent, forward, right, up, offset);
	VectorMA(offset, 96, forward, end);

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE));

	if ((tr.surfaceFlags & SURF_NOIMPACT) || tr.entityNum == ENTITYNUM_WORLD)
	{
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		pass2 = qtrue;
	}

	// look for a guy to push
#ifdef FEATURE_OMNIBOT
	if ((g_OmniBotFlags.integer & OBF_SHOVING) || !(ent->r.svFlags & SVF_BOT))
	{
#endif
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE));
		if (tr.entityNum >= 0)
		{
			gentity_t *traceEnt = &g_entities[tr.entityNum];

			if (traceEnt->client)
			{
				G_PushPlayer(ent, traceEnt);
				return;
			}
		}
#ifdef FEATURE_OMNIBOT
	}
#endif

	while (!(tr.surfaceFlags & SURF_NOIMPACT) && !(tr.entityNum == ENTITYNUM_WORLD))
	{
		qboolean found;

		found = Do_Activate2_f(ent, &g_entities[tr.entityNum]);

		if (found || pass2)
		{
			break;
		}

		pass2 = qtrue;
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
	}
}

/**
 * @brief SetPlayerSpawn
 * @param[in,out] ent
 * @param[in] spawn
 * @param[in] update
 */
void SetPlayerSpawn(gentity_t *ent, int spawn, qboolean update)
{
	int               resolvedSpawnPoint;
	int               targetSpawnPoint;
	spawnPointState_t *spawnPointState;
	spawnPointState_t *targetSpawnPointState;
	ent->client->sess.userSpawnPointValue = spawn;
	if (ent->client->sess.sessionTeam != TEAM_ALLIES && ent->client->sess.sessionTeam != TEAM_AXIS)
	{
		trap_SendServerCommand((int)(ent - g_entities), "print \"^3Warning! To select spawn points you should be in game.\n\"");
		return;
	}
	if (spawn < 0 || spawn > level.numSpawnPoints)
	{
		trap_SendServerCommand((int)(ent - g_entities), "print \"^3Warning! Spawn point is out of bounds. Selecting 'Auto Pick'.\n\"");
		trap_SendServerCommand((int)(ent - g_entities), "print \"         ^3Use '/listspawnpt' command to list available spawn points.\n\"");
		ent->client->sess.userSpawnPointValue = 0;
	}

	if (update)
	{
		G_UpdateSpawnPointStatePlayerCounts();
	}

	resolvedSpawnPoint = Com_Clamp(0, (level.numSpawnPoints - 1), ent->client->sess.resolvedSpawnPointIndex);
	targetSpawnPoint   = Com_Clamp(0, (level.numSpawnPoints - 1), (ent->client->sess.userSpawnPointValue - 1));
	spawnPointState    = &level.spawnPointStates[resolvedSpawnPoint];
	if (spawn > 0 && targetSpawnPoint != resolvedSpawnPoint)
	{
		targetSpawnPointState = &level.spawnPointStates[targetSpawnPoint];
		trap_SendServerCommand((int)(ent - g_entities), va("print \"^9Spawning at '^2%s^9', near the selected '^2%s^9'.\n\"", spawnPointState->description, targetSpawnPointState->description));
	}
	else
	{
		trap_SendServerCommand((int)(ent - g_entities), va("print \"^9Spawning at '^2%s^9'.\n\"", spawnPointState->description));
	}
}

/**
 * @brief Cmd_SetSpawnPoint_f
 * @param[in,out] ent
 */
void Cmd_SetSpawnPoint_f(gentity_t *ent)
{
	char              arg[MAX_TOKEN_CHARS];
	int               val, i;
	spawnPointState_t *spawnPointState;

	if (trap_Argc() != 2)
	{
		trap_SendServerCommand((int)(ent - g_entities), "print \"^3Warning! Spawn point number expected.\n\"");
		trap_SendServerCommand((int)(ent - g_entities), "print \"         ^3Use '/listspawnpt' command to list available spawn points.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	val = Q_atoi(arg);

	if (ent->client)
	{
		SetPlayerSpawn(ent, val, qtrue);
	}

	for (i = 0; i < level.numLimboCams; i++)
	{
		int targetSpawnPoint = g_entities[level.limboCams[i].targetEnt].count - CS_MULTI_SPAWNTARGETS;
		if (level.limboCams[i].spawn && (targetSpawnPoint + 1) == val)
		{
			spawnPointState = &level.spawnPointStates[targetSpawnPoint];
			// don't allow checking opposite team's spawn camp
			if (ent->client &&
			    ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			    ent->client->sess.sessionTeam != spawnPointState->team)
			{
				break;
			}
			VectorCopy(level.limboCams[i].origin, ent->s.origin2);
			ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
			trap_SendServerCommand((int)(ent - g_entities), va("portalcampos %i %i %i %i %i %i %i %i", val - 1, (int)level.limboCams[i].origin[0], (int)level.limboCams[i].origin[1], (int)level.limboCams[i].origin[2], (int)level.limboCams[i].angles[0], (int)level.limboCams[i].angles[1], (int)level.limboCams[i].angles[2], level.limboCams[i].hasEnt ? level.limboCams[i].targetEnt : -1));
			break;
		}
	}
}

void G_PrintAccuracyLog(gentity_t *ent);

/**
 * @brief Cmd_WeaponStat_f
 * @param[in] ent
 */
void Cmd_WeaponStat_f(gentity_t *ent)
{
	char             buffer[16];
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
 */
void Cmd_IntermissionWeaponStats_f(gentity_t *ent)
{
	char buffer[1024];
	int  i, clientNum;

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
 * @brief G_MakeReady
 * @param[in,out] ent
 */
void G_MakeReady(gentity_t *ent)
{
	ent->client->ps.eFlags |= EF_READY;
	ent->s.eFlags          |= EF_READY;
	ent->client->pers.ready = qtrue;
}

/**
 * @brief G_MakeUnready
 * @param[in,out] ent
 */
void G_MakeUnready(gentity_t *ent)
{
	ent->client->ps.eFlags &= ~EF_READY;
	ent->s.eFlags          &= ~EF_READY;
	ent->client->pers.ready = qfalse;
}

/**
 * @brief Cmd_IntermissionReady_f
 * @param[in,out] ent
 */
void Cmd_IntermissionReady_f(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (g_gametype.integer == GT_WOLF_MAPVOTE && g_gamestate.integer == GS_INTERMISSION)
	{
		trap_SendServerCommand(ent - g_entities, "print \"'imready' not allowed during intermission and gametype map voting!\n\"");
		return;
	}

	G_MakeReady(ent);
}

/**
 * @brief Cmd_IntermissionPlayerKillsDeaths_f
 * @param[in,out] ent
 */
void Cmd_IntermissionPlayerKillsDeaths_f(gentity_t *ent)
{
	char buffer[1024];
	int  i;

	if (!ent || !ent->client)
	{
		return;
	}

	Q_strncpyz(buffer, "impkd ", sizeof(buffer));
	for (i = 0; i < g_maxclients.integer; i++)
	{
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
 */
void Cmd_IntermissionPlayerTime_f(gentity_t *ent)
{
	char buffer[1024];
	int  i;

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

#ifdef FEATURE_RATING
/**
 * @brief Cmd_IntermissionSkillRating_f
 * @param[in] ent
 */
void Cmd_IntermissionSkillRating_f(gentity_t *ent)
{
	char      buffer[1024];
	int       i;
	gclient_t *cl;

	if (!ent || !ent->client)
	{
		return;
	}

	if (!g_skillRating.integer)
	{
		return;
	}

	Q_strncpyz(buffer, "imsr ", sizeof(buffer));
	for (i = 0; i < g_maxclients.integer; i++)
	{
		if (g_entities[i].inuse)
		{
			cl = &level.clients[i];
			Q_strcat(buffer, sizeof(buffer), va("%.3f %.3f ",
			                                    cl->sess.mu - 3 * cl->sess.sigma,
			                                    cl->sess.mu - 3 * cl->sess.sigma - (cl->sess.oldmu - 3 * cl->sess.oldsigma)));
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), "0 0 ");
		}
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}
#endif

#ifdef FEATURE_PRESTIGE
/**
 * @brief Cmd_IntermissionPrestige_f
 * @param[in] ent
 */
void Cmd_IntermissionPrestige_f(gentity_t *ent)
{
	char      buffer[1024];
	int       i;
	gclient_t *cl;

	if (!ent || !ent->client)
	{
		return;
	}

	if (!g_prestige.integer)
	{
		return;
	}

	Q_strncpyz(buffer, "impr ", sizeof(buffer));
	for (i = 0; i < g_maxclients.integer; i++)
	{
		if (g_entities[i].inuse)
		{
			cl = &level.clients[i];
			Q_strcat(buffer, sizeof(buffer), va("%i ", cl->sess.prestige));
		}
		else
		{
			Q_strcat(buffer, sizeof(buffer), "0 ");
		}
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

/**
 * @brief Cmd_IntermissionCollectPrestige_f
 * @param[in,out] ent
 */
void Cmd_IntermissionCollectPrestige_f(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return;
	}

	if (g_gametype.integer == GT_WOLF_CAMPAIGN || g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		trap_SendServerCommand(ent - g_entities, "print \"'imcollectpr' not allowed during current gametype!\n\"");
		return;
	}

	if (!g_prestige.integer)
	{
		return;
	}

	if (g_gamestate.integer != GS_INTERMISSION)
	{
		trap_SendServerCommand(ent - g_entities, "print \"'imcollectpr' only allowed during intermission!\n\"");
		return;
	}

	G_SetClientPrestige(ent->client, qfalse);
}
#endif

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
void Cmd_IntermissionWeaponAccuracies_f(gentity_t *ent)
{
	char buffer[1024];
	int  i;

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
 */
void Cmd_SelectedObjective_f(gentity_t *ent)
{
	int   i, val;
	char  buffer[16];
	vec_t dist, neardist = 0;
	int   nearest = -1;

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
 * @param[in] ent
 */
void Cmd_Ignore_f(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		trap_SendServerCommand(ent - g_entities, "print \"usage: Ignore <clientname>.\n\"");
		return;
	}

	cnum = G_refClientnumForName(ent, cmd); // prints not on server message

	if (cnum != MAX_CLIENTS)
	{
		COM_BitSet(ent->client->sess.ignoreClients, cnum);
		trap_SendServerCommand(ent - g_entities, va("print \"You are ignoring [lof]%s[lon]^7.\n\"", level.clients[cnum].pers.netname));
	}
}

/**
 * @brief Cmd_UnIgnore_f
 * @param[in] ent
 */
void Cmd_UnIgnore_f(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if (!*cmd)
	{
		trap_SendServerCommand(ent - g_entities, "print \"usage: Unignore <clientname>.\n\"");
		return;
	}

	cnum = G_refClientnumForName(ent, cmd); // prints not on server message

	if (cnum != MAX_CLIENTS)
	{
		COM_BitClear(ent->client->sess.ignoreClients, cnum);
		trap_SendServerCommand(ent - g_entities, va("print \"[lof]%s[lon]^7 is no longer ignored.\n\"", level.clients[cnum].pers.netname));
	}
}

#ifdef ETLEGACY_DEBUG
#ifdef FEATURE_OMNIBOT

/**
 * @brief Cmd_SwapPlacesWithBot_f
 * @param[in,out] ent
 * @param[in] botNum
 */
void Cmd_SwapPlacesWithBot_f(gentity_t *ent, int botNum)
{
	gentity_t          *botent = &g_entities[botNum];
	gclient_t          cl, *client = ent->client;
	clientPersistant_t saved;
	clientSession_t    sess;
	int                persistant[MAX_PERSISTANT];

	if (!botent->client)
	{
		return;
	}
	// if this bot is dead
	if (botent->health <= 0 && (botent->client->ps.pm_flags & PMF_LIMBO))
	{
		trap_SendServerCommand(ent - g_entities, "print \"Bot is in limbo mode, cannot swap places.\n\"");
		return;
	}

	if (client->sess.sessionTeam != botent->client->sess.sessionTeam)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Bot is on different team, cannot swap places.\n\"");
		return;
	}

	// copy the client information
	cl = *botent->client;

	G_DPrintf("Swapping places: %s in for %s\n", ent->client->pers.netname, botent->client->pers.netname);
	// kill the bot
	botent->flags                        &= ~FL_GODMODE;
	botent->client->ps.stats[STAT_HEALTH] = botent->health = 0;
	player_die(botent, ent, ent, 100000, MOD_SWAP_PLACES);
	// make sure they go into limbo mode right away, and dont show a corpse
	limbo(botent, qfalse);
	// respawn the player
	ent->client->ps.pm_flags &= ~PMF_LIMBO; // turns off limbo
	// copy the location
	VectorCopy(cl.ps.origin, ent->s.origin);
	VectorCopy(cl.ps.viewangles, ent->s.angles);
	// copy session data, so we spawn in as the same class
	// save items
	saved = client->pers;
	sess  = client->sess;
	Com_Memcpy(persistant, ent->client->ps.persistant, sizeof(persistant));
	// give them the right weapons/etc
	*client                    = cl;
	client->sess               = sess;
	client->sess.playerType    = ent->client->sess.latchPlayerType = cl.sess.playerType;
	client->sess.playerWeapon  = ent->client->sess.latchPlayerWeapon = cl.sess.playerWeapon;
	client->sess.playerWeapon2 = ent->client->sess.latchPlayerWeapon2 = cl.sess.playerWeapon2;
	// spawn them in
	ClientSpawn(ent, qfalse, qtrue, qtrue);
	// restore items
	client->pers = saved;
	Com_Memcpy(ent->client->ps.persistant, persistant, sizeof(persistant));
	client->ps           = cl.ps;
	client->ps.clientNum = ent->s.number;
	ent->health          = client->ps.stats[STAT_HEALTH];
	SetClientViewAngle(ent, cl.ps.viewangles);
	// make sure they dont respawn immediately after they die
	client->pers.lastReinforceTime = 0;
}
#endif  // FEATURE_OMNIBOT
#endif  // ETLEGACY_DEBUG

/**
 * @brief ClientCommand
 * @param[in] clientNum
 */
void ClientCommand(int clientNum)
{
	gentity_t *ent = g_entities + clientNum;
	char      cmd[MAX_TOKEN_CHARS];

	if (!ent->client)
	{
		return;     // not fully in game yet
	}

	trap_Argv(0, cmd, sizeof(cmd));

#ifdef FEATURE_LUA
	// LUA API callbacks
	if (G_LuaHook_ClientCommand(clientNum, cmd))
	{
		return;
	}

	if (Q_stricmp(cmd, "lua_status") == 0)
	{
		G_LuaStatus(ent);
		return;
	}
#endif

	if (Q_stricmp(cmd, "say") == 0)
	{
		if (!ent->client->sess.muted)
		{
			Cmd_Say_f(ent, SAY_ALL, qfalse);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't chat - you are muted\n\"");
		}
		return;
	}
	else if (Q_stricmp(cmd, "say_team") == 0)
	{
		if (!ent->client->sess.muted)
		{
			Cmd_Say_f(ent, SAY_TEAM, qfalse);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't team chat - you are muted\n\"");
		}
		return;
	}
	else if (Q_stricmp(cmd, "vsay") == 0)
	{
		if (!ent->client->sess.muted)
		{
			Cmd_Voice_f(ent, SAY_ALL, qfalse, qfalse);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't chat - you are muted\n\"");
		}
		return;
	}
	else if (Q_stricmp(cmd, "vsay_team") == 0)
	{
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't team chat as spectator\n\"");
			return;
		}

		if (!ent->client->sess.muted)
		{
			Cmd_Voice_f(ent, SAY_TEAM, qfalse, qfalse);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't team chat - you are muted\n\"");
		}
		return;
	}
	else if (Q_stricmp(cmd, "say_buddy") == 0)
	{
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't buddy chat as spectator\n\"");
			return;
		}

		if (!ent->client->sess.muted)
		{
			Cmd_Say_f(ent, SAY_BUDDY, qfalse);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't buddy chat - you are muted\n\"");
		}
		return;
	}
	else if (Q_stricmp(cmd, "vsay_buddy") == 0)
	{
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't buddy chat as spectator\n\"");
			return;
		}

		if (!ent->client->sess.muted)
		{
			Cmd_Voice_f(ent, SAY_BUDDY, qfalse, qfalse);
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Can't buddy chat - you are muted\n\"");
		}
		return;
	}
	else if (Q_stricmp(cmd, "score") == 0)
	{
		Cmd_Score_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "vote") == 0)
	{
		Cmd_Vote_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "fireteam") == 0)
	{
		Cmd_FireTeam_MP_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "showstats") == 0)
	{
		G_PrintAccuracyLog(ent);
		return;
	}
	else if (Q_stricmp(cmd, "rconAuth") == 0)
	{
		Cmd_AuthRcon_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "ignore") == 0)
	{
		Cmd_Ignore_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "unignore") == 0)
	{
		Cmd_UnIgnore_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "obj") == 0)
	{
		Cmd_SelectedObjective_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "impkd"))
	{
		Cmd_IntermissionPlayerKillsDeaths_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "impt"))
	{
		Cmd_IntermissionPlayerTime_f(ent);
		return;
	}
#ifdef FEATURE_RATING
	else if (!Q_stricmp(cmd, "imsr"))
	{
		Cmd_IntermissionSkillRating_f(ent);
		return;
	}
#endif
#ifdef FEATURE_PRESTIGE
	else if (!Q_stricmp(cmd, "impr"))
	{
		Cmd_IntermissionPrestige_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "imcollectpr"))
	{
		Cmd_IntermissionCollectPrestige_f(ent);
		return;
	}
#endif
	else if (!Q_stricmp(cmd, "imwa"))
	{
		Cmd_IntermissionWeaponAccuracies_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "imws"))
	{
		Cmd_IntermissionWeaponStats_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "imready"))
	{
		Cmd_IntermissionReady_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "ws") == 0)
	{
		Cmd_WeaponStat_f(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "forcetapout"))
	{
		if (ent->client->ps.stats[STAT_HEALTH] <= 0 && (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES))
		{
			limbo(ent, qtrue);
		}

		return;
	}
	else if (!Q_stricmp(cmd, "mapvote")) // MAPVOTE
	{
		G_IntermissionMapVote(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "immaplist")) // MAPVOTE
	{
		G_IntermissionMapList(ent);
		return;
	}
	else if (!Q_stricmp(cmd, "imvotetally")) // MAPVOTE
	{
		G_IntermissionVoteTally(ent);
		return;
	}

	// Do these outside as we don't want to advertise it in the help screen
	if (!Q_stricmp(cmd, "wstats"))
	{
		G_statsPrint(ent, 1);
		return;
	}
	if (!Q_stricmp(cmd, "sgstats"))       // Player game stats
	{
		G_statsPrint(ent, 2);
		return;
	}
	if (!Q_stricmp(cmd, "stshots"))       // "Topshots" accuracy rankings
	{
		G_weaponStatsLeaders_cmd(ent, qtrue, qtrue);
		return;
	}
	if (!Q_stricmp(cmd, "rs"))
	{
		Cmd_ResetSetup_f(ent);
		return;
	}

	if (G_commandCheck(ent, cmd, qtrue))
	{
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		CPx(clientNum, va("print \"^3%s^7 not allowed during intermission.\n\"", cmd));
		return;
	}

	if (Q_stricmp(cmd, "give") == 0)
	{
		Cmd_Give_f(ent);
	}
	else if (Q_stricmp(cmd, "god") == 0)
	{
		Cmd_God_f(ent);
	}
	else if (Q_stricmp(cmd, "nofatigue") == 0)
	{
		Cmd_Nofatigue_f(ent);
	}
	else if (Q_stricmp(cmd, "notarget") == 0)
	{
		Cmd_Notarget_f(ent);
	}
	else if (Q_stricmp(cmd, "noclip") == 0)
	{
		Cmd_Noclip_f(ent);
	}
	else if (Q_stricmp(cmd, "nostamina") == 0)
	{
		Cmd_Nostamina_f(ent);
	}
	else if (Q_stricmp(cmd, "kill") == 0)
	{
		Cmd_Kill_f(ent);
	}
	else if (Q_stricmp(cmd, "dropobj") == 0)
	{
		Cmd_DropObjective_f(ent);
	}
	else if (Q_stricmp(cmd, "follownext") == 0)
	{
		Cmd_FollowCycle_f(ent, 1, qfalse);
	}
	else if (Q_stricmp(cmd, "followprev") == 0)
	{
		Cmd_FollowCycle_f(ent, -1, qfalse);
	}
	else if (Q_stricmp(cmd, "where") == 0)
	{
		Cmd_Where_f(ent);
	}
	else if (Q_stricmp(cmd, "setviewpos") == 0)
	{
		Cmd_SetViewpos_f(ent);
	}
	else if (Q_stricmp(cmd, "setspawnpt") == 0)
	{
		Cmd_SetSpawnPoint_f(ent);
	}
	else if (G_commandCheck(ent, cmd, qfalse))
	{
		return;
	}
	else
	{
		trap_SendServerCommand(clientNum, va("print \"unknown cmd[lof] %s\n\"", cmd));
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
