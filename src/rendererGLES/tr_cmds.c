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
 * @file rendererGLES/tr_cmds.c
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

	if (r_speeds->integer)
	{
		Ren_Print("%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
		          backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes,
		          backEnd.pc.c_indexes / 3, backEnd.pc.c_totalIndexes / 3,
		          R_SumOfUsedImages() / (1000000.0), (double)backEnd.pc.c_overDraw / (double)(glConfig.vidWidth * glConfig.vidHeight));
	}

	if (r_speeds->integer == 2)
	{
		Ren_Print("(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
		          tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out,
		          tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out);
		Ren_Print("(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
		          tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out,
		          tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out);
		Ren_Print("(gen) %i sin %i sout %i pin %i pout\n",
		          tr.pc.c_sphere_cull_in, tr.pc.c_sphere_cull_out,
		          tr.pc.c_plane_cull_in, tr.pc.c_plane_cull_out);
	}
	else if (r_speeds->integer == 3)
	{
		Ren_Print("viewcluster: %i\n", tr.viewCluster);
	}
	else if (r_speeds->integer == 4)
	{
		Ren_Print("dlight srf:%i  culled:%i  verts:%i  tris:%i\n",
		          tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
		          backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3);
	}
	else if (r_speeds->integer == 6)
	{
		Ren_Print("flare adds:%i tests:%i renders:%i\n",
		          backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders);
	}
	else if (r_speeds->integer == 7)
	{
		Ren_Print("decal projectors: %d test surfs: %d clip surfs: %d decal surfs: %d created: %d\n",
		          tr.pc.c_decalProjectors, tr.pc.c_decalTestSurfaces, tr.pc.c_decalClipSurfaces, tr.pc.c_decalSurfaces, tr.pc.c_decalSurfacesCreated);
	}

	Com_Memset(&tr.pc, 0, sizeof(tr.pc));
	Com_Memset(&backEnd.pc, 0, sizeof(backEnd.pc));
}

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
 * @brief Issue any pending commands and wait for them to complete.
 * After exiting, the render thread will have completed its work
 * and will remain idle and the main thread is free to issue
 * OpenGL calls until R_IssueRenderCommands is called.
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
 *
 * @param[in] bytes
 * @return
 */
void *R_GetCommandBuffer(unsigned int bytes)
{
	renderCommandList_t *cmdList = &backEndData->commands;

	// always leave room for the swap buffers and end of list commands
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
 * @brief R_AddDrawSurfCmd
 * @param[in] drawSurfs
 * @param[in] numDrawSurfs
 */
void R_AddDrawSurfCmd(drawSurf_t *drawSurfs, int numDrawSurfs)
{
	drawSurfsCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs    = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

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

	cmd = R_GetCommandBuffer(sizeof(*cmd));
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
void RE_StretchPic(float x, float y, float w, float h,
                   float s1, float t1, float s2, float t2, qhandle_t hShader)
{
	stretchPicCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
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

extern int r_numpolyverts;

/**
 * @brief RE_2DPolyies
 * @param[in] verts
 * @param[in] numverts
 * @param[in] hShader
 */
void RE_2DPolyies(polyVert_t *verts, int numverts, qhandle_t hShader)
{
	poly2dCommand_t *cmd;

	if (r_numpolyverts + numverts >= r_maxPolyVerts->integer)
	{
		Ren_Print("Warning RE_2DPolyies: r_maxpolyverts reached\n");
		return;
	}

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}

	cmd->commandId = RC_2DPOLYS;
	cmd->verts     = &backEndData->polyVerts[r_numpolyverts];
	cmd->numverts  = numverts;
	Com_Memcpy(cmd->verts, verts, sizeof(polyVert_t) * numverts);
	cmd->shader = R_GetShaderByHandle(hShader);

	r_numpolyverts += numverts;
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
void RE_RotatedPic(float x, float y, float w, float h,
                   float s1, float t1, float s2, float t2, qhandle_t hShader, float angle)
{
	stretchPicCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
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

	// fixup
	cmd->w /= 2;
	cmd->h /= 2;
	cmd->x += cmd->w;
	cmd->y += cmd->h;
	cmd->w  = (float)sqrt((double)((cmd->w * cmd->w) + (cmd->h * cmd->h)));
	cmd->h  = cmd->w;

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
                           float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType)
{
	stretchPicCommand_t *cmd;

	cmd = R_GetCommandBuffer(sizeof(*cmd));
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
 * @brief RE_SetGlobalFog
 * @param[in] restore flag can be used to restore the original level fog
 * @param[in] duration can be set to fade over a certain period
 * @param[in] r colour
 * @param[in] g colour
 * @param[in] b colour
 * @param[in] depthForOpaque is depth for opaque
 */
void RE_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque)
{
	if (restore)
	{
		if (duration > 0)
		{
			VectorCopy(tr.world->fogs[tr.world->globalFog].shader->fogParms.color, tr.world->globalTransStartFog);
			tr.world->globalTransStartFog[3] = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;

			Vector4Copy(tr.world->globalOriginalFog, tr.world->globalTransEndFog);

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime   = tr.refdef.time + duration;
		}
		else
		{
			VectorCopy(tr.world->globalOriginalFog, tr.world->fogs[tr.world->globalFog].shader->fogParms.color);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4(tr.world->globalOriginalFog[0] * tr.identityLight,
			                                                                            tr.world->globalOriginalFog[1] * tr.identityLight,
			                                                                            tr.world->globalOriginalFog[2] * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = tr.world->globalOriginalFog[3];
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale        = 1.0f / (tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque);
		}
	}
	else
	{
		if (duration > 0)
		{
			VectorCopy(tr.world->fogs[tr.world->globalFog].shader->fogParms.color, tr.world->globalTransStartFog);
			tr.world->globalTransStartFog[3] = tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque;

			VectorSet(tr.world->globalTransEndFog, r, g, b);
			tr.world->globalTransEndFog[3] = depthForOpaque < 1 ? 1 : depthForOpaque;

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime   = tr.refdef.time + duration;
		}
		else
		{
			VectorSet(tr.world->fogs[tr.world->globalFog].shader->fogParms.color, r, g, b);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.colorInt = ColorBytes4(r * tr.identityLight,
			                                                                            g * tr.identityLight,
			                                                                            b * tr.identityLight, 1.0);
			tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque = depthForOpaque < 1 ? 1 : depthForOpaque;
			tr.world->fogs[tr.world->globalFog].shader->fogParms.tcScale        = 1.0f / (tr.world->fogs[tr.world->globalFog].shader->fogParms.depthForOpaque);
		}
	}
}

/**
 * @brief Will be called once for each RE_EndFrame
 */
void RE_BeginFrame(void)
{
	drawBufferCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}
	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

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
		unsigned int err;

		R_IssuePendingRenderCommands();
		if ((err = qglGetError()) != GL_NO_ERROR)
		{
			Ren_Fatal("RE_BeginFrame() - glGetError() failed (0x%x)!\n", err);
		}
	}

	// draw buffer stuff
	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;

	cmd->buffer = (int)GL_BACK;
}

/**
 * @brief Returns the number of msec spent in the back end
 * @param[out] frontEndMsec
 * @param[out] backEndMsec
 */
void RE_EndFrame(int *frontEndMsec, int *backEndMsec)
{
	renderCommandList_t *cmdList;

	if (!tr.registered)
	{
		return;
	}

	// Needs to use reserved space, so no R_GetCommandBuffer.
	cmdList = &backEndData->commands;
	etl_assert(cmdList != NULL);
	// add swap-buffers command
	*( int * )(cmdList->cmds + cmdList->used) = RC_SWAP_BUFFERS;
	cmdList->used                            += sizeof(swapBuffersCommand_t);

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

	//ri.Printf( PRINT_ALL, "RE_RenderToTexture\n" );

	if (textureid > tr.numImages || textureid < 0)
	{
		Ren_Print("Warning: trap_R_RenderToTexture textureid %d out of range.\n", textureid);
		return;
	}

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_RENDERTOTEXTURE;
	cmd->image     = tr.images[textureid];
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

	cmd = R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd)
	{
		return;
	}
	cmd->commandId = RC_FINISH;
}

/**
 * @brief RE_TakeVideoFrame
 * @param[in] width
 * @param[in] height
 * @param[in] captureBuffer
 * @param[in] encodeBuffer
 * @param[in] motionJpeg
 */
void RE_TakeVideoFrame(int width, int height,
                       byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg)
{
	videoFrameCommand_t *cmd;

	if (!tr.registered)
	{
		return;
	}

	cmd = R_GetCommandBuffer(sizeof(*cmd));
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
