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
 * @file renderer/tr_backend.c
 */

#include "tr_local.h"
#include "vulkan/vk.h"

backEndData_t  *backEndData;
backEndState_t backEnd;


/// last image given to GL_Bind(), used by the immediate-mode emulation
/// below (renderer1 debug code binds, then draws with glBegin/glEnd)
static image_t *vkImmBoundImage;

/**
 * @brief GL_Bind - Vulkan draws take their image explicitly; this only
 * marks the image as used this frame and remembers it for the
 * immediate-mode emulation.
 * @param[in] image
 */
void GL_Bind(image_t *image)
{
	if (!image)
	{
		Ren_Warning("GL_Bind: NULL image\n");
		image = tr.defaultImage;
	}

	if (r_noBind->integer && tr.dlightImage)            // performance evaluation option
	{
		image = tr.dlightImage;
	}

	image->frameUsed = tr.frameCount;
	vkImmBoundImage  = image;
}

/**
 * @brief GL_State - records the renderer1 state bits. Vulkan pipelines are
 * selected per draw from explicit state bits; the recorded value is used
 * by the immediate-mode emulation below.
 * @param[in] stateBits
 */
void GL_State(unsigned long stateBits)
{
	glState.glStateBits = stateBits;
}

/**
 * @brief A player has predicted a teleport, but hasn't arrived yet
 */
static void RB_Hyperspace(void)
{
	float c = (backEnd.refdef.time & 255) / 255.0f;

	VK_SetClearColor(c, c, c, 1);
	VK_ClearSceneColor(qfalse);

	backEnd.isHyperspace = qtrue;
}

/**
 * @brief SetViewportAndScissor
 */
static void SetViewportAndScissor(void)
{

	VK_SetProjectionMatrix(backEnd.viewParms.projectionMatrix);

	// set the window clipping

	// Must be set per-view, not cached per-frame: a frame may contain
	// multiple 3D views with different viewport rects (e.g. the limbo
	// menu's small player-model preview alongside the main world view).
	VK_SetSceneViewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
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

	// renderer1 makes the stencil clear an alternative to the
	// rest of this chain, so with r_shadows 2 depth is never cleared per
	// view. Vulkan clears stencil together with depth (VK_ClearSceneDepth)
	// whenever the attachment has stencil, so keep the depth logic intact.
	if (r_measureOverdraw->integer || r_shadows->integer == 2)
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	// global q3 fog volume
	if (tr.world && tr.world->globalFog >= 0)
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;
		clearBits |= GL_COLOR_BUFFER_BIT;
		//
		VK_SetClearColor(tr.world->fogs[tr.world->globalFog].shader->fogParms.color[0] * tr.identityLight,
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
					VK_SetClearColor(glfogsettings[FOG_PORTALVIEW].color[0], glfogsettings[FOG_PORTALVIEW].color[1], glfogsettings[FOG_PORTALVIEW].color[2], glfogsettings[FOG_PORTALVIEW].color[3]);
				}
				else if (glfogNum > FOG_NONE && glfogsettings[FOG_CURRENT].registered)
				{
					VK_SetClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);
				}
				else
				{
					VK_SetClearColor(0.5, 0.5, 0.5, 1.0);
				}
			}
			else                                                        // rendered sky (either clear color or draw quake sky)
			{
				if (glfogsettings[FOG_PORTALVIEW].registered)
				{
					VK_SetClearColor(glfogsettings[FOG_PORTALVIEW].color[0], glfogsettings[FOG_PORTALVIEW].color[1], glfogsettings[FOG_PORTALVIEW].color[2], glfogsettings[FOG_PORTALVIEW].color[3]);

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

				VK_SetClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);
			}
			else if (!(r_portalSky->integer)) // portal skies have been manually turned off, clear bg color
			{
				clearBits |= GL_COLOR_BUFFER_BIT;
				VK_SetClearColor(0.5, 0.5, 0.5, 1.0);
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
				VK_SetClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);
			}
			else
			{
				VK_SetClearColor(0.05f, 0.05f, 0.05f, 1.0f);    // JPW NERVE changed per id req was 0.5s
			}
		}
		else  // world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
		{
			if (glfogsettings[FOG_CURRENT].registered)     // try to clear fastsky with current fog color
			{
				VK_SetClearColor(glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3]);

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

	// depth is reset per view so a portal/mirror subview's depth doesn't
	// leak into the view drawn after it
	if (clearBits & GL_DEPTH_BUFFER_BIT)
	{
		VK_ClearSceneDepth();
	}

	// color clears (r_fastsky, fog clearscreen, portal sky fog, ...) use
	// the color last given to VK_SetClearColor() above
	if (clearBits & GL_COLOR_BUFFER_BIT)
	{
		VK_ClearSceneColor(qfalse);
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


	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if (backEnd.viewParms.isPortal)
	{
		float  plane[4];
		double plane2[4]; // keep this, glew expects double

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct(backEnd.viewParms.orientation.axis[0], plane);
		plane2[1] = DotProduct(backEnd.viewParms.orientation.axis[1], plane);
		plane2[2] = DotProduct(backEnd.viewParms.orientation.axis[2], plane);
		plane2[3] = DotProduct(plane, backEnd.viewParms.orientation.origin) - plane[3];


		// renderer1 hands plane2 to glClipPlane with
		// s_flipMatrix (see tr_main.c) loaded. Real GL's glClipPlane transforms the given plane
		// by the *current* modelview's inverse-transpose at call time -
		// which is s_flipMatrix here (just loaded), not the full world
		// modelview - converting plane2 from the engine's native
		// (X-forward) convention into GL's (-Z-forward) eye space, the
		// same space our fragEyePos varying is computed in (since
		// VK_SetModelViewMatrix() is fed backEnd.viewParms.world.modelMatrix,
		// which itself bakes in this same flip). s_flipMatrix is
		// orthogonal, so its inverse-transpose equals itself; apply it
		// directly to the normal (the distance term is unaffected by a
		// pure rotation of the normal). s_flipMatrix maps view-space
		// (x, y, z) to eye-space (-y, z, -x), so the plane's normal maps
		// the same way: (a, b, c) -> (-b, c, -a).
		{
			float planeEqf[4];

			planeEqf[0] = (float)-plane2[1];
			planeEqf[1] = (float)plane2[2];
			planeEqf[2] = (float)-plane2[0];
			planeEqf[3] = (float)plane2[3];
			VK_SetPortalClip(planeEqf);
		}
	}
	else
	{
		VK_ClearPortalClip();
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


			VK_SetModelViewMatrix(backEnd.orientation.modelMatrix);

			// change depthrange if needed
			if (oldDepthRange != depthRange)
			{
				if (depthRange)
				{
					VK_SetDepthRange(0, 0.3);
				}
				else
				{
					VK_SetDepthRange(0, 1);
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


	VK_SetModelViewMatrix(backEnd.viewParms.world.modelMatrix);

	if (depthRange)
	{
		VK_SetDepthRange(0, 1);
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

	GL_State(GLS_DEPTHTEST_DISABLE |
	         GLS_SRCBLEND_SRC_ALPHA |
	         GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);


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
	if (!tr.registered)
	{
		return;
	}
	R_IssuePendingRenderCommands();

	// we definitely want to sync every frame for the cinematics

	RE_UploadCinematic(0, 0, cols, rows, data, client, dirty);

	if (!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	// renderer1 draws this quad in immediate mode; submit it
	// through the Vulkan 2D path instead. Flush any batched 2D surface
	// first so draw order is kept.
	if (tess.numIndexes)
	{
		RB_EndSurface();
	}

	{
		const float s0 = 0.5f / cols, s1 = (cols - 0.5f) / cols;
		const float t0 = 0.5f / rows, t1 = (rows - 0.5f) / rows;
		const byte  c = (byte)tr.identityLightByte;
		int         i;

		tess.numVertexes = 4;
		tess.numIndexes  = 6;

		VectorSet(tess.xyz[0], x, y, 0);
		VectorSet(tess.xyz[1], x + w, y, 0);
		VectorSet(tess.xyz[2], x + w, y + h, 0);
		VectorSet(tess.xyz[3], x, y + h, 0);

		tess.svars.texcoords[0][0][0] = s0; tess.svars.texcoords[0][0][1] = t0;
		tess.svars.texcoords[0][1][0] = s1; tess.svars.texcoords[0][1][1] = t0;
		tess.svars.texcoords[0][2][0] = s1; tess.svars.texcoords[0][2][1] = t1;
		tess.svars.texcoords[0][3][0] = s0; tess.svars.texcoords[0][3][1] = t1;

		for (i = 0; i < 4; i++)
		{
			tess.svars.colors[i][0] = c;
			tess.svars.colors[i][1] = c;
			tess.svars.colors[i][2] = c;
			tess.svars.colors[i][3] = 255;
		}

		tess.indexes[0] = 0; tess.indexes[1] = 1; tess.indexes[2] = 2;
		tess.indexes[3] = 0; tess.indexes[4] = 2; tess.indexes[5] = 3;

		VK_Draw2D(&tess, tr.scratchImage[client], GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

		tess.numVertexes = 0;
		tess.numIndexes  = 0;
	}

	// Without binding a texture the cinematic will not be displayed correctly
	GL_Bind(tr.defaultImage);
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
	int     start;
	image_t *image;

	if (client < 0 || client >= (sizeof(tr.scratchImage) / sizeof(tr.scratchImage[0])))
	{
		Ren_Drop("RE_UploadCinematic: image offset out of range");
	}

	if (!tr.scratchImage[client])
	{
		tr.scratchImage[client] = R_CreateImage(va("*scratch%i", client), data, cols, rows, qfalse, qtrue, GL_CLAMP_TO_EDGE);
		dirty                   = qfalse;
	}

	// Also covers the first frame: R_CreateImage() went through Upload32(), which may
	// have resampled/picmipped, so recreate at the raw cinematic size.
	if (tr.scratchImage[client] &&
	    (dirty || tr.scratchImage[client]->uploadWidth != cols || tr.scratchImage[client]->uploadHeight != rows))
	{
		VK_UploadScratchImage(tr.scratchImage[client], data, cols, rows);
	}

	start = 0;
	if (r_speeds->integer)
	{
		start = ri.Milliseconds();
	}

	image = tr.scratchImage[client];

	GL_Bind(image);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if (cols != image->width || rows != image->height)
	{
		image->width  = image->uploadWidth = cols;
		image->height = image->uploadHeight = rows;
	}
	else if (dirty)
	{
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression
	}

	if (r_speeds->integer)
	{
		int end = ri.Milliseconds();

		Ren_Print("glTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
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

	angle                 = cmd->angle * M_TAU_F;
	tess.xyz[numVerts][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts][2] = 0;

	tess.texCoords[numVerts][0][0] = cmd->s1;
	tess.texCoords[numVerts][0][1] = cmd->t1;

	angle                     = cmd->angle * M_TAU_F + 0.25 * M_TAU_F;
	tess.xyz[numVerts + 1][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts + 1][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts + 1][2] = 0;

	tess.texCoords[numVerts + 1][0][0] = cmd->s2;
	tess.texCoords[numVerts + 1][0][1] = cmd->t1;

	angle                     = cmd->angle * M_TAU_F + 0.50 * M_TAU_F;
	tess.xyz[numVerts + 2][0] = cmd->x + (cos(angle) * cmd->w);
	tess.xyz[numVerts + 2][1] = cmd->y + (sin(angle) * cmd->h);
	tess.xyz[numVerts + 2][2] = 0;

	tess.texCoords[numVerts + 2][0][0] = cmd->s2;
	tess.texCoords[numVerts + 2][0][1] = cmd->t2;

	angle                     = cmd->angle * M_TAU_F + 0.75 * M_TAU_F;
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
	if (r_clear->integer && tr.frameSceneNum != 0)
	{
		VK_SetClearColor(1, 0, 0.5f, 1);
		VK_ClearSceneColor(qtrue);
	}

	return ( const void * ) (cmd + 1);
}

/**
 * @brief RB_GammaScreen
 */
void RB_GammaScreen(void)
{
	// We force the 2D drawing
	RB_SetGL2D();

	// r_gamma and the overbright scale for VK_EndFrame()'s gamma pass
	// (renderer1's R_ScreenGamma()); neutral when the pass doesn't apply
	// them (r_ignorehwgamma, see R_Init())
	if (tr.gammaInShader)
	{
		VK_SetGammaParams(r_gamma->value, (float)(1 << tr.overbrightBits));
	}
	else
	{
		VK_SetGammaParams(1.0f, 1.0f);
	}
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

	VK_ClearSceneColor(qtrue);


	start = ri.Milliseconds();

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

		GL_Bind(image);
		VKimm_Begin(GL_QUADS);
		VKimm_TexCoord2f(0, 0);
		VKimm_Vertex2f(x, y);
		VKimm_TexCoord2f(1, 0);
		VKimm_Vertex2f(x + w, y);
		VKimm_TexCoord2f(1, 1);
		VKimm_Vertex2f(x + w, y + h);
		VKimm_TexCoord2f(0, 1);
		VKimm_Vertex2f(x, y + h);
		VKimm_End();
	}


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

    // box corners
    VKimm_Begin(GL_LINES);
    VKimm_Color3f(1, 1, 1);

    VKimm_Vertex3f(mins[0], mins[1], mins[2]);
    VKimm_Vertex3f(maxs[0], mins[1], mins[2]);
    VKimm_Vertex3f(mins[0], mins[1], mins[2]);
    VKimm_Vertex3f(mins[0], maxs[1], mins[2]);
    VKimm_Vertex3f(mins[0], mins[1], mins[2]);
    VKimm_Vertex3f(mins[0], mins[1], maxs[2]);

    VKimm_Vertex3f(maxs[0], maxs[1], maxs[2]);
    VKimm_Vertex3f(mins[0], maxs[1], maxs[2]);
    VKimm_Vertex3f(maxs[0], maxs[1], maxs[2]);
    VKimm_Vertex3f(maxs[0], mins[1], maxs[2]);
    VKimm_Vertex3f(maxs[0], maxs[1], maxs[2]);
    VKimm_Vertex3f(maxs[0], maxs[1], mins[2]);
    VKimm_End();

    center[0] = (mins[0] + maxs[0]) * 0.5f;
    center[1] = (mins[1] + maxs[1]) * 0.5f;
    center[2] = (mins[2] + maxs[2]) * 0.5f;

    // center axis
    VKimm_Begin(GL_LINES);
    VKimm_Color3f(1, 0.85f, 0);

    VKimm_Vertex3f(mins[0], center[1], center[2]);
    VKimm_Vertex3f(maxs[0], center[1], center[2]);
    VKimm_Vertex3f(center[0], mins[1], center[2]);
    VKimm_Vertex3f(center[0], maxs[1], center[2]);
    VKimm_Vertex3f(center[0], center[1], mins[2]);
    VKimm_Vertex3f(center[0], center[1], maxs[2]);
    VKimm_End();
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

	RB_GammaScreen();

	cmd = ( const swapBuffersCommand_t * ) data;

	Ren_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	// End the render pass, submit the recorded command buffer and present
	// the swapchain image (counterpart to VK_BeginFrame() called at the top
	// of RE_BeginFrame()).
	VK_EndFrame();

	// Screenshots / levelshot / avi frame of the frame just
	// captured by VK_EndFrame()
	RB_FinishFrameCaptures();

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
	//glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cmd->x, cmd->y, cmd->w, cmd->h );

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
		data = PADP(data, sizeof(intptr_t));
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

/*
 * Immediate-mode emulation (declared in tr_local.h).
 *
 * Vertices are collected with the current color/texcoord between glBegin
 * and glEnd; glEnd converts the GL primitive into a triangle or line list
 * and draws it with the current GL_State() bits and GL_Bind() image -
 * through VK_Draw2D() in 2D mode, VK_DrawImmediate3D() otherwise.
 * GL_POINTS become small crosses (Vulkan point size needs shader support).
 */

#define VKIMM_MAX_VERTS   4096
#define VKIMM_MAX_INDEXES (VKIMM_MAX_VERTS * 6)

static struct
{
	int mode;
	qboolean active;
	int numVerts;
	vec4_t xyz[VKIMM_MAX_VERTS];
	float st[VKIMM_MAX_VERTS][2];
	byte color[VKIMM_MAX_VERTS][4];
	float curColor[4];
	float curST[2];
	float pointSize;
	unsigned int indexes[VKIMM_MAX_INDEXES];
} vkImm = { 0, qfalse, 0, { { 0 } }, { { 0 } }, { { 0 } }, { 1, 1, 1, 1 }, { 0, 0 }, 1.0f, { 0 } };

void VKimm_Color4f(float r, float g, float b, float a)
{
	vkImm.curColor[0] = r;
	vkImm.curColor[1] = g;
	vkImm.curColor[2] = b;
	vkImm.curColor[3] = a;
}

void VKimm_TexCoord2f(float s, float t)
{
	vkImm.curST[0] = s;
	vkImm.curST[1] = t;
}

void VKimm_PointSize(float size)
{
	vkImm.pointSize = size;
}

void VKimm_Begin(int mode)
{
	vkImm.mode     = mode;
	vkImm.active   = qtrue;
	vkImm.numVerts = 0;
}

static void VKimm_AddVertex(const float *xyz, const float *st, const float *color)
{
	int i = vkImm.numVerts;

	if (i >= VKIMM_MAX_VERTS)
	{
		return;
	}

	VectorCopy(xyz, vkImm.xyz[i]);
	vkImm.xyz[i][3]   = 1.0f;
	vkImm.st[i][0]    = st[0];
	vkImm.st[i][1]    = st[1];
	vkImm.color[i][0] = (byte)(Com_Clamp(0.0f, 1.0f, color[0]) * 255.0f);
	vkImm.color[i][1] = (byte)(Com_Clamp(0.0f, 1.0f, color[1]) * 255.0f);
	vkImm.color[i][2] = (byte)(Com_Clamp(0.0f, 1.0f, color[2]) * 255.0f);
	vkImm.color[i][3] = (byte)(Com_Clamp(0.0f, 1.0f, color[3]) * 255.0f);
	vkImm.numVerts++;
}

void VKimm_Vertex3f(float x, float y, float z)
{
	vec3_t v;

	if (!vkImm.active)
	{
		return;
	}

	VectorSet(v, x, y, z);
	VKimm_AddVertex(v, vkImm.curST, vkImm.curColor);
}

void VKimm_End(void)
{
	int      n = vkImm.numVerts, numIndexes = 0, i;
	qboolean lines;

	if (!vkImm.active)
	{
		return;
	}
	vkImm.active = qfalse;

	switch (vkImm.mode)
	{
	case GL_POINTS:
	{
		// each point -> a 3-axis cross (6 extra vertices)
		float half = vkImm.pointSize * 0.5f;
		int   base = n;

		for (i = 0; i < base && vkImm.numVerts + 6 <= VKIMM_MAX_VERTS; i++)
		{
			int   axis;
			float color[4];

			color[0] = vkImm.color[i][0] / 255.0f;
			color[1] = vkImm.color[i][1] / 255.0f;
			color[2] = vkImm.color[i][2] / 255.0f;
			color[3] = vkImm.color[i][3] / 255.0f;

			for (axis = 0; axis < 3; axis++)
			{
				vec3_t a, b;

				VectorCopy(vkImm.xyz[i], a);
				VectorCopy(vkImm.xyz[i], b);
				a[axis]                    -= half;
				b[axis]                    += half;
				vkImm.indexes[numIndexes++] = vkImm.numVerts;
				VKimm_AddVertex(a, vkImm.st[i], color);
				vkImm.indexes[numIndexes++] = vkImm.numVerts;
				VKimm_AddVertex(b, vkImm.st[i], color);
			}
		}
		lines = qtrue;
		break;
	}
	case GL_LINES:
		for (i = 0; i + 1 < n; i += 2)
		{
			vkImm.indexes[numIndexes++] = i;
			vkImm.indexes[numIndexes++] = i + 1;
		}
		lines = qtrue;
		break;
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		for (i = 0; i + 1 < n; i++)
		{
			vkImm.indexes[numIndexes++] = i;
			vkImm.indexes[numIndexes++] = i + 1;
		}
		if (vkImm.mode == GL_LINE_LOOP && n > 2)
		{
			vkImm.indexes[numIndexes++] = n - 1;
			vkImm.indexes[numIndexes++] = 0;
		}
		lines = qtrue;
		break;
	case GL_TRIANGLES:
		for (i = 0; i + 2 < n; i += 3)
		{
			vkImm.indexes[numIndexes++] = i;
			vkImm.indexes[numIndexes++] = i + 1;
			vkImm.indexes[numIndexes++] = i + 2;
		}
		lines = qfalse;
		break;
	case GL_TRIANGLE_STRIP:
	case GL_QUAD_STRIP:
		for (i = 0; i + 2 < n; i++)
		{
			// keep GL's alternating winding
			vkImm.indexes[numIndexes++] = (i & 1) ? i + 1 : i;
			vkImm.indexes[numIndexes++] = (i & 1) ? i : i + 1;
			vkImm.indexes[numIndexes++] = i + 2;
		}
		lines = qfalse;
		break;
	case GL_QUADS:
		for (i = 0; i + 3 < n; i += 4)
		{
			vkImm.indexes[numIndexes++] = i;
			vkImm.indexes[numIndexes++] = i + 1;
			vkImm.indexes[numIndexes++] = i + 2;
			vkImm.indexes[numIndexes++] = i;
			vkImm.indexes[numIndexes++] = i + 2;
			vkImm.indexes[numIndexes++] = i + 3;
		}
		lines = qfalse;
		break;
	case GL_TRIANGLE_FAN:
	case GL_POLYGON:
	default:
		for (i = 1; i + 1 < n; i++)
		{
			vkImm.indexes[numIndexes++] = 0;
			vkImm.indexes[numIndexes++] = i;
			vkImm.indexes[numIndexes++] = i + 1;
		}
		lines = qfalse;
		break;
	}

	if (numIndexes == 0)
	{
		return;
	}

	if (backEnd.projection2D)
	{
		// 2D (e.g. r_showImages): triangles only, through the UI path
		if (lines)
		{
			return;
		}

		if (tess.numIndexes)
		{
			RB_EndSurface();
		}

		for (i = 0; i < vkImm.numVerts; i++)
		{
			VectorCopy(vkImm.xyz[i], tess.xyz[i]);
			tess.svars.texcoords[0][i][0] = vkImm.st[i][0];
			tess.svars.texcoords[0][i][1] = vkImm.st[i][1];
			tess.svars.colors[i][0]       = vkImm.color[i][0];
			tess.svars.colors[i][1]       = vkImm.color[i][1];
			tess.svars.colors[i][2]       = vkImm.color[i][2];
			tess.svars.colors[i][3]       = vkImm.color[i][3];
		}
		for (i = 0; i < numIndexes; i++)
		{
			tess.indexes[i] = vkImm.indexes[i];
		}
		tess.numVertexes = vkImm.numVerts;
		tess.numIndexes  = numIndexes;

		VK_Draw2D(&tess, vkImmBoundImage, glState.glStateBits);

		tess.numVertexes = 0;
		tess.numIndexes  = 0;
		return;
	}

	VK_DrawImmediate3D((const float (*)[4]) vkImm.xyz, (const byte(*)[4]) vkImm.color, (const float (*)[2]) vkImm.st, vkImm.numVerts,
	                   vkImm.indexes, numIndexes, lines, vkImmBoundImage, glState.glStateBits);
}
