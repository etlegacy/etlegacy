/**
 * @file tvg_lua.c
 * @brief ET <-> *Lua* interface source file.
 *
 * @copyright This code is derived from ETPub and NQ, and inspired from ETPro
 * All credits go to their teams especially to quad and pheno!
 *
 * [League Lua API]: https://etlegacy-lua-docs.readthedocs.io
 */
#ifdef FEATURE_LUA

#include "tvg_lua.h"

#ifdef FEATURE_LUASQL
#include "../luasql/luasql.h"
#include "../luasql/luasql.c"
#include "../luasql/ls_sqlite3.c"
#endif

extern field_t fields[];

lua_vm_t *lVM[LUA_NUM_VM];

/**
 * @brief C_gentity_ptr_to_entNum registers a name for this Lua module
 * @param[in] addr pointer to a gentity (gentity*)
 * @param[out] the entity number.
 *             if (input==0) return = -1
 *             if (input address is out of g_entities[] memory range) return -1;
 */
static int C_gentity_ptr_to_entNum(uintptr_t addr)
{
	// no NULL address,
	// address must also be in the range of the g_entities array memory space..
	// address must also be pointing to the start of an entity (invalid if it points just somewhere halfway into the entity)
	if (!addr ||
	    (gentity_t *)addr < &g_entities[0] || (gentity_t *)addr > &g_entities[MAX_GENTITIES - 1] ||
	    (addr - (uintptr_t)&g_entities[0]) % sizeof(gentity_t) != 0)
	{
		return -1;
	}
	return (int)((gentity_t *)addr - g_entities);
}

/**
 * -------------------------------------
 * Mod function calls from lua
 * -------------------------------------
 * @addtogroup lua_etfncs
 * @{
 */

/**
 * @brief _et_RegisterModname registers a name for this Lua module
 *        et.RegisterModname( modname )
 * @param[in] modname
 */
static int _et_RegisterModname(lua_State *L)
{
	const char *modname = luaL_checkstring(L, 1);

	if (modname)
	{
		lua_vm_t *vm = TVG_LuaGetVM(L);
		if (vm)
		{
			Q_strncpyz(vm->mod_name, modname, sizeof(vm->mod_name));
		}
	}
	return 0;
}

/**
 * @brief _et_FindSelf gets slot number assigned to this lua VM
 *        vmnumber = et.FindSelf()
 * @param[in] vmnumber
 * @param[out] slot number between 0 and #LUA_NUM_VM
 */
static int _et_FindSelf(lua_State *L)
{
	lua_vm_t *vm = TVG_LuaGetVM(L);

	if (vm)
	{
		lua_pushinteger(L, vm->id);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

/**
 * @brief _et_FindMod gets name of lua module at slot @p vmnumber and its signature hash @lua modname
 *        signature = et.FindMod( vmnumber )
 * @param[in] vmnumber
 * @param[out] VM name registered by _et_RegisterModname() and SHA-1 signature of the VM
 */
static int _et_FindMod(lua_State *L)
{
	int      vmnumber = (int)luaL_checkinteger(L, 1);
	lua_vm_t *vm      = lVM[vmnumber];

	if (vm)
	{
		lua_pushstring(L, vm->mod_name);
		lua_pushstring(L, vm->mod_signature);
	}
	else
	{
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 2;
}

/**
 * @brief _et_IPCSend
 *        success = et.IPCSend( vmnumber, message )
 * @param[in] vmnumber
 * @param[in] message
 * @return success
 */
static int _et_IPCSend(lua_State *L)
{
	int        vmnumber = (int)luaL_checkinteger(L, 1);
	const char *message = luaL_checkstring(L, 2);

	lua_vm_t *sender = TVG_LuaGetVM(L);
	lua_vm_t *vm     = lVM[vmnumber];

	if (!vm || vm->err)
	{
		lua_pushinteger(L, 0);
		return 1;
	}

	// Find callback
	if (!TVG_LuaGetNamedFunction(vm, "et_IPCReceive"))
	{
		lua_pushinteger(L, 0);
		return 1;
	}

	// Arguments
	if (sender)
	{
		lua_pushinteger(vm->L, sender->id);
	}
	else
	{
		lua_pushnil(vm->L);
	}
	lua_pushstring(vm->L, message);

	// Call
	if (!TVG_LuaCall(vm, "et.IPCSend", 2, 0))
	{
		//TVG_LuaStopVM(vm);
		lua_pushinteger(L, 0);
		return 1;
	}

	// Success
	lua_pushinteger(L, 1);
	return 1;
}

// Printing

/**
 * @brief _et_G_Print
 *        et.G_Print( text )
 * @param[in] text
 */
static int _et_G_Print(lua_State *L)
{
	char text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));
	trap_Printf(text);
	return 0;
}

/**
 * @brief _et_G_LogPrint
 *        et.G_LogPrint( text )
 * @param[in] text
 */
static int _et_G_LogPrint(lua_State *L)
{
	char text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));

	G_Printf("%s", text);

	// Additional logging
	if (level.logFile)
	{
		char string[1024];

		//if ( g_logOptions.integer & LOGOPTS_REALTIME )
		//{
		//    Com_sprintf(string, sizeof(string), "%s %s", G_GetRealTime(), text);
		//}
		//else
		{
			int min, tens, sec;

			sec  = level.time / 1000;
			min  = sec / 60;
			sec -= min * 60;
			tens = sec / 10;
			sec -= tens * 10;

			Com_sprintf(string, sizeof(string), "%i:%i%i %s", min, tens, sec, text);
		}

		trap_FS_Write(string, strlen(string), level.logFile);
	}
	return 0;
}

// Argument Handling

/**
 * @brief _et_ConcatArgs
 *        args = et.ConcatArgs( index )
 * @param[in] index
 * @return args
 */
static int _et_ConcatArgs(lua_State *L)
{
	int index = (int)luaL_checkinteger(L, 1);

	lua_pushstring(L, ConcatArgs(index));
	return 1;
}

/**
 * @brief _et_trap_Argc
 *        argcount = et.trap_Argc()
 * @return argcount
 */
static int _et_trap_Argc(lua_State *L)
{
	lua_pushinteger(L, trap_Argc());
	return 1;
}

/**
 * @brief _et_trap_Argv
 *        arg = et.trap_Argv( argnum )
 * @param[in] argnum
 * @return arg
 */
static int _et_trap_Argv(lua_State *L)
{
	char buff[MAX_STRING_CHARS];
	int  argnum = (int)luaL_checkinteger(L, 1);

	trap_Argv(argnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// Cvars

/**
 * @brief _et_trap_Cvar_Get
 *        cvarvalue = et.trap_Cvar_Get( cvarname )
 * @param[in] cvarname
 * @return cvarvalue
 */
static int _et_trap_Cvar_Get(lua_State *L)
{
	char       buff[MAX_CVAR_VALUE_STRING];
	const char *cvarname = luaL_checkstring(L, 1);

	trap_Cvar_VariableStringBuffer(cvarname, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

/**
 * @brief _et_trap_Cvar_Set
 *        et.trap_Cvar_Set( cvarname, cvarvalue )
 * @param[in] cvarname
 * @param[in] cvarvalue
 * @return
 */
static int _et_trap_Cvar_Set(lua_State *L)
{
	const char *cvarname  = luaL_checkstring(L, 1);
	const char *cvarvalue = luaL_checkstring(L, 2);

	trap_Cvar_Set(cvarname, cvarvalue);
	return 0;
}

// Config Strings

/**
 * @brief _et_trap_GetConfigstring
 *        configstringvalue = et.trap_GetConfigstring( index )
 * @param[in] index
 * @return configstringvalue
 */
static int _et_trap_GetConfigstring(lua_State *L)
{
	char buff[MAX_STRING_CHARS];
	int  index = (int)luaL_checkinteger(L, 1);

	trap_GetConfigstring(index, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

/**
 * @brief _et_trap_SetConfigstring
 *        et.trap_SetConfigstring( index, configstringvalue )
 * @param[in] index
 * @param[in] configstringvalue
 * @return
 */
static int _et_trap_SetConfigstring(lua_State *L)
{
	int        index = (int)luaL_checkinteger(L, 1);
	const char *csv  = luaL_checkstring(L, 2);

	trap_SetConfigstring(index, csv);
	return 0;
}

// Server

/**
 * @brief _et_trap_SendConsoleCommand
 *        et.trap_SendConsoleCommand( when, command )
 * @param[in] when
 * @param[in] command
 * @return
 */
static int _et_trap_SendConsoleCommand(lua_State *L)
{
	int        when = (int)luaL_checkinteger(L, 1);
	const char *cmd = luaL_checkstring(L, 2);

	trap_SendConsoleCommand(when, cmd);
	return 0;
}

// String Utility Functions

/**
 * @brief _et_Info_RemoveKey
 *        infostring = et.Info_RemoveKey( infostring, key )
 * @param[in] infostring
 * @param[in] key
 * @return infostring
 */
static int _et_Info_RemoveKey(lua_State *L)
{
	char       buff[MAX_INFO_STRING];
	const char *key = luaL_checkstring(L, 2);

	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Info_RemoveKey(buff, key);
	lua_pushstring(L, buff);
	return 1;
}

/**
 * @brief _et_Info_SetValueForKey
 *        infostring = et.Info_SetValueForKey( infostring, key, value )
 * @param[in] infostring
 * @param[in] key
 * @param[in] value
 * @return infostring
 */
static int _et_Info_SetValueForKey(lua_State *L)
{
	char       buff[MAX_INFO_STRING];
	const char *key   = luaL_checkstring(L, 2);
	const char *value = luaL_checkstring(L, 3);

	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Info_SetValueForKey(buff, key, value);
	lua_pushstring(L, buff);
	return 1;
}

/**
 * @brief _et_Info_ValueForKey
 *        keyvalue = et.Info_ValueForKey( infostring, key )
 * @param[in] infostring
 * @param[in] key
 * @return keyvalue
 */
static int _et_Info_ValueForKey(lua_State *L)
{
	const char *infostring = luaL_checkstring(L, 1);
	const char *key        = luaL_checkstring(L, 2);

	lua_pushstring(L, Info_ValueForKey(infostring, key));
	return 1;
}

/**
 * @brief _et_Q_CleanStr
 *        cleanstring = et.Q_CleanStr( string )
 * @param[in] string
 * @return cleanstring
 */
static int _et_Q_CleanStr(lua_State *L)
{
	char buff[MAX_STRING_CHARS];

	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Q_CleanStr(buff);
	lua_pushstring(L, buff);
	return 1;
}

// ET Filesystem

/**
 * @brief _et_trap_FS_FOpenFile
 *        fd, len = et.trap_FS_FOpenFile( filename, mode )
 * @param[in] filename
 * @param[in] mode
 * @return fd, len
 */
static int _et_trap_FS_FOpenFile(lua_State *L)
{
	fileHandle_t fd;
	int          len;
	const char   *filename = luaL_checkstring(L, 1);
	int          mode      = (int)luaL_checkinteger(L, 2);

	len = trap_FS_FOpenFile(filename, &fd, mode);
	lua_pushinteger(L, fd);
	lua_pushinteger(L, len);
	return 2;
}

/**
 * @brief _et_trap_FS_Read
 *        filedata = et.trap_FS_Read( fd, count )
 * @param[in] fd
 * @param[in] count
 * @return filedata
 */
static int _et_trap_FS_Read(lua_State *L)
{
	char         *filedata;
	fileHandle_t fd    = (int)luaL_checkinteger(L, 1);
	int          count = (int)luaL_checkinteger(L, 2);

	filedata = Com_Allocate(count + 1);

	if (filedata == NULL)
	{
		G_Printf("%s Lua: %sMemory allocation error for _et_trap_FS_Read file data\n", LUA_VERSION, S_COLOR_BLUE);
		return 0;
	}

	trap_FS_Read(filedata, count, fd);
	*(filedata + count) = '\0';
	lua_pushstring(L, filedata);
	Com_Dealloc(filedata);
	return 1;
}

/**
 * @brief _et_trap_FS_Write
 *        count = et.trap_FS_Write( filedata, count, fd )
 * @param[in] filedata
 * @param[in] count
 * @param[in] fd
 * @return count
 */
static int _et_trap_FS_Write(lua_State *L)
{
	const char *filedata = luaL_checkstring(L, 1);
	int        count     = (int)luaL_checkinteger(L, 2);

	fileHandle_t fd = (int)luaL_checkinteger(L, 3);
	lua_pushinteger(L, trap_FS_Write(filedata, count, fd));
	return 1;
}

/**
 * @brief _et_trap_FS_FCloseFile
 *        et.trap_FS_FCloseFile( fd )
 * @param[in] fd
 * @return
 */
static int _et_trap_FS_FCloseFile(lua_State *L)
{
	fileHandle_t fd = (int)luaL_checkinteger(L, 1);
	trap_FS_FCloseFile(fd);
	return 0;
}

/**
 * @brief _et_trap_FS_Rename
 *        et.trap_FS_Rename( oldname, newname )
 * @param[in] oldname
 * @param[in] newname
 * @return
 */
static int _et_trap_FS_Rename(lua_State *L)
{
	const char *oldname = luaL_checkstring(L, 1);
	const char *newname = luaL_checkstring(L, 2);

	trap_FS_Rename(oldname, newname);
	return 0;
}

extern char bigTextBuffer[100000];

/**
 * @brief _et_trap_FS_GetFileList
 *        filelist = et.trap_FS_GetFileList( dirname, fileextension )
 * @param[in] dirname
 * @param[in] fileextension
 * @return filelist
 */
static int _et_trap_FS_GetFileList(lua_State *L)
{
	const char *dirname            = luaL_checkstring(L, 1);
	const char *filename_extension = luaL_checkstring(L, 2);
	int        newTable, index = 1, i, filelen, numfiles;
	char       filename[MAX_QPATH];
	char       *filenameptr = bigTextBuffer;

	numfiles = trap_FS_GetFileList(dirname, filename_extension, bigTextBuffer, sizeof(bigTextBuffer));

	lua_createtable(L, numfiles, 0);
	newTable = lua_gettop(L);

	for (i = 0; i < numfiles; i++, filenameptr += filelen + 1)
	{
		filelen = strlen(filenameptr);
		Q_strncpyz(filename, filenameptr, sizeof(filename));

		lua_pushstring(L, filename);
		lua_rawseti(L, newTable, index++);
	}

	return 1;
}

/**
 * @brief _et_TVG_SoundIndex
 *        soundindex = et.TVG_SoundIndex( filename )
 * @param[in] filename
 * @return
 */
static int _et_TVG_SoundIndex(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	lua_pushinteger(L, TVG_SoundIndex(filename));
	return 1;
}

/**
 * @brief _et_TVG_ModelIndex
 *        modelindex = et.TVG_ModelIndex( filename )
 * @param[in] filename
 * @return modelindex
 */
static int _et_TVG_ModelIndex(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	lua_pushinteger(L, TVG_ModelIndex((char *)filename));
	return 1;
}

/**
 * @brief _et_trap_Milliseconds
 *        milliseconds = et.trap_Milliseconds()
 * @return milliseconds
 */
static int _et_trap_Milliseconds(lua_State *L)
{
	lua_pushinteger(L, trap_Milliseconds());
	return 1;
}

/**
 * @brief _et_isBitSet little helper for accessing bitmask values
 *        if bit 'bit' is set in 'value', true is returned, else false
 *        success = et.isBitSet(bit,value)
 * @param[in] bit
 * @param[in] value
 * @return success
 */
static int _et_isBitSet(lua_State *L)
{
	int b = (int)luaL_checkinteger(L, 1);
	int v = (int)luaL_checkinteger(L, 2);

	if (v & b)
	{
		lua_pushboolean(L, 1);
	}
	else
	{
		lua_pushboolean(L, 0);
	}
	return 1;
}

/**
 * @brief playerState_fields playerState_t fields
 */
static const tvgame_field_t playerState_fields[] =
{
	_et_gclient_addfield(ps.commandTime,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.pm_type,              FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.bobCycle,             FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.pm_flags,             FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.pm_time,              FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.eFlags,               FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.origin,               FIELD_VEC3,      FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.viewangles,           FIELD_VEC3,      FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.velocity,             FIELD_VEC3,      FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.delta_angles,         FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.groundEntityNum,      FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.leanf,                FIELD_FLOAT,     FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.viewheight,           FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.mins,                 FIELD_VEC3,      FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.maxs,                 FIELD_VEC3,      FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.weapon,               FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.nextWeapon,           FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.weaponstate,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.weaponTime,           FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.weaponDelay,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.grenadeTimeLeft,      FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.curWeapHeat,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.aimSpreadScale,       FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.ammo,                 FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.ammoclip,             FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.classWeaponTime,      FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.eventSequence,        FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.events,               FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.eventParms,           FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.oldEventSequence,     FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.stats,                FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.persistant,           FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.ping,                 FIELD_INT,       FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gclient_addfield(ps.powerups,             FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),

	_et_gclient_addfield(ps.damageEvent,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.damageYaw,            FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.damagePitch,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.damageCount,          FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.clientNum,            FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.viewlocked,           FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.viewlocked_entNum,    FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.teamNum,              FIELD_INT,       FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.serverCursorHint,     FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.serverCursorHintVal,  FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.identifyClient,       FIELD_INT,       FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.identifyClientHealth, FIELD_INT,       FIELD_FLAG_READONLY),

	{ NULL },
};

/**
 * @brief clientPersistant_fields clientPersistant_t fields
 */
static const tvgame_field_t clientPersistant_fields[] =
{
	_et_gclient_addfield(pers.connected,       FIELD_INT,     FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.cmd,             FIELD_USERCMD, FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.oldcmd,          FIELD_USERCMD, FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.netname,         FIELD_STRING,  FIELD_FLAG_NOPTR),
	_et_gclient_addfield(pers.localClient,     FIELD_INT,     FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.enterTime,       FIELD_INT,     FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.connectTime,     FIELD_INT,     FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.teamState.state, FIELD_INT,     FIELD_FLAG_READONLY),

	{ NULL },
};

/**
 * @brief clientSession_fields clientSession_t fields
 */
static const tvgame_field_t clientSession_fields[] =
{
	_et_gclient_addfield(sess.sessionTeam,             FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.spectatorState,          FIELD_INT, 0),
	_et_gclient_addfield(sess.spectatorClient,         FIELD_INT, 0),
	_et_gclient_addfield(sess.playerType,              FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.muted,                   FIELD_INT, 0),
	_et_gclient_addfield(sess.referee,                 FIELD_INT, 0),
	_et_gclient_addfield(sess.spec_team,               FIELD_INT, 0),

	_et_gclient_addfield(sess.nextReliableTime,        FIELD_INT, 0),
	_et_gclient_addfield(sess.numReliableCommands,     FIELD_INT, 0),
	_et_gclient_addfield(sess.nextCommandDecreaseTime, FIELD_INT, 0),

	_et_gclient_addfield(sess.tvchat,                  FIELD_INT, 0),

	{ NULL },
};

/**
 * @brief gclient_fields gclient_t fields
 */
static const tvgame_field_t gclient_fields[] =
{
	_et_gclient_addfield(noclip,                FIELD_INT, 0),
	_et_gclient_addfield(inactivityTime,        FIELD_INT, 0),
	_et_gclient_addfield(inactivityWarning,     FIELD_INT, 0),
	_et_gclient_addfield(inactivitySecondsLeft, FIELD_INT, 0),

	_et_gclient_addfield(buttons,               FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(oldbuttons,            FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(latched_buttons,       FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(wbuttons,              FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(oldwbuttons,           FIELD_INT, FIELD_FLAG_READONLY),
	_et_gclient_addfield(latched_wbuttons,      FIELD_INT, FIELD_FLAG_READONLY),

	{ NULL },
};

/**
 * @brief entityShared_fields entityShared_t fields
 */
static const tvgame_field_t entityShared_fields[] =
{
	_et_gentity_addfield(r.linked,           FIELD_INT,  FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.svFlags,          FIELD_INT,  FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.singleClient,     FIELD_INT,  FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.bmodel,           FIELD_INT,  FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.maxs,             FIELD_VEC3, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.mins,             FIELD_VEC3, FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.contents,         FIELD_INT,  FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.absmin,           FIELD_VEC3, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.absmax,           FIELD_VEC3, FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.currentOrigin,    FIELD_VEC3, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.currentAngles,    FIELD_VEC3, FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.ownerNum,         FIELD_INT,  FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.eventTime,        FIELD_INT,  FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.worldflags,       FIELD_INT,  FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.snapshotCallback, FIELD_INT,  FIELD_FLAG_READONLY),

	{ NULL },
};

/**
 * @brief entityState_fields entityState_t fields
 */
static const tvgame_field_t entityState_fields[] =
{
	_et_gentity_addfield(s.number,          FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.eType,           FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.eFlags,          FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.pos,             FIELD_TRAJECTORY, FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.apos,            FIELD_TRAJECTORY, FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.time,            FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.time2,           FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.origin,          FIELD_VEC3,       FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.origin2,         FIELD_VEC3,       FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.angles,          FIELD_VEC3,       FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.angles2,         FIELD_VEC3,       FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.otherEntityNum,  FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.otherEntityNum2, FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.groundEntityNum, FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.constantLight,   FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.dl_intensity,    FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.loopSound,       FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.modelindex,      FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.modelindex2,     FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.clientNum,       FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.frame,           FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.solid,           FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.event,           FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.eventParm,       FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.eventSequence,   FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.events,          FIELD_INT_ARRAY,  FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_gentity_addfield(s.eventParms,      FIELD_INT_ARRAY,  FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),

	_et_gentity_addfield(s.powerups,        FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.weapon,          FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.legsAnim,        FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.torsoAnim,       FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.density,         FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.dmgFlags,        FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.onFireEnd,       FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.onFireStart,     FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.nextWeapon,      FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.teamNum,         FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(s.effect1Time,     FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.effect2Time,     FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.effect3Time,     FIELD_INT,        FIELD_FLAG_READONLY),

	{ NULL },
};

/**
 * @brief gentity_fields entity fields
 */
static const tvgame_field_t gentity_fields[] =
{
	_et_gentity_addfield(classname,  FIELD_STRING, FIELD_FLAG_READONLY),
	_et_gentity_addfield(flags,      FIELD_INT,    FIELD_FLAG_READONLY),
	_et_gentity_addfield(inuse,      FIELD_INT,    FIELD_FLAG_READONLY),
	_et_gentity_addfield(spawnflags, FIELD_INT,    FIELD_FLAG_READONLY),
	_et_gentity_addfield(targetname, FIELD_STRING, FIELD_FLAG_READONLY),

	{ NULL },
};

/**
 * @brief level_fields level_locals_t fields
 */
static const tvgame_field_t level_fields[] =
{
	_et_level_addfield(time,                  FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(previousTime,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(frameTime,             FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(startTime,             FIELD_INT,       FIELD_FLAG_READONLY),

	_et_level_addfield(rawmapname,            FIELD_STRING,    FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),
	_et_level_addfield(intermission_origin,   FIELD_VEC3,      0),
	_et_level_addfield(intermission_angle,    FIELD_VEC3,      0),

	_et_level_addfield(queueSeconds,          FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(intermission,          FIELD_INT,       FIELD_FLAG_READONLY),

	_et_level_addfield(numConnectedClients,   FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(sortedClients,         FIELD_INT_ARRAY, FIELD_FLAG_READONLY),

	_et_level_addfield(numValidMasterClients, FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(validMasterClients,    FIELD_INT_ARRAY, FIELD_FLAG_READONLY | FIELD_FLAG_NOPTR),

	_et_level_addfield(mod,                   FIELD_INT,       FIELD_FLAG_READONLY),

	_et_level_addfield(tvcmdsCount,           FIELD_INT,       FIELD_FLAG_READONLY),
	_et_level_addfield(tvcmds,                FIELD_TVCMDS,    0),

	{ NULL },
};

/**
 * @brief _et_gettvgame_field fields helper function
 * @param[in] fieldname
 * @param[in] array
 * @return game_field_t
 */
static tvgame_field_t *_et_gettvgame_field(const char *fieldname, const tvgame_field_t *array)
{
	int i;

	for (i = 0; array[i].name; i++)
	{
		if (!Q_stricmp(fieldname, array[i].name))
		{
			return (tvgame_field_t *)&array[i];
		}
	}

	return NULL;
}

/**
 * @brief _et_getfield fields helper function
 * @param[in] fieldname
 * @param[in] fieldFlag
 * @return game_field_t
 */
static tvgame_field_t *_et_getfield(const char *fieldname, int fieldFlag)
{
	switch (fieldFlag)
	{
	case FIELD_FLAG_GENTITY:
		if (!Q_strncmp(fieldname, "s.", 2))
		{
			return _et_gettvgame_field(fieldname, entityState_fields);
		}

		if (!Q_strncmp(fieldname, "r.", 2))
		{
			return _et_gettvgame_field(fieldname, entityShared_fields);
		}

		return _et_gettvgame_field(fieldname, gentity_fields);
	case FIELD_FLAG_GCLIENT:
		if (!Q_strncmp(fieldname, "ps.", 3))
		{
			return _et_gettvgame_field(fieldname, playerState_fields);
		}

		if (!Q_strncmp(fieldname, "sess.", 5))
		{
			return _et_gettvgame_field(fieldname, clientSession_fields);
		}

		if (!Q_strncmp(fieldname, "pers.", 5))
		{
			return _et_gettvgame_field(fieldname, clientPersistant_fields);
		}

		return _et_gettvgame_field(fieldname, gclient_fields);
	case FIELD_FLAG_LEVEL:
		return _et_gettvgame_field(fieldname, level_fields);
	default:
		return NULL;
	}
}

/**
 * @brief _et_getvec3
 * @param[in,out] L
 * @param[in] vec3
 */
static void _et_getvec3(lua_State *L, vec3_t vec3)
{
	lua_newtable(L);
	lua_pushnumber(L, vec3[0]);
	lua_rawseti(L, -2, 1);
	lua_pushnumber(L, vec3[1]);
	lua_rawseti(L, -2, 2);
	lua_pushnumber(L, vec3[2]);
	lua_rawseti(L, -2, 3);
}

/**
 * @brief _et_setvec3
 * @param[in] L
 * @param[in,out] vec3
 */
static void _et_setvec3(lua_State *L, vec3_t *vec3)
{
	lua_pushnumber(L, 1);
	lua_gettable(L, -2);
	(*vec3)[0] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, 2);
	lua_gettable(L, -2);
	(*vec3)[1] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, 3);
	lua_gettable(L, -2);
	(*vec3)[2] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
}

/**
 * @brief _et_gettrajectory
 * @param[in,out] L
 * @param[in] traj
 */
static void _et_gettrajectory(lua_State *L, trajectory_t *traj)
{
	int index;

	lua_newtable(L);
	index = lua_gettop(L);
	lua_pushstring(L, "trType");
	lua_pushinteger(L, traj->trType);
	lua_settable(L, -3);
	lua_pushstring(L, "trTime");
	lua_pushinteger(L, traj->trTime);
	lua_settable(L, -3);
	lua_pushstring(L, "trDuration");
	lua_pushinteger(L, traj->trDuration);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trBase");
	_et_getvec3(L, traj->trBase);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trDelta");
	_et_getvec3(L, traj->trDelta);
	lua_settable(L, -3);
}

/**
 * @brief _et_settrajectory
 * @param[in] L
 * @param[in,out] traj
 */
static void _et_settrajectory(lua_State *L, trajectory_t *traj)
{
	lua_pushstring(L, "trType");
	lua_gettable(L, -2);
	traj->trType = (trType_t)lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trTime");
	lua_gettable(L, -2);
	traj->trTime = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trDuration");
	lua_gettable(L, -2);
	traj->trDuration = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trBase");
	lua_gettable(L, -2);
	_et_setvec3(L, (vec3_t *)traj->trBase);
	lua_pop(L, 1);
	lua_pushstring(L, "trDelta");
	lua_gettable(L, -2);
	_et_setvec3(L, (vec3_t *)traj->trDelta);
	lua_pop(L, 1);
}

/**
 * @brief _et_getusercmd
 * @param[in,out] L
 * @param[in] cmd
 */
static void _et_getusercmd(lua_State *L, usercmd_t *cmd)
{
	lua_newtable(L);
	lua_pushinteger(L, cmd->serverTime);
	lua_setfield(L, -2, "serverTime");

	lua_pushinteger(L, cmd->buttons);
	lua_setfield(L, -2, "buttons");

	lua_pushinteger(L, cmd->wbuttons);
	lua_setfield(L, -2, "wbuttons");

	lua_pushinteger(L, cmd->weapon);
	lua_setfield(L, -2, "weapon");

	lua_pushinteger(L, cmd->flags);
	lua_setfield(L, -2, "flags");

	lua_newtable(L);
	lua_pushinteger(L, cmd->angles[0]);
	lua_rawseti(L, -2, 1);
	lua_pushinteger(L, cmd->angles[1]);
	lua_rawseti(L, -2, 2);
	lua_pushinteger(L, cmd->angles[1]);
	lua_rawseti(L, -2, 3);
	lua_setfield(L, -2, "angles");

	lua_pushinteger(L, cmd->forwardmove);
	lua_setfield(L, -2, "forwardmove");

	lua_pushinteger(L, cmd->rightmove);
	lua_setfield(L, -2, "rightmove");

	lua_pushinteger(L, cmd->upmove);
	lua_setfield(L, -2, "upmove");

	lua_pushinteger(L, cmd->doubleTap);
	lua_setfield(L, -2, "doubleTap");

	lua_pushinteger(L, cmd->identClient);
	lua_setfield(L, -2, "identClient");
}

/**
 * @brief _et_setusercmd unused (usercmd_t is read-only)
 * @param[in,out] L
 * @param[in] cmd
 */
static void _et_setusercmd(lua_State *L, usercmd_t *cmd)
{
	lua_pushstring(L, "serverTime");
	lua_gettable(L, -2);
	cmd->serverTime = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "buttons");
	lua_gettable(L, -2);
	cmd->buttons = (byte)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "wbuttons");
	lua_gettable(L, -2);
	cmd->wbuttons = (byte)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "weapon");
	lua_gettable(L, -2);
	cmd->weapon = (byte)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "flags");
	lua_gettable(L, -2);
	cmd->flags = (byte)lua_tointeger(L, -1);
	lua_pop(L, 1);

	{
		lua_pushstring(L, "angles");
		lua_gettable(L, -2);

		lua_pushinteger(L, 1);
		lua_gettable(L, -2);
		cmd->angles[0] = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pushinteger(L, 2);
		lua_gettable(L, -2);
		cmd->angles[1] = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pushinteger(L, 3);
		lua_gettable(L, -2);
		cmd->angles[2] = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pop(L, 1);
	}

	lua_pushstring(L, "forwardmove");
	lua_gettable(L, -2);
	cmd->forwardmove = (signed char)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "rightmove");
	lua_gettable(L, -2);
	cmd->rightmove = (signed char)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "upmove");
	lua_gettable(L, -2);
	cmd->upmove = (signed char)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "doubleTap");
	lua_gettable(L, -2);
	cmd->doubleTap = (byte)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "identClient");
	lua_gettable(L, -2);
	cmd->identClient = (byte)lua_tointeger(L, -1);
	lua_pop(L, 1);
}

/**
 * @brief _et_findtvcmd
 * @param[in] tvcmd
 * @param[in] name
 * @param[in,out] index
 * @return
 */
static qboolean _et_findtvcmd(tvcmd_reference_t *tvcmd, const char *name, int *index)
{
	int i;

	for (i = 0; tvcmd[i].pszCommandName; i++)
	{
		if (!Q_stricmp(tvcmd[i].pszCommandName, name))
		{
			*index = i;
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief _et_gettvcmd
 * @param[in,out] L
 * @param[in] tvcmd
 * @param[in] name
 */
static void _et_gettvcmd(lua_State *L, tvcmd_reference_t *tvcmd, const char *name)
{
	int index;

	if (!_et_findtvcmd(tvcmd, name, &index))
	{
		// command not found, return nil
		lua_pushnil(L);
		return;
	}

	lua_newtable(L);

	lua_pushstring(L, tvcmd[index].pszCommandName);
	lua_setfield(L, -2, "name");

	lua_pushinteger(L, tvcmd[index].flag);
	lua_setfield(L, -2, "flag");

	lua_pushinteger(L, tvcmd[index].value);
	lua_setfield(L, -2, "value");

	lua_pushinteger(L, tvcmd[index].updateInterval);
	lua_setfield(L, -2, "updateInterval");

	lua_pushinteger(L, tvcmd[index].lastUpdateTime);
	lua_setfield(L, -2, "lastUpdateTime");

	lua_pushinteger(L, tvcmd[index].floodProtected);
	lua_setfield(L, -2, "floodProtected");

	lua_pushinteger(L, tvcmd[index].mods);
	lua_setfield(L, -2, "mods");
}

/**
 * @brief _et_settvcmd
 * @param[in,out] L
 * @param[in] tvcmd
 * @param[in] name
 */
static void _et_settvcmd(lua_State *L, tvcmd_reference_t *tvcmd, const char *name)
{
	int index;

	if (!_et_findtvcmd(tvcmd, name, &index))
	{
		// command not found
		luaL_error(L, "tried to modify not existing tvcmd \"%s\"", name);
		return;
	}

	lua_pushstring(L, "flag");
	lua_gettable(L, -2);
	tvcmd[index].flag = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "value");
	lua_gettable(L, -2);
	tvcmd[index].value = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "updateInterval");
	lua_gettable(L, -2);
	tvcmd[index].updateInterval = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "lastUpdateTime");
	lua_gettable(L, -2);
	tvcmd[index].lastUpdateTime = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "floodProtected");
	lua_gettable(L, -2);
	tvcmd[index].floodProtected = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "mods");
	lua_gettable(L, -2);
	tvcmd[index].mods = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
}

/**
 * @brief _et_field_get get field
 * @param[in,out] L
 * @param[in] field
 * @param[in] addr
 * @param[in] arg
 * @return success
 */
static int _et_field_get(lua_State *L, tvgame_field_t *field, uintptr_t addr, int arg)
{
	if (!addr)
	{
		lua_pushnil(L);
		return 1;
	}

	addr += field->mapping;

	switch (field->type)
	{
	case FIELD_INT:
		lua_pushinteger(L, *(int *)addr);
		return 1;
	case FIELD_STRING:
		if (field->flags & FIELD_FLAG_NOPTR)
		{
			lua_pushstring(L, (char *)addr);
		}
		else
		{
			lua_pushstring(L, *(char **)addr);
		}
		return 1;
	case FIELD_FLOAT:
		lua_pushnumber(L, *(float *)addr);
		return 1;
	case FIELD_ENTITY:
	{
		// core: return the entity-number  of the entity that the pointer is pointing at.
		int entNum = C_gentity_ptr_to_entNum(*(uintptr_t *)addr);

		if (entNum < 0)
		{
			lua_pushnil(L);
		}
		else
		{
			lua_pushinteger(L, entNum);
		}

		return 1;
	}
	case FIELD_VEC3:
		_et_getvec3(L, *(vec3_t *)addr);
		return 1;
	case FIELD_INT_ARRAY:
		if (field->flags & FIELD_FLAG_NOPTR)
		{
			lua_pushinteger(L, (*(int *)(addr + (sizeof(int) * (int)luaL_optinteger(L, arg, 0)))));
		}
		else
		{
			lua_pushinteger(L, (*(int *)(*(uintptr_t *)addr + (sizeof(int) * (int)luaL_optinteger(L, arg, 0)))));
		}
		return 1;
	case FIELD_TRAJECTORY:
		_et_gettrajectory(L, (trajectory_t *)addr);
		return 1;
	case FIELD_FLOAT_ARRAY:
		lua_pushnumber(L, (*(float *)(addr + (sizeof(int) * (int)luaL_optinteger(L, arg, 0)))));
		return 1;
	case FIELD_USERCMD:
		_et_getusercmd(L, (usercmd_t *)addr);
		return 1;
	case FIELD_TVCMDS:
		_et_gettvcmd(L, (tvcmd_reference_t *)*(uintptr_t *)addr, luaL_optstring(L, arg, 0));
		return 1;
	default:
		G_Printf("Lua API: field_get with no valid field type\n");
		break;
	}

	return 0;
}

/**
 * @brief _et_field_set set field
 * @param[in,out] L
 * @param[in] field
 * @param[in] addr
 * @param[in] arg1
 * @param[in] arg2
 * @return success
 */
static int _et_field_set(lua_State *L, tvgame_field_t *field, uintptr_t addr, int arg1, int arg2)
{
	if (!addr)
	{
		lua_pushnil(L);
		return 1;
	}

	addr += field->mapping;

	switch (field->type)
	{
	case FIELD_INT:
		*(int *)addr = (int)luaL_checkinteger(L, arg1);
		break;
	case FIELD_STRING:
	{
		const char *buffer = luaL_checkstring(L, arg1);

		if (field->flags & FIELD_FLAG_NOPTR)
		{
			Q_strncpyz((char *)addr, buffer, strlen((char *)addr));
		}
		else
		{
			Com_Dealloc(*(char **)addr);
			*(char **)addr = Com_Allocate(strlen(buffer) + 1);
			Q_strncpyz(*(char **)addr, buffer, strlen(buffer));
		}
		break;
	}
	case FIELD_FLOAT:
		*(float *)addr = (float)luaL_checknumber(L, arg1);
		break;
	case FIELD_ENTITY:
		// pointer-fields are read-only..
		break;
	case FIELD_VEC3:
		_et_setvec3(L, (vec3_t *)addr);
		break;
	case FIELD_INT_ARRAY:
		*(int *)(addr + (sizeof(int) * (int)luaL_checkinteger(L, arg1))) = (int)luaL_checkinteger(L, arg2);
		break;
	case FIELD_TRAJECTORY:
		_et_settrajectory(L, (trajectory_t *)addr);
		break;
	case FIELD_FLOAT_ARRAY:
		*(float *)(addr + (sizeof(int) * (int)luaL_checkinteger(L, arg1))) = (float)luaL_checknumber(L, arg2);
		return 1;
	case FIELD_USERCMD:
		_et_setusercmd(L, (usercmd_t *)addr);
		return 1;
	case FIELD_TVCMDS:
		_et_settvcmd(L, (tvcmd_reference_t *)*(uintptr_t *)addr, luaL_optstring(L, arg1, 0));
		return 1;
	default:
		G_Printf("Lua API: field_set with no valid field type\n");
		break;
	}
	return 0;
}

/**
 * @brief _et_level_get access to level data (level_locals_t)
 *        et.level_get( fieldname, arrayindex )
 * @param[in] fieldname
 * @param[in] arrayindex
 * @return requested field value
 */
static int _et_level_get(lua_State *L)
{
	const char     *fieldname = luaL_checkstring(L, 1);
	tvgame_field_t *field     = _et_getfield(fieldname, FIELD_FLAG_LEVEL);

	if (!field)
	{
		luaL_error(L, "tried to get invalid level field \"%s\"", fieldname);
		return 0;
	}

	return _et_field_get(L, field, (uintptr_t)&level, 2);
}

/**
 * @brief _et_level_set
		  et.level_set( fieldname, arrayindex, value )
 * @param[in] fieldname
 * @param[in] arrayindex
 * @param[in] value
 * @return
 */
static int _et_level_set(lua_State *L)
{
	const char     *fieldname = luaL_checkstring(L, 1);
	tvgame_field_t *field     = _et_getfield(fieldname, FIELD_FLAG_LEVEL);

	if (!field)
	{
		luaL_error(L, "tried to set invalid level field \"%s\"", fieldname);
		return 0;
	}

	if (field->flags & FIELD_FLAG_READONLY)
	{
		luaL_error(L, "tried to set read-only level field \"%s\"", fieldname);
		return 0;
	}

	return _et_field_set(L, field, (uintptr_t)&level, 2, 3);
}

/**
 * @brief _et_gentity_get access to master server entity data (entityState_t and entityShared_t)
 *        et.gentity_get( entnum, fieldname, arrayindex )
 * @param[in] entnum
 * @param[in] fieldname
 * @param[in] arrayindex
 * @return requested field value
 */
static int _et_gentity_get(lua_State *L)
{
	gentity_t      *ent       = g_entities + (int)luaL_checkinteger(L, 1);
	const char     *fieldname = luaL_checkstring(L, 2);
	tvgame_field_t *field     = _et_getfield(fieldname, FIELD_FLAG_GENTITY);

	if (!field)
	{
		luaL_error(L, "tried to get invalid gentity field \"%s\"", fieldname);
		return 0;
	}

	return _et_field_get(L, field, (uintptr_t)ent, 3);
}

/**
 * @brief _et_gclient_get access to viewers data (gclient_t)
 *        et.gclient_get( clientnum, fieldname, arrayindex )
 * @param[in] clientnum
 * @param[in] fieldname
 * @param[in] arrayindex
 * @return requested field value
 */
static int _et_gclient_get(lua_State *L)
{
	gclient_t      *client    = level.clients + (int)luaL_checkinteger(L, 1);
	const char     *fieldname = luaL_checkstring(L, 2);
	tvgame_field_t *field     = _et_getfield(fieldname, FIELD_FLAG_GCLIENT);

	if (!field)
	{
		luaL_error(L, "tried to get invalid gclient field \"%s\"", fieldname);
		return 0;
	}

	return _et_field_get(L, field, (uintptr_t)client, 3);
}

/**
 * @brief _et_gclient_set
		  et.gclient_set( clientnum, fieldname, arrayindex, value )
 * @param[in] entnum
 * @param[in] fieldname
 * @param[in] arrayindex
 * @param[in] value
 * @return
 */
static int _et_gclient_set(lua_State *L)
{
	gclient_t      *client    = level.clients + (int)luaL_checkinteger(L, 1);
	const char     *fieldname = luaL_checkstring(L, 2);
	tvgame_field_t *field     = _et_getfield(fieldname, FIELD_FLAG_GCLIENT);

	if (!field)
	{
		luaL_error(L, "tried to set invalid gclient field \"%s\"", fieldname);
		return 0;
	}

	if (field->flags & FIELD_FLAG_READONLY)
	{
		luaL_error(L, "tried to set read-only gclient field \"%s\"", fieldname);
		return 0;
	}

	return _et_field_set(L, field, (uintptr_t)client, 3, 4);
}

/**
 * @brief _et_ps_get access to master server playerstates data (playerState_t)
 *        et.ps_get( clientnum, fieldname, arrayindex )
 * @param[in] clientnum
 * @param[in] fieldname
 * @param[in] arrayindex
 * @return requested field value
 */
static int _et_ps_get(lua_State *L)
{
	int            clientnum  = (int)luaL_checkinteger(L, 1);
	const char     *fieldname = luaL_checkstring(L, 2);
	tvgame_field_t *field     = _et_getfield(fieldname, FIELD_FLAG_GCLIENT);
	gclient_t      client;

	if (!field || Q_strncmp("ps.", fieldname, 3))
	{
		luaL_error(L, "tried to get invalid playerstate field \"%s\"", fieldname);
		return 0;
	}

	if (clientnum < -1 || clientnum >= MAX_CLIENTS)
	{
		luaL_error(L, "invalid clientnum \"%d\"", clientnum);
		return 0;
	}

	if (clientnum == -1)
	{
		client.ps = level.ettvMasterPs;
	}
	else if (level.ettvMasterClients[clientnum].valid)
	{
		client.ps = level.ettvMasterClients[clientnum].ps;
	}
	else
	{
		luaL_error(L, "tried to get invalid playerstate \"%d\"", clientnum);
		return 0;
	}

	return _et_field_get(L, field, (uintptr_t)&client, 3);
}

/**
 * @brief _et_TVG_AddEvent
          et.TVG_AddEvent( client, event, eventparm )
 * @param[in] client
 * @param[in] event
 * @param[in] eventparm
 * @return
 */
static int _et_TVG_AddEvent(lua_State *L)
{
	int client    = (int)luaL_checkinteger(L, 1);
	int event     = (int)luaL_checkinteger(L, 2);
	int eventparm = (int)luaL_checkinteger(L, 3);
	TVG_AddEvent(level.clients + client, event, eventparm);
	return 0;
}

/**
 * @brief _et_getclipplane
 * @param[in,out] L
 * @param[in] plane
 */
static void _et_getclipplane(lua_State *L, cplane_t *plane)
{
	lua_newtable(L);
	_et_getvec3(L, plane->normal);
	lua_setfield(L, -2, "normal");
	lua_pushnumber(L, plane->dist);
	lua_setfield(L, -2, "dist");
	lua_pushinteger(L, plane->type);
	lua_setfield(L, -2, "type");
	lua_pushinteger(L, plane->signbits);
	lua_setfield(L, -2, "signbits");
	lua_newtable(L);
	lua_pushinteger(L, plane->pad[0]);
	lua_rawseti(L, -2, 1);
	lua_pushinteger(L, plane->pad[1]);
	lua_rawseti(L, -2, 2);
	lua_setfield(L, -2, "pad");
}

/**
 * @brief _et_gettrace
 * @param[in,out] L
 * @param[in] tr
 */
static void _et_gettrace(lua_State *L, trace_t *tr)
{
	lua_newtable(L);
	lua_pushboolean(L, tr->allsolid);
	lua_setfield(L, -2, "allsolid");
	lua_pushboolean(L, tr->startsolid);
	lua_setfield(L, -2, "startsolid");
	lua_pushnumber(L, tr->fraction);
	lua_setfield(L, -2, "fraction");
	_et_getvec3(L, tr->endpos);
	lua_setfield(L, -2, "endpos");
	_et_getclipplane(L, &tr->plane);
	lua_setfield(L, -2, "plane");
	lua_pushinteger(L, tr->surfaceFlags);
	lua_setfield(L, -2, "surfaceFlags");
	lua_pushinteger(L, tr->contents);
	lua_setfield(L, -2, "contents");
	lua_pushinteger(L, tr->entityNum);
	lua_setfield(L, -2, "entityNum");
}

/**
 * @brief _etH_toVec3
 * @param[in] inx
 * @return vec3_t
 */
static vec3_t *_etH_toVec3(lua_State *L, int inx)
{
	static vec3_t vec;

	lua_pushvalue(L, inx);
	_et_setvec3(L, &vec);
	lua_pop(L, 1);

	return &vec;
}

/**
 * @brief _et_trap_Trace
          et.trap_Trace( start, mins, maxs, end, entNum, mask )
 * @param[in] start
 * @param[in] mins
 * @param[in] maxs
 * @param[in] end
 * @param[in] entNum
 * @param[in] mask
 * @return
 */
static int _et_trap_Trace(lua_State *L)
{
	trace_t tr;
	vec3_t  start, end, mins, maxs;
	vec3_t  *minsPtr = NULL, *maxsPtr = NULL;
	int     entNum, mask;

	if (!lua_istable(L, 1))
	{
		luaL_error(L, "trap_Trace: \"start\" argument should be an instance of table");
	}
	VectorCopy(*_etH_toVec3(L, 1), start);

	if (lua_istable(L, 2))
	{
		VectorCopy(*_etH_toVec3(L, 2), mins);
		minsPtr = &mins;
	}

	if (lua_istable(L, 3))
	{
		VectorCopy(*_etH_toVec3(L, 3), maxs);
		maxsPtr = &maxs;
	}

	if (!lua_istable(L, 4))
	{
		luaL_error(L, "trap_Trace: \"end\" should be an instance of table");
	}
	VectorCopy(*_etH_toVec3(L, 4), end);

	entNum = luaL_checkinteger(L, 5);
	mask   = luaL_checkinteger(L, 6);

	trap_Trace(&tr, start, *minsPtr, *maxsPtr, end, entNum, mask);
	_et_gettrace(L, &tr);

	return 1;
}

// Clients

/**
 * @brief _et_trap_SendServerCommand
 *        et.trap_SendServerCommand( clientnum, command )
 * @param[in] clientNum
 * @param[in] command
 * @return
 */
static int _et_trap_SendServerCommand(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	const char *cmd      = luaL_checkstring(L, 2);

	trap_SendServerCommand(clientnum, cmd);
	return 0;
}

/**
 * @brief _et_trap_DropClient
 *        et.trap_DropClient( clientnum, reason, ban_time )
 * @param[in] clientNum
 * @param[in] reason
 * @param[in] ban_time
 * @return
 */
static int _et_trap_DropClient(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	const char *reason   = luaL_checkstring(L, 2);
	int        ban_time  = (int)luaL_checkinteger(L, 3);

	trap_DropClient(clientnum, reason, ban_time);
	return 0;
}

/**
 * @brief _et_ClientNumberFromString searches for one partial match with @p string,
 *        if one is found the clientnum is returned,
*         if there is none or more than one match nil is returned.
 *        clientnum = et.ClientNumberFromString( string )
 * @param[in] string
 * @return
 */
static int _et_ClientNumberFromString(lua_State *L)
{
	const char *search = luaL_checkstring(L, 1);
	int        pids[MAX_CLIENTS];

	// only send exact matches, otherwise -1
	if (TVG_ClientNumbersFromString((char *)search, pids) == 1)
	{
		lua_pushinteger(L, pids[0]);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

/**
 * @brief _et_MasterClientNumberFromString searches through master clients for one partial match with @p string,
 *        if one is found the clientnum is returned,
*         if there is none or more than one match nil is returned.
 *        clientnum = et.MasterClientNumberFromString( string )
 * @param[in] string
 * @return
 */
static int _et_MasterClientNumberFromString(lua_State *L)
{
	const char *search = luaL_checkstring(L, 1);
	int        pids[MAX_CLIENTS];

	// only send exact matches, otherwise -1
	if (TVG_MasterClientNumbersFromString((char *)search, pids) == 1)
	{
		lua_pushinteger(L, pids[0]);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

/**
 * @brief _et_TVG_Say
 *        et.TVG_Say( clientNum, mode, text )
 * @param[in] clientNum
 * @param[in] mode
 * @param[in] text
 * @return
 */
static int _et_TVG_Say(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	int        mode      = (int)luaL_checkinteger(L, 2);
	const char *text     = luaL_checkstring(L, 3);

	TVG_Say(level.clients + clientnum, NULL, mode, text);
	return 0;
}

/**
 * @brief _et_MutePlayer duration is in seconds.
 *        et.MutePlayer( clientnum, duration, reason )
 * @param[in] clientNum
 * @param[in] duration
 * @param[in] reason
 * @return
 */
static int _et_MutePlayer(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	gclient_t  *client   = level.clients + clientnum;
	int        duration  = (int)luaL_checkinteger(L, 2);
	const char *reason   = luaL_optstring(L, 3, NULL);

	if (!client || clientnum < 0 || clientnum >= level.maxclients)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	client->sess.muted = qtrue;

	if (duration == -1)
	{
		if (reason == NULL)
		{
			CPx(clientnum, va("print \"You've been muted by Lua.\n\""));
			AP(va("chat \"%s^7 has been muted by Lua.\"", client->pers.netname));
		}
		else
		{
			CPx(clientnum, va("print \"You've been muted by Lua. %s\n\"", reason));
			AP(va("chat \"%s^7 has been muted by Lua. %s\"", client->pers.netname, reason));
		}
	}
	else
	{
		if (reason == NULL)
		{
			CPx(clientnum, va("print \"You've been muted for ^3%d^7 seconds by Lua.\n\"", duration));
			AP(va("chat \"%s^7 has been muted for ^3%d^7 seconds by Lua.\"", client->pers.netname, duration));
		}
		else
		{
			CPx(clientnum, va("print \"You've been muted for ^3%d^7 seconds by Lua. %s\n\"", duration, reason));
			AP(va("chat \"%s^7 has been muted for ^3%d^7 seconds by Lua. %s\"", client->pers.netname, duration, reason));
		}
	}
	return 0;
}

/**
 * @brief _et_UnmutePlayer
 *        et.UnmutePlayer( clientnum )
 * @param[in] clientNum
 * @return
 */
static int _et_UnmutePlayer(lua_State *L)
{
	int       clientnum = (int)luaL_checkinteger(L, 1);
	gclient_t *client   = level.clients + clientnum;

	if (!client || clientnum < 0 || clientnum >= level.maxclients)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	client->sess.muted = qfalse;

	CPx(clientnum, "print \"^5You've been auto-unmuted. Lua penalty lifted.\n\"");
	AP(va("chat \"%s^7 has been auto-unmuted. Lua penalty lifted.\"", client->pers.netname));
	return 0;
}

/**
 * @brief _et_TeleportPlayer teleport shifts origin[2] by +1
 *        et.TeleportPlayer( clientnum, vec3_t origin, vec3_t angles )
 * @param[in] clientNum
 * @return
 */
static int _et_TeleportPlayer(lua_State *L)
{
	int       clientnum = (int)luaL_checkinteger(L, 1);
	gclient_t *client   = level.clients + clientnum;
	vec3_t    origin, angles;

	if (!lua_istable(L, 2))
	{
		luaL_error(L, "et.TeleportPlayer: \"origin\" argument should be an instance of table");
		return 0;
	}

	if (!lua_istable(L, 3))
	{
		luaL_error(L, "et.TeleportPlayer: \"angles\" should be an instance of table");
		return 0;
	}

	if (!client || clientnum < 0 || clientnum >= level.maxclients)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	VectorCopy(*_etH_toVec3(L, 2), origin);
	VectorCopy(*_etH_toVec3(L, 3), angles);

	TVG_TeleportPlayer(client, origin, angles);

	return 0;
}

// Userinfo

/**
 * @brief _et_trap_GetUserinfo
 *        userinfo = et.trap_GetUserinfo( clientnum )
 * @param[in] clientNum
 * @return userinfo
 */
static int _et_trap_GetUserinfo(lua_State *L)
{
	char buff[MAX_STRING_CHARS];
	int  clientnum = (int)luaL_checkinteger(L, 1);

	trap_GetUserinfo(clientnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

/**
 * @brief _et_trap_SetUserinfo
 *        et.trap_SetUserinfo( clientnum, userinfo )
 * @param[in] clientNum
 * @param[in] userinfo
 * @return
 */
static int _et_trap_SetUserinfo(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	const char *userinfo = luaL_checkstring(L, 2);

	trap_SetUserinfo(clientnum, userinfo);
	return 0;
}

/**
 * @brief _et_ClientUserinfoChanged
 *        et.ClientUserinfoChanged( clientNum )
 * @param[in] clientNum
 * @return
 */
static int _et_ClientUserinfoChanged(lua_State *L)
{
	int clientnum = (int)luaL_checkinteger(L, 1);

	TVG_ClientUserinfoChanged(clientnum);
	return 0;
}

/** @}*/ // doxygen addtogroup lua_etfncs

/**
 * @brief et library initialisation array
 */
static const luaL_Reg etlib[] =
{
	// ET Library Calls
	{ "RegisterModname",              _et_RegisterModname              },
	{ "FindSelf",                     _et_FindSelf                     },
	{ "FindMod",                      _et_FindMod                      },
	{ "IPCSend",                      _et_IPCSend                      },

	// Printing
	{ "G_Print",                      _et_G_Print                      },
	{ "G_LogPrint",                   _et_G_LogPrint                   },

	// Argument Handling
	{ "ConcatArgs",                   _et_ConcatArgs                   },
	{ "trap_Argc",                    _et_trap_Argc                    },
	{ "trap_Argv",                    _et_trap_Argv                    },

	// Cvars
	{ "trap_Cvar_Get",                _et_trap_Cvar_Get                },
	{ "trap_Cvar_Set",                _et_trap_Cvar_Set                },

	// Config Strings
	{ "trap_GetConfigstring",         _et_trap_GetConfigstring         },
	{ "trap_SetConfigstring",         _et_trap_SetConfigstring         },

	// Server
	{ "trap_SendConsoleCommand",      _et_trap_SendConsoleCommand      },

	// Clients
	{ "trap_SendServerCommand",       _et_trap_SendServerCommand       },
	{ "trap_DropClient",              _et_trap_DropClient              },
	{ "ClientNumberFromString",       _et_ClientNumberFromString       },
	{ "MasterClientNumberFromString", _et_MasterClientNumberFromString },

	{ "TVG_Say",                      _et_TVG_Say                      },
	{ "MutePlayer",                   _et_MutePlayer                   },
	{ "UnmutePlayer",                 _et_UnmutePlayer                 },
	{ "TeleportPlayer",               _et_TeleportPlayer               },

	// Userinfo
	{ "trap_GetUserinfo",             _et_trap_GetUserinfo             },
	{ "trap_SetUserinfo",             _et_trap_SetUserinfo             },
	{ "ClientUserinfoChanged",        _et_ClientUserinfoChanged        },

	// String Utility
	{ "Info_RemoveKey",               _et_Info_RemoveKey               },
	{ "Info_SetValueForKey",          _et_Info_SetValueForKey          },
	{ "Info_ValueForKey",             _et_Info_ValueForKey             },
	{ "Q_CleanStr",                   _et_Q_CleanStr                   },

	// ET Filesystem
	{ "trap_FS_FOpenFile",            _et_trap_FS_FOpenFile            },
	{ "trap_FS_Read",                 _et_trap_FS_Read                 },
	{ "trap_FS_Write",                _et_trap_FS_Write                },
	{ "trap_FS_Rename",               _et_trap_FS_Rename               },
	{ "trap_FS_FCloseFile",           _et_trap_FS_FCloseFile           },
	{ "trap_FS_GetFileList",          _et_trap_FS_GetFileList          },

	// Indexes
	{ "TVG_SoundIndex",               _et_TVG_SoundIndex               },
	{ "TVG_ModelIndex",               _et_TVG_ModelIndex               },

	// Miscellaneous
	{ "trap_Milliseconds",            _et_trap_Milliseconds            },
	{ "isBitSet",                     _et_isBitSet                     },

	// Entities
	{ "gentity_get",                  _et_gentity_get                  },
	{ "level_get",                    _et_level_get                    },
	{ "level_set",                    _et_level_set                    },
	{ "ps_get",                       _et_ps_get                       },
	{ "gclient_get",                  _et_gclient_get                  },
	{ "gclient_set",                  _et_gclient_set                  },

	{ "TVG_AddEvent",                 _et_TVG_AddEvent                 },
	{ "trap_Trace",                   _et_trap_Trace                   },
	{ NULL },
};

/*************/
/* Lua API   */
/*************/

/**
 * @brief TVG_LuaRunIsolated creates and runs specified module in isolated state
 * @param[in] modName
 * @return
 */
qboolean TVG_LuaRunIsolated(const char *modName)
{
	int          freeVM, flen = 0;
	static char  allowedModules[MAX_CVAR_VALUE_STRING];
	char         *code, *signature;
	fileHandle_t f;
	lua_vm_t     *vm;

	for (freeVM = 0; freeVM < LUA_NUM_VM; freeVM++)
	{
		if (lVM[freeVM] == NULL)
		{
			break;
		}
	}

	if (freeVM == LUA_NUM_VM)
	{
		G_Printf("%s API: %sno free VMs left to load module: \"%s\" \n", LUA_VERSION, S_COLOR_BLUE, modName);
		return qfalse;
	}

	Q_strncpyz(allowedModules, Q_strupr(lua_allowedModules.string), sizeof(allowedModules));

	// try to open lua file
	flen = trap_FS_FOpenFile(modName, &f, FS_READ);
	if (flen < 0)
	{
		G_Printf("%s API: %scan not open file '%s'\n", LUA_VERSION, S_COLOR_BLUE, modName);
	}
	else if (flen > LUA_MAX_FSIZE)
	{
		// Let's not load arbitrarily big files to memory.
		// If your lua file exceeds the limit, let me know.
		G_Printf("%s API: %signoring file '%s' (too big)\n", LUA_VERSION, S_COLOR_BLUE, modName);
		trap_FS_FCloseFile(f);
	}
	else
	{
		code = Com_Allocate(flen + 1);

		if (code == NULL)
		{
			G_Error("%s API: %smemory allocation error for '%s' data\n", LUA_VERSION, S_COLOR_BLUE, modName);
		}

		trap_FS_Read(code, flen, f);
		*(code + flen) = '\0';
		trap_FS_FCloseFile(f);
		signature = G_SHA1(code);

		if (Q_stricmp(lua_allowedModules.string, "") && !strstr(allowedModules, signature))
		{
			// don't load disallowed lua modules into vm
			Com_Dealloc(code); // fixed memory leaking in Lua API - thx ETPub/goesa
			G_Printf("%s API: %sLua module [%s] [%s] disallowed by ACL\n", LUA_VERSION, S_COLOR_BLUE, modName, signature);
		}
		else
		{
			// Init lua_vm_t struct
			vm = (lua_vm_t *)Com_Allocate(sizeof(lua_vm_t));

			if (vm == NULL)
			{
				G_Error("%s API: %svm memory allocation error for %s data\n", LUA_VERSION, S_COLOR_BLUE, modName);
			}

			vm->id = -1;
			Q_strncpyz(vm->file_name, modName, sizeof(vm->file_name));
			Q_strncpyz(vm->mod_name, "", sizeof(vm->mod_name));
			Q_strncpyz(vm->mod_signature, signature, sizeof(vm->mod_signature));
			vm->code      = code;
			vm->code_size = flen;
			vm->err       = 0;

			// Start lua virtual machine
			if (TVG_LuaStartVM(vm))
			{
				vm->id      = freeVM;
				lVM[freeVM] = vm;
				return qtrue;
			}
			else
			{
				TVG_LuaStopVM(vm);
			}
		}
	}
	return qfalse;
}

/**
 * @brief TVG_LuaInit initialises the Lua API interface
 * @return
 */
qboolean TVG_LuaInit(void)
{
	int          i, num_vm = 0, len;
	char         buff[MAX_CVAR_VALUE_STRING], *crt, *list, *pList;
	fileHandle_t f;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		lVM[i] = NULL;
	}

	if (tvg_luaModuleList.string[0])
	{
		if (lua_modules.string[0])
		{
			G_Printf("%s API: %slua_modules cvar will be ignored since tvg_luaModuleList is set\n", LUA_VERSION, S_COLOR_BLUE);
		}

		len = trap_FS_FOpenFile(tvg_luaModuleList.string, &f, FS_READ);
		if (len < 0)
		{
			G_Printf("%s API: %scan not open file '%s'\n", LUA_VERSION, S_COLOR_BLUE, tvg_luaModuleList.string);
			return qfalse;
		}

		list = Com_Allocate(len + 1);

		if (list == NULL)
		{
			G_Error("%s API: %smemory allocation error for '%s' data\n", LUA_VERSION, S_COLOR_BLUE, tvg_luaModuleList.string);
			return qfalse;
		}

		trap_FS_Read(list, len, f);
		*(list + len) = '\0';
		trap_FS_FCloseFile(f);

		pList = list;
		while ((crt = COM_Parse(&pList)) && *crt)
		{
			if (num_vm >= LUA_NUM_VM)
			{
				G_Printf("%s API: %stoo many lua files specified, only the first %d have been loaded\n", LUA_VERSION, S_COLOR_BLUE, LUA_NUM_VM);
				break;
			}

			if (TVG_LuaRunIsolated(crt))
			{
				num_vm++;
			}
		}

		Com_Dealloc(list);
	}
	else if (lua_modules.string[0])
	{
		Q_strncpyz(buff, lua_modules.string, sizeof(buff));
		len = strlen(buff);
		crt = buff;
		for (i = 0; i <= len; i++)
		{
			if (buff[i] == ' ' || buff[i] == '\0' || buff[i] == ',' || buff[i] == ';')
			{
				buff[i] = '\0';

				if (num_vm >= LUA_NUM_VM)
				{
					G_Printf("%s API: %stoo many lua files specified, only the first %d have been loaded\n", LUA_VERSION, S_COLOR_BLUE, LUA_NUM_VM);
					break;
				}

				if (TVG_LuaRunIsolated(crt))
				{
					num_vm++;
				}

				// prepare for next iteration
				if (i + 1 < len)
				{
					crt = buff + i + 1;
				}
				else
				{
					crt = NULL;
				}
			}
		}
	}
	else
	{
		G_Printf("%s API: %sno Lua files set\n", LUA_VERSION, S_COLOR_BLUE);
	}

	return qtrue;
}

/**
 * @brief TVG_LuaCall calls a function already on the stack.
 * @param[in] vm
 * @param[in] func
 * @param[in] nargs
 * @param[in] nresults
 * @return
 */
qboolean TVG_LuaCall(lua_vm_t *vm, const char *func, int nargs, int nresults)
{
	switch (lua_pcall(vm->L, nargs, nresults, 0))
	{
	case LUA_ERRRUN:
		// made output more ETPro compatible
		G_Printf("%s API: %s%s error running lua script: '%s'\n", LUA_VERSION, S_COLOR_BLUE, func, lua_tostring(vm->L, -1));
		lua_pop(vm->L, 1);
		vm->err++;
		return qfalse;
	case LUA_ERRMEM:
		G_Printf("%s API: %smemory allocation error #2 ( %s )\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);
		vm->err++;
		return qfalse;
	case LUA_ERRERR:
		G_Printf("%s API: %straceback error ( %s )\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);
		vm->err++;
		return qfalse;
	default:
		return qtrue;
	}
	return qtrue;
}

/**
 * @brief TVG_LuaGetNamedFunction finds a function by name and puts it onto the stack
                                  if the function does not exist, returns qfalse.
 * @param[in] vm
 * @param[in] name
 * @return
 */
qboolean TVG_LuaGetNamedFunction(lua_vm_t *vm, const char *name)
{
	if (vm->L)
	{
		lua_getglobal(vm->L, name);
		if (lua_isfunction(vm->L, -1))
		{
			return qtrue;
		}
		else
		{
			lua_pop(vm->L, 1);
			return qfalse;
		}
	}
	return qfalse;
}

/**
 * @brief TVG_LuaStackDump dump the lua stack to console
 *                         executed by the ingame "lua_api" command
 */
void TVG_LuaStackDump(void)
{
	lua_vm_t *vm = (lua_vm_t *) Com_Allocate(sizeof(lua_vm_t));

	if (vm == NULL)
	{
		G_Printf("%s API: %smemory allocation error\n", LUA_VERSION, S_COLOR_BLUE);
		return;
	}

	Q_strncpyz(vm->file_name, "current API available to scripts", sizeof(vm->file_name));
	vm->code      = "";
	vm->code_size = 0;
	vm->err       = 0;

	// Start lua virtual machine
	if (TVG_LuaStartVM(vm))
	{
		lua_State *L = vm->L;

		lua_getglobal(L, "et");
		if (!lua_istable(L, -1))
		{
			G_Printf("%s API: %serror - et prefix is not correctly registered\n", LUA_VERSION, S_COLOR_BLUE);
		}
		else
		{
			int i, types[] = { LUA_TSTRING, LUA_TTABLE, LUA_TBOOLEAN, LUA_TNUMBER, LUA_TFUNCTION };

			G_Printf("----------------------------------------------------------------\n");
			G_Printf("%-42s%-17s%-10s\n", "Name", "Type", "Value");
			G_Printf("----------------------------------------------------------------\n");

			// et namespace
			for (i = 0; i < ARRAY_LEN(types); i++)
			{
				lua_pushnil(L); // stack now contains: -1 => nil; -2 => table
				while (lua_next(L, -2))
				{
					// order by variable data type
					if (lua_type(L, -1) == types[i])
					{
						G_Printf("et.%-39s^%i%-17s^7%-10s\n", lua_tostring(L, -2), i, lua_typename(L, lua_type(L, -1)), (lua_isfunction(L, -1) ? "N/A" : lua_tostring(L, -1)));
					}
					lua_pop(L, 1);
				}
			}
			// globals
			lua_pushglobaltable(L);
			lua_pushnil(L);
			while (lua_next(L, -2))
			{
				if (lua_type(L, -1) == LUA_TSTRING)
				{
					G_Printf("%-42s^8%-17s^7%-10s\n", lua_tostring(L, -2), "global string", lua_tostring(L, -1));
				}
				lua_pop(L, 1);
			}
		}
	}
	lua_close(vm->L);
	vm->L = NULL;
	Com_Dealloc(vm);
}

/**
 * @brief TVG_RegisterConfigstringConstants mod dependent
 * @param[in] vm
 */
static void TVG_RegisterConfigstringConstants(lua_vm_t *vm)
{
	// Config string:
	// q_shared.h
	lua_regconstinteger(vm->L, CS_SERVERINFO); // an info string with all the serverinfo cvars
	lua_regconstinteger(vm->L, CS_SYSTEMINFO); // an info string for server system to client system configuration (timescale, etc)

	// bg_public.h
	lua_regconstinteger(vm->L, CS_MUSIC);
	lua_regconstinteger(vm->L, CS_MESSAGE);    // from the map worldspawn's message field
	lua_regconstinteger(vm->L, CS_MOTD);       // g_motd string for server message of the day
	lua_regconstinteger(vm->L, CS_WARMUP);     // server time when the match will be restarted
	lua_regconstinteger(vm->L, CS_VOTE_TIME);
	lua_regconstinteger(vm->L, CS_VOTE_STRING);
	lua_regconstinteger(vm->L, CS_VOTE_YES);
	lua_regconstinteger(vm->L, CS_VOTE_NO);
	lua_regconstinteger(vm->L, CS_GAME_VERSION);

	lua_regconstinteger(vm->L, CS_LEVEL_START_TIME); // so the timer only shows the current level
	lua_regconstinteger(vm->L, CS_INTERMISSION);     // when 1, intermission will start in a second or two
	lua_regconstinteger(vm->L, CS_MULTI_INFO);
	lua_regconstinteger(vm->L, CS_MULTI_MAPWINNER);
	lua_regconstinteger(vm->L, CS_MULTI_OBJECTIVE);

	lua_regconstinteger(vm->L, CS_SCREENFADE); // used to tell clients to fade their screen to black/normal
	lua_regconstinteger(vm->L, CS_FOGVARS);    // used for saving the current state/settings of the fog
	lua_regconstinteger(vm->L, CS_SKYBOXORG);  // this is where we should view the skybox from

	lua_regconstinteger(vm->L, CS_TARGETEFFECT);
	lua_regconstinteger(vm->L, CS_WOLFINFO);
	lua_regconstinteger(vm->L, CS_FIRSTBLOOD);            // Team that has first blood
	lua_regconstinteger(vm->L, CS_ROUNDSCORES1);          // Axis round wins
	lua_regconstinteger(vm->L, CS_ROUNDSCORES2);          // Allied round wins
	lua_regconstinteger(vm->L, CS_MAIN_AXIS_OBJECTIVE);   // Most important current objective
	lua_regconstinteger(vm->L, CS_MAIN_ALLIES_OBJECTIVE); // Most important current objective
	lua_regconstinteger(vm->L, CS_MUSIC_QUEUE);
	lua_regconstinteger(vm->L, CS_SCRIPT_MOVER_NAMES);
	lua_regconstinteger(vm->L, CS_CONSTRUCTION_NAMES);

	lua_regconstinteger(vm->L, CS_VERSIONINFO);           // Versioning info for demo playback compatibility
	lua_regconstinteger(vm->L, CS_REINFSEEDS);            // Reinforcement
	lua_regconstinteger(vm->L, CS_SERVERTOGGLES);         // Shows current enable/disabled settings (for voting UI)
	lua_regconstinteger(vm->L, CS_GLOBALFOGVARS);
	lua_regconstinteger(vm->L, CS_AXIS_MAPS_XP);
	lua_regconstinteger(vm->L, CS_ALLIED_MAPS_XP);
	lua_regconstinteger(vm->L, CS_INTERMISSION_START_TIME);
	lua_regconstinteger(vm->L, CS_ENDGAME_STATS);
	lua_regconstinteger(vm->L, CS_CHARGETIMES);
	lua_regconstinteger(vm->L, CS_FILTERCAMS);

	lua_regconstinteger(vm->L, CS_MODINFO);
	lua_regconstinteger(vm->L, CS_SVCVAR);
	lua_regconstinteger(vm->L, CS_CONFIGNAME);

	lua_regconstinteger(vm->L, CS_TEAMRESTRICTIONS);
	lua_regconstinteger(vm->L, CS_UPGRADERANGE);

	lua_regconstinteger(vm->L, CS_MODELS);
	lua_regconstinteger(vm->L, CS_SOUNDS);
	lua_regconstinteger(vm->L, CS_SHADERS);
	lua_regconstinteger(vm->L, CS_SHADERSTATE);
	lua_regconstinteger(vm->L, CS_SKINS);
	lua_regconstinteger(vm->L, CS_CHARACTERS);
	lua_regconstinteger(vm->L, CS_PLAYERS);
	lua_regconstinteger(vm->L, CS_MULTI_SPAWNTARGETS);
	lua_regconstinteger(vm->L, CS_OID_TRIGGERS);
	lua_regconstinteger(vm->L, CS_OID_DATA);
	lua_regconstinteger(vm->L, CS_DLIGHTS);
	lua_regconstinteger(vm->L, CS_SPLINES);
	lua_regconstinteger(vm->L, CS_TAGCONNECTS);
	lua_regconstinteger(vm->L, CS_FIRETEAMS);
	lua_regconstinteger(vm->L, CS_CUSTMOTD);
	lua_regconstinteger(vm->L, CS_STRINGS);
	lua_regconstinteger(vm->L, CS_MAX);
}

/**
 * @brief TVG_RegisterPowerupConstants mod dependent
 * @param[in] vm
 */
static void TVG_RegisterPowerupConstants(lua_vm_t *vm)
{
	lua_regconstinteger(vm->L, PW_NONE);
	lua_regconstinteger(vm->L, PW_INVULNERABLE);
	lua_regconstinteger(vm->L, PW_NOFATIGUE);
	lua_regconstinteger(vm->L, PW_REDFLAG);
	lua_regconstinteger(vm->L, PW_BLUEFLAG);
	lua_regconstinteger(vm->L, PW_OPS_DISGUISED);
	lua_regconstinteger(vm->L, PW_OPS_CLASS_1);
	lua_regconstinteger(vm->L, PW_OPS_CLASS_2);
	lua_regconstinteger(vm->L, PW_OPS_CLASS_3);
	lua_regconstinteger(vm->L, PW_ADRENALINE);
	lua_regconstinteger(vm->L, PW_BLACKOUT);

	lua_regconstinteger(vm->L, PW_NUM_POWERUPS);
}

/**
 * @brief TVG_RegisterModConstants mod dependent
 * @param[in] vm
 */
static void TVG_RegisterWeaponConstants(lua_vm_t *vm)
{
	lua_regconstinteger(vm->L, WP_NONE);                 // 0
	lua_regconstinteger(vm->L, WP_KNIFE);                // 1
	lua_regconstinteger(vm->L, WP_LUGER);                // 2
	lua_regconstinteger(vm->L, WP_MP40);                 // 3
	lua_regconstinteger(vm->L, WP_GRENADE_LAUNCHER);     // 4
	lua_regconstinteger(vm->L, WP_PANZERFAUST);          // 5
	lua_regconstinteger(vm->L, WP_FLAMETHROWER);         // 6
	lua_regconstinteger(vm->L, WP_COLT);                 // 7 - equivalent american weapon to german luger
	lua_regconstinteger(vm->L, WP_THOMPSON);             // 8 - equivalent american weapon to german mp40
	lua_regconstinteger(vm->L, WP_GRENADE_PINEAPPLE);    // 9

	lua_regconstinteger(vm->L, WP_STEN);                 // 10 - silenced sten sub-machinegun
	lua_regconstinteger(vm->L, WP_MEDIC_SYRINGE);        // 11 - broken out from CLASS_SPECIAL per Id request
	lua_regconstinteger(vm->L, WP_AMMO);                 // 12 - likewise
	lua_regconstinteger(vm->L, WP_ARTY);                 // 13
	lua_regconstinteger(vm->L, WP_SILENCER);             // 14 - used to be sp5
	lua_regconstinteger(vm->L, WP_DYNAMITE);             // 15
	lua_regconstinteger(vm->L, WP_SMOKETRAIL);           // 16
	lua_regconstinteger(vm->L, WP_MAPMORTAR);            // 17
	lua_regconstinteger(vm->L, VERYBIGEXPLOSION);        // 18 - explosion effect for airplanes
	lua_regconstinteger(vm->L, WP_MEDKIT);               // 19

	lua_regconstinteger(vm->L, WP_BINOCULARS);           // 20
	lua_regconstinteger(vm->L, WP_PLIERS);               // 21
	lua_regconstinteger(vm->L, WP_SMOKE_MARKER);         // 22 - changed name to cause less confusion
	lua_regconstinteger(vm->L, WP_KAR98);                // 23 - WolfXP weapons
	lua_regconstinteger(vm->L, WP_CARBINE);              // 24
	lua_regconstinteger(vm->L, WP_GARAND);               // 25
	lua_regconstinteger(vm->L, WP_LANDMINE);             // 26
	lua_regconstinteger(vm->L, WP_SATCHEL);              // 27
	lua_regconstinteger(vm->L, WP_SATCHEL_DET);          // 28
	lua_regconstinteger(vm->L, WP_SMOKE_BOMB);           // 29

	lua_regconstinteger(vm->L, WP_MOBILE_MG42);          // 30
	lua_regconstinteger(vm->L, WP_K43);                  // 31
	lua_regconstinteger(vm->L, WP_FG42);                 // 32
	lua_regconstinteger(vm->L, WP_DUMMY_MG42);           // 33 - for storing heat on mounted mg42s...
	lua_regconstinteger(vm->L, WP_MORTAR);               // 34
	lua_regconstinteger(vm->L, WP_AKIMBO_COLT);          // 35
	lua_regconstinteger(vm->L, WP_AKIMBO_LUGER);         // 36

	lua_regconstinteger(vm->L, WP_GPG40);                // 37
	lua_regconstinteger(vm->L, WP_M7);                   // 38
	lua_regconstinteger(vm->L, WP_SILENCED_COLT);        // 39

	lua_regconstinteger(vm->L, WP_GARAND_SCOPE);         // 40
	lua_regconstinteger(vm->L, WP_K43_SCOPE);            // 41
	lua_regconstinteger(vm->L, WP_FG42_SCOPE);           // 42
	lua_regconstinteger(vm->L, WP_MORTAR_SET);           // 43
	lua_regconstinteger(vm->L, WP_MEDIC_ADRENALINE);     // 44
	lua_regconstinteger(vm->L, WP_AKIMBO_SILENCEDCOLT);  // 45
	lua_regconstinteger(vm->L, WP_AKIMBO_SILENCEDLUGER); // 46
	lua_regconstinteger(vm->L, WP_MOBILE_MG42_SET);      // 47

	// league weapons
	lua_regconstinteger(vm->L, WP_KNIFE_KABAR);          // 48
	lua_regconstinteger(vm->L, WP_MOBILE_BROWNING);      // 49
	lua_regconstinteger(vm->L, WP_MOBILE_BROWNING_SET);  // 50
	lua_regconstinteger(vm->L, WP_MORTAR2);              // 51
	lua_regconstinteger(vm->L, WP_MORTAR2_SET);          // 52
	lua_regconstinteger(vm->L, WP_BAZOOKA);              // 53
	lua_regconstinteger(vm->L, WP_MP34);                 // 54
	lua_regconstinteger(vm->L, WP_AIRSTRIKE);            // 55
	lua_regconstinteger(vm->L, WP_NUM_WEAPONS);
}

/**
 * @brief TVG_RegisterModConstants mod dependent
 * @param[in] vm
 */
static void TVG_RegisterModConstants(lua_vm_t *vm)
{
	lua_regconstinteger(vm->L, MOD_UNKNOWN);
	lua_regconstinteger(vm->L, MOD_MACHINEGUN);
	lua_regconstinteger(vm->L, MOD_BROWNING);
	lua_regconstinteger(vm->L, MOD_MG42);
	lua_regconstinteger(vm->L, MOD_GRENADE);

	// modified wolf weap mods
	lua_regconstinteger(vm->L, MOD_KNIFE);
	lua_regconstinteger(vm->L, MOD_LUGER);
	lua_regconstinteger(vm->L, MOD_COLT);
	lua_regconstinteger(vm->L, MOD_MP40);
	lua_regconstinteger(vm->L, MOD_THOMPSON);
	lua_regconstinteger(vm->L, MOD_STEN);
	lua_regconstinteger(vm->L, MOD_GARAND);

	lua_regconstinteger(vm->L, MOD_SILENCER);
	lua_regconstinteger(vm->L, MOD_FG42);
	lua_regconstinteger(vm->L, MOD_FG42SCOPE);
	lua_regconstinteger(vm->L, MOD_PANZERFAUST);
	lua_regconstinteger(vm->L, MOD_GRENADE_LAUNCHER);
	lua_regconstinteger(vm->L, MOD_FLAMETHROWER);
	lua_regconstinteger(vm->L, MOD_GRENADE_PINEAPPLE);

	lua_regconstinteger(vm->L, MOD_MAPMORTAR);
	lua_regconstinteger(vm->L, MOD_MAPMORTAR_SPLASH);

	lua_regconstinteger(vm->L, MOD_KICKED);

	lua_regconstinteger(vm->L, MOD_DYNAMITE);
	lua_regconstinteger(vm->L, MOD_AIRSTRIKE);
	lua_regconstinteger(vm->L, MOD_SYRINGE);
	lua_regconstinteger(vm->L, MOD_AMMO);
	lua_regconstinteger(vm->L, MOD_ARTY);

	lua_regconstinteger(vm->L, MOD_WATER);
	lua_regconstinteger(vm->L, MOD_SLIME);
	lua_regconstinteger(vm->L, MOD_LAVA);
	lua_regconstinteger(vm->L, MOD_CRUSH);
	lua_regconstinteger(vm->L, MOD_TELEFRAG);
	lua_regconstinteger(vm->L, MOD_FALLING);
	lua_regconstinteger(vm->L, MOD_SUICIDE);
	lua_regconstinteger(vm->L, MOD_TARGET_LASER);
	lua_regconstinteger(vm->L, MOD_TRIGGER_HURT);
	lua_regconstinteger(vm->L, MOD_EXPLOSIVE);

	lua_regconstinteger(vm->L, MOD_CARBINE);
	lua_regconstinteger(vm->L, MOD_KAR98);
	lua_regconstinteger(vm->L, MOD_GPG40);
	lua_regconstinteger(vm->L, MOD_M7);
	lua_regconstinteger(vm->L, MOD_LANDMINE);
	lua_regconstinteger(vm->L, MOD_SATCHEL);

	lua_regconstinteger(vm->L, MOD_SMOKEBOMB);
	lua_regconstinteger(vm->L, MOD_MOBILE_MG42);
	lua_regconstinteger(vm->L, MOD_SILENCED_COLT);
	lua_regconstinteger(vm->L, MOD_GARAND_SCOPE);

	lua_regconstinteger(vm->L, MOD_CRUSH_CONSTRUCTION);
	lua_regconstinteger(vm->L, MOD_CRUSH_CONSTRUCTIONDEATH);
	lua_regconstinteger(vm->L, MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER);

	lua_regconstinteger(vm->L, MOD_K43);
	lua_regconstinteger(vm->L, MOD_K43_SCOPE);

	lua_regconstinteger(vm->L, MOD_MORTAR);

	lua_regconstinteger(vm->L, MOD_AKIMBO_COLT);
	lua_regconstinteger(vm->L, MOD_AKIMBO_LUGER);
	lua_regconstinteger(vm->L, MOD_AKIMBO_SILENCEDCOLT);
	lua_regconstinteger(vm->L, MOD_AKIMBO_SILENCEDLUGER);

	lua_regconstinteger(vm->L, MOD_SMOKEGRENADE);

	lua_regconstinteger(vm->L, MOD_SWAP_PLACES);

	// keep these 2 entries last
	lua_regconstinteger(vm->L, MOD_SWITCHTEAM);

	lua_regconstinteger(vm->L, MOD_SHOVE);

	lua_regconstinteger(vm->L, MOD_KNIFE_KABAR);
	lua_regconstinteger(vm->L, MOD_MOBILE_BROWNING);
	lua_regconstinteger(vm->L, MOD_MORTAR2);
	lua_regconstinteger(vm->L, MOD_BAZOOKA);
	lua_regconstinteger(vm->L, MOD_BACKSTAB);
	lua_regconstinteger(vm->L, MOD_MP34);

	lua_regconstinteger(vm->L, MOD_NUM_MODS);
}

/**
 * @brief TVG_RegisterSurfaceConstants
 * @param[in] vm
 */
static void TVG_RegisterSurfaceConstants(lua_vm_t *vm)
{
	lua_regconstinteger(vm->L, CONTENTS_NONE);
	lua_regconstinteger(vm->L, CONTENTS_SOLID);
	lua_regconstinteger(vm->L, CONTENTS_LIGHTGRID);
	lua_regconstinteger(vm->L, CONTENTS_LAVA);
	lua_regconstinteger(vm->L, CONTENTS_SLIME);
	lua_regconstinteger(vm->L, CONTENTS_WATER);
	lua_regconstinteger(vm->L, CONTENTS_FOG);
	lua_regconstinteger(vm->L, CONTENTS_MISSILECLIP);
	lua_regconstinteger(vm->L, CONTENTS_ITEM);
	lua_regconstinteger(vm->L, CONTENTS_MOVER);
	lua_regconstinteger(vm->L, CONTENTS_AREAPORTAL);
	lua_regconstinteger(vm->L, CONTENTS_PLAYERCLIP);
	lua_regconstinteger(vm->L, CONTENTS_MONSTERCLIP);
	lua_regconstinteger(vm->L, CONTENTS_TELEPORTER);
	lua_regconstinteger(vm->L, CONTENTS_JUMPPAD);
	lua_regconstinteger(vm->L, CONTENTS_CLUSTERPORTAL);
	lua_regconstinteger(vm->L, CONTENTS_DONOTENTER);
	lua_regconstinteger(vm->L, CONTENTS_DONOTENTER_LARGE);
	lua_regconstinteger(vm->L, CONTENTS_ORIGIN);
	lua_regconstinteger(vm->L, CONTENTS_BODY);
	lua_regconstinteger(vm->L, CONTENTS_CORPSE);
	lua_regconstinteger(vm->L, CONTENTS_DETAIL);
	lua_regconstinteger(vm->L, CONTENTS_STRUCTURAL);
	lua_regconstinteger(vm->L, CONTENTS_TRANSLUCENT);
	lua_regconstinteger(vm->L, CONTENTS_TRIGGER);
	lua_regconstinteger(vm->L, CONTENTS_NODROP);

	lua_regconstinteger(vm->L, SURF_NODAMAGE);
	lua_regconstinteger(vm->L, SURF_SLICK);
	lua_regconstinteger(vm->L, SURF_SKY);
	lua_regconstinteger(vm->L, SURF_LADDER);
	lua_regconstinteger(vm->L, SURF_NOIMPACT);
	lua_regconstinteger(vm->L, SURF_NOMARKS);
	lua_regconstinteger(vm->L, SURF_SPLASH);
	lua_regconstinteger(vm->L, SURF_NODRAW);
	lua_regconstinteger(vm->L, SURF_HINT);
	lua_regconstinteger(vm->L, SURF_SKIP);
	lua_regconstinteger(vm->L, SURF_NOLIGHTMAP);
	lua_regconstinteger(vm->L, SURF_POINTLIGHT);
	lua_regconstinteger(vm->L, SURF_METAL);
	lua_regconstinteger(vm->L, SURF_NOSTEPS);
	lua_regconstinteger(vm->L, SURF_NONSOLID);
	lua_regconstinteger(vm->L, SURF_LIGHTFILTER);
	lua_regconstinteger(vm->L, SURF_ALPHASHADOW);
	lua_regconstinteger(vm->L, SURF_NODLIGHT);
	lua_regconstinteger(vm->L, SURF_WOOD);
	lua_regconstinteger(vm->L, SURF_GRASS);
	lua_regconstinteger(vm->L, SURF_GRAVEL);
	lua_regconstinteger(vm->L, SURF_GLASS);
	lua_regconstinteger(vm->L, SURF_SNOW);
	lua_regconstinteger(vm->L, SURF_ROOF);
	lua_regconstinteger(vm->L, SURF_RUBBLE);
	lua_regconstinteger(vm->L, SURF_CARPET);
	lua_regconstinteger(vm->L, SURF_MONSTERSLICK);
	lua_regconstinteger(vm->L, SURF_MONSLICK_W);
	lua_regconstinteger(vm->L, SURF_MONSLICK_N);
	lua_regconstinteger(vm->L, SURF_MONSLICK_E);
	lua_regconstinteger(vm->L, SURF_MONSLICK_S);
	lua_regconstinteger(vm->L, SURF_LANDMINE);

	lua_regconstinteger(vm->L, MASK_ALL);
	lua_regconstinteger(vm->L, MASK_SOLID);
	lua_regconstinteger(vm->L, MASK_PLAYERSOLID);
	lua_regconstinteger(vm->L, MASK_WATER);
	lua_regconstinteger(vm->L, MASK_OPAQUE);
	lua_regconstinteger(vm->L, MASK_SHOT);
	lua_regconstinteger(vm->L, MASK_MISSILESHOT);
}

/**
 * @brief TVG_RegisterConstants
 * @param[in] vm
 */
static void TVG_RegisterConstants(lua_vm_t *vm)
{
	// max constants
	// from q_shared.h
	lua_regconstinteger(vm->L, MAX_CLIENTS);
	lua_regconstinteger(vm->L, MAX_MODELS);
	lua_regconstinteger(vm->L, MAX_SOUNDS);
	lua_regconstinteger(vm->L, MAX_CS_SKINS);
	lua_regconstinteger(vm->L, MAX_CSSTRINGS);

	lua_regconstinteger(vm->L, MAX_CS_SHADERS);
	lua_regconstinteger(vm->L, MAX_SERVER_TAGS);
	lua_regconstinteger(vm->L, MAX_TAG_FILES);
	lua_regconstinteger(vm->L, MAX_MULTI_SPAWNTARGETS);
	lua_regconstinteger(vm->L, MAX_DLIGHT_CONFIGSTRINGS);
	lua_regconstinteger(vm->L, MAX_SPLINE_CONFIGSTRINGS);
	// misc bg_public.h
	lua_regconstinteger(vm->L, MAX_OID_TRIGGERS);
	lua_regconstinteger(vm->L, MAX_CHARACTERS);
	lua_regconstinteger(vm->L, MAX_TAGCONNECTS);
	lua_regconstinteger(vm->L, MAX_FIRETEAMS);
	lua_regconstinteger(vm->L, MAX_MOTDLINES);

	// GS constants
	lua_regconstinteger(vm->L, GS_INITIALIZE);
	lua_regconstinteger(vm->L, GS_PLAYING);
	lua_regconstinteger(vm->L, GS_WARMUP_COUNTDOWN);
	lua_regconstinteger(vm->L, GS_WARMUP);
	lua_regconstinteger(vm->L, GS_INTERMISSION);
	lua_regconstinteger(vm->L, GS_WAITING_FOR_PLAYERS);
	lua_regconstinteger(vm->L, GS_RESET);

	// TEAM constants
	lua_regconstinteger(vm->L, TEAM_FREE);
	lua_regconstinteger(vm->L, TEAM_AXIS);
	lua_regconstinteger(vm->L, TEAM_ALLIES);
	lua_regconstinteger(vm->L, TEAM_SPECTATOR);
	lua_regconstinteger(vm->L, TEAM_NUM_TEAMS);

	// SK constants
	lua_regconstinteger(vm->L, SK_BATTLE_SENSE);
	lua_regconstinteger(vm->L, SK_EXPLOSIVES_AND_CONSTRUCTION);
	lua_regconstinteger(vm->L, SK_FIRST_AID);
	lua_regconstinteger(vm->L, SK_SIGNALS);
	lua_regconstinteger(vm->L, SK_LIGHT_WEAPONS);
	lua_regconstinteger(vm->L, SK_HEAVY_WEAPONS);
	lua_regconstinteger(vm->L, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS);
	lua_regconstinteger(vm->L, SK_NUM_SKILLS);

	// EXEC constants
	lua_regconstinteger(vm->L, EXEC_NOW);
	lua_regconstinteger(vm->L, EXEC_INSERT);
	lua_regconstinteger(vm->L, EXEC_APPEND);

	// FS constants
	lua_regconstinteger(vm->L, FS_READ);
	lua_regconstinteger(vm->L, FS_WRITE);
	lua_regconstinteger(vm->L, FS_APPEND);
	lua_regconstinteger(vm->L, FS_APPEND_SYNC);

	// chat/message constants
	lua_regconstinteger(vm->L, SAY_ALL);
	lua_regconstinteger(vm->L, SAY_TEAM);
	lua_regconstinteger(vm->L, SAY_BUDDY);
	lua_regconstinteger(vm->L, SAY_TEAMNL);

	lua_regconststring(vm->L, HOSTARCH);

	// pmtype_t
	lua_regconstinteger(vm->L, PM_NORMAL);
	lua_regconstinteger(vm->L, PM_NOCLIP);
	lua_regconstinteger(vm->L, PM_SPECTATOR);
	lua_regconstinteger(vm->L, PM_FREEZE);
	lua_regconstinteger(vm->L, PM_INTERMISSION);

	// statIndex_t
	lua_regconstinteger(vm->L, STAT_HEALTH);
	lua_regconstinteger(vm->L, STAT_KEYS);
	lua_regconstinteger(vm->L, STAT_DEAD_YAW);
	lua_regconstinteger(vm->L, STAT_MAX_HEALTH);
	lua_regconstinteger(vm->L, STAT_PLAYER_CLASS);
	lua_regconstinteger(vm->L, STAT_XP);
	lua_regconstinteger(vm->L, STAT_PS_FLAGS);
	lua_regconstinteger(vm->L, STAT_AIRLEFT);
	lua_regconstinteger(vm->L, STAT_SPRINTTIME);
	lua_regconstinteger(vm->L, STAT_ANTIWARP_DELAY);

	// mods_t
	lua_regconstinteger(vm->L, LEGACY);
	lua_regconstinteger(vm->L, ETJUMP);
	lua_regconstinteger(vm->L, ETPRO);
	lua_regconstinteger(vm->L, UNKNOWN);
	lua_regconstinteger(vm->L, ALL);

	// tvcmdUsageFlag_t
	lua_regconstinteger(vm->L, CMD_USAGE_ANY_TIME);
	lua_regconstinteger(vm->L, CMD_USAGE_INTERMISSION_ONLY);
	lua_regconstinteger(vm->L, CMD_USAGE_NO_INTERMISSION);
	lua_regconstinteger(vm->L, CMD_USAGE_AUTOUPDATE);

	// cs, weapon and MOD constants
	TVG_RegisterConfigstringConstants(vm);
	TVG_RegisterPowerupConstants(vm);
	TVG_RegisterWeaponConstants(vm);
	TVG_RegisterModConstants(vm);
	TVG_RegisterSurfaceConstants(vm);

	// FIXME: add support of other mod's constants
	if (!(level.mod & LEGACY))
	{
		G_Printf("%s API: %sWARNING: Lua registered constants are legacy mod constants, not all may correctly apply to current mod.\n", LUA_VERSION, S_COLOR_BLUE);
	}
}

/**
 * @brief TVG_LuaStartVM starts one individual virtual machine.
 * @param[in] vm
 * @return
 */
qboolean TVG_LuaStartVM(lua_vm_t *vm)
{
	int        res;
	char       basepath[MAX_OSPATH];
	char       homepath[MAX_OSPATH];
	char       gamepath[MAX_OSPATH];
	const char *luaPath, *luaCPath;

	// Open a new lua state
	vm->L = luaL_newstate();
	if (!vm->L)
	{
		G_Printf("%s API: %sLua failed to initialise.\n", LUA_VERSION, S_COLOR_BLUE);
		return qfalse;
	}

	// Initialise the lua state
	luaL_openlibs(vm->L);

#ifdef FEATURE_LUASQL
	// register LuaSQL backend
	luaL_getsubtable(vm->L, LUA_REGISTRYINDEX, "_PRELOAD");
	lua_pushcfunction(vm->L, luaopen_luasql_sqlite3);
	lua_setfield(vm->L, -2, "luasql.sqlite3");
	lua_pop(vm->L, 1);
#endif

	// set LUA_PATH and LUA_CPATH
	trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof(basepath));
	trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof(homepath));
	trap_Cvar_VariableStringBuffer("fs_game", gamepath, sizeof(gamepath));

	luaPath = va("%s%s%s%s?.lua;%s%s%s%slualibs%s?.lua",
	             homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP,
	             homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP);

	luaCPath = va("%s%s%s%slualibs%s?.%s",
	              homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, EXTENSION);

	// add fs_basepath if different from fs_homepath
	if (Q_stricmp(basepath, homepath))
	{
		luaPath = va("%s%s%s%s?.lua;%s%s%s%slualibs%s?.lua;%s",
		             basepath, LUA_DIRSEP, gamepath, LUA_DIRSEP,
		             basepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, luaPath);

		luaCPath = va("%s%s%s%slualibs%s?.%s;%s",
		              basepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, EXTENSION,
		              luaCPath);
	}

	lua_getglobal(vm->L, LUA_LOADLIBNAME);
	if (lua_istable(vm->L, -1))
	{
		lua_pushstring(vm->L, luaPath);
		lua_setfield(vm->L, -2, "path");
		lua_pushstring(vm->L, luaCPath);
		lua_setfield(vm->L, -2, "cpath");
	}
	lua_pop(vm->L, 1);

	// register globals
	lua_registerglobal(vm->L, "LUA_PATH", luaPath);
	lua_registerglobal(vm->L, "LUA_CPATH", luaCPath);

	lua_registerglobal(vm->L, "LUA_DIRSEP", LUA_DIRSEP);

	// register functions
	luaL_newlib(vm->L, etlib);

	// register predefined constants
	TVG_RegisterConstants(vm);

	lua_pushvalue(vm->L, -1);
	lua_setglobal(vm->L, "et");

	res = luaL_loadbuffer(vm->L, vm->code, vm->code_size, vm->file_name);

	switch (res)
	{
	case LUA_OK:
		break;
	case LUA_ERRSYNTAX:
		G_Printf("%s API: %ssyntax error during pre-compilation: %s\n", LUA_VERSION, S_COLOR_BLUE, lua_tostring(vm->L, -1));
		lua_pop(vm->L, 1);
		vm->err++;
		return qfalse;
	case LUA_ERRMEM:
		G_Printf("%s API: %smemory allocation error #1 ( %s )\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);
		vm->err++;
		return qfalse;
	default:
		G_Printf("%s API: %sunknown error %i ( %s )\n", LUA_VERSION, S_COLOR_BLUE, res, vm->file_name);
		vm->err++;
		return qfalse;
	}

	// Execute the code
	if (!TVG_LuaCall(vm, "G_LuaStartVM", 0, 0))
	{
		G_Printf("%s API: %sLua VM start failed ( %s )\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);
		return qfalse;
	}

	// Load the code
	G_Printf("%s API: %sfile '%s' loaded into Lua VM\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);

	return qtrue;
}

/**
 * @brief TVG_LuaStopVM stops one virtual machine, and calls its et_Quit callback.
 * @param[in] vm
 * @return
 */
void TVG_LuaStopVM(lua_vm_t *vm)
{
	if (vm == NULL)
	{
		return;
	}
	if (vm->code != NULL)
	{
		Com_Dealloc(vm->code);
		vm->code = NULL;
	}
	if (vm->L)
	{
		if (TVG_LuaGetNamedFunction(vm, "et_Quit"))
		{
			TVG_LuaCall(vm, "et_Quit", 0, 0);
		}
		lua_close(vm->L);
		vm->L = NULL;
	}
	if (vm->id >= 0)
	{
		if (lVM[vm->id] == vm)
		{
			lVM[vm->id] = NULL;
		}
		if (!vm->err)
		{
			G_Printf("%s API: %sLua module [%s] [%s] unloaded.\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name, vm->mod_signature);
		}
	}
	Com_Dealloc(vm);
}

/**
 * @brief TVG_LuaShutdown shuts down everything related to Lua API.
 */
void TVG_LuaShutdown(void)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			TVG_LuaStopVM(vm);
		}
	}
}

/**
 * @brief TVG_LuaRestart restart Lua API
 */
void TVG_LuaRestart(void)
{
	TVG_LuaShutdown();
	TVG_LuaInit();
}

/**
 * @brief TVG_LuaStatus prints information on the Lua virtual machines.
 * @param[in] client
 */
void TVG_LuaStatus(gclient_t *client)
{
	int i, cnt = 0;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		if (lVM[i])
		{
			cnt++;
		}
	}

	if (cnt == 0)
	{
		TVG_refPrintf(client, "%s API: %sno scripts loaded.", LUA_VERSION, S_COLOR_BLUE);
		return;
	}
	else if (cnt == 1)
	{
		TVG_refPrintf(client, "%s API: %sshowing lua information ( 1 module loaded )", LUA_VERSION, S_COLOR_BLUE);
	}
	else
	{
		TVG_refPrintf(client, "%s API: %sshowing lua information ( %d modules loaded )", LUA_VERSION, S_COLOR_BLUE, cnt);
	}
	TVG_refPrintf(client, "%-2s %-24s %-40s %-24s", "VM", "Modname", "Signature", "Filename");
	TVG_refPrintf(client, "-- ------------------------ ---------------------------------------- ------------------------");
	for (i = 0; i < LUA_NUM_VM; i++)
	{
		if (lVM[i])
		{
			TVG_refPrintf(client, "%2d %-24s %-40s %-24s", lVM[i]->id, lVM[i]->mod_name, lVM[i]->mod_signature, lVM[i]->file_name);
		}
	}
	TVG_refPrintf(client, "-- ------------------------ ---------------------------------------- ------------------------");
}

/**
 * @brief TVG_LuaGetVM retrieves the VM for a given lua_State
 * @param[in] L
 * @return
 */
lua_vm_t *TVG_LuaGetVM(lua_State *L)
{
	int i;

	for (i = 0; i < LUA_NUM_VM; i++)
		if (lVM[i] && lVM[i]->L == L)
		{
			return lVM[i];
		}
	return NULL;
}

/**
 * -------------------------------------
 * Lua API hooks / callbacks
 * -------------------------------------
 * @addtogroup lua_etevents
 * @{
 */

/**
  * @brief TVG_LuaHook_InitGame
  *		  et_InitGame( levelTime, randomSeed, restart ) callback
  * @param[in] levelTime
  * @param[in] randomSeed
  * @param[in] restart
  */
void TVG_LuaHook_InitGame(int levelTime, int randomSeed, int restart)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_InitGame"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, levelTime);
			lua_pushinteger(vm->L, randomSeed);
			lua_pushinteger(vm->L, restart);
			// Call
			if (!TVG_LuaCall(vm, "et_InitGame", 3, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_ShutdownGame
 *		  et_ShutdownGame( restart )  callback
 * @param[in] restart
 */
void TVG_LuaHook_ShutdownGame(int restart)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ShutdownGame"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, restart);
			// Call
			if (!TVG_LuaCall(vm, "et_ShutdownGame", 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_RunFrame
 *		  et_RunFrame( levelTime )  callback
 * @param[in] levelTime
 */
void TVG_LuaHook_RunFrame(int levelTime)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_RunFrame"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, levelTime);
			// Call
			if (!TVG_LuaCall(vm, "et_RunFrame", 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_ClientConnect
 *		  rejectreason = et_ClientConnect( clientNum, firstTime, isBot ) callback
 * @param[in] clientNum
 * @param[in] firstTime
 * @param[in] isBot
 * @return reject reason
 */
qboolean TVG_LuaHook_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot, char *reason)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ClientConnect"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushinteger(vm->L, (int)firstTime);
			lua_pushinteger(vm->L, (int)isBot);
			// Call
			if (!TVG_LuaCall(vm, "et_ClientConnect", 3, 1))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isstring(vm->L, -1))
			{
				Q_strncpyz(reason, lua_tostring(vm->L, -1), MAX_STRING_CHARS);
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * @brief TVG_LuaHook_ClientDisconnect
 *		  et_ClientDisconnect( clientNum ) callback
 * @param[in] clientNum
 */
void TVG_LuaHook_ClientDisconnect(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ClientDisconnect"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!TVG_LuaCall(vm, "et_ClientDisconnect", 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_ClientBegin
 *		  et_ClientBegin( clientNum ) callback
 * @param[in] clientNum
 */
void TVG_LuaHook_ClientBegin(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ClientBegin"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!TVG_LuaCall(vm, "et_ClientBegin", 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_ClientUserinfoChanged
 *		  et_ClientUserinfoChanged( clientNum ) callback
 * @param[in] clientNum
 */
void TVG_LuaHook_ClientUserinfoChanged(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ClientUserinfoChanged"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!TVG_LuaCall(vm, "et_ClientUserinfoChanged", 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_ClientSpawn
 *        et_ClientSpawn( clientNum ) callback
 * @param[in] clientNum
 */
void TVG_LuaHook_ClientSpawn(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ClientSpawn"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!TVG_LuaCall(vm, "et_ClientSpawn", 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * @brief TVG_LuaHook_ClientCommand
 *        intercepted = et_ClientCommand( clientNum, command ) callback
 * @param[in] clientNum
 * @param[in] command
 * @return intercepted
 */
qboolean TVG_LuaHook_ClientCommand(int clientNum, char *command)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ClientCommand"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushstring(vm->L, command);
			// Call
			if (!TVG_LuaCall(vm, "et_ClientCommand", 2, 1))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isnumber(vm->L, -1))
			{
				if (lua_tointeger(vm->L, -1) == 1)
				{
					lua_pop(vm->L, 1);
					return qtrue;
				}
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * @brief TVG_LuaHook_ConsoleCommand
 *        intercepted = et_ConsoleCommand( command ) callback
 * @param[in] command
 * @return intercepted
 */
qboolean TVG_LuaHook_ConsoleCommand(char *command)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, "et_ConsoleCommand"))
			{
				continue;
			}
			// Arguments
			lua_pushstring(vm->L, command);
			// Call
			if (!TVG_LuaCall(vm, "et_ConsoleCommand", 1, 1))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isnumber(vm->L, -1))
			{
				if (lua_tointeger(vm->L, -1) == 1)
				{
					lua_pop(vm->L, 1);
					return qtrue;
				}
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

static luaPrintFunctions_t tvg_luaPrintFunctions[] =
{
	{ GPRINT_TEXT,      "et_Print"  },
	{ GPRINT_DEVELOPER, "et_DPrint" },
	{ GPRINT_ERROR,     "et_Error"  }
};


/**
 * @brief TVG_LuaHook_Print
 *        et_Print( text ) callback
 * @param[in] category
 * @param[in] text
 */
void TVG_LuaHook_Print(printMessageType_t category, char *text)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!TVG_LuaGetNamedFunction(vm, tvg_luaPrintFunctions[category].function))
			{
				continue;
			}
			// Arguments
			lua_pushstring(vm->L, text);
			// Call
			if (!TVG_LuaCall(vm, tvg_luaPrintFunctions[category].function, 1, 0))
			{
				//TVG_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** @} */ // doxygen addtogroup lua_etevents

#endif // FEATURE_LUA
