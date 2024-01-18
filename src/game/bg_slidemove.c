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
 * @file bg_slidemove.c
 * @brief Part of bg_pmove functionality
 */

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

/*
input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean
*/

#define MAX_CLIP_PLANES 5

/**
 * @brief PM_SlideMove
 * @param[in] gravity
 * @return qtrue if the velocity was clipped in some way
 */
qboolean PM_SlideMove(qboolean gravity)
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
			PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
			                pm->ps->velocity, OVERCLIP);
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

	for (bumpcount = 0 ; bumpcount < numbumps ; bumpcount++)
	{
		// calculate position we are trying to move to
		VectorMA(pm->ps->origin, time_left, pm->ps->velocity, end);

		// see if we can make it there
		PM_TraceAll(&trace, pm->ps->origin, end);
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
		PM_AddTouchEnt(trace.entityNum);

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

		for (i = 0 ; i < numplanes ; i++)
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
					PM_TraceAll(&trace, pm->ps->origin, end);
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
		for (i = 0 ; i < numplanes ; i++)
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
			PM_ClipVelocity(pm->ps->velocity, planes[i], clipVelocity, OVERCLIP);

			// slide along the plane
			PM_ClipVelocity(endVelocity, planes[i], endClipVelocity, OVERCLIP);

			// see if there is a second plane that the new move enters
			for (j = 0 ; j < numplanes ; j++)
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
				PM_ClipVelocity(clipVelocity, planes[j], clipVelocity, OVERCLIP);
				PM_ClipVelocity(endClipVelocity, planes[j], endClipVelocity, OVERCLIP);

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
				for (k = 0 ; k < numplanes ; k++)
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
 * @brief PM_StepSlideMove
 * @param[in] gravity
 */
void PM_StepSlideMove(qboolean gravity)
{
	vec3_t  start_o, start_v;
	vec3_t  down_o, down_v;
	trace_t trace;
	vec3_t  up, down;

	VectorCopy(pm->ps->origin, start_o);
	VectorCopy(pm->ps->velocity, start_v);

	if (pm->debugLevel)
	{
		qboolean wassolid, slidesucceed;

		PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
		wassolid = trace.allsolid;

		slidesucceed = (PM_SlideMove(gravity) == 0);

		PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
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
		if (PM_SlideMove(gravity) == 0)
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

	PM_TraceAll(&trace, start_o, down);
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

	VectorCopy(pm->ps->origin, down_o);
	VectorCopy(pm->ps->velocity, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	PM_TraceAll(&trace, up, up);
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

	PM_SlideMove(gravity);

	// push down the final amount
	VectorCopy(pm->ps->origin, down);
	down[2] -= STEPSIZE;

	// check legs&head separately
	if (pm->ps->eFlags & EF_PRONE)
	{
		PM_TraceLegs(&trace, NULL, pm->ps->origin, down, NULL, pm->ps->viewangles, pm->trace, pm->ps->clientNum, pm->tracemask);
		if (trace.fraction < 1.0f)
		{
			// legs don't step, just fuzz.
			VectorCopy(down_o, pm->ps->origin);
			VectorCopy(down_v, pm->ps->velocity);
			if (pm->debugLevel)
			{
				Com_Printf("%i:legs unsteppable\n", c_pmove);
			}
			return;
		}

		PM_TraceHead(&trace, pm->ps->origin, down, NULL, pm->ps->viewangles, pm->trace, pm->ps->clientNum, pm->tracemask);
		if (trace.fraction < 1.0f)
		{
			VectorCopy(down_o, pm->ps->origin);
			VectorCopy(down_v, pm->ps->velocity);
			if (pm->debugLevel)
			{
				Com_Printf("%i:head unsteppable\n", c_pmove);
			}
			return;
		}
	}

	// if dead, body trace with a square bbox so wounded players don't clip into solids
	if (pm->ps->eFlags & EF_DEAD)
	{
		const vec3_t squareMaxs = { 18.f, 18.f, 16.f }; // mins is -18 -18 -24
		pm->trace(&trace, pm->ps->origin, pm->mins, squareMaxs, down, pm->ps->clientNum, pm->tracemask);
	}
	else
	{
		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	}

	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, pm->ps->origin);
	}
	if (trace.fraction < 1.0f)
	{
		PM_ClipVelocity(pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP);
	}

#if 0
	// if the down trace can trace back to the original position directly, don't step
	PM_TraceAll(&trace, pm->ps->origin, start_o);
	if (trace.fraction == 1.0)
	{
		// use the original move
		VectorCopy(down_o, pm->ps->origin);
		VectorCopy(down_v, pm->ps->velocity);
		if (pm->debugLevel)
		{
			Com_Printf("%i:bend\n", c_pmove);
		}
	}
	else
#endif
	{
		// use the step move
		float delta = pm->ps->origin[2] - start_o[2];

		if (delta > 2)
		{
			if (delta < 7)
			{
				PM_AddEvent(EV_STEP_4);
			}
			else if (delta < 11)
			{
				PM_AddEvent(EV_STEP_8);
			}
			else if (delta < 15)
			{
				PM_AddEvent(EV_STEP_12);
			}
			else
			{
				PM_AddEvent(EV_STEP_16);
			}
		}
		if (pm->debugLevel)
		{
			Com_Printf("%i:stepped\n", c_pmove);
		}
	}
}
