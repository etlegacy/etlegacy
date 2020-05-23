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
 * @file g_xp_saver.c
 * @brief Stores, loads and resets XP
 */

#include "g_local.h"

#ifdef FEATURE_DBMS
#include <sqlite3.h>
#endif

#ifndef _MSC_VER
#define __FUNCTION__ __func__
#endif

#define bf_write(bf, T, input) *((T*)bf++) = (T)input;
#define bf_read(bf, T, output) output = *((T*)bf++);
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
		xp_data.medals[i] = (int)cl->sess.medals[i];
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
		pSkills = (int*)sqlite3_column_blob(sqlstmt, 1);
		assert_return(pSkills, 1, sqlite3_errmsg(level.database.db));

		pMedals = (int*)sqlite3_column_blob(sqlstmt, 2);
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
		result = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_INSERT, xp_data->guid), -1, &sqlstmt, NULL);
	}
	else
	{
		result = sqlite3_prepare(level.database.db, va(XPUSERS_SQLWRAP_UPDATE, xp_data->guid), -1, &sqlstmt, NULL);
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
	int          result;
	char         *err_msg = NULL;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSaver_Clear: access to non-initialized database\n");
		return 1;
	}

	result = sqlite3_exec(level.database.db, XPUSERS_SQLWRAP_DELETE, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSaver_Clear: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	return 0;
}
