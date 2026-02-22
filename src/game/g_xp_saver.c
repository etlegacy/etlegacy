/*
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
 */
/**
 * @file g_xp_saver.c
 * @brief Stores, loads and resets XP
 */

#include "g_local.h"

#ifdef FEATURE_DBMS
#include <sqlite3.h>
#else
#error Well here we are. Fix this. This whole file and functionality needs to be macroed away.
#endif

#ifndef _MSC_VER
#define __FUNCTION__ __func__
#endif

#define bf_write(bf, T, input) *((T *)bf++) = (T)input;
#define bf_read(bf, T, output) output       = *((T *)bf++);
#define assert_return(cond, status, msg) \
		if (!(cond)) { \
			if (msg) { \
				G_Printf("^1%s (%i): failed: %s\n", __func__, __LINE__, msg); \
			} \
			return status; \
		}

typedef struct xpData_s
{
	const unsigned char *guid;
	int skillpoints[SK_NUM_SKILLS];
	int medals[SK_NUM_SKILLS];
} xpData_t;

static int G_XPSaver_Read(xpData_t *xp_data);
static int G_XPSaver_Write(xpData_t *xp_data);
void G_XPList_Files(const char *directory);
void G_XPImportAll_IntoDatabase();
void G_XPCheck_Expiration(xpData_t *xp_data);

#define XP_FILE_MAGIC 0x0ACED00D  // Little-endian version of 0D D0 CE 0A
#define NUM_FILES_BUFFER 262144

#define XPCHECK_SQLWRAP_TABLES "SELECT * FROM xpsave_users;"
#define XPCHECK_SQLWRAP_SCHEMA "SELECT guid, skills, medals, created, updated FROM xpsave_users;"
#define XPUSERS_SQLWRAP_SELECT "SELECT * FROM xpsave_users WHERE guid = '%s';"
#define XPUSERS_SQLWRAP_INSERT "INSERT INTO xpsave_users (guid, skills, medals, created, updated) VALUES ('%s', ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);"
#define XPUSERS_SQLWRAP_UPDATE "UPDATE xpsave_users SET skills = ?, medals = ?, updated = CURRENT_TIMESTAMP WHERE guid = '%s';"
#define XPUSERS_SQLWRAP_DELETE "DELETE FROM xpsave_users"

/**
 * @brief Checks if database exists, if tables exist and if schemas are correct
 * @param[in] db_path
 * @param[in] db_mode
 * @return 0 if database check is successful, 1 otherwise.
 */
int G_XPSaver_CheckDB(char *db_path, int db_mode)
{
	int     result;
	sqlite3 *db;

	if (!db_path || db_path[0] == '\0')
	{
		G_Printf("G_XPSaver_CheckDB: invalid path specified\n");
		return 1;
	}

	// check if database can be opened
	if (db_mode == 1)
	{
		result = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_MEMORY | SQLITE_OPEN_SHAREDCACHE, NULL);
	}
	else // db_mode 2
	{
		result = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
	}

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSaver_CheckDB: sqlite3_open_v2 failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	// check if tables exist
	result = sqlite3_exec(db, XPCHECK_SQLWRAP_TABLES, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSaver_CheckDB: sqlite3_exec XPCHECK_SQLWRAP_TABLES failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_XPSaver_CheckDB: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// check schema
	result = sqlite3_exec(db, XPCHECK_SQLWRAP_SCHEMA, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSaver_CheckDB: sqlite3_exec XPCHECK_SQLWRAP_SCHEMA failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_XPSaver_CheckDB: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// all ok
	result = sqlite3_close(db);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSaver_CheckDB: sqlite3_close failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	return 0;
}

/**
 * @brief Retrieves xp for a client
 * @param[in] cl
 */
void G_XPSaver_Load(gclient_t *cl)
{
	char      userinfo[MAX_INFO_STRING];
	char      *guid;
	int       clientNum, i;
	xpData_t  xp_data;
	gentity_t *ent;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Load: access to non-initialized database\n");
		return;
	}

	if (!cl)
	{
		return;
	}

	clientNum = cl - level.clients;

	// ignore bots
	ent = g_entities + clientNum;

	if (ent->r.svFlags & SVF_BOT)
	{
		return;
	}

	// retrieve guid
	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	guid = Info_ValueForKey(userinfo, "cl_guid");

	// assign guid
	xp_data.guid = (const unsigned char *)guid;

	// retrieve current xp or assign default values
	if (G_XPSaver_Read(&xp_data))
	{
		return;
	}

	// assign user data to session
	cl->sess.startxptotal = 0;
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		cl->sess.skillpoints[i]      = xp_data.skillpoints[i];
		cl->sess.startskillpoints[i] = xp_data.skillpoints[i];
		cl->sess.startxptotal       += xp_data.skillpoints[i];
		cl->sess.medals[i]          += xp_data.medals[i];
	}
}

/**
 * @brief Updates xp stats and timestamp for client
 * @param[in] cl
 */
void G_XPSaver_Store(gclient_t *cl)
{
	char      userinfo[MAX_INFO_STRING];
	char      *guid;
	int       clientNum, i;
	xpData_t  xp_data;
	gentity_t *ent;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Store: access to non-initialized database\n");
		return;
	}

	if (!cl)
	{
		return;
	}

	// don't record any data in warmup
	if (level.warmupTime)
	{
		return;
	}

	clientNum = cl - level.clients;

	// ignore bots
	ent = g_entities + clientNum;

	if (ent->r.svFlags & SVF_BOT)
	{
		return;
	}

	// retrieve guid
	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	guid = Info_ValueForKey(userinfo, "cl_guid");

	xp_data.guid = (const unsigned char *)guid;

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		xp_data.skillpoints[i] = (int)cl->sess.skillpoints[i];
		xp_data.medals[i]      = (int)cl->sess.medals[i];
	}

	// save or update xp
	if (G_XPSaver_Write(&xp_data))
	{
		return;
	}
}

/**
 * @brief Retrieves XP from the xpsave_users table
 * @param[in] xp_data
 * @return 0 if successful, 1 otherwise.
 */
static int G_XPSaver_Read(xpData_t *xp_data)
{
	int          result, i;
	const char   *err;
	sqlite3_stmt *sqlstmt;
	const int    *pSkills;
	const int    *pMedals;

	Com_Memset(xp_data->skillpoints, 0, sizeof(xp_data->skillpoints));
	Com_Memset(xp_data->medals, 0, sizeof(xp_data->medals));

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Read: access to non-initialized database\n");
		return 1;
	}

	result = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_SELECT, xp_data->guid), -1, &sqlstmt, NULL);
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_ROW)
	{
		/* retrieve skills */
		pSkills = (int *)sqlite3_column_blob(sqlstmt, 1);
		assert_return(pSkills, 1, sqlite3_errmsg(level.database.db));

		pMedals = (int *)sqlite3_column_blob(sqlstmt, 2);
		assert_return(pMedals, 2, sqlite3_errmsg(level.database.db));

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			bf_read(pSkills, int, xp_data->skillpoints[i]);
			bf_read(pMedals, int, xp_data->medals[i]);
		}
	}
	// no entry found or other failure
	else if (result != SQLITE_DONE)
	{
		err = sqlite3_errmsg(level.database.db);
		if (err)
		{
			G_Printf("^3%s (%i): failed: %s\n", __func__, __LINE__, err);
		}
		sqlite3_finalize(sqlstmt);
		return 1;
	}

	result = sqlite3_finalize(sqlstmt);
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	G_XPCheck_Expiration(xp_data);

	return 0;
}

/**
 * @brief Sets or updates skills and medals
 * @param[in] xp_data
 * @return 0 if successful, 1 otherwise.
 */
static int G_XPSaver_Write(xpData_t *xp_data)
{
	int          i;
	int          result;
	const char   *err;
	sqlite3_stmt *sqlstmt;
	int          buffer[SK_NUM_SKILLS * 2];
	int          *pSkills;
	int          *pMedals;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Write: access to non-initialized database\n");
		return 1;
	}

	result = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_SELECT, xp_data->guid), -1, &sqlstmt, NULL);
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	result = sqlite3_step(sqlstmt);

	pSkills = buffer;
	pMedals = buffer + SK_NUM_SKILLS;
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		bf_write(pSkills, int, xp_data->skillpoints[i]);
		bf_write(pMedals, int, xp_data->medals[i]);
	}

	if (result == SQLITE_DONE)
	{
		sqlite3_finalize(sqlstmt);
		sqlstmt = NULL;
		result  = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_INSERT, xp_data->guid), -1, &sqlstmt, NULL);
	}
	else if (result == SQLITE_ROW)
	{
		sqlite3_finalize(sqlstmt);
		sqlstmt = NULL;
		result  = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_UPDATE, xp_data->guid), -1, &sqlstmt, NULL);
	}
	else
	{
		err = sqlite3_errmsg(level.database.db);
		if (err)
		{
			G_Printf("^3%s (%i): failed: %s\n", __func__, __LINE__, err);
		}
		sqlite3_finalize(sqlstmt);
		return 1;
	}
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	result = sqlite3_bind_blob(sqlstmt, 1, buffer, sizeof(int) * SK_NUM_SKILLS, SQLITE_STATIC);
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	result = sqlite3_bind_blob(sqlstmt, 2, buffer + SK_NUM_SKILLS, sizeof(int) * SK_NUM_SKILLS, SQLITE_STATIC);
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	result = sqlite3_step(sqlstmt);
	assert_return(result == SQLITE_DONE, 1, sqlite3_errmsg(level.database.db));

	result = sqlite3_finalize(sqlstmt);
	assert_return(result == SQLITE_OK, 1, sqlite3_errmsg(level.database.db));

	return 0;
}

/**
 * @brief Clears any xp data from the table
 * @return 0 if successful, 1 otherwise.
 */
int G_XPSaver_Clear()
{
	int  result;
	char *err_msg = NULL;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Clear: access to non-initialized database\n");
		return 1;
	}

	result = sqlite3_exec(level.database.db, XPUSERS_SQLWRAP_DELETE, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSaver_Clear: sqlite3_exec failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	return 0;
}

/**
 * @brief Reset xp and medals for player
 */
void G_XPCheck_Expiration(xpData_t *xp_data)
{
	int          result;
	const char   *err;
	sqlite3_stmt *sqlstmt;
	const char   *updated;
	int          age = 0, len;
	int          t, t2 = 0;
	qtime_t      ct;
	struct tm    tm;

	// Get the current time
	trap_RealTime(&ct);

	// Use the stdc mktime and struct tm to convert qtime_t
	// Initialise our tm structure
	tm.tm_sec   = ct.tm_sec;
	tm.tm_min   = ct.tm_min;
	tm.tm_hour  = ct.tm_hour;
	tm.tm_mday  = ct.tm_mday;
	tm.tm_mon   = ct.tm_mon;
	tm.tm_year  = ct.tm_year;
	tm.tm_wday  = ct.tm_wday;
	tm.tm_yday  = ct.tm_yday;
	tm.tm_isdst = ct.tm_isdst;
	// Perform the conversion and return
	t = (int) mktime(&tm);

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Read: access to non-initialized database\n");
	}

	result = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_SELECT, xp_data->guid), -1, &sqlstmt, NULL);

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_ROW)
	{
		/* retrieve updated */
		updated = (const char *)sqlite3_column_blob(sqlstmt, 4);
		len     = sqlite3_column_bytes(sqlstmt, 4);
		if (updated && len == 19)
		{
			struct tm tm_old;
			int       y, m, d, hh, mm, ss;

			if (Q_sscanf(updated, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss) == 6)
			{
				tm_old.tm_year = y - 1900;
				tm_old.tm_mon  = m - 1;
				tm_old.tm_mday = d;
				tm_old.tm_hour = hh;
				tm_old.tm_min  = mm;
				tm_old.tm_sec  = ss;

				t2 = (int)mktime(&tm_old);
			}
		}
		age = t - t2;
	}
	// no entry found or other failure
	else if (result != SQLITE_DONE)
	{
		err = sqlite3_errmsg(level.database.db);
		if (err)
		{
			G_Printf("^3%s (%i): failed: %s\n", __func__, __LINE__, err);
		}
		sqlite3_finalize(sqlstmt);
	}

	result = sqlite3_finalize(sqlstmt);

	if (age > g_xpSaverMaxAge.integer)
	{
		Com_Memset(xp_data->skillpoints, 0, sizeof(xp_data->skillpoints));
		Com_Memset(xp_data->medals, 0, sizeof(xp_data->medals));
		G_XPSaver_Write(xp_data);
	}

}

/**
 * @brief Convert/Import any xp file into the database
 * It can parse 3000 xp files
 */
void G_XPSaver_Convert()
{
	G_XPList_Files("xpsave");
	G_XPImportAll_IntoDatabase();
}

/**
 * @brief Parses the .xp file
 * @return 0 if successful, 1 otherwise.
 */
int G_XPFile_Parse(const char *filepath, xpData_t *xp_data)
{
	fileHandle_t f;
	int          len;
	int32_t      magic;
	char         name_buffer[40];
	char         guid_str[33];
	const char   *dot;
	int32_t      dummy;
	float        skill_float;
	int          i;

	// Extract GUID from filename
	const char *basename = strrchr(filepath, '/');
	if (!basename)
	{
		basename = strrchr(filepath, '\\');
	}
	basename = basename ? basename + 1 : filepath;

	dot = strrchr(basename, '.');

	if (dot && !Q_stricmp(dot, ".xp"))
	{
		size_t guid_len = dot - basename;
		if (guid_len == MAX_GUID_LENGTH)
		{
			Com_Memcpy(guid_str, basename, 32);
			guid_str[MAX_GUID_LENGTH] = '\0';
			xp_data->guid             = (const unsigned char *)strdup(guid_str);
		}
		else
		{
			G_Printf("ParseXPFile: Invalid GUID length in filename: %s\n", basename);
			return 1;
		}
	}
	else
	{
		G_Printf("ParseXPFile: Not an XP file: %s\n", basename);
		return 1;
	}

	len = trap_FS_FOpenFile(filepath, &f, FS_READ);
	if (len <= 0 || f == 0)
	{
		G_Printf("ParseXPFile: Could not open file: %s (len: %d)\n", filepath, len);
		Com_Dealloc((void *)xp_data->guid);
		return 1;
	}

	// Check file size (should be at least 76 bytes)
	if (len < 0x4C)
	{
		G_Printf("ParseXPFile: File too small: %s (size: %d)\n", filepath, len);
		trap_FS_FCloseFile(f);
		Com_Dealloc((void *)xp_data->guid);
		return 1;
	}

	// Read and verify magic number
	trap_FS_Read(&magic, sizeof(int32_t), f);

	if (magic != XP_FILE_MAGIC)
	{
		G_Printf("ParseXPFile: Invalid magic number in: %s (got: 0x%08X, expected: 0x%08X)\n", filepath, magic, XP_FILE_MAGIC);
		trap_FS_FCloseFile(f);
		Com_Dealloc((void *)xp_data->guid);
		return 1;
	}

	// Skip version/flags (4 bytes) - FS_Read doesn't return bytes read count
	// We need to read into a dummy buffer
	trap_FS_Read(&dummy, sizeof(int32_t), f);

	// Read and skip player name (40 bytes)
	trap_FS_Read(name_buffer, 40, f);

	// Read skillpoints
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		trap_FS_Read(&skill_float, sizeof(float), f);
		xp_data->skillpoints[i] = (int)skill_float;
		xp_data->medals[i]      = 0; // .xp files don't contain medals
	}

	trap_FS_FCloseFile(f);

	G_Printf("ParseXPFile: Successfully parsed %s\n", basename);
	return 0;
}

/**
 * @brief Prints content of .xp files into server console
 */
void G_XPList_Files(const char *directory)
{
	int      numFiles, i, j;
	char     *fileList = NULL;
	char     *fileBuf  = NULL;
	char     *filePtr;
	xpData_t xp_data;

	fileList = Com_Allocate(NUM_FILES_BUFFER);
	if (!fileList)
	{
		G_Printf("ERROR: Cannot allocate file list buffer (%d bytes)\n", NUM_FILES_BUFFER);
		return;
	}

	fileBuf = Com_Allocate(NUM_FILES_BUFFER);
	if (!fileBuf)
	{
		G_Printf("ERROR: Cannot allocate file buffer (%d bytes)\n", NUM_FILES_BUFFER);
		Com_Dealloc(fileList);
		return;
	}

	numFiles = trap_FS_GetFileList(directory, ".xp", fileList, NUM_FILES_BUFFER);

	if (numFiles <= 0)
	{
		G_Printf("No XP files found in %s\n", directory);
		Com_Dealloc(fileList);
		Com_Dealloc(fileBuf);
		return;
	}

	G_Printf("Found %d XP files in %s:\n", numFiles, directory);

	Com_Memcpy(fileBuf, fileList, NUM_FILES_BUFFER);

	filePtr = fileBuf;
	for (i = 0; i < numFiles; i++)
	{
		char filepath[MAX_QPATH];
		int  fileLen;

		if (!filePtr || !*filePtr)
		{
			break;
		}

		fileLen = strlen(filePtr);

		Com_sprintf(filepath, sizeof(filepath), "%s/%s", directory, filePtr);

		G_Printf(" [%d] %s\n", i + 1, filePtr);

		// Process the file
		if (G_XPFile_Parse(filepath, &xp_data) == 0)
		{
			// We assume we have all the players with GUIDS
			G_Printf(" GUID: %s\n", xp_data.guid);

			G_Printf(" Skills: ");
			for (j = 0; j < SK_NUM_SKILLS; j++)
			{
				G_Printf("%d ", xp_data.skillpoints[j]);
			}
			G_Printf("\n");

			Com_Dealloc((void *)xp_data.guid);
		}

		filePtr += fileLen + 1;
	}

	Com_Dealloc(fileList);
	Com_Dealloc(fileBuf);
}

/**
 * @brief Storage all the parsed .xp files into database
 */
void G_XPImportAll_IntoDatabase()
{
	const char *xp_dir = "xpsave";
	int        numFiles, i;
	char       *fileList = NULL;
	char       *filePtr;
	xpData_t   xp_data;

	G_Printf("=== Starting XP Import from %s ===\n", xp_dir);

	fileList = Com_Allocate(NUM_FILES_BUFFER);
	if (!fileList)
	{
		G_Printf("ERROR: Cannot allocate file list buffer (%d bytes)\n", NUM_FILES_BUFFER);
		return;
	}

	numFiles = trap_FS_GetFileList(xp_dir, ".xp", fileList, NUM_FILES_BUFFER);

	if (numFiles <= 0)
	{
		G_Printf("No XP files found\n");
		Com_Dealloc(fileList);
		return;
	}

	filePtr = fileList;
	for (i = 0; i < numFiles; i++)
	{
		char filepath[MAX_QPATH];
		int  fileLen = strlen(filePtr);

		Com_sprintf(filepath, sizeof(filepath), "%s/%s", xp_dir, filePtr);

		if (G_XPFile_Parse(filepath, &xp_data) == 0)
		{
			if (G_XPSaver_Write(&xp_data))
			{
				Com_Dealloc((void *)xp_data.guid);
				continue;
			}

			Com_Dealloc((void *)xp_data.guid);
		}
		else
		{
			G_Printf("Failed to parse: %s\n", filePtr);
		}

		filePtr += fileLen + 1;
	}

	Com_Dealloc(fileList);
	G_Printf("=== Import Complete ===\n");
}
