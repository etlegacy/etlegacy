/**
 * @file g_lua.c
 * @brief ET <-> *Lua* interface source file.
 *
 * @copyright This code is derived from ETPub and NQ, and inspired from ETPro
 * All credits go to their teams especially to quad and pheno!
 *
 * [League Lua API]: https://etlegacy-lua-docs.readthedocs.io
 */
#ifdef FEATURE_LUA

#include "g_lua.h"

#ifdef FEATURE_LUASQL
#include "../luasql/luasql.h"
#include "../luasql/luasql.c"
#include "../luasql/ls_sqlite3.c"
#endif

#ifdef FEATURE_LUAJIT
#include "../luajit/compat.h"
#endif

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

extern field_t fields[];

lua_vm_t *lVM[LUA_NUM_VM];

/**
 * @param addr pointer to a gentity (gentity*)
 * @returns the entity number.
 *          if (input==0) return = -1
 *          if (input address is out of g_entities[] memory range) return -1;
 */
static int C_gentity_ptr_to_entNum(uintptr_t addr)
{
	// no NULL address,
	// address must also be in the range of the g_entities array memory space…
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
 * Registers a descriptive name for this mod.
 *
 * @lua_def_prototype et.RegisterModname(modname)
 * @lua_def ---@param modname string the name to register the Lua module.
 */
static int _et_RegisterModname(lua_State *L)
{
	const char *modname = luaL_checkstring(L, 1);

	if (modname)
	{
		lua_vm_t *vm = G_LuaGetVM(L);
		if (vm)
		{
			Q_strncpyz(vm->mod_name, modname, sizeof(vm->mod_name));
		}
	}
	return 0;
}

/**
 * Returns the assigned Lua VM slot number.
 *
 * @lua_def_prototype et.FindSelf()
 * @lua_def ---@return number vmnumber the returned slot number assigned to this Lua VM.
 */
static int _et_FindSelf(lua_State *L)
{
	lua_vm_t *vm = G_LuaGetVM(L);

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
 * Returns the name and SHA1 signature for the mod loaded in a VM slot.
 *
 * @lua_def_prototype et.FindMod(vmnumber)
 * @lua_def ---@param vmnumber number the VM slot number of the Lua module.
 * @lua_def ---@return string? modname the returned registered module's name, nil if the VM slot is invalid.
 * @lua_def ---@return string? signature the returned registered module's SHA-1 signature hash, nil if the VM slot is invalid.
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
 * Sends a message string to the mod in the another VM slot.
 *
 * IMPORTANT: The mod receiving message must have an `et.IPCReceive` callback.
 *
 * NOTE: Data cannot be received and sent back in the same server frame.
 *
 * @lua_def_prototype et.IPCSend(vmnumber, message)
 * @lua_def ---@param vmnumber number the VM slot number of the Lua module to send a message to.
 * @lua_def ---@param message string the message to sent to the Lua module.
 * @lua_def ---@return number success '1' if the message is sent successfully, '0' if it fails.
 */
static int _et_IPCSend(lua_State *L)
{
	int        vmnumber = (int)luaL_checkinteger(L, 1);
	const char *message = luaL_checkstring(L, 2);

	lua_vm_t *sender = G_LuaGetVM(L);
	lua_vm_t *vm     = lVM[vmnumber];

	if (!vm || vm->err)
	{
		lua_pushinteger(L, 0);
		return 1;
	}

	// Find callback
	if (!G_LuaGetNamedFunction(vm, "et_IPCReceive"))
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
	if (!G_LuaCall(vm, "et.IPCSend", 2, 0))
	{
		//G_LuaStopVM(vm);
		lua_pushinteger(L, 0);
		return 1;
	}

	// Success
	lua_pushinteger(L, 1);
	return 1;
}

// Printing

/**
 * Prints text to the server console.
 *
 * @lua_def_prototype et.G_Print(text)
 * @lua_def ---@param text string the printed string.
 */
static int _et_G_Print(lua_State *L)
{
	char text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));
	trap_Printf(text);
	return 0;
}

/**
 * Prints text to the server console and writes it to the server log.
 *
 * @lua_def_prototype et.G_LogPrint(text)
 * @lua_def ---@param text string the printed and logged string.
 */
static int _et_G_LogPrint(lua_State *L)
{
	char text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));
	G_LogPrintf("%s", text);
	return 0;
}

// Argument Handling

/**
 * Returns all arguments beginning concatenated into a single string.
 *
 * @lua_def_prototype et.ConcatArgs(index)
 * @lua_def ---@param index number the index of the first argument in the concatenated string.
 * @lua_def ---@return string args the returned concatenated string.
 */
static int _et_ConcatArgs(lua_State *L)
{
	int index = (int)luaL_checkinteger(L, 1);

	lua_pushstring(L, ConcatArgs(index));
	return 1;
}

/**
 * Returns the number of command line arguments in the server command.
 *
 * @lua_def_prototype et.trap_Argc()
 * @lua_def ---@return number argcount the returned count of arguments.
 */
static int _et_trap_Argc(lua_State *L)
{
	lua_pushinteger(L, trap_Argc());
	return 1;
}

/**
 * Returns the contents of the command line argument.
 *
 * @lua_def_prototype et.trap_Argv(index)
 * @lua_def ---@param index number the index of the argument to return.
 * @lua_def ---@return string arg the returned argument.
 */
static int _et_trap_Argv(lua_State *L)
{
	char buff[MAX_STRING_CHARS];
	int  argnum = (int)luaL_checkinteger(L, 1);

	trap_Argv(argnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

/**
 * Returns the value of the given cvar.
 *
 * @lua_def_prototype et.trap_Cvar_Get(name)
 * @lua_def ---@param name string the name of the cvar.
 * @lua_def ---@return string cvarvalue the returned string containing the value. If there is no cvar with the given name, the returning string has zero length.
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
 * Sets value to a cvar.
 *
 * @lua_def_prototype et.trap_Cvar_Set(name, cvarvalue)
 * @lua_def ---@param name string the name of the cvar to set.
 * @lua_def ---@param cvarvalue string the new value for the cvar.
 */
static int _et_trap_Cvar_Set(lua_State *L)
{
	const char *cvarname  = luaL_checkstring(L, 1);
	const char *cvarvalue = luaL_checkstring(L, 2);

	trap_Cvar_Set(cvarname, cvarvalue);
	return 0;
}

/**
 * Returns content of the configstring index.
 *
 * @lua_def_prototype et.trap_GetConfigstring(index)
 * @lua_def ---@param index number the index of the configstring. See `et.CS_` for possible values.
 * @lua_def ---@return string configstring the returned string containing the full configstring.
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
 * Sets the full configstring.
 *
 * @lua_def_prototype et.trap_SetConfigstring(index, value)
 * @lua_def ---@param index number the configstring index. See `et.CS_*` for possible values.
 * @lua_def ---@return string value the full configstring to set.
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
 * Sends command to the server console.
 *
 * @lua_def_prototype et.trap_SendConsoleCommand(when, command)
 * @lua_def ---@param when number tells when the command is executed. See `et.EXEC_*` for possible values - by default you probably want 'et.EXEC_APPEND'.
 * @lua_def ---@param command string the full command to execute.
 */
static int _et_trap_SendConsoleCommand(lua_State *L)
{
	int        when = (int)luaL_checkinteger(L, 1);
	const char *cmd = luaL_checkstring(L, 2);

	trap_SendConsoleCommand(when, cmd);
	return 0;
}

// Clients


/**
 * Sends the command command to the client clientnum. If clientnum is `-1`, the command is broadcast to all clients.
 *
 * TIP: See `SendServerCommand()` for a detailed example usage of possible commands.
 *
 * @lua_def_prototype et.trap_SendServerCommand(clientnum, command)
 * @lua_def ---@param clientnum number if clientnum is `-1`, the command is broadcast to all clients.
 * @lua_def ---@param command string the full command to send.
 */
static int _et_trap_SendServerCommand(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	const char *cmd      = luaL_checkstring(L, 2);

	trap_SendServerCommand(clientnum, cmd);
	return 0;
}

/**
 * Disconnects client from the server.
 *
 * @lua_def_prototype et.trap_DropClient(clientnum, reason, bantime)
 * @lua_def ---@param clientnum number the slot number of the client.
 * @lua_def ---@param reason string the descriptive reason for the kick which is reported to the client.
 * @lua_def ---@param bantime number the length of the ban in seconds.
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
 * Searches for one partial match with passed 'string'.
 *
 * If one is found the 'clientNum' is returned, if there is none or more than
 * one match 'nil' is returned.
 *
 * @lua_def_prototype et.ClientNumberFromString(string)
 * @lua_def ---@param string string a pattern to match against client names.
 * @lua_def ---@return number clientnum the returned client slot number if one match is found, otherwise **nil** is returned (none or more than one match).
 */
static int _et_ClientNumberFromString(lua_State *L)
{
	const char *search = luaL_checkstring(L, 1);
	int        pids[MAX_CLIENTS];

	// only send exact matches, otherwise -1
	if (G_ClientNumbersFromString(search, pids) == 1)
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
 * Sends a chat command on behalf of client.
 *
 * @lua_def_prototype et.G_Say(clientNum, mode, text)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@param mode number the broadcast mode. See `et.SAY_* constants.
 * @lua_def ---@param text string the chat text.
 */
static int _et_G_Say(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	int        mode      = (int)luaL_checkinteger(L, 2);
	const char *text     = luaL_checkstring(L, 3);

	G_Say(g_entities + clientnum, NULL, mode, text);
	return 0;
}

// et.MutePlayer( clientnum, duration, reason )
// duration is in seconds.

/**
 * Mutes the specified player.
 *
 * @lua_def_prototype et.MutePlayer(clientnum, duration, reason)
 * @lua_def ---@param clientnum number the slot number of the client to mute.
 * @lua_def ---@param duration number the optional duration of the mute in seconds.
 * @lua_def ---@param reason string? the optional reason of the mute.
 */
static int _et_MutePlayer(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	gentity_t  *ent      = g_entities + clientnum;
	int        duration  = (int)luaL_checkinteger(L, 2);
	const char *reason   = luaL_optstring(L, 3, NULL);

	if (!ent->client)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	ent->client->sess.muted = qtrue;

	//ClientConfigStringChanged( ent );

	if (duration == -1)
	{
		if (reason == NULL)
		{
			CPx(clientnum, va("print \"You've been muted by Lua.\n\""));
			AP(va("chat \"%s^7 has been muted by Lua.\"", ent->client->pers.netname));
		}
		else
		{
			CPx(clientnum, va("print \"You've been muted by Lua. %s\n\"", reason));
			AP(va("chat \"%s^7 has been muted by Lua. %s\"", ent->client->pers.netname, reason));
		}
	}
	else
	{
		if (reason == NULL)
		{
			CPx(clientnum, va("print \"You've been muted for ^3%d^7 seconds by Lua.\n\"", duration));
			AP(va("chat \"%s^7 has been muted for ^3%d^7 seconds by Lua.\"", ent->client->pers.netname, duration));
		}
		else
		{
			CPx(clientnum, va("print \"You've been muted for ^3%d^7 seconds by Lua. %s\n\"", duration, reason));
			AP(va("chat \"%s^7 has been muted for ^3%d^7 seconds by Lua. %s\"", ent->client->pers.netname, duration, reason));
		}
	}
	return 0;
}

/**
 * Unmutes the specified player.
 *
 * @lua_def_prototype et.UnmutePlayer(clientnum)
 * @lua_def ---@param clientnum number the slot number of the client to unmute.
 */
static int _et_UnmutePlayer(lua_State *L)
{
	int       clientnum = (int)luaL_checkinteger(L, 1);
	gentity_t *ent      = g_entities + clientnum;

	if (!ent->client)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	ent->client->sess.muted = qfalse;

	// ClientConfigStringChanged( ent );
	CPx(clientnum, "print \"^5You've been auto-unmuted. Lua penalty lifted.\n\"");
	AP(va("chat \"%s^7 has been auto-unmuted. Lua penalty lifted.\"", ent->client->pers.netname));
	return 0;
}

// Userinfo

/**
 * Returns the userinfo string of a client.
 *
 * @lua_def_prototype et.trap_GetUserinfo(clientNum)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@return string userinfo the returned string of the specified client - empty string if no player connected.
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
 * Sets the userinfo string of the client to the specified userinfo.
 *
 * NOTE: The `et.ClientUserinfoChanged()` function must be called after this function for the changes to take effect.
 *
 * @lua_def_prototype et.trap_SetUserinfo(clientnum, userinfo)
 * @lua_def ---@param clientnum number the slot number of the client.
 * @lua_def ---@param userinfo string the userinfo string that replaces the current userinfo.
 */
static int _et_trap_SetUserinfo(lua_State *L)
{
	int        clientnum = (int)luaL_checkinteger(L, 1);
	const char *userinfo = luaL_checkstring(L, 2);

	trap_SetUserinfo(clientnum, userinfo);
	return 0;
}

/**
 * Loads the new userinfo string of the client and sets the client settings to
 * match it.
 *
 * @lua_def_prototype et.ClientUserinfoChanged(clientnum)
 * @lua_def ---@param clientnum number the slot number of the client.
 */
static int _et_ClientUserinfoChanged(lua_State *L)
{
	int clientnum = (int)luaL_checkinteger(L, 1);

	ClientUserinfoChanged(clientnum);
	return 0;
}

// String Utility Functions

/**
 * Removes a key and its associated value from an infostring.
 *
 * @lua_def_prototype et.Info_RemoveKey(infostring, key)
 * @lua_def ---@param infostring string the infostring from which to remove the key.
 * @lua_def ---@param key string the key to remove.
 * @lua_def ---@return string newinfostring the returned modified infostring without the key.
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
 * Sets a value in an infostring.
 *
 * @lua_def_prototype et.Info_SetValueForKey(infostring, key, value)
 * @lua_def ---@param infostring string the original infostring.
 * @lua_def ---@param key string the key to set.
 * @lua_def ---@param value string the value to set to the key. If empty, the key is removed from the infostring.
 * @lua_def ---@return string newinfostring the returned modified infostring.
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
 * Returns a value from an infostring.
 *
 * @lua_def_prototype et.Info_ValueForKey(infostring, key)
 * @lua_def ---@param infostring string the infostring from where to search the key.
 * @lua_def ---@param key string the key which value is returned.
 * @lua_def ---@return string keyvalue the returned value from the searched key. If key is not present in the infostring, an empty string is returned.
 */
static int _et_Info_ValueForKey(lua_State *L)
{
	const char *infostring = luaL_checkstring(L, 1);
	const char *key        = luaL_checkstring(L, 2);

	lua_pushstring(L, Info_ValueForKey(infostring, key));
	return 1;
}

/**
 * Returns string stripped of all color codes and special characters.
 *
 * @lua_def_prototype et.Q_CleanStr(string)
 * @lua_def ---@param string string the string to clean.
 * @lua_def ---@return string cleanstring the returned cleaned string.
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
 * Opens a file in the local file system.
 *
 * @lua_def_prototype et.trap_FS_FOpenFile(filename, mode)
 * @lua_def ---@param filename string the name of the file to open. The file is opened under the current working directory and absolute paths will not work.
 * @lua_def ---@param mode number the access mode the file is opened. See `et.FS_*` for possible values.
 * @lua_def ---@return number fd the returned descriptor of the file.
 * @lua_def ---@return number len the returned length of the file, or `-1` if
 * an error occured.
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
 * Reads from an open file.
 *
 * @lua_def_prototype et.trap_FS_Read(fd, count)
 * @lua_def ---@param fd number the descriptor of the file to read.
 * @lua_def ---@param count number the amount of bytes to read.
 * @lua_def ---@return string filedata the returned value that have the read bytes.
 */
static int _et_trap_FS_Read(lua_State *L)
{
	char         *filedata = "";
	fileHandle_t fd        = (int)luaL_checkinteger(L, 1);
	int          count     = (int)luaL_checkinteger(L, 2);

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
 * Writes at the end of an open file.
 *
 * @lua_def_prototype et.trap_FS_Write(filedata, count, fd)
 * @lua_def ---@param filedata string a block of bytes to write.
 * @lua_def ---@param count number the size of the block to write.
 * @lua_def ---@param fd number the descriptor of the file.
 * @lua_def ---@return number bytecount the returned amount of bytes written to the file.
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
 * Closes an opened file.
 *
 * @lua_def_prototype et.trap_FS_FCloseFile(fd)
 * @lua_def ---@param fd number the descriptor of the opened file.
 */
static int _et_trap_FS_FCloseFile(lua_State *L)
{
	fileHandle_t fd = (int)luaL_checkinteger(L, 1);
	trap_FS_FCloseFile(fd);
	return 0;
}

/**
 * Renames a file in the local file system.
 *
 * @lua_def_prototype et.trap_FS_Rename(oldname, newname)
 * @lua_def ---@param oldname string the name of the file to rename.
 * @lua_def ---@param newname string the name the old file name is changed to.
 */
static int _et_trap_FS_Rename(lua_State *L)
{
	const char *oldname = luaL_checkstring(L, 1);
	const char *newname = luaL_checkstring(L, 2);

	trap_FS_Rename(oldname, newname);
	return 0;
}

// filelist = et.trap_FS_GetFileList( dirname, fileextension )
extern char bigTextBuffer[100000];

/**
 * Retrieves list of files from a directory.
 *
 * @lua_def_prototype et.trap_FS_GetFileList(dirname, fileextension)
 * @lua_def ---@param dirname string the name of the directory.
 * @lua_def ---@param fileextension string the file extension of file names to retrieve.
 * @lua_def ---@return string[] filelist the returned array of file names strings.
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

// Indexes

/**
 * Returns the index to the searched soundfile.
 *
 * @lua_def_prototype et.G_SoundIndex(filename)
 * @lua_def ---@param filename string the sound file name that is searched.
 * @lua_def ---@return number soundindex the returned string index that includes the filename or 0 if not found.
 */
static int _et_G_SoundIndex(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	lua_pushinteger(L, G_SoundIndex(filename));
	return 1;
}

/**
 * Returns the index to the searched model.
 *
 * @lua_def_prototype et.G_ModelIndex(filename)
 * @lua_def ---@param filename string the name that is searched.
 * @lua_def ---@return number modelindex the returned string index that included the filename or 0 if not found.
 */
static int _et_G_ModelIndex(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	lua_pushinteger(L, G_ModelIndex((char *)filename));
	return 1;
}

// Sound

/**
 * Plays a sound to all connected clients.
 *
 * @lua_def_prototype et.G_globalSound(sound)
 * @lua_def ---@param sound string the name of the sound to play.
 */
static int _et_G_globalSound(lua_State *L)
{
	const char *sound = luaL_checkstring(L, 1);

	G_globalSound((char *)sound);
	return 0;
}

/**
 * Plays a sound originating from position of an entity.
 *
 * @lua_def_prototype et.G_Sound(entnum, soundindex)
 * @lua_def ---@param entnum number the number of the entity which position is used as the sound origin.
 * @lua_def ---@param soundindex number the index of the sound that is played.
 */
static int _et_G_Sound(lua_State *L)
{
	int entnum     = (int)luaL_checkinteger(L, 1);
	int soundindex = (int)luaL_checkinteger(L, 2);

	G_Sound(g_entities + entnum, soundindex);
	return 0;
}

/**
 * Plays a sound originating from a client entity to the team members of that
 * client.
 *
 * @lua_def_prototype et.G_ClientSound(clientnum, soundindex)
 * @lua_def ---@param clientnum number the slot number of the connected player.
 * @lua_def ---@param soundindex number the index to the sound to play.
 */
static int _et_G_ClientSound(lua_State *L)
{
	int clientnum  = (int)luaL_checkinteger(L, 1);
	int soundindex = (int)luaL_checkinteger(L, 2);

	G_ClientSound(g_entities + clientnum, soundindex);
	return 0;
}

// Miscellaneous {{{

/**
 * Returns level time.
 *
 * @lua_def_prototype et.trap_Milliseconds()
 * @lua_def ---@return number milliseconds the returned time in milliseconds.
 */
static int _et_trap_Milliseconds(lua_State *L)
{
	lua_pushinteger(L, trap_Milliseconds());
	return 1;
}

/**
 * Checks bit status of a bitmask value.
 *
 * @lua_def_prototype et.isBitSet(bit, value)
 * @lua_def ---@param bit number the checked bit.
 * @lua_def ---@param value number the bitmask value.
 * @lua_def ---@return number success returns '1' if the bit is set in the bitmask value, and '0' if it is not.
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
 * Damages target entity on behalf of the attacker entity.
 *
 * @lua_def_prototype et.G_Damage(target, inflictor, attacker, damage, dflags, mod)
 * @lua_def ---@param target number The entity number to damage.
 * @lua_def ---@param inflictor number The entity number that does the damage.
 * @lua_def ---@param attacker number The entity number that causes the *inflictor* entity to cause damage to *target*.
 * @lua_def ---@param damage number The amount of damage to inflict.
 * @lua_def ---@param dflags number The type of damage to inflict. See `et.DAMAGE_*` for possible values.
 * @lua_def ---@param mod number The means of death. See `et.MOD_*` constants for possible values.
 */
static int _et_G_Damage(lua_State *L)
{
	int            target    = (int)luaL_checkinteger(L, 1);
	int            inflictor = (int)luaL_checkinteger(L, 2);
	int            attacker  = (int)luaL_checkinteger(L, 3);
	int            damage    = (int)luaL_checkinteger(L, 4);
	int            dflags    = (int)luaL_checkinteger(L, 5);
	meansOfDeath_t mod       = (meansOfDeath_t)(luaL_checkinteger(L, 6));

	G_Damage(g_entities + target,
	         g_entities + inflictor,
	         g_entities + attacker,
	         NULL,
	         NULL,
	         damage,
	         dflags,
	         mod);

	return 0;
}

/**
 * Adds points to the client's skill.
 *
 * @lua_def_prototype et.G_AddSkillPoints(clientNum, skill, points, reason)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@param skill number identifies the skill that the points are added to. See `et.SK_` for possible values.
 * @lua_def ---@param points number the amount of points to add.
 * @lua_def ---@param reason string a message advertised.
 */
static int _et_G_AddSkillPoints(lua_State *L)
{
	size_t     l;
	gentity_t  *ent    = g_entities + (int)luaL_checkinteger(L, 1);
	int        skill   = (int)luaL_checkinteger(L, 2);
	float      points  = luaL_checknumber(L, 3);
	const char *reason = luaL_checklstring(L, 4, &l);

	G_AddSkillPoints(ent, skill, points, reason);
	return 0;
}

/**
 * Removes points to the client's skill.
 *
 * @lua_def_prototype et.G_LoseSkillPoints(clientNum, skill, points)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@param skill number specific skill to lose points for, see `et.SK_*`.
 * @lua_def ---@param points number the amount of points to remove.
 */
static int _et_G_LoseSkillPoints(lua_State *L)
{
	size_t     l;
	gentity_t  *ent    = g_entities + (int)luaL_checkinteger(L, 1);
	int        skill   = (int)luaL_checkinteger(L, 2);
	float      points  = luaL_checknumber(L, 3);
	const char *reason = luaL_checklstring(L, 4, &l);

	G_LoseSkillPoints(ent, skill, points, reason);
	return 0;
}

/**
 * Sets XP of the client.
 *
 * @lua_def_prototype et.G_XP_Set(clientNum , xp, skill, add)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@param xp number the number of XP points.
 * @lua_def ---@param skill number specific skill to grant xp to, see `et.SK_*`.
 * @lua_def ---@param add number if set to '1', the skill xp will be added to the current skill xp of the player - otherwise it will be (re-)set to 'xp'
 */
static int _et_G_XP_Set(lua_State *L)
{
	gentity_t *ent      = NULL;
	int       clientNum = (int)luaL_checkinteger(L, 1);
	float     xp        = (float)luaL_checknumber(L, 2);
	int       skill     = (int)luaL_checkinteger(L, 3);
	int       add       = (int)luaL_checkinteger(L, 4); // 'add' just checks to be 0 or not to be 0

	ent = &g_entities[clientNum];

	// Did comment the following lines to set XP via Lua on client connect()
	// - If used on connect() a moment later the rest of the entity data is set, and the entity data is valid
	// - If a client is not 'inuse' and this function is called the client is not in game for real
	//   and the data should be overwritten again, when the next player uses this client num/slot

	// Check if the entity is valid
	//if ( !ent->inuse ) {
	//	luaL_error(L, "clientNum \"%d\" is not an used entity", clientNum);
	//	return 0;
	//}

	// Check if the entity is a client
	if (!ent->client)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientNum);
		return 0;
	}

	// Check if the skill is in the range
	if (skill < 0 || skill > SK_NUM_SKILLS - 1)
	{
		luaL_error(L, "\"skill\" must be a number from 0 to 6 both included");
		return 0;
	}

	// Check if the xp value is negative
	if (xp < 0)
	{
		luaL_error(L, "negative xp values are not allowed");
		return 0;
	}

	// special case for 0 adds
	if (add == 0)
	{
		float oldxp = ent->client->sess.skillpoints[skill];

		ent->client->sess.skillpoints[skill] = xp;
		//ent->client->sess.mapstartSkillpoints[skill] = xp;
		ent->client->sess.startxptotal -= oldxp;
		ent->client->sess.startxptotal += xp;
	}
	else
	{
		ent->client->sess.skillpoints[skill] += xp;
		//ent->client->sess.mapstartSkillpoints[skill] += xp;
		ent->client->sess.startxptotal += xp;
	}

	ent->client->ps.stats[STAT_XP] = (int)ent->client->sess.startxptotal;

	G_CalcRank(ent->client);
	BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, level.time, qfalse);

	return 1;
}

/**
 * Resets XP of the client.
 *
 * @lua_def_prototype et.G_ResetXP(clientNum)
 * @lua_def ---@param clientNum number the slot number of the client.
 */
static int _et_G_ResetXP(lua_State *L)
{
	int       entnum = luaL_optinteger(L, 1, -1);
	gentity_t *ent;

	if (entnum > -1 && entnum < MAX_CLIENTS)
	{
		ent = g_entities + entnum;

		if (!ent->client)
		{
			luaL_error(L, "clientNum \"%d\" is not a client entity", entnum);
			return 0;
		}

		G_ResetXP(ent);
	}
	else
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity number", entnum);
	}
	return 0;
}

/**
 * Adds a weapon to a client.
 *
 * NOTE: Adding a weapon does not automatically add its associated alternate
 * weapon.
 *
 * @lua_def_prototype et.AddWeaponToPlayer( clientNum, weapon, ammo, ammoclip, setcurrent )
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@param weapon number the weapon to add. See `et.WP_* constants <constants.html#wp-constants>`__ for possible values.
 * @lua_def ---@param ammo number the number of ammo to add.
 * @lua_def ---@param ammoclip number the number of ammo clip to add.
 */
static int _et_AddWeaponToPlayer(lua_State *L)
{
	int       clientnum  = (int)luaL_checkinteger(L, 1);
	gentity_t *ent       = g_entities + clientnum;
	weapon_t  weapon     = (int)luaL_checkinteger(L, 2);
	int       ammo       = (int)luaL_checkinteger(L, 3);
	int       ammoclip   = (int)luaL_checkinteger(L, 4);
	int       setcurrent = (int)luaL_checkinteger(L, 5);

	if (!ent->client)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	if (!IS_VALID_WEAPON(weapon))
	{
		luaL_error(L, "weapon \"%d\" is not a valid weapon", weapon);
		return 0;
	}

	COM_BitSet(ent->client->ps.weapons, weapon);
	ent->client->ps.ammoclip[GetWeaponTableData(weapon)->clipIndex] = ammoclip;
	ent->client->ps.ammo[GetWeaponTableData(weapon)->ammoIndex]     = ammo;

	if (setcurrent == 1)
	{
		ent->client->ps.weapon = weapon;
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_AddWeapon(ent->client->ps.clientNum, Bot_WeaponGameToBot(weapon));
#endif

	return 1;
}

/**
 * Removes a weapon from a client.
 *
 * NOTE: Removing a weapon also removes its associated alternate weapon.
 *
 * @lua_def_prototype et.RemoveWeaponFromPlayer(clientNum, weapon)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@param weapon number the weapon to add. See `et.WP_* constants for possible values.
 */
static int _et_RemoveWeaponFromPlayer(lua_State *L)
{
	int       clientnum = (int)luaL_checkinteger(L, 1);
	gentity_t *ent      = g_entities + clientnum;
	weapon_t  weapon    = (int)luaL_checkinteger(L, 2);

	if (!ent->client)
	{
		luaL_error(L, "clientNum \"%d\" is not a client entity", clientnum);
		return 0;
	}

	if (!IS_VALID_WEAPON(weapon))
	{
		luaL_error(L, "weapon \"%d\" is not a valid weapon", weapon);
		return 0;
	}

	COM_BitClear(ent->client->ps.weapons, weapon);

	if (GetWeaponTableData(weapon)->weapAlts)
	{
		weapon_t weapAlts = GetWeaponTableData(weapon)->weapAlts;

		if (GetWeaponTableData(weapAlts)->type & (WEAPON_TYPE_RIFLENADE | WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET))
		{
			COM_BitClear(ent->client->ps.weapons, weapAlts);
		}
	}

	if (weapon == ent->client->ps.weapon)
	{
		// Clear out empty weapon, change to next best weapon
		ent->client->ps.weapon = 0;
		G_AddEvent(ent, EV_WEAPONSWITCHED, 0);
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_RemoveWeapon(ent->client->ps.clientNum, Bot_WeaponGameToBot(weapon));
#endif

	return 1;
}

/**
 * Return weapon, ammo, ammoclip from a client.
 *
 * @lua_def_prototype et.GetCurrentWeapon(clientNum)
 * @lua_def ---@param clientNum number the slot number of the client.
 * @lua_def ---@return number weapon weapon number, see `et.WP_`.
 * @lua_def ---@return number ammo current ammo.
 * @lua_def ---@return number ammoclip current clip ammo.
 */
static int _et_GetCurrentWeapon(lua_State *L)
{
	gentity_t *ent;
	gclient_t *client;
	int       clientNum, ammo, ammoclip;

	clientNum = luaL_checkinteger(L, 1);
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		luaL_error(L, "\"clientNum\" is out of bounds: %d", clientNum);
		return 0;
	}

	ent = g_entities + clientNum;
	if (!ent->client)
	{
		luaL_error(L, "\"clientNum\" \"%d\" is not a client entity", clientNum);
		return 0;
	}

	client   = ent->client;
	ammo     = client->ps.ammo[GetWeaponTableData(client->ps.weapon)->ammoIndex];
	ammoclip = client->ps.ammoclip[GetWeaponTableData(client->ps.weapon)->clipIndex];

	lua_pushinteger(L, client->ps.weapon);
	lua_pushinteger(L, ammo);
	lua_pushinteger(L, ammoclip);

	return 3;
}

// Entities
// client entity fields
static const gentity_field_t gclient_fields[] =
{
	_et_gclient_addfield(noclip,                            FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(lastKillTime,                      FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(saved_persistant,                  FIELD_INT_ARRAY,           FIELD_FLAG_READONLY),
	_et_gclient_addfield(lastConstructibleBlockingWarnTime, FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(landmineSpottedTime,               FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(lasthurt_client,                   FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(lasthurt_mod,                      FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(lasthurt_time,                     FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(respawnTime,                       FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(inactivityTime,                    FIELD_INT,                 0),
	_et_gclient_addfield(inactivityWarning,                 FIELD_INT,                 0),
	_et_gclient_addfield(combatState,                       FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(deathAnimTime,                     FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(deathTime,                         FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(disguiseClientNum,                 FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(medals,                            FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(acc,                               FIELD_FLOAT,               FIELD_FLAG_READONLY),
	_et_gclient_addfield(hspct,                             FIELD_FLOAT,               FIELD_FLAG_READONLY),
	_et_gclient_addfield(freezed,                           FIELD_INT,                 0),
	_et_gclient_addfield(constructSoundTime,                FIELD_INT,                 FIELD_FLAG_READONLY),

	// to be compatible with ETPro:
	_et_gclient_addfieldalias(client.inactivityTime,        inactivityTime,            FIELD_INT,           0),
	_et_gclient_addfieldalias(client.inactivityWarning,     inactivityWarning,         FIELD_INT,           0),

	_et_gclient_addfield(pers.connected,                    FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.netname,                      FIELD_STRING,              FIELD_FLAG_NOPTR),
	_et_gclient_addfield(pers.localClient,                  FIELD_INT,                 0),
	_et_gclient_addfield(pers.initialSpawn,                 FIELD_INT,                 0),
	_et_gclient_addfield(pers.enterTime,                    FIELD_INT,                 0),
	_et_gclient_addfield(pers.connectTime,                  FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.teamState.state,              FIELD_INT,                 0),
	_et_gclient_addfield(pers.voteCount,                    FIELD_INT,                 0),
	_et_gclient_addfield(pers.complaints,                   FIELD_INT,                 0),
	_et_gclient_addfield(pers.complaintClient,              FIELD_INT,                 0),
	_et_gclient_addfield(pers.complaintEndTime,             FIELD_INT,                 0),
	_et_gclient_addfield(pers.lastReinforceTime,            FIELD_INT,                 0),
	_et_gclient_addfield(pers.applicationClient,            FIELD_INT,                 0),
	_et_gclient_addfield(pers.applicationEndTime,           FIELD_INT,                 0),
	_et_gclient_addfield(pers.invitationClient,             FIELD_INT,                 0),
	_et_gclient_addfield(pers.invitationEndTime,            FIELD_INT,                 0),
	_et_gclient_addfield(pers.propositionClient,            FIELD_INT,                 0),
	_et_gclient_addfield(pers.propositionClient2,           FIELD_INT,                 0),
	_et_gclient_addfield(pers.propositionEndTime,           FIELD_INT,                 0),
	_et_gclient_addfield(pers.autofireteamEndTime,          FIELD_INT,                 0),
	_et_gclient_addfield(pers.autofireteamCreateEndTime,    FIELD_INT,                 0),
	_et_gclient_addfield(pers.autofireteamJoinEndTime,      FIELD_INT,                 0),
	_et_gclient_addfield(pers.lastSpawnTime,                FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.ready,                        FIELD_INT,                 0),
	_et_gclient_addfield(pers.lastkilled_client,            FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lastrevive_client,            FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lastkiller_client,            FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lastammo_client,              FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lasthealth_client,            FIELD_INT,                 0),
	_et_gclient_addfield(pers.lastteambleed_client,         FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lastteambleed_dmg,            FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.playerStats.hitRegions,       FIELD_INT_ARRAY,           FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lastBattleSenseBonusTime,     FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.lastHQMineReportTime,         FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.maxHealth,                    FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(pers.playerStats.selfkills,        FIELD_INT,                 FIELD_FLAG_READONLY),

	_et_gclient_addfield(ps.pm_flags,                       FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.pm_time,                        FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.pm_type,                        FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.eFlags,                         FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.weapon,                         FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.weaponstate,                    FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.stats,                          FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(ps.persistant,                     FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(ps.ping,                           FIELD_INT,                 FIELD_FLAG_READONLY),// no ping change for lua scripts
	_et_gclient_addfield(ps.powerups,                       FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(ps.origin,                         FIELD_VEC3,                0),
	_et_gclient_addfield(ps.viewangles,                     FIELD_VEC3,                0),
	_et_gclient_addfield(ps.velocity,                       FIELD_VEC3,                0),
	_et_gclient_addfield(ps.ammo,                           FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(ps.ammoclip,                       FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(ps.classWeaponTime,                FIELD_INT,                 0),
	_et_gclient_addfield(ps.viewheight,                     FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(ps.leanf,                          FIELD_FLOAT,               FIELD_FLAG_READONLY),


	// same order as in g_local.h
	_et_gclient_addfield(sess.sessionTeam,                  FIELD_INT,                 0),
	_et_gclient_addfield(sess.spectatorTime,                FIELD_INT,                 0),
	_et_gclient_addfield(sess.spectatorState,               FIELD_INT,                 0),
	_et_gclient_addfield(sess.spectatorClient,              FIELD_INT,                 0),
	_et_gclient_addfield(sess.playerType,                   FIELD_INT,                 0),
	_et_gclient_addfield(sess.playerWeapon,                 FIELD_INT,                 0),
	_et_gclient_addfield(sess.playerWeapon2,                FIELD_INT,                 0),
	_et_gclient_addfield(sess.userSpawnPointValue,          FIELD_INT,                 0),
	_et_gclient_addfield(sess.latchPlayerType,              FIELD_INT,                 0),
	_et_gclient_addfield(sess.latchPlayerWeapon,            FIELD_INT,                 0),
	_et_gclient_addfield(sess.latchPlayerWeapon2,           FIELD_INT,                 0),
	_et_gclient_addfield(sess.ignoreClients,                FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(sess.muted,                        FIELD_INT,                 0),
	_et_gclient_addfield(sess.skillpoints,                  FIELD_FLOAT_ARRAY,         FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.startskillpoints,             FIELD_FLOAT_ARRAY,         FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.startxptotal,                 FIELD_FLOAT,               FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.skill,                        FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(sess.rank,                         FIELD_INT,                 0),
	_et_gclient_addfield(sess.medals,                       FIELD_INT_ARRAY,           0),
	_et_gclient_addfield(sess.referee,                      FIELD_INT,                 0),
	_et_gclient_addfield(sess.shoutcaster,                  FIELD_INT,                 0),
	_et_gclient_addfield(sess.rounds,                       FIELD_INT,                 0),
	_et_gclient_addfield(sess.spec_invite,                  FIELD_INT,                 0),
	_et_gclient_addfield(sess.spec_team,                    FIELD_INT,                 0),
	_et_gclient_addfield(sess.kills,                        FIELD_INT,                 0),
	_et_gclient_addfield(sess.deaths,                       FIELD_INT,                 0),
	_et_gclient_addfield(sess.gibs,                         FIELD_INT,                 0),
	_et_gclient_addfield(sess.self_kills,                   FIELD_INT,                 0),
	_et_gclient_addfield(sess.team_kills,                   FIELD_INT,                 0),
	_et_gclient_addfield(sess.team_gibs,                    FIELD_INT,                 0),
	_et_gclient_addfield(sess.damage_given,                 FIELD_INT,                 0),
	_et_gclient_addfield(sess.damage_received,              FIELD_INT,                 0),
	_et_gclient_addfield(sess.team_damage_given,            FIELD_INT,                 0),
	_et_gclient_addfield(sess.team_damage_received,         FIELD_INT,                 0),
	_et_gclient_addfield(sess.time_axis,                    FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.time_allies,                  FIELD_INT,                 FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.time_played,                  FIELD_INT,                 FIELD_FLAG_READONLY),
#ifdef FEATURE_RATING
	_et_gclient_addfield(sess.mu,                           FIELD_FLOAT,               FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.sigma,                        FIELD_FLOAT,               FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.oldmu,                        FIELD_FLOAT,               FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.oldsigma,                     FIELD_FLOAT,               FIELD_FLAG_READONLY),
#endif
#ifdef FEATURE_PRESTIGE
	_et_gclient_addfield(sess.prestige,                     FIELD_INT,                 FIELD_FLAG_READONLY),
#endif
	_et_gclient_addfield(sess.uci,                          FIELD_INT,                 0),

#ifdef LEGACY_AUTH
	_et_gclient_addfield(sess.authName,                     FIELD_STRING,              FIELD_FLAG_READONLY),
	_et_gclient_addfield(sess.authId,                       FIELD_INT,                 FIELD_FLAG_READONLY),
#endif

	_et_gclient_addfield(sess.aWeaponStats,                 FIELD_WEAPONSTAT,          FIELD_FLAG_READONLY),

	//_et_gclient_addfieldalias(aWeaponStats, sess.aWeaponStats, FIELD_WEAPONSTAT_EXT, FIELD_FLAG_READONLY),

	// origin: use ps.origin instead of r.currentOrigin
	// for client entities
	_et_gclient_addfieldalias(origin,                       ps.origin,                 FIELD_VEC3,          0),

	_et_gclient_addfieldalias(sess.team_damage,             sess.team_damage_given,    FIELD_INT,           0),
	_et_gclient_addfieldalias(sess.team_received,           sess.team_damage_received, FIELD_INT,           0),

	{ NULL },
};

// entity fields
// R/W access see see http://wolfwiki.anime.net/index.php/Fieldname
static const gentity_field_t gentity_fields[] =
{
	_et_gentity_addfield(activator,           FIELD_ENTITY,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(chain,               FIELD_ENTITY,     0),
	_et_gentity_addfield(classname,           FIELD_STRING,     0),
	_et_gentity_addfield(clipmask,            FIELD_INT,        0),
	_et_gentity_addfield(closespeed,          FIELD_FLOAT,      0),
	_et_gentity_addfield(count,               FIELD_INT,        0),
	_et_gentity_addfield(count2,              FIELD_INT,        0),
	_et_gentity_addfield(damage,              FIELD_INT,        0),
	_et_gentity_addfield(deathType,           FIELD_INT,        0),
	_et_gentity_addfield(delay,               FIELD_FLOAT,      0),
	_et_gentity_addfield(dl_atten,            FIELD_INT,        0),
	_et_gentity_addfield(dl_color,            FIELD_VEC3,       0),
	_et_gentity_addfield(dl_shader,           FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(dl_stylestring,      FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(duration,            FIELD_FLOAT,      0),
	_et_gentity_addfield(end_size,            FIELD_INT,        0),
	_et_gentity_addfield(enemy,               FIELD_ENTITY,     0),
	_et_gentity_addfield(entstate,            FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(flags,               FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(harc,                FIELD_FLOAT,      0),
	_et_gentity_addfield(health,              FIELD_INT,        0),
	_et_gentity_addfield(inuse,               FIELD_INT,        0),
	_et_gentity_addfield(isProp,              FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(item,                FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(key,                 FIELD_INT,        0),
	_et_gentity_addfield(message,             FIELD_STRING,     0),
	_et_gentity_addfield(methodOfDeath,       FIELD_INT,        0),
	_et_gentity_addfield(mg42BaseEnt,         FIELD_INT,        0),
	_et_gentity_addfield(missionLevel,        FIELD_INT,        0),
	_et_gentity_addfield(model,               FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(model2,              FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(nextTrain,           FIELD_ENTITY,     0),
	_et_gentity_addfield(noise_index,         FIELD_INT,        0),
	_et_gentity_addfield(prevTrain,           FIELD_ENTITY,     0),
	_et_gentity_addfield(props_frame_state,   FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(r.absmax,            FIELD_VEC3,       FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.absmin,            FIELD_VEC3,       FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.bmodel,            FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.contents,          FIELD_INT,        0),
	_et_gentity_addfield(r.currentAngles,     FIELD_VEC3,       0),
	_et_gentity_addfield(r.currentOrigin,     FIELD_VEC3,       0),
	_et_gentity_addfield(r.eventTime,         FIELD_INT,        0),
	//_et_gentity_addfield(r.linkcount, FIELD_INT, FIELD_FLAG_READONLY), // no need to provide it
	_et_gentity_addfield(r.linked,            FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.maxs,              FIELD_VEC3,       0),
	_et_gentity_addfield(r.mins,              FIELD_VEC3,       0),
	_et_gentity_addfield(r.ownerNum,          FIELD_INT,        0),
	_et_gentity_addfield(r.singleClient,      FIELD_INT,        0),
	_et_gentity_addfield(r.svFlags,           FIELD_INT,        0),
	_et_gentity_addfield(r.worldflags,        FIELD_INT,        FIELD_FLAG_READONLY),

	_et_gentity_addfield(radius,              FIELD_INT,        0),
	_et_gentity_addfield(random,              FIELD_FLOAT,      0),
	_et_gentity_addfield(rotate,              FIELD_VEC3,       0),

	_et_gentity_addfield(s.angles,            FIELD_VEC3,       0),
	_et_gentity_addfield(s.angles2,           FIELD_VEC3,       0),
	_et_gentity_addfield(s.apos,              FIELD_TRAJECTORY, 0),
	_et_gentity_addfield(s.clientNum,         FIELD_INT,        0),
	_et_gentity_addfield(s.constantLight,     FIELD_INT,        0),
	_et_gentity_addfield(s.density,           FIELD_INT,        0),
	_et_gentity_addfield(s.dl_intensity,      FIELD_INT,        0),
	_et_gentity_addfield(s.dmgFlags,          FIELD_INT,        0),
	_et_gentity_addfield(s.eFlags,            FIELD_INT,        0),
	_et_gentity_addfield(s.eType,             FIELD_INT,        0),
	_et_gentity_addfield(s.effect1Time,       FIELD_INT,        0),
	_et_gentity_addfield(s.effect2Time,       FIELD_INT,        0),
	_et_gentity_addfield(s.effect3Time,       FIELD_INT,        0),
	_et_gentity_addfield(s.frame,             FIELD_INT,        0),
	_et_gentity_addfield(s.groundEntityNum,   FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.loopSound,         FIELD_INT,        0),
	_et_gentity_addfield(s.modelindex,        FIELD_INT,        0),
	_et_gentity_addfield(s.modelindex2,       FIELD_INT,        0),
	_et_gentity_addfield(s.number,            FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.onFireEnd,         FIELD_INT,        0),
	_et_gentity_addfield(s.onFireStart,       FIELD_INT,        0),
	_et_gentity_addfield(s.origin,            FIELD_VEC3,       0),
	_et_gentity_addfield(s.origin2,           FIELD_VEC3,       0),
	_et_gentity_addfield(s.pos,               FIELD_TRAJECTORY, 0),
	_et_gentity_addfield(s.powerups,          FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.solid,             FIELD_INT,        0),
	_et_gentity_addfield(s.teamNum,           FIELD_INT,        0),
	_et_gentity_addfield(s.time,              FIELD_INT,        0),
	_et_gentity_addfield(s.time2,             FIELD_INT,        0),
	_et_gentity_addfield(s.weapon,            FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.eventParm,         FIELD_INT,        0),

	_et_gentity_addfield(scriptName,          FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(spawnflags,          FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(spawnitem,           FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(speed,               FIELD_INT,        0),
	_et_gentity_addfield(splashDamage,        FIELD_INT,        0),
	_et_gentity_addfield(splashMethodOfDeath, FIELD_INT,        0),
	_et_gentity_addfield(splashRadius,        FIELD_INT,        0),
	_et_gentity_addfield(start_size,          FIELD_INT,        0),
	_et_gentity_addfield(tagName,             FIELD_STRING,     FIELD_FLAG_NOPTR + FIELD_FLAG_READONLY),
	_et_gentity_addfield(tagParent,           FIELD_ENTITY,     0),
	_et_gentity_addfield(takedamage,          FIELD_INT,        0),
	_et_gentity_addfield(tankLink,            FIELD_ENTITY,     0),
	_et_gentity_addfield(target,              FIELD_STRING,     0),
	_et_gentity_addfield(TargetAngles,        FIELD_VEC3,       0),
	_et_gentity_addfield(TargetFlag,          FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(targetname,          FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(teamchain,           FIELD_ENTITY,     0),
	_et_gentity_addfield(teammaster,          FIELD_ENTITY,     0),
	_et_gentity_addfield(track,               FIELD_STRING,     FIELD_FLAG_READONLY),
	_et_gentity_addfield(varc,                FIELD_FLOAT,      0),
	_et_gentity_addfield(wait,                FIELD_FLOAT,      0),
	_et_gentity_addfield(waterlevel,          FIELD_INT,        FIELD_FLAG_READONLY),
	_et_gentity_addfield(watertype,           FIELD_INT,        FIELD_FLAG_READONLY),

	// To be compatible with ETPro:
	// origin: use r.currentOrigin instead of ps.origin
	//         for non client entities
	_et_gentity_addfieldalias(origin,         r.currentOrigin,  FIELD_VEC3,                             0),
	{ NULL },
};

// gentity fields helper functions
static gentity_field_t *_etH_gentity_getfield(gentity_t *ent, char *fieldname)
{
	int i;

	// search through client fields first
	if (ent->client)
	{
		for (i = 0; gclient_fields[i].name; i++)
		{
			if (Q_stricmp(fieldname, gclient_fields[i].name) == 0)
			{
				return (gentity_field_t *)&gclient_fields[i];
			}
		}
	}

	for (i = 0; gentity_fields[i].name; i++)
	{
		if (Q_stricmp(fieldname, gentity_fields[i].name) == 0)
		{
			return (gentity_field_t *)&gentity_fields[i];
		}
	}

	return 0;
}

static void _etH_gentity_getvec3(lua_State *L, vec3_t vec3)
{
	lua_newtable(L);
	lua_pushnumber(L, vec3[0]);
	lua_rawseti(L, -2, 1);
	lua_pushnumber(L, vec3[1]);
	lua_rawseti(L, -2, 2);
	lua_pushnumber(L, vec3[2]);
	lua_rawseti(L, -2, 3);
}

static void _etH_gentity_setvec3(lua_State *L, vec3_t *vec3)
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

static void _etH_gentity_gettrajectory(lua_State *L, trajectory_t *traj)
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
	_etH_gentity_getvec3(L, traj->trBase);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trDelta");
	_etH_gentity_getvec3(L, traj->trDelta);
	lua_settable(L, -3);
}

static void _etH_gentity_settrajectory(lua_State *L, trajectory_t *traj)
{
	lua_pushstring(L, "trType");
	lua_gettable(L, -2);
	traj->trType = (trType_t)lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trTime");
	lua_gettable(L, -2);
	traj->trTime = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trDuration");
	lua_gettable(L, -2);
	traj->trDuration = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trBase");
	lua_gettable(L, -2);
	_etH_gentity_setvec3(L, (vec3_t *)traj->trBase);
	lua_pop(L, 1);
	lua_pushstring(L, "trDelta");
	lua_gettable(L, -2);
	_etH_gentity_setvec3(L, (vec3_t *)traj->trDelta);
	lua_pop(L, 1);
}

static void _etH_gentity_getweaponstat(lua_State *L, weapon_stat_t *ws)
{
	lua_newtable(L);
	lua_pushinteger(L, 1);
	lua_pushinteger(L, ws->atts);
	lua_settable(L, -3);
	lua_pushinteger(L, 2);
	lua_pushinteger(L, ws->deaths);
	lua_settable(L, -3);
	lua_pushinteger(L, 3);
	lua_pushinteger(L, ws->headshots);
	lua_settable(L, -3);
	lua_pushinteger(L, 4);
	lua_pushinteger(L, ws->hits);
	lua_settable(L, -3);
	lua_pushinteger(L, 5);
	lua_pushinteger(L, ws->kills);
	lua_settable(L, -3);
}

gentity_t *G_Lua_CreateEntity(char *params)
{
	gentity_t *create;
	char      *token;
	char      *p = params;
	char      key[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];

	level.numSpawnVars     = 0;
	level.numSpawnVarChars = 0;

	while (1)
	{
		token = COM_ParseExt(&p, qfalse);
		if (!token[0])
		{
			break;
		}

		Q_strncpyz(key, token, sizeof(key));

		token = COM_ParseExt(&p, qfalse);
		if (!token[0])
		{
			// note: we migth do more than a simple return here
			// nextmap?
			G_Printf("%s API: spawn key \"%s\" has no valu\n", LUA_VERSION, key);
			return NULL;
		}

		Q_strncpyz(value, token, sizeof(value));

		if (g_scriptDebug.integer)
		{
			G_Printf("%s API %d: set [%s] [%s] [%s]\n", LUA_VERSION, level.time, MODNAME, key, value);
		}

		if (level.numSpawnVars == MAX_SPAWN_VARS)
		{
			// see above note
			G_Printf("%s API: can't spawn an entity - MAX_SPAWN_VARS reached.\n", LUA_VERSION);
			return NULL;
		}

		level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken(key);
		level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken(value);

		level.numSpawnVars++;
	}

	create = G_SpawnGEntityFromSpawnVars();

	if (!create)
	{
		return NULL;
	}

	trap_LinkEntity(create);
	return create;
}


/**
 * Creates a new entity.
 *
 * This function expects same as G_ScriptAction_Create -  keys & values
 * see http://wolfwiki.anime.net/index.php/Map_scripting
 * was et.G_Spawn() before 2.75 (... and  did not work)
 *
 * @lua_def_prototype et.G_CreateEntity(params)
 * @lua_def ---@param params string mapscript parameters
 * @lua_def ---@return number entnum the returned number of the new entity.
 */
static int _et_G_CreateEntity(lua_State *L)
{
	gentity_t *entnum;
	char      *params = (char *)luaL_checkstring(L, 1); // make 2 params for classname?

	entnum = G_Lua_CreateEntity(params);

	if (entnum == NULL)
	{
		//luaL_error(L, "can't create entity");
		return 0;
	}

	lua_pushinteger(L, entnum - g_entities);

	return 1;
}

/**
 * Deletes an entity.
 *
 * @lua_def_prototype et.G_DeleteEntity(params)
 * @lua_def ---@param params string mapscript parameters
 * @lua_def ---@return number deleted '1' if actual entities were deleted, '0' otherwise
 */
static int _et_G_DeleteEntity(lua_State *L)
{
	char *params = (char *)luaL_checkstring(L, 1);

	lua_pushinteger(L, G_ScriptAction_Delete(NULL, params));
	return 1;
}

/**
 * Spawns a new temp entity to a location.
 *
 * @lua_def_prototype et.G_TempEntity(origin, event)
 * @lua_def ---@param origin Vec3 the location the temp entity is placed.
 * @lua_def ---@param event number the event type of the entity. See `Event types <misc.html#event-types>`__ for possible values.
 * @lua_def ---@return number entnum the returned the number of the new entity.
 */
static int _et_G_TempEntity(lua_State *L)
{
	vec3_t origin;
	int    event = (int)luaL_checkinteger(L, 2);

	lua_pop(L, 1);
	_etH_gentity_setvec3(L, &origin);
	lua_pushinteger(L, G_TempEntity(origin, event) - g_entities);
	return 1;
}

/**
 * Marks a given entity as free.
 *
 * @lua_def_prototype et.G_FreeEntity(entnum)
 * @lua_def ---@param entnum number entity number of the entity to be freed.
 */
static int _et_G_FreeEntity(lua_State *L)
{
	int entnum = (int)luaL_checkinteger(L, 1);

	G_FreeEntity(g_entities + entnum);
	return 1;
}

/**
 * Calculates all free entities.
 *
 * NOTE: Free client entities (slots) are not counted.
 *
 * @lua_def_prototype et.G_EntitiesFree()
 * @lua_def ---@return number count the returned number of free entities.
 */
static int _et_G_EntitiesFree(lua_State *L)
{
	lua_pushinteger(L, G_EntitiesFree());
	return 1;
}

/**
 * Sets an entity state.
 *
 * @lua_def_prototype et.G_SetEntState(entnum, newstate)
 * @lua_def ---@param entnum number the entity number.
 * @lua_def ---@param newstate number the new entity state.
 */
static int _et_G_SetEntState(lua_State *L)
{
	gentity_t  *ent;
	int        entnum   = (int)luaL_checkinteger(L, 1);
	entState_t newstate = (int)luaL_checkinteger(L, 2);

	if (entnum > -1 && entnum < ENTITYNUM_MAX_NORMAL) // don't do this with world ent
	{
		ent = g_entities + entnum;
		G_SetEntState(ent, newstate);
	}
	else
	{
		luaL_error(L, "entity number \"%d\" is out of range", entnum);
	}
	return 0;
}

/**
 * Links an entity.
 *
 * @lua_def_prototype et.trap_LinkEntity(entnum)
 * @lua_def ---@param entnum number the entity number to link.
 */
static int _et_trap_LinkEntity(lua_State *L)
{
	int entnum = (int)luaL_checkinteger(L, 1);

	trap_LinkEntity(g_entities + entnum);
	return 0;
}

/**
 * Unlinks an entity.
 *
 * @lua_def_prototype et.trap_UnlinkEntity(entnum)
 * @lua_def ---@param entnum number the entity number to unlink.
 */
static int _et_trap_UnlinkEntity(lua_State *L)
{
	int entnum = (int)luaL_checkinteger(L, 1);

	trap_UnlinkEntity(g_entities + entnum);
	return 0;
}

/**
 * Returns a value of a spawnVar.
 *
 * This function works with fields ( g_spawn.c @ 72 )
 *
 * Description:
 *   The mapper, using his map-editor, assigns spawnvars.
 *   Spawnvars, and their values, are represented in code as members of gentity_t.
 *   Spawnvar names can be different from the corresponding gentity_t membernames.
 *   For example the spawnvar "shortname" is used with trigger_objective_info entities in the map-editor,
 *    while in code the gentity_t membername is "message"..
 *   This function _et_G_GetSpawnVar() returns the value of a gentity_t member,
 *    where the argument is a spawnvar name.
 *   (the array called "fields" in g_spawn.c is a mapping of spawnvars<->members)
 *
 * @lua_def_prototype et.G_GetSpawnVar(entnum, key)
 * @lua_def ---@param entnum number the entity number of the target.
 * @lua_def ---@param key string the key for the value to return.
 *
 * @lua_def ---@return string|number|Vec3|nil spawnVal the returned spawn value.
 */
static int _et_G_GetSpawnVar(lua_State *L)
{
	gentity_t   *ent;
	int         entnum = (int)luaL_checkinteger(L, 1);
	const char  *key   = luaL_checkstring(L, 2);
	int         index  = GetFieldIndex(key);
	fieldtype_t type   = GetFieldType(key);
	int         ofs;

	// break on invalid gentity field
	if (index == -1)
	{
		luaL_error(L, "field \"%s\" index is -1", key);
		return 0;
	}

	if (entnum < 0 || entnum >= MAX_GENTITIES)
	{
		luaL_error(L, "entnum \"%d\" is out of range", entnum);
		return 0;
	}

	ent = &g_entities[entnum];

	// If the entity is not in use, return nil
	if (!ent->inuse)
	{
		lua_pushnil(L);
		return 1;
	}

	ofs = fields[index].ofs;

	switch (type)
	{
	case F_INT:
		lua_pushinteger(L, *(int *) ((byte *)ent + ofs));
		return 1;
	case F_FLOAT:
		lua_pushnumber(L, *(float *) ((byte *)ent + ofs));
		return 1;
	case F_LSTRING:
	case F_GSTRING:
		if (fields[index].flags & FIELD_FLAG_NOPTR)
		{
			lua_pushstring(L, (char *) ((byte *)ent + ofs));
		}
		else
		{
			lua_pushstring(L, *(char **) ((byte *)ent + ofs));
		}
		return 1;
	case F_VECTOR:
	case F_ANGLEHACK:
		_etH_gentity_getvec3(L, *(vec3_t *)((byte *)ent + ofs));
		return 1;
	case F_ENTITY:
	{
		// return the entity-number  of the entity that the pointer is pointing at.
		int entNum = C_gentity_ptr_to_entNum(*(int *)((byte *)ent + ofs));

		if (entNum < 0)
		{
			lua_pushnil(L);
		}
		else
		{
			lua_pushinteger(L, entNum);
		}
	}
		return 1;
	case F_ITEM:
	case F_CLIENT:
	case F_IGNORE:
	default:
		lua_pushnil(L);
		return 1;
	}
	return 0;
}

/**
 * Sets spawn value to an entity.
 *
 * This function works with fields ( g_spawn.c @ 72 )
 *
 * @lua_def_prototype et.G_SetSpawnVar(entnum, key, value)
 * @lua_def ---@param entnum number the target entity.
 * @lua_def ---@param key string the key for the value..
 * @lua_def ---@param value string|number|Vec3 the new value for the key.
 */
static int _et_G_SetSpawnVar(lua_State *L)
{
	gentity_t   *ent;
	int         entnum = (int)luaL_checkinteger(L, 1);
	const char  *key   = luaL_checkstring(L, 2);
	int         index  = GetFieldIndex(key);
	fieldtype_t type   = GetFieldType(key);
	int         ofs;
	const char  *buffer;

	// break on invalid gentity field
	if (index == -1)
	{
		luaL_error(L, "field \"%s\" index is -1", key);
		return 0;
	}

	if (entnum < 0 || entnum >= MAX_GENTITIES)
	{
		luaL_error(L, "entnum \"%d\" is out of range", entnum);
		return 0;
	}

	ent = &g_entities[entnum];

	// If the entity is not in use, return nil
	if (!ent->inuse)
	{
		lua_pushnil(L);
		return 1;
	}

	ofs = fields[index].ofs;

	switch (type)
	{
	case F_INT:
		*(int *) ((byte *)ent + ofs) = (int)luaL_checkinteger(L, 3);
		return 1;
	case F_FLOAT:
		*(float *) ((byte *)ent + ofs) = (float)luaL_checknumber(L, 3);
		return 1;
	case F_LSTRING:
	case F_GSTRING:
		buffer = luaL_checkstring(L, 3);
		if (fields[index].flags & FIELD_FLAG_NOPTR)
		{
			Q_strncpyz((char *)((byte *)ent + ofs), buffer, MAX_STRING_CHARS);
		}
		else
		{
			Com_Dealloc(*(char **)((byte *)ent + ofs));
			*(char **)((byte *)ent + ofs) = Com_Allocate(strlen(buffer));
			Q_strncpyz(*(char **)((byte *)ent + ofs), buffer, strlen(buffer));
		}
		return 1;
	case F_VECTOR:
	case F_ANGLEHACK:
		_etH_gentity_setvec3(L, (vec3_t *)((byte *)ent + ofs));
		return 1;
	case F_ENTITY:
		// pointer-fields are read-only..
		//*(gentity_t **)((byte *)ent + ofs) = g_entities + (int)luaL_checkinteger(L, 3);
		return 0;
	case F_ITEM:
	case F_CLIENT:
	case F_IGNORE:
	default:
		lua_pushnil(L);
		return 1;
	}

	return 0;
}

/**
 * Returns a field value associated with an entity.
 *
 * NOTE: `arrayindex` is required when accessing array type fields. Array indexes start at 0.
 *
 * @lua_def_prototype et.gentity_get(entnum, fieldname, arrayindex)
 * @lua_def ---@param entnum number the number of the entity.
 * @lua_def ---@param fieldname string the name of the field to get.
 * @lua_def ---@param arrayindex? number if present, specifies which element of an array entity field to get.
 * @lua_def ---@return nil|string|number|number[] value the returned field value. For NULL entities or clients, **nil** is returned.
 */
static int _et_gentity_get(lua_State *L)
{
	gentity_t       *ent       = g_entities + (int)luaL_checkinteger(L, 1);
	const char      *fieldname = luaL_checkstring(L, 2);
	gentity_field_t *field     = _etH_gentity_getfield(ent, (char *)fieldname);
	uintptr_t       addr;

	// break on invalid gentity field
	if (!field)
	{
		luaL_error(L, "tried to get invalid gentity field \"%s\"", fieldname);
		return 0;
	}

	if (field->flags & FIELD_FLAG_GENTITY)
	{
		addr = (uintptr_t)ent;
	}
	else
	{
		addr = (uintptr_t)ent->client;
	}

	// for NULL entities, return nil (prevents server crashes!)
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
		// core: return the entity-number of the entity that the pointer is pointing at.
		int entNum = C_gentity_ptr_to_entNum(*(uintptr_t *)addr);

		if (entNum < 0)
		{
			lua_pushnil(L);
		}
		else
		{
			lua_pushinteger(L, entNum);
		}
	}
		return 1;
	case FIELD_VEC3:
		_etH_gentity_getvec3(L, *(vec3_t *)addr);
		return 1;
	case FIELD_INT_ARRAY:
		lua_pushinteger(L, (*(int *)(addr + (sizeof(int) * (int)luaL_optinteger(L, 3, 0)))));
		return 1;
	case FIELD_TRAJECTORY:
		_etH_gentity_gettrajectory(L, (trajectory_t *)addr);
		return 1;
	case FIELD_FLOAT_ARRAY:
		lua_pushnumber(L, (*(float *)(addr + (sizeof(int) * (int)luaL_optinteger(L, 3, 0)))));
		return 1;
	case FIELD_WEAPONSTAT:
		_etH_gentity_getweaponstat(L, (weapon_stat_t *)(addr + (sizeof(weapon_stat_t) * (int)luaL_optinteger(L, 3, 0))));
		return 1;

	}
	return 0;
}

/**
 * Sets a value in an entity.
 *
 * @lua_def_prototype et.gentity_set(entnum, fieldname, val1, val2)
 * @lua_def ---@param entnum number the entity number that is manipulated.
 * @lua_def ---@param fieldname string the name of the field to manipulate.
 * @lua_def ---@param val1 nil|string|number|number[] the value to be set - if 'val2' is set 'val1' becomes the index to be set for the vector-field and 'val2' the value to be set.
 * @lua_def ---@param val2? nil|string|number if set, makes 'val1' the index of the vector-field and 'val2' the value of that index to be set.
 */
static int _et_gentity_set(lua_State *L)
{
	gentity_t       *ent       = g_entities + (int)luaL_checkinteger(L, 1);
	const char      *fieldname = luaL_checkstring(L, 2);
	gentity_field_t *field     = _etH_gentity_getfield(ent, (char *)fieldname);
	uintptr_t       addr;
	const char      *buffer;

	// break on invalid gentity field
	if (!field)
	{
		luaL_error(L, "tried to set invalid gentity field \"%s\"", fieldname);
		return 0;
	}

	// break on read-only gentity field
	if (field->flags & FIELD_FLAG_READONLY)
	{
		luaL_error(L, "tried to set read-only gentity field \"%s\"", fieldname);
		return 0;
	}

	if (field->flags & FIELD_FLAG_GENTITY)
	{
		addr = (uintptr_t )ent;
	}
	else
	{
		addr = (uintptr_t)ent->client;
	}

	// for NULL entities, return nil (prevents server crashes!)
	if (!addr)
	{
		lua_pushnil(L);
		return 1;
	}

	addr += field->mapping;

	switch (field->type)
	{
	case FIELD_INT:
		*(int *)addr = (int)luaL_checkinteger(L, 3);
		break;
	case FIELD_STRING:
		buffer = luaL_checkstring(L, 3);
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
	case FIELD_FLOAT:
		*(float *)addr = (float)luaL_checknumber(L, 3);
		break;
	case FIELD_ENTITY:
		// pointer-fields are read-only..
		break;
	case FIELD_VEC3:
		_etH_gentity_setvec3(L, (vec3_t *)addr);
		break;
	case FIELD_INT_ARRAY:
		*(int *)(addr + (sizeof(int) * (int)luaL_checkinteger(L, 3))) = (int)luaL_checkinteger(L, 4);
		break;
	case FIELD_TRAJECTORY:
		_etH_gentity_settrajectory(L, (trajectory_t *)addr);
		break;
	case FIELD_FLOAT_ARRAY:
		*(float *)(addr + (sizeof(int) * (int)luaL_checkinteger(L, 3))) = luaL_checknumber(L, 4);
		return 1;
	default:
		G_Printf("Lua API: et.gentity_set with no valid field type\n");
		break;
	}
	return 0;
}

/**
 * Adds an event to the entity event sequence.
 *
 * @lua_def_prototype et.G_AddEvent(ent, event, eventparm)
 * @lua_def ---@param ent number the entity which event sequence is handled.
 * @lua_def ---@param event number the event to add.
 * @lua_def ---@param eventparm number optional parameter for the event.
 */
static int _et_G_AddEvent(lua_State *L)
{
	int ent       = (int)luaL_checkinteger(L, 1);
	int event     = (int)luaL_checkinteger(L, 2);
	int eventparm = (int)luaL_checkinteger(L, 3);
	G_AddEvent(g_entities + ent, event, eventparm);
	return 0;
}

// Shaders

/**
 * Remaps shader.
 *
 * @lua_def_prototype et.G_ShaderRemap(oldShader, newShader)
 * @lua_def ---@param oldShader string the old shader.
 * @lua_def ---@param newShader string the new shader.
 */
static int _et_G_ShaderRemap(lua_State *L)
{
	float      f          = level.time * 0.001;
	const char *oldShader = luaL_checkstring(L, 1);
	const char *newShader = luaL_checkstring(L, 2);

	AddRemap(oldShader, newShader, f);
	return 0;
}

/**
 * Resets remapped shaders.
 *
 * @lua_def_prototype et.G_ResetRemappedShaders()
 */
static int _et_G_ResetRemappedShaders(lua_State *L)
{
	G_ResetRemappedShaders();
	return 0;
}

/**
 * Flushes remapped shaders.
 *
 * @lua_def_prototype et.G_ShaderRemapFlush()
 */
static int _et_G_ShaderRemapFlush(lua_State *L)
{
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
	return 0;
}

/**
 * Sets global fog to a specific color and density.
 *
 * e.g.
 * setglobalfog 0 <duration> <float:r> <float:g> <float:b> <float:depthForOpaque>
 * setglobalfog 1 <duration>
 *
 * @lua_def_prototype et.G_SetGlobalFog(params)
 * @lua_def ---@param params string all the parameters to be passed on packed into a string.
 */
static int _et_G_SetGlobalFog(lua_State *L)
{
	char *params = (char *)luaL_checkstring(L, 1);

	lua_pushinteger(L, G_ScriptAction_SetGlobalFog(NULL, params));
	return 1;
}

static void _etH_getclipplane(lua_State *L, cplane_t *plane)
{
	lua_newtable(L);
	_etH_gentity_getvec3(L, plane->normal);
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

static void _etH_gettrace(lua_State *L, trace_t *tr)
{
	lua_newtable(L);
	lua_pushboolean(L, tr->allsolid);
	lua_setfield(L, -2, "allsolid");
	lua_pushboolean(L, tr->startsolid);
	lua_setfield(L, -2, "startsolid");
	lua_pushnumber(L, tr->fraction);
	lua_setfield(L, -2, "fraction");
	_etH_gentity_getvec3(L, tr->endpos);
	lua_setfield(L, -2, "endpos");
	_etH_getclipplane(L, &tr->plane);
	lua_setfield(L, -2, "plane");
	lua_pushinteger(L, tr->surfaceFlags);
	lua_setfield(L, -2, "surfaceFlags");
	lua_pushinteger(L, tr->contents);
	lua_setfield(L, -2, "contents");
	lua_pushinteger(L, tr->entityNum);
	lua_setfield(L, -2, "entityNum");
}

static vec3_t *_etH_toVec3(lua_State *L, int inx)
{
	static vec3_t vec;

	lua_pushvalue(L, inx);
	_etH_gentity_setvec3(L, &vec);
	lua_pop(L, 1);

	return &vec;
}

/**
 * Traces an entity.
 *
 * @lua_def_prototype et.trap_Trace(start, mins, maxs, endPos, entNum, mask)
 * @lua_def ---@param start Vec3 the starting position.
 * @lua_def ---@param mins Vec3 the minimum point of the bounding box.
 * @lua_def ---@param maxs Vec3 the maximum point of the bounding box.
 * @lua_def ---@param endPos Vec3 the ending position.
 * @lua_def ---@param entNum number the entity number that is being ignored by the trace function.
 * @lua_def ---@param mask number the content mask.
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
	_etH_gettrace(L, &tr);

	return 1;
}

/**
 * Runs a trace with players in historical positions.
 *
 * @lua_def_prototype et.G_HistoricalTrace(ent, start, mins, maxs, endPos, entNum, mask)
 * @lua_def ---@param ent number the entity which trace history is handled.
 * @lua_def ---@param start Vec3 the starting position.
 * @lua_def ---@param mins Vec3 the minimum point of the bounding box.
 * @lua_def ---@param maxs Vec3 the maximum point of the bounding box.
 * @lua_def ---@param endPos Vec3 the ending position.
 * @lua_def ---@param entNum number the entity number that is being ignored by the trace function.
 * @lua_def ---@param mask number the content mask.
 */
static int _et_G_HistoricalTrace(lua_State *L)
{
	gentity_t *gent;
	trace_t   tr;
	vec3_t    start, mins, maxs, end;
	vec3_t    *minsPtr = NULL, *maxsPtr = NULL;
	int       entNum, mask, ent;

	ent = luaL_checkinteger(L, 1);

	if (ent < 0 || ent >= MAX_GENTITIES)
	{
		luaL_error(L, "G_HistoricalTrace: \"ent\" is out of bounds");
	}

	gent = g_entities + ent;

	if (!lua_istable(L, 2))
	{
		luaL_error(L, "G_HistoricalTrace: \"start\" argument should be an instance of table");
	}

	VectorCopy(*_etH_toVec3(L, 2), start);

	if (lua_istable(L, 3))
	{
		VectorCopy(*_etH_toVec3(L, 3), mins);
		minsPtr = &mins;
	}

	if (lua_istable(L, 4))
	{
		VectorCopy(*_etH_toVec3(L, 4), maxs);
		maxsPtr = &maxs;
	}

	if (!lua_istable(L, 5))
	{
		luaL_error(L, "G_HistoricalTrace: \"end\" should be an instance of table");
	}

	VectorCopy(*_etH_toVec3(L, 5), end);

	entNum = luaL_checkinteger(L, 6);
	mask   = luaL_checkinteger(L, 7);

	G_HistoricalTrace(gent, &tr, start, *minsPtr, *maxsPtr, end, entNum, mask);
	_etH_gettrace(L, &tr);

	return 1;
}

/** @}*/ // doxygen addtogroup lua_etfncs

// et library initialization array
static const luaL_Reg etlib[] =
{
	// ET Library Calls
	{ "RegisterModname",         _et_RegisterModname         },
	{ "FindSelf",                _et_FindSelf                },
	{ "FindMod",                 _et_FindMod                 },
	{ "IPCSend",                 _et_IPCSend                 },
	// Printing
	{ "G_Print",                 _et_G_Print                 },
	{ "G_LogPrint",              _et_G_LogPrint              },
	// Argument Handling
	{ "ConcatArgs",              _et_ConcatArgs              },
	{ "trap_Argc",               _et_trap_Argc               },
	{ "trap_Argv",               _et_trap_Argv               },
	// Cvars
	{ "trap_Cvar_Get",           _et_trap_Cvar_Get           },
	{ "trap_Cvar_Set",           _et_trap_Cvar_Set           },
	// Config Strings
	{ "trap_GetConfigstring",    _et_trap_GetConfigstring    },
	{ "trap_SetConfigstring",    _et_trap_SetConfigstring    },
	// Server
	{ "trap_SendConsoleCommand", _et_trap_SendConsoleCommand },
	// Clients
	{ "trap_SendServerCommand",  _et_trap_SendServerCommand  },
	{ "trap_DropClient",         _et_trap_DropClient         },
	{ "ClientNumberFromString",  _et_ClientNumberFromString  },
	//{"trap_SendMessage",			_et_trap_SendMessage},
	//{"trap_MessageStatus",			_et_trap_MessageStatus},
	{ "G_Say",                   _et_G_Say                   },
	{ "MutePlayer",              _et_MutePlayer              },
	{ "UnmutePlayer",            _et_UnmutePlayer            },
	// Userinfo
	{ "trap_GetUserinfo",        _et_trap_GetUserinfo        },
	{ "trap_SetUserinfo",        _et_trap_SetUserinfo        },
	{ "ClientUserinfoChanged",   _et_ClientUserinfoChanged   },
	// String Utility
	{ "Info_RemoveKey",          _et_Info_RemoveKey          },
	{ "Info_SetValueForKey",     _et_Info_SetValueForKey     },
	{ "Info_ValueForKey",        _et_Info_ValueForKey        },
	{ "Q_CleanStr",              _et_Q_CleanStr              },
	// ET Filesystem
	{ "trap_FS_FOpenFile",       _et_trap_FS_FOpenFile       },
	{ "trap_FS_Read",            _et_trap_FS_Read            },
	{ "trap_FS_Write",           _et_trap_FS_Write           },
	{ "trap_FS_Rename",          _et_trap_FS_Rename          },
	{ "trap_FS_FCloseFile",      _et_trap_FS_FCloseFile      },
	{ "trap_FS_GetFileList",     _et_trap_FS_GetFileList     },
	// Indexes
	{ "G_SoundIndex",            _et_G_SoundIndex            },
	{ "G_ModelIndex",            _et_G_ModelIndex            },
	// Sound
	{ "G_globalSound",           _et_G_globalSound           },
	{ "G_Sound",                 _et_G_Sound                 },
	{ "G_ClientSound",           _et_G_ClientSound           },
	// Miscellaneous
	{ "trap_Milliseconds",       _et_trap_Milliseconds       },
	{ "isBitSet",                _et_isBitSet                },
	{ "G_Damage",                _et_G_Damage                },
	{ "G_AddSkillPoints",        _et_G_AddSkillPoints        },
	{ "G_LoseSkillPoints",       _et_G_LoseSkillPoints       },
	{ "G_XP_Set",                _et_G_XP_Set                },
	{ "G_ResetXP",               _et_G_ResetXP               },
	{ "AddWeaponToPlayer",       _et_AddWeaponToPlayer       },
	{ "RemoveWeaponFromPlayer",  _et_RemoveWeaponFromPlayer  },
	{ "GetCurrentWeapon",        _et_GetCurrentWeapon        },
	// Entities
	{ "G_CreateEntity",          _et_G_CreateEntity          },
	{ "G_DeleteEntity",          _et_G_DeleteEntity          },
	{ "G_TempEntity",            _et_G_TempEntity            },
	{ "G_FreeEntity",            _et_G_FreeEntity            },
	{ "G_EntitiesFree",          _et_G_EntitiesFree          },
	{ "G_SetEntState",           _et_G_SetEntState           },
	{ "trap_LinkEntity",         _et_trap_LinkEntity         },
	{ "trap_UnlinkEntity",       _et_trap_UnlinkEntity       },
	{ "G_GetSpawnVar",           _et_G_GetSpawnVar           },
	{ "G_SetSpawnVar",           _et_G_SetSpawnVar           },
	{ "gentity_get",             _et_gentity_get             },
	{ "gentity_set",             _et_gentity_set             },
	{ "G_AddEvent",              _et_G_AddEvent              },
	// Shaders
	{ "G_ShaderRemap",           _et_G_ShaderRemap           },
	{ "G_ResetRemappedShaders",  _et_G_ResetRemappedShaders  },
	{ "G_ShaderRemapFlush",      _et_G_ShaderRemapFlush      },
	{ "G_SetGlobalFog",          _et_G_SetGlobalFog          },
	{ "trap_Trace",              _et_trap_Trace              },
	{ "G_HistoricalTrace",       _et_G_HistoricalTrace       },
	{ NULL },
};

/*************/
/* Lua API   */
/*************/

/*
 * G_LuaRunIsolated(modName)
 * Creates and runs specified module in isolated state
 */
qboolean G_LuaRunIsolated(const char *modName)
{
	int          freeVM, flen = 0;
	static char  allowedModules[MAX_CVAR_VALUE_STRING];
	char         filename[MAX_OSPATH];
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

	Q_strncpyz(filename, modName, sizeof(filename));

	if (!COM_CompareExtension(filename, ".lua"))
	{
		Q_strcat(filename, sizeof(filename), ".lua");
	}

	Q_strncpyz(allowedModules, Q_strupr(lua_allowedModules.string), sizeof(allowedModules));

	// try to open lua file
	flen = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (flen < 0)
	{
		G_Printf("%s API: %scan not open file '%s'\n", LUA_VERSION, S_COLOR_BLUE, filename);
		return qfalse;
	}
	else if (flen > LUA_MAX_FSIZE)
	{
		// Let's not load arbitrarily big files to memory.
		// If your lua file exceeds the limit, let me know.
		G_Printf("%s API: %signoring file '%s' (too big)\n", LUA_VERSION, S_COLOR_BLUE, filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	code = Com_Allocate(flen + 1);

	if (code == NULL)
	{
		G_Error("%s API: %smemory allocation error for '%s' data\n", LUA_VERSION, S_COLOR_BLUE, filename);
	}

	trap_FS_Read(code, flen, f);
	*(code + flen) = '\0';
	trap_FS_FCloseFile(f);
	signature = G_SHA1(code);

	if (Q_stricmp(lua_allowedModules.string, "") && !strstr(allowedModules, signature))
	{
		// don't load disallowed lua modules into vm
		Com_Dealloc(code); // fixed memory leaking in Lua API - thx ETPub/goesa
		G_Printf("%s API: %sLua module [%s] [%s] disallowed by ACL\n", LUA_VERSION, S_COLOR_BLUE, filename, signature);
	}
	else
	{
		// Init lua_vm_t struct
		vm = (lua_vm_t *)Com_Allocate(sizeof(lua_vm_t));

		if (vm == NULL)
		{
			G_Error("%s API: %svm memory allocation error for %s data\n", LUA_VERSION, S_COLOR_BLUE, filename);
		}

		vm->id = -1;
		Q_strncpyz(vm->file_name, filename, sizeof(vm->file_name));
		Q_strncpyz(vm->mod_name, "", sizeof(vm->mod_name));
		Q_strncpyz(vm->mod_signature, signature, sizeof(vm->mod_signature));
		vm->code      = code;
		vm->code_size = flen;
		vm->err       = 0;

		// Start lua virtual machine
		if (G_LuaStartVM(vm))
		{
			vm->id      = freeVM;
			lVM[freeVM] = vm;
			return qtrue;
		}
		else
		{
			G_LuaStopVM(vm);
		}
	}

	return qfalse;
}


/*
 * G_LuaInit()
 * Initialises the Lua API interface
 */
qboolean G_LuaInit(void)
{
	int          i, num_vm = 0, len;
	char         buff[MAX_CVAR_VALUE_STRING], *crt, *list, *pList;
	fileHandle_t f;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		lVM[i] = NULL;
	}

	if (g_luaModuleList.string[0])
	{
		if (lua_modules.string[0])
		{
			G_Printf("%s API: %slua_modules cvar will be ignored since g_luaModuleList is set\n", LUA_VERSION, S_COLOR_BLUE);
		}

		len = trap_FS_FOpenFile(g_luaModuleList.string, &f, FS_READ);
		if (len < 0)
		{
			G_Printf("%s API: %scan not open file '%s'\n", LUA_VERSION, S_COLOR_BLUE, g_luaModuleList.string);
			return qfalse;
		}

		list = Com_Allocate(len + 1);

		if (list == NULL)
		{
			G_Error("%s API: %smemory allocation error for '%s' data\n", LUA_VERSION, S_COLOR_BLUE, g_luaModuleList.string);
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

			if (G_LuaRunIsolated(crt))
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

				if (G_LuaRunIsolated(crt))
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

/*
 * G_LuaCall( func, vm, nargs, nresults )
 * Calls a function already on the stack.
 */
qboolean G_LuaCall(lua_vm_t *vm, const char *func, int nargs, int nresults)
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

/*
 * G_LuaGetNamedFunction( vm, name )
 * Finds a function by name and puts it onto the stack.
 * If the function does not exist, returns qfalse.
 */
qboolean G_LuaGetNamedFunction(lua_vm_t *vm, const char *name)
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
 * @brief Dump the lua stack to console
 *        Executed by the ingame "lua_api" command
 */
void G_LuaStackDump()
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
	if (G_LuaStartVM(vm))
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

static void registerConfigstringConstants(lua_vm_t *vm)
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

static void registerEntityFlags(lua_vm_t *vm)
{
	lua_regconstinteger(vm->L, EF_NONE);
	lua_regconstinteger(vm->L, EF_DEAD);
	lua_regconstinteger(vm->L, EF_NONSOLID_BMODEL);
	lua_regconstinteger(vm->L, EF_TELEPORT_BIT);
	lua_regconstinteger(vm->L, EF_READY);

	lua_regconstinteger(vm->L, EF_CROUCHING);
	lua_regconstinteger(vm->L, EF_MG42_ACTIVE);
	lua_regconstinteger(vm->L, EF_NODRAW);
	lua_regconstinteger(vm->L, EF_FIRING);
	lua_regconstinteger(vm->L, EF_INHERITSHADER);

	lua_regconstinteger(vm->L, EF_SPINNING);
	lua_regconstinteger(vm->L, EF_BREATH);
	lua_regconstinteger(vm->L, EF_TALK);
	lua_regconstinteger(vm->L, EF_CONNECTION);
	lua_regconstinteger(vm->L, EF_SMOKINGBLACK);

	lua_regconstinteger(vm->L, EF_HEADSHOT);
	lua_regconstinteger(vm->L, EF_SMOKING);
	lua_regconstinteger(vm->L, EF_OVERHEATING);
	lua_regconstinteger(vm->L, EF_VOTED);
	lua_regconstinteger(vm->L, EF_TAGCONNECT);
	lua_regconstinteger(vm->L, EF_MOUNTEDTANK);

	lua_regconstinteger(vm->L, EF_FAKEBMODEL);
	lua_regconstinteger(vm->L, EF_PATH_LINK);
	lua_regconstinteger(vm->L, EF_ZOOMING);
	lua_regconstinteger(vm->L, EF_PRONE);

	lua_regconstinteger(vm->L, EF_PRONE_MOVING);
	lua_regconstinteger(vm->L, EF_VIEWING_CAMERA);
	lua_regconstinteger(vm->L, EF_AAGUN_ACTIVE);
	lua_regconstinteger(vm->L, EF_SPARE0);

	lua_regconstinteger(vm->L, EF_SPARE1);
	lua_regconstinteger(vm->L, EF_SPARE2);
	lua_regconstinteger(vm->L, EF_BOUNCE);
	lua_regconstinteger(vm->L, EF_BOUNCE_HALF);
	lua_regconstinteger(vm->L, EF_MOVER_STOP);
	lua_regconstinteger(vm->L, EF_MOVER_BLOCKED);
}

static void registerPowerupConstants(lua_vm_t *vm)
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

#ifdef FEATURE_MULTIVIEW
	lua_regconstinteger(vm->L, PW_MVCLIENTLIST);
#endif

	lua_regconstinteger(vm->L, PW_NUM_POWERUPS);
}

static void registerWeaponConstants(lua_vm_t *vm)
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

static void registerModConstants(lua_vm_t *vm)
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

static void registerSurfaceConstants(lua_vm_t *vm)
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

static void registerConstants(lua_vm_t *vm)
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

	// cs, weapon and MOD constants
	registerConfigstringConstants(vm);
	registerEntityFlags(vm);
	registerPowerupConstants(vm);
	registerWeaponConstants(vm);
	registerModConstants(vm);
	registerSurfaceConstants(vm);
}

/*
 * G_LuaStartVM( vm )
 * Starts one individual virtual machine.
 */
qboolean G_LuaStartVM(lua_vm_t *vm)
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
	registerConstants(vm);

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
	if (!G_LuaCall(vm, "G_LuaStartVM", 0, 0))
	{
		G_Printf("%s API: %sLua VM start failed ( %s )\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);
		return qfalse;
	}

	// Load the code
	G_Printf("%s API: %sfile '%s' loaded into Lua VM\n", LUA_VERSION, S_COLOR_BLUE, vm->file_name);

	return qtrue;
}

/*
 * G_LuaStopVM( vm )
 * Stops one virtual machine, and calls its et_Quit callback.
 */
void G_LuaStopVM(lua_vm_t *vm)
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
		if (G_LuaGetNamedFunction(vm, "et_Quit"))
		{
			G_LuaCall(vm, "et_Quit", 0, 0);
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

/*
 * G_LuaShutdown()
 * Shuts down everything related to Lua API.
 */
void G_LuaShutdown(void)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			G_LuaStopVM(vm);
		}
	}
}

/*
 * G_LuaRestart()
 * Restart Lua API
 */
void G_LuaRestart(void)
{
	G_LuaShutdown();
	G_LuaInit();
}

/*
 * G_LuaStatus( ent )
 * Prints information on the Lua virtual machines.
 */
void G_LuaStatus(gentity_t *ent)
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
		G_refPrintf(ent, "%s API: %sno scripts loaded.", LUA_VERSION, S_COLOR_BLUE);
		return;
	}
	else if (cnt == 1)
	{
		G_refPrintf(ent, "%s API: %sshowing lua information ( 1 module loaded )", LUA_VERSION, S_COLOR_BLUE);
	}
	else
	{
		G_refPrintf(ent, "%s API: %sshowing lua information ( %d modules loaded )", LUA_VERSION, S_COLOR_BLUE, cnt);
	}
	G_refPrintf(ent, "%-2s %-24s %-40s %-24s", "VM", "Modname", "Signature", "Filename");
	G_refPrintf(ent, "-- ------------------------ ---------------------------------------- ------------------------");
	for (i = 0; i < LUA_NUM_VM; i++)
	{
		if (lVM[i])
		{
			G_refPrintf(ent, "%2d %-24s %-40s %-24s", lVM[i]->id, lVM[i]->mod_name, lVM[i]->mod_signature, lVM[i]->file_name);
		}
	}
	G_refPrintf(ent, "-- ------------------------ ---------------------------------------- ------------------------");
}

/*
 * G_LuaGetVM
 * Retrieves the VM for a given lua_State
 */
lua_vm_t *G_LuaGetVM(lua_State *L)
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
 * Called when another module sends an `et.IPCSend()` message to this module.
 *
 * IMPORTANT: The sender module must be loaded earlier in the `lua_modules` cvar, otherwise the receiver module cannot find it.
 *
 * @lua_def_prototype et_IPCReceive(vmnumber, message)
 * @lua_def ---@param vmnumber number the VM slot number of the sender.
 * @lua_def ---@param message string the message sent.
 */
void G_LuaHook_IPCReceive(int vmnumber, const char *message)
{
	// XXX - implemented by mod, just for documentation purposes ...
}

/**
 * Called when qagame initializes.
 *
 * @lua_def_prototype et_InitGame(levelTime, randomSeed, restart)
 * @lua_def ---@param levelTime number the current level time in milliseconds.
 * @lua_def ---@param randomSeed number a number that can be used to seed random number generators.
 * @lua_def ---@param restart number indicates if et_InitGame() is being called due to a `map_restart` (1) or not (0).
 */
void G_LuaHook_InitGame(int levelTime, int randomSeed, int restart)
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
			if (!G_LuaGetNamedFunction(vm, "et_InitGame"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, levelTime);
			lua_pushinteger(vm->L, randomSeed);
			lua_pushinteger(vm->L, restart);
			// Call
			if (!G_LuaCall(vm, "et_InitGame", 3, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called when qagame shuts down.
 *
 * @lua_def_prototype et_ShutdownGame(restart)
 * @lua_def ---@param restart number indicates if the shutdown is being called due to a `map_restart` (1) or not (0).
 */
void G_LuaHook_ShutdownGame(int restart)
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
			if (!G_LuaGetNamedFunction(vm, "et_ShutdownGame"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, restart);
			// Call
			if (!G_LuaCall(vm, "et_ShutdownGame", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/*
 * G_LuaHook_RunFrame
 * et_RunFrame( levelTime )  callback
 */

/**
 * A Hook that is run every server frame.
 *
 * @lua_def_prototype et_RunFrame(levelTime)
 * @lua_def ---@param levelTime number Level time of the server.
 */
void G_LuaHook_RunFrame(int levelTime)
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
			if (!G_LuaGetNamedFunction(vm, "et_RunFrame"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, levelTime);
			// Call
			if (!G_LuaCall(vm, "et_RunFrame", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called when a client attempts to connect to the server.
 *
 * @lua_def_prototype et_ClientConnect(clientNum, firstTime, isBot)
 * @lua_def ---@param clientNum number the client slot id.
 * @lua_def ---@param firstTime number indicates if this is a new connection (1) or a reconnection (0).
 * @lua_def ---@param isBot number indicates if the client is a bot (1) or not (0).
 * @lua_def ---@return string|nil result 'nil' if mod accepts the connection, otherwise a string describing the reason the client connection was rejected
 */
qboolean G_LuaHook_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot, char *reason)
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
			if (!G_LuaGetNamedFunction(vm, "et_ClientConnect"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushinteger(vm->L, (int)firstTime);
			lua_pushinteger(vm->L, (int)isBot);
			// Call
			if (!G_LuaCall(vm, "et_ClientConnect", 3, 1))
			{
				//G_LuaStopVM(vm);
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
 * Called when a client disconnects.
 *
 * @lua_def_prototype et_ClientDisconnect(clientNum)
 * @lua_def ---@param clientNum number the client slot id.
 */
void G_LuaHook_ClientDisconnect(int clientNum)
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
			if (!G_LuaGetNamedFunction(vm, "et_ClientDisconnect"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!G_LuaCall(vm, "et_ClientDisconnect", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called when a client begins (becomes active, and enters the gameworld).
 *
 * @lua_def_prototype et_ClientBegin(clientNum)
 * @lua_def ---@param clientNum number the client slot id.
 */
void G_LuaHook_ClientBegin(int clientNum)
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
			if (!G_LuaGetNamedFunction(vm, "et_ClientBegin"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!G_LuaCall(vm, "et_ClientBegin", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called when a client's Userinfo string has changed.
 *
 * NOTE: This only gets called when the players `CS_PLAYERS` config string changes, rather than every time the userinfo changes. This only happens for a subset of userinfo fields.
 *
 * @lua_def_prototype et_ClientUserinfoChanged(clientNum)
 * @lua_def ---@param clientNum number the client slot id.
 */
void G_LuaHook_ClientUserinfoChanged(int clientNum)
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
			if (!G_LuaGetNamedFunction(vm, "et_ClientUserinfoChanged"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!G_LuaCall(vm, "et_ClientUserinfoChanged", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called when a client is spawned.
 *
 * @lua_def_prototype et_ClientSpawn(clientNum, revived, teamChange, restoreHealth)
 * @lua_def ---@param clientNum number the client slot id.
 * @lua_def ---@param revived number indicates if the client was spawned by being revived (1) or not (0).
 * @lua_def ---@param teamChange number indicates if the client changed team (1) or not (0).
 * @lua_def ---@param restoreHealth number indicates if the player health bar is fully restored (1) or not (0).
 */
void G_LuaHook_ClientSpawn(int clientNum, qboolean revived, qboolean teamChange, qboolean restoreHealth)
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
			if (!G_LuaGetNamedFunction(vm, "et_ClientSpawn"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushinteger(vm->L, (int)revived);
			lua_pushinteger(vm->L, (int)teamChange);
			lua_pushinteger(vm->L, (int)restoreHealth);
			// Call
			if (!G_LuaCall(vm, "et_ClientSpawn", 4, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called when a command is received from a client.
 *
 * TIP: The actual command can be accessed through the argument handling functions.
 *
 * @lua_def_prototype et_ClientCommand(clientNum, command)
 * @lua_def ---@param clientNum number the client slot id.
 * @lua_def ---@param command string the command.
 * @lua_def ---@return number result '1' if the command was intercepted by the mod, and '0' if the command was ignored and passed through to the server (and other mods in the chain).
 */
qboolean G_LuaHook_ClientCommand(int clientNum, char *command)
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
			if (!G_LuaGetNamedFunction(vm, "et_ClientCommand"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushstring(vm->L, command);
			// Call
			if (!G_LuaCall(vm, "et_ClientCommand", 2, 1))
			{
				//G_LuaStopVM(vm);
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
 * Called when a command is entered on the server console.
 *
 * TIP: The actual command can be accessed through the argument handling functions.
 *
 * @lua_def_prototype et_ConsoleCommand(command)
 * @lua_def ---@param command string
 * @lua_def ---@return number result returns 1 if the command was intercepted, and 0 if the command was ignored and passed through to the server (and other mods in the chain).
 */
qboolean G_LuaHook_ConsoleCommand(char *command)
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
			if (!G_LuaGetNamedFunction(vm, "et_ConsoleCommand"))
			{
				continue;
			}
			// Arguments
			lua_pushstring(vm->L, command);
			// Call
			if (!G_LuaCall(vm, "et_ConsoleCommand", 1, 1))
			{
				//G_LuaStopVM(vm);
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
 * Called when a client gets a skill upgrade.
 *
 * @lua_def_prototype et_UpgradeSkill(clientNum, skill)
 * @lua_def ---@param clientNum number the client slot.
 * @lua_def ---@param skill number the skill number (see `et.SK_`).
 * @lua_def ---@return number result returns -1 to override (abort) the qagame function, anything else to "passthrough". Callback may modify skills (or do anything else it wants) during passthrough.
 */
qboolean G_LuaHook_UpgradeSkill(int cno, skillType_t skill)
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
			if (!G_LuaGetNamedFunction(vm, "et_UpgradeSkill"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, cno);
			lua_pushinteger(vm->L, (int)skill);
			// Call
			if (!G_LuaCall(vm, "et_UpgradeSkill", 2, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isnumber(vm->L, -1))
			{
				if (lua_tointeger(vm->L, -1) == -1)
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
 * Called when a client skill is set.
 *
 * @lua_def_prototype et_SetPlayerSkill(clientNum, skill)
 * @lua_def ---@param clientNum number the client slot.
 * @lua_def ---@param skill number the skill number (see `et.SK_`).
 * @lua_def ---@return number result returns -1 to override (abort) the qagame function, anything else to "passthrough". Callback may modify skills (or do anything else it wants) during passthrough.
 */
qboolean G_LuaHook_SetPlayerSkill(int cno, skillType_t skill)
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
			if (!G_LuaGetNamedFunction(vm, "et_SetPlayerSkill"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, cno);
			lua_pushinteger(vm->L, (int)skill);
			// Call
			if (!G_LuaCall(vm, "et_SetPlayerSkill", 2, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isnumber(vm->L, -1))
			{
				if (lua_tointeger(vm->L, -1) == -1)
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

static luaPrintFunctions_t g_luaPrintFunctions[] =
{
	{ GPRINT_TEXT,      "et_Print"  },
	{ GPRINT_DEVELOPER, "et_DPrint" },
	{ GPRINT_ERROR,     "et_Error"  }
};

/**
 * Called whenever the server or qagame prints a string to the console.
 *
 * WARNING: **DO NOT TRUST STRINGS OBTAINED IN THIS WAY!**
 * WARNING: Text may contain a player name and their chat message, which makes it very easy to spoof.
 *
 * @lua_def_prototype et_Print(text)
 * @lua_def ---@param text string
 */
void G_LuaHook_Print(printMessageType_t category, char *text)
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
			if (!G_LuaGetNamedFunction(vm, g_luaPrintFunctions[category].function))
			{
				continue;
			}
			// Arguments
			lua_pushstring(vm->L, text);
			// Call
			if (!G_LuaCall(vm, g_luaPrintFunctions[category].function, 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/**
 * Called whenever a player is killed.
 *
 * Different to ETPub which supports custom obituaries
 * this is 'ETPro like' implementation.
 *
 * @lua_def_prototype et_Obituary(target, attacker, meansOfDeath)
 * @lua_def ---@param target number the victim.
 * @lua_def ---@param attacker number the killer.
 * @lua_def ---@param meansOfDeath number the means of death (see `et.MOD_`).
 */
qboolean G_LuaHook_Obituary(int victim, int killer, int meansOfDeath)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_Obituary"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, victim);
			lua_pushinteger(vm->L, killer);
			lua_pushinteger(vm->L, meansOfDeath);

			// Call
			if (!G_LuaCall(vm, "et_Obituary", 3, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isstring(vm->L, -1))
			{
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * Called whenever a player gets revived.
 *
 * @lua_def_prototype et_Revive(revivee, reviver, invulnEndTime)
 * @lua_def ---@param revivee number the one who got revived
 * @lua_def ---@param reviver number the one who did the reviving
 * @lua_def ---@param invulnEndTime number timestamp when the spawn shield/invulnerability ends
 */
qboolean G_LuaHook_Revive(int revivee, int reviver, int invulnEndTime)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_Revive"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, revivee);
			lua_pushinteger(vm->L, reviver);
			lua_pushinteger(vm->L, invulnEndTime);
			// Call
			if (!G_LuaCall(vm, "et_Revive", 3, 1))
			{
				continue;
			}
			// Return values
			if (lua_tointeger(vm->L, -1) == 1)
			{
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * Called whenever a player gets damage.
 *
 * @lua_def_prototype et_Damage(target, attacker, damage, damageFlags, meansOfDeath)
 * @lua_def ---@param target number the victim.
 * @lua_def ---@param attacker number the killer.
 * @lua_def ---@param damage number the amount of damage.
 * @lua_def ---@param damageFlags number
 * @lua_def ---@param meansOfDeath number the means of death. See `et.MOD_*` for possible values.
 */
qboolean G_LuaHook_Damage(int target, int attacker, int damage, int dflags, meansOfDeath_t mod)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_Damage"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, target);
			lua_pushinteger(vm->L, attacker);
			lua_pushinteger(vm->L, damage);
			lua_pushinteger(vm->L, dflags);
			lua_pushinteger(vm->L, mod);
			// Call
			if (!G_LuaCall(vm, "et_Damage", 5, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_tointeger(vm->L, -1) == 1)
			{
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * Called whenever a weapon is shot.
 *
 * @lua_def_prototype et_WeaponFire(clientNum, weapon)
 * @lua_def ---@param clientNum number the client slot.
 * @lua_def ---@param weapon number the weapon shot (see `et.WP_`).
 * @lua_def ---@return number result returns 1 to override (abort) the qagame function, 0 to "passthrough". Callback may do anything it wants during passthrough.
 */
qboolean G_LuaHook_WeaponFire(int clientNum, weapon_t weapon, gentity_t **pFiredShot)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_WeaponFire"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushinteger(vm->L, weapon);
			// Call
			if (!G_LuaCall(vm, "et_WeaponFire", 2, 2))
			{
				continue;
			}
			// Return values
			if (lua_tointeger(vm->L, -2) == 1)
			{
				if (lua_isinteger(vm->L, -1))
				{
					int entNum = lua_tointeger(vm->L, -1);
					if (entNum >= 0 && entNum < MAX_GENTITIES)
					{
						*pFiredShot = g_entities + entNum;
					}
				}
				lua_pop(vm->L, 2);
				return qtrue;
			}
			lua_pop(vm->L, 2);
		}
	}
	return qfalse;
}

/**
 * Called whenever a fixed machine gun is shot.
 *
 * @lua_def_prototype et_FixedMGFire(clientNum)
 * @lua_def ---@param clientNum number the client slot.
 * @lua_def ---@return number result returns 1 to override (abort) the qagame function, 0 to "passthrough". Callback may do anything it wants during passthrough.
 */
qboolean G_LuaHook_FixedMGFire(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_FixedMGFire"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!G_LuaCall(vm, "et_FixedMGFire", 1, 1))
			{
				continue;
			}
			// Return values
			if (lua_tointeger(vm->L, -1) == 1)
			{
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * Called whenever a mounted machine gun is shot.
 *
 * @lua_def_prototype et_MountedMGFire(clientNum)
 * @lua_def ---@param clientNum number the client slot.
 * @lua_def ---@return number result returns 1 to override (abort) the qagame function, 0 to "passthrough". Callback may do anything it wants during passthrough.
 */
qboolean G_LuaHook_MountedMGFire(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_MountedMGFire"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!G_LuaCall(vm, "et_MountedMGFire", 1, 1))
			{
				continue;
			}
			// Return values
			if (lua_tointeger(vm->L, -1) == 1)
			{
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * Called whenever an anti-aircraft gun is shot.
 *
 * Returns '1' to override (abort) the qagame function, '0' to "passthrough". Callback may do anything it wants during passthrough.
 *
 * @lua_def_prototype et_AAGunFire(clientNum)
 * @lua_def ---@param clientNum number the client slot.
 * @lua_def ---@return number result returns '1' to override (abort) the qagame function, '0' to "passthrough". Callback may do anything it wants during passthrough.
 */
qboolean G_LuaHook_AAGunFire(int clientNum)
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_AAGunFire"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if (!G_LuaCall(vm, "et_AAGunFire", 1, 1))
			{
				continue;
			}
			// Return values
			if (lua_tointeger(vm->L, -1) == 1)
			{
				lua_pop(vm->L, 1);
				return qtrue;
			}
			lua_pop(vm->L, 1);
		}
	}
	return qfalse;
}

/**
 * Called when an entity definition is parsed to spawn gentities.
 *
 * @lua_def_prototype et_SpawnEntitiesFromString()
 */
void G_LuaHook_SpawnEntitiesFromString()
{
	int      i;
	lua_vm_t *vm;

	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0 /*|| vm->err*/)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_SpawnEntitiesFromString"))
			{
				continue;
			}

			// Call
			if (!G_LuaCall(vm, "et_SpawnEntitiesFromString", 0, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
		}
	}
}

/**
 * Called whenever a player says/chats something.
 *
 * Return '1' as 1st return value to override the message with a string passed
 * in 2nd return value.
 *
 * Otherwise passes the message through.
 *
 * @lua_def_prototype et_Chat(sender, receiver, message)
 * @lua_def ---@param sender number
 * @lua_def ---@param receiver number
 * @lua_def ---@param message string
 * @lua_def ---@return number override if set to '1' override the message with 'overrideMessage'
 * @lua_def ---@return string overrideMessage
 */
const char *G_LuaHook_Chat(int sender, int receiver, const char *message, char *buffer, size_t bufsize)
{
	int        i;
	lua_vm_t   *vm;
	const char *result, *newMessage;

	newMessage = message;
	for (i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if (vm)
		{
			if (vm->id < 0) //|| vm->err)
			{
				continue;
			}
			if (!G_LuaGetNamedFunction(vm, "et_Chat"))
			{
				continue;
			}
			// Arguments
			lua_pushinteger(vm->L, sender);
			lua_pushinteger(vm->L, receiver);
			lua_pushstring(vm->L, newMessage);
			// Call
			if (!G_LuaCall(vm, "et_Chat", 3, 2))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if (lua_isinteger(vm->L, -2) &&
			    lua_tointeger(vm->L, -2) != qfalse &&
			    lua_isstring(vm->L, -1))
			{
				result = luaL_checkstring(vm->L, -1);
				Q_strncpyz(buffer, result, bufsize);
				newMessage = buffer;
			}
			lua_pop(vm->L, 2);
		}
	}
	return newMessage;
}

/** @} */ // doxygen addtogroup lua_etevents

void Svcmd_LoadLua_f(void)
{
	char buffer[MAX_QPATH];
	int  i = 1;
	if (trap_Argc() < 2)
	{
		return;
	}

	for (; i < trap_Argc(); i++)
	{
		buffer[0] = '\0';
		trap_Argv(i, buffer, sizeof(buffer));
		G_LuaRunIsolated(buffer);
	}
}

#endif // FEATURE_LUA
