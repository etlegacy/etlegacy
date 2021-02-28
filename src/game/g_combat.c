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
 * @file g_combat.c
 */

#include "g_local.h"
#include "g_mdx.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

extern vec3_t muzzleTrace;

/**
 * @brief Adds score to both the client and his team, only used for LMS
 * @param[in,out] ent
 * @param[in] score
 */
void AddKillScore(gentity_t *ent, int score)
{
	if (!ent || !ent->client)
	{
		return;
	}

	// no scoring during pre-match warmup
	if (level.warmupTime)
	{
		return;
	}

	// someone already won
	if (level.lmsWinningTeam)
	{
		return;
	}

	ent->client->ps.persistant[PERS_SCORE]                  += score;
	level.teamScores[ent->client->ps.persistant[PERS_TEAM]] += score;
}

/**
 * @brief Toss the weapon and powerups for the killed player
 * @param[in] self
 */
void TossWeapons(gentity_t *self)
{
	/*gitem_t       *item;
	int         weapon;
	gentity_t   *drop = 0;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.

	// always drop what you were switching to
	if( self->client->ps.weaponstate == WEAPON_DROPPING ) {
	    weapon = self->client->pers.cmd.weapon;
	}

	if( !( COM_BitCheck( self->client->ps.weapons, weapon ) ) ) {
	    return;
	}

	if((self->client->ps.persistant[PERS_HWEAPON_USE])) {
	    return;
	}

	// don't drop these weapon types
	switch( weapon ) {
	    case WP_NONE:
	    case WP_KNIFE:
	    case WP_DYNAMITE:
	    case WP_ARTY:
	    case WP_MEDIC_SYRINGE:
	    case WP_SMOKETRAIL:
	    case WP_MAPMORTAR:
	    case VERYBIGEXPLOSION:
	    case WP_MEDKIT:
	    case WP_BINOCULARS:
	    case WP_PLIERS:
	    case WP_SMOKE_MARKER:
	    case WP_SMOKE_BOMB:
	    case WP_DUMMY_MG42:
	    case WP_MEDIC_ADRENALINE:
	        return;
	    case WP_MORTAR_SET:
	        weapon = WP_MORTAR;
	        break;
	    case WP_K43_SCOPE:
	        weapon = WP_K43;
	        break;
	    case WP_GARAND_SCOPE:
	        weapon = WP_GARAND;
	        break;
	    case WP_FG42_SCOPE:
	        weapon = WP_FG42;
	        break;
	    case WP_M7:
	        weapon = WP_CARBINE;
	        break;
	    case WP_GPG40:
	        weapon = WP_KAR98;
	        break;
	    case WP_MOBILE_MG42_SET:
	        weapon = WP_MOBILE_MG42;
	        break;
	        // browning ... mortar2
	}

	// find the item type for this weapon
	item = BG_FindItemForWeapon( weapon );
	// spawn the item

	drop = Drop_Item( self, item, 0, qfalse );
	drop->count = self->client->ps.ammoclip[BG_FindClipForWeapon(weapon)];
	drop->item->quantity = self->client->ps.ammoclip[BG_FindClipForWeapon(weapon)];*/

	weapon_t primaryWeapon;

	if (g_gamestate.integer == GS_INTERMISSION)
	{
		return;
	}

	if (self->client->sess.playerType == PC_SOLDIER && self->client->sess.skill[SK_HEAVY_WEAPONS] >= 4)
	{
		primaryWeapon = G_GetPrimaryWeaponForClientSoldier(self->client);
	}
	else
	{
		primaryWeapon = G_GetPrimaryWeaponForClient(self->client);
	}

	if (primaryWeapon)
	{
		// drop our primary weapon
		G_DropWeapon(self, primaryWeapon); // FIXME; drop secondary too?!
	}
}

/**
 * @brief LookAtKiller
 * @param[in,out] self
 * @param[in] inflictor
 * @param[in] attacker
 */
void LookAtKiller(gentity_t *self, gentity_t *inflictor, gentity_t *attacker)
{
	vec3_t dir;

	if (attacker && attacker != self)
	{
		VectorSubtract(attacker->s.pos.trBase, self->s.pos.trBase, dir);
	}
	else if (inflictor && inflictor != self)
	{
		VectorSubtract(inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	}
	else
	{
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw(dir);
}

/**
 * @brief GibEntity
 * @param[in,out] self
 * @param[in] killer
 */
void GibEntity(gentity_t *self, int killer)
{
	gentity_t *other = &g_entities[killer];
	vec3_t    dir;

	VectorClear(dir);
	if (other->inuse)
	{
		if (other->client)
		{
			VectorSubtract(self->r.currentOrigin, other->r.currentOrigin, dir);
			VectorNormalize(dir);
		}
		else if (!VectorCompare(other->s.pos.trDelta, vec3_origin))
		{
			VectorNormalize2(other->s.pos.trDelta, dir);
		}
	}

	G_AddEvent(self, EV_GIB_PLAYER, DirToByte(dir));
	self->takedamage = qfalse;
	self->s.eType    = ET_INVISIBLE;
	self->r.contents = 0;
}

/**
 * @brief body_die
 * @param[in,out] self
 * @param inflictor - unused
 * @param attacker - unused
 * @param damage - unused
 * @param meansOfDeath - unused
 */
void body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t meansOfDeath)
{
	if (self->health <= GIB_HEALTH)
	{
		GibEntity(self, ENTITYNUM_WORLD);
	}
}

/**
 * @brief player_die
 * @param[in,out] self
 * @param[in] inflictor
 * @param[in,out] attacker
 * @param[in] damage
 * @param[in] meansOfDeath
 */
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t meansOfDeath)
{
	weapon_t  weap;
	gclient_t *client;
	int       contents = 0, i, killer = ENTITYNUM_WORLD;
	char      *killerName = "<world>";
	qboolean  killedintank = qfalse;
	qboolean  attackerClient, dieFromSameTeam = qfalse;

	//G_Printf( "player_die\n" );

	if (!self->client)
	{
		return;
	}

	// don't broadcast invalid MODs (this shouldn't occure ...)
	if (!IS_VALID_MOD(meansOfDeath))
	{
		G_Printf("Warning: invalid meansOfDeath [%i] in player_die\n", meansOfDeath);
		return;
	}

	switch (meansOfDeath)
	{
	case MOD_FALLING:
		weap = WP_NONE;
		if (self->client->pmext.shoved)
		{
			attacker     = &g_entities[self->client->pmext.pusher];
			meansOfDeath = MOD_SHOVE;
		}
		break;
	case MOD_BACKSTAB: // overwritten for client attackers below
		weap = WP_KNIFE;
		break;
	default:
		weap = GetMODTableData(meansOfDeath)->weaponIcon;
		break;
	}

	attackerClient = (attacker && attacker->client) ? qtrue : qfalse;

	if (attackerClient)
	{
		// dieFromSameTeam is valid client attacker only
		dieFromSameTeam = OnSameTeam(self, attacker) || self->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor);

		self->client->pers.lastkiller_client     = attacker->s.clientNum;
		attacker->client->pers.lastkilled_client = self->s.clientNum;

		if (meansOfDeath == MOD_BACKSTAB)
		{
			weap = attacker->s.weapon;
		}
	}

	// this is used for G_DropLimboHealth()/G_DropLimboAmmo()
	if (!self->client->deathTime)
	{
		self->client->deathTime = level.time;
	}

	// unlagged - backward reconciliation #2
	// make sure the body shows up in the client's current position
	if (g_antilag.integer)
	{
		G_ReAdjustSingleClientPosition(self);
	}
	// unlagged - backward reconciliation #2

	if (attacker == self)
	{
		self->client->pers.playerStats.selfkills++;
		self->isProp = qtrue; // selfkill is teamkill ...
	}
	else if (dieFromSameTeam)
	{
		G_LogTeamKill(attacker, weap);
		self->isProp = qtrue; // teamkilled
	}
	else
	{
		G_LogDeath(self, weap);

		if (attackerClient)
		{
			G_LogKill(attacker, weap);

			if (g_gamestate.integer == GS_PLAYING)
			{
				attacker->client->combatState |= (1 << COMBATSTATE_KILLEDPLAYER);
			}
		}

		self->isProp = qfalse; // no teamkill
	}

	// if we got killed by a landmine, update our map
	if (meansOfDeath == MOD_LANDMINE)
	{
		// if it's an enemy mine, update both teamlists
		mapEntityData_t *mEnt;

		if ((mEnt = G_FindMapEntityData(&mapEntityData[0], inflictor - g_entities)) != NULL)
		{
			G_FreeMapEntityData(&mapEntityData[0], mEnt);
		}

		if ((mEnt = G_FindMapEntityData(&mapEntityData[1], inflictor - g_entities)) != NULL)
		{
			G_FreeMapEntityData(&mapEntityData[1], mEnt);
		}
	}

	{
		mapEntityData_Team_t *teamList = self->client->sess.sessionTeam == TEAM_AXIS ? &mapEntityData[1] : &mapEntityData[0];   // swapped, cause enemy team
		mapEntityData_t      *mEnt     = G_FindMapEntityDataSingleClient(teamList, NULL, self->s.number, -1);

		while (mEnt)
		{
			if (mEnt->type == ME_PLAYER_DISGUISED)
			{
				mapEntityData_t *mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient(teamList, mEnt, self->s.number, -1);

				G_FreeMapEntityData(teamList, mEntFree);
			}
			else
			{
				mEnt = G_FindMapEntityDataSingleClient(teamList, mEnt, self->s.number, -1);
			}
		}
	}

	if (self->tankLink)
	{
		G_LeaveTank(self, qfalse);

		killedintank = qtrue;
	}

	if (self->client->ps.pm_type == PM_DEAD || g_gamestate.integer == GS_INTERMISSION)
	{
		return;
	}

	// death stats handled out-of-band of G_Damage for external calls
	G_addStats(self, attacker, damage, meansOfDeath);

	self->client->ps.pm_type = PM_DEAD;

	G_AddEvent(self, EV_STOPSTREAMINGSOUND, 0);

	if (attacker)
	{
		killer     = attacker->s.number;
		killerName = (attacker->client) ? attacker->client->pers.netname : "<non-client>";
	}

	if (attacker == 0 || killer < 0 || killer >= MAX_CLIENTS)
	{
		killer     = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if (g_gamestate.integer == GS_PLAYING)
	{
#ifdef FEATURE_OMNIBOT
		// send the events
		Bot_Event_Death(self - g_entities, &g_entities[attacker - g_entities], GetMODTableData(meansOfDeath)->modName);
		Bot_Event_KilledSomeone(attacker - g_entities, &g_entities[self - g_entities], GetMODTableData(meansOfDeath)->modName);
#endif

		G_LogPrintf("Kill: %i %i %i: ^7%s^7 killed %s^7 by %s\n", killer, self->s.number, meansOfDeath, killerName, self->client->pers.netname, GetMODTableData(meansOfDeath)->modName);
	}

#ifdef FEATURE_LUA
	if (G_LuaHook_Obituary(self->s.number, killer, meansOfDeath))
	{
		if (self->s.number < 0 || self->s.number >= MAX_CLIENTS)
		{
			G_Error("G_LuaHook_Obituary: target out of range");
		}
	}
#endif

	{
		// broadcast the death event to everyone
		gentity_t *ent;

		ent = G_TempEntityNotLinked(EV_OBITUARY);

		ent->s.eventParm       = meansOfDeath;
		ent->s.otherEntityNum  = self->s.number;
		ent->s.otherEntityNum2 = killer;
		ent->r.svFlags         = SVF_BROADCAST; // send to everyone
		ent->s.weapon          = weap;
	}

	self->enemy = attacker;

	// Make sure covert ops lose their disguises
	self->client->ps.powerups[PW_OPS_DISGUISED] = 0;
	self->client->disguiseClientNum             = -1;

	self->client->ps.persistant[PERS_KILLED]++;

	// if player is holding ticking grenade, drop it
	if (self->client->ps.grenadeTimeLeft)
	{
		vec3_t launchvel, launchspot;

		launchvel[0] = crandom();
		launchvel[1] = crandom();
		launchvel[2] = random();
		VectorScale(launchvel, 160, launchvel);
		VectorCopy(self->r.currentOrigin, launchspot);
		launchspot[2] += 40;

		// fixes premature grenade explosion, ta bani ;)
		fire_missile(self, launchspot, launchvel, self->s.weapon);

		// decrease ammo
		if (GetWeaponTableData(self->client->ps.weapon)->type & WEAPON_TYPE_GRENADE)
		{
			// decrease ammo on hand
			self->client->ps.ammoclip[GetWeaponTableData(self->client->ps.weapon)->ammoIndex]--;

			// check ammo in reserve
			if (!self->client->ps.ammo[GetWeaponTableData(self->client->ps.weapon)->ammoIndex])
			{
				// remove nade from weapon bank
				COM_BitClear(self->client->ps.weapons, self->client->ps.weapon);
			}
		}
	}

	if (attackerClient)
	{
		attacker->client->pers.lastkilled_client = self->s.clientNum;

		if (attacker == self || dieFromSameTeam)
		{
			// Complaint lodging
			if (attacker != self && level.warmupTime <= 0 && g_gamestate.integer == GS_PLAYING)
			{
				if (attacker->client->pers.localClient)
				{
					if (attacker->r.svFlags & SVF_BOT)
					{
						trap_SendServerCommand(self - g_entities, "complaint -5");
					}
					else
					{
						trap_SendServerCommand(self - g_entities, "complaint -4");
					}
				}
				else
				{
					if (meansOfDeath != MOD_CRUSH_CONSTRUCTION && meansOfDeath != MOD_CRUSH_CONSTRUCTIONDEATH && meansOfDeath != MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER)
					{
						if (g_complaintlimit.integer)
						{
							if (!(meansOfDeath == MOD_LANDMINE && (g_disableComplaints.integer & TKFL_MINES)) &&
							    !((meansOfDeath == MOD_ARTY || meansOfDeath == MOD_AIRSTRIKE) && (g_disableComplaints.integer & TKFL_AIRSTRIKE)) &&
							    !((meansOfDeath == MOD_MORTAR || meansOfDeath == MOD_MORTAR2) && (g_disableComplaints.integer & TKFL_MORTAR)))
							{
								trap_SendServerCommand(self - g_entities, va("complaint %i", attacker->s.number));
								if (meansOfDeath != MOD_DYNAMITE || !(inflictor->etpro_misc_1 & 1))   // do not allow complain when tked by dynamite on objective
								{
									self->client->pers.complaintClient  = attacker->s.clientNum;
									self->client->pers.complaintEndTime = level.time + 20500;
								}
							}
						}
					}
				}
			}

			// high penalty to offset medic heal
			if (g_gametype.integer == GT_WOLF_LMS)
			{
				AddKillScore(attacker, -3);
			}
		}
		else
		{
			if (g_gametype.integer == GT_WOLF_LMS)
			{
				if (level.firstbloodTeam == -1)
				{
					level.firstbloodTeam = attacker->client->sess.sessionTeam;
				}

				AddKillScore(attacker, 1);
			}

			attacker->client->lastKillTime = level.time;
		}
	}
	else
	{
		if (g_gametype.integer == GT_WOLF_LMS)
		{
			AddKillScore(self, -1);
		}
	}

	// prepare scoreboard
	CalculateRanks();

	G_DropItems(self);

	// send a fancy "MEDIC!" scream.  Sissies, ain' they?
	if (self->health > GIB_HEALTH &&
	    !GetMODTableData(meansOfDeath)->noYellMedic && // these mods gib -> no fancy scream
	    !killedintank &&
	    self->waterlevel < 3)
	{
		G_AddEvent(self, EV_MEDIC_CALL, 0);
#ifdef FEATURE_OMNIBOT
		// ATM: only register the goal if the target isn't in water.
		if (self->waterlevel <= 1)
		{
			Bot_AddFallenTeammateGoals(self, self->client->sess.sessionTeam);
		}
#endif
	}

	Cmd_Score_f(self);          // show scores

	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for (i = 0; i < level.numConnectedClients; i++)
	{
		client = &level.clients[level.sortedClients[i]];

		if (client->pers.connected != CON_CONNECTED)
		{
			continue;
		}
		if (client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			continue;
		}

		if (client->sess.spectatorClient == self->s.number)
		{
			Cmd_Score_f(g_entities + level.sortedClients[i]);
		}
	}

	self->takedamage = qtrue;   // can still be gibbed
	self->r.contents = CONTENTS_CORPSE;

	self->s.powerups  = 0;
	self->s.loopSound = 0;

	self->client->limboDropWeapon = self->s.weapon; // store this so it can be dropped in limbo

	LookAtKiller(self, inflictor, attacker);
	self->client->ps.viewangles[0] = 0;
	self->client->ps.viewangles[2] = 0;

	self->r.maxs[2]          = DEAD_BODYHEIGHT_BBOX;     // so bodies don't clip into world, was crouchMaxZ => 24
	self->client->ps.maxs[2] = DEAD_BODYHEIGHT_BBOX;     // so bodies don't clip into world, was crouchMaxZ => 24
	trap_LinkEntity(self);

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	if (self->health > GIB_HEALTH)
	{
		self->client->respawnTime = level.timeCurrent + 800;
	}

	Com_Memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

	// never gib in a nodrop
	// FIXME: contents is always 0 here
	if (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP))
	{
		GibEntity(self, killer);
	}
	else if (meansOfDeath != MOD_SWAP_PLACES)
	{
		// normal death
		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if (self->health <= GIB_HEALTH)
		{
			self->health = GIB_HEALTH + 1;
		}

		// set enemy weapon
		BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_WEAPON, weap, qtrue);

		// set enemy location
		BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_POSITION, 0, qfalse);

		// play specific anim on suicide
		BG_UpdateConditionValue(self->s.number, ANIM_COND_SUICIDE, meansOfDeath == MOD_SUICIDE, qtrue);

		// FIXME: add POSITION_RIGHT, POSITION_LEFT
		if (infront(self, inflictor))
		{
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_POSITION, POSITION_INFRONT, qtrue);
		}
		else
		{
			BG_UpdateConditionValue(self->s.number, ANIM_COND_ENEMY_POSITION, POSITION_BEHIND, qtrue);
		}

		self->client->ps.pm_time = BG_AnimScriptEvent(&self->client->ps, self->client->pers.character->animModelInfo, ANIM_ET_DEATH, qfalse, qtrue);

		// record the death animation to be used later on by the corpse
		self->client->torsoDeathAnim = self->client->ps.torsoAnim;
		self->client->legsDeathAnim  = self->client->ps.legsAnim;
		self->client->deathAnimTime  = level.time + self->client->ps.pm_time;

		// the body can still be gibbed
		self->die = body_die;
	}

	// don't fade our own satchel if suicide with it, explosion effect will not be done
	if (meansOfDeath != MOD_SATCHEL || attacker != self)
	{
		G_FadeItems(self, MOD_SATCHEL);
	}

	CalculateRanks();

	// automatically go to limbo from tank
	if (killedintank)
	{
#ifdef FEATURE_SERVERMDX
		self->client->deathAnim = qfalse;    // add no animation time
#endif
		limbo(self, qfalse);   // but no corpse
	}
	else if (meansOfDeath == MOD_SUICIDE)
	{
#ifdef FEATURE_SERVERMDX
		self->client->deathAnim = qtrue;    // add animation time
#endif
		limbo(self, qtrue);
	}
	else if (g_gametype.integer == GT_WOLF_LMS)
	{
#ifdef FEATURE_SERVERMDX
		self->client->deathAnim = qfalse;    // add no animation time
#endif
		if (!G_CountTeamMedics(self->client->sess.sessionTeam, qtrue))
		{
			limbo(self, qtrue);
		}
	}
	else
	{
#ifdef FEATURE_SERVERMDX
		self->client->deathAnim = qtrue;    // add animation time
#endif
	}
}

#define REALHEAD_HEAD 1

/**
 * @brief G_BuildHead
 * @param[in] ent
 * @param[in] refent
 * @param[in] newRefent
 * @return
 */
gentity_t *G_BuildHead(gentity_t *ent, grefEntity_t *refent, qboolean newRefent)
{
	gentity_t     *head;
	orientation_t orientation;

	head            = G_Spawn();
	head->classname = "head"; // see also ET_TEMPHEAD

	VectorSet(head->r.mins, -6, -6, -2);   // changed this z from -12 to -6 for crouching, also removed standing offset
	VectorSet(head->r.maxs, 6, 6, 10);     // changed this z from 0 to 6

#ifdef FEATURE_SERVERMDX
	if (g_realHead.integer & REALHEAD_HEAD)
	{
		// zinx - realistic hitboxes
		if (newRefent)
		{
			mdx_gentity_to_grefEntity(ent, refent, ent->timeShiftTime ? ent->timeShiftTime : level.time);
		}
		mdx_head_position(ent, refent, orientation.origin);

		G_SetOrigin(head, orientation.origin);

		VectorSet(head->r.mins, -6, -6, -6);
		VectorSet(head->r.maxs, 6, 6, 6);
	}
	else if (trap_GetTag(ent->s.number, 0, "tag_head", &orientation))
#else
	if (trap_GetTag(ent->s.number, 0, "tag_head", &orientation))
#endif
	{
		G_SetOrigin(head, orientation.origin);
	}
	else
	{
		float  height, dest;
		vec3_t v, angles, forward, up, right;

		VectorClear(v);
		G_SetOrigin(head, ent->r.currentOrigin);

		if (ent->client->ps.eFlags & EF_PRONE)
		{
			height = ent->client->ps.viewheight - 60;
		}
		else if (ent->client->ps.eFlags & EF_DEAD)
		{
			height = ent->client->ps.viewheight - 64;
		}
		else if (ent->client->ps.pm_flags & PMF_DUCKED)          // closer fake offset for 'head' box when crouching
		{
			height = ent->client->ps.crouchViewHeight - 12;
		}
		else
		{
			height = ent->client->ps.viewheight;
		}

		// this matches more closely with WolfMP models
		VectorCopy(ent->client->ps.viewangles, angles);
		if (angles[PITCH] > 180)
		{
			dest = (-360 + angles[PITCH]) * 0.75f;
		}
		else
		{
			dest = angles[PITCH] * 0.75f;
		}
		angles[PITCH] = dest;

		// the angles need to be clamped for prone
		// or the head entity will be underground or
		// far too tall
		if ((ent->client->ps.eFlags & EF_PRONE))
		{
			angles[PITCH] = -10;
		}

		AngleVectors(angles, forward, right, up);
		if (ent->client->ps.eFlags & EF_PRONE)
		{
			VectorScale(forward, 24, v);
		}
		else if (ent->client->ps.eFlags & EF_DEAD)
		{
			VectorScale(forward, -26, v);
			VectorMA(v, 5.0f, right, v);
		}
		else
		{
			VectorScale(forward, 5, v);
			VectorMA(v, 5.0f, right, v);
		}
		VectorMA(v, 18, up, v);

		VectorAdd(v, head->r.currentOrigin, head->r.currentOrigin);
		head->r.currentOrigin[2] += height / 2;
	}

	VectorCopy(head->r.currentOrigin, head->s.origin);
	VectorCopy(ent->r.currentAngles, head->s.angles);
	VectorCopy(head->s.angles, head->s.apos.trBase);
	VectorCopy(head->s.angles, head->s.apos.trDelta);

	head->clipmask   = CONTENTS_SOLID;
	head->r.contents = CONTENTS_SOLID;
	head->parent     = ent;
	head->s.eType    = ET_TEMPHEAD;

	trap_LinkEntity(head);

	return head;
}

/**
 * @brief G_BuildLeg
 * @param[in] ent
 * @param[in] refent
 * @param[in] newRefent
 * @return
 */
gentity_t *G_BuildLeg(gentity_t *ent, grefEntity_t *refent, qboolean newRefent)
{
	gentity_t *leg;
	vec3_t    org;

	if (!(ent->client->ps.eFlags & (EF_PRONE | EF_DEAD)))
	{
		return NULL;
	}

	leg            = G_Spawn();
	leg->classname = "leg"; // see also ET_TEMPLEG

#ifdef FEATURE_SERVERMDX
	if (g_realHead.integer & REALHEAD_HEAD)
	{
		// zinx - realistic hitboxes
		if (newRefent)
		{
			mdx_gentity_to_grefEntity(ent, refent, ent->timeShiftTime ? ent->timeShiftTime : level.time);
		}
		mdx_legs_position(ent, refent, org);

		org[2] += ent->client->pmext.proneLegsOffset;
		org[2] -= (playerlegsProneMins[2] + playerlegsProneMaxs[2]) * 0.5f;

	}
	else
#endif
	{
		vec3_t flatforward;

		AngleVectors(ent->client->ps.viewangles, flatforward, NULL, NULL);
		flatforward[2] = 0;
		VectorNormalizeFast(flatforward);
		if (ent->client->ps.eFlags & EF_PRONE)
		{
			org[0] = ent->r.currentOrigin[0] + flatforward[0] * -32;
			org[1] = ent->r.currentOrigin[1] + flatforward[1] * -32;
		}
		else
		{
			org[0] = ent->r.currentOrigin[0] + flatforward[0] * 32;
			org[1] = ent->r.currentOrigin[1] + flatforward[1] * 32;
		}
		org[2] = ent->r.currentOrigin[2] + ent->client->pmext.proneLegsOffset;
	}

	G_SetOrigin(leg, org);

	VectorCopy(leg->r.currentOrigin, leg->s.origin);

	VectorCopy(playerlegsProneMins, leg->r.mins);
	VectorCopy(playerlegsProneMaxs, leg->r.maxs);
	leg->clipmask   = CONTENTS_SOLID;
	leg->r.contents = CONTENTS_SOLID;
	leg->parent     = ent;
	leg->s.eType    = ET_TEMPLEGS;

	trap_LinkEntity(leg);

	return leg;
}

/**
 * @brief IsHeadShot
 * @param[in] targ
 * @param[in] dir
 * @param[in] point
 * @param[in] mod
 * @param[in] refent
 * @param[in] newRefent
 * @return
 */
qboolean IsHeadShot(gentity_t *targ, vec3_t dir, vec3_t point, meansOfDeath_t mod, grefEntity_t *refent, qboolean newRefent)
{
	gentity_t *head;
	trace_t   tr;
	vec3_t    start, end;
	gentity_t *traceEnt;

	// not a player or critter so bail
	if (!(targ->client))
	{
		return qfalse;
	}

	// no hs for corpses, we don't want to gib too fast in case of multi HS in row.
	// it could broke revive mechanics as the player have less chance to be revived
	// after getting wounded. So, there is not head hit box on wounded player.
	if (targ->health <= 0)
	{
		return qfalse;
	}

	if (!GetMODTableData(mod)->isHeadshot)
	{
		return qfalse;
	}

	head = G_BuildHead(targ, refent, newRefent);

	// trace another shot see if we hit the head
	VectorCopy(point, start);
	VectorMA(start, 64, dir, end);
	trap_Trace(&tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT);

	traceEnt = &g_entities[tr.entityNum];

	if (traceEnt == head)
	{
		if (g_debugBullets.integer >= 3)     // show hit player head bb
		{
			G_RailBox(head->r.currentOrigin, head->r.mins, head->r.maxs, tv(1.f, 0.f, 0.f), head->s.number | HITBOXBIT_HEAD);

			// show headshot trace
			// end the headshot trace at the head box if it hits
			if (tr.fraction != 1.f)
			{
				VectorMA(start, (tr.fraction * 64.f), dir, end);
			}
			G_RailTrail(start, end, tv(1.f, 0.f, 0.f));
		}

		if (g_antilag.integer)
		{
			// Why??
			// Because we are overwriting flag for head shot registration
			// and we don't want to see the helmet pop back after each HS
			G_ReAdjustSingleClientPosition(targ);
		}

		G_FreeEntity(head);
		return qtrue;
	}

	G_FreeEntity(head);
	return qfalse;
}

/**
 * @brief IsLegShot
 * @param[in] targ
 * @param[in] dir
 * @param[in] point
 * @param[in] mod
 * @param[in] refent
 * @param[in] newRefent
 * @return
 */
qboolean IsLegShot(gentity_t *targ, vec3_t dir, vec3_t point, meansOfDeath_t mod, grefEntity_t *refent, qboolean newRefent)
{
	gentity_t *leg;

	if (!(targ->client))
	{
		return qfalse;
	}

	if (targ->health <= 0)
	{
		return qfalse;
	}

	if (!point)
	{
		return qfalse;
	}

	if (!GetMODTableData(mod)->isHeadshot)
	{
		return qfalse;
	}

	leg = G_BuildLeg(targ, refent, newRefent);   // legs are built only if ent->client->ps.eFlags & EF_PRONE is set?!

	if (leg)
	{
		gentity_t *traceEnt;
		vec3_t    start, end;
		trace_t   tr;

		// trace another shot see if we hit the legs
		VectorCopy(point, start);
		VectorMA(start, 64, dir, end);
		trap_Trace(&tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT);

		traceEnt = &g_entities[tr.entityNum];

		if (traceEnt == leg)
		{
			if (g_debugBullets.integer >= 3)     // show hit player leg bb
			{
				G_RailBox(leg->r.currentOrigin, leg->r.mins, leg->r.maxs, tv(1.f, 0.f, 0.f), leg->s.number | HITBOXBIT_LEGS);

				// show headshot trace
				// end the headshot trace at the head box if it hits
				if (tr.fraction != 1.f)
				{
					VectorMA(start, (tr.fraction * 64.f), dir, end);
				}

				G_RailTrail(start, end, tv(1.f, 0.f, 0.f));
			}

			//if (g_antilag.integer)
			//{
			//	G_ReAdjustSingleClientPosition(targ);
			//}

			G_FreeEntity(leg);
			return qtrue;
		}

		G_FreeEntity(leg);
	}
	else
	{
		float height  = point[2] - targ->r.absmin[2];
		float theight = targ->r.absmax[2] - targ->r.absmin[2];

		if (height < (theight * 0.4f))
		{
			// note: this is a hit we don't draw when g_debugBullets.integer >= 3
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief IsArmShot
 * @param[in] targ
 * @param ent - unused
 * @param[in] point
 * @param[in] mod
 * @return
 */
qboolean IsArmShot(gentity_t *targ, gentity_t *ent, vec3_t point, meansOfDeath_t mod)
{
	vec3_t path, view;
	vec_t  dot;

	if (!(targ->client))
	{
		return qfalse;
	}

	if (targ->health <= 0)
	{
		return qfalse;
	}

	if (!GetMODTableData(mod)->isHeadshot)
	{
		return qfalse;
	}

	VectorSubtract(targ->client->ps.origin, point, path);
	path[2] = 0;

	AngleVectors(targ->client->ps.viewangles, view, NULL, NULL);
	view[2] = 0;

	VectorNormalize(path);

	dot = DotProduct(path, view);

	if (dot > 0.4f || dot < -0.75f)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @var refent
 * @brief This variable needs to be here in order for isHeadShot() to access it..
 */
static grefEntity_t refent;

/**
 * @brief G_Damage
 *
 * @param[in,out] targ        entity that is being damaged
 * @param[in,out] inflictor   entity that is causing the damage
 * @param[in,out] attacker    entity that caused the inflictor to damage targ
 *     example: targ=monster, inflictor=rocket, attacker=player
 *
 * @param[in] dir         direction of the attack for knockback
 * @param[in] point       point at which the damage is being inflicted, used for headshots
 * @param[in] damage      amount of damage being inflicted
 *
 * @param[in] dflags      these flags are used to control how T_Damage works
 *     DAMAGE_RADIUS           damage was indirect (from a nearby explosion)
 *     DAMAGE_NO_ARMOR         armor does not protect from this damage
 *     DAMAGE_NO_KNOCKBACK     do not affect velocity, just view angles
 *     DAMAGE_NO_PROTECTION    kills godmode, armor, everything
 *
 * @param[in] mod
 *
 * @note inflictor, attacker, dir, and point can be NULL for environmental effects
 *
 * @note This parameter was previously present : knockback   force to be applied against targ as a result of the damage
 *
 */
void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, meansOfDeath_t mod)
{
	int         take;
	int         knockback;
	int			hitEventType = HIT_NONE;
	qboolean    wasAlive, onSameTeam;
	hitRegion_t hr = HR_NUM_HITREGIONS;

	if (!targ->takedamage || targ->entstate == STATE_INVISIBLE || targ->entstate == STATE_UNDERCONSTRUCTION) // invisible entities can't be damaged
	{
		return;
	}

	// DEBUG
	//trap_SendServerCommand( -1, va("print \"%i\n\"", targ->health) );

	// The intermission has already been qualified for, so don't allow any extra scoring.
	// Don't do damage if at warmup and warmupdamage is set to 'None' and the target is a client.
	if (level.intermissionQueued || (g_gamestate.integer != GS_PLAYING && match_warmupDamage.integer == 0 && targ->client))
	{
		return;
	}

	if (!inflictor)
	{
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if (!attacker)
	{
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// was the bot alive before applying any damage?
	wasAlive   = (targ->health > 0);
	onSameTeam = OnSameTeam(attacker, targ);

	// combatstate
	if (targ->client && attacker && attacker->client && attacker != targ)
	{
		if (g_gamestate.integer == GS_PLAYING)
		{
			if (!onSameTeam)
			{
				targ->client->combatState |= (1 << COMBATSTATE_DAMAGERECEIVED);
				if (attacker->client->sess.sessionTeam != TEAM_SPECTATOR)
				{
					attacker->client->combatState |= (1 << COMBATSTATE_DAMAGEDEALT);
				}
			}
		}
	}

	if (targ->waterlevel >= 3 && mod == MOD_FLAMETHROWER)
	{
		return;
	}

	switch (targ->s.eType)
	{
	case ET_MOVER:

		// shootable doors / buttons don't actually have any health
		if (!targ->isProp && !targ->scriptName)
		{
			if (targ->use && targ->moverState == MOVER_POS1)
			{
				G_UseEntity(targ, inflictor, attacker);
			}
			return;
		}

		if ((targ->spawnflags & 4) && !targ->isProp)
		{
			if (!GetMODTableData(mod)->isExplosive)
			{
				return;
			}

			// check for team
			if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
			{
				return;
			}
		}

		if ((targ->spawnflags & 1024) && !targ->isProp)
		{
			if (mod != MOD_FLAMETHROWER)
			{
				return;
			}

			// check for team
			if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
			{
				return;
			}
		}
		break;
	case ET_EXPLOSIVE:
		if (targ->parent && GetMODTableData(mod)->weaponClassForMOD == WEAPON_CLASS_FOR_MOD_DYNAMITE)
		{
			return;
		}

		// check for team
		if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
		{
			return;
		}

		if (GetMODTableData(mod)->weaponClassForMOD < targ->constructibleStats.weaponclass)
		{
			return;
		}
		break;
	case ET_MISSILE:
		if (targ->methodOfDeath == MOD_LANDMINE)
		{
			if (targ->s.modelindex2)
			{
				if (GetMODTableData(mod)->isExplosive)
				{
					mapEntityData_t *mEnt;

					if ((mEnt = G_FindMapEntityData(&mapEntityData[0], targ - g_entities)) != NULL)
					{
						G_FreeMapEntityData(&mapEntityData[0], mEnt);
					}

					if ((mEnt = G_FindMapEntityData(&mapEntityData[1], targ - g_entities)) != NULL)
					{
						G_FreeMapEntityData(&mapEntityData[1], mEnt);
					}

					G_ExplodeMissile(targ);
				}
			}
			return;
		}
		break;
	case ET_CONSTRUCTIBLE:
		if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
		{
			return;
		}

		if (GetMODTableData(mod)->weaponClassForMOD < targ->constructibleStats.weaponclass)
		{
			return;
		}
		// bani - fix #238
		if (mod == MOD_DYNAMITE)
		{
			if (!(inflictor->etpro_misc_1 & 1))
			{
				return;
			}
		}
		break;
	default:
		break;
	}

	if (targ->client)
	{
		if (targ->client->noclip || targ->client->ps.powerups[PW_INVULNERABLE])
		{
			return;
		}
	}

	// check for godmode
	if (targ->flags & FL_GODMODE)
	{
		return;
	}

	if (!dir)
	{
		dflags |= DAMAGE_NO_KNOCKBACK;
	}
	else
	{
		VectorNormalize(dir);
	}

	if ((targ->flags & FL_NO_KNOCKBACK) || (dflags & DAMAGE_NO_KNOCKBACK) ||
	    (targ->client && g_friendlyFire.integer && (onSameTeam || (attacker->client && targ->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor)))))
	{
		knockback = 0;
	}
	else
	{
		knockback = (damage > 200) ? 200 : damage;

		if (dflags & DAMAGE_HALF_KNOCKBACK)
		{
			knockback *= 0.5;
		}

		// set weapons means less knockback
		if (targ->client && (GetWeaponTableData(targ->client->ps.weapon)->type & WEAPON_TYPE_SET))
		{
			knockback *= 0.5;
		}
	}

	// figure momentum add, even if the damage won't be taken
	if (knockback && targ->client)
	{
		vec3_t kvel;
		float  mass = 200;

		VectorScale(dir, g_knockback.value * (float)knockback / mass, kvel);
		VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		// are we pushed? Do not count when already flying ...
		if (attacker && attacker->client && (targ->client->ps.groundEntityNum != ENTITYNUM_NONE || GetMODTableData(mod)->isExplosive))
		{
			targ->client->pmext.shoved = qtrue;
			targ->client->pmext.pusher = attacker - g_entities;
		}

		// MOD_ROCKET removed (now MOD_EXPLOSIVE) which is never targ == attacker
		if (targ == attacker && !(mod != MOD_GRENADE &&
		                          mod != MOD_GRENADE_LAUNCHER &&
		                          mod != MOD_GRENADE_PINEAPPLE &&
		                          mod != MOD_DYNAMITE
		                          && mod != MOD_GPG40 // ?!
		                          && mod != MOD_M7 // ?!
		                          && mod != MOD_LANDMINE
		                          ))
		{
			targ->client->ps.velocity[2] *= 0.25f;
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if (!targ->client->ps.pm_time)
		{
			int t = knockback * 2;

			if (t < 50)
			{
				t = 50;
			}
			if (t > 200)
			{
				t = 200;
			}
			targ->client->ps.pm_time   = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if (!(dflags & DAMAGE_NO_PROTECTION))
	{
		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if (targ != attacker && (onSameTeam || (targ->client && attacker->client && targ->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor))))
		{
			if ((g_gamestate.integer != GS_PLAYING && match_warmupDamage.integer == 1))
			{
				return;
			}
			else if (!g_friendlyFire.integer)
			{
				return;
			}
		}
	}

	// add to the attacker's hit counter (but only if target is a client)
	if (attacker && attacker->client && targ->client  && targ != attacker && targ->health > FORCE_LIMBO_HEALTH &&
	    mod != MOD_SWITCHTEAM && mod != MOD_SWAP_PLACES && mod != MOD_SUICIDE)
	{
		if (onSameTeam || (targ->client->ps.powerups[PW_OPS_DISGUISED] && (g_friendlyFire.integer & 1)))
		{
			hitEventType = HIT_TEAMSHOT;
		}
		else if (!targ->client->ps.powerups[PW_OPS_DISGUISED])
		{
			hitEventType = HIT_BODYSHOT;
		}

		BG_UpdateConditionValue(targ->client->ps.clientNum, ANIM_COND_ENEMY_WEAPON, attacker->client->ps.weapon, qtrue);
	}

	if (damage < 0)
	{
		damage = 0;
	}
	take = damage;

	// adrenaline junkie!
	if (targ->client && targ->client->ps.powerups[PW_ADRENALINE])
	{
		take *= .5f;
	}

	// save some from flak jacket
	if (targ->client && targ->client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 4 && targ->client->sess.playerType == PC_ENGINEER)
	{
		if (GetMODTableData(mod)->isExplosive)
		{
			take -= take * .5f;
		}
	}

	if (IsHeadShot(targ, dir, point, mod, &refent, qtrue))
	{
		// FIXME: also when damage is 0 ?
		if (take * 2 < 50)     // head shots, all weapons, do minimum 50 points damage
		{
			take = 50;
		}
		else
		{
			take *= 2; // sniper rifles can do full-kill (and knock into limbo)

		}
		if (dflags & DAMAGE_DISTANCEFALLOFF)
		{
			vec_t dist;
			float scale;

			dist = VectorDistance(point, muzzleTrace);

			// start at 100% at 1500 units (and before),
			// and go to 20% at 2500 units (and after)

			// 1500 to 2500 -> 0.0 to 1.0
			scale = (dist - 1500.f) / (2500.f - 1500.f);
			// 0.0 to 1.0 -> 0.0 to 0.8
			scale *= 0.8f;
			// 0.0 to 0.8 -> 1.0 to 0.2
			scale = 1.0f - scale;

			// And, finally, cap it.
			if (scale > 1.0f)
			{
				scale = 1.0f;
			}
			else if (scale < 0.2f)
			{
				scale = 0.2f;
			}

			take *= scale;
		}

		if (!(targ->client->ps.eFlags & EF_HEADSHOT))          // only toss hat on first headshot
		{
			G_AddEvent(targ, EV_LOSE_HAT, DirToByte(dir));

			if (mod != MOD_K43_SCOPE && mod != MOD_GARAND_SCOPE)
			{
				take *= .8f;    // helmet gives us some protection
			}
		}

		targ->client->ps.eFlags |= EF_HEADSHOT;

		// Record the headshot
		if (targ->client && attacker && attacker->client
#ifndef DEBUG_STATS
		    && attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam
#endif
		    )
		{
			G_addStatsHeadShot(attacker, mod);

			// Upgrade the hit event to headshot if we have not yet classified it as a teamshot (covertops etc..)
			if(hitEventType != HIT_TEAMSHOT)
			{
				hitEventType = HIT_HEADSHOT;
			}
		}

		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Head Shot\n\"");
		}
		G_LogRegionHit(attacker, HR_HEAD);
		hr = HR_HEAD;

		//BG_UpdateConditionValue(targ->client->ps.clientNum, ANIM_COND_IMPACT_POINT, IMPACTPOINT_HEAD, qtrue);
		//BG_AnimScriptEvent(&targ->client->ps, targ->client->pers.character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
	}
	else if (IsLegShot(targ, dir, point, mod, &refent, qfalse))
	{
		G_LogRegionHit(attacker, HR_LEGS);
		hr = HR_LEGS;
		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Leg Shot\n\"");
		}

		//BG_UpdateConditionValue(targ->client->ps.clientNum, ANIM_COND_IMPACT_POINT, (rand() + 1) ? IMPACTPOINT_KNEE_RIGHT : IMPACTPOINT_KNEE_LEFT, qtrue);
		//BG_AnimScriptEvent(&targ->client->ps, targ->client->pers.character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
	}
	else if (IsArmShot(targ, attacker, point, mod))
	{
		G_LogRegionHit(attacker, HR_ARMS);
		hr = HR_ARMS;
		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Arm Shot\n\"");
		}

		//BG_UpdateConditionValue(targ->client->ps.clientNum, ANIM_COND_IMPACT_POINT, (rand() + 1) ? IMPACTPOINT_SHOULDER_RIGHT : IMPACTPOINT_SHOULDER_LEFT, qtrue);
		//BG_AnimScriptEvent(&targ->client->ps, targ->client->pers.character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
	}
	else if (targ->client && targ->health > 0)
	{
		if (GetMODTableData(mod)->isHeadshot)
		{
			G_LogRegionHit(attacker, HR_BODY);
			hr = HR_BODY;
			if (g_debugBullets.integer)
			{
				trap_SendServerCommand(attacker - g_entities, "print \"Body Shot\n\"");
			}
		}
		else if (GetMODTableData(mod)->isExplosive)
		{
			//BG_UpdateConditionValue(targ->client->ps.clientNum, ANIM_COND_STUNNED, 1, qtrue);
		}

		//BG_AnimScriptEvent(&targ->client->ps, targ->client->pers.character->animModelInfo, ANIM_ET_PAIN, qfalse, qtrue);
	}

#ifndef DEBUG_STATS
	if (g_debugDamage.integer)
#endif
	{
		G_Printf("client:%i health:%i damage:%i mod:%s\n", targ->s.number, targ->health, take, GetMODTableData(mod)->modName);
	}

	if(hitEventType)
	{
		G_AddEvent(attacker, EV_PLAYER_HIT, hitEventType);
	}

#ifdef FEATURE_LUA
	if (G_LuaHook_Damage(targ->s.number, attacker->s.number, take, dflags, mod))
	{
		return;
	}
#endif

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (targ->client)
	{
		if (attacker)
		{
			targ->client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		}
		else
		{
			targ->client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		targ->client->damage_blood     += take;
		targ->client->damage_knockback += knockback;

		if (dir)
		{
			VectorCopy(dir, targ->client->damage_from);
			targ->client->damage_fromWorld = qfalse;
		}
		else
		{
			VectorCopy(targ->r.currentOrigin, targ->client->damage_from);
			targ->client->damage_fromWorld = qtrue;
		}
	}

	if (targ->client)
	{
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod    = mod;
		targ->client->lasthurt_time   = level.time;
		if (onSameTeam && wasAlive && attacker != targ)
		{
			targ->client->pers.lastteambleed_client = attacker->s.number;
			targ->client->pers.lastteambleed_dmg    = take;
		}
	}

	// do the damage
	if (take)
	{
		targ->health -= take;

		// can't gib with bullet weapons
		// - attacker == inflictor can happen in other cases as well! (movers trying to gib things)
		//if ( attacker == inflictor && targ->health <= GIB_HEALTH) {
		if (targ->health <= GIB_HEALTH)
		{
			if (!GetMODTableData(mod)->isExplosive)
			{
				targ->health = GIB_HEALTH + 1;
			}
		}

		// overcome previous chunk of code for making grenades work again
		// if ((take > 190)) // 190 is greater than 2x mauser headshot, so headshots don't gib
		// - only player entities! messes up ents like func_constructibles and func_explosives otherwise
		if (targ->s.number < MAX_CLIENTS && take > 190)
		{
			targ->health = GIB_HEALTH - 1;
		}

		if (targ->s.eType == ET_MOVER && !Q_stricmp(targ->classname, "script_mover"))
		{
			targ->s.dl_intensity = (int)(255.f * (targ->health / (float)targ->count));   // send it to the client
		}

		//G_Printf("health at: %d\n", targ->health);
		if (targ->health <= 0)
		{
			if (targ->client && !wasAlive)
			{
				targ->flags |= FL_NO_KNOCKBACK;
				// special hack to not count attempts for body gibbage
				if (targ->client->ps.pm_type == PM_DEAD)
				{
					G_addStats(targ, attacker, take, mod);
				}

				if ((targ->health < FORCE_LIMBO_HEALTH) && (targ->health > GIB_HEALTH))
				{
					limbo(targ, qtrue);
				}
				if (targ->health <= GIB_HEALTH)
				{
					GibEntity(targ, 0);
				}
			}
			else
			{
				targ->sound1to2 = hr;
				targ->sound2to1 = mod;
				targ->sound2to3 = (dflags & DAMAGE_RADIUS) ? 1 : 0;

				if (targ->client)
				{
					if (G_GetTeamFromEntity(inflictor) != G_GetTeamFromEntity(targ))
					{
						G_AddKillSkillPoints(attacker, mod, hr, (dflags & DAMAGE_RADIUS));
					}
				}

				if (targ->health < -999)
				{
					targ->health = -999;
				}

				targ->enemy     = attacker;
				targ->deathType = mod;

				if (targ->die)
				{
					// Kill the entity.  Note that this funtion can set ->die to another
					// function pointer, so that next time die is applied to the dead body.
					targ->die(targ, inflictor, attacker, take, mod);
					// kill stats in player_die function
				}

				if (targ->s.eType == ET_MOVER && !Q_stricmp(targ->classname, "script_mover") && (targ->spawnflags & 8))
				{
					return; // reseructable script mover doesn't unlink itself but we don't want a second death script to be called
				}

				// if we freed ourselves in death function
				if (!targ->inuse)
				{
					return;
				}

				// entity scripting
				if (targ->health <= 0)       // might have revived itself in death function
				{
					if ((!(targ->r.svFlags & SVF_BOT) && targ->s.eType != ET_CONSTRUCTIBLE && targ->s.eType != ET_EXPLOSIVE) ||
					    (targ->s.eType == ET_CONSTRUCTIBLE && !targ->desstages))                 // call manually if using desstages
					{
						G_Script_ScriptEvent(targ, "death", "");
					}
				}
			}
		}
		else if (targ->pain)
		{
			if (dir)      // had to add this to fix NULL dir crash
			{
				VectorCopy(dir, targ->rotate);
				VectorCopy(point, targ->pos3);   // this will pass loc of hit
			}
			else
			{
				VectorClear(targ->rotate);
				VectorClear(targ->pos3);
			}

			targ->pain(targ, attacker, take, point);
		}
		else
		{
			// update weapon/dmg stats
			G_addStats(targ, attacker, take, mod);
		}

		// entity scripting
		G_Script_ScriptEvent(targ, "pain", va("%d %d", targ->health, targ->health + take));

#ifdef FEATURE_OMNIBOT
		// record bot pain
		if (targ->s.number < level.maxclients)
		{
			// notify omni-bot framework
			Bot_Event_TakeDamage(targ - g_entities, attacker);
		}
#endif

		// this needs to be done last, incase the health is altered in one of the event calls
		if (targ->client)
		{
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
	}
}

void G_RailTrail(vec_t *start, vec_t *end, vec_t *color)
{
	gentity_t *temp;

	temp = G_TempEntity(start, EV_RAILTRAIL);
	VectorCopy(end, temp->s.origin2);

	temp->s.dmgFlags  = 0;
	temp->s.angles[0] = (int)(color[0] * 255);
	temp->s.angles[1] = (int)(color[1] * 255);
	temp->s.angles[2] = (int)(color[2] * 255);
	temp->s.density   = -1;
}

/**
 * @brief G_RailBox
 * @param[in] origin
 * @param[in] mins
 * @param[in] maxs
 * @param[in] color
 * @param[in] index
 */
void G_RailBox(vec_t *origin, vec_t *mins, vec_t *maxs, vec_t *color, int index)
{
	vec3_t    b1;
	vec3_t    b2;
	gentity_t *temp;

	VectorCopy(origin, b1);
	VectorCopy(origin, b2);
	VectorAdd(b1, mins, b1);
	VectorAdd(b2, maxs, b2);

	temp = G_TempEntity(b1, EV_RAILTRAIL);

	VectorCopy(b2, temp->s.origin2);
	VectorCopy(color, temp->s.angles);
	temp->s.dmgFlags = 1;

	temp->s.angles[0] = (int)(color[0] * 255);
	temp->s.angles[1] = (int)(color[1] * 255);
	temp->s.angles[2] = (int)(color[2] * 255);

	temp->s.effect1Time = index + 1;
}

#define MASK_CAN_DAMAGE     (CONTENTS_SOLID | CONTENTS_BODY)

/**
 * @brief Used for explosions and melee attacks.
 * @param[in] targ
 * @param[in] origin
 * @return qtrue if the inflictor can directly damage the target.
 */
qboolean CanDamage(gentity_t *targ, vec3_t origin)
{
	vec3_t  dest;
	trace_t tr;
	vec3_t  midpoint;
	vec3_t  offsetmins = { -16.f, -16.f, -16.f };
	vec3_t  offsetmaxs = { 16.f, 16.f, 16.f };

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	// - well, um, just check then ...
	if (targ->r.currentOrigin[0] != 0.f || targ->r.currentOrigin[1] != 0.f || targ->r.currentOrigin[2] != 0.f)
	{
		VectorCopy(targ->r.currentOrigin, midpoint);

		if (targ->s.eType == ET_MOVER)
		{
			midpoint[2] += 32;
		}
	}
	else
	{
		VectorAdd(targ->r.absmin, targ->r.absmax, midpoint);
		VectorScale(midpoint, 0.5f, midpoint);
	}

	//G_RailTrail( origin, dest );

	trap_Trace(&tr, origin, vec3_origin, vec3_origin, midpoint, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.0f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	if (targ->client)
	{
		VectorCopy(targ->client->ps.mins, offsetmins);
		VectorCopy(targ->client->ps.maxs, offsetmaxs);
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate
	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	// =========================

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.f || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief G_AdjustedDamageVec
 * @param[in] ent
 * @param[in] origin
 * @param[out] vec
 */
void G_AdjustedDamageVec(gentity_t *ent, vec3_t origin, vec3_t vec)
{
	if (!ent->r.bmodel)
	{
		VectorSubtract(ent->r.currentOrigin, origin, vec); // simpler centroid check that doesn't have box alignment weirdness
	}
	else
	{
		int i;

		for (i = 0 ; i < 3 ; i++)
		{
			if (origin[i] < ent->r.absmin[i])
			{
				vec[i] = ent->r.absmin[i] - origin[i];
			}
			else if (origin[i] > ent->r.absmax[i])
			{
				vec[i] = origin[i] - ent->r.absmax[i];
			}
			else
			{
				vec[i] = 0;
			}
		}
	}
}

/**
 * @brief G_RadiusDamage
 * @param[in] origin
 * @param[in] inflictor
 * @param[in] attacker
 * @param[in] damage
 * @param[in] radius
 * @param[in] ignore
 * @param[in] mod
 * @return
 */
qboolean G_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, meansOfDeath_t mod)
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[MAX_GENTITIES];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	vec3_t    dir;
	int       i, e;
	qboolean  hitClient = qfalse;
	float     boxradius;
	vec3_t    dest;
	trace_t   tr;
	vec3_t    midpoint;
	int       flags = DAMAGE_RADIUS;

	if (mod == MOD_SATCHEL || mod == MOD_LANDMINE)
	{
		flags |= DAMAGE_HALF_KNOCKBACK;
	}

	if (radius < 1)
	{
		radius = 1;
	}

	boxradius = M_SQRT2 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for (i = 0 ; i < 3 ; i++)
	{
		mins[i] = origin[i] - boxradius;
		maxs[i] = origin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (e = 0 ; e < level.num_entities ; e++)
	{
		g_entities[e].dmginloop = qfalse;
	}

	for (e = 0 ; e < numListedEntities ; e++)
	{
		ent = &g_entities[entityList[e]];

		if (ent == ignore)
		{
			continue;
		}
		if (!ent->takedamage && (!ent->dmgparent || !ent->dmgparent->takedamage)
		    && !(mod == MOD_DYNAMITE && ent->s.weapon == WP_DYNAMITE))
		{
			continue;
		}

		G_AdjustedDamageVec(ent, origin, v);

		dist = VectorLength(v);
		if (dist >= radius)
		{
			continue;
		}

		// dyno chaining
		// only if within blast radius and both on the same objective or both or no objectives
		if (mod == MOD_DYNAMITE && ent->s.weapon == WP_DYNAMITE)
		{
			G_DPrintf("dyno chaining: inflictor: %p, ent: %p\n", inflictor->onobjective, ent->onobjective);

			if (inflictor->onobjective == ent->onobjective)
			{
				if (g_dynamiteChaining.integer & DYNAMITECHAINING_FREE)
				{
					// free the other dynamite now too since they are peers
					ent->nextthink = level.time;
					ent->think     = G_ChainFree;
				}
				else
				{
					// blow up the other dynamite now too since they are peers
					// set the nextthink just past us by a 1/4 of a second or so
					ent->nextthink = level.time + 250;
				}
			}
		}

		points = damage * (1.0f - dist / radius);

		if (CanDamage(ent, origin))
		{
			if (ent->dmgparent)
			{
				ent = ent->dmgparent;
			}

			if (ent->dmginloop)
			{
				continue;
			}

			if (AccuracyHit(ent, attacker))
			{
				hitClient = qtrue;
			}
			VectorSubtract(ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;

			G_Damage(ent, inflictor, attacker, dir, origin, round(points), flags, mod);
		}
		else
		{
			VectorAdd(ent->r.absmin, ent->r.absmax, midpoint);
			VectorScale(midpoint, 0.5f, midpoint);
			VectorCopy(midpoint, dest);

			trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
			if (tr.fraction < 1.0f)
			{
				VectorSubtract(dest, origin, dest);
				dist = VectorLength(dest);
				if (dist < radius * 0.2f)     // closer than 1/4 dist (actually 1/5)
				{
					if (ent->dmgparent)
					{
						ent = ent->dmgparent;
					}

					if (ent->dmginloop)
					{
						continue;
					}

					if (AccuracyHit(ent, attacker))
					{
						hitClient = qtrue;
					}
					VectorSubtract(ent->r.currentOrigin, origin, dir);
					dir[2] += 24;
					G_Damage(ent, inflictor, attacker, dir, origin, round(points * 0.1f), flags, mod);
				}
			}
		}
	}
	return hitClient;
}

/**
 * @brief Mutation of G_RadiusDamage which lets us selectively damage only clients or only non clients
 * @param[in] origin
 * @param[in] inflictor
 * @param[in] attacker
 * @param[in] damage
 * @param[in] radius
 * @param[in] ignore
 * @param[in] mod
 * @param[in] clientsonly
 * @return
 */
qboolean etpro_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, meansOfDeath_t mod, qboolean clientsonly)
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[MAX_GENTITIES];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	vec3_t    dir;
	int       i, e;
	qboolean  hitClient = qfalse;
	float     boxradius;
	vec3_t    dest;
	trace_t   tr;
	vec3_t    midpoint;
	int       flags = DAMAGE_RADIUS;

	if (mod == MOD_SATCHEL || mod == MOD_LANDMINE)
	{
		flags |= DAMAGE_HALF_KNOCKBACK;
	}

	if (radius < 1)
	{
		radius = 1;
	}

	boxradius = M_SQRT2 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for (i = 0 ; i < 3 ; i++)
	{
		mins[i] = origin[i] - boxradius;
		maxs[i] = origin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (e = 0 ; e < level.num_entities ; e++)
	{
		g_entities[e].dmginloop = qfalse;
	}

	for (e = 0 ; e < numListedEntities ; e++)
	{
		ent = &g_entities[entityList[e]];

		// dyno chaining
		// only if within blast radius and both on the same objective or both or no objectives
		if (mod == MOD_DYNAMITE && ent->s.weapon == WP_DYNAMITE)
		{
			G_DPrintf("dyno chaining: inflictor: %p, ent: %p\n", inflictor->onobjective, ent->onobjective);

			if (inflictor->onobjective == ent->onobjective)
			{
				if (g_dynamiteChaining.integer & DYNAMITECHAINING_FREE)
				{
					// free the other dynamite now too since they are peers
					ent->nextthink = level.time;
					ent->think     = G_ChainFree;
				}
				else
				{
					// blow up the other dynamite now too since they are peers
					// set the nextthink just past us by a 1/4 of a second or so
					ent->nextthink = level.time + 250;
				}
			}
		}

		if (ent == ignore)
		{
			continue;
		}
		if (!ent->takedamage && (!ent->dmgparent || !ent->dmgparent->takedamage))
		{
			continue;
		}

		// need to include corpses in clients only since they
		// will be neglected from G_TempTraceIgnorePlayersAndBodies()
		if (clientsonly && !ent->client && ent->s.eType != ET_CORPSE)
		{
			continue;
		}
		if (!clientsonly && ent->client)
		{
			continue;
		}

		G_AdjustedDamageVec(ent, origin, v);

		dist = VectorLength(v);
		if (dist >= radius)
		{
			continue;
		}

		points = damage * (1.0f - dist / radius);

		if (CanDamage(ent, origin))
		{
			if (ent->dmgparent)
			{
				ent = ent->dmgparent;
			}

			if (ent->dmginloop)
			{
				continue;
			}

			if (AccuracyHit(ent, attacker))
			{
				hitClient = qtrue;
			}
			VectorSubtract(ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;

			G_Damage(ent, inflictor, attacker, dir, origin, round(points), flags, mod);
		}
		else
		{
			VectorAdd(ent->r.absmin, ent->r.absmax, midpoint);
			VectorScale(midpoint, 0.5f, midpoint);
			VectorCopy(midpoint, dest);

			trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
			if (tr.fraction < 1.0f)
			{
				VectorSubtract(dest, origin, dest);
				dist = VectorLength(dest);
				if (dist < radius * 0.2f)     // closer than 1/4 dist
				{
					if (ent->dmgparent)
					{
						ent = ent->dmgparent;
					}

					if (ent->dmginloop)
					{
						continue;
					}

					if (AccuracyHit(ent, attacker))
					{
						hitClient = qtrue;
					}
					VectorSubtract(ent->r.currentOrigin, origin, dir);
					dir[2] += 24;
					G_Damage(ent, inflictor, attacker, dir, origin, round(points * 0.1f), flags, mod);
				}
			}
		}
	}

	return hitClient;
}
