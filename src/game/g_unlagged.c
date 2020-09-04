/**
===========================================================================
Copyright (C) 2006 Neil Toronto.

This file is part of the Unlagged source code.

Unlagged source code is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Unlagged source code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Unlagged source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

/**
 * @brief Clear out the given client's history (should be called when the teleport bit is flipped)
 * @param[in,out] ent
 */
void G_ResetHistory(gentity_t *ent)
{
	int i, time;

	// fill up the history with data (assume the current position)
	ent->client->historyHead = NUM_CLIENT_HISTORY - 1;
	for (i = ent->client->historyHead, time = level.time; i >= 0; i--, time -= 50)
	{
		VectorCopy(ent->r.mins, ent->client->history[i].mins);
		VectorCopy(ent->r.maxs, ent->client->history[i].maxs);
		VectorCopy(ent->r.currentOrigin, ent->client->history[i].currentOrigin);
		ent->client->history[i].leveltime = time;
	}
}

/**
 * @brief Keep track of where the client's been
 * @param[in,out] ent
 */
void G_StoreHistory(gentity_t *ent)
{
	int head, frametime;

	frametime = level.time - level.previousTime;

	ent->client->historyHead++;
	if (ent->client->historyHead >= NUM_CLIENT_HISTORY)
	{
		ent->client->historyHead = 0;
	}

	head = ent->client->historyHead;

	// store all the collision-detection info and the time
	VectorCopy(ent->r.mins, ent->client->history[head].mins);
	VectorCopy(ent->r.maxs, ent->client->history[head].maxs);
	VectorCopy(ent->s.pos.trBase, ent->client->history[head].currentOrigin);
	SnapVector(ent->client->history[head].currentOrigin);
	ent->client->history[head].leveltime = level.time;
}

/**
 * @brief Used below to interpolate between two previous vectors
 * @param[in] frac
 * @param[in] start
 * @param[in] end
 * @param[out] result a vector "frac" times the distance between "start" and "end"
 */
static void TimeShiftLerp(float frac, vec3_t start, vec3_t end, vec3_t result)
{
// From CG_InterpolateEntityPosition in cg_ents.c:
/*
    cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
    cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
    cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );
*/
// Making these exactly the same should avoid floating-point error

	result[0] = start[0] + frac * (end[0] - start[0]);
	result[1] = start[1] + frac * (end[1] - start[1]);
	result[2] = start[2] + frac * (end[2] - start[2]);
}

/**
 * @brief Move a client back to where he was at the specified "time"
 * @param ent
 * @param time
 * @param debug
 * @param debugger
 */
void G_TimeShiftClient(gentity_t *ent, int time, qboolean debug, gentity_t *debugger)
{
	int  j, k;
	char msg[2048];

	// this will dump out the head index, and the time for all the stored positions
/*
    if ( debug ) {
        char	str[MAX_STRING_CHARS];

        Com_sprintf(str, sizeof(str), "print \"head: %d, %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n\"",
            ent->client->historyHead,
            ent->client->history[0].leveltime,
            ent->client->history[1].leveltime,
            ent->client->history[2].leveltime,
            ent->client->history[3].leveltime,
            ent->client->history[4].leveltime,
            ent->client->history[5].leveltime,
            ent->client->history[6].leveltime,
            ent->client->history[7].leveltime,
            ent->client->history[8].leveltime,
            ent->client->history[9].leveltime,
            ent->client->history[10].leveltime,
            ent->client->history[11].leveltime,
            ent->client->history[12].leveltime,
            ent->client->history[13].leveltime,
            ent->client->history[14].leveltime,
            ent->client->history[15].leveltime,
            ent->client->history[16].leveltime);

        trap_SendServerCommand( debugger - g_entities, str );
    }
*/

	// find two entries in the history whose times sandwich "time"
	// assumes no two adjacent records have the same timestamp
	j = k = ent->client->historyHead;
	do
	{
		if (ent->client->history[j].leveltime <= time)
		{
			break;
		}

		k = j;
		j--;
		if (j < 0)
		{
			j = NUM_CLIENT_HISTORY - 1;
		}
	}
	while (j != ent->client->historyHead);

	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if (j != k)
	{
		// make sure it doesn't get re-saved
		if (ent->client->saved.leveltime != level.time)
		{
			// save the current origin and bounding box
			VectorCopy(ent->r.mins, ent->client->saved.mins);
			VectorCopy(ent->r.maxs, ent->client->saved.maxs);
			VectorCopy(ent->r.currentOrigin, ent->client->saved.currentOrigin);
			ent->client->saved.leveltime = level.time;
		}

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if (j != ent->client->historyHead)
		{
			float frac = (float)(time - ent->client->history[j].leveltime) /
			             (float)(ent->client->history[k].leveltime - ent->client->history[j].leveltime);

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp(frac,
			              ent->client->history[j].currentOrigin, ent->client->history[k].currentOrigin,
			              ent->r.currentOrigin);

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp(frac,
			              ent->client->history[j].mins, ent->client->history[k].mins,
			              ent->r.mins);

			TimeShiftLerp(frac,
			              ent->client->history[j].maxs, ent->client->history[k].maxs,
			              ent->r.maxs);

			if (debug && debugger != NULL)
			{
				// print some debugging stuff exactly like what the client does

				// it starts with "Rec:" to let you know it backward-reconciled
				Com_sprintf(msg, sizeof(msg),
				            "print \"^1Rec: time: %d, j: %d, k: %d, origin: %0.2f %0.2f %0.2f\n"
				            "^2frac: %0.4f, origin1: %0.2f %0.2f %0.2f, origin2: %0.2f %0.2f %0.2f\n"
				            "^7level.time: %d, est time: %d, level.time delta: %d, est real ping: %d\n\"",
				            time, ent->client->history[j].leveltime, ent->client->history[k].leveltime,
				            ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2],
				            frac,
				            ent->client->history[j].currentOrigin[0],
				            ent->client->history[j].currentOrigin[1],
				            ent->client->history[j].currentOrigin[2],
				            ent->client->history[k].currentOrigin[0],
				            ent->client->history[k].currentOrigin[1],
				            ent->client->history[k].currentOrigin[2],
				            level.time, level.time + debugger->client->frameOffset,
				            level.time - time, level.time + debugger->client->frameOffset - time);

				trap_SendServerCommand(debugger - g_entities, msg);
			}

			// this will recalculate absmin and absmax
			trap_LinkEntity(ent);
		}
		else
		{
			// we wrapped, so grab the earliest
			VectorCopy(ent->client->history[k].currentOrigin, ent->r.currentOrigin);
			VectorCopy(ent->client->history[k].mins, ent->r.mins);
			VectorCopy(ent->client->history[k].maxs, ent->r.maxs);

			// this will recalculate absmin and absmax
			trap_LinkEntity(ent);
		}
	}
	else
	{
		// this only happens when the client is using a negative timenudge, because that
		// number is added to the command time

		// print some debugging stuff exactly like what the client does

		// it starts with "No rec:" to let you know it didn't backward-reconcile
		if (debug && debugger != NULL)
		{
			Com_sprintf(msg, sizeof(msg),
			            "print \"^1No rec: time: %d, j: %d, k: %d, origin: %0.2f %0.2f %0.2f\n"
			            "^2frac: %0.4f, origin1: %0.2f %0.2f %0.2f, origin2: %0.2f %0.2f %0.2f\n"
			            "^7level.time: %d, est time: %d, level.time delta: %d, est real ping: %d\n\"",
			            time, level.time, level.time,
			            ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2],
			            0.0f,
			            ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2],
			            ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2],
			            level.time, level.time + debugger->client->frameOffset,
			            level.time - time, level.time + debugger->client->frameOffset - time);

			trap_SendServerCommand(debugger - g_entities, msg);
		}
	}
}

/**
 * @brief Move ALL clients back to where they were at the specified "time",
 * except for "skip"
 * @param[in] time
 * @param[in] skip
 */
void G_TimeShiftAllClients(int time, gentity_t *skip)
{
	int       i;
	gentity_t *ent;
	qboolean  debug = (skip != NULL && skip->client && skip->client->pers.debugDelag);

	// for every client
	ent = &g_entities[0];
	for (i = 0; i < MAX_CLIENTS; i++, ent++)
	{
		if (ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip)
		{
			G_TimeShiftClient(ent, time, debug, skip);
		}
	}
}

/**
 * @brief Decide what time to shift everyone back to, and do it
 * @param[in] ent
 */
void G_DoTimeShiftFor(gentity_t *ent)
{
#ifndef MISSIONPACK
	int wpflags[WP_NUM_WEAPONS] = { 0, 0, 2, 4, 0, 0, 8, 16, 0, 0, 0 };
#else
	int wpflags[WP_NUM_WEAPONS] = { 0, 0, 2, 4, 0, 0, 8, 16, 0, 0, 0, 32, 0, 64 };
#endif
	int wpflag = wpflags[ent->client->ps.weapon];
	int time;

	// don't time shift for mistakes or bots
	if (!ent->inuse || !ent->client || (ent->r.svFlags & SVF_BOT))
	{
		return;
	}

	// if it's enabled server-side and the client wants it or wants it for this weapon
	if (g_delagHitscan.integer && (ent->client->pers.delag & 1 || ent->client->pers.delag & wpflag))
	{
		// do the full lag compensation, except what the client nudges
		time = ent->client->attackTime + ent->client->pers.cmdTimeNudge;
	}
	else
	{
		// do just 50ms
		time = level.previousTime + ent->client->frameOffset;
	}

	G_TimeShiftAllClients(time, ent);
}

/**
 * @brief Move a client back to where he was before the time shift
 * @param[in] ent
 */
void G_UnTimeShiftClient(gentity_t *ent)
{
	// if it was saved
	if (ent->client->saved.leveltime == level.time)
	{
		// move it back
		VectorCopy(ent->client->saved.mins, ent->r.mins);
		VectorCopy(ent->client->saved.maxs, ent->r.maxs);
		VectorCopy(ent->client->saved.currentOrigin, ent->r.currentOrigin);
		ent->client->saved.leveltime = 0;

		// this will recalculate absmin and absmax
		trap_LinkEntity(ent);
	}
}

/**
 * @brief Move ALL the clients back to where they were before the time shift,
 * except for "skip"
 * @param[in] skip
 */
void G_UnTimeShiftAllClients(gentity_t *skip)
{
	int       i;
	gentity_t *ent;

	ent = &g_entities[0];
	for (i = 0; i < MAX_CLIENTS; i++, ent++)
	{
		if (ent->client && ent->inuse && ent->client->sess.sessionTeam < TEAM_SPECTATOR && ent != skip)
		{
			G_UnTimeShiftClient(ent);
		}
	}
}

/**
 * @brief Put everyone except for this client back where they were
 * @param[in] ent
 */
void G_UndoTimeShiftFor(gentity_t *ent)
{

	// don't un-time shift for mistakes or bots
	if (!ent->inuse || !ent->client || (ent->r.svFlags & SVF_BOT))
	{
		return;
	}

	G_UnTimeShiftAllClients(ent);
}

#define OVERCLIP        1.001f

/**
 * @brief Slide on the impacting surface
 * @param[in] in
 * @param[in] normal
 * @param[out] out
 */
void G_PredictPlayerClipVelocity(vec3_t in, vec3_t normal, vec3_t out)
{
	float backoff;

	// find the magnitude of the vector "in" along "normal"
	backoff = DotProduct(in, normal);

	// tilt the plane a bit to avoid floating-point error issues
	if (backoff < 0)
	{
		backoff *= OVERCLIP;
	}
	else
	{
		backoff /= OVERCLIP;
	}

	// slide along
	VectorMA(in, -backoff, normal, out);
}

#define MAX_CLIP_PLANES 5

/**
 * @brief Advance the given entity frametime seconds, sliding as appropriate
 * @param[in,out] ent
 * @param[in] frametime
 * @return
 */
qboolean G_PredictPlayerSlideMove(gentity_t *ent, float frametime)
{
	int     bumpcount, numbumps;
	vec3_t  dir;
	float   d;
	int     numplanes;
	vec3_t  planes[MAX_CLIP_PLANES];
	vec3_t  primal_velocity, velocity, origin;
	vec3_t  clipVelocity;
	int     i, j, k;
	trace_t trace;
	vec3_t  end;
	float   time_left;
	float   into;
	vec3_t  endVelocity;
	vec3_t  endClipVelocity;
	vec3_t  worldUp = { 0.0f, 0.0f, 1.0f };

	numbumps = 4;

	VectorCopy(ent->s.pos.trDelta, primal_velocity);
	VectorCopy(primal_velocity, velocity);
	VectorCopy(ent->s.pos.trBase, origin);

	VectorCopy(velocity, endVelocity);

	time_left = frametime;

	numplanes = 0;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{

		// calculate position we are trying to move to
		VectorMA(origin, time_left, velocity, end);

		// see if we can make it there
		trap_Trace(&trace, origin, ent->r.mins, ent->r.maxs, end, ent->s.number, ent->clipmask);

		if (trace.allsolid)
		{
			// entity is completely trapped in another solid
			VectorClear(velocity);
			VectorCopy(origin, ent->s.pos.trBase);
			return qtrue;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, origin);
		}

		if (trace.fraction == 1)
		{
			break;      // moved the entire distance
		}

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorClear(velocity);
			VectorCopy(origin, ent->s.pos.trBase);
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for (i = 0; i < numplanes; i++)
		{
			if (DotProduct(trace.plane.normal, planes[i]) > 0.99)
			{
				VectorAdd(trace.plane.normal, velocity, velocity);
				break;
			}
		}

		if (i < numplanes)
		{
			continue;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for (i = 0; i < numplanes; i++)
		{
			into = DotProduct(velocity, planes[i]);
			if (into >= 0.1)
			{
				continue;       // move doesn't interact with the plane
			}

			// slide along the plane
			G_PredictPlayerClipVelocity(velocity, planes[i], clipVelocity);

			// slide along the plane
			G_PredictPlayerClipVelocity(endVelocity, planes[i], endClipVelocity);

			// see if there is a second plane that the new move enters
			for (j = 0; j < numplanes; j++)
			{
				if (j == i)
				{
					continue;
				}

				if (DotProduct(clipVelocity, planes[j]) >= 0.1)
				{
					continue;       // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				G_PredictPlayerClipVelocity(clipVelocity, planes[j], clipVelocity);
				G_PredictPlayerClipVelocity(endClipVelocity, planes[j], endClipVelocity);

				// see if it goes back into the first clip plane
				if (DotProduct(clipVelocity, planes[i]) >= 0)
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, velocity);
				VectorScale(dir, d, clipVelocity);

				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, endVelocity);
				VectorScale(dir, d, endClipVelocity);

				// see if there is a third plane the the new move enters
				for (k = 0; k < numplanes; k++)
				{
					if (k == i || k == j)
					{
						continue;
					}

					if (DotProduct(clipVelocity, planes[k]) >= 0.1)
					{
						continue;       // move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear(velocity);
					VectorCopy(origin, ent->s.pos.trBase);
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy(clipVelocity, velocity);
			VectorCopy(endClipVelocity, endVelocity);
			break;
		}
	}

	VectorCopy(endVelocity, velocity);
	VectorCopy(origin, ent->s.pos.trBase);

	return (bumpcount != 0);
}

#define STEPSIZE 18

/**
 * @brief Advance the given entity frametime seconds, stepping and sliding as appropriate
 * @param[in,out] ent
 * @param[in] frametime
 */
void G_PredictPlayerStepSlideMove(gentity_t *ent, float frametime)
{
	vec3_t  start_o, start_v, down_o, down_v;
	vec3_t  down, up;
	trace_t trace;
	float   stepSize;

	VectorCopy(ent->s.pos.trBase, start_o);
	VectorCopy(ent->s.pos.trDelta, start_v);

	if (!G_PredictPlayerSlideMove(ent, frametime))
	{
		// not clipped, so forget stepping
		return;
	}

	VectorCopy(ent->s.pos.trBase, down_o);
	VectorCopy(ent->s.pos.trDelta, down_v);

	VectorCopy(start_o, up);
	up[2] += STEPSIZE;

	// test the player position if they were a stepheight higher
	trap_Trace(&trace, start_o, ent->r.mins, ent->r.maxs, up, ent->s.number, ent->clipmask);
	if (trace.allsolid)
	{
		return;     // can't step up
	}

	stepSize = trace.endpos[2] - start_o[2];

	// try slidemove from this position
	VectorCopy(trace.endpos, ent->s.pos.trBase);
	VectorCopy(start_v, ent->s.pos.trDelta);

	G_PredictPlayerSlideMove(ent, frametime);

	// push down the final amount
	VectorCopy(ent->s.pos.trBase, down);
	down[2] -= stepSize;
	trap_Trace(&trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, ent->clipmask);
	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, ent->s.pos.trBase);
	}
	if (trace.fraction < 1.0)
	{
		G_PredictPlayerClipVelocity(ent->s.pos.trDelta, trace.plane.normal, ent->s.pos.trDelta);
	}
}

/**
 * @brief Advance the given entity frametime seconds, stepping and sliding as appropriate
 * This is the entry point to the server-side-only prediction code
 * @param[in] ent
 * @param[in] frametime
 */
void G_PredictPlayerMove(gentity_t *ent, float frametime)
{
	G_PredictPlayerStepSlideMove(ent, frametime);
}
