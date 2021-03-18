/**
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
 *
 * @file ui_public.h
 */

#ifndef INCLUDE_UI_PUBLIC_H
#define INCLUDE_UI_PUBLIC_H

#define UI_API_VERSION  4

/**
 * @struct uiClientState_t
 * @brief
 */
typedef struct
{
	connstate_t connState;
	int connectPacketCount;
	int clientNum;
	char servername[MAX_STRING_CHARS];
	char updateInfoString[MAX_STRING_CHARS];
	char messageString[MAX_STRING_CHARS];
} uiClientState_t;

/**
 * @enum uiImport_t
 * @brief
 */
typedef enum
{
	UI_ERROR,
	UI_PRINT,
	UI_MILLISECONDS,
	UI_CVAR_SET,
	UI_CVAR_VARIABLEVALUE,
	UI_CVAR_VARIABLESTRINGBUFFER,
	UI_CVAR_LATCHEDVARIABLESTRINGBUFFER,
	UI_CVAR_SETVALUE,
	UI_CVAR_RESET,
	UI_CVAR_CREATE,
	UI_CVAR_INFOSTRINGBUFFER,
	UI_ARGC,
	UI_ARGV,
	UI_CMD_EXECUTETEXT,
	UI_ADDCOMMAND,
	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_FS_DELETEFILE,
	UI_FS_COPYFILE,
	UI_R_REGISTERMODEL,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADERNOMIP,
	UI_R_CLEARSCENE,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDPOLYSTOSCENE,

	UI_R_ADDLIGHTTOSCENE,

	UI_R_ADDCORONATOSCENE,

	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAW2DPOLYS,
	UI_R_DRAWSTRETCHPIC,
	UI_R_DRAWROTATEDPIC,
	UI_UPDATESCREEN,                        ///< 30
	UI_CM_LERPTAG,
	UI_CM_LOADMODEL,
	UI_S_REGISTERSOUND,
	UI_S_STARTLOCALSOUND,
	UI_S_FADESTREAMINGSOUND,
	UI_S_FADEALLSOUNDS,
	UI_KEY_KEYNUMTOSTRINGBUF,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_BINDINGTOKEYS,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,
	UI_KEY_SETCATCHER,
	UI_GETCLIPBOARDDATA,
	UI_GETGLCONFIG,
	UI_GETCLIENTSTATE,
	UI_GETCONFIGSTRING,
	UI_LAN_GETLOCALSERVERCOUNT,
	UI_LAN_GETLOCALSERVERADDRESSSTRING,
	UI_LAN_GETGLOBALSERVERCOUNT,            ///< 50
	UI_LAN_GETGLOBALSERVERADDRESSSTRING,
	UI_LAN_GETPINGQUEUECOUNT,
	UI_LAN_CLEARPING,
	UI_LAN_GETPING,
	UI_LAN_GETPINGINFO,
	UI_CVAR_REGISTER,
	UI_CVAR_UPDATE,
	UI_MEMORY_REMAINING,

	UI_GET_CDKEY,                           ///< do not remove
	UI_SET_CDKEY,                           ///< do not remove
	UI_R_REGISTERFONT,
	UI_R_MODELBOUNDS,
	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_REMOVE_ALL_GLOBAL_DEFINES,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,
	UI_PC_SOURCE_FILE_AND_LINE,
	UI_PC_UNREAD_TOKEN,
	UI_S_STOPBACKGROUNDTRACK,
	UI_S_STARTBACKGROUNDTRACK,
	UI_REAL_TIME,
	UI_LAN_GETSERVERCOUNT,
	UI_LAN_GETSERVERADDRESSSTRING,
	UI_LAN_GETSERVERINFO,
	UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_LAN_RESETPINGS,
	UI_LAN_LOADCACHEDSERVERS,
	UI_LAN_SAVECACHEDSERVERS,
	UI_LAN_ADDSERVER,
	UI_LAN_REMOVESERVER,
	UI_CIN_PLAYCINEMATIC,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS,
	UI_R_REMAP_SHADER,
	UI_VERIFY_CDKEY,                        ///< do not remove
	UI_LAN_SERVERSTATUS,
	UI_LAN_GETSERVERPING,
	UI_LAN_SERVERISVISIBLE,
	UI_LAN_COMPARESERVERS,
	UI_LAN_SERVERISINFAVORITELIST,
	UI_CL_GETLIMBOSTRING,
	UI_SET_PBCLSTATUS,
	UI_CHECKAUTOUPDATE,
	UI_GET_AUTOUPDATE,
	UI_CL_TRANSLATE_STRING,
	UI_OPENURL,
	UI_SET_PBSVSTATUS,

	UI_MEMSET = 200,
	UI_MEMCPY,
	UI_STRNCPY,
	UI_SIN,
	UI_COS,
	UI_ATAN2,
	UI_SQRT,
	UI_FLOOR,
	UI_CEIL,
	UI_GETHUNKDATA,

#if !defined(UIDLL) && !defined(CGAMEDLL)
	UI_TRAP_GETVALUE = COM_TRAP_GETVALUE,
#endif

} uiImport_t;

// Number of columns in the server list
#define SORT_HOST           0
#define SORT_MAP            1
#define SORT_CLIENTS        2
#define SORT_GAME           3
#define SORT_PING           4
#define SORT_FILTERS        5
#define SORT_FAVOURITES     6

/**
 * @enum uiExport_t
 * @brief
 */
typedef enum
{
	UI_GETAPIVERSION = 0,   ///< system reserved

	UI_INIT,
	///< void	UI_Init( void );

	UI_SHUTDOWN,
	///< void	UI_Shutdown( void );

	UI_KEY_EVENT,
	///< void	UI_KeyEvent( int key );

	UI_MOUSE_EVENT,
	///< void	UI_MouseEvent( int dx, int dy );

	UI_REFRESH,
	///< void	UI_Refresh( int time );

	UI_IS_FULLSCREEN,
	///< qboolean UI_IsFullscreen( void );

	UI_SET_ACTIVE_MENU,
	///< void	UI_SetActiveMenu( uiMenuCommand_t menu );

	UI_GET_ACTIVE_MENU,
	///< void	UI_GetActiveMenu( void );

	UI_CONSOLE_COMMAND,
	///< qboolean UI_ConsoleCommand( void );

	UI_DRAW_CONNECT_SCREEN,
	///< void	UI_DrawConnectScreen( qboolean overlay );
	UI_HASUNIQUECDKEY, ///< do not remove
	///< if !overlay, the background will be drawn, otherwise it will be
	///< overlayed over whatever the cgame has drawn.
	///< a GetClientState syscall will be made to get the current strings
	UI_CHECKEXECKEY,

	UI_WANTSBINDKEYS,

} uiExport_t;

#endif // #ifndef INCLUDE_UI_PUBLIC_H
