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
 * @file cg_scoreboard.c
 * @brief Draw the scoreboard on top of the game screen
 */

#include "cg_local.h"

char *Binding_FromName(const char *cvar);

// colors and fonts for overlays
vec4_t SB_bg     = { 0.16f, 0.2f, 0.17f, 0.8f };
vec4_t SB_bg2    = { 0.0f, 0.0f, 0.0f, 0.6f };
vec4_t SB_border = { 1.0f, 1.0f, 1.0f, 0.3f };
vec4_t SB_text   = { 0.6f, 0.6f, 0.6f, 1.0f };

#define FONT_HEADER         &cgs.media.limboFont1
#define FONT_TEXT           &cgs.media.limboFont2

/*
=================
WM_DrawObjectives
=================
*/

#define INFO_PLAYER_WIDTH       134
#define INFO_SCORE_WIDTH        76
#define INFO_XP_WIDTH           56
#define INFO_CLASS_WIDTH        34
#define INFO_LATENCY_WIDTH      36
#define INFO_LIVES_WIDTH        20
#define INFO_LINE_HEIGHT        30
#define INFO_TOTAL_WIDTH        (INFO_PLAYER_WIDTH + INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH)

#define INFOTEXT_STARTX         8
#define INFOTEXT_STARTY         146

/**
 * @brief Draw a client country flag
 *
 * @details All flags are stored in one single image where they are aligned
 * into a grid of 16x16 fields. Each flag has an id number starting
 * with 0 at the top left corner and ending with 255 in the bottom right
 * corner. Client's flag id is stored in the "uci" field of configstrings.
 *
 * @ingroup GeoIP
 * @param[in] x
 * @param[in] y
 * @param[in] fade
 * @param[in] clientNum
 * @return
 */
qboolean CG_DrawFlag(float x, float y, float fade, int clientNum)
{
	int client_flag = Q_atoi(Info_ValueForKey(CG_ConfigString(clientNum + CS_PLAYERS), "u"));  // uci

	if (client_flag < MAX_COUNTRY_NUM)
	{
		const int flag_size = 32;  // dimensions of a single flag
		const int all_flags = 512; // dimensions of the picture containing all flags

		float alpha[4] = { 1.f, 1.f, 1.f, fade };
		float x1       = (float)((client_flag * flag_size) % all_flags);
		float y1       = (float)(floor((client_flag * flag_size) / all_flags) * flag_size);
		float x2       = x1 + flag_size;
		float y2       = y1 + flag_size;

		trap_R_SetColor(alpha);

		CG_DrawPicST(x, y, 14, 14, x1 / all_flags, y1 / all_flags, x2 / all_flags, y2 / all_flags, cgs.media.countryFlags);

		trap_R_SetColor(NULL);
		return qtrue;
	}
	return qfalse;
}

/**
 * @brief WM_DrawObjectives
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] fade - unused
 * @return
 */
int WM_DrawObjectives(int x, int y, int width, float fade)
{
	const char *s;
	const char *t;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		static qhandle_t alliesFlag = 0;
		static qhandle_t textAllies = 0;
		static qhandle_t axisFlag   = 0;
		static qhandle_t textAxis   = 0;
		static qhandle_t textWin    = 0;

		const char *buf;
		qhandle_t  *flagshader = NULL, *nameshader = NULL;
		int        baseRows    = 8; // base number of rows for Y coordinate calculations
		int        currentRows = 8; // current number of rows for Y coordinate calculations

		// Without scaling, flags are drawn at 210x136 size
		// and text at 127x64.
		float flagSizeX = 210.0f;
		float flagSizeY = 136.0f;
		float textSizeX = 127.0f;
		float textSizeY = 64.0f;
		float scale     = 1.0f;
		float flagX1;
		float flagX2;

		if (cg.teamPlayers[TEAM_ALLIES] > 8 || cg.teamPlayers[TEAM_AXIS] > 8)
		{
			// teams might not be balanced, so pick the team with more players
			currentRows = cg.teamPlayers[TEAM_ALLIES] > cg.teamPlayers[TEAM_AXIS] ? cg.teamPlayers[TEAM_ALLIES] : cg.teamPlayers[TEAM_AXIS];

			// only consider up to 16 players per team
			if (currentRows > 16)
			{
				currentRows = 16;
			}
		}

		y += 16 * (baseRows - 1);

		s   = CG_ConfigString(CS_MULTI_MAPWINNER);
		buf = Info_ValueForKey(s, "w");

#ifdef FEATURE_RATING
		// update intermission scoreboard
		if (cgs.skillRating > 1)
		{
			const char *info = CG_ConfigString(CS_MODINFO);

			cgs.mapProb = (float)atof(Info_ValueForKey(info, "M"));
		}
#endif

		if (Q_atoi(buf) == -1)
		{
			// "ITS A TIE!";
		}
		else if (Q_atoi(buf))
		{
			// "ALLIES";

			if (!alliesFlag)
			{
				alliesFlag = trap_R_RegisterShaderNoMip("ui/assets/portraits/allies_win_flag.tga");
			}

			if (!textAllies)
			{
				textAllies = trap_R_RegisterShaderNoMip("ui/assets/portraits/text_allies.tga");
			}

			flagshader = &alliesFlag;
			nameshader = &textAllies;
		}
		else
		{
			// "AXIS";

			if (!axisFlag)
			{
				axisFlag = trap_R_RegisterShaderNoMip("ui/assets/portraits/axis_win_flag.tga");
			}

			if (!textAxis)
			{
				textAxis = trap_R_RegisterShaderNoMip("ui/assets/portraits/text_axis.tga");
			}

			flagshader = &axisFlag;
			nameshader = &textAxis;
		}

		y += 16 * ((baseRows - 2) / 2);

		// If a team has more than 12 players, start scaling down flag sizes.
		// Row height at this point is 12.
		if (currentRows > 12)
		{
			int i;

			for (i = 13; i <= currentRows; i++)
			{
				scale      = (flagSizeY - 12) / flagSizeY;
				flagSizeX *= scale;
				flagSizeY *= scale;
				textSizeX *= scale;
				textSizeY *= scale;

				y -= 12;
			}
		}

		flagX1 = SCREEN_WIDTH / 2 + cgs.wideXoffset - 5 - (flagSizeX + cgs.wideXoffset);      // left flag
		flagX2 = SCREEN_WIDTH / 2 + 5;

		if (flagshader)
		{                                              // right flag
			CG_DrawPic(flagX1 + cgs.wideXoffset, 10, flagSizeX, flagSizeY, *flagshader);
			CG_DrawPic(flagX2 + cgs.wideXoffset, 10, flagSizeX, flagSizeY, *flagshader);
		}

		if (nameshader)
		{
			float textX1 = flagX1 + (flagSizeX * 0.5f) - (textSizeX * 0.5f);                            // left flag text
			float textX2 = flagX2 + (flagSizeX * 0.5f) - (textSizeX * 0.5f);                            // right flag text
			float textY  = 10 + (flagSizeY * 0.5f) - (textSizeY * 0.5f);

			if (!textWin)
			{
				textWin = trap_R_RegisterShaderNoMip("ui/assets/portraits/text_win.tga");
			}

			CG_DrawPic(textX1 + cgs.wideXoffset, textY, textSizeX, textSizeY, *nameshader);
			CG_DrawPic(textX2 + cgs.wideXoffset, textY, textSizeX, textSizeY, textWin);
		}
		return y;
	}
	// mission time & reinforce time
	else
	{
		int msec, mins, seconds, tens, w;

		CG_FillRect(x - 5, y - 2, width + 5, 34, SB_bg);
		CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 34, 1, SB_border);

		y += 13;

		if (cgs.timelimit > 0.0f)
		{
			msec = (int)(cgs.timelimit * 60000.f) - (cg.time - cgs.levelStartTime); // 60.f * 1000.f

			seconds  = msec / 1000;
			mins     = seconds / 60;
			seconds -= mins * 60;
			tens     = seconds / 10;
			seconds -= tens * 10;
		}
		else
		{
			msec = mins = tens = seconds = 0;
		}

		if (cgs.gamestate != GS_PLAYING)
		{
			s = va("%s ^7%s", CG_TranslateString("MISSION TIME:"), CG_TranslateString("WARMUP"));
		}
		else if (msec < 0 && cgs.timelimit > 0.0f)
		{
			if (cgs.gamestate == GS_WAITING_FOR_PLAYERS)
			{
				s = va("%s ^7%s", CG_TranslateString("MISSION TIME:"), CG_TranslateString("GAME STOPPED"));
			}
			else
			{
				s = va("%s ^7%s", CG_TranslateString("MISSION TIME:"), CG_TranslateString("SUDDEN DEATH"));
			}
		}
		else
		{
			s = va("%s   ^7%2.f:%i%i", CG_TranslateString("MISSION TIME:"), (float)mins, tens, seconds);
			w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

			// time limit
			if (cgs.timelimit > 0.0f)
			{
				msec = (int)(cgs.timelimit * 60000.f);

				seconds  = msec / 1000;
				mins     = seconds / 60;
				seconds -= mins * 60;
				tens     = seconds / 10;
				seconds -= tens * 10;
			}
			else
			{
				msec = mins = tens = seconds = 0;
			}

			t = va(" / %2.f:%i%i", (float)mins, tens, seconds);
			CG_Text_Paint_Ext(x + w, y, 0.19f, 0.19f, SB_text, t, 0, 0, 0, FONT_HEADER);
		}

		CG_Text_Paint_Ext(x, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);

		if (cg.warmup)
		{
			int sec = (cg.warmup - cg.time) / 1000;

			if (sec > 0)
			{
				s = va("%s %s%i", CG_TranslateString("MATCH BEGINS IN:"), ((cg.warmup - cg.time) / 1000) < 4 ? "^1" : "^2", sec);
			}
			else
			{
				s = va("%s", CG_TranslateString("MATCH BEGINS NOW!"));
			}
			CG_Text_Paint_Ext(SCREEN_WIDTH - 20 - CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER) + cgs.wideXoffset, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
		}
		else if ((cgs.gamestate == GS_WARMUP && !cg.warmup) || cgs.gamestate == GS_WAITING_FOR_PLAYERS)
		{
			s = va(CG_TranslateString("WAITING ON ^2%i ^9%s"), cgs.minclients, cgs.minclients == 1 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));
			CG_Text_Paint_Ext(SCREEN_WIDTH - 20 - CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER) + cgs.wideXoffset, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
		}
		else if (cgs.gametype != GT_WOLF_LMS && !cg.warmup)
		{
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS || cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES)
			{
				msec = CG_CalculateReinfTime(qfalse) * 1000;
			}
			else  // no team (spectator mode)
			{
				msec = 0;
			}

			if (msec)
			{
				seconds  = msec / 1000;
				mins     = seconds / 60;
				seconds -= mins * 60;
				tens     = seconds / 10;
				seconds -= tens * 10;

				if (tens > 0)
				{
					s = va("%s ^F%i%i", CG_TranslateString("REINFORCE TIME:"), tens, seconds);
				}
				else
				{
					s = va("%s   %s%i", CG_TranslateString("REINFORCE TIME:"), (seconds <= 2 &&
					                                                            cgs.clientinfo[cg.clientNum].health == 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW)) ? "^1" : "^F", seconds);
				}
				CG_Text_Paint_Ext(SCREEN_WIDTH - 20 - CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER) + cgs.wideXoffset, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
			}
		}

		switch (cgs.gametype)
		{
		case GT_WOLF_STOPWATCH:
			s = va("%s %i", CG_TranslateString("STOPWATCH ROUND"), cgs.currentRound + 1);
			break;
		case GT_WOLF_LMS:
			s = va("%s %i  %s %i-%i", CG_TranslateString("ROUND"), cgs.currentRound + 1, CG_TranslateString("SCORE"), cg.teamWonRounds[1], cg.teamWonRounds[0]);
			break;
		case GT_WOLF_CAMPAIGN:
			s = va(CG_TranslateString("MAP %i of %i"), cgs.currentCampaignMap + 1, cgs.campaignData.mapCount);
			break;
		case GT_WOLF_MAPVOTE:
			s = (cgs.mapVoteMapY ? va(CG_TranslateString("MAP %i of %i"), cgs.mapVoteMapX + 1, cgs.mapVoteMapY) : "MAP");
			break;
		default:
			s = "MAP";
			break;
		}
		w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

		CG_Text_Paint_Ext(x + 300 - w * 0.5f, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);

		y += 12;

		if (CG_ConfigString(CS_CONFIGNAME)[0])
		{
			s = va(CG_TranslateString("Config: ^7%s^7"), CG_ConfigString(CS_CONFIGNAME));
			CG_Text_Paint_Ext(x, y, 0.20f, 0.20f, SB_text, s, 0, 0, 0, FONT_TEXT);
		}

		// map name
		if (cgs.gametype == GT_WOLF_CAMPAIGN)
		{
			s = cgs.campaignInfoLoaded ? cgs.campaignData.arenas[cgs.currentCampaignMap].longname : cgs.campaignData.mapnames[cgs.currentCampaignMap];
		}
		else
		{
			s = cgs.arenaInfoLoaded ? cgs.arenaData.longname : cgs.rawmapname;
		}

		w = CG_Text_Width_Ext(s, 0.20f, 0, FONT_TEXT);
		CG_Text_Paint_Ext(x + 300 - w * 0.5f, y, 0.20f, 0.20f, SB_text, s, 0, 0, 0, FONT_TEXT);

		y += 12;
	}
	return y;
}

/**
 * @brief SkillNumForClass
 * @param[in] classNum
 * @return
 */
int SkillNumForClass(int classNum)
{
	switch (classNum)
	{
	case PC_SOLDIER:
		return SK_HEAVY_WEAPONS;
	case PC_MEDIC:
		return SK_FIRST_AID;
	case PC_ENGINEER:
		return SK_EXPLOSIVES_AND_CONSTRUCTION;
	case PC_FIELDOPS:
		return SK_SIGNALS;
	case PC_COVERTOPS:
		return SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS;
	default:
		return SK_BATTLE_SENSE;
	}
}

/**
 * @brief Real printable charater count
 * @param[in] str
 * @return
 */
int CG_drawStrlen(const char *str)
{
	int cnt = 0;

	while (*str)
	{
		if (Q_IsColorString(str))
		{
			str += 2;
		}
		else
		{
			cnt++;
			str++;
		}
	}
	return(cnt);
}

static void WM_DrawClientScore_Highlight(int x, int y, int rowHeight, float fade, const score_t *score)
{
	vec4_t hcolor;

	if (score->client == cg.snap->ps.clientNum)
	{
		hcolor[3] = fade * 0.3f;
		VectorSet(hcolor, .5f, .5f, .2f); // yellow

		CG_FillRect(x - 5, y, (INFO_TOTAL_WIDTH + 5), rowHeight - 1, hcolor);
	}
}

static qboolean WM_DrawClientScore_Flag(int x, int y, float fade, const clientInfo_t *ci, const score_t *score, int *maxchars)
{
	if (score->ping == -1 || score->ping == 999 || !cg_countryflags.integer)
	{
		return qfalse;
	}

	return CG_DrawFlag(x - 3, y - 11, fade, ci->clientNum);
}

static int WM_DrawClientScore_PlayerIcons(int x, int y, const clientInfo_t *ci, const score_t *score)
{
	int drawnIcons = 0;

	// ready icon
	if ((score->scoreflags & 1) && cgs.gamestate != GS_PLAYING)
	{
		CG_DrawPic(x - 1, y - 9, 10, 10, cgs.media.readyShader);
		x += 12;
		drawnIcons++;
	}

	// objective icon
	if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)) && cgs.gamestate != GS_INTERMISSION)
	{
		CG_DrawPic(x - 1, y - 9, 10, 10, cgs.media.objectiveShader);
		x += 12;
		drawnIcons++;
	}
	// disguise
	else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team
	         && (ci->powerups & (1 << PW_OPS_DISGUISED)) && cgs.gamestate != GS_INTERMISSION)
	{
		CG_DrawPic(x - 1, y - 9, 10, 10, ci->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
		x += 12;
		drawnIcons++;
	}
	// invulnerability
	else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team
	         && cgs.clientinfo[score->client].health > 0 && (ci->powerups & (1 << PW_INVULNERABLE)) && cgs.gamestate != GS_INTERMISSION)
	{
		CG_DrawPic(x - 1, y - 9, 10, 10, cgs.media.spawnInvincibleShader);
		x += 12;
		drawnIcons++;
	}

	// skull icon
	if (score->respawnsLeft == -2 || (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == -1))
	{
		CG_DrawPic(x - 1, y - 9, 10, 10, cgs.media.scoreEliminatedShader);
		x += 12;
		drawnIcons++;
	}
	else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team
	         && cgs.clientinfo[score->client].health == 0 && cgs.gamestate != GS_INTERMISSION)
	{
		CG_DrawPic(x - 1, y - 9, 10, 10, cgs.media.medicIcon);
		x += 12;
		drawnIcons++;
	}

	return drawnIcons;
}

static void WM_DrawClientScore_Medals(int x, int y, float scaleX, float scaleY, const clientInfo_t *ci, int maxchars)
{
	int  i = 0, j = 0;
	char buf[64];

	buf[0] = '\0';
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		for (j = 0; j < ci->medals[i]; j++)
		{
			Q_strcat(buf, sizeof(buf), va("^%c%c", COLOR_RED + i, GetSkillTableData(i)->skillNames[0]));
		}
		maxchars--;
	}

	// CG_Text_Paint_Ext will draw everything if maxchars <= 0
	if (maxchars > 0)
	{
		CG_Text_Paint_Ext(x, y, scaleX, scaleY, colorWhite, buf, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
}

static void WM_DrawClientScore_Score(int x, int y, float scaleX, float scaleY, const score_t *score)
{
#ifdef FEATURE_RATING
	if (cgs.skillRating && cg_scoreboard.integer == SCOREBOARD_SR)
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, va("^7%5.2f", (double) score->rating), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
#endif
#ifdef FEATURE_PRESTIGE
	if (cgs.prestige && cg_scoreboard.integer == SCOREBOARD_PR)
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, va("^7%6i", score->prestige), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
#endif
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, va("^7%6i", score->score), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
}

static void WM_DrawClientScore_Ping(int x, int y, float scaleX, float scaleY, const score_t *score)
{
	if (score->ping == -1)
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorRed, "^1CONN^7", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else if (score->scoreflags & 2)
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, " BOT", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, va("%4i", score->ping), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
}

static void WM_DrawClientScore_LivesLeft(int x, int y, float scaleX, float scaleY, const score_t *score, qboolean livesleft)
{
	if (cg_gameType.integer == GT_WOLF_LMS || !livesleft)
	{
		return;
	}

	if (score->respawnsLeft >= 0)
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, va("%2i", score->respawnsLeft), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
	{
		CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, " -", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
}

static void WM_DrawClientScore_Spectator(int x, int y, float scaleX, float scaleY, const clientInfo_t *ci,
                                         const score_t *score, float fade, int maxchars, qboolean livesleft)
{
	const char *s, *p;
	int        playerWidth = 0;

	if (WM_DrawClientScore_Flag(x, y, fade, ci, score, &maxchars))
	{
		playerWidth += 15;
		x           += 15;
		maxchars    -= 2;
	}

	// draw name
	CG_Text_Paint_Ext(x, y, scaleX, scaleY, colorWhite, ci->name, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	maxchars -= CG_Text_Width_Ext(ci->name, scaleX, 0, FONT_TEXT);

	WM_DrawClientScore_Medals(x + (CG_drawStrlen(ci->name) * 8 + 8), y, scaleX, scaleY, ci, maxchars);

	x -= playerWidth;
	x += INFO_PLAYER_WIDTH;

	// add the extra room here
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft)
	{
		x += INFO_LIVES_WIDTH;
	}

	// subtract distance from border
	// everything is aligned right from here onwards
	x -= 5;
	x += INFO_CLASS_WIDTH;
	x += cg_gameType.integer == GT_WOLF_LMS ? INFO_SCORE_WIDTH : INFO_XP_WIDTH;

	// show connecting people as CONNECTING
	if (score->ping == -1)
	{
		s = CG_TranslateString("CONNECTING");
		p = "";
	}
	else
	{
		s = CG_TranslateString(va("%s", ci->shoutcaster ? "SHOUTCASTER" : "SPECTATOR"));
		p = va("%4i", score->ping);
	}

	CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorYellow, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);

	x += INFO_LATENCY_WIDTH;

	CG_Text_Paint_RightAligned_Ext(x, y, scaleX, scaleY, colorWhite, p, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
}

static void WM_DrawClientScore_Player(int x, int y, float scaleX, float scaleY, const clientInfo_t *ci,
                                      const score_t *score, float fade, int rowHeight, int maxchars, qboolean livesleft)
{
	int drawnIcons;
	int playerWidth = 0;

	if (WM_DrawClientScore_Flag(x, y, fade, ci, score, &maxchars))
	{
		playerWidth += 15;
		x           += 15;
		maxchars    -= 2;
	}

	drawnIcons   = WM_DrawClientScore_PlayerIcons(x, y, ci, score);
	playerWidth += drawnIcons * 12;
	x           += drawnIcons * 12;
	maxchars    -= drawnIcons * 2;

	// draw name
	CG_Text_Paint_Ext(x, y, scaleX, scaleY, colorWhite, ci->name, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	maxchars -= CG_Text_Width_Ext(ci->name, scaleX, 0, FONT_TEXT);

	WM_DrawClientScore_Medals(x + (CG_drawStrlen(ci->name) * 8 + 8), y, scaleX, scaleY, ci, maxchars);

	x -= playerWidth;
	x += INFO_PLAYER_WIDTH;

	// add the extra room here
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft)
	{
		x += INFO_LIVES_WIDTH;
	}

	// allow MV clients see the class of its merged client's on the scoreboard
	if (cg.snap->ps.persistant[PERS_TEAM] == ci->team || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_INTERMISSION
		#ifdef FEATURE_MULTIVIEW
	    || CG_mvMergedClientLocate(score->client)
#endif
	    )
	{
		CG_DrawPic(x - 3, y - (rowHeight * 0.75), rowHeight - 2, rowHeight - 2, cgs.media.skillPics[SkillNumForClass(ci->cls)]);

		if (cgs.clientinfo[ci->clientNum].rank > 0)
		{
			CG_DrawPic(x + 13, y - (rowHeight * 0.75), rowHeight - 2, rowHeight - 2, rankicons[cgs.clientinfo[ci->clientNum].rank][cgs.clientinfo[ci->clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
		}
	}

	x += INFO_CLASS_WIDTH;

	// subtract distance from border
	// everything is aligned right from here onwards
	x -= 5;

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		x += INFO_SCORE_WIDTH;
	}
	else
	{
		x += INFO_XP_WIDTH;
	}

	WM_DrawClientScore_Score(x, y, scaleX, scaleY, score);

	x += INFO_LATENCY_WIDTH;

	WM_DrawClientScore_Ping(x, y, scaleX, scaleY, score);

	if (cg_gameType.integer == GT_WOLF_LMS || livesleft)
	{
		x += INFO_LIVES_WIDTH;
	}

	WM_DrawClientScore_LivesLeft(x, y, scaleX, scaleY, score, livesleft);
}

/**
 * @brief WM_DrawClientScore
 * @param[in] x
 * @param[in] y
 * @param[in] score
 * @param[in] fade
 * @param[in] livesleft
 */
static void WM_DrawClientScore(int x, int y, score_t *score, float fade, qboolean livesleft)
{
	int          maxchars = 16;
	int          rowHeight = 16;
	float        scaleX = 0.24f, scaleY = 0.28f;
	int          offsetY = 12;
	clientInfo_t *ci     = &cgs.clientinfo[score->client];

	WM_DrawClientScore_Highlight(x, y, rowHeight, fade, score);

	y += offsetY;

	// add some extra space when not showing lives in non-LMS
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft)
	{
		maxchars += 2;
	}

	if (ci->team == TEAM_SPECTATOR)
	{
		WM_DrawClientScore_Spectator(x, y, scaleX, scaleY, ci, score, fade, maxchars, livesleft);

		return;
	}

	WM_DrawClientScore_Player(x, y, scaleX, scaleY, ci, score, fade, rowHeight, maxchars, livesleft);
}

/**
 * @brief WM_TimeToString
 * @param[in] msec
 * @return
 */
const char *WM_TimeToString(float msec)
{
	int mins, seconds, tens;

	seconds  = (int)(msec / 1000);
	mins     = seconds / 60;
	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;

	return va("%i:%i%i", mins, tens, seconds);
}

/**
 * @brief WM_DrawClientScore_Small
 * @param[in] x
 * @param[in] y
 * @param[in] score
 * @param[in] fade
 * @param[in] livesleft
 */
static void WM_DrawClientScore_Small(int x, int y, score_t *score, float fade, qboolean livesleft)
{
	int          maxchars = 23;
	int          rowHeight = 12;
	float        scaleX = 0.20f, scaleY = 0.25f;
	int          offsetY = 10;
	clientInfo_t *ci     = &cgs.clientinfo[score->client];

	WM_DrawClientScore_Highlight(x, y, rowHeight, fade, score);

	y += offsetY;

	if (ci->team == TEAM_SPECTATOR)
	{
		WM_DrawClientScore_Spectator(x, y, scaleX, scaleY, ci, score, fade, maxchars, livesleft);

		return;
	}

	WM_DrawClientScore_Player(x, y, scaleX, scaleY, ci, score, fade, rowHeight, maxchars, livesleft);
}

/**
 * @brief WM_DrawInfoLine
 * @param x - unused
 * @param[in] y
 * @param fade - unused
 * @return
 */
static int WM_DrawInfoLine(int x, int y, float fade)
{
	int        w, defender, winner;
	const char *s;

	if (cg.snap->ps.pm_type != PM_INTERMISSION)
	{
		return y;
	}

	w = 360;

	s        = CG_ConfigString(CS_MULTI_INFO);
	defender = Q_atoi(Info_ValueForKey(s, "d")); // defender

	s      = CG_ConfigString(CS_MULTI_MAPWINNER);
	winner = Q_atoi(Info_ValueForKey(s, "w"));

	if (cgs.currentRound)
	{
		// first round
		s = va(CG_TranslateString("CLOCK IS NOW SET TO %s!"), WM_TimeToString(cgs.nextTimeLimit * 60000.f)); // 60.f * 1000.f
	}
	else
	{
		// second round
		if (!defender)
		{
			if (winner != defender)
			{
				s = "ALLIES SUCCESSFULLY BEAT THE CLOCK!";
			}
			else
			{
				s = "ALLIES COULDN'T BEAT THE CLOCK!";
			}
		}
		else
		{
			if (winner != defender)
			{
				s = "AXIS SUCCESSFULLY BEAT THE CLOCK!";
			}
			else
			{
				s = "AXIS COULDN'T BEAT THE CLOCK!";
			}
		}

		s = CG_TranslateString(s);
	}

	CG_FillRect(Ccg_WideX(320) - w / 2, y, w, 20, SB_bg2);
	CG_DrawRect_FixedBorder(Ccg_WideX(320) - w / 2, y, w, 20, 1, SB_border);

	w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

	CG_Text_Paint_Ext(Ccg_WideX(320) - w * 0.5f, y + 15, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);

	return y + INFO_LINE_HEIGHT + 6;
}

/**
 * @brief WM_TeamScoreboard
 * @param[in] x
 * @param[in] y
 * @param[in] team
 * @param[in] fade
 * @param[in] maxrows
 * @return
 */
static int WM_TeamScoreboard(int x, int y, team_t team, float fade, int maxrows, qboolean use_mini_chars)
{
	vec4_t     hcolor;
	float      tempx, tempy;
	int        i;
	int        count = 0;
	int        width = INFO_TOTAL_WIDTH;
	int        row_height;
	qboolean   livesleft = qfalse;
	const char *buffer   = CG_ConfigString(CS_SERVERINFO);
	const char *str      = Info_ValueForKey(buffer, "g_maxlives");

	if (str && *str && Q_atoi(str))
	{
		livesleft = qtrue;
	}

	if (!livesleft)
	{
		str = Info_ValueForKey(buffer, "g_alliedmaxlives");
		if (str && *str && Q_atoi(str))
		{
			livesleft = qtrue;
		}
	}

	if (!livesleft)
	{
		str = Info_ValueForKey(buffer, "g_axismaxlives");
		if (str && *str && Q_atoi(str))
		{
			livesleft = qtrue;
		}
	}

	CG_FillRect(x - 5, y - 2, width + 5, 21, SB_bg);
	CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 21, 1, SB_border);

	Vector4Set(hcolor, 0, 0, 0, fade);

	// draw header
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		char *s;

		if (team == TEAM_AXIS)
		{
			s = va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));
			s = va("%s ^3%s", s, cg.teamFirstBlood == TEAM_AXIS ? CG_TranslateString("FIRST BLOOD") : "");

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
		}
		else if (team == TEAM_ALLIES)
		{
			s = va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));
			s = va("%s ^3%s", s, cg.teamFirstBlood == TEAM_ALLIES ? CG_TranslateString("FIRST BLOOD") : "");

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
		}
	}
	else
	{
		char *s, *s2;

		if (team == TEAM_AXIS)
		{
#ifdef FEATURE_RATING
			if (cgs.skillRating && cg_scoreboard.integer == SCOREBOARD_SR)
			{
				s = va("%s [%.1f%%] (%d %s)", CG_TranslateString("AXIS"), (double)cg.axisProb, cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));

				if (cgs.mapProb != 0.f)
				{
					float mapBias = 100.f * (cgs.mapProb - 0.5f);
					if (mapBias > 0.f)
					{
						s2 = va("%s: ^2+%.1f%%^9", CG_TranslateString("MAP BIAS"), (double)mapBias);
					}
					else if (mapBias < 0.f)
					{
						s2 = va("%s: ^1%.1f%%^9", CG_TranslateString("MAP BIAS"), (double)mapBias);
					}
					else
					{
						s2 = va("%s: ^3%.1f%%^9", CG_TranslateString("MAP BIAS"), (double)mapBias);
					}
					CG_Text_Paint_Ext(x + width - 5 - CG_Text_Width_Ext(s2, 0.19f, 0, FONT_HEADER), y + 13, 0.19f, 0.19f, SB_text, s2, 0, 0, 0, FONT_HEADER);
				}
			}
			else
#endif
#ifdef FEATURE_PRESTIGE
			if (cgs.prestige && cg_scoreboard.integer == SCOREBOARD_PR)
			{
				s = va("%s (%d %s)", CG_TranslateString("AXIS"), cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));

				s2 = va("%s", CG_TranslateString("PRESTIGE"));
				CG_Text_Paint_Ext(x + width - 5 - CG_Text_Width_Ext(s2, 0.19f, 0, FONT_HEADER), y + 13, 0.19f, 0.19f, SB_text, s2, 0, 0, 0, FONT_HEADER);
			}
			else
#endif
			{
				s = va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));

				s2 = va("%s: %.0f±%.0fms", CG_TranslateString("AVG PING"), (double)cg.teamPingMean[team], (double)cg.teamPingSd[team]);
				CG_Text_Paint_Ext(x + width - 5 - CG_Text_Width_Ext(s2, 0.19f, 0, FONT_HEADER), y + 13, 0.19f, 0.19f, SB_text, s2, 0, 0, 0, FONT_HEADER);
			}

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
		}
		else if (team == TEAM_ALLIES)
		{
#ifdef FEATURE_RATING
			if (cgs.skillRating && cg_scoreboard.integer == SCOREBOARD_SR)
			{
				s = va("%s [%.1f%%] (%d %s)", CG_TranslateString("ALLIES"), (double)cg.alliesProb, cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));

				if (cgs.mapProb != 0.f)
				{
					float mapBias = 100.f * (0.5f - cgs.mapProb);
					if (mapBias > 0.f)
					{
						s2 = va("%s: ^2+%.1f%%^9", CG_TranslateString("MAP BIAS"), (double)mapBias);
					}
					else if (mapBias < 0.f)
					{
						s2 = va("%s: ^1%.1f%%^9", CG_TranslateString("MAP BIAS"), (double)mapBias);
					}
					else
					{
						s2 = va("%s: ^3%.1f%%^9", CG_TranslateString("MAP BIAS"), (double)mapBias);
					}
					CG_Text_Paint_Ext(x + width - 5 - CG_Text_Width_Ext(s2, 0.19f, 0, FONT_HEADER), y + 13, 0.19f, 0.19f, SB_text, s2, 0, 0, 0, FONT_HEADER);
				}
			}
			else
#endif
#ifdef FEATURE_PRESTIGE
			if (cgs.prestige && cg_scoreboard.integer == SCOREBOARD_PR)
			{
				s = va("%s (%d %s)", CG_TranslateString("ALLIES"), cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));

				s2 = va("%s", CG_TranslateString("PRESTIGE"));
				CG_Text_Paint_Ext(x + width - 5 - CG_Text_Width_Ext(s2, 0.19f, 0, FONT_HEADER), y + 13, 0.19f, 0.19f, SB_text, s2, 0, 0, 0, FONT_HEADER);
			}
			else
#endif
			{
				s = va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS"));

				s2 = va("%s: %.0f±%.0fms", CG_TranslateString("AVG PING"), (double)cg.teamPingMean[team], (double)cg.teamPingSd[team]);
				CG_Text_Paint_Ext(x + width - 5 - CG_Text_Width_Ext(s2, 0.19f, 0, FONT_HEADER), y + 13, 0.19f, 0.19f, SB_text, s2, 0, 0, 0, FONT_HEADER);
			}

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
		}
	}

	y += 19;

	tempx = x;

	CG_FillRect(x - 5, y, width + 5, 18, SB_bg2);
	trap_R_SetColor(colorBlack);
	CG_DrawBottom_NoScale(x - 5, y, width + 5, 18, 1);
	trap_R_SetColor(NULL);

	// draw player info headings
	CG_Text_Paint_Ext(tempx, y + 13, 0.24f, 0.28f, colorWhite, CG_TranslateString("Name"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	tempx += INFO_PLAYER_WIDTH;

	// add some extra space when not showing lives in non-LMS
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft)
	{
		tempx += INFO_LIVES_WIDTH;
	}

	CG_Text_Paint_Ext(tempx, y + 13, 0.24f, 0.28f, colorWhite, CG_TranslateString("C R"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	tempx += INFO_CLASS_WIDTH;

	// subtract distance from border
	// everything is aligned right from here onwards
	tempx -= 5;

	if (cgs.gametype == GT_WOLF_LMS)
	{
		tempx += INFO_SCORE_WIDTH;
		CG_Text_Paint_RightAligned_Ext(tempx, y + 13, 0.24f, 0.28f, colorWhite, CG_TranslateString("Score"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
	{
		tempx += INFO_XP_WIDTH;
#ifdef FEATURE_RATING
		if (cgs.skillRating && cg_scoreboard.integer == SCOREBOARD_SR)
		{
			CG_Text_Paint_RightAligned_Ext(tempx, y + 13, 0.24f, 0.28f, colorWhite, "SR", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}
		else
#endif
#ifdef FEATURE_PRESTIGE
		if (cgs.prestige && cg_scoreboard.integer == SCOREBOARD_PR)
		{
			CG_DrawPic(tempx - 14, y + 2, 14, 14, cgs.media.prestigePics[0]);
		}
		else
#endif
		{
			CG_Text_Paint_RightAligned_Ext(tempx, y + 13, 0.24f, 0.28f, colorWhite, CG_TranslateString("XP"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}
	}

	tempx += INFO_LATENCY_WIDTH;
	CG_Text_Paint_RightAligned_Ext(tempx, y + 13, 0.24f, 0.28f, colorWhite, CG_TranslateString("Ping"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);

	if (cgs.gametype != GT_WOLF_LMS && livesleft)
	{
		CG_DrawPicST(tempx + 8, y, INFO_LIVES_WIDTH - 4, INFO_LIVES_WIDTH - 4, 0.f, 0.f, 0.5f, 1.f, team == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);
		//tempx += INFO_LIVES_WIDTH;
	}

	y += 18;

	cg.teamPlayers[TEAM_SPECTATOR] = 0;
	cg.teamPlayers[team]           = 0;
	cg.teamPingMean[team]          = 0.f;
	cg.teamPingSd[team]            = 0.f;

	for (i = 0; i < cg.numScores; i++)
	{
		if (team != cgs.clientinfo[cg.scores[i].client].team)
		{
			if (cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR)
			{
				cg.teamPlayers[TEAM_SPECTATOR]++;
			}
			continue;
		}

		cg.teamPlayers[team]++;
		cg.teamPingMean[team] += cg.scores[i].ping;
	}

	if (cg.teamPlayers[team] != 0)
	{
		cg.teamPingMean[team] = cg.teamPingMean[team] / cg.teamPlayers[team];

		for (i = 0; i < cg.numScores; i++)
		{
			if (team != cgs.clientinfo[cg.scores[i].client].team)
			{
				continue;
			}

			cg.teamPingSd[team] += pow(cg.scores[i].ping - cg.teamPingMean[team], 2);
		}

		cg.teamPingSd[team] = sqrtf(cg.teamPingSd[team] / cg.teamPlayers[team]);
	}

	row_height = use_mini_chars ? 12 : 16;

	// save off y val
	tempy = y;

	// draw color bands
	for (i = 0; i < maxrows; i++)
	{
		if (i % 2 == 0)
		{
			VectorSet(hcolor, .3f, .3f, .3f);  // light
		}
		else
		{
			VectorSet(hcolor, 0.f, 0.f, 0.f);  // dark
		}
		hcolor[3] = fade * 0.3f;

		CG_FillRect(x - 5, y, width + 5, row_height, hcolor);
		trap_R_SetColor(colorBlack);
		CG_DrawBottom_NoScale(x - 5, y, width + 5, row_height, 1);
		trap_R_SetColor(NULL);
		y += row_height;
	}

	hcolor[3] = 1;

	y = (int)tempy;

	// draw player info
	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	for (i = 0; i < cg.numScores && count < maxrows; i++)
	{
		if (team != cgs.clientinfo[cg.scores[i].client].team)
		{
			continue;
		}

		if (y + row_height >= 470)
		{
			continue;
		}

		if (use_mini_chars)
		{
			WM_DrawClientScore_Small(x, y, &cg.scores[i], fade, livesleft);
		}
		else
		{
			WM_DrawClientScore(x, y, &cg.scores[i], fade, livesleft);
		}
		y += row_height;

		count++;
	}

	// draw spectators
	y += row_height;

	for (i = 0; i < cg.numScores; i++)
	{
		if (cgs.clientinfo[cg.scores[i].client].team != TEAM_SPECTATOR)
		{
			continue;
		}
		if (team == TEAM_AXIS && (i % 2))
		{
			continue;
		}
		if (team == TEAM_ALLIES && ((i + 1) % 2))
		{
			continue;
		}

		if (y + row_height >= 470)
		{
			continue;
		}

		if (use_mini_chars)
		{
			WM_DrawClientScore_Small(x, y, &cg.scores[i], fade, livesleft);
			y += row_height;
		}
		else
		{
			WM_DrawClientScore(x, y, &cg.scores[i], fade, livesleft);
			y += row_height;
		}
	}

	return y;
}

/**
 * @brief Draw the normal in-game scoreboard
 * @return
 */
qboolean CG_DrawScoreboard(void)
{
	int   x = 20, y = 6, x_right = SCREEN_WIDTH - x - (INFO_TOTAL_WIDTH - 5);
	float fade;
	int   width = SCREEN_WIDTH - 2 * x + 5;
#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
	int        w;
	const char *s, *s2, *s3;
#endif
	float    fontScale = cg_fontScaleSP.value;
	int      maxrows;
	int      absmaxrows;
	qboolean use_mini_chars = qfalse;

	x       += cgs.wideXoffset;
	x_right += cgs.wideXoffset;

	// don't draw anything if the menu or console is up
	if (cg_paused.integer)
	{
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	// also for pesky scoreboards in demos
	if ((cg.warmup || (cg.demoPlayback && cg.snap->ps.pm_type != PM_INTERMISSION)) && !cg.showScores)
	{
		return qfalse;
	}

	if (cg.showScores || cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		fade = 1.0f;
	}
	else
	{
		float *fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if (!fadeColor)
		{
			return qfalse;
		}
		fade = fadeColor[3];
	}

	y = WM_DrawObjectives(x, y, width, fade);

	if (cgs.gametype == GT_WOLF_STOPWATCH && cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		y = WM_DrawInfoLine(x, 155, fade);

		maxrows    = 8;
		absmaxrows = 10;
	}
	else if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		maxrows    = 9;
		absmaxrows = 16;
	}
	else
	{
		maxrows    = 22;
		absmaxrows = 30;
	}

	qboolean players_with_specs_exceed_limit = cg.teamPlayers[TEAM_SPECTATOR] > 0 && (
		(cg.teamPlayers[TEAM_AXIS] + 1 + (cg.teamPlayers[TEAM_SPECTATOR] + 1) / 2) > maxrows ||
		(cg.teamPlayers[TEAM_ALLIES] + 1 + (cg.teamPlayers[TEAM_SPECTATOR] + 1) / 2) > maxrows
		);
	qboolean players_without_specs_exceed_limit = cg.teamPlayers[TEAM_AXIS] > maxrows || cg.teamPlayers[TEAM_ALLIES] > maxrows;

	if ((cg.snap->ps.pm_type != PM_INTERMISSION && players_with_specs_exceed_limit) || players_without_specs_exceed_limit)
	{
		maxrows        = absmaxrows;
		use_mini_chars = qtrue;
	}

	WM_TeamScoreboard(x, y, TEAM_AXIS, fade, maxrows, use_mini_chars);
	x = x_right;
	WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, maxrows, use_mini_chars);

#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
	if (cg_descriptiveText.integer && cgs.gamestate != GS_INTERMISSION)
	{
		s2 = Binding_FromName("+scores");
		if (!Q_stricmp(s2, "(+scores)"))
		{
			s2 = "TAB";
		}

#ifdef FEATURE_RATING
		if (cgs.skillRating && cg_scoreboard.integer == SCOREBOARD_SR) // Skill Rating
		{
			s3 = CG_TranslateString("Skill Rating view");
		}
		else
#endif
#ifdef FEATURE_PRESTIGE
		if (cgs.prestige && cg_scoreboard.integer == SCOREBOARD_PR)
		{
			s3 = CG_TranslateString("Prestige view");
		}
		else
#endif
		{
			s3 = CG_TranslateString("XP view");
		}
		s = va(CG_TranslateString("%s - Press double-%s quickly to switch scoreboard"), s3, s2);

		w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);
		x = Ccg_WideX(SCREEN_WIDTH / 2) - w / 2;
		y = 450;
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}
#endif

	return qtrue;
}
