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
 * @file g_trigger.c
 */

#include "g_local.h"

/**
 * @brief InitTrigger
 * @param[in,out] self
 */
void InitTrigger(gentity_t *self)
{
	if (!VectorCompare(self->s.angles, vec3_origin))
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	if (self->model)
	{
		trap_SetBrushModel(self, self->model);
	}
	else
	{
		// empty models for ETPro mapscripting
		G_DPrintf("^6InitTrigger: trap_SetBrushModel(NULL) skipped for scriptName %s\n", self->scriptName);
	}

	self->r.contents = CONTENTS_TRIGGER;        // replaces the -1 from trap_SetBrushModel
	self->r.svFlags  = SVF_NOCLIENT;
}

/**
 * @brief The wait time has passed, so set back up for another activation
 * @param[out] ent
 */
void multi_wait(gentity_t *ent)
{
	ent->nextthink = 0;
}

/**
 * @brief The trigger was just activated
 *
 * @details ent->activator should be set to the activator so it can be held through a delay
 * so wait for the delay time before firing
 *
 * @param[in,out] ent
 * @param[in] activator
 */
void multi_trigger(gentity_t *ent, gentity_t *activator)
{
	ent->activator = activator;

	if (ent->numPlayers > 1)
	{
		gentity_t *tnt;   // temp ent for counting players
		int       i;
		int       entList[MAX_GENTITIES];   // list of entities
		int       cnt     = trap_EntitiesInBox(ent->r.mins, ent->r.maxs, entList, MAX_GENTITIES);
		int       players = 0;   // number of ents in trigger

		for (i = 0; i < cnt; i++)
		{
			tnt = &g_entities[entList[i]];

			if (tnt->client)
			{
				players++;
			}
		}

		// not enough players, return
		if (players < ent->numPlayers)
		{
			return;
		}
	}

	G_Script_ScriptEvent(ent, "activate", activator->client->sess.sessionTeam == TEAM_AXIS ? "axis" : "allies");

	if (ent->nextthink)
	{
		return;     // can't retrigger until the wait is over
	}

	G_UseTargets(ent, ent->activator);

	if (ent->wait > 0)
	{
		ent->think     = multi_wait;
		ent->nextthink = (int)(level.time + (ent->wait + ent->random * crandom()) * 1000);
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch     = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think     = G_FreeEntity;
	}
}

/**
 * @brief Use_Multi
 * @param[in] ent
 * @param other - unused
 * @param[in] activator
 */
void Use_Multi(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	multi_trigger(ent, activator);
}

/**
 * @brief Touch_Multi
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void Touch_Multi(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (!other->client)
	{
		return;
	}

	if (self->spawnflags & MULTI_TRIGGER_AXIS_ONLY)
	{
		if (other->client->sess.sessionTeam != TEAM_AXIS)
		{
			return;
		}
	}
	else if (self->spawnflags & MULTI_TRIGGER_ALLIED_ONLY)
	{
		if (other->client->sess.sessionTeam != TEAM_ALLIES)
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_NOBOT)
	{
		if (other->r.svFlags & SVF_BOT)
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_BOTONLY)
	{
		if (!(other->r.svFlags & SVF_BOT))
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_SOLDIERONLY)
	{
		if (!(other->client->sess.playerType == PC_SOLDIER))
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_FIELDOPSONLY)
	{
		if (!(other->client->sess.playerType == PC_FIELDOPS))
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_MEDICONLY)
	{
		if (!(other->client->sess.playerType == PC_MEDIC))
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_ENGINEERONLY)
	{
		if (!(other->client->sess.playerType == PC_ENGINEER))
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_COVERTOPSONLY)
	{
		if (!(other->client->sess.playerType == PC_COVERTOPS))
		{
			return;
		}
	}

	// mod specific spawnflags

	if (self->spawnflags & MULTI_TRIGGER_DISGUISEDSONLY)
	{
		if (!(other->client->ps.powerups[PW_OPS_DISGUISED]))
		{
			return;
		}
	}

	if (self->spawnflags & MULTI_TRIGGER_OBJECTIVEONLY)
	{
		if (!(other->client->ps.powerups[PW_BLUEFLAG] || other->client->ps.powerups[PW_REDFLAG]))
		{
			return;
		}
	}

	multi_trigger(self, other);
}

/**
 * @brief SP_trigger_multiple
 * @details QUAKED trigger_multiple (.5 .5 .5) ? AXIS_ONLY ALLIED_ONLY NOBOT BOTONLY SOLDIERONLY LTONLY MEDICONLY ENGINEERONLY COVERTOPSONLY
 * "wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
 * "random"	wait variance, default is 0
 * Variable sized repeatable trigger.  Must be targeted at one or more entities.
 * so, the basic time between firing is a random time between
 * (wait - random) and (wait + random)
 *
 * @param[in,out] ent
 */
void SP_trigger_multiple(gentity_t *ent)
{
	G_SpawnFloat("wait", "0.5", &ent->wait);
	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnInt("numPlayers", "1", &ent->numPlayers);

	if (ent->random >= ent->wait && ent->wait >= 0)
	{
		ent->random = ent->wait - (FRAMETIME * 0.001f);
		G_Printf("trigger_multiple has random >= wait\n");
	}

	ent->touch   = Touch_Multi;
	ent->use     = Use_Multi;
	ent->s.eType = ET_TRIGGER_MULTIPLE;

	InitTrigger(ent);

#ifdef VISIBLE_TRIGGERS
	ent->r.svFlags &= ~SVF_NOCLIENT;
#endif // VISIBLE_TRIGGERS

	trap_LinkEntity(ent);
}

/*
==============================================================================
trigger_always
==============================================================================
*/

/**
 * @brief trigger_always_think
 * @param[in] ent
 */
void trigger_always_think(gentity_t *ent)
{
	G_UseTargets(ent, ent);
	G_FreeEntity(ent);
}

/**
 * @brief This trigger will always fire. It is activated by the world.
 * @details QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
 * @param ent
 */
void SP_trigger_always(gentity_t *ent)
{
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think     = trigger_always_think;
}

/*
==============================================================================
trigger_push
==============================================================================
*/

/*
 * @brief trigger_push_touch
 * @param self  - unused
 * @param other - unused
 * @param trace - unused
 *
 * @note Unused
void trigger_push_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
}
*/

/**
 * @brief Calculate origin2 so the target apogee will be hit
 * @param[in,out] self
 */
void AimAtTarget(gentity_t *self)
{
	gentity_t *ent;
	vec3_t    origin;
	float     height, gravity, time, forward;
	float     dist;

	VectorAdd(self->r.absmin, self->r.absmax, origin);
	VectorScale(origin, 0.5f, origin);

	ent = G_PickTarget(self->target);
	if (!ent)
	{
		G_FreeEntity(self);
		return;
	}

	height  = ent->s.origin[2] - origin[2];
	gravity = g_gravity.value;
	time    = (float)(sqrt(Q_fabs(height / (0.5f * gravity))));
	if (time == 0.f)
	{
		G_FreeEntity(self);
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract(ent->s.origin, origin, self->s.origin2);
	self->s.origin2[2] = 0;
	dist               = VectorNormalize(self->s.origin2);

	forward = dist / time;
	VectorScale(self->s.origin2, forward, self->s.origin2);

	self->s.origin2[2] = time * gravity;
}

/**
 * @brief SP_trigger_push
 *
 * @details QUAKED trigger_push (.5 .5 .5) ? TOGGLE REMOVEAFTERTOUCH PUSHPLAYERONLY
 * Must point at a target_position, which will be the apex of the leap.
 * This will be client side predicted, unlike target_push
 *
 * @param self - unused
 */
void SP_trigger_push(gentity_t *self)
{
	G_Printf("trigger_push has no effect. Please delete it.\n");
}

/**
 * @brief Use_target_push
 * @param[in] self
 * @param other - unused
 * @param[in,out] activator
 */
void Use_target_push(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (!activator->client)
	{
		return;
	}

	if (activator->client->ps.pm_type != PM_NORMAL)
	{
		return;
	}

	VectorCopy(self->s.origin2, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if (activator->fly_sound_debounce_time < level.time)
	{
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound(activator, self->noise_index);
	}
}

/**
 * @brief SP_target_push
 *
 * @details QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
 * Pushes the activator in the direction.of angle, or towards a target apex.
 * "speed"		defaults to 1000
 * if "bouncepad", play bounce noise instead of windfly
 *
 * @param[in,out] self
 */
void SP_target_push(gentity_t *self)
{
	if (self->speed == 0.f)
	{
		self->speed = 1000;
	}
	G_SetMovedir(self->s.angles, self->s.origin2);
	VectorScale(self->s.origin2, self->speed, self->s.origin2);

	if (self->spawnflags & TARGET_PUSH_BOUNCEPAD)
	{
		self->noise_index = G_SoundIndex("sound/world/jumppad.wav");
	}
	else
	{
		self->noise_index = G_SoundIndex("sound/weapons/impact/flesh1.wav"); // was sound/misc/windfly.wav and not in path
	}
	if (self->target)
	{
		VectorCopy(self->s.origin, self->r.absmin);
		VectorCopy(self->s.origin, self->r.absmax);
		self->think     = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

/*
==============================================================================
trigger_teleport
==============================================================================
*/

/**
 * @brief trigger_teleporter_touch
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void trigger_teleporter_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t *dest;

	if (!other->client)
	{
		return;
	}
	if (other->client->ps.pm_type == PM_DEAD)
	{
		return;
	}

	dest = G_PickTarget(self->target);
	if (!dest)
	{
		G_Printf("Couldn't find teleporter destination '%s'\n", self->target);
		return;
	}

	TeleportPlayer(other, dest->s.origin, dest->s.angles);
}

/**
 * @brief QUAKED trigger_teleport (.5 .5 .5) ?
 * Allows client side prediction of teleportation events.
 * Must point at a target_position, which will be the teleport destination.
 *
 * @param[out] self
 */
void SP_trigger_teleport(gentity_t *self)
{
	InitTrigger(self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex("sound/world/jumppad.wav");

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch   = trigger_teleporter_touch;

	trap_LinkEntity(self);
}

/*
==============================================================================
trigger_hurt
==============================================================================
*/

/**
 * @brief hurt_touch
 *
 * @details QUAKED trigger_hurt (.5 .5 .5) ? START_OFF - SILENT NO_PROTECTION SLOW ONCE
 * Any entity that touches this will be hurt.
 * It does dmg points of damage each server frame
 * Targeting the trigger will toggle its on / off state.
 *
 * SILENT			supresses playing the sound
 * SLOW			changes the damage rate to once per second
 * NO_PROTECTION	*nothing* stops the damage
 *
 * "dmg"			default 5 (whole numbers only)
 *
 * "life"	time this brush will exist if value is zero will live for ever ei 0.5 sec 2.sec
 * default is zero
 *
 * the entity must be used first before it will count down its life
 *
 * @param[in,out] self
 * @param[in] other
 * @param trace - unused
 */
void hurt_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int dflags;

	if (!other->takedamage)
	{
		return;
	}

	if (self->timestamp > level.time)
	{
		return;
	}

	if (self->spawnflags & 16)
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if (!(self->spawnflags & 4))
	{
		G_Sound(other, self->noise_index);
	}

	if (self->spawnflags & 8)
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}
	G_Damage(other, self, self, NULL, NULL, self->damage, dflags, MOD_TRIGGER_HURT);

	if (self->spawnflags & 32)
	{
		self->touch = NULL;
	}
}

/**
 * @brief hurt_think
 * @param[in,out] ent
 */
void hurt_think(gentity_t *ent)
{
	ent->nextthink = level.time + FRAMETIME;

	if (ent->wait < level.time)
	{
		G_FreeEntity(ent);
	}
}

/**
 * @brief hurt_use
 * @param[in,out] self
 * @param other     - unused
 * @param activator - unused
 */
void hurt_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (self->touch)
	{
		self->touch = NULL;
	}
	else
	{
		self->touch = hurt_touch;
	}

	if (self->delay != 0.f)
	{
		self->nextthink = level.time + 50;
		self->think     = hurt_think;
		self->wait      = level.time + (self->delay * 1000);
	}
}

/**
 * @brief SP_trigger_hurt
 * @param[in,out] self
 */
void SP_trigger_hurt(gentity_t *self)
{
	char  *life, *sound;
	float dalife;

	InitTrigger(self);

	G_SpawnString("sound", "sound/player/hurt_barbwire.wav", &sound);

	self->noise_index = G_SoundIndex(sound);

	if (!self->damage)
	{
		self->damage = 5;
	}

	self->use = hurt_use;

	// link in to the world if starting active
	if (!(self->spawnflags & 1))
	{
		self->touch = hurt_touch;
	}

	G_SpawnString("life", "0", &life);
	dalife      = (float)(atof(life));
	self->delay = dalife;
}

/*
==============================================================================
trigger_heal
==============================================================================
*/

/**
 * @brief G_IsAllowedHeal
 *
 * @details QUAKED trigger_heal (.5 .5 .5) ?
 * Any entity that touches this will be healed at a specified rate up to a specified
 * maximum.
 *
 * "healrate"		rate of healing per second, default 5 (whole numbers only)
 * "healtotal"		the maximum of healing this trigger can do. if <= 0, it's unlimited.
 *                 default 0 (whole numbers only)
 * "target"		cabinet that this entity is linked to
 *
 * @param ent
 * @return
 */
qboolean G_IsAllowedHeal(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return qfalse;
	}

	if (ent->health <= 0)
	{
		return qfalse;
	}

	if (ent->client->ps.stats[STAT_HEALTH] >= ent->client->ps.stats[STAT_MAX_HEALTH])
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief heal_touch
 * @param[in,out] self
 * @param[in] other
 * @param trace - unused
 */
void heal_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int       i, j, clientcount = 0;
	gentity_t *touchClients[MAX_CLIENTS];
	int       healvalue;

	Com_Memset(touchClients, 0, sizeof(gentity_t *) * MAX_CLIENTS);

	if (!other->client)
	{
		return;
	}

	if (self->timestamp > level.time)
	{
		return;
	}

	if (self->target_ent && self->target_ent->s.eType == ET_CABINET_H)
	{
		if (other->client->pers.autoActivate == PICKUP_ACTIVATE)
		{
			return;
		}

		if (other->client->pers.autoActivate == PICKUP_FORCE)            // autoactivate probably forced by the "Cmd_Activate_f()" function
		{
			other->client->pers.autoActivate = PICKUP_ACTIVATE;          // so reset it.
		}
	}

	self->timestamp = level.time + 1000;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (trap_EntityContactCapsule(g_entities[j].r.absmin, g_entities[j].r.absmax, self) && G_IsAllowedHeal(&g_entities[j]))
		{
			touchClients[clientcount] = &g_entities[j];
			clientcount++;
		}
	}

	if (clientcount == 0)
	{
		return;
	}

	for (i = 0; i < clientcount; i++)
	{
		healvalue = MIN(touchClients[i]->client->ps.stats[STAT_MAX_HEALTH] - touchClients[i]->health, self->damage);

		if (self->health != -9999)
		{
			healvalue = MIN(healvalue, self->health);
		}
		if (healvalue <= 0)
		{
			continue;
		}

		touchClients[i]->health += healvalue;
		// add the medicheal event (to get sound, etc.)
		G_AddPredictableEvent(other, EV_ITEM_PICKUP, ITEM_HEALTH_CABINET);

		if (self->health != -9999)
		{
			self->health -= healvalue;
		}
	}
}

#define HEALTH_REGENTIME 10000

/**
 * @brief trigger_heal_think
 * @param[in,out] self
 */
void trigger_heal_think(gentity_t *self)
{
	self->nextthink = level.time + HEALTH_REGENTIME;
	self->health   += self->damage;

	if (self->health > self->count)
	{
		self->health = self->count;
	}
}

#define TRIGGER_HEAL_CANTHINK(self) self->count != -9999

/**
 * @brief trigger_heal_setup
 * @param[in,out] self
 */
void trigger_heal_setup(gentity_t *self)
{
	self->target_ent = G_FindByTargetname(NULL, self->target);
	if (!self->target_ent)
	{
		G_Error("trigger_heal failed to find target: %s\n", self->target);
	}

	self->target_ent->parent = self;

	if (TRIGGER_HEAL_CANTHINK(self))
	{
		self->think     = trigger_heal_think;
		self->nextthink = level.time + FRAMETIME;
	}
}

/**
 * @brief SP_misc_cabinet_health
 * @details QUAKED misc_cabinet_health (.5 .5 .5) (-20 -20 0) (20 20 60)
 * @param[out] self
 */
void SP_misc_cabinet_health(gentity_t *self)
{
	VectorSet(self->r.mins, -20, -20, 0);
	VectorSet(self->r.maxs, 20, 20, 60);

	G_SetOrigin(self, self->s.origin);
	G_SetAngle(self, self->s.angles);

	self->s.eType = ET_CABINET_H;

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;

	trap_LinkEntity(self);
}

/**
 * @brief SP_trigger_heal
 * @param[in,out] self
 */
void SP_trigger_heal(gentity_t *self)
{
	char *spawnstr;
	int  healvalue;

	InitTrigger(self);

	self->touch = heal_touch;

	// healtotal specifies the maximum amount of health this trigger area restores
	G_SpawnString("healtotal", "0", &spawnstr);
	healvalue = Q_atoi(spawnstr);
	// -9999 means infinite now
	self->health = healvalue;
	if (self->health <= 0)
	{
		self->health = -9999;
	}
	self->count   = self->health;
	self->s.eType = ET_HEALER;

	self->target_ent = NULL;
	if (self->target && *self->target)
	{
		self->think     = trigger_heal_setup;
		self->nextthink = level.time + FRAMETIME;
	}
	else if (TRIGGER_HEAL_CANTHINK(self))
	{
		self->think     = trigger_heal_think;
		self->nextthink = level.time + HEALTH_REGENTIME;
	}

	// healrate specifies the amount of healing per second
	G_SpawnString("healrate", "20", &spawnstr);
	healvalue    = Q_atoi(spawnstr);
	self->damage = healvalue;   // store the rate of heal in damage
}

/*
==============================================================================
trigger_ammo
==============================================================================
*/

/**
 * @brief G_IsAllowedAmmo
 *
 * @details QUAKED trigger_ammo (.5 .5 .5) ?
 * Any entity that touches this will get additional ammo a specified rate up to a
 * specified maximum.
 *
 * "ammorate"		rate of ammo clips per second. default 1. (whole number only)
 * "ammototal"		the maximum clips of ammo this trigger can add. if <= 0, it's unlimited.
 *                 default 0 (whole numbers only)
 * "target"		cabinet that this entity is linked to
 *
 * @param[in] ent
 *
 * @return
 */
qboolean G_IsAllowedAmmo(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return qfalse;
	}

	if (ent->health < 0)
	{
		return qfalse;
	}

	if (!AddMagicAmmo(ent, 0))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief ammo_touch
 * @param[in,out] self
 * @param[in] other
 * @param trace - unused
 */
void ammo_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int       i, j, clientcount = 0, count;
	gentity_t *touchClients[MAX_CLIENTS];

	Com_Memset(touchClients, 0, sizeof(gentity_t *) * MAX_CLIENTS);

	if (other->client == NULL)
	{
		return;
	}

	// flags is for the last entity number that got ammo
	if (self->timestamp > level.time)
	{
		return;
	}
	self->timestamp = level.time + 1000;

	if (self->target_ent && self->target_ent->s.eType == ET_CABINET_A)
	{
		if (other->client->pers.autoActivate == PICKUP_ACTIVATE)
		{
			return;
		}

		if (other->client->pers.autoActivate == PICKUP_FORCE)            // autoactivate probably forced by the "Cmd_Activate_f()" function
		{
			other->client->pers.autoActivate = PICKUP_ACTIVATE;          // so reset it.
		}
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (trap_EntityContactCapsule(g_entities[j].r.absmin, g_entities[j].r.absmax, self) && G_IsAllowedAmmo(&g_entities[j]))
		{
			touchClients[clientcount] = &g_entities[j];
			clientcount++;
		}
	}

	if (clientcount == 0)
	{
		return;
	}

	// if low, just give out what's left
	if (self->health == -9999)
	{
		count = clientcount;
	}
	else
	{
		count = (int)(MIN(clientcount, self->health / (float)self->damage));
	}

	for (i = 0; i < count; i++)
	{
		// self->damage contains the amount of ammo to add
		if (AddMagicAmmo(touchClients[i], self->damage))
		{
			// add the ammo pack event (to get sound, etc.)
			G_AddPredictableEvent(touchClients[i], EV_ITEM_PICKUP, ITEM_WEAPON_MAGICAMMO);
			if (self->health != -9999)
			{
				// reduce the ammount of available ammo by the added clip number
				self->health -= self->damage;
				//G_Printf("%i clips left\n", self->health );
			}
		}
	}
}

#define AMMO_REGENTIME 60000

/**
 * @brief trigger_ammo_think
 * @param[in,out] self
 */
void trigger_ammo_think(gentity_t *self)
{
	self->nextthink = level.time + AMMO_REGENTIME;
	self->health   += self->damage;

	if (self->health > self->count)
	{
		self->health = self->count;
	}
}

#define TRIGGER_AMMO_CANTHINK(self) self->count != -9999

/**
 * @brief trigger_ammo_setup
 * @param[in,out] self
 */
void trigger_ammo_setup(gentity_t *self)
{
	self->target_ent = G_FindByTargetname(NULL, self->target);
	if (!self->target_ent)
	{
		G_Error("trigger_ammo failed to find target: %s\n", self->target);
	}

	self->target_ent->parent = self;

	if (TRIGGER_AMMO_CANTHINK(self))
	{
		self->think     = trigger_ammo_think;
		self->nextthink = level.time + FRAMETIME;
	}
}

/**
 * @brief SP_misc_cabinet_supply
 * @details QUAKED misc_cabinet_supply (.5 .5 .5) (-20 -20 0) (20 20 60)
 * @param[in,out] self
 */
void SP_misc_cabinet_supply(gentity_t *self)
{
	VectorSet(self->r.mins, -20, -20, 0);
	VectorSet(self->r.maxs, 20, 20, 60);

	G_SetOrigin(self, self->s.origin);
	G_SetAngle(self, self->s.angles);

	self->s.eType = ET_CABINET_A;

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;

	trap_LinkEntity(self);
}

/**
 * @brief SP_trigger_ammo
 * @param[in,out] self
 */
void SP_trigger_ammo(gentity_t *self)
{
	char *spawnstr;
	int  ammovalue;

	InitTrigger(self);

	self->touch = ammo_touch;

	// ammototal specifies the maximum amount of ammo this trigger contains
	G_SpawnString("ammototal", "0", &spawnstr);
	ammovalue = Q_atoi(spawnstr);
	// -9999 means infinite now
	self->health = ammovalue;
	if (self->health <= 0)
	{
		self->health = -9999;
	}
	self->count   = self->health;
	self->s.eType = ET_SUPPLIER;

	self->target_ent = NULL;
	if (self->target && *self->target)
	{
		self->think     = trigger_ammo_setup;
		self->nextthink = level.time + FRAMETIME;
	}
	else if (TRIGGER_AMMO_CANTHINK(self))
	{
		self->think     = trigger_ammo_think;
		self->nextthink = level.time + AMMO_REGENTIME;
	}

	// ammorate specifies the amount of ammo added per second
	G_SpawnString("ammorate", "1", &spawnstr);
	ammovalue = Q_atoi(spawnstr);
	// store the rate of ammo addition in damage
	self->damage = ammovalue;
}

/*
==============================================================================
timer
==============================================================================
*/

/**
 * @brief func_timer_think
 *
 * @details QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
 * This should be renamed trigger_timer...
 * Repeatedly fires its targets.
 * Can be turned on or off by using.
 *
 * "wait"			base time between triggering all targets, default is 1
 * "random"		wait variance, default is 0
 * so, the basic time between firing is a random time between
 * (wait - random) and (wait + random)
 *
 * @param[in,out] self
 */
void func_timer_think(gentity_t *self)
{
	G_UseTargets(self, self->activator);
	// set time before next firing
	self->nextthink = (int)(level.time + 1000 * (self->wait + crandom() * self->random));
}

/**
 * @brief func_timer_use
 * @param[in,out] self
 * @param other - unused
 * @param[in] activator
 */
void func_timer_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;

	// if on, turn it off
	if (self->nextthink)
	{
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think(self);
}

/**
 * @brief SP_func_timer
 * @param[in,out] self
 */
void SP_func_timer(gentity_t *self)
{
	G_SpawnFloat("random", "0", &self->random);
	G_SpawnFloat("wait", "1", &self->wait);

	self->use   = func_timer_use;
	self->think = func_timer_think;

	if (self->random >= self->wait)
	{
		self->random = self->wait - (FRAMETIME / 1000.f);   //  div 1000 for milisecs...*cough*
		G_Printf("func_timer at %s has random >= wait\n", vtos(self->s.origin));
	}

	if (self->spawnflags & 1)
	{
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}

/*
==============================================================================
Wolf triggers
==============================================================================
*/

/**
 * @brief SP_trigger_once
 *
 * @details QUAKED trigger_once (.5 .5 .5) ? AI_Touch
 * Must be targeted at one or more entities.
 * Once triggered, this entity is destroyed
 * (you can actually do the same thing with trigger_multiple with a wait of -1)
 *
 * @param[out] ent
 */
void SP_trigger_once(gentity_t *ent)
{
	ent->wait  = -1;            // this will remove itself after one use
	ent->touch = Touch_Multi;
	ent->use   = Use_Multi;

	InitTrigger(ent);
	trap_LinkEntity(ent);
}

// Multiplayer triggers

#define RED_FLAG 1
#define BLUE_FLAG 2

#ifdef FEATURE_OMNIBOT
void Bot_Util_SendTrigger(gentity_t *_ent, gentity_t *_activator, const char *_tagname, const char *_action);
#endif

/**
 * @brief Touch_flagonly
 * @param[in,out] ent
 * @param[in,out] other
 * @param trace - unused
 */
void Touch_flagonly(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	gentity_t *tmp;

	if (!other->client)
	{
		return;
	}

	if ((ent->spawnflags & RED_FLAG) && other->client->ps.powerups[PW_REDFLAG])
	{
		if (ent->spawnflags & 4)
		{
			other->client->ps.powerups[PW_REDFLAG] = 0;
			other->client->speedScale              = 0;

			// update objective indicator
			level.redFlagCounter -= 1;
		}

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Allies captured %s", ent->scriptName), "");
#endif
		// unset objective indicator
		if (!level.redFlagCounter)
		{
			level.flagIndicator &= ~(1 << PW_REDFLAG);
		}
		G_globalFlagIndicator();

		ent->parent = tmp;

		// Removes itself
		ent->touch     = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think     = G_FreeEntity;
	}
	else if ((ent->spawnflags & BLUE_FLAG) && other->client->ps.powerups[PW_BLUEFLAG])
	{
		if (ent->spawnflags & 4)
		{
			other->client->ps.powerups[PW_BLUEFLAG] = 0;
			other->client->speedScale               = 0;

			// update objective indicator
			level.blueFlagCounter -= 1;
		}

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Axis captured %s", ent->scriptName), "");
#endif
		// unset objective indicator
		if (!level.blueFlagCounter)
		{
			level.flagIndicator &= ~(1 << PW_BLUEFLAG);
		}
		G_globalFlagIndicator();

		ent->parent = tmp;

		// Removes itself
		ent->touch     = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think     = G_FreeEntity;
	}
}

/**
 * @brief Touch_flagonly_multiple
 * @param[in,out] ent
 * @param[in,out] other
 * @param trace - unused
 */
void Touch_flagonly_multiple(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	gentity_t *tmp;

	if (!other->client)
	{
		return;
	}

	if ((ent->spawnflags & RED_FLAG) && other->client->ps.powerups[PW_REDFLAG])
	{
		other->client->ps.powerups[PW_REDFLAG] = 0;
		other->client->speedScale              = 0;

		// update objective indicator
		level.redFlagCounter -= 1;

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Allies captured %s", ent->scriptName), "");
#endif
		// unset objective indicator
		if (!level.redFlagCounter)
		{
			level.flagIndicator &= ~(1 << PW_REDFLAG);
		}
		G_globalFlagIndicator();

		ent->parent = tmp;
	}
	else if ((ent->spawnflags & BLUE_FLAG) && other->client->ps.powerups[PW_BLUEFLAG])
	{
		other->client->ps.powerups[PW_BLUEFLAG] = 0;
		other->client->speedScale               = 0;

		// update objective indicator
		level.blueFlagCounter -= 1;

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Axis captured %s", ent->scriptName), "");
#endif
		// unset objective indicator
		if (!level.blueFlagCounter)
		{
			level.flagIndicator &= ~(1 << PW_BLUEFLAG);
		}
		G_globalFlagIndicator();

		ent->parent = tmp;
	}
}

/**
 * @brief SP_trigger_flagonly
 *
 * @details QUAKED trigger_flagonly (.5 .5 .5) ? RED_FLAG BLUE_FLAG KILL_FLAG
 * Player must be carrying the proper flag for it to trigger.
 * It will call the "death" function in the object's script.
 *
 * "scriptName"	The object name in the script file
 *
 * RED_FLAG -- only trigger if player is carrying red flag
 * BLUE_FLAG -- only trigger if player is carrying blue flag
 *
 * @param[in,out] ent
 */
void SP_trigger_flagonly(gentity_t *ent)
{
	char *scorestring;
	ent->touch = Touch_flagonly;

	InitTrigger(ent);

	// if this trigger has a "score" field set, then completing an objective
	// inside of this field will add "score" to the right player team.  storing this
	// in ent->accuracy since that's unused.
	G_SpawnString("score", "20", &scorestring);
	ent->accuracy = (float)(atof(scorestring));
	ent->s.eType  = ET_TRIGGER_FLAGONLY;
#ifdef VISIBLE_TRIGGERS
	ent->r.svFlags &= ~SVF_NOCLIENT;
#endif // VISIBLE_TRIGGERS

	trap_LinkEntity(ent);
}

/**
 * @brief SP_trigger_flagonly_multiple
 *
 * @details QUAKED trigger_flagonly_multiple (.5 .5 .5) ? RED_FLAG BLUE_FLAG
 * Player must be carrying the proper flag for it to trigger.
 * It will call the "death" function in the object's script.
 *
 * "scriptName"	The object name in the script file
 *
 * RED_FLAG -- only trigger if player is carrying red flag
 * BLUE_FLAG -- only trigger if player is carrying blue flag
 *
 *
 * @param ent
 */
void SP_trigger_flagonly_multiple(gentity_t *ent)
{
	char *scorestring;
	ent->touch = Touch_flagonly_multiple;

	InitTrigger(ent);

	// if this trigger has a "score" field set, then completing an objective
	//  inside of this field will add "score" to the right player team.  storing this
	//  in ent->accuracy since that's unused.
	G_SpawnString("score", "20", &scorestring);
	ent->accuracy = (float)(atof(scorestring));
	ent->s.eType  = ET_TRIGGER_FLAGONLY_MULTIPLE;
#ifdef VISIBLE_TRIGGERS
	ent->r.svFlags &= ~SVF_NOCLIENT;
#endif // VISIBLE_TRIGGERS

	trap_LinkEntity(ent);
}

/**
 * @brief Spawn an explosive indicator
 * @param[in,out] ent
 */
void explosive_indicator_think(gentity_t *ent)
{
	gentity_t *parent = &g_entities[ent->r.ownerNum];

	if (!parent->inuse || (parent->s.eType == ET_CONSTRUCTIBLE && !parent->r.linked))
	{
		// update our map
		{
			mapEntityData_t *mEnt;

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[0], mEnt);
			}
			if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(&mapEntityData[1], mEnt);
			}
		}
		G_FreeEntity(ent);
		return;
	}

	if (ent->s.eType == ET_TANK_INDICATOR || ent->s.eType == ET_TANK_INDICATOR_DEAD)
	{
		VectorCopy(ent->parent->r.currentOrigin, ent->s.pos.trBase);
	}
	ent->nextthink = level.time + FRAMETIME;

	if (parent->s.eType == ET_OID_TRIGGER && parent->target_ent)
	{
		ent->s.effect1Time = parent->target_ent->constructibleStats.weaponclass;
	}
	else
	{
		ent->s.effect1Time = parent->constructibleStats.weaponclass;
	}
}

/**
 * @brief Spawn a constructible indicator
 * @param[in,out] ent
 */
void constructible_indicator_think(gentity_t *ent)
{
	gentity_t *parent        = &g_entities[ent->r.ownerNum];
	gentity_t *constructible = parent->target_ent;

	if (parent->chain)
	{
		// use the target that has the same team as the indicator
		if (constructible->s.teamNum != ent->s.teamNum)
		{
			constructible = parent->chain;
		}
	}

	if (!parent->inuse || !parent->r.linked || (constructible && constructible->s.angles2[1] != 0.0f))
	{
		// update our map
		{
			mapEntityData_t      *mEnt;
			mapEntityData_Team_t *teamList;

			if (parent->spawnflags & 8)
			{
				if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
				{
					G_FreeMapEntityData(&mapEntityData[0], mEnt);
				}
				if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL)
				{
					G_FreeMapEntityData(&mapEntityData[1], mEnt);
				}
			}
			else
			{
				teamList = ent->s.teamNum == TEAM_AXIS ? &mapEntityData[0] : &mapEntityData[1];
				if ((mEnt = G_FindMapEntityData(teamList, ent - g_entities)) != NULL)
				{
					G_FreeMapEntityData(teamList, mEnt);
				}
			}
		}

		parent->count2 = 0;
		G_FreeEntity(ent);
		return;
	}

	if (ent->s.eType == ET_TANK_INDICATOR || ent->s.eType == ET_TANK_INDICATOR_DEAD)
	{
		VectorCopy(ent->parent->r.currentOrigin, ent->s.pos.trBase);
	}
	ent->s.effect1Time = parent->constructibleStats.weaponclass;
	ent->nextthink     = level.time + FRAMETIME;
}

/**
 * @brief G_SetConfigStringValue
 * @param[in] num
 * @param[in] key
 * @param[in] value
 */
void G_SetConfigStringValue(int num, const char *key, const char *value)
{
	char cs[MAX_STRING_CHARS];

	trap_GetConfigstring(num, cs, sizeof(cs));
	Info_SetValueForKey(cs, key, value);
	trap_SetConfigstring(num, cs);
}

/**
 * @brief Touch_ObjectiveInfo
 * @param[in] ent
 * @param[in,out] other
 * @param trace - unused
 */
void Touch_ObjectiveInfo(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	if (!other->client)
	{
		return;
	}

	other->client->touchingTOI = ent;
}

/**
 * @brief Links the trigger to it's objective, determining if it's a func_explosive
 * of func_constructible and spawning the right indicator
 * @param[in,out] ent
 */
void Think_SetupObjectiveInfo(gentity_t *ent)
{
	ent->target_ent = G_FindByTargetname(&g_entities[MAX_CLIENTS - 1], ent->target);

	if (!ent->target_ent)
	{
		G_Error("'trigger_objective_info' has a missing target '%s'\n", ent->target);
	}

	if (ent->target_ent->s.eType == ET_EXPLOSIVE)
	{
		// this is for compass usage
		if ((ent->spawnflags & AXIS_OBJECTIVE) || (ent->spawnflags & ALLIED_OBJECTIVE))
		{
			gentity_t *e;

			e = G_Spawn();

			e->r.svFlags = SVF_BROADCAST;
			e->classname = "explosive_indicator";
			if (ent->spawnflags & 8)
			{
				e->s.eType = ET_TANK_INDICATOR;
			}
			else
			{
				e->s.eType = ET_EXPLOSIVE_INDICATOR;
			}
			e->parent       = ent;
			e->s.pos.trType = TR_STATIONARY;

			if (ent->spawnflags & AXIS_OBJECTIVE)
			{
				e->s.teamNum = 1;
			}
			else if (ent->spawnflags & ALLIED_OBJECTIVE)
			{
				e->s.teamNum = 2;
			}

			G_SetOrigin(e, ent->r.currentOrigin);

			e->s.modelindex2 = ent->s.teamNum;
			e->r.ownerNum    = ent->s.number;
			e->think         = explosive_indicator_think;
			e->nextthink     = level.time + FRAMETIME;

			e->s.effect1Time = ent->target_ent->constructibleStats.weaponclass;

			if (ent->tagParent)
			{
				e->tagParent = ent->tagParent;
				Q_strncpyz(e->tagName, ent->tagName, MAX_QPATH);
			}
			else
			{
				VectorCopy(ent->r.absmin, e->s.pos.trBase);
				VectorAdd(ent->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
				VectorScale(e->s.pos.trBase, 0.5f, e->s.pos.trBase);
			}

			SnapVector(e->s.pos.trBase);

			trap_LinkEntity(e);

			ent->target_ent->parent = ent;
		}
	}
	else if (ent->target_ent->s.eType == ET_CONSTRUCTIBLE)
	{
		gentity_t *constructibles[2];
		int       team[2] = { 0 };

		ent->target_ent->parent = ent;

		constructibles[0] = ent->target_ent;
		constructibles[1] = G_FindByTargetname(constructibles[0], ent->target);     // see if we are targetting a 2nd one for two team constructibles

		team[0] = (constructibles[0]->spawnflags & AXIS_CONSTRUCTIBLE) ? TEAM_AXIS : TEAM_ALLIES;

		constructibles[0]->s.otherEntityNum2 = ent->s.teamNum;

		if (constructibles[1])
		{
			team[1] = (constructibles[1]->spawnflags & AXIS_CONSTRUCTIBLE) ? TEAM_AXIS : TEAM_ALLIES;

			if (constructibles[1]->s.eType != ET_CONSTRUCTIBLE)
			{
				G_Error("'trigger_objective_info' targets multiple entities with targetname '%s', the second one isn't a 'func_constructible' [%d]\n", ent->target, constructibles[1]->s.eType);
			}

			if (team[0] == team[1])
			{
				G_Error("'trigger_objective_info' targets two 'func_constructible' entities with targetname '%s' that are constructible by the same team\n", ent->target);
			}

			constructibles[1]->s.otherEntityNum2 = ent->s.teamNum;

			ent->chain         = constructibles[1];
			ent->chain->parent = ent;

			constructibles[0]->chain = constructibles[1];
			constructibles[1]->chain = constructibles[0];
		}
		else
		{
			constructibles[0]->chain = NULL;
		}

		// if already constructed (in case of START_BUILT)
		if (constructibles[0]->s.angles2[1] == 0.f)
		{
			// spawn a constructible icon - this is for compass usage
			gentity_t *e;

			e = G_Spawn();

			e->r.svFlags      = SVF_BROADCAST;
			e->classname      = "constructible_indicator";
			e->targetnamehash = -1;
			if (ent->spawnflags & 8)
			{
				e->s.eType = ET_TANK_INDICATOR_DEAD;
			}
			else
			{
				e->s.eType = ET_CONSTRUCTIBLE_INDICATOR;
			}
			e->s.pos.trType = TR_STATIONARY;

			if (constructibles[1])
			{
				// see if one of the two is still partially built (happens when a multistage destructible construction blows up for the first time)
				if (constructibles[0]->count2 && constructibles[0]->grenadeFired > 1)
				{
					e->s.teamNum = team[0];
				}
				else if (constructibles[1]->count2 && constructibles[1]->grenadeFired > 1)
				{
					e->s.teamNum = team[1];
				}
				else
				{
					e->s.teamNum = 3;   // both teams
				}
			}
			else
			{
				e->s.teamNum = team[0];
			}

			e->s.modelindex2 = ent->s.teamNum;
			e->r.ownerNum    = ent->s.number;
			ent->count2      = (e - g_entities);
			e->think         = constructible_indicator_think;
			e->nextthink     = level.time + FRAMETIME;

			e->parent = ent;

			if (ent->tagParent)
			{
				e->tagParent = ent->tagParent;
				Q_strncpyz(e->tagName, ent->tagName, MAX_QPATH);
			}
			else
			{
				VectorCopy(ent->r.absmin, e->s.pos.trBase);
				VectorAdd(ent->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
				VectorScale(e->s.pos.trBase, 0.5f, e->s.pos.trBase);
			}

			SnapVector(e->s.pos.trBase);

			trap_LinkEntity(e);     // moved down
		}
		ent->touch = Touch_ObjectiveInfo;

	}
	else if (ent->target_ent->s.eType == ET_COMMANDMAP_MARKER)
	{
		ent->target_ent->parent = ent;
	}

	trap_LinkEntity(ent);
}

#define AXIS_OBJECTIVE      1
#define ALLIED_OBJECTIVE    2
#define MESSAGE_OVERRIDE    4
#define TANK                8

/**
 * @brief QUAKED trigger_objective_info (.5 .5 .5) ? AXIS_OBJECTIVE ALLIED_OBJECTIVE MESSAGE_OVERRIDE TANK IS_OBJECTIVE IS_HEALTHAMMOCABINET IS_COMMANDPOST
 * Players in this field will see a message saying that they are near an objective.
 *
 *   "track"		Mandatory, this is the text that is appended to "You are near "
 *   "shortname"	Short name to show on command centre
 *
 * @param[in,out] ent
 */
void SP_trigger_objective_info(gentity_t *ent)
{
	char *scorestring;
	char *customimage;
	int  cix, cia, objflags;

	if (!ent->track)
	{
		G_Error("'trigger_objective_info' does not have a 'track' \n");
	}

	if (ent->spawnflags & MESSAGE_OVERRIDE)
	{
		if (!ent->spawnitem)
		{
			G_Error("'trigger_objective_info' has override flag set but no override text\n");
		}
	}

	// for specifying which commandmap objectives this entity "belongs" to
	G_SpawnInt("objflags", "0", &objflags);

	if (G_SpawnString("customimage", "", &customimage))
	{
		cix = cia = G_ShaderIndex(customimage);
	}
	else
	{
		if (G_SpawnString("customaxisimage", "", &customimage))
		{
			cix = G_ShaderIndex(customimage);
		}
		else
		{
			cix = 0;
		}

		if (G_SpawnString("customalliesimage", "", &customimage))
		{
			cia = G_ShaderIndex(customimage);
		}
		else if (G_SpawnString("customalliedimage", "", &customimage))
		{
			cia = G_ShaderIndex(customimage);
		}
		else
		{
			cia = 0;
		}
	}

	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "e", va("%i", (int)(ent - g_entities)));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "o", va("%i", objflags));
	if (cix)
	{
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "cix", va("%i", cix));
	}
	if (cia)
	{
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "cia", va("%i", cia));
	}
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "s", va("%i", ent->spawnflags));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "n", ent->message ? ent->message : "");

	if (level.numOidTriggers >= MAX_OID_TRIGGERS)
	{
		G_Error("Exceeded maximum number of 'trigger_objective_info' entities\n");
	}

	// if this trigger has a "score" field set, then blowing up an objective
	// inside of this field will add "score" to the right player team.  storing this
	// in ent->accuracy since that's unused.
	G_SpawnString("score", "0", &scorestring);
	ent->accuracy = (float)(atof(scorestring));

	trap_SetConfigstring(CS_OID_TRIGGERS + level.numOidTriggers, ent->track);

	InitTrigger(ent);

	if (ent->s.origin[0] != 0.f || ent->s.origin[1] != 0.f || ent->s.origin[2] != 0.f)
	{
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "x", va("%i", (int)ent->s.origin[0]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "y", va("%i", (int)ent->s.origin[1]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "z", va("%i", (int)ent->s.origin[2]));
	}
	else
	{
		vec3_t mid;

		VectorAdd(ent->r.absmin, ent->r.absmax, mid);
		VectorScale(mid, 0.5f, mid);

		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "x", va("%i", (int)mid[0]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "y", va("%i", (int)mid[1]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "z", va("%i", (int)mid[2]));
	}

	ent->s.teamNum = level.numOidTriggers++;

	// unlike other triggers, we need to send this one to the client
	ent->r.svFlags &= ~SVF_NOCLIENT;
	ent->s.eType    = ET_OID_TRIGGER;

	if (!ent->target)
	{
		// no target - just link and go
		trap_LinkEntity(ent);
	}
	else
	{
		// finalize spawing on fourth frame to allow for proper linking with targets
		ent->nextthink = level.time + (3 * FRAMETIME);
		ent->think     = Think_SetupObjectiveInfo;
	}
}

/**
 * @brief SP_trigger_concussive_dust
 *
 * @details QUAKED trigger_concussive_dust (.5 .5 .5) ?
 * Allows client side prediction of teleportation events.
 * Must point at a target_position, which will be the teleport destination.
 *
 * @param[in] self
 */
void SP_trigger_concussive_dust(gentity_t *self)
{
	G_Printf("trigger_concussive_dust is obsolete, please delete it.\n");
	G_FreeEntity(self);
}
