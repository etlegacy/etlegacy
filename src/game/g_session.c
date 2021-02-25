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
 * @file g_session.c
 * @brief Session data is the only data that stays persistant across level loads
 *        and tournament restarts.
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

/**
 * SESSION DATA
*/

/**
 * @brief Called on game shutdown
 * @param[in] client
 * @param[in] restart
 */
void G_WriteClientSessionData(gclient_t *client, qboolean restart)
{
#ifdef FEATURE_MULTIVIEW
	int mvc = G_smvGenerateClientList(g_entities + (client - level.clients));
#endif
	const char *s;

	// stats reset check
	if (level.fResetStats)
	{
		G_deleteStats(client - level.clients);
	}

#ifdef FEATURE_MULTIVIEW
#ifdef FEATURE_RATING
#ifdef FEATURE_PRESTIGE
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i %i %i %i",
#else
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i %i %i",
#endif
#else
#ifdef FEATURE_PRESTIGE
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#else
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#endif
#endif
#else
#ifdef FEATURE_RATING
#ifdef FEATURE_PRESTIGE
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i %i",
#else
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i",
#endif
#else
#ifdef FEATURE_PRESTIGE
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#else
	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#endif
#endif
#endif
	       client->sess.sessionTeam,
	       client->sess.spectatorTime,
	       client->sess.spectatorState,
	       client->sess.spectatorClient,
	       client->sess.playerType,
	       client->sess.playerWeapon,
	       client->sess.playerWeapon2,
	       client->sess.latchPlayerType,
	       client->sess.latchPlayerWeapon,
	       client->sess.latchPlayerWeapon2,

	       client->sess.referee,
	       client->sess.shoutcaster,
	       client->sess.spec_invite,
	       client->sess.spec_team,
	       client->sess.kills,
	       client->sess.deaths,
	       client->sess.gibs,
	       client->sess.self_kills,
	       client->sess.team_kills,
	       client->sess.team_gibs,

	       client->sess.time_axis,
	       client->sess.time_allies,
	       client->sess.time_played,
#ifdef FEATURE_RATING
	       client->sess.mu,
	       client->sess.sigma,
	       client->sess.oldmu,
	       client->sess.oldsigma,
#endif
#ifdef FEATURE_PRESTIGE
	       client->sess.prestige,
#endif
#ifdef FEATURE_MULTIVIEW
	       (mvc & 0xFFFF),
	       ((mvc >> 16) & 0xFFFF),
#endif
	       // Damage and rounds played rolled in with weapon stats (below)
	       client->sess.muted,

	       client->sess.ignoreClients[0],
	       client->sess.ignoreClients[1],
	       client->pers.enterTime,
	       restart ? client->sess.userSpawnPointValue : 0,
	       client->sess.uci
	       );

	trap_Cvar_Set(va("session%i", (int)(client - level.clients)), s);

	// store the clients stats (7) and medals (7)
	// addition: but only if it isn't a forced map_restart (done by someone on the console)
	if (!(restart && !level.warmupTime))
	{
		s = va("%.2f %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i %i %i %i %i",
		       client->sess.skillpoints[0],
		       client->sess.skillpoints[1],
		       client->sess.skillpoints[2],
		       client->sess.skillpoints[3],
		       client->sess.skillpoints[4],
		       client->sess.skillpoints[5],
		       client->sess.skillpoints[6],
		       client->sess.medals[0],
		       client->sess.medals[1],
		       client->sess.medals[2],
		       client->sess.medals[3],
		       client->sess.medals[4],
		       client->sess.medals[5],
		       client->sess.medals[6]
		       );

		trap_Cvar_Set(va("sessionstats%i", (int)(client - level.clients)), s);
	}

	// save weapon stats too
	if (!level.fResetStats)
	{
		trap_Cvar_Set(va("wstats%i", (int)(client - level.clients)), G_createStats(&g_entities[client - level.clients]));
	}
}

/**
 * @brief Client swap handling
 * @param[in,out] client
 */
void G_ClientSwap(gclient_t *client)
{
	int flags = 0;

	if (client->sess.sessionTeam == TEAM_AXIS)
	{
		client->sess.sessionTeam = TEAM_ALLIES;

		// swap primary weapon
		if (GetWeaponTableData(client->sess.playerWeapon)->weapEquiv)
		{
			client->sess.playerWeapon = client->sess.latchPlayerWeapon = GetWeaponTableData(client->sess.playerWeapon)->weapEquiv;
		}

		// swap secondary weapon
		if (GetWeaponTableData(client->sess.playerWeapon2)->weapEquiv)
		{
			client->sess.playerWeapon2 = client->sess.latchPlayerWeapon2 = GetWeaponTableData(client->sess.playerWeapon2)->weapEquiv;
		}
	}
	else if (client->sess.sessionTeam == TEAM_ALLIES)
	{
		client->sess.sessionTeam = TEAM_AXIS;

		// swap primary weapon
		if (GetWeaponTableData(client->sess.playerWeapon)->weapEquiv)
		{
			client->sess.playerWeapon = client->sess.latchPlayerWeapon = GetWeaponTableData(client->sess.playerWeapon)->weapEquiv;
		}

		// swap secondary weapon
		if (GetWeaponTableData(client->sess.playerWeapon2)->weapEquiv)
		{
			client->sess.playerWeapon2 = client->sess.latchPlayerWeapon2 = GetWeaponTableData(client->sess.playerWeapon2)->weapEquiv;
		}
	}

	// Swap spec invites as well
	if (client->sess.spec_invite & TEAM_AXIS)
	{
		flags |= TEAM_ALLIES;
	}
	if (client->sess.spec_invite & TEAM_ALLIES)
	{
		flags |= TEAM_AXIS;
	}

	client->sess.spec_invite = flags;

	// Swap spec follows as well
	flags = 0;
	if (client->sess.spec_team & TEAM_AXIS)
	{
		flags |= TEAM_ALLIES;
	}
	if (client->sess.spec_team & TEAM_ALLIES)
	{
		flags |= TEAM_AXIS;
	}

	client->sess.spec_team = flags;
}

/**
 * @brief G_CalcRank
 * @param[in,out] client
 */
void G_CalcRank(gclient_t *client)
{
	int i, highestskill = 0;

#ifdef FEATURE_RATING
	// rating values for rank levels
	// lognormal(e, 2/e) for each 1/11th percentile
	float rankRating[NUM_EXPERIENCE_LEVELS] = { 0.000001,
	                                            5.674106,  7.766937,  9.712880,  11.724512, 13.933123,
	                                            16.482425, 19.587310, 23.644035, 29.567854, 40.473632 };

	if (g_skillRating.integer)
	{
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			G_SetPlayerSkill(client, i);
		}

		// set rank
		for (i = 0; i < NUM_EXPERIENCE_LEVELS; i++)
		{
			if (client->sess.mu - 3 * client->sess.sigma <= rankRating[i])
			{
				client->sess.rank = i - 1;
				break;
			}
			else
			{
				client->sess.rank = 10;
			}
		}

		if (client->sess.rank < 0)
		{
			client->sess.rank = 0;
		}

		return;
	}
#endif

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		G_SetPlayerSkill(client, i);
		if (client->sess.skill[i] > highestskill)
		{
			highestskill = client->sess.skill[i];
		}
	}

	// set rank
	client->sess.rank = highestskill;

	if (client->sess.rank >= 4)
	{
		int cnt = 0;

		// count the number of maxed out skills
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			if (client->sess.skill[i] >= 4)
			{
				cnt++;
			}
		}

		client->sess.rank = cnt + 3;
		if (client->sess.rank > 10)
		{
			client->sess.rank = 10;
		}
	}
}

/**
 * @brief Called on a reconnect
 * @param[in] client
 */
void G_ReadSessionData(gclient_t *client)
{
#ifdef FEATURE_MULTIVIEW
	int mvc_l, mvc_h;
#endif
	int      j;
	char     s[MAX_STRING_CHARS];
	qboolean test;

	trap_Cvar_VariableStringBuffer(va("session%i", (int)(client - level.clients)), s, sizeof(s));

#ifdef FEATURE_MULTIVIEW
#ifdef FEATURE_RATING
#ifdef FEATURE_PRESTIGE
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i %i %i %i",
#else
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i %i %i",
#endif
#else
#ifdef FEATURE_PRESTIGE
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#else
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#endif
#endif
#else
#ifdef FEATURE_RATING
#ifdef FEATURE_PRESTIGE
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i %i",
#else
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %f %f %f %i %i %i %i %i %i",
#endif
#else
#ifdef FEATURE_PRESTIGE
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#else
	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
#endif
#endif
#endif
	       (int *)&client->sess.sessionTeam,
	       &client->sess.spectatorTime,
	       (int *)&client->sess.spectatorState,
	       &client->sess.spectatorClient,
	       &client->sess.playerType,
	       (int *)&client->sess.playerWeapon,
	       (int *)&client->sess.playerWeapon2,
	       &client->sess.latchPlayerType,
	       (int *)&client->sess.latchPlayerWeapon,
	       (int *)&client->sess.latchPlayerWeapon2,

	       &client->sess.referee,
	       &client->sess.shoutcaster,
	       &client->sess.spec_invite,
	       &client->sess.spec_team,
	       &client->sess.kills,
	       &client->sess.deaths,
	       &client->sess.gibs,
	       &client->sess.self_kills,
	       &client->sess.team_kills,
	       &client->sess.team_gibs,

	       &client->sess.time_axis,
	       &client->sess.time_allies,
	       &client->sess.time_played,
#ifdef FEATURE_RATING
	       &client->sess.mu,
	       &client->sess.sigma,
	       &client->sess.oldmu,
	       &client->sess.oldsigma,
#endif
#ifdef FEATURE_PRESTIGE
	       &client->sess.prestige,
#endif
#ifdef FEATURE_MULTIVIEW
	       &mvc_l,
	       &mvc_h,
#endif
	       // Damage and round count rolled in with weapon stats (below)
	       (int *)&client->sess.muted,

	       &client->sess.ignoreClients[0],
	       &client->sess.ignoreClients[1],
	       &client->pers.enterTime,
	       &client->sess.userSpawnPointValue,
	       &client->sess.uci
	       );

#ifdef FEATURE_MULTIVIEW
	// reinstate MV clients
	client->pers.mvReferenceList = (mvc_h << 16) | mvc_l;
#endif

	// pull and parse weapon stats
	*s = 0;
	trap_Cvar_VariableStringBuffer(va("wstats%i", (int)(client - level.clients)), s, sizeof(s));
	if (*s)
	{
		G_parseStats(s);
		if (g_gamestate.integer == GS_PLAYING)
		{
			client->sess.rounds++;
		}
	}

	// likely there are more cases in which we don't want this
	if (g_gametype.integer != GT_SINGLE_PLAYER &&
	    g_gametype.integer != GT_COOP &&
	    g_gametype.integer != GT_WOLF &&
	    g_gametype.integer != GT_WOLF_STOPWATCH &&
	    !(g_gametype.integer == GT_WOLF_CAMPAIGN && (g_campaigns[level.currentCampaign].current == 0  || level.newCampaign)) &&
	    !(g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer == 0))
	{

		trap_Cvar_VariableStringBuffer(va("sessionstats%i", (int)(client - level.clients)), s, sizeof(s));

		// read the clients stats (7) and medals (7)
		sscanf(s, "%f %f %f %f %f %f %f %i %i %i %i %i %i %i",
		       &client->sess.skillpoints[0],
		       &client->sess.skillpoints[1],
		       &client->sess.skillpoints[2],
		       &client->sess.skillpoints[3],
		       &client->sess.skillpoints[4],
		       &client->sess.skillpoints[5],
		       &client->sess.skillpoints[6],
		       &client->sess.medals[0],
		       &client->sess.medals[1],
		       &client->sess.medals[2],
		       &client->sess.medals[3],
		       &client->sess.medals[4],
		       &client->sess.medals[5],
		       &client->sess.medals[6]
		       );
	}

	G_CalcRank(client);

	test = (g_altStopwatchMode.integer != 0 || g_currentRound.integer == 1);

	if (g_gametype.integer == GT_WOLF_STOPWATCH && g_gamestate.integer != GS_PLAYING && test)
	{
		G_ClientSwap(client);
	}

	if (g_swapteams.integer)
	{
		trap_Cvar_Set("g_swapteams", "0");
		G_ClientSwap(client);
	}

	client->sess.startxptotal = 0;
	for (j = 0; j < SK_NUM_SKILLS; j++)
	{
		client->sess.startskillpoints[j] = client->sess.skillpoints[j];
		client->sess.startxptotal += client->sess.skillpoints[j];
	}
}

/**
 * @brief Called on a first-time connect
 * @param[in] client
 * @param userinfo
 */
void G_InitSessionData(gclient_t *client, const char *userinfo)
{
	clientSession_t *sess = &client->sess;

	// initial team determination
	sess->sessionTeam = TEAM_SPECTATOR;

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

	sess->latchPlayerType = sess->playerType = 0;
	sess->latchPlayerWeapon = sess->playerWeapon = WP_NONE;
	sess->latchPlayerWeapon2 = sess->playerWeapon2 = WP_NONE;

	sess->userSpawnPointValue = 0;

	Com_Memset(sess->ignoreClients, 0, sizeof(sess->ignoreClients));

	sess->muted = qfalse;
	Com_Memset(sess->skill, 0, sizeof(sess->skill));
	Com_Memset(sess->skillpoints, 0, sizeof(sess->skillpoints));
	Com_Memset(sess->startskillpoints, 0, sizeof(sess->startskillpoints));
	Com_Memset(sess->medals, 0, sizeof(sess->medals));
	sess->rank = 0;
	sess->startxptotal = 0;

	// we set ref in ClientUserinfoChanged
	sess->referee = RL_NONE; // (client->pers.localClient) ? RL_REFEREE : RL_NONE;
	sess->spec_invite = 0;
	sess->spec_team = 0;

	sess->uci = 0; // GeoIP

	G_deleteStats(client - level.clients);

	G_WriteClientSessionData(client, qfalse);
}

/**
 * @brief G_InitWorldSession
 */
void G_InitWorldSession(void)
{
	char s[MAX_STRING_CHARS];
	int  gt;
	int  i, j;

	trap_Cvar_VariableStringBuffer("session", s, sizeof(s));
	gt = Q_atoi(s);

	// if the gametype changed since the last session, don't use any
	// client sessions
	if (g_gametype.integer != gt)
	{
		level.fResetStats = qtrue;
		G_Printf("Gametype changed, clearing session data.\n");
	}
	else
	{
		char     *tmp = s;
		qboolean test = (g_altStopwatchMode.integer != 0 || g_currentRound.integer == 1);

#define GETVAL(x) if ((tmp = strchr(tmp, ' ')) == NULL) { return; } x = Q_atoi(++tmp);

		// Get team lock stuff
		GETVAL(gt);
		teamInfo[TEAM_AXIS].spec_lock = (gt & TEAM_AXIS) ? qtrue : qfalse;
		teamInfo[TEAM_ALLIES].spec_lock = (gt & TEAM_ALLIES) ? qtrue : qfalse;

		// See if we need to clear player stats
		// FIXME: deal with the multi-map missions
		if (g_gametype.integer != GT_WOLF_CAMPAIGN)
		{
			if ((tmp = strchr(va("%s", tmp), ' ')) != NULL)
			{
				tmp++;
				trap_GetServerinfo(s, sizeof(s));
				if (Q_stricmp(tmp, Info_ValueForKey(s, "mapname")))
				{
					level.fResetStats = qtrue;
					G_Printf("Map changed, clearing player stats.\n");
				}
			}
		}

		// have to make sure spec locks follow the right teams
		if (g_gametype.integer == GT_WOLF_STOPWATCH && g_gamestate.integer != GS_PLAYING && test)
		{
			G_swapTeamLocks();
		}

		if (g_swapteams.integer)
		{
			G_swapTeamLocks();
		}
	}

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		char *p, *c;

		trap_Cvar_VariableStringBuffer(va("fireteam%i", i), s, sizeof(s));

		p = Info_ValueForKey(s, "id");
		j = Q_atoi(p);
		if (!*p || j == -1)
		{
			level.fireTeams[i].inuse = qfalse;
		}
		else
		{
			level.fireTeams[i].inuse = qtrue;
		}
		level.fireTeams[i].ident = j + 1;

		p = Info_ValueForKey(s, "p");
		level.fireTeams[i].priv = !Q_atoi(p) ? qfalse : qtrue;

		p = Info_ValueForKey(s, "i");

		j = 0;
		if (p && *p)
		{
			c = p;
			for (c = strchr(c, ' ') + 1; c && *c; )
			{
				char str[8];
				char *l = strchr(c, ' ');

				if (!l)
				{
					break;
				}
				Q_strncpyz(str, c, l - c + 1);
				str[l - c] = '\0';
				level.fireTeams[i].joinOrder[j++] = Q_atoi(str);
				c = l + 1;
			}
		}

		for ( ; j < MAX_CLIENTS; j++)
		{
			level.fireTeams[i].joinOrder[j] = -1;
		}
		G_UpdateFireteamConfigString(&level.fireTeams[i]);
	}
}

/**
 * @brief G_WriteSessionData
 * @param[in] restart
 */
void G_WriteSessionData(qboolean restart)
{
	int  i;
	char strServerInfo[MAX_INFO_STRING];
	int  j;

	trap_GetServerinfo(strServerInfo, sizeof(strServerInfo));
	trap_Cvar_Set("session", va("%i %i %s", g_gametype.integer,
	                            (teamInfo[TEAM_AXIS].spec_lock * TEAM_AXIS | teamInfo[TEAM_ALLIES].spec_lock * TEAM_ALLIES),
	                            Info_ValueForKey(strServerInfo, "mapname")));

	// Keep stats for all players in sync
	for (i = 0; !level.fResetStats && i < level.numConnectedClients; i++)
	{
		if ((g_gamestate.integer == GS_WARMUP_COUNTDOWN &&
		     ((g_gametype.integer == GT_WOLF_STOPWATCH && level.clients[level.sortedClients[i]].sess.rounds >= 2) ||
		      (g_gametype.integer != GT_WOLF_STOPWATCH && level.clients[level.sortedClients[i]].sess.rounds >= 1))))
		{
			level.fResetStats = qtrue;
		}
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		// Also take care of slow connecters and a short warmup
		if (level.clients[level.sortedClients[i]].pers.connected == CON_CONNECTED || level.fResetStats)
		{
			G_WriteClientSessionData(&level.clients[level.sortedClients[i]], restart);
		}
	}

	for (i = 0; i < MAX_FIRETEAMS; i++)
	{
		char buffer[MAX_STRING_CHARS];

		if (!level.fireTeams[i].inuse)
		{
			Com_sprintf(buffer, MAX_STRING_CHARS, "\\id\\-1");
		}
		else
		{
			char buffer2[MAX_STRING_CHARS];
			char p[8];

			*buffer2 = '\0';
			for (j = 0; j < MAX_CLIENTS; j++)
			{
				Com_sprintf(p, 8, " %i", level.fireTeams[i].joinOrder[j]);
				Q_strcat(buffer2, MAX_STRING_CHARS, p);
			}
			Com_sprintf(buffer, MAX_STRING_CHARS, "\\id\\%i\\i\\%s\\p\\%i", level.fireTeams[i].ident - 1, buffer2, level.fireTeams[i].priv ? 1 : 0);
		}

		trap_Cvar_Set(va("fireteam%i", i), buffer);
	}
}
