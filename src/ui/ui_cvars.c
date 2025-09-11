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

#include "ui_local.h"

// CVARS {{{1
vmCvar_t ui_brassTime;
vmCvar_t ui_drawCrosshair;
vmCvar_t ui_drawCrosshairInfo;
vmCvar_t ui_drawCrosshairPickups;
vmCvar_t ui_drawSpectatorNames;
vmCvar_t ui_marks;
vmCvar_t ui_autoactivate;

vmCvar_t ui_selectedPlayer;
vmCvar_t ui_selectedPlayerName;
vmCvar_t ui_netSource;
vmCvar_t ui_menuFiles;
vmCvar_t ui_gameType;
vmCvar_t ui_netGameType;
vmCvar_t ui_joinGameType;
vmCvar_t ui_dedicated;

// cvars for multiplayer
vmCvar_t ui_serverFilterType;
vmCvar_t ui_currentNetMap;
vmCvar_t ui_currentMap;
vmCvar_t ui_mapIndex;

vmCvar_t ui_browserShowEmptyOrFull;
vmCvar_t ui_browserShowPasswordProtected;
vmCvar_t ui_browserShowFriendlyFire;
vmCvar_t ui_browserShowMaxlives;

vmCvar_t ui_browserShowAntilag;
vmCvar_t ui_browserShowWeaponsRestricted;
vmCvar_t ui_browserShowTeamBalanced;
vmCvar_t ui_browserShowHumans;

vmCvar_t ui_browserModFilter;
vmCvar_t ui_browserMapFilter;
vmCvar_t ui_browserServerNameFilterCheckBox;

vmCvar_t ui_disableOssFilter;

vmCvar_t ui_serverStatusTimeOut;

vmCvar_t ui_friendlyFire;

vmCvar_t ui_userAlliedRespawnTime;
vmCvar_t ui_userAxisRespawnTime;
vmCvar_t ui_glCustom;
vmCvar_t ui_glPreset;

vmCvar_t g_gameType;

vmCvar_t cl_profile;
vmCvar_t cl_defaultProfile;
vmCvar_t ui_profile;
vmCvar_t ui_currentNetCampaign;
vmCvar_t ui_currentCampaign;
vmCvar_t ui_campaignIndex;
vmCvar_t ui_currentCampaignCompleted;

// cgame mappings
vmCvar_t ui_blackout;       // For speclock
vmCvar_t ui_cg_crosshairColor;
vmCvar_t ui_cg_crosshairColorAlt;
vmCvar_t ui_cg_crosshairAlpha;
vmCvar_t ui_cg_crosshairAlphaAlt;
vmCvar_t ui_cg_crosshairSize;
vmCvar_t ui_cg_crosshairScaleX;
vmCvar_t ui_cg_crosshairScaleY;
vmCvar_t ui_cg_crosshairSVG;

vmCvar_t cl_bypassMouseInput;

vmCvar_t ui_serverBrowserSettings;

vmCvar_t ui_cg_shoutcastTeamNameRed;
vmCvar_t ui_cg_shoutcastTeamNameBlue;
vmCvar_t ui_cg_shoutcastDrawHealth;
vmCvar_t ui_cg_shoutcastGrenadeTrail;

vmCvar_t ui_customFont1;
vmCvar_t ui_customFont2;

// Table {{{1
typedef struct
{
	vmCvar_t *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int cvarFlags;
	int modificationCount;          // for tracking changes
} cvarTable_t;

static cvarTable_t cvarTable[] =
{
	{ NULL,                                "ui_textfield_temp",                   "",                           CVAR_TEMP,                      0 },
	{ &ui_glCustom,                        "ui_glCustom",                         "1",                          CVAR_ROM | CVAR_NOTABCOMPLETE,  0 },
	{ &ui_glPreset,                        "ui_glPreset",                         "0",                          CVAR_ROM | CVAR_NOTABCOMPLETE,  0 },

	{ &ui_friendlyFire,                    "g_friendlyFire",                      "1",                          CVAR_ARCHIVE,                   0 },

	{ &ui_userAlliedRespawnTime,           "ui_userAlliedRespawnTime",            "0",                          0,                              0 },
	{ &ui_userAxisRespawnTime,             "ui_userAxisRespawnTime",              "0",                          0,                              0 },

	{ &ui_brassTime,                       "cg_brassTime",                        "2500",                       CVAR_ARCHIVE,                   0 },
	{ &ui_drawCrosshair,                   "cg_drawCrosshair",                    "1",                          CVAR_ARCHIVE,                   0 },
	{ &ui_drawCrosshairPickups,            "cg_drawCrosshairPickups",             "1",                          CVAR_ARCHIVE,                   0 },
	{ &ui_drawSpectatorNames,              "cg_drawSpectatorNames",               "2",                          CVAR_ARCHIVE,                   0 },
	{ &ui_marks,                           "cg_markTime",                         "20000",                      CVAR_ARCHIVE,                   0 },
	{ &ui_autoactivate,                    "cg_autoactivate",                     "1",                          CVAR_ARCHIVE,                   0 },

	{ &ui_dedicated,                       "ui_dedicated",                        "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_selectedPlayer,                  "cg_selectedPlayer",                   "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_selectedPlayerName,              "cg_selectedPlayerName",               "",                           CVAR_ARCHIVE,                   0 },
	{ &ui_netSource,                       "ui_netSource",                        "1",                          CVAR_ARCHIVE,                   0 },
	{ &ui_menuFiles,                       "ui_menuFiles",                        DEFAULT_MENU_FILE,            CVAR_ARCHIVE,                   0 },
	{ &ui_gameType,                        "ui_gametype",                         "3",                          CVAR_ARCHIVE,                   0 },
	{ &ui_joinGameType,                    "ui_joinGametype",                     "-1",                         CVAR_ARCHIVE,                   0 },
	{ &ui_netGameType,                     "ui_netGametype",                      "4",                          CVAR_ARCHIVE,                   0 }, // hardwired for now

	// multiplayer cvars
	{ &ui_mapIndex,                        "ui_mapIndex",                         "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_currentMap,                      "ui_currentMap",                       "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_currentNetMap,                   "ui_currentNetMap",                    "0",                          CVAR_ARCHIVE,                   0 },

	{ &ui_browserShowEmptyOrFull,          "ui_browserShowEmptyOrFull",           "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowPasswordProtected,    "ui_browserShowPasswordProtected",     "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowFriendlyFire,         "ui_browserShowFriendlyFire",          "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowMaxlives,             "ui_browserShowMaxlives",              "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowAntilag,              "ui_browserShowAntilag",               "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowWeaponsRestricted,    "ui_browserShowWeaponsRestricted",     "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowTeamBalanced,         "ui_browserShowTeamBalanced",          "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowHumans,               "ui_browserShowHumans",                "0",                          CVAR_ARCHIVE,                   0 },

	{ &ui_browserModFilter,                "ui_browserModFilter",                 "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserMapFilter,                "ui_browserMapFilter",                 "",                           CVAR_ARCHIVE,                   0 },
	{ &ui_browserServerNameFilterCheckBox, "ui_browserServerNameFilterCheckBox",  "0",                          CVAR_ARCHIVE,                   0 },

	{ &ui_disableOssFilter,                "ui_disableOssFilter",                 "0",                          CVAR_ARCHIVE,                   0 },

	{ &ui_serverStatusTimeOut,             "ui_serverStatusTimeOut",              "7000",                       CVAR_ARCHIVE,                   0 },

	{ &g_gameType,                         "g_gameType",                          "4",                          CVAR_SERVERINFO | CVAR_LATCH,   0 },
	{ NULL,                                "cg_showblood",                        "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_bloodFlash",                       "1.0",                        CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_autoReload",                       "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_weapaltReloads",                   "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_weapaltSwitches",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_sharetimerText",                   "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_scopedSensitivityScaler",          "0.6",                        CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_noAmmoAutoSwitch",                 "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_useWeapsForZoom",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_zoomDefaultSniper",                "20",                         CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_zoomStepSniper",                   "2",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_voiceSpriteTime",                  "6000",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_printObjectiveInfo",               "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_drawGun",                          "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_coronafardist",                    "1536",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_password",                          "none",                       CVAR_USERINFO,                  0 },
	{ NULL,                                "g_antilag",                           "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE, 0 },
	{ NULL,                                "g_warmup",                            "60",                         CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_lms_roundlimit",                    "3",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_lms_matchlimit",                    "2",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_lms_followTeamOnly",                "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_heavyWeaponRestriction",            "100",                        CVAR_ARCHIVE | CVAR_SERVERINFO, 0 },
	{ &cl_profile,                         "cl_profile",                          "",                           CVAR_ROM,                       0 },
	{ &cl_defaultProfile,                  "cl_defaultProfile",                   "",                           CVAR_ROM,                       0 },
	{ &ui_profile,                         "ui_profile",                          "",                           CVAR_ROM,                       0 },
	{ &ui_currentCampaign,                 "ui_currentCampaign",                  "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_currentNetCampaign,              "ui_currentNetCampaign",               "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_campaignIndex,                   "ui_campaignIndex",                    "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_currentCampaignCompleted,        "ui_currentCampaignCompleted",         "0",                          CVAR_ARCHIVE,                   0 },

	// cgame mappings
	{ &ui_blackout,                        "ui_blackout",                         "0",                          CVAR_ROM,                       0 },
	{ NULL,                                "cg_useCvarCrosshair",                 "1",                          CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairAlpha,               "cg_crosshairAlpha",                   "1.0",                        CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairAlphaAlt,            "cg_crosshairAlphaAlt",                "1.0",                        CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairColor,               "cg_crosshairColor",                   "White",                      CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairColorAlt,            "cg_crosshairColorAlt",                "White",                      CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairSize,                "cg_crosshairSize",                    "48",                         CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairScaleX,              "cg_crosshairScaleX",                  "1.0",                        CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairScaleY,              "cg_crosshairScaleY",                  "1.0",                        CVAR_ARCHIVE,                   0 },
	{ &ui_cg_crosshairSVG,                 "cg_crosshairSVG",                     "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_crosshairPulse",                   "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "cg_crosshairHealth",                  "0",                          CVAR_ARCHIVE,                   0 },

	{ &ui_cg_shoutcastTeamNameRed,         "cg_shoutcastTeamNameRed",             "",                           CVAR_ARCHIVE,                   0 },
	{ &ui_cg_shoutcastTeamNameBlue,        "cg_shoutcastTeamNameBlue",            "",                           CVAR_ARCHIVE,                   0 },
	{ &ui_cg_shoutcastDrawHealth,          "cg_shoutcastDrawHealth",              "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_cg_shoutcastGrenadeTrail,        "cg_shoutcastGrenadeTrail",            "0",                          CVAR_ARCHIVE,                   0 },

	// game mappings (for create server option)
	{ NULL,                                "g_altStopwatchMode",                  "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_ipcomplaintlimit",                  "3",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_complaintlimit",                    "6",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_doWarmup",                          "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_inactivity",                        "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_maxLives",                          "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "refereePassword",                     "none",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                                "shoutcastPassword",                   "none",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_teamForceBalance",                  "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "sv_maxRate",                          "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_spectatorInactivity",               "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_latejoin",                      "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_minplayers",                    MATCH_MINPLAYERS,             CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_mutespecs",                     "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_readypercent",                  "100",                        CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_timeoutcount",                  "3",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_timeoutlength",                 "180",                        CVAR_ARCHIVE,                   0 },
	{ NULL,                                "match_warmupDamage",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "g_customConfig",                      "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "server_motd0",                        " ^NEnemy Territory ^7MOTD ", CVAR_ARCHIVE,                   0 },
	{ NULL,                                "server_motd1",                        "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "server_motd2",                        "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "server_motd3",                        "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "server_motd4",                        "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "server_motd5",                        "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "team_maxplayers",                     "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "team_nocontrols",                     "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_gametype",                 "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_kick",                     "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_map",                      "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_mutespecs",                "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_nextmap",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_config",                   "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_referee",                  "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_shuffleteams",             "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_shuffleteams_norestart",   "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_swapteams",                "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_friendlyfire",             "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_timelimit",                "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_warmupdamage",             "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_antilag",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_muting",                   "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_kick",                     "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_limit",                          "5",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_percent",                        "50",                         CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_surrender",                "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_restartcampaign",          "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_nextcampaign",             "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_cointoss",                 "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                                "vote_allow_poll",                     "1",                          CVAR_ARCHIVE,                   0 },

	{ NULL,                                "ui_cl_lang",                          "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_r_mode",                           "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_r_ext_texture_filter_anisotropic", "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_r_ext_multisample",                "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_cg_shadows",                       "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_rate",                             "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_handedness",                       "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_sensitivity",                      "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                                "ui_profile_mousePitch",               "",                           CVAR_ARCHIVE,                   0 },

	{ &cl_bypassMouseInput,                "cl_bypassMouseInput",                 "0",                          CVAR_TEMP,                      0 },

	{ NULL,                                "g_currentCampaign",                   "",                           CVAR_WOLFINFO | CVAR_ROM,       0 },
	{ NULL,                                "g_currentCampaignMap",                "0",                          CVAR_WOLFINFO | CVAR_ROM,       0 },

	{ NULL,                                "ui_showtooltips",                     "1",                          CVAR_ARCHIVE,                   0 },

	{ NULL,                                "cg_locations",                        "3",                          CVAR_ARCHIVE,                   0 },

	{ &ui_serverBrowserSettings,           "ui_serverBrowserSettings",            "0",                          CVAR_INIT,                      0 },
	{ NULL,                                "cg_allowGeoIP",                       "1",                          CVAR_ARCHIVE | CVAR_USERINFO,   0 },
};

static const unsigned int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);

// Functions {{{1

/**
 * @brief UI_RegisterCvars
 */
void UI_RegisterCvars(void)
{
	unsigned int i;
	cvarTable_t  *cv;

	Com_Printf("%u UI cvars in use\n", cvarTableSize);

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar != NULL)
		{
			cv->modificationCount = cv->vmCvar->modificationCount;
		}
	}

	// Always force this to 0 on init
	trap_Cvar_Set("ui_blackout", "0");

	Q_ParseColor(ui_cg_crosshairColor.string, uiInfo.xhairColor);
	uiInfo.xhairColor[3] = ui_cg_crosshairAlpha.value;

	Q_ParseColor(ui_cg_crosshairColorAlt.string, uiInfo.xhairColorAlt);
	uiInfo.xhairColorAlt[3] = ui_cg_crosshairAlphaAlt.value;
}

/**
 * @brief UI_UpdateCvars
 */
void UI_UpdateCvars(void)
{
	size_t      i;
	cvarTable_t *cv;

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		if (cv->vmCvar)
		{
			trap_Cvar_Update(cv->vmCvar);
			if (cv->modificationCount != cv->vmCvar->modificationCount)
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				if (cv->vmCvar == &ui_cg_crosshairColor || cv->vmCvar == &ui_cg_crosshairAlpha)
				{
					Q_ParseColor(ui_cg_crosshairColor.string, uiInfo.xhairColor);
					uiInfo.xhairColor[3] = ui_cg_crosshairAlpha.value;
				}

				if (cv->vmCvar == &ui_cg_crosshairColorAlt || cv->vmCvar == &ui_cg_crosshairAlphaAlt)
				{
					Q_ParseColor(ui_cg_crosshairColorAlt.string, uiInfo.xhairColorAlt);
					uiInfo.xhairColorAlt[3] = ui_cg_crosshairAlphaAlt.value;
				}
			}
		}
	}

	if (uiInfo.etLegacyClient)
	{
		static int ui_customFont1_lastMod = 1;
		static int ui_customFont2_lastMod = 1;

		trap_Cvar_Update(&ui_customFont1);
		trap_Cvar_Update(&ui_customFont2);

		if (ui_customFont1.modificationCount != ui_customFont1_lastMod)
		{
			ui_customFont1_lastMod = ui_customFont1.modificationCount;
			RegisterSharedFonts();
			UI_Load();
		}
		else if (ui_customFont2.modificationCount != ui_customFont2_lastMod)
		{
			ui_customFont2_lastMod = ui_customFont2.modificationCount;
			RegisterSharedFonts();
			UI_Load();
		}
	}
}
