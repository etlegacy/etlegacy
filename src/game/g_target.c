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
 * @file g_target.c
 */

#include "g_local.h"

//==========================================================

/**
 * @brief Gives the activator all the items pointed to.
 * QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
 * @param[in] ent
 * @param other - unused
 * @param[in] activator
 */
void Use_Target_Give(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	gentity_t *t;
	trace_t   trace;

	if (!activator->client)
	{
		return;
	}

	if (!ent->target)
	{
		return;
	}

	Com_Memset(&trace, 0, sizeof(trace));
	t = NULL;
	while ((t = G_FindByTargetname(t, ent->target)) != NULL)
	{
		if (!t->item)
		{
			continue;
		}
		Touch_Item(t, activator, &trace);

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity(t);
	}
}
/**
 * @brief SP_target_give
 * @param[out] ent
 */
void SP_target_give(gentity_t *ent)
{
	ent->use = Use_Target_Give;
}

//==========================================================

/**
 * @brief Takes away all the activators powerups.
 * Used to drop flight powerups into death puts.
 * QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
 * @param ent - unused
 * @param other - unused
 * @param[in,out] activator
 */
void Use_target_remove_powerups(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (!activator->client)
	{
		return;
	}

	if (activator->client->ps.powerups[PW_REDFLAG])
	{
		// update objective indicator
		level.redFlagCounter -= 1;
		Team_ReturnFlag(&g_entities[activator->client->flagParent]);
	}
	if (activator->client->ps.powerups[PW_BLUEFLAG])
	{
		// update objective indicator
		level.blueFlagCounter -= 1;
		Team_ReturnFlag(&g_entities[activator->client->flagParent]);
	}

	Com_Memset(activator->client->ps.powerups, 0, sizeof(activator->client->ps.powerups));
}

/**
 * @brief SP_target_remove_powerups
 * @param[out] ent
 */
void SP_target_remove_powerups(gentity_t *ent)
{
	ent->use = Use_target_remove_powerups;
}

//==========================================================

/**
 * @brief QUAKED target_delay (1 1 0) (-8 -8 -8) (8 8 8)
 * "wait" seconds to pause before firing targets.
 * "random" delay variance, total delay = delay +/- random seconds
 * @param ent
 */
void Think_Target_Delay(gentity_t *ent)
{
	G_UseTargets(ent, ent->activator);
}

/**
 * @brief Use_Target_Delay
 * @param[in,out] ent
 * @param other - unused
 * @param[in] activator
 */
void Use_Target_Delay(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	ent->nextthink = (int)(level.time + (ent->wait + ent->random * crandom()) * 1000);
	ent->think     = Think_Target_Delay;
	ent->activator = activator;
}

/**
 * @brief SP_target_delay
 * @param[in,out] ent
 */
void SP_target_delay(gentity_t *ent)
{
	// check delay for backwards compatability
	if (!G_SpawnFloat("delay", "0", &ent->wait))
	{
		G_SpawnFloat("wait", "1", &ent->wait);
	}

	if (ent->wait == 0.f)
	{
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}

/**
 * @brief Use_Target_Print
 * QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
 * "message"	text to print
 * If "private", only the activator gets the message.  If no checks, all clients get the message.
 * @param[in] ent
 * @param other - unused
 * @param[in] activator
 */
void Use_Target_Print(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if ((ent->spawnflags & 4))
	{
		if (!activator)
		{
			G_Error("G_scripting: call to client only target_print with no activator\n");
		}

		if (activator->client)
		{
			trap_SendServerCommand(activator - g_entities, va("cp \"%s\"", ent->message));
			return;
		}
	}

	if (ent->spawnflags & 3)
	{
		if (ent->spawnflags & 1)
		{
			G_TeamCommand(TEAM_AXIS, va("cp \"%s\"", ent->message));
		}
		if (ent->spawnflags & 2)
		{
			G_TeamCommand(TEAM_ALLIES, va("cp \"%s\"", ent->message));
		}
		return;
	}

	trap_SendServerCommand(-1, va("cp \"%s\"", ent->message));
}

/**
 * @brief SP_target_print
 * @param[out] ent
 */
void SP_target_print(gentity_t *ent)
{
	ent->use = Use_Target_Print;
}

//==========================================================

/**
 * @brief Use_Target_Speaker
 * @details QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) LOOPED_ON LOOPED_OFF GLOBAL ACTIVATOR VIS_MULTIPLE NO_PVS
 * "noise"		wav file to play
 * A global sound will play full volume throughout the level.
 * Activator sounds will play on the player that activated the target.
 * Global and activator sounds can't be combined with looping.
 * Normal sounds play each time the target is used.
 * Looped sounds will be toggled by use functions.
 * Multiple identical looping sounds will just increase volume without any speed cost.
 * NO_PVS - this sound will not turn off when not in the player's PVS
 * "wait" : Seconds between auto triggerings, 0 = don't auto trigger
 * "random" : wait variance, default is 0
 * "volume" volume control 255 is default
 *
 * @param[in,out] ent
 * @param other
 * @param[in] activator - unused
 */
void Use_Target_Speaker(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->spawnflags & 3)      // looping sound toggles
	{
		if (ent->s.loopSound)
		{
			ent->s.loopSound = 0;   // turn it off
		}
		else
		{
			ent->s.loopSound = ent->noise_index;    // start it
		}
	}
	else     // normal sound
	{
		if (ent->spawnflags & 8)
		{
			G_AddEvent(activator, EV_GENERAL_SOUND_VOLUME, ent->noise_index);
		}
		else // if (ent->spawnflags & 4) & others
		{
			G_AddEvent(ent, EV_GENERAL_SOUND_VOLUME, ent->noise_index);
		}
	}
}

/**
 * @brief target_speaker_multiple
 * @param[in] ent
 */
void target_speaker_multiple(gentity_t *ent)
{
	gentity_t *vis_dummy = NULL;

	if (!(ent->target))
	{
		G_Error("target_speaker missing target at pos %s", vtos(ent->s.origin));
	}

	vis_dummy = G_FindByTargetname(NULL, ent->target);

	if (vis_dummy)
	{
		ent->s.otherEntityNum = vis_dummy->s.number;
	}
	else
	{
		G_Error("target_speaker cant find vis_dummy_multiple %s\n", vtos(ent->s.origin));
	}
}

/**
 * @brief SP_target_speaker
 * @param[in,out] ent
 */
void SP_target_speaker(gentity_t *ent)
{
	char buffer[MAX_QPATH];
	char *s;

	G_SpawnFloat("wait", "0", &ent->wait);
	G_SpawnFloat("random", "0", &ent->random);

	if (!G_SpawnString("noise", "NOSOUND", &s))
	{
		G_Error("target_speaker without a noise key at %s\n", vtos(ent->s.origin));
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if (s[0] == '*')
	{
		ent->spawnflags |= 8;
	}

	// had to disable this so we can use sound scripts
	// don't worry, if the script isn't found, it'll default back to
	// .wav on the client-side
	//if (!strstr( s, ".wav" )) {
	//	Com_sprintf (buffer, sizeof(buffer), "%s.wav", s );
	//} else {
	Q_strncpyz(buffer, s, sizeof(buffer));
	//}
	ent->noise_index = G_SoundIndex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType     = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame     = (int)(ent->wait * 10);
	ent->s.clientNum = (int)(ent->random * 10);

	// check for prestarted looping sound
	if (ent->spawnflags & 1)
	{
		ent->s.loopSound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	// GLOBAL
	if (ent->spawnflags & (4 | 32))
	{
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy(ent->s.origin, ent->s.pos.trBase);

	if (ent->spawnflags & 16)
	{
		ent->think     = target_speaker_multiple;
		ent->nextthink = level.time + 50;
	}

	// NO_PVS
	if (ent->spawnflags & 32)
	{
		ent->s.density = 1;
	}
	else
	{
		ent->s.density = 0;
	}

	if (ent->radius)
	{
		ent->s.dmgFlags = ent->radius;  // store radius in dmgflags
	}
	else
	{
		ent->s.dmgFlags = 0;
	}

	// Volume control!, i want some cookies for this Tim! :o
	G_SpawnInt("volume", "255", &ent->s.onFireStart);
	if (!ent->s.onFireStart)
	{
		ent->s.onFireStart = 255;
	}

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity(ent);
}

/**
 * @brief When on, displays a electric beam from target to target2.
 * @details QUAKED misc_beam (0 .5 .8) (-8 -8 -8) (8 8 8)
 *
 * "target"	start of beam
 * "target2"	end of beam
 * "shader"	the shader
 * "color"		colour of beam		*NOT WORKIN YET*
 * "scale"		width of beam		*NOT WORKIN YET*
 *
 * @param[in,out] self
 */
void misc_beam_think(gentity_t *self)
{
	if (self->enemy)
	{
		if (self->enemy != self)
		{
			//VectorCopy ( self->enemy->s.origin, self->s.origin2 );
			self->s.apos.trType     = self->enemy->s.pos.trType;
			self->s.apos.trTime     = self->enemy->s.pos.trTime;
			self->s.apos.trDuration = self->enemy->s.pos.trDuration;
			VectorCopy(self->enemy->s.pos.trBase, self->s.apos.trBase);
			VectorCopy(self->enemy->s.pos.trDelta, self->s.apos.trDelta);

			self->s.effect2Time = self->enemy->s.effect2Time;
		}
		else
		{
			self->s.apos.trType = TR_STATIONARY;
			VectorCopy(self->s.origin, self->s.apos.trBase);
		}
	}

	self->s.pos.trType     = self->target_ent->s.pos.trType;
	self->s.pos.trTime     = self->target_ent->s.pos.trTime;
	self->s.pos.trDuration = self->target_ent->s.pos.trDuration;
	VectorCopy(self->target_ent->s.pos.trBase, self->s.pos.trBase);
	VectorCopy(self->target_ent->s.pos.trDelta, self->s.pos.trDelta);

	self->s.effect1Time = self->target_ent->s.effect2Time;

	self->nextthink = level.time + FRAMETIME;

	if (self->s.pos.trType != TR_STATIONARY || self->s.apos.trType != TR_STATIONARY || self->accuracy == 0.f)
	{
		int i;

		self->accuracy = 1;

		self->r.contents = CONTENTS_SOLID;
		VectorCopy(self->s.pos.trBase, self->r.mins);
		VectorCopy(self->s.apos.trBase, self->r.maxs);

		for (i = 0; i < 3; i++)
		{
			if (self->r.maxs[i] < self->r.mins[i])
			{
				float bleh = self->r.mins[i];
				self->r.mins[i] = self->r.maxs[i];
				self->r.maxs[i] = bleh;
			}
		}

		self->r.mins[0] -= 4;
		self->r.mins[1] -= 4;
		self->r.mins[2] -= 4;
		self->r.maxs[0] += 4;
		self->r.maxs[1] += 4;
		self->r.maxs[2] += 4;

		VectorCopy(self->s.origin, self->r.currentOrigin);
		VectorSubtract(self->r.mins, self->r.currentOrigin, self->r.mins);
		VectorSubtract(self->r.maxs, self->r.currentOrigin, self->r.maxs);

		trap_LinkEntity(self);
	}
}

/**
 * @brief misc_beam_start
 * @param[in,out] self
 */
void misc_beam_start(gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM_2;

	if (self->target)
	{
		ent = G_FindByTargetname(NULL, self->target);
		if (!ent)
		{
			G_Printf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
			G_FreeEntity(self);
			return;
		}
		self->target_ent = ent;
	}
	else
	{
		G_Printf("%s at %s: with no target\n", self->classname, vtos(self->s.origin));
		G_FreeEntity(self);
		return;
	}

	if (self->message)
	{
		ent = G_FindByTargetname(NULL, self->message);
		if (!ent)
		{
			G_Printf("%s at %s: %s is a bad target2\n", self->classname, vtos(self->s.origin), self->message);
			G_FreeEntity(self);
			return; // No targets by this name.
		}
		self->enemy = ent;
	}
	else     // the misc_beam is it's own ending point
	{
		self->enemy = self;
	}

	self->accuracy  = 0;
	self->think     = misc_beam_think;
	self->nextthink = level.time + FRAMETIME;
}

/**
 * @brief SP_misc_beam
 * @param[in,out] self
 */
void SP_misc_beam(gentity_t *self)
{
	char *str;

	G_SpawnString("target2", "", &str);
	if (*str)
	{
		self->message = G_NewString(str);
	}

	G_SpawnString("shader", "lightningBolt", &str);
	if (*str)
	{
		self->s.modelindex2 = G_ShaderIndex(str);
	}

	G_SpawnInt("scale", "1", &self->s.torsoAnim);
	G_SpawnVector("color", "1 1 1", self->s.angles2);

	// let everything else get spawned before we start firing
	self->accuracy  = 0;
	self->think     = misc_beam_start;
	self->nextthink = level.time + FRAMETIME;
}

//==========================================================

/**
 * @brief When triggered, fires a laser.  You can either set a target or a direction.
 * @details QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
 * @param[in,out] self
 */
void target_laser_think(gentity_t *self)
{
	vec3_t  end;
	trace_t tr;
	vec3_t  point;

	// if pointed at another entity, set movedir to point at it
	if (self->enemy)
	{
		VectorMA(self->enemy->s.origin, 0.5f, self->enemy->r.mins, point);
		VectorMA(point, 0.5f, self->enemy->r.maxs, point);
		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);
	}

	// fire forward and see what we hit
	VectorMA(self->s.origin, 2048, self->movedir, end);

	trap_Trace(&tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE);

	if (tr.entityNum)
	{
		// hurt it if we can
		G_Damage(&g_entities[tr.entityNum], self, self->activator, self->movedir,
		         tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy(tr.endpos, self->s.origin2);

	trap_LinkEntity(self);
	self->nextthink = level.time + FRAMETIME;
}

/**
 * @brief target_laser_on
 * @param[in,out] self
 */
void target_laser_on(gentity_t *self)
{
	if (!self->activator)
	{
		self->activator = self;
	}
	target_laser_think(self);
}

/**
 * @brief target_laser_off
 * @param[out] self
 */
void target_laser_off(gentity_t *self)
{
	trap_UnlinkEntity(self);
	self->nextthink = 0;
}

/**
 * @brief target_laser_use
 * @param[in,out] self
 * @param other - unused
 * @param[in] activator
 */
void target_laser_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;
	if (self->nextthink > 0)
	{
		target_laser_off(self);
	}
	else
	{
		target_laser_on(self);
	}
}

/**
 * @brief target_laser_start
 * @param[in,out] self
 */
void target_laser_start(gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target)
	{
		ent = G_FindByTargetname(NULL, self->target);
		if (!ent)
		{
			G_Printf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		self->enemy = ent;
	}
	else
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->use   = target_laser_use;
	self->think = target_laser_think;

	if (!self->damage)
	{
		self->damage = 1;
	}

	if (self->spawnflags & 1)
	{
		target_laser_on(self);
	}
	else
	{
		target_laser_off(self);
	}
}

/**
 * @brief SP_target_laser
 * @param[out] self
 */
void SP_target_laser(gentity_t *self)
{
	self->s.legsAnim = 1;

	self->s.angles2[0] = 1.f;
	self->s.angles2[1] = 1.f;
	self->s.angles2[2] = 1.f;

	// let everything else get spawned before we start firing
	self->think     = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}

//==========================================================

/**
 * @brief target_teleporter_use
 * @param[in] self
 * @param other - unused
 * @param[in] activator
 */
void target_teleporter_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	gentity_t *dest;

	if (!activator->client)
	{
		return;
	}
	dest = G_PickTarget(self->target);
	if (!dest)
	{
		G_Printf("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer(activator, dest->s.origin, dest->s.angles);
}

/**
 * @brief The activator will be teleported away.
 * QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * @param[in,out] self
 */
void SP_target_teleporter(gentity_t *self)
{
	if (!self->targetname)
	{
		G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));
	}

	self->use = target_teleporter_use;
}

//==========================================================

/**
 * @brief target_relay_use
 * @details QUAKED target_relay (1 1 0) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM NOKEY_ONLY TAKE_KEY NO_LOCKED_NOISE
 * This doesn't perform any actions except fire its targets.
 * The activator can be forced to be from a certain team.
 * if RANDOM is checked, only one of the targets will be fired, not all of them
 * "key" specifies an item you can be carrying that affects the operation of this relay
 * this key is currently an int (1-16) which matches the id of a key entity (key_key1 = 1, etc)
 * NOKEY_ONLY means "fire only if I do /not/ have the specified key"
 * TAKE_KEY removes the key from the players inventory
 * "lockednoise" specifies a .wav file to play if the relay is used and the player doesn't have the necessary key.
 * By default this sound is "sound/movers/doors/default_door_locked.wav"
 * NO_LOCKED_NOISE specifies that it will be silent if activated without proper key
 *
 * @param[in] self
 * @param other - unused
 * @param[in] activator
 */
void target_relay_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ((self->spawnflags & 1) && activator && activator->client
	    && activator->client->sess.sessionTeam != TEAM_AXIS)
	{
		return;
	}
	if ((self->spawnflags & 2) && activator && activator->client
	    && activator->client->sess.sessionTeam != TEAM_ALLIES)
	{
		return;
	}

	if (self->spawnflags & 4)
	{
		gentity_t *ent;

		ent = G_PickTarget(self->target);
		if (ent && ent->use)
		{
			G_UseEntity(ent, self, activator);
		}
		return;
	}

	if (activator)     // activator can be NULL if called from script
	{
		if (self->key)
		{
			// removed keys
			//gitem_t *item;

			if (self->key == -1)     // relay permanently locked
			{
				if (self->soundPos1)
				{
					G_Sound(self, self->soundPos1);
				}
				return;
			}

			/*
			if(self->spawnflags & 16) {	// take key
			    activator->client->ps.stats[STAT_KEYS] &= ~(1<<item->giTag);
			    // TODO: "took inventory item" sound
			}*/
		}
	}

	G_UseTargets(self, activator);
}

/**
 * @brief SP_target_relay
 * @param[in,out] self
 */
void SP_target_relay(gentity_t *self)
{
	char *sound;

	self->use = target_relay_use;

	if (!(self->spawnflags & 32))        // !NO_LOCKED_NOISE
	{
		if (G_SpawnString("lockednoise", "0", &sound))
		{
			self->soundPos1 = G_SoundIndex(sound);
		}
		else
		{
			self->soundPos1 = G_SoundIndex("sound/movers/doors/default_door_locked.wav");
		}
	}
}

//==========================================================

/*



*/

/**
 * @brief Kills the activator. (default)
 * @details QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8) kill_user_too
 * If targets, they will be killed when this is fired
 * "kill_user_too" will still kill the activator when this ent has targets (default is only kill targets, not activator)
 *
 * @param[in] target
 * @param[in] ignore
 * @param[in] killer
 * @param[in] mod
 */
void G_KillEnts(const char *target, gentity_t *ignore, gentity_t *killer, meansOfDeath_t mod)
{
	gentity_t *targ = NULL;

	while ((targ = G_FindByTargetname(targ, target)))
	{
		// make sure it isn't going to respawn or show any events
		targ->nextthink = 0;

		if (targ == ignore)
		{
			continue;
		}

		// script_movers should die!
		if (targ->s.eType == ET_MOVER && !Q_stricmp(targ->classname, "script_mover") && targ->die)
		{
			G_Damage(targ, killer, killer, NULL, NULL, targ->client ? GIB_DAMAGE(targ->health) : GIB_ENT, DAMAGE_NO_PROTECTION, MOD_EXPLOSIVE);
			continue;
		}

		if (targ->s.eType == ET_CONSTRUCTIBLE)
		{
			if (killer)
			{
				G_AddKillSkillPointsForDestruction(killer, mod, &targ->constructibleStats);
			}
			targ->die(targ, killer, killer, targ->health, MOD_UNKNOWN);
			continue;
		}

		trap_UnlinkEntity(targ);
		targ->nextthink = level.time + FRAMETIME;

		targ->use   = NULL;
		targ->touch = NULL;
		targ->think = G_FreeEntity;
	}
}

/**
 * @brief target_kill_use
 * @param[in] self
 * @param other - unused
 * @param[in] activator
 */
void target_kill_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (self->spawnflags & 1)      // kill usertoo
	{
		G_Damage(activator, NULL, NULL, NULL, NULL, GIB_DAMAGE(activator->health), DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}

	G_KillEnts(self->target, activator, self, MOD_UNKNOWN);
}

/**
 * @brief SP_target_kill
 * @param[out] self
 */
void SP_target_kill(gentity_t *self)
{
	self->use = target_kill_use;
}

/**
 * @brief Used as a positional target for in-game calculation, like jumppad targets.
 * @details DEFUNCT target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
 * @param self
 */
void SP_target_position(gentity_t *self)
{
	G_SetOrigin(self, self->s.origin);
}

/**
 * @brief SP_target_location
 * @details QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
 * Set "message" to the name of this location.
 * Set "count" to 0-7 for color.
 * 0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white
 *
 * Closest target_location in sight used for the location, if none
 * in site, closest in distance
 * @param self
 */
void SP_target_location(gentity_t *self)
{
	G_Printf(S_COLOR_YELLOW "WARNING: target_location entities are now obsolete. Please remove ASAP\n");

	G_FreeEntity(self);
}

// Wolf targets

/**
 * @brief Use_Target_Counter
 * @param[in,out] ent
 * @param[in] other
 * @param activator - unused
 */
void Use_Target_Counter(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->count < 0)     // if the count has already been hit, ignore this
	{
		return;
	}

	ent->count -= 1;    // dec count

	//	G_Printf("count at: %d\n", ent->count);

	if (!ent->count)       // specified count is now hit
	{ //		G_Printf("firing!!\n");
		G_UseTargets(ent, other);
	}
}

/**
 * @brief Use_Target_Lock
 * @param[in] ent
 * @param other - unused
 * @param activator - unused
 */
void Use_Target_Lock(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	gentity_t *t = 0;

	while ((t = G_Find(t, FOFS(targetname), ent->target)) != NULL)
	{
		G_Printf("target_lock locking entity with key: %d\n", ent->count);
		t->key = ent->key;
	}
}

//==========================================================

/**
 * @brief Use_target_fog
 * @param[in,out] ent
 * @param other - unused
 * @param activator - unused
 */
void Use_target_fog(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
//	CS_FOGVARS reads:
//		near
//		far
//		density
//		r,g,b
//		time to complete
	trap_SetConfigstring(CS_FOGVARS, va("%f %f %f %f %f %f %i", 1.0, (double)ent->s.density, 1.0, (double)ent->dl_color[0], (double)ent->dl_color[1], (double)ent->dl_color[2], ent->s.time));
}

/**
 * @brief SP_target_fog
 * @details QUAKED target_fog (1 1 0) (-8 -8 -8) (8 8 8)
 * color picker chooses color of fog
 * "distance" sets fog distance.  Use value '0' to give control back to the game (and use the fog values specified in the sky shader if present)
 * "time" time it takes to change fog to new value.  default time is 1 sec
 * @param[out] ent
 */
void SP_target_fog(gentity_t *ent)
{
	int   dist;
	float ftime;

	ent->use = Use_target_fog;

	// ent->s.density will carry the 'distance' value
	if (G_SpawnInt("distance", "0", &dist))
	{
		if (dist >= 0)
		{
			ent->s.density = dist;
		}
	}

	// ent->s.time will carry the 'time' value
	if (G_SpawnFloat("time", "0.5", &ftime))
	{
		if (ftime >= 0)
		{
			ent->s.time = (int)(ftime * 1000); // sec to ms
		}
	}
}

//==========================================================

/**
 * @brief QUAKED target_counter (1 1 0) (-8 -8 -8) (8 8 8)
 * Increments the counter pointed to.
 * "count" is the key for the count value
 * @param[out] ent
 */
void SP_target_counter(gentity_t *ent)
{
//	G_Printf("target counter created with val of: %d\n", ent->count);
	ent->use = Use_Target_Counter;
}

//==========================================================

/**
 * @brief SP_target_lock
 * @details QUAKED target_lock (1 1 0) (-8 -8 -8) (8 8 8)
 * Sets the door to a state requiring key n
 * "key" is the required key
 * so:
 * key:0  unlocks the door
 * key:-1 locks the door until a target_lock with key:0
 * key:n  means the door now requires key n
 *
 * @param[out] ent
 */
void SP_target_lock(gentity_t *ent)
{
	ent->use = Use_Target_Lock;
}

/**
 * @brief Use_Target_Alarm
 * @param[in] ent
 * @param[in] other
 * @param activator - unused
 */
void Use_Target_Alarm(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	G_UseTargets(ent, other);
}

/**
 * @brief QUAKED target_alarm (1 1 0) (-4 -4 -4) (4 4 4)
 * does nothing yet (effectively a relay right now)
 * @param[out] ent
 */
void SP_target_alarm(gentity_t *ent)
{
	ent->use = Use_Target_Alarm;
}

/**
 * @brief smoke_think
 * @details QUAKED target_smoke (1 0 0) (-32 -32 -16) (32 32 16) Black White SmokeON Gravity
 * 1 second	= 1000
 * 1 FRAME		= 100
 * delay		= 100 = one millisecond default this is the maximum smoke that will show up
 * time		= 5000 default before the smoke disipates
 * duration	= 2000 before the smoke starts to alpha
 * start_size	= 24 default
 * end_size	= 96 default
 * wait		= default is 50 the rate at which it will travel up
 * shader		= custom shader to use for particles
 * @param ent
 */
void smoke_think(gentity_t *ent)
{
	ent->nextthink = level.time + ent->s.constantLight;

	if (!(ent->spawnflags & 4))
	{
		return;
	}

	if (ent->s.dl_intensity)
	{
		ent->s.dl_intensity--;
		if (!ent->s.dl_intensity)
		{
			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + FRAMETIME;
		}
	}
}

/**
 * @brief smoke_toggle
 * @param[in,out] ent
 * @param self - unused
 * @param activator - unused
 */
void smoke_toggle(gentity_t *ent, gentity_t *self, gentity_t *activator)
{
	if (ent->spawnflags & 4)     // smoke is on turn it off
	{
		ent->spawnflags &= ~4;
		trap_UnlinkEntity(ent);
	}
	else
	{
		ent->spawnflags |= 4;
		trap_LinkEntity(ent);
	}
}

/**
 * @brief smoke_init
 * @param[in,out] ent
 */
void smoke_init(gentity_t *ent)
{
	gentity_t *target;
	vec3_t    vec;

	ent->think     = smoke_think;
	ent->nextthink = level.time + FRAMETIME;

	if (ent->target)
	{
		target = G_Find(NULL, FOFS(targetname), ent->target);
		if (target)
		{
			VectorSubtract(target->s.origin, ent->s.origin, vec);
			VectorCopy(vec, ent->s.origin2);
		}
		else
		{
			VectorSet(ent->s.origin2, 0, 0, 1);
		}
	}
	else
	{
		VectorSet(ent->s.origin2, 0, 0, 1);
	}

	if (ent->spawnflags & 4)
	{
		trap_LinkEntity(ent);
	}
}

/**
 * @brief SP_target_smoke
 * @param[in,out] ent
 */
void SP_target_smoke(gentity_t *ent)
{
	char *buffer;

	if (G_SpawnString("shader", "", &buffer))
	{
		ent->s.modelindex2 = G_ShaderIndex(buffer);
	}
	else
	{
		ent->s.modelindex2 = 0;
	}

	// modified this a lot to be sent to the client as one entity and then is shown at the client
	if (ent->delay == 0.f)
	{
		ent->delay = 100;
	}

	ent->use = smoke_toggle;

	ent->think     = smoke_init;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_SMOKER;

	if (ent->spawnflags & 2)
	{
		ent->s.density = 4;
	}
	else
	{
		ent->s.density = 0;
	}

	// using "time"
	ent->s.time = ent->speed;
	if (!ent->s.time)
	{
		ent->s.time = 5000; // 5 seconds

	}
	ent->s.time2 = ent->duration;
	if (!ent->s.time2)
	{
		ent->s.time2 = 2000;
	}

	ent->s.angles2[0] = ent->start_size;
	if (ent->s.angles2[0] == 0.f)
	{
		ent->s.angles2[0] = 24;
	}

	ent->s.angles2[1] = ent->end_size;
	if (ent->s.angles2[1] == 0.f)
	{
		ent->s.angles2[1] = 96;
	}

	ent->s.angles2[2] = ent->wait;
	if (ent->s.angles2[2] == 0.f)
	{
		ent->s.angles2[2] = 50;
	}

	// idiot check
	if (ent->s.time < ent->s.time2)
	{
		ent->s.time = ent->s.time2 + 100;
	}

	if (ent->spawnflags & 8)
	{
		ent->s.frame = 1;
	}

	ent->s.dl_intensity  = ent->health;
	ent->s.constantLight = ent->delay;

	if (ent->spawnflags & 4)
	{
		trap_LinkEntity(ent);
	}
}

/**
 * @brief When used it will fire its targets
 * @details QUAKED target_script_trigger (1 .7 .2) (-8 -8 -8) (8 8 8)
 * must have an aiName
 * must have a target
 *
 * @param[in] ent
 * @param[in] other
 *
 * @param activator - unused
 */
void target_script_trigger_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	qboolean found = qfalse;
	// for all entities/bots with this ainame
	gentity_t *trent = NULL;

	// Are we using ainame to find another ent instead of using scriptname for this one?
	if (ent->aiName)
	{
		// Find the first entity with this name
		trent = G_Find(trent, FOFS(scriptName), ent->aiName);

		// Was there one?
		if (trent)
		{
			// We found it
			found = qtrue;

			// Play the script
			G_Script_ScriptEvent(trent, "trigger", ent->target);

		} // if (trent)...

	} // if (ent->aiName)...

	// Use the old method if we didn't find an entity with the ainame
	if (!found)
	{
		if (ent->scriptName)
		{
			G_Script_ScriptEvent(ent, "trigger", ent->target);
		}
	}

	G_UseTargets(ent, other);
}

/**
 * @brief SP_target_script_trigger
 * @param[out] ent
 */
void SP_target_script_trigger(gentity_t *ent)
{
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;
	ent->use       = target_script_trigger_use;
}

// note: Unused
//int rumble_snd;

/**
 * @brief target_rumble_think
 * @details QUAKED target_rumble (0 0.75 0.8) (-8 -8 -8) (8 8 8) STARTOFF
 * wait = default is 2 seconds = time the entity will enable rumble effect
 * "pitch" value from 1 to 10 default is 5
 * "yaw"   value from 1 to 10 default is 5
 *
 * "rampup" how much time it will take to reach maximum pitch and yaw in seconds
 * "rampdown" how long till effect ends after rampup is reached in seconds
 *
 * "startnoise" startingsound
 * "noise"  the looping sound entity is to make
 * "endnoise" endsound
 *
 * "duration" the amount of time the effect is to last ei 1.0 sec 3.6 sec
 *
 * @param[in,out] ent
 */
void target_rumble_think(gentity_t *ent)
{
	float    ratio;
	float    dapitch, dayaw;
	qboolean validrumble = qtrue;

	if (!(ent->count))
	{
		ent->timestamp = level.time;
		ent->count++;
		// start sound here
		if (ent->soundPos1)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos1);
		}
	}
	else
	{
		// looping sound
		ent->s.loopSound = ent->soundLoop;
	}

	dapitch = ent->delay;
	dayaw   = ent->random;
	ratio   = 1.0f;

	if (ent->start_size)
	{
		int time, time2;

		if (level.time < (ent->timestamp + ent->start_size))
		{
			time  = level.time - ent->timestamp;
			time2 = (ent->timestamp + ent->start_size) - ent->timestamp;
			ratio = time / time2;
		}
		else if (level.time < (ent->timestamp + ent->end_size + ent->start_size))
		{
			time  = level.time - ent->timestamp;
			time2 = (ent->timestamp + ent->start_size + ent->end_size) - ent->timestamp;
			ratio = time2 / time;   // FIXME: div / 0
		}
		else
		{
			validrumble = qfalse;
		}
	}

	if (validrumble)
	{
		gentity_t *tent = G_TempEntity(ent->r.currentOrigin, EV_RUMBLE_EFX);

		tent->s.angles[0] = dapitch * ratio;
		tent->s.angles[1] = dayaw * ratio;
	}

	// end sound
	if (level.time > ent->duration + ent->timestamp)
	{
		if (ent->soundPos2)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos2);
			ent->s.loopSound = 0;
		}

		ent->nextthink = 0;
	}
	else
	{
		ent->nextthink = level.time + 50;
	}
}

/**
 * @brief target_rumble_use
 * @param[out] ent
 * @param other - unused
 * @param activator - unused
 */
void target_rumble_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->spawnflags & 1)
	{
		ent->spawnflags &= ~1;
		ent->think       = target_rumble_think;
		ent->count       = 0;
		ent->nextthink   = level.time + 50;
	}
	else
	{
		ent->spawnflags |= 1;
		ent->think       = NULL;
		ent->count       = 0;
	}
}

/**
 * @brief SP_target_rumble
 * @param[in,out] self
 */
void SP_target_rumble(gentity_t *self)
{
	char  *pitch;
	char  *yaw;
	char  *rampup;
	char  *rampdown;
	float dapitch;
	float dayaw;
	char  *sound;
	char  *startsound;
	char  *endsound;

	if (G_SpawnString("noise", "", &sound))
	{
		self->soundLoop = G_SoundIndex(sound);
	}

	if (G_SpawnString("startnoise", "", &startsound))
	{
		self->soundPos1 = G_SoundIndex(startsound);
	}

	if (G_SpawnString("endnoise", "", &endsound))
	{
		self->soundPos2 = G_SoundIndex(endsound);
	}

	self->use = target_rumble_use;

	G_SpawnString("pitch", "0", &pitch);
	dapitch     = (float)(atof(pitch));
	self->delay = dapitch;
	if (self->delay == 0.f)
	{
		self->delay = 5;
	}

	G_SpawnString("yaw", "0", &yaw);
	dayaw        = (float)(atof(yaw));
	self->random = dayaw;
	if (self->random == 0.f)
	{
		self->random = 5;
	}

	G_SpawnString("rampup", "0", &rampup);
	self->start_size = Q_atoi(rampup) * 1000;
	if (!self->start_size)
	{
		self->start_size = 1000;
	}

	G_SpawnString("rampdown", "0", &rampdown);
	self->end_size = Q_atoi(rampdown) * 1000;
	if (!self->end_size)
	{
		self->end_size = 1000;
	}

	if (self->duration == 0.f)
	{
		self->duration = 1000;
	}
	else
	{
		self->duration *= 1000;
	}

	trap_LinkEntity(self);
}
