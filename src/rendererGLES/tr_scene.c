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
 * @file rendererGLES/tr_scene.c
 */

#include "tr_local.h"

int r_firstSceneDrawSurf;

int r_numdlights;
int r_firstSceneDlight;

int r_numcoronas;
int r_firstSceneCorona;

int r_numentities;
int r_firstSceneEntity;

int r_numpolys;
int r_firstScenePoly;

int r_numpolyverts;

// TESTING
int r_firstScenePolybuffer;
int r_numpolybuffers;

// decals
int r_firstSceneDecalProjector;
int r_numDecalProjectors;
int r_firstSceneDecal;

int skyboxportal;

/**
 * @brief R_InitNextFrame
 */
void R_InitNextFrame(void)
{
	backEndData->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights       = 0;
	r_firstSceneDlight = 0;

	r_numcoronas       = 0;
	r_firstSceneCorona = 0;

	r_numentities      = 0;
	r_firstSceneEntity = 0;

	r_numpolys       = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;

	// TESTING
	r_numpolybuffers       = 0;
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
	// clear everything else
	r_firstSceneDlight = r_numdlights;
	r_firstSceneCorona = r_numcoronas;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly   = r_numpolys;
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

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	for (i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys ; i++, poly++)
	{
		sh = R_GetShaderByHandle(poly->hShader);

		R_AddDrawSurf(( void * )poly, sh, poly->fogIndex, 0, 0);
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
	srfPoly_t *poly;
	int       fogIndex;
	fog_t     *fog;
	vec3_t    bounds[2];

	if (!tr.registered)
	{
		return;
	}

	if (!hShader)
	{
		Ren_Warning("WARNING RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	if (((r_numpolyverts + numVerts) >= r_maxPolyVerts->integer) || (r_numpolys >= r_maxPolys->integer))
	{
		Ren_Developer("WARNING RE_AddPolyToScene: r_maxpolyverts or r_maxpolys reached\n");
		return;
	}

	poly              = &backEndData->polys[r_numpolys];
	poly->surfaceType = SF_POLY;
	poly->hShader     = hShader;
	poly->numVerts    = numVerts;
	poly->verts       = &backEndData->polyVerts[r_numpolyverts];

	Com_Memcpy(poly->verts, verts, numVerts * sizeof(*verts));

	r_numpolys++;
	r_numpolyverts += numVerts;

	// see if it is in a fog volume
	if (tr.world->numfogs == 1)
	{
		fogIndex = 0;
	}
	else
	{
		int i;

		// find which fog volume the poly is in
		VectorCopy(poly->verts[0].xyz, bounds[0]);
		VectorCopy(poly->verts[0].xyz, bounds[1]);
		for (i = 1 ; i < poly->numVerts ; i++)
		{
			AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
		}
		for (fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++)
		{
			fog = &tr.world->fogs[fogIndex];
			if (bounds[1][0] >= fog->bounds[0][0]
			    && bounds[1][1] >= fog->bounds[0][1]
			    && bounds[1][2] >= fog->bounds[0][2]
			    && bounds[0][0] <= fog->bounds[1][0]
			    && bounds[0][1] <= fog->bounds[1][1]
			    && bounds[0][2] <= fog->bounds[1][2])
			{
				break;
			}
		}
		if (fogIndex == tr.world->numfogs)
		{
			fogIndex = 0;
		}
	}
	poly->fogIndex = fogIndex;
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
	srfPoly_t *poly;
	int       i;
	int       fogIndex;
	fog_t     *fog;
	vec3_t    bounds[2];
	int       j;

	if (!tr.registered)
	{
		return;
	}

	if (!hShader)
	{
		Ren_Warning("WARNING RE_AddPolysToScene: NULL poly shader\n");
		return;
	}

	for (j = 0; j < numPolys; j++)
	{
		if (r_numpolyverts + numVerts >= r_maxPolyVerts->integer)
		{
			Ren_Developer("WARNING RE_AddPolysToScene: r_maxpolyverts[%i] reached. r_numpolyverts: %i - numVerts: %i - numPolys %i - shader %i\n", r_maxPolyVerts->integer, r_numpolyverts, numVerts, numPolys, hShader);
			return;
		}
		if (r_numpolys >= r_maxPolys->integer)
		{
			Ren_Developer("WARNING RE_AddPolysToScene: r_maxpolys[%i] reached. r_numpolys: %i\n", r_maxPolys->integer, r_numpolys);
			return;
		}

		poly              = &backEndData->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader     = hShader;
		poly->numVerts    = numVerts;
		poly->verts       = &backEndData->polyVerts[r_numpolyverts];

		Com_Memcpy(poly->verts, &verts[numVerts * j], numVerts * sizeof(*verts));

		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		if (tr.world == NULL)
		{
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if (tr.world->numfogs == 1)
		{
			fogIndex = 0;
		}
		else
		{
			// find which fog volume the poly is in
			VectorCopy(poly->verts[0].xyz, bounds[0]);
			VectorCopy(poly->verts[0].xyz, bounds[1]);
			for (i = 1 ; i < poly->numVerts ; i++)
			{
				AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
			}
			for (fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++)
			{
				fog = &tr.world->fogs[fogIndex];
				if (bounds[1][0] >= fog->bounds[0][0]
				    && bounds[1][1] >= fog->bounds[0][1]
				    && bounds[1][2] >= fog->bounds[0][2]
				    && bounds[0][0] <= fog->bounds[1][0]
				    && bounds[0][1] <= fog->bounds[1][1]
				    && bounds[0][2] <= fog->bounds[1][2])
				{
					break;
				}
			}
			if (fogIndex == tr.world->numfogs)
			{
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
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

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	for (i = 0, polybuffer = tr.refdef.polybuffers; i < tr.refdef.numPolyBuffers ; i++, polybuffer++)
	{
		sh = R_GetShaderByHandle(polybuffer->pPolyBuffer->shader);

		R_AddDrawSurf(( void * )polybuffer, sh, polybuffer->fogIndex, 0, 0);
	}
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

	if (r_numpolybuffers >= MAX_POLYBUFFERS)
	{
		Ren_Warning("WARNING RE_AddPolyBufferToScene: MAX_POLYBUFFERS (%d) reached\n", MAX_POLYBUFFERS);
		return;
	}

	pPolySurf = &backEndData->polybuffers[r_numpolybuffers];
	r_numpolybuffers++;

	pPolySurf->surfaceType = SF_POLYBUFFER;
	pPolySurf->pPolyBuffer = pPolyBuffer;

	VectorCopy(pPolyBuffer->xyz[0], bounds[0]);
	VectorCopy(pPolyBuffer->xyz[0], bounds[1]);
	for (i = 1 ; i < pPolyBuffer->numVerts ; i++)
	{
		AddPointToBounds(pPolyBuffer->xyz[i], bounds[0], bounds[1]);
	}
	for (fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++)
	{
		fog = &tr.world->fogs[fogIndex];
		if (bounds[1][0] >= fog->bounds[0][0]
		    && bounds[1][1] >= fog->bounds[0][1]
		    && bounds[1][2] >= fog->bounds[0][2]
		    && bounds[0][0] <= fog->bounds[1][0]
		    && bounds[0][1] <= fog->bounds[1][1]
		    && bounds[0][2] <= fog->bounds[1][2])
		{
			break;
		}
	}
	if (fogIndex == tr.world->numfogs)
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

	// NOTE: fixed was ENTITYNUM_WORLD
	if (r_numentities >= MAX_REFENTITIES)
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

	backEndData->entities[r_numentities].e                  = *ent;
	backEndData->entities[r_numentities].lightingCalculated = qfalse;

	r_numentities++;

	// add projected shadows for this model
	R_AddModelShadow(ent);
}

/**
 * @brief Modified dlight system to support seperate radius and intensity
 * @param[in] org
 * @param[in] radius
 * @param[in] intensity
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] hShader
 * @param[in] flags
 */
void RE_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags)
{
	dlight_t *dl;

	// early out
	if (!tr.registered  || radius <= 0 || intensity <= 0)
	{
		return;
	}

	if (r_numdlights >= MAX_DLIGHTS)
	{
		Ren_Print("WARNING RE_AddLightToScene: Dropping dlight, reached MAX_DLIGHTS\n");
		return;
	}

	// allow us to force some dlights under all circumstances
	if (!(flags & REF_FORCE_DLIGHT))
	{
		if (r_dynamicLight->integer == 0)
		{
			return;
		}
	}

	// set up a new dlight
	dl = &backEndData->dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	VectorCopy(org, dl->transformed);
	dl->radius             = radius;
	dl->radiusInverseCubed = (1.0f / dl->radius);
	dl->radiusInverseCubed = dl->radiusInverseCubed * dl->radiusInverseCubed * dl->radiusInverseCubed;
	dl->intensity          = intensity;
	dl->color[0]           = r;
	dl->color[1]           = g;
	dl->color[2]           = b;
	dl->shader             = R_GetShaderByHandle(hShader);
	if (dl->shader == tr.defaultShader)
	{
		dl->shader = NULL;
	}
	dl->flags = flags;
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

	if (fd->rdflags & RDF_SKYBOXPORTAL)
	{
		skyboxportal = 1;
	}

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL))
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

	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs    = backEndData->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities     = &backEndData->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights     = &backEndData->dlights[r_firstSceneDlight];
	tr.refdef.dlightBits  = 0;

	tr.refdef.num_coronas = r_numcoronas - r_firstSceneCorona;
	tr.refdef.coronas     = &backEndData->coronas[r_firstSceneCorona];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys    = &backEndData->polys[r_firstScenePoly];

	tr.refdef.numPolyBuffers = r_numpolybuffers - r_firstScenePolybuffer;
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

	// setup view parms for the initial view
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	Com_Memset(&parms, 0, sizeof(parms));
	parms.viewportX      = tr.refdef.x;
	parms.viewportY      = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
	parms.viewportWidth  = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal       = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy(fd->vieworg, parms.orientation.origin);
	VectorCopy(fd->viewaxis[0], parms.orientation.axis[0]);
	VectorCopy(fd->viewaxis[1], parms.orientation.axis[1]);
	VectorCopy(fd->viewaxis[2], parms.orientation.axis[2]);

	VectorCopy(fd->vieworg, parms.pvsOrigin);

	R_RenderView(&parms);

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf   = tr.refdef.numDrawSurfs;
	r_firstSceneDecal     += tr.refdef.numDecals;
	r_firstSceneEntity     = r_numentities;
	r_firstSceneDlight     = r_numdlights;
	r_firstScenePoly       = r_numpolys;
	r_firstScenePolybuffer = r_numpolybuffers;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}
