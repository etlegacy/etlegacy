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
 * @file g_props.c
 */

#include "g_local.h"

/**
 * @brief DropToFloorG
 * @param[in,out] ent
 */
void DropToFloorG(gentity_t *ent)
{
	vec3_t  dest;
	trace_t tr;

	VectorSet(dest, ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2] - 4096);
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID);

	if (tr.startsolid)
	{
		return;
	}

	ent->s.groundEntityNum = tr.entityNum;
	G_SetOrigin(ent, tr.endpos);
	ent->nextthink = level.time + FRAMETIME;
}

/**
 * @brief DropToFloor
 * @param[in,out] ent
 */
void DropToFloor(gentity_t *ent)
{
	vec3_t  dest;
	trace_t tr;

	VectorSet(dest, ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2] - 4096);
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID);

	if (tr.startsolid)
	{
		return;
	}

	if (Q_fabs((ent->r.currentOrigin[2] - tr.endpos[2])) > 1.0f)
	{
		tr.endpos[2] = (ent->r.currentOrigin[2] - 1.0f);
	}

	ent->s.groundEntityNum = tr.entityNum;

	G_SetOrigin(ent, tr.endpos);

	ent->think     = DropToFloorG;
	ent->nextthink = level.time + FRAMETIME;
}

/**
 * @brief moveit
 * @param[in,out] ent
 * @param[in] yaw
 * @param[in] dist
 */
void moveit(gentity_t *ent, float yaw, float dist)
{
	vec3_t  move;
	vec3_t  origin;
	trace_t tr;
	vec3_t  mins, maxs;

	yaw = (float)(yaw * M_TAU_F / 360);

	move[0] = (float)cos((double)yaw) * dist;
	move[1] = (float)sin((double)yaw) * dist;
	move[2] = 0;

	VectorAdd(ent->r.currentOrigin, move, origin);

	mins[0] = ent->r.mins[0];
	mins[1] = ent->r.mins[1];
	mins[2] = ent->r.mins[2] + .01f;

	maxs[0] = ent->r.maxs[0];
	maxs[1] = ent->r.maxs[1];
	maxs[2] = ent->r.maxs[2] - .01f;

	trap_Trace(&tr, ent->r.currentOrigin, mins, maxs, origin, ent->s.number, MASK_SHOT);

	if ((tr.endpos[0] != origin[0]) || (tr.endpos[1] != origin[1]))
	{
		mins[0] = ent->r.mins[0] - 2.0f;
		mins[1] = ent->r.mins[1] - 2.0f;
		maxs[0] = ent->r.maxs[0] + 2.0f;
		maxs[1] = ent->r.maxs[1] + 2.0f;

		trap_Trace(&tr, ent->r.currentOrigin, mins, maxs, origin, ent->s.number, MASK_SHOT);
	}

	VectorCopy(tr.endpos, ent->r.currentOrigin);

	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

	trap_LinkEntity(ent);
}

/**
 * @brief touch_props_box_32
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void touch_props_box_32(gentity_t *self, gentity_t *other, trace_t *trace)
{
	float  ratio;
	vec3_t v;

	if (other->r.currentOrigin[2] > (self->r.currentOrigin[2] + 10 + 15))
	{
		return;
	}

	ratio = 2.5f;
	VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, v);
	moveit(self, vectoyaw(v), (20 * ratio * FRAMETIME) * .001f);
}

/**
 * @brief SP_props_box_32
 * QUAKED props_box_32 (1 0 0) (-16 -16 -16) (16 16 16)
 * @param[out] self
 */
void SP_props_box_32(gentity_t *self)
{
	self->s.modelindex = G_ModelIndex("models/mapobjects/boxes/box32.md3");

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;
	self->r.svFlags  = 0;

	VectorSet(self->r.mins, -16, -16, -16);
	VectorSet(self->r.maxs, 16, 16, 16);

	self->touch = touch_props_box_32;

	trap_LinkEntity(self);

	self->think     = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

/**
 * @brief touch_props_box_48
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void touch_props_box_48(gentity_t *self, gentity_t *other, trace_t *trace)
{
	float  ratio;
	vec3_t v;

	if (other->r.currentOrigin[2] > (self->r.currentOrigin[2] + 10 + 23))
	{
		return;
	}

	ratio = 2.0;
	VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, v);
	moveit(self, vectoyaw(v), (20 * ratio * FRAMETIME) * .001f);
}

/**
 * @brief SP_props_box_48
 * QUAKED props_box_48 (1 0 0) (-24 -24 -24) (24 24 24)
 * @param[out] self
 */
void SP_props_box_48(gentity_t *self)
{
	self->s.modelindex = G_ModelIndex("models/mapobjects/boxes/box48.md3");

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;
	self->r.svFlags  = 0;

	VectorSet(self->r.mins, -24, -24, -24);
	VectorSet(self->r.maxs, 24, 24, 24);

	self->touch = touch_props_box_48;

	trap_LinkEntity(self);

	self->think     = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

/**
 * @brief touch_props_box_64
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void touch_props_box_64(gentity_t *self, gentity_t *other, trace_t *trace)
{
	float  ratio;
	vec3_t v;

	if (other->r.currentOrigin[2] > (self->r.currentOrigin[2] + 10 + 31))
	{
		return;
	}

	ratio = 1.5;
	VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, v);
	moveit(self, vectoyaw(v), (20 * ratio * FRAMETIME) * .001f);
}

/**
 * @brief SP_props_box_64
 * QUAKED props_box_64 (1 0 0) (-32 -32 -32) (32 32 32)
 * @param[out] self
 */
void SP_props_box_64(gentity_t *self)
{
	self->s.modelindex = G_ModelIndex("models/mapobjects/boxes/box64.md3");

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;
	self->r.svFlags  = 0;

	VectorSet(self->r.mins, -32, -32, -32);
	VectorSet(self->r.maxs, 32, 32, 32);

	self->touch = touch_props_box_64;

	trap_LinkEntity(self);

	self->think     = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

/**
 * @brief Psmoke_think
 * @param[out] ent
 */
void Psmoke_think(gentity_t *ent)
{
	gentity_t *tent;

	ent->count++;

	if (ent->count == 30)
	{
		ent->think = G_FreeEntity;
	}

	tent = G_TempEntity(ent->s.origin, EV_SMOKE);
	VectorCopy(ent->s.origin, tent->s.origin);
	tent->s.time       = 3000;
	tent->s.time2      = 100;
	tent->s.density    = 0;
	tent->s.angles2[0] = 4;
	tent->s.angles2[1] = 32;
	tent->s.angles2[2] = 50;

	ent->nextthink = level.time + FRAMETIME;
}

/**
 * @brief prop_smoke
 * @param[in] ent
 *
 * @note Unused
 */
void prop_smoke(gentity_t *ent)
{
	gentity_t *Psmoke;

	Psmoke = G_Spawn();

	VectorCopy(ent->r.currentOrigin, Psmoke->s.origin);
	Psmoke->think     = Psmoke_think;
	Psmoke->nextthink = level.time + FRAMETIME;
}

/**
 * @brief PGUNsparks_use
 *
 * @details QUAKED props_sparks (.8 .46 .16) (-8 -8 -8) (8 8 8) ELECTRIC
 * the default direction is strait up use info_no_null for alt direction
 *
 * delay = how long till next spark effect
 * wait = life of the spark with some random variance default 1.0 sec
 * health = random number of sparks upto specified amount default 8
 *
 * start_size default 8 along the x
 * end_size default 8 along the y
 * by changing the size will change the spawn origin of the individual spark
 * ei 16 x 8 or 24 x 32 would cause the sparks to spawn that many units from
 * the origin
 *
 * speed controls how quickly the sparks will travel default is 2
 *
 * @param[in] ent
 * @param self - unused
 * @param activator - unused
 */
void PGUNsparks_use(gentity_t *ent, gentity_t *self, gentity_t *activator)
{
	gentity_t *tent;

	tent = G_TempEntity(ent->r.currentOrigin, EV_GUNSPARKS);
	VectorCopy(ent->r.currentOrigin, tent->s.origin);
	VectorCopy(ent->r.currentAngles, tent->s.angles);
	tent->s.density    = ent->health;
	tent->s.angles2[2] = ent->speed;
}

/**
 * @brief Psparks_think
 * @param[in] ent
 *
 * @todo FIXME: MOVE TO CLIENT!
 */
void Psparks_think(gentity_t *ent)
{
	//gentity_t *tent;

	// FIXME MOVE TO CLIENT!
	return;

	/*
	if (ent->spawnflags & 1)
	{
	    tent = G_TempEntity(ent->r.currentOrigin, EV_SPARKS_ELECTRIC);
	}
	else
	{
	    tent = G_TempEntity(ent->r.currentOrigin, EV_SPARKS);
	}
	VectorCopy(ent->r.currentOrigin, tent->s.origin);
	VectorCopy(ent->r.currentAngles, tent->s.angles);
	tent->s.density    = ent->health;
	tent->s.frame      = ent->wait;
	tent->s.angles2[0] = ent->start_size;
	tent->s.angles2[1] = ent->end_size;
	tent->s.angles2[2] = ent->speed;

	ent->nextthink = level.time + FRAMETIME + ent->delay + (rand() % 600);
	*/
}

/**
 * @brief sparks_angles_think
 * @param[in,out] ent
 */
void sparks_angles_think(gentity_t *ent)
{
	gentity_t *target = NULL;

	if (ent->target)
	{
		target = G_FindByTargetname(NULL, ent->target);
	}

	if (!target)
	{
		VectorSet(ent->r.currentAngles, 0, 0, 1);
	}
	else
	{
		vec3_t vec;

		VectorSubtract(ent->s.origin, target->s.origin, vec);
		VectorNormalize(vec);
		VectorCopy(vec, ent->r.currentAngles);
	}

	trap_LinkEntity(ent);

	ent->nextthink = level.time + FRAMETIME;
	if (!Q_stricmp(ent->classname, "props_sparks"))
	{
		ent->think = Psparks_think;
	}
	else
	{
		ent->use = PGUNsparks_use;
	}
}

/**
 * @brief SP_props_sparks
 * @param[out] ent
 */
void SP_props_sparks(gentity_t *ent)
{
	// don't use in multiplayer right now since it makes decyphering net messages almost impossible
	ent->think = G_FreeEntity;
	return;

	/* FIXME
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;

	ent->think     = sparks_angles_think;
	ent->nextthink = level.time + FRAMETIME;

	if (!ent->health)
	{
	    ent->health = 8;
	}

	if (!ent->wait)
	{
	    ent->wait = 1200;
	}
	else
	{
	    ent->wait *= 1000;
	}

	if (!ent->start_size)
	{
	    ent->start_size = 8;
	}

	if (!ent->end_size)
	{
	    ent->end_size = 8;
	}

	if (!ent->speed)
	{
	    ent->speed = 2;
	}

	trap_LinkEntity(ent);
	*/
}

/**
 * @brief SP_props_gunsparks
 *
 * @details QUAKED props_gunsparks (.8 .46 .16) (-8 -8 -8) (8 8 8)
 * the default direction is strait up use info_no_null for alt direction
 *
 * this entity must be used to see the effect
 *
 * "speed" default is 20
 * "health" number to spawn default is 4
 *
 * @param ent
 */
void SP_props_gunsparks(gentity_t *ent)
{
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;

	ent->think     = sparks_angles_think;
	ent->nextthink = level.time + FRAMETIME;

	if (ent->speed == 0.f)
	{
		ent->speed = 20;
	}

	if (!ent->health)
	{
		ent->health = 4;
	}

	trap_LinkEntity(ent);
}

/**
 * @brief QUAKED props_smokedust (.8 .46 .16) (-8 -8 -8) (8 8 8)
 * health = how many pieces 16 is default
 * @param[in] ent
 * @param self - unused
 * @param activator - unused
 */
void smokedust_use(gentity_t *ent, gentity_t *self, gentity_t *activator)
{
	int       i;
	gentity_t *tent;
	vec3_t    forward;

	AngleVectors(ent->r.currentAngles, forward, NULL, NULL);

	for (i = 0; i < ent->health; i++)
	{
		tent = G_TempEntity(ent->r.currentOrigin, EV_SMOKE);
		VectorCopy(ent->r.currentOrigin, tent->s.origin);
		VectorCopy(forward, tent->s.origin2);
		tent->s.time    = 1000;
		tent->s.time2   = 750;
		tent->s.density = 3;
	}
}

/**
 * @brief SP_SmokeDust
 * @param[in,out] ent
 */
void SP_SmokeDust(gentity_t *ent)
{
	ent->use = smokedust_use;

	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;

	if (!ent->health)
	{
		ent->health = 16;
	}
	trap_LinkEntity(ent);
}

/**
 * @brief dust_use
 *
 * @details QUAKED props_dust (.7 .3 .16) (-8 -8 -8) (8 8 8) WHITE
 * you should give this ent a target use a not null
 * or you could set its angles in the editor
 *
 * @param[in] ent
 * @param self - unused
 * @param activator - unused
 */
void dust_use(gentity_t *ent, gentity_t *self, gentity_t *activator)
{
	gentity_t *tent;

	if (ent->target)
	{
		tent = G_TempEntity(ent->r.currentOrigin, EV_DUST);
		VectorCopy(ent->r.currentOrigin, tent->s.origin);
		VectorCopy(ent->r.currentAngles, tent->s.angles);
		if (ent->spawnflags & 1)
		{
			tent->s.density = 1;
		}
	}
	else
	{
		vec3_t forward;

		AngleVectors(ent->r.currentAngles, forward, NULL, NULL);

		tent = G_TempEntity(ent->r.currentOrigin, EV_DUST);
		VectorCopy(ent->r.currentOrigin, tent->s.origin);
		VectorCopy(forward, tent->s.angles);
		if (ent->spawnflags & 1)
		{
			tent->s.density = 1;
		}
	}
}

/**
 * @brief dust_angles_think
 * @param[in] ent
 */
void dust_angles_think(gentity_t *ent)
{
	gentity_t *target;
	vec3_t    vec;

	target = G_FindByTargetname(NULL, ent->target);

	if (!target)
	{
		return;
	}

	VectorSubtract(ent->s.origin, target->s.origin, vec);
	VectorCopy(vec, ent->r.currentAngles);
	trap_LinkEntity(ent);
}

/**
 * @brief SP_Dust
 * @param[in,out] ent
 */
void SP_Dust(gentity_t *ent)
{
	ent->use = dust_use;
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;

	if (ent->target)
	{
		ent->think     = dust_angles_think;
		ent->nextthink = level.time + FRAMETIME;
	}

	trap_LinkEntity(ent);
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

extern void G_ExplodeMissile(gentity_t *ent);

/*
 * @brief propExplosionLarge
 * @param ent
 *
 * @note unused
void propExplosionLarge(gentity_t *ent)
{
    gentity_t *bolt = G_Spawn();

    // for explosion type
    bolt->accuracy = 2;

    bolt->classname = "props_explosion_large";
    bolt->nextthink = level.time + FRAMETIME;
    bolt->think     = G_ExplodeMissile;
    bolt->s.eType   = ET_MISSILE;
    bolt->r.svFlags = 0;

    bolt->s.weapon = WP_NONE;

    bolt->s.eFlags            = EF_BOUNCE_HALF;
    bolt->r.ownerNum          = ent->s.number;
    bolt->parent              = ent;
    bolt->damage              = ent->health;
    bolt->splashDamage        = ent->health;
    bolt->splashRadius        = ent->health * 1.5;
    bolt->methodOfDeath       = MOD_GRENADE;
    bolt->splashMethodOfDeath = MOD_GRENADE;
    bolt->clipmask            = MASK_SHOT;

    VectorCopy(ent->r.currentOrigin, bolt->s.pos.trBase);
    VectorCopy(ent->r.currentOrigin, bolt->r.currentOrigin);
}
*/

void G_ExplodeMissile(gentity_t *ent);

/**
 * @brief propExplosion
 * @param[in] ent
 */
void propExplosion(gentity_t *ent)
{
	gentity_t *bolt;

	bolt = G_Spawn();

	bolt->classname = "props_explosion";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think     = G_ExplodeMissile;
	bolt->s.eType   = ET_MISSILE;
	bolt->r.svFlags = 0;

	// storing explosion type
	bolt->accuracy = 1;

	bolt->s.weapon = WP_NONE;

	bolt->s.eFlags            = EF_BOUNCE_HALF;
	bolt->r.ownerNum          = ent->s.number;
	bolt->parent              = ent;
	bolt->damage              = ent->health;
	bolt->splashDamage        = ent->health;
	bolt->splashRadius        = (int)(ent->health * 1.5);
	bolt->methodOfDeath       = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE;
	bolt->clipmask            = MASK_SHOT;

	VectorCopy(ent->r.currentOrigin, bolt->s.pos.trBase);
	VectorCopy(ent->r.currentOrigin, bolt->r.currentOrigin);
}

/**
 * @brief InitProp
 * @param[in,out] ent
 */
void InitProp(gentity_t *ent)
{
	float    light;
	vec3_t   color;
	qboolean lightSet, colorSet;
	char     *sound;

	// FIXME: the following models are missing in genuine ET pk3s - relics ?

	if (!Q_stricmp(ent->classname, "props_bench"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/furniture/bench/bench_sm.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_radio"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/mapobjects/electronics/radio1.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_locker_tall"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/furniture/storage/lockertall.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_flippy_table"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/furniture/table/woodflip.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_crate_32x64"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/furniture/crate/crate32x64.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_58x112tablew"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/furniture/table/56x112tablew.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_castlebed"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/furniture/bed/castlebed.md3");
	}
	else if (!Q_stricmp(ent->classname, "props_radioSEVEN"))
	{
		ent->s.modelindex2 = G_ModelIndex("models/mapobjects/electronics/radios.md3");
	}

	// if the "loopsound" key is set, use a constant looping sound when moving
	if (G_SpawnString("noise", "100", &sound))
	{
		ent->s.loopSound = G_SoundIndex(sound);
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat("light", "100", &light);
	colorSet = G_SpawnVector("color", "1 1 1", color);
	if (lightSet || colorSet)
	{
		int r, g, b, i;

		r = (int)(color[0] * 255);
		if (r > 255)
		{
			r = 255;
		}
		g = (int)(color[1] * 255);
		if (g > 255)
		{
			g = 255;
		}
		b = (int)(color[2] * 255);
		if (b > 255)
		{
			b = 255;
		}
		i = (int)(light / 4);
		if (i > 255)
		{
			i = 255;
		}
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}

	ent->isProp = qtrue;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);
}

/**
 * @brief props_bench_think
 * @param[in,out] ent
 */
void props_bench_think(gentity_t *ent)
{
	ent->s.frame++;

	if (ent->s.frame < 28)
	{
		ent->nextthink = level.time + (FRAMETIME / 2);
	}
	else
	{
		ent->clipmask   = 0;
		ent->r.contents = 0;
		ent->takedamage = qfalse;

		G_UseTargets(ent, NULL);
	}
}

/**
 * @brief props_bench_die
 * @param[out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void props_bench_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think     = props_bench_think;
	ent->nextthink = level.time + FRAMETIME;
}

/**
 * @brief SP_Props_Bench
 *
 * @details QUAKED props_bench (.8 .6 .2) ?
 * requires an origin brush
 * health = 10 by default
 *
 * @param ent
 */
void SP_Props_Bench(gentity_t *ent)
{
	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_bench_die;

	trap_LinkEntity(ent);
}

/**
 * @brief props_radio_die
 * @param[in,out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void props_radio_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	propExplosion(ent);

	ent->takedamage = qfalse;

	G_UseTargets(ent, NULL);

	G_FreeEntity(ent);
}

/**
 * @brief SP_Props_Radio
 *
 * @details QUAKED props_radio (.8 .6 .2) ?
 * requires an origin brush
 * health = defaults to 100
 *
 * @param[in,out] ent
 */
void SP_Props_Radio(gentity_t *ent)
{
	if (!ent->model)
	{
		G_Printf(S_COLOR_RED "props_radio with NULL model\n");
		return;
	}

	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 100;
	}

	ent->takedamage = qtrue;

	ent->die = props_radio_die;

	trap_LinkEntity(ent);
}

/**
 * @brief props_radio_dieSEVEN
 * @param[in,out] ent
 * @param[in] inflictor
 * @param attacker - unused
 * @param damage   - unused
 * @param mod      - unused
 */
void props_radio_dieSEVEN(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	int i;

	propExplosion(ent);

	for (i = 0; i < 20; i++)
	{
		Spawn_Shard(ent, inflictor, 1, ent->count);
	}

	Prop_Break_Sound(ent);

	ent->takedamage = qfalse;
	ent->die        = NULL;

	trap_LinkEntity(ent);

	G_UseTargets(ent, NULL);

	G_FreeEntity(ent);
}

/**
 * @brief SP_Props_RadioSEVEN
 *
 * @details QUAKED props_radioSEVEN (.8 .6 .2) ?
 * requires an origin brush
 * health = defaults to 100
 *
 *   the models dims are
 *   x 32
 *   y 136
 *   z 32
 *
 *   if you want more explosions you'll need func explosive
 *
 *   it will fire all its targets upon death
 *
 * @param[in,out] ent
 */
void SP_Props_RadioSEVEN(gentity_t *ent)
{
	if (!ent->model)
	{
		G_Printf(S_COLOR_RED "props_radio with NULL model\n");
		return;
	}

	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 100;
	}

	ent->takedamage = qtrue;

	ent->die = props_radio_dieSEVEN;

	ent->count = FXTYPE_METAL; // metal shard and sound

	trap_LinkEntity(ent);
}

/**
 * @brief locker_tall_think
 * @param[in,out] ent
 */
void locker_tall_think(gentity_t *ent)
{
	if (ent->s.frame == 30)
	{
		G_UseTargets(ent, NULL);

	}
	else
	{
		ent->s.frame++;
		ent->nextthink = level.time + (FRAMETIME / 2);
	}
}

/**
 * @brief props_locker_tall_die
 * @param[out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void props_locker_tall_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think     = locker_tall_think;
	ent->nextthink = level.time + FRAMETIME;

	ent->takedamage = qfalse;

	G_UseTargets(ent, NULL);
}

/**
 * @brief SP_Props_Locker_Tall
 *
 * @details QUAKED props_locker_tall (.8 .6 .2) ?
 * requires an origin brush
 *
 * @param ent
 */
void SP_Props_Locker_Tall(gentity_t *ent)
{
	if (!ent->model)
	{
		G_Printf(S_COLOR_RED "props_locker_tall with NULL model\n");
		return;
	}

	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 100;
	}

	ent->takedamage = qtrue;

	ent->die = props_locker_tall_die;

	trap_LinkEntity(ent);
}

/*
QUAKED props_chair_chat(.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
    FXTYPE_WOOD = 0,
    FXTYPE_GLASS = 1,
    FXTYPE_METAL = 2

*/

/*
QUAKED props_chair_chatarm(.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
    FXTYPE_WOOD = 0,
    FXTYPE_GLASS = 1,
    FXTYPE_METAL = 2

*/

/*
QUAKED props_chair_side (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
    FXTYPE_WOOD = 0,
    FXTYPE_GLASS = 1,
    FXTYPE_METAL = 2

*/


/*
QUAKED props_chair_hiback (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
    FXTYPE_WOOD = 0,
    FXTYPE_GLASS = 1,
    FXTYPE_METAL = 2

*/

/*
QUAKED props_chair (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
    FXTYPE_WOOD = 0,
    FXTYPE_GLASS = 1,
    FXTYPE_METAL = 2

*/
void Props_Chair_Think(gentity_t *self);
void Props_Chair_Touch(gentity_t *self, gentity_t *other, trace_t *trace);
void Props_Chair_Die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod);

/**
 * @brief Just_Got_Thrown
 * @param[in,out] self
 */
void Just_Got_Thrown(gentity_t *self)
{
	float len = 0;

	if (self->s.groundEntityNum == -1)
	{
		self->nextthink = level.time + FRAMETIME;

		if (self->enemy)
		{
			G_Damage(self->enemy, self, self, NULL, NULL, 5, 0, MOD_CRUSH);

			self->die = Props_Chair_Die;

			self->die(self, self, NULL, 10, MOD_UNKNOWN);
		}

		return;
	}
	else
	{
		len = Distance(self->r.currentOrigin, self->s.origin2);

		{
			trace_t trace;
			vec3_t  end;
			//gentity_t *traceEnt;

			VectorCopy(self->r.currentOrigin, end);
			end[2] += 1;

			trap_Trace(&trace, self->r.currentOrigin, self->r.mins, self->r.maxs, end, self->s.number, MASK_SHOT);

			//traceEnt = &g_entities[trace.entityNum];

			if (trace.startsolid)
			{
				len = 9999;
			}
		}
	}

	self->think      = Props_Chair_Think;
	self->touch      = Props_Chair_Touch;
	self->die        = Props_Chair_Die;
	self->s.eType    = ET_MOVER;
	self->s.dmgFlags = HINT_CHAIR;  // so client knows what kind of mover it is for cursorhints

	self->nextthink = level.time + FRAMETIME;

	self->r.ownerNum = self->s.number;

	if (len > 256)
	{
		self->die(self, self, NULL, 10, 0);
	}
}

/**
 * @brief Props_TurnLightsOff
 * @param[in,out] self
 */
void Props_TurnLightsOff(gentity_t *self)
{
	if (!Q_stricmp(self->classname, "props_desklamp"))
	{
		if (self->target)
		{
			G_UseTargets(self, NULL);
			self->target = NULL;
		}
	}
}

/**
 * @brief Props_Activated
 * @param[in,out] self
 */
void Props_Activated(gentity_t *self)
{
	vec3_t angles;
	vec3_t dest;
	vec3_t forward, right;

	gentity_t *owner = &g_entities[self->r.ownerNum];;

	self->nextthink = level.time + 50;

	if (!owner->client)
	{
		return;
	}

	Props_TurnLightsOff(self);

	if (owner->active == qfalse)
	{
		vec3_t velocity;
		vec3_t prop_ang;

		gentity_t *prop;

		self->physicsObject = qtrue;
		self->physicsBounce = 0.2f;

		self->s.groundEntityNum = -1;

		self->s.pos.trType = TR_GRAVITY;
		self->s.pos.trTime = level.time;

		self->active = qfalse;

		AngleVectors(owner->client->ps.viewangles, velocity, NULL, NULL);
		VectorScale(velocity, 250, velocity);
		velocity[2] += 100 + crandom() * 25;
		VectorCopy(velocity, self->s.pos.trDelta);

		self->think     = NULL;
		self->nextthink = 0;

		prop               = G_Spawn();
		prop->s.modelindex = self->s.modelindex;
		G_SetOrigin(prop, self->r.currentOrigin);

		VectorCopy(owner->client->ps.viewangles, prop_ang);
		prop_ang[0] = 0;

		G_SetAngle(prop, prop_ang);

		prop->clipmask   = CONTENTS_SOLID;
		prop->r.contents = CONTENTS_SOLID;
		prop->r.svFlags  = 0;
		prop->isProp     = qtrue;

		VectorSet(prop->r.mins, -12, -12, 0);
		VectorSet(prop->r.maxs, 12, 12, 48);

		prop->physicsObject = qtrue;
		prop->physicsBounce = 0.2f;

		VectorCopy(owner->client->ps.origin, prop->s.pos.trBase);

		VectorCopy(self->s.pos.trDelta, prop->s.pos.trDelta);

		prop->s.pos.trType = TR_GRAVITY;
		prop->s.pos.trTime = level.time;

		prop->active = qfalse;

		prop->health = self->health;

		prop->duration = self->health;

		prop->count = self->count;

		prop->think     = Just_Got_Thrown;
		prop->nextthink = level.time + FRAMETIME;

		prop->takedamage = qtrue;

		prop->wait = self->wait;

		prop->classname = self->classname;

		prop->s.groundEntityNum = -1;

		VectorCopy(self->r.currentOrigin, prop->s.origin2);

		prop->die = Props_Chair_Die;

		prop->r.ownerNum = owner->s.number;

		trap_LinkEntity(prop);

		G_FreeEntity(self);

		return;
	}
	else
	{
		if (!Q_stricmp(self->classname, "props_chair_hiback"))
		{
			self->s.frame   = 23;
			self->s.density = 1;
		}
		else if (!Q_stricmp(self->classname, "props_chair"))
		{
			self->s.frame   = 28;
			self->s.density = 1;
		}
		else if (!Q_stricmp(self->classname, "props_chair_side"))
		{
			self->s.frame   = 23;
			self->s.density = 1;
		}
	}

	trap_UnlinkEntity(self);

	// move the entity in step with the activators movement
	VectorCopy(owner->client->ps.viewangles, angles);
	angles[0] = 0;

	self->s.apos.trBase[YAW] = owner->client->ps.viewangles[YAW];

	AngleVectors(angles, forward, right, NULL);
	VectorCopy(owner->r.currentOrigin, dest);

	VectorCopy(dest, self->r.currentOrigin);
	VectorCopy(dest, self->s.pos.trBase);

	self->s.eType = ET_PROP;

	trap_LinkEntity(self);
}

void Prop_Check_Ground(gentity_t *self);

/**
 * @brief Props_Chair_Think
 * @param[in,out] self
 */
void Props_Chair_Think(gentity_t *self)
{
	trace_t tr;

	if (self->active)
	{
		Props_Activated(self);
		return;
	}

	//trap_UnlinkEntity (self);

	BG_EvaluateTrajectory(&self->s.pos, level.time, self->s.pos.trBase, qfalse, self->s.effect2Time);

	if (level.time > self->s.pos.trDuration)
	{
		VectorClear(self->s.pos.trDelta);
		self->s.pos.trDuration = 0;
		self->s.pos.trType     = TR_STATIONARY;
	}
	else
	{
		vec3_t mins, maxs;

		VectorCopy(self->r.mins, mins);
		VectorCopy(self->r.maxs, maxs);

		mins[2] += 1;

		trap_Trace(&tr, self->r.currentOrigin, mins, maxs, self->s.pos.trBase, self->s.number, MASK_SHOT);

		if (tr.fraction == 1.f)
		{
			VectorCopy(self->s.pos.trBase, self->r.currentOrigin);
		}
		else
		{
			VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
			VectorClear(self->s.pos.trDelta);
			self->s.pos.trDuration = 0;
			self->s.pos.trType     = TR_STATIONARY;
		}
	}

	if (self->s.groundEntityNum == -1)
	{
		self->physicsObject = qtrue;
		self->physicsBounce = 0.2f;

		self->s.pos.trDelta[2] -= 200;

		self->s.pos.trType = TR_GRAVITY;
		self->s.pos.trTime = level.time;

		self->active = qfalse;

		self->think = Just_Got_Thrown;
	}

	// we assume groundentity will never change from under a stationary object.
	//Prop_Check_Ground (self);

	self->nextthink = level.time + 50;

	// prevent unneeded links
	if (!VectorCompare(self->r.currentOrigin, self->gDelta))
	{
		Prop_Check_Ground(self);
		trap_LinkEntity(self);
		VectorCopy(self->r.currentOrigin, self->gDelta);
	}
}

/**
 * @brief Prop_Touch
 * @param[in,out] self
 * @param[in] other
 * @param[in] v
 * @return
 */
qboolean Prop_Touch(gentity_t *self, gentity_t *other, vec3_t v)
{
	vec3_t  forward;
	vec3_t  dest;
	vec3_t  angle;
	vec3_t  start, end;
	vec3_t  mins, maxs;
	trace_t tr;

	if (!other->client)
	{
		return qfalse;
	}

	vectoangles(v, angle);
	angle[0] = 0;
	AngleVectors(angle, forward, NULL, NULL);
	VectorClear(dest);
	VectorMA(dest, 128, forward, dest);
	VectorMA(self->r.currentOrigin, 32, forward, end);

	VectorCopy(self->r.currentOrigin, start);
	end[2]   += 8;
	start[2] += 8;

	VectorCopy(self->r.mins, mins);
	VectorCopy(self->r.maxs, maxs);

	mins[2] += 1;

	trap_Trace(&tr, start, mins, maxs, end, self->s.number, MASK_SHOT);

	if (tr.fraction != 1.f)
	{
		return qfalse;
	}

	VectorCopy(dest, self->s.pos.trDelta);
	VectorCopy(self->r.currentOrigin, self->s.pos.trBase);

	self->s.pos.trDuration = level.time + FRAMETIME;
	self->s.pos.trTime     = level.time;
	self->s.pos.trType     = TR_LINEAR;

	self->physicsObject = qtrue;

	return qtrue;
}

/**
 * @brief Prop_Check_Ground
 * @param[in,out] self
 */
void Prop_Check_Ground(gentity_t *self)
{
	vec3_t  mins, maxs;
	vec3_t  start, end;
	trace_t tr;

	VectorCopy(self->r.currentOrigin, start);
	VectorCopy(self->r.currentOrigin, end);

	end[2] -= 4;

	VectorCopy(self->r.mins, mins);
	VectorCopy(self->r.maxs, maxs);

	trap_Trace(&tr, start, mins, maxs, end, self->s.number, MASK_SHOT);

	if (tr.fraction == 1.f)
	{
		self->s.groundEntityNum = -1;
	}
}

/**
 * @brief Props_Chair_Touch
 * @param[in,out] self
 * @param[in] other
 * @param trace - unused
 */
void Props_Chair_Touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	vec3_t   v;
	qboolean has_moved;

	if (!other->client)
	{
		return;
	}

	if (other->r.currentOrigin[2] > (self->r.currentOrigin[2] + 10 + 15))
	{
		return;
	}

	if (self->active)     // someone has activated me
	{
		return;
	}

	VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, v);

	has_moved = Prop_Touch(self, other, v);

	Prop_Check_Ground(self);

	if (level.time > self->random && has_moved)
	{
		G_AddEvent(self, EV_GENERAL_SOUND, GAMESOUND_WORLD_CHAIRCREAK);
		self->random = level.time + 1000 + (rand() % 200);
	}

	if (!Q_stricmp(self->classname, "props_desklamp"))
	{
		// player may have picked it up before
		if (self->target)
		{
			G_UseTargets(self, NULL);
			self->target = NULL;
		}
	}
}

/**
 * @brief Props_Chair_Animate
 * @param[in,out] ent
 */
void Props_Chair_Animate(gentity_t *ent)
{
	ent->touch = NULL;

	if (!Q_stricmp(ent->classname, "props_chair"))
	{
		if (ent->s.frame >= 27)
		{
			ent->s.frame = 27;
			G_UseTargets(ent, NULL);
			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + 2000;
			ent->s.time    = level.time;
			ent->s.time2   = level.time + 2000;
			return;
		}
		else
		{
			ent->nextthink = level.time + (FRAMETIME / 2);
		}
	}
	else if (
		(!Q_stricmp(ent->classname, "props_chair_side")) ||
		(!Q_stricmp(ent->classname, "props_chair_chat")) ||
		(!Q_stricmp(ent->classname, "props_chair_chatarm")) ||
		(!Q_stricmp(ent->classname, "props_chair_hiback"))
		)
	{
		if (ent->s.frame >= 20)
		{
			ent->s.frame = 20;
			G_UseTargets(ent, NULL);
			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + 2000;
			ent->s.time    = level.time;
			ent->s.time2   = level.time + 2000;
			return;
		}
		else
		{
			ent->nextthink = level.time + (FRAMETIME / 2);
		}
	}
	else if (!Q_stricmp(ent->classname, "props_desklamp"))
	{
		if (ent->s.frame >= 11)
		{
			// player may have picked it up before
			if (ent->target)
			{
				G_UseTargets(ent, NULL);
			}

			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + 2000;
			ent->s.time    = level.time;
			ent->s.time2   = level.time + 2000;
			return;
		}
		else
		{
			ent->nextthink = level.time + (FRAMETIME / 2);
		}
	}

	ent->s.frame++;

	if (ent->enemy)
	{
		//float  ratio = 2.5f;
		vec3_t v;

		VectorSubtract(ent->r.currentOrigin, ent->enemy->r.currentOrigin, v);
		moveit(ent, vectoyaw(v), (ent->delay * 2.5f * FRAMETIME) * .001f);
	}
}

/**
 * @brief Spawn_Shard
 * @param[in] ent
 * @param[in] inflictor
 * @param[in] quantity
 * @param[in] type
 */
void Spawn_Shard(gentity_t *ent, gentity_t *inflictor, float quantity, int type)
{
	gentity_t *sfx;
	vec3_t    dir, start;

	VectorCopy(ent->r.currentOrigin, start);

	if (!Q_stricmp(ent->classname, "props_radioSEVEN"))
	{
		start[0] += crandom() * 32;
		start[1] += crandom() * 32;
		VectorSubtract(inflictor->r.currentOrigin, ent->r.currentOrigin, dir);
		VectorNormalize(dir);
	}
	else if (inflictor)
	{
		VectorSubtract(inflictor->r.currentOrigin, ent->r.currentOrigin, dir);
		VectorNormalize(dir);
		VectorNegate(dir, dir);
	}
	else
	{
		VectorSet(dir, 0, 0, 1);
	}

	sfx = G_Spawn();

	sfx->s.density = type;

	if (type < 4)
	{
		start[2] += 32;
	}

	G_SetOrigin(sfx, start);
	G_SetAngle(sfx, ent->r.currentAngles);

	G_AddEvent(sfx, EV_SHARD, DirToByte(dir));

	sfx->think = G_FreeEntity;

	sfx->nextthink = level.time + 1000;

	sfx->s.frame = (int)quantity;

	trap_LinkEntity(sfx);
}

/**
 * @brief This should work for all fx types now
 * @param ent
 */
void Prop_Break_Sound(gentity_t *ent)
{
	// don't always play the wood sound
	//G_AddEvent(ent, EV_FX_SOUND, FXTYPE_WOOD);

	// note: props only use 3 types of sounds from the obsolete shard_t enum
	if (ent->count < FXTYPE_WOOD || ent->count >= FXTYPE_MAX)
	{
		return;
	}
	G_AddEvent(ent, EV_FX_SOUND, ent->count);
}

/**
 * @brief Props_Chair_Die
 * @param[in,out] ent
 * @param[in] inflictor
 * @param attacker - unused
 * @param[in] damage
 * @param mod - unused
 */
void Props_Chair_Die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think     = Props_Chair_Animate;
	ent->nextthink = level.time + FRAMETIME;

	ent->health     = (int)ent->duration;
	ent->delay      = damage;
	ent->takedamage = qfalse;
	//ent->enemy = inflictor;

	Spawn_Shard(ent, inflictor, ent->wait, ent->count);

	Prop_Break_Sound(ent);

	trap_UnlinkEntity(ent);

	ent->clipmask   = 0;
	ent->r.contents = 0;
	ent->s.eType    = ET_GENERAL;

	trap_LinkEntity(ent);
}

/**
 * @brief Props_Chair_Skyboxtouch
 * @param[out] ent
 */
void Props_Chair_Skyboxtouch(gentity_t *ent)
{
	ent->think = G_FreeEntity;
}

/**
 * @brief SP_Props_Chair
 * @param[in,out] ent
 */
void SP_Props_Chair(gentity_t *ent)
{
	int mass;

	ent->s.modelindex = G_ModelIndex("models/furniture/chair/chair_office3.md3");

	ent->delay = 0; // inherits damage value

	if (G_SpawnInt("mass", "5", &mass))
	{
		ent->wait = mass;
	}
	else
	{
		ent->wait = 5;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;
	ent->s.dmgFlags = HINT_CHAIR;   // so client knows what kind of mover it is for cursorhints

	ent->isProp = qtrue;

	VectorSet(ent->r.mins, -12, -12, 0);
	VectorSet(ent->r.maxs, 12, 12, 48);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->duration = ent->health;

	if (!ent->count)
	{
		ent->count = FXTYPE_WOOD;
	}

	ent->think     = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch      = Props_Chair_Touch;
	ent->die        = Props_Chair_Die;
	ent->takedamage = qtrue;
	trap_LinkEntity(ent);
}

/**
 * @brief SP_Props_ChairHiback
 * @param[in,out] ent
 */
void SP_Props_ChairHiback(gentity_t *ent)
{
	int mass;

	ent->s.modelindex = G_ModelIndex("models/furniture/chair/hiback5.md3");

	ent->delay = 0; // inherits damage value

	if (G_SpawnInt("mass", "5", &mass))
	{
		ent->wait = mass;
	}
	else
	{
		ent->wait = 5;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;
	ent->s.dmgFlags = HINT_CHAIR;   // so client knows what kind of mover it is for cursorhints

	ent->isProp = qtrue;

	VectorSet(ent->r.mins, -12, -12, 0);
	VectorSet(ent->r.maxs, 12, 12, 48);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->duration = ent->health;

	if (!ent->count)
	{
		ent->count = FXTYPE_WOOD;
	}

	ent->think     = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch      = Props_Chair_Touch;
	ent->die        = Props_Chair_Die;
	ent->takedamage = qtrue;
	trap_LinkEntity(ent);
}

/**
 * @brief SP_Props_ChairSide
 * @param[in,out] ent
 */
void SP_Props_ChairSide(gentity_t *ent)
{
	int mass;

	ent->s.modelindex = G_ModelIndex("models/furniture/chair/sidechair3.md3");

	ent->delay = 0; // inherits damage value

	if (G_SpawnInt("mass", "5", &mass))
	{
		ent->wait = mass;
	}
	else
	{
		ent->wait = 5;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;
	ent->s.dmgFlags = HINT_CHAIR;   // so client knows what kind of mover it is for cursorhints

	ent->isProp = qtrue;

	VectorSet(ent->r.mins, -12, -12, 0);
	VectorSet(ent->r.maxs, 12, 12, 48);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->duration = ent->health;

	if (!ent->count)
	{
		ent->count = FXTYPE_WOOD;
	}

	ent->think     = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch      = Props_Chair_Touch;
	ent->die        = Props_Chair_Die;
	ent->takedamage = qtrue;
	trap_LinkEntity(ent);
}

/**
 * @brief SP_Props_ChateauChair
 * @param[in,out] ent
 *
 * @note Can be one of two types, but they have the same animations/etc, so re-use what you can
 */
void SP_Props_ChateauChair(gentity_t *ent)
{
	int mass;

	ent->delay = 0; // inherits damage value

	if (G_SpawnInt("mass", "5", &mass))
	{
		ent->wait = mass;
	}
	else
	{
		ent->wait = 5;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;
	ent->s.dmgFlags = HINT_CHAIR;   // so client knows what kind of mover it is for cursorhints

	ent->isProp = qtrue;

	VectorSet(ent->r.mins, -12, -12, 0);
	VectorSet(ent->r.maxs, 12, 12, 48);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->duration = ent->health;

	if (!ent->count)
	{
		ent->count = FXTYPE_WOOD;
	}

	ent->think     = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch      = Props_Chair_Touch;
	ent->die        = Props_Chair_Die;
	ent->takedamage = qtrue;
	trap_LinkEntity(ent);
}

/**
 * @brief SP_Props_ChairChat
 * @param[in,out] ent
 */
void SP_Props_ChairChat(gentity_t *ent)
{
	ent->s.modelindex = G_ModelIndex("models/furniture/chair/chair_chat.md3");
	SP_Props_ChateauChair(ent);
}

/**
 * @brief SP_Props_ChairChatArm
 * @param[in,out] ent
 */
void SP_Props_ChairChatArm(gentity_t *ent)
{
	ent->s.modelindex = G_ModelIndex("models/furniture/chair/chair_chatarm.md3");
	SP_Props_ChateauChair(ent);
}

/**
 * @brief Use_DamageInflictor
 * @param[in] ent
 * @param other - unused
 * @param activator - unused
 */
void Use_DamageInflictor(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	gentity_t *daent = NULL;

	while ((daent = G_FindByTargetname(daent, daent ? daent->target : "")) != NULL)
	{
		if (daent == ent)
		{
			G_Printf("Use_DamageInflictor damaging self.\n");
		}
		else
		{
			G_Damage(daent, ent, ent, NULL, NULL, daent->client ? GIB_DAMAGE(daent->health) : GIB_ENT, 0, MOD_CRUSH);
		}
	}

	G_FreeEntity(ent);
}

/**
 * @brief SP_Props_DamageInflictor
 *
 * @details QUAKED props_damageinflictor (.8 .6 .6) (-8 -8 -8) (8 8 8)
 * this entity when used will cause 9999 damage to all entities it is targeting
 * then it will be removed
 *
 * @param[in,out] ent
 */
void SP_Props_DamageInflictor(gentity_t *ent)
{
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;

	ent->use = Use_DamageInflictor;
	trap_LinkEntity(ent);
}

/**
 * @brief Use_Props_Shard_Generator
 *
 * @details QUAKED props_shard_generator (.8 .5 .1) (-4 -4 -4) (4 4 4)
 *
 * wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_WOOD = 0,
 *     FXTYPE_GLASS = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in] ent
 * @param other - unused
 * @param activator - unused
 */
void Use_Props_Shard_Generator(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	gentity_t *inflictor;

	inflictor = G_Find(NULL, FOFS(targetname), ent->target);

	if (inflictor)
	{
		Spawn_Shard(ent, inflictor, ent->wait, ent->count);
	}

	G_FreeEntity(ent);
}

/**
 * @brief SP_props_shard_generator
 * @param[in,out] ent
 *
 * @note Unused
 */
void SP_props_shard_generator(gentity_t *ent)
{
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;
	ent->use       = Use_Props_Shard_Generator;

	if (ent->count < FXTYPE_WOOD || ent->count >= FXTYPE_MAX)
	{
		ent->count = FXTYPE_WOOD;
	}

	if (ent->wait == 0.f)
	{
		ent->wait = 5;
	}

	trap_LinkEntity(ent);
}

/**
 * @brief SP_Props_Desklamp
 *
 * @details QUAKED props_desklamp (.8 .6 .2) (-16 -16 0) (16 16 32)
 * point entity
 * health = default = 10
 * wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_GLASS = 0,
 *     FXTYPE_WOOD = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in,out] ent
 */
void SP_Props_Desklamp(gentity_t *ent)
{
	int mass;

	ent->s.modelindex = G_ModelIndex("models/furniture/lights/desklamp.md3");

	ent->delay = 0; // inherits damage value

	if (G_SpawnInt("mass", "5", &mass))
	{
		ent->wait = mass;
	}
	else
	{
		ent->wait = 2;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;

	ent->isProp = qtrue;

	VectorSet(ent->r.mins, -6, -6, 0);
	VectorSet(ent->r.maxs, 6, 6, 14);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->duration = ent->health;

	if (!ent->count)
	{
		ent->count = FXTYPE_METAL;
	}

	ent->think     = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch      = Props_Chair_Touch;
	ent->die        = Props_Chair_Die;
	ent->takedamage = qtrue;
	trap_LinkEntity(ent);
}
/**
 * @brief Props_Barrel_Touch
 *
 * @details QUAKED props_flamebarrel (.8 .6 .2) (-13 -13 0) (13 13 40) SMOKING NOLID OIL -
 * angle will determine which way the lid will fly off when it explodes
 *
 * when selecting the OIL spawnflag you have the option of giving it a target
 * this will ensure that the oil sprite will show up where you want it
 * ( be sure to put it on the floor )
 * the default is in the middle of the barrel on the floor
 *
 * @param self  - unused
 * @param other - unused
 * @param trace - unused
 *
 * @note Barrels cant move
 */
void Props_Barrel_Touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	return; // barrels cant move
}

/**
 * @brief Props_Barrel_Animate
 * @param[in,out] ent
 */
void Props_Barrel_Animate(gentity_t *ent)
{
	vec3_t v;

	if (ent->s.frame == 14)
	{

		// FIXME : Duplicate branch if/else
		if (ent->spawnflags & 1)
		{
			//  G_UseTargets (ent, NULL);
			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + 25000;
			return;
		}
		else
		{
			//G_UseTargets (ent, NULL);
			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + 25000;
			//ent->s.time = level.time;
			//ent->s.time2 = level.time + 2000;
			return;
		}
	}
	else
	{
		ent->nextthink = level.time + (FRAMETIME / 2);
	}

	ent->s.frame++;

	if (!(ent->spawnflags & 1))
	{
		//float ratio = 2.5;
		VectorSubtract(ent->r.currentOrigin, ent->enemy->r.currentOrigin, v);
		moveit(ent, vectoyaw(v), (ent->delay * 2.5f * FRAMETIME) * .001f);
	}
}

/**
 * @brief barrel_smoke
 * @param[in] ent
 */
void barrel_smoke(gentity_t *ent)
{
	gentity_t *tent;
	vec3_t    point;

	VectorCopy(ent->r.currentOrigin, point);

	tent = G_TempEntity(point, EV_SMOKE);
	VectorCopy(point, tent->s.origin);
	tent->s.time       = 4000;
	tent->s.time2      = 1000;
	tent->s.density    = 0;
	tent->s.angles2[0] = 8;
	tent->s.angles2[1] = 64;
	tent->s.angles2[2] = 50;
}

/**
 * @brief smoker_think
 * @param[in,out] ent
 *
 * @note Unused
 */
void smoker_think(gentity_t *ent)
{
	ent->count--;

	if (!ent->count)
	{
		G_FreeEntity(ent);
	}
	else
	{
		barrel_smoke(ent);
		ent->nextthink = level.time + FRAMETIME;
	}
}

/**
 * @brief SP_OilSlick
 * @param[in] ent
 */
void SP_OilSlick(gentity_t *ent)
{
	gentity_t *tent;

	tent = G_TempEntity(ent->r.currentOrigin, EV_OILSLICK);
	VectorCopy(ent->r.currentOrigin, tent->s.origin);
	tent->s.angles2[0] = 16;
	tent->s.angles2[1] = 48;
	tent->s.angles2[2] = 10000;
	tent->s.density    = ent->s.number;
}

/**
 * @brief OilParticles_think
 * @param[in,out] ent
 *
 * @note Unused ? See SP_OilParticles
 */
void OilParticles_think(gentity_t *ent)
{
	gentity_t *tent;
	gentity_t *owner = &g_entities[ent->s.density];

	if (owner && owner->takedamage && ent->count2 > level.time - 5000)
	{
		ent->nextthink = (level.time + FRAMETIME / 2);

		tent = G_TempEntity(ent->r.currentOrigin, EV_OILPARTICLES);
		VectorCopy(ent->r.currentOrigin, tent->s.origin);
		tent->s.time    = ent->count2;
		tent->s.density = ent->s.density;
		VectorCopy(ent->rotate, tent->s.origin2);
	}
	else
	{
		G_FreeEntity(ent);
	}
}

/*
 * @brief Delayed_Leak_Think
 * @param[in] ent
 *
 * @note Unused
void Delayed_Leak_Think(gentity_t *ent)
{
    vec3_t    point;
    gentity_t *tent;

    VectorCopy(ent->r.currentOrigin, point);

    tent = G_TempEntity(point, EV_OILSLICK);
    VectorCopy(point, tent->s.origin);

    tent->s.angles2[0] = 0;
    tent->s.angles2[1] = 0;
    tent->s.angles2[2] = 2000;
    tent->s.density    = ent->count;
}
*/

/**
 * @brief validOilSlickSpawnPoint
 * @param[in] point
 * @param[in] ent
 * @return
 *
 * @note Unused
 */
qboolean validOilSlickSpawnPoint(vec3_t point, gentity_t *ent)
{
	trace_t   tr;
	vec3_t    end;
	gentity_t *traceEnt;

	VectorCopy(point, end);
	end[2] -= 9999;

	trap_Trace(&tr, point, NULL, NULL, end, ent->s.number, MASK_SHOT);

	traceEnt = &g_entities[tr.entityNum];

	if (traceEnt && traceEnt->classname)
	{
		if (!Q_stricmp(traceEnt->classname, "worldspawn"))
		{
			if (tr.plane.normal[0] == 0.f && tr.plane.normal[1] == 0.f && tr.plane.normal[2] == 1.f)
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/**
 * @brief SP_OilParticles
 * @param ent
 *
 * @todo FIXME: move this to client !
 */
void SP_OilParticles(gentity_t *ent)
{

	// Note to self quick fix
	// need to move this to client
	return;

	/* FIXME
	gentity_t *OilLeak;
	vec3_t    point;
	vec3_t    vec;
	vec3_t    forward;

	OilLeak = G_Spawn();

	VectorCopy(ent->r.currentOrigin, point);

	point[2] = ent->pos3[2];

	VectorSubtract(ent->pos3, point, vec);
	vectoangles(vec, vec);
	AngleVectors(vec, forward, NULL, NULL);
	VectorMA(point, 12, forward, point);

	G_SetOrigin(OilLeak, point);

	G_SetAngle(OilLeak, ent->r.currentAngles);

	VectorCopy(forward, OilLeak->rotate);

	OilLeak->think     = OilParticles_think;
	OilLeak->nextthink = level.time + FRAMETIME;

	OilLeak->s.density = ent->s.number;
	OilLeak->count2    = level.time;

	trap_LinkEntity(OilLeak);
	*/
}

/**
 * @brief Props_Barrel_Pain
 * @param[in,out] ent
 * @param attacker - unused
 * @param damage   - unused
 * @param point    - unused
 */
void Props_Barrel_Pain(gentity_t *ent, gentity_t *attacker, int damage, vec3_t point)
{
	if (ent->health <= 0)
	{
		return;
	}

	if (!(ent->spawnflags & 8))
	{
		SP_OilSlick(ent);
		ent->spawnflags |= 8;
	}

	ent->count2++;

	if (ent->count2 < 6)
	{
		SP_OilParticles(ent);
	}
}

/**
 * @brief OilSlick_remove_think
 * @param[in] ent
 */
void OilSlick_remove_think(gentity_t *ent)
{
	gentity_t *tent;

	tent = G_TempEntity(ent->r.currentOrigin, EV_OILSLICKREMOVE);

	tent->s.density = ent->s.density;
}

/**
 * @brief OilSlick_remove
 * @param[in] ent
 */
void OilSlick_remove(gentity_t *ent)
{
	gentity_t *remove;

	remove = G_Spawn();

	remove->s.density = ent->s.number;
	remove->think     = OilSlick_remove_think;
	remove->nextthink = level.time + 1000;
	VectorCopy(ent->r.currentOrigin, remove->r.currentOrigin);
	trap_LinkEntity(remove);
}

/**
 * @brief Props_Barrel_Die
 * @param[in,out] ent
 * @param[in] inflictor
 * @param attacker
 * @param[in] damage
 * @param mod
 */
void Props_Barrel_Die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	vec3_t dir;

	if (ent->spawnflags & 1)
	{
		ent->s.eFlags = EF_SMOKINGBLACK;
	}

	G_UseTargets(ent, NULL);

	if (ent->spawnflags & 4)
	{
		OilSlick_remove(ent);
	}

	ent->health = 100;
	propExplosion(ent);
	ent->health = 0;

	ent->takedamage = qfalse;

	AngleVectors(ent->r.currentAngles, dir, NULL, NULL);
	dir[2] = 1;

	if (!(ent->spawnflags & 2))
	{
		fire_flamebarrel(ent, ent->r.currentOrigin, dir);
	}

	ent->touch = NULL;

	ent->think     = Props_Barrel_Animate;
	ent->nextthink = level.time + FRAMETIME;

	ent->health = (int)ent->duration;
	ent->delay  = damage;
	ent->enemy  = inflictor;

	if (inflictor)
	{
		Spawn_Shard(ent, inflictor, ent->wait, ent->count);
	}

	Prop_Break_Sound(ent);

	trap_UnlinkEntity(ent);

	ent->clipmask   = 0;
	ent->r.contents = 0;
	ent->s.eType    = ET_GENERAL;

	trap_LinkEntity(ent);
}

/**
 * @brief Props_Barrel_Think
 * @param[in,out] self
 */
void Props_Barrel_Think(gentity_t *self)
{
	self->active = qfalse;
	Props_Chair_Think(self);
}

/**
 * @brief SP_Props_Flamebarrel
 * @param[in,out] ent
 */
void SP_Props_Flamebarrel(gentity_t *ent)
{
	int mass;

	if (ent->spawnflags & 4)
	{
		ent->s.modelindex = G_ModelIndex("models/furniture/barrel/barrel_c.md3");
	}
	else if (ent->spawnflags & 1)
	{
		ent->s.modelindex = G_ModelIndex("models/furniture/barrel/barrel_d.md3");
	}
	else
	{
		ent->s.modelindex = G_ModelIndex("models/furniture/barrel/barrel_b.md3");
	}

	ent->delay = 0; // inherits damage value

	if (G_SpawnInt("mass", "5", &mass))
	{
		ent->wait = mass;
	}
	else
	{
		ent->wait = 10;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = 0;
	ent->s.eType    = ET_MOVER;

	ent->isProp = qtrue;

	VectorSet(ent->r.mins, -13, -13, 0);
	VectorSet(ent->r.maxs, 13, 13, 36);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->health)
	{
		ent->health = 20;
	}

	ent->duration = ent->health;

	ent->count = FXTYPE_METAL; // metal shards

	ent->think     = Props_Barrel_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch = Props_Barrel_Touch;

	ent->die = Props_Barrel_Die;

	if (ent->spawnflags & 4)
	{
		ent->pain = Props_Barrel_Pain;
	}

	ent->takedamage = qtrue;
	trap_LinkEntity(ent);
}

/**
 * @brief touch_crate_64
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void touch_crate_64(gentity_t *self, gentity_t *other, trace_t *trace)
{
	//float ratio = 1.5;
	vec3_t v;

	if (other->r.currentOrigin[2] > (self->r.currentOrigin[2] + 10 + 31))
	{
		return;
	}

	VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, v);
	moveit(self, vectoyaw(v), (20 * 1.5f * FRAMETIME) * .001f);
}

/**
 * @brief crate_animate
 * @param[in,out] ent
 */
void crate_animate(gentity_t *ent)
{
	if (ent->s.frame == 17)
	{
		G_UseTargets(ent, NULL);
		ent->think     = G_FreeEntity;
		ent->nextthink = level.time + 2000;
		ent->s.time    = level.time;
		ent->s.time2   = level.time + 2000;
		return;
	}

	ent->s.frame++;
	ent->nextthink = level.time + (FRAMETIME / 2);
}

/**
 * @brief crate_die
 * @param[in,out] ent
 * @param[in] inflictor
 * @param attacker - unused
 * @param damage   - unused
 * @param mod      - unused
 */
void crate_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	Spawn_Shard(ent, inflictor, ent->wait, ent->count);

	ent->takedamage = qfalse;
	ent->think      = crate_animate;
	ent->nextthink  = level.time + FRAMETIME;
	ent->touch      = NULL;

	trap_UnlinkEntity(ent);

	ent->clipmask   = 0;
	ent->r.contents = 0;
	ent->s.eType    = ET_GENERAL;

	trap_LinkEntity(ent);
}

/**
 * @brief SP_crate_64
 *
 * @details QUAKED props_crate_64 (.8 .6 .2) (-32 -32 0) (32 32 64)
 * breakable pushable
 *
 *   health = default = 20
 * wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_GLASS = 0,
 *     FXTYPE_WOOD = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in,out] self
 */
void SP_crate_64(gentity_t *self)
{
	self->s.modelindex = G_ModelIndex("models/furniture/crate/crate64.md3");

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;
	self->r.svFlags  = 0;

	VectorSet(self->r.mins, -32, -32, 0);
	VectorSet(self->r.maxs, 32, 32, 64);

	self->s.eType = ET_MOVER;

	self->isProp = qtrue;
	G_SetOrigin(self, self->s.origin);
	G_SetAngle(self, self->s.angles);

	self->touch = touch_crate_64;
	self->die   = crate_die;

	self->takedamage = qtrue;

	if (!self->health)
	{
		self->health = 20;
	}

	if (!self->count)
	{
		self->count = 1;
	}

	if (self->wait == 0.f)
	{
		self->wait = 10;
	}

	self->isProp = qtrue;

	trap_LinkEntity(self);

	self->think     = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

/**
 * @brief SP_crate_32
 *
 * @details QUAKED props_crate_32 (.8 .6 .2) (-16 -16 0) (16 16 32)
 * breakable pushable
 *
 *   health = default = 20
 * wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_GLASS = 0,
 *     FXTYPE_WOOD = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in,out] self
 */
void SP_crate_32(gentity_t *self)
{
	self->s.modelindex = G_ModelIndex("models/furniture/crate/crate32.md3");

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;
	self->r.svFlags  = 0;

	VectorSet(self->r.mins, -16, -16, 0);
	VectorSet(self->r.maxs, 16, 16, 32);

	self->s.eType = ET_MOVER;

	self->isProp = qtrue;
	G_SetOrigin(self, self->s.origin);
	G_SetAngle(self, self->s.angles);

	self->touch = touch_crate_64;
	self->die   = crate_die;

	self->takedamage = qtrue;

	if (!self->health)
	{
		self->health = 20;
	}

	if (!self->count)
	{
		self->count = 1;
	}

	if (self->wait == 0.f)
	{
		self->wait = 10;
	}

	self->isProp = qtrue;

	trap_LinkEntity(self);

	self->think     = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

//////////////////////////////////////////////

/**
 * @brief props_crate32x64_think
 *
 * @details QUAKED props_crate_32x64 (.8 .6 .2) ?
 * requires an origin brush
 *
 * breakable NOT pushable
 *
 * brushmodel only
 *
 *   health = default = 20
 * wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_GLASS = 0,
 *     FXTYPE_WOOD = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in,out] ent
 */
void props_crate32x64_think(gentity_t *ent)
{
	ent->s.frame++;

	if (ent->s.frame < 17)
	{
		ent->nextthink = level.time + (FRAMETIME / 2);
	}
	else
	{
		ent->clipmask   = 0;
		ent->r.contents = 0;
		ent->takedamage = qfalse;

		G_UseTargets(ent, NULL);
	}
}

/**
 * @brief props_crate32x64_die
 * @param[out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void props_crate32x64_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think     = props_crate32x64_think;
	ent->nextthink = level.time + FRAMETIME;
}

/**
 * @brief SP_Props_Crate32x64
 * @param[in,out] ent
 */
void SP_Props_Crate32x64(gentity_t *ent)
{
	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_crate32x64_die;

	trap_LinkEntity(ent);
}

/**
 * @brief flippy_table_use
 *
 * @details QUAKED props_flippy_table (.8 .6 .2) ? - - X_AXIS Y_AXIS LEADER
 * this entity will need a leader and an origin brush
 * !!!!!!!!!!!!!!
 * just a reminder to put the origin brush in the proper location for the leader and the
 * slave so that the table will flip over correctly.
 *  *
 * @param[in] ent
 * @param[in] other
 * @param activator - unused
 */
void flippy_table_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	// it would be odd to flip a table if your standing on it
	if (!other || other->s.groundEntityNum == ent->s.number)
	{
		// G_Printf ("can't push table over while standing on it\n");
		return;
	}

	ent->use = NULL;

	if (infront(ent, other))
	{
		gentity_t *slave;

		// need to swap the team leader with the slave
		for (slave = ent ; slave ; slave = slave->teamchain)
		{
			if (slave == ent)
			{
				continue;
			}

			slave->s.pos.trType     = ent->s.pos.trType;
			slave->s.pos.trTime     = ent->s.pos.trTime;
			slave->s.pos.trDuration = ent->s.pos.trDuration;
			VectorCopy(ent->s.pos.trBase, slave->s.pos.trBase);
			VectorCopy(ent->s.pos.trDelta, slave->s.pos.trDelta);

			slave->s.apos.trType     = ent->s.apos.trType;
			slave->s.apos.trTime     = ent->s.apos.trTime;
			slave->s.apos.trDuration = ent->s.apos.trDuration;
			VectorCopy(ent->s.apos.trBase, slave->s.apos.trBase);
			VectorCopy(ent->s.apos.trDelta, slave->s.apos.trDelta);

			slave->think     = ent->think;
			slave->nextthink = ent->nextthink;

			VectorCopy(ent->pos1, slave->pos1);
			VectorCopy(ent->pos2, slave->pos2);

			slave->speed = ent->speed;

			slave->flags &= ~FL_TEAMSLAVE;
			// make it visible
			trap_LinkEntity(slave);

			Use_BinaryMover(slave, other, other);
		}

		trap_UnlinkEntity(ent);
	}
	else
	{
		Use_BinaryMover(ent, other, other);
	}
}

/**
 * @brief flippy_table_animate
 * @param ent
 *
 * @todo FIXME: remove ?
 */
void flippy_table_animate(gentity_t *ent)
{
	return;

	/* FIXME: remove ?
	if (ent->s.frame == 9)
	{
	    G_UseTargets(ent, NULL);
	    ent->think     = G_FreeEntity;
	    ent->nextthink = level.time + 2000;
	}
	else
	{
	    ent->s.frame++;
	    ent->nextthink = level.time + (FRAMETIME / 2);
	}
	*/
}

/**
 * @brief props_flippy_table_die
 * @param[out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 *
 * @note Unused (see SP_Props_Flipping_Table)
 */
void props_flippy_table_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think      = flippy_table_animate;
	ent->nextthink  = level.time + FRAMETIME;
	ent->takedamage = qfalse;

	G_UseTargets(ent, NULL);
}

/**
 * @brief props_flippy_blocked
 * @param[in] ent
 * @param[in,out] other
 */
void props_flippy_blocked(gentity_t *ent, gentity_t *other)
{
	vec3_t velocity;
	vec3_t angles;
	vec3_t kvel;
	// just for now
	float angle = ent->r.currentAngles[YAW];

	if (other->client)
	{
		// shoot the player off of it
		VectorCopy(ent->s.apos.trBase, angles);
		angles[YAW]  += angle;
		angles[PITCH] = 0;  // always forward

		AngleVectors(angles, velocity, NULL, NULL);
		VectorScale(velocity, 24, velocity);
		velocity[2] += 100 + crandom() * 50;

		VectorScale(velocity, 32, kvel);
		VectorAdd(other->client->ps.velocity, kvel, other->client->ps.velocity);
	}
	else if (other->s.eType == ET_ITEM)
	{
		VectorCopy(ent->s.apos.trBase, angles);
		angles[YAW]  += angle;
		angles[PITCH] = 0;  // always forward

		AngleVectors(angles, velocity, NULL, NULL);
		VectorScale(velocity, 150, velocity);
		velocity[2] += 300 + crandom() * 50;

		VectorScale(velocity, 8, kvel);
		other->s.pos.trType = TR_GRAVITY;
		other->s.pos.trTime = level.time;
		VectorCopy(kvel, other->s.pos.trDelta);

		other->s.eFlags |= EF_BOUNCE;
	}
	else
	{
		// just delete it or destroy it
		G_FreeEntity(other);
		return;
	}
}

/**
 * @brief SP_Props_Flipping_Table
 * @param[in,out] ent
 */
void SP_Props_Flipping_Table(gentity_t *ent)
{
	if (!ent->model)
	{
		G_Printf(S_COLOR_RED "props_Flipping_Table with NULL model\n");
		return;
	}

	trap_SetBrushModel(ent, ent->model);

	ent->speed = 500;
	ent->angle = 90;

	// ent->spawnflags |= 8;
	if (!(ent->spawnflags & 4) && !(ent->spawnflags & 8))
	{
		G_Printf("you forgot to select the X or Y Axis\n");
	}

	VectorClear(ent->rotate);

	if      (ent->spawnflags & 4)
	{
		ent->rotate[2] = 1;
	}
	else if (ent->spawnflags & 8)
	{
		ent->rotate[0] = 1;
	}
	else
	{
		ent->rotate[1] = 1;
	}

	ent->spawnflags |= 64; // stay open

	InitMoverRotate(ent);

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);
	VectorCopy(ent->s.apos.trBase, ent->r.currentAngles);

	ent->blocked = props_flippy_blocked;

	if (!ent->health)
	{
		ent->health = 100;
	}

	ent->wait *= 1000;

	//ent->takedamage = qtrue;

	//ent->die = props_flippy_table_die;

	ent->use = flippy_table_use;

	trap_LinkEntity(ent);
}

/**
 * @brief props_58x112tablew_think
 *
 * @details QUAKED props_58x112tablew (.8 .6 .2) ?
 * dimensions are 58 x 112 x 32 (x,y,z)
 *
 * requires an origin brush
 *
 * breakable NOT pushable
 *
 * brushmodel only
 *
 *   health = default = 10
 * wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_GLASS = 0,
 *     FXTYPE_WOOD = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in,out] ent
 */
void props_58x112tablew_think(gentity_t *ent)
{
	ent->s.frame++;

	if (ent->s.frame < 16)
	{
		ent->nextthink = level.time + (FRAMETIME / 2);
	}
	else
	{
		ent->clipmask   = 0;
		ent->r.contents = 0;

		G_UseTargets(ent, NULL);
	}
}

/**
 * @brief props_58x112tablew_die
 * @param[out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void props_58x112tablew_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think      = props_58x112tablew_think;
	ent->nextthink  = level.time + FRAMETIME;
	ent->takedamage = qfalse;
}

/**
 * @brief SP_Props_58x112tablew
 * @param[in,out] ent
 */
void SP_Props_58x112tablew(gentity_t *ent)
{
	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 10;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_58x112tablew_die;

	trap_LinkEntity(ent);
}

/**
 * @brief props_castlebed_touch
 *
 * @details QUAKED props_castlebed (.8 .6 .2) ?
 * dimensions are 112 x 128 x 80 (x,y,z)
 *
 * requires an origin brush
 *
 * breakable NOT pushable
 *
 * brushmodel only
 *
 *   health = default = 20
 * wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )
 *
 * shard =
 *     FXTYPE_GLASS = 0,
 *     FXTYPE_WOOD = 1,
 *     FXTYPE_METAL = 2
 *
 * @param[in] ent
 * @param[in,out] other
 * @param trace - unused
 *
 * @note Unused
 */
void props_castlebed_touch(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	if (!other->client)
	{
		return;
	}

	if ((other->client->ps.pm_flags & PMF_JUMP_HELD)
	    && other->s.groundEntityNum == ent->s.number
	    && !other->client->ps.pm_time)
	{
		G_Damage(ent, other, other, NULL, NULL, 1, 0, MOD_CRUSH);

		// TDB: need sound of bed springs for this
		G_Printf("SOUND sqweeky\n");

		other->client->ps.velocity[2] += 250;

		other->client->ps.pm_time   = 250;
		other->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

/**
 * @brief props_castlebed_animate
 * @param[in,out] ent
 *
 * @note Unused
 */
void props_castlebed_animate(gentity_t *ent)
{
	ent->s.frame++;

	if (ent->s.frame < 8)
	{
		ent->nextthink = level.time + (FRAMETIME / 2);
	}
	else
	{
		ent->clipmask   = 0;
		ent->r.contents = 0;
		G_UseTargets(ent, NULL);
	}
}

/**
 * @brief props_castlebed_die
 * @param[out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 *
 *
 * @note Unused
 */
void props_castlebed_die(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->think      = props_castlebed_animate;
	ent->nextthink  = level.time + FRAMETIME;
	ent->touch      = NULL;
	ent->takedamage = qfalse;

	ent->count = FXTYPE_WOOD;
	Prop_Break_Sound(ent);
}

/**
 * @brief SP_props_castlebed
 * @param[in,out] ent
 *
 * @note Unused
 */
void SP_props_castlebed(gentity_t *ent)
{
	trap_SetBrushModel(ent, ent->model);

	InitProp(ent);

	if (!ent->health)
	{
		ent->health = 20;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die   = props_castlebed_die;
	ent->touch = props_castlebed_touch;

	trap_LinkEntity(ent);
}

/**
 * @brief props_snowGenerator_think
 *
 * @details QUAKED props_snowGenerator (3 2 7) ? TOGGLE_ON ALWAYS_ON
 * entity brush need to be targeted to an info notnull this
 * will determine the direction the snow particles will travel.
 *
 * speed
 * gravity
 * turb
 *
 * count is the number of snowflurries 3 to 5 would be a good number
 *
 * duration is how long the effect will last 1 is 1 second
 *
 * @param[in,out] ent
 */
void props_snowGenerator_think(gentity_t *ent)
{
	gentity_t *tent;
	float     high, wide, deep;
	int       i;
	vec3_t    point;

	if (!(ent->spawnflags & 1))
	{
		return;
	}

	high = ent->r.maxs[2] - ent->r.mins[2];
	wide = ent->r.maxs[1] - ent->r.mins[1];
	deep = ent->r.maxs[0] - ent->r.mins[0];

	for (i = 0; i < ent->count; i++)
	{
		VectorCopy(ent->pos1, point);

		// we need to randomize to the extent of the brush
		point[0] += crandom() * (deep * 0.5f);
		point[1] += crandom() * (wide * 0.5f);
		point[2] += crandom() * (high * 0.5f);

		tent = G_TempEntity(point, EV_SNOWFLURRY);
		VectorCopy(point, tent->s.origin);
		VectorCopy(ent->movedir, tent->s.angles);
		tent->s.time  = 2000; // life time
		tent->s.time2 = 1000; // alpha fade start
	}

	if (ent->spawnflags & 2)
	{
		ent->nextthink = level.time + FRAMETIME;
	}
	else if (ent->wait < level.time)
	{
		ent->nextthink = level.time + FRAMETIME;
	}
}

/**
 * @brief props_snowGenerator_use
 * @param[in,out] ent
 * @param other     - unused
 * @param activator - unused
 */
void props_snowGenerator_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (!(ent->spawnflags & 1))
	{
		ent->spawnflags |= 1;
		ent->think       = props_snowGenerator_think;
		ent->nextthink   = level.time + FRAMETIME;
		ent->wait        = level.time + ent->duration;
	}
	else
	{
		ent->spawnflags &= ~1;
	}
}

/**
 * @brief SP_props_snowGenerator
 * @param[in,out] ent
 */
void SP_props_snowGenerator(gentity_t *ent)
{
	vec3_t center;

	trap_SetBrushModel(ent, ent->model);

	VectorAdd(ent->r.absmin, ent->r.absmax, center);
	VectorScale(center, 0.5f, center);

	VectorCopy(center, ent->pos1);

	if (!ent->target)
	{
		G_Printf("snowGenerator at loc %s does not have a target\n", vtos(center));
		return;
	}
	else
	{
		gentity_t *target;

		target = G_Find(NULL, FOFS(targetname), ent->target);
		if (!target)
		{
			G_Printf("error snowGenerator at loc %s does cant find target %s\n", vtos(center), ent->target);
			return;
		}

		VectorSubtract(target->s.origin, ent->s.origin, ent->movedir);
		VectorNormalize(ent->movedir);
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->r.svFlags  = SVF_NOCLIENT;

	if ((ent->spawnflags & 1) || (ent->spawnflags & 2))
	{
		ent->think     = props_snowGenerator_think;
		ent->nextthink = level.time + FRAMETIME;

		if (ent->spawnflags & 2)
		{
			ent->spawnflags |= 1;
		}
	}

	ent->use = props_snowGenerator_use;

	if (ent->delay == 0.f)
	{
		ent->delay = 100;
	}
	else
	{
		ent->delay *= 100;
	}

	if (!ent->count)
	{
		ent->count = 32;
	}

	if (ent->duration == 0.f)
	{
		ent->duration = 1;
	}

	ent->duration *= 1000;

	trap_LinkEntity(ent);
}

/////////////////////////////
// FIRES AND EXPLOSION PROPS
/////////////////////////////

/**
 * @brief props_decoration_animate
 *
 * @details QUAKED props_decoration (.6 .7 .7) (-8 -8 0) (8 8 16) STARTINVIS DEBRIS ANIMATE KEEPBLOCK TOUCHACTIVATE LOOPING STARTON
 * "model2" will specify the model to load
 *
 * "noise"  the looping sound entity is to make
 *
 * "type" type of debris ("glass", "wood", "metal", "rubble") default is "wood"
 * "count" how much debris ei. default 4 pieces
 *
 * you will need to specify the bounding box for the entity
 * "high"  default is 4
 * "wide"  default is 4
 *
 * "frames"    how many frames of animation to play
 * "loop" when the animation is done start again on this frame
 * "startonframe" on what frame do you want to start the animation
 *
 * @param[in,out] ent
 */
void props_decoration_animate(gentity_t *ent)
{
	ent->s.frame++;
	ent->s.eType = ET_GENERAL;

	if (ent->s.frame > ent->count2)
	{
		if ((ent->spawnflags & 32) || (ent->spawnflags & 64))
		{
			ent->s.frame = ent->props_frame_state;

			if (!(ent->spawnflags & 64))
			{
				ent->takedamage = qfalse;
			}
		}
		else
		{
			ent->s.frame    = ent->count2;
			ent->takedamage = qfalse;

			return;
		}
	}

	ent->nextthink = level.time + 50;
}

/**
 * @brief props_decoration_death
 * @param[in,out] ent
 * @param[in] inflictor
 * @param attacker - unused
 * @param damage   - unused
 * @param mod      - unused
 */
void props_decoration_death(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	if (!(ent->spawnflags & 8))
	{
		ent->clipmask   = 0;
		ent->r.contents = 0;
		ent->s.eType    = ET_GENERAL;
		trap_LinkEntity(ent);
	}

	ent->takedamage = qfalse;

	G_UseTargets(ent, NULL);

	if (ent->spawnflags & 2)
	{
		Spawn_Shard(ent, inflictor, ent->count, ent->key);
	}

	if (ent->spawnflags & 4)
	{
		ent->nextthink = level.time + 50;
		ent->think     = props_decoration_animate;
		return;
	}

	G_FreeEntity(ent);
}

/**
 * @brief Use_props_decoration
 * @param[in,out] ent
 * @param self      - unused
 * @param activator - unused
 */
void Use_props_decoration(gentity_t *ent, gentity_t *self, gentity_t *activator)
{
	if (ent->spawnflags & 1)
	{
		trap_LinkEntity(ent);
		ent->spawnflags &= ~1;
	}
	else if (ent->spawnflags & 4)
	{
		ent->nextthink = level.time + 50;
		ent->think     = props_decoration_animate;
	}
	else
	{
		trap_UnlinkEntity(ent);
		ent->spawnflags |= 1;
	}
}

/**
 * @brief props_touch
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void props_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (self->spawnflags & 16)
	{
		props_decoration_death(self, other, other, 9999, MOD_CRUSH);
	}
}

/**
 * @brief SP_props_decoration
 * @param[in,out] ent
 */
void SP_props_decoration(gentity_t *ent)
{
	float    light;
	vec3_t   color;
	qboolean lightSet, colorSet;
	char     *sound;
	char     *frames;
	float    num_frames;

	char *loop;
	char *startonframe;

	if (G_SpawnString("startonframe", "0", &startonframe))
	{
		ent->s.frame = Q_atoi(startonframe);
	}

	if (ent->model2)
	{
		ent->s.modelindex = G_ModelIndex(ent->model2);
	}

	if (G_SpawnString("noise", "100", &sound))
	{
		ent->s.loopSound = G_SoundIndex(sound);
	}

	if ((ent->spawnflags & 32) && G_SpawnString("loop", "100", &loop))
	{
		ent->props_frame_state = Q_atoi(loop);
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat("light", "100", &light);
	colorSet = G_SpawnVector("color", "1 1 1", color);
	if (lightSet || colorSet)
	{
		int r, g, b, i;

		r = (int)(color[0] * 255);
		if (r > 255)
		{
			r = 255;
		}
		g = (int)(color[1] * 255);
		if (g > 255)
		{
			g = 255;
		}
		b = (int)(color[2] * 255);
		if (b > 255)
		{
			b = 255;
		}
		i = (int)(light / 4);
		if (i > 255)
		{
			i = 255;
		}
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}

	if (ent->health)
	{
		float height;
		float width;
		char  *type;
		char  *high;
		char  *wide;

		ent->isProp     = qtrue;
		ent->takedamage = qtrue;
		ent->die        = props_decoration_death;

		G_SpawnString("type", "wood", &type);
		if (!Q_stricmp(type, "wood"))
		{
			ent->key = 1;
		}
		else if (!Q_stricmp(type, "glass"))
		{
			ent->key = 0;
		}
		else if (!Q_stricmp(type, "metal"))
		{
			ent->key = 2;
		}
		else if (!Q_stricmp(type, "rubble"))
		{
			ent->key = 3;
		}

		G_SpawnString("high", "0", &high);
		height = (float)atof(high);

		if (height == 0.f)
		{
			height = 4;
		}

		G_SpawnString("wide", "0", &wide);
		width = (float)atof(wide);

		if (width == 0.f)
		{
			width = 4;
		}

		width /= 2;

		if (Q_stricmp(ent->classname, "props_decorBRUSH"))
		{
			VectorSet(ent->r.mins, -width, -width, 0);
			VectorSet(ent->r.maxs, width, width, height);
		}

		ent->clipmask   = CONTENTS_SOLID;
		ent->r.contents = CONTENTS_SOLID;
		ent->s.eType    = ET_MOVER;

		G_SpawnString("frames", "0", &frames);
		num_frames = (float)atof(frames);

		ent->count2 = (int)num_frames;

		if (ent->targetname)
		{
			ent->use = Use_props_decoration;
		}

		ent->touch = props_touch;

	}
	else if (!(ent->health) && (ent->spawnflags & 4))
	{
		G_SpawnString("frames", "0", &frames);
		num_frames = (float)atof(frames);

		ent->count2 = (int)num_frames;
		ent->use    = Use_props_decoration;
	}

	if (ent->spawnflags & 64)
	{
		ent->nextthink = level.time + 50;
		ent->think     = props_decoration_animate;
	}

	ent->r.svFlags = 0;

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!(ent->spawnflags & 1))
	{
		trap_LinkEntity(ent);
	}
	else
	{
		ent->use = Use_props_decoration;
	}
}

/**
 * @brief SP_props_decorBRUSH
 *
 * @details QUAKED props_decorBRUSH (.6 .7 .7) ? STARTINVIS DEBRIS ANIMATE KEEPBLOCK - LOOPING STARTON
 * ANIMATE animate on death
 * STARTON playanimation on death
 * must have an origin brush
 *
 * "model2" will specify the model to load
 *
 * "noise"  the looping sound entity is to make
 *
 * "type" type of debris ("glass", "wood", "metal", "rubble") default is "wood"
 * "count" how much debris ei. default 4 pieces
 *
 * "frames"    how many frames of animation to play
 * "loop" when the animation is done start again on this frame
 * "startonframe" on what frame do you want to start the animation
 *
 * @param[in,out] self
 */
void SP_props_decorBRUSH(gentity_t *self)
{
	trap_SetBrushModel(self, self->model);

	SP_props_decoration(self);

	if (self->model2)
	{
		self->s.modelindex2 = G_ModelIndex(self->model2);
	}
}

/**
 * @brief SP_props_decor_Scale
 *
 * @details QUAKED props_decoration_scale (.6 .7 .7) (-8 -8 0) (8 8 16) STARTINVIS DEBRIS ANIMATE KEEPBLOCK TOUCHACTIVATE LOOPING STARTON
 *
 * "modelscale" - Scale multiplier (defaults to 1.0 and scales uniformly)
 * "modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both the "modelscale" is ignored
 *
 * "model2" will specify the model to load
 *
 * "noise"  the looping sound entity is to make
 *
 * "type" type of debris ("glass", "wood", "metal", "rubble") default is "wood"
 * "count" how much debris ei. default 4 pieces
 *
 * you will need to specify the bounding box for the entity
 * "high"  default is 4
 * "wide"  default is 4
 *
 * "frames"    how many frames of animation to play
 * "loop" when the animation is done start again on this frame
 * "startonframe" on what frame do you want to start the animation
 *
 * @param[in,out] ent
 */
void SP_props_decor_Scale(gentity_t *ent)
{
	float  scale[3] = { 1, 1, 1 };
	vec3_t scalevec;

	SP_props_decoration(ent);

	ent->s.eType = ET_GAMEMODEL;

	// look for general scaling
	if (G_SpawnFloat("modelscale", "1", &scale[0]))
	{
		scale[2] = scale[1] = scale[0];
	}

	// look for axis specific scaling
	if (G_SpawnVector("modelscale_vec", "1 1 1", &scalevec[0]))
	{
		VectorCopy(scalevec, scale);
	}

	// scale is stored in 'angles2'
	VectorCopy(scale, ent->s.angles2);

	trap_LinkEntity(ent);
}

/**
 * @brief SP_skyportal
 *
 * @details QUAKED props_skyportal (.6 .7 .7) (-8 -8 0) (8 8 16)
 * "fov" for the skybox default is 90
 * To have the portal sky fogged, enter any of the following values:
 * "fogcolor" (r g b) (values 0.0-1.0)
 * "fognear" distance from entity to start fogging
 * "fogfar" distance from entity that fog is opaque
 *
 * @param ent
 */
void SP_skyportal(gentity_t *ent)
{
	char   *fov;
	vec3_t fogv;
	int    fogn;
	int    fogf;
	int    isfog = 0;
	float  fov_x;

	G_SpawnString("fov", "90", &fov);
	fov_x = (float)atof(fov);

	isfog += G_SpawnVector("fogcolor", "0 0 0", fogv);
	isfog += G_SpawnInt("fognear", "0", &fogn);
	isfog += G_SpawnInt("fogfar", "300", &fogf);

	trap_SetConfigstring(CS_SKYBOXORG, va("%.2f %.2f %.2f %.1f %i %.2f %.2f %.2f %i %i", (double)ent->s.origin[0], (double)ent->s.origin[1], (double)ent->s.origin[2], (double)fov_x, isfog, (double)fogv[0], (double)fogv[1], (double)fogv[2], fogn, fogf));
}

/**
 * @brief props_statue_blocked
 *
 * @details QUAKED props_statue (.6 .3 .2) (-8 -8 0) (8 8 128) HURT DEBRIS ANIMATE KEEPBLOCK
 * "model2" will specify the model to load
 *
 * "noise"  the sound entity is to make
 *
 * "type" type of debris ("glass", "wood", "metal", "rubble") default is "wood"
 * "count" how much debris ei. default 4 pieces
 *
 * you will need to specify the bounding box for the entity
 * "high"  default is 4
 * "wide"  default is 4
 *
 * "frames"    how many frames of animation to play
 * "delay"     how long of a delay before damage is inflicted ei. 0.5 sec or 2.7 sec
 *
 * "damage"    amount of damage to be inflicted
 *
 * @param[in] ent
 */
void props_statue_blocked(gentity_t *ent)
{
	trace_t   trace;
	vec3_t    start, end, mins, maxs;
	vec3_t    forward;
	float     dist;
	gentity_t *traceEnt;


	if (!Q_stricmp(ent->classname, "props_statueBRUSH"))
	{
		return;
	}

	VectorCopy(ent->s.origin, start);
	start[2] += 24;

	VectorSet(mins, ent->r.mins[0], ent->r.mins[1], -23);
	VectorSet(maxs, ent->r.maxs[0], ent->r.maxs[1], 23);

	AngleVectors(ent->r.currentAngles, forward, NULL, NULL);

	VectorCopy(start, end);

	dist = ((ent->r.maxs[2] + 16) / ent->count2) * ent->s.frame;

	VectorMA(end, dist, forward, end);

	trap_Trace(&trace, start, mins, maxs, end, ent->s.number, MASK_SHOT);

	if (trace.surfaceFlags & SURF_NOIMPACT)     // bogus test but just in case
	{
		return;
	}

	traceEnt = &g_entities[trace.entityNum];

	if (traceEnt->takedamage && traceEnt->client)
	{
		//int  grav = 128;
		vec3_t kvel;

		G_Damage(traceEnt, ent, ent, NULL, trace.endpos, ent->damage, 0, MOD_CRUSH);

		// TBD: push client back a bit
		//VectorScale(forward, grav, kvel);
		VectorScale(forward, 128, kvel);
		VectorAdd(traceEnt->client->ps.velocity, kvel, traceEnt->client->ps.velocity);

		if (!traceEnt->client->ps.pm_time)
		{
			//int t = grav * 2;
			//if (t < 50)
			//{
			//	t = 50;
			//}
			//if (t > 200)
			//{
			//	t = 200;
			//}
			traceEnt->client->ps.pm_time   = 200;
			traceEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}
	else
	{
		G_Damage(traceEnt, ent, ent, NULL, trace.endpos, GIB_DAMAGE(traceEnt->health), 0, MOD_CRUSH);
	}
}

/**
 * @brief props_statue_animate
 * @param[in,out] ent
 */
void props_statue_animate(gentity_t *ent)
{
	qboolean takeashot = qfalse;

	ent->s.frame++;
	ent->s.eType = ET_GENERAL;

	if (ent->s.frame > ent->count2)
	{
		ent->s.frame    = ent->count2;
		ent->takedamage = qfalse;
	}

	if (((ent->delay * 1000) + ent->timestamp) > level.time)
	{
		ent->count = 0;
	}
	else if (ent->count == 5)
	{
		takeashot  = qtrue;
		ent->count = 0;
	}
	else
	{
		ent->count++;
	}

	if (takeashot)
	{
		props_statue_blocked(ent);
	}

	if (ent->s.frame < ent->count2)
	{
		ent->nextthink = level.time + 50;
	}
}

/**
 * @brief props_statue_death
 * @param[in] ent
 * @param[in] inflictor
 * @param attacker - unused
 * @param damage   - unused
 * @param mod      - unused
 */
void props_statue_death(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->timestamp = level.time;

	G_AddEvent(ent, EV_GENERAL_SOUND, ent->noise_index);

	if (!(ent->spawnflags & 8))
	{
		ent->clipmask   = 0;
		ent->r.contents = 0;
		ent->s.eType    = ET_GENERAL;
		trap_LinkEntity(ent);
	}

	ent->takedamage = qfalse;

	G_UseTargets(ent, NULL);

	if (ent->spawnflags & 2)
	{
		Spawn_Shard(ent, inflictor, ent->count, ent->key);
	}

	if (ent->spawnflags & 4)
	{
		ent->nextthink = level.time + 50;
		ent->think     = props_statue_animate;
		return;
	}

	G_FreeEntity(ent);
}

/**
 * @brief props_statue_touch
 * @param[in] self
 * @param[in] other
 * @param trace - unused
 */
void props_statue_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	props_statue_death(self, other, other, 9999, MOD_CRUSH);
}

/**
 * @brief SP_props_statue
 * @param[in,out] ent
 */
void SP_props_statue(gentity_t *ent)
{
	float    light;
	vec3_t   color;
	qboolean lightSet, colorSet;
	char     *sound;
	char     *type;
	char     *high;
	char     *wide;
	char     *frames;
	float    height;
	float    width;
	float    num_frames;

	if (ent->model2)
	{
		ent->s.modelindex = G_ModelIndex(ent->model2);
	}

	if (G_SpawnString("noise", "100", &sound))
	{
		ent->noise_index = G_SoundIndex(sound);
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat("light", "100", &light);
	colorSet = G_SpawnVector("color", "1 1 1", color);
	if (lightSet || colorSet)
	{
		int r, g, b, i;

		r = (int)(color[0] * 255);
		if (r > 255)
		{
			r = 255;
		}
		g = (int)(color[1] * 255);
		if (g > 255)
		{
			g = 255;
		}
		b = (int)(color[2] * 255);
		if (b > 255)
		{
			b = 255;
		}
		i = (int)(light / 4);
		if (i > 255)
		{
			i = 255;
		}
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}

	ent->isProp     = qtrue;
	ent->takedamage = qtrue;
	ent->die        = props_statue_death;

	G_SpawnString("type", "wood", &type);
	if (!Q_stricmp(type, "wood"))
	{
		ent->key = 1;
	}
	else if (!Q_stricmp(type, "glass"))
	{
		ent->key = 0;
	}
	else if (!Q_stricmp(type, "metal"))
	{
		ent->key = 2;
	}
	else if (!Q_stricmp(type, "rubble"))
	{
		ent->key = 3;
	}

	G_SpawnString("high", "0", &high);
	height = (float)atof(high);
	if (height == 0.f)
	{
		height = 4;
	}

	G_SpawnString("wide", "0", &wide);
	width = (float)atof(wide);

	if (width == 0.f)
	{
		width = 4;
	}

	width /= 2;

	if (Q_stricmp(ent->classname, "props_statueBRUSH"))
	{
		VectorSet(ent->r.mins, -width, -width, 0);
		VectorSet(ent->r.maxs, width, width, height);
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->s.eType    = ET_MOVER;

	G_SpawnString("frames", "0", &frames);
	num_frames = (float)atof(frames);

	ent->count2 = (int)num_frames;

	ent->touch = props_statue_touch;

	ent->r.svFlags = 0;

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!ent->damage)
	{
		ent->damage = 1;
	}

	trap_LinkEntity(ent);
}

/**
 * @brief SP_props_statueBRUSH
 *
 * @details QUAKED props_statueBRUSH (.6 .3 .2) ? HURT DEBRIS ANIMATE KEEPBLOCK
 * needs an origin brush
 *
 * "model2" will specify the model to load
 *
 * "noise"  the sound entity is to make
 *
 * "type" type of debris ("glass", "wood", "metal", "rubble") default is "wood"
 * "count" how much debris ei. default 4 pieces
 *
 * "frames"    how many frames of animation to play
 * "delay"     how long of a delay before damage is inflicted ei. 0.5 sec or 2.7 sec
 *
 * THE damage has been disabled at the moment
 * "damage"    amount of damage to be inflicted
 *
 * @param[in,out] self
 */
void SP_props_statueBRUSH(gentity_t *self)
{
	trap_SetBrushModel(self, self->model);

	SP_props_statue(self);

	if (self->model2)
	{
		self->s.modelindex2 = G_ModelIndex(self->model2);
	}

	if (!(self->health))
	{
		self->health = 6;
	}
}

//////////////////////////////////////////////////
// Lockers
//////////////////////////////////////////////////

#define LOCKER_ANIM_USEEND      5
#define LOCKER_ANIM_DEATHSTART  6
#define LOCKER_ANIM_DEATHEND    11

//////////////////////////////////////////////////
void init_locker(gentity_t *ent);
void props_locker_death(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod);
//////////////////////////////////////////////////
#define MAX_LOCKER_DEBRIS       5

int locker_debris_model[MAX_LOCKER_DEBRIS];
//////////////////////////////////////////////////

/**
 * @brief Spawn_Junk
 * @param[in] ent
 *
 * @note Unused
 */
void Spawn_Junk(gentity_t *ent)
{
	gentity_t *sfx;
	vec3_t    dir, start;

	VectorCopy(ent->r.currentOrigin, start);

	start[0] += crandom() * 32;
	start[1] += crandom() * 32;
	start[2] += 16;

	VectorSubtract(start, ent->r.currentOrigin, dir);
	VectorNormalize(dir);

	sfx = G_Spawn();

	G_SetOrigin(sfx, start);
	G_SetAngle(sfx, ent->r.currentAngles);

	G_AddEvent(sfx, EV_JUNK, DirToByte(dir));

	sfx->think = G_FreeEntity;

	sfx->nextthink = level.time + 1000;

	trap_LinkEntity(sfx);
}

/**
 * @brief props_locker_endrattle
 * @param[out] ent
 */
void props_locker_endrattle(gentity_t *ent)
{
	ent->s.frame   = 0; // idle
	ent->think     = 0;
	ent->nextthink = 0;
	ent->delay     = 0;
}

/**
 * @brief props_locker_use
 * @param[in,out] ent
 * @param other     - unused
 * @param activator - unused
 */
void props_locker_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->delay == 0.f)
	{
		ent->s.frame = 1;   // rattle when pain starts
	}
	ent->delay     = 1;
	ent->think     = props_locker_endrattle;
	ent->nextthink = level.time + 1000; // rattle a sec
}

/**
 * @brief props_locker_pain
 * @param[in] ent
 * @param[in] attacker
 * @param damage   - unused
 * @param point    - unused
 */
void props_locker_pain(gentity_t *ent, gentity_t *attacker, int damage, vec3_t point)
{
	props_locker_use(ent, attacker, attacker);
}

/**
 * @brief init_locker
 * @param[out] ent
 */
void init_locker(gentity_t *ent)
{
	ent->isProp     = qtrue;
	ent->takedamage = qtrue;
	ent->delay      = 0;

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	// TODO: change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
	ent->s.eType = ET_TRAP;

	ent->s.frame = 0;   // closed animation

	ent->count2 = LOCKER_ANIM_DEATHEND;

	ent->die  = props_locker_death;
	ent->use  = props_locker_use;   // trying it rattles the lock (could also allow 'waking' from trigger)
	ent->pain = props_locker_pain;

	// drop origin down 8 so the designer can put the box entity on the floor rather than /in/ the floor
	// remove if you get a new model from jason w/ the origin moved up 8
	ent->s.origin[2] -= 8;

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	if (!(ent->health))
	{
		ent->health = 1;
	}

	trap_LinkEntity(ent);
}

/**
 * @brief props_locker_spawn_item
 * @param[in] ent
 */
void props_locker_spawn_item(gentity_t *ent)
{
	gitem_t   *item;
	gentity_t *drop;

	item = BG_FindItem(ent->spawnitem);

	if (!item)     // empty
	{
		return;
	}

	//drop = Drop_Item (ent, item, 0, qtrue);
	drop = LaunchItem(item, ent->r.currentOrigin, tv(0, 0, 20), ent->s.number);


	if (!drop)
	{
		G_Printf("-----> WARNING <-------\n");
		G_Printf("props_locker_spawn_item at %s failed!\n", vtos(ent->r.currentOrigin));
	}
}

/**
 * @brief props_locker_mass
 * @param[in] ent
 *
 * @note Unused
 */
void props_locker_mass(gentity_t *ent)
{
	gentity_t *tent;
	vec3_t    start;
	vec3_t    dir;

	VectorCopy(ent->r.currentOrigin, start);

	start[0] += crandom() * 32;
	start[1] += crandom() * 32;
	start[2] += 16;

	VectorSubtract(start, ent->r.currentOrigin, dir);
	VectorNormalize(dir);

	tent = G_TempEntity(ent->r.currentOrigin, EV_EFFECT);
	VectorCopy(ent->r.currentOrigin, tent->s.origin);
	VectorCopy(dir, tent->s.angles2);

	tent->s.dl_intensity = 0;

	trap_SetConfigstring(CS_TARGETEFFECT, ent->dl_shader);      // allow shader to be set from entity

	tent->s.frame = ent->key;

	tent->s.eventParm = 8;
	tent->s.density   = 100;
}

/**
 * @brief props_locker_death
 *
 * @details QUAKED props_footlocker (.6 .7 .3) (-12 -21 -12) (12 21 12) ? NO_JUNK
 * "noise"  the sound entity is to make upon death
 * the default sounds are:
 *   "wood"    - "sound/world/boardbreak.wav"
 *   "glass"   - "sound/world/glassbreak.wav"
 *   "metal"   - "sound/world/metalbreak.wav"
 *   "gibs"    - "sound/player/gibsplit1.wav"
 *   "brick"   - "sound/world/debris1.wav"
 *   "stone"   - "sound/world/stonefall.wav"
 *   "fabric"  - "sound/world/fabricbreak.wav"
 *
 * "locknoise" the locked sound to play
 * "wait"   denotes how long the wait is going to be before the locked sound is played again default is 1 sec
 * "health" default is 1
 *
 * "spawnitem" - will spawn this item unpon death use the pickup_name ei 9mm
 *
 * "type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
 * "mass" - defaults to 75.  This determines how much debris is emitted.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.
 *
 * "dl_shader" needs to be set the same way as a target_effect
 *
 * @todo FIXME/TBD: the spawning of junk still pending and animation when used
 *
 * -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
 * model="models/mapobjects/furniture/footlocker.md3"
 *
 * @param[in,out] ent
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void props_locker_death(gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	ent->takedamage = qfalse;
	ent->s.frame    = 2; // opening animation
	ent->think      = 0;
	ent->nextthink  = 0;

	trap_UnlinkEntity(ent);
	ent->r.maxs[2] = 11;    // make the dead bb half height so the item can look like it's sitting inside
	props_locker_spawn_item(ent);
	trap_LinkEntity(ent);
}

/**
 * @brief SP_props_footlocker
 * @param[in,out] self
 */
void SP_props_footlocker(gentity_t *self)
{
	char *type;
	char *sound;
	char *locked;
	int  mass;

	//trap_SetBrushModel (self, self->model);

	// if angle is xx or yy, rotate the bounding box 90 deg to match
	// NOTE:    Non axis-aligned orientation not allowed.  It will work, but
	//          the bounding box will not exactly match the model.
	if (self->s.angles[1] == 90.f || self->s.angles[1] == 270.f)
	{
		VectorSet(self->r.mins, -21, -12, 0);
		VectorSet(self->r.maxs, 21, 12, 24);
	}
	else
	{
		VectorSet(self->r.mins, -12, -21, 0);
		VectorSet(self->r.maxs, 12, 21, 24);
	}

	self->s.modelindex = G_ModelIndex("models/mapobjects/furniture/footlocker.md3");

	if (G_SpawnString("noise", "NOSOUND", &sound))
	{
		self->noise_index = G_SoundIndex(sound);
	}

	if (G_SpawnString("locknoise", "NOSOUND", &locked))
	{
		self->soundPos1 = G_SoundIndex(locked);
	}

	if (self->wait == 0.f)
	{
		self->wait = 1000;
	}
	else
	{
		self->wait *= 1000;
	}

	if (G_SpawnInt("mass", "75", &mass))
	{
		self->count = mass;
	}
	else
	{
		self->count = 75;
	}

	if (G_SpawnString("type", "wood", &type))
	{
		if (!Q_stricmp(type, "wood"))
		{
			self->key = FXTYPE_WOOD;
		}
		else if (!Q_stricmp(type, "glass"))
		{
			self->key = FXTYPE_GLASS;
		}
		else if (!Q_stricmp(type, "metal"))
		{
			self->key = FXTYPE_METAL;
		}
		else if (!Q_stricmp(type, "gibs"))
		{
			self->key = FXTYPE_GIBS;
		}
		else if (!Q_stricmp(type, "brick"))
		{
			self->key = FXTYPE_BRICK;
		}
		else if (!Q_stricmp(type, "rock"))
		{
			self->key = FXTYPE_STONE;
		}
		else if (!Q_stricmp(type, "fabric"))
		{
			self->key = FXTYPE_FABRIC; // FIXME: test this
		}
	}
	else
	{
		self->key = 0;
	}

	self->delay = level.time + self->wait;

	init_locker(self);
}

/**
 * @brief props_flamethrower_think
 *
 * @details QUAKED props_flamethrower (.6 .7 .3) (-8 -8 -8) (8 8 8) TRACKING NOSOUND
 * the effect occurs when this entity is used
 * needs to aim at a info_notnull
 * "duration" how long the effect is going to last for example 1.2 sec 2.7 sec
 * "random" how long of a random variance so the effect isnt exactly the same each time for example 1.1 sec or 0.2 sec
 * "size" valid ranges are 1.0 to 0.1
 *
 * NOSOUND - silent (duh)
 *
 * @param[in,out] ent
 */
void props_flamethrower_think(gentity_t *ent)
{
	gentity_t *target = NULL;
	// actually create flamechunks that do damage in this direction
	vec3_t flameDir;

	if (ent->spawnflags & 1)     // tracking
	{
		if (ent->target)
		{
			target = G_FindByTargetname(NULL, ent->target);
		}

		if (!target)
		{
			//VectorSet (ent->r.currentAngles, 0, 0, 1);  // wasn't working
			VectorSet(ent->s.apos.trBase, 0, 0, 1);
			// try that for the flame too
			VectorSet(flameDir, 0, 0, 1);
		}
		else
		{
			vec3_t angles, vec;

			VectorSubtract(target->s.origin, ent->s.origin, vec);
			VectorNormalize(vec);
			vectoangles(vec, angles);
			//VectorCopy (angles, ent->r.currentAngles);  // wasn't working
			VectorCopy(angles, ent->s.apos.trBase);

			// we want the vector going the other way for the flame
			VectorSubtract(ent->s.origin, target->s.origin, flameDir);
		}
	}
	else
	{
		if (ent->target)
		{
			target = G_FindByTargetname(NULL, ent->target);
		}

		if (!target)
		{
			// try that for the flame too
			VectorSet(flameDir, 0, 0, 1);
		}
		else
		{
			// we want the vector going the other way for the flame
			VectorSubtract(ent->s.origin, target->s.origin, flameDir);
		}
	}

	if ((ent->timestamp + ent->duration) > level.time)
	{
		G_AddEvent(ent, EV_FLAMETHROWER_EFFECT, 0);

		ent->nextthink = level.time + 50;

		//      The flamethrower effect above is purely visual
		//      we actual need to create an entity that is the fire and will do damage
		VectorNormalize(flameDir);
		VectorScale(flameDir, FLAME_START_SPEED, flameDir);

		fire_flamechunk(ent, ent->r.currentOrigin, flameDir);

		{
			int rnd;

			if (ent->random != 0.f)
			{
				int rval = (int)(ent->random * 1000);

				rnd = rand() % rval;
			}
			else
			{
				rnd = 0;
			}

			ent->timestamp = level.time + rnd;
			ent->nextthink = ent->timestamp + 50;
		}
	}
}

/**
 * @brief props_flamethrower_use
 * @param[in,out] ent
 * @param other     - unused
 * @param activator - unused
 */
void props_flamethrower_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	int rnd;

	if (ent->spawnflags & 2)
	{
		ent->spawnflags &= ~2;
		ent->think       = NULL; // wasn't working
		ent->nextthink   = 0;
		return;
	}
	else
	{
		ent->spawnflags |= 2;
	}

	if (ent->random != 0.f)
	{
		int rval = (int)(ent->random * 1000);

		rnd = rand() % rval;
	}
	else
	{
		rnd = 0;
	}

	ent->timestamp = level.time + rnd;

	ent->think     = props_flamethrower_think;
	ent->nextthink = level.time + 50;
}

/**
 * @brief props_flamethrower_init
 * @param[in,out] ent
 */
void props_flamethrower_init(gentity_t *ent)
{
	gentity_t *target = NULL;

	if (ent->target)
	{
		target = G_FindByTargetname(NULL, ent->target);
	}

	if (!target)
	{
		//VectorSet (ent->r.currentAngles, 0, 0, 1);
		VectorSet(ent->s.apos.trBase, 0, 0, 1);
	}
	else
	{
		vec3_t vec;
		vec3_t angles;

		VectorSubtract(target->s.origin, ent->s.origin, vec);
		VectorNormalize(vec);
		vectoangles(vec, angles);
		//VectorCopy (angles, ent->r.currentAngles);
		VectorCopy(angles, ent->s.apos.trBase);
		VectorCopy(angles, ent->s.angles);   // added to fix weird release build issues
	}

	trap_LinkEntity(ent);
}

/**
 * @brief SP_props_flamethrower
 * @param[in,out] ent
 */
void SP_props_flamethrower(gentity_t *ent)
{
	char  *size;
	float dsize;

	ent->think     = props_flamethrower_init;
	ent->nextthink = level.time + 50;
	ent->use       = props_flamethrower_use;

	G_SetOrigin(ent, ent->s.origin);

	if (ent->duration == 0.f)
	{
		ent->duration = 1000;
	}
	else
	{
		ent->duration *= 1000;
	}

	G_SpawnString("size", "0", &size);
	dsize = (float)atof(size);
	if (dsize == 0.f)
	{
		dsize = 1;
	}
	ent->accuracy = dsize;
}
