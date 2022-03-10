/*
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
 */
/**
 * @file g_prestige.c
 * @brief Prestige tracking
 */
#ifdef FEATURE_PRESTIGE

#include "g_local.h"

#ifdef FEATURE_DBMS
#include <sqlite3.h>
#endif

#define PRCHECK_SQLWRAP_TABLES "SELECT * FROM prestige_users;"
#define PRCHECK_SQLWRAP_SCHEMA "SELECT guid, prestige, streak, skill0, skill1, skill2, skill3, skill4, skill5, skill6, created, updated FROM prestige_users;"
#define PRUSERS_SQLWRAP_SELECT "SELECT * FROM prestige_users WHERE guid = '%s';"
#define PRUSERS_SQLWRAP_INSERT "INSERT INTO prestige_users " \
	                           "(guid, prestige, streak, skill0, skill1, skill2, skill3, skill4, skill5, skill6, created, updated) VALUES ('%s', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);"
#define PRUSERS_SQLWRAP_UPDATE "UPDATE prestige_users SET prestige = '%i', streak = '%i', skill0 = '%i', skill1 = '%i', skill2 = '%i', skill3 = '%i', skill4 = '%i', skill5 = '%i', skill6 = '%i', updated = CURRENT_TIMESTAMP WHERE guid = '%s';"

/**
 * @brief Checks if database exists, if tables exist and if schemas are correct
 * @param[in] db_path
 * @param[in] db_mode
 * @return 0 if database check is successful, 1 otherwise.
 */
int G_PrestigeDBCheck(char *db_path, int db_mode)
{
	int     result;
	sqlite3 *db;

	if (!db_path || db_path[0] == '\0')
	{
		G_Printf("G_PrestigeDBCheck: invalid path specified\n");
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
		G_Printf("G_PrestigeDBCheck: sqlite3_open_v2 failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	// check if tables exist
	result = sqlite3_exec(db, PRCHECK_SQLWRAP_TABLES, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_PrestigeDBCheck: sqlite3_exec PRCHECK_SQLWRAP_TABLES failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_PrestigeDBCheck: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// check schema
	result = sqlite3_exec(db, PRCHECK_SQLWRAP_SCHEMA, NULL, NULL, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_PrestigeDBCheck: sqlite3_exec PRCHECK_SQLWRAP_SCHEMA failed: %s\n", sqlite3_errstr(result));

		result = sqlite3_close(db);

		if (result != SQLITE_OK)
		{
			G_Printf("G_PrestigeDBCheck: sqlite3_close failed: %s\n", sqlite3_errstr(result));
			return 1;
		}
		return 1;
	}

	// all ok
	result = sqlite3_close(db);

	if (result != SQLITE_OK)
	{
		G_Printf("G_PrestigeDBCheck: sqlite3_close failed: %s\n", sqlite3_errstr(result));
		return 1;
	}

	return 0;
}

/**
 * @brief Retrieve prestige for client
 *         Called on ClientConnect
 * @param[in] cl
 */
void G_GetClientPrestige(gclient_t *cl)
{
	char      userinfo[MAX_INFO_STRING];
	char      *guid;
	int       clientNum, i;
	prData_t  pr_data;
	gentity_t *ent;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_CAMPAIGN || g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (!level.database.initialized)
	{
		G_Printf("G_GetClientPrestige: access to non-initialized database\n");
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
	pr_data.guid = (const unsigned char *)guid;

	// retrieve current prestige or assign default values
	if (G_ReadPrestige(&pr_data))
	{
		return;
	}

	// assign user data to session
	cl->sess.prestige     = pr_data.prestige;
	cl->sess.startxptotal = 0;

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		cl->sess.skillpoints[i]      = pr_data.skillpoints[i];
		cl->sess.startskillpoints[i] = pr_data.skillpoints[i];
		cl->sess.startxptotal       += pr_data.skillpoints[i];
	}
}

/**
 * @brief Sets or updates prestige and timestamp for client
 *         Called on ClientDisconnect and on G_LogExit before intermissionQueued
 * @param[in] cl
 */
void G_SetClientPrestige(gclient_t *cl, qboolean streakUp)
{
	char      userinfo[MAX_INFO_STRING];
	char      *guid;
	int       clientNum, i, j, skillMax, cnt = 0;
	prData_t  pr_data;
	gentity_t *ent;
	qboolean  hasMapXPs = qfalse;

	// disable for these game types
	if (g_gametype.integer == GT_WOLF_CAMPAIGN || g_gametype.integer == GT_WOLF_STOPWATCH || g_gametype.integer == GT_WOLF_LMS)
	{
		return;
	}

	if (!level.database.initialized)
	{
		G_Printf("G_SetClientPrestige: access to non-initialized database\n");
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

	pr_data.guid = (const unsigned char *)guid;

	// count the number of maxed out skills
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		skillMax = 0;

		// check skill max level
		for (j = NUM_SKILL_LEVELS - 1; j >= 0; j--)
		{
			if (GetSkillTableData(i)->skillLevels[j] >= 0)
			{
				skillMax = j;
				break;
			}
		}

		if (cl->sess.skill[i] >= skillMax)
		{
			cnt++;
		}
	}

	// retrieve current streak or assign default values
	if (G_ReadPrestige(&pr_data))
	{
		return;
	}

	// increase streak if all skills are maxed out
	if (cnt >= SK_NUM_SKILLS && streakUp)
	{
		pr_data.streak += 1;
	}

	// prestige button clicked in intermission
	if (!level.intermissionQueued && level.intermissiontime)
	{
		if (cnt < SK_NUM_SKILLS)
		{
			return;
		}

		// increase prestige and reset skill points
		cl->sess.prestige = cl->sess.prestige + 1;

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			cl->sess.skillpoints[i] = 0;
		}

		// reset skills and starting points for correct debriefing display
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			cl->sess.skill[i]            = 0;
			cl->sess.startskillpoints[i] = 0;
		}

		// reset streak
		pr_data.streak = 0;
	}

	// assign match data
	pr_data.prestige = cl->sess.prestige;

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		pr_data.skillpoints[i] = (int)cl->sess.skillpoints[i];

		// check for new points this map
		if (!hasMapXPs && (cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) != 0.f) // Skillpoints can be negative
		{
			hasMapXPs = qtrue;
		}
	}

	// player has not collected any new point and can't collect
	if (!hasMapXPs && cnt < SK_NUM_SKILLS)
	{
		return;
	}

	// save or update prestige
	if (G_WritePrestige(&pr_data))
	{
		return;
	}
}

/**
 * @brief Retrieve prestige from the prestige_users table
 * @param[in] pr_data
 * @return 0 if successful, 1 otherwise.
 */
int G_ReadPrestige(prData_t *pr_data)
{
	int          result, i;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_ReadPrestige: access to non-initialized database\n");
		return 1;
	}

	sql = va(PRUSERS_SQLWRAP_SELECT, pr_data->guid);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_ReadPrestige: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_ROW)
	{
		// assign prestige data
		pr_data->prestige = sqlite3_column_int(sqlstmt, 1);
		pr_data->streak   = sqlite3_column_int(sqlstmt, 2);

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			pr_data->skillpoints[i] = sqlite3_column_int(sqlstmt, i + 3);
		}
	}
	else
	{
		// no entry found or other failure
		if (result == SQLITE_DONE)
		{
			// assign default values
			pr_data->prestige = 0;
			pr_data->streak   = 0;

			for (i = 0; i < SK_NUM_SKILLS; i++)
			{
				pr_data->skillpoints[i] = 0;
			}
		}
		else
		{
			sqlite3_finalize(sqlstmt);

			G_Printf("G_ReadPrestige: sqlite3_step failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_ReadPrestige: sqlite3_finalize failed\n");
		return 1;
	}

	return 0;
}

/**
 * @brief Sets or updates skills and prestige points
 * @param[in] pr_data
 * @return 0 if successful, 1 otherwise.
 */
int G_WritePrestige(prData_t *pr_data)
{
	int          result;
	char         *err_msg = NULL;
	char         *sql;
	sqlite3_stmt *sqlstmt;

	if (!level.database.initialized)
	{
		G_Printf("G_WritePrestige: access to non-initialized database\n");
		return 1;
	}

	sql = va(PRUSERS_SQLWRAP_SELECT, pr_data->guid);

	result = sqlite3_prepare(level.database.db, sql, strlen(sql), &sqlstmt, NULL);

	if (result != SQLITE_OK)
	{
		G_Printf("G_WritePrestige: sqlite3_prepare failed: %s\n", err_msg);
		sqlite3_free(err_msg);
		return 1;
	}

	result = sqlite3_step(sqlstmt);

	if (result == SQLITE_DONE)
	{
		sql = va(PRUSERS_SQLWRAP_INSERT,
		         pr_data->guid,
		         pr_data->prestige,
		         pr_data->streak,
		         pr_data->skillpoints[0],
		         pr_data->skillpoints[1],
		         pr_data->skillpoints[2],
		         pr_data->skillpoints[3],
		         pr_data->skillpoints[4],
		         pr_data->skillpoints[5],
		         pr_data->skillpoints[6]);

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_WritePrestige: sqlite3_exec:INSERT failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}
	else
	{
		sql = va(PRUSERS_SQLWRAP_UPDATE,
		         pr_data->prestige,
		         pr_data->streak,
		         (int)pr_data->skillpoints[0],
		         (int)pr_data->skillpoints[1],
		         (int)pr_data->skillpoints[2],
		         (int)pr_data->skillpoints[3],
		         (int)pr_data->skillpoints[4],
		         (int)pr_data->skillpoints[5],
		         (int)pr_data->skillpoints[6],
		         pr_data->guid);

		result = sqlite3_exec(level.database.db, sql, NULL, NULL, &err_msg);

		if (result != SQLITE_OK)
		{
			G_Printf("G_WritePrestige: sqlite3_exec:UPDATE failed: %s\n", err_msg);
			sqlite3_free(err_msg);
			return 1;
		}
	}

	result = sqlite3_finalize(sqlstmt);

	if (result != SQLITE_OK)
	{
		G_Printf("G_WritePrestige: sqlite3_finalize failed\n");
		return 1;
	}

	return 0;
}

#endif
