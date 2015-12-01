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
 */
/**
 * @file cg_scoreboard.c
 * @brief Draw the scoreboard on top of the game screen
 */

#include "cg_local.h"

// colors and fonts for overlays
vec4_t SB_bg = { 0.16, 0.2f, 0.17f, 0.8f };
vec4_t SB_bg2 = { 0.0f, 0.0f, 0.0f, 0.6f };
vec4_t SB_border = { 1.0f, 1.0f, 1.0f, 0.3f };
vec4_t SB_text = { 0.6f, 0.6f, 0.6f, 1.0f };

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
//#define INFO_TEAM_HEIGHT        24
//#define INFO_BORDER             2
#define INFO_LINE_HEIGHT        30
#define INFO_TOTAL_WIDTH        (INFO_PLAYER_WIDTH + INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH)

/**
 * @brief Draw a client country flag
 *
 * All flags are stored in one single image where they are aligned
 * into a grid of 16x16 fields. Each flag has an id number starting
 * with 0 at the top left corner and ending with 255 in the bottom right
 * corner. Client's flag id is stored in the "uci" field of configstrings.
 *
 * @ingroup GeoIP
 */
static qboolean CG_DrawFlag(float x, float y, float fade, int clientNum)
{
	int client_flag = atoi(Info_ValueForKey(CG_ConfigString(clientNum + CS_PLAYERS), "u"));    // uci

	if (client_flag < 255) // MAX_COUNTRY_NUM
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

int WM_DrawObjectives(int x, int y, int width, float fade)
{
	const char *s;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		const char *buf, *flagshader = NULL, *nameshader = NULL;
		int        rows = 8;

		y += 16 * (rows - 1);

		s   = CG_ConfigString(CS_MULTI_MAPWINNER);
		buf = Info_ValueForKey(s, "w");

		if (atoi(buf) == -1)
		{
			// "ITS A TIE!";
		}
		else if (atoi(buf))
		{
			// "ALLIES";
			flagshader = "ui/assets/portraits/allies_win_flag.tga";
			nameshader = "ui/assets/portraits/text_allies.tga";
		}
		else
		{
			// "AXIS";
			flagshader = "ui/assets/portraits/axis_win_flag.tga";
			nameshader = "ui/assets/portraits/text_axis.tga";
		}

		y += 16 * ((rows - 2) / 2);

		if (flagshader)
		{
			CG_DrawPic(100 + cgs.wideXoffset, 10, 210, 136, trap_R_RegisterShaderNoMip(flagshader));
			CG_DrawPic(325 + cgs.wideXoffset, 10, 210, 136, trap_R_RegisterShaderNoMip(flagshader));
		}

		if (nameshader)
		{
			CG_DrawPic(140 + cgs.wideXoffset, 50, 127, 64, trap_R_RegisterShaderNoMip(nameshader));
			CG_DrawPic(365 + cgs.wideXoffset, 50, 127, 64, trap_R_RegisterShaderNoMip("ui/assets/portraits/text_win.tga"));
		}
		return y;
	}
	// mission time & reinforce time
	else
	{
		int msec, mins, seconds, tens, w;

		CG_FillRect(x - 5, y - 2, width + 5, 21, SB_bg);

		if (CG_ConfigString(CS_CONFIGNAME)[0])
		{
			CG_FillRect(x - 5, y + 19, width + 5, 18, SB_bg);
			CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 40, 1, SB_border);
		}
		else
		{
			CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 21, 1, SB_border);
		}

		y += 14;

		if (cgs.timelimit > 0.0f)
		{
			msec = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);

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
			s = va("%s   ^7%2.0f:%i%i", CG_TranslateString("MISSION TIME:"), (float)mins, tens, seconds);     // float cast to line up with reinforce time
		}

		CG_Text_Paint_Ext(x, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);

		if (cg.warmup)
		{
			s = va("%s %s%i", CG_TranslateString("MATCH BEGINS IN:"), ((cg.warmup - cg.time) / 1000) < 3 ? "^1" : "^2", (cg.warmup - cg.time) / 1000 + 1);
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
			else     // no team (spectator mode)
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
					s = va("%s   %s%i", CG_TranslateString("REINFORCE TIME:"), (seconds <= 2 && cgs.clientinfo[cg.clientNum].health == 0) ? "^3" : "^F", seconds);
				}
				CG_Text_Paint_Ext(SCREEN_WIDTH - 20 - CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER) + cgs.wideXoffset, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
			}
		}

		switch (cgs.gametype)
		{
		case GT_WOLF_STOPWATCH:
			s = va("%s %i", CG_TranslateString("STOPWATCH ROUND"), cgs.currentRound + 1);
			w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
			break;
		case GT_WOLF_LMS:
			s = va("%s %i  %s %i-%i", CG_TranslateString("ROUND"), cgs.currentRound + 1, CG_TranslateString("SCORE"), cg.teamWonRounds[1], cg.teamWonRounds[0]);
			w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
			break;
		case GT_WOLF_CAMPAIGN:
			s = va(CG_TranslateString("MAP %i of %i"), cgs.currentCampaignMap + 1, cgs.campaignData.mapCount);
			w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
			break;
		case GT_WOLF_MAPVOTE:
			s = (cgs.mapVoteMapY ? va(CG_TranslateString("MAP %i of %i"), cgs.mapVoteMapX + 1, cgs.mapVoteMapY) : "");
			w = CG_Text_Width_Ext(s, 0.25f, 0, FONT_HEADER);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y, 0.25f, 0.25f, SB_text, s, 0, 0, 0, FONT_HEADER);
			break;
		default:
			break;
		}

		y += 18;

		if (CG_ConfigString(CS_CONFIGNAME)[0])
		{
			s = va(CG_TranslateString("Config: ^7%s^7"), CG_ConfigString(CS_CONFIGNAME));
			CG_Text_Paint_Ext(x, y, 0.24f, 0.28f, SB_text, s, 0, 0, 0, FONT_TEXT);

			y += 18;
		}
	}
	return y;
}

int SkillNumForClass(int classNum)
{
	switch (classNum)
	{
	case PC_SOLDIER:
		return SK_SOLDIER;
	case PC_MEDIC:
		return SK_MEDIC;
	case PC_ENGINEER:
		return SK_ENGINEER;
	case PC_FIELDOPS:
		return SK_SIGNALS;
	case PC_COVERTOPS:
		return SK_COVERTOPS;
	default:
		return SK_BATTLE_SENSE;
	}
}

/**
 * @brief Real printable charater count
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

static void WM_DrawClientScore(int x, int y, score_t *score, float *color, float fade, qboolean livesleft)
{
	int          maxchars = 16, offset = 0;
	int          i, j;
	float        tempx;
	vec4_t       hcolor;
	clientInfo_t *ci;
	char         buf[64];

	if (y + 16 >= 470)
	{
		return;
	}

	ci = &cgs.clientinfo[score->client];

	if (score->client == cg.snap->ps.clientNum)
	{
		hcolor[3] = fade * 0.3;
		VectorSet(hcolor, .5f, .5f, .2f);           // yellow

		CG_FillRect(x - 5, y, (INFO_TOTAL_WIDTH + 5), 15, hcolor);
	}

	tempx = x;

	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	y += 12;

	// add some extra space when not showing lives in non-LMS
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft)
	{
		maxchars += 2;
	}

	// draw GeoIP flag
	if (score->ping != -1 && score->ping != 999 && cg_countryflags.integer)
	{
		if (CG_DrawFlag(tempx - 3, y - 11, fade, ci->clientNum))
		{
			offset   += 15;
			tempx    += 15;
			maxchars -= 2;
		}
	}

	if (ci->team != TEAM_SPECTATOR)
	{
		// draw ready icon if client is ready..
		if ((score->scoreflags & 1) && cgs.gamestate != GS_PLAYING)
		{
			CG_DrawPic(tempx - 1, y - 10, 10, 10, cgs.media.readyShader);
			offset   += 12;
			tempx    += 12;
			maxchars -= 2;
		}

		if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			CG_DrawPic(tempx - 1, y - 10, 10, 10, cgs.media.objectiveShader);
			offset   += 12;
			tempx    += 12;
			maxchars -= 2;
		}
		else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && (ci->powerups & (1 << PW_OPS_DISGUISED)))
		{
			CG_DrawPic(tempx - 1, y - 10, 10, 10, ci->team == TEAM_AXIS ? cgs.media.alliedUniformShader : cgs.media.axisUniformShader);
			offset   += 12;
			tempx    += 12;
			maxchars -= 2;
		}

		// draw the skull icon if out of lives
		if (score->respawnsLeft == -2 || (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == -1))
		{
			CG_DrawPic(tempx - 1, y - 10, 10, 10, cgs.media.scoreEliminatedShader);
			offset   += 12;
			tempx    += 12;
			maxchars -= 2;
		}
		else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == 0 && cgs.gamestate != GS_INTERMISSION)
		{
			CG_DrawPic(tempx - 1, y - 10, 10, 10, cgs.media.medicIcon);
			offset   += 12;
			tempx    += 12;
			maxchars -= 2;
		}
	}

	// draw name
	CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorWhite, ci->name, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	maxchars -= CG_Text_Width_Ext(ci->name, 0.24, 0, FONT_TEXT);

	// draw medals
	buf[0] = '\0';
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		for (j = 0; j < ci->medals[i]; j++)
		{
			Q_strcat(buf, sizeof(buf), va("^%c%c", COLOR_RED + i, skillNames[i][0]));
		}
	}
	maxchars--;

	// CG_Text_Paint_Ext will draw everything if maxchars <= 0
	if (maxchars > 0)
	{
		CG_Text_Paint_Ext(tempx + (CG_drawStrlen(ci->name) * 8 + 8), y, 0.24, 0.28, colorWhite, buf, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}

	tempx += INFO_PLAYER_WIDTH - offset;

	// add the extra room here
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft && ci->team != TEAM_SPECTATOR)
	{
		tempx += INFO_LIVES_WIDTH;
	}

	if (ci->team == TEAM_SPECTATOR)
	{
		const char *s, *p;
		int        w, totalwidth = INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH;

		// Show connecting people as CONNECTING
		if (score->ping == -1)
		{
			s = CG_TranslateString("^3CONNECTING");
			p = "";
		}
		else
		{
			s = CG_TranslateString("^3SPECTATOR");
			p = va("%4i", score->ping);
		}

		w = CG_Text_Width_Ext(s, 0.24, 0, FONT_TEXT);
		CG_Text_Paint_Ext(tempx + totalwidth - w - INFO_LATENCY_WIDTH, y, 0.24, 0.28, colorYellow, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		CG_Text_Paint_Ext(tempx + totalwidth - INFO_LATENCY_WIDTH, y, 0.24, 0.28, colorWhite, p, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);

		return;
	}
	// allow MV clients see the class of its merged client's on the scoreboard
	else if (cg.snap->ps.persistant[PERS_TEAM] == ci->team || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_INTERMISSION
#if FEATURE_MULTIVIEW
	         || CG_mvMergedClientLocate(score->client)
#endif
	         )
	{
		CG_DrawPic(tempx - 3, y - 12, 14, 14, cgs.media.skillPics[SkillNumForClass(ci->cls)]);

		if (cgs.clientinfo[ci->clientNum].rank > 0)
		{
			CG_DrawPic(tempx + 13, y - 12, 16, 16, rankicons[cgs.clientinfo[ci->clientNum].rank][cgs.clientinfo[ci->clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
		}
	}

	tempx += INFO_CLASS_WIDTH;

	CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorWhite, va("^7%6i", score->score), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		tempx += INFO_SCORE_WIDTH;
	}
	else
	{
		tempx += INFO_XP_WIDTH;
	}

	if (score->ping == -1)
	{
		CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorRed, "^1CONN^7", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else if (score->scoreflags & 2)
	{
		CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorWhite, " BOT", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
	{
		CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorWhite, va("%4i", score->ping), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	tempx += INFO_LATENCY_WIDTH;

	if (cg_gameType.integer != GT_WOLF_LMS && livesleft)
	{
		if (score->respawnsLeft >= 0)
		{
			CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorWhite, va("%2i", score->respawnsLeft), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}
		else
		{
			CG_Text_Paint_Ext(tempx, y, 0.24, 0.28, colorWhite, " -", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}
	}
}

const char *WM_TimeToString(float msec)
{
	int mins, seconds, tens;

	seconds  = msec / 1000;
	mins     = seconds / 60;
	seconds -= mins * 60;
	tens     = seconds / 10;
	seconds -= tens * 10;

	return va("%i:%i%i", mins, tens, seconds);
}

static void WM_DrawClientScore_Small(int x, int y, score_t *score, float *color, float fade, qboolean livesleft)
{
	int          maxchars = 23, offset = 0;
	float        tempx;
	vec4_t       hcolor;
	clientInfo_t *ci;
	int          i, j;    // To draw medals
	char         buf[64]; // To draw medals

	if (y + 12 >= 470)
	{
		return;
	}

	ci = &cgs.clientinfo[score->client];

	if (score->client == cg.snap->ps.clientNum)
	{
		hcolor[3] = fade * 0.3;
		VectorSet(hcolor, .5f, .5f, .2f); // yellow

		CG_FillRect(x - 5, y, (INFO_TOTAL_WIDTH + 5), 11, hcolor);
	}

	tempx = x;

	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	y += 10;

	// draw GeoIP flag
	if (score->ping != -1 && score->ping != 999 && cg_countryflags.integer)
	{
		if (CG_DrawFlag(tempx - 3, y - 10, fade, ci->clientNum))
		{
			offset   += 15;
			tempx    += 15;
			maxchars -= 2;
		}
	}

	if (ci->team != TEAM_SPECTATOR)
	{
		// draw ready icon if client is ready..
		if ((score->scoreflags & 1) && (cgs.gamestate == GS_WARMUP || cgs.gamestate == GS_INTERMISSION))
		{
			CG_DrawPic(tempx + 1, y - 9, 10, 10, cgs.media.readyShader);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}

		if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			CG_DrawPic(tempx + 1, y - 9, 10, 10, cgs.media.objectiveShader);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}

		// draw the skull icon if out of lives
		if (score->respawnsLeft == -2 || (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == -1))
		{
			CG_DrawPic(tempx + 1, y - 9, 10, 10, cgs.media.scoreEliminatedShader);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}
		else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == 0 && cgs.gamestate != GS_INTERMISSION)
		{
			CG_DrawPic(tempx + 1, y - 9, 10, 10, cgs.media.medicIcon);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}
	}

	// draw name
	CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorWhite, ci->name, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	maxchars -= CG_Text_Width_Ext(ci->name, 0.20, 0, FONT_TEXT);

	// draw medals
	buf[0] = '\0';
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		for (j = 0; j < ci->medals[i]; j++)
		{
			Q_strcat(buf, sizeof(buf), va("^%c%c", COLOR_RED + i, skillNames[i][0]));
		}
		maxchars--;
	}

	if (maxchars > 0)
	{
		CG_Text_Paint_Ext(tempx + (CG_drawStrlen(ci->name) * 8 + 8), y, 0.20, 0.25, colorWhite, buf, 0, maxchars, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}

	tempx += INFO_PLAYER_WIDTH - offset;

	// add the extra room here
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft && ci->team != TEAM_SPECTATOR)
	{
		tempx += INFO_LIVES_WIDTH;
	}

	if (ci->team == TEAM_SPECTATOR)
	{
		const char *s, *p;
		int        w, totalwidth = INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH + 6;

		// Show connecting people as CONNECTING
		if (score->ping == -1)
		{
			s = CG_TranslateString("^3CONNECTING");
			p = "";
		}
		else
		{
			s = CG_TranslateString("^3SPECTATOR");
			p = va("%4i", score->ping);
		}
		w = CG_Text_Width_Ext(s, 0.20, 0, FONT_TEXT);
		CG_Text_Paint_Ext(tempx + totalwidth - w - INFO_LATENCY_WIDTH, y, 0.20, 0.25, colorYellow, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		CG_Text_Paint_Ext(tempx + totalwidth - INFO_LATENCY_WIDTH, y, 0.20, 0.25, colorWhite, p, 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		return;
	}
	else if (cg.snap->ps.persistant[PERS_TEAM] == ci->team || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_DrawPic(tempx, y - 9, 10, 10, cgs.media.skillPics[SkillNumForClass(ci->cls)]);

		if (cgs.clientinfo[ci->clientNum].rank > 0)
		{
			CG_DrawPic(tempx + 13, y - 9, 12, 12, rankicons[cgs.clientinfo[ci->clientNum].rank][cgs.clientinfo[ci->clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
		}
	}

	tempx += INFO_CLASS_WIDTH + 6;

	CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorWhite, va("^7%6i", score->score), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		tempx += INFO_SCORE_WIDTH;
	}
	else
	{
		tempx += INFO_XP_WIDTH;
	}

	if (score->ping == -1)
	{
		CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorRed, "^1CONN^7", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else if (score->scoreflags & 2)
	{
		CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorWhite, " BOT", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}
	else
	{
		CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorWhite, va("%4i", score->ping), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	}

	tempx += INFO_LATENCY_WIDTH;

	if (cg_gameType.integer != GT_WOLF_LMS && livesleft)
	{
		if (score->respawnsLeft >= 0)
		{
			CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorWhite, va("%2i", score->respawnsLeft), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}
		else
		{
			CG_Text_Paint_Ext(tempx, y, 0.20, 0.25, colorWhite, " -", 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		}
		tempx += INFO_LIVES_WIDTH;
	}
}

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
	defender = atoi(Info_ValueForKey(s, "d")); // defender

	s      = CG_ConfigString(CS_MULTI_MAPWINNER);
	winner = atoi(Info_ValueForKey(s, "w"));

	if (cgs.currentRound)
	{
		// first round
		s = va(CG_TranslateString("CLOCK IS NOW SET TO %s!"), WM_TimeToString(cgs.nextTimeLimit * 60.f * 1000.f));
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

static int WM_TeamScoreboard(int x, int y, team_t team, float fade, int maxrows, int absmaxrows)
{
	vec4_t     hcolor;
	float      tempx, tempy;
	int        i;
	int        count          = 0;
	int        width          = INFO_TOTAL_WIDTH;
	qboolean   use_mini_chars = qfalse, livesleft = qfalse;
	const char *buffer        = CG_ConfigString(CS_SERVERINFO);
	const char *str           = Info_ValueForKey(buffer, "g_maxlives");

	if (str && *str && atoi(str))
	{
		livesleft = qtrue;
	}

	if (!livesleft)
	{
		str = Info_ValueForKey(buffer, "g_alliedmaxlives");
		if (str && *str && atoi(str))
		{
			livesleft = qtrue;
		}
	}

	if (!livesleft)
	{
		str = Info_ValueForKey(buffer, "g_axismaxlives");
		if (str && *str && atoi(str))
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
		if (team == TEAM_AXIS)
		{
			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, SB_text, va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS")), 0, 0, 0, FONT_HEADER);
		}
		else if (team == TEAM_ALLIES)
		{
			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, SB_text, va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], cg.teamPlayers[team] < 2 ? CG_TranslateString("PLAYER") : CG_TranslateString("PLAYERS")), 0, 0, 0, FONT_HEADER);
		}
	}

	y += 19;

	tempx = x;

	CG_FillRect(x - 5, y, width + 5, 18, SB_bg2);
	trap_R_SetColor(colorBlack);
	CG_DrawBottom_NoScale(x - 5, y, width + 5, 18, 1);
	trap_R_SetColor(NULL);

	// draw player info headings
	CG_Text_Paint_Ext(tempx, y + 13, 0.24, 0.28, colorWhite, CG_TranslateString("Name"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	tempx += INFO_PLAYER_WIDTH;

	// add some extra space when not showing lives in non-LMS
	if (cg_gameType.integer != GT_WOLF_LMS && !livesleft)
	{
		tempx += INFO_LIVES_WIDTH;
	}

	CG_Text_Paint_Ext(tempx, y + 13, 0.24, 0.28, colorWhite, CG_TranslateString("C R"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	tempx += INFO_CLASS_WIDTH;

	if (cgs.gametype == GT_WOLF_LMS)
	{
		CG_Text_Paint_Ext(tempx + 22, y + 13, 0.24, 0.28, colorWhite, CG_TranslateString("Score"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		tempx += INFO_SCORE_WIDTH;
	}
	else
	{
		CG_Text_Paint_Ext(tempx + 30, y + 13, 0.24, 0.28, colorWhite, CG_TranslateString("XP"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
		tempx += INFO_XP_WIDTH;
	}

	CG_Text_Paint_Ext(tempx, y + 13, 0.24, 0.28, colorWhite, CG_TranslateString("Ping"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, FONT_TEXT);
	tempx += INFO_LATENCY_WIDTH;

	if (cgs.gametype != GT_WOLF_LMS && livesleft)
	{
		CG_DrawPicST(tempx + 2, y, INFO_LIVES_WIDTH - 4, 16, 0.f, 0.f, 0.5f, 1.f, team == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);
		tempx += INFO_LIVES_WIDTH;
	}

	y += 18;

	cg.teamPlayers[team] = 0;
	for (i = 0; i < cg.numScores; i++)
	{
		if (team != cgs.clientinfo[cg.scores[i].client].team)
		{
			continue;
		}

		cg.teamPlayers[team]++;
	}

	if (cg.teamPlayers[team] > maxrows)
	{
		maxrows        = absmaxrows;
		use_mini_chars = qtrue;

	}
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
		hcolor[3] = fade * 0.3;

		if (use_mini_chars)
		{
			CG_FillRect(x - 5, y, width + 5, 12, hcolor);
			trap_R_SetColor(colorBlack);
			CG_DrawBottom_NoScale(x - 5, y, width + 5, 12, 1);
			trap_R_SetColor(NULL);
			y += 12;

		}
		else
		{
			CG_FillRect(x - 5, y, width + 5, 16, hcolor);
			trap_R_SetColor(colorBlack);
			CG_DrawBottom_NoScale(x - 5, y, width + 5, 16, 1);
			trap_R_SetColor(NULL);
			y += 16;
		}
	}

	hcolor[3] = 1;

	y = tempy;

	// draw player info
	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	for (i = 0; i < cg.numScores && count < maxrows; i++)
	{
		if (team != cgs.clientinfo[cg.scores[i].client].team)
		{
			continue;
		}

		if (use_mini_chars)
		{
			WM_DrawClientScore_Small(x, y, &cg.scores[i], hcolor, fade, livesleft);
			y += 12;
		}
		else
		{
			WM_DrawClientScore(x, y, &cg.scores[i], hcolor, fade, livesleft);
			y += 16;
		}

		count++;
	}

	// draw spectators
	if (use_mini_chars)
	{
		y += 12;
	}
	else
	{
		y += 16;
	}

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

		if (use_mini_chars)
		{
			WM_DrawClientScore_Small(x, y, &cg.scores[i], hcolor, fade, livesleft);
			y += 12;
		}
		else
		{
			WM_DrawClientScore(x, y, &cg.scores[i], hcolor, fade, livesleft);
			y += 16;
		}
	}

	return y;
}

/*
=================
Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard(void)
{
	int   x = 20, y = 6, x_right = SCREEN_WIDTH - x - (INFO_TOTAL_WIDTH - 5);
	float fade;
	int   width = SCREEN_WIDTH - 2 * x + 5;

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

		WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 8, 10);
		x = x_right;
		WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, 8, 10);
	}
	else
	{
		if (cg.snap->ps.pm_type == PM_INTERMISSION)
		{
			WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 9, 12);
			x = x_right;
			WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, 9, 12);
		}
		else
		{
			WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 24, 32);
			x = x_right;
			WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, 24, 32);
		}
	}

	return qtrue;
}
