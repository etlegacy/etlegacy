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
 * @file cg_consolecmds.c
 * @brief text commands typed in at the local console, or executed by a key
 * binding
 */

#include "cg_local.h"

void CG_TargetCommand_f(void)
{
	int  targetNum;
	char test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum)
	{
		return;
	}

	trap_Argv(1, test, 4);
	trap_SendConsoleCommand(va("gc %i %i", targetNum, atoi(test)));
}

/**
 * @brief Debugging command to print the current position
 */
static void CG_Viewpos_f(void)
{
	CG_Printf("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
	          (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
	          (int)cg.refdefViewAngles[YAW]);
}

void CG_LimboMenu_f(void)
{
	if (cg.showGameView)
	{
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}
	else
	{
		CG_EventHandling(CGAME_EVENT_GAMEVIEW, qfalse);
	}
}

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

void CG_topshotsDown_f(void)
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

void CG_topshotsUp_f(void)
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

void CG_ScoresDown_f(void)
{
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

void CG_ScoresUp_f(void)
{
	if (cg.showScores)
	{
		cg.showScores    = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

static void CG_TellTarget_f(void)
{
	int  clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void)
{
	int  clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1)
	{
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

/////////// cameras

#define MAX_CAMERAS 64  // matches define in splines.cpp
qboolean cameraInuse[MAX_CAMERAS];

int CG_LoadCamera(const char *name)
{
	int i;

	for (i = 1; i < MAX_CAMERAS; i++) // start at '1' since '0' is always taken by the cutscene camera
	{
		if (!cameraInuse[i])
		{
			if (trap_loadCamera(i, name))
			{
				cameraInuse[i] = qtrue;
				return i;
			}
		}
	}
	return -1;
}

void CG_FreeCamera(int camNum)
{
	cameraInuse[camNum] = qfalse;
}

// @TEST.  See if we can get an initial camera started at the first frame.
char     g_initialCamera[256]      = "";
qboolean g_initialCameraStartBlack = qfalse;

/*
==============
CG_SetInitialCamera
==============
*/
void CG_SetInitialCamera(const char *name, qboolean startBlack)
{
	// Store this info to get reset after first snapshot inited
	strcpy(g_initialCamera, name);
	g_initialCameraStartBlack = startBlack;
}

/*
==============
CG_StartCamera
==============
*/
void CG_StartCamera(const char *name, qboolean startBlack)
{
	char lname[MAX_QPATH];

	COM_StripExtension(name, lname, sizeof(lname));
	strcat(lname, ".camera");

	if (trap_loadCamera(CAM_PRIMARY, va("cameras/%s", lname)))
	{
		cg.cameraMode = qtrue;                    // camera on in cgame
		if (startBlack)
		{
			CG_Fade(0, 0, 0, 255, cg.time, 0);    // go black
		}
		trap_Cvar_Set("cg_letterbox", "1");       // go letterbox
		//trap_SendClientCommand("startCamera");  // camera on in game
		trap_startCamera(CAM_PRIMARY, cg.time);   // camera on in client
	}
	else
	{
		// removed check for cams in main dir
		cg.cameraMode = qfalse;                 // camera off in cgame
		trap_SendClientCommand("stopCamera");   // camera off in game
		trap_stopCamera(CAM_PRIMARY);           // camera off in client
		CG_Fade(0, 0, 0, 0, cg.time, 0);        // ensure fadeup
		trap_Cvar_Set("cg_letterbox", "0");
		CG_Printf("Unable to load camera %s\n", lname);
	}
}

/*
==============
CG_StartInitialCamera
==============
*/
void CG_StartInitialCamera()
{
	// See if we've got a camera name
	if (g_initialCamera[0] != 0)
	{
		// Start a camera with the initial data we stored.
		CG_StartCamera(g_initialCamera, g_initialCameraStartBlack);

		// Clear it now so we don't get re-entrance problems
		g_initialCamera[0]        = 0;
		g_initialCameraStartBlack = qfalse;

	} // if (g_initialCamera[0] != 0)...
}

/*
==============
CG_StopCamera
==============
*/
void CG_StopCamera(void)
{
	cg.cameraMode = qfalse;                 // camera off in cgame
	trap_SendClientCommand("stopCamera");   // camera off in game
	trap_stopCamera(CAM_PRIMARY);           // camera off in client
	trap_Cvar_Set("cg_letterbox", "0");

	// fade back into world
	CG_Fade(0, 0, 0, 255, 0, 0);
	CG_Fade(0, 0, 0, 0, cg.time + 500, 2000);
}

static void CG_Fade_f(void)
{
	int   r, g, b, a;
	float duration;

	if (trap_Argc() < 6)
	{
		return;
	}

	r = atof(CG_Argv(1));
	g = atof(CG_Argv(2));
	b = atof(CG_Argv(3));
	a = atof(CG_Argv(4));

	duration = atof(CG_Argv(5)) * 1000;

	CG_Fade(r, g, b, a, cg.time, duration);
}

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

void CG_QuickFireteamMessage_f(void)
{
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
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

static void CG_QuickFireteams_f(void)
{
	if (cg.showFireteamMenu)
	{
		if (cgs.ftMenuMode == 0)
		{
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		else
		{
			cgs.ftMenuMode = 0;
		}
	}
	else if (CG_IsOnFireteam(cg.clientNum))
	{
		CG_EventHandling(CGAME_EVENT_FIRETEAMMSG, qfalse);
		cgs.ftMenuMode = 0;
	}
}

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

// say, team say, etc
static void CG_MessageMode_f(void)
{
	char cmd[64];

	if (cgs.eventHandling != CGAME_EVENT_NONE)
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
	else if (!Q_stricmp(cmd, "messagemode3"))
	{
		trap_Cvar_Set("cg_messageType", "3");
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

static void CG_MessageSend_f(void)
{
	char messageText[256];
	int  messageType;

	// get values
	trap_Cvar_VariableStringBuffer("cg_messageType", messageText, 256);
	messageType = atoi(messageText);
	trap_Cvar_VariableStringBuffer("cg_messageText", messageText, 256);

	// reset values
	trap_Cvar_Set("cg_messageText", "");
	trap_Cvar_Set("cg_messageType", "");
	trap_Cvar_Set("cg_messagePlayer", "");

	// don't send empty messages
	if (messageText[0] == '\0')
	{
		return;
	}

	if (messageType == 2) // team say
	{
		trap_SendConsoleCommand(va("say_team \"%s\"\n", messageText));
	}
	else if (messageType == 3) // fireteam say
	{
		trap_SendConsoleCommand(va("say_buddy \"%s\"\n", messageText));
	}
	else // normal say
	{
		trap_SendConsoleCommand(va("say \"%s\"\n", messageText));
	}
}

static void CG_SetWeaponCrosshair_f(void)
{
	char crosshair[64];

	trap_Argv(1, crosshair, 64);
	cg.newCrosshairIndex = atoi(crosshair) + 1;
}

static void CG_SelectBuddy_f(void)
{
	int          pos = atoi(CG_Argv(1));
	int          i;
	clientInfo_t *ci;

	// 0 - 5 = select that person
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
		if (!CG_IsOnFireteam(cg.clientNum))
		{
			break;     // we aren't a leader, so dont allow selection
		}

		ci = CG_SortedFireTeamPlayerForPosition(pos);
		if (!ci)
		{
			break;     // there was no-one in this position
		}

		ci->selected ^= qtrue;
		break;

	case -1:
		if (!CG_IsOnFireteam(cg.clientNum))
		{
			break;     // we aren't a leader, so dont allow selection
		}

		for (i = 0; i < 6; i++)
		{
			ci = CG_SortedFireTeamPlayerForPosition(i);
			if (!ci)
			{
				break;     // there was no-one in this position
			}

			ci->selected = qfalse;
		}
		break;

	case -2:
		if (!CG_IsOnFireteam(cg.clientNum))
		{
			break;     // we aren't a leader, so dont allow selection
		}

		for (i = 0; i < 6; i++)
		{
			ci = CG_SortedFireTeamPlayerForPosition(i);
			if (!ci)
			{
				break;     // there was no-one in this position
			}

			ci->selected = qtrue;
		}
		break;
	}
}

extern void CG_AdjustAutomapZoom(int zoomIn);

static void CG_AutomapZoomIn_f(void)
{
	if (!cgs.autoMapOff)
	{
		CG_AdjustAutomapZoom(qtrue);
	}
}

static void CG_AutomapZoomOut_f(void)
{
	if (!cgs.autoMapOff)
	{
		CG_AdjustAutomapZoom(qfalse);
	}
}

static void CG_AutomapExpandDown_f(void)
{
	if (!cgs.autoMapExpanded)
	{
		cgs.autoMapExpanded = qtrue;
		if (cg.time - cgs.autoMapExpandTime < 250.f)
		{
			cgs.autoMapExpandTime = cg.time - (250.f - (cg.time - cgs.autoMapExpandTime));
		}
		else
		{
			cgs.autoMapExpandTime = cg.time;
		}
	}
}

static void CG_AutomapExpandUp_f(void)
{
	if (cgs.autoMapExpanded)
	{
		cgs.autoMapExpanded = qfalse;
		if (cg.time - cgs.autoMapExpandTime < 250.f)
		{
			cgs.autoMapExpandTime = cg.time - (250.f - (cg.time - cgs.autoMapExpandTime));
		}
		else
		{
			cgs.autoMapExpandTime = cg.time;
		}
	}
}

static void CG_ToggleAutomap_f(void)
{
	cgs.autoMapOff = !cgs.autoMapOff;
}

const char *aMonths[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void CG_currentTime_f(void)
{
	qtime_t ct;

	trap_RealTime(&ct);
	CG_Printf("[cgnotify]Current time: ^3%02d:%02d:%02d (%02d %s %d)\n", ct.tm_hour, ct.tm_min, ct.tm_sec, ct.tm_mday, aMonths[ct.tm_mon], 1900 + ct.tm_year);
}

// Dynamically names a demo and sets up the recording
void CG_autoRecord_f(void)
{
	trap_SendConsoleCommand(va("record %s\n", CG_generateFilename()));
}

// Dynamically names a screenshot[JPEG]
void CG_autoScreenShot_f(void)
{
	trap_SendConsoleCommand(va("screenshot%s %s\n", ((cg_useScreenshotJPEG.integer) ? "JPEG" : ""), CG_generateFilename()));
}

void CG_vstrDown_f(void)
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

void CG_vstrUp_f(void)
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

void CG_keyOff_f(void)
{
	if (!cg.demoPlayback)
	{
		return;
	}
	CG_EventHandling(CGAME_EVENT_NONE, qfalse);
}

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

void CG_wStatsUp_f(void)
{
	cg.showStats = qfalse;
	CG_windowFree(cg.statsWindow);
	cg.statsWindow = NULL;
}

void CG_toggleSpecHelp_f(void)
{
#ifdef FEATURE_MULTIVIEW
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
#endif
}

static void CG_EditSpeakers_f(void)
{
	if (cg.editingSpeakers)
	{
		CG_DeActivateEditSoundMode();
	}
	else
	{
		const char *s = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "sv_cheats");

		if (s[0] != '1')
		{
			CG_Printf("editSpeakers is cheat protected.\n");
			return;
		}
		CG_ActivateEditSoundMode();
	}
}

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
	            wait = atoi( valueptr );
	        else if( !Q_stricmp( soundfile, "random" ) )
	            random = atoi( valueptr );
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

	memset(&speaker, 0, sizeof(speaker));

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

static void CG_ModifySpeaker_f(void)
{
	if (cg.editingSpeakers)
	{
		CG_ModifyEditSpeaker();
	}
}

static void CG_UndoSpeaker_f(void)
{
	if (cg.editingSpeakers)
	{
		CG_UndoEditSpeaker();
	}
}

void CG_ForceTapOut_f(void)
{
	trap_SendClientCommand("forcetapout");
}

/**
 * @brief Shows a popup message.
 */
static void CG_CPM_f(void)
{
	CG_AddPMItem(PM_MESSAGE, CG_Argv(1), cgs.media.voiceChatShader, NULL);
}

/**
 * @brief ETPro style enemy spawntimer
 */
void CG_TimerSet_f(void)
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
		spawnPeriod = atoi(buff);

		if (spawnPeriod == 0)
		{
			trap_Cvar_Set("cg_spawnTimer_set", "-1");
		}
		else if (spawnPeriod < 1 || spawnPeriod > 60)
		{
			CG_Printf("Argument must be a number between 1 and 60 - no argument will disable the spawn timer.\n");
		}
		else
		{
			int msec = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);

			trap_Cvar_Set("cg_spawnTimer_period", buff);
			trap_Cvar_Set("cg_spawnTimer_set", va("%d", msec / 1000));
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
void CG_ResetTimer_f(void)
{
	int msec;

	if (cgs.gamestate != GS_PLAYING)
	{
		CG_Printf("You may only use this command during the match.\n");
		return;
	}

	msec = (cgs.timelimit * 60.f * 1000.f) - (cg.time - cgs.levelStartTime);
	trap_Cvar_Set("cg_spawnTimer_set", va("%d", msec / 1000));
}

/**
 * @brief Sends an class setup message. Enables etpro like classscripts
 */
void CG_Class_f(void)
{
	char             cls[64];
	const char       *classtype, *teamstring;
	int              weapon1, weapon2, playerclass;
	bg_playerclass_t *classinfo;
	team_t           team;
	weaponType_t     *wt;

	if (cg.demoPlayback)
	{
		return;
	}

	team = cgs.clientinfo[cg.clientNum].team;

	if (team == TEAM_SPECTATOR)
	{
		return;
	}

	if (trap_Argc() < 2)
	{
		CG_Printf("Invalid command format.\n");
		return;
	}

	switch (team)
	{
	case TEAM_AXIS:
		classtype  = "r";
		teamstring = "Axis";
		break;
	case TEAM_ALLIES:
		classtype  = "b";
		teamstring = "Allies";
		break;
	default:
		CG_Printf("Invalid team.\n");
		return;
	}

	trap_Argv(1, cls, 64);

	if (!Q_stricmp(cls, "s"))
	{
		playerclass = PC_SOLDIER;
	}
	else if (!Q_stricmp(cls, "m"))
	{
		playerclass = PC_MEDIC;
	}
	else if (!Q_stricmp(cls, "e"))
	{
		playerclass = PC_ENGINEER;
	}
	else if (!Q_stricmp(cls, "f"))
	{
		playerclass = PC_FIELDOPS;
	}
	else if (!Q_stricmp(cls, "c"))
	{
		playerclass = PC_COVERTOPS;
	}
	else
	{
		CG_Printf("Invalid class format.\n");
		return;
	}

	classinfo = BG_GetPlayerClassInfo(team, playerclass);

	if (trap_Argc() > 2)
	{
		trap_Argv(2, cls, 64);
		weapon1 = atoi(cls);
		if (!classinfo->classWeapons[weapon1 - 1])
		{
			CG_Printf("Invalid command format for weapon.\n");
			return;
		}
	}
	else
	{
		weapon1 = 1;
	}

	if (cgs.clientinfo[cg.clientNum].skill[SK_HEAVY_WEAPONS] >= 4 && playerclass == PC_SOLDIER)
	{
		weapon2 = (team == TEAM_AXIS) ? WP_MP40 : WP_THOMPSON;
	}
	else if (cgs.clientinfo[cg.clientNum].skill[SK_LIGHT_WEAPONS] >= 4)
	{
		if (playerclass == PC_COVERTOPS)
		{
			weapon2 = (team == TEAM_AXIS) ? WP_AKIMBO_SILENCEDLUGER : WP_AKIMBO_SILENCEDCOLT;
		}
		else
		{
			weapon2 = (team == TEAM_AXIS) ? WP_AKIMBO_LUGER : WP_AKIMBO_COLT;
		}
	}
	else
	{
		if (playerclass == PC_COVERTOPS)
		{
			weapon2 = (team == TEAM_AXIS) ? WP_SILENCER : WP_SILENCED_COLT;
		}
		else
		{
			weapon2 = (team == TEAM_AXIS) ? WP_LUGER : WP_COLT;
		}
	}

	// Print out the selected class and weapon info
	wt = WM_FindWeaponTypeForWeapon(classinfo->classWeapons[weapon1 - 1]);
	CG_PriorityCenterPrint(va(CG_TranslateString("You will spawn as a %s %s with a %s."), teamstring, BG_ClassnameForNumber(playerclass), wt ? wt->desc : "^1UNKNOWN WEAPON"), SCREEN_HEIGHT - 88, SMALLCHAR_WIDTH, -1);
	// Send the switch command to the server
	trap_SendClientCommand(va("team %s %i %i %i\n", classtype, playerclass, classinfo->classWeapons[weapon1 - 1], weapon2));
}

void CG_ReadHuds_f(void)
{
	CG_ReadHudScripts();
}

static consoleCommand_t commands[] =
{
	{ "testgun",             CG_TestGun_f            },
	{ "testmodel",           CG_TestModel_f          },
	{ "nextframe",           CG_TestModelNextFrame_f },
	{ "prevframe",           CG_TestModelPrevFrame_f },
	{ "nextskin",            CG_TestModelNextSkin_f  },
	{ "prevskin",            CG_TestModelPrevSkin_f  },
	{ "viewpos",             CG_Viewpos_f            },
	{ "+scores",             CG_ScoresDown_f         },
	{ "-scores",             CG_ScoresUp_f           },
	{ "zoomin",              CG_ZoomIn_f             },
	{ "zoomout",             CG_ZoomOut_f            },
	{ "weaplastused",        CG_LastWeaponUsed_f     },
	{ "weapnextinbank",      CG_NextWeaponInBank_f   },
	{ "weapprevinbank",      CG_PrevWeaponInBank_f   },
	{ "weapnext",            CG_NextWeapon_f         },
	{ "weapprev",            CG_PrevWeapon_f         },
	{ "weapalt",             CG_AltWeapon_f          },
	{ "weapon",              CG_Weapon_f             },
	{ "weaponbank",          CG_WeaponBank_f         },
	{ "tell_target",         CG_TellTarget_f         },
	{ "tell_attacker",       CG_TellAttacker_f       },
	{ "tcmd",                CG_TargetCommand_f      },
	{ "fade",                CG_Fade_f               },

	{ "mp_QuickMessage",     CG_QuickMessage_f       },
	{ "mp_fireteammsg",      CG_QuickFireteams_f     },
	{ "mp_fireteamadmin",    CG_QuickFireteamAdmin_f },
	{ "wm_sayPlayerClass",   CG_SayPlayerClass_f     },
	{ "wm_ftsayPlayerClass", CG_FTSayPlayerClass_f   },


	{ "VoiceChat",           CG_VoiceChat_f          },
	{ "VoiceTeamChat",       CG_TeamVoiceChat_f      },

	// say, teamsay, etc
	{ "messageMode",         CG_MessageMode_f        },
	{ "messageMode2",        CG_MessageMode_f        },
	{ "messageMode3",        CG_MessageMode_f        },
	{ "messageSend",         CG_MessageSend_f        },

	{ "SetWeaponCrosshair",  CG_SetWeaponCrosshair_f },

	{ "VoiceFireTeamChat",   CG_BuddyVoiceChat_f     },

	{ "openlimbomenu",       CG_LimboMenu_f          },

	{ "+stats",              CG_StatsDown_f          },
	{ "-stats",              CG_StatsUp_f            },
	{ "+topshots",           CG_topshotsDown_f       },
	{ "-topshots",           CG_topshotsUp_f         },

	{ "autoRecord",          CG_autoRecord_f         },
	{ "autoScreenshot",      CG_autoScreenShot_f     },
	{ "currentTime",         CG_currentTime_f        },
	{ "keyoff",              CG_keyOff_f             },
	{ "keyon",               CG_keyOn_f              },
#ifdef FEATURE_MULTIVIEW
	{ "mvactivate",          CG_mvToggleAll_f        },
	{ "mvdel",               CG_mvDelete_f           },
	{ "mvhide",              CG_mvHideView_f         },
	{ "mvnew",               CG_mvNew_f              },
	{ "mvshow",              CG_mvShowView_f         },
	{ "mvswap",              CG_mvSwapViews_f        },
	{ "mvtoggle",            CG_mvToggleView_f       },
	{ "spechelp",            CG_toggleSpecHelp_f     },
#endif
	{ "statsdump",           CG_dumpStats_f          },
	{ "+vstr",               CG_vstrDown_f           },
	{ "-vstr",               CG_vstrUp_f             },

	{ "selectbuddy",         CG_SelectBuddy_f        },

	{ "MapZoomIn",           CG_AutomapZoomIn_f      },
	{ "MapZoomOut",          CG_AutomapZoomOut_f     },
	{ "+mapexpand",          CG_AutomapExpandDown_f  },
	{ "-mapexpand",          CG_AutomapExpandUp_f    },

	{ "generateTracemap",    CG_GenerateTracemap     },

	{ "ToggleAutoMap",       CG_ToggleAutomap_f      }, // toggle automap on/off

	{ "editSpeakers",        CG_EditSpeakers_f       },
	{ "dumpSpeaker",         CG_DumpSpeaker_f        },
	{ "modifySpeaker",       CG_ModifySpeaker_f      },
	{ "undoSpeaker",         CG_UndoSpeaker_f        },
	{ "cpm",                 CG_CPM_f                },
	{ "forcetapout",         CG_ForceTapOut_f        },
	{ "timerSet",            CG_TimerSet_f           },
	{ "resetTimer",          CG_ResetTimer_f         },
	{ "class",               CG_Class_f              },
	{ "readhuds",            CG_ReadHuds_f           },
};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand(void)
{
	const char *cmd;
	int        i;

	// don't allow console commands until a snapshot is present
	if (!cg.snap)
	{
		return qfalse;
	}

	cmd = CG_Argv(0);

	for (i = 0 ; i < sizeof(commands) / sizeof(commands[0]) ; i++)
	{
		if (!Q_stricmp(cmd, commands[i].cmd))
		{
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
 * Let the client system know about all of our commands so it can perform tab
 * completion
 */
void CG_InitConsoleCommands(void)
{
	int        i;
	const char *s;

	for (i = 0 ; i < sizeof(commands) / sizeof(commands[0]) ; i++)
	{
		trap_AddCommand(commands[i].cmd);
	}

	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("notarget");
	trap_AddCommand("noclip");
	trap_AddCommand("team");
	trap_AddCommand("follow");
	trap_AddCommand("setviewpos");
	trap_AddCommand("callvote");
	trap_AddCommand("vote");

	trap_AddCommand("nofatigue");

	trap_AddCommand("follownext");
	trap_AddCommand("followprev");

	trap_AddCommand("start_match");
	trap_AddCommand("reset_match");
	trap_AddCommand("swap_teams");

	trap_AddCommand("?");
	trap_AddCommand("bottomshots");
	trap_AddCommand("commands");
	trap_AddCommand("lock");
#ifdef FEATURE_MULTIVIEW
	trap_AddCommand("mvadd");
	trap_AddCommand("mvaxis");
	trap_AddCommand("mvallies");
	trap_AddCommand("mvall");
	trap_AddCommand("mvnone");
#endif
	trap_AddCommand("notready");
	trap_AddCommand("pause");
	trap_AddCommand("players");
	trap_AddCommand("readyteam");
	trap_AddCommand("ready");
	trap_AddCommand("ref");
	trap_AddCommand("say_teamnl");
	trap_AddCommand("say_team");
	trap_AddCommand("scores");
	trap_AddCommand("specinvite");
	trap_AddCommand("speclock");
	trap_AddCommand("specunlock");
	trap_AddCommand("statsall");
	trap_AddCommand("statsdump");
	trap_AddCommand("timein");
	trap_AddCommand("timeout");
	trap_AddCommand("topshots");
	trap_AddCommand("unlock");
	trap_AddCommand("unpause");
	trap_AddCommand("unready");
	trap_AddCommand("weaponstats");

	trap_AddCommand("fireteam");
	trap_AddCommand("showstats");

	trap_AddCommand("ignore");
	trap_AddCommand("unignore");

	trap_AddCommand("campaign");
	trap_AddCommand("listcampaigns");

	trap_AddCommand("imready");
	trap_AddCommand("say_buddy");
	trap_AddCommand("setspawnpt");
	trap_AddCommand("vsay");
	trap_AddCommand("vsay_buddy");
	trap_AddCommand("vsay_team");
	trap_AddCommand("where");
#ifdef FEATURE_LUA
	trap_AddCommand("lua_status");
#endif

	// remove engine commands to avoid abuse
	trap_RemoveCommand("+lookup");
	trap_RemoveCommand("-lookup");
	trap_RemoveCommand("+lookdown");
	trap_RemoveCommand("-lookdown");

	// only allow configstrings command when cheats enabled
	s = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO),
	                     "sv_cheats");
	if (s[0] != '1')
	{
		trap_RemoveCommand("configstrings");
	}
}

void CG_parseMapVoteListInfo()
{
	int i;
	cgs.dbNumMaps = (trap_Argc() - 2) / 4;

	if (atoi(CG_Argv(1)))
	{
		cgs.dbMapMultiVote = qtrue;
	}

	for (i = 0; i < cgs.dbNumMaps; i++)
	{
		Q_strncpyz(cgs.dbMaps[i], CG_Argv((i * 4) + 2),
		           sizeof(cgs.dbMaps[0]));
		cgs.dbMapVotes[i]      = 0;
		cgs.dbMapID[i]         = atoi(CG_Argv((i * 4) + 3));
		cgs.dbMapLastPlayed[i] = atoi(CG_Argv((i * 4) + 4));
		cgs.dbMapTotalVotes[i] = atoi(CG_Argv((i * 4) + 5));
		if (CG_FindArenaInfo(va("scripts/%s.arena", cgs.dbMaps[i]),
		                     cgs.dbMaps[i], &cgs.arenaData))
		{
			Q_strncpyz(cgs.dbMapDispName[i],
			           cgs.arenaData.longname,
			           sizeof(cgs.dbMaps[0]));
		}
		else
		{
			Q_strncpyz(cgs.dbMapDispName[i],
			           cgs.dbMaps[i],
			           sizeof(cgs.dbMaps[0]));
		}
	}

	CG_LocateArena();

	cgs.dbMapListReceived = qtrue;

	return;
}
