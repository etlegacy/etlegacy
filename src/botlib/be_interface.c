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
	//initialize byte swapping (litte endian etc.)
	Log_Open("botlib.log");

	botimport.Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");

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

	be_botlib_export.BotLibSetup    = Export_BotLibSetup;
	be_botlib_export.BotLibShutdown = Export_BotLibShutdown;
	be_botlib_export.BotLibVarSet   = Export_BotLibVarSet;
	be_botlib_export.BotLibVarGet   = Export_BotLibVarGet;

	be_botlib_export.PC_AddGlobalDefine        = PC_AddGlobalDefine;
	be_botlib_export.PC_RemoveAllGlobalDefines = PC_RemoveAllGlobalDefines;
	be_botlib_export.PC_LoadSourceHandle       = PC_LoadSourceHandle;
	be_botlib_export.PC_FreeSourceHandle       = PC_FreeSourceHandle;
	be_botlib_export.PC_ReadTokenHandle        = PC_ReadTokenHandle;
	be_botlib_export.PC_SourceFileAndLine      = PC_SourceFileAndLine;
	be_botlib_export.PC_UnreadLastTokenHandle  = PC_UnreadLastTokenHandle;

	return &be_botlib_export;
}
