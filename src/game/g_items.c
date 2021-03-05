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
 * @file g_items.c
 * @brief Items are any object that a player can touch to gain some effect.
 *
 * Pickup will return the number of seconds until they should respawn.
 * all items should pop when dropped in lava or slime.
 * Respawnable items don't actually go away when picked up, they are just made
 * invisible and untouchable.  This allows them to ride movers and respawn
 * apropriately.
 */

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

#define RESPAWN_NEVER       -1
#define RESPAWN_KEY          4
#define RESPAWN_ARMOR        25
#define RESPAWN_TEAM_WEAPON  30
#define RESPAWN_HEALTH       35
#define RESPAWN_AMMO         40
#define RESPAWN_HOLDABLE     60
#define RESPAWN_MEGAHEALTH   120
#define RESPAWN_POWERUP      120
#define RESPAWN_PARTIAL      998    // for multi-stage ammo/health
#define RESPAWN_PARTIAL_DONE 999    // for multi-stage ammo/health

/**
 * @brief Add the specified ammount of ammo into the clip.
 * @param[in,out] ps which player
 * @param[in] weapon to add ammo for
 * @param[in] ammomove amount to add. 0 means fill the clip if possible
 * @param[in] outOfReserve amount to be added out of reserve
 * @return qboolean whether ammo was added to the clip.
 */
int AddToClip(playerState_t *ps, weapon_t weapon, int ammomove, int outOfReserve)
{
	int      inclip, maxclip;
	weapon_t ammoweap = GetWeaponTableData(weapon)->ammoIndex;

	if (!IS_VALID_WEAPON(weapon))
	{
		return qfalse;
	}

	inclip  = ps->ammoclip[GetWeaponTableData(weapon)->clipIndex];
	maxclip = GetWeaponTableData(weapon)->maxClip;

	if (!ammomove)     // amount to add to the clip not specified
	{
		ammomove = maxclip - inclip;    // max amount that can be moved into the clip
	}
	else if (ammomove > maxclip - inclip)
	{
		ammomove = maxclip - inclip;
	}

	if (outOfReserve)
	{
		// cap move amount if it's more than you've got in reserve
		if (ammomove > ps->ammo[ammoweap])
		{
			ammomove = ps->ammo[ammoweap];
		}
	}

	if (ammomove)
	{
		if (outOfReserve)
		{
			ps->ammo[ammoweap] -= ammomove;
		}
		ps->ammoclip[GetWeaponTableData(weapon)->clipIndex] += ammomove;
		return qtrue;
	}
	return qfalse;
}


/**
 * @brief Push reserve ammo into available space in the clip.
 * @param[in] ps
 * @param[in] weapon
 */
void Fill_Clip(playerState_t *ps, weapon_t weapon)
{
	AddToClip(ps, weapon, 0, qtrue);
}

/**
 * @brief Add ammo.
 * @param[in,out] ent
 * @param[in] weapon
 * @param[in] count
 * @param[in] fillClip
 * @return whether any ammo was added
 */
int Add_Ammo(gentity_t *ent, weapon_t weapon, int count, qboolean fillClip)
{
	weapon_t ammoweap      = GetWeaponTableData(weapon)->ammoIndex;
	int      maxammo       = BG_MaxAmmoForWeapon(ammoweap, ent->client->sess.skill, ent->client->ps.stats[STAT_PLAYER_CLASS]);
	int      originalCount = ent->client->ps.ammo[ammoweap];

	if ((GetWeaponTableData(ammoweap)->type & WEAPON_TYPE_GRENADE) || ammoweap == WP_DYNAMITE || ammoweap == WP_SATCHEL_DET) // make sure if he picks it up that he get's the "launcher" too
	{
		COM_BitSet(ent->client->ps.weapons, ammoweap);
		fillClip = qtrue;   // always filter into the "clip"
	}

	if (fillClip)
	{
		Fill_Clip(&ent->client->ps, weapon);
	}

	ent->client->ps.ammo[ammoweap] += count;

	if (!GetWeaponTableData(ammoweap)->useClip)
	{
		maxammo -= ent->client->ps.ammoclip[ammoweap];
	}

	if (ent->client->ps.ammo[ammoweap] > maxammo)
	{
		ent->client->ps.ammo[ammoweap] = maxammo;   // - ent->client->ps.ammoclip[BG_FindClipForWeapon(weapon)];
	}

	if (count >= 999)     // 'really, give /all/'
	{
		ent->client->ps.ammo[ammoweap] = count;
	}

	return (ent->client->ps.ammo[ammoweap] > originalCount);
}

/**
 * @brief Add the specified number of clips of magic ammo
 * @param[in] receiver
 * @param[in] numOfClips
 * @return whether any ammo was actually added
 */
qboolean AddMagicAmmo(gentity_t *receiver, int numOfClips)
{
	return BG_AddMagicAmmo(&receiver->client->ps, receiver->client->sess.skill, receiver->client->sess.sessionTeam, numOfClips);
}

/**
 * @brief Get the primary weapon of the client.
 * @param[in] client
 * @return the primary weapon of the client
 */
weapon_t G_GetPrimaryWeaponForClient(gclient_t *client)
{
	int i, team;

	if (client->sess.sessionTeam != TEAM_ALLIES && client->sess.sessionTeam != TEAM_AXIS)
	{
		return WP_NONE;
	}

	for (team = TEAM_AXIS; team <= TEAM_ALLIES; team++)
	{
		bg_playerclass_t *classInfo = GetPlayerClassesData(team, client->sess.playerType);

		for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
		{
			if (COM_BitCheck(client->ps.weapons, classInfo->classPrimaryWeapons[i].weapon))
			{
				return classInfo->classPrimaryWeapons[i].weapon;
			}
		}
	}

	return WP_NONE;
}

/*
 * @brief G_GetSecondaryWeaponForClient
 * @param[in] client
 * @param primary - unused
 * @return
 *
 * @note Unused
 *
weapon_t G_GetSecondaryWeaponForClient(gclient_t *client, weapon_t primary)
{
    weapon_t secondary = WP_NONE;

    // early out if not on a team
    if (client->sess.sessionTeam != TEAM_ALLIES && client->sess.sessionTeam != TEAM_AXIS)
    {
        return WP_NONE;
    }

    // Record our secondary weapon (usually a pistol sidearm)
    // Colts
    if (COM_BitCheck(client->ps.weapons, WP_AKIMBO_SILENCEDCOLT))
    {
        secondary = WP_AKIMBO_SILENCEDCOLT;
    }
    else if (COM_BitCheck(client->ps.weapons, WP_AKIMBO_COLT))
    {
        secondary = WP_AKIMBO_COLT;
    }
    else if (COM_BitCheck(client->ps.weapons, WP_SILENCED_COLT))
    {
        secondary = WP_SILENCED_COLT;
    }
    else if (COM_BitCheck(client->ps.weapons, WP_COLT))
    {
        secondary = WP_COLT;
    }
    // Lugers
    else if (COM_BitCheck(client->ps.weapons, WP_AKIMBO_SILENCEDLUGER))
    {
        secondary = WP_AKIMBO_SILENCEDLUGER;
    }
    else if (COM_BitCheck(client->ps.weapons, WP_AKIMBO_LUGER))
    {
        secondary = WP_AKIMBO_LUGER;
    }
    else if (COM_BitCheck(client->ps.weapons, WP_SILENCER))
    {
        secondary = WP_SILENCER;
    }
    else if (COM_BitCheck(client->ps.weapons, WP_LUGER))
    {
        secondary = WP_LUGER;
    }

    return secondary;
}
*/

/**
 * @brief Get the primary weapon of the client.
 * @param[in] weapon
 * @param[in] client
 * @return the primary weapon of the soldier client
 */
weapon_t G_GetPrimaryWeaponForClientSoldier(gclient_t *client)
{
	int i, team;

	if (client->sess.sessionTeam != TEAM_ALLIES && client->sess.sessionTeam != TEAM_AXIS)
	{
		return WP_NONE;
	}

	for (team = TEAM_AXIS; team <= TEAM_ALLIES; team++)
	{
		// if weapons are SMG & HW, return HW if picking up HW
		bg_playerclass_t *classInfo = GetPlayerClassesData(team, client->sess.playerType);

		for (i = 1; i < MAX_WEAPS_PER_CLASS; i++)
		{
			if (COM_BitCheck(client->ps.weapons, classInfo->classPrimaryWeapons[i].weapon))
			{
				return classInfo->classPrimaryWeapons[i].weapon;
			}
		}
	}

	for (team = TEAM_AXIS; team <= TEAM_ALLIES; team++)
	{
		// if weapons are SMG & HW, return HW if picking up HW
		bg_playerclass_t *classInfo = GetPlayerClassesData(team, client->sess.playerType);

		if (COM_BitCheck(client->ps.weapons, classInfo->classPrimaryWeapons[0].weapon))
		{
			return classInfo->classPrimaryWeapons[0].weapon;
		}
	}

	return WP_NONE;
}

/**
 * @brief Drop Weapon
 * @param[in] ent
 * @param[in] weapon
 */
void G_DropWeapon(gentity_t *ent, weapon_t weapon)
{
	vec3_t    angles, velocity, org, offset, mins, maxs;
	gclient_t *client = ent->client;
	gentity_t *ent2;
	gitem_t   *item;
	trace_t   tr;

	if (!IS_VALID_WEAPON(weapon))
	{
		return;
	}

	item = BG_GetItem(GetWeaponTableData(weapon)->item);

	if (item->giType != IT_WEAPON || item->giWeapon != weapon)
	{
		Com_Error(ERR_DROP, "Couldn't get item for weapon %i", weapon);
	}

	VectorCopy(client->ps.viewangles, angles);

	// clamp pitch
	if (angles[PITCH] < -30)
	{
		angles[PITCH] = -30;
	}
	else if (angles[PITCH] > 30)
	{
		angles[PITCH] = 30;
	}

	AngleVectors(angles, velocity, NULL, NULL);
	VectorScale(velocity, 64, offset);
	offset[2] += client->ps.viewheight / 2.f;
	VectorScale(velocity, 75, velocity);
	velocity[2] += 50 + random() * 35;

	VectorAdd(client->ps.origin, offset, org);

	VectorSet(mins, -ITEM_RADIUS, -ITEM_RADIUS, 0);
	VectorSet(maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS);

	trap_Trace(&tr, client->ps.origin, mins, maxs, org, ent->s.number, MASK_SOLID);
	VectorCopy(tr.endpos, org);

	ent2 = LaunchItem(item, org, velocity, client->ps.clientNum);
	COM_BitClear(client->ps.weapons, weapon);

	if (GetWeaponTableData(weapon)->weapAlts)
	{
		weapon_t weapAlts = GetWeaponTableData(weapon)->weapAlts;

		if (GetWeaponTableData(weapAlts)->type & (WEAPON_TYPE_RIFLENADE | WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET))
		{
			COM_BitClear(client->ps.weapons, weapAlts);
		}
	}

	// Clear out empty weapon, change to next best weapon
	G_AddEvent(ent, EV_WEAPONSWITCHED, 0);

	if (weapon == client->ps.weapon)
	{
		client->ps.weapon = 0;
	}

	if (!GetWeaponTableData(weapon)->useClip)
	{
		ent2->count = client->ps.ammo[GetWeaponTableData(weapon)->ammoIndex] + client->ps.ammoclip[GetWeaponTableData(weapon)->clipIndex];
	}
	else
	{
		ent2->count = client->ps.ammoclip[GetWeaponTableData(weapon)->clipIndex];
	}

	if (weapon == WP_KAR98 || weapon == WP_CARBINE)
	{
		ent2->delay = client->ps.ammo[GetWeaponTableData(GetWeaponTableData(weapon)->weapAlts)->ammoIndex];
	}
	else
	{
		ent2->delay = 0;
	}

	//  ent2->item->quantity = client->ps.ammoclip[BG_FindClipForWeapon(weapon)]; // um, modifying an item is not a good idea
	client->ps.ammoclip[GetWeaponTableData(weapon)->clipIndex] = 0;

#ifdef FEATURE_OMNIBOT
	Bot_Event_RemoveWeapon(client->ps.clientNum, Bot_WeaponGameToBot(weapon));
#endif
}

/**
 * @brief Check if a weapon can be picked up.
 * @param[in] weapon
 * @param[in] ent
 * @return
 */
qboolean G_CanPickupWeapon(weapon_t weapon, gentity_t *ent)
{
	// prevent picking up while reloading
	if (ent->client->ps.weaponstate == WEAPON_RELOADING)
	{
		return qfalse;
	}

	// prevent picking up when overheating
	if (ent->client->ps.weaponTime > 0)
	{
		return qfalse;
	}

	// get an equivalent weapon if the client team is different of the weapon team, if not keep the current
	if (ent->client->sess.sessionTeam != GetWeaponTableData(weapon)->team && GetWeaponTableData(weapon)->weapEquiv)
	{
		weapon = GetWeaponTableData(weapon)->weapEquiv;
	}

	// single weapon restrictions
	if (G_IsWeaponDisabled(ent, weapon))
	{
		return qfalse;
	}

	return BG_WeaponIsPrimaryForClassAndTeam(ent->client->sess.playerType, ent->client->sess.sessionTeam, weapon);
}
/**
 * @brief Pick a weapon up.
 * @param[in,out] ent
 * @param[in,out] other
 * @return
 */
int Pickup_Weapon(gentity_t *ent, gentity_t *other)
{
	int      quantity;
	qboolean alreadyHave = qfalse;

	// magic ammo for any two-handed weapon
	if (ent->item->giWeapon == WP_AMMO)
	{
		AddMagicAmmo(other, ent->count);
		if (ent->parent && ent->parent->client)
		{
			other->client->pers.lastammo_client = ent->parent->s.clientNum;
		}

		// if field ops isn't giving ammo to self or the enemy, give him some props
		if (ent->parent && (ent->parent->client != other->client))
		{
			if (ent->parent && ent->parent->client && other->client->sess.sessionTeam == ent->parent->client->sess.sessionTeam)
			{
				G_AddSkillPoints(ent->parent, SK_SIGNALS, 1.f);
				G_DebugAddSkillPoints(ent->parent, SK_SIGNALS, 1.f, "ammo pack picked up");

#ifdef FEATURE_OMNIBOT
				//omni-bot event
				if (ent->parent)
				{
					Bot_Event_ReceivedAmmo(other - g_entities, ent->parent);
				}
#endif
				// extracted code originally here into AddMagicAmmo
				// add 1 clip of magic ammo for any two-handed weapon
			}
			return RESPAWN_NEVER;
		}
	}

	quantity = ent->count;

	// check if player already had the weapon
	alreadyHave = COM_BitCheck(other->client->ps.weapons, ent->item->giWeapon);

	// prevents drop/pickup weapon "quick reload" exploit
	if (alreadyHave)
	{
		Add_Ammo(other, ent->item->giWeapon, quantity, qfalse);

		// secondary weapon ammo (riflenade)
		if (ent->delay != 0.f)
		{
			Add_Ammo(other, GetWeaponTableData(ent->item->giWeapon)->weapAlts, ent->delay, qfalse);
		}
	}
	else
	{
		weapon_t primaryWeapon;
		qboolean canPickup;

		if (level.time - other->client->dropWeaponTime < 1000)
		{
			return 0;
		}

		// don't pick up when MG or mortar is set
		if (GetWeaponTableData(other->client->ps.weapon)->type & WEAPON_TYPE_SET)
		{
			return 0;
		}

		// see if we can pick it up
		canPickup = G_CanPickupWeapon(ent->item->giWeapon, other);

		if (!canPickup)
		{
			return 0;
		}

		if (other->client->sess.playerType == PC_SOLDIER && other->client->sess.skill[SK_HEAVY_WEAPONS] >= 4)
		{
			primaryWeapon = G_GetPrimaryWeaponForClientSoldier(other->client);
		}
		else
		{
			primaryWeapon = G_GetPrimaryWeaponForClient(other->client);
		}

		// drop our primary weapon if one exist
		if (primaryWeapon)
		{
			G_DropWeapon(other, primaryWeapon);
		}

		// now pickup the other one
		other->client->dropWeaponTime = level.time;

		// add the weapon
		COM_BitSet(other->client->ps.weapons, ent->item->giWeapon);

		// fixup mauser/sniper issues
		if (GetWeaponTableData(ent->item->giWeapon)->weapAlts)
		{
			weapon_t weapAlts = GetWeaponTableData(ent->item->giWeapon)->weapAlts;

			if (GetWeaponTableData(weapAlts)->type & (WEAPON_TYPE_RIFLENADE | WEAPON_TYPE_SCOPED | WEAPON_TYPE_SET))
			{
				COM_BitSet(other->client->ps.weapons, weapAlts);
			}
		}

		other->client->ps.ammoclip[GetWeaponTableData(ent->item->giWeapon)->clipIndex] = 0;
		other->client->ps.ammo[GetWeaponTableData(ent->item->giWeapon)->ammoIndex]     = 0;

		if (GetWeaponTableData(ent->item->giWeapon)->useClip)
		{
			other->client->ps.ammoclip[GetWeaponTableData(ent->item->giWeapon)->clipIndex] = quantity;
		}
		else
		{
			other->client->ps.ammo[GetWeaponTableData(ent->item->giWeapon)->clipIndex] = quantity;
		}

		// secondary weapon ammo (riflenade)
		if (ent->delay != 0.f)
		{
			other->client->ps.ammo[GetWeaponTableData(ent->item->giWeapon)->weapAlts] = ent->delay;
		}

		// update userinfo
		other->client->sess.playerWeapon = ent->item->giWeapon;
		ClientUserinfoChanged(other->client->ps.clientNum);
	}

#ifdef FEATURE_OMNIBOT
	Bot_Event_AddWeapon(other->client->ps.clientNum, Bot_WeaponGameToBot(ent->item->giWeapon));
#endif

	return RESPAWN_NEVER;
}

/**
 * @brief Pick health.
 * @param[in,out] ent
 * @param[in,out] other
 * @return
 */
int Pickup_Health(gentity_t *ent, gentity_t *other)
{
	if (ent->parent && ent->parent->client)
	{
		other->client->pers.lasthealth_client = ent->parent->s.clientNum;
	}

	// if medic isn't giving ammo to self or the enemy, give him some props
	if (ent->parent && ent->parent->client && ent->parent->client != other->client && other->client->sess.sessionTeam == ent->parent->client->sess.sessionTeam)
	{
		G_AddSkillPoints(ent->parent, SK_FIRST_AID, 1.f);
		G_DebugAddSkillPoints(ent->parent, SK_FIRST_AID, 1.f, "health pack picked up");
	}

	other->health += ent->item->quantity;
	if (other->health > other->client->ps.stats[STAT_MAX_HEALTH])
	{
		other->health = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

#ifdef FEATURE_OMNIBOT
	// omni-bot event
	if (ent->parent)
	{
		Bot_Event_Healed(other - g_entities, ent->parent);
	}
#endif

	return -1;
}

/**
 * @brief Respawn item.
 * @param[in,out] ent
 */
void RespawnItem(gentity_t *ent)
{
	// randomly select from teamed entities
	if (ent->team)
	{
		gentity_t *master;
		int       count;
		int       choice;

		if (!ent->teammaster)
		{
			G_Error("RespawnItem: bad teammaster\n");
		}
		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++)
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.eFlags &= ~EF_NODRAW;
	ent->flags     &= ~FL_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity(ent);

	ent->nextthink = 0;
}

/**
 * @brief Auto action when touching an item.
 *
 * PICKUP_ACTIVATE  (0), he will pick up items only when using +activate
 * PICKUP_TOUCH     (1), he will pickup items when touched
 * PICKUP_FORCE     (2), he will pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)
 *
 * @param[in,out] ent
 * @param[in,out] other
 * @param[in] trace
 *
 */
void Touch_Item_Auto(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	if (other->client->pers.autoActivate == PICKUP_ACTIVATE)
	{
		return;
	}

	if (!ent->active && ent->item->giType == IT_WEAPON)
	{
		if (ent->item->giWeapon != WP_AMMO)
		{
			if (!COM_BitCheck(other->client->ps.weapons, ent->item->giWeapon))
			{
				return; // force activate only
			}
		}
	}

	ent->active = qtrue;
	Touch_Item(ent, other, trace);

	if (other->client->pers.autoActivate == PICKUP_FORCE)        // autoactivate probably forced by the "Cmd_Activate_f()" function
	{
		other->client->pers.autoActivate = PICKUP_ACTIVATE;      // so reset it.
	}
}

/**
 * @brief Action when touching an item.
 * @param[in,out] ent
 * @param[in] other
 * @param trace - unused
 */
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace)
{
	int respawn;

	// only activated items can be picked up
	if (!ent->active)
	{
		return;
	}
	else
	{
		// need to set active to false if player is maxed out
		ent->active = qfalse;
	}

	if (!other->client)
	{
		return;
	}

	if (other->health <= 0)
	{
		return;     // dead people can't pickup
	}

	// the same pickup rules are used for client side and server side
	if (!BG_CanItemBeGrabbed(&ent->s, &other->client->ps, other->client->sess.skill, other->client->sess.sessionTeam))
	{
		return;
	}

	if (g_gamestate.integer == GS_PLAYING)
	{
		G_LogPrintf("Item: %i %s\n", other->s.number, ent->item->classname);
	}
	else
	{
		// Don't let them pickup winning stuff in warmup
		if (ent->item->giType != IT_WEAPON &&
		    ent->item->giType != IT_AMMO &&
		    ent->item->giType != IT_HEALTH)
		{
			return;
		}
	}

	//G_LogPrintf( "Calling item pickup function for %s\n", ent->item->classname );

	// call the item-specific pickup function
	switch (ent->item->giType)
	{
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	default:
		return;
	}

	//G_LogPrintf( "Finished pickup function\n" );

	if (!respawn)
	{
		return;
	}

	// play sounds
	if (ent->noise_index)
	{
		// a sound was specified in the entity, so play that sound
		// (this G_AddEvent) and send the pickup as "EV_ITEM_PICKUP_QUIET"
		// so it doesn't make the default pickup sound when the pickup event is received
		G_AddEvent(other, EV_GENERAL_SOUND, ent->noise_index);
		G_AddEvent(other, EV_ITEM_PICKUP_QUIET, ent->s.modelindex);
	}
	else
	{
		G_AddEvent(other, EV_ITEM_PICKUP, ent->s.modelindex);
	}

	// powerup pickups are global broadcasts
	if (ent->item->giType == IT_TEAM)
	{
		gentity_t *te = G_TempEntity(ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP);

		te->s.eventParm = ent->s.modelindex;
		te->r.svFlags  |= SVF_BROADCAST;
	}

	//G_LogPrintf( "Firing item targets\n" );

	// fire item targets
	G_UseTargets(ent, other);

	// dropped items will not respawn
	if (ent->flags & FL_DROPPED_ITEM)
	{
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->flags     |= FL_NODRAW;
	ent->r.contents = 0;

	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if (respawn <= 0)
	{
		ent->nextthink = 0;
		ent->think     = 0;
	}
	else
	{
		ent->nextthink = level.time + respawn * 1000;
		ent->think     = RespawnItem;
	}
	trap_LinkEntity(ent);
}

/**
 * @brief Spawns an item and tosses it forward.
 * @param[in] item
 * @param[in] origin
 * @param[in] velocity
 * @param[in] ownerNum
 * @return
 */
gentity_t *LaunchItem(gitem_t *item, vec3_t origin, vec3_t velocity, int ownerNum)
{
	gentity_t *dropped = G_Spawn();
	trace_t   tr;
	vec3_t    vec, temp;

	dropped->s.eType           = ET_ITEM;
	dropped->s.modelindex      = item->id ; // store item number in modelindex
	dropped->s.otherEntityNum2 = 1; // this is taking modelindex2's place for a dropped item

	dropped->classname = item->classname;
	dropped->item      = item;
	VectorSet(dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0);              // so items sit on the ground
	VectorSet(dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS);    // so items sit on the ground
	dropped->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;

	dropped->clipmask = CONTENTS_SOLID | CONTENTS_MISSILECLIP;      // fix for items falling through grates

	dropped->touch = Touch_Item_Auto;

	trap_Trace(&tr, origin, dropped->r.mins, dropped->r.maxs, origin, ownerNum, MASK_SOLID);
	if (tr.startsolid)
	{
		int i;

		VectorSubtract(g_entities[ownerNum].s.origin, origin, temp);
		VectorNormalize(temp);

		for (i = 16; i <= 48; i += 16)
		{
			VectorScale(temp, i, vec);
			VectorAdd(origin, vec, origin);

			trap_Trace(&tr, origin, dropped->r.mins, dropped->r.maxs, origin, ownerNum, MASK_SOLID);
			if (!tr.startsolid)
			{
				break;
			}
		}
	}

	G_SetOrigin(dropped, origin);
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy(velocity, dropped->s.pos.trDelta);

	// set yaw to parent angles
	temp[PITCH] = 0;
	temp[YAW]   = g_entities[ownerNum].s.apos.trBase[YAW];
	temp[ROLL]  = 0;
	G_SetAngle(dropped, temp);

	dropped->s.eFlags     |= EF_BOUNCE_HALF;
	dropped->physicsBounce = 0.25;

	if (item->giType == IT_TEAM)     // Special case for CTF flags
	{
		gentity_t *flag = &g_entities[g_entities[ownerNum].client->flagParent];

		dropped->s.otherEntityNum               = g_entities[ownerNum].client->flagParent;    // store the entitynum of our original flag spawner
		g_entities[ownerNum].client->flagParent = 0;
		dropped->s.density                      = 1;
		dropped->think                          = Team_DroppedFlagThink;
		dropped->nextthink                      = level.time + 30000;

		if (level.gameManager)
		{
			G_Script_ScriptEvent(level.gameManager, "trigger", flag->item->giPowerUp == PW_REDFLAG ? "allied_object_dropped" : "axis_object_dropped");
		}
		G_Script_ScriptEvent(flag, "trigger", "dropped");
	}
	else     // auto-remove after 30 seconds
	{
		dropped->think     = G_MagicSink;
		dropped->nextthink = level.time + 30000;
		dropped->s.time    = level.time + 30000; // for simple items colouring
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity(dropped);

	return dropped;
}

/**
 * @brief Spawns an item and tosses it forward.
 * @param[in] ent
 * @param[in] item
 * @param[in] angle
 * @param[in] novelocity
 * @return
 */
gentity_t *Drop_Item(gentity_t *ent, gitem_t *item, float angle, qboolean novelocity)
{
	vec3_t velocity;
	vec3_t angles;

	VectorCopy(ent->s.apos.trBase, angles);
	angles[YAW]  += angle;
	angles[PITCH] = 0;  // always forward

	if (novelocity)
	{
		VectorClear(velocity);
	}
	else
	{
		AngleVectors(angles, velocity, NULL, NULL);
		VectorScale(velocity, 150, velocity);
		velocity[2] += 200 + crandom() * 50;
	}

	return LaunchItem(item, ent->s.pos.trBase, velocity, ent->s.number);
}

/**
 * @brief Respawn the item.
 * @param[in] ent
 * @param other - unused
 * @param activator - unused
 */
void Use_Item(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	RespawnItem(ent);
}

/**
 * @brief Traces down to find where an item should rest, instead of letting them
 *        free fall from their spawn points.
 *
 * @param[in,out] ent
 */
void FinishSpawningItem(gentity_t *ent)
{
	trace_t tr;
	vec3_t  dest;
	vec3_t  maxs;

	if (ent->spawnflags & 1)     // suspended
	{
		VectorSet(ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
		VectorSet(ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
		VectorCopy(ent->r.maxs, maxs);
	}
	else
	{
		// had to modify this so that items would spawn in shelves
		VectorSet(ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0);
		VectorSet(ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
		VectorCopy(ent->r.maxs, maxs);
		maxs[2] /= 2;
	}

	ent->r.contents   = CONTENTS_TRIGGER | CONTENTS_ITEM;
	ent->touch        = Touch_Item_Auto;
	ent->s.eType      = ET_ITEM;
	ent->s.modelindex = ent->item->id;        // store item number in modelindex

	ent->s.otherEntityNum2 = 0;     // takes modelindex2's place in signaling a dropped item
	// we don't use this (yet, anyway) so I'm taking it so you can specify a model for treasure items and clipboards
	//ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item
	if (ent->model)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model);
	}

	// using an item causes it to respawn
	ent->use = Use_Item;

	// moved this up so it happens for suspended items too (and made it a function)
	G_SetAngle(ent, ent->s.angles);

	if (ent->spawnflags & 1)        // suspended
	{
		G_SetOrigin(ent, ent->s.origin);
	}
	else
	{
		VectorSet(dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096);
		trap_Trace(&tr, ent->s.origin, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID);

		if (tr.startsolid)
		{
			vec3_t temp;

			VectorCopy(ent->s.origin, temp);
			temp[2] -= ITEM_RADIUS;

			VectorSet(dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096);
			trap_Trace(&tr, temp, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID);
		}

		if (tr.startsolid)
		{
			G_Printf("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity(ent);
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin(ent, tr.endpos);
	}

	if (ent->spawnflags & 2)          // spin
	{
		ent->s.eFlags |= EF_SPINNING;
	}

	// team slaves and targeted items aren't present at start
	if ((ent->flags & FL_TEAMSLAVE) || ent->targetname)
	{
		ent->flags |= FL_NODRAW;
		//ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// health/ammo can potentially be multi-stage (multiple use)
	if (ent->item->giType == IT_HEALTH || ent->item->giType == IT_AMMO)
	{
		int i;

		// having alternate models defined in bg_misc.c for a health or ammo item specify it as "multi-stage"
		// - left-hand operand of comma expression has no effect
		for (i = 0; i < MAX_ITEM_MODELS && ent->item->world_model[i] ; i++)
		{
		}

		ent->s.density = i - 1;   // store number of stages in 'density' for client (most will have '1')
	}

	trap_LinkEntity(ent);
}

/**
 * @brief Sets the clipping size and plants the object on the floor.
 * @param[in,out] ent
 * @param[in] item
 */
void G_SpawnItem(gentity_t *ent, gitem_t *item)
{
	char *noise;

	G_SpawnFloat("random", "0", &ent->random);
	G_SpawnFloat("wait", "0", &ent->wait);

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think     = FinishSpawningItem;

	if (G_SpawnString("noise", 0, &noise))
	{
		ent->noise_index = G_SoundIndex(noise);
	}

	ent->physicsBounce = 0.50;      // items are bouncy

	if (ent->model)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model);
	}

	if (item->giType == IT_TEAM)
	{
		G_SpawnInt("count", "1", &ent->s.density);
		G_SpawnInt("speedscale", "100", &ent->splashDamage);
		if (!ent->splashDamage)
		{
			ent->splashDamage = 100;
		}
	}
}

/**
 * @brief Bounce an item.
 * @param[in,out] ent
 * @param[in,out] trace
 */
void G_BounceItem(gentity_t *ent, trace_t *trace)
{
	vec3_t velocity;
	float  dot;
	int    hitTime = (int)(level.previousTime + (level.time - level.previousTime) * trace->fraction);

	// reflect the velocity on the trace plane
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity, qfalse, ent->s.effect2Time);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	// cut the velocity to keep from bouncing forever
	VectorScale(ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta);

	if (trace->plane.normal[2] >= 0.7f || VectorLength(ent->s.pos.trDelta) < 16)
	{
		if (VectorLength(ent->s.pos.trDelta) < 40 && trace->plane.normal[2] > 0)
		{
			if (trace->plane.normal[2] > 0.7f &&
			    (trace->plane.normal[0] != 0.0f || trace->plane.normal[1] != 0.0f ||
			     trace->plane.normal[2] != 1.0f))
			{
				vec3_t  forward, start, end;
				trace_t tr;
				vec3_t  outAxis[3];

				AngleVectors(ent->r.currentAngles, forward, NULL, NULL);
				VectorCopy(trace->plane.normal, outAxis[2]);
				ProjectPointOnPlane(outAxis[0], forward, outAxis[2]);

				if (!VectorNormalize(outAxis[0]))
				{
					AngleVectors(ent->r.currentAngles, NULL, NULL, forward);
					ProjectPointOnPlane(outAxis[0], forward, outAxis[2]);
					VectorNormalize(outAxis[0]);
				}

				CrossProduct(outAxis[0], outAxis[2], outAxis[1]);

				VectorNegate(outAxis[1], outAxis[1]);

				AxisToAngles(outAxis, ent->r.currentAngles);
				VectorMA(trace->endpos, -64.f, trace->plane.normal, end);
				VectorMA(trace->endpos, 1.f, trace->plane.normal, start);

				if (ent->s.eType == ET_CORPSE)
				{
					trap_TraceCapsule(&tr, start, NULL, NULL, end, ent->s.number, MASK_SOLID);
				}
				else
				{
					trap_Trace(&tr, start, NULL, NULL, end, ent->s.number, MASK_SOLID);
				}

				if (!tr.startsolid)
				{
					VectorMA(trace->endpos, tr.fraction * -64.f, trace->plane.normal, trace->endpos);
				}

				// make sure it is off ground
				VectorMA(trace->endpos, 1.0f, trace->plane.normal, trace->endpos);
			}
			else
			{
				trace->endpos[2] += 1.0f;
			}

			G_SetAngle(ent, ent->r.currentAngles);
			SnapVector(trace->endpos);
			G_SetOrigin(ent, trace->endpos);
			ent->s.groundEntityNum = trace->entityNum;

			if (ent->s.groundEntityNum != ENTITYNUM_WORLD)
			{
				ent->s.pos.trType = TR_GRAVITY_PAUSED;  // allow entity to be affected by gravity again
			}

			return;
		}

		// bounce the angles
		if (ent->s.apos.trType != TR_STATIONARY)
		{
			VectorScale(ent->s.apos.trDelta, ent->physicsBounce, ent->s.apos.trDelta);
			ent->s.apos.trTime = level.time;
		}
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

/**
 * @brief Run item prop.
 * @param[in,out] ent
 * @param[in] origin
 */
void G_RunItemProp(gentity_t *ent, vec3_t origin)
{
	gentity_t *traceEnt;
	trace_t   trace;
	gentity_t *owner = &g_entities[ent->r.ownerNum];
	vec3_t    start;
	vec3_t    end;

	VectorCopy(ent->r.currentOrigin, start);
	start[2] += 1;

	VectorCopy(origin, end);
	end[2] += 1;

	trap_Trace(&trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, end,
	           ent->r.ownerNum, MASK_SHOT);

	traceEnt = &g_entities[trace.entityNum];

	if (traceEnt && traceEnt->takedamage && traceEnt != ent)
	{
		ent->enemy = traceEnt;
	}

	if (owner->client && trace.startsolid && traceEnt != owner && traceEnt != ent /* && !traceEnt->active*/)
	{
		ent->takedamage = qfalse;
		ent->die(ent, ent, NULL, 10, MOD_UNKNOWN);
		Prop_Break_Sound(ent);

		return;
	}
	else if (trace.surfaceFlags & SURF_NOIMPACT)
	{
		ent->takedamage = qfalse;

		Props_Chair_Skyboxtouch(ent);

		return;
	}
}

/**
 * @brief Run item.
 * @param[in,out] ent
 */
void G_RunItem(gentity_t *ent)
{
	vec3_t  origin;
	trace_t tr;
	int     contents;
	int     mask = ent->clipmask ? ent->clipmask : MASK_SOLID;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if (ent->s.groundEntityNum == -1)
	{
		if (ent->s.pos.trType != TR_GRAVITY)
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if (ent->s.pos.trType == TR_STATIONARY) // check think function
	{
		G_RunThink(ent);
		return;
	}
	else if (ent->s.pos.trType == TR_GRAVITY_PAUSED)    // check if ent can start falling again
	{
		vec3_t newOrigin;

		VectorCopy(ent->r.currentOrigin, newOrigin);
		newOrigin[2] -= 4;

		if (ent->s.eType == ET_CORPSE)
		{
			trap_TraceCapsule(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, newOrigin, ent->s.number, mask);
		}
		else
		{
			trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, newOrigin, ent->s.number, mask);
		}

		if (tr.fraction == 1.f && !tr.startsolid)
		{
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trType      = TR_GRAVITY;
			ent->s.pos.trTime      = level.time;
			ent->s.groundEntityNum = -1;
		}

		G_RunThink(ent);
		return;
	}
	else if (ent->s.pos.trType == TR_LINEAR && (!ent->clipmask && !ent->r.contents))
	{
		// check think function
		G_RunThink(ent);
		return;
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin, qfalse, ent->s.effect2Time);

	// trace a line from the previous position to the current position
	if (ent->s.eType == ET_CORPSE)
	{
		trap_TraceCapsule(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		                  ent->r.ownerNum, mask);
	}
	else
	{
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		           ent->r.ownerNum, mask);
	}

	if (ent->isProp && ent->takedamage)
	{
		G_RunItemProp(ent, origin);
	}

	VectorCopy(tr.endpos, ent->r.currentOrigin);

	if (tr.startsolid)
	{
		tr.fraction = 0;
	}

	trap_LinkEntity(ent);

	// check think function
	G_RunThink(ent);

	if (tr.fraction == 1.f)
	{
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents(ent->r.currentOrigin, -1);
	if (contents & CONTENTS_NODROP)
	{
		if (ent->item && ent->item->giType == IT_TEAM)
		{
			Team_ReturnFlag(ent);
		}
		else
		{
			G_FreeEntity(ent);
		}
		return;
	}

	G_BounceItem(ent, &tr);
}
