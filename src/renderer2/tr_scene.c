/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_scene.c
 */

#include "tr_local.h"

static int r_firstSceneDrawSurf;
static int r_firstSceneInteraction;

static int r_numLights;
static int r_firstSceneLight;

static int r_numcoronas;
static int r_firstSceneCorona;

static int r_numEntities;
static int r_firstSceneEntity;

static int r_numPolys;
static int r_firstScenePoly;

int r_numPolyVerts;

int r_firstScenePolybuffer;
int r_numPolybuffers;

// decals
int r_firstSceneDecalProjector;
int r_numDecalProjectors;
int r_firstSceneDecal;

/**
 * @brief R_InitNextFrame
 */
void R_InitNextFrame(void)
{
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf    = 0;
	r_firstSceneInteraction = 0;

	r_numLights       = 0;
	r_firstSceneLight = 0;

	r_numcoronas      = 0;
	r_firstSceneCorona = 0;

	r_numEntities      = 0;
	r_firstSceneEntity = 0;

	r_numPolys       = 0;
	r_firstScenePoly = 0;

	r_numPolyVerts = 0;

	r_numPolybuffers       = 0;
	r_firstScenePolybuffer = 0;

	// decals
	r_numDecalProjectors       = 0;
	r_firstSceneDecalProjector = 0;
	r_firstSceneDecal          = 0;
}

/**
 * @brief RE_ClearScene
 */
void RE_ClearScene(void)
{
	r_firstSceneLight  = r_numLights;
	r_firstSceneCorona = r_numcoronas;
	r_firstSceneEntity = r_numEntities;
	r_firstScenePoly   = r_numPolys;
}

/*
===========================================================================
DISCRETE POLYS
===========================================================================
*/

/**
 * @brief Adds all the scene's polys into this view's drawsurf list
 */
void R_AddPolygonSurfaces(void)
{
	int       i;
	shader_t  *sh;
	srfPoly_t *poly;

	if (!r_drawpolies->integer)
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	for (i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys; i++, poly++)
	{
		sh = R_GetShaderByHandle(poly->hShader);
		R_AddDrawSurf((surfaceType_t *)poly, sh, LIGHTMAP_NONE, poly->fogIndex);
	}
}

/**
 * @brief Adds all the scene's polys into this view's drawsurf list
 */
void R_AddPolygonBufferSurfaces(void)
{
	int             i;
	shader_t        *sh;
	srfPolyBuffer_t *polybuffer;

	tr.currentEntity = &tr.worldEntity;

	for (i = 0, polybuffer = tr.refdef.polybuffers; i < tr.refdef.numPolybuffers; i++, polybuffer++)
	{
		sh = R_GetShaderByHandle(polybuffer->pPolyBuffer->shader);

		//R_AddDrawSurf((void *)polybuffer, sh, polybuffer->fogIndex, 0, 0);
		R_AddDrawSurf((surfaceType_t *)polybuffer, sh, LIGHTMAP_NONE, polybuffer->fogIndex);
	}
}

/**
 * @brief R_AddPolysToScene
 * @param[in] hShader
 * @param[in] numVerts
 * @param[in] verts
 * @param[in] numPolys
 */
static void R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys)
{
	srfPoly_t *poly;
	int       i, j;
	int       fogIndex;
	fog_t     *fog;
	vec3_t    bounds[2];

	if (!tr.registered)
	{
		return;
	}

	if (!r_drawpolies->integer)
	{
		return;
	}

	if (!hShader)
	{
		Ren_Developer("WARNING: RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	for (j = 0; j < numPolys; j++)
	{
		if (r_numPolyVerts + numVerts >= r_maxPolyVerts->integer || r_numPolys >= r_maxPolys->integer)
		{
			/*
			   NOTE TTimo this was initially a PRINT_WARNING
			   but it happens a lot with high fighting scenes and particles
			   since we don't plan on changing the const and making for room for those effects
			   simply cut this message to developer only
			 */
			Ren_Developer("WARNING: RE_AddPolyToScene: r_maxPolyVerts or r_maxPolys reached\n");
			return;
		}

		poly              = &backEndData->polys[r_numPolys];
		poly->surfaceType = SF_POLY;
		poly->hShader     = hShader;
		poly->numVerts    = numVerts;
		poly->verts       = &backEndData->polyVerts[r_numPolyVerts];

		Com_Memcpy(poly->verts, &verts[numVerts * j], numVerts * sizeof(*verts));

		// done.
		r_numPolys++;
		r_numPolyVerts += numVerts;

		// if no world is loaded
		if (tr.world == NULL)
		{
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if (tr.world->numFogs == 1)
		{
			fogIndex = 0;
		}
		else
		{
			// find which fog volume the poly is in
			VectorCopy(poly->verts[0].xyz, bounds[0]);
			VectorCopy(poly->verts[0].xyz, bounds[1]);

			for (i = 1; i < poly->numVerts; i++)
			{
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			}

			for (fogIndex = 1; fogIndex < tr.world->numFogs; fogIndex++)
			{
				fog = &tr.world->fogs[fogIndex];


				if (BoundsIntersect(bounds[0], bounds[1], fog->bounds[0], fog->bounds[1]))
				{
					break;
				}
			}

			if (fogIndex == tr.world->numFogs)
			{
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}

/**
 * @brief RE_AddPolyToScene
 * @param[in] hShader
 * @param[in] numVerts
 * @param[in] verts
 */
void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts)
{
	R_AddPolysToScene(hShader, numVerts, verts, 1);
}

/**
 * @brief RE_AddPolysToScene
 * @param[in] hShader
 * @param[in] numVerts
 * @param[in] verts
 * @param[in] numPolys
 */
void RE_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys)
{
	R_AddPolysToScene(hShader, numVerts, verts, numPolys);
}

/**
 * @brief RE_AddPolyBufferToScene
 * @param[in] pPolyBuffer
 */
void RE_AddPolyBufferToScene(polyBuffer_t *pPolyBuffer)
{
	srfPolyBuffer_t *pPolySurf;
	int             fogIndex;
	fog_t           *fog;
	vec3_t          bounds[2];
	int             i;

	if (!r_drawpolies->integer)
	{
		return;
	}

	if (r_numPolybuffers >= MAX_POLYBUFFERS)
	{
		Ren_Warning("WARNING RE_AddPolyBufferToScene: MAX_POLYBUFFERS (%d) reached\n", MAX_POLYBUFFERS);
		return;
	}

	pPolySurf = &backEndData->polybuffers[r_numPolybuffers];
	r_numPolybuffers++;

	pPolySurf->surfaceType = SF_POLYBUFFER;
	pPolySurf->pPolyBuffer = pPolyBuffer;

	VectorCopy(pPolyBuffer->xyz[0], bounds[0]);
	VectorCopy(pPolyBuffer->xyz[0], bounds[1]);
	for (i = 1; i < pPolyBuffer->numVerts; i++)
	{
		AddPointToBounds(pPolyBuffer->xyz[i], bounds[0], bounds[1]);
	}

	for (fogIndex = 1; fogIndex < tr.world->numFogs; fogIndex++)
	{
		fog = &tr.world->fogs[fogIndex];

		if (BoundsIntersect(bounds[0], bounds[1], fog->bounds[0], fog->bounds[1]))
		{
			break;
		}
	}
	if (fogIndex == tr.world->numFogs)
	{
		fogIndex = 0;
	}

	pPolySurf->fogIndex = fogIndex;
}

//=================================================================================

/**
 * @brief RE_AddRefEntityToScene
 * @param[in] ent
 */
void RE_AddRefEntityToScene(const refEntity_t *ent)
{
	if (!tr.registered)
	{
		return;
	}

	// fixed was ENTITYNUM_WORLD
	if (r_numEntities >= MAX_REFENTITIES)
	{
		// some servers/mods might throw this - let's see what we are missing in the scene
		Ren_Developer("WARNING RE_AddRefEntityToScene: Dropping refEntity [%i] model '%s', reached MAX_REFENTITIES\n", ent->entityNum, R_GetModelByHandle(ent->hModel)->name);
		return;
	}

	if (Q_isnan(ent->origin[0]) || Q_isnan(ent->origin[1]) || Q_isnan(ent->origin[2]))
	{
		static qboolean firstTime = qtrue;

		if (firstTime)
		{
			firstTime = qfalse;
			Ren_Print("WARNING RE_AddRefEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}

	if ((int)ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE)
	{
		Ren_Drop("RE_AddRefEntityToScene: bad reType %i", ent->reType);
	}

	Com_Memcpy(&backEndData->entities[r_numEntities].e, ent, sizeof(refEntity_t));
	//backEndData->entities[r_numentities].e = *ent;
	backEndData->entities[r_numEntities].lightingCalculated = qfalse;

	r_numEntities++;
}

#if defined(USE_REFLIGHT)
/**
 * @brief RE_AddRefLightToScene
 * @param[in] l
 */
void RE_AddRefLightToScene(const refLight_t *light)
{
	trRefLight_t *trlight;

	if (!tr.registered)
	{
		return;
	}

	if (r_numLights >= MAX_REF_LIGHTS)
	{
		Ren_Print("WARNING RE_AddRefLightToScene: Dropping light, reached MAX_REF_LIGHTS\n");
		return;
	}

	if (light->radius[0] <= 0 && VectorLength(light->radius) == 0.f && VectorLength(light->projTarget) == 0.f)
	{
		return;
	}

	if ((unsigned)light->rlType >= RL_MAX_REF_LIGHT_TYPE)
	{
		Ren_Drop("RE_AddRefLightToScene: bad rlType %i", light->rlType);
	}

	if (r_numLights >= MAX_REF_LIGHTS)
	{
		Ren_Developer("WARNING RE_AddRefLightToScene: Dropping light, reached MAX_REF_LIGHTS\n");
		return;
	}

	trlight = &backEndData->lights[r_numLights++];
	Com_Memcpy(&trlight->l, light, sizeof(trlight->l));

	trlight->isStatic = qfalse;
	trlight->additive = qtrue;

	trlight->l.scale *= r_lightScale->value; // cvar for dynamic scaling only?

	if (trlight->l.scale < 0)
	{
		trlight->l.scale = r_lightScale->value;
	}

	if (!HDR_ENABLED())
	{
		if (trlight->l.scale > r_lightScale->value)
		{
			trlight->l.scale = r_lightScale->value;
		}
	}

	if (!r_dynamicLightShadows->integer && !trlight->l.inverseShadows)
	{
		trlight->l.noShadows = qtrue;
	}
}
#endif

/**
 * @brief R_AddWorldLightsToScene
 */
static void R_AddWorldLightsToScene()
{
	int          i;
	trRefLight_t *light;

	if (!tr.registered)
	{
		return;
	}

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	for (i = 0; i < tr.world->numLights; i++)
	{
		light = tr.currentLight = &tr.world->lights[i];

		if (r_numLights >= MAX_REF_LIGHTS)
		{
			Ren_Developer("WARNING R_AddWorldLightsToScene: Dropping light, reached MAX_REF_LIGHTS\n");
			return;
		}

		/*
		   if(light->radius[0] <= 0 && !VectorLength(light->radius) && light->distance <= 0)
		   {
		   continue;
		   }
		 */

		if (!light->firstInteractionCache)
		{
			// this light has no interactions precached
			continue;
		}

		Com_Memcpy(&backEndData->lights[r_numLights], light, sizeof(trRefLight_t));
		r_numLights++;
	}
}

/**
 * @brief modified dlight system to support seperate radius and intensity
 * @param[in] org
 * @param[in] radius
 * @param[in] intensity
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param hShader
 * @param flags   - unused
 *
 * @note vanilla mods deliver 0 hShader only
 */
void RE_AddDynamicLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags)
{
	trRefLight_t *light;

	if (!tr.registered)
	{
		return;
	}

	if (r_numLights >= MAX_REF_LIGHTS)
	{
		Ren_Print("WARNING RE_AddDynamicLightToScene: Dropping light, reached MAX_REF_LIGHTS\n");
		return;
	}

	if (intensity <= 0 || radius <= 0)
	{
		return;
	}

	light = &backEndData->lights[r_numLights++];

	light->l.rlType = RL_OMNI;

	VectorCopy(org, light->l.origin);

	QuatClear(light->l.rotation);
	VectorClear(light->l.center);

	light->shader = R_GetShaderByHandle(hShader);

	light->l.attenuationShader = 0;

	light->l.radius[0] = radius;
	light->l.radius[1] = radius;
	light->l.radius[2] = radius;

	light->l.color[0] = r;
	light->l.color[1] = g;
	light->l.color[2] = b;

	light->l.noShadows      = r_dynamicLightShadows->integer ? qfalse : qtrue;
	light->l.inverseShadows = qfalse;

	light->isStatic = qfalse;
	light->additive = qtrue;

	light->l.scale = intensity * r_lightScale->value; // cvar for dynamic only?

	if (light->l.scale < 0)
	{
		light->l.scale = r_lightScale->value;
	}
}

/**
 * @brief RE_AddCoronaToScene
 * @param[in] org
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] scale
 * @param[in] id
 * @param[in] visible
 */
void RE_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible)
{
	corona_t *cor;

	if (!tr.registered)
	{
		return;
	}

	if (!visible)
	{
		return;
	}

	if (r_numcoronas >= MAX_CORONAS)
	{
		Ren_Developer("WARNING RE_AddCoronaToScene: Dropping corona, reached MAX_CORONAS\n");
		return;
	}

	cor = &backEndData->coronas[r_numcoronas++];
	VectorCopy(org, cor->origin);
	cor->color[0] = r;
	cor->color[1] = g;
	cor->color[2] = b;
	cor->scale    = scale;
	cor->id       = id;
	cor->visible  = visible;
}

/**
 * @brief Draw a 3D view into a part of the window, then return
 * to 2D drawing.
 *
 * Rendering a scene may require multiple views to be rendered
 * to handle mirrors.
 *
 * @param[in] fd
 */
void RE_RenderScene(const refdef_t *fd)
{
	viewParms_t parms;
	int         startTime;

	if (!tr.registered)
	{
		return;
	}
	Ren_LogComment("====== RE_RenderScene =====\n");

	if (r_noreFresh->integer)
	{
		return;
	}

	startTime = ri.Milliseconds();

	if (!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL))
	{
		Ren_Drop("R_RenderScene: NULL worldmodel");
	}

	Com_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

	tr.refdef.x      = fd->x;
	tr.refdef.y      = fd->y;
	tr.refdef.width  = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x  = fd->fov_x;
	tr.refdef.fov_y  = fd->fov_y;

	VectorCopy(fd->vieworg, tr.refdef.vieworg);
	VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
	VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
	VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

	tr.refdef.time    = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	if(fd->rdflags & RDF_SKYBOXPORTAL)
	{
	    tr.world->hasSkyboxPortal = qtrue; // see int skyboxportal var tr_scene.c/r1
	}

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL) && !((tr.refdef.rdflags & RDF_SKYBOXPORTAL) && tr.world && tr.world->numSkyNodes > 0))
	{
		int areaDiff;
		int i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0; i < MAX_MAP_AREA_BYTES / 4; i++)
		{
			areaDiff                      |= ((int *)tr.refdef.areamask)[i] ^ ((int *)fd->areamask)[i];
			((int *)tr.refdef.areamask)[i] = ((int *)fd->areamask)[i];
		}

		if (areaDiff)
		{
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}

	R_AddWorldLightsToScene();

	// derived info
	tr.refdef.floatTime = tr.refdef.time * 0.001;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs    = backEndData->drawSurfs;

	tr.refdef.numInteractions = r_firstSceneInteraction;
	tr.refdef.interactions    = backEndData->interactions;

	tr.refdef.numEntities = r_numEntities - r_firstSceneEntity;
	tr.refdef.entities    = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.numLights = r_numLights - r_firstSceneLight;
	tr.refdef.lights    = &backEndData->lights[r_firstSceneLight];

	tr.refdef.num_coronas = r_numcoronas - r_firstSceneCorona;
	tr.refdef.coronas     = &backEndData->coronas[r_firstSceneCorona];

	tr.refdef.numPolys = r_numPolys - r_firstScenePoly;
	tr.refdef.polys    = &backEndData->polys[r_firstScenePoly];

	tr.refdef.numPolybuffers = r_numPolybuffers - r_firstScenePolybuffer;
	tr.refdef.polybuffers    = &backEndData->polybuffers[r_firstScenePolybuffer];

	tr.refdef.numDecalProjectors = r_numDecalProjectors - r_firstSceneDecalProjector;
	tr.refdef.decalProjectors    = &backEndData->decalProjectors[r_firstSceneDecalProjector];

	tr.refdef.numDecals = 0;
	tr.refdef.decals    = &backEndData->decals[r_firstSceneDecal];


	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// a scene can have multiple views caused by mirrors or portals
	// the number of views is restricted so we can use hardware occlusion queries
	// and put them into the BSP nodes for each view
	tr.viewCount = -1;

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset(&parms, 0, sizeof(parms));

#if 1
	if (tr.refdef.pixelTarget == NULL)
	{
		parms.viewportX = tr.refdef.x;
		parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	}
	else
	{
		// Driver bug, if we try and do pixel target work along the top edge of a window
		// we can end up capturing part of the status bar. (see screenshot corruption..)
		// Soooo.. use the middle.
		parms.viewportX = glConfig.vidWidth / 2;
		parms.viewportY = glConfig.vidHeight / 2;
	}
#else
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
#endif

	parms.viewportWidth  = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;

	Vector4Set(parms.viewportVerts[0], parms.viewportX, parms.viewportY, 0, 1);
	Vector4Set(parms.viewportVerts[1], parms.viewportX + parms.viewportWidth, parms.viewportY, 0, 1);
	Vector4Set(parms.viewportVerts[2], parms.viewportX + parms.viewportWidth, parms.viewportY + parms.viewportHeight, 0, 1);
	Vector4Set(parms.viewportVerts[3], parms.viewportX, parms.viewportY + parms.viewportHeight, 0, 1);

	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy(fd->vieworg, parms.orientation.origin);
	VectorCopy(fd->viewaxis[0], parms.orientation.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.orientation.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.orientation.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	R_RenderView(&parms);

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf    = tr.refdef.numDrawSurfs;
	r_firstSceneInteraction = tr.refdef.numInteractions;
	r_firstSceneDecal      += tr.refdef.numDecals;
	r_firstSceneEntity      = r_numEntities;
	r_firstSceneLight       = r_numLights;
	r_firstScenePoly        = r_numPolys;
	r_firstScenePolybuffer  = r_numPolybuffers;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}
