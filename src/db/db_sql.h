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
 * @file db_sql.h
 * @brief ET: Legacy SQL interface
 */

#ifndef INCLUDE_DB_SQL_H
#define INCLUDE_DB_SQL_H

#include <sqlite3.h>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

// when updating our DB schema in DB_CreateSchema increase this
// for existing databases on servers we do the upgrade with Lua scripts or SQL commands stored in version table ...
#define SQL_DBMS_SCHEMA_VERSION 2

extern cvar_t *db_mode;     // 0 - disabled, 1 - sqlite3 memory db, 2 - sqlite3 file db
extern cvar_t *db_uri;

extern sqlite3  *db;        // our sqlite3 database
extern qboolean isDBActive; // general flag for active dbms (db_mode is latched)

qboolean DB_Init(void);
qboolean DB_Create(void);
qboolean DB_DeInit(void);
qboolean DB_CheckUpdates(void);
int DB_LoadOrSaveDb(sqlite3 *, const char *, int);
// int DB_BackupDB(const char *, void *));
qboolean DB_SaveMemDB(void); // use in code

int DB_Callback(void *, int, char **, char **);

void DB_SaveMemDB_f(void); // console command to store memory db at any time to disk
void DB_ExecSQLCommand_f(void);

#endif // INCLUDE_DB_SQL_H
