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
typedef struct cameraEditorInfo
{
	// TODO: move to cgs when ready
	cameraPoint_t *currentCamera;
	cameraPoint_t *pointingCamera;
	cameraPoint_t *selectedCamera;
	cameraPoint_t *currentPlayCamera;
	cameraPoint_t *editingPoint;

	float cameraPoint;
	float cameraTotalLength;
	float cameraUnitsInSecond;

	int cameraPointOffset;
	cameraPoint_t cameraPoints[MAX_CAMERA_POINTS];

	vec3_t backupOrigin;
	vec3_t backupAngles;
} cameraEditorInfo;

static cameraEditorInfo cameraInfo;

static void CG_AddCameraModel(const vec3_t inOrigin, const vec3_t angles)
{
	vec3_t      mins, maxs, origin;
	refEntity_t re;

	if (!cgs.media.videoCameraModel)
	{
		cgs.media.videoCameraModel = trap_R_RegisterModel("models/editorcamera/camera.md3");
	}

	Com_Memset(&re, 0, sizeof(re));
	re.hModel = cgs.media.videoCameraModel;
	// re.customShader = cgs.media.genericConstructionShader;

	trap_R_ModelBounds(re.hModel, mins, maxs);

	vec3_copy(inOrigin, origin);
	origin[2] += -0.5f * (mins[2] + maxs[2]);
	origin[1] += 0.5f * (mins[1] + maxs[1]);

	vec3_copy(origin, re.origin);
	angles_to_axis(angles, re.axis);
	trap_R_AddRefEntityToScene(&re);
}

static inline void CG_TeleportPlayer(const vec3_t origin, const vec3_t angles, qboolean useViewHeight)
{
	// only send the viewpos command if we are not playing a demo file and we are only spectating
	if (cg.demoPlayback || cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		return;
	}

	trap_SendClientCommand(va("setviewpos %f %f %f %f %f %f %i",
	                          origin[0], origin[1], origin[2],
	                          angles[PITCH], angles[YAW], angles[ROLL],
	                          useViewHeight));
}

static inline void CG_OverrideMouse(qboolean value)
{
	trap_Cvar_Set("cl_bypassmouseinput", value ? "1" : "0");
}

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

qboolean CG_CameraCheckExecKey(int key, qboolean down, qboolean doAction)
{
	char buf[MAX_STRING_TOKENS];

	// we don't want these duplicate key presses
	if (key & K_CHAR_FLAG)
	{
		return qfalse;
	}

	// Escape does escape stuff.
	if (key == K_ESCAPE)
	{
		if (doAction && !down)
		{
			CG_OverrideMouse(qtrue);
			cameraInfo.selectedCamera = NULL;
		}
		return qtrue;
	}

	trap_Key_GetBindingBuf(key, buf, sizeof(buf));

	if (!buf[0])
	{
		return qfalse;
	}

	if (buf[0] == '+' && !down)
	{
		buf[0] = '-';
	}

	if (!Q_stricmp(buf, "-attack"))
	{
		if (doAction)
		{
			if (cameraInfo.pointingCamera)
			{
				CG_OverrideMouse(qfalse);
				cameraInfo.selectedCamera = cameraInfo.pointingCamera;
			}
		}
		return qtrue;
	}
	else if (!Q_stricmp(buf, "dropobj") && !down)
	{
		if (doAction)
		{
			CG_CameraAddCurrentPoint();
		}
		return qtrue;
	}

	return qfalse;
}

void CG_CameraEditor_KeyHandling(int key, qboolean down)
{
	if (!cg.editingCameras)
	{
		return;
	}

	CG_CameraCheckExecKey(key, down, qtrue);
}

void CG_CameraEditorMouseMove_Handling(int x, int y)
{
	if (!cg.editingCameras)
	{
		return;
	}
}

void CG_CameraEditorDraw(void)
{
	if (!cg.editingCameras)
	{
		return;
	}

	if (!cameraInfo.selectedCamera)
	{
		int    bindingKey[2];
		char   binding[2][32];
		vec4_t colour;
		float  y;

		VectorCopy(colorWhite, colour);
		colour[3] = .8f;
		y         = 442;

		CG_Text_Paint_Ext(8, y, .2f, .2f, colorRed, "Camera editor active",
		                  0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		trap_Key_KeysForBinding("dropobj", &bindingKey[0], &bindingKey[1]);
		trap_Key_KeynumToStringBuf(bindingKey[0], binding[0], sizeof(binding[0]));
		trap_Key_KeynumToStringBuf(bindingKey[1], binding[1], sizeof(binding[1]));
		Q_strupr(binding[0]);
		Q_strupr(binding[1]);
		CG_Text_Paint_Ext(8, y + 10, .2f, .2f, colour,
		                  va("Create new camera point: %s%s", bindingKey[0] != -1 ? binding[0] : "???",
		                     bindingKey[1] != -1 ? va(" or %s", binding[1]) : ""),
		                  0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		trap_Key_KeysForBinding("+attack", &bindingKey[0], &bindingKey[1]);
		trap_Key_KeynumToStringBuf(bindingKey[0], binding[0], sizeof(binding[0]));
		trap_Key_KeynumToStringBuf(bindingKey[1], binding[1], sizeof(binding[1]));
		Q_strupr(binding[0]);
		Q_strupr(binding[1]);
		CG_Text_Paint_Ext(8, y + 20, .2f, .2f, colour,
		                  va("Modify target camera point: %s%s", bindingKey[0] != -1 ? binding[0] : "???",
		                     bindingKey[1] != -1 ? va(" or %s", binding[1]) : ""),
		                  0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		// render crosshair
		CG_DrawCrosshair(&CG_GetActiveHUD()->crosshair);
	}
	else
	{
		// render interface
		// BG_PanelButtonsRender(cameraEditorButtons);

		// render cursor
		trap_R_SetColor(NULL);
		CG_DrawCursor(cgDC.cursorx, cgDC.cursory);
	}
}

void CG_ActivateCameraEditor(void)
{
	cg.editingCameras = qtrue;
	CG_EventHandling(CGAME_EVENT_CAMERAEDITOR, qfalse);
	CG_OverrideMouse(qtrue);
}

void CG_DeActivateCameraEditor(void)
{
	if (cgs.eventHandling == CGAME_EVENT_CAMERAEDITOR)
	{
		CG_EventHandling(-CGAME_EVENT_CAMERAEDITOR, qtrue);
		CG_OverrideMouse(qfalse);
	}
	cg.editingCameras = qfalse;
}

void CG_ClearCamera(void)
{
	Com_Memset(&cameraInfo, 0, sizeof(cameraEditorInfo));
}

static cameraPoint_t *CG_GetNewCameraPoint(void)
{
	if (cameraInfo.cameraPointOffset + 1 >= MAX_CAMERA_POINTS)
	{
		return NULL;
	}

	if (!cameraInfo.cameraPointOffset)
	{
		Com_Memset(cameraInfo.cameraPoints, 0, 50 * sizeof(cameraPoint_t));
		cameraInfo.currentCamera = &cameraInfo.cameraPoints[0];
	}

	return &cameraInfo.cameraPoints[cameraInfo.cameraPointOffset++];
}

void CG_CameraAddCurrentPoint(void)
{
	cameraPoint_t *point = CG_GetNewCameraPoint();
	cameraPoint_t *last  = cameraInfo.currentCamera;

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
	cameraPoint_t *last = cameraInfo.currentCamera;

	while (last && last->next)
	{
		last = last->next;
	}

	if (vec3_isClear(last->ctIn))
	{
		vec3_sub(cg.refdef.vieworg, last->origin, last->ctIn);
	}
	else if (last->prev)
	{
		vec3_sub(cg.refdef.vieworg, last->prev->origin, last->prev->ctOut);
	}

}

void CG_PlayCurrentCamera(unsigned int seconds)
{
	vec3_t        bezCt1, bezCt2;
	cameraPoint_t *last = cameraInfo.currentCamera;

	if (!seconds)
	{
		return;
	}

	cameraInfo.cameraTotalLength = 0.f;
	while (last && last->next)
	{
		if (!vec3_isClear(last->ctOut) || !vec3_isClear(last->next->ctIn))
		{
			vec3_add(last->origin, last->ctOut, bezCt1);
			vec3_add(last->next->origin, last->next->ctIn, bezCt2);
			last->len = CG_CalcBezierArcLengths(last->origin, bezCt1, bezCt2, last->next->origin, NULL, 20);
		}
		else
		{
			last->len = vec3_distance(last->origin, last->next->origin);
		}

		cameraInfo.cameraTotalLength += last->len;
		last                          = last->next;
	}

	CG_TeleportPlayer(cameraInfo.currentCamera->origin, cameraInfo.currentCamera->angles, qtrue);

	vec3_copy(cg.snap->ps.origin, cameraInfo.backupOrigin);
	vec3_copy(cg.snap->ps.viewangles, cameraInfo.backupAngles);

	cameraInfo.cameraUnitsInSecond  = cameraInfo.cameraTotalLength / (float) seconds;
	cameraInfo.currentPlayCamera    = cameraInfo.currentCamera;
	cgs.demoCamera.renderingFreeCam = qtrue;
}

void CG_RunCamera(void)
{
	cameraPoint_t *current, *next;
	vec3_t        bezCt1, bezCt2;

	if (!cameraInfo.currentPlayCamera || !cameraInfo.currentPlayCamera->next)
	{
		cameraInfo.currentPlayCamera = NULL;
		return;
	}

	current = cameraInfo.currentPlayCamera;
	next    = cameraInfo.currentPlayCamera->next;

	if (!vec3_isClear(current->ctOut) || !vec3_isClear(next->ctIn))
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
		float actualPoint       = cameraInfo.cameraPoint;
		vec_t expected          = current->len * cameraInfo.cameraPoint;
		vec3_add(current->origin, current->ctOut, bezCt1);
		vec3_add(next->origin, next->ctIn, bezCt2);

		// We need to calculate the lengths since the curve points are not in same distances
		CG_CalcBezierArcLengths(current->origin, bezCt1, bezCt2, next->origin, bezierLengths, len);
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

		CG_CalcBezierPoint(current->origin, bezCt1, bezCt2, next->origin, actualPoint, cgs.demoCamera.camOrigin);
#endif
	}
	else
	{
		vec3_lerp(current->origin, next->origin, cameraInfo.cameraPoint, cgs.demoCamera.camOrigin);
	}

	angles_lerp(current->angles, next->angles, cameraInfo.cameraPoint, cgs.demoCamera.camAngle);
	cgs.demoCamera.setCamAngles = qtrue;

	// CG_TeleportPlayer(cgs.demoCamera.camOrigin, cgs.demoCamera.camAngle);

	{
		float diff = ((float) (cg.time - cg.oldTime)) / 1000;

		// how much we should move
		float requiredMoveAmount = cameraInfo.cameraUnitsInSecond * diff;

		while (cameraInfo.currentPlayCamera)
		{
			float currentCameraOffset = cameraInfo.cameraPoint * cameraInfo.currentPlayCamera->len;

			if (currentCameraOffset + requiredMoveAmount > cameraInfo.currentPlayCamera->len)
			{
				requiredMoveAmount = (currentCameraOffset + requiredMoveAmount) - cameraInfo.currentPlayCamera->len;

				cameraInfo.currentPlayCamera = cameraInfo.currentPlayCamera->next;

				if (cameraInfo.currentPlayCamera)
				{
					// When we reach a camera point, teleport the player to that position so the in pvs entities are loaded correctly
					if (!cg.demoPlayback)
					{
						CG_TeleportPlayer(cameraInfo.currentPlayCamera->origin, cameraInfo.currentPlayCamera->angles, qtrue);
					}
					cameraInfo.cameraPoint = requiredMoveAmount / cameraInfo.currentPlayCamera->len;
				}
			}
			else
			{
				cameraInfo.cameraPoint += (requiredMoveAmount / cameraInfo.currentPlayCamera->len);
				break;
			}
		}
	}

	if (!cameraInfo.currentPlayCamera || !cameraInfo.currentPlayCamera->next)
	{
		CG_TeleportPlayer(cameraInfo.backupOrigin, cameraInfo.backupAngles, qtrue);
		cameraInfo.cameraPoint          = 0.f;
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
	cameraPoint_t *point     = cameraInfo.currentCamera;
	const float   bezierStep = 1.f / CAMERA_BEZIER_POINTS;

	minDist = Square(8.f);

	// don't render the route while playback
	if (cameraInfo.currentPlayCamera)
	{
		return;
	}

	if (point)
	{
		CG_AddCameraModel(point->origin, point->angles);
	}

	while (point)
	{
		if (cameraInfo.pointingCamera == point)
		{
			CG_AddOnScreenText(va(S_COLOR_RED "%i", ++offset), point->origin, qfalse);
		}
		else
		{
			CG_AddOnScreenText(va(S_COLOR_WHITE "%i", ++offset), point->origin, qfalse);
		}

		if (point->prev)
		{
			if (!vec3_isClear(point->prev->ctOut) || !vec3_isClear(point->ctIn))
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

		if (cameraInfo.pointingCamera == point)
		{
			// CG_DrawMoveGizmo(point->origin, -1);
			CG_DrawRotateGizmo(point->origin, GIZMO_DEFAULT_RADIUS, 40, -1);

			if (point->prev)
			{
				if (!vec3_isClear(point->ctIn))
				{
					vec3_add(point->origin, point->ctIn, vec);
					CG_DrawMoveGizmo(vec, GIZMO_DEFAULT_RADIUS, -1);
				}
				else
				{
					vec3_sub(point->prev->origin, point->origin, vec);
					vec3_norm(vec);
					vec3_ma(point->origin, GIZMO_DEFAULT_RADIUS * 2, vec, vec);
					CG_DrawMoveGizmo(vec, GIZMO_DEFAULT_RADIUS, -1);
				}
			}

			if (point->next)
			{
				if (!vec3_isClear(point->ctOut))
				{
					vec3_add(point->origin, point->ctOut, vec);
					CG_DrawMoveGizmo(vec, GIZMO_DEFAULT_RADIUS, -1);
				}
				else
				{
					vec3_sub(point->next->origin, point->origin, vec);
					vec3_norm(vec);
					vec3_ma(point->origin, GIZMO_DEFAULT_RADIUS * 2, vec, vec);
					CG_DrawMoveGizmo(vec, GIZMO_DEFAULT_RADIUS, -1);
				}
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

	cameraInfo.pointingCamera = closest;
}

void CG_CameraCommandComplete(void)
{
	int count = trap_Argc();

	if (count < 3)
	{
		trap_CommandComplete("open");
		trap_CommandComplete("close");
		trap_CommandComplete("add");
		trap_CommandComplete("ct");
		trap_CommandComplete("play");
		trap_CommandComplete("clear");
	}
}
