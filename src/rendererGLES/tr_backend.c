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
 * @file rendererGLES/tr_backend.c
 */

#include "tr_local.h"

backEndData_t  *backEndData;
backEndState_t backEnd;

/**
 * @var s_flipMatrix
 * @brief Convert from our coordinate system (looking down X)
 * to OpenGL's coordinate system (looking down -Z)
 */
static float s_flipMatrix[16] =
{
	0,  0, -1, 0,
	-1, 0, 0,  0,
	0,  1, 0,  0,
	0,  0, 0,  1
};

/**
 * @brief GL_Bind
 * @param[in,out] image
 */
void GL_Bind(image_t *image)
{
	int texnum;

	if (!image)
	{
		Ren_Warning("GL_Bind: NULL image\n");
		texnum = tr.defaultImage->texnum;
	}
	else
	{
		texnum = image->texnum;
	}

	if (r_noBind->integer && tr.dlightImage)            // performance evaluation option
	{
		texnum = tr.dlightImage->texnum;
	}

	if (glState.currenttextures[glState.currenttmu] != texnum)
	{
		if (image)
		{
			image->frameUsed = tr.frameCount;
		}

		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture(GL_TEXTURE_2D, texnum);
	}
}

/**
 * @brief GL_SelectTexture
 * @param[in] unit
 */
void GL_SelectTexture(int unit)
{
	if (glState.currenttmu == unit)
	{
		return;
	}

	if (unit == 0)
	{
		qglActiveTextureARB(GL_TEXTURE0_ARB);
		Ren_LogComment("glActiveTextureARB( GL_TEXTURE0_ARB )\n");
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		Ren_LogComment("glClientActiveTextureARB( GL_TEXTURE0_ARB )\n");
	}
	else if (unit == 1)
	{
		qglActiveTextureARB(GL_TEXTURE1_ARB);
		Ren_LogComment("glActiveTextureARB( GL_TEXTURE1_ARB )\n");
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		Ren_LogComment("glClientActiveTextureARB( GL_TEXTURE1_ARB )\n");
	}
	else
	{
		Ren_Drop("GL_SelectTexture: unit = %i", unit);
	}

	glState.currenttmu = unit;
}

/*
 * @brief GL_BindMultitexture
 * @param image0
 * @param env0 - unused
 * @param image1
 * @param env1 - unused
 *
 * @note Unused
void GL_BindMultitexture(image_t *image0, GLuint env0, image_t *image1, GLuint env1)
{
    int texnum0 = image0->texnum;
    int texnum1 = image1->texnum;

    if (r_nobind->integer && tr.dlightImage)            // performance evaluation option
    {
        texnum0 = texnum1 = tr.dlightImage->texnum;
    }

    if (glState.currenttextures[1] != texnum1)
    {
        GL_SelectTexture(1);
        image1->frameUsed          = tr.frameCount;
        glState.currenttextures[1] = texnum1;
        qglBindTexture(GL_TEXTURE_2D, texnum1);
    }
    if (glState.currenttextures[0] != texnum0)
    {
        GL_SelectTexture(0);
        image0->frameUsed          = tr.frameCount;
        glState.currenttextures[0] = texnum0;
        qglBindTexture(GL_TEXTURE_2D, texnum0);
    }
}
*/

/**
 * @brief GL_Cull
 * @param[in] cullType
 */
void GL_Cull(int cullType)
{
	if (glState.faceCulling == cullType)
	{
		return;
	}

	glState.faceCulling = cullType;

	if (cullType == CT_TWO_SIDED)
	{
		qglDisable(GL_CULL_FACE);
	}
	else
	{
		qboolean cullFront;
		qglEnable(GL_CULL_FACE);

		cullFront = (cullType == CT_FRONT_SIDED);
		if (backEnd.viewParms.isMirror)
		{
			cullFront = !cullFront;
		}

		qglCullFace(cullFront ? GL_FRONT : GL_BACK);
	}
}

/**
 * @brief GL_TexEnv
 * @param[in] env
 */
void GL_TexEnv(int env)
{
	if (env == glState.texEnv[glState.currenttmu])
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch (env)
	{
	case GL_MODULATE:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case GL_REPLACE:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	case GL_DECAL:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		break;
	case GL_ADD:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		break;
	default:
		Ren_Drop("GL_TexEnv: invalid env '%d' passed\n", env);
	}
}

/**
 * @brief This routine is responsible for setting the most commonly changed state in Q3.
 * @param[in] stateBits
 */
void GL_State(unsigned long stateBits)
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if (!diff)
	{
		return;
	}

	// check depthFunc bits
	if (diff & GLS_DEPTHFUNC_EQUAL)
	{
		if (stateBits & GLS_DEPTHFUNC_EQUAL)
		{
			qglDepthFunc(GL_EQUAL);
		}
		else
		{
			qglDepthFunc(GL_LEQUAL);
		}
	}

	// check blend bits
	if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
	{
		GLenum srcFactor, dstFactor;

		if (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
		{
			switch (stateBits & GLS_SRCBLEND_BITS)
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				srcFactor = GL_ONE;     // to get warning to shut up
				Ren_Drop("GL_State: invalid src blend state bits\n");
			}

			switch (stateBits & GLS_DSTBLEND_BITS)
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				dstFactor = GL_ONE;     // to get warning to shut up
				Ren_Drop("GL_State: invalid dst blend state bits\n");
			}

			qglEnable(GL_BLEND);
			qglBlendFunc(srcFactor, dstFactor);
		}
		else
		{
			qglDisable(GL_BLEND);
		}
	}

	// check depthmask
	if (diff & GLS_DEPTHMASK_TRUE)
	{
		if (stateBits & GLS_DEPTHMASK_TRUE)
		{
			qglDepthMask(GL_TRUE);
		}
		else
		{
			qglDepthMask(GL_FALSE);
		}
	}

	// depthtest
	if (diff & GLS_DEPTHTEST_DISABLE)
	{
		if (stateBits & GLS_DEPTHTEST_DISABLE)
		{
			qglDisable(GL_DEPTH_TEST);
		}
		else
		{
			qglEnable(GL_DEPTH_TEST);
		}
	}

	// alpha test
	if (diff & GLS_ATEST_BITS)
	{
		switch (stateBits & GLS_ATEST_BITS)
		{
		case 0:
			qglDisable(GL_ALPHA_TEST);
			break;
		case GLS_ATEST_GT_0:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GREATER, 0.0f);
			break;
		case GLS_ATEST_LT_80:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_LESS, 0.5f);
			break;
		case GLS_ATEST_GE_80:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GEQUAL, 0.5f);
			break;
		default:
			etl_assert(0);
			break;
		}
	}

	glState.glStateBits = stateBits;
}

/**
 * @brief A player has predicted a teleport, but hasn't arrived yet
 */
static void RB_Hyperspace(void)
{
	float c = (backEnd.refdef.time & 255) / 255.0f;

	qglClearColor(c, c, c, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	backEnd.isHyperspace = qtrue;
}

/**
 * @brief SetViewportAndScissor
 */
static void SetViewportAndScissor(void)
{
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf(backEnd.viewParms.projectionMatrix);
	qglMatrixMode(GL_MODELVIEW);

	// set the window clipping
	qglViewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
	qglScissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}

/**
 * @brief Any mirrored or portaled views have already been drawn, so prepare
 * to actually render the visible surfaces for this view
 */
void RB_BeginDrawingView(void)
{
	int clearBits = 0;

	// sync with gl if needed
	if (r_finish->integer == 1 && !glState.finishCalled)
	{
		qglFinish();
		glState.finishCalled = qtrue;
	}
	if (r_finish->integer == 0)
	{
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	// set the modelview matrix for the viewer
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State(GLS_DEFAULT);


	////////// modified to ensure one glclear() per frame at most

	// clear relevant buffers
	clearBits = 0;

	if (r_measureOverdraw->integer || r_shadows->integer == 2)
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	// global q3 fog volume
	else if (tr.world && tr.world->globalFog >= 0)
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;
		clearBits |= GL_COLOR_BUFFER_BIT;
		//
		qglClearColor(tr.world->fogs[tr.world->globalFog].shader->fogParms.color[0] * tr.identityLight,
		              tr.world->fogs[tr.world->globalFog].shader->fogParms.color[1] * tr.identityLight,
		              tr.world->fogs[tr.world->globalFog].shader->fogParms.color[2] * tr.identityLight, 1.0);
	}
	else if (skyboxportal)
	{
		if (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)     // portal scene, clear whatever is necessary
		{
			clearBits |= GL_DEPTH_BUFFER_BIT;

			if (r_fastSky->integer || (backEnd.refdef.rdflags & RDF_NOWORLDMODEL))      // fastsky: clear color
			{   // try clearing first with the portal sky fog color, then the world fog color, then finally a default
				clearBits |= GL_COLOR_BUFFER_BIT;
				if (glfogsettings[FOG_PORTALVIEW].registered)
				{
					qglClearColor(glfogsettings[FOG_PORTALVIEW].color[0], glfogsettings[FOG_PORTALVIEW].color[1], glfogsettings[FOG_PORTALVIEW].color[2], glfogsettings[FOG_PORTALVIEW].color[3]);
				}
				else if (glfogNum > FOG_NONE && glfogsettings[FOG_CURRENT].registered)
				{
					qglClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);
				}
				else
				{
					qglClearColor(0.5, 0.5, 0.5, 1.0);
				}
			}
			else                                                        // rendered sky (either clear color or draw quake sky)
			{
				if (glfogsettings[FOG_PORTALVIEW].registered)
				{
					qglClearColor(glfogsettings[FOG_PORTALVIEW].color[0], glfogsettings[FOG_PORTALVIEW].color[1], glfogsettings[FOG_PORTALVIEW].color[2], glfogsettings[FOG_PORTALVIEW].color[3]);

					if (glfogsettings[FOG_PORTALVIEW].clearscreen)        // portal fog requests a screen clear (distance fog rather than quake sky)
					{
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}

			}
		}
		else  // world scene with portal sky, don't clear any buffers, just set the fog color if there is one
		{
			clearBits |= GL_DEPTH_BUFFER_BIT;   // this will go when I get the portal sky rendering way out in the zbuffer (or not writing to zbuffer at all)

			if (glfogNum > FOG_NONE && glfogsettings[FOG_CURRENT].registered)
			{
				if (backEnd.refdef.rdflags & RDF_UNDERWATER)
				{
					if (glfogsettings[FOG_CURRENT].mode == GL_LINEAR)
					{
						clearBits |= GL_COLOR_BUFFER_BIT;
					}

				}
				else if (!(r_portalSky->integer)) // portal skies have been manually turned off, clear bg color
				{
					clearBits |= GL_COLOR_BUFFER_BIT;
				}

				qglClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);
			}
			else if (!(r_portalSky->integer)) // portal skies have been manually turned off, clear bg color
			{
				clearBits |= GL_COLOR_BUFFER_BIT;
				qglClearColor(0.5, 0.5, 0.5, 1.0);
			}
		}
	}
	else // world scene with no portal sky
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;

		// we don't want to clear the buffer when no world model is specified
		if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		{
			clearBits &= ~GL_COLOR_BUFFER_BIT;
		}
		else if (r_fastSky->integer || (backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{

			clearBits |= GL_COLOR_BUFFER_BIT;

			if (glfogsettings[FOG_CURRENT].registered)     // try to clear fastsky with current fog color
			{
				qglClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);
			}
			else
			{
				qglClearColor(0.05f, 0.05f, 0.05f, 1.0f);    // JPW NERVE changed per id req was 0.5s
			}
		}
		else  // world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
		{
			if (glfogsettings[FOG_CURRENT].registered)     // try to clear fastsky with current fog color
			{
				qglClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);

				if (glfogsettings[FOG_CURRENT].clearscreen)       // world fog requests a screen clear (distance fog rather than quake sky)
				{
					clearBits |= GL_COLOR_BUFFER_BIT;
				}
			}
		}
	}

	// don't clear the color buffer when no world model is specified
	if (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		clearBits &= ~GL_COLOR_BUFFER_BIT;
	}

	if (clearBits)
	{
		qglClear(clearBits);
	}

	if ((backEnd.refdef.rdflags & RDF_HYPERSPACE))
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1; // force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if (backEnd.viewParms.isPortal)
	{
		float  plane[4], plane2[4]; // OpenGLES expecting float here on both

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct(backEnd.viewParms.orientation.axis[0], plane);
		plane2[1] = DotProduct(backEnd.viewParms.orientation.axis[1], plane);
		plane2[2] = DotProduct(backEnd.viewParms.orientation.axis[2], plane);
		plane2[3] = DotProduct(plane, backEnd.viewParms.orientation.origin) - plane[3];

		qglLoadMatrixf(s_flipMatrix);
		qglClipPlane(GL_CLIP_PLANE0, plane2);
		qglEnable(GL_CLIP_PLANE0);
	}
	else
	{
		qglDisable(GL_CLIP_PLANE0);
	}
}

/**
 * @brief RB_RenderDrawSurfList
 * @param[in] drawSurfs
 * @param[in] numDrawSurfs
 */
void RB_RenderDrawSurfList(drawSurf_t *drawSurfs, int numDrawSurfs)
{
	shader_t   *shader, *oldShader;
	int        fogNum, oldFogNum;
	int        entityNum, oldEntityNum;
	int        frontFace;
	int        dlighted, oldDlighted;
	qboolean   depthRange, oldDepthRange;
	int        i;
	drawSurf_t *drawSurf;
	int        oldSort;
	double     originalTime = backEnd.refdef.floatTime; // save original time for entity shader offsets

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView();

	// draw everything
	oldEntityNum          = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader             = NULL;
	oldFogNum             = -1;
	oldDepthRange         = qfalse;
	oldDlighted           = qfalse;
	oldSort               = -1;
	depthRange            = qfalse;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++)
	{
		if (drawSurf->sort == oldSort)
		{
			// fast path, same as previous sort
			rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &frontFace, &dlighted);

		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader && (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted
		               || (entityNum != oldEntityNum && !shader->entityMergable)))
		{
			if (oldShader != NULL)
			{
				RB_EndSurface();
			}
			RB_BeginSurface(shader, fogNum);
			oldShader   = shader;
			oldFogNum   = fogNum;
			oldDlighted = dlighted;
		}

		// change the modelview matrix if needed
		if (entityNum != oldEntityNum)
		{
			depthRange = qfalse;

			if (entityNum != ENTITYNUM_WORLD)
			{
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];

				// FIXME: e.shaderTime must be passed as int to avoid fp-precision loss issues
				backEnd.refdef.floatTime = originalTime; // - backEnd.currentEntity->e.shaderTime; // JPW NERVE pulled this to match q3ta

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				// tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				// set up the dynamic lighting if needed
				if (backEnd.currentEntity->needDlights)
				{
					R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation);
				}

				if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity    = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orientation      = backEnd.viewParms.world;

				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				// tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation);
			}

			qglLoadMatrixf(backEnd.orientation.modelMatrix);

			// change depthrange if needed
			if (oldDepthRange != depthRange)
			{
				if (depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
	}

	// draw the contents of the last shader batch
	if (oldShader != NULL)
	{
		RB_EndSurface();
	}

	// go back to the world modelview matrix
	backEnd.currentEntity    = &tr.worldEntity;
	backEnd.refdef.floatTime = originalTime;
	backEnd.orientation      = backEnd.viewParms.world;
	R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation);

	qglLoadMatrixf(backEnd.viewParms.world.modelMatrix);
	if (depthRange)
	{
		qglDepthRange(0, 1);
	}

	// draw sun
	RB_DrawSun();

	// darken down any stencil shadows
	RB_ShadowFinish();

	// add light flares on lights that aren't obscured
	RB_RenderFlares();
}

/*
============================================================================
RENDER BACK END FUNCTIONS
============================================================================
*/

/**
 * @brief RB_SetGL2D
 */
void RB_SetGL2D(void)
{
	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	GL_State(GLS_DEPTHTEST_DISABLE |
	         GLS_SRCBLEND_SRC_ALPHA |
	         GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	qglDisable(GL_CULL_FACE);
	qglDisable(GL_CLIP_PLANE0);

	// set time for 2D shaders
	backEnd.refdef.time      = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001;
}

/**
 * @brief Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
 * Used for cinematics.
 *
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] cols
 * @param[in] rows
 * @param[in] data
 * @param[in] client
 * @param[in] dirty
 *
 * @todo FIXME: not exactly backend
 */
void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	int i, j;
	int start;

	if (!tr.registered)
	{
		return;
	}
	R_IssuePendingRenderCommands();

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = 0;
	if (r_speeds->integer)
	{
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for (i = 0; (1 << i) < cols; i++)
	{
	}
	for (j = 0; (1 << j) < rows; j++)
	{
	}
	if ((1 << i) != cols || (1 << j) != rows)
	{
		Ren_Drop("Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	GL_Bind(tr.scratchImage[client]);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if (cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height)
	{
		tr.scratchImage[client]->width  = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
#ifndef __ANDROID__
		qglTexImage2D(GL_TEXTURE_2D, 0, 3, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#else
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#endif
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		if (dirty)
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}

	if (r_speeds->integer)
	{
		int end = ri.Milliseconds();

		Ren_Print("qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	RB_SetGL2D();

	qglColor3f(tr.identityLight, tr.identityLight, tr.identityLight);

	// OpenGLES implementation
	GLfloat tex[] =
	{
		0.5f / cols,          0.5f / rows,
		(cols - 0.5f) / cols, 0.5f / rows,
		(cols - 0.5f) / cols, (rows - 0.5f) / rows,
		0.5f / cols,          (rows - 0.5f) / rows
	};
	GLfloat vtx[] =
	{
		x,     y,
		x + w, y,
		x + w, y + h,
		x,     y + h
	};
	GLboolean text  = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (glcol)
	{
		qglDisableClientState(GL_COLOR_ARRAY);
	}
	if (!text)
	{
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	qglEnableClientState(GL_VERTEX_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tex);
	qglVertexPointer(2, GL_FLOAT, 0, vtx);
	qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	if (glcol)
	{
		qglEnableClientState(GL_COLOR_ARRAY);
	}
	if (!text)
	{
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
}

/**
 * @brief RE_UploadCinematic
 *
 * @param w - unused
 * @param h - unused
 * @param[in] cols
 * @param[in] rows
 * @param[in] data
 * @param[in] client
 * @param[in] dirty
 */
void RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
	GL_Bind(tr.scratchImage[client]);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if (cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height)
	{
		tr.scratchImage[client]->width  = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
#ifndef __ANDROID__
		qglTexImage2D(GL_TEXTURE_2D, 0, 3, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#else
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#endif
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		if (dirty)
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
}

/**
 * @brief RB_SetColor
 * @param[in] data
 * @return
 */
const void *RB_SetColor(const void *data)
{
	const setColorCommand_t *cmd = ( const setColorCommand_t * ) data;

	backEnd.color2D[0] = (byte)(cmd->color[0] * 255);
	backEnd.color2D[1] = (byte)(cmd->color[1] * 255);
	backEnd.color2D[2] = (byte)(cmd->color[2] * 255);
	backEnd.color2D[3] = (byte)(cmd->color[3] * 255);

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_StretchPic
 * @param[in] data
 * @return
 */
const void *RB_StretchPic(const void *data)
{
	const stretchPicCommand_t *cmd = ( const stretchPicCommand_t * ) data;
	shader_t                  *shader;
	int                       numVerts, numIndexes;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.shader)
	{
		if (tess.numIndexes)
		{
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(4, 6);
	numVerts   = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes  += 6;

	tess.indexes[numIndexes]     = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	*( int * ) tess.vertexColors[numVerts]                 =
	    *( int * ) tess.vertexColors[numVerts + 1]         =
	        *( int * ) tess.vertexColors[numVerts + 2]     =
	            *( int * ) tess.vertexColors[numVerts + 3] = *( int * ) backEnd.color2D;

	tess.xyz[numVerts][0] = cmd->x;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_Draw2dPolys
 * @param[in] data
 * @return
 */
const void *RB_Draw2dPolys(const void *data)
{
	const poly2dCommand_t *cmd = ( const poly2dCommand_t * ) data;
	shader_t              *shader;
	int                   i;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.shader)
	{
		if (tess.numIndexes)
		{
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(cmd->numverts, (cmd->numverts - 2) * 3);

	for (i = 0; i < cmd->numverts - 2; i++)
	{
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes                  += 3;
	}

	for (i = 0; i < cmd->numverts; i++)
	{
		tess.xyz[tess.numVertexes][0] = cmd->verts[i].xyz[0];
		tess.xyz[tess.numVertexes][1] = cmd->verts[i].xyz[1];
		tess.xyz[tess.numVertexes][2] = 0;

		tess.texCoords[tess.numVertexes][0][0] = cmd->verts[i].st[0];
		tess.texCoords[tess.numVertexes][0][1] = cmd->verts[i].st[1];

		tess.vertexColors[tess.numVertexes][0] = cmd->verts[i].modulate[0];
		tess.vertexColors[tess.numVertexes][1] = cmd->verts[i].modulate[1];
		tess.vertexColors[tess.numVertexes][2] = cmd->verts[i].modulate[2];
		tess.vertexColors[tess.numVertexes][3] = cmd->verts[i].modulate[3];
		tess.numVertexes++;
	}

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_RotatedPic
 * @param[in] data
 * @return
 */
const void *RB_RotatedPic(const void *data)
{
	const stretchPicCommand_t *cmd = ( const stretchPicCommand_t * ) data;
	shader_t                  *shader;
	int                       numVerts, numIndexes;
	float                     angle;
	float                     pi2 = M_PI * 2;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.shader)
	{
		if (tess.numIndexes)
		{
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(4, 6);
	numVerts   = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes  += 6;

	tess.indexes[numIndexes]     = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	*( int * ) tess.vertexColors[numVerts]                 =
	    *( int * ) tess.vertexColors[numVerts + 1]         =
	        *( int * ) tess.vertexColors[numVerts + 2]     =
	            *( int * ) tess.vertexColors[numVerts + 3] = *( int * ) backEnd.color2D;

	angle                 = cmd->angle * pi2;
	tess.xyz[numVerts][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	angle                     = cmd->angle * pi2 + 0.25 * pi2;
	tess.xyz[numVerts + 1][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts + 1][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	angle                     = cmd->angle * pi2 + 0.50 * pi2;
	tess.xyz[numVerts + 2][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts + 2][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	angle                     = cmd->angle * pi2 + 0.75 * pi2;
	tess.xyz[numVerts + 3][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts + 3][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_StretchPicGradient
 * @param[in] data
 * @return
 */
const void *RB_StretchPicGradient(const void *data)
{
	const stretchPicCommand_t *cmd = ( const stretchPicCommand_t * ) data;
	shader_t                  *shader;
	int                       numVerts, numIndexes;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if (shader != tess.shader)
	{
		if (tess.numIndexes)
		{
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(4, 6);
	numVerts   = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes  += 6;

	tess.indexes[numIndexes]     = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;

	*( int * ) tess.vertexColors[numVerts]         =
	    *( int * ) tess.vertexColors[numVerts + 1] = *( int * ) backEnd.color2D;

	*( int * ) tess.vertexColors[numVerts + 2]     =
	    *( int * ) tess.vertexColors[numVerts + 3] = *( int * ) cmd->gradientColor;

	tess.xyz[numVerts][0] = cmd->x;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;

	tess.texCoords[numVerts + 3][0][0] = cmd->s1;
	tess.texCoords[numVerts + 3][0][1] = cmd->t2;

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_DrawSurfs
 * @param[in] data
 * @return
 */
const void *RB_DrawSurfs(const void *data)
{
	const drawSurfsCommand_t *cmd;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	cmd = ( const drawSurfsCommand_t * ) data;

	backEnd.refdef    = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList(cmd->drawSurfs, cmd->numDrawSurfs);

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_DrawBuffer
 * @param[in] data
 * @return
 */
const void *RB_DrawBuffer(const void *data)
{
	const drawBufferCommand_t *cmd = ( const drawBufferCommand_t * ) data;

	// clear screen for debugging
	if (r_clear->integer)
	{
		qglClearColor(1, 0, 0.5, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return ( const void * ) (cmd + 1);
}



/**
 * @brief Draw all the images to the screen, on top of whatever
 * was there.  This is used to test for texture thrashing.
 *
 * Also called by RE_EndRegistration
 */
void RB_ShowImages(void)
{
	int     i;
	image_t *image;
	float   x, y, w, h;
	int     start, end;

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	qglClear(GL_COLOR_BUFFER_BIT);

	qglFinish();

	start = ri.Milliseconds();
	// OpenGLES Implementation
	GLboolean text  = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (glcol)
	{
		qglDisableClientState(GL_COLOR_ARRAY);
	}
	if (!text)
	{
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	for (i = 0 ; i < tr.numImages ; i++)
	{
		image = tr.images[i];

		w = glConfig.vidWidth / 40;
		h = glConfig.vidHeight / 30;

		x = i % 40 * w;
		y = i / 30 * h;

		// show in proportional size in mode 2
		if (r_showImages->integer == 2)
		{
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		// OpenGLES Implementation
		GLfloat tex[] =
		{
			0, 0,
			1, 0,
			1, 1,
			0, 1
		};
		GLfloat vtx[] =
		{
			x,     y,
			x + w, y,
			x + w, y + h,
			x,     y + h
		};
		qglTexCoordPointer(2, GL_FLOAT, 0, tex);
		qglVertexPointer(2, GL_FLOAT, 0, vtx);
		qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	}

	if (glcol)
	{
		qglEnableClientState(GL_COLOR_ARRAY);
	}
	if (!text)
	{
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);

	}

	qglFinish();

	end = ri.Milliseconds();
	Ren_Print("%i msec to draw all images\n", end - start);
}

/*
 * @brief RB_DrawBounds
 * @param[in,out] mins
 * @param[in,out] maxs
 *
 * @note Unused.
void RB_DrawBounds(vec3_t mins, vec3_t maxs)
{
    vec3_t center;

    GL_Bind(tr.whiteImage);
    GL_State(GLS_POLYMODE_LINE);

    // OpenGLES Implementation
    // box corners
    GLboolean text  = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
    GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
    if (glcol)
    {
        qglDisableClientState(GL_COLOR_ARRAY);
    }
    if (text)
    {
        qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    GLfloat vtx1[] =
    {
        mins[0], mins[1], mins[2],
        maxs[0], mins[1], mins[2],
        mins[0], mins[1], mins[2],
        mins[0], maxs[1], mins[2],
        mins[0], mins[1], mins[2],
        mins[0], mins[1], maxs[2],
        maxs[0], maxs[1], maxs[2],
        mins[0], maxs[1], maxs[2],
        maxs[0], maxs[1], maxs[2],
        maxs[0], mins[1], maxs[2],
        maxs[0], maxs[1], maxs[2],
        maxs[0], maxs[1], mins[2]
    };
    qglColor3f(1, 1, 1);
    qglVertexPointer(3, GL_FLOAT, 0, vtx1);
    qglDrawArrays(GL_LINES, 0, 12);
    center[0] = (mins[0] + maxs[0]) * 0.5f;
    center[1] = (mins[1] + maxs[1]) * 0.5f;
    center[2] = (mins[2] + maxs[2]) * 0.5f;

    // OpenGLES Implementation
    // center axis
    GLfloat vtx2[] =
    {
        mins[0], center[1], center[2],
        maxs[0], center[1], center[2],
        center[0], mins[1], center[2],
        center[0], maxs[1], center[2],
        center[0], center[1], mins[2],
        center[0], center[1], maxs[2]
    };
    qglColor3f(1, 0.85, 0);
    qglVertexPointer(3, GL_FLOAT, 0, vtx2);
    qglDrawArrays(GL_LINES, 0, 6);
    if (glcol)
    {
        qglEnableClientState(GL_COLOR_ARRAY);
    }
    if (text)
    {
        qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}
*/

/**
 * @brief RB_SwapBuffers
 * @param[in] data
 * @return
 */
const void *RB_SwapBuffers(const void *data)
{
	const swapBuffersCommand_t *cmd;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	// texture swapping test
	if (r_showImages->integer)
	{
		RB_ShowImages();
	}

	cmd = ( const swapBuffersCommand_t * ) data;

	if (!glState.finishCalled)
	{
		qglFinish();
	}

	Ren_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	ri.GLimp_SwapFrame();

	backEnd.projection2D = qfalse;

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_RenderToTexture
 * @param[in] data
 * @return
 */
const void *RB_RenderToTexture(const void *data)
{
	const renderToTextureCommand_t *cmd = ( const renderToTextureCommand_t * ) data;

	//ri.Printf( PRINT_ALL, "RB_RenderToTexture\n" );

	GL_Bind(cmd->image);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
	qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cmd->x, cmd->y, cmd->w, cmd->h, 0);
	//qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cmd->x, cmd->y, cmd->w, cmd->h );

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_Finish
 * @param[in] data
 * @return
 */
const void *RB_Finish(const void *data)
{
	const renderFinishCommand_t *cmd = ( const renderFinishCommand_t * ) data;

	//ri.Printf( PRINT_ALL, "RB_Finish\n" );

	qglFinish();

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_ExecuteRenderCommands
 * @param[in] data
 */
void RB_ExecuteRenderCommands(const void *data)
{
	int t1, t2;

	t1 = ri.Milliseconds();

	while (1)
	{
		switch (*( const int * ) data)
		{
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic(data);
			break;
		case RC_2DPOLYS:
			data = RB_Draw2dPolys(data);
			break;
		case RC_ROTATED_PIC:
			data = RB_RotatedPic(data);
			break;
		case RC_STRETCH_PIC_GRADIENT:
			data = RB_StretchPicGradient(data);
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs(data);
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer(data);
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd(data);
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd(data);
			break;
		case RC_RENDERTOTEXTURE:
			data = RB_RenderToTexture(data);
			break;
		case RC_FINISH:
			data = RB_Finish(data);
			break;
		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			t2              = ri.Milliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}
