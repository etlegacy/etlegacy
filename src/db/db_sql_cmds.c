/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 * @file db_sql_cmds.c
 * @brief SQL console commands for ETL
 */

#include "db_sql.h"

/**
 * @brief command to enter sql querries on the console
 */
void DB_ExecSQLCommand_f(void)
{
	int  result;
	//char *cmd;
	char *sql;
	char *err_msg = 0;

	//cmd = Cmd_Argv(0);
	sql = Cmd_Args();

	if (db_mode->integer == 0)
	{
		Com_Printf("SQL db disabled!\n");
		return;
	}

	if (!sql || !sql[0])
	{
		Com_Printf("Usage: sql \"<sql_statement>\"\n");
		return;
	}

	if (!db || !isDBActive)
	{
		Com_Printf("SQL db not available!\n");
		return;
	}

	result = sqlite3_exec(db, sql, DB_Callback, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("SQL query '%s' failed: \n%s\n", sql, err_msg);
		sqlite3_free(err_msg);
		return;
	}

	Com_DPrintf("Executed SQL query: '%s'\n", sql);
}

/**
 * @brief saves memory db to disk
 */
void DB_SaveMemDB_f(void)
{
	if (!db || db_mode->integer == 0)
	{
		Com_Printf("saveDB: db not available or disabled!\n");
		return;
	}

	if (db_mode->integer != 1)
	{
		Com_Printf("saveDB: command only available for memory DBMS\n");
		return;
	}

	if (!DB_SaveMemDB())
	{
		Com_Printf("saveDB: can't save database.\n");
	}
}
