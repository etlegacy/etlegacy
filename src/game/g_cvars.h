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

#ifndef INCLUDE_G_CVARS_H
#define INCLUDE_G_CVARS_H

#include "../qcommon/q_shared.h"

#ifdef FEATURE_OMNIBOT
extern vmCvar_t g_OmniBotPath;
extern vmCvar_t g_OmniBotEnable;
extern vmCvar_t g_OmniBotFlags;
extern vmCvar_t g_OmniBotPlaying;
#ifdef ETLEGACY_DEBUG
extern vmCvar_t g_allowBotSwap;
#endif
#endif

extern vmCvar_t g_gametype;

extern vmCvar_t g_log;
extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;               ///< allow this many total, including spectators
extern vmCvar_t g_maxGameClients;           ///< allow this many active
extern vmCvar_t g_minGameClients;           ///< we need at least this many before match actually starts
extern vmCvar_t g_restarted;
extern vmCvar_t g_logSync;
extern vmCvar_t g_logTimestamp;

extern vmCvar_t g_timelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t sv_privatepassword;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;

extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_debugBullets;
#ifdef ALLOW_GSYNC
extern vmCvar_t g_synchronousClients;
#endif // ALLOW_GSYNC
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t voteFlags;

extern vmCvar_t g_complaintlimit;           ///< number of complaints allowed before kick/ban
extern vmCvar_t g_teambleedComplaint;       ///< max health minimum percentage required to consider damage as wanted team bleed (negative value = disable)
extern vmCvar_t g_ipcomplaintlimit;
extern vmCvar_t g_filtercams;
extern vmCvar_t g_maxlives;                 ///< number of respawns allowed (0==infinite)
extern vmCvar_t g_maxlivesRespawnPenalty;
extern vmCvar_t g_voiceChatsAllowed;        ///< number before spam control
extern vmCvar_t g_alliedmaxlives;
extern vmCvar_t g_axismaxlives;
extern vmCvar_t g_fastres;                  ///< Fast medic res'ing
extern vmCvar_t g_enforcemaxlives;          ///< Temp ban with maxlives between rounds

extern vmCvar_t g_needpass;
extern vmCvar_t g_balancedteams;
extern vmCvar_t g_doWarmup;

extern vmCvar_t g_teamForceBalance;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;

extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t g_scriptName;               ///< name of script file to run (instead of default for that map)
extern vmCvar_t g_scriptDebug;              ///< what level of detail do we want script printing to go to.
extern vmCvar_t g_scriptDebugLevel;         ///< filter out script debug messages from other entities
extern vmCvar_t g_scriptDebugTarget;

extern vmCvar_t g_userAim;
extern vmCvar_t g_developer;

// multiplayer
extern vmCvar_t g_redlimbotime;
extern vmCvar_t g_bluelimbotime;
extern vmCvar_t g_medicChargeTime;
extern vmCvar_t g_engineerChargeTime;
extern vmCvar_t g_fieldopsChargeTime;
extern vmCvar_t g_soldierChargeTime;
extern vmCvar_t g_covertopsChargeTime;

extern vmCvar_t g_debugConstruct;
extern vmCvar_t g_landminetimeout;

/// How fast do SP player and allied bots move?
extern vmCvar_t g_movespeed;

extern vmCvar_t g_axismapxp;
extern vmCvar_t g_alliedmapxp;

extern vmCvar_t g_currentCampaign;
extern vmCvar_t g_currentCampaignMap;

// For LMS
extern vmCvar_t g_axiswins;
extern vmCvar_t g_alliedwins;
extern vmCvar_t g_lms_teamForceBalance;
extern vmCvar_t g_lms_roundlimit;
extern vmCvar_t g_lms_matchlimit;
extern vmCvar_t g_lms_currentMatch;
extern vmCvar_t g_lms_lockTeams;
extern vmCvar_t g_lms_followTeamOnly;

extern vmCvar_t g_nextTimeLimit;

extern vmCvar_t g_userTimeLimit;
extern vmCvar_t g_userAlliedRespawnTime;
extern vmCvar_t g_userAxisRespawnTime;
extern vmCvar_t g_currentRound;
extern vmCvar_t g_noTeamSwitching;
extern vmCvar_t g_altStopwatchMode;
extern vmCvar_t g_gamestate;
extern vmCvar_t g_swapteams;

extern vmCvar_t g_antilag;

extern vmCvar_t refereePassword;
extern vmCvar_t shoutcastPassword;
extern vmCvar_t g_spectatorInactivity;
extern vmCvar_t match_latejoin;
extern vmCvar_t match_minplayers;
extern vmCvar_t match_mutespecs;
extern vmCvar_t match_readypercent;
extern vmCvar_t match_timeoutcount;
extern vmCvar_t match_timeoutlength;
extern vmCvar_t match_warmupDamage;
extern vmCvar_t server_motd0;
extern vmCvar_t server_motd1;
extern vmCvar_t server_motd2;
extern vmCvar_t server_motd3;
extern vmCvar_t server_motd4;
extern vmCvar_t server_motd5;
extern vmCvar_t team_maxplayers;
extern vmCvar_t team_nocontrols;

// NOTE!!! If any vote flags are added, MAKE SURE to update the voteFlags struct in bg_misc.c w/appropriate info,
//         menudef.h for the mask and g_main.c for vote_allow_* flag updates
extern vmCvar_t vote_allow_config;
extern vmCvar_t vote_allow_gametype;
extern vmCvar_t vote_allow_kick;
extern vmCvar_t vote_allow_map;
extern vmCvar_t vote_allow_matchreset;
extern vmCvar_t vote_allow_mutespecs;
extern vmCvar_t vote_allow_nextmap;
extern vmCvar_t vote_allow_referee;
extern vmCvar_t vote_allow_shuffleteams;
extern vmCvar_t vote_allow_shuffleteams_norestart;
extern vmCvar_t vote_allow_swapteams;
extern vmCvar_t vote_allow_friendlyfire;
extern vmCvar_t vote_allow_timelimit;
extern vmCvar_t vote_allow_warmupdamage;
extern vmCvar_t vote_allow_antilag;
extern vmCvar_t vote_allow_balancedteams;
extern vmCvar_t vote_allow_muting;
extern vmCvar_t vote_limit;
extern vmCvar_t vote_percent;
extern vmCvar_t vote_allow_surrender;
extern vmCvar_t vote_allow_restartcampaign;
extern vmCvar_t vote_allow_nextcampaign;
extern vmCvar_t vote_allow_poll;
extern vmCvar_t vote_allow_maprestart;
extern vmCvar_t vote_allow_cointoss;

extern vmCvar_t g_debugSkills;
extern vmCvar_t g_heavyWeaponRestriction;
extern vmCvar_t g_autoFireteams;

extern vmCvar_t g_nextmap;
extern vmCvar_t g_nextcampaign;

extern vmCvar_t g_disableComplaints;

extern vmCvar_t g_antiwarp;
extern vmCvar_t g_maxWarp;

#ifdef FEATURE_LUA
extern vmCvar_t lua_modules;
extern vmCvar_t lua_allowedModules;
extern vmCvar_t g_luaModuleList;
#endif

extern vmCvar_t g_guidCheck;
extern vmCvar_t g_protect;

extern vmCvar_t g_dropHealth;
extern vmCvar_t g_dropAmmo;

extern vmCvar_t g_shove;
extern vmCvar_t g_shoveNoZ;

// MAPVOTE
extern vmCvar_t g_mapVoteFlags;
extern vmCvar_t g_maxMapsVotedFor;
extern vmCvar_t g_minMapAge;
extern vmCvar_t g_excludedMaps;
extern vmCvar_t g_resetXPMapCount;

extern vmCvar_t g_campaignFile;

extern vmCvar_t g_countryflags;

// arty/airstrike rate limiting
extern vmCvar_t team_maxAirstrikes;
extern vmCvar_t team_maxArtillery;

// team class/weapon limiting
// classes
extern vmCvar_t team_maxSoldiers;
extern vmCvar_t team_maxMedics;
extern vmCvar_t team_maxEngineers;
extern vmCvar_t team_maxFieldops;
extern vmCvar_t team_maxCovertops;
// weapons
extern vmCvar_t team_maxMortars;
extern vmCvar_t team_maxFlamers;
extern vmCvar_t team_maxMachineguns;
extern vmCvar_t team_maxRockets;
extern vmCvar_t team_maxRiflegrenades;
extern vmCvar_t team_maxLandmines;
// misc
extern vmCvar_t team_riflegrenades;
// skills
extern vmCvar_t skill_soldier;
extern vmCvar_t skill_medic;
extern vmCvar_t skill_engineer;
extern vmCvar_t skill_fieldops;
extern vmCvar_t skill_covertops;
extern vmCvar_t skill_battlesense;
extern vmCvar_t skill_lightweapons;

extern vmCvar_t g_misc;

extern vmCvar_t g_intermissionTime;
extern vmCvar_t g_intermissionReadyPercent;

extern vmCvar_t g_mapScriptDirectory;
extern vmCvar_t g_mapConfigs;
extern vmCvar_t g_customConfig;

extern vmCvar_t g_moverScale;

extern vmCvar_t g_debugHitboxes;
extern vmCvar_t g_debugPlayerHitboxes;

extern vmCvar_t g_voting; ///< see VOTEF_* defines

extern vmCvar_t g_corpses;

extern vmCvar_t g_realHead;

extern vmCvar_t sv_fps;
extern vmCvar_t g_skipCorrection;

extern vmCvar_t g_extendedNames;

#ifdef FEATURE_RATING
extern vmCvar_t g_skillRating;
#endif

#ifdef FEATURE_PRESTIGE
extern vmCvar_t g_prestige;
#endif

#ifdef FEATURE_MULTIVIEW
extern vmCvar_t g_multiview;
#endif

extern vmCvar_t g_stickyCharge;
extern vmCvar_t g_xpSaver;

extern vmCvar_t g_debugForSingleClient;
extern vmCvar_t g_debugEvents;


extern vmCvar_t g_suddenDeath;
extern vmCvar_t g_dropObjDelay;

// flood protection
extern vmCvar_t g_floodProtection;
extern vmCvar_t g_floodLimit;
extern vmCvar_t g_floodWait;

extern vmCvar_t g_etltv_flags;

void G_RegisterCvars(void);

#endif  // #ifndef INCLUDE_G_CVARS_H
