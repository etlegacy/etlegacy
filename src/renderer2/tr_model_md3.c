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
 * @file renderer2/tr_model_md3.c
 * @brief Quake 3 model loading and caching
 */

#include "tr_local.h"

#if 0
/**
 * @brief Compare function for qsort()
 * @param[in] a
 * @param[in] b
 * @return
 */
static int MDXSurfaceCompare(const void *a, const void *b)
{
	mdvSurface_t *aa, *bb;

	aa = *( mdvSurface_t ** ) a;
	bb = *( mdvSurface_t ** ) b;

	// shader first
	if (&aa->shader < &bb->shader)
	{
		return -1;
	}
	else if (&aa->shader > &bb->shader)
	{
		return 1;
	}

	return 0;
}

#endif

/**
 * @brief R_MD3_CreateVBO_Surfaces
 * @param[in,out] mdvModel
 */
static void R_MD3_CreateVBO_Surfaces(mdvModel_t *mdvModel)
{
	int            i, j, k;
	mdvSurface_t   *surf; //, *surface;
	srfTriangle_t  *tri;
	mdvNormTanBi_t *vertexes;
	mdvNormTanBi_t *vert;

	growList_t      vboSurfaces;
	srfVBOMDVMesh_t *vboSurf;

	byte *data;
	int  dataSize;
	int  dataOfs;

	vec4_t tmp;

	GLuint ofsTexCoords;
	GLuint ofsTangents;
	GLuint ofsBinormals;
	GLuint ofsNormals;

	GLuint sizeXYZ       = 0;
	GLuint sizeTangents  = 0;
	GLuint sizeBinormals = 0;
	GLuint sizeNormals   = 0;

	int vertexesNum;
	int f;

	Com_InitGrowList(&vboSurfaces, 32);

	for (i = 0, surf = mdvModel->surfaces; i < mdvModel->numSurfaces; i++, surf++)
	{
		//allocate temp memory for vertex data
		vertexes = (mdvNormTanBi_t *)ri.Hunk_AllocateTempMemory(sizeof(*vertexes) * surf->numVerts * mdvModel->numFrames);

		// calc tangent spaces
		{
			for (j = 0, vert = vertexes; j < (surf->numVerts * mdvModel->numFrames); j++, vert++)
			{
				VectorClear(vert->tangent);
				VectorClear(vert->binormal);
				VectorClear(vert->normal);
			}

			for (f = 0; f < mdvModel->numFrames; f++)
			{
				for (j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
				{
					vec3_t tangent  = { 0, 0, 0 };
					vec3_t binormal = { 0, 0, 0 };
					vec3_t normal   = { 0, 0, 0 };

					const float *v0 = surf->verts[surf->numVerts * f + tri->indexes[0]].xyz;
					const float *v1 = surf->verts[surf->numVerts * f + tri->indexes[1]].xyz;
					const float *v2 = surf->verts[surf->numVerts * f + tri->indexes[2]].xyz;

					const float *t0 = surf->st[tri->indexes[0]].st;
					const float *t1 = surf->st[tri->indexes[1]].st;
					const float *t2 = surf->st[tri->indexes[2]].st;
#if 1
					R_CalcTangentSpace(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);
#else
					R_CalcNormalForTriangle(normal, v0, v1, v2);
					R_CalcTangentsForTriangle(tangent, binormal, v0, v1, v2, t0, t1, t2);
#endif

					for (k = 0; k < 3; k++)
					{
						float *v = vertexes[surf->numVerts * f + tri->indexes[k]].tangent;
						VectorAdd(v, tangent, v);

						v = vertexes[surf->numVerts * f + tri->indexes[k]].binormal;
						VectorAdd(v, binormal, v);

						v = vertexes[surf->numVerts * f + tri->indexes[k]].normal;
						VectorAdd(v, normal, v);
					}
				}
			}

			for (j = 0, vert = vertexes; j < (surf->numVerts * mdvModel->numFrames); j++, vert++)
			{
				VectorNormalize(vert->tangent);
				VectorNormalize(vert->binormal);
				VectorNormalize(vert->normal);
			}
/* FIXME: Hunk_FreeTempMemory: not the final block
            // Note: This does basically work - vanilla truck is fine with smoothed normals
            //       ... but new r2 truck model fails ... :/ looks better w/o
            if (r_smoothNormals->integer & FLAGS_SMOOTH_MD3) // do another extra smoothing for normals to avoid flat shading
            {
                int vf, vfj, vfk, numSame;
                vec3_t   vertexj, vertexk, avgVector;
                int *same;
                qboolean *done;

                // allocate temp memory for the "points already checked" array
                done = (qboolean *)ri.Hunk_AllocateTempMemory(sizeof(done) * surf->numVerts);

                 // allocate temp memory for the "points are the same" array
                same = (int *)ri.Hunk_AllocateTempMemory(sizeof(same) * surf->numVerts);

                for (f = 0; f < mdvModel->numFrames; f++)
                {
                    // clear 'done' array:
                    for (j = 0; j < surf->numVerts; j++)
                    {
                        done[j] = qfalse; // use memset(&done[0], 0, surf->numVerts) ?
                    }

                    vf = surf->numVerts * f;
                    for (j = 0; j < surf->numVerts; j++)
                    {
                        if (done[j])
                        {
                            continue; // skip what is done
                        }

                        done[j] = qtrue; // , mark as done, and process..

                        vfj = vf + j;

                        numSame = 0; // new test, so reset the count..

                        // store this vertex number, and increment the found total of same vertices
                        same[numSame++] = vfj;

                        VectorCopy(surf->verts[vfj].xyz, vertexj);

                        // so far, there is only 1 vertex, so the average normal is simply the vertex's normal
                        VectorCopy(avgVector, vertexes[vfj].normal);

                        for (k = j + 1; k < surf->numVerts; k++)
                        {
                            if (done[k])
                            {
                                continue;
                            }

                            vfk = vf + k;

                            VectorCopy(surf->verts[vfk].xyz, vertexk);

                            //if (VectorCompare(vertexj, vertexk))
                            if (VectorCompareEpsilon(vertexj, vertexk, 0.02f))
                            {
                                done[k] = qtrue;

                                VectorAdd(avgVector, vertexes[vfk].normal, avgVector);

                                // store this vertex number, and increment the found total of same vertices
                                same[numSame++] = vfk;
                            }
                        }

                        // average the vector
                        VectorScale(avgVector, 1.0 / (float)numSame, avgVector);
                        //VectorNormalize(avgVector); //?!

                        // now write back the newly calculated average normal for all the same vertices
                        for (k = 0; k < numSame; k++)
                        {
                            VectorCopy(avgVector, vertexes[same[k]].normal);
                        }
                    }
                }
                ri.Hunk_FreeTempMemory(done);
                ri.Hunk_FreeTempMemory(same);
            }
*/
		}

		//Ren_Print("...calculating MD3 mesh VBOs ( '%s', %i verts %i tris )\n", surf->name, surf->numVerts, surf->numTriangles);

		// create surface
		vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
		Com_AddToGrowList(&vboSurfaces, vboSurf);

		vboSurf->surfaceType = SF_VBO_MDVMESH;
		vboSurf->mdvModel    = mdvModel;
		vboSurf->mdvSurface  = surf;
		vboSurf->numIndexes  = surf->numTriangles * 3;
		vboSurf->numVerts    = surf->numVerts;

		/*
		vboSurf->vbo = R_CreateVBO2(va("staticWorldMesh_vertices %i", vboSurfaces.currentElements), numVerts, optimizedVerts,
		ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL
		| ATTR_COLOR);
		*/

		vboSurf->ibo = R_CreateIBO2(va("staticMD3Mesh_IBO %s", surf->name), surf->numTriangles, surf->triangles, VBO_USAGE_STATIC);

		vertexesNum = surf->numVerts;

		//allocate vbo data
		dataSize = (surf->numVerts * mdvModel->numFrames * sizeof(vec4_t) * 4) +      // xyz, tangent, binormal, normal
		           (surf->numVerts * sizeof(vec4_t)); // texcoords

		data    = ri.Hunk_AllocateTempMemory(dataSize);
		dataOfs = 0;

		// feed vertex XYZ
		for (f = 0; f < mdvModel->numFrames; f++)
		{
			for (j = 0; j < vertexesNum; j++)
			{
				for (k = 0; k < 3; k++)
				{
					tmp[k] = surf->verts[f * vertexesNum + j].xyz[k];
				}

				tmp[3] = 1;
				Com_Memcpy(data + dataOfs, (vec_t *)tmp, sizeof(vec4_t));
				dataOfs += sizeof(vec4_t);
			}

			if (f == 0)
			{
				sizeXYZ = dataOfs;
			}
		}

		// feed vertex texcoords
		ofsTexCoords = dataOfs;

		for (j = 0; j < vertexesNum; j++)
		{
			for (k = 0; k < 2; k++)
			{
				tmp[k] = surf->st[j].st[k];
			}

			tmp[2] = 0;
			tmp[3] = 1;
			Com_Memcpy(data + dataOfs, (vec_t *)tmp, sizeof(vec4_t));
			dataOfs += sizeof(vec4_t);
		}

		// feed vertex tangents
		ofsTangents = dataOfs;

		for (f = 0; f < mdvModel->numFrames; f++)
		{
			for (j = 0; j < vertexesNum; j++)
			{
				for (k = 0; k < 3; k++)
				{
					tmp[k] = vertexes[f * vertexesNum + j].tangent[k];
				}

				tmp[3] = 1;
				Com_Memcpy(data + dataOfs, (vec_t *)tmp, sizeof(vec4_t));
				dataOfs += sizeof(vec4_t);
			}

			if (f == 0)
			{
				sizeTangents = dataOfs - ofsTangents;
			}
		}

		// feed vertex binormals
		ofsBinormals = dataOfs;

		for (f = 0; f < mdvModel->numFrames; f++)
		{
			for (j = 0; j < vertexesNum; j++)
			{
				for (k = 0; k < 3; k++)
				{
					tmp[k] = vertexes[f * vertexesNum + j].binormal[k];
				}

				tmp[3] = 1;
				Com_Memcpy(data + dataOfs, (vec_t *)tmp, sizeof(vec4_t));
				dataOfs += sizeof(vec4_t);
			}

			if (f == 0)
			{
				sizeBinormals = dataOfs - ofsBinormals;
			}
		}

		// feed vertex normals
		ofsNormals = dataOfs;

		for (f = 0; f < mdvModel->numFrames; f++)
		{
			for (j = 0; j < vertexesNum; j++)
			{
				for (k = 0; k < 3; k++)
				{
					tmp[k] = vertexes[f * vertexesNum + j].normal[k];
				}

				tmp[3] = 1;
				Com_Memcpy(data + dataOfs, (vec_t *)tmp, sizeof(vec4_t));
				dataOfs += sizeof(vec4_t);
			}

			if (f == 0)
			{
				sizeNormals = dataOfs - ofsNormals;
			}
		}

		vboSurf->vbo                 = R_CreateVBO(va("staticMD3Mesh_VBO '%s'", surf->name), data, dataSize, VBO_USAGE_STATIC);
		vboSurf->vbo->ofsXYZ         = 0;
		vboSurf->vbo->ofsTexCoords   = ofsTexCoords;
		vboSurf->vbo->ofsLightCoords = ofsTexCoords;
		vboSurf->vbo->ofsTangents    = ofsTangents;
		vboSurf->vbo->ofsBinormals   = ofsBinormals;
		vboSurf->vbo->ofsNormals     = ofsNormals;

		vboSurf->vbo->sizeXYZ       = sizeXYZ;
		vboSurf->vbo->sizeTangents  = sizeTangents;
		vboSurf->vbo->sizeBinormals = sizeBinormals;
		vboSurf->vbo->sizeNormals   = sizeNormals;

		ri.Hunk_FreeTempMemory(data);
		ri.Hunk_FreeTempMemory(vertexes);
	}

	// move VBO surfaces list to hunk
	mdvModel->numVBOSurfaces = vboSurfaces.currentElements;
	mdvModel->vboSurfaces    = ri.Hunk_Alloc(mdvModel->numVBOSurfaces * sizeof(*mdvModel->vboSurfaces), h_low);

	for (i = 0; i < mdvModel->numVBOSurfaces; i++)
	{
		mdvModel->vboSurfaces[i] = (srfVBOMDVMesh_t *)Com_GrowListElement(&vboSurfaces, i);
	}

	Com_DestroyGrowList(&vboSurfaces);
}

/**
 * @brief R_LoadMD3
 * @param[in,out] mod
 * @param[in] lod
 * @param[in,out] buffer
 * @param bufferSize - unused
 * @param[in] modName
 * @return
 */
qboolean R_LoadMD3(model_t *mod, int lod, void *buffer, int bufferSize, const char *modName)
{
	int            i, j;
	md3Header_t    *md3Model = ( md3Header_t * ) buffer;
	md3Frame_t     *md3Frame;
	md3Surface_t   *md3Surf;
	md3Shader_t    *md3Shader;
	md3Triangle_t  *md3Tri;
	md3St_t        *md3st;
	md3XyzNormal_t *md3xyz;
	md3Tag_t       *md3Tag;
	mdvModel_t     *mdvModel;
	mdvFrame_t     *frame;
	mdvSurface_t   *surf; //, *surface;
	srfTriangle_t  *tri;
	mdvXyz_t       *v;
	mdvSt_t        *st;
	mdvTag_t       *tag;
	mdvTagName_t   *tagName;
	int            version;
	int            size;

	version = LittleLong(md3Model->version);

	if (version != MD3_VERSION)
	{
		Ren_Warning("R_LoadMD3: %s has wrong version (%i should be %i)\n", modName, version, MD3_VERSION);
		return qfalse;
	}

	mod->type      = MOD_MESH;
	size           = LittleLong(md3Model->ofsEnd);
	mod->dataSize += size;
	mdvModel       = mod->mdv[lod] = ri.Hunk_Alloc(sizeof(mdvModel_t), h_low);

	//Com_Memcpy(mod->md3[lod], buffer, LittleLong(md3Model->ofsEnd));

	LL(md3Model->ident);
	LL(md3Model->version);
	LL(md3Model->numFrames);
	LL(md3Model->numTags);
	LL(md3Model->numSurfaces);
	LL(md3Model->ofsFrames);
	LL(md3Model->ofsTags);
	LL(md3Model->ofsSurfaces);
	LL(md3Model->ofsEnd);

	if (md3Model->numFrames < 1)
	{
		Ren_Warning("R_LoadMD3: %s has no frames\n", modName);
		return qfalse;
	}

	// swap all the frames
	mdvModel->numFrames = md3Model->numFrames;
	mdvModel->frames    = frame = ri.Hunk_Alloc(sizeof(*frame) * md3Model->numFrames, h_low);

	md3Frame = ( md3Frame_t * )(( byte * ) md3Model + md3Model->ofsFrames);

	for (i = 0; i < md3Model->numFrames; i++, frame++, md3Frame++)
	{
		frame->radius = LittleFloat(md3Frame->radius);

		for (j = 0; j < 3; j++)
		{
			frame->bounds[0][j]   = LittleFloat(md3Frame->bounds[0][j]);
			frame->bounds[1][j]   = LittleFloat(md3Frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(md3Frame->localOrigin[j]);
		}
	}

	// swap all the tags
	mdvModel->numTags = md3Model->numTags;
	mdvModel->tags    = tag = ri.Hunk_Alloc(sizeof(*tag) * (md3Model->numTags * md3Model->numFrames), h_low);

	md3Tag = ( md3Tag_t * )(( byte * ) md3Model + md3Model->ofsTags);

	for (i = 0; i < md3Model->numTags * md3Model->numFrames; i++, tag++, md3Tag++)
	{
		for (j = 0; j < 3; j++)
		{
			tag->origin[j]  = LittleFloat(md3Tag->origin[j]);
			tag->axis[0][j] = LittleFloat(md3Tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(md3Tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(md3Tag->axis[2][j]);
		}
	}

	mdvModel->tagNames = tagName = ri.Hunk_Alloc(sizeof(*tagName) * (md3Model->numTags), h_low);

	md3Tag = ( md3Tag_t * )(( byte * ) md3Model + md3Model->ofsTags);

	for (i = 0; i < md3Model->numTags; i++, tagName++, md3Tag++)
	{
		Q_strncpyz(tagName->name, md3Tag->name, sizeof(tagName->name));
	}

	// swap all the surfaces
	mdvModel->numSurfaces = md3Model->numSurfaces;
	mdvModel->surfaces    = surf = ri.Hunk_Alloc(sizeof(*surf) * md3Model->numSurfaces, h_low);

	md3Surf = ( md3Surface_t * )(( byte * ) md3Model + md3Model->ofsSurfaces);

	for (i = 0; i < md3Model->numSurfaces; i++)
	{
		LL(md3Surf->ident);
		LL(md3Surf->flags);
		LL(md3Surf->numFrames);
		LL(md3Surf->numShaders);
		LL(md3Surf->numTriangles);
		LL(md3Surf->ofsTriangles);
		LL(md3Surf->numVerts);
		LL(md3Surf->ofsShaders);
		LL(md3Surf->ofsSt);
		LL(md3Surf->ofsXyzNormals);
		LL(md3Surf->ofsEnd);

		if (md3Surf->numVerts > SHADER_MAX_VERTEXES)
		{
			Ren_Drop("R_LoadMD3: %s has more than %i verts on a surface (%i)",
			         modName, SHADER_MAX_VERTEXES, md3Surf->numVerts);
		}

		if (md3Surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			Ren_Drop("R_LoadMD3: %s has more than %i triangles on a surface (%i)",
			         modName, SHADER_MAX_INDEXES / 3, md3Surf->numTriangles);
		}

		// change to surface identifier
		surf->surfaceType = SF_MDV;

		// give pointer to model for Tess_SurfaceMDX
		surf->model = mdvModel;

		// copy surface name
		Q_strncpyz(surf->name, md3Surf->name, sizeof(surf->name));

		// lowercase the surface name so skin compares are faster
		Q_strlwr(surf->name);

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen(surf->name);

		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders

		/*
		   surf->numShaders = md3Surf->numShaders;
		   surf->shaders = shader = ri.Hunk_Alloc(sizeof(*shader) * md3Surf->numShaders, h_low);

		   md3Shader = (md3Shader_t *) ((byte *) md3Surf + md3Surf->ofsShaders);
		   for(j = 0; j < md3Surf->numShaders; j++, shader++, md3Shader++)
		   {
		   shader_t       *sh;

		   sh = R_FindShader(md3Shader->name, SHADER_3D_DYNAMIC, RSF_DEFAULT);
		   if(sh->defaultShader)
		   {
		   shader->shaderIndex = 0;
		   }
		   else
		   {
		   shader->shaderIndex = sh->index;
		   }
		   }
		 */

		// only consider the first shader
		md3Shader    = ( md3Shader_t * )(( byte * ) md3Surf + md3Surf->ofsShaders);
		surf->shader = R_FindShader(md3Shader->name, SHADER_3D_DYNAMIC, qtrue);

		// swap all the triangles
		surf->numTriangles = md3Surf->numTriangles;
		surf->triangles    = tri = ri.Hunk_Alloc(sizeof(*tri) * md3Surf->numTriangles, h_low);

		md3Tri = ( md3Triangle_t * )(( byte * ) md3Surf + md3Surf->ofsTriangles);

		for (j = 0; j < md3Surf->numTriangles; j++, tri++, md3Tri++)
		{
			tri->indexes[0] = LittleLong(md3Tri->indexes[0]);
			tri->indexes[1] = LittleLong(md3Tri->indexes[1]);
			tri->indexes[2] = LittleLong(md3Tri->indexes[2]);
		}

		// swap all the XyzNormals
		surf->numVerts = md3Surf->numVerts;
		surf->verts    = v = ri.Hunk_Alloc(sizeof(*v) * (md3Surf->numVerts * md3Surf->numFrames), h_low);

		md3xyz = ( md3XyzNormal_t * )(( byte * ) md3Surf + md3Surf->ofsXyzNormals);

		for (j = 0; j < md3Surf->numVerts * md3Surf->numFrames; j++, md3xyz++, v++)
		{
			v->xyz[0] = LittleShort(md3xyz->xyz[0]) * MD3_XYZ_SCALE;
			v->xyz[1] = LittleShort(md3xyz->xyz[1]) * MD3_XYZ_SCALE;
			v->xyz[2] = LittleShort(md3xyz->xyz[2]) * MD3_XYZ_SCALE;

			md3xyz->normal = LittleShort(md3xyz->normal); // from r1
		}

		// swap all the ST
		surf->st = st = ri.Hunk_Alloc(sizeof(*st) * md3Surf->numVerts, h_low);

		md3st = ( md3St_t * )(( byte * ) md3Surf + md3Surf->ofsSt);

		for (j = 0; j < md3Surf->numVerts; j++, md3st++, st++)
		{
			st->st[0] = LittleFloat(md3st->st[0]);
			st->st[1] = LittleFloat(md3st->st[1]);
		}

		// find the next surface
		md3Surf = ( md3Surface_t * )(( byte * ) md3Surf + md3Surf->ofsEnd);
		surf++;
	}

	// create VBO surfaces from md3 surfaces
	R_MD3_CreateVBO_Surfaces(mdvModel);

	return qtrue;
}
