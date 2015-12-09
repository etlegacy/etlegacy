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
 */
/**
 * @file g_stats.c
 */

#include "g_local.h"

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

void G_LogDeath(gentity_t *ent, weapon_t weap)
{
	if (!ent->client)
	{
		return;
	}

	ent->client->pers.playerStats.weaponStats[weap].killedby++;
}

void G_LogKill(gentity_t *ent, weapon_t weap)
{
	if (!ent->client)
	{
		return;
	}

	ent->client->pers.playerStats.weaponStats[weap].kills++;
}

void G_LogTeamKill(gentity_t *ent, weapon_t weap)
{
	if (!ent->client)
	{
		return;
	}

	ent->client->pers.playerStats.weaponStats[weap].teamkills++;
}

void G_LogRegionHit(gentity_t *ent, hitRegion_t hr)
{
	if (!ent->client)
	{
		return;
	}
	ent->client->pers.playerStats.hitRegions[hr]++;
}

void G_PrintAccuracyLog(gentity_t *ent)
{
	int  i;
	char buffer[2048];

	Q_strncpyz(buffer, "WeaponStats", 2048);

	for (i = WP_KNIFE; i < WP_NUM_WEAPONS; i++)
	{
		if (BG_WeapStatForWeapon(i) == WS_MAX)
		{
			continue;
		}

		Q_strcat(buffer, 2048, va(" %i %i %i",
		                          ent->client->pers.playerStats.weaponStats[i].kills,
		                          ent->client->pers.playerStats.weaponStats[i].killedby,
		                          ent->client->pers.playerStats.weaponStats[i].teamkills));
	}

	Q_strcat(buffer, 2048, va(" %i", ent->client->pers.playerStats.selfkills));

	for (i = 0; i < HR_NUM_HITREGIONS; i++)
	{
		Q_strcat(buffer, 2048, va(" %i", ent->client->pers.playerStats.hitRegions[i]));
	}

	Q_strcat(buffer, 2048, va(" %i", 6 /*level.numOidTriggers*/));

	for (i = 0; i < 6 /*level.numOidTriggers*/; i++)
	{
		Q_strcat(buffer, 2048, va(" %i", ent->client->pers.playerStats.objectiveStats[i]));
		Q_strcat(buffer, 2048, va(" %i", ent->client->sess.sessionTeam == TEAM_AXIS ? level.objectiveStatsAxis[i] : level.objectiveStatsAllies[i]));
	}

	trap_SendServerCommand(ent - g_entities, buffer);
}

void G_SetPlayerScore(gclient_t *client)
{
	int i;

	for (client->ps.persistant[PERS_SCORE] = 0, i = 0; i < SK_NUM_SKILLS; i++)
	{
		client->ps.persistant[PERS_SCORE] += client->sess.skillpoints[i];
	}
}

void G_SetPlayerSkill(gclient_t *client, skillType_t skill)
{
	int i;

#ifdef FEATURE_LUA
	// *LUA* API callbacks
	if (G_LuaHook_SetPlayerSkill(client - level.clients, skill))
	{
		return;
	}
#endif

	for (i = NUM_SKILL_LEVELS - 1; i >= 0; i--)
	{
		if (skillLevels[skill][i] != -1 && client->sess.skillpoints[skill] >= skillLevels[skill][i])
		{
			client->sess.skill[skill] = i;
			break;
		}
	}

	G_SetPlayerScore(client);
}

extern qboolean AddWeaponToPlayer(gclient_t *client, weapon_t weapon, int ammo, int ammoclip, qboolean setcurrent);

// Local func to actual do skill upgrade, used by both MP skill system, and SP scripted skill system
void G_UpgradeSkill(gentity_t *ent, skillType_t skill)
{
	int i;

#ifdef FEATURE_LUA
	// *LUA* API callbacks
	if (G_LuaHook_UpgradeSkill(g_entities - ent, skill))
	{
		return;
	}
#endif

	// See if this is the first time we've reached this skill level
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		if (i == skill)
		{
			continue;
		}

		if (ent->client->sess.skill[skill] <= ent->client->sess.skill[i])
		{
			break;
		}
	}

	G_DebugAddSkillLevel(ent, skill);

	if (i == SK_NUM_SKILLS)
	{
		// increase rank
		ent->client->sess.rank++;
	}

	if (ent->client->sess.rank >= 4)
	{
		int cnt = 0;

		// count the number of maxed out skills
		for (i = 0; i < SK_NUM_SKILLS; i++)
		{
			if (ent->client->sess.skill[i] >= 4)
			{
				cnt++;
			}
		}

		ent->client->sess.rank = cnt + 3;
		if (ent->client->sess.rank > 10)
		{
			ent->client->sess.rank = 10;
		}
	}

	ClientUserinfoChanged(ent - g_entities);

	// Give em rightaway
	if (skill == SK_BATTLE_SENSE && ent->client->sess.skill[skill] == 1)
	{
		if (AddWeaponToPlayer(ent->client, WP_BINOCULARS, 1, 0, qfalse))
		{
			ent->client->ps.stats[STAT_KEYS] |= (1 << INV_BINOCS);
		}
	}
	else if (skill == SK_FIRST_AID && ent->client->sess.playerType == PC_MEDIC && ent->client->sess.skill[skill] == 4)
	{
		AddWeaponToPlayer(ent->client, WP_MEDIC_ADRENALINE, ent->client->ps.ammo[BG_FindAmmoForWeapon(WP_MEDIC_ADRENALINE)], ent->client->ps.ammoclip[BG_FindClipForWeapon(WP_MEDIC_ADRENALINE)], qfalse);
	}
}

void G_LoseSkillPoints(gentity_t *ent, skillType_t skill, float points)
{
	int   oldskill;
	float oldskillpoints;

	if (!ent->client)
	{
		return;
	}

	// no skill loss during warmup
	if (g_gamestate.integer != GS_PLAYING)
	{
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_AXIS && ent->client->sess.sessionTeam != TEAM_ALLIES)
	{
		return;
	}

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		return; // no xp in LMS
	}

	oldskillpoints                        = ent->client->sess.skillpoints[skill];
	ent->client->sess.skillpoints[skill] -= points;

	// see if player increased in skill
	oldskill = ent->client->sess.skill[skill];
	G_SetPlayerSkill(ent->client, skill);
	if (oldskill != ent->client->sess.skill[skill])
	{
		ent->client->sess.skill[skill]       = oldskill;
		ent->client->sess.skillpoints[skill] = skillLevels[skill][oldskill];
	}

	G_Printf("%s ^7just lost %.0f skill points for skill %s\n", ent->client->pers.netname, oldskillpoints - ent->client->sess.skillpoints[skill], skillNames[skill]);

	level.teamScores[ent->client->ps.persistant[PERS_TEAM]]        -= oldskillpoints - ent->client->sess.skillpoints[skill];
	level.teamXP[skill][ent->client->sess.sessionTeam - TEAM_AXIS] -= oldskillpoints - ent->client->sess.skillpoints[skill];
}

void G_ResetXP(gentity_t *ent)
{
	int i = 0;
	int ammo[MAX_WEAPONS], ammoclip[MAX_WEAPONS];
	int oldWeapon; //, newWeapon;

	if (!ent || !ent->client)
	{
		return;
	}

	ent->client->sess.rank = 0;
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		ent->client->sess.skillpoints[i] = 0.0f;
		ent->client->sess.skill[i]       = 0;
	}

	G_CalcRank(ent->client);
	ent->client->ps.stats[STAT_XP]         = 0;
	ent->client->ps.persistant[PERS_SCORE] = 0;

	// zero out all weapons and grab the default weapons for a player of this XP level.
	// backup..
	memcpy(ammo, ent->client->ps.ammo, sizeof(ammo));
	memcpy(ammoclip, ent->client->ps.ammoclip, sizeof(ammoclip));
	oldWeapon = ent->client->ps.weapon;
	// Check weapon validity, maybe dump some weapons for this (now) unskilled player..
	// It also sets the (possibly new) amounts of ammo for weapons.
	SetWolfSpawnWeapons(ent->client);
	// restore..
	//newWeapon = ent->client->ps.weapon;

	for (i = WP_NONE; i < WP_NUM_WEAPONS; ++i)    //i<MAX_WEAPONS
	{   // only restore ammo for valid weapons..
		// ..they might have lost some weapons because of skill changes.
		// Also restore to the old amount of ammo, because..
		// ..SetWolfSpawnWeapons sets amount of ammo for a fresh spawning player,
		// which is usually more than the player currently has left.
		if (COM_BitCheck(ent->client->ps.weapons, i))
		{
			if (ammo[i] < ent->client->ps.ammo[i])
			{
				ent->client->ps.ammo[i] = ammo[i];
			}
			if (ammoclip[i] < ent->client->ps.ammoclip[i])
			{
				ent->client->ps.ammoclip[i] = ammoclip[i];
			}
		}
		else
		{
			ent->client->ps.ammo[i]     = 0;
			ent->client->ps.ammoclip[i] = 0;
		}
	}
	// check if the old weapon is still valid.
	// If so, restore to the last used weapon..
	if (COM_BitCheck(ent->client->ps.weapons, oldWeapon))
	{
		ent->client->ps.weapon = oldWeapon;
	}
	ClientUserinfoChanged(ent - g_entities);
}

void G_AddSkillPoints(gentity_t *ent, skillType_t skill, float points)
{
	int oldskill;

	if (!ent->client)
	{
		return;
	}

	// no skill gaining during warmup
	if (g_gamestate.integer != GS_PLAYING)
	{
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_AXIS && ent->client->sess.sessionTeam != TEAM_ALLIES)
	{
		return;
	}

	if (g_gametype.integer == GT_WOLF_LMS)
	{
		return; // no xp in LMS
	}

	level.teamXP[skill][ent->client->sess.sessionTeam - TEAM_AXIS] += points;

	ent->client->sess.skillpoints[skill] += points;

	level.teamScores[ent->client->ps.persistant[PERS_TEAM]] += points;

	//G_Printf( "%s just got %.0f skill points for skill %s\n", ent->client->pers.netname, points, skillNames[skill] );

	// see if player increased in skill
	oldskill = ent->client->sess.skill[skill];
	G_SetPlayerSkill(ent->client, skill);
	if (oldskill != ent->client->sess.skill[skill])
	{
		// call the new func that encapsulates the skill giving behavior
		G_UpgradeSkill(ent, skill);
	}
}

/**
 * @brief Loose skill for evil tkers :E
 */
void G_LoseKillSkillPoints(gentity_t *tker, meansOfDeath_t mod, hitRegion_t hr, qboolean splash)
{
	if (!tker->client)
	{
		return;
	}

	switch (mod)
	{
	// light weapons
	case MOD_KNIFE:
	case MOD_KNIFE_KABAR:
	case MOD_LUGER:
	case MOD_COLT:
	case MOD_MP40:
	case MOD_THOMPSON:
	case MOD_STEN:
	case MOD_GARAND:
	case MOD_SILENCER:
	case MOD_FG42:
	case MOD_CARBINE:
	case MOD_KAR98:
	case MOD_SILENCED_COLT:
	case MOD_K43:
	// akimbo weapons lose score now as well
	case MOD_AKIMBO_COLT:
	case MOD_AKIMBO_LUGER:
	case MOD_AKIMBO_SILENCEDCOLT:
	case MOD_AKIMBO_SILENCEDLUGER:
	case MOD_GRENADE_LAUNCHER:
	case MOD_GRENADE_PINEAPPLE:
	// airstrike marker kills
	case MOD_SMOKEGRENADE:
		G_LoseSkillPoints(tker, SK_LIGHT_WEAPONS, 3.f);
		//G_DebugAddSkillPoints( attacker, SK_LIGHT_WEAPONS, 2.f, "kill" );
		break;

	// scoped weapons
	case MOD_GARAND_SCOPE:
	case MOD_K43_SCOPE:
	case MOD_FG42SCOPE:
	case MOD_SATCHEL:
		G_LoseSkillPoints(tker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f);
		//G_DebugAddSkillPoints( attacker, SK_LIGHT_WEAPONS, 2.f, "legshot kill" );
		break;

	case MOD_MOBILE_MG42:
	case MOD_MOBILE_BROWNING:
	case MOD_MACHINEGUN:
	case MOD_BROWNING:
	case MOD_MG42:
	case MOD_PANZERFAUST:
	case MOD_BAZOOKA:
	case MOD_FLAMETHROWER:
	case MOD_MORTAR:
	case MOD_MORTAR2:
		G_LoseSkillPoints(tker, SK_HEAVY_WEAPONS, 3.f);
		//G_DebugAddSkillPoints( attacker, SK_HEAVY_WEAPONS, 3.f, "emplaced mg42 kill" );
		break;

	case MOD_DYNAMITE:
	case MOD_LANDMINE:
	case MOD_GPG40:
	case MOD_M7:
		G_LoseSkillPoints(tker, SK_EXPLOSIVES_AND_CONSTRUCTION, 3.f);
		//G_DebugAddSkillPoints( attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, 4.f, "dynamite or landmine kill" );
		break;

	case MOD_ARTY:
	case MOD_AIRSTRIKE:
		G_LoseSkillPoints(tker, SK_SIGNALS, 3.f);
		//G_DebugAddSkillPoints( attacker, SK_SIGNALS, 4.f, "artillery kill" );
		break;

	case MOD_SHOVE:
		G_LoseSkillPoints(tker, SK_BATTLE_SENSE, 5.f);
		break;

	// no skills for anything else
	default:
		break;
	}
}

void G_AddKillSkillPoints(gentity_t *attacker, meansOfDeath_t mod, hitRegion_t hr, qboolean splash)
{
	if (!attacker->client)
	{
		return;
	}

	switch (mod)
	{
	// light weapons
	case MOD_KNIFE:
	case MOD_KNIFE_KABAR:
		G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f, "knife kill");
		break;

	case MOD_LUGER:
	case MOD_COLT:
	case MOD_MP40:
	case MOD_THOMPSON:
	case MOD_STEN:
	case MOD_GARAND:
	case MOD_SILENCER:
	case MOD_FG42:
	case MOD_CARBINE:
	case MOD_KAR98:
	case MOD_SILENCED_COLT:
	case MOD_K43:
	case MOD_AKIMBO_COLT:
	case MOD_AKIMBO_LUGER:
	case MOD_AKIMBO_SILENCEDCOLT:
	case MOD_AKIMBO_SILENCEDLUGER:
		switch (hr)
		{
		case HR_HEAD:   G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 5.f); G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 5.f, "headshot kill"); break;
		case HR_ARMS:   G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f, "armshot kill"); break;
		case HR_BODY:   G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f, "bodyshot kill"); break;
		case HR_LEGS:   G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f, "legshot kill");  break;
		default:        G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f, "kill"); break;               // for weapons that don't have localized damage
		}
		break;

	// heavy weapons
	case MOD_MOBILE_MG42:
	case MOD_MOBILE_BROWNING:
		G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
		G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "mobile machinegun kill");
		break;

	// scoped weapons
	case MOD_GARAND_SCOPE:
	case MOD_K43_SCOPE:
	case MOD_FG42SCOPE:
		switch (hr)
		{
		case HR_HEAD:   G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f); G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f, "headshot kill"); break;
		case HR_ARMS:   G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 2.f, "armshot kill"); break;
		case HR_BODY:   G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f, "bodyshot kill"); break;
		case HR_LEGS:   G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 2.f, "legshot kill"); break;
		default:        G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f); G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f, "kill"); break;             // for weapons that don't have localized damage
		}
		break;

	// misc weapons (individual handling)
	case MOD_SATCHEL:
		G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f);
		G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 5.f, "satchel charge kill");
		break;

	case MOD_MACHINEGUN:
	case MOD_BROWNING:
	case MOD_MG42:
		G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
		G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "emplaced machinegun kill");
		break;

	case MOD_PANZERFAUST:
	case MOD_BAZOOKA:
		if (splash)
		{
			G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
			G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "rocket launcher splash damage kill");
		}
		else
		{
			G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
			G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "rocket launcher direct hit kill");
		}
		break;
	case MOD_FLAMETHROWER:
		G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
		G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "flamethrower kill");
		break;
	case MOD_MORTAR:
	case MOD_MORTAR2:
		if (splash)
		{
			G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
			G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "mortar splash damage kill");
		}
		else
		{
			G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f);
			G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, 3.f, "mortar direct hit kill");
		}
		break;
	case MOD_GRENADE_LAUNCHER:
	case MOD_GRENADE_PINEAPPLE:
	// airstrike marker kills
	case MOD_SMOKEGRENADE:
		G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f);
		G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, 3.f, "hand grenade kill");
		break;
	case MOD_DYNAMITE:
	case MOD_LANDMINE:
		G_AddSkillPoints(attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, 4.f);
		G_DebugAddSkillPoints(attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, 4.f, "dynamite or landmine kill");
		break;
	case MOD_ARTY:
		G_AddSkillPoints(attacker, SK_SIGNALS, 4.f);
		G_DebugAddSkillPoints(attacker, SK_SIGNALS, 4.f, "artillery kill");
		break;
	case MOD_AIRSTRIKE:
		G_AddSkillPoints(attacker, SK_SIGNALS, 3.f);
		G_DebugAddSkillPoints(attacker, SK_SIGNALS, 3.f, "airstrike kill");
		break;
	case MOD_GPG40:
	case MOD_M7:
		G_AddSkillPoints(attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, 3.f);
		G_DebugAddSkillPoints(attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, 3.f, "rifle grenade kill");
		break;

	case MOD_SHOVE:
		G_AddSkillPoints(attacker, SK_BATTLE_SENSE, 5.f);
		G_DebugAddSkillPoints(attacker, SK_BATTLE_SENSE, 5.f, "shove kill");
		break;

	// no skills for anything else
	default:
		break;
	}
}

void G_AddKillSkillPointsForDestruction(gentity_t *attacker, meansOfDeath_t mod, g_constructible_stats_t *constructibleStats)
{
	switch (mod)
	{
	case MOD_GRENADE_LAUNCHER:
	case MOD_GRENADE_PINEAPPLE:
		G_AddSkillPoints(attacker, SK_LIGHT_WEAPONS, constructibleStats->destructxpbonus);
		G_DebugAddSkillPoints(attacker, SK_LIGHT_WEAPONS, constructibleStats->destructxpbonus, "destroying a constructible/explosive");
		break;
	case MOD_GPG40:
	case MOD_M7:
	case MOD_DYNAMITE:
	case MOD_LANDMINE:
		G_AddSkillPoints(attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, constructibleStats->destructxpbonus);
		G_DebugAddSkillPoints(attacker, SK_EXPLOSIVES_AND_CONSTRUCTION, constructibleStats->destructxpbonus, "destroying a constructible/explosive");
		break;
	case MOD_PANZERFAUST:
	case MOD_BAZOOKA:
	case MOD_MORTAR:
	case MOD_MORTAR2:
		G_AddSkillPoints(attacker, SK_HEAVY_WEAPONS, constructibleStats->destructxpbonus);
		G_DebugAddSkillPoints(attacker, SK_HEAVY_WEAPONS, constructibleStats->destructxpbonus, "destroying a constructible/explosive");
		break;
	case MOD_ARTY:
	case MOD_AIRSTRIKE:
		G_AddSkillPoints(attacker, SK_SIGNALS, constructibleStats->destructxpbonus);
		G_DebugAddSkillPoints(attacker, SK_SIGNALS, constructibleStats->destructxpbonus, "destroying a constructible/explosive");
		break;
	case MOD_SATCHEL:
		G_AddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, constructibleStats->destructxpbonus);
		G_DebugAddSkillPoints(attacker, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, constructibleStats->destructxpbonus, "destroying a constructible/explosive");
		break;
	default:
		break;
	}
}

/////// SKILL DEBUGGING ///////

static fileHandle_t skillDebugLog = -1;

void G_DebugOpenSkillLog(void)
{
	vmCvar_t mapname;
	qtime_t  ct;
	char     *s;

	if (g_debugSkills.integer < 2)
	{
		return;
	}

	trap_Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

	trap_RealTime(&ct);

	if (trap_FS_FOpenFile(va("skills-%d-%02d-%02d-%02d%02d%02d-%s.log",
	                         1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday,
	                         ct.tm_hour, ct.tm_min, ct.tm_sec,
	                         mapname.string), &skillDebugLog, FS_APPEND_SYNC) < 0)
	{
		return;
	}

	s = va("%02d:%02d:%02d : Logfile opened.\n", ct.tm_hour, ct.tm_min, ct.tm_sec);

	trap_FS_Write(s, strlen(s), skillDebugLog);
}

void G_DebugCloseSkillLog(void)
{
	qtime_t ct;
	char    *s;

	if (skillDebugLog == -1)
	{
		return;
	}

	trap_RealTime(&ct);

	s = va("%02d:%02d:%02d : Logfile closed.\n", ct.tm_hour, ct.tm_min, ct.tm_sec);

	trap_FS_Write(s, strlen(s), skillDebugLog);

	trap_FS_FCloseFile(skillDebugLog);
}

void G_DebugAddSkillLevel(gentity_t *ent, skillType_t skill)
{
	qtime_t ct;

	if (!g_debugSkills.integer)
	{
		return;
	}

	trap_SendServerCommand(ent - g_entities, va("sdbg \"^%c(SK: %2i XP: %.0f) %s: You raised your skill level to %i.\"\n",
	                                            COLOR_RED + skill, ent->client->sess.skill[skill], ent->client->sess.skillpoints[skill], skillNames[skill], ent->client->sess.skill[skill]));

	trap_RealTime(&ct);

	if (g_debugSkills.integer >= 2 && skillDebugLog != -1)
	{
		char *s = va("%02d:%02d:%02d : ^%c(SK: %2i XP: %.0f) %s: %s raised in skill level to %i.\n",
		             ct.tm_hour, ct.tm_min, ct.tm_sec,
		             COLOR_RED + skill, ent->client->sess.skill[skill], ent->client->sess.skillpoints[skill], skillNames[skill], ent->client->pers.netname, ent->client->sess.skill[skill]);
		trap_FS_Write(s, strlen(s), skillDebugLog);
	}
}

void G_DebugAddSkillPoints(gentity_t *ent, skillType_t skill, float points, const char *reason)
{
	qtime_t ct;

	if (!g_debugSkills.integer)
	{
		return;
	}

	trap_SendServerCommand(ent - g_entities, va("sdbg \"^%c(SK: %2i XP: %.0f) %s: You gained %.0fXP, reason: %s.\"\n",
	                                            COLOR_RED + skill, ent->client->sess.skill[skill], ent->client->sess.skillpoints[skill], skillNames[skill], points, reason));

	trap_RealTime(&ct);

	if (g_debugSkills.integer >= 2 && skillDebugLog != -1)
	{
		char *s = va("%02d:%02d:%02d : ^%c(SK: %2i XP: %.0f) %s: %s gained %.0fXP, reason: %s.\n",
		             ct.tm_hour, ct.tm_min, ct.tm_sec,
		             COLOR_RED + skill, ent->client->sess.skill[skill], ent->client->sess.skillpoints[skill], skillNames[skill], ent->client->pers.netname, points, reason);
		trap_FS_Write(s, strlen(s), skillDebugLog);
	}
}

// - send name, team and value when there is a winner - else empty string
// and TEAM_FREE = 0 (client structure is only used for awards!)
// - connectedClients have a team but keep the check for TEAM_FREE
// ... we'll never know for sure, connectedClients are determined in CalculateRanks
void G_BuildEndgameStats(void)
{
	char      buffer[1024];
	int       i, j;
	gclient_t *best = NULL;
	int       bestClientNum;

	G_CalcClientAccuracies();

	for (i = 0; i < level.numConnectedClients; i++)
	{
		level.clients[i].hasaward = qfalse;
	}

	*buffer = '\0';

	// highest fragger - check kills, then damage given
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->sess.kills <= 0)
		{
			continue;
		}
		if (!best || cl->sess.kills > best->sess.kills)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.kills == best->sess.kills && cl->sess.damage_given > best->sess.damage_given)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.kills, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// highest experience points - check XP
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->ps.persistant[PERS_SCORE] <= 0)
		{
			continue;
		}
		if (!best || cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->ps.persistant[PERS_SCORE], best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// highest ranking officer - check rank, then medals and XP
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (!best || cl->sess.rank > best->sess.rank)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.rank == best->sess.rank && cl->medals > best->medals)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.rank == best->sess.rank && cl->medals == best->medals && cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i 0 %i ", bestClientNum, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// most highly decorated - check medals then XP
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->medals <= 0)
		{
			continue;
		}
		if (!best || cl->medals > best->medals)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->medals == best->medals && cl->ps.persistant[PERS_SCORE] > best->ps.persistant[PERS_SCORE])
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->medals, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	// highest skills - check skills points (this map only, min lvl 1)
	for (i = 0; i < SK_NUM_SKILLS; i++)
	{
		best = NULL;

		for (j = 0; j < level.numConnectedClients; j++)
		{
			gclient_t *cl = &level.clients[level.sortedClients[j]];
			if (cl->sess.sessionTeam == TEAM_FREE)
			{
				continue;
			}
			if ((cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) <= 0)
			{
				continue;
			}
			if (cl->sess.skill[i] < 1)
			{
				continue;
			}
			if (!best || (cl->sess.skillpoints[i] - cl->sess.startskillpoints[i]) > (best->sess.skillpoints[i] - best->sess.startskillpoints[i]))
			{
				best          = cl;
				bestClientNum = level.sortedClients[j];
			}
		}
		if (best)
		{
			best->hasaward = qtrue;
			Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, (int)(best->sess.skillpoints[i] - best->sess.startskillpoints[i]), best->sess.sessionTeam));
		}
		else
		{
			Q_strcat(buffer, 1024, "-1 0 0 ");
		}
	}

	best = NULL;

	// highest accuracy
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->acc <= 0)
		{
			continue;
		}
		if (!best || cl->acc > best->acc)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %.1f %i ", bestClientNum, best->acc < 100.f ? best->acc : 100.f, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// highest HS percentage
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->hspct <= 0)
		{
			continue;
		}
		if (!best || cl->hspct > best->hspct)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %.1f %i ", bestClientNum, best->hspct < 100.f ? best->hspct : 100.f, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// best survivor - check time played percentage (min 50% of map duration)
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if ((level.time - cl->pers.enterTime) / (float)(level.time - level.intermissiontime) < 0.5f)
		{
			continue;
		}
		if (!best || (cl->sess.time_played / (float)(level.time - cl->pers.enterTime)) > (best->sess.time_played / (float)(level.time - best->pers.enterTime)))
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %f %i ", bestClientNum, 100 * best->sess.time_played / (float)(level.time - best->pers.enterTime), best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// most gibs - check gibs, then damage given
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->sess.gibs <= 0)
		{
			continue;
		}
		if (!best || cl->sess.gibs > best->sess.gibs)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.gibs == best->sess.gibs && cl->sess.damage_given > best->sess.damage_given)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.gibs, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// most selfkill - check selfkills, then deaths
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->sess.self_kills <= 0)
		{
			continue;
		}
		if (!best || cl->sess.self_kills > best->sess.self_kills)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.self_kills == best->sess.self_kills && cl->sess.deaths > best->sess.deaths)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.self_kills, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// most deaths - check deaths, then damage received
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (cl->sess.deaths <= 0)
		{
			continue;
		}
		if (!best || cl->sess.deaths > best->sess.deaths)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.deaths == best->sess.deaths && cl->sess.damage_received > best->sess.damage_received)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
		Q_strcat(buffer, 1024, va("%i %i %i ", bestClientNum, best->sess.deaths, best->sess.sessionTeam));
	}
	else
	{
		Q_strcat(buffer, 1024, "-1 0 0 ");
	}

	best = NULL;

	// I ain't got no friends award - check team kills, then team damage given (min 5 tks)
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (!best || cl->sess.team_kills > best->sess.team_kills)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
		else if (cl->sess.team_kills == best->sess.team_kills && cl->sess.team_damage_given > best->sess.team_damage_given)
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		best->hasaward = qtrue;
	}
	Q_strcat(buffer, 1024, va("%i %i %i ", best && best->sess.team_kills >= 5 ? bestClientNum : -1, best ? best->sess.team_kills : 0, best && best->sess.team_kills >= 5 ? best->sess.sessionTeam : TEAM_FREE));

	best = NULL;

	// welcome newbie! award - dont get this if any other award given or > 100 xp (this map)
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gclient_t *cl = &level.clients[level.sortedClients[i]];
		if (cl->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}
		if (!best || (cl->ps.persistant[PERS_SCORE] / (float)(level.time - cl->pers.enterTime)) > (best->ps.persistant[PERS_SCORE] / (float)(level.time - best->pers.enterTime)))
		{
			best          = cl;
			bestClientNum = level.sortedClients[i];
		}
	}
	if (best)
	{
		if ((best->sess.startxptotal - best->ps.persistant[PERS_SCORE]) >= 100 || best->medals || best->hasaward)
		{
			best = NULL;
		}
	}
	Q_strcat(buffer, 1024, va("%i %i %i ", best ? bestClientNum : -1, best ? best->ps.persistant[PERS_SCORE] : 0, best ? best->sess.sessionTeam : TEAM_FREE));

	trap_SetConfigstring(CS_ENDGAME_STATS, buffer);
}
