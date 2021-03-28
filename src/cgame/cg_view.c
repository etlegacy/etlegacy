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
 * @file cg_view.c
 * @brief Setup all the parameters (position, angle, etc) for a 3D rendering
 */

#include "cg_local.h"

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
default.cfg see CONFIG_NAME_DEFAULT

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/**
 * @brief Creates an entity in front of the current position, which
 * can then be moved around
 */
void CG_TestModel_f(void)
{
	vec3_t angles;

	Com_Memset(&cg.testModelEntity, 0, sizeof(cg.testModelEntity));
	if (trap_Argc() < 2)
	{
		return;
	}

	Q_strncpyz(cg.testModelName, CG_Argv(1), MAX_QPATH);
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);

	if (trap_Argc() == 3)
	{
		cg.testModelEntity.backlerp = (float)atof(CG_Argv(2));
		cg.testModelEntity.frame    = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (!cg.testModelEntity.hModel)
	{
		CG_Printf("Can't register model\n");
		return;
	}

	VectorMA(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin);

	angles[PITCH] = 0;
	angles[YAW]   = 180 + cg.refdefViewAngles[1];
	angles[ROLL]  = 0;

	AnglesToAxis(angles, cg.testModelEntity.axis);
	cg.testGun = qfalse;
}

/**
 * @brief Replaces the current view weapon with the given model
 */
void CG_TestGun_f(void)
{
	CG_TestModel_f();
	cg.testGun                  = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}

/**
 * @brief CG_TestModelNextFrame_f
 */
void CG_TestModelNextFrame_f(void)
{
	cg.testModelEntity.frame++;
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

/**
 * @brief CG_TestModelPrevFrame_f
 */
void CG_TestModelPrevFrame_f(void)
{
	cg.testModelEntity.frame--;
	if (cg.testModelEntity.frame < 0)
	{
		cg.testModelEntity.frame = 0;
	}
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

/**
 * @brief CG_TestModelNextSkin_f
 */
void CG_TestModelNextSkin_f(void)
{
	cg.testModelEntity.skinNum++;
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

/**
 * @brief CG_TestModelPrevSkin_f
 */
void CG_TestModelPrevSkin_f(void)
{
	cg.testModelEntity.skinNum--;
	if (cg.testModelEntity.skinNum < 0)
	{
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

/**
 * @brief CG_AddTestModel
 */
static void CG_AddTestModel(void)
{
	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);
	if (!cg.testModelEntity.hModel)
	{
		CG_Printf("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if (cg.testGun)
	{
		int i;

		VectorCopy(cg.refdef.vieworg, cg.testModelEntity.origin);
		VectorCopy(cg.refdef.viewaxis[0], cg.testModelEntity.axis[0]);
		VectorCopy(cg.refdef.viewaxis[1], cg.testModelEntity.axis[1]);
		VectorCopy(cg.refdef.viewaxis[2], cg.testModelEntity.axis[2]);

		// allow the position to be adjusted
		for (i = 0 ; i < 3 ; i++)
		{
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene(&cg.testModelEntity);
}

//============================================================================

/**
 * @brief Sets the coordinates of the rendered window
 * @param[in] xsize
 * @param[in] ysize
 * @param[in] center
 */
void CG_Letterbox(float xsize, float ysize, qboolean center)
{
	// normal aspect is xx:xx
	// letterbox is yy:yy  (85% of 'normal' height)
	if (cg_letterbox.integer)
	{
		float lbheight = ysize * 0.85f;
		float lbdiff   = ysize - lbheight;

		if (!center)
		{
			int offset = (cgs.glconfig.vidHeight * (.5f * lbdiff)) / 100;

			offset      &= ~1;
			cg.refdef.y += offset;
		}

		ysize = lbheight;
	}

	cg.refdef.width  = cgs.glconfig.vidWidth * xsize / 100;
	cg.refdef.width &= ~1;

	cg.refdef.height  = cgs.glconfig.vidHeight * ysize / 100;
	cg.refdef.height &= ~1;

	if (center)
	{
		cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width) / 2;
		cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height) / 2;
	}
}

/**
 * @brief CG_CalcVrect
 */
static void CG_CalcVrect(void)
{
	if (cg.showGameView)
	{
		float x = LIMBO_3D_X, y = LIMBO_3D_Y, w = LIMBO_3D_W, h = LIMBO_3D_H;

		// the limbopanel is horizontally centered
		x += cgs.wideXoffset;

		CG_AdjustFrom640(&x, &y, &w, &h);

		cg.refdef.x      = (int)x;
		cg.refdef.y      = (int)y;
		cg.refdef.width  = (int)w;
		cg.refdef.height = (int)h;

		CG_Letterbox((LIMBO_3D_W / 640.f) * 100, (LIMBO_3D_H / 480.f) * 100, qfalse);

		// the limbopanel objective camera is always rendered at a 4:3 aspectratio ...
		if (!Ccg_Is43Screen())
		{
			cg.refdef.width *= cgs.r43da;
		}

		return;
	}

	CG_Letterbox(100, 100, qtrue);
}

//==============================================================================

#ifdef FEATURE_EDV

extern void CG_DrawPVShint(void);

/**
 * @brief CG_OffsetFreeCamView
 */
static void CG_OffsetFreeCamView(void)
{
	if (cgs.demoCamera.renderingWeaponCam || cgs.demoCamera.setCamAngles)
	{
		VectorCopy(cgs.demoCamera.camAngle, cg.refdefViewAngles);
	}

	VectorCopy(cgs.demoCamera.camOrigin, cg.refdef_current->vieworg);

	if (demo_lookat.integer != -1)
	{
		centity_t *temp;
		vec3_t    dir;

		temp = &cg_entities[demo_lookat.integer];
		VectorSubtract(temp->lerpOrigin, cgs.demoCamera.camOrigin, dir);

		vectoangles(dir, cg.refdefViewAngles);
	}

	if (demo_pvshint.integer != 0)
	{
		CG_DrawPVShint();
	}
}
#endif

#define FOCUS_DISTANCE  400 //800   //512

/**
 * @brief CG_OffsetThirdPersonView
 */
void CG_OffsetThirdPersonView(void)
{
	vec3_t        forward, right, up;
	vec3_t        view;
	vec3_t        focusAngles;
	trace_t       trace;
	static vec3_t mins = { -4, -4, -4 };
	static vec3_t maxs = { 4, 4, 4 };
	vec3_t        focusPoint;
	float         focusDist;
	float         forwardScale, sideScale;

	cg.refdef_current->vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy(cg.refdefViewAngles, focusAngles);

	// If dead, look at medic or allow freelook if none in range
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0
	    && !(cg.predictedPlayerState.pm_flags & PMF_LIMBO))
	{
		// Force yaw to 0 if we're tracking a medic
		if (cg.snap->ps.viewlocked != VIEWLOCK_MEDIC)
		{
			// Do short2angle AFTER the network part
			focusAngles[YAW]         = SHORT2ANGLE(cg.predictedPlayerState.stats[STAT_DEAD_YAW]);
			cg.refdefViewAngles[YAW] = SHORT2ANGLE(cg.predictedPlayerState.stats[STAT_DEAD_YAW]);
		}
	}

	if (focusAngles[PITCH] > 45)
	{
		focusAngles[PITCH] = 45;        // don't go too far overhead
	}
	AngleVectors(focusAngles, forward, NULL, NULL);

	if (cg_thirdPerson.integer == 2)
	{
		VectorCopy(cg.predictedPlayerState.origin, focusPoint);
	}
	else
	{
		VectorMA(cg.refdef_current->vieworg, FOCUS_DISTANCE, forward, focusPoint);
	}

	VectorCopy(cg.refdef_current->vieworg, view);

	view[2]                    += 8;
	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	forwardScale = cos(cg_thirdPersonAngle.value / 180 * M_PI);
	sideScale    = sin(cg_thirdPersonAngle.value / 180 * M_PI);
	VectorMA(view, -cg_thirdPersonRange.value * forwardScale, forward, view);
	VectorMA(view, -cg_thirdPersonRange.value * sideScale, right, view);

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	CG_Trace(&trace, cg.refdef_current->vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);

	if (trace.fraction != 1.0f)
	{
		VectorCopy(trace.endpos, view);
		view[2] += (1.0f - trace.fraction) * 32.f;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out

		CG_Trace(&trace, cg.refdef_current->vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);
		VectorCopy(trace.endpos, view);
	}

	VectorCopy(view, cg.refdef_current->vieworg);

	// select pitch to look at focus point from vieword
	VectorSubtract(focusPoint, cg.refdef_current->vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if (focusDist < 1)
	{
		focusDist = 1;  // should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
	cg.refdefViewAngles[YAW]  -= cg_thirdPersonAngle.value;
}

/**
 * @brief CG_StepOffset
 *
 * @warning This causes a compiler bug on mac MrC compiler
 */
static void CG_StepOffset(void)
{
	// smooth out stair climbing
	int timeDelta = cg.time - cg.stepTime;

	if (timeDelta < 0)
	{
		cg.stepTime = cg.time;
	}
	if (timeDelta < STEP_TIME)
	{
		cg.refdef_current->vieworg[2] -= cg.stepChange * (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

/**
 * @brief CG_KickAngles
 */
void CG_KickAngles(void)
{
	const vec3_t centerSpeed = { 2400, 2400, 2400 };
	const float  recoilCenterSpeed = 200;
	const float  recoilIgnoreCutoff = 15;
	const float  recoilMaxSpeed = 50;
	const vec3_t maxKickAngles = { 10, 10, 10 };
	float        idealCenterSpeed, kickChange;
	int          i, frametime, t;
	float        ft;
#define STEP 20
	char buf[32];

	// this code is frametime-dependant, so split it up into small chunks
	//cg.kickAngles[PITCH] = 0;
	cg.recoilPitchAngle = 0;
	for (t = cg.frametime; t > 0; t -= STEP)
	{
		if (t > STEP)
		{
			frametime = STEP;
		}
		else
		{
			frametime = t;
		}

		ft = frametime / 1000.f;

		// kickAngles is spring-centered
		for (i = 0; i < 3; i++)
		{
			if (cg.kickAVel[i] != 0.f || cg.kickAngles[i] != 0.f)
			{
				// apply centering forces to kickAvel
				if (cg.kickAngles[i] != 0.f && frametime)
				{
					idealCenterSpeed = -(2.0f * (cg.kickAngles[i] > 0) - 1.0f) * centerSpeed[i];
					if (idealCenterSpeed != 0.f)
					{
						cg.kickAVel[i] += idealCenterSpeed * ft;
					}
				}
				// add the kickAVel to the kickAngles
				kickChange = cg.kickAVel[i] * ft;
				if (cg.kickAngles[i] != 0.f && (cg.kickAngles[i] < 0) != (kickChange < 0))         // slower when returning to center
				{
					kickChange *= 0.06;
				}
				// check for crossing back over the center point
				if (cg.kickAngles[i] == 0.f || ((cg.kickAngles[i] + kickChange) < 0) == (cg.kickAngles[i] < 0))
				{
					cg.kickAngles[i] += kickChange;
					if (cg.kickAngles[i] == 0.f && frametime)
					{
						cg.kickAVel[i] = 0;
					}
					else if (Q_fabs(cg.kickAngles[i]) > maxKickAngles[i])
					{
						cg.kickAngles[i] = maxKickAngles[i] * ((2 * (cg.kickAngles[i] > 0)) - 1);
						cg.kickAVel[i]   = 0; // force Avel to return us to center rather than keep going outside range
					}
				}
				else     // about to cross, so just zero it out
				{
					cg.kickAngles[i] = 0;
					cg.kickAVel[i]   = 0;
				}
			}
		}

		// recoil is added to input viewangles per frame
		if (cg.recoilPitch != 0.f)
		{
			// apply max recoil
			if (Q_fabs(cg.recoilPitch) > recoilMaxSpeed)
			{
				if (cg.recoilPitch > 0)
				{
					cg.recoilPitch = recoilMaxSpeed;
				}
				else
				{
					cg.recoilPitch = -recoilMaxSpeed;
				}
			}
			// apply centering forces to kickAvel
			if (frametime)
			{
				idealCenterSpeed = -(2.0f * (cg.recoilPitch > 0) - 1.0f) * recoilCenterSpeed * ft;
				if (idealCenterSpeed != 0.f)
				{
					if (Q_fabs(idealCenterSpeed) < Q_fabs(cg.recoilPitch))
					{
						cg.recoilPitch += idealCenterSpeed;
					}
					else        // back zero out
					{
						cg.recoilPitch = 0;
					}
				}
			}
		}
		if (Q_fabs(cg.recoilPitch) > recoilIgnoreCutoff)
		{
			cg.recoilPitchAngle += cg.recoilPitch * ft;
		}
	}

	// only change cg_recoilPitch cvar when we need to
	trap_Cvar_VariableStringBuffer("cg_recoilPitch", buf, sizeof(buf));

	if (atof(buf) != cg.recoilPitchAngle)
	{
		// encode the kick angles into a 24bit number, for sending to the client exe
		trap_Cvar_Set("cg_recoilPitch", va("%f", cg.recoilPitchAngle));
	}
}

/**
 * @brief Sway for scoped weapons.
 * this takes aimspread into account so the view settles after a bit
 */
static void CG_ZoomSway(void)
{
	float spreadfrac;
	float phase;

	if (cg.zoomval == 0.f)     // not zoomed
	{
		return;
	}

	if ((cg.snap->ps.eFlags & EF_MG42_ACTIVE) || (cg.snap->ps.eFlags & EF_AAGUN_ACTIVE))     // don't draw when on mg_42
	{
		return;
	}

	spreadfrac = cg.snap->ps.aimSpreadScale / 255.0f;

	phase                       = cg.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_TAU_F;
	cg.refdefViewAngles[PITCH] += ZOOM_PITCH_AMPLITUDE * sin(phase) * (spreadfrac + ZOOM_PITCH_MIN_AMPLITUDE);

	phase                     = cg.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_TAU_F;
	cg.refdefViewAngles[YAW] += ZOOM_YAW_AMPLITUDE * sin(phase) * (spreadfrac + ZOOM_YAW_MIN_AMPLITUDE);
}

/**
 * @brief CG_OffsetFirstPersonView
 */
static void CG_OffsetFirstPersonView(void)
{
	vec3_t forward;
	float  *origin;
	float  *angles;
	float  bob;
	float  delta;
	float  speed;
	float  f;
	int    timeDelta;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	origin = cg.refdef_current->vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if (!(cg.snap->ps.pm_flags & PMF_LIMBO) && cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		angles[ROLL]  = 40;
		angles[PITCH] = -15;

		// force yaw to 0 if we're tracking a medic
		// medic tracking doesn't seem to happen in this case?
		if (cg.snap->ps.viewlocked == VIEWLOCK_MEDIC)
		{
			angles[YAW] = 0;
		}
		else
		{
			// do short2angle AFTER the network part
			angles[YAW] = SHORT2ANGLE(cg.snap->ps.stats[STAT_DEAD_YAW]);
		}

		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	VectorAdd(angles, cg.kick_angles, angles);

	// add new weapon kick angles
	CG_KickAngles();
	VectorAdd(angles, cg.kickAngles, angles);
	// pitch is already added
	//angles[0] -= cg.kickAngles[PITCH];

	// add angles based on damage kick
	if (cg.damageTime != 0.f)
	{
		float ratio = cg.time - cg.damageTime;

		if (ratio < DAMAGE_DEFLECT_TIME)
		{
			ratio         /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL]  += ratio * cg.v_dmg_roll;
		}
		else
		{
			ratio = 1.0f - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
			if (ratio > 0)
			{
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL]  += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = (cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
	{
		ratio = 0;
	}
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on bob
	if (cg_bobbing.integer)
	{
		qboolean useLastValidBob = qfalse;

		// make sure the bob is visible even at low speeds
		speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

		if (cg.bobfracsin == 0.f && cg.lastvalidBobfracsin > 0)
		{
			// 200 msec to get back to center from 1
			// that's 1/200 per msec = 0.005 per msec
			cg.lastvalidBobfracsin -= 0.005 * cg.frametime;
			useLastValidBob         = qtrue;
		}

		delta = useLastValidBob ? cg.lastvalidBobfracsin * 0.002 * speed : cg.bobfracsin * 0.002 * speed;
		if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		{
			delta *= 3;     // crouching
		}

		angles[PITCH] += delta;
		delta          = useLastValidBob ? cg.lastvalidBobfracsin * 0.002 * speed : cg.bobfracsin * 0.002 * speed;
		if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		{
			delta *= 3;     // crouching accentuates roll
		}
		if (useLastValidBob)
		{
			if (cg.lastvalidBobcycle & 1)
			{
				delta = -delta;
			}
		}
		else if (cg.bobcycle & 1)
		{
			delta = -delta;
		}
		angles[ROLL] += delta;
	}

//===================================

	AngleVectors(cg.refdefViewAngles, forward, NULL, NULL);
	forward[2] = 0;
	VectorNormalizeFast(forward);

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if (cg.predictedPlayerState.eFlags & EF_PRONE)
	{
		// move the view origin a bit forward (limit between body and head BBox)
		origin[0] += forward[0] * 18;
		origin[1] += forward[1] * 18;

		if (timeDelta < 0)
		{
			cg.duckTime = cg.time - PRONE_TIME;
		}
		if (timeDelta < PRONE_TIME)
		{
			cg.refdef_current->vieworg[0] -= (forward[0] * 18) * (PRONE_TIME - timeDelta) / PRONE_TIME;
			cg.refdef_current->vieworg[1] -= (forward[1] * 18) * (PRONE_TIME - timeDelta) / PRONE_TIME;
			cg.refdef_current->vieworg[2] -= cg.duckChange * (PRONE_TIME - timeDelta) / PRONE_TIME;
		}
	}
	else
	{
		if (timeDelta < 0)
		{
			cg.duckTime = cg.time - DUCK_TIME;
		}
		if (timeDelta < DUCK_TIME)
		{
			if (cg.wasProne)
			{
				cg.refdef_current->vieworg[0] += (forward[0] * 18) * (DUCK_TIME - timeDelta) / DUCK_TIME;
				cg.refdef_current->vieworg[1] += (forward[1] * 18) * (DUCK_TIME - timeDelta) / DUCK_TIME;
			}
			cg.refdef_current->vieworg[2] -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
		}
	}

	// add bob height
	if (cg_bobbing.integer)
	{
		bob = cg.bobfracsin * cg.xyspeed * 0.005;
		if (bob > 6)
		{
			bob = 6;
		}
		if (bob < 0)
		{
			bob = 0;
		}

		origin[2] += bob;
	}

	// add fall height
	delta = cg.time - cg.landTime;
	if (delta < 0)
	{
		cg.landTime = cg.time - (LAND_DEFLECT_TIME + LAND_RETURN_TIME);
	}
	if (delta < LAND_DEFLECT_TIME)
	{
		f                              = delta / LAND_DEFLECT_TIME;
		cg.refdef_current->vieworg[2] += cg.landChange * f;
	}
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
	{
		delta                         -= LAND_DEFLECT_TIME;
		f                              = 1.0f - (delta / LAND_RETURN_TIME);
		cg.refdef_current->vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	CG_ZoomSway();

	// adjust for 'lean'
	if (cg.predictedPlayerState.leanf != 0.f)
	{
		// add leaning offset
		vec3_t right;

		cg.refdefViewAngles[2] += cg.predictedPlayerState.leanf / 2.0f;
		AngleVectors(cg.refdefViewAngles, NULL, right, NULL);
		VectorMA(cg.refdef_current->vieworg, cg.predictedPlayerState.leanf, right, cg.refdef_current->vieworg);
	}

	// add kick offset
	VectorAdd(origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define NECK_LENGTH     8
		vec3_t forward, up;

		cg.refdef_current->vieworg[2] -= NECK_LENGTH;
		AngleVectors(cg.refdefViewAngles, forward, NULL, up);
		VectorMA(cg.refdef_current->vieworg, 3, forward, cg.refdef_current->vieworg);
		VectorMA(cg.refdef_current->vieworg, NECK_LENGTH, up, cg.refdef_current->vieworg);
	}
#endif
}

//======================================================================

// Zoom controls

/**
 * @brief CG_AdjustZoomVal
 * @param[in] val
 * @param[in] type
 */
void CG_AdjustZoomVal(float val, int zoomOut, int zoomIn)
{
	cg.zoomval += val;
	if (cg.zoomval > zoomOut)
	{
		cg.zoomval = zoomOut;
	}
	if (cg.zoomval < zoomIn)
	{
		cg.zoomval = zoomIn;
	}
}

/**
 * @brief CG_ZoomIn_f
 */
void CG_ZoomIn_f(void)
{
	// fixed being able to "latch" your zoom by weaponcheck + quick zoomin
	// - change for zoom view in demos
	// zoom if weapon is scoped or if binoc is scoped
	if (GetWeaponTableData(cg_entities[cg.snap->ps.clientNum].currentState.weapon)->type & WEAPON_TYPE_SCOPED)
	{
		CG_AdjustZoomVal(-(cg_zoomStepSniper.value)
		                 , GetWeaponTableData(cg_entities[cg.snap->ps.clientNum].currentState.weapon)->zoomOut
		                 , GetWeaponTableData(cg_entities[cg.snap->ps.clientNum].currentState.weapon)->zoomIn);
	}
	else if (cg.zoomedBinoc)    // case where the binocular is used but not holded (equipped)
	{
		CG_AdjustZoomVal(-(cg_zoomStepSniper.value)
		                 , GetWeaponTableData(WP_BINOCULARS)->zoomOut
		                 , GetWeaponTableData(WP_BINOCULARS)->zoomIn);
	}
}

/**
 * @brief CG_ZoomOut_f
 */
void CG_ZoomOut_f(void)
{
	// zoom if weapon is scoped or if binoc is scoped
	if (GetWeaponTableData(cg_entities[cg.snap->ps.clientNum].currentState.weapon)->type & WEAPON_TYPE_SCOPED)
	{
		CG_AdjustZoomVal(cg_zoomStepSniper.value
		                 , GetWeaponTableData(cg_entities[cg.snap->ps.clientNum].currentState.weapon)->zoomOut
		                 , GetWeaponTableData(cg_entities[cg.snap->ps.clientNum].currentState.weapon)->zoomIn);
	}
	else if (cg.zoomedBinoc)    // case where the binocular is used but not holded (equipped)
	{
		CG_AdjustZoomVal(cg_zoomStepSniper.value
		                 , GetWeaponTableData(WP_BINOCULARS)->zoomOut
		                 , GetWeaponTableData(WP_BINOCULARS)->zoomIn);
	}
}

/**
 * @brief Check for scope wepon in use, and change if necessary - spec/demo scaling allowances
 * or show a zoomed binoculars view .. (still not actively zooming in/out)
 */
void CG_Zoom(void)
{
	int weapon;

	// fix for demo playback
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		cg.predictedPlayerState.eFlags = cg.snap->ps.eFlags;
		weapon                         = cg.predictedPlayerState.weapon = cg.snap->ps.weapon;
	}
	else
	{
		weapon = cg.weaponSelect;
	}

	if ((cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
#ifdef FEATURE_EDV
	    || cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam            //fix for edv
#endif
	    )
	{
		cg.zoomedBinoc = qfalse;
		cg.zoomed      = qfalse;
		cg.zoomTime    = 0;
		cg.zoomval     = 0;
	}
	else if (cg.predictedPlayerState.eFlags & EF_ZOOMING)       // binoc
	{
		if (cg.zoomedBinoc)
		{
			return;
		}

		cg.zoomedBinoc = qtrue;
		cg.zoomTime    = cg.time;
		cg.zoomval     = cg_zoomDefaultSniper.value; // was DefaultBinoc, changed per atvi req
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_SCOPED) // check for scope weapon in use, and change to if necessary
	{
		if (cg.zoomed)
		{
			return;
		}

		cg.zoomed   = qtrue;
		cg.zoomTime = cg.time;
		cg.zoomval  = cg_zoomDefaultSniper.value;    // was DefaultFG, changed per atvi req
	}
	else
	{
		if (cg.zoomedBinoc || cg.zoomed)
		{
			cg.zoomedBinoc = qfalse;
			cg.zoomed      = qfalse;
			cg.zoomTime    = cg.time;
			cg.zoomval     = 0;
		}
		else
		{
			// we now sanity check to make sure we can't zoom non-zoomable weapons
			// but don't sanity check while following
			if (!((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback))
			{
				cg.zoomval = 0;
			}
		}
	}
}

#define WAVE_AMPLITUDE  1
#define WAVE_FREQUENCY  0.4

/**
 * @brief Fixed fov at intermissions, otherwise account for fov variable and zooms.
 * @return
 */
static int CG_CalcFov(void)
{
	static float lastfov = 90;      // for transitions back from zoomed in modes
	float        x;
	int          contents;
	float        fov_x = 0.0f, fov_y;
	int          inwater;

	CG_Zoom();

	if (cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		fov_x = cg_fov.value;
		if (!developer.integer)
		{
			if (fov_x < 90)
			{
				fov_x = 90;
			}
			else if (fov_x > 160)
			{
				fov_x = 160;
			}
		}

		if (!cg.renderingThirdPerson || developer.integer)
		{
			float zoomFov;
			float f;

			// account for zooms
			if (cg.zoomval != 0.f)
			{
				zoomFov = cg.zoomval;   // use user scrolled amount

				if (zoomFov < 1)
				{
					zoomFov = 1;
				}
				else if (zoomFov > 160)
				{
					zoomFov = 160;
				}
			}
			else
			{
				zoomFov = lastfov;
			}

			// do smooth transitions for the binocs
			if (cg.zoomedBinoc)         // binoc zooming in
			{
				f = (cg.time - cg.zoomTime) / ZOOM_TIME;
				if (f > 1.0f)
				{
					fov_x = zoomFov;
				}
				else
				{
					fov_x = fov_x + f * (zoomFov - fov_x);
				}
				lastfov = fov_x;
			}
			else if (cg.zoomed)        // zoomed by sniper/snooper
			{
				fov_x   = cg.zoomval;
				lastfov = fov_x;
			}
			else                        // binoc zooming out
			{
				f = (cg.time - cg.zoomTime) / ZOOM_TIME;
				if (f > 1.0f)
				{
					// fov_x = fov_x;
				}
				else
				{
					fov_x = zoomFov + f * (fov_x - zoomFov);
				}
			}
		}
	}

	cg.refdef_current->rdflags &= ~RDF_SNOOPERVIEW;

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		//do nothing
	}
	else
#endif
	// mg42 zoom
	if (cg.snap->ps.persistant[PERS_HWEAPON_USE])
	{
		fov_x = 55;
	}
	else if (CHECKBITWISE(GetWeaponTableData(cg.snap->ps.weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
	{
		fov_x = 55;
	}
	else if (cg.snap->ps.eFlags & EF_MOUNTEDTANK)
	{
		fov_x = 75;
	}

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		// if in intermission, use a fixed value
		fov_x = 90;
	}

	if (cg.showGameView)
	{
		fov_x = 60.f;
		// fov_y = 60.f; // this isn't used and overwritten below
	}

	// automatic fov adjustment for wide screens
	fov_x = atan(tan(fov_x * M_PI / 360.0f) * 0.75f * (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight) * 360.0f / M_PI;

	// this is weird... (but ensures square pixel ratio!)
	x     = cg.refdef_current->width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef_current->height, x);
	fov_y = fov_y * 360 / M_PI;
	// And this seems better - but isn't really
	//fov_y = fov_x / cgs.glconfig.windowAspect;

	// warp if underwater
	//if ( cg_pmove.waterlevel == 3 ) {
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_TAU_F;
		float v     = WAVE_AMPLITUDE * sin(phase);

		fov_x                      += v;
		fov_y                      -= v;
		inwater                     = qtrue;
		cg.refdef_current->rdflags |= RDF_UNDERWATER;
	}
	else
	{
		cg.refdef_current->rdflags &= ~RDF_UNDERWATER;
		inwater                     = qfalse;
	}

	// set it
	cg.refdef_current->fov_x = fov_x;
	cg.refdef_current->fov_y = fov_y;

	/*
	    if( cg.predictedPlayerState.eFlags & EF_PRONE ) {
	        cg.zoomSensitivity = cg.refdef.fov_y / 500.0;
	    } else
	*/
	// allow freelook when dead until we tap out into limbo
	if (cg.snap->ps.pm_type == PM_FREEZE || (cg.snap->ps.pm_type == PM_DEAD && (cg.snap->ps.pm_flags & PMF_LIMBO)) || (cg.snap->ps.pm_flags & PMF_TIME_LOCKPLAYER))
	{
		// No movement for pauses
		cg.zoomSensitivity = 0;
	}
	else if (!cg.zoomedBinoc)
	{
		// fix for zoomed in/out movement bug
		if (cg.zoomval != 0.f)
		{
			cg.zoomSensitivity = 0.6f * (cg.zoomval / 90.f);     // changed to get less sensitive as you zoom in
		}
		else
		{
			cg.zoomSensitivity = 1;
		}
	}
	else
	{
		cg.zoomSensitivity = cg.refdef_current->fov_y / 75.0f;
	}

	return inwater;
}

#define UNDERWATER_BIT 16

/**
 * @brief CG_UnderwaterSounds
 */
static void CG_UnderwaterSounds(void)
{
	trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.underWaterSound, 255 | (1 << UNDERWATER_BIT), 0);
}

/**
 * @brief CG_DamageBlendBlob
 */
static void CG_DamageBlendBlob(void)
{
	int i;

	if (cg_bloodDamageBlend.value <= 0.f)
	{
		return;
	}

	// no damage blend blobs if in limbo or spectator, and in the limbo menu
	if (((cg.snap->ps.pm_flags & PMF_LIMBO) || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) && cg.showGameView)
	{
		return;
	}

	for (i = 0; i < MAX_VIEWDAMAGE; i++)
	{
		int          maxTime;
		int          t;
		refEntity_t  ent;
		viewDamage_t *vd = &cg.viewDamage[i];

		if (vd->damageValue == 0.f)
		{
			continue;
		}

		maxTime = vd->damageDuration;
		t       = cg.time - vd->damageTime;
		if (t <= 0 || t >= maxTime)
		{
			vd->damageValue = 0;
			continue;
		}

		// if not point Damage, only do flash blend
		if (vd->damageX == 0.f && vd->damageY == 0.f)
		{
			continue;
		}

		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType   = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		VectorMA(cg.refdef_current->vieworg, 8, cg.refdef_current->viewaxis[0], ent.origin);
		VectorMA(ent.origin, vd->damageX * -10, cg.refdef_current->viewaxis[1], ent.origin);
		VectorMA(ent.origin, vd->damageY * 6, cg.refdef_current->viewaxis[2], ent.origin);

		ent.radius = vd->damageValue * 0.4f * (0.5f + 0.5f * (float)t / maxTime) * (0.75f + 0.5f * Q_fabs((float)sin(vd->damageTime)));

		ent.customShader  = cgs.media.viewBloodAni[(int)(floor(((double)t / maxTime) * 4.9))];      //cgs.media.viewBloodShader;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = (byte)(255 * Com_Clamp(0.f, 1.f, cg_bloodDamageBlend.value));

		trap_R_AddRefEntityToScene(&ent);
	}
}

/**
 * @brief Sets cg.refdef view values
 * @return
 */
int CG_CalcViewValues(void)
{
	playerState_t *ps;

	Com_Memset(cg.refdef_current, 0, sizeof(cg.refdef));

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	// intermission view
	if (ps->pm_type == PM_INTERMISSION)
	{
		VectorCopy(ps->origin, cg.refdef_current->vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef_current->viewaxis);

		return CG_CalcFov();
	}

	if (cg.bobfracsin > 0 && !ps->bobCycle)
	{
		cg.lastvalidBobcycle   = cg.bobcycle;
		cg.lastvalidBobfracsin = cg.bobfracsin;
	}

	cg.bobcycle   = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed    = sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);

	if (cg.showGameView)
	{
		VectorCopy(cgs.ccPortalPos, cg.refdef_current->vieworg);
		if (cg.showGameView && cgs.ccPortalEnt != -1)
		{
			vec3_t vec;

			VectorSubtract(cg_entities[cgs.ccPortalEnt].lerpOrigin, cg.refdef_current->vieworg, vec);
			vectoangles(vec, cg.refdefViewAngles);
		}
		else
		{
			VectorCopy(cgs.ccPortalAngles, cg.refdefViewAngles);
		}
	}
#ifdef FEATURE_EDV
	else if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		//do nothing
	}
#endif

	else if (cg.renderingThirdPerson && ((ps->eFlags & EF_MG42_ACTIVE) || (ps->eFlags & EF_AAGUN_ACTIVE))) // see if we're attached to a gun
	{
		centity_t *mg42 = &cg_entities[ps->viewlocked_entNum];
		vec3_t    forward;

		AngleVectors(ps->viewangles, forward, NULL, NULL);
		if (ps->eFlags & EF_AAGUN_ACTIVE)
		{
			VectorMA(mg42->currentState.pos.trBase, -40, forward, cg.refdef_current->vieworg);
		}
		else
		{
			VectorMA(mg42->currentState.pos.trBase, -36, forward, cg.refdef_current->vieworg);
		}

		cg.refdef_current->vieworg[2] = ps->origin[2];
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}
	else if (ps->eFlags & EF_MOUNTEDTANK)
	{
		centity_t *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

		VectorCopy(tank->mountedMG42Player.origin, cg.refdef_current->vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}
	else
	{
#ifdef FEATURE_EDV
		if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
		{
#endif
		VectorCopy(ps->origin, cg.refdef_current->vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
#ifdef FEATURE_EDV
	}
#endif
	}

	if (!cg.showGameView)
	{
		// add error decay
		if (cg_errorDecay.value > 0)
		{
			int   t = cg.time - cg.predictedErrorTime;
			float f, errorDecay = cg_errorDecay.value;

			if (errorDecay > 500)
			{
				errorDecay = 500;
			}

			f = (errorDecay - t) / errorDecay;

			if (f > 0 && f < 1)
			{
				VectorMA(cg.refdef_current->vieworg, f, cg.predictedError, cg.refdef_current->vieworg);
			}
			else
			{
				cg.predictedErrorTime = 0;
			}
		}

		// lock the viewangles if the game has told us to
#ifdef FEATURE_EDV
		if (ps->viewlocked && !cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#else
		if (ps->viewlocked)
#endif
		{
			// don't bother evaluating if set to 7 (look at medic)
			if (ps->viewlocked != VIEWLOCK_MEDIC && ps->viewlocked != VIEWLOCK_MG42 && ps->viewlocked != VIEWLOCK_JITTER)
			{
				BG_EvaluateTrajectory(&cg_entities[ps->viewlocked_entNum].currentState.apos, cg.time, cg.refdefViewAngles, qtrue, cg_entities[ps->viewlocked_entNum].currentState.effect2Time);
			}

			if (ps->viewlocked == VIEWLOCK_JITTER)
			{
				cg.refdefViewAngles[0] += crandom();
				cg.refdefViewAngles[1] += crandom();
			}
		}

#ifdef FEATURE_EDV
		if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
		{
			CG_OffsetFreeCamView();
		}
		else if (cg.renderingThirdPerson)
		{
			VectorCopy(cg.refdef.vieworg, cgs.demoCamera.camOrigin);
			VectorCopy(cg.refdefViewAngles, cgs.demoCamera.camAngle);
#else
		if (cg.renderingThirdPerson)
		{
#endif
			// back away from character
			CG_OffsetThirdPersonView();
		}
		else
		{
#ifdef FEATURE_EDV
			VectorCopy(cg.refdef.vieworg, cgs.demoCamera.camOrigin);
			VectorCopy(cg.refdefViewAngles, cgs.demoCamera.camAngle);
#endif

			// offset for local bobbing and kicks
			CG_OffsetFirstPersonView();

			if (cg.editingSpeakers)
			{
				CG_SetViewanglesForSpeakerEditor();
			}
		}

		// lock the viewangles if the game has told us to
#ifdef FEATURE_EDV
		//freecam must not get viewlocked
		if (ps->viewlocked && (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam))
		{
			//do nothing
		}
		else
#endif
		if (ps->viewlocked == VIEWLOCK_MEDIC)
		{
			centity_t *tent = &cg_entities[ps->viewlocked_entNum];
			vec3_t    vec;

			VectorCopy(tent->lerpOrigin, vec);
			VectorSubtract(vec, cg.refdef_current->vieworg, vec);
			vectoangles(vec, cg.refdefViewAngles);
		}
		else if (ps->viewlocked)
		{
			vec3_t fwd;
			float  oldZ = cg.refdef_current->vieworg[2]; // set our position to be behind it

			AngleVectors(cg.refdefViewAngles, fwd, NULL, NULL);
			if (cg.predictedPlayerState.eFlags & EF_AAGUN_ACTIVE)
			{
				VectorMA(cg_entities[ps->viewlocked_entNum].lerpOrigin, 0, fwd, cg.refdef_current->vieworg);
			}
			else
			{
				VectorMA(cg_entities[ps->viewlocked_entNum].lerpOrigin, -34, fwd, cg.refdef_current->vieworg);
			}
			cg.refdef_current->vieworg[2] = oldZ;
		}
	}

#ifdef FEATURE_EDV
	if (cgs.demoCamera.startLean)
	{
		cg.refdefViewAngles[2] = 0.0;
	}

	cgs.demoCamera.startLean = qfalse;
#endif

	// position eye relative to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef_current->viewaxis);

	if (cg.hyperspace)
	{
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}

//=========================================================================

/**
 * @brief CG_MustParse
 * @param[in] pString
 * @param[in] pErrorMsg
 * @return
 */
char *CG_MustParse(char **pString, const char *pErrorMsg)
{
	char *token = COM_Parse(pString);

	if (!token[0])
	{
		CG_Error("%s", pErrorMsg);
	}
	return token;
}

/**
 * @brief CG_ParseSkyBox
 */
void CG_ParseSkyBox(void)
{
	char *cstr, *token;

	cstr = (char *)CG_ConfigString(CS_SKYBOXORG);

	if (!*cstr)
	{
		cg.skyboxEnabled = qfalse;
		return;
	}

	token               = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring. No skyboxViewOrg[0]\n");
	cg.skyboxViewOrg[0] = (float)atof(token);

	token               = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring. No skyboxViewOrg[1]\n");
	cg.skyboxViewOrg[1] = (float)atof(token);

	token               = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring. No skyboxViewOrg[2]\n");
	cg.skyboxViewOrg[2] = (float)atof(token);

	token            = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring. No skyboxViewFov\n");
	cg.skyboxViewFov = Q_atoi(token);

	if (cg.skyboxViewFov == 0.f)
	{
		cg.skyboxViewFov = 90;
	}

	// setup fog the first time, ignore this part of the configstring after that
	token = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring. No fog state\n");
	if (atoi(token))         // this camera has fog
	{
		vec4_t fogColor;
		int fogStart, fogEnd;

		token       = CG_MustParse(&cstr, "CG_DrawSkyBoxPortal: error parsing skybox configstring. No fog[0]\n");
		fogColor[0] = (float)atof(token);

		token       = CG_MustParse(&cstr, "CG_DrawSkyBoxPortal: error parsing skybox configstring. No fog[1]\n");
		fogColor[1] = (float)atof(token);

		token       = CG_MustParse(&cstr, "CG_DrawSkyBoxPortal: error parsing skybox configstring. No fog[2]\n");
		fogColor[2] = (float)atof(token);

		token    = COM_ParseExt(&cstr, qfalse);
		fogStart = Q_atoi(token);

		token  = COM_ParseExt(&cstr, qfalse);
		fogEnd = Q_atoi(token);

		trap_R_SetFog(FOG_PORTALVIEW, fogStart, fogEnd, fogColor[0], fogColor[1], fogColor[2], 1.1f);
	}
	else
	{
		trap_R_SetFog(FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0);      // init to null
	}

	cg.skyboxEnabled = qtrue;
}

/**
 * @brief CG_ParseTagConnects
 */
void CG_ParseTagConnects(void)
{
	int i;

	for (i = CS_TAGCONNECTS; i < CS_TAGCONNECTS + MAX_TAGCONNECTS; i++)
	{
		CG_ParseTagConnect(i);
	}
}

/**
 * @brief CG_ParseTagConnect
 * @param tagNum
 */
void CG_ParseTagConnect(int tagNum)
{
	char *token, *pString = (char *)CG_ConfigString(tagNum);  //  bleh, i hate that cast away of the const
	int entNum;

	if (!*pString)
	{
		return;
	}

	token = CG_MustParse(&pString, "Invalid TAGCONNECT configstring\n");

	entNum = Q_atoi(token);
	if (entNum < 0 || entNum >= MAX_GENTITIES)
	{
		CG_Error("Invalid TAGCONNECT entitynum\n");
	}

	token = CG_MustParse(&pString, "Invalid TAGCONNECT configstring\n");

	cg_entities[entNum].tagParent = Q_atoi(token);
	if (cg_entities[entNum].tagParent < 0 || cg_entities[entNum].tagParent >= MAX_GENTITIES)
	{
		CG_Error("Invalid TAGCONNECT tagparent\n");
	}

	token = CG_MustParse(&pString, "Invalid TAGCONNECT configstring\n");
	Q_strncpyz(cg_entities[entNum].tagName, token, MAX_QPATH);
}

/**
 * @brief CG_DrawSkyBoxPortal
 * @param[in] fLocalView
 */
void CG_DrawSkyBoxPortal(qboolean fLocalView)
{
	refdef_t rd;
	static float lastfov = 90;      // for transitions back from zoomed in modes

	if (!cg_skybox.integer || !cg.skyboxEnabled)
	{
		return;
	}

	Com_Memcpy(&rd, cg.refdef_current, sizeof(refdef_t));
	VectorCopy(cg.skyboxViewOrg, rd.vieworg);

	// Updates for window views...
	if (fLocalView)
	{
		float fov_x;
		float fov_y;
		float x;

		if (cg.predictedPlayerState.pm_type == PM_INTERMISSION)
		{
			// if in intermission, use a fixed value
			fov_x = 90;
		}
		else
		{
			float zoomFov;
			float f;

			// user selectable
			fov_x = cg_fov.value;
			if (fov_x < 1)
			{
				fov_x = 1;
			}
			else if (fov_x > 160)
			{
				fov_x = 160;
			}

			// account for zooms
			if (cg.zoomval != 0.f)
			{
				zoomFov = cg.zoomval;   // use user scrolled amount
				if (zoomFov < 1)
				{
					zoomFov = 1;
				}
				else if (zoomFov > 160)
				{
					zoomFov = 160;
				}
			}
			else
			{
				zoomFov = lastfov;
			}

			// do smooth transitions for the binocs
			if (cg.zoomedBinoc)            // binoc zooming in
			{
				f       = (cg.time - cg.zoomTime) / ZOOM_TIME;
				fov_x   = (f > 1.0f) ? zoomFov : fov_x + f * (zoomFov - fov_x);
				lastfov = fov_x;
			}
			else if (cg.zoomval != 0.f)       // zoomed by sniper/snooper
			{
				fov_x   = cg.zoomval;
				lastfov = fov_x;
			}
			else                        // binoc zooming out
			{
				f     = (cg.time - cg.zoomTime) / ZOOM_TIME;
				fov_x = (f > 1.0f) ? fov_x : zoomFov + f * (fov_x - zoomFov);
			}
		}

		rd.rdflags &= ~RDF_SNOOPERVIEW;

		if (BG_PlayerMounted(cg.snap->ps.eFlags) || (CHECKBITWISE(GetWeaponTableData(cg.predictedPlayerState.weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET)))
		{
			fov_x = 55;
		}

		// automatic fov adjustment for wide screens
		fov_x = atan(tan(fov_x * M_PI / 360.0f) * 0.75f * (float)cgs.glconfig.vidWidth / (float)cgs.glconfig.vidHeight) * 360.0f / M_PI;

		x     = rd.width / tan(fov_x / 360 * M_PI);
		fov_y = atan2(rd.height, x);
		fov_y = fov_y * 360 / M_PI;

		rd.fov_x = fov_x;
		rd.fov_y = fov_y;
	}

	rd.time     = cg.time;
	rd.rdflags |= RDF_SKYBOXPORTAL;

	// draw the skybox
	trap_R_RenderScene(&rd);
}

//=========================================================================

// Frustum code

// some culling bits
typedef struct plane_s
{
	vec3_t normal;
	float dist;
} plane_t;

static plane_t frustum[4];

//  CG_SetupFrustum
void CG_SetupFrustum(void)
{
	float ang = cg.refdef_current->fov_x / 180 * M_PI * 0.5;
	float xs  = sin(ang);
	float xc  = cos(ang);
	int i;

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, cg.refdef_current->viewaxis[1], frustum[0].normal);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, cg.refdef_current->viewaxis[1], frustum[1].normal);

	ang = cg.refdef.fov_y / 180 * M_PI * 0.5;
	xs  = sin(ang);
	xc  = cos(ang);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, cg.refdef_current->viewaxis[2], frustum[2].normal);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, cg.refdef_current->viewaxis[2], frustum[3].normal);

	for (i = 0 ; i < 4 ; i++)
	{
		frustum[i].dist = DotProduct(cg.refdef_current->vieworg, frustum[i].normal);
	}
}

/**
 * @brief CG_CullPoint
 * @param pt
 * @return true if culled otherwise false
 */
qboolean CG_CullPoint(vec3_t pt)
{
	int i;
	plane_t *frust;

	// check against frustum planes
	for (i = 0 ; i < 4 ; i++)
	{
		frust = &frustum[i];

		if ((DotProduct(pt, frust->normal) - frust->dist) < 0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_CullPointAndRadius
 * @param[in] pt
 * @param[in] radius
 * @return
 */
qboolean CG_CullPointAndRadius(const vec3_t pt, vec_t radius)
{
	int i;
	plane_t *frust;

	// check against frustum planes
	for (i = 0 ; i < 4 ; i++)
	{
		frust = &frustum[i];

		if ((DotProduct(pt, frust->normal) - frust->dist) < -radius)
		{
			return qtrue;
		}
	}

	return qfalse;
}

//=========================================================================

/**
 * @brief CG_RenderLocations
 */
static void CG_RenderLocations(void)
{
	refEntity_t re;
	location_t *location;
	int i;

	if (cgs.numLocations < 1)
	{
		return;
	}

	for (i = 0; i < cgs.numLocations; ++i)
	{
		location = &cgs.location[i];

		if (VectorDistance(cg.refdef.vieworg, location->origin) > 3000)
		{
			continue;
		}

		if (!trap_R_inPVS(cg.refdef.vieworg, location->origin))
		{
			continue;
		}

		Com_Memset(&re, 0, sizeof(re));
		re.reType = RT_SPRITE;
		VectorCopy(location->origin, re.origin);
		VectorCopy(location->origin, re.oldorigin);
		re.radius = 8;
	}
}

/**
 * @brief CG_ProcessCvars
 */
void CG_ProcessCvars()
{
	char currentVal[256];
	float cvalF, val1F, val2F;
	int i, cvalI, val1I, val2I;
	qboolean cvalIsF, val1IsF, val2IsF;

	for (i = 0; i < cg.svCvarCount; ++i)
	{
		trap_Cvar_VariableStringBuffer(cg.svCvars[i].cvarName, currentVal, sizeof(currentVal));

		cvalF   = (float)atof(currentVal);
		val1F   = (float)atof(cg.svCvars[i].Val1);
		val2F   = (float)atof(cg.svCvars[i].Val2);
		cvalI   = Q_atoi(currentVal);
		val1I   = Q_atoi(cg.svCvars[i].Val1);
		val2I   = Q_atoi(cg.svCvars[i].Val2);
		cvalIsF = (strstr(currentVal, ".")) ? qtrue : qfalse;
		val1IsF = (strstr(cg.svCvars[i].Val1, ".")) ? qtrue : qfalse;
		val2IsF = (strstr(cg.svCvars[i].Val2, ".")) ? qtrue : qfalse;

		switch (cg.svCvars[i].mode)
		{
		case SVC_EQUAL:
			if (Q_stricmp(cg.svCvars[i].Val1, currentVal))
			{
				trap_Cvar_Set(cg.svCvars[i].cvarName, cg.svCvars[i].Val1);
			}
			break;
		case SVC_GREATER:
			if (cvalF <= val1F)
			{
				if (cvalIsF || val1IsF)
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, va("%8.4f", val1F + 0.0001f));
				}
				else
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", val1I + 1));
				}
			}
			break;
		case SVC_GREATEREQUAL:
			if (cvalF < val1F)
			{
				trap_Cvar_Set(cg.svCvars[i].cvarName, cg.svCvars[i].Val1);
			}
			break;
		case SVC_LOWER:
			if (cvalF >= val1F)
			{
				if (cvalIsF || val1IsF)
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, va("%8.4f", val1F - 0.0001f));
				}
				else
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", val1I - 1));
				}
			}
			break;
		case SVC_LOWEREQUAL:
			if (cvalF > val1F)
			{
				if (cvalIsF || val1IsF)
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, va("%8.4f", val1F));
				}
				else
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", val1I));
				}
			}
			break;
		case SVC_INSIDE:
			if (val1F != 0.f || val1I)
			{
				if (cvalF < val1F)
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, cg.svCvars[i].Val1);
				}
			}
			if (val2F != 0.f || val2I)
			{
				if (cvalF > val2F)
				{
					trap_Cvar_Set(cg.svCvars[i].cvarName, cg.svCvars[i].Val2);
				}
			}
			break;
		case SVC_OUTSIDE:
			if (val1F != 0.f || val1I)
			{
				if (cvalF >= val1F)
				{
					if (val2F == 0.f || cvalF < val2F)
					{
						if (cvalIsF || val1IsF)
						{
							trap_Cvar_Set(cg.svCvars[i].cvarName, va("%8.4f", val1F - 0.0001f));
						}
						else
						{
							trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", val1I - 1));
						}
					}
				}
			}
			if (val2F != 0.f || val2I)
			{
				if (cvalF <= val2F)
				{
					if (cvalF > val1F)
					{
						if (cvalIsF || val2IsF)
						{
							trap_Cvar_Set(cg.svCvars[i].cvarName, va("%8.4f", val2F + 0.0001f));
						}
						else
						{
							trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", val2I + 1));
						}
					}
				}
			}
			break;
		case SVC_INCLUDE:
			if (!strstr(currentVal, cg.svCvars[i].Val1))
			{
				trap_Cvar_Set(cg.svCvars[i].cvarName, cg.svCvars[i].Val2);
			}
			break;
		case SVC_EXCLUDE:
			if (strstr(currentVal, cg.svCvars[i].Val1))
			{
				trap_Cvar_Set(cg.svCvars[i].cvarName, cg.svCvars[i].Val2);
			}
			break;
		case SVC_WITHBITS:
			if (!(cvalI & val1I))
			{
				trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", cvalI + val1I));
			}
			break;
		case SVC_WITHOUTBITS:
			if (cvalI & val1I)
			{
				trap_Cvar_Set(cg.svCvars[i].cvarName, va("%i", cvalI - val1I));
			}
			break;
		}
	}
}

//#define DEBUGTIME_ENABLED
#ifdef DEBUGTIME_ENABLED
#define DEBUGTIME elapsed = (trap_Milliseconds() - dbgTime); if (dbgCnt++ == 1) { CG_Printf("t%i:%i ", dbgCnt, elapsed = (trap_Milliseconds() - dbgTime)); } dbgTime += elapsed;
#else
#define DEBUGTIME
#endif

#ifdef ETLEGACY_DEBUG
//#define FAKELAG
#ifdef FAKELAG
extern int snapshotDelayTime;
#endif // FAKELAG
#endif // ETLEGACY_DEBUG

extern void CG_SetupDlightstyles(void);

/**
 * @brief Generates and draws a game scene and status information at the given time.
 * @param[in] serverTime
 * @param[in] demoPlayback
 */
void CG_DrawActiveFrame(int serverTime, qboolean demoPlayback)
{
#ifdef DEBUGTIME_ENABLED
	int dbgTime = trap_Milliseconds(), elapsed;
	int dbgCnt = 0;
#endif

	cg.time         = serverTime;
	cgDC.realTime   = cg.time;
	cg.demoPlayback = demoPlayback;

#ifdef FAKELAG
	cg.time -= snapshotDelayTime;
#endif // FAKELAG

	CG_ProcessCvars();

#ifdef DEBUGTIME_ENABLED
	CG_Printf("\n");
#endif
	DEBUGTIME

	// update cvars
	CG_UpdateCvars();

	DEBUGTIME

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if (cg.infoScreenText[0] != 0)
	{
		CG_DrawInformation(qfalse);
		return;
	}

	CG_PB_ClearPolyBuffers();

	CG_UpdatePMLists();

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds();

	CG_UpdateBufferedSoundScripts();

	DEBUGTIME

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	DEBUGTIME

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if (!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE))
	{
		CG_DrawInformation(qfalse);
		return;
	}

	// check for server set weapons we might not know about
	// (FIXME: this is a hack for the time being since a scripted "selectweapon" does
	// not hit the first snap, the server weapon set in cg_playerstate.c line 219 doesn't
	// do the trick)
	if (!cg.weaponSelect && cg.snap->ps.weapon)
	{
		cg.weaponSelect     = cg.snap->ps.weapon;
		cg.weaponSelectTime = cg.time;
	}

	// The very first frame
	if (cg.clientFrame == 0)
	{
		// autorecord on late joins
		if (!cg.demoPlayback && cgs.gamestate == GS_PLAYING && (cg_autoAction.integer & AA_DEMORECORD))
		{
			if (!cl_demorecording.integer)
			{
				CG_autoRecord_f();
			}
		}
	}

	DEBUGTIME

	if (!cg.lightstylesInited)
	{
		CG_SetupDlightstyles();
	}

	DEBUGTIME

#ifdef FEATURE_EDV
	if (cg.demoPlayback)
	{
		CG_EDV_RunInput();
	}
#endif

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	DEBUGTIME

#ifdef FEATURE_MULTIVIEW
	// MV handling
#ifdef FEATURE_EDV
	if (cg.mvCurrentMainview != NULL && cg.snap->ps.pm_type != PM_INTERMISSION && cgs.mvAllowed && !cgs.demoCamera.renderingFreeCam)
#else
	if (cg.mvCurrentMainview != NULL && cg.snap->ps.pm_type != PM_INTERMISSION && cgs.mvAllowed)
#endif
	{
		CG_mvDraw(cg.mvCurrentMainview);
		// FIXME: not valid for demo playback
		cg.zoomSensitivity = mv_sensitivity.value / int_sensitivity.value;
	}
	else
#endif
	{
		int inwater;

		// clear all the render lists
		trap_R_ClearScene();

		DEBUGTIME

		// decide on third person view
#ifdef FEATURE_EDV
		cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0) || cg.showGameView || cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam;
#else
		cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0) || cg.showGameView;
#endif

		// build cg.refdef
		inwater = CG_CalcViewValues();
		CG_SetupFrustum();

		DEBUGTIME

#ifdef FEATURE_EDV
		if (demo_autotimescaleweapons.integer != 0 && cg.demoPlayback)
		{
			if (cgs.demoCamera.renderingWeaponCam)
			{
				cgs.demoCamera.wasRenderingWeaponCam = qtrue;
			}
			else
			{
				cgs.demoCamera.wasRenderingWeaponCam = qfalse;
			}

			if (!cgs.demoCamera.wasRenderingWeaponCam)
			{
				trap_Cvar_Set("timescale", va("%0.2f", (double)cg_timescale.value));
			}
		}

		cgs.demoCamera.renderingWeaponCam = qfalse;
#endif

		// draw the skyboxportal
		CG_DrawSkyBoxPortal(qtrue);

		DEBUGTIME

		if (inwater)
		{
			CG_UnderwaterSounds();
		}

		DEBUGTIME

		// first person blend blobs, done after AnglesToAxis
		if (!cg.renderingThirdPerson)
		{
			CG_DamageBlendBlob();
		}

		DEBUGTIME

		// build the render lists
		if (!cg.hyperspace)
		{
			CG_AddPacketEntities();         // after calcViewValues, so predicted player state is correct
			CG_AddMarks();

			DEBUGTIME

			CG_AddScriptSpeakers();

			DEBUGTIME

			if (cg_locations.integer & LOC_DEBUG)
			{
				CG_RenderLocations();
			}

			DEBUGTIME

			// particles
			CG_AddParticles();

			DEBUGTIME

			CG_AddLocalEntities();

			DEBUGTIME

			CG_AddSmokeSprites();

			DEBUGTIME

			CG_AddAtmosphericEffects();
		}

		// mg42
		if (!cg.showGameView && !cgs.dbShowing)
		{
			if (!cg.snap->ps.persistant[PERS_HWEAPON_USE])
			{
				CG_AddViewWeapon(&cg.predictedPlayerState);
			}
			else
			{
				if (cg.time - cg.predictedPlayerEntity.overheatTime < 3000)
				{
					vec3_t muzzle;

					if (CG_CalcMuzzlePoint(cg.snap->ps.clientNum, muzzle))
					{
						muzzle[2] -= 32;
					}

					if (!(rand() % 3))
					{
						float alpha = 1.0f - ((cg.time - cg.predictedPlayerEntity.overheatTime) / 3000.0f);

						alpha *= 0.25f;     // .25 max alpha
						CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, muzzle, 1000, 8, 20, 30, alpha, 8.f);
					}
				}
			}
		}

		// play buffered voice chats
		CG_PlayBufferedVoiceChats();

		DEBUGTIME
		// trails
		if (!cg.hyperspace)
		{
			CG_AddFlameChunks();

			// this must come last, so the trails dropped this frame get drawn
			CG_AddTrails();
		}

		DEBUGTIME

		// finish up the rest of the refdef
		if (cg.testModelEntity.hModel)
		{
			CG_AddTestModel();
		}

		cg.refdef.time = cg.time;
		Com_Memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));

		DEBUGTIME

		//lagometer sample and frame timing
		cg.frametime = cg.time - cg.oldTime;
		if (cg.frametime < 0)
		{
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();

		DEBUGTIME

		// let client system know our predicted origin
		trap_SetClientLerpOrigin(cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);

		// actually issue the rendering calls
		CG_DrawActive();

		DEBUGTIME

		// update audio positions
		trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);
	}

	if (cg_stats.integer)
	{
		CG_Printf("cg.clientFrame:%i\n", cg.clientFrame);
	}

	DEBUGTIME

	// let the client system know what our weapon, holdable item and zoom settings are
	trap_SetUserCmdValue(cg.weaponSelect, cg.showGameView ? 0x01 : 0x00, cg.zoomSensitivity, cg.identifyClientRequest);
}
