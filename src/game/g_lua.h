/**
 * @file g_lua.h
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

#include "g_local.h"

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
//#define FIELD_WEAPONSTAT_EXT	9

#define FIELD_FLAG_GENTITY  1 ///< marks a gentity_s field
#define FIELD_FLAG_GCLIENT  2 ///< marks a gclient_s field
#define FIELD_FLAG_NOPTR    4
#define FIELD_FLAG_READONLY 8 ///< read-only access

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

// macros to add gentity and gclient fields
#define _et_gentity_addfield(n, t, f) { #n, t, offsetof(struct gentity_s, n), FIELD_FLAG_GENTITY + f }
#define _et_gentity_addfieldalias(n, a, t, f) { #n, t, offsetof(struct gentity_s, a), FIELD_FLAG_GENTITY + f }
#define _et_gclient_addfield(n, t, f) { #n, t, offsetof(struct gclient_s, n), FIELD_FLAG_GCLIENT + f }
#define _et_gclient_addfieldalias(n, a, t, f) { #n, t, offsetof(struct gclient_s, a), FIELD_FLAG_GCLIENT + f }

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
 * @struct gentity_field_s
 * @brief
 */
typedef struct
{
	const char *name;
	int type;
	unsigned long mapping;
	int flags;
} gentity_field_t;

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
qboolean G_LuaInit(void);
qboolean G_LuaCall(lua_vm_t *vm, const char *func, int nargs, int nresults);
qboolean G_LuaGetNamedFunction(lua_vm_t *vm, const char *name);
qboolean G_LuaStartVM(lua_vm_t *vm);
qboolean G_LuaRunIsolated(const char *modName);
void G_LuaStopVM(lua_vm_t *vm);
void G_LuaShutdown(void);
void G_LuaRestart(void);
void G_LuaStatus(gentity_t *ent);
void G_LuaStackDump();
lua_vm_t *G_LuaGetVM(lua_State *L);

// Callbacks
void G_LuaHook_InitGame(int levelTime, int randomSeed, int restart);
void G_LuaHook_ShutdownGame(int restart);
void G_LuaHook_RunFrame(int levelTime);
qboolean G_LuaHook_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot, char *reason);
void G_LuaHook_ClientDisconnect(int clientNum);
void G_LuaHook_ClientBegin(int clientNum);
void G_LuaHook_ClientUserinfoChanged(int clientNum);
void G_LuaHook_ClientSpawn(int clientNum, qboolean revived, qboolean teamChange, qboolean restoreHealth);
qboolean G_LuaHook_ClientCommand(int clientNum, char *command);
qboolean G_LuaHook_ConsoleCommand(char *command);
qboolean G_LuaHook_UpgradeSkill(int cno, skillType_t skill);
qboolean G_LuaHook_SetPlayerSkill(int cno, skillType_t skill);
void G_LuaHook_Print(printMessageType_t category, char *text);
qboolean G_LuaHook_Obituary(int victim, int killer, int meansOfDeath);
qboolean G_LuaHook_Damage(int target, int attacker, int damage, int dflags, meansOfDeath_t mod);
void G_LuaHook_SpawnEntitiesFromString();
qboolean G_ScriptAction_Delete(gentity_t *ent, char *params);
qboolean G_LuaHook_WeaponFire(int clientNum, weapon_t weapon, gentity_t **pFiredShot);
qboolean G_LuaHook_FixedMGFire(int clientNum);
qboolean G_LuaHook_MountedMGFire(int clientNum);
qboolean G_LuaHook_AAGunFire(int clientNum);

#endif // #ifndef INCLUDE_G_LUA_H

#endif // FEATURE_LUA
