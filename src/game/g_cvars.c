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

#include "g_local.h"

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

// Declarations {{{1

#ifdef FEATURE_OMNIBOT
vmCvar_t g_OmniBotPath;
vmCvar_t g_OmniBotEnable;
vmCvar_t g_OmniBotFlags;
vmCvar_t g_OmniBotPlaying;
#ifdef ETLEGACY_DEBUG
vmCvar_t g_allowBotSwap;
#endif
#endif
vmCvar_t g_gametype;

vmCvar_t g_timelimit;
vmCvar_t g_friendlyFire;
vmCvar_t g_password;
vmCvar_t sv_privatepassword;
vmCvar_t g_maxclients;
vmCvar_t g_maxGameClients;
vmCvar_t g_minGameClients;
vmCvar_t g_dedicated;
vmCvar_t g_speed;
vmCvar_t g_gravity;
vmCvar_t g_cheats;
vmCvar_t g_knockback;

vmCvar_t g_forcerespawn;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_debugDamage;
vmCvar_t g_debugAlloc;
vmCvar_t g_debugBullets;
vmCvar_t g_motd;
#ifdef ALLOW_GSYNC
vmCvar_t g_synchronousClients;
#endif // ALLOW_GSYNC
vmCvar_t g_warmup;

vmCvar_t g_nextTimeLimit;

vmCvar_t g_userTimeLimit;
vmCvar_t g_userAlliedRespawnTime;
vmCvar_t g_userAxisRespawnTime;
vmCvar_t g_currentRound;
vmCvar_t g_noTeamSwitching;
vmCvar_t g_altStopwatchMode;
vmCvar_t g_gamestate;
vmCvar_t g_swapteams;

vmCvar_t g_restarted;
vmCvar_t g_log;
vmCvar_t g_logSync;
vmCvar_t g_logTimestamp;

vmCvar_t voteFlags;
vmCvar_t g_complaintlimit;
vmCvar_t g_teambleedComplaint;
vmCvar_t g_ipcomplaintlimit;
vmCvar_t g_filtercams;
vmCvar_t g_maxlives;
vmCvar_t g_maxlivesRespawnPenalty;
vmCvar_t g_voiceChatsAllowed;
vmCvar_t g_alliedmaxlives;
vmCvar_t g_axismaxlives;
vmCvar_t g_fastres;
vmCvar_t g_enforcemaxlives;

vmCvar_t g_needpass;
vmCvar_t g_balancedteams;
vmCvar_t g_doWarmup;

vmCvar_t g_teamForceBalance;
vmCvar_t g_banIPs;
vmCvar_t g_filterBan;

vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

vmCvar_t g_scriptName; // name of script file to run (instead of default for that map)

vmCvar_t g_developer;

vmCvar_t g_userAim;

// multiplayer reinforcement times
vmCvar_t g_redlimbotime;
vmCvar_t g_bluelimbotime;
// charge times for character class special weapons
vmCvar_t g_medicChargeTime;
vmCvar_t g_engineerChargeTime;
vmCvar_t g_fieldopsChargeTime;
vmCvar_t g_soldierChargeTime;
vmCvar_t g_covertopsChargeTime;

vmCvar_t g_antilag;

vmCvar_t g_spectatorInactivity;
vmCvar_t match_latejoin;
vmCvar_t match_minplayers;
vmCvar_t match_mutespecs;
vmCvar_t match_readypercent;
vmCvar_t match_timeoutcount;
vmCvar_t match_timeoutlength;
vmCvar_t match_warmupDamage;
vmCvar_t team_maxplayers;
vmCvar_t team_nocontrols;
vmCvar_t server_motd0;
vmCvar_t server_motd1;
vmCvar_t server_motd2;
vmCvar_t server_motd3;
vmCvar_t server_motd4;
vmCvar_t server_motd5;
vmCvar_t vote_allow_config;
vmCvar_t vote_allow_gametype;
vmCvar_t vote_allow_kick;
vmCvar_t vote_allow_map;
vmCvar_t vote_allow_matchreset;
vmCvar_t vote_allow_mutespecs;
vmCvar_t vote_allow_nextmap;
vmCvar_t vote_allow_referee;
vmCvar_t vote_allow_shuffleteams;
vmCvar_t vote_allow_shuffleteams_norestart;
vmCvar_t vote_allow_swapteams;
vmCvar_t vote_allow_friendlyfire;
vmCvar_t vote_allow_timelimit;
vmCvar_t vote_allow_warmupdamage;
vmCvar_t vote_allow_antilag;
vmCvar_t vote_allow_balancedteams;
vmCvar_t vote_allow_muting;
vmCvar_t vote_limit;
vmCvar_t vote_percent;
vmCvar_t vote_allow_surrender;
vmCvar_t vote_allow_restartcampaign;
vmCvar_t vote_allow_nextcampaign;
vmCvar_t vote_allow_poll;
vmCvar_t vote_allow_maprestart;
vmCvar_t vote_allow_cointoss;

vmCvar_t refereePassword;
vmCvar_t shoutcastPassword;
vmCvar_t g_debugConstruct;
vmCvar_t g_landminetimeout;

// Variable for setting the current level of debug printing/logging
// enabled in bot scripts and regular scripts.
vmCvar_t g_scriptDebug;
vmCvar_t g_scriptDebugLevel;
vmCvar_t g_scriptDebugTarget;
vmCvar_t g_movespeed;

vmCvar_t g_axismapxp;
vmCvar_t g_alliedmapxp;

vmCvar_t g_currentCampaign;
vmCvar_t g_currentCampaignMap;

// for LMS
vmCvar_t g_axiswins;
vmCvar_t g_alliedwins;
vmCvar_t g_lms_teamForceBalance;
vmCvar_t g_lms_roundlimit;
vmCvar_t g_lms_matchlimit;
vmCvar_t g_lms_currentMatch;
vmCvar_t g_lms_lockTeams;
vmCvar_t g_lms_followTeamOnly;

vmCvar_t mod_version;
vmCvar_t mod_url;
vmCvar_t url;

vmCvar_t g_debugSkills;
vmCvar_t g_heavyWeaponRestriction;
vmCvar_t g_autoFireteams;

vmCvar_t g_nextmap;
vmCvar_t g_nextcampaign;

vmCvar_t g_disableComplaints;

// zinx etpro antiwarp
vmCvar_t g_antiwarp;
vmCvar_t g_maxWarp;

#ifdef FEATURE_LUA
vmCvar_t lua_modules;
vmCvar_t lua_allowedModules;
vmCvar_t g_luaModuleList;
#endif

vmCvar_t g_guidCheck;
vmCvar_t g_protect; // similar to sv_protect game cvar
                    // 0 - no protection - default to have ref for localhost clients on listen servers
                    // 1 - disabled auto ref for localhost clients

vmCvar_t g_dropHealth;
vmCvar_t g_dropAmmo;

vmCvar_t g_shove;
vmCvar_t g_shoveNoZ;

// MAPVOTE
vmCvar_t g_mapVoteFlags;
vmCvar_t g_maxMapsVotedFor;
vmCvar_t g_minMapAge;
vmCvar_t g_excludedMaps;
vmCvar_t g_resetXPMapCount;

vmCvar_t g_campaignFile;

vmCvar_t g_countryflags; // GeoIP

// arty/airstrike rate limiting
vmCvar_t team_maxAirstrikes;
vmCvar_t team_maxArtillery;

// team class/weapon limiting
// classes
vmCvar_t team_maxSoldiers;
vmCvar_t team_maxMedics;
vmCvar_t team_maxEngineers;
vmCvar_t team_maxFieldops;
vmCvar_t team_maxCovertops;
// weapons
vmCvar_t team_maxMortars;
vmCvar_t team_maxFlamers;
vmCvar_t team_maxMachineguns;
vmCvar_t team_maxRockets;
vmCvar_t team_maxRiflegrenades;
vmCvar_t team_maxLandmines;
// misc
vmCvar_t team_riflegrenades;
// skills
vmCvar_t skill_soldier;
vmCvar_t skill_medic;
vmCvar_t skill_engineer;
vmCvar_t skill_fieldops;
vmCvar_t skill_covertops;
vmCvar_t skill_battlesense;
vmCvar_t skill_lightweapons;

vmCvar_t g_misc;

vmCvar_t g_intermissionTime;
vmCvar_t g_intermissionReadyPercent;

vmCvar_t g_mapScriptDirectory;
vmCvar_t g_mapConfigs;
vmCvar_t g_customConfig;

vmCvar_t g_moverScale;

vmCvar_t g_fixedphysics;
vmCvar_t g_fixedphysicsfps;

vmCvar_t g_pronedelay;

vmCvar_t g_debugHitboxes;
vmCvar_t g_debugPlayerHitboxes;

vmCvar_t g_voting;        // see VOTEF_ defines

vmCvar_t g_corpses; // dynamic body que FIXME: limit max bodies by var value

// os support - this SERVERINFO cvar specifies supported client operating systems on server
// supported platforms are in the 'oss_t' enum
vmCvar_t g_oss;

vmCvar_t g_realHead; // b_realHead functionality from ETPro

vmCvar_t sv_fps;
vmCvar_t g_skipCorrection;

vmCvar_t g_extendedNames;

#ifdef FEATURE_RATING
vmCvar_t g_skillRating;
#endif

#ifdef FEATURE_PRESTIGE
vmCvar_t g_prestige;
#endif

#ifdef FEATURE_MULTIVIEW
vmCvar_t g_multiview; // 0 - off, other - enabled
#endif

vmCvar_t g_stickyCharge;
vmCvar_t g_xpSaver;

vmCvar_t g_debugForSingleClient;
vmCvar_t g_debugEvents;

vmCvar_t g_debugAnim;

vmCvar_t g_suddenDeath;

vmCvar_t g_dropObjDelay;

// flood protection
vmCvar_t g_floodProtection;
vmCvar_t g_floodLimit;
vmCvar_t g_floodWait;

vmCvar_t g_etltv_flags;

// Table {{{1

typedef struct
{
	vmCvar_t *vmCvar;
	char *cvarName;
	char *defaultString;
	int cvarFlags;
	int modificationCount;          // for tracking changes
	qboolean trackChange;           // track this variable, and announce if changed
	qboolean fConfigReset;          // set this var to the default on a config reset
} cvarTable_t;

cvarTable_t gameCvarTable[] =
{
	// don't override the cheat state set by the system
	{ &g_cheats,                          "sv_cheats",                         "",                           0,                                               0, qfalse, qfalse },
	{ &sv_fps,                            "sv_fps",                            DEFAULT_SV_FPS_STR,           CVAR_SYSTEMINFO,                                 0, qfalse, qfalse },

	// noset vars
	{ NULL,                               "gamename",                          MODNAME,                      CVAR_SERVERINFO | CVAR_ROM,                      0, qfalse, qfalse },
	{ NULL,                               "gamedate",                          __DATE__,                     CVAR_ROM,                                        0, qfalse, qfalse },
	{ &g_restarted,                       "g_restarted",                       "0",                          CVAR_ROM,                                        0, qfalse, qfalse },
	{ NULL,                               "sv_mapname",                        "",                           CVAR_SERVERINFO | CVAR_ROM,                      0, qfalse, qfalse },

	// latched vars
	{ &g_gametype,                        "g_gametype",                        "4",                          CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qfalse },     // default to GT_WOLF_CAMPAIGN

	// multiplayer stuffs
	{ &g_redlimbotime,                    "g_redlimbotime",                    "30000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qfalse },
	{ &g_bluelimbotime,                   "g_bluelimbotime",                   "30000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qfalse },
	{ &g_medicChargeTime,                 "g_medicChargeTime",                 "45000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qtrue  },
	{ &g_engineerChargeTime,              "g_engineerChargeTime",              "30000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qtrue  },
	{ &g_fieldopsChargeTime,              "g_fieldopsChargeTime",              "40000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qtrue  },
	{ &g_soldierChargeTime,               "g_soldierChargeTime",               "20000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qtrue  },
	{ &g_covertopsChargeTime,             "g_covertopsChargeTime",             "30000",                      CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qtrue  },
	{ &g_landminetimeout,                 "g_landminetimeout",                 "1",                          CVAR_ARCHIVE,                                    0, qfalse, qtrue  },

	{ &g_oss,                             "g_oss",                             "0",                          CVAR_SERVERINFO | CVAR_ROM,                      0, qfalse, qfalse },

	{ &g_maxclients,                      "sv_maxclients",                     "20",                         CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE,     0, qfalse, qfalse },
	{ &g_maxGameClients,                  "g_maxGameClients",                  "0",                          CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE,     0, qfalse, qfalse },
	{ &g_minGameClients,                  "g_minGameClients",                  "8",                          CVAR_SERVERINFO,                                 0, qfalse, qfalse },

	// change anytime vars
	{ &g_timelimit,                       "timelimit",                         "0",                          CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue,  qfalse },

#ifdef ALLOW_GSYNC
	{ &g_synchronousClients,              "g_synchronousClients",              "0",                          CVAR_SYSTEMINFO | CVAR_CHEAT,                    0, qfalse, qfalse },
#endif // ALLOW_GSYNC

	{ &g_friendlyFire,                    "g_friendlyFire",                    "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE,                  0, qtrue,  qtrue  },

	{ &g_teamForceBalance,                "g_teamForceBalance",                "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },     // merge from team arena

	{ &g_warmup,                          "g_warmup",                          "60",                         CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_doWarmup,                        "g_doWarmup",                        "0",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },

	{ &g_nextTimeLimit,                   "g_nextTimeLimit",                   "0",                          CVAR_WOLFINFO,                                   0, qfalse, qfalse },
	{ &g_currentRound,                    "g_currentRound",                    "0",                          CVAR_WOLFINFO,                                   0, qfalse, qfalse },
	{ &g_altStopwatchMode,                "g_altStopwatchMode",                "0",                          CVAR_ARCHIVE,                                    0, qtrue,  qtrue  },
	{ &g_gamestate,                       "gamestate",                         "-1",                         CVAR_WOLFINFO | CVAR_ROM,                        0, qfalse, qfalse },

	{ &g_noTeamSwitching,                 "g_noTeamSwitching",                 "0",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },

	{ &g_userTimeLimit,                   "g_userTimeLimit",                   "0",                          0,                                               0, qfalse, qtrue  },
	{ &g_userAlliedRespawnTime,           "g_userAlliedRespawnTime",           "0",                          0,                                               0, qfalse, qtrue  },
	{ &g_userAxisRespawnTime,             "g_userAxisRespawnTime",             "0",                          0,                                               0, qfalse, qtrue  },

	{ &g_swapteams,                       "g_swapteams",                       "0",                          CVAR_ROM,                                        0, qfalse, qfalse },

	{ &g_log,                             "g_log",                             "",                           CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_logSync,                         "g_logSync",                         "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_logTimestamp,                    "g_logTimestamp",                    "1",                          CVAR_TEMP,                                       0, qfalse, qfalse },

	{ &g_password,                        "g_password",                        "none",                       CVAR_USERINFO,                                   0, qfalse, qfalse },
	{ &sv_privatepassword,                "sv_privatepassword",                "",                           CVAR_TEMP,                                       0, qfalse, qfalse },
	{ &g_banIPs,                          "g_banIPs",                          "",                           CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	{ &g_filterBan,                       "g_filterBan",                       "1",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	{ &g_dedicated,                       "dedicated",                         "0",                          0,                                               0, qfalse, qfalse },

	{ &g_speed,                           "g_speed",                           "320",                        0,                                               0, qtrue,  qtrue  },
	{ &g_gravity,                         "g_gravity",                         "800",                        0,                                               0, qtrue,  qtrue  },
	{ &g_knockback,                       "g_knockback",                       "1000",                       0,                                               0, qtrue,  qtrue  },

	{ &g_needpass,                        "g_needpass",                        "0",                          CVAR_SERVERINFO | CVAR_ROM,                      0, qtrue,  qfalse },
	{ &g_balancedteams,                   "g_balancedteams",                   "0",                          CVAR_SERVERINFO | CVAR_ROM,                      0, qtrue,  qfalse },
	{ &g_forcerespawn,                    "g_forcerespawn",                    "0",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_inactivity,                      "g_inactivity",                      "0",                          0,                                               0, qtrue,  qfalse },
	{ &g_debugMove,                       "g_debugMove",                       "0",                          0,                                               0, qfalse, qfalse },
	{ &g_debugDamage,                     "g_debugDamage",                     "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	{ &g_debugAlloc,                      "g_debugAlloc",                      "0",                          0,                                               0, qfalse, qfalse },
	{ &g_debugBullets,                    "g_debugBullets",                    "0",                          0,                                               0, qfalse, qfalse },
	{ &g_motd,                            "g_motd",                            "",                           CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	{ &voteFlags,                         "voteFlags",                         "0",                          CVAR_TEMP | CVAR_ROM | CVAR_SERVERINFO,          0, qfalse, qfalse },

	{ &g_complaintlimit,                  "g_complaintlimit",                  "6",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_teambleedComplaint,              "g_teambleedComplaint",              "50",                         CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_ipcomplaintlimit,                "g_ipcomplaintlimit",                "3",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_filtercams,                      "g_filtercams",                      "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_maxlives,                        "g_maxlives",                        "0",                          CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO,     0, qtrue,  qfalse },
	{ &g_maxlivesRespawnPenalty,          "g_maxlivesRespawnPenalty",          "0",                          CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO,     0, qtrue,  qfalse },
	{ &g_voiceChatsAllowed,               "g_voiceChatsAllowed",               "5",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	{ &g_alliedmaxlives,                  "g_alliedmaxlives",                  "0",                          CVAR_LATCH | CVAR_SERVERINFO,                    0, qtrue,  qfalse },
	{ &g_axismaxlives,                    "g_axismaxlives",                    "0",                          CVAR_LATCH | CVAR_SERVERINFO,                    0, qtrue,  qfalse },
	{ &g_fastres,                         "g_fastres",                         "0",                          CVAR_ARCHIVE,                                    0, qtrue,  qtrue  },     // Fast Medic Resing
	{ &g_enforcemaxlives,                 "g_enforcemaxlives",                 "1",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },     // Gestapo enforce maxlives stuff by temp banning

	{ &g_developer,                       "developer",                         "0",                          CVAR_TEMP,                                       0, qfalse, qfalse },
	{ &g_userAim,                         "g_userAim",                         "1",                          CVAR_CHEAT,                                      0, qfalse, qfalse },

	{ &pmove_fixed,                       "pmove_fixed",                       "0",                          CVAR_SYSTEMINFO,                                 0, qfalse, qfalse },
	{ &pmove_msec,                        "pmove_msec",                        "8",                          CVAR_SYSTEMINFO,                                 0, qfalse, qfalse },

	{ &g_scriptName,                      "g_scriptName",                      "",                           CVAR_CHEAT,                                      0, qfalse, qfalse },

	{ &g_antilag,                         "g_antilag",                         "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE,                  0, qfalse, qfalse },

	{ NULL,                               "P",                                 "",                           CVAR_SERVERINFO_NOUPDATE,                        0, qfalse, qfalse },

	{ &refereePassword,                   "refereePassword",                   "none",                       0,                                               0, qfalse, qfalse },
	{ &shoutcastPassword,                 "shoutcastPassword",                 "none",                       0,                                               0, qfalse, qfalse },
	{ &g_spectatorInactivity,             "g_spectatorInactivity",             "0",                          0,                                               0, qfalse, qfalse },
	{ &match_latejoin,                    "match_latejoin",                    "1",                          0,                                               0, qfalse, qfalse },
	{ &match_minplayers,                  "match_minplayers",                  MATCH_MINPLAYERS,             0,                                               0, qfalse, qfalse },
	{ &match_mutespecs,                   "match_mutespecs",                   "0",                          0,                                               0, qfalse, qtrue  },
	{ &match_readypercent,                "match_readypercent",                "100",                        0,                                               0, qfalse, qtrue  },
	{ &match_timeoutcount,                "match_timeoutcount",                "3",                          0,                                               0, qfalse, qtrue  },
	{ &match_timeoutlength,               "match_timeoutlength",               "180",                        0,                                               0, qfalse, qtrue  },
	{ &match_warmupDamage,                "match_warmupDamage",                "1",                          0,                                               0, qfalse, qfalse },
	{ &server_motd0,                      "server_motd0",                      " ^NEnemy Territory ^7MOTD ", 0,                                               0, qfalse, qfalse },
	{ &server_motd1,                      "server_motd1",                      "",                           0,                                               0, qfalse, qfalse },
	{ &server_motd2,                      "server_motd2",                      "",                           0,                                               0, qfalse, qfalse },
	{ &server_motd3,                      "server_motd3",                      "",                           0,                                               0, qfalse, qfalse },
	{ &server_motd4,                      "server_motd4",                      "",                           0,                                               0, qfalse, qfalse },
	{ &server_motd5,                      "server_motd5",                      "",                           0,                                               0, qfalse, qfalse },
	{ &team_maxplayers,                   "team_maxplayers",                   "0",                          0,                                               0, qfalse, qfalse },
	{ &team_nocontrols,                   "team_nocontrols",                   "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_config,                 "vote_allow_config",                 "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_gametype,               "vote_allow_gametype",               "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_kick,                   "vote_allow_kick",                   "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_map,                    "vote_allow_map",                    "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_matchreset,             "vote_allow_matchreset",             "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_mutespecs,              "vote_allow_mutespecs",              "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_nextmap,                "vote_allow_nextmap",                "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_referee,                "vote_allow_referee",                "0",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_shuffleteams,           "vote_allow_shuffleteams",           "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_shuffleteams_norestart, "vote_allow_shuffleteams_norestart", "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_swapteams,              "vote_allow_swapteams",              "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_friendlyfire,           "vote_allow_friendlyfire",           "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_timelimit,              "vote_allow_timelimit",              "0",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_warmupdamage,           "vote_allow_warmupdamage",           "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_antilag,                "vote_allow_antilag",                "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_balancedteams,          "vote_allow_balancedteams",          "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_muting,                 "vote_allow_muting",                 "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_limit,                        "vote_limit",                        "5",                          0,                                               0, qfalse, qfalse },
	{ &vote_percent,                      "vote_percent",                      "50",                         0,                                               0, qfalse, qfalse },
	{ &vote_allow_surrender,              "vote_allow_surrender",              "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_restartcampaign,        "vote_allow_restartcampaign",        "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_nextcampaign,           "vote_allow_nextcampaign",           "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_poll,                   "vote_allow_poll",                   "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_maprestart,             "vote_allow_maprestart",             "1",                          0,                                               0, qfalse, qfalse },
	{ &vote_allow_cointoss,               "vote_allow_cointoss",               "1",                          0,                                               0, qfalse, qfalse },

	{ &g_voting,                          "g_voting",                          "0",                          0,                                               0, qfalse, qfalse },

	// state vars
	{ &g_debugConstruct,                  "g_debugConstruct",                  "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },

	{ &g_scriptDebug,                     "g_scriptDebug",                     "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	// What level of detail do we want script printing to go to.
	{ &g_scriptDebugLevel,                "g_scriptDebugLevel",                "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	{ &g_scriptDebugTarget,               "g_scriptDebugTarget",               "",                           CVAR_CHEAT,                                      0, qfalse, qfalse },

	// How fast do we want Allied single player movement?
	{ &g_movespeed,                       "g_movespeed",                       "76",                         CVAR_CHEAT,                                      0, qfalse, qfalse },

#ifdef FEATURE_OMNIBOT
	// Omni-bot user defined path to load bot library from.
	{ &g_OmniBotPath,                     "omnibot_path",                      MODNAME "/omni-bot",          CVAR_ARCHIVE | CVAR_NORESTART,                   0, qfalse, qfalse },
	{ &g_OmniBotEnable,                   "omnibot_enable",                    "0",                          CVAR_ARCHIVE | CVAR_NORESTART,                   0, qfalse, qfalse },
	{ &g_OmniBotPlaying,                  "omnibot_playing",                   "0",                          CVAR_SERVERINFO_NOUPDATE | CVAR_ROM,             0, qfalse, qfalse },
	{ &g_OmniBotFlags,                    "omnibot_flags",                     "0",                          CVAR_ARCHIVE | CVAR_NORESTART,                   0, qfalse, qfalse },
#ifdef ETLEGACY_DEBUG
	{ &g_allowBotSwap,                    "g_allowBotSwap",                    "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
#endif
#endif

	// LMS
	{ &g_lms_teamForceBalance,            "g_lms_teamForceBalance",            "1",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_lms_roundlimit,                  "g_lms_roundlimit",                  "3",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_lms_matchlimit,                  "g_lms_matchlimit",                  "2",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_lms_currentMatch,                "g_lms_currentMatch",                "0",                          CVAR_ROM,                                        0, qfalse, qfalse },
	{ &g_lms_lockTeams,                   "g_lms_lockTeams",                   "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_lms_followTeamOnly,              "g_lms_followTeamOnly",              "1",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_axiswins,                        "g_axiswins",                        "0",                          CVAR_ROM,                                        0, qfalse, qfalse },
	{ &g_alliedwins,                      "g_alliedwins",                      "0",                          CVAR_ROM,                                        0, qfalse, qfalse },
	{ &g_axismapxp,                       "g_axismapxp",                       "0",                          CVAR_ROM,                                        0, qfalse, qfalse },
	{ &g_alliedmapxp,                     "g_alliedmapxp",                     "0",                          CVAR_ROM,                                        0, qfalse, qfalse },

	{ &g_currentCampaign,                 "g_currentCampaign",                 "",                           CVAR_WOLFINFO | CVAR_ROM,                        0, qfalse, qfalse },
	{ &g_currentCampaignMap,              "g_currentCampaignMap",              "0",                          CVAR_WOLFINFO | CVAR_ROM,                        0, qfalse, qfalse },

	{ &mod_version,                       "mod_version",                       ETLEGACY_VERSION,             CVAR_SERVERINFO | CVAR_ROM,                      0, qfalse, qfalse },
	// points to the URL for mod information, should not be modified by server admin
	{ &mod_url,                           "mod_url",                           MODURL,                       CVAR_SERVERINFO | CVAR_ROM,                      0, qfalse, qfalse },
	// configured by the server admin, points to the web pages for the server
	{ &url,                               "URL",                               "",                           CVAR_SERVERINFO | CVAR_ARCHIVE,                  0, qfalse, qfalse },

	{ &g_debugSkills,                     "g_debugSkills",                     "0",                          0,                                               0, qfalse, qfalse },

	{ &g_heavyWeaponRestriction,          "g_heavyWeaponRestriction",          "100",                        CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	{ &g_autoFireteams,                   "g_autoFireteams",                   "1",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	{ &g_nextmap,                         "nextmap",                           "",                           CVAR_TEMP,                                       0, qfalse, qfalse },
	{ &g_nextcampaign,                    "nextcampaign",                      "",                           CVAR_TEMP,                                       0, qfalse, qfalse },

	{ &g_disableComplaints,               "g_disableComplaints",               "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	// zinx etpro antiwarp
	{ &g_maxWarp,                         "g_maxWarp",                         "4",                          0,                                               0, qfalse, qfalse },
	{ &g_antiwarp,                        "g_antiwarp",                        "1",                          0,                                               0, qfalse, qfalse },
#ifdef FEATURE_LUA
	{ &lua_modules,                       "lua_modules",                       "",                           0,                                               0, qfalse, qfalse },
	{ &lua_allowedModules,                "lua_allowedModules",                "",                           0,                                               0, qfalse, qfalse },
	{ &g_luaModuleList,                   "g_luaModuleList",                   "",                           0,                                               0, qfalse, qfalse },
#endif

	{ &g_guidCheck,                       "g_guidCheck",                       "1",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_protect,                         "g_protect",                         "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_dropHealth,                      "g_dropHealth",                      "0",                          0,                                               0, qfalse, qfalse },
	{ &g_dropAmmo,                        "g_dropAmmo",                        "0",                          0,                                               0, qfalse, qfalse },
	{ &g_shove,                           "g_shove",                           "60",                         0,                                               0, qfalse, qfalse },
	{ &g_shoveNoZ,                        "g_shoveNoZ",                        "0",                          0,                                               0, qfalse, qfalse },

	// MAPVOTE
	{ &g_mapVoteFlags,                    "g_mapVoteFlags",                    "0",                          0,                                               0, qfalse, qfalse },
	{ &g_maxMapsVotedFor,                 "g_maxMapsVotedFor",                 "6",                          0,                                               0, qfalse, qfalse },
	{ &g_minMapAge,                       "g_minMapAge",                       "3",                          0,                                               0, qfalse, qfalse },
	{ &g_excludedMaps,                    "g_excludedMaps",                    "",                           0,                                               0, qfalse, qfalse },
	{ &g_resetXPMapCount,                 "g_resetXPMapCount",                 "0",                          0,                                               0, qfalse, qfalse },

	{ &g_campaignFile,                    "g_campaignFile",                    "",                           0,                                               0, qfalse, qfalse },

	{ &g_countryflags,                    "g_countryflags",                    "1",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },

	{ &team_maxAirstrikes,                "team_maxAirstrikes",                "0.0",                        0,                                               0, qfalse, qfalse },
	{ &team_maxArtillery,                 "team_maxArtillery",                 "0.0",                        0,                                               0, qfalse, qfalse },
	// team class/weapon limiting
	//classes
	{ &team_maxSoldiers,                  "team_maxSoldiers",                  "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxMedics,                    "team_maxMedics",                    "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxEngineers,                 "team_maxEngineers",                 "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxFieldops,                  "team_maxFieldops",                  "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxCovertops,                 "team_maxCovertops",                 "-1",                         0,                                               0, qfalse, qfalse },
	//weapons
	{ &team_maxMortars,                   "team_maxMortars",                   "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxFlamers,                   "team_maxFlamers",                   "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxMachineguns,               "team_maxMachineguns",               "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxRockets,                   "team_maxRockets",                   "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxRiflegrenades,             "team_maxRiflegrenades",             "-1",                         0,                                               0, qfalse, qfalse },
	{ &team_maxLandmines,                 "team_maxLandmines",                 "10",                         0,                                               0, qfalse, qfalse },
	//misc
	{ &team_riflegrenades,                "team_riflegrenades",                "1",                          0,                                               0, qfalse, qfalse },
	//skills
	{ &skill_soldier,                     "skill_soldier",                     "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &skill_medic,                       "skill_medic",                       "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &skill_fieldops,                    "skill_fieldops",                    "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &skill_engineer,                    "skill_engineer",                    "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &skill_covertops,                   "skill_covertops",                   "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &skill_battlesense,                 "skill_battlesense",                 "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &skill_lightweapons,                "skill_lightweapons",                "20 50 90 140",               CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_misc,                            "g_misc",                            "0",                          0,                                               0, qfalse, qfalse },
	{ &g_intermissionTime,                "g_intermissionTime",                "60",                         0,                                               0, qfalse, qfalse },
	{ &g_intermissionReadyPercent,        "g_intermissionReadyPercent",        "100",                        0,                                               0, qfalse, qfalse },
	{ &g_mapScriptDirectory,              "g_mapScriptDirectory",              "mapscripts",                 0,                                               0, qfalse, qfalse },
	{ &g_mapConfigs,                      "g_mapConfigs",                      "",                           0,                                               0, qfalse, qfalse },
	{ &g_customConfig,                    "g_customConfig",                    "defaultpublic",              CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_moverScale,                      "g_moverScale",                      "1.0",                        0,                                               0, qfalse, qfalse },
	{ &g_fixedphysics,                    "g_fixedphysics",                    "1",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_fixedphysicsfps,                 "g_fixedphysicsfps",                 "125",                        CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_pronedelay,                      "g_pronedelay",                      "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	// Debug
	{ &g_debugHitboxes,                   "g_debugHitboxes",                   "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	{ &g_debugPlayerHitboxes,             "g_debugPlayerHitboxes",             "0",                          0,                                               0, qfalse, qfalse },     // no need to make this CVAR_CHEAT
	{ &g_debugForSingleClient,            "g_debugForSingleClient",            "-1",                         0,                                               0, qfalse, qfalse },     // no need to make this CVAR_CHEAT
	{ &g_debugEvents,                     "g_debugevents",                     "0",                          0,                                               0, qfalse, qfalse },
	{ &g_debugAnim,                       "g_debuganim",                       "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },

	{ &g_corpses,                         "g_corpses",                         "0",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },
	{ &g_realHead,                        "g_realHead",                        "1",                          0,                                               0, qfalse, qfalse },
	{ &g_skipCorrection,                  "g_skipCorrection",                  "1",                          0,                                               0, qfalse, qfalse },
	{ &g_extendedNames,                   "g_extendedNames",                   "1",                          0,                                               0, qfalse, qfalse },
#ifdef FEATURE_RATING
	{ &g_skillRating,                     "g_skillRating",                     "2",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },
#endif
#ifdef FEATURE_PRESTIGE
	{ &g_prestige,                        "g_prestige",                        "1",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },
#endif
#ifdef FEATURE_MULTIVIEW
	{ &g_multiview,                       "g_multiview",                       "0",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },
#endif
	{ &g_stickyCharge,                    "g_stickyCharge",                    "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_xpSaver,                         "g_xpSaver",                         "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_suddenDeath,                     "g_suddenDeath",                     "0",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_dropObjDelay,                    "g_dropObjDelay",                    "3000",                       CVAR_ARCHIVE,                                    0, qtrue,  qfalse },

	{ &g_floodProtection,                 "g_floodProtection",                 "1",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_floodLimit,                      "g_floodLimit",                      "5",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_floodWait,                       "g_floodWait",                       "1000",                       CVAR_ARCHIVE,                                    0, qtrue,  qfalse },

	{ &g_etltv_flags,                     "g_etltv_flags",                     "3",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
};

/**
 * @var gameCvarTableSize
 * @brief Made static to avoid aliasing
 */
static int gameCvarTableSize = sizeof(gameCvarTable) / sizeof(gameCvarTable[0]);

// Functions {{{1

/**
 * @brief G_RegisterCvars
 */
void G_RegisterCvars(void)
{
	int         i;
	cvarTable_t *cv;

	level.server_settings = 0;

	G_Printf("%d cvars in use\n", gameCvarTableSize);

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar)
		{
			cv->modificationCount = cv->vmCvar->modificationCount;
			// update vote info for clients, if necessary
			G_checkServerToggle(cv->vmCvar);
		}
	}

	// check some things
	// Gametype is currently restricted to supported types only
	if ((g_gametype.integer < GT_WOLF || g_gametype.integer >= GT_MAX_GAME_TYPE))
	{
		trap_Cvar_Set("g_gametype", va("%i", GT_WOLF));
		trap_Cvar_Update(&g_gametype);
		// FIXME: auto restart?
		// g_gametype is latched and won't use the above value for current game. but running league with invalid gametype is resulting in bad behaviour
		// let's drop the game... (unfortunately we can't immediately restart the server here (exec map_restart isn't working)
		G_Error("Invalid game type %i detected - defaulting to %s (%i). Start your server again with no gametype set!\n", g_gametype.integer, gameNames[GT_WOLF], GT_WOLF);
	}

	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));

	if (match_readypercent.integer < 1)
	{
		trap_Cvar_Set("match_readypercent", "1");
		trap_Cvar_Update(&match_readypercent);
	}

	if (pmove_msec.integer < 8)
	{
		trap_Cvar_Set("pmove_msec", "8");
		trap_Cvar_Update(&pmove_msec);
	}
	else if (pmove_msec.integer > 33)
	{
		trap_Cvar_Set("pmove_msec", "33");
		trap_Cvar_Update(&pmove_msec);
	}
}

/**
 * @brief G_UpdateCvars
 */
void G_UpdateCvars(void)
{
	int         i;
	cvarTable_t *cv;
	qboolean    fToggles           = qfalse;
	qboolean    fVoteFlags         = qfalse;
	qboolean    chargetimechanged  = qfalse;
	qboolean    clsweaprestriction = qfalse;
	qboolean    skillLevelPoints   = qfalse;

	for (i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++)
	{
		if (cv->vmCvar)
		{
			trap_Cvar_Update(cv->vmCvar);

			if (cv->modificationCount != cv->vmCvar->modificationCount)
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				if (cv->trackChange && !(cv->cvarFlags & CVAR_LATCH))
				{
					trap_SendServerCommand(-1, va("print \"[lon]Server:[lof] %s [lon]changed to[lof] %s\n\"", cv->cvarName, cv->vmCvar->string));
				}

				if (cv->vmCvar == &g_filtercams)
				{
					trap_SetConfigstring(CS_FILTERCAMS, va("%i", g_filtercams.integer));
				}
				else if (cv->vmCvar == &g_soldierChargeTime)
				{
					level.soldierChargeTime[0] = g_soldierChargeTime.integer * level.soldierChargeTimeModifier[0];
					level.soldierChargeTime[1] = g_soldierChargeTime.integer * level.soldierChargeTimeModifier[1];
					chargetimechanged          = qtrue;
				}
				else if (cv->vmCvar == &g_medicChargeTime)
				{
					level.medicChargeTime[0] = g_medicChargeTime.integer * level.medicChargeTimeModifier[0];
					level.medicChargeTime[1] = g_medicChargeTime.integer * level.medicChargeTimeModifier[1];
					chargetimechanged        = qtrue;
				}
				else if (cv->vmCvar == &g_engineerChargeTime)
				{
					level.engineerChargeTime[0] = g_engineerChargeTime.integer * level.engineerChargeTimeModifier[0];
					level.engineerChargeTime[1] = g_engineerChargeTime.integer * level.engineerChargeTimeModifier[1];
					chargetimechanged           = qtrue;
				}
				else if (cv->vmCvar == &g_fieldopsChargeTime)
				{
					level.fieldopsChargeTime[0] = g_fieldopsChargeTime.integer * level.fieldopsChargeTimeModifier[0];
					level.fieldopsChargeTime[1] = g_fieldopsChargeTime.integer * level.fieldopsChargeTimeModifier[1];
					chargetimechanged           = qtrue;
				}
				else if (cv->vmCvar == &g_covertopsChargeTime)
				{
					level.covertopsChargeTime[0] = g_covertopsChargeTime.integer * level.covertopsChargeTimeModifier[0];
					level.covertopsChargeTime[1] = g_covertopsChargeTime.integer * level.covertopsChargeTimeModifier[1];
					chargetimechanged            = qtrue;
				}
				else if (cv->vmCvar == &match_readypercent)
				{
					if (match_readypercent.integer < 1)
					{
						trap_Cvar_Set(cv->cvarName, "1");
					}
					else if (match_readypercent.integer > 100)
					{
						trap_Cvar_Set(cv->cvarName, "100");
					}
				}
				else if (cv->vmCvar == &g_warmup)
				{
					if (g_gamestate.integer != GS_PLAYING)
					{
						level.warmupTime = level.time + (((g_warmup.integer < 10) ? 11 : g_warmup.integer + 1) * 1000);
						trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
					}
				}
				// Moved this check out of the main world think loop
				else if (cv->vmCvar == &g_gametype)
				{
					int  worldspawnflags = g_entities[ENTITYNUM_WORLD].spawnflags;
					int  gt, gametype;
					char buffer[32];

					trap_Cvar_LatchedVariableStringBuffer("g_gametype", buffer, sizeof(buffer));
					gametype = Q_atoi(buffer);

					if (gametype == GT_WOLF_CAMPAIGN && gametype != g_gametype.integer)
					{
						if (!G_MapIsValidCampaignStartMap())
						{
							gt = g_gametype.integer;
							if (gt != GT_WOLF_LMS)
							{
								if (!(worldspawnflags & NO_GT_WOLF))
								{
									gt = GT_WOLF;   // Default wolf
								}
								else
								{
									gt = GT_WOLF_LMS;   // Last man standing
								}
							}

							G_Printf("Map '%s' isn't a valid campaign start map, resetting game type to '%i'\n", level.rawmapname, gt);
							trap_Cvar_Set("g_gametype", va("%i", gt));
						}
						continue;
					}

					if (!level.latchGametype && g_gamestate.integer == GS_PLAYING &&
					    (((g_gametype.integer == GT_WOLF || g_gametype.integer == GT_WOLF_CAMPAIGN || g_gametype.integer == GT_WOLF_MAPVOTE) && (worldspawnflags & NO_GT_WOLF)) ||
					     (g_gametype.integer == GT_WOLF_STOPWATCH && (worldspawnflags & NO_STOPWATCH)) ||
					     (g_gametype.integer == GT_WOLF_LMS && (worldspawnflags & NO_LMS)))
					    )
					{

						if (!(worldspawnflags & NO_GT_WOLF))
						{
							gt = GT_WOLF;   // Default wolf
						}
						else
						{
							gt = GT_WOLF_LMS;   // Last man standing
						}

						level.latchGametype = qtrue;
						AP("print \"Invalid gametype was specified, Restarting\n\"");
						trap_SendConsoleCommand(EXEC_APPEND, va("wait 2 ; g_gametype %i ; map_restart 10 0\n", gt));
					}
				}
				else if (cv->vmCvar == &pmove_msec)
				{
					if (pmove_msec.integer < 8)
					{
						trap_Cvar_Set(cv->cvarName, "8");
					}
					else if (pmove_msec.integer > 33)
					{
						trap_Cvar_Set(cv->cvarName, "33");
					}
				}
				else if (cv->vmCvar == &team_maxSoldiers || cv->vmCvar == &team_maxMedics || cv->vmCvar == &team_maxEngineers || cv->vmCvar == &team_maxFieldops || cv->vmCvar == &team_maxCovertops || cv->vmCvar == &team_maxMortars || cv->vmCvar == &team_maxFlamers || cv->vmCvar == &team_maxMachineguns || cv->vmCvar == &team_maxRockets || cv->vmCvar == &team_maxRiflegrenades || cv->vmCvar == &team_maxplayers)
				{
					clsweaprestriction = qtrue;
				}
				else if (cv->vmCvar == &skill_battlesense || cv->vmCvar == &skill_covertops || cv->vmCvar == &skill_engineer || cv->vmCvar == &skill_fieldops || cv->vmCvar == &skill_lightweapons || cv->vmCvar == &skill_medic || cv->vmCvar == &skill_soldier)
				{
					G_SetSkillLevelsByCvar(cv->vmCvar);
					skillLevelPoints = qtrue;
				}
				else if (cv->vmCvar == &shoutcastPassword)
				{
					// logout all currently logged in shoutcasters
					// when changing shoutcastPassword to '' or 'none'
					if (!Q_stricmp(shoutcastPassword.string, "none") || !shoutcastPassword.string[0])
					{
						G_RemoveAllShoutcasters();
					}
				}
#ifdef FEATURE_LUA
				else if (cv->vmCvar == &lua_modules || cv->vmCvar == &lua_allowedModules)
				{
					G_LuaShutdown();
				}
#endif

				{
					// CS_MODINFO
					char cs[MAX_INFO_STRING];

					cs[0] = '\0';

					// MAPVOTE
					// FIXME: mapvote & xp
					if (g_gametype.integer == GT_WOLF_MAPVOTE)
					{
						Info_SetValueForKey(cs, "X", va("%i", (level.mapsSinceLastXPReset >= g_resetXPMapCount.integer) ? 0 : level.mapsSinceLastXPReset));
						Info_SetValueForKey(cs, "Y", (va("%i", g_resetXPMapCount.integer)));
					}

#ifdef FEATURE_RATING
					Info_SetValueForKey(cs, "R", va("%i", g_skillRating.integer));

					if (g_skillRating.integer > 1)
					{
						Info_SetValueForKey(cs, "M", va("%f", level.mapProb));
					}
#endif

#ifdef FEATURE_PRESTIGE
					Info_SetValueForKey(cs, "P", va("%i", g_prestige.integer));
#endif

#ifdef FEATURE_MULTIVIEW
					Info_SetValueForKey(cs, "MV", va("%i", g_multiview.integer));
#endif
					Info_SetValueForKey(cs, "fp", va("%i", g_fixedphysics.integer));
					Info_SetValueForKey(cs, "fpv", va("%i", g_fixedphysicsfps.integer));
					Info_SetValueForKey(cs, "pd", va("%i", g_pronedelay.integer));


					trap_SetConfigstring(CS_MODINFO, cs);
				}

				// Update vote info for clients, if necessary
				if (cv->vmCvar == &vote_allow_kick            || cv->vmCvar == &vote_allow_map            ||
				    cv->vmCvar == &vote_allow_matchreset      || cv->vmCvar == &vote_allow_gametype       ||
				    cv->vmCvar == &vote_allow_mutespecs       || cv->vmCvar == &vote_allow_nextmap        ||
				    cv->vmCvar == &vote_allow_config          || cv->vmCvar == &vote_allow_referee        ||
				    cv->vmCvar == &vote_allow_shuffleteams    || cv->vmCvar == &vote_allow_shuffleteams_norestart ||
				    cv->vmCvar == &vote_allow_swapteams       || cv->vmCvar == &vote_allow_friendlyfire   ||
				    cv->vmCvar == &vote_allow_timelimit       || cv->vmCvar == &vote_allow_warmupdamage   ||
				    cv->vmCvar == &vote_allow_antilag         || cv->vmCvar == &vote_allow_balancedteams  ||
				    cv->vmCvar == &vote_allow_muting          || cv->vmCvar == &vote_allow_surrender      ||
				    cv->vmCvar == &vote_allow_restartcampaign || cv->vmCvar == &vote_allow_nextcampaign   ||
				    cv->vmCvar == &vote_allow_poll            || cv->vmCvar == &vote_allow_maprestart     ||
				    cv->vmCvar == &vote_allow_cointoss)
				{
					fVoteFlags = qtrue;
				}
				else
				{
					fToggles = (G_checkServerToggle(cv->vmCvar) || fToggles);
				}
			}
		}
	}

	if (fVoteFlags)
	{
		G_voteFlags();
	}

	if (fToggles)
	{
		trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
	}

	if (chargetimechanged)
	{
		char cs[MAX_INFO_STRING];

		cs[0] = '\0';
		Info_SetValueForKey(cs, "x0", va("%i", level.soldierChargeTime[0]));
		Info_SetValueForKey(cs, "a0", va("%i", level.soldierChargeTime[1]));
		Info_SetValueForKey(cs, "x1", va("%i", level.medicChargeTime[0]));
		Info_SetValueForKey(cs, "a1", va("%i", level.medicChargeTime[1]));
		Info_SetValueForKey(cs, "x2", va("%i", level.engineerChargeTime[0]));
		Info_SetValueForKey(cs, "a2", va("%i", level.engineerChargeTime[1]));
		Info_SetValueForKey(cs, "x3", va("%i", level.fieldopsChargeTime[0]));
		Info_SetValueForKey(cs, "a3", va("%i", level.fieldopsChargeTime[1]));
		Info_SetValueForKey(cs, "x4", va("%i", level.covertopsChargeTime[0]));
		Info_SetValueForKey(cs, "a4", va("%i", level.covertopsChargeTime[1]));
		trap_SetConfigstring(CS_CHARGETIMES, cs);
	}

	if (clsweaprestriction)
	{
		char cs[MAX_INFO_STRING];

		cs[0] = '\0';

		Info_SetValueForKey(cs, "c0", team_maxSoldiers.string);
		Info_SetValueForKey(cs, "c1", team_maxMedics.string);
		Info_SetValueForKey(cs, "c2", team_maxEngineers.string);
		Info_SetValueForKey(cs, "c3", team_maxFieldops.string);
		Info_SetValueForKey(cs, "c4", team_maxCovertops.string);
		Info_SetValueForKey(cs, "w0", team_maxMortars.string);
		Info_SetValueForKey(cs, "w1", team_maxFlamers.string);
		Info_SetValueForKey(cs, "w2", team_maxMachineguns.string);
		Info_SetValueForKey(cs, "w3", team_maxRockets.string);
		Info_SetValueForKey(cs, "w4", team_maxRiflegrenades.string);
		Info_SetValueForKey(cs, "w5", team_maxLandmines.string);
		Info_SetValueForKey(cs, "m", team_maxplayers.string);
		trap_SetConfigstring(CS_TEAMRESTRICTIONS, cs);
	}

	if (skillLevelPoints)
	{
		int x;
		for (i = 0; i < level.numConnectedClients; i++)
		{
			//somewhat of waste of bandwidth but this should not happen very ofter (next to never)
			for (x = 0; x < SK_NUM_SKILLS; x++)
			{
				int oldskill = g_entities[level.sortedClients[i]].client->sess.skill[x];
				G_SetPlayerSkill(g_entities[level.sortedClients[i]].client, x);
				if (oldskill != g_entities[level.sortedClients[i]].client->sess.skill[x])
				{
					// call the new func that encapsulates the skill giving behavior
					G_UpgradeSkill(&g_entities[level.sortedClients[i]], x);
				}
			}
		}
		G_UpdateSkillsToClients();
	}
}

/**
 * @brief Reset particular server variables back to defaults if a config is voted in.
 */
void G_wipeCvars(void)
{
	int         i;
	cvarTable_t *pCvars;

	for (i = 0, pCvars = gameCvarTable; i < gameCvarTableSize; i++, pCvars++)
	{
		if (pCvars->vmCvar && pCvars->fConfigReset)
		{
			G_Printf("set %s %s\n", pCvars->cvarName, pCvars->defaultString);
			trap_Cvar_Set(pCvars->cvarName, pCvars->defaultString);
		}
	}

	G_UpdateCvars();
}
