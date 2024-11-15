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
  * @file tvg_pmove.c
  * @brief Player movement code
  *
  * Takes a playerstate and a usercmd as input and returns a modifed playerstate.
  */

#include "tvg_local.h"
#include "../game/bg_local.h"

pmove_t *pm;
pml_t   pml;

// movement parameters
float pm_stopspeed = 100;

float pm_accelerate    = 10;
float pm_flyaccelerate = 8;

float pm_friction          = 6;
float pm_spectatorfriction = 5.0f;

int c_pmove = 0;

/**
 * @brief Handles the sequence numbers
 * @param[in] newEvent
 * @param[in] eventParm
 * @param[out] ps
 */
static void TVG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps)
{
	ps->events[ps->eventSequence & (MAX_EVENTS - 1)]     = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_EVENTS - 1)] = eventParm;
	ps->eventSequence++;
}

/**
 * @brief TVG_PM_AddEvent
 * @param[in] newEvent
 */
static void TVG_PM_AddEvent(int newEvent)
{
	TVG_AddPredictableEventToPlayerstate(newEvent, 0, pm->ps);
}

/**
 * @brief This can be used as another entry point when only the viewangles
 *        are being updated instead of a full move
 * @param[in,out] ps
 * @param[in] cmd
 */
static void TVG_PM_UpdateViewAngles(playerState_t *ps, usercmd_t *cmd)
{
	short  temp;
	int    i;
	// vec3_t oldViewAngles;

	if (ps->pm_type == PM_INTERMISSION || (ps->pm_flags & PMF_TIME_LOCKPLAYER))
	{
		// reset all angle changes, so it does not suddenly happens after unlocked
		ps->delta_angles[0] = ANGLE2SHORT(ps->viewangles[0]) - cmd->angles[0];
		ps->delta_angles[1] = ANGLE2SHORT(ps->viewangles[1]) - cmd->angles[1];
		ps->delta_angles[2] = ANGLE2SHORT(ps->viewangles[2]) - cmd->angles[2];
		return;     // no view changes at all
	}

	// VectorCopy(ps->viewangles, oldViewAngles);

	// circularly clamp the angles with deltas
	// - game-side delta_angles modifications are broken here if you exclude the ROLL calculation
	for (i = 0; i < 3; i++)
	{
		temp = cmd->angles[i] + ps->delta_angles[i];
		if (i == PITCH)
		{
			// don't let the player look up or down more than 90 degrees
			if (temp > 16000)
			{
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp                = 16000;
			}
			else if (temp < -16000)
			{
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp                = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

#define LEAN_MAX        28.0f
#define LEAN_TIME_TO    200.0f  // time to get to/from full lean
#define LEAN_TIME_FR    300.0f  // time to get to/from full lean

/**
 * @brief TVG_PM_UpdateLean
 * @param[in,out] ps
 * @param[in,out] cmd
 * @param[in] tpm
 */
static void TVG_PM_UpdateLean(playerState_t *ps, usercmd_t *cmd, pmove_t *tpm)
{
	vec3_t  start, tmins, tmaxs, right, end;
	vec3_t  viewangles;
	trace_t trace;
	int     leaning = 0;        // -1 left, 1 right
	float   leanofs = ps->leanf;

	if ((cmd->wbuttons & (WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT)) && ps->pm_type == PM_SPECTATOR)
	{
		// if both are pressed, result is no lean
		if (cmd->wbuttons & WBUTTON_LEANLEFT)
		{
			leaning -= 1;
		}

		if (cmd->wbuttons & WBUTTON_LEANRIGHT)
		{
			leaning += 1;
		}
	}

	if (!leaning)      // go back to center position
	{
		if (leanofs > 0)            // right
		{
			leanofs -= (((float)pml.msec / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs < 0)
			{
				leanofs = 0;
			}

			ps->leanf = leanofs;
		}
		else if (leanofs < 0)         // left
		{
			leanofs += (((float)pml.msec / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs > 0)
			{
				leanofs = 0;
			}

			ps->leanf = leanofs;
		}

		if (level.mod & LEGACY)
		{
			ps->stats[STAT_PS_FLAGS] &= ~(STAT_LEAN_LEFT | STAT_LEAN_RIGHT);
		}

		// also early return if already in center position
		if (!leanofs)
		{
			return;
		}
	}
	else if (leaning > 0)       // right
	{
		if (leanofs < LEAN_MAX)
		{
			leanofs += (((float)pml.msec / (float)LEAN_TIME_TO) * LEAN_MAX);
		}

		if (leanofs > LEAN_MAX)
		{
			leanofs = LEAN_MAX;
		}

		if (level.mod & LEGACY)
		{
			ps->stats[STAT_PS_FLAGS] |= STAT_LEAN_RIGHT;
		}
	}
	else                  // left
	{
		if (leanofs > -LEAN_MAX)
		{
			leanofs -= (((float)pml.msec / (float)LEAN_TIME_TO) * LEAN_MAX);
		}

		if (leanofs < -LEAN_MAX)
		{
			leanofs = -LEAN_MAX;
		}

		if (level.mod & LEGACY)
		{
			ps->stats[STAT_PS_FLAGS] |= STAT_LEAN_LEFT;
		}
	}

	VectorCopy(ps->origin, start);
	start[2] += ps->viewheight;

	VectorCopy(ps->viewangles, viewangles);
	viewangles[ROLL] += leanofs / 2.0f;
	AngleVectors(viewangles, NULL, right, NULL);
	VectorMA(start, leanofs, right, end);

	VectorSet(tmins, -8, -8, -7);   // NOTE: ATVI Wolfenstein Misc #472, bumped from -4 to cover gun clipping issue
	VectorSet(tmaxs, 8, 8, 4);

	if (pm)
	{
		pm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
	}
	else
	{
		tpm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
	}

	ps->leanf = leanofs * trace.fraction;

	// Allow Spectators to lean while moving
	if (ps->leanf && ps->pm_type != PM_SPECTATOR)
	{
		cmd->rightmove = 0;     // also disallowed in cl_input ~391
	}
}

/**
 * @brief TVG_PM_TraceAllParts
 * @param[in,out] trace
 * @param[in] start
 * @param[in] end
 */
static void TVG_PM_TraceAllParts(trace_t *trace, vec3_t start, vec3_t end)
{
	pm->trace(trace, start, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);
}

/**
 * @brief TVG_PM_TraceAll
 * @param[in,out] trace
 * @param[in] start
 * @param[in] end
 */
static void TVG_PM_TraceAll(trace_t *trace, vec3_t start, vec3_t end)
{
	TVG_PM_TraceAllParts(trace, start, end);
}

/**
 * @brief Sets mins, maxs, and pm->ps->viewheight
 */
static void TVG_PM_CheckDuck(void)
{
	// modified this for configurable bounding boxes
	pm->mins[0] = pm->ps->mins[0];
	pm->mins[1] = pm->ps->mins[1];

	pm->maxs[0] = pm->ps->maxs[0];
	pm->maxs[1] = pm->ps->maxs[1];

	pm->mins[2] = pm->ps->mins[2];

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2]        = pm->ps->maxs[2]; // NOTE: must set death bounding box in game code
		pm->ps->viewheight = pm->ps->deadViewHeight;
		return;
	}

	// duck
	if (pm->cmd.upmove < 0)
	{
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else // stand up if possible
	{
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			trace_t trace;

			// try to stand up
			pm->maxs[2] = pm->ps->maxs[2];
			TVG_PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
			if (trace.fraction == 1.0f)
			{
				pm->ps->pm_flags &= ~PMF_DUCKED;
			}
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2]        = pm->ps->crouchMaxZ;
		pm->ps->viewheight = pm->ps->crouchViewHeight;
	}
	else
	{
		pm->maxs[2]        = pm->ps->maxs[2];
		pm->ps->viewheight = pm->ps->standViewHeight;
	}
}

/**
 * @brief Handles both ground friction and water friction
 */
static void TVG_PM_Friction(void)
{
	vec3_t vec;
	float  speed, newspeed;
	float  drop;
	float  *vel = pm->ps->velocity;

	VectorCopy(vel, vec);
	if (pml.walking)
	{
		vec[2] = 0; // ignore slope movement
	}

	speed = VectorLength(vec);
	// don't do this for PM_SPECTATOR/PM_NOCLIP, we always want them to stop
	if (speed < 1 && pm->ps->pm_type != PM_SPECTATOR && pm->ps->pm_type != PM_NOCLIP)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	if (speed == 0.0f)
	{
		return;
	}

	drop = speed * pm_spectatorfriction * pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	// if we're barely moving and barely slowing down, we want to
	// help things along--we don't want to end up getting snapped back to
	// our previous speed
	if (pm->ps->pm_type == PM_SPECTATOR || pm->ps->pm_type == PM_NOCLIP)
	{
		if (drop < 1.0f && speed < 3.0f)
		{
			newspeed = 0.0;
		}
	}

	// used VectorScale instead of multiplying by hand
	VectorScale(vel, newspeed, vel);
}

/**
 * @brief This allows the clients to use axial -127 to 127 values for all directions
 * without getting a sqrt(2) distortion in speed.
 * @param[in] cmd
 * @return The scale factor to apply to cmd movements
 */
static float TVG_PM_CmdScale(usercmd_t *cmd)
{
	int   max;
	float total;
	float scale;

	max = abs(cmd->forwardmove);
	if (abs(cmd->rightmove) > max)
	{
		max = abs(cmd->rightmove);
	}
	if (abs(cmd->upmove) > max)
	{
		max = abs(cmd->upmove);
	}
	if (!max)
	{
		return 0;
	}

	total = sqrt(cmd->forwardmove * cmd->forwardmove
	             + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove);
	scale = (float)pm->ps->speed * max / (127.0f * total);

	if ((pm->cmd.buttons & BUTTON_SPRINT) && pm->pmext->sprintTime > 50)
	{
		scale *= pm->ps->sprintSpeedScale;
	}
	else
	{
		scale *= pm->ps->runSpeedScale;
	}

	if (pm->ps->pm_type == PM_NOCLIP)
	{
		scale *= 3;
	}

	return scale;
}

/**
 * @brief Handles user intended acceleration
 * @param[in] wishdir
 * @param[in] wishspeed
 * @param[in] accel
 */
static void TVG_PM_Accelerate(vec3_t wishdir, float wishspeed, float accel)
{
	// q2 style
	int   i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(pm->ps->velocity, wishdir);
	addspeed     = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	accelspeed = accel * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	// variable friction for AI's
	if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		accelspeed *= (1.0f / pm->ps->friction);
	}
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++)
	{
		pm->ps->velocity[i] += accelspeed * wishdir[i];
	}
}

/**
 * @brief Slide off of the impacting surface
 * @param[in] in
 * @param[in] normal
 * @param[out] out
 * @param[in] overbounce
 */
static void TVG_PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float backoff = DotProduct(in, normal);

	if (backoff < 0)
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	out[0] = in[0] - (normal[0] * backoff);
	out[1] = in[1] - (normal[1] * backoff);
	out[2] = in[2] - (normal[2] * backoff);
}

#define MAX_CLIP_PLANES 5

/**
 * @brief TVG_PM_SlideMove
 * @param[in] gravity
 * @return qtrue if the velocity was clipped in some way
 */
static qboolean TVG_PM_SlideMove(qboolean gravity)
{
	int     bumpcount, numbumps = 4, extrabumps = 0;
	vec3_t  dir;
	float   d;
	int     numplanes;
	vec3_t  planes[MAX_CLIP_PLANES];
	vec3_t  primal_velocity;
	vec3_t  clipVelocity;
	int     i, j, k;
	trace_t trace;
	vec3_t  end;
	float   time_left;
	float   into;
	vec3_t  endVelocity;
	vec3_t  endClipVelocity;

	VectorCopy(pm->ps->velocity, primal_velocity);

	if (gravity)
	{
		VectorCopy(pm->ps->velocity, endVelocity);
		endVelocity[2]     -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = (pm->ps->velocity[2] + endVelocity[2]) * 0.5f;
		primal_velocity[2]  = endVelocity[2];
		if (pml.groundPlane)
		{
			// slide along the ground plane
			TVG_PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);
		}
	}
	else
	{
		VectorClear(endVelocity);
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if (pml.groundPlane)
	{
		numplanes = 1;
		VectorCopy(pml.groundTrace.plane.normal, planes[0]);
	}
	else
	{
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2(pm->ps->velocity, planes[numplanes]);
	numplanes++;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		// calculate position we are trying to move to
		VectorMA(pm->ps->origin, time_left, pm->ps->velocity, end);

		// see if we can make it there
		TVG_PM_TraceAll(&trace, pm->ps->origin, end);
		if (pm->debugLevel > 1)
		{
			Com_Printf("%i:%d %d (%f %f %f)\n",
			           c_pmove, trace.allsolid, trace.startsolid,
			           (double)trace.endpos[0],
			           (double)trace.endpos[1],
			           (double)trace.endpos[2]
			           );
		}

		if (trace.allsolid)
		{
			if (pm->debugLevel)
			{
				Com_Printf("%i:trappedinsolid\n", c_pmove);
			}

			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;    // don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, pm->ps->origin);
		}

		if (trace.fraction == 1.f)
		{
			if (pm->debugLevel > 1)
			{
				Com_Printf("%i:moved the entire distance at bump %d\n", c_pmove, bumpcount);
			}
			break;      // moved the entire distance
		}

		// save entity for contact
		//PM_AddTouchEnt(trace.entityNum);

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES)
		{
			if (pm->debugLevel)
			{
				Com_Printf("%i:MAX_CLIP_PLANES reached (this shouldn't really happen)\n", c_pmove);
			}

			// this shouldn't really happen
			VectorClear(pm->ps->velocity);
			return qtrue;
		}

		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes

		for (i = 0; i < numplanes; i++)
		{
			if (DotProduct(trace.plane.normal, planes[i]) > 0.99f)
			{
				if (extrabumps <= 0)
				{
					VectorAdd(trace.plane.normal, pm->ps->velocity, pm->ps->velocity);
					extrabumps++;
					numbumps++;

					if (pm->debugLevel)
					{
						Com_Printf("%i:planevelocitynudge\n", c_pmove);
					}
				}
				else
				{
					// zinx - if it happens again, nudge the origin instead,
					// and trace it, to make sure we don't end up in a solid

					VectorAdd(pm->ps->origin, trace.plane.normal, end);
					TVG_PM_TraceAll(&trace, pm->ps->origin, end);
					VectorCopy(trace.endpos, pm->ps->origin);

					if (pm->debugLevel)
					{
						Com_Printf("%i:planeoriginnudge\n", c_pmove);
					}
				}
				break;
			}
		}
		if (i < numplanes)
		{
			continue;
		}
		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify velocity so it parallels all of the clip planes

		// find a plane that it enters
		for (i = 0; i < numplanes; i++)
		{
			into = DotProduct(pm->ps->velocity, planes[i]);
			if (into >= 0.1f)
			{
				continue;       // move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if (-into > pml.impactSpeed)
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			TVG_PM_ClipVelocity(pm->ps->velocity, planes[i], clipVelocity, OVERCLIP);

			// slide along the plane
			TVG_PM_ClipVelocity(endVelocity, planes[i], endClipVelocity, OVERCLIP);

			// see if there is a second plane that the new move enters
			for (j = 0; j < numplanes; j++)
			{
				if (j == i)
				{
					continue;
				}
				if (DotProduct(clipVelocity, planes[j]) >= 0.1f)
				{
					continue;       // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				TVG_PM_ClipVelocity(clipVelocity, planes[j], clipVelocity, OVERCLIP);
				TVG_PM_ClipVelocity(endClipVelocity, planes[j], endClipVelocity, OVERCLIP);

				// see if it goes back into the first clip plane
				if (DotProduct(clipVelocity, planes[i]) >= 0)
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, pm->ps->velocity);
				VectorScale(dir, d, clipVelocity);

				//CrossProduct(planes[i], planes[j], dir); // this has just been done
				//VectorNormalize(dir);                    // and this too..  no need to do it again
				d = DotProduct(dir, endVelocity);
				VectorScale(dir, d, endClipVelocity);

				// see if there is a third plane the new move enters
				for (k = 0; k < numplanes; k++)
				{
					if (k == i || k == j)
					{
						continue;
					}
					if (DotProduct(clipVelocity, planes[k]) >= 0.1f)
					{
						continue;       // move doesn't interact with the plane
					}

					if (pm->debugLevel)
					{
						Com_Printf("%i:third plane interaction\n", c_pmove);
					}

					// stop dead at a tripple plane interaction
					VectorClear(pm->ps->velocity);
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy(clipVelocity, pm->ps->velocity);
			VectorCopy(endClipVelocity, endVelocity);
			break;
		}
	}

	if (gravity)
	{
		VectorCopy(endVelocity, pm->ps->velocity);
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if (pm->ps->pm_time)
	{
		VectorCopy(primal_velocity, pm->ps->velocity);
	}

	return (bumpcount != 0);
}

/**
 * @brief TVG_PM_StepSlideMove
 * @param[in] gravity
 */
static void TVG_PM_StepSlideMove(qboolean gravity)
{
	vec3_t  start_o, start_v;
	// vec3_t  down_o, down_v;
	trace_t trace;
	vec3_t  up, down;

	float delta;

	VectorCopy(pm->ps->origin, start_o);
	VectorCopy(pm->ps->velocity, start_v);

	if (pm->debugLevel)
	{
		qboolean wassolid, slidesucceed;

		TVG_PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
		wassolid = trace.allsolid;

		slidesucceed = (TVG_PM_SlideMove(gravity) == 0);

		TVG_PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
		if (trace.allsolid && !wassolid)
		{
			Com_Printf("%i:PM_SlideMove solidified! (%f %f %f) -> (%f %f %f)\n", c_pmove,
			           (double)start_o[0],
			           (double)start_o[1],
			           (double)start_o[2],
			           (double)pm->ps->origin[0],
			           (double)pm->ps->origin[1],
			           (double)pm->ps->origin[2]
			           );
		}

		if (slidesucceed)
		{
			return;
		}
	}
	else
	{
		if (TVG_PM_SlideMove(gravity) == 0)
		{
			return;     // we got exactly where we wanted to go first try
		}
	}

	if (pm->debugLevel)
	{
		Com_Printf("%i:stepping\n", c_pmove);
	}

	VectorCopy(start_o, down);
	down[2] -= STEPSIZE;

	TVG_PM_TraceAll(&trace, start_o, down);
	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if (pm->ps->velocity[2] > 0 && (trace.fraction == 1.0f || DotProduct(trace.plane.normal, up) < 0.7f))
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:up velocity can't step\n", c_pmove);
		}
		return;
	}

	// VectorCopy(pm->ps->origin, down_o);
	// VectorCopy(pm->ps->velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	TVG_PM_TraceAll(&trace, up, up);
	if (trace.allsolid)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		return;     // can't step up
	}

	// try slidemove from this position
	VectorCopy(up, pm->ps->origin);
	VectorCopy(start_v, pm->ps->velocity);

	TVG_PM_SlideMove(gravity);

	// push down the final amount
	VectorCopy(pm->ps->origin, down);
	down[2] -= STEPSIZE;

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);

	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, pm->ps->origin);
	}
	if (trace.fraction < 1.0f)
	{
		TVG_PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);
	}

	// use the step move
	delta = pm->ps->origin[2] - start_o[2];

	if (delta > 2)
	{
		if (delta < 7)
		{
			TVG_PM_AddEvent(EV_STEP_4);
		}
		else if (delta < 11)
		{
			TVG_PM_AddEvent(EV_STEP_8);
		}
		else if (delta < 15)
		{
			TVG_PM_AddEvent(EV_STEP_12);
		}
		else
		{
			TVG_PM_AddEvent(EV_STEP_16);
		}
	}

	if (pm->debugLevel)
	{
		Com_Printf("%i:stepped\n", c_pmove);
	}
}

/**
 * @brief TVG_PM_NoclipMove
 */
static void TVG_PM_NoclipMove(void)
{
	float  speed;
	int    i;
	vec3_t wishvel;
	float  fmove, smove;
	vec3_t wishdir;
	float  wishspeed;
	float  scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction
	speed = VectorLength(pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy(vec3_origin, pm->ps->velocity);
	}
	else
	{
		float drop     = 0;
		float friction = pm_friction * 1.5f; // extra friction
		float control  = speed < pm_stopspeed ? pm_stopspeed : speed;
		float newspeed;

		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
		{
			newspeed = 0;
		}
		newspeed /= speed;

		VectorScale(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = TVG_PM_CmdScale(&pm->cmd);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i = 0; i < 3; i++)
	{
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] += pm->cmd.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	TVG_PM_Accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	VectorMA(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

/**
 * @brief TVG_PM_FlyMove
 */
static void TVG_PM_FlyMove(void)
{
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	// normal slowdown
	TVG_PM_Friction();

	scale = TVG_PM_CmdScale(&pm->cmd);

	// spectator boost
	if (pm->cmd.buttons & BUTTON_SPRINT && (level.mod & LEGACY))
	{
		scale *= 2;
	}

	// user intentions
	if (scale == 0.f)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}
	else
	{
		int i;

		for (i = 0; i < 3; i++)
		{
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	TVG_PM_Accelerate(wishdir, wishspeed, pm_flyaccelerate);

	TVG_PM_StepSlideMove(qfalse);
}

/**
 * @brief TVG_PM_DropTimers
 */
static void TVG_PM_DropTimers(void)
{
	// drop misc timing counter
	if (pm->ps->pm_time)
	{
		if (pml.msec >= pm->ps->pm_time)
		{
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time   = 0;
		}
		else
		{
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if (pm->ps->legsTimer > 0)
	{
		pm->ps->legsTimer -= pml.msec;
		if (pm->ps->legsTimer < 0)
		{
			pm->ps->legsTimer = 0;
		}
	}

	if (pm->ps->torsoTimer > 0)
	{
		pm->ps->torsoTimer -= pml.msec;
		if (pm->ps->torsoTimer < 0)
		{
			pm->ps->torsoTimer = 0;
		}
	}
}

/**
 * @brief TVG_PmoveSingle
 * @param[in,out] pmove
 */
void TVG_PmoveSingle(pmove_t *pmove)
{
	pm = pmove;

	if (pm->ps->viewheight == pm->ps->crouchViewHeight)
	{
		pm->ps->eFlags |= EF_CROUCHING;
	}
	else
	{
		pm->ps->eFlags &= ~EF_CROUCHING;
	}

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	// clear results
	pm->numtouch   = 0;
	pm->watertype  = 0;
	pm->waterlevel = 0;

	if (!pm->activateLean)
	{
		if (pm->cmd.wbuttons & WBUTTON_LEANLEFT && pm->cmd.buttons & BUTTON_ACTIVATE)
		{
			pm->cmd.rightmove = -127;
			pm->cmd.wbuttons ^= WBUTTON_LEANLEFT;
		}
		else if (pm->cmd.wbuttons & WBUTTON_LEANRIGHT && pm->cmd.buttons & BUTTON_ACTIVATE)
		{
			pm->cmd.rightmove = 127;
			pm->cmd.wbuttons ^= WBUTTON_LEANRIGHT;
		}
	}

	if (level.mod & ETJUMP)
	{
// statIndex_t
#define STAT_USERCMD_BUTTONS 9
#define STAT_USERCMD_MOVE    10

// Keys pressed
#define UMOVE_FORWARD  BIT(0)
#define UMOVE_BACKWARD BIT(1)
#define UMOVE_LEFT     BIT(2)
#define UMOVE_RIGHT    BIT(3)
#define UMOVE_UP       BIT(4)
#define UMOVE_DOWN     BIT(5)

		// setup - copy pressed keys into playerstate
		// buttons, wbuttons
		pm->ps->stats[STAT_USERCMD_BUTTONS] = pm->cmd.buttons << 8;
		// stats are sent as 16-bit signed integers, so we need to mask out
		// BUTTON_ANY, since 128 << 8 will overflow, and it isn't required anyway
		pm->ps->stats[STAT_USERCMD_BUTTONS] &= ~(BUTTON_ANY << 8);
		pm->ps->stats[STAT_USERCMD_BUTTONS] |= pm->cmd.wbuttons;
		pm->ps->stats[STAT_USERCMD_MOVE]     = 0;

		// forwardmove
		if (pm->cmd.forwardmove > 0)
		{
			pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_FORWARD;
		}
		else if (pm->cmd.forwardmove < 0)
		{
			pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_BACKWARD;
		}

		// rightmove
		if (pm->cmd.rightmove > 0)
		{
			pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_RIGHT;
		}
		else if (pm->cmd.rightmove < 0)
		{
			pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_LEFT;
		}

		// upmove
		if (pm->cmd.upmove > 0)
		{
			pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_UP;
		}
		else if (pm->cmd.upmove < 0)
		{
			pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_DOWN;
		}
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if (abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64)
	{
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if (pm->cmd.buttons & BUTTON_TALK)
	{
		pm->ps->eFlags |= EF_TALK;
	}
	else
	{
		pm->ps->eFlags &= ~EF_TALK;
	}

	pm->ps->eFlags &= ~(EF_FIRING | EF_ZOOMING);

	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		// clear the respawned flag if attack button are cleared
		// don't clear if a weapon change is needed to prevent early weapon change
		if (pm->ps->stats[STAT_HEALTH] > 0 &&
		    !(pm->cmd.wbuttons & WBUTTON_RELOAD) &&
		    !(pm->cmd.buttons & BUTTON_ATTACK) &&  // & (BUTTON_ATTACK /*| BUTTON_USE_HOLDABLE
		    (pm->ps->weapon == pm->cmd.weapon))       // bit hacky, stop the slight lag from client -> server even on locahost, switching back to the weapon you were holding
		                                              // and then back to what weapon you should have, became VERY noticible for the kar98/carbine + gpg40, esp now i've added the
		                                              // animation locking
		{
			pm->ps->pm_flags &= ~PMF_RESPAWNED;
		}
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if (pmove->cmd.buttons & BUTTON_TALK)
	{
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons     = BUTTON_TALK;
		pmove->cmd.wbuttons    = 0;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove   = 0;
		pmove->cmd.upmove      = 0;
		pmove->cmd.doubleTap   = 0;
	}

	// clear all pmove local vars
	Com_Memset(&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if (pml.msec < 1)
	{
		pml.msec = 1;
	}
	else if (pml.msec > 200)
	{
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy(pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy(pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001f;

	// update the viewangles
	if (pm->ps->pm_type != PM_FREEZE)
	{
		TVG_PM_UpdateViewAngles(pm->ps, &pm->cmd);
		TVG_PM_UpdateLean(pm->ps, &pm->cmd, pm);
	}

	AngleVectors(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if (pm->cmd.upmove < 10)
	{
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if (pm->cmd.forwardmove < 0)
	{
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	}
	else if (pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove))
	{
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if (pm->ps->pm_type >= PM_DEAD || (pm->ps->pm_flags & (PMF_LIMBO | PMF_TIME_LOCKPLAYER)))
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove   = 0;
		pm->cmd.upmove      = 0;
	}

	switch (pm->ps->pm_type)
	{
	case PM_SPECTATOR:
		TVG_PM_CheckDuck();
		TVG_PM_FlyMove();
		TVG_PM_DropTimers();
		return;
	case PM_NOCLIP:
		TVG_PM_NoclipMove();
		TVG_PM_DropTimers();
		return;
	case PM_FREEZE:
	case PM_INTERMISSION: // no movement at all
		return;
	case PM_NORMAL:
		return;
	default:
		return;
	}
}

/**
 * @brief TVG_Pmove
 * @param[in] pmove
 */
void TVG_Pmove(pmove_t *pmove)
{
	int msec;
	int finalTime = pmove->cmd.serverTime;
	int gravity   = pmove->ps->gravity;

	if (finalTime < pmove->ps->commandTime)
	{
		return;   // should not happen
	}

	if (finalTime > pmove->ps->commandTime + 1000)
	{
		pmove->ps->commandTime = finalTime - 1000;
	}

	pm = pmove;

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while (pmove->ps->commandTime != finalTime)
	{
		msec = finalTime - pmove->ps->commandTime;

		if (pmove->pmove_fixed)
		{
			if (msec > pmove->pmove_msec)
			{
				msec = pmove->pmove_msec;
			}
		}
		else
		{
			// this was 66 (15fps), but I've changed it to
			// 50 (20fps, max rate of mg42) to alleviate some of the
			// framerate dependency with the mg42.
			// in reality, this should be split according to sv_fps,
			// and pmove() shouldn't handle weapon firing
			if (msec > 50)
			{
				msec = 50;
			}
		}

		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		pmove->ps->gravity    = gravity;

		TVG_PmoveSingle(pmove);

		if (pmove->ps->pm_flags & PMF_JUMP_HELD)
		{
			pmove->cmd.upmove = 20;
		}
	}
}
