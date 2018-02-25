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
 * @file rendererGLES/tr_animation_mds.c
 */

#include "tr_local.h"

/*
All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.
*/

//#define HIGH_PRECISION_BONES	// enable this for 32bit precision bones
//#define DBG_PROFILE_BONES

//-----------------------------------------------------------------------------
// Static Vars, ugly but easiest (and fastest) means of seperating RB_SurfaceAnim
// and R_CalcBones

static float                    frontlerp, backlerp;
static float                    torsoFrontlerp, torsoBacklerp;
static int                      *triangles, *boneRefs;
static int                      indexes;
static glIndex_t                *pIndexes;
static int                      baseIndex, baseVertex, oldIndexes;
static int                      numVerts;
static mdsVertex_t              *v;
static mdsBoneFrame_t           bones[MDS_MAX_BONES], rawBones[MDS_MAX_BONES], oldBones[MDS_MAX_BONES];
static char                     validBones[MDS_MAX_BONES];
static char                     newBones[MDS_MAX_BONES];
static mdsBoneFrame_t           *bonePtr, *bone, *parentBone;
static mdsBoneFrameCompressed_t *cBonePtr, *cTBonePtr, *cOldBonePtr, *cOldTBonePtr, *cBoneList, *cOldBoneList, *cBoneListTorso, *cOldBoneListTorso;
static mdsBoneInfo_t            *boneInfo, *thisBoneInfo, *parentBoneInfo;
static mdsFrame_t               *frame, *torsoFrame;
static mdsFrame_t               *oldFrame, *oldTorsoFrame;
static int                      frameSize;
static short                    *sh, *sh2;
static float                    *pf;
static vec3_t                   angles, tangles, torsoParentOffset, torsoAxis[3], tmpAxis[3];
static float                    *tempVert, *tempNormal;
static vec3_t                   vec, v2, dir;
static float                    diff, a1, a2;
static int                      render_count;
static float                    lodRadius, lodScale;
static int                      *collapse_map, *pCollapseMap;
static int                      collapse[MDS_MAX_VERTS], *pCollapse;
static int                      p0, p1, p2;
static qboolean                 isTorso, fullTorso;
static vec4_t                   m1[4], m2[4];
static vec3_t                   t;
static refEntity_t              lastBoneEntity;

static int totalrv, totalrt, totalv, totalt;

//-----------------------------------------------------------------------------

/**
 * @brief RB_ProjectRadius
 * @param[in] r
 * @param[in] location
 * @return
 */
static float RB_ProjectRadius(float r, vec3_t location)
{
	float  pr;
	float  dist;
	float  c;
	vec3_t p;
	float  projected[4];

	c    = DotProduct(backEnd.viewParms.orientation.axis[0], backEnd.viewParms.orientation.origin);
	dist = DotProduct(backEnd.viewParms.orientation.axis[0], location) - c;

	if (dist <= 0)
	{
		return 0;
	}

	p[0] = 0;
	p[1] = Q_fabs(r);
	p[2] = -dist;

	projected[0] = p[0] * backEnd.viewParms.projectionMatrix[0] +
	               p[1] * backEnd.viewParms.projectionMatrix[4] +
	               p[2] * backEnd.viewParms.projectionMatrix[8] +
	               backEnd.viewParms.projectionMatrix[12];

	projected[1] = p[0] * backEnd.viewParms.projectionMatrix[1] +
	               p[1] * backEnd.viewParms.projectionMatrix[5] +
	               p[2] * backEnd.viewParms.projectionMatrix[9] +
	               backEnd.viewParms.projectionMatrix[13];

	projected[2] = p[0] * backEnd.viewParms.projectionMatrix[2] +
	               p[1] * backEnd.viewParms.projectionMatrix[6] +
	               p[2] * backEnd.viewParms.projectionMatrix[10] +
	               backEnd.viewParms.projectionMatrix[14];

	projected[3] = p[0] * backEnd.viewParms.projectionMatrix[3] +
	               p[1] * backEnd.viewParms.projectionMatrix[7] +
	               p[2] * backEnd.viewParms.projectionMatrix[11] +
	               backEnd.viewParms.projectionMatrix[15];


	pr = projected[1] / projected[3];

	if (pr > 1.0f)
	{
		pr = 1.0f;
	}

	return pr;
}

/**
 * @brief R_CullModel
 * @param[in] header
 * @param[in] ent
 * @return
 */
static int R_CullModel(mdsHeader_t *header, trRefEntity_t *ent)
{
	vec3_t bounds[2];
	int    i;
	int    frameSize = (int) (sizeof(mdsFrame_t) - sizeof(mdsBoneFrameCompressed_t) + header->numBones * sizeof(mdsBoneFrameCompressed_t));
	// compute frame pointers
	mdsFrame_t *newFrame = ( mdsFrame_t * )(( byte * ) header + header->ofsFrames + ent->e.frame * frameSize);
	mdsFrame_t *oldFrame = ( mdsFrame_t * )(( byte * ) header + header->ofsFrames + ent->e.oldframe * frameSize);

	// cull bounding sphere ONLY if this is not an upscaled entity
	if (!ent->e.nonNormalizedAxes)
	{
		if (ent->e.frame == ent->e.oldframe)
		{
			switch (R_CullLocalPointAndRadius(newFrame->localOrigin, newFrame->radius))
			{
			case CULL_OUT:
				tr.pc.c_sphere_cull_md3_out++;
				return CULL_OUT;

			case CULL_IN:
				tr.pc.c_sphere_cull_md3_in++;
				return CULL_IN;

			case CULL_CLIP:
				tr.pc.c_sphere_cull_md3_clip++;
				break;
			}
		}
		else
		{
			int sphereCull, sphereCullB;

			sphereCull = R_CullLocalPointAndRadius(newFrame->localOrigin, newFrame->radius);
			if (newFrame == oldFrame)
			{
				sphereCullB = sphereCull;
			}
			else
			{
				sphereCullB = R_CullLocalPointAndRadius(oldFrame->localOrigin, oldFrame->radius);
			}

			if (sphereCull == sphereCullB)
			{
				if (sphereCull == CULL_OUT)
				{
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				}
				else if (sphereCull == CULL_IN)
				{
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				}
				else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	for (i = 0 ; i < 3 ; i++)
	{
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch (R_CullLocalBox(bounds))
	{
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}

/**
 * @brief RB_CalcMDSLod
 * @param[in] refent
 * @param[in] origin
 * @param[in] radius
 * @param[in] modelBias
 * @param[in] modelScale
 * @return
 */
float RB_CalcMDSLod(refEntity_t *refent, vec3_t origin, float radius, float modelBias, float modelScale)
{
	float flod;
	float projectedRadius;

	// compute projected bounding sphere and use that as a criteria for selecting LOD

	projectedRadius = RB_ProjectRadius(radius, origin);
	if (projectedRadius != 0.f)
	{
		float lodScale = r_lodScale->value;   // fudge factor since MDS uses a much smoother method of LOD

		// ri.Printf (PRINT_ALL, "projected radius: %f\n", projectedRadius);
		flod = projectedRadius * lodScale * modelScale;
	}
	else
	{
		// object intersects near view plane, e.g. view weapon
		flod = 1.0f;
	}

	if (refent->reFlags & REFLAG_FORCE_LOD)
	{
		flod *= 0.5f;
	}
	// like reflag_force_lod, but separate for the moment
	if (refent->reFlags & REFLAG_DEAD_LOD)
	{
		flod *= 0.8f;
	}

	flod -= 0.25f * (r_lodBias->value) + modelBias;

	if (flod < 0.0f)
	{
		flod = 0.0f;
	}
	else if (flod > 1.0f)
	{
		flod = 1.0f;
	}

	return flod;
}

/**
 * @brief R_ComputeFogNum
 * @param[in] header
 * @param[in] ent
 * @return
 */
static int R_ComputeFogNum(mdsHeader_t *header, trRefEntity_t *ent)
{
	int        i, j;
	fog_t      *fog;
	mdsFrame_t *mdsFrame;
	vec3_t     localOrigin;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	// FIXME: non-normalized axis issues
	mdsFrame = ( mdsFrame_t * )(( byte * ) header + header->ofsFrames + (sizeof(mdsFrame_t) + sizeof(mdsBoneFrameCompressed_t) * (header->numBones - 1)) * ent->e.frame);
	VectorAdd(ent->e.origin, mdsFrame->localOrigin, localOrigin);
	for (i = 1 ; i < tr.world->numfogs ; i++)
	{
		fog = &tr.world->fogs[i];
		for (j = 0 ; j < 3 ; j++)
		{
			if (localOrigin[j] - mdsFrame->radius >= fog->bounds[1][j])
			{
				break;
			}
			if (localOrigin[j] + mdsFrame->radius <= fog->bounds[0][j])
			{
				break;
			}
		}
		if (j == 3)
		{
			return i;
		}
	}

	return 0;
}

/**
 * @brief R_AddAnimSurfaces
 * @param[in] ent
 */
void R_AddAnimSurfaces(trRefEntity_t *ent)
{
	mdsHeader_t  *header = tr.currentModel->model.mds;
	mdsSurface_t *surface;
	shader_t     *shader = 0;
	int          i, fogNum, cull;
	qboolean     personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal; // don't add third_person objects if not in a portal

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	cull = R_CullModel(header, ent);
	if (cull == CULL_OUT)
	{
		return;
	}

	// set up lighting now that we know we aren't culled
	if (!personalModel || r_shadows->integer > 1)
	{
		R_SetupEntityLighting(&tr.refdef, ent);
	}

	// see if we are in a fog volume
	fogNum = R_ComputeFogNum(header, ent);

	surface = ( mdsSurface_t * )((byte *)header + header->ofsSurfaces);
	for (i = 0 ; i < header->numSurfaces ; i++)
	{

		if (ent->e.customShader)
		{
			shader = R_GetShaderByHandle(ent->e.customShader);
		}
		else if (ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins)
		{
			skin_t *skin;
			int    hash;
			int    j;

			skin = R_GetSkinByHandle(ent->e.customSkin);

			// match the surface name to something in the skin file
			shader = tr.defaultShader;

			if (ent->e.renderfx & RF_BLINK)
			{
				char *s = va("%s_b", surface->name);   // append '_b' for 'blink'
				hash = Com_HashKey(s, strlen(s));
				for (j = 0 ; j < skin->numSurfaces ; j++)
				{
					if (hash != skin->surfaces[j].hash)
					{
						continue;
					}
					if (!strcmp(skin->surfaces[j].name, s))
					{
						shader = skin->surfaces[j].shader;
						break;
					}
				}
			}

			if (shader == tr.defaultShader)        // blink reference in skin was not found
			{
				hash = Com_HashKey(surface->name, sizeof(surface->name));
				for (j = 0 ; j < skin->numSurfaces ; j++)
				{
					// the names have both been lowercased
					if (hash != skin->surfaces[j].hash)
					{
						continue;
					}
					if (!strcmp(skin->surfaces[j].name, surface->name))
					{
						shader = skin->surfaces[j].shader;
						break;
					}
				}
			}

			if (shader == tr.defaultShader)
			{
				Ren_Developer("WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name);
			}
			else if (shader->defaultShader)
			{
				Ren_Developer("WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
			}
		}
		else
		{
			shader = R_GetShaderByHandle(surface->shaderIndex);
		}

		// don't add third_person objects if not viewing through a portal
		if (!personalModel)
		{
			R_AddDrawSurf((void *)surface, shader, fogNum, 0, 0);
		}

		surface = ( mdsSurface_t * )((byte *)surface + surface->ofsEnd);
	}
}

/**
 * @brief LocalMatrixTransformVector
 * @param[in] in
 * @param[in] mat
 * @param[out] out
 */
static ID_INLINE void LocalMatrixTransformVector(vec3_t in, vec3_t mat[3], vec3_t out)
{
	out[0] = in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2];
	out[1] = in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2];
	out[2] = in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2];
}

/*
 * @brief LocalMatrixTransformVectorTranslate
 * @param[in] in
 * @param[in] mat
 * @param[in] tr
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalMatrixTransformVectorTranslate(vec3_t in, vec3_t mat[3], vec3_t tr, vec3_t out)
{
    out[0] = in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2] + tr[0];
    out[1] = in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2] + tr[1];
    out[2] = in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2] + tr[2];
}
*/

/**
 * @brief LocalScaledMatrixTransformVector
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[out] out
 */
static ID_INLINE void LocalScaledMatrixTransformVector(vec3_t in, float s, vec3_t mat[3], vec3_t out)
{
	out[0] = (1.0f - s) * in[0] + s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2]);
	out[1] = (1.0f - s) * in[1] + s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2]);
	out[2] = (1.0f - s) * in[2] + s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2]);
}

/*
 * @brief LocalScaledMatrixTransformVectorTranslate
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[in] tr
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalScaledMatrixTransformVectorTranslate(vec3_t in, float s, vec3_t mat[3], vec3_t tr, vec3_t out)
{
    out[0] = (1.0f - s) * in[0] + s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2] + tr[0]);
    out[1] = (1.0f - s) * in[1] + s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2] + tr[1]);
    out[2] = (1.0f - s) * in[2] + s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2] + tr[2]);
}
*/

/*
 * @brief LocalScaledMatrixTransformVectorFullTranslate
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[in] tr
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalScaledMatrixTransformVectorFullTranslate(vec3_t in, float s, vec3_t mat[3], vec3_t tr, vec3_t out)
{
    out[0] = (1.0f - s) * in[0] + s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2]) + tr[0];
    out[1] = (1.0f - s) * in[1] + s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2]) + tr[1];
    out[2] = (1.0f - s) * in[2] + s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2]) + tr[2];
}
*/

/*
 * @brief LocalAddScaledMatrixTransformVectorFullTranslate
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[in] tr
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalAddScaledMatrixTransformVectorFullTranslate(vec3_t in, float s, vec3_t mat[3], vec3_t tr, vec3_t out)
{
    out[0] += s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2]) + tr[0];
    out[1] += s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2]) + tr[1];
    out[2] += s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2]) + tr[2];
}
*/

/**
 * @brief LocalAddScaledMatrixTransformVectorTranslate
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[in] tr
 * @param[out] out
 */
static ID_INLINE void LocalAddScaledMatrixTransformVectorTranslate(vec3_t in, float s, vec3_t mat[3], vec3_t tr, vec3_t out)
{
	out[0] += s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2] + tr[0]);
	out[1] += s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2] + tr[1]);
	out[2] += s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2] + tr[2]);
}

/*
 * @brief LocalAddScaledMatrixTransformVector
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalAddScaledMatrixTransformVector(vec3_t in, float s, vec3_t mat[3], vec3_t out)
{
    out[0] += s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2]);
    out[1] += s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2]);
    out[2] += s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2]);
}
*/

static float LAVangle;
static float sp, sy, cp, cy;

/**
 * @brief LocalAngleVector
 * @param[in] angles
 * @param[out] forward
 */
static ID_INLINE void LocalAngleVector(vec3_t angles, vec3_t forward)
{
	LAVangle = angles[YAW] * (M_PI * 2 / 360);
	sy       = sin(LAVangle);
	cy       = cos(LAVangle);
	LAVangle = angles[PITCH] * (M_PI * 2 / 360);
	sp       = sin(LAVangle);
	cp       = cos(LAVangle);

	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
}

/**
 * @brief LocalVectorMA
 * @param[in] org
 * @param[in] dist
 * @param[in] vec
 * @param[out] out
 */
static ID_INLINE void LocalVectorMA(vec3_t org, float dist, vec3_t vec, vec3_t out)
{
	out[0] = org[0] + dist * vec[0];
	out[1] = org[1] + dist * vec[1];
	out[2] = org[2] + dist * vec[2];
}

#define ANGLES_SHORT_TO_FLOAT(pf, sh)     { *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); }

/**
 * @brief SLerp_Normal
 * @param[in] from
 * @param[in] to
 * @param[in] tt
 * @param[out] out
 */
static ID_INLINE void SLerp_Normal(vec3_t from, vec3_t to, float tt, vec3_t out)
{
	float ft = 1.0f - tt;

	out[0] = from[0] * ft + to[0] * tt;
	out[1] = from[1] * ft + to[1] * tt;
	out[2] = from[2] * ft + to[2] * tt;

	VectorNormalize(out);
}

/*
===============================================================================
4x4 Matrices
===============================================================================
*/

/*
 * @brief Matrix4Multiply
 * @param[in] a
 * @param[in] b
 * @param[out] dst
 *
 * @note Unused
static ID_INLINE void Matrix4Multiply(const vec4_t a[4], const vec4_t b[4], vec4_t dst[4])
{
    dst[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
    dst[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
    dst[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
    dst[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

    dst[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
    dst[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
    dst[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
    dst[1][3] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

    dst[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
    dst[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
    dst[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
    dst[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];

    dst[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];
    dst[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];
    dst[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];
    dst[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];
}
*/

/**
 * @brief Matrix4MultiplyInto3x3AndTranslation
 * @param[in] a
 * @param[in] b
 * @param[out] dst
 * @param[out] t
 *
 * @note const usage would require an explicit cast, non ANSI C
 * @see unix/const-arg.c
 */
static ID_INLINE void Matrix4MultiplyInto3x3AndTranslation(/*const*/ vec4_t a[4], /*const*/ vec4_t b[4], vec3_t dst[3], vec3_t t)
{
	dst[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
	dst[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
	dst[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
	t[0]      = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

	dst[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
	dst[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
	dst[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
	t[1]      = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

	dst[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
	dst[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
	dst[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
	t[2]      = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];
}

/*
 * @brief Matrix4Transpose
 * @param[in] matrix
 * @param[out] transpose
 *
 * @note Unused
static ID_INLINE void Matrix4Transpose(const vec4_t matrix[4], vec4_t transpose[4])
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            transpose[i][j] = matrix[j][i];
        }
    }
}
*/

/*
 * @brief Matrix4FromAxis
 * @param[in] axis
 * @param[out] dst
 *
 * @note Unused
static ID_INLINE void Matrix4FromAxis(const vec3_t axis[3], vec4_t dst[4])
{
    int i, j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            dst[i][j] = axis[i][j];
        }
        dst[3][i] = 0;
        dst[i][3] = 0;
    }
    dst[3][3] = 1;
}
*/

/*
 * @brief Matrix4FromScaledAxis
 * @param[in] axis
 * @param[in] scale
 * @param[out] dst
 *
 * @note Unused
static ID_INLINE void Matrix4FromScaledAxis(const vec3_t axis[3], const float scale, vec4_t dst[4])
{
    int i, j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            dst[i][j] = scale * axis[i][j];
            if (i == j)
            {
                dst[i][j] += 1.0f - scale;
            }
        }
        dst[3][i] = 0;
        dst[i][3] = 0;
    }
    dst[3][3] = 1;
}
*/

/*
 * @brief Matrix4FromTranslation
 * @param[in] t
 * @param[out] dst
 *
 * @note Unused
static ID_INLINE void Matrix4FromTranslation(const vec3_t t, vec4_t dst[4])
{
    int i, j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (i == j)
            {
                dst[i][j] = 1;
            }
            else
            {
                dst[i][j] = 0;
            }
        }
        dst[i][3] = t[i];
        dst[3][i] = 0;
    }
    dst[3][3] = 1;
}
*/

/**
 * @brief Can put an axis rotation followed by a translation directly into one matrix
 * @param[in] axis
 * @param[in] t
 * @param[out] dst
 *
 * @note const usage would require an explicit cast, non ANSI C
 *
 * @see unix/const-arg.c
 */
static ID_INLINE void Matrix4FromAxisPlusTranslation(/*const*/ vec3_t axis[3], const vec3_t t, vec4_t dst[4])
{
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			dst[i][j] = axis[i][j];
		}
		dst[3][i] = 0;
		dst[i][3] = t[i];
	}
	dst[3][3] = 1;
}

/**
 * @brief Can put a scaled axis rotation followed by a translation directly into one matrix
 * @param[in] axis
 * @param[in] scale
 * @param[in] t
 * @param[out] dst
 *
 * @note const usage would require an explicit cast, non ANSI C
 *
 * @see unix/const-arg.c
 */
static ID_INLINE void Matrix4FromScaledAxisPlusTranslation(/*const*/ vec3_t axis[3], const float scale, const vec3_t t, vec4_t dst[4])
{
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			dst[i][j] = scale * axis[i][j];
			if (i == j)
			{
				dst[i][j] += 1.0f - scale;
			}
		}
		dst[3][i] = 0;
		dst[i][3] = t[i];
	}
	dst[3][3] = 1;
}

/*
 * @brief Matrix4FromScale
 * @param[in] scale
 * @param[out ] dst
 *
 * @note Unused
static ID_INLINE void Matrix4FromScale(const float scale, vec4_t dst[4])
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (i == j)
            {
                dst[i][j] = scale;
            }
            else
            {
                dst[i][j] = 0;
            }
        }
    }
    dst[3][3] = 1;
}
*/

/*
 * @brief Matrix4TransformVector
 * @param[in] m
 * @param[in] src
 * @param[out] dst
 *
 * @note Unused
static ID_INLINE void Matrix4TransformVector(const vec4_t m[4], const vec3_t src, vec3_t dst)
{
    dst[0] = m[0][0] * src[0] + m[0][1] * src[1] + m[0][2] * src[2] + m[0][3];
    dst[1] = m[1][0] * src[0] + m[1][1] * src[1] + m[1][2] * src[2] + m[1][3];
    dst[2] = m[2][0] * src[0] + m[2][1] * src[1] + m[2][2] * src[2] + m[2][3];
}
*/

/*
===============================================================================
3x3 Matrices
===============================================================================
*/

/**
 * @brief Matrix3Transpose
 * @param[in] matrix
 * @param[out] transpose
 */
static ID_INLINE void Matrix3Transpose(const vec3_t matrix[3], vec3_t transpose[3])
{
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			transpose[i][j] = matrix[j][i];
		}
	}
}

/**
 * @brief R_CalcBone
 * @param[in] header
 * @param refent - unused
 * @param[in] boneNum
 */
void R_CalcBone(mdsHeader_t *header, const refEntity_t *refent, int boneNum)
{
	thisBoneInfo = &boneInfo[boneNum];
	if (thisBoneInfo->torsoWeight != 0.f)
	{
		cTBonePtr = &cBoneListTorso[boneNum];
		isTorso   = qtrue;
		if (thisBoneInfo->torsoWeight == 1.0f)
		{
			fullTorso = qtrue;
		}
	}
	else
	{
		isTorso   = qfalse;
		fullTorso = qfalse;
	}
	cBonePtr = &cBoneList[boneNum];

	bonePtr = &bones[boneNum];

	// we can assume the parent has already been uncompressed for this frame + lerp
	if (thisBoneInfo->parent >= 0)
	{
		parentBone     = &bones[thisBoneInfo->parent];
		parentBoneInfo = &boneInfo[thisBoneInfo->parent];
	}
	else
	{
		parentBone     = NULL;
		parentBoneInfo = NULL;
	}

#ifdef HIGH_PRECISION_BONES
	// rotation
	if (fullTorso)
	{
		VectorCopy(cTBonePtr->angles, angles);
	}
	else
	{
		VectorCopy(cBonePtr->angles, angles);
		if (isTorso)
		{
			VectorCopy(cTBonePtr->angles, tangles);
			// blend the angles together
			for (j = 0; j < 3; j++)
			{
				diff = tangles[j] - angles[j];
				if (Q_fabs(diff) > 180)
				{
					diff = AngleNormalize180(diff);
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}
		}
	}
#else
	// rotation
	if (fullTorso)
	{
		sh = (short *)cTBonePtr->angles;
		pf = angles;
		ANGLES_SHORT_TO_FLOAT(pf, sh);
	}
	else
	{
		sh = (short *)cBonePtr->angles;
		pf = angles;
		ANGLES_SHORT_TO_FLOAT(pf, sh);
		if (isTorso)
		{
			int j;

			sh = (short *)cTBonePtr->angles;
			pf = tangles;
			ANGLES_SHORT_TO_FLOAT(pf, sh);
			// blend the angles together
			for (j = 0; j < 3; j++)
			{
				diff = tangles[j] - angles[j];
				if (Q_fabs(diff) > 180)
				{
					diff = AngleNormalize180(diff);
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}
		}
	}
#endif
	AnglesToAxis(angles, bonePtr->matrix);

	// translation
	if (parentBone)
	{

#ifdef HIGH_PRECISION_BONES
		if (fullTorso)
		{
			angles[0] = cTBonePtr->ofsAngles[0];
			angles[1] = cTBonePtr->ofsAngles[1];
			angles[2] = 0;
			LocalAngleVector(angles, vec);
			LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
		}
		else
		{
			angles[0] = cBonePtr->ofsAngles[0];
			angles[1] = cBonePtr->ofsAngles[1];
			angles[2] = 0;
			LocalAngleVector(angles, vec);

			if (isTorso)
			{
				tangles[0] = cTBonePtr->ofsAngles[0];
				tangles[1] = cTBonePtr->ofsAngles[1];
				tangles[2] = 0;
				LocalAngleVector(tangles, v2);

				// blend the angles together
				SLerp_Normal(vec, v2, thisBoneInfo->torsoWeight, vec);
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);

			}
			else        // legs bone
			{
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
		}
#else
		if (fullTorso)
		{
			sh      = (short *)cTBonePtr->ofsAngles; pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
			LocalAngleVector(angles, vec);
			LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
		}
		else
		{
			sh      = (short *)cBonePtr->ofsAngles; pf = angles;
			*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
			LocalAngleVector(angles, vec);

			if (isTorso)
			{
				sh      = (short *)cTBonePtr->ofsAngles;
				pf      = tangles;
				*(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = SHORT2ANGLE(*(sh++)); *(pf++) = 0;
				LocalAngleVector(tangles, v2);

				// blend the angles together
				SLerp_Normal(vec, v2, thisBoneInfo->torsoWeight, vec);
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);

			}
			else        // legs bone
			{
				LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
		}
#endif
	}
	else        // just use the frame position
	{
		bonePtr->translation[0] = frame->parentOffset[0];
		bonePtr->translation[1] = frame->parentOffset[1];
		bonePtr->translation[2] = frame->parentOffset[2];
	}

	if (boneNum == header->torsoParent)     // this is the torsoParent
	{
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}

	validBones[boneNum] = 1;

	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;

}

/**
 * @brief R_CalcBoneLerp
 * @param[in] header
 * @param[in] refent
 * @param[in] boneNum
 */
void R_CalcBoneLerp(mdsHeader_t *header, const refEntity_t *refent, int boneNum)
{
	if (!refent || !header || boneNum < 0 || boneNum >= MDS_MAX_BONES)
	{
		return;
	}

	thisBoneInfo = &boneInfo[boneNum];

	if (!thisBoneInfo)
	{
		return;
	}

	if (thisBoneInfo->parent >= 0)
	{
		parentBone     = &bones[thisBoneInfo->parent];
		parentBoneInfo = &boneInfo[thisBoneInfo->parent];
	}
	else
	{
		parentBone     = NULL;
		parentBoneInfo = NULL;
	}

	if (thisBoneInfo->torsoWeight != 0.f)
	{
		cTBonePtr    = &cBoneListTorso[boneNum];
		cOldTBonePtr = &cOldBoneListTorso[boneNum];
		isTorso      = qtrue;
		if (thisBoneInfo->torsoWeight == 1.0f)
		{
			fullTorso = qtrue;
		}
	}
	else
	{
		isTorso   = qfalse;
		fullTorso = qfalse;
	}
	cBonePtr    = &cBoneList[boneNum];
	cOldBonePtr = &cOldBoneList[boneNum];

	bonePtr = &bones[boneNum];

	newBones[boneNum] = 1;

	// rotation (take into account 170 to -170 lerps, which need to take the shortest route)
	if (fullTorso)
	{
		sh  = (short *)cTBonePtr->angles;
		sh2 = (short *)cOldTBonePtr->angles;
		pf  = angles;

		a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
		a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
		a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;

	}
	else
	{
		sh  = (short *)cBonePtr->angles;
		sh2 = (short *)cOldBonePtr->angles;
		pf  = angles;

		a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;
		a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;
		a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;

		if (isTorso)
		{
			int j;

			sh  = (short *)cTBonePtr->angles;
			sh2 = (short *)cOldTBonePtr->angles;
			pf  = tangles;

			a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;
			a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;
			a1      = SHORT2ANGLE(*(sh++)); a2 = SHORT2ANGLE(*(sh2++)); diff = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;

			// blend the angles together
			for (j = 0; j < 3; j++)
			{
				diff = tangles[j] - angles[j];
				if (Q_fabs(diff) > 180)
				{
					diff = AngleNormalize180(diff);
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}

		}

	}
	AnglesToAxis(angles, bonePtr->matrix);

	if (parentBone)
	{
		if (fullTorso)
		{
			sh  = (short *)cTBonePtr->ofsAngles;
			sh2 = (short *)cOldTBonePtr->ofsAngles;
		}
		else
		{
			sh  = (short *)cBonePtr->ofsAngles;
			sh2 = (short *)cOldBonePtr->ofsAngles;
		}

		pf      = angles;
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = 0;
		LocalAngleVector(angles, v2);       // new

		pf      = angles;
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = 0;
		LocalAngleVector(angles, vec);      // old

		// blend the angles together
		if (fullTorso)
		{
			SLerp_Normal(vec, v2, torsoFrontlerp, dir);
		}
		else
		{
			SLerp_Normal(vec, v2, frontlerp, dir);
		}

		// translation
		if (!fullTorso && isTorso)        // partial legs/torso, need to lerp according to torsoWeight
		{   // calc the torso frame
			sh  = (short *)cTBonePtr->ofsAngles;
			sh2 = (short *)cOldTBonePtr->ofsAngles;

			pf      = angles;
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = 0;
			LocalAngleVector(angles, v2);       // new

			pf      = angles;
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = 0;
			LocalAngleVector(angles, vec);      // old

			// blend the angles together
			SLerp_Normal(vec, v2, torsoFrontlerp, v2);

			// blend the torso/legs together
			SLerp_Normal(dir, v2, thisBoneInfo->torsoWeight, dir);

		}

		LocalVectorMA(parentBone->translation, thisBoneInfo->parentDist, dir, bonePtr->translation);
	}
	else        // just interpolate the frame positions
	{
		bonePtr->translation[0] = frontlerp * frame->parentOffset[0] + backlerp * oldFrame->parentOffset[0];
		bonePtr->translation[1] = frontlerp * frame->parentOffset[1] + backlerp * oldFrame->parentOffset[1];
		bonePtr->translation[2] = frontlerp * frame->parentOffset[2] + backlerp * oldFrame->parentOffset[2];
	}

	if (boneNum == header->torsoParent)     // this is the torsoParent
	{
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}
	validBones[boneNum] = 1;

	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;
}

/**
 * @brief R_CalcBones
 * @param[in] header
 * @param[in] refent
 * @param[in,out] boneList This list should only be built and modified from within here
 * @param[in] numBones
 */
void R_CalcBones(mdsHeader_t *header, const refEntity_t *refent, int *boneList, int numBones)
{
	int   i;
	int   *boneRefs;
	float torsoWeight;

	// if the entity has changed since the last time the bones were built, reset them
	if (memcmp(&lastBoneEntity, refent, sizeof(refEntity_t)))
	{
		// different, cached bones are not valid
		Com_Memset(validBones, 0, header->numBones);
		lastBoneEntity = *refent;

		// (SA) also reset these counter statics
		//----(SA)	print stats for the complete model (not per-surface)
		if (r_bonesDebug->integer == 4 && totalrt)
		{
			Ren_Print("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n",
			          lodScale,
			          totalrv,
			          totalv,
			          totalrt,
			          totalt,
			          ( float )(100.0 * totalrt) / (float) totalt);
		}
		totalrv = totalrt = totalv = totalt = 0;
	}

	Com_Memset(newBones, 0, header->numBones);

	if (refent->oldframe == refent->frame)
	{
		backlerp  = 0;
		frontlerp = 1;
	}
	else
	{
		backlerp  = refent->backlerp;
		frontlerp = 1.0f - backlerp;
	}

	if (refent->oldTorsoFrame == refent->torsoFrame)
	{
		torsoBacklerp  = 0;
		torsoFrontlerp = 1;
	}
	else
	{
		torsoBacklerp  = refent->torsoBacklerp;
		torsoFrontlerp = 1.0f - torsoBacklerp;
	}

	frameSize = (int) (sizeof(mdsFrame_t) + (header->numBones - 1) * sizeof(mdsBoneFrameCompressed_t));

	frame = ( mdsFrame_t * )((byte *)header + header->ofsFrames +
	                         refent->frame * frameSize);
	torsoFrame = ( mdsFrame_t * )((byte *)header + header->ofsFrames +
	                              refent->torsoFrame * frameSize);
	oldFrame = ( mdsFrame_t * )((byte *)header + header->ofsFrames +
	                            refent->oldframe * frameSize);
	oldTorsoFrame = ( mdsFrame_t * )((byte *)header + header->ofsFrames +
	                                 refent->oldTorsoFrame * frameSize);

	// lerp all the needed bones (torsoParent is always the first bone in the list)
	cBoneList      = frame->bones;
	cBoneListTorso = torsoFrame->bones;

	boneInfo = ( mdsBoneInfo_t * )((byte *)header + header->ofsBones);
	boneRefs = boneList;
	//
	Matrix3Transpose(refent->torsoAxis, torsoAxis);

#ifdef HIGH_PRECISION_BONES
	if (qtrue)
	{
#else
	if (backlerp == 0.f && torsoBacklerp == 0.f)
	{
#endif
		for (i = 0; i < numBones; i++, boneRefs++)
		{
			if (validBones[*boneRefs])
			{
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ((boneInfo[*boneRefs].parent >= 0) && (!validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent]))
			{
				R_CalcBone(header, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBone(header, refent, *boneRefs);
		}
	}
	else        // interpolated
	{
		cOldBoneList      = oldFrame->bones;
		cOldBoneListTorso = oldTorsoFrame->bones;

		for (i = 0; i < numBones; i++, boneRefs++)
		{
			if (validBones[*boneRefs])
			{
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ((boneInfo[*boneRefs].parent >= 0) && (!validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent]))
			{
				R_CalcBoneLerp(header, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBoneLerp(header, refent, *boneRefs);
		}
	}

	// adjust for torso rotations
	torsoWeight = 0;
	boneRefs    = boneList;
	for (i = 0; i < numBones; i++, boneRefs++)
	{
		thisBoneInfo = &boneInfo[*boneRefs];
		bonePtr      = &bones[*boneRefs];
		// add torso rotation
		if (thisBoneInfo->torsoWeight > 0)
		{
			if (!newBones[*boneRefs])
			{
				// just copy it back from the previous calc
				bones[*boneRefs] = oldBones[*boneRefs];
				continue;
			}

			if (!(thisBoneInfo->flags & BONEFLAG_TAG))
			{
				// 1st multiply with the bone->matrix
				// 2nd translation for rotation relative to bone around torso parent offset
				VectorSubtract(bonePtr->translation, torsoParentOffset, t);
				Matrix4FromAxisPlusTranslation(bonePtr->matrix, t, m1);
				// 3rd scaled rotation
				// 4th translate back to torso parent offset
				// use previously created matrix if available for the same weight
				if (torsoWeight != thisBoneInfo->torsoWeight)
				{
					Matrix4FromScaledAxisPlusTranslation(torsoAxis, thisBoneInfo->torsoWeight, torsoParentOffset, m2);
					torsoWeight = thisBoneInfo->torsoWeight;
				}
				// multiply matrices to create one matrix to do all calculations
				Matrix4MultiplyInto3x3AndTranslation(m2, m1, bonePtr->matrix, bonePtr->translation);

			}
			else        // tag's require special handling
			{   // rotate each of the axis by the torsoAngles
				LocalScaledMatrixTransformVector(bonePtr->matrix[0], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[0]);
				LocalScaledMatrixTransformVector(bonePtr->matrix[1], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[1]);
				LocalScaledMatrixTransformVector(bonePtr->matrix[2], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[2]);
				Com_Memcpy(bonePtr->matrix, tmpAxis, sizeof(tmpAxis));

				// rotate the translation around the torsoParent
				VectorSubtract(bonePtr->translation, torsoParentOffset, t);
				LocalScaledMatrixTransformVector(t, thisBoneInfo->torsoWeight, torsoAxis, bonePtr->translation);
				VectorAdd(bonePtr->translation, torsoParentOffset, bonePtr->translation);
			}
		}
	}

	// backup the final bones
	Com_Memcpy(oldBones, bones, sizeof(bones[0]) * header->numBones);
}

#ifdef DBG_PROFILE_BONES
#define DBG_SHOWTIME    Ren_Print("%i: %i, ", di++, (dt = ri.Milliseconds()) - ldt); ldt = dt;
#else
#define DBG_SHOWTIME    ;
#endif

/**
 * @brief RB_SurfaceAnim
 * @param[in] surface
 */
void RB_SurfaceAnim(mdsSurface_t *surface)
{
	int         j, k;
	refEntity_t *refent;
	int         *boneList;
	mdsHeader_t *header;

#ifdef DBG_PROFILE_BONES
	int di = 0, dt, ldt;

	dt  = ri.Milliseconds();
	ldt = dt;
#endif

	refent   = &backEnd.currentEntity->e;
	boneList = ( int * )((byte *)surface + surface->ofsBoneReferences);
	header   = ( mdsHeader_t * )((byte *)surface + surface->ofsHeader);

	R_CalcBones(header, (const refEntity_t *)refent, boneList, surface->numBoneReferences);

	DBG_SHOWTIME

	// calculate LOD
	// TODO: lerp the radius and origin
	VectorAdd(refent->origin, frame->localOrigin, vec);
	lodRadius = frame->radius;
	lodScale  = RB_CalcMDSLod(refent, vec, lodRadius, header->lodBias, header->lodScale);


//DBG_SHOWTIME

	// modification to allow dead skeletal bodies to go below minlod (experiment)
	if (refent->reFlags & REFLAG_DEAD_LOD)
	{
		if (lodScale < 0.35f)       // allow dead to lod down to 35% (even if below surf->minLod) (%35 is arbitrary and probably not good generally.  worked for the blackguard/infantry as a test though)
		{
			lodScale = 0.35f;
		}
		render_count = round(surface->numVerts * lodScale);

	}
	else
	{
		render_count = round(surface->numVerts * lodScale);
		if (render_count < surface->minLod)
		{
			if (!(refent->reFlags & REFLAG_DEAD_LOD))
			{
				render_count = surface->minLod;
			}
		}
	}

	if (render_count > surface->numVerts)
	{
		render_count = surface->numVerts;
	}

	RB_CheckOverflow(render_count, surface->numTriangles * 3);

//DBG_SHOWTIME

	// setup triangle list

//DBG_SHOWTIME

	collapse_map = ( int * )(( byte * )surface + surface->ofsCollapseMap);
	triangles    = ( int * )((byte *)surface + surface->ofsTriangles);
	indexes      = surface->numTriangles * 3;
	baseIndex    = tess.numIndexes;
	baseVertex   = tess.numVertexes;
	oldIndexes   = baseIndex;

	tess.numVertexes += render_count;

	pIndexes = &tess.indexes[baseIndex];

//DBG_SHOWTIME

	if (render_count == surface->numVerts)
	{
		for (j = 0; j < indexes; j++)
		{
			pIndexes[j] = triangles[j] + baseVertex;
		}
		tess.numIndexes += indexes;
	}
	else
	{
		int *collapseEnd;

		pCollapse = collapse;
		for (j = 0; j < render_count; pCollapse++, j++)
		{
			*pCollapse = j;
		}

		pCollapseMap = &collapse_map[render_count];
		for (collapseEnd = collapse + surface->numVerts ; pCollapse < collapseEnd; pCollapse++, pCollapseMap++)
		{
			*pCollapse = collapse[*pCollapseMap];
		}

		for (j = 0 ; j < indexes ; j += 3)
		{
			p0 = collapse[*(triangles++)];
			p1 = collapse[*(triangles++)];
			p2 = collapse[*(triangles++)];

			// FIXME
			// note:  serious optimization opportunity here,
			//  by sorting the triangles the following "continue"
			//  could have been made into a "break" statement.
			if (p0 == p1 || p1 == p2 || p2 == p0)
			{
				continue;
			}

			*(pIndexes++)    = baseVertex + p0;
			*(pIndexes++)    = baseVertex + p1;
			*(pIndexes++)    = baseVertex + p2;
			tess.numIndexes += 3;
		}

		baseIndex = tess.numIndexes;
	}

//DBG_SHOWTIME

	// deform the vertexes by the lerped bones

	numVerts   = surface->numVerts;
	v          = ( mdsVertex_t * )((byte *)surface + surface->ofsVerts);
	tempVert   = ( float * )(tess.xyz + baseVertex);
	tempNormal = ( float * )(tess.normal + baseVertex);
	for (j = 0; j < render_count; j++, tempVert += 4, tempNormal += 4)
	{
		mdsWeight_t *w;

		VectorClear(tempVert);

		w = v->weights;
		for (k = 0 ; k < v->numWeights ; k++, w++)
		{
			bone = &bones[w->boneIndex];
			LocalAddScaledMatrixTransformVectorTranslate(w->offset, w->boneWeight, bone->matrix, bone->translation, tempVert);
		}

		LocalMatrixTransformVector(v->normal, bones[v->weights[0].boneIndex].matrix, tempNormal);

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (mdsVertex_t *)&v->weights[v->numWeights];
	}

	DBG_SHOWTIME

	if (r_bonesDebug->integer)
	{
		if (r_bonesDebug->integer < 3)
		{
			int i;

			// DEBUG: show the bones as a stick figure with axis at each bone
			boneRefs = ( int * )((byte *)surface + surface->ofsBoneReferences);
			for (i = 0; i < surface->numBoneReferences; i++, boneRefs++)
			{
				bonePtr = &bones[*boneRefs];

				GL_Bind(tr.whiteImage);
				qglLineWidth(1);
#if 0
				// TODO: OpenGL ES renderer
				qglBegin(GL_LINES);
				for (j = 0; j < 3; j++)
				{
					VectorClear(vec);
					vec[j] = 1;
					qglColor3fv(vec);
					qglVertex3fv(bonePtr->translation);
					VectorMA(bonePtr->translation, 5, bonePtr->matrix[j], vec);
					qglVertex3fv(vec);
				}
				qglEnd();
#endif // 0
				// connect to our parent if it's valid
				if (validBones[boneInfo[*boneRefs].parent])
				{
					qglLineWidth(2);
#if 0
					// TODO: OpenGL ES renderer
					qglBegin(GL_LINES);
					qglColor3f(.6f, .6f, .6f);
					qglVertex3fv(bonePtr->translation);
					qglVertex3fv(bones[boneInfo[*boneRefs].parent].translation);
					qglEnd();
#endif
				}

				qglLineWidth(1);
			}
		}

		if (r_bonesDebug->integer == 3 || r_bonesDebug->integer == 4)
		{
			int render_indexes = (tess.numIndexes - oldIndexes);

			// show mesh edges
			tempVert   = ( float * )(tess.xyz + baseVertex);
			tempNormal = ( float * )(tess.normal + baseVertex);

			GL_Bind(tr.whiteImage);
			qglLineWidth(1);
#if 0
			// TODO: OpenGL ES renderer
			qglBegin(GL_LINES);
			qglColor3f(.0f, .0f, .8f);

			pIndexes = &tess.indexes[oldIndexes];
			for (j = 0; j < render_indexes / 3; j++, pIndexes += 3)
			{
				qglVertex3fv(tempVert + 4 * pIndexes[0]);
				qglVertex3fv(tempVert + 4 * pIndexes[1]);

				qglVertex3fv(tempVert + 4 * pIndexes[1]);
				qglVertex3fv(tempVert + 4 * pIndexes[2]);

				qglVertex3fv(tempVert + 4 * pIndexes[2]);
				qglVertex3fv(tempVert + 4 * pIndexes[0]);
			}

			qglEnd();
#endif // 0

			// track debug stats
			if (r_bonesDebug->integer == 4)
			{
				totalrv += render_count;
				totalrt += render_indexes / 3;
				totalv  += surface->numVerts;
				totalt  += surface->numTriangles;
			}

			if (r_bonesDebug->integer == 3)
			{
				Ren_Print("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n", lodScale, render_count, surface->numVerts, render_indexes / 3, surface->numTriangles,
				          (100.0 * render_indexes / 3) / (double) surface->numTriangles);
			}
		}
	}

	if (r_bonesDebug->integer > 1)
	{
		// dont draw the actual surface
		tess.numIndexes  = oldIndexes;
		tess.numVertexes = baseVertex;
		return;
	}

#ifdef DBG_PROFILE_BONES
	Ren_Print("\n");
#endif
}

/**
 * @brief R_RecursiveBoneListAdd
 * @param[in] bi
 * @param[in,out] boneList
 * @param[in,out] numBones
 * @param[in] boneInfoList
 */
void R_RecursiveBoneListAdd(int bi, int *boneList, int *numBones, mdsBoneInfo_t *boneInfoList)
{
	if (boneInfoList[bi].parent >= 0)
	{

		R_RecursiveBoneListAdd(boneInfoList[bi].parent, boneList, numBones, boneInfoList);

	}

	boneList[(*numBones)++] = bi;
}

/**
 * @brief R_GetBoneTag
 * @param[out] outTag
 * @param[in] mds
 * @param[in] startTagIndex
 * @param[in] refent
 * @param[in] tagName
 * @return
 */
int R_GetBoneTag(orientation_t *outTag, mdsHeader_t *mds, int startTagIndex, const refEntity_t *refent, const char *tagName)
{
	int           i;
	mdsTag_t      *pTag;
	mdsBoneInfo_t *boneInfoList;
	int           boneList[MDS_MAX_BONES];
	int           numBones;

	if (startTagIndex > mds->numTags)
	{
		Com_Memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// find the correct tag

	pTag = ( mdsTag_t * )((byte *)mds + mds->ofsTags);

	pTag += startTagIndex;

	for (i = startTagIndex; i < mds->numTags; i++, pTag++)
	{
		if (!strcmp(pTag->name, tagName))
		{
			break;
		}
	}

	if (i >= mds->numTags)
	{
		Com_Memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// now build the list of bones we need to calc to get this tag's bone information

	boneInfoList = ( mdsBoneInfo_t * )((byte *)mds + mds->ofsBones);
	numBones     = 0;

	R_RecursiveBoneListAdd(pTag->boneIndex, boneList, &numBones, boneInfoList);

	// calc the bones

	R_CalcBones((mdsHeader_t *)mds, refent, boneList, numBones);

	// now extract the orientation for the bone that represents our tag

	Com_Memcpy(outTag->axis, bones[pTag->boneIndex].matrix, sizeof(outTag->axis));
	VectorCopy(bones[pTag->boneIndex].translation, outTag->origin);

/* code not functional, not in backend
    if (r_bonesDebug->integer == 4) {
        int j;
        // DEBUG: show the tag position/axis
        GL_Bind( tr.whiteImage );
        qglLineWidth( 2 );
        qglBegin( GL_LINES );
        for (j=0; j<3; j++) {
            VectorClear(vec);
            vec[j] = 1;
            qglColor3fv( vec );
            qglVertex3fv( outTag->origin );
            VectorMA( outTag->origin, 8, outTag->axis[j], vec );
            qglVertex3fv( vec );
        }
        qglEnd();

        qglLineWidth( 1 );
    }
*/

	return i;
}
