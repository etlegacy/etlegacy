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
 * @file cg_public.h
 */

#ifndef INCLUDE_CG_PUBLIC_H
#define INCLUDE_CG_PUBLIC_H

/// Allow a lot of command backups for very fast systems
/// multiple commands may be combined into a single packet, so this
/// needs to be larger than PACKET_BACKUP
#define CMD_BACKUP          64
#define CMD_MASK            (CMD_BACKUP - 1)

#define MAX_ENTITIES_IN_SNAPSHOT    512

/**
 * @struct snapshot_s
 *
 * @brief Snapshots are a view of the server at a given time.
 *
 * @details Snapshots are generated at regular time intervals by the server,
 * but they may not be sent if a client's rate level is exceeded, or
 * they may be dropped by the network.
 */
typedef struct
{
	int snapFlags;                      ///< SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;                     ///< server time the message is valid for (in msec)

	byte areamask[MAX_MAP_AREA_BYTES];  ///< portalarea visibility bits

	playerState_t ps;                   ///< complete information about the current player at this time

	int numEntities;                    ///< all of the entities that need to be presented
	entityState_t entities[MAX_ENTITIES_IN_SNAPSHOT];   ///< at the time of this snapshot

	int numServerCommands;              ///< text based server commands to execute when this
	int serverCommandSequence;          ///< snapshot becomes current
} snapshot_t;

/**
 * @enum cgameEvent_e
 * @typedef cgameEvent_t
 * @brief
 */
typedef enum cgameEvent_e
{
	CGAME_EVENT_NONE,
	CGAME_EVENT_GAMEVIEW,
	CGAME_EVENT_SPEAKEREDITOR,
	CGAME_EVENT_CAMPAIGNBREIFING,
	CGAME_EVENT_DEMO,
	CGAME_EVENT_FIRETEAMMSG,
	CGAME_EVENT_SHOUTCAST,
	CGAME_EVENT_SPAWNPOINTMSG,
	CGAME_EVENT_MULTIVIEW
} cgameEvent_t;

#define CGAME_IMPORT_API_VERSION    3

/**
 * @enum cgameImport_t
 * @brief Functions imported from the main executable
 */
typedef enum
{
	CG_PRINT = 0,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_VARIABLESTRINGBUFFER,
	CG_CVAR_LATCHEDVARIABLESTRINGBUFFER,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,                    ///< 10
	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_FS_GETFILELIST,
	CG_FS_DELETEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,            ///< 20
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_LOADMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,

	CG_CM_CAPSULETRACE,         ///< 30
	CG_CM_TRANSFORMEDCAPSULETRACE,
	CG_CM_TEMPCAPSULEMODEL,

	CG_CM_MARKFRAGMENTS,
	CG_R_PROJECTDECAL,          ///< projects a decal onto brush models
	CG_R_CLEARDECALS,           ///< clears world/entity decals
	CG_S_STARTSOUND,
	CG_S_STARTSOUNDEX,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_CLEARSOUNDS,           ///< 40
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,

	CG_S_GETVOICEAMPLITUDE,     ///< talking animations

	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_S_FADESTREAMINGSOUND,
	CG_S_FADEALLSOUNDS,         ///< added for fading out everything
	CG_S_STARTSTREAMINGSOUND,
	CG_S_GETSOUNDLENGTH,        ///< 50 get the length (in milliseconds) of the sound
	CG_S_GETCURRENTSOUNDTIME,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,

	CG_R_GETSKINMODEL,          ///< client allowed to view what the .skin loaded so they can set their model appropriately
	CG_R_GETMODELSHADER,        ///< client allowed the shader handle for given model/surface (for things like debris inheriting shader from explosive)

	CG_R_REGISTERFONT,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,   ///< 60
	CG_GET_ENTITY_TOKEN,
	CG_R_ADDPOLYTOSCENE,

	CG_R_ADDPOLYSTOSCENE,

	CG_R_ADDPOLYBUFFERTOSCENE,
	CG_R_ADDLIGHTTOSCENE,

	CG_R_ADDCORONATOSCENE,
	CG_R_SETFOG,
	CG_R_SETGLOBALFOG,

	CG_R_RENDERSCENE,
	CG_R_SAVEVIEWPARMS,         ///< 70
	CG_R_RESTOREVIEWPARMS,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_DRAWSTRETCHPIC_GRADIENT,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,             ///< 80
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_SETCLIENTLERPORIGIN,
	CG_R_REGISTERSHADERNOMIP,
	CG_MEMORY_REMAINING,

	CG_KEY_ISDOWN,
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,          ///< 90
	CG_KEY_GETKEY,
	CG_KEY_GETOVERSTRIKEMODE,
	CG_KEY_SETOVERSTRIKEMODE,

	CG_PC_ADD_GLOBAL_DEFINE,
	CG_PC_LOAD_SOURCE,
	CG_PC_FREE_SOURCE,
	CG_PC_READ_TOKEN,
	CG_PC_SOURCE_FILE_AND_LINE,
	CG_PC_UNREAD_TOKEN,

	CG_S_STOPBACKGROUNDTRACK,   ///< 100
	CG_REAL_TIME,
	CG_SNAPVECTOR,
	CG_REMOVECOMMAND,
	CG_R_LIGHTFORPOINT,         ///< unused

	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,
	CG_R_REMAP_SHADER,          ///< 110
	CG_S_ADDREALLOOPINGSOUND,
	CG_S_STOPSTREAMINGSOUND,

	CG_LOADCAMERA,              ///< unused
	CG_STARTCAMERA,             ///< unused
	CG_STOPCAMERA,              ///< unused
	CG_GETCAMERAINFO,           ///< unused

	CG_MEMSET = 150,
	CG_MEMCPY,
	CG_STRNCPY,
	CG_SIN,
	CG_COS,
	CG_ATAN2,
	CG_SQRT,
	CG_FLOOR,
	CG_CEIL,

	CG_TESTPRINTINT,
	CG_TESTPRINTFLOAT,          ///< 160
	CG_ACOS,

	CG_INGAME_POPUP,

	CG_INGAME_CLOSEPOPUP,

	CG_R_DRAWROTATEDPIC,
	CG_R_DRAW2DPOLYS,

	CG_KEY_GETBINDINGBUF,
	CG_KEY_SETBINDING,
	CG_KEY_KEYNUMTOSTRINGBUF,
	CG_KEY_BINDINGTOKEYS,

	CG_TRANSLATE_STRING,        ///< 170

	CG_R_INPVS,
	CG_GETHUNKDATA,

	CG_PUMPEVENTLOOP,

	CG_SENDMESSAGE,
	CG_MESSAGESTATUS,

	CG_R_LOADDYNAMICSHADER,

	CG_R_RENDERTOTEXTURE,

	CG_R_GETTEXTUREID,

	CG_R_FINISH,                ///< 179

#ifndef CGAMEDLL
	CG_TRAP_GETVALUE = COM_TRAP_GETVALUE,
#endif

} cgameImport_t;

/**
 * @enum cgameExport_t
 * @brief Functions exported to the main executable
 */
typedef enum
{
	CG_INIT = 0,
	///<  void CG_Init( int serverMessageNum, int serverCommandSequence )
	///< called when the level loads or when the renderer is restarted
	///< all media should be registered at this time
	///< cgame will display loading status by calling SCR_Update, which
	///< will call CG_DrawInformation during the loading process
	///< reliableCommandSequence will be 0 on fresh loads, but higher for
	///< demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
	///<  void (*CG_Shutdown)( void );
	///< oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
	///<  qboolean (*CG_ConsoleCommand)( void );
	///< a console command has been issued locally that is not recognized by the
	///< main game system.
	///< use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	///< command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
	///<  void (*CG_DrawActiveFrame)( int serverTime, qboolean demoPlayback );
	///< Generates and draws a game scene and status information at the given time.
	///< If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
	///<  int (*CG_CrosshairPlayer)( void );

	CG_LAST_ATTACKER,
	///<  int (*CG_LastAttacker)( void );

	CG_KEY_EVENT,
	///<  void    (*CG_KeyEvent)( int key, qboolean down );

	CG_MOUSE_EVENT,
	///<  void    (*CG_MouseEvent)( int dx, int dy );
	CG_EVENT_HANDLING,
	///<  void (*CG_EventHandling)(int type, qboolean fForced);

	CG_GET_TAG,
	///<  qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or );

	CG_CHECKEXECKEY,

	CG_WANTSBINDKEYS,

	CG_MESSAGERECEIVED,
	///<  void (*CG_MessageReceived)( const char *buf, int buflen, int serverTime );

} cgameExport_t;

#endif // #ifndef INCLUDE_CG_PUBLIC_H
