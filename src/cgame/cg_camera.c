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
 * @file cg_camera.c
 * @brief Handle "recorded" camera playback and recording
 *
 * TODO: add brief
 */

#include "cg_local.h"

typedef struct cameraPoint_s
{
	vec3_t origin; ///< Camera origin point
	vec3_t angles; ///< Camera angles
	vec3_t ctIn; ///< Bezier curve incoming control point
	vec3_t ctOut; ///< Bezier curve outgoing control point
	float fov; // FIXME: implement
	int time; // FIXME: implement
	uint8_t timescale; // FIXME: implement
	uint32_t flags; // TODO: do we need this after all?
	float len; ///< Spline length, this is calculated when the playback is started.
	struct cameraPoint_s *next, *prev;
} cameraPoint_t;

//FIXME: allocate the points since locking this to 50 might be problematic?
#define MAX_CAMERA_POINTS 50
static cameraPoint_t cameraPoints[MAX_CAMERA_POINTS];
static int           cameraPointOffset = 0;

// TODO: move to cgs when ready
static cameraPoint_t *currentCamera  = NULL;
static cameraPoint_t *pointingCamera = NULL;

static cameraPoint_t *currentPlayCamera  = NULL;
static float         cameraPoint         = 0.f;
static float         cameraTotalLength   = 0.f;
static float         cameraUnitsInSecond = 0.f;

/*
 * @brief CG_CalcBezierPoint
 * @param[in] start
 * @param[in] ctrl1
 * @param[in] ctrl2
 * @param[in] end
 * @param[in] offset
 * @param[out] out
 **/
void CG_CalcBezierPoint(const vec3_t start, const vec3_t ctrl1, const vec3_t ctrl2, const vec3_t end, float offset, vec3_t out)
{
	float var1, var2, var3;

	// Below is the equation for a 4 control point bezier curve:
	// B(t) = P1 * ( 1 - t )^3 + P2 * 3 * t * ( 1 - t )^2 + P3 * 3 * t^2 * ( 1 - t ) + P4 * t^3

	var1   = 1.f - offset;
	var2   = var1 * var1 * var1;
	var3   = offset * offset * offset;
	out[0] = var2 * start[0] + 3 * offset * var1 * var1 * ctrl1[0] + 3 * offset * offset * var1 * ctrl2[0] + var3 * end[0];
	out[1] = var2 * start[1] + 3 * offset * var1 * var1 * ctrl1[1] + 3 * offset * offset * var1 * ctrl2[1] + var3 * end[1];
	out[2] = var2 * start[2] + 3 * offset * var1 * var1 * ctrl1[2] + 3 * offset * offset * var1 * ctrl2[2] + var3 * end[2];
}

float CG_CalcBezierArcLengths(const vec3_t start, const vec3_t ctrl1, const vec3_t ctrl2, const vec3_t end, vec_t *out, int fractions)
{
	int    i;
	float  len = 0, frac = 1.f / (float) fractions;
	vec3_t startVec, endVec;

	vec3_copy(start, startVec);

	for (i = 0; i < fractions; i++)
	{
		CG_CalcBezierPoint(start, ctrl1, ctrl2, end, frac + (frac * (float)i), endVec);
		len += vec3_distance(startVec, endVec);

		if (out)
		{
			out[i] = len;
		}

		vec3_copy(endVec, startVec);
	}

	return len;
}

void CG_ClearCamera(void)
{
	Com_Memset(cameraPoints, 0, 50 * sizeof(cameraPoint_t));
	cameraPointOffset = 0;
	currentCamera     = NULL;
}

static cameraPoint_t *CG_GetNewCameraPoint(void)
{
	if (cameraPointOffset + 1 >= MAX_CAMERA_POINTS)
	{
		return NULL;
	}

	if (!cameraPointOffset)
	{
		Com_Memset(cameraPoints, 0, 50 * sizeof(cameraPoint_t));
		currentCamera = &cameraPoints[0];
	}

	return &cameraPoints[cameraPointOffset++];
}

void CG_CameraAddCurrentPoint(void)
{
	cg.editingCameras = qtrue;

	cameraPoint_t *point = CG_GetNewCameraPoint();
	cameraPoint_t *last  = currentCamera;

	while (last && last->next)
	{
		last = last->next;
	}

	if (last != point)
	{
		last->next  = point;
		point->prev = last;
	}

	VectorCopy(cg.refdef.vieworg, point->origin);
	VectorCopy(cg.refdefViewAngles, point->angles);
	point->fov  = cg_fov.value;
	point->time = cg.time;
}

void CG_AddControlPoint(void)
{
	cameraPoint_t *last = currentCamera;

	while (last && last->next)
	{
		last = last->next;
	}

	if (vec3_equals(last->ctIn, 0, 0, 0))
	{
		vec3_sub(cg.refdef.vieworg, last->origin, last->ctIn);
	}
	else if (last->prev)
	{
		vec3_sub(cg.refdef.vieworg, last->prev->origin, last->prev->ctOut);
	}

}

void CG_PlayCurrentCamera(int seconds)
{
	vec3_t        bezCt1, bezCt2;
	cameraPoint_t *last = currentCamera;

	cameraTotalLength = 0.f;
	while (last && last->next)
	{
		if (!vec3_equals(last->ctOut, 0, 0, 0) || !vec3_equals(last->next->ctIn, 0, 0, 0))
		{
			vec3_add(last->origin, last->ctOut, bezCt1);
			vec3_add(last->next->origin, last->next->ctIn, bezCt2);
			last->len = CG_CalcBezierArcLengths(last->origin, bezCt1, bezCt2, last->next->origin, NULL, 20);
		}
		else
		{
			last->len = vec3_distance(last->origin, last->next->origin);
		}

		cameraTotalLength += last->len;
		last               = last->next;
	}

	cameraUnitsInSecond             = cameraTotalLength / (float) seconds;
	currentPlayCamera               = currentCamera;
	cgs.demoCamera.renderingFreeCam = qtrue;
}

void CG_RunCamera(void)
{
	cameraPoint_t *next;
	vec3_t        bezCt1, bezCt2;

	if (!currentPlayCamera)
	{
		cgs.demoCamera.renderingFreeCam = qfalse;
		return;
	}

	if (!currentPlayCamera->next)
	{
		cgs.demoCamera.renderingFreeCam = qfalse;
		currentPlayCamera               = NULL;
		return;
	}

	next = currentPlayCamera->next;

	if (!vec3_equals(currentPlayCamera->ctOut, 0, 0, 0) || !vec3_equals(next->ctIn, 0, 0, 0))
	{
#if 0
		vec3_add(currentPlayCamera->origin, currentPlayCamera->ctOut, bezCt1);
		vec3_add(next->origin, next->ctIn, bezCt2);
		CG_CalcBezierPoint(currentPlayCamera->origin, bezCt1, bezCt2, next->origin, cameraPoint, cgs.demoCamera.camOrigin);
#else
		int   i;
		vec_t bezierLengths[20] = { 0.f };
		int   len               = ARRAY_LEN(bezierLengths);
		float ff                = 1.f / (float)len;
		float actualPoint       = cameraPoint;
		vec_t expected          = currentPlayCamera->len * cameraPoint;
		vec3_add(currentPlayCamera->origin, currentPlayCamera->ctOut, bezCt1);
		vec3_add(next->origin, next->ctIn, bezCt2);

		// We need to calculate the lengths since the curve points are not in same distances
		CG_CalcBezierArcLengths(currentPlayCamera->origin, bezCt1, bezCt2, next->origin, bezierLengths, len);
		for (i = 0; i < len; i++)
		{
			if (bezierLengths[i] > expected)
			{
				float tmp1 = expected - bezierLengths[i - 1];
				float tmp2 = bezierLengths[i] - bezierLengths[i - 1];
				float rr   = ff * (float)i;
				rr         += (tmp1 / tmp2) * ff;
				actualPoint = rr;

				break;
			}
		}

		CG_CalcBezierPoint(currentPlayCamera->origin, bezCt1, bezCt2, next->origin, actualPoint, cgs.demoCamera.camOrigin);
#endif
	}
	else
	{
		vec3_lerp(currentPlayCamera->origin, next->origin, cameraPoint, cgs.demoCamera.camOrigin);
	}

	angles_lerp(currentPlayCamera->angles, next->angles, cameraPoint, cgs.demoCamera.camAngle);
	cgs.demoCamera.setCamAngles = qtrue;


	if (qfalse)
	{
		float diff = ((float) (cg.time - cg.oldTime)) / 1000;

		cameraPoint += diff;

		while (currentPlayCamera && cameraPoint >= 1.f)
		{
			currentPlayCamera = currentPlayCamera->next;
			cameraPoint      -= 1.f;
		}
	}
	else
	{
		float diff = ((float) (cg.time - cg.oldTime)) / 1000;

		// how much we should move
		float requiredMoveAmount = cameraUnitsInSecond * diff;

		/*
		double currentCameraOffset = cameraPoint * currentPlayCamera->len;

		if (currentCameraOffset + requiredMoveAmount > currentPlayCamera->len)
		{
			requiredMoveAmount = (currentCameraOffset + requiredMoveAmount) - currentPlayCamera->len;

			currentPlayCamera  = currentPlayCamera->next;

			if (currentPlayCamera)
			{
				cameraPoint = requiredMoveAmount / currentPlayCamera->len;
			}
		}
		else
		{
			cameraPoint += (requiredMoveAmount / currentPlayCamera->len);
		}
		*/


		while (currentPlayCamera)
		{
			float currentCameraOffset = cameraPoint * currentPlayCamera->len;

			if (currentCameraOffset + requiredMoveAmount > currentPlayCamera->len)
			{
				requiredMoveAmount = (currentCameraOffset + requiredMoveAmount) - currentPlayCamera->len;

				currentPlayCamera = currentPlayCamera->next;

				if (currentPlayCamera)
				{
					cameraPoint = requiredMoveAmount / currentPlayCamera->len;
				}
			}
			else
			{
				cameraPoint += (requiredMoveAmount / currentPlayCamera->len);
				break;
			}
		}

		/*
		cameraPoint += (movAmmount / currentPlayCamera->len);

		// FIXME: actually calc. this
		if (cameraPoint >= 1.f)
		{
			cameraPoint = cameraPoint - 1.f;
			float nextCamMove = movAmmount * cameraPoint;
			currentPlayCamera = currentPlayCamera->next;

			if (!currentPlayCamera)
			{
				cameraPoint = 0.f;
			}
			else if (currentPlayCamera->len < nextCamMove)
			{
				// nextCamMove -= currentPlayCamera->len;
				// FIXME: actually calc this..
				cameraPoint = 0.f;
			}
			else if (currentPlayCamera->len)
			{
				cameraPoint = nextCamMove / currentPlayCamera->len;
			}
		}
		*/
	}

	if (!currentPlayCamera || !currentPlayCamera->next)
	{
		cameraPoint                     = 0.f;
		cgs.demoCamera.setCamAngles     = qfalse;
		cgs.demoCamera.renderingFreeCam = qfalse;
	}
}

#define CAMERA_BEZIER_POINTS 20

void CG_RenderCameraPoints(void)
{
	int           offset = 0, i;
	float         dist, minDist;
	vec3_t        vec;
	vec3_t        bezTarget, bezSource, bezCt1, bezCt2;
	cameraPoint_t *closest   = NULL;
	cameraPoint_t *point     = currentCamera;
	const float   bezierStep = 1.f / CAMERA_BEZIER_POINTS;

	minDist = Square(8.f);

	// don't render the route while playback
	if (currentPlayCamera)
	{
		return;
	}

	while (point)
	{
		if (pointingCamera == point)
		{
			CG_AddOnScreenText(va(S_COLOR_RED "%i", ++offset), point->origin, qfalse);
		}
		else
		{
			CG_AddOnScreenText(va(S_COLOR_WHITE "%i", ++offset), point->origin, qfalse);
		}

		if (point->prev)
		{
			if (!vec3_equals(point->prev->ctOut, 0, 0, 0) || !vec3_equals(point->ctIn, 0, 0, 0))
			{
				vec3_add(point->prev->origin, point->prev->ctOut, bezCt1);
				vec3_add(point->origin, point->ctIn, bezCt2);

				// Should we draw the control point helpers?
				if (qtrue)
				{
					if (vec3_distance(point->prev->origin, bezCt1) > 1.f)
					{
						CG_DrawLine(point->prev->origin, bezCt1, 1.5f, colorWhite, cgs.media.railCoreShader);
					}

					if (vec3_distance(bezCt1, bezCt2) > 1.f)
					{
						CG_DrawLine(bezCt1, bezCt2, 1.5f, colorWhite, cgs.media.railCoreShader);
					}

					if (vec3_distance(bezCt2, point->origin) > 1.f)
					{
						CG_DrawLine(bezCt2, point->origin, 1.5f, colorWhite, cgs.media.railCoreShader);
					}
				}

				// FIXME: there is vec3_dist and vec3_distance functions that are the same?

				vec3_copy(point->prev->origin, bezSource);
				for (i = 1; i < CAMERA_BEZIER_POINTS; i++)
				{
					CG_CalcBezierPoint(point->prev->origin, bezCt1, bezCt2, point->origin, bezierStep * (float) i, bezTarget);
					CG_DrawLine(bezSource, bezTarget, 1.5f, colorGreen, cgs.media.railCoreShader);
					vec3_copy(bezTarget, bezSource);
				}

				CG_DrawLine(bezSource, point->origin, 1.5f, colorGreen, cgs.media.railCoreShader);
			}
			else
			{
				CG_DrawLine(point->prev->origin, point->origin, 1.5f, colorGreen, cgs.media.railCoreShader);
			}
		}

		// TODO: check if we are currently editing a camera and ignore this then..
		if (qtrue)
		{
			vec3_sub(point->origin, cg.refdef_current->vieworg, vec);
			dist = vec3_dot(vec, cg.refdef_current->viewaxis[0]);
			vec3_ma(cg.refdef_current->vieworg, dist, cg.refdef_current->viewaxis[0], vec);
			vec3_sub(point->origin, vec, vec);
			dist = vec3_length_squared(vec);
			if (dist <= minDist)
			{
				minDist = dist;
				closest = point;
			}
		}
		point = point->next;
	}

	pointingCamera = closest;
}
