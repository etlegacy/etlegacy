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

#define MAX_DEBUGPOLYS      4096

typedef struct bot_debugpoly_s
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
} bot_debugpoly_t;

typedef void (*BotPolyFunc)(int color, int numPoints, float *points);

//Print types
#define PRT_MESSAGE             1
#define PRT_WARNING             2
#define PRT_ERROR               3
#define PRT_FATAL               4
#define PRT_EXIT                5

//botlib error codes
#define BLERR_NOERROR                   0   //no error
#define BLERR_LIBRARYNOTSETUP           1   //library not setup

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
	// Gordon: direct hookup into rendering, stop using this silly debugpoly faff
	void (*BotDrawPolygon)(int color, int numPoints, float *points);
} botlib_import_t;

//bot AI library imported functions
typedef struct botlib_export_s
{
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
