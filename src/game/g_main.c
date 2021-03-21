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
 * @file g_main.c
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

#ifdef FEATURE_SERVERMDX
#include "g_mdx.h"
#endif

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

g_campaignInfo_t g_campaigns[MAX_CAMPAIGNS];

mapEntityData_Team_t mapEntityData[2];

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

vmCvar_t g_protect; // similar to sv_protect game cvar
                    // 0 - no protection - default to have ref for localhost clients on listen servers
                    // 1 - disabled auto ref for localhost clients

vmCvar_t g_dropHealth;
vmCvar_t g_dropAmmo;

vmCvar_t g_shove;

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
vmCvar_t g_oss; //   0 - vanilla/unknown/ET:L auto setup
                //   1 - Windows
                //   2 - Linux
                //   4 - Linux 64
                //   8 - Mac OS X
                //  16 - Android

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

vmCvar_t g_dynamiteChaining;

vmCvar_t g_playerHitBoxHeight;

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

	{ &g_oss,                             "g_oss",                             "15",                         CVAR_SERVERINFO | CVAR_LATCH,                    0, qfalse, qfalse },

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
	{ &g_debugBullets,                    "g_debugBullets",                    "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
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
#endif

	{ &g_protect,                         "g_protect",                         "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_dropHealth,                      "g_dropHealth",                      "0",                          0,                                               0, qfalse, qfalse },
	{ &g_dropAmmo,                        "g_dropAmmo",                        "0",                          0,                                               0, qfalse, qfalse },
	{ &g_shove,                           "g_shove",                           "60",                         0,                                               0, qfalse, qfalse },

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
	{ &g_fixedphysics,                    "g_fixedphysics",                    "1",                          CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	{ &g_fixedphysicsfps,                 "g_fixedphysicsfps",                 "125",                        CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	{ &g_pronedelay,                      "g_pronedelay",                      "0",                          CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
	// Debug
	{ &g_debugHitboxes,                   "g_debugHitboxes",                   "0",                          CVAR_CHEAT,                                      0, qfalse, qfalse },
	{ &g_debugPlayerHitboxes,             "g_debugPlayerHitboxes",             "0",                          0,                                               0, qfalse, qfalse },     // no need to make this CVAR_CHEAT

	{ &g_corpses,                         "g_corpses",                         "0",                          CVAR_LATCH | CVAR_ARCHIVE,                       0, qfalse, qfalse },
	{ &g_realHead,                        "g_realHead",                        "1",                          0,                                               0, qfalse, qfalse },
	{ &sv_fps,                            "sv_fps",                            "20",                         CVAR_SYSTEMINFO,                                 0, qfalse, qfalse },
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
	{ &g_dynamiteChaining,                "g_dynamiteChaining",                "0",                          CVAR_ARCHIVE,                                    0, qfalse, qfalse },
	{ &g_playerHitBoxHeight,              "g_playerHitBoxHeight",              "36",                         CVAR_ARCHIVE | CVAR_SERVERINFO,                  0, qfalse, qfalse },
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

void G_InitGame(int levelTime, int randomSeed, int restart, int legacyServer, int serverVersion);
void G_RunFrame(int levelTime);
void G_ShutdownGame(int restart);
void CheckExitRules(void);

/**
 * @brief G_SnapshotCallback
 * @param[in] entityNum
 * @param[in] clientNum
 * @return
 */
qboolean G_SnapshotCallback(int entityNum, int clientNum)
{
	gentity_t *ent = &g_entities[entityNum];

	if (ent->s.eType == ET_MISSILE)
	{
		if (ent->s.weapon == WP_LANDMINE)
		{
			return G_LandmineSnapshotCallback(entityNum, clientNum);
		}
	}

	return qtrue;
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
		float time = trap_Milliseconds();
		Com_Printf(S_COLOR_MDGREY "Initializing %s game " S_COLOR_GREEN ETLEGACY_VERSION "\n", MODNAME);
#ifdef FEATURE_OMNIBOT

		Bot_Interface_InitHandles();
#endif
		G_InitGame(arg0, arg1, arg2, arg3, arg4);
		G_Printf("Game Initialization completed in %.2f seconds\n", ((float)trap_Milliseconds() - time) / 1000.f);
#ifdef FEATURE_OMNIBOT

		time = trap_Milliseconds();

		if (!Bot_Interface_Init())
		{
			G_Printf(S_COLOR_RED "Unable to Initialize Omni-Bot\n");
		}

		if (g_OmniBotEnable.integer >= 1)
		{
			// that's the only way to print the used bot version atm
			trap_SendConsoleCommand(EXEC_APPEND, va("%s", "bot version\n"));

			G_Printf(S_COLOR_GREEN "Omni-Bot Initialization completed in %.2f seconds\n", ((float)trap_Milliseconds() - time) / 1000.f);
		}
#endif
	}
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame(arg0);
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect(arg0, arg1, arg2);
	case GAME_CLIENT_THINK:
		ClientThink(arg0);
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged(arg0);
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect(arg0);
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin(arg0);
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand(arg0);
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame(arg0);
#ifdef FEATURE_OMNIBOT
		Bot_Interface_Update();
#endif
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case GAME_SNAPSHOT_CALLBACK:
		return G_SnapshotCallback(arg0, arg1);
	case GAME_MESSAGERECEIVED:
		return -1;
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
 * @brief Returns whether the ent should be ignored for cursor hint purpose (because the ent may have the designed content type
 * but nevertheless should not display any cursor hint)
 *
 * @param[in] traceEnt
 * @param clientEnt - unused
 * @return
 */
static qboolean G_CursorHintIgnoreEnt(gentity_t *traceEnt, gentity_t *clientEnt)
{
	return (traceEnt->s.eType == ET_OID_TRIGGER || traceEnt->s.eType == ET_TRIGGER_MULTIPLE) ? qtrue : qfalse;
}

/**
 * @brief G_EmplacedGunIsMountable
 * @param[in] ent
 * @param[in] other
 * @return
 */
qboolean G_EmplacedGunIsMountable(gentity_t *ent, gentity_t *other)
{
	if (Q_stricmp(ent->classname, "misc_mg42") && Q_stricmp(ent->classname, "misc_aagun"))
	{
		return qfalse;
	}

	if (!other->client)
	{
		return qfalse;
	}

	if (GetWeaponTableData(other->client->ps.weapon)->type & (WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET))
	{
		return qfalse;
	}

	if (other->client->ps.pm_flags & PMF_DUCKED)
	{
		return qfalse;
	}

	if (other->client->ps.persistant[PERS_HWEAPON_USE])
	{
		return qfalse;
	}

	if (ent->r.currentOrigin[2] - other->r.currentOrigin[2] >= 40)
	{
		return qfalse;
	}

	if (ent->r.currentOrigin[2] - other->r.currentOrigin[2] < 0)
	{
		return qfalse;
	}

	if (ent->s.frame != 0)
	{
		return qfalse;
	}

	if (ent->active)
	{
		return qfalse;
	}

	if (other->client->ps.weaponDelay)
	{
		return qfalse;
	}

	if (other->client->ps.grenadeTimeLeft)
	{
		return qfalse;
	}

	if (infront(ent, other))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief G_EmplacedGunIsRepairable
 * @param[in] ent
 * @param[in] other
 * @return
 */
qboolean G_EmplacedGunIsRepairable(gentity_t *ent, gentity_t *other)
{
	if (Q_stricmp(ent->classname, "misc_mg42") && Q_stricmp(ent->classname, "misc_aagun"))
	{
		return qfalse;
	}

	if (!other->client)
	{
		return qfalse;
	}

	if (GetWeaponTableData(other->client->ps.weapon)->type & (WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET))
	{
		return qfalse;
	}

	if (other->client->ps.persistant[PERS_HWEAPON_USE])
	{
		return qfalse;
	}

	if (ent->s.frame == 0)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief non-AI's check for cursor hint contacts
 *
 * @details server-side because there's info we want to show that the client
 * just doesn't know about.  (health or other info of an explosive,invisible_users,items,etc.)
 *
 * traceEnt is the ent hit by the trace, checkEnt is the ent that is being
 * checked against (in case the traceent was an invisible_user or something)
 *
 * @param[in,out] ent
 */
void G_CheckForCursorHints(gentity_t *ent)
{
	vec3_t        forward, right, up, offset, end;
	trace_t       *tr;
	float         dist;
	gentity_t     *checkEnt, *traceEnt = 0;
	playerState_t *ps;
	static int    hintValMax = 255;     // Breakable damage indicator can wrap when the entity has a lot of health
	int           hintType, hintDist, hintVal;
	qboolean      zooming;
	int           trace_contents;
	int           numOfIgnoredEnts = 0;

	if (!ent->client)
	{
		return;
	}

	ps = &ent->client->ps;

	zooming = (qboolean)(ps->eFlags & EF_ZOOMING);

	AngleVectors(ps->viewangles, forward, right, up);

	VectorCopy(ps->origin, offset);
	offset[2] += ps->viewheight;

	// lean
	if (ps->leanf != 0.f)
	{
		VectorMA(offset, ps->leanf, right, offset);
	}

	if (zooming)
	{
		VectorMA(offset, CH_MAX_DIST_ZOOM, forward, end);
	}
	else
	{
		VectorMA(offset, CH_MAX_DIST, forward, end);
	}

	tr = &ps->serverCursorHintTrace;

	G_TempTraceRealHitBox(ent);
	trace_contents = (CONTENTS_TRIGGER | CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE);
	trap_Trace(tr, offset, NULL, NULL, end, ps->clientNum, trace_contents);
	G_ResetTempTraceRealHitBox();

	// reset all
	hintType = ps->serverCursorHint = HINT_NONE;
	hintVal  = ps->serverCursorHintVal = 0;

	dist = VectorDistanceSquared(offset, tr->endpos);

	if (zooming)
	{
		hintDist = CH_MAX_DIST_ZOOM;
	}
	else
	{
		hintDist = CH_MAX_DIST;
	}

	if (ps->stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
	{
		if (ent->client->landmineSpottedTime && level.time - ent->client->landmineSpottedTime < 500)
		{
			ps->serverCursorHint    = HINT_LANDMINE;
			ps->serverCursorHintVal = ent->client->landmineSpotted ? ent->client->landmineSpotted->count2 : 0;
			return;
		}
	}

	if (tr->fraction == 1.f)
	{
		return;
	}

	traceEnt = &g_entities[tr->entityNum];
	G_TempTraceRealHitBox(ent);
	while (G_CursorHintIgnoreEnt(traceEnt, ent) && numOfIgnoredEnts < 10)
	{
		// we may hit multiple invalid ents at the same point
		// count them to prevent too many loops
		numOfIgnoredEnts++;

		// advance offset (start point) past the entity to ignore
		VectorMA(tr->endpos, 0.1f, forward, offset);

		trap_Trace(tr, offset, NULL, NULL, end, traceEnt->s.number, trace_contents);

		// (hintDist - dist) is the actual distance in the above
		// trap_Trace call. update dist accordingly.
		dist += VectorDistanceSquared(offset, tr->endpos);
		if (tr->fraction == 1.f)
		{
			G_ResetTempTraceRealHitBox();
			return;
		}
		traceEnt = &g_entities[tr->entityNum];
	}
	G_ResetTempTraceRealHitBox();

	if (tr->entityNum == ENTITYNUM_WORLD)
	{
		// NOTE: these hint are managed client side !
		// if ((tr->contents & CONTENTS_WATER))
		// {
		//  hintDist = CH_WATER_DIST;
		//  hintType = HINT_WATER;
		// }
		// else if ((tr->surfaceFlags & SURF_LADDER) && !(ps->pm_flags & PMF_LADDER))           // ladder
		// {
		//  hintDist = CH_LADDER_DIST;
		//  hintType = HINT_LADDER;
		// }

		// building something - add this here because we don't have anything solid to trace to - quite ugly-ish
		if (ent->client->touchingTOI && ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
		{
			gentity_t *constructible;

			if ((constructible = G_IsConstructible(ent->client->sess.sessionTeam, ent->client->touchingTOI)))
			{
				ps->serverCursorHint    = HINT_CONSTRUCTIBLE;
				ps->serverCursorHintVal = (int)constructible->s.angles2[0];
				return;
			}
		}
	}
	else if (tr->entityNum < MAX_CLIENTS)
	{
		// show medics a syringe if they can revive someone
		if (traceEnt->client && traceEnt->client->sess.sessionTeam == ent->client->sess.sessionTeam)
		{
			if (ps->stats[STAT_PLAYER_CLASS] == PC_MEDIC && traceEnt->client->ps.pm_type == PM_DEAD && !(traceEnt->client->ps.pm_flags & PMF_LIMBO))
			{
				hintDist = CH_REVIVE_DIST;        // matches weapon_syringe in g_weapon.c
				hintType = HINT_REVIVE;
			}
		}
	}
	else
	{
		checkEnt = traceEnt;

		// invisible entities don't show hints
		if (traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION)
		{
			return;
		}

		// check invisible_users first since you don't want to draw a hint based
		// on that ent, but rather on what they are targeting.
		// so find the target and set checkEnt to that to show the proper hint.
		if (traceEnt->s.eType == ET_GENERAL)
		{
			// ignore trigger_aidoor.  can't just not trace for triggers, since I need invisible_users...
			// damn, I would like to ignore some of these triggers though.
			if (!Q_stricmp(traceEnt->classname, "trigger_aidoor"))
			{
				return;
			}

			if (!Q_stricmp(traceEnt->classname, "func_invisible_user"))
			{
				// Put this back in only in multiplayer
				if (traceEnt->s.dmgFlags)      // hint icon specified in entity
				{
					hintType = traceEnt->s.dmgFlags;
					hintDist = CH_ACTIVATE_DIST;
					checkEnt = 0;
				}
				else // use target for hint icon
				{
					checkEnt = G_FindByTargetname(NULL, traceEnt->target);
					if (!checkEnt)         // no target found
					{
						hintType = HINT_BAD_USER;
						hintDist = CH_ACTIVATE_DIST;
					}
				}
			}
		}

		if (checkEnt)
		{
			// This entire function could be the poster boy for converting to OO programming!!!
			// I'm making this into a switch in a vain attempt to make this readable so I can find which
			// brackets don't match!!!
			switch (checkEnt->s.eType)
			{
			case ET_CORPSE:
				if (!ent->client->ps.powerups[PW_BLUEFLAG] && !ent->client->ps.powerups[PW_REDFLAG])
				{
					if (BODY_TEAM(traceEnt) < 4 && BODY_TEAM(traceEnt) != ent->client->sess.sessionTeam && traceEnt->nextthink == traceEnt->timestamp + BODY_TIME)
					{
						if (ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
						{
							hintDist = CH_CORPSE_DIST;
							hintType = HINT_UNIFORM;
							hintVal  = BODY_VALUE(traceEnt);
							if (hintVal > 255)
							{
								hintVal = 255;
							}
						}
					}
				}
				break;
			case ET_GENERAL:
			case ET_MG42_BARREL:
			case ET_AAGUN:
				//hintType = HINT_FORCENONE;

				if (G_EmplacedGunIsMountable(traceEnt, ent))
				{
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_MG42;
					hintVal  = 0;
				}
				else
				{
					if (ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER && G_EmplacedGunIsRepairable(traceEnt, ent))
					{
						hintType = HINT_BUILD;
						hintDist = CH_BREAKABLE_DIST;
						hintVal  = traceEnt->health;
						if (hintVal > 255)
						{
							hintVal = 255;
						}
					}
					else
					{
						hintDist = CH_NONE_DIST;
						hintType = ps->serverCursorHint = HINT_FORCENONE;
						hintVal  = ps->serverCursorHintVal = 0;
					}
				}
				break;
			case ET_EXPLOSIVE:
			{
				if (checkEnt->spawnflags & EXPLOSIVE_TANK)
				{
					hintDist = CH_BREAKABLE_DIST * 2;
					hintType = HINT_TANK;
					hintVal  = ps->serverCursorHintVal = 0;         // no health for tank destructibles
				}
				else
				{
					switch (checkEnt->constructibleStats.weaponclass)
					{
					case 0:
						hintDist = CH_BREAKABLE_DIST;
						hintType = HINT_BREAKABLE;
						hintVal  = checkEnt->health;                // also send health to client for visualization

						// Breakable damage indicator can wrap when the entity has a lot of health
						if (hintVal > hintValMax)
						{
							hintValMax = hintVal;
						}
						hintVal = (hintVal * 255) / hintValMax;
						break;
					case 1:
						hintDist = CH_BREAKABLE_DIST * 2;
						hintType = HINT_SATCHELCHARGE;
						hintVal  = ps->serverCursorHintVal = 0;     // no health for satchel charges
						break;
					case 2:
						hintDist = CH_NONE_DIST;
						hintType = ps->serverCursorHint = HINT_FORCENONE;
						hintVal  = ps->serverCursorHintVal = 0;

						if (checkEnt->parent && checkEnt->parent->s.eType == ET_OID_TRIGGER)
						{
							if (((ent->client->sess.sessionTeam == TEAM_AXIS) && (checkEnt->parent->spawnflags & ALLIED_OBJECTIVE)) ||
							    ((ent->client->sess.sessionTeam == TEAM_ALLIES) && (checkEnt->parent->spawnflags & AXIS_OBJECTIVE)))
							{
								hintDist = CH_BREAKABLE_DIST * 2;
								hintType = HINT_BREAKABLE_DYNAMITE;
								hintVal  = ps->serverCursorHintVal = 0;         // no health for dynamite
							}
						}
						break;
					default:
						if (checkEnt->health > 0)
						{
							hintDist = CH_BREAKABLE_DIST;
							hintType = HINT_BREAKABLE;
							hintVal  = checkEnt->health;            // also send health to client for visualization

							// Breakable damage indicator can wrap when the entity has a lot of health
							if (hintVal > hintValMax)
							{
								hintValMax = hintVal;
							}
							hintVal = (hintVal * 255) / hintValMax;
						}
						else
						{
							hintDist = CH_NONE_DIST;
							hintType = ps->serverCursorHint = HINT_FORCENONE;
							hintVal  = ps->serverCursorHintVal = 0;
						}
						break;
					}
				}

				break;
			}
			case ET_CONSTRUCTIBLE:
			case ET_CONSTRUCTIBLE_INDICATOR:
			case ET_CONSTRUCTIBLE_MARKER:
				if (G_ConstructionIsPartlyBuilt(checkEnt) && !(checkEnt->spawnflags & CONSTRUCTIBLE_INVULNERABLE))
				{
					// only show hint for players who can blow it up
					if (checkEnt->s.teamNum != ent->client->sess.sessionTeam)
					{
						switch (checkEnt->constructibleStats.weaponclass)
						{
						case 0:
							hintDist = CH_BREAKABLE_DIST;
							hintType = HINT_BREAKABLE;
							hintVal  = checkEnt->health;            // also send health to client for visualization

							// Breakable damage indicator can wrap when the entity has a lot of health
							if (hintVal > hintValMax)
							{
								hintValMax = hintVal;
							}
							hintVal = (hintVal * 255) / hintValMax;
							break;
						case 1:
							hintDist = CH_BREAKABLE_DIST * 2;
							hintType = HINT_SATCHELCHARGE;
							hintVal  = ps->serverCursorHintVal = 0;         // no health for satchel charges
							break;
						case 2:
							hintDist = CH_BREAKABLE_DIST * 2;
							hintType = HINT_BREAKABLE_DYNAMITE;
							hintVal  = ps->serverCursorHintVal = 0;         // no health for dynamite
							break;
						default:
							hintDist = CH_NONE_DIST;
							hintType = ps->serverCursorHint = HINT_FORCENONE;
							hintVal  = ps->serverCursorHintVal = 0;
							break;
						}
					}
					else
					{
						// hintDist = CH_NONE_DIST;
						// hintType = ps->serverCursorHint = HINT_FORCENONE;
						// hintVal  = ps->serverCursorHintVal = 0;
						return;
					}
				}
				else if (ent->client->touchingTOI && ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
				{
					gentity_t *constructible;

					if ((constructible = G_IsConstructible(ent->client->sess.sessionTeam, ent->client->touchingTOI)))
					{
						hintDist = CH_ACTIVATE_DIST;
						hintType = ps->serverCursorHint = HINT_CONSTRUCTIBLE;
						hintVal  = ps->serverCursorHintVal = (int)constructible->s.angles2[0];
					}
				}
				break;
			case ET_ALARMBOX:
				if (checkEnt->health > 0)
				{
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_ACTIVATE;
					hintVal  = ps->serverCursorHintVal = 0;
				}
				break;
			case ET_ITEM:
			{
				hintDist = CH_ACTIVATE_DIST;

				switch (checkEnt->item->giType)
				{
				case IT_HEALTH:
					hintType = HINT_HEALTH;
					break;
				case IT_WEAPON: {
					qboolean canPickup = COM_BitCheck(ent->client->ps.weapons, checkEnt->item->giWeapon);

					if (!canPickup)
					{
						if (checkEnt->item->giWeapon == WP_AMMO)
						{
							canPickup = qtrue;
						}
					}

					if (!canPickup)
					{
						canPickup = G_CanPickupWeapon(checkEnt->item->giWeapon, ent);
					}

					if (canPickup)
					{
						hintType = HINT_WEAPON;
					}
					else
					{
						int weapon = checkEnt->item->giWeapon;

						// get an equivalent weapon if the client team is different of the weapon team, if not keep the current
						if (ent->client->sess.sessionTeam != GetWeaponTableData(weapon)->team && GetWeaponTableData(weapon)->weapEquiv)
						{
							weapon = GetWeaponTableData(weapon)->weapEquiv;
						}

						if (BG_WeaponIsPrimaryForClassAndTeam(ent->client->sess.playerType, ent->client->sess.sessionTeam, weapon) &&
						    G_IsWeaponDisabled(ent, weapon))
						{
							hintType = HINT_RESTRICTED;
						}
					}
					break;
				}
				case IT_AMMO:
					hintType = HINT_AMMO;
					break;
				case IT_TEAM:
					if (!Q_stricmp(traceEnt->classname, "team_CTF_redflag") && ent->client->sess.sessionTeam == TEAM_ALLIES)
					{
						hintType = HINT_POWERUP;
					}
					else if (!Q_stricmp(traceEnt->classname, "team_CTF_blueflag") && ent->client->sess.sessionTeam == TEAM_AXIS)
					{
						hintType = HINT_POWERUP;
					}
					break;
				case IT_BAD:
				default:
					break;
				}
				break;
			}
			case ET_MOVER:
				if (!Q_stricmp(checkEnt->classname, "script_mover"))
				{
					if (G_TankIsMountable(checkEnt, ent))
					{
						hintDist = CH_ACTIVATE_DIST;
						hintType = HINT_ACTIVATE;
					}
					else if (ent->client->touchingTOI && ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
					{
						gentity_t *constructible;

						if ((constructible = G_IsConstructible(ent->client->sess.sessionTeam, ent->client->touchingTOI)))
						{
							hintDist = CH_ACTIVATE_DIST;
							hintType = ps->serverCursorHint = HINT_CONSTRUCTIBLE;
							hintVal  = ps->serverCursorHintVal = (int)constructible->s.angles2[0];
						}
					}
				}
				else if (!Q_stricmp(checkEnt->classname, "func_door_rotating"))
				{
					if (checkEnt->moverState == MOVER_POS1ROTATE)        // stationary/closed
					{
						hintDist = CH_DOOR_DIST;
						hintType = HINT_DOOR_ROTATING;
						if (checkEnt->key == -1 || !G_AllowTeamsAllowed(checkEnt, ent))          // locked
						{
							hintType = HINT_DOOR_ROTATING_LOCKED;
						}
					}
				}
				else if (!Q_stricmp(checkEnt->classname, "func_door"))
				{
					if (checkEnt->moverState == MOVER_POS1)              // stationary/closed
					{
						hintDist = CH_DOOR_DIST;
						hintType = HINT_DOOR;

						if (checkEnt->key == -1 || !G_AllowTeamsAllowed(checkEnt, ent))          // locked
						{
							hintType = HINT_DOOR_LOCKED;
						}
					}
				}
				else if (!Q_stricmp(checkEnt->classname, "func_button"))
				{
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_BUTTON;
				}
				else if (!Q_stricmp(checkEnt->classname, "props_flamebarrel"))
				{
					hintDist = CH_BREAKABLE_DIST * 2;
					hintType = HINT_BREAKABLE;
				}
				else if (!Q_stricmp(checkEnt->classname, "props_statue"))
				{
					hintDist = CH_BREAKABLE_DIST * 2;
					hintType = HINT_BREAKABLE;
				}
				else if (ent->client->touchingTOI && ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
				{
					gentity_t *constructible;

					if ((constructible = G_IsConstructible(ent->client->sess.sessionTeam, ent->client->touchingTOI)))
					{
						hintDist = CH_ACTIVATE_DIST;
						hintType = ps->serverCursorHint = HINT_CONSTRUCTIBLE;
						hintVal  = ps->serverCursorHintVal = (int)constructible->s.angles2[0];
					}
				}
				break;
			case ET_MISSILE:
				if (ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER)
				{
					hintDist = CH_BREAKABLE_DIST;
					hintType = HINT_DISARM;
					hintVal  = checkEnt->health;            // also send health to client for visualization
					if (hintVal > 255)
					{
						hintVal = 255;
					}
				}

				// hint icon specified in entity (and proper contact was made, so hintType was set)
				// first try the checkent...
				if (checkEnt->s.dmgFlags && hintType)
				{
					hintType = checkEnt->s.dmgFlags;
				}

				// then the traceent
				if (traceEnt->s.dmgFlags && hintType)
				{
					hintType = traceEnt->s.dmgFlags;
				}
				break;
			case ET_CABINET_A:
				hintDist = CH_ACTIVATE_DIST;
				hintType = HINT_HEALTH;
				break;
			case ET_CABINET_H:
				hintDist = CH_ACTIVATE_DIST;
				hintType = HINT_AMMO;
				break;
			default:
				break;
			}
		}
	}

	if (zooming)
	{
		hintDist = CH_MAX_DIST_ZOOM;

		// allow hint ladder while zooming
		if (hintType != HINT_LADDER)
		{
			return;
		}
	}

	// set hint distance
	if (dist <= Square(hintDist))
	{
		ps->serverCursorHint    = hintType;
		ps->serverCursorHintVal = hintVal;
	}
}

/**
 * @brief G_SetTargetName
 * @param[out] ent
 * @param[in] targetname
 */
void G_SetTargetName(gentity_t *ent, char *targetname)
{
	if (targetname && *targetname)
	{
		ent->targetname     = targetname;
		ent->targetnamehash = BG_StringHashValue(targetname);
	}
	else
	{
		ent->targetnamehash = -1;
	}
}

/**
 * @brief G_SetSkillLevels
 * @param[in] skill
 * @param[in] string
 */
void G_SetSkillLevels(int skill, const char *string)
{
	char **temp = (char **) &string;
	char *nextLevel;
	int  levels[NUM_SKILL_LEVELS - 1];
	int  count;

	for (count = 0; count < NUM_SKILL_LEVELS - 1; count++)
	{
		nextLevel = COM_ParseExt(temp, qfalse);
		if (nextLevel[0])
		{
			levels[count] = Q_atoi(nextLevel);
			if (levels[count] < 0)
			{
				levels[count] = -1;
			}
		}
		else
		{
			levels[count] = -1;
		}
	}

	for (count = 1; count < NUM_SKILL_LEVELS; count++)
	{
		GetSkillTableData(skill)->skillLevels[count] = levels[count - 1];
	}
}

/**
 * @brief G_SetSkillLevelsByCvar
 * @param[in] cvar
 */
void G_SetSkillLevelsByCvar(vmCvar_t *cvar)
{
	const char *skillstring;
	int        skill = -1;

	if (cvar == &skill_battlesense)
	{
		skill       = SK_BATTLE_SENSE;
		skillstring = skill_battlesense.string;
	}
	else if (cvar == &skill_engineer)
	{
		skill       = SK_EXPLOSIVES_AND_CONSTRUCTION;
		skillstring = skill_engineer.string;
	}
	else if (cvar == &skill_medic)
	{
		skill       = SK_FIRST_AID;
		skillstring = skill_medic.string;
	}
	else if (cvar == &skill_fieldops)
	{
		skill       = SK_SIGNALS;
		skillstring = skill_fieldops.string;
	}
	else if (cvar == &skill_lightweapons)
	{
		skill       = SK_LIGHT_WEAPONS;
		skillstring = skill_lightweapons.string;
	}
	else if (cvar == &skill_soldier)
	{
		skill       = SK_HEAVY_WEAPONS;
		skillstring = skill_soldier.string;
	}
	else if (cvar == &skill_covertops)
	{
		skill       = SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS;
		skillstring = skill_covertops.string;
	}

	if (skill >= 0)
	{
		G_SetSkillLevels(skill, skillstring);
	}
}

#define SKILLSTRING(skill) va("%i,%i,%i,%i", GetSkillTableData(skill)->skillLevels[1], GetSkillTableData(skill)->skillLevels[2], GetSkillTableData(skill)->skillLevels[3], GetSkillTableData(skill)->skillLevels[4])

/**
 * @brief G_UpdateSkillsToClients
 */
void G_UpdateSkillsToClients()
{
	char cs[MAX_INFO_STRING];

	cs[0] = '\0';

	Info_SetValueForKey(cs, "bs", SKILLSTRING(SK_BATTLE_SENSE));
	Info_SetValueForKey(cs, "en", SKILLSTRING(SK_EXPLOSIVES_AND_CONSTRUCTION));
	Info_SetValueForKey(cs, "md", SKILLSTRING(SK_FIRST_AID));
	Info_SetValueForKey(cs, "fo", SKILLSTRING(SK_SIGNALS));
	Info_SetValueForKey(cs, "lw", SKILLSTRING(SK_LIGHT_WEAPONS));
	Info_SetValueForKey(cs, "sd", SKILLSTRING(SK_HEAVY_WEAPONS));
	Info_SetValueForKey(cs, "cv", SKILLSTRING(SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS));
	trap_SetConfigstring(CS_UPGRADERANGE, cs);
}

/**
 * @brief G_InitSkillLevels
 */
void G_InitSkillLevels()
{
	G_SetSkillLevelsByCvar(&skill_battlesense);
	G_SetSkillLevelsByCvar(&skill_engineer);
	G_SetSkillLevelsByCvar(&skill_medic);
	G_SetSkillLevelsByCvar(&skill_fieldops);
	G_SetSkillLevelsByCvar(&skill_lightweapons);
	G_SetSkillLevelsByCvar(&skill_soldier);
	G_SetSkillLevelsByCvar(&skill_covertops);
}

/**
 * @brief Chain together all entities with a matching team field.
 * Entity teams are used for item groups and multi-entity mover groups.
 *
 * All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
 * All but the last will have the teamchain field set to the next one
 */
void G_FindTeams(void)
{
	gentity_t *e, *e2;
	int       i, j;
	int       c = 0, c2 = 0;

	for (i = MAX_CLIENTS, e = g_entities + i ; i < level.num_entities ; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (!e->team)
		{
			continue;
		}

		if (e->flags & FL_TEAMSLAVE)
		{
			continue;
		}

		c++;
		c2++;
		for (j = i + 1, e2 = e + 1 ; j < level.num_entities ; j++, e2++)
		{
			if (!e2->inuse)
			{
				continue;
			}
			if (!e2->team)
			{
				continue;
			}
			if (e2->flags & FL_TEAMSLAVE)
			{
				continue;
			}
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain  = e->teamchain;
				e->teamchain   = e2;
				e2->teammaster = e;
				//e2->key = e->key;	// I can't set the key here since the master door hasn't finished spawning yet and therefore has a key of -1
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if (e2->targetname)
				{
					G_SetTargetName(e, e2->targetname);

					// note: added this because of problems
					// pertaining to keys and double doors
					if (Q_stricmp(e2->classname, "func_door_rotating"))
					{
						e2->targetname = NULL;
					}
				}
			}
		}
	}

	G_Printf("%i teams with %i entities\n", c, c2);
}

/**
 * @brief G_ServerCheck
 */
void G_ServerCheck(void)
{
	if (!level.etLegacyServer)
	{
		G_Error("Error: %s does not support server version %s\n", MODNAME, FAKE_VERSION);
	}
}

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
					trap_SendServerCommand(-1, va("print \"Server:[lof] %s [lon]changed to[lof] %s\n\"", cv->cvarName, cv->vmCvar->string));
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

#define SNIPSIZE 250

/**
 * @brief Copies max num chars from beginning of dest into src and returns pointer to new src
 * @param[out] dest
 * @param[in] src
 * @param[in] num
 * @return
 */
char *strcut(char *dest, char *src, int num)
{
	int i;

	if (!dest || !src || !num)
	{
		return NULL;
	}

	for (i = 0 ; i < num ; i++)
	{
		if ((char)*src)
		{
			*dest = *src;
			dest++;
			src++;
		}
		else
		{
			break;
		}
	}
	*dest = (char)0;

	return src;
}

/**
 * @brief g_{axies,allies}mapxp overflows and crashes the server
 */
void G_ClearMapXP(void)
{
	trap_SetConfigstring(CS_AXIS_MAPS_XP, "");
	trap_SetConfigstring(CS_ALLIED_MAPS_XP, "");

	trap_Cvar_Set(va("%s_axismapxp0", MODNAME), "");
	trap_Cvar_Set(va("%s_alliedmapxp0", MODNAME), "");
}

/**
 * @brief G_StoreMapXP
 */
void G_StoreMapXP(void)
{
	char cs[MAX_STRING_CHARS];
	char u[MAX_STRING_CHARS];
	char *k;
	int  i, j;

	// axis
	trap_GetConfigstring(CS_AXIS_MAPS_XP, cs, sizeof(cs));
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		Q_strcat(cs, sizeof(cs), va(" %i", (int)level.teamXP[i][0]));
	}
	trap_SetConfigstring(CS_AXIS_MAPS_XP, cs);

	j = 0;
	k = strcut(u, cs, SNIPSIZE);
	while (strlen(u))
	{
		//"to be continued..."
		if (strlen(u) == SNIPSIZE)
		{
			strcat(u, "+");
		}
		trap_Cvar_Set(va("%s_axismapxp%i", MODNAME, j), u);
		j++;
		k = strcut(u, k, SNIPSIZE);
	}

	// allies
	trap_GetConfigstring(CS_ALLIED_MAPS_XP, cs, sizeof(cs));
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		Q_strcat(cs, sizeof(cs), va(" %i", (int)level.teamXP[i][1]));
	}
	trap_SetConfigstring(CS_ALLIED_MAPS_XP, cs);

	j = 0;
	k = strcut(u, cs, SNIPSIZE);
	while (strlen(u))
	{
		// "to be continued..."
		if (strlen(u) == SNIPSIZE)
		{
			strcat(u, "+");
		}
		trap_Cvar_Set(va("%s_alliedmapxp%i", MODNAME, j), u);
		j++;
		k = strcut(u, k, SNIPSIZE);
	}
}

/**
 * @brief G_GetMapXP
 */
void G_GetMapXP(void)
{
	int  j = 0;
	char s[MAX_STRING_CHARS];
	char t[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer(va("%s_axismapxp%i", MODNAME, j), s, sizeof(s));
	// reassemble string...
	while (strrchr(s, '+'))
	{
		j++;
		*strrchr(s, '+') = (char)0;
		trap_Cvar_VariableStringBuffer(va("%s_axismapxp%i", MODNAME, j), t, sizeof(t));
		strcat(s, t);
	}
	trap_SetConfigstring(CS_AXIS_MAPS_XP, s);

	j = 0;
	trap_Cvar_VariableStringBuffer(va("%s_alliedmapxp%i", MODNAME, j), s, sizeof(s));
	// reassemble string...
	while (strrchr(s, '+'))
	{
		j++;
		*strrchr(s, '+') = (char)0;
		trap_Cvar_VariableStringBuffer(va("%s_alliedmapxp%i", MODNAME, j), t, sizeof(t));
		strcat(s, t);
	}
	trap_SetConfigstring(CS_ALLIED_MAPS_XP, s);
}

/**
 * @brief G_InitGame
 * @param[in] levelTime
 * @param[in] randomSeed
 * @param[in] restart
 * @param[in] etLegacyServer
 * @param[in] serverVersion
 */
void G_InitGame(int levelTime, int randomSeed, int restart, int etLegacyServer, int serverVersion)
{
	int    i;
	char   cs[MAX_INFO_STRING];
	time_t aclock;
	char   timeFt[32];

	// mod version check
	MOD_CHECK_ETLEGACY(etLegacyServer, serverVersion, level.etLegacyServer);

	// server version check
	G_ServerCheck();

	G_Printf("------- Game Initialization -------\n");
	G_Printf("gamename: %s\n", MODNAME);
	G_Printf("gamedate: %s\n", __DATE__);

	srand(randomSeed);

	G_RegisterCvars();

	// enforcemaxlives stuff

	// we need to clear the list even if enforce maxlives is not active
	// in case the g_maxlives was changed, and a map_restart happened
	ClearMaxLivesBans();

	// just for verbosity
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if (g_enforcemaxlives.integer &&
		    (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0))
		{
			G_Printf("EnforceMaxLives-Cleared GUID List\n");
		}
	}

	G_ProcessIPBans();

	G_InitMemory();

	G_InitSkillLevels();

	// intialize gamestate
	if (g_gamestate.integer == GS_INITIALIZE)
	{
		trap_Cvar_Set("gamestate", va("%i", GS_WARMUP));
	}

	// set some level globals
	i = level.server_settings;
	{
		qboolean   oldspawning = level.spawning;
		voteInfo_t votedata;

		Com_Memcpy(&votedata, &level.voteInfo, sizeof(voteInfo_t));

		Com_Memset(&level, 0, sizeof(level));

		Com_Memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));

		level.spawning = oldspawning;
	}
	level.time            = levelTime;
	level.startTime       = levelTime;
	level.server_settings = i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		level.clients[level.sortedClients[i]].sess.userSpawnPointValue     = 0;
		level.clients[level.sortedClients[i]].sess.resolvedSpawnPointIndex = 0;
	}

	// init the anim scripting
	level.animScriptData.soundIndex = G_SoundIndex;
	level.animScriptData.playSound  = G_AnimScriptSound;

	level.warmupModificationCount = g_warmup.modificationCount;

	level.soldierChargeTime[0]  = level.soldierChargeTime[1] = g_soldierChargeTime.integer;
	level.medicChargeTime[0]    = level.medicChargeTime[1] = g_medicChargeTime.integer;
	level.engineerChargeTime[0] = level.engineerChargeTime[1] = g_engineerChargeTime.integer;
	level.fieldopsChargeTime[0] = level.fieldopsChargeTime[1] = g_fieldopsChargeTime.integer;

	level.covertopsChargeTime[0] = level.covertopsChargeTime[1] = g_covertopsChargeTime.integer;

	level.soldierChargeTimeModifier[0]   = level.soldierChargeTimeModifier[1] = 1.f;
	level.medicChargeTimeModifier[0]     = level.medicChargeTimeModifier[1] = 1.f;
	level.engineerChargeTimeModifier[0]  = level.engineerChargeTimeModifier[1] = 1.f;
	level.fieldopsChargeTimeModifier[0]  = level.fieldopsChargeTimeModifier[1] = 1.f;
	level.covertopsChargeTimeModifier[0] = level.covertopsChargeTimeModifier[1] = 1.f;

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
	trap_SetConfigstring(CS_FILTERCAMS, va("%i", g_filtercams.integer));

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
	Info_SetValueForKey(cs, "m", team_maxplayers.string);
	trap_SetConfigstring(CS_TEAMRESTRICTIONS, cs);

	G_UpdateSkillsToClients();

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		Info_SetValueForKey(cs, "w", "-1");
		trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);

		level.firstbloodTeam = -1;

		if (g_currentRound.integer == 0)
		{
			trap_Cvar_Set("g_axiswins", "0");
			trap_Cvar_Set("g_alliedwins", "0");

			trap_Cvar_Update(&g_axiswins);
			trap_Cvar_Update(&g_alliedwins);
		}

		trap_SetConfigstring(CS_ROUNDSCORES1, va("%i", g_axiswins.integer));
		trap_SetConfigstring(CS_ROUNDSCORES2, va("%i", g_alliedwins.integer));
	}

	if (g_gametype.integer == GT_WOLF || g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		G_ClearMapXP();
	}

	// time
	time(&aclock);
	strftime(timeFt, sizeof(timeFt), "%a %b %d %X %Y", localtime(&aclock));

	if (g_log.string[0])
	{
		if (g_logSync.integer)
		{
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND_SYNC);
		}
		else
		{
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND);
		}

		if (!level.logFile)
		{
			G_Printf("WARNING: Couldn't open logfile: %s\n", g_log.string);
		}
		else
		{
			G_LogPrintf("------------------------------------------------------------\n");
			G_LogPrintf("InitGame: %s\n", cs);
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

	G_ParseCampaigns();
	if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		if (g_campaigns[level.currentCampaign].current == 0 || level.newCampaign)
		{
			trap_Cvar_Set("g_axiswins", "0");
			trap_Cvar_Set("g_alliedwins", "0");

			G_ClearMapXP();

			trap_Cvar_Update(&g_axiswins);
			trap_Cvar_Update(&g_alliedwins);
		}
		else
		{
			G_GetMapXP();
		}

		//FIXME? - print more info about campaign status? current map/total map num
		//int campaignCount;
		//int currentCampaign;
		//qboolean newCampaign;
	}

	trap_SetConfigstring(CS_SCRIPT_MOVER_NAMES, "");     // clear out

	G_DebugOpenSkillLog();

	G_ResetRemappedShaders();

	if (g_mapConfigs.string[0])
	{
		char mapConfig[MAX_STRING_CHARS];

		Q_strncpyz(mapConfig, "exec ", sizeof(mapConfig));
		Q_strcat(mapConfig, sizeof(mapConfig), g_mapConfigs.string);
		Q_strcat(mapConfig, sizeof(mapConfig), "/default.cfg\n");
		trap_SendConsoleCommand(EXEC_APPEND, mapConfig);

		Q_strncpyz(mapConfig, "exec ", sizeof(mapConfig));
		Q_strcat(mapConfig, sizeof(mapConfig), g_mapConfigs.string);
		Q_strcat(mapConfig, sizeof(mapConfig), "/");
		Q_strcat(mapConfig, sizeof(mapConfig), level.rawmapname);
		Q_strcat(mapConfig, sizeof(mapConfig), ".cfg\n");
		trap_SendConsoleCommand(EXEC_APPEND, mapConfig);
	}

	G_InitWorldSession();

	// MAPVOTE
	if (g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		char mapConfig[MAX_STRING_CHARS];

		//trap_Cvar_Set("C", va("%d,%d",
		//        ((level.mapsSinceLastXPReset >= g_resetXPMapCount.integer) ?
		//               0 : level.mapsSinceLastXPReset)+1,
		//       g_resetXPMapCount.integer));

		if (g_mapConfigs.string[0] && g_resetXPMapCount.integer)
		{
			Q_strncpyz(mapConfig, "exec ", sizeof(mapConfig));
			Q_strcat(mapConfig, sizeof(mapConfig), g_mapConfigs.string);
			i = level.mapsSinceLastXPReset;
			if (i == 0 || i == g_resetXPMapCount.integer)
			{
				i = 2;
			}
			else if (i + 2 <= g_resetXPMapCount.integer)
			{
				i += 2;
			}
			else
			{
				i = 1;
			}
			Q_strcat(mapConfig, sizeof(mapConfig), va("/vote_%d.cfg", i));

			trap_SendConsoleCommand(EXEC_APPEND, mapConfig);
		}

		level.mapVotePlayersCount = CG_ParseMapVotePlayersCountConfig();
	}

	// Clear out spawn target config strings
	trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof(cs));
	Info_SetValueForKey(cs, "s", "0"); // numspawntargets
	trap_SetConfigstring(CS_MULTI_INFO, cs);

	for (i = CS_MULTI_SPAWNTARGETS; i < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS; i++)
	{
		trap_SetConfigstring(i, "");
	}

	G_ResetTeamMapData();

	// initialize all entities for this game
	Com_Memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	Com_Memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;

	// set client fields on player ents
	for (i = 0 ; i < level.maxclients ; i++)
	{
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for (i = 0 ; i < MAX_CLIENTS ; i++)
	{
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entities are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t),
	                    &level.clients[0].ps, sizeof(level.clients[0]));

	// reserve some spots for dead player bodies
	InitBodyQue();

	// load level script
	G_Script_ScriptLoad();

	//Set the game config
	G_configSet(g_customConfig.string);

	numSplinePaths = 0 ;
	numPathCorners = 0;

	// MAPVOTE
	level.mapsSinceLastXPReset = 0;

	// init objective indicator
	level.flagIndicator   = 0;
	level.redFlagCounter  = 0;
	level.blueFlagCounter = 0;

#ifdef FEATURE_DBMS
	// check and initialize db
	if (G_DB_Init() != 0)
	{
#ifdef FEATURE_RATING
		if (g_skillRating.integer)
		{
			G_Printf("^3WARNING: g_skillRating changed to 0\n");
			trap_Cvar_Set("g_skillRating", "0");
		}
#endif

#ifdef FEATURE_PRESTIGE
		if (g_prestige.integer)
		{
			G_Printf("^3WARNING: g_prestige changed to 0\n");
			trap_Cvar_Set("g_prestige", "0");
		}
#endif

		if (g_xpSaver.integer)
		{
			G_Printf("^3WARNING: g_xpSaver changed to 0\n");
			trap_Cvar_Set("g_xpSaver", "0");
		}
	}
#endif

#ifdef FEATURE_RATING
	if (g_skillRating.integer)
	{
		// ensure temporary table is empty
		if (G_SkillRatingPrepareMatchRating())
		{
			trap_Cvar_Set("g_skillRating", "0");
		}

		// get map rating
		if (g_skillRating.integer > 1)
		{
			level.mapProb = G_SkillRatingGetMapRating(level.rawmapname);
		}
	}
#endif

	if (g_xpSaver.integer && g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		if (g_campaigns[level.currentCampaign].current == 0 || level.newCampaign)
		{
			G_XPSaver_Clear();
		}
	}

#ifdef FEATURE_LUA
	G_LuaInit();
#else
	G_Printf("%sNo Lua API available\n", S_COLOR_BLUE);
#endif

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// debris test
	G_LinkDebris();

	// link up damage parents
	G_LinkDamageParents();

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", level.rawmapname));

	// ===================

	if (!level.gameManager)
	{
		G_Printf("^1ERROR No 'script_multiplayer' found in map\n");
	}

	level.tracemapLoaded = BG_LoadTraceMap(level.rawmapname, level.mapcoordsMins, level.mapcoordsMaxs);
	if (!level.tracemapLoaded)
	{
		G_Printf("^1ERROR No tracemap found for map\n");
	}

	// Link all the splines up
	BG_BuildSplinePaths();

	// general initialization
	G_FindTeams();

	G_Printf("-----------------------------------\n");

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	G_RegisterPlayerClasses();

	// Match init work
	G_loadMatchGame();

	GeoIP_open(); // GeoIP open/update

#ifdef FEATURE_LUA
	G_LuaHook_InitGame(levelTime, randomSeed, restart);
#endif

#ifdef FEATURE_MULTIVIEW
	// Reinstate any MV views for clients -- need to do this after all init is complete
	// --- maybe not the best place to do this... seems to be some race conditions on map_restart
	G_spawnPrintf(DP_MVSPAWN, level.time + 2000, NULL);
#endif
}

/**
 * @brief G_ShutdownGame
 * @param[in] restart
 */
void G_ShutdownGame(int restart)
{
	time_t aclock;
	char   timeFt[32];

#ifdef FEATURE_DBMS
	if (level.database.initialized)
	{
		// deinitialize db at the last moment to ensure bots/players connecting
		// after intermission but before next map is loaded still have db access
		G_DB_DeInit();
	}
#endif

#ifdef FEATURE_LUA
	G_LuaHook_ShutdownGame(restart);
	G_LuaShutdown();
#endif
	// gametype latching
	if (((g_gametype.integer == GT_WOLF || g_gametype.integer == GT_WOLF_CAMPAIGN  || g_gametype.integer == GT_WOLF_MAPVOTE) && (g_entities[ENTITYNUM_WORLD].r.worldflags & NO_GT_WOLF)) ||
	    (g_gametype.integer == GT_WOLF_STOPWATCH && (g_entities[ENTITYNUM_WORLD].r.worldflags & NO_STOPWATCH)) ||
	    (g_gametype.integer == GT_WOLF_LMS && (g_entities[ENTITYNUM_WORLD].r.worldflags & NO_LMS)))
	{
		if (!(g_entities[ENTITYNUM_WORLD].r.worldflags & NO_GT_WOLF))
		{
			trap_Cvar_Set("g_gametype", va("%i", GT_WOLF));
		}
		else
		{
			trap_Cvar_Set("g_gametype", va("%i", GT_WOLF_LMS));
		}

		trap_Cvar_Update(&g_gametype);
	}

	G_Printf("==== ShutdownGame (%i - %s) ====\n", restart, level.rawmapname);

	// time
	time(&aclock);
	strftime(timeFt, sizeof(timeFt), "%a %b %d %X %Y", localtime(&aclock));
	G_Printf("gametime: %s\n", timeFt);

#ifdef FEATURE_OMNIBOT
	if (!Bot_Interface_Shutdown())
	{
		G_Printf(S_COLOR_RED "Error shutting down Omni-Bot\n");
	}
	else
	{
		if (g_OmniBotEnable.integer)
		{
			G_Printf("^2ShutdownOmniBot\n");
		}
	}
#endif

	G_DebugCloseSkillLog();

	if (level.logFile)
	{
		G_LogPrintf("ShutdownGame:\n");
		G_LogPrintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}

	GeoIP_close();

#ifdef FEATURE_SERVERMDX
	// zinx - realistic hitboxes
	mdx_cleanup();
#endif

	// write all the client session data so we can get it back
	G_WriteSessionData(restart);
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
 * @brief SortRanks
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL SortRanks(const void *a, const void *b)
{
	gclient_t *ca = &level.clients[*(const int *)a];
	gclient_t *cb = &level.clients[*(const int *)b];

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING)
	{
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING)
	{
		return -1;
	}

	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		if (ca->sess.spectatorTime < cb->sess.spectatorTime)
		{
			return -1;
		}
		if (ca->sess.spectatorTime > cb->sess.spectatorTime)
		{
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return -1;
	}

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		// then sort by score
		if (ca->ps.persistant[PERS_SCORE] > cb->ps.persistant[PERS_SCORE])
		{
			return -1;
		}
		if (ca->ps.persistant[PERS_SCORE] < cb->ps.persistant[PERS_SCORE])
		{
			return 1;
		}
	}
	else
	{
		int i, totalXP[2];

		for (totalXP[0] = totalXP[1] = 0, i = 0; i < SK_NUM_SKILLS; i++)
		{
			totalXP[0] += ca->sess.skillpoints[i];
			totalXP[1] += cb->sess.skillpoints[i];
		}

		if (!((g_gametype.integer == GT_WOLF_CAMPAIGN && g_xpSaver.integer) ||
		      (g_gametype.integer == GT_WOLF_CAMPAIGN && (g_campaigns[level.currentCampaign].current != 0 && !level.newCampaign)) ||
		      (g_gametype.integer == GT_WOLF_LMS && g_currentRound.integer != 0)))
		{
			// current map XPs only
			totalXP[0] -= ca->sess.startxptotal;
			totalXP[1] -= cb->sess.startxptotal;
		}

		// then sort by xp
		if (totalXP[0] > totalXP[1])
		{
			return -1;
		}
		if (totalXP[0] < totalXP[1])
		{
			return 1;
		}
	}
	return 0;
}

/**
 * @brief etpro_PlayerInfo
 */
void etpro_PlayerInfo(void)
{
	//128 bits
	char      playerinfo[MAX_CLIENTS + 1];
	gentity_t *e = &g_entities[0];
	team_t    playerteam;
	int       i;
	int       lastclient = -1;

	Com_Memset(playerinfo, 0, sizeof(playerinfo));

	for (i = 0; i < MAX_CLIENTS; i++, e++)
	{
		if (e->client == NULL || e->client->pers.connected == CON_DISCONNECTED)
		{
			playerinfo[i] = '-';
			continue;
		}

		// keep track of highest connected/connecting client
		lastclient = i;

		if (e->inuse == qfalse)
		{
			playerteam = 0;
		}
		else
		{
			playerteam = e->client->sess.sessionTeam;
		}
		playerinfo[i] = (char)'0' + playerteam;
	}
	// terminate the string, if we have any non-0 clients
	if (lastclient != -1)
	{
		playerinfo[lastclient + 1] = (char)0;
	}
	else
	{
		playerinfo[0] = (char)0;
	}

	trap_Cvar_Set("P", playerinfo);
}

/**
 * @brief Recalculates the score ranks of all players.
 * This will be called on every client connect, begin, disconnect, death,
 * and team change.
 */
void CalculateRanks(void)
{
	int       i;
	char      teaminfo[TEAM_NUM_TEAMS][256];
	gclient_t *cl;

	level.numConnectedClients       = 0;
	level.numNonSpectatorClients    = 0;
	level.numPlayingClients         = 0;
	level.voteInfo.numVotingClients = 0; // don't count bots

	level.numFinalDead[0] = 0;
	level.numFinalDead[1] = 0;

	level.voteInfo.numVotingTeamClients[0] = 0;
	level.voteInfo.numVotingTeamClients[1] = 0;

	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		if (i < 2)
		{
			level.numTeamClients[i] = 0;
		}
		teaminfo[i][0] = 0;
	}

	for (i = 0 ; i < level.maxclients ; i++)
	{
		if (level.clients[i].pers.connected != CON_DISCONNECTED)
		{
			int team = level.clients[i].sess.sessionTeam;

			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if (team != TEAM_SPECTATOR)
			{
				level.numNonSpectatorClients++;

				Q_strcat(teaminfo[team], sizeof(teaminfo[team]) - 1, va("%d ", level.numConnectedClients));

				// decide if this should be auto-followed
				if (level.clients[i].pers.connected == CON_CONNECTED)
				{
					int teamIndex = level.clients[i].sess.sessionTeam == TEAM_AXIS ? 0 : 1;

					level.numPlayingClients++;
					if (!(g_entities[i].r.svFlags & SVF_BOT))
					{
						level.voteInfo.numVotingClients++;
					}

					if (level.clients[i].sess.sessionTeam == TEAM_AXIS ||
					    level.clients[i].sess.sessionTeam == TEAM_ALLIES)
					{
						if (g_gametype.integer == GT_WOLF_LMS)
						{
							if (g_entities[i].health <= 0 || (level.clients[i].ps.pm_flags & PMF_LIMBO))
							{
								level.numFinalDead[teamIndex]++;
							}
						}
						else
						{
							if (level.clients[i].ps.persistant[PERS_RESPAWNS_LEFT] == 0 && g_entities[i].health <= 0)
							{
								level.numFinalDead[teamIndex]++;
							}
						}

						level.numTeamClients[teamIndex]++;
						if (!(g_entities[i].r.svFlags & SVF_BOT))
						{
							level.voteInfo.numVotingTeamClients[teamIndex]++;
						}
					}
				}
			}
		}
	}

	for (i = 0; i < TEAM_NUM_TEAMS; i++)
	{
		if (0 == teaminfo[i][0])
		{
			Q_strncpyz(teaminfo[i], "(None)", sizeof(teaminfo[i]));
		}
	}

	qsort(level.sortedClients, level.numConnectedClients,
	      sizeof(level.sortedClients[0]), SortRanks);

	// set the rank value for all clients that are connected and not spectators
	// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];
		if (level.teamScores[TEAM_AXIS] == level.teamScores[TEAM_ALLIES])
		{
			cl->ps.persistant[PERS_RANK] = 2;
		}
		else if (level.teamScores[TEAM_AXIS] > level.teamScores[TEAM_ALLIES])
		{
			cl->ps.persistant[PERS_RANK] = 0;
		}
		else
		{
			cl->ps.persistant[PERS_RANK] = 1;
		}
	}

	trap_SetConfigstring(CS_FIRSTBLOOD, va("%i", level.firstbloodTeam));
	trap_SetConfigstring(CS_ROUNDSCORES1, va("%i", g_axiswins.integer));
	trap_SetConfigstring(CS_ROUNDSCORES2, va("%i", g_alliedwins.integer));

	etpro_PlayerInfo();

	// if we are at the intermission, send the new info to everyone
	if (g_gamestate.integer == GS_INTERMISSION)
	{
		SendScoreboardMessageToAllClients();
	}
	else
	{
		// see if it is time to end the level
		CheckExitRules();
	}
}

/*
========================================================================
MAP CHANGING
========================================================================
*/

/**
 * @brief Do this at BeginIntermission time and whenever ranks are recalculated
 * due to enters/exits/forced team changes
 */
void SendScoreboardMessageToAllClients(void)
{
	int i;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		if (level.clients[level.sortedClients[i]].pers.connected == CON_CONNECTED)
		{
			level.clients[level.sortedClients[i]].wantsscore = qtrue;
//			G_SendScore(g_entities + level.sortedClients[i]);
		}
	}
}

/**
 * @brief When the intermission starts, this will be called for all players.
 * If a new client connects, this will be called after the spawn function.
 * @param[in,out] ent Client
 * @param[in] hasVoted keep tracking clients vote if they change team during intermission
 */
void MoveClientToIntermission(gentity_t *ent, qboolean hasVoted)
{
	// take out of follow mode if needed
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
	{
		StopFollowing(ent);
	}

	// move to the spot
	VectorCopy(level.intermission_origin, ent->s.origin);
	VectorCopy(level.intermission_origin, ent->client->ps.origin);
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	// Com_Memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	// Player view is distorted in intermission if you have ridden a vehicle,
	// mounted a tank
	if (ent->tankLink)
	{
		G_LeaveTank(ent, qfalse);
	}

	// initialize vars
	if (!hasVoted && g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		ent->client->sess.mapVotedFor[0] = -1;
		ent->client->sess.mapVotedFor[1] = -1;
		ent->client->sess.mapVotedFor[2] = -1;
	}

	ent->client->ps.eFlags |= hasVoted ? EF_VOTED : 0;
	ent->s.eFlags           = 0;
	ent->s.eType            = ET_GENERAL;
	ent->s.modelindex       = 0;
	ent->s.loopSound        = 0;
	ent->s.event            = 0;
	ent->s.events[0]        = ent->s.events[1] = ent->s.events[2] = ent->s.events[3] = 0;
	ent->r.contents         = 0;
}

/**
 * @brief This is also used for spectator spawns
 */
void FindIntermissionPoint(void)
{
	gentity_t *ent = NULL, *target;
	vec3_t    dir;
	char      cs[MAX_STRING_CHARS];
	char      *buf;
	int       winner;

	// if the match hasn't ended yet, and we're just a spectator
	if (!level.intermissiontime)
	{
		// try to find the intermission spawnpoint with no team flags set
		ent = G_Find(NULL, FOFS(classname), "info_player_intermission");

		for ( ; ent; ent = G_Find(ent, FOFS(classname), "info_player_intermission"))
		{
			if (!ent->spawnflags)
			{
				break;
			}
		}
	}

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

	if (!ent)
	{
		ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
		while (ent)
		{
			if (ent->spawnflags & winner)
			{
				break;
			}

			ent = G_Find(ent, FOFS(classname), "info_player_intermission");
		}
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

// MAPVOTE

/**
 * @brief G_SortMapsByzOrder
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL G_SortMapsByzOrder(const void *a, const void *b)
{
	int z1 = *(const int *)a;
	int z2 = *(const int *)b;

	if (z1 == -1 && z2 == -1)
	{
		return 0;
	}
	else if (z1 == -1)
	{
		return 1;
	}
	else if (z2 == -1)
	{
		return -1;
	}

	// no_randomize ??.. it doesn't sort the list, so NO_SORTING would be a better name for this..
	// !!!!! what about maps that have been excluded, or have been played too many times..
	// !!!!! This list always needs to be sorted, to get those maps last in the list.
	//if ( g_mapVoteFlags.integer & MAPVOTE_NO_RANDOMIZE ) {
	//  return 0;
	//}

	if (level.mapvoteinfo[z1].zOrder > level.mapvoteinfo[z2].zOrder)
	{
		return -1;
	}
	else if (level.mapvoteinfo[z2].zOrder > level.mapvoteinfo[z1].zOrder)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * @brief BeginIntermission
 */
void BeginIntermission(void)
{
	int       i;
	gentity_t *client;
	int       itime;

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		return;     // already active
	}

	// MAPVOTE: initialize and populate the map struct for map voting
	if (g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		char bspNames[8192];    // 32 x 128 chars = 4096 minimum. Up to 64 (full-sized) mapnames fit in 8192 bytes..
		int  len, maxMaps;
		char *bspptr;
		char *bspptrTmp;
		char *bspptrEnd;
		char str[128] = "\0";

		level.mapVoteNumMaps = trap_FS_GetFileList("maps", ".bsp", bspNames, sizeof(bspNames));
		bspptr               = bspNames;

		// A real shuffle ...
		// This way not always the same maps will be on top of the list.
		// Servers with more than 32 maps will then be able to vote on more maps (64 is absolute max.).
		// Note: They will never see more than 32 maps in the list (to vote for).
		{
			char     shuffledNames[64][128];
			qboolean shuffled[64] = { 0 };
			int      j            = 0;

			// shuffle the complete list:
			// Backup the list, erase the list, shuffle names, fill the list again..
			maxMaps   = (level.mapVoteNumMaps < 64) ? level.mapVoteNumMaps : 64;
			bspptrTmp = bspptr;
			for (i = 0; i < maxMaps; ++i)
			{
				// check for excluded maps..
				Q_strncpyz(str, bspptrTmp, strlen(bspptrTmp) + 1);
				Q_strncpyz(str, Q_StrReplace(str, ".bsp", ""), sizeof(str));
				if (strstr(g_excludedMaps.string, va(":%s:", str)))
				{
					bspptrTmp += strlen(bspptrTmp) + 1;
					continue;
				}

				// check maps depending of players count
				if (level.mapVotePlayersCount)
				{
					int k;
					int isValid = qtrue;

					for (k = 0; mapVotePlayersCount[k].map[0]; k++)
					{
						if (!Q_stricmp(mapVotePlayersCount[k].map, str))
						{
							if ((mapVotePlayersCount[k].min >= 0 && mapVotePlayersCount[k].min > level.numConnectedClients) ||
							    (mapVotePlayersCount[k].max >= 0 && mapVotePlayersCount[k].max < level.numConnectedClients))
							{
								isValid = qfalse;
							}
							break;
						}
					}

					if (!isValid)
					{
						bspptrTmp += strlen(bspptrTmp) + 1;
						continue;
					}
				}

				Q_strncpyz(shuffledNames[j], str, sizeof(shuffledNames[j]));
				shuffled[j] = qfalse;
				++j;
				bspptrTmp += strlen(bspptrTmp) + 1;
			}
			maxMaps              = j;
			level.mapVoteNumMaps = maxMaps;

			// rebuild the bspNames[] array..
			// I know, perhaps this is a silly way of shuffling the list,
			// but at least the rest of code stays the same.
			Com_Memset(bspNames, 0, sizeof(bspNames));
			bspptrTmp = bspptr;
			for (i = 0; i < maxMaps; ++i)
			{
				int r = rand() % maxMaps;

				while (shuffled[r])
				{
					--r;
					if (r < 0)
					{
						r = maxMaps - 1;
					}
				}
				shuffled[r] = qtrue;
				Q_strncpyz(bspptrTmp, shuffledNames[r], strlen(shuffledNames[r]) + 1);
				bspptrTmp += strlen(shuffledNames[r]) + 1;
			}
		}

		if (level.mapVoteNumMaps > MAX_VOTE_MAPS)
		{
			level.mapVoteNumMaps = MAX_VOTE_MAPS;
		}

		// for checking buffer overflow access ...
		bspptrEnd = bspptr + sizeof(bspNames);
		bspptrTmp = bspptr;

		for (i = 0; i < level.mapVoteNumMaps; i++, bspptr += len + 1)
		{
			len = strlen(bspptr);

			// for checking buffer overflow access..
			// Function trap_FS_GetFileList() can return a greater number of BSP-files found on the server
			// than will fit in the bspNames[] array.
			// So never read beyond its boundries, even if the function reported there are more files..
			bspptrTmp += len + 1;
			if (bspptrTmp >= bspptrEnd)
			{
				level.mapVoteNumMaps = i;
				break;
			}

			// replace via some temporary string
			Q_strncpyz(str, bspptr, strlen(bspptr) + 1);
			Q_strncpyz(str, Q_StrReplace(str, ".bsp", ""), sizeof(str));
			Q_strncpyz(level.mapvoteinfo[i].bspName, str, sizeof(level.mapvoteinfo[0].bspName));

			level.mapvoteinfo[i].zOrder     = rand();
			level.mapvoteinfo[i].lastPlayed = -1;
			level.sortedMaps[i]             = i;
		}
		for (i = level.mapVoteNumMaps; i < MAX_VOTE_MAPS; i++)
		{
			level.sortedMaps[i] = -1;
		}

		G_MapVoteInfoRead();

		len = level.mapVoteNumMaps;
		// zero out maps not available for voting this round
		for (i = 0; i < len; i++)
		{
			if (!Q_stricmp(level.mapvoteinfo[i].bspName, level.rawmapname))
			{
				level.mapvoteinfo[i].lastPlayed = 0;
				level.mapvoteinfo[i].timesPlayed++;
			}
			// played too recently?
			if (level.mapvoteinfo[i].lastPlayed != -1 && level.mapvoteinfo[i].lastPlayed <= g_minMapAge.integer)
			{
				level.sortedMaps[i]         = -1;
				level.mapvoteinfo[i].zOrder = 0;
				level.mapVoteNumMaps--;
				level.mapvoteinfo[i].lastPlayed++;
			}
		}

		qsort(level.sortedMaps, len, sizeof(level.sortedMaps[0]), G_SortMapsByzOrder);

		maxMaps = g_maxMapsVotedFor.integer;
		if (maxMaps > level.mapVoteNumMaps)
		{
			maxMaps = level.mapVoteNumMaps;
		}

		for (i = 0; i < maxMaps; ++i)
		{
			level.mapvoteinfo[level.sortedMaps[i]].voteEligible++;
		}
	}

	level.intermissiontime = level.time;

	// fix for tricking the client into drawing the correct countdown timer.
	itime = level.intermissiontime;
	if (g_intermissionTime.integer > 0)
	{
		itime -= (60000 - (g_intermissionTime.integer * 1000));
	}

	trap_SetConfigstring(CS_INTERMISSION_START_TIME, va("%i", itime));
	trap_Cvar_Set("gamestate", va("%i", GS_INTERMISSION));
	trap_Cvar_Update(&g_gamestate);

	FindIntermissionPoint();

	// move all clients to the intermission point
	for (i = 0 ; i < level.maxclients ; i++)
	{
		client = g_entities + i;
		if (!client->inuse)
		{
			continue;
		}
		MoveClientToIntermission(client, qfalse);
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

	fActions = 0; // reset actions
}

/**
 * @brief When the intermission has been exited, the server is either killed
 * or moved to a new level based on the "nextmap" cvar
 */
void ExitLevel(void)
{
	int i;

	switch (g_gametype.integer)
	{
	case GT_WOLF_CAMPAIGN:
	{
		g_campaignInfo_t *campaign = &g_campaigns[level.currentCampaign];

		if (campaign->current + 1 < campaign->mapCount)
		{
			trap_Cvar_Set("g_currentCampaignMap", va("%i", campaign->current + 1));

			trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", campaign->mapnames[campaign->current + 1]));
		}
		else
		{
			char s[MAX_STRING_CHARS];
			trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));

			if (*s)
			{
				trap_SendConsoleCommand(EXEC_APPEND, "vstr nextcampaign\n");
			}
			else
			{
				// restart the campaign
				trap_Cvar_Set("g_currentCampaignMap", "0");

				trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", campaign->mapnames[0]));
			}

			// FIXME: do we want to do something here?
			//trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
		}
		break;
	}
	case GT_WOLF_LMS:
		if (level.lmsDoNextMap)
		{
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
		}
		else
		{
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
		}
		break;

	// MAPVOTE
	case GT_WOLF_MAPVOTE:
	{
		int nextMap = 0, highMapVote = 0, curMapVotes = 0, maxMaps, highMapAge = 0, curMapAge = 0;

		if (g_resetXPMapCount.integer)
		{
			level.mapsSinceLastXPReset++;
		}
		maxMaps = g_maxMapsVotedFor.integer;
		if (maxMaps > level.mapVoteNumMaps)
		{
			maxMaps = level.mapVoteNumMaps;
		}
		for (i = 0; i < maxMaps; i++)
		{
			if (level.mapvoteinfo[level.sortedMaps[i]].lastPlayed != -1)
			{
				level.mapvoteinfo[level.sortedMaps[i]].lastPlayed++;
			}
			curMapVotes = level.mapvoteinfo[level.sortedMaps[i]].numVotes;
			curMapAge   = level.mapvoteinfo[level.sortedMaps[i]].lastPlayed;
			if (curMapAge == -1)
			{
				curMapAge = 9999;   // -1 means never, so set suitably high
			}

			if (curMapVotes > highMapVote ||
			    (curMapVotes == highMapVote && curMapVotes > 0 &&
			     ((!(g_mapVoteFlags.integer & MAPVOTE_TIE_LEASTPLAYED) && curMapAge < highMapAge) ||
			      ((g_mapVoteFlags.integer & MAPVOTE_TIE_LEASTPLAYED) && curMapAge > highMapAge))))
			{
				nextMap     = level.sortedMaps[i];
				highMapVote = curMapVotes;
				highMapAge  = curMapAge;
			}
		}
		if (highMapVote > 0 && level.mapvoteinfo[nextMap].bspName[0])
		{
			trap_SendConsoleCommand(EXEC_APPEND, va("map %s;set nextmap %s\n", level.mapvoteinfo[nextMap].bspName, g_nextmap.string));
		}
		else
		{
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
		}
		break;
	}
	default:
		trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
		break;
	}

	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_AXIS]   = 0;
	level.teamScores[TEAM_ALLIES] = 0;
	if (g_gametype.integer != GT_WOLF_CAMPAIGN)
	{
		gclient_t *cl;

		for (i = 0 ; i < g_maxclients.integer ; i++)
		{
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED)
			{
				continue;
			}
			cl->ps.persistant[PERS_SCORE] = 0;
		}
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData(qfalse);

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i = 0 ; i < g_maxclients.integer ; i++)
	{

		if (level.clients[i].pers.connected == CON_CONNECTED)
		{
			level.clients[i].pers.connected = CON_CONNECTING;
			trap_UnlinkEntity(&g_entities[i]);
		}
	}

	// MAPVOTE
	if (g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		G_MapVoteInfoWrite();
	}

	G_LogPrintf("ExitLevel: executed\n");
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

/**
 * @brief Append information about this game to the log file
 * @param[in] string
 */
void G_LogExit(const char *string)
{
	int       i;
	gclient_t *cl;
	char      cs[MAX_STRING_CHARS];

	// do not allow G_LogExit to be called in non-playing gamestate
	if (g_gamestate.integer != GS_PLAYING)
	{
		return;
	}

	// ensure exit is not triggered twice due to faulty map scripts
	if (level.intermissionQueued)
	{
		G_LogPrintf("Exit: %s (already triggered)\n", string);
		return;
	}

	G_LogPrintf("Exit: %s\n", string);

#ifdef FEATURE_RATING
	// record match ratings
	if (g_skillRating.integer && g_gametype.integer != GT_WOLF_STOPWATCH && g_gametype.integer != GT_WOLF_LMS)
	{
		for (i = 0; i < level.numConnectedClients; i++)
		{
			gentity_t *ent = &g_entities[level.sortedClients[i]];

			if (!ent->inuse)
			{
				continue;
			}

			// record match rating before intermission
			G_SkillRatingSetClientRating(ent->client);
		}
	}
#endif

#ifdef FEATURE_PRESTIGE
	// record prestige
	if (g_prestige.integer && g_gametype.integer != GT_WOLF_CAMPAIGN && g_gametype.integer != GT_WOLF_STOPWATCH && g_gametype.integer != GT_WOLF_LMS)
	{
		for (i = 0; i < level.numConnectedClients; i++)
		{
			gentity_t *ent = &g_entities[level.sortedClients[i]];

			if (!ent->inuse)
			{
				continue;
			}

			// record prestige before intermission
			G_SetClientPrestige(ent->client, qtrue);
		}
	}
#endif
	if (g_xpSaver.integer && g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		for (i = 0; i < level.numConnectedClients; i++)
		{
			gentity_t *ent = &g_entities[level.sortedClients[i]];

			if (!ent->inuse)
			{
				continue;
			}

			// record xp before intermission
			G_XPSaver_Store(ent->client);
		}
	}

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION, "1");

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];

		G_MakeUnready(&g_entities[level.sortedClients[i]]);

		if (cl->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		if (cl->pers.connected == CON_CONNECTING)
		{
			continue;
		}

		// Make sure all the stats are recalculated and accurate
		G_CalcRank(cl);

		G_LogPrintf("score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], (cl->ps.ping < 999 ? cl->ps.ping : 999), level.sortedClients[i], cl->pers.netname);
	}

	G_LogPrintf("axis:%i  allies:%i\n", level.teamScores[TEAM_AXIS], level.teamScores[TEAM_ALLIES]);

	// Send gameCompleteStatus message to master servers
	trap_SendConsoleCommand(EXEC_APPEND, "gameCompleteStatus\n");

#ifdef FEATURE_RATING
	// calculate skill ratings once intermission is queued
	if (g_skillRating.integer)
	{
		G_CalculateSkillRatings();
	}
#endif

	if (g_gametype.integer == GT_WOLF_STOPWATCH)
	{
		int winner, defender;

		trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof(cs));
		defender = Q_atoi(Info_ValueForKey(cs, "d")); // defender

		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		winner = Q_atoi(Info_ValueForKey(cs, "w"));

		if (!g_currentRound.integer)
		{
			if (winner == defender)
			{
				// if the defenders won, use default timelimit
				trap_Cvar_Set("g_nextTimeLimit", va("%f", g_timelimit.value));
			}
			else
			{
				// use remaining time as next timer
				trap_Cvar_Set("g_nextTimeLimit", va("%f", (level.timeCurrent - level.startTime) / 60000.f));
			}
		}
		else
		{
			// reset timer
			trap_Cvar_Set("g_nextTimeLimit", "0");
		}

		trap_Cvar_Set("g_currentRound", va("%i", !g_currentRound.integer));

		G_StoreMapXP();
	}
	else if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		int winner;

		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		winner = Q_atoi(Info_ValueForKey(cs, "w"));

		if (winner == 0)
		{
			g_axiswins.integer |= (1 << g_campaigns[level.currentCampaign].current);
			trap_Cvar_Set("g_axiswins", va("%i", g_axiswins.integer));
			trap_Cvar_Update(&g_axiswins);
		}
		else if (winner == 1)
		{
			g_alliedwins.integer |= (1 << g_campaigns[level.currentCampaign].current);
			trap_Cvar_Set("g_alliedwins", va("%i", g_alliedwins.integer));
			trap_Cvar_Update(&g_alliedwins);
		}

		trap_SetConfigstring(CS_ROUNDSCORES1, va("%i", g_axiswins.integer));
		trap_SetConfigstring(CS_ROUNDSCORES2, va("%i", g_alliedwins.integer));

		G_StoreMapXP();
	}
	else if (g_gametype.integer == GT_WOLF_LMS)
	{
		int winner;
		int roundLimit       = g_lms_roundlimit.integer < 3 ? 3 : g_lms_roundlimit.integer;
		int numWinningRounds = (roundLimit / 2) + 1;

		roundLimit -= 1;    // -1 as it starts at 0

		trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
		winner = Q_atoi(Info_ValueForKey(cs, "w"));

		if (winner == -1)
		{
			// who drew first blood?
			if (level.firstbloodTeam == TEAM_AXIS)
			{
				winner = 0;
			}
			else
			{
				winner = 1;
			}
		}

		if (winner == 0)
		{
			trap_Cvar_Set("g_axiswins", va("%i", g_axiswins.integer + 1));
			trap_Cvar_Update(&g_axiswins);
		}
		else
		{
			trap_Cvar_Set("g_alliedwins", va("%i", g_alliedwins.integer + 1));
			trap_Cvar_Update(&g_alliedwins);
		}

		if (g_currentRound.integer >= roundLimit || g_axiswins.integer == numWinningRounds || g_alliedwins.integer == numWinningRounds)
		{
			trap_Cvar_Set("g_currentRound", "0");
			if (g_lms_currentMatch.integer + 1 >= g_lms_matchlimit.integer)
			{
				trap_Cvar_Set("g_lms_currentMatch", "0");
				level.lmsDoNextMap = qtrue;
			}
			else
			{
				trap_Cvar_Set("g_lms_currentMatch", va("%i", g_lms_currentMatch.integer + 1));
				level.lmsDoNextMap = qfalse;
			}
		}
		else
		{
			trap_Cvar_Set("g_currentRound", va("%i", g_currentRound.integer + 1));
			trap_Cvar_Update(&g_currentRound);
		}
	}
	else if (g_gametype.integer == GT_WOLF || g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		G_StoreMapXP();
	}

	// award medals
	if (g_gametype.integer == GT_WOLF || g_gametype.integer == GT_WOLF_CAMPAIGN || g_gametype.integer == GT_WOLF_MAPVOTE)
	{
		int highestskillpoints, highestskillpointsclient, j, teamNum;
		int highestskillpointsincrease;

		for (teamNum = TEAM_AXIS; teamNum <= TEAM_ALLIES; teamNum++)
		{
			for (i = 0; i < SK_NUM_SKILLS; i++)
			{
				highestskillpoints         = 0;
				highestskillpointsincrease = 0;
				highestskillpointsclient   = -1;
				for (j = 0; j < level.numConnectedClients; j++)
				{
					cl = &level.clients[level.sortedClients[j]];

					if (cl->sess.sessionTeam != teamNum)
					{
						continue;
					}

					// Make sure the player got some skills
					if (cl->sess.skill[i] < 1)
					{
						continue;
					}

					// Only battlesense and light weapons medals are awarded to
					// the highest score. Class medals get awarded to best ones.
					if (i == SK_BATTLE_SENSE || i == SK_LIGHT_WEAPONS)
					{
						if (cl->sess.skillpoints[i] > highestskillpoints)
						{
							highestskillpoints       = cl->sess.skillpoints[i];
							highestskillpointsclient = j;
						}
					}
					else
					{
						if ((cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) > highestskillpointsincrease)
						{
							highestskillpointsincrease = (cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]);
							highestskillpointsclient   = j;
						}
					}
				}

				if (highestskillpointsclient >= 0)
				{
					// highestskillpointsclient is the first client that has this highest
					// score. See if there are more clients with this same score. If so,
					// give them medals too
					for (j = highestskillpointsclient; j < level.numConnectedClients; j++)
					{
						cl = &level.clients[level.sortedClients[j]];

						if (cl->sess.sessionTeam != teamNum)
						{
							continue;
						}

						// Make sure the player got some skills
						if (cl->sess.skill[i] < 1)
						{
							continue;
						}

						// Only battlesense and light weapons medals are awarded to
						// the highest score. Class medals get awarded to best ones.
						if (i == SK_BATTLE_SENSE || i == SK_LIGHT_WEAPONS)
						{
							if (cl->sess.skillpoints[i] == highestskillpoints)
							{
								cl->sess.medals[i]++;
								ClientUserinfoChanged(level.sortedClients[j]);
							}
						}
						else
						{
							if ((cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) == highestskillpointsincrease)
							{
								cl->sess.medals[i]++;
								ClientUserinfoChanged(level.sortedClients[j]);
							}
						}
					}
				}
			}
		}
	}

#ifdef FEATURE_OMNIBOT
	Bot_Util_SendTrigger(NULL, NULL, "Round End.", "roundend");
#endif

	G_BuildEndgameStats();
}

/**
 * @brief The level will stay at the intermission for a minimum of 5 seconds
 * If all players wish to continue, the level will then exit.
 * If one or more players have not acknowledged the continue, the game will
 * wait 10 seconds before going on.
 */
void CheckIntermissionExit(void)
{
	// end-of-level auto-actions
	if (!(fActions & EOM_WEAPONSTATS) && level.time - level.intermissiontime > 300)
	{
		G_matchInfoDump(EOM_WEAPONSTATS);
		fActions |= EOM_WEAPONSTATS;
	}
	if (!(fActions & EOM_MATCHINFO) && level.time - level.intermissiontime > 800)
	{
		G_matchInfoDump(EOM_MATCHINFO);
		fActions |= EOM_MATCHINFO;
	}

	// empty servers will rotate immediatly except in case of gt mapvote
	if (level.numConnectedClients)
	{
		gclient_t *cl;
		int       i, ready = 0, readyVoters = 0;
		qboolean  exit = qfalse;

		for (i = 0; i < level.numConnectedClients; ++i)
		{
			// spectators and people who are still loading
			// don't have to be ready at the end of the round.
			// additionally, make readypercent apply here.

			cl = level.clients + level.sortedClients[i];

			// changed from counting bots to just ignoring them
			if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR
			    || (g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT))
			{
				continue;
			}

			readyVoters++;

			if (cl->pers.ready)
			{
				ready++;
			}
		}

		// if((100.0f * (ready / (level.numConnectedClients * 1.0f))) >=
		// changed to not include spectators since they are excluded above
		if (readyVoters && g_gametype.integer != GT_WOLF_MAPVOTE)
		{
			if ((100.0f * (ready / (readyVoters * 1.0f))) >= (g_intermissionReadyPercent.value * 1.0f))
			{
				exit = qtrue;
			}
		}

		if (level.ref_allready)
		{
			level.ref_allready = qfalse;
			exit               = qtrue;
		}

		// time set to a minute
		// MAPVOTE note - if g_intermissionTime is low there is no time to vote for a map!
		if (!exit && (level.time < (level.intermissiontime + 1000 * g_intermissionTime.integer)))
		{
			return;
		}
	}

	ExitLevel();
}

/**
 * @brief ScoreIsTied
 * @return
 */
qboolean ScoreIsTied(void)
{
	int  a;
	char cs[MAX_STRING_CHARS];
	char *buf;

	// GT_WOLF checks the current value of
	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));

	buf = Info_ValueForKey(cs, "w");
	a   = Q_atoi(buf);

	return a == -1;
}

/**
 * @brief There will be a delay between the time the exit is qualified for
 * and the time everyone is moved to the intermission spot, so you
 * can see the last frag.
 */
void CheckExitRules(void)
{
	char cs[MAX_STRING_CHARS];

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (g_gamestate.integer == GS_INTERMISSION)
	{
		// if ExitLevel has run, but still no new map was loaded, try restarting the map
		if (level.intermissiontime == 0)
		{
			G_Printf("^3%s\n", "WARNING: failed to load the next map or campaign");
			G_Printf("^3%s\n", "Restarting level...");
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			return;
		}
		CheckIntermissionExit();
		return;
	}

	if (level.intermissionQueued)
	{
		level.intermissionQueued = 0;
		BeginIntermission();
		return;
	}

	if (g_timelimit.integer < 0 || g_timelimit.integer > INT_MAX / 60000)
	{
		G_Printf("timelimit %i is out of range, defaulting to 0\n", g_timelimit.integer);
		trap_Cvar_Set("timelimit", "0");
		trap_Cvar_Update(&g_timelimit);
	}

	if (g_timelimit.value != 0.f && !level.warmupTime)
	{
		if ((level.timeCurrent - level.startTime) >= (g_timelimit.value * 60000))
		{
			// Check who has the most players alive
			if (g_gametype.integer == GT_WOLF_LMS)
			{
				int axisSurvivors, alliedSurvivors;

				axisSurvivors   = level.numTeamClients[0] - level.numFinalDead[0];
				alliedSurvivors = level.numTeamClients[1] - level.numFinalDead[1];

				// if team was eliminated < 3 sec before round end, we _properly_ end it here
				if (level.teamEliminateTime)
				{
					G_LogExit(va("%s team eliminated.", level.lmsWinningTeam == TEAM_ALLIES ? "Axis" : "Allied"));
				}

				if (axisSurvivors == alliedSurvivors)
				{
					// First blood wins
					if (level.firstbloodTeam == TEAM_AXIS)
					{
						trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
						Info_SetValueForKey(cs, "w", "0");
						trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
						G_LogExit("Axis team wins by drawing First Blood.");
						trap_SendServerCommand(-1, "print \"Axis team wins by drawing First Blood.\n\"");
					}
					else if (level.firstbloodTeam == TEAM_ALLIES)
					{
						trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
						Info_SetValueForKey(cs, "w", "1");
						trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
						G_LogExit("Allied team wins by drawing First Blood.");
						trap_SendServerCommand(-1, "print \"Allied team wins by drawing First Blood.\n\"");
					}
					else
					{
						// no winner yet - sudden death!
						return;
					}
				}
				else if (axisSurvivors > alliedSurvivors)
				{
					trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
					Info_SetValueForKey(cs, "w", "0");
					trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
					G_LogExit("Axis team has the most survivors.");
					trap_SendServerCommand(-1, "print \"Axis team has the most survivors.\n\"");
					return;
				}
				else
				{
					trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
					Info_SetValueForKey(cs, "w", "1");
					trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
					G_LogExit("Allied team has the most survivors.");
					trap_SendServerCommand(-1, "print \"Allied team has the most survivors.\n\"");
					return;
				}
			}
			else
			{
				// check for sudden death
				if (ScoreIsTied())
				{
					// score is tied, so don't end the game
					return;
				}
			}

			if (level.gameManager)
			{
				G_Script_ScriptEvent(level.gameManager, "trigger", "timelimit_hit");
			}

			// do not allow G_LogExit to be called in non-playing gamestate
			// - This already happens in G_LogExit, but we need it for the print command
			if (g_gamestate.integer != GS_PLAYING)
			{
				return;
			}

			trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
			G_LogExit("Timelimit hit.");

			return;
		}
	}

	// i dont really get the point of the delay anyway, why not end it immediately like maxlives games?
	if (g_gametype.integer == GT_WOLF_LMS)
	{
		if (!level.teamEliminateTime)
		{
			if (level.numFinalDead[0] >= level.numTeamClients[0] && level.numTeamClients[0] > 0)
			{
				level.teamEliminateTime = level.time;
				level.lmsWinningTeam    = TEAM_ALLIES;
				trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
				Info_SetValueForKey(cs, "w", "1");
				trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
			}
			else if (level.numFinalDead[1] >= level.numTeamClients[1] && level.numTeamClients[1] > 0)
			{
				level.teamEliminateTime = level.time;
				level.lmsWinningTeam    = TEAM_AXIS;
				trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
				Info_SetValueForKey(cs, "w", "0");
				trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
			}
		}
		else if (level.teamEliminateTime + 3000 < level.time)
		{
			G_LogExit(va("%s team eliminated.", level.lmsWinningTeam == TEAM_ALLIES ? "Axis" : "Allied"));
		}
		return;
	}

	if (level.numPlayingClients < 2)
	{
		return;
	}

	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0)
		{
			if (level.numFinalDead[0] >= level.numTeamClients[0] && level.numTeamClients[0] > 0)
			{
				trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
				Info_SetValueForKey(cs, "w", "1");
				trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
				G_LogExit("Axis team eliminated.");
			}
			else if (level.numFinalDead[1] >= level.numTeamClients[1] && level.numTeamClients[1] > 0)
			{
				trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
				Info_SetValueForKey(cs, "w", "0");
				trap_SetConfigstring(CS_MULTI_MAPWINNER, cs);
				G_LogExit("Allied team eliminated.");
			}
		}
	}
}

/*
========================================================================
FUNCTIONS CALLED EVERY FRAME
========================================================================
*/

/**
 * @brief Once a frame, check for changes in wolf MP player state
 */
void CheckWolfMP(void)
{
	// check because we run 6 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if (g_gametype.integer >= GT_WOLF)
	{
		switch (g_gamestate.integer)
		{
		case GS_PLAYING:
		case GS_INTERMISSION:
			if (level.intermissiontime && g_gamestate.integer != GS_INTERMISSION)
			{
				trap_Cvar_Set("gamestate", va("%i", GS_INTERMISSION));
			}
			return;
		case  GS_WARMUP: // check warmup latch
			if (!g_doWarmup.integer ||
			    (level.numPlayingClients >= match_minplayers.integer &&
			     level.lastRestartTime + 1000 < level.time && G_readyMatchState()))
			{
				int delay = (g_warmup.integer < 10) ? 11 : g_warmup.integer + 1;

				level.warmupTime = level.time + (delay * 1000);
				trap_Cvar_Set("gamestate", va("%i", GS_WARMUP_COUNTDOWN));
				trap_Cvar_Update(&g_gamestate);
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
			}
			break;
		case GS_WARMUP_COUNTDOWN: // if the warmup time has counted down, restart
			if (level.time > level.warmupTime)
			{
				level.warmupTime += 10000;
				trap_Cvar_Set("g_restarted", "1");
				trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
				level.restarted = qtrue;
				return;
			}
			break;
		default:
			break;
		}
	}
}

/**
 * @brief CheckVote
 */
void CheckVote(void)
{
	if (!level.voteInfo.voteTime ||
	    level.voteInfo.vote_fn == NULL ||
	    level.time - level.voteInfo.voteTime < 1000)
	{
		return;
	}

	{
		int pcnt = vote_percent.integer;
		int total;

		if (pcnt > 99)
		{
			pcnt = 99;
		}
		if (pcnt < 1)
		{
			pcnt = 1;
		}

		if ((g_voting.integer & VOTEF_USE_TOTAL_VOTERS) &&
		    level.time - level.voteInfo.voteTime >= VOTE_TIME)
		{

			total = (level.voteInfo.voteYes
			         + level.voteInfo.voteNo);
		}
		else if (level.voteInfo.vote_fn == G_Kick_v ||
		         level.voteInfo.vote_fn == G_Surrender_v)
		{

			gentity_t *other = &g_entities[level.voteInfo.voteCaller];

			if (!other->client ||
			    other->client->sess.sessionTeam == TEAM_SPECTATOR)
			{

				total = level.voteInfo.numVotingClients;
			}
			else
			{
				total = level.voteInfo.numVotingTeamClients[other->client->sess.sessionTeam == TEAM_AXIS ? 0 : 1];
			}
		}
		else
		{
			total = level.voteInfo.numVotingClients;
		}

		if (level.voteInfo.voteYes > pcnt * total / 100)
		{
			// execute the command, then remove the vote
			if (level.voteInfo.voteYes > total + 1)
			{
				// Don't announce some votes, as in comp mode, it is generally a ref
				// who is policing people who shouldn't be joining and players don't want
				// this sort of spam in the console
				if (level.voteInfo.vote_fn != G_Kick_v)
				{
					AP(va("cpm \"^5Referee changed setting! ^7(%s)\n\"", level.voteInfo.voteString));
				}
				G_LogPrintf("Referee Setting: %s\n", level.voteInfo.voteString);
			}
			else
			{
				AP(va("cpm \"^5Vote passed! ^7(^2Y:%d^7-^1N:%d^7) ^7(%s)\n\"", level.voteInfo.voteYes, level.voteInfo.voteNo, level.voteInfo.voteString));
				G_LogPrintf("Vote Passed: (Y:%d-N:%d) %s\n", level.voteInfo.voteYes, level.voteInfo.voteNo, level.voteInfo.voteString);
			}

			// Perform the passed vote
			level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);

			// don't penalize player if the vote passes.
			if ((g_voting.integer & VOTEF_NO_POPULIST_PENALTY))
			{
				gentity_t *ent = &g_entities[level.voteInfo.voteCaller];

				if (ent->client)
				{
					ent->client->pers.voteCount--;
				}
			}

		}
		else if (level.voteInfo.voteNo && (level.voteInfo.voteNo >= (100 - pcnt) * total / 100))
		{
			// same behavior as a no response vote
			AP(va("cpm \"^1Vote FAILED! ^7(^2Y:%d^7-^1N:%d^7) ^7(%s)\n\"", level.voteInfo.voteYes, level.voteInfo.voteNo, level.voteInfo.voteString));
			G_LogPrintf("Vote Failed: (Y:%d-N:%d) %s\n", level.voteInfo.voteYes, level.voteInfo.voteNo, level.voteInfo.voteString);
		}
		else if (level.time - level.voteInfo.voteTime >= VOTE_TIME) // timeout, no enough vote
		{
			// same behavior as a no response vote
			AP(va("cpm \"^1Vote TIMEOUT! No enough voters to pass vote ^7(^1%d^7/^2%d^7) ^7(%s)\n\"", level.voteInfo.voteYes, pcnt * total / 100, level.voteInfo.voteString));
			G_LogPrintf("Vote TIMEOUT! No enough voters to pass vote (%d/%d) %s\n", level.voteInfo.voteYes, pcnt * total / 100, level.voteInfo.voteString);
		}
		else
		{
			// still waiting for a majority
			return;
		}

		level.voteInfo.voteTime = 0;
		trap_SetConfigstring(CS_VOTE_TIME, "");
	}
}

/**
 * @brief CheckCvars
 */
void CheckCvars(void)
{
	static int g_password_lastMod             = -1;
	static int g_teamForceBalance_lastMod     = -1;
	static int g_lms_teamForceBalance_lastMod = -1;

	if (g_password.modificationCount != g_password_lastMod)
	{
		g_password_lastMod = g_password.modificationCount;
		if (*g_password.string && Q_stricmp(g_password.string, "none"))
		{
			trap_Cvar_Set("g_needpass", "1");
		}
		else
		{
			trap_Cvar_Set("g_needpass", "0");
		}
	}

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		if (g_lms_teamForceBalance.modificationCount != g_lms_teamForceBalance_lastMod)
		{
			g_lms_teamForceBalance_lastMod = g_lms_teamForceBalance.modificationCount;
			if (g_lms_teamForceBalance.integer)
			{
				trap_Cvar_Set("g_balancedteams", "1");
			}
			else
			{
				trap_Cvar_Set("g_balancedteams", "0");
			}
		}
	}
	else
	{
		if (g_teamForceBalance.modificationCount != g_teamForceBalance_lastMod)
		{
			g_teamForceBalance_lastMod = g_teamForceBalance.modificationCount;
			if (g_teamForceBalance.integer)
			{
				trap_Cvar_Set("g_balancedteams", "1");
			}
			else
			{
				trap_Cvar_Set("g_balancedteams", "0");
			}
		}
	}
}

/**
 * @brief Runs thinking code for this frame if necessary
 *
 * @param[in,out] ent
 */
void G_RunThink(gentity_t *ent)
{
	// If paused, push nextthink
	if (level.match_pause != PAUSE_NONE && (ent - g_entities) >= g_maxclients.integer &&
	    ent->nextthink > level.time && strstr(ent->classname, "DPRINTF_") == NULL)
	{
		ent->nextthink += level.time - level.previousTime;
	}

	// run scripting
	if (ent->s.number >= MAX_CLIENTS)
	{
		G_Script_ScriptRun(ent);
	}

	if (ent->nextthink <= 0)
	{
		return;
	}
	if (ent->nextthink > level.time)
	{
		return;
	}

	ent->nextthink = 0;
	if (!ent->think)
	{
		G_Error("NULL ent->think\n");
	}
	ent->think(ent);
}

void G_RunEntity(gentity_t *ent, int msec);

/**
 * @brief G_PositionEntityOnTag
 * @param[in,out] entity
 * @param[in] parent
 * @param[out] tagName
 * @return
 */
qboolean G_PositionEntityOnTag(gentity_t *entity, gentity_t *parent, char *tagName)
{
	int           i;
	orientation_t tag;
	vec3_t        axis[3];

	AnglesToAxis(parent->r.currentAngles, axis);

	VectorCopy(parent->r.currentOrigin, entity->r.currentOrigin);

	if (!trap_GetTag(-1, parent->tagNumber, tagName, &tag))
	{
		return qfalse;
	}

	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->r.currentOrigin, tag.origin[i], axis[i], entity->r.currentOrigin);
	}

	if (entity->client && (entity->s.eFlags & EF_MOUNTEDTANK))
	{
		// moved tank hack to here
		// - fix tank bb
		// - figured out real values, only tag_player is applied,
		// so there are two left:
		// mg42upper attaches to tag_mg42nest[mg42base] at:
		// 0.03125, -1.171875, 27.984375
		// player attaches to tag_playerpo[mg42upper] at:
		// 3.265625, -1.359375, 2.96875
		// this is a hack, by the way.
		entity->r.currentOrigin[0] += 0.03125 + 3.265625;
		entity->r.currentOrigin[1] += -1.171875 + -1.359375;
		entity->r.currentOrigin[2] += 27.984375 + 2.96875;
	}

	G_SetOrigin(entity, entity->r.currentOrigin);

	if (entity->r.linked && !entity->client)
	{
		if (!VectorCompare(entity->oldOrigin, entity->r.currentOrigin))
		{
			trap_LinkEntity(entity);
		}
	}

	return qtrue;
}

/**
 * @brief G_TagLinkEntity
 * @param[in,out] ent
 * @param[in] msec
 */
void G_TagLinkEntity(gentity_t *ent, int msec)
{
	gentity_t *parent = &g_entities[ent->s.torsoAnim];
	vec3_t    origin, angles = { 0, 0, 0 };
	vec3_t    v;

	if (ent->linkTagTime >= level.time)
	{
		return;
	}

	G_RunEntity(parent, msec);

	if (!(parent->s.eFlags & EF_PATH_LINK))
	{
		if (parent->s.pos.trType == TR_LINEAR_PATH)
		{
			int   pos;
			float frac;

			if ((ent->backspline = BG_GetSplineData(parent->s.effect2Time, &ent->back)) == NULL)
			{
				return;
			}

			ent->backdelta = parent->s.pos.trDuration ? (level.time - parent->s.pos.trTime) / ((float)parent->s.pos.trDuration) : 0;

			if (ent->backdelta < 0.f)
			{
				ent->backdelta = 0.f;
			}
			else if (ent->backdelta > 1.f)
			{
				ent->backdelta = 1.f;
			}

			if (ent->back)
			{
				ent->backdelta = 1 - ent->backdelta;
			}

			pos = floor(ent->backdelta * (MAX_SPLINE_SEGMENTS));
			if (pos >= MAX_SPLINE_SEGMENTS)
			{
				pos  = MAX_SPLINE_SEGMENTS - 1;
				frac = ent->backspline->segments[pos].length;
			}
			else
			{
				frac = ((ent->backdelta * (MAX_SPLINE_SEGMENTS)) - pos) * ent->backspline->segments[pos].length;
			}


			VectorMA(ent->backspline->segments[pos].start, frac, ent->backspline->segments[pos].v_norm, v);
			if (parent->s.apos.trBase[0] != 0.f)
			{
				BG_LinearPathOrigin2(parent->s.apos.trBase[0], &ent->backspline, &ent->backdelta, v, ent->back);
			}

			VectorCopy(v, origin);

			if (ent->s.angles2[0] != 0.f)
			{
				BG_LinearPathOrigin2(ent->s.angles2[0], &ent->backspline, &ent->backdelta, v, ent->back);
			}

			VectorCopy(v, ent->backorigin);

			if (ent->s.angles2[0] < 0)
			{
				VectorSubtract(v, origin, v);
				vectoangles(v, angles);
			}
			else if (ent->s.angles2[0] > 0)
			{
				VectorSubtract(origin, v, v);
				vectoangles(v, angles);
			}
			else
			{
				VectorCopy(vec3_origin, origin);
			}

			ent->moving = qtrue;
		}
		else
		{
			ent->moving = qfalse;
		}
	}
	else
	{
		if (parent->moving)
		{
			VectorCopy(parent->backorigin, v);

			ent->back       = parent->back;
			ent->backdelta  = parent->backdelta;
			ent->backspline = parent->backspline;

			VectorCopy(v, origin);

			if (ent->s.angles2[0] != 0.f)
			{
				BG_LinearPathOrigin2(ent->s.angles2[0], &ent->backspline, &ent->backdelta, v, ent->back);
			}

			VectorCopy(v, ent->backorigin);

			if (ent->s.angles2[0] < 0)
			{
				VectorSubtract(v, origin, v);
				vectoangles(v, angles);
			}
			else if (ent->s.angles2[0] > 0)
			{
				VectorSubtract(origin, v, v);
				vectoangles(v, angles);
			}
			else
			{
				VectorCopy(vec3_origin, origin);
			}

			ent->moving = qtrue;
		}
		else
		{
			ent->moving = qfalse;
		}
	}

	if (ent->moving)
	{
		vec3_t    amove, move;
		gentity_t *obstacle;

		VectorSubtract(origin, ent->r.currentOrigin, move);
		VectorSubtract(angles, ent->r.currentAngles, amove);

		if (!G_MoverPush(ent, move, amove, &obstacle))
		{
			script_mover_blocked(ent, obstacle);
		}

		VectorCopy(origin, ent->s.pos.trBase);
		VectorCopy(angles, ent->s.apos.trBase);
	}
	else
	{
		Com_Memset(&ent->s.pos, 0, sizeof(ent->s.pos));
		Com_Memset(&ent->s.apos, 0, sizeof(ent->s.apos));

		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
	}

	ent->linkTagTime = level.time;
}

/**
 * @brief G_DrawEntBBox
 * @param ent
 *
 * @todo TODO: make this client side one day -> a loooooot of saved entities, and less brandwith
 */
void G_DrawEntBBox(gentity_t *ent)
{
	vec3_t maxs, mins;

	if (G_EntitiesFree() < 64)
	{
		return;
	}

	if (g_debugHitboxes.string[0] && Q_isalpha(g_debugHitboxes.string[0]))
	{
		if (ent->classname && !Q_stricmp(ent->classname, g_debugHitboxes.string))
		{
			G_RailBox(ent->r.currentOrigin, ent->r.mins, ent->r.maxs, tv(0.5f, 0.f, 0.5f), ent->s.number);
		}
		return;
	}

	switch (ent->s.eType)
	{
	case ET_CORPSE:
	case ET_PLAYER:
		if (g_debugHitboxes.integer != 3)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		maxs[2] = ClientHitboxMaxZ(ent);
		break;
	case ET_MISSILE:
		if (g_debugHitboxes.integer != 4)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_EXPLOSIVE:
		if (g_debugHitboxes.integer != 5)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_ITEM:
		if (g_debugHitboxes.integer != 6)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_MOVER:
		if (g_debugHitboxes.integer != 7)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_MG42_BARREL:
		if (g_debugHitboxes.integer != 8)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_CONSTRUCTIBLE_INDICATOR:
	case ET_CONSTRUCTIBLE:
	case ET_CONSTRUCTIBLE_MARKER:
		if (g_debugHitboxes.integer != 9)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_TELEPORT_TRIGGER:
	case ET_OID_TRIGGER:
	case ET_TRIGGER_MULTIPLE:
	case ET_TRIGGER_FLAGONLY:
	case ET_TRIGGER_FLAGONLY_MULTIPLE:
		if (g_debugHitboxes.integer != 10)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_CABINET_H:
	case ET_CABINET_A:
	case ET_HEALER:
	case ET_SUPPLIER:
		if (g_debugHitboxes.integer != 11)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_ALARMBOX:
	case ET_PROP:
	case ET_TRAP:
		if (g_debugHitboxes.integer != 12)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_GAMEMODEL:
		if (g_debugHitboxes.integer != 13)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_GENERAL:
		if (g_debugHitboxes.integer != 14)
		{
			return;
		}
		// this is a bit hacky, but well..
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	case ET_AIRSTRIKE_PLANE:
		if (g_debugHitboxes.integer != 15)
		{
			return;
		}
		VectorCopy(ent->r.maxs, maxs);
		VectorCopy(ent->r.mins, mins);
		break;
	default:
		return;
	}

	G_RailBox(ent->r.currentOrigin, mins, maxs, tv(0.f, 1.f, 0.f), ent->s.number);
}

/**
 * @brief G_RunEntity
 * @param[in,out] ent
 * @param[in] msec
 */
void G_RunEntity(gentity_t *ent, int msec)
{
	if (ent->runthisframe)
	{
		return;
	}

	ent->runthisframe = qtrue;

	if (!ent->inuse)
	{
		return;
	}

	if (g_debugHitboxes.integer > 0)
	{
		G_DrawEntBBox(ent);
	}

	if (ent->tagParent)
	{
		G_RunEntity(ent->tagParent, msec);

		if (ent->tagParent)
		{
			if (G_PositionEntityOnTag(ent, ent->tagParent, ent->tagName))
			{
				if (!ent->client)
				{
					if (!ent->s.density)
					{
						BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time);
						VectorAdd(ent->tagParent->r.currentAngles, ent->r.currentAngles, ent->r.currentAngles);
					}
					else
					{
						BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time);
					}
				}
			}
		}
	}
	else if (ent->s.eFlags & EF_PATH_LINK)
	{
		G_TagLinkEntity(ent, msec);
	}

	// hack for instantaneous velocity
	VectorCopy(ent->r.currentOrigin, ent->oldOrigin);

	// check EF_NODRAW status for non-clients
	if (ent - g_entities > level.maxclients)
	{
		if (ent->flags & FL_NODRAW)
		{
			ent->s.eFlags |= EF_NODRAW;
		}
		else
		{
			ent->s.eFlags &= ~EF_NODRAW;
		}
	}

	// clear events that are too old
	if (level.time - ent->eventTime > EVENT_VALID_MSEC)
	{
		if (ent->s.event)
		{
			ent->s.event = 0;
		}
		if (ent->freeAfterEvent)
		{
			// tempEntities or dropped items completely go away after their event
			G_FreeEntity(ent);
			return;
		}
		else if (ent->unlinkAfterEvent)
		{
			// items that will respawn will hide themselves after their pickup event
			ent->unlinkAfterEvent = qfalse;
			trap_UnlinkEntity(ent);
		}
	}

	// temporary entities don't think
	if (ent->freeAfterEvent)
	{
		return;
	}

	// invisible entities don't think
	// NOTE: hack - constructible one does
	if (ent->s.eType != ET_CONSTRUCTIBLE && (ent->entstate == STATE_INVISIBLE || ent->entstate == STATE_UNDERCONSTRUCTION))
	{
		// we want them still to run scripts tho :p
		if (ent->s.number >= MAX_CLIENTS)
		{
			G_Script_ScriptRun(ent);
		}
		return;
	}

	if (!ent->r.linked && ent->neverFree)
	{
		return;
	}

	switch (ent->s.eType)
	{
	case ET_MISSILE:
	case ET_FLAMEBARREL:
	case ET_RAMJET:
		// pausing
		if (level.match_pause == PAUSE_NONE)
		{
			G_RunMissile(ent);
		}
		else
		{
			// During a pause, gotta keep track of stuff in the air
			ent->s.pos.trTime += level.time - level.previousTime;
			// Keep pulsing right for dynmamite
			if (ent->methodOfDeath == MOD_DYNAMITE && ent->s.effect1Time)
			{
				ent->s.effect1Time += level.time - level.previousTime;
			}
			G_RunThink(ent);
		}
		return;
	case  ET_FLAMETHROWER_CHUNK: // Server-side collision for flamethrower
		G_RunFlamechunk(ent);

		// hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);
		return;
	case ET_AIRSTRIKE_PLANE:
		// get current position
		BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->r.currentOrigin, qfalse, ent->s.effect2Time);
		trap_LinkEntity(ent);
		G_RunThink(ent);
		return;
	default:
		break;
	}

	if (ent->s.eType == ET_ITEM || ent->physicsObject)
	{
		G_RunItem(ent);

		// hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent->s.eType == ET_MOVER || ent->s.eType == ET_PROP)
	{
		G_RunMover(ent);

		// hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent - g_entities < MAX_CLIENTS)
	{
		G_RunClient(ent);

		// hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

#ifdef FEATURE_MULTIVIEW
	if (ent->s.eType == ET_PORTAL && G_smvRunCamera(ent))
	{
		return;
	}
#endif

	if ((ent->s.eType == ET_HEALER || ent->s.eType == ET_SUPPLIER) && ent->target_ent)
	{
		ent->target_ent->s.onFireStart = ent->health;
		ent->target_ent->s.onFireEnd   = ent->count;
	}

	G_RunThink(ent);

	// hack for instantaneous velocity
	VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
	VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);
}

/**
 * @brief Advances the non-player objects in the world
 * @param[in] levelTime
 */
void G_RunFrame(int levelTime)
{
	int i, msec;

	// if we are waiting for the level to restart, do nothing
	if (level.restarted)
	{
		return;
	}

	// workaround for q3 bug
	// levelTime will start over when the timelimit expires on dual objective maps.
	if (level.previousTime > level.time)
	{
		level.overTime = level.previousTime;
	}

	levelTime = levelTime + level.overTime;

	// Handling of pause offsets
	if (level.match_pause == PAUSE_NONE)
	{
		level.timeCurrent = levelTime - level.timeDelta;
	}
	else
	{
		level.timeDelta = levelTime - level.timeCurrent;
		if ((level.time % 500) == 0)
		{
			// FIXME: set a PAUSE cs and let the client adjust their local starttimes
			//        instead of this spam
			trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime + level.timeDelta));
		}
	}

	level.frameTime = trap_Milliseconds();

	level.framenum++;
	level.previousTime = level.time;
	level.time         = levelTime;

	msec = level.time - level.previousTime;

	level.axisAirstrikeCounter   -= msec;
	level.alliedAirstrikeCounter -= msec;
	level.axisArtilleryCounter   -= msec;
	level.alliedArtilleryCounter -= msec;

	if (level.axisAirstrikeCounter < 0)
	{
		level.axisAirstrikeCounter = 0;
	}
	if (level.alliedAirstrikeCounter < 0)
	{
		level.alliedAirstrikeCounter = 0;
	}

	if (level.axisArtilleryCounter < 0)
	{
		level.axisArtilleryCounter = 0;
	}
	if (level.alliedArtilleryCounter < 0)
	{
		level.alliedArtilleryCounter = 0;
	}

	// get any cvar changes
	G_UpdateCvars();

	G_ConfigCheckLocked();

	for (i = 0; i < level.num_entities; i++)
	{
		g_entities[i].runthisframe = qfalse;
	}

	// go through all allocated objects
	for (i = 0; i < level.num_entities; i++)
	{
		G_RunEntity(&g_entities[i], msec);
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		ClientEndFrame(&g_entities[level.sortedClients[i]]);
	}

	CheckWolfMP();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// for tracking changes
	CheckCvars();

	G_UpdateTeamMapData();

	if (level.gameManager)
	{
		level.gameManager->s.otherEntityNum  = team_maxLandmines.integer - G_CountTeamLandmines(TEAM_AXIS);
		level.gameManager->s.otherEntityNum2 = team_maxLandmines.integer - G_CountTeamLandmines(TEAM_ALLIES);
	}
#ifdef FEATURE_LUA
	G_LuaHook_RunFrame(levelTime);
#endif

	level.frameStartTime = trap_Milliseconds();
}

// MAPVOTE

/**
 * @brief G_WriteConfigFileString
 * @param[in] s
 * @param[in] f
 */
void G_WriteConfigFileString(const char *s, fileHandle_t f)
{
	if (s[0])
	{
		char buf[MAX_STRING_CHARS];

		buf[0] = '\0';

		//Q_strcat(buf, sizeof(buf), s);
		Q_strncpyz(buf, s, sizeof(buf));
		trap_FS_Write(buf, strlen(buf), f);
	}
	trap_FS_Write("\n", 1, f);
}

/**
 * @brief G_MapVoteInfoWrite
 */
void G_MapVoteInfoWrite()
{
	fileHandle_t f;
	int          i, count = 0;

	trap_FS_FOpenFile("mapvoteinfo.txt", &f, FS_WRITE);

	for (i = 0; i < MAX_VOTE_MAPS; ++i)
	{
		if (level.mapvoteinfo[i].bspName[0])
		{
			trap_FS_Write("[mapvoteinfo]\n", 14, f);

			trap_FS_Write("name             = ", 19, f);
			G_WriteConfigFileString(level.mapvoteinfo[i].bspName, f);
			trap_FS_Write("times_played     = ", 19, f);
			G_WriteConfigFileString(va("%d", level.mapvoteinfo[i].timesPlayed), f);
			trap_FS_Write("last_played      = ", 19, f);
			G_WriteConfigFileString(va("%d", level.mapvoteinfo[i].lastPlayed), f);
			trap_FS_Write("total_votes      = ", 19, f);
			G_WriteConfigFileString(va("%d", level.mapvoteinfo[i].totalVotes), f);
			trap_FS_Write("vote_eligible    = ", 19, f);
			G_WriteConfigFileString(va("%d", level.mapvoteinfo[i].voteEligible), f);

			trap_FS_Write("\n", 1, f);
			count++;
		}
	}
	G_Printf("mapvoteinfo: wrote %d of %d map vote stats\n", count, MAX_VOTE_MAPS);

	trap_FS_FCloseFile(f);
	return;
}

/**
 * @brief G_ReadConfigFileString
 * @param[in] cnf
 * @param[out] s
 * @param[in] size
 */
void G_ReadConfigFileString(char **cnf, char *s, size_t size)
{
	char *t;

	t = COM_ParseExt(cnf, qfalse);
	if (!strcmp(t, "="))
	{
		t = COM_ParseExt(cnf, qfalse);
	}
	else
	{
		G_Printf("G_ReadConfigFileString: warning missing = before "
		         "\"%s\" on line %d\n",
		         t,
		         COM_GetCurrentParseLine());
	}
	s[0] = '\0';
	while (t[0])
	{
		if ((s[0] == '\0' && strlen(t) <= size) ||
		    (strlen(t) + strlen(s) < size))
		{

			Q_strcat(s, size, t);
			Q_strcat(s, size, " ");
		}
		t = COM_ParseExt(cnf, qfalse);
	}
	// trim the trailing space
	if (strlen(s) > 0 && s[strlen(s) - 1] == ' ')
	{
		s[strlen(s) - 1] = '\0';
	}
}

/**
 * @brief G_ReadConfigFileInt
 * @param[in] cnf
 * @param[out] v
 */
void G_ReadConfigFileInt(char **cnf, int *v)
{
	char *t;

	t = COM_ParseExt(cnf, qfalse);
	if (!strcmp(t, "="))
	{
		t = COM_ParseExt(cnf, qfalse);
	}
	else
	{
		G_Printf("G_ReadConfigFileInt: warning missing = before "
		         "\"%s\" on line %d\n",
		         t,
		         COM_GetCurrentParseLine());
	}
	*v = Q_atoi(t);
}

/**
 * @brief G_MapVoteInfoRead
 */
void G_MapVoteInfoRead()
{
	fileHandle_t f;
	int          i;
	int          curMap = -1;
	int          len;
	char         *cnf, *cnf2;
	char         *t;
	char         bspName[128];

	len = trap_FS_FOpenFile("mapvoteinfo.txt", &f, FS_READ) ;

	if (len < 0)
	{
		trap_FS_FCloseFile(f);
		G_Printf("G_MapVoteInfoRead: could not open mapvoteinfo.txt file\n");
		return;
	}

	cnf = Com_Allocate(len + 1);

	if (cnf == NULL)
	{
		trap_FS_FCloseFile(f);
		G_Printf("G_MapVoteInfoRead: memory allocation error for mapvoteinfo.txt data\n");
		return;
	}

	cnf2 = cnf;
	trap_FS_Read(cnf, len, f);
	*(cnf + len) = '\0';
	trap_FS_FCloseFile(f);

	COM_BeginParseSession("MapvoteinfoRead");

	t = COM_Parse(&cnf);
	while (t[0])
	{
		if (!Q_stricmp(t, "name"))
		{
			G_ReadConfigFileString(&cnf, bspName, sizeof(bspName));
			curMap = -1;
			for (i = 0; i < level.mapVoteNumMaps; i++)
			{
				if (!Q_stricmp(bspName, level.mapvoteinfo[i].bspName))
				{
					curMap = i;
					break;
				}
			}
		}
		if (curMap != -1)
		{
			if (!Q_stricmp(t, "times_played"))
			{
				G_ReadConfigFileInt(&cnf, &level.mapvoteinfo[curMap].timesPlayed);
			}
			else if (!Q_stricmp(t, "last_played"))
			{
				G_ReadConfigFileInt(&cnf, &level.mapvoteinfo[curMap].lastPlayed);
			}
			else if (!Q_stricmp(t, "total_votes"))
			{
				G_ReadConfigFileInt(&cnf, &level.mapvoteinfo[curMap].totalVotes);
			}
			else if (!Q_stricmp(t, "vote_eligible"))
			{
				G_ReadConfigFileInt(&cnf, &level.mapvoteinfo[curMap].voteEligible);
			}
			else if (!Q_stricmp(t, "[mapvoteinfo]"))
			{
				// do nothing for the moment
			}
			else if (!t[0])
			{
				// do nothing for another moment (empty token)
			}
			else
			{
				G_Printf("G_MapVoteInfoRead: [mapvoteinfo] parse error near '%s' on line %i\n", t, COM_GetCurrentParseLine());
			}
		}
		t = COM_Parse(&cnf);
	}

	Com_Dealloc(cnf2);

	return;
}
