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
 * @file renderer2/tr_cmds.c
 */

#include "tr_local.h"

/**
 * @brief R_PerformanceCounters
 */
void R_PerformanceCounters(void)
{
	if (!r_speeds->integer)
	{
		// clear the counters even if we aren't printing
		Com_Memset(&tr.pc, 0, sizeof(tr.pc));
		Com_Memset(&backEnd.pc, 0, sizeof(backEnd.pc));
		return;
	}

	switch (r_speeds->integer)
	{
	case RSPEEDS_GENERAL:
		Ren_Print("%i views %i portals %i batches %i surfs %i leafs %i verts %i tris\n",
		          backEnd.pc.c_views, backEnd.pc.c_portals, backEnd.pc.c_batches, backEnd.pc.c_surfaces, tr.pc.c_leafs,
		          backEnd.pc.c_vertexes, backEnd.pc.c_indexes / 3);

		Ren_Print("%i lights %i bout %i pvsout %i queryout %i interactions\n",
		          tr.pc.c_dlights + tr.pc.c_slights - backEnd.pc.c_occlusionQueriesLightsCulled,
		          tr.pc.c_box_cull_light_out,
		          tr.pc.c_pvs_cull_light_out,
		          backEnd.pc.c_occlusionQueriesLightsCulled,
		          tr.pc.c_dlightInteractions + tr.pc.c_slightInteractions - backEnd.pc.c_occlusionQueriesInteractionsCulled);

		Ren_Print("%i draws %i queries %i CHC++ ms %i vbos %i ibos %i verts %i tris\n",
		          backEnd.pc.c_drawElements,
		          tr.pc.c_occlusionQueries,
		          tr.pc.c_CHCTime,
		          backEnd.pc.c_vboVertexBuffers, backEnd.pc.c_vboIndexBuffers,
		          backEnd.pc.c_vboVertexes, backEnd.pc.c_vboIndexes / 3);

		Ren_Print("%i multidraws %i primitives %i tris\n",
		          backEnd.pc.c_multiDrawElements,
		          backEnd.pc.c_multiDrawPrimitives,
		          backEnd.pc.c_multiVboIndexes / 3);
		break;
	case RSPEEDS_CULLING:
		Ren_Print("(gen) %i sin %i sout %i pin %i pout\n",
		          tr.pc.c_sphere_cull_in, tr.pc.c_sphere_cull_out, tr.pc.c_plane_cull_in, tr.pc.c_plane_cull_out);

		Ren_Print("(patch) %i sin %i sclip %i sout %i bin %i bclip %i bout\n",
		          tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip,
		          tr.pc.c_sphere_cull_patch_out, tr.pc.c_box_cull_patch_in,
		          tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out);

		Ren_Print("(mdx) %i sin %i sclip %i sout %i bin %i bclip %i bout\n",
		          tr.pc.c_sphere_cull_mdx_in, tr.pc.c_sphere_cull_mdx_clip,
		          tr.pc.c_sphere_cull_mdx_out, tr.pc.c_box_cull_mdx_in, tr.pc.c_box_cull_mdx_clip, tr.pc.c_box_cull_mdx_out);

		Ren_Print("(md5) %i bin %i bclip %i bout\n",
		          tr.pc.c_box_cull_md5_in, tr.pc.c_box_cull_md5_clip, tr.pc.c_box_cull_md5_out);
		break;
	case RSPEEDS_VIEWCLUSTER:
		Ren_Print("viewcluster: %i\n", tr.visClusters[tr.visIndex]);
		break;
	case RSPEEDS_LIGHTS:
		Ren_Print("dlight srf:%i culled:%i\n", tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled);

		Ren_Print("dlights:%i interactions:%i\n", tr.pc.c_dlights, tr.pc.c_dlightInteractions);

		Ren_Print("slights:%i interactions:%i\n", tr.pc.c_slights, tr.pc.c_slightInteractions);
		break;
	case RSPEEDS_SHADOWCUBE_CULLING:
		Ren_Print("omni pyramid tests:%i bin:%i bclip:%i bout:%i\n",
		          tr.pc.c_pyramidTests, tr.pc.c_pyramid_cull_ent_in, tr.pc.c_pyramid_cull_ent_clip, tr.pc.c_pyramid_cull_ent_out);
		break;
	case RSPEEDS_FOG:
		Ren_Print("fog srf:%i batches:%i\n", backEnd.pc.c_fogSurfaces, backEnd.pc.c_fogBatches);
		break;
	case RSPEEDS_FLARES:
		Ren_Print("flare adds:%i tests:%i renders:%i\n",
		          backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders);
		break;
	case RSPEEDS_OCCLUSION_QUERIES:
		Ren_Print("occlusion queries:%i multi:%i saved:%i culled lights:%i culled entities:%i culled leafs:%i response time:%i fetch time:%i\n",
		          backEnd.pc.c_occlusionQueries,
		          backEnd.pc.c_occlusionQueriesMulti,
		          backEnd.pc.c_occlusionQueriesSaved,
		          backEnd.pc.c_occlusionQueriesLightsCulled,
		          backEnd.pc.c_occlusionQueriesEntitiesCulled,
		          backEnd.pc.c_occlusionQueriesLeafsCulled,
		          backEnd.pc.c_occlusionQueriesResponseTime,
		          backEnd.pc.c_occlusionQueriesFetchTime);
		break;
#if 0
	case RSPEEDS_DEPTH_BOUNDS_TESTS:
		Ren_Print("depth bounds tests:%i rejected:%i\n", tr.pc.c_depthBoundsTests, tr.pc.c_depthBoundsTestsRejected);
		break;
#endif
	case RSPEEDS_SHADING_TIMES:
		Ren_Print("forward shading times: ambient:%i lighting:%i\n", backEnd.pc.c_forwardAmbientTime,
		          backEnd.pc.c_forwardLightingTime);
		break;
	case RSPEEDS_CHC:
		Ren_Print("%i CHC++ ms %i queries %i multi queries %i saved\n",
		          tr.pc.c_CHCTime,
		          tr.pc.c_occlusionQueries,
		          tr.pc.c_occlusionQueriesMulti,
		          tr.pc.c_occlusionQueriesSaved);
		break;
	case RSPEEDS_NEAR_FAR:
		Ren_Print("zNear: %.0f zFar: %.0f\n", tr.viewParms.zNear, tr.viewParms.zFar);
		break;
	case RSPEEDS_DECALS:
		Ren_Print("decal projectors: %d test surfs: %d clip surfs: %d decal surfs: %d created: %d\n",
		          tr.pc.c_decalProjectors, tr.pc.c_decalTestSurfaces, tr.pc.c_decalClipSurfaces, tr.pc.c_decalSurfaces,
		          tr.pc.c_decalSurfacesCreated);
		break;
	}

	Com_Memset(&tr.pc, 0, sizeof(tr.pc));
	Com_Memset(&backEnd.pc, 0, sizeof(backEnd.pc));
}

// FIXME: Unused ?
// int c_blockedOnRender;
// int c_blockedOnMain;

/**
 * @brief R_IssueRenderCommands
 * @param[in] runPerformanceCounters
 */
void R_IssueRenderCommands(qboolean runPerformanceCounters)
{
	renderCommandList_t *cmdList = &backEndData->commands;

	etl_assert(cmdList != NULL);
	// add an end-of-list command
	*(int *)(cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if (runPerformanceCounters)
	{
		R_PerformanceCounters();
	}

	// actually start the commands going
	if (!r_skipBackEnd->integer)
	{
		// let it start on the new batch
		RB_ExecuteRenderCommands(cmdList->cmds);
	}
}

/**
 * @brief Issue any pending commands
 */
void R_IssuePendingRenderCommands(void)
{
	if (!tr.registered)
	{
		return;
	}
	R_IssueRenderCommands(qfalse);
}

/**
 * @brief Make sure there is enough command space, waiting on the
 * render thread if needed.
 * @param[in] bytes
 * @return
 */
void *R_GetCommandBuffer(unsigned int bytes)
{
	renderCommandList_t *cmdList = &backEndData->commands;

	// always leave room for the swap buffers and end of list commands
	// - added swapBuffers_t from ET
	if (cmdList->used + bytes + (sizeof(swapBuffersCommand_t) + sizeof(int)) > MAX_RENDER_COMMANDS)
	{
		if (bytes > MAX_RENDER_COMMANDS - (sizeof(swapBuffersCommand_t) + sizeof(int)))
		{
			Ren_Fatal("R_GetCommandBuffer: bad size %u", bytes);
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

/**
 * @brief R_AddDrawViewCmd
 */
void R_AddDrawViewCmd()
{
	drawViewCommand_t *cmd;

	cmd = (drawViewCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_DRAW_VIEW;

	cmd->refdef    = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/**
 * @brief Passing NULL will set the color to white
 * @param[in] rgba
 */
void RE_SetColor(const float *rgba)
{
	setColorCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}
	cmd = (setColorCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	if (!rgba)
	{
		static float colorWhite[4] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[0] = rgba[0];
	cmd->color[1] = rgba[1];
	cmd->color[2] = rgba[2];
	cmd->color[3] = rgba[3];
}

/**
 * @brief R_ClipRegion
 * @param[in,out] x
 * @param[in,out] y
 * @param[in,out] w
 * @param[in,out] h
 * @param[in,out] s1
 * @param[in,out] t1
 * @param[in,out] s2
 * @param[in,out] t2
 * @return
 */
static qboolean R_ClipRegion(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2)
{
	float left, top, right, bottom;
	float _s1, _t1, _s2, _t2;
	float clipLeft, clipTop, clipRight, clipBottom;

	if (tr.clipRegion[2] <= tr.clipRegion[0] ||
	    tr.clipRegion[3] <= tr.clipRegion[1])
	{
		return qfalse;
	}

	left   = *x;
	top    = *y;
	right  = *x + *w;
	bottom = *y + *h;

	_s1 = *s1;
	_t1 = *t1;
	_s2 = *s2;
	_t2 = *t2;

	clipLeft   = tr.clipRegion[0];
	clipTop    = tr.clipRegion[1];
	clipRight  = tr.clipRegion[2];
	clipBottom = tr.clipRegion[3];

	// Completely clipped away
	if (right <= clipLeft || left >= clipRight ||
	    bottom <= clipTop || top >= clipBottom)
	{
		return qtrue;
	}

	// Clip left edge
	if (left < clipLeft)
	{
		float f = (clipLeft - left) / (right - left);
		*s1 = (f * (_s2 - _s1)) + _s1;
		*x  = clipLeft;
		*w -= (clipLeft - left);
	}

	// Clip right edge
	if (right > clipRight)
	{
		float f = (clipRight - right) / (left - right);
		*s2 = (f * (_s1 - _s2)) + _s2;
		*w  = clipRight - *x;
	}

	// Clip top edge
	if (top < clipTop)
	{
		float f = (clipTop - top) / (bottom - top);
		*t1 = (f * (_t2 - _t1)) + _t1;
		*y  = clipTop;
		*h -= (clipTop - top);
	}

	// Clip bottom edge
	if (bottom > clipBottom)
	{
		float f = (clipBottom - bottom) / (top - bottom);
		*t2 = (f * (_t1 - _t2)) + _t2;
		*h  = clipBottom - *y;
	}

	return qfalse;
}

/**
 * @brief RE_SetClipRegion
 * @param[in] region
 */
void RE_SetClipRegion(const float *region)
{
	if (region == NULL)
	{
		Com_Memset(tr.clipRegion, 0, sizeof(vec4_t));
	}
	else
	{
		Vector4Copy(region, tr.clipRegion);
	}
}

/**
 * @brief RE_StretchPic
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 */
void RE_StretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader)
{
	stretchPicCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}

	if (R_ClipRegion(&x, &y, &w, &h, &s1, &t1, &s2, &t2))
	{
		return;
	}

	cmd = (stretchPicCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader    = R_GetShaderByHandle(hShader);
	cmd->x         = x;
	cmd->y         = y;
	cmd->w         = w;
	cmd->h         = h;
	cmd->s1        = s1;
	cmd->t1        = t1;
	cmd->s2        = s2;
	cmd->t2        = t2;
}

extern int r_numPolyVerts;

/**
 * @brief RE_2DPolyies
 * @param[in] verts
 * @param[in] numverts
 * @param[in] hShader
 */
void RE_2DPolyies(polyVert_t *verts, int numverts, qhandle_t hShader)
{
	poly2dCommand_t *cmd;

	if (r_numPolyVerts + numverts > r_maxPolyVerts->integer)
	{
		return;
	}

	cmd = (poly2dCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_2DPOLYS;
	cmd->verts     = &backEndData->polyVerts[r_numPolyVerts];
	cmd->numverts  = numverts;
	Com_Memcpy(cmd->verts, verts, sizeof(polyVert_t) * numverts);
	cmd->shader = R_GetShaderByHandle(hShader);

	r_numPolyVerts += numverts;
}

/**
 * @brief RE_RotatedPic
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 * @param[in] angle
 */
void RE_RotatedPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle)
{
	stretchPicCommand_t *cmd;

	cmd = (stretchPicCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_ROTATED_PIC;
	cmd->shader    = R_GetShaderByHandle(hShader);
	cmd->x         = x;
	cmd->y         = y;
	cmd->w         = w;
	cmd->h         = h;

	cmd->angle = angle;
	cmd->s1    = s1;
	cmd->t1    = t1;
	cmd->s2    = s2;
	cmd->t2    = t2;
}

/**
 * @brief RE_StretchPicGradient
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] s1
 * @param[in] t1
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 * @param[in] gradientColor
 * @param[in] gradientType
 */
void RE_StretchPicGradient(float x, float y, float w, float h,
                           float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor,
                           int gradientType)
{
	stretchPicCommand_t *cmd;

	cmd = (stretchPicCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_STRETCH_PIC_GRADIENT;
	cmd->shader    = R_GetShaderByHandle(hShader);
	cmd->x         = x;
	cmd->y         = y;
	cmd->w         = w;
	cmd->h         = h;
	cmd->s1        = s1;
	cmd->t1        = t1;
	cmd->s2        = s2;
	cmd->t2        = t2;

	if (!gradientColor)
	{
		static float colorWhite[4] = { 1, 1, 1, 1 };

		gradientColor = colorWhite;
	}

	cmd->gradientColor[0] = (byte)(gradientColor[0] * 255);
	cmd->gradientColor[1] = (byte)(gradientColor[1] * 255);
	cmd->gradientColor[2] = (byte)(gradientColor[2] * 255);
	cmd->gradientColor[3] = (byte)(gradientColor[3] * 255);
	cmd->gradientType     = gradientType;
}

/**
 * @brief Will be called once for each RE_EndFrame
 */
void RE_BeginFrame()
{
	drawBufferCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}

	Ren_LogComment("--- RE_BeginFrame ---\n");

	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;
	tr.viewCount     = 0;

	// do overdraw measurement
	if (r_measureOverdraw->integer)
	{
		if (glConfig.stencilBits < 4)
		{
			Ren_Print("Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits);
			ri.Cvar_Set("r_measureOverdraw", "0");
		}
		else if (r_shadows->integer == 2)
		{
			Ren_Print("Warning: stencil shadows and overdraw measurement are mutually exclusive\n");
			ri.Cvar_Set("r_measureOverdraw", "0");
		}
		else
		{
			R_IssuePendingRenderCommands();
			glEnable(GL_STENCIL_TEST);
			glStencilMask(~0U);
			GL_ClearStencil(0U);
			glStencilFunc(GL_ALWAYS, 0U, ~0U);
			glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if (r_measureOverdraw->modified)
		{
			R_IssuePendingRenderCommands();
			glDisable(GL_STENCIL_TEST);
		}
		r_measureOverdraw->modified = qfalse;
	}

	// texturemode stuff
	if (r_textureMode->modified)
	{
		R_IssuePendingRenderCommands();
		GL_TextureMode(r_textureMode->string);
		r_textureMode->modified = qfalse;
	}

	// gamma stuff
	if (r_gamma->modified)
	{
		r_gamma->modified = qfalse;
		R_IssuePendingRenderCommands();
		R_SetColorMappings();
	}

	// check for errors
	if (!r_ignoreGLErrors->integer)
	{
		int  err;
		char s[128];

		R_IssuePendingRenderCommands();

		if ((err = glGetError()) != GL_NO_ERROR)
		{
			switch (err)
			{
			case GL_INVALID_ENUM:
				Q_strcpy(s, "GL_INVALID_ENUM");
				break;
			case GL_INVALID_VALUE:
				Q_strcpy(s, "GL_INVALID_VALUE");
				break;
			case GL_INVALID_OPERATION:
				Q_strcpy(s, "GL_INVALID_OPERATION");
				break;
			case GL_STACK_OVERFLOW:
				Q_strcpy(s, "GL_STACK_OVERFLOW");
				break;
			case GL_STACK_UNDERFLOW:
				Q_strcpy(s, "GL_STACK_UNDERFLOW");
				break;
			case GL_OUT_OF_MEMORY:
				Q_strcpy(s, "GL_OUT_OF_MEMORY");
				break;
			case GL_TABLE_TOO_LARGE:
				Q_strcpy(s, "GL_TABLE_TOO_LARGE");
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
				Q_strcpy(s, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT");
				break;
			default:
				Com_sprintf(s, sizeof(s), "0x%X", err);
				break;
			}

			//Ren_Fatal( "caught OpenGL error: %s in file %s line %i", s, filename, line);
			Ren_Fatal("RE_BeginFrame() - glGetError() failed (%s)!\n", s);
		}
	}

	// draw buffer stuff
	cmd = (drawBufferCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_DRAW_BUFFER;

	if (!Q_stricmp(r_drawBuffer->string, "GL_FRONT"))
	{
		cmd->buffer = (int)GL_FRONT;
	}
	else
	{
		cmd->buffer = (int)GL_BACK;
	}
}

/**
 * @brief Returns the number of msec spent in the back end
 * @param[out] frontEndMsec
 * @param[out] backEndMsec
 */
void RE_EndFrame(int *frontEndMsec, int *backEndMsec)
{
	swapBuffersCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}

	cmd = (swapBuffersCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_SWAP_BUFFERS;

	R_BindNullVBO();
	R_BindNullIBO();

	R_IssueRenderCommands(qtrue);

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_InitNextFrame();

	if (frontEndMsec)
	{
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if (backEndMsec)
	{
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}

/**
 * @brief RE_TakeVideoFrame
 * @param[in] width
 * @param[in] height
 * @param[in] captureBuffer
 * @param[in] encodeBuffer
 * @param[in] motionJpeg
 */
void RE_TakeVideoFrame(int width, int height, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg)
{
	videoFrameCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}

	cmd = (videoFrameCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_VIDEOFRAME;

	cmd->width         = width;
	cmd->height        = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer  = encodeBuffer;
	cmd->motionJpeg    = motionJpeg;
}

/**
 * @brief RE_RenderToTexture
 * @param[in] textureid
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 */
void RE_RenderToTexture(int textureid, int x, int y, int w, int h)
{
	renderToTextureCommand_t *cmd;

	// note: see also Com_GrowListElement checking against tr.images->currentElements
	if (textureid > tr.numImages || textureid < 0)
	{
		Ren_Print("Warning: trap_R_RenderToTexture textureid %d out of range.\n", textureid);
		return;
	}

	cmd = (renderToTextureCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_RENDERTOTEXTURE;
	cmd->image     = (image_t *) Com_GrowListElement(&tr.images, textureid);
	cmd->x         = x;
	cmd->y         = y;
	cmd->w         = w;
	cmd->h         = h;
}

/**
 * @brief RE_Finish
 */
void RE_Finish(void)
{
	renderFinishCommand_t *cmd;

	Ren_Print("RE_Finish\n");

	cmd = (renderFinishCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_FINISH;
}
