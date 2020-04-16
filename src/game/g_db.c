/*
 * ET: Legacy
 * Copyright (C) 2012-2020 ET:Legacy team <mail@etlegacy.com>
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
 */
/**
 * @file g_db.c
 * @brief Database initialization functions
 */

#ifdef FEATURE_DBMS
#include "g_local.h"
#include <sqlite3.h>

/**
 * @brief G_DB_Init
 * @return 0 if database is successfully initialized, 1 otherwise.
 */
int G_DB_Init()
{
	int db_mode;
	int result;

	if (level.database.initialized)
	{
		G_Printf("G_DB_Init: attempt to initialize already initialized database\n");
		return 1;
	}

	db_mode = trap_Cvar_VariableIntegerValue("db_mode");

	// check if engine db is enabled
	if (db_mode == 0)
	{
		G_Printf("... DBMS is disabled\n");
		return 1;
	}

	// check db mode
	if (db_mode == 1)
	{
		char *db_uri = "file::memory:?mode=memory&cache=shared";
		Q_strncpyz(level.database.path, db_uri, MAX_OSPATH);
	}
	else // db_mode == 2
	{
		char       homepath[MAX_OSPATH];
		char       db_uri[MAX_OSPATH];
		const char *db_path;

		trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof(homepath));
		trap_Cvar_VariableStringBuffer("db_uri", db_uri, sizeof(db_uri));

		db_path = va("%s/%s", homepath, db_uri);

		Q_strncpyz(level.database.path, db_path, MAX_OSPATH);
	}

	// db sanity check
#ifdef FEATURE_RATING
	if (G_SkillRatingDBCheck(level.database.path, db_mode))
	{
		return 1;
	}
#endif

#ifdef FEATURE_PRESTIGE
	if (G_PrestigeDBCheck(level.database.path, db_mode))
	{
		return 1;
	}
#endif
	if (G_XPSaver_CheckDB(level.database.path, db_mode))
	{
		return 1;
	}
	// open db
	if (db_mode == 1)
	{
		result = sqlite3_open_v2(level.database.path, &level.database.db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_MEMORY | SQLITE_OPEN_SHAREDCACHE, NULL);

		if (result != SQLITE_OK)
		{
			G_Printf("G_DB_Init: sqlite3_open_v2 failed: %s\n", sqlite3_errstr(result));
			return 1;
		}

		result = sqlite3_enable_shared_cache(1);

		if (result != SQLITE_OK)
		{
			G_Printf("G_DB_Init: sqlite3_enable_shared_cache failed: %s\n", sqlite3_errstr(result));
			(void) sqlite3_close(level.database.db);
			return 1;
		}
	}
	else // db_mode == 2
	{
		char         *err_msg = NULL;
		sqlite3_stmt *sqlstmt;

		result = sqlite3_open_v2(level.database.path, &level.database.db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,NULL);

		if (result != SQLITE_OK)
		{
			G_Printf("G_DB_Init: sqlite3_open_v2 failed: %s\n", sqlite3_errstr(result));
			return 1;
		}

		// set pragma sync off
		result = sqlite3_prepare(level.database.db, "PRAGMA synchronous = OFF", -1, &sqlstmt, NULL);

		if (result != SQLITE_OK)
		{
			G_Printf("G_DB_Init: sqlite3_prepare failed: %s\n", sqlite3_errstr(result));
			return 1;
		}

		result = sqlite3_step(sqlstmt);

		if (result == SQLITE_DONE)
		{
			result = sqlite3_exec(level.database.db, "PRAGMA synchronous = OFF", NULL, NULL, &err_msg);

			if (result != SQLITE_OK)
			{
				G_Printf("G_DB_Init: sqlite3_exec:PRAGMA failed: %s\n", err_msg);
				sqlite3_free(err_msg);
				return 1;
			}
		}

		result = sqlite3_finalize(sqlstmt);

		if (result != SQLITE_OK)
		{
			G_Printf("G_DB_Init: sqlite3_finalize failed\n");
			return 1;
		}
	}

	// initialize db - keep it open until deinit
	level.database.initialized = 1;

	return 0;
}

/**
 * @brief G_DB_DeInit
 * @return 0 if database is successfully deinitialized, 1 otherwise.
 */
int G_DB_DeInit()
{
	int result;

	if (!level.database.initialized)
	{
		G_Printf("G_DB_DeInit: access to non-initialized database\n");
		return 1;
	}

	// close db
	result = sqlite3_close(level.database.db);
	if (result != SQLITE_OK)
	{
		G_Printf("G_DB_DeInit: sqlite3_close failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	level.database.db          = NULL;
	level.database.path[0]     = '\0';
	level.database.initialized = 0;

	return 0;
}
#endif
