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
 * @file g_public.h
 * @brief Game module information visible to server (server.h includes this)
 */

#ifndef INCLUDE_G_PUBLIC_H
#define INCLUDE_G_PUBLIC_H

#define GAME_API_VERSION    8

//===============================================================

typedef qboolean (*addToSnapshotCallback)(int entityNum, int clientNum);

/**
  * @struct entityShared_s
  * @brief entityShared_t
  *
  * @warning Don't add or remove fields to keep 2.60 compatibility
  */
typedef struct
{
	qboolean linked;                ///< qfalse if not in any good cluster
	int linkcount;

	int svFlags;                    ///<  SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;               ///<  only send to this client when SVF_SINGLECLIENT is set

	qboolean bmodel;                ///<  if false, assume an explicit mins / maxs bounding box
	///<  only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;                   ///<  CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
	///<  a non-solid entity should set to 0

	vec3_t absmin, absmax;          ///<  derived from mins/maxs and origin + rotation

	/// currentOrigin will be used for all collision detection and world linking.
	/// it will not necessarily be the same as the trajectory evaluation for the current
	/// time, because each entity must be moved one at a time after time is advanced
	/// to avoid simultanious collision issues
	///
	vec3_t currentOrigin;
	vec3_t currentAngles;

	/// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	/// an ent will be excluded from testing if:
	/// ent->s.number == passEntityNum   (don't interact with self)
	/// ent->s.ownerNum = passEntityNum  (don't interact with your own missiles)
	/// entity[ent->s.ownerNum].ownerNum = passEntityNum (don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;

	int worldflags;

	qboolean snapshotCallback;
} entityShared_t;

/**
  @struct sharedEntity_t
  @brief The server looks at a sharedEntity, which is the start of the game's gentity_t structure
  */
typedef struct
{
	entityState_t s;                ///< communicated by server to clients
	entityShared_t r;               ///< shared by both the server system and game
} sharedEntity_t;

//===============================================================

/**
  * @enum gameImport_t
  * @brief System traps provided by the main engine
  */
typedef enum
{
	/** general Quake services */

	G_PRINT = 0,        ///< ( const char *string );
	///< print message on the local console

	G_ERROR,            ///< ( const char *string );
	///< abort the game

	G_MILLISECONDS,     ///< ( void );
	///< get current time for profiling reasons
	///< this should NOT be used for any game related tasks,
	///< because it is not journaled

	/** console variable interaction */

	G_CVAR_REGISTER,    ///< ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	G_CVAR_UPDATE,      ///< ( vmCvar_t *vmCvar );
	G_CVAR_SET,         ///< ( const char *var_name, const char *value );
	G_CVAR_VARIABLE_INTEGER_VALUE,  ///< ( const char *var_name );

	G_CVAR_VARIABLE_STRING_BUFFER,  ///< ( const char *var_name, char *buffer, int bufsize );

	G_CVAR_LATCHEDVARIABLESTRINGBUFFER,

	G_ARGC,         ///< ( void );
	///< ClientCommand and ServerCommand parameter access

	G_ARGV,         ///< ( int n, char *buffer, int bufferLength );

	G_FS_FOPEN_FILE, ///< ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ,       ///< ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE,      ///< ( const void *buffer, int len, fileHandle_t f );
	G_FS_RENAME,
	G_FS_FCLOSE_FILE,       ///< ( fileHandle_t f );

	G_SEND_CONSOLE_COMMAND, ///< ( const char *text );
	///< add commands to the console as if they were typed in
	///< for map changing, etc


	/** server specific functionality */

	G_LOCATE_GAME_DATA,     ///< ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	///< playerState_t *clients, int sizeofGameClient );
	///< the game needs to let the server system know where and how big the gentities
	///< are, so it can look at them directly without going through an interface

	G_DROP_CLIENT,          ///< ( int clientNum, const char *reason );
	///< kick a client off the server with a message

	G_SEND_SERVER_COMMAND,  ///< ( int clientNum, const char *fmt, ... );
	///< reliably sends a command string to be interpreted by the given
	///< client.  If clientNum is -1, it will be sent to all clients

	G_SET_CONFIGSTRING,     ///< ( int num, const char *string );
	///< config strings hold all the index strings, and various other information
	///< that is reliably communicated to all clients
	///< All of the current configstrings are sent to clients when
	///< they connect, and changes are sent to all connected clients.
	///< All confgstrings are cleared at each level start.

	G_GET_CONFIGSTRING, ///< ( int num, char *buffer, int bufferSize );

	G_GET_USERINFO,     ///< ( int num, char *buffer, int bufferSize );
	///< userinfo strings are maintained by the server system, so they
	///< are persistant across level loads, while all other game visible
	///< data is completely reset

	G_SET_USERINFO,     ///< ( int num, const char *buffer );

	G_GET_SERVERINFO,   ///< ( char *buffer, int bufferSize );
	///< the serverinfo info string has all the cvars visible to server browsers

	G_SET_BRUSH_MODEL,  ///< ( gentity_t *ent, const char *name );
	///< sets mins and maxs based on the brushmodel name

	G_TRACE,            ///< ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	///< collision detection against all linked entities

	G_POINT_CONTENTS,   ///< ( const vec3_t point, int passEntityNum );
	///< point contents against all linked entities

	G_IN_PVS,           ///< ( const vec3_t p1, const vec3_t p2 );

	G_IN_PVS_IGNORE_PORTALS,    ///< ( const vec3_t p1, const vec3_t p2 );

	G_ADJUST_AREA_PORTAL_STATE, ///< ( gentity_t *ent, qboolean open );

	G_AREAS_CONNECTED,  ///< ( int area1, int area2 );

	G_LINKENTITY,       ///< ( gentity_t *ent );
	///< an entity will never be sent to a client or used for collision
	///< if it is not passed to linkentity.  If the size, position, or
	///< solidity changes, it must be relinked.

	G_UNLINKENTITY,     ///< ( gentity_t *ent );
	///< call before removing an interactive entity

	G_ENTITIES_IN_BOX,  ///< ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	///< EntitiesInBox will return brush models based on their bounding box,
	///< so exact determination must still be done with EntityContact

	G_ENTITY_CONTACT,   ///< ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	///< perform an exact check against inline brush models of non-square shape

/// @todo FIXME: precompiler macros for engine ?
// #ifdef FEATURE_OMNIBOT
	G_BOT_ALLOCATE_CLIENT = 36, ///< ( int clientNum ); -  used by OmniBot
// #endif

	G_GET_USERCMD = 38, ///< ( int clientNum, usercmd_t *cmd )

	G_GET_ENTITY_TOKEN, ///< qboolean ( char *buffer, int bufferSize )
	///< Retrieves the next string token from the entity spawn text, returning
	///< false when all tokens have been parsed.
	///< This should only be done at GAME_INIT time.

	G_FS_GETFILELIST,
	G_DEBUG_POLYGON_CREATE,
	G_DEBUG_POLYGON_DELETE,
	G_REAL_TIME,
	G_SNAPVECTOR,

	G_TRACECAPSULE, ///< ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	///< collision detection using capsule against all linked entities

	G_ENTITY_CONTACTCAPSULE,    ///< ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	///< perform an exact check against inline brush models of non-square shape

	G_GETTAG,

	G_REGISTERTAG,      ///< load a serverside tag

	G_REGISTERSOUND,    ///< register the sound
	G_GET_SOUND_LENGTH, ///< get the length of the sound

	// 200
	BOTLIB_SETUP = 200, ///< returns  0 - dummy trap keep this for 'etmain' compatibility
	BOTLIB_SHUTDOWN,    ///< returns -1 - dummy trap keep this for 'etmain' compatibility
	BOTLIB_LIBVAR_SET,  ///< returns  0 - dummy trap keep this for 'etmain' compatibility
	BOTLIB_LIBVAR_GET,  ///< returns  0 - dummy trap keep this for 'etmain' compatibility

	BOTLIB_GET_CONSOLE_MESSAGE = 210,    ///< ( int client, char *message, int size );
	BOTLIB_USER_COMMAND        = 211,    ///< ( int client, usercmd_t *ucmd );

	// 400
	BOTLIB_EA_COMMAND = 407,

	// 500
	/** files */
	BOTLIB_PC_LOAD_SOURCE = 579,
	BOTLIB_PC_FREE_SOURCE,
	BOTLIB_PC_READ_TOKEN,
	BOTLIB_PC_SOURCE_FILE_AND_LINE,
	BOTLIB_PC_UNREAD_TOKEN,

	PB_STAT_REPORT = 584, ///< don't remove, vanilla clients might call this

	G_SENDMESSAGE = 585,
	G_MESSAGESTATUS,

#ifndef GAMEDLL
	// engine extensions
	G_TRAP_GETVALUE = COM_TRAP_GETVALUE
#endif

} gameImport_t;


/**
  * @enum gameExport_t
  * @brief Functions exported by the game subsystem
  */
typedef enum
{
	GAME_INIT = 0,  ///< ( int levelTime, int randomSeed, int restart );
	///< init and shutdown will be called every single level
	///< The game should call G_GET_ENTITY_TOKEN to parse through all the
	///< entity configuration text and spawn gentities.

	GAME_SHUTDOWN,  ///< (void);

	GAME_CLIENT_CONNECT,    ///< ( int clientNum, qboolean firstTime, qboolean isBot );
	///< return NULL if the client is allowed to connect, otherwise return
	///< a text string with the reason for denial

	GAME_CLIENT_BEGIN,              ///< ( int clientNum );

	GAME_CLIENT_USERINFO_CHANGED,   ///< ( int clientNum );

	GAME_CLIENT_DISCONNECT,         ///< ( int clientNum );

	GAME_CLIENT_COMMAND,            ///< ( int clientNum );

	GAME_CLIENT_THINK,              ///< ( int clientNum );

	GAME_RUN_FRAME,                 ///< ( int levelTime );

	GAME_CONSOLE_COMMAND,           ///< ( void );
	///< ConsoleCommand will be called when a command has been issued
	///< that is not recognized as a builtin function.
	///< The game can issue trap_argc() / trap_argv() commands to get the command
	///< and parameters.  Return qfalse if the game doesn't recognize it as a command.

	GAME_SNAPSHOT_CALLBACK = 10,    ///< ( int entityNum, int clientNum ); // return qfalse if you don't want it to be added

	/** @note These gameExport_t are not used anymore - kept as reminder */
	// BOTAI_START_FRAME,
	// BOT_VISIBLEFROMPOS,
	// BOT_CHECKATTACKATPOS,

	GAME_MESSAGERECEIVED = 14,      ///< ( int cno, const char *buf, int buflen, int commandTime );

} gameExport_t;

#endif // #ifndef INCLUDE_G_PUBLIC_H
