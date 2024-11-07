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
 * @file cg_consolecmds.c
 * @brief Text commands typed in at the local console, or executed by a key
 *        binding
 */

#include "cg_local.h"

char *Binding_FromName(const char *cvar);

/**
 * @brief Concat multiple args from range into a single string
 * @param offset start offset
 * @param count how many token to add
 * @param buffer output buffer
 * @param bufferLength output buffer size
 */
static void CG_Args(int offset, int count, char *buffer, int bufferLength)
{
	static char tmp[MAX_TOKEN_CHARS];
	int         i, total = trap_Argc();

	buffer[0] = '\0';

	if (count > 0 && total > offset + count)
	{
		total = offset + count;
	}

	for (i = offset; i < total; i++)
	{
		tmp[0] = '\0';
		trap_Argv(i, tmp, MAX_TOKEN_CHARS);
		Q_strcat(buffer, bufferLength, tmp);
		if (i != total - 1)
		{
			Q_strcat(buffer, bufferLength, " ");
		}
	}
}

static ID_INLINE const char * CG_ArgsStat(int offset, int count)
{
	static char buffer[MAX_TOKEN_CHARS];
	CG_Args(offset, count, buffer, sizeof(buffer));
	return buffer;
}

/**
 * @brief Debugging command to print the current position
 */
static void CG_Viewpos_f(void)
{
	CG_Printf("(%i %i %i) : %.0f %.0f %.0f\n", (int) cg.refdef.vieworg[0],
	          (int) cg.refdef.vieworg[1], (int) cg.refdef.vieworg[2],
	          round(cg.refdefViewAngles[PITCH]), round(cg.refdefViewAngles[YAW]), round(cg.refdefViewAngles[ROLL]));
}

/**
 * @brief CG_LimboMenu_f
 */
void CG_LimboMenu_f(void)
{
	// disallow opening the limbo menu on demoplayback
	if (cg.demoPlayback)
	{
		return;
	}

	if (cg.showGameView)
	{
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}
	else
	{
		CG_EventHandling(CGAME_EVENT_GAMEVIEW, qfalse);
	}
}

/**
 * @brief CG_StatsDown_f
 */
static void CG_StatsDown_f(void)
{
	if (!cg.demoPlayback)
	{
		if (
#ifdef FEATURE_MULTIVIEW
			cg.mvTotalClients < 1 &&
#endif
			cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		{
			Pri("You must be a player or following a player to use +stats\n");
			return;
		}

		if (cgs.gamestats.show == SHOW_SHUTDOWN && cg.time < cgs.gamestats.fadeTime)
		{
			cgs.gamestats.fadeTime = 2 * cg.time + STATS_FADE_TIME - cgs.gamestats.fadeTime;
		}
		else if (cgs.gamestats.show != SHOW_ON)
		{
			cgs.gamestats.fadeTime = cg.time + STATS_FADE_TIME;
		}

		cgs.gamestats.show = SHOW_ON;

		if (cgs.gamestats.requestTime < cg.time)
		{
			int i =
#ifdef FEATURE_MULTIVIEW
				(cg.mvTotalClients > 0) ? (cg.mvCurrentActive->mvInfo & MV_PID) :
					#endif
				cg.snap->ps.clientNum;

			cgs.gamestats.requestTime = cg.time + 2000;
			trap_SendClientCommand(va("sgstats %d", i));
		}
	}
}

/**
 * @brief CG_StatsUp_f
 */
static void CG_StatsUp_f(void)
{
	if (cgs.gamestats.show == SHOW_ON)
	{
		cgs.gamestats.show = SHOW_SHUTDOWN;
		if (cg.time < cgs.gamestats.fadeTime)
		{
			cgs.gamestats.fadeTime = 2 * cg.time + STATS_FADE_TIME - cgs.gamestats.fadeTime;
		}
		else
		{
			cgs.gamestats.fadeTime = cg.time + STATS_FADE_TIME;
		}
	}
}

/**
 * @brief CG_topshotsDown_f
 */
static void CG_topshotsDown_f(void)
{
	if (!cg.demoPlayback)
	{
		if (cgs.topshots.show == SHOW_SHUTDOWN && cg.time < cgs.topshots.fadeTime)
		{
			cgs.topshots.fadeTime = 2 * cg.time + STATS_FADE_TIME - cgs.topshots.fadeTime;
		}
		else if (cgs.topshots.show != SHOW_ON)
		{
			cgs.topshots.fadeTime = cg.time + STATS_FADE_TIME;
		}

		cgs.topshots.show = SHOW_ON;

		if (cgs.topshots.requestTime < cg.time)
		{
			cgs.topshots.requestTime = cg.time + 2000;
			trap_SendClientCommand("stshots");
		}
	}
}

/**
 * @brief CG_topshotsUp_f
 */
static void CG_topshotsUp_f(void)
{
	if (cgs.topshots.show == SHOW_ON)
	{
		cgs.topshots.show = SHOW_SHUTDOWN;
		if (cg.time < cgs.topshots.fadeTime)
		{
			cgs.topshots.fadeTime = 2 * cg.time + STATS_FADE_TIME - cgs.topshots.fadeTime;
		}
		else
		{
			cgs.topshots.fadeTime = cg.time + STATS_FADE_TIME;
		}
	}
}

/**
 * @brief CG_objectivesDown_f
 */
static void CG_objectivesDown_f(void)
{
	if (!cg.demoPlayback)
	{
		if (cgs.objectives.show == SHOW_SHUTDOWN && cg.time < cgs.objectives.fadeTime)
		{
			cgs.objectives.fadeTime = 2 * cg.time + STATS_FADE_TIME - cgs.objectives.fadeTime;
		}
		else if (cgs.objectives.show != SHOW_ON)
		{
			cgs.objectives.fadeTime = cg.time + STATS_FADE_TIME;
		}
		cgs.objectives.show = SHOW_ON;
	}
}

/**
 * @brief CG_objectivesUp_f
 */
void CG_objectivesUp_f(void)
{
	if (cgs.objectives.show == SHOW_ON)
	{
		cgs.objectives.show = SHOW_SHUTDOWN;
		if (cg.time < cgs.objectives.fadeTime)
		{
			cgs.objectives.fadeTime = 2 * cg.time + STATS_FADE_TIME - cgs.objectives.fadeTime;
		}
		else
		{
			cgs.objectives.fadeTime = cg.time + STATS_FADE_TIME;
		}
	}
}

/**
 * @brief CG_ScoresDown_f
 */
void CG_ScoresDown_f(void)
{
#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
	if (
#if defined(FEATURE_RATING)
		cgs.skillRating
#endif
#if defined(FEATURE_RATING) && defined(FEATURE_PRESTIGE)
		||
#endif
#if defined(FEATURE_PRESTIGE)
		cgs.prestige
#endif
		)
	{
		if (!cg.showScores && cg.scoresDownTime + 250 > cg.time && cg.scoreToggleTime < (cg.time - 500))
		{
			int sb = cg_scoreboard.integer + 1;

#ifdef FEATURE_RATING
			if (cgs.skillRating && sb == SCOREBOARD_SR && (cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS))
			{
				sb += 1;
			}
#else
			if (sb == SCOREBOARD_SR)
			{
				sb += 1;
			}
#endif

#ifdef FEATURE_PRESTIGE
			if (cgs.prestige && sb == SCOREBOARD_PR && (cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS || cgs.gametype == GT_WOLF_CAMPAIGN))
			{
				sb += 1;
			}
#else
			if (sb == SCOREBOARD_PR)
			{
				sb += 1;
			}
#endif
			// cycle scoreboard type with a quick tap of +scores
			if (sb < SCOREBOARD_XP || sb > SCOREBOARD_PR)
			{
				sb = SCOREBOARD_XP;
			}

			trap_Cvar_Set("cg_scoreboard", va("%i", sb));

			cg.scoreToggleTime = cg.time;
		}
		cg.scoresDownTime = cg.time;
	}
	else
#endif
	{
		trap_Cvar_Set("cg_scoreboard", "0"); // SCOREBOARD_XP
	}

	if (cg.scoresRequestTime + 2000 < cg.time)
	{
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;

		// we get periodic score updates if we are merging clients
		if (!cg.demoPlayback
			#ifdef FEATURE_MULTIVIEW
		    && cg.mvTotalClients < 1
#endif
		    )
		{
			trap_SendClientCommand("score");
		}

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if (!cg.showScores)
		{
			cg.showScores = qtrue;
			if (!cg.demoPlayback
				#ifdef FEATURE_MULTIVIEW
			    && cg.mvTotalClients < 1
#endif
			    )
			{
				cg.numScores = 0;
			}
		}
	}
	else
	{
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

/**
 * @brief CG_ScoresUp_f
 */
void CG_ScoresUp_f(void)
{
	if (cg.showScores)
	{
		cg.showScores    = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

/**
 * @brief CG_Fade_f
 */
static void CG_Fade_f(void)
{
	int r, g, b, a, duration;

	if (trap_Argc() < 6)
	{
		return;
	}

	r = Q_atoi(CG_Argv(1));
	g = Q_atoi(CG_Argv(2));
	b = Q_atoi(CG_Argv(3));
	a = Q_atoi(CG_Argv(4));

	duration = (int) (Q_atof(CG_Argv(5)) * 1000);

	CG_Fade(r, g, b, a, cg.time, duration);
}

/**
 * @brief CG_QuickMessage_f
 */
void CG_QuickMessage_f(void)
{
	CG_EventHandling(CGAME_EVENT_NONE, qfalse);

	if (cg_quickMessageAlt.integer)
	{
		trap_UI_Popup(UIMENU_WM_QUICKMESSAGEALT);
	}
	else
	{
		trap_UI_Popup(UIMENU_WM_QUICKMESSAGE);
	}
}

/**
 * @brief CG_QuickFireteamMessage_f
 */
void CG_QuickFireteamMessage_f(void)
{
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
	{
		return;
	}

	CG_EventHandling(CGAME_EVENT_NONE, qfalse);

	if (cg_quickMessageAlt.integer)
	{
		trap_UI_Popup(UIMENU_WM_FTQUICKMESSAGEALT);
	}
	else
	{
		trap_UI_Popup(UIMENU_WM_FTQUICKMESSAGE);
	}
}

/**
 * @brief CG_QuickFireteamAdmin_f
 */
void CG_QuickFireteamAdmin_f(void)
{
	trap_UI_Popup(UIMENU_NONE);

	if (cg.showFireteamMenu)
	{
		if (cgs.ftMenuMode == 1)
		{
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		else
		{
			cgs.ftMenuMode = 1;
		}
	}
	else if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		CG_EventHandling(CGAME_EVENT_FIRETEAMMSG, qfalse);
		cgs.ftMenuMode = 1;
	}
}

/**
 * @brief CG_QuickFireteams_f
 */
static void CG_QuickFireteams_f(void)
{
	if (!CG_IsOnFireteam(cg.clientNum))
	{
		return;
	}

	if (cg.showFireteamMenu)
	{
		if (cgs.ftMenuMode == 0)
		{
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		else
		{
			cgs.ftMenuMode = 0;
			CG_Printf("2\n");
		}
	}

	CG_EventHandling(CGAME_EVENT_FIRETEAMMSG, qfalse);
	cgs.ftMenuMode = 0;
}

/**
 * @brief CG_FTSayPlayerClass_f
 */
static void CG_FTSayPlayerClass_f(void)
{
	int        playerType = cgs.clientinfo[cg.clientNum].cls;
	const char *s;

	if (playerType == PC_MEDIC)
	{
		s = "IamMedic";
	}
	else if (playerType == PC_ENGINEER)
	{
		s = "IamEngineer";
	}
	else if (playerType == PC_FIELDOPS)
	{
		s = "IamFieldOps";
	}
	else if (playerType == PC_COVERTOPS)
	{
		s = "IamCovertOps";
	}
	else
	{
		s = "IamSoldier";
	}

	if (cg.snap && (cg.snap->ps.pm_type != PM_INTERMISSION))
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
		{
			CG_Printf("%s", CG_TranslateString("Can't team voice chat as a spectator.\n"));
			return;
		}
	}

	trap_SendConsoleCommand(va("cmd vsay_buddy -1 %s %s\n", CG_BuildSelectedFirteamString(), s));
}

/**
 * @brief CG_SayPlayerClass_f
 */
static void CG_SayPlayerClass_f(void)
{
	int        playerType = cgs.clientinfo[cg.clientNum].cls;
	const char *s;

	if (playerType == PC_MEDIC)
	{
		s = "IamMedic";
	}
	else if (playerType == PC_ENGINEER)
	{
		s = "IamEngineer";
	}
	else if (playerType == PC_FIELDOPS)
	{
		s = "IamFieldOps";
	}
	else if (playerType == PC_COVERTOPS)
	{
		s = "IamCovertOps";
	}
	else
	{
		s = "IamSoldier";
	}

	if (cg.snap && (cg.snap->ps.pm_type != PM_INTERMISSION))
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
		{
			CG_Printf("%s", CG_TranslateString("Can't team voice chat as a spectator.\n"));
			return;
		}
	}

	trap_SendConsoleCommand(va("cmd vsay_team %s\n", s));
}

/**
 * @brief CG_VoiceChat_f
 */
static void CG_VoiceChat_f(void)
{
	char chatCmd[64];

	if (trap_Argc() != 2)
	{
		return;
	}

	trap_Argv(1, chatCmd, 64);
	trap_SendConsoleCommand(va("cmd vsay %s\n", chatCmd));
}

/**
 * @brief CG_TeamVoiceChat_f
 */
static void CG_TeamVoiceChat_f(void)
{
	char chatCmd[64];

	if (trap_Argc() != 2)
	{
		return;
	}

	// don't let spectators voice chat
	// NOTE - This cg.snap will be the person you are following, but its just for intermission test
	if (cg.snap && (cg.snap->ps.pm_type != PM_INTERMISSION))
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
		{
			CG_Printf("%s", CG_TranslateString("Can't team voice chat as a spectator.\n")); // FIXME? find a way to print this on screen
			return;
		}
	}

	trap_Argv(1, chatCmd, 64);

	trap_SendConsoleCommand(va("cmd vsay_team %s\n", chatCmd));
}

/**
 * @brief CG_BuddyVoiceChat_f
 */
static void CG_BuddyVoiceChat_f(void)
{
	char chatCmd[64];

	if (trap_Argc() != 2)
	{
		return;
	}

	// don't let spectators voice chat
	// NOTE - This cg.snap will be the person you are following, but its just for intermission test
	if (cg.snap && (cg.snap->ps.pm_type != PM_INTERMISSION))
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
		{
			CG_Printf("%s", CG_TranslateString("Can't buddy voice chat as a spectator.\n"));
			return;
		}
	}

	trap_Argv(1, chatCmd, 64);

	trap_SendConsoleCommand(va("cmd vsay_buddy -1 %s %s\n", CG_BuildSelectedFirteamString(), chatCmd));
}

/**
 * @brief CG_MessageMode_f
 * @details say, team say, etc
 */
static void CG_MessageMode_f(void)
{
	char cmd[64];

	if (cgs.eventHandling != CGAME_EVENT_NONE && cgs.eventHandling != CGAME_EVENT_SHOUTCAST)
	{
		return;
	}

	// get the actual command
	trap_Argv(0, cmd, 64);

	// team say
	if (!Q_stricmp(cmd, "messagemode2"))
	{
		trap_Cvar_Set("cg_messageType", "2");
	}
	// fireteam say
	else if (!Q_stricmp(cmd, "messagemode3") && cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		if (CG_IsOnFireteam(cg.clientNum))
		{
			trap_Cvar_Set("cg_messageType", "3");
		}
		else
		{
			// fallback to team say
			trap_Cvar_Set("cg_messageType", "2");
		}
	}
	// (normal) say
	else
	{
		trap_Cvar_Set("cg_messageType", "1");
	}

	// clear the chat text
	trap_Cvar_Set("cg_messageText", "");

	// open the menu
	trap_UI_Popup(UIMENU_INGAME_MESSAGEMODE);
}

/**
 * @brief CG_MessageSend_f
 */
static void CG_MessageSend_f(void)
{
	char messageText[MAX_SAY_TEXT];
	int  messageType;

	// get values
	trap_Cvar_VariableStringBuffer("cg_messageType", messageText, MAX_SAY_TEXT);
	messageType = Q_atoi(messageText);
	trap_Cvar_VariableStringBuffer("cg_messageText", messageText, MAX_SAY_TEXT);

	// reset values
	trap_Cvar_Set("cg_messageText", "");
	trap_Cvar_Set("cg_messageType", "");

	// don't send empty messages
	if (messageText[0] == '\0')
	{
		return;
	}

	Q_EscapeUnicodeInPlace(messageText, MAX_SAY_TEXT);

	if (messageType == 2) // team say
	{
		trap_SendConsoleCommand(va("say_team \"%s\"\n", messageText));
	}
	else if (messageType == 3)   // fireteam say
	{
		trap_SendConsoleCommand(va("say_buddy \"%s\"\n", messageText));
	}
	else   // normal say
	{
		trap_SendConsoleCommand(va("say \"%s\"\n", messageText));
	}
}

/**
 * @brief CG_SetWeaponCrosshair_f
 */
static void CG_SetWeaponCrosshair_f(void)
{
	char crosshair[64];

	trap_Argv(1, crosshair, 64);
	cg.newCrosshairIndex = Q_atoi(crosshair) + 1;
}

/**
 * @brief CG_SelectBuddy_f
 */
static void CG_SelectBuddy_f(void)
{
	int          pos = Q_atoi(CG_Argv(1));
	int          i;
	clientInfo_t *ci;

	// 0 - 7 = select that person
	// -1 = none
	// -2 = all
	switch (pos)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		if (!CG_IsOnFireteam(cg.clientNum))
		{
			break;         // we aren't a leader, so dont allow selection
		}

		ci = CG_SortedFireTeamPlayerForPosition(pos);
		if (!ci)
		{
			break;         // there was no-one in this position
		}

		ci->selected ^= qtrue;
		break;

	case -1:
		if (!CG_IsOnFireteam(cg.clientNum))
		{
			break;         // we aren't a leader, so dont allow selection
		}

		for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
		{
			ci = CG_SortedFireTeamPlayerForPosition(i);
			if (!ci)
			{
				break;         // there was no-one in this position
			}

			ci->selected = qfalse;
		}
		break;

	case -2:
		if (!CG_IsOnFireteam(cg.clientNum))
		{
			break;         // we aren't a leader, so dont allow selection
		}

		for (i = 0; i < MAX_FIRETEAM_MEMBERS; i++)
		{
			ci = CG_SortedFireTeamPlayerForPosition(i);
			if (!ci)
			{
				break;         // there was no-one in this position
			}

			ci->selected = qtrue;
		}
		break;
	}
}

/**
 * @brief CG_QuickSpawnpoints_f
 */
static void CG_QuickSpawnpoint_f(void)
{
	if (cg.demoPlayback)
	{
		return;
	}

	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
	{
		return;
	}

	if (cg.showSpawnpointsMenu)
	{
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}

	CG_EventHandling(CGAME_EVENT_SPAWNPOINTMSG, qfalse);
}

extern void CG_AdjustAutomapZoom(int zoomIn);

/**
 * @brief CG_AutomapZoomIn_f
 */
static void CG_AutomapZoomIn_f(void)
{
	if (!cgs.autoMapOff)
	{
		CG_AdjustAutomapZoom(qtrue);
	}
}

/**
 * @brief CG_AutomapZoomOut_f
 */
static void CG_AutomapZoomOut_f(void)
{
	if (!cgs.autoMapOff)
	{
		CG_AdjustAutomapZoom(qfalse);
	}
}

/**
 * @brief CG_AutomapExpandDown_f
 */
static void CG_AutomapExpandDown_f(void)
{
	if (!cgs.autoMapExpanded)
	{
		cgs.autoMapExpanded = qtrue;
		if (cg.time - cgs.autoMapExpandTime < cg_commandMapTime.integer)
		{
			cgs.autoMapExpandTime = cg.time - (cg_commandMapTime.integer - (cg.time - cgs.autoMapExpandTime));
		}
		else
		{
			cgs.autoMapExpandTime = cg.time;
		}
	}
}

/**
 * @brief CG_AutomapExpandUp_f
 */
static void CG_AutomapExpandUp_f(void)
{
	if (cgs.autoMapExpanded)
	{
		cgs.autoMapExpanded = qfalse;
		if (cg.time - cgs.autoMapExpandTime < cg_commandMapTime.integer)
		{
			cgs.autoMapExpandTime = cg.time - (cg_commandMapTime.integer - (cg.time - cgs.autoMapExpandTime));
		}
		else
		{
			cgs.autoMapExpandTime = cg.time;
		}
	}
}

/**
 * @brief CG_ToggleAutomap_f
 */
static void CG_ToggleAutomap_f(void)
{
	cgs.autoMapOff = (qboolean) !cgs.autoMapOff;
}

const char *aMonths[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/**
 * @brief CG_currentTime_f
 */
static void CG_currentTime_f(void)
{
	qtime_t ct;

	trap_RealTime(&ct);
	CG_Printf("[cgnotify]Current time: ^3%02d:%02d:%02d (%02d %s %d)\n", ct.tm_hour, ct.tm_min, ct.tm_sec, ct.tm_mday, aMonths[ct.tm_mon], 1900 + ct.tm_year);
}

/**
 * @brief Dynamically names a demo and sets up the recording
 */
void CG_autoRecord_f(void)
{
	trap_SendConsoleCommand(va("record %s\n", CG_generateFilename()));
}

/**
 * @brief Starts or stops demo recording
 */
void CG_toggleRecord_f(void)
{
	char bindstr[32];
	Q_strncpyz(bindstr, Binding_FromName("togglerecord"), sizeof(bindstr));

	if (!cl_demorecording.integer)
	{
		trap_SendConsoleCommand(va("record %s\n", CG_generateFilename()));
		CG_Printf("Press ^3%s ^7again to stop recording.\n", bindstr);
	}
	else
	{
		trap_SendConsoleCommand("stoprecord\n");
	}
}

/**
 * @brief Dynamically names a screenshot
 */
void CG_autoScreenShot_f(void)
{
	trap_SendConsoleCommand(va("screenshot %s\n", CG_generateFilename()));
}

/**
 * @brief CG_vstrDown_f
 */
static void CG_vstrDown_f(void)
{
	// The engine also passes back the key code and time of the key press
	if (trap_Argc() == 5)
	{
		trap_SendConsoleCommand(va("vstr %s;", CG_Argv(1)));
	}
	else
	{
		CG_Printf("[cgnotify]Usage: +vstr [down_vstr] [up_vstr]\n");
	}
}

/**
 * @brief CG_vstrUp_f
 */
static void CG_vstrUp_f(void)
{
	// The engine also passes back the key code and time of the key press
	if (trap_Argc() == 5)
	{
		trap_SendConsoleCommand(va("vstr %s;", CG_Argv(2)));
	}
	else
	{
		CG_Printf("[cgnotify]Usage: +vstr [down_vstr] [up_vstr]\n");
	}
}

/**
 * @brief CG_keyOn_f
 */
void CG_keyOn_f(void)
{
	if (!cg.demoPlayback)
	{
		CG_Printf("[cgnotify]^3*** NOT PLAYING A DEMO!!\n");
		return;
	}

	if (demo_infoWindow.integer > 0)
	{
		CG_ShowHelp_On(&cg.demohelpWindow);
	}

	CG_EventHandling(CGAME_EVENT_DEMO, qtrue);
}

/**
 * @brief CG_keyOff_f
 */
void CG_keyOff_f(void)
{
	if (!cg.demoPlayback)
	{
		return;
	}
	CG_EventHandling(CGAME_EVENT_NONE, qfalse);
}

/**
 * @brief CG_dumpStats_f
 */
void CG_dumpStats_f(void)
{
	if (cgs.dumpStatsTime < cg.time)
	{
		cgs.dumpStatsTime = cg.time + 2000;
		trap_SendClientCommand(
#ifdef FEATURE_MULTIVIEW
			(cg.mvTotalClients < 1) ?
				#endif
			"weaponstats"
				#ifdef FEATURE_MULTIVIEW
			                            : "statsall"
#endif
			);
	}
}

/*
 * @brief CG_wStatsDown_f
 * @note Unused
void CG_wStatsDown_f(void)
{
    if (
#ifdef FEATURE_MULTIVIEW
        cg.mvTotalClients < 1 &&
#endif
        cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
    {
        Pri("You must be a player or following a player to use +wstats\n");
        return;
    }

    if (cg.statsRequestTime < cg.time)
    {
        int i =
#ifdef FEATURE_MULTIVIEW
            (cg.mvTotalClients > 0) ? (cg.mvCurrentActive->mvInfo & MV_PID) :
#endif
            cg.snap->ps.clientNum;

        cg.statsRequestTime = cg.time + 500;
        trap_SendClientCommand(va("wstats %d", i));
    }

    cg.showStats = qtrue;
}
*/

/*
 * @brief CG_wStatsUp_f
 * @note unused
void CG_wStatsUp_f(void)
{
    cg.showStats = qfalse;
    CG_windowFree(cg.statsWindow);
    cg.statsWindow = NULL;
}
*/

#ifdef FEATURE_MULTIVIEW

/**
 * @brief CG_toggleSpecHelp_f
 */
void CG_toggleSpecHelp_f(void)
{
	if (cg.mvTotalClients > 0 && !cg.demoPlayback)
	{
		if (cg.spechelpWindow != SHOW_ON && cg_specHelp.integer > 0)
		{
			CG_ShowHelp_On(&cg.spechelpWindow);
		}
		else if (cg.spechelpWindow == SHOW_ON)
		{
			CG_ShowHelp_Off(&cg.spechelpWindow);
		}
	}
}

#endif

/**
 * @brief CG_EditSpeakers_f
 */
static void CG_EditSpeakers_f(void)
{
	if (cg.editingSpeakers)
	{
		CG_DeActivateEditSoundMode();
	}
	else
	{
		if (!cgs.sv_cheats)
		{
			CG_Printf("editSpeakers is cheat protected.\n");
			return;
		}
		CG_ActivateEditSoundMode();
	}
}

/**
 * @brief CG_DumpSpeaker_f
 */
static void CG_DumpSpeaker_f(void)
{
	/*  char sscrfilename[MAX_QPATH];
	    char soundfile[MAX_STRING_CHARS];
	    int i, wait, random;
	    char *extptr, *buffptr;
	    fileHandle_t f;

	        // Check for argument
	    if( trap_Argc() < 2 || trap_Argc() > 4 )
	    {
	        CG_Printf( "Usage: dumpspeaker <soundfile> ( <wait=value>|<random=value> )\n" );
	        return;
	    }

	    wait = random = 0;

	    // parse the other parameters
	    for( i = 2; i < trap_Argc(); i++ ) {
	        char *valueptr = NULL;

	        trap_Argv( i, soundfile, sizeof(soundfile) );

	        for( buffptr = soundfile; *buffptr; buffptr++ ) {
	            if( *buffptr == '=' ) {
	                valueptr = buffptr + 1;
	                break;
	            }
	        }

	        Q_strncpyz( soundfile, soundfile, buffptr - soundfile + 1 );

	        if( !Q_stricmp( soundfile, "wait" ) )
	            wait = Q_atoi( valueptr );
	        else if( !Q_stricmp( soundfile, "random" ) )
	            random = Q_atoi( valueptr );
	    }

	    // parse soundfile
	    trap_Argv( 1, soundfile, sizeof(soundfile) );

	    // Open soundfile
	    Q_strncpyz( sscrfilename, cgs.mapname, sizeof(sscrfilename) );
	    extptr = sscrfilename + strlen( sscrfilename ) - 4;
	    if( extptr < sscrfilename || Q_stricmp( extptr, ".bsp" ) )
	    {
	        CG_Printf( "Unable to dump, unknown map name?\n" );
	        return;
	    }
	    Q_strncpyz( extptr, ".sscr", 5 );
	    trap_FS_FOpenFile( sscrfilename, &f, FS_APPEND_SYNC );
	    if( !f )
	    {
	        CG_Printf( "Failed to open '%s' for writing.\n", sscrfilename );
	        return;
	    }

	        // Build the entity definition
	    Com_sprintf( soundfile, sizeof(soundfile), "{\n\"classname\" \"target_speaker\"\n\"origin\" \"%i %i %i\"\n\"noise\" \"%s\"\n",
	                                                (int) cg.snap->ps.origin[0], (int) cg.snap->ps.origin[1], (int) cg.snap->ps.origin[2], soundfile );

	    if( wait ) {
	        Q_strcat( soundfile, sizeof(soundfile), va( "\"wait\" \"%i\"\n", wait ) );
	    }
	    if( random ) {
	        Q_strcat( soundfile, sizeof(soundfile), va( "\"random\" \"%i\"\n", random ) );
	    }
	    Q_strcat( soundfile, sizeof(soundfile), "}\n\n" );

	        // And write out/acknowledge
	    trap_FS_Write( soundfile, strlen( soundfile ), f );
	    trap_FS_FCloseFile( f );
	    CG_Printf( "Entity dumped to '%s' (%i %i %i).\n", sscrfilename,
	               (int) cg.snap->ps.origin[0], (int) cg.snap->ps.origin[1], (int) cg.snap->ps.origin[2] );*/
	bg_speaker_t speaker;
	trace_t      tr;
	vec3_t       end;

	if (!cg.editingSpeakers)
	{
		CG_Printf("Speaker Edit mode needs to be activated to dump speakers\n");
		return;
	}

	Com_Memset(&speaker, 0, sizeof(speaker));

	speaker.volume = 127;
	speaker.range  = 1250;

	VectorMA(cg.refdef_current->vieworg, 32, cg.refdef_current->viewaxis[0], end);
	CG_Trace(&tr, cg.refdef_current->vieworg, NULL, NULL, end, -1, MASK_SOLID);

	if (tr.fraction < 1.f)
	{
		VectorCopy(tr.endpos, speaker.origin);
		VectorMA(speaker.origin, -4, cg.refdef_current->viewaxis[0], speaker.origin);
	}
	else
	{
		VectorCopy(tr.endpos, speaker.origin);
	}

	if (!BG_SS_StoreSpeaker(&speaker))
	{
		CG_Printf(S_COLOR_RED "ERROR: Failed to store speaker\n");
	}
}

/**
 * @brief CG_ModifySpeaker_f
 */
static void CG_ModifySpeaker_f(void)
{
	if (cg.editingSpeakers)
	{
		CG_ModifyEditSpeaker();
	}
}

/**
 * @brief CG_UndoSpeaker_f
 */
static void CG_UndoSpeaker_f(void)
{
	if (cg.editingSpeakers)
	{
		CG_UndoEditSpeaker();
	}
}

/**
 * @brief CG_ForceTapOut_f
 */
void CG_ForceTapOut_f(void)
{
	trap_SendClientCommand("forcetapout");
}

/**
 * @brief Shows a popup message.
 */
static void CG_CPM_f(void)
{
	int        iconnumber;
	const char *iconstring;

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

	// this is custom, don't localize!
	CG_AddPMItem(PM_MESSAGE, CG_Argv(1), " ", cgs.media.pmImages[iconnumber], 0, 0, colorWhite);
}

qboolean resetmaxspeed = qfalse;

static void CG_ResetMaxSpeed_f(void)
{
	resetmaxspeed = qtrue;
}

/**
 * @brief ETPro style enemy spawntimer
 */
static void CG_TimerSet_f(void)
{
	if (cgs.gamestate != GS_PLAYING)
	{
		CG_Printf("You may only use this command during the match.\n");
		return;
	}

	if (trap_Argc() == 1)
	{
		trap_Cvar_Set("cg_spawnTimer_set", "-1");
	}
	else if (trap_Argc() == 2)
	{
		char buff[32] = { "" };
		int  spawnPeriod;

		trap_Argv(1, buff, sizeof(buff));
		spawnPeriod = Q_atoi(buff);

		if (spawnPeriod == 0)
		{
			trap_Cvar_Set("cg_spawnTimer_period", 0);
		}
		else if (spawnPeriod < 1 || spawnPeriod > 60)
		{
			CG_Printf("Argument must be a number between 1 and 60 - no argument will disable the spawn timer.\n");
		}
		else
		{
			trap_Cvar_Set("cg_spawnTimer_period", buff);
			trap_Cvar_Set("cg_spawnTimer_set", va("%i", (cg.time - cgs.levelStartTime)));
		}
	}
	else
	{
		CG_Printf("Usage: timerSet [seconds]\n");
	}
}

/**
 * @brief ETPro style timer resetting
 */
static void CG_TimerReset_f(void)
{
	if (cgs.gamestate != GS_PLAYING)
	{
		CG_Printf("You may only use this command during the match.\n");
		return;
	}

	trap_Cvar_Set("cg_spawnTimer_period", 0);
	trap_Cvar_Set("cg_spawnTimer_set", va("%d", cg.time - cgs.levelStartTime));
}

/**
* @brief CG_IsClassFull
* @param[in] ent
* @param[in] classIndex
* @param[in] selectedTeam
* @return
*/
qboolean CG_IsClassFull(int playerType, team_t team)
{
	int classCount, playerCount;

	if (playerType < PC_SOLDIER || playerType > PC_COVERTOPS || team == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	classCount  = CG_LimboPanel_ClassCount(team, playerType);
	playerCount = CG_LimboPanel_TeamCount(-1);

	if (classCount >= CG_LimboPanel_MaxCount(playerCount, cg.maxPlayerClasses[playerType]))
	{
		CG_PriorityCenterPrint(CG_TranslateString(va("^1%s^7 is not available! Choose another class!", BG_ClassnameForNumber(playerType))), -1);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Checks for heavy and rifle weapons
 * @param[in] weapon
 * @return
 * @note FIXME: this function is based on G_IsWeaponDisabled which needs some rework.
 */
qboolean CG_IsWeaponDisabled(weapon_t weapon)
{
	int        count, wcount;
	const char *maxCount;

	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return qtrue;
	}

	// never restrict normal weapons
	if (!(GetWeaponTableData(weapon)->skillBased == SK_HEAVY_WEAPONS || (GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->type & WEAPON_TYPE_RIFLENADE)))
	{
		return qfalse;
	}

	count  = CG_LimboPanel_TeamCount(-1);
	wcount = CG_LimboPanel_TeamCount(weapon);

	// heavy weapon restriction
	if (GetWeaponTableData(weapon)->skillBased == SK_HEAVY_WEAPONS)
	{
		if (wcount >= ceil(count * cgs.weaponRestrictions))
		{
			return qtrue;
		}
	}

	if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_PANZER)
	{
		maxCount = cg.maxRockets;
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MORTAR)
	{
		maxCount = cg.maxMortars;
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MG)
	{
		maxCount = cg.maxMachineguns;
	}
	else if (GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->type & WEAPON_TYPE_RIFLENADE)
	{
		maxCount = cg.maxRiflegrenades;
	}
	else if (weapon == WP_FLAMETHROWER)
	{
		maxCount = cg.maxFlamers;
	}
	else
	{
		return qfalse;
	}

	if (GetWeaponTableData(weapon)->weapAlts)
	{
		// add alt weapons
		wcount += CG_LimboPanel_TeamCount(GetWeaponTableData(weapon)->weapAlts);
	}

	if (wcount >= CG_LimboPanel_MaxCount(count, maxCount))
	{
		CG_PriorityCenterPrint(va(CG_TranslateString("^1%s^7 is not available! Choose another weapon!"), GetWeaponTableData(weapon)->desc), -1);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief class change menu
 */
static void CG_ClassMenu_f(void)
{
	if (cg.demoPlayback)
	{
		return;
	}

	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	CG_EventHandling(CGAME_EVENT_NONE, qfalse);

	if (cg_quickMessageAlt.integer)
	{
		trap_UI_Popup(UIMENU_WM_CLASSALT);
	}
	else
	{
		trap_UI_Popup(UIMENU_WM_CLASS);
	}
}

/**
 * @brief Sends a class setup message. Enables etpro like class scripts
 */
static void CG_Class_f(void)
{
	char             cls[64];
	const char       *classtype, *teamstring;
	int              weapon1, weapon2, playerclass;
	bg_playerclass_t *classinfo;
	team_t           team;

	if (cg.demoPlayback)
	{
		return;
	}

	if (trap_Argc() < 2)
	{
		CG_Printf("Usage: class <s|m|e|f|c> <weapon1> <weapon2>\n");
		return;
	}

	team = cgs.clientinfo[cg.clientNum].team;

	// TODO: handle missing case ?
	switch (team)
	{
	case TEAM_AXIS:
		classtype  = "r";
		teamstring = CG_TranslateString("Axis");
		break;
	case TEAM_ALLIES:
		classtype  = "b";
		teamstring = CG_TranslateString("Allies");
		break;
	default:
		CG_Printf("class: must be in a team.\n");
		return;
	}

	trap_Argv(1, cls, 64);

	if (!Q_stricmp(cls, "s") || !Q_stricmp(cls, "0"))
	{
		playerclass = PC_SOLDIER;
	}
	else if (!Q_stricmp(cls, "m") || !Q_stricmp(cls, "1"))
	{
		playerclass = PC_MEDIC;
	}
	else if (!Q_stricmp(cls, "e") || !Q_stricmp(cls, "2"))
	{
		playerclass = PC_ENGINEER;
	}
	else if (!Q_stricmp(cls, "f") || !Q_stricmp(cls, "3"))
	{
		playerclass = PC_FIELDOPS;
	}
	else if (!Q_stricmp(cls, "c") || !Q_stricmp(cls, "4"))
	{
		playerclass = PC_COVERTOPS;
	}
	else
	{
		CG_Printf("Invalid class format.\n");
		return;
	}

	if (CG_IsClassFull(playerclass, team))
	{
		CG_Printf("class: class is not available.\n");
		return;
	}

	classinfo = BG_GetPlayerClassInfo(team, playerclass);

	if (trap_Argc() > 2)
	{
		trap_Argv(2, cls, 64);
		weapon1 = Q_atoi(cls);
		if (weapon1 <= 0 || weapon1 > MAX_WEAPS_PER_CLASS)
		{
			weapon1 = classinfo->classPrimaryWeapons[0].weapon;
		}
		else if (!classinfo->classPrimaryWeapons[weapon1 - 1].weapon)
		{
			CG_Printf("Invalid command format for weapon.\n");
			return;
		}
		else
		{
			weapon1 = classinfo->classPrimaryWeapons[weapon1 - 1].weapon;
		}
	}
	else
	{
		weapon1 = classinfo->classPrimaryWeapons[0].weapon;
	}

	if (CG_IsWeaponDisabled(weapon1))
	{
		CG_Printf("class: weapon is not available.\n");
		return;
	}

	if (trap_Argc() > 3)
	{
		trap_Argv(3, cls, 64);
		weapon2 = Q_atoi(cls);

		// ensure the index is valid and different from primary weapon
		if (weapon2 <= 0 || weapon2 > MAX_WEAPS_PER_CLASS || !classinfo->classSecondaryWeapons[weapon2 - 1].weapon
		    || classinfo->classSecondaryWeapons[weapon2 - 1].weapon == weapon1)
		{
			weapon2 = BG_GetBestSecondaryWeapon(playerclass, team, weapon1, cgs.clientinfo[cg.clientNum].skill);
		}
		else
		{
			weapon2 = classinfo->classSecondaryWeapons[weapon2 - 1].weapon;
		}
	}
	else
	{
		weapon2 = BG_GetBestSecondaryWeapon(playerclass, team, weapon1, cgs.clientinfo[cg.clientNum].skill);
	}

	// Print out the selected class and weapon info
	if (BG_IsSkillAvailable(cgs.clientinfo[cg.clientNum].skill, SK_HEAVY_WEAPONS, SK_SOLDIER_SMG) && playerclass == PC_SOLDIER &&
	    !Q_stricmp(GetWeaponTableData(weapon1)->desc, GetWeaponTableData(weapon2)->desc))
	{
		CG_PriorityCenterPrint(va(CG_TranslateString("You will spawn as an %s %s with a %s."), teamstring, BG_ClassnameForNumber(playerclass), GetWeaponTableData(weapon1)->desc),
		                       -1);
	}
	else if (GetWeaponTableData(weapon2)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		CG_PriorityCenterPrint(
			va(CG_TranslateString("You will spawn as an %s %s with a %s and %s."), teamstring, BG_ClassnameForNumber(playerclass), GetWeaponTableData(weapon1)->desc,
			   GetWeaponTableData(weapon2)->desc), -1);
	}
	else
	{
		CG_PriorityCenterPrint(
			va(CG_TranslateString("You will spawn as an %s %s with a %s and a %s."), teamstring, BG_ClassnameForNumber(playerclass), GetWeaponTableData(weapon1)->desc,
			   GetWeaponTableData(weapon2)->desc), -1);
	}
	// Send the switch command to the server
	trap_SendClientCommand(va("class %s %i %i %i", classtype, playerclass, weapon1, weapon2));
}

/**
 * @brief team change menu
 */
static void CG_TeamMenu_f(void)
{
	if (cg.demoPlayback)
	{
		return;
	}

	CG_EventHandling(CGAME_EVENT_NONE, qfalse);

	if (cg_quickMessageAlt.integer)
	{
		trap_UI_Popup(UIMENU_WM_TEAMALT);
	}
	else
	{
		trap_UI_Popup(UIMENU_WM_TEAM);
	}
}

/**
 * @brief read huds from the default files or from a user provided path
 */
static void CG_ReadHuds_f(void)
{
	int count = trap_Argc();

	if (count == 2)
	{
		size_t len;
		char   tmp[MAX_QPATH] = { 0 };
		trap_Argv(1, tmp, MAX_QPATH);
		len = strlen(tmp);

		if (!tmp[0])
		{
			return;
		}

		if (len <= 4 || strcmp(tmp + len - 4, ".dat") != 0)
		{
			Q_strcat(tmp, MAX_QPATH, ".dat");
		}

		if (!CG_TryReadHudFromFile(tmp, qtrue))
		{
			CG_Printf(S_COLOR_RED "^1ERROR while reading hud file: %s\n", tmp);
		}

		return;
	}
	else if (count > 2)
	{
		CG_Printf(S_COLOR_RED "^1ERROR invalid number of arguments\n");
		return;
	}

	CG_ReadHudsFromFile();
}

/**
 * @brief Save huds to the default file
 */
static void CG_WriteHuds_f(void)
{
	CG_WriteHudsToFile();
}

static void CG_ShareTimer_f(void)
{
	qtime_t ct;
	char    *cmd, *stChar, text[MAX_SAY_TEXT];
	int     st, limboTime, nextSpawn;
	stChar = CG_SpawnTimerText();

	if (stChar == NULL)
	{
		return;
	}

	cmd       = !Q_stricmp(CG_Argv(0), "sharetimer") ? "say_team" : "say_buddy";
	st        = Q_atoi(stChar);
	limboTime = (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? cg_bluelimbotime.integer : cg_redlimbotime.integer) / 1000;
	CG_RoundTime(&ct);
	nextSpawn = MOD(ct.tm_sec - st, 60);

	trap_Cvar_VariableStringBuffer("cg_sharetimerText", text, MAX_SAY_TEXT);
	if (!strlen(text))
	{
		trap_Args(text, sizeof(text));
	}
	if (strlen(text))
	{
		const char *nextSpawnText      = "${nextspawn}";
		const char *enemyLimbotimeText = "${enemylimbotime}";

		if (Q_stristr(text, nextSpawnText))
		{
			Q_strncpyz(text, Q_StrReplace(text, nextSpawnText, va("%i", nextSpawn)), sizeof(text));
		}

		if (Q_stristr(text, enemyLimbotimeText))
		{
			Q_strncpyz(text, Q_StrReplace(text, enemyLimbotimeText, va("%i", limboTime)), sizeof(text));
		}
		trap_SendConsoleCommand(va("%s %s\n", cmd, text));
	}
	else
	{
		trap_SendConsoleCommand(va("%s Enemy spawns every %i seconds: next at %i\n", cmd, limboTime, nextSpawn));
	}

}

#ifdef FEATURE_EDV

/**
 * @brief CG_FreecamTurnLeftDown_f
 */
static void CG_FreecamTurnLeftDown_f(void)
{
	cgs.demoCamera.turn |= 0x01;
}

/**
 * @brief CG_FreecamTurnLeftUp_f
 */
static void CG_FreecamTurnLeftUp_f(void)
{
	cgs.demoCamera.turn &= ~0x01;
}

/**
 * @brief CG_FreecamTurnRightDown_f
 */
static void CG_FreecamTurnRightDown_f(void)
{
	cgs.demoCamera.turn |= 0x02;
}

/**
 * @brief CG_FreecamTurnRightUp_f
 */
static void CG_FreecamTurnRightUp_f(void)
{
	cgs.demoCamera.turn &= ~0x02;
}

/**
 * @brief CG_FreecamTurnDownDown_f
 */
static void CG_FreecamTurnDownDown_f(void)
{
	cgs.demoCamera.turn |= 0x04;
}

/**
 * @brief CG_FreecamTurnDownUp_f
 */
static void CG_FreecamTurnDownUp_f(void)
{
	cgs.demoCamera.turn &= ~0x04;
}

/**
 * @brief CG_FreecamTurnUpDown_f
 */
static void CG_FreecamTurnUpDown_f(void)
{
	cgs.demoCamera.turn |= 0x08;
}

/**
 * @brief CG_FreecamTurnUpUp_f
 */
static void CG_FreecamTurnUpUp_f(void)
{
	cgs.demoCamera.turn &= ~0x08;
}

/**
 * @brief CG_FreecamRollLeftDown_f
 */
static void CG_FreecamRollLeftDown_f(void)
{
	cgs.demoCamera.turn |= 0x20;
}

/**
 * @brief CG_FreecamRollLeftUp_f
 */
static void CG_FreecamRollLeftUp_f(void)
{
	cgs.demoCamera.turn &= ~0x20;
}

/**
 * @brief CG_FreecamRollRightDown_f
 */
static void CG_FreecamRollRightDown_f(void)
{
	cgs.demoCamera.turn |= 0x10;
}

/**
 * @brief CG_FreecamRollRightUp_f
 */
static void CG_FreecamRollRightUp_f(void)
{
	cgs.demoCamera.turn &= ~0x10;
}

/**
 * @brief CG_Freecam_f
 */
static void CG_Freecam_f(void)
{
	char state[MAX_TOKEN_CHARS];

	if (!cg.demoPlayback)
	{
		CG_Printf("Not playing a demo.\n");
		return;
	}

	trap_Argv(1, state, sizeof(state));

	if (!Q_stricmp(state, "on"))
	{
		cgs.demoCamera.renderingFreeCam = qtrue;
	}
	else if (!Q_stricmp(state, "off"))
	{
		cgs.demoCamera.renderingFreeCam = qfalse;
	}
	else
	{
		cgs.demoCamera.renderingFreeCam ^= qtrue;
	}

	CG_Printf("freecam %s\n", cgs.demoCamera.renderingFreeCam ? "ON" : "OFF");

	if (cgs.demoCamera.renderingFreeCam)
	{
		int viewheight;

		if (cg.snap->ps.eFlags & EF_CROUCHING)
		{
			viewheight = CROUCH_VIEWHEIGHT;
		}
		else if (cg.snap->ps.eFlags & EF_PRONE || cg.snap->ps.eFlags & EF_PRONE_MOVING)
		{
			viewheight = PRONE_VIEWHEIGHT;
		}
		else
		{
			viewheight = DEFAULT_VIEWHEIGHT;
		}
		cgs.demoCamera.camOrigin[2] += viewheight;
	}
}

/**
 * @brief CG_FreecamGetPos_f
 */
static void CG_FreecamGetPos_f(void)
{
	if (cg.demoPlayback)
	{
		CG_Printf("freecam origin: %.0f %.0f %.0f\n", (double) cgs.demoCamera.camOrigin[0], (double) cgs.demoCamera.camOrigin[1], (double) cgs.demoCamera.camOrigin[2]);
	}
	else
	{
		CG_Printf("freecam origin: %.0f %.0f %.0f\n", (double) cg.refdef_current->vieworg[0], (double) cg.refdef_current->vieworg[1], (double) cg.refdef_current->vieworg[2]);
	}
}

/**
 * @brief etpro_float_Argv
 * @param[in] argnum
 * @return
 */
static float etpro_float_Argv(int argnum)
{
	char buffer[MAX_TOKEN_CHARS];

	trap_Argv(argnum, buffer, sizeof(buffer));
	return Q_atof(buffer);
}

/**
 * @brief CG_FreecamSetPos_f
 */
static void CG_FreecamSetPos_f(void)
{
	int n;

	if (!cg.demoPlayback)
	{
		CG_Printf("Cheats must be enabled.\n");
		return;
	}

	n = trap_Argc();
	if (n < 4)
	{
		CG_Printf("^1Syntax: freecamSetPos x y z\n");
		return;
	}
	if (n > 4 && n < 7)
	{
		CG_Printf("^1Syntax: freecamSetPos x y z pitch yaw roll\n");
		return;
	}

	cgs.demoCamera.camOrigin[0] = etpro_float_Argv(1);
	cgs.demoCamera.camOrigin[1] = etpro_float_Argv(2);
	cgs.demoCamera.camOrigin[2] = etpro_float_Argv(3);

	if (n >= 7)
	{
		cgs.demoCamera.camAngle[0]  = etpro_float_Argv(4);
		cgs.demoCamera.camAngle[1]  = etpro_float_Argv(5);
		cgs.demoCamera.camAngle[2]  = etpro_float_Argv(6);
		cgs.demoCamera.setCamAngles = qtrue;
	}
	else
	{
		cgs.demoCamera.setCamAngles = qfalse;
	}

}

/**
 * @brief noclip in demos
 */
static void CG_NoClip_f(void)
{
	char buffer[MAX_TOKEN_CHARS];
	char state[MAX_TOKEN_CHARS];

	trap_Argv(0, buffer, sizeof(buffer));
	trap_Args(state, sizeof(state));

	if (!cg.demoPlayback)
	{
		if (trap_Argc() > 1)
		{
			trap_SendClientCommand(va("noclip %s", state));
		}
		else
		{
			trap_SendClientCommand("noclip");
		}
	}
	else
	{
		if (!Q_stricmp(state, "on"))
		{
			cgs.demoCamera.noclip = qtrue;
		}
		else if (!Q_stricmp(state, "off"))
		{
			cgs.demoCamera.noclip = qfalse;
		}
		else
		{
			cgs.demoCamera.noclip ^= qtrue;
		}
		CG_Printf("noclip %s\n", cgs.demoCamera.noclip ? "ON" : "OFF");
	}
}

#endif

static void CG_PrintObjectiveInfo_f(void)
{
	int i;

	CG_Printf("^2Objective Info\n");

	for (i = 0; i < MAX_OID_TRIGGERS; i++)
	{
		if (cgs.oidInfo[i].name[0] != '\0')
		{
			// add
			// - customimageallies
			// - customimageaxis
			// - origin ?!
			CG_Printf("[%2i] %-26s -> num: %3i - spawnflags: %3i - objflags: %3i\n", i + 1, cgs.oidInfo[i].name, cgs.oidInfo[i].entityNum, cgs.oidInfo[i].spawnflags,
			          cgs.oidInfo[i].objflags);
		}
		else
		{
			break;
		}
	}

	CG_Printf("^2%i from %i objectives defined\n", i, MAX_OID_TRIGGERS);
}

static void CG_ListSpawnPoints_f(void)
{
	int i;
	CG_Printf("^2Spawn Points\n");
	for (i = 0; i < cg.spawnCount; i++)
	{
		// autospawn
		if (i == 0)
		{
			CG_Printf("^7[^2%2i^7]   ^o%-26s\n", i, cg.spawnPoints[i]);
		}
		else if ((cg.spawnTeams[i] & 0xF) == 0)
		{
			continue;
		}
		else if (cg.spawnTeams[i] & 256)   // inactive
		{
			CG_Printf("^9[%2i] %s %-26s\n", i, ((cg.spawnTeams[i] & 0xF) == TEAM_AXIS) ? "X" : "A", cg.spawnPoints[i]);
		}
		else
		{
			CG_Printf("^7[^2%2i^7] %s ^o%-26s\n", i, (cg.spawnTeams[i] == TEAM_AXIS) ? "^1X" : "^$A", cg.spawnPoints[i]);
		}
	}
}

static void CG_Location_f(void)
{
	char token[MAX_TOKEN_CHARS];
	int  args = trap_Argc();

	if (args < 2)
	{
		CG_Printf("^1loc needs at least 2 arguments\n");
		return;
	}

	if (!cgs.sv_cheats)
	{
		CG_Printf("^1loc is cheat protected\n");
		return;
	}

	trap_Argv(1, token, sizeof(token));

	if (!Q_stricmp(token, "open"))
	{
		CG_LocationsEditor(qtrue);
	}
	else if (!Q_stricmp(token, "close"))
	{
		CG_LocationsEditor(qfalse);
	}
	else if (!Q_stricmp(token, "save"))
	{
		if (args >= 3)
		{
			trap_Argv(2, token, sizeof(token));
			CG_LocationsSave(token);
		}
		else
		{
			CG_LocationsSave(NULL);
		}
	}
	else if (!Q_stricmp(token, "rename"))
	{
		if (args < 3)
		{
			CG_Printf(S_COLOR_RED "Message text required\n");
			return;
		}
		trap_Argv(2, token, sizeof(token));
		CG_LocationsRenameCurrent(token);
	}
	else if (!Q_stricmp(token, "add"))
	{
		if (args < 3)
		{
			CG_Printf(S_COLOR_RED "Message text required\n");
			return;
		}
		trap_Argv(2, token, sizeof(token));
		CG_LocationsAdd(token);
	}
	else if (!Q_stricmp(token, "remove"))
	{
		CG_LocationsRemoveCurrent();
	}
	else if (!Q_stricmp(token, "move"))
	{
		CG_LocationsMoveCurrent();
	}
	else if (!Q_stricmp(token, "dump"))
	{
		CG_LocationsDump();
	}
	else if (!Q_stricmp(token, "reload"))
	{
		CG_LocationsReload();
	}
	else
	{
		CG_Printf("^1loc: unknown argument: %s\nSupported arguments: open/close/save/rename/add/remove/move/dump/reload\n", token);
	}
}

static void CG_Camera_f(void)
{
	char token[MAX_TOKEN_CHARS];
	int  args = trap_Argc();

	if (args < 2)
	{
		CG_Printf("^1camera needs at least 2 arguments\n");
		return;
	}

	// FIXME: maybe allow playback for shoutcasters? -- Enabled when actually ready..
	if (!cgs.sv_cheats)
	{
		CG_Printf("^1camera is cheat protected\n");
		return;
	}

	trap_Argv(1, token, sizeof(token));

	if (!Q_stricmp(token, "open"))
	{
		CG_ActivateCameraEditor();
	}
	else if (!Q_stricmp(token, "close"))
	{
		CG_DeActivateCameraEditor();
	}
	else if (!Q_stricmp(token, "add"))
	{
		CG_CameraAddCurrentPoint();
	}
	else if (!Q_stricmp(token, "ct"))
	{
		CG_AddControlPoint();
	}
	else if (!Q_stricmp(token, "play"))
	{
		if (trap_Argc() > 2)
		{
			trap_Argv(2, token, sizeof(token));
			CG_PlayCurrentCamera(Q_atoi(token));
		}
		else
		{
			CG_PlayCurrentCamera(1);
		}
	}
	else if (!Q_stricmp(token, "clear"))
	{
		CG_ClearCamera();
	}
	else
	{
		CG_Printf("^1camera: unknown argument: %s\nSupported arguments: #FIXME\n", token);
	}
}

static void CG_EditHud_f(void)
{
	if (cg.editingHud)
	{
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}
	else
	{
		CG_EventHandling(CGAME_EVENT_HUDEDITOR, qfalse);
	}
}

static qboolean CG_ParseFloatValueAtIndex(int *argIndex, float *value, char fieldLetter)
{
	char token[MAX_TOKEN_CHARS];

	trap_Argv(++*argIndex, token, sizeof(token));

	if (!Q_isanumber(token))
	{
		CG_Printf("^1Invalid ^3<%c> ^1argument, not a number\n", fieldLetter);
		return qfalse;
	}

	*value = Q_atof(token);

	return qtrue;
}

static qboolean CG_SetRectComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	rectDef_t *value = (rectDef_t *)((char *)comp + offset);

	if ((trap_Argc() - *argIndex) <= 4)
	{
		CG_Printf("^3rect field component needs at least 4 arguments <x> <y> <w> <h>\n");
		CG_Printf("^7Current value is %f %f %f %f\n", value->x, value->y, value->w, value->h);
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->x, 'x'))
	{
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->y, 'y'))
	{
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->w, 'w'))
	{
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->h, 'h'))
	{
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_SetInternalRectComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	if (!CG_SetRectComponentFromCommand(argIndex, comp, offset))
	{
		return qfalse;
	}

	if (!CG_ComputeComponentPosition(comp, 0))
	{
		CG_Printf("^3component location could not be calculated\n");
		return qfalse;
	}

	return qtrue;
}

typedef enum compPositionIndex_s
{
	COMPPOS_CENTER,
	COMPPOS_LEFT,
	COMPPOS_RIGHT,
	COMPPOS_TOP,
	COMPPOS_BOTTOM,
	COMPPOS_TOPLEFT,
	COMPPOS_BOTTOMLEFT,
	COMPPOS_TOPRIGHT,
	COMPPOS_BOTTOMRIGHT,
	COMPPOS_MAX,

} compPositionIndex_t;

typedef struct compPosition_s
{
	compPositionIndex_t index;
	char *name;
} compPosition_t;

static compPosition_t compPosition[] =
{
	{ COMPPOS_CENTER,      "center"      },
	{ COMPPOS_LEFT,        "left"        },
	{ COMPPOS_RIGHT,       "right"       },
	{ COMPPOS_TOP,         "top"         },
	{ COMPPOS_BOTTOM,      "bottom"      },
	{ COMPPOS_TOPLEFT,     "topleft"     },
	{ COMPPOS_BOTTOMLEFT,  "bottomleft"  },
	{ COMPPOS_TOPRIGHT,    "topright"    },
	{ COMPPOS_BOTTOMRIGHT, "bottomright" },
	{ COMPPOS_MAX,         NULL          },
};

static void CG_SetPositionComponentHelp(float x, float y)
{
	int  i;
	char *str = NULL;

	CG_Printf("^3pos field component needs at least 1 argument <posName> or 2 arguments <x> <y> or 3 arguments <posName> <offsetX> <offsetY>\n");
	CG_Printf("^7Current value is %f %f\n", x, y);

	for (i = 0; compPosition[i].name; ++i)
	{
		str = va("%s%-11s%s", str ? str : "", compPosition[i].name, !((i + 1) % 5) ? "\n" : "    ");
	}

	CG_Printf("\n\nAvailable ^3<posName> ^7:\n\n%s", str);
}


static qboolean CG_SetPositionComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	char      token[MAX_TOKEN_CHARS];
	rectDef_t *value = (rectDef_t *)((char *)comp + offset);

	if ((trap_Argc() - *argIndex) <= 1)
	{
		CG_SetPositionComponentHelp(value->x, value->y);
		return qfalse;
	}

	trap_Argv(++*argIndex, token, sizeof(token));

	// posName argument
	if (!Q_isanumber(token))
	{
		int   i;
		float x, y;

		for (i = 0; compPosition[i].name; ++i)
		{
			if (!Q_strncmp(token, compPosition[i].name, MAX_TOKEN_CHARS))
			{
				break;
			}
		}

		if (i == COMPPOS_MAX)
		{
			CG_Printf("^1Invalid ^3<%s> ^1argument, not a valid position name\n", token);
			return qfalse;
		}

		switch (i)
		{
		case COMPPOS_CENTER:      value->x = (Ccg_WideX(SCREEN_WIDTH) - value->w) * .5f; value->y = (SCREEN_HEIGHT - value->h) * .5f; break;
		case COMPPOS_LEFT:        value->x = 0; break;
		case COMPPOS_RIGHT:       value->x = Ccg_WideX(SCREEN_WIDTH) - value->w; break;
		case COMPPOS_TOP:         value->y = 0; break;
		case COMPPOS_BOTTOM:      value->y = SCREEN_HEIGHT - value->h; break;
		case COMPPOS_TOPLEFT:     value->x = 0; value->y = 0; break;
		case COMPPOS_BOTTOMLEFT:  value->x = 0; value->y = SCREEN_HEIGHT - value->h; break;
		case COMPPOS_TOPRIGHT:    value->x = Ccg_WideX(SCREEN_WIDTH) - value->w; value->y = 0; break;
		case COMPPOS_BOTTOMRIGHT: value->x = Ccg_WideX(SCREEN_WIDTH) - value->w; value->y = SCREEN_HEIGHT - value->h; break;
		default: break;     // should not be possible
		}

		// <x> <y> arguments for adding offset
		if ((trap_Argc() - *argIndex) <= 2)
		{
			return qtrue;
		}

		if (!CG_ParseFloatValueAtIndex(argIndex, &x, 'x'))
		{
			return qfalse;
		}

		if (!CG_ParseFloatValueAtIndex(argIndex, &y, 'y'))
		{
			return qfalse;
		}

		value->x += x;
		value->y += y;

		return qtrue;
	}

	// <x> <y> arguments
	if ((trap_Argc() - --*argIndex) <= 2)
	{
		CG_SetPositionComponentHelp(value->x, value->y);
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->x, 'x'))
	{
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->y, 'y'))
	{
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_SetInternalPositionComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	rectDef_t *value = (rectDef_t *)((char *)comp + offset);

	// <x> <y> arguments
	if ((trap_Argc() - *argIndex) <= 2)
	{
		CG_SetPositionComponentHelp(value->x, value->y);
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->x, 'x'))
	{
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->y, 'y'))
	{
		return qfalse;
	}

	if (!CG_ComputeComponentPosition(comp, 0))
	{
		CG_Printf("^3component location could not be calculated\n");
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_SetSizeComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	rectDef_t *value = (rectDef_t *)((char *)comp + offset);

	if ((trap_Argc() - *argIndex) <= 2)
	{
		CG_Printf("^3size field component needs at least 2 arguments <w> <h>\n");
		CG_Printf("^7Current value is %f %f\n", value->w, value->h);
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->w, 'w'))
	{
		return qfalse;
	}

	if (!CG_ParseFloatValueAtIndex(argIndex, &value->h, 'h'))
	{
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_SetFloatComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	char  token[MAX_TOKEN_CHARS];
	float *value = (float *) ((char *) comp + offset);

	if ((trap_Argc() - *argIndex) <= 1)
	{
		CG_Printf("^3float field component needs at least 1 argument <value>\n");
		CG_Printf("^7Current value is %f\n", *value);
		return qfalse;
	}

	trap_Argv(++*argIndex, token, sizeof(token));

	if (!Q_isanumber(token))
	{
		CG_Printf("^1Invalid ^3<float> ^1argument, not a number\n");
		return qfalse;
	}

	*value = Q_atof(token);

	return qtrue;
}

static qboolean CG_SetIntComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	char token[MAX_TOKEN_CHARS];
	int  *value = (int *) ((char *) comp + offset);

	if ((trap_Argc() - *argIndex) <= 1)
	{
		CG_Printf("^3int field component needs at least 1 argument <value>\n");
		CG_Printf("^7Current value is %d\n", *value);
		return qfalse;
	}

	trap_Argv(++*argIndex, token, sizeof(token));

	if (!Q_isanumber(token))
	{
		CG_Printf("^1Invalid ^3<int> ^1argument, not a number\n");
		return qfalse;
	}

	*value = Q_atoi(token);

	return qtrue;
}

static qboolean CG_SetAnchorPointFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	char          token[MAX_TOKEN_CHARS];
	anchorPoint_t *anchor      = (anchorPoint_t *) ((char *) comp + offset);
	int           tmpAnchorVal = -1;

	if ((trap_Argc() - *argIndex) <= 1)
	{
		CG_Printf("^3point field component needs at least 1 argument <value>\n");
		CG_Printf("^7Current value is %i\n", *anchor);
		return qfalse;
	}

	trap_Argv(++*argIndex, token, sizeof(token));
	tmpAnchorVal = Q_atoi(token);

	if (tmpAnchorVal >= TOP_LEFT && tmpAnchorVal <= CENTER)
	{
		if (*anchor == tmpAnchorVal)
		{
			return qtrue;
		}

		*anchor = tmpAnchorVal;
		if (!CG_ComputeComponentPosition(comp, 0))
		{
			CG_Printf("^3component location could not be calculated\n");
		}
	}
	else
	{
		CG_Printf("^3point field component valid values are %i - %i\n", TOP_LEFT, CENTER);
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_SetAnchorParentComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	char           token[MAX_TOKEN_CHARS];
	hudComponent_t *tmp         = NULL;
	hudComponent_t **parentComp = (hudComponent_t **) ((char *) comp + offset);

	if ((trap_Argc() - *argIndex) <= 1)
	{
		const char *name = CG_FindComponentName(hudData.active, *parentComp);
		CG_Printf("^3string field component needs at least 1 argument <value>\n");
		CG_Printf("^7Current value is %s\n", name ? name : "empty");
		return qfalse;
	}

	trap_Argv(++*argIndex, token, sizeof(token));

	if (!Q_stricmp(token, "null") || !Q_stricmp(token, "empty"))
	{
		*parentComp = NULL;
	}
	else
	{
		tmp = CG_FindComponentByName(hudData.active, token);
		if (!tmp)
		{
			CG_Printf("^3invalid component name given\n");
		}
		else
		{
			*parentComp = tmp;
		}
	}

	return qtrue;
}

static qboolean CG_SetColorsComponentFromCommand(int *argIndex, hudComponent_t *comp, int offset)
{
	char   token[MAX_TOKEN_CHARS];
	int    tokenCount = 0;
	vec4_t *value     = (vec4_t *) ((char *) comp + offset);

	if ((trap_Argc() - *argIndex) <= 1)
	{
		int  i;
		char *str = NULL;

		CG_Printf("^3color field component needs at least 1 argument <colorname> / <0xRRGGBB[AA]> or 3-4 arguments <r> <g> <b> [a]\n");
		CG_Printf("^7Current value is %f %f %f %f\n", (*value)[0], (*value)[1], (*value)[2], (*value)[3]);

		for (i = 0; Q_GetColorString(i); ++i)
		{
			str = va("%s%-9s%s", str ? str : "", Q_GetColorString(i), !((i + 1) % 5) ? "\n" : "    ");
		}

		CG_Printf("\n\nAvailable ^3<colorname> ^7:\n\n%s", str);

		return qfalse;
	}

	CG_Args(*argIndex + 1, 4, token, sizeof(token));

	tokenCount = Q_ParseColor(token, *value);
	if (tokenCount)
	{
		*argIndex += tokenCount;
		return qtrue;
	}
	else
	{
		CG_Printf("^1Invalid argument: (^3%s^1), not a color value (name/hex/float,3-4x/int,3-4x)\n", token);
	}

	return qfalse;
}

const hudComponentMembersFields_t hudComponentMembersFields[] =
{
	{ HUDMF(location),         CG_SetRectComponentFromCommand             },
	{ "x",                     offsetof(hudComponent_t, location) + offsetof(rectDef_t, x), CG_SetFloatComponentFromCommand},
	{ "y",                     offsetof(hudComponent_t, location) + offsetof(rectDef_t, y), CG_SetFloatComponentFromCommand},
	{ "w",                     offsetof(hudComponent_t, location) + offsetof(rectDef_t, w), CG_SetFloatComponentFromCommand},
	{ "h",                     offsetof(hudComponent_t, location) + offsetof(rectDef_t, h), CG_SetFloatComponentFromCommand},
	{ "position",              offsetof(hudComponent_t, location), CG_SetPositionComponentFromCommand},
	{ "size",                  offsetof(hudComponent_t, location), CG_SetSizeComponentFromCommand},
	{ HUDMF(visible),          CG_SetIntComponentFromCommand              },
	{ HUDMF(style),            CG_SetIntComponentFromCommand              },
	{ HUDMF(scale),            CG_SetFloatComponentFromCommand            },
	{ HUDMF(colorMain),        CG_SetColorsComponentFromCommand           },
	{ HUDMF(colorSecondary),   CG_SetColorsComponentFromCommand           },
	{ HUDMF(showBackGround),   CG_SetIntComponentFromCommand              },
	{ HUDMF(colorBackground),  CG_SetColorsComponentFromCommand           },
	{ HUDMF(showBorder),       CG_SetIntComponentFromCommand              },
	{ HUDMF(colorBorder),      CG_SetColorsComponentFromCommand           },
	{ HUDMF(styleText),        CG_SetIntComponentFromCommand              },
	{ HUDMF(alignText),        CG_SetIntComponentFromCommand              },
	{ HUDMF(autoAdjust),       CG_SetIntComponentFromCommand              },
	{ HUDMF(internalLocation), CG_SetInternalRectComponentFromCommand     },
	{ "internalPosition",      offsetof(hudComponent_t, internalLocation), CG_SetInternalPositionComponentFromCommand},
	{ HUDMF(anchorPoint),      CG_SetAnchorPointFromCommand               },
	{ "parentAnchorPoint",     offsetof(hudComponent_t, parentAnchor) + offsetof(anchor_t, point), CG_SetAnchorPointFromCommand},
	{ "parentAnchorComponent", offsetof(hudComponent_t, parentAnchor) + offsetof(anchor_t, parent), CG_SetAnchorParentComponentFromCommand},
	{ NULL,                    0, NULL                                    },
};

/**
 * @brief CG_ShowEditComponentHelp
 */
static void CG_ShowEditComponentHelp()
{
	int  i;
	char *str = NULL;

	CG_Printf("^3edit component usage :\n"
	          "\"save\"\n"
	          "\"clone|delete\" <hudnumber>\n"
	          "<compname> <field> <value> [ <field2> <value2> <field3> <value3> ... ]");

	for (i = 0; hudComponentFields[i].name; i++)
	{
		str = va("%s%-16s%s", str ? str : "", hudComponentFields[i].name, !((i + 1) % 5) ? "\n" : "    ");
	}

	CG_Printf("\n\nAvailable ^3<compname> ^7:\n\n%s", str);

	str = NULL;

	for (i = 0; hudComponentMembersFields[i].name; i++)
	{
		str = va("%s%-16s%s", str ? str : "", hudComponentMembersFields[i].name, !((i + 1) % 5) ? "\n" : "    ");
	}

	CG_Printf("\n\nAvailable ^3<field> ^7:\n\n%s\n", str);
}

static void CG_ShowEditComponentStyleHelp(const hudComponentFields_t *compField, int *value)
{
	int  i;
	char *str = NULL;

	for (i = 0; i < MAXSTYLES && compField->styles[i]; ++i)
	{
		str = va("%s%s%5d : %-16s%s", str ? str : "", ((*value) & 1 << i) ? "^2" : "^7", 1 << i, compField->styles[i], !((i + 1) % 3) ? "\n" : "    ");
	}

	if (str)
	{
		CG_Printf("Available ^3<style>^7 for %s :\n\n%s\n", compField->name, str);
	}
	else
	{
		CG_Printf("No ^3<style>^7 available for %s\n", compField->name);
	}
}

/**
 * @brief CG_EditComponent_f
 */
static void CG_EditComponent_f(void)
{
	char                       token[MAX_TOKEN_CHARS];
	const hudComponentFields_t *compField = NULL;
	hudComponent_t             *comp      = NULL;
	int                        i;
	int                        argc = trap_Argc();

	if (argc < 2)
	{
		CG_ShowEditComponentHelp();
		return;
	}

	trap_Argv(1, token, sizeof(token));

	if (!Q_stricmp(token, "?") || !Q_stricmp(token, "help"))
	{
		CG_ShowEditComponentHelp();
		return;
	}

	if (!Q_stricmp(token, "save"))
	{
		CG_HudSave(-1, -1);
		return;
	}

	if (argc < 3)
	{
		CG_ShowEditComponentHelp();

		return;
	}

	if (!Q_stricmp(token, "clone"))
	{
		trap_Argv(2, token, sizeof(token));

		CG_HudSave(Q_atoi(token), -1);
		return;
	}

	if (!Q_stricmp(token, "delete"))
	{
		trap_Argv(2, token, sizeof(token));

		CG_HudSave(-1, Q_atoi(token));
		return;
	}

	// find components name
	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (!Q_stricmp(token, hudComponentFields[i].name))
		{
			compField = &hudComponentFields[i];
			comp      = (hudComponent_t *)((byte *)hudData.active + compField->offset);
			break;
		}
	}

	if (!comp)
	{
		CG_Printf("^1 Cannot find component name: %s\n", token);
		return;
	}

	// parse command input arguments
	for (i = 2; i < argc; i++)
	{
		int      j;
		qboolean fieldFound = qfalse;

		trap_Argv(i, token, sizeof(token));

		// find field name
		for (j = 0; hudComponentMembersFields[j].name; j++)
		{
			if (!Q_stricmp(token, hudComponentMembersFields[j].name))
			{
				fieldFound = qtrue;

				// try to parse the field arguments
				if (!hudComponentMembersFields[j].parse(&i, comp, hudComponentMembersFields[j].offset))
				{
					// display specific help for style
					if (!Q_stricmp(hudComponentMembersFields[j].name, "style"))
					{
						CG_ShowEditComponentStyleHelp(compField, (int *)((char *)comp + hudComponentMembersFields[j].offset));
						return;
					}

					// in case there is no next argument, don't display an error as the user request help
					if (++i != argc)
					{
						CG_Printf("^1Failed to parse ^3<%s> ^1field arguments\n", hudComponentMembersFields[j].name);
					}

					return;
				}
				else
				{
					// update the component internals if needed
					CG_CalculateComponentInternals(hudData.active, comp);
				}

				break;
			}
		}

		if (!fieldFound)
		{
			CG_Printf("^1 Cannot find field name: %s\n", token);
			return;
		}
	}
}

static qboolean CG_EditHudComponentFieldsComplete()
{
	int  i;
	char token[MAX_TOKEN_CHARS];

	// don't auto complete if previous arg is members field
	trap_Argv(trap_Argc() - 1, token, MAX_TOKEN_CHARS);

	for (i = 0; hudComponentMembersFields[i].name; i++)
	{
		if (!Q_strncmp(token, hudComponentMembersFields[i].name, MAX_TOKEN_CHARS))
		{
			return qfalse;
		}
	}

	trap_Argv(1, token, MAX_TOKEN_CHARS);

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (!Q_strncmp(token, hudComponentFields[i].name, MAX_TOKEN_CHARS))
		{
			for (i = 0; hudComponentMembersFields[i].name; i++)
			{
				trap_CommandComplete(hudComponentMembersFields[i].name);
			}

			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_EditHudComponentMembersFieldsComplete()
{
	int  i;
	char token[MAX_TOKEN_CHARS];

	for (i = 1; i < 3; i++)
	{
		trap_Argv(trap_Argc() - i, token, MAX_TOKEN_CHARS);

		// don't auto complete if previous arg is a position or a color value
		if (i == 1)
		{
			int j;

			for (j = 0; compPosition[j].name; j++)
			{
				if (!Q_strncmp(token, compPosition[j].name, MAX_TOKEN_CHARS))
				{
					return qfalse;
				}
			}

			for (j = 0; Q_GetColorString(j); j++)
			{
				if (!Q_strncmp(token, Q_GetColorString(j), MAX_TOKEN_CHARS))
				{
					return qfalse;
				}
			}
		}

		if (!Q_strncmp(token, "position", MAX_TOKEN_CHARS))
		{
			for (i = 0; compPosition[i].name; i++)
			{
				trap_CommandComplete(compPosition[i].name);
			}

			return qtrue;
		}

		if (!Q_strncmp(token, "colorMain", MAX_TOKEN_CHARS)
		    || !Q_strncmp(token, "colorSecondary", MAX_TOKEN_CHARS)
		    || !Q_strncmp(token, "colorBackground", MAX_TOKEN_CHARS)
		    || !Q_strncmp(token, "colorBorder", MAX_TOKEN_CHARS))
		{
			for (i = 0; Q_GetColorString(i); i++)
			{
				trap_CommandComplete(Q_GetColorString(i));
			}

			return qtrue;
		}
	}

	return qfalse;
}

static void CG_EditHudComponentComplete(void)
{
	if (trap_Argc() > 1)
	{
		if (CG_EditHudComponentMembersFieldsComplete())
		{
			return;
		}

		if (CG_EditHudComponentFieldsComplete())
		{
			return;
		}
	}

	if (trap_Argc() < 3)
	{
		int  i;
		char token[MAX_TOKEN_CHARS];

		trap_Argv(trap_Argc() - 1, token, MAX_TOKEN_CHARS);

		// don't auto complete if previous arg is members field
		for (i = 0; hudComponentMembersFields[i].name; i++)
		{
			if (!Q_strncmp(token, hudComponentMembersFields[i].name, MAX_TOKEN_CHARS))
			{
				return;
			}
		}

		trap_CommandComplete("help");
		trap_CommandComplete("save");
		trap_CommandComplete("clone");
		trap_CommandComplete("delete");

		for (i = 0; hudComponentFields[i].name; i++)
		{
			trap_CommandComplete(hudComponentFields[i].name);
		}
	}
}

#define EDITCOMPONENT_CROSSHAIR_STRING "editcomponent crosshair"

static void CG_CrosshairSizePos_f(void)
{
	if (trap_Argc() > 1)
	{
		const char *token = CG_Argv(1);

		if (Q_isanumber(token))
		{
			const float value = Q_atof(token);

			float sizeX = cg_crosshairSize.value;
			float sizeY = cg_crosshairSize.value;

			float scaleX = cg_crosshairScaleX.value;
			float scaleY = cg_crosshairScaleY.value;

			float offsetX = cg_crosshairX.value;
			float offsetY = cg_crosshairY.value;

			const char *cmd = CG_Argv(0);

			if (!Q_stricmp(cmd, "cg_crosshairSize_f"))
			{
				sizeX = sizeY = value;
			}
			else if (!Q_stricmp(cmd, "cg_crosshairX_f"))
			{
				offsetX = value;
			}
			else if (!Q_stricmp(cmd, "cg_crosshairY_f"))
			{
				offsetY = value;
			}
			else if (!Q_stricmp(cmd, "cg_crosshairScaleX_f"))
			{
				scaleX = value;
			}
			else if (!Q_stricmp(cmd, "cg_crosshairScaleY_f"))
			{
				scaleY = value;
			}

			// always perform scaling no matter the command, so we get the correct final size
			sizeX *= scaleX;
			sizeY *= scaleY;

			CG_GetActiveHUD()->crosshair.location.x = (Ccg_WideX(SCREEN_WIDTH) - sizeX + offsetX) * .5f;
			CG_GetActiveHUD()->crosshair.location.y = (SCREEN_HEIGHT - sizeY + offsetY) * .5f;
			CG_GetActiveHUD()->crosshair.location.w = sizeX;
			CG_GetActiveHUD()->crosshair.location.h = sizeY;
		}
	}
}

static void CG_CrosshairAlpha_f(void)
{
	if (trap_Argc() > 1)
	{
		const char *token;

		token = CG_Argv(1);

		if (Q_isanumber(token))
		{
			CG_GetActiveHUD()->crosshair.colorMain[3] = Q_atof(token);
		}
	}
}

static void CG_CrosshairColor_f(void)
{
	if (trap_Argc() > 1)
	{
		const char *colString = CG_ArgsStat(1, 4);

		if (!Q_ParseColor(colString, CG_GetActiveHUD()->crosshair.colorMain))
		{
			CG_Printf("^1Invalid crosshair color args: (^3%s^1), not a color value (name/hex/float,3-4x/int,3-4x)\n", colString);
		}
	}
}

static void CG_CrosshairAlphaAlt_f(void)
{
	if (trap_Argc() > 1)
	{
		const char *token;

		token = CG_Argv(1);

		if (Q_isanumber(token))
		{
			CG_GetActiveHUD()->crosshair.colorSecondary[3] = Q_atof(token);
		}
	}
}

static void CG_CrosshairColorAlt_f(void)
{
	if (trap_Argc() > 1)
	{
		const char *colString = CG_ArgsStat(1, 4);

		if (!Q_ParseColor(colString, CG_GetActiveHUD()->crosshair.colorSecondary))
		{
			CG_Printf("^1Invalid crosshair color args: (^3%s^1), not a color value (name/hex/float,3-4x/int,3-4x)\n", colString);
		}
	}
}

static void CG_CrosshairPulse_f(void)
{
	if (trap_Argc() > 1)
	{
		if (Q_atoi(CG_Argv(1)))
		{
			ENABLEBIT(CG_GetActiveHUD()->crosshair.style, 0);
		}
		else
		{
			CLEARBIT(CG_GetActiveHUD()->crosshair.style, 0);
		}
	}
}

static void CG_CrosshairHealth_f(void)
{
	if (trap_Argc() > 1)
	{
		if (Q_atoi(CG_Argv(1)))
		{
			ENABLEBIT(CG_GetActiveHUD()->crosshair.style, 2);
			ENABLEBIT(CG_GetActiveHUD()->crosshair.style, 3);
		}
		else
		{
			CLEARBIT(CG_GetActiveHUD()->crosshair.style, 2);
			CLEARBIT(CG_GetActiveHUD()->crosshair.style, 3);
		}
	}
}

static consoleCommand_t commands[] =
{
	{ "testgun",                CG_TestGun_f              },
	{ "testmodel",              CG_TestModel_f            },
	{ "nextframe",              CG_TestModelNextFrame_f   },
	{ "prevframe",              CG_TestModelPrevFrame_f   },
	{ "nextskin",               CG_TestModelNextSkin_f    },
	{ "prevskin",               CG_TestModelPrevSkin_f    },
	{ "viewpos",                CG_Viewpos_f              },
	{ "+scores",                CG_ScoresDown_f           },
	{ "-scores",                CG_ScoresUp_f             },
	{ "zoomin",                 CG_ZoomIn_f               },
	{ "zoomout",                CG_ZoomOut_f              },
	{ "weaplastused",           CG_LastWeaponUsed_f       },
	{ "weapnextinbank",         CG_NextWeaponInBank_f     },
	{ "weapprevinbank",         CG_PrevWeaponInBank_f     },
	{ "weapnext",               CG_NextWeapon_f           },
	{ "weapprev",               CG_PrevWeapon_f           },
	{ "weapalt",                CG_AltWeapon_f            },
	{ "weapon",                 CG_Weapon_f               },
	{ "weaponbank",             CG_WeaponBank_f           },
	{ "fade",                   CG_Fade_f                 },

	{ "mp_QuickMessage",        CG_QuickMessage_f         },
	{ "mp_fireteammsg",         CG_QuickFireteams_f       },
	{ "mp_fireteamadmin",       CG_QuickFireteamAdmin_f   },
	{ "wm_sayPlayerClass",      CG_SayPlayerClass_f       },
	{ "wm_ftsayPlayerClass",    CG_FTSayPlayerClass_f     },

	{ "spawnmenu",              CG_QuickSpawnpoint_f      },

	{ "VoiceChat",              CG_VoiceChat_f            },
	{ "VoiceTeamChat",          CG_TeamVoiceChat_f        },

	// say, teamsay, etc
	{ "messageMode",            CG_MessageMode_f          },
	{ "messageMode2",           CG_MessageMode_f          },
	{ "messageMode3",           CG_MessageMode_f          },
	{ "messageSend",            CG_MessageSend_f          },

	{ "SetWeaponCrosshair",     CG_SetWeaponCrosshair_f   },

	{ "VoiceFireTeamChat",      CG_BuddyVoiceChat_f       },

	{ "openlimbomenu",          CG_LimboMenu_f            },

	{ "+stats",                 CG_StatsDown_f            },
	{ "-stats",                 CG_StatsUp_f              },
	{ "+topshots",              CG_topshotsDown_f         },
	{ "-topshots",              CG_topshotsUp_f           },
	{ "+objectives",            CG_objectivesDown_f       },
	{ "-objectives",            CG_objectivesUp_f         },

	{ "autoRecord",             CG_autoRecord_f           },
	{ "toggleRecord",           CG_toggleRecord_f         },
	{ "autoScreenshot",         CG_autoScreenShot_f       },
	{ "currentTime",            CG_currentTime_f          },
	{ "keyoff",                 CG_keyOff_f               },
	{ "keyon",                  CG_keyOn_f                },
#ifdef FEATURE_MULTIVIEW
	{ "mvactivate",             CG_mvToggleAll_f          },
	{ "mvdel",                  CG_mvDelete_f             },
	{ "mvhide",                 CG_mvHideView_f           },
	{ "mvnew",                  CG_mvNew_f                },
	{ "mvshow",                 CG_mvShowView_f           },
	{ "mvswap",                 CG_mvSwapViews_f          },
	{ "mvtoggle",               CG_mvToggleView_f         },
	{ "spechelp",               CG_toggleSpecHelp_f       },
#endif
	{ "statsdump",              CG_dumpStats_f            },
	{ "+vstr",                  CG_vstrDown_f             },
	{ "-vstr",                  CG_vstrUp_f               },

	{ "selectbuddy",            CG_SelectBuddy_f          },

	{ "MapZoomIn",              CG_AutomapZoomIn_f        },
	{ "MapZoomOut",             CG_AutomapZoomOut_f       },
	{ "+mapexpand",             CG_AutomapExpandDown_f    },
	{ "-mapexpand",             CG_AutomapExpandUp_f      },

	{ "generateTracemap",       CG_GenerateTracemap       },

	{ "ToggleAutoMap",          CG_ToggleAutomap_f        }, // toggle automap on/off

	{ "editSpeakers",           CG_EditSpeakers_f         },
	{ "dumpSpeaker",            CG_DumpSpeaker_f          },
	{ "modifySpeaker",          CG_ModifySpeaker_f        },
	{ "undoSpeaker",            CG_UndoSpeaker_f          },
	{ "cpm",                    CG_CPM_f                  },
	{ "forcetapout",            CG_ForceTapOut_f          },
	{ "timerSet",               CG_TimerSet_f             },
	{ "timerReset",             CG_TimerReset_f           },
	{ "resetTimer",             CG_TimerReset_f           }, // keep ETPro compatibility
	{ "class",                  CG_Class_f                },
	{ "classmenu",              CG_ClassMenu_f            },
	{ "teammenu",               CG_TeamMenu_f             },
	{ "readHuds",               CG_ReadHuds_f             },
	{ "writeHuds",              CG_WriteHuds_f            },
	{ "sharetimer",             CG_ShareTimer_f           },
	{ "sharetimer_buddy",       CG_ShareTimer_f           },
#ifdef FEATURE_EDV
	{ "+freecam_turnleft",      CG_FreecamTurnLeftDown_f  },
	{ "-freecam_turnleft",      CG_FreecamTurnLeftUp_f    },
	{ "+freecam_turnright",     CG_FreecamTurnRightDown_f },
	{ "-freecam_turnright",     CG_FreecamTurnRightUp_f   },

	{ "+freecam_turnup",        CG_FreecamTurnUpDown_f    },
	{ "-freecam_turnup",        CG_FreecamTurnUpUp_f      },
	{ "+freecam_turndown",      CG_FreecamTurnDownDown_f  },
	{ "-freecam_turndown",      CG_FreecamTurnDownUp_f    },

	{ "+freecam_rollleft",      CG_FreecamRollLeftDown_f  },
	{ "-freecam_rollleft",      CG_FreecamRollLeftUp_f    },
	{ "+freecam_rollright",     CG_FreecamRollRightDown_f },
	{ "-freecam_rollright",     CG_FreecamRollRightUp_f   },
	{ "freecam",                CG_Freecam_f              },
	{ "freecamsetpos",          CG_FreecamSetPos_f        },
	{ "freecamgetpos",          CG_FreecamGetPos_f        },

	{ "noclip",                 CG_NoClip_f               },
#endif
	// objective info list for mappers/scripters (and players? - we might extend it)
	{ "oinfo",                  CG_PrintObjectiveInfo_f   },
	{ "resetmaxspeed",          CG_ResetMaxSpeed_f        },
	{ "listspawnpt",            CG_ListSpawnPoints_f      },

	{ "loc",                    CG_Location_f             },
	{ "camera",                 CG_Camera_f               },
	{ "edithud",                CG_EditHud_f              },
	{ "editcomponent",          CG_EditComponent_f        },

	// TODO: Implement "alias" system and create those as customizable alias command
	{ "cg_crosshairSize_f",     CG_CrosshairSizePos_f     },
	{ "cg_crosshairAlpha_f",    CG_CrosshairAlpha_f       },
	{ "cg_crosshairColor_f",    CG_CrosshairColor_f       },
	{ "cg_crosshairAlphaAlt_f", CG_CrosshairAlphaAlt_f    },
	{ "cg_crosshairColorAlt_f", CG_CrosshairColorAlt_f    },
	{ "cg_crosshairPulse_f",    CG_CrosshairPulse_f       },
	{ "cg_crosshairHealth_f",   CG_CrosshairHealth_f      },
	{ "cg_crosshairX_f",        CG_CrosshairSizePos_f     },
	{ "cg_crosshairY_f",        CG_CrosshairSizePos_f     },
	{ "cg_crosshairScaleX_f",   CG_CrosshairSizePos_f     },
	{ "cg_crosshairScaleY_f",   CG_CrosshairSizePos_f     },

	{ NULL,                     NULL                      }
};

/**
 * @var gameCommand
 * @brief Game command list
 *
 * @todo Shared the command list in both mod
 * or delete this list and use the help '?' command to provided a list of command
 * or retrieved the command list by parsing the help '?' command
 * or delete the help '?' command on game side and write help on cgame side
 */
static const char *gameCommand[] =
{
	"say",
	"say_team",
	"say_buddy",
	"say_teamnl",
	"vsay",
	"vsay_team",
	"vsay_buddy",
	"?",
	// copy of ?
	"commands",
	"help",
	"+stats",
	"+topshots",
	"+objectives",
	"autorecord",
	"autoscreenshot",
	"bottomshots",
	"callvote",
	"currenttime",
	"dropobj",
	"fireteam",
	"follow",
	"follownext",
	"followprev",
	"forcetapout",
	"give",
	"god",
	"ignore",
#ifdef FEATURE_PRESTIGE
	"imcollectpr",
#endif
	"immaplist",
	"immaphistory",
	"impkd",
#ifdef FEATURE_PRESTIGE
	"impr",
#endif
	"impt",
	"imready",
#ifdef FEATURE_RATING
	"imsr",
#endif
	"imvotetally",
	"imwa",
	"imws",
	//   "invite",
	"kill",
	"lock",
	"mapvote",
#ifdef FEATURE_MULTIVIE
	"mvadd",
	"mvallies",
	"mvaxis",
	"mvall",
	"mvnone",
	"mvdel",
#endif
	"noclip",
	"nofatigue",
	"nostamina",
	"notarget",
	"notready",
	"obj",
	"pause",
	"players",
	"rconAuth",
	"ready",
	"readyteam",
	"ref",
	//   "remove",
	"rs",
	"sclogin",
	"sclogout",
	//"score",      // used to updated score in debriefing and ingame score board. Not meant to be sent manually.
	"scores",       // display the scores in console
	"setviewpos",
	"setspawnpt",
	"sgstats",
	"showstats",
	"specinvite",
	"specuninvite",
	"speclock",
	//   "speconly",
	"specunlock",
	"statsall",
	"statsdump",
	"stoprecord",
	"stshots",
	"team",
	"timein",
	"timeout",
	"topshots",
	"unignore",
	"unlock",
	"unpause",
	"unready",
	"vote",
	"weaponstats",
	"where",
	"ws",
	"wstats",
	NULL,
};

/**
 * @var gameConsoleCommand
 * @brief Game console command list
 *
 * @todo List the server command
 * find a more elegant / dynamic way to sharesonsole command list
 */
static const char *gameConsoleCommand[] =
{
	"entitylist",
	"csinfo",
	"forceteam",
	"game_memory",
	"addip",
	"removeip",
	"listip",
	"listmaxlivesip",
	"start_match",
	"reset_match",
	"swap_teams",
	"shuffle_teams",
	"shuffle_teams_norestart",
#ifdef FEATURE_RATING
	"shuffle_teams_sr",
	"shuffle_teams_sr_norestart",
#endif
	"makeReferee",
	"removeReferee",
	"makeShoutcaster",
	"removeShoutcaster",
	"mute",
	"unmute",
	"ban",
	"campaign",
	"listcampaigns",
	"revive",
	"kick",                                // moved from engine
	"clientkick",
#ifdef FEATURE_OMNIBOT
	"bot",
#endif
	"cp",
	"reloadConfig",
	"loadConfig",
	"sv_cvarempty",
	"sv_cvar",
	"playsound",
	"playsound_env",
	"gib",
	"die",
	"freeze",
	"unfreeze",
	"burn",
	"pip",
	"throw",
#ifdef ETLEGACY_DEBUG
	"ae",                                  //ae <playername> <animEvent>
#endif
	"ref",                                 // console also gets ref commands
	"passvote",
	"cancelvote",
	NULL
};

/**
 * @brief CG_AddGameConsoleCommand
 */
void CG_AddGameConsoleCommand()
{
	if (cgs.localServer || cgs.clientinfo[cg.clientNum].refStatus != RL_NONE)
	{
		int i;

		for (i = 0; gameConsoleCommand[i]; i++)
		{
			trap_AddCommand(gameConsoleCommand[i]);
		}
	}
}

/**
 * @brief CG_RemoveGameConsoleCommand
 */
void CG_RemoveGameConsoleCommand()
{
	if (!cgs.localServer)
	{
		int i;

		for (i = 0; gameConsoleCommand[i]; i++)
		{
			trap_RemoveCommand(gameConsoleCommand[i]);
		}
	}
}

/**
 * @brief The string has been tokenized and can be retrieved with
 * Cmd_Argc() / Cmd_Argv()
 * @return
 */
qboolean CG_ConsoleCommand(void)
{
	const char   *cmd;
	unsigned int i;

	// don't allow console commands until a snapshot is present
	if (!cg.snap)
	{
		return qfalse;
	}

	cmd = CG_Argv(0);

	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		if (!Q_stricmp(cmd, commands[i].cmd))
		{
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}

static commandComplete_t completeFuncs[] =
{
	{ "camera",        CG_CameraCommandComplete    },
	{ "editcomponent", CG_EditHudComponentComplete },
	{ NULL,            NULL                        }
};

qboolean CG_ConsoleCompleteArgument(void)
{
	const char *baseCmd;
	int        i;

	baseCmd = CG_Argv(0);
	if (baseCmd[0] == '\\' || baseCmd[0] == '/')
	{
		baseCmd++;
	}

	for (i = 0; completeFuncs[i].cmd; i++)
	{
		if (!Q_stricmp(baseCmd, completeFuncs[i].cmd))
		{
			completeFuncs[i].complete();
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Let the client system know about all of our commands so it can perform tab
 * completion
 */
void CG_InitConsoleCommands(void)
{
	unsigned int i;

	for (i = 0; commands[i].cmd; i++)
	{
		trap_AddCommand(commands[i].cmd);
	}

	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally

	// game commands
	for (i = 0; gameCommand[i]; i++)
	{
		trap_AddCommand(gameCommand[i]);
	}

	// game console commands
	CG_AddGameConsoleCommand();

	// remove engine commands to avoid abuse
	trap_RemoveCommand("+lookup");
	trap_RemoveCommand("-lookup");
	trap_RemoveCommand("+lookdown");
	trap_RemoveCommand("-lookdown");

	// only allow configstrings command when cheats enabled
	if (!cgs.sv_cheats)
	{
		trap_RemoveCommand("configstrings");
	}
}
