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

#else
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

float pm_waterSwimScale = 0.5f;
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
 * @brief PM_IdleAnimForWeapon
 * @param[in] weapon
 * @return
 */
int PM_IdleAnimForWeapon(int weapon)
{
	if (GetWeaponTableData(weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_RIFLENADE))
	{
		return WEAP_IDLE2;
	}

	return WEAP_IDLE1;
}

/**
 * @brief PM_ReloadAnimForWeapon
 * @param[in] weapon
 * @return
 */
int PM_ReloadAnimForWeapon(int weapon)
{
	if ((pm->skill[SK_LIGHT_WEAPONS] >= 2 && GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_FAST_RELOAD)      // faster reload
	    || GetWeaponTableData(weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_RIFLENADE))
	{
		return WEAP_RELOAD2;
	}

	return WEAP_RELOAD1;
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
	if ((pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == anim)
	{
		return;
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
void PM_TraceLegs(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end, trace_t *bodytrace, vec3_t viewangles,
                  void(tracefunc) (trace_t *results,
                                   const vec3_t start,
                                   const vec3_t mins,
                                   const vec3_t maxs,
                                   const vec3_t end,
                                   int passEntityNum,
                                   int contentMask),
                  int ignoreent,
                  int tracemask)
{
	vec3_t ofs, org, point;

	// zinx - don't let players block legs
	tracemask &= ~(CONTENTS_BODY | CONTENTS_CORPSE);

	if (legsOffset)
	{
		*legsOffset = 0;
	}

	// legs position
	BG_LegsCollisionBoxOffset(viewangles, pm->ps->eFlags, ofs);
	//VectorAdd(start, ofs, org);
	VectorCopy(start, org);
	VectorAdd(end, ofs, point);

	tracefunc(trace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
	if (!bodytrace || trace->fraction < bodytrace->fraction || trace->allsolid)
	{
		trace_t steptrace;

		// legs are clipping sooner than body
		// see if our legs can step up

		// give it a try with the new height
		point[2] += STEPSIZE;
		org[2]   += STEPSIZE;

		//VectorAdd(start, ofs, org);
		//VectorAdd(end, ofs, point);
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
				org[2]   -= STEPSIZE;

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
                  void(tracefunc) (trace_t *results,
                                   const vec3_t start,
                                   const vec3_t mins,
                                   const vec3_t maxs,
                                   const vec3_t end,
                                   int passEntityNum,
                                   int contentMask),
                  int ignoreent,
                  int tracemask)
{
	vec3_t ofs, org, point;

	// don't let players block head
	tracemask &= ~(CONTENTS_BODY | CONTENTS_CORPSE);

	// head position
	BG_HeadCollisionBoxOffset(viewangles, pm->ps->eFlags, ofs);
	//VectorAdd(start, ofs, org);
	VectorCopy(start, org);
	VectorAdd(end, ofs, point);

	tracefunc(trace, org, playerHeadProneMins, playerHeadProneMaxs, point, ignoreent, tracemask);
	if (!bodytrace || trace->fraction < bodytrace->fraction || trace->allsolid)
	{
		trace_t steptrace;

		// head are clipping sooner than body
		// see if our head can step up

		// give it a try with the new height
		point[2] += STEPSIZE;
		org[2]   += STEPSIZE;

		//VectorAdd(start, ofs, org);
		//VectorAdd(end, ofs, point);
		tracefunc(&steptrace, org, playerHeadProneMins, playerHeadProneMaxs, point, ignoreent, tracemask);
		if (!steptrace.allsolid && !steptrace.startsolid &&
		    steptrace.fraction > trace->fraction)
		{
			// the step trace did better -- use it instead
			*trace = steptrace;
		}
	}
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
	if (GetWeaponTableData(pm->ps->weapon)->skillBased == SK_HEAVY_WEAPONS && !CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
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

		if (pm->ps->weaponDelay && (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_PANZER))
		{
			return qfalse;
		}

		if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
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
			vec3_t  end;
			vec3_t  oldOrigin;

			pm->mins[0] = pm->ps->mins[0];
			pm->mins[1] = pm->ps->mins[1];

			pm->maxs[0] = pm->ps->maxs[0];
			pm->maxs[1] = pm->ps->maxs[1];

			pm->mins[2] = pm->ps->mins[2];
			pm->maxs[2] = pm->ps->crouchMaxZ;

			BG_LegsCollisionBoxOffset(pm->ps->viewangles, EF_PRONE, end);
			VectorAdd(pm->ps->origin, end, end);

			pm->trace(&trace, pm->ps->origin, playerlegsProneMins, playerlegsProneMaxs, end, pm->ps->clientNum, pm->tracemask);

			if (trace.fraction != 1.f)
			{
				VectorSubtract(trace.endpos, end, end);
				VectorCopy(pm->ps->origin, oldOrigin);

				pm->ps->eFlags |= EF_PRONE;
				PM_StepSlideMove(qfalse);
				PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);

				if (trace.startsolid || trace.allsolid || trace.fraction != 1.f)
				{
					// push back the origin and retry
					VectorAdd(oldOrigin, end, pm->ps->origin);
					PM_StepSlideMove(qfalse);
					PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);

					if (trace.startsolid || trace.allsolid || trace.fraction != 1.f)
					{
						pm->ps->eFlags &= ~EF_PRONE;
						VectorCopy(oldOrigin, pm->ps->origin);
						return qfalse;
					}
				}
				pm->ps->eFlags &= ~EF_PRONE;
			}
			else
			{
				BG_HeadCollisionBoxOffset(pm->ps->viewangles, EF_PRONE, end);
				VectorAdd(pm->ps->origin, end, end);

				pm->trace(&trace, pm->ps->origin, playerHeadProneMins, playerHeadProneMaxs, end, pm->ps->clientNum, pm->tracemask);

				if (trace.fraction != 1.f)
				{
					VectorSubtract(trace.endpos, end, end);
					VectorCopy(pm->ps->origin, oldOrigin);

					pm->ps->eFlags |= EF_PRONE;
					PM_StepSlideMove(qfalse);
					PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);

					if (trace.startsolid || trace.allsolid || trace.fraction != 1.f)
					{
						// push back the origin and retry
						VectorAdd(oldOrigin, end, pm->ps->origin);
						PM_StepSlideMove(qfalse);
						PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);

						if (trace.startsolid || trace.allsolid || trace.fraction != 1.f)
						{
							pm->ps->eFlags &= ~EF_PRONE;
							VectorCopy(oldOrigin, pm->ps->origin);
							return qfalse;
						}
					}
					pm->ps->eFlags &= ~EF_PRONE;
				}
			}

			if (PM_PRONEDELAY)
			{
				pm->ps->aimSpreadScale      = AIMSPREAD_MAXSPREAD;
				pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;
			}

			// go prone
			if (trace.fraction == 1.0f)
			{
				pm->ps->pm_flags    |= PMF_DUCKED;                           // crouched as well
				pm->ps->eFlags      |= EF_PRONE;
				pm->pmext->proneTime = pm->cmd.serverTime;                           // timestamp 'go prone'
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
				if (GetWeaponTableData(pm->ps->weapon)->type & (WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET))
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
		pm->maxs[2]        = PRONE_BODYHEIGHT_BBOX;
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
			    (pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHFROM ||
			    (pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHTO)
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

	// push back from ladder
	if (pm->ps->pm_flags & PMF_LADDER)
	{
		float  angle;
		vec3_t flatforward;

		angle          = DEG2RAD(pm->ps->viewangles[YAW]);
		flatforward[0] = cos(angle);
		flatforward[1] = sin(angle);
		flatforward[2] = 0;

		VectorMA(pm->ps->origin, -32, flatforward, pm->ps->origin);

		PM_StepSlideMove(qtrue);

		pm->ps->pm_flags &= ~PMF_LADDER;
	}

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
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_50, PM_FootstepForSurface());
				//BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_IMPACT_POINT, (rand() + 1) ? IMPACTPOINT_KNEE_RIGHT : IMPACTPOINT_KNEE_LEFT, qtrue);
				//BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
			}
		}
		else if (delta > 58)
		{
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_25, PM_FootstepForSurface());
				//BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_IMPACT_POINT, (rand() + 1) ? IMPACTPOINT_KNEE_RIGHT : IMPACTPOINT_KNEE_LEFT, qtrue);
				//BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
			}
		}
		else if (delta > 48)
		{
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_15, PM_FootstepForSurface());
				//BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_IMPACT_POINT, (rand() + 1) ? IMPACTPOINT_KNEE_RIGHT : IMPACTPOINT_KNEE_LEFT, qtrue);
				//BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
			}
		}
		else if (delta > 38.75f)
		{
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0)
			{
				PM_AddEventExt(EV_FALL_DMG_10, PM_FootstepForSurface());
				//BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_IMPACT_POINT, (rand() + 1) ? IMPACTPOINT_KNEE_RIGHT : IMPACTPOINT_KNEE_LEFT, qtrue);
				//BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
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

	// duck
	if ((pm->cmd.upmove < 0 && !(pm->ps->eFlags & EF_MOUNTEDTANK) && !(pm->ps->pm_flags & PMF_LADDER))
	    || CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
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
			if (pm->ps->eFlags & EF_TALK && !(GetWeaponTableData(pm->ps->weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_SCOPED)))
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
			if (pm->ps->eFlags & EF_TALK && !(GetWeaponTableData(pm->ps->weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_SCOPED)))
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
			if (pm->ps->eFlags & EF_TALK && !(GetWeaponTableData(pm->ps->weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_SCOPED)))
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
	if (pm->ps->ammoclip[GetWeaponTableData(weapon)->clipIndex] >= GetWeaponTableData(weapon)->maxClip)
	{
		// akimbo should also check other weapon status
		if (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
		{
			if (pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex] >= GetWeaponTableData(GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex)->maxClip)
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// no reload when leaning (this includes manual and auto reloads but no throwable as nade and canister)
	if (pm->ps->leanf != 0.f && !(GetWeaponTableData(weapon)->firingMode & WEAPON_FIRING_MODE_THROWABLE))
	{
		return;
	}

	// easier check now that the animation system handles the specifics
	if (!(GetWeaponTableData(weapon)->firingMode & WEAPON_FIRING_MODE_THROWABLE))
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

	if (!(GetWeaponTableData(weapon)->type & WEAPON_TYPE_MORTAR))
	{
		PM_ContinueWeaponAnim(PM_ReloadAnimForWeapon(pm->ps->weapon));
	}

	// okay to reload while overheating without tacking the reload time onto the end of the
	// current weaponTime (the reload time is partially absorbed into the overheat time)
	reloadTime = GetWeaponTableData(weapon)->reloadTime;
	if (pm->skill[SK_LIGHT_WEAPONS] >= 2 && (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_FAST_RELOAD))
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

	// FIXME: Currently, the rifle nade play an extra reload sound which wasn't used before. Maybe we should get ride of this in pak0.
	// once theses sound remove, we can remove this check below
	if (GetWeaponTableData(weapon)->useClip)
	{
		PM_AddEvent(EV_FILL_CLIP);      // play reload sound
	}
}

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
	if (GetWeaponTableData(oldWeapon)->weapAlts && pm->ps->weaponstate == WEAPON_RAISING &&
	    ((pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHFROM || (pm->ps->weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHTO))
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

	// it's an alt mode, play different anim
	if (newWeapon == GetWeaponTableData(oldWeapon)->weapAlts)
	{
		// don't send change weapon event after firing with riflenade
		if (!(GetWeaponTableData(oldWeapon)->type & WEAPON_TYPE_RIFLENADE) || pm->ps->ammoclip[GetWeaponTableData(oldWeapon)->ammoIndex])
		{
			PM_AddEvent(EV_CHANGE_WEAPON_2);

			// special case for silenced pistol
			if (((GetWeaponTableData(oldWeapon)->type & WEAPON_TYPE_PISTOL) && (GetWeaponTableData(oldWeapon)->attributes & WEAPON_ATTRIBUT_SILENCED)))
			{
				PM_StartWeaponAnim(WEAP_ALTSWITCHTO);

				if (pm->ps->eFlags & EF_PRONE)
				{
					BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse);
				}
				else
				{
					BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE, qfalse, qfalse);
				}

				pm->ps->weaponTime += GetWeaponTableData(newWeapon)->altSwitchTimeTo;
			}

			if (GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_SET)
			{
				vec3_t axis[3];

				VectorCopy(pml.forward, axis[0]);
				VectorCopy(pml.right, axis[2]);
				CrossProduct(axis[0], axis[2], axis[1]);
				AxisToAngles(axis, pm->pmext->mountedWeaponAngles);
			}
		}
	}
	else
	{
		PM_AddEvent(EV_CHANGE_WEAPON);

		PM_StartWeaponAnim(WEAP_DROP);

		// play an animation
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_DROPWEAPON, qfalse, qfalse);

		pm->ps->weaponTime += GetWeaponTableData(oldWeapon)->switchTimeBegin;            // dropping/raising usually takes 1/4 sec.
	}

	pm->ps->weaponstate = reload ? WEAPON_DROPPING_TORELOAD : WEAPON_DROPPING;
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
	if (GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_SCOPED)
	{
		pm->ps->aimSpreadScale      = AIMSPREAD_MAXSPREAD;       // initially at lowest accuracy
		pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;       // initially at lowest accuracy
	}
	else if (GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_PISTOL)
	{
		if (GetWeaponTableData(newWeapon)->attributes & WEAPON_ATTRIBUT_SILENCED)
		{
			pm->pmext->silencedSideArm |= 1;
		}
		else
		{
			pm->pmext->silencedSideArm &= ~1;
		}
	}
	else if (GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_RIFLE)
	{
		pm->pmext->silencedSideArm &= ~2;
	}
	else if (GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_RIFLENADE)
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
		if (((GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_RIFLE) && !pm->ps->ammoclip[GetWeaponTableData(oldWeapon)->ammoIndex])
		    || ((GetWeaponTableData(oldWeapon)->type & WEAPON_TYPE_PISTOL) && (GetWeaponTableData(oldWeapon)->attributes & WEAPON_ATTRIBUT_SILENCED)))
		{
			return;
		}

		BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_WEAPON, newWeapon, qtrue);

		if (GetWeaponTableData(oldWeapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_SCOPED | WEAPON_TYPE_RIFLENADE))
		{
			PM_StartWeaponAnim(WEAP_ALTSWITCHTO);

			if (pm->ps->eFlags & EF_PRONE)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse);
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE, qfalse, qfalse);
			}

			pm->ps->weaponTime += GetWeaponTableData(newWeapon)->altSwitchTimeTo;
		}
		else
		{
			PM_StartWeaponAnim(WEAP_ALTSWITCHFROM);

			if (pm->ps->eFlags & EF_PRONE)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_DO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse);
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_DO_ALT_WEAPON_MODE, qfalse, qfalse);
			}

			pm->ps->weaponTime += GetWeaponTableData(oldWeapon)->altSwitchTimeFrom;
		}
	}
	else
	{
		BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_WEAPON, newWeapon, qtrue);

		if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_RAISEWEAPONPRONE, qfalse, qfalse);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_RAISEWEAPON, qfalse, qfalse);
		}

		PM_StartWeaponAnim(WEAP_RAISE);

		pm->ps->weaponTime += GetWeaponTableData(newWeapon)->switchTimeFinish;                  // dropping/raising usually takes 1/4 sec.
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
	if (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
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
	PM_StartWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon));
}

/**
 * @brief PM_CheckForReload
 * @param[in] weapon
 */
void PM_CheckForReload(weapon_t weapon)
{
	if (pm->noWeapClips)     // no need to reload
	{
		return;
	}

	// some weapons don't use ammo and no need to reload
	if (!GetWeaponTableData(weapon)->useAmmo)
	{
		return;
	}

	if (pm->ps->eFlags & EF_ZOOMING)
	{
		return;
	}

	if (pm->ps->weaponstate != WEAPON_READY && pm->ps->weaponstate != WEAPON_FIRING)
	{
		return;
	}

	if (pm->ps->weaponTime <= 0)
	{
		qboolean doReload = qfalse;

		// user is forcing a reload (manual reload)
		if (pm->cmd.wbuttons & WBUTTON_RELOAD)
		{
			if (pm->ps->ammo[GetWeaponTableData(weapon)->ammoIndex])
			{
				if (pm->ps->ammoclip[GetWeaponTableData(weapon)->clipIndex] < GetWeaponTableData(weapon)->maxClip)
				{
					doReload = qtrue;
				}

				// akimbo should also check other weapon status
				if (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
				{
					if (pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex] < GetWeaponTableData(GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex)->maxClip)
					{
						doReload = qtrue;
					}
				}
			}
		}
		else if (pm->pmext->bAutoReload || !(GetWeaponTableData(weapon)->firingMode & (WEAPON_FIRING_MODE_AUTOMATIC | WEAPON_FIRING_MODE_SEMI_AUTOMATIC)))   // auto reload
		{
			if (!pm->ps->ammoclip[GetWeaponTableData(weapon)->clipIndex] && pm->ps->ammo[GetWeaponTableData(weapon)->ammoIndex])
			{
				if (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
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
			if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_SCOPED)
			{
				PM_BeginWeaponChange(weapon, GetWeaponTableData(weapon)->weapAlts, qtrue);
			}

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
	if (!(GetWeaponTableData(pm->ps->weapon)->firingMode & (WEAPON_FIRING_MODE_ONE_SHOT | WEAPON_FIRING_MODE_THROWABLE)))
	{
		return;
	}

	// In multiplayer, pfaust fires once then switches to pistol since it's useless for a while
	// after throwing landmine, let switch to pliers
	if (!(GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_ONE_SHOT))
	{
		// don't consumme ammo
		if (!GetWeaponTableData(pm->ps->weapon)->useAmmo)
		{
			return;
		}

		// use clip and still got ammo in clip
		if (GetWeaponTableData(pm->ps->weapon)->useClip && pm->ps->ammoclip[GetWeaponTableData(pm->ps->weapon)->clipIndex])
		{
			return;
		}

		// still got ammo in reserve
		if (pm->ps->ammo[GetWeaponTableData(pm->ps->weapon)->ammoIndex])
		{
			return;
		}
	}

	// If this was the last one, remove the weapon and switch away before the player tries to fire next
	// NOTE: giving grenade ammo to a player will re-give him the weapon (if you do it through add_ammo())
	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_GRENADE)
	{
		COM_BitClear(pm->ps->weapons, pm->ps->weapon);
	}
	else if (pm->ps->weapon == WP_SATCHEL)
	{
		pm->ps->ammoclip[WP_SATCHEL_DET] = 1;
		pm->ps->ammoclip[WP_SATCHEL]     = 0;
	}

	// force switching to rifle
	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_RIFLENADE)
	{
		PM_BeginWeaponChange(pm->ps->weapon, GetWeaponTableData(pm->ps->weapon)->weapAlts, qfalse);
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
		int takeweapon = GetWeaponTableData(wp)->clipIndex;

		if (GetWeaponTableData(wp)->attributes & WEAPON_ATTRIBUT_AKIMBO)
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

	if (GetWeaponTableData(wp)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		if (!BG_AkimboFireSequence(wp, pm->ps->ammoclip[GetWeaponTableData(wp)->clipIndex], pm->ps->ammoclip[GetWeaponTableData(GetWeaponTableData(wp)->akimboSideArm)->clipIndex]))
		{
			return pm->ps->ammoclip[GetWeaponTableData(wp)->akimboSideArm];
		}
	}

	return pm->ps->ammoclip[GetWeaponTableData(wp)->clipIndex];
}

/**
 * @brief Accounts for clips being used/not used
 * @param[in] wp
 * @return
 */
qboolean PM_WeaponClipEmpty(weapon_t wp)
{
	if (pm->noWeapClips)
	{
		return !pm->ps->ammo[GetWeaponTableData(wp)->ammoIndex];
	}

	return !pm->ps->ammoclip[GetWeaponTableData(wp)->clipIndex];
}

/**
 * @brief PM_CoolWeapons
 */
void PM_CoolWeapons(void)
{
	weapon_t wp;

	for (wp = WP_KNIFE; wp < WP_NUM_WEAPONS; wp++)
	{
		// if the weapon can heat and you have the weapon
		// dummy MG42 can't be equipped but still have a cool down
		if (GetWeaponTableData(wp)->maxHeat && (COM_BitCheck(pm->ps->weapons, wp) || wp == WP_DUMMY_MG42))
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

	// current weapon can heat, convert current heat value to 0-255 range for client transmission
	if (BG_PlayerMounted(pm->ps->eFlags))
	{
		pm->ps->curWeapHeat = (int)(floor((pm->ps->weapHeat[WP_DUMMY_MG42] / (double)GetWeaponTableData(WP_DUMMY_MG42)->maxHeat) * 255));
	}
	else if (GetWeaponTableData(pm->ps->weapon)->maxHeat)
	{
		pm->ps->curWeapHeat = (int)(floor((pm->ps->weapHeat[pm->ps->weapon] / (double)GetWeaponTableData(pm->ps->weapon)->maxHeat) * 255));
	}
	else
	{
		pm->ps->curWeapHeat = 0;
	}

	// sanity check weapon heat
	if (pm->ps->curWeapHeat > 255)
	{
		pm->ps->curWeapHeat = 255;
	}
	else if (pm->ps->curWeapHeat < 0)
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

		if ((GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_SCOPED) && pm->skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 3)
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
		if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_SCOPED)
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

#define weaponstateFiring (pm->ps->weaponstate == WEAPON_FIRING /*|| pm->ps->weaponstate == WEAPON_FIRINGALT*/)

/**
 * @brief Special mounted mg42 handling
 * @return
 */
static qboolean PM_MountedFire(void)
{
	// player is not using mounted weapon
	if (!BG_PlayerMounted(pm->ps->eFlags))
	{
		return qfalse;
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
		if (pm->ps->eFlags & EF_AAGUN_ACTIVE)
		{
			PM_AddEvent(EV_FIRE_WEAPON_AAGUN);
			pm->ps->weaponTime += AAGUN_RATE_OF_FIRE;

			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue);
			//pm->ps->viewlocked = VIEWLOCK_JITTER;     // this enable screen jitter when firing
		}
		else    // EF_MOUNTEDTANK | EF_MG42_ACTIVE
		{
			pm->ps->weaponTime += GetWeaponTableData(WP_DUMMY_MG42)->nextShotTime;
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue);

			if (pm->ps->eFlags & EF_MG42_ACTIVE)
			{
				PM_AddEvent(EV_FIRE_WEAPON_MG42);
				pm->ps->viewlocked = VIEWLOCK_JITTER;             // this enable screen jitter when firing
			}
			else
			{
				PM_AddEvent(EV_FIRE_WEAPON_MOUNTEDMG42);
				//pm->ps->viewlocked = VIEWLOCK_JITTER;             // this enable screen jitter when firing
			}

			pm->ps->weapHeat[WP_DUMMY_MG42] += GetWeaponTableData(WP_DUMMY_MG42)->nextShotTime;

			// check for overheat
			if (pm->ps->weapHeat[WP_DUMMY_MG42] >= GetWeaponTableData(WP_DUMMY_MG42)->maxHeat)
			{
				pm->ps->weapHeat[WP_DUMMY_MG42] = GetWeaponTableData(WP_DUMMY_MG42)->maxHeat;        // cap heat to max
				PM_AddEvent(EV_WEAP_OVERHEAT);
				pm->ps->weaponTime = GetWeaponTableData(WP_DUMMY_MG42)->heatRecoveryTime;         // force "heat recovery minimum" right now
			}
		}
	}

	return qtrue;
}

/**
 * @brief PM_CheckGrenade
 * @return
 */
static qboolean PM_CheckGrenade()
{
	if (pm->ps->grenadeTimeLeft > 0)
	{
		pm->ps->grenadeTimeLeft -= pml.msec;

		if (pm->ps->grenadeTimeLeft <= 100)         // give two frames advance notice so there's time to launch and detonate
		{
			pm->ps->grenadeTimeLeft = 100;
		}
		else if ((pm->cmd.buttons & BUTTON_ATTACK) && !(pm->ps->eFlags & EF_PRONE_MOVING))
		{
			BG_SetConditionBitFlag(pm->ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_HOLDING);
			return qtrue;
		}
	}
	else if (pm->ps->weapon == WP_DYNAMITE)
	{
		// keep dynamite in hand until fire button is released
		if ((pm->cmd.buttons & BUTTON_ATTACK) && !(pm->ps->eFlags & EF_PRONE_MOVING) && weaponstateFiring && !pm->ps->weaponTime)
		{
			BG_ClearConditionBitFlag(pm->ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_HOLDING);
			return qtrue;
		}
	}

	if (BG_GetConditionBitFlag(pm->ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_HOLDING))
	{
		if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE,
			                   GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_AUTOMATIC, qtrue);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON,
			                   GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_AUTOMATIC, qtrue);
		}
	}

	BG_ClearConditionBitFlag(pm->ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_HOLDING);
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
	qboolean delayedFire       = qfalse; // true if the delay time has just expired and this is the frame to send the fire event
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

	if (GetWeaponTableData(pm->ps->weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
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

	//if (pm->ps->weaponstate == WEAPON_RELAXING)
	//{
	//	pm->ps->weaponstate = WEAPON_READY;
	//	return;
	//}

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
		if (GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_SEMI_AUTOMATIC)
		{
			// moved releasedFire into pmext instead of ps
			if (pm->pmext->releasedFire)
			{
				if (pm->cmd.buttons & BUTTON_ATTACK)
				{
					// akimbo weapons only have a 200ms delay, so
					// use a shorter time for quickfire (#255)
					if (GetWeaponTableData(pm->ps->weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
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
		PM_StartWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon));
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
		PM_ContinueWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon));
		return;
	}

	// this is possible since the player starts with nothing
	if (pm->ps->weapon == WP_NONE)
	{
		return;
	}

	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_PANZER)
	{
		if (pm->ps->eFlags & EF_PRONE)
		{
			return;
		}
	}

	// a not mounted mortar can't fire
	if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, (WEAPON_TYPE_MORTAR | WEAPON_TYPE_SETTABLE)))
	{
		return;
	}

	// check for fire
	// if not on fire button and there's not a delayed shot this frame...
	// consider also leaning, with delayed attack reset
	if ((!(pm->cmd.buttons & BUTTON_ATTACK) && !(pm->cmd.wbuttons & WBUTTON_ATTACK2) && !delayedFire) ||
	    (pm->ps->leanf != 0.f && !GetWeaponTableData(pm->ps->weapon)->grenadeTime))
	{
		pm->ps->weaponTime  = 0;
		pm->ps->weaponDelay = 0;

		if (weaponstateFiring)     // you were just firing, time to relax
		{
			PM_ContinueWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon));
		}

		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// don't allow some weapons to fire if charge bar isn't full
	if (GetWeaponTableData(pm->ps->weapon)->attributes & WEAPON_ATTRIBUT_CHARGE_TIME)
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
			// check if there is enough charge to fire
			if (pm->cmd.serverTime - pm->ps->classWeaponTime < chargeTime * coeff)
			{
				if ((pm->ps->weapon == WP_MEDKIT || pm->ps->weapon == WP_AMMO) && (pm->cmd.buttons & BUTTON_ATTACK))
				{
					BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_NOPOWER, qtrue, qfalse);
				}

				return;
			}

			// ready to fire, handle the charge time
			if (weaponstateFiring)
			{
				if (coeff != 1.f)
				{
					if (pm->cmd.serverTime - pm->ps->classWeaponTime > chargeTime)
					{
						pm->ps->classWeaponTime = pm->cmd.serverTime - chargeTime;
					}

					pm->ps->classWeaponTime += coeff * chargeTime;
				}
				else
				{
					pm->ps->classWeaponTime = pm->cmd.serverTime;
				}
			}
		}
	}

	// player is zooming or using binocular - no fire
	// PC_FIELDOPS needs to zoom to call artillery
	if ((pm->ps->eFlags & EF_ZOOMING) || pm->ps->weapon == WP_BINOCULARS)
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
	if (pm->waterlevel == 3 && !(GetWeaponTableData(pm->ps->weapon)->attributes & WEAPON_ATTRIBUT_FIRE_UNDERWATER))
	{
		PM_AddEvent(EV_NOFIRE_UNDERWATER);      // event for underwater 'click' for nofire
		PM_ContinueWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon));
		pm->ps->weaponTime  = 500;
		pm->ps->weaponDelay = 0;                // avoid insta-fire after water exit on delayed weapon attacks
		return;
	}

	// start the animation even if out of ammo
	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_MELEE)
	{
		if (!delayedFire)
		{
			if (pm->ps->eFlags & EF_PRONE)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qfalse);
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qfalse);
			}
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_THROWABLE
	         && pm->ps->weapon != WP_AMMO && pm->ps->weapon != WP_MEDKIT)   // medic/ammo pack are dropped continuously
	{
		if (!delayedFire)
		{
			if (PM_WeaponAmmoAvailable(pm->ps->weapon))
			{
				// start at and count down
				pm->ps->grenadeTimeLeft = GetWeaponTableData(pm->ps->weapon)->grenadeTime;

				PM_StartWeaponAnim(WEAP_ATTACK1);
			}

			pm->ps->weaponDelay = GetWeaponTableData(pm->ps->weapon)->fireDelayTime;
		}
		else if (!GetWeaponTableData(pm->ps->weapon)->grenadeTime)
		{
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
		if (!weaponstateFiring)
		{
			// pfaust has spinup time in MP
			if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_PANZER)
			{
				PM_AddEvent(EV_SPINUP);
			}
			else if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
			{
				// don't play sound/animation if out of ammo
				if (GetWeaponTableData(pm->ps->weapon)->uses <= PM_WeaponAmmoAvailable(pm->ps->weapon))
				{
					PM_AddEvent(EV_SPINUP);
					PM_StartWeaponAnim(WEAP_ATTACK2);
				}
			}
			else if (pm->ps->weapon == WP_SATCHEL_DET)
			{
				PM_AddEvent(EV_SPINUP);
				PM_ContinueWeaponAnim(WEAP_ATTACK1);
			}

			pm->ps->weaponDelay = GetWeaponTableData(pm->ps->weapon)->fireDelayTime;
		}
		else if (pm->ps->eFlags & EF_PRONE)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, akimboFire ? ANIM_ET_FIREWEAPON2PRONE : ANIM_ET_FIREWEAPONPRONE,
			                   GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_AUTOMATIC, qtrue);
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, akimboFire ? ANIM_ET_FIREWEAPON2 : ANIM_ET_FIREWEAPON,
			                   GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_AUTOMATIC, qtrue);
		}
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if (GetWeaponTableData(pm->ps->weapon)->uses > PM_WeaponAmmoAvailable(pm->ps->weapon))
	{
		PM_AddEvent(EV_NOAMMO);
		PM_ContinueWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon));
		pm->ps->weaponTime += 500;
		pm->ps->weaponDelay = 0;            // avoid unwanted 2nd click firing on delayed weapon attacks

		return;
	}

	if (pm->ps->weaponDelay > 0)
	{
		// if it hits here, the 'fire' has just been hit and the weapon dictated a delay.
		// animations have been started, weaponstate has been set, but no weapon events yet. (except possibly EV_NOAMMO)
		// checks for delayed weapons that have already been fired are return'ed above.
		return;
	}

	// fire weapon

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
			PM_WeaponUseAmmo(pm->ps->weapon, GetWeaponTableData(pm->ps->weapon)->uses);
		}
	}

	// first person weapon animations

	// if this was the last round in the clip, play the 'lastshot' animation
	// this animation has the weapon in a "ready to reload" state
	if (GetWeaponTableData(pm->ps->weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		if (akimboFire)
		{
			weapattackanim = WEAP_ATTACK1;
			PM_AddEvent(EV_FIRE_WEAPON);
		}
		else
		{
			weapattackanim = WEAP_ATTACK2;
			PM_AddEvent(EV_FIRE_WEAPONB);
		}
	}
	else
	{
		if (GetWeaponTableData(pm->ps->weapon)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_RIFLENADE))
		{
			weapattackanim = WEAP_ATTACK2;
			PM_AddEvent(EV_FIRE_WEAPON);
		}
		else if (PM_WeaponClipEmpty(pm->ps->weapon))
		{
			weapattackanim = WEAP_ATTACK_LASTSHOT;
			PM_AddEvent(EV_FIRE_WEAPON_LASTSHOT);
		}
		else
		{
			weapattackanim = WEAP_ATTACK1;
			PM_AddEvent(EV_FIRE_WEAPON);
		}
	}

	// weapon firing animation
	// FG42 is exclude because the continue animation don't look great with it (no recoil, look stuck)
	if ((GetWeaponTableData(pm->ps->weapon)->firingMode & WEAPON_FIRING_MODE_AUTOMATIC) && pm->ps->weapon != WP_FG42 && pm->ps->weapon != WP_FG42_SCOPE)
	{
		PM_ContinueWeaponAnim(weapattackanim);
	}
	else if (!CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))  // no animation for mortar set
	{
		PM_StartWeaponAnim(weapattackanim);
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
	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_SCOPED)
	{
		if (pm->ps->weapon == WP_FG42_SCOPE)
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
	else if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SETTABLE))
	{
		if ((pm->ps->pm_flags & PMF_DUCKED) || (pm->ps->eFlags & EF_PRONE))
		{
			pm->pmext->weapRecoilYaw   = crandom() * .5f;
			pm->pmext->weapRecoilPitch = .45f * random() * .15f;
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_PISTOL)
	{
		if (pm->skill[SK_LIGHT_WEAPONS] >= 3)
		{
			pm->pmext->weapRecoilDuration = 70;
			pm->pmext->weapRecoilPitch    = .25f * random() * .15f;
		}
	}

	// Aim Spread Scale handle
	// add randomness
	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_SMG)
	{
		aimSpreadScaleAdd += rand() % 10;
	}

	// add the recoil amount to the aimSpreadScale
	pm->ps->aimSpreadScaleFloat += 3.0 * aimSpreadScaleAdd;

	if (pm->ps->aimSpreadScaleFloat > AIMSPREAD_MAXSPREAD)
	{
		pm->ps->aimSpreadScaleFloat = AIMSPREAD_MAXSPREAD;
	}

	// covert ops received a reduction of 50% reduction in both recoil jump and weapon sway with Scoped Weapons ONLY
	if ((GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_SCOPED) && pm->skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 3
	    && pm->ps->stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
	{
		pm->ps->aimSpreadScaleFloat *= .5f;
	}

	pm->ps->aimSpreadScale = (int)(pm->ps->aimSpreadScaleFloat);

	if (GetWeaponTableData(pm->ps->weapon)->type & WEAPON_TYPE_MG)
	{
		if (weapattackanim == WEAP_ATTACK_LASTSHOT)
		{
			addTime = 0;
		}
	}
	else if (GetWeaponTableData(pm->ps->weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
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
	if (GetWeaponTableData(pm->ps->weapon)->maxHeat)
	{
		pm->ps->weapHeat[pm->ps->weapon] += GetWeaponTableData(pm->ps->weapon)->nextShotTime;

		// it is overheating
		if (pm->ps->weapHeat[pm->ps->weapon] >= GetWeaponTableData(pm->ps->weapon)->maxHeat)
		{
			pm->ps->weapHeat[pm->ps->weapon] = GetWeaponTableData(pm->ps->weapon)->maxHeat;         // cap heat to max
			PM_AddEvent(EV_WEAP_OVERHEAT);
			//PM_StartWeaponAnim(PM_IdleAnimForWeapon(pm->ps->weapon)); // removed.  client handles anim in overheat event
			addTime = GetWeaponTableData(pm->ps->weapon)->heatRecoveryTime;     // force "heat recovery minimum" right now
		}

		// sync heat for overheat check
		if (GetWeaponTableData(pm->ps->weapon)->weapAlts)
		{
			pm->ps->weapHeat[GetWeaponTableData(pm->ps->weapon)->weapAlts] = pm->ps->weapHeat[pm->ps->weapon];
		}
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
	vec3_t  start, tmins, tmaxs, right, end;
	vec3_t  viewangles;
	trace_t trace;
	int     leaning = 0;        // -1 left, 1 right
	float   leanofs = ps->leanf;

	if (cmd->wbuttons & (WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT))
	{
		if (ps->pm_type == PM_SPECTATOR ||                      // allow spectators to lean while moving
		    ((!cmd->forwardmove && cmd->upmove <= 0) &&         // not allow to lean while moving
		     !BG_PlayerMounted(ps->eFlags) &&                   // not allow to lean while on mg42
		     !(ps->eFlags & EF_FIRING) &&                       // not allow to lean while firing
		     !(ps->eFlags & EF_DEAD) &&                         // not allow to lean while dead
		     !(ps->eFlags & EF_PRONE) &&                        // not allow to lean while prone
		     !(ps->weaponstate == WEAPON_FIRING && ps->weapon == WP_DYNAMITE) && // don't allow to lean while tossing dynamite. NOTE: ATVI Wolfenstein Misc #479 - initial fix to #270 would crash in g_synchronousClients 1 situation
		     !CHECKBITWISE(GetWeaponTableData(ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET)))     // not allow to lean while mortar set
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

		ps->stats[STAT_PS_FLAGS] &= ~(STAT_LEAN_LEFT | STAT_LEAN_RIGHT);

		return; // also early return if already in center position
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
	if (ps->leanf != 0.f && ps->pm_type != PM_SPECTATOR)
	{
		cmd->rightmove = 0;     // also disallowed in cl_input ~391
	}
}

/**
 * @brief This can be used as another entry point when only the viewangles
 * are being updated instead of a full move
 *
 * @param[in,out] ps
 * @param[in,out] pmext
 * @param[in] cmd
 * @param[in] tracemask Take a tracemask as well - we can't use anything out of pm
 *
 * @note Any changes to mounted/prone view should be duplicated in BotEntityWithinView()
 *
 * @note Unused trace parameter
 */
void PM_UpdateViewAngles(playerState_t *ps, pmoveExt_t *pmext, usercmd_t *cmd, void(trace) (trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int tracemask)                      //   modified
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
	else if (CHECKBITWISE(GetWeaponTableData(ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
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
		if (CHECKBITWISE(GetWeaponTableData(ps->weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
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

		// Check if we rotated into a wall with our legs or head, if so, undo yaw
		if (ps->viewangles[YAW] != oldYaw)
		{
			// see if we have the space to go prone
			// we know our main body isn't in a solid, check for our legs then head
			vec3_t start, end;

			BG_LegsCollisionBoxOffset(pm->ps->viewangles, pm->ps->eFlags, start);
			BG_LegsCollisionBoxOffset(oldViewAngles, pm->ps->eFlags, end);

			VectorAdd(pm->ps->origin, start, start);
			VectorAdd(pm->ps->origin, end, end);

			// check the new angles position for legs
			pm->trace(&traceres, start, playerlegsProneMins, playerlegsProneMaxs, end, pm->ps->clientNum, tracemask);

			if (traceres.startsolid)
			{
				VectorSubtract(traceres.endpos, start, traceres.endpos);
				VectorMA(ps->origin, traceres.fraction, traceres.endpos,
				         end);

				// check the new position for head
				PM_TraceHead(&traceres, end, end, NULL,
				             pm->ps->viewangles, pm->trace, pm->ps->clientNum,
				             pm->tracemask);

				if (traceres.fraction != 1.f /* && trace.entityNum >= MAX_CLIENTS */)
				{
					if (pm->debugLevel)
					{
						Com_Printf("%i:can't rotate\n", c_pmove);
					}

					// starting in a solid, no space
					ps->viewangles[YAW]   = oldYaw;
					ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];

					return;
				}

				VectorCopy(end, ps->origin);
			}
			else
			{
				BG_HeadCollisionBoxOffset(pm->ps->viewangles, pm->ps->eFlags, start);
				BG_HeadCollisionBoxOffset(oldViewAngles, pm->ps->eFlags, end);

				VectorAdd(pm->ps->origin, start, start);
				VectorAdd(pm->ps->origin, end, end);

				// check the new angles position for head
				pm->trace(&traceres, start, playerHeadProneMins, playerHeadProneMaxs, end, pm->ps->clientNum, tracemask);

				if (traceres.startsolid)
				{
					// adjust position by bumping
					VectorSubtract(traceres.endpos, start, traceres.endpos);
					VectorMA(ps->origin, traceres.fraction, traceres.endpos,
					         end);

					// check the new position for legs
					PM_TraceLegs(&traceres, &pmext->proneLegsOffset, end, end, NULL,
					             pm->ps->viewangles, pm->trace, pm->ps->clientNum,
					             pm->tracemask);

					if (traceres.fraction != 1.f /* && trace.entityNum >= MAX_CLIENTS */)
					{
						if (pm->debugLevel)
						{
							Com_Printf("%i:can't rotate\n", c_pmove);
						}

						// starting in a solid, no space
						ps->viewangles[YAW]   = oldYaw;
						ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];

						return;
					}

					VectorCopy(end, ps->origin);
				}
			}

			// all fine
			ps->delta_angles[YAW] = newDeltaAngle;
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

	if ((pm->cmd.wbuttons & WBUTTON_ZOOM) && pm->ps->stats[STAT_HEALTH] >= 0 && !pm->ps->weaponDelay && pm->ps->weaponstate == WEAPON_READY)
	{
		if (pm->ps->stats[STAT_KEYS] & (1 << INV_BINOCS))               // binoculars are an inventory item (inventory==keys)
		{
			// don't allow binocs:
			if (!(GetWeaponTableData(pm->ps->weapon)->type & (WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET)) &&  // if using the sniper scope or set weapon
			    !BG_PlayerMounted(pm->ps->eFlags) &&                    // or if mounted on a weapon
			    !(pm->ps->eFlags & EF_PRONE_MOVING) &&                  // when prone moving
			    !pm->ps->grenadeTimeLeft)                               // if in the middle of throwing grenade
			{
				pm->ps->eFlags |= EF_ZOOMING;
			}
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
		if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
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

		if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
		{
			pm->ps->weapon = GetWeaponTableData(pm->ps->weapon)->weapAlts;
		}
		else if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
		{
			pm->ps->weapon = GetWeaponTableData(pm->ps->weapon)->weapAlts;
		}
	}
	else
	{
		if (CHECKBITWISE(GetWeaponTableData(pm->ps->weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
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
