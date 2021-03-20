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
 * @file cg_servercmds.c
 * @brief Reliably sequenced text commands sent by the server
 *
 * These are processed at snapshot transition time, so there will definately be
 * a valid snapshot this frame.
 */

#include "cg_local.h"

#ifdef FEATURE_RATING
static void CG_ParseSkillRating(int version)
{
	int i, r;
	int argc = trap_Argc();
	cg.axisProb   = (float)atof(CG_Argv(1));
	cg.alliesProb = (float)atof(CG_Argv(2));

	for (i = 0, r = 3; i < MAX_CLIENTS && r < argc; i++, r++)
	{
		cg.rating[i] = (float)atof(CG_Argv(r));
		// sr
		if (version == 1)
		{
			r++; // skip delta ratings
		}
	}
}
#endif

#ifdef FEATURE_PRESTIGE
/**
 * @brief CG_ParsePrestige
 */
static void CG_ParsePrestige()
{
	int        i = 0;
	const char *s;

	s = CG_Argv(i);
	while (*s)
	{
		cg.prestige[i] = (float)atof(CG_Argv(i + 1));
		i++;
		s = CG_Argv(i);
	}
}
#endif

/**
 * @brief CG_ParseScore
 * @param[in] team
 * @note NOTE: team doesnt actually signify team
 */
static void CG_ParseScore(team_t team)
{
	int i, j, powerups;
	int numScores;
	int offset;

	if (team == TEAM_AXIS)
	{
		cg.numScores = 0;

		cg.teamScores[0] = Q_atoi(CG_Argv(1));
		cg.teamScores[1] = Q_atoi(CG_Argv(2));

		offset = 4;
	}
	else
	{
		offset = 2;
	}

	numScores = Q_atoi(CG_Argv(offset - 1));

	for (j = 0; j < numScores; j++)
	{
		i = cg.numScores;

		cg.scores[i].client       = Q_atoi(CG_Argv(offset + 0 + (j * 7)));
		cg.scores[i].score        = Q_atoi(CG_Argv(offset + 1 + (j * 7)));
		cg.scores[i].ping         = Q_atoi(CG_Argv(offset + 2 + (j * 7)));
		cg.scores[i].time         = Q_atoi(CG_Argv(offset + 3 + (j * 7)));
		powerups                  = Q_atoi(CG_Argv(offset + 4 + (j * 7)));
		cg.scores[i].scoreflags   = Q_atoi(CG_Argv(offset + 5 + (j * 7)));
		cg.scores[i].respawnsLeft = Q_atoi(CG_Argv(offset + 6 + (j * 7)));

		if (cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS)
		{
			cg.scores[i].client = 0;
		}

		cgs.clientinfo[cg.scores[i].client].score    = cg.scores[i].score;
		cgs.clientinfo[cg.scores[i].client].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;

#ifdef FEATURE_RATING
		// skill rating
		if (cgs.skillRating)
		{
			cg.scores[i].rating = cg.rating[i];
		}
#endif

#ifdef FEATURE_PRESTIGE
		if (cgs.prestige)
		{
			cg.scores[i].prestige = cg.prestige[i];
		}
#endif

		cg.numScores++;

		if (cg.intermissionStarted)
		{
			cgs.dbLastScoreReceived = qtrue;
		}
	}
}

#define TEAMINFOARGS 6

/**
 * @brief CG_ParseTeamInfo
 */
static void CG_ParseTeamInfo(void)
{
	int i;
	int client;
	int numSortedTeamPlayers;

	numSortedTeamPlayers = Q_atoi(CG_Argv(1));

	if (numSortedTeamPlayers < 0 || numSortedTeamPlayers >= MAX_CLIENTS)
	{
		CG_Printf("CG_ParseTeamInfo: numSortedTeamPlayers out of range (%i)\n", numSortedTeamPlayers);
		return;
	}

	for (i = 0 ; i < numSortedTeamPlayers ; i++)
	{
		client = Q_atoi(CG_Argv(i * TEAMINFOARGS + 2));

		if (client < 0 || client >= MAX_CLIENTS)
		{
			CG_Printf("CG_ParseTeamInfo: bad client number: %i\n", client);
			return;
		}

		// FIXME: get origin/location from somewhere else and shorten tinfo
		// cg_entities[client]>lerpOrigin works on first view and small distances
		// but isn't updated for unknown reasons when current client is inactive and other players are far away
		// check cent->currentState.origin
		cgs.clientinfo[client].location[0] = Q_atoi(CG_Argv(i * TEAMINFOARGS + 3));
		cgs.clientinfo[client].location[1] = Q_atoi(CG_Argv(i * TEAMINFOARGS + 4));
		cgs.clientinfo[client].location[2] = Q_atoi(CG_Argv(i * TEAMINFOARGS + 5));
		cgs.clientinfo[client].health      = Q_atoi(CG_Argv(i * TEAMINFOARGS + 6));
		cgs.clientinfo[client].powerups    = Q_atoi(CG_Argv(i * TEAMINFOARGS + 7));
	}
}

/**
 * @brief This is called explicitly when the gamestate is first received,
 * and whenever the server updates any serverinfo flagged cvars
 */
void CG_ParseServerinfo(void)
{
	const char *info;
	char       *mapname;

	info = CG_ConfigString(CS_SERVERINFO);

	cg_gameType.integer = cgs.gametype = Q_atoi(Info_ValueForKey(info, "g_gametype"));
	cg_antilag.integer  = cgs.antilag = Q_atoi(Info_ValueForKey(info, "g_antilag"));
	if (!cgs.localServer)
	{
		trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
		trap_Cvar_Set("g_antilag", va("%i", cgs.antilag));
		trap_Cvar_Update(&cg_antilag);
		trap_Cvar_Update(&cg_gameType);
	}
	cgs.timelimit  = (float)atof(Info_ValueForKey(info, "timelimit"));
	cgs.maxclients = Q_atoi(Info_ValueForKey(info, "sv_maxclients"));
	mapname        = Info_ValueForKey(info, "mapname");
	Q_strncpyz(cgs.rawmapname, mapname, sizeof(cgs.rawmapname));
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);

	// prolly should parse all CS_SERVERINFO keys automagically, but I don't want to break anything that might be improperly set for wolf SP, so I'm just parsing MP relevant stuff here
	trap_Cvar_Set("g_redlimbotime", Info_ValueForKey(info, "g_redlimbotime"));
	cg_redlimbotime.integer = Q_atoi(Info_ValueForKey(info, "g_redlimbotime"));
	trap_Cvar_Set("g_bluelimbotime", Info_ValueForKey(info, "g_bluelimbotime"));
	cg_bluelimbotime.integer = Q_atoi(Info_ValueForKey(info, "g_bluelimbotime"));
	cgs.weaponRestrictions   = Q_atoi(Info_ValueForKey(info, "g_heavyWeaponRestriction")) * 0.01f;

	cgs.minclients = Q_atoi(Info_ValueForKey(info, "g_minGameClients")); //  overloaded for ready counts

	cgs.fixedphysics    = Q_atoi(Info_ValueForKey(info, "g_fixedphysics"));
	cgs.fixedphysicsfps = Q_atoi(Info_ValueForKey(info, "g_fixedphysicsfps"));
	cgs.pronedelay      = Q_atoi(Info_ValueForKey(info, "g_pronedelay"));

	cgs.playerHitBoxHeight = Q_atoi(Info_ValueForKey(info, "g_playerHitBoxHeight"));

	// make this available for ingame_callvote
	trap_Cvar_Set("cg_ui_voteFlags", ((authLevel.integer == RL_NONE) ? Info_ValueForKey(info, "voteFlags") : "0"));
}

/**
 * @brief CG_inSVCVARBackupList
 * @param[in] cvar1
 * @return
 */
static qboolean CG_inSVCVARBackupList(const char *cvar1)
{
	int j;

	for (j = 0; j < cg.cvarBackupsCount; ++j)
	{
		if (!Q_stricmp(cg.cvarBackups[j].cvarName, cvar1))
		{
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief CG_UpdateSvCvars
 */
void CG_UpdateSvCvars(void)
{
	const char *info;
	int        i;
	char       *token;
	char       *buffer;

	info = CG_ConfigString(CS_SVCVAR);

	cg.svCvarCount = Q_atoi(Info_ValueForKey(info, "N"));

	for (i = 0; i < cg.svCvarCount; i++)
	{
		// get what is it
		buffer = Info_ValueForKey(info, va("V%i", i));
		// get a mode pf ot
		token              = strtok(buffer, " ");
		cg.svCvars[i].mode = Q_atoi(token);

		token = strtok(NULL, " ");
		Q_strncpyz(cg.svCvars[i].cvarName, token, sizeof(cg.svCvars[0].cvarName));

		token = strtok(NULL, " ");
		Q_strncpyz(cg.svCvars[i].Val1, token, sizeof(cg.svCvars[0].Val1));

		token = strtok(NULL, " ");
		if (token)
		{
			Q_strncpyz(cg.svCvars[i].Val2, token, sizeof(cg.svCvars[0].Val2));
		}

		// FIFO! - only put into backup list if not already in
		if (!CG_inSVCVARBackupList(cg.svCvars[i].cvarName))
		{
			// do a backup
			Q_strncpyz(cg.cvarBackups[cg.cvarBackupsCount].cvarName, cg.svCvars[i].cvarName, sizeof(cg.cvarBackups[0].cvarName));
			trap_Cvar_VariableStringBuffer(cg.svCvars[i].cvarName, cg.cvarBackups[cg.cvarBackupsCount].cvarValue, sizeof(cg.cvarBackups[0].cvarValue));
			cg.cvarBackupsCount++;
		}
	}
}

void CG_ParseSysteminfo(void)
{
	const char *info;

	info = CG_ConfigString(CS_SYSTEMINFO);

/*
    cgs.pmove_fixed = (atoi(Info_ValueForKey(info, "pmove_fixed"))) ? qtrue : qfalse;
    cgs.pmove_msec  = Q_atoi(Info_ValueForKey(info, "pmove_msec"));
    if (cgs.pmove_msec < 8)
    {
        cgs.pmove_msec = 8;
    }
    else if ( cgs.pmove_msec > 33)
    {
        cgs.pmove_msec = 33;
    }
*/
	cgs.sv_fps = Q_atoi(Info_ValueForKey(info, "sv_fps"));

	cgs.sv_cheats = (atoi(Info_ValueForKey(info, "sv_cheats"))) ? qtrue : qfalse;

/*

    bg_evaluategravity = atof(Info_ValueForKey(info, "g_gravity"));
*/
}


/**
 * @brief CG_ParseModInfo
 */
void CG_ParseModInfo(void)
{
	const char *info;

	info = CG_ConfigString(CS_MODINFO);

	cgs.mapVoteMapX = Q_atoi(Info_ValueForKey(info, "X"));
	cgs.mapVoteMapY = Q_atoi(Info_ValueForKey(info, "Y"));
#ifdef FEATURE_RATING
	cgs.skillRating = Q_atoi(Info_ValueForKey(info, "R"));
	if (cgs.skillRating > 1)
	{
		cgs.mapProb = (float)atof(Info_ValueForKey(info, "M"));
	}
#endif
#ifdef FEATURE_PRESTIGE
	cgs.prestige = Q_atoi(Info_ValueForKey(info, "P"));
#endif

#ifdef FEATURE_MULTIVIEW
	cgs.mvAllowed = Q_atoi(Info_ValueForKey(info, "MV"));
#endif
}

/**
 * @brief CG_ParseWarmup
 */
static void CG_ParseWarmup(void)
{
	const char *info;
	int        warmup;

	info   = CG_ConfigString(CS_WARMUP);
	warmup = Q_atoi(info);

	if (warmup == 0 && cg.warmup)
	{

	}
	else if (warmup > 0 && cg.warmup <= 0 && cgs.gamestate != GS_WARMUP)
	{
		if (cg.warmupCount >= 0)
		{
			Pri("^3All players ready!^7\nMatch starting...\n");
			CPri("^3All players ready!^7\nMatch starting...");
		}
	}

	if (cgs.gamestate != GS_WARMUP || cg.warmup > 0)
	{
		cg.warmup = warmup;
	}

	cg.warmupCount++;
}

/**
 * @brief CG_OIDInfoForEntityNum
 * @param[in] num
 * @return
 *
 * @note Unused
oidInfo_t *CG_OIDInfoForEntityNum(int num)
{
    int i;

    for (i = 0; i < MAX_OID_TRIGGERS; i++)
    {
        if (cgs.oidInfo[i].entityNum == num)
        {
            return &cgs.oidInfo[i];
        }
    }

    return NULL;
}
*/

/**
 * @brief CG_ParseOIDInfo
 * @param[in] num
 */
void CG_ParseOIDInfo(int num)
{
	const char *info;
	const char *cs;
	int        index = num - CS_OID_DATA;

	info = CG_ConfigString(num);

	Com_Memset(&cgs.oidInfo[index], 0, sizeof(cgs.oidInfo[0]));

	if (!info || !*info)
	{
		return;
	}

	cs = Info_ValueForKey(info, "s");
	if (cs && *cs)
	{
		cgs.oidInfo[index].spawnflags = Q_atoi(cs);
	}

	cs = Info_ValueForKey(info, "cia");
	if (cs && *cs)
	{
		cgs.oidInfo[index].customimageallies = cgs.gameShaders[atoi(cs)];
	}

	cs = Info_ValueForKey(info, "cix");
	if (cs && *cs)
	{
		cgs.oidInfo[index].customimageaxis = cgs.gameShaders[atoi(cs)];
	}

	cs = Info_ValueForKey(info, "o");
	if (cs && *cs)
	{
		cgs.oidInfo[index].objflags = Q_atoi(cs);
	}

	cs = Info_ValueForKey(info, "e");
	if (cs && *cs)
	{
		cgs.oidInfo[index].entityNum = Q_atoi(cs);
	}

	cs = Info_ValueForKey(info, "n");
	if (cs && *cs)
	{
		Q_strncpyz(cgs.oidInfo[index].name, cs, sizeof(cgs.oidInfo[0].name));
	}

	cs = Info_ValueForKey(info, "x");
	if (cs && *cs)
	{
		cgs.oidInfo[index].origin[0] = Q_atoi(cs);
	}

	cs = Info_ValueForKey(info, "y");
	if (cs && *cs)
	{
		cgs.oidInfo[index].origin[1] = Q_atoi(cs);
	}

	cs = Info_ValueForKey(info, "z");
	if (cs && *cs)
	{
		cgs.oidInfo[index].origin[2] = Q_atoi(cs);
	}
}

/**
 * @brief CG_ParseOIDInfos
 */
void CG_ParseOIDInfos(void)
{
	int i;

	for (i = 0; i < MAX_OID_TRIGGERS; i++)
	{
		CG_ParseOIDInfo(CS_OID_DATA + i);
	}
}

/**
 * @brief CG_ParseWolfinfo
 */
void CG_ParseWolfinfo(void)
{
	int        old_gs = cgs.gamestate;
	const char *info;

	info = CG_ConfigString(CS_WOLFINFO);

	cgs.currentRound       = Q_atoi(Info_ValueForKey(info, "g_currentRound"));
	cgs.nextTimeLimit      = (float)atof(Info_ValueForKey(info, "g_nextTimeLimit"));
	cgs.gamestate          = (gamestate_t)(atoi(Info_ValueForKey(info, "gamestate")));
	cgs.currentCampaign    = Info_ValueForKey(info, "g_currentCampaign");
	cgs.currentCampaignMap = Q_atoi(Info_ValueForKey(info, "g_currentCampaignMap"));

	// Announce game in progress if we are really playing
	if (old_gs != GS_PLAYING && cgs.gamestate == GS_PLAYING)
	{
		if (cg_announcer.integer)
		{
			trap_S_StartLocalSound(cgs.media.countFight, CHAN_ANNOUNCER);
		}

		Pri("^1FIGHT!\n");
		CPri(CG_TranslateString("^1FIGHT!\n"));
	}

	if (!cgs.localServer)
	{
		trap_Cvar_Set("gamestate", va("%i", cgs.gamestate));
	}

	if (old_gs != GS_WARMUP_COUNTDOWN && cgs.gamestate == GS_WARMUP_COUNTDOWN)
	{
		CG_ParseWarmup();
	}
}

/**
 * @brief CG_ParseSpawns
 */
void CG_ParseSpawns(void)
{
	const char *info;
	const char *s;
	int        i;
	int        newteam;

	info = CG_ConfigString(CS_MULTI_INFO);

	s = Info_ValueForKey(info, "s"); // numspawntargets

	if (!s || !strlen(s))
	{
		return;
	}

	// first index is for autopicking
	Q_strncpyz(cg.spawnPoints[0], CG_TranslateString("Auto Pick"), MAX_SPAWNDESC);

	cg.spawnCount = Q_atoi(s) + 1;

	for (i = 1; i < cg.spawnCount; i++)
	{
		info = CG_ConfigString(CS_MULTI_SPAWNTARGETS + i - 1);

		s = Info_ValueForKey(info, "s"); // spawn_targ

		if (!s || !strlen(s))
		{
			return;
		}

		Q_strncpyz(cg.spawnPoints[i], CG_TranslateString(s), MAX_SPAWNDESC);

		s = Info_ValueForKey(info, "x");
		if (!s || !strlen(s))
		{
			return;
		}
		cg.spawnCoordsUntransformed[i][0] = cg.spawnCoords[i][0] = (float)atof(s);

		s = Info_ValueForKey(info, "y");
		if (!s || !strlen(s))
		{
			return;
		}
		cg.spawnCoordsUntransformed[i][1] = cg.spawnCoords[i][1] = (float)atof(s);

		if (cgs.ccLayers)
		{
			s = Info_ValueForKey(info, "z");
			if (!s || !strlen(s))
			{
				return;
			}
			cg.spawnCoordsUntransformed[i][2] = cg.spawnCoords[i][2] = (float)atof(s);
		}

		CG_TransformToCommandMapCoord(&cg.spawnCoords[i][0], &cg.spawnCoords[i][1]);

		s = Info_ValueForKey(info, "t");

		newteam = Q_atoi(s);
		if (cg.spawnTeams[i] != newteam)
		{
			cg.spawnTeams_old[i]        = cg.spawnTeams[i];
			cg.spawnTeams_changeTime[i] = cg.time;
			cg.spawnTeams[i]            = (team_t)newteam;
		}

		s                       = Info_ValueForKey(info, "c");
		cg.spawnPlayerCounts[i] = Q_atoi(s);
	}
}

/**
 * @brief CG_ParseScreenFade
 */
static void CG_ParseScreenFade(void)
{
	const char *info;
	char       *token;

	info = CG_ConfigString(CS_SCREENFADE);

	token         = COM_Parse((char **)&info);
	cgs.fadeAlpha = (float)atof(token);

	token             = COM_Parse((char **)&info);
	cgs.fadeStartTime = Q_atoi(token);
	token             = COM_Parse((char **)&info);
	cgs.fadeDuration  = Q_atoi(token);

	if (cgs.fadeStartTime + cgs.fadeDuration < cg.time)
	{
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
}

/**
 * @brief CG_ParseFog
 * @note :
 *  float near dist
 *  float far dist
 *  float density
 *  float[3] r,g,b
 *  int     time
 */
static void CG_ParseFog(void)
{
	const char *info;
	char       *token;
	float      ne, fa, r, g, b, density;
	int        time;

	info = CG_ConfigString(CS_FOGVARS);

	token   = COM_Parse((char **)&info);
	ne      = (float)atof(token);
	token   = COM_Parse((char **)&info);
	fa      = (float)atof(token);
	token   = COM_Parse((char **)&info);
	density = (float)atof(token);
	token   = COM_Parse((char **)&info);
	r       = (float)atof(token);
	token   = COM_Parse((char **)&info);
	g       = (float)atof(token);
	token   = COM_Parse((char **)&info);
	b       = (float)atof(token);
	token   = COM_Parse((char **)&info);
	time    = Q_atoi(token);

	if (fa != 0.f)        // far of '0' from a target_fog means "return to map fog"
	{
		trap_R_SetFog(FOG_SERVER, (int)ne, (int)fa, r, g, b, density + .1f);
		trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_SERVER, time, 0, 0, 0, 0);
	}
	else
	{
		trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, time, 0, 0, 0, 0);
	}
}

/**
 * @brief CG_ParseGlobalFog
 */
static void CG_ParseGlobalFog(void)
{
	const char *info;
	char       *token;
	qboolean   restore;
	int        duration;

	info = CG_ConfigString(CS_GLOBALFOGVARS);

	token    = COM_Parse((char **)&info);
	restore  = (float)atoi(token);
	token    = COM_Parse((char **)&info);
	duration = Q_atoi(token);

	if (restore)
	{
		trap_R_SetGlobalFog(qtrue, duration, 0.f, 0.f, 0.f, 0);
	}
	else
	{
		float r, g, b, depthForOpaque;

		token          = COM_Parse((char **)&info);
		r              = (float)atof(token);
		token          = COM_Parse((char **)&info);
		g              = (float)atof(token);
		token          = COM_Parse((char **)&info);
		b              = (float)atof(token);
		token          = COM_Parse((char **)&info);
		depthForOpaque = (float)atof(token);

		trap_R_SetGlobalFog(qfalse, duration, r, g, b, depthForOpaque);
	}
}

/**
 * @brief Parse server version info (for demo playback compatibility)
 * @param[in] pszVersionInfo
 */
void CG_ParseServerVersionInfo(const char *pszVersionInfo)
{
	// This will expand to a tokenized string, eventually but for
	// now we only need to worry about 1 number :)
	cgs.game_versioninfo = Q_atoi(pszVersionInfo);
}

/**
 * @brief Parse reinforcement offsets
 * @param[in] pszReinfSeedString
 */
void CG_ParseReinforcementTimes(const char *pszReinfSeedString)
{
	const char *tmp = pszReinfSeedString, *tmp2;
	int        i, j, dwOffset[TEAM_NUM_TEAMS];

#define GETVAL(x, y) if ((tmp = strchr(tmp, ' ')) == NULL) { return; } x = Q_atoi(++tmp) / y;

	dwOffset[TEAM_ALLIES] = Q_atoi(pszReinfSeedString) >> REINF_BLUEDELT;
	GETVAL(dwOffset[TEAM_AXIS], (1 << REINF_REDDELT));
	tmp2 = tmp;

	for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
	{
		tmp = tmp2;
		for (j = 0; j < MAX_REINFSEEDS; j++)
		{
			if (j == dwOffset[i])
			{
				GETVAL(cgs.aReinfOffset[i], aReinfSeeds[j]);
				cgs.aReinfOffset[i] *= 1000;
				break;
			}
			if ((tmp = strchr(tmp, ' ')) == NULL)
			{
				return;
			}
			++tmp;
		}
	}
}

/**
 * @brief Called on load to set the initial values from configure strings
 */
void CG_SetConfigValues(void)
{
	cgs.levelStartTime        = Q_atoi(CG_ConfigString(CS_LEVEL_START_TIME));
	cgs.intermissionStartTime = Q_atoi(CG_ConfigString(CS_INTERMISSION_START_TIME));
	cg.warmup                 = Q_atoi(CG_ConfigString(CS_WARMUP));

	// set all of this crap in cgs - it won't be set if it doesn't
	// change, otherwise.  consider:
	// vote was called 5 minutes ago for 'Match Reset'.  you connect.
	// you're sent that value for CS_VOTE_STRING, but ignore it, so
	// you have nothing to use if another 'Match Reset' vote is called
	// (no update will be sent because the string will be the same.)

	cgs.voteTime = Q_atoi(CG_ConfigString(CS_VOTE_TIME));
	cgs.voteYes  = Q_atoi(CG_ConfigString(CS_VOTE_YES));
	cgs.voteNo   = Q_atoi(CG_ConfigString(CS_VOTE_NO));
	Q_strncpyz(cgs.voteString, CG_ConfigString(CS_VOTE_STRING), sizeof(cgs.voteString));

	cg.teamFirstBlood = Q_atoi(CG_ConfigString(CS_FIRSTBLOOD));
	// yes, the order is this way on purpose. not my fault!
	cg.teamWonRounds[1] = Q_atoi(CG_ConfigString(CS_ROUNDSCORES1));
	cg.teamWonRounds[0] = Q_atoi(CG_ConfigString(CS_ROUNDSCORES2));

	CG_ParseServerVersionInfo(CG_ConfigString(CS_VERSIONINFO));
	CG_ParseReinforcementTimes(CG_ConfigString(CS_REINFSEEDS));
}

/**
 * @brief CG_ShaderStateChanged
 */
void CG_ShaderStateChanged(void)
{
	char       originalShader[MAX_QPATH];
	char       newShader[MAX_QPATH];
	char       timeOffset[16];
	const char *o;
	char       *n, *t;

	o = CG_ConfigString(CS_SHADERSTATE);

	while (o && *o)
	{
		n = strstr(o, "=");
		if (n && *n)
		{
			strncpy(originalShader, o, n - o);
			originalShader[n - o] = 0;
			n++;
			t = strstr(n, ":");
			if (t && *t)
			{
				strncpy(newShader, n, t - n);
				newShader[t - n] = 0;
			}
			else
			{
				break;
			}
			t++;
			o = strstr(t, "@");
			if (o)
			{
				strncpy(timeOffset, t, o - t);
				timeOffset[o - t] = 0;
				o++;
				trap_R_RemapShader(cgs.gameShaderNames[atoi(originalShader)],
				                   cgs.gameShaderNames[atoi(newShader)],
				                   timeOffset);
			}
		}
		else
		{
			break;
		}
	}
}

/**
 * @brief CG_ChargeTimesChanged
 */
void CG_ChargeTimesChanged(void)
{
	const char *info;

	info = CG_ConfigString(CS_CHARGETIMES);

	cg.soldierChargeTime[0]   = Q_atoi(Info_ValueForKey(info, "x0"));
	cg.soldierChargeTime[1]   = Q_atoi(Info_ValueForKey(info, "a0"));
	cg.medicChargeTime[0]     = Q_atoi(Info_ValueForKey(info, "x1"));
	cg.medicChargeTime[1]     = Q_atoi(Info_ValueForKey(info, "a1"));
	cg.engineerChargeTime[0]  = Q_atoi(Info_ValueForKey(info, "x2"));
	cg.engineerChargeTime[1]  = Q_atoi(Info_ValueForKey(info, "a2"));
	cg.fieldopsChargeTime[0]  = Q_atoi(Info_ValueForKey(info, "x3"));
	cg.fieldopsChargeTime[1]  = Q_atoi(Info_ValueForKey(info, "a3"));
	cg.covertopsChargeTime[0] = Q_atoi(Info_ValueForKey(info, "x4"));
	cg.covertopsChargeTime[1] = Q_atoi(Info_ValueForKey(info, "a4"));
}

/**
 * @brief CG_TeamRestrictionsChanged
 */
void CG_TeamRestrictionsChanged(void)
{
	const char *info;
	int        i;

	info = CG_ConfigString(CS_TEAMRESTRICTIONS);

	for (i = 0; i < NUM_PLAYER_CLASSES; i++)
	{
		Q_strncpyz(cg.maxPlayerClasses[i], Info_ValueForKey(info, va("c%i", i)), sizeof(cg.maxPlayerClasses[i]));
	}

	Q_strncpyz(cg.maxMortars, Info_ValueForKey(info, "w0"), sizeof(cg.maxMortars));
	Q_strncpyz(cg.maxFlamers, Info_ValueForKey(info, "w1"), sizeof(cg.maxFlamers));
	Q_strncpyz(cg.maxMachineguns, Info_ValueForKey(info, "w2"), sizeof(cg.maxMachineguns));
	Q_strncpyz(cg.maxRockets, Info_ValueForKey(info, "w3"), sizeof(cg.maxRockets));
	Q_strncpyz(cg.maxRiflegrenades, Info_ValueForKey(info, "w4"), sizeof(cg.maxRiflegrenades));
	cg.maxPlayers = Q_atoi(Info_ValueForKey(info, "m"));
}

#define SETSKILL(skill, string) sscanf(string, "%i,%i,%i,%i", &GetSkillTableData(skill)->skillLevels[1], &GetSkillTableData(skill)->skillLevels[2], &GetSkillTableData(skill)->skillLevels[3], &GetSkillTableData(skill)->skillLevels[4])

/**
 * @brief CG_SkillLevelsChanged
 */
void CG_SkillLevelsChanged(void)
{
	const char *info;

	info = CG_ConfigString(CS_UPGRADERANGE);

	SETSKILL(SK_BATTLE_SENSE, Info_ValueForKey(info, "bs"));
	SETSKILL(SK_EXPLOSIVES_AND_CONSTRUCTION, Info_ValueForKey(info, "en"));
	SETSKILL(SK_FIRST_AID, Info_ValueForKey(info, "md"));
	SETSKILL(SK_SIGNALS, Info_ValueForKey(info, "fo"));
	SETSKILL(SK_LIGHT_WEAPONS, Info_ValueForKey(info, "lw"));
	SETSKILL(SK_HEAVY_WEAPONS, Info_ValueForKey(info, "sd"));
	SETSKILL(SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, Info_ValueForKey(info, "cv"));
}

/**
 * @brief CG_ConfigStringModified
 */
static void CG_ConfigStringModified(void)
{
	int num;

	num = Q_atoi(CG_Argv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// do something with it if necessary
	switch (num)
	{
	case CS_SVCVAR:
		CG_UpdateSvCvars();
		break;
	case CS_MUSIC:
		CG_StartMusic();
		break;
	case CS_MUSIC_QUEUE:
		CG_QueueMusic();
		break;
	case CS_SERVERINFO:
		CG_ParseServerinfo();
		break;
	case CS_WARMUP:
		CG_ParseWarmup();
		break;
	case CS_WOLFINFO:
		CG_ParseWolfinfo();
		break;
	case CS_FIRSTBLOOD:
		cg.teamFirstBlood = Q_atoi(CG_ConfigString(num));
		break;
	case CS_ROUNDSCORES1:
		cg.teamWonRounds[1] = Q_atoi(CG_ConfigString(num));
		break;
	case CS_ROUNDSCORES2:
		cg.teamWonRounds[0] = Q_atoi(CG_ConfigString(num));
		break;
	case CS_VERSIONINFO:
		CG_ParseServerVersionInfo(CG_ConfigString(num));           // set versioning info for older demo playback
		break;
	case CS_REINFSEEDS:
		CG_ParseReinforcementTimes(CG_ConfigString(num));          // set reinforcement times for each team
		break;
	case CS_LEVEL_START_TIME:
		cgs.levelStartTime = Q_atoi(CG_ConfigString(num));
		break;
	case CS_INTERMISSION_START_TIME:
		cgs.intermissionStartTime = Q_atoi(CG_ConfigString(num));
		break;
	case CS_VOTE_TIME:
		cgs.voteTime     = Q_atoi(CG_ConfigString(num));
		cgs.voteModified = qtrue;
		break;
	case CS_VOTE_YES:
		cgs.voteYes      = Q_atoi(CG_ConfigString(num));
		cgs.voteModified = qtrue;
		break;
	case CS_VOTE_NO:
		cgs.voteNo       = Q_atoi(CG_ConfigString(num));
		cgs.voteModified = qtrue;
		break;
	case CS_VOTE_STRING:
		Q_strncpyz(cgs.voteString, CG_ConfigString(num), sizeof(cgs.voteString));
		break;
	case CS_INTERMISSION:
		cg.intermissionStarted = (qboolean)(atoi(CG_ConfigString(num)));
		break;
	case CS_SCREENFADE:
		CG_ParseScreenFade();
		break;
	case CS_FOGVARS:
		CG_ParseFog();
		break;
	case CS_GLOBALFOGVARS:
		CG_ParseGlobalFog();
		break;
	case CS_SHADERSTATE:
		CG_ShaderStateChanged();
		break;
	case CS_CHARGETIMES:
		CG_ChargeTimesChanged();
		break;
	case CS_TEAMRESTRICTIONS:
		CG_TeamRestrictionsChanged();
		break;
	case CS_UPGRADERANGE:
		CG_SkillLevelsChanged();
		break;
	case CS_SKYBOXORG:
		CG_ParseSkyBox();
		break;
	case CS_ALLIED_MAPS_XP:
	case CS_AXIS_MAPS_XP:
		CG_ParseTeamXPs(num - CS_AXIS_MAPS_XP);
		break;
	case CS_FILTERCAMS:
		cg.filtercams = Q_atoi(CG_ConfigString(num)) ? qtrue : qfalse;
		break;
	case CS_MODINFO:
		CG_ParseModInfo();
		break;
	case CS_SYSTEMINFO:
		CG_ParseSysteminfo();
		break;

	default:
		if (num >= CS_MULTI_SPAWNTARGETS && num < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS)
		{
			CG_ParseSpawns();
		}
		else if (num >= CS_MODELS && num < CS_MODELS + MAX_MODELS)
		{
			cgs.gameModels[num - CS_MODELS] = trap_R_RegisterModel(CG_ConfigString(num));

			if (!cgs.gameModels[num - CS_MODELS])
			{
				CG_Printf("^3Warning: Register server model '%s' failed. No valid file in paths.\n", CG_ConfigString(num));
			}
		}
		else if (num >= CS_SOUNDS && num < CS_SOUNDS + MAX_SOUNDS)
		{
			const char *str = CG_ConfigString(num);

			if (str[0] != '*')       // player specific sounds don't register here
			{   // register sound scripts seperately
				if (!strstr(str, ".wav") && !strstr(str, ".ogg"))
				{
					CG_SoundScriptPrecache(str);
				}
				else
				{
					cgs.gameSounds[num - CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);      //FIXME: add a compress flag?
				}
			}
		}
		else if (num >= CS_SHADERS && num < CS_SHADERS + MAX_CS_SHADERS)
		{
			const char *str = CG_ConfigString(num);

			cgs.gameShaders[num - CS_SHADERS] = str[0] == '*' ? trap_R_RegisterShader(str + 1) : trap_R_RegisterShaderNoMip(str);
			Q_strncpyz(cgs.gameShaderNames[num - CS_SHADERS], str[0] == '*' ? str + 1 : str, MAX_QPATH);
		}
		else if (num >= CS_SKINS && num < CS_SKINS + MAX_CS_SKINS)
		{
			cgs.gameModelSkins[num - CS_SKINS] = trap_R_RegisterSkin(CG_ConfigString(num));
		}
		else if (num >= CS_CHARACTERS && num < CS_CHARACTERS + MAX_CHARACTERS)
		{
			const char *str = CG_ConfigString(num);

			if (!BG_FindCharacter(str))
			{
				cgs.gameCharacters[num - CS_CHARACTERS] = BG_FindFreeCharacter(str);

				Q_strncpyz(cgs.gameCharacters[num - CS_CHARACTERS]->characterFile, str, sizeof(cgs.gameCharacters[num - CS_CHARACTERS]->characterFile));

				if (!CG_RegisterCharacter(str, cgs.gameCharacters[num - CS_CHARACTERS]))
				{
					CG_Error("ERROR: CG_ConfigStringModified: failed to load character file '%s'\n", str);
				}
			}
		}
		else if (num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS)
		{
			CG_NewClientInfo(num - CS_PLAYERS);
		}
		else if (num >= CS_DLIGHTS && num < CS_DLIGHTS + MAX_DLIGHT_CONFIGSTRINGS)
		{
			// FIXME - dlight changes ignored!
		}
		else if (num >= CS_FIRETEAMS && num < CS_FIRETEAMS + MAX_FIRETEAMS)
		{
			CG_ParseFireteams();
		}
		else if (num >= CS_TAGCONNECTS && num < CS_TAGCONNECTS + MAX_TAGCONNECTS)
		{
			CG_ParseTagConnect(num);
		}
		else if (num >= CS_OID_DATA && num < CS_OID_DATA + MAX_OID_TRIGGERS)
		{
			CG_ParseOIDInfo(num);
		}
		break;
	}
}

/**
 * @brief CG_AddToTeamChat
 * @param[in] str
 * @param[in] clientnum
 */
static void CG_AddToTeamChat(const char *str, int clientnum) // FIXME: add disguise?
{
	int  len;
	char *p, *ls;
	char lastcolor;
	int  chatHeight;
	int  chatWidth;

	// -1 is sent when console is chatting
	if (clientnum < -1 || clientnum >= MAX_CLIENTS) // FIXME: never return for console chat?
	{
		return;
	}

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
	{
		chatHeight = (cgs.gamestate == GS_INTERMISSION) ? TEAMCHAT_HEIGHT : cg_teamChatHeight.integer;
	}
	else
	{
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0 || cg_teamChatTime.integer <= 0) // FIXME: never return for console chat?
	{
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len       = 0;
	chatWidth = (cgs.gamestate == GS_INTERMISSION) ? TEAMCHAT_WIDTH + 30 : TEAMCHAT_WIDTH;

	p  = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str)
	{
		if (len > chatWidth - 1)
		{
			if (ls)
			{
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
			// console chat
			if (clientnum == -1)
			{
				cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = TEAM_SPECTATOR;
			}
			else
			{
				cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = cgs.clientinfo[clientnum].team;
			}
			cgs.teamChatPos++;
			p    = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p   = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len  = 0;
			ls   = NULL;
		}

		if (Q_IsColorString(str))
		{
			*p++      = *str++;
			lastcolor = *str;
			*p++      = *str++;
			continue;
		}
		if (*str == ' ')
		{
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	// console chat
	if (clientnum == -1)
	{
		cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = TEAM_SPECTATOR;
	}
	else
	{
		cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = cgs.clientinfo[clientnum].team;
	}

	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if (cgs.teamChatPos - cgs.teamLastChatPos > chatHeight)
	{
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
	}
}


/*
=======================
CG_AddToNotify
=======================
*/
void CG_AddToNotify(const char *str)
{
	int   len;
	char  *p, *ls;
	int   lastcolor;
	int   chatHeight;
	float notifytime;
	char  var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("con_notifytime", var, sizeof(var));
	notifytime = atof(var) * 1000;

	chatHeight = NOTIFY_HEIGHT;

	if (chatHeight <= 0 || notifytime <= 0)
	{
		// team chat disabled, dump into normal chat
		cgs.notifyPos = cgs.notifyLastPos = 0;
		return;
	}

	len = 0;

	p  = cgs.notifyMsgs[cgs.notifyPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str)
	{
		if (len > NOTIFY_WIDTH - 1 || (*str == '\n' && (*(str + 1) != 0)))
		{
			if (ls)
			{
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			//cgs.notifyMsgTimes[cgs.notifyPos % chatHeight] = cg.time;

			cgs.notifyPos++;
			p    = cgs.notifyMsgs[cgs.notifyPos % chatHeight];
			*p   = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len  = 0;
			ls   = NULL;
		}

		if (Q_IsColorString(str))
		{
			*p++      = *str++;
			lastcolor = *str;
			*p++      = *str++;
			continue;
		}
		if (*str == ' ')
		{
			ls = p;
		}
		while (*str == '\n')
		{
			str++;
		}

		if (*str)
		{
			*p++ = *str++;
			len++;
		}
	}
	*p = 0;

	//cgs.notifyMsgTimes[cgs.notifyPos % chatHeight] = cg.time;
	cgs.notifyPos++;

	if (cgs.notifyPos - cgs.notifyLastPos > chatHeight)
	{
		cgs.notifyLastPos = cgs.notifyPos - chatHeight;
	}
}

/**
 * @brief The server has issued a map_restart, so the next snapshot
 * is completely new and should not be interpolated to.
 *
 * A tournement restart will clear everything, but doesn't
 * require a reload of all the media
 */
static void CG_MapRestart(void)
{
	if (cg_showmiss.integer)
	{
		CG_Printf("CG_MapRestart\n");
	}

	Com_Memset(&cg.lastWeapSelInBank[0], 0, MAX_WEAP_BANKS_MP * sizeof(int)); // clear weapon bank selections

	cg.numbufferedSoundScripts = 0;

	cg.centerPrintTime = 0; // reset centerprint counter so previous messages don't re-appear
	cg.cursorHintFade  = 0; // reset cursor hint timer

	// Reset complaint system
	cgs.complaintClient  = -1;
	cgs.complaintEndTime = 0;

	// init crosshairMine + Dyna
	cg.crosshairMine = -1;
	cg.crosshairDyna = -1;

	// init objective indicator
	cg.flagIndicator   = 0;
	cg.redFlagCounter  = 0;
	cg.blueFlagCounter = 0;

	CG_LimboPanel_RequestObjective();

	// clear zoom (so no warpies)
	cg.zoomedBinoc = qfalse;
	cg.zoomed      = qfalse;
	cg.zoomTime    = 0;
	cg.zoomval     = 0;

	cgs.complaintEndTime          = 0;
	cgs.invitationEndTime         = 0;
	cgs.applicationEndTime        = 0;
	cgs.propositionEndTime        = 0;
	cgs.autoFireteamEndTime       = 0;
	cgs.autoFireteamCreateEndTime = 0;

	// reset fog to world fog (if present)
	trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0);

	// clear pmext
	Com_Memset(&cg.pmext, 0, sizeof(cg.pmext));

	cg.pmext.bAutoReload = (qboolean)(cg_autoReload.integer > 0);

	numSplinePaths     = 0;
	numPathCorners     = 0;
	cg.numOIDtriggers2 = 0;

	cgs.fadeStartTime = 0;
	cgs.fadeAlpha     = 0;
	trap_Cvar_Set("cg_letterbox", "0");

	cgs.gamestate = GS_INITIALIZE;

	CG_ParseWolfinfo();

	CG_ParseEntitiesFromString();

	CG_LoadObjectiveData();

	CG_InitLocalEntities();
	CG_InitMarkPolys();

	cg.editingSpeakers = qfalse;

	BG_BuildSplinePaths();

	InitSmokeSprites();

	// particles
	CG_ClearParticles();

	CG_ClearFlameChunks();
	CG_SoundInit();

	cg.intermissionStarted = qfalse;
	cg.lightstylesInited   = qfalse;
	cg.mapRestart          = qtrue;
	cg.timelimitWarnings   = 0;
	cgs.voteTime           = 0;
	cgs.dumpStatsTime      = 0;

	CG_StartMusic();

	trap_S_ClearLoopingSounds();
	trap_S_ClearSounds(qfalse);

	trap_R_ClearDecals();

	cg.latchAutoActions  = qfalse;
	cg.latchVictorySound = qfalse;

	// we really should clear more parts of cg here and stop sounds
	cg.v_dmg_time   = 0;
	cg.v_noFireTime = 0;
	cg.v_fireTime   = 0;

	cg.filtercams = Q_atoi(CG_ConfigString(CS_FILTERCAMS)) ? qtrue : qfalse;

	CG_ChargeTimesChanged();

	CG_ParseFireteams();

	CG_ParseOIDInfos();

	CG_InitPM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

	trap_Cvar_Set("cg_thirdPerson", "0");

#ifdef FEATURE_RATING
	// default scoreboard
	if (cgs.skillRating > 0 && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
	{
		trap_Cvar_Set("cg_scoreboard", "1"); // SCOREBOARD_SR
	}
	else
#endif
	{
		trap_Cvar_Set("cg_scoreboard", "0"); // SCOREBOARD_XP
	}
}

#define MAX_VOICEFILES      8

// we've got some really big VO files now
#define MAX_VOICEFILESIZE   32768
#define MAX_VOICECHATS      272
// NOTE: If we're worried about space - do we really need 96 possible sounds for any one chat?
//          I think this is used to allow multiple sound clips for one command, so do we need 96 available selection sounds?
#define MAX_VOICESOUNDS     32

#define MAX_CHATSIZE        64

typedef struct voiceChat_s
{
	char id[64];
	int numSounds;
	sfxHandle_t sounds[MAX_VOICESOUNDS];
	char chats[MAX_VOICESOUNDS][MAX_CHATSIZE];
	qhandle_t sprite[MAX_VOICESOUNDS];
} voiceChat_t;

typedef struct voiceChatList_s
{
	char name[64];
	int gender;
	int numVoiceChats;
	voiceChat_t voiceChats[MAX_VOICECHATS];
} voiceChatList_t;

voiceChatList_t voiceChatLists[MAX_VOICEFILES];

/**
 * @brief CG_ParseVoiceChats
 * @param[in] filename
 * @param[in,out] voiceChatList
 * @param[in] maxVoiceChats
 * @return
 */
int CG_ParseVoiceChats(const char *filename, voiceChatList_t *voiceChatList, int maxVoiceChats)
{
	int          len, i;
	int          current = 0;
	fileHandle_t f;
	char         buf[MAX_VOICEFILESIZE];
	char         **p, *ptr;
	char         *token;
	voiceChat_t  *voiceChats;
	qboolean     compress = qtrue;

	if (cg_buildScript.integer)
	{
		compress = qfalse;
	}

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (!f)
	{
		trap_Print(va(S_COLOR_RED "voice chat file not found: %s\n", filename));
		return qfalse;
	}
	if (len >= MAX_VOICEFILESIZE)
	{
		trap_Print(va(S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i", filename, len, MAX_VOICEFILESIZE));
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	ptr = buf;
	p   = &ptr;

	Com_sprintf(voiceChatList->name, sizeof(voiceChatList->name), "%s", filename);
	voiceChats = voiceChatList->voiceChats;
	for (i = 0; i < maxVoiceChats; i++)
	{
		voiceChats[i].id[0] = 0;
	}
	token = COM_ParseExt(p, qtrue);
	if (!token[0])
	{
		return qtrue;
	}
	if (!Q_stricmp(token, "female"))
	{
		voiceChatList->gender = GENDER_FEMALE;
	}
	else if (!Q_stricmp(token, "male"))
	{
		voiceChatList->gender = GENDER_MALE;
	}
	else if (!Q_stricmp(token, "neuter"))
	{
		voiceChatList->gender = GENDER_NEUTER;
	}
	else
	{
		trap_Print(va(S_COLOR_RED "expected gender not found in voice chat file: %s\n", filename));
		return qfalse;
	}

	// setting before call so we can load multiple files into one list
	// - if you really want to be able to load multiple files, you should take out the loop
	//   above that clears out all the commands "voiceChats[i].id[0] = 0;"
	//   We don't even want the MP voice chats in SP, so no need anyway
	voiceChatList->numVoiceChats = 0;
	while (1)
	{
		token = COM_ParseExt(p, qtrue);
		if (!token[0])
		{
			return qtrue;
		}

		Com_sprintf(voiceChats[voiceChatList->numVoiceChats].id, sizeof(voiceChats[voiceChatList->numVoiceChats].id), "%s", token);
		token = COM_ParseExt(p, qtrue);
		if (Q_stricmp(token, "{"))
		{
			trap_Print(va(S_COLOR_RED "expected { found %s in voice chat file: %s\n", token, filename));
			return qfalse;
		}
		voiceChats[voiceChatList->numVoiceChats].numSounds = 0;
		current                                            = voiceChats[voiceChatList->numVoiceChats].numSounds;

		while (1)
		{
			token = COM_ParseExt(p, qtrue);
			if (!token[0])
			{
				return qtrue;
			}
			if (!Q_stricmp(token, "}"))
			{
				break;
			}
			voiceChats[voiceChatList->numVoiceChats].sounds[current] = trap_S_RegisterSound(token, compress);
			token                                                    = COM_ParseExt(p, qtrue);
			if (!token[0])
			{
				return qtrue;
			}
			Com_sprintf(voiceChats[voiceChatList->numVoiceChats].chats[current], MAX_CHATSIZE, "%s", token);

			// Specify sprite shader to show above player's head
			token = COM_ParseExt(p, qfalse);
			if (!Q_stricmp(token, "}") || !token[0])
			{
				voiceChats[voiceChatList->numVoiceChats].sprite[current] = trap_R_RegisterShader("sprites/voiceChat");
				COM_RestoreParseSession(p);
			}
			else
			{
				voiceChats[voiceChatList->numVoiceChats].sprite[current] = trap_R_RegisterShader(token);
				if (voiceChats[voiceChatList->numVoiceChats].sprite[current] == 0)
				{
					voiceChats[voiceChatList->numVoiceChats].sprite[current] = trap_R_RegisterShader("sprites/voiceChat");
				}
			}

			voiceChats[voiceChatList->numVoiceChats].numSounds++;
			current = voiceChats[voiceChatList->numVoiceChats].numSounds;

			if (voiceChats[voiceChatList->numVoiceChats].numSounds >= MAX_VOICESOUNDS)
			{
				break;
			}
		}

		voiceChatList->numVoiceChats++;
		if (voiceChatList->numVoiceChats >= maxVoiceChats)
		{
			return qtrue;
		}
	}
}

/**
 * @brief CG_LoadVoiceChats
 */
void CG_LoadVoiceChats(void)
{
	voiceChatLists[0].numVoiceChats = 0;
	voiceChatLists[1].numVoiceChats = 0;

	CG_ParseVoiceChats("scripts/wm_axis_chat.voice", &voiceChatLists[0], MAX_VOICECHATS);
	CG_ParseVoiceChats("scripts/wm_allies_chat.voice", &voiceChatLists[1], MAX_VOICECHATS);
}

/**
 * @brief CG_GetVoiceChat
 * @param[in] voiceChatList
 * @param[in] id
 * @param[out] snd
 * @param[out] sprite
 * @param[out] chat
 * @return
 */
int CG_GetVoiceChat(voiceChatList_t *voiceChatList, const char *id, sfxHandle_t *snd, qhandle_t *sprite, char **chat)
{
	int i, rnd;

	for (i = 0; i < voiceChatList->numVoiceChats; i++)
	{
		if (!Q_stricmp(id, voiceChatList->voiceChats[i].id))
		{
			rnd     = (int)(random() * voiceChatList->voiceChats[i].numSounds);
			*snd    = voiceChatList->voiceChats[i].sounds[rnd];
			*sprite = voiceChatList->voiceChats[i].sprite[rnd];
			*chat   = voiceChatList->voiceChats[i].chats[rnd];
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief CG_VoiceChatListForClient
 * @param[in] clientNum
 * @return
 */
voiceChatList_t *CG_VoiceChatListForClient(int clientNum)
{
	if (cgs.clientinfo[clientNum].team == TEAM_AXIS)
	{
		return &voiceChatLists[0];
	}
	else
	{
		return &voiceChatLists[1];
	}
}

#define MAX_VOICECHATBUFFER 32

typedef struct bufferedVoiceChat_s
{
	int clientNum;
	sfxHandle_t snd;
	qhandle_t sprite;
	int voiceOnly;
	char cmd[MAX_SAY_TEXT];
	char message[MAX_SAY_TEXT];
	vec3_t origin;
} bufferedVoiceChat_t;

bufferedVoiceChat_t voiceChatBuffer[MAX_VOICECHATBUFFER];

/**
 * @brief CG_PlayVoiceChat
 * @param[in,out] vchat
 */
void CG_PlayVoiceChat(bufferedVoiceChat_t *vchat)
{
	if (cg_voiceChats.integer)
	{
		trap_S_StartLocalSound(vchat->snd, CHAN_VOICE);

		// don't show icons for the HQ (clientnum -1)
		if (vchat->clientNum != -1)
		{
			// Show icon above head
			if (vchat->clientNum == cg.snap->ps.clientNum)
			{
				cg.predictedPlayerEntity.voiceChatSprite = vchat->sprite;
				if (vchat->sprite == cgs.media.voiceChatShader)
				{
					cg.predictedPlayerEntity.voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer;
				}
				else
				{
					cg.predictedPlayerEntity.voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer * 2;
				}
			}
			else
			{
				cg_entities[vchat->clientNum].voiceChatSprite = vchat->sprite;
				VectorCopy(vchat->origin, cg_entities[vchat->clientNum].lerpOrigin);
				if (vchat->sprite == cgs.media.voiceChatShader)
				{
					cg_entities[vchat->clientNum].voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer;
				}
				else
				{
					cg_entities[vchat->clientNum].voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer * 2;
				}
			}
		}

	}
	if (!vchat->voiceOnly && cg_voiceText.integer)
	{
		CG_AddToTeamChat(vchat->message, vchat->clientNum);
		CG_Printf("[skipnotify]%s\n", vchat->message);
		CG_WriteToLog("%s\n", vchat->message);
	}
	voiceChatBuffer[cg.voiceChatBufferOut].snd = 0;
}

/**
 * @brief CG_PlayBufferedVoiceChats
 */
void CG_PlayBufferedVoiceChats(void)
{
	if (cg.voiceChatTime < cg.time)
	{
		if (cg.voiceChatBufferOut != cg.voiceChatBufferIn && voiceChatBuffer[cg.voiceChatBufferOut].snd)
		{
			CG_PlayVoiceChat(&voiceChatBuffer[cg.voiceChatBufferOut]);

			cg.voiceChatBufferOut = (cg.voiceChatBufferOut + 1) % MAX_VOICECHATBUFFER;
			cg.voiceChatTime      = cg.time + 1000;
		}
	}
}

/**
 * @brief CG_AddBufferedVoiceChat
 * @param[in,out] vchat
 * @note FIXME: Put this on a cvar to choose which to uses
 */
void CG_AddBufferedVoiceChat(bufferedVoiceChat_t *vchat)
{
	// new system doesn't buffer but overwrites vchats FIXME put this on a cvar to choose which to use
	Com_Memcpy(&voiceChatBuffer[0], vchat, sizeof(bufferedVoiceChat_t));
	cg.voiceChatBufferIn = 0;
	CG_PlayVoiceChat(&voiceChatBuffer[0]);
}

/**
 * @brief CG_VoiceChatLocal
 * @param[in] mode
 * @param[in] voiceOnly
 * @param[in] clientNum
 * @param[in] color
 * @param[in] cmd
 * @param[in] origin
 */
void CG_VoiceChatLocal(int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd, vec3_t origin)
{
	char            *chat;
	voiceChatList_t *voiceChatList;
	clientInfo_t    *ci;
	sfxHandle_t     snd;
	qhandle_t       sprite;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = 0;
	}
	ci = &cgs.clientinfo[clientNum];

	voiceChatList = CG_VoiceChatListForClient(clientNum);

	if (CG_GetVoiceChat(voiceChatList, cmd, &snd, &sprite, &chat))
	{
		if (mode == SAY_TEAM || mode == SAY_BUDDY || !cg_teamChatsOnly.integer || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			bufferedVoiceChat_t vchat;
			const char          *loc = " ";

			vchat.clientNum = clientNum;
			vchat.snd       = snd;
			vchat.sprite    = sprite;
			vchat.voiceOnly = voiceOnly;
			VectorCopy(origin, vchat.origin);
			Q_strncpyz(vchat.cmd, cmd, sizeof(vchat.cmd));

			if (mode != SAY_ALL)
			{
				// get location
				loc = CG_BuildLocationString(clientNum, origin, LOC_VCHAT);
				if (!loc || !*loc)
				{
					loc = " ";
				}
			}

			if (mode == SAY_TEAM)
			{
				// show latched class for sayplayerclass cmd
				if (cgs.clientinfo[clientNum].cls != cgs.clientinfo[clientNum].latchedcls)
				{
					if (!strcmp(cmd, "IamMedic") || !strcmp(cmd, "IamEngineer") || !strcmp(cmd, "IamFieldOps") || !strcmp(cmd, "IamCovertOps") || !strcmp(cmd, "IamSoldier"))
					{
						Com_sprintf(vchat.message, sizeof(vchat.message), "^7(%s^7)^3(%s^3): ^%c%s Next class: %s",
						            ci->name, loc, color, CG_TranslateString(chat), BG_ClassnameForNumber(cgs.clientinfo[clientNum].latchedcls)); // FIXME: CG_TranslateString doesn't make sense here
					}
					else // isn't sayplayerclass cmd
					{
						Com_sprintf(vchat.message, sizeof(vchat.message), "^7(%s^7)^3(%s^3): ^%c%s",
						            ci->name, loc, color, CG_TranslateString(chat));
					}
				}
				else
				{
					Com_sprintf(vchat.message, sizeof(vchat.message), "^7(%s^7)^3(%s^3): ^%c%s",
					            ci->name, loc, color, CG_TranslateString(chat));
				}
			}
			else if (mode == SAY_BUDDY)
			{
				Com_sprintf(vchat.message, sizeof(vchat.message), "^7(%s^7)^3(%s^3): ^%c%s",
				            ci->name, loc, color, CG_TranslateString(chat));  // FIXME: CG_TranslateString doesn't make sense here
			}
			else
			{
				Com_sprintf(vchat.message, sizeof(vchat.message), "^7%s^3: ^%c%s",
				            ci->name, color, CG_TranslateString(chat));  // FIXME: CG_TranslateString doesn't make sense here
			}
			CG_AddBufferedVoiceChat(&vchat);
		}
	}
}

/**
 * @brief CG_VoiceChat
 * @param[in] mode
 */
void CG_VoiceChat(int mode)
{
	const char *cmd;
	int        clientNum, color;
	qboolean   voiceOnly;
	vec3_t     origin = { 0 };

	voiceOnly = (qboolean)(atoi(CG_Argv(1)));
	clientNum = Q_atoi(CG_Argv(2));
	color     = Q_atoi(CG_Argv(3));

	if (mode != SAY_ALL)
	{
		origin[0] = Q_atoi(CG_Argv(5));
		origin[1] = Q_atoi(CG_Argv(6));
		origin[2] = Q_atoi(CG_Argv(7));
	}

	cmd = CG_Argv(4);

	CG_VoiceChatLocal(mode, voiceOnly, clientNum, color, cmd, origin);
}

/**
 * @brief CG_RemoveChatEscapeChar
 * @param[in,out] text
 */
static void CG_RemoveChatEscapeChar(char *text)
{
	int i, l = 0;

	for (i = 0; text[i]; i++)
	{
		if (text[i] == '\x19')
		{
			continue;
		}
		text[l++] = text[i];
	}
	text[l] = '\0';
}

static char *CG_FindNeedle(const char *haystack, const char *needle, size_t needle_len)
{
	if (needle_len == 0)
	{
		return (char *)haystack;
	}
	while (haystack[0])
	{
		if ((tolower(haystack[0]) == tolower(needle[0])) && (Q_stricmpn(haystack, needle, needle_len) == 0))
		{
			return (char *)haystack;
		}
		haystack++;
	}
	return NULL;
}

static const char *CG_AddChatMention(char *text, int clientNum)
{
	static char message[MAX_SAY_TEXT + 8];
	message[0] = 0;
	if (cg_teamChatMention.integer && cg.clientNum != clientNum)
	{
		char *mntStart, *mntEnd, *msgPart = text;
		if (clientNum < 0)
		{
			msgPart += strlen("console");
		}
		else
		{
			msgPart += strlen(cgs.clientinfo[clientNum].name);
		}
		if ((mntStart = strchr(msgPart, '@')))
		{
			// check that the previous character was whitespace
			if (*(mntStart - 1) != ' ')
			{
				return text;
			}
			mntStart++; // skip @ char
			mntEnd = mntStart;
			while (*mntEnd && *mntEnd != ' ')
			{
				mntEnd++;
			}
			if (mntEnd - mntStart < 3)
			{
				return text;
			}
			if (CG_FindNeedle(cgs.clientinfo[cg.clientNum].name, mntStart, mntEnd - mntStart))
			{
				Q_strcat(message, sizeof(message), "^3> ^7");
				Q_strcat(message, sizeof(message), text);
				return message;
			}
		}
	}
	return text;
}

/**
 * @brief Localize string sent from server
 * @details
 * - localization is ON by default.
 * - use [lof] in string to turn OFF
 * - use [lon] in string to turn back ON
 * @param buf
 * @return
 */
const char *CG_LocalizeServerCommand(const char *buf)
{
	static char token[MAX_TOKEN_CHARS];
	char        temp[MAX_TOKEN_CHARS];
	qboolean    togloc = qtrue;
	const char  *s     = buf;
	int         i, prev = 0;

	Com_Memset(token, 0, sizeof(token));

	for (i = 0; *s; i++, s++)
	{
		// line was: if ( *s == '[' && !Q_strncmp( s, "[lon]", 5 ) || !Q_strncmp( s, "[lof]", 5 ) ) {
		// || prevails on &&, gcc warning was 'suggest parentheses around && within ||'
		// modified to the correct behaviour
		if (*s == '[' && (!Q_strncmp(s, "[lon]", 5) || !Q_strncmp(s, "[lof]", 5)))
		{
			if (togloc)
			{
				Com_Memset(temp, 0, sizeof(temp));
				strncpy(temp, buf + prev, i - prev);
				Q_strcat(token, MAX_TOKEN_CHARS, CG_TranslateString(temp));
			}
			else
			{
				strncat(token, buf + prev, i - prev);
			}

			if (s[3] == 'n')
			{
				togloc = qtrue;
			}
			else
			{
				togloc = qfalse;
			}

			i   += 5;
			s   += 5;
			prev = i;
		}
	}

	if (togloc)
	{
		Com_Memset(temp, 0, sizeof(temp));
		strncpy(temp, buf + prev, i - prev);
		Q_strcat(token, MAX_TOKEN_CHARS, CG_TranslateString(temp));
	}
	else
	{
		strncat(token, buf + prev, i - prev);
	}

	return token;
}

/**
 * @brief CG_wstatsParse_cmd
 */
void CG_wstatsParse_cmd(void)
{
	if (cg.showStats)
	{
		if (cg.statsWindow == NULL
		    || cg.statsWindow->id != WID_STATS
		    || cg.statsWindow->inuse == qfalse
		    )
		{
			CG_createStatsWindow();
		}
		else if (cg.statsWindow->state == WSTATE_SHUTDOWN)
		{
			cg.statsWindow->state = WSTATE_START;
			cg.statsWindow->time  = trap_Milliseconds();
		}

		if (cg.statsWindow == NULL)
		{
			cg.showStats = qfalse;
		}
		else
		{
			cg.statsWindow->effects  |= WFX_TEXTSIZING;
			cg.statsWindow->lineCount = 0;
			cg.windowCurrent          = cg.statsWindow;
			CG_parseWeaponStats_cmd(CG_printWindow);
		}
	}
}

/**
 * @brief CG_topshotsParse_cmd
 * @param doBest - unused
 */
void CG_topshotsParse_cmd(qboolean doBest)
{
	int            iArg = 1;
	int            iWeap;
	int            cnum, hits, atts, kills, deaths, headshots;
	topshotStats_t *ts = &cgs.topshots;
	char           name[32];
	float          acc;

	iWeap = Q_atoi(CG_Argv(iArg++));

	ts->cWeapons = 0;

	while (iWeap)
	{
		cnum      = Q_atoi(CG_Argv(iArg++));
		hits      = Q_atoi(CG_Argv(iArg++));
		atts      = Q_atoi(CG_Argv(iArg++));
		kills     = Q_atoi(CG_Argv(iArg++));
		deaths    = Q_atoi(CG_Argv(iArg++));
		headshots = Q_atoi(CG_Argv(iArg++));
		acc       = (atts > 0) ? (float)(hits * 100) / (float)atts : 0.0f;

		if (ts->cWeapons < WS_MAX * 2 && aWeaponInfo[iWeap - 1].fHasHeadShots)
		{
			CG_cleanName(cgs.clientinfo[cnum].name, name, 17, qfalse);
			Q_strncpyz(ts->strWS[ts->cWeapons++],
			           va("%-12s %5.1f %4d/%-4d %5d %6d %8d  %s",
			              aWeaponInfo[iWeap - 1].pszName,
			              (double)acc, hits, atts, kills, deaths, headshots, name),
			           sizeof(ts->strWS[0]));
		}

		iWeap = Q_atoi(CG_Argv(iArg++));
	}
}

/**
 * @brief CG_ParseWeaponStats
 */
void CG_ParseWeaponStats(void)
{
	cgs.ccWeaponShots = Q_atoi(CG_Argv(1));
	cgs.ccWeaponHits  = Q_atoi(CG_Argv(2));
}

/**
 * @brief CG_ParsePortalPos
 */
void CG_ParsePortalPos(void)
{
	int i;

	cgs.ccCurrentCamObjective = Q_atoi(CG_Argv(1));
	cgs.ccPortalEnt           = Q_atoi(CG_Argv(8));

	for (i = 0; i < 3; i++)
	{
		cgs.ccPortalPos[i] = Q_atoi(CG_Argv(i + 2));
	}

	for (i = 0; i < 3; i++)
	{
		cgs.ccPortalAngles[i] = Q_atoi(CG_Argv(i + 5));
	}
}

/**
 * @brief Cached stats
 */
void CG_parseWeaponStatsGS_cmd(void)
{
	clientInfo_t *ci;
	gameStats_t  *gs = &cgs.gamestats;
	int          i, iArg = 1;
	int          nClientID;
	int          nRounds;
	int          weaponMask;
	int          skillMask, xp = 0;
	int          totHits      = 0;
	int          totShots     = 0;
	int          totKills     = 0;
	int          totDeaths    = 0;
	int          totHeadshots = 0;

	nClientID  = Q_atoi(CG_Argv(iArg++));
	nRounds    = Q_atoi(CG_Argv(iArg++));
	weaponMask = Q_atoi(CG_Argv(iArg++));

	gs->cWeapons  = 0;
	gs->cSkills   = 0;
	gs->fHasStats = qfalse;

	gs->nClientID = nClientID;
	gs->nRounds   = nRounds;

	ci = &cgs.clientinfo[nClientID];

	if (weaponMask != 0)
	{
		char  strName[MAX_STRING_CHARS];
		int   nHits;
		int   nShots;
		int   nKills;
		int   nDeaths;
		int   nHeadshots;
		float acc;

		for (i = WS_KNIFE; i < WS_MAX; i++)
		{
			if (weaponMask & (1 << i))
			{
				nHits      = Q_atoi(CG_Argv(iArg++));
				nShots     = Q_atoi(CG_Argv(iArg++));
				nKills     = Q_atoi(CG_Argv(iArg++));
				nDeaths    = Q_atoi(CG_Argv(iArg++));
				nHeadshots = Q_atoi(CG_Argv(iArg++));
				acc        = (nShots > 0) ? (float)(nHits * 100) / (float)nShots : 0.0f;

				totKills  += nKills;
				totDeaths += nDeaths;

				if (aWeaponInfo[i].fHasHeadShots)
				{
					totHits      += nHits;
					totShots     += nShots;
					totHeadshots += nHeadshots;
				}

				Q_strncpyz(strName, va("%-12s  ", aWeaponInfo[i].pszName), sizeof(strName));
				if (nShots > 0 || nHits > 0)
				{
					Q_strcat(strName, sizeof(strName), va("%s %4d/%-4d ", aWeaponInfo[i].fHasHeadShots ? va("%5.1f", (double)acc) : "     ", nHits, nShots));
				}
				else
				{
					Q_strcat(strName, sizeof(strName), va("                "));
				}

				// syringe doesn't have kill/death stats
				if (i == WS_SYRINGE)
				{
					Q_strncpyz(gs->strWS[gs->cWeapons++], va("%s", strName), sizeof(gs->strWS[0]));
				}
				else
				{
					Q_strncpyz(gs->strWS[gs->cWeapons++],
					           va("%s%5d %6d%s", strName, nKills, nDeaths, aWeaponInfo[i].fHasHeadShots ? va(" %8d", nHeadshots) : ""),
					           sizeof(gs->strWS[0]));
				}

				if (nShots > 0 || nHits > 0 || nKills > 0 || nDeaths)
				{
					gs->fHasStats = qtrue;
				}
			}
		}

		if (gs->fHasStats)
		{
			int   dmg_given;
			int   dmg_rcvd;
			int   team_dmg_given;
			int   team_dmg_rcvd;
			int   gibs;
			int   selfKills;
			int   teamKills;
			int   teamGibs;
			float ptRatio;
			float htRatio;
			float hsRatio;

			dmg_given      = Q_atoi(CG_Argv(iArg++));
			dmg_rcvd       = Q_atoi(CG_Argv(iArg++));
			team_dmg_given = Q_atoi(CG_Argv(iArg++));
			team_dmg_rcvd  = Q_atoi(CG_Argv(iArg++));
			gibs           = Q_atoi(CG_Argv(iArg++));
			selfKills      = Q_atoi(CG_Argv(iArg++));
			teamKills      = Q_atoi(CG_Argv(iArg++));
			teamGibs       = Q_atoi(CG_Argv(iArg++));
			ptRatio        = (float)atof(CG_Argv(iArg++));

			htRatio = (totShots == 0) ? 0.0f : (float)(totHits * 100.0f / (float)totShots);
			hsRatio = (totHits == 0) ? 0.0f : (float)(totHeadshots * 100.0f / (float)totHits);

			Q_strncpyz(gs->strExtra[0], va(CG_TranslateString("Damage Given: %6d      Team Damage Given: %6d"), dmg_given, team_dmg_given), sizeof(gs->strExtra[0]));
			Q_strncpyz(gs->strExtra[1], va(CG_TranslateString("Damage Recvd: %6d      Team Damage Recvd: %6d"), dmg_rcvd, team_dmg_rcvd), sizeof(gs->strExtra[0]));
			Q_strncpyz(gs->strExtra[2], "", sizeof(gs->strExtra[0]));
			Q_strncpyz(gs->strExtra[3], va(CG_TranslateString("Kills:  %3d    Team Kills: %3d    Accuracy:  %5.1f%%"), totKills, teamKills, (double)htRatio), sizeof(gs->strExtra[0]));
			Q_strncpyz(gs->strExtra[4], va(CG_TranslateString("Deaths: %3d    Self Kills: %3d    Headshots: %5.1f%%"), totDeaths, selfKills, (double)hsRatio), sizeof(gs->strExtra[0]));
			Q_strncpyz(gs->strExtra[5], va(CG_TranslateString("Gibs:   %3d    Team Gibs:  %3d    Playtime:  %5.1f%%"), gibs, teamGibs, (double)ptRatio), sizeof(gs->strExtra[0]));
		}
	}

	// Derive XP from individual skill XP
	skillMask = Q_atoi(CG_Argv(iArg++));
	for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
	{
		if (skillMask & (1 << i))
		{
#ifdef FEATURE_PRESTIGE
			if (cgs.prestige && cgs.gametype != GT_WOLF_CAMPAIGN && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
			{
				ci->skillpoints[i]      = Q_atoi(CG_Argv(iArg++));
				ci->deltaskillpoints[i] = Q_atoi(CG_Argv(iArg++));
				xp                     += ci->deltaskillpoints[i];
			}
			else
#endif
			{
				ci->skillpoints[i] = Q_atoi(CG_Argv(iArg++));
				xp                += ci->skillpoints[i];
			}
		}
	}

#ifdef FEATURE_RATING
	if (cgs.skillRating)
	{
		ci->rating      = (float) atof(CG_Argv(iArg++));
		ci->deltaRating = (float) atof(CG_Argv(iArg++));
	}
#endif
#ifdef FEATURE_PRESTIGE
	if (cgs.prestige)
	{
		ci->prestige = Q_atoi(CG_Argv(iArg++));
	}
#endif

#if defined(FEATURE_RATING) && defined(FEATURE_PRESTIGE)
	if (cgs.skillRating && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
	{
		if (cgs.prestige && cgs.gametype != GT_WOLF_CAMPAIGN)
		{
			Q_strncpyz(gs->strRank, va("%-21s %-8d %-14.2f %-3i", GetRankTableData(ci->team, ci->rank)->names, xp, (double)ci->rating, ci->prestige), sizeof(gs->strRank));
		}
		else
		{
			Q_strncpyz(gs->strRank, va("%-21s %-8d %-14.2f", GetRankTableData(ci->team, ci->rank)->names, xp, (double)ci->rating), sizeof(gs->strRank));
		}
	}
	else
#endif
#ifdef FEATURE_RATING
	if (cgs.skillRating && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
	{
		Q_strncpyz(gs->strRank, va("%-21s %-8d %-14.2f", GetRankTableData(ci->team, ci->rank)->names, xp, (double)ci->rating), sizeof(gs->strRank));
	}
	else
#endif
#ifdef FEATURE_PRESTIGE
	if (cgs.prestige && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS && cgs.gametype != GT_WOLF_CAMPAIGN)
	{
		Q_strncpyz(gs->strRank, va("%-21s %-8d %-14i", GetRankTableData(ci->team, ci->rank)->names, xp, ci->prestige), sizeof(gs->strRank));
	}
	else
#endif
	{
		Q_strncpyz(gs->strRank, va("%-21s %-8d", GetRankTableData(ci->team, ci->rank)->names, xp), sizeof(gs->strRank));
	}

	if (skillMask != 0)
	{
		char *str;

		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			if ((skillMask & (1 << i)) == 0)
			{
				continue;
			}

#ifdef FEATURE_PRESTIGE
			if (cgs.prestige && cgs.gametype != GT_WOLF_CAMPAIGN && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
			{
				if (ci->skill[i] < NUM_SKILL_LEVELS - 1)
				{
					str = va("%10d (%d/%d)", ci->deltaskillpoints[i], ci->skillpoints[i], GetSkillTableData(i)->skillLevels[ci->skill[i] + 1]);
				}
				else
				{
					str = va("%10d (%d)", ci->deltaskillpoints[i], ci->skillpoints[i]);
				}
			}
			else
#endif
			{
				if (ci->skill[i] < NUM_SKILL_LEVELS - 1)
				{
					str = va("%10d/%-10d", ci->skillpoints[i], GetSkillTableData(i)->skillLevels[ci->skill[i] + 1]);
				}
				else
				{
					str = va("%10d", ci->skillpoints[i]);
				}
			}

			if (cgs.gametype == GT_WOLF_CAMPAIGN)
			{
				Q_strncpyz(gs->strSkillz[gs->cSkills++], va("%-15s %3d %-24s %6d", GetSkillTableData(i)->skillNames, ci->skill[i], str, ci->medals[i]), sizeof(gs->strSkillz[0]));
			}
			else
			{
				Q_strncpyz(gs->strSkillz[gs->cSkills++], va("%-15s %3d %-24s", GetSkillTableData(i)->skillNames, ci->skill[i], str), sizeof(gs->strSkillz[0]));
			}
		}
	}
}

/**
 * @brief Client-side stat presentation
 */
void CG_parseWeaponStats_cmd(void (txt_dump) (const char *))
{
	clientInfo_t *ci;
	qboolean     fFull;
	qboolean     fHasStats = qfalse;
	char         strName[MAX_STRING_CHARS];
	int          atts, deaths, hits, kills, headshots;
	unsigned int i, iArg = 1;
	unsigned int nClient;
	unsigned int nRounds;
	unsigned int dwWeaponMask;
	unsigned int dwSkillPointMask;
	int          xp           = 0; // XP can be negative
	int          totHits      = 0;
	int          totShots     = 0;
	int          totKills     = 0;
	int          totDeaths    = 0;
	int          totHeadshots = 0;

	fFull = (qboolean)(txt_dump != CG_printWindow);

	nClient      = Q_atoi(CG_Argv(iArg++));
	nRounds      = Q_atoi(CG_Argv(iArg++));
	dwWeaponMask = Q_atoi(CG_Argv(iArg++));

	ci = &cgs.clientinfo[nClient];

	Q_strncpyz(strName, ci->name, sizeof(strName));
	CG_cleanName(cgs.clientinfo[nClient].name, strName, sizeof(strName), qfalse);

	txt_dump("\n");
	txt_dump(va("^7Overall stats for: ^3%s ^7(^2%d^7 Round%s)\n", strName, nRounds, ((nRounds != 1) ? "s" : "")));
	txt_dump("\n");

	if (fFull)
	{
		txt_dump(CG_TranslateString("^7Weapon      Acrcy Hits/Shts Kills Deaths Headshots\n"));
		txt_dump("^7--------------------------------------------------\n");
	}
	else
	{
		txt_dump(CG_TranslateString("^7Weapon      Acrcy Hits/Shts Kll Dth HS\n"));
		//txt_dump(     "^7--------------------------------------\n");
		txt_dump("\n");
	}

	if (!dwWeaponMask)
	{
		txt_dump(CG_TranslateString("^3No weapon info available.\n"));
	}
	else
	{
		int   dmg_given;
		int   dmg_rcvd;
		int   team_dmg_given;
		int   team_dmg_rcvd;
		int   gibs;
		int   selfKills;
		int   teamKills;
		int   teamGibs;
		float ptRatio;
		float htRatio;
		float hsRatio;

		for (i = WS_KNIFE; i < WS_MAX; i++)
		{
			if (dwWeaponMask & (1 << i))
			{
				hits      = Q_atoi(CG_Argv(iArg++));
				atts      = Q_atoi(CG_Argv(iArg++));
				kills     = Q_atoi(CG_Argv(iArg++));
				deaths    = Q_atoi(CG_Argv(iArg++));
				headshots = Q_atoi(CG_Argv(iArg++));

				totKills  += kills;
				totDeaths += deaths;

				if (aWeaponInfo[i].fHasHeadShots)
				{
					totHits      += hits;
					totShots     += atts;
					totHeadshots += headshots;
				}

				Q_strncpyz(strName, va("^3%-10s: ", aWeaponInfo[i].pszName), sizeof(strName));
				if (atts > 0 || hits > 0)
				{
					float acc = (atts == 0) ? 0.0f : (float)(hits * 100.0f / (float)atts);
					fHasStats = qtrue;

					Q_strcat(strName, sizeof(strName), va("^7%s ^5%4d/%-4d ", aWeaponInfo[i].fHasHeadShots ? va("%5.1f", (double)acc) : "     ", hits, atts));
				}
				else
				{
					Q_strcat(strName, sizeof(strName), va("                "));
					if (kills > 0 || deaths > 0)
					{
						fHasStats = qtrue;
					}
				}

				// syringe doesn't have kill/death stats
				if (i == WS_SYRINGE)
				{
					txt_dump(va("%s\n", strName));
				}
				else if (fFull)
				{
					txt_dump(va("%s^2%5d ^1%6d%s\n", strName, kills, deaths, aWeaponInfo[i].fHasHeadShots ? va(" ^3%9d", headshots) : ""));
				}
				else
				{
					txt_dump(va("%s^2%3d ^1%3d%s\n", strName, kills, deaths, aWeaponInfo[i].fHasHeadShots ? va(" ^3%2d", headshots) : ""));
				}
			}
		}

		if (fHasStats)
		{
			dmg_given      = Q_atoi(CG_Argv(iArg++));
			dmg_rcvd       = Q_atoi(CG_Argv(iArg++));
			team_dmg_given = Q_atoi(CG_Argv(iArg++));
			team_dmg_rcvd  = Q_atoi(CG_Argv(iArg++));
			gibs           = Q_atoi(CG_Argv(iArg++));
			selfKills      = Q_atoi(CG_Argv(iArg++));
			teamKills      = Q_atoi(CG_Argv(iArg++));
			teamGibs       = Q_atoi(CG_Argv(iArg++));
			ptRatio        = atof(CG_Argv(iArg++));

			htRatio = (totShots == 0) ? 0.0 : (float)(totHits * 100.0 / (float)totShots);
			hsRatio = (totHits == 0) ? 0.0 : (float)(totHeadshots * 100.0 / (float)totHits);

			if (!fFull)
			{
				txt_dump("\n\n\n");
			}
			else
			{
				txt_dump("\n");
			}

			txt_dump(va("^3Damage Given: ^7%6d     ^3Team Damage Given: ^7%6d\n", dmg_given, team_dmg_given));
			txt_dump(va("^3Damage Recvd: ^7%6d     ^3Team Damage Recvd: ^7%6d\n", dmg_rcvd, team_dmg_rcvd));
			txt_dump("\n");
			txt_dump(va("^3Kills:  ^7%3d   ^3Team Kills: ^7%3d   ^3Accuracy:  ^7 %5.1f%%\n", totKills, teamKills, htRatio));
			txt_dump(va("^3Deaths: ^7%3d   ^3Self Kills: ^7%3d   ^3Headshots: ^7 %5.1f%%\n", totDeaths, selfKills, hsRatio));
			txt_dump(va("^3Gibs:   ^7%3d   ^3Team Gibs:  ^7%3d   ^3Playtime:  ^7 %5.1f%%\n", gibs, teamGibs, ptRatio));
		}
	}

	if (!fFull)
	{
		txt_dump("\n\n\n");
	}
	else
	{
		txt_dump("\n");
	}

	// Derive XP from individual skill XP
	dwSkillPointMask = Q_atoi(CG_Argv(iArg++));
	for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
	{
		if (dwSkillPointMask & (1 << i))
		{
#ifdef FEATURE_PRESTIGE
			if (cgs.prestige && cgs.gametype != GT_WOLF_CAMPAIGN && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
			{
				ci->skillpoints[i]      = Q_atoi(CG_Argv(iArg++));
				ci->deltaskillpoints[i] = Q_atoi(CG_Argv(iArg++));
				xp                     += ci->skillpoints[i];
			}
			else
#endif
			{
				ci->skillpoints[i] = Q_atoi(CG_Argv(iArg++));
				xp                += ci->skillpoints[i];
			}
		}
	}

	txt_dump(va("^2Rank: ^7%s (%d XP)\n", GetRankTableData(ci->team, ci->rank)->names, xp));

#ifdef FEATURE_RATING
	if (cgs.skillRating && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
	{
		float rating;
		float deltaRating;

		// skill rating
		rating      = atof(CG_Argv(iArg++));
		deltaRating = atof(CG_Argv(iArg++));

		txt_dump(va("^2Skill Rating: ^7%5.2f   (^5%+5.2f^7)\n", rating, deltaRating));
	}
#endif

#ifdef FEATURE_PRESTIGE
	if (cgs.prestige && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS && cgs.gametype != GT_WOLF_CAMPAIGN)
	{
		int prestige;

		prestige = Q_atoi(CG_Argv(iArg++));

		txt_dump(va("^2Prestige: ^7%i\n", prestige));
	}
#endif

	if (!fFull)
	{
		txt_dump("\n\n\n");
	}
	else
	{
		txt_dump("\n");
	}

	// Medals only in campaign mode
	txt_dump(va("^7Skills         Level/Points%s\n", ((cgs.gametype == GT_WOLF_CAMPAIGN) ? CG_TranslateString("  Medals") : "")));
	if (fFull)
	{
		txt_dump(va("^7---------------------------%s\n", ((cgs.gametype == GT_WOLF_CAMPAIGN) ? "--------" : "")));
	}
	else
	{
		txt_dump("\n");
	}

	if (dwSkillPointMask == 0)
	{
		txt_dump(CG_TranslateString("^3No skills acquired!\n"));
	}
	else
	{
		char *str;

		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{

			if ((dwSkillPointMask & (1 << i)) == 0)
			{
				continue;
			}

			if (ci->skill[i] < NUM_SKILL_LEVELS - 1)
			{
				str = va("%d (%d/%d)", ci->skill[i], ci->skillpoints[i], GetSkillTableData(i)->skillLevels[ci->skill[i] + 1]);
			}
			else
			{
				str = va("%d (%d)", ci->skill[i], ci->skillpoints[i]);
			}

			if (cgs.gametype == GT_WOLF_CAMPAIGN)
			{
				txt_dump(va("%-14s ^3%-12s  ^2%6d\n", GetSkillTableData(i)->skillNames, str, ci->medals[i]));
			}
			else
			{
				txt_dump(va("%-14s ^3%-12s\n", GetSkillTableData(i)->skillNames, str));
			}
		}
	}
}

/**
 * @brief CG_parseBestShotsStats_cmd
 * @param[in] doTop
 */
static void CG_parseBestShotsStats_cmd(qboolean doTop, void (txt_dump) (const char *))
{
	int      iArg = 1;
	qboolean fFull;
	int      iWeap;

	fFull = (qboolean)(txt_dump != CG_printWindow);
	iWeap = Q_atoi(CG_Argv(iArg++));

	if (!iWeap)
	{
		txt_dump(va("^3No qualifying %sshot info available.\n", ((doTop) ? "top" : "bottom")));
		return;
	}

	txt_dump(va("^2%s Match Accuracies:\n", (doTop) ? "BEST" : "WORST"));
	if (fFull)
	{
		txt_dump("\n^3WP   Acrcy Hits/Shts Kills Deaths HdShts Player\n");
		txt_dump("-------------------------------------------------------------\n");
	}
	else
	{
		txt_dump("^3WP   Acrcy Hits/Shts Kll Dth HS Plr\n");
		//  txt_dump(    "-------------------------------------------\n");
		txt_dump("\n");
	}

	{
		int   cnum;
		int   hits;
		int   atts;
		int   kills;
		int   deaths;
		int   headshots;
		float acc;
		char  name[32];

		while (iWeap)
		{
			cnum      = Q_atoi(CG_Argv(iArg++));
			hits      = Q_atoi(CG_Argv(iArg++));
			atts      = Q_atoi(CG_Argv(iArg++));
			kills     = Q_atoi(CG_Argv(iArg++));
			deaths    = Q_atoi(CG_Argv(iArg++));
			headshots = Q_atoi(CG_Argv(iArg++));
			acc       = (atts > 0) ? (float)(hits * 100) / (float)atts : 0.0f;

			if (fFull)
			{
				CG_cleanName(cgs.clientinfo[cnum].name, name, 30, qfalse);
				txt_dump(va("^3%s ^7%s ^5%4d/%-4d ^2%5d ^1%6d ^3%s ^7%s\n",
				            aWeaponInfo[iWeap - 1].pszCode,
				            aWeaponInfo[iWeap - 1].fHasHeadShots ? va("%5.1f", (double)acc) : "     ",
				            hits, atts, kills, deaths,
				            aWeaponInfo[iWeap - 1].fHasHeadShots ? va("%6d", headshots) : "      ",
				            name));
			}
			else
			{
				CG_cleanName(cgs.clientinfo[cnum].name, name, 12, qfalse);
				txt_dump(va("^3%s ^7%s ^5%4d/%-4d ^2%3d ^1%3d ^3%s ^7%s\n",
				            aWeaponInfo[iWeap - 1].pszCode,
				            aWeaponInfo[iWeap - 1].fHasHeadShots ? va("%5.1f", (double)acc) : "     ",
				            hits, atts, kills, deaths,
				            aWeaponInfo[iWeap - 1].fHasHeadShots ? va("%2d", headshots) : "  ",
				            name));
			}

			iWeap = Q_atoi(CG_Argv(iArg++));
		}
	}
}

/**
 * @brief CG_parseTopShotsStats_cmd
 * @param[in] doTop
 */
static void CG_parseTopShotsStats_cmd(qboolean doTop, void (txt_dump) (const char *))
{
	int i, iArg = 1;
	int cClients;
	int iWeap;
	int wBestAcc;

	cClients = Q_atoi(CG_Argv(iArg++));
	iWeap    = Q_atoi(CG_Argv(iArg++));
	wBestAcc = Q_atoi(CG_Argv(iArg++));

	txt_dump(va("Weapon accuracies for: ^3%s\n",
	            (iWeap >= WS_KNIFE && iWeap < WS_MAX) ? aWeaponInfo[iWeap].pszName : "UNKNOWN"));

	txt_dump("\n^3  Acc Hits/Shts Kills Deaths HeadShots Player\n");
	txt_dump("----------------------------------------------------------\n");

	if (!cClients)
	{
		txt_dump("NO QUALIFYING WEAPON INFO AVAILABLE.\n");
		return;
	}

	{
		int        cnum;
		int        hits;
		int        atts;
		int        kills;
		int        deaths;
		int        headshots;
		float      acc;
		const char *color;
		char       name[32];

		for (i = 0; i < cClients; i++)
		{
			cnum      = Q_atoi(CG_Argv(iArg++));
			hits      = Q_atoi(CG_Argv(iArg++));
			atts      = Q_atoi(CG_Argv(iArg++));
			kills     = Q_atoi(CG_Argv(iArg++));
			deaths    = Q_atoi(CG_Argv(iArg++));
			headshots = Q_atoi(CG_Argv(iArg++));
			acc       = (atts > 0) ? (float)(hits * 100) / (float)atts : 0.0f;
			color     = (((doTop) ? (double)acc : ((double)wBestAcc) + 0.999) >= ((doTop) ? wBestAcc : (double)acc)) ? "^3" : "^7";

			CG_cleanName(cgs.clientinfo[cnum].name, name, 30, qfalse);
			txt_dump(va("%s%s ^5%4d/%-4d ^2%5d ^1%6d ^3%s %s%s\n", color,
			            aWeaponInfo[i].fHasHeadShots ? va("%5.1f", (double)acc) : "     ", hits, atts, kills, deaths,
			            aWeaponInfo[i].fHasHeadShots ? va("^3%9d", headshots) : "", color, name));
		}
	}
}

/**
 * @brief CG_scores_cmd
 */
static void CG_scores_cmd(void)
{
	const char *str;

	str = CG_Argv(1);

	CG_Printf("[skipnotify]%s", str);
	if (cgs.dumpStatsFile > 0)
	{
		char s[MAX_STRING_CHARS];

		CG_cleanName(str, s, sizeof(s), qtrue);
		trap_FS_Write(s, strlen(s), cgs.dumpStatsFile);
	}

	if (trap_Argc() > 2)
	{
		if (cgs.dumpStatsFile > 0)
		{
			qtime_t ct;

			trap_RealTime(&ct);
			str = va("\nStats recorded: %02d:%02d:%02d (%02d %s %d)\n\n\n",
			         ct.tm_hour, ct.tm_min, ct.tm_sec,
			         ct.tm_mday, aMonths[ct.tm_mon], 1900 + ct.tm_year);

			trap_FS_Write(str, strlen(str), cgs.dumpStatsFile);

			CG_Printf("[cgnotify]\n^3>>> Stats recorded to: ^7%s\n\n", cgs.dumpStatsFileName);
			trap_FS_FCloseFile(cgs.dumpStatsFile);
			cgs.dumpStatsFile = 0;
		}
		cgs.dumpStatsTime = 0;
	}
}

/**
 * @brief CG_printFile
 * @param[in] str
 */
static void CG_printFile(const char *str)
{
	CG_Printf("%s", str);
	if (cgs.dumpStatsFile > 0)
	{
		char s[MAX_STRING_CHARS];

		CG_cleanName(str, s, sizeof(s), qtrue);
		trap_FS_Write(s, strlen(s), cgs.dumpStatsFile);
	}
}

/**
 * @brief CG_dumpStats
 */
void CG_dumpStats(void)
{
	qtime_t    ct;
	qboolean   fDoScores = qfalse;
	const char *info;
	char       *s;

	info = CG_ConfigString(CS_SERVERINFO);
	s    = va("^3>>> %s: ^2%s\n\n", CG_TranslateString("Map"), Info_ValueForKey(info, "mapname"));

	trap_RealTime(&ct);
	if (cgs.dumpStatsFile == 0)
	{
		fDoScores             = qtrue;
		cgs.dumpStatsFileName = va("stats/%s.txt", CG_generateFilename());
	}

	if (cgs.dumpStatsFile != 0)
	{
		trap_FS_FCloseFile(cgs.dumpStatsFile);
	}
	trap_FS_FOpenFile(cgs.dumpStatsFileName, &cgs.dumpStatsFile, FS_APPEND);

	CG_printFile(s);
	CG_parseWeaponStats_cmd(CG_printFile);
	if (cgs.dumpStatsFile == 0)
	{
		CG_Printf("[cgnotify]\n^3>>> %s: %s\n\n", CG_TranslateString("Could not create logfile"), cgs.dumpStatsFileName);
	}

	// Daisy-chain to scores info
	//  -- we play a game here for a statsall dump:
	//      1. we still have more ws entries in the queue to parse
	//      2. on the 1st ws entry, go ahead and send out the scores request
	//      3. we'll just continue to parse the remaining ws entries and dump them to the log
	//         before the scores result would ever hit us.. thus, we still keep proper ordering :)
	if (fDoScores)
	{
		trap_SendClientCommand("scores");
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
// -----------

/**
 * @brief The string has been tokenized and can be retrieved with
 * Cmd_Argc() / Cmd_Argv()
 */
static void CG_ServerCommand(void)
{
	const char *cmd;

	cmd = CG_Argv(0);

	if (!cmd[0])
	{
		// server claimed the command
		return;
	}

	switch (BG_StringHashValue(cmd))
	{
	case ENTNFO_HASH:                     // "entnfo"
	{
		char buffer[16];
		int  allied_number, axis_number;

		trap_Argv(1, buffer, 16);
		axis_number = Q_atoi(buffer);

		trap_Argv(2, buffer, 16);
		allied_number = Q_atoi(buffer);

		CG_ParseMapEntityInfo(axis_number, allied_number);
		return;
	}
	case CS_HASH:                         // "cs"
		CG_ConfigStringModified();
		return;
	case TINFO_HASH:                      // "tinfo"
		CG_ParseTeamInfo();
		return;
	case SC0_HASH:                        // "sc0"
		CG_ParseScore(TEAM_AXIS);
		return;
	case SC1_HASH:                        // "sc1"
		CG_ParseScore(TEAM_ALLIES);
		return;
	case WEAPONSTATS_HASH:                // "WeaponStats"
	{
		int i, start = 1;

		for (i = WP_KNIFE; i < WP_NUM_WEAPONS; i++)
		{
			if (GetWeaponTableData(i)->indexWeaponStat == WS_MAX)
			{
				continue;
			}

			cgs.playerStats.weaponStats[i].kills     = Q_atoi(CG_Argv(start++));
			cgs.playerStats.weaponStats[i].killedby  = Q_atoi(CG_Argv(start++));
			cgs.playerStats.weaponStats[i].teamkills = Q_atoi(CG_Argv(start++));
		}

		cgs.playerStats.selfkills = Q_atoi(CG_Argv(start++));

		for (i = 0; i < HR_NUM_HITREGIONS; i++)
		{
			cgs.playerStats.hitRegions[i] = Q_atoi(CG_Argv(start++));
		}

		cgs.numOIDtriggers = Q_atoi(CG_Argv(start++));

		for (i = 0; i < cgs.numOIDtriggers; i++)
		{
			cgs.playerStats.objectiveStats[i] = Q_atoi(CG_Argv(start++));
			cgs.teamobjectiveStats[i]         = Q_atoi(CG_Argv(start++));
		}
		return;
	}
	case SC_HASH:                         // "sc"
		// match stats
		CG_scores_cmd();
		return;
	case CPM_HASH:                        // "cpm"
	{
		int                iconnumber;
		const char         *iconstring;
		popupMessageType_t pmType;

		iconstring = CG_Argv(2);

		// catch no cpm icon param
		if (!iconstring[0])
		{
			iconnumber = PM_MESSAGE; // default
		}
		else
		{
			iconnumber = Q_atoi(iconstring);
		}

		// only valid icon types
		if (iconnumber < 0 || iconnumber >= PM_NUM_TYPES)
		{
			iconnumber = PM_MESSAGE;
		}

		if (strstr(CG_Argv(1), " connected") || strstr(CG_Argv(1), " disconnected"))
		{
			pmType = PM_CONNECT;
		}
		else
		{
			pmType = PM_MESSAGE;
		}
		CG_AddPMItem(pmType, CG_LocalizeServerCommand(CG_Argv(1)), " ", cgs.media.pmImages[iconnumber], 0, 0, colorWhite);
		return;
	}
	case CP_HASH:                         // "cp"
	{
		int        args;
		const char *s;

		args = trap_Argc();

		if (args >= 3)
		{
			if (args == 4)
			{
				s = va("%s%s", CG_Argv(3), CG_TranslateString(CG_Argv(1)));
			}
			else
			{
				s = CG_TranslateString(CG_Argv(1));
			}

			// for client logging
			if (cg_printObjectiveInfo.integer > 0 && (args == 4 || Q_atoi(CG_Argv(2)) > 1))
			{
				CG_Printf("[cgnotify]*** ^3INFO: ^5%s\n", CG_LocalizeServerCommand(CG_Argv(1)));
			}
			CG_PriorityCenterPrint(s, 400, cg_fontScaleCP.value, Q_atoi(CG_Argv(2)));
		}
		else
		{
			CPri(CG_Argv(1));
		}
		return;
	}
	case PRINT_HASH:                      // "print"
		CG_Printf("[cgnotify]%s", CG_LocalizeServerCommand(CG_Argv(1)));
		return;
	case CHAT_HASH:                       // "chat"
	{
		char       text[MAX_SAY_TEXT];
		const char *s;
		int        clientNum = -1;     // console

		if (cg_teamChatsOnly.integer && cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) // FIXME: skip for console?
		{
			return;
		}

		if (trap_Argc() >= 3)
		{
			clientNum = Q_atoi(CG_Argv(2));
		}

		if (atoi(CG_Argv(3)))
		{
			s = CG_LocalizeServerCommand(CG_Argv(1));
		}
		else
		{
			s = CG_Argv(1);
		}

		Q_strncpyz(text, s, MAX_SAY_TEXT);
		Q_UnescapeUnicodeInPlace(text, MAX_SAY_TEXT);

		CG_RemoveChatEscapeChar(text);
		s = CG_AddChatMention(text, clientNum);
		CG_AddToTeamChat(s, clientNum);
		CG_Printf("%s\n", s);
		CG_WriteToLog("%s\n", s);
		return;
	}
	case VCHAT_HASH:                              // "vchat"
		CG_VoiceChat(SAY_ALL);              // enabled support
		return;
	case TCHAT_HASH:                              // "tchat"
	{
		char       text[MAX_SAY_TEXT];
		vec3_t     origin;
		char       *loc = NULL;
		const char *s;
		int        clientNum;

		clientNum = Q_atoi(CG_Argv(2));

		origin[0] = Q_atoi(CG_Argv(4));
		origin[1] = Q_atoi(CG_Argv(5));
		origin[2] = Q_atoi(CG_Argv(6));

		if (atoi(CG_Argv(3)))
		{
			s = CG_LocalizeServerCommand(CG_Argv(1));
		}
		else
		{
			s = CG_Argv(1);
		}

		loc = CG_BuildLocationString(clientNum, origin, LOC_TCHAT);

		if (!loc || !*loc)
		{
			loc = "";
		}

		// process locations and name
		Com_sprintf(text, sizeof(text), "(%s^7)^3(%s^3): %s", cgs.clientinfo[clientNum].name, loc, s);
		Q_UnescapeUnicodeInPlace(text, MAX_SAY_TEXT);

#ifdef FEATURE_EDV
		if ((strstr(text, "Fire Mission: ") || strstr(text, "Pilot: ")) && (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam))
		{
			return;
		}
#endif
		CG_RemoveChatEscapeChar(text);
		s = CG_AddChatMention(text, clientNum);
		CG_AddToTeamChat(s, clientNum); // disguise ?
		CG_Printf("%s\n", s);
		CG_WriteToLog("%s\n", s);
		return;
	}
	case VTCHAT_HASH:                                // "vtchat"
		CG_VoiceChat(SAY_TEAM);             // enabled support
		return;
	case VBCHAT_HASH:                                // "vbchat"
		CG_VoiceChat(SAY_BUDDY);
		return;
	case GAMECHAT_HASH:                              // "gamechat"
	{
		const char *s;
		char       text[MAX_SAY_TEXT];

		s = CG_LocalizeServerCommand(CG_Argv(1));

		Q_strncpyz(text, s, MAX_SAY_TEXT);
		Q_UnescapeUnicodeInPlace(text, MAX_SAY_TEXT);

		CG_RemoveChatEscapeChar(text);
		CG_AddToTeamChat(text, cg.snap->ps.clientNum);
		CG_Printf("%s\n", text);
		return;
	}
	case VSCHAT_HASH:                                // "vschat"
	{
		int        clNum;
		int        msgNum;
		const char *wav;;

		clNum  = Q_atoi(CG_Argv(1));
		msgNum = Q_atoi(CG_Argv(2));
		wav    = va("%s%s", cgs.clientinfo[clNum].team == TEAM_AXIS ? "axis" : "allies", HQMessages[msgNum].voiceScript);

		CG_SoundPlaySoundScript(wav, NULL, -1, qtrue);
		CG_AddToTeamChat(HQMessages[msgNum].chatString, clNum);  //  qfalse
		CG_Printf("%s\n", HQMessages[msgNum].chatString);
		return;
	}
	// weapon stats parsing
	case WS_HASH:                                // "ws"
		if (cgs.dumpStatsTime > cg.time)
		{
			CG_dumpStats();
		}
		else
		{
			CG_parseWeaponStats_cmd(CG_printConsoleString);
			cgs.dumpStatsTime = 0;
		}
		return;
	case WWS_HASH:                                      // "wws"
		CG_wstatsParse_cmd();
		return;
	case GSTATS_HASH:                                // "gstats"
		CG_parseWeaponStatsGS_cmd();
		return;
	// "topshots"-related commands
	case ASTATS_HASH:                                // "astats"
		CG_parseTopShotsStats_cmd(qtrue, CG_printConsoleString);
		return;
	case ASTATSB_HASH:                              // "astatsb"
		CG_parseTopShotsStats_cmd(qfalse, CG_printConsoleString);
		return;
	case BSTATS_HASH:                                // "bstats"
		CG_parseBestShotsStats_cmd(qtrue, CG_printConsoleString);
		return;
	case BSTATSB_HASH:                               // "bstatsb"
		CG_parseBestShotsStats_cmd(qfalse, CG_printConsoleString);
		return;
	case WBSTATS_HASH:                               // "wbstats"
		CG_topshotsParse_cmd(qtrue);
		return;
	// single weapon stat (requested weapon stats)
	case RWS_HASH:                                   // "rws"
		CG_ParseWeaponStats();
		return;
	case MAP_RESTART_HASH:                           // "map_restart"
		CG_MapRestart();
		return;
	case PORTALCAMPOS_HASH:                          // "portalcampos"
		CG_ParsePortalPos();
		return;
	case REMAPSHADER_HASH:                           // "remapShader"
	{
		if (trap_Argc() == 4)
		{
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];

			Q_strncpyz(shader1, CG_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, CG_Argv(2), sizeof(shader2));
			Q_strncpyz(shader3, CG_Argv(3), sizeof(shader3));

			trap_R_RemapShader(shader1, shader2, shader3);
		}
		return;
	}
	// ensure a file gets into a build (mainly for scripted music calls)
	case ADDTOBUILD_HASH:                            // "addToBuild"
	{
		fileHandle_t f;

		if (!cg_buildScript.integer)
		{
			return;
		}

		// just open the file so it gets copied to the build dir
		//CG_FileTouchForBuild(CG_Argv(1));
		trap_FS_FOpenFile(CG_Argv(1), &f, FS_READ);
		trap_FS_FCloseFile(f);
		return;
	}
	// server sends this command when it's about to kill the current server, before the client can reconnect
	case SPAWNSERVER_HASH:                     // "spawnserver"
		// print message informing player the server is restarting with a new map
		CG_PriorityCenterPrint(va("%s", CG_TranslateString("^3Server Restarting")), 100, cg_fontScaleTP.value, 999999);

		// hack here
		cg.serverRespawning = qtrue;

		// fade out over the course of 5 seconds, should be enough (nuking: atvi bug 3793)
		return;
	case APPLICATION_HASH:                                         //  "application"
		cgs.applicationEndTime = cg.time + 20000;
		cgs.applicationClient  = Q_atoi(CG_Argv(1));

		if (cgs.applicationClient < 0)
		{
			cgs.applicationEndTime = cg.time + 10000;
		}
		return;
	case INVITATION_HASH:                                          // "invitation"
		cgs.invitationEndTime = cg.time + 20000;
		cgs.invitationClient  = Q_atoi(CG_Argv(1));

		if (cgs.invitationClient < 0)
		{
			cgs.invitationEndTime = cg.time + 10000;
		}
		return;
	case PROPOSITION_HASH:                                        // "proposition"
		cgs.propositionEndTime = cg.time + 20000;
		cgs.propositionClient  = Q_atoi(CG_Argv(1));
		cgs.propositionClient2 = Q_atoi(CG_Argv(2));

		if (cgs.propositionClient < 0)
		{
			cgs.propositionEndTime = cg.time + 10000;
		}
		return;
	case AFT_HASH:                                                // "aft"
		cgs.autoFireteamEndTime = cg.time + 20000;
		cgs.autoFireteamNum     = Q_atoi(CG_Argv(1));

		if (cgs.autoFireteamNum < -1)
		{
			cgs.autoFireteamEndTime = cg.time + 10000;
		}
		return;
	case AFTC_HASH:                                              // "aftc"
		cgs.autoFireteamCreateEndTime = cg.time + 20000;
		cgs.autoFireteamCreateNum     = Q_atoi(CG_Argv(1));

		if (cgs.autoFireteamCreateNum < -1)
		{
			cgs.autoFireteamCreateEndTime = cg.time + 10000;
		}
		return;
	case AFTJ_HASH:                                              // "aftj"
		cgs.autoFireteamJoinEndTime = cg.time + 20000;
		cgs.autoFireteamJoinNum     = Q_atoi(CG_Argv(1));

		if (cgs.autoFireteamJoinNum < -1)
		{
			cgs.autoFireteamJoinEndTime = cg.time + 10000;
		}
		return;
	// Allow client to lodge a complaing
	case COMPLAINT_HASH:                                       // "complaint"
		if (cgs.gamestate == GS_PLAYING)
		{
			if (cg_complaintPopUp.integer == 0)
			{
				trap_SendClientCommand("vote no");
			}

			cgs.complaintEndTime = cg.time + 20000;
			cgs.complaintClient  = Q_atoi(CG_Argv(1));

			if (cgs.complaintClient < 0)
			{
				cgs.complaintEndTime = cg.time + 10000;
			}
		}
		return;
	case REQFORCESPAWN_HASH:                               // "reqforcespawn"
		if (cg_instanttapout.integer || cgs.gamestate != GS_PLAYING)
		{
			CG_ForceTapOut_f();
		}
		else
		{
			if (cgs.gametype == GT_WOLF_LMS)
			{
				trap_UI_Popup(UIMENU_WM_TAPOUT_LMS);
			}
			else
			{
				trap_UI_Popup(UIMENU_WM_TAPOUT);
			}
		}
		return;
	case SDBG_HASH:                                         // "sdbg"
		CG_StatsDebugAddText(CG_Argv(1));
		return;
	case IMMAPLIST_HASH: // MAPVOTE                         "immaplist"
		CG_parseMapVoteListInfo();
		return;
	case IMVOTETALLY_HASH: // MAPVOTE                      "imvotetally"
		CG_parseMapVoteTally();
		return;
	case SETSPAWNPT_HASH: //  "setspawnpt"
		cgs.ccSelectedSpawnPoint = Q_atoi(CG_Argv(1)) + 1;
		return;
	// debriefing server cmds
	case IMWA_HASH:                                         // "imwa"
		CG_Debriefing_ParseWeaponAccuracies();
		return;
	case IMWS_HASH:                                        // "imws"
		CG_Debriefing_ParseWeaponStats();
		return;
	case IMPKD_HASH:                                       // "impkd"
		CG_Debriefing_ParsePlayerKillsDeaths();
		return;
	case IMPT_HASH:                                        // "impt"
		CG_Debriefing_ParsePlayerTime();
		return;
#ifdef FEATURE_RATING
	case IMSR_HASH:                                        // "imsr"
		if (cgs.skillRating)
		{
			CG_Debriefing_ParseSkillRating();
		}
		return;
	case SR_HASH:                                          // "sr" - backward compatibility with 2.76 demos
		if (cgs.skillRating)
		{
			CG_ParseSkillRating(1);
		}
		return;
	case SRA_HASH:                                         // "sra"
		if (cgs.skillRating)
		{
			CG_ParseSkillRating(2);
		}
		return;
#endif
#ifdef FEATURE_PRESTIGE
	case IMPR_HASH:                                        // "impr"
		if (cgs.prestige)
		{
			CG_Debriefing_ParsePrestige();
		}
		return;
	case PR_HASH:                                          // "pr"
		if (cgs.prestige)
		{
			CG_ParsePrestige();
		}
		return;
#endif
	// music
	// loops \/
	case MU_START_HASH:                                      // "mu_start" has optional parameter for fade-up time
	{
		char text[MAX_SAY_TEXT];
		int  fadeTime = 0;   // default to instant start

		Q_strncpyz(text, CG_Argv(2), MAX_SAY_TEXT);
		if (*text)
		{
			fadeTime = Q_atoi(text);
		}

		trap_S_StartBackgroundTrack(CG_Argv(1), CG_Argv(1), fadeTime);
		return;
	}
	// plays once then back to whatever the loop was \/
	case MU_PLAY_HASH:                        // "mu_play" has optional parameter for fade-up time
	{
		char text[MAX_SAY_TEXT];
		int  fadeTime = 0;   // default to instant start

		Q_strncpyz(text, CG_Argv(2), MAX_SAY_TEXT);
		if (*text)
		{
			fadeTime = Q_atoi(text);
		}

		trap_S_StartBackgroundTrack(CG_Argv(1), "onetimeonly", fadeTime);
		return;
	}
	case MU_STOP_HASH:                                  // "mu_stop" has optional parameter for fade-down time
	{
		char text[MAX_SAY_TEXT];
		int  fadeTime = 0;   // default to instant stop

		Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
		if (*text)
		{
			fadeTime = Q_atoi(text);
		}

		trap_S_FadeBackgroundTrack(0.0f, fadeTime, 0);
		trap_S_StartBackgroundTrack("", "", -2);   // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
		return;
	}
	case MU_FADE_HASH:                                 // "mu_fade"
		trap_S_FadeBackgroundTrack(atof(CG_Argv(1)), Q_atoi(CG_Argv(2)), 0);
		return;
	case SND_FADE_HASH:                                // "snd_fade"
		trap_S_FadeAllSound(atof(CG_Argv(1)), Q_atoi(CG_Argv(2)), Q_atoi(CG_Argv(3)));
		return;
	case ROCKANDROLL_HASH: // "rockandroll"
		trap_S_FadeAllSound(1.0f, 1000, qfalse);      // fade sound up
		return;
	case BP_HASH: // "bp"
		CG_WordWrapString(CG_Argv(1), Com_Clamp(50, 80, (int)(cgs.screenXScale * 40.f)), cg.bannerPrint, sizeof(cg.bannerPrint));
		cg.bannerPrintTime = cg.time;
		break;
	default:
		CG_Printf("Unknown client game command: %s [%lu]\n", cmd, BG_StringHashValue(cmd));
		break;
	}
}

/**
 * @brief Execute all of the server commands that were received along
 * with this this snapshot.
 * @param[in] latestSequence
 */
void CG_ExecuteNewServerCommands(int latestSequence)
{
	while (cgs.serverCommandSequence < latestSequence)
	{
		if (trap_GetServerCommand(++cgs.serverCommandSequence))
		{
			CG_ServerCommand();
		}
	}
}
