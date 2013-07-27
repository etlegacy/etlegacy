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
 * @file g_client.c
 * @brief client functions that don't happen every frame
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

qboolean CompareIPNoPort(char const *ip1, char const *ip2);

// new bounding box
vec3_t playerMins = { -18, -18, -24 };
vec3_t playerMaxs = { 18, 18, 48 };

/*
QUAKED info_player_deathmatch (1 0 1) (-18 -18 -24) (18 18 48)
potential spawning position for deathmatch games.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
If the start position is targeting an entity, the players camera will start out facing that ent (like an info_notnull)
*/
void SP_info_player_deathmatch(gentity_t *ent)
{
	int    i;
	vec3_t dir;

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
		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}
}

/*
QUAKED info_player_checkpoint (1 0 0) (-16 -16 -24) (16 16 32) a b c d
these are start points /after/ the level start
the letter (a b c d) designates the checkpoint that needs to be complete in order to use this start position
*/
void SP_info_player_checkpoint(gentity_t *ent)
{
	ent->classname = "info_player_checkpoint";
	SP_info_player_deathmatch(ent);
}

/*
QUAKED info_player_start (1 0 0) (-18 -18 -24) (18 18 48)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent)
{
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/*
QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) AXIS ALLIED
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission(gentity_t *ent)
{
}

/*
=======================================================================
  SelectSpawnPoint
=======================================================================
*/

/*
================
SpotWouldTelefrag
================
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

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define MAX_SPAWN_POINTS    128
gentity_t *SelectNearestDeathmatchSpawnPoint(vec3_t from)
{
	gentity_t *spot = NULL;
	vec3_t    delta;
	float     dist, nearestDist = 999999;
	gentity_t *nearestSpot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		VectorSubtract(spot->r.currentOrigin, from, delta);
		dist = VectorLength(delta);
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_SPAWN_POINTS    128
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

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles)
{
	gentity_t *nearestSpot = SelectNearestDeathmatchSpawnPoint(avoidPoint);
	gentity_t *spot        = SelectRandomDeathmatchSpawnPoint();

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

/*
===========
SelectSpectatorSpawnPoint
============
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

/*
===============
InitBodyQue
===============
*/
void InitBodyQue(void)
{
	int       i;
	gentity_t *ent;

	level.bodyQueIndex = 0;
	for (i = 0; i < BODY_QUEUE_SIZE ; i++)
	{
		ent              = G_Spawn();
		ent->classname   = "bodyque";
		ent->neverFree   = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodyUnlink

Called by BodySink
=============
*/
void BodyUnlink(gentity_t *ent)
{
	trap_UnlinkEntity(ent);
	ent->physicsObject = qfalse;
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink2(gentity_t *ent)
{
	ent->physicsObject = qfalse;
	ent->nextthink     = level.time + BODY_TIME(BODY_TEAM(ent)) + 1500;
	ent->think         = BodyUnlink;
	ent->s.pos.trType  = TR_LINEAR;
	ent->s.pos.trTime  = level.time;
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	VectorSet(ent->s.pos.trDelta, 0, 0, -8);
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink(gentity_t *ent)
{
	if (ent->activator)
	{
		// see if parent is still disguised
		if (ent->activator->client->ps.powerups[PW_OPS_DISGUISED])
		{
			ent->nextthink = level.time + 100;
			return;
		}
		else
		{
			ent->activator = NULL;
		}
	}

	BodySink2(ent);
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue(gentity_t *ent)
{
	gentity_t *body;
	int       contents, i;

	trap_UnlinkEntity(ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents(ent->client->ps.origin, -1);
	if (contents & CONTENTS_NODROP)
	{
		return;
	}

	// grab a body que and cycle to the next one
	body               = level.bodyQue[level.bodyQueIndex];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

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

	//if ( ent->client->ps.powerups[PW_HELMETSHIELD])
	//{
	//	body->s.powerups |= (1 << PW_HELMETSHIELD);
	//}

	if (body->s.groundEntityNum == ENTITYNUM_NONE)
	{
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy(ent->client->ps.velocity, body->s.pos.trDelta);
	}
	else
	{
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// Clear out event system
	for (i = 0; i < MAX_EVENTS; i++)
	{
		body->s.events[i] = 0;
	}
	body->s.eventSequence = 0;

	// time needed to complete animation
	body->s.torsoAnim = body->s.legsAnim = ent->client->legsDeathAnim;

	body->r.svFlags = ent->r.svFlags & ~SVF_BOT;
	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);

	//  bodies have lower bounding box
	body->r.maxs[2] = 0;

	body->s.effect1Time = ent->client->deathAnimTime;

	// this'll look better
	if (body->s.onFireEnd > level.time)
	{
		body->s.onFireEnd = ent->client->deathAnimTime + 1500;
	}

	VectorCopy(body->s.pos.trBase, body->r.currentOrigin);
	/* WIP
	if ( ent->client->deathAnim )
	{
	    vec3_t			origin, offset;
	    grefEntity_t	refent;

	    mdx_PlayerAnimation( body );
	    mdx_gentity_to_grefEntity( body, &refent, level.time );
	    mdx_tag_position( body, &refent, origin, "tag_ubelt", 0, 0 );

	    // convert it to vector around null pos
	    offset[0] = origin[0] - body->r.currentOrigin[0];
	    offset[1] = origin[1] - body->r.currentOrigin[1];
	    offset[2] = origin[2] = body->r.currentOrigin[2];

	    G_StepSlideCorpse( body, origin );
	    // this is the max vector we can reach
	    VectorCopy ( body->s.pos.trBase, origin );

	    // change bounding box to be off the origin of the corpse
	    // that will make correct box agains model
	    body->r.maxs[0] = offset[0] + 18;
	    body->r.maxs[1] = offset[1] + 18;
	    body->r.mins[0] = offset[0] - 18;
	    body->r.mins[1] = offset[1] - 18;

	    body->r.currentOrigin[0] = origin[0] - offset[0];
	    body->r.currentOrigin[1] = origin[1] - offset[1];
	    body->r.currentOrigin[2] = origin[2];

	    // ok set it und Fertig!
	    VectorCopy ( body->r.currentOrigin, body->s.pos.trBase );
	}
	*/

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	// allow bullets to pass through bbox
	// need something to allow the hint for covert ops
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->r.ownerNum;

	BODY_TEAM(body)      = ent->client->sess.sessionTeam;
	BODY_CLASS(body)     = ent->client->sess.playerType;
	BODY_CHARACTER(body) = ent->client->pers.characterIndex;
	BODY_VALUE(body)     = 0;

	//if ( ent->client->ps.eFlags & EF_PANTSED ){
	//	body->s.time2 =	1;
	//}
	//else {
	body->s.time2 = 0;
	//}

	body->activator = NULL;
	body->nextthink = level.time + BODY_TIME(ent->client->sess.sessionTeam);
	body->think     = BodySink;
	body->die       = body_die;

	// don't take more damage if already gibbed
	if (ent->health <= GIB_HEALTH)
	{
		body->takedamage = qfalse;
	}
	else
	{
		body->takedamage = qtrue;
	}

	trap_LinkEntity(body);
}

//======================================================================

/*
==================
SetClientViewAngle
==================
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

/*
================
limbo
================
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
		ent->client->ps.viewlocked        = 0;
		ent->client->ps.viewlocked_entNum = 0;

		ent->r.maxs[2]           = 0;
		ent->r.currentOrigin[2] += 8;
		contents                 = trap_PointContents(ent->r.currentOrigin, -1); // drop stuff
		ent->s.weapon            = ent->client->limboDropWeapon; // stored in player_die()
		if (makeCorpse && !(contents & CONTENTS_NODROP))
		{
			TossClientItems(ent);
		}

		ent->client->sess.spectatorClient = startclient;
		Cmd_FollowCycle_f(ent, 1);  // get fresh spectatorClient

		if (ent->client->sess.spectatorClient == startclient)
		{
			// No one to follow, so just stay put
			ent->client->sess.spectatorState = SPECTATOR_FREE;
		}
		else
		{
			ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		}

		if (ent->client->sess.sessionTeam == TEAM_AXIS)
		{
			ent->client->deployQueueNumber = level.redNumWaiting;
			level.redNumWaiting++;
		}
		else if (ent->client->sess.sessionTeam == TEAM_ALLIES)
		{
			ent->client->deployQueueNumber = level.blueNumWaiting;
			level.blueNumWaiting++;
		}

		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];

			if (((cl->ps.pm_flags & PMF_LIMBO) ||
			     (cl->sess.sessionTeam == TEAM_SPECTATOR && cl->sess.spectatorState == SPECTATOR_FOLLOW)) &&
			    cl->sess.spectatorClient == ent - g_entities)     //ent->s.number ) {
			{
				Cmd_FollowCycle_f(&g_entities[level.sortedClients[i]], 1);
			}
		}
	}
}

/*
================
reinforce
    called when time expires for a team deployment cycle and there is at least one guy ready to go
================
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

/*
================
respawn
================
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

/*
================
TeamCount

    Returns number of players on a team
================
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

/*
================
PickTeam
================
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

/*
===========
AddExtraSpawnAmmo
===========
*/
static void AddExtraSpawnAmmo(gclient_t *client, weapon_t weaponNum)
{
	switch (weaponNum)
	{
	//case WP_KNIFE:
	case WP_LUGER:
	case WP_COLT:
	case WP_STEN:
	case WP_SILENCER:
	case WP_CARBINE:
	case WP_KAR98:
	case WP_SILENCED_COLT:
		if (client->sess.skill[SK_LIGHT_WEAPONS] >= 1)
		{
			client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
		}
		break;
	case WP_MP40:
	case WP_THOMPSON:
		if ((client->sess.skill[SK_FIRST_AID] >= 1 && client->sess.playerType == PC_MEDIC) || client->sess.skill[SK_LIGHT_WEAPONS] >= 1)
		{
			client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
		}
		break;
	case WP_M7:
	case WP_GPG40:
		if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
		{
			client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += 4;
		}
		break;
	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
		if (client->sess.playerType == PC_ENGINEER)
		{
			if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
			{
				client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 4;
			}
		}
		if (client->sess.playerType == PC_MEDIC)
		{
			if (client->sess.skill[SK_FIRST_AID] >= 1)
			{
				client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 1;
			}
		}
		break;
	case WP_MEDIC_SYRINGE:
	case WP_MEDIC_ADRENALINE:
		if (client->sess.skill[SK_FIRST_AID] >= 2)
		{
			client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 2;
		}
		break;
	case WP_GARAND:
	case WP_K43:
	case WP_FG42:
		if (client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1 || client->sess.skill[SK_LIGHT_WEAPONS] >= 1)
		{
			client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
		}
		break;
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		if (client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1)
		{
			client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
		}
		break;
	default:
		break;
	}
}

qboolean AddWeaponToPlayer(gclient_t *client, weapon_t weapon, int ammo, int ammoclip, qboolean setcurrent)
{
	COM_BitSet(client->ps.weapons, weapon);
	client->ps.ammoclip[BG_FindClipForWeapon(weapon)] = ammoclip;
	client->ps.ammo[BG_FindAmmoForWeapon(weapon)]     = ammo;
	if (setcurrent)
	{
		client->ps.weapon = weapon;
	}

	// skill handling
	AddExtraSpawnAmmo(client, weapon);

#ifdef FEATURE_OMNIBOT
	Bot_Event_AddWeapon(client->ps.clientNum, Bot_WeaponGameToBot(weapon));
#endif

	return qtrue;
}

/*
===========
SetWolfSpawnWeapons
===========
*/
void SetWolfSpawnWeapons(gclient_t *client)
{
	int pc = client->sess.playerType;

	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_ResetWeapons(client->ps.clientNum);
#endif

	// Reset special weapon time
	client->ps.classWeaponTime = -999999;

	// Communicate it to cgame
	client->ps.stats[STAT_PLAYER_CLASS] = pc;

	// Abuse teamNum to store player class as well (can't see stats for all clients in cgame)
	client->ps.teamNum = pc;

	// zero out all ammo counts
	memset(client->ps.ammo, 0, MAX_WEAPONS * sizeof(int));

	// All players start with a knife (not OR-ing so that it clears previous weapons)
	client->ps.weapons[0] = 0;
	client->ps.weapons[1] = 0;

	AddWeaponToPlayer(client, WP_KNIFE, 1, 0, qtrue);

	client->ps.weaponstate = WEAPON_READY;

	// Engineer gets dynamite
	if (pc == PC_ENGINEER)
	{
		AddWeaponToPlayer(client, WP_DYNAMITE, 0, 1, qfalse);
		AddWeaponToPlayer(client, WP_PLIERS, 0, 1, qfalse);

		if (g_knifeonly.integer != 1)
		{
			if (client->sess.skill[SK_BATTLE_SENSE] >= 1)
			{
				if (AddWeaponToPlayer(client, WP_BINOCULARS, 1, 0, qfalse))
				{
					client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
				}
			}

			if (client->sess.sessionTeam == TEAM_AXIS)
			{
				switch (client->sess.playerWeapon)
				{
				case WP_KAR98:
					if (AddWeaponToPlayer(client, WP_KAR98, GetAmmoTableData(WP_KAR98)->defaultStartingAmmo, GetAmmoTableData(WP_KAR98)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_GPG40, GetAmmoTableData(WP_GPG40)->defaultStartingAmmo, GetAmmoTableData(WP_GPG40)->defaultStartingClip, qfalse);
					}
					break;
				default:
					AddWeaponToPlayer(client, WP_MP40, GetAmmoTableData(WP_MP40)->defaultStartingAmmo, GetAmmoTableData(WP_MP40)->defaultStartingClip, qtrue);
					break;
				}
				AddWeaponToPlayer(client, WP_LANDMINE, GetAmmoTableData(WP_LANDMINE)->defaultStartingAmmo, GetAmmoTableData(WP_LANDMINE)->defaultStartingClip, qfalse);
				AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 4, qfalse);

			}
			else
			{
				switch (client->sess.playerWeapon)
				{
				case WP_CARBINE:
					if (AddWeaponToPlayer(client, WP_CARBINE, GetAmmoTableData(WP_CARBINE)->defaultStartingAmmo, GetAmmoTableData(WP_CARBINE)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_M7, GetAmmoTableData(WP_M7)->defaultStartingAmmo, GetAmmoTableData(WP_M7)->defaultStartingClip, qfalse);
					}
					break;
				default:
					AddWeaponToPlayer(client, WP_THOMPSON, GetAmmoTableData(WP_THOMPSON)->defaultStartingAmmo, GetAmmoTableData(WP_THOMPSON)->defaultStartingClip, qtrue);
					break;
				}
				AddWeaponToPlayer(client, WP_LANDMINE, GetAmmoTableData(WP_LANDMINE)->defaultStartingAmmo, GetAmmoTableData(WP_LANDMINE)->defaultStartingClip, qfalse);
				AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 4, qfalse);
			}
		}
	}

	if (g_knifeonly.integer != 1)
	{
		// Field ops gets binoculars, ammo pack, artillery, and a grenade
		if (pc == PC_FIELDOPS)
		{
			AddWeaponToPlayer(client, WP_AMMO, 0, 1, qfalse);

			if (AddWeaponToPlayer(client, WP_BINOCULARS, 1, 0, qfalse))
			{
				client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
			}

			AddWeaponToPlayer(client, WP_SMOKE_MARKER, GetAmmoTableData(WP_SMOKE_MARKER)->defaultStartingAmmo, GetAmmoTableData(WP_SMOKE_MARKER)->defaultStartingClip, qfalse);

			if (client->sess.sessionTeam == TEAM_AXIS)
			{
				AddWeaponToPlayer(client, WP_MP40, GetAmmoTableData(WP_MP40)->defaultStartingAmmo, GetAmmoTableData(WP_MP40)->defaultStartingClip, qtrue);
				AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 1, qfalse);
			}
			else
			{
				AddWeaponToPlayer(client, WP_THOMPSON, GetAmmoTableData(WP_THOMPSON)->defaultStartingAmmo, GetAmmoTableData(WP_THOMPSON)->defaultStartingClip, qtrue);
				AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 1, qfalse);
			}
		}
		else if (pc == PC_MEDIC)
		{
			if (client->sess.skill[SK_BATTLE_SENSE] >= 1)
			{
				if (AddWeaponToPlayer(client, WP_BINOCULARS, 1, 0, qfalse))
				{
					client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
				}
			}

			AddWeaponToPlayer(client, WP_MEDIC_SYRINGE, GetAmmoTableData(WP_MEDIC_SYRINGE)->defaultStartingAmmo, GetAmmoTableData(WP_MEDIC_SYRINGE)->defaultStartingClip, qfalse);
			if (client->sess.skill[SK_FIRST_AID] >= 4)
			{
				AddWeaponToPlayer(client, WP_MEDIC_ADRENALINE, GetAmmoTableData(WP_MEDIC_ADRENALINE)->defaultStartingAmmo, GetAmmoTableData(WP_MEDIC_ADRENALINE)->defaultStartingClip, qfalse);
			}

			AddWeaponToPlayer(client, WP_MEDKIT, GetAmmoTableData(WP_MEDKIT)->defaultStartingAmmo, GetAmmoTableData(WP_MEDKIT)->defaultStartingClip, qfalse);

			if (client->sess.sessionTeam == TEAM_AXIS)
			{
				AddWeaponToPlayer(client, WP_MP40, 0, GetAmmoTableData(WP_MP40)->defaultStartingClip, qtrue);
				AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 1, qfalse);
			}
			else
			{
				AddWeaponToPlayer(client, WP_THOMPSON, 0, GetAmmoTableData(WP_THOMPSON)->defaultStartingClip, qtrue);
				AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 1, qfalse);
			}
		}
		else if (pc == PC_SOLDIER)
		{
			if (client->sess.skill[SK_BATTLE_SENSE] >= 1)
			{
				if (AddWeaponToPlayer(client, WP_BINOCULARS, 1, 0, qfalse))
				{
					client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
				}
			}

			switch (client->sess.sessionTeam)
			{
			case TEAM_AXIS:
				switch (client->sess.playerWeapon)
				{
				default:
				case WP_MP40:
					AddWeaponToPlayer(client, WP_MP40, 2 * (GetAmmoTableData(WP_MP40)->defaultStartingAmmo), GetAmmoTableData(WP_MP40)->defaultStartingClip, qtrue);
					break;
				case WP_PANZERFAUST:
					AddWeaponToPlayer(client, WP_PANZERFAUST, GetAmmoTableData(WP_PANZERFAUST)->defaultStartingAmmo, GetAmmoTableData(WP_PANZERFAUST)->defaultStartingClip, qtrue);
					break;
				case WP_FLAMETHROWER:
					AddWeaponToPlayer(client, WP_FLAMETHROWER, GetAmmoTableData(WP_FLAMETHROWER)->defaultStartingAmmo, GetAmmoTableData(WP_FLAMETHROWER)->defaultStartingClip, qtrue);
					break;
				case WP_MOBILE_MG42:
					if (AddWeaponToPlayer(client, WP_MOBILE_MG42, GetAmmoTableData(WP_MOBILE_MG42)->defaultStartingAmmo, GetAmmoTableData(WP_MOBILE_MG42)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_MOBILE_MG42_SET, GetAmmoTableData(WP_MOBILE_MG42_SET)->defaultStartingAmmo, GetAmmoTableData(WP_MOBILE_MG42_SET)->defaultStartingClip, qfalse);
					}
					break;
				case WP_MORTAR:
					if (AddWeaponToPlayer(client, WP_MORTAR, GetAmmoTableData(WP_MORTAR)->defaultStartingAmmo, GetAmmoTableData(WP_MORTAR)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_MORTAR_SET, GetAmmoTableData(WP_MORTAR_SET)->defaultStartingAmmo, GetAmmoTableData(WP_MORTAR_SET)->defaultStartingClip, qfalse);
					}
					break;
				}
				break;
			case TEAM_ALLIES:
				switch (client->sess.playerWeapon)
				{
				default:
				case WP_THOMPSON:
					AddWeaponToPlayer(client, WP_THOMPSON, 2 * (GetAmmoTableData(WP_THOMPSON)->defaultStartingAmmo), GetAmmoTableData(WP_THOMPSON)->defaultStartingClip, qtrue);
					break;
				case WP_PANZERFAUST:
					AddWeaponToPlayer(client, WP_PANZERFAUST, GetAmmoTableData(WP_PANZERFAUST)->defaultStartingAmmo, GetAmmoTableData(WP_PANZERFAUST)->defaultStartingClip, qtrue);
					break;
				case WP_FLAMETHROWER:
					AddWeaponToPlayer(client, WP_FLAMETHROWER, GetAmmoTableData(WP_FLAMETHROWER)->defaultStartingAmmo, GetAmmoTableData(WP_FLAMETHROWER)->defaultStartingClip, qtrue);
					break;
				case WP_MOBILE_MG42:
					if (AddWeaponToPlayer(client, WP_MOBILE_MG42, GetAmmoTableData(WP_MOBILE_MG42)->defaultStartingAmmo, GetAmmoTableData(WP_MOBILE_MG42)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_MOBILE_MG42_SET, GetAmmoTableData(WP_MOBILE_MG42_SET)->defaultStartingAmmo, GetAmmoTableData(WP_MOBILE_MG42_SET)->defaultStartingClip, qfalse);
					}
					break;
				case WP_MORTAR:
					if (AddWeaponToPlayer(client, WP_MORTAR, GetAmmoTableData(WP_MORTAR)->defaultStartingAmmo, GetAmmoTableData(WP_MORTAR)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_MORTAR_SET, GetAmmoTableData(WP_MORTAR_SET)->defaultStartingAmmo, GetAmmoTableData(WP_MORTAR_SET)->defaultStartingClip, qfalse);
					}
					break;
				}
				break;
			default:
				break;
			}
		}
		else if (pc == PC_COVERTOPS)
		{
			switch (client->sess.playerWeapon)
			{
			case WP_K43:
			case WP_GARAND:
				if (client->sess.sessionTeam == TEAM_AXIS)
				{
					if (AddWeaponToPlayer(client, WP_K43, GetAmmoTableData(WP_K43)->defaultStartingAmmo, GetAmmoTableData(WP_K43)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_K43_SCOPE, GetAmmoTableData(WP_K43_SCOPE)->defaultStartingAmmo, GetAmmoTableData(WP_K43_SCOPE)->defaultStartingClip, qfalse);
					}
					break;
				}
				else
				{
					if (AddWeaponToPlayer(client, WP_GARAND, GetAmmoTableData(WP_GARAND)->defaultStartingAmmo, GetAmmoTableData(WP_GARAND)->defaultStartingClip, qtrue))
					{
						AddWeaponToPlayer(client, WP_GARAND_SCOPE, GetAmmoTableData(WP_GARAND_SCOPE)->defaultStartingAmmo, GetAmmoTableData(WP_GARAND_SCOPE)->defaultStartingClip, qfalse);
					}
					break;
				}
			case WP_FG42:
				if (AddWeaponToPlayer(client, WP_FG42, GetAmmoTableData(WP_FG42)->defaultStartingAmmo, GetAmmoTableData(WP_FG42)->defaultStartingClip, qtrue))
				{
					AddWeaponToPlayer(client, WP_FG42SCOPE, GetAmmoTableData(WP_FG42SCOPE)->defaultStartingAmmo, GetAmmoTableData(WP_FG42SCOPE)->defaultStartingClip, qfalse);
				}
				break;
			default:
				AddWeaponToPlayer(client, WP_STEN, 2 * (GetAmmoTableData(WP_STEN)->defaultStartingAmmo), GetAmmoTableData(WP_STEN)->defaultStartingClip, qtrue);
				break;
			}

			if (AddWeaponToPlayer(client, WP_BINOCULARS, 1, 0, qfalse))
			{
				client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
			}

			AddWeaponToPlayer(client, WP_SMOKE_BOMB, GetAmmoTableData(WP_SMOKE_BOMB)->defaultStartingAmmo, GetAmmoTableData(WP_SMOKE_BOMB)->defaultStartingClip, qfalse);

			// See if we already have a satchel charge placed - NOTE: maybe we want to change this so the thing voids on death
			if (G_FindSatchel(&g_entities[client->ps.clientNum]))
			{
				AddWeaponToPlayer(client, WP_SATCHEL, 0, 0, qfalse);        // Big Bang \o/
				AddWeaponToPlayer(client, WP_SATCHEL_DET, 0, 1, qfalse);    // Big Red Button for tha Big Bang
			}
			else
			{
				AddWeaponToPlayer(client, WP_SATCHEL, 0, 1, qfalse);        // Big Bang \o/
				AddWeaponToPlayer(client, WP_SATCHEL_DET, 0, 0, qfalse);    // Big Red Button for tha Big Bang
			}
		}

		switch (client->sess.sessionTeam)
		{
		case TEAM_AXIS:
			switch (pc)
			{
			case PC_SOLDIER:
				if (client->sess.skill[SK_HEAVY_WEAPONS] >= 4 && client->sess.playerWeapon2 == WP_MP40)
				{
					AddWeaponToPlayer(client, WP_MP40, 2 * (GetAmmoTableData(WP_MP40)->defaultStartingAmmo), GetAmmoTableData(WP_MP40)->defaultStartingClip, qtrue);
				}
				else if (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && client->sess.playerWeapon2 == WP_AKIMBO_LUGER)
				{
					client->ps.ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(WP_AKIMBO_LUGER))] = GetAmmoTableData(WP_AKIMBO_LUGER)->defaultStartingClip;
					AddWeaponToPlayer(client, WP_AKIMBO_LUGER, GetAmmoTableData(WP_AKIMBO_LUGER)->defaultStartingAmmo, GetAmmoTableData(WP_AKIMBO_LUGER)->defaultStartingClip, qfalse);
				}
				else
				{
					AddWeaponToPlayer(client, WP_LUGER, GetAmmoTableData(WP_LUGER)->defaultStartingAmmo, GetAmmoTableData(WP_LUGER)->defaultStartingClip, qfalse);
				}
				break;

			case PC_COVERTOPS:
				if (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && (client->sess.playerWeapon2 == WP_AKIMBO_SILENCEDLUGER || client->sess.playerWeapon2 == WP_AKIMBO_LUGER))
				{
					client->ps.ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(WP_AKIMBO_SILENCEDLUGER))] = GetAmmoTableData(WP_AKIMBO_SILENCEDLUGER)->defaultStartingClip;
					AddWeaponToPlayer(client, WP_AKIMBO_SILENCEDLUGER, GetAmmoTableData(WP_AKIMBO_SILENCEDLUGER)->defaultStartingAmmo, GetAmmoTableData(WP_AKIMBO_SILENCEDLUGER)->defaultStartingClip, qfalse);
				}
				else
				{
					AddWeaponToPlayer(client, WP_LUGER, GetAmmoTableData(WP_LUGER)->defaultStartingAmmo, GetAmmoTableData(WP_LUGER)->defaultStartingClip, qfalse);
					AddWeaponToPlayer(client, WP_SILENCER, GetAmmoTableData(WP_SILENCER)->defaultStartingAmmo, GetAmmoTableData(WP_SILENCER)->defaultStartingClip, qfalse);
					client->pmext.silencedSideArm = 1;
				}
				break;

			default:
				if (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && client->sess.playerWeapon2 == WP_AKIMBO_LUGER)
				{
					client->ps.ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(WP_AKIMBO_LUGER))] = GetAmmoTableData(WP_AKIMBO_LUGER)->defaultStartingClip;
					AddWeaponToPlayer(client, WP_AKIMBO_LUGER, GetAmmoTableData(WP_AKIMBO_LUGER)->defaultStartingAmmo, GetAmmoTableData(WP_AKIMBO_LUGER)->defaultStartingClip, qfalse);
				}
				else
				{
					AddWeaponToPlayer(client, WP_LUGER, GetAmmoTableData(WP_LUGER)->defaultStartingAmmo, GetAmmoTableData(WP_LUGER)->defaultStartingClip, qfalse);
				}
				break;
			}
			break;
		default:
			switch (pc)
			{
			case PC_SOLDIER:
				if (client->sess.skill[SK_HEAVY_WEAPONS] >= 4 && client->sess.playerWeapon2 == WP_THOMPSON)
				{
					AddWeaponToPlayer(client, WP_THOMPSON, 2 * (GetAmmoTableData(WP_THOMPSON)->defaultStartingAmmo), GetAmmoTableData(WP_THOMPSON)->defaultStartingClip, qtrue);
				}
				else if (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && client->sess.playerWeapon2 == WP_AKIMBO_COLT)
				{
					client->ps.ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(WP_AKIMBO_COLT))] = GetAmmoTableData(WP_AKIMBO_COLT)->defaultStartingClip;
					AddWeaponToPlayer(client, WP_AKIMBO_COLT, GetAmmoTableData(WP_AKIMBO_COLT)->defaultStartingAmmo, GetAmmoTableData(WP_AKIMBO_COLT)->defaultStartingClip, qfalse);
				}
				else
				{
					AddWeaponToPlayer(client, WP_COLT, GetAmmoTableData(WP_COLT)->defaultStartingAmmo, GetAmmoTableData(WP_COLT)->defaultStartingClip, qfalse);
				}
				break;

			case PC_COVERTOPS:
				if (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && (client->sess.playerWeapon2 == WP_AKIMBO_SILENCEDCOLT || client->sess.playerWeapon2 == WP_AKIMBO_COLT))
				{
					client->ps.ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(WP_AKIMBO_SILENCEDCOLT))] = GetAmmoTableData(WP_AKIMBO_SILENCEDCOLT)->defaultStartingClip;
					AddWeaponToPlayer(client, WP_AKIMBO_SILENCEDCOLT, GetAmmoTableData(WP_AKIMBO_SILENCEDCOLT)->defaultStartingAmmo, GetAmmoTableData(WP_AKIMBO_SILENCEDCOLT)->defaultStartingClip, qfalse);
				}
				else
				{
					AddWeaponToPlayer(client, WP_COLT, GetAmmoTableData(WP_COLT)->defaultStartingAmmo, GetAmmoTableData(WP_COLT)->defaultStartingClip, qfalse);
					AddWeaponToPlayer(client, WP_SILENCED_COLT, GetAmmoTableData(WP_SILENCED_COLT)->defaultStartingAmmo, GetAmmoTableData(WP_SILENCED_COLT)->defaultStartingClip, qfalse);
					client->pmext.silencedSideArm = 1;
				}
				break;

			default:
				if (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && client->sess.playerWeapon2 == WP_AKIMBO_COLT)
				{
					client->ps.ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(WP_AKIMBO_COLT))] = GetAmmoTableData(WP_AKIMBO_COLT)->defaultStartingClip;
					AddWeaponToPlayer(client, WP_AKIMBO_COLT, GetAmmoTableData(WP_AKIMBO_COLT)->defaultStartingAmmo, GetAmmoTableData(WP_AKIMBO_COLT)->defaultStartingClip, qfalse);
				}
				else
				{
					AddWeaponToPlayer(client, WP_COLT, GetAmmoTableData(WP_COLT)->defaultStartingAmmo, GetAmmoTableData(WP_COLT)->defaultStartingClip, qfalse);
				}
				break;
			}
		}

		if (pc == PC_SOLDIER)
		{
			if (client->sess.sessionTeam == TEAM_AXIS)
			{
				AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 4, qfalse);
			}
			else
			{
				AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 4, qfalse);
			}
		}
		if (pc == PC_COVERTOPS)
		{
			if (client->sess.sessionTeam == TEAM_AXIS)
			{
				AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 2, qfalse);
			}
			else
			{
				AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 2, qfalse);
			}
		}
	}
	else
	{
		// Knifeonly block
		if (pc == PC_MEDIC)
		{
			AddWeaponToPlayer(client, WP_MEDIC_SYRINGE, 0, 20, qfalse);
			if (client->sess.skill[SK_FIRST_AID] >= 4)
			{
				AddWeaponToPlayer(client, WP_MEDIC_ADRENALINE, 0, 10, qfalse);
			}

		}
		// End Knifeonly stuff -- Ensure that medics get their basic stuff
	}
}

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

// AddMedicTeamBonus
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

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
}

/*
===========
ClientCheckName
============
*/
static void ClientCleanName(const char *in, char *out, int outSize)
{
	int  len = 0, colorlessLen = 0;
	char ch;
	char *p;
	int  spaces = 0;

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

			// don't allow black in a name, period
/*			if( ColorIndex(*in) == 0 ) {
                in++;
                continue;
            }
*/
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

// courtesy of Dens
// FIXME: @IP6
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

// based on userinfocheck.lua and combinedfixes.lua
// FIXME: @IP6
char *CheckUserinfo(int clientNum, char *userinfo)
{
	char *value;
	int  length = strlen(userinfo);
	int  i, slashCount = 0, count = 0;

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

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
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
		                    client->pers.pmoveFixed = atoi(cs_value);
		                else
		                    client->pers.pmoveFixed = 0;
		                break;
		            case TOK_pmove_msec:
		                if ( cs_value[0] )
		                    client->pers.pmoveMsec = atoi(cs_value);
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
			Q_strncpyz(cs_name, cs_value, MAX_NETNAME);
			break;
		case TOK_cl_guid:
			if (!(ent->r.svFlags & SVF_BOT) &&
			    strcmp(client->pers.cl_guid, "unknown") &&
			    strcmp(client->pers.cl_guid, cs_value))
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
		CP("cp \"You cannot assign an empty playername! (Your name is reset)\"");
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

	if (!(g_protect.integer & G_PROTECT_LOCALHOST_REF))
	{
		// check for local client and set ref
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
	}
	else
	{
		if (cs_cg_uinfo[0])
		{
			sscanf(cs_cg_uinfo, "%i %i %i",
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
				client->pers.clientMaxPackets = 30;
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
			trap_SendServerCommand(-1, va("print \"[lof]%s" S_COLOR_WHITE " [lon]renamed to[lof] %s\n\"", oldname,
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
	s = va("n\\%s\\t\\%i\\c\\%i\\lc\\%i\\r\\%i\\m\\%s\\s\\%s\\dn\\%s\\dr\\%i\\w\\%i\\lw\\%i\\sw\\%i\\mu\\%i\\ref\\%i\\u\\%u",
	       client->pers.netname,
	       client->sess.sessionTeam,
	       client->sess.playerType,
	       client->sess.latchPlayerType,
	       client->sess.rank,
	       medalStr,
	       skillStr,
	       client->disguiseNetname,
	       client->disguiseRank,
	       client->sess.playerWeapon,
	       client->sess.latchPlayerWeapon,
	       client->sess.latchPlayerWeapon2,
	       client->sess.muted ? 1 : 0,
	       client->sess.referee,
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

/*
IsFakepConnection

    Ported from the etpro lua module

    FIXME: nice to have in engine too - implement this server side !!!
*/

// This defines the default value and also the minimum value we will allow the client to set...
#define DEF_IP_MAX_CLIENTS  3

// Return the length of the IP address if it has a port, or INT_MAX otherwise
int GetIPLength(char const *ip)
{
	char *start = strchr(ip, ':');

	return (start == NULL ? INT_MAX : start - ip);
}

qboolean CompareIPNoPort(char const *ip1, char const *ip2)
{
	int checkLength = MIN(GetIPLength(ip1), GetIPLength(ip2));

	// Don't compare the port - just the IP
	if (checkLength < INT_MAX && !Q_strncmp(ip1, ip2, checkLength))
	{
		return qtrue;
	}
	else if (checkLength == INT_MAX && !strcmp(ip1, ip2))
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

char *IsFakepConnection(int clientNum, char const *ip, char const *rate)
{
	gentity_t *ent;
	int       theirIPLength;
	int       checkLength;
	int       count      = 1; // we count as the first one
	int       max        = trap_Cvar_VariableIntegerValue("g_ip_max_clients");
	int       myIPLength = GetIPLength(ip);
	int       i;
	char      *theirIP;

	// Default it to DEF_IP_MAX_CLIENTS as the minimum
	if (!max || max <= 0)
	{
		max = DEF_IP_MAX_CLIENTS;
	}

	// validate userinfo to filter out the people blindly using hack code
	if (rate[0] == 0)
	{
		return "Invalid connection!";
	}

	// let localhost ( mostly bots and host player ) be
	// FIXME: move to loop and check for bots ... default of 3 connections should be enough
	// for localhost
	if (!strcmp(ip, "localhost"))
	{
		return NULL;
	}

	// Iterate over each connected client and check the IP, keeping a count of connections
	// from the same IP as this connecting client
	// FIXME: use connected clients
	for (i = 0; i < level.maxclients; ++i)
	{
		// Skip our own slot
		if (i == clientNum)
		{
			continue;
		}

		ent = &g_entities[i];
		if (ent->client == NULL)
		{
			continue;
		}

		theirIP       = ent->client->pers.client_ip;
		theirIPLength = GetIPLength(theirIP);
		checkLength   = MIN(theirIPLength, myIPLength);

		// pers.connected is set correctly for fake players can't rely on userinfo being empty
		if (ent->client->pers.connected != CON_DISCONNECTED)
		{
			qboolean ipMatch = qfalse;

			// Don't compare the port - just the IP
			if (checkLength < INT_MAX && !Q_strncmp(ip, theirIP, checkLength))
			{
				ipMatch = qtrue;
			}
			else if (checkLength == INT_MAX && !strcmp(ip, theirIP))
			{
				ipMatch = qtrue;
			}

			if (ipMatch == qtrue)
			{
				++count;
				if (count > max)
				{
					G_Printf("IsFakepConnection: too many connections from %s\n", ip);
					G_LogPrintf("IsFakepConnection: too many connections from %s\n", ip);
					// TODO: should we drop / ban all connections from this IP?
					return va("Only %d connection%s per IP %s allowed on this server!",
					          max,
					          max == 1 ? "" : "s",
					          max == 1 ? "is" : "are");
				}
			}
		}
	}

	// Ok let them connect
	return NULL;
}

extern const char *country_name[MAX_COUNTRY_NUM];

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	gclient_t  *client;
	gentity_t  *ent          = &g_entities[clientNum];
	const char *userinfo_ptr = NULL;
	char       userinfo[MAX_INFO_STRING];
	char       cs_key[MAX_STRING_CHARS]      = "";
	char       cs_value[MAX_STRING_CHARS]    = "";
	char       cs_ip[MAX_STRING_CHARS]       = "";
	char       cs_password[MAX_STRING_CHARS] = "";
	char       cs_name[MAX_NETNAME]          = "";
	char       cs_guid[MAX_GUID_LENGTH + 1]  = "";
	char       cs_rate[MAX_STRING_CHARS]     = "";
	char       *value;
#ifdef FEATURE_LUA
	char reason[MAX_STRING_CHARS] = "";
#endif

#ifdef USEXPSTORAGE
	ipXPStorage_t *xpBackup;
	int           i;
#endif // USEXPSTORAGE

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
		case TOK_rate:
			Q_strncpyz(cs_rate, cs_value, MAX_STRING_CHARS);
			break;
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

	{
		int i;

		// Avoid ext. ASCII chars in the CS
		for (i = 0; i < strlen(cs_name); ++i)
		{
			if (cs_name[i] < 0)  // extended ASCII chars have values between -128 and 0 (signed char)
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
			if (trap_Cvar_VariableIntegerValue("sv_punkbuster")) // <- FIXME: -> g_protect cvar
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
		// Check for fakep connections
		value = IsFakepConnection(clientNum, cs_ip, cs_rate);
		if (value != NULL)
		{
			// Too many connections from the same IP - don't permit the connection
			return value;
		}

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

	memset(client, 0, sizeof(*client));

	client->pers.connected   = CON_CONNECTING;
	client->pers.connectTime = level.time;

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
			// give bots country flag of the server location g_countryflags 2
			if (isBot && g_countryflags.integer & CF_BOTS)
			{
				char          server_ip[MAX_IP4_LENGTH];
				unsigned int  ret;
				unsigned long ip;

				// get the server flag
				trap_Cvar_VariableStringBuffer("net_ip", server_ip, sizeof(server_ip));
				ip  = GeoIP_addr_to_num(server_ip);
				ret = GeoIP_seek_record(gidb, ip);

				if (ret > 0)
				{
					client->sess.uci = ret;
				}
				else
				{
					// default
					client->sess.uci = 0;
				}
			}
			else
			{
				client->sess.uci = 0; // localhost players
			}
		}
		else
		{
			unsigned long ip = GeoIP_addr_to_num(cs_ip);

			//10.0.0.0/8			[RFC1918]
			//172.16.0.0/12			[RFC1918]
			//192.168.0.0/16		[RFC1918]
			//169.254.0.0/16		[RFC3330] we need this ?
			if (((ip & 0xFF000000) == 0x0A000000) ||
			    ((ip & 0xFFF00000) == 0xAC100000) ||
			    ((ip & 0xFFFF0000) == 0xC0A80000) ||
			    (ip  == 0x7F000001))                    // recognise also 127.0.0.1
			{
				client->sess.uci = 0;
			}
			else
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
		}
	}
	else
	{
		client->sess.uci = 255; //Don't draw anything if DB error
	} // end GeoIP

#ifdef USEXPSTORAGE
	value = Info_ValueForKey(userinfo, "ip");
	if (xpBackup = G_FindXPBackup(value))
	{
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			client->sess.skillpoints[i] = xpBackup->skills[i];
		}
		G_CalcRank(client);
	}
#endif // USEXPSTORAGE

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
	ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	// disabled for bots - see join message ... make cvar ?
	if (firstTime && !(ent->r.svFlags & SVF_BOT))
	{
		if ((g_countryflags.integer & CF_CONNECT) && client->sess.uci > 0 && client->sess.uci < MAX_COUNTRY_NUM)
		{
			trap_SendServerCommand(-1, va("cpm \"%s" S_COLOR_WHITE " connected from %s\n\"", client->pers.netname, country_name[client->sess.uci]));
		}
		else
		{
			trap_SendServerCommand(-1, va("cpm \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));
		}
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

// Scaling for late-joiners of maxlives based on current game time
int G_ComputeMaxLives(gclient_t *cl, int maxRespawns)
{
	float scaled = (float)(maxRespawns - 1) * (1.0f - ((float)(level.time - level.startTime) / (g_timelimit.value * 60000.0f)));
	int   val    = (int)scaled;

	// rain - #102 - don't scale of the timelimit is 0
	if (g_timelimit.value == 0.0)
	{
		return maxRespawns - 1;
	}

	val += ((scaled - (float)val) < 0.5f) ? 0 : 1;
	return(val);
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin(int clientNum)
{
	gentity_t *ent    = g_entities + clientNum;
	gclient_t *client = level.clients + clientNum;
	int       flags;
	int       spawn_count, lives_left;
	int       stat_xp, stat_xp_overflow; // restore xp
	qboolean  inIntermission = (g_gamestate.integer == GS_INTERMISSION && client->ps.pm_type == PM_INTERMISSION) ? qtrue : qfalse;

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

	// restore xp
	stat_xp          = ent->client->ps.stats[STAT_XP];
	stat_xp_overflow = ent->client->ps.stats[STAT_XP_OVERFLOW] ;

	memset(&client->ps, 0, sizeof(client->ps));

	if (ent->client->sess.spectatorState == SPECTATOR_FREE) // restore xp
	{
		ent->client->ps.stats[STAT_XP]          = stat_xp;
		ent->client->ps.stats[STAT_XP_OVERFLOW] = stat_xp_overflow;
	}

	if (inIntermission == qtrue)
	{
		client->ps.pm_type = PM_INTERMISSION;
	}

	client->ps.eFlags                         = flags;
	client->ps.persistant[PERS_SPAWN_COUNT]   = spawn_count;
	client->ps.persistant[PERS_RESPAWNS_LEFT] = lives_left;

	client->pers.complaintClient  = -1;
	client->pers.complaintEndTime = -1;

#ifdef FEATURE_OMNIBOT
	// Omni-bot
	client->sess.botSuicide = qfalse; // make sure this is not set
	client->sess.botPush    = (ent->r.svFlags & SVF_BOT) ? qtrue : qfalse;
#endif

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
			}
		}
	}

	// Start players in limbo mode if they change teams during the match
	if (g_gamestate.integer != GS_INTERMISSION && client->sess.sessionTeam != TEAM_SPECTATOR && (level.time - level.startTime > FRAMETIME * GAME_INIT_FRAMES))
	{
/*	  if( (client->sess.sessionTeam != TEAM_SPECTATOR && (level.time - client->pers.connectTime) > 60000) ||
        ( g_gamestate.integer == GS_PLAYING && ( client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES ) &&
         g_gametype.integer == GT_WOLF_LMS && ( level.numTeamClients[0] > 0 || level.numTeamClients[1] > 0 ) ) ) {*/
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
		trap_SendServerCommand(-1, va("print \"[lof]%s" S_COLOR_WHITE " [lon]entered the game\n\"", client->pers.netname));
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

			value = Info_ValueForKey(userinfo, "ip");
			G_LogPrintf("EnforceMaxLives-IP: %s\n", value);
			AddMaxLivesBan(value);
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

#define MAX_SPAWNPOINTFROMLIST_POINTS   16

gentity_t *SelectSpawnPointFromList(char *list, vec3_t spawn_origin, vec3_t spawn_angles)
{
	char      *pStr       = list, *token;
	gentity_t *spawnPoint = NULL, *trav;
	int       valid[MAX_SPAWNPOINTFROMLIST_POINTS];
	int       numValid = 0;

	memset(valid, 0, sizeof(valid));

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

#if 0 // not used
static char *G_CheckVersion(gentity_t *ent)
{
	// Prevent nasty version mismatches (or people sticking in Q3Aimbot cgames)

	char userinfo[MAX_INFO_STRING];
	char *s;

	trap_GetUserinfo(ent->s.number, userinfo, sizeof(userinfo));
	s = Info_ValueForKey(userinfo, "cg_etVersion");
	if (!s || strcmp(s, GAME_VERSION_DATED))
	{
		return(s);
	}
	return(NULL);
}
#endif

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent, qboolean revived, qboolean teamChange, qboolean restoreHealth)
{
	int                index = ent - g_entities;
	vec3_t             spawn_origin, spawn_angles;
	gclient_t          *client = ent->client;
	int                i;
	clientPersistant_t saved;
	clientSession_t    savedSess;
	int                persistant[MAX_PERSISTANT];
	gentity_t          *spawnPoint;
	int                flags;
	int                savedPing;
	int                savedTeam;
	qboolean           inIntermission = (g_gamestate.integer == GS_INTERMISSION && client->ps.pm_type == PM_INTERMISSION) ? qtrue : qfalse;

	G_UpdateSpawnCounts();

	client->pers.lastSpawnTime            = level.time;
	client->pers.lastBattleSenseBonusTime = level.timeCurrent;
	client->pers.lastHQMineReportTime     = level.timeCurrent;

/*#ifndef _DEBUG
    if( !client->sess.versionOK ) {
        char *clientMismatchedVersion = G_CheckVersion( ent );	// returns NULL if version is identical

        if( clientMismatchedVersion ) {
            trap_DropClient( ent - g_entities, va( "Client/Server game mismatch: '%s/%s'", clientMismatchedVersion, GAME_VERSION_DATED ) );
        } else {
            client->sess.versionOK = qtrue;
        }
    }
#endif*/

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
		// let's just be sure it does the right thing at all times. (well maybe not the right thing, but at least not the bad thing!)
		//if( client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.sessionTeam == TEAM_FREE ) {
		if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
		{
			spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
		}
		else
		{
			// if we have requested a specific spawn point, use it (fixme: what if this will place us inside another character?)
/*			spawnPoint = NULL;
            trap_GetUserinfo( ent->s.number, userinfo, sizeof(userinfo) );
            if( (str = Info_ValueForKey( userinfo, "spawnPoint" )) != NULL && str[0] ) {
                spawnPoint = SelectSpawnPointFromList( str, spawn_origin, spawn_angles );
                if (!spawnPoint) {
                    G_Printf( "WARNING: unable to find spawn point \"%s\" for bot \"%s\"\n", str, ent->aiName );
                }
            }
            //
            if( !spawnPoint ) {*/
			spawnPoint = SelectCTFSpawnPoint(client->sess.sessionTeam, client->pers.teamState.state, spawn_origin, spawn_angles, client->sess.spawnObjectiveIndex);
//			}
		}
	}

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags  = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;
	flags |= (client->ps.eFlags & EF_VOTED);
	// clear everything but the persistant data

	ent->s.eFlags &= ~EF_MOUNTEDTANK;

	saved     = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	savedTeam = client->ps.teamNum;

	if (inIntermission)
	{
		client->ps.pm_type = PM_INTERMISSION;
	}

	for (i = 0 ; i < MAX_PERSISTANT ; i++)
	{
		persistant[i] = client->ps.persistant[i];
	}

	{
		qboolean set = client->maxlivescalced;

		memset(client, 0, sizeof(*client));

		client->maxlivescalced = set;
	}

	client->pers       = saved;
	client->sess       = savedSess;
	client->ps.ping    = savedPing;
	client->ps.teamNum = savedTeam;

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

	client->ps.runSpeedScale    = 0.8;
	client->ps.sprintSpeedScale = 1.1;
	client->ps.crouchSpeedScale = 0.25;
	client->ps.weaponstate      = WEAPON_READY;

	client->pmext.sprintTime   = SPRINTTIME;
	client->ps.sprintExertTime = 0;

	client->ps.friction = 1.0;

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
			bg_playerclass_t *classInfo = BG_PlayerClassForPlayerState(&ent->client->ps);
			client->sess.latchPlayerWeapon = classInfo->classWeapons[0];
			update                         = qtrue;
		}

		if (client->sess.playerWeapon != client->sess.latchPlayerWeapon)
		{
			client->sess.playerWeapon = client->sess.latchPlayerWeapon;
			update                    = qtrue;
		}

		if (G_IsWeaponDisabled(ent, client->sess.playerWeapon))
		{
			bg_playerclass_t *classInfo = BG_PlayerClassForPlayerState(&ent->client->ps);
			client->sess.playerWeapon = classInfo->classWeapons[0];
			update                    = qtrue;
		}

		client->sess.playerWeapon2 = client->sess.latchPlayerWeapon2;

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

	G_UpdateCharacter(client);

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
		MoveClientToIntermission(ent);
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
	ent->client->ps.weapAnim = ((ent->client->ps.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | PM_IdleAnimForWeapon(ent->client->ps.weapon);

	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps, &ent->s, level.time, qtrue);

	G_ResetMarkers(ent);

	// start the scripting system
	if (!revived && client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		// call entity scripting event
		G_Script_ScriptEvent(ent, "playerstart", "");
	}
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect(int clientNum)
{
	gentity_t *ent  = g_entities + clientNum;
	gentity_t *flag = NULL;
	gitem_t   *item = NULL;
	vec3_t    launchvel;
	int       i;

	if (!ent->client)
	{
		return;
	}

#ifdef FEATURE_LUA
	// LUA API callbacks
	G_LuaHook_ClientDisconnect(clientNum);
#endif

#ifdef FEATURE_OMNIBOT
	Bot_Event_ClientDisConnected(clientNum);
#endif

#ifdef USEXPSTORAGE
	G_AddXPBackup(ent);
#endif // USEXPSTORAGE

	G_RemoveClientFromFireteams(clientNum, qtrue, qfalse);
	G_RemoveFromAllIgnoreLists(clientNum);
	G_LeaveTank(ent, qfalse);

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
		if (flag->client->ps.pm_flags & PMF_LIMBO
		    && flag->client->sess.spectatorClient == clientNum)
		{
			Cmd_FollowCycle_f(flag, 1);
		}
	}

	// remove complaint client
	for (i = 0 ; i < level.numConnectedClients ; i++)
	{
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
		TossClientItems(ent);

		// New code for tossing flags
		if (ent->client->ps.powerups[PW_REDFLAG])
		{
			item = BG_FindItem("Red Flag");
			if (!item)
			{
				item = BG_FindItem("Objective");
			}

			ent->client->ps.powerups[PW_REDFLAG] = 0;
		}
		if (ent->client->ps.powerups[PW_BLUEFLAG])
		{
			item = BG_FindItem("Blue Flag");
			if (!item)
			{
				item = BG_FindItem("Objective");
			}

			ent->client->ps.powerups[PW_BLUEFLAG] = 0;
		}

		if (item)
		{
			// fix for suicide drop exploit through walls/gates
			launchvel[0] = 0;    //crandom()*20;
			launchvel[1] = 0;    //crandom()*20;
			launchvel[2] = 0;    //10+random()*10;

			flag                = LaunchItem(item, ent->r.currentOrigin, launchvel, ent - g_entities);
			flag->s.modelindex2 = ent->s.otherEntityNum2;    // FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
			flag->message       = ent->message; // also restore item name

#ifdef FEATURE_OMNIBOT
			// FIXME: see ETPub G_DropItems()
			Bot_Util_SendTrigger(flag, NULL, va("%s dropped.", flag->message), "dropped");
#endif

			// Clear out player's temp copies
			ent->s.otherEntityNum2 = 0;
			ent->message           = NULL;
		}

		// Log stats too
		G_LogPrintf("WeaponStats: %s\n", G_createStats(ent));
	}

	G_LogPrintf("ClientDisconnect: %i\n", clientNum);

	trap_UnlinkEntity(ent);
	ent->s.modelindex                     = 0;
	ent->inuse                            = qfalse;
	ent->classname                        = "disconnected";
	ent->client->pers.connected           = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	i                                     = ent->client->sess.sessionTeam;
	ent->client->sess.sessionTeam         = TEAM_FREE;
	ent->active                           = 0;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	G_verifyMatchState(i);
#ifdef FEATURE_MULTIVIEW
	G_smvAllRemoveSingleClient(ent - g_entities);
#endif
}

// In just the GAME DLL, we want to store the groundtrace surface stuff,
// so we don't have to keep tracing.
void ClientStoreSurfaceFlags(int clientNum, int surfaceFlags)
{
	// Store the surface flags
	g_entities[clientNum].surfaceFlags = surfaceFlags;
}
