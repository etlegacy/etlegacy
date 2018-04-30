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
 * @file renderer2/tr_vbo.c
 */

#include "tr_local.h"

/**
 * @brief R_CreateVBO
 * @param[in] name
 * @param[in] vertexes
 * @param[in] vertexesSize
 * @param[in] usage
 * @return
 */
VBO_t *R_CreateVBO(const char *name, byte *vertexes, int vertexesSize, vboUsage_t usage)
{
	VBO_t *vbo;
	int   glUsage;

	switch (usage)
	{
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW;
		break;
	default:
		glUsage = 0; //Prevents warning
		Ren_Fatal("bad vboUsage_t given: %i", usage);
		//break;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreateVBO: \"%s\" is too long\n", name);
	}

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	vbo = (VBO_t *)ri.Hunk_Alloc(sizeof(*vbo), h_low);
	Com_AddToGrowList(&tr.vbos, vbo);

	Q_strncpyz(vbo->name, name, sizeof(vbo->name));

	vbo->ofsXYZ             = 0;
	vbo->ofsTexCoords       = 0;
	vbo->ofsLightCoords     = 0;
	vbo->ofsBinormals       = 0;
	vbo->ofsTangents        = 0;
	vbo->ofsNormals         = 0;
	vbo->ofsColors          = 0;
	//vbo->ofsPaintColors     = 0;
	//vbo->ofsLightDirections = 0;
	vbo->ofsBoneIndexes     = 0;
	vbo->ofsBoneWeights     = 0;

	vbo->sizeXYZ       = 0;
	vbo->sizeTangents  = 0;
	vbo->sizeBinormals = 0;
	vbo->sizeNormals   = 0;

	vbo->vertexesSize = vertexesSize;

	glGenBuffers(1, &vbo->vertexesVBO);

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexesSize, vertexes, glUsage);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GL_CheckErrors();

	return vbo;
}

/**
 * @brief R_CreateVBO2
 * @param[in] name
 * @param[in] numVertexes
 * @param[in] verts
 * @param[in] stateBits
 * @param[in] usage
 * @return
 */
VBO_t *R_CreateVBO2(const char *name, int numVertexes, srfVert_t *verts, unsigned int stateBits, vboUsage_t usage)
{
	VBO_t        *vbo;
	int          i, j;
	byte         *data;
	int          dataSize;
	int          dataOfs;
	int          glUsage;
	unsigned int bits;

	switch (usage)
	{
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW;
		break;
	default:
		glUsage = 0;
		Ren_Fatal("bad vboUsage_t given: %i", usage);
		//break;
	}

	if (!numVertexes)
	{
		return NULL;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreateVBO2: \"%s\" is too long\n", name);
	}

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	vbo = (VBO_t *)ri.Hunk_Alloc(sizeof(*vbo), h_low);
	Com_AddToGrowList(&tr.vbos, vbo);

	Q_strncpyz(vbo->name, name, sizeof(vbo->name));

	vbo->ofsXYZ             = 0;
	vbo->ofsTexCoords       = 0;
	vbo->ofsLightCoords     = 0;
	vbo->ofsBinormals       = 0;
	vbo->ofsTangents        = 0;
	vbo->ofsNormals         = 0;
	vbo->ofsColors          = 0;
	//vbo->ofsPaintColors     = 0;
	//vbo->ofsLightDirections = 0;
	vbo->ofsBoneIndexes     = 0;
	vbo->ofsBoneWeights     = 0;

	vbo->sizeXYZ       = 0;
	vbo->sizeTangents  = 0;
	vbo->sizeBinormals = 0;
	vbo->sizeNormals   = 0;

	// size VBO
	dataSize = 0;
	bits     = stateBits;
	while (bits)
	{
		if (bits & 1)
		{
			dataSize += sizeof(vec4_t);
		}
		bits >>= 1;
	}
	dataSize *= numVertexes;
	data      = (byte *)ri.Hunk_AllocateTempMemory(dataSize);
	dataOfs   = 0;

	// since this is all float, point tmp directly into data
	// 2-entry -> { memb[0], memb[1], 0, 1 }
	// 3-entry -> { memb[0], memb[1], memb[2], 1 }
#define VERTEXSIZE(memb) (sizeof(verts->memb) / sizeof(verts->memb[0]))
#define VERTEXCOPY(memb) \
	do { \
		vec_t *tmp = (vec_t *) (data + dataOfs); \
		for (i = 0; i < numVertexes; i++) \
		{ \
			for (j = 0; j < VERTEXSIZE(memb); j++) { *tmp++ = verts[i].memb[j]; } \
			if (VERTEXSIZE(memb) < 3) { *tmp++ = 0; } \
			if (VERTEXSIZE(memb) < 4) { *tmp++ = 1; } \
		} \
		dataOfs += i * sizeof(vec4_t); \
	} while (0)

	if (stateBits & ATTR_POSITION)
	{
		vbo->ofsXYZ = dataOfs;
		VERTEXCOPY(xyz);
	}

	// feed vertex texcoords
	if (stateBits & ATTR_TEXCOORD)
	{
		vbo->ofsTexCoords = dataOfs;
		VERTEXCOPY(st);
	}

	// feed vertex lightmap texcoords
	if (stateBits & ATTR_LIGHTCOORD)
	{
		vbo->ofsLightCoords = dataOfs;
		VERTEXCOPY(lightmap);
	}

	// feed vertex tangents
	if (stateBits & ATTR_TANGENT)
	{
		vbo->ofsTangents = dataOfs;
		VERTEXCOPY(tangent);
	}

	// feed vertex binormals
	if (stateBits & ATTR_BINORMAL)
	{
		vbo->ofsBinormals = dataOfs;
		VERTEXCOPY(binormal);
	}

	// feed vertex normals
	if (stateBits & ATTR_NORMAL)
	{
		vbo->ofsNormals = dataOfs;
		VERTEXCOPY(normal);
	}

	// feed vertex colors
	if (stateBits & ATTR_COLOR)
	{
		vbo->ofsColors = dataOfs;
		VERTEXCOPY(lightColor);
	}

	vbo->vertexesSize = dataSize;
	vbo->vertexesNum  = numVertexes;

	glGenBuffers(1, &vbo->vertexesVBO);

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);
	glBufferData(GL_ARRAY_BUFFER, dataSize, data, glUsage);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GL_CheckErrors();

	ri.Hunk_FreeTempMemory(data);

	return vbo;
}

/**
 * @brief R_CreateIBO
 * @param[in] name
 * @param[in] indexes
 * @param[in] indexesSize
 * @param[in] usage
 * @return
 */
IBO_t *R_CreateIBO(const char *name, byte *indexes, int indexesSize, vboUsage_t usage)
{
	IBO_t *ibo;
	int   glUsage;

	switch (usage)
	{
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW;
		break;
	default:
		glUsage = 0;
		Ren_Fatal("bad vboUsage_t given: %i", usage);
		//break;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreateIBO: \"%s\" is too long\n", name);
	}

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	ibo = (IBO_t *)ri.Hunk_Alloc(sizeof(*ibo), h_low);
	Com_AddToGrowList(&tr.ibos, ibo);

	Q_strncpyz(ibo->name, name, sizeof(ibo->name));

	ibo->indexesSize = indexesSize;

	glGenBuffers(1, &ibo->indexesVBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, glUsage);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GL_CheckErrors();

	return ibo;
}

/**
 * @brief R_CreateIBO2
 * @param[in] name
 * @param[in] numTriangles
 * @param[in] triangles
 * @param[in] usage
 * @return
 */
IBO_t *R_CreateIBO2(const char *name, int numTriangles, srfTriangle_t *triangles, vboUsage_t usage)
{
	IBO_t         *ibo;
	int           i, j;
	byte          *indexes;
	int           indexesSize;
	int           indexesOfs;
	srfTriangle_t *tri;
	glIndex_t     index;
	int           glUsage;

	switch (usage)
	{
	case VBO_USAGE_STATIC:
		glUsage = GL_STATIC_DRAW;
		break;
	case VBO_USAGE_DYNAMIC:
		glUsage = GL_DYNAMIC_DRAW;
		break;
	default:
		glUsage = 0;
		Ren_Fatal("bad vboUsage_t given: %i", usage);
		//break;
	}

	if (!numTriangles)
	{
		return NULL;
	}

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("R_CreateIBO2: \"%s\" is too long\n", name);
	}

	// make sure the render thread is stopped
	R_IssuePendingRenderCommands();

	ibo = (IBO_t *)ri.Hunk_Alloc(sizeof(*ibo), h_low);
	Com_AddToGrowList(&tr.ibos, ibo);

	Q_strncpyz(ibo->name, name, sizeof(ibo->name));

	indexesSize = numTriangles * 3 * sizeof(glIndex_t);
	indexes     = (byte *)ri.Hunk_AllocateTempMemory(indexesSize);
	indexesOfs  = 0;

	//Ren_Print("sizeof(glIndex_t) = %i\n", sizeof(glIndex_t));

	for (i = 0, tri = triangles; i < numTriangles; i++, tri++)
	{
		for (j = 0; j < 3; j++)
		{
			index = tri->indexes[j];
			Com_Memcpy(indexes + indexesOfs, &index, sizeof(glIndex_t));
			indexesOfs += sizeof(glIndex_t);
		}
	}

	ibo->indexesSize = indexesSize;
	ibo->indexesNum  = numTriangles * 3;

	glGenBuffers(1, &ibo->indexesVBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, glUsage);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GL_CheckErrors();

	ri.Hunk_FreeTempMemory(indexes);

	return ibo;
}

/**
 * @brief R_BindVBO
 * @param[in] vbo
 */
void R_BindVBO(VBO_t *vbo)
{
	if (!vbo)
	{
		//R_BindNullVBO();
		Ren_Drop("R_BindNullVBO: NULL vbo");
		//return;
	}

	Ren_LogComment("--- R_BindVBO( %s ) ---\n", vbo->name);

	if (glState.currentVBO != vbo)
	{
		glState.currentVBO              = vbo;
		glState.vertexAttribPointersSet = 0;

		glState.vertexAttribsInterpolation = 0;
		glState.vertexAttribsOldFrame      = 0;
		glState.vertexAttribsNewFrame      = 0;

		glBindBuffer(GL_ARRAY_BUFFER, vbo->vertexesVBO);

		backEnd.pc.c_vboVertexBuffers++;

		//GL_VertexAttribPointers(ATTR_BITS);
	}
}

/**
 * @brief R_BindNullVBO
 */
void R_BindNullVBO(void)
{
	Ren_LogComment("--- R_BindNullVBO ---\n");

	if (glState.currentVBO)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glState.currentVBO = NULL;
	}

	GL_CheckErrors();
}

/**
 * @brief R_BindIBO
 * @param[in] ibo
 */
void R_BindIBO(IBO_t *ibo)
{
	if (!ibo)
	{
		//R_BindNullIBO();
		Ren_Drop("R_BindIBO: NULL ibo");
		//return;
	}

	Ren_LogComment("--- R_BindIBO( %s ) ---\n", ibo->name);

	if (glState.currentIBO != ibo)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO);

		glState.currentIBO = ibo;

		backEnd.pc.c_vboIndexBuffers++;
	}
}

/**
 * @brief R_BindNullIBO
 */
void R_BindNullIBO(void)
{
	Ren_LogComment("--- R_BindNullIBO ---\n");

	if (glState.currentIBO)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glState.currentIBO              = NULL;
		glState.vertexAttribPointersSet = 0;
	}
}

/**
 * @brief R_InitUnitCubeVBO
 */
static void R_InitUnitCubeVBO()
{
	vec3_t        mins = { -1, -1, -1 };
	vec3_t        maxs = { 1, 1, 1 };
	int           i;
	srfVert_t     *verts;
	srfTriangle_t *triangles;

	if (glConfig.smpActive)
	{
		Ren_Fatal("R_InitUnitCubeVBO: FIXME SMP");
	}

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;

	Tess_AddCube(vec3_origin, mins, maxs, colorWhite);

	verts     = (srfVert_t *)ri.Hunk_AllocateTempMemory(tess.numVertexes * sizeof(srfVert_t));
	triangles = (srfTriangle_t *)ri.Hunk_AllocateTempMemory((tess.numIndexes / 3) * sizeof(srfTriangle_t));

	for (i = 0; i < tess.numVertexes; i++)
	{
		VectorCopy(tess.xyz[i], verts[i].xyz);
	}

	for (i = 0; i < (tess.numIndexes / 3); i++)
	{
		triangles[i].indexes[0] = tess.indexes[i * 3 + 0];
		triangles[i].indexes[1] = tess.indexes[i * 3 + 1];
		triangles[i].indexes[2] = tess.indexes[i * 3 + 2];
	}

	tr.unitCubeVBO = R_CreateVBO2("unitCube_VBO", tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC);
	tr.unitCubeIBO = R_CreateIBO2("unitCube_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC);

	ri.Hunk_FreeTempMemory(triangles);
	ri.Hunk_FreeTempMemory(verts);

	tess.multiDrawPrimitives = 0;
	tess.numIndexes          = 0;
	tess.numVertexes         = 0;
}

/**
 * @brief R_InitVBOs
 */
void R_InitVBOs(void)
{
	int  dataSize;
	byte *data;

	Ren_Print("------- R_InitVBOs -------\n");

	Com_InitGrowList(&tr.vbos, 16384);
	Com_InitGrowList(&tr.ibos, 2048);

	dataSize = sizeof(vec4_t) * SHADER_MAX_VERTEXES * 11;
	data     = (byte *)Com_Allocate(dataSize);
	Com_Memset(data, 0, dataSize);

	tess.vbo                 = R_CreateVBO("tessVertexArray_VBO", data, dataSize, VBO_USAGE_DYNAMIC);
	tess.vbo->ofsXYZ         = 0;
	tess.vbo->ofsTexCoords   = tess.vbo->ofsXYZ + sizeof(tess.xyz);
	tess.vbo->ofsLightCoords = tess.vbo->ofsTexCoords + sizeof(tess.texCoords);
	tess.vbo->ofsTangents    = tess.vbo->ofsLightCoords + sizeof(tess.lightCoords);
	tess.vbo->ofsBinormals   = tess.vbo->ofsTangents + sizeof(tess.tangents);
	tess.vbo->ofsNormals     = tess.vbo->ofsBinormals + sizeof(tess.binormals);
	tess.vbo->ofsColors      = tess.vbo->ofsNormals + sizeof(tess.normals);
	tess.vbo->sizeXYZ        = sizeof(tess.xyz);
	tess.vbo->sizeTangents   = sizeof(tess.tangents);
	tess.vbo->sizeBinormals  = sizeof(tess.binormals);
	tess.vbo->sizeNormals    = sizeof(tess.normals);

	Com_Dealloc(data);

	dataSize = sizeof(tess.indexes);
	data     = (byte *)Com_Allocate(dataSize);
	Com_Memset(data, 0, dataSize);

	tess.ibo = R_CreateIBO("tessVertexArray_IBO", data, dataSize, VBO_USAGE_DYNAMIC);

	Com_Dealloc(data);

	R_InitUnitCubeVBO();

	R_BindNullVBO();
	R_BindNullIBO();

	GL_CheckErrors();
}

/**
 * @brief R_ShutdownVBOs
 */
void R_ShutdownVBOs(void)
{
	int   i;
	VBO_t *vbo;
	IBO_t *ibo;

	Ren_Print("------- R_ShutdownVBOs -------\n");

	R_BindNullVBO();
	R_BindNullIBO();

	for (i = 0; i < tr.vbos.currentElements; i++)
	{
		vbo = (VBO_t *) Com_GrowListElement(&tr.vbos, i);

		if (vbo->vertexesVBO)
		{
			glDeleteBuffers(1, &vbo->vertexesVBO);
		}
	}

	for (i = 0; i < tr.ibos.currentElements; i++)
	{
		ibo = (IBO_t *) Com_GrowListElement(&tr.ibos, i);

		if (ibo->indexesVBO)
		{
			glDeleteBuffers(1, &ibo->indexesVBO);
		}
	}

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)

	if (tr.world)
	{
		int j;

		for (j = 0; j < MAX_VISCOUNTS; j++)
		{
			// FIXME: clean up this code
			for (i = 0; i < tr.world->clusterVBOSurfaces[j].currentElements; i++)
			{
				srfVBOMesh_t *vboSurf;

				vboSurf = (srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[j], i);
				ibo     = vboSurf->ibo;

				if (ibo->indexesVBO)
				{
					glDeleteBuffers(1, &ibo->indexesVBO);
				}
			}

			Com_DestroyGrowList(&tr.world->clusterVBOSurfaces[j]);
		}
	}

#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

	Com_DestroyGrowList(&tr.vbos);
	Com_DestroyGrowList(&tr.ibos);
}

/**
 * @brief R_VBOList_f
 */
void R_VBOList_f(void)
{
	int   i;
	VBO_t *vbo;
	IBO_t *ibo;
	int   vertexesSize = 0;
	int   indexesSize  = 0;

	Ren_Print(" size          name\n");
	Ren_Print("----------------------------------------------------------\n");

	for (i = 0; i < tr.vbos.currentElements; i++)
	{
		vbo = (VBO_t *) Com_GrowListElement(&tr.vbos, i);

		Ren_Print("%d.%02d MB %s\n", vbo->vertexesSize / (1024 * 1024),
		          (vbo->vertexesSize % (1024 * 1024)) * 100 / (1024 * 1024), vbo->name);

		vertexesSize += vbo->vertexesSize;
	}

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)

	if (tr.world)
	{
		int j;

		for (j = 0; j < MAX_VISCOUNTS; j++)
		{
			// FIXME: clean up this code
			for (i = 0; i < tr.world->clusterVBOSurfaces[j].currentElements; i++)
			{
				srfVBOMesh_t *vboSurf;

				vboSurf = (srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[j], i);
				ibo     = vboSurf->ibo;

				Ren_Print("%d.%02d MB %s\n", ibo->indexesSize / (1024 * 1024),
				          (ibo->indexesSize % (1024 * 1024)) * 100 / (1024 * 1024), ibo->name);

				indexesSize += ibo->indexesSize;
			}
		}
	}

#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

	for (i = 0; i < tr.ibos.currentElements; i++)
	{
		ibo = (IBO_t *) Com_GrowListElement(&tr.ibos, i);

		Ren_Print("%d.%02d MB %s\n", ibo->indexesSize / (1024 * 1024),
		          (ibo->indexesSize % (1024 * 1024)) * 100 / (1024 * 1024), ibo->name);

		indexesSize += ibo->indexesSize;
	}

	Ren_Print(" %i total VBOs\n", tr.vbos.currentElements);
	Ren_Print(" %d.%02d MB total vertices memory\n", vertexesSize / (1024 * 1024),
	          (vertexesSize % (1024 * 1024)) * 100 / (1024 * 1024));

	Ren_Print(" %i total IBOs\n", tr.ibos.currentElements);
	Ren_Print(" %d.%02d MB total triangle indices memory\n", indexesSize / (1024 * 1024),
	          (indexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
}
