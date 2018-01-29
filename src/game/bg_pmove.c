/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2017 ET:Legacy team <mail@etlegacy.com>
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
 * @file bg_pmove.c
 * @brief Both games player movement code
 *
 * Takes a playerstate and a usercmd as input and returns a modifed playerstate.
 */

#ifdef CGAMEDLL
#include "../cgame/cg_local.h"
#else
#include "../qcommon/q_shared.h"
#include "bg_public.h"
#endif // CGAMEDLL

#include "bg_local.h"

#ifdef CGAMEDLL
#define PM_FIXEDPHYSICS         cgs.fixedphysics
#define PM_FIXEDPHYSICSFPS      cgs.fixedphysicsfps
#define PM_PRONEDELAY           cgs.pronedelay

#elif GAMEDLL
extern vmCvar_t g_fixedphysics;
extern vmCvar_t g_fixedphysicsfps;
extern vmCvar_t g_pronedelay;

#define PM_FIXEDPHYSICS         g_fixedphysics.integer
#define PM_FIXEDPHYSICSFPS      g_fixedphysicsfps.integer
#define PM_PRONEDELAY           g_pronedelay.integer

#endif

#define AIMSPREAD_MAXSPREAD 255
#define MAX_AIMSPREAD_TIME 1000

pmove_t *pm;
pml_t   pml;

// movement parameters
float pm_stopspeed = 100;

float pm_waterSwimScale = 0.5;
//float pm_waterWadeScale = 0.70f;
float pm_slagSwimScale = 0.30f;
//float pm_slagWadeScale  = 0.70f;

float pm_proneSpeedScale = 0.21f;    // was: 0.18 (too slow) then: 0.24 (too fast)

float pm_accelerate      = 10;
float pm_airaccelerate   = 1;
float pm_wateraccelerate = 4;
float pm_slagaccelerate  = 2;
float pm_flyaccelerate   = 8;

float pm_friction      = 6;
float pm_waterfriction = 1;
float pm_slagfriction  = 1;
//float pm_flightfriction    = 3;
float pm_ladderfriction    = 14;
float pm_spectatorfriction = 5.0f;

int c_pmove = 0;

#ifdef GAMEDLL

// In just the GAME DLL, we want to store the groundtrace surface stuff,
// so we don't have to keep tracing.
void ClientStoreSurfaceFlags(int clientNum, int surfaceFlags);

#endif

static void PM_BeginWeaponChange(weapon_t oldWeapon, weapon_t newWeapon, qboolean reload);

/**
 * @brief PM_AddEvent
 * @param[in] newEvent
 */
void PM_AddEvent(int newEvent)
{
	BG_AddPredictableEventToPlayerstate(newEvent, 0, pm->ps);
}

/**
 * @brief PM_AddEventExt
 * @param[in] newEvent
 * @param[in] eventParm
 */
void PM_AddEventExt(int newEvent, int eventParm)
{
	BG_AddPredictableEventToPlayerstate(newEvent, eventParm, pm->ps);
}

/**
 * @brief PM_ReloadAnimForWeapon
 * @param[in] weapon
 * @return
 */
int PM_ReloadAnimForWeapon(int weapon)
{
	if (pm->skill[SK_LIGHT_WEAPONS] >= 2 && GetWeaponTableData(weapon)->isLightWeaponSupportingFastReload)
	{
		return WEAP_RELOAD2;        // faster reload
	}

	return GetWeaponTableData(weapon)->reloadAnim;
}

/**
 * @brief PM_AddTouchEnt
 * @param[in] entityNum
 */
void PM_AddTouchEnt(int entityNum)
{
	int i;

	if (entityNum == ENTITYNUM_WORLD)
	{
		return;
	}
	if (pm->numtouch == MAXTOUCH)
	{
		return;
	}

	// see if it is already added
	for (i = 0 ; i < pm->numtouch ; i++)
	{
		if (pm->touchents[i] == entityNum)
		{
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/**
 * @brief PM_StartWeaponAnim
 * @param[in] anim
 */
static void PM_StartWeaponAnim(int anim)
{
	if (pm->ps->pm_type >= PM_DEAD)
	{
		return;
	}

	if (pm->pmext->weapAnimTimer > 0)
	{
		return;
	}

	if (pm->cmd.weapon == WP_NONE)
	{
		return;
	}

	pm->ps->weapAnim = ((pm->ps->weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

/**
 * @brief PM_ContinueWeaponAnim
 * @param[in] anim
 */
void PM_ContinueWeaponAnim(int anim)
{
	if (pm->cmd.weapon == WP_NONE)
	{
		return;
	}

	if ((pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == anim)
	{
		return;
	}
	if (pm->pmext->weapAnimTimer > 0)
	{
		return;     // a high priority animation is running
	}
	PM_StartWeaponAnim(anim);
}

/**
 * @brief Slide off of the impacting surface
 * @param[in] in
 * @param[in] normal
 * @param[out] out
 * @param[in] overbounce
 */
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
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

/**

 */
/**
 * @brief finds worst trace of body/legs, for collision.
 * @param[in,out] trace
 * @param[out] legsOffset
 * @param[in] start
 * @param[in] end
 * @param[in] bodytrace
 * @param[in] viewangles
 * @param[in] ignoreent
 * @param[in] tracemask
 */
void PM_TraceLegs(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end, trace_t *bodytrace, vec3_t viewangles, void(tracefunc) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int ignoreent, int tracemask)
{
	vec3_t ofs, org, point;
	vec3_t flatforward;
	float  angle;

	// zinx - don't let players block legs
	tracemask &= ~(CONTENTS_BODY | CONTENTS_CORPSE);

	if (legsOffset)
	{
		*legsOffset = 0;
	}

	angle          = DEG2RAD(viewangles[YAW]);
	flatforward[0] = cos(angle);
	flatforward[1] = sin(angle);
	flatforward[2] = 0;

	if (pm->ps->eFlags & EF_PRONE)
	{
		VectorScale(flatforward, -32, ofs);
	}
	else
	{
		VectorScale(flatforward, 32, ofs);
	}

	VectorAdd(start, ofs, org);
	VectorAdd(end, ofs, point);
	tracefunc(trace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
	if (!bodytrace || trace->fraction < bodytrace->fraction ||
	    trace->allsolid)
	{
		trace_t steptrace;

		// legs are clipping sooner than body
		// see if our legs can step up

		// give it a try with the new height
		ofs[2] += STEPSIZE;

		VectorAdd(start, ofs, org);
		VectorAdd(end, ofs, point);
		tracefunc(&steptrace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
		if (!steptrace.allsolid && !steptrace.startsolid &&
		    steptrace.fraction > trace->fraction)
		{
			// the step trace did better -- use it instead
			*trace = steptrace;

			// get legs offset
			if (legsOffset)
			{
				*legsOffset = ofs[2];

				VectorCopy(steptrace.endpos, org);
				VectorCopy(steptrace.endpos, point);
				point[2] -= STEPSIZE;

				tracefunc(&steptrace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
				if (!steptrace.allsolid)
				{
					*legsOffset = ofs[2] - (org[2] - steptrace.endpos[2]);
				}
			}
		}
	}
}

/**
 * @brief PM_TraceHead
 * @param[in] trace
 * @param[in] start
 * @param[in] end
 * @param bodytrace - unused
 * @param[in] viewangles
 * @param[in] ignoreent
 * @param[in] tracemask
 */
void PM_TraceHead(trace_t *trace, vec3_t start, vec3_t end, trace_t *bodytrace, vec3_t viewangles,
                  void(tracefunc) (trace_t * results,
                                   const vec3_t start,
                                   const vec3_t mins,
                                   const vec3_t maxs,
                                   const vec3_t end,
                                   int passEntityNum,
                                   int contentMask),
                  int ignoreent,
                  int tracemask)
{
	vec3_t ofs;
	vec3_t flatforward;
	vec3_t point;
	float  angle;
	// more than just head, try to make a box for all the
	// player model that extends out (weapons and arms too)
	vec3_t mins = { -18.f, -18.f, -2.f };
	vec3_t maxs = { 18.f, 18.f, 10.f };

	// don't let players block head
	tracemask &= ~(CONTENTS_BODY | CONTENTS_CORPSE);

	angle          = DEG2RAD(viewangles[YAW]);
	flatforward[0] = cos(angle);
	flatforward[1] = sin(angle);
	flatforward[2] = 0;

	if (pm->ps->eFlags & EF_PRONE)
	{
		VectorScale(flatforward, 36, ofs);
	}
	else
	{
		VectorScale(flatforward, -36, ofs);
	}

	VectorAdd(end, ofs, point);
	tracefunc(trace, start, mins, maxs, point, ignoreent, tracemask);
}

/**
 * @brief Traces all player bboxes -- body, legs, and head
 * @param[in,out] trace
 * @param[out] legsOffset
 * @param[in] start
 * @param[in] end
 */
void PM_TraceAllParts(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end)
{
	pm->trace(trace, start, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

	// legs and head
	if ((pm->ps->eFlags & EF_PRONE) ||
	    (pm->ps->eFlags & EF_DEAD))
	{

		trace_t  legtrace;
		trace_t  headtrace;
		qboolean adjust = qfalse;

		PM_TraceLegs(&legtrace, legsOffset, start, end, trace,
		             pm->ps->viewangles, pm->trace, pm->ps->clientNum,
		             pm->tracemask);

		if (legtrace.fraction < trace->fraction ||
		    legtrace.startsolid ||
		    legtrace.allsolid)
		{

			*trace = legtrace;
			adjust = qtrue;
		}

		PM_TraceHead(&headtrace, start, end, trace,
		             pm->ps->viewangles, pm->trace, pm->ps->clientNum,
		             pm->tracemask);

		if (headtrace.fraction < trace->fraction ||
		    headtrace.startsolid ||
		    headtrace.allsolid)
		{

			*trace = headtrace;
			adjust = qtrue;
		}

		if (adjust)
		{
			VectorSubtract(end, start, trace->endpos);
			VectorMA(start, trace->fraction, trace->endpos,
			         trace->endpos);
		}
	}
}

/**
 * @brief PM_TraceAll
 * @param[in,out] trace
 * @param[in] start
 * @param[in] end
 */
void PM_TraceAll(trace_t *trace, vec3_t start, vec3_t end)
{
	PM_TraceAllParts(trace, NULL, start, end);
}

/**
 * @brief Handles both ground friction and water friction
 */
static void PM_Friction(void)
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
		vel[1] = 0;     // allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if (pm->waterlevel <= 1)
	{
		if (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK))
		{
			// if getting knocked back, no friction
			if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
			{
				float control = speed < pm_stopspeed ? pm_stopspeed : speed;

				drop += control * pm_friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if (pm->waterlevel)
	{
		if (pm->watertype & CONTENTS_SLIME)     // slag
		{
			drop += speed * pm_slagfriction * pm->waterlevel * pml.frametime;
		}
		else
		{
			drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
		}
	}

	if (pm->ps->pm_type == PM_SPECTATOR)
	{
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// apply ladder strafe friction
	if (pml.ladder)
	{
		drop += speed * pm_ladderfriction * pml.frametime;
	}

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
 * @brief Handles user intended acceleration
 * @param[in] wishdir
 * @param[in] wishspeed
 * @param[in] accel
 */
static void PM_Accelerate(vec3_t wishdir, float wishspeed, float accel)
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

	for (i = 0 ; i < 3 ; i++)
	{
		pm->ps->velocity[i] += accelspeed * wishdir[i];
	}
}

/**
 * @brief This allows the clients to use axial -127 to 127 values for all directions
 * without getting a sqrt(2) distortion in speed.
 * @param[in] cmd
 * @return The scale factor to apply to cmd movements
 */
static float PM_CmdScale(usercmd_t *cmd)
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

	// half move speed if heavy weapon is carried
	// this is the counterstrike way of doing it -- ie you can switch to a non-heavy weapon and move at
	// full speed.  not completely realistic (well, sure, you can run faster with the weapon strapped to your
	// back than in carry position) but more fun to play.  If it doesn't play well this way we'll bog down the
	// player if the own the weapon at all.
	if (GetWeaponTableData(pm->ps->weapon)->isHeavyWeapon && !GetWeaponTableData(pm->ps->weapon)->isMortarSet)
	{
		if (pm->ps->weapon == WP_FLAMETHROWER) // trying some different balance for the FT
		{
			if (!(pm->skill[SK_HEAVY_WEAPONS] >= 3) || (pm->cmd.buttons & BUTTON_ATTACK))
			{
				scale *= 0.7f;
			}
		}
		else
		{
			if (pm->skill[SK_HEAVY_WEAPONS] >= 3)
			{
				scale *= 0.75f;
			}
			else
			{
				scale *= 0.5f;
			}
		}
	}

	return scale;
}

/**
 * @brief Determine the rotation of the legs reletive
 * to the facing dir
 */
static void PM_SetMovementDir(void)
{
	// changed - for more realistic angles (at the cost of more network traffic?)
	float  speed;
	vec3_t moved;

	VectorSubtract(pm->ps->origin, pml.previous_origin, moved);

	if ((pm->cmd.forwardmove || pm->cmd.rightmove)
	    &&  (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	    &&  (speed = VectorLength(moved)) != 0.f
	    &&  (speed > pml.frametime * 5))          // if moving slower than 20 units per second, just face head angles
	{
		vec3_t dir;
		int    moveyaw;

		VectorNormalize2(moved, dir);
		vectoangles(dir, dir);

		moveyaw = (int)(AngleDelta(dir[YAW], pm->ps->viewangles[YAW]));

		if (pm->cmd.forwardmove < 0)
		{
			moveyaw = (int)(AngleNormalize180(moveyaw + 180));
		}

		if (abs(moveyaw) > 75)
		{
			if (moveyaw > 0)
			{
				moveyaw = 75;
			}
			else
			{
				moveyaw = -75;
			}
		}

		pm->ps->movementDir = (signed char)moveyaw;
	}
	else
	{
		pm->ps->movementDir = 0;
	}
}

/**
 * @brief PM_CheckJump
 * @return
 */
static qboolean PM_CheckJump(void)
{
	// no jumpin when prone
	if (pm->ps->eFlags & EF_PRONE)
	{
		return qfalse;
	}

	// jumping in multiplayer uses and requires sprint juice (to prevent turbo skating, sprint + jumps)
	// don't allow jump accel

	// revert to using pmext for this since pmext is fixed now.
	if (pm->cmd.serverTime - pm->pmext->jumpTime < 850)
	{
		return qfalse;
	}

	// don't allow if player tired
	//if (pm->pmext->sprintTime < 2500) // JPW pulled this per id request; made airborne jumpers wildly inaccurate with gunfire to compensate
	//    return qfalse;


	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return qfalse;      // don't allow jump until all buttons are up
	}

	if (pm->cmd.upmove < 10)
	{
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if (pm->ps->pm_flags & PMF_JUMP_HELD)
	{
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane   = qfalse;     // jumping away
	pml.walking       = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->velocity[2]     = JUMP_VELOCITY;

	if (pm->cmd.forwardmove >= 0)
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMP, qfalse, qtrue);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMPBK, qfalse, qtrue);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/**
 * @brief PM_CheckWaterJump
 * @return
 */
static qboolean PM_CheckWaterJump(void)
{
	vec3_t spot;
	int    cont;
	vec3_t flatforward;

	if (pm->ps->pm_time)
	{
		return qfalse;
	}

	// check for water jump
	if (pm->waterlevel != 2)
	{
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont     = pm->pointcontents(spot, pm->ps->clientNum);
	if (!(cont & CONTENTS_SOLID))
	{
		return qfalse;
	}

	spot[2] += 16;
	cont     = pm->pointcontents(spot, pm->ps->clientNum);
	if (cont)
	{
		return qfalse;
	}

	// jump out of water
	VectorScale(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time   = 2000;

	return qtrue;
}

/**
 * @brief Sets mins, maxs, and pm->ps->viewheight
 * @return
 */
static qboolean PM_CheckProne(void)
{
	//Com_Printf( "%i: PM_CheckProne\n", pm->cmd.serverTime);
	int pronedelay = 750;

	if (PM_PRONEDELAY)
	{
		pronedelay = 1750;
	}

	if (!(pm->ps->eFlags & EF_PRONE))
	{
		// can't go prone on ladders
		if (pm->ps->pm_flags & PMF_LADDER)
		{
			return qfalse;
		}

		// no prone when player is mounted
		if (BG_PlayerMounted(pm->ps->eFlags))
		{
			return qfalse;
		}

		if (pm->ps->weaponDelay && GetWeaponTableData(pm->ps->weapon)->isPanzer)
		{
			return qfalse;
		}

		if (GetWeaponTableData(pm->ps->weapon)->isMortarSet)
		{
			return qfalse;
		}

		// can't go prone while swimming
		if (pm->waterlevel > 1)
		{
			return qfalse;
		}

		if ((((pm->ps->pm_flags & PMF_DUCKED) && pm->cmd.doubleTap == DT_FORWARD) ||
		     (pm->cmd.wbuttons & WBUTTON_PRONE)) && pm->cmd.serverTime - -pm->pmext->proneTime > pronedelay)
		{
			trace_t trace;

			pm->mins[0] = pm->ps->mins[0];
			pm->mins[1] = pm->ps->mins[1];

			pm->maxs[0] = pm->ps->maxs[0];
			pm->maxs[1] = pm->ps->maxs[1];

			pm->mins[2] = pm->ps->mins[2];
			pm->maxs[2] = pm->ps->crouchMaxZ;

			pm->ps->eFlags |= EF_PRONE;
			PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
			pm->ps->eFlags &= ~EF_PRONE;

			if (PM_PRONEDELAY)
			{
				pm->ps->aimSpreadScale      = AIMSPREAD_MAXSPREAD;
				pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;
			}

			if (trace.fraction == 1.0f)
			{
				// go prone
				pm->ps->pm_flags    |= PMF_DUCKED;       // crouched as well
				pm->ps->eFlags      |= EF_PRONE;
				pm->pmext->proneTime = pm->cmd.serverTime;       // timestamp 'go prone'
			}
		}
	}

	if (pm->ps->eFlags & EF_PRONE)
	{
		if (pm->waterlevel > 1 ||
		    pm->ps->pm_type == PM_DEAD ||
		    BG_PlayerMounted(pm->ps->eFlags) ||
		    ((pm->cmd.doubleTap == DT_BACK || pm->cmd.upmove > 10 || (pm->cmd.wbuttons & WBUTTON_PRONE)) && pm->cmd.serverTime - pm->pmext->proneTime > pronedelay))
		{
			trace_t trace;

			// see if we have the space to stop prone
			pm->mins[0] = pm->ps->mins[0];
			pm->mins[1] = pm->ps->mins[1];

			pm->maxs[0] = pm->ps->maxs[0];
			pm->maxs[1] = pm->ps->maxs[1];

			pm->mins[2] = pm->ps->mins[2];
			pm->maxs[2] = pm->ps->crouchMaxZ;

			pm->ps->eFlags &= ~EF_PRONE;
			PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
			pm->ps->eFlags |= EF_PRONE;

			if (trace.fraction == 1.0f)
			{
				// crouch for a bit
				pm->ps->pm_flags |= PMF_DUCKED;

				// stop prone
				pm->ps->eFlags      &= ~EF_PRONE;
				pm->ps->eFlags      &= ~EF_PRONE_MOVING;
				pm->pmext->proneTime = -pm->cmd.serverTime; // timestamp 'stop prone'

				// don't let them keep scope out when
				// standing from prone or they will
				// look right through a wall
				if (GetWeaponTableData(pm->ps->weapon)->isScoped || GetWeaponTableData(pm->ps->weapon)->isMGSet)
				{
					PM_BeginWeaponChange((weapon_t)pm->ps->weapon, GetWeaponTableData(pm->ps->weapon)->weapAlts, qfalse);
				}

				// don't jump for a bit
				pm->pmext->jumpTime = pm->cmd.serverTime - 650;
				pm->ps->jumpTime    = pm->cmd.serverTime - 650;
			}
		}
	}

	if (pm->ps->eFlags & EF_PRONE)
	{
		// See if we are moving
		float    spd       = VectorLength(pm->ps->velocity);
		qboolean userinput = abs(pm->cmd.forwardmove) + abs(pm->cmd.rightmove) > 10 ? qtrue : qfalse;

		if (userinput && spd > 40.f && !(pm->ps->eFlags & EF_PRONE_MOVING))
		{
			pm->ps->eFlags |= EF_PRONE_MOVING;
		}
		else if (!userinput && spd < 20.0f && (pm->ps->eFlags & EF_PRONE_MOVING))
		{
			pm->ps->eFlags &= ~EF_PRONE_MOVING;
		}

		pm->mins[0] = pm->ps->mins[0];
		pm->mins[1] = pm->ps->mins[1];

		pm->maxs[0] = pm->ps->maxs[0];
		pm->maxs[1] = pm->ps->maxs[1];

		pm->mins[2] = pm->ps->mins[2];

		// it appears that 12 is the magic number
		// for the minimum maxs[2] that prevents
		// player from getting stuck into the world.
		pm->maxs[2]        = 12;
		pm->ps->viewheight = PRONE_VIEWHEIGHT;

		return qtrue;
	}

	return qfalse;
}

//============================================================================

/**
 * @brief Flying out of the water
 */
static void PM_WaterJumpMove(void)
{
	// waterjump has no control, but falls

	PM_StepSlideMove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0)
	{
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time   = 0;
	}
}

/**
 * @brief PM_WaterMove
 */
static void PM_WaterMove(void)
{
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	if (PM_CheckWaterJump())
	{
		PM_WaterJumpMove();
		return;
	}

	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);

	// user intentions

	if (scale == 0.f)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;     // sink towards bottom
	}
	else
	{
		int i;

		for (i = 0 ; i < 3 ; i++)
		{
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}
		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (pm->watertype & CONTENTS_SLIME)        // slag
	{
		if (wishspeed > pm->ps->speed * pm_slagSwimScale)
		{
			wishspeed = pm->ps->speed * pm_slagSwimScale;
		}

		PM_Accelerate(wishdir, wishspeed, pm_slagaccelerate);
	}
	else
	{
		if (wishspeed > pm->ps->speed * pm_waterSwimScale)
		{
			wishspeed = pm->ps->speed * pm_waterSwimScale;
		}

		PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);
	}

	// make sure we can go up slopes easily under water
	if (pml.groundPlane && DotProduct(pm->ps->velocity, pml.groundTrace.plane.normal) < 0)
	{
		float vel = VectorLength(pm->ps->velocity);

		// slide along the ground plane
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
		                pm->ps->velocity, OVERCLIP);

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove(qfalse);
}

/**
 * @brief Only with the flight powerup
 */
static void PM_FlyMove(void)
{
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);

	// spectator boost
	if (pm->cmd.buttons & BUTTON_SPRINT)
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

		for (i = 0 ; i < 3 ; i++)
		{
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate(wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove(qfalse);
}

/**
 * @brief PM_AirMove
 */
static void PM_AirMove(void)
{
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd   = pm->cmd;
	scale = PM_CmdScale(&cmd);

	pml.forward[2] = 0;
	pml.right[2]   = 0;

	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	wishvel[0] = pml.forward[0] * fmove + pml.right[0] * smove;
	wishvel[1] = pml.forward[1] * fmove + pml.right[1] * smove;

	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate(wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity slide along the steep plane
	if (pml.groundPlane)
	{
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
		                pm->ps->velocity, OVERCLIP);
	}

	PM_StepSlideMove(qtrue);

	// moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();
}

/**
 * @brief PM_WalkMove
 */
static void PM_WalkMove(void)
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;
	float     accelerate;
	float     vel;

	if (pm->waterlevel > 2 && DotProduct(pml.forward, pml.groundTrace.plane.normal) > 0)
	{
		// begin swimming
		PM_WaterMove();
		return;
	}

	if (PM_CheckJump())
	{
		// jumped away
		if (pm->waterlevel > 1)
		{
			PM_WaterMove();
		}
		else
		{
			PM_AirMove();
		}

		if (!(pm->cmd.serverTime - pm->pmext->jumpTime < 850))
		{
			pm->pmext->sprintTime -= 2500;
			if (pm->pmext->sprintTime < 0)
			{
				pm->pmext->sprintTime = 0;
			}

			pm->pmext->jumpTime = pm->cmd.serverTime;
		}

		pm->ps->jumpTime = pm->cmd.serverTime;

		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd   = pm->cmd;
	scale = PM_CmdScale(&cmd);

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2]   = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity(pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP);
	PM_ClipVelocity(pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP);
	//
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for (i = 0 ; i < 3 ; i++)
	{
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
	//wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if prone
	if (pm->ps->eFlags & EF_PRONE)
	{
		if (wishspeed > pm->ps->speed * pm_proneSpeedScale)
		{
			// cap the max prone speed while reloading and mouting/unmouting alt weapon
			if (pm->ps->weaponstate == WEAPON_RELOADING ||
			    (pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == GetWeaponTableData(pm->ps->weapon)->altSwitchFrom ||
			    (pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == GetWeaponTableData(pm->ps->weapon)->altSwitchTo)
			{
				wishspeed = (wishspeed < 40.f) ? pm->ps->speed * pm_proneSpeedScale : 40.f;
			}
			else
			{
				wishspeed = pm->ps->speed * pm_proneSpeedScale;
			}
		}
	}
	else if (pm->ps->pm_flags & PMF_DUCKED)       // clamp the speed lower if ducking
	{
		if (wishspeed > pm->ps->speed * pm->ps->crouchSpeedScale)
		{
			wishspeed = pm->ps->speed * pm->ps->crouchSpeedScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if (pm->waterlevel)
	{
		float waterScale;

		waterScale = pm->waterlevel / 3.0f;
		if (pm->watertype & CONTENTS_SLIME)     // slag cont
		{
			waterScale = 1.0f - (1.0f - pm_slagSwimScale) * waterScale;
		}
		else
		{
			waterScale = 1.0f - (1.0f - pm_waterSwimScale) * waterScale;
		}

		if (wishspeed > pm->ps->speed * waterScale)
		{
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ((pml.groundTrace.surfaceFlags & SURF_SLICK) || (pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
	{
		accelerate = pm_airaccelerate;
	}
	else
	{
		accelerate = pm_accelerate;
	}

	PM_Accelerate(wishdir, wishspeed, accelerate);

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ((pml.groundTrace.surfaceFlags & SURF_SLICK) || (pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
	{
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	}

	// show breath when standing on 'snow' surfaces
	if (pml.groundTrace.surfaceFlags & SURF_SNOW)
	{
		pm->ps->eFlags |= EF_BREATH;
	}
	else
	{
		pm->ps->eFlags &= ~EF_BREATH;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);

	// don't do anything if standing still
	if (pm->ps->velocity[0] == 0.f && pm->ps->velocity[1] == 0.f)
	{
		return;
	}

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	PM_StepSlideMove(qfalse);

	// moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();
}

/**
 * @brief PM_DeadMove
 */
static void PM_DeadMove(void)
{
	float forward;

	if (!pml.walking)
	{
		return;
	}

	// extra friction
	forward  = VectorLength(pm->ps->velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear(pm->ps->velocity);
	}
	else
	{
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

/**
 * @brief PM_NoclipMove
 */
static void PM_NoclipMove(void)
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
	scale = PM_CmdScale(&pm->cmd);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i = 0 ; i < 3 ; i++)
	{
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] += pm->cmd.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	VectorMA(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/**
 * @brief PM_FootstepForSurface
 * @return An event number apropriate for the groundsurface
 */
static int PM_FootstepForSurface(void)
{
#ifdef GAMEDLL
	// In just the GAME DLL, we want to store the groundtrace surface stuff,
	// so we don't have to keep tracing.
	ClientStoreSurfaceFlags(pm->ps->clientNum, pml.groundTrace.surfaceFlags);
#endif // GAMEDLL

	return BG_FootstepForSurface(pml.groundTrace.surfaceFlags);
}

/**
 * @brief Check for hard landings that generate sound events
 */
static void PM_CrashLand(void)
{
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// Ridah, only play this if coming down hard
	if (!pm->ps->legsTimer)
	{
		if (pml.previous_velocity[2] < -220)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_LAND, qfalse, qtrue);
		}
	}

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel  = pml.previous_velocity[2];
	acc  = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if (den < 0)
	{
		return;
	}
	t = (-b - (float)sqrt(den)) / (2 * a);

	delta = vel + t * acc;
	delta = delta * delta * 0.0001f;

	// never take falling damage if completely underwater
	if (pm->waterlevel == 3)
	{
#ifdef GAMEDLL // nothing to predict
		pm->pmext->shoved = qfalse;
#endif
		return;
	}

	// reduce falling damage if there is standing water
	if (pm->waterlevel == 2)
	{
		delta *= 0.25;
	}
	if (pm->waterlevel == 1)
	{
		delta *= 0.5;
	}

	if (delta < 1)
	{
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if (!(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) && !pm->predict)
	{
		if (pm->debugLevel)
		{
			Com_Printf("delta: %5.2f\n", (double)delta);
		}

		if (delta > 77)
		{
			PM_AddEventExt(EV_FALL_NDIE, PM_FootstepForSurface());
		}
		else if (delta > 67)
		{
			PM_AddEventExt(EV_FALL_DMG_50, PM_FootstepForSurface());
		}
		else if (delta > 58)
		{
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_25, PM_FootstepForSurface());
			}
		}
		else if (delta > 48)
		{
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_15, PM_FootstepForSurface());
			}
		}
		else if (delta > 38.75f)
		{
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_10, PM_FootstepForSurface());
			}
		}
		else if (delta > 7)
		{
			PM_AddEventExt(EV_FALL_SHORT, PM_FootstepForSurface());
		}
		else
		{
			PM_AddEventExt(EV_FOOTSTEP, PM_FootstepForSurface());
		}
	}

	// when falling damage happens, velocity is cleared, but
	// this needs to happen in pmove, not g_active!  (prediction will be
	// wrong, otherwise.)
	if (delta > 38.75f)
	{
		VectorClear(pm->ps->velocity);
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/**
 * @brief PM_CorrectAllSolid
 * @param[in] trace
 * @return
 */
static int PM_CorrectAllSolid(trace_t *trace)
{
	int    i, j, k;
	vec3_t point;

	if (pm->debugLevel)
	{
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++)
	{
		for (j = -1; j <= 1; j++)
		{
			for (k = -1; k <= 1; k++)
			{
				if (!i && !j && !k)   // same as checking input origin again..
				{
					continue;
				}

				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				PM_TraceAll(trace, point, point);
				if (!trace->allsolid)
				{
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25f;

					PM_TraceAll(trace, pm->ps->origin, point);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane         = qfalse;
	pml.walking             = qfalse;

	return qfalse;
}

/**
 * @brief The ground trace didn't hit a surface, so we are in freefall
 */
static void PM_GroundTraceMissed(void)
{
	if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		trace_t trace;
		vec3_t  point;

		// we just transitioned into freefall
		if (pm->debugLevel)
		{
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy(pm->ps->origin, point);
		point[2] -= 64;

		PM_TraceAll(&trace, pm->ps->origin, point);
		if (trace.fraction == 1.0f)
		{
			if (pm->cmd.forwardmove >= 0)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMP, qfalse, qtrue);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMPBK, qfalse, qtrue);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	// If we've never yet touched the ground, it's because we're spawning, so don't
	// set to "in air"
	if (pm->ps->groundEntityNum != -1)
	{
		// Signify that we're in mid-air
		pm->ps->groundEntityNum = ENTITYNUM_NONE;

	} // if (pm->ps->groundEntityNum != -1)...
	pml.groundPlane = qfalse;
	pml.walking     = qfalse;
}

/**
 * @brief PM_GroundTrace
 */
static void PM_GroundTrace(void)
{
	vec3_t  point;
	trace_t trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];

	if ((pm->ps->eFlags & EF_MG42_ACTIVE) || (pm->ps->eFlags & EF_AAGUN_ACTIVE))
	{
		point[2] = pm->ps->origin[2] - 1.f;
	}
	else
	{
		point[2] = pm->ps->origin[2] - 0.25f;
	}

	PM_TraceAllParts(&trace, &pm->pmext->proneLegsOffset, pm->ps->origin, point);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if (trace.allsolid && !(pm->ps->eFlags & EF_MOUNTEDTANK))
	{
		if (!PM_CorrectAllSolid(&trace))
		{
			return;
		}
	}

	// if the trace didn't hit anything, we are in free fall
	if (trace.fraction == 1.0f)
	{
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking     = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if (pm->ps->velocity[2] > 0 && DotProduct(pm->ps->velocity, trace.plane.normal) > 10 && !(pm->ps->eFlags & EF_PRONE))
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation (but not under water)
		if (pm->waterlevel < 3)
		{
			if (pm->cmd.forwardmove >= 0)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMP, qfalse, qfalse);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMPBK, qfalse, qfalse);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane         = qfalse;
		pml.walking             = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if (trace.plane.normal[2] < MIN_WALK_NORMAL)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane         = qtrue;
		pml.walking             = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking     = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time   = 0;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		// just hit the ground
		if (pm->debugLevel)
		{
			Com_Printf("%i:land\n", c_pmove);
		}

		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if (pml.previous_velocity[2] < -200)
		{
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time   = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
	//pm->ps->velocity[2] = 0;

	PM_AddTouchEnt(trace.entityNum);
}

/**
 * @brief PM_SetWaterLevel
 *
 * @todo FIXME: avoid this twice?  certainly if not moving
 */
static void PM_SetWaterLevel(void)
{
	vec3_t point;
	int    cont;

	// get waterlevel, accounting for ducking
	pm->waterlevel = 0;
	pm->watertype  = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + pm->ps->mins[2] + 1;
	cont     = pm->pointcontents(point, pm->ps->clientNum);

	if (cont & MASK_WATER)
	{
		int sample2 = pm->ps->viewheight - pm->ps->mins[2];
		int sample1 = sample2 / 2;

		pm->watertype  = cont;
		pm->waterlevel = 1;
		point[2]       = pm->ps->origin[2] + pm->ps->mins[2] + sample1;
		cont           = pm->pointcontents(point, pm->ps->clientNum);
		if (cont & MASK_WATER)
		{
			pm->waterlevel = 2;
			point[2]       = pm->ps->origin[2] + pm->ps->mins[2] + sample2;
			cont           = pm->pointcontents(point, pm->ps->clientNum);
			if (cont & MASK_WATER)
			{
				pm->waterlevel = 3;
			}
		}
	}

	// UNDERWATER
	BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_UNDERWATER, (pm->waterlevel > 2), qtrue);
}

/**
 * @brief Sets mins, maxs, and pm->ps->viewheight
 */
static void PM_CheckDuck(void)
{
	// modified this for configurable bounding boxes
	pm->mins[0] = pm->ps->mins[0];
	pm->mins[1] = pm->ps->mins[1];

	pm->maxs[0] = pm->ps->maxs[0];
	pm->maxs[1] = pm->ps->maxs[1];

	pm->mins[2] = pm->ps->mins[2];

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2]        = pm->ps->maxs[2];   // NOTE: must set death bounding box in game code
		pm->ps->viewheight = pm->ps->deadViewHeight;
		return;
	}

	if ((pm->cmd.upmove < 0 && !(pm->ps->eFlags & EF_MOUNTEDTANK) && !(pm->ps->pm_flags & PMF_LADDER)) || GetWeaponTableData(pm->ps->weapon)->isMortarSet) // duck
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
			PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
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

//===================================================================

/**
 * @brief PM_Footsteps
 */
static void PM_Footsteps(void)
{
	float    bobmove;
	int      old;
	qboolean footstep;
	int      animResult = -1;   // FIXME: never used

	if (pm->ps->eFlags & EF_DEAD)
	{
		//if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
		if (pm->ps->pm_flags & PMF_FLAILING)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_FLAILING, qtrue);

			if (!pm->ps->pm_time)
			{
				pm->ps->pm_flags &= ~PMF_FLAILING;  // the eagle has landed
			}
		}
		else if (!pm->ps->pm_time && !(pm->ps->pm_flags & PMF_LIMBO))         // before going to limbo, play a wounded/fallen animation
		{
			if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
			{
				// takeoff!
				pm->ps->pm_flags |= PMF_FLAILING;
				animResult        = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_FLAILING, qtrue);
			}
			else
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_FALLEN, qtrue);
			}
		}

		return;
	}

	// calculate speed and cycle to be used for
	// all cyclic walking effects
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] +  pm->ps->velocity[1] * pm->ps->velocity[1]);

	// mg42, always idle
	if (pm->ps->persistant[PERS_HWEAPON_USE])
	{
		animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);

		return;
	}

	// swimming
	if (pm->waterlevel > 2 || (pm->waterlevel > 1 && pm->ps->groundEntityNum == ENTITYNUM_NONE))
	{
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_SWIMBK, qtrue);
		}
		else
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_SWIM, qtrue);
		}

		return;
	}

	// in the air
	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		if (pm->ps->pm_flags & PMF_LADDER)                 // on ladder
		{
			if (pm->ps->velocity[2] >= 0)
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_CLIMBUP, qtrue);
			}
			else
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_CLIMBDOWN, qtrue);
			}
		}

		return;
	}

	// if not trying to move
	if (!pm->cmd.forwardmove && !pm->cmd.rightmove)
	{
		if (pm->xyspeed < 5)
		{
			pm->ps->bobCycle = 0;   // start at beginning of cycle again
		}
		if (pm->xyspeed > 120)
		{
			return; // continue what they were doing last frame, until we stop
		}

		if (pm->ps->eFlags & EF_PRONE)
		{
			if (pm->ps->eFlags & EF_TALK)
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RADIOPRONE, qtrue);
			}
			else
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLEPRONE, qtrue);
			}
		}
		else if (pm->ps->pm_flags & PMF_DUCKED)
		{
			if (pm->ps->eFlags & EF_TALK)
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RADIOCR, qtrue);
			}
			else
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLECR, qtrue);
			}
		}

		if (animResult < 0)
		{
			if (pm->ps->eFlags & EF_TALK)
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RADIO, qtrue);
			}
			else
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
			}
		}

		return;
	}

	footstep = qfalse;

	if (pm->ps->eFlags & EF_PRONE)
	{
		bobmove = 0.2f;  // prone characters bob slower
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_PRONEBK, qtrue);
		}
		else
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_PRONE, qtrue);
		}
		// prone characters never play footsteps
	}
	else if (pm->ps->pm_flags & PMF_DUCKED)
	{
		bobmove = 0.5;  // ducked characters bob much faster
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALKCRBK, qtrue);
		}
		else
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALKCR, qtrue);
		}
		// ducked characters never play footsteps
	}
	else if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
	{
		if (!(pm->cmd.buttons & BUTTON_WALKING))
		{
			bobmove  = 0.4f; // faster speeds bob faster
			footstep = qtrue;
			// check for strafing
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RUNBK, qtrue);
			}
		}
		else
		{
			bobmove = 0.3f;
			// check for strafing
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALKBK, qtrue);
			}
		}
	}
	else
	{
		if (!(pm->cmd.buttons & BUTTON_WALKING))
		{
			bobmove  = 0.4f; // faster speeds bob faster
			footstep = qtrue;
			// check for strafing
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RUN, qtrue);
			}
		}
		else
		{
			bobmove = 0.3f;  // walking bobs slow
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALK, qtrue);
			}
		}
	}

	// if no anim found yet, then just use the idle as default
	if (animResult < 0)
	{
		animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
	}

	// check for footstep / splash sounds
	old              = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	if (((old + 64) ^ (pm->ps->bobCycle + 64)) & 128)
	{
		switch (pm->waterlevel)
		{
		case 0:
			// on ground will only play sounds if running
			if (footstep && !pm->noFootsteps)
			{
				PM_AddEventExt(EV_FOOTSTEP, PM_FootstepForSurface());
			}
			break;
		case 1:
			// splashing
			PM_AddEvent(EV_FOOTSPLASH);
			break;
		case 2:
			// wading / swimming at surface
			PM_AddEvent(EV_SWIM);
			break;
		case 3:
			// no sound when completely underwater
			break;
		default:
			break;
		}
	}
}

/**
 * @brief Generate sound events for entering and leaving water
 */
static void PM_WaterEvents(void)
{
	// if just entered a water volume, play a sound
	if (!pml.previous_waterlevel && pm->waterlevel)
	{
		PM_AddEvent(EV_WATER_TOUCH);
	}

	// if just completely exited a water volume, play a sound
	if (pml.previous_waterlevel && !pm->waterlevel)
	{
		PM_AddEvent(EV_WATER_LEAVE);
	}

	// check for head just going under water
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3)
	{
		PM_AddEvent(EV_WATER_UNDER);
	}

	// check for head just coming out of water
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3)
	{
		if (pm->pmext->airleft < 6000)
		{
			PM_AddEventExt(EV_WATER_CLEAR, 1);
		}
		else
		{
			PM_AddEventExt(EV_WATER_CLEAR, 0);
		}
	}

	// fix prediction, ensure full HOLDBREATHTIME when airleft isn't in use
	if (pm->pmext->airleft < 0 || pm->pmext->airleft > HOLDBREATHTIME)
	{
		pm->ps->stats[STAT_AIRLEFT] = HOLDBREATHTIME;
	}
	else
	{
		pm->ps->stats[STAT_AIRLEFT] = pm->pmext->airleft;
	}
}

/**
 * @brief PM_BeginWeaponReload
 * @param weapon
 */
static void PM_BeginWeaponReload(weapon_t weapon)
{
	int reloadTime;

	// only allow reload if the weapon isn't already occupied (firing is okay)
	if (pm->ps->weaponstate != WEAPON_READY && pm->ps->weaponstate != WEAPON_FIRING)
	{
		return;
	}

	if (!IS_VALID_WEAPON(weapon))
	{
		return;
	}

	// weapon which don't use ammo don't have reload anim
	// TODO: this check seem useless ! weapon which don't use ammo don't enter in this function
	if (!GetWeaponTableData(weapon)->useAmmo)
	{
		return;
	}

	// fixing reloading with a full clip
	if (pm->ps->ammoclip[GetWeaponTableData(weapon)->ammoIndex] >= GetWeaponTableData(weapon)->maxClip)
	{
		return;
	}

	// no reload when leaning (this includes manual and auto reloads)
	if (pm->ps->leanf != 0.f)
	{
		return;
	}

	// easier check now that the animation system handles the specifics
	// TODO: this check seem useless ! throwable weapon don't enter in this function
	if (!GetWeaponTableData(weapon)->isThrowable)
	{
		// override current animation (so reloading after firing will work)
		if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_RELOADPRONE, qfalse, qtrue);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_RELOAD, qfalse, qtrue);
		}
	}

	if (!(GetWeaponTableData(weapon)->isMortar || GetWeaponTableData(weapon)->isMortarSet))
	{
		PM_ContinueWeaponAnim(PM_ReloadAnimForWeapon(pm->ps->weapon));
	}

	// okay to reload while overheating without tacking the reload time onto the end of the
	// current weaponTime (the reload time is partially absorbed into the overheat time)
	reloadTime = GetWeaponTableData(weapon)->reloadTime;
	if (pm->skill[SK_LIGHT_WEAPONS] >= 2 && GetWeaponTableData(weapon)->isLightWeaponSupportingFastReload)
	{
		reloadTime *= .65f;
	}

	if (pm->ps->weaponstate == WEAPON_READY)
	{
		pm->ps->weaponTime += reloadTime;
	}
	else if (pm->ps->weaponTime < reloadTime)
	{
		pm->ps->weaponTime += (reloadTime - pm->ps->weaponTime);
	}

	pm->ps->weaponstate = WEAPON_RELOADING;
	PM_AddEvent(EV_FILL_CLIP);      // play reload sound
}

static void PM_ReloadClip(weapon_t weapon);

/**
 * @brief PM_BeginWeaponChange
 * @param[in] oldweapon
 * @param[in] newweapon
 * @param[in] reload
 */
static void PM_BeginWeaponChange(weapon_t oldWeapon, weapon_t newWeapon, qboolean reload)        // modified to play 1st person alt-mode transition animations.
{
	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return;     // don't allow weapon switch until all buttons are up
	}

	if (!IS_VALID_WEAPON(newWeapon))
	{
		return;
	}

	if (!(COM_BitCheck(pm->ps->weapons, newWeapon)))
	{
		return;
	}

	if (pm->ps->weaponstate == WEAPON_DROPPING || pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ||
	    pm->ps->weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	// don't allow another weapon switch when we're still swapping alt weap, to prevent animation breaking
	// there we check the value of the animation to prevent any switch during raising and dropping alt weapon
	// until the animation is ended
	if ((pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == GetWeaponTableData(oldWeapon)->altSwitchFrom ||
	    (pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == GetWeaponTableData(oldWeapon)->altSwitchTo)
	{
		return;
	}

	// don't allow change during spinup
	if (pm->ps->weaponDelay)
	{
		return;
	}

	// don't allow switch if you're holding a hot potato or dynamite
	if (pm->ps->grenadeTimeLeft > 0)
	{
		return;
	}

	pm->ps->nextWeapon = newWeapon;

	if (GetWeaponTableData(newWeapon)->isRifle)
	{
		if (newWeapon != GetWeaponTableData(oldWeapon)->weapAlts)
		{
			PM_AddEvent(EV_CHANGE_WEAPON);
		}
	}
	else if (GetWeaponTableData(newWeapon)->isGrenade || newWeapon == WP_DYNAMITE || newWeapon == WP_SMOKE_BOMB)
	{
		// initialize the timer on the potato you're switching to
		pm->ps->grenadeTimeLeft = 0;
		PM_AddEvent(EV_CHANGE_WEAPON);
	}
	else if (GetWeaponTableData(newWeapon)->isMortarSet)
	{
		if (pm->ps->eFlags & EF_PRONE)
		{
			return;
		}

		if (pm->waterlevel == 3)
		{
			return;
		}
		PM_AddEvent(EV_CHANGE_WEAPON);
	}
	else
	{
		// only play the weapon switch sound for the player
		PM_AddEvent(reload ? EV_CHANGE_WEAPON_2 : EV_CHANGE_WEAPON);
	}

	// it's an alt mode, play different anim
	if (newWeapon == GetWeaponTableData(oldWeapon)->weapAlts)
	{
		PM_StartWeaponAnim(GetWeaponTableData(oldWeapon)->altSwitchFrom);

		if (GetWeaponTableData(oldWeapon)->isRifle)
		{
			if (!pm->ps->ammoclip[newWeapon] && pm->ps->ammo[newWeapon])
			{
				PM_ReloadClip(newWeapon);
			}
		}
		else if (GetWeaponTableData(oldWeapon)->isMG || GetWeaponTableData(oldWeapon)->isMortar)
		{
			vec3_t axis[3];

			VectorCopy(pml.forward, axis[0]);
			VectorCopy(pml.right, axis[2]);
			CrossProduct(axis[0], axis[2], axis[1]);
			AxisToAngles(axis, pm->pmext->mountedWeaponAngles);
		}
	}
	else
	{
		PM_StartWeaponAnim(GetWeaponTableData(oldWeapon)->dropAnim);
	}

	// play an animation
	if (GetWeaponTableData(oldWeapon)->isSilencedPistol)
	{
		if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE, qfalse, qfalse);
		}
	}
	else
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_DROPWEAPON, qfalse, qfalse);
	}

	if (reload)
	{
		pm->ps->weaponstate = WEAPON_DROPPING_TORELOAD;
	}
	else
	{
		pm->ps->weaponstate = WEAPON_DROPPING;
	}

	if (newWeapon == GetWeaponTableData(oldWeapon)->weapAlts)
	{
		pm->ps->weaponTime += GetWeaponTableData(oldWeapon)->altSwitchTimeBegin;
	}
	else
	{
		pm->ps->weaponTime += GetWeaponTableData(oldWeapon)->switchTimeBegin;    // dropping/raising usually takes 1/4 sec.
	}
}

/**
 * @brief PM_FinishWeaponChange
 */
static void PM_FinishWeaponChange(void)
{
	weapon_t oldWeapon = (weapon_t)pm->ps->weapon, newWeapon = (weapon_t)pm->ps->nextWeapon;

	// Cannot switch to an invalid weapon
	if (!IS_VALID_WEAPON(newWeapon))
	{
		newWeapon = WP_NONE;
	}

	// Cannot switch to a weapon you don't have
	if (!(COM_BitCheck(pm->ps->weapons, newWeapon)))
	{
		newWeapon = WP_NONE;
	}

	pm->ps->weapon = newWeapon;

	if (pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD)
	{
		pm->ps->weaponstate = WEAPON_RAISING_TORELOAD;
	}
	else
	{
		pm->ps->weaponstate = WEAPON_RAISING;
	}

	// don't really care about anim since these weapons don't show in view.
	// However, need to set the animspreadscale so they are initally at worst accuracy
	if (GetWeaponTableData(newWeapon)->isScoped)
	{
		pm->ps->aimSpreadScale      = AIMSPREAD_MAXSPREAD;       // initially at lowest accuracy
		pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;       // initially at lowest accuracy
	}
	else if (GetWeaponTableData(newWeapon)->isPistol)
	{
		pm->pmext->silencedSideArm &= ~1;
	}
	else if (GetWeaponTableData(newWeapon)->isSilencedPistol)
	{
		pm->pmext->silencedSideArm |= 1;
	}
	else if (GetWeaponTableData(newWeapon)->isRifle)
	{
		pm->pmext->silencedSideArm &= ~2;
	}
	else if (GetWeaponTableData(newWeapon)->isRiflenade)
	{
		pm->pmext->silencedSideArm |= 2;
	}

	// doesn't happen too often (player switched weapons away then back very quickly)
	if (oldWeapon == newWeapon)
	{
		return;
	}

	// play an animation
	if (GetWeaponTableData(oldWeapon)->weapAlts == newWeapon)
	{
		if (GetWeaponTableData(newWeapon)->isRifle && !pm->ps->ammoclip[GetWeaponTableData(oldWeapon)->ammoIndex])
		{
			return;
		}

		pm->ps->weaponTime += GetWeaponTableData(newWeapon)->altSwitchTimeFinish;
		BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_WEAPON, newWeapon, qtrue);

		if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_DO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_DO_ALT_WEAPON_MODE, qfalse, qfalse);
		}

		// alt weapon switch was played when switching away, just go into idle
		PM_StartWeaponAnim(GetWeaponTableData(newWeapon)->altSwitchTo);
	}
	else
	{
		pm->ps->weaponTime += GetWeaponTableData(newWeapon)->switchTimeFinish;              // dropping/raising usually takes 1/4 sec.
		BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_WEAPON, newWeapon, qtrue);

		if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_RAISEWEAPONPRONE, qfalse, qfalse);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_RAISEWEAPON, qfalse, qfalse);
		}

		PM_StartWeaponAnim(GetWeaponTableData(newWeapon)->raiseAnim);
	}
}

/**
 * @brief PM_ReloadClip
 * @param[in] weapon
 */
static void PM_ReloadClip(weapon_t weapon)
{
	int ammoreserve = pm->ps->ammo[GetWeaponTableData(weapon)->ammoIndex];
	int ammoclip    = pm->ps->ammoclip[GetWeaponTableData(weapon)->clipIndex];
	int ammomove    = GetWeaponTableData(weapon)->maxClip - ammoclip;

	if (ammoreserve < ammomove)
	{
		ammomove = ammoreserve;
	}

	if (ammomove)
	{
		pm->ps->ammo[GetWeaponTableData(weapon)->ammoIndex]     -= ammomove;
		pm->ps->ammoclip[GetWeaponTableData(weapon)->clipIndex] += ammomove;
	}

	// reload akimbo stuff
	if (GetWeaponTableData(weapon)->isAkimbo)
	{
		PM_ReloadClip(GetWeaponTableData(weapon)->akimboSideArm);
	}
}

/**
 * @brief PM_FinishWeaponReload
 */
static void PM_FinishWeaponReload(void)
{
	PM_ReloadClip(pm->ps->weapon);            // move ammo into clip
	pm->ps->weaponstate = WEAPON_READY;     // ready to fire
	PM_StartWeaponAnim(GetWeaponTableData(pm->ps->weapon)->idleAnim);
}

/**
 * @brief PM_CheckForReload
 * @param[in] weapon
 */
void PM_CheckForReload(weapon_t weapon)
{
	qboolean autoreload;
	qboolean reloadRequested;
	int      clipWeap, ammoWeap;

	if (pm->noWeapClips)     // no need to reload
	{
		return;
	}

	// some weapons don't reload
	if (!GetWeaponTableData(weapon)->isReload)
	{
		return;
	}

	if (pm->ps->eFlags & EF_ZOOMING)
	{
		return;
	}

	switch (pm->ps->weaponstate)
	{
	case WEAPON_RAISING:
	case WEAPON_RAISING_TORELOAD:
	case WEAPON_DROPPING:
	case WEAPON_DROPPING_TORELOAD:
	case WEAPON_READYING:
	case WEAPON_RELAXING:
	case WEAPON_RELOADING:
		return;
	default:
		break;
	}

	// user is forcing a reload (manual reload)
	reloadRequested = (qboolean)(pm->cmd.wbuttons & WBUTTON_RELOAD);

	autoreload = pm->pmext->bAutoReload || !GetWeaponTableData(weapon)->isAutoReload;
	clipWeap   = GetWeaponTableData(weapon)->clipIndex;
	ammoWeap   = GetWeaponTableData(weapon)->ammoIndex;

	if (GetWeaponTableData(weapon)->isScoped)
	{
		if (reloadRequested && pm->ps->ammo[ammoWeap] && pm->ps->ammoclip[clipWeap] < GetWeaponTableData(weapon)->maxClip)
		{
			PM_BeginWeaponChange(weapon, GetWeaponTableData(weapon)->weapAlts, !(pm->ps->ammo[ammoWeap]) ? qfalse : qtrue);
		}
	}

	if (pm->ps->weaponTime <= 0)
	{
		qboolean doReload = qfalse;

		if (reloadRequested)
		{
			if (pm->ps->ammo[ammoWeap])
			{
				if (pm->ps->ammoclip[clipWeap] < GetWeaponTableData(weapon)->maxClip)
				{
					doReload = qtrue;
				}

				// akimbo should also check other weapon status
				if (GetWeaponTableData(weapon)->isAkimbo)
				{
					if (pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex] < GetWeaponTableData(GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex)->maxClip)
					{
						doReload = qtrue;
					}
				}
			}
		}
		else if (autoreload)
		{
			if (!pm->ps->ammoclip[clipWeap] && pm->ps->ammo[ammoWeap])
			{
				if (GetWeaponTableData(weapon)->isAkimbo)
				{
					if (!pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex])
					{
						doReload = qtrue;
					}
				}
				else
				{
					doReload = qtrue;
				}
			}
		}

		if (doReload)
		{
			PM_BeginWeaponReload(weapon);
		}
	}
}

/**
 * @brief PM_SwitchIfEmpty
 */
static void PM_SwitchIfEmpty(void)
{
	// weapon here are explosives or syringe/adrenaline, if they are not --> return
	if (!GetWeaponTableData(pm->ps->weapon)->isExplosive && !GetWeaponTableData(pm->ps->weapon)->isSyringe)  // WP_KNIFE WP_SATCHEL_DET
	{
		return;
	}

	// In multiplayer, pfaust fires once then switches to pistol since it's useless for a while
	// after throwing landmine, let switch to pliers
	if (GetWeaponTableData(pm->ps->weapon)->useAmmo && !GetWeaponTableData(pm->ps->weapon)->isPanzer && pm->ps->weapon != WP_LANDMINE)
	{
		// WP_M7 and WP_GPG40 run out of ammo immediately after firing their last grenade
		if (GetWeaponTableData(pm->ps->weapon)->isRiflenade)
		{
			if (pm->ps->ammoclip[GetWeaponTableData(pm->ps->weapon)->clipIndex])
			{
				return;
			}
		}
		else if (GetWeaponTableData(pm->ps->weapon)->isMortarSet)                   // the ammo from mortar are stored in the mortar (not mortar set)
		{
			if (pm->ps->ammo[GetWeaponTableData(pm->ps->weapon)->weapAlts])
			{
				return;
			}
		}
		else
		{
			if (pm->ps->ammoclip[GetWeaponTableData(pm->ps->weapon)->clipIndex])    // still got ammo in clip
			{
				return;
			}

			if (pm->ps->ammo[GetWeaponTableData(pm->ps->weapon)->ammoIndex])        // still got ammo in reserve
			{
				return;
			}
		}
	}

	// If this was the last one, remove the weapon and switch away before the player tries to fire next
	// NOTE: giving grenade ammo to a player will re-give him the weapon (if you do it through add_ammo())
	if (GetWeaponTableData(pm->ps->weapon)->isGrenade)
	{
		COM_BitClear(pm->ps->weapons, pm->ps->weapon);
	}
	else if (pm->ps->weapon == WP_SATCHEL)
	{
		pm->ps->ammoclip[WP_SATCHEL_DET] = 1;
		pm->ps->ammo[WP_SATCHEL_DET]     = 1;
		pm->ps->ammo[WP_SATCHEL]         = 0;
		pm->ps->ammoclip[WP_SATCHEL]     = 0;
		PM_BeginWeaponChange(WP_SATCHEL, WP_SATCHEL_DET, qfalse);
	}

	PM_AddEvent(EV_NOAMMO);
}

/**
 * @brief Accounts for clips being used/not used
 * @param[in] wp
 * @param[in] amount
 */
void PM_WeaponUseAmmo(weapon_t wp, int amount)
{
	if (pm->noWeapClips)
	{
		pm->ps->ammo[GetWeaponTableData(wp)->ammoIndex] -= amount;
	}
	else
	{
		int takeweapon;

		takeweapon = GetWeaponTableData(wp)->clipIndex;

		if (GetWeaponTableData(wp)->isAkimbo)
		{
			if (!BG_AkimboFireSequence(wp, pm->ps->ammoclip[GetWeaponTableData(wp)->clipIndex], pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(wp)->akimboSideArm)->clipIndex]))
			{
				takeweapon = GetWeaponTableData(wp)->akimboSideArm;
			}
		}

		pm->ps->ammoclip[takeweapon] -= amount;
	}
}

/**
 * @brief Accounts for clips being used/not used
 * @param[in] wp
 * @return
 */
int PM_WeaponAmmoAvailable(weapon_t wp)
{
	if (pm->noWeapClips)
	{
		return pm->ps->ammo[GetWeaponTableData(wp)->ammoIndex];
	}
	else
	{
		int takeweapon;

		takeweapon = GetWeaponTableData(wp)->clipIndex;

		if (GetWeaponTableData(wp)->isAkimbo)
		{
			if (!BG_AkimboFireSequence(wp, pm->ps->ammoclip[GetWeaponTableData(wp)->clipIndex], pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(wp)->akimboSideArm)->clipIndex]))
			{
				takeweapon = GetWeaponTableData(wp)->akimboSideArm;
			}
		}

		return pm->ps->ammoclip[takeweapon];
	}
}

/**
 * @brief Accounts for clips being used/not used
 * @param[in] wp
 * @return
 */
int PM_WeaponClipEmpty(weapon_t wp)
{
	if (pm->noWeapClips)
	{
		if (!(pm->ps->ammo[GetWeaponTableData(wp)->ammoIndex]))
		{
			return 1;
		}
	}
	else
	{
		if (!(pm->ps->ammoclip[GetWeaponTableData(wp)->clipIndex]))
		{
			return 1;
		}
	}

	return 0;
}

/**
 * @brief[in] PM_CoolWeapons
 */
void PM_CoolWeapons(void)
{
	int wp, maxHeat;

	// FIXME: non bullet weapons don't have to be cooled - this loop is a waste
	for (wp = WP_KNIFE; wp < WP_NUM_WEAPONS; wp++)
	{
		// if the weapon can heat and you have the weapon
		if (GetWeaponTableData(wp)->canHeat && COM_BitCheck(pm->ps->weapons, wp))
		{
			// and it's hot
			if (pm->ps->weapHeat[wp])
			{
				if (pm->skill[SK_HEAVY_WEAPONS] >= 2 && pm->ps->stats[STAT_PLAYER_CLASS] == PC_SOLDIER)
				{
					pm->ps->weapHeat[wp] -= ((float)GetWeaponTableData(wp)->coolRate * 2.f * pml.frametime);
				}
				else
				{
					pm->ps->weapHeat[wp] -= ((float)GetWeaponTableData(wp)->coolRate * pml.frametime);
				}

				if (pm->ps->weapHeat[wp] < 0)
				{
					pm->ps->weapHeat[wp] = 0;
				}
			}
		}
	}

	// a weapon is currently selected and can heat, convert current heat value to 0-255 range for client transmission
	// note: we are still cooling WP_NONE and other non bullet weapons
	if (pm->ps->weapon && GetWeaponTableData(pm->ps->weapon)->canHeat)
	{
		if (BG_PlayerMounted(pm->ps->eFlags))
		{
			// floor to prevent 8-bit wrap
			pm->ps->curWeapHeat = floor(((float)pm->ps->weapHeat[WP_DUMMY_MG42] / (float)GetWeaponTableData(WP_DUMMY_MG42)->maxHeat) * 255.0f);
		}
		else
		{
			// don't divide by 0
			maxHeat = GetWeaponTableData(pm->ps->weapon)->maxHeat;

			// floor to prevent 8-bit wrap
			if (maxHeat != 0)
			{
				pm->ps->curWeapHeat = floor(((float)pm->ps->weapHeat[pm->ps->weapon] / (float)maxHeat) * 255.0f);
			}
			else
			{
				pm->ps->curWeapHeat = 0;
			}
		}
		//if(pm->ps->weapHeat[pm->ps->weapon])
		//  Com_Printf("pm heat: %d, %d\n", pm->ps->weapHeat[pm->ps->weapon], pm->ps->curWeapHeat);
	}
	else
	{
		pm->ps->curWeapHeat = 0;
	}
}

#define AIMSPREAD_DECREASE_RATE     200.0f      // when I made the increase/decrease floats (so slower weapon recover could happen for scoped weaps) the average rate increased significantly
#define AIMSPREAD_INCREASE_RATE     800.0f
#define AIMSPREAD_VIEWRATE_MIN      30.0f       // degrees per second
#define AIMSPREAD_VIEWRATE_RANGE    120.0f      // degrees per second

/**
 * @brief PM_AdjustAimSpreadScale
 */
void PM_AdjustAimSpreadScale(void)
{
	int   i;
	float increase, decrease;       // was losing lots of precision on slower weapons (scoped)
	float cmdTime, wpnScale;

	// all weapons are very inaccurate in zoomed mode
	if (pm->ps->eFlags & EF_ZOOMING)
	{
		pm->ps->aimSpreadScale      = AIMSPREAD_MAXSPREAD;
		pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;
		return;
	}

	cmdTime = (pm->cmd.serverTime - pm->oldcmd.serverTime) / 1000.0f;

	wpnScale = GetWeaponTableData(pm->ps->weapon)->spreadScale;

	if (wpnScale != 0.f)
	{
		float viewchange = 0;

		if (GetWeaponTableData(pm->ps->weapon)->isScoped && pm->skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 3)
		{
			wpnScale *= 0.5;
		}

		// crouched players recover faster (mostly useful for snipers)
		if ((pm->ps->eFlags & EF_CROUCHING) || (pm->ps->eFlags & EF_PRONE))
		{
			wpnScale *= 0.5;
		}

		decrease = (cmdTime * AIMSPREAD_DECREASE_RATE) / wpnScale;

		// take player movement into account (even if only for the scoped weapons)
		// TODO: also check for jump/crouch and adjust accordingly
		if (GetWeaponTableData(pm->ps->weapon)->isScoped)
		{
			for (i = 0; i < 2; i++)
			{
				viewchange += Q_fabs(pm->ps->velocity[i]);
			}
		}
		else
		{
			// take player view rotation into account
			for (i = 0; i < 2; i++)
			{
				viewchange += Q_fabs(SHORT2ANGLE(pm->cmd.angles[i]) - SHORT2ANGLE(pm->oldcmd.angles[i]));
			}
		}

		viewchange  = viewchange / cmdTime;  // convert into this movement for a second
		viewchange -= AIMSPREAD_VIEWRATE_MIN / wpnScale;
		if (viewchange <= 0)
		{
			viewchange = 0;
		}
		else if (viewchange > (AIMSPREAD_VIEWRATE_RANGE / wpnScale))
		{
			viewchange = AIMSPREAD_VIEWRATE_RANGE / wpnScale;
		}

		// now give us a scale from 0.0 to 1.0 to apply the spread increase
		viewchange = viewchange / (AIMSPREAD_VIEWRATE_RANGE / wpnScale);

		increase = (int)(cmdTime * viewchange * AIMSPREAD_INCREASE_RATE);
	}
	else
	{
		increase = 0;
		decrease = AIMSPREAD_DECREASE_RATE;
	}


	if (PM_PRONEDELAY && pm->ps->aimSpreadScaleFloat == AIMSPREAD_MAXSPREAD && pm->cmd.serverTime - pm->pmext->proneTime < MAX_AIMSPREAD_TIME)
	{
		return;
	}

	// update the aimSpreadScale
	pm->ps->aimSpreadScaleFloat += (increase - decrease);
	if (pm->ps->aimSpreadScaleFloat < 0)
	{
		pm->ps->aimSpreadScaleFloat = 0;
	}
	if (pm->ps->aimSpreadScaleFloat > AIMSPREAD_MAXSPREAD)
	{
		pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;
	}

	pm->ps->aimSpreadScale = (int)pm->ps->aimSpreadScaleFloat;  // update the int for the client
}

#define weaponstateFiring (pm->ps->weaponstate == WEAPON_FIRING || pm->ps->weaponstate == WEAPON_FIRINGALT)

/**
 * @brief Special mounted mg42 handling
 * @return
 */
static qboolean PM_MountedFire(void)
{
	switch (pm->ps->persistant[PERS_HWEAPON_USE])
	{
	case 1:
		if (pm->ps->weapHeat[WP_DUMMY_MG42])
		{
			pm->ps->weapHeat[WP_DUMMY_MG42] -= (300.f * pml.frametime);

			if (pm->ps->weapHeat[WP_DUMMY_MG42] < 0)
			{
				pm->ps->weapHeat[WP_DUMMY_MG42] = 0;
			}

			// floor() to prevent 8-bit wrap
			pm->ps->curWeapHeat = floor(((float)pm->ps->weapHeat[WP_DUMMY_MG42] / (float)GetWeaponTableData(WP_DUMMY_MG42)->maxHeat) * 255.0f);
		}

		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
			if (pm->ps->weaponTime <= 0)
			{
				if (!(pm->cmd.buttons & BUTTON_ATTACK))
				{
					pm->ps->weaponTime = 0;
					return qtrue;
				}
			}
			else
			{
				return qtrue;
			}
		}

		if (pm->cmd.buttons & BUTTON_ATTACK)
		{
			pm->ps->weapHeat[WP_DUMMY_MG42] += GetWeaponTableData(WP_DUMMY_MG42)->nextShotTime;
			PM_AddEvent(EV_FIRE_WEAPON_MG42);
			pm->ps->weaponTime += GetWeaponTableData(WP_DUMMY_MG42)->nextShotTime;

			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue);
			pm->ps->viewlocked = VIEWLOCK_JITTER;         // this enable screen jitter when firing

			if (pm->ps->weapHeat[WP_DUMMY_MG42] >= GetWeaponTableData(WP_DUMMY_MG42)->maxHeat)
			{
				pm->ps->weapHeat[WP_DUMMY_MG42] = GetWeaponTableData(WP_DUMMY_MG42)->maxHeat;    // cap heat to max
				PM_AddEvent(EV_WEAP_OVERHEAT);
				pm->ps->weaponTime = 2000;          // force "heat recovery minimum" to 2 sec right now
			}
		}
		return qtrue;
	case 2:
		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
			if (pm->ps->weaponTime <= 0)
			{
				if (!(pm->cmd.buttons & BUTTON_ATTACK))
				{
					pm->ps->weaponTime = 0;
					return qtrue;
				}
			}
			else
			{
				return qtrue;
			}
		}

		if (pm->cmd.buttons & BUTTON_ATTACK)
		{
			PM_AddEvent(EV_FIRE_WEAPON_AAGUN);
			pm->ps->weaponTime += AAGUN_RATE_OF_FIRE;

			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue);
			//pm->ps->viewlocked = VIEWLOCK_JITTER;     // this enable screen jitter when firing
		}
		return qtrue;
	default:
		break;
	}

	if (pm->ps->eFlags & EF_MOUNTEDTANK)
	{
		if (pm->ps->weapHeat[WP_DUMMY_MG42])
		{
			pm->ps->weapHeat[WP_DUMMY_MG42] -= (300.f * pml.frametime);

			if (pm->ps->weapHeat[WP_DUMMY_MG42] < 0)
			{
				pm->ps->weapHeat[WP_DUMMY_MG42] = 0;
			}

			// floor() to prevent 8-bit wrap
			pm->ps->curWeapHeat = floor(((float)pm->ps->weapHeat[WP_DUMMY_MG42] / (float)GetWeaponTableData(WP_DUMMY_MG42)->maxHeat) * 255.0f);
		}

		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
			if (pm->ps->weaponTime <= 0)
			{
				if (!(pm->cmd.buttons & BUTTON_ATTACK))
				{
					pm->ps->weaponTime = 0;
					return qtrue;
				}
			}
			else
			{
				return qtrue;
			}
		}

		if (pm->cmd.buttons & BUTTON_ATTACK)
		{
			pm->ps->weapHeat[WP_DUMMY_MG42] += GetWeaponTableData(WP_DUMMY_MG42)->nextShotTime;
			PM_AddEvent(EV_FIRE_WEAPON_MOUNTEDMG42);
			pm->ps->weaponTime += GetWeaponTableData(WP_DUMMY_MG42)->nextShotTime;

			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue);
			//pm->ps->viewlocked = VIEWLOCK_JITTER;       // this enable screen jitter when firing

			if (pm->ps->weapHeat[WP_DUMMY_MG42] >= GetWeaponTableData(WP_DUMMY_MG42)->maxHeat)
			{
				pm->ps->weaponTime = GetWeaponTableData(WP_DUMMY_MG42)->maxHeat; // cap heat to max
				PM_AddEvent(EV_WEAP_OVERHEAT);
				pm->ps->weaponTime = 2000;      // force "heat recovery minimum" to 2 sec right now
			}

		}
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief PM_CheckGrenade
 * @return
 */
static qboolean PM_CheckGrenade()
{
	switch (pm->ps->weapon)
	{
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_SMOKE_BOMB:
		//case WP_LANDMINE:
		//case WP_KNIFE:
		//case WP_SATCHEL_DET:
		break;
	default:
		return qfalse;
	}

	if (pm->ps->grenadeTimeLeft > 0)
	{
		qboolean forcethrow = qfalse;

		if (pm->ps->weapon == WP_DYNAMITE)
		{
			pm->ps->grenadeTimeLeft += pml.msec;

			// in multiplayer, dynamite becomes strategic, so start timer @ 30 seconds
			if (pm->ps->grenadeTimeLeft < 5000)
			{
				pm->ps->grenadeTimeLeft = 5000;
			}
		}
		else
		{
			pm->ps->grenadeTimeLeft -= pml.msec;

			if (pm->ps->grenadeTimeLeft <= 100)         // give two frames advance notice so there's time to launch and detonate
			{
				forcethrow = qtrue;

				pm->ps->grenadeTimeLeft = 100;
			}
		}

		if (!(pm->cmd.buttons & BUTTON_ATTACK) || forcethrow || (pm->ps->eFlags & EF_PRONE_MOVING))
		{
			if (pm->ps->weaponDelay == GetWeaponTableData(pm->ps->weapon)->fireDelayTime || forcethrow)
			{
				// released fire button.  Fire!!!
				if (pm->ps->eFlags & EF_PRONE)
				{
					BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qtrue);
				}
				else
				{
					BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue);
				}
			}
		}
		else
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief PM_HandleRecoil
 */
static void PM_HandleRecoil(void)
{
	if (pm->pmext->weapRecoilTime)
	{
		vec3_t muzzlebounce;
		int    i;
		int    cmdAngle;
		int    deltaTime = pm->cmd.serverTime - pm->pmext->weapRecoilTime;

		VectorCopy(pm->ps->viewangles, muzzlebounce);

		if (deltaTime > pm->pmext->weapRecoilDuration)
		{
			deltaTime = pm->pmext->weapRecoilDuration;
		}

		for (i = pm->pmext->lastRecoilDeltaTime; i < deltaTime; i += 15)
		{
			if (pm->pmext->weapRecoilPitch > 0.f)
			{
				muzzlebounce[PITCH] -= 2 * pm->pmext->weapRecoilPitch * cos(2.5 * (i) / pm->pmext->weapRecoilDuration);
				muzzlebounce[PITCH] -= 0.25f * random() * (1.0f - (i) / pm->pmext->weapRecoilDuration);
			}

			if (pm->pmext->weapRecoilYaw > 0.f)
			{
				muzzlebounce[YAW] += 0.5 * pm->pmext->weapRecoilYaw * cos(1.0 - (i) * 3 / pm->pmext->weapRecoilDuration);
				muzzlebounce[YAW] += 0.5f * crandom() * (1.0f - (i) / pm->pmext->weapRecoilDuration);
			}
		}

		// set the delta angle
		for (i = 0; i < 3; i++)
		{
			cmdAngle                = ANGLE2SHORT(muzzlebounce[i]);
			pm->ps->delta_angles[i] = cmdAngle - pm->cmd.angles[i];
		}
		VectorCopy(muzzlebounce, pm->ps->viewangles);

		if (deltaTime == pm->pmext->weapRecoilDuration)
		{
			pm->pmext->weapRecoilTime      = 0;
			pm->pmext->lastRecoilDeltaTime = 0;
		}
		else
		{
			pm->pmext->lastRecoilDeltaTime = deltaTime;
		}
	}
}

//#define DO_WEAPON_DBG 1

/**
 * @brief Generates weapon events and modifes the weapon counter
 */
static void PM_Weapon(void)
{
	int      addTime           = GetWeaponTableData(pm->ps->weapon)->nextShotTime;
	int      aimSpreadScaleAdd = GetWeaponTableData(pm->ps->weapon)->aimSpreadScaleAdd;
	int      ammoNeeded;
	qboolean delayedFire;       // true if the delay time has just expired and this is the frame to send the fire event
	int      weapattackanim;
	qboolean akimboFire;
#ifdef DO_WEAPON_DBG
	static int weaponstate_last = -1;
#endif

	// don't allow attack until all buttons are up
	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return;
	}

	// ignore if spectator
	if (pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	// check for dead player
	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		if (!(pm->ps->pm_flags & PMF_LIMBO))
		{
			// weapon cool down
			PM_CoolWeapons();
		}

		return;
	}

	// weapon cool down
	PM_CoolWeapons();

	// mounted mg42 handling
	if (PM_MountedFire())
	{
		return;
	}

	// commented - why do we do this in weapon code? This messes up water content type
	// only PM_SetWaterLevel and init functions should deal with this
	// if we have issues with water we know why ....
	//pm->watertype = 0;

	if (GetWeaponTableData(pm->ps->weapon)->isAkimbo)
	{
		akimboFire = BG_AkimboFireSequence(pm->ps->weapon, pm->ps->ammoclip[GetWeaponTableData(pm->ps->weapon)->clipIndex], pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(pm->ps->weapon)->akimboSideArm)->clipIndex]);
	}
	else
	{
		akimboFire = qfalse;
	}

#ifdef DO_WEAPON_DBG
	if (pm->ps->weaponstate != weaponstate_last)
	{
#ifdef CGAMEDLL
		Com_Printf(" CGAMEDLL\n");
#else
		Com_Printf("!CGAMEDLL\n");
#endif
		switch (pm->ps->weaponstate)
		{
		case WEAPON_READY:
			Com_Printf(" -- WEAPON_READY\n");
			break;
		case WEAPON_RAISING:
			Com_Printf(" -- WEAPON_RAISING\n");
			break;
		case WEAPON_RAISING_TORELOAD:
			Com_Printf(" -- WEAPON_RAISING_TORELOAD\n");
			break;
		case WEAPON_DROPPING:
			Com_Printf(" -- WEAPON_DROPPING\n");
			break;
		case WEAPON_READYING:
			Com_Printf(" -- WEAPON_READYING\n");
			break;
		case WEAPON_RELAXING:
			Com_Printf(" -- WEAPON_RELAXING\n");
			break;
		case WEAPON_DROPPING_TORELOAD:
			Com_Printf(" -- WEAPON_DROPPING_TORELOAD\n");
			break;
		case WEAPON_FIRING:
			Com_Printf(" -- WEAPON_FIRING\n");
			break;
		case WEAPON_FIRINGALT:
			Com_Printf(" -- WEAPON_FIRINGALT\n");
			break;
		case WEAPON_RELOADING:
			Com_Printf(" -- WEAPON_RELOADING\n");
			break;
		}
		weaponstate_last = pm->ps->weaponstate;
	}
#endif

	// check for weapon recoil
	// do the recoil before setting the values, that way it will be shown next frame and not this
	PM_HandleRecoil();

	delayedFire = qfalse;

	if (PM_CheckGrenade())
	{
		return;
	}

	if (pm->ps->weaponDelay > 0)
	{
		pm->ps->weaponDelay -= pml.msec;

		if (pm->ps->weaponDelay <= 0)
		{
			pm->ps->weaponDelay = 0;
			delayedFire         = qtrue; // weapon delay has expired.  Fire this frame
		}
	}

	if (pm->ps->weaponstate == WEAPON_RELAXING)
	{
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		pm->ps->weaponTime -= pml.msec;
		if (!(pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->weaponTime < 0)
		{
			pm->ps->weaponTime = 0;
		}

		// aha, THIS is the kewl quick fire mode :)
		// added back for multiplayer pistol balancing
		if (GetWeaponTableData(pm->ps->weapon)->quickFireMode)
		{
			// moved releasedFire into pmext instead of ps
			if (pm->pmext->releasedFire)
			{
				if (pm->cmd.buttons & BUTTON_ATTACK)
				{
					// akimbo weapons only have a 200ms delay, so
					// use a shorter time for quickfire (#255)
					if (GetWeaponTableData(pm->ps->weapon)->isAkimbo)
					{
						if (pm->ps->weaponTime <= 50)
						{
							pm->ps->weaponTime = 0;
						}
					}
					else
					{
						if (pm->ps->weaponTime <= 150)
						{
							pm->ps->weaponTime = 0;
						}
					}
				}
			}
			else if (!(pm->cmd.buttons & BUTTON_ATTACK))
			{
				// moved releasedFire into pmext instead of ps
				pm->pmext->releasedFire = qtrue;
			}
		}
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ((pm->ps->weaponTime <= 0 || (!weaponstateFiring && pm->ps->weaponDelay <= 0)) && !delayedFire)
	{
		if (pm->ps->weapon != pm->cmd.weapon)
		{
			PM_BeginWeaponChange(pm->ps->weapon, pm->cmd.weapon, qfalse);
		}
	}

	// waiting for attack animation to complete (eg. Mortar)
	if (pm->ps->weaponDelay > 0)
	{
		return;
	}

	// check for clip change
	PM_CheckForReload(pm->ps->weapon);

	// check if weapon is busy
	if (pm->ps->weaponTime > 0 || pm->ps->weaponDelay > 0)
	{
		return;
	}

	switch (pm->ps->weaponstate)
	{
	case WEAPON_RELOADING:
		PM_FinishWeaponReload();
		break;
	case WEAPON_DROPPING:
	case WEAPON_DROPPING_TORELOAD:
		PM_FinishWeaponChange();
		return;
	case WEAPON_RAISING:
		pm->ps->weaponstate = WEAPON_READY;
		PM_StartWeaponAnim(GetWeaponTableData(pm->ps->weapon)->idleAnim);
		return;
	case WEAPON_RAISING_TORELOAD:
		pm->ps->weaponstate = WEAPON_READY;
		PM_BeginWeaponReload(pm->ps->weapon);
		return;
	default:
		break;
	}

	// can't shoot while prone and moving
	if ((pm->ps->eFlags & EF_PRONE_MOVING) && !delayedFire)
	{
		PM_ContinueWeaponAnim(GetWeaponTableData(pm->ps->weapon)->idleAnim);
		return;
	}

	// this is possible since the player starts with nothing
	if (pm->ps->weapon == WP_NONE)
	{
		return;
	}

	if (GetWeaponTableData(pm->ps->weapon)->isPanzer)
	{
		if (pm->ps->eFlags & EF_PRONE)
		{
			return;
		}
	}

	// don't allow some weapons to fire if charge bar isn't full
	if (GetWeaponTableData(pm->ps->weapon)->useChargeTime)
	{
		skillType_t skill = GetWeaponTableData(pm->ps->weapon)->skillBased;
		float       coeff = GetWeaponTableData(pm->ps->weapon)->chargeTimeCoeff[pm->skill[skill]];
		int         chargeTime;

		switch (skill)
		{
		case SK_EXPLOSIVES_AND_CONSTRUCTION:              chargeTime = pm->engineerChargeTime;  break;
		case SK_FIRST_AID:                                chargeTime = pm->medicChargeTime;     break;
		case SK_SIGNALS:                                  chargeTime = pm->ltChargeTime;        break;
		case SK_HEAVY_WEAPONS:                            chargeTime = pm->soldierChargeTime;   break;
		case SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS: chargeTime = pm->covertopsChargeTime; break;
		case SK_BATTLE_SENSE:
		case SK_LIGHT_WEAPONS:
		case SK_NUM_SKILLS:
		default:                                          chargeTime = -1; break;
		}

		if (chargeTime != -1)
		{
			if (pm->cmd.serverTime - pm->ps->classWeaponTime < chargeTime * coeff)
			{
				if ((pm->ps->weapon == WP_MEDKIT || pm->ps->weapon == WP_AMMO) && pm->cmd.buttons & BUTTON_ATTACK)
				{
					BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_NOPOWER, qtrue, qfalse);
				}

				return;
			}
		}
	}

	if (GetWeaponTableData(pm->ps->weapon)->isMortarSet && !delayedFire)
	{
		pm->ps->weaponstate = WEAPON_READY;
	}

	// check for fire
	// if not on fire button and there's not a delayed shot this frame...
	// consider also leaning, with delayed attack reset
	if ((!(pm->cmd.buttons & BUTTON_ATTACK) && !(pm->cmd.wbuttons & WBUTTON_ATTACK2) && !delayedFire) ||
	    (pm->ps->leanf != 0.f && !GetWeaponTableData(pm->ps->weapon)->isGrenade && pm->ps->weapon != WP_SMOKE_BOMB))
	{
		pm->ps->weaponTime  = 0;
		pm->ps->weaponDelay = 0;

		if (weaponstateFiring)     // you were just firing, time to relax
		{
			PM_ContinueWeaponAnim(GetWeaponTableData(pm->ps->weapon)->idleAnim);
		}

		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// a not mounted mortar can't fire
	if (GetWeaponTableData(pm->ps->weapon)->isMortar)
	{
		return;
	}

	// player is zooming - no fire
	// PC_FIELDOPS needs to zoom to call artillery
	if (pm->ps->eFlags & EF_ZOOMING)
	{
#ifdef GAMEDLL
		if (pm->ps->stats[STAT_PLAYER_CLASS] == PC_FIELDOPS)
		{
			pm->ps->weaponTime += 500;
			PM_AddEvent(EV_FIRE_WEAPON);
		}
#endif
		return;
	}

	// player is underwater and weapon can't fire under water
	if (pm->waterlevel == 3 && GetWeaponTableData(pm->ps->weapon)->isUnderWaterFire == qfalse)
	{
		PM_AddEvent(EV_NOFIRE_UNDERWATER);      // event for underwater 'click' for nofire
		PM_ContinueWeaponAnim(GetWeaponTableData(pm->ps->weapon)->idleAnim);
		pm->ps->weaponTime  = 500;
		pm->ps->weaponDelay = 0;                // avoid insta-fire after water exit on delayed weapon attacks
		return;
	}

	// start the animation even if out of ammo
	if (GetWeaponTableData(pm->ps->weapon)->isMeleeWeapon || pm->ps->weapon == WP_LANDMINE)
	{
		if (!delayedFire)
		{
			qboolean isLandmine = pm->ps->weapon == WP_LANDMINE;

			if (pm->ps->eFlags & EF_PRONE)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, isLandmine);
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, isLandmine);
			}

			if (isLandmine)
			{
				pm->ps->weaponDelay = GetWeaponTableData(pm->ps->weapon)->fireDelayTime;
			}
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->isGrenade || pm->ps->weapon == WP_DYNAMITE || pm->ps->weapon == WP_SMOKE_BOMB || pm->ps->weapon == WP_SATCHEL)
	{
		if (!delayedFire)
		{
			if (PM_WeaponAmmoAvailable(pm->ps->weapon))
			{
				if (pm->ps->weapon == WP_DYNAMITE)
				{
					pm->ps->grenadeTimeLeft = 50;
				}
				else if (pm->ps->weapon != WP_SATCHEL)      // satchel don't have counter
				{
					pm->ps->grenadeTimeLeft = 4000;         // start at four seconds and count down
				}

				PM_StartWeaponAnim(GetWeaponTableData(pm->ps->weapon)->attackAnim);
			}

			pm->ps->weaponDelay = GetWeaponTableData(pm->ps->weapon)->fireDelayTime;
		}
	}
	else
	{
		if (!weaponstateFiring)
		{
			// pfaust has spinup time in MP
			if (GetWeaponTableData(pm->ps->weapon)->isPanzer)
			{
				PM_AddEvent(EV_SPINUP);
			}
			else if (GetWeaponTableData(pm->ps->weapon)->isMortarSet)
			{
				PM_AddEvent(EV_SPINUP);
				PM_StartWeaponAnim(GetWeaponTableData(pm->ps->weapon)->attackAnim);     // FIXME: returns WEAP_ATTACK1 anyway
			}
			else if (pm->ps->weapon == WP_SATCHEL_DET)
			{
				PM_AddEvent(EV_SPINUP);
				PM_ContinueWeaponAnim(GetWeaponTableData(WP_SATCHEL_DET)->attackAnim);
			}

			pm->ps->weaponDelay = GetWeaponTableData(pm->ps->weapon)->fireDelayTime;
		}
		else if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, akimboFire ? ANIM_ET_FIREWEAPON2PRONE : ANIM_ET_FIREWEAPONPRONE,
			                   GetWeaponTableData(pm->ps->weapon)->firingAuto, qtrue);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, akimboFire ? ANIM_ET_FIREWEAPON2 : ANIM_ET_FIREWEAPON,
			                   GetWeaponTableData(pm->ps->weapon)->firingAuto, qtrue);
		}
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	ammoNeeded = GetWeaponTableData(pm->ps->weapon)->uses;

	if (pm->ps->weapon)
	{
		int ammoAvailable;

		ammoAvailable = PM_WeaponAmmoAvailable(pm->ps->weapon);

		if (ammoNeeded > ammoAvailable)
		{
			// you have ammo for this, just not in the clip
			qboolean reloading = (qboolean)(ammoNeeded <= pm->ps->ammo[GetWeaponTableData(pm->ps->weapon)->ammoIndex]);

			// if not in auto-reload mode, and reload was not explicitely requested, just play the 'out of ammo' sound
			// scoped weapons not allowed to reload.  must switch back to primary first
			if ((!pm->pmext->bAutoReload && GetWeaponTableData(pm->ps->weapon)->isAutoReload && !(pm->cmd.wbuttons & WBUTTON_RELOAD))
			    || GetWeaponTableData(pm->ps->weapon)->isScoped)
			{
				reloading = qfalse;
			}

			// only play the switch sound if using a triggered weapon
			if (!GetWeaponTableData(pm->ps->weapon)->isGrenade && pm->ps->weapon != WP_LANDMINE)
			{
				if (!reloading)
				{
					PM_AddEvent(EV_NOAMMO);
				}
			}

			if (reloading)
			{
				PM_ContinueWeaponAnim(PM_ReloadAnimForWeapon(pm->ps->weapon));
			}
			else
			{
				PM_ContinueWeaponAnim(GetWeaponTableData(pm->ps->weapon)->idleAnim);
				pm->ps->weaponTime += 500;
			}

			return;
		}
	}

	if (pm->ps->weaponDelay > 0)
	{
		// if it hits here, the 'fire' has just been hit and the weapon dictated a delay.
		// animations have been started, weaponstate has been set, but no weapon events yet. (except possibly EV_NOAMMO)
		// checks for delayed weapons that have already been fired are return'ed above.
		return;
	}

	if (!(pm->ps->eFlags & EF_PRONE) && (pml.groundTrace.surfaceFlags & SURF_SLICK))
	{
		if (GetWeaponTableData(pm->ps->weapon)->knockback > 0.f)
		{
			// Add some knockback on slick
			vec3_t kvel;

			VectorScale(pml.forward, -1.f * (GetWeaponTableData(pm->ps->weapon)->knockback / 200), kvel);        // -1 as we get knocked backwards
			VectorAdd(pm->ps->velocity, kvel, pm->ps->velocity);

			if (!pm->ps->pm_time)
			{
				pm->ps->pm_time   = 100;
				pm->ps->pm_flags |= PMF_TIME_KNOCKBACK;
			}
		}
	}

	// take an ammo away if not infinite
	if (PM_WeaponAmmoAvailable(pm->ps->weapon) != -1)
	{
		// check for being mounted
		if (!BG_PlayerMounted(pm->ps->eFlags))
		{
			PM_WeaponUseAmmo(pm->ps->weapon, ammoNeeded);
		}
	}

	// fire weapon

	// add weapon heat
	if (GetWeaponTableData(pm->ps->weapon)->maxHeat)
	{
		pm->ps->weapHeat[pm->ps->weapon] += GetWeaponTableData(pm->ps->weapon)->nextShotTime;
	}

	// first person weapon animations

	// if this was the last round in the clip, play the 'lastshot' animation
	// this animation has the weapon in a "ready to reload" state
	if (GetWeaponTableData(pm->ps->weapon)->isAkimbo)
	{
		if (akimboFire)
		{
			weapattackanim = WEAP_ATTACK1;
		}
		else
		{
			weapattackanim = WEAP_ATTACK2;
		}
	}
	else
	{
		if (PM_WeaponClipEmpty(pm->ps->weapon))
		{
			weapattackanim = GetWeaponTableData(pm->ps->weapon)->lastAttackAnim;
		}
		else
		{
			weapattackanim = GetWeaponTableData(pm->ps->weapon)->attackAnim;
		}
	}

	// weapon firing animation
	if (GetWeaponTableData(pm->ps->weapon)->firingAuto)
	{
		PM_ContinueWeaponAnim(weapattackanim);
	}
	else if (!GetWeaponTableData(pm->ps->weapon)->isMortarSet)  // no animation for mortar set
	{
		PM_StartWeaponAnim(weapattackanim);
	}

	if (GetWeaponTableData(pm->ps->weapon)->isAkimbo)
	{
		if (akimboFire)
		{
			PM_AddEvent(EV_FIRE_WEAPON);
		}
		else
		{
			PM_AddEvent(EV_FIRE_WEAPONB);
		}
	}
	else
	{
		if (PM_WeaponClipEmpty(pm->ps->weapon))
		{
			PM_AddEvent(EV_FIRE_WEAPON_LASTSHOT);
		}
		else
		{
			PM_AddEvent(EV_FIRE_WEAPON);
		}
	}

	// moved releasedFire into pmext instead of ps
	pm->pmext->releasedFire = qfalse;
	pm->ps->lastFireTime    = pm->cmd.serverTime;

	// set weapon recoil (kickback)
	pm->pmext->lastRecoilDeltaTime = 0;
	pm->pmext->weapRecoilTime      = GetWeaponTableData(pm->ps->weapon)->weapRecoilDuration ? pm->cmd.serverTime : 0;
	pm->pmext->weapRecoilDuration  = GetWeaponTableData(pm->ps->weapon)->weapRecoilDuration;
	pm->pmext->weapRecoilYaw       = GetWeaponTableData(pm->ps->weapon)->weapRecoilYaw[0] * crandom() * GetWeaponTableData(pm->ps->weapon)->weapRecoilYaw[1];
	pm->pmext->weapRecoilPitch     = GetWeaponTableData(pm->ps->weapon)->weapRecoilPitch[0] * random() * GetWeaponTableData(pm->ps->weapon)->weapRecoilPitch[1];

	// handle case depending of player skill and position for weapon recoil
	if (GetWeaponTableData(pm->ps->weapon)->isScoped)
	{
		if (pm->ps->weapon == WP_FG42SCOPE)
		{
			if (pm->skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 3)
			{
				pm->pmext->weapRecoilPitch *= .5f;
			}
		}
		else
		{
			if (pm->skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 3)
			{
				pm->pmext->weapRecoilPitch = .25f;
			}
			else
			{
				pm->pmext->weapRecoilPitch = .5f;
			}
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->isMG)
	{
		if ((pm->ps->pm_flags & PMF_DUCKED) || (pm->ps->eFlags & EF_PRONE))
		{
			pm->pmext->weapRecoilYaw   = crandom() * .5f;
			pm->pmext->weapRecoilPitch = .45f * random() * .15f;
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->isPistol || GetWeaponTableData(pm->ps->weapon)->isSilencedPistol || GetWeaponTableData(pm->ps->weapon)->isAkimbo)
	{
		if (pm->skill[SK_LIGHT_WEAPONS] >= 3)
		{
			pm->pmext->weapRecoilDuration = 70;
			pm->pmext->weapRecoilPitch    = .25f * random() * .15f;
		}
	}

	// Aim Spread Scale handle
	// add randomness
	if (pm->ps->weapon == WP_MP40 || pm->ps->weapon == WP_THOMPSON || pm->ps->weapon == WP_STEN)
	{
		aimSpreadScaleAdd += rand() % 10;
	}

	// add the recoil amount to the aimSpreadScale
	pm->ps->aimSpreadScaleFloat += 3.0 * aimSpreadScaleAdd;

	if (pm->ps->aimSpreadScaleFloat > AIMSPREAD_MAXSPREAD)
	{
		pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;
	}

	if (pm->skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 3 && pm->ps->stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
	{
		pm->ps->aimSpreadScaleFloat *= .5f;
	}

	pm->ps->aimSpreadScale = (int)(pm->ps->aimSpreadScaleFloat);

	if (GetWeaponTableData(pm->ps->weapon)->isMG || GetWeaponTableData(pm->ps->weapon)->isMGSet)
	{
		// sync heat for overheat check
		pm->ps->weapHeat[GetWeaponTableData(pm->ps->weapon)->weapAlts] = pm->ps->weapHeat[pm->ps->weapon];

		if (weapattackanim == WEAP_ATTACK_LASTSHOT)
		{
			addTime = 0;
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->isAkimbo)
	{
		// if you're firing an akimbo weapon, and your other gun is dry,
		// nextshot needs to take 2x time

		// fixed the swapped usage of akimboFire vs. the colt
		// so that the last shot isn't delayed
		if (!pm->ps->ammoclip[GetWeaponTableData(pm->ps->weapon)->clipIndex])
		{
			if (!akimboFire)
			{
				addTime *= 2;
			}
		}
		else if (!pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(pm->ps->weapon)->akimboSideArm)->clipIndex])
		{
			if (akimboFire)
			{
				addTime *= 2;
			}
		}
	}

	// check for overheat
	// the weapon can overheat, and it is overheating
	if (GetWeaponTableData(pm->ps->weapon)->maxHeat && pm->ps->weapHeat[pm->ps->weapon] >= GetWeaponTableData(pm->ps->weapon)->maxHeat)
	{
		pm->ps->weapHeat[pm->ps->weapon] = GetWeaponTableData(pm->ps->weapon)->maxHeat;         // cap heat to max
		PM_AddEvent(EV_WEAP_OVERHEAT);
		//PM_StartWeaponAnim(WEAP_IDLE1); // removed.  client handles anim in overheat event
		addTime = 2000;         // force "heat recovery minimum" to 2 sec right now
	}

	pm->ps->weaponTime += addTime;

	PM_SwitchIfEmpty();
}

/**
 * @brief PM_DropTimers
 */
static void PM_DropTimers(void)
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

	// first person weapon counter
	if (pm->pmext->weapAnimTimer > 0)
	{
		pm->pmext->weapAnimTimer -= pml.msec;
		if (pm->pmext->weapAnimTimer < 0)
		{
			pm->pmext->weapAnimTimer = 0;
		}
	}
}

#define LEAN_MAX        28.0f
#define LEAN_TIME_TO    200.0f  // time to get to/from full lean
#define LEAN_TIME_FR    300.0f  // time to get to/from full lean

/**
 * @brief PM_UpdateLean
 * @param[in,out] ps
 * @param[in,out] cmd
 * @param[in] tpm
 */
void PM_UpdateLean(playerState_t *ps, usercmd_t *cmd, pmove_t *tpm)
{
	vec3_t start;
	int    leaning = 0;         // -1 left, 1 right
	float  leanofs = 0;

	if (cmd->wbuttons & (WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT))
	{
		// allow spectators to lean while moving
		if (ps->pm_type == PM_SPECTATOR)
		{
			if (cmd->wbuttons & WBUTTON_LEANLEFT)
			{
				leaning -= 1;
			}
			if (cmd->wbuttons & WBUTTON_LEANRIGHT)
			{
				leaning += 1;
			}
		}
		else if ((!cmd->forwardmove && cmd->upmove <= 0))
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
	}

	if (BG_PlayerMounted(ps->eFlags))
	{
		leaning = 0;    // leaning not allowed on mg42
	}

	if (ps->eFlags & EF_FIRING)
	{
		leaning = 0;    // not allowed to lean while firing

	}

	if (ps->eFlags & EF_DEAD)
	{
		leaning = 0;    // not allowed to lean while firing

	}

	// ATVI Wolfenstein Misc #479 - initial fix to #270 would crash in g_synchronousClients 1 situation
	if (ps->weaponstate == WEAPON_FIRING && ps->weapon == WP_DYNAMITE)
	{
		leaning = 0; // not allowed while tossing dynamite

	}

	if ((ps->eFlags & EF_PRONE) || GetWeaponTableData(ps->weapon)->isMortarSet)
	{
		leaning = 0;    // not allowed to lean while prone

	}
	leanofs = ps->leanf;


	if (!leaning)      // go back to center position
	{
		if (leanofs > 0)            // right
		{
			leanofs -= (((float)pml.msec / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs < 0)
			{
				leanofs = 0;
			}
		}
		else if (leanofs < 0)         // left
		{
			leanofs += (((float)pml.msec / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs > 0)
			{
				leanofs = 0;
			}
		}
	}

	if (leaning)
	{
		if (leaning > 0)       // right
		{
			if (leanofs < LEAN_MAX)
			{
				leanofs += (((float)pml.msec / (float)LEAN_TIME_TO) * LEAN_MAX);
			}

			if (leanofs > LEAN_MAX)
			{
				leanofs = LEAN_MAX;
			}
			ps->stats[STAT_PS_FLAGS] |= STAT_LEAN_RIGHT;
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
			ps->stats[STAT_PS_FLAGS] |= STAT_LEAN_LEFT;
		}
	}
	else
	{
		ps->stats[STAT_PS_FLAGS] &= ~(STAT_LEAN_LEFT | STAT_LEAN_RIGHT);
	}
	ps->leanf = leanofs;

	if (leaning)
	{
		vec3_t  tmins, tmaxs, right, end;
		vec3_t  viewangles;
		trace_t trace;

		VectorCopy(ps->origin, start);
		start[2] += ps->viewheight;

		VectorCopy(ps->viewangles, viewangles);
		viewangles[ROLL] += leanofs / 2.0f;
		AngleVectors(viewangles, NULL, right, NULL);
		VectorMA(start, leanofs, right, end);

		VectorSet(tmins, -8, -8, -7);   // ATVI Wolfenstein Misc #472, bumped from -4 to cover gun clipping issue
		VectorSet(tmaxs, 8, 8, 4);

		if (pm)
		{
			pm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
		}
		else
		{
			tpm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
		}

		ps->leanf *= trace.fraction;
	}

	// Allow Spectators to lean while moving
	if (ps->leanf != 0.f && ps->pm_type != PM_SPECTATOR)
	{
		cmd->rightmove = 0;     // also disallowed in cl_input ~391
	}
}

/**
 * @brief This can be used as another entry point when only the viewangles
 * are being updated isntead of a full move
 *
 * @param[in,out] ps
 * @param[in,out] pmext
 * @param[in] cmd
 * @param[in] tracemask Take a tracemask as well - we can't use anything out of pm
 *
 * @note Any changes to mounted/prone view should be duplicated in BotEntityWithinView()
 *
 * @note Tnused trace parameter
 */
void PM_UpdateViewAngles(playerState_t *ps, pmoveExt_t *pmext, usercmd_t *cmd, void(trace) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int tracemask)            //   modified
{
	short  temp;
	int    i;
	vec3_t oldViewAngles;

	// Added support for PMF_TIME_LOCKPLAYER
	if (ps->pm_type == PM_INTERMISSION || (ps->pm_flags & PMF_TIME_LOCKPLAYER))
	{
		// reset all angle changes, so it does not suddenly happens after unlocked
		ps->delta_angles[0] = ANGLE2SHORT(ps->viewangles[0]) - cmd->angles[0];
		ps->delta_angles[1] = ANGLE2SHORT(ps->viewangles[1]) - cmd->angles[1];
		ps->delta_angles[2] = ANGLE2SHORT(ps->viewangles[2]) - cmd->angles[2];
		return;     // no view changes at all
	}

	if (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0)
	{
		// Allow players to look around while 'wounded' or lock to a medic if nearby
		temp = cmd->angles[1] + ps->delta_angles[1];
		// always allow this.  viewlocking will take precedence
		// if a medic is found
		// using a full short and converting on the client so that
		// we get >1 degree resolution
		ps->stats[STAT_DEAD_YAW] = temp;
		return;     // no view changes at all
	}

	VectorCopy(ps->viewangles, oldViewAngles);

	// circularly clamp the angles with deltas
	// - game-side delta_angles modifications are broken here if you exclude the ROLL calculation
	for (i = 0 ; i < 3 ; i++)
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

	if (BG_PlayerMounted(ps->eFlags))
	{
		float arcMin, arcMax, arcDiff;
		float yaw    = ps->viewangles[YAW];
		float oldYaw = oldViewAngles[YAW];

		if (yaw - oldYaw > 180)
		{
			yaw -= 360;
		}
		if (yaw - oldYaw < -180)
		{
			yaw += 360;
		}

		if (yaw > oldYaw)
		{
			if (yaw - oldYaw > MG42_YAWSPEED * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw + MG42_YAWSPEED * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}
		else if (oldYaw > yaw)
		{
			if (oldYaw - yaw > MG42_YAWSPEED * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw - MG42_YAWSPEED * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}

		// limit harc and varc

		// pitch (varc)
		arcMax = pmext->varc;
		if (ps->eFlags & EF_AAGUN_ACTIVE)
		{
			arcMin = 0;
		}
		else if (ps->eFlags & EF_MOUNTEDTANK)
		{
			float angle;

			arcMin = 14;
			arcMax = 50;

			angle = cos(DEG2RAD(AngleNormalize180(pmext->centerangles[1] - ps->viewangles[1])));
			angle = -AngleNormalize360(angle * AngleNormalize180(0 - pmext->centerangles[0]));

			pmext->centerangles[PITCH] = angle;
		}
		else
		{
			arcMin = pmext->varc / 2;
		}

		arcDiff = AngleNormalize180(ps->viewangles[PITCH] - pmext->centerangles[PITCH]);

		if (arcDiff > arcMin)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->centerangles[PITCH] + arcMin);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
		else if (arcDiff < -arcMax)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->centerangles[PITCH] - arcMax);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}

		if (!(ps->eFlags & EF_MOUNTEDTANK))
		{
			// yaw (harc)
			arcMin  = arcMax = pmext->harc;
			arcDiff = AngleNormalize180(ps->viewangles[YAW] - pmext->centerangles[YAW]);

			if (arcDiff > arcMin)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->centerangles[YAW] + arcMin);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
			else if (arcDiff < -arcMax)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->centerangles[YAW] - arcMax);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}
	}
	else if (GetWeaponTableData(ps->weapon)->isMortarSet)
	{
		float degsSec = 60.f;
		float pitch, oldPitch;
		float pitchMax = 30.f;
		float yawDiff, pitchDiff;
		float yaw    = ps->viewangles[YAW];
		float oldYaw = oldViewAngles[YAW];

		if (yaw - oldYaw > 180)
		{
			yaw -= 360;
		}
		if (yaw - oldYaw < -180)
		{
			yaw += 360;
		}

		if (yaw > oldYaw)
		{
			if (yaw - oldYaw > degsSec * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw + degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}
		else if (oldYaw > yaw)
		{
			if (oldYaw - yaw > degsSec * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw - degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}

		pitch    = ps->viewangles[PITCH];
		oldPitch = oldViewAngles[PITCH];

		if (pitch - oldPitch > 180)
		{
			pitch -= 360;
		}
		if (pitch - oldPitch < -180)
		{
			pitch += 360;
		}

		if (pitch > oldPitch)
		{
			if (pitch - oldPitch > degsSec * pml.frametime)
			{
				ps->viewangles[PITCH] = oldPitch + degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
			}
		}
		else if (oldPitch > pitch)
		{
			if (oldPitch - pitch > degsSec * pml.frametime)
			{
				ps->viewangles[PITCH] = oldPitch - degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
			}
		}

		// yaw
		yawDiff = ps->viewangles[YAW] - pmext->mountedWeaponAngles[YAW];

		if (yawDiff > 180)
		{
			yawDiff -= 360;
		}
		else if (yawDiff < -180)
		{
			yawDiff += 360;
		}

		if (yawDiff > 30)
		{
			ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] + 30.f);

			// Set delta_angles properly
			ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
		}
		else if (yawDiff < -30)
		{
			ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] - 30.f);

			// Set delta_angles properly
			ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
		}

		// pitch
		pitchDiff = ps->viewangles[PITCH] - pmext->mountedWeaponAngles[PITCH];

		if (pitchDiff > 180)
		{
			pitchDiff -= 360;
		}
		else if (pitchDiff < -180)
		{
			pitchDiff += 360;
		}

		if (pitchDiff > (pitchMax - 10.f))
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] + (pitchMax - 10.f));

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
		else if (pitchDiff < -(pitchMax))
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] - (pitchMax));

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
	}
	else if (ps->eFlags & EF_PRONE)
	{
		//float degsSec = 60.f;
		//float oldYaw;
		trace_t traceres;         // renamed
		int     newDeltaAngle = ps->delta_angles[YAW];
		float   pitchMax      = 40.f;
		float   pitchDiff;
		float   oldYaw = oldViewAngles[YAW];
		//yaw = ps->viewangles[YAW];


		/*if ( yaw - oldYaw > 180 ) {
		    yaw -= 360;
		}
		if ( yaw - oldYaw < -180 ) {
		    yaw += 360;
		}

		if( yaw > oldYaw ) {
		    if( yaw - oldYaw > degsSec * pml.frametime ) {
		        ps->viewangles[YAW] = oldYaw + degsSec * pml.frametime;

		        // Set delta_angles properly
		        newDeltaAngle = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
		    }
		} else if( oldYaw > yaw ) {
		    if( oldYaw - yaw > degsSec * pml.frametime ) {
		        ps->viewangles[YAW] = oldYaw - degsSec * pml.frametime;

		        // Set delta_angles properly
		        newDeltaAngle = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
		    }
		}*/

		// Check if we are allowed to rotate to there
		if (GetWeaponTableData(ps->weapon)->isMGSet)
		{
			float yawDiff;

			pitchMax = 20.f;

			// yaw
			yawDiff = ps->viewangles[YAW] - pmext->mountedWeaponAngles[YAW];

			if (yawDiff > 180)
			{
				yawDiff -= 360;
			}
			else if (yawDiff < -180)
			{
				yawDiff += 360;
			}

			if (yawDiff > 20)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] + 20.f);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
			else if (yawDiff < -20)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] - 20.f);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}

		// pitch
		pitchDiff = ps->viewangles[PITCH] - pmext->mountedWeaponAngles[PITCH];

		if (pitchDiff > 180)
		{
			pitchDiff -= 360;
		}
		else if (pitchDiff < -180)
		{
			pitchDiff += 360;
		}

		if (pitchDiff > pitchMax)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] + pitchMax);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
		else if (pitchDiff < -pitchMax)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] - pitchMax);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}

		// Check if we rotated into a wall with our legs, if so, undo yaw
		if (ps->viewangles[YAW] != oldYaw)
		{
			// see if we have the space to go prone
			// we know our main body isn't in a solid, check for our legs

			// bugfix - use supplied trace - pm may not be set
			PM_TraceLegs(&traceres, &pmext->proneLegsOffset, ps->origin, ps->origin, NULL, ps->viewangles, pm->trace, ps->clientNum, tracemask);

			if (traceres.allsolid /* && trace.entityNum >= MAX_CLIENTS */)
			{
				// starting in a solid, no space
				ps->viewangles[YAW]   = oldYaw;
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
			else
			{
				// all fine
				ps->delta_angles[YAW] = newDeltaAngle;
			}
		}
	}
}

qboolean ladderforward;
vec3_t   laddervec;

#define TRACE_LADDER_DIST   48.0

/**
 * @brief Checks to see if we are on a ladder
 */
void PM_CheckLadderMove(void)
{
	vec3_t   spot;
	vec3_t   flatforward;
	trace_t  trace;
	float    tracedist;
	qboolean wasOnLadder;

	if (pm->ps->pm_time)
	{
		return;
	}

	if (pml.walking)
	{
		tracedist = 1.0;
	}
	else
	{
		tracedist = TRACE_LADDER_DIST;
	}

	wasOnLadder = ((pm->ps->pm_flags & PMF_LADDER) != 0);

	pml.ladder        = qfalse;
	pm->ps->pm_flags &= ~PMF_LADDER;    // clear ladder bit
	ladderforward     = qfalse;

	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane         = qfalse;
		pml.walking             = qfalse;
		return;
	}

	// Can't climb ladders while prone
	if (pm->ps->eFlags & EF_PRONE)
	{
		return;
	}

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, tracedist, flatforward, spot);
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask);
	if ((trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER))
	{
		pml.ladder = qtrue;
	}

	if (pml.ladder)
	{
		VectorCopy(trace.plane.normal, laddervec);
	}

	if (pml.ladder && !pml.walking && (trace.fraction * tracedist > 1.0f))
	{
		vec3_t mins;
		// if we are only just on the ladder, don't do this yet, or it may throw us back off the ladder
		pml.ladder = qfalse;
		VectorCopy(pm->mins, mins);
		mins[2] = -1;
		VectorMA(pm->ps->origin, -tracedist, laddervec, spot);
		pm->trace(&trace, pm->ps->origin, mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask);
		if ((trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER))
		{
			ladderforward     = qtrue;
			pml.ladder        = qtrue;
			pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
		}
		else
		{
			pml.ladder = qfalse;
		}
	}
	else if (pml.ladder)
	{
		pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
	}

	// create some up/down velocity if touching ladder
	if (pml.ladder)
	{
		if (pml.walking)
		{
			// we are currently on the ground, only go up and prevent X/Y if we are pushing forwards
			if (pm->cmd.forwardmove <= 0)
			{
				pml.ladder = qfalse;
			}
		}
	}

	// if we have just dismounted the ladder at the top, play dismount
	if (!pml.ladder && wasOnLadder && pm->ps->velocity[2] > 0)
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_CLIMB_DISMOUNT, qfalse, qfalse);
	}
	// if we have just mounted the ladder
	if (pml.ladder && !wasOnLadder && pm->ps->velocity[2] < 0)        // only play anim if going down ladder
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_CLIMB_MOUNT, qfalse, qfalse);
	}
}

/**
 * @brief PM_LadderMove
 */
void PM_LadderMove(void)
{
	float  wishspeed, scale;
	vec3_t wishdir, wishvel;
	float  upscale;

	if (ladderforward)
	{
		// move towards the ladder
		VectorScale(laddervec, -200.0, wishvel);
		pm->ps->velocity[0] = wishvel[0];
		pm->ps->velocity[1] = wishvel[1];
	}

	upscale = (pml.forward[2] + 0.5f) * 2.5f;
	if (upscale > 1.0f)
	{
		upscale = 1.0f;
	}
	else if (upscale < -1.0f)
	{
		upscale = -1.0f;
	}

	// forward/right should be horizontal only
	pml.forward[2] = 0;
	pml.right[2]   = 0;
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	// move depending on the view, if view is straight forward, then go up
	// if view is down more then X degrees, start going down
	// if they are back pedalling, then go in reverse of above
	scale = PM_CmdScale(&pm->cmd);
	VectorClear(wishvel);

	if (pm->cmd.forwardmove)
	{
		wishvel[2] = 0.9f * upscale * scale * (float)pm->cmd.forwardmove;
	}
	//Com_Printf("wishvel[2] = %i, fwdmove = %i\n", (int)wishvel[2], (int)pm->cmd.forwardmove );

	if (pm->cmd.rightmove)
	{
		// strafe, so we can jump off ladder
		vec3_t ladder_right, ang;
		vectoangles(laddervec, ang);
		AngleVectors(ang, NULL, ladder_right, NULL);

		// if we are looking away from the ladder, reverse the right vector
		if (DotProduct(laddervec, pml.forward) < 0)
		{
			VectorInverse(ladder_right);
		}

		//VectorMA( wishvel, 0.5 * scale * (float)pm->cmd.rightmove, pml.right, wishvel );
		VectorMA(wishvel, 0.5f * scale * (float)pm->cmd.rightmove, ladder_right, wishvel);
	}

	// do strafe friction
	PM_Friction();

	if (pm->ps->velocity[0] < 1 && pm->ps->velocity[0] > -1)
	{
		pm->ps->velocity[0] = 0;
	}
	if (pm->ps->velocity[1] < 1 && pm->ps->velocity[1] > -1)
	{
		pm->ps->velocity[1] = 0;
	}

	wishspeed = VectorNormalize2(wishvel, wishdir);

	PM_Accelerate(wishdir, wishspeed, pm_accelerate);
	if (wishvel[2] == 0.f)
	{
		if (pm->ps->velocity[2] > 0)
		{
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
			if (pm->ps->velocity[2] < 0)
			{
				pm->ps->velocity[2] = 0;
			}
		}
		else
		{
			pm->ps->velocity[2] += pm->ps->gravity * pml.frametime;
			if (pm->ps->velocity[2] > 0)
			{
				pm->ps->velocity[2] = 0;
			}
		}
	}

	//Com_Printf("vel[2] = %i\n", (int)pm->ps->velocity[2] );

	PM_StepSlideMove(qfalse);    // no gravity while going up ladder

	// always point legs forward
	pm->ps->movementDir = 0;
}

/**
 * @brief PM_Sprint
 */
void PM_Sprint(void)
{
	if (pm->waterlevel <= 1) // no sprint & no stamina recharge under water
	{
		if ((pm->cmd.buttons & BUTTON_SPRINT) && (pm->cmd.forwardmove || pm->cmd.rightmove) && !(pm->ps->pm_flags & PMF_DUCKED) && !(pm->ps->eFlags & EF_PRONE))
		{
			if (pm->ps->powerups[PW_ADRENALINE])
			{
				pm->pmext->sprintTime = SPRINTTIME;
			}
			else if (pm->ps->powerups[PW_NOFATIGUE])
			{
				// take time from powerup before taking it from sprintTime
				pm->ps->powerups[PW_NOFATIGUE] -= 50;

				// go ahead and continue to recharge stamina at double
				// rate with stamina powerup even when exerting
				pm->pmext->sprintTime += 10;
				if (pm->pmext->sprintTime > SPRINTTIME)
				{
					pm->pmext->sprintTime = SPRINTTIME;
				}

				if (pm->ps->powerups[PW_NOFATIGUE] < 0)
				{
					pm->ps->powerups[PW_NOFATIGUE] = 0;
				}
			}
			// sprint time tuned for multiplayer
			else
			{
				// adjusted for framerate independence
				pm->pmext->sprintTime -= 5000 * pml.frametime;
			}

			if (pm->pmext->sprintTime < 0)
			{
				pm->pmext->sprintTime = 0;
			}

			if (!pm->ps->sprintExertTime)
			{
				pm->ps->sprintExertTime = 1;
			}
		}
		else
		{
			// in multiplayer, recharge faster for top 75% of sprint bar
			// (for people that *just* use it for jumping, not sprint) this code was
			// mucked about with to eliminate client-side framerate-dependancy in wolf single player
			if (pm->ps->powerups[PW_ADRENALINE])
			{
				pm->pmext->sprintTime = SPRINTTIME;
			}
			else if (pm->ps->powerups[PW_NOFATIGUE])       // recharge at 2x with stamina powerup
			{
				pm->pmext->sprintTime += 10;
			}
			else
			{
				int rechargebase = 500;

				if (pm->skill[SK_BATTLE_SENSE] >= 2)
				{
					rechargebase *= 1.6f;
				}

				pm->pmext->sprintTime += rechargebase * pml.frametime;        // adjusted for framerate independence
				if (pm->pmext->sprintTime > 5000)
				{
					pm->pmext->sprintTime += rechargebase * pml.frametime;    // adjusted for framerate independence
				}
			}

			if (pm->pmext->sprintTime > SPRINTTIME)
			{
				pm->pmext->sprintTime = SPRINTTIME;
			}

			pm->ps->sprintExertTime = 0;
		}
	}
}

void trap_SnapVector(float *v);

/**
 * @brief PmoveSingle
 * @param[in,out] pmove
 */
void PmoveSingle(pmove_t *pmove)
{
	// update conditional values for anim system
	BG_AnimUpdatePlayerStateConditions(pmove);

	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch   = 0;
	pm->watertype  = 0;
	pm->waterlevel = 0;

	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		pm->tracemask  &= ~CONTENTS_BODY;   // corpses can fly through bodies
		pm->ps->eFlags &= ~EF_ZOOMING;
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

	// set the firing flag for continuous beam weapons

	pm->ps->eFlags &= ~(EF_FIRING | EF_ZOOMING);

	if ((pm->cmd.wbuttons & WBUTTON_ZOOM) && pm->ps->stats[STAT_HEALTH] >= 0 && !(pm->ps->weaponDelay) && pm->ps->weaponstate != WEAPON_RELOADING &&
	    pm->ps->weaponstate != WEAPON_RAISING && pm->ps->weaponstate != WEAPON_DROPPING)
	{
		if (pm->ps->stats[STAT_KEYS] & (1 << INV_BINOCS))               // binoculars are an inventory item (inventory==keys)
		{
			// don't allow binocs:
			if (!GetWeaponTableData(pm->ps->weapon)->isScoped &&        // if using the sniper scope
			    !BG_PlayerMounted(pm->ps->eFlags) &&                    // or if mounted on a weapon
			    !GetWeaponTableData(pm->ps->weapon)->isSetWeapon &&     // w/ mounted mob. MG42 or mortar either.
			    !(pm->ps->eFlags & EF_PRONE_MOVING))                    // when prone moving
			{
				pm->ps->eFlags |= EF_ZOOMING;
			}
		}

		// don't allow binocs if in the middle of throwing grenade
		if ((GetWeaponTableData(pm->ps->weapon)->isGrenade || pm->ps->weapon == WP_DYNAMITE) && pm->ps->grenadeTimeLeft > 0)
		{
			pm->ps->eFlags &= ~EF_ZOOMING;
		}
	}

	if (!(pm->ps->pm_flags & PMF_RESPAWNED) &&
	    (pm->ps->pm_type != PM_INTERMISSION))
	{
		// check for ammo
		if (PM_WeaponAmmoAvailable(pm->ps->weapon))
		{
			// check if zooming
			// Let's use the same flag we just checked above, Ok?
			if (!(pm->ps->eFlags & EF_ZOOMING))
			{
				if (pm->ps->leanf == 0.f)
				{
					if (pm->ps->weaponstate == WEAPON_READY || pm->ps->weaponstate == WEAPON_FIRING)
					{
						// all clear, fire!
						if ((pm->cmd.buttons & BUTTON_ATTACK) && !(pm->cmd.buttons & BUTTON_TALK))
						{
							pm->ps->eFlags |= EF_FIRING;
						}
					}
				}
			}
		}
	}

	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		if (pm->ps->stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
		{
			pm->pmext->silencedSideArm |= 1;
		}

		// clear the respawned flag if attack button are cleared
		// don't clear if a weapon change is needed to prevent early weapon change
		if (pm->ps->stats[STAT_HEALTH] > 0 &&
		    !(pm->cmd.buttons & BUTTON_ATTACK) &&  // & (BUTTON_ATTACK /*| BUTTON_USE_HOLDABLE
		    !(pm->cmd.wbuttons & WBUTTON_ATTACK2) &&
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

	// moved from engine to this very place
	// no movement while using a static mg42
	if (pm->ps->persistant[PERS_HWEAPON_USE])
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove   = 0;
		pmove->cmd.upmove      = 0;
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
	if (pm->ps->pm_type != PM_FREEZE)     // added PM_FREEZE
	{
		if (!(pm->ps->pm_flags & PMF_LIMBO))
		{
			// added tracemask
			PM_UpdateViewAngles(pm->ps, pm->pmext, &pm->cmd, pm->trace, pm->tracemask);     // modified
			// pmove_fixed - moved update lean out of UpdateViewAngles
			PM_UpdateLean(pm->ps, &pm->cmd, pm);
		}
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
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	case PM_NOCLIP:
		PM_NoclipMove();
		PM_DropTimers();
		return;
	case PM_FREEZE:       // no movement at all
	case PM_INTERMISSION: // no movement at all
		return;
	case PM_NORMAL:
		if (GetWeaponTableData(pm->ps->weapon)->isMortarSet)
		{
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove   = 0;
			pm->cmd.upmove      = 0;
		}
		break;

	default:
		break;
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	if (!PM_CheckProne())
	{
		PM_CheckDuck();
	}

	// set groundentity
	PM_GroundTrace();

	if (pm->ps->pm_type == PM_DEAD)
	{
		PM_DeadMove();

		if (GetWeaponTableData(pm->ps->weapon)->isMortarSet)
		{
			pm->ps->weapon = GetWeaponTableData(pm->ps->weapon)->weapAlts;
		}
	}
	else
	{
		if (GetWeaponTableData(pm->ps->weapon)->isMGSet)
		{
			if (!(pm->ps->eFlags & EF_PRONE))
			{
				PM_BeginWeaponChange(pm->ps->weapon, GetWeaponTableData(pm->ps->weapon)->weapAlts, qfalse);
#ifdef CGAMEDLL
				cg.weaponSelect = GetWeaponTableData(pm->ps->weapon)->weapAlts;
#endif // CGAMEDLL
			}
		}

		if (pm->ps->weapon == WP_SATCHEL_DET)
		{
			if (!(pm->ps->ammoclip[WP_SATCHEL_DET]))
			{
				PM_BeginWeaponChange(WP_SATCHEL_DET, WP_SATCHEL, qtrue);
#ifdef CGAMEDLL
				cg.weaponSelect = WP_SATCHEL;
#endif // CGAMEDLL
			}
		}
	}

	// ladders
	PM_CheckLadderMove();

	PM_DropTimers();

	if (pml.ladder)
	{
		PM_LadderMove();
	}
	else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		PM_WaterJumpMove();
	}
	else if (pm->waterlevel > 1)
	{
		// swimming
		PM_WaterMove();
	}
	else if (!(pm->ps->eFlags & EF_MOUNTEDTANK))
	{
		if (pml.walking)
		{
			// walking on ground
			PM_WalkMove();
		}
		else
		{
			// airborne
			PM_AirMove();
		}
	}
	else //if (pm->ps->eFlags & EF_MOUNTEDTANK)
	{
		VectorClear(pm->ps->velocity);

		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

		BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
	}

	PM_Sprint();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	if (PM_FIXEDPHYSICS)
	{
		// Pmove accurate code
		// the new way: don't care so much about 6 bytes/frame
		// or so of network bandwidth, and add some mostly framerate-
		// independent error to make up for the lack of rounding error
		// halt if not going fast enough (0.5 units/sec)
		if (VectorLengthSquared(pm->ps->velocity) < 0.25f)
		{
			VectorClear(pm->ps->velocity);
		}
		else
		{
			int   i;
			float fac;
			float fps = PM_FIXEDPHYSICSFPS;

			if (fps > 333)
			{
				fps = 333;
			}
			else if (fps < 60)
			{
				fps = 60;
			}

			fac = pml.msec / (1000.0f / fps);

			// add some error...
			for (i = 0; i < 3; ++i)
			{
				// ...if the velocity in this direction changed enough
				if (Q_fabs(pm->ps->velocity[i] - pml.previous_velocity[i]) > 0.5f / fac)
				{
					if (pm->ps->velocity[i] < 0)
					{
						pm->ps->velocity[i] -= 0.5f * fac;
					}
					else
					{
						pm->ps->velocity[i] += 0.5f * fac;
					}
				}
			}
			// we can stand a little bit of rounding error for the sake
			// of lower bandwidth
			VectorScale(pm->ps->velocity, 64.0f, pm->ps->velocity);
			// snap some parts of playerstate to save network bandwidth
			trap_SnapVector(pm->ps->velocity);
			VectorScale(pm->ps->velocity, 1.0f / 64.0f, pm->ps->velocity);
		}
	}
	else
	{
		// snap some parts of playerstate to save network bandwidth
		trap_SnapVector(pm->ps->velocity);
	}

	// save sprinttime for CG_DrawStaminaBar()
	pm->ps->stats[STAT_SPRINTTIME] = pm->pmext->sprintTime;
}

/**
 * @brief Can be called by either the server or the client
 * @param[in] pmove
 * @return
 */
int Pmove(pmove_t *pmove)
{
	int msec;
	int finalTime = pmove->cmd.serverTime;
	int gravity   = pmove->ps->gravity;

	if (finalTime < pmove->ps->commandTime)
	{
		return 0;   // should not happen
	}

	if (finalTime > pmove->ps->commandTime + 1000)
	{
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount + 1) & ((1 << PS_PMOVEFRAMECOUNTBITS) - 1);

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

		pmove->ps->gravity = gravity;
		PM_AdjustAimSpreadScale();
		PmoveSingle(pmove);

		if (pmove->ps->pm_flags & PMF_JUMP_HELD)
		{
			pmove->cmd.upmove = 20;
		}
	}

	// sanity check weapon heat
	if (pmove->ps->curWeapHeat > 255)
	{
		pmove->ps->curWeapHeat = 255;
	}
	else if (pmove->ps->curWeapHeat < 0)
	{
		pmove->ps->curWeapHeat = 0;
	}

	if ((pm->ps->stats[STAT_HEALTH] <= 0 || pm->ps->pm_type == PM_DEAD) && (pml.groundTrace.surfaceFlags & SURF_MONSTERSLICK))
	{
		return (pml.groundTrace.surfaceFlags);
	}
	else
	{
		return 0;
	}
}

/**
 * @brief Used to calculate player movement for g_skipCorrection.
 *
 *  Before calling PmovePredict() the following player state
 *  values should be backed up and then restored after the
 *  new values have been copied to the entity state.
 *
 *  PM_GroundTrace() and friends modify
 *      ps->groundEntityNum
 *      ps->pm_flags
 *      ps->pm_time
 *      ps->eFlags
 *
 *  PM_StepSlideMove() and friends modify
 *      ps->origin
 *      ps->velocity
 *
 * @param[in] pmove
 * @param[in] frametime
 */
void PmovePredict(pmove_t *pmove, float frametime)
{
	pm = pmove;
	Com_Memset(&pml, 0, sizeof(pml));
	pml.frametime = frametime;
	PM_GroundTrace();

	// don't bother to figure out gravity if already on the ground
	//      or moving on a ladder.
	if (pml.groundPlane || (pm->ps->pm_flags & PMF_LADDER))
	{
		PM_StepSlideMove(qfalse);
	}
	else
	{
		PM_StepSlideMove(qtrue);
	}
}
