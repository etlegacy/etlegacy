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

	// init scripting
	e->scriptStatus.scriptEventIndex = -1;

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
		e = &g_entities[MAX_CLIENTS];
		for (i = MAX_CLIENTS ; i < level.num_entities ; i++, e++)
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
	if ((ent->s.eType == ET_TEMPHEAD || ent->s.eType == ET_TEMPLEGS || ent->s.eType == ET_CORPSE || ent->s.eType >= ET_EVENTS)
	    && trap_Cvar_VariableIntegerValue("g_debugHitboxes") == 0 && trap_Cvar_VariableIntegerValue("g_debugPlayerHitboxes") == 0
	    && trap_Cvar_VariableIntegerValue("g_debugbullets") < 3)
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
	G_SetOrigin(e, snapped);

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
void G_SetOrigin(gentity_t *ent, vec3_t origin)
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
 * @brief Debug polygons only work when running a local game with r_debugSurface set to 2
 * @param[in] start
 * @param[in] end
 * @param[in] color
 * @return
 */
int DebugLine(vec3_t start, vec3_t end, int color)
{
	vec3_t points[4], dir, cross, up = { 0, 0, 1 };
	float  dot;

	VectorCopy(start, points[0]);
	VectorCopy(start, points[1]);
	VectorCopy(end, points[2]);
	VectorCopy(end, points[3]);


	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99f || dot < -0.99f)
	{
		VectorSet(cross, 1, 0, 0);
	}
	else
	{
		CrossProduct(dir, up, cross);
	}

	VectorNormalize(cross);

	VectorMA(points[0], 2, cross, points[0]);
	VectorMA(points[1], -2, cross, points[1]);
	VectorMA(points[2], -2, cross, points[2]);
	VectorMA(points[3], 2, cross, points[3]);

	return trap_DebugPolygonCreate(color, 4, points);
}

/**
 * @brief G_LoadCampaignsFromFile
 * @param[in] filename
 * @return
 */
static qboolean G_LoadCampaignsFromFile(const char *filename)
{
	int        handle;
	pc_token_t token;
	const char *s;
	qboolean   mapFound = qfalse;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: file not found: %s\n", filename);
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		trap_PC_FreeSource(handle);
		return qfalse;
	}
	if (*token.string != '{')
	{
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	while (trap_PC_ReadToken(handle, &token))
	{
		if (*token.string == '}')
		{
			level.campaignCount++;

			// can't handle any more.
			if (level.campaignCount >= MAX_CAMPAIGNS)
			{
				G_Printf(S_COLOR_RED "G_LoadCampaignsFromFile: MAX_CAMPAIGNS reached: '%i'\n", MAX_CAMPAIGNS);
				break;
			}

			if (!trap_PC_ReadToken(handle, &token))
			{
				// eof
				break;
			}

			if (*token.string != '{')
			{
				G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: unexpected token '%s' inside: %s\n", token.string, filename);
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "name") ||
		         !Q_stricmp(token.string, "description") ||
		         !Q_stricmp(token.string, "image"))
		{
			if ((s = PC_String_Parse(handle)) == NULL)
			{
				G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: unexpected end of file inside: %s\n", filename);
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shortname"))
		{
			if ((s = PC_String_Parse(handle)) == NULL)
			{
				G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: unexpected end of file inside: %s\n", filename);
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(g_campaigns[level.campaignCount].shortname, s, sizeof(g_campaigns[level.campaignCount].shortname));
			}
		}
		else if (!Q_stricmp(token.string, "next"))
		{
			if ((s = PC_String_Parse(handle)) == NULL)
			{
				G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: unexpected end of file inside: %s\n", filename);
				trap_PC_FreeSource(handle);
				return qfalse;
			}
			else
			{
				Q_strncpyz(g_campaigns[level.campaignCount].shortname, s, sizeof(g_campaigns[level.campaignCount].next));
			}
		}
		else if (!Q_stricmp(token.string, "type"))
		{
			if (!trap_PC_ReadToken(handle, &token))
			{
				G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: unexpected end of file inside: %s\n", filename);
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			if (strstr(token.string, "wolfsp"))
			{
				g_campaigns[level.campaignCount].typeBits |= (1 << GT_SINGLE_PLAYER);
			}
			if (strstr(token.string, "wolfcoop"))
			{
				g_campaigns[level.campaignCount].typeBits |= (1 << GT_COOP);
			}
			if (strstr(token.string, "wolfmp"))
			{
				g_campaigns[level.campaignCount].typeBits |= (1 << GT_WOLF);
			}
			if (strstr(token.string, "wolfsw"))
			{
				g_campaigns[level.campaignCount].typeBits |= (1 << GT_WOLF_STOPWATCH);
			}
			if (strstr(token.string, "wolflms"))
			{
				g_campaigns[level.campaignCount].typeBits |= (1 << GT_WOLF_LMS);
			}
		}
		else if (!Q_stricmp(token.string, "maps"))
		{
			char *ptr, mapname[128], *mapnamePtr;

			if (!trap_PC_ReadToken(handle, &token))
			{
				G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: unexpected end of file inside: %s\n", filename);
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			ptr = token.string;
			while (*ptr)
			{
				mapnamePtr = mapname;
				while (*ptr && *ptr != ';')
				{
					*mapnamePtr++ = *ptr++;
				}
				if (*ptr)
				{
					ptr++;
				}
				*mapnamePtr = '\0';

				if (g_gametype.integer == GT_WOLF_CAMPAIGN)
				{
					if (!mapFound &&
					    !Q_stricmp(g_campaigns[level.campaignCount].shortname, g_currentCampaign.string) &&
					    !Q_stricmp(mapname, level.rawmapname))
					{

						if (g_currentCampaignMap.integer == 0)
						{
							level.newCampaign = qtrue;
						}
						else
						{
							level.newCampaign = qfalse;
						}

						if (g_campaigns[level.campaignCount].mapCount == g_currentCampaignMap.integer)
						{
							g_campaigns[level.campaignCount].current = g_campaigns[level.campaignCount].mapCount;
							mapFound                                 = qtrue;
							//trap_Cvar_Set( "g_currentCampaignMap", va( "%i", g_campaigns[level.campaignCount].mapCount ) );
						}

						level.currentCampaign = level.campaignCount;
					}
				}
				// don't stomp out of bounds
				if (g_campaigns[level.campaignCount].mapCount < MAX_MAPS_PER_CAMPAIGN)
				{
					Q_strncpyz(g_campaigns[level.campaignCount].mapnames[g_campaigns[level.campaignCount].mapCount], mapname, MAX_QPATH);
					g_campaigns[level.campaignCount].mapCount++;
				}
				else
				{
					// yell if there are too many maps in this campaign,
					// and then skip it

					G_Printf(S_COLOR_RED "ERROR G_LoadCampaignsFromFile: Campaign %s (%s) has too many maps\n", g_campaigns[level.campaignCount].shortname, filename);
					// hack - end of campaign will increment this
					// again, so this one will be overwritten
					// clear out this campaign so that everything's
					// okay when when we add the next
					Com_Memset(&g_campaigns[level.campaignCount], 0, sizeof(g_campaigns[0]));
					level.campaignCount--;

					break;
				}
			}
		}
	}

	trap_PC_FreeSource(handle);
	return mapFound;
}

char bigTextBuffer[100000];
