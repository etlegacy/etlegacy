/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
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
 * @file cg_spawn.c
 * @brief Client sided only map entities
 */

#include "cg_local.h"

/**
 * @brief CG_SpawnString
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @return
 */
qboolean CG_SpawnString(const char *key, const char *defaultString, char **out)
{
	int i;

	if (!cg.spawning)
	{
		*out = (char *)defaultString;
		CG_Error("CG_SpawnString() called while not spawning\n");
	}

	for (i = 0 ; i < cg.numSpawnVars ; i++)
	{
		if (!strcmp(key, cg.spawnVars[i][0]))
		{
			*out = cg.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

/**
 * @brief CG_SpawnFloat
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @return
 */
qboolean CG_SpawnFloat(const char *key, const char *defaultString, float *out)
{
	char     *s;
	qboolean present;

	present = CG_SpawnString(key, defaultString, &s);
	*out    = (float)atof(s);
	return present;
}

/**
 * @brief CG_SpawnInt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @return
 */
qboolean CG_SpawnInt(const char *key, const char *defaultString, int *out)
{
	char     *s;
	qboolean present;

	present = CG_SpawnString(key, defaultString, &s);
	*out    = Q_atoi(s);
	return present;
}

/**
 * @brief CG_SpawnVector
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @return
 */
qboolean CG_SpawnVector(const char *key, const char *defaultString, float *out)
{
	char     *s;
	qboolean present;

	present = CG_SpawnString(key, defaultString, &s);
	sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

/**
 * @brief CG_SpawnVector2D
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @return
 */
qboolean CG_SpawnVector2D(const char *key, const char *defaultString, float *out)
{
	char     *s;
	qboolean present;

	present = CG_SpawnString(key, defaultString, &s);
	sscanf(s, "%f %f", &out[0], &out[1]);
	return present;
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
 * @brief SP_path_corner_2
 */
void SP_path_corner_2(void)
{
	char   *targetname;
	vec3_t origin;

	CG_SpawnString("targetname", "", &targetname);
	CG_SpawnVector("origin", "0 0 0", origin);

	if (!*targetname)
	{
		CG_Error("path_corner_2 with no targetname at %s\n", vtos(origin));
	}

	if (numPathCorners >= MAX_PATH_CORNERS)
	{
		CG_Error("Maximum path_corners hit\n");
	}

	BG_AddPathCorner(targetname, origin);
}

/**
 * @brief SP_info_train_spline_main
 */
void SP_info_train_spline_main(void)
{
	char         *targetname;
	char         *target;
	char         *control;
	vec3_t       origin;
	int          i;
	char         *end;
	splinePath_t *spline;

	if (!CG_SpawnVector("origin", "0 0 0", origin))
	{
		CG_Error("info_train_spline_main with no origin\n");
	}

	if (!CG_SpawnString("targetname", "", &targetname))
	{
		CG_Error("info_train_spline_main with no targetname at %s\n", vtos(origin));
	}

	CG_SpawnString("target", "", &target);

	spline = BG_AddSplinePath(targetname, target, origin);

	if (CG_SpawnString("end", "", &end))
	{
		spline->isEnd = qtrue;
	}
	else if (CG_SpawnString("start", "", &end))
	{
		spline->isStart = qtrue;
	}

	for (i = 1;; i++)
	{
		if (!CG_SpawnString(i == 1 ? va("control") : va("control%i", i), "", &control))
		{
			break;
		}

		BG_AddSplineControl(spline, control);
	}
}

/**
 * @brief CG_corona
 */
void CG_corona(void)
{
	cg_corona_t *corona;
	float       scale;
	vec3_t      color;
	vec3_t      org;
	char        *coronaStr;

	if (CG_SpawnString("targetname", "", &coronaStr) || CG_SpawnString("scriptname", "", &coronaStr) || CG_SpawnString("spawnflags", "", &coronaStr))
	{
		return;
	}

	if (cg.numCoronas >= MAX_GAMECORONAS)
	{
		CG_Error("^1MAX_GAMECORONAS(%i) hit", MAX_GAMECORONAS);
	}

	corona = &cgs.corona[cg.numCoronas++];

	CG_SpawnVector("origin", "0 0 0", org);

	VectorCopy(org, corona->org);

	CG_SpawnFloat("scale", "1", &scale);
	corona->scale = scale;

	if (!CG_SpawnVector("_color", "0 0 0", color))
	{
		if (!CG_SpawnVector("color", "0 0 0", color))
		{
			VectorSet(corona->color, 1.f, 1.f, 1.f);
		}
	}

	VectorCopy(color, corona->color);

	//CG_Printf("loaded corona %i \n", cg.numCoronas );
}

/**
 * @brief SP_misc_gamemodel
 */
void SP_misc_gamemodel(void)
{
	char           *model;
	vec_t          angle;
	vec3_t         angles;
	vec_t          scale;
	vec3_t         vScale;
	vec3_t         org;
	cg_gamemodel_t *gamemodel;
	int            i;

	if (CG_SpawnString("targetname", "", &model) || CG_SpawnString("scriptname", "", &model) || CG_SpawnString("spawnflags", "", &model))
	{
		// this model may not be static, so let the server handle it
		return;
	}

	if (cg.numMiscGameModels >= MAX_STATIC_GAMEMODELS)
	{
		CG_Error("^1MAX_STATIC_GAMEMODELS(%i) hit\n", MAX_STATIC_GAMEMODELS);
	}

	CG_SpawnString("model", "", &model);

	CG_SpawnVector("origin", "0 0 0", org);

	if (!CG_SpawnVector("angles", "0 0 0", angles))
	{
		if (CG_SpawnFloat("angle", "0", &angle))
		{
			angles[YAW] = angle;
		}
	}

	if (!CG_SpawnVector("modelscale_vec", "1 1 1", vScale))
	{
		if (CG_SpawnFloat("modelscale", "1", &scale))
		{
			VectorSet(vScale, scale, scale, scale);
		}
	}

	gamemodel = &cgs.miscGameModels[cg.numMiscGameModels++];

	// don't register models more than once (f.e. after warmup or map_restart)
	if (gamemodel->model == 0)
	{
		gamemodel->model = trap_R_RegisterModel(model);
	}

	AnglesToAxis(angles, gamemodel->axes);
	for (i = 0; i < 3; i++)
	{
		VectorScale(gamemodel->axes[i], vScale[i], gamemodel->axes[i]);
	}
	VectorCopy(org, gamemodel->org);

	if (gamemodel->model)
	{
		vec3_t mins, maxs;

		trap_R_ModelBounds(gamemodel->model, mins, maxs);

		for (i = 0; i < 3; i++)
		{
			mins[i] *= vScale[i];
			maxs[i] *= vScale[i];
		}

		gamemodel->radius = RadiusFromBounds(mins, maxs);
	}
	else
	{
		gamemodel->radius = 0;
	}
}

/**
 * @brief SP_trigger_objective_info
 */
void SP_trigger_objective_info(void)
{
	char *temp;

	CG_SpawnString("infoAllied", "^1No Text Supplied", &temp);
	Q_strncpyz(cg.oidTriggerInfoAllies[cg.numOIDtriggers2], temp, 256);

	CG_SpawnString("infoAxis", "^1No Text Supplied", &temp);
	Q_strncpyz(cg.oidTriggerInfoAxis[cg.numOIDtriggers2], temp, 256);

	cg.numOIDtriggers2++;
}

typedef struct
{
	const char *name;
	void (*spawn)(void);
} spawn_t;

spawn_t spawns[] =
{
	{ 0,                           0                         },
	{ "path_corner_2",             SP_path_corner_2          },
	{ "info_train_spline_main",    SP_info_train_spline_main },
	{ "info_train_spline_control", SP_path_corner_2          },

	{ "trigger_objective_info",    SP_trigger_objective_info },
	{ "misc_gamemodel",            SP_misc_gamemodel         },
	{ "corona",                    CG_corona                 },
};

#define NUMSPAWNS (int)(sizeof(spawns) / sizeof(spawn_t))

/**
 * @brief Spawn an entity and fill in all of the level fields from
 * cg.spawnVars[], then call the class specfic spawn function
 */
void CG_ParseEntityFromSpawnVars(void)
{
	int  i;
	char *classname;

	// check for "notteam" / "notfree" flags
	CG_SpawnInt("notteam", "0", &i);
	if (i)
	{
		return;
	}

	if (CG_SpawnString("classname", "", &classname))
	{
		for (i = 0; i < NUMSPAWNS; i++)
		{
			if (!Q_stricmp(spawns[i].name, classname))
			{
				spawns[i].spawn();
				break;
			}
		}
	}
}

/**
 * @brief CG_AddSpawnVarToken
 * @param[in] string
 * @return
 */
char *CG_AddSpawnVarToken(const char *string)
{
	size_t l;
	char   *dest;

	l = strlen(string);
	if (cg.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS)
	{
		CG_Error("CG_AddSpawnVarToken: MAX_SPAWN_VARS\n");
	}

	dest = cg.spawnVarChars + cg.numSpawnVarChars;
	Com_Memcpy(dest, string, l + 1);

	cg.numSpawnVarChars += l + 1;

	return dest;
}

/**
 * @brief Parses a brace bounded set of key / value pairs out of the
 * level's entity strings into cg.spawnVars[]
 *
 * @details This does not actually spawn an entity.
 * @return
 */
qboolean CG_ParseSpawnVars(void)
{
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	cg.numSpawnVars     = 0;
	cg.numSpawnVarChars = 0;

	// parse the opening brace
	if (!trap_GetEntityToken(com_token, sizeof(com_token)))
	{
		// end of spawn string
		return qfalse;
	}
	if (com_token[0] != '{')
	{
		CG_Error("CG_ParseSpawnVars: found %s when expecting {\n", com_token);
	}

	// go through all the key / value pairs
	while (1)
	{
		// parse key
		if (!trap_GetEntityToken(keyname, sizeof(keyname)))
		{
			CG_Error("CG_ParseSpawnVars: EOF without closing brace\n");
		}

		if (keyname[0] == '}')
		{
			break;
		}

		// parse value
		if (!trap_GetEntityToken(com_token, sizeof(com_token)))
		{
			CG_Error("CG_ParseSpawnVars: EOF without closing brace\n");
		}

		if (com_token[0] == '}')
		{
			CG_Error("CG_ParseSpawnVars: closing brace without data\n");
		}
		if (cg.numSpawnVars == MAX_SPAWN_VARS)
		{
			CG_Error("CG_ParseSpawnVars: MAX_SPAWN_VARS\n");
		}
		cg.spawnVars[cg.numSpawnVars][0] = CG_AddSpawnVarToken(keyname);
		cg.spawnVars[cg.numSpawnVars][1] = CG_AddSpawnVarToken(com_token);
		cg.numSpawnVars++;
	}

	return qtrue;
}

/**
 * @brief SP_worldspawn
 */
void SP_worldspawn(void)
{
	char   *s;
	int    i;
	vec3_t vec;

	CG_SpawnString("classname", "", &s);
	if (Q_stricmp(s, "worldspawn"))
	{
		CG_Error("SP_worldspawn: The first entity isn't 'worldspawn'\n");
	}

	cgs.ccLayers = 0;

	if (CG_SpawnVector2D("mapcoordsmins", "-128 128", cg.mapcoordsMins) &&     // top left
	    CG_SpawnVector2D("mapcoordsmaxs", "128 -128", cg.mapcoordsMaxs))       // bottom right
	{
		cg.mapcoordsValid = qtrue;
	}
	else
	{
		cg.mapcoordsValid = qfalse;
	}

	CG_ParseSpawns();

	CG_SpawnString("cclayers", "0", &s);
	cgs.ccLayers = Q_atoi(s);

	// Make sure the maximum commandmaps, doesn't overflow
	// the maximum allowed command map layers
	if (cgs.ccLayers > MAX_COMMANDMAP_LAYERS)
	{
		cgs.ccLayers = MAX_COMMANDMAP_LAYERS;
		CG_Printf("^3Warning: The maximum number (%i) of command map layers is exceeded.\n",
		          MAX_COMMANDMAP_LAYERS);
	}

	for (i = 0; i < cgs.ccLayers; i++)
	{
		CG_SpawnString(va("cclayerceil%i", i), "0", &s);
		cgs.ccLayerCeils[i] = Q_atoi(s);
	}

	cg.mapcoordsScale[0] = 1 / (cg.mapcoordsMaxs[0] - cg.mapcoordsMins[0]);
	cg.mapcoordsScale[1] = 1 / (cg.mapcoordsMaxs[1] - cg.mapcoordsMins[1]);

	BG_InitLocations(cg.mapcoordsMins, cg.mapcoordsMaxs);

	CG_SpawnString("atmosphere", "", &s);
	CG_EffectParse(s);

	cg.fiveMinuteSound_g[0]                       = \
		cg.fiveMinuteSound_a[0]                   = \
			cg.twoMinuteSound_g[0]                = \
				cg.twoMinuteSound_a[0]            = \
					cg.thirtySecondSound_g[0]     = \
						cg.thirtySecondSound_a[0] = '\0';

	CG_SpawnString("fiveMinuteSound_axis", "axis_hq_5minutes", &s);
	Q_strncpyz(cg.fiveMinuteSound_g, s, sizeof(cg.fiveMinuteSound_g));
	CG_SpawnString("fiveMinuteSound_allied", "allies_hq_5minutes", &s);
	Q_strncpyz(cg.fiveMinuteSound_a, s, sizeof(cg.fiveMinuteSound_a));

	CG_SpawnString("twoMinuteSound_axis", "axis_hq_2minutes", &s);
	Q_strncpyz(cg.twoMinuteSound_g, s, sizeof(cg.twoMinuteSound_g));
	CG_SpawnString("twoMinuteSound_allied", "allies_hq_2minutes", &s);
	Q_strncpyz(cg.twoMinuteSound_a, s, sizeof(cg.twoMinuteSound_a));

	CG_SpawnString("thirtySecondSound_axis", "axis_hq_30seconds", &s);
	Q_strncpyz(cg.thirtySecondSound_g, s, sizeof(cg.thirtySecondSound_g));
	CG_SpawnString("thirtySecondSound_allied", "allies_hq_30seconds", &s);
	Q_strncpyz(cg.thirtySecondSound_a, s, sizeof(cg.thirtySecondSound_a));

	// 5 minute axis
	if (!*cg.fiveMinuteSound_g)
	{
		cgs.media.fiveMinuteSound_g = 0;
	}
	else if (strstr(cg.fiveMinuteSound_g, ".wav") || strstr(cg.fiveMinuteSound_g, ".ogg"))
	{
		cgs.media.fiveMinuteSound_g = trap_S_RegisterSound(cg.fiveMinuteSound_g, qfalse);
	}
	else
	{
		cgs.media.fiveMinuteSound_g = -1;
	}

	// 5 minute allied
	if (!*cg.fiveMinuteSound_a)
	{
		cgs.media.fiveMinuteSound_a = 0;
	}
	else if (strstr(cg.fiveMinuteSound_a, ".wav") || strstr(cg.fiveMinuteSound_a, ".ogg"))
	{
		cgs.media.fiveMinuteSound_a = trap_S_RegisterSound(cg.fiveMinuteSound_a, qfalse);
	}
	else
	{
		cgs.media.fiveMinuteSound_a = -1;
	}

	// 2 minute axis
	if (!*cg.twoMinuteSound_g)
	{
		cgs.media.twoMinuteSound_g = 0;
	}
	else if (strstr(cg.twoMinuteSound_g, ".wav") || strstr(cg.twoMinuteSound_g, ".ogg"))
	{
		cgs.media.twoMinuteSound_g = trap_S_RegisterSound(cg.twoMinuteSound_g, qfalse);
	}
	else
	{
		cgs.media.twoMinuteSound_g = -1;
	}

	// 2 minute allied
	if (!*cg.twoMinuteSound_a)
	{
		cgs.media.twoMinuteSound_a = 0;
	}
	else if (strstr(cg.twoMinuteSound_a, ".wav") || strstr(cg.twoMinuteSound_a, ".ogg"))
	{
		cgs.media.twoMinuteSound_a = trap_S_RegisterSound(cg.twoMinuteSound_a, qtrue);
	}
	else
	{
		cgs.media.twoMinuteSound_a = -1;
	}

	// 30 seconds axis
	if (!*cg.thirtySecondSound_g)
	{
		cgs.media.thirtySecondSound_g = 0;
	}
	else if (strstr(cg.thirtySecondSound_g, ".wav") || strstr(cg.thirtySecondSound_g, ".ogg"))
	{
		cgs.media.thirtySecondSound_g = trap_S_RegisterSound(cg.thirtySecondSound_g, qfalse);
	}
	else
	{
		cgs.media.thirtySecondSound_g = -1;
	}

	// 30 seconds allied
	if (!*cg.thirtySecondSound_a)
	{
		cgs.media.thirtySecondSound_a = 0;
	}
	else if (strstr(cg.thirtySecondSound_a, ".wav") || strstr(cg.thirtySecondSound_a, ".ogg"))
	{
		cgs.media.thirtySecondSound_a = trap_S_RegisterSound(cg.thirtySecondSound_a, qfalse);
	}
	else
	{
		cgs.media.thirtySecondSound_a = -1;
	}

	// axis airstrike plane
	CG_SpawnString("airstrikePlane_axis", "models/mapobjects/etl_plane/junker88.md3", &s);

	if (!*s)
	{
		cgs.media.airstrikePlane[0] = trap_R_RegisterModel("models/mapobjects/etl_plane/junker88.md3");
	}
	else
	{
		cgs.media.airstrikePlane[0] = trap_R_RegisterModel(s); // axis
	}

	CG_SpawnVector("airstrikePlaneScale_axis", "0 0 0", vec);
	VectorCopy(vec, cg.airstrikePlaneScale[0]);

	// allies airstrike plane
	CG_SpawnString("airstrikePlane_allies", "models/mapobjects/etl_plane/b-25.md3", &s);

	if (!*s)
	{
		cgs.media.airstrikePlane[1] = trap_R_RegisterModel("models/mapobjects/etl_plane/b-25.md3");
	}
	else
	{
		cgs.media.airstrikePlane[1] = trap_R_RegisterModel(s);
	}

	CG_SpawnVector("airstrikePlaneScale_allies", "0 0 0", vec);
	VectorCopy(vec, cg.airstrikePlaneScale[1]);
}

/**
 * @brief Parses textual entity definitions out of an entstring and spawns gentities.
 */
void CG_ParseEntitiesFromString(void)
{
	// allow calls to CG_Spawn*()
	cg.spawning          = qtrue;
	cg.numSpawnVars      = 0;
	cg.numMiscGameModels = 0;
	cg.numCoronas        = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if (!CG_ParseSpawnVars())
	{
		CG_Error("ParseEntities: no entities\n");
	}
	SP_worldspawn();

	// parse ents
	while (CG_ParseSpawnVars())
	{
		CG_ParseEntityFromSpawnVars();
	}

	cg.spawning = qfalse;           // any future calls to CG_Spawn*() will be errors
}
