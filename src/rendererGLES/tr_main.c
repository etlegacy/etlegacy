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
 * @file rendererGLES/tr_main.c
 * @brief Main control flow for each frame
 */

#include "tr_local.h"

trGlobals_t tr;

static float s_flipMatrix[16] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0,  0, -1, 0,
	-1, 0, 0,  0,
	0,  1, 0,  0,
	0,  0, 0,  1
};

refimport_t ri;

/**
 * @var entitySurface
 * @brief Entities that will have procedurally generated surfaces will just
 * point at this for their sorting surface
 */
surfaceType_t entitySurface = SF_ENTITY;

// fog stuff
glfog_t     glfogsettings[NUM_FOGS];
glfogType_t glfogNum = FOG_NONE;
qboolean    fogIsOn  = qfalse;

/**
 * @brief R_Fog
 * @param[in] curfog
 */
void R_Fog(glfog_t *curfog)
{
	if (!r_wolfFog->integer)
	{
		R_FogOff();
		return;
	}

	if (!curfog->registered)
	{
		R_FogOff();
		return;
	}

	// assme values of '0' for these parameters means 'use default'
	if (curfog->density == 0.f)
	{
		curfog->density = 1;
	}
	if (!curfog->hint)
	{
		curfog->hint = GL_DONT_CARE;
	}
	if (!curfog->mode)
	{
		curfog->mode = GL_LINEAR;
	}

	R_FogOn();

	qglFogi(GL_FOG_MODE, curfog->mode);

	qglFogfv(GL_FOG_COLOR, curfog->color);

	qglFogf(GL_FOG_DENSITY, curfog->density);

	qglHint(GL_FOG_HINT, curfog->hint);

	qglFogf(GL_FOG_START, curfog->start);

	if (r_zFar->value != 0.f)                 // allow override for helping level designers test fog distances
	{
		qglFogf(GL_FOG_END, r_zFar->value);
	}
	else
	{
		qglFogf(GL_FOG_END, curfog->end);
	}

	qglClearColor(curfog->color[0], curfog->color[1], curfog->color[2], curfog->color[3]);
}

/**
 * @brief Allow disabling fog temporarily
 */
void R_FogOff(void)
{
	if (!fogIsOn)
	{
		return;
	}
	qglDisable(GL_FOG);
	fogIsOn = qfalse;
}

/**
 * @brief R_FogOn
 */
void R_FogOn(void)
{
	if (fogIsOn)
	{
		return;
	}

	if (!r_wolfFog->integer)
	{
		return;
	}

	if (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)     // don't force world fog on portal sky
	{
		if (!(glfogsettings[FOG_PORTALVIEW].registered))
		{
			return;
		}
	}
	else if (!glfogNum)
	{
		return;
	}

	qglEnable(GL_FOG);
	fogIsOn = qtrue;
}

/**
 * @brief R_SetFog
 *
 * @param[in] fogvar
 * @param[in] var1
 * @param[in] var2
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] density
 *
 * @note
 * if fogvar == FOG_CMD_SWITCHFOG {
 *   fogvar is the command
 *   var1 is the fog to switch to
 *   var2 is the time to transition
 * }
 * else {
 *   fogvar is the fog that's being set
 *   var1 is the near fog z value
 *   var2 is the far fog z value
 *   rgb = color
 *   density is density, and is used to derive the values of 'mode', 'drawsky', and 'clearscreen'
 * }
 */
void R_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density)
{
	if (fogvar != FOG_CMD_SWITCHFOG)       // just set the parameters and return
	{
		if (var1 == 0 && var2 == 0)        // clear this fog
		{
			glfogsettings[fogvar].registered = qfalse;
			return;
		}

		glfogsettings[fogvar].color[0] = r * tr.identityLight;
		glfogsettings[fogvar].color[1] = g * tr.identityLight;
		glfogsettings[fogvar].color[2] = b * tr.identityLight;
		glfogsettings[fogvar].color[3] = 1;
		glfogsettings[fogvar].start    = var1;
		glfogsettings[fogvar].end      = var2;
		if (density >= 1)
		{
			glfogsettings[fogvar].mode        = GL_LINEAR;
			glfogsettings[fogvar].drawsky     = qfalse;
			glfogsettings[fogvar].clearscreen = qtrue;
			glfogsettings[fogvar].density     = 1.0;
		}
		else
		{
			glfogsettings[fogvar].mode        = GL_EXP;
			glfogsettings[fogvar].drawsky     = qtrue;
			glfogsettings[fogvar].clearscreen = qfalse;
			glfogsettings[fogvar].density     = density;
		}
		glfogsettings[fogvar].hint       = GL_DONT_CARE;
		glfogsettings[fogvar].registered = qtrue;

		return;
	}

	// don't switch to invalid fogs
	if (glfogsettings[var1].registered != qtrue)
	{
		return;
	}

	glfogNum = var1;

	// transitioning to new fog, store the current values as the 'from'

	if (glfogsettings[FOG_CURRENT].registered)
	{
		Com_Memcpy(&glfogsettings[FOG_LAST], &glfogsettings[FOG_CURRENT], sizeof(glfog_t));
	}
	else
	{
		// if no current fog fall back to world fog
		// FIXME: handle transition if there is no FOG_MAP fog
		Com_Memcpy(&glfogsettings[FOG_LAST], &glfogsettings[FOG_MAP], sizeof(glfog_t));
	}

	Com_Memcpy(&glfogsettings[FOG_TARGET], &glfogsettings[glfogNum], sizeof(glfog_t));

	// setup transition times
	glfogsettings[FOG_TARGET].startTime  = tr.refdef.time;
	glfogsettings[FOG_TARGET].finishTime = tr.refdef.time + var2;
}

/**
 * @brief R_CullLocalBox
 * @param[in] bounds
 * @return CULL_IN, CULL_CLIP, or CULL_OUT
 */
int R_CullLocalBox(vec3_t bounds[2])
{
	int      i, j;
	vec3_t   transformed[8];
	float    dists[8];
	vec3_t   v;
	cplane_t *frust;
	int      anyBack;
	int      front, back;

	if (r_noCull->integer)
	{
		return CULL_CLIP;
	}

	// transform into world space
	for (i = 0 ; i < 8 ; i++)
	{
		v[0] = bounds[i & 1][0];
		v[1] = bounds[(i >> 1) & 1][1];
		v[2] = bounds[(i >> 2) & 1][2];

		VectorCopy(tr.orientation.origin, transformed[i]);
		VectorMA(transformed[i], v[0], tr.orientation.axis[0], transformed[i]);
		VectorMA(transformed[i], v[1], tr.orientation.axis[1], transformed[i]);
		VectorMA(transformed[i], v[2], tr.orientation.axis[2], transformed[i]);
	}

	// check against frustum planes
	anyBack = 0;
	for (i = 0 ; i < 5 ; i++)
	{
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for (j = 0 ; j < 8 ; j++)
		{
			dists[j] = DotProduct(transformed[j], frust->normal);
			if (dists[j] > frust->dist)
			{
				front = 1;
				if (back)
				{
					break;      // a point is in front
				}
			}
			else
			{
				back = 1;
			}
		}
		if (!front)
		{
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if (!anyBack)
	{
		return CULL_IN;     // completely inside frustum
	}

	return CULL_CLIP;       // partially clipped
}

/**
 * @brief R_CullLocalPointAndRadius
 * @param[in] origin
 * @param[in] radius
 * @return
 */
int R_CullLocalPointAndRadius(vec3_t origin, float radius)
{
	vec3_t transformed;

	R_LocalPointToWorld(origin, transformed);

	return R_CullPointAndRadius(transformed, radius);
}

/**
 * @brief R_CullPointAndRadius
 * @param[in] origin
 * @param[in] radius
 * @return
 */
int R_CullPointAndRadius(vec3_t origin, float radius)
{
	int      i;
	float    dist;
	cplane_t *frust;
	qboolean mightBeClipped = qfalse;

	if (r_noCull->integer)
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	for (i = 0 ; i < 5 ; i++)
	{
		frust = &tr.viewParms.frustum[i];

		dist = DotProduct(origin, frust->normal) - frust->dist;
		if (dist < -radius)
		{
			return CULL_OUT;
		}
		else if (dist <= radius)
		{
			mightBeClipped = qtrue;
		}
	}

	if (mightBeClipped)
	{
		return CULL_CLIP;
	}

	return CULL_IN;     // completely inside frustum
}

/**
 * @brief R_LocalNormalToWorld
 * @param[in] local
 * @param[out] world
 */
void R_LocalNormalToWorld(vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.orientation.axis[0][0] + local[1] * tr.orientation.axis[1][0] + local[2] * tr.orientation.axis[2][0];
	world[1] = local[0] * tr.orientation.axis[0][1] + local[1] * tr.orientation.axis[1][1] + local[2] * tr.orientation.axis[2][1];
	world[2] = local[0] * tr.orientation.axis[0][2] + local[1] * tr.orientation.axis[1][2] + local[2] * tr.orientation.axis[2][2];
}

/**
 * @brief R_LocalPointToWorld
 * @param[in] local
 * @param[out] world
 */
void R_LocalPointToWorld(vec3_t local, vec3_t world)
{
	world[0] = local[0] * tr.orientation.axis[0][0] + local[1] * tr.orientation.axis[1][0] + local[2] * tr.orientation.axis[2][0] + tr.orientation.origin[0];
	world[1] = local[0] * tr.orientation.axis[0][1] + local[1] * tr.orientation.axis[1][1] + local[2] * tr.orientation.axis[2][1] + tr.orientation.origin[1];
	world[2] = local[0] * tr.orientation.axis[0][2] + local[1] * tr.orientation.axis[1][2] + local[2] * tr.orientation.axis[2][2] + tr.orientation.origin[2];
}

/**
 * @brief R_WorldToLocal
 * @param[in] world
 * @param[out] local
 *
 * @note Unused
 */
void R_WorldToLocal(vec3_t world, vec3_t local)
{
	local[0] = DotProduct(world, tr.orientation.axis[0]);
	local[1] = DotProduct(world, tr.orientation.axis[1]);
	local[2] = DotProduct(world, tr.orientation.axis[2]);
}

/**
 * @brief R_TransformModelToClip
 * @param[in] src
 * @param[in] modelMatrix
 * @param[in] projectionMatrix
 * @param[out] eye
 * @param[out] dst
 */
void R_TransformModelToClip(const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
                            vec4_t eye, vec4_t dst)
{
	int i;

	for (i = 0 ; i < 4 ; i++)
	{
		eye[i] =
		    src[0] * modelMatrix[i + 0 * 4] +
		    src[1] * modelMatrix[i + 1 * 4] +
		    src[2] * modelMatrix[i + 2 * 4] +
		    1 * modelMatrix[i + 3 * 4];
	}

	for (i = 0 ; i < 4 ; i++)
	{
		dst[i] =
		    eye[0] * projectionMatrix[i + 0 * 4] +
		    eye[1] * projectionMatrix[i + 1 * 4] +
		    eye[2] * projectionMatrix[i + 2 * 4] +
		    eye[3] * projectionMatrix[i + 3 * 4];
	}
}

/**
 * @brief R_TransformClipToWindow
 * @param[in] clip
 * @param[in] view
 * @param[out] normalized
 * @param[out] window
 */
void R_TransformClipToWindow(const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window)
{
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = (clip[2] + clip[3]) / (2 * clip[3]);

	window[0] = 0.5f * (1.0f + normalized[0]) * view->viewportWidth;
	window[1] = 0.5f * (1.0f + normalized[1]) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) (window[0] + 0.5f);
	window[1] = (int) (window[1] + 0.5f);
}

/**
 * @brief myGlMultMatrix
 * @param[in] a
 * @param[in] b
 * @param[out] out
 */
void myGlMultMatrix(const float *a, const float *b, float *out)
{
	int i, j;

	for (i = 0 ; i < 4 ; i++)
	{
		for (j = 0 ; j < 4 ; j++)
		{
			out[i * 4 + j] =
			    a[i * 4 + 0] * b[0 * 4 + j]
			    + a[i * 4 + 1] * b[1 * 4 + j]
			    + a[i * 4 + 2] * b[2 * 4 + j]
			    + a[i * 4 + 3] * b[3 * 4 + j];
		}
	}
}

/**
 * @brief Generates an orientation for an entity and viewParms
 * Does NOT produce any GL calls
 * Called by both the front end and the back end
 * @param[in] ent
 * @param[in] viewParms
 * @param[out] orientation
 */
void R_RotateForEntity(const trRefEntity_t *ent, const viewParms_t *viewParms,
                       orientationr_t *orientation)
{
	float  glMatrix[16];
	vec3_t delta;
	float  axisLength;

	if (ent->e.reType != RT_MODEL)
	{
		*orientation = viewParms->world;
		return;
	}

	VectorCopy(ent->e.origin, orientation->origin);

	VectorCopy(ent->e.axis[0], orientation->axis[0]);
	VectorCopy(ent->e.axis[1], orientation->axis[1]);
	VectorCopy(ent->e.axis[2], orientation->axis[2]);

	glMatrix[0]  = orientation->axis[0][0];
	glMatrix[4]  = orientation->axis[1][0];
	glMatrix[8]  = orientation->axis[2][0];
	glMatrix[12] = orientation->origin[0];

	glMatrix[1]  = orientation->axis[0][1];
	glMatrix[5]  = orientation->axis[1][1];
	glMatrix[9]  = orientation->axis[2][1];
	glMatrix[13] = orientation->origin[1];

	glMatrix[2]  = orientation->axis[0][2];
	glMatrix[6]  = orientation->axis[1][2];
	glMatrix[10] = orientation->axis[2][2];
	glMatrix[14] = orientation->origin[2];

	glMatrix[3]  = 0;
	glMatrix[7]  = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix(glMatrix, viewParms->world.modelMatrix, orientation->modelMatrix);

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract(viewParms->orientation.origin, orientation->origin, delta);

	// compensate for scale in the axes if necessary
	if (ent->e.nonNormalizedAxes)
	{
		axisLength = VectorLength(ent->e.axis[0]);
		if (axisLength == 0.f)
		{
			axisLength = 0.f;
		}
		else
		{
			axisLength = 1.0f / axisLength;
		}
	}
	else
	{
		axisLength = 1.0f;
	}

	orientation->viewOrigin[0] = DotProduct(delta, orientation->axis[0]) * axisLength;
	orientation->viewOrigin[1] = DotProduct(delta, orientation->axis[1]) * axisLength;
	orientation->viewOrigin[2] = DotProduct(delta, orientation->axis[2]) * axisLength;
}

/**
 * @brief Sets up the modelview matrix for a given viewParm
 */
void R_RotateForViewer(void)
{
	float  viewerMatrix[16];
	vec3_t origin;

	Com_Memset(&tr.orientation, 0, sizeof(tr.orientation));
	tr.orientation.axis[0][0] = 1;
	tr.orientation.axis[1][1] = 1;
	tr.orientation.axis[2][2] = 1;
	VectorCopy(tr.viewParms.orientation.origin, tr.orientation.viewOrigin);

	// transform by the camera placement
	VectorCopy(tr.viewParms.orientation.origin, origin);

	viewerMatrix[0]  = tr.viewParms.orientation.axis[0][0];
	viewerMatrix[4]  = tr.viewParms.orientation.axis[0][1];
	viewerMatrix[8]  = tr.viewParms.orientation.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1]  = tr.viewParms.orientation.axis[1][0];
	viewerMatrix[5]  = tr.viewParms.orientation.axis[1][1];
	viewerMatrix[9]  = tr.viewParms.orientation.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2]  = tr.viewParms.orientation.axis[2][0];
	viewerMatrix[6]  = tr.viewParms.orientation.axis[2][1];
	viewerMatrix[10] = tr.viewParms.orientation.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3]  = 0;
	viewerMatrix[7]  = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix(viewerMatrix, s_flipMatrix, tr.orientation.modelMatrix);

	tr.viewParms.world = tr.orientation;
}

/**
 * @brief R_SetFrameFog
 */
void R_SetFrameFog(void)
{
	// new style global fog transitions
	if (tr.world->globalFogTransEndTime)
	{
		if (tr.world->globalFogTransEndTime >= tr.refdef.time)
		{
			int   fadeTime = tr.world->globalFogTransEndTime - tr.world->globalFogTransStartTime;
			float lerpPos  = (float)(tr.refdef.time - tr.world->globalFogTransStartTime) / (float)fadeTime;

			if (lerpPos > 1)
			{
				lerpPos = 1;
			}

			tr.world->fogs[tr.world->globalFog].shader->fogParms.color[0] = tr.world->globalTransStartFog[0] + ((tr.world->globalTransEndFog[0] - tr.world->globalTransStartFog[0]) * lerpPos);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.color[1] = tr.world->globalTransStartFog[1] + ((tr.world->globalTransEndFog[1] - tr.world->globalTransStartFog[1]) * lerpPos);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.color[2] = tr.world->globalTransStartFog[2] + ((tr.world->globalTransEndFog[2] - tr.world->globalTransStartFog[2]) * lerpPos);

			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4(tr.world->fogs[tr.world->globalFog].shader->fogParms.color[0] * tr.identityLight,
			                                                                            tr.world->fogs[tr.world->globalFog].shader->fogParms.color[1] * tr.identityLight,
			                                                                            tr.world->fogs[tr.world->globalFog].shader->fogParms.color[2] * tr.identityLight, 1.0);

			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = tr.world->globalTransStartFog[3] + ((tr.world->globalTransEndFog[3] - tr.world->globalTransStartFog[3]) * lerpPos);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale        = 1.0f / (tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque * 8);
		}
		else
		{
			// transition complete
			VectorCopy(tr.world->globalTransEndFog, tr.world->fogs[tr.world->globalFog].shader->fogParms.color);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4(tr.world->globalTransEndFog[0] * tr.identityLight,
			                                                                            tr.world->globalTransEndFog[1] * tr.identityLight,
			                                                                            tr.world->globalTransEndFog[2] * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = tr.world->globalTransEndFog[3];
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale        = 1.0f / (tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque * 8);

			tr.world->globalFogTransEndTime = 0;
		}
	}

	if (r_speeds->integer == 5)
	{
		if (!glfogsettings[FOG_TARGET].registered)
		{
			Ren_Print("no fog - calc zFar: %0.1f\n", tr.viewParms.zFar);
			return;
		}
	}

	// If fog is not valid, don't use it
	if (!glfogsettings[FOG_TARGET].registered)
	{
		return;
	}

	// still fading
	if (glfogsettings[FOG_TARGET].finishTime && glfogsettings[FOG_TARGET].finishTime >= tr.refdef.time)
	{
		// transitioning from density to distance
		if (glfogsettings[FOG_LAST].mode == GL_EXP && glfogsettings[FOG_TARGET].mode == GL_LINEAR)
		{
			// for now just fast transition to the target when dissimilar fogs are
			Com_Memcpy(&glfogsettings[FOG_CURRENT], &glfogsettings[FOG_TARGET], sizeof(glfog_t));
			glfogsettings[FOG_TARGET].finishTime = 0;
		}
		// transitioning from distance to density
		else if (glfogsettings[FOG_LAST].mode == GL_LINEAR && glfogsettings[FOG_TARGET].mode == GL_EXP)
		{
			Com_Memcpy(&glfogsettings[FOG_CURRENT], &glfogsettings[FOG_TARGET], sizeof(glfog_t));
			glfogsettings[FOG_TARGET].finishTime = 0;
		}
		// transitioning like fog modes
		else
		{
			float lerpPos;
			int   fadeTime = glfogsettings[FOG_TARGET].finishTime - glfogsettings[FOG_TARGET].startTime;

			if (fadeTime <= 0)
			{
				fadeTime = 1;   // avoid divide by zero
			}
			lerpPos = (float)(tr.refdef.time - glfogsettings[FOG_TARGET].startTime) / (float)fadeTime;
			if (lerpPos > 1)
			{
				lerpPos = 1;
			}

			// lerp near/far
			glfogsettings[FOG_CURRENT].start = glfogsettings[FOG_LAST].start + ((glfogsettings[FOG_TARGET].start - glfogsettings[FOG_LAST].start) * lerpPos);
			glfogsettings[FOG_CURRENT].end   = glfogsettings[FOG_LAST].end + ((glfogsettings[FOG_TARGET].end - glfogsettings[FOG_LAST].end) * lerpPos);

			// lerp color
			glfogsettings[FOG_CURRENT].color[0] = glfogsettings[FOG_LAST].color[0] + ((glfogsettings[FOG_TARGET].color[0] - glfogsettings[FOG_LAST].color[0]) * lerpPos);
			glfogsettings[FOG_CURRENT].color[1] = glfogsettings[FOG_LAST].color[1] + ((glfogsettings[FOG_TARGET].color[1] - glfogsettings[FOG_LAST].color[1]) * lerpPos);
			glfogsettings[FOG_CURRENT].color[2] = glfogsettings[FOG_LAST].color[2] + ((glfogsettings[FOG_TARGET].color[2] - glfogsettings[FOG_LAST].color[2]) * lerpPos);

			glfogsettings[FOG_CURRENT].density    = glfogsettings[FOG_TARGET].density;
			glfogsettings[FOG_CURRENT].mode       = glfogsettings[FOG_TARGET].mode;
			glfogsettings[FOG_CURRENT].registered = qtrue;

			// if either fog in the transition clears the screen, clear the background this frame to avoid hall of mirrors
			glfogsettings[FOG_CURRENT].clearscreen = (glfogsettings[FOG_TARGET].clearscreen || glfogsettings[FOG_LAST].clearscreen);
		}
	}
	else
	{
		// probably usually not necessary to copy the whole thing.
		// potential FIXME: since this is the most common occurance, diff first and only set changes
		Com_Memcpy(&glfogsettings[FOG_CURRENT], &glfogsettings[FOG_TARGET], sizeof(glfog_t));
	}

	// shorten the far clip if the fog opaque distance is closer than the procedural farcip dist

	if (glfogsettings[FOG_CURRENT].mode == GL_LINEAR)
	{
		if (glfogsettings[FOG_CURRENT].end < tr.viewParms.zFar)
		{
			tr.viewParms.zFar = glfogsettings[FOG_CURRENT].end;
		}
	}

	if (r_speeds->integer == 5)
	{
		if (glfogsettings[FOG_CURRENT].mode == GL_LINEAR)
		{
			Ren_Print("farclip fog - den: %0.1f  calc zFar: %0.1f  fog zfar: %0.1f\n", glfogsettings[FOG_CURRENT].density, tr.viewParms.zFar, glfogsettings[FOG_CURRENT].end);
		}
		else
		{
			Ren_Print("density fog - den: %0.4f  calc zFar: %0.1f  fog zFar: %0.1f\n", glfogsettings[FOG_CURRENT].density, tr.viewParms.zFar, glfogsettings[FOG_CURRENT].end);
		}
	}
}

/**
 * @brief SetFarClip
 */
static void SetFarClip(void)
{
	float farthestCornerDistance;
	int   i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		tr.viewParms.zFar = 2048;
		return;
	}

	// this lets you use r_zfar from the command line to experiment with different
	// distances, but setting it back to 0 uses the map (or procedurally generated) default
	if (r_zFar->value != 0.f)
	{
		tr.viewParms.zFar = r_zFar->integer;
		R_SetFrameFog();

		if (r_speeds->integer == 5)
		{
			Ren_Print("r_zfar value forcing farclip at: %f\n", tr.viewParms.zFar);
		}

		return;
	}

	// set far clipping planes dynamically
	farthestCornerDistance = 0;
	for (i = 0; i < 8; i++)
	{
		vec3_t v;
		vec3_t vecTo;
		float  distance;

		if (i & 1)
		{
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if (i & 2)
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if (i & 4)
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		VectorSubtract(v, tr.viewParms.orientation.origin, vecTo);

		distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if (distance > farthestCornerDistance)
		{
			farthestCornerDistance = distance;
		}
	}

	tr.viewParms.zFar = sqrt(farthestCornerDistance);

	// add global q3 fog
	if (tr.world != NULL && tr.world->globalFog >= 0 && tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque < tr.viewParms.zFar)
	{
		tr.viewParms.zFar = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;
	}

	R_SetFrameFog();
}

/**
 * @brief Setup that culling frustum planes for the current view
 */
void R_SetupFrustum(void)
{
	int   i;
	float ang = (float)(tr.viewParms.fovX / 180 * M_PI * 0.5);
	float xs  = (float)(sin(ang));
	float xc  = (float)(cos(ang));

	VectorScale(tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[0].normal);
	VectorMA(tr.viewParms.frustum[0].normal, xc, tr.viewParms.orientation.axis[1], tr.viewParms.frustum[0].normal);

	VectorScale(tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[1].normal);
	VectorMA(tr.viewParms.frustum[1].normal, -xc, tr.viewParms.orientation.axis[1], tr.viewParms.frustum[1].normal);

	ang = (float)(tr.viewParms.fovY / 180 * M_PI * 0.5);
	xs  = (float)(sin(ang));
	xc  = (float)(cos(ang));

	VectorScale(tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[2].normal);
	VectorMA(tr.viewParms.frustum[2].normal, xc, tr.viewParms.orientation.axis[2], tr.viewParms.frustum[2].normal);

	VectorScale(tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[3].normal);
	VectorMA(tr.viewParms.frustum[3].normal, -xc, tr.viewParms.orientation.axis[2], tr.viewParms.frustum[3].normal);

	for (i = 0; i < 4; i++)
	{
		tr.viewParms.frustum[i].type = PLANE_NON_AXIAL;
		tr.viewParms.frustum[i].dist = DotProduct(tr.viewParms.orientation.origin, tr.viewParms.frustum[i].normal);
		SetPlaneSignbits(&tr.viewParms.frustum[i]);
	}

	// farplane (testing! use farplane for real)
	VectorScale(tr.viewParms.orientation.axis[0], -1, tr.viewParms.frustum[4].normal);
	tr.viewParms.frustum[4].dist = DotProduct(tr.viewParms.orientation.origin, tr.viewParms.frustum[4].normal) - tr.viewParms.zFar;
	tr.viewParms.frustum[4].type = PLANE_NON_AXIAL;
	SetPlaneSignbits(&tr.viewParms.frustum[4]);
}

/**
 * @brief R_SetupProjection
 */
void R_SetupProjection(void)
{
	float xmin, xmax, ymin, ymax;
	float width, height, depth;
	float zNear, zFar;

	// dynamically compute far clip plane distance
	SetFarClip();

	// set frustum planes (this happens here because of zfar manipulation)
	R_SetupFrustum();

	// set up projection matrix
	zNear = r_zNear->value;

	// high fov values let players see through walls
	// solution is to move z near plane inward, which decreases zbuffer precision
	// but if a player wants to play with fov 160, then they can deal with z-fighting
	// assume fov 90 = scale 1, fov 180 = scale 1/16th
	if (tr.refdef.fov_x > 90.0f)
	{
		zNear /= ((tr.refdef.fov_x - 90.0f) * 0.09f + 1.0f);
	}

	if (r_zFar->value != 0.f)
	{
		zFar = r_zFar->value;   // allow override for helping level designers test fog distances
	}
	else
	{
		zFar = tr.viewParms.zFar;
	}

	ymax = (float)(zNear * tan(tr.refdef.fov_y * M_PI / 360.0));
	ymin = -ymax;

	xmax = (float)(zNear * tan(tr.refdef.fov_x * M_PI / 360.0));
	xmin = -xmax;

	width  = xmax - xmin;
	height = ymax - ymin;
	depth  = zFar - zNear;

	tr.viewParms.projectionMatrix[0]  = 2 * zNear / width;
	tr.viewParms.projectionMatrix[4]  = 0;
	tr.viewParms.projectionMatrix[8]  = (xmax + xmin) / width;  // normally 0
	tr.viewParms.projectionMatrix[12] = 0;

	tr.viewParms.projectionMatrix[1]  = 0;
	tr.viewParms.projectionMatrix[5]  = 2 * zNear / height;
	tr.viewParms.projectionMatrix[9]  = (ymax + ymin) / height;     // normally 0
	tr.viewParms.projectionMatrix[13] = 0;

	tr.viewParms.projectionMatrix[2]  = 0;
	tr.viewParms.projectionMatrix[6]  = 0;
	tr.viewParms.projectionMatrix[10] = -(zFar + zNear) / depth;
	tr.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[3]  = 0;
	tr.viewParms.projectionMatrix[7]  = 0;
	tr.viewParms.projectionMatrix[11] = -1;
	tr.viewParms.projectionMatrix[15] = 0;
}

/**
 * @brief R_MirrorPoint
 * @param[in] in
 * @param[in] surface
 * @param[in] camera
 * @param[out] out
 */
void R_MirrorPoint(vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int    i;
	vec3_t local;
	vec3_t transformed;
	float  d;

	VectorSubtract(in, surface->origin, local);

	VectorClear(transformed);
	for (i = 0 ; i < 3 ; i++)
	{
		d = DotProduct(local, surface->axis[i]);
		VectorMA(transformed, d, camera->axis[i], transformed);
	}

	VectorAdd(transformed, camera->origin, out);
}

/**
 * @brief R_MirrorVector
 * @param[in] in
 * @param[in] surface
 * @param[in] camera
 * @param[out] out
 */
void R_MirrorVector(vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out)
{
	int   i;
	float d;

	VectorClear(out);
	for (i = 0 ; i < 3 ; i++)
	{
		d = DotProduct(in, surface->axis[i]);
		VectorMA(out, d, camera->axis[i], out);
	}
}

/**
 * @brief R_PlaneForSurface
 * @param[in] surfType
 * @param[out] plane
 */
void R_PlaneForSurface(surfaceType_t *surfType, cplane_t *plane)
{
	if (!surfType)
	{
		Com_Memset(plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}

	switch (*surfType)
	{
	case SF_FACE:
		*plane = ((srfSurfaceFace_t *)surfType)->plane;
		return;
	case SF_TRIANGLES:
	{
		vec4_t         plane4;
		srfTriangles_t *tri = (srfTriangles_t *)surfType;
		drawVert_t     *v1  = tri->verts + tri->indexes[0];
		drawVert_t     *v2  = tri->verts + tri->indexes[1];
		drawVert_t     *v3  = tri->verts + tri->indexes[2];

		PlaneFromPoints(plane4, v1->xyz, v2->xyz, v3->xyz);
		VectorCopy(plane4, plane->normal);
		plane->dist = plane4[3];
	}
		return;
	case SF_POLY:
	{
		vec4_t    plane4;
		srfPoly_t *poly = (srfPoly_t *)surfType;

		PlaneFromPoints(plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz);
		VectorCopy(plane4, plane->normal);
		plane->dist = plane4[3];
	}
		return;
	default:
		Com_Memset(plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
}

/**
 * @brief R_GetPortalOrientations
 *
 * @param[in] drawSurf
 * @param[in] entityNum is the entity that the portal surface is a part of, which may
 * be moving and rotating.
 * @param[in,out] surface
 * @param[in,out] camera
 * @param[out] pvsOrigin
 * @param[out] mirror
 * @return qtrue if it should be mirrored
 */
qboolean R_GetPortalOrientations(drawSurf_t *drawSurf, int entityNum,
                                 orientation_t *surface, orientation_t *camera,
                                 vec3_t pvsOrigin, qboolean *mirror)
{
	int           i;
	cplane_t      originalPlane, plane;
	trRefEntity_t *e;
	float         d;
	vec3_t        transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface(drawSurf->surface, &originalPlane);

	// rotate the plane if necessary
	if (entityNum != ENTITYNUM_WORLD)
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity    = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.orientation);

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld(originalPlane.normal, plane.normal);
		plane.dist = originalPlane.dist + DotProduct(plane.normal, tr.orientation.origin);

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct(originalPlane.normal, tr.orientation.origin);
	}
	else
	{
		plane = originalPlane;
	}

	VectorCopy(plane.normal, surface->axis[0]);
	PerpendicularVector(surface->axis[1], surface->axis[0]);
	CrossProduct(surface->axis[0], surface->axis[1], surface->axis[2]);

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for (i = 0 ; i < tr.refdef.num_entities ; i++)
	{
		e = &tr.refdef.entities[i];
		if (e->e.reType != RT_PORTALSURFACE)
		{
			continue;
		}

		d = DotProduct(e->e.origin, originalPlane.normal) - originalPlane.dist;
		if (d > 64 || d < -64)
		{
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy(e->e.oldorigin, pvsOrigin);

		// if the entity is just a mirror, don't use as a camera point
		if (e->e.oldorigin[0] == e->e.origin[0] &&
		    e->e.oldorigin[1] == e->e.origin[1] &&
		    e->e.oldorigin[2] == e->e.origin[2])
		{
			VectorScale(plane.normal, plane.dist, surface->origin);
			VectorCopy(surface->origin, camera->origin);
			VectorSubtract(vec3_origin, surface->axis[0], camera->axis[0]);
			VectorCopy(surface->axis[1], camera->axis[1]);
			VectorCopy(surface->axis[2], camera->axis[2]);

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct(e->e.origin, plane.normal) - plane.dist;
		VectorMA(e->e.origin, -d, surface->axis[0], surface->origin);

		// now get the camera origin and orientation
		VectorCopy(e->e.oldorigin, camera->origin);
		AxisCopy(e->e.axis, camera->axis);
		VectorSubtract(vec3_origin, camera->axis[0], camera->axis[0]);
		VectorSubtract(vec3_origin, camera->axis[1], camera->axis[1]);

		// optionally rotate
		if (e->e.oldframe)
		{
			// if a speed is specified
			if (e->e.frame)
			{
				// continuous rotate
				d = (tr.refdef.time / 1000.0f) * e->e.frame;
				VectorCopy(camera->axis[1], transformed);
				RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
				CrossProduct(camera->axis[0], camera->axis[1], camera->axis[2]);
			}
			else
			{
				// bobbing rotate, with skinNum being the rotation offset
				d = sin(tr.refdef.time * 0.003);
				d = e->e.skinNum + d * 4;
				VectorCopy(camera->axis[1], transformed);
				RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
				CrossProduct(camera->axis[0], camera->axis[1], camera->axis[2]);
			}
		}
		else if (e->e.skinNum)
		{
			d = e->e.skinNum;
			VectorCopy(camera->axis[1], transformed);
			RotatePointAroundVector(camera->axis[1], camera->axis[0], transformed, d);
			CrossProduct(camera->axis[0], camera->axis[1], camera->axis[2]);
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}

/**
 * @brief IsMirror
 * @param[in] drawSurf
 * @param[in] entityNum
 * @return
 */
static qboolean IsMirror(const drawSurf_t *drawSurf, int entityNum)
{
	int           i;
	cplane_t      originalPlane, plane;
	trRefEntity_t *e;
	float         d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface(drawSurf->surface, &originalPlane);

	// rotate the plane if necessary
	if (entityNum != ENTITYNUM_WORLD)
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity    = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity(tr.currentEntity, &tr.viewParms, &tr.orientation);

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld(originalPlane.normal, plane.normal);
		plane.dist = originalPlane.dist + DotProduct(plane.normal, tr.orientation.origin);  // FIXME: plane is never read

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct(originalPlane.normal, tr.orientation.origin);
	}
	else
	{
		plane = originalPlane;  // FIXME: never read
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for (i = 0 ; i < tr.refdef.num_entities ; i++)
	{
		e = &tr.refdef.entities[i];
		if (e->e.reType != RT_PORTALSURFACE)
		{
			continue;
		}

		d = DotProduct(e->e.origin, originalPlane.normal) - originalPlane.dist;
		if (d > 64 || d < -64)
		{
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if (e->e.oldorigin[0] == e->e.origin[0] &&
		    e->e.oldorigin[1] == e->e.origin[1] &&
		    e->e.oldorigin[2] == e->e.origin[2])
		{
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}

/**
 * @brief Determines if a surface is completely offscreen.
 * @param[in] drawSurf
 * @param clipDest - unused
 * @return
 */
static qboolean SurfIsOffscreen(const drawSurf_t *drawSurf, vec4_t clipDest[128])
{
	float        shortest = 100000000;
	int          entityNum;
	int          numTriangles;
	shader_t     *shader;
	int          fogNum;
	int          frontFace;
	int          dlighted;
	vec4_t       clip, eye;
	int          i;
	unsigned int pointAnd = (unsigned int)~0;
	int          j;
	unsigned int pointFlags;

	R_RotateForViewer();

	R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &frontFace, &dlighted);
	RB_BeginSurface(shader, fogNum);
	rb_surfaceTable[*drawSurf->surface](drawSurf->surface);

	etl_assert(tess.numVertexes < 128);

	for (i = 0; i < tess.numVertexes; i++)
	{
		pointFlags = 0;

		R_TransformModelToClip(tess.xyz[i], tr.orientation.modelMatrix, tr.viewParms.projectionMatrix, eye, clip);

		for (j = 0; j < 3; j++)
		{
			if (clip[j] >= clip[3])
			{
				pointFlags |= (1 << (j * 2));
			}
			else if (clip[j] <= -clip[3])
			{
				pointFlags |= (1 << (j * 2 + 1));
			}
		}
		pointAnd &= pointFlags;
	}

	// trivially reject
	if (pointAnd)
	{
		return qtrue;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for (i = 0; i < tess.numIndexes; i += 3)
	{
		vec3_t normal;
		float  len;

		VectorSubtract(tess.xyz[tess.indexes[i]], tr.viewParms.orientation.origin, normal);

		len = VectorLengthSquared(normal);              // lose the sqrt
		if (len < shortest)
		{
			shortest = len;
		}

		if (DotProduct(normal, tess.normal[tess.indexes[i]]) >= 0)
		{
			numTriangles--;
		}
	}

	if (!numTriangles)
	{
		return qtrue;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if (IsMirror(drawSurf, entityNum))
	{
		return qfalse;
	}

	if (shortest > (tess.shader->portalRange * tess.shader->portalRange))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief R_MirrorViewBySurface
 * @param[in] drawSurf
 * @param[in] entityNum
 * @return qtrue if another view has been rendered
 */
qboolean R_MirrorViewBySurface(drawSurf_t *drawSurf, int entityNum)
{
	vec4_t        clipDest[128];
	viewParms_t   newParms;
	viewParms_t   oldParms;
	orientation_t surface, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal)
	{
		Ren_Developer("WARNING: recursive mirror/portal found\n");
		return qfalse;
	}

	if (r_noportals->integer || r_fastSky->integer)
	{
		return qfalse;
	}

	// trivially reject portal/mirror
	if (SurfIsOffscreen(drawSurf, clipDest))
	{
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms          = tr.viewParms;
	newParms.isPortal = qtrue;
	if (!R_GetPortalOrientations(drawSurf, entityNum, &surface, &camera,
	                             newParms.pvsOrigin, &newParms.isMirror))
	{
		return qfalse;      // bad portal, no portalentity
	}

	R_MirrorPoint(oldParms.orientation.origin, &surface, &camera, newParms.orientation.origin);

	VectorSubtract(vec3_origin, camera.axis[0], newParms.portalPlane.normal);
	newParms.portalPlane.dist = DotProduct(camera.origin, newParms.portalPlane.normal);

	R_MirrorVector(oldParms.orientation.axis[0], &surface, &camera, newParms.orientation.axis[0]);
	R_MirrorVector(oldParms.orientation.axis[1], &surface, &camera, newParms.orientation.axis[1]);
	R_MirrorVector(oldParms.orientation.axis[2], &surface, &camera, newParms.orientation.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView(&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

/**
 * @brief See if a sprite is inside a fog volume
 * @param ent
 * @return
 */
int R_SpriteFogNum(trRefEntity_t *ent)
{
	int   i, j;
	fog_t *fog;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	for (i = 1 ; i < tr.world->numfogs ; i++)
	{
		fog = &tr.world->fogs[i];
		for (j = 0 ; j < 3 ; j++)
		{
			if (ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j])
			{
				break;
			}
			if (ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
	}

	return 0;
}

/*
==========================================================================================
DRAWSURF SORTING
==========================================================================================
*/

/**
 * @brief R_Radix
 * @param[in] byte
 * @param[in] size
 * @param[in] source
 * @param[out] dest
 */
static ID_INLINE void R_Radix(int byte, int size, drawSurf_t *source, drawSurf_t *dest)
{
	int           count[256] = { 0 };
	int           index[256];
	int           i;
	unsigned char *sortKey;
	unsigned char *end;

	sortKey = ((unsigned char *)&source[0].sort) + byte;
	end     = sortKey + (size * sizeof(drawSurf_t));
	for ( ; sortKey < end; sortKey += sizeof(drawSurf_t))
		++count[*sortKey];

	index[0] = 0;

	for (i = 1; i < 256; ++i)
		index[i] = index[i - 1] + count[i - 1];

	sortKey = ((unsigned char *)&source[0].sort) + byte;
	for (i = 0; i < size; ++i, sortKey += sizeof(drawSurf_t))
		dest[index[*sortKey]++] = source[i];
}

/**
 * @brief Radix sort with 4 byte size buckets
 * @param[in,out] source
 * @param[in] size
 */
static void R_RadixSort(drawSurf_t *source, int size)
{
	static drawSurf_t scratch[MAX_DRAWSURFS];

#ifdef Q3_LITTLE_ENDIAN
	R_Radix(0, size, source, scratch);
	R_Radix(1, size, scratch, source);
	R_Radix(2, size, source, scratch);
	R_Radix(3, size, scratch, source);
#else
	R_Radix(3, size, source, scratch);
	R_Radix(2, size, scratch, source);
	R_Radix(1, size, source, scratch);
	R_Radix(0, size, scratch, source);
#endif //Q3_LITTLE_ENDIAN
}

/**
 * @brief R_AddDrawSurf
 * @param[in] surface
 * @param[in] shader
 * @param[in] fogNum
 * @param[in] frontFace
 * @param[in] dlightMap
 */
void R_AddDrawSurf(surfaceType_t *surface, shader_t *shader, int fogNum, int frontFace, int dlightMap)
{
	int index;

	// we can't just wrap around here, because the surface count will be
	// off.  Check for overflow, and drop new surfaces on overflow.
	if (tr.refdef.numDrawSurfs >= MAX_DRAWSURFS)
	{
		return;
	}

	if (*surface >= SF_NUM_SURFACE_TYPES)
	{
		Ren_Print("Warning R_AddDrawSurf: invalid surface type [%i] skipped (shader [%s] - fogNum [%i] - fontFace [%i] - dlightMap [%i])\n", *surface, shader->name, fogNum, frontFace, dlightMap);
		return;
	}

	index = tr.refdef.numDrawSurfs;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT)
	                                  | tr.shiftedEntityNum | (fogNum << QSORT_FOGNUM_SHIFT) | (frontFace << QSORT_FRONTFACE_SHIFT) | dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}

/**
 * @brief R_DecomposeSort
 * @param[in] sort
 * @param[out] entityNum
 * @param[out] shader
 * @param[out] fogNum
 * @param[out] frontFace
 * @param[out] dlightMap
 */
void R_DecomposeSort(unsigned sort, int *entityNum, shader_t **shader, int *fogNum, int *frontFace, int *dlightMap)
{
	*fogNum    = (sort >> QSORT_FOGNUM_SHIFT) & 31;
	*shader    = tr.sortedShaders[(sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1)];
	*entityNum = (sort >> QSORT_ENTITYNUM_SHIFT) & (MAX_GENTITIES - 1);       // (SA) uppded entity count for Wolf to 11 bits
	*frontFace = (sort >> QSORT_FRONTFACE_SHIFT) & 1;
	*dlightMap = sort & 1;
}

/**
 * @brief R_SortDrawSurfs
 * @param[in] drawSurfs
 * @param[in] numDrawSurfs
 */
void R_SortDrawSurfs(drawSurf_t *drawSurfs, int numDrawSurfs)
{
	shader_t *shader;
	int      fogNum;
	int      entityNum;
	int      frontFace;
	int      dlighted;
	int      i;

	// it is possible for some views to not have any surfaces
	if (numDrawSurfs < 1)
	{
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if (numDrawSurfs > MAX_DRAWSURFS)
	{
		Ren_Print("R_SortDrawSurfs Warning: MAX_DRAWSURFS limit reached\n");
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	R_RadixSort(drawSurfs, numDrawSurfs);

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for (i = 0 ; i < numDrawSurfs ; i++)
	{
		R_DecomposeSort((drawSurfs + i)->sort, &entityNum, &shader, &fogNum, &frontFace, &dlighted);

		if (shader->sort > SS_PORTAL)
		{
			break;
		}

		// no shader should ever have this sort type
		if (shader->sort == SS_BAD)
		{
			Ren_Drop("Shader '%s'with sort == SS_BAD", shader->name);
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if (R_MirrorViewBySurface((drawSurfs + i), entityNum))
		{
			// this is a debug option to see exactly what is being mirrored
			if (r_portalOnly->integer)
			{
				return;
			}
			break;      // only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd(drawSurfs, numDrawSurfs);
}

/**
 * @brief R_AddEntitySurfaces
 */
void R_AddEntitySurfaces(void)
{
	trRefEntity_t *ent;
	shader_t      *shader;

	if (!r_drawEntities->integer)
	{
		return;
	}

	for (tr.currentEntityNum = 0;
	     tr.currentEntityNum < tr.refdef.num_entities;
	     tr.currentEntityNum++)
	{
		ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = qfalse;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in
		// mirrors, because the true body position will already be drawn
		if ((ent->e.renderfx & RF_FIRST_PERSON) && tr.viewParms.isPortal)
		{
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch (ent->e.reType)
		{
		case RT_PORTALSURFACE:
			break;      // don't draw anything
		case RT_SPRITE:
		case RT_SPLASH:
		case RT_BEAM:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_CORE_TAPER:
		case RT_RAIL_RINGS:
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
			{
				continue;
			}
			shader = R_GetShaderByHandle(ent->e.customShader);
			R_AddDrawSurf(&entitySurface, shader, R_SpriteFogNum(ent), 0, 0);
			break;
		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity(ent, &tr.viewParms, &tr.orientation);

			tr.currentModel = R_GetModelByHandle(ent->e.hModel);
			if (!tr.currentModel)
			{
				R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0, 0);
			}
			else
			{
				switch (tr.currentModel->type)
				{
				case MOD_MESH:
					R_AddMD3Surfaces(ent);
					break;
				case MOD_MDC:
					R_AddMDCSurfaces(ent);
					break;
				case MOD_MDS:
					R_AddAnimSurfaces(ent);
					break;
				case MOD_MDM:
					R_MDM_AddAnimSurfaces(ent);
					break;
				case MOD_BRUSH:
					R_AddBrushModelSurfaces(ent);
					break;
				case MOD_BAD:       // null model axis
					if ((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
					{
						break;
					}
					R_AddDrawSurf(&entitySurface, tr.defaultShader, 0, 0, 0);
					break;
				default:
					Ren_Drop("R_AddEntitySurfaces: Bad modeltype");
				}
			}
			break;
		default:
			Ren_Drop("R_AddEntitySurfaces: Bad reType");
		}
	}
}

/**
 * @brief R_GenerateDrawSurfs
 */
void R_GenerateDrawSurfs(void)
{
	// set the projection matrix (and view frustum) here
	// first with max or fog distance so we can have proper
	// arbitrary frustum farplane culling optimization
	if ((tr.refdef.rdflags & RDF_NOWORLDMODEL) || tr.world == NULL)
	{
		VectorSet(tr.viewParms.visBounds[0], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD);
		VectorSet(tr.viewParms.visBounds[1], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD);
	}
	else
	{
		VectorCopy(tr.world->nodes->mins, tr.viewParms.visBounds[0]);
		VectorCopy(tr.world->nodes->maxs, tr.viewParms.visBounds[1]);
	}

	R_SetupProjection();

	R_CullDecalProjectors();
	R_CullDlights();

	R_AddWorldSurfaces();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	R_SetupProjection();

	R_AddEntitySurfaces();

	R_AddPolygonSurfaces();

	R_AddPolygonBufferSurfaces();
}

/**
 * @brief R_DebugPolygon
 * @param[in] color
 * @param[in] numPoints
 * @param[in] points
 */
void R_DebugPolygon(int color, int numPoints, float *points)
{
	int i;

	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

	// draw solid shade
	qglColor4f( color & 1, (color >> 1) & 1, (color >> 2) & 1, 1.0f );
	qglVertexPointer  ( 3, GL_FLOAT, 0, points );
	qglDrawArrays( GL_TRIANGLE_FAN, 0, numPoints );

	// draw wireframe outline
	qglDepthRange(0, 0);
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	qglVertexPointer  ( 3, GL_FLOAT, 0, points );
	qglDrawArrays( GL_LINES, 0, numPoints );
	qglDepthRange(0, 1);
}

/**
 * @brief R_DebugText
 * @param org          - unused
 * @param r            - unused
 * @param g            - unused
 * @param b            - unused
 * @param text         - unused
 * @param neverOcclude - unused
 *
 * @todo TODO: R_DebugText Unimplemented!
 */
void R_DebugText(const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude)
{
	Ren_Print("TODO: R_DebugText Unimplemented!\n");
	/*
	if ( neverOcclude ) {
	    qglDepthRange( 0, 0 );  // never occluded

	}
	qglColor3f( r, g, b );
	qglRasterPos3fv( org );
	qglPushAttrib( GL_LIST_BIT );
	qglListBase( gl_NormalFontBase );
	qglCallLists( strlen( text ), GL_UNSIGNED_BYTE, text );
	qglListBase( 0 );
	qglPopAttrib();

	if ( neverOcclude ) {
	    qglDepthRange( 0, 1 );
	}
	*/
}

/**
 * @brief Visualization aid for movement clipping debugging
 */
void R_DebugGraphics(void)
{
	if (!r_debugSurface->integer)
	{
		return;
	}

	R_FogOff(); // moved this in here to keep from /always/ doing the fog state change

	// the render thread can't make callbacks to the main thread
	R_IssuePendingRenderCommands();

	GL_Bind(tr.whiteImage);
	GL_Cull(CT_FRONT_SIDED);
	ri.CM_DrawDebugSurface(R_DebugPolygon);
}

/**
 * @brief A view may be either the actual camera view,
 * or a mirror / remote location
 * @param parms
 */
void R_RenderView(viewParms_t *parms)
{
	int firstDrawSurf;

	if (parms->viewportWidth <= 0 || parms->viewportHeight <= 0)
	{
		return;
	}

	// purge media that were left over from the last level
	if (r_cache->integer)
	{
		extern void R_PurgeBackupImages(int purgeCount);
		static int lastTime;

		if ((lastTime > tr.refdef.time) || (lastTime < (tr.refdef.time - 200)))
		{
			R_FreeImageBuffer();    // clear all image buffers
			R_PurgeShaders(10);
			R_PurgeBackupImages(1);
			R_PurgeModels(1);
			lastTime = tr.refdef.time;
		}
	}

	tr.viewCount++;

	tr.viewParms               = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount    = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer();

	R_GenerateDrawSurfs();

	R_SortDrawSurfs(tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf);

	// draw main system development information (surface outlines, etc)
	R_FogOff();
	R_DebugGraphics();
	R_FogOn();
}
