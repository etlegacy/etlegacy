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
 *
 * TODO:  - extend our db scheme
 *        - implement loading from disk (if there is any file, if not prepare first start)
 *        - implement version system & auto updates
 *        - ...
 *
 *        Tutorial: http://zetcode.com/db/sqlitec/
 */

#include "db_sql.h"

cvar_t *db_mode;
cvar_t *db_url;

sqlite3  *db;
qboolean isDBActive;

int DB_Init()
{
	int result;

	isDBActive = qfalse;

	db_mode = Cvar_Get("db_mode", "1", CVAR_ARCHIVE | CVAR_LATCH);
	db_url  = Cvar_Get("db_url", "etl.db", CVAR_ARCHIVE | CVAR_LATCH);

	if (!db_url->string[0])
	{
		Com_Printf("SQLite3 can't init database - empty url\n");
		return 1;
	}

	if (db_mode->integer < 1 || db_mode->integer > 2)
	{
		Com_Printf("SQLite3 DBMS disabled\n");
		return 1;
	}

	Com_Printf("SQLite3 database %s init - libversion %s\n", db_url->string, sqlite3_libversion());

	if (FS_SV_FileExists(db_url->string)) // FIXME
	{
		result = DB_LoadOrSaveDb(db, db_url->string, 0);

		if (result != SQLITE_OK)
		{
			Com_Printf("SQLite3 WARNING can't load database file %s\n", db_url->string); // how to deal with this?
			return 1;
		}

		// FIXME: check for updates ...if version is behind current version update the db ...
	}
	else
	{
		Com_Printf("SQLite3 no database %s found ... creating now\n", db_url->string);
		result = DB_Create();

		if (result != SQLITE_OK)
		{
			Com_Printf("SQLite3 WARNING can't create memory database\n"); // how to deal with this?
			return 1;
		}

		// save db
		result = DB_LoadOrSaveDb(db, db_url->string, 1);

		if (result != SQLITE_OK)
		{
			Com_Printf("SQLite3 WARNING can't save memory database file\n"); // how to deal with this?
			return 1;
		}
	}

	isDBActive = qtrue;
	return 0;
}

/**
 * @brief creates tables and populates our scheme
 */
static int DB_Create_Scheme()
{
	int          result;
	sqlite3_stmt *res;
	char         *err_msg = 0;

	// FIXME:
	// - split this into client and server DB?!
	// - set index for search fields // CREATE INDEX player_guid ON PLAYER(guid)

	// version table
	char *sql = "DROP TABLE IF EXISTS ETL_VERSION;"
	            "CREATE TABLE ETL_VERSION (Id INT PRIMARY KEY NOT NULL, name TEXT, sql TEXT, created TEXT);"
	            "INSERT INTO ETL_VERSION VALUES (1, 'ET: L DBMS', '', CURRENT_TIMESTAMP);"; // FIXME: separate version inserts for updates ...

	result = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("SQLite3 failed to create table version: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	// ban/mute table (ensure we can also do IP range ban entries)
	// type = mute/ban
	// af = AddressFamily
	sql = "DROP TABLE IF EXISTS BAN;"
	      "CREATE TABLE BAN (Id INT PRIMARY KEY NOT NULL, address TEXT, guid TEXT, type INT NOT NULL, reason TEXT, af INT, length TEXT, expires TEXT, created TEXT, updated TEXT);";

	result = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("SQLite3 failed to create table ban: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	// player/client table
	// FIXME: do we want to track player names as PB did?
	sql = "DROP TABLE IF EXISTS PLAYER;"
	      "CREATE TABLE PLAYER (Id INT PRIMARY KEY NOT NULL, name TEXT, guid TEXT, user TEXT, password TEXT, mail TEXT, created TEXT, updated TEXT);";

	result = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("SQLite3 failed to create table player: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	// session table - server side tracking of players, client side tracking of games
	// note: we might drop length field (created = start, updated = end of session
	sql = "DROP TABLE IF EXISTS SESSION;"
	      "CREATE TABLE SESSION (Id INT PRIMARY KEY NOT NULL, pId, INT, address TEXT, port INT, type INT, duration TEXT, map TEXT, length TEXT, created TEXT, updated TEXT);";

	result = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		Com_Printf("SQLite3 failed to create table session: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	// FIXME:
	// add server table - store available maps, uptime & such
	// add favourite table ?! (client)
	return 0;
}

int DB_Create()
{
	int result;

	result = sqlite3_open(":memory:", &db);
	// FIXME: set name of DB

	if (result != SQLITE_OK)
	{
		Com_Printf("SQLite3 failed to open memory database - error: %s\n", sqlite3_errstr(result));
		(void) sqlite3_close(db);
		return 1;
	}

	DB_Create_Scheme();

	Com_Printf("SQLite3 database %s created\n", db_url->string);
	return 0;
}

int DB_Close()
{
	int result;

	result = sqlite3_close(db);

	isDBActive = qfalse;

	if (result != SQLITE_OK)
	{
		Com_Printf("SQLite3 failed to close database.\n");
		return 1;
	}

	Com_Printf("SQLite3 database closed.\n");
	return 0;
}

/**
 * @brief Perform an online backup of our database db to the database file named
 * by zFilename. Used to store our memory table in between map changes to disk
 *
 * This function copies 5 database pages from pDb to
 * zFilename, then unlocks pDb and sleeps for 250 ms, then repeats the
 * process until the entire database is backed up.
 *
 * The third argument passed to this function must be a pointer to a progress
 * function. After each set of 5 pages is backed up, the progress function
 * is invoked with two integer parameters: the number of pages left to
 * copy, and the total number of pages in the source file. This information
 * may be used, for example, to update a GUI progress bar.
 *
 * While this function is running, another thread may use the database pDb, or
 * another process may access the underlying database file via a separate
 * connection.
 *
 * If the backup process is successfully completed, SQLITE_OK is returned.
 * Otherwise, if an error occurs, an SQLite error code is returned.
 */
int DB_BackupDB(const char *zFilename, void (*xProgress)(int, int)) // Progress function to invoke
{
	int            rc;        // Function return code
	sqlite3        *pFile;    // Database connection opened on zFilename
	sqlite3_backup *pBackup;  // Backup handle used to copy data

	// Open the database file identified by zFilename.
	rc = sqlite3_open(zFilename, &pFile);
	if (rc == SQLITE_OK)
	{
		// Open the sqlite3_backup object used to accomplish the transfer
		pBackup = sqlite3_backup_init(pFile, "main", db, "main");
		if (pBackup)
		{
			// Each iteration of this loop copies 5 database pages from database
			// pDb to the backup database. If the return value of backup_step()
			// indicates that there are still further pages to copy, sleep for
			// 250 ms before repeating.
			do
			{
				rc = sqlite3_backup_step(pBackup, 5);
				xProgress(sqlite3_backup_remaining(pBackup), sqlite3_backup_pagecount(pBackup));
				if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED)
				{
					sqlite3_sleep(250);
				}
			}
			while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

			// Release resources allocated by backup_init().
			(void) sqlite3_backup_finish(pBackup);
		}
		rc = sqlite3_errcode(pFile);
	}

	// Close the database connection opened on database file zFilename and return the result of this function.
	(void) sqlite3_close(pFile);
	return rc;
}

/**
 * @brief This function is used to load the contents of a database file on disk
 * into the "main" database of open database connection pInMemory, or
 * to save the current contents of the database opened by pInMemory into
 * a database file on disk. pInMemory is probably an in-memory database,
 * but this function will also work fine if it is not.
 *
 * Parameter zFilename points to a nul-terminated string containing the
 * name of the database file on disk to load from or save to. If parameter
 * isSave is non-zero, then the contents of the file zFilename are
 * overwritten with the contents of the database opened by pInMemory. If
 * parameter isSave is zero, then the contents of the database opened by
 * pInMemory are replaced by data loaded from the file zFilename.
 *
 * If the operation is successful, SQLITE_OK is returned. Otherwise, if
 * an error occurs, an SQLite error code is returned.
 */
int DB_LoadOrSaveDb(sqlite3 *pInMemory, const char *zFilename, int isSave)
{
	int            rc;      // Function return code
	sqlite3        *pFile;  // Database connection opened on zFilename
	sqlite3_backup *pBackup; // Backup object used to copy data
	sqlite3        *pTo;    // Database to copy to (pFile or pInMemory)
	sqlite3        *pFrom;  // Database to copy from (pFile or pInMemory)

	// Open the database file identified by zFilename. Exit early if this fails for any reason.
	rc = sqlite3_open(zFilename, &pFile);
	if (rc == SQLITE_OK)
	{
		// If this is a 'load' operation (isSave==0), then data is copied
		// from the database file just opened to database pInMemory.
		// Otherwise, if this is a 'save' operation (isSave==1), then data
		// is copied from pInMemory to pFile.  Set the variables pFrom and
		// pTo accordingly.
		pFrom = (isSave ? pInMemory : pFile);
		pTo   = (isSave ? pFile     : pInMemory);

		// Set up the backup procedure to copy from the "main" database of
		// connection pFile to the main database of connection pInMemory.
		// If something goes wrong, pBackup will be set to NULL and an error
		// code and  message left in connection pTo.
		//
		// If the backup object is successfully created, call backup_step()
		// to copy data from pFile to pInMemory. Then call backup_finish()
		// to release resources associated with the pBackup object.  If an
		// error occurred, then  an error code and message will be left in
		// connection pTo. If no error occurred, then the error code belonging
		// to pTo is set to SQLITE_OK.
		pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
		if (pBackup)
		{
			(void) sqlite3_backup_step(pBackup, -1);
			(void) sqlite3_backup_finish(pBackup);
		}
		rc = sqlite3_errcode(pTo);
	}

	// Close the database connection opened on database file zFilename and return the result of this function.
	(void) sqlite3_close(pFile);
	return rc;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	NotUsed = 0;

	//for (i = 0; i < argc; i++)
	//{
	//	Com_Printf("%s ", azColName[i]);
	//}
	//Com_Printf("\n");

	Com_Printf("|");
	for (i = 0; i < argc; i++)
	{
		Com_Printf("%s|", argv[i] && argv[i][0] ? argv[i] : "NULL");
	}
	Com_Printf("\n");

	return 0;
}
