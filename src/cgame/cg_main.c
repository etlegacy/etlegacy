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
 * @file cg_main.c
 * @brief Initialization and primary entry point for cgame
 */

#include "cg_local.h"

displayContextDef_t cgDC;

void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback, int etLegacyClient, demoPlayInfo_t *info, int clientVersion);
void CG_Shutdown(void);
qboolean CG_CheckExecKey(int key);
extern itemDef_t *g_bindItem;
extern qboolean  g_waitingForKey;

/**
 * @brief This is the only way control passes into the module.
 * @details This must be the very first function compiled into the .q3vm file
 * @param command
 * @param[in] arg0
 * @param[in] arg1
 * @param[in] arg2
 * @param[in] arg3
 * @param[in] arg4
 * @param[in] arg5
 * @param[in] arg6
 * @param[in] arg7 - unused
 * @param[in] arg8 - unused
 * @param[in] arg9 - unused
 * @param[in] arg10 - unused
 * @param[in] arg11 - unused
 * @return
 */
Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11)
{
	switch (command)
	{
	case CG_INIT:
		CG_Init(arg0, arg1, arg2, (qboolean)arg3, arg4, (demoPlayInfo_t *)arg5, arg6);
		cgs.initing = qfalse;
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame(arg0, (qboolean)arg2); // arg1 removed, order kept for vanilla client compatibility
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, (qboolean)arg1);
		return 0;
	case CG_MOUSE_EVENT:
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
		// when the limbopanel is open, we want the cursor to be able to move all the way to the right edge of the screen..
		// ... same for the debriefing screen
		if (cg.showGameView || cgs.dbShowing)
		{
			if (!Ccg_Is43Screen())
			{
				cgDC.cursorx *= cgs.adr43;
			}
		}
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0, qtrue);
		return 0;
	case CG_GET_TAG:
		return CG_GetTag(arg0, (char *)arg1, (orientation_t *)arg2);
	case CG_CHECKEXECKEY:
		return CG_CheckExecKey(arg0);
	case CG_WANTSBINDKEYS:
		return (g_waitingForKey && g_bindItem) ? qtrue : qfalse;
	case CG_MESSAGERECEIVED:
		return -1;
	default:
		CG_Error("vmMain: unknown command %li", (long)command);
		break;
	}
	return -1;
}

cg_t         cg;
cgs_t        cgs;
centity_t    cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];

vmCvar_t cg_centertime;
vmCvar_t cg_bobbing;
vmCvar_t cg_swingSpeed;
vmCvar_t cg_shadows;
vmCvar_t cg_gibs;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawPing;
vmCvar_t cg_lagometer;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_drawCrosshairInfo;
vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_drawCrosshairPickups;
vmCvar_t cg_drawSpectatorNames;
vmCvar_t cg_weaponCycleDelay;
vmCvar_t cg_cycleAllWeaps;
vmCvar_t cg_useWeapsForZoom;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_voiceChats;
vmCvar_t cg_voiceText;
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_drawSpreadScale;
vmCvar_t cg_railTrailTime;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_markTime;
vmCvar_t cg_brassTime;
vmCvar_t cg_letterbox;
vmCvar_t cg_drawGun;
vmCvar_t cg_cursorHints;
vmCvar_t cg_gun_frame;
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_tracerChance;
vmCvar_t cg_tracerWidth;
vmCvar_t cg_tracerLength;
vmCvar_t cg_tracerSpeed;
vmCvar_t cg_autoswitch;
vmCvar_t cg_fov;
vmCvar_t cg_muzzleFlash;
vmCvar_t cg_zoomStepSniper;
vmCvar_t cg_zoomDefaultSniper;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
#ifdef ALLOW_GSYNC
vmCvar_t cg_synchronousClients;
#endif // ALLOW_GSYNC
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatHeight;
vmCvar_t cg_teamChatMention;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_coronafardist;
vmCvar_t cg_coronas;
vmCvar_t cg_paused;
vmCvar_t cg_blood;
vmCvar_t cg_predictItems;
vmCvar_t cg_drawEnvAwareness;

vmCvar_t cg_autoactivate;

vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

vmCvar_t cg_gameType;
vmCvar_t cg_bloodTime;
vmCvar_t cg_skybox;

// say, team say, etc.
vmCvar_t cg_messageType;

vmCvar_t cg_timescale;

vmCvar_t cg_voiceSpriteTime;

vmCvar_t cg_drawCompass;
vmCvar_t cg_drawNotifyText;
vmCvar_t cg_quickMessageAlt;
vmCvar_t cg_descriptiveText;

vmCvar_t cg_redlimbotime;
vmCvar_t cg_bluelimbotime;

vmCvar_t cg_antilag;

vmCvar_t developer;

vmCvar_t authLevel;

vmCvar_t cf_wstats;                     // Font scale for +wstats window
vmCvar_t cf_wtopshots;                  // Font scale for +wtopshots window

vmCvar_t cg_autoFolders;
vmCvar_t cg_autoAction;
vmCvar_t cg_autoReload;
vmCvar_t cg_bloodDamageBlend;
vmCvar_t cg_bloodFlash;
vmCvar_t cg_bloodFlashTime;
vmCvar_t cg_complaintPopUp;
vmCvar_t cg_crosshairAlpha;
vmCvar_t cg_crosshairAlphaAlt;
vmCvar_t cg_crosshairColor;
vmCvar_t cg_crosshairColorAlt;
vmCvar_t cg_crosshairPulse;
vmCvar_t cg_drawReinforcementTime;
vmCvar_t cg_drawWeaponIconFlash;
vmCvar_t cg_noAmmoAutoSwitch;
vmCvar_t cg_printObjectiveInfo;
#ifdef FEATURE_MULTIVIEW
vmCvar_t cg_specHelp;
#endif
vmCvar_t cg_uinfo;

vmCvar_t demo_avifpsF1;
vmCvar_t demo_avifpsF2;
vmCvar_t demo_avifpsF3;
vmCvar_t demo_avifpsF4;
vmCvar_t demo_avifpsF5;
vmCvar_t demo_drawTimeScale;
vmCvar_t demo_infoWindow;

#ifdef FEATURE_MULTIVIEW
vmCvar_t mv_sensitivity;
#endif

#ifdef FEATURE_EDV
vmCvar_t demo_weaponcam;
vmCvar_t demo_followDistance;
vmCvar_t demo_yawPitchRollSpeed;
vmCvar_t demo_freecamspeed;
vmCvar_t demo_nopitch;
vmCvar_t demo_pvshint;
vmCvar_t demo_lookat;
vmCvar_t demo_autotimescale;
vmCvar_t demo_autotimescaleweapons;
vmCvar_t demo_teamonlymissilecam;
vmCvar_t cg_predefineddemokeys;
#endif

vmCvar_t int_cl_maxpackets;
vmCvar_t int_cl_timenudge;
vmCvar_t int_m_pitch;
vmCvar_t int_sensitivity;
vmCvar_t int_ui_blackout;

vmCvar_t cg_rconPassword;
vmCvar_t cg_refereePassword;
vmCvar_t cg_atmosphericEffects;

vmCvar_t cg_drawRoundTimer;

vmCvar_t cg_instanttapout;

vmCvar_t cg_debugSkills;
vmCvar_t cg_drawFireteamOverlay;
vmCvar_t cg_drawSmallPopupIcons;

// demo recording cvars
vmCvar_t cl_demorecording;
vmCvar_t cl_demofilename;
vmCvar_t cl_demooffset;
// wav recording cvars
vmCvar_t cl_waverecording;
vmCvar_t cl_wavefilename;
vmCvar_t cl_waveoffset;
vmCvar_t cg_recording_statusline;

vmCvar_t cg_announcer;
vmCvar_t cg_hitSounds;
vmCvar_t cg_locations;

vmCvar_t cg_spawnTimer_set;         // spawntimer
vmCvar_t cg_spawnTimer_period;      // spawntimer

vmCvar_t cg_logFile;

vmCvar_t cg_countryflags; // GeoIP

vmCvar_t cg_altHud;
vmCvar_t cg_altHudFlags;
vmCvar_t cg_tracers;
vmCvar_t cg_fireteamLatchedClass;

vmCvar_t cg_weapaltReloads;

vmCvar_t cg_simpleItems;

vmCvar_t cg_automapZoom;

vmCvar_t cg_drawTime;

vmCvar_t cg_popupFadeTime;
vmCvar_t cg_popupStayTime;
vmCvar_t cg_popupFilter;
vmCvar_t cg_popupBigFilter;
vmCvar_t cg_graphicObituaries;

vmCvar_t cg_fontScaleTP; // top print
vmCvar_t cg_fontScaleSP; // side print
vmCvar_t cg_fontScaleCP; // center print
vmCvar_t cg_fontScaleCN; // crosshair name

// unlagged optimized prediction
vmCvar_t cg_optimizePrediction;
vmCvar_t cg_debugPlayerHitboxes;

#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
// ratings scoreboard
vmCvar_t cg_scoreboard;
#endif

vmCvar_t cg_quickchat;

vmCvar_t cg_drawspeed;

vmCvar_t cg_visualEffects;
vmCvar_t cg_bannerTime;

vmCvar_t cg_shoutcastDrawPlayers;
vmCvar_t cg_shoutcastDrawTeamNames;
vmCvar_t cg_shoutcastTeamName1;
vmCvar_t cg_shoutcastTeamName2;
vmCvar_t cg_shoutcastDrawHealth;
vmCvar_t cg_shoutcastGrenadeTrail;
vmCvar_t cg_shoutcastDrawMinimap;

typedef struct
{
	vmCvar_t *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int cvarFlags;
	int modificationCount;
} cvarTable_t;

static cvarTable_t cvarTable[] =
{
	{ &cg_autoswitch,             "cg_autoswitch",             "2",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawGun,                "cg_drawGun",                "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_gun_frame,              "cg_gun_frame",              "0",           CVAR_TEMP,                    0 },
	{ &cg_cursorHints,            "cg_cursorHints",            "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_zoomDefaultSniper,      "cg_zoomDefaultSniper",      "20",          CVAR_ARCHIVE,                 0 },   // changed per atvi req
	{ &cg_zoomStepSniper,         "cg_zoomStepSniper",         "2",           CVAR_ARCHIVE,                 0 },
	{ &cg_fov,                    "cg_fov",                    "90",          CVAR_ARCHIVE,                 0 },
	{ &cg_muzzleFlash,            "cg_muzzleFlash",            "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_letterbox,              "cg_letterbox",              "0",           CVAR_TEMP,                    0 },
	{ &cg_shadows,                "cg_shadows",                "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_gibs,                   "cg_gibs",                   "1",           CVAR_ARCHIVE,                 0 },
	// we now draw reticles always in non demoplayback
	//  { &cg_draw2D, "cg_draw2D", "1", CVAR_CHEAT }, // JPW NERVE changed per atvi req to prevent sniper rifle zoom cheats
	{ &cg_draw2D,                 "cg_draw2D",                 "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawSpreadScale,        "cg_drawSpreadScale",        "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_railTrailTime,          "cg_railTrailTime",          "50",          CVAR_ARCHIVE,                 0 },
	{ &cg_drawStatus,             "cg_drawStatus",             "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawFPS,                "cg_drawFPS",                "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawPing,               "cg_drawPing",               "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_lagometer,              "cg_lagometer",              "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawSnapshot,           "cg_drawSnapshot",           "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshair,          "cg_drawCrosshair",          "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshairInfo,      "cg_drawCrosshairInfo",      "7",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshairNames,     "cg_drawCrosshairNames",     "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshairPickups,   "cg_drawCrosshairPickups",   "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawSpectatorNames,     "cg_drawSpectatorNames",     "2",           CVAR_ARCHIVE,                 0 },
	{ &cg_useWeapsForZoom,        "cg_useWeapsForZoom",        "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_weaponCycleDelay,       "cg_weaponCycleDelay",       "150",         CVAR_ARCHIVE,                 0 },
	{ &cg_cycleAllWeaps,          "cg_cycleAllWeaps",          "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairSize,          "cg_crosshairSize",          "48",          CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairHealth,        "cg_crosshairHealth",        "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairX,             "cg_crosshairX",             "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairY,             "cg_crosshairY",             "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_brassTime,              "cg_brassTime",              "2500",        CVAR_ARCHIVE,                 0 },
	{ &cg_markTime,               "cg_markTime",               "20000",       CVAR_ARCHIVE,                 0 },
	{ &cg_gun_x,                  "cg_gunX",                   "0",           CVAR_TEMP,                    0 },
	{ &cg_gun_y,                  "cg_gunY",                   "0",           CVAR_TEMP,                    0 },
	{ &cg_gun_z,                  "cg_gunZ",                   "0",           CVAR_TEMP,                    0 },
	{ &cg_centertime,             "cg_centertime",             "5",           CVAR_ARCHIVE,                 0 },   // changed from 3 to 5
	{ &cg_bobbing,                "cg_bobbing",                "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawEnvAwareness,       "cg_drawEnvAwareness",       "1",           CVAR_ARCHIVE,                 0 },

	{ &cg_autoactivate,           "cg_autoactivate",           "1",           CVAR_ARCHIVE,                 0 },

	// more fluid rotations
	{ &cg_swingSpeed,             "cg_swingSpeed",             "0.1",         CVAR_CHEAT,                   0 },   // was 0.3 for Q3
	{ &cg_bloodTime,              "cg_bloodTime",              "120",         CVAR_ARCHIVE,                 0 },

	{ &cg_skybox,                 "cg_skybox",                 "1",           CVAR_CHEAT,                   0 },

	// say, team say, etc.
	{ &cg_messageType,            "cg_messageType",            "1",           CVAR_TEMP,                    0 },

	{ &cg_animSpeed,              "cg_animspeed",              "1",           CVAR_CHEAT,                   0 },
	{ &cg_debugAnim,              "cg_debuganim",              "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugPosition,          "cg_debugposition",          "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugEvents,            "cg_debugevents",            "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugPlayerHitboxes,    "cg_debugPlayerHitboxes",    "0",           CVAR_CHEAT,                   0 },
	{ &cg_errorDecay,             "cg_errordecay",             "100",         0,                            0 },
	{ &cg_nopredict,              "cg_nopredict",              "0",           CVAR_CHEAT,                   0 },
	{ &cg_noPlayerAnims,          "cg_noplayeranims",          "0",           CVAR_CHEAT,                   0 },
	{ &cg_showmiss,               "cg_showmiss",               "0",           0,                            0 },
	{ &cg_tracerChance,           "cg_tracerchance",           "0.4",         CVAR_CHEAT,                   0 },
	{ &cg_tracerWidth,            "cg_tracerwidth",            "0.8",         CVAR_CHEAT,                   0 },
	{ &cg_tracerSpeed,            "cg_tracerSpeed",            "4500",        CVAR_CHEAT,                   0 },
	{ &cg_tracerLength,           "cg_tracerlength",           "160",         CVAR_CHEAT,                   0 },
	{ &cg_thirdPersonRange,       "cg_thirdPersonRange",       "80",          CVAR_CHEAT,                   0 },   // per atvi req
	{ &cg_thirdPersonAngle,       "cg_thirdPersonAngle",       "0",           CVAR_CHEAT,                   0 },
	{ &cg_thirdPerson,            "cg_thirdPerson",            "0",           CVAR_CHEAT,                   0 },   // per atvi req
	{ &cg_teamChatTime,           "cg_teamChatTime",           "8000",        CVAR_ARCHIVE,                 0 },
	{ &cg_teamChatHeight,         "cg_teamChatHeight",         "8",           CVAR_ARCHIVE,                 0 },
	{ &cg_teamChatMention,        "cg_teamChatMention",        "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_coronafardist,          "cg_coronafardist",          "1536",        CVAR_ARCHIVE,                 0 },
	{ &cg_coronas,                "cg_coronas",                "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_predictItems,           "cg_predictItems",           "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_stats,                  "cg_stats",                  "0",           0,                            0 },

	{ &cg_timescale,              "timescale",                 "1",           0,                            0 },

	{ &pmove_fixed,               "pmove_fixed",               "0",           CVAR_SYSTEMINFO,              0 },
	{ &pmove_msec,                "pmove_msec",                "8",           CVAR_SYSTEMINFO,              0 },

	{ &cg_voiceSpriteTime,        "cg_voiceSpriteTime",        "6000",        CVAR_ARCHIVE,                 0 },

	{ &cg_teamChatsOnly,          "cg_teamChatsOnly",          "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_voiceChats,             "cg_voiceChats",             "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_voiceText,              "cg_voiceText",              "1",           CVAR_ARCHIVE,                 0 },

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_buildScript,            "com_buildScript",           "0",           0,                            0 },   // force loading of all possible data and error on failures
	{ &cg_paused,                 "cl_paused",                 "0",           CVAR_ROM,                     0 },

	{ &cg_blood,                  "cg_showblood",              "1",           CVAR_ARCHIVE,                 0 },
#ifdef ALLOW_GSYNC
	{ &cg_synchronousClients,     "g_synchronousClients",      "0",           CVAR_SYSTEMINFO | CVAR_CHEAT, 0 },   // communicated by systeminfo
#endif // ALLOW_GSYNC

	{ &cg_gameType,               "g_gametype",                "0",           0,                            0 },   // communicated by systeminfo
	{ &cg_bluelimbotime,          "",                          "30000",       0,                            0 },   // communicated by systeminfo
	{ &cg_redlimbotime,           "",                          "30000",       0,                            0 },   // communicated by systeminfo
	{ &cg_drawCompass,            "cg_drawCompass",            "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawNotifyText,         "cg_drawNotifyText",         "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_quickMessageAlt,        "cg_quickMessageAlt",        "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_descriptiveText,        "cg_descriptiveText",        "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_antilag,                "g_antilag",                 "1",           0,                            0 },
	{ &developer,                 "developer",                 "0",           CVAR_CHEAT,                   0 },
	{ &cf_wstats,                 "cf_wstats",                 "1.2",         CVAR_ARCHIVE,                 0 },
	{ &cf_wtopshots,              "cf_wtopshots",              "1.0",         CVAR_ARCHIVE,                 0 },

	{ &cg_autoFolders,            "cg_autoFolders",            "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_autoAction,             "cg_autoAction",             "4",           CVAR_ARCHIVE,                 0 },
	{ &cg_autoReload,             "cg_autoReload",             "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_bloodDamageBlend,       "cg_bloodDamageBlend",       "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_bloodFlash,             "cg_bloodFlash",             "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_bloodFlashTime,         "cg_bloodFlashTime",         "1500",        CVAR_ARCHIVE,                 0 },
	{ &cg_complaintPopUp,         "cg_complaintPopUp",         "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairAlpha,         "cg_crosshairAlpha",         "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairAlphaAlt,      "cg_crosshairAlphaAlt",      "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairColor,         "cg_crosshairColor",         "White",       CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairColorAlt,      "cg_crosshairColorAlt",      "White",       CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairPulse,         "cg_crosshairPulse",         "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawReinforcementTime,  "cg_drawReinforcementTime",  "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawWeaponIconFlash,    "cg_drawWeaponIconFlash",    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_noAmmoAutoSwitch,       "cg_noAmmoAutoSwitch",       "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_printObjectiveInfo,     "cg_printObjectiveInfo",     "1",           CVAR_ARCHIVE,                 0 },
#ifdef FEATURE_MULTIVIEW
	{ &cg_specHelp,               "cg_specHelp",               "1",           CVAR_ARCHIVE,                 0 },
#endif
	{ &cg_uinfo,                  "cg_uinfo",                  "0",           CVAR_ROM | CVAR_USERINFO,     0 },

	{ &demo_avifpsF1,             "demo_avifpsF1",             "0",           CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF2,             "demo_avifpsF2",             "10",          CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF3,             "demo_avifpsF3",             "15",          CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF4,             "demo_avifpsF4",             "20",          CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF5,             "demo_avifpsF5",             "24",          CVAR_ARCHIVE,                 0 },
	{ &demo_drawTimeScale,        "demo_drawTimeScale",        "1",           CVAR_ARCHIVE,                 0 },
	{ &demo_infoWindow,           "demo_infoWindow",           "1",           CVAR_ARCHIVE,                 0 },

#ifdef FEATURE_EDV
	{ &demo_weaponcam,            "demo_weaponcam",            "0",           CVAR_ARCHIVE,                 0 },

	{ &demo_followDistance,       "demo_followDistance",       "50 0 20",     CVAR_ARCHIVE,                 0 },

	{ &demo_yawPitchRollSpeed,    "demo_yawPitchRollSpeed",    "140 140 140", CVAR_ARCHIVE,                 0 },

	{ &demo_freecamspeed,         "demo_freecamspeed",         "800",         CVAR_ARCHIVE,                 0 },
	{ &demo_nopitch,              "demo_nopitch",              "1",           CVAR_ARCHIVE,                 0 },
	{ &demo_pvshint,              "demo_pvshint",              "0",           CVAR_ARCHIVE,                 0 },
	{ &demo_lookat,               "demo_lookat",               "-1",          CVAR_CHEAT,                   0 },
	{ &demo_autotimescaleweapons, "demo_autotimescaleweapons", "0",           CVAR_ARCHIVE,                 0 },
	{ &demo_autotimescale,        "demo_autotimescale",        "1",           CVAR_ARCHIVE,                 0 },
	{ &demo_teamonlymissilecam,   "demo_teamonlymissilecam",   "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_predefineddemokeys,     "cg_predefineddemokeys",     "1",           CVAR_CHEAT | CVAR_ARCHIVE,    0 },
#endif

#ifdef FEATURE_MULTIVIEW
	{ &mv_sensitivity,            "mv_sensitivity",            "20",          CVAR_ARCHIVE,                 0 },
#endif

	// Engine mappings
	{ &int_cl_maxpackets,         "cl_maxpackets",             "125",         CVAR_ARCHIVE,                 0 },
	{ &int_cl_timenudge,          "cl_timenudge",              "0",           CVAR_ARCHIVE,                 0 },
	{ &int_m_pitch,               "m_pitch",                   "0.022",       CVAR_ARCHIVE,                 0 },
	{ &int_sensitivity,           "sensitivity",               "5",           CVAR_ARCHIVE,                 0 },
	{ &int_ui_blackout,           "ui_blackout",               "0",           CVAR_ROM,                     0 },

	{ &cg_atmosphericEffects,     "cg_atmosphericEffects",     "1",           CVAR_ARCHIVE,                 0 },
	{ &authLevel,                 "authLevel",                 "0",           CVAR_TEMP | CVAR_ROM,         0 },

	{ &cg_rconPassword,           "auth_rconPassword",         "",            CVAR_TEMP,                    0 },
	{ &cg_refereePassword,        "auth_refereePassword",      "",            CVAR_TEMP,                    0 },

	{ &cg_drawRoundTimer,         "cg_drawRoundTimer",         "1",           CVAR_ARCHIVE,                 0 },

	{ &cg_instanttapout,          "cg_instanttapout",          "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_debugSkills,            "cg_debugSkills",            "0",           0,                            0 },
	{ NULL,                       "cg_etVersion",              "",            CVAR_USERINFO | CVAR_ROM,     0 },
#if 0
	{ NULL,                       "cg_legacyVersion",          "",            CVAR_USERINFO | CVAR_ROM,     0 },
#endif
	{ &cg_drawFireteamOverlay,    "cg_drawFireteamOverlay",    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawSmallPopupIcons,    "cg_drawSmallPopupIcons",    "1",           CVAR_ARCHIVE,                 0 },

	// demo recording cvars
	{ &cl_demorecording,          "cl_demorecording",          "0",           CVAR_ROM,                     0 },
	{ &cl_demofilename,           "cl_demofilename",           "",            CVAR_ROM,                     0 },
	{ &cl_demooffset,             "cl_demooffset",             "0",           CVAR_ROM,                     0 },
	// wav recording cvars
	{ &cl_waverecording,          "cl_waverecording",          "0",           CVAR_ROM,                     0 },
	{ &cl_wavefilename,           "cl_wavefilename",           "",            CVAR_ROM,                     0 },
	{ &cl_waveoffset,             "cl_waveoffset",             "0",           CVAR_ROM,                     0 },
	{ &cg_recording_statusline,   "cg_recording_statusline",   "9",           CVAR_ARCHIVE,                 0 },

	{ &cg_announcer,              "cg_announcer",              "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_hitSounds,              "cg_hitSounds",              "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_locations,              "cg_locations",              "3",           CVAR_ARCHIVE,                 0 },

	{ &cg_spawnTimer_set,         "cg_spawnTimer_set",         "-1",          CVAR_TEMP,                    0 },

	{ &cg_spawnTimer_period,      "cg_spawnTimer_period",      "0",           CVAR_TEMP,                    0 },

	{ &cg_logFile,                "cg_logFile",                "",            CVAR_ARCHIVE,                 0 },   // we don't log the chats per default

	{ &cg_countryflags,           "cg_countryflags",           "1",           CVAR_ARCHIVE,                 0 },   // GeoIP
	{ &cg_altHud,                 "cg_altHud",                 "0",           CVAR_ARCHIVE,                 0 },   // Hudstyles
	{ &cg_altHudFlags,            "cg_altHudFlags",            "0",           CVAR_ARCHIVE,                 0 },   // Hudstyles
	{ &cg_tracers,                "cg_tracers",                "1",           CVAR_ARCHIVE,                 0 },   // Draw tracers
	{ &cg_fireteamLatchedClass,   "cg_fireteamLatchedClass",   "1",           CVAR_ARCHIVE,                 0 },   // Draw fireteam members latched class
	{ &cg_simpleItems,            "cg_simpleItems",            "0",           CVAR_ARCHIVE,                 0 },   // Bugged atm
	{ &cg_automapZoom,            "cg_automapZoom",            "5.159",       CVAR_ARCHIVE,                 0 },
	{ &cg_drawTime,               "cg_drawTime",               "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_popupFadeTime,          "cg_popupFadeTime",          "2500",        CVAR_ARCHIVE,                 0 },
	{ &cg_popupStayTime,          "cg_popupStayTime",          "2000",        CVAR_ARCHIVE,                 0 },
	{ &cg_popupFilter,            "cg_popupFilter",            "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_popupBigFilter,         "cg_popupBigFilter",         "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_graphicObituaries,      "cg_graphicObituaries",      "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_weapaltReloads,         "cg_weapaltReloads",         "0",           CVAR_ARCHIVE,                 0 },

	// Fonts
	{ &cg_fontScaleTP,            "cg_fontScaleTP",            "0.35",        CVAR_ARCHIVE,                 0 },   // TopPrint
	{ &cg_fontScaleSP,            "cg_fontScaleSP",            "0.22",        CVAR_ARCHIVE,                 0 },   // SidePrint
	{ &cg_fontScaleCP,            "cg_fontScaleCP",            "0.22",        CVAR_ARCHIVE,                 0 },   // CenterPrint
	{ &cg_fontScaleCN,            "cg_fontScaleCN",            "0.25",        CVAR_ARCHIVE,                 0 },   // CrossName

	{ &cg_optimizePrediction,     "cg_optimizePrediction",     "1",           CVAR_ARCHIVE,                 0 },   // unlagged optimized prediction

	{ &cg_scoreboard,             "cg_scoreboard",             "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_quickchat,              "cg_quickchat",              "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_drawspeed,              "cg_drawspeed",              "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_visualEffects,          "cg_visualEffects",          "1",           CVAR_ARCHIVE,                 0 },    // Draw visual effects (i.e : airstrike plane, debris ...)
	{ &cg_bannerTime,             "cg_bannerTime",             "10000",       CVAR_ARCHIVE,                 0 },

	{ &cg_visualEffects,          "cg_visualEffects",          "1",           CVAR_ARCHIVE,                 0 },   // Draw visual effects (i.e : airstrike plane, debris ...)
	{ &cg_bannerTime,             "cg_bannerTime",             "10000",       CVAR_ARCHIVE,                 0 },

	{ &cg_shoutcastDrawPlayers,   "cg_shoutcastDrawPlayers",   "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastDrawTeamNames, "cg_shoutcastDrawTeamNames", "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastTeamName1,     "cg_shoutcastTeamName1",     "",            CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastTeamName2,     "cg_shoutcastTeamName2",     "",            CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastDrawHealth,    "cg_shoutcastDrawHealth",    "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastGrenadeTrail,  "cg_shoutcastGrenadeTrail",  "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastDrawMinimap,   "cg_shoutcastDrawMinimap",   "1",           CVAR_ARCHIVE,                 0 },
};

static const unsigned int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);
static qboolean           cvarsLoaded   = qfalse;
void CG_setClientFlags(void);

/**
 * @brief CG_RegisterCvars
 */
void CG_RegisterCvars(void)
{
	unsigned int i;
	cvarTable_t  *cv;
	char         var[MAX_TOKEN_CHARS];

	CG_Printf("%d client cvars in use\n", cvarTableSize);

	trap_Cvar_Set("cg_letterbox", "0");   // force this for people who might have it in their cfg

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar != NULL)
		{
			// force the update to range check this cvar on first run
			if (cv->vmCvar == &cg_errorDecay)
			{
				cv->modificationCount = !cv->vmCvar->modificationCount;
			}
			else
			{
				cv->modificationCount = cv->vmCvar->modificationCount;
			}
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = (qboolean)(!!Q_atoi(var));

	// um, here, why?
	CG_setClientFlags();
	BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
	BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");

	cvarsLoaded = qtrue;
}

/**
 * @brief CG_UpdateCvars
 */
void CG_UpdateCvars(void)
{
	unsigned int i;
	qboolean     fSetFlags = qfalse;
	cvarTable_t  *cv;

	if (!cvarsLoaded)
	{
		return;
	}

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		if (cv->vmCvar)
		{
			trap_Cvar_Update(cv->vmCvar);
			if (cv->modificationCount != cv->vmCvar->modificationCount)
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				// Check if we need to update any client flags to be sent to the server
				if (cv->vmCvar == &cg_autoAction || cv->vmCvar == &cg_autoReload ||
				    cv->vmCvar == &int_cl_timenudge || cv->vmCvar == &int_cl_maxpackets ||
				    cv->vmCvar == &cg_autoactivate || cv->vmCvar == &cg_predictItems)
				{
					fSetFlags = qtrue;
				}
				else if (cv->vmCvar == &cg_crosshairColor || cv->vmCvar == &cg_crosshairAlpha)
				{
					BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
				}
				else if (cv->vmCvar == &cg_crosshairColorAlt || cv->vmCvar == &cg_crosshairAlphaAlt)
				{
					BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");
				}
				else if (cv->vmCvar == &cg_rconPassword && *cg_rconPassword.string)
				{
					trap_SendConsoleCommand(va("rconAuth %s", cg_rconPassword.string));
				}
				else if (cv->vmCvar == &cg_refereePassword && *cg_refereePassword.string)
				{
					trap_SendConsoleCommand(va("ref %s", cg_refereePassword.string));
				}
				else if (cv->vmCvar == &demo_infoWindow)
				{
					if (demo_infoWindow.integer == 0 && cg.demohelpWindow == SHOW_ON)
					{
						CG_ShowHelp_On(&cg.demohelpWindow);
					}
					else if (demo_infoWindow.integer > 0 && cg.demohelpWindow != SHOW_ON)
					{
						CG_ShowHelp_On(&cg.demohelpWindow);
					}
				}
				else if (cv->vmCvar == &cg_errorDecay)
				{
					// cap errordecay because
					// prediction is EXTREMELY broken
					// right now.
					if (cg_errorDecay.value < 0.0f)
					{
						trap_Cvar_Set("cg_errorDecay", "0");
					}
					else if (cg_errorDecay.value > 500.0f)
					{
						trap_Cvar_Set("cg_errorDecay", "500");
					}
				}
			}
		}
	}

	// Send any relevent updates
	if (fSetFlags)
	{
		CG_setClientFlags();
	}
}

/**
 * @brief CG_RestoreProfile
 */
void CG_RestoreProfile(void)
{
	int i;

	for (i = 0; i < cg.cvarBackupsCount; ++i)
	{
		if (i == 0)
		{
			CG_Printf(S_COLOR_GREEN "Restoring CVARS forced by server\n");
		}
		trap_Cvar_Set(cg.cvarBackups[i].cvarName, cg.cvarBackups[i].cvarValue);
		CG_Printf(S_COLOR_YELLOW "cvar: %s %s\n", cg.cvarBackups[i].cvarName, cg.cvarBackups[i].cvarValue);
	}
}

/**
 * @brief Try to exec a cfg file if it is found
 * @param[in] filename
 * @return
 */
qboolean CG_execFile(const char *filename)
{
	int handle = trap_PC_LoadSource(va("%s.cfg", filename));

	trap_PC_FreeSource(handle);

	if (!handle)
	{
		// file not found
		return qfalse;
	}

	trap_SendConsoleCommand(va("exec %s.cfg\n", filename));

	return qtrue;
}

/**
 * @brief CG_setClientFlags
 */
void CG_setClientFlags(void)
{
	if (cg.demoPlayback)
	{
		return;
	}

	cg.pmext.bAutoReload = (qboolean)(cg_autoReload.integer > 0);
	trap_Cvar_Set("cg_uinfo", va("%d %d %d",
	                             // Client Flags
								 (
									 ((cg_autoReload.integer > 0) ? CGF_AUTORELOAD : 0) |
									 ((cg_autoAction.integer & AA_STATSDUMP) ? CGF_STATSDUMP : 0) |
									 ((cg_autoactivate.integer > 0) ? CGF_AUTOACTIVATE : 0) |
									 ((cg_predictItems.integer > 0) ? CGF_PREDICTITEMS : 0)
									 // Add more in here, as needed
	                             ),

	                             // Timenudge
	                             int_cl_timenudge.integer,
	                             // MaxPackets
	                             int_cl_maxpackets.integer
	                             ));
}

/**
 * @brief CG_CrosshairPlayer
 * @return
 */
int CG_CrosshairPlayer(void)
{
	if (cg.time > (cg.crosshairClientTime + 1000))
	{
		return -1;
	}
	return cg.crosshairClientNum;
}

/**
 * @brief CG_LastAttacker
 * @return
 */
int CG_LastAttacker(void)
{
#ifdef FEATURE_MULTIVIEW
	// used for messaging clients in the currect active window
	if (cg.mvTotalClients > 0)
	{
		return(cg.mvCurrentActive->mvInfo & MV_PID);
	}
#endif

	return((!cg.attackerTime) ? -1 : cg.snap->ps.persistant[PERS_ATTACKER]);
}

/**
 * @brief CG_Printf
 * @param[in] msg
 */
void QDECL CG_Printf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);
	if (!Q_strncmp(text, "[cgnotify]", 10))
	{
		char buf[1024];

		if (!cg_drawNotifyText.integer)
		{
			Q_strncpyz(buf, &text[10], 1013);
			trap_Print(buf);
			return;
		}

		CG_AddToNotify(&text[10]);
		Q_strncpyz(buf, &text[10], 1013);
		Q_strncpyz(text, "[skipnotify]", 13);
		Q_strcat(text, 1011, buf);
	}

	trap_Print(text);
}

/**
 * @brief CG_DPrintf
 * @param[in] msg
 */
void QDECL CG_DPrintf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	if (developer.value == 0.f)
	{
		return;
	}

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);
	if (!Q_strncmp(text, "[cgnotify]", 10))
	{
		char buf[1024];

		if (!cg_drawNotifyText.integer)
		{
			Q_strncpyz(buf, &text[10], 1013);
			trap_Print(buf);
			return;
		}

		Q_strncpyz(buf, &text[10], 1013);
		Q_strncpyz(text, "[skipnotify]", 13);
		Q_strcat(text, 1011, buf);
	}

	trap_Print(text);
}

/**
 * @brief CG_Error
 * @param[in] msg
 */
void QDECL CG_Error(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

#ifndef CGAME_HARD_LINKED

/**
 * @brief Com_Error
 * @param[in] code - unused
 * @param[in] error
 * @note FIXME: this is only here so the functions in q_shared.c and bg_*.c can link
 */
void QDECL Com_Error(int code, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	CG_Error("%s", text);
}

/**
 * @brief Com_Printf
 * @param[in] msg
 */
void QDECL Com_Printf(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	CG_Printf("%s", text);
}

#endif

/**
 * @brief CG_Argv
 * @param[in] arg
 * @return
 */
const char *CG_Argv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

/**
 * @brief Generate standard naming for screenshots/demos
 * @return
 */
char *CG_generateFilename(void)
{
	static char fullFilename[MAX_OSPATH];
	char        prefix[MAX_QPATH];
	qtime_t     ct;
	const char  *pszServerInfo = CG_ConfigString(CS_SERVERINFO);

	trap_RealTime(&ct);
	fullFilename[0] = '\0';
	prefix[0]       = '\0';

	if (cg_autoFolders.integer)
	{
		Com_sprintf(prefix, sizeof(prefix), "%d-%02d/", 1900 + ct.tm_year, ct.tm_mon + 1);
	}

	Com_sprintf(fullFilename, sizeof(fullFilename), "%s%d-%02d-%02d-%02d%02d%02d-%s%s", prefix,
	            1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday,
	            ct.tm_hour, ct.tm_min, ct.tm_sec,
	            Info_ValueForKey(pszServerInfo, "mapname"),
#ifdef FEATURE_MULTIVIEW
	            (cg.mvTotalClients < 1) ?
#endif
	            ""
#ifdef FEATURE_MULTIVIEW
	          : "-MVD"
#endif
	            );

	return fullFilename;
}

/**
 * @brief Strip colors and control codes, copying up to dwMaxLength-1 "good" chars and nul-terminating
 * @param[in] pszIn
 * @param[out] pszOut
 * @param[in] dwMaxLength
 * @param[in] fCRLF
 * @return The length of the cleaned string
 */
int CG_cleanName(const char *pszIn, char *pszOut, int dwMaxLength, qboolean fCRLF)
{
	const char *pInCopy     = pszIn;
	const char *pszOutStart = pszOut;

	while (*pInCopy && (pszOut - pszOutStart < dwMaxLength - 1))
	{
		if (*pInCopy == '^')
		{
			pInCopy += ((pInCopy[1] == 0) ? 1 : 2);
		}
		else if ((*pInCopy < 32 && (!fCRLF || *pInCopy != '\n')) || (*pInCopy > 126))
		{
			pInCopy++;
		}
		else
		{
			*pszOut++ = *pInCopy++;
		}
	}

	*pszOut = 0;
	return(pszOut - pszOutStart);
}

/**
 * @brief CG_findClientNum
 * @param[in] s
 * @return
 */
int CG_findClientNum(const char *s)
{
	int      id;
	char     s2[64], n2[64];
	qboolean fIsNumber = qtrue;

	// See if its a number or string
	for (id = 0; id < (int)strlen(s) && s[id] != 0; id++)
	{
		if (s[id] < '0' || s[id] > '9')
		{
			fIsNumber = qfalse;
			break;
		}
	}

	// numeric values are just slot numbers
	if (fIsNumber)
	{
		id = Q_atoi(s);
		if (id >= 0 && id < cgs.maxclients && cgs.clientinfo[id].infoValid)
		{
			return(id);
		}
	}

	// check for a name match
	CG_cleanName(s, s2, sizeof(s2), qfalse);
	for (id = 0; id < cgs.maxclients; id++)
	{
		if (!cgs.clientinfo[id].infoValid)
		{
			continue;
		}

		CG_cleanName(cgs.clientinfo[id].name, n2, sizeof(n2), qfalse);
		if (!Q_stricmp(n2, s2))
		{
			return(id);
		}
	}

	CG_Printf("[cgnotify]%s ^3%s^7 %s.\n", CG_TranslateString("User"), s, CG_TranslateString("is not on the server"));
	return(-1);
}

/**
 * @brief CG_printConsoleString
 * @param[in] str
 */
void CG_printConsoleString(const char *str)
{
	CG_Printf("[skipnotify]%s", str);
}

/**
 * @brief CG_LoadObjectiveData
 */
void CG_LoadObjectiveData(void)
{
	pc_token_t token, token2;
	int        handle;

	if (cg_gameType.integer == GT_WOLF_LMS)
	{
		handle = trap_PC_LoadSource(va("maps/%s_lms.objdata", Q_strlwr(cgs.rawmapname)));
	}
	else
	{
		handle = trap_PC_LoadSource(va("maps/%s.objdata", Q_strlwr(cgs.rawmapname)));
	}

	if (!handle)
	{
		return;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (!Q_stricmp(token.string, "wm_mapdescription"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				CG_Printf("^1ERROR: bad objdata line : team parameter required\n");
				break;
			}

			if (!trap_PC_ReadToken(handle, &token2))
			{
				CG_Printf("^1ERROR: bad objdata line : description parameter required\n");
				break;
			}

			if (!Q_stricmp(token.string, "axis"))
			{
				Q_strncpyz(cg.objMapDescription_Axis, token2.string, sizeof(cg.objMapDescription_Axis));
			}
			else if (!Q_stricmp(token.string, "allied"))
			{
				Q_strncpyz(cg.objMapDescription_Allied, token2.string, sizeof(cg.objMapDescription_Allied));
			}
			else if (!Q_stricmp(token.string, "neutral"))
			{
				Q_strncpyz(cg.objMapDescription_Neutral, token2.string, sizeof(cg.objMapDescription_Neutral));
			}
		}
		else if (!Q_stricmp(token.string, "wm_objective_axis_desc"))
		{
			int i;

			if (!PC_Int_Parse(handle, &i))
			{
				CG_Printf("^1ERROR: bad objdata line : number parameter required\n");
				break;
			}

			if (!trap_PC_ReadToken(handle, &token))
			{
				CG_Printf("^1ERROR: bad objdata line :  description parameter required\n");
				break;
			}

			i--;

			if (i < 0 || i >= MAX_OBJECTIVES)
			{
				CG_Printf("^1ERROR: bad objdata line : invalid objective number\n");
				break;
			}

			Q_strncpyz(cg.objDescription_Axis[i], token.string, sizeof(cg.objDescription_Axis[i]));
		}
		else if (!Q_stricmp(token.string, "wm_objective_allied_desc"))
		{
			int i;

			if (!PC_Int_Parse(handle, &i))
			{
				CG_Printf("^1ERROR: bad objdata line : number parameter required\n");
				break;
			}

			if (!trap_PC_ReadToken(handle, &token))
			{
				CG_Printf("^1ERROR: bad objdata line :  description parameter required\n");
				break;
			}

			i--;

			if (i < 0 || i >= MAX_OBJECTIVES)
			{
				CG_Printf("^1ERROR: bad objdata line : invalid objective number\n");
				break;
			}

			Q_strncpyz(cg.objDescription_Allied[i], token.string, sizeof(cg.objDescription_Allied[i]));
		}
	}

	trap_PC_FreeSource(handle);
}

//========================================================================

/**
 * @brief CG_SetupDlightstyles
 */
void CG_SetupDlightstyles(void)
{
	int       i, j;
	char      *str;
	char      *token;
	int       entnum;
	centity_t *cent;

	cg.lightstylesInited = qtrue;

	for (i = 1; i < MAX_DLIGHT_CONFIGSTRINGS; i++)
	{
		str = (char *) CG_ConfigString(CS_DLIGHTS + i);
		if (!strlen(str))
		{
			break;
		}

		token  = COM_Parse(&str);    // ent num
		entnum = Q_atoi(token);
		cent   = &cg_entities[entnum];

		token = COM_Parse(&str);     // stylestring
		Q_strncpyz(cent->dl_stylestring, token, strlen(token));

		token             = COM_Parse(&str); // offset
		cent->dl_frame    = Q_atoi(token);
		cent->dl_oldframe = cent->dl_frame - 1;
		if (cent->dl_oldframe < 0)
		{
			cent->dl_oldframe = strlen(cent->dl_stylestring);
		}

		token          = COM_Parse(&str); // sound id
		cent->dl_sound = Q_atoi(token);

		token          = COM_Parse(&str); // attenuation
		cent->dl_atten = Q_atoi(token);

		for (j = 0; j < (int)strlen(cent->dl_stylestring); j++)
		{

			cent->dl_stylestring[j] += cent->dl_atten;  // adjust character for attenuation/amplification

			// clamp result
			if (cent->dl_stylestring[j] < 'a')
			{
				cent->dl_stylestring[j] = 'a';
			}
			if (cent->dl_stylestring[j] > 'z')
			{
				cent->dl_stylestring[j] = 'z';
			}
		}

		cent->dl_backlerp = 0.0;
		cent->dl_time     = cg.time;
	}
}

//========================================================================

/**
 * @brief The server says this item is used on this level
 * @param[in] itemNum
 */
static void CG_RegisterItemSounds(int itemNum)
{
	gitem_t *item = BG_GetItem(itemNum);

	if (item->pickup_sound && *item->pickup_sound)
	{
		cgs.media.itemPickUpSounds[itemNum] = trap_S_RegisterSound(item->pickup_sound, qfalse);
	}
}

/**
 * @brief CG_RegisterGameSounds
 */
void CG_RegisterGameSounds()
{
	cgs.cachedSounds[GAMESOUND_BLANK]         = trap_S_RegisterSound("sound/player/default/blank.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_PLAYER_GURP1]  = trap_S_RegisterSound("sound/player/gurp1.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_PLAYER_GURP2]  = trap_S_RegisterSound("sound/player/gurp2.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_PLAYER_BUBBLE] = trap_S_RegisterSound("sound/world/bubbles.wav", qfalse);

	cgs.cachedSounds[GAMESOUND_WORLD_CHAIRCREAK]     = trap_S_RegisterSound("sound/world/chaircreak.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_WORLD_BUILD]          = trap_S_RegisterSound("sound/world/build.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_WORLD_MG_CONSTRUCTED] = trap_S_RegisterSound("sound/world/mg_constructed.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_WPN_AIRSTRIKE_PLANE]  = trap_S_RegisterSound("sound/weapons/airstrike/airstrike_plane.wav", qfalse);
	//cgs.cachedSounds[GAMESOUND_WPN_ARTILLERY_FLY_1]  = trap_S_RegisterSound("sound/weapons/artillery/artillery_fly_1.wav", qfalse);   // moved in weap file
	//cgs.cachedSounds[GAMESOUND_WPN_ARTILLERY_FLY_2]  = trap_S_RegisterSound("sound/weapons/artillery/artillery_fly_2.wav", qfalse);   // moved in weap file
	//cgs.cachedSounds[GAMESOUND_WPN_ARTILLERY_FLY_3]  = trap_S_RegisterSound("sound/weapons/artillery/artillery_fly_3.wav", qfalse);   // moved in weap file
	cgs.cachedSounds[GAMESOUND_MISC_REVIVE]  = trap_S_RegisterSound("sound/misc/vo_revive.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_MISC_REFEREE] = trap_S_RegisterSound("sound/misc/referee.wav", qfalse);
	cgs.cachedSounds[GAMESOUND_MISC_VOTE]    = trap_S_RegisterSound("sound/misc/vote.wav", qfalse);
	//cgs.cachedSounds[GAMESOUND_MISC_BANNED] = trap_S_RegisterSound("sound/osp/banned.wav", qfalse);
	//cgs.cachedSounds[GAMESOUND_MISC_KICKED] = trap_S_RegisterSound("sound/osp/kicked.wav", qfalse);
}

/**
 * @brief Called during a precache command
 */
static void CG_RegisterSounds(void)
{
	int          i;
	char         name[MAX_QPATH];
	const char   *soundName;
	bg_speaker_t *speaker;

	// voice commands
	CG_LoadVoiceChats();

	// init sound scripts
	CG_SoundInit();

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", cgs.rawmapname));

	for (i = 0; i < BG_NumScriptSpeakers(); i++)
	{
		speaker        = BG_GetScriptSpeaker(i);
		speaker->noise = trap_S_RegisterSound(speaker->filename, qfalse);
	}

	cgs.media.noFireUnderwater = trap_S_RegisterSound("sound/weapons/misc/fire_water.wav", qfalse);
	cgs.media.selectSound      = trap_S_RegisterSound("sound/weapons/misc/change.wav", qfalse);
	cgs.media.landHurt         = trap_S_RegisterSound("sound/player/land_hurt.wav", qfalse);
	cgs.media.gibSound         = trap_S_RegisterSound("sound/player/gib.wav", qfalse);

	cgs.media.watrInSound     = trap_S_RegisterSound("sound/player/water_in.wav", qfalse);
	cgs.media.watrOutSound    = trap_S_RegisterSound("sound/player/water_out.wav", qfalse);
	cgs.media.watrUnSound     = trap_S_RegisterSound("sound/player/water_un.wav", qfalse);
	cgs.media.watrGaspSound   = trap_S_RegisterSound("sound/player/gasp.wav", qfalse);
	cgs.media.underWaterSound = trap_S_RegisterSound("sound/player/underwater.wav", qfalse);

	cgs.media.landSound[FOOTSTEP_NORMAL] = trap_S_RegisterSound("sound/player/footsteps/stone_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_SPLASH] = trap_S_RegisterSound("sound/player/footsteps/water_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_METAL]  = trap_S_RegisterSound("sound/player/footsteps/metal_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_WOOD]   = trap_S_RegisterSound("sound/player/footsteps/wood_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_GRASS]  = trap_S_RegisterSound("sound/player/footsteps/grass_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_GRAVEL] = trap_S_RegisterSound("sound/player/footsteps/gravel_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_ROOF]   = trap_S_RegisterSound("sound/player/footsteps/roof_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_SNOW]   = trap_S_RegisterSound("sound/player/footsteps/snow_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_CARPET] = trap_S_RegisterSound("sound/player/footsteps/carpet_jump.wav", qfalse);

	for (i = 0; i < 4; i++)
	{
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/stone%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/water%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/metal%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/wood%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/grass%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRASS][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/gravel%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRAVEL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/roof%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_ROOF][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/snow%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/carpet%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_CARPET][i] = trap_S_RegisterSound(name, qfalse);
	}

	for (i = 1 ; i < ITEM_MAX_ITEMS ; i++)
	{
		CG_RegisterItemSounds(i);
	}

	for (i = 1 ; i < MAX_SOUNDS ; i++)
	{
		soundName = CG_ConfigString(CS_SOUNDS + i);
		if (!soundName[0])
		{
			break;
		}
		if (soundName[0] == '*')
		{
			continue;   // custom sound
		}

		// register sound scripts seperately
		if (!strstr(soundName, ".wav") && !strstr(soundName, ".ogg"))
		{
			CG_SoundScriptPrecache(soundName);
		}
		else
		{
			cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);
		}
	}

	cgs.media.countFight   = trap_S_RegisterSound("sound/osp/fight.wav", qfalse);
	cgs.media.countPrepare = trap_S_RegisterSound("sound/osp/prepare.wav", qfalse);
	cgs.media.goatAxis     = trap_S_RegisterSound("sound/osp/goat.wav", qfalse);

	cgs.media.headShot = trap_S_RegisterSound("sound/hitsounds/hithead.wav", qfalse);
	cgs.media.bodyShot = trap_S_RegisterSound("sound/hitsounds/hit.wav", qfalse);
	cgs.media.teamShot = trap_S_RegisterSound("sound/hitsounds/hitteam.wav", qfalse);

	cgs.media.flameSound       = trap_S_RegisterSound("sound/weapons/flamethrower/flame_burn.wav", qfalse);
	cgs.media.flameBlowSound   = trap_S_RegisterSound("sound/weapons/flamethrower/flame_pilot.wav", qfalse);
	cgs.media.flameStartSound  = trap_S_RegisterSound("sound/weapons/flamethrower/flame_up.wav", qfalse);
	cgs.media.flameStreamSound = trap_S_RegisterSound("sound/weapons/flamethrower/flame_fire.wav", qfalse);

	cgs.media.grenadePulseSound[0] = trap_S_RegisterSound("sound/weapons/grenade/gren_timer1.wav", qfalse);
	cgs.media.grenadePulseSound[1] = trap_S_RegisterSound("sound/weapons/grenade/gren_timer2.wav", qfalse);
	cgs.media.grenadePulseSound[2] = trap_S_RegisterSound("sound/weapons/grenade/gren_timer3.wav", qfalse);
	cgs.media.grenadePulseSound[3] = trap_S_RegisterSound("sound/weapons/grenade/gren_timer4.wav", qfalse);

	cgs.media.boneBounceSound = trap_S_RegisterSound("sound/world/boardbreak.wav", qfalse);          // TODO: need a real sound for this

	cgs.media.sfx_rockexp = trap_S_RegisterSound("sound/weapons/rocket/rocket_expl.wav", qfalse);

	for (i = 0; i < 3; i++)
	{
		// bouncy shell sounds \o/
		cgs.media.sfx_brassSound[BRASSSOUND_METAL][i][0] = trap_S_RegisterSound(va("sound/weapons/misc/shell_metal%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_METAL][i][1] = trap_S_RegisterSound(va("sound/weapons/misc/sg_shell_metal%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_SOFT][i][0]  = trap_S_RegisterSound(va("sound/weapons/misc/shell_soft%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_SOFT][i][1]  = trap_S_RegisterSound(va("sound/weapons/misc/sg_shell_soft%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_STONE][i][0] = trap_S_RegisterSound(va("sound/weapons/misc/shell_stone%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_STONE][i][1] = trap_S_RegisterSound(va("sound/weapons/misc/sg_shell_stone%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_WOOD][i][0]  = trap_S_RegisterSound(va("sound/weapons/misc/shell_wood%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_WOOD][i][1]  = trap_S_RegisterSound(va("sound/weapons/misc/sg_shell_wood%i.wav", i + 1), qfalse);
		cgs.media.sfx_rubbleBounce[i]                    = trap_S_RegisterSound(va("sound/world/debris%i.wav", i + 1), qfalse);
	}

	cgs.media.uniformPickup     = trap_S_RegisterSound("sound/misc/body_pickup.wav", qfalse);
	cgs.media.buildDecayedSound = trap_S_RegisterSound("sound/world/build_abort.wav", qfalse);

	cgs.media.sndLimboSelect = trap_S_RegisterSound("sound/menu/select.wav", qfalse);
	cgs.media.sndLimboFilter = trap_S_RegisterSound("sound/menu/filter.wav", qfalse);
	//cgs.media.sndLimboCancel = trap_S_RegisterSound("sound/menu/cancel.wav", qfalse);

	cgs.media.sndRankUp  = trap_S_RegisterSound("sound/misc/rank_up.wav", qfalse);
	cgs.media.sndSkillUp = trap_S_RegisterSound("sound/misc/skill_up.wav", qfalse);

	cgs.media.sndMedicCall[0] = trap_S_RegisterSound("sound/chat/axis/medic.wav", qfalse);
	cgs.media.sndMedicCall[1] = trap_S_RegisterSound("sound/chat/allies/medic.wav", qfalse);

	cgs.media.shoveSound = trap_S_RegisterSound("sound/weapons/impact/flesh1.wav", qfalse);

	cgs.media.gibLeg       = trap_R_RegisterModel("models/gibs/leg.md3");
	cgs.media.gibIntestine = trap_R_RegisterModel("models/gibs/intestine.md3");
	cgs.media.gibChest     = trap_R_RegisterModel("models/gibs/skull.md3");

	CG_RegisterGameSounds();

	CG_PrecacheFXSounds();
}

/**
 * @brief CG_GetGameSound
 * @param[in] index
 * @return
 */
sfxHandle_t CG_GetGameSound(int index)
{
	// Cached game file
	if (index < GAMESOUND_MAX)
	{
		return cgs.cachedSounds[index];
	}
	return cgs.gameSounds[index - GAMESOUND_MAX] ? cgs.gameSounds[index - GAMESOUND_MAX] : 0;
}

//===================================================================================

/**
 * @brief CG_RegisterGraphics
 * @details This function may execute for a couple of minutes with a slow disk.
 */
static void CG_RegisterGraphics(void)
{
	char              name[MAX_QPATH];
	int               i;
	static const char *sb_nums[11] =
	{
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	CG_LoadingString(va(" - %s -", cgs.mapname));

	trap_R_LoadWorldMap(cgs.mapname);

	CG_LoadingString(" - entities -");

	numSplinePaths = 0;
	numPathCorners = 0;

	cg.numOIDtriggers2 = 0;

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	CG_ParseEntitiesFromString();

	CG_LoadObjectiveData();

	// precache status bar pics
	CG_LoadingString(" - game media -");

	CG_LoadingString(" - textures -");

	// dynamic shader api example
	// replaces a fueldump texture with a dynamically generated one.
#ifdef TEST_API_DYNAMICSHADER
	trap_R_LoadDynamicShader("my_terrain1_2",
	                         "my_terrain1_2\n"
	                         "{\n"
	                         "    qer_editorimage textures/stone/mxsnow3.tga\n"
	                         "    q3map_baseshader textures/fueldump/terrain_base\n"
	                         "    {\n"
	                         "        map textures/stone/mxrock1aa.tga\n"
	                         "        rgbGen identity\n"
	                         "        tcgen environment\n"
	                         "    }\n"
	                         "    {\n"
	                         "        lightmap $lightmap\n"
	                         "        blendFunc GL_DST_COLOR GL_ZERO\n"
	                         "        rgbgen identity\n"
	                         "    }\n"
	                         "}\n");
	trap_R_RegisterShader("my_terrain1_2");
	trap_R_RemapShader("textures/fueldump/terrain1_2", "my_terrain1_2", "0");
#endif

	for (i = 0 ; i < 11 ; i++)
	{
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);
	}

	cgs.media.fleshSmokePuffShader = trap_R_RegisterShader("fleshimpactsmokepuff");

	// blood cloud
	cgs.media.bloodCloudShader = trap_R_RegisterShader("bloodCloud");

#ifdef FEATURE_MULTIVIEW // commented, kept as reminder
	// MV cursor @multiview
	//cgs.media.cursor = trap_R_RegisterShaderNoMip( "ui/assets/mvcursor.tga" );
#endif

	// cannon
	cgs.media.smokePuffShaderdirty = trap_R_RegisterShader("smokePuffdirty");

	for (i = 0; i < 5; i++)
	{
		cgs.media.smokePuffShaderb[i] = trap_R_RegisterShader(va("smokePuffblack%i", i + 1));
	}

	// bleedanim
	for (i = 0; i < 5; i++)
	{
		cgs.media.viewBloodAni[i] = trap_R_RegisterShader(va("viewBloodBlend%i", i + 1));
	}

	for (i = 0; i < 16; i++)
	{
		cgs.media.viewFlashFire[i] = trap_R_RegisterShader(va("viewFlashFire%i", i + 1));
	}

	// cgs.media.smokePuffRageProShader = trap_R_RegisterShader("smokePuffRagePro"); // unused FIXME: remove from shader def
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
	cgs.media.bloodTrailShader       = trap_R_RegisterShader("bloodTrail");
	cgs.media.lagometerShader        = trap_R_RegisterShader("lagometer");
	cgs.media.reticleShaderSimple    = trap_R_RegisterShader("gfx/misc/reticlesimple");
	cgs.media.binocShaderSimple      = trap_R_RegisterShader("gfx/misc/binocsimple");
	cgs.media.snowShader             = trap_R_RegisterShader("snow_tri");
	cgs.media.oilParticle            = trap_R_RegisterShader("oilParticle");
	cgs.media.oilSlick               = trap_R_RegisterShader("oilSlick");
	cgs.media.waterBubbleShader      = trap_R_RegisterShader("waterBubble");
	cgs.media.tracerShader           = trap_R_RegisterShader("gfx/misc/tracer");

	// hint icon, some of these were never used in default wolf
	cgs.media.usableHintShader        = trap_R_RegisterShader("gfx/2d/usableHint");
	cgs.media.notUsableHintShader     = trap_R_RegisterShader("gfx/2d/notUsableHint");
	cgs.media.doorHintShader          = trap_R_RegisterShader("gfx/2d/doorHint");
	cgs.media.doorRotateHintShader    = trap_R_RegisterShader("gfx/2d/doorRotateHint");         // TODO: no icon, add it ?
	cgs.media.doorLockHintShader      = trap_R_RegisterShader("gfx/2d/lockedhint");
	cgs.media.mg42HintShader          = trap_R_RegisterShader("gfx/2d/mg42Hint");               // TODO: no icon, add it ?
	cgs.media.breakableHintShader     = trap_R_RegisterShader("gfx/2d/breakableHint");
	cgs.media.healthHintShader        = trap_R_RegisterShader("gfx/2d/healthHint");             // TODO: no icon, add it ?
	cgs.media.knifeHintShader         = trap_R_RegisterShader("gfx/2d/knifeHint");
	cgs.media.ladderHintShader        = trap_R_RegisterShader("gfx/2d/ladderHint");
	cgs.media.buttonHintShader        = trap_R_RegisterShader("gfx/2d/buttonHint");             // TODO: no icon, add it ?
	cgs.media.waterHintShader         = trap_R_RegisterShader("gfx/2d/waterHint");
	cgs.media.weaponHintShader        = trap_R_RegisterShader("gfx/2d/weaponHint");             // TODO: no icon, add it ?
	cgs.media.ammoHintShader          = trap_R_RegisterShader("gfx/2d/ammoHint");               // TODO: no icon, add it ?
	cgs.media.powerupHintShader       = trap_R_RegisterShader("gfx/2d/powerupHint");            // TODO: no icon, add it ?
	cgs.media.friendShader            = trap_R_RegisterShaderNoMip("gfx/2d/friendlycross.tga");
	cgs.media.buildHintShader         = trap_R_RegisterShader("gfx/2d/buildHint");
	cgs.media.disarmHintShader        = trap_R_RegisterShader("gfx/2d/disarmHint");             // TODO: no icon, add it ?
	cgs.media.reviveHintShader        = trap_R_RegisterShader("gfx/2d/reviveHint");
	cgs.media.dynamiteHintShader      = trap_R_RegisterShader("gfx/2d/dynamiteHint");
	cgs.media.tankHintShader          = trap_R_RegisterShaderNoMip("gfx/2d/tankHint");
	cgs.media.satchelchargeHintShader = trap_R_RegisterShaderNoMip("gfx/2d/satchelchargeHint");
	cgs.media.landmineHintShader      = trap_R_RegisterShaderNoMip("gfx/2d/landmineHint");
	cgs.media.uniformHintShader       = trap_R_RegisterShaderNoMip("gfx/2d/uniformHint");

	if (cgs.ccLayers)
	{
		for (i = 0; i < cgs.ccLayers; i++)
		{
			cgs.media.commandCentreMapShader[i]      = trap_R_RegisterShaderNoMip(va("levelshots/%s_%i_cc.tga", cgs.rawmapname, i));
			cgs.media.commandCentreMapShaderTrans[i] = trap_R_RegisterShaderNoMip(va("levelshots/%s_%i_cc_trans", cgs.rawmapname, i));
			cgs.media.commandCentreAutomapShader[i]  = trap_R_RegisterShaderNoMip(va("levelshots/%s_%i_cc_automap", cgs.rawmapname, i));
		}
	}
	else
	{
		cgs.media.commandCentreMapShader[0]      = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc.tga", cgs.rawmapname));
		cgs.media.commandCentreMapShaderTrans[0] = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc_trans", cgs.rawmapname));
		cgs.media.commandCentreAutomapShader[0]  = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc_automap", cgs.rawmapname));
	}
	cgs.media.commandCentreAutomapMaskShader    = trap_R_RegisterShaderNoMip("levelshots/automap_mask");
	cgs.media.commandCentreAutomapBorderShader  = trap_R_RegisterShaderNoMip("ui/assets2/maptrim_long");
	cgs.media.commandCentreAutomapBorder2Shader = trap_R_RegisterShaderNoMip("ui/assets2/maptrim_long2");
	cgs.media.commandCentreAutomapCornerShader  = trap_R_RegisterShaderNoMip("ui/assets2/maptrim_edge.tga");
	cgs.media.commandCentreAxisMineShader       = trap_R_RegisterShaderNoMip("sprites/landmine_axis");
	cgs.media.commandCentreAlliedMineShader     = trap_R_RegisterShaderNoMip("sprites/landmine_allied");
	cgs.media.commandCentreSpawnShader[0]       = trap_R_RegisterShaderNoMip("gfx/limbo/cm_flagaxis");
	cgs.media.commandCentreSpawnShader[1]       = trap_R_RegisterShaderNoMip("gfx/limbo/cm_flagallied");
	cgs.media.compassConstructShader            = trap_R_RegisterShaderNoMip("sprites/construct.tga");
	cgs.media.blackmask                         = trap_R_RegisterShaderNoMip("images/blackmask"); // etpro icons support

	cgs.media.countryFlags = trap_R_RegisterShaderNoMip("gfx/flags/world_flags");

	//cgs.media.compassDestroyShader = trap_R_RegisterShaderNoMip("sprites/destroy.tga");

	cgs.media.slashShader    = trap_R_RegisterShaderNoMip("gfx/2d/numbers/slash");
	cgs.media.compass2Shader = trap_R_RegisterShaderNoMip("gfx/2d/compass2.tga");
	cgs.media.compassShader  = trap_R_RegisterShaderNoMip("gfx/2d/compass.tga");
	cgs.media.buddyShader    = trap_R_RegisterShaderNoMip("sprites/buddy.tga");

	for (i = 0 ; i < NUM_CROSSHAIRS ; i++)
	{
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c", 'a' + i));
		cg.crosshairShaderAlt[i]     = trap_R_RegisterShader(va("gfx/2d/crosshair%c_alt", 'a' + i));
	}

	for (i = 0 ; i < SK_NUM_SKILLS ; i++)
	{
		cgs.media.medals[i] = trap_R_RegisterShaderNoMip(va("gfx/limbo/medals0%i", i));
	}

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");
	//cgs.media.noammoShader   = trap_R_RegisterShader("icons/noammo");  // unused FIXME: remove from shader def

	cgs.media.teamStatusBar = trap_R_RegisterShader("gfx/2d/colorbar.tga");

	cgs.media.hudSprintBar = trap_R_RegisterShader("sprintbar");

	cgs.media.hudAlliedHelmet = trap_R_RegisterShader("AlliedHelmet");
	cgs.media.hudAxisHelmet   = trap_R_RegisterShader("AxisHelmet");
	cgs.media.hudAdrenaline   = trap_R_RegisterShaderNoMip("gfx/hud/adrenaline");

	CG_LoadingString(" - models -");

	cgs.media.machinegunBrassModel  = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.panzerfaustBrassModel = trap_R_RegisterModel("models/weapons2/shells/pf_shell.md3");

	cgs.media.smallgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/sm_shell.md3");

	// wolf debris
	cgs.media.debBlock[0] = trap_R_RegisterModel("models/mapobjects/debris/brick1.md3");
	cgs.media.debBlock[1] = trap_R_RegisterModel("models/mapobjects/debris/brick2.md3");
	cgs.media.debBlock[2] = trap_R_RegisterModel("models/mapobjects/debris/brick3.md3");
	cgs.media.debBlock[3] = trap_R_RegisterModel("models/mapobjects/debris/brick4.md3");
	cgs.media.debBlock[4] = trap_R_RegisterModel("models/mapobjects/debris/brick5.md3");
	cgs.media.debBlock[5] = trap_R_RegisterModel("models/mapobjects/debris/brick6.md3");

	cgs.media.debRock[0] = trap_R_RegisterModel("models/mapobjects/debris/rubble1.md3");
	cgs.media.debRock[1] = trap_R_RegisterModel("models/mapobjects/debris/rubble2.md3");
	cgs.media.debRock[2] = trap_R_RegisterModel("models/mapobjects/debris/rubble3.md3");

	cgs.media.debWood[0] = trap_R_RegisterModel("models/gibs/wood/wood1.md3");
	cgs.media.debWood[1] = trap_R_RegisterModel("models/gibs/wood/wood2.md3");
	cgs.media.debWood[2] = trap_R_RegisterModel("models/gibs/wood/wood3.md3");
	cgs.media.debWood[3] = trap_R_RegisterModel("models/gibs/wood/wood4.md3");
	cgs.media.debWood[4] = trap_R_RegisterModel("models/gibs/wood/wood5.md3");
	cgs.media.debWood[5] = trap_R_RegisterModel("models/gibs/wood/wood6.md3");

	cgs.media.debFabric[0] = trap_R_RegisterModel("models/shards/fabric1.md3");
	cgs.media.debFabric[1] = trap_R_RegisterModel("models/shards/fabric2.md3");
	cgs.media.debFabric[2] = trap_R_RegisterModel("models/shards/fabric3.md3");

	cgs.media.spawnInvincibleShader = trap_R_RegisterShader("sprites/shield");
	cgs.media.scoreEliminatedShader = trap_R_RegisterShader("sprites/skull");
	cgs.media.medicReviveShader     = trap_R_RegisterShader("sprites/medic_revive");
	cgs.media.disguisedShader       = trap_R_RegisterShader("sprites/undercover");

	cgs.media.destroyShader = trap_R_RegisterShader("sprites/destroy");

	cgs.media.voiceChatShader = trap_R_RegisterShader("sprites/voiceChat");
	cgs.media.balloonShader   = trap_R_RegisterShader("sprites/balloon3");

	cgs.media.objectiveShader        = trap_R_RegisterShader("sprites/objective");
	cgs.media.objectiveTeamShader    = trap_R_RegisterShaderNoMip("sprites/objective_team");
	cgs.media.objectiveDroppedShader = trap_R_RegisterShaderNoMip("sprites/objective_dropped");
	cgs.media.objectiveEnemyShader   = trap_R_RegisterShaderNoMip("sprites/objective_enemy");
	cgs.media.objectiveBothTEShader  = trap_R_RegisterShaderNoMip("sprites/objective_both_te");
	cgs.media.objectiveBothTDShader  = trap_R_RegisterShaderNoMip("sprites/objective_both_td");
	cgs.media.objectiveBothDEShader  = trap_R_RegisterShaderNoMip("sprites/objective_both_de");
	cgs.media.objectiveSimpleIcon    = trap_R_RegisterShader("simpleicons/objective");
	cgs.media.readyShader            = trap_R_RegisterShader("sprites/ready");

	//cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion"); // unused FIXME: remove from shader def

	// water splash
	cgs.media.waterSplashModel  = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.waterSplashShader = trap_R_RegisterShader("waterSplash");

	// spark particles
	cgs.media.sparkParticleShader = trap_R_RegisterShader("sparkParticle");
	cgs.media.smokeTrailShader    = trap_R_RegisterShader("smokeTrail");

	cgs.media.flamethrowerFireStream = trap_R_RegisterShader("flamethrowerFireStream");
	//cgs.media.flamethrowerBlueStream    = trap_R_RegisterShader("flamethrowerBlueStream"); // unused FIXME: remove from shader def
	cgs.media.onFireShader2 = trap_R_RegisterShader("entityOnFire1");
	cgs.media.onFireShader  = trap_R_RegisterShader("entityOnFire2");
	//cgs.media.viewFadeBlack             = trap_R_RegisterShader("viewFadeBlack"); // unused FIXME: remove from shader def
	cgs.media.sparkFlareShader    = trap_R_RegisterShader("sparkFlareParticle");
	cgs.media.spotLightShader     = trap_R_RegisterShader("spotLight");
	cgs.media.spotLightBeamShader = trap_R_RegisterShader("lightBeam");
	//cgs.media.bulletParticleTrailShader = trap_R_RegisterShader("bulletParticleTrail"); // unused FIXME: remove from shader def
	cgs.media.smokeParticleShader = trap_R_RegisterShader("smokeParticle");

	cgs.media.genericConstructionShader = trap_R_RegisterShader("textures/sfx/construction");
	cgs.media.shoutcastLandmineShader   = trap_R_RegisterShader("textures/sfx/shoutcast_landmine");

	cgs.media.alliedUniformShader = trap_R_RegisterShader("sprites/uniform_allied");
	cgs.media.axisUniformShader   = trap_R_RegisterShader("sprites/uniform_axis");

	// used in: command map
	cgs.media.ccFilterPics[0] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_axis");
	cgs.media.ccFilterPics[1] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_allied");
	cgs.media.ccFilterPics[2] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_spawn");

	cgs.media.ccFilterPics[3] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_bo");
	cgs.media.ccFilterPics[4] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_healthammo");
	cgs.media.ccFilterPics[5] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_construction");
	cgs.media.ccFilterPics[6] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_destruction");
	cgs.media.ccFilterPics[7] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_landmine");
	cgs.media.ccFilterPics[8] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_objective");

	cgs.media.ccFilterBackOn  = trap_R_RegisterShaderNoMip("gfx/limbo/filter_back_on");
	cgs.media.ccFilterBackOff = trap_R_RegisterShaderNoMip("gfx/limbo/filter_back_off");

	// used in:
	//  statsranksmedals
	//  command map
	//  limbo menu
	cgs.media.ccStamps[0] = trap_R_RegisterShaderNoMip("ui/assets2/stamp_complete");
	cgs.media.ccStamps[1] = trap_R_RegisterShaderNoMip("ui/assets2/stamp_failed");

	cgs.media.ccPlayerHighlight    = trap_R_RegisterShaderNoMip("ui/assets/mp_player_highlight.tga");
	cgs.media.ccConstructIcon[0]   = trap_R_RegisterShaderNoMip("gfx/limbo/cm_constaxis");
	cgs.media.ccConstructIcon[1]   = trap_R_RegisterShaderNoMip("gfx/limbo/cm_constallied");
	cgs.media.ccDestructIcon[0][0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_axisgren");
	cgs.media.ccDestructIcon[0][1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_alliedgren");
	cgs.media.ccDestructIcon[1][0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_satchel");
	cgs.media.ccDestructIcon[1][1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_satchel");
	cgs.media.ccDestructIcon[2][0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_dynamite");
	cgs.media.ccDestructIcon[2][1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_dynamite");
	cgs.media.ccTankIcon           = trap_R_RegisterShaderNoMip("gfx/limbo/cm_churchill"); // FIXME: add gfx/limbo/cm_jagdpanther?

	cgs.media.ccCmdPost[0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_bo_axis");
	cgs.media.ccCmdPost[1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_bo_allied");

	cgs.media.ccMortarHit       = trap_R_RegisterShaderNoMip("gfx/limbo/cm_mort_hit");
	cgs.media.ccMortarTarget    = trap_R_RegisterShaderNoMip("gfx/limbo/cm_mort_target");
	cgs.media.mortarTarget      = trap_R_RegisterShaderNoMip("gfx/limbo/mort_target");
	cgs.media.mortarTargetArrow = trap_R_RegisterShaderNoMip("gfx/limbo/mort_targetarrow");

	cgs.media.skillPics[SK_BATTLE_SENSE]                             = trap_R_RegisterShaderNoMip("gfx/limbo/ic_battlesense");
	cgs.media.skillPics[SK_EXPLOSIVES_AND_CONSTRUCTION]              = trap_R_RegisterShaderNoMip("gfx/limbo/ic_engineer");
	cgs.media.skillPics[SK_FIRST_AID]                                = trap_R_RegisterShaderNoMip("gfx/limbo/ic_medic");
	cgs.media.skillPics[SK_SIGNALS]                                  = trap_R_RegisterShaderNoMip("gfx/limbo/ic_fieldops");
	cgs.media.skillPics[SK_LIGHT_WEAPONS]                            = trap_R_RegisterShaderNoMip("gfx/limbo/ic_lightweap");
	cgs.media.skillPics[SK_HEAVY_WEAPONS]                            = trap_R_RegisterShaderNoMip("gfx/limbo/ic_soldier");
	cgs.media.skillPics[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_covertops");

#ifdef FEATURE_PRESTIGE
	cgs.media.prestigePics[0] = trap_R_RegisterShaderNoMip("gfx/hud/prestige/prestige");
	cgs.media.prestigePics[1] = trap_R_RegisterShaderNoMip("gfx/hud/prestige/prestige_stamp");
	cgs.media.prestigePics[2] = trap_R_RegisterShaderNoMip("gfx/hud/prestige/prestige_collect");
#endif

	CG_LoadRankIcons();

	// limbo menu setup
	CG_LimboPanel_Init();

	CG_ChatPanel_Setup();

	CG_Fireteams_Setup();

	CG_Spawnpoints_Setup();

	cgs.media.railCoreShader = trap_R_RegisterShaderNoMip("railCore");       // for debugging server traces
	cgs.media.ropeShader     = trap_R_RegisterShader("textures/props/cable_m01");

	cgs.media.thirdPersonBinocModel = trap_R_RegisterModel("models/multiplayer/binocs/binocs.md3");
	cgs.media.flamebarrel           = trap_R_RegisterModel("models/furniture/barrel/barrel_a.md3");
	cgs.media.mg42muzzleflash       = trap_R_RegisterModel("models/weapons2/machinegun/mg42_flash.md3");

	// shards
	cgs.media.shardGlass1 = trap_R_RegisterModel("models/shards/glass1.md3");
	cgs.media.shardGlass2 = trap_R_RegisterModel("models/shards/glass2.md3");
	cgs.media.shardWood1  = trap_R_RegisterModel("models/shards/wood1.md3");
	cgs.media.shardWood2  = trap_R_RegisterModel("models/shards/wood2.md3");
	cgs.media.shardMetal1 = trap_R_RegisterModel("models/shards/metal1.md3");
	cgs.media.shardMetal2 = trap_R_RegisterModel("models/shards/metal2.md3");
	//cgs.media.shardCeramic1 = trap_R_RegisterModel( "models/shards/ceramic1.md3" );
	//cgs.media.shardCeramic2 = trap_R_RegisterModel( "models/shards/ceramic2.md3" );

	// see see cgs.media.debBlock
	//cgs.media.shardRubble1 = trap_R_RegisterModel("models/mapobjects/debris/brick000.md3");
	//cgs.media.shardRubble2 = trap_R_RegisterModel("models/mapobjects/debris/brick001.md3");
	//cgs.media.shardRubble3 = trap_R_RegisterModel("models/mapobjects/debris/brick002.md3");

	for (i = 0; i < MAX_LOCKER_DEBRIS; i++)
	{
		Com_sprintf(name, sizeof(name), "models/mapobjects/debris/personal%i.md3", i + 1);
		cgs.media.shardJunk[i] = trap_R_RegisterModel(name);
	}

	CG_LoadingString(" - heads-up display -");
	CG_Hud_Setup();

	Com_Memset(cg_weapons, 0, sizeof(cg_weapons));

	CG_LoadingString(" - weapons -");
	for (i = WP_KNIFE; i < WP_NUM_WEAPONS; i++)
	{
		CG_RegisterWeapon(i, qfalse);
	}

	CG_LoadingString(" - items -");
	for (i = 1 ; i < ITEM_MAX_ITEMS ; i++)
	{
		CG_RegisterItemVisuals(i);
	}

	// cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion"); // unused FIXME: remove from shader def
	// cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");   // unused

	cgs.media.hWeaponSnd     = trap_S_RegisterSound("sound/weapons/mg42/mg42_fire.wav", qfalse);
	cgs.media.hWeaponEchoSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_far.wav", qfalse);
	cgs.media.hWeaponHeatSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_heat.wav", qfalse);

	cgs.media.hWeaponSnd_2     = trap_S_RegisterSound("sound/weapons/browning/browning_fire.wav", qfalse);
	cgs.media.hWeaponEchoSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_far.wav", qfalse);
	cgs.media.hWeaponHeatSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_heat.wav", qfalse);

	// FIXME - find a better sound
	cgs.media.hflakWeaponSnd = trap_S_RegisterSound("sound/vehicles/misc/20mm_fire.wav", qfalse);

	cgs.media.minePrimedSound = trap_S_RegisterSound("sound/weapons/landmine/mine_on.wav", qfalse);

	// wall marks
	cgs.media.burnMarkShader     = trap_R_RegisterShaderNoMip("gfx/damage/burn_med_mrk");
	cgs.media.shadowFootShader   = trap_R_RegisterShaderNoMip("markShadowFoot");
	cgs.media.shadowTorsoShader  = trap_R_RegisterShaderNoMip("markShadowTorso");
	cgs.media.wakeMarkShader     = trap_R_RegisterShaderNoMip("wake");
	cgs.media.wakeMarkShaderAnim = trap_R_RegisterShaderNoMip("wakeAnim");

	for (i = 0 ; i < 5 ; i++)
	{
		//Com_sprintf( name, sizeof(name), "textures/decals/blood%i", i+1 );
		//cgs.media.bloodMarkShaders[i] = trap_R_RegisterShader( name );
		Com_sprintf(name, sizeof(name), "blood_dot%i", i + 1);
		cgs.media.bloodDotShaders[i] = trap_R_RegisterShader(name);
	}

	CG_LoadingString(" - inline models -");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	// as a safety check, let's not let the number of models exceed MAX_MODELS
	if (cgs.numInlineModels > MAX_MODELS)
	{
		CG_Error("CG_RegisterGraphics: Too many inline models: %i\n", cgs.numInlineModels);
		//cgs.numInlineModels = MAX_MODELS;
	}

	for (i = 1 ; i < cgs.numInlineModels ; i++)
	{
		vec3_t mins, maxs;
		int    j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for (j = 0 ; j < 3 ; j++)
		{
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5f * (maxs[j] - mins[j]);
		}
	}

	CG_LoadingString(" - server models -");

	// register all the server specified models
	for (i = 1 ; i < MAX_MODELS ; i++)
	{
		const char *modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if (!modelName[0])
		{
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);

		if (!cgs.gameModels[i])
		{
			CG_Printf("^3Warning: Register server model '%s' failed - no valid file in paths.\n", modelName);
		}
	}

	for (i = 1 ; i < MAX_CS_SKINS ; i++)
	{
		const char *skinName;

		skinName = CG_ConfigString(CS_SKINS + i);
		if (!skinName[0])
		{
			break;
		}
		cgs.gameModelSkins[i] = trap_R_RegisterSkin(skinName);
	}

	for (i = 1 ; i < MAX_CS_SHADERS ; i++)
	{
		const char *shaderName;

		shaderName = CG_ConfigString(CS_SHADERS + i);
		if (!shaderName[0])
		{
			break;
		}
		cgs.gameShaders[i] = shaderName[0] == '*' ? trap_R_RegisterShader(shaderName + 1) : trap_R_RegisterShaderNoMip(shaderName);
		Q_strncpyz(cgs.gameShaderNames[i], shaderName[0] == '*' ? shaderName + 1 : shaderName, MAX_QPATH);
	}

	for (i = 1 ; i < MAX_CHARACTERS ; i++)
	{
		const char *characterName;

		characterName = CG_ConfigString(CS_CHARACTERS + i);
		if (!characterName[0])
		{
			break;
		}

		if (!BG_FindCharacter(characterName))
		{
			cgs.gameCharacters[i] = BG_FindFreeCharacter(characterName);

			Q_strncpyz(cgs.gameCharacters[i]->characterFile, characterName, sizeof(cgs.gameCharacters[i]->characterFile));

			if (!CG_RegisterCharacter(characterName, cgs.gameCharacters[i]))
			{
				CG_Error("ERROR: CG_RegisterGraphics: failed to load character file '%s'\n", characterName);
			}
		}
	}

	CG_LoadingString(" - particles -");
	CG_InitParticles();

	InitSmokeSprites();

	CG_LoadingString(" - classes -");

	CG_RegisterPlayerClasses();

	CG_InitPMGraphics();

	// mounted gun on tank models
	cgs.media.hMountedMG42Base = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42nestbase.md3");
	cgs.media.hMountedMG42Nest = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42nest.md3");
	cgs.media.hMountedMG42     = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42.md3");
	cgs.media.hMountedBrowning = trap_R_RegisterModel("models/multiplayer/browning/thirdperson.md3");

	// FIXME: temp models
	// these 1st p models are registered twice ... MG and browning use it ...
	cgs.media.hMountedFPMG42     = trap_R_RegisterModel("models/multiplayer/mg42/v_mg42.md3");
	cgs.media.hMountedFPBrowning = trap_R_RegisterModel("models/multiplayer/browning/tankmounted.md3");

	// medic icon for commandmap
	cgs.media.medicIcon = trap_R_RegisterShaderNoMip("sprites/voiceMedic");

	RegisterFont("ariblk", 27, &cgs.media.limboFont1);
	RegisterFont("ariblk", 16, &cgs.media.limboFont1_lo);
	RegisterFont("courbd", 30, &cgs.media.limboFont2);
	RegisterFont("courbd", 21, &cgs.media.limboFont2_lo);

	cgs.media.medal_back = trap_R_RegisterShaderNoMip("gfx/limbo/medal_back");

	cgs.media.limboNumber_roll = trap_R_RegisterShaderNoMip("gfx/limbo/number_roll");
	cgs.media.limboNumber_back = trap_R_RegisterShaderNoMip("gfx/limbo/number_back");
	cgs.media.limboStar_roll   = trap_R_RegisterShaderNoMip("gfx/limbo/skill_roll");
	cgs.media.limboStar_back   = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back");
	cgs.media.limboLight_on    = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_on");
	cgs.media.limboLight_on2   = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_on02");
	cgs.media.limboLight_off   = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_off");

	cgs.media.limboWeaponNumber_off = trap_R_RegisterShaderNoMip("gfx/limbo/but_weap_off");
	cgs.media.limboWeaponNumber_on  = trap_R_RegisterShaderNoMip("gfx/limbo/but_weap_on");
	cgs.media.limboWeaponCard       = trap_R_RegisterShaderNoMip("gfx/limbo/weap_card");

	cgs.media.limboWeaponCardSurroundH = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_hor");
	cgs.media.limboWeaponCardSurroundV = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_vert");
	cgs.media.limboWeaponCardSurroundC = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_corn");

	cgs.media.limboWeaponCardOOS = trap_R_RegisterShaderNoMip("gfx/limbo/outofstock");

	cgs.media.limboClassButtons[PC_ENGINEER]  = trap_R_RegisterShaderNoMip("gfx/limbo/ic_engineer");
	cgs.media.limboClassButtons[PC_SOLDIER]   = trap_R_RegisterShaderNoMip("gfx/limbo/ic_soldier");
	cgs.media.limboClassButtons[PC_COVERTOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_covertops");
	cgs.media.limboClassButtons[PC_FIELDOPS]  = trap_R_RegisterShaderNoMip("gfx/limbo/ic_fieldops");
	cgs.media.limboClassButtons[PC_MEDIC]     = trap_R_RegisterShaderNoMip("gfx/limbo/ic_medic");
	cgs.media.limboSkillsBS                   = trap_R_RegisterShaderNoMip("gfx/limbo/ic_battlesense");
	cgs.media.limboSkillsLW                   = trap_R_RegisterShaderNoMip("gfx/limbo/ic_lightweap");

	cgs.media.limboClassButton2Back_on         = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back_on");
	cgs.media.limboClassButton2Back_off        = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back_off");
	cgs.media.limboClassButton2Wedge_off       = trap_R_RegisterShaderNoMip("gfx/limbo/skill_4pieces_off");
	cgs.media.limboClassButton2Wedge_on        = trap_R_RegisterShaderNoMip("gfx/limbo/skill_4pieces_on");
	cgs.media.limboClassButtons2[PC_ENGINEER]  = trap_R_RegisterShaderNoMip("gfx/limbo/skill_engineer");
	cgs.media.limboClassButtons2[PC_SOLDIER]   = trap_R_RegisterShaderNoMip("gfx/limbo/skill_soldier");
	cgs.media.limboClassButtons2[PC_COVERTOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_covops");
	cgs.media.limboClassButtons2[PC_FIELDOPS]  = trap_R_RegisterShaderNoMip("gfx/limbo/skill_fieldops");
	cgs.media.limboClassButtons2[PC_MEDIC]     = trap_R_RegisterShaderNoMip("gfx/limbo/skill_medic");

	cgs.media.limboTeamButtonBack_on  = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_on");
	cgs.media.limboTeamButtonBack_off = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_off");
	cgs.media.limboTeamButtonAllies   = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_allied");
	cgs.media.limboTeamButtonAxis     = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_axis");
	cgs.media.limboTeamButtonSpec     = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_spec");

	cgs.media.limboBlendThingy       = trap_R_RegisterShaderNoMip("gfx/limbo/cc_blend");
	cgs.media.limboWeaponBlendThingy = trap_R_RegisterShaderNoMip("gfx/limbo/weap_blend");

	cgs.media.limboCounterBorder = trap_R_RegisterShaderNoMip("gfx/limbo/number_border");

	cgs.media.hudPowerIcon  = trap_R_RegisterShaderNoMip("gfx/hud/ic_power");
	cgs.media.hudSprintIcon = trap_R_RegisterShaderNoMip("gfx/hud/ic_stamina");
	cgs.media.hudHealthIcon = trap_R_RegisterShaderNoMip("gfx/hud/ic_health");

	cgs.media.limboWeaponCardArrow = trap_R_RegisterShaderNoMip("gfx/limbo/weap_dnarrow.tga");

	cgs.media.limboObjectiveBack[0] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back_axis");
	cgs.media.limboObjectiveBack[1] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back_allied");
	cgs.media.limboObjectiveBack[2] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back");

	cgs.media.limboClassBar = trap_R_RegisterShaderNoMip("gfx/limbo/lightup_bar");

	cgs.media.limboBriefingButtonOn      = trap_R_RegisterShaderNoMip("gfx/limbo/but_play_on");
	cgs.media.limboBriefingButtonOff     = trap_R_RegisterShaderNoMip("gfx/limbo/but_play_off");
	cgs.media.limboBriefingButtonStopOn  = trap_R_RegisterShaderNoMip("gfx/limbo/but_stop_on");
	cgs.media.limboBriefingButtonStopOff = trap_R_RegisterShaderNoMip("gfx/limbo/but_stop_off");

	cgs.media.limboSpectator      = trap_R_RegisterShaderNoMip("gfx/limbo/spectator");
	cgs.media.limboShoutcaster    = trap_R_RegisterShaderNoMip("gfx/limbo/shoutcaster");
	cgs.media.limboRadioBroadcast = trap_R_RegisterShaderNoMip("ui/assets/radio_tower");

	cgs.media.limboTeamLocked = trap_R_RegisterShaderNoMip("gfx/limbo/lock");

	cgs.media.cursorIcon = trap_R_RegisterShaderNoMip("ui/assets/3_cursor3");

	cgs.media.hudDamagedStates[0] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood01.skin");
	cgs.media.hudDamagedStates[1] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood02.skin");
	cgs.media.hudDamagedStates[2] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood03.skin");
	cgs.media.hudDamagedStates[3] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood04.skin");

	cgs.media.axisFlag       = trap_R_RegisterShaderNoMip("gfx/limbo/flag_axis");
	cgs.media.alliedFlag     = trap_R_RegisterShaderNoMip("gfx/limbo/flag_allied");
	cgs.media.disconnectIcon = trap_R_RegisterShaderNoMip("gfx/2d/net");

	cgs.media.cm_spec_icon  = trap_R_RegisterShaderNoMip("ui/assets/mp_spec");
	cgs.media.cm_arrow_spec = trap_R_RegisterShaderNoMip("ui/assets/mp_arrow_spec");

	cgs.media.fireteamIcon = trap_R_RegisterShaderNoMip("sprites/fireteam");

	// NOTE: load smoke puff as last shader to always draw on top of other shaders
	// because renderer order the draw depth level by register index
	// i.e: this allow smoke grenade to hide players icons over head
	// or simple item icon on ground
	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");

	CG_LoadingString(" - game media -");
}

/**
 * @brief CG_RegisterClients
 */
static void CG_RegisterClients(void)
{
	int i;

	for (i = 0 ; i < MAX_CLIENTS ; i++)
	{
		const char *clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if (!clientInfo[0])
		{
			continue;
		}
		CG_NewClientInfo(i);
	}
}

//===========================================================================

/**
 * @brief CG_ConfigString
 * @param[in] index
 * @return
 */
const char *CG_ConfigString(int index)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		CG_Error("CG_ConfigString: bad index: %i\n", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

/**
 * @brief CG_ConfigStringCopy
 * @param[in] index
 * @param[out] buff
 * @param[in] buffsize
 * @return
 */
int CG_ConfigStringCopy(int index, char *buff, size_t buffsize)
{
	Q_strncpyz(buff, CG_ConfigString(index), buffsize);
	return strlen(buff);
}

//==================================================================

/**
 * @brief CG_StartMusic
 */
void CG_StartMusic(void)
{
	char *s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	if (strlen(parm1))
	{
		trap_S_StartBackgroundTrack(parm1, parm2, 0);
	}
}

/**
 * @brief CG_QueueMusic
 */
void CG_QueueMusic(void)
{
	char *s;
	char parm[MAX_QPATH];

	// prepare the next background track
	s = (char *)CG_ConfigString(CS_MUSIC_QUEUE);
	Q_strncpyz(parm, COM_Parse(&s), sizeof(parm));

	// even if no strlen(parm).  we want to be able to clear the queue

	// TODO: \/     the values stored in here will be made accessable so
	//              it doesn't have to go through startbackgroundtrack() (which is stupid)
	trap_S_StartBackgroundTrack(parm, "", -2);    // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
}

/**
 * @brief CG_OwnerDrawHandleKey
 * @param ownerDraw - unused
 * @param flags - unused
 * @param special - unused
 * @param key - unused
 * @return TODO: always true
 * @todo not yet implemented ? referenced in cgDC.ownerDrawHandleKey and return always true
 */
static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, int *special, int key)
{
	return qfalse;
}

/**
 * @brief CG_FeederCount
 * @param[in] feederID
 * @return
 */
static int CG_FeederCount(int feederID)
{
	int i, count = 0;

	if (feederID == FEEDER_REDTEAM_LIST)
	{
		for (i = 0; i < cg.numScores; i++)
		{
			if (cg.scores[i].team == TEAM_AXIS)
			{
				count++;
			}
		}
	}
	else if (feederID == FEEDER_BLUETEAM_LIST)
	{
		for (i = 0; i < cg.numScores; i++)
		{
			if (cg.scores[i].team == TEAM_ALLIES)
			{
				count++;
			}
		}
	}
	else if (feederID == FEEDER_SCOREBOARD)
	{
		return cg.numScores;
	}

	return count;
}

/**
 * @brief CG_InfoFromScoreIndex
 * @param[in] index
 * @param[in] team
 * @param[out] scoreIndex
 * @return
 */
static clientInfo_t *CG_InfoFromScoreIndex(int index, int team, int *scoreIndex)
{
	int i, count = 0;

	for (i = 0; i < cg.numScores; i++)
	{
		if (cg.scores[i].team == team)
		{
			if (count == index)
			{
				*scoreIndex = i;
				return &cgs.clientinfo[cg.scores[i].client];
			}
			count++;
		}
	}

	*scoreIndex = index;

	return &cgs.clientinfo[cg.scores[index].client];
}

/**
 * @brief CG_FeederItemText
 * @param[in] feederID
 * @param[in] index
 * @param[in] column
 * @param[out] handle
 * @param numhandles - unused
 * @return
 */
static const char *CG_FeederItemText(int feederID, int index, int column, qhandle_t *handle, int *numhandles)
{
	int          scoreIndex = 0;
	clientInfo_t *info      = NULL;
	int          team       = -1;
	score_t      *sp        = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST)
	{
		team = TEAM_AXIS;
	}
	else if (feederID == FEEDER_BLUETEAM_LIST)
	{
		team = TEAM_ALLIES;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp   = &cg.scores[scoreIndex];

	if (info && info->infoValid)
	{
		switch (column)
		{
		case 0:
			break;
		case 3:
			return info->name;
		case 4:
			return va("%i", info->score);
		case 5:
			return va("%4i", sp->time);
		case 6:
			if (sp->ping == -1)
			{
				return "connecting";
			}
			return va("%4i", sp->ping);
		}
	}

	return "";
}

/**
 * @brief CG_FeederItemImage
 * @param feederID - unused
 * @param index - unused
 * @return
 * @note not yet implemented. referenced in cgDC.feederItemImage but return always 0
 */
static qhandle_t CG_FeederItemImage(int feederID, int index)
{
	return 0;
}

/**
 * @brief CG_FeederSelection
 * @param[in] feederID
 * @param[in] index
 */
static void CG_FeederSelection(int feederID, int index)
{
	int i, count = 0;
	int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_AXIS : TEAM_ALLIES;

	for (i = 0; i < cg.numScores; i++)
	{
		if (cg.scores[i].team == team)
		{
			if (index == count)
			{
				cg.selectedScore = i;
			}
			count++;
		}
	}
}

/**
 * @brief CG_Cvar_Get
 * @param[in] cvar
 * @return
 */
float CG_Cvar_Get(const char *cvar)
{
	char buff[128];

	Com_Memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return (float)atof(buff);
}

/**
 * @brief CG_OwnerDrawWidth
 * @param ownerDraw - unused
 * @param scale - unused
 * @return
 * @note
 */
static int CG_OwnerDrawWidth(int ownerDraw, float scale)
{
	return 0;
}

/**
 * @brief CG_PlayCinematic
 * @param[in] name
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @return
 */
static int CG_PlayCinematic(const char *name, float x, float y, float w, float h)
{
	return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

/**
 * @brief CG_StopCinematic
 * @param[in] handle
 */
static void CG_StopCinematic(int handle)
{
	trap_CIN_StopCinematic(handle);
}

/**
 * @brief CG_DrawCinematic
 * @param[in] handle
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 */
static void CG_DrawCinematic(int handle, float x, float y, float w, float h)
{
	trap_CIN_SetExtents(handle, x, y, w, h);
	trap_CIN_DrawCinematic(handle);
}

/**
 * @brief CG_RunCinematicFrame
 * @param[in] handle
 */
static void CG_RunCinematicFrame(int handle)
{
	trap_CIN_RunCinematic(handle);
}

/**
 * @brief CG_LoadHudMenu
 */
void CG_LoadHudMenu(void)
{
	cgDC.registerShaderNoMip   = &trap_R_RegisterShaderNoMip;
	cgDC.setColor              = &trap_R_SetColor;
	cgDC.drawHandlePic         = &CG_DrawPic;
	cgDC.drawStretchPic        = &trap_R_DrawStretchPic;
	cgDC.drawText              = &CG_Text_Paint;
	cgDC.drawTextExt           = &CG_Text_Paint_Ext;
	cgDC.textWidth             = &CG_Text_Width;
	cgDC.textWidthExt          = &CG_Text_Width_Ext;
	cgDC.textHeight            = &CG_Text_Height;
	cgDC.textHeightExt         = &CG_Text_Height_Ext;
	cgDC.textFont              = &CG_Text_SetActiveFont;
	cgDC.registerModel         = &trap_R_RegisterModel;
	cgDC.modelBounds           = &trap_R_ModelBounds;
	cgDC.fillRect              = &CG_FillRect;
	cgDC.drawRect              = &CG_DrawRect;
	cgDC.drawSides             = &CG_DrawSides;
	cgDC.drawTopBottom         = &CG_DrawTopBottom;
	cgDC.clearScene            = &trap_R_ClearScene;
	cgDC.addRefEntityToScene   = &trap_R_AddRefEntityToScene;
	cgDC.renderScene           = &trap_R_RenderScene;
	cgDC.registerFont          = &trap_R_RegisterFont;
	cgDC.ownerDrawItem         = NULL;
	cgDC.getValue              = &CG_GetValue;
	cgDC.ownerDrawVisible      = &CG_OwnerDrawVisible;
	cgDC.runScript             = &CG_RunMenuScript;
	cgDC.getTeamColor          = &CG_GetTeamColor;
	cgDC.setCVar               = trap_Cvar_Set;
	cgDC.getCVarString         = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue          = CG_Cvar_Get;
	cgDC.drawTextWithCursor    = &CG_Text_PaintWithCursor;
	cgDC.drawTextWithCursorExt = &CG_Text_PaintWithCursor_Ext;
	cgDC.setOverstrikeMode     = &trap_Key_SetOverstrikeMode;
	cgDC.getOverstrikeMode     = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound       = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey    = &CG_OwnerDrawHandleKey;
	cgDC.feederCount           = &CG_FeederCount;
	cgDC.feederItemImage       = &CG_FeederItemImage;
	cgDC.feederItemText        = &CG_FeederItemText;
	cgDC.feederSelection       = &CG_FeederSelection;
	cgDC.setBinding            = &trap_Key_SetBinding;
	cgDC.getBindingBuf         = &trap_Key_GetBindingBuf;
	cgDC.getKeysForBinding     = &trap_Key_KeysForBinding;
	cgDC.keynumToStringBuf     = &trap_Key_KeynumToStringBuf;
	cgDC.translateString       = &CG_TranslateString;
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error          = &Com_Error;
	cgDC.Print          = &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound          = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack   = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack    = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic          = &CG_PlayCinematic;
	cgDC.stopCinematic          = &CG_StopCinematic;
	cgDC.drawCinematic          = &CG_DrawCinematic;
	cgDC.runCinematicFrame      = &CG_RunCinematicFrame;
	cgDC.descriptionForCampaign = &CG_DescriptionForCampaign;
	cgDC.nameForCampaign        = &CG_NameForCampaign;
	cgDC.add2dPolys             = &trap_R_Add2dPolys;
	cgDC.updateScreen           = &trap_UpdateScreen;
	cgDC.getHunkData            = &trap_GetHunkData;
	cgDC.getConfigString        = &CG_ConfigStringCopy;

	cgDC.xscale = cgs.screenXScale;
	cgDC.yscale = cgs.screenYScale;

	Init_Display(&cgDC);

	Menu_Reset();

	CG_Text_SetActiveFont(0);
}

/**
 * @brief CG_AssetCache
 */
void CG_AssetCache(void)
{
	cgDC.Assets.gradientBar         = trap_R_RegisterShaderNoMip(ASSET_GRADIENTBAR);
	cgDC.Assets.gradientRound       = trap_R_RegisterShaderNoMip(ASSET_GRADIENTROUND);
	cgDC.Assets.fxBasePic           = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	cgDC.Assets.fxPic[0]            = trap_R_RegisterShaderNoMip(ART_FX_RED);
	cgDC.Assets.fxPic[1]            = trap_R_RegisterShaderNoMip(ART_FX_YELLOW);
	cgDC.Assets.fxPic[2]            = trap_R_RegisterShaderNoMip(ART_FX_GREEN);
	cgDC.Assets.fxPic[3]            = trap_R_RegisterShaderNoMip(ART_FX_TEAL);
	cgDC.Assets.fxPic[4]            = trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	cgDC.Assets.fxPic[5]            = trap_R_RegisterShaderNoMip(ART_FX_CYAN);
	cgDC.Assets.fxPic[6]            = trap_R_RegisterShaderNoMip(ART_FX_WHITE);
	cgDC.Assets.scrollBar           = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR);
	cgDC.Assets.scrollBarArrowDown  = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWDOWN);
	cgDC.Assets.scrollBarArrowUp    = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWUP);
	cgDC.Assets.scrollBarArrowLeft  = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWLEFT);
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWRIGHT);
	cgDC.Assets.scrollBarThumb      = trap_R_RegisterShaderNoMip(ASSET_SCROLL_THUMB);
	cgDC.Assets.sliderBar           = trap_R_RegisterShaderNoMip(ASSET_SLIDER_BAR);
	cgDC.Assets.sliderThumb         = trap_R_RegisterShaderNoMip(ASSET_SLIDER_THUMB);
}

#ifdef ETLEGACY_DEBUG
#define DEBUG_INITPROFILE_INIT int elapsed, dbgTime = trap_Milliseconds();
#define DEBUG_INITPROFILE_EXEC(f) if (developer.integer) { CG_Printf("^5%s passed in %i msec\n", f, elapsed = trap_Milliseconds() - dbgTime);  dbgTime += elapsed; }
#else
#define DEBUG_INITPROFILE_INIT
#define DEBUG_INITPROFILE_EXEC(f)
#endif // ETLEGACY_DEBUG

/**
 * @brief Called after every level change or subsystem restart
 * Will perform callbacks to make the loading info screen update.
 * @param serverMessageNum
 * @param serverCommandSequence
 * @param clientNum
 * @param demoPlayback
 * @param etLegacyClient
 * @param info
 * @param clientVersion
 */
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback, int etLegacyClient, demoPlayInfo_t *info, int clientVersion)
{
	const char *s;
	int        i;
	char       versionString[128];
	DEBUG_INITPROFILE_INIT

	//int startat = trap_Milliseconds();

	Com_Printf(S_COLOR_MDGREY "Initializing %s cgame " S_COLOR_GREEN ETLEGACY_VERSION "\n", MODNAME);

	// clean up the config backup if one exists
	CG_RestoreProfile();

	// clear everything
	Com_Memset(&cgs, 0, sizeof(cgs));
	Com_Memset(&cg, 0, sizeof(cg));
	Com_Memset(cg_entities, 0, sizeof(cg_entities));
	Com_Memset(cg_weapons, 0, sizeof(cg_weapons));

	cgs.initing = qtrue;

	if (demoPlayback && info)
	{
		cg.demoinfo = info;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		cg.artilleryRequestTime[i] = -99999;
	}

	CG_InitStatsDebug();

	cgs.ccZoomFactor = 1.f;

	// sync to main refdef
	cg.refdef_current = &cg.refdef;

	cg.demoPlayback = demoPlayback;

	MOD_CHECK_ETLEGACY(etLegacyClient, clientVersion, cg.etLegacyClient);

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0f;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0f;


	if (cg.etLegacyClient <= 0)
	{
		cgs.glconfig.windowAspect = (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight;
	}

	// screen support ...
	cgs.adr43       = cgs.glconfig.windowAspect * RPRATIO43;       // aspectratio / (4/3)
	cgs.r43da       = RATIO43 * 1.0f / cgs.glconfig.windowAspect;  // (4/3) / aspectratio
	cgs.wideXoffset = (cgs.glconfig.windowAspect > RATIO43) ? (640.0f * cgs.adr43 - 640.0f) * 0.5f : 0.0f;

	// DEBUG
	//CG_Printf("Screen[%f][%f]: as: %f   adr43: %f  r43da: %f off: %f\n", cgs.screenXScale, cgs.screenYScale, cgs.glconfig.windowAspect, cgs.adr43, cgs.r43da, cgs.wideXoffset);

	// init the anim scripting
	cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
	cgs.animScriptData.playSound  = CG_SoundPlayIndexedScript;

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum  = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	cgs.ccRequestedObjective  = -1;
	cgs.ccCurrentCamObjective = -2;

	// Background images on the loading screen were not visible on the first call
	trap_R_SetColor(NULL);

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader     = trap_R_RegisterShader("gfx/2d/hudchars");   //trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.menucharsetShader = trap_R_RegisterShader("gfx/2d/hudchars");
	cgs.media.whiteShader       = trap_R_RegisterShader("white");
	cgs.media.charsetProp       = trap_R_RegisterShaderNoMip("menu/art/font1_prop.tga");
	cgs.media.charsetPropGlow   = trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
	cgs.media.charsetPropB      = trap_R_RegisterShaderNoMip("menu/art/font2_prop.tga");

	CG_RegisterCvars();

	if (cg_logFile.string[0])
	{
		trap_FS_FOpenFile(cg_logFile.string, &cg.logFile, FS_APPEND);

		if (!cg.logFile)
		{
			CG_Printf("^3WARNING: Couldn't open client log: %s\n", cg_logFile.string);
		}
	}
	else
	{
		CG_Printf("Not logging client output to disk.\n");
	}

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	cg.warmupCount = -1;

	CG_ParseSysteminfo();
	CG_ParseServerinfo();
	CG_ParseWolfinfo();

	CG_InitConsoleCommands();

	// moved this up so it's initialized for the loading screen
	CG_LoadHudMenu();      // load new hud stuff
	CG_AssetCache();

	// try execing map autoexec scripts
	if (!CG_execFile(va("autoexec_%s", cgs.rawmapname)))
	{
		CG_execFile("autoexec_default");
	}

	cgs.campaignInfoLoaded = qfalse;
	cgs.arenaInfoLoaded    = qfalse;

	if (cgs.gametype == GT_WOLF_CAMPAIGN)
	{
		CG_LocateCampaign();
	}
	else if (cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS || cgs.gametype == GT_WOLF || cgs.gametype == GT_WOLF_MAPVOTE)
	{
		CG_LocateArena();
	}

	CG_ClearTrails();

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if (strcmp(s, GAME_VERSION))
	{
		CG_Error("Client/Server game mismatch: '%s/%s'\n", GAME_VERSION, s);
	}

	/* mark old and new clients */
	if (cg.etLegacyClient <= 0)
	{
		trap_Cvar_VariableStringBuffer("version", versionString, sizeof(versionString));
		trap_Cvar_Set("cg_etVersion", versionString[0] ? versionString : "(undetected)");
	}
	else
	{
		sprintf(versionString, "%i", clientVersion);
		trap_Cvar_Set("cg_etVersion", va(PRODUCT_LABEL " v%c.%s %s", versionString[0], versionString + 1, CPUSTRING));
	}

#if 0
	trap_Cvar_Set("cg_legacyVersion", ETLEGACY_VERSION);
#endif

	s                  = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = Q_atoi(s);

	s                         = CG_ConfigString(CS_INTERMISSION_START_TIME);
	cgs.intermissionStartTime = Q_atoi(s);

	CG_ParseServerVersionInfo(CG_ConfigString(CS_VERSIONINFO));
	CG_ParseReinforcementTimes(CG_ConfigString(CS_REINFSEEDS));

	CG_initStrings();
	CG_windowInit();

	DEBUG_INITPROFILE_EXEC("initialization");

	// load the new map
	CG_LoadingString(" - collision map -");

	trap_CM_LoadMap(cgs.mapname);

	DEBUG_INITPROFILE_EXEC("loadmap");

	String_Init();

	cg.loading = qtrue;     // force players to load instead of defer

	CG_LoadingString(" - sounds -");

	CG_RegisterSounds();

	DEBUG_INITPROFILE_EXEC("sounds");

	CG_LoadingString(" - graphics -");

	CG_RegisterGraphics();

	CG_LoadingString(" - flamechunks -");

	CG_InitFlameChunks();       // register and clear all flamethrower resources

	DEBUG_INITPROFILE_EXEC("graphics");

	CG_LoadingString(" - clients -");

	CG_RegisterClients();       // if low on memory, some clients will be deferred

	DEBUG_INITPROFILE_EXEC("clients");

	cg.loading = qfalse;    // future players will be deferred

	CG_InitLocalEntities();

	BG_BuildSplinePaths();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	cg.lightstylesInited = qfalse;

	CG_LoadingString("");

	CG_ShaderStateChanged();

	CG_ChargeTimesChanged();

	CG_TeamRestrictionsChanged();

	CG_SkillLevelsChanged();

	trap_S_ClearLoopingSounds();
	trap_S_ClearSounds(qfalse);

	cg.teamWonRounds[1] = Q_atoi(CG_ConfigString(CS_ROUNDSCORES1));
	cg.teamWonRounds[0] = Q_atoi(CG_ConfigString(CS_ROUNDSCORES2));

	cg.filtercams = Q_atoi(CG_ConfigString(CS_FILTERCAMS)) ? qtrue : qfalse;

	CG_ParseFireteams();

	CG_ParseOIDInfos();

	CG_InitPM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

	DEBUG_INITPROFILE_EXEC("misc");

	CG_ParseSkyBox();

	CG_SetupCabinets();

	trap_S_FadeAllSound(1.0f, 0, qfalse);     // fade sound up

	cgs.dumpStatsFile = 0;
	cgs.dumpStatsTime = 0;

	CG_LoadLocations();
	Com_Memset(cgs.clientLocation, 0, sizeof(cgs.clientLocation));

	CG_UpdateSvCvars();

	CG_ParseModInfo();

	cg.crosshairMine = -1;
	cg.crosshairDyna = -1;

	//CG_Printf("Time taken: %i\n", trap_Milliseconds() - startat);

#ifdef FEATURE_EDV
	VectorSet(cgs.demoCamera.velocity, 0.0, 0.0, 0.0);
	cgs.demoCamera.startLean = qfalse;
	cgs.demoCamera.noclip    = qfalse;
	cgs.currentMenuLevel     = ML_MAIN;
#endif
}

/**
 * @brief Called before every level change or subsystem restart
 */
void CG_Shutdown(void)
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

	CG_EventHandling(CGAME_EVENT_NONE, qtrue);
	if (cg.demoPlayback)
	{
		trap_Cvar_Set("timescale", "1");
	}

	CG_RestoreProfile();

	if (cg.logFile)
	{
		trap_FS_FCloseFile(cg.logFile);
		cg.logFile = 0;
	}

	Q_UTF8_FreeFont(&cgs.media.limboFont1);
	Q_UTF8_FreeFont(&cgs.media.limboFont1_lo);
	Q_UTF8_FreeFont(&cgs.media.limboFont2);
	Q_UTF8_FreeFont(&cgs.media.limboFont2_lo);

	Q_UTF8_FreeFont(&cgs.media.bg_loadscreenfont1);
	Q_UTF8_FreeFont(&cgs.media.bg_loadscreenfont2);
}

/**
 * @brief CG_CheckExecKey
 * @param[in] key
 * @return
 */
qboolean CG_CheckExecKey(int key)
{
	if (cg.showFireteamMenu)
	{
		return CG_FireteamCheckExecKey(key, qfalse);
	}

	if (cg.showSpawnpointsMenu)
	{
		return CG_SpawnpointsCheckExecKey(key, qfalse);
	}

	if (cg.shoutcastMenu)
	{
		return CG_ShoutcastCheckExecKey(key, qfalse);
	}

	return qfalse;
}

/**
 * @brief Real time stamp
 * @return
 */
char *CG_GetRealTime(void)
{
	qtime_t tm;

	trap_RealTime(&tm);
	return va("%2i:%s%i:%s%i",
	          tm.tm_hour,
	          (tm.tm_min > 9 ? "" : "0"),  // minute padding
	          tm.tm_min,
	          (tm.tm_sec > 9 ? "" : "0"),  // second padding
	          tm.tm_sec);
}

/**
 * @brief CG_WriteToLog
 * @param[in] fmt
 */
void QDECL CG_WriteToLog(const char *fmt, ...)
{
	if (cg.logFile)
	{
		va_list argptr;
		char    string[1024];
		size_t  l;

		Com_sprintf(string, sizeof(string), "%s ", CG_GetRealTime());

		l = strlen(string);

		va_start(argptr, fmt);
		Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
		va_end(argptr);

		trap_FS_Write(string, (int)strlen(string), cg.logFile);
	}
}
