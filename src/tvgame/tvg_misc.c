/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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
 * @file tvg_misc.c
 */

#include "tvg_local.h"
#include "tvg_lua.h"

/**
=================================================================================
TELEPORTERS
=================================================================================
*/

/**
 * @brief Teleport player to a given origin and angles
 * @param[in,out] player
 * @param[in] origin
 * @param[in] angles
 */
void TVG_TeleportPlayer(gclient_t *client, const vec3_t origin, const vec3_t angles)
{
	VectorCopy(origin, client->ps.origin);
	client->ps.origin[2] += 1;

	// toggle the teleport bit so the client knows to not lerp
	client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	TVG_SetClientViewAngle(client, angles);
}

/**
 * @brief System to temporarily ignore certain ents during traces
 *
 * @note Unused
 */
void G_InitTempTraceIgnoreEnts(void)
{
	Com_Memset(level.tempTraceIgnoreEnts, 0, sizeof(level.tempTraceIgnoreEnts));
}

/**
 * @brief G_ResetTempTraceIgnoreEnts
 */
void G_ResetTempTraceIgnoreEnts(void)
{
	int i;

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		if (level.tempTraceIgnoreEnts[i])
		{
			g_entities[i].r.linked = qtrue;

			level.tempTraceIgnoreEnts[i] = qfalse;
		}
	}
}

/**
 * @brief G_TempTraceIgnoreEntity
 * @param[in,out] ent
 */
void G_TempTraceIgnoreEntity(gentity_t *ent)
{
	if (!ent->r.linked)
	{
		return;
	}

	level.tempTraceIgnoreEnts[ent - g_entities] = qtrue;
	ent->r.linked                               = qfalse;
}

/**
 * @brief G_TeamTraceIgnoreBodies
 */
void G_TempTraceIgnoreBodies(void)
{
	int i;

	{
		// slower way - improve by time
		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			if (g_entities[i].s.eType == ET_CORPSE)
			{
				G_TempTraceIgnoreEntity(&g_entities[i]);
			}
		}
	}
}

/**
 * @brief G_TempTraceIgnorePlayersAndBodies
 */
void G_TempTraceIgnorePlayersAndBodies(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		G_TempTraceIgnoreEntity(&g_entities[i]);
	}

	G_TempTraceIgnoreBodies();
}

/**
 * @brief G_TempTraceIgnorePlayers
 */
void G_TempTraceIgnorePlayers(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		G_TempTraceIgnoreEntity(&g_entities[i]);
	}
}

/**
 * @brief G_TempTraceIgnorePlayersAndBodiesFromTeam
 * @param[in] team
 */
void G_TempTraceIgnorePlayersFromTeam(team_t team)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (g_entities[i].client && g_entities[i].client->sess.sessionTeam == team)
		{
			G_TempTraceIgnoreEntity(&g_entities[i]);
		}
	}
}

/**
* @brief G_TempTraceIgnoreEntities
* @param[in] ent
*/
void G_TempTraceIgnoreEntities(gentity_t *ent)
{
	int           i;
	int           listLength;
	int           list[MAX_GENTITIES];
	gentity_t     *hit;
	vec3_t        BBmins, BBmaxs;
	static vec3_t range = { CH_BREAKABLE_DIST, CH_BREAKABLE_DIST, CH_BREAKABLE_DIST };

	if (!ent->client)
	{
		return;
	}

	VectorSubtract(ent->client->ps.origin, range, BBmins);
	VectorAdd(ent->client->ps.origin, range, BBmaxs);

	listLength = trap_EntitiesInBox(BBmins, BBmaxs, list, MAX_GENTITIES);

	for (i = 0; i < listLength; i++)
	{
		hit = &g_entities[list[i]];

		if (hit->s.eType == ET_OID_TRIGGER || hit->s.eType == ET_TRIGGER_MULTIPLE
		    || hit->s.eType == ET_TRIGGER_FLAGONLY || hit->s.eType == ET_TRIGGER_FLAGONLY_MULTIPLE)
		{
			G_TempTraceIgnoreEntity(hit);
		}

		if (hit->s.eType == ET_CORPSE && !(ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS))
		{
			G_TempTraceIgnoreEntity(hit);
		}

		if (hit->client && (!(ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC) || (ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC && ent->client->sess.sessionTeam != hit->client->sess.sessionTeam))
		    && hit->client->ps.pm_type == PM_DEAD && !(hit->client->ps.pm_flags & PMF_LIMBO))
		{
			G_TempTraceIgnoreEntity(hit);
		}
	}
}

/**
 * @brief ClientName
 * @param[in] client
 * @param[out] name
 * @param[in] size
 * @return client name
 */
char *ClientName(int client, char *name, size_t size)
{
	char buf[MAX_INFO_STRING];

	if (client < 0 || client >= MAX_CLIENTS)
	{
		G_Printf("^1ClientName: client out of range\n");
		return "[client out of range]";
	}
	trap_GetConfigstring(CS_PLAYERS + client, buf, sizeof(buf));
	Q_strncpyz(name, Info_ValueForKey(buf, "n"), size);
	Q_CleanStr(name);

	return name;
}
