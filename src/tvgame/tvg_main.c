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
 * @file tvg_main.c
 */

#include "tvg_local.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

#define Q_OSS_STR_INC
#include "../qcommon/q_oss.h"

#include "json.h"

level_locals_t level;

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

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

const char *gameNames[] =
{
	"Single Player",        // Obsolete
	"Cooperative",          // Obsolete
	"Objective",
	"Stopwatch",
	"Campaign",
	"Last Man Standing",
	"Map Voting"            // GT_WOLF_MAPVOTE
	// GT_MAX_GAME_TYPE
};

#define MAPVOTEINFO_FILE_NAME "mapvoteinfo.txt"

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

vmCvar_t voteFlags;
vmCvar_t g_complaintlimit;
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

vmCvar_t g_debugForSingleClient;

// flood protection
vmCvar_t g_floodProtection;
vmCvar_t g_floodLimit;
vmCvar_t g_floodWait;

vmCvar_t tvg_queue_ms;

cvarTable_t gameCvarTable[] =
{
	// don't override the cheat state set by the system
	{ &g_cheats,                          "sv_cheats",                         "",                           0,                                               0, qfalse, qfalse },

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
	{ &g_forcerespawn,                    "g_forcerespawn",                    "0",                          0,                                               0, qtrue,  qfalse },
	{ &g_inactivity,                      "g_inactivity",                      "0",                          0,                                               0, qtrue,  qfalse },
	{ &g_debugMove,                       "g_debugMove",                       "0",                          0,                                               0, qfalse, qfalse },
	{ &g_debugDamage,                     "g_debugDamage",                     "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	{ &g_debugAlloc,                      "g_debugAlloc",                      "0",                          0,                                               0, qfalse, qfalse },
	{ &g_debugBullets,                    "g_debugBullets",                    "0",                          0,                                               0, qfalse, qfalse },
	{ &g_motd,                            "g_motd",                            "",                           CVAR_ARCHIVE,                                    0, qfalse, qfalse },

	{ &voteFlags,                         "voteFlags",                         "0",                          CVAR_TEMP | CVAR_ROM | CVAR_SERVERINFO,          0, qfalse, qfalse },

	{ &g_complaintlimit,                  "g_complaintlimit",                  "6",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
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
	{ &g_fixedphysics,                    "g_fixedphysics",                    "1",                          CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	{ &g_fixedphysicsfps,                 "g_fixedphysicsfps",                 "125",                        CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	{ &g_pronedelay,                      "g_pronedelay",                      "0",                          CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	// Debug
	{ &g_debugHitboxes,                   "g_debugHitboxes",                   "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	{ &g_debugPlayerHitboxes,             "g_debugPlayerHitboxes",             "0",                          0,                                               0, qfalse, qfalse },     // no need to make this CVAR_CHEAT
	{ &g_debugForSingleClient,            "g_debugForSingleClient",            "-1",                         0,                                               0, qfalse, qfalse },     // no need to make this CVAR_CHEAT

	{ &g_corpses,                         "g_corpses",                         "0",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },
	{ &g_realHead,                        "g_realHead",                        "1",                          0,                                               0, qfalse, qfalse },
	{ &sv_fps,                            "sv_fps",                            "20",                         CVAR_SYSTEMINFO,                                 0, qfalse, qfalse },
	{ &g_skipCorrection,                  "g_skipCorrection",                  "1",                          0,                                               0, qfalse, qfalse },
	{ &g_extendedNames,                   "g_extendedNames",                   "1",                          0,                                               0, qfalse, qfalse },

	{ &g_floodProtection,                 "g_floodProtection",                 "1",                          CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qtrue,  qfalse },
	{ &g_floodLimit,                      "g_floodLimit",                      "5",                          CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &g_floodWait,                       "g_floodWait",                       "1000",                       CVAR_ARCHIVE,                                    0, qtrue,  qfalse },
	{ &tvg_queue_ms,                      "ettv_queue_ms",                     "-1",                         CVAR_ROM,                                        0, qfalse, qfalse },
};

/**
 * @var gameCvarTableSize
 * @brief Made static to avoid aliasing
 */
static int gameCvarTableSize = sizeof(gameCvarTable) / sizeof(gameCvarTable[0]);

/**
 * @var fActions
 * @brief Flag to store executed final auto-actions
 */
static int fActions = 0;

void TVG_InitGame(int levelTime, int randomSeed, int restart, int legacyServer, int serverVersion);
void TVG_RunFrame(int levelTime);
void TVG_ShutdownGame(int restart);
void TVG_ParsePlatformManifest(void);

/**
 * @brief G_SnapshotCallback
 * @param[in] entityNum
 * @param[in] clientNum
 * @return
 */
qboolean TVG_SnapshotCallback(int entityNum, int clientNum)
{
	//gentity_t *ent = &g_entities[entityNum];

	//Com_Printf("callback entityNum: %d clientNum: %d\n", entityNum, clientNum);

	//if (ent->s.eType == ET_MISSILE)
	//{
	//	if (ent->s.weapon == WP_LANDMINE)
	//	{
	//		return G_LandmineSnapshotCallback(entityNum, clientNum);
	//	}
	//}

	//// don't send if out of range
	//if (ent->s.eType == ET_EVENTS + EV_SHAKE)
	//{
	//	float len = VectorDistance(g_entities[clientNum].client->ps.origin, ent->s.pos.trBase);

	//	return len <= ent->s.onFireStart;
	//}

	return qtrue;
}

int dll_com_trapGetValue;
int dll_trap_DemoSupport;

static void TVG_ETTV_ConfigstringPassthrough(int index)
{
	char cs[MAX_TOKEN_CHARS];

	if (index == CS_SYSTEMINFO)
	{
		return;
	}

	trap_GetConfigstring(index, cs, sizeof(cs));
	trap_TVG_SetConfigstring(index, cs);
}

/**
 * @brief This is the only way control passes into the module.
 * This must be the very first function compiled into the .q3vm file
 *
 * @param[in] command
 * @param[in] arg0
 * @param[in] arg1
 * @param[in] arg2
 * @param[in] arg3
 * @param[in] arg4
 * @param arg5 - unused
 * @param arg6 - unused
 * @return
 */
Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6)
{
	switch (command)
	{
	case GAME_INIT:
	{
		int time = trap_Milliseconds();
		Com_Printf(S_COLOR_MDGREY "Initializing %s game " S_COLOR_GREEN ETLEGACY_VERSION "\n", MODNAME);
		TVG_ParsePlatformManifest();
		TVG_InitGame(arg0, arg1, arg2, arg3, arg4);
		G_Printf("Game Initialization completed in %.2f seconds\n", (float)(trap_Milliseconds() - time) / 1000.f);
	}
		return 0;
	case GAME_SHUTDOWN:
		TVG_ShutdownGame(arg0);
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)TVG_ClientConnect(arg0, arg1, arg2);
	case GAME_CLIENT_THINK:
		TVG_ClientThink(arg0);
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		TVG_ClientUserinfoChanged(arg0);
		return 0;
	case GAME_CLIENT_DISCONNECT:
		TVG_ClientDisconnect(arg0);
		return 0;
	case GAME_CLIENT_BEGIN:
		TVG_ClientBegin(arg0);
		return 0;
	case GAME_CLIENT_COMMAND:
		TVG_ClientCommand(arg0);
		return 0;
	case GAME_RUN_FRAME:
		TVG_RunFrame(arg0);
		return 0;
	case GAME_CONSOLE_COMMAND:
		return TVG_ConsoleCommand();
	case GAME_SNAPSHOT_CALLBACK:
		return TVG_SnapshotCallback(arg0, arg1);
	case GAME_MESSAGERECEIVED:
		return -1;
	case GAME_DEMOSTATECHANGED:
		return 0;
	case GAME_ETTV:
		TVG_ETTV_ConfigstringPassthrough(arg0);
		return 0;
	default:
		G_Printf("Bad game export type: %ld\n", (long int) command);
		break;
	}

	return -1;
}

/**
 * @brief G_Printf
 * @param[in] fmt
 */
void QDECL G_Printf(const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

#ifdef FEATURE_LUA
	// LUA* API callbacks
	G_LuaHook_Print(GPRINT_TEXT, text);
#endif

	trap_Printf(text);
}

void QDECL G_Printf(const char *fmt, ...) _attribute((format(printf, 1, 2)));

/**
 * @brief G_DPrintf
 * @param[in] fmt
 */
void QDECL G_DPrintf(const char *fmt, ...)
{
	if (!g_developer.integer)
	{
		return;
	}
	else
	{
		va_list argptr;
		char    text[1024];

		va_start(argptr, fmt);
		Q_vsnprintf(text, sizeof(text), fmt, argptr);
		va_end(argptr);

#ifdef FEATURE_LUA
		// LUA* API callbacks
		G_LuaHook_Print(GPRINT_DEVELOPER, text);
#endif

		trap_Printf(text);
	}
}

void QDECL G_DPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));

/**
 * @brief G_Error
 * @param[in] fmt
 */
void QDECL G_Error(const char *fmt, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

#ifdef FEATURE_LUA
	// LUA* API callbacks
	G_LuaHook_Print(GPRINT_ERROR, text);
#endif

	trap_Error(text);
}

void QDECL G_Error(const char *fmt, ...) _attribute((format(printf, 1, 2)));


/**
 * @brief TVG_ServerIsFloodProtected
 * @return
 */
qboolean TVG_ServerIsFloodProtected(void)
{
	return (!g_floodProtection.integer || !g_floodWait.integer || !g_floodLimit.integer) ? qfalse : qtrue;
}

/**
 * @brief TVG_RegisterCvars
 */
void TVG_RegisterCvars(void)
{
	int         i;
	cvarTable_t *cv;

	G_Printf("%d cvars in use\n", gameCvarTableSize);

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar)
		{
			cv->modificationCount = cv->vmCvar->modificationCount;
		}
	}
}

/**
 * @brief TVG_UpdateCvars
 */
void TVG_UpdateCvars(void)
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
#ifdef FEATURE_LUA
				else if (cv->vmCvar == &lua_modules || cv->vmCvar == &lua_allowedModules)
				{
					G_LuaShutdown();
				}
#endif
			}
		}
	}
}

static ID_INLINE void TVG_SetupExtensionTrap(char *value, int valueSize, int *trap, const char *name)
{
	if (trap_GetValue(value, valueSize, name))
	{
		*trap = Q_atoi(value);
	}
	else
	{
		*trap = qfalse;
	}
}

static ID_INLINE void TVG_SetupExtensions(void)
{
	char value[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));
	if (value[0])
	{
		dll_com_trapGetValue = Q_atoi(value);

		TVG_SetupExtensionTrap(value, MAX_CVAR_VALUE_STRING, &dll_trap_DemoSupport, "trap_DemoSupport_Legacy");
	}
}

extern void G_InitMemory(void);

/**
 * @brief TVG_InitGame
 * @param[in] levelTime
 * @param[in] randomSeed
 * @param[in] restart
 * @param[in] etLegacyServer
 * @param[in] serverVersion
 */
void TVG_InitGame(int levelTime, int randomSeed, int restart, int etLegacyServer, int serverVersion)
{
	char   cs[MAX_INFO_STRING];
	time_t aclock;
	char   timeFt[32];

	// mod version check
	MOD_CHECK_ETLEGACY(etLegacyServer, serverVersion, level.etLegacyServer);

	G_Printf("------- Game Initialization -------\n");
	G_Printf("gamename: %s\n", MODNAME);
	G_Printf("gamedate: %s\n", __DATE__);

	srand(randomSeed);

	if (g_maxclients.integer > MAX_CLIENTS)
	{
		G_Error("Error: tvgame does not support %d client slots.\n", g_maxclients.integer);
	}

	TVG_RegisterCvars();

	TVG_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	{
		qboolean oldspawning = level.spawning;

		Com_Memset(&level, 0, sizeof(level));

		level.spawning = oldspawning;
	}

	TVG_SetupExtensions();

	level.time      = levelTime;
	level.startTime = levelTime;

	// init the anim scripting
	level.animScriptData.soundIndex = G_SoundIndex;
	level.animScriptData.playSound  = TVG_AnimScriptSound;
	// time
	time(&aclock);
	strftime(timeFt, sizeof(timeFt), "%a %b %d %X %Y", localtime(&aclock));

	if (g_log.string[0])
	{
		if (trap_FS_FOpenFile(g_log.string, &level.logFile, g_logSync.integer ? FS_APPEND_SYNC : FS_APPEND) < 0)
		{
			G_Printf("WARNING: Couldn't open logfile: %s\n", g_log.string);
		}
		else
		{
			G_LogPrintf("------------------------------------------------------------\n");
			G_LogPrintf("InitTVGame\n");
		}
	}
	else
	{
		G_Printf("Not logging to disk\n");
	}

	trap_GetServerinfo(cs, sizeof(cs));
	Q_strncpyz(level.rawmapname, Info_ValueForKey(cs, "mapname"), sizeof(level.rawmapname));

	G_LogPrintf("map: %s\n", level.rawmapname);

	// array acces check is done in G_RegisterCvars - we won't execute this with invalid gametype
	G_LogPrintf("gametype: %s\n", gameNames[g_gametype.integer]);

	G_LogPrintf("gametime: %s\n", timeFt);

	// initialize all entities for this game
	Com_Memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	Com_Memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entities are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t),
	                    &level.clients[0].ps, sizeof(level.clients[0]));

	// disable server engine flood protection if we have mod-sided flood protection enabled
	// since they don't block the same commands
	if (TVG_ServerIsFloodProtected())
	{
		int sv_floodprotect = trap_Cvar_VariableIntegerValue("sv_floodprotect");
		if (sv_floodprotect)
		{
			trap_Cvar_Set("sv_floodprotect", "0");
			G_Printf("^3INFO: mod-sided flood protection enabled, disabling server-side protection.\n");
		}
	}

#ifdef FEATURE_LUA
	G_LuaInit();
#else
	G_Printf("%sNo Lua API available\n", S_COLOR_BLUE);
#endif

	// parse the key/value pairs and spawn gentities
	TVG_SpawnEntitiesFromString();

	TVG_FindIntermissionPoint();

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", level.rawmapname));

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	G_RegisterPlayerClasses();

#ifdef FEATURE_LUA
	G_LuaHook_InitGame(levelTime, randomSeed, restart);
#endif
}

/**
 * @brief TVG_ShutdownGame
 * @param[in] restart
 */
void TVG_ShutdownGame(int restart)
{
	time_t aclock;
	char   timeFt[32];

#ifdef FEATURE_LUA
	G_LuaHook_ShutdownGame(restart);
	G_LuaShutdown();
#endif

	G_Printf("==== TVShutdownGame (%i - %s) ====\n", restart, level.rawmapname);

	// time
	time(&aclock);
	strftime(timeFt, sizeof(timeFt), "%a %b %d %X %Y", localtime(&aclock));
	G_Printf("gametime: %s\n", timeFt);

	if (level.logFile)
	{
		G_LogPrintf("TVShutdownGame:\n");
		G_LogPrintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}

	// write all the client session data so we can get it back
	TVG_WriteSessionData(restart);
}

//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

/**
 * @brief Com_Error
 * @param code - unused
 * @param[in] error
 */
void QDECL Com_Error(int code, const char *error, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	G_Error("%s", text);
}

void QDECL Com_Error(int code, const char *error, ...) _attribute((format(printf, 2, 3)));

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

	G_Printf("%s", text);
}

void QDECL Com_Printf(const char *msg, ...) _attribute((format(printf, 1, 2)));

#endif

/*
========================================================================
PLAYER COUNTING / SCORE SORTING
========================================================================
*/

/**
 * @brief TVG_CalculateRanks
 */
void TVG_CalculateRanks(void)
{
	int i;

	level.numConnectedClients = 0;

	for (i = 0 ; i < level.maxclients ; i++)
	{
		if (level.clients[i].pers.connected != CON_DISCONNECTED)
		{
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;
		}
	}
}

/*
========================================================================
MAP CHANGING
========================================================================
*/

/**
 * @brief This is also used for spectator spawns
 */
void TVG_FindIntermissionPoint(void)
{
	gentity_t *ent = NULL, *target;
	vec3_t    dir;
	char      cs[MAX_STRING_CHARS];
	char      *buf;
	int       winner;

	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
	buf    = Info_ValueForKey(cs, "w");
	winner = Q_atoi(buf);

	// Change from scripting value for winner (0==AXIS, 1==ALLIES) to spawnflag value
	if (winner == 0)
	{
		winner = TEAM_AXIS;
	}
	else
	{
		winner = TEAM_ALLIES;
	}

	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
	while (ent)
	{
		if (ent->spawnflags & winner)
		{
			break;
		}

		ent = G_Find(ent, FOFS(classname), "info_player_intermission");
	}

	if (!ent) // the map creator forgot to put in an intermission point...
	{
		SelectSpawnPoint(vec3_origin, level.intermission_origin, level.intermission_angle);
	}
	else
	{
		VectorCopy(ent->s.origin, level.intermission_origin);
		VectorCopy(ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if (ent->target)
		{
			target = G_PickTarget(ent->target);
			if (target)
			{
				VectorSubtract(target->s.origin, level.intermission_origin, dir);
				vectoangles(dir, level.intermission_angle);
			}
		}
	}
}

/**
 * @brief Print to the logfile with a time stamp if it is open
 * @param fmt
 */
void QDECL G_LogPrintf(const char *fmt, ...)
{
	va_list argptr;
	char    string[1024];
	int     l;

	Com_sprintf(string, sizeof(string), "%8i ", level.time);

	l = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
	va_end(argptr);

	if (g_dedicated.integer)
	{
		G_Printf("%s", string + l);
	}

	if (!level.logFile)
	{
		return;
	}

	trap_FS_Write(string, strlen(string), level.logFile);
}

void QDECL G_LogPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));

/*
========================================================================
FUNCTIONS CALLED EVERY FRAME
========================================================================
*/

/**
 * @brief Advances the world
 * @param[in] levelTime
 */
void TVG_RunFrame(int levelTime)
{
	char *queueMsg, *s = "";
	int  i, queueSeconds;

	trap_ETTV_GetPlayerstate(-1, &level.ettvMasterPs);

	level.intermission          = level.ettvMasterPs.pm_type == PM_INTERMISSION;
	level.numValidMasterClients = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		level.ettvMasterClients[i].valid = trap_ETTV_GetPlayerstate(i, &level.ettvMasterClients[i].ps);

		if (level.ettvMasterClients[i].valid)
		{
			level.validMasterClients[level.numValidMasterClients++] = i;
		}
	}

	level.validMasterClients[level.numValidMasterClients++] = level.ettvMasterPs.clientNum;

	queueSeconds = tvg_queue_ms.integer / 1000;

	if (queueSeconds != level.queueSeconds)
	{
		level.queueSeconds = queueSeconds;

		if (level.queueSeconds < 1)
		{
			queueMsg = va("cp \"\n\"");
		}
		else
		{
			if (level.queueSeconds != 1)
			{
				s = "s";
			}

			queueMsg = va("cp \"t-%d second%s\n\"", level.queueSeconds, s);
		}

		for (i = 0; i < level.numConnectedClients; i++)
		{
			if (level.clients[level.sortedClients[i]].pers.connected == CON_CONNECTED)
			{
				trap_SendServerCommand(level.sortedClients[i], queueMsg);
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if (level.restarted)
	{
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time         = levelTime;
	level.frameTime    = level.time - level.previousTime;

	// get any cvar changes
	TVG_UpdateCvars();

	if (level.lastCmdsUpdate + 100 <= level.time)
	{
		TVG_SendCommands();
		level.lastCmdsUpdate = level.time;
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		TVG_ClientEndFrame(&level.clients[level.sortedClients[i]]);
	}
}

// FIXME: test if this works, .manifest extension is not supported in ettv
//        so it probably fails to open file for read
void TVG_ParsePlatformManifest(void)
{
	fileHandle_t fileHandle;
	char         *buffer, *token, *parse;
	int          len, i;
	unsigned int ossFlags = 0;

	len = trap_FS_FOpenFile("platforms.manifest", &fileHandle, FS_READ);
	if (len <= 0)
	{
		G_Printf(S_COLOR_RED "[G_OSS] no file found\n");
		trap_FS_FCloseFile(fileHandle);
		return;
	}

	buffer = Com_Allocate(len + 1);
	if (!buffer)
	{
		G_Printf(S_COLOR_RED "[G_OSS] failed to allocate %i bytes\n", len + 1);
		trap_FS_FCloseFile(fileHandle);
		return;
	}

	trap_FS_Read(buffer, len, fileHandle);
	buffer[len] = '\0';
	parse       = buffer;

	trap_FS_FCloseFile(fileHandle);

	COM_BeginParseSession(__FUNCTION__);

	token = COM_Parse(&parse);
	while (token[0])
	{
		for (i = 0; i < OSS_KNOWN_COUNT; i++)
		{
			if (!strcmp(oss_str[i], token))
			{
				G_DPrintf(S_COLOR_CYAN "[G_OSS] enabling support for platform: %s -> %i\n", token, (int)BIT(i));
				ossFlags |= (unsigned int) BIT(i);
			}
		}

		token = COM_Parse(&parse);
	}

	G_DPrintf("[G_OSS] parsing done with flag value: %i\n", ossFlags);

	trap_Cvar_Set("g_oss", va("%i", ossFlags));

	Com_Dealloc(buffer);
}
