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
 * @file tvg_local.h
 * @brief Local definitions for tvgame module
 */

#ifndef INCLUDE_G_LOCAL_H
#define INCLUDE_G_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../game/bg_public.h"
#include "tvg_public.h"

//==================================================================

// gentity->flags
#define FL_GODMODE              0x00000010
#define FL_NOTARGET             0x00000020
#define FL_TEAMSLAVE            0x00000400  ///< not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_DROPPED_ITEM         0x00001000
#define FL_NO_BOTS              0x00002000  ///< spawn point not for bot use
#define FL_NO_HUMANS            0x00004000  ///< spawn point just for bots
#define FL_NOSTAMINA            0x00008000  ///< cheat flag no stamina
#define FL_NOFATIGUE            0x00010000  ///< cheat flag no fatigue

#define FL_TOGGLE               0x00020000  ///< ent is toggling (doors use this for ex.)
#define FL_KICKACTIVATE         0x00040000  ///< ent has been activated by a kick (doors use this too for ex.)
#define FL_SOFTACTIVATE         0x00000040  ///< ent has been activated while 'walking' (doors use this too for ex.)
//#define FL_DEFENSE_GUARD        0x00080000  ///< warzombie defense pose

#define FL_NODRAW               0x01000000

#define TKFL_MINES              0x00000001
#define TKFL_AIRSTRIKE          0x00000002
#define TKFL_MORTAR             0x00000004

// Worldspawn spawnflags to indicate if a gametype is not supported
#define NO_GT_WOLF      1
#define NO_STOPWATCH    2
#define NO_CHECKPOINT   4 ///< unused
#define NO_LMS          8

#define MAX_CONSTRUCT_STAGES 3

#define ALLOW_AXIS_TEAM         1
#define ALLOW_ALLIED_TEAM       2
#define ALLOW_DISGUISED_CVOPS   4

// Autoaction values
#define AA_DEMORECORD   BIT(0)
#define AA_STATS        BIT(1)

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

//====================================================================

/**
 * @def MODNAME_TV
 * @brief TV Mod name
 */
#define MODNAME_TV "legacyTV"

/**
 * @enum mods_t
 * @brief
 */
typedef enum
{
	LEGACY  = BIT(0),
	ETJUMP  = BIT(1),
	ETPRO   = BIT(2),
	UNKNOWN = BIT(3),
	ALL     = LEGACY | ETJUMP | ETPRO | UNKNOWN
} mods_t;

/**
* @struct tvcmdUsageFlag_e
* @typedef tvcmdUsageFlag_t
*/
typedef enum tvcmdUsageFlag_e
{
	CMD_USAGE_ANY_TIME          = BIT(0),
	CMD_USAGE_INTERMISSION_ONLY = BIT(1),
	CMD_USAGE_NO_INTERMISSION   = BIT(2),
	CMD_USAGE_AUTOUPDATE        = BIT(3)
} tvcmdUsageFlag_t;

/**
* @struct tvcmd_reference_t
* @brief
*/
typedef struct tvcmd_reference_s
{
	char *pszCommandName;
	tvcmdUsageFlag_t flag;
	int value;
	int updateInterval;
	int lastUpdateTime;
	qboolean floodProtected;
	qboolean (*pCommand)(gclient_t *client, struct tvcmd_reference_s *self);
	int mods;
	const char *pszHelpInfo;
} tvcmd_reference_t;

//====================================================================

#define MAX_NETNAME         36
/**
 * @struct gentity_s
 * @typedef gentity_t
 * @brief
 */
struct gentity_s
{
	entityState_t s;                ///< communicated by server to clients
	entityShared_t r;               ///< shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	qboolean inuse;

	const char *classname;          ///< set in QuakeEd
	int spawnflags;                 ///< set in QuakeEd

	int flags;                      ///< FL_* variables

	int freetime;                   ///< level.time when the object was freed

	char *target;

	char *targetname;
	int targetnamehash;             ///< adding a hash for this for faster lookups

	void (*free)(gentity_t *self);

	gentity_t *enemy;

	int allowteams;

	int spawnTime;
};

/**
 * @enum clientConnected_t
 * @brief
 */
typedef enum
{
	CON_DISCONNECTED = 0,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

/**
 * @enum spectatorState_t
 * @brief
 */
typedef enum
{
	SPECTATOR_NOT = 0,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW
} spectatorState_t;

/**
 * @enum playerTeamStateState_t
 * @brief
 */
typedef enum
{
	TEAM_BEGIN = 0,         ///< Beginning a team game, spawn at base
	TEAM_ACTIVE             ///< Now actively playing
} playerTeamStateState_t;

/**
* @enum playerInfo_t
* @brief
*/
typedef enum
{
	INFO_WS = 0,
	INFO_WWS,
	INFO_GSTATS,

	INFO_IMWS,

	INFO_NUM
} playerInfoStats_t;

/**
 * @struct playerTeamState_t
 * @brief
 */
typedef struct
{
	playerTeamStateState_t state;
	int location[3];

} playerTeamState_t;

/**
 * @struct playerTeamStateState_t
 * @brief Client data that stays across multiple levels or tournament restarts
 * this is achieved by writing all the data to vmCvar strings at game shutdown
 * time and reading them back at connection time.  Anything added here
 * MUST be dealt with in TVG_InitSessionData() / TVG_ReadSessionData() / TVG_WriteSessionData()
 */
typedef struct
{
	team_t sessionTeam;
	spectatorState_t spectatorState;
	int spectatorClient;                                ///< for chasecam and follow mode
	int playerType;                                     ///< class
	//int ignoreClients[MAX_CLIENTS / (sizeof(int) * 8)];
	qboolean muted;
	int skill[SK_NUM_SKILLS];                           ///< skill

	int referee;
	int spec_team;

	// flood protection
	int nextReliableTime;                               ///< next time a command can be executed when flood limited
	int numReliableCommands;                            ///< how many commands have we sent
	int nextCommandDecreaseTime;                        ///< next time we decrease numReliableCommands

	qboolean tvchat;

} clientSession_t;

/**
 * @struct ipFilter_s
 * @typedef ipFilter_t
 * @brief
 */
typedef struct ipFilter_s
{
	unsigned mask;
	unsigned compare;
} ipFilter_t;

/**
 * @struct clientPersistant_t
 * @brief Client data that stays across multiple respawns, but is cleared
 * on each level change or team change at TVG_ClientBegin()
 */
typedef struct
{
	clientConnected_t connected;
	usercmd_t cmd;                      ///< we would lose angles if not persistant
	usercmd_t oldcmd;                   ///< previous command processed by pmove()
	qboolean localClient;               ///< true if "ip" info key is "localhost"
	qboolean activateLean;

	char netname[MAX_NETNAME];
	char client_ip[MAX_IP4_LENGTH];     ///< ip 'caching' - it won't change
	char cl_guid[MAX_GUID_LENGTH + 1];

	int enterTime;                      ///< level.time the client entered the game
	int connectTime;                    ///< level.time the client first connected to the server
	playerTeamState_t teamState;        ///< status in teamplay games
	int cmd_debounce;                   ///< Dampening of command spam

	bg_character_t *character;
	int characterIndex;
} clientPersistant_t;

/**
* @struct wantsPlayerInfoStats_s
* @typedef wantsPlayerInfoStats_s
* @brief
*/
typedef struct wantsPlayerInfoStats_s
{
	qboolean requested;
	int requestedClientNum;
} wantsPlayerInfoStats_t;

// ===================

/**
 * @struct gclient_s
 * @typedef gclient_t
 * @brief This structure is cleared on each TVG_ClientSpawn(),
 * except for 'client->pers' and 'client->sess'
 */
struct gclient_s
{
	// ps MUST be the first element, because the server expects it
	playerState_t ps;               ///< Communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	qboolean noclip;

	int buttons;
	int oldbuttons;
	int latched_buttons;

	int wbuttons;
	int oldwbuttons;
	int latched_wbuttons;

	// timers
	int inactivityTime;                     ///< kick players when time > this
	qboolean inactivityWarning;             ///< qtrue if the five seoond warning has been given
	int inactivitySecondsLeft;              ///< for displaying a counting-down time on clients (milliseconds before activity kicks in..)

	pmoveExt_t pmext;

	wantsPlayerInfoStats_t wantsInfoStats[INFO_NUM];

	int scoresIndex;
	qboolean wantsScores;
};

// this structure is cleared as each map is entered
#define MAX_SPAWN_VARS           64
#define MAX_SPAWN_VARS_CHARS     2048
#define VOTE_MAXSTRING           256     ///< Same value as MAX_STRING_TOKENS

#define MAX_HISTORY_MAPS 333     ///< (1024 - 2) / 3 ==> 340 (3 chars: 2 for map index (range [O-32]) + 1 space), down to 333 for convenience

typedef struct tvgamecommandsplayerstats_s
{
	int lastUpdateTime[MAX_CLIENTS];
	qboolean valid[MAX_CLIENTS];
	char data[MAX_CLIENTS][MAX_STRING_CHARS];
} tvgamecommandsplayerstats_t;

typedef struct tvgamecommands_s
{
	qboolean scoreHasTwoParts;
	char score[2][MAX_STRING_CHARS];

	qboolean sraValid;
	char sra[MAX_STRING_CHARS];

	qboolean prValid;
	char pr[MAX_STRING_CHARS];

	int scoresIndex;
	int scoresCount;
	char scores[MAX_SCORES_CMDS][MAX_STRING_CHARS];

	// "topshots"-related commands
	char astats[MAX_STRING_CHARS];
	char astatsb[MAX_STRING_CHARS];
	char bstats[MAX_STRING_CHARS];
	char bstatsb[MAX_STRING_CHARS];
	char wbstats[MAX_STRING_CHARS];

	qboolean waitingForIMWS;
	int IMWSClientNum;
	tvgamecommandsplayerstats_t infoStats[INFO_NUM];

	qboolean impkdValid;
	char impkd[2][MAX_STRING_CHARS];

	qboolean imprValid;
	char impr[MAX_STRING_CHARS];

	qboolean imptValid;
	char impt[MAX_STRING_CHARS];

	qboolean imsrValid;
	char imsr[MAX_STRING_CHARS];

	qboolean imwaValid;
	char imwa[MAX_STRING_CHARS];

	qboolean immaphistoryValid;
	char immaphistory[MAX_STRING_CHARS];

	qboolean immaplistValid;
	char immaplist[MAX_STRING_CHARS];

	qboolean imvotetallyValid;
	char imvotetally[MAX_STRING_CHARS];
} tvgamecommands_t;

/**
 * @struct level_locals_s
 * @typedef level_locals_t
 * @brief
 */
typedef struct level_locals_s
{
	struct gclient_s *clients;                  ///< [maxclients]

	struct gentity_s *gentities;

	int num_entities;                           ///< current number, <= MAX_GENTITIES - this is an index of highest used entity and grows very quickly to MAX_GENTITIES

	fileHandle_t logFile;

	qboolean etLegacyServer;

	char rawmapname[MAX_QPATH];

	int maxclients;                             ///< store latched cvars here that we want to get at often

	int validFramenum;
	int framenum;
	int time;                                   ///< in msec
	int previousTime;                           ///< so movers can back up when blocked
	int frameTime;                              ///< time delta

	int startTime;                              ///< level.time the map was started

	gamestate_t gamestate;
	int warmup;
	int warmupCount;

	int numConnectedClients;
	int *sortedClients;

	// spawn variables
	qboolean spawning;                          ///< the G_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];         ///< key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	vec3_t intermission_origins[3];             ///< 0 - spectator, 1 - axis, 2 - allies
	vec3_t intermission_angles[3];

	vec3_t intermission_origin;
	vec3_t intermission_angle;

	qboolean fLocalHost;

	qboolean mapcoordsValid;
	vec2_t mapcoordsMins, mapcoordsMaxs;

	qboolean tempTraceIgnoreEnts[MAX_GENTITIES];

	// sv_cvars
	svCvar_t svCvars[MAX_SVCVARS];
	int svCvarsCount;

	qboolean intermission;

	int lastCmdsUpdate;
	tvgamecommands_t cmds;

	playerState_t ettvMasterPs;
	ettvClientSnapshot_t ettvMasterClients[MAX_CLIENTS];

	int numValidMasterClients;
	int validMasterClients[MAX_CLIENTS];
	int queueSeconds;

	mods_t mod;

	int tvcmdsCount;
	tvcmd_reference_t *tvcmds;
} level_locals_t;

// tvg_spawn.c
#define TVG_SpawnString(key, def, out) TVG_SpawnStringExt(key, def, out, __FILE__, __LINE__)
#define TVG_SpawnFloat(key, def, out) TVG_SpawnFloatExt(key, def, out, __FILE__, __LINE__)
#define TVG_SpawnInt(key, def, out) TVG_SpawnIntExt(key, def, out, __FILE__, __LINE__)
#define TVG_SpawnVector(key, def, out) TVG_SpawnVectorExt(key, def, out, __FILE__, __LINE__)
#define TVG_SpawnVector2D(key, def, out) TVG_SpawnVector2DExt(key, def, out, __FILE__, __LINE__)

qboolean TVG_SpawnStringExt(const char *key, const char *defaultString, char **out, const char *file, int line);      // spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean TVG_SpawnFloatExt(const char *key, const char *defaultString, float *out, const char *file, int line);
qboolean TVG_SpawnIntExt(const char *key, const char *defaultString, int *out, const char *file, int line);
qboolean TVG_SpawnVectorExt(const char *key, const char *defaultString, float *out, const char *file, int line);
qboolean TVG_SpawnVector2DExt(const char *key, const char *defaultString, float *out, const char *file, int line);

void TVG_SpawnEntitiesFromString(void);
char *TVG_NewString(const char *string);

gentity_t *TVG_SpawnGEntityFromSpawnVars(void);
qboolean TVG_CallSpawn(gentity_t *ent);

char *TVG_AddSpawnVarToken(const char *string);
void TVG_ParseField(const char *key, const char *value, gentity_t *ent);

// tvg_cmds.c
qboolean TVG_Cmd_Score_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_Ignore_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_UnIgnore_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_IntermissionPlayerKillsDeaths_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_IntermissionPrestige_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_IntermissionPlayerTime_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_IntermissionSkillRating_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_IntermissionWeaponAccuracies_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_IntermissionWeaponStats_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_IntermissionMapList(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_IntermissionMapHistory(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_IntermissionVoteTally(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_WeaponStat_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_wStats_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_sgStats_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_WeaponStatsLeaders_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_Noclip_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_FollowNext_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_FollowPrevious_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_SetViewpos_f(gclient_t *ent, tvcmd_reference_t *self);
qboolean TVG_Cmd_CallVote_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_Cmd_SelectedObjective_f(gclient_t *ent, tvcmd_reference_t *self);

void TVG_StopFollowing(gclient_t *client);
void TVG_Cmd_FollowCycle_f(gclient_t *client, int dir, qboolean skipBots);

void TVG_statsPrint(gclient_t *client, int nType, int updateInterval);

qboolean TVG_CommandsAutoUpdate(tvcmd_reference_t *tvcmd);

qboolean TVG_ServerIsFloodProtected(void);

void TVG_ParseWolfinfo(void);
void TVG_ParseSvCvars(void);

// tvg_utils.c
int TVG_FindConfigstringIndex(const char *name, int start, int max, qboolean create);
void TVG_RemoveConfigstringIndex(const char *name, int start, int max);

int TVG_ModelIndex(const char *name);
int TVG_SoundIndex(const char *name);
int TVG_SkinIndex(const char *name);
int TVG_ShaderIndex(const char *name);
int TVG_CharacterIndex(const char *name);
int TVG_StringIndex(const char *string);
void TVG_TeamCommand(team_t team, const char *cmd);

gentity_t *TVG_Find(gentity_t *from, size_t fieldofs, const char *match);
gentity_t *TVG_FindByTargetname(gentity_t *from, const char *match);
gentity_t *TVG_PickTarget(const char *targetname);

void TVG_InitGentity(gentity_t *e);
gentity_t *TVG_Spawn(void);
void TVG_AnimScriptSound(int soundIndex, vec3_t org, int client);
void TVG_FreeEntity(gentity_t *ent);

char *TVG_VecToStr(const vec3_t v);

void TVG_AddEvent(gclient_t *client, int event, int eventParm);

void TVG_ResetTempTraceIgnoreEnts(void);
void TVG_TempTraceIgnoreEntity(gentity_t *ent);
void TVG_TempTraceIgnoreBodies(void);
void TVG_TempTraceIgnorePlayersAndBodies(void);
void TVG_TempTraceIgnorePlayers(void);
void TVG_TempTraceIgnorePlayersFromTeam(team_t team);

char *TVG_GenerateFilename(void);

long TVG_StringHashValue(const char *fname);

/**
 * @struct mapVotePlayersCount_s
 * @typedef mapVotePlayersCount_t
 * @brief
 */
typedef struct mapVotePlayersCount_s
{
	char map[MAX_QPATH];
	int min;
	int max;

} mapVotePlayersCount_t;

#define MAX_MAPVOTEPLAYERCOUNT 256
extern mapVotePlayersCount_t mapVotePlayersCount[MAX_MAPVOTEPLAYERCOUNT];

/**
 * @struct grefEntity_t
 * @brief cut down refEntity_t w/ only stuff needed for player bone calculation
 * Used only by game code - not engine
 * core: This struct was moved here from etpro_mdx.h
 */
typedef struct
{
	qhandle_t hModel;                   ///< opaque type outside refresh

	vec3_t headAxis[3];

	// most recent data
	vec3_t axis[3];                     ///< rotation vectors
	vec3_t torsoAxis[3];                ///< rotation vectors for torso section of skeletal animation
	// qboolean nonNormalizedAxes;         ///< axis are not normalized, i.e. they have scale
	float origin[3];
	int frame;
	qhandle_t frameModel;
	int torsoFrame;                     ///< skeletal torso can have frame independant of legs frame
	qhandle_t torsoFrameModel;

	// previous data for frame interpolation
	float oldorigin[3];
	int oldframe;
	qhandle_t oldframeModel;
	int oldTorsoFrame;
	qhandle_t oldTorsoFrameModel;
	float backlerp;                     ///< 0.0 = current, 1.0 = old
	float torsoBacklerp;
} grefEntity_t;

// damage flags
#define DAMAGE_RADIUS               0x00000001  ///< damage was indirect
#define DAMAGE_HALF_KNOCKBACK       0x00000002  ///< do less knockback
#define DAMAGE_NO_KNOCKBACK         0x00000008  ///< do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION        0x00000020  ///< armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  ///< unused. Armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_DISTANCEFALLOFF      0x00000040  ///< distance falloff

// tvg_misc.c
void TVG_TeleportPlayer(gclient_t *client, const vec3_t origin, const vec3_t angles);

// tvg_client.c
int TVG_TeamCount(int ignoreClientNum, team_t team);
void TVG_SetClientViewAngle(gclient_t *client, const vec3_t angle);
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles);

void TVG_ClientSpawn(gclient_t *client);
void TVG_CalculateRanks(void);

// *LUA* & map configs g_sha1.c
char *G_SHA1(const char *string);

#define CF_ENABLE    1 ///< enables common countryflags functionallity
#define CF_CONNECT   2 ///< draws country name in connect announcer

// tvg_character.c
qboolean G_RegisterCharacter(const char *characterFile, bg_character_t *character);
void G_RegisterPlayerClasses(void);
void G_UpdateCharacter(gclient_t *client);

// tvg_svcmds.c
qboolean TVG_ConsoleCommand(void);
void TVG_ProcessIPBans(void);
qboolean G_FilterIPBanPacket(char *from);
void AddIPBan(const char *str);

// tvg_cmds.c
void TVG_Say(gclient_t *client, gclient_t *target, int mode, const char *chatText);
void TVG_SayTo(gclient_t *ent, gclient_t *other, int mode, int color, const char *name, const char *message, qboolean localize);   // removed static declaration so it would link
qboolean TVG_Cmd_Follow_f(gclient_t *client, tvcmd_reference_t *self);
void TVG_Say_f(gclient_t *client, int mode /*, qboolean arg0*/);
void TVG_PlaySound_Cmd(void);
int TVG_ClientNumbersFromString(char *s, int *plist);
int TVG_ClientNumberFromString(gclient_t *to, char *s);
int TVG_MasterClientNumbersFromString(char *s, int *plist);
int TVG_MasterClientNumberFromString(gclient_t *to, char *s);

char *ConcatArgs(int start);

// tvg_main.c
void TVG_InitSpawnPoints(void);
void TVG_FindIntermissionPoint(void);
void QDECL G_LogPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_Printf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_DPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_Error(const char *fmt, ...) _attribute((noreturn, format(printf, 1, 2)));

// extension interface
qboolean trap_GetValue(char *value, int valueSize, const char *key);
extern int dll_com_trapGetValue;

qboolean trap_TVG_GetPlayerstate(int clientNum, playerState_t *ps);

// tvg_client.c
char *TVG_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void TVG_ClientUserinfoChanged(int clientNum);
void TVG_ClientDisconnect(int clientNum);
void TVG_ClientBegin(int clientNum);
void TVG_ClientCommand(int clientNum);

// tvg_active.c
void TVG_ClientThink(int clientNum);
void TVG_ClientEndFrame(gclient_t *client);
void TVG_ClientThink_cmd(gclient_t *client, usercmd_t *cmd);

// tvg_session.c
void TVG_ReadSessionData(gclient_t *client);
void TVG_InitSessionData(gclient_t *client, const char *userinfo);
void TVG_WriteSessionData(qboolean restart);

// tvg_cmd.c
void Cmd_Activate_f(gentity_t *ent);
void Cmd_Activate2_f(gentity_t *ent);

char *Q_AddCR(char *s);

extern level_locals_t level;
extern gentity_t      g_entities[];     ///< was explicitly set to MAX_ENTITIES

#define FOFS(x) (offsetof(gentity_t, x))

extern vmCvar_t tvg_gametype;

extern vmCvar_t tvg_log;
extern vmCvar_t tvg_logSync;
extern vmCvar_t tvg_dedicated;
extern vmCvar_t tvg_cheats;
extern vmCvar_t tvg_maxclients;               ///< allow this many total, including spectators

extern vmCvar_t g_password;
extern vmCvar_t sv_privatepassword;
extern vmCvar_t g_debugBullets;

extern vmCvar_t tvg_inactivity;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugBullets;
extern vmCvar_t tvg_motd;

extern vmCvar_t tvg_voiceChatsAllowed;        ///< number before spam control

extern vmCvar_t tvg_needpass;

extern vmCvar_t tvg_banIPs;
extern vmCvar_t tvg_filterBan;

extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t g_developer;

extern vmCvar_t tvg_currentRound;
extern vmCvar_t tvg_gamestate;

extern vmCvar_t refereePassword;
extern vmCvar_t tvg_spectatorInactivity;
extern vmCvar_t server_motd0;
extern vmCvar_t server_motd1;
extern vmCvar_t server_motd2;
extern vmCvar_t server_motd3;
extern vmCvar_t server_motd4;
extern vmCvar_t server_motd5;

#ifdef FEATURE_LUA
extern vmCvar_t lua_modules;
extern vmCvar_t lua_allowedModules;
extern vmCvar_t tvg_luaModuleList;
#endif

extern vmCvar_t tvg_voiceChatsAllowed;

extern vmCvar_t tvg_protect;

// misc
extern vmCvar_t team_riflegrenades;

extern vmCvar_t sv_fps;

extern vmCvar_t tvg_extendedNames;

extern vmCvar_t tvg_queue_ms;
extern vmCvar_t tvg_autoAction;

#define TVG_InactivityValue (tvg_inactivity.integer ? tvg_inactivity.integer : 60)

// flood protection
extern vmCvar_t tvg_floodProtection;
extern vmCvar_t tvg_floodLimit;
extern vmCvar_t tvg_floodWait;

void trap_Printf(const char *fmt);
void trap_Error(const char *fmt) _attribute((noreturn));
int trap_Milliseconds(void);
int trap_Argc(void);
void trap_Argv(int n, char *buffer, int bufferLength);
void trap_Args(char *buffer, int bufferLength);
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, int len, fileHandle_t f);
int trap_FS_Write(const void *buffer, int len, fileHandle_t f);
int trap_FS_Rename(const char *from, const char *to);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
void trap_SendConsoleCommand(int exec_when, const char *text);
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void trap_Cvar_Update(vmCvar_t *vmCvar);
void trap_Cvar_Set(const char *varName, const char *value);
int trap_Cvar_VariableIntegerValue(const char *varName);
float trap_Cvar_VariableValue(const char *varName);
void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize);
void trap_Cvar_LatchedVariableStringBuffer(const char *varName, char *buffer, int bufsize);
void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGClient);
void trap_DropClient(int clientNum, const char *reason, int length);
void trap_SendServerCommand(int clientNum, const char *text);
void trap_SetConfigstring(int num, const char *string);
void trap_GetConfigstring(int num, char *buffer, int bufferSize);
void trap_GetUserinfo(int num, char *buffer, int bufferSize);
void trap_SetUserinfo(int num, const char *buffer);
void trap_GetServerinfo(char *buffer, int bufferSize);
void trap_SetBrushModel(gentity_t *ent, const char *name);
void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void trap_TraceNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int trap_PointContents(const vec3_t point, int passEntityNum);
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2);
qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open);
qboolean trap_AreasConnected(int area1, int area2);
void trap_LinkEntity(gentity_t *ent);
void trap_UnlinkEntity(gentity_t *ent);
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxCount);
qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent);
qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent);

int trap_BotAllocateClient(int clientNum);

void trap_GetUsercmd(int clientNum, usercmd_t *cmd);
qboolean trap_GetEntityToken(char *buffer, int bufferSize);
qboolean trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t *orientation);
qboolean trap_LoadTag(const char *filename);

int trap_RealTime(qtime_t *qtime);

int trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void trap_DebugPolygonDelete(int id);

void trap_SnapVector(float *v);

qboolean trap_SendMessage(int clientNum, char *buf, int buflen);
messageStatus_t trap_MessageStatus(int clientNum);

void TVG_RemoveFromAllIgnoreLists(int clientNum);

// HRESULTS
#define G_OK         0
#define G_INVALID   -1
#define G_NOTFOUND  -2

#define AP(x) trap_SendServerCommand(-1, x)                     ///< Print to all
#define CP(x) trap_SendServerCommand(client - level.clients, x) ///< Print to client
#define CPx(x, y) trap_SendServerCommand(x, y)                  ///< Print to id = x

#define HELP_COLUMNS    4

#define CMD_DEBOUNCE    5000    ///< 5s between cmds

/**
 * @struct team_info
 * @brief Team extras
 */
typedef struct
{
	qboolean spec_lock;
	qboolean team_lock;
	char team_name[24];
	int team_score;
	int timeouts;
} team_info;

// tvg_main.c
void TVG_UpdateCvars(void);

// tvg_cmds_ext.c
qboolean TVG_commandCheck(gclient_t *client, const char *cmd);
void TVG_SendCommands(void);
qboolean TVG_commandHelp(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_cmdDebounce(gclient_t *client, const char *pszCommand);
qboolean TVG_commands_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_players_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_viewers_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_say_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_tvchat_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_scores_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_statsall_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_weaponRankings_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_weaponStats_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_weaponStatsLeaders_cmd(gclient_t *client, qboolean doTop, qboolean doWindow);

void TVG_SendMatchInfo(gclient_t *client);
void TVG_InitTVCmds(void);

// tvg_referee.c
qboolean TVG_Cmd_AuthRcon_f(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_ref_cmd(gclient_t *client, tvcmd_reference_t *self);
qboolean TVG_refCommandCheck(gclient_t *client, const char *cmd);
void TVG_refHelp_cmd(gclient_t *client);
void TVG_refWarning_cmd(gclient_t *client);
void TVG_refMute_cmd(gclient_t *client, qboolean mute);
void TVG_refLogout_cmd(gclient_t *client);
void TVG_refPrintf(gclient_t *client, const char *fmt, ...) _attribute((format(printf, 2, 3)));
void TVG_kick_cmd(gclient_t *client);
void TVG_PlayerBan(void);
void TVG_MakeReferee(void);
void TVG_RemoveReferee(void);
void TVG_MuteClient(void);
void TVG_UnMuteClient(void);

extern const char *aTeams[TEAM_NUM_TEAMS];
extern team_info  teamInfo[TEAM_NUM_TEAMS];

// tvg_pmove.c
void TVG_Pmove(pmove_t *pmove);

/**
 * @enum fieldtype_t
 * @brief
 */
typedef enum
{
	F_INT = 0,
	F_FLOAT,
	F_LSTRING,          ///< string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,          ///< string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY,           ///< index on disk, pointer in memory
	F_ITEM,             ///< index on disk, pointer in memory
	F_CLIENT,           ///< index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

/**
 * @struct field_t
 * @brief
 */
typedef struct
{
	char *name;
	size_t ofs;
	fieldtype_t type;
	int flags;
} field_t;

#ifdef FEATURE_LUA
int GetFieldIndex(const char *fieldname);
fieldtype_t GetFieldType(const char *fieldname);
#endif

// g_protect flags
#define G_PROTECT_LOCALHOST_REF        1
#define G_PROTECT_MAX_LIVES_BAN_GUID   2

// g_misc flags
#define G_MISC_SHOVE_Z  BIT(0)

/**
 * @struct consoleCommmandTable_s
 * @typedef consoleCommandTable_t
 * @brief
 */
typedef struct consoleCommandTable_s
{
	char *name;           ///< -
	void (*cmd)(void);    ///< -

} consoleCommandTable_t;

extern const char *gameNames[];

#endif // #ifndef INCLUDE_G_LOCAL_H
