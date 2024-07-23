/**
 * @file tvg_lua.h
 * @brief ET <-> *Lua* interface header file.
 *
 * @copyright This code is based on ETPub implementation.
 * All credits go to their team especially to quad and pheno!
 * http://etpub.org
 *
 * @defgroup lua_api Lua API
 * @{
 * Most exported variables and function can be accessed from the *et* namespace.
 *
 * @defgroup lua_etvars    Lua variables
 * @defgroup lua_etfncs    Lua functions
 * @defgroup lua_etevents  Lua callbacks
 * @}
 */
#ifdef FEATURE_LUA

#ifndef INCLUDE_G_LUA_H
#define INCLUDE_G_LUA_H

#include "tvg_local.h"

#ifdef BUNDLED_LUA
#    include "lua.h"
#    include "lauxlib.h"
#    include "lualib.h"
#else
#    include <lua.h>
#    include <lauxlib.h>
#    include <lualib.h>
#endif

#define LUA_NUM_VM 18
#define LUA_MAX_FSIZE 1024 * 1024 ///< 1MB

#define FIELD_INT           0
#define FIELD_STRING        1
#define FIELD_FLOAT         2
#define FIELD_ENTITY        3
#define FIELD_VEC3          4
#define FIELD_INT_ARRAY     5
#define FIELD_TRAJECTORY    6
#define FIELD_FLOAT_ARRAY   7
#define FIELD_WEAPONSTAT    8
#define FIELD_USERCMD       9
#define FIELD_TVCMDS        10

#define FIELD_FLAG_GENTITY  1  ///< marks a gentity_s field
#define FIELD_FLAG_GCLIENT  2  ///< marks a gclient_s field
#define FIELD_FLAG_NOPTR    4
#define FIELD_FLAG_READONLY 8  ///< read-only access
#define FIELD_FLAG_LEVEL    16 ///< marks a level_locals_t field

// define HOSTARCH and EXTENSION depending on host architecture
#if defined WIN32
  #define HOSTARCH    "WIN32"
  #define EXTENSION   "dll"
#elif defined __APPLE__
  #define HOSTARCH    "MACOS"
  #define EXTENSION   "so"
#else
  #define HOSTARCH    "UNIX"
  #define EXTENSION   "so"
#endif

// macros to register predefined constants
#define lua_registerglobal(L, n, v) (lua_pushstring(L, v), lua_setglobal(L, n))
#define lua_regconstinteger(L, n) (lua_pushstring(L, #n), lua_pushinteger(L, n), lua_settable(L, -3))
//#define lua_regconstinteger(L, n) (lua_pushinteger(L, n), lua_setfield(L, -2, #n))
#define lua_regconststring(L, n) (lua_pushstring(L, #n), lua_pushstring(L, n), lua_settable(L, -3))
//#define lua_regconststring(L, n) (lua_pushstring(L, n), lua_setfield(L, -2, #n))

// macros to add gentity, gclient and level fields
#define _et_gentity_addfield(n, t, f) { #n, t, offsetof(struct gentity_s, n), FIELD_FLAG_GENTITY + (f) }
#define _et_gentity_addfieldalias(n, a, t, f) { #n, t, offsetof(struct gentity_s, a), FIELD_FLAG_GENTITY + (f) }
#define _et_gclient_addfield(n, t, f) { #n, t, offsetof(struct gclient_s, n), FIELD_FLAG_GCLIENT + (f) }
#define _et_gclient_addfieldalias(n, a, t, f) { #n, t, offsetof(struct gclient_s, a), FIELD_FLAG_GCLIENT + (f) }
#define _et_level_addfield(n, t, f) { #n, t, offsetof(struct level_locals_s, n), FIELD_FLAG_LEVEL + (f) }
#define _et_level_addfieldalias(n, a, t, f) { #n, t, offsetof(struct level_locals_s, a), FIELD_FLAG_LEVEL + (f) }

/**
 * @struct lua_vm_s
 * @brief
 */
typedef struct
{
	int id;
	char file_name[MAX_QPATH];
	char mod_name[MAX_CVAR_VALUE_STRING];
	char mod_signature[41];
	char *code;
	int code_size;
	int err;
	lua_State *L;
} lua_vm_t;

/**
 * @struct tvgame_field_s
 * @brief
 */
typedef struct tvgame_field_s
{
	const char *name;
	int type;
	uintptr_t mapping;
	int flags;
} tvgame_field_t;

extern lua_vm_t *lVM[LUA_NUM_VM];

/**
 * @enum printMessageType_e
 * @typedef printMessageType_t
 * @brief
 */
typedef enum printMessageType_e
{
	GPRINT_TEXT = 0,
	GPRINT_DEVELOPER,
	GPRINT_ERROR
} printMessageType_t;

/**
 * @struct luaPrintFunctions_s
 * @typedef luaPrintFunctions_t
 * @brief
 */
typedef struct luaPrintFunctions_s
{
	printMessageType_t category;
	const char *function;
} luaPrintFunctions_t;

// API
qboolean TVG_LuaInit(void);
qboolean TVG_LuaCall(lua_vm_t *vm, const char *func, int nargs, int nresults);
qboolean TVG_LuaGetNamedFunction(lua_vm_t *vm, const char *name);
qboolean TVG_LuaStartVM(lua_vm_t *vm);
qboolean TVG_LuaRunIsolated(const char *modName);
void TVG_LuaStopVM(lua_vm_t *vm);
void TVG_LuaShutdown(void);
void TVG_LuaRestart(void);
void TVG_LuaStatus(gclient_t *client);
void TVG_LuaStackDump();
lua_vm_t *TVG_LuaGetVM(lua_State *L);

// Callbacks
void TVG_LuaHook_InitGame(int levelTime, int randomSeed, int restart);
void TVG_LuaHook_ShutdownGame(int restart);
void TVG_LuaHook_RunFrame(int levelTime);
qboolean TVG_LuaHook_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot, char *reason);
void TVG_LuaHook_ClientDisconnect(int clientNum);
void TVG_LuaHook_ClientBegin(int clientNum);
void TVG_LuaHook_ClientUserinfoChanged(int clientNum);
void TVG_LuaHook_ClientSpawn(int clientNum);
qboolean TVG_LuaHook_ClientCommand(int clientNum, char *command);
qboolean TVG_LuaHook_ConsoleCommand(char *command);
void TVG_LuaHook_Print(printMessageType_t category, char *text);

#endif // #ifndef INCLUDE_G_LUA_H

#endif // FEATURE_LUA
