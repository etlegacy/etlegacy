/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2015 ET: Legacy team <mail@etlegacy.com>
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
 * @file db_sql_console.c
 * @brief ET: SQL command line interface
 */

#include "db_sql.h"

/**
 * @brief command to enter sql querries on the console
 */
void DB_ExecSQLCommand_f(void)
{
	int  result;
	char *cmd;
	char *sql;
	char *err_msg = 0;

	cmd = Cmd_Argv(0);
	sql = Cmd_Args();

	if (!sql || !sql[0])
	{
		Com_Printf("Usage: %s sql <sql statement>\n", cmd);
		return;
	}

	if (!db || db_mode->integer == 0)
	{
		Com_Printf("sql db not available or disabled\n\n", cmd);
		return;
	}

	// debug
	//Com_Printf("sql: '%s'\n", sql);

	result = sqlite3_exec(db, sql, callback, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("sql command failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return;
	}

	return;
}
