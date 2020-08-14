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

/*
 *   PBO results are retrieved from another thread.
 *   A linked list is maintained of all the PBOs that need their results collected.
 */

 // the array to store the entries
#define MAX_PBO_RESULTS 24576                          // that's room for 4096 cubes
pboDownload_Cubemap_t pboDownloads[MAX_PBO_RESULTS];

// the linked lists:
// These lists have references to both previous & next entries (for easy entry deletion).
pboDownload_Cubemap_t *pboList_Cubemaps = NULL;        // the used entries
pboDownload_Cubemap_t *pboList_Available = NULL;       // the unused entries

// we want to know if there are objects, arrays and/or lists created that can be deleted/freed
qboolean pboDispose = qfalse;


/*
 *   Initialize the linked lists to manage outstanding pixel transfers
 */
void R_PBOInitDownloads(void)
{
	int i,j;
	pboList_Cubemaps = NULL;
	pboList_Available = NULL;
	for (i = 0; i < MAX_PBO_RESULTS; i++)
	{
		for (j = 0; j < 6; j++)
		{
			/*if (pboDownloads[i].cubeTemp[j])
			{
				ri.Free(pboDownloads[i].cubeTemp[j]);
			}*/
			pboDownloads[i].cubeTemp[j] = NULL;
		}

		pboDownloads[i].prev = (i==0)? NULL : &pboDownloads[i-1];
		pboDownloads[i].next = pboList_Available;
		pboList_Available = &pboDownloads[i];
	}
}


/*
 *   Add a probe to the list to handle the pbo results.
 *   The function will handle all 6 pbos (for all the sides of the cube).
 *   This function also allocates the memory needed to store the result's pixeldata (read from gpu).
 */
void R_PBOAddDownload(cubemapProbe_t *probe)
{
	pboDownload_Cubemap_t *download;
	int i;

	if (!pboList_Available)
	{
		return; // none available
	}

	// exit if it already exists in the list
	for (download = pboList_Cubemaps; download; download = download->next)
	{
		if (download->probe == probe)
		{
			return;
		}
	}

	// link an entry
	download                          = pboList_Available;
	if (pboList_Available->next)
	{
		pboList_Available->next->prev = pboList_Available->prev;
	}
	pboList_Available                 = pboList_Available->next;
	download->prev                    = (pboList_Cubemaps)? pboList_Cubemaps->prev : NULL;
	download->next                    = pboList_Cubemaps;
	if (pboList_Cubemaps)
	{
		pboList_Cubemaps->prev        = download;
	}
	pboList_Cubemaps                  = download;

	// ..
	download->probe = probe;
	download->results = (1<<5) | (1<<4) | (1<<3) | (1<<2) | (1<<1) | (1<<0); // a bit for every side of the cube

	// reserve memory for the pixeldata
	for (i = 0; i < 6; i++)
	{
		download->cubeTemp[i] = (byte *)ri.Z_Malloc(REF_CUBEMAP_TEXTURE_SIZE);
	}

	pboDispose = qtrue; // set the flag
}


/*
 *   Remove an entry from the list.
 *   This function also frees the memory that was needed to download the pixeldata from gpu.
 */
void R_PBORemoveDownload(pboDownload_Cubemap_t *download)
{
	int i;

	if (!download)
	{
		return;
	}

	// link an entry
	if (download->prev)
	{
		download->prev->next          = download->next;
	}
	if (download->next)
	{
		download->next->prev          = download->prev;
	}
	if (download == pboList_Cubemaps)
	{
		pboList_Cubemaps              = download->next;
	}
	download->prev                    = (pboList_Available)? pboList_Available->prev : NULL;
	download->next                    = pboList_Available;
	if (pboList_Available)
	{
		pboList_Available->prev       = download;
	}
	pboList_Available                 = download;

	// free the allocated memory
	for (i = 0; i < 6; i++)
	{
		if (download->cubeTemp[i])
		{
			ri.Free(download->cubeTemp[i]);
			download->cubeTemp[i] = NULL;
		}
	}
}


/*
 *   Check all outstanding pbo transfers.
 *   Check if results are ready, and if so, read the pixeldata associated with the pbo.
 *   Create the cubemap, if pixeldata of all the 6 sides of the cubemap have been downloaded.
 *   Save the cubemap to file.
 *   Clean up memory for objects, array and lists that isn't used anymore and can be disposed.
 */
void R_PBOCheckDownloads(void)
{
	pboDownload_Cubemap_t *download;
	cubemapProbe_t *probe;
	PBO_t *pbo;
	int i;

	// if there is no PBO currently syncing, then clear objects, arrays and lists,
	// if that is not already done.
	if (!pboList_Cubemaps && pboDispose)
	{
		R_ShutdownPBOs();
		return;
	}

	// check all probes for which we need to handle pixel transfers
	for (download = pboList_Cubemaps; download; download = download->next)
	{
		probe = download->probe;
		if (!probe) continue; // better return, because when that happens, it's bad..
		for (i = 0; i < 6; i++)
		{
			pbo = probe->pbo[i];
			if (!pbo || !pbo->sync) continue;
			if (!R_PBOResultAvailable(pbo)) continue;
			R_ReadPBO(pbo, download->cubeTemp[i], qfalse);
			download->results &= ~(1<<i); // clear bit i. When all 6 results are downloaded, ->results value == 0

			// if results of all 6 sides are downloaded, make the cubemap..
			if (!download->results)
			{
				// make the cubemap texture
				probe->cubemap = R_CreateCubeImage(va("_cubeProbe%d", probe->index), (const byte **)download->cubeTemp, REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE, IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP);
				probe->ready = qtrue; // this cube is ready for rendering

				// save the cubemap to file
				R_SaveCubeProbe(probe, download->cubeTemp, qfalse); // qfalse means: save only this one

				// the entry can now be deleted
				R_PBORemoveDownload(download);
				// and we are done for this download. Check the next download..
				goto R_PBOCheckDownloads_next; // a continue will only work on the for(i) loop
			}
		}
R_PBOCheckDownloads_next:
		; // this must be here..
	}
}

//---------------------------------------------------------------------------------------------


PBO_t* R_CreatePBO(pboUsage_t usage, int width, int height)
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
		bufUsage = GL_STREAM_COPY; //GL_STREAM_READ;
		break;
	}

	// make sure the render thread is stopped
//	R_IssuePendingRenderCommands();

	pbo = (PBO_t *)ri.Hunk_Alloc(sizeof(*pbo), h_low);
	Com_AddToGrowList(&tr.pbos, pbo);

	pbo->width = width;
	pbo->height = height;
	pbo->target = bufTarget;
	pbo->usage = bufUsage;
	pbo->bufferSize = width * height * 4; // RGBA

	glGenBuffers(1, &pbo->handle);
	glBindBuffer(pbo->target, pbo->handle);
	glBufferData(pbo->target, pbo->bufferSize, NULL, pbo->usage); // *data is NULL, so no data is copied, only the memory allocated

	if (usage == PBO_USAGE_READ)
	{
		pbo->texture = R_AllocImage("pbotexture", qfalse); // perhaps add an index number in the name: pbotexture0
	}

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

	R_PBOInitDownloads();

	R_BindNullPBO();

	pboDispose = qfalse; // reset the flag

	GL_CheckErrors();
}


void R_ShutdownPBOs(void)
{
	int i;
	pboDownload_Cubemap_t *download;
	PBO_t *pbo;

	R_BindNullPBO();

	// free the memory for the pbo pixeldata storage (for active pbos)
	for (download = pboList_Cubemaps; download; download = download->next)
	{
		R_PBORemoveDownload(download);
	}

	// delete the pbos
	for (i = 0; i < tr.pbos.currentElements; i++)
	{
		pbo = (PBO_t *) Com_GrowListElement(&tr.pbos, i);

		glDeleteBuffers(1, &pbo->handle);
	}

	// delete the growlist with all the pointers to the pbos
	Com_DestroyGrowList(&tr.pbos);

	pboDispose = qfalse; // reset the flag

	GL_CheckErrors();
}


// sync read
void R_SyncPBO(PBO_t *pbo)
{
	if (!pbo)
	{
		Ren_Drop("R_BindPBO: NULL pbo");
		return;
	}

	// if there is no more free result entry available from the list, then don't start a new sync
	if (!pboList_Available)
	{
		return;
	}

	glBindBuffer(pbo->target, pbo->handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pbo->texture->texnum);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0); // address offset
	pbo->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0); // glFenceSync is only supported if the GL version is 3.2 or greater.

	GL_CheckErrors();

	if (!pbo->sync)
	{
		Ren_Drop("R_BindPBO: pbo sync error");
		return;
	}
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
	void *mappedBuffer;

	if (!pbo)
	{
		Ren_Drop("R_ReadPBO: NULL pbo");
		return qfalse;
	}

	// it must be a read buffer, with an active sync
	if (pbo->usage != GL_STREAM_COPY || !pbo->sync)
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
/*	else {
		// if you don't waitForResult, you must be sure the result is available: R_PBOResultAvailable(pbo) == true
		// if (!R_PBOResultAvailable(pbo)) return qfalse;
		glGetSynciv(pbo->sync, GL_SYNC_STATUS, sizeof(result), NULL, &result);
		if (result != GL_SIGNALED) return qfalse;
	}*/

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->handle);                   // glBindBuffer(pbo->target, pbo->handle);
	mappedBuffer = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);    // mappedBuffer = glMapBuffer(pbo->target, GL_READ_ONLY);
	if (mappedBuffer)
	{
		Com_Memcpy(cpumemory, mappedBuffer, pbo->bufferSize);          //
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);                           // glUnmapBuffer(pbo->target);
	}

	GL_CheckErrors();

	pbo->sync = 0;

	return qtrue;
}


// glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
qboolean R_pboTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
	void *mappedBuffer;
	PBO_t *pbo = R_CreatePBO(PBO_USAGE_WRITE, width, height);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo->handle);
glBufferData(GL_PIXEL_UNPACK_BUFFER, pbo->bufferSize, 0, GL_STREAM_DRAW);
	mappedBuffer = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (mappedBuffer)
	{
		Com_Memcpy(mappedBuffer, pixels, pbo->bufferSize);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}
	glTexImage2D(target, level, internalformat, width, height, border, format, type, 0); // address 0
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	GL_CheckErrors();

	return qtrue;
}
