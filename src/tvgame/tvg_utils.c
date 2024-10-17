/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
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
 * @file tvg_utils.c
 * @brief Misc utility functions for tvgame module
 */

#include "tvg_local.h"

/**
=========================================================================
model / sound configstring indexes
=========================================================================
*/

/**
 * @brief TVG_FindConfigstringIndex
 * @param[in] name
 * @param[in] start
 * @param[in] max
 * @param[in] create
 * @return
 */
int TVG_FindConfigstringIndex(const char *name, int start, int max, qboolean create)
{
	int  i;
	char s[MAX_STRING_CHARS];

	if (!name || !name[0])
	{
		return 0;
	}

	for (i = 1 ; i < max ; i++)
	{
		trap_GetConfigstring(start + i, s, sizeof(s));
		if (!s[0])
		{
			break;
		}
		if (!strcmp(s, name))
		{
			return i;
		}
	}

	if (!create)
	{
		return 0;
	}

	if (i == max)
	{
		G_Error("TVG_FindConfigstringIndex: overflow '%s' (%i %i) max: %i\n", name, start, start + i, max);
	}

	trap_SetConfigstring(start + i, name);

	return i;
}

/**
 * @brief Prevent player always mounting the last gun used, on multiple tank maps.
 * Ported from the Bugfix project (#087)
 *
 * @param[in] name
 * @param[in] start
 * @param[in] max
 */
void TVG_RemoveConfigstringIndex(const char *name, int start, int max)
{
	int  i, j;
	char s[MAX_STRING_CHARS];

	if (!name || !name[0])
	{
		return;
	}

	for (i = 1; i < max; i++)
	{
		trap_GetConfigstring(start + i, s, sizeof(s));

		if (!s[0])
		{
			break;
		}

		if (strcmp(s, name) == 0)
		{
			trap_SetConfigstring(start + i, "");
			for (j = i + 1; j < max - 1; j++)
			{
				trap_GetConfigstring(start + j, s, sizeof(s));
				trap_SetConfigstring(start + j, "");
				trap_SetConfigstring(start + i, s);

			}
			break;
		}
	}
}

/**
 * @brief TVG_ModelIndex
 * @param[in] name
 * @return
 */
int TVG_ModelIndex(const char *name)
{
	return TVG_FindConfigstringIndex(name, CS_MODELS, MAX_MODELS, qtrue);
}

/**
 * @brief TVG_SoundIndex
 * @param[in] name
 * @return
 */
int TVG_SoundIndex(const char *name)
{
	return TVG_FindConfigstringIndex(name, CS_SOUNDS, MAX_SOUNDS, qtrue) + GAMESOUND_MAX;
}

/**
 * @brief TVG_SkinIndex
 * @param[in] name
 * @return
 */
int TVG_SkinIndex(const char *name)
{
	return TVG_FindConfigstringIndex(name, CS_SKINS, MAX_CS_SKINS, qtrue);
}

/**
 * @brief TVG_ShaderIndex
 * @param[in] name
 * @return
 */
int TVG_ShaderIndex(const char *name)
{
	return TVG_FindConfigstringIndex(name, CS_SHADERS, MAX_CS_SHADERS, qtrue);
}

/**
 * @brief TVG_CharacterIndex
 * @param[in] name
 * @return
 *
 * @note Unused
 */
int TVG_CharacterIndex(const char *name)
{
	return TVG_FindConfigstringIndex(name, CS_CHARACTERS, MAX_CHARACTERS, qtrue);
}

/**
 * @brief TVG_StringIndex
 * @param[in] string
 * @return
 */
int TVG_StringIndex(const char *string)
{
	return TVG_FindConfigstringIndex(string, CS_STRINGS, MAX_CSSTRINGS, qtrue);
}

//=====================================================================

/**
 * @brief Broadcasts a command to only a specific team
 * @param[in] team
 * @param[in] cmd
 */
void TVG_TeamCommand(team_t team, const char *cmd)
{
	int i;

	for (i = 0 ; i < level.maxclients ; i++)
	{
		if (level.clients[i].pers.connected == CON_CONNECTED)
		{
			if (level.clients[i].sess.sessionTeam == team)
			{
				trap_SendServerCommand(i, va("%s", cmd));
			}
		}
	}
}

/**
 * @brief Searches all active entities for the next one that holds
 * the matching string at fieldofs (use the FOFS() macro) in the structure.
 * Searches beginning at the entity after from, or the beginning if NULL
 * NULL will be returned if the end of the list is reached.
 *
 * @param[in,out] from
 * @param[in] fieldofs
 * @param[in] match
 * @return
 */
gentity_t *TVG_Find(gentity_t *from, size_t fieldofs, const char *match)
{
	char      *s;
	gentity_t *max = &g_entities[level.num_entities];

	if (!from)
	{
		from = g_entities;
	}
	else
	{
		from++;
	}

	for ( ; from < max ; from++)
	{
		if (!from->inuse)
		{
			continue;
		}
		s = *( char ** )((byte *)from + fieldofs);
		if (!s)
		{
			continue;
		}
		if (!Q_stricmp(s, match))
		{
			return from;
		}
	}

	return NULL;
}

/**
 * @brief TVG_FindByTargetname
 * @param[in,out] from
 * @param[in] match
 * @return
 */
gentity_t *TVG_FindByTargetname(gentity_t *from, const char *match)
{
	gentity_t *max = &g_entities[level.num_entities];
	int       hash;

	hash = TVG_StringHashValue(match);

	if (hash == -1) // if there is no name (not empty string!) TVG_StringHashValue returns -1
	{
		G_Printf("TVG_FindByTargetname WARNING: invalid match pointer '%s' - run devmap & g_scriptdebug 1 to get more info about\n", match);
		return NULL;
	}

	if (!from)
	{
		from = g_entities;
	}
	else
	{
		from++;
	}

	for ( ; from < max ; from++)
	{
		if (!from->inuse)
		{
			continue;
		}

		if (!from->targetname) // there are ents with no targetname set
		{
			continue;
		}

		if (from->targetnamehash == hash && !Q_stricmp(from->targetname, match))
		{
			return from;
		}
	}

	return NULL;
}

#define MAXCHOICES  32

/**
 * @brief Selects a random entity from among the targets
 * @param[in] targetname
 * @return
 */
gentity_t *TVG_PickTarget(const char *targetname)
{
	gentity_t *ent        = NULL;
	int       num_choices = 0;
	gentity_t *choice[MAXCHOICES];

	if (!targetname)
	{
		//G_Printf("TVG_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while (1)
	{
		ent = TVG_FindByTargetname(ent, targetname);
		if (!ent)
		{
			break;
		}
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
		{
			break;
		}
	}

	if (!num_choices)
	{
		G_Printf(S_COLOR_YELLOW "WARNING TVG_PickTarget: target %s not found or isn't in use - this might be a bug (returning NULL)\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

/**
 * @brief This is just a convenience function for printing vectors
 * @param[in] v
 * @return
 */
char *TVG_VecToStr(const vec3_t v)
{
	static int  index;
	static char str[8][32];
	char        *s;

	// use an array so that multiple vtos won't collide
	s     = str[index];
	index = (index + 1) & 7;

	Com_sprintf(s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}

/**
 * @brief TVG_InitGentity
 * @param[in,out] e
 */
void TVG_InitGentity(gentity_t *e)
{
	e->inuse      = qtrue;
	e->classname  = "noclass";
	e->s.number   = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
	e->free       = NULL;

	// mark the time
	e->spawnTime = level.time;
}

/**
 * @brief Either finds a free entity, or allocates a new one.
 *
 * @details The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
 * never be used by anything else.
 *
 * Try to avoid reusing an entity that was recently freed, because it
 * can cause the client to think the entity morphed into something else
 * instead of being removed and recreated, which can cause interpolated
 * angles and bad trails.
 *
 * @return
 */
gentity_t *TVG_Spawn(void)
{
	int       i = 0, force;
	gentity_t *e = NULL;

	for (force = 0 ; force < 2 ; force++)
	{
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[0];
		for (i = 0; i < level.num_entities ; i++, e++)
		{
			if (e->inuse)
			{
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			// FIXME: inspect -> add '&& level.startTime != 0' for warmup?
			if (!force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000)
			{
				continue;
			}

			// reuse this slot
			TVG_InitGentity(e);
			return e;
		}
		if (i != ENTITYNUM_MAX_NORMAL)
		{
			break;
		}
	}

	if (i == ENTITYNUM_MAX_NORMAL)
	{
		for (i = 0; i < MAX_GENTITIES; i++)
		{
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		G_Error("G_Spawn: no free entities\n");
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t),
	                    &level.clients[0].ps, sizeof(level.clients[0]));

	TVG_InitGentity(e);
	return e;
}

/**
 * @brief Marks the entity as free
 *
 * @param[in,out] ent
 */
void TVG_FreeEntity(gentity_t *ent)
{
	if (ent->free)
	{
		ent->free(ent);
	}

	trap_UnlinkEntity(ent);       // unlink from world

	// this tiny hack fixes level.num_entities rapidly reaching MAX_GENTITIES-1
	// some very often spawned entities don't have to relax (=spawned, immediately freed and not transmitted)
	// before all game entities did relax - now  ET_TEMPHEAD, ET_TEMPLEGS and ET_EVENTS no longer relax
	// - fix: ET_TEMP* entities are linked for a short amount of time but have no ent->r.svFlags set
	// - optimization: if events are freed EVENT_VALID_MSEC has already passed (keep in mind these are broadcasted)
	// - when enabled g_debugHitboxes, g_debugPlayerHitboxes or g_debugbullets 3 we want visible trace effects - don't free immediately
	// FIXME: remove tmp var l_free if we are sure there are no issues caused by this change (especially on network games)
	if (ent->s.eType == ET_CORPSE || ent->s.eType >= ET_EVENTS)
	{
		G_DPrintf("^2%4i entity freed - num_entities: %4i - %s\n", (int)(ent - g_entities), level.num_entities, ent->classname);

		// game entity is immediately available and a 'slot' will be reused
		Com_Memset(ent, 0, sizeof(*ent));
		ent->classname = "freed";
		ent->freetime  = -9999;  // e->freetime is never greater than level.startTime + 2000 see G_Spawn()
		ent->inuse     = qfalse;
	}
	else // all other game entities relax
	{
		Com_Memset(ent, 0, sizeof(*ent));
		ent->classname = "freed";
		ent->freetime  = level.time;
		ent->inuse     = qfalse;
	}
}

//==============================================================================

/**
 * @brief Adds an event+parm and twiddles the event counter
 * @param[in,out] client
 * @param[in] event
 * @param[in] eventParm
 */
void TVG_AddEvent(gclient_t *client, int event, int eventParm)
{
	if (!event)
	{
		G_Printf(S_COLOR_YELLOW "WARNING G_AddEvent: zero event added for client %i\n", (int)(client - level.clients));
		return;
	}

	// use the sequential event list
	if (client)
	{
		// commented in - externalEvents not being handled properly in Wolf right now
		client->ps.events[client->ps.eventSequence & (MAX_EVENTS - 1)]     = event;
		client->ps.eventParms[client->ps.eventSequence & (MAX_EVENTS - 1)] = eventParm;
		client->ps.eventSequence++;
	}
}

/**
 * @brief TVG_AnimScriptSound
 * @param[in] soundIndex
 * @param org - unused
 * @param[in] client
 */
void TVG_AnimScriptSound(int soundIndex, vec3_t org, int client)
{
	gclient_t *cl = &level.clients[client];

	TVG_AddEvent(cl, EV_GENERAL_SOUND, soundIndex);
}

//==============================================================================

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
* @brief TVG_ResetTempTraceIgnoreEnts
*/
void TVG_ResetTempTraceIgnoreEnts(void)
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
* @brief TVG_TempTraceIgnoreEntity
* @param[in,out] ent
*/
void TVG_TempTraceIgnoreEntity(gentity_t *ent)
{
	if (!ent->r.linked)
	{
		return;
	}

	level.tempTraceIgnoreEnts[ent - g_entities] = qtrue;
	ent->r.linked                               = qfalse;
}

/**
* @brief TVG_TeamTraceIgnoreBodies
*/
void TVG_TempTraceIgnoreBodies(void)
{
	int i;

	{
		// slower way - improve by time
		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			if (g_entities[i].s.eType == ET_CORPSE)
			{
				TVG_TempTraceIgnoreEntity(&g_entities[i]);
			}
		}
	}
}

/**
* @brief TVG_TempTraceIgnorePlayersAndBodies
*/
void TVG_TempTraceIgnorePlayersAndBodies(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		TVG_TempTraceIgnoreEntity(&g_entities[i]);
	}

	TVG_TempTraceIgnoreBodies();
}

/**
* @brief TVG_TempTraceIgnorePlayers
*/
void TVG_TempTraceIgnorePlayers(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		TVG_TempTraceIgnoreEntity(&g_entities[i]);
	}
}

/**
* @brief TVG_TempTraceIgnorePlayersAndBodiesFromTeam
* @param[in] team
*/
void TVG_TempTraceIgnorePlayersFromTeam(team_t team)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (g_entities[i].s.teamNum == team)
		{
			TVG_TempTraceIgnoreEntity(&g_entities[i]);
		}
	}
}

/**
 * @brief Generate standard naming
 * @return
 */
char *TVG_GenerateFilename(void)
{
	static char fullFilename[MAX_OSPATH];
	qtime_t     ct;

	trap_RealTime(&ct);
	fullFilename[0] = '\0';

	Com_sprintf(fullFilename, sizeof(fullFilename), "%d-%02d-%02d-%02d%02d%02d-%s",
	            1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday,
	            ct.tm_hour, ct.tm_min, ct.tm_sec,
	            level.rawmapname
	            );

	return fullFilename;
}

/**
 * @brief TVG_StringHashValue
 * @param[in] fname
 * @return A hash value for the given string
 */
long TVG_StringHashValue(const char *fname)
{
	int  i;
	long hash;

	if (!fname)
	{
		return -1;
	}

	hash = 0;
	i    = 0;
	while (fname[i] != '\0')
	{
		if (Q_isupper(fname[i]))
		{
			hash += (long)(fname[i] + ('a' - 'A')) * (i + 119);
		}
		else
		{
			hash += (long)(fname[i]) * (i + 119);
		}

		i++;
	}
	if (hash == -1)
	{
		hash = 0;   // never return -1
		Com_Printf("TVG_StringHashValue WARNING: fname with empty string returning 0");
	}
	return hash;
}

char bigTextBuffer[100000];
