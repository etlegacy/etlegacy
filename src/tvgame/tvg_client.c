/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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
 * @file tvg_client.c
 * @brief Client functions that don't happen every frame
 */

#include "tvg_local.h"
#include "../../etmain/ui/menudef.h"
#include "../game/g_strparse.h"

#ifdef FEATURE_LUA
#include "tvg_lua.h"
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

	for (i = 0; i < num; i++)
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
	spot = SelectRandomDeathmatchSpawnPoint();

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
	VectorCopy(level.intermission_origin, origin);
	VectorCopy(level.intermission_angle, angles);

	return NULL;
}

//======================================================================

/**
 * @brief TVG_SetClientViewAngle
 * @param[in,out] ent
 * @param[in] angle
 */
void TVG_SetClientViewAngle(gclient_t *client, const vec3_t angle)
{
	int i;
	int cmdAngle;

	// set the delta angle
	for (i = 0 ; i < 3 ; i++)
	{
		cmdAngle                        = ANGLE2SHORT(angle[i]);
		client->ps.delta_angles[i] = cmdAngle - client->pers.cmd.angles[i];
	}
	VectorCopy(angle, client->ps.viewangles);
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
		if (BG_IsSkillAvailable(client->sess.skill, SK_LIGHT_WEAPONS, SK_LIGHT_WEAPONS_EXTRA_AMMO))
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SMG)
	{
		if (BG_IsSkillAvailable(client->sess.skill, SK_LIGHT_WEAPONS, SK_LIGHT_WEAPONS_EXTRA_AMMO))
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLENADE)
	{
		if (BG_IsSkillAvailable(client->sess.skill, SK_EXPLOSIVES_AND_CONSTRUCTION, SK_ENGINEER_EXTRA_GRENADE))
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 4;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_GRENADE)
	{
		if (client->sess.playerType == PC_ENGINEER)
		{
			if (BG_IsSkillAvailable(client->sess.skill, SK_EXPLOSIVES_AND_CONSTRUCTION, SK_ENGINEER_EXTRA_GRENADE))
			{
				client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 4;
			}
		}
		if (client->sess.playerType == PC_MEDIC)
		{
			if (BG_IsSkillAvailable(client->sess.skill, SK_FIRST_AID, SK_MEDIC_EXTRA_AMMO))
			{
				client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 1;
			}
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SYRINGUE)
	{
		if (BG_IsSkillAvailable(client->sess.skill, SK_FIRST_AID, SK_MEDIC_RESOURCES))
		{
			client->ps.ammo[GetWeaponTableData(weaponNum)->ammoIndex] += 2;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLE)
	{
		if (BG_IsSkillAvailable(client->sess.skill, SK_LIGHT_WEAPONS, SK_LIGHT_WEAPONS_EXTRA_AMMO))
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
	}
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
	weapon_t         weaponPrimary;

	if (client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

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
		if (BG_IsSkillAvailable(client->sess.skill, classInfo->classPrimaryWeapons[i].skill, classInfo->classPrimaryWeapons[i].minSkillLevel)
		    && client->sess.skill[classInfo->classPrimaryWeapons[i].skill] >= classInfo->classPrimaryWeapons[i].minSkillLevel)
		{
			if (classInfo->classPrimaryWeapons[i].weapon == client->sess.playerWeapon)
			{
				weaponClassInfo = &classInfo->classPrimaryWeapons[i];
				break;
			}
		}
	}

	// add primary weapon (set to current weapon)
	weaponPrimary = weaponClassInfo->weapon;
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
		if (BG_IsSkillAvailable(client->sess.skill, classInfo->classSecondaryWeapons[i].skill, classInfo->classSecondaryWeapons[i].minSkillLevel)
		    && client->sess.skill[classInfo->classSecondaryWeapons[i].skill] >= classInfo->classSecondaryWeapons[i].minSkillLevel)
		{
			if (classInfo->classSecondaryWeapons[i].weapon == client->sess.playerWeapon2)
			{
				weaponClassInfo = &classInfo->classSecondaryWeapons[i];
				break;
			}
		}
	}

	// add secondary weapon, but only if it's different from the primary one
	if (weaponClassInfo->weapon != weaponPrimary)
	{
		AddWeaponToPlayer(client, weaponClassInfo->weapon, weaponClassInfo->startingAmmo, weaponClassInfo->startingClip, qfalse);
	}

	//
	// special weapons and items
	//
	for (i = 0; i < MAX_WEAPS_PER_CLASS && classInfo->classMiscWeapons[i].weapon; i++)
	{
		weaponClassInfo = &classInfo->classMiscWeapons[i];

		if (BG_IsSkillAvailable(client->sess.skill, classInfo->classMiscWeapons[i].skill, classInfo->classMiscWeapons[i].minSkillLevel)
		    && client->sess.skill[classInfo->classMiscWeapons[i].skill] >= classInfo->classMiscWeapons[i].minSkillLevel)
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

	if (BG_IsSkillAvailable(client->sess.skill, SK_BATTLE_SENSE, SK_BATTLE_SENSE_HEALTH))
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

	rc = Q_sscanf(ipadd, "%3u.%3u.%3u.%3u:%u%c", &b1, &b2, &b3, &b4, &port, &c);
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
	gclient_t  *client = level.clients + clientNum;
	int        i;
	const char *userinfo_ptr                 = NULL;
	char       cs_key[MAX_STRING_CHARS]      = "";
	char       cs_value[MAX_STRING_CHARS]    = "";
	char       cs_cg_uinfo[MAX_STRING_CHARS] = "";
	char       cs_skill[MAX_STRING_CHARS]    = "";
	char       *reason;
	char       *s;
	char       cs_name[MAX_NETNAME] = "";
	char       oldname[MAX_NAME_LENGTH];
	char       userinfo[MAX_INFO_STRING];
	char       skillStr[16] = "";
	char       medalStr[16] = "";

	client->ps.clientNum = clientNum;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	reason = CheckUserinfo(clientNum, userinfo);
	if (reason)
	{
		G_Printf("ClientUserinfoChanged: CheckUserinfo: client %d: %s\n", clientNum, reason);
		trap_DropClient(clientNum, va("^1%s", "Bad userinfo."), 0);
		return;
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
			trap_DropClient(clientNum, "Bad userinfo.", 0);
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
			if (CompareIPNoPort(client->pers.client_ip, cs_value) == qfalse)
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
				trap_DropClient(clientNum, va("Name too long (>%d). Plase change your name.", MAX_NETNAME - 1), 0);
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
			if (strcmp(client->pers.cl_guid, cs_value))
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
		//CP("cp \"You cannot assign an empty playername! Your name has been reset.\"");
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

	if (cs_cg_uinfo[0])
	{
		Q_sscanf(cs_cg_uinfo, "%u %u %u",
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

	client->pers.activateLean = (client->pers.clientFlags & CGF_ACTIVATELEAN) != 0 ? qtrue : qfalse;

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
	s = va("n\\%s\\t\\%i\\c\\%i\\lc\\%i\\r\\%i\\m\\%s\\s\\%s\\dn\\%i\\w\\%i\\lw\\%i\\sw\\%i\\lsw\\%i\\mu\\%i\\ref\\%i\\sc\\%i\\u\\%u",
	       client->pers.netname,
	       client->sess.sessionTeam,
	       client->sess.playerType,
	       client->sess.latchPlayerType,
	       client->sess.rank,
	       medalStr,
	       skillStr,
	       client->disguiseClientNum,
	       client->sess.playerWeapon,
	       client->sess.latchPlayerWeapon,
	       client->sess.playerWeapon2,
	       client->sess.latchPlayerWeapon2,
	       client->sess.muted ? 1 : 0,
	       client->sess.referee,
	       client->sess.shoutcaster
	       );

	G_LogPrintf("ClientUserinfoChanged: %i %s\n", clientNum, s);
	G_DPrintf("ClientUserinfoChanged: %i :: %s\n", clientNum, s);
}

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
	gclient_t  *client = level.clients + clientNum;
	const char *userinfo_ptr;
	char       userinfo[MAX_INFO_STRING];
	char       cs_key[MAX_STRING_CHARS]      = "";
	char       cs_value[MAX_STRING_CHARS]    = "";
	char       cs_ip[MAX_STRING_CHARS]       = "";
	char       cs_password[MAX_STRING_CHARS] = "";
	char       cs_name[MAX_NETNAME + 1]      = "";
	char       cs_guid[MAX_GUID_LENGTH + 1]  = "";
	//char       cs_rate[MAX_STRING_CHARS]     = "";
#ifdef FEATURE_LUA
	char reason[MAX_STRING_CHARS] = "";
#endif

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
			Q_strncpyz(cs_name, cs_value, MAX_NETNAME + 1);
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

	// don't permit empty name
	if (!strlen(cs_name))
	{
		return va("Bad name: Name is empty. Please change your name.");
	}

	// don't permit long names ... - see also MAX_NETNAME
	if (strlen(cs_name) >= MAX_NAME_LENGTH)
	{
		return va("Bad name: Name too long (>%d). Please change your name.", MAX_NAME_LENGTH - 1);
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
	if (client->pers.connected != CON_DISCONNECTED)
	{
		G_LogPrintf("Forcing disconnect on active client: %i\n", (int)(client - level.clients));
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect(client - level.clients);
	}

	Com_Memset(client, 0, sizeof(*client));

	client->pers.connected   = CON_CONNECTING;
	client->pers.connectTime = level.time;

	client->disguiseClientNum = -1;

	// Set the client ip and guid
	Q_strncpyz(client->pers.client_ip, cs_ip, MAX_IP4_LENGTH);
	Q_strncpyz(client->pers.cl_guid, cs_guid, MAX_GUID_LENGTH + 1);

	//if (firstTime)
	{
		client->pers.initialSpawn = qtrue;

		// read or initialize the session data
		G_InitSessionData(client, userinfo);
		client->pers.enterTime            = level.time;
		client->ps.persistant[PERS_SCORE] = 0;
	}
	//else
	//{
	//	G_ReadSessionData(client);
	//}

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

	if (firstTime)
	{
		// force into spectator
		client->sess.sessionTeam     = TEAM_SPECTATOR;
		client->sess.spectatorState  = SPECTATOR_FREE;
		client->sess.spectatorClient = 0;

	}

	// get and distribute relevent paramters
	G_LogPrintf("ClientConnect: %i\n", clientNum);

	ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	// disabled for bots - see join message ... make cvar ?
	if (firstTime)
	{

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
	gclient_t *client = level.clients + clientNum;
	int       flags;
	int       spawn_count, lives_left;
	int       stat_xp, score; // restore xp & score

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
	stat_xp = client->ps.stats[STAT_XP];
	score   = client->ps.persistant[PERS_SCORE];

	Com_Memset(&client->ps, 0, sizeof(client->ps));

	client->ps.persistant[PERS_SCORE] = score;

	if (client->sess.spectatorState == SPECTATOR_FREE) // restore xp
	{
		client->ps.stats[STAT_XP] = stat_xp;
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

	// init objective indicator if already set
	if (level.flagIndicator > 0)
	{
		//G_clientFlagIndicator(ent);
	}

	ClientSpawn(client, qfalse, qtrue, qtrue);

	if (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES)
	{
		client->inactivityTime        = level.time + G_InactivityValue * 1000;
		client->inactivitySecondsLeft = G_InactivityValue;
	}
	else
	{
		client->inactivityTime        = level.time + G_SpectatorInactivityValue * 1000;
		client->inactivitySecondsLeft = G_SpectatorInactivityValue;
	}

	if (client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_SendServerCommand(-1, va("print \"[lof]" S_COLOR_WHITE "%s" S_COLOR_WHITE " [lon]entered the game\n\"", client->pers.netname));
	}

	G_LogPrintf("ClientBegin: %i\n", clientNum);

	// count current clients and rank for scoreboard
	CalculateRanks();
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
void ClientSpawn(gclient_t *client, qboolean revived, qboolean teamChange, qboolean restoreHealth)
{
	int                index = client - level.clients;
	vec3_t             spawn_origin, spawn_angles;
	int                i;
	clientPersistant_t savedPers;
	clientSession_t    savedSess;
	int                persistant[MAX_PERSISTANT];
	gentity_t          *spawnPoint;
	int                flags;
	int                savedPing;
	int                savedTeam;
	int                savedDeathTime;
	int                oldWeapon, oldNextWeapon, oldWeaponstate, oldSilencedSideArm;
	int                oldAmmo[MAX_WEAPONS];                          // total amount of ammo
	int                oldAmmoclip[MAX_WEAPONS];                      // ammo in clip
	int                oldWeapons[MAX_WEAPONS / (sizeof(int) * 8)];   // 64 bits for weapons held

	client->pers.lastSpawnTime            = level.time;
	client->pers.lastBattleSenseBonusTime = level.timeCurrent;
	client->pers.lastHQMineReportTime     = level.timeCurrent;

	spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags  = client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;

	flags |= (client->ps.eFlags & EF_VOTED);

	if (!teamChange)
	{
		flags |= (client->ps.eFlags & EF_READY);
	}
	// clear everything but the persistant data

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
	client->ps.persistant[PERS_TEAM]        = client->sess.sessionTeam;
	client->ps.persistant[PERS_HWEAPON_USE] = 0;

	// breathbar
	client->ps.stats[STAT_AIRLEFT] = HOLDBREATHTIME;
	client->airOutTime             = level.time + HOLDBREATHTIME;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags                 = flags;
	client->deathTime                 = savedDeathTime;

	client->ps.classWeaponTime = -999999;

	// setup the bounding boxes and viewheights for prediction
	VectorCopy(playerMins, client->ps.mins);
	VectorCopy(playerMaxs, client->ps.maxs);

	client->ps.crouchViewHeight = CROUCH_VIEWHEIGHT;
	client->ps.standViewHeight  = DEFAULT_VIEWHEIGHT;
	client->ps.deadViewHeight   = DEAD_VIEWHEIGHT;

	client->ps.crouchMaxZ = client->ps.maxs[2] - (client->ps.standViewHeight - client->ps.crouchViewHeight);

	client->ps.runSpeedScale    = 0.8f;
	client->ps.sprintSpeedScale = 1.1f;
	client->ps.crouchSpeedScale = 0.25f;
	client->ps.weaponstate      = WEAPON_READY;

	client->ps.stats[STAT_SPRINTTIME] = SPRINTTIME;
	client->ps.sprintExertTime        = 0;

	client->ps.friction = 1.0f;

	// retrieve from the persistant storage (we use this in pmoveExt_t beause we need it in bg_*)
	client->pmext.bAutoReload = client->pers.bAutoReloadAux;

	client->ps.clientNum = level.ettvMasterPs.clientNum;

	// start tracing legs and head again if they were in solid
	client->pmext.deadInSolid = qfalse;

	trap_GetUsercmd(index, &client->pers.cmd);

	// increases stats[STAT_MAX_HEALTH] based on # of medics in game
	AddMedicTeamBonus(client);

	// client has (almost) no say in weapon selection when spawning
	client->pers.cmd.weapon = client->ps.weapon;

	client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	TVG_SetClientViewAngle(client, spawn_angles);
	
	client->respawnTime           = level.timeCurrent;
	client->inactivityTime        = level.time + G_InactivityValue * 1000;
	client->inactivityWarning     = qfalse;
	client->inactivitySecondsLeft = G_InactivityValue;
	client->latched_buttons       = 0;
	client->latched_wbuttons      = 0;
	client->deathTime             = 0;

	//if (level.intermissiontime)
	//{
	//	MoveClientToIntermission(ent, (EF_VOTED & client->ps.eFlags));

	//	// send current mapvote tally
	//	if (g_gametype.integer == GT_WOLF_MAPVOTE)
	//	{
	//		G_IntermissionVoteTally(ent);
	//	}
	//}
	//else

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime      = level.time - 100;
	client->pers.cmd.serverTime = level.time;

	ClientThink(index);

	// run the presend to set anything else
	ClientEndFrame(client);

	// set idle animation on weapon
	client->ps.weapAnim = ((client->ps.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | WEAP_IDLE1;
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

#ifdef FEATURE_LUA
	// LUA API callbacks
	G_LuaHook_ClientDisconnect(clientNum);
#endif

	G_RemoveFromAllIgnoreLists(clientNum);

	// stop any following clients
	//for (i = 0 ; i < level.numConnectedClients ; i++)
	//{
	//	flag = g_entities + level.sortedClients[i];
	//	if (flag->client->sess.sessionTeam == TEAM_SPECTATOR
	//	    && flag->client->sess.spectatorState == SPECTATOR_FOLLOW
	//	    && flag->client->sess.spectatorClient == clientNum)
	//	{
	//		StopFollowing(flag);
	//	}
	//	if ((flag->client->ps.pm_flags & PMF_LIMBO) && flag->client->sess.spectatorClient == clientNum)
	//	{
	//		Cmd_FollowCycle_f(flag, 1, qfalse);
	//	}
	//}

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
