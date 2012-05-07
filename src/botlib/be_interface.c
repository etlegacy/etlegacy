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
 * @file be_interface.c
 * @brief bot library interface
 */

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"

#include "../botlib/botlib.h"
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "be_interface.h"

//library globals in a structure
botlib_globals_t botlibglobals;

botlib_export_t be_botlib_export;
botlib_import_t botimport;

//qtrue if the library is setup
int botlibsetup = qfalse;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
// Ridah, faster Win32 code
#ifdef _WIN32
#undef MAX_PATH     // this is an ugly hack, to temporarily ignore the current definition, since it's also defined in windows.h
#include <windows.h>
#undef MAX_PATH
#define MAX_PATH    MAX_QPATH
#endif

int Sys_MilliSeconds(void)
{
// Ridah, faster Win32 code
#ifdef _WIN32
	int             sys_curtime;
	static qboolean initialized = qfalse;
	static int      sys_timeBase;

	if (!initialized)
	{
		sys_timeBase = timeGetTime();
		initialized  = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
#else
	return clock() * 1000 / CLOCKS_PER_SEC;
#endif
} //end of the function Sys_MilliSeconds


//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
qboolean BotLibSetup(char *str)
{
	if (!botlibglobals.botlibsetup)
	{
		botimport.Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotLibSetup

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
extern define_t *globaldefines;
int Export_BotLibSetup(qboolean singleplayer)
{
	int errnum;

	//initialize byte swapping (litte endian etc.)
	Log_Open("botlib.log");
	//
	botimport.Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");
	//
	botlibglobals.maxclients  = (int) LibVarValue("maxclients", "128");
	botlibglobals.maxentities = (int) LibVarValue("maxentities", "1024");


	errnum = EA_Setup();            //be_ea.c
	if ( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	globaldefines = NULL;

	botlibsetup               = qtrue;
	botlibglobals.botlibsetup = qtrue;

	return BLERR_NOERROR;
} //end of the function Export_BotLibSetup

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int Export_BotLibShutdown(void)
{
	static int recursive = 0;

	if (!BotLibSetup("BotLibShutdown"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}
	//
	if (recursive)
	{
		return BLERR_NOERROR;
	}
	recursive = 1;

	EA_Shutdown();

	// free all libvars
	LibVarDeAllocAll();
	// remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();
	// shut down library log file
	Log_Shutdown();
	//
	botlibsetup               = qfalse;
	botlibglobals.botlibsetup = qfalse;
	recursive                 = 0;
	// print any files still open
	PC_CheckOpenSourceHandles();
	//
#ifdef _DEBUG
	Log_AlwaysOpen("memory.log");
	PrintMemoryLabels();
	Log_Shutdown();
#endif
	return BLERR_NOERROR;
} //end of the function Export_BotLibShutdown

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int Export_BotLibVarSet(char *var_name, char *value)
{
	LibVarSet(var_name, value);
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarSet

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int Export_BotLibVarGet(char *var_name, char *value, int size)
{
	char *varvalue;

	varvalue = LibVarGetString(var_name);
	strncpy(value, varvalue, size - 1);
	value[size - 1] = '\0';
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarGet


/*
============
Init_EA_Export
============
*/
static void Init_EA_Export( ea_export_t *ea ) {
	//ClientCommand elementary actions
	ea->EA_Say = EA_Say;
	ea->EA_SayTeam = EA_SayTeam;
	ea->EA_UseItem = EA_UseItem;
	ea->EA_DropItem = EA_DropItem;
	ea->EA_UseInv = EA_UseInv;
	ea->EA_DropInv = EA_DropInv;
	ea->EA_Gesture = EA_Gesture;
	ea->EA_Command = EA_Command;
	ea->EA_SelectWeapon = EA_SelectWeapon;
	ea->EA_Talk = EA_Talk;
	ea->EA_Attack = EA_Attack;
	ea->EA_Reload = EA_Reload;
	ea->EA_Use = EA_Use;
	ea->EA_Respawn = EA_Respawn;
	ea->EA_Jump = EA_Jump;
	ea->EA_DelayedJump = EA_DelayedJump;
	ea->EA_Crouch = EA_Crouch;
	ea->EA_Walk = EA_Walk;
	ea->EA_MoveUp = EA_MoveUp;
	ea->EA_MoveDown = EA_MoveDown;
	ea->EA_MoveForward = EA_MoveForward;
	ea->EA_MoveBack = EA_MoveBack;
	ea->EA_MoveLeft = EA_MoveLeft;
	ea->EA_MoveRight = EA_MoveRight;
	ea->EA_Move = EA_Move;
	ea->EA_View = EA_View;
	ea->EA_GetInput = EA_GetInput;
	ea->EA_EndRegular = EA_EndRegular;
	ea->EA_ResetInput = EA_ResetInput;
	ea->EA_Prone = EA_Prone;
}


/*
============
GetBotLibAPI
============
*/
botlib_export_t *GetBotLibAPI(int apiVersion, botlib_import_t *import)
{
	botimport = *import;

	memset(&be_botlib_export, 0, sizeof(be_botlib_export));

	if (apiVersion != BOTLIB_API_VERSION)
	{
		botimport.Print(PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion);
		return NULL;
	}

	Init_EA_Export( &be_botlib_export.ea );

	be_botlib_export.BotLibSetup               = Export_BotLibSetup;
	be_botlib_export.BotLibShutdown            = Export_BotLibShutdown;
	be_botlib_export.BotLibVarSet              = Export_BotLibVarSet;
	be_botlib_export.BotLibVarGet              = Export_BotLibVarGet;
	be_botlib_export.PC_AddGlobalDefine        = PC_AddGlobalDefine;
	be_botlib_export.PC_RemoveAllGlobalDefines = PC_RemoveAllGlobalDefines;
	be_botlib_export.PC_LoadSourceHandle       = PC_LoadSourceHandle;
	be_botlib_export.PC_FreeSourceHandle       = PC_FreeSourceHandle;
	be_botlib_export.PC_ReadTokenHandle        = PC_ReadTokenHandle;
	be_botlib_export.PC_SourceFileAndLine      = PC_SourceFileAndLine;
	be_botlib_export.PC_UnreadLastTokenHandle  = PC_UnreadLastTokenHandle;

	return &be_botlib_export;
}
