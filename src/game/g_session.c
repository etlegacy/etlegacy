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
 * @file g_session.c
 * @brief Session data is the only data that stays persistant across level loads
 *        and tournament restarts.
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"

#include "json.h"

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
	cJSON *root, *campaign, *restartObj = NULL;
	char  fileName[MAX_QPATH] = { 0 };

	Com_sprintf(fileName, sizeof(fileName), "session/client%02i.json", (int)(client - level.clients));
	Com_Printf("Writing session file %s\n", fileName);

	// stats reset check
	if (level.fResetStats)
	{
		G_deleteStats(client - level.clients);
	}

	Q_JSONInit();

	root = cJSON_CreateObject();
	if (!root)
	{
		Com_Error(ERR_FATAL, "Could not allocate memory for session data\n");
	}

	cJSON_AddNumberToObject(root, "sessionTeam", client->sess.sessionTeam);
	cJSON_AddNumberToObject(root, "spectatorTime", client->sess.spectatorTime);
	cJSON_AddNumberToObject(root, "spectatorState", client->sess.spectatorState);
	cJSON_AddNumberToObject(root, "spectatorClient", client->sess.spectatorClient);
	cJSON_AddNumberToObject(root, "playerType", client->sess.playerType);
	cJSON_AddNumberToObject(root, "playerWeapon", client->sess.playerWeapon);
	cJSON_AddNumberToObject(root, "playerWeapon2", client->sess.playerWeapon2);
	cJSON_AddNumberToObject(root, "latchPlayerType", client->sess.latchPlayerType);
	cJSON_AddNumberToObject(root, "latchPlayerWeapon", client->sess.latchPlayerWeapon);
	cJSON_AddNumberToObject(root, "latchPlayerWeapon2", client->sess.latchPlayerWeapon2);
	cJSON_AddNumberToObject(root, "referee", client->sess.referee);
	cJSON_AddNumberToObject(root, "shoutcaster", client->sess.shoutcaster);
	cJSON_AddNumberToObject(root, "spec_invite", client->sess.spec_invite);
	cJSON_AddNumberToObject(root, "spec_team", client->sess.spec_team);
	cJSON_AddNumberToObject(root, "kills", client->sess.kills);
	cJSON_AddNumberToObject(root, "kill_assists", client->sess.kill_assists);
	cJSON_AddNumberToObject(root, "deaths", client->sess.deaths);
	cJSON_AddNumberToObject(root, "gibs", client->sess.gibs);
	cJSON_AddNumberToObject(root, "self_kills", client->sess.self_kills);
	cJSON_AddNumberToObject(root, "team_kills", client->sess.team_kills);
	cJSON_AddNumberToObject(root, "team_gibs", client->sess.team_gibs);
	cJSON_AddNumberToObject(root, "time_axis", client->sess.time_axis);
	cJSON_AddNumberToObject(root, "time_allies", client->sess.time_allies);
	cJSON_AddNumberToObject(root, "time_played", client->sess.time_played);

#ifdef FEATURE_RATING
	{
		cJSON *ratingObj = cJSON_AddObjectToObject(root, "rating");
		cJSON_AddNumberToObject(ratingObj, "mu", client->sess.mu);
		cJSON_AddNumberToObject(ratingObj, "sigma", client->sess.sigma);
		cJSON_AddNumberToObject(ratingObj, "oldmu", client->sess.oldmu);
		cJSON_AddNumberToObject(ratingObj, "oldsigma", client->sess.oldsigma);
	}
#endif

#ifdef FEATURE_PRESTIGE
	cJSON_AddNumberToObject(root, "prestige", client->sess.prestige);
#endif

#ifdef FEATURE_MULTIVIEW
	{
		int mvc = G_smvGenerateClientList(g_entities + (client - level.clients));
		// FIXME: naming
		cJSON *multiViewObj = cJSON_AddObjectToObject(root, "multiview");
		cJSON_AddNumberToObject(multiViewObj, "mv1", (mvc & 0xFFFF));
		cJSON_AddNumberToObject(multiViewObj, "mv2", ((mvc >> 16) & 0xFFFF));
	}
#endif

	cJSON_AddNumberToObject(root, "muted", client->sess.muted);
	cJSON_AddNumberToObject(root, "ignoreClients1", client->sess.ignoreClients[0]);
	cJSON_AddNumberToObject(root, "ignoreClients2", client->sess.ignoreClients[1]);
	cJSON_AddNumberToObject(root, "enterTime", client->pers.enterTime);
	cJSON_AddNumberToObject(root, "userSpawnPointValue", restart ? client->sess.userSpawnPointValue : 0);
	cJSON_AddNumberToObject(root, "userMinorSpawnPointValue", restart ? client->sess.userMinorSpawnPointValue : -1);
	cJSON_AddNumberToObject(root, "uci", client->sess.uci);
	cJSON_AddNumberToObject(root, "tvflags", client->sess.tvflags);

	// store the clients stats (7) and medals (7)
	// addition: but only if it isn't a forced map_restart (done by someone on the console)
	if (!(restart && !level.warmupTime))
	{
		restartObj = cJSON_AddObjectToObject(root, "restart");
		cJSON_AddItemToObject(restartObj, "skillpoints", cJSON_CreateFloatArray(client->sess.skillpoints, SK_NUM_SKILLS));
		cJSON_AddItemToObject(restartObj, "medals", cJSON_CreateIntArray(client->sess.medals, SK_NUM_SKILLS));
		restartObj = NULL;
	}
	else
	{
		// restore the restart info from the old session file
		cJSON *tmp = Q_FSReadJsonFrom(fileName);
		if (tmp)
		{
			restartObj = cJSON_GetObjectItemCaseSensitive(tmp, "restart");
			cJSON_AddItemReferenceToObject(root, "restart", restartObj);
		}
	}

	// save weapon stats too
	if (!level.fResetStats)
	{
		G_createStatsJson(&g_entities[client - level.clients], cJSON_AddObjectToObject(root, "wstats"));
	}

	if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		campaign = cJSON_AddObjectToObject(root, "campaign");
		cJSON_AddNumberToObject(campaign, "campaign", level.currentCampaign);
		cJSON_AddNumberToObject(campaign, "map", g_currentCampaignMap.integer);
	}

	if (!Q_FSWriteJSONTo(root, fileName))
	{
		Com_Error(ERR_FATAL, "Could not write session information\n");
	}

	if (restartObj)
	{
		cJSON_Delete(restartObj);
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
	int i, j, highestskill = 0;

#ifdef FEATURE_RATING
	// rating values for rank levels
	// lognormal(e, 2/e) for each 1/11th percentile
	float rankRating[NUM_EXPERIENCE_LEVELS] = { 0.000001,
		                                        5.674106, 7.766937,   9.712880,  11.724512, 13.933123,
		                                        16.482425,19.587310,  23.644035, 29.567854, 40.473632 };

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

		for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
		{
			if (GetSkillTableData(i)->skillLevels[j] >= 0 && client->sess.skillpoints[i] >= GetSkillTableData(i)->skillLevels[j])
			{
				if (j > highestskill)
				{
					highestskill = j;
				}
				break;
			}
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
			if (GetSkillTableData(i)->skillLevels[4] >= 0 && client->sess.skillpoints[i] >= GetSkillTableData(i)->skillLevels[4])
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
	char     fileName[MAX_QPATH] = { 0 };
	cJSON    *root = NULL, *wstats = NULL, *campaign = NULL;
	qboolean test, restoreStats = qtrue;
	int      i = 0;

	Com_sprintf(fileName, sizeof(fileName), "session/client%02i.json", (int)(client - level.clients));
	Com_Printf("Reading session file %s\n", fileName);

	root = Q_FSReadJsonFrom(fileName);

	if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		campaign = cJSON_GetObjectItem(root, "campaign");

		if (campaign)
		{
			restoreStats = Q_ReadIntValueJson(campaign, "campaign") == level.currentCampaign
			               && Q_ReadIntValueJson(campaign, "map") == g_currentCampaignMap.integer;
		}
	}

	client->sess.sessionTeam        = Q_ReadIntValueJson(root, "sessionTeam");
	client->sess.spectatorTime      = Q_ReadIntValueJson(root, "spectatorTime");
	client->sess.spectatorState     = Q_ReadIntValueJson(root, "spectatorState");
	client->sess.spectatorClient    = Q_ReadIntValueJson(root, "spectatorClient");
	client->sess.playerType         = Q_ReadIntValueJson(root, "playerType");
	client->sess.playerWeapon       = Q_ReadIntValueJson(root, "playerWeapon");
	client->sess.playerWeapon2      = Q_ReadIntValueJson(root, "playerWeapon2");
	client->sess.latchPlayerType    = Q_ReadIntValueJson(root, "latchPlayerType");
	client->sess.latchPlayerWeapon  = Q_ReadIntValueJson(root, "latchPlayerWeapon");
	client->sess.latchPlayerWeapon2 = Q_ReadIntValueJson(root, "latchPlayerWeapon2");
	client->sess.referee            = Q_ReadIntValueJson(root, "referee");
	client->sess.shoutcaster        = Q_ReadIntValueJson(root, "shoutcaster");
	client->sess.spec_invite        = Q_ReadIntValueJson(root, "spec_invite");
	client->sess.spec_team          = Q_ReadIntValueJson(root, "spec_team");

	if (restoreStats)
	{
		client->sess.kills        = Q_ReadIntValueJson(root, "kills");
		client->sess.kill_assists = Q_ReadIntValueJson(root, "kill_assists");
		client->sess.deaths       = Q_ReadIntValueJson(root, "deaths");
		client->sess.gibs         = Q_ReadIntValueJson(root, "gibs");
		client->sess.self_kills   = Q_ReadIntValueJson(root, "self_kills");
		client->sess.team_kills   = Q_ReadIntValueJson(root, "team_kills");
		client->sess.team_gibs    = Q_ReadIntValueJson(root, "team_gibs");
		client->sess.time_axis    = Q_ReadIntValueJson(root, "time_axis");
		client->sess.time_allies  = Q_ReadIntValueJson(root, "time_allies");
		client->sess.time_played  = Q_ReadIntValueJson(root, "time_played");
	}

#ifdef FEATURE_RATING
	{
		cJSON *ratingObj = cJSON_GetObjectItem(root, "rating");

		if (ratingObj)
		{
			client->sess.mu       = Q_ReadFloatValueJson(ratingObj, "mu");
			client->sess.sigma    = Q_ReadFloatValueJson(ratingObj, "sigma");
			client->sess.oldmu    = Q_ReadFloatValueJson(ratingObj, "oldmu");
			client->sess.oldsigma = Q_ReadFloatValueJson(ratingObj, "oldsigma");
		}
		else
		{
			Q_JsonError("Rating object missing\n");
		}
	}
#endif

#ifdef FEATURE_PRESTIGE
	client->sess.prestige = Q_ReadIntValueJson(root, "prestige");
#endif

#ifdef FEATURE_MULTIVIEW
	{
		cJSON *mvc = cJSON_GetObjectItem(root, "multiview");

		if (mvc)
		{
			int mvc_l, mvc_h;
			mvc_l = Q_ReadIntValueJson(mvc, "mv1");
			mvc_h = Q_ReadIntValueJson(mvc, "mv2");

			// reinstate MV clients
			client->pers.mvReferenceList = (mvc_h << 16) | mvc_l;
		}
		else
		{
			Q_JsonError("Multiview object missing\n");
		}
	}
#endif

	client->sess.muted                    = Q_ReadIntValueJson(root, "muted");
	client->sess.ignoreClients[0]         = Q_ReadIntValueJson(root, "ignoreClients1");
	client->sess.ignoreClients[1]         = Q_ReadIntValueJson(root, "ignoreClients2");
	client->pers.enterTime                = Q_ReadIntValueJson(root, "enterTime");
	client->sess.userSpawnPointValue      = Q_ReadIntValueJson(root, "userSpawnPointValue");
	client->sess.userMinorSpawnPointValue = Q_ReadIntValueJson(root, "userMinorSpawnPointValue");
	client->sess.uci                      = Q_ReadIntValueJson(root, "uci");
	client->sess.tvflags                  = Q_ReadIntValueJson(root, "tvflags");

	// pull and parse weapon stats
	wstats = cJSON_GetObjectItem(root, "wstats");
	if (wstats && restoreStats)
	{
		G_parseStatsJson(wstats);
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
		cJSON *restartObj = cJSON_GetObjectItem(root, "restart");

		if (restartObj)
		{
			cJSON *tmp, *tmp2;

			i   = 0;
			tmp = cJSON_GetObjectItem(restartObj, "skillpoints");
			cJSON_ArrayForEach(tmp2, tmp)
			{
				if (i > SK_NUM_SKILLS)
				{
					Q_JsonError("Invalid number of skills\n");
					break;
				}
				client->sess.skillpoints[i++] = (float) cJSON_GetNumberValue(tmp2);
			}

			i   = 0;
			tmp = cJSON_GetObjectItem(restartObj, "medals");
			cJSON_ArrayForEach(tmp2, tmp)
			{
				if (i > SK_NUM_SKILLS)
				{
					Q_JsonError("Invalid number of medals\n");
					break;
				}
				client->sess.medals[i++] = (int) cJSON_GetNumberValue(tmp2);
			}
		}
	}
	cJSON_Delete(root);

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
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		client->sess.startskillpoints[i] = client->sess.skillpoints[i];
		client->sess.startxptotal       += client->sess.skillpoints[i];
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
	int             protocol;

	// initial team determination
	sess->sessionTeam = TEAM_SPECTATOR;

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime  = level.time;

	sess->latchPlayerType    = sess->playerType = 0;
	sess->latchPlayerWeapon  = sess->playerWeapon = WP_NONE;
	sess->latchPlayerWeapon2 = sess->playerWeapon2 = WP_NONE;

	sess->userSpawnPointValue      = 0;
	sess->userMinorSpawnPointValue = -1;

	Com_Memset(sess->ignoreClients, 0, sizeof(sess->ignoreClients));

	sess->muted = qfalse;
	Com_Memset(sess->skill, 0, sizeof(sess->skill));
	Com_Memset(sess->skillpoints, 0, sizeof(sess->skillpoints));
	Com_Memset(sess->startskillpoints, 0, sizeof(sess->startskillpoints));
	Com_Memset(sess->medals, 0, sizeof(sess->medals));
	sess->rank         = 0;
	sess->startxptotal = 0;

	// we set ref in ClientUserinfoChanged
	sess->referee     = RL_NONE; // (client->pers.localClient) ? RL_REFEREE : RL_NONE;
	sess->spec_invite = 0;
	sess->spec_team   = 0;

	sess->uci = 0; // GeoIP

	protocol = Q_atoi(Info_ValueForKey(userinfo, "protocol"));

	if (protocol == ETTV_PROTOCOL_VERSION)
	{
		sess->tvflags = g_etltv_flags.integer;
	}

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

#define GETVAL(x) if ((tmp = strchr(tmp, ' ')) == NULL) { return; \
} x = Q_atoi(++tmp);

		// Get team lock stuff
		GETVAL(gt);
		teamInfo[TEAM_AXIS].spec_lock   = (gt & TEAM_AXIS) ? qtrue : qfalse;
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

		p                       = Info_ValueForKey(s, "p");
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
				str[l - c]                        = '\0';
				level.fireTeams[i].joinOrder[j++] = Q_atoi(str);
				c                                 = l + 1;
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
