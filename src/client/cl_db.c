/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2019 ET:Legacy team <mail@etlegacy.com>
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
 * @file cl_db.c
 * @brief client database functions
 */

#ifdef FEATURE_DBMS

#include "client.h"

#include "../db/db_sql.h"

/**
 * @brief Inserts given server into client_servers table if not already exists
 * @param[in] profile
 * @param[in] source
 * @param[in] name
 * @param[in] address
 * @param[in] mod
 */
void DB_InsertFavorite(const char *profile, int source, const char *name, const char *address, const char *mod)
{
	char         *sql;
	int          result;
	sqlite3_stmt *res;

	if (!profile[0]) // no profile given
	{
		Com_Printf("DB_InsertFavorite warning: Invalid profile.\n");
		return;
	}

	if (!address[0]) // no profile given
	{
		Com_Printf("DB_InsertFavorite warning: Invalid address.\n");
		return;
	}

	if (!isDBActive)
	{
		Com_Printf("DB_InsertFavorite warning: DB not active error\n");
		return;
	}

	if (cls.numfavoriteservers >= MAX_FAVOURITE_SERVERS - 1)
	{
		Com_Printf("DB_InsertFavorite warning: Can't insert. MAX_FAVOURITE_SERVERS reached.\n");
		return;
	}

	sql    = va("SELECT * from client_servers WHERE profile='%s' AND address='%s';", profile, address);
	result = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	// FIXME: name - use prepared statement

	if (result == SQLITE_OK)
	{
		// FIXME: check if client is connected and don't use name?! (not really required)
		// ingame adding of favorites won't pass correct hostname (address instead)
		// this is a bug in ET ui code

		result = sqlite3_step(res);

		if (result == SQLITE_ROW)
		{
			//Com_Printf("Favorite already stored\n");
			sqlite3_finalize(res);
			return;
		}
		else if(result == SQLITE_ERROR)
		{
			Com_Printf("SQL an error occured\n");
		}
		else
		{
			char *err_msg = 0;

			// FIXME: name - use prepared statement
			//sqlite3_bind_int(res, 1, 3);
			sql = va("INSERT INTO client_servers values('%s', %i, '%s', '%s', '%s', NULL, (datetime('now','localtime')))", profile, source, address, name, mod);
			result = sqlite3_exec(db, sql, 0, 0, &err_msg);

			if (result != SQLITE_OK)
			{
				Com_Printf("SQL command '%s' failed: %s\n", sql, err_msg);
				sqlite3_free(err_msg);
			}
			else
			{
				Com_Printf("Favorite '%s' for profile '%s' created.\n", address, profile);
				sqlite3_free(err_msg);
				sqlite3_finalize(res);
				return;
			}
		}
	}

	sqlite3_finalize(res);
	Com_Printf("Can't save favorite - db error\n");
}

/**
 * @brief
 * @param[in] profile
 * @param[in] address
 */
void DB_DeleteFavorite(const char *profile, const char *address)
{
	char         *sql;
	int          result;
	char         *err_msg = 0;
	//sqlite3_stmt *res;

	if (!isDBActive)
	{
		Com_Printf("DB_DeleteFavorite warning: DB not active error\n");
		return;
	}

	if (!profile[0]) // no profile given
	{
		Com_Printf("DB_DeleteFavorite warning: Invalid profile.\n");
		return;
	}

	if (address[0] == '*')
	{
		sql = va("DELETE FROM client_servers WHERE profile='%s';", profile);
	}
	else
	{
		sql = va("DELETE FROM client_servers WHERE profile='%s' AND address='%s';", profile, address);
	}

	result = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("SQL command '%s' failed: %s\n", sql, err_msg);
	}

	sqlite3_free(err_msg);

	if (address[0] == '*')
	{
		Com_Printf("All favorites for profile '%s' have been deleted.\n", profile);
	}
	else
	{
		Com_Printf("Favorite '%s' for profile '%s' has been deleted.\n", address, profile);
	}

	//sqlite3_finalize(res);
}

/**
 * @brief DB_callbackFavorites
 * @param[in] NotUsed
 * @param[in] argc
 * @param[in] argv
 * @param[in] azColName
 * @return 
 */
static int DB_callbackFavorites(UNUSED_VAR void *NotUsed, int argc, char **argv, char **azColName)
{
	netadr_t addr;

    // avoid array overflows
	if (cls.numfavoriteservers >= MAX_FAVOURITE_SERVERS - 1)
	{
		Com_Printf("Can't load all favorites. MAX_FAVOURITE_SERVERS reached.\n");
		return 0;
	}

	NET_StringToAdr(argv[2], &addr, NA_UNSPEC);
	CL_InitServerInfo(&cls.favoriteServers[cls.numfavoriteservers], &addr);
    Q_strncpyz(cls.favoriteServers[cls.numfavoriteservers].hostName, argv[3], MAX_SERVER_NAME_LENGTH);

    cls.favoriteServers[cls.numfavoriteservers].visible = qtrue;

    cls.numfavoriteservers++;

    return 0;
}

/**
 * @brief Loads favorites from db into favoriteServers list
 * @param[in]
 */
void DB_LoadFavorites(const char *profile)
{
	int          result;
	char         *sql;
	char         *err_msg = 0;

	// cls.numfavoriteservers = 0; already done before

	if (!isDBActive)
	{
		Com_Printf("DB_LoadFavorites warning: DB not active error\n");
		return;
	}

	if (!profile[0]) // no profile given
	{
		Com_Printf("DB_LoadFavorites warning: Invalid profile.\n");
		return;
	}

	sql = va("SELECT * FROM client_servers WHERE profile='%s';", profile);

	result = sqlite3_exec(db, sql, DB_callbackFavorites, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("Can't load favorites - db error %s\n", err_msg);
	}
	sqlite3_free(err_msg);

	Com_Printf("Total favorite servers restored: %i\n", cls.numfavoriteservers);
}

/**
 * @brief
 * @param(in]
 * @param(in]
 */
void DB_UpdateFavorite(const char *profile, const char *address)
{
	int          result;
	char         *sql;
	char         *err_msg = 0;
	sqlite3_stmt *res;

	if (!profile[0] || !address[0]) // nothing to do
	{
		return;
	}

	if (!isDBActive)
	{
		Com_Printf("DB_UpdateFavorite warning: DB not active error\n");
		return;
	}

	// get favorite
	sql    = va("SELECT * from client_servers WHERE profile='%s' AND address='%s';", profile, address);
	result = sqlite3_prepare_v2(db, sql, -1, &res, 0);
	// FIXME: name - use prepared statement

	if (result == SQLITE_OK) // we've found an entry
	{
		result = sqlite3_step(res);

		if (result == SQLITE_ROW)
		{
			Com_Printf("Favorite found.\n");

			// FIXME: prepared statement
			//sqlite3_bind_int(res, 1, 3);
			// FIXME: update name and mod if not set
			sql = va("UPDATE client_servers SET updated=(datetime('now','localtime')) WHERE profile='%s' AND address='%s';", profile, address);
			result = sqlite3_exec(db, sql, 0, 0, &err_msg);

			if (result != SQLITE_OK)
			{
				Com_Printf("Can't update favorite '%s' failed: %s\n", address, err_msg);
			}
			else
			{
				Com_Printf("Favorite '%s' for profile '%s' updated.\n", address, profile);
				sqlite3_finalize(res);
				sqlite3_free(err_msg);
				return;
			}
			
			sqlite3_free(err_msg);
		}
		else if(result == SQLITE_ERROR)
		{
			Com_Printf("SQL an error occured\n");
		}
		//else
		//{
		//	Com_Printf("No Favorite found\n");
		//}
	}
	else
	{
		Com_Printf("Can't update favorite '%s'\n", address);
	}

	sqlite3_finalize(res);
}

#endif
