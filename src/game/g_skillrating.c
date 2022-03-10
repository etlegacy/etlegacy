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
 * @file g_skillrating.c
 * @brief Bayesian skill rating
 */
#ifdef FEATURE_RATING

#include "g_local.h"

#ifdef FEATURE_DBMS
#include <sqlite3.h>
#endif

#define SRCHECK_SQLWRAP_TABLES "SELECT * FROM rating_users; " \
	                           "SELECT * FROM rating_match; " \
	                           "SELECT * FROM rating_maps;"
#define SRCHECK_SQLWRAP_SCHEMA "SELECT guid, mu, sigma, created, updated FROM rating_users; " \
	                           "SELECT guid, mu, sigma, time_axis, time_allies FROM rating_match; " \
	                           "SELECT mapname, win_axis, win_allies FROM rating_maps;"
#define SRMATCH_SQLWRAP_DELETE "DELETE FROM rating_match;"
#define SRMATCH_SQLWRAP_SELECT "SELECT * FROM rating_match WHERE guid = '%s';"
#define SRMATCH_SQLWRAP_INSERT "INSERT INTO rating_match " \
	                           "(guid, mu, sigma, time_axis, time_allies) VALUES ('%s', '%f', '%f', '%i', '%i');"
#define SRMATCH_SQLWRAP_UPDATE "UPDATE rating_match " \
	                           "SET mu = '%f', sigma = '%f', time_axis = '%i', time_allies = '%i' WHERE guid = '%s';"
#define SRUSERS_SQLWRAP_SELECT "SELECT * FROM rating_users WHERE guid = '%s';"
#define SRUSERS_SQLWRAP_INSERT "INSERT INTO rating_users " \
	                           "(guid, mu, sigma, created, updated) VALUES ('%s', '%f', '%f', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);"
#define SRUSERS_SQLWRAP_UPDATE "UPDATE rating_users " \
	                           "SET mu = '%f', sigma = '%f', updated = CURRENT_TIMESTAMP WHERE guid = '%s';"
#define SRMATCH_SQLWRAP_TABLE  "SELECT * FROM rating_match;"
#define SRMAPS_SQLWRAP_SELECT  "SELECT * FROM rating_maps WHERE mapname = '%s';"
#define SRMAPS_SQLWRAP_INSERT  "INSERT INTO rating_maps " \
	                           "(win_axis, win_allies, mapname) VALUES ('%i', '%i', '%s');"
#define SRMAPS_SQLWRAP_UPDATE  "UPDATE rating_maps " \
	                           "SET win_axis = win_axis + '%i', win_allies = win_allies + '%i' WHERE mapname = '%s';"

// MU      25            - mean
// SIGMA   MU / 3        - standard deviation
// BETA    SIGMA / 2     - skill chain length
// TAU     SIGMA / 100   - dynamics factor
// EPSILON 0.f           - draw margin (assumed null)
// LAMBDA  10            - map continuity correction

/**
 * @brief Checks if database exists, if tables exist and if schemas are correct
 * @param[in] db_path
 * @param[in] db_mode
 * @return 0 if database check is successful, 1 otherwise.
 */
int G_SkillRatingDBCheck(char *db_path, int db_mode)
{
	int     result;
	sqlite3 *db;

	if (!db_path || db_path[0] == '\0')
	{
		G_Printf("G_SkillRatingDBCheck: invalid path specified\n");
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
		G_Printf("G_SkillRatingDBCheck: sqlite3_open_v2 failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	// check if tables exist
	result = sqlite3_exec(db, SRCHECK_SQLWRAP_TABLES, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingDBCheck: sqlite3_exec SRCHECK_SQLWRAP_TABLES failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingDBCheck: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// check schema
	result = sqlite3_exec(db, SRCHECK_SQLWRAP_SCHEMA, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingDBCheck: sqlite3_exec SRCHECK_SQLWRAP_SCHEMA failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingDBCheck: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// all ok
	result = sqlite3_close(db);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingDBCheck: sqlite3_close failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	return 0;
}

/**
 * @brief Ensure rating_match table is empty
 * @return 0 if rating_table is successfully emptied, 1 otherwise.
 */
int G_SkillRatingPrepareMatchRating(void)
{
	int          result;
	char         *err_msg = NULL;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingPrepareMatchRating: access to non-initialized database\n");
		return 1;
	}

	result = sqlite3_prepare(level.database.db, SRMATCH_SQLWRAP_DELETE, strlen(SRMATCH_SQLWRAP_DELETE), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingPrepareMatchRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_DONE)
	{
		result = sqlite3_exec(level.database.db, SRMATCH_SQLWRAP_DELETE, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingPrepareMatchRating: sqlite3_exec:DELETE failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingPrepareMatchRating: sqlite3_finalize failed\n");
		return 1;
	}

	return 0;
}

/**
 * @brief Retrieve rating from the rating_match table
 * @param[in] sr_data
 * @return 0 if successful, 2 if data is not found, 1 otherwise.
 */
int G_SkillRatingGetMatchRating(srData_t *sr_data)
{
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;
	qboolean     datafound = qtrue;

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingGetMatchRating: access to non-initialized database\n");
		return 1;
	}

	sql = va(SRMATCH_SQLWRAP_SELECT, sr_data->guid);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingGetMatchRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_ROW)
	{
		// assign match data
		sr_data->mu          = sqlite3_column_double(sqlstmt, 1);
		sr_data->sigma       = sqlite3_column_double(sqlstmt, 2);
		sr_data->time_axis   = sqlite3_column_int(sqlstmt, 3);
		sr_data->time_allies = sqlite3_column_int(sqlstmt, 4);
	}
	else
	{
		// no entry found (failsafe) or other failure
		if (result == SQLITE_DONE)
		{
			// assign default values (failsafe)
			sr_data->mu          = MU;
			sr_data->sigma       = SIGMA;
			sr_data->time_axis   = 0;
			sr_data->time_allies = 0;

			datafound = qfalse;
		}
		else
		{
			sqlite3_finalize(sqlstmt);

			G_Printf("G_SkillRatingGetMatchRating: sqlite3_step failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingGetMatchRating: sqlite3_finalize failed\n");
		return 1;
	}

	if (!datafound)
	{
		return 2;
	}
	return 0;
}

/**
 * @brief Sets or updates rating and time played in the rating_match table
 * @param[in] sr_data
 * @return 0 if successful, 1 otherwise.
 */
int G_SkillRatingSetMatchRating(srData_t *sr_data)
{
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingSetMatchRating: access to non-initialized database\n");
		return 1;
	}

	sql = va(SRMATCH_SQLWRAP_SELECT, sr_data->guid);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingSetMatchRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_DONE)
	{
		sql = va(SRMATCH_SQLWRAP_INSERT, sr_data->guid, sr_data->mu, sr_data->sigma, sr_data->time_axis, sr_data->time_allies);

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingSetMatchRating: sqlite3_exec:INSERT failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}
	else
	{
		sql = va(SRMATCH_SQLWRAP_UPDATE, sr_data->mu, sr_data->sigma, sr_data->time_axis, sr_data->time_allies, sr_data->guid);

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingSetMatchRating: sqlite3_exec:UPDATE failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingSetMatchRating: sqlite3_finalize failed\n");
		return 1;
	}

	return 0;
}

/**
 * @brief Retrieve rating from the rating_users table
 * @param[in] sr_data
 * @return 0 if successful, 1 otherwise.
 */
int G_SkillRatingGetUserRating(srData_t *sr_data)
{
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingGetUserRating: access to non-initialized database\n");
		return 1;
	}

	sql = va(SRUSERS_SQLWRAP_SELECT, sr_data->guid);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingGetUserRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_ROW)
	{
		// assign match data
		sr_data->mu          = sqlite3_column_double(sqlstmt, 1);
		sr_data->sigma       = sqlite3_column_double(sqlstmt, 2);
		sr_data->time_axis   = 0;
		sr_data->time_allies = 0;
	}
	else
	{
		// no entry found or other failure
		if (result == SQLITE_DONE)
		{
			// assign default values
			sr_data->mu          = MU;
			sr_data->sigma       = SIGMA;
			sr_data->time_axis   = 0;
			sr_data->time_allies = 0;
		}
		else
		{
			sqlite3_finalize(sqlstmt);

			G_Printf("G_SkillRatingGetUserRating: sqlite3_step failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingGetUserRating: sqlite3_finalize failed\n");
		return 1;
	}

	return 0;
}

/**
 * @brief Sets or updates rating and timestamps in the rating_users table
 * @param[in] sr_data
 * @return 0 if successful, 1 otherwise.
 */
int G_SkillRatingSetUserRating(srData_t *sr_data)
{
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingSetUserRating: access to non-initialized database\n");
		return 1;
	}

	sql = va(SRUSERS_SQLWRAP_SELECT, sr_data->guid);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingSetUserRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_DONE)
	{
		sql = va(SRUSERS_SQLWRAP_INSERT, sr_data->guid, sr_data->mu, sr_data->sigma);

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingSetUserRating: sqlite3_exec:INSERT failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}
	else
	{
		sql = va(SRUSERS_SQLWRAP_UPDATE, sr_data->mu, sr_data->sigma, sr_data->guid);

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingSetUserRating: sqlite3_exec:UPDATE failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingSetUserRating: sqlite3_finalize failed\n");
		return 1;
	}

	return 0;
}

/**
 * @brief Retrieve rating for client
 *         Called on ClientConnect and on G_UpdateSkillRating
 * @param[in] cl
 */
void G_SkillRatingGetClientRating(gclient_t *cl)
{
	char         userinfo[MAX_INFO_STRING];
	char         *guid;
	int          clientNum;
	srData_t     sr_data;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingGetClientRating: access to non-initialized database\n");
		return;
	}

	if (!cl)
	{
		return;
	}

	clientNum = cl - level.clients;

	// retrieve guid
	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	guid = Info_ValueForKey(userinfo, "cl_guid");

	// assign guid
	sr_data.guid = (const unsigned char *)guid;

	// retrieve current rating or assign default values
	if (level.warmupTime || level.intermissionQueued || level.intermissiontime)
	{
		// retrieve rating from rating_users table
		if (G_SkillRatingGetUserRating(&sr_data))
		{
			return;
		}

		// assign user data to session
		cl->sess.mu          = sr_data.mu;
		cl->sess.sigma       = sr_data.sigma;

		// ensure auto statsdump is correct
		if (!level.intermissionQueued && !level.intermissiontime)
		{
			cl->sess.time_axis   = 0;
			cl->sess.time_allies = 0;
		}

		// prepare delta rating
		if (!level.intermissionQueued)
		{
			cl->sess.oldmu    = sr_data.mu;
			cl->sess.oldsigma = sr_data.sigma;
		}
	}
	else // playing
	{
		// retrieve rating from rating_match or rating_users table or set default values
		switch (G_SkillRatingGetMatchRating(&sr_data))
		{
			case 1:
				// error occurred
				return;
			case 2:
				// data not found in rating_match
				G_SkillRatingGetUserRating(&sr_data);
				break;
			case 0:
				// data found
			default:
				break;
		}

		// assign match data to session
		cl->sess.mu          = sr_data.mu;
		cl->sess.sigma       = sr_data.sigma;
		cl->sess.time_axis   = sr_data.time_axis;
		cl->sess.time_allies = sr_data.time_allies;

		// prepare delta rating
		cl->sess.oldmu    = sr_data.mu;
		cl->sess.oldsigma = sr_data.sigma;
	}
}

/**
 * @brief Sets or updates rating and timestamp for client
 *         Called on ClientDisconnect and on G_LogExit before intermissionQueued
 * @param[in] cl
 */
void G_SkillRatingSetClientRating(gclient_t *cl)
{
	char         userinfo[MAX_INFO_STRING];
	char         *guid;
	int          clientNum;
	srData_t     sr_data;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingSetClientRating: access to non-initialized database\n");
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

	// retrieve guid
	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	guid = Info_ValueForKey(userinfo, "cl_guid");

	// assign match data
	sr_data.guid        = (const unsigned char *)guid;
	sr_data.mu          = cl->sess.mu;
	sr_data.sigma       = cl->sess.sigma;
	sr_data.time_axis   = cl->sess.time_axis;
	sr_data.time_allies = cl->sess.time_allies;

	// save match rating or update new user rating after calculation
	if (!level.intermissionQueued)
	{
		// player has not played at all
		if (sr_data.time_axis == 0 && sr_data.time_allies == 0)
		{
			return;
		}

		// save or update rating in rating_match table
		if (G_SkillRatingSetMatchRating(&sr_data))
		{
			return;
		}
	}
	else
	{
		// save or update rating in rating_users table
		if (G_SkillRatingSetUserRating(&sr_data))
		{
			return;
		}
	}
}

/**
 * @brief Retrieve map bias from the rating_maps table
 * @param[in] mapname
 * @return mapProb
 */
float G_SkillRatingGetMapRating(char *mapname)
{
	float        mapProb;
	int          win_axis, win_allies;
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return 0.5f;
	}

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingGetMapRating: access to non-initialized database\n");
		return 0.5f;
	}

	sql = va(SRMAPS_SQLWRAP_SELECT, mapname);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingGetMapRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 0.5f;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_ROW)
	{
		// assign map data
		win_axis   = sqlite3_column_int(sqlstmt, 1);
		win_allies = sqlite3_column_int(sqlstmt, 2);

		// map bias continuity correction
		if (win_axis + win_allies < 2 * LAMBDA)
		{
			// use integer division to decay one value for every 2 matches played
			int win_corrected_axis   = win_axis + LAMBDA - (win_axis + win_allies) / 2;
			int win_corrected_allies = win_allies + LAMBDA - (win_axis + win_allies) / 2;

			win_axis   = win_corrected_axis;
			win_allies = win_corrected_allies;
		}

		// calculate map bias
		mapProb = win_axis / (float)(win_axis + win_allies);
	}
	else
	{
		// no entry found or other failure
		if (result == SQLITE_DONE)
		{
			// assign default value
			mapProb = 0.5f;
		}
		else
		{
			sqlite3_finalize(sqlstmt);

			G_Printf("G_SkillRatingGetMapRating: sqlite3_step failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingGetMapRating: sqlite3_finalize failed\n");
		return 0.5f;
	}

	return mapProb;
}

/**
 * @brief Sets or updates map bias in the rating_maps table
 * @param[in] mapname
 * @param[in] winner
 */
void G_SkillRatingSetMapRating(char *mapname, int winner)
{
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_SkillRatingSetMapRating: access to non-initialized database\n");
		return;
	}

	sql = va(SRMAPS_SQLWRAP_SELECT, mapname);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingSetMapRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_DONE)
	{
		if (winner == TEAM_AXIS)
		{
			sql = va(SRMAPS_SQLWRAP_INSERT, 1, 0, mapname);
		}
		else // winner == TEAM_ALLIES
		{
			sql = va(SRMAPS_SQLWRAP_INSERT, 0, 1, mapname);
		}

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingSetMapRating: sqlite3_exec:INSERT failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return;
		}
	}
	else
	{
		if (winner == TEAM_AXIS)
		{
			sql = va(SRMAPS_SQLWRAP_UPDATE, 1, 0, mapname);
		}
		else // winner == TEAM_ALLIES
		{
			sql = va(SRMAPS_SQLWRAP_UPDATE, 0, 1, mapname);
		}

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_SkillRatingSetMapRating: sqlite3_exec:UPDATE failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_SkillRatingSetMapRating: sqlite3_finalize failed\n");
		return;
	}
}

/**
 * @brief Calculate skill ratings
 *         Called when intermissionQueued is triggered
 * @details Rate players in this map based on team performance
 */
void G_CalculateSkillRatings(void)
{
	char cs[MAX_STRING_CHARS];
	char *buf;
	int  winner;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return;
	}

	// determine winner
	trap_GetConfigstring(CS_MULTI_MAPWINNER, cs, sizeof(cs));
	buf    = Info_ValueForKey(cs, "w");
	winner = Q_atoi(buf);

	// change from scripting value for winner to spawnflag value
	switch (winner)
	{
	case 0: // AXIS
		winner = TEAM_AXIS;
		break;
	case 1: // ALLIES
		winner = TEAM_ALLIES;
		break;
	default:
		break;
	}

	// log
	G_LogPrintf("SkillRating: Map: %s, Winner: %d, Time: %d, Timelimit: %d\n",
	            level.rawmapname, winner, level.intermissionQueued - level.startTime - level.timeDelta, g_timelimit.integer * 60000);

	// update map rating
	if (g_skillRating.integer > 1)
	{
		G_SkillRatingSetMapRating(level.rawmapname, winner);
		level.mapProb = G_SkillRatingGetMapRating(level.rawmapname);

		G_LogPrintf("SkillRating: Map bias: %.6f\n", level.mapProb);

		// update map bias on intermission scoreboard
		trap_GetConfigstring(CS_MODINFO, cs, sizeof(cs));
		Info_SetValueForKey(cs, "M", va("%f", level.mapProb));
		trap_SetConfigstring(CS_MODINFO, cs);
	}

	// log last estimated win probability
	G_LogPrintf("SkillRating: Win probability X/L: %.6f/%.6f\n", level.axisProb, level.alliesProb);

	G_UpdateSkillRating(winner);
}

/**
 * @brief Probability density function of standard normal distribution
 * @details Relative likelihood for a random variable x to take on a given value
 * @param[in] x
 * @return probability
 */
float pdf(float x)
{
	return exp(-0.5f * pow(x, 2)) / sqrtf(M_TAU_F);
}

/**
 * @brief Cumulative distribution function of standard normal distribution
 * @details Probability that a real-valued random variable with a given probability
 *          distribution will be found to have a value less than or equal to x
 * @param[in] x
 * @return probability
 */
float cdf(float x)
{
	return 0.5f * (1 + erff(x / sqrt(2)));
}

/**
 * @brief Mean additive truncated Gaussian function (non-draw version)
 * @details How much to update the mean after a win or loss
 * @param[in] t
 * @param[in] epsilon
 * @return update factor
 */
float V(float t, float epsilon)
{
	return pdf(t - epsilon) / cdf(t - epsilon);
}

/**
 * @brief Variance multiplicative function (non-draw version)
 * @details  How much to update the standard deviation after a win or loss
 * @param[in] t
 * @param[in] epsilon
 * @return update factor
 */
float W(float t, float epsilon)
{
	return V(t, epsilon) * (V(t, epsilon) + t - epsilon);
}

/**
 * @brief Map winning probability
 * @details Get wining parameter bias of the played map
 * @param[in] team
 * @return map win probability
 */
float G_MapWinProb(int team)
{
	if (!level.mapProb)
	{
		level.mapProb = 0.5f;
	}

	if (team == TEAM_AXIS)
	{
		return level.mapProb;
	}
	else
	{
		return 1.0f - level.mapProb;
	}
}

/**
 * @brief Update skill rating
 * @details Update player's skill rating based on team performance
 * @param[in] team
 */
void G_UpdateSkillRating(int winner)
{
	int          result;
	char         *err_msg = NULL;
	sqlite3_stmt *sqlstmt;
	srData_t     sr_data;

	int       i, playerTeam, rankFactor;
	float     c, v, w, t, winningMu, losingMu, muFactor, sigmaFactor;
	float     oldMu, oldSigma;
	gclient_t *cl;

	float teamMuX      = 0.f;
	float teamMuL      = 0.f;
	float teamSigmaSqX = 0.f;
	float teamSigmaSqL = 0.f;
	int   numPlayersX  = 0;
	int   numPlayersL  = 0;
	float mapProb      = 0.f;
	float mapMu        = 0.f;
	float mapSigma     = 0.f;
	float mapBeta      = 0.f;

	// total play time
	int totalTime = level.intermissionQueued - level.startTime - level.timeDelta;

	if (!level.database.initialized)
	{
		G_Printf("G_UpdateSkillRating: access to non-initialized database\n");
		return;
	}

	// map side parameter
	if (g_skillRating.integer > 1)
	{
		mapProb  = G_MapWinProb(winner);
		mapMu    = 2 * MU * mapProb;
		mapSigma = 2 * MU * sqrtf(mapProb * (1.0f - mapProb));
		mapBeta  = mapSigma / 2;
	}

	// player additive factors
	result = sqlite3_prepare(level.database.db, SRMATCH_SQLWRAP_TABLE, strlen(SRMATCH_SQLWRAP_TABLE), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_UpdateSkillRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return;
	}

	while (sqlite3_step(sqlstmt) == SQLITE_ROW)
	{
		// assign match data
		sr_data.mu          = sqlite3_column_double(sqlstmt, 1);
		sr_data.sigma       = sqlite3_column_double(sqlstmt, 2);
		sr_data.time_axis   = sqlite3_column_int(sqlstmt, 3);
		sr_data.time_allies = sqlite3_column_int(sqlstmt, 4);

		// player has not played at all
		if (sr_data.time_axis == 0 && sr_data.time_allies == 0)
		{
			continue;
		}

		// player has played in at least one of the team
		if (sr_data.time_axis > 0)
		{
			teamMuX      += sr_data.mu * (sr_data.time_axis / (float)totalTime);
			teamSigmaSqX += pow(sr_data.sigma, 2);
			numPlayersX++;
		}

		if (sr_data.time_allies > 0)
		{
			teamMuL      += sr_data.mu * (sr_data.time_allies / (float)totalTime);
			teamSigmaSqL += pow(sr_data.sigma, 2);
			numPlayersL++;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_UpdateSkillRating: sqlite3_finalize failed\n");
		return;
	}

	// normalizing constant
	if (g_skillRating.integer > 1)
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2) + pow(mapSigma, 2) + pow(mapBeta, 2));
	}
	else
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2));
	}

	// determine teams rank
	winningMu = (winner == TEAM_AXIS) ? teamMuX : teamMuL;
	losingMu  = (winner == TEAM_AXIS) ? teamMuL : teamMuX;

	// map bias
	if (g_skillRating.integer > 1)
	{
		if (mapProb > 0.5f)
		{
			winningMu += mapMu;
		}
		else if (mapProb < 0.5f)
		{
			losingMu += 2 * MU - mapMu;
		}
	}

	// team performance
	t = (winningMu - losingMu) / c;

	// truncated Gaussian correction
	v = V(t, EPSILON / c);
	w = W(t, EPSILON / c);

	// update players rating
	result = sqlite3_prepare(level.database.db, SRMATCH_SQLWRAP_TABLE, strlen(SRMATCH_SQLWRAP_TABLE), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_UpdateSkillRating: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return;
	}

	while (sqlite3_step(sqlstmt) == SQLITE_ROW)
	{
		// assign match data
		sr_data.guid        = sqlite3_column_text(sqlstmt, 0);
		sr_data.mu          = sqlite3_column_double(sqlstmt, 1);
		sr_data.sigma       = sqlite3_column_double(sqlstmt, 2);
		sr_data.time_axis   = sqlite3_column_int(sqlstmt, 3);
		sr_data.time_allies = sqlite3_column_int(sqlstmt, 4);

		// track old data
		oldMu    = sr_data.mu;
		oldSigma = sr_data.sigma;

		// player has not played at all
		if (sr_data.time_axis == 0 && sr_data.time_allies == 0)
		{
			continue;
		}

		// find which is team even when player has played on both side or has moved to spectator
		if (sr_data.time_axis - sr_data.time_allies > 0)
		{
			playerTeam = TEAM_AXIS;
		}
		else if (sr_data.time_allies - sr_data.time_axis > 0)
		{
			playerTeam = TEAM_ALLIES;
		}
		else
		{
			// player has played exact same time in each team
			continue;
		}

		// factors
		muFactor    = (pow(sr_data.sigma, 2) + pow(TAU, 2)) / c;
		sigmaFactor = (pow(sr_data.sigma, 2) + pow(TAU, 2)) / pow(c, 2);
		rankFactor  = (playerTeam == winner) ? 1 : -1;

		// rating update
		sr_data.mu    = sr_data.mu + rankFactor * muFactor * v * abs(sr_data.time_axis - sr_data.time_allies) / (float)totalTime;
		sr_data.sigma = sqrt((pow(sr_data.sigma, 2) + pow(TAU, 2)) * (1 - sigmaFactor * w));

		// save or update rating in rating_users table
		if (G_SkillRatingSetUserRating(&sr_data))
		{
			return;
		}

		G_LogPrintf("SkillRating: GUID: %s, Delta SR: %+.6f, SR: %.6f (%.6f, %.6f), Old SR: %.6f (%.6f, %.6f), Time X/L: %d/%d\n",
		            sr_data.guid,
		            (sr_data.mu - 3 * sr_data.sigma) - (oldMu - 3 * oldSigma),
		            sr_data.mu - 3 * sr_data.sigma, sr_data.mu, sr_data.sigma,
		            oldMu - 3 * oldSigma, oldMu, oldSigma,
		            sr_data.time_axis, sr_data.time_allies);
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_UpdateSkillRating: sqlite3_finalize failed\n");
		return;
	}

	// assign updated rating to connected players
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		G_SkillRatingGetClientRating(cl);

		// update rank
		G_CalcRank(cl);
		ClientUserinfoChanged(level.sortedClients[i]);
	}
}

/**
 * @brief Calculate win probability
 * @details Calculate win probability // Axis = winprob, Allies = 1.0 - winprob
 * @param[in] team
 * @return win probability
 */
float G_CalculateWinProbability(int team)
{
	int       i;
	float     c, t, winningMu, losingMu;
	gclient_t *cl;

	float teamMuX      = 0.f;
	float teamMuL      = 0.f;
	float teamSigmaSqX = 0.f;
	float teamSigmaSqL = 0.f;
	int   numPlayersX  = 0;
	int   numPlayersL  = 0;
	float mapProb      = 0.f;
	float mapMu        = 0.f;
	float mapSigma     = 0.f;
	float mapBeta      = 0.f;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return 0.5f;
	}

	// current play time
	int currentTime = level.timeCurrent - level.startTime - level.timeDelta;

	// map side parameter
	if (g_skillRating.integer > 1)
	{
		mapProb  = G_MapWinProb(team);
		mapMu    = 2 * MU * mapProb;
		mapSigma = 2 * MU * sqrtf(mapProb * (1.0f - mapProb));
		mapBeta  = mapSigma / 2;
	}

	// player additive factors (currently playing)
	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		if (g_gamestate.integer == GS_PLAYING)
		{
			// player has not played at all
			if (cl->sess.time_axis == 0 && cl->sess.time_allies == 0)
			{
				continue;
			}

			// player has played in at least one of the team
			if (cl->sess.time_axis > 0)
			{
				teamMuX      += cl->sess.mu * (cl->sess.time_axis / (float)currentTime);
				teamSigmaSqX += pow(cl->sess.sigma, 2);
				numPlayersX++;
			}
			if (cl->sess.time_allies > 0)
			{
				teamMuL      += cl->sess.mu * (cl->sess.time_allies / (float)currentTime);
				teamSigmaSqL += pow(cl->sess.sigma, 2);
				numPlayersL++;
			}
		}
		// warmup and intermission
		else
		{
			// avoid nan while players join teams
			if (level.numPlayingClients < 2)
			{
				return 0.5f;
			}

			// check actual team only
			if (cl->sess.sessionTeam == TEAM_AXIS)
			{
				teamMuX      += cl->sess.mu;
				teamSigmaSqX += pow(cl->sess.sigma, 2);
				numPlayersX++;
			}
			if (cl->sess.sessionTeam == TEAM_ALLIES)
			{
				teamMuL      += cl->sess.mu;
				teamSigmaSqL += pow(cl->sess.sigma, 2);
				numPlayersL++;
			}
		}
	}

	// player additive factors - take time of disconnected players into account
	if (g_gamestate.integer == GS_PLAYING)
	{
		int          result;
		char         *err_msg = NULL;
		sqlite3_stmt *sqlstmt;
		srData_t     sr_data;

		result = sqlite3_prepare(level.database.db, SRMATCH_SQLWRAP_TABLE, strlen(SRMATCH_SQLWRAP_TABLE), &sqlstmt, NULL);

		if (result != SQLITE_OK)
		{
			G_Printf("G_CalculateWinProbability: sqlite3_prepare failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 0.5f;
		}

		while (sqlite3_step(sqlstmt) == SQLITE_ROW)
		{
			// assign match data
			sr_data.guid        = sqlite3_column_text(sqlstmt, 0);
			sr_data.mu          = sqlite3_column_double(sqlstmt, 1);
			sr_data.sigma       = sqlite3_column_double(sqlstmt, 2);
			sr_data.time_axis   = sqlite3_column_int(sqlstmt, 3);
			sr_data.time_allies = sqlite3_column_int(sqlstmt, 4);

			// player has not played at all
			if (sr_data.time_axis == 0 && sr_data.time_allies == 0)
			{
				continue;
			}

			// player has reconnected
			qboolean isPlaying = qfalse;

			for (i = 0, cl = level.clients; i < level.maxclients; i++, cl++)
			{
				char userinfo[MAX_INFO_STRING];
				char *guid;

				trap_GetUserinfo(cl - level.clients, userinfo, sizeof(userinfo));
				guid = Info_ValueForKey(userinfo, "cl_guid");

				if (!Q_strncmp((const char *)sr_data.guid, guid, MAX_GUID_LENGTH + 1))
				{
					isPlaying = qtrue;
					break;
				}
			}

			if (isPlaying)
			{
				continue;
			}

			// player has played in at least one of the team
			if (sr_data.time_axis > 0)
			{
				teamMuX      += sr_data.mu * (sr_data.time_axis / (float)currentTime);
				teamSigmaSqX += pow(sr_data.sigma, 2);
				numPlayersX++;
			}

			if (sr_data.time_allies > 0)
			{
				teamMuL      += sr_data.mu * (sr_data.time_allies / (float)currentTime);
				teamSigmaSqL += pow(sr_data.sigma, 2);
				numPlayersL++;
			}
		}

		result = sqlite3_finalize(sqlstmt);

		if (result != SQLITE_OK)
		{
			G_Printf("G_CalculateWinProbability: sqlite3_finalize failed\n");
			return 0.5f;
		}
	}

	// normalizing constant
	if (g_skillRating.integer > 1)
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2) + pow(mapSigma, 2) + pow(mapBeta, 2));
	}
	else
	{
		c = sqrt(teamSigmaSqX + teamSigmaSqL + (numPlayersX + numPlayersL) * pow(BETA, 2));
	}

	// determine teams rank
	winningMu = (team == TEAM_AXIS) ? teamMuX : teamMuL;
	losingMu  = (team == TEAM_AXIS) ? teamMuL : teamMuX;

	// map bias
	if (g_skillRating.integer > 1)
	{
		if (mapProb > 0.5f)
		{
			winningMu += mapMu;
		}
		else if (mapProb < 0.5f)
		{
			losingMu += 2 * MU - mapMu;
		}
	}

	// team performance
	t = (winningMu - losingMu - EPSILON) / c;

	return cdf(t);
}

#endif
