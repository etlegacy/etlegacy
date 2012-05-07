/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file botlib.h
 * @brief bot AI library
 */

#define BOTLIB_API_VERSION      2

struct aas_entityinfo_s;
struct bot_consolemessage_s;
struct bot_match_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;
struct weaponinfo_s;

#define MAX_DEBUGPOLYS      4096

typedef struct bot_debugpoly_s
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
} bot_debugpoly_t;

typedef void (*BotPolyFunc)(int color, int numPoints, float *points);

// RF, these need to be here so the botlib also knows how many bot game entities there are
#define NUM_BOTGAMEENTITIES 384

#define BLOCKINGFLAG_MOVER  (~0x7fffffff)

//Print types
#define PRT_MESSAGE             1
#define PRT_WARNING             2
#define PRT_ERROR               3
#define PRT_FATAL               4
#define PRT_EXIT                5

//botlib error codes
#define BLERR_NOERROR                   0   //no error
#define BLERR_LIBRARYNOTSETUP           1   //library not setup
#define BLERR_LIBRARYALREADYSETUP       2   //BotSetupLibrary: library already setup
#define BLERR_INVALIDCLIENTNUMBER       3   //invalid client number
#define BLERR_INVALIDENTITYNUMBER       4   //invalid entity number
#define BLERR_NOAASFILE                 5   //BotLoadMap: no AAS file available
#define BLERR_CANNOTOPENAASFILE         6   //BotLoadMap: cannot open AAS file
#define BLERR_CANNOTSEEKTOAASFILE       7   //BotLoadMap: cannot seek to AAS file
#define BLERR_CANNOTREADAASHEADER       8   //BotLoadMap: cannot read AAS header
#define BLERR_WRONGAASFILEID            9   //BotLoadMap: incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION       10  //BotLoadMap: incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP         11  //BotLoadMap: cannot read AAS file lump
#define BLERR_NOBSPFILE                 12  //BotLoadMap: no BSP file available
#define BLERR_CANNOTOPENBSPFILE         13  //BotLoadMap: cannot open BSP file
#define BLERR_CANNOTSEEKTOBSPFILE       14  //BotLoadMap: cannot seek to BSP file
#define BLERR_CANNOTREADBSPHEADER       15  //BotLoadMap: cannot read BSP header
#define BLERR_WRONGBSPFILEID            16  //BotLoadMap: incorrect BSP file id
#define BLERR_WRONGBSPFILEVERSION       17  //BotLoadMap: incorrect BSP file version
#define BLERR_CANNOTREADBSPLUMP         18  //BotLoadMap: cannot read BSP file lump
#define BLERR_AICLIENTNOTSETUP          19  //BotAI: client not setup
#define BLERR_AICLIENTALREADYSETUP      20  //BotSetupClient: client already setup
#define BLERR_AIMOVEINACTIVECLIENT      21  //BotMoveClient: cannot move inactive client
#define BLERR_AIMOVETOACTIVECLIENT      22  //BotMoveClient: cannot move to active client
#define BLERR_AICLIENTALREADYSHUTDOWN   23  //BotShutdownClient: client not setup
#define BLERR_AIUPDATEINACTIVECLIENT    24  //BotUpdateClient: called for inactive client
#define BLERR_AICMFORINACTIVECLIENT     25  //BotConsoleMessage: called for inactive client
#define BLERR_SETTINGSINACTIVECLIENT    26  //BotClientSettings: called for inactive client
#define BLERR_CANNOTLOADICHAT           27  //BotSetupClient: cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS     28  //BotSetupClient: cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG      29  //BotSetupLibrary: cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS   30  //BotSetupClient: cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG    31  //BotSetupLibrary: cannot load weapon config
#define BLERR_INVALIDSOUNDINDEX         32  //BotAddSound: invalid sound index value

//action flags
#define ACTION_ATTACK           1
#define ACTION_USE              2
#define ACTION_RESPAWN          4
#define ACTION_JUMP             8
#define ACTION_MOVEUP           8
#define ACTION_CROUCH           16
#define ACTION_MOVEDOWN         16
#define ACTION_MOVEFORWARD      32
#define ACTION_MOVEBACK         64
#define ACTION_MOVELEFT         128
#define ACTION_MOVERIGHT        256
#define ACTION_DELAYEDJUMP      512
#define ACTION_TALK             1024
#define ACTION_GESTURE          2048
#define ACTION_WALK             4096
#define ACTION_RELOAD           8192
// START    xkan, 9/16/2002
#define ACTION_PRONE            16384
// END      xkan, 9/16/2002

//the bot input, will be converted to an usercmd_t
typedef struct bot_input_s
{
	float thinktime;        //time since last output (in seconds)
	vec3_t dir;             //movement direction
	float speed;            //speed in the range [0, 400]
	vec3_t viewangles;      //the view angles
	int actionflags;        //one of the ACTION_? flags
	int weapon;             //weapon to use
} bot_input_t;

//entity state
typedef struct bot_entitystate_s
{
	int type;               // entity type
	int flags;              // entity flags
	vec3_t origin;          // origin of the entity
	vec3_t angles;          // angles of the model
	vec3_t old_origin;      // for lerping
	vec3_t mins;            // bounding box minimums
	vec3_t maxs;            // bounding box maximums
	int groundent;          // ground entity
	int solid;              // solid type
	int modelindex;         // model used
	int modelindex2;        // weapons, CTF flags, etc
	int frame;              // model frame number
	int event;              // impulse events -- muzzle flashes, footsteps, etc
	int eventParm;          // even parameter
	int powerups;           // bit flags
	int weapon;             // determines weapon and flash model, etc
	int legsAnim;           // mask off ANIM_TOGGLEBIT
	int torsoAnim;          // mask off ANIM_TOGGLEBIT
} bot_entitystate_t;


// from be_aas.h
//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	qboolean allsolid;          // if true, plane is not valid
	qboolean startsolid;        // if true, the initial point was in a solid area
	float fraction;             // time completed, 1.0 = didn't hit anything
	vec3_t endpos;              // final position
	cplane_t plane;             // surface normal at impact
	float exp_dist;             // expanded plane distance
	int sidenum;                // number of the brush side hit
	bsp_surface_t surface;      // the hit point surface
	int contents;               // contents on other side of surface hit
	int ent;                    // number of entity hit
} bsp_trace_t;
// end be_aas.h

//bot AI library exported functions
typedef struct botlib_import_s
{
	//print messages from the bot library
	void (QDECL *Print)(int type, char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
	//trace a bbox through the world
	void (*Trace)(bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
	//trace a bbox against a specific entity
	void (*EntityTrace)(bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask);
	//retrieve the contents at the given point
	int (*PointContents)(vec3_t point);
	//check if the point is in potential visible sight
	int (*inPVS)(vec3_t p1, vec3_t p2);
	//retrieve the BSP entity data lump
	char *(*BSPEntityData)(void);
	//
	void (*BSPModelMinsMaxsOrigin)(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
	//send a bot client command
	void (*BotClientCommand)(int client, char *command);
	//memory allocation
	void *(*GetMemory)(int size);
	void (*FreeMemory)(void *ptr);
	void (*FreeZoneMemory)(void);
	void *(*HunkAlloc)(int size);
	//file system access
	int (*FS_FOpenFile)(const char *qpath, fileHandle_t *file, fsMode_t mode);
	int (*FS_Read)(void *buffer, int len, fileHandle_t f);
	int (*FS_Write)(const void *buffer, int len, fileHandle_t f);
	void (*FS_FCloseFile)(fileHandle_t f);
	int (*FS_Seek)(fileHandle_t f, long offset, int origin);
	//debug visualisation stuff
	int (*DebugLineCreate)(void);
	void (*DebugLineDelete)(int line);
	void (*DebugLineShow)(int line, vec3_t start, vec3_t end, int color);
	//
	int (*DebugPolygonCreate)(int color, int numPoints, vec3_t *points);
	bot_debugpoly_t *    (*DebugPolygonGetFree)(void);
	void (*DebugPolygonDelete)(int id);
	void (*DebugPolygonDeletePointer)(bot_debugpoly_t *pPoly);
	//
	// Ridah, Cast AI stuff
	qboolean (*BotVisibleFromPos)(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, qboolean updateVisPos);
	qboolean (*BotCheckAttackAtPos)(int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld);
	// done.
	// Gordon: ability for botlib to check for singleplayer
	// Arnout: removed again, botlibsetup already has a parameter 'singleplayer'
	//qboolean  (*BotGameIsSinglePlayer) ( void );


	// Gordon: direct hookup into rendering, stop using this silly debugpoly faff
	void (*BotDrawPolygon)(int color, int numPoints, float *points);
} botlib_import_t;


typedef struct ea_export_s
{
	//ClientCommand elementary actions
	void (*EA_Say)(int client, char *str);
	void (*EA_SayTeam)(int client, char *str);
	void (*EA_UseItem)(int client, char *it);
	void (*EA_DropItem)(int client, char *it);
	void (*EA_UseInv)(int client, char *inv);
	void (*EA_DropInv)(int client, char *inv);
	void (*EA_Gesture)(int client);
	void (*EA_Command)(int client, char *command);
	//regular elementary actions
	void (*EA_SelectWeapon)(int client, int weapon);
	void (*EA_Talk)(int client);
	void (*EA_Attack)(int client);
	void (*EA_Reload)(int client);
	void (*EA_Use)(int client);
	void (*EA_Respawn)(int client);
	void (*EA_Jump)(int client);
	void (*EA_DelayedJump)(int client);
	void (*EA_Crouch)(int client);
	void (*EA_Walk)(int client);
	void (*EA_MoveUp)(int client);
	void (*EA_MoveDown)(int client);
	void (*EA_MoveForward)(int client);
	void (*EA_MoveBack)(int client);
	void (*EA_MoveLeft)(int client);
	void (*EA_MoveRight)(int client);
	void (*EA_Move)(int client, vec3_t dir, float speed);
	void (*EA_View)(int client, vec3_t viewangles);
	void (*EA_Prone)(int client);
	//send regular input to the server
	void (*EA_EndRegular)(int client, float thinktime);
	void (*EA_GetInput)(int client, float thinktime, bot_input_t *input);
	void (*EA_ResetInput)(int client, bot_input_t *init);
} ea_export_t;



//bot AI library imported functions
typedef struct botlib_export_s
{
	//Elementary Action functions
	ea_export_t ea;

	//setup the bot library, returns BLERR_
	int (*BotLibSetup)(qboolean singleplayer);
	//shutdown the bot library, returns BLERR_
	int (*BotLibShutdown)(void);
	//sets a library variable returns BLERR_
	int (*BotLibVarSet)(char *var_name, char *value);
	//gets a library variable returns BLERR_
	int (*BotLibVarGet)(char *var_name, char *value, int size);

	//sets a C-like define returns BLERR_
	int (*PC_AddGlobalDefine)(char *string);
	void (*PC_RemoveAllGlobalDefines)(void);
	int (*PC_LoadSourceHandle)(const char *filename);
	int (*PC_FreeSourceHandle)(int handle);
	int (*PC_ReadTokenHandle)(int handle, pc_token_t *pc_token);
	int (*PC_SourceFileAndLine)(int handle, char *filename, int *line);
	void (*PC_UnreadLastTokenHandle)(int handle);

	//start a frame in the bot library
	int (*BotLibStartFrame)(float time);
	//load a new map in the bot library
	int (*BotLibLoadMap)(const char *mapname);
	//entity updates
	int (*BotLibUpdateEntity)(int ent, bot_entitystate_t *state);
	//just for testing
	int (*Test)(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);
} botlib_export_t;

//linking of bot library
botlib_export_t *GetBotLibAPI(int apiVersion, botlib_import_t *import);

