/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * Portions of this file were taken from the NQ project.
 * Credit goes to core
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
 * @file tvg_spawn.c
 */

#include "tvg_local.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
#endif

/**
 * @brief TVG_SpawnStringExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean TVG_SpawnStringExt(const char *key, const char *defaultString, char **out, const char *file, int line)
{
	int i;

	if (!level.spawning)
	{
		*out = (char *)defaultString;
		// see InitMover
		G_Error("G_SpawnString() called while not spawning, file %s, line %i\n", file, line);
	}

	for (i = 0 ; i < level.numSpawnVars ; i++)
	{
		if (!strcmp(key, level.spawnVars[i][0]))
		{
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

/**
 * @brief TVG_SpawnFloatExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean TVG_SpawnFloatExt(const char *key, const char *defaultString, float *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = TVG_SpawnStringExt(key, defaultString, &s, file, line);
	*out    = Q_atof(s);
	return present;
}

/**
 * @brief TVG_SpawnIntExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean TVG_SpawnIntExt(const char *key, const char *defaultString, int *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = TVG_SpawnStringExt(key, defaultString, &s, file, line);
	*out    = Q_atoi(s);
	return present;
}

/**
 * @brief TVG_SpawnVectorExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean TVG_SpawnVectorExt(const char *key, const char *defaultString, float *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = TVG_SpawnStringExt(key, defaultString, &s, file, line);
	Q_sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

/**
 * @brief G_SpawnVector2DExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean TVG_SpawnVector2DExt(const char *key, const char *defaultString, float *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = TVG_SpawnStringExt(key, defaultString, &s, file, line);
	Q_sscanf(s, "%f %f", &out[0], &out[1]);
	return present;
}

field_t fields[] =
{
	{ "classname",  FOFS(classname),  F_LSTRING,   0 },
	{ "origin",     FOFS(s.origin),   F_VECTOR,    0 },
	{ "spawnflags", FOFS(spawnflags), F_INT,       0 },

	{ "target",     FOFS(target),     F_LSTRING,   0 },
	{ "targetname", FOFS(targetname), F_LSTRING,   0 },

	{ "angles",     FOFS(s.angles),   F_VECTOR,    0 },
	{ "angle",      FOFS(s.angles),   F_ANGLEHACK, 0 },

	{ NULL,         0,                F_IGNORE,    0 }
};

typedef struct
{
	char *name;
	void (*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start(gentity_t *ent);
void SP_info_player_checkpoint(gentity_t *ent);
void SP_info_player_deathmatch(gentity_t *ent);
void SP_info_player_intermission(gentity_t *ent);

void SP_info_notnull(gentity_t *self);

void SP_team_CTF_redspawn(gentity_t *ent);
void SP_team_CTF_bluespawn(gentity_t *ent);

// for multiplayer spawnpoint selection
void SP_team_WOLF_objective(gentity_t *ent);
void SP_team_WOLF_checkpoint(gentity_t *ent);

spawn_t spawns[] =
{
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{ "info_player_start",        SP_info_player_start        },
	{ "info_player_checkpoint",   SP_info_player_checkpoint   },
	{ "info_player_deathmatch",   SP_info_player_deathmatch   },
	{ "info_player_intermission", SP_info_player_intermission },

	{ "info_notnull",             SP_info_notnull             }, // use target_position instead

	{ 0,                          0                           }
};

/**
 * @brief Finds the spawn function for the entity and calls it
 * @param ent
 * @return qfalse if spawn not found
 */
qboolean TVG_CallSpawn(gentity_t *ent)
{
	spawn_t *s;

	if (!ent->classname)
	{
		G_Printf("TVG_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check normal spawn functions
	for (s = spawns ; s->name ; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{
			// found it
			s->spawn(ent);

			return qtrue;
		}
	}

	// hack: this avoids spammy prints on start, bsp uses obsolete classnames!
	// bot_sniper_spot (railgun)
	//if (Q_stricmp(ent->classname, "bot_sniper_spot"))
	//{
	//	G_Printf("%s doesn't have a spawn function\n", ent->classname);
	//}

	return qfalse;
}

extern void *G_Alloc(unsigned int size);

/**
 * @brief Builds a copy of the string, translating \\n to real linefeeds
 * so message texts can be multi-line
 * @param string
 * @return
 */
char *TVG_NewString(const char *string)
{
	char         *newb, *new_p;
	unsigned int i, l;

	l = strlen(string) + 1;

	newb = G_Alloc(l);

	new_p = newb;

	// turn \n into a real linefeed
	for (i = 0 ; i < l ; i++)
	{
		if (i < l - 1 && string[i] == '\\')
		{
			i++;
			if (string[i] == 'n')
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '\\';
			}
		}
		else
		{
			*new_p++ = string[i];
		}
	}

	return newb;
}

/**
 * @brief Takes a key/value pair and sets the binary values
 * in a gentity
 * @param[in] key
 * @param[in] value
 * @param[in] ent
 */
void TVG_ParseField(const char *key, const char *value, gentity_t *ent)
{
	field_t *f;
	byte    *b;
	float   v;
	vec3_t  vec;

	for (f = fields ; f->name ; f++)
	{
		if (!Q_stricmp(f->name, key))
		{
			// found it
			b = (byte *)ent;

			switch (f->type)
			{
			case F_LSTRING:
				*( char ** )(b + f->ofs) = TVG_NewString(value);
				break;
			case F_VECTOR:
				Q_sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				(( float * )(b + f->ofs))[0] = vec[0];
				(( float * )(b + f->ofs))[1] = vec[1];
				(( float * )(b + f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*( int * )(b + f->ofs) = Q_atoi(value);
				break;
			case F_FLOAT:
				*( float * )(b + f->ofs) = Q_atof(value);
				break;
			case F_ANGLEHACK:
				v                            = Q_atof(value);
				(( float * )(b + f->ofs))[0] = 0;
				(( float * )(b + f->ofs))[1] = v;
				(( float * )(b + f->ofs))[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}

/**
 * @brief Spawn an entity and fill in all of the level fields from
 * level.spawnVars[], then call the class specfic spawn function
 * @return
 */
gentity_t *TVG_SpawnGEntityFromSpawnVars(void)
{
	int       i;
	gentity_t *ent;
	char      *str;

	ent = TVG_Spawn(); // get the next free entity

	for (i = 0 ; i < level.numSpawnVars ; i++)
	{
		TVG_ParseField(level.spawnVars[i][0], level.spawnVars[i][1], ent);
	}

	// check for "notteam" / "notfree" flags
	TVG_SpawnInt("notteam", "0", &i);
	if (i)
	{
		G_Printf("G_SpawnGEntityFromSpawnVars Warning: Can't spawn entity in team games - returning NULL\n");

		TVG_FreeEntity(ent);
		return NULL;
	}

	// allowteams handling
	TVG_SpawnString("allowteams", "", &str);
	if (str[0])
	{
		str = Q_strlwr(str);
		if (strstr(str, "axis"))
		{
			ent->allowteams |= ALLOW_AXIS_TEAM;
		}
		if (strstr(str, "allies"))
		{
			ent->allowteams |= ALLOW_ALLIED_TEAM;
		}
		if (strstr(str, "cvops"))
		{
			ent->allowteams |= ALLOW_DISGUISED_CVOPS;
		}
	}

	if (ent->targetname && *ent->targetname)
	{
		ent->targetnamehash = TVG_StringHashValue(ent->targetname);
	}
	else
	{
		ent->targetnamehash = -1;
	}

	// move editor origin to pos
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	// if we didn't get a classname, don't bother spawning anything
	if (!TVG_CallSpawn(ent))
	{
		TVG_FreeEntity(ent);
	}

	return ent;
}

/**
 * @brief TVG_AddSpawnVarToken
 * @param[in] string
 * @return
 */
char *TVG_AddSpawnVarToken(const char *string)
{
	size_t l;
	char   *dest;

	l = strlen(string);
	if (level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS)
	{
		G_Error("TVG_AddSpawnVarToken: MAX_SPAWN_VARS\n");
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	Com_Memcpy(dest, string, l + 1);

	level.numSpawnVarChars += l + 1;

	return dest;
}

/**
 * @brief Parses a brace bounded set of key / value pairs out of the
 * level's entity strings into level.spawnVars[]
 * This does not actually spawn an entity.
 * @return
 */
qboolean TVG_ParseSpawnVars(void)
{
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars     = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if (!trap_GetEntityToken(com_token, sizeof(com_token)))
	{
		// end of spawn string
		return qfalse;
	}
	if (com_token[0] != '{')
	{
		G_Error("TVG_ParseSpawnVars: found %s when expecting {\n", com_token);
	}

	// go through all the key / value pairs
	while (1)
	{
		// parse key
		if (!trap_GetEntityToken(keyname, sizeof(keyname)))
		{
			G_Error("TVG_ParseSpawnVars: EOF without closing brace\n");
		}

		if (keyname[0] == '}')
		{
			break;
		}

		// parse value
		if (!trap_GetEntityToken(com_token, sizeof(com_token)))
		{
			G_Error("TVG_ParseSpawnVars: EOF without closing brace\n");
		}

		if (com_token[0] == '}')
		{
			G_Error("TVG_ParseSpawnVars: closing brace without data\n");
		}
		if (level.numSpawnVars == MAX_SPAWN_VARS)
		{
			G_Error("TVG_ParseSpawnVars: MAX_SPAWN_VARS\n");
		}
		level.spawnVars[level.numSpawnVars][0] = TVG_AddSpawnVarToken(keyname);
		level.spawnVars[level.numSpawnVars][1] = TVG_AddSpawnVarToken(com_token);
		level.numSpawnVars++;
	}

	return qtrue;
}

/**
 * @brief Every map should have exactly one worldspawn.
 * @details QUAKED worldspawn (0 0 0) ? NO_GT_WOLF NO_GT_STOPWATCH NO_GT_CHECKPOINT NO_LMS
 *
 * "music"     Music wav file
 * "gravity"   800 is default gravity
 * "message" Text to print during connection process
 * "ambient"  Ambient light value (must use '_color')
 * "_color"    Ambient light color (must be used with 'ambient')
 * "sun"        Shader to use for 'sun' image
 */
void SP_worldspawn(void)
{
	char *s;

	TVG_SpawnString("classname", "", &s);
	if (Q_stricmp(s, "worldspawn"))
	{
		G_Error("SP_worldspawn: The first entity isn't 'worldspawn'\n");
	}

	level.mapcoordsValid = qfalse;
	if (TVG_SpawnVector2D("mapcoordsmins", "-128 128", level.mapcoordsMins) &&     // top left
	    TVG_SpawnVector2D("mapcoordsmaxs", "128 -128", level.mapcoordsMaxs))       // bottom right
	{
		level.mapcoordsValid = qtrue;
	}

	TVG_SpawnString("spawnflags", "0", &s);
	g_entities[ENTITYNUM_WORLD].spawnflags   = Q_atoi(s);
	g_entities[ENTITYNUM_WORLD].r.worldflags = g_entities[ENTITYNUM_WORLD].spawnflags;

	g_entities[ENTITYNUM_WORLD].s.number   = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname  = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number   = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname  = "nothing";
}

/**
 * @brief Parses textual entity definitions out of an entstring and spawns gentities.
 */
void TVG_SpawnEntitiesFromString(void)
{
	// allow calls to G_Spawn*()
	G_Printf("Enable spawning!\n");
	level.spawning     = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if (!TVG_ParseSpawnVars())
	{
		G_Error("SpawnEntities: no entities\n");
	}
	SP_worldspawn();

	// parse ents
	while (TVG_ParseSpawnVars())
	{
		TVG_SpawnGEntityFromSpawnVars();
	}

	G_Printf("Disable spawning!\n");
	level.spawning = qfalse;            // any future calls to G_Spawn*() will be errors
}

#ifdef FEATURE_LUA
//===============================================================
// Some helper functions for entity property handling..
// these functions are used by Lua.

/**
 * @brief GetFieldIndex
 * @param[in] fieldname
 * @return The index in the fiels[] array of the given fieldname, -1 if not found..
 */
int GetFieldIndex(const char *fieldname)
{
	int i;

	for (i = 0; fields[i].name; i++)
		if (!Q_stricmp(fields[i].name, fieldname))
		{
			return i;
		}
	return -1;
}

/**
 * @brief GetFieldType
 * @param[in] fieldname
 * @return The fieldType of the given fieldname, otherwise F_IGNORE if the field is not found.
 */
fieldtype_t GetFieldType(const char *fieldname)
{
	int index = GetFieldIndex(fieldname);

	if (index == -1)
	{
		return F_IGNORE;
	}
	return fields[index].type;
}
#endif // FEATURE_LUA
