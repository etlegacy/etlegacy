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
 * @file g_local.h
 * @brief Local definitions for game module
 */

#ifndef INCLUDE_G_LOCAL_H
#define INCLUDE_G_LOCAL_H

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"

//==================================================================

#define BODY_QUEUE_SIZE     8

#define EVENT_VALID_MSEC    300

#define MG42_MULTIPLAYER_HEALTH 350

#define MISSILE_PRESTEP_TIME    50

/**
 * @def BODY_TIME
 * @brief How long do bodies last? (in ms)
 *
 * @todo cvar it ?
 */
#define BODY_TIME 10000

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

/**
 * @enum moverState_t
 * @brief Movers are things like doors, plats, buttons, etc
 */
typedef enum
{
	MOVER_POS1 = 0,
	MOVER_POS2,
	MOVER_POS3,
	MOVER_1TO2,
	MOVER_2TO1,

	MOVER_2TO3,
	MOVER_3TO2,

	MOVER_POS1ROTATE,
	MOVER_POS2ROTATE,
	MOVER_1TO2ROTATE,
	MOVER_2TO1ROTATE
} moverState_t;

// Worldspawn spawnflags to indicate if a gametype is not supported
#define NO_GT_WOLF      1
#define NO_STOPWATCH    2
#define NO_CHECKPOINT   4 ///< unused
#define NO_LMS          8

#define MAX_CONSTRUCT_STAGES 3

#define ALLOW_AXIS_TEAM         1
#define ALLOW_ALLIED_TEAM       2
#define ALLOW_DISGUISED_CVOPS   4

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

//====================================================================

/**
 * @struct g_script_stack_action_t
 * @brief Scripting (parsed at each start)
 */
typedef struct
{
	char *actionString;
	qboolean (*actionFunc)(gentity_t *ent, char *params);
	int hash;
} g_script_stack_action_t;

/**
 * @struct g_script_stack_item_t
 * @brief
 */
typedef struct
{
	// set during script parsing
	g_script_stack_action_t *action;                ///< points to an action to perform
	char *params;
} g_script_stack_item_t;

/// value set high for the tank
#define G_MAX_SCRIPT_STACK_ITEMS    196

/**
 * @struct g_script_stack_t
 * @brief
 */
typedef struct
{
	g_script_stack_item_t items[G_MAX_SCRIPT_STACK_ITEMS];
	int numItems;
} g_script_stack_t;

/**
 * @struct g_script_stack_t
 * @brief
 */
typedef struct
{
	int eventNum;                           ///< index in scriptEvents[]
	char *params;                           ///< trigger targetname, etc
	g_script_stack_t stack;
} g_script_event_t;

/**
 * @struct g_script_event_define_t
 * @brief
 */
typedef struct
{
	char *eventStr;
	qboolean (*eventMatch)(g_script_event_t *event, const char *eventParm);
	int hash;
} g_script_event_define_t;

// Script Flags
#define SCFL_GOING_TO_MARKER    0x1
#define SCFL_ANIMATING          0x2
#define SCFL_FIRST_CALL         0x4

/**
 * @struct g_script_status_t
 * @brief Scripting Status
 * @warning: This MUST NOT contain any pointer vars
 */
typedef struct
{
	int scriptStackHead, scriptStackChangeTime;
	int scriptEventIndex;           ///< current event containing stack of actions to perform
	// scripting system variables
	int scriptId;                   ///< incremented each time the script changes
	int scriptFlags;
	int actionEndTime;              ///< time to end the current action
	char *animatingParams;          ///< read 8 lines up for why i love this code ;)
} g_script_status_t;

#define G_MAX_SCRIPT_ACCUM_BUFFERS 10

void G_Script_ScriptEvent(gentity_t *ent, const char *eventStr, const char *params);
//====================================================================

/**
 * @struct g_constructible_stats_s
 * @typedef g_constructible_stats_t
 * @brief
 */
typedef struct g_constructible_stats_s
{
	float chargebarreq;
	float constructxpbonus;
	float destructxpbonus;
	int health;
	int weaponclass;
	int duration;
} g_constructible_stats_t;

#define NUM_CONSTRUCTIBLE_CLASSES   3

extern g_constructible_stats_t g_constructible_classes[NUM_CONSTRUCTIBLE_CLASSES];

//====================================================================

#define MAX_NETNAME         36

#define MAX_COMMANDER_TEAM_SOUNDS 16

/**
 * @struct commanderTeamChat_s
 * @typedef commanderTeamChat_t
 * @brief
 */
typedef struct commanderTeamChat_s
{
	int index;
} commanderTeamChat_t;

/**
 * @struct mapVoteInfo_t
 * @brief MAPVOTE
 */
typedef struct
{
	char bspName[128];
	int numVotes;
	int timesPlayed;
	int lastPlayed;
	int totalVotes;
	int voteEligible;
	int zOrder;
} mapVoteInfo_t;

/**
 * @struct glerpFrame_t
 * @brief realistic hitboxes based on lerpFrame_t
 */
typedef struct
{
	qhandle_t oldFrameModel;
	qhandle_t frameModel;

	int oldFrame;
	int oldFrameTime;               ///< time when ->oldFrame was exactly on
	int oldFrameSnapshotTime;

	vec3_t oldFramePos;

	int frame;
	int frameTime;                  ///< time when ->frame will be exactly on

	float yawAngle;
	int yawing;
	float pitchAngle;
	int pitching;

	int moveSpeed;

	int animationNumber;            ///< may include ANIM_TOGGLEBIT
	int oldAnimationNumber;         ///< may include ANIM_TOGGLEBIT
	animation_t *animation;
	int animationTime;              ///< time when the first frame of the animation will be exact
	float animSpeedScale;

} glerpFrame_t;

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

	struct gclient_s *client;       ///< NULL if not a client

	qboolean inuse;

	vec3_t instantVelocity;         ///< per entity instantaneous velocity, set per frame

	const char *classname;          ///< set in QuakeEd
	int spawnflags;                 ///< set in QuakeEd

	qboolean neverFree;             ///< if true, FreeEntity will only unlink
	///< bodyque uses this

	int flags;                      ///< FL_* variables

	char *model;
	char *model2;
	int freetime;                   ///< level.time when the object was freed

	int eventTime;                  ///< events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;

	qboolean physicsObject;         ///< if true, it can be pushed by movers and fall off edges
	///< all game items are physicsObjects,
	float physicsBounce;            ///< 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;                   ///< brushes with this content value will be collided against
	///< when moving.  items and corpses do not collide against
	///< players, for instance

	int realClipmask;               ///< use these to backup the contents value when we go to state under construction
	int realContents;
	qboolean realNonSolidBModel;    ///< For script_movers with spawnflags 2 set

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	int sound2to3;
	int sound3to2;
	int soundPos3;

	int soundSoftopen;
	int soundSoftendo;
	int soundSoftclose;
	int soundSoftendc;

	gentity_t *parent;
	gentity_t *nextTrain;
	gentity_t *prevTrain;
	vec3_t pos1, pos2, pos3;

	char *message;

	int timestamp;                  ///< body queue sinking, etc

	float angle;                    ///< set in editor, -1 = up, -2 = down
	char *target;

	char *targetname;
	int targetnamehash;             ///< adding a hash for this for faster lookups

	char *team;
	gentity_t *target_ent;

	float speed;
	float closespeed;               ///< for movers that close at a different speed than they open
	vec3_t movedir;

	int gDuration;
	int gDurationBack;
	vec3_t gDelta;
	vec3_t gDeltaBack;

	int nextthink;
	void (*free)(gentity_t *self);
	void (*think)(gentity_t *self);
	void (*reached)(gentity_t *self);           ///< movers call this when hitting endpoint
	void (*blocked)(gentity_t *self, gentity_t *other);
	void (*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void (*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void (*pain)(gentity_t *self, gentity_t *attacker, int damage, vec3_t point);
	void (*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod);

	int pain_debounce_time;
	int fly_sound_debounce_time;    ///< wind tunnel

	int health;

	qboolean takedamage;

	int damage;
	int splashDamage;               ///< quad will increase this without increasing radius
	int splashRadius;
	meansOfDeath_t methodOfDeath;
	meansOfDeath_t splashMethodOfDeath;

	int count;

	gentity_t *chain;
	gentity_t *enemy;
	gentity_t *activator;
	gentity_t *teamchain;           ///< next entity in team
	gentity_t *teammaster;          ///< master of the team

	meansOfDeath_t deathType;

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	// sniper variable
	// sniper uses delay, random, radius
	int radius;
	float delay;

	int TargetFlag;
	float duration;
	vec3_t rotate;
	vec3_t TargetAngles;
	gitem_t *item;                  ///< for bonus items

	// Ridah, AI fields
	char *aiName;
	void (*AIScript_AlertEntity)(gentity_t *ent);

	char *aiSkin;

	vec3_t dl_color;
	char *dl_stylestring;
	char *dl_shader;
	int dl_atten;

	int key;                        ///< used by:  target_speaker->nopvs,

	qboolean active;

	// mg42
	float harc;
	float varc;

	int props_frame_state;

	int missionLevel;               ///< mission we are currently trying to complete
	// gets reset each new level
	int start_size;
	int end_size;

	qboolean isProp;                ///< teamkilled or not

	int mg42BaseEnt;

	char *spawnitem;

	int flameQuota, flameQuotaTime, flameBurnEnt;

	int count2;

	int grenadeExplodeTime;         ///< we've caught a grenade, which was due to explode at this time
	int grenadeFired;               ///< the grenade entity we last fired

	char *track;

	// entity scripting system
	char *scriptName;

	int numScriptEvents;
	g_script_event_t *scriptEvents;     ///< contains a list of actions to perform for each event type
	g_script_status_t scriptStatus;     ///< current status of scripting
	// the accumulation buffer
	int scriptAccumBuffer[G_MAX_SCRIPT_ACCUM_BUFFERS];

	float accuracy;

	char tagName[MAX_QPATH];            ///< name of the tag we are attached to
	gentity_t *tagParent;
	gentity_t *tankLink;

	int lastHintCheckTime;
	int voiceChatSquelch;
	int voiceChatPreviousTime;
	int lastBurnedFrameNumber;          ///< to fix FT instant-kill exploit

	entState_t entstate;
	char *constages;
	char *desstages;
	char *damageparent;
	int conbmodels[MAX_CONSTRUCT_STAGES + 1];
	int desbmodels[MAX_CONSTRUCT_STAGES];
	int partofstage;

	int allowteams;

	int spawnTime;

	gentity_t *dmgparent;
	qboolean dmginloop;

	int tagNumber;                      ///< "handle" to a tag header

	int linkTagTime;

	splinePath_t *backspline;
	vec3_t backorigin;
	float backdelta;
	qboolean back;
	qboolean moving;

	/// What sort of surface are we standing on?
	int surfaceFlags;

	char tagBuffer[32];

	// FIXME: bleh - ugly
	int backupWeaponTime;
	int mg42weapHeat;

	vec3_t oldOrigin;

	qboolean runthisframe;

	g_constructible_stats_t constructibleStats;

	int etpro_misc_1;                   ///< bit 0 = it's a planted/ticking dynamite
	int etpro_misc_2;                   ///< the entityNumber of the (last) planted dyna. bit strange it's only 1 dyna..

	int numPlayers;

	// realistic hitboxes
	glerpFrame_t legsFrame;
	glerpFrame_t torsoFrame;
	int timeShiftTime;

	// dyno chaining
	gentity_t *onobjective;

#ifdef FEATURE_OMNIBOT
	int numPlanted;                     ///< Omni-bot increment dyno count
#endif
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
 * @enum combatstate_t
 * @brief
 */
typedef enum
{
	COMBATSTATE_COLD = 0,
	COMBATSTATE_DAMAGEDEALT,
	COMBATSTATE_DAMAGERECEIVED,
	COMBATSTATE_KILLEDPLAYER
} combatstate_t;

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
 * @brief Weapon stat counters
 */
typedef struct
{
	unsigned int atts;
	unsigned int deaths;
	unsigned int headshots;
	unsigned int hits;
	unsigned int kills;
} weapon_stat_t;

/**
 * @struct playerTeamStateState_t
 * @brief Client data that stays across multiple levels or tournament restarts
 * this is achieved by writing all the data to vmCvar strings at game shutdown
 * time and reading them back at connection time.  Anything added here
 * MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
 */
typedef struct
{
	team_t sessionTeam;
	int spectatorTime;                                  ///< for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient;                                ///< for chasecam and follow mode
	int playerType;                                     ///< class
	weapon_t playerWeapon;                              ///< primary weapon
	weapon_t playerWeapon2;                             ///< secondary weapon
	int userSpawnPointValue;                            ///< index of objective to spawn nearest to (returned from UI)
	int resolvedSpawnPointIndex;                        ///< most possible objective to spawn nearest to
	int latchPlayerType;                                ///< latched class
	weapon_t latchPlayerWeapon;                         ///< latched primary weapon
	weapon_t latchPlayerWeapon2;                        ///< latched secondary weapon
	int ignoreClients[MAX_CLIENTS / (sizeof(int) * 8)];
	qboolean muted;
	float skillpoints[SK_NUM_SKILLS];                   ///< skillpoints
	float startskillpoints[SK_NUM_SKILLS];              ///< initial skillpoints at map beginning
	float startxptotal;
	int skill[SK_NUM_SKILLS];                           ///< skill
	int rank;                                           ///< rank
	int medals[SK_NUM_SKILLS];                          ///< medals

	int referee;
	int shoutcaster;
	int rounds;
	int spec_invite;
	int spec_team;
	int kills;
	int deaths;
	int gibs;
	int self_kills;
	int team_kills;
	int team_gibs;
	int damage_given;
	int damage_received;
	int team_damage_given;
	int team_damage_received;
	int time_axis;
	int time_allies;
	int time_played;
#ifdef FEATURE_RATING
	// skill rating
	float mu;
	float sigma;
	float oldmu;
	float oldsigma;
#endif
#ifdef FEATURE_PRESTIGE
	int prestige;
#endif

	// MAPVOTE
	int mapVotedFor[3];

	weapon_stat_t aWeaponStats[WS_MAX + 1];             ////< Weapon stats.  +1 to avoid invalid weapon check

	qboolean versionOK;

	unsigned int uci;                                   ////< GeoIP

#ifdef FEATURE_OMNIBOT
	//Omni-bot
	qboolean botSuicide;                                ///< /kill before next spawn
	qboolean botPush;                                   ///< allow for disabling of bot pushing via script
#endif

} clientSession_t;

#define PICKUP_ACTIVATE 0   ///< pickup items only when using "+activate"
#define PICKUP_TOUCH    1   ///< pickup items when touched
#define PICKUP_FORCE    2   ///< pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)

// zinx etpro antiwarp
#define LAG_MAX_COMMANDS 512
#define LAG_MAX_DELTA 75
#define LAG_MAX_DROP_THRESHOLD 800
#define LAG_MIN_DROP_THRESHOLD (LAG_MAX_DROP_THRESHOLD - 200)
#define LAG_DECAY 1.02f

#ifdef FEATURE_MULTIVIEW

#define MULTIVIEW_MAXVIEWS  16

/**
 * @struct mview_t
 * @brief Multiview handling
 */
typedef struct
{
	qboolean fActive;
	int entID;
	gentity_t *camera;
} mview_t;
#endif

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

#define MAX_COMPLAINTIPS 5

/**
 * @struct clientPersistant_t
 * @brief Client data that stays across multiple respawns, but is cleared
 * on each level change or team change at ClientBegin()
 */
typedef struct
{
	clientConnected_t connected;
	usercmd_t cmd;                      ///< we would lose angles if not persistant
	usercmd_t oldcmd;                   ///< previous command processed by pmove()
	qboolean localClient;               ///< true if "ip" info key is "localhost"
	qboolean initialSpawn;              ///< the first spawn should be at a cool location
	qboolean predictItemPickup;         ///< based on cg_predictItems userinfo
	qboolean pmoveFixed;                ///<
	int pmoveMsec;                      ///< antiwarp

	char netname[MAX_NETNAME];
	char client_ip[MAX_IP4_LENGTH];     ///< ip 'caching' - it won't change
	char cl_guid[MAX_GUID_LENGTH + 1];

	int autoActivate;                   ///< based on cg_autoactivate userinfo        (uses the PICKUP_ values above)

	int maxHealth;                      ///<
	int enterTime;                      ///< level.time the client entered the game
	int connectTime;                    ///< level.time the client first connected to the server
	playerTeamState_t teamState;        ///< status in teamplay games
	int voteCount;                      ///< to prevent people from constantly calling votes
	int teamVoteCount;                  ///< FIXME: unused - to prevent people from constantly calling votes

	int complaints;                     ///< number of complaints lodged against this client
	int complaintClient;                ///< able to lodge complaint against this client
	int complaintEndTime;               ///< until this time has expired

	int lastReinforceTime;              ///< last reinforcement

	qboolean teamInfo;                  ///< send team overlay updates?

	qboolean bAutoReloadAux;            ///< auxiliary storage for pmoveExt_t::bAutoReload, to achieve persistance

	int applicationClient;              ///< this client has requested to join your fireteam
	int applicationEndTime;             ///< you have X seconds to reply or this message will self destruct!

	int invitationClient;               ///< you have been invited to join this client's fireteam
	int invitationEndTime;              ///< quickly now, chop chop!.....

	int propositionClient;              ///< propositionClient2 has requested you invite this client to join the fireteam
	int propositionClient2;             ///<
	int propositionEndTime;             ///< tick, tick, tick....

	int autofireteamEndTime;
	int autofireteamCreateEndTime;
	int autofireteamJoinEndTime;

	playerStats_t playerStats;

	int lastBattleSenseBonusTime;
	int lastHQMineReportTime;
	int lastCCPulseTime;

	int lastSpawnTime;

	unsigned int autoaction;            ///< End-of-match auto-requests
	unsigned int clientFlags;           ///< Client settings that need server involvement
	unsigned int clientMaxPackets;      ///< Client com_maxpacket settings
	unsigned int clientTimeNudge;       ///< Client cl_timenudge settings
	int cmd_debounce;                   ///< Dampening of command spam
	unsigned int invite;                ///< Invitation to a team to join

#ifdef FEATURE_MULTIVIEW
	mview_t mv[MULTIVIEW_MAXVIEWS];     ///< portals
	int mvCount;                        ///< Number of active portals
	int mvReferenceList;                ///< Reference list used to generate views after a map_restart
	int mvScoreUpdate;                  ///< Period to send score info to MV clients
#endif

	int panzerDropTime;                 ///< Time which a player dropping panzer still "has it" if limiting panzer counts
	int panzerSelectTime;               ///< *when* a client selected a panzer as spawn weapon
	qboolean ready;                     ///< Ready state to begin play

	bg_character_t *character;
	int characterIndex;

	ipFilter_t complaintips[MAX_COMPLAINTIPS];

	int lastkilled_client;
	int lastrevive_client;
	int lastkiller_client;
	int lastammo_client;
	int lasthealth_client;
	int lastteambleed_client;
	int lastteambleed_dmg;

	int savedClassWeaponTimeMed;
	int savedClassWeaponTimeEng;
	int savedClassWeaponTimeFop;
	int savedClassWeaponTimeCvop;
	int savedClassWeaponTime;

} clientPersistant_t;

/**
 * @struct clientMarker_t
 * @brief
 */
typedef struct
{
	vec3_t mins;
	vec3_t maxs;

	vec3_t origin;

	// for BuildHead/Legs
	int eFlags;             ///< s.eFlags to ps.eFlags
	int viewheight;         ///< ps for both
	int pm_flags;           ///< ps for both
	vec3_t viewangles;      ///< s.apos.trBase to ps.viewangles

	int time;

	// torso markers
	qhandle_t torsoOldFrameModel;
	qhandle_t torsoFrameModel;
	int torsoOldFrame;
	int torsoFrame;
	int torsoOldFrameTime;
	int torsoFrameTime;
	float torsoYawAngle;
	float torsoPitchAngle;
	int torsoYawing;
	int torsoPitching;

	// leg markers
	qhandle_t legsOldFrameModel;
	qhandle_t legsFrameModel;
	int legsOldFrame;
	int legsFrame;
	int legsOldFrameTime;
	int legsFrameTime;
	float legsYawAngle;
	float legsPitchAngle;
	int legsYawing;
	qboolean legsPitching;
} clientMarker_t;

#define MAX_CLIENT_MARKERS 17

#define FIELDOPS_SPECIAL_PICKUP_MOD 3   ///< Number of times (minus one for modulo) field ops must drop ammo before scoring a point
#define MEDIC_SPECIAL_PICKUP_MOD    4   ///< Same thing for medic

/**
 * @struct debrisChunk_s
 * @typedef debrisChunk_t
 * @brief Debris test
 */
typedef struct debrisChunk_s
{
	vec3_t origin;
	int model;
	vec3_t velocity;
	char target[32];
	char targetname[32];
} debrisChunk_t;

#define MAX_DEBRISCHUNKS        256

// ===================

/**
 * @struct gclient_s
 * @typedef gclient_t
 * @brief This structure is cleared on each ClientSpawn(),
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

	// we can't just use pers.lastCommand.time, because
	// of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	int wbuttons;
	int oldwbuttons;
	int latched_wbuttons;
	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_blood;                       ///< damage taken out of health
	int damage_knockback;                   ///< impact damage
	vec3_t damage_from;                     ///< origin for vector calculation
	qboolean damage_fromWorld;              ///< if true, don't use the damage_from vector

	//int         accuracy_shots;             ///< total number of shots
	//int         accuracy_hits;              ///< total number of hits

	// ingame status
	int lasthurt_client;                    ///< last client that damaged this client
	int lasthurt_mod;                       ///< type of damage the client did
	int lasthurt_time;                      ///< level.time of last damage

	// timers
	int respawnTime;                        ///< can respawn when time > this, force after g_forcerespwan
	int inactivityTime;                     ///< kick players when time > this
	qboolean inactivityWarning;             ///< qtrue if the five seoond warning has been given
	int inactivitySecondsLeft;              ///< for displaying a counting-down time on clients (milliseconds before activity kicks in..)

	int airOutTime;

	int lastKillTime;                       ///< for multiple kill rewards FIXME: implement this/make available to Lua

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	float currentAimSpreadScale;

	int pickObjectiveTime;                  ///< last time an objective was taken
	int dropObjectiveTime;                  ///< last time an objective was dropped
	int dropWeaponTime;                     ///< last time a weapon was dropped
	int limboDropWeapon;                    ///< weapon to drop in limbo
	int lastBurnTime;                       ///< last time index for flamethrower burn
	int saved_persistant[MAX_PERSISTANT];   ///< Save ps->persistant here during Limbo

	gentity_t *touchingTOI;                 ///< the trigger_objective_info a player is touching this frame

	int lastConstructibleBlockingWarnTime;

	int landmineSpottedTime;
	gentity_t *landmineSpotted;

	int speedScale;

	combatstate_t combatState;

	int topMarker;
	clientMarker_t clientMarkers[MAX_CLIENT_MARKERS];
	clientMarker_t backupMarker;

	// zinx etpro antiwarp
	int lastUpdateFrame;
	int frameOffset; // unused - FIXME?
	qboolean warping;
	qboolean warped;
	int lastCmdRealTime;
	int cmdhead;                            ///< antiwarp command queue head
	int cmdcount;                           ///< antiwarp command queue # valid commands
	float cmddelta;                         ///< antiwarp command queue # valid commands
	usercmd_t cmds[LAG_MAX_COMMANDS];       ///< antiwarp command queue

	gentity_t *tempHead;                    ///< storing a temporary head for bullet head shot detection
	gentity_t *tempLeg;                     ///< storing a temporary leg for bullet head shot detection

	int flagParent;

	// the next 2 are used to play the proper animation on the body
	int torsoDeathAnim;
	int legsDeathAnim;

	int lastSpammyCentrePrintTime;
	pmoveExt_t pmext;
#ifdef FEATURE_SERVERMDX
	qboolean deathAnim;                     ///< if true body has effect1Time set so death animation may start
#endif
	int deathAnimTime;                      ///< time when anim ends

	int deathTime;                          ///< if we are dead, when did we die

	int disguiseClientNum;

	int medals;
	float acc;
	float hspct;

	int flametime;                          ///< flamethrower exploit fix

	qboolean hasaward;
	qboolean wantsscore;
	qboolean maxlivescalced;

	qboolean freezed;                       ///< client is frozen see PM_FREEZE

	int constructSoundTime;                 ///< construction sound time
};

/**
 * @struct brushmodelInfo_t
 * @brief
 */
typedef struct
{
	char modelname[32];
	int model;
} brushmodelInfo_t;

/**
 * @struct limbo_cam_s
 * @typedef limbo_cam_t
 * @brief
 */
typedef struct limbo_cam_s
{
	qboolean hasEnt;
	int targetEnt;
	vec3_t angles;
	vec3_t origin;
	qboolean spawn;
	int info;
} limbo_cam_t;

#define MAX_LIMBO_CAMS 32

// this structure is cleared as each map is entered
#define MAX_SPAWN_VARS           64
#define MAX_SPAWN_VARS_CHARS     2048
#define VOTE_MAXSTRING           256     ///< Same value as MAX_STRING_TOKENS
#define MAX_SCRIPT_ACCUM_BUFFERS 10      ///< increased from 8 to 10 for compatability with maps that relied on it before Project: Bug Fix #055

/**
 * @struct voteInfo_s
 * @typedef voteInfo_t
 * @brief
 */
typedef struct voteInfo_s
{
	char voteString[MAX_STRING_CHARS];
	int voteTime;                       ///< level.time vote was called
	int voteYes;
	int voteNo;
	int numVotingClients;               ///< set by CalculateRanks
	int numVotingTeamClients[2];
	int (*vote_fn)(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
	char vote_value[VOTE_MAXSTRING];    ///< Desired vote item setting.
	int voteCaller;                     ///< id of the vote caller
	int voteTeam;                       ///< id of the vote caller's team
} voteInfo_t;

/**
 * @struct cfgCvar_s
 * @typedef cfgCvar_t
 * @brief
 */
typedef struct cfgCvar_s
{
	char name[256];
	char value[256];
} cfgCvar_t;

/**
 * @struct config_s
 * @typedef config_t
 * @brief
 */
typedef struct config_s
{
	char name[256];
	char version[256];
	char signature[256];
	char mapscripthash[256];
	cfgCvar_t setl[256];
	int numSetl;
	qboolean loaded;
	qboolean publicConfig;
} config_t;

#ifdef FEATURE_DBMS
#include "sqlite3.h"

typedef struct database_s
{
	char path[MAX_OSPATH];
	sqlite3 *db;
	int initialized;
} database_t;
#endif

typedef struct spawnPointState_s
{
	vec3_t origin;
	team_t team;
	int playerCount;
	int isActive;
	char description[128];
} spawnPointState_t;

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

	int warmupTime;                             ///< restart match at this time

	fileHandle_t logFile;

	qboolean etLegacyServer;

	char rawmapname[MAX_QPATH];

	int maxclients;                             ///< store latched cvars here that we want to get at often

	int framenum;
	int time;                                   ///< in msec
	int overTime;                               ///< workaround for dual objective timelimit bug
	int previousTime;                           ///< so movers can back up when blocked
	int frameTime;                              ///< time the frame started, for antilag stuff

	int startTime;                              ///< level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime;                   ///< last time of client team location update

	qboolean restarted;                         ///< waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;                 ///< includes connecting clients
	int numPlayingClients;                      ///< connected, non-spectators
	int sortedClients[MAX_CLIENTS];             ///< sorted by score

	int warmupModificationCount;                ///< for detecting if g_warmup is changed

	// voting
	voteInfo_t voteInfo;
	int numTeamClients[2];
	int numVotingTeamClients[2];

	// spawn variables
	qboolean spawning;                          ///< the G_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];         ///< key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	/// intermission state
	int intermissionQueued;                     ///< intermission was qualified, but wait INTERMISSION_DELAY_TIME before
	///< actually going there so the last frag can be watched. Disable future
	///< kills during this delay
	int intermissiontime;                       ///< time the intermission was started
	int exitTime;
	vec3_t intermission_origin;                 ///< also used for spectator spawns
	vec3_t intermission_angle;
	qboolean lmsDoNextMap;                      ///< should LMS do a map_restart or a vstr nextmap

	int bodyQueIndex;                           ///< dead bodies
	gentity_t *bodyQue[BODY_QUEUE_SIZE];

	int numSpawnPoints;                        ////< number of spawn points in this map
	spawnPointState_t spawnPointStates[MAX_MULTI_SPAWNTARGETS];

	/// entity scripting
	char *scriptEntity;

	/// player/AI model scripting (server repository)
	animScriptData_t animScriptData;

	int lastRestartTime;

	int numFinalDead[2];                        ///< unable to respawn and in limbo (per team)
	int numOidTriggers;

	qboolean latchGametype;

	int globalAccumBuffer[MAX_SCRIPT_ACCUM_BUFFERS];

	int soldierChargeTime[2];
	int medicChargeTime[2];
	int engineerChargeTime[2];
	int fieldopsChargeTime[2];
	int covertopsChargeTime[2];

	int lastMapEntityUpdate;
	int lastMapSpottedMinesUpdate;
	int objectiveStatsAllies[MAX_OBJECTIVES];
	int objectiveStatsAxis[MAX_OBJECTIVES];

	int lastSystemMsgTime[2];

	float soldierChargeTimeModifier[2];
	float medicChargeTimeModifier[2];
	float engineerChargeTimeModifier[2];
	float fieldopsChargeTimeModifier[2];
	float covertopsChargeTimeModifier[2];

	int firstbloodTeam;
	int teamEliminateTime;
	int lmsWinningTeam;

	int campaignCount;
	int currentCampaign;
	qboolean newCampaign;

	brushmodelInfo_t brushModelInfo[128];
	int numBrushModels;
	gentity_t *gameManager;

	qboolean doorAllowTeams;                    ///< used by bots to decide whether or not to use team travel flags

	/// for multiplayer fireteams
	fireteamData_t fireTeams[MAX_FIRETEAMS];

	qboolean ccLayers;

	int dwBlueReinfOffset;
	int dwRedReinfOffset;
	qboolean fLocalHost;
	qboolean fResetStats;
	int match_pause;                            ///< Paused state of the match
	qboolean ref_allready;                      ///< Referee forced match start
	int server_settings;                        ///<
	int sortedStats[MAX_CLIENTS];               ///< sorted by weapon stat
	int timeCurrent;                            ///< Real game clock
	int timeDelta;                              ///< Offset from internal clock - used to calculate real match time

	qboolean mapcoordsValid, tracemapLoaded;
	vec2_t mapcoordsMins, mapcoordsMaxs;

	char tinfoAxis[1024];                       ///< sent as server command (limited to 1022 chars)
	char tinfoAllies[1024];                     ///< sent as server command (limited to 1022 chars)

	// debris test
	int numDebrisChunks;
	debrisChunk_t debrisChunks[MAX_DEBRISCHUNKS];

	qboolean disableTankExit;
	qboolean disableTankEnter;

	int axisAirstrikeCounter, alliedAirstrikeCounter; ///< airstrike rate limiting
	int axisArtilleryCounter, alliedArtilleryCounter; ///< artillery rate limiting
	int axisAutoSpawn, alliesAutoSpawn;

	limbo_cam_t limboCams[MAX_LIMBO_CAMS];
	int numLimboCams;

	float teamXP[SK_NUM_SKILLS][2];

	commanderTeamChat_t commanderSounds[2][MAX_COMMANDER_TEAM_SOUNDS];

	qboolean tempTraceIgnoreEnts[MAX_GENTITIES];

#ifdef FEATURE_OMNIBOT
	// Omni-bot time triggers
	qboolean twoMinute;
	qboolean thirtySecond;
#endif

	// MAPVOTE information
	int sortedMaps[MAX_VOTE_MAPS];
	mapVoteInfo_t mapvoteinfo[MAX_VOTE_MAPS];
	int mapVoteNumMaps;
	int mapsSinceLastXPReset;
	qboolean mapVotePlayersCount;

	// sv_cvars
	svCvar_t svCvars[MAX_SVCVARS];
	int svCvarsCount;

	config_t config;            ///< etpro style config

	// objective indicator
	int flagIndicator;
	int redFlagCounter;
	int blueFlagCounter;

#ifdef FEATURE_RATING
	// skill rating
	float alliesProb;
	float axisProb;
	float mapProb;             ///< win prob of Axis team
#endif

#ifdef FEATURE_DBMS
	database_t database;
#endif

	int frameStartTime;
} level_locals_t;

/**
 * @struct g_campaignInfo_t
 * @brief
 */
typedef struct
{
	char mapnames[MAX_MAPS_PER_CAMPAIGN][MAX_QPATH];
	//arenaInfo_t   arenas[MAX_MAPS_PER_CAMPAIGN];
	int mapCount;
	int current;

	char shortname[256];
	char next[256];
	int typeBits;

#ifdef FEATURE_OMNIBOT
	//Omni-bot time triggers
	qboolean twoMinute;
	qboolean thirtySecond;
#endif

} g_campaignInfo_t;

// g_spawn.c
#define G_SpawnString(key, def, out) G_SpawnStringExt(key, def, out, __FILE__, __LINE__)
#define G_SpawnFloat(key, def, out) G_SpawnFloatExt(key, def, out, __FILE__, __LINE__)
#define G_SpawnInt(key, def, out) G_SpawnIntExt(key, def, out, __FILE__, __LINE__)
#define G_SpawnVector(key, def, out) G_SpawnVectorExt(key, def, out, __FILE__, __LINE__)
#define G_SpawnVector2D(key, def, out) G_SpawnVector2DExt(key, def, out, __FILE__, __LINE__)

qboolean G_SpawnStringExt(const char *key, const char *defaultString, char **out, const char *file, int line);      // spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean G_SpawnFloatExt(const char *key, const char *defaultString, float *out, const char *file, int line);
qboolean G_SpawnIntExt(const char *key, const char *defaultString, int *out, const char *file, int line);
qboolean G_SpawnVectorExt(const char *key, const char *defaultString, float *out, const char *file, int line);
qboolean G_SpawnVector2DExt(const char *key, const char *defaultString, float *out, const char *file, int line);

void G_SpawnEntitiesFromString(void);
char *G_NewString(const char *string);

gentity_t *G_SpawnGEntityFromSpawnVars(void);
qboolean G_CallSpawn(gentity_t *ent);

char *G_AddSpawnVarToken(const char *string);
void G_ParseField(const char *key, const char *value, gentity_t *ent);

// g_cmds.c
void Cmd_Score_f(gentity_t *ent);
void StopFollowing(gentity_t *ent);
void G_TeamDataForString(const char *teamstr, int clientNum, team_t *team, spectatorState_t *sState);
qboolean SetTeam(gentity_t *ent, const char *s, qboolean force, weapon_t w1, weapon_t w2, qboolean setweapons);
void G_SetClientWeapons(gentity_t *ent, weapon_t w1, weapon_t w2, qboolean updateclient);
void Cmd_FollowCycle_f(gentity_t *ent, int dir, qboolean skipBots);
qboolean G_FollowSame(gentity_t *ent);
void Cmd_Kill_f(gentity_t *ent);

#ifdef ETLEGACY_DEBUG
#ifdef FEATURE_OMNIBOT
void Cmd_SwapPlacesWithBot_f(gentity_t *ent, int botNum);
#endif
#endif

// MAPVOTE
void G_IntermissionMapVote(gentity_t *ent);
void G_IntermissionMapList(gentity_t *ent);
void G_IntermissionVoteTally(gentity_t *ent);

void G_EntitySound(gentity_t *ent, const char *soundId, int volume); // Unused.
void G_EntitySoundNoCut(gentity_t *ent, const char *soundId, int volume); // Unused.
qboolean G_MatchOnePlayer(int *plist, char *err, size_t len);
int ClientNumberFromString(gentity_t *to, char *s);
void SanitizeString(char *in, char *out, qboolean fToLower);

// g_items.c
void G_RunItem(gentity_t *ent);
void RespawnItem(gentity_t *ent);

gentity_t *Drop_Item(gentity_t *ent, gitem_t *item, float angle, qboolean novelocity);
gentity_t *LaunchItem(gitem_t *item, vec3_t origin, vec3_t velocity, int ownerNum);
void G_SpawnItem(gentity_t *ent, gitem_t *item);
void FinishSpawningItem(gentity_t *ent);

void Fill_Clip(playerState_t *ps, weapon_t weapon);
int Add_Ammo(gentity_t *ent, weapon_t weapon, int count, qboolean fillClip);
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace);
qboolean AddMagicAmmo(gentity_t *receiver, int numOfClips);
weapon_t G_GetPrimaryWeaponForClient(gclient_t *client);
weapon_t G_GetPrimaryWeaponForClientSoldier(gclient_t *client);
void G_DropWeapon(gentity_t *ent, weapon_t weapon);

// Touch_Item_Auto is bound by the rules of autoactivation (if cg_autoactivate is 0, only touch on "activate")
void Touch_Item_Auto(gentity_t *ent, gentity_t *other, trace_t *trace);

void Prop_Break_Sound(gentity_t *ent);
void Spawn_Shard(gentity_t *ent, gentity_t *inflictor, float quantity, int type);

// g_utils.c
int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create);
void G_RemoveConfigstringIndex(const char *name, int start, int max);

int G_ModelIndex(const char *name);
int G_SoundIndex(const char *name);
int G_SkinIndex(const char *name);
int G_ShaderIndex(const char *name);
int G_CharacterIndex(const char *name);
int G_StringIndex(const char *string);
qboolean G_AllowTeamsAllowed(gentity_t *ent, gentity_t *activator);
void G_UseEntity(gentity_t *ent, gentity_t *other, gentity_t *activator);
qboolean G_IsWeaponDisabled(gentity_t *ent, weapon_t weapon);
void G_TeamCommand(team_t team, const char *cmd);

gentity_t *G_Find(gentity_t *from, int fieldofs, const char *match);
gentity_t *G_FindInt(gentity_t *from, int fieldofs, int match);
gentity_t *G_FindFloat(gentity_t *from, int fieldofs, float match);
gentity_t *G_FindVector(gentity_t *from, int fieldofs, const vec3_t match);
gentity_t *G_FindByTargetname(gentity_t *from, const char *match);
gentity_t *G_FindByTargetnameFast(gentity_t *from, const char *match, int hash);
gentity_t *G_PickTarget(const char *targetname);
void G_UseTargets(gentity_t *ent, gentity_t *activator);
void G_SetMovedir(vec3_t angles, vec3_t movedir);

void G_InitGentity(gentity_t *e);
gentity_t *G_Spawn(void);
gentity_t *G_TempEntity(vec3_t origin, entity_event_t event);
gentity_t *G_TempEntityNotLinked(entity_event_t event);
gentity_t *G_PopupMessage(popupMessageType_t type);
void G_Sound(gentity_t *ent, int soundIndex);
void G_AnimScriptSound(int soundIndex, vec3_t org, int client);
void G_FreeEntity(gentity_t *ent);
int G_EntitiesFree(void);
void G_ClientSound(gentity_t *ent, int soundIndex);

void G_TouchTriggers(gentity_t *ent);

char *vtos(const vec3_t v);

void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm);
void G_AddEvent(gentity_t *ent, int event, int eventParm);
void G_SetOrigin(gentity_t *ent, vec3_t origin);
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
void G_ResetRemappedShaders(void);
const char *BuildShaderStateConfig(void);

void G_SetAngle(gentity_t *ent, vec3_t angle);

qboolean infront(gentity_t *self, gentity_t *other);

void G_ProcessTagConnect(gentity_t *ent, qboolean clearAngles);

void G_SetEntState(gentity_t *ent, entState_t state);
void G_ParseCampaigns(void);
qboolean G_MapIsValidCampaignStartMap(void);

team_t G_GetTeamFromEntity(gentity_t *ent);

const char *G_StringContains(const char *str1, const char *str2, int casesensitive);
qboolean G_MatchString(const char *filter, const char *name, int casesensitive);

qboolean CG_ParseMapVotePlayersCountConfig(void);

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

// g_combat.c
void G_AdjustedDamageVec(gentity_t *ent, vec3_t origin, vec3_t vec);
qboolean CanDamage(gentity_t *targ, vec3_t origin);
void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, meansOfDeath_t mod);
qboolean G_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, meansOfDeath_t mod);
qboolean etpro_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, meansOfDeath_t mod, qboolean clientsonly);
void body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t meansOfDeath);
void TossWeapons(gentity_t *self);
gentity_t *G_BuildHead(gentity_t *ent, grefEntity_t *refent, qboolean newRefent);
gentity_t *G_BuildLeg(gentity_t *ent, grefEntity_t *refent, qboolean newRefent);

// damage flags
#define DAMAGE_RADIUS               0x00000001  ///< damage was indirect
#define DAMAGE_HALF_KNOCKBACK       0x00000002  ///< do less knockback
#define DAMAGE_NO_KNOCKBACK         0x00000008  ///< do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION        0x00000020  ///< armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  ///< unused. Armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_DISTANCEFALLOFF      0x00000040  ///< distance falloff

// g_missile.c
void G_RunMissile(gentity_t *ent);
int G_PredictMissile(gentity_t *ent, int duration, vec3_t endPos, qboolean allowBounce); // unused

// server side flamethrower collision
void G_RunFlamechunk(gentity_t *ent);

gentity_t *fire_flamechunk(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_missile(gentity_t *self, vec3_t start, vec3_t dir, int weapon);

void Fire_Lead_Ext(gentity_t *ent, gentity_t *activator, float spread, int damage, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, meansOfDeath_t mod);

gentity_t *fire_flamebarrel(gentity_t *self, vec3_t start, vec3_t dir);

// g_mover.c
gentity_t *G_TestEntityPosition(gentity_t *ent);
void G_RunMover(gentity_t *ent);
qboolean G_MoverPush(gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle);
void Use_BinaryMover(gentity_t *ent, gentity_t *other, gentity_t *activator);

void G_TryDoor(gentity_t *ent, gentity_t *other, gentity_t *activator);

void InitMoverRotate(gentity_t *ent);

void InitMover(gentity_t *ent);
void SetMoverState(gentity_t *ent, moverState_t moverState, int time);

void func_constructible_underconstructionthink(gentity_t *ent);

// g_trigger.c
void Think_SetupObjectiveInfo(gentity_t *ent);

// g_misc.c
void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles);
void mg42_fire(gentity_t *other);
void mg42_stopusing(gentity_t *self);
void aagun_fire(gentity_t *other);
void aagun_stopusing(gentity_t *self);

float AngleDifference(float ang1, float ang2);
qboolean G_FlingClient(gentity_t *vic, int flingType);

void G_PreFilledMissileEntity(gentity_t *ent, int weaponNum, int realWeapon, int ownerNum, team_t teamNum, int clientNum, gentity_t *parent, const vec3_t start, const vec3_t dir);

int G_GetEnemyPosition(gentity_t *ent, gentity_t *targ);

void G_MagicSink(gentity_t *self);

// g_weapon.c
qboolean AccuracyHit(gentity_t *target, gentity_t *attacker);
void CalcMuzzlePoint(gentity_t *ent, int weapon, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void SnapVectorTowards(vec3_t v, vec3_t to);

void G_FadeItems(gentity_t *ent, int modType);
gentity_t *G_FindSatchel(gentity_t *ent);
void G_ExplodeMines(gentity_t *ent);
qboolean G_ExplodeSatchels(gentity_t *ent);
void G_FreeSatchel(gentity_t *ent);

void CalcMuzzlePoints(gentity_t *ent, int weapon);
void CalcMuzzlePointForActivate(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);

void Weapon_MagicAmmo_Ext(gentity_t *ent, vec3_t viewpos, vec3_t tosspos, vec3_t velocity);
void Weapon_Medic_Ext(gentity_t *ent, vec3_t viewpos, vec3_t tosspos, vec3_t velocity);

// g_client.c
int TeamCount(int ignoreClientNum, team_t team);
team_t PickTeam(int ignoreClientNum);
void SetClientViewAngle(gentity_t *ent, vec3_t angle);
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles);
void respawn(gentity_t *ent);
void BeginIntermission(void);
void InitBodyQue(void);
void ClientSpawn(gentity_t *ent, qboolean revived, qboolean teamChange, qboolean restoreHealth);
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t meansOfDeath);
void AddKillScore(gentity_t *ent, int score);
void CalculateRanks(void);
qboolean SpotWouldTelefrag(gentity_t *spot);
void AddMedicTeamBonus(gclient_t *client);

void SetWolfSpawnWeapons(gclient_t *client);
void limbo(gentity_t *ent, qboolean makeCorpse);
void reinforce(gentity_t *ent);

// *LUA* & map configs g_sha1.c
char *G_SHA1(const char *string);

#define CF_ENABLE    1 ///< enables common countryflags functionallity
#define CF_CONNECT   2 ///< draws country name in connect announcer

// g_character.c
qboolean G_RegisterCharacter(const char *characterFile, bg_character_t *character);
void G_RegisterPlayerClasses(void);
void G_UpdateCharacter(gclient_t *client);

// g_svcmds.c
qboolean ConsoleCommand(void);
void G_ProcessIPBans(void);
qboolean G_FilterIPBanPacket(char *from);
qboolean G_FilterMaxLivesPacket(char *from);
qboolean G_FilterMaxLivesIPPacket(char *from);
void AddMaxLivesGUID(const char *str);
void AddMaxLivesBan(const char *str);
void ClearMaxLivesBans(void);
void AddIPBan(const char *str);

void Svcmd_ShuffleTeamsXP_f(qboolean restart);
void Svcmd_ShuffleTeamsSR_f(qboolean restart);

// g_weapon.c
void FireWeapon(gentity_t *ent);
void G_BurnMeGood(gentity_t *self, gentity_t *body, gentity_t *chunk);

void MoveClientToIntermission(gentity_t *ent, qboolean hasVoted);
void G_SendScore(gentity_t *ent);

// g_cmds.c
void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText);
void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, qboolean localize);   // removed static declaration so it would link
void G_HQSay(gentity_t *other, int color, const char *name, const char *message);
qboolean Cmd_CallVote_f(gentity_t *ent, unsigned int dwCommand, qboolean fRefCommand);
void Cmd_Follow_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0);
void Cmd_Team_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_PlaySound_Cmd(void);
int ClientNumbersFromString(char *s, int *plist);
char *ConcatArgs(int start);
void G_DropItems(gentity_t *self);

// g_main.c
void FindIntermissionPoint(void);
void G_RunThink(gentity_t *ent);
void QDECL G_LogPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void G_LogExit(const char *string);
void SendScoreboardMessageToAllClients(void);
void QDECL G_Printf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_DPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_Error(const char *fmt, ...) _attribute ((noreturn, format(printf, 1, 2)));

// g_client.c
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void ClientUserinfoChanged(int clientNum);
void ClientDisconnect(int clientNum);
void ClientBegin(int clientNum);
void ClientCommand(int clientNum);
float ClientHitboxMaxZ(gentity_t *hitEnt);

// g_active.c
void ClientThink(int clientNum);
void ClientEndFrame(gentity_t *ent);
void G_RunClient(gentity_t *ent);
void ClientThink_cmd(gentity_t *ent, usercmd_t *cmd);

// et-antiwarp.c
void etpro_AddUsercmd(int clientNum, usercmd_t *cmd);
void DoClientThinks(gentity_t *ent);
qboolean G_DoAntiwarp(gentity_t *ent);

// Does ent have enough "energy" to call artillery?
qboolean ReadyToCallArtillery(gentity_t *ent);
// to call airstrike?
qboolean ReadyToCallAirstrike(gentity_t *ent);
// Are we ready to construct?  Optionally, will also update the time while we are constructing
qboolean ReadyToConstruct(gentity_t *ent, gentity_t *constructible, qboolean updateState);

// g_team.c
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);
//int Team_ClassForString(const char *string); // Unused

// g_mem.c
void *G_Alloc(unsigned int size);
void G_InitMemory(void);
void Svcmd_GameMem_f(void);

// g_session.c
void G_ReadSessionData(gclient_t *client);
void G_InitSessionData(gclient_t *client, const char *userinfo);

void G_InitWorldSession(void);
void G_WriteSessionData(qboolean restart);

void G_CalcRank(gclient_t *client);

// g_cmd.c
void Cmd_Activate_f(gentity_t *ent);
void Cmd_Activate2_f(gentity_t *ent);
qboolean Do_Activate_f(gentity_t *ent, gentity_t *traceEnt);
void G_LeaveTank(gentity_t *ent, qboolean position);

char *Q_AddCR(char *s);

// g_script.c
void G_Script_ScriptParse(gentity_t *ent);
qboolean G_Script_ScriptRun(gentity_t *ent);
void G_Script_ScriptLoad(void);

void mountedmg42_fire(gentity_t *other);
void script_mover_use(gentity_t *ent, gentity_t *other, gentity_t *activator);
void script_mover_blocked(gentity_t *ent, gentity_t *other);

qboolean G_ScriptAction_GotoMarker(gentity_t *ent, char *params);
qboolean G_ScriptAction_Wait(gentity_t *ent, char *params);
qboolean G_ScriptAction_Trigger(gentity_t *ent, char *params);
qboolean G_ScriptAction_PlaySound(gentity_t *ent, char *params);
qboolean G_ScriptAction_PlayAnim(gentity_t *ent, char *params);
qboolean G_ScriptAction_AlertEntity(gentity_t *ent, char *params);
qboolean G_ScriptAction_ToggleSpeaker(gentity_t *ent, char *params);
qboolean G_ScriptAction_DisableSpeaker(gentity_t *ent, char *params);
qboolean G_ScriptAction_EnableSpeaker(gentity_t *ent, char *params);
qboolean G_ScriptAction_Accum(gentity_t *ent, char *params);
qboolean G_ScriptAction_GlobalAccum(gentity_t *ent, char *params);
qboolean G_ScriptAction_Print(gentity_t *ent, char *params);
qboolean G_ScriptAction_FaceAngles(gentity_t *ent, char *params);
qboolean G_ScriptAction_ResetScript(gentity_t *ent, char *params);
qboolean G_ScriptAction_TagConnect(gentity_t *ent, char *params);
qboolean G_ScriptAction_Halt(gentity_t *ent, char *params);
qboolean G_ScriptAction_StopSound(gentity_t *ent, char *params);
qboolean G_ScriptAction_EntityScriptName(gentity_t *ent, char *params);
qboolean G_ScriptAction_AxisRespawntime(gentity_t *ent, char *params);
qboolean G_ScriptAction_AlliedRespawntime(gentity_t *ent, char *params);
qboolean G_ScriptAction_NumberofObjectives(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetWinner(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetDefendingTeam(gentity_t *ent, char *params);
qboolean G_ScriptAction_Announce(gentity_t *ent, char *params);
qboolean G_ScriptAction_Announce_Icon(gentity_t *ent, char *params);
qboolean G_ScriptAction_AddTeamVoiceAnnounce(gentity_t *ent, char *params);
qboolean G_ScriptAction_RemoveTeamVoiceAnnounce(gentity_t *ent, char *params);
qboolean G_ScriptAction_TeamVoiceAnnounce(gentity_t *ent, char *params);
qboolean G_ScriptAction_EndRound(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetRoundTimelimit(gentity_t *ent, char *params);
qboolean G_ScriptAction_RemoveEntity(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetState(gentity_t *ent, char *params);
qboolean G_ScriptAction_VoiceAnnounce(gentity_t *ent, char *params);
qboolean G_ScriptAction_FollowSpline(gentity_t *ent, char *params);
qboolean G_ScriptAction_FollowPath(gentity_t *ent, char *params);
qboolean G_ScriptAction_AbortMove(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetSpeed(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetRotation(gentity_t *ent, char *params);
qboolean G_ScriptAction_StopRotation(gentity_t *ent, char *params);
qboolean G_ScriptAction_StartAnimation(gentity_t *ent, char *params);
qboolean G_ScriptAction_AttatchToTrain(gentity_t *ent, char *params);
qboolean G_ScriptAction_FreezeAnimation(gentity_t *ent, char *params);
qboolean G_ScriptAction_UnFreezeAnimation(gentity_t *ent, char *params);
qboolean G_ScriptAction_ShaderRemap(gentity_t *ent, char *params);
qboolean G_ScriptAction_ShaderRemapFlush(gentity_t *ent, char *params);
qboolean G_ScriptAction_ChangeModel(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetChargeTimeFactor(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetDamagable(gentity_t *ent, char *params);
qboolean G_ScriptAction_RepairMG42(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetHQStatus(gentity_t *ent, char *params);
qboolean G_ScriptAction_PrintAccum(gentity_t *ent, char *params);
qboolean G_ScriptAction_PrintGlobalAccum(gentity_t *ent, char *params);

qboolean G_ScriptAction_ObjectiveStatus(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetModelFromBrushmodel(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetPosition(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetAutoSpawn(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetMainObjective(gentity_t *ent, char *params);
qboolean G_ScriptAction_SpawnRubble(gentity_t *ent, char *params);
qboolean G_ScriptAction_AllowTankExit(gentity_t *ent, char *params);
qboolean G_ScriptAction_AllowTankEnter(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetTankAmmo(gentity_t *ent, char *params);
qboolean G_ScriptAction_AddTankAmmo(gentity_t *ent, char *params);
qboolean G_ScriptAction_Kill(gentity_t *ent, char *params);
qboolean G_ScriptAction_DisableMessage(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetGlobalFog(gentity_t *ent, char *params);

qboolean G_ScriptAction_Cvar(gentity_t *ent, char *params);
qboolean G_ScriptAction_AbortIfWarmup(gentity_t *ent, char *params);
qboolean G_ScriptAction_AbortIfNotSinglePlayer(gentity_t *ent, char *params);
qboolean G_ScriptAction_MusicStart(gentity_t *ent, char *params);
qboolean G_ScriptAction_MusicPlay(gentity_t *ent, char *params);
qboolean G_ScriptAction_MusicStop(gentity_t *ent, char *params);
qboolean G_ScriptAction_MusicQueue(gentity_t *ent, char *params);
qboolean G_ScriptAction_MusicFade(gentity_t *ent, char *params);
qboolean G_ScriptAction_SetDebugLevel(gentity_t *ent, char *params);
qboolean G_ScriptAction_FadeAllSounds(gentity_t *ent, char *params);

qboolean G_ScriptAction_Construct(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleClass(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleChargeBarReq(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleConstructXPBonus(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleDestructXPBonus(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleHealth(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleWeaponclass(gentity_t *ent, char *params) ;
qboolean G_ScriptAction_ConstructibleDuration(gentity_t *ent, char *params) ;

qboolean etpro_ScriptAction_SetValues(gentity_t *ent, char *params);
qboolean G_ScriptAction_Create(gentity_t *ent, char *params);
qboolean G_ScriptAction_Delete(gentity_t *ent, char *params);

// g_props.c
void Props_Chair_Skyboxtouch(gentity_t *ent);

extern level_locals_t   level;
extern gentity_t        g_entities[];   ///< was explicitly set to MAX_ENTITIES
extern g_campaignInfo_t g_campaigns[];

#define FOFS(x) ((size_t)&(((gentity_t *)0)->x))

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
#endif

extern vmCvar_t g_protect;

extern vmCvar_t g_dropHealth;
extern vmCvar_t g_dropAmmo;

extern vmCvar_t g_shove;

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

#define STICKYCHARGE_NONE 0 // default, reset charge on any death
#define STICKYCHARGE_SELFKILL 1 // keep charge after selfkill, mortal self damage, teamkill, mortal world damage
#define STICKYCHARGE_ANYDEATH 2 // keep charge after any death (for eg. death by enemy)
extern vmCvar_t g_stickyCharge;
extern vmCvar_t g_xpSaver;

#define DYNAMITECHAINING_EXPLOSION 0 // default, explode dynamites chaining
#define DYNAMITECHAINING_FREE 1      // free dynamites chaining
extern vmCvar_t g_dynamiteChaining;

extern vmCvar_t g_playerHitBoxHeight;

/**
 * @struct GeoIPTag
 * @typedef GeoIP
 * @brief
 */
typedef struct GeoIPTag
{
	fileHandle_t GeoIPDatabase;
	unsigned char *cache;
	unsigned int memsize;
} GeoIP;

unsigned long GeoIP_addr_to_num(const char *addr);
unsigned int GeoIP_seek_record(GeoIP *gi, unsigned long ipnum);
void GeoIP_open(void);
void GeoIP_close(void);

extern GeoIP *gidb;

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
#ifdef FEATURE_OMNIBOT
int trap_BotGetServerCommand(int clientNum, char *message, int size);
void trap_BotUserCommand(int clientNum, usercmd_t *ucmd);
void trap_EA_Command(int clientNum, char *command);
#endif

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

void G_ExplodeMissile(gentity_t *ent);

void Svcmd_StartMatch_f(void);
void Svcmd_ResetMatch_f(qboolean fDoReset, qboolean fDoRestart);
void Svcmd_SwapTeams_f(void);

// g_antilag.c
void G_StoreClientPosition(gentity_t *ent);
qboolean G_ReAdjustSingleClientPosition(gentity_t *ent);
void G_ResetMarkers(gentity_t *ent);
void G_HistoricalTrace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void G_HistoricalTraceBegin(gentity_t *ent);
void G_HistoricalTraceEnd(gentity_t *ent);
void G_Trace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void G_PredictPmove(gentity_t *ent, float frametime);

#define BODY_VALUE(ENT) ENT->watertype
#define BODY_TEAM(ENT) ENT->s.modelindex
#define BODY_CLASS(ENT) ENT->s.modelindex2
#define BODY_CHARACTER(ENT) ENT->s.onFireStart

void Cmd_FireTeam_MP_f(gentity_t *ent);

void G_RemoveFromAllIgnoreLists(int clientNum);

// g_teammapdata.c

/**
 * @struct mapEntityData_s
 * @typedef mapEntityData_t
 * @brief
 */
typedef struct mapEntityData_s
{
	vec3_t org;
	int yaw;
	int data;
	int type;
	int startTime;
	int singleClient;

	int entNum;
	struct mapEntityData_s *next, *prev;
} mapEntityData_t;

/**
 * @struct mapEntityData_Team_s
 * @typedef mapEntityData_Team_t
 * @brief
 */
typedef struct mapEntityData_Team_s
{
	mapEntityData_t mapEntityData_Team[MAX_GENTITIES];
	mapEntityData_t *freeMapEntityData;                 ///< single linked list
	mapEntityData_t activeMapEntityData;                ///< double linked list
} mapEntityData_Team_t;

extern mapEntityData_Team_t mapEntityData[2];

void G_InitMapEntityData(mapEntityData_Team_t *teamList);
mapEntityData_t *G_FreeMapEntityData(mapEntityData_Team_t *teamList, mapEntityData_t *mEnt);
mapEntityData_t *G_AllocMapEntityData(mapEntityData_Team_t *teamList);
mapEntityData_t *G_FindMapEntityData(mapEntityData_Team_t *teamList, int entNum);
mapEntityData_t *G_FindMapEntityDataSingleClient(mapEntityData_Team_t *teamList, mapEntityData_t *start, int entNum, int clientNum);

void G_ResetTeamMapData(void);
void G_UpdateTeamMapData(void);

void G_SetupFrustum(gentity_t *ent);
void G_SetupFrustum_ForBinoculars(gentity_t *ent);
qboolean G_VisibleFromBinoculars(gentity_t *viewer, gentity_t *ent, vec3_t origin);

void G_LogTeamKill(gentity_t *ent, weapon_t weap);
void G_LogDeath(gentity_t *ent, weapon_t weap);
void G_LogKill(gentity_t *ent, weapon_t weap);
void G_LogRegionHit(gentity_t *ent, hitRegion_t hr);

// Skills
void G_ResetXP(gentity_t *ent);
void G_SetPlayerScore(gclient_t *client);
void G_SetPlayerSkill(gclient_t *client, skillType_t skill);
void G_AddSkillPoints(gentity_t *ent, skillType_t skill, float points);
void G_LoseSkillPoints(gentity_t *ent, skillType_t skill, float points);
void G_AddKillSkillPoints(gentity_t *attacker, meansOfDeath_t mod, hitRegion_t hr, qboolean splash);
void G_AddKillSkillPointsForDestruction(gentity_t *attacker, meansOfDeath_t mod, g_constructible_stats_t *constructibleStats);
void G_LoseKillSkillPoints(gentity_t *tker, meansOfDeath_t mod, hitRegion_t hr, qboolean splash);

void G_DebugOpenSkillLog(void);
void G_DebugCloseSkillLog(void);
void G_DebugAddSkillLevel(gentity_t *ent, skillType_t skill);
void G_DebugAddSkillPoints(gentity_t *ent, skillType_t skill, float points, const char *reason);

//void G_CheckForNeededClasses(void); // unused
//void G_CheckMenDown(void);
void G_SendMapEntityInfo(gentity_t *e);
void G_SendSystemMessage(sysMsg_t message, int team);
int G_GetSysMessageNumber(const char *sysMsg);
int G_CountTeamLandmines(team_t team);
qboolean G_SweepForLandmines(vec3_t origin, float radius, int team);

void G_AddClientToFireteam(int entityNum, int leaderNum);
void G_InviteToFireTeam(int entityNum, int otherEntityNum);
void G_UpdateFireteamConfigString(fireteamData_t *ft);
void G_RemoveClientFromFireteams(int entityNum, qboolean update, qboolean print);

void G_PrintClientSpammyCenterPrint(int entityNum, const char *text);

// Match settings
#define PAUSE_NONE      0x00    ///< Match is NOT paused.
#define PAUSE_UNPAUSING 0x01    ///< Pause is about to expire

// HRESULTS
#define G_OK         0
#define G_INVALID   -1
#define G_NOTFOUND  -2

#define AP(x) trap_SendServerCommand(-1, x)                     ///< Print to all
#define CP(x) trap_SendServerCommand(ent - g_entities, x)       ///< Print to an ent
#define CPx(x, y) trap_SendServerCommand(x, y)                  ///< Print to id = x

#define HELP_COLUMNS    4

#define CMD_DEBOUNCE    5000    ///< 5s between cmds

#define EOM_WEAPONSTATS 0x01    ///< Dump of player weapon stats at end of match.
#define EOM_MATCHINFO   0x02    ///< Dump of match stats at end of match.

#define AA_STATSALL     0x01    ///< Client AutoAction: Dump ALL player stats
#define AA_STATSTEAM    0x02    ///< Client AutoAction: Dump TEAM player stats

/**
 * @enum enum_t_dp
 * @brief "Delayed Print" ent enumerations
 */
typedef enum
{
	DP_PAUSEINFO = 0,   ///< Print current pause info
	DP_UNPAUSING,       ///< Print unpause countdown + unpause
	DP_CONNECTINFO,     ///< Display info on connect
	DP_MVSPAWN          ///< Set up MV views for clients who need them
} enum_t_dp;

// Remember: Axis = RED, Allies = BLUE ... right?!

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

// g_main.c
void G_UpdateCvars(void);
void G_wipeCvars(void);

// g_cmds_ext.c
qboolean G_commandCheck(gentity_t *ent, const char *cmd, qboolean fDoAnytime);
qboolean G_commandHelp(gentity_t *ent, const char *pszCommand, unsigned int dwCommand);
qboolean G_cmdDebounce(gentity_t *ent, const char *pszCommand);
void G_commands_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_lock_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock);
void G_pause_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fPause);
void G_players_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump);
void G_ready_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump);
void G_say_teamnl_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_sclogin_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_sclogout_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_makesc_cmd(void);
void G_removesc_cmd(void);
void G_scores_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_specinvite_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock);
void G_specuninvite_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock);
void G_speclock_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fLock);
void G_statsall_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump);
void G_teamready_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump);
void G_weaponRankings_cmd(gentity_t *ent, unsigned int dwCommand, qboolean state);
void G_weaponStats_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump);
void G_weaponStatsLeaders_cmd(gentity_t *ent, qboolean doTop, qboolean doWindow);
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly, float randomNum);

// g_config.c
qboolean G_configSet(const char *configname);
void G_ConfigCheckLocked(void);
void G_PrintConfigs(gentity_t *ent);
qboolean G_isValidConfig(gentity_t *ent, const char *configname);
void G_ReloadConfig(void);

// g_match.c
void G_addStats(gentity_t *targ, gentity_t *attacker, int damage, meansOfDeath_t mod);
void G_addStatsHeadShot(gentity_t *attacker, meansOfDeath_t mod);
int G_checkServerToggle(vmCvar_t *cv);
char *G_createStats(gentity_t *ent);
void G_deleteStats(int nClient);
qboolean G_desiredFollow(gentity_t *ent, int nTeam);
void G_globalSound(const char *sound);
void G_globalSoundEnum(int sound);
void G_initMatch(void);
void G_loadMatchGame(void);
void G_matchInfoDump(unsigned int dwDumpType);
void G_printMatchInfo(gentity_t *ent);
void G_parseStats(const char *pszStatsInfo);
void G_printFull(const char *str, gentity_t *ent);
void G_resetModeState(void);
void G_resetRoundState(void);
void G_spawnPrintf(int print_type, int print_time, gentity_t *owner);
void G_statsPrint(gentity_t *ent, int nType);

#ifdef FEATURE_DBMS
int G_DB_Init(void);
int G_DB_DeInit(void);
#endif

#ifdef FEATURE_RATING
// g_skillrating.c
#define MU      25.f            ///< mean
#define SIGMA   (MU / 3)        ///< standard deviation
#define BETA    (SIGMA / 2)     ///< skill chain length
#define TAU     (SIGMA / 100)   ///< dynamics factor
#define EPSILON 0.f             ///< draw margin (assumed null)
#define LAMBDA  10              ///< map continuity correction (n = 2 * LAMBDA, n >= 20)

void G_CalculateSkillRatings(void);
float G_CalculateWinProbability(int team);
void G_UpdateSkillRating(int winner);

typedef struct srData_s
{
	const unsigned char *guid;
	float mu;
	float sigma;
	int time_axis;
	int time_allies;
} srData_t;

int G_SkillRatingDBCheck(char *db_path, int db_mode);
int G_SkillRatingPrepareMatchRating(void);
int G_SkillRatingGetMatchRating(srData_t *sr_data);
int G_SkillRatingSetMatchRating(srData_t *sr_data);
int G_SkillRatingSetUserRating(srData_t *sr_data);
void G_SkillRatingGetClientRating(gclient_t *cl);
void G_SkillRatingSetClientRating(gclient_t *cl);
float G_SkillRatingGetMapRating(char *mapname);
void G_SkillRatingSetMapRating(char *mapname, int winner);
#endif

#ifdef FEATURE_PRESTIGE
// g_prestige.c
typedef struct prData_s
{
	const unsigned char *guid;
	int prestige;
	int streak;
	int skillpoints[SK_NUM_SKILLS];
} prData_t;

int G_PrestigeDBCheck(char *db_path, int db_mode);
void G_GetClientPrestige(gclient_t *cl);
void G_SetClientPrestige(gclient_t *cl, qboolean streakUp);
int G_ReadPrestige(prData_t *pr_data);
int G_WritePrestige(prData_t *pr_data);
#endif

int G_XPSaver_CheckDB(char *db_path, int db_mode);
void G_XPSaver_Load(gclient_t *cl);
void G_XPSaver_Store(gclient_t *cl);
int G_XPSaver_Clear();

// g_stats.c
void G_UpgradeSkill(gentity_t *ent, skillType_t skill);

#ifdef FEATURE_MULTIVIEW
// g_multiview.c
qboolean G_smvCommands(gentity_t *ent, const char *cmd);
void G_smvAdd_cmd(gentity_t *ent);
void G_smvAddTeam_cmd(gentity_t *ent, int nTeam);
void G_smvDel_cmd(gentity_t *ent);

void G_smvAddView(gentity_t *ent, int pID);
void G_smvAllRemoveSingleClient(int pID);
unsigned int G_smvGenerateClientList(gentity_t *ent);
qboolean G_smvLocateEntityInMVList(gentity_t *ent, int pID, qboolean fRemove);
void G_smvRegenerateClients(gentity_t *ent, int clientList);
void G_smvRemoveEntityInMVList(gentity_t *ent, mview_t *ref);
void G_smvRemoveInvalidClients(gentity_t *ent, int nTeam);
qboolean G_smvRunCamera(gentity_t *ent);
void G_smvUpdateClientCSList(gentity_t *ent);
#endif

// g_referee.c
void Cmd_AuthRcon_f(gentity_t *ent);
void G_refAllReady_cmd(gentity_t *ent);
void G_ref_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
qboolean G_refCommandCheck(gentity_t *ent, const char *cmd);
void G_refHelp_cmd(gentity_t *ent);
void G_refLockTeams_cmd(gentity_t *ent, qboolean fLock);
void G_refPause_cmd(gentity_t *ent, qboolean fPause);
void G_refPlayerPut_cmd(gentity_t *ent, team_t team_id);
void G_refRemove_cmd(gentity_t *ent);
void G_refSpeclockTeams_cmd(gentity_t *ent, qboolean fLock);
void G_refWarmup_cmd(gentity_t *ent);
void G_refWarning_cmd(gentity_t *ent);
void G_refMute_cmd(gentity_t *ent, qboolean mute);
void G_refMakeShoutcaster_cmd(gentity_t *ent);
void G_refRemoveShoutcaster_cmd(gentity_t *ent);
void G_refLogout_cmd(gentity_t *ent);
int G_refClientnumForName(gentity_t *ent, const char *name);
void G_refPrintf(gentity_t *ent, const char *fmt, ...) _attribute((format(printf, 2, 3)));
void G_PlayerBan(void);
void G_MakeReferee(void);
void G_RemoveReferee(void);
void G_MuteClient(void);
void G_UnMuteClient(void);
qboolean G_IsShoutcastPasswordSet(void);
qboolean G_IsShoutcastStatusAvailable(gentity_t *ent);
void G_MakeShoutcaster(gentity_t *ent);
void G_RemoveShoutcaster(gentity_t *ent);
void G_RemoveAllShoutcasters(void);

// g_team.c
extern const char *aTeams[TEAM_NUM_TEAMS];
extern team_info  teamInfo[TEAM_NUM_TEAMS];

qboolean G_allowFollow(gentity_t *ent, int nTeam);
int G_blockoutTeam(gentity_t *ent, int nTeam);
qboolean G_checkReady(void);
qboolean G_readyMatchState(void);
void G_removeSpecInvite(int team);
void G_shuffleTeamsXP(void);
void G_shuffleTeamsSR(void);
void G_swapTeamLocks(void);
void G_swapTeams(void);
qboolean G_teamJoinCheck(team_t nTeam, gentity_t *ent);
void G_teamReset(int team_num, qboolean fClearSpecLock);
void G_verifyMatchState(team_t nTeam);
void G_updateSpecLock(int nTeam, qboolean fLock);

int OtherTeam(int team);
const char *TeamName(int team);
const char *TeamColorString(int team);
void Team_DroppedFlagThink(gentity_t *ent);
void Team_ReturnFlag(gentity_t *ent);
gentity_t *SelectCTFSpawnPoint(team_t team, int teamstate, vec3_t origin, vec3_t angles, int spawnObjective);
void TeamplayInfoMessage(team_t team);
void CheckTeamStatus(void);
int Pickup_Team(gentity_t *ent, gentity_t *other);
void G_globalFlagIndicator(void);
void G_clientFlagIndicator(gentity_t *ent);

// g_vote.c
int G_voteCmdCheck(gentity_t *ent, char *arg, char *arg2, qboolean fRefereeCmd);
void G_voteFlags(void);
void G_voteHelp(gentity_t *ent, qboolean fShowVote);
void G_playersMessage(gentity_t *ent);
// Actual voting commands
int G_Gametype_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Kick_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Mute_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_UnMute_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Map_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Campaign_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_MapRestart_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_MatchReset_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Mutespecs_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Nextmap_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Referee_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_ShuffleTeams_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_ShuffleTeams_NoRestart_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_StartMatch_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_SwapTeams_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_FriendlyFire_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Timelimit_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Warmupfire_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Unreferee_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_AntiLag_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_BalancedTeams_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Surrender_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_RestartCampaign_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_NextCampaign_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Poll_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Config_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_CoinToss_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);

void G_LinkDebris(void);
void G_LinkDamageParents(void);
int EntsThatRadiusCanDamage(vec3_t origin, float radius, int *damagedList);

qboolean G_LandmineTriggered(gentity_t *ent);
qboolean G_LandmineArmed(gentity_t *ent);
qboolean G_LandmineUnarmed(gentity_t *ent);
qboolean G_LandmineSpotted(gentity_t *ent);
qboolean G_AvailableAirstrike(gentity_t *ent);
qboolean G_AvailableArtillery(gentity_t *ent);
void G_AddAirstrikeToCounters(gentity_t *ent);
void G_AddArtilleryToCounters(gentity_t *ent);
int G_MaxAvailableAirstrikes(gentity_t *ent);
int G_MaxAvailableArtillery(gentity_t *ent);

void G_SetTargetName(gentity_t *ent, char *targetname);
void G_KillEnts(const char *target, gentity_t *ignore, gentity_t *killer, meansOfDeath_t mod);
void trap_EngineerTrace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);

int G_CountTeamMedics(team_t team, qboolean alivecheck);
int G_CountTeamFieldops(team_t team);
qboolean G_TankIsOccupied(gentity_t *ent);
qboolean G_TankIsMountable(gentity_t *ent, gentity_t *other);

qboolean G_ConstructionBegun(gentity_t *ent);
qboolean G_ConstructionIsFullyBuilt(gentity_t *ent);
qboolean G_ConstructionIsPartlyBuilt(gentity_t *ent);
gentity_t *G_ConstructionForTeam(gentity_t *toi, team_t team);
gentity_t *G_IsConstructible(team_t team, gentity_t *toi);
qboolean G_EmplacedGunIsRepairable(gentity_t *ent, gentity_t *other);
qboolean G_EmplacedGunIsMountable(gentity_t *ent, gentity_t *other);
void G_CheckForCursorHints(gentity_t *ent);
void G_CalcClientAccuracies(void);
void G_BuildEndgameStats(void);
int G_TeamCount(gentity_t *ent, int weap);

qboolean G_IsFireteamLeader(int entityNum, fireteamData_t **teamNum);
fireteamData_t *G_FindFreePublicFireteam(team_t team);
void G_RegisterFireteam(int entityNum);

void weapon_callAirStrike(gentity_t *ent);
void weapon_checkAirStrikeThink(gentity_t *ent);
void weapon_callSecondPlane(gentity_t *ent);
qboolean weapon_checkAirStrike(gentity_t *ent);
void weapon_smokeBombExplode(gentity_t *ent);

void DynaSink(gentity_t *self);
void DynaFree(gentity_t *self);
void G_ChainFree(gentity_t *self);

void G_MakeReady(gentity_t *ent);
void G_MakeUnready(gentity_t *ent);

void SetPlayerSpawn(gentity_t *ent, int spawn, qboolean update);
void G_UpdateSpawnPointState(gentity_t *ent);
void G_UpdateSpawnPointStatePlayerCounts(void);

void G_SetConfigStringValue(int num, const char *key, const char *value);
void G_GlobalClientEvent(entity_event_t event, int param, int client);

void G_InitTempTraceIgnoreEnts(void);
void G_ResetTempTraceIgnoreEnts(void);
void G_TempTraceIgnoreEntity(gentity_t *ent);
void G_TempTraceIgnoreBodies(void);
void G_TempTraceIgnorePlayersAndBodies(void);
void G_TempTraceIgnorePlayersFromTeam(team_t team);
void G_TempTraceRealHitBox(gentity_t *ent);
void G_ResetTempTraceRealHitBox(void);

qboolean G_CanPickupWeapon(weapon_t weapon, gentity_t *ent);

qboolean G_LandmineSnapshotCallback(int entityNum, int clientNum);

// Spawnflags

// trigger_objective_info spawnflags (objective info display)
#define OBJECTIVE_INFO_AXIS_OBJECTIVE       1
#define OBJECTIVE_INFO_ALLIED_OBJECTIVE     2
#define OBJECTIVE_INFO_MESSAGE_OVERRIDE     4
#define OBJECTIVE_INFO_TANK                 8
#define OBJECTIVE_INFO_IS_OBJECTIVE         16  ///<(see cg_commandmap - TODO: make these available to client)
#define OBJECTIVE_INFO_IS_HEALTHAMMOCABINET 32  ///<(see cg_commandmap)
#define OBJECTIVE_INFO_IS_COMMANDPOST       64  ///<(see cg_commandmap)
///< 128 is disabled

// script_mover spawnflags
#define MOVER_TRIGGERSPAWN              1
#define MOVER_SOLID                     2
#define MOVER_EXPLOSIVEDAMAGEONLY       4
#define MOVER_RESURECTABLE              8
#define MOVER_COMPASS                   16
#define MOVER_ALLIED                    32
#define MOVER_AXIS                      64
#define MOVER_MOUNTEDGUN                128

// func trigger_multiple
#define MULTI_TRIGGER_AXIS_ONLY         1
#define MULTI_TRIGGER_ALLIED_ONLY       2
#define MULTI_TRIGGER_NOBOT             4
#define MULTI_TRIGGER_BOTONLY           8
#define MULTI_TRIGGER_SOLDIERONLY       16
#define MULTI_TRIGGER_FIELDOPSONLY      32
#define MULTI_TRIGGER_MEDICONLY         64
#define MULTI_TRIGGER_ENGINEERONLY      128
#define MULTI_TRIGGER_COVERTOPSONLY     256
#define MULTI_TRIGGER_DISGUISEDSONLY    512  ///< mod specific only
#define MULTI_TRIGGER_OBJECTIVEONLY     1024 ///< mod specific only

#define TARGET_PUSH_BOUNCEPAD           1

// func_door_rotating spawnflags
#define DOOR_ROTATING_TOGGLE            2
#define DOOR_ROTATING_X_AXIS            4
#define DOOR_ROTATING_Y_AXIS            8
#define DOOR_ROTATING_REVERSE           16
#define DOOR_ROTATING_FORCE             32
#define DOOR_ROTATING_STAYOPEN          64

// func_door spawnflags
#define DOOR_START_OPEN                 1
#define DOOR_TOGGLE                     2
#define DOOR_CRUSHER                    4
#define DOOR_TOUCH                      8

// teamplay specific stuff
#define AXIS_OBJECTIVE                   1
#define ALLIED_OBJECTIVE                 2
#define OBJECTIVE_DESTROYED              4

#define CONSTRUCTIBLE_START_BUILT            1
#define CONSTRUCTIBLE_INVULNERABLE           2
#define AXIS_CONSTRUCTIBLE                   4
#define ALLIED_CONSTRUCTIBLE                 8
#define CONSTRUCTIBLE_BLOCK_PATHS_WHEN_BUILD 16
#define CONSTRUCTIBLE_NO_AAS_BLOCKING        32
//#define CONSTRUCTIBLE_AAS_SCRIPTED           64

#define EXPLOSIVE_START_INVIS            1
#define EXPLOSIVE_TOUCHABLE              2
#define EXPLOSIVE_USESHADER              4
#define EXPLOSIVE_LOWGRAV                8
//#define EXPLOSIVE_NO_AAS_BLOCKING        16 // obsolete (might be used in older maps)
#define EXPLOSIVE_TANK                   32

// MAPVOTE - used when mapvoting is enabled
#define MAPVOTE_TIE_LEASTPLAYED  1
#define MAPVOTE_ALT_INTERMISSION 2 ///< unused
#define MAPVOTE_MULTI_VOTE       4
#define MAPVOTE_NO_RANDOMIZE     8 ///< unused
#define MAPVOTE_NEXTMAP_VOTEMAP  16


// Spawnflags end

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
	int ofs;
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

// MAPVOTE
void G_MapVoteInfoWrite(void);
void G_MapVoteInfoRead(void);

// g_misc flags
#define G_MISC_SHOVE_Z                 BIT(0)
// BIT(1) unused
// BIT(2) unused
#define G_MISC_CROSSHAIR_DYNAMITE      BIT(3)
#define G_MISC_CROSSHAIR_LANDMINE      BIT(4)

// g_voting flags
#define VOTEF_USE_TOTAL_VOTERS      1   ///< use total voters instead of total players to decide if a vote passes
#define VOTEF_NO_POPULIST_PENALTY   2   ///< successful votes do not count against vote_limit
#define VOTEF_DISP_CALLER           4   ///< append "(called by name)" in vote string

void G_RailTrail(vec_t *start, vec_t *end, vec_t *color);
void G_RailBox(vec_t *origin, vec_t *mins, vec_t *maxs, vec_t *color, int index);

/**
 * @struct weapFireTable_s
 * @typedef weapFireFunction_t
 * @brief
 */
typedef struct weapFireTable_t
{
	weapon_t weapon;
	gentity_t *(*fire)(gentity_t * ent);  ///< -
	void (*think)(gentity_t *ent);        ///< -
	void (*free)(gentity_t *ent);         ///< -
	int eType;                            ///< -
	int eFlags;                           ///< -
	int svFlags;                          ///< -
	int contents;                         ///< -
	int trType;                           ///< -
	int trTime;                           ///< -
	float boundingBox[2][3];              ///< - mins / maxs bounding box vectors (for missile ent)
	float hitBox[2][3];                   ///< - mins / maxs hit box vectors (for missile ent)
	int clipMask;                         ///< -
	int nextThink;                        ///< -
	int accuracy;                         ///< -
	int health;                           ///< -
	int timeStamp;                        ///< -
	int impactDamage;                     ///< - func_explosives damage, probably adjust this based on velocity for throwable missile

} weapFireTable_t;

// Lookup table to find weapon fire properties
extern weapFireTable_t weapFireTable[WP_NUM_WEAPONS];
#define GetWeaponFireTableData(weapIndex) ((weapFireTable_t *)(&weapFireTable[weapIndex]))

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
