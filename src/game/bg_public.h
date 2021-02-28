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
 * @file bg_public.h
 * @brief Definitions shared by both the server game and client game modules. (server.h includes this)
 *
 * @note Because games can change separately from the main system version, we need a
 * second version that must match between game and cgame
 */

#ifndef INCLUDE_BG_PUBLIC_H
#define INCLUDE_BG_PUBLIC_H

#define GAME_VERSION        "Enemy Territory"
#define GAME_VERSION_DATED  (GAME_VERSION ", ET 2.60b")

#if defined(CGAMEDLL) || defined(FEATURE_SERVERMDX)
#define USE_MDXFILE
#endif

#define SPRINTTIME 20000.0f

#define DEFAULT_GRAVITY     800
#define FORCE_LIMBO_HEALTH  -113
#define GIB_ENT             99999
#define GIB_HEALTH          -175
#define GIB_DAMAGE(health) health - GIB_HEALTH + 1

#define HOLDBREATHTIME      12000

#define RANK_TIED_FLAG      0x4000

#define ITEM_RADIUS         10     ///< item sizes are needed for client side pickup detection
///< - changed the radius so that the items would fit in the 3 new containers

#define MAX_TRACE           8192.0f///< whenever you change this make sure bullet_Endpos for scope weapons is in sync!

#define VOTE_TIME           30000  ///< 30 seconds before vote times out

#define DEFAULT_VIEWHEIGHT  40
#define CROUCH_VIEWHEIGHT   16
#define DEAD_VIEWHEIGHT     -16
#define PRONE_VIEWHEIGHT    -8

#define DEFAULT_BODYHEIGHT     36  ///< delta height -4
#define CROUCH_BODYHEIGHT      24  ///< delta height 8
#define CROUCH_IDLE_BODYHEIGHT 18  ///< delta height 2
#define DEAD_BODYHEIGHT        4   ///< delta height 20
#define PRONE_BODYHEIGHT       -8  ///< delta height 0

#define DEFAULT_BODYHEIGHT_DELTA        -4  ///< default body height 36
#define CROUCH_BODYHEIGHT_DELTA          8  ///< crouch  body height 24
#define CROUCH_IDLE_BODYHEIGHT_DELTA     2  ///< crouch  idle body height 18
#define DEAD_BODYHEIGHT_DELTA            20 ///< dead    body height 4
#define PRONE_BODYHEIGHT_DELTA           0  ///< prone   body height -8

#define PRONE_BODYHEIGHT_BBOX 12    ///< it appears that 12 is the magic number for the minimum maxs[2] that prevents player from getting stuck into the world.
#define DEAD_BODYHEIGHT_BBOX 24     ///< was the result of DEFAULT_VIEWHEIGHT - CROUCH_VIEWHEIGHT (40 - 16) stored in crouchMaxZ

extern vec3_t playerlegsProneMins;
extern vec3_t playerlegsProneMaxs;

/**
 * @var playerHeadProneMins
 * @brief more than just head, try to make a box for all the
 * player model that extends out (weapons and arms too)
 */
extern vec3_t playerHeadProneMins;

/**
 * @var playerHeadProneMaxs
 * @brief more than just head, try to make a box for all the
 * player model that extends out (weapons and arms too)
 */
extern vec3_t playerHeadProneMaxs;

#define MAX_COMMANDMAP_LAYERS   16

// on fire effects
#define FIRE_FLASH_TIME         2000
#define FIRE_FLASH_FADEIN_TIME  1000

#define LIGHTNING_FLASH_TIME    150

#define AAGUN_DAMAGE        25
#define AAGUN_SPREAD        10

#define AAGUN_RATE_OF_FIRE  100
#define MG42_YAWSPEED       300.f      ///< degrees per second

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_BUDDY   2
#define SAY_TEAMNL  3

#define MAX_FIRETEAMS           12
#define MAX_FIRETEAM_MEMBERS    6

#define MAX_SVCVARS 128

#define SVC_EQUAL           0
#define SVC_GREATER         1
#define SVC_GREATEREQUAL    2
#define SVC_LOWER           3
#define SVC_LOWEREQUAL      4
#define SVC_INSIDE          5
#define SVC_OUTSIDE         6
#define SVC_INCLUDE         7
#define SVC_EXCLUDE         8
#define SVC_WITHBITS        9
#define SVC_WITHOUTBITS     10

/**
 * entity->svFlags
 * the server does not know how to interpret most of the values
 * in entityStates (level eType), so the game must explicitly flag
 * special server behaviors
 */

#define SVF_NONE                0x00000000  ///< none
#define SVF_NOCLIENT            0x00000001  ///< don't send entity to clients, even if it has effects
#define SVF_VISDUMMY            0x00000004  ///< this ent is a "visibility dummy" and needs it's master to be sent to clients that can see it even if they can't see the master ent
#define SVF_BOT                 0x00000008
//#define SVF_POW                 0x00000010  ///< Unused

#define SVF_BROADCAST           0x00000020  ///< send to all connected clients
#define SVF_PORTAL              0x00000040  ///< merge a second pvs at origin2 into snapshots
#define SVF_BLANK               0x00000080  ///< removed SVF_USE_CURRENT_ORIGIN as it plain doesnt do anything
#define SVF_NOFOOTSTEPS         0x00000100  ///< Unused

#define SVF_CAPSULE             0x00000200  ///< use capsule for collision detection

#define SVF_VISDUMMY_MULTIPLE   0x00000400  ///< so that one vis dummy can add to snapshot multiple speakers

// recent id changes
#define SVF_SINGLECLIENT        0x00000800  ///< only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO        0x00001000  ///< don't send CS_SERVERINFO updates to this client
// so that it can be updated for ping tools without
// lagging clients
#define SVF_NOTSINGLECLIENT     0x00002000  ///< send entity to everyone but one client
// (entityShared_t->singleClient)

#define SVF_IGNOREBMODELEXTENTS     0x00004000  ///< just use origin for in pvs check for snapshots, ignore the bmodel extents
#define SVF_SELF_PORTAL             0x00008000  ///< use self->origin2 as portal
#define SVF_SELF_PORTAL_EXCLUSIVE   0x00010000  ///< use self->origin2 as portal and DONT add self->origin PVS ents

/**
 * @struct svCvar_s
 * @typedef svCvar_t
 * @brief
 */
typedef struct svCvar_s
{
	char cvarName[MAX_CVAR_VALUE_STRING];
	int mode;
	char Val1[MAX_CVAR_VALUE_STRING];
	char Val2[MAX_CVAR_VALUE_STRING];
} svCvar_t;

/**
 * @struct forceCvar_s
 * @typedef forceCvar_t
 * @brief
 */
typedef struct forceCvar_s
{
	char cvarName[MAX_CVAR_VALUE_STRING];
	char cvarValue[MAX_CVAR_VALUE_STRING];
} forceCvar_t;

// client damage identifiers

/**
 * @enum entState_t
 * @brief Different entity states
 */
typedef enum
{
	STATE_DEFAULT = 0,         ///< ent is linked, can be used and is solid
	STATE_INVISIBLE,           ///< ent is unlinked, can't be used, doesn't think and is not solid
	STATE_UNDERCONSTRUCTION    ///< ent is being constructed
} entState_t;

#define MAX_TAGCONNECTS     64

// zoom sway values
#define ZOOM_PITCH_AMPLITUDE        0.13f
#define ZOOM_PITCH_FREQUENCY        0.24f
#define ZOOM_PITCH_MIN_AMPLITUDE    0.1f       ///< minimum amount of sway even if completely settled on target

#define ZOOM_YAW_AMPLITUDE          0.7f
#define ZOOM_YAW_FREQUENCY          0.12f
#define ZOOM_YAW_MIN_AMPLITUDE      0.2f

#define MAX_OBJECTIVES      8
#define MAX_OID_TRIGGERS    18

#define MAX_GAMETYPES 16

/**
 * @struct mapInfo
 * @brief
 */
typedef struct
{
	const char *mapName;
	const char *mapLoadName;
	const char *imageName;

	int typeBits;
	int cinematic;

	qhandle_t levelShot;
	qboolean active;

	int Timelimit;
	int AxisRespawnTime;
	int AlliedRespawnTime;

	vec2_t mappos;

	const char *briefing;
	const char *lmsbriefing;
	const char *objectives;
} mapInfo;

/// Campaign saves
#define MAX_CAMPAIGNS           512

/// changed this from 6 to 10
#define MAX_MAPS_PER_CAMPAIGN   10

/**
 * @struct cpsMap_t
 * @brief
 */
typedef struct
{
	int mapnameHash;
} cpsMap_t;

/**
 * @struct cpsCampaign_t
 * @brief
 */
typedef struct
{
	int shortnameHash;
	int progress;

	cpsMap_t maps[MAX_MAPS_PER_CAMPAIGN];
} cpsCampaign_t;

/**
 * @struct cpsHeader_t
 * @brief
 */
typedef struct
{
	int ident;
	int version;

	int numCampaigns;
	int profileHash;
} cpsHeader_t;

/**
 * @struct cpsFile_t
 * @brief
 */
typedef struct
{
	cpsHeader_t header;
	cpsCampaign_t campaigns[MAX_CAMPAIGNS];
} cpsFile_t;

/**
 * @struct campaignInfo_t
 * @brief
 */
typedef struct
{
	const char *campaignShortName;
	const char *campaignName;
	const char *campaignDescription;
	const char *nextCampaignShortName;
	const char *maps;
	int mapCount;
	mapInfo *mapInfos[MAX_MAPS_PER_CAMPAIGN];
	vec2_t mapTC[2];
	cpsCampaign_t *cpsCampaign;  ///< if this campaign was found in the campaignsave, more detailed info can be found here

	const char *campaignShotName;
	int campaignCinematic;
	qhandle_t campaignShot;

	qboolean unlocked;
	int progress;

	qboolean initial;
	int order;

	int typeBits;
} campaignInfo_t;

// Random reinforcement seed settings
#define MAX_REINFSEEDS  8
#define REINF_RANGE     16     ///< (0 to n-1 second offset)
#define REINF_BLUEDELT  3      ///< Allies shift offset
#define REINF_REDDELT   2      ///< Axis shift offset
extern const int aReinfSeeds[MAX_REINFSEEDS];

// Client flags for server processing
#define CGF_AUTORELOAD      0x01
#define CGF_STATSDUMP       0x02
#define CGF_AUTOACTIVATE    0x04
#define CGF_PREDICTITEMS    0x08

#define MAX_MOTDLINES   6

#ifdef FEATURE_MULTIVIEW
// Multiview settings
#define MAX_MVCLIENTS               32
#define MV_SCOREUPDATE_INTERVAL     5000   ///< in msec
#endif

#define MAX_CHARACTERS  16

/// GeoIP
#define MAX_COUNTRY_NUM 256

// config strings are a general means of communicating variable length strings
// from the server to all connected clients.

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
/**
 * @addtogroup lua_etvars
 * @{
 */
#define CS_MUSIC                        2
#define CS_MESSAGE                      3      ///< from the map worldspawn's message field
#define CS_MOTD                         4      ///< g_motd string for server message of the day
#define CS_WARMUP                       5      ///< server time when the match will be restarted
#define CS_VOTE_TIME                    6
#define CS_VOTE_STRING                  7
#define CS_VOTE_YES                     8
#define CS_VOTE_NO                      9
#define CS_GAME_VERSION                 10

#define CS_LEVEL_START_TIME             11     ///< so the timer only shows the current level
#define CS_INTERMISSION                 12     ///< when 1, intermission will start in a second or two
#define CS_MULTI_INFO                   13     ///< stores number of objectives, defender and num of spawn targets
#define CS_MULTI_MAPWINNER              14
#define CS_MULTI_OBJECTIVE              15

#define CS_SCREENFADE                   17     ///< used to tell clients to fade their screen to black/normal
#define CS_FOGVARS                      18     ///< used for saving the current state/settings of the fog
#define CS_SKYBOXORG                    19     ///< this is where we should view the skybox from

#define CS_TARGETEFFECT                 20
#define CS_WOLFINFO                     21
#define CS_FIRSTBLOOD                   22     ///< Team that has first blood
#define CS_ROUNDSCORES1                 23     ///< Axis round wins
#define CS_ROUNDSCORES2                 24     ///< Allied round wins
#define CS_MAIN_AXIS_OBJECTIVE          25     ///< unused - Most important current objective
#define CS_MAIN_ALLIES_OBJECTIVE        26     ///< unused - Most important current objective
#define CS_MUSIC_QUEUE                  27
#define CS_SCRIPT_MOVER_NAMES           28
#define CS_CONSTRUCTION_NAMES           29

#define CS_VERSIONINFO                  30     ///< Versioning info for demo playback compatibility
#define CS_REINFSEEDS                   31     ///< Reinforcement seeds
#define CS_SERVERTOGGLES                32     ///< Shows current enable/disabled settings (for voting UI)
#define CS_GLOBALFOGVARS                33
#define CS_AXIS_MAPS_XP                 34
#define CS_ALLIED_MAPS_XP               35
#define CS_INTERMISSION_START_TIME      36
#define CS_ENDGAME_STATS                37
#define CS_CHARGETIMES                  38
#define CS_FILTERCAMS                   39

#define CS_MODINFO                   40
#define CS_SVCVAR                       41
#define CS_CONFIGNAME                   42

#define CS_TEAMRESTRICTIONS             43     ///< Class restrictions have been changed
#define CS_UPGRADERANGE                 44     ///< Upgrade range levels have been changed

#define CS_MODELS                       64
#define CS_SOUNDS                       (CS_MODELS +               MAX_MODELS)              ///< 320 (256)
#define CS_SHADERS                      (CS_SOUNDS +               MAX_SOUNDS)              ///< 576 (256)
#define CS_SHADERSTATE                  (CS_SHADERS +              MAX_CS_SHADERS)          ///< 608 (32) this MUST be after CS_SHADERS
#define CS_SKINS                        (CS_SHADERSTATE +          1)                       ///< 609 (1)
#define CS_CHARACTERS                   (CS_SKINS +                MAX_CS_SKINS)            ///< 673 (64)
#define CS_PLAYERS                      (CS_CHARACTERS +           MAX_CHARACTERS)          ///< 689 (16)
#define CS_MULTI_SPAWNTARGETS           (CS_PLAYERS +              MAX_CLIENTS)             ///< 753 (64)
#define CS_OID_TRIGGERS                 (CS_MULTI_SPAWNTARGETS +   MAX_MULTI_SPAWNTARGETS)  ///< 769 (16)
#define CS_OID_DATA                     (CS_OID_TRIGGERS +         MAX_OID_TRIGGERS)        ///< 787 (18)
#define CS_DLIGHTS                      (CS_OID_DATA +             MAX_OID_TRIGGERS)        ///< 805 (18)
#define CS_SPLINES                      (CS_DLIGHTS +              MAX_DLIGHT_CONFIGSTRINGS)///< 821 (16)
#define CS_TAGCONNECTS                  (CS_SPLINES +              MAX_SPLINE_CONFIGSTRINGS)///< 829 (8)
#define CS_FIRETEAMS                    (CS_TAGCONNECTS +          MAX_TAGCONNECTS)         ///< 893 (64)
#define CS_CUSTMOTD                     (CS_FIRETEAMS +            MAX_FIRETEAMS)           ///< 905 (12)
#define CS_STRINGS                      (CS_CUSTMOTD +             MAX_MOTDLINES)           ///< 911 (6)
#define CS_MAX                          (CS_STRINGS +              MAX_CSSTRINGS)           ///< 943 (32)
/** @}*////< doxygen addtogroup lua_etvars

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

/**
 * @enum gametype_t
 * @brief
 */
typedef enum
{
	GT_SINGLE_PLAYER = 0,///< obsolete
	GT_COOP,             ///< obsolete
	GT_WOLF,
	GT_WOLF_STOPWATCH,
	GT_WOLF_CAMPAIGN,    ///< Exactly the same as GT_WOLF, but uses campaign roulation (multiple maps form one virtual map)
	GT_WOLF_LMS,
	GT_WOLF_MAPVOTE,     ///< Credits go to ETPub team. TU!
	GT_MAX_GAME_TYPE
} gametype_t;

/**
 * @enum gender_t
 * @brief
 */
typedef enum
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTER
} gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

/**
 * @enum pmtype_t
 * @brief
 */
typedef enum
{
	PM_NORMAL = 0,     ///< can accelerate and turn
	PM_NOCLIP,         ///< noclip movement
	PM_SPECTATOR,      ///< still run into walls
	PM_DEAD,           ///< no acceleration or turning, but free falling
	PM_FREEZE,         ///< stuck in place with no control
	PM_INTERMISSION    ///< no movement or status bar
} pmtype_t;

/**
 * @enum weaponstate_t
 * @brief
 */
typedef enum
{
	WEAPON_READY = 0,
	WEAPON_RAISING,
	WEAPON_RAISING_TORELOAD,
	WEAPON_DROPPING,
	WEAPON_DROPPING_TORELOAD,
	WEAPON_READYING,            ///< unused - getting from 'ready' to 'firing'
	WEAPON_RELAXING,            ///< unused - weapon is ready, but since not firing, it's on it's way to a "relaxed" stance
	WEAPON_FIRING,
	WEAPON_FIRINGALT,           ///< unused -
	WEAPON_RELOADING,
} weaponstate_t;

/**
 * @enum weaponstateCompact_t
 * @brief
 */
typedef enum
{
	WSTATE_IDLE = 0,
	WSTATE_SWITCH,
	WSTATE_FIRE,
	WSTATE_RELOAD
} weaponstateCompact_t;

// pmove->pm_flags	(sent as max 16 bits in msg.c)
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_LADDER          4          ///< player is on a ladder
#define PMF_BACKWARDS_JUMP  8          ///< go into backwards land
#define PMF_BACKWARDS_RUN   16         ///< coast down to backwards run
#define PMF_TIME_LAND       32         ///< pm_time is time before rejump
#define PMF_TIME_KNOCKBACK  64         ///< pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP  256        ///< pm_time is waterjump
#define PMF_RESPAWNED       512        ///< clear after attack and jump buttons come up
//#define PMF_PRONE_BIPOD		1024	 ///< prone with a bipod set
#define PMF_FLAILING        2048
#define PMF_FOLLOW          4096       ///< spectate following another player
//#define PMF_TIME_LOAD       8192       ///< unused (hold for this time after a load game, and prevent large thinks)
#define PMF_LIMBO           16384      ///< limbo state, pm_time is time until reinforce
#define PMF_TIME_LOCKPLAYER 32768      ///< Lock all movement and view changes

#define PMF_ALL_TIMES   (PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK | PMF_TIME_LOCKPLAYER)

/**
 * @struct pmoveExt_s
 * @typedef pmoveExt_t
 * @brief
 */
typedef struct pmoveExt_s
{
	qboolean bAutoReload;          ///< do we predict autoreload of weapons

	int jumpTime;                  ///< used in MP to prevent jump accel

	int silencedSideArm;           ///< Keep track of whether the luger/colt is silenced "in holster", prolly want to do this for the kar98 etc too
	int sprintTime;

	int airleft;

	// MG42 aiming
	float varc, harc;
	vec3_t centerangles;

	int proneTime;                 ///< time a go-prone or stop-prone move starts, to sync the animation to

	float proneLegsOffset;         ///< offset legs bounding box

	vec3_t mountedWeaponAngles;    ///< mortar, mg42 (prone), etc

	int weapRecoilTime;            ///< time at which a weapon that has a recoil kickback has been fired last
	int weapRecoilDuration;
	float weapRecoilYaw;
	float weapRecoilPitch;
	int lastRecoilDeltaTime;

	int shoveTime;

#ifdef GAMEDLL
	qboolean shoved;
	int pusher;
#endif

	qboolean releasedFire;

} pmoveExt_t;  ///< data used both in client and server - store it here
///< instead of playerstate to prevent different engine versions of playerstate between XP and MP

#define MAXTOUCH    32

/**
 * @struct pmove_t
 * @brief
 */
typedef struct
{
	// state (in / out)
	playerState_t *ps;
	pmoveExt_t *pmext;
	struct bg_character_s *character;

	// command (in)
	usercmd_t cmd, oldcmd;
	int tracemask;                 ///< collide against these types of surfaces
	int debugLevel;                ///< if set, diagnostic output will be printed
	qboolean noFootsteps;          ///< if the game is setup for no footsteps by the server
	qboolean noWeapClips;          ///< if the game is setup for no weapon clips by the server

	int gametype;
	int ltChargeTime;              ///< fieldopsChargeTime in cgame and ui. Cannot change here because of compatibility
	int soldierChargeTime;
	int engineerChargeTime;
	int medicChargeTime;

	int covertopsChargeTime;

	// results (out)
	int numtouch;
	int touchents[MAXTOUCH];

	vec3_t mins, maxs;             ///< bounding box size

	int watertype;
	int waterlevel;

	float xyspeed;

	int *skill;                    ///< player skills

	// for fixed msec Pmove
	int pmove_fixed;
	int pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void (*trace)(trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
	int (*pointcontents)(const vec3_t point, int passEntityNum);

	/// used to determine if the player move is for prediction if it is, the movement should trigger no events
	qboolean predict;

} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles(playerState_t *ps, pmoveExt_t *pmext, usercmd_t *cmd, void (trace) (trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int tracemask);
int Pmove(pmove_t *pmove);
void PmovePredict(pmove_t *pmove, float frametime);

//===================================================================================

#define PC_SOLDIER              0  ///< shoot stuff
#define PC_MEDIC                1  ///< heal stuff
#define PC_ENGINEER             2  ///< build stuff
#define PC_FIELDOPS             3  ///< bomb stuff
#define PC_COVERTOPS            4  ///< sneak about ;o

#define NUM_PLAYER_CLASSES      5

// leaning flags..
#define STAT_LEAN_LEFT          0x00000001
#define STAT_LEAN_RIGHT         0x00000002

/**
 * @enum statIndex_t
 * @brief player_state->stats[] indexes
 */
typedef enum
{
	STAT_HEALTH = 0,
	STAT_KEYS,                     ///< 16 bit fields
	STAT_DEAD_YAW,                 ///< look this direction when dead
	STAT_MAX_HEALTH,               ///< health / armor limit
	STAT_PLAYER_CLASS,             ///< player class in multiplayer
	STAT_XP,                       ///< "realtime" version of xp that doesnt need to go thru the scoreboard
	STAT_PS_FLAGS,
	STAT_AIRLEFT,                  ///< airtime for CG_DrawBreathBar()
	STAT_SPRINTTIME,               ///< sprinttime for CG_DrawStaminaBar()
	STAT_ANTIWARP_DELAY            ///< extra lag on the lagometer to reflect warp status
} statIndex_t;

/**
 * @enum persEnum_t
 * @brief player_state->persistant[] indexes
 * these fields are the only part of player_state that isn't
 * cleared on respawn
 */
typedef enum
{
	PERS_SCORE = 0,                ///< !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,                     ///< Deprecated. Remove?
	PERS_RANK,
	PERS_TEAM,
	PERS_SPAWN_COUNT,              ///< incremented every respawn
	PERS_ATTACKER,                 ///< clientnum of last damage inflicter
	PERS_KILLED,                   ///< count of the number of times you died
	// these were added for single player awards tracking
	PERS_RESPAWNS_LEFT,            ///< number of remaining respawns
	PERS_RESPAWNS_PENALTY,         ///< how many respawns you have to sit through before respawning again

	PERS_REVIVE_COUNT,
	PERS_HEADSHOTS,                ///< Deprecated. Remove?
	PERS_BLEH_3,

	// mg42                        ///< TODO: I don't understand these here. Can someone explain?
	PERS_HWEAPON_USE,              ///< enum 12 - don't change
	// wolfkick
	PERS_WOLFKICK
} persEnum_t;

// entityState_t->eFlags
#define EF_NONE             0x00000000                         ///< none
#define EF_DEAD             0x00000001                         ///< don't draw a foe marker over players with EF_DEAD
#define EF_NONSOLID_BMODEL  0x00000002                         ///< bmodel is visible, but not solid
#define EF_TELEPORT_BIT     0x00000004                         ///< toggled every time the origin abruptly changes
#define EF_READY            0x00000008                         ///< player is ready

#define EF_CROUCHING        0x00000010                         ///< player is crouching
#define EF_MG42_ACTIVE      0x00000020                         ///< currently using an MG42
#define EF_NODRAW           0x00000040                         ///< may have an event, but no model (unspawned items)
#define EF_FIRING           0x00000080                         ///< for lightning gun
#define EF_INHERITSHADER    EF_FIRING                          ///< some ents will never use EF_FIRING, hijack it for "USESHADER"

#define EF_SPINNING         0x00000100                         ///< added for level editor control of spinning pickup items
#define EF_BREATH           EF_SPINNING                        ///< Characters will not have EF_SPINNING set, hijack for drawing character breath
#define EF_TALK             0x00000200                         ///< draw a talk balloon
#define EF_CONNECTION       0x00000400                         ///< draw a connection trouble sprite
#define EF_SMOKINGBLACK     0x00000800                         ///< like EF_SMOKING only darker & bigger

#define EF_HEADSHOT         0x00001000                         ///< last hit to player was head shot (NOTE: not last hit, but has BEEN shot in the head since respawn)
#define EF_SMOKING          0x00002000                         ///< ET_GENERAL ents will emit smoke if set///< JPW switched to this after my code change
#define EF_OVERHEATING      (EF_SMOKING | EF_SMOKINGBLACK)     ///< ydnar: light smoke/steam effect
#define EF_VOTED            0x00004000                         ///< already cast a vote
#define EF_TAGCONNECT       0x00008000                         ///< connected to another entity via tag
#define EF_MOUNTEDTANK      EF_TAGCONNECT                      ///< duplicated for clarity

#define EF_FAKEBMODEL       0x00010000                         ///< from etpro
#define EF_PATH_LINK        0x00020000                         ///< linking trains together
#define EF_ZOOMING          0x00040000                         ///< client is zooming
#define EF_PRONE            0x00080000                         ///< player is prone

#define EF_PRONE_MOVING     0x00100000                         ///< player is prone and moving
//#ifdef FEATURE_MULTIVIEW
#define EF_VIEWING_CAMERA   0x00200000                         ///< player is viewing a camera
//#endif
#define EF_AAGUN_ACTIVE     0x00400000                         ///< player is manning an AA gun
#define EF_SPARE0           0x00800000                         ///< freed

// !! NOTE: only place flags that don't need to go to the client beyond 0x00800000
#define EF_SPARE1           0x01000000                         ///< freed
#define EF_SPARE2           0x02000000                         ///< freed
#define EF_BOUNCE           0x04000000                         ///< for missiles
#define EF_BOUNCE_HALF      0x08000000                         ///< for missiles
#define EF_MOVER_STOP       0x10000000                         ///< will push otherwise	///< moved down to make space for one more client flag
#define EF_MOVER_BLOCKED    0x20000000                         ///< mover was blocked dont lerp on the client///< moved down to make space for client flag

#define BG_PlayerMounted(eFlags) ((eFlags & EF_MG42_ACTIVE) || (eFlags & EF_MOUNTEDTANK) || (eFlags & EF_AAGUN_ACTIVE))

/**
 * @enum powerup_t
 * @brief
 * @note NOTE: only place flags that don't need to go to the client beyond 0x00800000
 */
typedef enum
{
	PW_NONE = 0,           ///< unused

	PW_INVULNERABLE,

	PW_NOFATIGUE = 4,      ///< not dependant on level.time

	PW_REDFLAG,            ///< not dependant on level.time - flags never expire WASTE: boolean
	PW_BLUEFLAG,           ///< not dependant on level.time - flags never expire WASTE: boolean

	PW_OPS_DISGUISED,      ///< not dependant on level.time
	PW_OPS_CLASS_1,        ///< not dependant on level.time
	PW_OPS_CLASS_2,        ///< not dependant on level.time
	PW_OPS_CLASS_3,        ///< not dependant on level.time

	PW_ADRENALINE,

	PW_BLACKOUT = 14,      ///< spec blackouts. WASTE: we don't need 32bits here (boolean only)...relocate

#ifdef FEATURE_MULTIVIEW
	PW_MVCLIENTLIST = 15,  ///< static MV client info.. need a full 32 bits
#endif

	PW_NUM_POWERUPS
} powerup_t;

/**
 * @enum wkey_t
 * @brief These will probably all change to INV_n to get the word 'key' out of the game.
 * id and DM don't want references to 'keys' in the game.
 * @todo TODO: I'll change to 'INV' as the item becomes 'permanent' and not a test item.
 *
 * @warning Conflicts with types.h
 */
typedef enum
{
	KEY_NONE = 0,
	KEY_1,                 ///< skull
	KEY_2,                 ///< chalice
	KEY_3,                 ///< eye
	KEY_4,                 ///< field radio          unused
	KEY_5,                 ///< satchel charge
	INV_BINOCS,            ///< binoculars
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_10,
	KEY_11,
	KEY_12,
	KEY_13,
	KEY_14,
	KEY_15,
	KEY_16,
	KEY_LOCKED_PICKABLE,   ///< ent can be unlocked with the WP_LOCKPICK. FIXME: remove
	KEY_NUM_KEYS
} wkey_t;

#define NO_AIRSTRIKE    1
#define NO_ARTILLERY    2

/**
 * @enum weapon_t
 * @brief
 *
 * @note We can only use up to 15 in the client-server stream
 * @note Should be 31 now (I added 1 bit in msg.c)
 * @note This cannot be larger than 64 for AI/player weapons!
 */
typedef enum
{
	WP_NONE = 0,               ///< 0  propExplosion() uses this for init
	WP_KNIFE,                  ///< 1
	WP_LUGER,                  ///< 2
	WP_MP40,                   ///< 3
	WP_GRENADE_LAUNCHER,       ///< 4
	WP_PANZERFAUST,            ///< 5
	WP_FLAMETHROWER,           ///< 6
	WP_COLT,                   ///< 7	equivalent american weapon to german luger
	WP_THOMPSON,               ///< 8	equivalent american weapon to german mp40
	WP_GRENADE_PINEAPPLE,      ///< 9

	WP_STEN,                   ///< 10	silenced sten sub-machinegun
	WP_MEDIC_SYRINGE,          ///< 11	broken out from CLASS_SPECIAL per Id request
	WP_AMMO,                   ///< 12	likewise
	WP_ARTY,                   ///< 13
	WP_SILENCER,               ///< 14	used to be sp5
	WP_DYNAMITE,               ///< 15
	WP_SMOKETRAIL,             ///< 16
	WP_MAPMORTAR,              ///< 17
	VERYBIGEXPLOSION,          ///< 18	explosion effect for airplanes
	WP_MEDKIT,                 ///< 19

	WP_BINOCULARS,             ///< 20
	WP_PLIERS,                 ///< 21
	WP_SMOKE_MARKER,           ///< 22	changed name to cause less confusion
	WP_KAR98,                  ///< 23	WolfXP weapons
	WP_CARBINE,                ///< 24
	WP_GARAND,                 ///< 25
	WP_LANDMINE,               ///< 26
	WP_SATCHEL,                ///< 27
	WP_SATCHEL_DET,            ///< 28
	WP_SMOKE_BOMB,             ///< 29

	WP_MOBILE_MG42,            ///< 30
	WP_K43,                    ///< 31
	WP_FG42,                   ///< 32
	WP_DUMMY_MG42,             ///< 33 for storing heat on mounted mg42s...
	WP_MORTAR,                 ///< 34
	WP_AKIMBO_COLT,            ///< 35
	WP_AKIMBO_LUGER,           ///< 36

	WP_GPG40,                  ///< 37
	WP_M7,                     ///< 38
	WP_SILENCED_COLT,          ///< 39

	WP_GARAND_SCOPE,           ///< 40
	WP_K43_SCOPE,              ///< 41
	WP_FG42_SCOPE,             ///< 42
	WP_MORTAR_SET,             ///< 43
	WP_MEDIC_ADRENALINE,       ///< 44
	WP_AKIMBO_SILENCEDCOLT,    ///< 45
	WP_AKIMBO_SILENCEDLUGER,   ///< 46
	WP_MOBILE_MG42_SET,        ///< 47

	// league weapons
	WP_KNIFE_KABAR,            ///< 48
	WP_MOBILE_BROWNING,        ///< 49
	WP_MOBILE_BROWNING_SET,    ///< 50
	WP_MORTAR2,                ///< 51
	WP_MORTAR2_SET,            ///< 52
	WP_BAZOOKA,                ///< 53
	WP_MP34,                   ///< 54
	WP_AIRSTRIKE,              ///< 55

	WP_NUM_WEAPONS             ///< 56
	///< NOTE: this cannot be larger than 64 for AI/player weapons!
} weapon_t;

/**
 * @struct weaponStats_t
 * @brief
 */
typedef struct
{
	int kills, teamkills, killedby;
} weaponStats_t;

/**
 * @enum hitRegion_t
 * @brief
 */
typedef enum
{
	HR_HEAD = 0,
	HR_ARMS,
	HR_BODY,
	HR_LEGS,
	HR_NUM_HITREGIONS,
} hitRegion_t;

typedef enum
{
	HIT_NONE = 0,
	HIT_TEAMSHOT,
	HIT_HEADSHOT,
	HIT_BODYSHOT
} hitEvent_t;

// MAPVOTE

#define MAX_VOTE_MAPS 32

/**
 * @enum skillType_t
 * @brief
 */
typedef enum
{
	SK_BATTLE_SENSE = 0,
	SK_EXPLOSIVES_AND_CONSTRUCTION,
	SK_FIRST_AID,
	SK_SIGNALS,
	SK_LIGHT_WEAPONS,
	SK_HEAVY_WEAPONS,
	SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS,
	SK_NUM_SKILLS
} skillType_t;

/**
 * @enum meansOfDeath_t
 * @brief Means of death
 */
typedef enum
{
	MOD_UNKNOWN = 0,
	MOD_MACHINEGUN,
	MOD_BROWNING,
	MOD_MG42,
	MOD_GRENADE,

	// modified wolf weap mods
	MOD_KNIFE,
	MOD_LUGER,
	MOD_COLT,
	MOD_MP40,
	MOD_THOMPSON,
	MOD_STEN,
	MOD_GARAND,

	MOD_SILENCER,
	MOD_FG42,
	MOD_FG42SCOPE,
	MOD_PANZERFAUST,
	MOD_GRENADE_LAUNCHER,
	MOD_FLAMETHROWER,
	MOD_GRENADE_PINEAPPLE,

	MOD_MAPMORTAR,
	MOD_MAPMORTAR_SPLASH,

	MOD_KICKED,

	MOD_DYNAMITE,
	MOD_AIRSTRIKE,
	MOD_SYRINGE,
	MOD_AMMO,
	MOD_ARTY,

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_EXPLOSIVE,

	MOD_CARBINE,
	MOD_KAR98,
	MOD_GPG40,
	MOD_M7,
	MOD_LANDMINE,
	MOD_SATCHEL,

	MOD_SMOKEBOMB,
	MOD_MOBILE_MG42,
	MOD_SILENCED_COLT,
	MOD_GARAND_SCOPE,

	MOD_CRUSH_CONSTRUCTION,
	MOD_CRUSH_CONSTRUCTIONDEATH,
	MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER,

	MOD_K43,
	MOD_K43_SCOPE,

	MOD_MORTAR,

	MOD_AKIMBO_COLT,
	MOD_AKIMBO_LUGER,
	MOD_AKIMBO_SILENCEDCOLT,
	MOD_AKIMBO_SILENCEDLUGER,

	MOD_SMOKEGRENADE,

	MOD_SWAP_PLACES,

	// keep these 2 entries last
	MOD_SWITCHTEAM,

	MOD_SHOVE,

	MOD_KNIFE_KABAR,
	MOD_MOBILE_BROWNING,
	MOD_MORTAR2,
	MOD_BAZOOKA,
	MOD_BACKSTAB,
	MOD_MP34,

	MOD_NUM_MODS

} meansOfDeath_t;

/**
 * @struct weaponType_s
 * @typedef weaponType_t
 * @brief
 */
typedef enum weaponType_s
{
	WEAPON_TYPE_NONE      = 0,
	WEAPON_TYPE_MELEE     = BIT(0),
	WEAPON_TYPE_PISTOL    = BIT(1),
	WEAPON_TYPE_SMG       = BIT(2),
	WEAPON_TYPE_RIFLE     = BIT(3),
	WEAPON_TYPE_GRENADE   = BIT(4),
	WEAPON_TYPE_RIFLENADE = BIT(5),
	WEAPON_TYPE_MORTAR    = BIT(6),
	WEAPON_TYPE_MG        = BIT(7),
	WEAPON_TYPE_PANZER    = BIT(8),
	WEAPON_TYPE_SYRINGUE  = BIT(9),
	WEAPON_TYPE_SCOPABLE  = BIT(10),
	WEAPON_TYPE_SCOPED    = BIT(11),
	WEAPON_TYPE_SETTABLE  = BIT(12),
	WEAPON_TYPE_SET       = BIT(13),
	WEAPON_TYPE_BEAM      = BIT(14)         ///< flamethrower

} weaponType_t;

/**
 * @struct weaponFiringMode_s
 * @typedef weaponFiringMode_t
 * @brief
 */
typedef enum weaponFiringMode_s
{
	WEAPON_FIRING_MODE_NONE           = 0,
	WEAPON_FIRING_MODE_THROWABLE      = BIT(0),
	WEAPON_FIRING_MODE_ONE_SHOT       = BIT(1),
	WEAPON_FIRING_MODE_MANUAL         = BIT(2),
	WEAPON_FIRING_MODE_SEMI_AUTOMATIC = BIT(3),
	WEAPON_FIRING_MODE_AUTOMATIC      = BIT(4)

} weaponFiringMode_t;

/**
 * @struct weaponAttribut_s
 * @typedef weaponAttribut_t
 * @brief
 */
typedef enum weaponAttribut_s
{
	WEAPON_ATTRIBUT_NONE                = 0,
	WEAPON_ATTRIBUT_SILENCED            = BIT(0),
	WEAPON_ATTRIBUT_FAST_RELOAD         = BIT(1),
	WEAPON_ATTRIBUT_AKIMBO              = BIT(2),
	WEAPON_ATTRIBUT_FIRE_UNDERWATER     = BIT(3),
	WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE = BIT(4),
	WEAPON_ATTRIBUT_KEEP_DESGUISE       = BIT(5),
	WEAPON_ATTRIBUT_SHAKE               = BIT(6),
	WEAPON_ATTRIBUT_CHARGE_TIME         = BIT(7),
	WEAPON_ATTRIBUT_FALL_OFF            = BIT(8)


} weaponAttribut_t;

#define NUM_SKILL_LEVELS 5

/**
 * @struct skilltable_s
 * @typedef skilltable_t
 * @brief
 */
typedef struct skilltable_s
{
	int skill;
	const char *skillNames;
	const char *skillNamesLine1;
	const char *skillNamesLine2;
	const char *medalNames;
	int skillLevels[NUM_SKILL_LEVELS];

} skilltable_t;

/**
 * @struct playerStats_t
 * @brief
 */
typedef struct
{
	weaponStats_t weaponStats[WP_NUM_WEAPONS];
	int selfkills;
	int hitRegions[HR_NUM_HITREGIONS];
	int objectiveStats[MAX_OBJECTIVES];
} playerStats_t;

/**
 * @struct weaponTable_s
 * @typedef weaponTable_t
 * @brief
 */
typedef struct weapontable_s
{
	int weapon;                     ///< bg      - the weapon id reference
	int item;                       ///< bg g    - the item id linked to the weapon entity
	int team;                       ///< bg g    -
	skillType_t skillBased;         ///< bg g cg -
	weapon_t weapAlts;              ///< bg g cg - the id of the alternative weapon
	weapon_t weapEquiv;             ///< g cg    - the id of the opposite team's weapon (but not for WP_GPG40 <-> WP_M7 - see CG_OutOfAmmoChange).
	weapon_t akimboSideArm;         ///< bg g cg -

	weapon_t ammoIndex;             ///< bg g cg - type of weapon ammo this uses.
	weapon_t clipIndex;             ///< bg g cg - which clip this weapon uses. This allows the sniper rifle to use the same clip as the garand, etc.

	int damage;                     ///< g    -
	float spread;                   ///< g cg -
	float spreadScale;              ///< bg   -
	int splashDamage;               ///< g    -
	int splashRadius;               ///< g    -

	int type;                       ///< bg g cg -
	int firingMode;                 ///< bg g cg -
	int attributes;                 ///< bg g cg -

	int zoomOut;                    ///< cg -
	int zoomIn;                     ///< cg -

	// client
	// icons
	const char *desc;               ///< g cg  - description for spawn weapons

	unsigned int indexWeaponStat;   ///< g cg  - index for weapon stat info

	qboolean useAmmo;               ///< bg g cg -
	qboolean useClip;               ///< bg g cg -

	int maxAmmo;                    ///< bg      - max player ammo carrying capacity.
	int uses;                       ///< bg      - how many 'rounds' it takes/costs to fire one cycle.
	int maxClip;                    ///< bg g    - max 'rounds' in a clip.
	int reloadTime;                 ///< bg      - time from start of reload until ready to fire.
	int fireDelayTime;              ///< bg      - time from pressing 'fire' until first shot is fired. (used for delaying fire while weapon is 'readied' in animation)
	int nextShotTime;               ///< bg      - when firing continuously, this is the time between shots
	int grenadeTime;                ///< bg g cg -
	int aimSpreadScaleAdd;          ///< bg      -

	int maxHeat;                    ///< bg g cg - max active firing time before weapon 'overheats' (at which point the weapon will fail)
	int coolRate;                   ///< bg      - how fast the weapon cools down. (per second)
	int heatRecoveryTime;           ///< bg      - time from overheats until weapon can fire again

	int switchTimeBegin;            ///< bg -
	int switchTimeFinish;           ///< bg -
	int altSwitchTimeFrom;          ///< bg -
	int altSwitchTimeTo;            ///< bg -

	float knockback;                ///< bg -
	int muzzlePointOffset[3];       ///< g  - forward, left, up

	int weapRecoilDuration;         ///< bg -
	float weapRecoilPitch[2];       ///< bg -
	float weapRecoilYaw[2];         ///< bg -

	const char *className;          ///< g -
	const char *weapFile;           ///< cg -

	float chargeTimeCoeff[NUM_SKILL_LEVELS];      ///< bg cg -

	meansOfDeath_t mod;                           ///< g - means of death
	meansOfDeath_t splashMod;                     ///< g - splash means of death

} weaponTable_t;

#define WEAPON_CLASS_FOR_MOD_NO       -1
#define WEAPON_CLASS_FOR_MOD_EXPLOSIVE 0
#define WEAPON_CLASS_FOR_MOD_SATCHEL   1
#define WEAPON_CLASS_FOR_MOD_DYNAMITE  2

/**
 * @struct modTable_s
 * @typedef modTable_t
 * @brief
 */
typedef struct modtable_s
{
	meansOfDeath_t mod;                             ///< reference
	weapon_t weaponIcon;                            ///< cg g

	qboolean isHeadshot;                            ///< g
	qboolean isExplosive;                           ///< g

	int weaponClassForMOD;                          ///< g
	int noYellMedic;                                ///< g
	const char *obituaryKillMessage1;               ///< cg
	const char *obituaryKillMessage2;               ///< cg
	const char *obituarySelfKillMessage;            ///< cg
	const char *obituaryNoAttackerMessage;          ///< cg
	const char *modName;                            ///< g - These are just for logging, the client prints its own messages
	skillType_t skillType;                          ///< g
	float defaultKillPoints;                        ///< g
	float splashKillPoints;                         ///< g
	float hitRegionKillPoints[HR_NUM_HITREGIONS];   ///< g
	qboolean hasHitRegion;                          ///< g
	const char *debugReasonMsg;                     ///< g
	unsigned int indexWeaponStat;                   ///< g

} modTable_t;

extern weaponTable_t *GetWeaponTableData(int weaponIndex);

#define IS_VALID_WEAPON(w) ((w) > WP_NONE && (w) < WP_NUM_WEAPONS)
#define IS_VALID_MOD(mod) ((mod) >= MOD_UNKNOWN && (mod) < MOD_NUM_MODS)
#define IS_VALID_AMMUNITION(a) ((a) >= MISSILE_NONE && (a) < MOD_NUM_MISIILES)

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1       0x00000100
#define EV_EVENT_BIT2       0x00000200
#define EV_EVENT_BITS       (EV_EVENT_BIT1 | EV_EVENT_BIT2)

/**
 * @enum entity_event_t
 * @brief
 *
 * @note If you add events also add eventnames - see bg_misc.c
 */
typedef enum
{
	EV_NONE = 0,
	EV_FOOTSTEP,
	//EV_FOOTSTEP_METAL,
	//EV_FOOTSTEP_WOOD,
	//EV_FOOTSTEP_GRASS,
	//EV_FOOTSTEP_GRAVEL,
	//EV_FOOTSTEP_ROOF,
	//EV_FOOTSTEP_SNOW,
	//EV_FOOTSTEP_CARPET,
	EV_FOOTSPLASH = 9,
	//EV_FOOTWADE,
	EV_SWIM = 11,
	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,
	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_FALL_NDIE,
	EV_FALL_DMG_10,
	EV_FALL_DMG_15,
	EV_FALL_DMG_25,
	EV_FALL_DMG_50,
	EV_WATER_TOUCH,        ///< foot touches
	EV_WATER_LEAVE,        ///< foot leaves
	EV_WATER_UNDER,        ///< head touches
	EV_WATER_CLEAR,        ///< head leaves
	EV_ITEM_PICKUP,        ///< normal item pickups are predictable
	EV_ITEM_PICKUP_QUIET,  ///< same, but don't play the default pickup sound as it was specified in the ent
	EV_GLOBAL_ITEM_PICKUP, ///< powerup / team sounds are broadcast to everyone
	EV_NOAMMO,
	EV_WEAPONSWITCHED,
	//EV_EMPTYCLIP,
	EV_FILL_CLIP = 34,
	EV_MG42_FIXED,
	EV_WEAP_OVERHEAT,
	EV_CHANGE_WEAPON,
	EV_CHANGE_WEAPON_2,
	EV_FIRE_WEAPON,
	EV_FIRE_WEAPONB,
	EV_FIRE_WEAPON_LASTSHOT,
	EV_NOFIRE_UNDERWATER,
	EV_FIRE_WEAPON_MG42,       ///< mounted MG
	EV_FIRE_WEAPON_MOUNTEDMG42,///< tank MG
	//EV_ITEM_RESPAWN,
	//EV_ITEM_POP,
	//EV_PLAYER_TELEPORT_IN,
	//EV_PLAYER_TELEPORT_OUT,
	EV_GRENADE_BOUNCE = 49,         ///< eventParm will be the soundindex
	EV_GENERAL_SOUND,
	EV_GENERAL_SOUND_VOLUME,
	EV_GLOBAL_SOUND,       ///< no attenuation
	EV_GLOBAL_CLIENT_SOUND,///< no attenuation, only plays for specified client
	EV_GLOBAL_TEAM_SOUND,  ///< no attenuation, team only
	EV_FX_SOUND,
	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_RAILTRAIL,
	EV_BULLET,             ///< otherEntity is the shooter
	EV_LOSE_HAT,
	EV_PAIN,
	//EV_CROUCH_PAIN,
	//EV_DEATH1,
	//EV_DEATH2,
	//EV_DEATH3,
	EV_OBITUARY = 68,
	EV_STOPSTREAMINGSOUND,///< swiped from sherman
	//EV_POWERUP_QUAD,
	//EV_POWERUP_BATTLESUIT,
	//EV_POWERUP_REGEN,
	EV_GIB_PLAYER = 73,         ///< gib a previously living player
	//EV_DEBUG_LINE,
	//EV_STOPLOOPINGSOUND,
	//EV_TAUNT,
	EV_SMOKE = 77,
	EV_SPARKS,
	EV_SPARKS_ELECTRIC,
	EV_EXPLODE,    ///< func_explosive
	EV_RUBBLE,
	EV_EFFECT,     ///< target_effect
	EV_MORTAREFX,  ///< mortar firing
	EV_SPINUP,     ///< panzerfaust preamble
	//EV_SNOW_ON,
	//EV_SNOW_OFF,
	EV_MISSILE_MISS_SMALL = 87,
	EV_MISSILE_MISS_LARGE,
	EV_MORTAR_IMPACT,
	EV_MORTAR_MISS,
	//EV_SPIT_HIT,
	//EV_SPIT_MISS,
	EV_SHARD = 93,
	EV_JUNK,
	EV_EMITTER,///< generic particle emitter that uses client-side particle scripts
	EV_OILPARTICLES,
	EV_OILSLICK,
	EV_OILSLICKREMOVE,
	//EV_MG42EFX,
	//EV_FLAKGUN1,
	//EV_FLAKGUN2,
	//EV_FLAKGUN3,
	//EV_FLAKGUN4,
	//EV_EXERT1,
	//EV_EXERT2,
	//EV_EXERT3,
	EV_SNOWFLURRY = 107,
	//EV_CONCUSSIVE,
	EV_DUST = 109,
	EV_RUMBLE_EFX,
	EV_GUNSPARKS,
	EV_FLAMETHROWER_EFFECT,
	//EV_POPUP,
	//EV_POPUPBOOK,
	//EV_GIVEPAGE,
	EV_MG42BULLET_HIT_FLESH = 116, ///< these two send the seed as well
	EV_MG42BULLET_HIT_WALL,
	EV_SHAKE,
	EV_DISGUISE_SOUND,
	EV_BUILDDECAYED_SOUND,
	EV_FIRE_WEAPON_AAGUN,
	EV_DEBRIS,
	EV_ALERT_SPEAKER,
	EV_POPUPMESSAGE,
	EV_ARTYMESSAGE,
	EV_AIRSTRIKEMESSAGE,
	EV_MEDIC_CALL,     ///< end of vanilla events
	EV_SHOVE_SOUND,    ///< 127 - ETL shove
	EV_BODY_DP,        ///< 128
	EV_FLAG_INDICATOR, ///< 129 - objective indicator
	EV_MISSILE_FALLING,///< 130
	EV_PLAYER_HIT,     ///< 131
	EV_MAX_EVENTS      ///< 132 - just added as an 'endcap'
} entity_event_t;

extern const char *eventnames[EV_MAX_EVENTS];

/*
 * @enum animNumber_t
 * @brief
 * @note unused
 *
typedef enum
{
    BOTH_DEATH1 = 0,
    BOTH_DEAD1,
    BOTH_DEAD1_WATER,
    BOTH_DEATH2,
    BOTH_DEAD2,
    BOTH_DEAD2_WATER,
    BOTH_DEATH3,
    BOTH_DEAD3,
    BOTH_DEAD3_WATER,

    BOTH_CLIMB,
    BOTH_CLIMB_DOWN,    ///< 10
    BOTH_CLIMB_DISMOUNT,

    BOTH_SALUTE,

    BOTH_PAIN1,    ///< head
    BOTH_PAIN2,    ///< chest
    BOTH_PAIN3,    ///< groin
    BOTH_PAIN4,    ///< right shoulder
    BOTH_PAIN5,    ///< left shoulder
    BOTH_PAIN6,    ///< right knee
    BOTH_PAIN7,    ///< left knee
    BOTH_PAIN8,    ///< 20 dazed

    BOTH_GRAB_GRENADE,

    BOTH_ATTACK1,
    BOTH_ATTACK2,
    BOTH_ATTACK3,
    BOTH_ATTACK4,
    BOTH_ATTACK5,

    BOTH_EXTRA1,
    BOTH_EXTRA2,
    BOTH_EXTRA3,
    BOTH_EXTRA4,    ///< 30
    BOTH_EXTRA5,
    BOTH_EXTRA6,
    BOTH_EXTRA7,
    BOTH_EXTRA8,
    BOTH_EXTRA9,
    BOTH_EXTRA10,
    BOTH_EXTRA11,
    BOTH_EXTRA12,
    BOTH_EXTRA13,
    BOTH_EXTRA14,    ///< 40
    BOTH_EXTRA15,
    BOTH_EXTRA16,
    BOTH_EXTRA17,
    BOTH_EXTRA18,
    BOTH_EXTRA19,
    BOTH_EXTRA20,

    TORSO_GESTURE,
    TORSO_GESTURE2,
    TORSO_GESTURE3,
    TORSO_GESTURE4, ///< 50

    TORSO_DROP,

    TORSO_RAISE,   ///< (low)
    TORSO_ATTACK,
    TORSO_STAND,
    TORSO_STAND_ALT1,
    TORSO_STAND_ALT2,
    TORSO_READY,
    TORSO_RELAX,

    TORSO_RAISE2,  ///< (high)
    TORSO_ATTACK2,  ///< 60
    TORSO_STAND2,
    TORSO_STAND2_ALT1,
    TORSO_STAND2_ALT2,
    TORSO_READY2,
    TORSO_RELAX2,

    TORSO_RAISE3,  ///< (pistol)
    TORSO_ATTACK3,
    TORSO_STAND3,
    TORSO_STAND3_ALT1,
    TORSO_STAND3_ALT2,  ///< 70
    TORSO_READY3,
    TORSO_RELAX3,

    TORSO_RAISE4,  ///< (shoulder)
    TORSO_ATTACK4,
    TORSO_STAND4,
    TORSO_STAND4_ALT1,
    TORSO_STAND4_ALT2,
    TORSO_READY4,
    TORSO_RELAX4,

    TORSO_RAISE5,   ///< 80 (throw)
    TORSO_ATTACK5,
    TORSO_ATTACK5B,
    TORSO_STAND5,
    TORSO_STAND5_ALT1,
    TORSO_STAND5_ALT2,
    TORSO_READY5,
    TORSO_RELAX5,

    TORSO_RELOAD1, ///< (low)
    TORSO_RELOAD2, ///< (high)
    TORSO_RELOAD3,  ///< 90 (pistol)
    TORSO_RELOAD4, ///< (shoulder)

    TORSO_MG42,    ///< firing tripod mounted weapon animation

    TORSO_MOVE,    ///< torso anim to play while moving and not firing (swinging arms type thing)
    TORSO_MOVE_ALT,

    TORSO_EXTRA,
    TORSO_EXTRA2,
    TORSO_EXTRA3,
    TORSO_EXTRA4,
    TORSO_EXTRA5,
    TORSO_EXTRA6,   ///< 100
    TORSO_EXTRA7,
    TORSO_EXTRA8,
    TORSO_EXTRA9,
    TORSO_EXTRA10,

    LEGS_WALKCR,
    LEGS_WALKCR_BACK,
    LEGS_WALK,
    LEGS_RUN,
    LEGS_BACK,
    LEGS_SWIM,      ///< 110
    LEGS_SWIM_IDLE,

    LEGS_JUMP,
    LEGS_JUMPB,
    LEGS_LAND,

    LEGS_IDLE,
    LEGS_IDLE_ALT,///< LEGS_IDLE2
    LEGS_IDLECR,

    LEGS_TURN,

    LEGS_BOOT,     ///< kicking animation

    LEGS_EXTRA1,    ///< 120
    LEGS_EXTRA2,
    LEGS_EXTRA3,
    LEGS_EXTRA4,
    LEGS_EXTRA5,
    LEGS_EXTRA6,
    LEGS_EXTRA7,
    LEGS_EXTRA8,
    LEGS_EXTRA9,
    LEGS_EXTRA10,

    MAX_ANIMATIONS  ///< 130
} animNumber_t;
*/

//extern const char *animStrings[];    ///< unused - text representation for scripting - defined in bg_misc.c

/**
 * @enum weapAnimNumber_t
 * @brief
 */
typedef enum
{
	WEAP_IDLE1 = 0,
	WEAP_IDLE2,
	WEAP_ATTACK1,
	WEAP_ATTACK2,
	WEAP_ATTACK_LASTSHOT,   ///< used when firing the last round before having an empty clip.
	WEAP_DROP,
	WEAP_RAISE,
	WEAP_RELOAD1,
	WEAP_RELOAD2,
	WEAP_RELOAD3,           ///< unused (was used as RAISE2)
	WEAP_ALTSWITCHFROM,     ///< switch from alt fire mode weap (scoped/silencer/etc)
	WEAP_ALTSWITCHTO,       ///< switch to alt fire mode weap
	WEAP_DROP2,             ///< unused
	MAX_WP_ANIMATIONS
} weapAnimNumber_t;

/**
 * @enum hudHeadAnimNumber_e
 * @typedef hudHeadAnimNumber_t
 * @brief
 */
typedef enum hudHeadAnimNumber_e
{
	HD_IDLE1 = 0,
	HD_IDLE2,
	HD_IDLE3,
	HD_IDLE4,
	HD_IDLE5,
	HD_IDLE6,
	HD_IDLE7,
	HD_IDLE8,
	HD_DAMAGED_IDLE1,
	HD_DAMAGED_IDLE2,
	HD_DAMAGED_IDLE3,
	HD_LEFT,
	HD_RIGHT,
	HD_ATTACK,
	HD_ATTACK_END,
	HD_PAIN,
	MAX_HD_ANIMATIONS
} hudHeadAnimNumber_t;

#define ANIMFL_LADDERANIM    0x1
#define ANIMFL_FIRINGANIM    0x2
#define ANIMFL_REVERSED      0x4
#define ANIMFL_RELOADINGANIM 0x8

/**
 * @struct animation_s
 * @typedef animation_t
 * @brief
 */
typedef struct animation_s
{
#ifdef USE_MDXFILE
	qhandle_t mdxFile;
#else
	char mdxFileName[MAX_QPATH];
#endif///< CGAMEDLL
	char name[MAX_QPATH];
	int firstFrame;
	int numFrames;
	int loopFrames;                ///< 0 to numFrames
	int frameLerp;                 ///< msec between frames
	int initialLerp;               ///< msec to get to first frame
	int moveSpeed;
	int animBlend;                 ///< take this long to blend to next anim

	// derived
	int duration;
	int nameHash;
	int flags;
	int movetype;
} animation_t;

/*
 * @enum animHeadNumber_t
 * @brief Head animations
 * @note Unused
typedef enum
{
    HEAD_NEUTRAL_CLOSED = 0,
    HEAD_NEUTRAL_A,
    HEAD_NEUTRAL_O,
    HEAD_NEUTRAL_I,
    HEAD_NEUTRAL_E,
    HEAD_HAPPY_CLOSED,
    HEAD_HAPPY_O,
    HEAD_HAPPY_I,
    HEAD_HAPPY_E,
    HEAD_HAPPY_A,
    HEAD_ANGRY_CLOSED,
    HEAD_ANGRY_O,
    HEAD_ANGRY_I,
    HEAD_ANGRY_E,
    HEAD_ANGRY_A,

    MAX_HEAD_ANIMS
} animHeadNumber_t;
*/

/*
 * @struct headAnimation_s
 * @typedef headAnimation_t
 * @brief Head animations
 * @note Unused
typedef struct headAnimation_s
{
    int firstFrame;
    int numFrames;
} headAnimation_t;
*/

/// flip the togglebit every time an animation
/// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT      (1 << (ANIM_BITS - 1))

/**
 * @enum team_t
 * @brief Head animations
 *
 * @todo TODO: renamed these to team_axis/allies, it really was awful....
 */
typedef enum
{
	TEAM_FREE = 0,
	TEAM_AXIS,
	TEAM_ALLIES,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

/// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME       1000

/**
 * @enum extWeaponStats_e
 * @typedef extWeaponStats_t
 * @brief Weapon stat info: mapping between MOD_ and WP_ types ()
 *
 * @todo FIXME: for new ET weapons
 */
typedef enum extWeaponStats_e
{
	WS_KNIFE = 0,      ///< 0
	WS_KNIFE_KBAR,     ///< 1
	WS_LUGER,          ///< 2
	WS_COLT,           ///< 3
	WS_MP40,           ///< 4
	WS_THOMPSON,       ///< 5
	WS_STEN,           ///< 6
	WS_FG42,           ///< 7
	WS_PANZERFAUST,    ///< 8
	WS_BAZOOKA,        ///< 9
	WS_FLAMETHROWER,   ///< 10
	WS_GRENADE,        ///< 11   -- includes axis and allies grenade types
	WS_MORTAR,         ///< 12
	WS_MORTAR2,        ///< 13
	WS_DYNAMITE,       ///< 14
	WS_AIRSTRIKE,      ///< 15   -- Fieldops smoke grenade attack
	WS_ARTILLERY,      ///< 16   -- Fieldops binocular attack
	WS_SATCHEL,        ///< 17
	WS_GRENADELAUNCHER,///< 18
	WS_LANDMINE,       ///< 19
	WS_MG42,           ///< 20
	WS_BROWNING,       ///< 21
	WS_CARBINE,        ///< 22
	WS_KAR98,          ///< 23
	WS_GARAND,         ///< 24
	WS_K43,            ///< 25
	WS_MP34,           ///< 26
	WS_SYRINGE,        ///< 27

	WS_MAX
} extWeaponStats_t;

/**
 * @struct weap_ws_t
 * @brief
 */
typedef struct
{
	qboolean fHasHeadShots;
	const char *pszCode;
	const char *pszName;
} weap_ws_t;

extern const weap_ws_t aWeaponInfo[WS_MAX];

//---------------------------------------------------------

/**
 * @enum itemType_t
 * @brief gitem_t->type.
 */
typedef enum
{
	IT_BAD = 0,
	IT_WEAPON,                 ///< EFX: rotate + upscale + minlight

	IT_AMMO,                   ///< EFX: rotate
	//IT_ARMOR,                ///< EFX: rotate + minlight, unused
	IT_HEALTH = 4,                 ///< EFX: static external sphere + rotating internal
	//IT_HOLDABLE,             ///< #100 obsolete

	//IT_KEY,                  ///< EFX: rotate + bob, unused
	//IT_TREASURE,             ///< #100 obsolete gold bars, etc. things that can be picked up and counted for a tally at end-level
	IT_TEAM = 8,
} itemType_t;

#define MAX_ITEM_MODELS 3

#ifdef CGAMEDLL

#define MAX_ITEM_ICONS 4

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct
{
	qboolean registered;
	qhandle_t models[MAX_ITEM_MODELS];
	qhandle_t icons[MAX_ITEM_ICONS];
} itemInfo_t;
#endif

/**
 * @struct item_s
 * @typedef item_t
 * @brief
 */
typedef enum item_s
{
	ITEM_NONE,
	ITEM_HEALTH_SMALL,
	ITEM_HEALTH,
	ITEM_HEALTH_LARGE,
	ITEM_HEALTH_CABINET,
	ITEM_HEALTH_TURKEY,
	ITEM_HEALTH_BREADANDMEAT,
	ITEM_HEALTH_WALL,
	ITEM_WEAPON_KNIFE,
	ITEM_WEAPON_KNIFE_KABAR,
	ITEM_WEAPON_LUGER,
	ITEM_WEAPON_AKIMBO_LUGER,
	ITEM_WEAPON_AKIMBO_SILENCED_LUGER,
	ITEM_WEAPON_THOMPSON,
	ITEM_WEAPON_DUMMY_MG42,
	ITEM_WEAPON_STEN,
	ITEM_WEAPON_MP34,
	ITEM_WEAPON_COLT,
	ITEM_WEAPON_AKIMBO_COLT,
	ITEM_WEAPON_AKIMBO_SILENCED_COLT,
	ITEM_WEAPON_MP40,
	ITEM_WEAPON_PANZERFAUST,
	ITEM_WEAPON_BAZOOKA,
	ITEM_WEAPON_GRENADE_LAUNCHER,
	ITEM_WEAPON_GRENADE_PINEAPPLE,
	ITEM_WEAPON_SMOKE_MARKER,
	ITEM_WEAPON_SMOKETRAIL,
	ITEM_WEAPON_MEDIC_HEAL,
	ITEM_WEAPON_DYNAMITE,
	ITEM_WEAPON_FLAMETHROWER,
	ITEM_WEAPON_MAPMORTAR,
	ITEM_WEAPON_PLIERS,
	ITEM_WEAPON_ARTY,
	ITEM_WEAPON_AIRSTRIKE,
	ITEM_WEAPON_MEDIC_SYRINGE,
	ITEM_WEAPON_MEDIC_ADRENALINE,
	ITEM_WEAPON_MAGICAMMO,
	ITEM_WEAPON_MAGICAMMO2,
	ITEM_WEAPON_BINOCULARS,
	ITEM_WEAPON_K43,
	ITEM_WEAPON_K43_SCOPE,
	ITEM_WEAPON_KAR98,
	ITEM_WEAPON_GPG40,
	ITEM_WEAPON_M7,
	ITEM_WEAPON_CARBINE,
	ITEM_WEAPON_GARAND,
	ITEM_WEAPON_GARAND_SCOPE,
	ITEM_WEAPON_FG42,
	ITEM_WEAPON_FG42_SCOPE,
	ITEM_WEAPON_MORTAR,
	ITEM_WEAPON_MORTAR_SET,
	ITEM_WEAPON_MORTAR2,
	ITEM_WEAPON_MORTAR2_SET,
	ITEM_WEAPON_LANDMINE,
	ITEM_WEAPON_SATCHEL,
	ITEM_WEAPON_SATCHELDET,
	ITEM_WEAPON_SMOKE_BOMB,
	ITEM_WEAPON_MOBILE_MG42,
	ITEM_WEAPON_MOBILE_MG42_SET,
	ITEM_WEAPON_MOBILE_BROWNING_SET,
	ITEM_WEAPON_MOBILE_BROWNING,
	ITEM_WEAPON_SILENCER,
	ITEM_WEAPON_SILENCED_COLT,
	ITEM_AMMO_SYRINGE,
	ITEM_AMMO_SMOKE_GRENADE,
	ITEM_AMMO_DYNAMITE,
	ITEM_AMMO_DISGUISE,
	ITEM_AMMO_AIRSTRIKE,
	ITEM_AMMO_LANDMINE,
	ITEM_AMMO_SATCHEL_CHARGE,
	ITEM_AMMO_9MM_SMALL,
	ITEM_AMMO_9MM,
	ITEM_AMMO_9MM_LARGE,
	ITEM_AMMO_45CAL_SMALL,
	ITEM_AMMO_45CAL,
	ITEM_AMMO_45CAL_LARGE,
	ITEM_AMMO_30CAL_SMALL,
	ITEM_AMMO_30CAL,
	ITEM_AMMO_30CAL_LARGE,
	ITEM_RED_FLAG,
	ITEM_BLUE_FLAG,
	ITEM_MAX_ITEMS,

} item_t;

/**
 * @struct gitem_s
 * @typedef gitem_t
 * @brief
 */
typedef struct gitem_s
{
	item_t id;                  ///< g - identifier
	const char *classname;      ///< g cg bg - spawning name
	const char *pickup_sound;   ///< cg -
	const char *world_model[MAX_ITEM_MODELS];   ///< g cg -

	const char *icon;           ///< cg
	const char *ammoicon;       ///< unused
	const char *pickup_name;    ///< bg cg - for printing on pickup

	int quantity;               ///< g - for ammo how much, or duration of powerup (value not necessary for ammo/health.  that value set in gameskillnumber[] below)
	itemType_t giType;          ///< g cg bg - IT_* flags

	weapon_t giWeapon;          ///< g cg bg -
	powerup_t giPowerUp;        ///< g cg bg -

#ifdef CGAMEDLL
	itemInfo_t itemInfo;        ///< cg -
#endif

} gitem_t;

// included in both the game dll and the client
extern gitem_t bg_itemlist[];

gitem_t *BG_FindItem(const char *pickupName);
gitem_t *BG_FindItemForClassName(const char *className);
gitem_t *BG_GetItem(int index);

qboolean BG_AkimboFireSequence(int weapon, int akimboClip, int mainClip);

qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, int *skill, team_t teamNum);

// content masks
#define MASK_ALL                (-1)
#define MASK_SOLID              (CONTENTS_SOLID)
#define MASK_PLAYERSOLID        (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY)
//#define MASK_DEADSOLID          (CONTENTS_SOLID | CONTENTS_PLAYERCLIP)///< unused
#define MASK_WATER              (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME)
//#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_OPAQUE             (CONTENTS_SOLID | CONTENTS_LAVA)       ///< modified since slime is no longer deadly
#define MASK_SHOT               (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE)
#define MASK_MISSILESHOT        (MASK_SHOT | CONTENTS_MISSILECLIP)

// entityState_t->eType

/**
 * @enum hintType_t
 * @brief cursorhints (stored in ent->s.dmgFlags since that's only used for players at the moment)
 */
typedef enum
{
	HINT_NONE = 0,              ///< reserved
	HINT_FORCENONE,             ///< reserved
	// HINT_PLAYER,             ///< unused
	HINT_ACTIVATE = 3,
	HINT_DOOR,
	HINT_DOOR_ROTATING,         ///< 5
	HINT_DOOR_LOCKED,
	HINT_DOOR_ROTATING_LOCKED,
	HINT_MG42,
	HINT_BREAKABLE,
	HINT_BREAKABLE_DYNAMITE,    ///< 10
	HINT_CHAIR,
	// HINT_ALARM,              ///< unused
	HINT_HEALTH = 13,
	// HINT_TREASURE,
	HINT_KNIFE = 15,            ///< 15
	HINT_LADDER,
	HINT_BUTTON,
	HINT_WATER,
	//HINT_CAUTION,             ///< unused
	//HINT_DANGER,				///< 20 unused
	//HINT_SECRET,              ///< unused
	//HINT_QUESTION,            ///< unused
	//HINT_EXCLAMATION,         ///< unused
	//HINT_CLIPBOARD,           ///< unused
	HINT_WEAPON = 25,           ///< 25
	HINT_AMMO,
	//HINT_ARMOR,				///< unused
	HINT_POWERUP = 28,
	HINT_HOLDABLE,
	//HINT_INVENTORY,           ///< unused
	//HINT_SCENARIC,            ///< 30 unused
	//HINT_EXIT,                ///< unused
	//HINT_NOEXIT,              ///< unused
	//HINT_PLYR_FRIEND,         ///< unused
	//HINT_PLYR_NEUTRAL,        ///< 35 unused
	//HINT_PLYR_ENEMY,          ///< unused
	//HINT_PLYR_UNKNOWN,        ///< unused
	HINT_BUILD = 38,
	HINT_DISARM,
	HINT_REVIVE,                ///< 40
	HINT_DYNAMITE,
	HINT_CONSTRUCTIBLE,
	HINT_UNIFORM,
	HINT_LANDMINE,
	HINT_TANK,                  ///< 45
	HINT_SATCHELCHARGE,
	//HINT_LOCKPICK,            ///< unused
	HINT_RESTRICTED = 48,       ///< invisible user with no target

	HINT_BAD_USER,              ///< invisible user with no target

	HINT_NUM_HINTS = 50,
} hintType_t;

void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splinePath);
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splineData);
void BG_GetMarkDir(const vec3_t dir, const vec3_t normal, vec3_t out);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);

void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, int time, qboolean snap);

qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime);
qboolean BG_PlayerSeesItem(playerState_t *ps, entityState_t *item, int atTime);
qboolean BG_AddMagicAmmo(playerState_t *ps, int *skill, team_t teamNum, int numOfClips);

int PM_IdleAnimForWeapon(int weapon);

#define OVERCLIP        1.001f

void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);

#define MAX_ARENAS          64
#define MAX_ARENAS_TEXT     8192

#define MAX_CAMPAIGNS_TEXT  8192

/**
 * @enum footstep_t
 * @brief
 */
typedef enum
{
	FOOTSTEP_NORMAL = 0,
	FOOTSTEP_METAL,
	FOOTSTEP_WOOD,
	FOOTSTEP_GRASS,
	FOOTSTEP_GRAVEL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_ROOF,
	FOOTSTEP_SNOW,
	FOOTSTEP_CARPET,

	FOOTSTEP_TOTAL
} footstep_t;

/**
 * @enum brassSound_t
 * @brief
 */
typedef enum
{
	BRASSSOUND_METAL = 0,
	BRASSSOUND_SOFT,
	BRASSSOUND_STONE,
	BRASSSOUND_WOOD,
	BRASSSOUND_MAX,
} brassSound_t;

/**
 * @enum fxType_t
 * @brief
 */
typedef enum
{
	FXTYPE_WOOD = 0,
	FXTYPE_GLASS,
	FXTYPE_METAL,
	FXTYPE_GIBS,
	FXTYPE_BRICK,
	FXTYPE_STONE,
	FXTYPE_FABRIC,
	FXTYPE_MAX
} fxType_t;

//==================================================================
// New Animation Scripting Defines

#define MAX_ANIMSCRIPT_MODELS               32
#define MAX_ANIMSCRIPT_ITEMS_PER_MODEL      2048
#define MAX_MODEL_ANIMATIONS                512    ///< animations per model
#define MAX_ANIMSCRIPT_ANIMCOMMANDS         8
#define MAX_ANIMSCRIPT_ITEMS                128
// NOTE: these must all be in sync with string tables in bg_animation.c

/**
 * @enum scriptAnimMoveTypes_t
 * @brief
 */
typedef enum
{
	ANIM_MT_UNUSED = 0,
	ANIM_MT_IDLE,
	ANIM_MT_IDLECR,
	ANIM_MT_WALK,
	ANIM_MT_WALKBK,
	ANIM_MT_WALKCR,
	ANIM_MT_WALKCRBK,
	ANIM_MT_RUN,
	ANIM_MT_RUNBK,
	ANIM_MT_SWIM,
	ANIM_MT_SWIMBK,        ///< 10
	ANIM_MT_STRAFERIGHT,
	ANIM_MT_STRAFELEFT,
	ANIM_MT_TURNRIGHT,
	ANIM_MT_TURNLEFT,
	ANIM_MT_CLIMBUP,
	ANIM_MT_CLIMBDOWN,
	ANIM_MT_FALLEN,        ///< dead, before limbo
	ANIM_MT_PRONE,
	ANIM_MT_PRONEBK,
	ANIM_MT_IDLEPRONE,     ///< 20
	ANIM_MT_FLAILING,
	//ANIM_MT_TALK,
	//ANIM_MT_SNEAK,
	//ANIM_MT_AFTERBATTLE,  ///< just finished battle

	ANIM_MT_RADIO,
	ANIM_MT_RADIOCR,
	ANIM_MT_RADIOPRONE,

	ANIM_MT_DEAD,

	NUM_ANIM_MOVETYPES
} scriptAnimMoveTypes_t;

/**
 * @enum scriptAnimEventTypes_t
 * @brief
 */
typedef enum
{
	ANIM_ET_PAIN = 0,
	ANIM_ET_DEATH,
	ANIM_ET_FIREWEAPON,
	ANIM_ET_FIREWEAPON2,
	ANIM_ET_JUMP,
	ANIM_ET_JUMPBK,
	ANIM_ET_LAND,      ///< used, but not defined in script
	ANIM_ET_DROPWEAPON,///< used, but not defined in script
	ANIM_ET_RAISEWEAPON,
	ANIM_ET_CLIMB_MOUNT,
	ANIM_ET_CLIMB_DISMOUNT,
	ANIM_ET_RELOAD,
	ANIM_ET_REVIVE,
	ANIM_ET_DO_ALT_WEAPON_MODE,
	ANIM_ET_UNDO_ALT_WEAPON_MODE,
	ANIM_ET_DO_ALT_WEAPON_MODE_PRONE,
	ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE,
	ANIM_ET_FIREWEAPONPRONE,
	ANIM_ET_FIREWEAPON2PRONE,
	ANIM_ET_RAISEWEAPONPRONE,
	ANIM_ET_RELOADPRONE,
	ANIM_ET_NOPOWER,

	NUM_ANIM_EVENTTYPES
} scriptAnimEventTypes_t;

/**
 * @enum animBodyPart_t
 * @brief
 */
typedef enum
{
	ANIM_BP_UNUSED = 0,
	ANIM_BP_LEGS,
	ANIM_BP_TORSO,
	ANIM_BP_BOTH,

	NUM_ANIM_BODYPARTS
} animBodyPart_t;

/**
 * @enum scriptAnimConditions_t
 * @brief
 */
typedef enum
{
	ANIM_COND_WEAPON = 0,
	ANIM_COND_ENEMY_POSITION,
	ANIM_COND_ENEMY_WEAPON,
	ANIM_COND_UNDERWATER,
	ANIM_COND_MOUNTED,
	ANIM_COND_MOVETYPE,
	ANIM_COND_UNDERHAND,
	ANIM_COND_LEANING,
	ANIM_COND_IMPACT_POINT,
	ANIM_COND_CROUCHING,
	ANIM_COND_STUNNED,
	ANIM_COND_FIRING,
	ANIM_COND_SHORT_REACTION,
	ANIM_COND_ENEMY_TEAM,
	ANIM_COND_PARACHUTE,
	ANIM_COND_CHARGING,
	ANIM_COND_SECONDLIFE,
	ANIM_COND_HEALTH_LEVEL,
	ANIM_COND_FLAILING_TYPE,
	ANIM_COND_GEN_BITFLAG,     ///< general bit flags (to save some space)
	ANIM_COND_AISTATE,         ///< our current ai state (sometimes more convenient than creating a separate section)
	ANIM_COND_SUICIDE,

	NUM_ANIM_CONDITIONS
} scriptAnimConditions_t;

//-------------------------------------------------------------------

/**
 * @struct animStringItem_t
 * @brief
 */
typedef struct
{
	const char *string;
	int hash;
} animStringItem_t;

/**
 * @struct animScriptCondition_t
 * @brief
 */
typedef struct
{
	int index;          ///< reference into the table of possible conditionals
	int value[2];       ///< can store anything from weapon bits, to position enums, etc
	qboolean negative;  ///< (,)NOT \<condition\>
} animScriptCondition_t;

/**
 * @struct animScriptCommand_t
 * @brief
 */
typedef struct
{
	short int bodyPart[2];     ///< play this animation on legs/torso/both
	short int animIndex[2];    ///< animation index in our list of animations
	short int animDuration[2];
	short int soundIndex;
} animScriptCommand_t;

/**
 * @struct animScriptItem_t
 * @brief
 */
typedef struct
{
	int numConditions;
	animScriptCondition_t conditions[NUM_ANIM_CONDITIONS];
	int numCommands;
	animScriptCommand_t commands[MAX_ANIMSCRIPT_ANIMCOMMANDS];
} animScriptItem_t;

/**
 * @struct animScript_t
 * @brief
 */
typedef struct
{
	int numItems;
	animScriptItem_t *items[MAX_ANIMSCRIPT_ITEMS];     ///< pointers into a global list of items
} animScript_t;

/**
 * @struct animModelInfo_t
 * @brief
 */
typedef struct
{
	char animationGroup[MAX_QPATH];
	char animationScript[MAX_QPATH];

	// parsed from the start of the cfg file (this is basically obsolete now - need to get rid of it)
	gender_t gender;
	footstep_t footsteps;
	vec3_t headOffset;
	int version;
	qboolean isSkeletal;

	// parsed from animgroup file
	animation_t *animations[MAX_MODEL_ANIMATIONS];                  ///< anim names, frame ranges, etc
	//headAnimation_t headAnims[MAX_HEAD_ANIMS];                    ///< unused
	int numAnimations; /*, numHeadAnims;*/

	// parsed from script file
	animScript_t scriptAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES];     ///< locomotive anims, etc
	animScript_t scriptCannedAnims[NUM_ANIM_MOVETYPES];             ///< played randomly
	animScript_t scriptEvents[NUM_ANIM_EVENTTYPES];                 ///< events that trigger special anims

	// global list of script items for this model
	animScriptItem_t scriptItems[MAX_ANIMSCRIPT_ITEMS_PER_MODEL];
	int numScriptItems;

} animModelInfo_t;

/**
 * @struct animScriptData_t
 * @brief This is the main structure that is duplicated on the client and server
 */
typedef struct
{
	animModelInfo_t modelInfo[MAX_ANIMSCRIPT_MODELS];
	int clientConditions[MAX_CLIENTS][NUM_ANIM_CONDITIONS][2];

	// pointers to functions from the owning module
	// constify the arg
	int (*soundIndex)(const char *name);
	void (*playSound)(int soundIndex, vec3_t org, int clientNum);
} animScriptData_t;

//------------------------------------------------------------------
// Conditional Constants

/**
 * @enum animScriptPosition_t
 * @brief
 */
typedef enum
{
	POSITION_UNUSED = 0,
	POSITION_BEHIND,
	POSITION_INFRONT,
	POSITION_RIGHT,
	POSITION_LEFT,

	NUM_ANIM_COND_POSITIONS
} animScriptPosition_t;

/**
 * @enum animScriptMounted_t
 * @brief
 */
typedef enum
{
	MOUNTED_UNUSED = 0,
	MOUNTED_MG42,
	MOUNTED_AAGUN,

	NUM_ANIM_COND_MOUNTED
} animScriptMounted_t;

/**
 * @enum animScriptLeaning_t
 * @brief
 */
typedef enum
{
	LEANING_UNUSED = 0,
	LEANING_RIGHT,
	LEANING_LEFT,

	NUM_ANIM_COND_LEANING
} animScriptLeaning_t;

/**
 * @enum animScriptImpactPoint_t
 * @brief
 */
typedef enum
{
	IMPACTPOINT_UNUSED = 0,
	IMPACTPOINT_HEAD,
	IMPACTPOINT_CHEST,
	IMPACTPOINT_GUT,
	IMPACTPOINT_GROIN,
	IMPACTPOINT_SHOULDER_RIGHT,
	IMPACTPOINT_SHOULDER_LEFT,
	IMPACTPOINT_KNEE_RIGHT,
	IMPACTPOINT_KNEE_LEFT,

	NUM_ANIM_COND_IMPACTPOINT
} animScriptImpactPoint_t;

/**
 * @enum animScriptFlailingType_t
 * @brief
 */
typedef enum
{
	FLAILING_UNUSED = 0,
	FLAILING_INAIR,
	FLAILING_VCRASH,
	FLAILING_HCRASH,

	NUM_ANIM_COND_FLAILING
} animScriptFlailingType_t;

/**
 * @enum animScriptGenBitFlag_t
 * @brief
 */
typedef enum
{
	//ANIM_BITFLAG_SNEAKING,
	//ANIM_BITFLAG_AFTERBATTLE,
	ANIM_BITFLAG_ZOOMING = 0,
	ANIM_BITFLAG_HOLDING = 1,

	NUM_ANIM_COND_BITFLAG
} animScriptGenBitFlag_t;

/**
 * @enum accType_t
 * @brief
 */
typedef enum
{
	ACC_BELT_LEFT = 0,///< belt left (lower)
	ACC_BELT_RIGHT,   ///< belt right (lower)
	ACC_BELT,         ///< belt (upper)
	ACC_BACK,         ///< back (upper)
	ACC_WEAPON,       ///< weapon (upper)
	ACC_WEAPON2,      ///< weapon2 (upper)
	ACC_HAT,          ///< hat (head)
	ACC_MOUTH2,       ///<
	ACC_MOUTH3,       ///<
	ACC_RANK,         ///<
	ACC_MAX           ///< this is bound by network limits, must change network stream to increase this
} accType_t;

#define MAX_GIB_MODELS      16

#define MAX_WEAPS_PER_CLASS 8   ///< was 10


/**
 * @struct bg_weaponclass_t
 * @brief store weapon for class specificity
 */
typedef struct
{
	weapon_t weapon;    ///< weapon
	skillType_t skill;  ///< skill related
	int minSkillLevel;  ///< minimum skill level needed to handle it
	int startingAmmo;   ///< default starting ammo (reserve)
	int startingClip;   ///< default starting in clip

} bg_weaponclass_t;

/**
 * @struct bg_playerclass_t
 * @brief
 */
typedef struct
{
	int classNum;
	const char *characterFile;
	const char *iconName;
	const char *iconArrow;

	bg_weaponclass_t classKnifeWeapon;
	bg_weaponclass_t classPrimaryWeapons[MAX_WEAPS_PER_CLASS];
	bg_weaponclass_t classSecondaryWeapons[MAX_WEAPS_PER_CLASS];
	bg_weaponclass_t classGrenadeWeapon;
	bg_weaponclass_t classMiscWeapons[MAX_WEAPS_PER_CLASS];

	qhandle_t icon;
	qhandle_t arrow;

} bg_playerclass_t;

/**
 * @struct bg_character_s
 * @typedef bg_character_t
 * @brief
 */
typedef struct bg_character_s
{
	char characterFile[MAX_QPATH];

#ifdef USE_MDXFILE
	qhandle_t mesh;
	qhandle_t skin;

	qhandle_t headModel;
	qhandle_t headSkin;

	qhandle_t accModels[ACC_MAX];
	qhandle_t accSkins[ACC_MAX];

	qhandle_t gibModels[MAX_GIB_MODELS];

	qhandle_t undressedCorpseModel;
	qhandle_t undressedCorpseSkin;

	qhandle_t hudhead;
	qhandle_t hudheadskin;
	animation_t hudheadanimations[MAX_HD_ANIMATIONS];
#endif///< CGAMEDLL

	animModelInfo_t *animModelInfo;
} bg_character_t;

//------------------------------------------------------------------
// Global Function Decs

void BG_InitWeaponStrings(void);
void BG_AnimParseAnimScript(animModelInfo_t *animModelInfo, animScriptData_t *scriptData, const char *filename, char *input);
int BG_AnimScriptAnimation(playerState_t *ps, animModelInfo_t *animModelInfo, scriptAnimMoveTypes_t movetype, qboolean isContinue);
int BG_AnimScriptCannedAnimation(playerState_t *ps, animModelInfo_t *modelInfo);
int BG_AnimScriptEvent(playerState_t *ps, animModelInfo_t *animModelInfo, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force);
int BG_IndexForString(char *token, animStringItem_t *strings, qboolean allowFail);
int BG_PlayAnimName(playerState_t *ps, animModelInfo_t *animModelInfo, char *animName, animBodyPart_t bodyPart, qboolean setTimer, qboolean isContinue, qboolean force);
void BG_ClearAnimTimer(playerState_t *ps, animBodyPart_t bodyPart);

char *BG_GetAnimString(animModelInfo_t *animModelInfo, int anim);
void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
int BG_GetConditionValue(int client, int condition, qboolean checkConversion);
qboolean BG_GetConditionBitFlag(int client, int condition, int bitNumber);
void BG_SetConditionBitFlag(int client, int condition, int bitNumber);
void BG_ClearConditionBitFlag(int client, int condition, int bitNumber);
int BG_GetAnimScriptAnimation(int client, animModelInfo_t *animModelInfo, aistateEnum_t aistate, scriptAnimMoveTypes_t movetype);
void BG_AnimUpdatePlayerStateConditions(pmove_t *pmove);
animation_t *BG_AnimationForString(char *string, animModelInfo_t *animModelInfo);
animation_t *BG_GetAnimationForIndex(animModelInfo_t *animModelInfo, int index);
int BG_GetAnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event);
void PM_ContinueWeaponAnim(int anim);

extern animStringItem_t animStateStr[];
extern animStringItem_t animBodyPartsStr[];

bg_playerclass_t *BG_GetPlayerClassInfo(int team, int cls);
bg_playerclass_t *BG_PlayerClassForPlayerState(playerState_t *ps);
qboolean BG_ClassHasWeapon(bg_playerclass_t *classInfo, weapon_t weap);
qboolean BG_WeaponIsPrimaryForClassAndTeam(int classnum, team_t team, weapon_t weapon);
int BG_ClassWeaponCount(bg_playerclass_t *classInfo, team_t team);
const char *BG_ShortClassnameForNumber(int classNum);
const char *BG_ClassnameForNumber(int classNum);
const char *BG_ClassnameForNumberFilename(int classNum);
const char *BG_ClassLetterForNumber(int classNum);
const char *BG_TeamnameForNumber(team_t teamNum);

extern bg_playerclass_t bg_playerClasses[2][NUM_PLAYER_CLASSES];
#define GetPlayerClassesData(team, classe) ((bg_playerclass_t *)(&bg_playerClasses[(team) == TEAM_AXIS ? 0 : 1][classe]))

#define MAX_PATH_CORNERS        512

/**
 * @struct pathCorner_t
 * @brief
 */
typedef struct
{
	char name[64];
	vec3_t origin;
} pathCorner_t;

extern int          numPathCorners;
extern pathCorner_t pathCorners[MAX_PATH_CORNERS];

#define NUM_EXPERIENCE_LEVELS 11

/**
 * @enum mapEntityType_t
 * @brief
 */
typedef enum
{
	ME_PLAYER = 0,
	ME_PLAYER_REVIVE,
	ME_PLAYER_DISGUISED,
	ME_PLAYER_OBJECTIVE,
	ME_CONSTRUCT,
	ME_DESTRUCT,
	ME_DESTRUCT_2,
	ME_LANDMINE,
	ME_TANK,
	ME_TANK_DEAD,
	ME_COMMANDMAP_MARKER,
} mapEntityType_t;

/**
 * @struct ranktable_s
 * @typedef ranktable_t
 * @brief
 */
typedef struct ranktable_s
{
	const char *names;
	const char *miniNames;
	const char *soundNames;

} ranktable_t;

#define MAX_SPLINE_PATHS        512
#define MAX_SPLINE_CONTROLS     4
#define MAX_SPLINE_SEGMENTS     16

typedef struct splinePath_s splinePath_t;

/**
 * @struct splineSegment_t
 * @brief
 */
typedef struct
{
	vec3_t start;
	vec3_t v_norm;
	float length;
} splineSegment_t;

/**
 * @struct splinePath_s
 * @brief
 */
struct splinePath_s
{
	pathCorner_t point;

	char strTarget[64];

	splinePath_t *next;
	splinePath_t *prev;

	pathCorner_t controls[MAX_SPLINE_CONTROLS];
	int numControls;
	splineSegment_t segments[MAX_SPLINE_SEGMENTS];

	float length;

	qboolean isStart;
	qboolean isEnd;
};

extern int          numSplinePaths;
extern splinePath_t splinePaths[MAX_SPLINE_PATHS];

pathCorner_t *BG_Find_PathCorner(const char *match);
splinePath_t *BG_GetSplineData(int number, qboolean *backwards);
void BG_AddPathCorner(const char *name, vec3_t origin);
splinePath_t *BG_AddSplinePath(const char *name, const char *target, vec3_t origin);
void BG_BuildSplinePaths(void);
splinePath_t *BG_Find_Spline(const char *match);
float BG_SplineLength(splinePath_t *pSpline);
void BG_AddSplineControl(splinePath_t *spline, const char *name);
void BG_LinearPathOrigin2(float radius, splinePath_t **pSpline, float *deltaTime, vec3_t result, qboolean backwards);

int BG_MaxAmmoForWeapon(weapon_t weaponNum, const int *skill, int cls);

void BG_InitLocations(vec2_t world_mins, vec2_t world_maxs);
char *BG_GetLocationString(float xpos, float ypos);

extern const char *bg_fireteamNamesAllies[MAX_FIRETEAMS / 2];
extern const char *bg_fireteamNamesAxis[MAX_FIRETEAMS / 2];

/**
 * @struct fireteamData_t
 * @brief
 */
typedef struct
{
	int ident;
	char joinOrder[MAX_CLIENTS];    ///< order in which clients joined the fire team (server), client uses to store if a client is on this fireteam
	int leader;                     ///< leader = joinOrder[0] on server, stored here on client
	qboolean inuse;
	qboolean priv;
#ifdef CGAMEDLL
	int membersNumber;              ///< store members number client side on parsing CS
#endif
} fireteamData_t;

long BG_StringHashValue(const char *fname);
long BG_StringHashValue_Lwr(const char *fname);

int trap_PC_AddGlobalDefine(char *define);
int trap_PC_LoadSource(const char *filename);
int trap_PC_FreeSource(int handle);
int trap_PC_ReadToken(int handle, pc_token_t *pc_token);
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line);
int trap_PC_UnReadToken(int handle);

void PC_SourceError(int handle, const char *format, ...);
//void PC_SourceWarning(int handle, const char *format, ...); // Unused

#ifdef GAMEDLL
const char *PC_String_Parse(int handle);
const char *PC_Line_Parse(int handle);
#else
const char *String_Alloc(const char *p);
qboolean PC_String_Parse(int handle, const char **out);
#endif
qboolean PC_String_ParseNoAlloc(int handle, char *out, size_t size);
qboolean PC_Int_Parse(int handle, int *i);
qboolean PC_Color_Parse(int handle, vec4_t *c);
qboolean PC_Vec_Parse(int handle, vec3_t *c);
qboolean PC_Float_Parse(int handle, float *f);
qboolean PC_Point_Parse(int handle, vec2_t *c);

/**
 * @enum uiMenuCommand_t
 * @brief
 */
typedef enum
{
	UIMENU_NONE = 0,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_NEED_CD,    ///< Obsolete
	UIMENU_BAD_CD_KEY, ///< Obsolete
	UIMENU_TEAM,
	UIMENU_POSTGAME,
	UIMENU_HELP,

	UIMENU_WM_QUICKMESSAGE,
	UIMENU_WM_QUICKMESSAGEALT,

	UIMENU_WM_FTQUICKMESSAGE,
	UIMENU_WM_FTQUICKMESSAGEALT,

	UIMENU_WM_QUICKSPAWNPOINT,
	UIMENU_WM_QUICKSPAWNPOINTALT,

	UIMENU_WM_TAPOUT,
	UIMENU_WM_TAPOUT_LMS,

	UIMENU_WM_AUTOUPDATE,

	UIMENU_WM_CLASS,
	UIMENU_WM_CLASSALT,

	UIMENU_WM_TEAM,
	UIMENU_WM_TEAMALT,

	// say, team say, etc
	UIMENU_INGAME_MESSAGEMODE,
} uiMenuCommand_t;

void BG_AdjustAAGunMuzzleForBarrel(vec_t *origin, vec_t *forward, vec_t *right, vec_t *up, int barrel);

int BG_ClassTextToClass(const char *token);
skillType_t BG_ClassSkillForClass(int classnum);

int BG_FootstepForSurface(int surfaceFlags);
int BG_SurfaceForFootstep(int surfaceFlags);

void BG_HeadCollisionBoxOffset(vec3_t viewangles, int eFlags, vec3_t headOffset);
void BG_LegsCollisionBoxOffset(vec3_t viewangles, int eFlags, vec3_t legsOffset);

#define MATCH_MINPLAYERS "4"///<"1"	// Minimum # of players needed to start a match

#ifdef FEATURE_MULTIVIEW
// Multiview support
int BG_simpleHintsCollapse(int hint, int val);
int BG_simpleHintsExpand(int hint, int val);
#endif
int BG_simpleWeaponState(int ws);

// Crosshair support
void BG_setCrosshair(char *colString, float *col, float alpha, const char *cvarName);

// Voting
#define VOTING_DISABLED     ((1 << numVotesAvailable) - 1)

/**
 * @struct voteType_t
 * @brief
 */
typedef struct
{
	const char *pszCvar;
	int flag;
} voteType_t;

extern const voteType_t voteToggles[];
extern int              numVotesAvailable;

// Tracemap
#ifdef CGAMEDLL
void CG_GenerateTracemap(void);
#endif///< CGAMEDLL
qboolean BG_LoadTraceMap(char *rawmapname, vec2_t world_mins, vec2_t world_maxs);
float BG_GetSkyHeightAtPoint(vec3_t pos);
float BG_GetSkyGroundHeightAtPoint(vec3_t pos);
float BG_GetGroundHeightAtPoint(vec3_t pos);
int BG_GetTracemapGroundFloor(void);
int BG_GetTracemapGroundCeil(void);
int BG_GetTracemapSkyGroundFloor(void);
int BG_GetTracemapSkyGroundCeil(void);

// bg_animgroup.c

void BG_ClearAnimationPool(void);
qboolean BG_R_RegisterAnimationGroup(const char *filename, animModelInfo_t *animModelInfo);

// bg_character.c

/**
 * @struct bg_characterDef_s
 * @typedef bg_characterDef_t
 * @brief
 */
typedef struct bg_characterDef_s
{
	char mesh[MAX_QPATH];
	char animationGroup[MAX_QPATH];
	char animationScript[MAX_QPATH];
	char skin[MAX_QPATH];
	char undressedCorpseModel[MAX_QPATH];
	char undressedCorpseSkin[MAX_QPATH];
	char hudhead[MAX_QPATH];
	char hudheadanims[MAX_QPATH];
	char hudheadskin[MAX_QPATH];
} bg_characterDef_t;

qboolean BG_ParseCharacterFile(const char *filename, bg_characterDef_t *characterDef);
bg_character_t *BG_GetCharacter(int team, int cls);
bg_character_t *BG_GetCharacterForPlayerstate(playerState_t *ps);
void BG_ClearCharacterPool(void);
bg_character_t *BG_FindFreeCharacter(const char *characterFile);
bg_character_t *BG_FindCharacter(const char *characterFile);

// bg_sscript.c

/**
 * @enum speakerLoopType_t
 * @brief
 */
typedef enum
{
	S_LT_NOT_LOOPED = 0,
	S_LT_LOOPED_ON,
	S_LT_LOOPED_OFF
} speakerLoopType_t;

/**
 * @enum speakerBroadcastType_t
 * @brief
 */
typedef enum
{
	S_BT_LOCAL = 0,
	S_BT_GLOBAL,
	S_BT_NOPVS
} speakerBroadcastType_t;

/**
 * @struct bg_speaker_s
 * @typedef bg_speaker_t
 * @brief
 */
typedef struct bg_speaker_s
{
	char filename[MAX_QPATH];
	qhandle_t noise;
	vec3_t origin;
	char targetname[32];
	long targetnamehash;

	speakerLoopType_t loop;
	speakerBroadcastType_t broadcast;
	int wait;
	int random;
	int volume;
	int range;

	qboolean activated;
	int nextActivateTime;
	int soundTime;
} bg_speaker_t;

void BG_ClearScriptSpeakerPool(void);
int BG_NumScriptSpeakers(void);
int BG_GetIndexForSpeaker(bg_speaker_t *speaker);
bg_speaker_t *BG_GetScriptSpeaker(int index);
qboolean BG_SS_DeleteSpeaker(int index);
qboolean BG_SS_StoreSpeaker(bg_speaker_t *speaker);
qboolean BG_LoadSpeakerScript(const char *filename);

// Lookup table to find weapon table entry
extern weaponTable_t weaponTable[WP_NUM_WEAPONS];
#define GetWeaponTableData(weaponIndex) ((weaponTable_t *)(&weaponTable[weaponIndex]))

// Lookup table to find mod properties
extern modTable_t modTable[MOD_NUM_MODS];
#define GetMODTableData(modIndex) ((modTable_t *)(&modTable[modIndex]))

// Lookup table to find skill properties
extern skilltable_t skillTable[SK_NUM_SKILLS];
#define GetSkillTableData(skillIndex) ((skilltable_t *)(&skillTable[skillIndex]))

// Lookup table to find rank table entry
extern ranktable_t rankTable[2][NUM_EXPERIENCE_LEVELS];
#define GetRankTableData(team, rankIndex) ((ranktable_t *)(&rankTable[team == TEAM_AXIS ? 0 : 1][rankIndex]))

#define MAX_MAP_SIZE 65536

qboolean BG_BBoxCollision(vec3_t min1, vec3_t max1, vec3_t min2, vec3_t max2);

//#define VISIBLE_TRIGGERS

/**
 * @enum popupMessageType_e
 * @typedef popupMessageType_t
 * @brief
 */
typedef enum popupMessageType_e
{
	PM_DYNAMITE = 0,
	PM_CONSTRUCTION,
	PM_MINES,
	PM_DEATH,
	PM_MESSAGE,
	PM_OBJECTIVE,
	PM_DESTRUCTION,
	PM_TEAM,
	PM_AMMOPICKUP,
	PM_HEALTHPICKUP,
	PM_WEAPONPICKUP,
	PM_CONNECT,
	PM_NUM_TYPES
} popupMessageType_t;

/**
 * @enum popupMessageBigType_e
 * @typedef popupMessageBigType_t
 * @brief
 */
typedef enum popupMessageBigType_e
{
	PM_SKILL = 0,
	PM_RANK,
#ifdef FEATURE_PRESTIGE
	PM_PRESTIGE,
#endif
	PM_BIG_NUM_TYPES
} popupMessageBigType_t;

#define HITBOXBIT_HEAD   1024
#define HITBOXBIT_LEGS   2048
#define HITBOXBIT_CLIENT 4096

void PM_TraceLegs(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end, trace_t *bodytrace, vec3_t viewangles, void (tracefunc)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int ignoreent, int tracemask);
void PM_TraceHead(trace_t *trace, vec3_t start, vec3_t end, trace_t *bodytrace, vec3_t viewangles, void (tracefunc)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int ignoreent, int tracemask);
void PM_TraceAllParts(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end);
void PM_TraceAll(trace_t *trace, vec3_t start, vec3_t end);

/**
 * @enum sysMsg_t
 * @brief
 */
typedef enum
{
	SM_NEED_MEDIC,
	SM_NEED_ENGINEER,
	SM_NEED_LT,
	SM_NEED_COVERTOPS,
	SM_LOST_MEN,
	SM_OBJ_CAPTURED,
	SM_OBJ_LOST,
	SM_OBJ_DESTROYED,
	SM_CON_COMPLETED,
	SM_CON_FAILED,
	SM_CON_DESTROYED,
	SM_NUM_SYS_MSGS,
} sysMsg_t;

typedef struct sysMessage_s
{
	char *codeString;
	char *voiceScript;
	char *chatString;
} sysMessage_t;

extern sysMessage_t HQMessages[SM_NUM_SYS_MSGS];

/**
 * @enum gameSounds
 * @brief Store all sounds used in server engine and send them to client in events only as Enums
 */
typedef enum
{
	GAMESOUND_BLANK = 0,
	GAMESOUND_PLAYER_GURP1,        ///< "sound/player/gurp1.wav"                         Player takes damage from drowning
	GAMESOUND_PLAYER_GURP2,        ///< "sound/player/gurp2.wav"
	GAMESOUND_PLAYER_BUBBLE,
	GAMESOUND_WPN_AIRSTRIKE_PLANE, ///< "sound/weapons/airstrike/airstrike_plane.wav"    Used by Airstrike marker after it triggers
	//GAMESOUND_WPN_ARTILLERY_FLY_1, ///< "sound/weapons/artillery/artillery_fly_1.wav"    Used by Artillery before impact // moved in weap file
	//GAMESOUND_WPN_ARTILLERY_FLY_2, ///< "sound/weapons/artillery/artillery_fly_2.wav"                                    // moved in weap file
	//GAMESOUND_WPN_ARTILLERY_FLY_3, ///< "sound/weapons/artillery/artillery_fly_3.wav"                                    // moved in weap file

	GAMESOUND_MISC_REVIVE = 8,     ///< "sound/misc/vo_revive.wav"                       Used by revival Needle
	GAMESOUND_MISC_REFEREE,        ///< "sound/misc/referee.wav"                         Game Referee performs action
	GAMESOUND_MISC_VOTE,           ///< "sound/misc/vote.wav"                            Vote is issued

	//GAMESOUND_MISC_BANNED,       ///< "sound/osp/banned.wav"                           Player is banned
	//GAMESOUND_MISC_KICKED,       ///< "sound/osp/kicked.wav"                           Player is kicked

	GAMESOUND_WORLD_BUILD,         ///< "sound/world/build.wav"
	GAMESOUND_WORLD_CHAIRCREAK,    ///< "sound/world/chaircreak.wav"                     Common code
	GAMESOUND_WORLD_MG_CONSTRUCTED,///< "sound/world/mg_constructed.wav"

	GAMESOUND_MAX
} gameSounds;

// defines for viewlocking (mg/medics etc)
#define VIEWLOCK_NONE               0  ///< disabled, let them look around
#define VIEWLOCK_JITTER             2  ///< this enable screen jitter when firing
#define VIEWLOCK_MG42               3  ///< tell the client to lock the view in the direction of the gun
#define VIEWLOCK_MEDIC              7  ///< look at the nearest medic

// cursor hint & trace distances
#define CH_NONE_DIST        0
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96
#define CH_ACTIVATE_DIST    96
#define CH_REVIVE_DIST      64
#define CH_KNIFE_DIST       48
#define CH_CORPSE_DIST      48
#define CH_DIST             100///<128      ///< use the largest value from above
#define CH_MAX_DIST         1024   ///< use the largest value from above
#define CH_MAX_DIST_ZOOM    8192   ///< max dist for zooming hints

// FLAME & FLAMER constants
#define FLAMETHROWER_RANGE  2500.f   ///< multiplayer range, was 850 in SP

// these define how the flame looks and flamer acts
#define FLAME_START_SIZE        1.0f    ///< bg
#define FLAME_START_MAX_SIZE    140.0f  ///< bg
#define FLAME_MAX_SIZE          200.0f  ///< cg flame sprites cannot be larger than this
#define FLAME_START_SPEED       1200.0f ///< cg speed of flame as it leaves the nozzle
#define FLAME_MIN_SPEED         60.0f   ///< bg 200.0
#define FLAME_CHUNK_DIST        8.0f    ///< cg space in between chunks when fired

#define FLAME_BLUE_LENGTH       130.0f  ///< cg
#define FLAME_BLUE_MAX_ALPHA    1.0f    ///< cg

// these are calculated (don't change)
#define FLAME_LENGTH            (FLAMETHROWER_RANGE + 50.0f)  ///< NOTE: only modify the range, since this should always reflect that range

#define FLAME_LIFETIME          (int)((FLAME_LENGTH / FLAME_START_SPEED) * 1000.0f)       ///< life duration in milliseconds
#define FLAME_FRICTION_PER_SEC  (2.0f * FLAME_START_SPEED)///< bg
#define FLAME_BLUE_LIFE         (int)((FLAME_BLUE_LENGTH / FLAME_START_SPEED) * 1000.0f)///< cg

#define FLAME_BLUE_FADEIN_TIME(x)       (0.2f * x) ///< cg
#define FLAME_BLUE_FADEOUT_TIME(x)      (0.05f * x)///< cg
#define GET_FLAME_BLUE_SIZE_SPEED(x)    (((float)x / FLAME_LIFETIME) / 1.0f)      ///< cg x is the current sizeMax
#define GET_FLAME_SIZE_SPEED(x)         (((float)x / FLAME_LIFETIME) / 0.3f)      ///< cg x is the current sizeMax

/**
 * @enum VOTE_CLIENT_RESPONSE
 * @brief
 */
enum VOTE_CLIENT_RESPONSE
{
	VOTE_CLIENT_NONE = 0,
	VOTE_CLIENT_YES,
	VOTE_CLIENT_NO
};

/**
 * @enum VOTE_TYPE_ENUM
 * @brief
 */
enum VOTE_TYPE_ENUM
{
	VOTE_COMPLAINT = 0,
	VOTE_APPLICATION,
	VOTE_PROPOSITION,
	VOTE_INVITATION,
	VOTE_AUTOFIRETEAM,
	VOTE_GENERIC
};

/**
 * @enum VOTE_FLAGS_ENUM
 * @brief
 */
enum VOTE_FLAGS_ENUM
{
	VOTE_FLAG_GLOBAL          = BIT(0),
	VOTE_FLAG_PERSONAL        = BIT(1),
	VOTE_FLAG_AXIS            = BIT(2),
	VOTE_FLAG_ALLIES          = BIT(3),
	VOTE_FLAG_CREATE_FIRETEAM = BIT(4),
	VOTE_FLAG_JOIN_FIRETEAM   = BIT(5)
};

/**
 * @struct client_vote_s
 * @typedef client_vote_t
 * @brief
 */
typedef struct client_vote_s
{
	int id;
	int type;
	int flags;
	int client_started;
	int start_time;
	int end_time;
	char message[MAX_STRING_TOKENS];
	int clients[MAX_CLIENTS];
} client_vote_t;

// hash values for several

// script events
#define SPAWN_HASH                          66910
#define TRIGGER_HASH                        92198
#define PAIN_HASH                           51093
#define DEATH_HASH                          62701
#define ACTIVATE_HASH                       104037
#define STOPCAM_HASH                        92530
#define PLAYERSTART_HASH                    150212
#define BUILT_HASH                          65851
#define BUILDSTART_HASH                     134191
#define DECAYED_HASH                        87740
#define DESTROYED_HASH                      120424
#define REBIRTH_HASH                        91760
#define FAILED_HASH                         74482
#define DYNAMITED_HASH                      117917
#define DEFUSED_HASH                        89805
#define MG42_HASH                           37723
#define MESSAGE_HASH                        90364
#define EXPLODED_HASH                       104425

// script actions
#define GOTOMARKER_HASH                     133741
#define PLAYSOUND_HASH                      121881
#define PLAYSOUND_ENV_HASH                  176828
#define PLAYANIM_HASH                       105217
#define WAIT_HASH                           52658
//#define TRIGGER_HASH                        92198///< already defined
#define ALERTENTITY_HASH                    149582
#define TOGGLESPEAKER_HASH                  173558
#define DISABLESPEAKER_HASH                 184664
#define ENABLESPEAKER_HASH                  170317
#define ACCUM_HASH                          63083
#define GLOBALACCUM_HASH                    142136
#define PRINT_HASH                          67401
#define FACEANGLES_HASH                     127680
#define RESETSCRIPT_HASH                    149825
#define ATTACHTOTAG_HASH                    145327
#define HALT_HASH                           51236
#define STOPSOUND_HASH                      123794
#define ENTITYSCRIPTNAME_HASH               220877
#define WM_AXIS_RESPAWNTIME_HASH            262921
#define WM_ALLIED_RESPAWNTIME_HASH          288581
#define WM_NUMBER_OF_OBJECTIVES_HASH        317829
#define WM_SETWINNER_HASH                   163577
#define WM_SET_DEFENDING_TEAM_HASH          283627
#define WM_ANNOUNCE_HASH                    146028
#define WM_TEAMVOICEANNOUNCE_HASH           274257
#define WM_ADDTEAMVOICEANNOUNCE_HASH        316227
#define WM_REMOVETEAMVOICEANNOUNCE_HASH     366546
#define WM_ANNOUNCE_ICON_HASH               214704
#define WM_ENDROUND_HASH                    147063
#define WM_SET_ROUND_TIMELIMIT_HASH         306988
#define WM_VOICEANNOUNCE_HASH               216473
#define WM_OBJECTIVE_STATUS_HASH            262477
#define WM_SET_MAIN_OBJECTIVE_HASH          286439
#define REMOVE_HASH                         79455
#define SETSTATE_HASH                       107393
#define FOLLOWSPLINE_HASH                   163074
#define FOLLOWPATH_HASH                     134377
#define ABORTMOVE_HASH                      119996
#define SETSPEED_HASH                       105396
#define SETROTATION_HASH                    150260
#define STOPROTATION_HASH                   165999
#define STARTANIMATION_HASH                 190460
#define ATTATCHTOTRAIN_HASH                 190029
#define FREEZEANIMATION_HASH                201793
#define UNFREEZEANIMATION_HASH              232118
#define REMAPSHADER_HASH                    144301
#define REMAPSHADERFLUSH_HASH               216384
#define CHANGEMODEL_HASH                    141782
#define SETCHARGETIMEFACTOR_HASH            258593
#define SETDAMAGABLE_HASH                   154516
#define REPAIRMG42_HASH                     117730
#define SETHQSTATUS_HASH                    151953
#define PRINTACCUM_HASH                     133089
#define PRINTGLOBALACCUM_HASH               215267
#define CVAR_HASH                           51586
#define ABORTIFWARMUP_HASH                  176528
#define ABORTIFNOTSINGLEPLAYER_HASH         307668
#define SETDEBUGLEVEL_HASH                  173363
#define SETPOSITION_HASH                    150892
#define SETAUTOSPAWN_HASH                   165106
#define SETMODELFROMBRUSHMODEL_HASH         307347
#define FADEALLSOUNDS_HASH                  172841
#define MU_START_HASH                       107698
#define MU_PLAY_HASH                        92607
#define MU_STOP_HASH                        94568
#define MU_QUEUE_HASH                       106558
#define MU_FADE_HASH                        87906
#define CONSTRUCT_HASH                      122676
#define SPAWNRUBBLE_HASH                    147318
#define SETGLOBALFOG_HASH                   158408
#define ALLOWTANKEXIT_HASH                  176962
#define ALLOWTANKENTER_HASH                 190185
#define SETTANKAMMO_HASH                    147275
#define ADDTANKAMMO_HASH                    143077
#define KILL_HASH                           51577
#define DISABLEMESSAGE_HASH                 183871
#define SET_HASH                            39841
#define CONSTRUCTIBLE_CLASS_HASH            260698
#define CONSTRUCTIBLE_CHARGEBARREQ_HASH     362499
#define CONSTRUCTIBLE_CONSTRUCTXPBONUS_HASH 438745
#define CONSTRUCTIBLE_DESTRUCTXPBONUS_HASH  421228
#define CONSTRUCTIBLE_HEALTH_HASH           273962
#define CONSTRUCTIBLE_WEAPONCLASS_HASH      351977
#define CONSTRUCTIBLE_DURATION_HASH         307340
#define CREATE_HASH                         76308
#define DELETE_HASH                         76202

#endif // #ifndef INCLUDE_BG_PUBLIC_H
