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
 * @file renderer2/tr_model_md5.c
 * @brief Doom 3 .md5mesh model loading and caching
 */

#include "tr_local.h"
#include "tr_model_skel.h"

/**
 * @brief R_LoadMD5
 * @param[in,out] mod
 * @param[in,out] buffer
 * @param bufferSize - unused
 * @param[in] modName
 * @return
 */
qboolean R_LoadMD5(model_t *mod, void *buffer, int bufferSize, const char *modName)
{
	int           i, j, k;
	md5Model_t    *md5;
	md5Bone_t     *bone;
	md5Surface_t  *surf;
	srfTriangle_t *tri;
	md5Vertex_t   *v;
	md5Weight_t   *weight;
	int           version;
	shader_t      *sh;
	char          *buf_p = ( char * ) buffer;
	char          *token;
	vec3_t        boneOrigin;
	quat_t        boneQuat;
	mat4_t        boneMat;
	int           numRemaining;
	growList_t    sortedTriangles;
	growList_t    vboTriangles;
	growList_t    vboSurfaces;
	int           numBoneReferences;
	int           boneReferences[MAX_BONES];

	// skip MD5Version indent string
	(void) COM_ParseExt2(&buf_p, qfalse);

	// check version
	token   = COM_ParseExt2(&buf_p, qfalse);
	version = Q_atoi(token);

	if (version != MD5_VERSION)
	{
		Ren_Warning("R_LoadMD5: %s has wrong version (%i should be %i)\n", modName, version, MD5_VERSION);
		return qfalse;
	}

	mod->type      = MOD_MD5;
	mod->dataSize += sizeof(md5Model_t);
	md5            = mod->md5 = ri.Hunk_Alloc(sizeof(md5Model_t), h_low);

	// skip commandline <arguments string>
	token = COM_ParseExt2(&buf_p, qtrue);
	token = COM_ParseExt2(&buf_p, qtrue);
	//  Ren_Print("%s\n", token);

	// parse numJoints <number>
	token = COM_ParseExt2(&buf_p, qtrue);

	if (Q_stricmp(token, "numJoints"))
	{
		Ren_Warning("R_LoadMD5: expected 'numJoints' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	token         = COM_ParseExt2(&buf_p, qfalse);
	md5->numBones = Q_atoi(token);

	// parse numMeshes <number>
	token = COM_ParseExt2(&buf_p, qtrue);

	if (Q_stricmp(token, "numMeshes"))
	{
		Ren_Warning("R_LoadMD5: expected 'numMeshes' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	token            = COM_ParseExt2(&buf_p, qfalse);
	md5->numSurfaces = Q_atoi(token);
	//Ren_Print("R_LoadMD5: '%s' has %i surfaces\n", modName, md5->numSurfaces);

	if (md5->numBones < 1)
	{
		Ren_Warning("R_LoadMD5: '%s' has no bones\n", modName);
		return qfalse;
	}

	if (md5->numBones > MAX_BONES)
	{
		Ren_Warning("R_LoadMD5: '%s' has more than %i bones (%i)\n", modName, MAX_BONES, md5->numBones);
		return qfalse;
	}

	//Ren_Print("R_LoadMD5: '%s' has %i bones\n", modName, md5->numBones);

	// parse all the bones
	md5->bones = ri.Hunk_Alloc(sizeof(*bone) * md5->numBones, h_low);

	// parse joints {
	token = COM_ParseExt2(&buf_p, qtrue);

	if (Q_stricmp(token, "joints"))
	{
		Ren_Warning("R_LoadMD5: expected 'joints' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	token = COM_ParseExt2(&buf_p, qfalse);

	if (Q_stricmp(token, "{"))
	{
		Ren_Warning("R_LoadMD5: expected '{' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	for (i = 0, bone = md5->bones; i < md5->numBones; i++, bone++)
	{
		token = COM_ParseExt2(&buf_p, qtrue);
		Q_strncpyz(bone->name, token, sizeof(bone->name));

		//Ren_Print("R_LoadMD5: '%s' has bone '%s'\n", modName, bone->name);

		token             = COM_ParseExt2(&buf_p, qfalse);
		bone->parentIndex = Q_atoi(token);

		//Ren_Print("R_LoadMD5: '%s' has bone '%s' with parent index %i\n", modName, bone->name, bone->parentIndex);

		if (bone->parentIndex >= md5->numBones)
		{
			Ren_Drop("R_LoadMD5: '%s' has bone '%s' with bad parent index %i while numBones is %i", modName,
			         bone->name, bone->parentIndex, md5->numBones);
		}

		// skip (
		token = COM_ParseExt2(&buf_p, qfalse);

		if (Q_stricmp(token, "("))
		{
			Ren_Warning("R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		for (j = 0; j < 3; j++)
		{
			token         = COM_ParseExt2(&buf_p, qfalse);
			boneOrigin[j] = atof(token);
		}

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);

		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		// skip (
		token = COM_ParseExt2(&buf_p, qfalse);

		if (Q_stricmp(token, "("))
		{
			Ren_Warning("R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		for (j = 0; j < 3; j++)
		{
			token       = COM_ParseExt2(&buf_p, qfalse);
			boneQuat[j] = atof(token);
		}

		QuatCalcW(boneQuat);
		mat4_from_quat(boneMat, boneQuat);

		VectorCopy(boneOrigin, bone->origin);
		quat_copy(boneQuat, bone->rotation);

		MatrixSetupTransformFromQuat(bone->inverseTransform, boneQuat, boneOrigin);
		mat4_inverse_self(bone->inverseTransform);

		// skip )
		token = COM_ParseExt2(&buf_p, qfalse);

		if (Q_stricmp(token, ")"))
		{
			Ren_Warning("R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}
	}

	// parse }
	token = COM_ParseExt2(&buf_p, qtrue);

	if (Q_stricmp(token, "}"))
	{
		Ren_Warning("R_LoadMD5: expected '}' found '%s' in model '%s'\n", token, modName);
		return qfalse;
	}

	// parse all the surfaces
	if (md5->numSurfaces < 1)
	{
		Ren_Warning("R_LoadMD5: '%s' has no surfaces\n", modName);
		return qfalse;
	}

	//Ren_Print("R_LoadMD5: '%s' has %i surfaces\n", modName, md5->numSurfaces);

	md5->surfaces = ri.Hunk_Alloc(sizeof(*surf) * md5->numSurfaces, h_low);

	for (i = 0, surf = md5->surfaces; i < md5->numSurfaces; i++, surf++)
	{
		// parse mesh {
		token = COM_ParseExt2(&buf_p, qtrue);

		if (Q_stricmp(token, "mesh"))
		{
			Ren_Warning("R_LoadMD5: expected 'mesh' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		token = COM_ParseExt2(&buf_p, qfalse);

		if (Q_stricmp(token, "{"))
		{
			Ren_Warning("R_LoadMD5: expected '{' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		// change to surface identifier
		surf->surfaceType = SF_MD5;

		// give pointer to model for Tess_SurfaceMD5
		surf->model = md5;

		// parse shader <name>
		token = COM_ParseExt2(&buf_p, qtrue);

		if (Q_stricmp(token, "shader"))
		{
			Ren_Warning("R_LoadMD5: expected 'shader' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		token = COM_ParseExt2(&buf_p, qfalse);
		Q_strncpyz(surf->shader, token, sizeof(surf->shader));

		//Ren_Print("R_LoadMD5: '%s' uses shader '%s'\n", modName, surf->shader);

		// FIXME .md5mesh meshes don't have surface names
		// lowercase the surface name so skin compares are faster
		//Q_strlwr(surf->name);
		//Ren_Print("R_LoadMD5: '%s' has surface '%s'\n", modName, surf->name);

		// register the shaders
		sh = R_FindShader(surf->shader, SHADER_3D_DYNAMIC, qtrue);

		if (sh->defaultShader)
		{
			surf->shaderIndex = 0;
		}
		else
		{
			surf->shaderIndex = sh->index;
		}

		// parse numVerts <number>
		token = COM_ParseExt2(&buf_p, qtrue);

		if (Q_stricmp(token, "numVerts"))
		{
			Ren_Warning("R_LoadMD5: expected 'numVerts' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		token          = COM_ParseExt2(&buf_p, qfalse);
		surf->numVerts = Q_atoi(token);

		if (surf->numVerts > SHADER_MAX_VERTEXES)
		{
			Ren_Drop("R_LoadMD5: '%s' has more than %i verts on a surface (%i)",
			         modName, SHADER_MAX_VERTEXES, surf->numVerts);
		}

		surf->verts = ri.Hunk_Alloc(sizeof(*v) * surf->numVerts, h_low);

		for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			// skip vert <number>
			token = COM_ParseExt2(&buf_p, qtrue);

			if (Q_stricmp(token, "vert"))
			{
				Ren_Warning("R_LoadMD5: expected 'vert' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			(void) COM_ParseExt2(&buf_p, qfalse);

			// skip (
			token = COM_ParseExt2(&buf_p, qfalse);

			if (Q_stricmp(token, "("))
			{
				Ren_Warning("R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			for (k = 0; k < 2; k++)
			{
				token           = COM_ParseExt2(&buf_p, qfalse);
				v->texCoords[k] = atof(token);
			}

			// skip )
			token = COM_ParseExt2(&buf_p, qfalse);

			if (Q_stricmp(token, ")"))
			{
				Ren_Warning("R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			token          = COM_ParseExt2(&buf_p, qfalse);
			v->firstWeight = Q_atoi(token);

			token         = COM_ParseExt2(&buf_p, qfalse);
			v->numWeights = Q_atoi(token);

			if (v->numWeights > MAX_WEIGHTS)
			{
				Ren_Drop("R_LoadMD5: vertex %i requires more than %i weights on surface (%i) in model '%s'",
				         j, MAX_WEIGHTS, i, modName);
			}
		}

		// parse numTris <number>
		token = COM_ParseExt2(&buf_p, qtrue);

		if (Q_stricmp(token, "numTris"))
		{
			Ren_Warning("R_LoadMD5: expected 'numTris' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		token              = COM_ParseExt2(&buf_p, qfalse);
		surf->numTriangles = Q_atoi(token);

		if (surf->numTriangles > SHADER_MAX_TRIANGLES)
		{
			Ren_Drop("R_LoadMD5: '%s' has more than %i triangles on a surface (%i)",
			         modName, SHADER_MAX_TRIANGLES, surf->numTriangles);
		}

		surf->triangles = ri.Hunk_Alloc(sizeof(*tri) * surf->numTriangles, h_low);

		for (j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
		{
			// skip tri <number>
			token = COM_ParseExt2(&buf_p, qtrue);

			if (Q_stricmp(token, "tri"))
			{
				Ren_Warning("R_LoadMD5: expected 'tri' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			(void) COM_ParseExt2(&buf_p, qfalse);

			for (k = 0; k < 3; k++)
			{
				token           = COM_ParseExt2(&buf_p, qfalse);
				tri->indexes[k] = Q_atoi(token);
			}
		}

		// parse numWeights <number>
		token = COM_ParseExt2(&buf_p, qtrue);

		if (Q_stricmp(token, "numWeights"))
		{
			Ren_Warning("R_LoadMD5: expected 'numWeights' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		token            = COM_ParseExt2(&buf_p, qfalse);
		surf->numWeights = Q_atoi(token);

		surf->weights = ri.Hunk_Alloc(sizeof(*weight) * surf->numWeights, h_low);

		for (j = 0, weight = surf->weights; j < surf->numWeights; j++, weight++)
		{
			// skip weight <number>
			token = COM_ParseExt2(&buf_p, qtrue);

			if (Q_stricmp(token, "weight"))
			{
				Ren_Warning("R_LoadMD5: expected 'weight' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			(void) COM_ParseExt2(&buf_p, qfalse);

			token             = COM_ParseExt2(&buf_p, qfalse);
			weight->boneIndex = Q_atoi(token);

			token              = COM_ParseExt2(&buf_p, qfalse);
			weight->boneWeight = atof(token);

			// skip (
			token = COM_ParseExt2(&buf_p, qfalse);

			if (Q_stricmp(token, "("))
			{
				Ren_Warning("R_LoadMD5: expected '(' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}

			for (k = 0; k < 3; k++)
			{
				token             = COM_ParseExt2(&buf_p, qfalse);
				weight->offset[k] = atof(token);
			}

			// skip )
			token = COM_ParseExt2(&buf_p, qfalse);

			if (Q_stricmp(token, ")"))
			{
				Ren_Warning("R_LoadMD5: expected ')' found '%s' in model '%s'\n", token, modName);
				return qfalse;
			}
		}

		// parse }
		token = COM_ParseExt2(&buf_p, qtrue);

		if (Q_stricmp(token, "}"))
		{
			Ren_Warning("R_LoadMD5: expected '}' found '%s' in model '%s'\n", token, modName);
			return qfalse;
		}

		// loop trough all vertices and set up the vertex weights
		for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			v->weights = ri.Hunk_Alloc(sizeof(*v->weights) * v->numWeights, h_low);

			for (k = 0; k < v->numWeights; k++)
			{
				v->weights[k] = surf->weights + (v->firstWeight + k);
			}
		}
	}

	// loading is done now calculate the bounding box and tangent spaces
	ClearBounds(md5->bounds[0], md5->bounds[1]);

	for (i = 0, surf = md5->surfaces; i < md5->numSurfaces; i++, surf++)
	{
		for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
		{
			vec3_t      tmpVert;
			md5Weight_t *w;

			VectorClear(tmpVert);

			for (k = 0, w = v->weights[0]; k < v->numWeights; k++, w++)
			{
				vec3_t offsetVec;

				bone = &md5->bones[w->boneIndex];

				QuatTransformVector(bone->rotation, w->offset, offsetVec);
				VectorAdd(bone->origin, offsetVec, offsetVec);

				VectorMA(tmpVert, w->boneWeight, offsetVec, tmpVert);
			}

			VectorCopy(tmpVert, v->position);
			AddPointToBounds(tmpVert, md5->bounds[0], md5->bounds[1]);
		}

		// calc tangent spaces
#if 1
		{
			const float *v0, *v1, *v2;
			const float *t0, *t1, *t2;
			vec3_t      tangent;
			vec3_t      binormal;
			vec3_t      normal;

			for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				VectorClear(v->tangent);
				VectorClear(v->binormal);
				VectorClear(v->normal);
			}

			for (j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
			{
				v0 = surf->verts[tri->indexes[0]].position;
				v1 = surf->verts[tri->indexes[1]].position;
				v2 = surf->verts[tri->indexes[2]].position;

				t0 = surf->verts[tri->indexes[0]].texCoords;
				t1 = surf->verts[tri->indexes[1]].texCoords;
				t2 = surf->verts[tri->indexes[2]].texCoords;

#if 1
				R_CalcTangentSpace(tangent, binormal, normal, v0, v1, v2, t0, t1, t2);
#else
				R_CalcNormalForTriangle(normal, v0, v1, v2);
				R_CalcTangentsForTriangle(tangent, binormal, v0, v1, v2, t0, t1, t2);
#endif

				for (k = 0; k < 3; k++)
				{
					float *v;

					v = surf->verts[tri->indexes[k]].tangent;
					VectorAdd(v, tangent, v);

					v = surf->verts[tri->indexes[k]].binormal;
					VectorAdd(v, binormal, v);

					v = surf->verts[tri->indexes[k]].normal;
					VectorAdd(v, normal, v);
				}
			}

			for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				VectorNormalize(v->tangent);
				VectorNormalize(v->binormal);
				VectorNormalize(v->normal);
			}
		}
#else
		{
			int         k;
			float       bb, s, t;
			vec3_t      bary;
			vec3_t      faceNormal;
			md5Vertex_t *dv[3];

			for (j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
			{
				dv[0] = &surf->verts[tri->indexes[0]];
				dv[1] = &surf->verts[tri->indexes[1]];
				dv[2] = &surf->verts[tri->indexes[2]];

				R_CalcNormalForTriangle(faceNormal, dv[0]->position, dv[1]->position, dv[2]->position);

				// calculate barycentric basis for the triangle
				bb = (dv[1]->texCoords[0] - dv[0]->texCoords[0]) * (dv[2]->texCoords[1] - dv[0]->texCoords[1]) - (dv[2]->texCoords[0] - dv[0]->texCoords[0]) * (dv[1]->texCoords[1] -
				                                                                                                                                                dv[0]->texCoords[1]);

				if (Q_fabs(bb) < 0.00000001f)
				{
					continue;
				}

				// do each vertex
				for (k = 0; k < 3; k++)
				{
					// calculate s tangent vector
					s       = dv[k]->texCoords[0] + 10.0f;
					t       = dv[k]->texCoords[1];
					bary[0] = ((dv[1]->texCoords[0] - s) * (dv[2]->texCoords[1] - t) - (dv[2]->texCoords[0] - s) * (dv[1]->texCoords[1] - t)) / bb;
					bary[1] = ((dv[2]->texCoords[0] - s) * (dv[0]->texCoords[1] - t) - (dv[0]->texCoords[0] - s) * (dv[2]->texCoords[1] - t)) / bb;
					bary[2] = ((dv[0]->texCoords[0] - s) * (dv[1]->texCoords[1] - t) - (dv[1]->texCoords[0] - s) * (dv[0]->texCoords[1] - t)) / bb;

					dv[k]->tangent[0] = bary[0] * dv[0]->position[0] + bary[1] * dv[1]->position[0] + bary[2] * dv[2]->position[0];
					dv[k]->tangent[1] = bary[0] * dv[0]->position[1] + bary[1] * dv[1]->position[1] + bary[2] * dv[2]->position[1];
					dv[k]->tangent[2] = bary[0] * dv[0]->position[2] + bary[1] * dv[1]->position[2] + bary[2] * dv[2]->position[2];

					VectorSubtract(dv[k]->tangent, dv[k]->position, dv[k]->tangent);
					VectorNormalize(dv[k]->tangent);

					// calculate t tangent vector (binormal)
					s       = dv[k]->texCoords[0];
					t       = dv[k]->texCoords[1] + 10.0f;
					bary[0] = ((dv[1]->texCoords[0] - s) * (dv[2]->texCoords[1] - t) - (dv[2]->texCoords[0] - s) * (dv[1]->texCoords[1] - t)) / bb;
					bary[1] = ((dv[2]->texCoords[0] - s) * (dv[0]->texCoords[1] - t) - (dv[0]->texCoords[0] - s) * (dv[2]->texCoords[1] - t)) / bb;
					bary[2] = ((dv[0]->texCoords[0] - s) * (dv[1]->texCoords[1] - t) - (dv[1]->texCoords[0] - s) * (dv[0]->texCoords[1] - t)) / bb;

					dv[k]->binormal[0] = bary[0] * dv[0]->position[0] + bary[1] * dv[1]->position[0] + bary[2] * dv[2]->position[0];
					dv[k]->binormal[1] = bary[0] * dv[0]->position[1] + bary[1] * dv[1]->position[1] + bary[2] * dv[2]->position[1];
					dv[k]->binormal[2] = bary[0] * dv[0]->position[2] + bary[1] * dv[1]->position[2] + bary[2] * dv[2]->position[2];

					VectorSubtract(dv[k]->binormal, dv[k]->position, dv[k]->binormal);
					VectorNormalize(dv[k]->binormal);

					// calculate the normal as cross product N=TxB
#if 0
					CrossProduct(dv[k]->tangent, dv[k]->binormal, dv[k]->normal);
					VectorNormalize(dv[k]->normal);

					// Gram-Schmidt orthogonalization process for B
					// compute the cross product B=NxT to obtain
					// an orthogonal basis
					CrossProduct(dv[k]->normal, dv[k]->tangent, dv[k]->binormal);

					if (DotProduct(dv[k]->normal, faceNormal) < 0)
					{
						VectorInverse(dv[k]->normal);
						//VectorInverse(dv[k]->tangent);
						//VectorInverse(dv[k]->binormal);
					}

#else
					VectorAdd(dv[k]->normal, faceNormal, dv[k]->normal);
#endif
				}
			}

#if 1
			for (j = 0, v = surf->verts; j < surf->numVerts; j++, v++)
			{
				//VectorNormalize(v->tangent);
				//VectorNormalize(v->binormal);
				VectorNormalize(v->normal);
			}
#endif
		}
#endif

		if (r_smoothNormals->integer & FLAGS_SMOOTH_MD5) // do another extra smoothing for normals to avoid flat shading
		{
			// FIXME: this doesn't work properly - see head of players in game
			for (j = 0; j < surf->numVerts; j++)
			{
				for (k = 0; k < surf->numVerts; k++)
				{
					if (j == k)
					{
						continue;
					}

					if (VectorCompare(surf->verts[j].position, surf->verts[k].position))
					{
						VectorAdd(surf->verts[j].normal, surf->verts[k].normal, surf->verts[j].normal);
					}
				}

				VectorNormalize(surf->verts[j].normal);
			}
		}
	}

	// split the surfaces into VBO surfaces by the maximum number of GPU vertex skinning bones
	Com_InitGrowList(&vboSurfaces, 32);

	for (i = 0, surf = md5->surfaces; i < md5->numSurfaces; i++, surf++)
	{
		// sort triangles
		Com_InitGrowList(&sortedTriangles, 1000);

		for (j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++)
		{
			skelTriangle_t *sortTri = Com_Allocate(sizeof(*sortTri));

			for (k = 0; k < 3; k++)
			{
				sortTri->indexes[k]  = tri->indexes[k];
				sortTri->vertexes[k] = &surf->verts[tri->indexes[k]];
			}

			sortTri->referenced = qfalse;

			Com_AddToGrowList(&sortedTriangles, sortTri);
		}

		//qsort(sortedTriangles.elements, sortedTriangles.currentElements, sizeof(void *), CompareTrianglesByBoneReferences);

#if 0
		for (j = 0; j < sortedTriangles.currentElements; j++)
		{
			int b[MAX_WEIGHTS * 3];

			skelTriangle_t *sortTri = Com_GrowListElement(&sortedTriangles, j);

			for (k = 0; k < 3; k++)
			{
				v = sortTri->vertexes[k];

				for (l = 0; l < MAX_WEIGHTS; l++)
				{
					b[k * 3 + l] = (l < v->numWeights) ? v->weights[l]->boneIndex : 9999;
				}

				qsort(b, MAX_WEIGHTS * 3, sizeof(int), CompareBoneIndices);
				//Ren_Print("bone indices: %i %i %i %i\n", b[k * 3 + 0], b[k * 3 + 1], b[k * 3 + 2], b[k * 3 + 3]);
			}
		}
#endif

		numRemaining = sortedTriangles.currentElements;

		while (numRemaining)
		{
			numBoneReferences = 0;
			Com_Memset(boneReferences, 0, sizeof(boneReferences));

			Com_InitGrowList(&vboTriangles, 1000);

			for (j = 0; j < sortedTriangles.currentElements; j++)
			{
				skelTriangle_t *sortTri = Com_GrowListElement(&sortedTriangles, j);

				if (sortTri->referenced)
				{
					continue;
				}

				if (AddTriangleToVBOTriangleList(&vboTriangles, sortTri, &numBoneReferences, boneReferences))
				{
					sortTri->referenced = qtrue;
				}
			}

			if (!vboTriangles.currentElements)
			{
				Ren_Warning("R_LoadMD5: could not add triangles to a remaining VBO surfaces for model '%s'\n", modName);
				Com_DestroyGrowList(&vboTriangles);
				break;
			}

			AddSurfaceToVBOSurfacesList(&vboSurfaces, &vboTriangles, md5, surf, i, numBoneReferences, boneReferences);
			numRemaining -= vboTriangles.currentElements;

			Com_DestroyGrowList(&vboTriangles);
		}

		for (j = 0; j < sortedTriangles.currentElements; j++)
		{
			skelTriangle_t *sortTri = Com_GrowListElement(&sortedTriangles, j);

			Com_Dealloc(sortTri);
		}

		Com_DestroyGrowList(&sortedTriangles);
	}

	// move VBO surfaces list to hunk
	md5->numVBOSurfaces = vboSurfaces.currentElements;
	md5->vboSurfaces    = ri.Hunk_Alloc(md5->numVBOSurfaces * sizeof(*md5->vboSurfaces), h_low);

	for (i = 0; i < md5->numVBOSurfaces; i++)
	{
		md5->vboSurfaces[i] = ( srfVBOMD5Mesh_t * ) Com_GrowListElement(&vboSurfaces, i);
	}

	Com_DestroyGrowList(&vboSurfaces);

	return qtrue;
}
