/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file g_utils.c
 * @brief misc utility functions for game module
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

typedef struct
{
	char oldShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	float timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS 128

int           remapCount = 0;
shaderRemap_t remappedShaders[MAX_SHADER_REMAPS];

void AddRemap(const char *oldShader, const char *newShader, float timeOffset)
{
	int i;

	for (i = 0; i < remapCount; i++)
	{
		if (Q_stricmp(oldShader, remappedShaders[i].oldShader) == 0)
		{
			// found it, just update this one
			strcpy(remappedShaders[i].newShader, newShader);
			remappedShaders[i].timeOffset = timeOffset;
			return;
		}
	}
	if (remapCount < MAX_SHADER_REMAPS)
	{
		strcpy(remappedShaders[remapCount].newShader, newShader);
		strcpy(remappedShaders[remapCount].oldShader, oldShader);
		remappedShaders[remapCount].timeOffset = timeOffset;
		remapCount++;
	}
	else
	{
		// this new but important warning might confuse the community
		// map makers didn't know about this so it might occure
		G_Printf(S_COLOR_YELLOW "WARNING AddRemap: MAX_SHADER_REMAPS 128 reached - shader not added\n");
	}
}

void G_ResetRemappedShaders(void)
{
	int i;

	remapCount = 0;

	// we don't actually have to do this but it's clean ...
	for (i = 0; i < MAX_SHADER_REMAPS; i++)
	{
		strcpy(remappedShaders[i].newShader, "");
		strcpy(remappedShaders[i].oldShader, "");
		remappedShaders[i].timeOffset = 0;
	}
}

const char *BuildShaderStateConfig()
{
	static char buff[MAX_STRING_CHARS * 4];
	char        out[(MAX_QPATH * 2) + 5];
	int         i;
	int         i1, i2;

	memset(buff, 0, MAX_STRING_CHARS);
	for (i = 0; i < remapCount; i++)
	{
		i1 = G_ShaderIndex(remappedShaders[i].oldShader);
		i2 = G_ShaderIndex(remappedShaders[i].newShader);

		Com_sprintf(out, (MAX_QPATH * 2) + 5, "%i=%i:%5.2f@", i1, i2, remappedShaders[i].timeOffset);
		Q_strcat(buff, sizeof(buff), out);
	}
	return buff;
}

/*
=========================================================================
model / sound configstring indexes
=========================================================================
*/

/*
================
G_FindConfigstringIndex
================
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

int G_ModelIndex(char *name)
{
	return G_FindConfigstringIndex(name, CS_MODELS, MAX_MODELS, qtrue);
}

int G_SoundIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_SOUNDS, MAX_SOUNDS, qtrue) + GAMESOUND_MAX;
}

int G_SkinIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_SKINS, MAX_CS_SKINS, qtrue);
}

int G_ShaderIndex(char *name)
{
	return G_FindConfigstringIndex(name, CS_SHADERS, MAX_CS_SHADERS, qtrue);
}

int G_CharacterIndex(const char *name)
{
	return G_FindConfigstringIndex(name, CS_CHARACTERS, MAX_CHARACTERS, qtrue);
}

int G_StringIndex(const char *string)
{
	return G_FindConfigstringIndex(string, CS_STRINGS, MAX_CSSTRINGS, qtrue);
}

//=====================================================================

/*
================
G_TeamCommand
Broadcasts a command to only a specific team
================
*/
void G_TeamCommand(team_t team, char *cmd)
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

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.
=============
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

/*
=============
G_FindInt

Like G_Find, but searches for integer values..
=============
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

/*
=============
G_FindFloat

Like G_Find, but searches for float values..
=============
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

/*
=============
G_FindVector

Like G_Find, but searches for vector values..
=============
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


/*
=============
G_FindByTargetname
=============
*/
gentity_t *G_FindByTargetname(gentity_t *from, const char *match)
{
	gentity_t *max = &g_entities[level.num_entities];
	int       hash = BG_StringHashValue(match);

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

		if (from->targetnamehash == hash && !Q_stricmp(from->targetname, match))
		{
			return from;
		}
	}

	return NULL;
}

// this version should be used for loops, saves the constant hash building
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

		if (from->targetnamehash == hash && !Q_stricmp(from->targetname, match))
		{
			return from;
		}
	}

	return NULL;
}

/*
=============
G_PickTarget

Selects a random entity from among the targets
=============
*/
#define MAXCHOICES  32

gentity_t *G_PickTarget(char *targetname)
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
		G_Printf(S_COLOR_YELLOW "WARNING G_PickTarget: target %s not found - returning NULL\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

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

/*
=============
G_UseEntity

Added to allow more checking on what uses what
=============
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

/*
==============================
G_UseTargets

"activator" should be set to the entity that initiated the firing.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets(gentity_t *ent, gentity_t *activator)
{
	gentity_t *t;
	int       hash;

	if (!ent)
	{
		return;
	}

	if (!ent->target)
	{
		return;
	}

	t    = NULL;
	hash = BG_StringHashValue(ent->target);
	while ((t = G_FindByTargetnameFast(t, ent->target, hash)) != NULL)
	{
		if (t == ent)
		{
			G_Printf(S_COLOR_YELLOW "WARNING G_UseTargets: Entity used itself.\n");
		}
		else
		{
			if (t->use)
			{
				//G_Printf ("ent->classname %s ent->targetname %s t->targetname %s t->s.number %d\n", ent->classname, ent->targetname, t->targetname, t->s.number);

				t->flags |= (ent->flags & FL_KICKACTIVATE);   // If 'ent' was kicked to activate, pass this along to it's targets.
				                                              //		It may become handy to put a "KICKABLE" flag in ents so that it knows whether to pass this along or not
				                                              //		Right now, the only situation where it would be weird would be an invisible_user that is a 'button' near
				                                              //		a rotating door that it triggers.  Kick the switch and the door next to it flies open.

				t->flags |= (ent->flags & FL_SOFTACTIVATE);   // likewise for soft activation

				if (activator &&
				    ((Q_stricmp(t->classname, "func_door") == 0) ||
				     (Q_stricmp(t->classname, "func_door_rotating") == 0)
				    )
				    )
				{
					// check door usage rules before allowing any entity to trigger a door open
					G_TryDoor(t, ent, activator);         // (door,other,activator)
				}
				else
				{
					G_UseEntity(t, ent, activator);
				}
			}
		}
		if (!ent->inuse)
		{
			G_Printf(S_COLOR_YELLOW "WARNING G_UseTargets: entity was removed while using targets\n");
			return;
		}
	}
}

/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
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
char *vtosf(const vec3_t v)
{
	static int  index;
	static char str[8][64];
	char        *s;

	// use an array so that multiple vtos won't collide
	s     = str[index];
	index = (index + 1) & 7;

	Com_sprintf(s, 64, "(%f %f %f)", v[0], v[1], v[2]);

	return s;
}

/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
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

#ifdef FEATURE_OMNIBOT
	// Notify omni-bot
	Bot_Queue_EntityCreated(e);
#endif
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn(void)
{
	int       i  = 0, force;
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

/*
=================
G_EntitiesFree
=================
*/
qboolean G_EntitiesFree(void)
{
	int       i;
	gentity_t *e = &g_entities[MAX_CLIENTS];

	for (i = MAX_CLIENTS; i < level.num_entities; i++, e++)
	{
		if (e->inuse)
		{
			continue;
		}
		// slot available
		return qtrue;
	}
	return qfalse;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity(gentity_t *ed)
{
#ifdef FEATURE_OMNIBOT
	Bot_Event_EntityDeleted(ed);
#endif

	if (ed->free)
	{
		ed->free(ed);
	}

	trap_UnlinkEntity(ed);       // unlink from world

	if (ed->neverFree)
	{
		return;
	}

	memset(ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime  = level.time;
	ed->inuse     = qfalse;
}

/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempEntity(vec3_t origin, int event)
{
	gentity_t *e = G_Spawn();
	vec3_t    snapped;

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

/*
=================
G_TempEntityNotLinked

Spawns an event entity that will be auto-removed
Use this for non visible and not origin based events like global sounds etc.

Note: Don't forget to call e->r.svFlags = SVF_BROADCAST; after
=================
*/
gentity_t *G_TempEntityNotLinked(int event)
{
	gentity_t *e = G_Spawn();

	e->s.eType        = ET_EVENTS + event;
	e->classname      = "tempEntity";
	e->eventTime      = level.time;
	e->r.eventTime    = level.time;
	e->freeAfterEvent = qtrue;
	e->r.linked       = qtrue; // don't link for real

	return e;
}

gentity_t *G_PopupMessage(popupMessageType_t type)
{
	gentity_t *e = G_Spawn();

	e->s.eType        = ET_EVENTS + EV_POPUPMESSAGE;
	e->classname      = "messageent";
	e->eventTime      = level.time;
	e->r.eventTime    = level.time;
	e->freeAfterEvent = qtrue;
	e->r.svFlags      = SVF_BROADCAST;
	e->s.effect1Time  = type;

	// find cluster for PVS
	//trap_LinkEntity(e);
	e->r.linked = qtrue; // don't link for real
	return e;
}

//==============================================================================

/*
===============
G_AddPredictableEvent

Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm)
{
	if (!ent->client)
	{
		return;
	}
	BG_AddPredictableEventToPlayerstate(event, eventParm, &ent->client->ps);
}

/*
===============
G_AddEvent

Adds an event+parm and twiddles the event counter
===============
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

/*
=============
G_Sound

  removed channel parm, since it wasn't used, and could cause confusion
=============
*/
void G_Sound(gentity_t *ent, int soundIndex)
{
	gentity_t *te = G_TempEntity(ent->r.currentOrigin, EV_GENERAL_SOUND);

	te->s.eventParm = soundIndex;
}

/*
=============
G_ClientSound
=============
*/
void G_ClientSound(gentity_t *ent, int soundIndex)
{
	if (ent && ent->client)
	{
		gentity_t *te = G_TempEntityNotLinked(EV_GLOBAL_CLIENT_SOUND);

		te->s.teamNum   = (ent->client - level.clients);
		te->s.eventParm = soundIndex;

		te->r.singleClient = ent->s.number;
		te->r.svFlags      = SVF_SINGLECLIENT | SVF_BROADCAST;
	}
}

/*
=============
G_AnimScriptSound
=============
*/
void G_AnimScriptSound(int soundIndex, vec3_t org, int client)
{
	gentity_t *e = &g_entities[client];

	G_AddEvent(e, EV_GENERAL_SOUND, soundIndex);
}

//==============================================================================

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
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

/*
==============
G_SetAngle
==============
*/
void G_SetAngle(gentity_t *ent, vec3_t angle)
{
	VectorCopy(angle, ent->s.apos.trBase);
	ent->s.apos.trType     = TR_STATIONARY;
	ent->s.apos.trTime     = 0;
	ent->s.apos.trDuration = 0;
	VectorClear(ent->s.apos.trDelta);

	VectorCopy(angle, ent->r.currentAngles);
}

/*
====================
infront
====================
*/
qboolean infront(gentity_t *self, gentity_t *other)
{
	vec3_t vec;
	float  dot;
	vec3_t forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->r.currentOrigin, self->r.currentOrigin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);
	// G_Printf( "other %5.2f\n",	dot);
	if (dot > 0.0)
	{
		return qtrue;
	}
	return qfalse;
}

// tag connections
/*
==================
G_ProcessTagConnect
==================
*/
void G_ProcessTagConnect(gentity_t *ent, qboolean clearAngles)
{
	if (!ent->tagName)
	{
		G_Error("G_ProcessTagConnect: NULL ent->tagName\n");
	}
	if (!ent->tagParent)
	{
		G_Error("G_ProcessTagConnect: NULL ent->tagParent\n");
	}
	G_FindConfigstringIndex(va("%i %i %s", ent->s.number, ent->tagParent->s.number, ent->tagName), CS_TAGCONNECTS, MAX_TAGCONNECTS, qtrue);
	ent->s.eFlags |= EF_TAGCONNECT;

	if (ent->client)
	{
		ent->client->ps.eFlags |= EF_TAGCONNECT;
		ent->client->ps.eFlags &= ~EF_PRONE_MOVING;
		ent->client->ps.eFlags &= ~EF_PRONE;
		ent->s.eFlags          &= ~EF_PRONE_MOVING;
		ent->s.eFlags          &= ~EF_PRONE;
	}

	if (clearAngles)
	{
		// clear out the angles so it always starts out facing the tag direction
		VectorClear(ent->s.angles);
		VectorCopy(ent->s.angles, ent->s.apos.trBase);
		ent->s.apos.trTime     = level.time;
		ent->s.apos.trDuration = 0;
		ent->s.apos.trType     = TR_STATIONARY;
		VectorClear(ent->s.apos.trDelta);
		VectorClear(ent->r.currentAngles);
	}
}

/*
================
DebugLine

  debug polygons only work when running a local game
  with r_debugSurface set to 2
================
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
	if (dot > 0.99 || dot < -0.99)
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

/*
================
G_SetEntState

  sets the entstate of an entity.
================
*/
void G_SetEntState(gentity_t *ent, entState_t state)
{
	if (ent->entstate == state)
	{
		G_DPrintf("G_SetEntState: entity %i already in desired state [%i]\n", ent->s.number, state);
		return;
	}

	switch (state)
	{
	case STATE_DEFAULT:
		if (ent->entstate == STATE_UNDERCONSTRUCTION)
		{
			ent->clipmask   = ent->realClipmask;
			ent->r.contents = ent->realContents;
			if (!ent->realNonSolidBModel)
			{
				ent->s.eFlags &= ~EF_NONSOLID_BMODEL;
			}
		}

		ent->entstate   = STATE_DEFAULT;
		ent->s.powerups = STATE_DEFAULT;

		if (ent->s.eType == ET_WOLF_OBJECTIVE)
		{
			char cs[MAX_STRING_CHARS];

			trap_GetConfigstring(ent->count, cs, sizeof(cs));
			ent->count2 &= ~256;
			Info_SetValueForKey(cs, "t", va("%i", ent->count2));
			trap_SetConfigstring(ent->count, cs);
		}

		if (ent->s.eType != ET_COMMANDMAP_MARKER)
		{
			trap_LinkEntity(ent);
		}

		// deal with any entities in the solid
		{
			int       listedEntities, e;
			int       entityList[MAX_GENTITIES];
			gentity_t *check, *block;

			listedEntities = trap_EntitiesInBox(ent->r.absmin, ent->r.absmax, entityList, MAX_GENTITIES);

			for (e = 0; e < listedEntities; e++)
			{
				check = &g_entities[entityList[e]];

				// ignore everything but items, players and missiles (grenades too)
				if (check->s.eType != ET_MISSILE && check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physicsObject)
				{
					continue;
				}

				if ((block = G_TestEntityPosition(check)) == NULL)
				{
					continue;
				}

				if (block != ent)
				{
					// the entity is blocked by another entity - that block this should take care of this itself
					continue;
				}

				if (check->client || check->s.eType == ET_CORPSE)
				{
					// gibs anything player like
					G_Damage(check, ent, ent, NULL, NULL, 9999, DAMAGE_NO_PROTECTION, MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER);
				}
				else if (check->s.eType == ET_ITEM && check->item->giType == IT_TEAM)
				{
					// see if it's a critical entity, one that we can't just simply kill (basically flags)
					Team_DroppedFlagThink(check);
				}
				else
				{
					// remove the landmine from both teamlists
					if (check->s.eType == ET_MISSILE && check->methodOfDeath == MOD_LANDMINE)
					{
						mapEntityData_t *mEnt;

						if ((mEnt = G_FindMapEntityData(&mapEntityData[0], check - g_entities)) != NULL)
						{
							G_FreeMapEntityData(&mapEntityData[0], mEnt);
						}

						if ((mEnt = G_FindMapEntityData(&mapEntityData[1], check - g_entities)) != NULL)
						{
							G_FreeMapEntityData(&mapEntityData[1], mEnt);
						}
					}

					// just get rid of it
					G_FreeEntity(check);
				}
			}
		}

		break;
	case STATE_UNDERCONSTRUCTION:
		ent->entstate     = STATE_UNDERCONSTRUCTION;
		ent->s.powerups   = STATE_UNDERCONSTRUCTION;
		ent->realClipmask = ent->clipmask;
		if (ent->s.eType != ET_CONSTRUCTIBLE)                               // don't make nonsolid as we want to make them partially solid for staged construction
		{
			ent->clipmask = 0;
		}
		ent->realContents = ent->r.contents;
		if (ent->s.eType != ET_CONSTRUCTIBLE)
		{
			ent->r.contents = 0;
		}
		if (ent->s.eFlags & EF_NONSOLID_BMODEL)
		{
			ent->realNonSolidBModel = qtrue;
		}
		else if (ent->s.eType != ET_CONSTRUCTIBLE)
		{
			ent->s.eFlags |= EF_NONSOLID_BMODEL;
		}

		if (!Q_stricmp(ent->classname, "misc_mg42"))
		{
			// stop using the mg42
			mg42_stopusing(ent);
		}

		if (ent->s.eType == ET_COMMANDMAP_MARKER)
		{
			mapEntityData_t *mEnt;

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[0], mEnt);
			}
			if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[1], mEnt);
			}
		}

		trap_LinkEntity(ent);
		break;
	case STATE_INVISIBLE:
		if (ent->entstate == STATE_UNDERCONSTRUCTION)
		{
			ent->clipmask   = ent->realClipmask;
			ent->r.contents = ent->realContents;
			if (!ent->realNonSolidBModel)
			{
				ent->s.eFlags &= ~EF_NONSOLID_BMODEL;
			}
		}

		ent->entstate   = STATE_INVISIBLE;
		ent->s.powerups = STATE_INVISIBLE;

		if (!Q_stricmp(ent->classname, "misc_mg42"))
		{
			mg42_stopusing(ent);
		}
		else if (ent->s.eType == ET_WOLF_OBJECTIVE)
		{
			char cs[MAX_STRING_CHARS];

			trap_GetConfigstring(ent->count, cs, sizeof(cs));
			ent->count2 |= 256;
			Info_SetValueForKey(cs, "t", va("%i", ent->count2));
			trap_SetConfigstring(ent->count, cs);
		}

		if (ent->s.eType == ET_COMMANDMAP_MARKER)
		{
			mapEntityData_t *mEnt;

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[0], mEnt);
			}
			if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[1], mEnt);
			}
		}

		trap_UnlinkEntity(ent);
		break;
	}
}

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
					memset(&g_campaigns[level.campaignCount], 0, sizeof(g_campaigns[0]));
					level.campaignCount--;

					break;
				}
			}
		}
	}

	trap_PC_FreeSource(handle);
	return mapFound;
}

qboolean G_MapIsValidCampaignStartMap(void)
{
	int i;

	for (i = 0; i < level.campaignCount; i++)
	{
		if (!Q_stricmp(g_campaigns[i].mapnames[0], level.rawmapname))
		{
			return qtrue;
		}
	}

	return qfalse;
}

char bigTextBuffer[100000];

void G_ParseCampaigns(void)
{
	int      i;
	qboolean mapFound = qfalse;

	level.campaignCount   = 0;
	level.currentCampaign = -1;
	memset(&g_campaigns, 0, sizeof(g_campaignInfo_t) * MAX_CAMPAIGNS);

	if (g_gametype.integer != GT_WOLF_CAMPAIGN)
	{
		trap_Cvar_Set("g_oldCampaign", "");
		trap_Cvar_Set("g_currentCampaign", "");
		trap_Cvar_Set("g_currentCampaignMap", "0");
		return;
	}

	if (g_campaignFile.string[0])
	{
		if (G_LoadCampaignsFromFile(g_campaignFile.string))
		{
			mapFound = qtrue;
		}
	}

	if (!mapFound)
	{
		// get all campaigns from .campaign files
		int  dirlen;
		int  numdirs = trap_FS_GetFileList("scripts", ".campaign", bigTextBuffer, sizeof(bigTextBuffer));
		char filename[MAX_QPATH]; // was 128
		char *dirptr = bigTextBuffer;

		for (i = 0; i < numdirs; i++, dirptr += dirlen + 1)
		{
			// log a warning if server has more than MAX_CAMPAIGNS
			if (level.campaignCount >= MAX_CAMPAIGNS)
			{
				G_LogPrintf("WARNING G_ParseCampaigns: number of campaigns larger then MAX_CAMPAIGNS\n");
				break;
			}

			dirlen = strlen(dirptr);
			strcpy(filename, "scripts/");
			strcat(filename, dirptr);

			if (G_LoadCampaignsFromFile(filename))
			{
				mapFound = qtrue;
			}
		}
	}

	if (!mapFound)
	{
		// map isn't found in the current campaign, see if it's the first map in another campaign
		for (i = 0; i < level.campaignCount; i++)
		{
			if (!Q_stricmp(g_campaigns[i].mapnames[0], level.rawmapname))
			{
				// someone manually specified a /map command, and it's the first map in a campaign
				trap_Cvar_Set("g_currentCampaign", g_campaigns[i].shortname);
				trap_Cvar_Set("g_currentCampaignMap", "0");

				level.newCampaign = qtrue;

				g_campaigns[level.campaignCount].current = 0;
				level.currentCampaign                    = i;

				break;
			}
		}

		if (i == level.campaignCount)
		{
			char buf[MAX_STRING_CHARS];

			if (trap_Argc() < 1) // command not found, throw error
			{
				G_Error("Usage 'map <mapname>\n'");
			}

			trap_Argv(0, buf, sizeof(buf));

			if (!(*buf)) // command not found, throw error
			{
				G_Error("Usage 'map <mapname>\n'");
			}

			// no campaign found, fallback to GT_WOLF
			// and reload the map
			trap_Cvar_Set("g_gametype", "2");
			trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", buf, level.rawmapname));
		}
	}
}

void G_PrintClientSpammyCenterPrint(int entityNum, char *text)
{
	if (!g_entities[entityNum].client)
	{
		return;
	}

	if (level.time - g_entities[entityNum].client->lastSpammyCentrePrintTime < 1000)
	{
		return;
	}

	trap_SendServerCommand(entityNum, va("cp \"%s\" 1", text));
	g_entities[entityNum].client->lastSpammyCentrePrintTime = level.time;
}

team_t G_GetTeamFromEntity(gentity_t *ent)
{
	switch (ent->s.eType)
	{
	case ET_PLAYER:
		if (ent->client)
		{
			return ent->client->sess.sessionTeam;
		}
		else
		{
			return TEAM_FREE;
		}
		break;
	case ET_MISSILE:
	case ET_GENERAL:
		switch (ent->methodOfDeath)
		{
		case MOD_GRENADE_LAUNCHER:
		case MOD_GRENADE_PINEAPPLE:
		case MOD_PANZERFAUST:
		case MOD_GPG40:
		case MOD_M7:
		case MOD_ARTY:
		case MOD_AIRSTRIKE:
		case MOD_MORTAR:
		case MOD_SMOKEGRENADE:
			return ent->s.teamNum;
		case MOD_SATCHEL:
		case MOD_DYNAMITE:
		case MOD_LANDMINE:
			return ent->s.teamNum % 4;
		}
		break;
	case ET_MOVER:
		if (!Q_stricmp(ent->classname, "script_mover"))
		{
			return ent->s.teamNum;
		}
		break;
	case ET_CONSTRUCTIBLE:
		return ent->s.teamNum;
		break;
	case ET_MG42_BARREL:
		return G_GetTeamFromEntity(&g_entities[ent->r.ownerNum]);

	default:
		break;
	}

	return TEAM_FREE;
}
