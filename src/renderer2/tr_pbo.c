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
 * @file renderer2/tr_pbo.c
 *
 *  Pixel Buffer Objects
 */

#include "tr_local.h"


PBO_t* R_CreatePBO(const char* name, pboUsage_t usage, int bufferSize)
{
	PBO_t *pbo;
	int bufTarget = GL_PIXEL_PACK_BUFFER;
	pboUsage_t bufUsage = GL_STREAM_READ;

	switch (usage)
	{
	case PBO_USAGE_WRITE:
		bufTarget = GL_PIXEL_UNPACK_BUFFER;
		bufUsage = GL_STREAM_DRAW;
		break;
	case PBO_USAGE_READ:
	default:
		bufTarget = GL_PIXEL_PACK_BUFFER;
		bufUsage = GL_STREAM_COPY; //GL_STREAM_READ; // GL_STREAM_COPY
		break;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreatePBO: \"%s\" is too long\n", name);
		return NULL;
	}

	// make sure the render thread is stopped
//	R_IssuePendingRenderCommands();

	pbo = (PBO_t *)ri.Hunk_Alloc(sizeof(*pbo), h_low);
	Com_AddToGrowList(&tr.pbos, pbo);

	Q_strncpyz(pbo->name, name, sizeof(pbo->name));
	pbo->target = bufTarget;
	pbo->usage = bufUsage;
	pbo->bufferSize = bufferSize;

	glGenBuffers(1, &pbo->handle);
	glBindBuffer(pbo->target, pbo->handle);
	glBufferData(pbo->target, pbo->bufferSize, NULL, pbo->usage);

	pbo->texture = R_AllocImage("pbotexture", qfalse); // perhaps add an index number in the name: pbotexture0

	glBindBuffer(pbo->target, 0);

	GL_CheckErrors();

	return pbo;
}


void R_BindPBO(PBO_t *pbo)
{
	if (!pbo)
	{
		Ren_Drop("R_BindPBO: NULL pbo");
		return;
	}

	glBindBuffer(pbo->target, pbo->handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pbo->texture->texnum);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0); // address offset
	pbo->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); // glFenceSync is only supported if the GL version is 3.2 or greater.

	GL_CheckErrors();
}


void R_BindNullPBO(void)
{
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	GL_CheckErrors();
}


void R_InitPBOs(void)
{
	Com_InitGrowList(&tr.pbos, 100);

	R_BindNullPBO();

	GL_CheckErrors();
}


void R_ShutdownPBOs(void)
{
	int i;
	PBO_t *pbo;

	R_BindNullPBO();

	for (i = 0; i < tr.pbos.currentElements; i++)
	{
		pbo = (PBO_t *) Com_GrowListElement(&tr.pbos, i);

		glDeleteBuffers(1, &pbo->handle);
	}

	GL_CheckErrors();

	Com_DestroyGrowList(&tr.pbos);
}


qboolean R_PBOResultAvailable(PBO_t *pbo)
{
	GLint result;

	if (!pbo)
	{
		Ren_Drop("R_PBOResultAvailable: NULL pbo");
		return qfalse;
	}
	
	if (!pbo->sync)
	{
		return qfalse;
	}
	
	glGetSynciv(pbo->sync, GL_SYNC_STATUS, sizeof(result), NULL, &result);

	GL_CheckErrors();

	return (result == GL_SIGNALED);
}


qboolean R_ReadPBO(PBO_t *pbo, byte *cpumemory, qboolean waitForResult)
{
	GLint result = GL_UNSIGNALED;

	if (!pbo)
	{
		Ren_Drop("R_ReadPBO: NULL pbo");
		return qfalse;
	}
	
	if (pbo->usage != PBO_USAGE_READ || !pbo->sync)
	{
		return qfalse;
	}

	if (waitForResult)
	{
		// while (!R_PBOResultAvailable(pbo));
		while (result != GL_SIGNALED)
		{
			glGetSynciv(pbo->sync, GL_SYNC_STATUS, sizeof(result), NULL, &result);
		}
	}
	// if you don't waitForResult, you must be sure the result is available: R_PBOResultAvailable(pbo) == true

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->handle);
    void* mappedBuffer = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	Com_Memcpy(cpumemory, mappedBuffer, pbo->bufferSize);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	pbo->sync = 0;

	GL_CheckErrors();

	return qtrue;
}
