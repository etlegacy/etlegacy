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

int rowCounter = 0;

/**
 * @brief Inserts given server into client_servers table if not already exists
 * @param[in] profile
 * @param[in] source
 * @param[in] name
 * @param[in] address
 * @param[in] mod
 */
void DB_insertFavorite(const char *profile, int source, const char *name, const char *address, const char *mod)
{
	char         *sql;
	int          result;
	char         *err_msg = 0;
	sqlite3_stmt *res;

	if (!isDBActive)
	{
		Com_Printf("insertFavoriteDB warning: DB not active error\n");
		return;
	}

	if (cls.numfavoriteservers >= MAX_FAVOURITE_SERVERS - 1)
	{
		Com_Printf("insertFavoriteDB warning: Can't insert. MAX_FAVOURITE_SERVERS reached\n");
		return;
	}

	//
	// FIXME: name (colors/inject.)
	//

	sql    = va("SELECT * from client_servers WHERE profile='%s' AND address='%s';", profile, address);
	result = sqlite3_prepare_v2(db, sql, -1, &res, 0);

	if (result != SQLITE_OK)
	{
		Com_Printf("Can't save favorite - db error\n"); // FIXME: sqlite3_errmsg(db) this is leaking?
		//sqlite3_free(err_msg);
	}
	else
	{
		result = sqlite3_step(res);

		if (result == SQLITE_ROW)
		{
			Com_Printf("Favorite already stored\n");
		}
		else if(result == SQLITE_ERROR)
		{
			Com_Printf("SQL an error occured\n");
		}
		else
		{
			sql = va("INSERT INTO client_servers values('%s', %i, '%s', '%s', '%s', NULL, CURRENT_TIMESTAMP)", profile, source, address, name, mod);
			result = sqlite3_exec(db, sql, 0, 0, &err_msg);

			if (result != SQLITE_OK)
			{
				Com_Printf("SQL command '%s' failed: %s\n", sql, err_msg);
				sqlite3_free(err_msg);
			}

			Com_Printf("^4Favorite '%s' for profile '%s' created\n", address, profile);
		}
	}
}

/**
 * @brief
 * @param[in] profile
 * @param[in] address
 */
void DB_deleteFavorite(const char *profile, const char *address)
{
	char         *sql;
	int          result;
	char         *err_msg = 0;
	sqlite3_stmt *res;

	if (!isDBActive)
	{
		Com_Printf("DB_deleteFavorite warning: DB not active error\n");
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
		sqlite3_free(err_msg);
	}

	if (address[0] == '*')
	{
		Com_Printf("All favorites for profile '%s' have been deleted.\n", profile);
	}
	else
	{
		Com_Printf("Favorite '%s' for profile '%s' has been deleted.\n", address, profile);
	}
}

/**
 * @brief Callback function for DB_loadFavorites
 * @param[out]
 * @param[out]
 * @param[out]
 * @param[out]
 */
int DB_callbackFavorites(void *NotUsed, int argc, char **argv, char **azColName)
{
	netadr_t addr;
	int      i, j;

    NotUsed = 0;

    // avoid array overflows
	if (rowCounter >= MAX_FAVOURITE_SERVERS - 1)
	{
		Com_Printf("Can't load all Favorites. MAX_FAVOURITE_SERVERS reached.\n");
		return 0;
	}

/*
    for (int i = 0; i < argc; i++)
    {

    	Com_Printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "*NULL*");
    }
*/
	NET_StringToAdr(argv[2], &addr, NA_UNSPEC);
	CL_InitServerInfo(&cls.favoriteServers[rowCounter], &addr);
    Q_strncpyz(cls.favoriteServers[rowCounter].hostName, argv[3], MAX_SERVER_NAME_LENGTH);

    cls.favoriteServers[rowCounter].visible = qtrue;

    rowCounter++;

    return 0;
}

/**
 * @brief Loads favorites from db into favoriteServers list
 */
void DB_loadFavorites(const char *profile)
{
	int          result;
	char         *sql;
	char         *err_msg = 0;
	sqlite3_stmt *res;

	if (!isDBActive)
	{
		Com_Printf("DB_loadFavorites warning: DB not active error\n");
	}

	sql = va("SELECT * FROM client_servers WHERE profile='%s';", profile);
    Com_Printf(va("%s",sql));
	rowCounter = 0;

	result = sqlite3_exec(db, sql, DB_callbackFavorites, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("Can't load favorites - db error %s\n", err_msg);
		sqlite3_free(err_msg);
	}

	cls.numfavoriteservers = rowCounter;

	Com_Printf("Total favorite servers restored: %i\n", cls.numfavoriteservers);
}

#endif
