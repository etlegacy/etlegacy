/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file cg_servercmds.c
 * @brief reliably sequenced text commands sent by the server
 *
 * these are processed at snapshot transition time, so there will definately be
 * a valid snapshot this frame
 */

#include "cg_local.h"

/*
=================
CG_ParseScores
=================
*/
// NOTE: team doesnt actually signify team, think i was on drugs that day.....
static void CG_ParseScore(team_t team)
{
	int i, j, powerups;
	int numScores;
	int offset;

	if (team == TEAM_AXIS)
	{
		cg.numScores = 0;

		cg.teamScores[0] = atoi(CG_Argv(1));
		cg.teamScores[1] = atoi(CG_Argv(2));

		offset = 4;
	}
	else
	{
		offset = 2;
	}

	numScores = atoi(CG_Argv(offset - 1));

	for (j = 0; j < numScores; j++)
	{
		i = cg.numScores;

		cg.scores[i].client       = atoi(CG_Argv(offset + 0 + (j * 7)));
		cg.scores[i].score        = atoi(CG_Argv(offset + 1 + (j * 7)));
		cg.scores[i].ping         = atoi(CG_Argv(offset + 2 + (j * 7)));
		cg.scores[i].time         = atoi(CG_Argv(offset + 3 + (j * 7)));
		powerups                  = atoi(CG_Argv(offset + 4 + (j * 7)));
		cg.scores[i].scoreflags   = atoi(CG_Argv(offset + 5 + (j * 7)));
		cg.scores[i].respawnsLeft = atoi(CG_Argv(offset + 6 + (j * 7)));

		if (cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS)
		{
			cg.scores[i].client = 0;
		}

		cgs.clientinfo[cg.scores[i].client].score    = cg.scores[i].score;
		cgs.clientinfo[cg.scores[i].client].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;

		cg.numScores++;
	}
}

/*
=================
CG_ParseTeamInfo
=================
*/
#define NUMARGS 5
static void CG_ParseTeamInfo(void)
{
	int i;
	int client;
	int numSortedTeamPlayers;

	numSortedTeamPlayers = atoi(CG_Argv(1));

	if (numSortedTeamPlayers < 0 || numSortedTeamPlayers >= MAX_CLIENTS)
	{
		CG_Printf("CG_ParseTeamInfo: numSortedTeamPlayers out of range (%i)", numSortedTeamPlayers);
		return;
	}

	for (i = 0 ; i < numSortedTeamPlayers ; i++)
	{
		client = atoi(CG_Argv(i * NUMARGS + 2));

		if (client < 0 || client >= MAX_CLIENTS)
		{
			CG_Printf("CG_ParseTeamInfo: bad client number: %i", client);
			return;
		}

		cgs.clientinfo[client].location[0] = atoi(CG_Argv(i * NUMARGS + 3));
		cgs.clientinfo[client].location[1] = atoi(CG_Argv(i * NUMARGS + 4));
		cgs.clientinfo[client].health      = atoi(CG_Argv(i * NUMARGS + 5));
		cgs.clientinfo[client].powerups    = atoi(CG_Argv(i * NUMARGS + 6));
	}
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo(void)
{
	const char *info = CG_ConfigString(CS_SERVERINFO);
	char       *mapname;

	cg_gameType.integer = cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	cg_antilag.integer  = cgs.antilag = atoi(Info_ValueForKey(info, "g_antilag"));
	if (!cgs.localServer)
	{
		trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
		trap_Cvar_Set("g_antilag", va("%i", cgs.antilag));
		trap_Cvar_Update(&cg_antilag);
		trap_Cvar_Update(&cg_gameType);
	}
	cgs.timelimit  = atof(Info_ValueForKey(info, "timelimit"));
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	mapname        = Info_ValueForKey(info, "mapname");
	Q_strncpyz(cgs.rawmapname, mapname, sizeof(cgs.rawmapname));
	Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);

	// prolly should parse all CS_SERVERINFO keys automagically, but I don't want to break anything that might be improperly set for wolf SP, so I'm just parsing MP relevant stuff here
	trap_Cvar_Set("g_redlimbotime", Info_ValueForKey(info, "g_redlimbotime"));
	cg_redlimbotime.integer = atoi(Info_ValueForKey(info, "g_redlimbotime"));
	trap_Cvar_Set("g_bluelimbotime", Info_ValueForKey(info, "g_bluelimbotime"));
	cg_bluelimbotime.integer = atoi(Info_ValueForKey(info, "g_bluelimbotime"));
	cgs.weaponRestrictions   = atoi(Info_ValueForKey(info, "g_heavyWeaponRestriction")) * 0.01f;

	cgs.minclients = atoi(Info_ValueForKey(info, "g_minGameClients")); //  overloaded for ready counts

	// make this available for ingame_callvote
	trap_Cvar_Set("cg_ui_voteFlags", ((authLevel.integer == RL_NONE) ? Info_ValueForKey(info, "voteFlags") : "0"));
}

qboolean CG_inSVCVARBackupList(const char *cvar1)
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

void CG_UpdateSvCvars(void)
{
	const char *info = CG_ConfigString(CS_SVCVAR);
	int        i;
	char       *token;
	char       *buffer;

	cg.svCvarCount = atoi(Info_ValueForKey(info, "N"));

	for (i = 0; i < cg.svCvarCount; i++)
	{
		// get what is it
		buffer = Info_ValueForKey(info, va("V%i", i));
		// get a mode pf ot
		token              = strtok(buffer, " ");
		cg.svCvars[i].mode = atoi(token);

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

void CG_ParseLegacyinfo(void)
{
	const char *info = CG_ConfigString(CS_LEGACYINFO);

	cgs.mapVoteMapX = atoi(Info_ValueForKey(info, "X"));
	cgs.mapVoteMapY = atoi(Info_ValueForKey(info, "Y"));
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup(void)
{
	const char *info  = CG_ConfigString(CS_WARMUP);
	int        warmup = atoi(info);

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

/*
==================
CG_ParseOIDInfo
==================
*/

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

void CG_ParseOIDInfo(int num)
{
	const char *info = CG_ConfigString(num);
	const char *cs;
	int        index = num - CS_OID_DATA;

	memset(&cgs.oidInfo[index], 0, sizeof(cgs.oidInfo[0]));

	if (!info || !*info)
	{
		return;
	}

	cs = Info_ValueForKey(info, "s");
	if (cs && *cs)
	{
		cgs.oidInfo[index].spawnflags = atoi(cs);
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
		cgs.oidInfo[index].objflags = atoi(cs);
	}

	cs = Info_ValueForKey(info, "e");
	if (cs && *cs)
	{
		cgs.oidInfo[index].entityNum = atoi(cs);
	}

	cs = Info_ValueForKey(info, "n");
	if (cs && *cs)
	{
		Q_strncpyz(cgs.oidInfo[index].name, cs, sizeof(cgs.oidInfo[0].name));
	}

	cs = Info_ValueForKey(info, "x");
	if (cs && *cs)
	{
		cgs.oidInfo[index].origin[0] = atoi(cs);
	}

	cs = Info_ValueForKey(info, "y");
	if (cs && *cs)
	{
		cgs.oidInfo[index].origin[1] = atoi(cs);
	}

	cs = Info_ValueForKey(info, "z");
	if (cs && *cs)
	{
		cgs.oidInfo[index].origin[2] = atoi(cs);
	}
}

void CG_ParseOIDInfos(void)
{
	int i;

	for (i = 0; i < MAX_OID_TRIGGERS; i++)
	{
		CG_ParseOIDInfo(CS_OID_DATA + i);
	}
}

/*
==================
CG_ParseWolfinfo
==================
*/
void CG_ParseWolfinfo(void)
{
	int        old_gs = cgs.gamestate;
	const char *info  = CG_ConfigString(CS_WOLFINFO);

	cgs.currentRound       = atoi(Info_ValueForKey(info, "g_currentRound"));
	cgs.nextTimeLimit      = atof(Info_ValueForKey(info, "g_nextTimeLimit"));
	cgs.gamestate          = atoi(Info_ValueForKey(info, "gamestate"));
	cgs.currentCampaign    = Info_ValueForKey(info, "g_currentCampaign");
	cgs.currentCampaignMap = atoi(Info_ValueForKey(info, "g_currentCampaignMap"));

	// Announce game in progress if we are really playing
	if (old_gs != GS_PLAYING && cgs.gamestate == GS_PLAYING)
	{
		trap_S_StartLocalSound(cgs.media.countFight, CHAN_ANNOUNCER);

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

/*
==================
CG_ParseSpawns
==================
*/
void CG_ParseSpawns(void)
{
	const char *info = CG_ConfigString(CS_MULTI_INFO);
	const char *s;
	int        i;
	int        newteam;

	s = Info_ValueForKey(info, "numspawntargets");

	if (!s || !strlen(s))
	{
		return;
	}

	// first index is for autopicking
	Q_strncpyz(cg.spawnPoints[0], CG_TranslateString("Auto Pick"), MAX_SPAWNDESC);

	cg.spawnCount = atoi(s) + 1;

	for (i = 1; i < cg.spawnCount; i++)
	{
		info = CG_ConfigString(CS_MULTI_SPAWNTARGETS + i - 1);

		s = Info_ValueForKey(info, "spawn_targ");

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
		cg.spawnCoordsUntransformed[i][0] = cg.spawnCoords[i][0] = atof(s);

		s = Info_ValueForKey(info, "y");
		if (!s || !strlen(s))
		{
			return;
		}
		cg.spawnCoordsUntransformed[i][1] = cg.spawnCoords[i][1] = atof(s);

		if (cgs.ccLayers)
		{
			s = Info_ValueForKey(info, "z");
			if (!s || !strlen(s))
			{
				return;
			}
			cg.spawnCoordsUntransformed[i][2] = cg.spawnCoords[i][2] = atof(s);
		}

		CG_TransformToCommandMapCoord(&cg.spawnCoords[i][0], &cg.spawnCoords[i][1]);

		s = Info_ValueForKey(info, "t");

		newteam = atoi(s);
		if (cg.spawnTeams[i] != newteam)
		{
			cg.spawnTeams_old[i]        = cg.spawnTeams[i];
			cg.spawnTeams_changeTime[i] = cg.time;
			cg.spawnTeams[i]            = newteam;
		}

		s                       = Info_ValueForKey(info, "c");
		cg.spawnPlayerCounts[i] = atoi(s);
	}
}

/*
=====================
CG_ParseScreenFade
=====================
*/
static void CG_ParseScreenFade(void)
{
	const char *info = CG_ConfigString(CS_SCREENFADE);
	char       *token;

	token         = COM_Parse((char **)&info);
	cgs.fadeAlpha = atof(token);

	token             = COM_Parse((char **)&info);
	cgs.fadeStartTime = atoi(token);
	token             = COM_Parse((char **)&info);
	cgs.fadeDuration  = atoi(token);

	if (cgs.fadeStartTime + cgs.fadeDuration < cg.time)
	{
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
}

/*
==============
CG_ParseFog
    float near dist
    float far dist
    float density
    float[3] r,g,b
    int     time
==============
*/
static void CG_ParseFog(void)
{
	const char *info = CG_ConfigString(CS_FOGVARS);
	char       *token;
	float      ne, fa, r, g, b, density;
	int        time;

	token   = COM_Parse((char **)&info);
	ne      = atof(token);
	token   = COM_Parse((char **)&info);
	fa      = atof(token);
	token   = COM_Parse((char **)&info);
	density = atof(token);
	token   = COM_Parse((char **)&info);
	r       = atof(token);
	token   = COM_Parse((char **)&info);
	g       = atof(token);
	token   = COM_Parse((char **)&info);
	b       = atof(token);
	token   = COM_Parse((char **)&info);
	time    = atoi(token);

	if (fa)        // far of '0' from a target_fog means "return to map fog"
	{
		trap_R_SetFog(FOG_SERVER, (int)ne, (int)fa, r, g, b, density + .1);
		trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_SERVER, time, 0, 0, 0, 0);
	}
	else
	{
		trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, time, 0, 0, 0, 0);
	}
}

static void CG_ParseGlobalFog(void)
{
	const char *info = CG_ConfigString(CS_GLOBALFOGVARS);
	char       *token;
	qboolean   restore;
	int        duration;

	token    = COM_Parse((char **)&info);
	restore  = atoi(token);
	token    = COM_Parse((char **)&info);
	duration = atoi(token);

	if (restore)
	{
		trap_R_SetGlobalFog(qtrue, duration, 0.f, 0.f, 0.f, 0);
	}
	else
	{
		float r, g, b, depthForOpaque;

		token          = COM_Parse((char **)&info);
		r              = atof(token);
		token          = COM_Parse((char **)&info);
		g              = atof(token);
		token          = COM_Parse((char **)&info);
		b              = atof(token);
		token          = COM_Parse((char **)&info);
		depthForOpaque = atof(token);

		trap_R_SetGlobalFog(qfalse, duration, r, g, b, depthForOpaque);
	}
}

// Parse server version info (for demo playback compatibility)
void CG_ParseServerVersionInfo(const char *pszVersionInfo)
{
	// This will expand to a tokenized string, eventually but for
	// now we only need to worry about 1 number :)
	cgs.game_versioninfo = atoi(pszVersionInfo);
}

// Parse reinforcement offsets
void CG_ParseReinforcementTimes(const char *pszReinfSeedString)
{
	const char   *tmp = pszReinfSeedString, *tmp2;
	unsigned int i, j, dwDummy, dwOffset[TEAM_NUM_TEAMS];

#define GETVAL(x, y) if ((tmp = strchr(tmp, ' ')) == NULL) { return; } x = atoi(++tmp) / y;

	dwOffset[TEAM_ALLIES] = atoi(pszReinfSeedString) >> REINF_BLUEDELT;
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
			GETVAL(dwDummy, 1); // FIXME: inspect this, from cppcheck:
			                    // Variable 'dwDummy' is assigned a value that is never used
			                    // (++tmp?)
		}
	}
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues(void)
{
	cgs.levelStartTime        = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
	cgs.intermissionStartTime = atoi(CG_ConfigString(CS_INTERMISSION_START_TIME));
	cg.warmup                 = atoi(CG_ConfigString(CS_WARMUP));

	// set all of this crap in cgs - it won't be set if it doesn't
	// change, otherwise.  consider:
	// vote was called 5 minutes ago for 'Match Reset'.  you connect.
	// you're sent that value for CS_VOTE_STRING, but ignore it, so
	// you have nothing to use if another 'Match Reset' vote is called
	// (no update will be sent because the string will be the same.)

	cgs.voteTime = atoi(CG_ConfigString(CS_VOTE_TIME));
	cgs.voteYes  = atoi(CG_ConfigString(CS_VOTE_YES));
	cgs.voteNo   = atoi(CG_ConfigString(CS_VOTE_NO));
	Q_strncpyz(cgs.voteString, CG_ConfigString(CS_VOTE_STRING), sizeof(cgs.voteString));

	cg.teamFirstBlood = atoi(CG_ConfigString(CS_FIRSTBLOOD));
	// yes, the order is this way on purpose. not my fault!
	cg.teamWonRounds[1] = atoi(CG_ConfigString(CS_ROUNDSCORES1));
	cg.teamWonRounds[0] = atoi(CG_ConfigString(CS_ROUNDSCORES2));

	CG_ParseServerVersionInfo(CG_ConfigString(CS_VERSIONINFO));
	CG_ParseReinforcementTimes(CG_ConfigString(CS_REINFSEEDS));
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged(void)
{
	char       originalShader[MAX_QPATH];
	char       newShader[MAX_QPATH];
	char       timeOffset[16];
	const char *o = CG_ConfigString(CS_SHADERSTATE);
	char       *n, *t;

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

/*
===============
CG_ChargeTimesChanged
===============
*/
void CG_ChargeTimesChanged(void)
{
	const char *info = CG_ConfigString(CS_CHARGETIMES);

	cg.soldierChargeTime[0]   = atoi(Info_ValueForKey(info, "x0"));
	cg.soldierChargeTime[1]   = atoi(Info_ValueForKey(info, "a0"));
	cg.medicChargeTime[0]     = atoi(Info_ValueForKey(info, "x1"));
	cg.medicChargeTime[1]     = atoi(Info_ValueForKey(info, "a1"));
	cg.engineerChargeTime[0]  = atoi(Info_ValueForKey(info, "x2"));
	cg.engineerChargeTime[1]  = atoi(Info_ValueForKey(info, "a2"));
	cg.ltChargeTime[0]        = atoi(Info_ValueForKey(info, "x3"));
	cg.ltChargeTime[1]        = atoi(Info_ValueForKey(info, "a3"));
	cg.covertopsChargeTime[0] = atoi(Info_ValueForKey(info, "x4"));
	cg.covertopsChargeTime[1] = atoi(Info_ValueForKey(info, "a4"));
}

/*
===============
CG_TeamRestrictionsChanged
===============
*/
void CG_TeamRestrictionsChanged(void)
{
	const char *info = CG_ConfigString(CS_TEAMRESTRICTIONS);

	Q_strncpyz(cg.maxSoldiers, Info_ValueForKey(info, "c0"), sizeof(cg.maxSoldiers));
	Q_strncpyz(cg.maxMedics, Info_ValueForKey(info, "c1"), sizeof(cg.maxMedics));
	Q_strncpyz(cg.maxEngineers, Info_ValueForKey(info, "c2"), sizeof(cg.maxEngineers));
	Q_strncpyz(cg.maxFieldops, Info_ValueForKey(info, "c3"), sizeof(cg.maxFieldops));
	Q_strncpyz(cg.maxCovertops, Info_ValueForKey(info, "c4"), sizeof(cg.maxCovertops));
	Q_strncpyz(cg.maxMortars, Info_ValueForKey(info, "w0"), sizeof(cg.maxMortars));
	Q_strncpyz(cg.maxFlamers, Info_ValueForKey(info, "w1"), sizeof(cg.maxFlamers));
	Q_strncpyz(cg.maxMg42s, Info_ValueForKey(info, "w2"), sizeof(cg.maxMg42s));
	Q_strncpyz(cg.maxPanzers, Info_ValueForKey(info, "w3"), sizeof(cg.maxPanzers));
	Q_strncpyz(cg.maxRiflegrenades, Info_ValueForKey(info, "w4"), sizeof(cg.maxRiflegrenades));
	cg.maxPlayers = atoi(Info_ValueForKey(info, "m"));
}

/*
================
CG_ConfigStringModified
================
*/
static void CG_ConfigStringModified(void)
{
	int num;

	num = atoi(CG_Argv(1));

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
		cg.teamFirstBlood = atoi(CG_ConfigString(num));
		break;
	case CS_ROUNDSCORES1:
		cg.teamWonRounds[1] = atoi(CG_ConfigString(num));
		break;
	case CS_ROUNDSCORES2:
		cg.teamWonRounds[0] = atoi(CG_ConfigString(num));
		break;
	case CS_VERSIONINFO:
		CG_ParseServerVersionInfo(CG_ConfigString(num));           // set versioning info for older demo playback
		break;
	case CS_REINFSEEDS:
		CG_ParseReinforcementTimes(CG_ConfigString(num));          // set reinforcement times for each team
		break;
	case CS_LEVEL_START_TIME:
		cgs.levelStartTime = atoi(CG_ConfigString(num));
		break;
	case CS_INTERMISSION_START_TIME:
		cgs.intermissionStartTime = atoi(CG_ConfigString(num));
		break;
	case CS_VOTE_TIME:
		cgs.voteTime     = atoi(CG_ConfigString(num));
		cgs.voteModified = qtrue;
		break;
	case CS_VOTE_YES:
		cgs.voteYes      = atoi(CG_ConfigString(num));
		cgs.voteModified = qtrue;
		break;
	case CS_VOTE_NO:
		cgs.voteNo       = atoi(CG_ConfigString(num));
		cgs.voteModified = qtrue;
		break;
	case CS_VOTE_STRING:
		Q_strncpyz(cgs.voteString, CG_ConfigString(num), sizeof(cgs.voteString));
		break;
	case CS_INTERMISSION:
		cg.intermissionStarted = atoi(CG_ConfigString(num));
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
	case CS_SKYBOXORG:
		CG_ParseSkyBox();
		break;
	case CS_ALLIED_MAPS_XP:
	case CS_AXIS_MAPS_XP:
		CG_ParseTeamXPs(num - CS_AXIS_MAPS_XP);
		break;
	case CS_FILTERCAMS:
		cg.filtercams = atoi(CG_ConfigString(num)) ? qtrue : qfalse;
		break;
	case CS_LEGACYINFO:
		CG_ParseLegacyinfo();
		break;

	default:
		if (num >= CS_MULTI_SPAWNTARGETS && num < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS)
		{
			CG_ParseSpawns();
		}
		else if (num >= CS_MODELS && num < CS_MODELS + MAX_MODELS)
		{
			cgs.gameModels[num - CS_MODELS] = trap_R_RegisterModel(CG_ConfigString(num));
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

/*
=======================
CG_AddToTeamChat
=======================
*/
static void CG_AddToTeamChat(const char *str, int clientnum)
{
	int  len;
	char *p, *ls;
	int  lastcolor;
	int  chatHeight;

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
	{
		chatHeight = cg_teamChatHeight.integer;
	}
	else
	{
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0 || cg_teamChatTime.integer <= 0)
	{
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;

	p  = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str)
	{
		if (len > TEAMCHAT_WIDTH - 1)
		{
			if (ls)
			{
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
			cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = cgs.clientinfo[clientnum].team;

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

	cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = cgs.clientinfo[clientnum].team;
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

			cgs.notifyMsgTimes[cgs.notifyPos % chatHeight] = cg.time;

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
			// gcc warning: value computed is not used was *str++;
			str++;
		}

		if (*str)
		{
			*p++ = *str++;
			len++;
		}
	}
	*p = 0;

	cgs.notifyMsgTimes[cgs.notifyPos % chatHeight] = cg.time;
	cgs.notifyPos++;

	if (cgs.notifyPos - cgs.notifyLastPos > chatHeight)
	{
		cgs.notifyLastPos = cgs.notifyPos - chatHeight;
	}
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart(void)
{
	if (cg_showmiss.integer)
	{
		CG_Printf("CG_MapRestart\n");
	}

	memset(&cg.lastWeapSelInBank[0], 0, MAX_WEAP_BANKS_MP * sizeof(int)); // clear weapon bank selections

	cg.numbufferedSoundScripts = 0;

	cg.centerPrintTime = 0; // reset centerprint counter so previous messages don't re-appear
	cg.cursorHintFade  = 0; // reset cursor hint timer

	// Reset complaint system
	cgs.complaintClient  = -1;
	cgs.complaintEndTime = 0;

	CG_LimboPanel_RequestObjective();

	// clear zoom (so no warpies)
	cg.zoomedBinoc = qfalse;
	cg.zoomedScope = qfalse;
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
	memset(&cg.pmext, 0, sizeof(cg.pmext));

	cg.pmext.bAutoReload = (cg_autoReload.integer > 0);

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

	cg.filtercams = atoi(CG_ConfigString(CS_FILTERCAMS)) ? qtrue : qfalse;

	CG_ChargeTimesChanged();

	CG_ParseFireteams();

	CG_ParseOIDInfos();

	CG_InitPM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

	trap_Cvar_Set("cg_thirdPerson", "0");
}

#define MAX_VOICEFILES      8

// we've got some really big VO files now
#define MAX_VOICEFILESIZE   32768
#define MAX_VOICECHATS      272
// NOTE: If we're worried about space - do we really need 96 possible sounds for any one chat?
//          I think this is used to allow multiple sound clips for one command, so do we need 96 available selection sounds?
#define MAX_VOICESOUNDS     32

#define MAX_CHATSIZE        64
#define MAX_HEADMODELS      64

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

/*
=================
CG_ParseVoiceChats
=================
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
	if (!token || token[0] == 0)
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
		if (!token || token[0] == 0)
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
			if (!token || token[0] == 0)
			{
				return qtrue;
			}
			if (!Q_stricmp(token, "}"))
			{
				break;
			}
			voiceChats[voiceChatList->numVoiceChats].sounds[current] = trap_S_RegisterSound(token, compress);
			token                                                    = COM_ParseExt(p, qtrue);
			if (!token || token[0] == 0)
			{
				return qtrue;
			}
			Com_sprintf(voiceChats[voiceChatList->numVoiceChats].chats[current], MAX_CHATSIZE, "%s", token);

			// Specify sprite shader to show above player's head
			token = COM_ParseExt(p, qfalse);
			if (!Q_stricmp(token, "}") || !token || token[0] == 0)
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
	return qtrue;
}

/*
=================
CG_LoadVoiceChats
=================
*/
void CG_LoadVoiceChats(void)
{
	voiceChatLists[0].numVoiceChats = 0;
	voiceChatLists[1].numVoiceChats = 0;

	CG_ParseVoiceChats("scripts/wm_axis_chat.voice", &voiceChatLists[0], MAX_VOICECHATS);
	CG_ParseVoiceChats("scripts/wm_allies_chat.voice", &voiceChatLists[1], MAX_VOICECHATS);
}

/*
=================
CG_HeadModelVoiceChats
=================
*/
int CG_HeadModelVoiceChats(char *filename)
{
	int          len, i;
	fileHandle_t f;
	char         buf[MAX_VOICEFILESIZE];
	char         **p, *ptr;
	char         *token;

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (!f)
	{
		trap_Print(va("voice chat file not found: %s\n", filename));
		return -1;
	}
	if (len >= MAX_VOICEFILESIZE)
	{
		trap_Print(va(S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i", filename, len, MAX_VOICEFILESIZE));
		trap_FS_FCloseFile(f);
		return -1;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	ptr = buf;
	p   = &ptr;

	token = COM_ParseExt(p, qtrue);
	if (!token || token[0] == 0)
	{
		return -1;
	}

	for (i = 0; i < MAX_VOICEFILES; i++)
	{
		if (!Q_stricmp(token, voiceChatLists[i].name))
		{
			return i;
		}
	}

	//FIXME: maybe try to load the .voice file which name is stored in token?

	return -1;
}

/*
=================
CG_GetVoiceChat
=================
*/
int CG_GetVoiceChat(voiceChatList_t *voiceChatList, const char *id, sfxHandle_t *snd, qhandle_t *sprite, char **chat)
{
	int i, rnd;

	for (i = 0; i < voiceChatList->numVoiceChats; i++)
	{
		if (!Q_stricmp(id, voiceChatList->voiceChats[i].id))
		{
			rnd     = random() * voiceChatList->voiceChats[i].numSounds;
			*snd    = voiceChatList->voiceChats[i].sounds[rnd];
			*sprite = voiceChatList->voiceChats[i].sprite[rnd];
			*chat   = voiceChatList->voiceChats[i].chats[rnd];
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
CG_VoiceChatListForClient
=================
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

/*
=================
CG_PlayVoiceChat
=================
*/
void CG_PlayVoiceChat(bufferedVoiceChat_t *vchat)
{
	if (!cg_noVoiceChats.integer)
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
	if (!vchat->voiceOnly && !cg_noVoiceText.integer)
	{
		CG_AddToTeamChat(vchat->message, vchat->clientNum);
		CG_Printf("[skipnotify]: %s\n", vchat->message);
		CG_WriteToLog("%s\n", vchat->message);
	}
	voiceChatBuffer[cg.voiceChatBufferOut].snd = 0;
}

/*
=====================
CG_PlayBufferedVoieChats
=====================
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

/*
=====================
CG_AddBufferedVoiceChat
=====================
*/
void CG_AddBufferedVoiceChat(bufferedVoiceChat_t *vchat)
{
	// new system doesn't buffer but overwrites vchats FIXME put this on a cvar to choose which to use
	memcpy(&voiceChatBuffer[0], vchat, sizeof(bufferedVoiceChat_t));
	cg.voiceChatBufferIn = 0;
	CG_PlayVoiceChat(&voiceChatBuffer[0]);
}

/*
=================
CG_VoiceChatLocal
=================
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

	cgs.currentVoiceClient = clientNum;

	voiceChatList = CG_VoiceChatListForClient(clientNum);

	if (CG_GetVoiceChat(voiceChatList, cmd, &snd, &sprite, &chat))
	{
		//
		if (mode == SAY_TEAM || !cg_teamChatsOnly.integer)
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
				Com_sprintf(vchat.message, sizeof(vchat.message), "(%s)%c%c(%s): %c%c%s",
				            ci->name, Q_COLOR_ESCAPE, COLOR_YELLOW, loc, Q_COLOR_ESCAPE, color, CG_TranslateString(chat)); // FIXME: CG_TranslateString doesn't make sense here
			}
			else if (mode == SAY_BUDDY)
			{
				Com_sprintf(vchat.message, sizeof(vchat.message), "<%s>%c%c<%s>: %c%c%s",
				            ci->name, Q_COLOR_ESCAPE, COLOR_YELLOW, loc, Q_COLOR_ESCAPE, color, CG_TranslateString(chat));  // FIXME: CG_TranslateString doesn't make sense here
			}
			else
			{
				Com_sprintf(vchat.message, sizeof(vchat.message), "%s%c%c: %c%c%s",
				            ci->name, Q_COLOR_ESCAPE, COLOR_YELLOW, Q_COLOR_ESCAPE, color, CG_TranslateString(chat));  // FIXME: CG_TranslateString doesn't make sense here
			}
			CG_AddBufferedVoiceChat(&vchat);
		}
	}
}

/*
=================
CG_VoiceChat
=================
*/
void CG_VoiceChat(int mode)
{
	const char *cmd;
	int        clientNum, color;
	qboolean   voiceOnly;
	vec3_t     origin;

	voiceOnly = atoi(CG_Argv(1));
	clientNum = atoi(CG_Argv(2));
	color     = atoi(CG_Argv(3));

	if (mode != SAY_ALL)
	{
		origin[0] = atoi(CG_Argv(5));
		origin[1] = atoi(CG_Argv(6));
		origin[2] = atoi(CG_Argv(7));
	}

	cmd = CG_Argv(4);

	CG_VoiceChatLocal(mode, voiceOnly, clientNum, color, cmd, origin);
}

/*
=================
CG_RemoveChatEscapeChar
=================
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

/*
=================
CG_LocalizeServerCommand

  localize string sent from server

- localization is ON by default.
- use [lof] in string to turn OFF
- use [lon] in string to turn back ON
=================
*/
const char *CG_LocalizeServerCommand(const char *buf)
{
	static char token[MAX_TOKEN_CHARS];
	char        temp[MAX_TOKEN_CHARS];
	qboolean    togloc = qtrue;
	const char  *s     = buf;
	int         i, prev = 0;

	memset(token, 0, sizeof(token));

	for (i = 0; *s; i++, s++)
	{
		// line was: if ( *s == '[' && !Q_strncmp( s, "[lon]", 5 ) || !Q_strncmp( s, "[lof]", 5 ) ) {
		// || prevails on &&, gcc warning was 'suggest parentheses around && within ||'
		// modified to the correct behaviour
		if (*s == '[' && (!Q_strncmp(s, "[lon]", 5) || !Q_strncmp(s, "[lof]", 5)))
		{
			if (togloc)
			{
				memset(temp, 0, sizeof(temp));
				strncpy(temp, buf + prev, i - prev);
				strcat(token, CG_TranslateString(temp));
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
		memset(temp, 0, sizeof(temp));
		strncpy(temp, buf + prev, i - prev);
		strcat(token, CG_TranslateString(temp));
	}
	else
	{
		strncat(token, buf + prev, i - prev);
	}

	return token;
}

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

void CG_topshotsParse_cmd(qboolean doBest)
{
	int            iArg  = 1;
	int            iWeap = atoi(CG_Argv(iArg++));
	int            cnum, hits, atts, kills;
	topshotStats_t *ts = &cgs.topshots;
	char           name[32];
	float          acc;

	ts->cWeapons = 0;

	while (iWeap)
	{
		cnum  = atoi(CG_Argv(iArg++));
		hits  = atoi(CG_Argv(iArg++));
		atts  = atoi(CG_Argv(iArg++));
		kills = atoi(CG_Argv(iArg++));
		// unused
		//int deaths = atoi(CG_Argv(iArg++));
		acc = (atts > 0) ? (float)(hits * 100) / (float)atts : 0.0f;

		// bump up iArg since we didn't push it into deaths, above
		iArg++;

		if (ts->cWeapons < WS_MAX * 2)
		{
			BG_cleanName(cgs.clientinfo[cnum].name, name, 17, qfalse);
			Q_strncpyz(ts->strWS[ts->cWeapons++],
			           va("%-12s %5.1f %4d/%-4d %5d  %s",
			              aWeaponInfo[iWeap - 1].pszName,
			              acc, hits, atts,
			              kills,
			              name),
			           sizeof(ts->strWS[0]));
		}

		iWeap = atoi(CG_Argv(iArg++));
	}
}

void CG_ParseWeaponStats(void)
{
	cgs.ccWeaponShots = atoi(CG_Argv(1));
	cgs.ccWeaponHits  = atoi(CG_Argv(2));
}

void CG_ParsePortalPos(void)
{
	int i;

	cgs.ccCurrentCamObjective = atoi(CG_Argv(1));
	cgs.ccPortalEnt           = atoi(CG_Argv(8));

	for (i = 0; i < 3; i++)
	{
		cgs.ccPortalPos[i] = atoi(CG_Argv(i + 2));
	}

	for (i = 0; i < 3; i++)
	{
		cgs.ccPortalAngles[i] = atoi(CG_Argv(i + 5));
	}
}

// Cached stats
void CG_parseWeaponStatsGS_cmd(void)
{
	clientInfo_t *ci;
	gameStats_t  *gs = &cgs.gamestats;
	int          i, iArg = 1;
	int          nClientID  = atoi(CG_Argv(iArg++));
	int          nRounds    = atoi(CG_Argv(iArg++));
	int          weaponMask = atoi(CG_Argv(iArg++));
	int          skillMask, xp = 0;

	gs->cWeapons  = 0;
	gs->cSkills   = 0;
	gs->fHasStats = qfalse;

	gs->nClientID = nClientID;
	gs->nRounds   = nRounds;

	ci = &cgs.clientinfo[nClientID];

	if (weaponMask != 0)
	{
		char strName[MAX_STRING_CHARS];

		for (i = WS_KNIFE; i < WS_MAX; i++)
		{
			if (weaponMask & (1 << i))
			{
				int nHits      = atoi(CG_Argv(iArg++));
				int nShots     = atoi(CG_Argv(iArg++));
				int nKills     = atoi(CG_Argv(iArg++));
				int nDeaths    = atoi(CG_Argv(iArg++));
				int nHeadshots = atoi(CG_Argv(iArg++));

				Q_strncpyz(strName, va("%-12s  ", aWeaponInfo[i].pszName), sizeof(strName));
				if (nShots > 0 || nHits > 0)
				{
					Q_strcat(strName, sizeof(strName), va("%5.1f %4d/%-4d ",
					                                      ((nShots == 0) ? 0.0 : (float)(nHits * 100.0 / (float)nShots)),
					                                      nHits, nShots));
				}
				else
				{
					Q_strcat(strName, sizeof(strName), va("                "));
				}

				Q_strncpyz(gs->strWS[gs->cWeapons++],
				           va("%s%5d %6d%s", strName, nKills, nDeaths, ((aWeaponInfo[i].fHasHeadShots) ? va(" %9d", nHeadshots) : "")),
				           sizeof(gs->strWS[0]));

				if (nShots > 0 || nHits > 0 || nKills > 0 || nDeaths)
				{
					gs->fHasStats = qtrue;
				}
			}
		}

		if (gs->fHasStats)
		{
			int dmg_given = atoi(CG_Argv(iArg++));
			int dmg_rcvd  = atoi(CG_Argv(iArg++));
			int team_dmg  = atoi(CG_Argv(iArg++));

			Q_strncpyz(gs->strExtra[0], va("Damage Given: %-6d  Team Damage: %d", dmg_given, team_dmg), sizeof(gs->strExtra[0]));
			Q_strncpyz(gs->strExtra[1], va("Damage Recvd: %d", dmg_rcvd), sizeof(gs->strExtra[0]));
		}
	}

	// Derive XP from individual skill XP
	skillMask = atoi(CG_Argv(iArg++));
	for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
	{
		if (skillMask & (1 << i))
		{
			ci->skillpoints[i] = atoi(CG_Argv(iArg++));
			xp                += ci->skillpoints[i];
		}
	}

	Q_strncpyz(gs->strRank, va("%-13s %d", ((ci->team == TEAM_AXIS) ? rankNames_Axis : rankNames_Allies)[ci->rank], xp), sizeof(gs->strRank));

	if (skillMask != 0)
	{
		char *str;

		for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
		{
			if ((skillMask & (1 << i)) == 0)
			{
				continue;
			}

			if (ci->skill[i] < NUM_SKILL_LEVELS - 1)
			{
				str = va("%4d/%-4d", ci->skillpoints[i], skillLevels[i][ci->skill[i] + 1]);
			}
			else
			{
				str = va("%-8d", ci->skillpoints[i]);
			}

			if (cgs.gametype == GT_WOLF_CAMPAIGN)
			{
				Q_strncpyz(gs->strSkillz[gs->cSkills++], va("%-15s %3d %-15s %6d", skillNames[i], ci->skill[i], str, ci->medals[i]), sizeof(gs->strSkillz[0]));
			}
			else
			{
				Q_strncpyz(gs->strSkillz[gs->cSkills++], va("%-15s %3d %-15s", skillNames[i], ci->skill[i], str), sizeof(gs->strSkillz[0]));
			}
		}
	}
}

// Client-side stat presentation
void CG_parseWeaponStats_cmd(void (txt_dump) (char *))
{
	clientInfo_t *ci;
	qboolean     fFull     = (txt_dump != CG_printWindow);
	qboolean     fHasStats = qfalse;
	char         strName[MAX_STRING_CHARS];
	int          atts, deaths, dmg_given, dmg_rcvd, hits, kills, team_dmg, headshots;
	unsigned int i, iArg = 1;
	unsigned int nClient      = atoi(CG_Argv(iArg++));
	unsigned int nRounds      = atoi(CG_Argv(iArg++));
	unsigned int dwWeaponMask = atoi(CG_Argv(iArg++));
	unsigned int dwSkillPointMask;
	int          xp = 0; // XP can be negative

	ci = &cgs.clientinfo[nClient];

	Q_strncpyz(strName, ci->name, sizeof(strName));
	BG_cleanName(cgs.clientinfo[nClient].name, strName, sizeof(strName), qfalse);
	txt_dump(va("^7Overall stats for: ^3%s ^7(^2%d^7 Round%s)\n\n", strName, nRounds, ((nRounds != 1) ? "s" : "")));

	if (fFull)
	{
		txt_dump("Weapon     Acrcy Hits/Atts Kills Deaths Headshots\n");
		txt_dump("-------------------------------------------------\n");
	}
	else
	{
		txt_dump("Weapon     Acrcy Hits/Atts Kll Dth HS\n");
		//txt_dump(     "-------------------------------------\n");
		txt_dump("\n");
	}

	if (!dwWeaponMask)
	{
		txt_dump("^3No weapon info available.\n");
	}
	else
	{
		for (i = WS_KNIFE; i < WS_MAX; i++)
		{
			if (dwWeaponMask & (1 << i))
			{
				hits      = atoi(CG_Argv(iArg++));
				atts      = atoi(CG_Argv(iArg++));
				kills     = atoi(CG_Argv(iArg++));
				deaths    = atoi(CG_Argv(iArg++));
				headshots = atoi(CG_Argv(iArg++));

				Q_strncpyz(strName, va("^3%-9s: ", aWeaponInfo[i].pszName), sizeof(strName));
				if (atts > 0 || hits > 0)
				{
					fHasStats = qtrue;
					Q_strcat(strName, sizeof(strName), va("^7%5.1f ^5%4d/%-4d ",
					                                      ((atts == 0) ? 0.0 : (float)(hits * 100.0 / (float)atts)),
					                                      hits, atts));
				}
				else
				{
					Q_strcat(strName, sizeof(strName), va("                "));
					if (kills > 0 || deaths > 0)
					{
						fHasStats = qtrue;
					}
				}

				if (fFull)
				{
					txt_dump(va("%s^2%5d ^1%6d%s\n", strName, kills, deaths, ((aWeaponInfo[i].fHasHeadShots) ? va(" ^3%9d", headshots) : "")));
				}
				else
				{
					txt_dump(va("%s^2%3d ^1%3d%s\n", strName, kills, deaths, ((aWeaponInfo[i].fHasHeadShots) ? va(" ^3%2d", headshots) : "")));
				}
			}
		}

		if (fHasStats)
		{
			dmg_given = atoi(CG_Argv(iArg++));
			dmg_rcvd  = atoi(CG_Argv(iArg++));
			team_dmg  = atoi(CG_Argv(iArg++));

			if (!fFull)
			{
				txt_dump("\n\n\n");
			}
			else
			{
				txt_dump("\n");
			}

			txt_dump(va("^3Damage Given: ^7%-6d  ^3Team Damage: ^7%d\n", dmg_given, team_dmg));
			txt_dump(va("^3Damage Recvd: ^7%d\n", dmg_rcvd));
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
	dwSkillPointMask = atoi(CG_Argv(iArg++));
	for (i = SK_BATTLE_SENSE; i < SK_NUM_SKILLS; i++)
	{
		if (dwSkillPointMask & (1 << i))
		{
			ci->skillpoints[i] = atoi(CG_Argv(iArg++));
			xp                += ci->skillpoints[i];
		}
	}

	txt_dump(va("^2Rank: ^7%s (%d XP)\n", ((ci->team == TEAM_AXIS) ? rankNames_Axis : rankNames_Allies)[ci->rank], xp));

	if (!fFull)
	{
		txt_dump("\n\n\n");
	}

	// Medals only in campaign mode
	txt_dump(va("Skills         Level/Points%s\n", ((cgs.gametype == GT_WOLF_CAMPAIGN) ? "  Medals" : "")));
	if (fFull)
	{
		txt_dump(va("---------------------------%s\n", ((cgs.gametype == GT_WOLF_CAMPAIGN) ? "--------" : "")));
	}
	else
	{
		txt_dump("\n");
	}

	if (dwSkillPointMask == 0)
	{
		txt_dump("^3No skills acquired!\n");
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
				str = va("%d (%d/%d)", ci->skill[i], ci->skillpoints[i], skillLevels[i][ci->skill[i] + 1]);
			}
			else
			{
				str = va("%d (%d)", ci->skill[i], ci->skillpoints[i]);
			}

			if (cgs.gametype == GT_WOLF_CAMPAIGN)
			{
				txt_dump(va("%-14s ^3%-12s  ^2%6d\n", skillNames[i], str, ci->medals[i]));
			}
			else
			{
				txt_dump(va("%-14s ^3%-12s\n", skillNames[i], str));
			}
		}
	}
}

void CG_parseBestShotsStats_cmd(qboolean doTop, void (txt_dump) (char *))
{
	int      iArg  = 1;
	qboolean fFull = (txt_dump != CG_printWindow);
	int      iWeap = atoi(CG_Argv(iArg++));

	if (!iWeap)
	{
		txt_dump(va("^3No qualifying %sshot info available.\n", ((doTop) ? "top" : "bottom")));
		return;
	}

	txt_dump(va("^2%s Match Accuracies:\n", (doTop) ? "BEST" : "WORST"));
	if (fFull)
	{
		txt_dump("\n^3WP   Acrcy Hits/Atts Kills Deaths\n");
		txt_dump("-------------------------------------------------------------\n");
	}
	else
	{
		txt_dump("^3WP   Acrcy Hits/Atts Kll Dth\n");
		//  txt_dump(    "-------------------------------------------\n");
		txt_dump("\n");
	}

	while (iWeap)
	{
		int   cnum   = atoi(CG_Argv(iArg++));
		int   hits   = atoi(CG_Argv(iArg++));
		int   atts   = atoi(CG_Argv(iArg++));
		int   kills  = atoi(CG_Argv(iArg++));
		int   deaths = atoi(CG_Argv(iArg++));
		float acc    = (atts > 0) ? (float)(hits * 100) / (float)atts : 0.0;
		char  name[32];

		if (fFull)
		{
			BG_cleanName(cgs.clientinfo[cnum].name, name, 30, qfalse);
			txt_dump(va("^3%s ^7%5.1f ^5%4d/%-4d ^2%5d ^1%6d ^7%s\n",
			            aWeaponInfo[iWeap - 1].pszCode, acc, hits, atts, kills, deaths, name));
		}
		else
		{
			BG_cleanName(cgs.clientinfo[cnum].name, name, 12, qfalse);
			txt_dump(va("^3%s ^7%5.1f ^5%4d/%-4d ^2%3d ^1%3d ^7%s\n",
			            aWeaponInfo[iWeap - 1].pszCode, acc, hits, atts, kills, deaths, name));
		}

		iWeap = atoi(CG_Argv(iArg++));
	}
}

void CG_parseTopShotsStats_cmd(qboolean doTop, void (txt_dump) (char *))
{
	int i, iArg = 1;
	int cClients = atoi(CG_Argv(iArg++));
	int iWeap    = atoi(CG_Argv(iArg++));
	int wBestAcc = atoi(CG_Argv(iArg++));

	txt_dump(va("Weapon accuracies for: ^3%s\n",
	            (iWeap >= WS_KNIFE && iWeap < WS_MAX) ? aWeaponInfo[iWeap].pszName : "UNKNOWN"));

	txt_dump("\n^3  Acc Hits/Atts Kills Deaths\n");
	txt_dump("----------------------------------------------------------\n");

	if (!cClients)
	{
		txt_dump("NO QUALIFYING WEAPON INFO AVAILABLE.\n");
		return;
	}

	for (i = 0; i < cClients; i++)
	{
		int        cnum   = atoi(CG_Argv(iArg++));
		int        hits   = atoi(CG_Argv(iArg++));
		int        atts   = atoi(CG_Argv(iArg++));
		int        kills  = atoi(CG_Argv(iArg++));
		int        deaths = atoi(CG_Argv(iArg++));
		float      acc    = (atts > 0) ? (float)(hits * 100) / (float)atts : 0.0;
		const char *color = (((doTop) ? acc : ((float)wBestAcc) + 0.999) >= ((doTop) ? wBestAcc : acc)) ? "^3" : "^7";
		char       name[32];

		BG_cleanName(cgs.clientinfo[cnum].name, name, 30, qfalse);
		txt_dump(va("%s%5.1f ^5%4d/%-4d ^2%5d ^1%6d %s%s\n", color, acc, hits, atts, kills, deaths, color, name));
	}
}

void CG_scores_cmd(void)
{
	const char *str = CG_Argv(1);

	CG_Printf("[skipnotify]%s", str);
	if (cgs.dumpStatsFile > 0)
	{
		char s[MAX_STRING_CHARS];

		BG_cleanName(str, s, sizeof(s), qtrue);
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

void CG_printFile(char *str)
{
	CG_Printf("%s", str);
	if (cgs.dumpStatsFile > 0)
	{
		char s[MAX_STRING_CHARS];

		BG_cleanName(str, s, sizeof(s), qtrue);
		trap_FS_Write(s, strlen(s), cgs.dumpStatsFile);
	}
}

void CG_dumpStats(void)
{
	qtime_t    ct;
	qboolean   fDoScores = qfalse;
	const char *info     = CG_ConfigString(CS_SERVERINFO);
	char       *s        = va("^3>>> %s: ^2%s\n\n", CG_TranslateString("Map"), Info_ValueForKey(info, "mapname"));

	trap_RealTime(&ct);
	// /me holds breath (using circular va() buffer)
	if (cgs.dumpStatsFile == 0)
	{
		fDoScores             = qtrue;
		cgs.dumpStatsFileName = va("stats/%d.%02d.%02d/%02d%02d%02d.txt",
		                           1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday,
		                           ct.tm_hour, ct.tm_min, ct.tm_sec);
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

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/

void CG_ForceTapOut_f(void);

static void CG_ServerCommand(void)
{
	const char *cmd;

	cmd = CG_Argv(0);

	if (!cmd[0])
	{
		// server claimed the command
		return;
	}

	if (!strcmp(cmd, "tinfo"))
	{
		CG_ParseTeamInfo();
		return;
	}
	else if (!strcmp(cmd, "sc0"))
	{
		CG_ParseScore(TEAM_AXIS);
		return;
	}
	else if (!strcmp(cmd, "sc1"))
	{
		CG_ParseScore(TEAM_ALLIES);
		return;
	}
	else if (!strcmp(cmd, "WeaponStats"))
	{
		int i, start = 1;

		for (i = 0; i < WP_NUM_WEAPONS; i++)
		{
			if (!BG_ValidStatWeapon(i))
			{
				continue;
			}

			cgs.playerStats.weaponStats[i].kills     = atoi(CG_Argv(start++));
			cgs.playerStats.weaponStats[i].killedby  = atoi(CG_Argv(start++));
			cgs.playerStats.weaponStats[i].teamkills = atoi(CG_Argv(start++));
		}

		cgs.playerStats.suicides = atoi(CG_Argv(start++));

		for (i = 0; i < HR_NUM_HITREGIONS; i++)
		{
			cgs.playerStats.hitRegions[i] = atoi(CG_Argv(start++));
		}

		cgs.numOIDtriggers = atoi(CG_Argv(start++));

		for (i = 0; i < cgs.numOIDtriggers; i++)
		{
			cgs.playerStats.objectiveStats[i] = atoi(CG_Argv(start++));
			cgs.teamobjectiveStats[i]         = atoi(CG_Argv(start++));
		}

		return;
	}
	else if (!Q_stricmp(cmd, "cpm"))
	{
		CG_AddPMItem(PM_MESSAGE, CG_LocalizeServerCommand(CG_Argv(1)), cgs.media.voiceChatShader, NULL);
		return;
	}
	else if (!Q_stricmp(cmd, "cp"))
	{
		int  args = trap_Argc();
		char *s;

		if (args >= 3)
		{
			s = CG_TranslateString(CG_Argv(1));

			if (args == 4)
			{
				s = va("%s%s", CG_Argv(3), s);
			}

			// for client logging
			if (cg_printObjectiveInfo.integer > 0 && (args == 4 || atoi(CG_Argv(2)) > 1))
			{
				CG_Printf("[cgnotify]*** ^3INFO: ^5%s\n", CG_LocalizeServerCommand(CG_Argv(1)));
			}
			CG_PriorityCenterPrint(s, SCREEN_HEIGHT - (SCREEN_HEIGHT * 0.20), SMALLCHAR_WIDTH, atoi(CG_Argv(2)));
		}
		else
		{
			CG_CenterPrint(CG_LocalizeServerCommand(CG_Argv(1)), SCREEN_HEIGHT - (SCREEN_HEIGHT * 0.20), SMALLCHAR_WIDTH);
		}
		return;
	}
	else if (!Q_stricmp(cmd, "reqforcespawn"))
	{
		if (cg_instanttapout.integer)
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
	}
	else if (!Q_stricmp(cmd, "sdbg"))
	{
		CG_StatsDebugAddText(CG_Argv(1));
		return;
	}
	else if (!Q_stricmp(cmd, "cs"))
	{
		CG_ConfigStringModified();
		return;
	}
	else if (!Q_stricmp(cmd, "print"))
	{
		CG_Printf("[cgnotify]%s", CG_LocalizeServerCommand(CG_Argv(1)));
		return;
	}
	else if (!Q_stricmp(cmd, "entnfo"))
	{
		char buffer[16];
		int  allied_number, axis_number;

		trap_Argv(1, buffer, 16);
		axis_number = atoi(buffer);

		trap_Argv(2, buffer, 16);
		allied_number = atoi(buffer);

		CG_ParseMapEntityInfo(axis_number, allied_number);

		return;
	}
	else if (!Q_stricmp(cmd, "chat"))
	{
		char       text[MAX_SAY_TEXT];
		const char *s;
		int        clientNum = -1;

		if (cg_teamChatsOnly.integer)
		{
			return;
		}

		if (trap_Argc() >= 3)
		{
			clientNum = atoi(CG_Argv(2));
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
		CG_RemoveChatEscapeChar(text);
		CG_AddToTeamChat(text, clientNum);
		CG_Printf("%s\n", text);
		CG_WriteToLog("%s\n", text);

		return;
	}
	else if (!Q_stricmp(cmd, "tchat"))
	{
		char       text[MAX_SAY_TEXT];
		vec3_t     origin;
		char       *locStr = NULL;
		const char *s;
		int        clientNum;

		clientNum = atoi(CG_Argv(2));

		origin[0] = atoi(CG_Argv(4));
		origin[1] = atoi(CG_Argv(5));
		origin[2] = atoi(CG_Argv(6));

		if (atoi(CG_Argv(3)))
		{
			s = CG_LocalizeServerCommand(CG_Argv(1));
		}
		else
		{
			s = CG_Argv(1);
		}

		locStr = CG_BuildLocationString(clientNum, origin, LOC_TCHAT);

		if (!locStr || !*locStr)
		{
			locStr = "";
		}

		// process locations and name
		Com_sprintf(text, sizeof(text), "(%s^7)^3(%s):%s", cgs.clientinfo[clientNum].name, locStr, s);

		CG_RemoveChatEscapeChar(text);
		CG_AddToTeamChat(text, clientNum); // disguise ?
		CG_Printf("%s\n", text);
		CG_WriteToLog("%s\n", text);

		return;
	}
	else if (!Q_stricmp(cmd, "vchat"))
	{
		CG_VoiceChat(SAY_ALL);              // enabled support
		return;
	}
	else if (!Q_stricmp(cmd, "vtchat"))
	{
		CG_VoiceChat(SAY_TEAM);             // enabled support
		return;
	}
	else if (!Q_stricmp(cmd, "vbchat"))
	{
		CG_VoiceChat(SAY_BUDDY);
		return;
	}
	// Allow client to lodge a complaing
	else if (!Q_stricmp(cmd, "complaint") && cgs.gamestate == GS_PLAYING)
	{
		if (cg_complaintPopUp.integer == 0)
		{
			trap_SendClientCommand("vote no");
		}

		cgs.complaintEndTime = cg.time + 20000;
		cgs.complaintClient  = atoi(CG_Argv(1));

		if (cgs.complaintClient < 0)
		{
			cgs.complaintEndTime = cg.time + 10000;
		}

		return;
	}
	else if (!Q_stricmp(cmd, "map_restart"))
	{
		CG_MapRestart();
		return;
	}
	// match stats
	else if (!Q_stricmp(cmd, "sc"))
	{
		CG_scores_cmd();
		return;
	}
	// weapon stats parsing
	else if (!Q_stricmp(cmd, "ws"))
	{
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
	}
	else if (!Q_stricmp(cmd, "wws"))
	{
		CG_wstatsParse_cmd();
		return;
	}
	else if (!Q_stricmp(cmd, "gstats"))
	{
		CG_parseWeaponStatsGS_cmd();
		return;
	}
	// "topshots"-related commands
	else if (!Q_stricmp(cmd, "astats"))
	{
		CG_parseTopShotsStats_cmd(qtrue, CG_printConsoleString);
		return;
	}
	else if (!Q_stricmp(cmd, "astatsb"))
	{
		CG_parseTopShotsStats_cmd(qfalse, CG_printConsoleString);
		return;
	}
	else if (!Q_stricmp(cmd, "bstats"))
	{
		CG_parseBestShotsStats_cmd(qtrue, CG_printConsoleString);
		return;
	}
	else if (!Q_stricmp(cmd, "bstatsb"))
	{
		CG_parseBestShotsStats_cmd(qfalse, CG_printConsoleString);
		return;
	}
	else if (!Q_stricmp(cmd, "immaplist")) //MAPVOTE
	{
		CG_parseMapVoteListInfo();
		return;
	}
	else if (!Q_stricmp(cmd, "imvotetally")) //MAPVOTE
	{
		CG_parseMapVoteTally();
		return;
	}
	else if (!Q_stricmp(cmd, "wbstats"))
	{
		CG_topshotsParse_cmd(qtrue);
		return;
	}
	// single weapon stat (requested weapon stats)
	else if (!Q_stricmp(cmd, "rws"))
	{
		CG_ParseWeaponStats();
		return;
	}
	else if (!Q_stricmp(cmd, "portalcampos"))
	{
		CG_ParsePortalPos();
		return;
	}
	else if (!Q_stricmp(cmd, "startCam"))
	{
		CG_StartCamera(CG_Argv(1), atoi(CG_Argv(2)));
		return;
	}
	else if (!Q_stricmp(cmd, "SetInitialCamera"))
	{
		CG_SetInitialCamera(CG_Argv(1), atoi(CG_Argv(2)));
		return;
	}
	else if (!Q_stricmp(cmd, "stopCam"))
	{
		CG_StopCamera();
		return;
	}
	else if (!Q_stricmp(cmd, "setspawnpt"))
	{
		cg.selectedSpawnPoint = atoi(CG_Argv(1)) + 1;
		return;
	}
	else if (!strcmp(cmd, "rockandroll"))         // map loaded, game is ready to begin. - FIXME: obsolete?
	{   // FIXME: re-enable when we get menus that deal with fade properly
		//CG_Fade(0, 0, 0, 255, cg.time, 0);    // go black
		//trap_UI_Popup("pregame");             // start pregame menu
		//trap_Cvar_Set("cg_norender", "1");    // don't render the world until the player clicks in and the 'playerstart'
		// func has been called (g_main in G_UpdateCvars() ~ilne 949)
		// cg_norender has been removed.
		trap_S_FadeAllSound(1.0f, 1000, qfalse);      // fade sound up

		return;
	}
	else if (!Q_stricmp(cmd, "application"))
	{
		cgs.applicationEndTime = cg.time + 20000;
		cgs.applicationClient  = atoi(CG_Argv(1));

		if (cgs.applicationClient < 0)
		{
			cgs.applicationEndTime = cg.time + 10000;
		}

		return;
	}
	else if (!Q_stricmp(cmd, "invitation"))
	{
		cgs.invitationEndTime = cg.time + 20000;
		cgs.invitationClient  = atoi(CG_Argv(1));

		if (cgs.invitationClient < 0)
		{
			cgs.invitationEndTime = cg.time + 10000;
		}

		return;
	}
	else if (!Q_stricmp(cmd, "proposition"))
	{
		cgs.propositionEndTime = cg.time + 20000;
		cgs.propositionClient  = atoi(CG_Argv(1));
		cgs.propositionClient2 = atoi(CG_Argv(2));

		if (cgs.propositionClient < 0)
		{
			cgs.propositionEndTime = cg.time + 10000;
		}

		return;
	}
	else if (!Q_stricmp(cmd, "aft"))
	{
		cgs.autoFireteamEndTime = cg.time + 20000;
		cgs.autoFireteamNum     = atoi(CG_Argv(1));

		if (cgs.autoFireteamNum < -1)
		{
			cgs.autoFireteamEndTime = cg.time + 10000;
		}
		return;
	}
	else if (!Q_stricmp(cmd, "aftc"))
	{
		cgs.autoFireteamCreateEndTime = cg.time + 20000;
		cgs.autoFireteamCreateNum     = atoi(CG_Argv(1));

		if (cgs.autoFireteamCreateNum < -1)
		{
			cgs.autoFireteamCreateEndTime = cg.time + 10000;
		}
		return;
	}
	else if (!Q_stricmp(cmd, "aftj"))
	{
		cgs.autoFireteamJoinEndTime = cg.time + 20000;
		cgs.autoFireteamJoinNum     = atoi(CG_Argv(1));

		if (cgs.autoFireteamJoinNum < -1)
		{
			cgs.autoFireteamJoinEndTime = cg.time + 10000;
		}
		return;
	}
	else if (Q_stricmp(cmd, "remapShader") == 0)
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
	// GS Copied in code from old source for mu_start, mu_play & mu_stop

	// music

	// loops \/
	else if (!Q_stricmp(cmd, "mu_start"))         // has optional parameter for fade-up time
	{
		char text[MAX_SAY_TEXT];
		int  fadeTime = 0;   // default to instant start

		Q_strncpyz(text, CG_Argv(2), MAX_SAY_TEXT);
		if (*text)
		{
			fadeTime = atoi(text);
		}

		trap_S_StartBackgroundTrack(CG_Argv(1), CG_Argv(1), fadeTime);
		return;
	}
	// plays once then back to whatever the loop was \/
	else if (!Q_stricmp(cmd, "mu_play"))          // has optional parameter for fade-up time
	{
		char text[MAX_SAY_TEXT];
		int  fadeTime = 0;   // default to instant start

		Q_strncpyz(text, CG_Argv(2), MAX_SAY_TEXT);
		if (*text)
		{
			fadeTime = atoi(text);
		}

		trap_S_StartBackgroundTrack(CG_Argv(1), "onetimeonly", fadeTime);
		return;
	}
	else if (!Q_stricmp(cmd, "mu_stop"))          // has optional parameter for fade-down time
	{
		char text[MAX_SAY_TEXT];
		int  fadeTime = 0;   // default to instant stop

		Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
		if (*text)
		{
			fadeTime = atoi(text);
		}

		trap_S_FadeBackgroundTrack(0.0f, fadeTime, 0);
		trap_S_StartBackgroundTrack("", "", -2);   // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
		return;
	}
	else if (!Q_stricmp(cmd, "mu_fade"))
	{
		trap_S_FadeBackgroundTrack(atof(CG_Argv(1)), atoi(CG_Argv(2)), 0);
		return;
	}
	else if (!Q_stricmp(cmd, "snd_fade"))
	{
		trap_S_FadeAllSound(atof(CG_Argv(1)), atoi(CG_Argv(2)), atoi(CG_Argv(3)));
		return;
	}
	// ensure a file gets into a build (mainly for scripted music calls)
	else if (!Q_stricmp(cmd, "addToBuild"))
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
	else if (!Q_stricmp(cmd, "spawnserver"))
	{
		// print message informing player the server is restarting with a new map
		CG_PriorityCenterPrint(va("%s", CG_TranslateString("^3Server Restarting")), SCREEN_HEIGHT - (SCREEN_HEIGHT * 0.25), SMALLCHAR_WIDTH, 999999);

		// hack here
		cg.serverRespawning = qtrue;

		// fade out over the course of 5 seconds, should be enough (nuking: atvi bug 3793)
		//CG_Fade( 0, 0, 0, 255, cg.time, 5000 );

		return;
	}
	else if (CG_Debriefing_ServerCommand(cmd))
	{
		return;
	}

	CG_Printf("Unknown client game command: %s\n", cmd);
}

/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
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
