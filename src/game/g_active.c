/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
 * @file g_active.c
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

/**
 * @param[in,out] player Player Entity
 *
 * Called just before a snapshot is sent to the given player.
 * Totals up all damage and generates both the player_state_t
 * damage values to that client for pain blends and kicks, and
 * global pain sound events for all clients.
 */
void P_DamageFeedback(gentity_t *player)
{
	gclient_t *client = player->client;
	float     count;

	if (client->ps.pm_type == PM_DEAD)
	{
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood;
	if (count == 0)
	{
		return;     // didn't take any damage
	}

	if (count > 127)
	{
		count = 127;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if (client->damage_fromWorld)
	{
		client->ps.damagePitch = 255;
		client->ps.damageYaw   = 255;

		client->damage_fromWorld = qfalse;
	}
	else
	{
		vec3_t angles;

		vectoangles(client->damage_from, angles);
		client->ps.damagePitch = angles[PITCH] / 360.0 * 256;
		client->ps.damageYaw   = angles[YAW] / 360.0 * 256;
	}

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && !(player->s.powerups & PW_INVULNERABLE))
	{
		player->pain_debounce_time = level.time + 700;
		G_AddEvent(player, EV_PAIN, player->health);
	}

	client->ps.damageEvent++;   // always increment this since we do multiple view damage anims

	client->ps.damageCount = count;

	// clear totals
	client->damage_blood     = 0;
	client->damage_knockback = 0;
}

#define MIN_BURN_INTERVAL 399 // set burn timeinterval so we can do more precise damage (was 199 old model)

/**
 * @brief Check for lava/slime contents and drowning
 * @param[in,out] ent Entity
 */
void P_WorldEffects(gentity_t *ent)
{
	int waterlevel;

	if (ent->client->noclip)
	{
		ent->client->airOutTime = level.time + HOLDBREATHTIME;
		// air left while underwater..
		ent->client->pmext.airleft          = ent->client->airOutTime - level.time;
		ent->client->ps.stats[STAT_AIRLEFT] = HOLDBREATHTIME;
		return;
	}

	waterlevel = ent->waterlevel;

	// check for drowning
	if (waterlevel == 3)
	{
		// if out of air, start drowning
		if (ent->client->airOutTime < level.time)
		{
			// drown!
			ent->client->airOutTime += 1000;
			if (ent->health > 0)
			{
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
				{
					ent->damage = 15;
				}

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage)
				{
					G_Sound(ent, GAMESOUND_PLAYER_BUBBLE);
				}
				else if (rand() & 1)
				{
					G_Sound(ent, GAMESOUND_PLAYER_GURP1);
				}
				else
				{
					G_Sound(ent, GAMESOUND_PLAYER_GURP2);
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage(ent, NULL, NULL, NULL, NULL, ent->damage, 0, MOD_WATER);
			}
		}
	}
	else
	{
		ent->client->airOutTime = level.time + HOLDBREATHTIME;
		// air left while underwater..
		ent->client->pmext.airleft          = ent->client->airOutTime - level.time;
		ent->client->ps.stats[STAT_AIRLEFT] = HOLDBREATHTIME;
		ent->damage                         = 2;
	}

	// check for sizzle damage (move to pmove?)
	if (waterlevel && (ent->watertype & CONTENTS_LAVA))
	{
		if (ent->health > 0 && ent->pain_debounce_time <= level.time)
		{
			if (ent->watertype & CONTENTS_LAVA)
			{
				G_Damage(ent, NULL, NULL, NULL, NULL,
				         30 * waterlevel, 0, MOD_LAVA);
			}
		}
	}

	// check for burning from flamethrower - MP way
	if (ent->s.onFireEnd && ent->client)
	{
		if (level.time - ent->client->lastBurnTime >= MIN_BURN_INTERVAL)
		{
			// server-side incremental damage routine / player damage/health is int (not float)
			// so I can't allocate 1.5 points per server tick, and 1 is too weak and 2 is too strong.
			// solution: allocate damage far less often (MIN_BURN_INTERVAL often) and do more damage.
			// That way minimum resolution (1 point) damage changes become less critical.

			ent->client->lastBurnTime = level.time;
			if ((ent->s.onFireEnd > level.time) && (ent->health > 0))
			{
				gentity_t *attacker;
				attacker = g_entities + ent->flameBurnEnt;
				G_Damage(ent, attacker, attacker, NULL, NULL, 5, DAMAGE_NO_KNOCKBACK, MOD_FLAMETHROWER);   //  was 7
			}
		}
	}
}

/**
 * @brief Disables sound looping in entity
 * @param[in,out] ent Entity
 */
void G_SetClientSound(gentity_t *ent)
{
	ent->s.loopSound = 0;
}

#ifdef FEATURE_OMNIBOT
void PushBot(gentity_t *ent, gentity_t *other)
{
	vec3_t dir, ang, f, r;
	float  oldspeed;
	float  s;

	// dont push when mounted in certain stationary weapons or scripted not to be pushed
	if (other->client)
	{
		if (Bot_Util_AllowPush(other->client->ps.weapon) == qfalse || !other->client->sess.botPush)
		{
			return;
		}
	}

	oldspeed = VectorLength(other->client->ps.velocity);
	if (oldspeed < 200)
	{
		oldspeed = 200;
	}

	VectorSubtract(other->r.currentOrigin, ent->r.currentOrigin, dir);
	VectorNormalize(dir);
	vectoangles(dir, ang);
	AngleVectors(ang, f, r, NULL);
	f[2] = 0;
	r[2] = 0;

	VectorMA(other->client->ps.velocity, 200, f, other->client->ps.velocity);
	s = 100 * ((level.time + (ent->s.number * 1000)) % 4000 < 2000 ? 1.0 : -1.0);
	VectorMA(other->client->ps.velocity, s, r, other->client->ps.velocity);

	if (VectorLengthSquared(other->client->ps.velocity) > Square(oldspeed))
	{
		VectorNormalize(other->client->ps.velocity);
		VectorScale(other->client->ps.velocity, oldspeed, other->client->ps.velocity);
	}
}
#endif

/**
 * @brief Does ent have enough "energy" to call artillery?
 * @param[in] ent Entity
 */
qboolean ReadyToCallArtillery(gentity_t *ent)
{
	if (ent->client->sess.skill[SK_SIGNALS] >= 2)
	{
		if (level.time - ent->client->ps.classWeaponTime <= (level.lieutenantChargeTime[ent->client->sess.sessionTeam - 1] * 0.66f))
		{
			return qfalse;
		}
	}
	else if (level.time - ent->client->ps.classWeaponTime <= level.lieutenantChargeTime[ent->client->sess.sessionTeam - 1])
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Are we ready to construct?
 * @param[in,out] ent           Entity
 * @param[in]     constructible Constructible Entity
 * @param[in]     updateState   Do we want to update Entity weapon time?
 *
 * Optionally, will also update the time while we are constructing
 */
qboolean ReadyToConstruct(gentity_t *ent, gentity_t *constructible, qboolean updateState)
{
	int weaponTime = ent->client->ps.classWeaponTime;

	// "Ammo" for this weapon is time based
	if (weaponTime + level.engineerChargeTime[ent->client->sess.sessionTeam - 1] < level.time)
	{
		weaponTime = level.time - level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
	}

	if (g_debugConstruct.integer)
	{
		weaponTime += 0.5f * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam - 1] / (constructible->constructibleStats.duration / (float)FRAMETIME));
	}
	else
	{
		if (ent->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 3)
		{
			weaponTime += 0.66f * constructible->constructibleStats.chargebarreq * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam - 1] / (constructible->constructibleStats.duration / (float)FRAMETIME));
		}
		//weaponTime += 0.66f*((float)level.engineerChargeTime[ent->client->sess.sessionTeam-1]/(constructible->wait/(float)FRAMETIME));
		//weaponTime += 0.66f * 2.f * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam-1]/(constructible->wait/(float)FRAMETIME));
		else
		{
			weaponTime += constructible->constructibleStats.chargebarreq * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam - 1] / (constructible->constructibleStats.duration / (float)FRAMETIME));
		}
		//weaponTime += 2.f * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam-1]/(constructible->wait/(float)FRAMETIME));
	}

	// if the time is in the future, we have NO energy left
	if (weaponTime > level.time)
	{
		// if we're supposed to update the state, reset the time to now
		//if (updateState)
		//{
		//	ent->client->ps.classWeaponTime = level.time;
		//}

		return qfalse;
	}

	// only set the actual weapon time for this entity if they want us to
	if (updateState)
	{
		ent->client->ps.classWeaponTime = weaponTime;
	}

	return qtrue;
}

//==============================================================

/**
 * @param[in] ent Entity
 * @param[in] pm  Player Move
 */
void ClientImpacts(gentity_t *ent, pmove_t *pm)
{
	int       i, j;
	gentity_t *other;
	trace_t   trace;

	memset(&trace, 0, sizeof(trace));
	for (i = 0 ; i < pm->numtouch ; i++)
	{
		for (j = 0 ; j < i ; j++)
		{
			if (pm->touchents[j] == pm->touchents[i])
			{
				break;
			}
		}

		if (j != i)
		{
			continue;   // duplicated
		}
		other = &g_entities[pm->touchents[i]];

#ifdef FEATURE_OMNIBOT
		if ((ent->client) /*&& !(ent->r.svFlags & SVF_BOT)*/ && (other->r.svFlags & SVF_BOT) &&
		    !other->client->ps.powerups[PW_INVULNERABLE])
		{
			PushBot(ent, other);
		}

		// if we are standing on their head, then we should be pushed also
		if ((ent->r.svFlags & SVF_BOT) && (ent->s.groundEntityNum == other->s.number && other->client) &&
		    !other->client->ps.powerups[PW_INVULNERABLE])
		{
			PushBot(other, ent);
		}
#endif

		if (!other->touch)
		{
			continue;
		}

		other->touch(other, ent, &trace);
	}
}

/**
 * @brief Find all trigger entities that ent's current position touches.
 * @param[in,out] ent Entity
 *
 * Spectators will only interact with teleporters.
 */
void G_TouchTriggers(gentity_t *ent)
{
	int           i, num;
	int           touch[MAX_GENTITIES];
	gentity_t     *hit;
	trace_t       trace;
	vec3_t        mins, maxs;
	static vec3_t range = { 40, 40, 52 };

	if (!ent->client)
	{
		return;
	}

	// reset the pointer that keeps track of trigger_objective_info tracking
	ent->client->touchingTOI = NULL;

	// dead clients don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	VectorSubtract(ent->client->ps.origin, range, mins);
	VectorAdd(ent->client->ps.origin, range, maxs);

	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd(ent->client->ps.origin, ent->r.mins, mins);
	VectorAdd(ent->client->ps.origin, ent->r.maxs, maxs);

	for (i = 0 ; i < num ; i++)
	{
		hit = &g_entities[touch[i]];

		if (!hit->touch && !ent->touch)
		{
			continue;
		}
		if (!(hit->r.contents & CONTENTS_TRIGGER))
		{
			continue;
		}

		// invisible entities can't be touched
		if (hit->entstate == STATE_INVISIBLE ||
		    hit->entstate == STATE_UNDERCONSTRUCTION)
		{
			continue;
		}

		// ignore most entities if a spectator
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			if (hit->s.eType != ET_TELEPORT_TRIGGER)
			{
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if (hit->s.eType == ET_ITEM)
		{
			if (!BG_PlayerTouchesItem(&ent->client->ps, &hit->s, level.time))
			{
				continue;
			}
		}
		else
		{
			// always use capsule for player
			if (!trap_EntityContactCapsule(mins, maxs, hit))
			{
				//if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset(&trace, 0, sizeof(trace));

		if (hit->touch)
		{
			hit->touch(hit, ent, &trace);
		}
	}
}

void SpectatorThink(gentity_t *ent, usercmd_t *ucmd)
{
	gclient_t *client       = ent->client;
	gentity_t *crosshairEnt = crosshairEnt = &g_entities[ent->client->ps.identifyClient];

	// sanity check - check .active in case the client sends us something completely bogus

	if (crosshairEnt->inuse && crosshairEnt->client &&
	    (ent->client->sess.sessionTeam == crosshairEnt->client->sess.sessionTeam ||
	     crosshairEnt->client->ps.powerups[PW_OPS_DISGUISED]))
	{

		// identifyClientHealth sent as unsigned char, so we
		// can't transmit negative numbers
		if (crosshairEnt->health >= 0)
		{
			ent->client->ps.identifyClientHealth = crosshairEnt->health;
		}
		else
		{
			ent->client->ps.identifyClientHealth = 0;
		}
	}

	if (client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		pmove_t pm;

		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed   = 800; // was: 400 // faster than normal
		if (client->ps.sprintExertTime)
		{
			client->ps.speed *= 3;  // allow sprint in free-cam mode
		}
		// dead players are frozen too, in a timeout
		if ((client->ps.pm_flags & PMF_LIMBO) && level.match_pause != PAUSE_NONE)
		{
			client->ps.pm_type = PM_FREEZE;
		}
		else if (client->noclip)
		{
			client->ps.pm_type = PM_NOCLIP;
		}

		// set up for pmove
		memset(&pm, 0, sizeof(pm));
		pm.ps            = &client->ps;
		pm.pmext         = &client->pmext;
		pm.character     = client->pers.character;
		pm.cmd           = *ucmd;
		pm.skill         = client->sess.skill;
		pm.tracemask     = MASK_PLAYERSOLID & ~CONTENTS_BODY; // spectators can fly through bodies
		pm.trace         = trap_TraceCapsuleNoEnts;
		pm.pointcontents = trap_PointContents;

		Pmove(&pm);

		// Activate - made it a latched event (occurs on keydown only)
		if (client->latched_buttons & BUTTON_ACTIVATE)
		{
			Cmd_Activate_f(ent);
		}

		// save results of pmove
		VectorCopy(client->ps.origin, ent->s.origin);

		G_TouchTriggers(ent);
		trap_UnlinkEntity(ent);
	}

	if (ent->flags & FL_NOFATIGUE)
	{
		ent->client->pmext.sprintTime = SPRINTTIME;
	}

	client->oldbuttons = client->buttons;
	client->buttons    = ucmd->buttons;

	client->oldwbuttons = client->wbuttons;
	client->wbuttons    = ucmd->wbuttons;

#ifdef FEATURE_MULTIVIEW
	// MV clients use these buttons locally for other things
	if (client->pers.mvCount < 1)
	{
#endif
	// attack button cycles through spectators
	if ((client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK))
	{
		Cmd_FollowCycle_f(ent, 1);
	}
#ifdef DEBUG
#ifdef FEATURE_OMNIBOT
	// activate button swaps places with bot
	else if (client->sess.sessionTeam != TEAM_SPECTATOR && g_allowBotSwap.integer &&
	         ((client->buttons & BUTTON_ACTIVATE) && !(client->oldbuttons & BUTTON_ACTIVATE)) &&
	         (g_entities[ent->client->sess.spectatorClient].client) &&
	         (g_entities[ent->client->sess.spectatorClient].r.svFlags & SVF_BOT))
	{
		Cmd_SwapPlacesWithBot_f(ent, ent->client->sess.spectatorClient);
	}
#endif
#endif
	else if (
	    (client->sess.sessionTeam == TEAM_SPECTATOR) &&       // don't let dead team players do free fly
	    (client->sess.spectatorState == SPECTATOR_FOLLOW) &&
	    (((client->buttons & BUTTON_ACTIVATE) &&
	      !(client->oldbuttons & BUTTON_ACTIVATE)) || ucmd->upmove > 0) &&
	    G_allowFollow(ent, TEAM_AXIS) && G_allowFollow(ent, TEAM_ALLIES))
	{
		// code moved to StopFollowing
		StopFollowing(ent);
	}
#ifdef FEATURE_MULTIVIEW
}
#endif
}

/**
 * @return Returns qfalse if the client is dropped
 * @param client Client
 *
 * @brief
 * g_inactivity and g_spectatorinactivity :
 * Values have to be higher then 10 seconds, that is the time after the warn message is sent.
 * (if it's lower than that value, it will not work at all)
 */
qboolean ClientInactivityTimer(gclient_t *client)
{
	int      inactivity     = (g_inactivity.integer) ? g_inactivity.integer : 60;
	int      inactivityspec = (g_spectatorInactivity.integer) ? g_spectatorInactivity.integer : 60;
	qboolean inTeam         = (client->sess.sessionTeam == TEAM_ALLIES || client->sess.sessionTeam == TEAM_AXIS) ? qtrue : qfalse;

#ifdef OMNIBOTS
	qboolean doDrop = (g_spectatorInactivity.integer && (g_maxclients.integer - level.numNonSpectatorClients + g_OmniBotPlaying.integer <= 0)) ? qtrue : qfalse;
#else
	qboolean doDrop = (g_spectatorInactivity.integer && (g_maxclients.integer - level.numNonSpectatorClients <= 0)) ? qtrue : qfalse;
#endif

	// inactivity settings disabled?
	if (g_inactivity.integer == 0 && g_spectatorInactivity.integer == 0)
	{
		// Give everyone some time, so if the operator sets g_inactivity or g_spectatorinactivity during
		// gameplay, everyone isn't kicked or moved to spectators
		client->inactivityTime    = level.time + 60000;
		client->inactivityWarning = qfalse;
		return qtrue;
	}

	// the client is still active?
	if (client->pers.cmd.forwardmove || client->pers.cmd.rightmove || client->pers.cmd.upmove ||
	    client->pers.cmd.wbuttons & (WBUTTON_ATTACK2 | WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT)  ||
	    client->pers.cmd.buttons & BUTTON_ATTACK ||
	    client->ps.eFlags & (EF_MOUNTEDTANK | EF_MG42_ACTIVE) ||
	    (client->ps.pm_type == PM_DEAD /*&& !(client->ps.eFlags & EF_PLAYDEAD)*/))     // playdead sets PM_DEAD, so check if playing dead ...
	{
		client->inactivityWarning = qfalse;

		if (inTeam)
		{
			client->inactivityTime = 1000 * inactivity;
		}
		else
		{
			client->inactivityTime = level.time + 1000 * inactivityspec;
		}
		return qtrue;
	}

	// No inactivity for localhost and shrubbot-set clients
	if (client->pers.localClient /*|| G_shrubbot_permission(&g_entities[client-level.clients], SBF_ACTIVITY)*/)    // FIXME: refs?, how to deal with game manager
	{
		return qtrue;
	}

	// start displaying the initial warning when the countdown is halfway (1000/2=500)..
	if (!client->inactivityWarning)
	{
		if (g_inactivity.integer &&
		    (level.time > client->inactivityTime - 500 * inactivity) &&
		    inTeam)
		{
			CPx(client - level.clients, va("cp \"^3%i seconds until moving to spectator for inactivity!\n\"", inactivity));
			CPx(client - level.clients, va("print \"^3%i seconds until moving to spectator for inactivity!\n\"", inactivity));
			G_Printf("Inactivity warning issued to: %s\n", client->pers.netname);
			client->inactivityWarning     = qtrue;
			client->inactivityTime        = level.time + 500 * inactivity;
			client->inactivitySecondsLeft = inactivity;
		}
		// if a player will not be kicked from the server (because there are still free slots),
		// do not display messages for inactivity-drop/kick.
		else if (doDrop && g_spectatorInactivity.integer &&
		         (level.time > client->inactivityTime - 500 * inactivityspec) &&
		         !inTeam)
		{
			CPx(client - level.clients, va("cp \"^3%i seconds until inactivity drop!\n\"", inactivityspec));
			CPx(client - level.clients, va("print \"^3%i seconds until inactivity drop!\n\"", inactivityspec));
			G_Printf("Spectator Inactivity warning issued to: %s\n", client->pers.netname);
			client->inactivityWarning     = qtrue;
			client->inactivityTime        = level.time + 500 * inactivityspec;
			client->inactivitySecondsLeft = inactivityspec;
		}
	}
	else
	{
		if (level.time > client->inactivityTime)
		{
			if (inTeam)
			{
				SetTeam(&g_entities[client - level.clients], "s", qtrue, -1, -1, qfalse);
				client->inactivityWarning     = qfalse;
				client->inactivityTime        = level.time + 1000 * inactivityspec;
				client->inactivitySecondsLeft = inactivityspec;
			}
			else
			{
				// slots occupied by bots should be considered "free",
				// because bots will disconnect if a human player connects..
				if (doDrop)
				{
					trap_DropClient(client - level.clients, "Dropped due to inactivity", 0);
					return qfalse;
				}
			}
		}
		else
		{
			qboolean showMsg     = qfalse;
			int      secondsLeft = (client->inactivityTime - level.time) / 1000;

			// a second has passed since last message was displayed??
			if (secondsLeft != client->inactivitySecondsLeft)
			{
				client->inactivitySecondsLeft = secondsLeft;
				if (secondsLeft <= 0)
				{
					// countdown expired..
					showMsg = qtrue;
				}
				// wait at least half the (inactivity)timeout, before sending new messages.
				// to prevent players seeing messages immediately when they don't move anymore..
				else if (inTeam && g_inactivity.integer && (secondsLeft > (g_inactivity.integer >> 1)))     // integer >>1 == /2 == *0.5
				{
					return qtrue;
				}
				else if (!inTeam && g_spectatorInactivity.integer && (secondsLeft > (g_spectatorInactivity.integer >> 1)))
				{
					return qtrue;
				}
				// display a message every second..
				else if (secondsLeft < 15)
				{
					showMsg = qtrue;
				}
				// display a message every 5 seconds..
				else if (secondsLeft < 30 && secondsLeft % 5 == 0)
				{
					showMsg = qtrue;
				}
				// display a message every 10 seconds..
				else if (secondsLeft < 60 && secondsLeft % 10 == 0)
				{
					showMsg = qtrue;
				}
				// display a message every 30 seconds..
				else if (secondsLeft % 30 == 0)
				{
					showMsg = qtrue;
				}
			}
			if (showMsg)
			{
				// give a few more messages, so people can see the time counting down..
				if (g_inactivity.integer && inTeam)
				{
					if (secondsLeft <= 0)
					{
						CPx(client - level.clients, "cp \"^3moved to spectator for inactivity!\n\"");
					}
					else
					{
						CPx(client - level.clients, va("cp \"^3%i seconds until moving to spectator for inactivity!\n\"", secondsLeft));
					}
				}
				else if (g_spectatorInactivity.integer && !inTeam && doDrop)
				{
					if (secondsLeft <= 0)
					{
						CPx(client - level.clients, "cp \"^3dropped for inactivity!\n\"");
					}
					else
					{
						CPx(client - level.clients, va("cp \"^3%i seconds until inactivity drop!\n\"", secondsLeft));
					}
				}
			}
		}
	}

	// do not kick by default..
	return qtrue;
}

/*
 * @brief Actions that happen once a second
 * @param[in,out] ent  Entity
 * @param         msec Scheduler time
 */
void ClientTimerActions(gentity_t *ent, int msec)
{
	gclient_t *client = ent->client;

	client->timeResidual += msec;

	while (client->timeResidual >= 1000)
	{
		client->timeResidual -= 1000;

		// regenerate
		if (client->sess.playerType == PC_MEDIC)
		{
			if (ent->health < client->ps.stats[STAT_MAX_HEALTH])
			{
				ent->health += 3;
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1)
				{
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
			}
			else if (ent->health < client->ps.stats[STAT_MAX_HEALTH] * 1.12)
			{
				ent->health += 2;
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.12)
				{
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.12;
				}
			}
		}
		else
		{
			// count down health when over max
			if (ent->health > client->ps.stats[STAT_MAX_HEALTH])
			{
				ent->health--;
			}
		}
	}
}

/**
 * @param[in,out] client Client
 */
void ClientIntermissionThink(gclient_t *client)
{
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons    = client->pers.cmd.buttons;

	client->oldwbuttons = client->wbuttons;
	client->wbuttons    = client->pers.cmd.wbuttons;
}

/**
 * @param[in] ent              Pointer to Entity
 * @param     oldEventSequence Old event sequence number
 *
 * Events will be passed on to the clients for presentation,
 * but any server game effects are handled here
 */
void ClientEvents(gentity_t *ent, int oldEventSequence)
{
	int       i;
	int       event;
	gclient_t *client = ent->client;
	int       damage;

	if (oldEventSequence < client->ps.eventSequence - MAX_EVENTS)
	{
		oldEventSequence = client->ps.eventSequence - MAX_EVENTS;
	}
	for (i = oldEventSequence ; i < client->ps.eventSequence ; i++)
	{
		event = client->ps.events[i & (MAX_EVENTS - 1)];

		switch (event)
		{
		case EV_FALL_NDIE:
		//case EV_FALL_SHORT:
		case EV_FALL_DMG_10:
		case EV_FALL_DMG_15:
		case EV_FALL_DMG_25:
		case EV_FALL_DMG_50:
			// VectorClear() used to be done here whenever falling
			// damage occured, but I moved it to bg_pmove where it belongs.

			if (ent->s.eType != ET_PLAYER)
			{
				break;      // not in the player model
			}
			if (event == EV_FALL_NDIE)
			{
				damage = 9999;
			}
			else if (event == EV_FALL_DMG_50)
			{
				damage                    = 50;
				ent->client->ps.pm_time   = 1000;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
			else if (event == EV_FALL_DMG_25)
			{
				damage                    = 25;
				ent->client->ps.pm_time   = 250;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
			else if (event == EV_FALL_DMG_15)
			{
				damage                    = 15;
				ent->client->ps.pm_time   = 1000;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
			else if (event == EV_FALL_DMG_10)
			{
				damage                    = 10;
				ent->client->ps.pm_time   = 1000;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
			else
			{
				damage = 5; // never used
			}
			ent->pain_debounce_time = level.time + 200; // no normal pain sound
			G_Damage(ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			ent->client->pmext.shoved = qfalse;
			break;

		case EV_FIRE_WEAPON_MG42:
			// reset player disguise on stealing docs
			ent->client->ps.powerups[PW_OPS_DISGUISED] = 0;

			mg42_fire(ent);

			// Only 1 stats bin for mg42
#ifndef DEBUG_STATS
			if (g_gamestate.integer == GS_PLAYING)
#endif
			ent->client->sess.aWeaponStats[BG_WeapStatForWeapon(WP_MOBILE_MG42)].atts++;

			break;
		case EV_FIRE_WEAPON_MOUNTEDMG42:
			// reset player disguise on stealing docs
			ent->client->ps.powerups[PW_OPS_DISGUISED] = 0;

			mountedmg42_fire(ent);
			// Only 1 stats bin for mg42
#ifndef DEBUG_STATS
			if (g_gamestate.integer == GS_PLAYING)
#endif
			ent->client->sess.aWeaponStats[BG_WeapStatForWeapon(WP_MOBILE_MG42)].atts++;

			break;

		case EV_FIRE_WEAPON_AAGUN:
			// reset player disguise on stealing docs
			ent->client->ps.powerups[PW_OPS_DISGUISED] = 0;

			aagun_fire(ent);
			break;

		case EV_FIRE_WEAPON:
		case EV_FIRE_WEAPONB:
		case EV_FIRE_WEAPON_LASTSHOT:
			FireWeapon(ent);
			break;

		default:
			break;
		}
	}
}

/**
 * @param[in,out] self Current Player Entity
 */
void WolfFindMedic(gentity_t *self)
{
	int       i, medic = -1;
	gclient_t *cl;
	vec3_t    start, end;
	trace_t   tr;
	float     bestdist = 1024, dist;

	self->client->ps.viewlocked_entNum    = 0;
	self->client->ps.viewlocked           = 0;
	self->client->ps.stats[STAT_DEAD_YAW] = 999;

	VectorCopy(self->s.pos.trBase, start);
	start[2] += self->client->ps.viewheight;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = &level.clients[level.sortedClients[i]];

		if (level.sortedClients[i] == self->client->ps.clientNum)
		{
			continue;
		}

		if (cl->sess.sessionTeam != self->client->sess.sessionTeam)
		{
			continue;
		}

		if (cl->ps.pm_type == PM_DEAD)
		{
			continue;
		}

		// limbo'd players are not PM_DEAD or STAT_HEALTH <= 0.
		// and we certainly don't want to lock to them
		if (cl->ps.pm_flags & PMF_LIMBO)
		{
			continue;
		}

		if (cl->ps.stats[STAT_HEALTH] <= 0)
		{
			continue;
		}

		if (cl->ps.stats[STAT_PLAYER_CLASS] != PC_MEDIC)
		{
			continue;
		}

		VectorCopy(g_entities[level.sortedClients[i]].s.pos.trBase, end);
		end[2] += cl->ps.viewheight;

		trap_Trace(&tr, start, NULL, NULL, end, self->s.number, CONTENTS_SOLID);
		if (tr.fraction < 0.95)
		{
			continue;
		}

		VectorSubtract(end, start, end);
		dist = VectorNormalize(end);

		if (dist < bestdist)
		{
			medic = cl->ps.clientNum;
#if 0 // FIXME: not sure what the point of this is
			vectoangles(end, temp);
			self->client->ps.stats[STAT_DEAD_YAW] = temp[YAW];
#endif
			bestdist = dist;
		}
	}

	if (medic >= 0)
	{
		self->client->ps.viewlocked_entNum = medic;
		self->client->ps.viewlocked        = 7;
	}
}

/**
 * @param[in,out] ent Entity
 *
 * This will be called once for each client frame, which will
 * usually be a couple times for each server frame on fast clients.
 *
 * If "g_synchronousClients 1" is set, this will be called exactly
 * once for each server frame, which makes for smooth demo recording.
 */
void ClientThink_real(gentity_t *ent)
{
	int       msec, oldEventSequence;
	pmove_t   pm;
	usercmd_t *ucmd;
	gclient_t *client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED)
	{
		return;
	}

	if (ent->s.eFlags & EF_MOUNTEDTANK && ent->tagParent)
	{
		client->pmext.centerangles[YAW]   = ent->tagParent->r.currentAngles[YAW];
		client->pmext.centerangles[PITCH] = ent->tagParent->r.currentAngles[PITCH];
	}

	/*if (client->cameraPortal) {
	    G_SetOrigin( client->cameraPortal, client->ps.origin );
	    trap_LinkEntity(client->cameraPortal);
	    VectorCopy( client->cameraOrigin, client->cameraPortal->s.origin2);
	}*/

	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	ent->client->ps.identifyClient = ucmd->identClient;

	// zinx etpro antiwarp
	if (client->warping && g_maxWarp.integer && G_DoAntiwarp(ent))
	{
		int frames = (level.framenum - client->lastUpdateFrame);

		if (frames > g_maxWarp.integer)
		{
			frames = g_maxWarp.integer;
		}
		// if the difference between commandTime and the last command
		// time is small, you won't move as far since it's doing
		// velocity*time for updating your position
		client->ps.commandTime = level.previousTime -
		                         (frames  * (level.time - level.previousTime));
		client->warped = qtrue;
	}

	client->warping         = qfalse;
	client->lastUpdateFrame = level.framenum;
	// end zinx etpro antiwarp

	// sanity check the command time to prevent speedup cheating
	if (ucmd->serverTime > level.time + 200 && !G_DoAntiwarp(ent))
	{
		ucmd->serverTime = level.time + 200;
		//G_Printf("serverTime <<<<<\n" );
	}
	if (ucmd->serverTime < level.time - 1000 && !G_DoAntiwarp(ent))
	{
		ucmd->serverTime = level.time - 1000;
		//G_Printf("serverTime >>>>>\n" );
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if (msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW)
	{
		return;
	}
	if (msec > 200)
	{
		msec = 200;
	}

	// pmove fix
	if (pmove_msec.integer < 8)
	{
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33)
	{
		trap_Cvar_Set("pmove_msec", "33");
	}

	// zinx etpro antiwarp
	client->pers.pmoveMsec = pmove_msec.integer;
	if (G_DoAntiwarp(ent) && (pmove_fixed.integer || client->pers.pmoveFixed))
	{
		ucmd->serverTime = ((ucmd->serverTime + client->pers.pmoveMsec - 1) /
		                    client->pers.pmoveMsec) * client->pers.pmoveMsec;
	}

	if (client->wantsscore)
	{
		G_SendScore(ent);
		client->wantsscore = qfalse;
	}

	// check for exiting intermission
	if (level.intermissiontime)
	{
		ClientIntermissionThink(client);
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	// moved here to allow for spec inactivity checks as well
	if (!ClientInactivityTimer(client))
	{
		return;
	}

	if (!(ent->r.svFlags & SVF_BOT) && level.time - client->pers.lastCCPulseTime > 2000)
	{
		G_SendMapEntityInfo(ent);
		client->pers.lastCCPulseTime = level.time;
	}

	if (!(ucmd->flags & 0x01) || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove || ucmd->wbuttons || ucmd->doubleTap)
	{
		ent->r.svFlags &= ~(SVF_SELF_PORTAL_EXCLUSIVE | SVF_SELF_PORTAL);
	}

	// spectators don't do much
	// In limbo use SpectatorThink
	if (client->sess.sessionTeam == TEAM_SPECTATOR || (client->ps.pm_flags & PMF_LIMBO))
	{
		SpectatorThink(ent, ucmd);
		return;
	}

	if ((client->ps.eFlags & EF_VIEWING_CAMERA) || level.match_pause != PAUSE_NONE)
	{
		ucmd->buttons     = 0;
		ucmd->forwardmove = 0;
		ucmd->rightmove   = 0;
		ucmd->upmove      = 0;
		ucmd->wbuttons    = 0;
		ucmd->doubleTap   = 0;

		// freeze player (RELOAD_FAILED still allowed to move/look)
		if (level.match_pause != PAUSE_NONE)
		{
			client->ps.pm_type = PM_FREEZE;
		}
		else if ((client->ps.eFlags & EF_VIEWING_CAMERA))
		{
			VectorClear(client->ps.velocity);
			client->ps.pm_type = PM_FREEZE;
		}
	}
	else if (client->noclip)
	{
		client->ps.pm_type = PM_NOCLIP;
	}
	else if (client->ps.stats[STAT_HEALTH] <= 0)
	{
		client->ps.pm_type = PM_DEAD;
	}
	else
	{
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.aiState = AISTATE_COMBAT;
	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	if (client->speedScale)                  // Goalitem speed scale
	{
		client->ps.speed *= (client->speedScale * 0.01);
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	client->currentAimSpreadScale = (float)client->ps.aimSpreadScale / 255.0;

	memset(&pm, 0, sizeof(pm));

	pm.ps        = &client->ps;
	pm.pmext     = &client->pmext;
	pm.character = client->pers.character;
	pm.cmd       = *ucmd;
	pm.oldcmd    = client->pers.oldcmd;
	// always use capsule for AI and player
	pm.trace = trap_TraceCapsule;
	if (pm.ps->pm_type == PM_DEAD)
	{
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
		// added: EF_DEAD is checked for in Pmove functions, but wasn't being set until after Pmove
		pm.ps->eFlags |= EF_DEAD;
	}
	else if (pm.ps->pm_type == PM_SPECTATOR)
	{
		pm.trace = trap_TraceCapsuleNoEnts;
	}
	else
	{
		pm.tracemask = MASK_PLAYERSOLID;
	}
	// We've gone back to using normal bbox traces
	//pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel    = g_debugMove.integer;
	pm.noFootsteps   = qfalse;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec  = pmove_msec.integer;

	pm.noWeapClips = qfalse;

	VectorCopy(client->ps.origin, client->oldOrigin);

	pm.gametype           = g_gametype.integer;
	pm.ltChargeTime       = level.lieutenantChargeTime[client->sess.sessionTeam - 1];
	pm.soldierChargeTime  = level.soldierChargeTime[client->sess.sessionTeam - 1];
	pm.engineerChargeTime = level.engineerChargeTime[client->sess.sessionTeam - 1];
	pm.medicChargeTime    = level.medicChargeTime[client->sess.sessionTeam - 1];

	pm.skill = client->sess.skill;

	client->pmext.airleft = ent->client->airOutTime - level.time;

	pm.covertopsChargeTime = level.covertopsChargeTime[client->sess.sessionTeam - 1];

	if (client->ps.pm_type != PM_DEAD && level.timeCurrent - client->pers.lastBattleSenseBonusTime > 45000)
	{
		/*switch (client->combatState)
		{
		case COMBATSTATE_COLD:
		    G_AddSkillPoints(ent, SK_BATTLE_SENSE, 0.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 0.f, "combatstate cold");
		    break;
		case COMBATSTATE_WARM:
		    G_AddSkillPoints(ent, SK_BATTLE_SENSE, 2.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 2.f, "combatstate warm");
		    break;
		case COMBATSTATE_HOT:
		    G_AddSkillPoints(ent, SK_BATTLE_SENSE, 5.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 5.f, "combatstate hot");
		    break;
		case COMBATSTATE_SUPERHOT:
		    G_AddSkillPoints(ent, SK_BATTLE_SENSE, 8.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 8.f, "combatstate super-hot");
		    break;
		}*/

		if (client->combatState != COMBATSTATE_COLD)
		{
			if ((client->combatState & (1 << COMBATSTATE_KILLEDPLAYER)) && (client->combatState & (1 << COMBATSTATE_DAMAGERECEIVED)))
			{
				G_AddSkillPoints(ent, SK_BATTLE_SENSE, 8.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 8.f, "combatstate super-hot");
			}
			else if ((client->combatState & (1 << COMBATSTATE_DAMAGEDEALT)) && (client->combatState & (1 << COMBATSTATE_DAMAGERECEIVED)))
			{
				G_AddSkillPoints(ent, SK_BATTLE_SENSE, 5.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 5.f, "combatstate hot");
			}
			else
			{
				G_AddSkillPoints(ent, SK_BATTLE_SENSE, 2.f); G_DebugAddSkillPoints(ent, SK_BATTLE_SENSE, 2.f, "combatstate warm");
			}
		}

		client->pers.lastBattleSenseBonusTime = level.timeCurrent;
		client->combatState                   = COMBATSTATE_COLD; // cool down again
	}

	// bit hacky, stop the slight lag from client -> server even on locahost, switching back to the weapon you were holding
	// and then back to what weapon you should have, became VERY noticible for the kar98/carbine + gpg40, esp now i've added the
	// animation locking
	if (level.time - client->pers.lastSpawnTime < 1000)
	{
		pm.cmd.weapon = client->ps.weapon;
	}

	Pmove(&pm); // monsterslick

	// fix leaning players bug
	VectorCopy(client->ps.velocity, ent->s.pos.trDelta);
	SnapVector(ent->s.pos.trDelta);

	// server cursor hints
	// bots don't need to check for cursor hints
	if (!(ent->r.svFlags & SVF_BOT) && ent->lastHintCheckTime < level.time)
	{
		G_CheckForCursorHints(ent);

		ent->lastHintCheckTime = level.time + FRAMETIME;
	}

	// Set animMovetype to 1 if ducking
	if (ent->client->ps.pm_flags & PMF_DUCKED)
	{
		ent->s.animMovetype = 1;
	}
	else
	{
		ent->s.animMovetype = 0;
	}

	// save results of pmove
	if (ent->client->ps.eventSequence != oldEventSequence)
	{
		ent->eventTime   = level.time;
		ent->r.eventTime = level.time;
	}

	BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, level.time, qfalse);

	// use the precise origin for linking
	//VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);

	VectorCopy(pm.mins, ent->r.mins);
	VectorCopy(pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype  = pm.watertype;

	// execute client events
	if (level.match_pause == PAUSE_NONE)
	{
		ClientEvents(ent, oldEventSequence);
		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			if (!(ent->client->ps.pm_flags & PMF_TIME_LAND))
			{
				ent->client->pmext.shoved = qfalse;
			}
		}
	}

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity(ent);
	if (!ent->client->noclip)
	{
		G_TouchTriggers(ent);
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

	// touch other objects
	ClientImpacts(ent, &pm);

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence)
	{
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons      = client->buttons;
	client->buttons         = ucmd->buttons;
	client->latched_buttons = client->buttons & ~client->oldbuttons;
	//client->latched_buttons |= client->buttons & ~client->oldbuttons;	// FIXME:? MP method (causes problems for us.  activate 'sticks')

	client->oldwbuttons      = client->wbuttons;
	client->wbuttons         = ucmd->wbuttons;
	client->latched_wbuttons = client->wbuttons & ~client->oldwbuttons;
	//client->latched_wbuttons |= client->wbuttons & ~client->oldwbuttons;	// FIXME:? MP method

	// Activate
	// made it a latched event (occurs on keydown only)
	if (client->latched_buttons & BUTTON_ACTIVATE)
	{
		Cmd_Activate_f(ent);
	}

	if (ent->flags & FL_NOFATIGUE)
	{
		ent->client->pmext.sprintTime = SPRINTTIME;
	}

	if (g_entities[ent->client->ps.identifyClient].team == ent->team && g_entities[ent->client->ps.identifyClient].client)
	{
		ent->client->ps.identifyClientHealth = g_entities[ent->client->ps.identifyClient].health;
	}
	else
	{
		ent->client->ps.identifyClient       = -1;
		ent->client->ps.identifyClientHealth = 0;
	}

#ifdef FEATURE_OMNIBOT
	// Omni-bot: used for class changes, bot will /kill 2 seconds before spawn
	Bot_Util_CheckForSuicide(ent);
#endif

	// check for respawning
	if (client->ps.stats[STAT_HEALTH] <= 0)
	{
		WolfFindMedic(ent);

		// See if we need to hop to limbo
		if (level.timeCurrent > client->respawnTime && !(ent->client->ps.pm_flags & PMF_LIMBO))
		{
			if (ucmd->upmove > 0)
			{
				if (g_gametype.integer == GT_WOLF_LMS || client->ps.persistant[PERS_RESPAWNS_LEFT] >= 0)
				{
					trap_SendServerCommand(ent - g_entities, "reqforcespawn");
				}
				else
				{
					limbo(ent, (client->ps.stats[STAT_HEALTH] > GIB_HEALTH));
				}
			}

			if ((g_forcerespawn.integer > 0 && level.timeCurrent - client->respawnTime > g_forcerespawn.integer * 1000) || client->ps.stats[STAT_HEALTH] <= GIB_HEALTH)
			{
				limbo(ent, (client->ps.stats[STAT_HEALTH] > GIB_HEALTH));
			}
		}

		return;
	}

	if (level.gameManager && level.timeCurrent - client->pers.lastHQMineReportTime > 20000)      // NOTE: 60 seconds? bit much innit
	{
		if (level.gameManager->s.modelindex && client->sess.sessionTeam == TEAM_AXIS)
		{
			if (G_SweepForLandmines(ent->r.currentOrigin, 256.f, TEAM_AXIS))
			{
				client->pers.lastHQMineReportTime = level.timeCurrent;
				trap_SendServerCommand(ent - g_entities, "cp \"Mines have been reported in this area.\" 1");
			}
		}
		else if (level.gameManager->s.modelindex2 && client->sess.sessionTeam == TEAM_ALLIES)
		{
			if (G_SweepForLandmines(ent->r.currentOrigin, 256.f, TEAM_ALLIES))
			{
				client->pers.lastHQMineReportTime = level.timeCurrent;
				trap_SendServerCommand(ent - g_entities, "cp \"Mines have been reported in this area.\" 1");
			}
		}
	}

	// perform once-a-second actions
	if (level.match_pause == PAUSE_NONE)
	{
		ClientTimerActions(ent, msec);
	}
}

/**
 * @brief etpro antiwarp
 * @author zinx
 * @param[in,out] ent Entity
 * @param[in]     cmd User Command
 */
void ClientThink_cmd(gentity_t *ent, usercmd_t *cmd)
{
	ent->client->pers.oldcmd = ent->client->pers.cmd;
	ent->client->pers.cmd    = *cmd;
	ClientThink_real(ent);
}

/**
 * @brief A new command has arrived from the client
 * @param clientNum Client Number from 0 to MAX_CLIENTS
 */
void ClientThink(int clientNum)
{
	gentity_t *ent = g_entities + clientNum;
	usercmd_t newcmd;

	trap_GetUsercmd(clientNum, &newcmd);

#ifdef ALLOW_GSYNC
	if (!g_synchronousClients.integer)
#endif // ALLOW_GSYNC
	{
		if (G_DoAntiwarp(ent))
		{
			// zinx etpro antiwarp
			etpro_AddUsercmd(clientNum, &newcmd);
			DoClientThinks(ent);
		}
		else
		{
			ClientThink_cmd(ent, &newcmd);
		}
	}
}

/**
 * @param[int,out] ent Entity
 */
void G_RunClient(gentity_t *ent)
{
	// special case for uniform grabbing
	if (ent->client->pers.cmd.buttons & BUTTON_ACTIVATE)
	{
		Cmd_Activate2_f(ent);
	}

	if (ent->health <= 0 && (ent->client->ps.pm_flags & PMF_LIMBO))
	{
		if (ent->r.linked)
		{
			trap_UnlinkEntity(ent);
		}
	}

	// adding zinx antiwarp - if we are using antiwarp, then follow the antiwarp way
	if (G_DoAntiwarp(ent))
	{
		// use zinx antiwarp code
		DoClientThinks(ent);
	}

#ifdef ALLOW_GSYNC
	if (!g_synchronousClients.integer)
#endif // ALLOW_GSYNC
	{
		return;
	}
}

/**
 * @param[in,out] ent Spectator Entity
 */
void SpectatorClientEndFrame(gentity_t *ent)
{
#ifdef FEATURE_MULTIVIEW
	// specs periodically get score updates for useful demo playback info
	if (/*ent->client->pers.mvCount > 0 &&*/ ent->client->pers.mvScoreUpdate < level.time)
	{
		ent->client->pers.mvScoreUpdate = level.time + MV_SCOREUPDATE_INTERVAL;
		ent->client->wantsscore         = qtrue;
	}
#endif

	// do this to keep current xp of spectators up to date especially on first connect to get xpsave state in limbo
	if (ent->client->sess.spectatorState == SPECTATOR_FREE)
	{
		int i;
		ent->client->ps.stats[STAT_XP] = 0;

		for (i = 0; i < SK_NUM_SKILLS; ++i)
		{
			ent->client->ps.stats[STAT_XP] += ent->client->sess.skillpoints[i];
		}

		// to avoid overflows for big XP values(>= 32768), count each overflow and add it
		// again in cg_draw.c at display time
		ent->client->ps.stats[STAT_XP_OVERFLOW] = ent->client->ps.stats[STAT_XP] >> 15;     // >>15 == /32768
		ent->client->ps.stats[STAT_XP]          = ent->client->ps.stats[STAT_XP] & 0x7FFF;
	}


	// if we are doing a chase cam or a remote view, grab the latest info
	if ((ent->client->sess.spectatorState == SPECTATOR_FOLLOW) || (ent->client->ps.pm_flags & PMF_LIMBO))
	{
		int       clientNum, testtime;
		gclient_t *cl;
		qboolean  do_respawn = qfalse;

		// Players can respawn quickly in warmup
		if (g_gamestate.integer != GS_PLAYING && ent->client->respawnTime <= level.timeCurrent &&
		    ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			do_respawn = qtrue;
		}
		else if (ent->client->sess.sessionTeam == TEAM_AXIS)
		{
			testtime                            = (level.dwRedReinfOffset + level.timeCurrent - level.startTime) % g_redlimbotime.integer;
			do_respawn                          = (testtime < ent->client->pers.lastReinforceTime);
			ent->client->pers.lastReinforceTime = testtime;
		}
		else if (ent->client->sess.sessionTeam == TEAM_ALLIES)
		{
			testtime                            = (level.dwBlueReinfOffset + level.timeCurrent - level.startTime) % g_bluelimbotime.integer;
			do_respawn                          = (testtime < ent->client->pers.lastReinforceTime);
			ent->client->pers.lastReinforceTime = testtime;
		}

		if (g_gametype.integer != GT_WOLF_LMS)
		{
			if ((g_maxlives.integer > 0 || g_alliedmaxlives.integer > 0 || g_axismaxlives.integer > 0)
			    && ent->client->ps.persistant[PERS_RESPAWNS_LEFT] == 0)
			{
				if (do_respawn)
				{
					if (g_maxlivesRespawnPenalty.integer)
					{
						if (ent->client->ps.persistant[PERS_RESPAWNS_PENALTY] > 0)
						{
							ent->client->ps.persistant[PERS_RESPAWNS_PENALTY]--;
							do_respawn = qfalse;
						}
					}
					else
					{
						do_respawn = qfalse;
					}
				}
			}
		}

		if (g_gametype.integer == GT_WOLF_LMS && g_gamestate.integer == GS_PLAYING)
		{
			// Force respawn in LMS when nobody is playing and we aren't at the timelimit yet
			if (!level.teamEliminateTime &&
			    level.numTeamClients[0] == level.numFinalDead[0] && level.numTeamClients[1] == level.numFinalDead[1] &&
			    ent->client->respawnTime <= level.timeCurrent && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
			{
				do_respawn = qtrue;
			}
			else
			{
				do_respawn = qfalse;
			}
		}

		if (do_respawn)
		{
			reinforce(ent);
			return;
		}

#ifdef FEATURE_MULTIVIEW
		// Limbos aren't following while in MV
		if ((ent->client->ps.pm_flags & PMF_LIMBO) && ent->client->pers.mvCount > 0)
		{
			return;
		}
#endif

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if (clientNum == -1)
		{
			clientNum = level.follow1;
		}
		else if (clientNum == -2)
		{
			clientNum = level.follow2;
		}

		if (clientNum >= 0)
		{
			cl = &level.clients[clientNum];
			if (cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR)
			{
				int flags = (cl->ps.eFlags & ~(EF_VOTED)) | (ent->client->ps.eFlags & (EF_VOTED));
				int ping  = ent->client->ps.ping;

				if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && (ent->client->ps.pm_flags & PMF_LIMBO))
				{
					int savedScore          = ent->client->ps.persistant[PERS_SCORE];
					int savedRespawns       = ent->client->ps.persistant[PERS_RESPAWNS_LEFT];
					int savedRespawnPenalty = ent->client->ps.persistant[PERS_RESPAWNS_PENALTY];
					int savedClass          = ent->client->ps.stats[STAT_PLAYER_CLASS];
#ifdef FEATURE_MULTIVIEW
					int savedMVList = ent->client->ps.powerups[PW_MVCLIENTLIST];
#endif
					do_respawn = ent->client->ps.pm_time;

					ent->client->ps           = cl->ps;
					ent->client->ps.pm_flags |= PMF_FOLLOW;
					ent->client->ps.pm_flags |= PMF_LIMBO;

					ent->client->ps.pm_time                           = do_respawn; // put pm_time back
					ent->client->ps.persistant[PERS_RESPAWNS_LEFT]    = savedRespawns;
					ent->client->ps.persistant[PERS_RESPAWNS_PENALTY] = savedRespawnPenalty;
					ent->client->ps.persistant[PERS_SCORE]            = savedScore; // put score back
#ifdef FEATURE_MULTIVIEW
					ent->client->ps.powerups[PW_MVCLIENTLIST] = savedMVList;
#endif
					ent->client->ps.stats[STAT_PLAYER_CLASS] = savedClass;          //  put player class back
				}
				else
				{
					ent->client->ps           = cl->ps;
					ent->client->ps.pm_flags |= PMF_FOLLOW;
				}

				// carry flags over
				ent->client->ps.eFlags = flags;
				ent->client->ps.ping   = ping;

				return;
			}
			else
			{
				// drop them to free spectators unless they are dedicated camera followers
				if (ent->client->sess.spectatorClient >= 0)
				{
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin(ent->client - level.clients);
				}
			}
		}
	}

	// we are at a free-floating spec state for a player,
	// set speclock status, as appropriate
	//	 --> Can we use something besides a powerup slot?
#ifdef FEATURE_MULTIVIEW
	if (ent->client->pers.mvCount < 1)
	{
#endif
	ent->client->ps.powerups[PW_BLACKOUT] = (G_blockoutTeam(ent, TEAM_AXIS) * TEAM_AXIS) |
	                                        (G_blockoutTeam(ent, TEAM_ALLIES) * TEAM_ALLIES);
#ifdef FEATURE_MULTIVIEW
}
#endif
}


// After reviving a player, their contents stay CONTENTS_CORPSE until it is determine
// to be safe to return them to PLAYERSOLID

/**
 * @param[in,out] self Current Player Entity
 */
qboolean StuckInClient(gentity_t *self)
{
	int       i;
	vec3_t    hitmin, hitmax;
	vec3_t    selfmin, selfmax;
	gentity_t *hit;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		hit = g_entities + level.sortedClients[i];

		if (!hit->inuse || hit == self || !hit->client ||
		    !hit->s.solid || hit->health <= 0)
		{
			continue;
		}

		VectorAdd(hit->r.currentOrigin, hit->r.mins, hitmin);
		VectorAdd(hit->r.currentOrigin, hit->r.maxs, hitmax);
		VectorAdd(self->r.currentOrigin, self->r.mins, selfmin);
		VectorAdd(self->r.currentOrigin, self->r.maxs, selfmax);

		if (hitmin[0] > selfmax[0])
		{
			continue;
		}
		if (hitmax[0] < selfmin[0])
		{
			continue;
		}
		if (hitmin[1] > selfmax[1])
		{
			continue;
		}
		if (hitmax[1] < selfmin[1])
		{
			continue;
		}
		if (hitmin[2] > selfmax[2])
		{
			continue;
		}
		if (hitmax[2] < selfmin[2])
		{
			continue;
		}

		return qtrue;
	}

	return qfalse;
}

extern vec3_t playerMins, playerMaxs;
#define WR_PUSHAMOUNT 25

void WolfRevivePushEnt(gentity_t *self, gentity_t *other)
{
	vec3_t dir, push;

	if (self->props_frame_state == -1)
	{
		return;
	}

	VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, dir);
	dir[2] = 0;
	VectorNormalizeFast(dir);

	VectorScale(dir, WR_PUSHAMOUNT, push);

	if (self->client)
	{
		VectorAdd(self->s.pos.trDelta, push, self->s.pos.trDelta);
		VectorAdd(self->client->ps.velocity, push, self->client->ps.velocity);
	}

	VectorScale(dir, -WR_PUSHAMOUNT, push);
	push[2] = WR_PUSHAMOUNT / 2;

	VectorAdd(other->s.pos.trDelta, push, other->s.pos.trDelta);
	VectorAdd(other->client->ps.velocity, push, other->client->ps.velocity);
}

/**
 * @param[in,out] self Current Player Entity
 * @note completely revived for capsules
 */
void WolfReviveBbox(gentity_t *self)
{
	int       touch[MAX_GENTITIES];
	int       num, i, touchnum = 0;
	gentity_t *hit = G_TestEntityPosition(self);
	vec3_t    mins, maxs;

	if (hit && (hit->s.number == ENTITYNUM_WORLD || (hit->client && (hit->client->ps.persistant[PERS_HWEAPON_USE] || (hit->client->ps.eFlags & EF_MOUNTEDTANK)))))
	{
		G_DPrintf("WolfReviveBbox: Player stuck in world or MG42 using player\n");
		// Move corpse directly to the person who revived them
		if (self->props_frame_state >= 0)
		{
			//trap_UnlinkEntity( self );
			VectorCopy(g_entities[self->props_frame_state].client->ps.origin, self->client->ps.origin);
			VectorCopy(self->client->ps.origin, self->r.currentOrigin);
			trap_LinkEntity(self);

			// Reset value so we don't continue to warp them
			self->props_frame_state = -1;
		}
		return;
	}

	VectorAdd(self->r.currentOrigin, playerMins, mins);
	VectorAdd(self->r.currentOrigin, playerMaxs, maxs);

	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (i = 0 ; i < num ; i++)
	{
		hit = &g_entities[touch[i]];

		// Always use capsule for player
		if (!trap_EntityContactCapsule(mins, maxs, hit))
		{
			//if ( !trap_EntityContact( mins, maxs, hit ) ) {
			continue;
		}

		if (hit->client && hit->health > 0)
		{
			if (hit->s.number != self->s.number)
			{
				WolfRevivePushEnt(hit, self);
				touchnum++;
			}
		}
		else if (hit->r.contents & (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_PLAYERCLIP))
		{
			WolfRevivePushEnt(hit, self);
			touchnum++;
		}
	}

	G_DPrintf("WolfReviveBbox: Touchnum: %d\n", touchnum);

	if (touchnum == 0)
	{
		G_DPrintf("WolfReviveBbox:  Player is solid now!\n");
		self->r.contents = CONTENTS_BODY;
	}
}

/**
 * @param[in,out] ent Entity
 *
 * Called at the end of each server frame for each connected client
 * A fast client will have multiple ClientThink for each ClientEndFrame,
 * while a slow client may have multiple ClientEndFrame between ClientThink.
 */
void ClientEndFrame(gentity_t *ent)
{
	int i;
	int frames;

	// used for informing of speclocked teams.
	// Zero out here and set only for certain specs
	ent->client->ps.powerups[PW_BLACKOUT] = 0;

	if ((ent->client->sess.sessionTeam == TEAM_SPECTATOR) || (ent->client->ps.pm_flags & PMF_LIMBO))
	{
		SpectatorClientEndFrame(ent);
		return;
	}

	// turn off any expired powerups
	// range changed for MV
	for (i = 0 ; i < PW_NUM_POWERUPS ; i++)
	{

		if (i == PW_FIRE ||                 // these aren't dependant on level.time
		    i == PW_ELECTRIC ||
		    i == PW_BREATHER ||
		    i == PW_NOFATIGUE ||
		    ent->client->ps.powerups[i] == 0
		    || i == PW_OPS_CLASS_1
		    || i == PW_OPS_CLASS_2
		    || i == PW_OPS_CLASS_3
		    || i == PW_OPS_DISGUISED
		    )
		{
			continue;
		}
		// If we're paused, update powerup timers accordingly.
		// Make sure we dont let stuff like CTF flags expire.
		if (level.match_pause != PAUSE_NONE &&
		    ent->client->ps.powerups[i] != INT_MAX)
		{
			ent->client->ps.powerups[i] += level.time - level.previousTime;
		}


		if (ent->client->ps.powerups[i] < level.time)
		{
			ent->client->ps.powerups[i] = 0;
		}
	}

	ent->client->ps.stats[STAT_XP] = 0;
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		ent->client->ps.stats[STAT_XP] += ent->client->sess.skillpoints[i];
	}

	// to avoid overflows for big XP values(>= 32768), count each overflow and add it
	// again in cg_draw.c at display time
	ent->client->ps.stats[STAT_XP_OVERFLOW] = ent->client->ps.stats[STAT_XP] >> 15;     // >>15 == /32768
	ent->client->ps.stats[STAT_XP]          = ent->client->ps.stats[STAT_XP] & 0x7FFF;  // & 0x7FFF == %32768

	// If we're paused, make sure other timers stay in sync
	//		--> Any new things in ET we should worry about?
	if (level.match_pause != PAUSE_NONE)
	{
		int time_delta = level.time - level.previousTime;

		ent->client->airOutTime         += time_delta;
		ent->client->inactivityTime     += time_delta;
		ent->client->lastBurnTime       += time_delta;
		ent->client->pers.connectTime   += time_delta;
		ent->client->pers.enterTime     += time_delta;
		ent->client->ps.classWeaponTime += time_delta;
		//ent->client->respawnTime += time_delta;
		ent->lastHintCheckTime  += time_delta;
		ent->pain_debounce_time += time_delta;
		ent->s.onFireEnd        += time_delta;
	}

	// save network bandwidth
#if 0
	if (!g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL)
	{
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear(ent->client->ps.viewangles);
	}
#endif

	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	if (level.intermissiontime)
	{
		return;
	}

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// mark as not missing updates initially
	ent->client->ps.eFlags &= ~EF_CONNECTION;

	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	G_SetClientSound(ent);

	// set the latest infor

	BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, level.time, qfalse);

	// If it's been a couple frames since being revived, and props_frame_state
	// wasn't reset, go ahead and reset it
	if (ent->props_frame_state >= 0 && ((level.time - ent->s.effect3Time) > 100))
	{
		ent->props_frame_state = -1;
	}

	if (ent->health > 0 && StuckInClient(ent))
	{
		G_DPrintf("%s is stuck in a client.\n", ent->client->pers.netname);
		ent->r.contents = CONTENTS_CORPSE;
	}

	if (ent->health > 0 && ent->r.contents == CONTENTS_CORPSE && !(ent->s.eFlags & EF_MOUNTEDTANK))
	{
		WolfReviveBbox(ent);
	}

	// Reset 'count2' for flamethrower
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->count2 = 0;
	}

	// run touch functions here too, so movers don't have to wait
	// until the next ClientThink, which will be too late for some map
	// scripts (railgun)
	G_TouchTriggers(ent);

	// run entity scripting
	G_Script_ScriptRun(ent);

	// zinx etpro antiwarp
	frames = level.framenum - ent->client->lastUpdateFrame - 1;
	if (g_maxWarp.integer && frames > g_maxWarp.integer && G_DoAntiwarp(ent))
	{
		ent->client->warping = qtrue;
	}
	ent->client->warped = qfalse;

	// store the client's current position for antilag traces
	G_StoreClientPosition(ent);
}
