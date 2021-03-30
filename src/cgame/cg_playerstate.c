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
 * @file cg_playerstate.c
 * @brief Acts on changes in a new playerState_t
 *
 * With normal play, this will be done after local prediction, but when
 * following another player or playing back a demo, it will be checked when the
 * snapshot transitions like all the other entities
 */

#include "cg_local.h"

/**
 * @brief CG_DamageFeedback
 * @param[in] yawByte
 * @param[in] pitchByte
 * @param[in] damage
 */
void CG_DamageFeedback(int yawByte, int pitchByte, int damage)
{
	float        kick;
	int          health = cg.snap->ps.stats[STAT_HEALTH];
	float        scale;
	int          slot;
	viewDamage_t *vd;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	if (health < 40)
	{
		scale = 1;
	}
	else
	{
		scale = 40.0f / health;
	}

	kick = Com_Clamp(5, 10, damage * scale);

	// find a free slot
	for (slot = 0; slot < MAX_VIEWDAMAGE; slot++)
	{
		if (cg.viewDamage[slot].damageTime + cg.viewDamage[slot].damageDuration < cg.time)
		{
			break;
		}
	}

	if (slot == MAX_VIEWDAMAGE)
	{
		return;     // no free slots, never override or splats will suddenly disappear
	}

	vd = &cg.viewDamage[slot];

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if (yawByte == 255 && pitchByte == 255)
	{
		vd->damageX    = 0;
		vd->damageY    = 0;
		cg.v_dmg_roll  = 0;
		cg.v_dmg_pitch = -kick;
		cg.v_dmg_angle = -1;
	}
	else
	{
		vec3_t dir;
		vec3_t angles;
		float  left, front, up;

		// positional
		angles[PITCH] = pitchByte / 255.0f * 360;
		angles[YAW]   = yawByte / 255.0f * 360;
		angles[ROLL]  = 0;

		AngleVectors(angles, dir, NULL, NULL);
		VectorSubtract(vec3_origin, dir, dir);

		front = DotProduct(dir, cg.refdef.viewaxis[0]);
		left  = DotProduct(dir, cg.refdef.viewaxis[1]);
		up    = DotProduct(dir, cg.refdef.viewaxis[2]);

		dir[0] = front;
		dir[1] = -left;
		dir[2] = up;

		vectoangles(dir, angles);

		cg.v_dmg_roll  = kick * left;
		cg.v_dmg_pitch = -kick * front;
		cg.v_dmg_angle = angles[YAW];

		vd->damageX = -left;
		vd->damageY = front;
	}

	// don't let the screen flashes vary as much
	if (kick > 10)
	{
		kick = 10;
	}

	vd->damageValue    = kick;
	cg.v_dmg_time      = cg.time + cg_bloodFlashTime.value;
	vd->damageTime     = cg.snap->serverTime;
	vd->damageDuration = (int)(kick * 50 * (1 + 2 * (!vd->damageX && !vd->damageY)));
	cg.damageTime      = cg.snap->serverTime;
	cg.damageIndex     = slot;
}

/**
 * @brief A respawn happened this snapshot
 * @param[in] revived
 */
void CG_Respawn(qboolean revived)
{
	static int oldTeam = -1;
	static int oldCls  = -1;
	cg.serverRespawning = qfalse;   // just in case

	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// need to reset client-side weapon animations
	cg.predictedPlayerState.weapAnim    = ((cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | WEAP_IDLE1;      // reset weapon animations
	cg.predictedPlayerState.weaponstate = WEAPON_READY; // hmm, set this?  what to?

	// display weapons available
	cg.weaponSelectTime = cg.time;

	cg.cursorHintIcon = 0;
	cg.cursorHintTime = 0;

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;

	// clear even more things on respawn
	cg.zoomedBinoc = qfalse;
	cg.zoomed      = qfalse;
	cg.zoomTime    = 0;
	cg.zoomval     = 0;

	trap_SendConsoleCommand("-zoom\n");
	cg.binocZoomTime = 0;

	// ensure scoped weapons are reset after revive
	if (revived)
	{
		if (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_SCOPED)
		{
			CG_FinishWeaponChange(cg.snap->ps.weapon, GetWeaponTableData(cg.snap->ps.weapon)->weapAlts);
		}
	}

	// clear pmext
	Com_Memset(&cg.pmext, 0, sizeof(cg.pmext));

	cg.pmext.bAutoReload = (qboolean)(cg_autoReload.integer > 0);

	cg.pmext.sprintTime = SPRINTTIME;

	if (!revived)
	{
		cgs.limboLoadoutSelected = qfalse;

		// reset switch back weapon
		cg.switchbackWeapon = WP_NONE;
	}

	// Saves the state of sidearm (riflenade weapon is considered as one too)
	// Puts the silencer on if class is COVERTOPS
	// Puts riflenade on if current weapon is riflenade weapon
	if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
	{
		cg.pmext.silencedSideArm = 1;
	}
	else if (GetWeaponTableData(cg.predictedPlayerState.weapon)->type & WEAPON_TYPE_RIFLENADE)
	{
		cg.pmext.silencedSideArm = 2;
	}

	cg.proneMovingTime = 0;

	// reset fog to world fog (if present)
	trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0);

	// try to exec a cfg file if it is found
	if (!revived)
	{
		if ((cgs.clientinfo[cg.clientNum].team == TEAM_AXIS || cgs.clientinfo[cg.clientNum].team == TEAM_ALLIES) && (cgs.clientinfo[cg.clientNum].cls != oldCls))
		{
			CG_execFile(va("autoexec_%s", BG_ClassnameForNumberFilename(cgs.clientinfo[cg.clientNum].cls)));
			oldCls = cgs.clientinfo[cg.clientNum].cls;
		}
		if (cgs.clientinfo[cg.clientNum].team != oldTeam)
		{
			CG_execFile(va("autoexec_%s", BG_TeamnameForNumber(cgs.clientinfo[cg.clientNum].team)));
			oldTeam = cgs.clientinfo[cg.clientNum].team;
		}
	}
}

/**
 * @brief CG_CheckPlayerstateEvents
 * @param[in] ps
 * @param[in] ops
 */
void CG_CheckPlayerstateEvents(playerState_t *ps, playerState_t *ops)
{
	int       i;
	int       event;
	centity_t *cent;

	if (ps->externalEvent && ps->externalEvent != ops->externalEvent)
	{
		cent                         = &cg_entities[ps->clientNum];
		cent->currentState.event     = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for (i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++)
	{
		// if we have a new predictable event
		if (i >= ops->eventSequence
		    // or the server told us to play another event instead of a predicted event we already issued
		    // or something the server told us changed our prediction causing a different event
		    || (i > ops->eventSequence - MAX_EVENTS && ps->events[i & (MAX_EVENTS - 1)] != ops->events[i & (MAX_EVENTS - 1)]))
		{
			event                        = ps->events[i & (MAX_EVENTS - 1)];
			cent->currentState.event     = event;
			cent->currentState.eventParm = ps->eventParms[i & (MAX_EVENTS - 1)];
			CG_EntityEvent(cent, cent->lerpOrigin);

			cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;

			cg.eventSequence++;
		}
	}
}

/*
 * @brief CG_CheckChangedPredictableEvents
 * @param ps
 * @note Unused
void CG_CheckChangedPredictableEvents(playerState_t *ps)
{
    int       i;
    int       event;
    centity_t *cent = &cg.predictedPlayerEntity;

    for (i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++)
    {
        if (i >= cg.eventSequence)
        {
            continue;
        }
        // if this event is not further back in than the maximum predictable events we remember
        if (i > cg.eventSequence - MAX_PREDICTED_EVENTS)
        {
            // if the new playerstate event is different from a previously predicted one
            if (ps->events[i & (MAX_EVENTS - 1)] != cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)])
            {
                event                        = ps->events[i & (MAX_EVENTS - 1)];
                cent->currentState.event     = event;
                cent->currentState.eventParm = ps->eventParms[i & (MAX_EVENTS - 1)];
                CG_EntityEvent(cent, cent->lerpOrigin);

                cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;

                if (cg_showmiss.integer)
                {
                    CG_Printf("WARNING: changed predicted event\n");
                }
            }
        }
    }
}
*/

/**
 * @brief CG_CheckLocalSounds
 * @param[in] ps
 * @param[in] ops
 */
void CG_CheckLocalSounds(playerState_t *ps, playerState_t *ops)
{
	// health changes of more than -1 should make pain sounds
	if (ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1)
	{
		if (ps->stats[STAT_HEALTH] > 0)
		{
			CG_PainEvent(&cg.predictedPlayerEntity, ps->stats[STAT_HEALTH], qfalse);

			cg.painTime = cg.time;
		}
	}

	// timelimit warnings
	if (cgs.timelimit > 0 && cgs.gamestate == GS_PLAYING)
	{
		int msec = cg.time - cgs.levelStartTime;

		if (cgs.timelimit > 5 && !(cg.timelimitWarnings & 1) && (msec > (cgs.timelimit - 5) * 60000) &&
		    (msec < (cgs.timelimit - 5) * 60000 + 1000)) // 60 * 1000
		{
			cg.timelimitWarnings |= 1;
			if (ps->persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.media.fiveMinuteSound_g == -1)
				{
					CG_SoundPlaySoundScript(cg.fiveMinuteSound_g, NULL, -1, qtrue);
				}
				else if (cgs.media.fiveMinuteSound_g)
				{
					trap_S_StartLocalSound(cgs.media.fiveMinuteSound_g, CHAN_ANNOUNCER);
				}
			}
			else if (ps->persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.media.fiveMinuteSound_a == -1)
				{
					CG_SoundPlaySoundScript(cg.fiveMinuteSound_a, NULL, -1, qtrue);
				}
				else if (cgs.media.fiveMinuteSound_a)
				{
					trap_S_StartLocalSound(cgs.media.fiveMinuteSound_a, CHAN_ANNOUNCER);
				}
			}
		}
		if (cgs.timelimit > 2 && !(cg.timelimitWarnings & 2) && (msec > (cgs.timelimit - 2) * 60000) &&
		    (msec < (cgs.timelimit - 2) * 60000 + 1000)) // 60 * 1000
		{
			cg.timelimitWarnings |= 2;
			if (ps->persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.media.twoMinuteSound_g == -1)
				{
					CG_SoundPlaySoundScript(cg.twoMinuteSound_g, NULL, -1, qtrue);
				}
				else if (cgs.media.twoMinuteSound_g)
				{
					trap_S_StartLocalSound(cgs.media.twoMinuteSound_g, CHAN_ANNOUNCER);
				}
			}
			else if (ps->persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.media.twoMinuteSound_a == -1)
				{
					CG_SoundPlaySoundScript(cg.twoMinuteSound_a, NULL, -1, qtrue);
				}
				else if (cgs.media.twoMinuteSound_a)
				{
					trap_S_StartLocalSound(cgs.media.twoMinuteSound_a, CHAN_ANNOUNCER);
				}
			}
		}
		if (!(cg.timelimitWarnings & 4) && (msec > (cgs.timelimit) * 60000 - 30000) &&
		    (msec < cgs.timelimit * 60000 - 29000)) // 60 * 1000
		{
			cg.timelimitWarnings |= 4;
			if (ps->persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.media.thirtySecondSound_g == -1)
				{
					CG_SoundPlaySoundScript(cg.thirtySecondSound_g, NULL, -1, qtrue);
				}
				else if (cgs.media.thirtySecondSound_g)
				{
					trap_S_StartLocalSound(cgs.media.thirtySecondSound_g, CHAN_ANNOUNCER);
				}
			}
			else if (ps->persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.media.thirtySecondSound_a == -1)
				{
					CG_SoundPlaySoundScript(cg.thirtySecondSound_a, NULL, -1, qtrue);
				}
				else if (cgs.media.thirtySecondSound_a)
				{
					trap_S_StartLocalSound(cgs.media.thirtySecondSound_a, CHAN_ANNOUNCER);
				}
			}
		}
	}
}

/**
 * @brief CG_TransitionPlayerState
 * @param[in] ps
 * @param[in] ops
 */
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops)
{
#ifdef FEATURE_MULTIVIEW
	// MV client handling
	if (cg.mvTotalClients > 0)
	{
		if (ps->clientNum != ops->clientNum)
		{
			cg.thisFrameTeleport = qtrue;

			// clear voicechat
			cg.predictedPlayerEntity.voiceChatSpriteTime   = 0; // CHECKME: should we do this here?
			cg_entities[ps->clientNum].voiceChatSpriteTime = 0;

			*ops = *ps;
		}
		CG_CheckLocalSounds(ps, ops);
		return;
	}
#endif

	// check for changing follow mode
	if (ps->clientNum != ops->clientNum)
	{
		cg.thisFrameTeleport = qtrue;

		// clear voicechat
		cg.predictedPlayerEntity.voiceChatSpriteTime   = 0;
		cg_entities[ps->clientNum].voiceChatSpriteTime = 0;

		// make sure we don't get any unwanted transition effects
		*ops = *ps;

		// After Limbo, make sure and do a CG_Respawn
		if (ps->clientNum == cg.clientNum)
		{
			ops->persistant[PERS_SPAWN_COUNT]--;
		}
	}

	if (ps->eFlags & EF_FIRING)
	{
		cg.lastFiredWeaponTime = 0;
		cg.weaponFireTime     += cg.frametime;
	}
	else
	{
		if (cg.weaponFireTime > 500 /*&& cg.weaponFireTime*/)
		{
			cg.lastFiredWeaponTime = cg.time;
		}

		cg.weaponFireTime = 0;
	}

	// damage events (player is getting wounded)
	if (ps->damageEvent != ops->damageEvent && ps->damageCount)
	{
		CG_DamageFeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);
	}

	// respawning
	if (ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT])
	{
		CG_Respawn(ps->persistant[PERS_REVIVE_COUNT] != ops->persistant[PERS_REVIVE_COUNT] ? qtrue : qfalse);
	}

	if (cg.mapRestart)
	{
		CG_Respawn(qfalse);
		cg.mapRestart = qfalse;
	}

	if (cg.snap->ps.pm_type != PM_INTERMISSION
	    && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		CG_CheckLocalSounds(ps, ops);
	}

	if (ps->eFlags & EF_PRONE_MOVING)
	{
		if (ps->weapon == WP_BINOCULARS)
		{
			if (ps->eFlags & EF_ZOOMING)
			{
				trap_SendConsoleCommand("-zoom\n");
			}
		}
		else if (GetWeaponTableData(ps->weapon)->type & WEAPON_TYPE_SCOPED)
		{
			CG_FinishWeaponChange(ps->weapon, GetWeaponTableData(ps->weapon)->weapAlts);
		}

		if (!(ops->eFlags & EF_PRONE_MOVING))
		{
			// this screws up auto-switching when dynamite planted or grenade thrown/out of ammo
			//CG_FinishWeaponChange( cg.weaponSelect, ps->nextWeapon );

			cg.proneMovingTime = cg.time;
		}
	}
	else if (ops->eFlags & EF_PRONE_MOVING)
	{
		cg.proneMovingTime = -cg.time;
	}

	if (!(ps->eFlags & EF_PRONE) && (ops->eFlags & EF_PRONE))
	{
		if (CHECKBITWISE(GetWeaponTableData(cg.weaponSelect)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
		{
			CG_FinishWeaponChange(cg.weaponSelect, ps->nextWeapon);
		}
	}

	// don't let players run with rifles -- speed 80 == crouch, 128 == walk, 256 == run until player start to don't run
	if ((GetWeaponTableData(ps->weapon)->type & WEAPON_TYPE_SCOPED) && VectorLength(ps->velocity) > 127)
	{
		CG_FinishWeaponChange(ps->weapon, GetWeaponTableData(ps->weapon)->weapAlts);
	}

	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	// smooth the ducking viewheight change
	if (ps->viewheight != ops->viewheight)
	{
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime   = cg.time;
		cg.wasProne   = ops->eFlags & EF_PRONE;
	}
}
