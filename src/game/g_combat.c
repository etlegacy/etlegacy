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
 * @file g_combat.c
 */

#include "g_local.h"
#include "../qcommon/q_shared.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

extern vec3_t muzzleTrace;

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore(gentity_t *ent, int score)
{
	if (!ent || !ent->client)
	{
		return;
	}
	// no scoring during pre-match warmup
	if (g_gamestate.integer != GS_PLAYING)
	{
		return;
	}

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		return;
	}

	//ent->client->ps.persistant[PERS_SCORE] += score;
	ent->client->sess.game_points += score;

	//level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] += score;
	CalculateRanks();
}

/*
============
AddKillScore

Adds score to both the client and his team, only used for playerkills, for lms
============
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

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		ent->client->ps.persistant[PERS_SCORE]                  += score;
		level.teamScores[ent->client->ps.persistant[PERS_TEAM]] += score;
	}
	ent->client->sess.game_points += score;

	CalculateRanks();
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems(gentity_t *self)
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
	    case WP_FG42SCOPE:
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

	primaryWeapon = G_GetPrimaryWeaponForClient(self->client);

	if (primaryWeapon)
	{
		// drop our primary weapon
		G_DropWeapon(self, primaryWeapon);
	}
}

/*
==================
LookAtKiller
==================
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

/*
==================
GibEntity
==================
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

/*
==================
body_die
==================
*/
void body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath)
{
	if (self->health <= GIB_HEALTH)
	{
		GibEntity(self, 0);
	}
}

// these are just for logging, the client prints its own messages
char *modNames[] =
{
	"MOD_UNKNOWN",
	"MOD_MACHINEGUN",
	"MOD_BROWNING",
	"MOD_MG42",
	"MOD_GRENADE",

	// modified wolf weap mods
	"MOD_KNIFE",
	"MOD_LUGER",
	"MOD_COLT",
	"MOD_MP40",
	"MOD_THOMPSON",
	"MOD_STEN",
	"MOD_GARAND",
	"MOD_SILENCER",
	"MOD_FG42",
	"MOD_FG42SCOPE",
	"MOD_PANZERFAUST",
	"MOD_GRENADE_LAUNCHER",
	"MOD_FLAMETHROWER",
	"MOD_GRENADE_PINEAPPLE",

	"MOD_MAPMORTAR",
	"MOD_MAPMORTAR_SPLASH",

	"MOD_KICKED",

	"MOD_DYNAMITE",
	"MOD_AIRSTRIKE",
	"MOD_SYRINGE",
	"MOD_AMMO",
	"MOD_ARTY",

	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_EXPLOSIVE",

	"MOD_CARBINE",
	"MOD_KAR98",
	"MOD_GPG40",
	"MOD_M7",
	"MOD_LANDMINE",
	"MOD_SATCHEL",

	"MOD_SMOKEBOMB",
	"MOD_MOBILE_MG42",
	"MOD_SILENCED_COLT",
	"MOD_GARAND_SCOPE",

	"MOD_CRUSH_CONSTRUCTION",
	"MOD_CRUSH_CONSTRUCTIONDEATH",
	"MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER",

	"MOD_K43",
	"MOD_K43_SCOPE",

	"MOD_MORTAR",

	"MOD_AKIMBO_COLT",
	"MOD_AKIMBO_LUGER",
	"MOD_AKIMBO_SILENCEDCOLT",
	"MOD_AKIMBO_SILENCEDLUGER",

	"MOD_SMOKEGRENADE",

	"MOD_SWAP_PLACES",

	// keep these 2 entries last
	"MOD_SWITCHTEAM",

	"MOD_SHOVE",

	// MOD_NUM_MODS
};

/*
==================
player_die
==================
*/
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath)
{
	weapon_t  weap = BG_WeaponForMOD(meansOfDeath);
	gclient_t *client;
	gitem_t   *item           = NULL;
	int       contents        = 0, i, killer = ENTITYNUM_WORLD;
	char      *killerName     = "<world>";
	qboolean  nogib           = qtrue;
	qboolean  killedintank    = qfalse;
	qboolean  dieFromSameTeam = OnSameTeam(self, attacker) || (attacker->client && self->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor));

	//G_Printf( "player_die\n" );

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
	}

	// this is used for G_DropLimboHealth()/G_DropLimboAmmo()
	if (!self->client->deathTime)
	{
		self->client->deathTime = level.time;
	}

	if (attacker == self)
	{
		if (self->client)
		{
			self->client->pers.playerStats.suicides++;
		}
	}
	else if (dieFromSameTeam)
	{
		G_LogTeamKill(attacker, weap);
	}
	else
	{
		G_LogDeath(self, weap);
		G_LogKill(attacker, weap);

		if (g_gamestate.integer == GS_PLAYING)
		{
			if (attacker->client)
			{
				attacker->client->combatState |= (1 << COMBATSTATE_KILLEDPLAYER);
			}
		}
	}

	if (!dieFromSameTeam)
	{
		self->isProp = qfalse;  // were we teamkilled or not?
	}
	else
	{
		self->isProp = qtrue;
	}

	// if we got killed by a landmine, update our map
	if (self->client && meansOfDeath == MOD_LANDMINE)
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
		char *obit;

		if (meansOfDeath < 0 || meansOfDeath >= sizeof(modNames) / sizeof(modNames[0]))
		{
			obit = "<bad obituary>";
		}
		else
		{
			obit = modNames[meansOfDeath];
		}

#ifdef FEATURE_OMNIBOT
		// send the events
		Bot_Event_Death(self - g_entities, &g_entities[attacker - g_entities], obit);
		Bot_Event_KilledSomeone(attacker - g_entities, &g_entities[self - g_entities], obit);
#endif

		G_LogPrintf("Kill: %i %i %i: %s killed %s by %s\n", killer, self->s.number, meansOfDeath, killerName, self->client->pers.netname, obit);
	}

#ifdef FEATURE_LUA
	// pheno: Lua API callbacks
	// IRATA NQ like rework (Etpro style)
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
		gentity_t *ent = G_TempEntityNotLinked(EV_OBITUARY);

		ent->s.eventParm       = meansOfDeath;
		ent->s.otherEntityNum  = self->s.number;
		ent->s.otherEntityNum2 = killer;
		ent->r.svFlags         = SVF_BROADCAST; // send to everyone
	}

	self->enemy = attacker;

	// Make sure covert ops lose their disguises
	self->client->ps.powerups[PW_OPS_DISGUISED] = 0;

	self->client->ps.persistant[PERS_KILLED]++;

	// if player is holding ticking grenade, drop it
	if ((self->client->ps.grenadeTimeLeft) && (self->s.weapon != WP_DYNAMITE) && (self->s.weapon != WP_LANDMINE) && (self->s.weapon != WP_SATCHEL))
	{
		vec3_t launchvel, launchspot;

		launchvel[0] = crandom();
		launchvel[1] = crandom();
		launchvel[2] = random();
		VectorScale(launchvel, 160, launchvel);
		VectorCopy(self->r.currentOrigin, launchspot);
		launchspot[2] += 40;

		{
			// fixes premature grenade explosion, ta bani ;)
			gentity_t *m = fire_grenade(self, launchspot, launchvel, self->s.weapon);
			m->damage = 0;
		}
	}

	if (attacker && attacker->client)
	{
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
							    !(meansOfDeath == MOD_MORTAR && (g_disableComplaints.integer & TKFL_MORTAR)))
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
				AddKillScore(attacker, WOLF_FRIENDLY_PENALTY);
			}
		}
		else
		{
			// mostly added as conveneience so we can tweak from the #defines all in one place
			AddScore(attacker, WOLF_FRAG_BONUS);

			if (g_gametype.integer == GT_WOLF_LMS)
			{
				if (level.firstbloodTeam == -1)
				{
					level.firstbloodTeam = attacker->client->sess.sessionTeam;
				}

				AddKillScore(attacker, WOLF_FRAG_BONUS);
			}

			attacker->client->lastKillTime = level.time;
		}
	}
	else
	{
		AddScore(self, -1);

		if (g_gametype.integer == GT_WOLF_LMS)
		{
			AddKillScore(self, -1);
		}
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// drop flag regardless
	if (self->client->ps.powerups[PW_REDFLAG])
	{
		item = BG_FindItem("Red Flag");
		if (!item)
		{
			item = BG_FindItem("Objective");
		}

		self->client->ps.powerups[PW_REDFLAG] = 0;
	}
	if (self->client->ps.powerups[PW_BLUEFLAG])
	{
		item = BG_FindItem("Blue Flag");
		if (!item)
		{
			item = BG_FindItem("Objective");
		}

		self->client->ps.powerups[PW_BLUEFLAG] = 0;
	}

	if (item)
	{
		vec3_t    launchvel = { 0, 0, 0 };
		gentity_t *flag     = LaunchItem(item, self->r.currentOrigin, launchvel, self->s.number);

		flag->s.modelindex2 = self->s.otherEntityNum2; // FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
		flag->message       = self->message; // also restore item name
		// Clear out player's temp copies
		self->s.otherEntityNum2 = 0;
		self->message           = NULL;
	}

	// send a fancy "MEDIC!" scream.  Sissies, ain' they?
	if (self->client != NULL)
	{
		if (self->health > GIB_HEALTH && meansOfDeath != MOD_SUICIDE && meansOfDeath != MOD_SWITCHTEAM && self->waterlevel < 3)
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

	self->r.maxs[2]          = self->client->ps.crouchMaxZ; //% 0;  // so bodies don't clip into world
	self->client->ps.maxs[2] = self->client->ps.crouchMaxZ; //% 0;  // so bodies don't clip into world
	trap_LinkEntity(self);

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.timeCurrent + 800;

	// remove powerups
	// FIXME: but not FLAKJACKET and HELMETSHIELD
	{
		//int flakJacket=self->client->ps.powerups[PW_FLAKJACKET];
		//int helmetArmor=self->client->ps.powerups[PW_HELMETSHIELD];

		memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

		//self->client->ps.powerups[PW_FLAKJACKET]=flakJacket;
		//self->client->ps.powerups[PW_HELMETSHIELD]=helmetArmor;
	}

	// never gib in a nodrop
	// FIXME: contents is always 0 here
	if (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP))
	{
		GibEntity(self, killer);
		nogib = qfalse;
	}

	if (nogib)
	{
		// normal death
		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if (self->health <= GIB_HEALTH)
		{
			self->health = GIB_HEALTH + 1;
		}

		// FIXME: re-enable this for flailing
		/*      if( self->client->ps.groundEntityNum == ENTITYNUM_NONE ) {
		            self->client->ps.pm_flags |= PMF_FLAILING;
		            self->client->ps.pm_time = 750;
		            BG_AnimScriptAnimation( &self->client->ps, ANIM_MT_FLAILING, qtrue );

		            // Face explosion directory
		            {
		                vec3_t angles;

		                vectoangles( self->client->ps.velocity, angles );
		                self->client->ps.viewangles[YAW] = angles[YAW];
		                SetClientViewAngle( self, self->client->ps.viewangles );
		            }
		        } else*/

		self->client->ps.pm_time = BG_AnimScriptEvent(&self->client->ps, self->client->pers.character->animModelInfo, ANIM_ET_DEATH, qfalse, qtrue);

		// record the death animation to be used later on by the corpse
		self->client->torsoDeathAnim = self->client->ps.torsoAnim;
		self->client->legsDeathAnim  = self->client->ps.legsAnim;
		self->client->deathAnimTime  = level.time + self->client->ps.pm_time;

		// the body can still be gibbed
		self->die = body_die;
	}

	if (meansOfDeath == MOD_MACHINEGUN)
	{
		switch (self->client->sess.sessionTeam)
		{
		case TEAM_AXIS:
			level.axisMG42Counter = level.time;
			break;
		case TEAM_ALLIES:
			level.alliesMG42Counter = level.time;
			break;
		default:
			break;
		}
	}

	G_FadeItems(self, MOD_SATCHEL);

	CalculateRanks();

	// automatically go to limbo from tank
	if (killedintank)
	{
		limbo(self, qfalse);   // but no corpse
	}
	else if ((meansOfDeath == MOD_SUICIDE && g_gamestate.integer == GS_PLAYING))
	{
		limbo(self, qtrue);
	}
	else if (g_gametype.integer == GT_WOLF_LMS)
	{
		if (!G_CountTeamMedics(self->client->sess.sessionTeam, qtrue))
		{
			limbo(self, qtrue);
		}
	}
}

qboolean IsHeadShotWeapon(int mod)
{
	// players are allowed headshots from these weapons
	if (mod == MOD_LUGER ||
	    mod == MOD_COLT ||
	    mod == MOD_AKIMBO_COLT ||
	    mod == MOD_AKIMBO_LUGER ||
	    mod == MOD_AKIMBO_SILENCEDCOLT ||
	    mod == MOD_AKIMBO_SILENCEDLUGER ||
	    mod == MOD_MP40 ||
	    mod == MOD_THOMPSON ||
	    mod == MOD_STEN ||
	    mod == MOD_GARAND

	    || mod == MOD_KAR98
	    || mod == MOD_K43
	    || mod == MOD_K43_SCOPE
	    || mod == MOD_CARBINE
	    || mod == MOD_GARAND
	    || mod == MOD_GARAND_SCOPE
	    || mod == MOD_SILENCER
	    || mod == MOD_SILENCED_COLT
	    || mod == MOD_FG42
	    || mod == MOD_FG42SCOPE
	    )
	{
		return qtrue;
	}

	return qfalse;
}

gentity_t *G_BuildHead(gentity_t *ent)
{
	gentity_t     *head;
	orientation_t orientation;

	head = G_Spawn();

	if (trap_GetTag(ent->s.number, 0, "tag_head", &orientation))
	{
		G_SetOrigin(head, orientation.origin);
	}
	else
	{
		float  height, dest;
		vec3_t v, angles, forward, up, right;

		G_SetOrigin(head, ent->r.currentOrigin);

		if (ent->client->ps.eFlags & EF_PRONE)
		{
			height = ent->client->ps.viewheight - 56;
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
			dest = (-360 + angles[PITCH]) * 0.75;
		}
		else
		{
			dest = angles[PITCH] * 0.75;
		}
		angles[PITCH] = dest;

		AngleVectors(angles, forward, right, up);
		if (ent->client->ps.eFlags & EF_PRONE)
		{
			VectorScale(forward, 24, v);
		}
		else
		{
			VectorScale(forward, 5, v);
		}
		VectorMA(v, 18, up, v);

		VectorAdd(v, head->r.currentOrigin, head->r.currentOrigin);
		head->r.currentOrigin[2] += height / 2;
	}

	VectorCopy(head->r.currentOrigin, head->s.origin);
	VectorCopy(ent->r.currentAngles, head->s.angles);
	VectorCopy(head->s.angles, head->s.apos.trBase);
	VectorCopy(head->s.angles, head->s.apos.trDelta);
	VectorSet(head->r.mins, -6, -6, -2);   // changed this z from -12 to -6 for crouching, also removed standing offset
	VectorSet(head->r.maxs, 6, 6, 10);     // changed this z from 0 to 6
	head->clipmask   = CONTENTS_SOLID;
	head->r.contents = CONTENTS_SOLID;
	head->parent     = ent;
	head->s.eType    = ET_TEMPHEAD;

	trap_LinkEntity(head);

	return head;
}

gentity_t *G_BuildLeg(gentity_t *ent)
{
	gentity_t *leg;
	vec3_t    flatforward, org;

	if (!(ent->client->ps.eFlags & EF_PRONE))
	{
		return NULL;
	}

	leg = G_Spawn();

	AngleVectors(ent->client->ps.viewangles, flatforward, NULL, NULL);
	flatforward[2] = 0;
	VectorNormalizeFast(flatforward);

	org[0] = ent->r.currentOrigin[0] + flatforward[0] * -32;
	org[1] = ent->r.currentOrigin[1] + flatforward[1] * -32;
	org[2] = ent->r.currentOrigin[2] + ent->client->pmext.proneLegsOffset;

	G_SetOrigin(leg, org);

	VectorCopy(leg->r.currentOrigin, leg->s.origin);
	VectorCopy(ent->r.currentAngles, leg->s.angles);
	VectorCopy(leg->s.angles, leg->s.apos.trBase);
	VectorCopy(leg->s.angles, leg->s.apos.trDelta);
	VectorCopy(playerlegsProneMins, leg->r.mins);
	VectorCopy(playerlegsProneMaxs, leg->r.maxs);
	leg->clipmask   = CONTENTS_SOLID;
	leg->r.contents = CONTENTS_SOLID;
	leg->parent     = ent;
	leg->s.eType    = ET_TEMPLEGS;

	trap_LinkEntity(leg);

	return leg;
}

qboolean IsHeadShot(gentity_t *targ, vec3_t dir, vec3_t point, int mod)
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

	if (targ->health <= 0)
	{
		return qfalse;
	}

	if (!IsHeadShotWeapon(mod))
	{
		return qfalse;
	}

	head = G_BuildHead(targ);

	// trace another shot see if we hit the head
	VectorCopy(point, start);
	VectorMA(start, 64, dir, end);
	trap_Trace(&tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT);

	traceEnt = &g_entities[tr.entityNum];

	if (g_debugBullets.integer >= 3)     // show hit player head bb
	{
		gentity_t *tent;
		vec3_t    b1, b2;

		VectorCopy(head->r.currentOrigin, b1);
		VectorCopy(head->r.currentOrigin, b2);
		VectorAdd(b1, head->r.mins, b1);
		VectorAdd(b2, head->r.maxs, b2);
		tent = G_TempEntity(b1, EV_RAILTRAIL);
		VectorCopy(b2, tent->s.origin2);
		tent->s.dmgFlags = 1;

		// show headshot trace
		// end the headshot trace at the head box if it hits
		if (tr.fraction != 1)
		{
			VectorMA(start, (tr.fraction * 64), dir, end);
		}
		tent = G_TempEntity(start, EV_RAILTRAIL);
		VectorCopy(end, tent->s.origin2);
		tent->s.dmgFlags = 0;
	}

	G_FreeEntity(head);

	if (traceEnt == head)
	{
		level.totalHeadshots++;
		return qtrue;
	}
	else
	{
		level.missedHeadshots++;
	}
	return qfalse;
}

qboolean IsLegShot(gentity_t *targ, vec3_t dir, vec3_t point, int mod)
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

	if (!IsHeadShotWeapon(mod))
	{
		return qfalse;
	}

	leg = G_BuildLeg(targ);

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

		if (g_debugBullets.integer >= 3)     // show hit player head bb
		{
			gentity_t *tent;
			vec3_t    b1, b2;
			VectorCopy(leg->r.currentOrigin, b1);
			VectorCopy(leg->r.currentOrigin, b2);
			VectorAdd(b1, leg->r.mins, b1);
			VectorAdd(b2, leg->r.maxs, b2);
			tent = G_TempEntity(b1, EV_RAILTRAIL);
			VectorCopy(b2, tent->s.origin2);
			tent->s.dmgFlags = 1;

			// show headshot trace
			// end the headshot trace at the head box if it hits
			if (tr.fraction != 1)
			{
				VectorMA(start, (tr.fraction * 64), dir, end);
			}
			tent = G_TempEntity(start, EV_RAILTRAIL);
			VectorCopy(end, tent->s.origin2);
			tent->s.dmgFlags = 0;
		}

		G_FreeEntity(leg);

		if (traceEnt == leg)
		{
			return qtrue;
		}
	}
	else
	{
		float height  = point[2] - targ->r.absmin[2];
		float theight = targ->r.absmax[2] - targ->r.absmin[2];

		if (height < (theight * 0.4f))
		{
			return qtrue;
		}
	}

	return qfalse;
}

qboolean IsArmShot(gentity_t *targ, gentity_t *ent, vec3_t point, int mod)
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

	if (!IsHeadShotWeapon(mod))
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

/*
============
G_Damage

targ        entity that is being damaged
inflictor   entity that is causing the damage
attacker    entity that caused the inflictor to damage targ
    example: targ=monster, inflictor=rocket, attacker=player

dir         direction of the attack for knockback
point       point at which the damage is being inflicted, used for headshots
damage      amount of damage being inflicted
knockback   force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags      these flags are used to control how T_Damage works
    DAMAGE_RADIUS           damage was indirect (from a nearby explosion)
    DAMAGE_NO_ARMOR         armor does not protect from this damage
    DAMAGE_NO_KNOCKBACK     do not affect velocity, just view angles
    DAMAGE_NO_PROTECTION    kills godmode, armor, everything
============
*/
void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod)
{
	gclient_t   *client;
	int         take;
	int         knockback;
	qboolean    wasAlive;
	hitRegion_t hr = HR_NUM_HITREGIONS;

	if (!targ->takedamage)
	{
		return;
	}

	// DEBUG
	//trap_SendServerCommand( -1, va("print \"%i\n\"\n", targ->health) );

	// The intermission has allready been qualified for, so don't allow any extra scoring.
	// Don't do damage if at warmup and warmupdamage is set to 'None' and the target is a client.
	if (level.intermissionQueued || (g_gamestate.integer != GS_PLAYING
	                                 && match_warmupDamage.integer == 0 && targ->client))
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

	// invisible entities can't be damaged
	if (targ->entstate == STATE_INVISIBLE ||
	    targ->entstate == STATE_UNDERCONSTRUCTION)
	{
		return;
	}

	// was the bot alive before applying any damage?
	wasAlive = (targ->health > 0);

	// combatstate
	if (targ->client && attacker && attacker->client && attacker != targ)
	{
		/*vec_t dist = -1.f;

		if( targ->client->combatState < COMBATSTATE_HOT ) {
		    vec3_t shotvec;

		    VectorSubtract( targ->r.currentOrigin, attacker->r.currentOrigin, shotvec );
		    dist = VectorLengthSquared( shotvec );

		    if( dist < Square(1500.f) && targ->client->combatState == COMBATSTATE_WARM )
		        targ->client->combatState = COMBATSTATE_HOT;
		}

		if( attacker->client->combatState < COMBATSTATE_HOT ) {
		    if( dist < 0.f ) {
		        vec3_t shotvec;

		        VectorSubtract( targ->r.currentOrigin, attacker->r.currentOrigin, shotvec );
		        dist = VectorLengthSquared( shotvec );
		    }

		    if( dist > Square(1500.f) )
		        attacker->client->combatState = COMBATSTATE_WARM;
		    else if( attacker->client->combatState == COMBATSTATE_WARM )
		        attacker->client->combatState = COMBATSTATE_HOT;
		}*/

		if (g_gamestate.integer == GS_PLAYING)
		{
			if (!OnSameTeam(attacker, targ))
			{
				targ->client->combatState |= (1 << COMBATSTATE_DAMAGERECEIVED);
				if (attacker->client->sess.sessionTeam != TEAM_SPECTATOR)
				{
					attacker->client->combatState |= (1 << COMBATSTATE_DAMAGEDEALT);
				}
			}
		}
	}

	if ((targ->waterlevel >= 3) && (mod == MOD_FLAMETHROWER))
	{
		return;
	}

	// shootable doors / buttons don't actually have any health
	if (targ->s.eType == ET_MOVER && !(targ->isProp) && !targ->scriptName)
	{
		if (targ->use && targ->moverState == MOVER_POS1)
		{
			G_UseEntity(targ, inflictor, attacker);
		}
		return;
	}

	// In the old code, this check wasn't done for props, so I put that check back in to make props_statue properly work
	// 4 means destructible
	if (targ->s.eType == ET_MOVER && (targ->spawnflags & 4) && !targ->isProp)
	{
		if (!G_WeaponIsExplosive(mod))
		{
			return;
		}

		// check for team
		if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
		{
			return;
		}
	}
	else if (targ->s.eType == ET_EXPLOSIVE)
	{
#if 0
		// 32 Explosive
		// 64 Dynamite only
		// 256 Airstrike/artillery only
		// 512 Satchel only
		if ((targ->spawnflags & 32) || (targ->spawnflags & 64) || (targ->spawnflags & 256) || (targ->spawnflags & 512))
		{
			switch (mod)
			{
			case MOD_GRENADE:
			case MOD_GRENADE_LAUNCHER:
			case MOD_GRENADE_PINEAPPLE:
			case MOD_MAPMORTAR:
			case MOD_EXPLOSIVE:
			case MOD_LANDMINE:
			case MOD_GPG40:
			case MOD_M7:
				if (!(targ->spawnflags & 32))
				{
					return;
				}
				break;
			case MOD_SATCHEL:
				if (!(targ->spawnflags & 512))
				{
					return;
				}
				break;
			case MOD_ARTY:
			case MOD_AIRSTRIKE:
				if (!(targ->spawnflags & 256))
				{
					return;
				}
				break;
			case MOD_DYNAMITE:
				if (!(targ->spawnflags & 64))
				{
					return;
				}
				break;
			default:
				return;
			}

			// check for team
			if (targ->s.teamNum == inflictor->s.teamNum)
			{
				return;
			}
		}
#endif // 0

		if (targ->parent && G_GetWeaponClassForMOD(mod) == 2)
		{
			return;
		}

		// check for team
		if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
		{
			return;
		}

		if (G_GetWeaponClassForMOD(mod) < targ->constructibleStats.weaponclass)
		{
			return;
		}
	}
	else if (targ->s.eType == ET_MISSILE && targ->methodOfDeath == MOD_LANDMINE)
	{
		if (targ->s.modelindex2)
		{
			if (G_WeaponIsExplosive(mod))
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

				if (attacker && attacker->client)
				{
					AddScore(attacker, 1);
				}

				G_ExplodeMissile(targ);
			}
		}
		return;
	}
	else if (targ->s.eType == ET_CONSTRUCTIBLE)
	{
		if (G_GetTeamFromEntity(inflictor) == G_GetTeamFromEntity(targ))
		{
			return;
		}

		if (G_GetWeaponClassForMOD(mod) < targ->constructibleStats.weaponclass)
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
	}

	client = targ->client;

	if (client)
	{
		if (client->noclip || client->ps.powerups[PW_INVULNERABLE])
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

	knockback = damage;
	if (knockback > 200)
	{
		knockback = 200;
	}
	if (targ->flags & FL_NO_KNOCKBACK)
	{
		knockback = 0;
	}
	if (dflags & DAMAGE_NO_KNOCKBACK)
	{
		knockback = 0;
	}
	else if (dflags & DAMAGE_HALF_KNOCKBACK)
	{
		knockback *= 0.5f;
	}

	// set weapons means less knockback
	if (client && (client->ps.weapon == WP_MORTAR_SET || client->ps.weapon == WP_MOBILE_MG42_SET))
	{
		knockback *= 0.5;
	}

	if (targ->client && g_friendlyFire.integer && (OnSameTeam(targ, attacker) || (attacker->client && targ->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor))))
	{
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if (knockback && targ->client)
	{
		vec3_t kvel;
		float  mass = 200;

		VectorScale(dir, g_knockback.value * (float)knockback / mass, kvel);
		VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		// are we pushed? Do not count when already flying ...
		if (attacker && attacker->client && (targ->client->ps.groundEntityNum != ENTITYNUM_NONE || G_WeaponIsExplosive(mod)))
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
			targ->client->ps.velocity[2] *= 0.25;
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
		if (targ != attacker && (OnSameTeam(targ, attacker) || (targ->client && attacker->client && targ->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor))))
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
	if (targ->client && attacker->client && targ != attacker && targ->health > 0)
	{
		if (OnSameTeam(targ, attacker) || targ->client->sess.sessionTeam == G_GetTeamFromEntity(inflictor))
		{
			attacker->client->ps.persistant[PERS_HITS] -= damage;
		}
		else
		{
			attacker->client->ps.persistant[PERS_HITS] += damage;
		}
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
		switch (mod)
		{
		case MOD_GRENADE:
		case MOD_GRENADE_LAUNCHER:
		case MOD_GRENADE_PINEAPPLE:
		case MOD_MAPMORTAR:
		case MOD_MAPMORTAR_SPLASH:
		case MOD_EXPLOSIVE:
		case MOD_LANDMINE:
		case MOD_GPG40:
		case MOD_M7:
		case MOD_SATCHEL:
		case MOD_ARTY:
		case MOD_AIRSTRIKE:
		case MOD_DYNAMITE:
		case MOD_MORTAR:
		case MOD_PANZERFAUST:
			take -= take * .5f;
			break;
		default:
			break;
		}
	}

	if (IsHeadShot(targ, dir, point, mod))
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
			vec_t  dist;
			vec3_t shotvec;
			float  scale;

			VectorSubtract(point, muzzleTrace, shotvec);
			dist = VectorLength(shotvec);

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

			if (mod != MOD_K43_SCOPE &&
			    mod != MOD_GARAND_SCOPE)
			{
				take *= .8f;    // helmet gives us some protection
			}
		}

		targ->client->ps.eFlags |= EF_HEADSHOT;

		// Record the headshot
		if (client && attacker && attacker->client
#ifndef DEBUG_STATS
		    && attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam
#endif
		    )
		{
			G_addStatsHeadShot(attacker, mod);
			attacker->client->ps.persistant[PERS_HEADSHOTS]++;
		}

		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Head Shot\n\"\n");
		}
		G_LogRegionHit(attacker, HR_HEAD);
		hr = HR_HEAD;
	}
	else if (IsLegShot(targ, dir, point, mod))
	{
		G_LogRegionHit(attacker, HR_LEGS);
		hr = HR_LEGS;
		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Leg Shot\n\"\n");
		}
	}
	else if (IsArmShot(targ, attacker, point, mod))
	{
		G_LogRegionHit(attacker, HR_ARMS);
		hr = HR_ARMS;
		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Arm Shot\n\"\n");
		}
	}
	else if (targ->client && targ->health > 0 && IsHeadShotWeapon(mod))
	{
		G_LogRegionHit(attacker, HR_BODY);
		hr = HR_BODY;
		if (g_debugBullets.integer)
		{
			trap_SendServerCommand(attacker - g_entities, "print \"Body Shot\n\"\n");
		}
	}

#ifndef DEBUG_STATS
	if (g_debugDamage.integer)
#endif
	{
		G_Printf("client:%i health:%i damage:%i mod:%s\n", targ->s.number, targ->health, take, modNames[mod]);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		if (attacker)
		{
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		}
		else
		{
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_blood     += take;
		client->damage_knockback += knockback;

		if (dir)
		{
			VectorCopy(dir, client->damage_from);
			client->damage_fromWorld = qfalse;
		}
		else
		{
			VectorCopy(targ->r.currentOrigin, client->damage_from);
			client->damage_fromWorld = qtrue;
		}
	}

	if (targ->client)
	{
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod    = mod;
	}

	// do the damage
	if (take)
	{
		targ->health -= take;

		// can't gib with bullet weapons (except VENOM)
		// - attacker == inflictor can happen in other cases as well! (movers trying to gib things)
		//if ( attacker == inflictor && targ->health <= GIB_HEALTH) {
		if (targ->health <= GIB_HEALTH)
		{
			if (!G_WeaponIsExplosive(mod))
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
			targ->s.dl_intensity = 255.f * (targ->health / (float)targ->count);   // send it to the client
		}

		//G_Printf("health at: %d\n", targ->health);
		if (targ->health <= 0)
		{
			if (client && !wasAlive)
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
			}
			else
			{
				targ->sound1to2 = hr;
				targ->sound2to1 = mod;
				targ->sound2to3 = (dflags & DAMAGE_RADIUS) ? 1 : 0;

				if (client)
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

				// mg42 doesn't have die func (FIXME)
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

void G_RailTrail(vec_t *start, vec_t *end)
{
	gentity_t *temp = G_TempEntity(start, EV_RAILTRAIL);

	VectorCopy(end, temp->s.origin2);
	temp->s.dmgFlags = 0;
}

#define MASK_CAN_DAMAGE     (CONTENTS_SOLID | CONTENTS_BODY)

/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
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
	if (targ->r.currentOrigin[0] || targ->r.currentOrigin[1] || targ->r.currentOrigin[2])
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
		VectorScale(midpoint, 0.5, midpoint);
	}

	//G_RailTrail( origin, dest );

	trap_Trace(&tr, origin, vec3_origin, vec3_origin, midpoint, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1.0)
	{
		return qtrue;
	}

	if (&g_entities[tr.entityNum] == targ)
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
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	// =========================

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[2];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE);
	if (tr.fraction == 1 || &g_entities[tr.entityNum] == targ)
	{
		return qtrue;
	}

	return qfalse;
}

void G_AdjustedDamageVec(gentity_t *ent, vec3_t origin, vec3_t v)
{
	if (!ent->r.bmodel)
	{
		VectorSubtract(ent->r.currentOrigin, origin, v); // simpler centroid check that doesn't have box alignment weirdness
	}
	else
	{
		int i;

		for (i = 0 ; i < 3 ; i++)
		{
			if (origin[i] < ent->r.absmin[i])
			{
				v[i] = ent->r.absmin[i] - origin[i];
			}
			else if (origin[i] > ent->r.absmax[i])
			{
				v[i] = origin[i] - ent->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}
	}
}

/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod)
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
		if (!ent->takedamage && (!ent->dmgparent || !ent->dmgparent->takedamage))
		{
			continue;
		}

		G_AdjustedDamageVec(ent, origin, v);

		dist = VectorLength(v);
		if (dist >= radius)
		{
			continue;
		}

		points = damage * (1.0 - dist / radius);

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

			G_Damage(ent, inflictor, attacker, dir, origin, ROUND_INT(points), flags, mod);
		}
		else
		{
			VectorAdd(ent->r.absmin, ent->r.absmax, midpoint);
			VectorScale(midpoint, 0.5, midpoint);
			VectorCopy(midpoint, dest);

			trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
			if (tr.fraction < 1.0)
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
					G_Damage(ent, inflictor, attacker, dir, origin, ROUND_INT(points * 0.1f), flags, mod);
				}
			}
		}
	}
	return hitClient;
}

/*
============
etpro_RadiusDamage
mutation of G_RadiusDamage which lets us selectively damage only clients or only non clients
============
*/
qboolean etpro_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod, qboolean clientsonly)
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
		if (!ent->takedamage && (!ent->dmgparent || !ent->dmgparent->takedamage))
		{
			continue;
		}

		if (clientsonly && !ent->client)
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

		points = damage * (1.0 - dist / radius);

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

			G_Damage(ent, inflictor, attacker, dir, origin, ROUND_INT(points), flags, mod);
		}
		else
		{
			VectorAdd(ent->r.absmin, ent->r.absmax, midpoint);
			VectorScale(midpoint, 0.5, midpoint);
			VectorCopy(midpoint, dest);

			trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
			if (tr.fraction < 1.0)
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
					G_Damage(ent, inflictor, attacker, dir, origin, ROUND_INT(points * 0.1f), flags, mod);
				}
			}
		}
	}

	return hitClient;
}
