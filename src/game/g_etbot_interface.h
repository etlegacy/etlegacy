/*
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
 * @file g_etbot_interface.h
 * @brief ET <-> Omni-Bot interface header file.
 */

#ifndef INCLUDE_G_ETBOT_INTERFACE_H
#define INCLUDE_G_ETBOT_INTERFACE_H

//#include "q_shared.h"
#include "g_local.h"

//#define NO_BOT_SUPPORT

#define OMNIBOT_MODNAME    MODNAME
#define OMNIBOT_MODVERSION ETLEGACY_VERSION_SHORT

//////////////////////////////////////////////////////////////////////////
/**
 * @enum BotFlagOptions
 * @brief g_OmniBotFlags bits
 */
enum BotFlagOptions
{
	OBF_DONT_XPSAVE        = (1 << 0),  ///< Disables XPSave for bots
	OBF_DONT_MOUNT_TANKS   = (1 << 1),  ///< Bots cannot mount tanks
	OBF_DONT_MOUNT_GUNS    = (1 << 2),  ///< Bots cannot mount emplaced guns
	OBF_DONT_SHOW_BOTCOUNT = (1 << 3),  ///< Don't count bots
	OBF_GIBBING            = (1 << 4),  ///< Bots will target ungibbed enemies
	OBF_TRIGGER_MINES      = (1 << 5),  ///< Bots will trigger team and spotted mines
	OBF_SHOVING            = (1 << 6),  ///< Bots can use g_shove
	OBF_NEXT_FLAG          = (1 << 16), ///< mod specific flags start from here
};
//////////////////////////////////////////////////////////////////////////

int Bot_Interface_Init();
void Bot_Interface_InitHandles();
int Bot_Interface_Shutdown();

void Bot_Interface_Update();

void Bot_Interface_ConsoleCommand(void);

qboolean Bot_Util_AllowPush(int weaponId);
qboolean Bot_Util_CheckForSuicide(gentity_t *ent);
const char *_GetEntityName(gentity_t *_ent);

//void Bot_Util_AddGoal(gentity_t *_ent, int _goaltype, int _team, const char *_tag, obUserData *_bud);
void Bot_Util_SendTrigger(gentity_t *_ent, gentity_t *_activator, const char *_tagname, const char *_action);

int Bot_WeaponGameToBot(int weapon);
int Bot_TeamGameToBot(int team);
int Bot_PlayerClassGameToBot(int playerClass);

void Bot_Queue_EntityCreated(gentity_t *pEnt);
void Bot_Event_EntityDeleted(gentity_t *pEnt);

//////////////////////////////////////////////////////////////////////////

void Bot_Event_ClientConnected(int _client, qboolean _isbot);
void Bot_Event_ClientDisConnected(int _client);

void Bot_Event_ResetWeapons(int _client);
void Bot_Event_AddWeapon(int _client, int _weaponId);
void Bot_Event_RemoveWeapon(int _client, int _weaponId);

void Bot_Event_TakeDamage(int _client, gentity_t *_ent);
void Bot_Event_Death(int _client, gentity_t *_killer, const char *_meansofdeath);
void Bot_Event_KilledSomeone(int _client, gentity_t *_victim, const char *_meansofdeath);
void Bot_Event_Revived(int _client, gentity_t *_whodoneit);
void Bot_Event_Healed(int _client, gentity_t *_whodoneit);
void Bot_Event_ReceivedAmmo(int _client, gentity_t *_whodoneit);

void Bot_Event_FireWeapon(int _client, int _weaponId, gentity_t *_projectile);
void Bot_Event_PreTriggerMine(int _client, gentity_t *_mine);
void Bot_Event_PostTriggerMine(int _client, gentity_t *_mine);
void Bot_Event_MortarImpact(int _client, vec3_t _pos);

void Bot_Event_Spectated(int _client, int _who);

void Bot_Event_ChatMessage(int _client, gentity_t *_source, int _type, const char *_message);
void Bot_Event_VoiceMacro(int _client, gentity_t *_source, int _type, const char *_message);

void Bot_Event_Sound(gentity_t *_source, int _sndtype, const char *_name);

void Bot_Event_FireTeamCreated(int _client, int _fireteamnum);
void Bot_Event_FireTeamDestroyed(int _client);
void Bot_Event_JoinedFireTeam(int _client, gentity_t *leader);
void Bot_Event_LeftFireTeam(int _client);
void Bot_Event_InviteFireTeam(int _inviter, int _invitee);
void Bot_Event_FireTeam_Proposal(int _client, int _proposed);
void Bot_Event_FireTeam_Warn(int _client, int _warned);

// goal helpers
void Bot_AddDynamiteGoal(gentity_t *_ent, int _team, const char *_tag);
void Bot_AddFallenTeammateGoals(gentity_t *_teammate, int _team);
void AddDeferredGoal(gentity_t *ent);
void UpdateGoalEntity(gentity_t *oldent, gentity_t *newent);
void GetEntityCenter(gentity_t *ent, vec3_t pos);

#endif
