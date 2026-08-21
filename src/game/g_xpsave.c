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
 * @file g_xpsave.c
 * @brief Stores, loads and resets XP
 */
#ifdef FEATURE_XPSAVE

#include "g_local.h"

#include <math.h>
#include <time.h>

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
	time_t updated;
} xpData_t;

static int G_XPSave_Read(xpData_t *xp_data);
static int G_XPSave_Write(xpData_t *xp_data);
static void G_XPSave_ApplyDecay(xpData_t *xp_data);

#define XPCHECK_SQLWRAP_TABLES "SELECT * FROM xpsave_users;"
#define XPCHECK_SQLWRAP_SCHEMA "SELECT guid, skills, medals, created, updated FROM xpsave_users;"
#define XPUSERS_SQLWRAP_SELECT "SELECT guid, skills, medals, created, strftime('%%s', updated) FROM xpsave_users WHERE guid = '%s';"
#define XPUSERS_SQLWRAP_INSERT "INSERT INTO xpsave_users (guid, skills, medals, created, updated) VALUES ('%s', ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);"
#define XPUSERS_SQLWRAP_UPDATE "UPDATE xpsave_users SET skills = ?, medals = ?, updated = CURRENT_TIMESTAMP WHERE guid = '%s';"
#define XPUSERS_SQLWRAP_DELETE "DELETE FROM xpsave_users"
#define XPUSERS_SQLWRAP_DELETE_GUID "DELETE FROM xpsave_users WHERE guid = '%s';"

/**
 * @brief Checks if database exists, if tables exist and if schemas are correct
 * @param[in] db_path
 * @param[in] db_mode
 * @return 0 if database check is successful, 1 otherwise.
 */
int G_XPSave_CheckDB(char *db_path, int db_mode)
{
	int     result;
	sqlite3 *db;

	if (!db_path || db_path[0] == '\0')
	{
		G_Printf("G_XPSave_CheckDB: invalid path specified\n");
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
		G_Printf("G_XPSave_CheckDB: sqlite3_open_v2 failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	// check if tables exist
	result = sqlite3_exec(db, XPCHECK_SQLWRAP_TABLES, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSave_CheckDB: sqlite3_exec XPCHECK_SQLWRAP_TABLES failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_XPSave_CheckDB: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// check schema
	result = sqlite3_exec(db, XPCHECK_SQLWRAP_SCHEMA, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSave_CheckDB: sqlite3_exec XPCHECK_SQLWRAP_SCHEMA failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_XPSave_CheckDB: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// all ok
	result = sqlite3_close(db);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSave_CheckDB: sqlite3_close failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	return 0;
}

/**
 * @brief Retrieves xp for a client
 * @param[in] cl
 */
void G_XPSave_Load(gclient_t *cl)
{
	int      i;
	xpData_t xp_data;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSave_Load: access to non-initialized database\n");
		return;
	}

	if (!cl)
	{
		return;
	}

	// skip clients without a usable GUID (e.g. ETLTV slaves)
	// pers.cl_guid is set at connect time (format-validated when g_guidCheck is enabled); userinfo may be stale on slot reuse
	if (strlen(cl->pers.cl_guid) < MAX_GUID_LENGTH)
	{
		return;
	}

	// assign guid
	xp_data.guid = (const unsigned char *)cl->pers.cl_guid;

	// retrieve current xp or assign default values
	if (G_XPSave_Read(&xp_data))
	{
		return;
	}

	// apply decay to inactive players
	G_XPSave_ApplyDecay(&xp_data);

	// persist decayed values so the next connect does not re-decay
	if (xp_data.updated != 0 && g_xpSaveResetMode.integer == 3 && g_xpSaveResetThreshold.integer > 0)
	{
		G_XPSave_Write(&xp_data);
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
void G_XPSave_Store(gclient_t *cl)
{
	int      clientNum, i, j;
	xpData_t xp_data;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSave_Store: access to non-initialized database\n");
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

	// skip clients without a usable GUID (e.g. ETLTV slaves)
	// pers.cl_guid is set at connect time (format-validated when g_guidCheck is enabled); userinfo may be stale on slot reuse
	if (strlen(cl->pers.cl_guid) < MAX_GUID_LENGTH)
	{
		return;
	}

	// don't overwrite the database row while another connected client shares
	// the same GUID (can happen with g_guidCheck disabled or cheats)
	for (j = 0; j < level.maxclients; j++)
	{
		if (j == clientNum)
		{
			continue;
		}
		if (level.clients[j].pers.connected == CON_DISCONNECTED)
		{
			continue;
		}
		if (strlen(level.clients[j].pers.cl_guid) < MAX_GUID_LENGTH)
		{
			continue;
		}
		if (!Q_stricmp(cl->pers.cl_guid, level.clients[j].pers.cl_guid))
		{
			return;
		}
	}

	xp_data.guid = (const unsigned char *)cl->pers.cl_guid;

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		xp_data.skillpoints[i] = (int)cl->sess.skillpoints[i];
		xp_data.medals[i]      = (int)cl->sess.medals[i];
	}

	// save or update xp
	if (G_XPSave_Write(&xp_data))
	{
		return;
	}
}

/**
 * @brief Retrieves XP from the xpsave_users table
 * @param[in] xp_data
 * @return 0 if successful, 1 otherwise.
 */
static int G_XPSave_Read(xpData_t *xp_data)
{
	int          result, i;
	const char   *err;
	sqlite3_stmt *sqlstmt;
	const int    *pSkills;
	const int    *pMedals;

	Com_Memset(xp_data->skillpoints, 0, sizeof(xp_data->skillpoints));
	Com_Memset(xp_data->medals, 0, sizeof(xp_data->medals));
	xp_data->updated = 0;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSave_Read: access to non-initialized database\n");
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

		xp_data->updated = (time_t)sqlite3_column_int64(sqlstmt, 4);
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
static int G_XPSave_Write(xpData_t *xp_data)
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
		G_Printf("G_XPSave_Write: access to non-initialized database\n");
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
 * @brief Removes xp data for a single player from the table
 * @param[in] guid
 * @return 0 if successful, 1 otherwise.
 */
int G_XPSave_Reset(const unsigned char *guid)
{
	int  result;
	char *err_msg = NULL;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSave_Reset: access to non-initialized database\n");
		return 1;
	}

	if (!guid || guid[0] == '\0')
	{
		G_Printf("G_XPSave_Reset: invalid guid\n");
		return 1;
	}

	result = sqlite3_exec(level.database.db, va(XPUSERS_SQLWRAP_DELETE_GUID, guid), 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSave_Reset: sqlite3_exec failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	return 0;
}

/**
 * @brief Clears any xp data from the table
 * @return 0 if successful, 1 otherwise.
 */
int G_XPSave_Clear()
{
	int  result;
	char *err_msg = NULL;

	if (!level.database.initialized)
	{
		G_Printf("G_XPSave_Clear: access to non-initialized database\n");
		return 1;
	}

	result = sqlite3_exec(level.database.db, XPUSERS_SQLWRAP_DELETE, 0, 0, &err_msg);

	if (result != SQLITE_OK)
	{
		G_Printf("G_XPSave_Clear: sqlite3_exec failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	return 0;
}

/**
 * @brief Applies exponential decay to skillpoints based on inactivity.
 *        g_xpSaveResetThreshold is interpreted as the half-life in days.
 *        Medals are not decayed.
 * @param[in,out] xp_data
 */
static void G_XPSave_ApplyDecay(xpData_t *xp_data)
{
	time_t now;
	double days, halfLife, factor;
	int    i, newXp, decayed = qfalse;

	if (g_xpSaveResetMode.integer != 3)
	{
		return;
	}

	if (g_xpSaveResetThreshold.integer <= 0)
	{
		return;
	}

	if (xp_data->updated == 0)
	{
		return;
	}

	now      = time(NULL);
	days     = difftime(now, xp_data->updated) / 86400.0;
	halfLife = (double)g_xpSaveResetThreshold.integer;

	if (days <= 0.0 || halfLife <= 0.0)
	{
		return;
	}

	factor = pow(0.5, days / halfLife);

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		if (xp_data->skillpoints[i] == 0)
		{
			continue;
		}

		newXp = (int)(xp_data->skillpoints[i] * factor);

		if (newXp < 0)
		{
			newXp = 0;
		}

		if (newXp != xp_data->skillpoints[i])
		{
			xp_data->skillpoints[i] = newXp;
			decayed                 = qtrue;
		}
	}

	if (decayed)
	{
		G_DPrintf("XP save: decayed skills after %.2f days of inactivity (half-life %.0f days)\n", days, halfLife);
	}
}

#endif
