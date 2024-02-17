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
 * @file renderer/tr_animation_mdm.c
 *
 * @brief All bones should be an identity orientation to display the mesh exactly
 * as it is specified.
 *
 * For all other frames, the bones represent the transformation from the
 * orientation of the bone in the base frame to the orientation in this
 * frame.
 */

#include "tr_local.h"

// undef to use floating-point lerping with explicit trig funcs
#define YD_INGLES

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
static mdmVertex_t              *v;
static mdxBoneFrame_t           bones[MDX_MAX_BONES], rawBones[MDX_MAX_BONES], oldBones[MDX_MAX_BONES];
static char                     validBones[MDX_MAX_BONES];
static char                     newBones[MDX_MAX_BONES];
static mdxBoneFrame_t           *bonePtr, *bone, *parentBone;
static mdxBoneFrameCompressed_t *cBonePtr, *cTBonePtr, *cOldBonePtr, *cOldTBonePtr, *cBoneList, *cOldBoneList, *cBoneListTorso, *cOldBoneListTorso;
static mdxBoneInfo_t            *boneInfo, *thisBoneInfo, *parentBoneInfo;
static mdxFrame_t               *frame, *torsoFrame;
static mdxFrame_t               *oldFrame, *oldTorsoFrame;
static int                      frameSize;
static short                    *sh, *sh2;
static float                    *pf;
static int                      ingles[3], tingles[3];
static vec3_t                   angles, tangles, torsoParentOffset, torsoAxis[3]; //, tmpAxis[3];	// unused
static float                    *tempVert, *tempNormal;
static vec3_t                   vec, v2, dir;
static float                    diff; //, a1, a2;	// unused
static int                      render_count;
static float                    lodRadius, lodScale;
static int                      *collapse_map, *pCollapseMap;
static int                      collapse[MDM_MAX_VERTS], *pCollapse;
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
 * @param[in] ent
 * @return
 */
static int R_CullModel(trRefEntity_t *ent)
{
	vec3_t      bounds[2];
	mdxHeader_t *oldFrameHeader, *newFrameHeader;
	mdxFrame_t  *oldFrame, *newFrame;
	int         i;

	newFrameHeader = R_GetModelByHandle(ent->e.frameModel)->model.mdx;
	oldFrameHeader = R_GetModelByHandle(ent->e.oldframeModel)->model.mdx;

	if (!newFrameHeader || !oldFrameHeader)
	{
		return CULL_OUT;
	}

	// compute frame pointers
	newFrame = (mdxFrame_t *) ((byte *) newFrameHeader + newFrameHeader->ofsFrames +
	                           ent->e.frame * (int)(sizeof(mdxBoneFrameCompressed_t)) * newFrameHeader->numBones +
	                           ent->e.frame * sizeof(mdxFrame_t));
	oldFrame = (mdxFrame_t *) ((byte *) oldFrameHeader + oldFrameHeader->ofsFrames +
	                           ent->e.oldframe * (int)(sizeof(mdxBoneFrameCompressed_t)) * oldFrameHeader->numBones +
	                           ent->e.oldframe * sizeof(mdxFrame_t));

	// cull bounding sphere ONLY if this is not an upscaled entity
	if (!ent->e.nonNormalizedAxes)
	{
		if (ent->e.frame == ent->e.oldframe && ent->e.frameModel == ent->e.oldframeModel)
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
 * @brief RB_CalcMDMLod
 * @param[in] refent
 * @param[in] origin
 * @param[in] radius
 * @param[in] modelBias
 * @param[in] modelScale
 * @return
 */
static float RB_CalcMDMLod(refEntity_t *refent, vec3_t origin, float radius, float modelBias, float modelScale)
{
	float flod;
	float projectedRadius;

	// compute projected bounding sphere and use that as a criteria for selecting LOD

	projectedRadius = RB_ProjectRadius(radius, origin);
	if (projectedRadius != 0.f)
	{
		// ri.Printf (PRINT_ALL, "projected radius: %f\n", projectedRadius);

		float lodScale = r_lodScale->value;   // fudge factor since MDS uses a much smoother method of LOD

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

	flod -= 0.25f * r_lodBias->value + modelBias;

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
 * @param[in] ent
 * @return
 */
static int R_ComputeFogNum(trRefEntity_t *ent)
{
	int         i, j;
	fog_t       *fog;
	mdxHeader_t *header;
	mdxFrame_t  *mdxFrame;
	vec3_t      localOrigin;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return 0;
	}

	header = R_GetModelByHandle(ent->e.frameModel)->model.mdx;

	// compute frame pointers
	mdxFrame = ( mdxFrame_t * )(( byte * ) header + header->ofsFrames +
	                            ent->e.frame * (int) (sizeof(mdxBoneFrameCompressed_t)) * header->numBones +
	                            ent->e.frame * sizeof(mdxFrame_t));

	// FIXME: non-normalized axis issues
	VectorAdd(ent->e.origin, mdxFrame->localOrigin, localOrigin);
	for (i = 1 ; i < tr.world->numfogs ; i++)
	{
		fog = &tr.world->fogs[i];
		for (j = 0 ; j < 3 ; j++)
		{
			if (localOrigin[j] - mdxFrame->radius >= fog->bounds[1][j])
			{
				break;
			}
			if (localOrigin[j] + mdxFrame->radius <= fog->bounds[0][j])
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
 * @brief R_MDM_AddAnimSurfaces
 * @param[in] ent
 */
void R_MDM_AddAnimSurfaces(trRefEntity_t *ent)
{
	mdmHeader_t  *header = tr.currentModel->model.mdm;
	mdmSurface_t *surface;
	shader_t     *shader = 0;
	int          i, fogNum, cull;
	qboolean     personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal; // don't add third_person objects if not in a portal

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	cull = R_CullModel(ent);
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
	fogNum = R_ComputeFogNum(ent);

	surface = ( mdmSurface_t * )((byte *)header + header->ofsSurfaces);
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

			if (shader == tr.defaultShader)      // blink reference in skin was not found
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

		surface = ( mdmSurface_t * )((byte *)surface + surface->ofsEnd);
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

/*
 * @brief LocalScaledMatrixTransformVector
 * @param[in] in
 * @param[in] s
 * @param[in] mat
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalScaledMatrixTransformVector(vec3_t in, float s, vec3_t mat[3], vec3_t out)
{
    out[0] = (1.0f - s) * in[0] + s * (in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2]);
    out[1] = (1.0f - s) * in[1] + s * (in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2]);
    out[2] = (1.0f - s) * in[2] + s * (in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2]);
}
*/

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

// static float LAVangle;
static float sp, sy, cp, cy, sr, cr;

/*
 * @brief LocalAngleVector
 * @param[in] angles
 * @param[out] forward
 *
 * @note Unused
static ID_INLINE void LocalAngleVector(vec3_t angles, vec3_t forward)
{
    LAVangle = angles[YAW] * (M_TAU_F / 360);
    sy       = sin(LAVangle);
    cy       = cos(LAVangle);
    LAVangle = angles[PITCH] * (M_TAU_F / 360);
    sp       = sin(LAVangle);
    cp       = cos(LAVangle);

    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;
}
*/

/*
 * @brief LocalVectorMA
 * @param[in] org
 * @param[in] dist
 * @param[in] vec
 * @param[out] out
 *
 * @note Unused
static ID_INLINE void LocalVectorMA(vec3_t org, float dist, vec3_t vec, vec3_t out)
{
    out[0] = org[0] + dist * vec[0];
    out[1] = org[1] + dist * vec[1];
    out[2] = org[2] + dist * vec[2];
}
*/

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

	// VectorNormalize( out );
	VectorNormalizeFast(out);
}

#define FUNCTABLE_SHIFT     (16 - FUNCTABLE_SIZE2)
#define SIN_TABLE(i)      tr.sinTable[(i) >> FUNCTABLE_SHIFT];
#define COS_TABLE(i)      tr.sinTable[(((i) >> FUNCTABLE_SHIFT) + (FUNCTABLE_SIZE / 4))&FUNCTABLE_MASK];

/**
 * @brief LocalIngleVector
 * @param[in] ingles
 * @param[out] forward
 */
static ID_INLINE void LocalIngleVector(int ingles[3], vec3_t forward)
{
	sy = SIN_TABLE(ingles[YAW] & 65535);
	cy = COS_TABLE(ingles[YAW] & 65535);
	sp = SIN_TABLE(ingles[PITCH] & 65535);
	cp = COS_TABLE(ingles[PITCH] & 65535);

	// sy = sin( SHORT2ANGLE( ingles[ YAW ] ) * (M_TAU_F / 360) );
	// cy = cos( SHORT2ANGLE( ingles[ YAW ] ) * (M_TAU_F / 360) );
	// sp = sin( SHORT2ANGLE( ingles[ PITCH ] ) * (M_TAU_F / 360) );
	// cp = cos( SHORT2ANGLE( ingles[ PITCH ] ) *  (M_TAU_F / 360) );

	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
}

/**
 * @brief InglesToAxis
 * @param[in] ingles
 * @param[out] axis
 */
static void InglesToAxis(int ingles[3], vec3_t axis[3])
{
	// get sine/cosines for angles
	sy = SIN_TABLE(ingles[YAW] & 65535);
	cy = COS_TABLE(ingles[YAW] & 65535);
	sp = SIN_TABLE(ingles[PITCH] & 65535);
	cp = COS_TABLE(ingles[PITCH] & 65535);
	sr = SIN_TABLE(ingles[ROLL] & 65535);
	cr = COS_TABLE(ingles[ROLL] & 65535);

	// calculate axis vecs
	axis[0][0] = cp * cy;
	axis[0][1] = cp * sy;
	axis[0][2] = -sp;

	axis[1][0] = sr * sp * cy + cr * -sy;
	axis[1][1] = sr * sp * sy + cr * cy;
	axis[1][2] = sr * cp;

	axis[2][0] = cr * sp * cy + -sr * -sy;
	axis[2][1] = cr * sp * sy + -sr * cy;
	axis[2][2] = cr * cp;
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
 * @note const usage would require an explicit cast, non ANSI C - see unix/const-arg.c
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
 * @note const usage would require an explicit cast, non ANSI C - see unix/const-arg.c
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
 * @note const usage would require an explicit cast, non ANSI C - see unix/const-arg.c
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
 * @param[out] dst
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
 * @param[in] torsoParent
 * @param refent - unused
 * @param[in] boneNum
 */
static void R_CalcBone(const int torsoParent, const refEntity_t *refent, int boneNum)
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
	AnglesToAxis(angles, bonePtr->matrix);

	// translation
	if (parentBone)
	{
		if (fullTorso)
		{
#ifndef YD_INGLES
			sh      = (short *)cTBonePtr->ofsAngles;
			pf      = angles;
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = 0;
			LocalAngleVector(angles, vec);
#else
			sh        = (short *)cTBonePtr->ofsAngles;
			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, vec);
#endif

			VectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
		}
		else
		{
#ifndef YD_INGLES
			sh      = (short *)cBonePtr->ofsAngles;
			pf      = angles;
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = 0;
			LocalAngleVector(angles, vec);
#else
			sh        = (short *)cBonePtr->ofsAngles;
			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, vec);
#endif
			if (isTorso)
			{
#ifndef YD_INGLES
				sh      = (short *)cTBonePtr->ofsAngles;
				pf      = tangles;
				*(pf++) = SHORT2ANGLE(*(sh++));
				*(pf++) = SHORT2ANGLE(*(sh++));
				*(pf++) = 0;
				LocalAngleVector(tangles, v2);
#else
				sh         = (short *)cTBonePtr->ofsAngles;
				tingles[0] = sh[0];
				tingles[1] = sh[1];
				tingles[2] = 0;
				LocalIngleVector(tingles, v2);
#endif
				// blend the angles together
				SLerp_Normal(vec, v2, thisBoneInfo->torsoWeight, vec);
				VectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
			else        // legs bone
			{
				VectorMA(parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation);
			}
		}
	}
	else        // just use the frame position
	{
		bonePtr->translation[0] = frame->parentOffset[0];
		bonePtr->translation[1] = frame->parentOffset[1];
		bonePtr->translation[2] = frame->parentOffset[2];
	}

	if (boneNum == torsoParent)     // this is the torsoParent
	{
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}

	validBones[boneNum] = 1;

	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;
}

/**
 * @brief R_CalcBoneLerp
 * @param[in] torsoParent
 * @param[in] refent
 * @param[in] boneNum
 */
static void R_CalcBoneLerp(const int torsoParent, const refEntity_t *refent, int boneNum)
{
	int j;

	if (!refent || boneNum < 0 || boneNum >= MDX_MAX_BONES)
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
#ifndef YD_INGLES
	if (fullTorso)
	{
		sh  = (short *)cTBonePtr->angles;
		sh2 = (short *)cOldTBonePtr->angles;
		pf  = angles;

		a1      = SHORT2ANGLE(*(sh++));
		a2      = SHORT2ANGLE(*(sh2++));
		diff    = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
		a1      = SHORT2ANGLE(*(sh++));
		a2      = SHORT2ANGLE(*(sh2++));
		diff    = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
		a1      = SHORT2ANGLE(*(sh++));
		a2      = SHORT2ANGLE(*(sh2++));
		diff    = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - torsoBacklerp * diff;
	}
	else
	{
		sh  = (short *)cBonePtr->angles;
		sh2 = (short *)cOldBonePtr->angles;
		pf  = angles;

		a1      = SHORT2ANGLE(*(sh++));
		a2      = SHORT2ANGLE(*(sh2++));
		diff    = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;
		a1      = SHORT2ANGLE(*(sh++));
		a2      = SHORT2ANGLE(*(sh2++));
		diff    = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;
		a1      = SHORT2ANGLE(*(sh++));
		a2      = SHORT2ANGLE(*(sh2++));
		diff    = AngleNormalize180(a1 - a2);
		*(pf++) = a1 - backlerp * diff;

		if (isTorso)
		{
			sh  = (short *)cTBonePtr->angles;
			sh2 = (short *)cOldTBonePtr->angles;
			pf  = tangles;

			a1      = SHORT2ANGLE(*(sh++));
			a2      = SHORT2ANGLE(*(sh2++));
			diff    = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;
			a1      = SHORT2ANGLE(*(sh++));
			a2      = SHORT2ANGLE(*(sh2++));
			diff    = AngleNormalize180(a1 - a2);
			*(pf++) = a1 - torsoBacklerp * diff;
			a1      = SHORT2ANGLE(*(sh++));
			a2      = SHORT2ANGLE(*(sh2++));
			diff    = AngleNormalize180(a1 - a2);
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

#else
	// ingles-based bone code
	if (fullTorso)
	{
		sh  = (short *)cTBonePtr->angles;
		sh2 = (short *)cOldTBonePtr->angles;
		for (j = 0; j < 3; j++)
		{
			ingles[j] = (sh[j] - sh2[j]) & 65535;
			if (ingles[j] > 32767)
			{
				ingles[j] -= 65536;
			}
			ingles[j] = sh[j] - torsoBacklerp * ingles[j];
		}
	}
	else
	{
		sh  = (short *)cBonePtr->angles;
		sh2 = (short *)cOldBonePtr->angles;
		for (j = 0; j < 3; j++)
		{
			ingles[j] = (sh[j] - sh2[j]) & 65535;
			if (ingles[j] > 32767)
			{
				ingles[j] -= 65536;
			}
			ingles[j] = sh[j] - backlerp * ingles[j];
		}

		if (isTorso)
		{
			sh  = (short *)cTBonePtr->angles;
			sh2 = (short *)cOldTBonePtr->angles;
			for (j = 0; j < 3; j++)
			{
				tingles[j] = (sh[j] - sh2[j]) & 65535;
				if (tingles[j] > 32767)
				{
					tingles[j] -= 65536;
				}
				tingles[j] = sh[j] - torsoBacklerp * tingles[j];

				// blend torso and angles
				tingles[j] = (tingles[j] - ingles[j]) & 65535;
				if (tingles[j] > 32767)
				{
					tingles[j] -= 65536;
				}
				ingles[j] += thisBoneInfo->torsoWeight * tingles[j];
			}
		}
	}

	InglesToAxis(ingles, bonePtr->matrix);

#endif

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

#ifndef YD_INGLES
		pf      = angles;
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = SHORT2ANGLE(*(sh++));
		*(pf++) = 0;
		LocalAngleVector(angles, v2);   // new

		pf      = angles;
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = SHORT2ANGLE(*(sh2++));
		*(pf++) = 0;
		LocalAngleVector(angles, vec);  // old
#else
		ingles[0] = sh[0];
		ingles[1] = sh[1];
		ingles[2] = 0;
		LocalIngleVector(ingles, v2);   // new

		ingles[0] = sh2[0];
		ingles[1] = sh2[1];
		ingles[2] = 0;
		LocalIngleVector(ingles, vec);  // old
#endif

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
		if (!fullTorso && isTorso)
		{
			// partial legs/torso, need to lerp according to torsoWeight

			// calc the torso frame
			sh  = (short *)cTBonePtr->ofsAngles;
			sh2 = (short *)cOldTBonePtr->ofsAngles;

#ifndef YD_INGLES
			pf      = angles;
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = SHORT2ANGLE(*(sh++));
			*(pf++) = 0;
			LocalAngleVector(angles, v2);   // new

			pf      = angles;
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = SHORT2ANGLE(*(sh2++));
			*(pf++) = 0;
			LocalAngleVector(angles, vec);  // old
#else
			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, v2);   // new

			ingles[0] = sh[0];
			ingles[1] = sh[1];
			ingles[2] = 0;
			LocalIngleVector(ingles, vec);  // old
#endif

			// blend the angles together
			SLerp_Normal(vec, v2, torsoFrontlerp, v2);

			// blend the torso/legs together
			SLerp_Normal(dir, v2, thisBoneInfo->torsoWeight, dir);
		}

		VectorMA(parentBone->translation, thisBoneInfo->parentDist, dir, bonePtr->translation);
	}
	else
	{
		// just interpolate the frame positions
		bonePtr->translation[0] = frontlerp * frame->parentOffset[0] + backlerp * oldFrame->parentOffset[0];
		bonePtr->translation[1] = frontlerp * frame->parentOffset[1] + backlerp * oldFrame->parentOffset[1];
		bonePtr->translation[2] = frontlerp * frame->parentOffset[2] + backlerp * oldFrame->parentOffset[2];
	}

	if (boneNum == torsoParent)     // this is the torsoParent
	{
		VectorCopy(bonePtr->translation, torsoParentOffset);
	}
	validBones[boneNum] = 1;

	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;
}

/**
 * @brief R_BonesStillValid
 * @param[in] refent
 * @return
 *
 * @todo FIXME: optimization opportunity here, profile which values change most often and check for those first to get early outs
 *
 * Other way we could do this is doing a random memory probe, which in worst case scenario ends up being the memcmp? - BAD as only a few values are used
 *
 * Another solution: bones cache on an entity basis?
 */
static qboolean R_BonesStillValid(const refEntity_t *refent)
{
	if (lastBoneEntity.hModel != refent->hModel)
	{
		return qfalse;
	}
	else if (lastBoneEntity.frame != refent->frame)
	{
		return qfalse;
	}
	else if (lastBoneEntity.oldframe != refent->oldframe)
	{
		return qfalse;
	}
	else if (lastBoneEntity.frameModel != refent->frameModel)
	{
		return qfalse;
	}
	else if (lastBoneEntity.oldframeModel != refent->oldframeModel)
	{
		return qfalse;
	}
	else if (lastBoneEntity.backlerp != refent->backlerp)
	{
		return qfalse;
	}
	else if (lastBoneEntity.torsoFrame != refent->torsoFrame)
	{
		return qfalse;
	}
	else if (lastBoneEntity.oldTorsoFrame != refent->oldTorsoFrame)
	{
		return qfalse;
	}
	else if (lastBoneEntity.torsoFrameModel != refent->torsoFrameModel)
	{
		return qfalse;
	}
	else if (lastBoneEntity.oldTorsoFrameModel != refent->oldTorsoFrameModel)
	{
		return qfalse;
	}
	else if (lastBoneEntity.torsoBacklerp != refent->torsoBacklerp)
	{
		return qfalse;
	}
	else if (lastBoneEntity.reFlags != refent->reFlags)
	{
		return qfalse;
	}
	else if (!VectorCompare(lastBoneEntity.torsoAxis[0], refent->torsoAxis[0]) ||
	         !VectorCompare(lastBoneEntity.torsoAxis[1], refent->torsoAxis[1]) ||
	         !VectorCompare(lastBoneEntity.torsoAxis[2], refent->torsoAxis[2]))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief The list of bones[] should only be built and modified from within here
 * @param[in] refent
 * @param[in] boneList
 * @param[in] numBones
 */
static void R_CalcBones(const refEntity_t *refent, int *boneList, int numBones)
{
	int         i;
	int         *boneRefs;
	float       torsoWeight;
	mdxHeader_t *mdxFrameHeader         = R_GetModelByHandle(refent->frameModel)->model.mdx;
	mdxHeader_t *mdxOldFrameHeader      = R_GetModelByHandle(refent->oldframeModel)->model.mdx;
	mdxHeader_t *mdxTorsoFrameHeader    = R_GetModelByHandle(refent->torsoFrameModel)->model.mdx;
	mdxHeader_t *mdxOldTorsoFrameHeader = R_GetModelByHandle(refent->oldTorsoFrameModel)->model.mdx;

	if (!mdxFrameHeader || !mdxOldFrameHeader || !mdxTorsoFrameHeader || !mdxOldTorsoFrameHeader)
	{
		return;
	}

	// if the entity has changed since the last time the bones were built, reset them
	if (!R_BonesStillValid(refent))
	{
		// different, cached bones are not valid
		Com_Memset(validBones, 0, mdxFrameHeader->numBones);
		lastBoneEntity = *refent;

		// also reset these counter statics
		// print stats for the complete model (not per-surface)
		if (r_bonesDebug->integer == 4 && totalrt)
		{
			Ren_Print("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n",
			          lodScale, totalrv, totalv, totalrt, totalt, (float)(100.0 * totalrt) / (float)totalt);
		}
		totalrv = totalrt = totalv = totalt = 0;
	}

	Com_Memset(newBones, 0, mdxFrameHeader->numBones);

	if (refent->oldframe == refent->frame && refent->oldframeModel == refent->frameModel)
	{
		backlerp  = 0;
		frontlerp = 1;
	}
	else
	{
		backlerp  = refent->backlerp;
		frontlerp = 1.0f - backlerp;
	}

	if (refent->oldTorsoFrame == refent->torsoFrame && refent->oldTorsoFrameModel == refent->oldframeModel)
	{
		torsoBacklerp  = 0;
		torsoFrontlerp = 1;
	}
	else
	{
		torsoBacklerp  = refent->torsoBacklerp;
		torsoFrontlerp = 1.0f - torsoBacklerp;
	}

	frame = (mdxFrame_t *) ((byte *) mdxFrameHeader + mdxFrameHeader->ofsFrames +
	                        refent->frame * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxFrameHeader->numBones +
	                        refent->frame * sizeof(mdxFrame_t));
	torsoFrame = (mdxFrame_t *) ((byte *) mdxTorsoFrameHeader + mdxTorsoFrameHeader->ofsFrames +
	                             refent->torsoFrame * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxTorsoFrameHeader->numBones +
	                             refent->torsoFrame * sizeof(mdxFrame_t));
	oldFrame = (mdxFrame_t *) ((byte *) mdxOldFrameHeader + mdxOldFrameHeader->ofsFrames +
	                           refent->oldframe * (int)(sizeof(mdxBoneFrameCompressed_t)) * mdxOldFrameHeader->numBones +
	                           refent->oldframe * sizeof(mdxFrame_t));
	oldTorsoFrame = (mdxFrame_t *) ((byte *) mdxOldTorsoFrameHeader + mdxOldTorsoFrameHeader->ofsFrames +
	                                refent->oldTorsoFrame * (int)(sizeof(mdxBoneFrameCompressed_t)) *
	                                mdxOldTorsoFrameHeader->numBones + refent->oldTorsoFrame * sizeof(mdxFrame_t));

	// lerp all the needed bones (torsoParent is always the first bone in the list)

	frameSize = (int)sizeof(mdxBoneFrameCompressed_t) * mdxFrameHeader->numBones;

	cBoneList = (mdxBoneFrameCompressed_t *) ((byte *) mdxFrameHeader + mdxFrameHeader->ofsFrames +
	                                          (refent->frame + 1) * sizeof(mdxFrame_t) + refent->frame * frameSize);
	cBoneListTorso = (mdxBoneFrameCompressed_t *) ((byte *) mdxTorsoFrameHeader + mdxTorsoFrameHeader->ofsFrames +
	                                               (refent->torsoFrame + 1) * sizeof(mdxFrame_t) +
	                                               refent->torsoFrame * frameSize);

	boneInfo = (mdxBoneInfo_t *) ((byte *) mdxFrameHeader + mdxFrameHeader->ofsBones);
	boneRefs = boneList;
	//
	Matrix3Transpose(refent->torsoAxis, torsoAxis);

	if (backlerp == 0.f && torsoBacklerp == 0.f)
	{
		for (i = 0; i < numBones; i++, boneRefs++)
		{
			if (validBones[*boneRefs])
			{
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ((boneInfo[*boneRefs].parent >= 0) &&
			    (!validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent]))
			{
				R_CalcBone(mdxFrameHeader->torsoParent, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBone(mdxFrameHeader->torsoParent, refent, *boneRefs);
		}
	}
	else
	{
		// interpolated
		cOldBoneList = (mdxBoneFrameCompressed_t *) ((byte *) mdxOldFrameHeader + mdxOldFrameHeader->ofsFrames +
		                                             (refent->oldframe + 1) * sizeof(mdxFrame_t) + refent->oldframe * frameSize);
		cOldBoneListTorso = (mdxBoneFrameCompressed_t *) ((byte *) mdxOldTorsoFrameHeader + mdxOldTorsoFrameHeader->ofsFrames +
		                                                  (refent->oldTorsoFrame + 1) * sizeof(mdxFrame_t) +
		                                                  refent->oldTorsoFrame * frameSize);

		for (i = 0; i < numBones; i++, boneRefs++)
		{
			if (validBones[*boneRefs])
			{
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ((boneInfo[*boneRefs].parent >= 0) &&
			    (!validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent]))
			{
				R_CalcBoneLerp(mdxFrameHeader->torsoParent, refent, boneInfo[*boneRefs].parent);
			}

			R_CalcBoneLerp(mdxFrameHeader->torsoParent, refent, *boneRefs);
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

			//if ( !(thisBoneInfo->flags & BONEFLAG_TAG) ) {

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

			/*
			} else {  // tag's require special handling

			   // rotate each of the axis by the torsoAngles
			   LocalScaledMatrixTransformVector( bonePtr->matrix[0], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[0] );
			   LocalScaledMatrixTransformVector( bonePtr->matrix[1], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[1] );
			   LocalScaledMatrixTransformVector( bonePtr->matrix[2], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[2] );
			   Com_Memcpy( bonePtr->matrix, tmpAxis, sizeof(tmpAxis) );

			   // rotate the translation around the torsoParent
			   VectorSubtract( bonePtr->translation, torsoParentOffset, t );
			   LocalScaledMatrixTransformVector( t, thisBoneInfo->torsoWeight, torsoAxis, bonePtr->translation );
			   VectorAdd( bonePtr->translation, torsoParentOffset, bonePtr->translation );

			}
			*/
		}
	}

	// backup the final bones
	Com_Memcpy(oldBones, bones, sizeof(bones[0]) * mdxFrameHeader->numBones);
}

#ifdef DBG_PROFILE_BONES
#define DBG_SHOWTIME    Ren_Print("%i: %i, ", di++, (dt = ri.Milliseconds()) - ldt); ldt = dt;
#else
#define DBG_SHOWTIME    ;
#endif

/**
 * @brief RB_MDM_SurfaceAnim
 * @param[in] surface
 */
void RB_MDM_SurfaceAnim(mdmSurface_t *surface)
{
	int         j, k;
	refEntity_t *refent   = &backEnd.currentEntity->e;
	int         *boneList = ( int * )((byte *)surface + surface->ofsBoneReferences);
	mdmHeader_t *header   = ( mdmHeader_t * )((byte *)surface + surface->ofsHeader);

#ifdef DBG_PROFILE_BONES
	int di = 0, dt, ldt;

	dt  = ri.Milliseconds();
	ldt = dt;
#endif

	R_CalcBones((const refEntity_t *)refent, boneList, surface->numBoneReferences);

	DBG_SHOWTIME

	// calculate LOD
	// TODO: lerp the radius and origin
	VectorAdd(refent->origin, frame->localOrigin, vec);
	lodRadius = frame->radius;
	lodScale  = RB_CalcMDMLod(refent, vec, lodRadius, header->lodBias, header->lodScale);

	// debug code
	//%	lodScale = 0.15;

//DBG_SHOWTIME

	// modification to allow dead skeletal bodies to go below minlod (experiment)
	if (refent->reFlags & REFLAG_DEAD_LOD)
	{
		if (lodScale < 0.35f)       // allow dead to lod down to 35% (even if below surf->minLod) (%35 is arbitrary and probably not good generally.  worked for the blackguard/infantry as a test though)
		{
			lodScale = 0.35f;
		}
		render_count = round((float) surface->numVerts * lodScale);

	}
	else
	{
		render_count = round((float) surface->numVerts * lodScale);
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

	// to profile bone transform performance only
	if (r_bonesDebug->integer == 10)
	{
		return;
	}

	RB_CheckOverflow(render_count, surface->numTriangles * 3);

//DBG_SHOWTIME

	//
	// setup triangle list
	//
	// no need to do this twice
	//%	RB_CheckOverflow( surface->numVerts, surface->numTriangles * 3);

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
	v          = ( mdmVertex_t * )((byte *)surface + surface->ofsVerts);
	tempVert   = ( float * )(tess.xyz + baseVertex);
	tempNormal = ( float * )(tess.normal + baseVertex);
	for (j = 0; j < render_count; j++, tempVert += 4, tempNormal += 4)
	{
		mdmWeight_t *w;

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

		v = (mdmVertex_t *)&v->weights[v->numWeights];
	}

	DBG_SHOWTIME

	if (r_bonesDebug->integer)
	{
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
		if (r_bonesDebug->integer < 3 || r_bonesDebug->integer == 5 || r_bonesDebug->integer == 8 || r_bonesDebug->integer == 9)
		{
			int i;

			// DEBUG: show the bones as a stick figure with axis at each bone
			boneRefs = ( int * )((byte *)surface + surface->ofsBoneReferences);
			for (i = 0; i < surface->numBoneReferences; i++, boneRefs++)
			{
				bonePtr = &bones[*boneRefs];

				GL_Bind(tr.whiteImage);
				if (r_bonesDebug->integer != 9)
				{
					glLineWidth(1);
					glBegin(GL_LINES);
					for (j = 0; j < 3; j++)
					{
						VectorClear(vec);
						vec[j] = 1;
						glColor3fv(vec);
						glVertex3fv(bonePtr->translation);
						VectorMA(bonePtr->translation, (r_bonesDebug->integer == 8 ? 1.5f : 5), bonePtr->matrix[j], vec);
						glVertex3fv(vec);
					}
					glEnd();
				}

				// connect to our parent if it's valid
				if (validBones[boneInfo[*boneRefs].parent])
				{
					glLineWidth(r_bonesDebug->integer == 8 ? 4 : 2);
					glBegin(GL_LINES);
					glColor3f(.6f, .6f, .6f);
					glVertex3fv(bonePtr->translation);
					glVertex3fv(bones[boneInfo[*boneRefs].parent].translation);
					glEnd();
				}

				glLineWidth(1);
			}

			if (r_bonesDebug->integer == 8)
			{
				// FIXME: Actually draw the whole skeleton
				//if( surface == (mdmSurface_t *)((byte *)header + header->ofsSurfaces) ) {
				mdxHeader_t *mdxHeader = R_GetModelByHandle(refent->frameModel)->model.mdx;
				boneRefs = ( int * )((byte *)surface + surface->ofsBoneReferences);

				glDepthRange(0, 0);        // never occluded
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

				for (i = 0; i < surface->numBoneReferences; i++, boneRefs++)
				{
					vec3_t        diff;
					mdxBoneInfo_t *mdxBoneInfo = ( mdxBoneInfo_t * )((byte *)mdxHeader + mdxHeader->ofsBones + *boneRefs * sizeof(mdxBoneInfo_t));

					bonePtr = &bones[*boneRefs];

					VectorSet(vec, 0.f, 0.f, 32.f);
					VectorSubtract(bonePtr->translation, vec, diff);
					vec[0] = vec[0] + diff[0] * 6;
					vec[1] = vec[1] + diff[1] * 6;
					vec[2] = vec[2] + diff[2] * 3;

					glEnable(GL_BLEND);
					glBegin(GL_LINES);
					glColor4f(1.f, .4f, .05f, .35f);
					glVertex3fv(bonePtr->translation);
					glVertex3fv(vec);
					glEnd();
					glDisable(GL_BLEND);

					R_DebugText(vec, 1.f, 1.f, 1.f, mdxBoneInfo->name, qfalse);         // qfalse, as there is no reason to set depthrange again
				}

				glDepthRange(0, 1);
				//}
			}
			else if (r_bonesDebug->integer == 9)
			{
				if (surface == ( mdmSurface_t * )((byte *)header + header->ofsSurfaces))
				{
					mdmTag_t *pTag = ( mdmTag_t * )((byte *)header + header->ofsTags);

					glDepthRange(0, 0);    // never occluded
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);

					for (i = 0; i < header->numTags; i++)
					{
						mdxBoneFrame_t *tagBone;
						orientation_t  outTag;
						vec3_t         diff;

						// now extract the orientation for the bone that represents our tag
						tagBone = &bones[pTag->boneIndex];
						VectorClear(outTag.origin);
						LocalAddScaledMatrixTransformVectorTranslate(pTag->offset, 1.f, tagBone->matrix, tagBone->translation, outTag.origin);
						for (j = 0; j < 3; j++)
						{
							LocalMatrixTransformVector(pTag->axis[j], bone->matrix, outTag.axis[j]);
						}

						GL_Bind(tr.whiteImage);
						glLineWidth(2);
						glBegin(GL_LINES);
						for (j = 0; j < 3; j++)
						{
							VectorClear(vec);
							vec[j] = 1;
							glColor3fv(vec);
							glVertex3fv(outTag.origin);
							VectorMA(outTag.origin, 5, outTag.axis[j], vec);
							glVertex3fv(vec);
						}
						glEnd();

						VectorSet(vec, 0.f, 0.f, 32.f);
						VectorSubtract(outTag.origin, vec, diff);
						vec[0] = vec[0] + diff[0] * 2;
						vec[1] = vec[1] + diff[1] * 2;
						vec[2] = vec[2] + diff[2] * 1.5f;

						glLineWidth(1);
						glEnable(GL_BLEND);
						glBegin(GL_LINES);
						glColor4f(1.f, .4f, .05f, .35f);
						glVertex3fv(outTag.origin);
						glVertex3fv(vec);
						glEnd();
						glDisable(GL_BLEND);

						R_DebugText(vec, 1.f, 1.f, 1.f, pTag->name, qfalse);    // qfalse, as there is no reason to set depthrange again

						pTag = ( mdmTag_t * )((byte *)pTag + pTag->ofsEnd);
					}
					glDepthRange(0, 1);
				}
			}
		}

		if (r_bonesDebug->integer >= 3 && r_bonesDebug->integer <= 6)
		{
			int render_indexes = tess.numIndexes - oldIndexes;

			// show mesh edges
			tempVert   = ( float * )(tess.xyz + baseVertex);
			tempNormal = ( float * )(tess.normal + baseVertex);

			GL_Bind(tr.whiteImage);
			glLineWidth(1);
			glBegin(GL_LINES);
			glColor3f(.0, .0, .8f);

			pIndexes = &tess.indexes[oldIndexes];
			for (j = 0; j < render_indexes / 3; j++, pIndexes += 3)
			{
				glVertex3fv(tempVert + 4 * pIndexes[0]);
				glVertex3fv(tempVert + 4 * pIndexes[1]);

				glVertex3fv(tempVert + 4 * pIndexes[1]);
				glVertex3fv(tempVert + 4 * pIndexes[2]);

				glVertex3fv(tempVert + 4 * pIndexes[2]);
				glVertex3fv(tempVert + 4 * pIndexes[0]);
			}

			glEnd();


			if (r_bonesDebug->integer == 4) // track debug stats
			{
				totalrv += render_count;
				totalrt += render_indexes / 3;
				totalv  += surface->numVerts;
				totalt  += surface->numTriangles;
			}

			if (r_bonesDebug->integer == 3)
			{
				Ren_Print("Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n", lodScale, render_count, surface->numVerts, render_indexes / 3, surface->numTriangles,
				          100.0 * render_indexes / 3 / (double)surface->numTriangles);
			}
		}

		if (r_bonesDebug->integer == 6 || r_bonesDebug->integer == 7)
		{
			v        = ( mdmVertex_t * )((byte *)surface + surface->ofsVerts);
			tempVert = ( float * )(tess.xyz + baseVertex);
			GL_Bind(tr.whiteImage);
			glPointSize(5);
			glBegin(GL_POINTS);
			for (j = 0; j < render_count; j++, tempVert += 4)
			{
				if (v->numWeights > 1)
				{
					if (v->numWeights == 2)
					{
						glColor3f(.4f, .4f, 0.f);
					}
					else if (v->numWeights == 3)
					{
						glColor3f(.8f, .4f, 0.f);
					}
					else
					{
						glColor3f(1.f, .4f, 0.f);
					}
					glVertex3fv(tempVert);
				}
				v = (mdmVertex_t *)&v->weights[v->numWeights];
			}
			glEnd();
		}
	}

/*	if( r_showmodelbounds->integer ) {
        vec3_t diff, v1, v2, v3, v4, v5, v6;
        mdxHeader_t	*mdxHeader = R_GetModelByHandle( refent->frameModel )->model.mdx;
        mdxFrame_t	*mdxFrame = (mdxFrame_t *)((byte *)mdxHeader + mdxHeader->ofsFrames +
                                refent->frame * (int) ( sizeof( mdxBoneFrameCompressed_t ) ) * mdxHeader->numBones +
                                refent->frame * sizeof(mdxFrame_t));

        // show model bounds
        GL_Bind( tr.whiteImage );
        GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
        glLineWidth( 1 );
        glColor3f( .0,.8,.0 );
        glBegin( GL_LINES );

        VectorSubtract( mdxFrame->bounds[0], mdxFrame->bounds[1], diff);

        VectorCopy( mdxFrame->bounds[0], v1 );
        VectorCopy( mdxFrame->bounds[0], v2 );
        VectorCopy( mdxFrame->bounds[0], v3 );
        v1[0] -= diff[0];
        v2[1] -= diff[1];
        v3[2] -= diff[2];
        glVertex3fv( mdxFrame->bounds[0] );
        glVertex3fv( v1 );
        glVertex3fv( mdxFrame->bounds[0] );
        glVertex3fv( v2 );
        glVertex3fv( mdxFrame->bounds[0] );
        glVertex3fv( v3 );

        VectorCopy( mdxFrame->bounds[1], v4 );
        VectorCopy( mdxFrame->bounds[1], v5 );
        VectorCopy( mdxFrame->bounds[1], v6 );
        v4[0] += diff[0];
        v5[1] += diff[1];
        v6[2] += diff[2];
        glVertex3fv( mdxFrame->bounds[1] );
        glVertex3fv( v4 );
        glVertex3fv( mdxFrame->bounds[1] );
        glVertex3fv( v5 );
        glVertex3fv( mdxFrame->bounds[1] );
        glVertex3fv( v6 );

        glVertex3fv( v2 );
        glVertex3fv( v6 );
        glVertex3fv( v6 );
        glVertex3fv( v1 );
        glVertex3fv( v1 );
        glVertex3fv( v5 );

        glVertex3fv( v2 );
        glVertex3fv( v4 );
        glVertex3fv( v4 );
        glVertex3fv( v3 );
        glVertex3fv( v3 );
        glVertex3fv( v5 );

        glEnd();
    }*/

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
 * @brief R_MDM_GetBoneTag
 * @param[out] outTag
 * @param[in] mdm
 * @param[in] startTagIndex
 * @param[in] refent
 * @param[in] tagName
 * @return
 */
int R_MDM_GetBoneTag(orientation_t *outTag, mdmHeader_t *mdm, int startTagIndex, const refEntity_t *refent, const char *tagName)
{
	int      i, j;
	mdmTag_t *pTag;
	int      *boneList;

	if (startTagIndex > mdm->numTags)
	{
		Com_Memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// find the correct tag
	pTag = ( mdmTag_t * )((byte *)mdm + mdm->ofsTags);

	if (startTagIndex)
	{
		for (i = 0; i < startTagIndex; i++)
		{
			pTag = ( mdmTag_t * )((byte *)pTag + pTag->ofsEnd);
		}
	}

	for (i = startTagIndex; i < mdm->numTags; i++)
	{
		if (!strcmp(pTag->name, tagName))
		{
			break;
		}
		pTag = ( mdmTag_t * )((byte *)pTag + pTag->ofsEnd);
	}

	if (i >= mdm->numTags)
	{
		Com_Memset(outTag, 0, sizeof(*outTag));
		return -1;
	}

	// calc the bones
	boneList = ( int * )((byte *)pTag + pTag->ofsBoneReferences);
	R_CalcBones(refent, boneList, pTag->numBoneReferences);

	// now extract the orientation for the bone that represents our tag
	bone = &bones[pTag->boneIndex];
	VectorClear(outTag->origin);
	LocalAddScaledMatrixTransformVectorTranslate(pTag->offset, 1.f, bone->matrix, bone->translation, outTag->origin);
	for (j = 0; j < 3; j++)
	{
		LocalMatrixTransformVector(pTag->axis[j], bone->matrix, outTag->axis[j]);
	}
	return i;
}
