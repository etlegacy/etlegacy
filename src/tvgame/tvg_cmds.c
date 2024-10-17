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
 * @file tvg_cmds.c
 */

#include "tvg_local.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

const char *aTeams[TEAM_NUM_TEAMS] = { "FFA", "^1Axis^7", "^$Allies^7", "^2Spectators^7" };
team_info  teamInfo[TEAM_NUM_TEAMS];

#define MAX_MATCHES 10 + 1

/**
* @brief G_MatchOnePlayer
* @param[in] plist
* @param[out] err
* @param[in] len
* @return
*/
static qboolean TVG_MatchOnePlayer(int *plist, char *err, size_t len)
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

			if (found == MAX_MATCHES - 1)
			{
				break;
			}
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
	int pids[MAX_MATCHES];

	// no match or more than 1 player matchs, out error
	if (TVG_ClientNumbersFromString(s, pids) != 1)
	{
		char err[MAX_STRING_CHARS];

		TVG_MatchOnePlayer(pids, err, sizeof(err));

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

/**
* @brief TVG_MatchOnePlayerFromMaster
* @param[in] plist
* @param[out] err
* @param[in] len
* @return
*/
static qboolean TVG_MatchOnePlayerFromMaster(int *plist, char *err, size_t len)
{
	char cs[MAX_STRING_CHARS];

	err[0] = '\0';

	if (plist[0] == -1)
	{
		Q_strcat(err, len, "no connected player by that name or slot #");
		return qfalse;
	}

	if (plist[1] != -1)
	{
		char line[MAX_NAME_LENGTH + 10];
		int  *p;

		line[0] = '\0';

		Q_strcat(err, len, "more than one player name matches be more specific or use the slot #:\n");
		for (p = plist; *p != -1; p++)
		{
			trap_GetConfigstring(CS_PLAYERS + level.validMasterClients[*p], cs, sizeof(cs));
			Com_sprintf(line, MAX_NAME_LENGTH + 10, "%2i - %s^7\n", *p, Info_ValueForKey(cs, "n"));
			if (strlen(err) + strlen(line) > len)
			{
				break;
			}
			Q_strcat(err, len, line);
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
int TVG_MasterClientNumbersFromString(char *s, int *plist)
{
	int  i, found = 0;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];
	char *m;
	char cs[MAX_STRING_CHARS];

	*plist = -1;

	// if a number is provided, it might be a slot #
	if (Q_isanumber(s))
	{
		i = Q_atoi(s);

		if (i >= 0 && i < MAX_CLIENTS)
		{
			if (level.ettvMasterClients[i].valid)
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

	for (i = 0; i < level.numValidMasterClients; ++i)
	{
		trap_GetConfigstring(CS_PLAYERS + level.validMasterClients[i], cs, sizeof(cs));

		Q_strncpyz(n2, Info_ValueForKey(cs, "n"), sizeof(n2));
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
int TVG_MasterClientNumberFromString(gclient_t *to, char *s)
{
	int pids[MAX_CLIENTS];

	// no match or more than 1 player matchs, out error
	if (TVG_MasterClientNumbersFromString(s, pids) != 1)
	{
		char err[MAX_STRING_CHARS];

		TVG_MatchOnePlayerFromMaster(pids, err, sizeof(err));

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

/**
 * @brief TVG_PlaySound_Cmd
 */
void TVG_PlaySound_Cmd(void)
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
		gclient_t *victim;

		cnum = TVG_ClientNumberFromString(NULL, name);

		if (cnum == -1)
		{
			return;
		}

		victim = &level.clients[cnum];

		if (!Q_stricmp(cmd, "playsound_env"))
		{
			TVG_AddEvent(victim, EV_GENERAL_SOUND, TVG_SoundIndex(sound));
		}
		else
		{
			G_Printf("no sound found/played\n");
		}
	}
	else
	{
		G_Printf("no sound found/played\n");
	}
}

/**
* @brief TVG_CommandsAutoUpdate update stats
* @param[in] tvcmd
*/
qboolean TVG_CommandsAutoUpdate(tvcmd_reference_t *tvcmd)
{
	if (tvcmd->lastUpdateTime + tvcmd->updateInterval <= level.time)
	{
		trap_SendServerCommand(-2, tvcmd->pszCommandName);
		tvcmd->lastUpdateTime = level.time;
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Sends current scoreboard information
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_Score_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_CommandsAutoUpdate(self);
	}

	if (level.cmds.sraValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.sra);
	}

	if (level.cmds.prValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.pr);
	}

	trap_SendServerCommand(client - level.clients, level.cmds.score[0]);
	if (level.cmds.scoreHasTwoParts)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.score[1]);
	}

	return qtrue;
}

/**
 * @brief TVG_CheatsOk
 * @param[in] client
 * @return
 */
qboolean TVG_CheatsOk(gclient_t *client)
{
	if (!tvg_cheats.integer)
	{
		trap_SendServerCommand(client - level.clients, va("print \"Cheats are not enabled on this server.\n\""));
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
 * @brief TVG_Cmd_Noclip_f
 * @param[in] client
 * @param[in] self
 *
 * @note argv(0) noclip
 */
qboolean TVG_Cmd_Noclip_f(gclient_t *client, tvcmd_reference_t *self)
{
	char *msg;
	char *name;

	name = ConcatArgs(1);

	if (!TVG_CheatsOk(client))
	{
		return qtrue;
	}

	if (!Q_stricmp(name, "on") || Q_atoi(name))
	{
		client->noclip = qtrue;
	}
	else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0"))
	{
		client->noclip = qfalse;
	}
	else
	{
		client->noclip = !client->noclip;
	}

	if (client->noclip)
	{
		msg = "noclip ON\n";
	}
	else
	{
		msg = "noclip OFF\n";
	}

	trap_SendServerCommand(client - level.clients, va("print \"%s\"", msg));

	return qtrue;
}

/**
 * @brief If the client being followed leaves the game, or you just want to drop
 * to free floating spectator mode.
 *
 * @param[in,out] client
 */
void TVG_StopFollowing(gclient_t *client)
{
	// drop to free floating, somewhere above the current position (that's the client you were following)
	vec3_t pos, angle;

	client->sess.spectatorState  = SPECTATOR_FREE;
	client->sess.spectatorClient = 0;

	VectorCopy(client->ps.origin, pos);
	VectorCopy(client->ps.viewangles, angle);

	TVG_ClientBegin(client - level.clients);

	VectorCopy(pos, client->ps.origin);
	TVG_SetClientViewAngle(client, angle);
}

/**
 * @brief "Topshots" accuracy rankings
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_WeaponStatsLeaders_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_CommandsAutoUpdate(self);
	}

	TVG_weaponStatsLeaders_cmd(client, qtrue, qtrue);

	return qtrue;
}

/**
* @brief Sends a player's stats to the requesting client.
* @param[in] client
* @param[in] nType
*/
void TVG_statsPrint(gclient_t *client, int nType, int updateInterval)
{
	int        pid;
	char       arg[MAX_TOKEN_CHARS];
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
	if (level.cmds.infoStats[nType].lastUpdateTime[pid] + updateInterval <= level.time)
	{
		level.cmds.infoStats[nType].valid[pid]          = qfalse;
		level.cmds.infoStats[nType].lastUpdateTime[pid] = level.time;

		trap_SendServerCommand(-2, va("%s %d\n", cmd, pid));
	}
}

/**
 * @brief TVG_Cmd_wStats_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_wStats_f(gclient_t *ent, tvcmd_reference_t *self)
{
	TVG_statsPrint(ent, self->value, self->updateInterval);

	return qtrue;
}

/**
 * @brief Player game stats
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_sgStats_f(gclient_t *client, tvcmd_reference_t *self)
{
	TVG_statsPrint(client, self->value, self->updateInterval);

	return qtrue;
}

/**
 * @brief Cmd_Follow_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_Follow_f(gclient_t *client, tvcmd_reference_t *self)
{
	int  cnum;
	char arg[MAX_TOKEN_CHARS];

	if (trap_Argc() != 2)
	{
		if (client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			TVG_StopFollowing(client);
		}
		return qtrue;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (!Q_stricmp(arg, "allies") || !Q_stricmp(arg, "axis"))
	{
		team_t team;
		team = (!Q_stricmp(arg, "allies") ? TEAM_ALLIES : TEAM_AXIS);

		if (!TVG_TeamCount(client - level.clients, team))
		{
			CP(va("print \"The %s team %s empty!  Follow command ignored.\n\"", aTeams[team],
			      ((client->sess.sessionTeam != team) ? "is" : "would be")));
			return qtrue;
		}

		// Allow for simple toggle
		if (client->sess.spec_team != team)
		{
			client->sess.spec_team = team;
			CP(va("print \"Spectator follow is now locked on the %s team.\n\"", aTeams[team]));
			TVG_Cmd_FollowCycle_f(client, 1, qfalse);
		}
		else
		{
			client->sess.spec_team = 0;
			CP(va("print \"%s team spectating is now disabled.\n\"", aTeams[team]));
		}

		return qtrue;
	}

	cnum = TVG_MasterClientNumberFromString(client, arg);

	if (cnum == -1)
	{
		return qtrue;
	}

	if (level.ettvMasterClients[cnum].ps.pm_flags & PMF_LIMBO)
	{
		return qtrue;
	}

	client->sess.spectatorState  = SPECTATOR_FOLLOW;
	client->sess.spectatorClient = cnum;

	return qtrue;
}

/**
* @brief figure out if we are allowed/want to follow a given player
* @param[in] client
* @param[in] nTeam
* @return
*/
static qboolean TVG_desiredFollow(gclient_t *client, int nTeam)
{
	if (client->sess.spec_team == 0 || client->sess.spec_team == nTeam)
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief TVG_Cmd_FollowCycle_f
 * @param[in,out] ent
 * @param[in] dir
 * @param[in] skipBots
 */
void TVG_Cmd_FollowCycle_f(gclient_t *client, int dir, qboolean skipBots)
{
	int clientnum;

	if (dir != 1 && dir != -1)
	{
		G_Error("TVG_Cmd_FollowCycle_f: bad dir %i\n", dir);
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

		if (level.ettvMasterClients[clientnum].ps.pm_flags & PMF_LIMBO)
		{
			continue;
		}

		if (!TVG_desiredFollow(client, level.ettvMasterClients[clientnum].ps.teamNum))
		{
			continue;
		}

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
 * @brief TVG_Cmd_FollowNext_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_FollowNext_f(gclient_t *client, tvcmd_reference_t *self)
{
	TVG_Cmd_FollowCycle_f(client, 1, qfalse);

	return qtrue;
}

/**
 * @brief TVG_Cmd_FollowPrevious_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_FollowPrevious_f(gclient_t *client, tvcmd_reference_t *self)
{
	TVG_Cmd_FollowCycle_f(client, -1, qfalse);

	return qtrue;
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

	Q_strncpyz(cmd, "chat", sizeof(cmd));

	trap_SendServerCommand((int)(other - level.clients),
	                       va("%s \"%c%cTV%c%c: %s%c%c%s%s\" %i %i",
	                          cmd, Q_COLOR_ESCAPE, COLOR_RED, Q_COLOR_ESCAPE, COLOR_WHITE,
	                          name, Q_COLOR_ESCAPE, color, message,
	                          (!Q_stricmp(cmd, "print")) ? "\n" : "",
	                          (int)(client - level.clients), localize));
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
		G_LogPrintf("say: ^1TV^7:%s^7: ^2%s\n", client->pers.netname, chatText);
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
		//if (!COM_BitCheck(target->sess.ignoreClients, client - level.clients))
		{
			TVG_SayTo(client, target, mode, color, name, text, qfalse);
		}
		return;
	}

	// echo the text to the console
	if (tvg_dedicated.integer)
	{
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.numConnectedClients; j++)
	{
		other = &level.clients[level.sortedClients[j]];
		if (/*!COM_BitCheck(other->sess.ignoreClients, client - level.clients) && */ other->sess.tvchat)
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
 * @brief TVG_Cmd_SetViewpos_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_SetViewpos_f(gclient_t *client, tvcmd_reference_t *self)
{
	vec3_t origin, angles;
	char   buffer[MAX_TOKEN_CHARS];
	int    i;

	if (!TVG_CheatsOk(client))
	{
		return qtrue;
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
		qboolean useViewHeight;

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
		useViewHeight = Q_atof(buffer);

		if (useViewHeight)
		{
			origin[2] -= (client->ps.viewheight + 1);  // + 1 to account for teleport event origin shift
		}
	}
	else
	{
		trap_SendServerCommand(client - level.clients, va("print \"usage: setviewpos x y z yaw\n       setviewpos x y z pitch yaw roll useViewHeight(1/0)\n\""));
		return qtrue;
	}

	TVG_TeleportPlayer(client, origin, angles);

	return qtrue;
}

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
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_WeaponStat_f(gclient_t *client, tvcmd_reference_t *self)
{
	return qtrue;
}

/**
* @brief TVG_IntermissionStatsRequest should the update be requested
* @param[in] tvcmd
*/
static qboolean TVG_IntermissionStatsUpdate(tvcmd_reference_t *tvcmd)
{
	if (tvcmd->lastUpdateTime)
	{
		return qfalse;
	}

	trap_SendServerCommand(-2, tvcmd->pszCommandName);
	tvcmd->lastUpdateTime = level.time;

	return qtrue;
}

/**
* @brief TVG_IntermissionMapList
* @param[in] client
* @param[in] self
*/
qboolean TVG_IntermissionMapList(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.immaplistValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.immaplist);
	}

	return qtrue;
}

/**
* @brief TVG_IntermissionMapHistory
* @param[in] client
* @param[in] self
*/
qboolean TVG_IntermissionMapHistory(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.immaphistoryValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.immaphistory);
	}

	return qtrue;
}

/**
* @brief TVG_IntermissionVoteTally
* @param[in] client
* @param[in] self
*/
qboolean TVG_IntermissionVoteTally(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.imvotetallyValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.imvotetally);
	}

	return qtrue;
}

/**
 * @brief TVG_Cmd_IntermissionWeaponStats_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_IntermissionWeaponStats_f(gclient_t *client, tvcmd_reference_t *self)
{
	char buffer[MAX_TOKEN_CHARS];
	int  clientNum;

	if (!client)
	{
		if (self->lastUpdateTime)
		{
			return qtrue;
		}

		// waiting for response for previous command
		if (level.cmds.waitingForIMWS)
		{
			return qtrue;
		}

		if (self->value < 0 || self->value >= level.numValidMasterClients)
		{
			self->lastUpdateTime = level.time; // stop updating
			return qtrue;
		}

		clientNum = level.validMasterClients[self->value++];

		// shouldn't happen
		if (level.cmds.infoStats[INFO_IMWS].valid[clientNum])
		{
			return qfalse;
		}

		level.cmds.waitingForIMWS = qtrue;
		level.cmds.IMWSClientNum  = clientNum;
		trap_SendServerCommand(-2, va("imws %d", clientNum));

		return qtrue;
	}

	trap_Argv(1, buffer, sizeof(buffer));

	clientNum = Q_atoi(buffer);

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return qtrue;
	}

	if (level.cmds.infoStats[INFO_IMWS].valid[clientNum])
	{
		trap_SendServerCommand(client - level.clients, level.cmds.infoStats[INFO_IMWS].data[clientNum]);
	}

	return qtrue;
}

/**
 * @brief TVG_Cmd_IntermissionPlayerKillsDeaths_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_IntermissionPlayerKillsDeaths_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.impkdValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.impkd[0]);

		if (level.mod & LEGACY)
		{
			trap_SendServerCommand(client - level.clients, level.cmds.impkd[1]);
		}
	}

	return qtrue;
}

/**
* @brief TVG_Cmd_IntermissionPrestige_f
* @param[in] client
* @param[in] self
*/
qboolean TVG_Cmd_IntermissionPrestige_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.imprValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.impr);
	}

	return qtrue;
}

/**
 * @brief TVG_Cmd_IntermissionPlayerTime_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_IntermissionPlayerTime_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.imptValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.impt);
	}

	return qtrue;
}

/**
* @brief TVG_Cmd_IntermissionSkillRating_f
* @param[in] client
* @param[in] self
*/
qboolean TVG_Cmd_IntermissionSkillRating_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.imsrValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.imsr);
	}

	return qtrue;
}

/**
 * @brief TVG_Cmd_IntermissionWeaponAccuracies_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_IntermissionWeaponAccuracies_f(gclient_t *client, tvcmd_reference_t *self)
{
	if (!client)
	{
		return TVG_IntermissionStatsUpdate(self);
	}

	if (level.cmds.imwaValid)
	{
		trap_SendServerCommand(client - level.clients, level.cmds.imwa);
	}

	return qtrue;
}

/**
* @brief TVG_Cmd_CallVote_f
* @param[in] client
* @param[in] self
* @return
*/
qboolean TVG_Cmd_CallVote_f(gclient_t *client, tvcmd_reference_t *self)
{
	trap_SendServerCommand(client - level.clients, "print \"Callvote is disabled on this server.\"");
	return qfalse;
}

/**
* @brief TVG_Cmd_SelectedObjective_f
* @param[in] client
* @param[in] self
*/
qboolean TVG_Cmd_SelectedObjective_f(gclient_t *ent, tvcmd_reference_t *self)
{
	return qtrue;
}

/**
 * @brief TVG_Cmd_Ignore_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_Ignore_f(gclient_t *client, tvcmd_reference_t *self)
{
	//char cmd[MAX_TOKEN_CHARS], name[MAX_NAME_LENGTH];
	//int cnum;

	//trap_Argv(0, cmd, sizeof(cmd));
	//trap_Argv(1, name, sizeof(cmd));

	//if (!*name)
	//{
	//	trap_SendServerCommand(ent - g_entities, "print \"usage: Ignore <clientname>.\n\"");
	//	return;
	//}

	//cnum = TVG_ClientNumberFromString(ent, name);

	//if (cnum == -1)
	//{
	//	return;
	//}

	//COM_BitSet(ent->client->sess.ignoreClients, cnum);
	//trap_SendServerCommand(ent - g_entities, va("print \"[lon]You are ignoring [lof]%s[lon]^7.\n\"", level.clients[cnum].pers.netname));

	return qtrue;
}

/**
 * @brief TVG_Cmd_UnIgnore_f
 * @param[in] client
 * @param[in] self
 */
qboolean TVG_Cmd_UnIgnore_f(gclient_t *client, tvcmd_reference_t *self)
{
	//char cmd[MAX_TOKEN_CHARS], name[MAX_NAME_LENGTH];
	//int cnum;

	/*trap_Argv(0, cmd, sizeof(cmd));
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
	trap_SendServerCommand(ent - g_entities, va("print \"[lof]%s[lon]^7 is no longer ignored.\n\"", level.clients[cnum].pers.netname));*/

	return qtrue;
}

/**
* @brief TVG_ParseWarmup
*/
static void TVG_ParseWarmup(void)
{
	char cs[MAX_STRING_CHARS];

	trap_GetConfigstring(CS_WARMUP, cs, sizeof(cs));

	if (level.gamestate != GS_WARMUP || level.warmup)
	{
		level.warmup = Q_atoi(cs);
	}
}

/**
* @brief TVG_ParseWolfinfo
*/
void TVG_ParseWolfinfo(void)
{
	char        cs[MAX_STRING_CHARS];
	gamestate_t oldGamestate = level.gamestate;

	trap_GetConfigstring(CS_WOLFINFO, cs, sizeof(cs));

	level.gamestate = (gamestate_t)(atoi(Info_ValueForKey(cs, "gamestate")));

	if (oldGamestate != GS_WARMUP_COUNTDOWN && level.gamestate == GS_WARMUP_COUNTDOWN)
	{
		TVG_ParseWarmup();
	}
}

/**
* @brief TVG_ParseSvCvars
*/
void TVG_ParseSvCvars(void)
{
	if (!(level.mod & LEGACY))
	{
		return;
	}

	trap_SetConfigstring(CS_SVCVAR, "");
}

/**
* @brief TVG_ConfigStringModified *NEVER* attempt sendig configstrings directly to client!
*/
static void TVG_ConfigStringModified(void)
{
	char cmd[MAX_TOKEN_CHARS];
	int  num;

	trap_Argv(1, cmd, sizeof(cmd));

	num = Q_atoi(cmd);

	switch (num)
	{
	case CS_SVCVAR:
		TVG_ParseSvCvars();
		break;
	case CS_WARMUP:
		TVG_ParseWarmup();
		break;
	case CS_WOLFINFO:
		TVG_ParseWolfinfo();
		break;
	default:
		break;
	}
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
#define TCHAT_HASH          64370
#define VTCHAT_HASH         78944
#define VBCHAT_HASH         76784
#define GAMECHAT_HASH       101222
#define VSCHAT_HASH         78824
#define WS_HASH             27961
#define WWS_HASH            42356
#define GSTATS_HASH         80455
#define ASTATS_HASH         79741
#define ASTATSB_HASH        91991
#define BSTATS_HASH         79860
#define BSTATSB_HASH        92110
#define WBSTATS_HASH        94678
#define RWS_HASH            41761
#define MAP_RESTART_HASH    147165
#define PORTALCAMPOS_HASH   161962
#define REMAPSHADER_HASH    144301
#define ADDTOBUILD_HASH     129971
#define SPAWNSERVER_HASH    150779
#define APPLICATION_HASH    145376
#define INVITATION_HASH     134986
#define PROPOSITION_HASH    151490
#define AFT_HASH            37819
#define AFTC_HASH           49897
#define AFTJ_HASH           50751
#define COMPLAINT_HASH      118983
#define REQFORCESPAWN_HASH  176027
#define SDBG_HASH           50109
#define IMMAPLIST_HASH      120113
#define IMMAPHISTORY_HASH   164261
#define IMVOTETALLY_HASH    150058
#define SETSPAWNPT_HASH     137482
#define IMWA_HASH           51808
#define IMWS_HASH           54004
#define IMPKD_HASH          64481
#define IMPT_HASH           53279
#define IMSR_HASH           53398
#define SR_HASH             27365
#define SRA_HASH            39102
#define IMPR_HASH           53035
#define PR_HASH             27008
#define MU_START_HASH       107698
#define MU_PLAY_HASH        92607
#define MU_STOP_HASH        94568
#define MU_FADE_HASH        87906
#define SND_FADE_HASH       100375
#define ROCKANDROLL_HASH    146207
#define BP_HASH             25102
#define XPGAIN_HASH         78572
#define DISCONNECT_HASH     131683

// Legacy
#define IMPKD0_HASH 70433
#define IMPKD1_HASH 70557

// ETJUMP
#define GUID_REQUEST_HASH 161588
#define HAS_TIMERUN_HASH  134442

/**
* @brief TVG_MasterServerCommand This handles server commands (server responses to client commands)
* @details TV Client connected to master changes messages to clientNum = -2
           if we detect that incoming cmd is a broadcast message we should immediately sent it (clientNum = -1).
		   TV Server can send ClientCommands to master server too, for example requesting scores, stats etc.
		   the response will come with clientNum -2 again
* @param[in] cmd
*/
static void TVG_MasterServerCommand(char *cmd)
{
	char passcmd[MAX_TOKEN_CHARS];
	char *token;
	int  clientNum = 0;

	if (!cmd[0])
	{
		return;
	}

	Q_strncpyz(passcmd, cmd, sizeof(passcmd));

	token = strtok(passcmd, " ");

	switch (TVG_StringHashValue(token))
	{
	case ENTNFO_HASH:                     // "entnfo" = teammapdata
		trap_SendServerCommand(-1, cmd);
		return;
	case CS_HASH:                         // "cs" = configstring - do not send
		TVG_ConfigStringModified();       // update will be created just before sending snapshot
		return;
	case TINFO_HASH:                      // "tinfo" = teamplayinfo
		trap_SendServerCommand(-1, cmd);
		return;
	case SC0_HASH:                        // "sc0" = scoreboard (/score)
		level.cmds.scoreHasTwoParts = qfalse;
		Q_strncpyz(level.cmds.score[0], cmd, sizeof(level.cmds.score[0]));
		return;
	case SC1_HASH:                        // "sc1" = scoreboard (/score)
		level.cmds.scoreHasTwoParts = qtrue;
		Q_strncpyz(level.cmds.score[1], cmd, sizeof(level.cmds.score[0]));
		return;
	case WEAPONSTATS_HASH:                // "WeaponStats"
		return;
	case SC_HASH:                         // "sc" = /scores
	{
		int count;
		token = strtok(NULL, " ");

		if (Q_ParseInt(token, &count))
		{
			level.cmds.scoresIndex = 0;
			level.cmds.scoresCount = count < MAX_SCORES_CMDS ? count : MAX_SCORES_CMDS;
		}

		if (level.cmds.scoresIndex <= level.cmds.scoresCount)
		{
			Q_strncpyz(level.cmds.scores[level.cmds.scoresIndex++], cmd, sizeof(level.cmds.scores[0]));
		}
		else
		{
			G_DPrintf(S_COLOR_YELLOW "WARNING: Scores index overflow, dropping command\n");
		}
		return;
	}
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
	//case TCHAT_HASH:                                      // "tchat"
	//	return;
	//case VTCHAT_HASH:                                     // "vtchat"
	//	return;
	//case VBCHAT_HASH:                                     // "vbchat"
	//	return;
	//case GAMECHAT_HASH:                                   // "gamechat"
	//	return;
	//case VSCHAT_HASH:                                     // "vschat"
	//	return;
	// weapon stats parsing
	case WS_HASH:                                           // "ws"
		token     = strtok(NULL, " ");
		clientNum = Q_atoi(token);

		level.cmds.infoStats[INFO_WS].valid[clientNum] = qtrue;
		Q_strncpyz(level.cmds.infoStats[INFO_WS].data[clientNum], cmd, sizeof(level.cmds.infoStats[INFO_WS].data[0]));
		return;
	case WWS_HASH:                                          // "wws"
		token     = strtok(NULL, " ");
		clientNum = Q_atoi(token);

		level.cmds.infoStats[INFO_WWS].valid[clientNum] = qtrue;
		Q_strncpyz(level.cmds.infoStats[INFO_WWS].data[clientNum], cmd, sizeof(level.cmds.infoStats[INFO_WWS].data[0]));
		return;
	case GSTATS_HASH:                                       // "gstats"
		token     = strtok(NULL, " ");
		clientNum = Q_atoi(token);

		level.cmds.infoStats[INFO_GSTATS].valid[clientNum] = qtrue;
		Q_strncpyz(level.cmds.infoStats[INFO_GSTATS].data[clientNum], cmd, sizeof(level.cmds.infoStats[INFO_GSTATS].data[0]));
		return;
	//	// "topshots"-related commands
	case ASTATS_HASH:                                       // "astats"
		Q_strncpyz(level.cmds.astats, cmd, sizeof(level.cmds.astats));
		return;
	case ASTATSB_HASH:                                      // "astatsb"
		Q_strncpyz(level.cmds.astatsb, cmd, sizeof(level.cmds.astatsb));
		return;
	case BSTATS_HASH:                                       // "bstats"
		Q_strncpyz(level.cmds.bstats, cmd, sizeof(level.cmds.bstats));
		return;
	case BSTATSB_HASH:                                      // "bstatsb"
		Q_strncpyz(level.cmds.bstatsb, cmd, sizeof(level.cmds.bstatsb));
		return;
	case WBSTATS_HASH:                                      // "wbstats"
		Q_strncpyz(level.cmds.wbstats, cmd, sizeof(level.cmds.wbstats));
		return;
	//	// single weapon stat (requested weapon stats)
	//case RWS_HASH:                                        // "rws"
	//	return;
	case MAP_RESTART_HASH:                                  // "map_restart"
		trap_SendServerCommand(-1, cmd);
		return;
	//case PORTALCAMPOS_HASH:                               // "portalcampos"
	//	return;
	//case REMAPSHADER_HASH:                                // "remapShader"
	//	return;
	//// ensure a file gets into a build (mainly for scripted music calls)
	//case ADDTOBUILD_HASH:                                // "addToBuild"
	//	return;
	//// server sends this command when it's about to kill the current server, before the client can reconnect
	case SPAWNSERVER_HASH:                                 // "spawnserver"
		trap_SendServerCommand(-1, cmd);
		return;
	//case APPLICATION_HASH:                               //  "application"
	//	return;
	//case INVITATION_HASH:                                // "invitation"
	//	return;
	//case PROPOSITION_HASH:                               // "proposition"
	//	return;
	//case AFT_HASH:                                       // "aft"
	//	return;
	//case AFTC_HASH:                                      // "aftc"
	//	return;
	//case AFTJ_HASH:                                      // "aftj"
	//	return;
	//	// Allow client to lodge a complaing
	//case COMPLAINT_HASH:                                 // "complaint"
	//	return;
	//case REQFORCESPAWN_HASH:                             // "reqforcespawn"
	//	return;
	//case SDBG_HASH:                                      // "sdbg"
	//	return;
	case IMMAPLIST_HASH: // MAPVOTE                        "immaplist"
		level.cmds.immaplistValid = qtrue;
		Q_strncpyz(level.cmds.immaplist, cmd, sizeof(level.cmds.immaplist));
		return;
	case IMMAPHISTORY_HASH: // MAPVOTE                     "immaphistory"
		level.cmds.immaphistoryValid = qtrue;
		Q_strncpyz(level.cmds.immaphistory, cmd, sizeof(level.cmds.immaphistory));
		return;
	case IMVOTETALLY_HASH: // MAPVOTE                      "imvotetally"
		level.cmds.imvotetallyValid = qtrue;
		Q_strncpyz(level.cmds.imvotetally, cmd, sizeof(level.cmds.imvotetally));
		return;
	//case SETSPAWNPT_HASH: //  "setspawnpt"
	//	return;
	//	// debriefing server cmds
	case IMWA_HASH:                                        // "imwa"
		level.cmds.imwaValid = qtrue;
		Q_strncpyz(level.cmds.imwa, cmd, sizeof(level.cmds.imwa));
		return;
	case IMWS_HASH:                                        // "imws"
		level.cmds.waitingForIMWS                                       = qfalse;
		level.cmds.infoStats[INFO_IMWS].valid[level.cmds.IMWSClientNum] = qtrue;
		Q_strncpyz(level.cmds.infoStats[INFO_IMWS].data[level.cmds.IMWSClientNum], cmd, sizeof(level.cmds.infoStats[INFO_IMWS].data[0]));
		return;
	case IMPKD_HASH:                                       // "impkd"
	case IMPKD0_HASH:                                      // "impkd0"
		level.cmds.impkdValid = qtrue;
		Q_strncpyz(level.cmds.impkd[0], cmd, sizeof(level.cmds.impkd[0]));
		return;
	case IMPKD1_HASH:                                      // "impkd1"
		Q_strncpyz(level.cmds.impkd[1], cmd, sizeof(level.cmds.impkd[1]));
		return;
	case IMPT_HASH:                                        // "impt"
		level.cmds.imptValid = qtrue;
		Q_strncpyz(level.cmds.impt, cmd, sizeof(level.cmds.impt));
		return;
	// music loops \/
	case MU_START_HASH:                                    // "mu_start" has optional parameter for fade-up time
		trap_SendServerCommand(-1, cmd);
		return;
	// plays once then back to whatever the loop was \/
	case MU_PLAY_HASH:                                     // "mu_play" has optional parameter for fade-up time
		trap_SendServerCommand(-1, cmd);
		return;
	case MU_STOP_HASH:                                     // "mu_stop" has optional parameter for fade-down time
		trap_SendServerCommand(-1, cmd);
		return;
	case MU_FADE_HASH:                                     // "mu_fade"
		trap_SendServerCommand(-1, cmd);
		return;
	case SND_FADE_HASH:                                    // "snd_fade"
		trap_SendServerCommand(-1, cmd);
		return;
	//case ROCKANDROLL_HASH:                               // "rockandroll"
	//	return;
	case BP_HASH: // "bp"
		trap_SendServerCommand(-1, cmd);
		return;
	//case XPGAIN_HASH:                                    // "xpgain"
	//	return;
	case DISCONNECT_HASH:                                  // "disconnect"
		trap_SendServerCommand(-1, cmd);
		return;

	// Legacy
	case IMSR_HASH:                                        // "imsr"
		level.cmds.imsrValid = qtrue;
		Q_strncpyz(level.cmds.imsr, cmd, sizeof(level.cmds.imsr));
		return;
	//case SR_HASH:                                        // "sr" - backward compatibility with 2.76 demos
	//	return;
	case SRA_HASH:                                         // "sra"
		level.cmds.sraValid = qtrue;
		Q_strncpyz(level.cmds.sra, cmd, sizeof(level.cmds.sra));
		return;
	case IMPR_HASH:                                        // "impr"
		level.cmds.imprValid = qtrue;
		Q_strncpyz(level.cmds.impr, cmd, sizeof(level.cmds.impr));
		return;
	case PR_HASH:                                          // "pr"
		level.cmds.prValid = qtrue;
		Q_strncpyz(level.cmds.pr, cmd, sizeof(level.cmds.pr));
		return;

	// ETJump
	case GUID_REQUEST_HASH:
	case HAS_TIMERUN_HASH:
		return;

	default:
		G_Printf("TVGAME: Unknown master server game command: %s [%lu]\n", cmd, TVG_StringHashValue(token));
		break;
	}
}

/**
 * @brief TVG_ClientCommand
 * @param[in] clientNum
 */
void TVG_ClientCommand(int clientNum)
{
	gclient_t *client;
	char      cmd[MAX_TOKEN_CHARS];

	trap_Argv(0, cmd, sizeof(cmd));

#ifdef FEATURE_LUA
	if (TVG_LuaHook_ClientCommand(clientNum, cmd))
	{
		return;
	}
#endif

	if (clientNum == -2)
	{
		TVG_MasterServerCommand(cmd);
		return;
	}

	client = level.clients + clientNum;

#ifdef FEATURE_LUA
	if (Q_stricmp(cmd, "lua_status") == 0)
	{
		TVG_LuaStatus(client);
		return;
	}
#endif

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
