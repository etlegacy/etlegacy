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
 * @file g_team.c
 */

#include <limits.h>

#include "g_local.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

/**
 * @brief OtherTeam
 * @param[in] team
 * @return
 *
 * @note Unused
 */
int OtherTeam(int team)
{
	if (team == TEAM_AXIS)
	{
		return TEAM_ALLIES;
	}
	else if (team == TEAM_ALLIES)
	{
		return TEAM_AXIS;
	}
	return team;
}

/**
 * @brief TeamName
 * @param[in] team
 * @return
 */
const char *TeamName(int team)
{
	if (team == TEAM_AXIS)
	{
		return "RED";
	}
	else if (team == TEAM_ALLIES)
	{
		return "BLUE";
	}
	else if (team == TEAM_SPECTATOR)
	{
		return "SPECTATOR";
	}
	return "FREE";
}

/**
 * @brief TeamColorString
 * @param[in] team
 * @return
 *
 * @note Unused
 */
const char *TeamColorString(int team)
{
	if (team == TEAM_AXIS)
	{
		return S_COLOR_RED;
	}
	else if (team == TEAM_ALLIES)
	{
		return S_COLOR_BLUE;
	}
	else if (team == TEAM_SPECTATOR)
	{
		return S_COLOR_YELLOW;
	}
	return S_COLOR_WHITE;
}

/**
 * @brief PrintMsg
 * @param ent
 * @param fmt
 *
 * @note NULL for everyone
 */
void QDECL PrintMsg(gentity_t *ent, const char *fmt, ...)
{
	char    msg[1024];
	va_list argptr;
	char    *p;

	// NOTE: if buffer overflow, it's more likely to corrupt stack and crash than do a proper G_Error?
	va_start(argptr, fmt);
	if (Q_vsnprintf(msg, sizeof(msg), fmt, argptr) > sizeof(msg))
	{
		G_Error("PrintMsg overrun\n");
	}
	va_end(argptr);

	// double quotes are bad
	while ((p = strchr(msg, '"')) != NULL)
	{
		*p = '\'';
	}

	trap_SendServerCommand(((ent == NULL) ? -1 : ent - g_entities), va("print \"%s\"", msg));
}

/**
 * @brief OnSameTeam
 * @param[in] ent1
 * @param[in] ent2
 * @return
 */
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2)
{
	if (!ent1 || !ent1->client || !ent2 || !ent2->client)
	{
		return qfalse;
	}

	if (ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam)
	{
		return qtrue;
	}

	return qfalse;
}

#define WCP_ANIM_NOFLAG             0
#define WCP_ANIM_RAISE_AXIS         1
#define WCP_ANIM_RAISE_AMERICAN     2
#define WCP_ANIM_AXIS_RAISED        3
#define WCP_ANIM_AMERICAN_RAISED    4
#define WCP_ANIM_AXIS_TO_AMERICAN   5
#define WCP_ANIM_AMERICAN_TO_AXIS   6
#define WCP_ANIM_AXIS_FALLING       7
#define WCP_ANIM_AMERICAN_FALLING   8

/**
 * @brief Team_ResetFlag
 * @param[in] ent
 */
void Team_ResetFlag(gentity_t *ent)
{
	if (!ent)
	{
		G_Printf("Warning: NULL passed to Team_ResetFlag\n");
		return;
	}

	if (ent->flags & FL_DROPPED_ITEM)
	{
		Team_ResetFlag(&g_entities[ent->s.otherEntityNum]);
		G_FreeEntity(ent);
	}
	else
	{
		ent->s.density++;

		// do we need to respawn?
		if (ent->s.density == 1)
		{
			RespawnItem(ent);
		}

#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Flag returned %s!", _GetEntityName(ent)), "returned");
#endif

		// unset objective indicator
		switch (ent->item->giPowerUp == PW_REDFLAG ? TEAM_AXIS : TEAM_ALLIES)
		{
		case TEAM_AXIS:
			if (!level.redFlagCounter)
			{
				level.flagIndicator &= ~(1 << PW_REDFLAG);
			}
			break;
		case TEAM_ALLIES:
			if (!level.blueFlagCounter)
			{
				level.flagIndicator &= ~(1 << PW_BLUEFLAG);
			}
			break;
		default:
			break;
		}
		G_globalFlagIndicator();
	}
}

/**
 * @brief Team_ReturnFlagSound
 * @param[in] ent
 * @param[in] team
 */
void Team_ReturnFlagSound(gentity_t *ent, int team)
{
	// play powerup spawn sound to all clients
	gentity_t *pm;

	if (ent == NULL)
	{
		G_Printf("Warning: NULL passed to Team_ReturnFlagSound\n");
		return;
	}

	pm                = G_PopupMessage(PM_OBJECTIVE);
	pm->s.effect3Time = G_StringIndex(ent->message);
	pm->s.effect2Time = team;
	pm->s.density     = 1; // 1 = returned
}

/**
 * @brief Team_ReturnFlag
 * @param[in] ent
 */
void Team_ReturnFlag(gentity_t *ent)
{
	int team = ent->item->giPowerUp == PW_REDFLAG ? TEAM_AXIS : TEAM_ALLIES;

	Team_ReturnFlagSound(ent, team);
	Team_ResetFlag(ent);
	PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team)); // FIXME: returns RED/BLUE flag ... change to Axis/Allies?
}

/**
 * @brief Automatically set in Launch_Item if the item is one of the flags
 *
 * @details Flags are unique in that if they are dropped, the base flag must be respawned when they time out
 *
 * @param[in] ent
 */
void Team_DroppedFlagThink(gentity_t *ent)
{
	if (ent->item->giPowerUp == PW_REDFLAG)
	{
		G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");

		Team_ReturnFlagSound(ent, TEAM_AXIS);
		Team_ResetFlag(ent);

		if (level.gameManager)
		{
			G_Script_ScriptEvent(level.gameManager, "trigger", "axis_object_returned");
		}
	}
	else if (ent->item->giPowerUp == PW_BLUEFLAG)
	{
		G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");

		Team_ReturnFlagSound(ent, TEAM_ALLIES);
		Team_ResetFlag(ent);

		if (level.gameManager)
		{
			G_Script_ScriptEvent(level.gameManager, "trigger", "allied_object_returned");
		}
	}
	// Reset Flag will delete this entity
}

/**
 * @brief Team_TouchOurFlag
 * @param[in] ent
 * @param[in] other
 * @param[in] team
 * @return
 */
int Team_TouchOurFlag(gentity_t *ent, gentity_t *other, int team)
{
	gclient_t *cl = other->client;

	if (ent->flags & FL_DROPPED_ITEM)
	{
		// hey, its not home.  return it by teleporting it back
		if (cl->sess.sessionTeam == TEAM_AXIS)
		{
			if (level.gameManager)
			{
				G_Script_ScriptEvent(level.gameManager, "trigger", "axis_object_returned");
			}
			G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");
#ifdef FEATURE_OMNIBOT
			{
				const char *pName = ent->message ? ent->message : _GetEntityName(ent);
				Bot_Util_SendTrigger(ent, NULL, va("Axis have returned %s!", pName ? pName : ""), "returned");
			}
#endif
			// unset objective indicator
			if (!level.redFlagCounter)
			{
				level.flagIndicator &= ~(1 << PW_REDFLAG);
			}
			G_globalFlagIndicator();
		}
		else
		{
			if (level.gameManager)
			{
				G_Script_ScriptEvent(level.gameManager, "trigger", "allied_object_returned");
			}
			G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");
#ifdef FEATURE_OMNIBOT
			{
				const char *pName = ent->message ? ent->message : _GetEntityName(ent);
				Bot_Util_SendTrigger(ent, NULL, va("Allies have returned %s!", pName ? pName : ""), "returned");
			}
#endif
			// unset objective indicator
			if (!level.blueFlagCounter)
			{
				level.flagIndicator &= ~(1 << PW_BLUEFLAG);
			}
			G_globalFlagIndicator();
		}

		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound(ent, team);
		Team_ResetFlag(ent);
		return 0;
	}

	// GT_WOLF doesn't support capturing the flag
	return 0;
}

/**
 * @brief Team_TouchEnemyFlag
 * @param[in,out] ent
 * @param[out] other
 * @param[in] team
 * @return
 */
int Team_TouchEnemyFlag(gentity_t *ent, gentity_t *other, int team)
{
	gclient_t *cl = other->client;
	gentity_t *tmp;

	ent->s.density--;

	// hey, its not our flag, pick it up
	tmp         = ent->parent;
	ent->parent = other;

	if (cl->sess.sessionTeam == TEAM_AXIS)
	{
		gentity_t *pm = G_PopupMessage(PM_OBJECTIVE);

		pm->s.effect3Time = G_StringIndex(ent->message);
		pm->s.effect2Time = TEAM_AXIS;
		pm->s.density     = 0; // 0 = stolen

		if (level.gameManager)
		{
			G_Script_ScriptEvent(level.gameManager, "trigger", "allied_object_stolen");
		}
		G_Script_ScriptEvent(ent, "trigger", "stolen");
#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Axis have stolen %s!", ent->message), "stolen");
#endif
	}
	else
	{
		gentity_t *pm = G_PopupMessage(PM_OBJECTIVE);

		pm->s.effect3Time = G_StringIndex(ent->message);
		pm->s.effect2Time = TEAM_ALLIES;
		pm->s.density     = 0; // 0 = stolen

		if (level.gameManager)
		{
			G_Script_ScriptEvent(level.gameManager, "trigger", "axis_object_stolen");
		}
		G_Script_ScriptEvent(ent, "trigger", "stolen");
#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Allies have stolen %s!", ent->message), "stolen");
#endif
	}

	ent->parent = tmp;

	// reset player disguise on stealing docs
	other->client->ps.powerups[PW_OPS_DISGUISED] = 0;
	other->client->disguiseClientNum             = -1;

	if (team == TEAM_AXIS)
	{
		cl->ps.powerups[PW_REDFLAG] = INT_MAX;
		// update objective indicator
		level.flagIndicator  |= (1 << PW_REDFLAG);
		level.redFlagCounter += 1;
	}
	else
	{
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX;
		// update objective indicator
		level.flagIndicator   |= (1 << PW_BLUEFLAG);
		level.blueFlagCounter += 1;
	} // flags never expire

	// set objective indicator
	G_globalFlagIndicator();

	// store the entitynum of our original flag spawner
	if (ent->flags & FL_DROPPED_ITEM)
	{
		cl->flagParent = ent->s.otherEntityNum;
	}
	else
	{
		cl->flagParent = ent->s.number;
	}

	other->client->speedScale = ent->splashDamage; // Alter player speed

	if (ent->s.density > 0)
	{
		return 1; // We have more flags to give out, spawn back quickly
	}
	else
	{
		return -1; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
	}
}

/**
 * @brief Pickup_Team
 * @param[in] ent
 * @param[in,out] other
 * @return
 */
int Pickup_Team(gentity_t *ent, gentity_t *other)
{
	int       team;
	gclient_t *cl = other->client;

	// figure out what team this flag is
	if (strcmp(ent->classname, "team_CTF_redflag") == 0)
	{
		team = TEAM_AXIS;
	}
	else if (strcmp(ent->classname, "team_CTF_blueflag") == 0)
	{
		team = TEAM_ALLIES;
	}
	else
	{
		PrintMsg(other, "Don't know what team the flag is on.\n");
		return 0;
	}

	// ensure we don't pick a dropped obj up right away
	if (level.time - cl->dropObjectiveTime < 2000)
	{
		return 0;
	}

	trap_SendServerCommand(other - g_entities, "cp \"You picked up the objective!\"");

	// set timer
	cl->pickObjectiveTime = level.time;

	// set flag model in carrying entity if multiplayer and flagmodel is set
	other->message           = ent->message;
	other->s.otherEntityNum2 = ent->s.modelindex2;

	return ((team == cl->sess.sessionTeam) ? Team_TouchOurFlag : Team_TouchEnemyFlag)(ent, other, team);
}

/**
 * @brief G_globalFlagIndicator
 * @param[in] flagIndicator
 */
void G_globalFlagIndicator()
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_FLAG_INDICATOR);

	te->s.eventParm       = level.flagIndicator;
	te->s.otherEntityNum  = level.redFlagCounter;
	te->s.otherEntityNum2 = level.blueFlagCounter;
	te->r.svFlags        |= SVF_BROADCAST;
}

/**
 * @brief G_clientFlagIndicator
 * @param[in] flagIndicator
 */
void G_clientFlagIndicator(gentity_t *ent)
{
	gentity_t *te;

	te = G_TempEntityNotLinked(EV_FLAG_INDICATOR);

	te->s.eventParm       = level.flagIndicator;
	te->s.otherEntityNum  = level.redFlagCounter;
	te->s.otherEntityNum2 = level.blueFlagCounter;
	te->r.singleClient    = ent->s.number;
	te->r.svFlags        |= SVF_SINGLECLIENT;
}

/*---------------------------------------------------------------------------*/

#define MAX_TEAM_SPAWN_POINTS   256

/**
 * @brief Go to a random point that doesn't telefrag
 * @param teamstate - unused
 * @param[in] team
 * @param[in] spawnObjective
 * @return
 */
gentity_t *SelectRandomTeamSpawnPoint(int teamstate, team_t team, int spawnObjective)
{
	gentity_t *spot;
	gentity_t *spots[MAX_TEAM_SPAWN_POINTS];
	int       count, closest;
	int       i = 0;
	char      *classname;
	float     shortest, tmp;
	vec3_t    target;
	vec3_t    farthest;

	if (team == TEAM_AXIS)
	{
		classname = "team_CTF_redspawn";
	}
	else if (team == TEAM_ALLIES)
	{
		classname = "team_CTF_bluespawn";
	}
	else
	{
		return NULL;
	}

	count = 0;

	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), classname)) != NULL)
	{
		if (SpotWouldTelefrag(spot))
		{
			continue;
		}

		// modified to allow initial spawnpoints to be disabled at gamestart
		if (!(spot->spawnflags & 2))
		{
			continue;
		}

		// invisible entities can't be used for spawning
		if (spot->entstate == STATE_INVISIBLE || spot->entstate == STATE_UNDERCONSTRUCTION)
		{
			continue;
		}

		spots[count] = spot;
		if (++count == MAX_TEAM_SPAWN_POINTS)
		{
			break;
		}
	}

	if (!count)     // no spots that won't telefrag
	{
		spot = NULL;
		while ((spot = G_Find(spot, FOFS(classname), classname)) != NULL)
		{
			// modified to allow initial spawnpoints to be disabled at gamestart
			if (!(spot->spawnflags & 2))
			{
				continue;
			}

			// invisible entities can't be used for spawning
			if (spot->entstate == STATE_INVISIBLE || spot->entstate == STATE_UNDERCONSTRUCTION)
			{
				continue;
			}

			return spot;
		}

		return G_Find(NULL, FOFS(classname), classname);
	}

	if ((!level.numSpawnPoints))
	{
		G_Error("No spawnpoints found\n");
		return NULL;
	}
	else
	{
		// adding ability to set autospawn
		if (!spawnObjective)
		{
			switch (team)
			{
			case TEAM_AXIS:
				spawnObjective = level.axisAutoSpawn + 1;
				break;
			case TEAM_ALLIES:
				spawnObjective = level.alliesAutoSpawn + 1;
				break;
			default:
				break;
			}
		}

		i = spawnObjective - 1;

		VectorCopy(level.spawnPointStates[i].origin, farthest);

		// now that we've got farthest vector, figure closest spawnpoint to it
		VectorSubtract(farthest, spots[0]->s.origin, target);
		shortest = VectorLength(target);
		closest  = 0;
		for (i = 0; i < count; i++)
		{
			VectorSubtract(farthest, spots[i]->s.origin, target);
			tmp = VectorLength(target);

			if (tmp < shortest)
			{
				shortest = tmp;
				closest  = i;
			}
		}
		return spots[closest];
	}
}

/**
 * @brief SelectCTFSpawnPoint
 * @param[in] team
 * @param[in] teamstate
 * @param[in,out] origin
 * @param[in,out] angles
 * @param[in] spawnObjective
 * @return
 */
gentity_t *SelectCTFSpawnPoint(team_t team, int teamstate, vec3_t origin, vec3_t angles, int spawnObjective)
{
	gentity_t *spot;

	spot = SelectRandomTeamSpawnPoint(teamstate, team, spawnObjective);

	if (!spot)
	{
		return SelectSpawnPoint(vec3_origin, origin, angles);
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/

/**
 * @brief TeamplayInfoMessage
 * @details Format: clientNum location health armor weapon powerups
 * @param[in] team
 */
void TeamplayInfoMessage(team_t team)
{
	char      entry[1024];
	char      string[1024];
	size_t    stringlength = 0;
	int       i;
	size_t    j;
	gentity_t *player;
	int       cnt;
	int       h;
	char      *bufferedData;
	char      *tinfo; // currently 32 players in team create about max 750 chars of tinfo
	                  // note: trap_SendServerCommand won't send tinfo > 1022 - also see string[1024]

	// send the latest information on all clients
	string[0] = 0;

	for (i = 0, cnt = 0; i < level.numConnectedClients; i++)
	{
		player = g_entities + level.sortedClients[i];
		if (player->inuse && player->client->sess.sessionTeam == team)
		{
			// If in LIMBO, don't show followee's health
			if (player->client->ps.pm_flags & PMF_LIMBO)
			{
				h = -1;
			}
			else
			{
				h = player->client->ps.stats[STAT_HEALTH];
				if (h < 0)
				{
					h = 0;
				}
			}

			Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i %i", level.sortedClients[i], player->client->pers.teamState.location[0], player->client->pers.teamState.location[1], player->client->pers.teamState.location[2], h, player->s.powerups);

			j = strlen(entry);
			if (stringlength + j > sizeof(string) - 10) // reserve some chars for tinfo prefix
			{
				G_Printf("Warning: tinfo exceeds limit");
				break;
			}
			strcpy(string + stringlength, entry);
			stringlength += j;
			cnt++;
		}
	}

	bufferedData = team == TEAM_AXIS ? level.tinfoAxis : level.tinfoAllies;

	tinfo = va("tinfo %i%s", cnt, string);
	if (!Q_stricmp(bufferedData, tinfo))       // no change so just return
	{
		return;
	}

	Q_strncpyz(bufferedData, tinfo, 1024);

	for (i = 0; i < level.numConnectedClients; i++)
	{
		player = g_entities + level.sortedClients[i];

		if (player->inuse && (player->client->sess.sessionTeam == team || player->client->sess.shoutcaster) && !(player->r.svFlags & SVF_BOT) && player->client->pers.connected == CON_CONNECTED)
		{
			trap_SendServerCommand(player - g_entities, tinfo);
		}
	}
}

/**
 * @brief CheckTeamStatus
 */
void CheckTeamStatus(void)
{
	if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME)
	{
		int       i;
		gentity_t *ent;

		level.lastTeamLocationTime = level.time;
		for (i = 0; i < level.numConnectedClients; i++)
		{
			ent = g_entities + level.sortedClients[i];
			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES))
			{
				ent->client->pers.teamState.location[0] = (int)ent->r.currentOrigin[0];
				ent->client->pers.teamState.location[1] = (int)ent->r.currentOrigin[1];
				ent->client->pers.teamState.location[2] = (int)ent->r.currentOrigin[2];
			}
		}

		TeamplayInfoMessage(TEAM_AXIS);
		TeamplayInfoMessage(TEAM_ALLIES);
	}
}

/*-----------------------------------------------------------------*/

/**
 * @brief Use_Team_Spawnpoint
 * @param[in,out] ent
 * @param other - unused
 * @param activator - unused
 */
void Use_Team_Spawnpoint(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->spawnflags & 2)
	{
		ent->spawnflags &= ~2;

		G_DPrintf("setting %s %s inactive\n", ent->classname, ent->targetname);
	}
	else
	{
		ent->spawnflags |= 2;

		G_DPrintf("setting %s %s active\n", ent->classname, ent->targetname);
	}
}

void DropToFloor(gentity_t *ent);

/**
 * @brief SP_team_CTF_redspawn
 * @details QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32) ? INVULNERABLE STARTACTIVE
 * potential spawning position for axis team in wolfdm games.
 *
 * TODO: SelectRandomTeamSpawnPoint() will choose team_CTF_redspawn point that:
 *
 * 1) has been activated (FL_SPAWNPOINT_ACTIVE)
 * 2) isn't occupied and
 * 3) is closest to team_WOLF_objective
 *
 * This allows spawnpoints to advance across the battlefield as new ones are
 * placed and/or activated.
 *
 * If target is set, point spawnpoint toward target activation
 *
 * @param[in,out] ent
 *
 * @note edited quaked def
 */
void SP_team_CTF_redspawn(gentity_t *ent)
{
	ent->enemy = G_PickTarget(ent->target);
	if (ent->enemy)
	{
		vec3_t dir;

		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}

	ent->use = Use_Team_Spawnpoint;

	VectorSet(ent->r.mins, -16, -16, -24);
	VectorSet(ent->r.maxs, 16, 16, 32);

	ent->think = DropToFloor;
}

/**
 * @brief SP_team_CTF_bluespawn
 *
 * @details QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32) ? INVULNERABLE STARTACTIVE
 * potential spawning position for allied team in wolfdm games.
 *
 * TODO: SelectRandomTeamSpawnPoint() will choose team_CTF_bluespawn point that:
 *
 * 1) has been activated (active)
 * 2) isn't occupied and
 * 3) is closest to selected team_WOLF_objective
 *
 * This allows spawnpoints to advance across the battlefield as new ones are
 * placed and/or activated.
 *
 * If target is set, point spawnpoint toward target activation
 *
 * @param[in,out] ent
 *
 * @note edited quaked def
 */
void SP_team_CTF_bluespawn(gentity_t *ent)
{
	ent->enemy = G_PickTarget(ent->target);
	if (ent->enemy)
	{
		vec3_t dir;

		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}

	ent->use = Use_Team_Spawnpoint;

	VectorSet(ent->r.mins, -16, -16, -24);
	VectorSet(ent->r.maxs, 16, 16, 32);

	ent->think = DropToFloor;
}

/**
 * @brief Swaps the team
 *
 * @details QUAKED team_WOLF_objective (1 1 0.3) (-16 -16 -24) (16 16 32) DEFAULT_AXIS DEFAULT_ALLIES
 * marker for objective
 *
 * This marker will be used for computing effective radius for
 * dynamite damage, as well as generating a list of objectives
 * that players can elect to spawn near to in the limbo spawn
 * screen.
 *
 *     "description"   short text key for objective name that will appear in objective selection in limbo UI.
 *
 * DEFAULT_AXIS - This spawn region belongs to the Axis at the start of the map
 * DEFAULT_ALLIES - This spawn region belongs to the Alles at the start of the map
 *
 * @param[in,out] self
 *
 * @param other - unused
 * @param activator - unused
 */
void team_wolf_objective_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	// 256 is a disabled flag
	if ((self->count2 & ~256) == TEAM_AXIS)
	{
		self->count2 = (self->count2 & 256) + TEAM_ALLIES;
	}
	else if ((self->count2 & ~256) == TEAM_ALLIES)
	{
		self->count2 = (self->count2 & 256) + TEAM_AXIS;
	}

	G_UpdateSpawnPointState(self);
}

/**
 * @brief objective_Register
 * @param[in,out] self
 */
void objective_Register(gentity_t *self)
{
	static char cs[MAX_STRING_CHARS];
	char        numspawntargets[128];
	int         cs_obj = CS_MULTI_SPAWNTARGETS;

	if (level.numSpawnPoints == MAX_MULTI_SPAWNTARGETS)
	{
		G_Error("SP_team_WOLF_objective: exceeded MAX_MULTI_SPAWNTARGETS (%d)\n", MAX_MULTI_SPAWNTARGETS);
	}
	else     // Set config strings
	{
		cs_obj     += level.numSpawnPoints;
		self->use   = team_wolf_objective_use;
		self->count = cs_obj;
		G_UpdateSpawnPointState(self);
	}

	level.numSpawnPoints++;

	// set current # spawntargets
	trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof(cs));
	Com_sprintf(numspawntargets, 128, "%d", level.numSpawnPoints);
	Info_SetValueForKey(cs, "s", numspawntargets); // numspawntargets
	trap_SetConfigstring(CS_MULTI_INFO, cs);
}

/**
 * @brief SP_team_WOLF_objective
 * @param[in,out] ent
 */
void SP_team_WOLF_objective(gentity_t *ent)
{
	char *desc;

	G_SpawnString("description", "WARNING: No objective description set", &desc);

	// FIXME: wtf is this g_alloced? just use a static buffer fgs...
	ent->message = G_Alloc(strlen(desc) + 1);
	Q_strncpyz(ent->message, desc, strlen(desc) + 1);

	ent->nextthink = level.time + FRAMETIME;
	ent->think     = objective_Register;
	ent->s.eType   = ET_WOLF_OBJECTIVE;

	if (ent->spawnflags & 1)
	{
		ent->count2 = TEAM_AXIS;
	}
	else if (ent->spawnflags & 2)
	{
		ent->count2 = TEAM_ALLIES;
	}
}

// Capture and Hold Checkpoint flag
#define SPAWNPOINT  1
#define CP_HOLD     2
#define AXIS_ONLY   4
#define ALLIED_ONLY 8

void checkpoint_touch(gentity_t *self, gentity_t *other, trace_t *trace);

/**
 * @brief checkpoint_use_think
 * @param[in,out] self
 */
void checkpoint_use_think(gentity_t *self)
{
	self->count2 = -1;

	if (self->count == TEAM_AXIS)
	{
		self->health = 0;
	}
	else
	{
		self->health = 10;
	}
}

/**
 * @brief checkpoint_use
 * @param[in,out] ent
 * @param[out] other
 * @param[in] activator
 */
void checkpoint_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	int holderteam;
	int time;

	if (!activator->client)
	{
		return;
	}

	if (ent->count < 0)
	{
		checkpoint_touch(ent, activator, NULL);
	}

	holderteam = activator->client->sess.sessionTeam;

	if (ent->count == holderteam)
	{
		return;
	}

	if (ent->count2 == level.time)
	{
		if (holderteam == TEAM_AXIS)
		{
			time = ent->health / 2;
			time++;
			trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\"", time));
		}
		else
		{
			time = (10 - ent->health) / 2;
			time++;
			trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\"", time));
		}
		return;
	}

	if (holderteam == TEAM_AXIS)
	{
		ent->health--;
		if (ent->health < 0)
		{
			checkpoint_touch(ent, activator, NULL);
			return;
		}

		time = ent->health / 2;
		time++;
		trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\"", time));
	}
	else
	{
		ent->health++;
		if (ent->health > 10)
		{
			checkpoint_touch(ent, activator, NULL);
			return;
		}

		time = (10 - ent->health) / 2;
		time++;
		trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\"", time));
	}

	ent->count2    = level.time;
	ent->think     = checkpoint_use_think;
	ent->nextthink = level.time + 2000;

	// reset player disguise on touching flag
	other->client->ps.powerups[PW_OPS_DISGUISED] = 0;
	other->client->disguiseClientNum             = -1;
}

void checkpoint_spawntouch(gentity_t *self, gentity_t *other, trace_t *trace);

/**
 * @brief checkpoint_hold_think
 * @param[out] self
 *
 * @note Unused
 */
void checkpoint_hold_think(gentity_t *self)
{
	self->nextthink = level.time + 5000;
}

/**
 * @brief checkpoint_think
 * @param[in,out] self
 */
void checkpoint_think(gentity_t *self)
{
	switch (self->s.frame)
	{
	case WCP_ANIM_NOFLAG:
		break;
	case WCP_ANIM_RAISE_AXIS:
		self->s.frame = WCP_ANIM_AXIS_RAISED;
		break;
	case WCP_ANIM_RAISE_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AXIS_RAISED:
		break;
	case WCP_ANIM_AMERICAN_RAISED:
		break;
	case WCP_ANIM_AXIS_TO_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AMERICAN_TO_AXIS:
		self->s.frame = WCP_ANIM_AXIS_RAISED;
		break;
	case WCP_ANIM_AXIS_FALLING:
		self->s.frame = WCP_ANIM_NOFLAG;
		break;
	case WCP_ANIM_AMERICAN_FALLING:
		self->s.frame = WCP_ANIM_NOFLAG;
		break;
	default:
		break;
	}

	if (self->spawnflags & SPAWNPOINT)
	{
		self->touch = checkpoint_spawntouch;
	}
	else if (!(self->spawnflags & CP_HOLD))
	{
		self->touch = checkpoint_touch;
	}
	self->nextthink = 0;
}

/**
 * @brief checkpoint_touch
 * @param[in,out] self
 * @param[in,out] other
 * @param trace - unused
 */
void checkpoint_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (self->count == other->client->sess.sessionTeam)
	{
		return;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if (self->count == TEAM_AXIS)
	{
		if (self->s.frame == WCP_ANIM_NOFLAG)
		{
			self->s.frame = WCP_ANIM_RAISE_AXIS;
		}
		else if (self->s.frame == WCP_ANIM_AMERICAN_RAISED)
		{
			self->s.frame = WCP_ANIM_AMERICAN_TO_AXIS;
		}
		else
		{
			self->s.frame = WCP_ANIM_AXIS_RAISED;
		}
	}
	else
	{
		if (self->s.frame == WCP_ANIM_NOFLAG)
		{
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		}
		else if (self->s.frame == WCP_ANIM_AXIS_RAISED)
		{
			self->s.frame = WCP_ANIM_AXIS_TO_AMERICAN;
		}
		else
		{
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	self->parent = other;

	// reset player disguise on touching flag
	other->client->ps.powerups[PW_OPS_DISGUISED] = 0;
	other->client->disguiseClientNum             = -1;

	// Run script trigger
	if (self->count == TEAM_AXIS)
	{
		self->health = 0;
		G_Script_ScriptEvent(self, "trigger", "axis_capture");
	}
	else
	{
		self->health = 10;
		G_Script_ScriptEvent(self, "trigger", "allied_capture");
	}

	// Play a sound
	G_AddEvent(self, EV_GENERAL_SOUND, self->soundPos1);

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think     = checkpoint_think;
	self->nextthink = level.time + 1000;
}

/**
 * @brief If spawn flag is set, use this touch fn instead to turn on/off targeted spawnpoints
 * @param[in,out] self
 * @param[in,out] other
 * @param trace - unused
 */
void checkpoint_spawntouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t *ent      = NULL;
	qboolean  playsound = qtrue;
	qboolean  firsttime = qfalse;
#ifdef FEATURE_OMNIBOT
	char *flagAction = "touch";
#endif

	// dead guys don't capture spawns
	if (other->client->ps.eFlags & EF_DEAD)
	{
		return;
	}

	if (self->count == other->client->sess.sessionTeam)
	{
		return;
	}

	if (self->count < 0)
	{
		firsttime = qtrue;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if (self->count == TEAM_AXIS)
	{
		if (self->s.frame == WCP_ANIM_NOFLAG && !(self->spawnflags & ALLIED_ONLY))
		{
			self->s.frame = WCP_ANIM_RAISE_AXIS;
#ifdef FEATURE_OMNIBOT
			flagAction = "capture";
#endif
		}
		else if (self->s.frame == WCP_ANIM_NOFLAG)
		{
			self->s.frame = WCP_ANIM_NOFLAG;
			playsound     = qfalse;
		}
		else if (self->s.frame == WCP_ANIM_AMERICAN_RAISED && !(self->spawnflags & ALLIED_ONLY))
		{
			self->s.frame = WCP_ANIM_AMERICAN_TO_AXIS;
#ifdef FEATURE_OMNIBOT
			flagAction = "reclaims";
#endif
		}
		else if (self->s.frame == WCP_ANIM_AMERICAN_RAISED)
		{
			self->s.frame = WCP_ANIM_AMERICAN_FALLING;
#ifdef FEATURE_OMNIBOT
			flagAction = "neutralized";
#endif
		}
		else
		{
			self->s.frame = WCP_ANIM_AXIS_RAISED;
		}
	}
	else
	{
		if (self->s.frame == WCP_ANIM_NOFLAG && !(self->spawnflags & AXIS_ONLY))
		{
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
#ifdef FEATURE_OMNIBOT
			flagAction = "capture";
#endif
		}
		else if (self->s.frame == WCP_ANIM_NOFLAG)
		{
			self->s.frame = WCP_ANIM_NOFLAG;
			playsound     = qfalse;
		}
		else if (self->s.frame == WCP_ANIM_AXIS_RAISED && !(self->spawnflags & AXIS_ONLY))
		{
			self->s.frame = WCP_ANIM_AXIS_TO_AMERICAN;
#ifdef FEATURE_OMNIBOT
			flagAction = "reclaims";
#endif
		}
		else if (self->s.frame == WCP_ANIM_AXIS_RAISED)
		{
			self->s.frame = WCP_ANIM_AXIS_FALLING;
#ifdef FEATURE_OMNIBOT
			flagAction = "neutralized";
#endif
		}
		else
		{
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	// If this is the first time it's being touched, and it was the opposing team
	// touching a single-team reinforcement flag... don't do anything.
	if (firsttime && !playsound)
	{
		return;
	}

	// Play a sound
	if (playsound)
	{
		G_AddEvent(self, EV_GENERAL_SOUND, self->soundPos1);
	}

	self->parent = other;

	// reset player disguise on touching flag
	other->client->ps.powerups[PW_OPS_DISGUISED] = 0;
	other->client->disguiseClientNum             = -1;

	// Run script trigger
	if (self->count == TEAM_AXIS)
	{
		G_Script_ScriptEvent(self, "trigger", "axis_capture");
#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(self, NULL, va("axis_%s_%s", flagAction, _GetEntityName(self)), flagAction);
#endif
	}
	else
	{
		G_Script_ScriptEvent(self, "trigger", "allied_capture");
#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(self, NULL, va("allies_%s_%s", flagAction, _GetEntityName(self)), flagAction);
#endif
	}

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think     = checkpoint_think;
	self->nextthink = level.time + 1000;

	// activate all targets
	// updated this to allow toggling of initial spawnpoints as well, plus now it only
	// toggles spawnflags 2 for spawnpoint entities
	if (self->target)
	{
		int hash;

		hash = BG_StringHashValue(self->target);

		while (1)
		{
			ent = G_FindByTargetnameFast(ent, self->target, hash);

			if (!ent)
			{
				break;
			}
			if (other->client->sess.sessionTeam == TEAM_AXIS)
			{
				if (!strcmp(ent->classname, "team_CTF_redspawn"))
				{
					ent->spawnflags |= 2;
				}
				else if (!strcmp(ent->classname, "team_CTF_bluespawn"))
				{
					ent->spawnflags &= ~2;
				}
			}
			else
			{
				if (!strcmp(ent->classname, "team_CTF_bluespawn"))
				{
					ent->spawnflags |= 2;
				}
				else if (!strcmp(ent->classname, "team_CTF_redspawn"))
				{
					ent->spawnflags &= ~2;
				}
			}
		}
	}
}

/**
 * @brief SP_team_WOLF_checkpoint
 * @details QUAKED team_WOLF_checkpoint (.9 .3 .9) (-16 -16 0) (16 16 128) SPAWNPOINT CP_HOLD AXIS_ONLY ALLIED_ONLY
 * This is the flagpole players touch in Capture and Hold game scenarios.
 *
 * It will call specific trigger funtions in the map script for this object.
 * When allies capture, it will call "allied_capture".
 * When axis capture, it will call "axis_capture".
 *
 * if spawnpoint flag is set, think will turn on spawnpoints (specified as targets)
 * for capture team and turn *off* targeted spawnpoints for opposing team
 *
 * @param[in,out] ent
 */
void SP_team_WOLF_checkpoint(gentity_t *ent)
{
	char *capture_sound;

	if (!ent->scriptName)
	{
		G_Error("team_WOLF_checkpoint must have a \"scriptname\"\n");
	}

	// Make sure the ET_TRAP entity type stays valid
	ent->s.eType = ET_TRAP;

	// Model is user assignable, but it will always try and use the animations for flagpole.md3
	if (ent->model)
	{
		ent->s.modelindex = G_ModelIndex(ent->model);
	}
	else
	{
		ent->s.modelindex = G_ModelIndex("models/multiplayer/flagpole/flagpole.md3");
	}

	G_SpawnString("noise", "sound/movers/doors/door6_open.wav", &capture_sound);
	ent->soundPos1 = G_SoundIndex(capture_sound);

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;

	VectorSet(ent->r.mins, -8, -8, 0);
	VectorSet(ent->r.maxs, 8, 8, 128);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	// s.frame is the animation number
	ent->s.frame = WCP_ANIM_NOFLAG;

	// s.teamNum is which set of animations to use ( only 1 right now )
	ent->s.teamNum = 1;

	// Used later to set animations (and delay between captures)
	ent->nextthink = 0;

	// Used to time how long it must be "held" to switch
	ent->health = -1;
	ent->count2 = -1;

	// 'count' signifies which team holds the checkpoint
	ent->count = -1;

	if (ent->spawnflags & SPAWNPOINT)
	{
		ent->touch = checkpoint_spawntouch;
	}
	else
	{
		if (ent->spawnflags & CP_HOLD)
		{
			ent->use = checkpoint_use;
		}
		else
		{
			ent->touch = checkpoint_touch;
		}
	}

	trap_LinkEntity(ent);
}

/*
 * @brief Team_ClassForString
 * @param[in] string
 * @return
 *
 * @note Unused
int Team_ClassForString(const char *string)
{
    if (!Q_stricmp(string, "soldier"))
    {
        return PC_SOLDIER;
    }
    else if (!Q_stricmp(string, "medic"))
    {
        return PC_MEDIC;
    }
    else if (!Q_stricmp(string, "engineer"))
    {
        return PC_ENGINEER;
    }
    else if (!Q_stricmp(string, "fieldops"))
    {
        return PC_FIELDOPS;
    }
    else if (!Q_stricmp(string, "covertops"))
    {
        return PC_COVERTOPS;
    }
    return -1;
}
*/

const char *aTeams[TEAM_NUM_TEAMS] = { "FFA", "^1Axis^7", "^$Allies^7", "^2Spectators^7" };
team_info  teamInfo[TEAM_NUM_TEAMS];

/**
 * @brief Resets a team's settings
 * @param[in] team_num
 * @param[in] fClearSpecLock
 */
void G_teamReset(int team_num, qboolean fClearSpecLock)
{
	teamInfo[team_num].team_lock    = (match_latejoin.integer == 0 && g_gamestate.integer == GS_PLAYING);
	teamInfo[team_num].team_name[0] = 0;
	teamInfo[team_num].team_score   = 0;
	teamInfo[team_num].timeouts     = match_timeoutcount.integer;

	if (fClearSpecLock)
	{
		teamInfo[team_num].spec_lock = qfalse;
	}
}

/**
 * @brief Swaps active players on teams
 */
void G_swapTeams(void)
{
	int       i;
	gclient_t *cl;

	for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
	{
		G_teamReset(i, qtrue);
	}

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		if (cl->sess.sessionTeam == TEAM_AXIS)
		{
			cl->sess.sessionTeam = TEAM_ALLIES;
		}
		else if (cl->sess.sessionTeam == TEAM_ALLIES)
		{
			cl->sess.sessionTeam = TEAM_AXIS;
		}
		else
		{
			continue;
		}

		// swap primary weapon
		if (GetWeaponTableData(cl->sess.playerWeapon)->weapEquiv)
		{
			cl->sess.playerWeapon = cl->sess.latchPlayerWeapon = GetWeaponTableData(cl->sess.playerWeapon)->weapEquiv;
		}

		// swap secondary weapon
		if (GetWeaponTableData(cl->sess.playerWeapon2)->weapEquiv)
		{
			cl->sess.playerWeapon2 = cl->sess.latchPlayerWeapon2 = GetWeaponTableData(cl->sess.playerWeapon2)->weapEquiv;
		}

		G_UpdateCharacter(cl);
		ClientUserinfoChanged(level.sortedClients[i]);
		ClientBegin(level.sortedClients[i]);
	}

	AP("cp \"^1Teams have been swapped!\n\"");
}

/**
 * @brief G_SortPlayersByXP
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL G_SortPlayersByXP(const void *a, const void *b)
{
	gclient_t *cla = &level.clients[*((const int *)a)];
	gclient_t *clb = &level.clients[*((const int *)b)];

	if (cla->ps.stats[STAT_XP] > clb->ps.stats[STAT_XP])
	{
		return -1;
	}
	if (clb->ps.stats[STAT_XP] > cla->ps.stats[STAT_XP])
	{
		return 1;
	}

	return 0;
}

#ifdef FEATURE_RATING
/**
 * @brief G_SortPlayersBySR
 * @param[in] a
 * @param[in] b
 * @return
 */
int QDECL G_SortPlayersBySR(const void *a, const void *b)
{
	gclient_t *cla = &level.clients[*((const int *)a)];
	gclient_t *clb = &level.clients[*((const int *)b)];

	if ((cla->sess.mu - 3 * cla->sess.sigma)  > (clb->sess.mu - 3 * clb->sess.sigma))
	{
		return -1;
	}
	if ((clb->sess.mu - 3 * clb->sess.sigma)  > (cla->sess.mu - 3 * cla->sess.sigma))
	{
		return 1;
	}

	return 0;
}
#endif

/**
 * @brief Shuffle active players onto teams
 */
void G_shuffleTeamsXP(void)
{
	int       i;
	team_t    cTeam; //, cMedian = level.numNonSpectatorClients / 2;
	int       cnt = 0;
	int       sortClients[MAX_CLIENTS];
	gclient_t *cl;

	G_teamReset(TEAM_AXIS, qtrue);
	G_teamReset(TEAM_ALLIES, qtrue);

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		if (cl->sess.sessionTeam != TEAM_AXIS && cl->sess.sessionTeam != TEAM_ALLIES)
		{
			continue;
		}

		sortClients[cnt++] = level.sortedClients[i];
	}

	qsort(sortClients, cnt, sizeof(int), G_SortPlayersByXP);

	for (i = 0; i < cnt; i++)
	{
		cl = level.clients + sortClients[i];

		//	cTeam = (i % 2) + TEAM_AXIS;  // 0101...
		cTeam = (((i + 1) % 4) - ((i + 1) % 2)) / 2 + TEAM_AXIS; // 0110... fairer shuffle

		if (cl->sess.sessionTeam != cTeam)
		{
			G_LeaveTank(g_entities + sortClients[i], qfalse);
			G_RemoveClientFromFireteams(sortClients[i], qtrue, qfalse);
			if (g_landminetimeout.integer)
			{
				G_ExplodeMines(g_entities + sortClients[i]);
			}
			G_FadeItems(g_entities + sortClients[i], MOD_SATCHEL);

			// swap primary weapon
			if (GetWeaponTableData(cl->sess.playerWeapon)->weapEquiv)
			{
				cl->sess.playerWeapon = cl->sess.latchPlayerWeapon = GetWeaponTableData(cl->sess.playerWeapon)->weapEquiv;
			}

			// swap secondary weapon
			if (GetWeaponTableData(cl->sess.playerWeapon2)->weapEquiv)
			{
				cl->sess.playerWeapon2 = cl->sess.latchPlayerWeapon2 = GetWeaponTableData(cl->sess.playerWeapon2)->weapEquiv;
			}
		}

		cl->sess.sessionTeam = cTeam;

		G_UpdateCharacter(cl);
		ClientUserinfoChanged(sortClients[i]);
		ClientBegin(sortClients[i]);
	}

	AP("cp \"^1Teams have been shuffled by XP!\n\"");
}


#ifdef FEATURE_RATING
/**
 * @brief Shuffle active players onto teams by skill rating
 */
void G_shuffleTeamsSR(void)
{
	int       i;
	team_t    cTeam; //, cMedian = level.numNonSpectatorClients / 2;
	int       cnt = 0;
	int       sortClients[MAX_CLIENTS];
	int       mapBias = 0;
	gclient_t *cl;

	G_teamReset(TEAM_AXIS, qtrue);
	G_teamReset(TEAM_ALLIES, qtrue);

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];

		if (cl->sess.sessionTeam != TEAM_AXIS && cl->sess.sessionTeam != TEAM_ALLIES)
		{
			continue;
		}

		sortClients[cnt++] = level.sortedClients[i];
	}

	qsort(sortClients, cnt, sizeof(int), G_SortPlayersBySR);

	// map bias check (1 = axis advantage)
	if (g_skillRating.integer > 1)
	{
		mapBias = level.mapProb > 0.5f ? 1 : 0;
	}

	for (i = 0; i < cnt; i++)
	{
		cl = level.clients + sortClients[i];

		// put best rated player on weakest side
		if (g_skillRating.integer > 1 && mapBias)
		{
			cTeam = 3 - ((((i + 1) % 4) - ((i + 1) % 2)) / 2 + TEAM_AXIS);
		}
		else
		{
			cTeam = (((i + 1) % 4) - ((i + 1) % 2)) / 2 + TEAM_AXIS;
		}

		if (cl->sess.sessionTeam != cTeam)
		{
			G_LeaveTank(g_entities + sortClients[i], qfalse);
			G_RemoveClientFromFireteams(sortClients[i], qtrue, qfalse);
			if (g_landminetimeout.integer)
			{
				G_ExplodeMines(g_entities + sortClients[i]);
			}
			G_FadeItems(g_entities + sortClients[i], MOD_SATCHEL);

			// swap primary weapon
			if (GetWeaponTableData(cl->sess.playerWeapon)->weapEquiv)
			{
				cl->sess.playerWeapon = cl->sess.latchPlayerWeapon = GetWeaponTableData(cl->sess.playerWeapon)->weapEquiv;
			}

			// swap secondary weapon
			if (GetWeaponTableData(cl->sess.playerWeapon2)->weapEquiv)
			{
				cl->sess.playerWeapon2 = cl->sess.latchPlayerWeapon2 = GetWeaponTableData(cl->sess.playerWeapon2)->weapEquiv;
			}
		}

		cl->sess.sessionTeam = cTeam;

		G_UpdateCharacter(cl);
		ClientUserinfoChanged(sortClients[i]);
		ClientBegin(sortClients[i]);
	}

	AP("cp \"^1Teams have been shuffled by Skill Rating!\n\"");
}
#endif

/**
 * @brief Determine if the "ready" player threshold has been reached.
 * @return
 */
qboolean G_checkReady(void)
{
	int       ready = 0, notReady = match_minplayers.integer;
	gclient_t *cl;

	if (0 == g_doWarmup.integer)
	{
		return qtrue;
	}

	// Ensure we have enough real players
	if (level.numNonSpectatorClients >= match_minplayers.integer && level.voteInfo.numVotingClients > 0)
	{
		int i;

		// Step through all active clients
		notReady = 0;
		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = level.clients + level.sortedClients[i];

			if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR)
			{
				continue;
			}
			else if (cl->pers.ready || (g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT))
			{
				ready++;
			}
			else
			{
				notReady++;
			}
		}
	}

	notReady = (notReady > 0 || ready > 0) ? notReady : match_minplayers.integer;
	if (g_minGameClients.integer != notReady)
	{
		trap_Cvar_Set("g_minGameClients", va("%d", notReady));
	}

	// Do we have enough "ready" players?
	return(level.ref_allready || ((ready + notReady > 0) && 100 * ready / (ready + notReady) >= match_readypercent.integer));
}

/**
 * @brief Checks ready states to start/stop the sequence to get the match rolling.
 * @return
 */
qboolean G_readyMatchState(void)
{
	if ((g_doWarmup.integer ||
	     (g_gametype.integer == GT_WOLF_LMS && g_lms_lockTeams.integer) ||
	     level.warmupTime > (level.time + 10 * 1000)) &&
	    g_gamestate.integer == GS_WARMUP && G_checkReady())
	{
		level.ref_allready = qfalse;
		if (g_doWarmup.integer > 0 || (g_gametype.integer == GT_WOLF_LMS && g_lms_lockTeams.integer))
		{
			teamInfo[TEAM_AXIS].team_lock   = qtrue;
			teamInfo[TEAM_ALLIES].team_lock = qtrue;
		}

		return qtrue;

	}
	else if (!G_checkReady())
	{
		if (g_gamestate.integer == GS_WARMUP_COUNTDOWN)
		{
			AP("cp \"^1COUNTDOWN STOPPED!^7  Back to warmup...\n\"");
		}
		level.lastRestartTime = level.time;
		trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", GS_WARMUP));
	}

	return qfalse;
}

/**
 * @brief Check if we need to reset the game state due to an empty team
 * @param[in] nTeam
 */
void G_verifyMatchState(team_t nTeam)
{
	if ((level.lastRestartTime + 1000) < level.time && (nTeam == TEAM_ALLIES || nTeam == TEAM_AXIS) &&
	    (g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_WARMUP_COUNTDOWN || g_gamestate.integer == GS_INTERMISSION))
	{
		if (TeamCount(-1, nTeam) == 0)
		{
			if (g_doWarmup.integer > 0)
			{
				level.lastRestartTime = level.time;
				if (g_gametype.integer == GT_WOLF_STOPWATCH)
				{
					trap_Cvar_Set("g_currentRound", "0");
					trap_Cvar_Set("g_nextTimeLimit", "0");
				}

				trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", GS_WARMUP));

			}
			else
			{
				teamInfo[nTeam].team_lock = qfalse;
			}

			G_teamReset(nTeam, qtrue);
		}
	}

	// Cleanup of ready count
	G_checkReady();
}

/**
 * @brief Checks to see if a specified team is allowing players to join.
 * @param[in] nTeam
 * @param[in] ent
 * @return
 */
qboolean G_teamJoinCheck(team_t nTeam, gentity_t *ent)
{
	int cnt = TeamCount(-1, nTeam);

	// Sanity check
	if (cnt == 0)
	{
		G_teamReset(nTeam, qtrue);
		teamInfo[nTeam].team_lock = qfalse;
	}

	// Check for locked teams
	if ((nTeam == TEAM_AXIS || nTeam == TEAM_ALLIES))
	{
		if (ent->client->sess.sessionTeam == nTeam)
		{
			return qtrue;
		}

		// shoutcasters aren't allowed to join
		if (ent->client->sess.shoutcaster)
		{
			return qfalse;
		}

		if (g_gametype.integer != GT_WOLF_LMS)
		{
			// Check for full teams
			if (team_maxplayers.integer > 0 && team_maxplayers.integer <= cnt)
			{
				G_printFull(va("The %s team is full!", aTeams[nTeam]), ent);
				return qfalse;

				// Check for locked teams
			}
			else if (teamInfo[nTeam].team_lock && (!(ent->client->pers.invite & nTeam)))
			{
				G_printFull(va("The %s team is LOCKED!", aTeams[nTeam]), ent);
				return qfalse;
			}
		}
		else
		{
			if (team_maxplayers.integer > 0 && team_maxplayers.integer <= cnt)
			{
				G_printFull(va("The %s team is full!", aTeams[nTeam]), ent);
				return qfalse;
			}
			else if (g_gamestate.integer == GS_PLAYING && g_lms_lockTeams.integer && (!(ent->client->pers.invite & nTeam)))
			{
				G_printFull(va("The %s team is LOCKED!", aTeams[nTeam]), ent);
				return qfalse;
			}
		}
	}

	return qtrue;
}

/**
 * @brief Update specs for blackout, as needed
 * @param[in] nTeam
 * @param[in] fLock
 */
void G_updateSpecLock(int nTeam, qboolean fLock)
{
	int       i;
	gentity_t *ent;

	teamInfo[nTeam].spec_lock = fLock;
	for (i = 0; i < level.numConnectedClients; i++)
	{
		ent = g_entities + level.sortedClients[i];

		if (ent->client->sess.referee)
		{
			continue;
		}

		// shoutcasters are not affected by speclock
		if (ent->client->sess.shoutcaster)
		{
			continue;
		}

		ent->client->sess.spec_invite &= ~nTeam;

		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			continue;
		}

		if (!fLock)
		{
			continue;
		}

#ifdef FEATURE_MULTIVIEW
		if (ent->client->pers.mvCount > 0)
		{
			G_smvRemoveInvalidClients(ent, nTeam);
		}
		else
#endif
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW)
		{
			StopFollowing(ent);
			ent->client->sess.spec_team &= ~nTeam;
		}

#ifdef FEATURE_MULTIVIEW
		// ClientBegin sets blackout
		if (ent->client->pers.mvCount < 1)
		{
#endif
		SetTeam(ent, "s", qtrue, WP_NONE, WP_NONE, qfalse);
#ifdef FEATURE_MULTIVIEW
	}
#endif
	}
}

/**
 * @brief Swap team speclocks
 */
void G_swapTeamLocks(void)
{
	qboolean fLock = teamInfo[TEAM_AXIS].spec_lock;
	teamInfo[TEAM_AXIS].spec_lock   = teamInfo[TEAM_ALLIES].spec_lock;
	teamInfo[TEAM_ALLIES].spec_lock = fLock;

	fLock                           = teamInfo[TEAM_AXIS].team_lock;
	teamInfo[TEAM_AXIS].team_lock   = teamInfo[TEAM_ALLIES].team_lock;
	teamInfo[TEAM_ALLIES].team_lock = fLock;
}

/**
 * @brief Removes everyone's specinvite for a particular team.
 * @param[in] team
 *
 * @note Unused
 */
void G_removeSpecInvite(int team)
{
	int i;
	gentity_t *cl;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		cl = g_entities + level.sortedClients[i];
		if (!cl->inuse || cl->client->sess.referee || cl->client->sess.shoutcaster)
		{
			continue;
		}

		cl->client->sess.spec_invite &= ~team;  // none = 0, red = 1, blue = 2
	}
}

/**
 * @brief Return blockout status for a player
 * @param[in] ent
 * @param[in] nTeam
 * @return
 */
int G_blockoutTeam(gentity_t *ent, int nTeam)
{
	return(!G_allowFollow(ent, nTeam));
}

/**
 * @brief Figure out if we are allowed/want to follow a given player
 * @param[in] ent
 * @param[in] nTeam
 * @return
 */
qboolean G_allowFollow(gentity_t *ent, int nTeam)
{
	if (g_gametype.integer == GT_WOLF_LMS && g_lms_followTeamOnly.integer)
	{
		if ((ent->client->sess.spec_invite & nTeam) == nTeam)
		{
			return qtrue;
		}
		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
		    ent->client->sess.sessionTeam != nTeam)
		{
			return qfalse;
		}
	}

	if (level.time - level.startTime > 2500)
	{
		if (TeamCount(-1, TEAM_AXIS) == 0)
		{
			teamInfo[TEAM_AXIS].spec_lock = qfalse;
		}
		if (TeamCount(-1, TEAM_ALLIES) == 0)
		{
			teamInfo[TEAM_ALLIES].spec_lock = qfalse;
		}
	}

	return((!teamInfo[nTeam].spec_lock || ent->client->sess.sessionTeam != TEAM_SPECTATOR || (ent->client->sess.spec_invite & nTeam) == nTeam));
}

/**
 * @brief Figure out if we are allowed/want to follow a given player
 * @param[in] ent
 * @param[in] nTeam
 * @return
 */
qboolean G_desiredFollow(gentity_t *ent, int nTeam)
{
	if (G_allowFollow(ent, nTeam) &&
	    (ent->client->sess.spec_team == 0 || ent->client->sess.spec_team == nTeam))
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Finds suitable spawn point index for the given team.
 *        If player selected spawn point is invalid, tries to resolve it.
 * @param[in] team
 * @param[in] targetSpawnPt Player selected spawn point.
 * @return Spawn point index or -1.
 */
static int G_ResolveSpawnPointIndex(team_t team, int targetSpawnPt)
{
	int i;
	if (targetSpawnPt >= 0 && targetSpawnPt < level.numSpawnPoints)
	{
		int closestTeamSpawnPt;
		float closestTeamSpawnPtDist;
		spawnPointState_t *tagetSpawnPointState = &level.spawnPointStates[targetSpawnPt];
		// if this spawn point is already owned by the team, no further actions necessary
		if (tagetSpawnPointState->isActive && tagetSpawnPointState->team == team)
		{
			return targetSpawnPt;
		}
		// if target spawn point is owned by the opposite team,
		// find closest team spawn point to the target.
		// this works similar to an actual algorithm used to pick spawn blob.
		// however, it might be inaccurate in some cases.
		closestTeamSpawnPt     = -1;
		closestTeamSpawnPtDist = -1;
		for (i = 0; i < level.numSpawnPoints; i++)
		{
			vec3_t diffVector;
			float distance;
			spawnPointState_t *teamSpawnPointState = &level.spawnPointStates[i];
			if (!teamSpawnPointState->isActive || teamSpawnPointState->team != team)
			{
				continue;
			}
			VectorSubtract(tagetSpawnPointState->origin, teamSpawnPointState->origin, diffVector);
			distance = VectorLength(diffVector);
			if ((closestTeamSpawnPtDist < 0) || (closestTeamSpawnPtDist > distance))
			{
				closestTeamSpawnPt     = i;
				closestTeamSpawnPtDist = distance;
			}
		}
		return closestTeamSpawnPt;
	}
	// fallback: find first team spawn point
	for (i = 0; i < level.numSpawnPoints; i++)
	{
		spawnPointState_t *spawnPointState = level.spawnPointStates + i;
		if (spawnPointState->team == team)
		{
			return i;
		}
	}
	// found nothing
	return -1;
}

/**
 * @brief Converts spawn point value into spawn point index.
 * @param[in] spawnPointValue
 * @param[in] defaultSpawnPointIndex
 * @return
 */
static int G_ConvertToSpawnPointIndex(int spawnPointValue, int defaultSpawnPointIndex)
{
	if (spawnPointValue > 0 && spawnPointValue <= level.numSpawnPoints)
	{
		return (spawnPointValue - 1);
	}
	return defaultSpawnPointIndex;
}

/**
 * @brief Udates player counts in all spawn point states.
 * @return
 */
void G_UpdateSpawnPointStatePlayerCounts()
{
	static char cs[MAX_STRING_CHARS];
	int playerCounts[MAX_MULTI_SPAWNTARGETS] = { 0 };
	gclient_t *client;
	int resolvedAutoSpawnPts[2] =
	{
		G_ResolveSpawnPointIndex(TEAM_AXIS,   level.axisAutoSpawn),
		G_ResolveSpawnPointIndex(TEAM_ALLIES, level.alliesAutoSpawn)
	};
	int i, teamAutoSpawnPt, resolvedSpawnPt;
	// check updates
	for (i = 0; i < level.numConnectedClients; i++)
	{
		client = &level.clients[level.sortedClients[i]];
		// skip non-playing clients
		if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
		{
			continue;
		}
		teamAutoSpawnPt = resolvedAutoSpawnPts[(client->sess.sessionTeam == TEAM_AXIS) ? 0 : 1];
		// no spawn points are found for the given team
		if (teamAutoSpawnPt == -1)
		{
			continue;
		}
		resolvedSpawnPt = G_ConvertToSpawnPointIndex(client->sess.userSpawnPointValue, teamAutoSpawnPt);
		// selected spawn point is owned by the opposite team, find closest team spawn point
		if (level.spawnPointStates[resolvedSpawnPt].team != client->sess.sessionTeam ||
		    level.spawnPointStates[resolvedSpawnPt].isActive != 1)
		{
			resolvedSpawnPt = G_ResolveSpawnPointIndex(client->sess.sessionTeam, resolvedSpawnPt);
			if (resolvedSpawnPt == -1)
			{
				continue;
			}
		}
		playerCounts[resolvedSpawnPt]       += 1;
		client->sess.resolvedSpawnPointIndex = resolvedSpawnPt;
	}
	// update configstring, if necessary
	for (i = 0; i < level.numSpawnPoints; i++)
	{
		spawnPointState_t *spawnPointState = &level.spawnPointStates[i];
		// no updates
		if (spawnPointState->playerCount == playerCounts[i])
		{
			continue;
		}
		spawnPointState->playerCount = playerCounts[i];
		trap_GetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs, sizeof(cs));
		Info_SetValueForKey(cs, "c", va("%i", playerCounts[i]));
		trap_SetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs);
	}
}

/**
 * @brief Udates spawn point state.
 * @param[in] ent wolf_objective entity.
 * @return
 */
void G_UpdateSpawnPointState(gentity_t *ent)
{
	static char cs[MAX_STRING_CHARS];
	if (ent == NULL)
	{
		return;
	}
	spawnPointState_t *spawnPointState = &level.spawnPointStates[ent->count - CS_MULTI_SPAWNTARGETS];
	// update state
	VectorCopy(ent->s.origin, spawnPointState->origin);
	spawnPointState->team = (team_t)(ent->count2 & 0xF);
	strncpy(spawnPointState->description, ent->message, 128);
	spawnPointState->description[127] = 0;
	spawnPointState->isActive         = (ent->entstate == STATE_DEFAULT) ? 1 : 0;
	// and update configstring
	trap_GetConfigstring(ent->count, cs, sizeof(cs));
	Info_SetValueForKey(cs, "s", ent->message); // spawn_targ
	Info_SetValueForKey(cs, "x", va("%i", (int)ent->s.origin[0]));
	Info_SetValueForKey(cs, "y", va("%i", (int)ent->s.origin[1]));
	if (level.ccLayers)
	{
		Info_SetValueForKey(cs, "z", va("%i", (int)ent->s.origin[2]));
	}
	Info_SetValueForKey(cs, "t", va("%i", ent->count2));
	trap_SetConfigstring(ent->count, cs);
	G_UpdateSpawnPointStatePlayerCounts();
}
