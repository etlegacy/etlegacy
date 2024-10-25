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
} tvcvarTable_t;

gentity_t g_entities[MAX_GENTITIES];
gclient_t *g_clients;

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

vmCvar_t tvg_gametype;

vmCvar_t g_password;
vmCvar_t sv_privatepassword;
vmCvar_t tvg_maxclients;
vmCvar_t tvg_dedicated;
vmCvar_t tvg_cheats;

vmCvar_t tvg_inactivity;
vmCvar_t g_debugAlloc;
vmCvar_t g_debugBullets;
vmCvar_t tvg_motd;

vmCvar_t tvg_currentRound;
vmCvar_t tvg_gamestate;

vmCvar_t tvg_log;
vmCvar_t tvg_logSync;

vmCvar_t tvg_voiceChatsAllowed;

vmCvar_t tvg_needpass;

vmCvar_t tvg_banIPs;
vmCvar_t tvg_filterBan;

vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

vmCvar_t g_developer;

vmCvar_t tvg_spectatorInactivity;
vmCvar_t server_motd0;
vmCvar_t server_motd1;
vmCvar_t server_motd2;
vmCvar_t server_motd3;
vmCvar_t server_motd4;
vmCvar_t server_motd5;

vmCvar_t refereePassword;

vmCvar_t mod_version;
vmCvar_t mod_url;
vmCvar_t url;

#ifdef FEATURE_LUA
vmCvar_t lua_modules;
vmCvar_t lua_allowedModules;
vmCvar_t tvg_luaModuleList;
#endif

vmCvar_t tvg_protect; // similar to sv_protect game cvar
                      // 0 - no protection - default to have ref for localhost clients on listen servers
                      // 1 - disabled auto ref for localhost clients

// misc
vmCvar_t team_riflegrenades;

vmCvar_t g_fixedphysics;
vmCvar_t g_fixedphysicsfps;
vmCvar_t g_pronedelay;
vmCvar_t g_debugAnim;

vmCvar_t sv_fps;

vmCvar_t tvg_extendedNames;

// flood protection
vmCvar_t tvg_floodProtection;
vmCvar_t tvg_floodLimit;
vmCvar_t tvg_floodWait;

vmCvar_t tvg_queue_ms;
vmCvar_t tvg_autoAction;

tvcvarTable_t gameCvarTable[] =
{
	// don't override the cheat state set by the system
	{ &tvg_cheats,              "sv_cheats",               "",                           0,                                           0, qfalse },
	{ &tvg_dedicated,           "dedicated",               "0",                          0,                                           0, qfalse,},

	{ &sv_fps,                  "sv_fps",                  "20",                         CVAR_SYSTEMINFO,                             0, qfalse,},
	{ &pmove_fixed,             "pmove_fixed",             "0",                          CVAR_SYSTEMINFO | CVAR_ROM,                  0, qfalse,},
	{ &pmove_msec,              "pmove_msec",              "8",                          CVAR_SYSTEMINFO | CVAR_ROM,                  0, qfalse,},

	// noset vars, server info
	{ NULL,                     "gamenametv",              MODNAME_TV,                   CVAR_SERVERINFO | CVAR_ROM,                  0, qfalse },
	{ NULL,                     "gamedate",                __DATE__,                     CVAR_ROM,                                    0, qfalse },
	{ NULL,                     "sv_mapname",              "",                           CVAR_SERVERINFO | CVAR_ROM,                  0, qfalse },
	{ NULL,                     "P",                       "",                           CVAR_SERVERINFO_NOUPDATE,                    0, qfalse,},

	// server info
	{ &tvg_gametype,            "g_gametype",              "4",                          CVAR_SERVERINFO | CVAR_ROM,                  0, qfalse },                              // default to GT_WOLF_CAMPAIGN
	{ &tvg_needpass,            "g_needpass",              "0",                          CVAR_SERVERINFO | CVAR_ROM,                  0, qtrue, },

	{ &tvg_maxclients,          "sv_maxclients",           "20",                         CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse },

	{ &mod_version,             "mod_version",             ETLEGACY_VERSION,             CVAR_SERVERINFO | CVAR_ROM,                  0, qfalse,},
	// points to the URL for mod information, should not be modified by server admin
	{ &mod_url,                 "mod_url",                 MODURL,                       CVAR_SERVERINFO | CVAR_ROM,                  0, qfalse,},
	// configured by the server admin, points to the web pages for the server
	{ &url,                     "URL",                     "",                           CVAR_SERVERINFO | CVAR_ARCHIVE,              0, qfalse,},

	// CVAR_WOLFINFO
	{ &tvg_currentRound,        "g_currentRound",          "0",                          CVAR_WOLFINFO | CVAR_ROM,                    0, qfalse,},
	{ &tvg_gamestate,           "gamestate",               "-1",                         CVAR_WOLFINFO | CVAR_ROM,                    0, qfalse,},

	// change anytime vars
	{ &tvg_floodProtection,     "tvg_floodProtection",     "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE,              0, qtrue, },
	{ &tvg_floodLimit,          "tvg_floodLimit",          "5",                          CVAR_ARCHIVE,                                0, qtrue, },
	{ &tvg_floodWait,           "tvg_floodWait",           "1000",                       CVAR_ARCHIVE,                                0, qtrue, },
	{ &tvg_log,                 "tvg_log",                 "",                           CVAR_ARCHIVE,                                0, qfalse,},
	{ &tvg_logSync,             "tvg_logSync",             "0",                          CVAR_ARCHIVE,                                0, qfalse,},

	{ &g_password,              "g_password",              "none",                       CVAR_USERINFO,                               0, qfalse,},
	{ &sv_privatepassword,      "sv_privatepassword",      "",                           CVAR_TEMP,                                   0, qfalse,},

	{ &tvg_banIPs,              "tvg_banIPs",              "",                           CVAR_ARCHIVE,                                0, qfalse,},
	{ &tvg_filterBan,           "tvg_filterBan",           "1",                          CVAR_ARCHIVE,                                0, qfalse,},

	{ &tvg_inactivity,          "tvg_inactivity",          "0",                          0,                                           0, qtrue, },
	{ &tvg_spectatorInactivity, "tvg_spectatorInactivity", "0",                          0,                                           0, qfalse,},

	{ &tvg_voiceChatsAllowed,   "tvg_voiceChatsAllowed",   "5",                          CVAR_ARCHIVE,                                0, qfalse,},

	{ &refereePassword,         "refereePassword",         "none",                       0,                                           0, qfalse,},

	{ &tvg_motd,                "tvg_motd",                "",                           CVAR_ARCHIVE,                                0, qfalse,},
	{ &server_motd0,            "server_motd0",            " ^NEnemy Territory ^7MOTD ", 0,                                           0, qfalse,},
	{ &server_motd1,            "server_motd1",            "",                           0,                                           0, qfalse,},
	{ &server_motd2,            "server_motd2",            "",                           0,                                           0, qfalse,},
	{ &server_motd3,            "server_motd3",            "",                           0,                                           0, qfalse,},
	{ &server_motd4,            "server_motd4",            "",                           0,                                           0, qfalse,},
	{ &server_motd5,            "server_motd5",            "",                           0,                                           0, qfalse,},

#ifdef FEATURE_LUA
	{ &lua_modules,             "lua_modules",             "",                           0,                                           0, qfalse,},
	{ &lua_allowedModules,      "lua_allowedModules",      "",                           0,                                           0, qfalse,},
	{ &tvg_luaModuleList,       "tvg_luaModuleList",       "",                           0,                                           0, qfalse,},
#endif

	{ &tvg_protect,             "tvg_protect",             "0",                          CVAR_ARCHIVE,                                0, qfalse,},

	{ &tvg_extendedNames,       "tvg_extendedNames",       "1",                          0,                                           0, qfalse,},

	// used in bg_* or g_*
	{ &g_developer,             "developer",               "0",                          CVAR_TEMP,                                   0, qfalse,},
	{ &g_debugAlloc,            "g_debugAlloc",            "0",                          0,                                           0, qfalse,},
	{ &g_debugBullets,          "g_debugBullets",          "0",                          0,                                           0, qfalse,},
	{ &team_riflegrenades,      "team_riflegrenades",      "1",                          CVAR_ROM,                                    0, qfalse,},
	{ &g_fixedphysics,          "g_fixedphysics",          "1",                          CVAR_ROM,                                    0, qfalse,},
	{ &g_fixedphysicsfps,       "g_fixedphysicsfps",       "125",                        CVAR_ROM,                                    0, qfalse,},
	{ &g_pronedelay,            "g_pronedelay",            "0",                          CVAR_ROM,                                    0, qfalse,},
	{ &g_debugAnim,             "g_debugAnim",             "0",                          CVAR_ROM,                                    0, qfalse,},

	// tvgame specific
	{ &tvg_queue_ms,            "ettv_queue_ms",           "-1",                         CVAR_ROM,                                    0, qfalse,},
	{ &tvg_autoAction,          "tvg_autoAction",          "2",                          CVAR_ARCHIVE,                                0, qfalse,},
};

/**
 * @var gameCvarTableSize
 * @brief Made static to avoid aliasing
 */
static int gameCvarTableSize = sizeof(gameCvarTable) / sizeof(gameCvarTable[0]);

void TVG_InitGame(int levelTime, int randomSeed, int restart, int legacyServer, int serverVersion);
void TVG_RunFrame(int levelTime);
void TVG_ShutdownGame(int restart);

/**
 * @brief G_SnapshotCallback
 * @param[in] entityNum
 * @param[in] clientNum
 * @return
 */
qboolean TVG_SnapshotCallback(int entityNum, int clientNum)
{
	return qtrue;
}

int dll_com_trapGetValue;

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
		Com_Printf(S_COLOR_MDGREY "Initializing %s game " S_COLOR_GREEN ETLEGACY_VERSION "\n", MODNAME_TV);
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
	case GAME_ETTV:
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
	TVG_LuaHook_Print(GPRINT_TEXT, text);
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
		TVG_LuaHook_Print(GPRINT_DEVELOPER, text);
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
	TVG_LuaHook_Print(GPRINT_ERROR, text);
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
	return (!tvg_floodProtection.integer || !tvg_floodWait.integer || !tvg_floodLimit.integer) ? qfalse : qtrue;
}

/**
 * @brief TVG_RegisterCvars
 */
void TVG_RegisterCvars(void)
{
	int           i;
	tvcvarTable_t *cv;

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
	int           i;
	tvcvarTable_t *cv;

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
					TVG_LuaShutdown();
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
	}
}

extern void G_InitMemory(void);

/**
 * @brief TVG_ModCheck
 */
static void TVG_ModCheck(void)
{
	char fs_game[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("fs_game", fs_game, sizeof(fs_game));

	if (!Q_stricmp(fs_game, "legacy"))
	{
		level.mod = LEGACY;
	}
	else if (!Q_stricmp(fs_game, "etjump"))
	{
		level.mod = ETJUMP;
	}
	else if (!Q_stricmp(fs_game, "etpro"))
	{
		level.mod = ETPRO;
	}
	else
	{
		level.mod = UNKNOWN;
	}
}

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
	char   gclients[MAX_CVAR_VALUE_STRING];
	char   cs[MAX_INFO_STRING];
	time_t aclock;
	char   timeFt[32];
	int    i;

	// mod version check
	MOD_CHECK_ETLEGACY(etLegacyServer, serverVersion, level.etLegacyServer);

	G_Printf("------- TVGame Initialization -------\n");
	G_Printf("gamenametv: %s\n", MODNAME_TV);
	G_Printf("gamedate: %s\n", __DATE__);

	srand(randomSeed);

	// set some level globals
	{
		qboolean    oldspawning  = level.spawning;
		gamestate_t oldGamestate = level.gamestate;

		Com_Memset(&level, 0, sizeof(level));

		level.spawning  = oldspawning;
		level.gamestate = restart ? oldGamestate : GS_INITIALIZE;
	}

	TVG_ModCheck();

	TVG_InitTVCmds();

	TVG_RegisterCvars();

	TVG_ProcessIPBans();

	G_InitMemory();

	TVG_SetupExtensions();

	level.time      = levelTime;
	level.startTime = levelTime;

	// time
	time(&aclock);
	strftime(timeFt, sizeof(timeFt), "%a %b %d %X %Y", localtime(&aclock));

	if (tvg_log.string[0])
	{
		if (trap_FS_FOpenFile(tvg_log.string, &level.logFile, tvg_logSync.integer ? FS_APPEND_SYNC : FS_APPEND) < 0)
		{
			G_Printf("WARNING: Couldn't open logfile: %s\n", tvg_log.string);
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
	G_LogPrintf("gametype: %s\n", gameNames[tvg_gametype.integer]);

	G_LogPrintf("gametime: %s\n", timeFt);

	TVG_ParseWolfinfo();
	TVG_ParseSvCvars();

	// initialize all entities for this game
	Com_Memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = tvg_maxclients.integer;

	trap_Cvar_VariableStringBuffer("gclients", gclients, sizeof(gclients));

	if (!Q_stricmp(gclients, ""))
	{
		g_clients = Com_Allocate(tvg_maxclients.integer * sizeof(gclient_t));
		Com_Memset(g_clients, 0, tvg_maxclients.integer * sizeof(gclient_t));

		trap_Cvar_Set("gclients", va("%p", g_clients));
	}
	else
	{
		Q_sscanf(gclients, "%p", &g_clients);
	}

	level.sortedClients = Com_Allocate(tvg_maxclients.integer * sizeof(int));
	Com_Memset(level.sortedClients, 0, tvg_maxclients.integer * sizeof(int));

	level.clients = g_clients;

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
	TVG_LuaInit();
#else
	G_Printf("%sNo Lua API available\n", S_COLOR_BLUE);
#endif

	// parse the key/value pairs and spawn gentities
	TVG_SpawnEntitiesFromString();

	TVG_InitSpawnPoints();

	for (i = 0; i < level.num_entities; i++)
	{
		TVG_FreeEntity(g_entities + i);
	}

	level.num_entities = 0;
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t),
	                    &level.clients[0].ps, sizeof(level.clients[0]));

#ifdef FEATURE_LUA
	TVG_LuaHook_InitGame(levelTime, randomSeed, restart);
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
	TVG_LuaHook_ShutdownGame(restart);
	TVG_LuaShutdown();
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

	free(level.sortedClients);
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
 * @brief TVG_InitSpawnPoint
 */
static void TVG_InitSpawnPoint(gentity_t *ent, int index)
{
	gentity_t *target;
	vec3_t    dir;

	VectorCopy(ent->s.origin, level.intermission_origins[index]);
	VectorCopy(ent->s.angles, level.intermission_angles[index]);

	if (ent->target)
	{
		target = TVG_PickTarget(ent->target);
		if (target)
		{
			VectorSubtract(target->s.origin, level.intermission_origins[index], dir);
			vectoangles(dir, level.intermission_angles[index]);
		}
	}
}

/**
 * @brief TVG_InitSpawnPoints
 */
void TVG_InitSpawnPoints(void)
{
	gentity_t *ent = NULL;

	ent = TVG_Find(NULL, FOFS(classname), "info_player_intermission");

	// if info_player_intermission doesn't exist look for info_player_deathmatch
	if (!ent)
	{
		SelectSpawnPoint(vec3_origin, level.intermission_origins[0], level.intermission_angles[0]);
		return;
	}

	while (ent)
	{
		if (!ent->spawnflags)
		{
			TVG_InitSpawnPoint(ent, 0);
		}
		else
		{
			if (ent->spawnflags & 1)
			{
				TVG_InitSpawnPoint(ent, 1);
			}

			if (ent->spawnflags & 2)
			{
				TVG_InitSpawnPoint(ent, 2);
			}
		}

		ent = TVG_Find(ent, FOFS(classname), "info_player_intermission");
	}

	TVG_FindIntermissionPoint();
}

/**
 * @brief This is also used for spectator spawns,
 *        only used at TVG_InitGame and superseded as soon as available by master playerstate,
 *        the only reason this is needed is because of tvg_queue_ms delay
 *        when there is no valid master playerstate for a long time
 */
void TVG_FindIntermissionPoint(void)
{
	char cs[MAX_STRING_CHARS];
	int  winner;

	// if the match hasn't ended yet, and we're just a spectator
	if (!level.intermission)
	{
		VectorCopy(level.intermission_origins[0], level.intermission_origin);
		VectorCopy(level.intermission_angles[0], level.intermission_angle);
		return;
	}

	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
	winner = Q_atoi(Info_ValueForKey(cs, "w"));

	// change from scripting value for winner (0==AXIS, 1==ALLIES) to spawnflag value
	if (winner == 0)
	{
		winner = TEAM_AXIS;
	}
	else
	{
		winner = TEAM_ALLIES;
	}

	VectorCopy(level.intermission_origins[winner], level.intermission_origin);
	VectorCopy(level.intermission_angles[winner], level.intermission_angle);
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

	if (tvg_dedicated.integer)
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
 * @brief Dynamically names a demo and sets up the recording
 */
static void TVG_AutoRecord(void)
{
	trap_SendConsoleCommand(EXEC_APPEND, va("record %s\n", TVG_GenerateFilename()));
}

/**
 * @brief TVG_AutoActions
 */
static void TVG_AutoActions(void)
{
	if ((tvg_autoAction.integer & AA_STATS) && level.lastCmdsUpdate + FRAMETIME <= level.time)
	{
		TVG_SendCommands();
		level.lastCmdsUpdate = level.time;
	}

	if (level.warmup > 0)
	{
		static int processedWarmupCount = -1;
		int        sec                  = (level.warmup - level.time) / 1000;
		int        warmupAnnounceSec    = 10;

		// process warmup actions
		if (sec <= warmupAnnounceSec && processedWarmupCount != level.warmupCount)
		{
			if (tvg_autoAction.integer & AA_DEMORECORD)
			{
				TVG_AutoRecord();
			}

			processedWarmupCount = level.warmupCount;
		}
	}

	// autorecord on late joins
	if (level.validFramenum == 1 && level.gamestate == GS_PLAYING && (tvg_autoAction.integer & AA_DEMORECORD))
	{
		TVG_AutoRecord();
	}
}

/**
 * @brief Advances the world
 * @param[in] levelTime
 */
void TVG_RunFrame(int levelTime)
{
	char     *queueMsg, *s = "";
	int      i;
	int      oldPMType    = level.ettvMasterPs.pm_type;
	int      queueSeconds = tvg_queue_ms.integer / 1000;
	qboolean validFrame;

	level.framenum++;
	level.previousTime = level.time;
	level.time         = levelTime;
	level.frameTime    = level.time - level.previousTime;

	level.numValidMasterClients = 0;
	validFrame                  = trap_TVG_GetPlayerstate(-1, &level.ettvMasterPs);

	if (validFrame)
	{
		level.validFramenum++;
		level.intermission = level.ettvMasterPs.pm_type == PM_INTERMISSION;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			level.ettvMasterClients[i].valid = trap_TVG_GetPlayerstate(i, &level.ettvMasterClients[i].ps);

			if (level.ettvMasterClients[i].valid)
			{
				level.validMasterClients[level.numValidMasterClients++] = i;
			}
		}

		level.validMasterClients[level.numValidMasterClients++] = level.ettvMasterPs.clientNum;
	}

	if (!queueSeconds && validFrame && oldPMType != level.ettvMasterPs.pm_type)
	{
		VectorCopy(level.ettvMasterPs.origin, level.intermission_origin);
		VectorCopy(level.ettvMasterPs.viewangles, level.intermission_angle);
	}

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

	// get any cvar changes
	TVG_UpdateCvars();

	TVG_AutoActions();

	for (i = 0; i < level.numConnectedClients; i++)
	{
		TVG_ClientEndFrame(&level.clients[level.sortedClients[i]]);
	}

#ifdef FEATURE_LUA
	TVG_LuaHook_RunFrame(levelTime);
#endif
}
