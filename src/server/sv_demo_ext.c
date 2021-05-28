/*
 * Copyright (C) 2012 Stephen Larroque <lrq3000@gmail.com>
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
 * @file sv_demo_ext.c
 * @brief Server side demo recording (supplementary functions)
 */

#include "../game/g_local.h" // get both the definitions of gentity_t (to get gentity_t->health field) AND sharedEntity_t, so that we can convert a sharedEntity_t into a gentity_t (see more details in SV_GentityUpdateHealthField() notes)
#include "../qcommon/q_shared.h" // needed so that the public declarations in server.h can access these functions (because server.h links to qcommon.h, so that it does server.h->qcommon.h->sv_demo_ext.c -- in the end, no includes redundancy conflicts and every server files can access these functions!)

//#include "server.h" // DO NOT DO THAT! if you include server.h directly, you won't be able to include g_local.h, and you're stuck!

/***********************************************
 * AUXILIARY FUNCTIONS: UPDATING OF GENTITY_T->HEALTH
 *
 * Functions used to update some special aspects of the demo and which need to be separated from the main sv_demo.c file because of different includes that are necessary
 ***********************************************/

/*
 * @brief Get the value of the gentity_t->health field (only used for testing purposes)
 * @param[in] gent
 * @return
 *
 * @note Unused - Test purpose
int SV_GentityGetHealthField(sharedEntity_t *gent)
{
	gentity_t *ent = (gentity_t *)gent;

	//Com_Printf("DEMODEBUG GENGETFIELD: health: %i\n", ent->health);
	return ent->health;
}
*/

/*
 * @brief Set the value of the gentity_t->health field (only used for testing purposes)
 * @param[out] gent
 * @param[in] value
 *
 * @note Unused - Test purpose
void SV_GentitySetHealthField(sharedEntity_t *gent, int value)
{
	gentity_t *ent = (gentity_t *)gent;

	ent->health = value;
}
*/

/**
 * @brief Update the value of the gentity_t->health field with playerState_t->stats[STAT_HEALTH] for a given player
 *
 * @details
 * You need to supply the player's sharedEntity and playerState (because since we have special includes here,
 * we don't have access to the functions that can return a player from their int id).
 *
 * The concrete effect is that when replaying a demo,
 * the players' health will be updated on the HUD (if it weren't for this function to update the health,
 * the health wouldn't change and stay kinda static).
 *
 * @param[out] gent
 * @param[in] player
 *
 * @note 1. We need to do that because the demo can only records this stats[stat_health], which is concretely the same as gentity_t->health.
 * The latter should have been removed altogether considering the comments in g_active.c :
 * (ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...),
 * but it seems to have survived because it allows non-player entities to have health, such as obelisks.
 * And weirdly, gentity_t->health has ascendence over stat_health (meaning stat_health is updated following gentity_t->health, but never the other way around),
 * when for example stat_armor has ascendence over anything else of the same kind, so here we have to update it by ourselves.
 *
 * @note 2. This works pretty simply: sharedEntity_t = gentity_t but with only the first 2 fields declared (entityShared_t and entityState_t),
 * but all the other fields are still in memory!
 * We only need to get a valid declaration for gentity_t (which we do by doing the right includes at the top of this file, in g_local.h),
 * and then we can convert the limited sharedEntity_t into a gentity_t with all the fields!
 */
void SV_GentityUpdateHealthField(sharedEntity_t *gent, playerState_t *player)
{
	gentity_t *ent = (gentity_t *)gent; // convert the sharedEntity_t to a gentity_t by using a simple cast (now that we have included g_local.h that contains the definition of gentity_t, and at the same time we have linked to g_public.h via g_local.h with the definition of sharedEntity_t)

	ent->health = player->stats[STAT_HEALTH]; // update player's health from playerState_t->stats[STAT_HEALTH] field

	return;
}

/**
* @brief Movers running over items causing crashes because the item field is null.
* @param[in] gent
*
* @note It would be better to set the item to correct item 'ent->item = ent->s.modelindex' and not a dummy, but can't access 'bg_itemlist' at the time.
*/
void SV_GentityUpdateItemField(sharedEntity_t *gent)
{
	gentity_t *ent = (gentity_t *)gent;
	//ent->item = BG_GetItem(ent->s.modelindex);
	//gitem_t *item = &bg_itemlist[ent->s.modelindex];

	gitem_t item =
	{
		ITEM_NONE,
		NULL,                   // classname
		NULL,                   // pickup_sound
		{
			0,                  // world_model[0]
			0,                  // world_model[1]
			0                   // world_model[2]
		},
		NULL,                   // icon
		NULL,                   // ammoicon
		NULL,                   // pickup_name
		0,                      // quantity
		IT_BAD,                 // giType
		WP_NONE,                // giWeapon
		PW_NONE,                // giPowerUp
	};

	ent->item = &item;

	return;
}

/**
* @brief Flamethrower causing crashes because the parent field is null.
* @param[in] gent
*/
void SV_GentityUpdateParentField(sharedEntity_t *gent, sharedEntity_t *parent)
{
	gentity_t *ent = (gentity_t *)gent;

	if (!ent->parent)
	{
		ent->parent = (gentity_t *)parent;
	}

	return;
}
