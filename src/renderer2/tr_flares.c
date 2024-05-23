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
 * @file renderer2/tr_flares.c
 * @brief LIGHT FLARES
 *
 * A light flare is an effect that takes place inside the eye when bright light
 * sources are visible.  The size of the flare reletive to the screen is nearly
 * constant, irrespective of distance, but the intensity should be proportional
 * to the projected area of the light source.
 *
 * A surface that has been flagged as having a light flare will calculate the
 * depth buffer value that it's midpoint should have when the surface is added.
 *
 * After all opaque surfaces have been rendered, the depth buffer is read back
 * for each flare in view.  If the point has not been obscured by a closer
 * surface, the flare should be drawn.
 *
 * Surfaces that have a repeated texture should never be flagged as flaring,
 * because there will only be a single flare added at the midpoint of the
 * polygon.
 *
 * To prevent abrupt popping, the intensity of the flare is interpolated up and
 * down as it changes visibility.  This involves scene to scene state, unlike
 * almost all other aspects of the renderer, and is complicated by the fact that
 * a single frame may have multiple scenes.
 *
 * RB_RenderFlares() will be called once per view (twice in a mirrored scene,
 * potentially up to five or more times in a frame with 3D status bar icons).
 */

#include "tr_local.h"

// we do not want to render the query surfaces as 3D objects..
//#define FLARE_QUERY_3D   1

/**
 * @struct flare_s
 * @typedef flare_t
 * @brief Flare states maintain visibility over multiple frames for fading
 * layers: view, mirror, menu
 */
typedef struct flare_s
{
	struct flare_s *next;           ///< for active chain

	int addedFrame;

	qboolean inPortal;              ///< true if in a portal view of the scene
	int frameSceneNum;
	void *surface;
	int fogNum;

	int fadeTime;

	qboolean cgvisible;             ///< for coronas, the client determines current visibility, but it's still inserted so it will fade out properly
	qboolean visible;               ///< state of last test
	float drawIntensity;            ///< may be non 0 even if !visible due to fading

	int windowX, windowY;           ///< the position of the flare in screen coordinates
	float windowZ;                  ///< the z depth of the 2D ortho surface in 3D perspective space
	float eyeZ;                     ///< the distance from camera/eye to the flare/corona origin, in world coordinates
	vec3_t color;

	qboolean isCorona;

	qboolean querying;              ///< occlusion query started on this flare
	uint32_t occlusionQueryObject;  ///< the handle to the query object
	uint32_t occlusionQuerySamples; ///< the result of the query (how many pixels are rendered)
	uint32_t flareSamples;          ///< the total number of samples in the flare
	vec4_t quadVerts[4];            ///< the quad surface of the query object
#if FLARE_QUERY_3D
	vec3_t origin;                  ///< the position of the flare/corona in world coordinates
#endif
} flare_t;

#define         MAX_FLARES 128

flare_t r_flareStructs[MAX_FLARES];
flare_t *r_activeFlares, *r_inactiveFlares;

int flareCoeff;
float flareSize; // half the size of a flare/corona (in screen coordinates)

// you start to see coronas/flares from this distance.
// At this distance, the drawIntensity is 0.0  (drawIntensity is 1.0 at the origin of the corona/flare)
#define CORONAFARDISTANCE 3000.f


/**
 * @brief R_ClearFlares
 */
void R_ClearFlares(void)
{
	int i;

	Com_Memset(r_flareStructs, 0, sizeof(r_flareStructs));
	r_activeFlares   = NULL;
	r_inactiveFlares = NULL;

	for (i = 0; i < MAX_FLARES; i++)
	{
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares       = &r_flareStructs[i];
	}
}

/**
 * @brief This is called at surface tesselation time
 * @param[in] surface
 * @param[in] fogNum
 * @param[in] point
 * @param[in] color
 * @param[in] normal
 * @param[in] visible
 * @param[in] isCorona
 * @param[in] flareHandle  (SF_FLARE surface flares have handle 0xFFFFFFFF)
 */
void RB_AddFlare(void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal, qboolean visible, qboolean isCorona, uint32_t flareHandle)
{
	int     i;
	flare_t *f;
	vec4_t  eye, clip, normalized, window;
	/*vec3_t  local;*/
	const float distBias = 512.0;
	const float distLerp = 0.5;
	float d1 = 0.0f, d2 = 0.0f;

	backEnd.pc.c_flareAdds++;

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip(point, backEnd.orientation.modelViewMatrix, backEnd.viewParms.projectionMatrix, eye, clip);

	// check to see if the point is completely off screen
	for (i = 0; i < 3; i++)
	{
		if (clip[i] >= clip[3] || clip[i] <= -clip[3])
		{
			return;
		}
	}

	R_TransformClipToWindow(clip, &backEnd.viewParms, normalized, window);

	if (window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth ||
	    window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight)
	{
		return; // shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	for (f = r_activeFlares; f; f = f->next)
	{
		if (f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum &&
		    f->inPortal == backEnd.viewParms.isPortal)
		{
			break;
		}
	}

	// allocate a new one
	if (!f)
	{
		if (!r_inactiveFlares)
		{
			// the list is completely full
			return;
		}

		f                = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next          = r_activeFlares;
		r_activeFlares   = f;

		f->surface       = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal      = backEnd.viewParms.isPortal;
		f->addedFrame    = -1;
		f->isCorona      = isCorona;

		// occlusion query object stuff:
		// provide a unique handle that doesn't change when flares are added, or removed from the list
		f->occlusionQueryObject = 0;
		if (r_occludeFlares->integer && glConfig2.occlusionQueryBits && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			if (flareHandle != 0xFFFFFFFF) // for now, skip surface-flares SF_FLARE
			{
				f->occlusionQueryObject = tr.occlusionQueryObjectsAsync[flareHandle];
			}
			f->occlusionQuerySamples = 0; // so many samples passed the test
			f->flareSamples = 228803; // tested this by debugging code and check results..
			f->querying = qfalse; // a query has not been started for this flare
#if FLARE_QUERY_3D
			VectorCopy(point, f->origin);
#endif

			// We must get the ortho z values for some perspecive projected query surface:
			//	   https://stackoverflow.com/questions/8990735/how-to-use-opengl-orthographic-projection-with-the-depth-buffer
			//     ndcZ = (2 * winZ) - 1
			//     clipZ = ndcZ
			//     cameraZ = ((clipZ + (zFar + zNear)/(zFar - zNear)) * (zFar - zNear))/-2
			//
			// That post on stackoverflow explains it perfectly.
			// RB_SetViewMVPM() sets the orthographic near & far distances to -99999.f & 99999.f
			// We find the z value of an ortho rendered object, so that the final z is in the same range as the perspective rendered world.
			// That way, the 2D rendered flares get obscured by the 3D world.
			// A 3D perspecive rendered object gets smaller the farther away it is, and bigger when it's closer to the camera.
			// That makes it inpractical to use the query results, because you never know how many samples were tested.
			// The 2D flares keep the same size, no matter at what distance they are from the camera.
			// This makes it ideal for testing how many query samples passed the test. The tested query surfaces are always of the same size.
			f->windowZ = ((window[2] * 2.f - 1.f) * (99999.f - -99999.f)) * -0.5f;
			// That^^ method produces almost correct results..
//!!!DEBUG!!! this windowZ value is not quite correct..
		}
	}

	f->cgvisible = visible;

	if (f->addedFrame != backEnd.viewParms.frameCount - 1)
	{
		f->visible  = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum     = fogNum;

	VectorCopy(color, f->color);
/*
	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	if (normal)
	{
		d2 = -eye[2];
		if (d2 > distBias)
		{
			if (d2 > (distBias * 2.0f))
			{
				d2 = (distBias * 2.0f);
			}
			d2 -= distBias;
			//d2  = 1.0f - (d2 * (1.0f / distBias));
			d2 = 1.0f - (d2 * rcp(distBias));
			d2 *= distLerp;
		}
		else
		{
			d2 = distLerp;
		}

		VectorSubtract(backEnd.viewParms.orientation.origin, point, local);
		VectorNormalizeOnly(local);
		//d1  = DotProduct(local, normal);
		Dot(local, normal, d1);
		d1 *= (1.0f - distLerp);
		d1 += d2;

		VectorScale(f->color, d1, f->color);
	}
*/
	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];

	f->eyeZ = eye[2];
}

/**
 * @brief RB_AddLightFlares
 */
void RB_AddLightFlares(void)
{
	int          i, j, k;
	trRefLight_t *l;
	fog_t        *fog;

	if (!r_flares->integer)
	{
		return;
	}

	l = backEnd.refdef.lights;
	//fog = tr.world->fogs;

	for (i = 0; i < backEnd.refdef.numLights; i++, l++)
	{
		if (!l->isStatic)
		{
			continue;
		}

		// find which fog volume the light is in
		for (j = 1; j < tr.world->numFogs; j++)
		{
			fog = &tr.world->fogs[j];
			for (k = 0; k < 3; k++)
			{
				if (l->l.origin[k] < fog->bounds[0][k] || l->l.origin[k] > fog->bounds[1][k])
				{
					break;
				}
			}
			if (k == 3)
			{
				break;
			}
		}

		if (j == tr.world->numFogs)
		{
			j = 0;
		}

		RB_AddFlare((void *)l, j, l->l.origin, l->l.color, NULL, qtrue, qfalse, i);
	}
}

/**
 * @brief RB_AddCoronaFlares
 */
void RB_AddCoronaFlares(void)
{
	corona_t *cor;
	int      i, j, k;
	fog_t    *fog;

	if (r_flares->integer != 1 && r_flares->integer != 3) // 3?
	{
		return;
	}

	if (!tr.world) // possible currently at the player model selection menu
	{
		return;
	}

	cor = backEnd.refdef.coronas;

	for (i = 0 ; i < backEnd.refdef.num_coronas ; i++, cor++)
	{
		// find which fog volume the corona is in
		for (j = 1 ; j < tr.world->numFogs ; j++)
		{
			fog = &tr.world->fogs[j];
			for (k = 0 ; k < 3 ; k++)
			{
				if (cor->origin[k] < fog->bounds[0][k] || cor->origin[k] > fog->bounds[1][k])
				{
					break;
				}
			}
			if (k == 3)
			{
				break;
			}
		}
		if (j == tr.world->numFogs)
		{
			j = 0;
		}
		RB_AddFlare((void *)cor, j, cor->origin, cor->color, NULL, cor->visible, qtrue, backEnd.refdef.num_coronas + i); // cor->scale -> eye
	}
}

/*
===============================================================================
FLARE BACK END
===============================================================================
*/

// flare occlusion query objects:

qboolean CreateFlareOcclusionSurface(flare_t *f)
{
	float size = flareSize;
#if FLARE_QUERY_3D
	vec3_t bounds[2];
#endif

	// the number of samples for this flare
	// Because the flare images are so big (a small flare in the center, and lots of pixels to fade the image out),
	// we use a smaller sized query object surface..
//	size *= 0.50;
//	f->flareSamples = (2.f * size) * (2.f * size);

#if FLARE_QUERY_3D
	// make a quad that is always facing the camera (a billboard)
	vec3_t left, up, q0, q1, q2, q3;
	VectorScale(backEnd.viewParms.orientation.axis[1], size, left);
	VectorScale(backEnd.viewParms.orientation.axis[2], size, up);

	VectorAdd(f->origin, left, q0);
	VectorAdd(f->origin, left, q1);
	VectorSubtract(f->origin, left, q2);
	VectorSubtract(f->origin, left, q3);

	VectorSubtract(q0, up, q0);
	VectorAdd(q1, up, q1);
	VectorAdd(q2, up, q2);
	VectorSubtract(q3, up, q3);
		
	Vector4Set(f->quadVerts[0], q0[0], q0[1], q0[2], 1.f);
	Vector4Set(f->quadVerts[1], q1[0], q1[1], q1[2], 1.f);
	Vector4Set(f->quadVerts[2], q2[0], q2[1], q2[2], 1.f);
	Vector4Set(f->quadVerts[3], q3[0], q3[1], q3[2], 1.f);

	// if the surface is completely off screen, the surface is invalid for querying
	VectorCopy(f->quadVerts[0], bounds[0]);
	VectorCopy(f->quadVerts[2], bounds[1]);
	if (R_CullBox(bounds) == CULL_OUT) {
		return qfalse;
	}
#else
//	https://stackoverflow.com/questions/8990735/how-to-use-opengl-orthographic-projection-with-the-depth-buffer

	Vector4Set(f->quadVerts[0], (float)f->windowX - size, (float)f->windowY - size, f->windowZ, 1.f);
	Vector4Set(f->quadVerts[1], (float)f->windowX - size, (float)f->windowY + size, f->windowZ, 1.f);
	Vector4Set(f->quadVerts[2], (float)f->windowX + size, (float)f->windowY + size, f->windowZ, 1.f);
	Vector4Set(f->quadVerts[3], (float)f->windowX + size, (float)f->windowY - size, f->windowZ, 1.f);
#endif
	return qtrue;
}

// render the occulsion query surface
static void RenderFlareOcclusionSurface(flare_t *f)
{
	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	Tess_AddQuadStamp2(f->quadVerts, colorBlue);
	Tess_UpdateVBOs(ATTR_POSITION); //tess.attribsSet);
	Tess_DrawElements();
}

// execute/start the query
static qboolean IssueFlareOcclusionQuery(flare_t *f)
{
	if (f->occlusionQueryObject == 0)
	{
		return qfalse;
	}

	// the surface must be valid to be used for a query
	if (!CreateFlareOcclusionSurface(f))
	{
		return qfalse;
	}

	GL_CheckErrors();

	// begin the occlusion query
	glBeginQuery(GL_SAMPLES_PASSED, f->occlusionQueryObject);

	GL_CheckErrors();

	RenderFlareOcclusionSurface(f);

	// end the query
	glEndQuery(GL_SAMPLES_PASSED);

	if (!glIsQuery(f->occlusionQueryObject))
	{
		Ren_Fatal("IssueFlareOcclusionQuery: flare has no occlusion query object in slot %i: %lu", backEnd.viewParms.viewCount, (unsigned long)f->occlusionQueryObject);
	}

	backEnd.pc.c_occlusionQueries++;

	GL_CheckErrors();

	return qtrue;
}

// check if the query is finished (and has a valid result)
static int FlareOcclusionResultAvailable(flare_t *f)
{
	GLint available = 0;

	if (f->occlusionQueryObject == 0)
	{
		return available;
	}

	glGetQueryObjectiv(f->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available);
	GL_CheckErrors();

	return available;
}

// get the result of a finished query
static void GetFlareOcclusionQueryResult(flare_t *f)
{
	uint32_t ocSamples;

	if (f->occlusionQueryObject == 0)
	{
		return;
	}

	// this function is only called when there is a valid query result. No need to explicitely check for an available result
	//if (!FlareOcclusionResultAvailable(f)) return;

	backEnd.pc.c_occlusionQueriesAvailable++;

	glGetQueryObjectuiv(f->occlusionQueryObject, GL_QUERY_RESULT, &ocSamples);

	GL_CheckErrors();

	f->occlusionQuerySamples = ocSamples;
}

// render all the needed queries from the list
void RB_RenderFlareOcclusionQueries()
{
	flare_t *f;
	flare_t **active;

	GL_CheckErrors();

	GL_PushMatrix();

#if FLARE_QUERY_3D
	GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
	backEnd.orientation = backEnd.viewParms.world;
	GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
#else
	RB_SetViewMVPM(); // orthogonal
#endif

	glVertexAttrib4f(ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f);

	SetMacrosAndSelectProgram(trProg.gl_genericShader,
								USE_ALPHA_TESTING, qfalse,
								USE_PORTAL_CLIPPING, qfalse, //backEnd.viewParms.isPortal,
								USE_VERTEX_SKINNING, qfalse,
								USE_VERTEX_ANIMATION, qfalse,
								USE_DEFORM_VERTEXES, qfalse,
								USE_TCGEN_ENVIRONMENT, qfalse,
								USE_TCGEN_LIGHTMAP, qfalse);

	GLSL_SetRequiredVertexPointers(trProg.gl_genericShader);

	GL_Cull(CT_TWO_SIDED);

	SetUniformMatrix16(UNIFORM_MODELMATRIX, backEnd.orientation.transformMatrix);
	SetUniformMatrix16(UNIFORM_MODELVIEWPROJECTIONMATRIX, GLSTACK_MVPM);

	// set uniforms
	GLSL_SetUniform_ColorModulate(trProg.gl_genericShader, CGEN_CONST, AGEN_CONST);
	SetUniformVec4(UNIFORM_COLOR, colorBlue);

	// bind u_ColorMap
	SelectTexture(TEX_COLOR);
	GL_Bind(tr.whiteImage);
	SetUniformMatrix16(UNIFORM_COLORTEXTUREMATRIX, matrixIdentity);
/*
	if (backEnd.viewParms.isPortal)
	{
		clipPortalPlane();
	}
*/
	if (backEnd.viewParms.isPortal)
	{
		glDisable(GL_CLIP_PLANE0);
	}

	if (r_showOcclusionQueries->integer)
	{
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
	else
	{
		// don't write to the color buffer or depth buffer
		GL_State(GLS_COLORMASK_BITS);
	}

	// render all flare query surfaces
	active = &r_activeFlares;
	while ((f = *active) != NULL)
	{
		// 1st: skip this flare if it currently is being queried already.
		if (f->querying)
		{
			active = &f->next;
			continue;
		}

		// 2nd: throw out any flares that weren't added this frame,
		// but only if the flare isn't currently being queried. (this we did check before).
		if (f->addedFrame != backEnd.viewParms.frameCount)
		{
			*active          = f->next;
			f->next          = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// 3rd: only draw flares that are from this scene/portal
		if (f->frameSceneNum != backEnd.viewParms.frameSceneNum || f->inPortal != backEnd.viewParms.isPortal)
		{
			active = &f->next;
			continue;
		}

		// drawing the query can be rejected, if the object is culled out completely (before drawing).
		f->querying = IssueFlareOcclusionQuery(f);

		active = &f->next;
	}

	// The render commands to draw the query objects are now done. 
	// Checking for errors ensures the render pipeline is flushed.
	GL_CheckErrors();

	// remove flares that are rendered invisible; The occlusion query result was: 0 samples rendered
	active = &r_activeFlares;
	while ((f = *active) != NULL)
	{
		// skip this flare if there is no query executing.
		// only draw flares that are from this scene/portal.
		if (!f->querying || f->frameSceneNum != backEnd.viewParms.frameSceneNum || f->inPortal != backEnd.viewParms.isPortal)
		{
			active = &f->next;
			continue;
		}

		// If commented out FlareOcclusionResultAvailable calls, any calls to GetFlareOcclusionQueryResult will make the queries run synchronously
		// (meaning, GetFlareOcclusionQueryResult will wait for the result to be available, before returning the result)
		if (FlareOcclusionResultAvailable(f))
		{
			GetFlareOcclusionQueryResult(f);

 			f->querying = qfalse;

			// if nothing of the query surface is rendered, remove the flare from the list
			if (!f->occlusionQuerySamples)
			{
				*active          = f->next;
				f->next          = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		// set the intensity of the flare: initially by the distance flare to camera
		f->drawIntensity = (-f->eyeZ >= CORONAFARDISTANCE)? 0.f : 1.f - (-f->eyeZ / CORONAFARDISTANCE);  //cg_coronafardist = 1536
		// and by the amount of samples rendered
//		f->drawIntensity *= (f->flareSamples == 0)? 0.f : (f->occlusionQuerySamples >= f->flareSamples) ? 1.f : (float)f->occlusionQuerySamples / (float)f->flareSamples;

		active = &f->next;
	}

	// go back to the world modelview matrix
	backEnd.orientation = backEnd.viewParms.world;
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

	// reenable writes to depth and color buffers
	GL_State(GLS_DEPTHMASK_TRUE);

	GL_PopMatrix();
}

///^ end flare occlusion query objects



/**
 * @brief RB_TestFlare
 * @param[in,out] f
 */ /* // this function is now unused..
void RB_TestFlare(flare_t *f)
{
	float    depth;
	qboolean visible;
	float    fade;
	float    screenZ;

	backEnd.pc.c_flareTests++;

	//visible = f->cgvisible;

	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse; //? true?
//glFinish();
//glState.finishCalled = qtrue;

	// read back the z buffer contents
	glReadPixels(f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	screenZ = backEnd.viewParms.projectionMatrix[14] /
	          ((2.f * depth - 1.f) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10]);

	visible = (-f->eyeZ - -screenZ) < 24.f;

	if (visible)
	{
		if (!f->visible)
		{
			f->visible  = qtrue;
			f->fadeTime = backEnd.refdef.time - 1500;
		}
		fade = ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;
	}
	else
	{
		if (f->visible)
		{
			f->visible  = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ((backEnd.refdef.time - f->fadeTime) / 1000.0f) * r_flareFade->value;
	}

	if (fade < 0)
	{
		fade = 0;
	}
	if (fade > 1)
	{
		fade = 1;
	}

	f->drawIntensity = fade;
}
*/

/**
 * @brief RB_RenderFlare
 * @param[in] f
 *
 *   This renders the actual flare image on screen
 */
void RB_RenderFlare(flare_t *f)
{
	float /*@ size, distance,*/ intensity, factor;
	vec3_t color;

	backEnd.pc.c_flareRenders++;

	if (f->isCorona) // corona case
	{
//@		size = (float)backEnd.viewParms.viewportWidth * ((r_flareSize->value / 640.0f) + (8.f / -f->eyeZ));
		VectorScale(f->color, f->drawIntensity, color);
	}
	else
	{
		/*
		   As flare sizes stay nearly constant with increasing distance we must decrease the intensity
		   to achieve a reasonable visual result. The intensity is ~ (size^2 / distance^2) which can be
		   got by considering the ratio of  (flaresurface on screen) : (Surface of sphere defined by flare origin and distance from flare)
		   An important requirement is: intensity <= 1 for all distances.

		   The formula used here to compute the intensity is as follows:
		   intensity = flareCoeff * size^2 / (distance + size*sqrt(flareCoeff))^2.
		   As you can see, the intensity will have a max. of 1 when the distance is 0.

		   The coefficient flareCoeff will determine the falloff speed with increasing distance.
		 */
/*@
		// We don't want too big values anyways when dividing by distance
		if (f->eyeZ > -1.0f)
		{
			distance = 1.0f;
		}
		else
		{
			distance = -f->eyeZ;
		}

		// calculate the flare size
		size = (float)backEnd.viewParms.viewportWidth * ((r_flareSize->value / 640.0f) + (8.f / distance));

		factor    = distance + size * sqrt(flareCoeff);
		intensity = flareCoeff * size * size / (factor * factor);
		VectorScale(f->color, f->drawIntensity * intensity, color);
*/
		factor    = flareSize * sqrt(flareCoeff) - f->eyeZ;
		intensity = flareCoeff * flareSize * flareSize / (factor * factor);
		VectorScale(f->color, f->drawIntensity * intensity, color);
	}

	Tess_Begin(Tess_StageIteratorGeneric, NULL, tr.flareShader, NULL, qtrue, qfalse, LIGHTMAP_NONE, f->fogNum);

	// FIXME: use quadstamp?
	Vector4Set(tess.xyz[tess.numVertexes], f->windowX - flareSize, f->windowY - flareSize, 0.f, 1.f);
	Vector4Set(tess.texCoords[tess.numVertexes], 0.f, 0.f, 0.f, 1.f);
	Vector4Set(tess.colors[tess.numVertexes], color[0], color[1], color[2], 1.f);
	tess.numVertexes++;

	Vector4Set(tess.xyz[tess.numVertexes], f->windowX - flareSize, f->windowY + flareSize, 0.f, 1.f);
	Vector4Set(tess.texCoords[tess.numVertexes], 0.f, 1.f, 0.f, 1.f);
	Vector4Set(tess.colors[tess.numVertexes], color[0], color[1], color[2], 1.f);
	tess.numVertexes++;

	Vector4Set(tess.xyz[tess.numVertexes], (float)f->windowX + flareSize, (float)f->windowY + flareSize, 0.f, 1.f);
	Vector4Set(tess.texCoords[tess.numVertexes], 1.f, 1.f, 0.f, 1.f);
	Vector4Set(tess.colors[tess.numVertexes], color[0], color[1], color[2], 1.f);
	tess.numVertexes++;

	Vector4Set(tess.xyz[tess.numVertexes], (float)f->windowX + flareSize, (float)f->windowY - flareSize, 0.f, 1.f);
	Vector4Set(tess.texCoords[tess.numVertexes], 1.f, 0.f, 0.f, 1.f);
	Vector4Set(tess.colors[tess.numVertexes], color[0], color[1], color[2], 1.f);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	Tess_End();
}

/**
 * @brief Because flares are simulating an occular effect, they should be drawn after
 * everything (all views) in the entire frame has been drawn.
 *
 * Because of the way portals use the depth buffer to mark off areas, the
 * needed information would be lost after each view, so we are forced to draw
 * flares after each view.
 *
 * The resulting artifact is that flares in mirrors or portals don't dim properly
 * when occluded by something in the main view, and portal flares that should
 * extend past the portal edge will be overwritten.
 */
void RB_RenderFlares(void)
{
	flare_t *f;
	flare_t **active;
	qboolean draw;

	// the flare size remains constant.
	flareSize = (float)backEnd.viewParms.viewportWidth * (r_flareSize->value / 640.0f);

	if (!r_flares->integer)
	{
		return;
	}

	if (r_flareCoeff->modified)
	{
		flareCoeff = (r_flareCoeff->value == 0.0f)? atof("150") : r_flareCoeff->value;
		r_flareCoeff->modified = qfalse;
	}

	// reset currentEntity to world so that any previously referenced entities don't have influence
	// on the rendering of these flares (i.e. RF_ renderer flags).
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.orientation   = backEnd.viewParms.world;
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

	if (tr.world != NULL)
	{
		RB_AddLightFlares();
		RB_AddCoronaFlares();
	}

	if (r_occludeFlares->integer && glConfig2.occlusionQueryBits && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		// use occlusion queries to find out how much of a flare is drawn (and how much is occluded)
		RB_RenderFlareOcclusionQueries();

	} else {
		draw = qfalse;
		active = &r_activeFlares;
		while ((f = *active) != NULL)
		{
			// throw out any flares that weren't added last frame
			if (f->addedFrame < backEnd.viewParms.frameCount - 1)
			{
				*active          = f->next;
				f->next          = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}

			// don't draw any here that aren't from this scene / portal
			f->drawIntensity = 0;
			if (f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal)
			{
//				RB_TestFlare(f); // perform z buffer readback on each flare in this view
				f->drawIntensity = (-f->eyeZ >= CORONAFARDISTANCE)? 0.f : 1.f - (-f->eyeZ / CORONAFARDISTANCE);  //cg_coronafardist = 1536

				if (f->drawIntensity != 0.f)
				{
					draw = qtrue;
				}
				else
				{
					// this flare has completely faded out, so remove it from the chain
					*active          = f->next;
					f->next          = r_inactiveFlares;
					r_inactiveFlares = f;
					continue;
				}
			}

			active = &f->next;
		}

		if (!draw)
		{
			// none visible
			return;
		}
	}

	if (backEnd.viewParms.isPortal)
	{
		glDisable(GL_CLIP_PLANE0);
	}

	GL_CheckErrors();

	// this translucent stuff is rendering slow..
	GL_PushMatrix();
	RB_SetViewMVPM(); // ortho projection

	for (f = r_activeFlares; f; f = f->next)
	{
		if (f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal && f->drawIntensity != 0.f)
		{
			RB_RenderFlare(f);
		}
	}

	GL_PopMatrix();

	GL_CheckErrors();
}
