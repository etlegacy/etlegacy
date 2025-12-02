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

#include "cg_local.h"

vmCvar_t cg_centertime;
vmCvar_t cg_bobbing;
vmCvar_t cg_swingSpeed;
vmCvar_t cg_shadows;
vmCvar_t cg_gibs;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_drawCrosshairFade;
vmCvar_t cg_drawCrosshairPickups;
vmCvar_t cg_drawSpectatorNames;
vmCvar_t cg_drawHintFade;
vmCvar_t cg_weaponCycleDelay;
vmCvar_t cg_cycleAllWeaps;
vmCvar_t cg_useWeapsForZoom;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_teamVoiceChatsOnly;
vmCvar_t cg_voiceChats;
vmCvar_t cg_voiceText;
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_railTrailTime;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_markTime;
vmCvar_t cg_bloodPuff;
vmCvar_t cg_brassTime;
vmCvar_t cg_letterbox;
vmCvar_t cg_drawGun;
vmCvar_t cg_weapAnims;
vmCvar_t cg_weapBankCollisions;
vmCvar_t cg_weapSwitchNoAmmoSounds;
vmCvar_t cg_gun_frame;
vmCvar_t cg_gunFovOffset;
vmCvar_t cg_gunReviveFadeIn;
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
vmCvar_t cg_muzzleFlashDlight;
vmCvar_t cg_muzzleFlashOld;
vmCvar_t cg_zoomStepSniper;
vmCvar_t cg_zoomDefaultSniper;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_scopedSensitivityScaler;
#ifdef ALLOW_GSYNC
vmCvar_t cg_synchronousClients;
#endif // ALLOW_GSYNC
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatMention;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_coronafardist;
vmCvar_t cg_coronas;
vmCvar_t cg_paused;
vmCvar_t cg_blood;
vmCvar_t cg_predictItems;
vmCvar_t cg_drawEnvAwareness;
vmCvar_t cg_drawEnvAwarenessScale;
vmCvar_t cg_drawEnvAwarenessIconSize;
vmCvar_t cg_dynamicIcons;
vmCvar_t cg_dynamicIconsDistance;
vmCvar_t cg_dynamicIconsSize;
vmCvar_t cg_dynamicIconsMaxScale;
vmCvar_t cg_dynamicIconsMinScale;

vmCvar_t cg_autoactivate;

vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

vmCvar_t cg_gameType;
vmCvar_t cg_bloodTime;
vmCvar_t cg_skybox;

// say, team say, etc.
vmCvar_t cg_messageType;

vmCvar_t cg_timescale;

vmCvar_t cg_spritesFollowHeads;
vmCvar_t cg_voiceSpriteTime;

vmCvar_t cg_drawNotifyText;
vmCvar_t cg_quickMessageAlt;

vmCvar_t cg_redlimbotime;
vmCvar_t cg_bluelimbotime;

vmCvar_t cg_limboClassClickConfirm;

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
vmCvar_t cg_bloodForcePuffsForDamage;
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

vmCvar_t cg_instanttapout;

vmCvar_t cg_debugSkills;

// demo recording cvars
vmCvar_t cl_demorecording;
vmCvar_t cl_demofilename;
vmCvar_t cl_demooffset;
// wav recording cvars
vmCvar_t cl_waverecording;
vmCvar_t cl_wavefilename;
vmCvar_t cl_waveoffset;

vmCvar_t cg_announcer;
vmCvar_t cg_hitSounds;
vmCvar_t cg_locations;
vmCvar_t cg_locationMaxChars;

vmCvar_t cg_spawnTimer_set;         // spawntimer
vmCvar_t cg_spawnTimer_period;      // spawntimer

vmCvar_t cg_logFile;

vmCvar_t cg_countryflags; // GeoIP

vmCvar_t cg_altHud;
vmCvar_t cg_shoutcasterHud;
vmCvar_t cg_tracers;
vmCvar_t cg_fireteamNameMaxChars;
vmCvar_t cg_fireteamNameAlign;
vmCvar_t cg_fireteamSprites;
vmCvar_t cg_fireteamSpritesColor;
vmCvar_t cg_fireteamSpritesColorSelected;

vmCvar_t cg_weapaltReloads;
vmCvar_t cg_weapaltSwitches;
vmCvar_t cg_weapaltMgAutoProne;

vmCvar_t cg_sharetimerText;

vmCvar_t cg_simpleItems;
vmCvar_t cg_simpleItemsScale;

vmCvar_t cg_automapZoom;
vmCvar_t cg_autoCmd;

vmCvar_t cg_popupFadeTime;
vmCvar_t cg_popupStayTime;
vmCvar_t cg_popupTime;

vmCvar_t cg_popupXPGainFadeTime;
vmCvar_t cg_popupXPGainStayTime;
vmCvar_t cg_popupXPGainTime;

vmCvar_t cg_fontScaleSP; // side print

// unlagged optimized prediction
vmCvar_t cg_optimizePrediction;
vmCvar_t cg_debugPlayerHitboxes;
vmCvar_t cg_debugBullets;

#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
// ratings scoreboard
vmCvar_t cg_scoreboard;
#endif

vmCvar_t cg_quickchat;

vmCvar_t cg_drawUnit;

vmCvar_t cg_visualEffects;
vmCvar_t cg_bannerTime;

vmCvar_t cg_shoutcastTeamNameRed;
vmCvar_t cg_shoutcastTeamNameBlue;
vmCvar_t cg_shoutcastDrawHealth;
vmCvar_t cg_shoutcastGrenadeTrail;

vmCvar_t cg_activateLean;

vmCvar_t cg_drawBreathPuffs;
vmCvar_t cg_drawAirstrikePlanes;

vmCvar_t cg_customFont1;
vmCvar_t cg_customFont2;

vmCvar_t cg_drawSpawnpoints;

vmCvar_t cg_useCvarCrosshair;
vmCvar_t cg_crosshairSVG;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairAlpha;
vmCvar_t cg_crosshairColor;
vmCvar_t cg_crosshairAlphaAlt;
vmCvar_t cg_crosshairColorAlt;
vmCvar_t cg_crosshairPulse;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairScaleX;
vmCvar_t cg_crosshairScaleY;

vmCvar_t cg_customCrosshair;
vmCvar_t cg_customCrosshairDotWidth;
vmCvar_t cg_customCrosshairDotColor;
vmCvar_t cg_customCrosshairDotOutlineRounded;
vmCvar_t cg_customCrosshairDotOutlineColor;
vmCvar_t cg_customCrosshairDotOutlineWidth;
vmCvar_t cg_customCrosshairCrossWidth;
vmCvar_t cg_customCrosshairCrossLength;
vmCvar_t cg_customCrosshairCrossGap;
vmCvar_t cg_customCrosshairCrossSpreadDistance;
vmCvar_t cg_customCrosshairCrossSpreadOTGCoef;
vmCvar_t cg_customCrosshairCrossColor;
vmCvar_t cg_customCrosshairCrossOutlineRounded;
vmCvar_t cg_customCrosshairCrossOutlineColor;
vmCvar_t cg_customCrosshairCrossOutlineWidth;
vmCvar_t cg_customCrosshairHealth;

vmCvar_t cg_scopeReticleStyle;
vmCvar_t cg_scopeReticleColor;
vmCvar_t cg_scopeReticleDotColor;
vmCvar_t cg_scopeReticleLineThickness;
vmCvar_t cg_scopeReticleDotThickness;

vmCvar_t cg_commandMapTime;

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
	{ &cg_autoswitch,                         "cg_autoswitch",                         "2",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawGun,                            "cg_drawGun",                            "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_weapAnims,                          "cg_weapAnims",                          "15",          CVAR_ARCHIVE,                 0 },
	{ &cg_weapBankCollisions,                 "cg_weapBankCollisions",                 "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_weapSwitchNoAmmoSounds,             "cg_weapSwitchNoAmmoSounds",             "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_gun_frame,                          "cg_gun_frame",                          "0",           CVAR_TEMP,                    0 },
	{ &cg_zoomDefaultSniper,                  "cg_zoomDefaultSniper",                  "20",          CVAR_ARCHIVE,                 0 }, // changed per atvi req
	{ &cg_zoomStepSniper,                     "cg_zoomStepSniper",                     "2",           CVAR_ARCHIVE,                 0 },
	{ &cg_fov,                                "cg_fov",                                "90",          CVAR_ARCHIVE,                 0 },
	{ &cg_muzzleFlash,                        "cg_muzzleFlash",                        "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_muzzleFlashDlight,                  "cg_muzzleFlashDlight",                  "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_muzzleFlashOld,                     "cg_muzzleFlashOld",                     "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_letterbox,                          "cg_letterbox",                          "0",           CVAR_TEMP,                    0 },
	{ &cg_shadows,                            "cg_shadows",                            "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_gibs,                               "cg_gibs",                               "1",           CVAR_ARCHIVE,                 0 },
	// we now draw reticles always in non demoplayback
	//  { &cg_draw2D, "cg_draw2D", "1", CVAR_CHEAT }, // JPW NERVE changed per atvi req to prevent sniper rifle zoom cheats
	{ &cg_draw2D,                             "cg_draw2D",                             "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_railTrailTime,                      "cg_railTrailTime",                      "750",         CVAR_ARCHIVE,                 0 },
	{ &cg_drawStatus,                         "cg_drawStatus",                         "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawFPS,                            "cg_drawFPS",                            "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshair,                      "cg_drawCrosshair",                      "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshairFade,                  "cg_drawCrosshairFade",                  "250",         CVAR_ARCHIVE,                 0 },
	{ &cg_drawCrosshairPickups,               "cg_drawCrosshairPickups",               "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawSpectatorNames,                 "cg_drawSpectatorNames",                 "2",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawHintFade,                       "cg_drawHintFade",                       "250",         CVAR_ARCHIVE,                 0 },
	{ &cg_useWeapsForZoom,                    "cg_useWeapsForZoom",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_weaponCycleDelay,                   "cg_weaponCycleDelay",                   "150",         CVAR_ARCHIVE,                 0 },
	{ &cg_cycleAllWeaps,                      "cg_cycleAllWeaps",                      "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_brassTime,                          "cg_brassTime",                          "2500",        CVAR_ARCHIVE,                 0 },
	{ &cg_markTime,                           "cg_markTime",                           "20000",       CVAR_ARCHIVE,                 0 },
	{ &cg_bloodPuff,                          "cg_bloodPuff",                          "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_gunReviveFadeIn,                    "cg_gunReviveFadeIn",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_gunFovOffset,                       "cg_gunFovOffset",                       "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_gun_x,                              "cg_gunX",                               "0",           CVAR_TEMP,                    0 },
	{ &cg_gun_y,                              "cg_gunY",                               "0",           CVAR_TEMP,                    0 },
	{ &cg_gun_z,                              "cg_gunZ",                               "0",           CVAR_TEMP,                    0 },
	{ &cg_centertime,                         "cg_centertime",                         "5",           CVAR_ARCHIVE,                 0 }, // changed from 3 to 5
	{ &cg_bobbing,                            "cg_bobbing",                            "0.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_drawEnvAwareness,                   "cg_drawEnvAwareness",                   "7",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawEnvAwarenessScale,              "cg_drawEnvAwarenessScale",              "0.80",        CVAR_ARCHIVE,                 0 },
	{ &cg_drawEnvAwarenessIconSize,           "cg_drawEnvAwarenessIconSize",           "14",          CVAR_ARCHIVE,                 0 },

	{ &cg_dynamicIcons,                       "cg_dynamicIcons",                       "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_dynamicIconsDistance,               "cg_dynamicIconsDistance",               "400",         CVAR_ARCHIVE,                 0 },
	{ &cg_dynamicIconsSize,                   "cg_dynamicIconsSize",                   "20",          CVAR_ARCHIVE,                 0 },
	{ &cg_dynamicIconsMaxScale,               "cg_dynamicIconsMaxScale",               "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_dynamicIconsMinScale,               "cg_dynamicIconsMinScale",               "0.5",         CVAR_ARCHIVE,                 0 },

	{ &cg_autoactivate,                       "cg_autoactivate",                       "1",           CVAR_ARCHIVE,                 0 },

	// more fluid rotations
	{ &cg_swingSpeed,                         "cg_swingSpeed",                         "0.1",         CVAR_ARCHIVE,                 0 },           // was 0.3 for Q3
	{ &cg_bloodTime,                          "cg_bloodTime",                          "120",         CVAR_ARCHIVE,                 0 },

	{ &cg_skybox,                             "cg_skybox",                             "1",           CVAR_ARCHIVE,                 0 },

	// say, team say, etc.
	{ &cg_messageType,                        "cg_messageType",                        "1",           CVAR_TEMP,                    0 },

	{ &cg_animSpeed,                          "cg_animspeed",                          "1",           CVAR_CHEAT,                   0 },
	{ &cg_debugAnim,                          "cg_debuganim",                          "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugPosition,                      "cg_debugposition",                      "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugEvents,                        "cg_debugevents",                        "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugPlayerHitboxes,                "cg_debugPlayerHitboxes",                "0",           CVAR_CHEAT,                   0 },
	{ &cg_debugBullets,                       "cg_debugBullets",                       "0",           CVAR_CHEAT,                   0 },
	{ &cg_errorDecay,                         "cg_errordecay",                         "100",         CVAR_CHEAT,                   0 },
	{ &cg_nopredict,                          "cg_nopredict",                          "0",           CVAR_CHEAT,                   0 },
	{ &cg_noPlayerAnims,                      "cg_noplayeranims",                      "0",           CVAR_CHEAT,                   0 },
	{ &cg_showmiss,                           "cg_showmiss",                           "0",           0,                            0 },
	{ &cg_tracerChance,                       "cg_tracerchance",                       "0.4",         CVAR_CHEAT,                   0 },
	{ &cg_tracerWidth,                        "cg_tracerwidth",                        "0.8",         CVAR_CHEAT,                   0 },
	{ &cg_tracerSpeed,                        "cg_tracerSpeed",                        "4500",        CVAR_CHEAT,                   0 },
	{ &cg_tracerLength,                       "cg_tracerlength",                       "160",         CVAR_CHEAT,                   0 },
	{ &cg_thirdPersonRange,                   "cg_thirdPersonRange",                   "80",          CVAR_CHEAT,                   0 },           // per atvi req
	{ &cg_thirdPersonAngle,                   "cg_thirdPersonAngle",                   "0",           CVAR_CHEAT,                   0 },
	{ &cg_thirdPerson,                        "cg_thirdPerson",                        "0",           CVAR_CHEAT,                   0 },           // per atvi req
	{ &cg_scopedSensitivityScaler,            "cg_scopedSensitivityScaler",            "0.6",         CVAR_ARCHIVE,                 0 },           // per atvi req
	{ &cg_teamChatTime,                       "cg_teamChatTime",                       "8000",        CVAR_ARCHIVE,                 0 },
	{ &cg_teamChatMention,                    "cg_teamChatMention",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_coronafardist,                      "cg_coronafardist",                      "1536",        CVAR_ARCHIVE,                 0 },
	{ &cg_coronas,                            "cg_coronas",                            "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_predictItems,                       "cg_predictItems",                       "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_stats,                              "cg_stats",                              "0",           0,                            0 },

	{ &cg_timescale,                          "timescale",                             "1",           0,                            0 },

	{ &pmove_fixed,                           "pmove_fixed",                           "0",           CVAR_SYSTEMINFO,              0 },
	{ &pmove_msec,                            "pmove_msec",                            "8",           CVAR_SYSTEMINFO,              0 },

	{ &cg_spritesFollowHeads,                 "cg_spritesFollowHeads",                 "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_voiceSpriteTime,                    "cg_voiceSpriteTime",                    "6000",        CVAR_ARCHIVE,                 0 },

	{ &cg_teamChatsOnly,                      "cg_teamChatsOnly",                      "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_teamVoiceChatsOnly,                 "cg_teamVoiceChatsOnly",                 "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_voiceChats,                         "cg_voiceChats",                         "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_voiceText,                          "cg_voiceText",                          "1",           CVAR_ARCHIVE,                 0 },

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_buildScript,                        "com_buildScript",                       "0",           0,                            0 },           // force loading of all possible data and error on failures
	{ &cg_paused,                             "cl_paused",                             "0",           CVAR_ROM,                     0 },

	{ &cg_blood,                              "cg_showblood",                          "1",           CVAR_ARCHIVE,                 0 },
#ifdef ALLOW_GSYNC
	{ &cg_synchronousClients,                 "g_synchronousClients",                  "0",           CVAR_SYSTEMINFO | CVAR_CHEAT, 0 },           // communicated by systeminfo
#endif // ALLOW_GSYNC

	{ &cg_gameType,                           "g_gametype",                            "0",           0,                            0 }, // communicated by systeminfo
	{ &cg_bluelimbotime,                      "",                                      "30000",       0,                            0 }, // communicated by systeminfo
	{ &cg_redlimbotime,                       "",                                      "30000",       0,                            0 }, // communicated by systeminfo
	{ &cg_limboClassClickConfirm,             "cg_limboClassClickConfirm",             "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawNotifyText,                     "cg_drawNotifyText",                     "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_quickMessageAlt,                    "cg_quickMessageAlt",                    "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_antilag,                            "g_antilag",                             "1",           0,                            0 },
	{ &developer,                             "developer",                             "0",           CVAR_CHEAT,                   0 },
	{ &cf_wstats,                             "cf_wstats",                             "1.2",         CVAR_ARCHIVE,                 0 },
	{ &cf_wtopshots,                          "cf_wtopshots",                          "1.0",         CVAR_ARCHIVE,                 0 },

	{ &cg_autoFolders,                        "cg_autoFolders",                        "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_autoAction,                         "cg_autoAction",                         "4",           CVAR_ARCHIVE,                 0 },
	{ &cg_autoReload,                         "cg_autoReload",                         "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_bloodDamageBlend,                   "cg_bloodDamageBlend",                   "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_bloodFlash,                         "cg_bloodFlash",                         "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_bloodFlashTime,                     "cg_bloodFlashTime",                     "1500",        CVAR_ARCHIVE,                 0 },
	{ &cg_bloodForcePuffsForDamage,           "cg_bloodForcePuffsForDamage",           "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_noAmmoAutoSwitch,                   "cg_noAmmoAutoSwitch",                   "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_printObjectiveInfo,                 "cg_printObjectiveInfo",                 "1",           CVAR_ARCHIVE,                 0 },
#ifdef FEATURE_MULTIVIEW
	{ &cg_specHelp,                           "cg_specHelp",                           "1",           CVAR_ARCHIVE,                 0 },
#endif
	{ &cg_uinfo,                              "cg_uinfo",                              "0",           CVAR_ROM | CVAR_USERINFO,     0 },

	{ &demo_avifpsF1,                         "demo_avifpsF1",                         "0",           CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF2,                         "demo_avifpsF2",                         "10",          CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF3,                         "demo_avifpsF3",                         "15",          CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF4,                         "demo_avifpsF4",                         "20",          CVAR_ARCHIVE,                 0 },
	{ &demo_avifpsF5,                         "demo_avifpsF5",                         "24",          CVAR_ARCHIVE,                 0 },
	{ &demo_drawTimeScale,                    "demo_drawTimeScale",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &demo_infoWindow,                       "demo_infoWindow",                       "1",           CVAR_ARCHIVE,                 0 },

#ifdef FEATURE_EDV
	{ &demo_weaponcam,                        "demo_weaponcam",                        "0",           CVAR_ARCHIVE,                 0 },

	{ &demo_followDistance,                   "demo_followDistance",                   "50 0 20",     CVAR_ARCHIVE,                 0 },

	{ &demo_yawPitchRollSpeed,                "demo_yawPitchRollSpeed",                "140 140 140", CVAR_ARCHIVE,                 0 },

	{ &demo_freecamspeed,                     "demo_freecamspeed",                     "800",         CVAR_ARCHIVE,                 0 },
	{ &demo_nopitch,                          "demo_nopitch",                          "1",           CVAR_ARCHIVE,                 0 },
	{ &demo_pvshint,                          "demo_pvshint",                          "0",           CVAR_ARCHIVE,                 0 },
	{ &demo_lookat,                           "demo_lookat",                           "-1",          CVAR_CHEAT,                   0 },
	{ &demo_autotimescaleweapons,             "demo_autotimescaleweapons",             "0",           CVAR_ARCHIVE,                 0 },
	{ &demo_autotimescale,                    "demo_autotimescale",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &demo_teamonlymissilecam,               "demo_teamonlymissilecam",               "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_predefineddemokeys,                 "cg_predefineddemokeys",                 "1",           CVAR_CHEAT | CVAR_ARCHIVE,    0 },
#endif

#ifdef FEATURE_MULTIVIEW
	{ &mv_sensitivity,                        "mv_sensitivity",                        "20",          CVAR_ARCHIVE,                 0 },
#endif

	// Engine mappings

	{ &int_cl_maxpackets,                     "cl_maxpackets",                         "125",         CVAR_ARCHIVE,                 0 },
	{ &int_cl_timenudge,                      "cl_timenudge",                          "0",           CVAR_ARCHIVE,                 0 },
	{ &int_m_pitch,                           "m_pitch",                               "0.022",       CVAR_ARCHIVE,                 0 },
	{ &int_sensitivity,                       "sensitivity",                           "5",           CVAR_ARCHIVE,                 0 },
	{ &int_ui_blackout,                       "ui_blackout",                           "0",           CVAR_ROM,                     0 },

	{ &cg_atmosphericEffects,                 "cg_atmosphericEffects",                 "1",           CVAR_ARCHIVE,                 0 },
	{ &authLevel,                             "authLevel",                             "0",           CVAR_TEMP | CVAR_ROM,         0 },

	{ &cg_rconPassword,                       "auth_rconPassword",                     "",            CVAR_TEMP,                    0 },
	{ &cg_refereePassword,                    "auth_refereePassword",                  "",            CVAR_TEMP,                    0 },

	{ &cg_instanttapout,                      "cg_instanttapout",                      "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_debugSkills,                        "cg_debugSkills",                        "0",           0,                            0 },
	{ NULL,                                   "cg_etVersion",                          "",            CVAR_USERINFO | CVAR_ROM,     0 },
#if 0
	{ NULL,                                   "cg_legacyVersion",                      "",            CVAR_USERINFO | CVAR_ROM,     0 },
#endif
	// demo recording cvars
	{ &cl_demorecording,                      "cl_demorecording",                      "0",           CVAR_ROM,                     0 },
	{ &cl_demofilename,                       "cl_demofilename",                       "",            CVAR_ROM,                     0 },
	{ &cl_demooffset,                         "cl_demooffset",                         "0",           CVAR_ROM,                     0 },
	// wav recording cvars
	{ &cl_waverecording,                      "cl_waverecording",                      "0",           CVAR_ROM,                     0 },
	{ &cl_wavefilename,                       "cl_wavefilename",                       "",            CVAR_ROM,                     0 },
	{ &cl_waveoffset,                         "cl_waveoffset",                         "0",           CVAR_ROM,                     0 },

	{ &cg_announcer,                          "cg_announcer",                          "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_hitSounds,                          "cg_hitSounds",                          "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_locations,                          "cg_locations",                          "3",           CVAR_ARCHIVE,                 0 },
	{ &cg_locationMaxChars,                   "cg_locationMaxChars",                   "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_spawnTimer_set,                     "cg_spawnTimer_set",                     "-1",          CVAR_TEMP,                    0 },

	{ &cg_spawnTimer_period,                  "cg_spawnTimer_period",                  "0",           CVAR_TEMP,                    0 },

	{ &cg_logFile,                            "cg_logFile",                            "",            CVAR_ARCHIVE,                 0 },           // we don't log the chats per default

	{ &cg_countryflags,                       "cg_countryflags",                       "1",           CVAR_ARCHIVE,                 0 },           // GeoIP
	{ &cg_altHud,                             "cg_altHud",                             "0",           CVAR_ARCHIVE,                 0 },           // Hudstyles
	{ &cg_shoutcasterHud,                     "cg_shoutcasterHud",                     "Shoutcaster", CVAR_ARCHIVE,                 0 },
	{ &cg_tracers,                            "cg_tracers",                            "1",           CVAR_ARCHIVE,                 0 },           // Draw tracers
	{ &cg_fireteamNameMaxChars,               "cg_fireteamNameMaxChars",               "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_fireteamNameAlign,                  "cg_fireteamNameAlign",                  "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_fireteamSprites,                    "cg_fireteamSprites",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_fireteamSpritesColor,               "cg_fireteamSpritesColor",               "Green",       CVAR_ARCHIVE,                 0 },
	{ &cg_fireteamSpritesColorSelected,       "cg_fireteamSpritesColorSelected",       "Red",         CVAR_ARCHIVE,                 0 },

	{ &cg_simpleItems,                        "cg_simpleItems",                        "0",           CVAR_ARCHIVE,                 0 },           // Bugged atm
	{ &cg_simpleItemsScale,                   "cg_simpleItemsScale",                   "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_automapZoom,                        "cg_automapZoom",                        "5.159",       CVAR_ARCHIVE,                 0 },
	{ &cg_autoCmd,                            "cg_autoCmd",                            "",            CVAR_TEMP,                    0 },
	{ &cg_popupFadeTime,                      "cg_popupFadeTime",                      "2500",        CVAR_ARCHIVE,                 0 },
	{ &cg_popupStayTime,                      "cg_popupStayTime",                      "2000",        CVAR_ARCHIVE,                 0 },
	{ &cg_popupTime,                          "cg_popupTime",                          "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_popupXPGainFadeTime,                "cg_popupXPGainFadeTime",                "250",         CVAR_ARCHIVE,                 0 },
	{ &cg_popupXPGainStayTime,                "cg_popupXPGainStayTime",                "1000",        CVAR_ARCHIVE,                 0 },
	{ &cg_popupXPGainTime,                    "cg_popupXPGainTime",                    "200",         CVAR_ARCHIVE,                 0 },
	{ &cg_weapaltReloads,                     "cg_weapaltReloads",                     "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_weapaltSwitches,                    "cg_weapaltSwitches",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_weapaltMgAutoProne,                 "cg_weapaltMgAutoProne",                 "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_sharetimerText,                     "cg_sharetimerText",                     "",            CVAR_ARCHIVE,                 0 },

	// Fonts
	{ &cg_fontScaleSP,                        "cg_fontScaleSP",                        "0.22",        CVAR_ARCHIVE,                 0 },           // SidePrint

	{ &cg_optimizePrediction,                 "cg_optimizePrediction",                 "1",           CVAR_ARCHIVE,                 0 },           // unlagged optimized prediction

#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
	{ &cg_scoreboard,                         "cg_scoreboard",                         "0",           CVAR_ARCHIVE,                 0 },
#endif

	{ &cg_quickchat,                          "cg_quickchat",                          "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_drawUnit,                           "cg_drawUnit",                           "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_visualEffects,                      "cg_visualEffects",                      "1",           CVAR_ARCHIVE,                 0 },           // (e.g. : smoke, debris, ...)
	{ &cg_bannerTime,                         "cg_bannerTime",                         "10000",       CVAR_ARCHIVE,                 0 },

	{ &cg_shoutcastTeamNameRed,               "cg_shoutcastTeamNameRed",               "Axis",        CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastTeamNameBlue,              "cg_shoutcastTeamNameBlue",              "Allies",      CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastDrawHealth,                "cg_shoutcastDrawHealth",                "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_shoutcastGrenadeTrail,              "cg_shoutcastGrenadeTrail",              "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_activateLean,                       "cg_activateLean",                       "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_drawBreathPuffs,                    "cg_drawBreathPuffs",                    "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_drawAirstrikePlanes,                "cg_drawAirstrikePlanes",                "1",           CVAR_ARCHIVE,                 0 },

	{ &cg_drawSpawnpoints,                    "cg_drawSpawnpoints",                    "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_useCvarCrosshair,                   "cg_useCvarCrosshair",                   "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairSVG,                       "cg_crosshairSVG",                       "0",           CVAR_ARCHIVE | CVAR_LATCH,    0 },
	{ &cg_crosshairSize,                      "cg_crosshairSize",                      "48",          CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairAlpha,                     "cg_crosshairAlpha",                     "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairColor,                     "cg_crosshairColor",                     "White",       CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairAlphaAlt,                  "cg_crosshairAlphaAlt",                  "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairColorAlt,                  "cg_crosshairColorAlt",                  "White",       CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairPulse,                     "cg_crosshairPulse",                     "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairHealth,                    "cg_crosshairHealth",                    "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairX,                         "cg_crosshairX",                         "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairY,                         "cg_crosshairY",                         "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairScaleX,                    "cg_crosshairScaleX",                    "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_crosshairScaleY,                    "cg_crosshairScaleY",                    "1.0",         CVAR_ARCHIVE,                 0 },

	{ &cg_customCrosshair,                    "cg_customCrosshair",                    "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairDotWidth,            "cg_customCrosshairDotWidth",            "2.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairDotColor,            "cg_customCrosshairDotColor",            "#00FF00E6",   CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairDotOutlineRounded,   "cg_customCrosshairDotOutlineRounded",   "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairDotOutlineColor,     "cg_customCrosshairDotOutlineColor",     "#000000E6",   CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairDotOutlineWidth,     "cg_customCrosshairDotOutlineWidth",     "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossWidth,          "cg_customCrosshairCrossWidth",          "2.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossLength,         "cg_customCrosshairCrossLength",         "8.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossGap,            "cg_customCrosshairCrossGap",            "4.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossSpreadDistance, "cg_customCrosshairCrossSpreadDistance", "25.0",        CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossSpreadOTGCoef,  "cg_customCrosshairCrossSpreadOTGCoef",  "2.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossColor,          "cg_customCrosshairCrossColor",          "#00FF00E6",   CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossOutlineRounded, "cg_customCrosshairCrossOutlineRounded", "1",           CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossOutlineColor,   "cg_customCrosshairCrossOutlineColor",   "#000000E6",   CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairCrossOutlineWidth,   "cg_customCrosshairCrossOutlineWidth",   "1.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_customCrosshairHealth,              "cg_customCrosshairHealth",              "0",           CVAR_ARCHIVE,                 0 },

	{ &cg_scopeReticleStyle,                  "cg_scopeReticleStyle",                  "0",           CVAR_ARCHIVE,                 0 },
	{ &cg_scopeReticleColor,                  "cg_scopeReticleColor",                  "#000000FF",   CVAR_ARCHIVE,                 0 },
	{ &cg_scopeReticleDotColor,               "cg_scopeReticleDotColor",               "#000000FF",   CVAR_ARCHIVE,                 0 },
	{ &cg_scopeReticleLineThickness,          "cg_scopeReticleLineThickness",          "2.0",         CVAR_ARCHIVE,                 0 },
	{ &cg_scopeReticleDotThickness,           "cg_scopeReticleDotThickness",           "2.0",         CVAR_ARCHIVE,                 0 },

	{ &cg_commandMapTime,                     "cg_commandMapTime",                     "0",           CVAR_ARCHIVE,                 0 },
};

static const unsigned int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);
static qboolean           cvarsLoaded   = qfalse;
void CG_setClientFlags(void);

static qboolean CG_RegisterOrUpdateCvars(cvarTable_t *cv)
{
	if (cv->vmCvar == &cg_customCrosshairDotColor)
	{
		Q_ParseColor(cg_customCrosshairDotColor.string, cgs.customCrosshairDotColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_customCrosshairDotOutlineColor)
	{
		Q_ParseColor(cg_customCrosshairDotOutlineColor.string, cgs.customCrosshairDotOutlineColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_customCrosshairCrossColor)
	{
		Q_ParseColor(cg_customCrosshairCrossColor.string, cgs.customCrosshairCrossColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_customCrosshairCrossOutlineColor)
	{
		Q_ParseColor(cg_customCrosshairCrossOutlineColor.string, cgs.customCrosshairCrossOutlineColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_scopeReticleColor)
	{
		Q_ParseColor(cg_scopeReticleColor.string, cgs.scopeReticleColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_scopeReticleDotColor)
	{
		Q_ParseColor(cg_scopeReticleDotColor.string, cgs.scopeReticleDotColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_fireteamSpritesColor)
	{
		Q_ParseColor(cg_fireteamSpritesColor.string, cgs.fireteamSpritesColor);
		return qtrue;
	}
	else if (cv->vmCvar == &cg_fireteamSpritesColorSelected)
	{
		Q_ParseColor(cg_fireteamSpritesColorSelected.string, cgs.fireteamSpritesColorSelected);
		return qtrue;
	}

	return qfalse;
}

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

	// custom fonts, register here since these are ETL-specific features
	if (cg.etLegacyClient)
	{
		trap_Cvar_Register(&cg_customFont1, "cg_customFont1", "", CVAR_ARCHIVE);
		trap_Cvar_Register(&cg_customFont2, "cg_customFont2", "", CVAR_ARCHIVE);
	}

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
			else if (cg_useCvarCrosshair.integer
			         && (cv->vmCvar == &cg_crosshairSize
			             || cv->vmCvar == &cg_crosshairAlpha || cv->vmCvar == &cg_crosshairColor
			             || cv->vmCvar == &cg_crosshairAlphaAlt || cv->vmCvar == &cg_crosshairColorAlt
			             || cv->vmCvar == &cg_crosshairPulse || cv->vmCvar == &cg_crosshairHealth
			             || cv->vmCvar == &cg_crosshairX || cv->vmCvar == &cg_crosshairY
			             || cv->vmCvar == &cg_crosshairScaleX || cv->vmCvar == &cg_crosshairScaleY))
			{
				// force usage of crosshair values
				cv->modificationCount = -1;
			}
			else
			{
				if (CG_RegisterOrUpdateCvars(cv))
				{
					cv->modificationCount = cv->vmCvar->modificationCount;
				}
			}
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = (qboolean)(!!Q_atoi(var));

	// um, here, why?
	CG_setClientFlags();

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
				    cv->vmCvar == &cg_autoactivate || cv->vmCvar == &cg_predictItems ||
				    cv->vmCvar == &cg_activateLean)
				{
					fSetFlags = qtrue;
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
				else if (cv->vmCvar == &cg_crosshairSize
				         || cv->vmCvar == &cg_crosshairAlpha || cv->vmCvar == &cg_crosshairColor
				         || cv->vmCvar == &cg_crosshairAlphaAlt || cv->vmCvar == &cg_crosshairColorAlt
				         || cv->vmCvar == &cg_crosshairPulse || cv->vmCvar == &cg_crosshairHealth
				         || cv->vmCvar == &cg_crosshairX || cv->vmCvar == &cg_crosshairY
				         || cv->vmCvar == &cg_crosshairScaleX || cv->vmCvar == &cg_crosshairScaleY)
				{
					if (cg.clientFrame == 0)
					{
						// wait for the next frame, otherwise the hud load
						// will erase the forced value
						cv->modificationCount = -1;
					}
					else
					{
						trap_SendConsoleCommand(va("%s_f %s\n", cv->cvarName, cv->vmCvar->string));
					}
				}
				else
				{
					CG_RegisterOrUpdateCvars(cv);
				}
			}
		}
	}

	if (cg.etLegacyClient)
	{
		static int cg_customFont1_lastMod = 1;
		static int cg_customFont2_lastMod = 1;

		trap_Cvar_Update(&cg_customFont1);
		trap_Cvar_Update(&cg_customFont2);

		if (cg_customFont1.modificationCount != cg_customFont1_lastMod)
		{
			cg_customFont1_lastMod = cg_customFont1.modificationCount;
			CG_RegisterFonts();
		}
		else if (cg_customFont2.modificationCount != cg_customFont2_lastMod)
		{
			cg_customFont2_lastMod = cg_customFont2.modificationCount;
			CG_RegisterFonts();
		}
	}

	// Send any relevant updates
	if (fSetFlags)
	{
		CG_setClientFlags();
	}
}
