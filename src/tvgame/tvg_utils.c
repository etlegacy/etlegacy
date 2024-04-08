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
 * @brief G_FindConfigstringIndex
 * @param[in] name
 * @param[in] start
 * @param[in] max
 * @param[in] create
 * @return
 */
int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create)
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
		G_Error("G_FindConfigstringIndex: overflow '%s' (%i %i) max: %i\n", name, start, start + i, max);
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
void G_RemoveConfigstringIndex(const char *name, int start, int max)
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
 * @brief G_ModelIndex
 * @param[in] name
 * @return
 */
int G_ModelIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_MODELS, MAX_MODELS, qtrue);
}

/**
 * @brief G_SoundIndex
 * @param[in] name
 * @return
 */
int G_SoundIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_SOUNDS, MAX_SOUNDS, qtrue) + GAMESOUND_MAX;
}

/**
 * @brief G_SkinIndex
 * @param[in] name
 * @return
 */
int G_SkinIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_SKINS, MAX_CS_SKINS, qtrue);
}

/**
 * @brief G_ShaderIndex
 * @param[in] name
 * @return
 */
int G_ShaderIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_SHADERS, MAX_CS_SHADERS, qtrue);
}

/**
 * @brief G_CharacterIndex
 * @param[in] name
 * @return
 *
 * @note Unused
 */
int G_CharacterIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_CHARACTERS, MAX_CHARACTERS, qtrue);
}

/**
 * @brief G_StringIndex
 * @param[in] string
 * @return
 */
int G_StringIndex(const char *string)
{
	return G_FindConfigstringIndex(string, CS_STRINGS, MAX_CSSTRINGS, qtrue);
}

//=====================================================================

/**
 * @brief Broadcasts a command to only a specific team
 * @param[in] team
 * @param[in] cmd
 */
void G_TeamCommand(team_t team, const char *cmd)
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
gentity_t *G_Find(gentity_t *from, int fieldofs, const char *match)
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
 * @brief Like G_Find, but searches for integer values.
 * @param[in,out] from
 * @param[in] fieldofs
 * @param[in] match
 * @return
 */
gentity_t *G_FindInt(gentity_t *from, int fieldofs, int match)
{
	int       i;
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
		i = *(int *) ((byte *)from + fieldofs);
		if (i == match)
		{
			return from;
		}
	}

	return NULL;
}

/**
 * @brief Like G_Find, but searches for float values..
 * @param[in,out] from
 * @param[in] fieldofs
 * @param[in] match
 * @return
 */
gentity_t *G_FindFloat(gentity_t *from, int fieldofs, float match)
{
	float     f;
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
		f = *(float *) ((byte *)from + fieldofs);
		if (f == match)
		{
			return from;
		}
	}

	return NULL;
}

/**
 * @brief Like G_Find, but searches for vector values..
 * @param[in,out] from
 * @param[in] fieldofs
 * @param[in] match
 * @return
 */
gentity_t *G_FindVector(gentity_t *from, int fieldofs, const vec3_t match)
{
	vec3_t    vec;
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
		vec[0] = *(vec_t *) ((byte *)from + fieldofs + 0);
		vec[1] = *(vec_t *) ((byte *)from + fieldofs + 4);
		vec[2] = *(vec_t *) ((byte *)from + fieldofs + 8);

		if (vec[0] == match[0] && vec[1] == match[1] && vec[2] == match[2])
		{
			return from;
		}
	}

	return NULL;
}


/**
 * @brief G_FindByTargetname
 * @param[in,out] from
 * @param[in] match
 * @return
 */
gentity_t *G_FindByTargetname(gentity_t *from, const char *match)
{
	gentity_t *max = &g_entities[level.num_entities];
	int       hash;

	hash = BG_StringHashValue(match);

	if (hash == -1) // if there is no name (not empty string!) BG_StringHashValue returns -1
	{
		G_Printf("G_FindByTargetname WARNING: invalid match pointer '%s' - run devmap & g_scriptdebug 1 to get more info about\n", match);
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

/**
 * @brief This version should be used for loops, saves the constant hash building
 * @param[in,out] from
 * @param[in] match
 * @param[in] hash
 * @return
 */
gentity_t *G_FindByTargetnameFast(gentity_t *from, const char *match, int hash)
{
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
gentity_t *G_PickTarget(const char *targetname)
{
	gentity_t *ent        = NULL;
	int       num_choices = 0;
	gentity_t *choice[MAXCHOICES];

	if (!targetname)
	{
		//G_Printf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while (1)
	{
		ent = G_FindByTargetname(ent, targetname);
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
		G_Printf(S_COLOR_YELLOW "WARNING G_PickTarget: target %s not found or isn't in use - this might be a bug (returning NULL)\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

/**
 * @brief G_AllowTeamsAllowed
 * @param[in] ent
 * @param[in] activator
 * @return
 */
qboolean G_AllowTeamsAllowed(gentity_t *ent, gentity_t *activator)
{
	if (ent->allowteams && activator && activator->client)
	{
		if (activator->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			int checkTeam = activator->client->sess.sessionTeam;

			if (!(ent->allowteams & checkTeam))
			{
				if ((ent->allowteams & ALLOW_DISGUISED_CVOPS) && activator->client->ps.powerups[PW_OPS_DISGUISED])
				{
					if (checkTeam == TEAM_AXIS)
					{
						checkTeam = TEAM_ALLIES;
					}
					else if (checkTeam == TEAM_ALLIES)
					{
						checkTeam = TEAM_AXIS;
					}
				}

				if (!(ent->allowteams & checkTeam))
				{
					return qfalse;
				}
			}
		}
	}

	return qtrue;
}

/**
 * @brief Added to allow more checking on what uses what
 * @param[in,out] ent
 * @param[in] other
 * @param[in] activator
 */
void G_UseEntity(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	// check for allowteams
	if (!G_AllowTeamsAllowed(ent, activator))
	{
		return;
	}

	// Woop we got through, let's use the entity
	ent->use(ent, other, activator);
}

/**
 * @brief This is just a convenience function for printing vectors
 * @param[in] v
 * @return
 */
char *vtos(const vec3_t v)
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
 * @brief The editor only specifies a single value for angles (yaw),
 * but we have special constants to generate an up or down direction.
 * Angles will be cleared, because it is being used to represent a direction
 * instead of an orientation.
 *
 * @param[in,out] angles
 * @param[out] movedir
 */
void G_SetMovedir(vec3_t angles, vec3_t movedir)
{
	static vec3_t VEC_UP       = { 0, -1, 0 };
	static vec3_t MOVEDIR_UP   = { 0, 0, 1 };
	static vec3_t VEC_DOWN     = { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

	if (VectorCompare(angles, VEC_UP))
	{
		VectorCopy(MOVEDIR_UP, movedir);
	}
	else if (VectorCompare(angles, VEC_DOWN))
	{
		VectorCopy(MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors(angles, movedir, NULL, NULL);
	}
	VectorClear(angles);
}

/**
 * @brief G_InitGentity
 * @param[in,out] e
 */
void G_InitGentity(gentity_t *e)
{
	e->inuse      = qtrue;
	e->classname  = "noclass";
	e->s.number   = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
	e->nextthink  = 0;
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
gentity_t *G_Spawn(void)
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
			G_InitGentity(e);
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

	G_InitGentity(e);
	return e;
}

/**
 * @brief Marks the entity as free
 *
 * @param[in,out] ent
 */
void G_FreeEntity(gentity_t *ent)
{
	if (ent->free)
	{
		ent->free(ent);
	}

	trap_UnlinkEntity(ent);       // unlink from world

	if (ent->neverFree)
	{
		return;
	}

	// this tiny hack fixes level.num_entities rapidly reaching MAX_GENTITIES-1
	// some very often spawned entities don't have to relax (=spawned, immediately freed and not transmitted)
	// before all game entities did relax - now  ET_TEMPHEAD, ET_TEMPLEGS and ET_EVENTS no longer relax
	// - fix: ET_TEMP* entities are linked for a short amount of time but have no ent->r.svFlags set
	// - optimization: if events are freed EVENT_VALID_MSEC has already passed (keep in mind these are broadcasted)
	// - when enabled g_debugHitboxes, g_debugPlayerHitboxes or g_debugbullets 3 we want visible trace effects - don't free immediately
	// FIXME: remove tmp var l_free if we are sure there are no issues caused by this change (especially on network games)
	if (ent->s.eType == ET_TEMPHEAD || ent->s.eType == ET_TEMPLEGS || ent->s.eType == ET_CORPSE || ent->s.eType >= ET_EVENTS)
	{
		// debug
		if (g_developer.integer)
		{
			if (ent->s.eType >= ET_EVENTS)
			{
				G_DPrintf("^3%4i event entity freed - num_entities: %4i - %s [%s]\n", (int)(ent - g_entities), level.num_entities, ent->classname, eventnames[ent->s.eType - ET_EVENTS]);
			}
			else
			{
				G_DPrintf("^2%4i entity freed - num_entities: %4i - %s\n", (int)(ent - g_entities), level.num_entities, ent->classname);
			}
		}

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

/**
 * @brief Spawns an event entity that will be auto-removed.
 *
 * @details The origin will be snapped to save net bandwidth, so care
 * must be taken if the origin is right on a surface (snap towards start vector first)
 *
 * @param[in] origin
 * @param[in] event
 * @return
 */
gentity_t *G_TempEntity(vec3_t origin, entity_event_t event)
{
	gentity_t *e;
	vec3_t    snapped;

	e = G_Spawn();

	e->s.eType = ET_EVENTS + event;

	e->classname      = "tempEntity";
	e->eventTime      = level.time;
	e->r.eventTime    = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy(origin, snapped);
	SnapVector(snapped);        // save network bandwidth
	TVG_SetOrigin(e, snapped);

	// find cluster for PVS
	trap_LinkEntity(e);

	return e;
}

/**
 * @brief Spawns an event entity that will be auto-removed
 * Use this for non visible and not origin based events like global sounds etc.
 *
 * @param[in] event
 *
 * @return
 *
 * @note Don't forget to call e->r.svFlags = SVF_BROADCAST; after
 */
gentity_t *G_TempEntityNotLinked(entity_event_t event)
{
	gentity_t *e;

	e = G_Spawn();

	e->s.eType        = ET_EVENTS + event;
	e->classname      = "tempEntity";
	e->eventTime      = level.time;
	e->r.eventTime    = level.time;
	e->freeAfterEvent = qtrue;
	e->r.linked       = qtrue; // don't link for real

	return e;
}

//==============================================================================

/**
 * @brief Adds an event+parm and twiddles the event counter
 * @param[in,out] ent
 * @param[in] event
 * @param[in] eventParm
 */
void G_AddEvent(gentity_t *ent, int event, int eventParm)
{
	if (!event)
	{
		G_Printf(S_COLOR_YELLOW "WARNING G_AddEvent: zero event added for entity %i\n", ent->s.number);
		return;
	}

	// use the sequential event list
	if (ent->client)
	{
		// commented in - externalEvents not being handled properly in Wolf right now
		ent->client->ps.events[ent->client->ps.eventSequence & (MAX_EVENTS - 1)]     = event;
		ent->client->ps.eventParms[ent->client->ps.eventSequence & (MAX_EVENTS - 1)] = eventParm;
		ent->client->ps.eventSequence++;
	}
	else
	{
		// commented in - externalEvents not being handled properly in Wolf right now
		ent->s.events[ent->s.eventSequence & (MAX_EVENTS - 1)]     = event;
		ent->s.eventParms[ent->s.eventSequence & (MAX_EVENTS - 1)] = eventParm;
		ent->s.eventSequence++;
	}
	ent->eventTime   = level.time;
	ent->r.eventTime = level.time;
}

/**
 * @brief Removed channel parm, since it wasn't used, and could cause confusion
 * @param[in] ent
 * @param[in] soundIndex
 */
void G_Sound(gentity_t *ent, int soundIndex)
{
	gentity_t *te;

	te = G_TempEntity(ent->r.currentOrigin, EV_GENERAL_SOUND);

	te->s.eventParm = soundIndex;
}

/**
 * @brief G_ClientSound
 * @param[in] ent
 * @param[in] soundIndex
 */
void G_ClientSound(gentity_t *ent, int soundIndex)
{
	if (ent && ent->client)
	{
		gentity_t *te;

		te = G_TempEntityNotLinked(EV_GLOBAL_CLIENT_SOUND);

		te->s.teamNum   = (ent->client - level.clients);
		te->s.eventParm = soundIndex;

		te->r.singleClient = ent->s.number;
		te->r.svFlags      = SVF_SINGLECLIENT | SVF_BROADCAST;
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
	gentity_t *e = &g_entities[client];

	G_AddEvent(e, EV_GENERAL_SOUND, soundIndex);
}

//==============================================================================

/**
 * @brief Sets the pos trajectory for a fixed position
 * @param[out] ent
 * @param[in] origin
 */
void TVG_SetOrigin(gentity_t *ent, vec3_t origin)
{
	VectorCopy(origin, ent->s.pos.trBase);
	ent->s.pos.trType     = TR_STATIONARY;
	ent->s.pos.trTime     = 0;
	ent->s.pos.trDuration = 0;
	VectorClear(ent->s.pos.trDelta);

	VectorCopy(origin, ent->s.origin);
	VectorCopy(origin, ent->r.currentOrigin);

	if (ent->client)
	{
		VectorCopy(origin, ent->client->ps.origin);
	}
}

/**
* @brief Plays specified sound globally.
* @param[in] sound
*/
void TVG_globalSound(const char *sound)
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_GLOBAL_SOUND);

	te->s.eventParm = G_SoundIndex(sound);
	te->r.svFlags  |= SVF_BROADCAST;
}

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
		if (g_entities[i].client && g_entities[i].client->sess.sessionTeam == team)
		{
			TVG_TempTraceIgnoreEntity(&g_entities[i]);
		}
	}
}

/**
* @brief TVG_TempTraceIgnoreEntities
* @param[in] ent
*/
void TVG_TempTraceIgnoreEntities(gentity_t *ent)
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
			TVG_TempTraceIgnoreEntity(hit);
		}

		if (hit->s.eType == ET_CORPSE && !(ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS))
		{
			TVG_TempTraceIgnoreEntity(hit);
		}

		if (hit->client && (!(ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC) || (ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC && ent->client->sess.sessionTeam != hit->client->sess.sessionTeam))
		    && hit->client->ps.pm_type == PM_DEAD && !(hit->client->ps.pm_flags & PMF_LIMBO))
		{
			TVG_TempTraceIgnoreEntity(hit);
		}
	}
}

char bigTextBuffer[100000];
