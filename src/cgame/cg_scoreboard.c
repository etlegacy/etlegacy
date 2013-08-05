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
 * @file cg_scoreboard.c
 * @brief draw the scoreboard on top of the game screen
 */

#include "cg_local.h"

#define SCOREBOARD_WIDTH    (31 * BIGCHAR_WIDTH)

vec4_t clrUiBack = { 0.f, 0.f, 0.f, .6f };
vec4_t clrUiBar = { .16f, .2f, .17f, .8f };
vec4_t tclr = { 0.6f, 0.6f, 0.6f, 1.0f };

/*
=================
WM_DrawObjectives
=================
*/

#define INFO_PLAYER_WIDTH       134
#define INFO_SCORE_WIDTH        56
#define INFO_XP_WIDTH           36
#define INFO_CLASS_WIDTH        50
#define INFO_LATENCY_WIDTH      40
#define INFO_LIVES_WIDTH        20
#define INFO_TEAM_HEIGHT        24
#define INFO_BORDER             2
#define INFO_LINE_HEIGHT        30
#define INFO_TOTAL_WIDTH        (INFO_PLAYER_WIDTH + INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH)

// GeoIP
#define FLAG_STEP       32.0f
qboolean cf_draw(float x, float y, float fade, int clientNum)
{
	unsigned int client_flag = atoi(Info_ValueForKey(CG_ConfigString(clientNum + CS_PLAYERS), "u"));    // uci

	if (client_flag < 255)
	{
		unsigned int flag_sd = 512;
		float        alpha[4];
		float        x1 = (float)((client_flag * (unsigned int)FLAG_STEP) % flag_sd);
		float        y1 = (float)(floor((client_flag * FLAG_STEP) / flag_sd) * FLAG_STEP);
		float        x2 = x1 + FLAG_STEP;
		float        y2 = y1 + FLAG_STEP;

		alpha[0] = alpha[1] = alpha[2] = 1.0; alpha[3] = fade;

		trap_R_SetColor(alpha);
		CG_DrawPicST(x, y, FLAG_STEP, FLAG_STEP, x1 / flag_sd, y1 / flag_sd, x2 / flag_sd, y2 / flag_sd, cgs.media.countryFlags);
		trap_R_SetColor(NULL);
		return qtrue;
	}
	return qfalse;
}

int WM_DrawObjectives(int x, int y, int width, float fade)
{
	const char *s;
	int        rows;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		const char *s, *buf, *shader = NULL, *flagshader = NULL, *nameshader = NULL;

		rows = 8;
		y   += SMALLCHAR_HEIGHT * (rows - 1);

		s   = CG_ConfigString(CS_MULTI_MAPWINNER);
		buf = Info_ValueForKey(s, "winner");

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

		y += SMALLCHAR_HEIGHT * ((rows - 2) / 2);

		if (flagshader)
		{
			CG_DrawPic(100 + cgs.wideXoffset, 10, 210, 136, trap_R_RegisterShaderNoMip(flagshader));
			CG_DrawPic(325 + cgs.wideXoffset, 10, 210, 136, trap_R_RegisterShaderNoMip(flagshader));
		}

		if (shader)
		{
			CG_DrawPic(229 + cgs.wideXoffset, 10, 182, 136, trap_R_RegisterShaderNoMip(shader));
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
		int msec, mins, seconds, tens;

		rows = 1;

		CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBack);
		CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBar);
		CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 21, 1, colorBlack);

		y += SMALLCHAR_HEIGHT * (rows - 1);
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
			s = va("%s %s", CG_TranslateString("MISSION TIME:"), CG_TranslateString("WARMUP"));
		}
		else if (msec < 0 && cgs.timelimit > 0.0f)
		{
			if (cgs.gamestate == GS_WAITING_FOR_PLAYERS)
			{
				s = va("%s %s", CG_TranslateString("MISSION TIME:"), CG_TranslateString("GAME STOPPED"));
			}
			else
			{
				s = va("%s %s", CG_TranslateString("MISSION TIME:"), CG_TranslateString("SUDDEN DEATH"));
			}
		}
		else
		{
			s = va("%s   %2.0f:%i%i", CG_TranslateString("MISSION TIME:"), (float)mins, tens, seconds);     // float cast to line up with reinforce time
		}

		CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);

		// FIXME: do a switch for gametype
		if (cgs.gametype != GT_WOLF_LMS)
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

				s = va("%s %2.0f:%i%i", CG_TranslateString("REINFORCE TIME:"), (float)mins, tens, seconds);
				CG_Text_Paint_Ext(SCREEN_WIDTH - 20 - CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1) + cgs.wideXoffset, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
			}
		}
		if (cgs.gametype == GT_WOLF_STOPWATCH)
		{
			int w;

			s = va("%s %i", CG_TranslateString("STOPWATCH ROUND"), cgs.currentRound + 1);
			w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
		else if (cgs.gametype == GT_WOLF_LMS)
		{
			int w;

			s = va("%s %i  %s %i-%i", CG_TranslateString("ROUND"), cgs.currentRound + 1, CG_TranslateString("SCORE"), cg.teamWonRounds[1], cg.teamWonRounds[0]);
			w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
		else if (cgs.gametype == GT_WOLF_CAMPAIGN)
		{
			int w;

			s = va("MAP %i of %i", cgs.currentCampaignMap + 1, cgs.campaignData.mapCount);
			w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
		// MAPVOTE
		else if (cgs.gametype == GT_WOLF_MAPVOTE)
		{
			int w;

			s = (cgs.mapVoteMapY ? va("MAP %i of %i", cgs.mapVoteMapX + 1, cgs.mapVoteMapY) : "");
			w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);

			CG_Text_Paint_Ext(x + 300 - w * 0.5f, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}

		y += SMALLCHAR_HEIGHT * 2;
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

static void WM_DrawClientScore(int x, int y, score_t *score, float *color, float fade)
{
	int          maxchars, offset;
	int          i, j;
	float        tempx;
	vec4_t       hcolor;
	clientInfo_t *ci;
	char         buf[64];

	if (y + SMALLCHAR_HEIGHT >= 470)
	{
		return;
	}

	ci = &cgs.clientinfo[score->client];

	if (score->client == cg.snap->ps.clientNum)
	{
		hcolor[3] = fade * 0.3;
		VectorSet(hcolor, .5f, .5f, .2f);           // DARK-RED

		CG_FillRect(x - 5, y, (INFO_TOTAL_WIDTH + 5), SMALLCHAR_HEIGHT - 1, hcolor);
	}

	tempx = x;

	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	maxchars = 16;
	offset   = 0;

	if (ci->team != TEAM_SPECTATOR)
	{
		/* FIXME: adjust x,y coordinates ...
		// draw ready icon if client is ready..
		if (score->scoreflags & 1 && cgs.gamestate != GS_PLAYING)
		{
		    CG_DrawPic(tempx - 3, y + 1, 14, 14, cgs.media.readyIcon);
		    offset += 14;
		    tempx += 14;
		    maxchars -= 2;
		}
		*/

		if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			CG_DrawPic(tempx - 1, y + 1, 10, 10, cgs.media.objectiveShader);
			offset   += 10;
			tempx    += 10;
			maxchars -= 2;
		}

		// draw the skull icon if out of lives
		if (score->respawnsLeft == -2 || (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == -1))
		{
			CG_DrawPic(tempx - 1, y + 1, 10, 10, cgs.media.scoreEliminatedShader);
			offset   += 10;
			tempx    += 10;
			maxchars -= 2;
		}
		else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == 0)
		{
			CG_DrawPic(tempx - 1, y + 1, 10, 10, cgs.media.medicIcon);
			offset   += 10;
			tempx    += 10;
			maxchars -= 2;
		}
	}

	// GeoIP - draw flag before name
	if (score->ping != -1 && score->ping != 999 && cg_countryflags.integer)
	{
		if (cf_draw(tempx - 11, y - 8, fade, ci->clientNum))
		{
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}
	}

	// draw name
	CG_DrawStringExt(tempx, y, ci->name, hcolor, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, maxchars);
	maxchars -= CG_DrawStrlen(ci->name);

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

	// CG_DrawStringExt will draw everything if maxchars <= 0
	if (maxchars > 0)
	{
		CG_DrawStringExt(tempx + (BG_drawStrlen(ci->name) * SMALLCHAR_WIDTH + SMALLCHAR_WIDTH),
		                 y, buf, hcolor, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, maxchars);
	}

	tempx += INFO_PLAYER_WIDTH - offset;

	if (ci->team == TEAM_SPECTATOR)
	{
		const char *s;
		int        w, totalwidth = INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH - 8;

		// Show connecting people as CONNECTING
		if (score->ping == -1)
		{
			s = CG_TranslateString("^3CONNECTING");
		}
		else
		{
			s = CG_TranslateString("^3SPECTATOR");
		}
		w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;
		CG_DrawSmallString(tempx + totalwidth - w, y, s, fade);
		return;
	}
	// allow MV clients see the class of its merged client's on the scoreboard
	else if (cg.snap->ps.persistant[PERS_TEAM] == ci->team || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_INTERMISSION
#if FEATURE_MULTIVIEW
	         || CG_mvMergedClientLocate(score->client)
#endif
	         )
	{
		CG_DrawPic(tempx - 3, y + 1, 14, 14, cgs.media.skillPics[SkillNumForClass(ci->cls)]);
	}
	tempx += INFO_CLASS_WIDTH;

	CG_DrawSmallString(tempx, y, va("^7%3i", score->score), fade);
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
		CG_DrawSmallString(tempx, y, "^1CONN^7", fade);
	}
	else if (score->scoreflags & 2)
	{
		CG_DrawSmallString(tempx, y, " BOT", fade);
	}
	else
	{
		CG_DrawSmallString(tempx, y, va("%4i", score->ping), fade);
	}
	tempx += INFO_LATENCY_WIDTH;

	if (cg_gameType.integer != GT_WOLF_LMS)
	{
		if (score->respawnsLeft >= 0)
		{
			CG_DrawSmallString(tempx, y, va("%2i", score->respawnsLeft), fade);
		}
		else
		{
			CG_DrawSmallString(tempx, y, " -", fade);
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

static void WM_DrawClientScore_Small(int x, int y, score_t *score, float *color, float fade)
{
	int          maxchars, offset;
	float        tempx;
	vec4_t       hcolor;
	clientInfo_t *ci;
	int          i, j;    // To draw medals
	char         buf[64]; // To draw medals

	if (y + MINICHAR_HEIGHT >= 470)
	{
		return;
	}

	ci = &cgs.clientinfo[score->client];

	if (score->client == cg.snap->ps.clientNum)
	{
		hcolor[3] = fade * 0.3;
		VectorSet(hcolor, .5f, .5f, .2f); // DARK-RED

		CG_FillRect(x - 5, y, (INFO_TOTAL_WIDTH + 5), MINICHAR_HEIGHT - 1, hcolor);
	}

	tempx = x;

	VectorSet(hcolor, 1, 1, 1);
	hcolor[3] = fade;

	maxchars = 16;
	offset   = 0;

	if (ci->team != TEAM_SPECTATOR)
	{

		/* FIXME adjust x,y coordinates ...
		// draw ready icon if client is ready..
		if ( score->scoreflags & 1 && ( cgs.gamestate == GS_WARMUP || cgs.gamestate == GS_INTERMISSION ) ) {
		    CG_DrawPic( tempx-2 + 1, y + 1, 14, 14, cgs.media.readyIcon );
		    offset += 14;
		    tempx += 14;
		    maxchars -= 2;
		}
		*/

		if (ci->powerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG)))
		{
			CG_DrawPic(tempx + 1, y + 1, 10, 10, cgs.media.objectiveShader);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}

		// draw the skull icon if out of lives
		if (score->respawnsLeft == -2 || (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == -1))
		{
			CG_DrawPic(tempx, y, 12, 12, cgs.media.scoreEliminatedShader);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}
		else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && ci->team == cgs.clientinfo[cg.clientNum].team && cgs.clientinfo[score->client].health == 0)
		{
			CG_DrawPic(tempx + 1, y + 1, 10, 10, cgs.media.medicIcon);
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}
	}

	// GeoIP - draw flag before name
	if (score->ping != -1 && score->ping != 999 && cg_countryflags.integer)
	{
		if (cf_draw(tempx - 11, y - 10, fade, ci->clientNum))
		{
			offset   += 14;
			tempx    += 14;
			maxchars -= 2;
		}
	}

	// draw name
	CG_DrawStringExt(tempx, y, ci->name, hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, maxchars);

	// draw medals
	maxchars -= CG_DrawStrlen(ci->name);

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
		CG_DrawStringExt(tempx + (BG_drawStrlen(ci->name) * MINICHAR_WIDTH + MINICHAR_WIDTH),
		                 y, buf, hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, maxchars);
	}

	tempx += INFO_PLAYER_WIDTH - offset;

	if (ci->team == TEAM_SPECTATOR)
	{
		const char *s;
		int        w, totalwidth = INFO_CLASS_WIDTH + INFO_SCORE_WIDTH + INFO_LATENCY_WIDTH - 8;

		// Show connecting people as CONNECTING
		if (score->ping == -1)
		{
			s = CG_TranslateString("^3CONNECTING");
		}
		else
		{
			s = CG_TranslateString("^3SPECTATOR");
		}
		w = CG_DrawStrlen(s) * MINICHAR_WIDTH;

		CG_DrawStringExt(tempx + totalwidth - w, y, s, hcolor, qfalse, qfalse,
		                 MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
		return;
	}
	else if (cg.snap->ps.persistant[PERS_TEAM] == ci->team || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_DrawPic(tempx, y - 2, 12, 12, cgs.media.skillPics[SkillNumForClass(ci->cls)]);
	}
	tempx += INFO_CLASS_WIDTH;

	CG_DrawStringExt(tempx, y, va("^7%3i", score->score), hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
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
		CG_DrawStringExt(tempx, y, "^1CONN", hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
	}
	else if (score->scoreflags & 2)
	{
		CG_DrawStringExt(tempx, y, " BOT", hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
	}
	else
	{
		CG_DrawStringExt(tempx, y, va("%4i", score->ping), hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
	}

	tempx += INFO_LATENCY_WIDTH;

	if (cg_gameType.integer != GT_WOLF_LMS)
	{
		if (score->respawnsLeft >= 0)
		{
			CG_DrawStringExt(tempx, y, va("%2i", score->respawnsLeft), hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
		}
		else
		{
			CG_DrawStringExt(tempx, y, " -", hcolor, qfalse, qfalse, MINICHAR_WIDTH, MINICHAR_HEIGHT, 0);
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
	defender = atoi(Info_ValueForKey(s, "defender"));

	s      = CG_ConfigString(CS_MULTI_MAPWINNER);
	winner = atoi(Info_ValueForKey(s, "winner"));

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
				s = "ALLIES SUCCESSFULLY BEAT THE CLOCK!"; // FIXME: translations
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

	CG_FillRect(320 - w / 2, y, w, 20, clrUiBar);
	CG_DrawRect_FixedBorder(320 - w / 2, y, w, 20, 1, colorBlack);

	w = CG_Text_Width_Ext(s, 0.25f, 0, &cgs.media.limboFont1);

	CG_Text_Paint_Ext(320 - w * 0.5f, y + 15, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);

	return y + INFO_LINE_HEIGHT + 6;
}

static int WM_TeamScoreboard(int x, int y, team_t team, float fade, int maxrows,
                             int absmaxrows)
{
	vec4_t   hcolor;
	float    tempx, tempy;
	int      i;
	int      count          = 0;
	int      width          = INFO_TOTAL_WIDTH;
	qboolean use_mini_chars = qfalse;

	CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBack);
	CG_FillRect(x - 5, y - 2, width + 5, 21, clrUiBar);

	Vector4Set(hcolor, 0, 0, 0, fade);
	CG_DrawRect_FixedBorder(x - 5, y - 2, width + 5, 21, 1, colorBlack);

	// draw header
	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		char *s;

		if (team == TEAM_AXIS)
		{
			s = va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], CG_TranslateString("PLAYERS"));
			s = va("%s ^3%s", s, cg.teamFirstBlood == TEAM_AXIS ? CG_TranslateString("FIRST BLOOD") : "");

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
		else if (team == TEAM_ALLIES)
		{
			s = va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], CG_TranslateString("PLAYERS"));
			s = va("%s ^3%s", s, cg.teamFirstBlood == TEAM_ALLIES ? CG_TranslateString("FIRST BLOOD") : "");

			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
		}
	}
	else
	{
		if (team == TEAM_AXIS)
		{
			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, va("%s [%d] (%d %s)", CG_TranslateString("AXIS"), cg.teamScores[0], cg.teamPlayers[team], CG_TranslateString("PLAYERS")), 0, 0, 0, &cgs.media.limboFont1);
		}
		else if (team == TEAM_ALLIES)
		{
			CG_Text_Paint_Ext(x, y + 13, 0.25f, 0.25f, tclr, va("%s [%d] (%d %s)", CG_TranslateString("ALLIES"), cg.teamScores[1], cg.teamPlayers[team], CG_TranslateString("PLAYERS")), 0, 0, 0, &cgs.media.limboFont1);
		}
	}

	y += SMALLCHAR_HEIGHT + 3;

	tempx = x;

	CG_FillRect(x - 5, y, width + 5, 18, clrUiBack);
	trap_R_SetColor(colorBlack);
	CG_DrawBottom_NoScale(x - 5, y, width + 5, 18, 1);
	trap_R_SetColor(NULL);

	// draw player info headings
	CG_DrawSmallString(tempx, y, CG_TranslateString("Name"), fade);
	tempx += INFO_PLAYER_WIDTH;

	CG_DrawSmallString(tempx, y, CG_TranslateString("Class"), fade);
	tempx += INFO_CLASS_WIDTH;

	if (cgs.gametype == GT_WOLF_LMS)
	{
		CG_DrawSmallString(tempx, y, CG_TranslateString("Score"), fade);
		tempx += INFO_SCORE_WIDTH;
	}
	else
	{
		CG_DrawSmallString(tempx + 1 * SMALLCHAR_WIDTH, y, CG_TranslateString("XP"), fade);
		tempx += INFO_XP_WIDTH;
	}

	CG_DrawSmallString(tempx, y, CG_TranslateString("Ping"), fade);
	tempx += INFO_LATENCY_WIDTH;

	if (cgs.gametype != GT_WOLF_LMS)
	{
		CG_DrawPicST(tempx + 2, y, INFO_LIVES_WIDTH - 4, 16, 0.f, 0.f, 0.5f, 1.f, team == TEAM_ALLIES ? cgs.media.hudAlliedHelmet : cgs.media.hudAxisHelmet);
		tempx += INFO_LIVES_WIDTH;
	}

	y += SMALLCHAR_HEIGHT + 2;

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
			VectorSet(hcolor, (80.f / 255.f), (80.f / 255.f), (80.f / 255.f));  // LIGHT BLUE
		}
		else
		{
			VectorSet(hcolor, (0.f / 255.f), (0.f / 255.f), (0.f / 255.f));     // DARK BLUE
		}
		hcolor[3] = fade * 0.3;

		if (use_mini_chars)
		{
			CG_FillRect(x - 5, y, width + 5, MINICHAR_HEIGHT, hcolor);
			trap_R_SetColor(colorBlack);
			CG_DrawBottom_NoScale(x - 5, y, width + 5, MINICHAR_HEIGHT, 1);
			trap_R_SetColor(NULL);
			y += MINICHAR_HEIGHT;

		}
		else
		{
			CG_FillRect(x - 5, y, width + 5, SMALLCHAR_HEIGHT, hcolor);
			trap_R_SetColor(colorBlack);
			CG_DrawBottom_NoScale(x - 5, y, width + 5, SMALLCHAR_HEIGHT, 1);
			trap_R_SetColor(NULL);
			y += SMALLCHAR_HEIGHT;
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
			WM_DrawClientScore_Small(x, y, &cg.scores[i], hcolor, fade);
			y += MINICHAR_HEIGHT;
		}
		else
		{
			WM_DrawClientScore(x, y, &cg.scores[i], hcolor, fade);
			y += SMALLCHAR_HEIGHT;
		}

		count++;
	}

	// draw spectators
	if (use_mini_chars)
	{
		y += MINICHAR_HEIGHT;
	}
	else
	{
		y += SMALLCHAR_HEIGHT;
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
			WM_DrawClientScore_Small(x, y, &cg.scores[i], hcolor, fade);
			y += MINICHAR_HEIGHT;
		}
		else
		{
			WM_DrawClientScore(x, y, &cg.scores[i], hcolor, fade);
			y += SMALLCHAR_HEIGHT;
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

	// don't draw if in cameramode
	if (cg.cameraMode)
	{
		return qtrue;
	}

	if (cg.showScores || cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		fade = 1.0;
	}
	else
	{
		float *fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if (!fadeColor)
		{
			// next time scoreboard comes up, don't print killer
			*cg.killerName = 0;
			return qfalse;
		}
		fade = fadeColor[3];
	}

	y = WM_DrawObjectives(x, y, width, fade);

	if (cgs.gametype == GT_WOLF_STOPWATCH && (cg.snap->ps.pm_type == PM_INTERMISSION))
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
			WM_TeamScoreboard(x, y, TEAM_AXIS, fade, 25, 33);
			x = x_right;
			WM_TeamScoreboard(x, y, TEAM_ALLIES, fade, 25, 33);
		}
	}

	return qtrue;
}
