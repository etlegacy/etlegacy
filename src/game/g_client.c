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
 * @file g_client.c
 * @brief Client functions that don't happen every frame
 */

#include "g_local.h"
#include "../../etmain/ui/menudef.h"
#include "g_strparse.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

#ifdef FEATURE_SERVERMDX
#include "g_mdx.h"
#endif

// new bounding box
vec3_t playerMins = { -18, -18, -24 };
vec3_t playerMaxs = { 18, 18, 48 };

/**
 * @brief QUAKED info_player_deathmatch (1 0 1) (-18 -18 -24) (18 18 48)
 * potential spawning position for deathmatch games.
 * Targets will be fired when someone spawns in on them.
 * "nobots" will prevent bots from using this spot.
 * "nohumans" will prevent non-bots from using this spot.
 *
 * If the start position is targeting an entity, the players camera will start out facing that ent (like an info_notnull)
 *
 * @param[in,out] ent
 */
void SP_info_player_deathmatch(gentity_t *ent)
{
	int i;

	G_SpawnInt("nobots", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i)
	{
		ent->flags |= FL_NO_HUMANS;
	}

	ent->enemy = G_PickTarget(ent->target);
	if (ent->enemy)
	{
		vec3_t dir;

		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}
}

/**
 * @brief QUAKED info_player_checkpoint (1 0 0) (-16 -16 -24) (16 16 32) a b c d
 * these are start points /after/ the level start
 * the letter (a b c d) designates the checkpoint that needs to be complete in order to use this start position
 *
 * @param[in] ent
 */
void SP_info_player_checkpoint(gentity_t *ent)
{
	ent->classname = "info_player_checkpoint";
	SP_info_player_deathmatch(ent);
}

/**
 * @brief QUAKED info_player_start (1 0 0) (-18 -18 -24) (18 18 48)
 * equivelant to info_player_deathmatch
 * @param ent
 */
void SP_info_player_start(gentity_t *ent)
{
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/**
 * @brief QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) AXIS ALLIED
 * The intermission will be viewed from this point.  Target an info_notnull for the view direction.
 *
 * @param ent - unused
 */
void SP_info_player_intermission(gentity_t *ent)
{
}

/*
=======================================================================
  SelectSpawnPoint
=======================================================================
*/

/**
 * @brief SpotWouldTelefrag
 * @param[in] spot
 * @return
 */
qboolean SpotWouldTelefrag(gentity_t *spot)
{
	int       i, num;
	int       touch[MAX_GENTITIES];
	gentity_t *hit;
	vec3_t    mins, maxs;

	VectorAdd(spot->r.currentOrigin, playerMins, mins);
	VectorAdd(spot->r.currentOrigin, playerMaxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (i = 0 ; i < num ; i++)
	{
		hit = &g_entities[touch[i]];
		if (hit->client && hit->client->ps.stats[STAT_HEALTH] > 0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Find the spot that we DON'T want to use
 * @param[in] from
 * @return
 */
gentity_t *SelectNearestDeathmatchSpawnPoint(vec3_t from)
{
	gentity_t *spot = NULL;
	float     dist, nearestDist = 999999;
	gentity_t *nearestSpot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		dist = VectorDistance(spot->r.currentOrigin, from);
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

#define MAX_SPAWN_POINTS    128

/**
 * @brief Go to a random point that doesn't telefrag
 * @return
 */
gentity_t *SelectRandomDeathmatchSpawnPoint(void)
{
	gentity_t *spot = NULL;
	int       count = 0;
	int       selection;
	gentity_t *spots[MAX_SPAWN_POINTS];

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if (SpotWouldTelefrag(spot))
		{
			continue;
		}
		spots[count] = spot;
		count++;
	}

	if (!count)     // no spots that won't telefrag
	{
		return G_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[selection];
}

/**
 * @brief Chooses a player start, deathmatch start, etc
 *
 * @param[in] avoidPoint
 * @param[in] origin
 * @param[in] angles
 * @return
 */
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles)
{
	gentity_t *nearestSpot;
	gentity_t *spot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint(avoidPoint);
	spot        = SelectRandomDeathmatchSpawnPoint();

	if (spot == nearestSpot)
	{
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint();
		if (spot == nearestSpot)
		{
			// last try
			spot = SelectRandomDeathmatchSpawnPoint();
		}
	}

	// find a player start spot
	if (!spot)
	{
		G_Error("Couldn't find a spawn point\n");
	}

	VectorCopy(spot->r.currentOrigin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

/**
 * @brief SelectSpectatorSpawnPoint
 * @param[out] origin
 * @param[out] angles
 * @return
 */
gentity_t *SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles)
{
	FindIntermissionPoint();

	VectorCopy(level.intermission_origin, origin);
	VectorCopy(level.intermission_angle, angles);

	return NULL;
}

/*
=======================================================================
BODYQUE
=======================================================================
*/

/**
 * @brief InitBodyQue
 */
void InitBodyQue(void)
{
	int       i;
	gentity_t *ent;

	// no need to init when dyn BQ is set
	if (g_corpses.integer > 0)
	{
		return;
	}

	level.bodyQueIndex = 0;
	for (i = 0; i < BODY_QUEUE_SIZE ; i++)
	{
		ent              = G_Spawn();
		ent->classname   = "bodyque";
		ent->neverFree   = qtrue;
		level.bodyQue[i] = ent;
	}
}

/**
 * @brief Called by BodySink
 * @param[in,out] ent
 */
void BodyUnlink(gentity_t *ent)
{
	gentity_t *tent;

	tent = G_TempEntity(ent->r.currentOrigin, EV_BODY_DP);     // so clients will Com_Memset them off

	tent->s.otherEntityNum2 = ent->s.number;
	tent->r.svFlags         = SVF_BROADCAST; // send to everyone

	trap_UnlinkEntity(ent);
	ent->physicsObject = qfalse;
}

/**
 * @brief G_BodyDP
 * @param[in,out] ent
 */
void G_BodyDP(gentity_t *ent)
{
	gentity_t *tent;

	tent = G_TempEntity(ent->r.currentOrigin, EV_BODY_DP);     // so clients will Com_Memset them off

	tent->s.otherEntityNum2 = ent->s.number;
	tent->r.svFlags         = SVF_BROADCAST; // send to everyone

	G_FreeEntity(ent);
}

/**
 * @brief After sitting around for five seconds, fall into the ground and dissapear
 * @param[in,out] ent
 */
void BodySink2(gentity_t *ent)
{
	ent->physicsObject = qfalse;
	ent->nextthink     = level.time + 1800;
	ent->think         = BodyUnlink;

	if (g_corpses.integer == 0)
	{
		ent->think = BodyUnlink;
	}
	else
	{
		// let's free the dead guy
		ent->think = G_BodyDP;
	}

	ent->s.pos.trType = TR_LINEAR;
	ent->s.pos.trTime = level.time;
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	VectorSet(ent->s.pos.trDelta, 0, 0, -8);
}

/**
 * @brief After sitting around for five seconds, fall into the ground and dissapear
 * @param[in,out] ent
 */
void BodySink(gentity_t *ent)
{
	if (ent->activator)
	{
		// see if parent is still disguised
		if (ent->activator->client->ps.powerups[PW_OPS_DISGUISED])
		{
			ent->nextthink = level.time + FRAMETIME;
			return;
		}
		else
		{
			ent->activator = NULL;
		}
	}

	BodySink2(ent);
}

#ifdef FEATURE_SERVERMDX

/**
 * @brief G_IsPositionOK
 * @param[in,out] ent
 * @param[in] newOrigin
 * @return
 */
static qboolean G_IsPositionOK(gentity_t *ent, vec3_t newOrigin)
{
	trace_t trace;

	trap_TraceCapsule(&trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, newOrigin, ent->s.number, MASK_PLAYERSOLID);

	if (trace.fraction == 1.f)
	{
		VectorCopy(trace.endpos, ent->s.pos.trBase);
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief G_StepSlideCorpse
 *
 * @param[in,out] ent
 * @param[in] newOrigin
 *
 * @note note that this is only with first stage, corpse can just use slidemove
 */
static void G_StepSlideCorpse(gentity_t *ent, vec3_t newOrigin)
{
	vec3_t  start, down, up;
	trace_t trace;

	VectorCopy(ent->s.pos.trBase, start);

	if (G_IsPositionOK(ent, newOrigin))
	{
		// so check if we can fall even more down

		VectorCopy(ent->s.pos.trBase, down);
		down[2] -= 16;
		// item code is using these
		trap_TraceCapsule(&trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_PLAYERSOLID);
		if (trace.fraction == 1.f)
		{
			// begin with falling again
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
		else
		{
			VectorCopy(trace.endpos, ent->s.pos.trBase);
		}

		return;     // we got exactly where we wanted to go first try
	}

	VectorCopy(ent->s.pos.trBase, down);

	down[2] -= 18;

	trap_TraceCapsule(&trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_PLAYERSOLID);

	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if (ent->s.pos.trDelta[2] > 0 && (trace.fraction == 1.0f || DotProduct(trace.plane.normal, up) < 0.7f))
	{
		return;
	}

	VectorCopy(ent->s.pos.trBase, down);

	VectorCopy(start, up);
	up[2] += 18;

	// test the player position if they were a stepheight higher
	trap_TraceCapsule(&trace, start, ent->r.mins, ent->r.maxs, up, ent->s.number, MASK_PLAYERSOLID);

	if (trace.allsolid)
	{
		return;     // can't step up
	}

	// try slidemove from this position
	VectorCopy(trace.endpos, ent->s.pos.trBase);

	G_IsPositionOK(ent, newOrigin);

	// push down the final amount
	VectorCopy(ent->s.pos.trBase, down);
	down[2] -= 18;

	Com_Memset(&trace, 0, sizeof(trace));

	trap_TraceCapsule(&trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_PLAYERSOLID);

	if (!trace.allsolid)
	{
		VectorCopy(trace.endpos, ent->s.pos.trBase);
	}

	// so check if we can fall even more down
	if (trace.fraction == 1.f)
	{
		down[2] -= 16;
		// item code is using these
		trap_TraceCapsule(&trace, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, down, ent->s.number, MASK_PLAYERSOLID);
		if (trace.fraction == 1.f)
		{
			// begin with falling again
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
		else
		{
			VectorCopy(trace.endpos, ent->s.pos.trBase);
		}
	}
}
#endif

/**
 * @brief A player is respawning, so make an entity that looks
 * just like the existing corpse to leave behind.
 * @param[in] ent
 */
void CopyToBodyQue(gentity_t *ent)
{
	gentity_t *body;

	trap_UnlinkEntity(ent);

	// if client is in a nodrop area, don't leave the body
	if (trap_PointContents(ent->client->ps.origin, -1) & CONTENTS_NODROP)
	{
		return;
	}

	if (g_corpses.integer == 0)
	{
		// grab a body que and cycle to the next one
		body               = level.bodyQue[level.bodyQueIndex];
		level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;
	}
	else
	{
		body = G_Spawn();
	}

	body->s        = ent->s;
	body->s.eFlags = EF_DEAD;       // clear EF_TALK, etc

	if (ent->client->ps.eFlags & EF_HEADSHOT)
	{
		body->s.eFlags |= EF_HEADSHOT;          // make sure the dead body draws no head (if killed that way)
	}

	body->s.eType   = ET_CORPSE;
	body->classname = "corpse";

	body->s.powerups    = 0; // clear powerups
	body->s.loopSound   = 0; // clear lava burning
	body->s.number      = body - g_entities;
	body->timestamp     = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;        // don't bounce
	//body->physicsSlide = qfalse;
	//body->physicsFlush = qfalse;

	VectorCopy(ent->client->ps.viewangles, body->s.angles);
	VectorCopy(ent->client->ps.viewangles, body->r.currentAngles);

	if (body->s.groundEntityNum == ENTITYNUM_NONE)
	{
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy(ent->client->ps.velocity, body->s.pos.trDelta);
	}
	else
	{
		body->s.pos.trType     = TR_STATIONARY;
		body->s.pos.trTime     = 0;
		body->s.pos.trDuration = 0;
		VectorClear(body->s.pos.trDelta);
	}

	body->s.event = 0;

	// Clear out event system
	Com_Memset(body->s.events, 0, sizeof(body->s.events));
	body->s.eventSequence = 0;

	// time needed to complete animation
	body->s.torsoAnim = ent->client->torsoDeathAnim;
	body->s.legsAnim  = ent->client->legsDeathAnim;

	body->r.svFlags = ent->r.svFlags & ~SVF_BOT;
	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);

	//  bodies have lower bounding box
	body->r.maxs[2] = DEAD_BODYHEIGHT_BBOX;

	body->s.effect1Time = ent->client->deathAnimTime;

	// this'll look better
	if (body->s.onFireEnd > level.time)
	{
		body->s.onFireEnd = ent->client->deathAnimTime + 1500;
	}

	VectorCopy(body->s.pos.trBase, body->r.currentOrigin);

#ifdef FEATURE_SERVERMDX
	if (ent->client->deathAnim)
	{
		vec3_t       origin, offset;
		grefEntity_t refent;

		mdx_PlayerAnimation(body);
		mdx_gentity_to_grefEntity(body, &refent, level.time);
		mdx_tag_position(body, &refent, origin, "tag_ubelt", 0, 0);

		// convert it to vector around null pos
		offset[0] = origin[0] - body->r.currentOrigin[0];
		offset[1] = origin[1] - body->r.currentOrigin[1];
		offset[2] = origin[2] = body->r.currentOrigin[2];

		G_StepSlideCorpse(body, origin);
		// this is the max vector we can reach
		VectorCopy(body->s.pos.trBase, origin);

		// change bounding box to be off the origin of the corpse
		// that will make correct box agains model
		// NOTE: we force rounding number for avoiding unwanted colision
		body->r.maxs[0] = (int)(offset[0] + playerMaxs[0]);
		body->r.maxs[1] = (int)(offset[1] + playerMaxs[1]);
		body->r.mins[0] = (int)(offset[0] + playerMins[0]);
		body->r.mins[1] = (int)(offset[1] + playerMins[1]);

		body->r.currentOrigin[0] = origin[0] - offset[0];
		body->r.currentOrigin[1] = origin[1] - offset[1];
		body->r.currentOrigin[2] = origin[2] + 1;           // make sure it is off ground

		// ok set it und Fertig!
		VectorCopy(body->r.currentOrigin, body->s.pos.trBase);
	}
#endif

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	// allow bullets to pass through bbox
	// need something to allow the hint for covert ops
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->r.ownerNum;

	BODY_TEAM(body)      = ent->client->sess.sessionTeam;
	BODY_CLASS(body)     = ent->client->sess.playerType;
	BODY_CHARACTER(body) = ent->client->pers.characterIndex;
	BODY_VALUE(body) = 0;

	//if ( ent->client->ps.eFlags & EF_PANTSED ){
	//	body->s.time2 =	1;
	//}
	//else {
	body->s.time2 = 0;
	//}

	body->activator = NULL;
	body->nextthink = level.time + BODY_TIME;
	body->think     = BodySink;
	body->die       = body_die;

	// don't take more damage if already gibbed
	body->takedamage = ent->health > GIB_HEALTH;

	trap_LinkEntity(body);
}

//======================================================================

/**
 * @brief SetClientViewAngle
 * @param[in,out] ent
 * @param[in] angle
 */
void SetClientViewAngle(gentity_t *ent, vec3_t angle)
{
	int i;
	int cmdAngle;

	// set the delta angle
	for (i = 0 ; i < 3 ; i++)
	{
		cmdAngle                        = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy(angle, ent->s.angles);
	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
}

/**
 * @brief G_DropLimboHealth
 * @param[in,out] ent
 */
void G_DropLimboHealth(gentity_t *ent)
{
	if (g_dropHealth.integer == 0 || !ent->client || ent->client->sess.playerType != PC_MEDIC || g_gamestate.integer != GS_PLAYING)
	{
		return;
	}

	{
		vec3_t launchvel, launchspot;
		int    i, packs = g_dropHealth.integer;
		int    cwt = ent->client->ps.classWeaponTime;

		if (g_dropHealth.integer == -1 || g_dropHealth.integer > 5)
		{
			packs                            = 5; // max packs
			ent->client->ps.classWeaponTime += (level.time - ent->client->deathTime + 10000);
		}

		for (i = 0; i < packs; ++i)
		{
			if (g_dropHealth.integer == -1 && ent->client->ps.classWeaponTime >= level.time)
			{
				break;
			}
			launchvel[0] = crandom();
			launchvel[1] = crandom();
			launchvel[2] = 0;
			VectorScale(launchvel, 100, launchvel);
			launchvel[2] = 100;
			VectorCopy(ent->r.currentOrigin, launchspot);

			Weapon_Medic_Ext(ent, launchspot, launchspot, launchvel);
		}
		ent->client->ps.classWeaponTime = cwt;
	}
}

/**
 * @brief G_DropLimboAmmo
 * @param[in,out] ent
 */
void G_DropLimboAmmo(gentity_t *ent)
{
	if (g_dropAmmo.integer == 0 || !ent->client || ent->client->sess.playerType != PC_FIELDOPS)
	{
		return;
	}

	{
		vec3_t launchvel, launchspot;
		int    i, packs = g_dropAmmo.integer;
		int    cwt = ent->client->ps.classWeaponTime;

		if (g_dropAmmo.integer == -1 || g_dropAmmo.integer > 5)
		{
			packs = 5; // max packs
			// adjust the charge timer so that the fops only drops as much ammo as he could have before he died.
			// don't ask me where the 10000 comes from, it just makes this more accurate.
			ent->client->ps.classWeaponTime += (level.time - ent->client->deathTime + 10000);
		}

		for (i = 0; i < packs; ++i)
		{
			if (g_dropAmmo.integer == -1 && ent->client->ps.classWeaponTime >= level.time)
			{
				break;
			}

			launchvel[0] = crandom();
			launchvel[1] = crandom();
			launchvel[2] = 0;

			VectorScale(launchvel, 100, launchvel);

			launchvel[2] = 100;
			VectorCopy(ent->r.currentOrigin, launchspot);

			Weapon_MagicAmmo_Ext(ent, launchspot, launchspot, launchvel);
		}

		ent->client->ps.classWeaponTime = cwt;
	}
}

/**
 * @brief limbo
 * @param[in,out] ent
 * @param[in] makeCorpse
 */
void limbo(gentity_t *ent, qboolean makeCorpse)
{
	if (!(ent->client->ps.pm_flags & PMF_LIMBO))
	{
		gclient_t *cl;
		int       i, contents;
		int       startclient = ent->client->ps.clientNum;

		if (ent->client->ps.persistant[PERS_RESPAWNS_LEFT] == 0)
		{
			if (g_maxlivesRespawnPenalty.integer)
			{
				ent->client->ps.persistant[PERS_RESPAWNS_PENALTY] = g_maxlivesRespawnPenalty.integer;
			}
			else
			{
				ent->client->ps.persistant[PERS_RESPAWNS_PENALTY] = -1;
			}
		}

		// First save off persistant info we'll need for respawn
		for (i = 0; i < MAX_PERSISTANT; i++)
		{
			ent->client->saved_persistant[i] = ent->client->ps.persistant[i];
		}

		if (g_stickyCharge.integer > 0 && ent->client->ps.classWeaponTime >= 0)
		{
			switch (ent->client->sess.playerType)
			{
			case PC_MEDIC:
				ent->client->pers.savedClassWeaponTimeMed = level.time - ent->client->ps.classWeaponTime;
				break;
			case PC_ENGINEER:
				ent->client->pers.savedClassWeaponTimeEng = level.time - ent->client->ps.classWeaponTime;
				break;
			case PC_FIELDOPS:
				ent->client->pers.savedClassWeaponTimeFop = level.time - ent->client->ps.classWeaponTime;
				break;
			case PC_COVERTOPS:
				ent->client->pers.savedClassWeaponTimeCvop = level.time - ent->client->ps.classWeaponTime;
				break;
			default:
				ent->client->pers.savedClassWeaponTime = level.time - ent->client->ps.classWeaponTime;
			}
		}


		ent->client->ps.pm_flags |= PMF_LIMBO;
		ent->client->ps.pm_flags |= PMF_FOLLOW;

#ifdef FEATURE_OMNIBOT
		ent->client->sess.botSuicide = qfalse; // cs: avoid needlessly /killing at next spawn
#endif

		if (makeCorpse)
		{
			G_DropLimboHealth(ent);
			G_DropLimboAmmo(ent);
			CopyToBodyQue(ent);   // make a nice looking corpse
		}
		else
		{
			trap_UnlinkEntity(ent);
		}

		// reset these values
		ent->client->ps.viewlocked        = VIEWLOCK_NONE;
		ent->client->ps.viewlocked_entNum = 0;

		ent->r.maxs[2]           = 0;
		ent->r.currentOrigin[2] += 8;
		contents                 = trap_PointContents(ent->r.currentOrigin, -1); // drop stuff
		ent->s.weapon            = ent->client->limboDropWeapon; // stored in player_die()
		if (makeCorpse && !(contents & CONTENTS_NODROP))
		{
			TossWeapons(ent);
		}

		if (G_FollowSame(ent))
		{
			ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		}
		else
		{
			ent->client->sess.spectatorClient = startclient;
			Cmd_FollowCycle_f(ent, 1, qfalse); // get fresh spectatorClient
			if (ent->client->sess.spectatorClient == startclient)
			{
				// No one to follow, so just stay put
				ent->client->sess.spectatorState = SPECTATOR_FREE;
			}
			else
			{
				ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
			}
		}

		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];

			if ((cl->ps.pm_flags & PMF_LIMBO) && cl->sess.spectatorClient == ent - g_entities)
			{
				Cmd_FollowCycle_f(&g_entities[level.sortedClients[i]], 1, qfalse);
			}
		}
	}
}

/**
 * @brief Called when time expires for a team deployment cycle and there is at least one guy ready to go
 * @param[in,out] ent
 */
void reinforce(gentity_t *ent)
{
	int p;

	if (!(ent->client->ps.pm_flags & PMF_LIMBO))
	{
		G_Printf("player already deployed, skipping\n");
		return;
	}

#ifdef FEATURE_MULTIVIEW
	if (ent->client->pers.mvCount > 0)
	{
		G_smvRemoveInvalidClients(ent, TEAM_AXIS);
		G_smvRemoveInvalidClients(ent, TEAM_ALLIES);
	}
#endif
	// restore persistant data now that we're out of Limbo
	for (p = 0; p < MAX_PERSISTANT; p++)
	{
		ent->client->ps.persistant[p] = ent->client->saved_persistant[p];
	}

	respawn(ent);
}

/**
 * @brief respawn
 * @param[in,out] ent
 */
void respawn(gentity_t *ent)
{
	ent->client->ps.pm_flags &= ~PMF_LIMBO; // turns off limbo

	// Decrease the number of respawns left
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if (ent->client->ps.persistant[PERS_RESPAWNS_LEFT] > 0 && g_gamestate.integer == GS_PLAYING)
		{
			if (g_maxlives.integer > 0)
			{
				ent->client->ps.persistant[PERS_RESPAWNS_LEFT]--;
			}
			else
			{
				if (g_alliedmaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_ALLIES)
				{
					ent->client->ps.persistant[PERS_RESPAWNS_LEFT]--;
				}
				if (g_axismaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_AXIS)
				{
					ent->client->ps.persistant[PERS_RESPAWNS_LEFT]--;
				}
			}
		}
	}

	G_DPrintf("Respawning %s, %i lives left\n", ent->client->pers.netname, ent->client->ps.persistant[PERS_RESPAWNS_LEFT]);

	ClientSpawn(ent, qfalse, qfalse, qtrue);
}

/**
 * @brief Count the number of players on a team
 *
 * @param[in] ignoreClientNum
 * @param[in] team
 *
 * @return Number of players on a team
 */
int TeamCount(int ignoreClientNum, team_t team)
{
	int i, ref, count = 0;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		if ((ref = level.sortedClients[i]) == ignoreClientNum)
		{
			continue;
		}
		if (level.clients[ref].sess.sessionTeam == team)
		{
			count++;
		}
	}

	return count;
}

/**
 * @brief PickTeam
 * @param[in] ignoreClientNum
 * @return
 */
team_t PickTeam(int ignoreClientNum)
{
	int counts[TEAM_NUM_TEAMS] = { 0, 0, 0 };

	counts[TEAM_ALLIES] = TeamCount(ignoreClientNum, TEAM_ALLIES);
	counts[TEAM_AXIS]   = TeamCount(ignoreClientNum, TEAM_AXIS);

	if (counts[TEAM_ALLIES] > counts[TEAM_AXIS])
	{
		return(TEAM_AXIS);
	}
	if (counts[TEAM_AXIS] > counts[TEAM_ALLIES])
	{
		return(TEAM_ALLIES);
	}

	// equal team count, so join the team with the lowest score
	return(((level.teamScores[TEAM_ALLIES] > level.teamScores[TEAM_AXIS]) ? TEAM_AXIS : TEAM_ALLIES));
}

/**
 * @brief AddExtraSpawnAmmo
 * @param[in,out] client
 * @param[in] weaponNum
 */
static void AddExtraSpawnAmmo(gclient_t *client, weapon_t weaponNum)
{
	// no extra ammo if it don't use ammo
	if (!GetWeaponTableData(weaponNum)->useAmmo)
	{
		return;
	}

	if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_PISTOL)
	{
		if (client->sess.skill[SK_LIGHT_WEAPONS] >= 1)
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SMG)
	{
		if (client->sess.skill[SK_LIGHT_WEAPONS] >= 1)
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLENADE)
	{
		if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 4;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_GRENADE)
	{
		if (client->sess.playerType == PC_ENGINEER)
		{
			if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
			{
				client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 4;
			}
		}
		if (client->sess.playerType == PC_MEDIC)
		{
			if (client->sess.skill[SK_FIRST_AID] >= 1)
			{
				client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 1;
			}
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SYRINGUE)
	{
		if (client->sess.skill[SK_FIRST_AID] >= 2)
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 2;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLE)
	{
		if (client->sess.skill[SK_LIGHT_WEAPONS] >= 1 || (client->sess.playerType == PC_COVERTOPS && client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1))
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
}

/**
 * @brief AddWeaponToPlayer
 * @param[in,out] client
 * @param[in] weapon
 * @param[in] ammo
 * @param[in] ammoclip
 * @param[in] setcurrent
 */
void AddWeaponToPlayer(gclient_t *client, weapon_t weapon, int ammo, int ammoclip, qboolean setcurrent)
{
	if (team_riflegrenades.integer == 0)
	{
		switch (weapon)
		{
		case WP_GPG40:
		case WP_M7:
			return;
		default:
			break;
		}
	}

	COM_BitSet(client->ps.weapons, weapon);
	client->ps.ammoclip[GetWeaponTableData(weapon)->clipIndex] = ammoclip;
	client->ps.ammo[GetWeaponTableData(weapon)->ammoIndex]    += ammo;

	if (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		client->ps.ammoclip[GetWeaponTableData(GetWeaponTableData(weapon)->akimboSideArm)->clipIndex] = ammoclip;
	}

	if (weapon == WP_BINOCULARS)
	{
		client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
	}

	if (setcurrent)
	{
		client->ps.weapon = weapon;
	}

	// skill handling
	AddExtraSpawnAmmo(client, weapon);

	// add alternative weapon if exist for primary weapon
	if (GetWeaponTableData(weapon)->weapAlts)
	{
		// Covertops got silenced secondary weapon
		if ((GetWeaponTableData(weapon)->type & WEAPON_TYPE_PISTOL) && !(GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO))
		{
			if (client->sess.playerType != PC_COVERTOPS)
			{
				return;
			}

			client->pmext.silencedSideArm = 1;
		}

		COM_BitSet(client->ps.weapons, GetWeaponTableData(weapon)->weapAlts);

#ifdef FEATURE_OMNIBOT
		Bot_Event_AddWeapon(client->ps.clientNum, Bot_WeaponGameToBot(GetWeaponTableData(weapon)->weapAlts));
#endif
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_AddWeapon(client->ps.clientNum, Bot_WeaponGameToBot(weapon));
#endif
}

/**
 * @brief SetWolfSpawnWeapons
 * @param[in,out] client
 */
void SetWolfSpawnWeapons(gclient_t *client)
{
	int              pc   = client->sess.playerType;
	int              team = client->sess.sessionTeam;
	int              i;
	bg_weaponclass_t *weaponClassInfo;
	bg_playerclass_t *classInfo;

	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_ResetWeapons(client->ps.clientNum);
#endif

	classInfo = BG_GetPlayerClassInfo(team, pc);

	// Communicate it to cgame
	client->ps.stats[STAT_PLAYER_CLASS] = pc;
	client->ps.teamNum                  = team;

	// zero out all ammo counts
	Com_Memset(client->ps.ammo, 0, MAX_WEAPONS * sizeof(int));
	Com_Memset(client->ps.ammoclip, 0, MAX_WEAPONS * sizeof(int));

	// All players start with a knife (not OR-ing so that it clears previous weapons)
	client->ps.weapons[0] = 0;
	client->ps.weapons[1] = 0;

	client->ps.weaponstate = WEAPON_READY;

	//
	// knife
	//
	weaponClassInfo = &classInfo->classKnifeWeapon;
	AddWeaponToPlayer(client, weaponClassInfo->weapon, weaponClassInfo->startingAmmo, weaponClassInfo->startingClip, qtrue);

	//
	// grenade
	//
	weaponClassInfo = &classInfo->classGrenadeWeapon;
	AddWeaponToPlayer(client, weaponClassInfo->weapon, weaponClassInfo->startingAmmo, weaponClassInfo->startingClip, qfalse);

	//
	// primary weapon
	//
	weaponClassInfo = &classInfo->classPrimaryWeapons[0]; // default primary weapon

	// ensure weapon is valid
	if (!IS_VALID_WEAPON(client->sess.playerWeapon))
	{
		client->sess.playerWeapon = weaponClassInfo->weapon;
	}

	// parse available primary weapons and check is valid for current class
	for (i = 0; i < MAX_WEAPS_PER_CLASS && classInfo->classPrimaryWeapons[i].weapon; i++)
	{
		if (client->sess.skill[classInfo->classPrimaryWeapons[i].skill] >= classInfo->classPrimaryWeapons[i].minSkillLevel)
		{
			if (classInfo->classPrimaryWeapons[i].weapon == client->sess.playerWeapon)
			{
				weaponClassInfo = &classInfo->classPrimaryWeapons[i];
				break;
			}
		}
	}

	// add primary weapon (set to current weapon)
	AddWeaponToPlayer(client, weaponClassInfo->weapon, weaponClassInfo->startingAmmo, weaponClassInfo->startingClip, qtrue);

	//
	// secondary weapon
	//
	weaponClassInfo = &classInfo->classSecondaryWeapons[0];   // default secondary weapon

	// ensure weapon is valid
	if (!IS_VALID_WEAPON(client->sess.playerWeapon2))
	{
		client->sess.playerWeapon2 = weaponClassInfo->weapon;
	}

	// parse available secondary weapons and check is valid for current class
	for (i = 0; i < MAX_WEAPS_PER_CLASS && classInfo->classSecondaryWeapons[i].weapon; i++)
	{
		if (client->sess.skill[classInfo->classSecondaryWeapons[i].skill] >= classInfo->classSecondaryWeapons[i].minSkillLevel)
		{
			if (classInfo->classSecondaryWeapons[i].weapon == client->sess.playerWeapon2)
			{
				weaponClassInfo = &classInfo->classSecondaryWeapons[i];
				break;
			}
		}
	}

	// add secondary weapon
	AddWeaponToPlayer(client, weaponClassInfo->weapon, weaponClassInfo->startingAmmo, weaponClassInfo->startingClip, qfalse);

	//
	// special weapons and items
	//
	for (i = 0; i < MAX_WEAPS_PER_CLASS && classInfo->classMiscWeapons[i].weapon; i++)
	{
		weaponClassInfo = &classInfo->classMiscWeapons[i];

		if (client->sess.skill[classInfo->classMiscWeapons[i].skill] >= classInfo->classMiscWeapons[i].minSkillLevel)
		{
			// special check for riflenade, we need the launcher to use it
			if (GetWeaponTableData(weaponClassInfo->weapon)->type & WEAPON_TYPE_RIFLENADE)
			{
				if (!COM_BitCheck(client->ps.weapons, GetWeaponTableData(weaponClassInfo->weapon)->weapAlts))
				{
					continue;
				}
			}

			// add each
			AddWeaponToPlayer(client, weaponClassInfo->weapon, weaponClassInfo->startingAmmo, weaponClassInfo->startingClip, qfalse);
		}
	}
}

/**
 * @brief G_CountTeamMedics
 * @param[in] team
 * @param[in] alivecheck
 * @return
 */
int G_CountTeamMedics(team_t team, qboolean alivecheck)
{
	int numMedics = 0;
	int i, j;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (level.clients[j].sess.sessionTeam != team)
		{
			continue;
		}

		if (level.clients[j].sess.playerType != PC_MEDIC)
		{
			continue;
		}

		if (alivecheck)
		{
			if (g_entities[j].health <= 0)
			{
				continue;
			}

			if (level.clients[j].ps.pm_type == PM_DEAD || (level.clients[j].ps.pm_flags & PMF_LIMBO))
			{
				continue;
			}
		}

		numMedics++;
	}

	return numMedics;
}

/**
 * @brief AddMedicTeamBonus
 * @param[in,out] client
 */
void AddMedicTeamBonus(gclient_t *client)
{
	// compute health mod
	client->pers.maxHealth = 100 + 10 * G_CountTeamMedics(client->sess.sessionTeam, qfalse);

	if (client->pers.maxHealth > 125)
	{
		client->pers.maxHealth = 125;
	}

	if (client->sess.skill[SK_BATTLE_SENSE] >= 3)
	{
		client->pers.maxHealth += 15;
	}

	if (client->sess.playerType == PC_MEDIC)
	{
		client->pers.maxHealth *= 1.12;
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
}

/**
 * @brief G_CountTeamFieldops
 * @param[in] team
 * @param[in] alivecheck
 * @return
 */
int G_CountTeamFieldops(team_t team)
{
	int numFieldops = 0;
	int i, j;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		j = level.sortedClients[i];

		if (level.clients[j].sess.sessionTeam != team)
		{
			continue;
		}

		if (level.clients[j].sess.playerType != PC_FIELDOPS)
		{
			continue;
		}

		numFieldops++;
	}

	return numFieldops;
}

/**
 * @brief ClientCleanName
 * @param[in] in
 * @param[out] out
 * @param[in] outSize
 */
static void ClientCleanName(const char *in, char *out, size_t outSize)
{
	size_t len          = 0;
	int    colorlessLen = 0;
	char   ch;
	char   *p;
	int    spaces = 0;

	// save room for trailing null byte
	outSize--;
	p  = out;
	*p = 0;

	while (1)
	{
		ch = *in++;
		if (!ch)
		{
			break;
		}

		// don't allow leading spaces
		if (!*p && ch == ' ')
		{
			continue;
		}

		// check colors
		if (ch == Q_COLOR_ESCAPE)
		{
			// solo trailing carat is not a color prefix
			if (!*in)
			{
				break;
			}

			// make sure room in dest for both chars
			if (len > outSize - 2)
			{
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len   += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if (ch == ' ')
		{
			spaces++;
			if (spaces > 3)
			{
				continue;
			}
		}
		else
		{
			spaces = 0;
		}

		if (len > outSize - 1)
		{
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if (*p == 0 || colorlessLen == 0)
	{
		Q_strncpyz(p, "UnnamedPlayer", outSize);
	}
}

/**
 * @brief GetParsedIP
 * @param[in] ipadd
 * @return
 *
 * @todo FIXME: IP6
 *
 * @note Courtesy of Dens
 */
const char *GetParsedIP(const char *ipadd)
{
	// code by Dan Pop, http://bytes.com/forum/thread212174.html
	unsigned      b1, b2, b3, b4, port = 0;
	unsigned char c;
	int           rc;
	static char   ipge[20];

	if (!Q_strncmp(ipadd, "localhost", 9))
	{
		return "localhost";
	}

	rc = sscanf(ipadd, "%3u.%3u.%3u.%3u:%u%c", &b1, &b2, &b3, &b4, &port, &c);
	if (rc < 4 || rc > 5)
	{
		return NULL;
	}
	if ((b1 | b2 | b3 | b4) > 255 || port > 65535)
	{
		return NULL;
	}
	if (strspn(ipadd, "0123456789.:") < strlen(ipadd))
	{
		return NULL;
	}

	Com_sprintf(ipge, sizeof(ipge), "%u.%u.%u.%u", b1, b2, b3, b4);
	return ipge;
}

/**
 * @brief Based on userinfocheck.lua and combinedfixes.lua
 * @param clientNum - unused
 * @param[in] userinfo
 * @return
 *
 * @note FIXME: IP6
 */
char *CheckUserinfo(int clientNum, char *userinfo)
{
	char         *value;
	unsigned int length = strlen(userinfo);
	int          i, slashCount = 0, count = 0;

	if (length < 1)
	{
		return "Userinfo too short";
	}

	// 44 is a bit random now: MAX_INFO_STRING - 44 = 980. The LUA script
	// uses 980, but I don't know if there is a specific reason for that
	// number. Userinfo should never get this big anyway, unless someone is
	// trying to force the engine to truncate it, so that the real IP is lost
	if (length > MAX_INFO_STRING - 44)
	{
		return "Userinfo too long.";
	}

	// userinfo always has to have a leading slash
	if (userinfo[0] != '\\')
	{
		return "Missing leading slash in userinfo.";
	}
	// the engine always adds ip\ip:port at the end, so there will never
	// be a trailing slash
	if (userinfo[length - 1] == '\\')
	{
		return "Trailing slash in userinfo.";
	}

	for (i = 0; userinfo[i]; ++i)
	{
		if (userinfo[i] == '\\')
		{
			slashCount++;
		}
	}
	if (slashCount % 2 != 0)
	{
		return "Bad number of slashes in userinfo.";
	}

	// make sure there is only one ip, cl_guid, name and cl_punkbuster field
	if (length > 4)
	{
		for (i = 0; userinfo[i + 3]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'i' &&
			    userinfo[i + 2] == 'p' && userinfo[i + 3] == '\\')
			{
				count++;
			}
		}
	}
	if (count == 0)
	{
		return "Missing IP in userinfo.";
	}
	else if (count > 1)
	{
		return "Too many IP fields in userinfo.";
	}
	else
	{
		if (GetParsedIP(Info_ValueForKey(userinfo, "ip")) == NULL)
		{
			return "Malformed IP in userinfo.";
		}
	}
	count = 0;

	if (length > 9)
	{
		for (i = 0; userinfo[i + 8]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'c' &&
			    userinfo[i + 2] == 'l' && userinfo[i + 3] == '_' &&
			    userinfo[i + 4] == 'g' && userinfo[i + 5] == 'u' &&
			    userinfo[i + 6] == 'i' && userinfo[i + 7] == 'd' &&
			    userinfo[i + 8] == '\\')
			{
				count++;
			}
		}
	}
	if (count > 1)
	{
		return "Too many cl_guid fields in userinfo.";
	}
	count = 0;

	if (length > 6)
	{
		for (i = 0; userinfo[i + 5]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'n' &&
			    userinfo[i + 2] == 'a' && userinfo[i + 3] == 'm' &&
			    userinfo[i + 4] == 'e' && userinfo[i + 5] == '\\')
			{
				count++;
			}
		}
	}
	if (count == 0)
	{
		// if an empty name is entered, the userinfo will not contain a /name key/value at all..
		// FIXME: replace ?
		return "Missing name field in userinfo.";
	}
	else if (count > 1)
	{
		return "Too many name fields in userinfo.";
	}
	count = 0;

	if (length > 15)
	{
		for (i = 0; userinfo[i + 14]; ++i)
		{
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'c' &&
			    userinfo[i + 2] == 'l' && userinfo[i + 3] == '_' &&
			    userinfo[i + 4] == 'p' && userinfo[i + 5] == 'u' &&
			    userinfo[i + 6] == 'n' && userinfo[i + 7] == 'k' &&
			    userinfo[i + 8] == 'b' && userinfo[i + 9] == 'u' &&
			    userinfo[i + 10] == 's' && userinfo[i + 11] == 't' &&
			    userinfo[i + 12] == 'e' && userinfo[i + 13] == 'r' &&
			    userinfo[i + 14] == '\\')
			{
				count++;
			}
		}
	}
	if (count > 1)
	{
		return "Too many cl_punkbuster fields in userinfo.";
	}

	value = Info_ValueForKey(userinfo, "rate");
	if (value == NULL || value[0] == '\0')
	{
		return "Wrong rate field in userinfo.";
	}

	return 0;
}

/**
 * @brief Called from ClientConnect when the player first connects and
 * directly by the server system when the player updates a userinfo variable.
 *
 * The game can override any of the settings and call trap_SetUserinfo
 * if desired.
 *
 * @param[in] clientNum
 */
void ClientUserinfoChanged(int clientNum)
{
	gentity_t  *ent    = g_entities + clientNum;
	gclient_t  *client = ent->client;
	int        i;
	const char *userinfo_ptr                 = NULL;
	char       cs_key[MAX_STRING_CHARS]      = "";
	char       cs_value[MAX_STRING_CHARS]    = "";
	char       cs_cg_uinfo[MAX_STRING_CHARS] = "";
	char       cs_skill[MAX_STRING_CHARS]    = "";
	char       *reason;
	char       *s;
	char       cs_name[MAX_NETNAME] = "";
	char       oldname[MAX_STRING_CHARS];
	char       userinfo[MAX_INFO_STRING];
	char       skillStr[16] = "";
	char       medalStr[16] = "";

	client->ps.clientNum = clientNum;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	if (!(ent->r.svFlags & SVF_BOT))
	{
		reason = CheckUserinfo(clientNum, userinfo);
		if (reason)
		{
			G_Printf("ClientUserinfoChanged: CheckUserinfo: client %d: %s\n", clientNum, reason);
			trap_DropClient(clientNum, va("^1%s", "Bad userinfo."), 99999);
			return;
		}
	}

	// Cache config string values used in ClientUserinfoChanged
	// Every time we call Info_ValueForKey we search through the string from the start....
	// let's grab the values we need in just one pass... key matching is fast because
	// we will use our minimal perfect hashing function.
	userinfo_ptr = userinfo;

	while (1)
	{
		g_StringToken_t token;
		// Get next key/value pair
		if (qfalse == Info_NextPair(&userinfo_ptr, cs_key, cs_value))
		{
			// This would only happen if the end user is trying to manually modify the user info string
			G_Printf("ClientUserinfoChanged: client %d hacking clientinfo, empty key found!\n", clientNum);

			if (!(ent->r.svFlags & SVF_BOT))
			{
				trap_DropClient(clientNum, "Bad userinfo.", 0);
			}
			return;
		}
		// Empty string for key and we exit...
		if (cs_key[0] == 0)
		{
			break;
		}

		token = G_GetTokenForString(cs_key);
		switch (token)
		{
		case TOK_ip:
		{
			if (!(ent->r.svFlags & SVF_BOT) && CompareIPNoPort(client->pers.client_ip, cs_value) == qfalse)
			{
				// They're trying to hack their ip address....
				G_Printf("ClientUserinfoChanged: client %d hacking ip, old=%s, new=%s\n", clientNum, client->pers.client_ip, cs_value);
				trap_DropClient(clientNum, "Bad userinfo.", 0);
				return;
			}
			Q_strncpyz(client->pers.client_ip, cs_value, MAX_IP4_LENGTH);
			break;
		}
		case TOK_cg_uinfo:
			Q_strncpyz(cs_cg_uinfo, cs_value, MAX_STRING_CHARS);
			break;
		/*
		            case TOK_pmove_fixed:
		                if ( cs_value[0] )
		                    client->pers.pmoveFixed = Q_atoi(cs_value);
		                else
		                    client->pers.pmoveFixed = 0;
		                break;
		            case TOK_pmove_msec:
		                if ( cs_value[0] )
		                    client->pers.pmoveMsec = Q_atoi(cs_value);
		                else
		                    client->pers.pmoveMsec = 8;
		                // paranoia
		                if (  client->pers.pmoveMsec > 33 )
		                    client->pers.pmoveMsec = 33;
		                if ( client->pers.pmoveMsec < 3 )
		                    client->pers.pmoveMsec = 3;
		                break;
		*/
		case TOK_name:
			// see also MAX_NAME_LENGTH
			if (strlen(cs_value) >= MAX_NETNAME)
			{
				// They're trying long names
				G_Printf("ClientUserinfoChanged: client %d kicked for long name in config string old=%s, new=%s\n", clientNum, client->pers.cl_guid, cs_value);
				trap_DropClient(clientNum, "Name too long. Plase change your name.", 0);
				return;
			}
			if (!g_extendedNames.integer)
			{
				unsigned int i;

				// Avoid ext. ASCII chars in the CS
				for (i = 0; i < strlen(cs_value); ++i)
				{
					// extended ASCII chars have values between -128 and 0 (signed char) and the ASCII code flags are 0-31
					if (cs_value[i] < 32)
					{
						G_Printf("ClientUserinfoChanged: client %d kicked for extended ASCII characters name in config string old=%s, new=%s\n", clientNum, client->pers.cl_guid, cs_value);
						trap_DropClient(clientNum, "Server does not allow extended ASCII characters. Please change your name.", 0);
						return;
					}
				}
			}
			Q_strncpyz(cs_name, cs_value, MAX_NETNAME);
			break;
		case TOK_cl_guid:
			if (!(ent->r.svFlags & SVF_BOT) && strcmp(client->pers.cl_guid, cs_value))
			{
				// They're trying to hack their guid...
				G_Printf("ClientUserinfoChanged: client %d hacking cl_guid, old=%s, new=%s\n", clientNum, client->pers.cl_guid, cs_value);
				trap_DropClient(clientNum, "Bad userinfo.", 0);
				return;
			}
			Q_strncpyz(client->pers.cl_guid, cs_value, MAX_GUID_LENGTH + 1);
			break;
		case TOK_skill:
			Q_strncpyz(cs_skill, cs_value, MAX_STRING_CHARS);
			break;
		default:
			continue;
		}
	}

	// overwrite empty names with a default name, but do not kick those players ...
	// (player did delete the profile or profile can't be read)
	if (strlen(cs_name) == 0)
	{
		Q_strncpyz(cs_name, va("Target #%i", clientNum), 15);
		Info_SetValueForKey(userinfo, "name", cs_name);
		trap_SetUserinfo(clientNum, userinfo);
		CP("cp \"You cannot assign an empty playername! Your name has been reset.\"");
		G_LogPrintf("ClientUserinfoChanged: %i User with empty name. (Changed to: \"Target #%i\")\n", clientNum, clientNum);
		G_DPrintf("ClientUserinfoChanged: %i User with empty name. (Changed to: \"Target #%i\")\n", clientNum, clientNum);
	}

	client->medals = 0;
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		client->medals += client->sess.medals[i];
	}

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		Q_strncpyz(userinfo, "\\name\\badinfo", sizeof(userinfo));
		G_Printf("ClientUserinfoChanged: CheckUserinfo: client %d: Invalid userinfo\n", clientNum);
		trap_DropClient(clientNum, "Invalid userinfo", 300);
		return;
	}

#ifndef DEBUG_STATS
	if (g_developer.integer || *g_log.string || g_dedicated.integer)
#endif
	{
		G_Printf("Userinfo: %s\n", userinfo);
	}

	if (g_protect.integer & G_PROTECT_LOCALHOST_REF)
	{
		if (ent->r.svFlags & SVF_BOT)
		{
			client->pers.localClient = qtrue; // don't kick or vote against bots ... but don't set referee!
		}
	}
	else // no protection, check for local client and set ref (for LAN/listen server games)
	{
		if (!strcmp(client->pers.client_ip, "localhost"))
		{
			client->pers.localClient = qtrue;
			level.fLocalHost         = qtrue;
			client->sess.referee     = RL_REFEREE;
		}
	}

	// added for zinx etpro antiwarp
	client->pers.pmoveMsec = pmove_msec.integer;

	// extra client info settings
	//		 FIXME: move other userinfo flag settings in here
	if (ent->r.svFlags & SVF_BOT)
	{
		client->pers.autoActivate      = PICKUP_TOUCH;
		client->pers.bAutoReloadAux    = qtrue;
		client->pmext.bAutoReload      = qtrue;
		client->pers.predictItemPickup = qfalse;
		client->pers.pmoveFixed        = qfalse;
		client->pers.pmoveMsec         = 8;
	}
	else
	{
		if (cs_cg_uinfo[0])
		{
			sscanf(cs_cg_uinfo, "%u %u %u",
			       &client->pers.clientFlags,
			       &client->pers.clientTimeNudge,
			       &client->pers.clientMaxPackets);
		}
		else
		{
			// cg_uinfo was empty and useless.
			// Let us fill these values in with something useful,
			// if there not present to prevent auto reload
			// from failing. - forty
			if (!client->pers.clientFlags)
			{
				client->pers.clientFlags = 13;
			}
			if (!client->pers.clientTimeNudge)
			{
				client->pers.clientTimeNudge = 0;
			}
			if (!client->pers.clientMaxPackets)
			{
				client->pers.clientMaxPackets = 125;
			}
		}

		client->pers.autoActivate      = (client->pers.clientFlags & CGF_AUTOACTIVATE) ? PICKUP_TOUCH : PICKUP_ACTIVATE;
		client->pers.predictItemPickup = ((client->pers.clientFlags & CGF_PREDICTITEMS) != 0);

		if (client->pers.clientFlags & CGF_AUTORELOAD)
		{
			client->pers.bAutoReloadAux = qtrue;
			client->pmext.bAutoReload   = qtrue;
		}
		else
		{
			client->pers.bAutoReloadAux = qfalse;
			client->pmext.bAutoReload   = qfalse;
		}
	}

	// set name
	Q_strncpyz(oldname, client->pers.netname, sizeof(oldname));
	ClientCleanName(cs_name, client->pers.netname, sizeof(client->pers.netname));

	if (client->pers.connected == CON_CONNECTED)
	{
		if (strcmp(oldname, client->pers.netname))
		{
			trap_SendServerCommand(-1, va("print \"[lof]" S_COLOR_WHITE "%s" S_COLOR_WHITE " [lon]renamed to[lof] %s\n\"", oldname,
			                              client->pers.netname));
		}
	}

	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		Q_strcat(skillStr, sizeof(skillStr), va("%i", client->sess.skill[i]));
		Q_strcat(medalStr, sizeof(medalStr), va("%i", client->sess.medals[i]));
		// FIXME: Gordon: wont this break if medals > 9 arnout?
		// Medal count is tied to skill count :()
		// er, it's based on >> skill per map, so for a huuuuuuge campaign it could break...
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// To communicate it to cgame
	client->ps.stats[STAT_PLAYER_CLASS] = client->sess.playerType;
	// Gordon: Not needed any more as it's in clientinfo?

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
#ifdef FEATURE_PRESTIGE
	s = va("n\\%s\\t\\%i\\c\\%i\\lc\\%i\\r\\%i\\p\\%i\\m\\%s\\s\\%s\\dn\\%i\\w\\%i\\lw\\%i\\sw\\%i\\lsw\\%i\\mu\\%i\\ref\\%i\\sc\\%i\\u\\%u",
#else
	s = va("n\\%s\\t\\%i\\c\\%i\\lc\\%i\\r\\%i\\m\\%s\\s\\%s\\dn\\%i\\w\\%i\\lw\\%i\\sw\\%i\\lsw\\%i\\mu\\%i\\ref\\%i\\sc\\%i\\u\\%u",
#endif
	       client->pers.netname,
	       client->sess.sessionTeam,
	       client->sess.playerType,
	       client->sess.latchPlayerType,
	       client->sess.rank,
#ifdef FEATURE_PRESTIGE
	       client->sess.prestige,
#endif
	       medalStr,
	       skillStr,
	       client->disguiseClientNum,
	       client->sess.playerWeapon,
	       client->sess.latchPlayerWeapon,
	       client->sess.playerWeapon2,
	       client->sess.latchPlayerWeapon2,
	       client->sess.muted ? 1 : 0,
	       client->sess.referee,
	       client->sess.shoutcaster,
	       client->sess.uci
	       );

	trap_GetConfigstring(CS_PLAYERS + clientNum, oldname, sizeof(oldname));
	trap_SetConfigstring(CS_PLAYERS + clientNum, s);

	if (!Q_stricmp(oldname, s)) // not changed
	{
		return;
	}

#ifdef FEATURE_LUA
	// *LUA* API callbacks
	// This only gets called when the ClientUserinfo is changed, replicating ETPro's behaviour.
	G_LuaHook_ClientUserinfoChanged(clientNum);
#endif

	G_LogPrintf("ClientUserinfoChanged: %i %s\n", clientNum, s);
	G_DPrintf("ClientUserinfoChanged: %i :: %s\n", clientNum, s);
}

extern const char *country_name[MAX_COUNTRY_NUM];

/**
 * @brief Called when a player begins connecting to the server.
 * Called again for every map change or tournement restart.
 *
 * @details The session information will be valid after exit.
 *
 * Return NULL if the client should be allowed, otherwise return
 * a string with the reason for denial.
 *
 * Otherwise, the client will be sent the current gamestate
 * and will eventually get to ClientBegin.
 *
 * @param[in] clientNum
 * @param[in] firstTime will be qtrue the very first time a client connects to the server machine, but qfalse on map changes and tournement restarts.
 * @param[in] isBot
 *
 * @return NULL if the client should be allowed, otherwise return
 * a string with the reason for denial.
 */
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	gclient_t  *client;
	gentity_t  *ent = &g_entities[clientNum];
	const char *userinfo_ptr;
	char       userinfo[MAX_INFO_STRING];
	char       cs_key[MAX_STRING_CHARS]      = "";
	char       cs_value[MAX_STRING_CHARS]    = "";
	char       cs_ip[MAX_STRING_CHARS]       = "";
	char       cs_password[MAX_STRING_CHARS] = "";
	char       cs_name[MAX_NETNAME]          = "";
	char       cs_guid[MAX_GUID_LENGTH + 1]  = "";
	//char       cs_rate[MAX_STRING_CHARS]     = "";
#ifdef FEATURE_LUA
	char reason[MAX_STRING_CHARS] = "";
#endif
	qboolean allowGeoIP = qtrue;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// grab the values we need in just one pass
	// Use our minimal perfect hash generated code to give us the
	// result token in optimal time
	userinfo_ptr = userinfo;
	while (1)
	{
		g_StringToken_t token;
		// Get next key/value pair
		Info_NextPair(&userinfo_ptr, cs_key, cs_value);
		// Empty string for key and we exit...
		if (cs_key[0] == 0)
		{
			break;
		}
		if (!Q_stricmp(cs_key, "cg_allowGeoIP") && cs_value[0])
		{
			allowGeoIP = cs_value[0] >= '1' ? qtrue : qfalse;
			continue;
		}

		token = G_GetTokenForString(cs_key);
		switch (token)
		{
		case TOK_ip:
			Q_strncpyz(cs_ip, cs_value, MAX_STRING_CHARS);
			break;
		case TOK_name:
			Q_strncpyz(cs_name, cs_value, MAX_NETNAME);
			break;
		case TOK_cl_guid:
			Q_strncpyz(cs_guid, cs_value, MAX_GUID_LENGTH + 1);
			break;
		case TOK_password:
			Q_strncpyz(cs_password, cs_value, MAX_STRING_CHARS);
			break;
		//case TOK_rate:
		//	Q_strncpyz(cs_rate, cs_value, MAX_STRING_CHARS);
		//	break;
		default:
			continue;
		}
	}

	// IP filtering
	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
	// check to see if they are on the banned IP list
	if (G_FilterIPBanPacket(cs_ip))
	{
		return "You are banned from this server.";
	}

	// don't permit long names ... - see also MAX_NETNAME
	if (strlen(cs_name) >= MAX_NAME_LENGTH)
	{
		return "Bad name: Name too long. Please change your name.";
	}

	if (!g_extendedNames.integer)
	{
		unsigned int i;

		// Avoid ext. ASCII chars in the CS
		for (i = 0; i < strlen(cs_name); ++i)
		{
			// extended ASCII chars have values between -128 and 0 (signed char) and the ASCII code flags are 0-31
			if (cs_name[i] < 32)
			{
				return "Bad name: Extended ASCII characters. Please change your name.";
			}
		}
	}

	// check for max lives enforcement ban
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if (g_enforcemaxlives.integer && (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0))
		{
			if (g_protect.integer & G_PROTECT_MAX_LIVES_BAN_GUID)
			{
				if (G_FilterMaxLivesPacket(cs_guid))
				{
					return "Max Lives Enforcement Temp Ban. You will be able to reconnect when the next round starts. This ban is enforced to ensure you don't reconnect to get additional lives.";
				}
			}
			else
			{
				// this isn't really needed, oh well.
				if (G_FilterMaxLivesIPPacket(cs_ip))
				{
					return "Max Lives Enforcement Temp Ban. You will be able to reconnect when the next round starts. This ban is enforced to ensure you don't reconnect to get additional lives.";
				}
			}
		}
	}

	if (!isBot)
	{
		// we don't check password for bots and local client
		// NOTE: local client <-> "ip" "localhost"
		//   this means this client is not running in our current process
		if ((strcmp(cs_ip, "localhost") != 0))
		{
			// check for a password
			if (g_password.string[0] && Q_stricmp(g_password.string, "none") && strcmp(g_password.string, cs_password) != 0)
			{
				if (!sv_privatepassword.string[0] || strcmp(sv_privatepassword.string, cs_password))
				{
					return "Invalid password";
				}
			}
		}
	}

	// porting q3f flag bug fix
	// If a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	// see ZOMBIE state
	if (ent->inuse)
	{
		G_LogPrintf("Forcing disconnect on active client: %i\n", (int)(ent - g_entities));
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect(ent - g_entities);
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client      = ent->client;

	Com_Memset(client, 0, sizeof(*client));

	client->pers.connected   = CON_CONNECTING;
	client->pers.connectTime = level.time;

	client->disguiseClientNum = -1;

	// Set the client ip and guid
	Q_strncpyz(client->pers.client_ip, cs_ip, MAX_IP4_LENGTH);
	Q_strncpyz(client->pers.cl_guid, cs_guid, MAX_GUID_LENGTH + 1);

	if (firstTime)
	{
		client->pers.initialSpawn = qtrue;

		// read or initialize the session data
		G_InitSessionData(client, userinfo);
		client->pers.enterTime            = level.time;
		client->ps.persistant[PERS_SCORE] = 0;
	}
	else
	{
		G_ReadSessionData(client);
	}

	// GeoIP
	// comment out the following check to hide the country flag for bots
	if (gidb != NULL /*&& !isBot*/)
	{
		//value = Info_ValueForKey (userinfo, "ip");
		if (!strcmp(cs_ip, "localhost"))
		{
			if (isBot)
			{
				client->sess.uci = 0; // bots
			}
			else
			{
				client->sess.uci = 246; // localhost players
			}
		}
		else
		{
			unsigned long ip = GeoIP_addr_to_num(cs_ip);

			// 10.0.0.0/8			[RFC1918]
			// 172.16.0.0/12		[RFC1918]
			// 192.168.0.0/16		[RFC1918]
			// 169.254.0.0/16		[RFC3330] we need this ?
			if (((ip & 0xFF000000) == 0x0A000000) ||
			    ((ip & 0xFFF00000) == 0xAC100000) ||
			    ((ip & 0xFFFF0000) == 0xC0A80000) ||
			    (ip  == 0x7F000001))                    // recognise also 127.0.0.1
			{
				client->sess.uci = 246;
			}
			else if (allowGeoIP)
			{
				unsigned int ret = GeoIP_seek_record(gidb, ip);

				if (ret > 0)
				{
					client->sess.uci = ret;
				}
				else
				{
					client->sess.uci = 246;
					G_LogPrintf("GeoIP: This IP:%s cannot be located\n", cs_ip);
				}
			}
			else
			{
				client->sess.uci = 246;
			}
		}
	}
	else
	{
		client->sess.uci = 255; //Don't draw anything if DB error
	} // end GeoIP

	if (g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		if (g_campaigns[level.currentCampaign].current == 0 || level.newCampaign)
		{
			client->pers.enterTime = level.time;
		}
	}
	else
	{
		client->pers.enterTime = level.time;
	}

	if (isBot)
	{
		ent->s.number   = clientNum;
		ent->r.svFlags |= SVF_BOT;
		ent->inuse      = qtrue;

	}
	else if (firstTime)
	{
		// force into spectator
		client->sess.sessionTeam     = TEAM_SPECTATOR;
		client->sess.spectatorState  = SPECTATOR_FREE;
		client->sess.spectatorClient = 0;

		// unlink the entity - just in case they were already connected
		trap_UnlinkEntity(ent);
	}

#ifdef FEATURE_LUA
	// LUA API callbacks (check with Lua scripts)
	if (G_LuaHook_ClientConnect(clientNum, firstTime, isBot, reason))
	{
		if (!isBot && !(ent->r.svFlags & SVF_BOT))
		{
			return va("You are excluded from this server. %s\n", reason);
		}
	}
#endif

	// get and distribute relevent paramters
	G_LogPrintf("ClientConnect: %i\n", clientNum);

#ifdef FEATURE_OMNIBOT
	Bot_Event_ClientConnected(clientNum, isBot);
#endif

	G_UpdateCharacter(client);

#ifdef FEATURE_RATING
	if (g_skillRating.integer)
	{
		G_SkillRatingGetClientRating(client);
		G_CalcRank(client);
	}
#endif

#ifdef FEATURE_PRESTIGE
	if (g_prestige.integer && g_gametype.integer != GT_WOLF_CAMPAIGN && g_gametype.integer != GT_WOLF_STOPWATCH && g_gametype.integer != GT_WOLF_LMS)
	{
		int i;

		G_GetClientPrestige(client);

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			G_SetPlayerSkill(client, i);
		}
	}
#endif

	if (firstTime && g_xpSaver.integer && g_gametype.integer == GT_WOLF_CAMPAIGN)
	{
		int i;
		G_XPSaver_Load(client);

		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			G_SetPlayerSkill(client, i);
		}
	}

	ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	// disabled for bots - see join message ... make cvar ?
	if (firstTime && !(ent->r.svFlags & SVF_BOT))
	{
		if ((g_countryflags.integer & CF_CONNECT) && client->sess.uci > 0 && client->sess.uci < MAX_COUNTRY_NUM && allowGeoIP)
		{
			trap_SendServerCommand(-1, va("cpm \"" S_COLOR_WHITE "%s" S_COLOR_WHITE " connected from %s\n\"", client->pers.netname, country_name[client->sess.uci]));
		}
		else
		{
			trap_SendServerCommand(-1, va("cpm \"" S_COLOR_WHITE "%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));
		}
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

/**
 * @brief Scaling for late-joiners of maxlives based on current game time
 * @param cl - unused
 * @param[in] maxRespawns
 * @return
 */
int G_ComputeMaxLives(gclient_t *cl, int maxRespawns)
{
	float scaled;
	int   val;

	// don't scale of the timelimit is 0
	if (g_timelimit.value == 0.0f) // map end
	{
		return maxRespawns - 1;
	}

	if (g_gamestate.integer != GS_PLAYING) // warmup
	{
		return maxRespawns - 1;
	}

	scaled = (float)(maxRespawns - 1) * (1.0f - ((float)(level.time - level.startTime) / (g_timelimit.value * 60000.0f)));
	val    = (int)scaled;

	val += ((scaled - (float)val) < 0.5f) ? 0 : 1;

	return val;
}

/**
 * @brief Called when a client has finished connecting, and is ready
 * to be placed into the level.  This will happen every level load,
 * and on transition between teams, but doesn't happen on respawns
 *
 * @param[in] clientNum
 */
void ClientBegin(int clientNum)
{
	gentity_t *ent    = g_entities + clientNum;
	gclient_t *client = level.clients + clientNum;
	int       flags;
	int       spawn_count, lives_left;
	int       stat_xp, score; // restore xp & score

#ifdef FEATURE_LUA
	// call LUA clientBegin only once when player connects
	qboolean firsttime = qfalse;
	if (client->pers.connected == CON_CONNECTING)
	{
		firsttime = qtrue;
	}
#endif

	if (ent->r.linked)
	{
		trap_UnlinkEntity(ent);
	}

	G_InitGentity(ent);
	ent->touch  = 0;
	ent->pain   = 0;
	ent->client = client;

	client->pers.connected       = CON_CONNECTED;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	// Also save PERS_SPAWN_COUNT, so that CG_Respawn happens
	spawn_count = client->ps.persistant[PERS_SPAWN_COUNT];

	if (client->ps.persistant[PERS_RESPAWNS_LEFT] > 0)
	{
		lives_left = client->ps.persistant[PERS_RESPAWNS_LEFT] - 1;
	}
	else
	{
		lives_left = client->ps.persistant[PERS_RESPAWNS_LEFT];
	}
	flags = client->ps.eFlags;

	// restore xp & score
	stat_xp = ent->client->ps.stats[STAT_XP];
	score   = ent->client->ps.persistant[PERS_SCORE];

	Com_Memset(&client->ps, 0, sizeof(client->ps));

	ent->client->ps.persistant[PERS_SCORE] = score;

	if (ent->client->sess.spectatorState == SPECTATOR_FREE) // restore xp
	{
		ent->client->ps.stats[STAT_XP] = stat_xp;
	}

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		client->ps.pm_type = PM_INTERMISSION;
	}

	client->ps.eFlags                         = flags;
	client->ps.persistant[PERS_SPAWN_COUNT]   = spawn_count;
	client->ps.persistant[PERS_RESPAWNS_LEFT] = lives_left;

	client->pers.complaintClient      = -1;
	client->pers.complaintEndTime     = -1;
	client->pers.lastkilled_client    = -1;
	client->pers.lastammo_client      = -1;
	client->pers.lasthealth_client    = -1;
	client->pers.lastrevive_client    = -1;
	client->pers.lastkiller_client    = -1;
	client->pers.lastteambleed_client = -1;
	client->pers.lastteambleed_dmg    = -1;

	client->pers.savedClassWeaponTime     = -999999;
	client->pers.savedClassWeaponTimeCvop = -999999;
	client->pers.savedClassWeaponTimeEng  = -999999;
	client->pers.savedClassWeaponTimeFop  = -999999;
	client->pers.savedClassWeaponTimeMed  = -999999;

#ifdef FEATURE_OMNIBOT
	// Omni-bot
	client->sess.botSuicide = qfalse; // make sure this is not set
	client->sess.botPush    = (ent->r.svFlags & SVF_BOT) ? qtrue : qfalse;
#endif

	// init objective indicator if already set
	if (level.flagIndicator > 0)
	{
		G_clientFlagIndicator(ent);
	}

	ClientSpawn(ent, qfalse, qtrue, qtrue);

	if (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES)
	{
		client->inactivityTime        = level.time + (g_inactivity.integer ? g_inactivity.integer : 60) * 1000;
		client->inactivitySecondsLeft = (g_inactivity.integer) ? g_inactivity.integer : 60;
	}
	else
	{
		client->inactivityTime        = level.time + (g_spectatorInactivity.integer ? g_spectatorInactivity.integer : 60) * 1000;
		client->inactivitySecondsLeft = (g_spectatorInactivity.integer) ? g_spectatorInactivity.integer : 60;
	}

	// Changed below for team independant maxlives
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if ((client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES))
		{
			if (!client->maxlivescalced)
			{
				if (g_maxlives.integer > 0)
				{
					client->ps.persistant[PERS_RESPAWNS_LEFT] = G_ComputeMaxLives(client, g_maxlives.integer);
				}
				else
				{
					client->ps.persistant[PERS_RESPAWNS_LEFT] = -1;
				}

				if (g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0)
				{
					if (client->sess.sessionTeam == TEAM_AXIS)
					{
						client->ps.persistant[PERS_RESPAWNS_LEFT] = G_ComputeMaxLives(client, g_axismaxlives.integer);
					}
					else if (client->sess.sessionTeam == TEAM_ALLIES)
					{
						client->ps.persistant[PERS_RESPAWNS_LEFT] = G_ComputeMaxLives(client, g_alliedmaxlives.integer);
					}
					else
					{
						client->ps.persistant[PERS_RESPAWNS_LEFT] = -1;
					}
				}

				client->maxlivescalced = qtrue;
			}
			else
			{
				if (g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0)
				{
					if (g_gamestate.integer == GS_PLAYING)
					{
						if (client->sess.sessionTeam == TEAM_AXIS)
						{
							if (client->ps.persistant[PERS_RESPAWNS_LEFT] > g_axismaxlives.integer)
							{
								client->ps.persistant[PERS_RESPAWNS_LEFT] = g_axismaxlives.integer;
							}
						}
						else if (client->sess.sessionTeam == TEAM_ALLIES)
						{
							if (client->ps.persistant[PERS_RESPAWNS_LEFT] > g_alliedmaxlives.integer)
							{
								client->ps.persistant[PERS_RESPAWNS_LEFT] = g_alliedmaxlives.integer;
							}
						}
					}
					else // warmup
					{
						if (client->sess.sessionTeam == TEAM_AXIS)
						{
							client->ps.persistant[PERS_RESPAWNS_LEFT] = g_axismaxlives.integer;
						}
						else if (client->sess.sessionTeam == TEAM_ALLIES)
						{
							client->ps.persistant[PERS_RESPAWNS_LEFT] = g_alliedmaxlives.integer;
						}
					}
				}
			}
		}
	}

	// Start players in limbo mode if they change teams during the match
	if (g_gamestate.integer != GS_INTERMISSION && client->sess.sessionTeam != TEAM_SPECTATOR && (level.time - level.startTime > FRAMETIME * GAME_INIT_FRAMES))
	{
		ent->health     = 0;
		ent->r.contents = CONTENTS_CORPSE;

		client->ps.pm_type            = PM_DEAD;
		client->ps.stats[STAT_HEALTH] = 0;

		if (g_gametype.integer != GT_WOLF_LMS)
		{
			if (g_maxlives.integer > 0)
			{
				client->ps.persistant[PERS_RESPAWNS_LEFT]++;
			}
		}

		limbo(ent, qfalse);
	}

	if (client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_SendServerCommand(-1, va("print \"[lof]" S_COLOR_WHITE "%s" S_COLOR_WHITE " [lon]entered the game\n\"", client->pers.netname));
	}

	G_LogPrintf("ClientBegin: %i\n", clientNum);

	// Check for maxlives enforcement
	if (g_gametype.integer != GT_WOLF_LMS)
	{
		if (g_enforcemaxlives.integer == 1 && (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0))
		{
			char *value;
			char userinfo[MAX_INFO_STRING];

			trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
			value = Info_ValueForKey(userinfo, "cl_guid");
			G_LogPrintf("EnforceMaxLives-GUID: %s\n", value);
			AddMaxLivesGUID(value);

			if (!(g_entities[client->ps.clientNum].r.svFlags & SVF_BOT))
			{
				value = Info_ValueForKey(userinfo, "ip");
				G_LogPrintf("EnforceMaxLives-IP: %s\n", value);
				AddMaxLivesBan(value);
			}
		}
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// No surface determined yet.
	ent->surfaceFlags = 0;

#ifdef FEATURE_MULTIVIEW
	G_smvUpdateClientCSList(ent);
#endif

#ifdef FEATURE_LUA
	// call LUA clientBegin only once
	if (firsttime == qtrue)
	{
		// LUA API callbacks
		G_LuaHook_ClientBegin(clientNum);
	}
#endif
}

#if 0 // not used

#define MAX_SPAWNPOINTFROMLIST_POINTS   16

/**
 * @brief SelectSpawnPointFromList
 * @param[in] list
 * @param[out] spawn_origin
 * @param[out] spawn_angles
 * @return
 *
 * @note Unused
 */
gentity_t *SelectSpawnPointFromList(char *list, vec3_t spawn_origin, vec3_t spawn_angles)
{
	char      *pStr = list, *token;
	gentity_t *spawnPoint = NULL, *trav;
	int       valid[MAX_SPAWNPOINTFROMLIST_POINTS];
	int       numValid = 0;

	Com_Memset(valid, 0, sizeof(valid));

	while ((token = COM_Parse(&pStr)) != NULL && token[0])
	{
		trav = g_entities + level.maxclients;
		while ((trav = G_FindByTargetname(trav, token)) != NULL)
		{
			if (!spawnPoint)
			{
				spawnPoint = trav;
			}
			if (!SpotWouldTelefrag(trav))
			{
				valid[numValid++] = trav->s.number;
				if (numValid >= MAX_SPAWNPOINTFROMLIST_POINTS)
				{
					break;
				}
			}
		}
	}

	if (numValid)
	{
		spawnPoint = &g_entities[valid[rand() % numValid]];

		// Set the origin of where the bot will spawn
		VectorCopy(spawnPoint->r.currentOrigin, spawn_origin);
		spawn_origin[2] += 9;

		// Set the angle we'll spawn in to
		VectorCopy(spawnPoint->s.angles, spawn_angles);
	}

	return spawnPoint;
}

/**
 * @brief G_CheckVersion
 * @param ent
 * @return
 *
 * @note Unused
 */
static char *G_CheckVersion(gentity_t *ent)
{
	// Check cgame version against qagame's one

	char userinfo[MAX_INFO_STRING];
	char *s;

	trap_GetUserinfo(ent->s.number, userinfo, sizeof(userinfo));
	s = Info_ValueForKey(userinfo, "cg_legacyVersion");
	if (!s || strcmp(s, LEGACY_VERSION))
	{
		return(s);
	}
	return NULL;
}
#endif

static qboolean isMortalSelfDamage(gentity_t *ent)
{
	return (
		(ent->enemy && ent->enemy->s.number >= MAX_CLIENTS) // worldkill
		|| (ent->enemy == ent) // selfkill
		|| OnSameTeam(ent->enemy, ent) // teamkill
		);
}

/**
 * @brief Called every time a client is placed fresh in the world:
 * after the first ClientBegin, and after each respawn
 * Initializes all non-persistant parts of playerState
 *
 * @param[in,out] ent
 * @param[in] revived
 * @param[in] teamChange
 * @param[in] restoreHealth
 */
void ClientSpawn(gentity_t *ent, qboolean revived, qboolean teamChange, qboolean restoreHealth)
{
	int                index = ent - g_entities;
	vec3_t             spawn_origin, spawn_angles;
	gclient_t          *client = ent->client;
	int                i;
	clientPersistant_t savedPers;
	clientSession_t    savedSess;
	int                persistant[MAX_PERSISTANT];
	gentity_t          *spawnPoint;
	int                flags;
	int                savedPing;
	int                savedTeam;
	int                savedDeathTime;

	client->pers.lastSpawnTime            = level.time;
	client->pers.lastBattleSenseBonusTime = level.timeCurrent;
	client->pers.lastHQMineReportTime     = level.timeCurrent;

/*
#ifndef ETLEGACY_DEBUG
    if( !client->sess.versionOK ) {
        char *clientMismatchedVersion = G_CheckVersion( ent );	// returns NULL if version is identical

        if( clientMismatchedVersion ) {
            trap_DropClient( ent - g_entities, va( "Client/Server game mismatch: '%s/%s'", clientMismatchedVersion, LEGACY_VERSION ) );
        } else {
            client->sess.versionOK = qtrue;
        }
    }
#endif
*/

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if (revived)
	{
		spawnPoint = ent;
		VectorCopy(ent->r.currentOrigin, spawn_origin);
		spawn_origin[2] += 9;   // spawns seem to be sunk into ground?
		VectorCopy(ent->s.angles, spawn_angles);
	}
	else
	{
		G_UpdateSpawnPointStatePlayerCounts();
		// let's just be sure it does the right thing at all times. (well maybe not the right thing, but at least not the bad thing!)
		//if( client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.sessionTeam == TEAM_FREE ) {
		if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
		{
			spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
		}
		else
		{
			spawnPoint = SelectCTFSpawnPoint(client->sess.sessionTeam, client->pers.teamState.state, spawn_origin, spawn_angles, client->sess.userSpawnPointValue);
		}
	}

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags  = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;

	// unlagged reset history markers
	G_ResetMarkers(ent);
	ent->client->backupMarker.time = 0;
	// unlagged

	flags |= (client->ps.eFlags & EF_VOTED);

	if (!teamChange)
	{
		flags |= (client->ps.eFlags & EF_READY);
	}
	// clear everything but the persistant data

	ent->s.eFlags &= ~EF_MOUNTEDTANK;

	savedPers      = client->pers;
	savedSess      = client->sess;
	savedPing      = client->ps.ping;
	savedTeam      = client->ps.teamNum;
	savedDeathTime = client->deathTime;

	for (i = 0 ; i < MAX_PERSISTANT ; i++)
	{
		persistant[i] = client->ps.persistant[i];
	}

	{
		qboolean set = client->maxlivescalced;

		Com_Memset(client, 0, sizeof(*client));

		client->maxlivescalced = set;
	}

	client->pers              = savedPers;
	client->sess              = savedSess;
	client->ps.ping           = savedPing;
	client->ps.teamNum        = savedTeam;
	client->disguiseClientNum = -1;

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		client->ps.pm_type = PM_INTERMISSION;
	}

	for (i = 0 ; i < MAX_PERSISTANT ; i++)
	{
		client->ps.persistant[i] = persistant[i];
	}

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	if (revived)
	{
		client->ps.persistant[PERS_REVIVE_COUNT]++;
	}
	client->ps.persistant[PERS_TEAM]        = client->sess.sessionTeam;
	client->ps.persistant[PERS_HWEAPON_USE] = 0;

	client->airOutTime = level.time + HOLDBREATHTIME;
	// breathbar
	ent->client->pmext.airleft          = ent->client->airOutTime - level.time;
	ent->client->ps.stats[STAT_AIRLEFT] = HOLDBREATHTIME;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags                 = flags;
	client->deathTime                 = savedDeathTime;

	// Reset/restore special weapon time
	if (
		((g_stickyCharge.integer & STICKYCHARGE_SELFKILL) && isMortalSelfDamage(ent))
		|| (g_stickyCharge.integer & STICKYCHARGE_ANYDEATH)
		)
	{
		switch (client->sess.latchPlayerType)
		{
		case PC_MEDIC:
			client->ps.classWeaponTime = client->pers.savedClassWeaponTimeMed;
			break;
		case PC_ENGINEER:
			client->ps.classWeaponTime = client->pers.savedClassWeaponTimeEng;
			break;
		case PC_FIELDOPS:
			client->ps.classWeaponTime = client->pers.savedClassWeaponTimeFop;
			break;
		case PC_COVERTOPS:
			client->ps.classWeaponTime = client->pers.savedClassWeaponTimeCvop;
			break;
		default:
			client->ps.classWeaponTime = client->pers.savedClassWeaponTime;
		}

		if (client->ps.classWeaponTime >= 0)
		{
			// class change
			if (client->sess.playerType != client->sess.latchPlayerType)
			{
				// restore the weapon time as it was on the moment of death
				client->ps.classWeaponTime = level.time - client->ps.classWeaponTime;
			}
			else
			{
				// compenstate limbo time
				client->ps.classWeaponTime = savedDeathTime - client->ps.classWeaponTime;
			}
		}
	}
	else
	{
		client->ps.classWeaponTime = -999999;
	}

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client            = &level.clients[index];
	ent->takedamage        = qtrue;
	ent->inuse             = qtrue;
	ent->classname         = "player";
	ent->r.contents        = CONTENTS_BODY;
	ent->clipmask          = MASK_PLAYERSOLID;

	// Init to -1 on first spawn;
	if (!revived)
	{
		ent->props_frame_state = -1;
	}

	ent->die        = player_die;
	ent->waterlevel = 0;
	ent->watertype  = 0;
	ent->flags      = 0;

	VectorCopy(playerMins, ent->r.mins);
	VectorCopy(playerMaxs, ent->r.maxs);

	// setup the bounding boxes and viewheights for prediction
	VectorCopy(ent->r.mins, client->ps.mins);
	VectorCopy(ent->r.maxs, client->ps.maxs);

	client->ps.crouchViewHeight = CROUCH_VIEWHEIGHT;
	client->ps.standViewHeight  = DEFAULT_VIEWHEIGHT;
	client->ps.deadViewHeight   = DEAD_VIEWHEIGHT;

	client->ps.crouchMaxZ = client->ps.maxs[2] - (client->ps.standViewHeight - client->ps.crouchViewHeight);

	client->ps.runSpeedScale    = 0.8f;
	client->ps.sprintSpeedScale = 1.1f;
	client->ps.crouchSpeedScale = 0.25f;
	client->ps.weaponstate      = WEAPON_READY;

	client->pmext.sprintTime   = SPRINTTIME;
	client->ps.sprintExertTime = 0;

	client->ps.friction = 1.0f;

	// retrieve from the persistant storage (we use this in pmoveExt_t beause we need it in bg_*)
	client->pmext.bAutoReload = client->pers.bAutoReloadAux;

	client->ps.clientNum = index;

	trap_GetUsercmd(client - level.clients, &ent->client->pers.cmd);

	// Add appropriate weapons
	if (!revived)
	{
		qboolean update = qfalse;

		if (client->sess.playerType != client->sess.latchPlayerType)
		{
			update = qtrue;
		}

		client->sess.playerType = client->sess.latchPlayerType;

		if (G_IsWeaponDisabled(ent, client->sess.latchPlayerWeapon))
		{
			bg_playerclass_t *classInfo;

			classInfo = BG_PlayerClassForPlayerState(&ent->client->ps);

			client->sess.latchPlayerWeapon = classInfo->classPrimaryWeapons[0].weapon;
			update                         = qtrue;
		}

		if (client->sess.playerWeapon != client->sess.latchPlayerWeapon)
		{
			client->sess.playerWeapon = client->sess.latchPlayerWeapon;
			update                    = qtrue;
		}

		if (G_IsWeaponDisabled(ent, client->sess.playerWeapon))
		{
			bg_playerclass_t *classInfo;

			classInfo = BG_PlayerClassForPlayerState(&ent->client->ps);

			client->sess.playerWeapon = classInfo->classPrimaryWeapons[0].weapon;
			update                    = qtrue;
		}

		if (client->sess.playerWeapon2 != client->sess.latchPlayerWeapon2)
		{
			client->sess.playerWeapon2 = client->sess.latchPlayerWeapon2;
			update                     = qtrue;
		}

		if (update)
		{
			ClientUserinfoChanged(index);
		}
	}

	// keep it isolated from spectator to be safe still
	if (client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		// Moved the invul. stuff out of SetWolfSpawnWeapons and put it here for clarity
		if (g_fastres.integer == 1 && revived)
		{
			client->ps.powerups[PW_INVULNERABLE] = level.time + 1000;
		}
		else
		{
			client->ps.powerups[PW_INVULNERABLE] = level.time + 3000;
		}
	}

	// no need to update character for revive
	if (!revived)
	{
		G_UpdateCharacter(client);
	}

	SetWolfSpawnWeapons(client);

	// increases stats[STAT_MAX_HEALTH] based on # of medics in game
	AddMedicTeamBonus(client);

	if (!revived)
	{
		client->pers.cmd.weapon = ent->client->ps.weapon;
	}

	// ***NOTE*** the following line is order-dependent and must *FOLLOW* SetWolfSpawnWeapons() in multiplayer
	// AddMedicTeamBonus() now adds medic team bonus and stores in ps.stats[STAT_MAX_HEALTH].

	if (client->sess.skill[SK_BATTLE_SENSE] >= 3)
	{
		// We get some extra max health, but don't spawn with that much
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] - 15;
	}
	else
	{
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];
	}

	if (ent->client->sess.playerType == PC_MEDIC)
	{
		ent->health = ent->client->ps.stats[STAT_HEALTH] /= 1.12;
	}

	G_SetOrigin(ent, spawn_origin);
	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	if (!revived)
	{
		SetClientViewAngle(ent, spawn_angles);
	}
	else
	{
		// we try to orient them in the freelook direction when revived
		vec3_t newangle;

		newangle[YAW]   = SHORT2ANGLE(ent->client->pers.cmd.angles[YAW] + ent->client->ps.delta_angles[YAW]);
		newangle[PITCH] = 0;
		newangle[ROLL]  = 0;

		SetClientViewAngle(ent, newangle);
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_LinkEntity(ent);
	}

	client->respawnTime           = level.timeCurrent;
	client->inactivityTime        = level.time + g_inactivity.integer * 1000;
	client->inactivityWarning     = qfalse;
	client->inactivitySecondsLeft = (g_inactivity.integer) ? g_inactivity.integer : 60;
	client->latched_buttons       = 0;
	client->latched_wbuttons      = 0;
	client->deathTime             = 0;

	if (level.intermissiontime)
	{
		MoveClientToIntermission(ent, (EF_VOTED & client->ps.eFlags));

		// send current mapvote tally
		if (g_gametype.integer == GT_WOLF_MAPVOTE)
		{
			G_IntermissionVoteTally(ent);
		}
	}
	else
	{
		// fire the targets of the spawn point
		if (!revived)
		{
			G_UseTargets(spawnPoint, ent);
		}
	}

#ifdef FEATURE_LUA
	// *LUA* API callbacks
	G_LuaHook_ClientSpawn(ent - g_entities, revived, teamChange, restoreHealth);
#endif

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime           = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;

	ClientThink(ent - g_entities);

	// positively link the client, even if the command times are weird
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		BG_PlayerStateToEntityState(&client->ps, &ent->s, level.time, qtrue);
		VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);
		trap_LinkEntity(ent);
	}

	// run the presend to set anything else
	ClientEndFrame(ent);

	// set idle animation on weapon
	ent->client->ps.weapAnim = ((ent->client->ps.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | WEAP_IDLE1;

	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps, &ent->s, level.time, qtrue);

	// G_ResetMarkers(ent);

	// start the scripting system
	if (!revived && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		// call entity scripting event
		G_Script_ScriptEvent(ent, "playerstart", "");
	}

	// when switching teams, reset the commandmap/radar icons
	// so your own (meanwhile invalid) landmine-markers don't show up ...
	if (teamChange && !g_landminetimeout.integer)
	{
		G_ResetTeamMapData();
	}
}

/**
 * @brief Called when a player drops from the server.
 * Will not be called between levels.
 *
 * @details This should NOT be called directly by any game logic,
 * call trap_DropClient(), which will call this and do
*  server system housekeeping.
 *
 * @param[in] clientNum
 */
void ClientDisconnect(int clientNum)
{
	gentity_t *ent  = g_entities + clientNum;
	gentity_t *flag = NULL;
	int       i;

	if (!ent->client)
	{
		return;
	}

#ifdef FEATURE_RATING
	// rating already recorded before intermission
	if (g_skillRating.integer && !level.intermissiontime)
	{
		G_SkillRatingSetClientRating(ent->client);
	}
#endif

#ifdef FEATURE_PRESTIGE
	if (g_prestige.integer && !level.intermissiontime)
	{
		G_SetClientPrestige(ent->client, qfalse);
	}
#endif

	if (g_xpSaver.integer && g_gametype.integer == GT_WOLF_CAMPAIGN && !level.intermissiontime)
	{
		G_XPSaver_Store(ent->client);
	}

#ifdef FEATURE_LUA
	// LUA API callbacks
	G_LuaHook_ClientDisconnect(clientNum);
#endif

#ifdef FEATURE_OMNIBOT
	Bot_Event_ClientDisConnected(clientNum);
#endif

	G_RemoveClientFromFireteams(clientNum, qtrue, qfalse);
	G_RemoveFromAllIgnoreLists(clientNum);
	G_LeaveTank(ent, qfalse);

	// update uniform owners
	for (i = 0 ; i < level.numConnectedClients ; i++)
	{
		flag = g_entities + level.sortedClients[i];
		if (flag->client->disguiseClientNum == clientNum && flag->client->ps.powerups[PW_OPS_DISGUISED])
		{
			CPx(flag->s.number, "cp \"Your cover has been blown, steal a new uniform soon!\" 1");
			flag->client->disguiseClientNum = flag->s.clientNum;
			// sound effect
			G_AddEvent(flag, EV_DISGUISE_SOUND, 0); // FIXME: find a new sound?
			ClientUserinfoChanged(flag->s.clientNum);
			// no break - uniform might be stolen more than once
		}
	}

	// stop any following clients
	for (i = 0 ; i < level.numConnectedClients ; i++)
	{
		flag = g_entities + level.sortedClients[i];
		if (flag->client->sess.sessionTeam == TEAM_SPECTATOR
		    && flag->client->sess.spectatorState == SPECTATOR_FOLLOW
		    && flag->client->sess.spectatorClient == clientNum)
		{
			StopFollowing(flag);
		}
		if ((flag->client->ps.pm_flags & PMF_LIMBO) && flag->client->sess.spectatorClient == clientNum)
		{
			Cmd_FollowCycle_f(flag, 1, qfalse);
		}
	}

	// remove complaint client
	for (i = 0 ; i < level.numConnectedClients ; i++)
	{
		flag = g_entities + level.sortedClients[i];
		if (flag->client->pers.complaintEndTime > level.time && flag->client->pers.complaintClient == clientNum)
		{
			flag->client->pers.complaintClient  = -1;
			flag->client->pers.complaintEndTime = -1;

			CPx(level.sortedClients[i], "complaint -2");
			break;
		}
	}

	if (g_landminetimeout.integer)
	{
		G_ExplodeMines(ent);
	}
	G_FadeItems(ent, MOD_SATCHEL);

	// remove ourself from teamlists
	{
		mapEntityData_t      *mEnt;
		mapEntityData_Team_t *teamList;
		mapEntityData_t      *mEntFree;

		for (i = 0; i < 2; i++)
		{
			teamList = &mapEntityData[i];

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL)
			{
				G_FreeMapEntityData(teamList, mEnt);
			}

			mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

			while (mEnt)
			{
				mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);

				G_FreeMapEntityData(teamList, mEntFree);
			}
		}
	}

	// send effect if they were completely connected
	if (ent->client->pers.connected == CON_CONNECTED
	    && ent->client->sess.sessionTeam != TEAM_SPECTATOR
	    && !(ent->client->ps.pm_flags & PMF_LIMBO))
	{

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossWeapons(ent);

		G_DropItems(ent);

		// Log stats too
		G_LogPrintf("WeaponStats: %s\n", G_createStats(ent));
	}

	// remove mapvote
	if (g_gametype.integer == GT_WOLF_MAPVOTE && g_gamestate.integer == GS_INTERMISSION)
	{
		if (g_mapVoteFlags.integer & MAPVOTE_MULTI_VOTE && ent->client->ps.eFlags & EF_VOTED)
		{
			for (i = 0; i < 3; i++)
			{
				if (ent->client->sess.mapVotedFor[i] != -1)
				{
					level.mapvoteinfo[ent->client->sess.mapVotedFor[i]].numVotes   -= (i + 1);
					level.mapvoteinfo[ent->client->sess.mapVotedFor[i]].totalVotes -= (i + 1);
				}
			}
		}
		else if (ent->client->ps.eFlags & EF_VOTED)
		{
			level.mapvoteinfo[ent->client->sess.mapVotedFor[0]].numVotes--;
			level.mapvoteinfo[ent->client->sess.mapVotedFor[0]].totalVotes--;
		}

		// send updated vote tally to all
		G_IntermissionVoteTally(NULL);
	}

	G_LogPrintf("ClientDisconnect: %i\n", clientNum);

	trap_UnlinkEntity(ent);
	ent->s.modelindex                      = 0;
	ent->inuse                             = qfalse;
	ent->classname                         = "disconnected";
	ent->client->hasaward                  = qfalse;
	ent->client->medals                    = 0;
	ent->client->pers.connected            = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM]  = TEAM_FREE;
	ent->client->ps.persistant[PERS_SCORE] = 0;
	i                                      = ent->client->sess.sessionTeam;
	ent->client->sess.sessionTeam          = TEAM_FREE;
	ent->active                            = 0;

	// this needs to be cleared
	ent->r.svFlags &= ~SVF_BOT;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	G_verifyMatchState((team_t)i);
#ifdef FEATURE_MULTIVIEW
	G_smvAllRemoveSingleClient(ent - g_entities);
#endif

#ifdef FEATURE_RATING
	if (g_skillRating.integer)
	{
		level.axisProb   = G_CalculateWinProbability(TEAM_AXIS);
		level.alliesProb = 1.0f - level.axisProb;
	}
#endif
}


/**
 * @brief In just the GAME DLL, we want to store the groundtrace surface stuff,
 * so we don't have to keep tracing.
 *
 * @param[in] clientNum
 * @param[in] surfaceFlags
 */
void ClientStoreSurfaceFlags(int clientNum, int surfaceFlags)
{
	// Store the surface flags
	g_entities[clientNum].surfaceFlags = surfaceFlags;
}

/**
 * @brief ClientHitboxMaxZ
 * @param[in] hitEnt
 * @return the proper value to use for the entity's r.maxs[2] when running a trace.
 */
float ClientHitboxMaxZ(gentity_t *hitEnt)
{
	if (!hitEnt)
	{
		return 0;
	}
	if (!hitEnt->client)
	{
		return hitEnt->r.maxs[2];
	}

	if (hitEnt->client->ps.eFlags & EF_DEAD)
	{
		return DEAD_BODYHEIGHT;
	}
	else if (hitEnt->client->ps.eFlags & EF_PRONE)
	{
		return PRONE_BODYHEIGHT;
	}
	else if (hitEnt->client->ps.eFlags & EF_CROUCHING &&
	         hitEnt->client->ps.velocity[0] == 0.f && hitEnt->client->ps.velocity[1] == 0.f)
	{
		// crouched idle animation is lower than the moving one
		return CROUCH_IDLE_BODYHEIGHT;
	}
	else if (hitEnt->client->ps.eFlags & EF_CROUCHING)
	{
		// crouched moving animation is higher than the idle one
		return CROUCH_BODYHEIGHT;
	}
	else
	{
		return g_playerHitBoxHeight.value;
	}
}
