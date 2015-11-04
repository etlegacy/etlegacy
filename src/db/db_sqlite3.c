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
 * @file db_sqlite3.c
 * @brief ET: Legacy SQL interface
 */

#include "../qcommon/q_shared.h"

#include <sqlite3.h>

//char *dbname = "C:\\about.db";

sqlite3 *db;
sqlite3_stmt *stmt;

char message[255];

int date;
char *description;
char *venue;

int init(char *dbname)
{
        int result = sqlite3_open(dbname, &db);

        if (result != SQLITE_OK)
        {
        		Com_Printf("Failed to open database %s\n", sqlite3_errstr(result));
                sqlite3_close(db);
                return 1;
        }
        Com_Printf("DB %s opened\n", dbname);

        return 0;
}

int close()
{
        int result = sqlite3_close(db);

        Com_Printf("SQLite3 database closed.\n");
        return 0;
}
