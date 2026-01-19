/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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

#ifndef INCLUDE_LUAJIT_COMPAT_H
#define INCLUDE_LUAJIT_COMPAT_H

#ifdef BUNDLED_LUA
#    include "lua.h"
#    include "lauxlib.h"
#    include "lualib.h"
#else
#    include <lua.h>
#    include <lauxlib.h>
#    include <lualib.h>
#endif

// Minimal LuaJIT compat for Lua 5.2+
#ifndef lua_pushglobaltable
#define lua_pushglobaltable(L)  lua_pushvalue((L), LUA_GLOBALSINDEX)
#endif

#ifndef lua_rawlen
#define lua_rawlen(L, idx)      lua_objlen((L), (idx))
#endif

#ifndef LUA_OK
#define LUA_OK 0
#endif

#ifndef luaL_newlib
#define luaL_newlib(L, l)       (lua_newtable((L)), luaL_setfuncs((L), (l), 0))
#endif

#if !defined(luaL_setfuncs) && !defined(FEATURE_LUASQL)
static void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
{
	luaL_checkstack(L, nup, "too many upvalues");
	for (; l->name != NULL; l++)
	{
		int i;
		for (i = 0; i < nup; i++)
			lua_pushvalue(L, -nup);
		lua_pushcclosure(L, l->func, nup);
		lua_setfield(L, -(nup + 2), l->name);
	}
	lua_pop(L, nup);
}
#endif

#ifndef luaL_getsubtable
/* Returns 1 if existing, 0 if created; leaves the table on top. */
static inline int luaL_getsubtable(lua_State *L, int idx, const char *fname)
{
	lua_getfield(L, idx, fname);         /* t[k] */
	if (lua_istable(L, -1))
	{
		return 1;                        /* found */
	}
	lua_pop(L, 1);
	lua_newtable(L);                      /* create */
	lua_pushvalue(L, -1);
	lua_setfield(L, idx, fname);          /* t[k] = new */
	return 0;                             /* created */
}
#endif

#ifndef lua_isinteger
static inline int lua_isinteger(lua_State *L, int idx)
{
	if (!lua_isnumber(L, idx))
	{
		return 0;
	}
	{
		lua_Number  n = lua_tonumber(L, idx);
		lua_Integer i = (lua_Integer)n;
		return (lua_Number)i == n;
	}
}
#endif

#endif // #ifndef INCLUDE_LUAJIT_COMPAT_H
