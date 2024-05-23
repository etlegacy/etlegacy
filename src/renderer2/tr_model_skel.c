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
 * @file renderer2/tr_model_skel.c
 * @brief Model loading and caching
 */

#include "tr_local.h"

/*
 * @brief CompareBoneIndices
 * @param[in] a
 * @param[in] b
 * @return
 *
 * @note Unused
static int CompareBoneIndices(const void *a, const void *b)
{
        return *(const int *)a - *(const int *)b;
}
*/

/*
 * @brief CompareTrianglesByBoneReferences
 * @param[in] a
 * @param[in] b
 * @return
 *
 * @note Unused
static int CompareTrianglesByBoneReferences(const void *a, const void *b)
{
        int             i, j;

        skelTriangle_t *t1, *t2;
        md5Vertex_t    *v1, *v2;
        int       b1[MAX_BONES], b2[MAX_BONES];
        //int       s1, s2;

        t1 = (skelTriangle_t *) *(void **)a;
        t2 = (skelTriangle_t *) *(void **)b;

#if 1
        for(i = 0; i < MAX_BONES; i++)
        {
                b1[i] = b2[i] = 0;
        }

        for(i = 0; i < 3; i++)
        {
                v1 = t1->vertexes[i];
                v2 = t1->vertexes[i];

                for(j = 0; j < MAX_WEIGHTS; j++)
                {
                        if(j < v1->numWeights)
                        {
                                b1[v1->weights[j]->boneIndex]++;
                        }

                        if(j < v2->numWeights)
                        {
                                b1[v2->weights[j]->boneIndex]++;
                        }
                }
        }

        qsort(b1, MAX_WEIGHTS * 3, sizeof(int), CompareBoneIndices);
        qsort(b2, MAX_WEIGHTS * 3, sizeof(int), CompareBoneIndices);

        for(j = 0; j < MAX_BONES; j++)
        {
                if(b1[j] < b2[j])
                        return -1;

                if(b1[j] > b2[j])
                        return 1;
        }
#else

        // calculate the bone sums
        s1 = s2 = 0;
        for(i = 0; i < 3; i++)
        {
                v1 = t1->vertexes[i];
                v2 = t1->vertexes[i];

                for(j = 0; j < MAX_WEIGHTS; j++)
                {
                        s1 = (j < v1->numWeights) ? (v1->weights[j]->boneIndex * v1->weights[j]->boneWeight) : 0;
                        s2 = (j < v2->numWeights) ? (v2->weights[j]->boneIndex * v2->weights[j]->boneWeight) : 0;
                }
        }

        if(s1 < s2)
                return -1;

        if(s1 > s2)
                return 1;

#endif

        return 0;
}*/

/**
 * @brief AddTriangleToVBOTriangleList
 * @param[in] vboTriangles
 * @param[in] tri
 * @param[in,out] numBoneReferences
 * @param[in,out] boneReferences
 * @return
 */
qboolean AddTriangleToVBOTriangleList(growList_t *vboTriangles, skelTriangle_t *tri, int *numBoneReferences, int boneReferences[MAX_BONES])
{
	int         i, j, k;
	md5Vertex_t *v;
	int         boneIndex;
	int         numNewReferences = 0;
	int         newReferences[MAX_WEIGHTS * 3];   // a single triangle can have up to 12 new bone references !
	qboolean    hasWeights = qfalse;

	Com_Memset(newReferences, -1, sizeof(newReferences));

	for (i = 0; i < 3; i++)
	{
		v = tri->vertexes[i];

		// can the bones be referenced?
		for (j = 0; j < MAX_WEIGHTS; j++)
		{
			if (j < v->numWeights)
			{
				boneIndex  = v->weights[j]->boneIndex;
				hasWeights = qtrue;

				// is the bone already referenced?
				if (!boneReferences[boneIndex])
				{
					// the bone isn't yet and we have to test if we can give the mesh this bone at all
					if ((*numBoneReferences + numNewReferences) >= glConfig2.maxVertexSkinningBones)
					{
						return qfalse;
					}
					else
					{
						for (k = 0; k < (MAX_WEIGHTS * 3); k++)
						{
							if (newReferences[k] == boneIndex)
							{
								break;
							}
						}

						if (k == (MAX_WEIGHTS * 3))
						{
							newReferences[numNewReferences] = boneIndex;
							numNewReferences++;
						}
					}
				}
			}
		}
	}

	// reference them!
	for (j = 0; j < numNewReferences; j++)
	{
		boneIndex = newReferences[j];

		boneReferences[boneIndex]++;

		*numBoneReferences = *numBoneReferences + 1;
	}

#if 0
	if (numNewReferences)
	{
		Ren_Print("bone indices: %i %i %i %i %i %i %i %i %i %i %i %i\n",
		          newReferences[0],
		          newReferences[1],
		          newReferences[2],
		          newReferences[3],
		          newReferences[4],
		          newReferences[5],
		          newReferences[6],
		          newReferences[7],
		          newReferences[8],
		          newReferences[9],
		          newReferences[10],
		          newReferences[11]);
	}
#endif

	if (hasWeights)
	{
		Com_AddToGrowList(vboTriangles, tri);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief AddSurfaceToVBOSurfacesList
 * @param[in] vboSurfaces
 * @param[in] vboTriangles
 * @param[in] md5
 * @param[in] surf
 * @param[in] skinIndex
 * @param numBoneReferences - unused
 * @param[in] boneReferences
 */
void AddSurfaceToVBOSurfacesList(growList_t *vboSurfaces, growList_t *vboTriangles, md5Model_t *md5, md5Surface_t *surf, int skinIndex, int numBoneReferences, int boneReferences[MAX_BONES])
{
	int             j, k;
	int             vertexesNum = surf->numVerts;
	byte            *data;
	int             dataSize;
	int             dataOfs;
	GLuint          ofsTexCoords;
	GLuint          ofsTangents;
	GLuint          ofsBinormals;
	GLuint          ofsNormals;
	GLuint          ofsBoneIndexes;
	GLuint          ofsBoneWeights;
	int             indexesNum = vboTriangles->currentElements * 3;
	byte            *indexes;
	int             indexesSize;
	int             indexesOfs;
	skelTriangle_t  *tri;
	vec4_t          tmp;
	int             index;
	srfVBOMD5Mesh_t *vboSurf;
	md5Vertex_t     *v;

	// create surface
	vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
	Com_AddToGrowList(vboSurfaces, vboSurf);

	vboSurf->surfaceType = SF_VBO_MD5MESH;
	vboSurf->md5Model    = md5;
	vboSurf->shader      = R_GetShaderByHandle(surf->shaderIndex);
	vboSurf->skinIndex   = skinIndex;
	vboSurf->numIndexes  = indexesNum;
	vboSurf->numVerts    = vertexesNum;

	dataSize = vertexesNum * (sizeof(vec4_t) * 8);
	data     = ri.Hunk_AllocateTempMemory(dataSize);
	dataOfs  = 0;

	indexesSize = indexesNum * sizeof(int);
	indexes     = ri.Hunk_AllocateTempMemory(indexesSize);
	indexesOfs  = 0;

	//Ren_Print("AddSurfaceToVBOSurfacesList( %i verts, %i tris )\n", surf->numVerts, vboTriangles->currentElements);

	vboSurf->numBoneRemap = 0;
	Com_Memset(vboSurf->boneRemap, 0, sizeof(vboSurf->boneRemap));
	Com_Memset(vboSurf->boneRemapInverse, 0, sizeof(vboSurf->boneRemapInverse));

	//Ren_Print("referenced bones: ");
	for (j = 0; j < MAX_BONES; j++)
	{
		if (boneReferences[j] > 0)
		{
			vboSurf->boneRemap[j]                            = vboSurf->numBoneRemap;
			vboSurf->boneRemapInverse[vboSurf->numBoneRemap] = j;

			vboSurf->numBoneRemap++;

			//Ren_Print("(%i -> %i) ", j, vboSurf->boneRemap[j]);
		}
	}

	//Ren_Print("\n");

	//for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
	for (j = 0; j < vboTriangles->currentElements; j++)
	{
		tri = Com_GrowListElement(vboTriangles, j);

		for (k = 0; k < 3; k++)
		{
			index = tri->indexes[k];

			Com_Memcpy(indexes + indexesOfs, &index, sizeof(int));
			indexesOfs += sizeof(int);
		}
	}

	// feed vertex XYZ
	for (j = 0; j < vertexesNum; j++)
	{
		for (k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].position[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex texcoords
	ofsTexCoords = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		for (k = 0; k < 2; k++)
		{
			tmp[k] = surf->verts[j].texCoords[k];
		}

		tmp[2] = 0;
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex tangents
	ofsTangents = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		for (k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].tangent[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex binormals
	ofsBinormals = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		for (k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].binormal[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex normals
	ofsNormals = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		for (k = 0; k < 3; k++)
		{
			tmp[k] = surf->verts[j].normal[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed bone indices
	ofsBoneIndexes = dataOfs;

	for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
	{
		for (k = 0; k < MAX_WEIGHTS; k++)
		{
			if (k < v->numWeights)
			{
				index = vboSurf->boneRemap[v->weights[k]->boneIndex];
			}
			else
			{
				index = 0;
			}

			Com_Memcpy(data + dataOfs, &index, sizeof(int));
			dataOfs += sizeof(int);
		}
	}

	// feed bone weights
	ofsBoneWeights = dataOfs;

	for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
	{
		for (k = 0; k < MAX_WEIGHTS; k++)
		{
			if (k < v->numWeights)
			{
				tmp[k] = v->weights[k]->boneWeight;
			}
			else
			{
				tmp[k] = 0;
			}
		}

		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	vboSurf->vbo                 = R_CreateVBO(va("staticMD5Mesh_VBO %i", vboSurfaces->currentElements), data, dataSize, VBO_USAGE_STATIC);
	vboSurf->vbo->ofsXYZ         = 0;
	vboSurf->vbo->ofsTexCoords   = ofsTexCoords;
	vboSurf->vbo->ofsLightCoords = ofsTexCoords;
	vboSurf->vbo->ofsTangents    = ofsTangents;
	vboSurf->vbo->ofsBinormals   = ofsBinormals;
	vboSurf->vbo->ofsNormals     = ofsNormals;
	vboSurf->vbo->ofsBoneIndexes = ofsBoneIndexes;
	vboSurf->vbo->ofsBoneWeights = ofsBoneWeights;

	vboSurf->ibo = R_CreateIBO(va("staticMD5Mesh_IBO %i", vboSurfaces->currentElements), indexes, indexesSize, VBO_USAGE_STATIC);

	ri.Hunk_FreeTempMemory(indexes);
	ri.Hunk_FreeTempMemory(data);

	// megs

	/*
	   Ren_Print("md5 mesh data VBO size: %d.%02d MB\n", dataSize / (1024 * 1024),
	   (dataSize % (1024 * 1024)) * 100 / (1024 * 1024));
	   Ren_Print("md5 mesh tris VBO size: %d.%02d MB\n", indexesSize / (1024 * 1024),
	   (indexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
	 */
}

/**
 * @brief AddSurfaceToVBOSurfacesList2
 * @param[in] vboSurfaces
 * @param[in] vboTriangles
 * @param[in] vboVertexes
 * @param[in] md5
 * @param[in] skinIndex
 * @param[in] materialName
 * @param numBoneReferences - unused
 * @param[in] boneReferences
 */
void AddSurfaceToVBOSurfacesList2(growList_t *vboSurfaces, growList_t *vboTriangles, growList_t *vboVertexes, md5Model_t *md5, int skinIndex, const char *materialName, int numBoneReferences, int boneReferences[MAX_BONES])
{
	int             j, k;
	int             vertexesNum = vboVertexes->currentElements;
	byte            *data;
	int             dataSize;
	int             dataOfs;
	GLuint          ofsTexCoords;
	GLuint          ofsTangents;
	GLuint          ofsBinormals;
	GLuint          ofsNormals;
	GLuint          ofsBoneIndexes;
	GLuint          ofsBoneWeights;
	int             indexesNum = vboTriangles->currentElements * 3;
	byte            *indexes;
	int             indexesSize;
	int             indexesOfs;
	skelTriangle_t  *tri;
	vec4_t          tmp;
	int             index;
	srfVBOMD5Mesh_t *vboSurf;
	md5Vertex_t     *v;
	shader_t        *shader;
	int             shaderIndex;

	// create surface
	vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
	Com_AddToGrowList(vboSurfaces, vboSurf);

	vboSurf->surfaceType = SF_VBO_MD5MESH;
	vboSurf->md5Model    = md5;

	Ren_Print("AddSurfaceToVBOSurfacesList2: loading shader '%s'\n", materialName);
	shader = R_FindShader(materialName, SHADER_3D_DYNAMIC, qtrue);

	if (shader->defaultShader)
	{
		shaderIndex = 0;
	}
	else
	{
		shaderIndex = shader->index;
	}

	vboSurf->shader = R_GetShaderByHandle(shaderIndex);

	vboSurf->skinIndex  = skinIndex;
	vboSurf->numIndexes = indexesNum;
	vboSurf->numVerts   = vertexesNum;

	dataSize = vertexesNum * (sizeof(vec4_t) * 8);
	data     = ri.Hunk_AllocateTempMemory(dataSize);
	dataOfs  = 0;

	indexesSize = indexesNum * sizeof(int);
	indexes     = ri.Hunk_AllocateTempMemory(indexesSize);
	indexesOfs  = 0;

	//Ren_Print("AddSurfaceToVBOSurfacesList( %i verts, %i tris )\n", surf->numVerts, vboTriangles->currentElements);

	vboSurf->numBoneRemap = 0;
	Com_Memset(vboSurf->boneRemap, 0, sizeof(vboSurf->boneRemap));
	Com_Memset(vboSurf->boneRemapInverse, 0, sizeof(vboSurf->boneRemapInverse));

	//Ren_Print("referenced bones: ");
	for (j = 0; j < MAX_BONES; j++)
	{
		if (boneReferences[j] > 0)
		{
			vboSurf->boneRemap[j]                            = vboSurf->numBoneRemap;
			vboSurf->boneRemapInverse[vboSurf->numBoneRemap] = j;

			vboSurf->numBoneRemap++;

			//Ren_Print("(%i -> %i) ", j, vboSurf->boneRemap[j]);
		}
	}

	//Ren_Print("\n");

	//for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
	for (j = 0; j < vboTriangles->currentElements; j++)
	{
		tri = Com_GrowListElement(vboTriangles, j);

		for (k = 0; k < 3; k++)
		{
			index = tri->indexes[k];

			Com_Memcpy(indexes + indexesOfs, &index, sizeof(int));
			indexesOfs += sizeof(int);
		}
	}

	// feed vertex XYZ
	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < 3; k++)
		{
			tmp[k] = v->position[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex texcoords
	ofsTexCoords = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < 2; k++)
		{
			tmp[k] = v->texCoords[k];
		}

		tmp[2] = 0;
		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex tangents
	ofsTangents = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < 3; k++)
		{
			tmp[k] = v->tangent[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex binormals
	ofsBinormals = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < 3; k++)
		{
			tmp[k] = v->binormal[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed vertex normals
	ofsNormals = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < 3; k++)
		{
			tmp[k] = v->normal[k];
		}

		tmp[3] = 1;
		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	// feed bone indices
	ofsBoneIndexes = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < MAX_WEIGHTS; k++)
		{
			if (k < v->numWeights)
			{
				index = vboSurf->boneRemap[v->weights[k]->boneIndex];
			}
			else
			{
				index = 0;
			}

			Com_Memcpy(data + dataOfs, &index, sizeof(int));
			dataOfs += sizeof(int);
		}
	}

	// feed bone weights
	ofsBoneWeights = dataOfs;

	for (j = 0; j < vertexesNum; j++)
	{
		v = Com_GrowListElement(vboVertexes, j);

		for (k = 0; k < MAX_WEIGHTS; k++)
		{
			if (k < v->numWeights)
			{
				tmp[k] = v->weights[k]->boneWeight;
			}
			else
			{
				tmp[k] = 0;
			}
		}

		Com_Memcpy(data + dataOfs, ( vec_t * ) tmp, sizeof(vec4_t));
		dataOfs += sizeof(vec4_t);
	}

	vboSurf->vbo                 = R_CreateVBO(va("staticMD5Mesh_VBO %i", vboSurfaces->currentElements), data, dataSize, VBO_USAGE_STATIC);
	vboSurf->vbo->ofsXYZ         = 0;
	vboSurf->vbo->ofsTexCoords   = ofsTexCoords;
	vboSurf->vbo->ofsLightCoords = ofsTexCoords;
	vboSurf->vbo->ofsTangents    = ofsTangents;
	vboSurf->vbo->ofsBinormals   = ofsBinormals;
	vboSurf->vbo->ofsNormals     = ofsNormals;
	vboSurf->vbo->ofsBoneIndexes = ofsBoneIndexes;
	vboSurf->vbo->ofsBoneWeights = ofsBoneWeights;

	vboSurf->ibo = R_CreateIBO(va("staticMD5Mesh_IBO %i", vboSurfaces->currentElements), indexes, indexesSize, VBO_USAGE_STATIC);

	ri.Hunk_FreeTempMemory(indexes);
	ri.Hunk_FreeTempMemory(data);

	// megs

	/*
	   Ren_Print("md5 mesh data VBO size: %d.%02d MB\n", dataSize / (1024 * 1024),
	   (dataSize % (1024 * 1024)) * 100 / (1024 * 1024));
	   Ren_Print("md5 mesh tris VBO size: %d.%02d MB\n", indexesSize / (1024 * 1024),
	   (indexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
	 */

	Ren_Print("created VBO surface %i with %i vertices and %i triangles\n", vboSurfaces->currentElements, vboSurf->numVerts, vboSurf->numIndexes / 3);
}
