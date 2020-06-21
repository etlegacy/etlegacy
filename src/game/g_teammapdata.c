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
 * @file g_teammapdata.c
 */

#include "g_local.h"

/**
 * @brief G_PushMapEntityToBuffer
 * @param[out] buffer
 * @param[in] size
 * @param[in] mEnt
 */
void G_PushMapEntityToBuffer(char *buffer, size_t size, mapEntityData_t *mEnt)
{
	char buf[32];

	if (level.ccLayers)
	{
		Com_sprintf(buf, sizeof(buf), "%i %i %i", ((int)mEnt->org[0]) / 128, ((int)mEnt->org[1]) / 128, ((int)mEnt->org[2]) / 128);
	}
	else
	{
		Com_sprintf(buf, sizeof(buf), "%i %i", ((int)mEnt->org[0]) / 128, ((int)mEnt->org[1]) / 128);
	}

	switch (mEnt->type)
	{
	// These need to be send so that icons can display correct command map layer
	case ME_CONSTRUCT:
	case ME_DESTRUCT:
	case ME_DESTRUCT_2:
	case ME_TANK:
	case ME_TANK_DEAD:
	case ME_COMMANDMAP_MARKER:
		Q_strcat(buffer, size, va(" %i %s %i", mEnt->type, buf, mEnt->data));
		break;
	default:
		Q_strcat(buffer, size, va(" %i %s %i %i", mEnt->type, buf, mEnt->yaw, mEnt->data));
		break;
	}
}

/**
 * @brief G_InitMapEntityData
 * @param[out] teamList
 */
void G_InitMapEntityData(mapEntityData_Team_t *teamList)
{
	int             i;
	mapEntityData_t *trav, *lasttrav;

	Com_Memset(teamList, 0, sizeof(mapEntityData_Team_t));

	teamList->activeMapEntityData.next = &teamList->activeMapEntityData;
	teamList->activeMapEntityData.prev = &teamList->activeMapEntityData;
	teamList->freeMapEntityData        = teamList->mapEntityData_Team;

	for (i = 0, trav = teamList->mapEntityData_Team + 1, lasttrav = teamList->mapEntityData_Team ; i < MAX_GENTITIES - 1 ; i++, trav++)
	{
		lasttrav->next = trav;
		lasttrav       = trav;
	}
}

/**
 * @brief G_FreeMapEntityData
 * @param[in,out] teamList
 * @param[in,out] mEnt
 * @return Next entity in the array
 */
mapEntityData_t *G_FreeMapEntityData(mapEntityData_Team_t *teamList, mapEntityData_t *mEnt)
{
	mapEntityData_t *ret = mEnt->next;

	if (!mEnt->prev)
	{
		G_Error("G_FreeMapEntityData: not active\n");
	}

	// remove from the doubly linked active list
	mEnt->prev->next = mEnt->next;
	mEnt->next->prev = mEnt->prev;

	// the free list is only singly linked
	mEnt->next                  = teamList->freeMapEntityData;
	teamList->freeMapEntityData = mEnt;

	return(ret);
}

/**
 * @brief G_AllocMapEntityData
 * @param[in,out] teamList
 * @return
 */
mapEntityData_t *G_AllocMapEntityData(mapEntityData_Team_t *teamList)
{
	mapEntityData_t *mEnt;

	if (!teamList->freeMapEntityData)
	{
		// no free entities - bomb out
		G_Error("G_AllocMapEntityData: out of entities\n");
	}

	mEnt                        = teamList->freeMapEntityData;
	teamList->freeMapEntityData = teamList->freeMapEntityData->next;

	Com_Memset(mEnt, 0, sizeof(*mEnt));

	mEnt->singleClient = -1;

	// link into the active list
	mEnt->next                               = teamList->activeMapEntityData.next;
	mEnt->prev                               = &teamList->activeMapEntityData;
	teamList->activeMapEntityData.next->prev = mEnt;
	teamList->activeMapEntityData.next       = mEnt;
	return mEnt;
}

/**
 * @brief G_FindMapEntityData
 * @param[in] teamList
 * @param[in] entNum
 * @return
 */
mapEntityData_t *G_FindMapEntityData(mapEntityData_Team_t *teamList, int entNum)
{
	mapEntityData_t *mEnt;

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{
		if (mEnt->singleClient >= 0)
		{
			continue;
		}
		if (entNum == mEnt->entNum)
		{
			return mEnt;
		}
	}

	// not found
	return NULL;
}

/**
 * @brief G_FindMapEntityDataSingleClient
 * @param[in] teamList
 * @param[in] start
 * @param[in] entNum
 * @param[in] clientNum
 * @return
 */
mapEntityData_t *G_FindMapEntityDataSingleClient(mapEntityData_Team_t *teamList, mapEntityData_t *start, int entNum, int clientNum)
{
	mapEntityData_t *mEnt;

	if (start)
	{
		mEnt = start->next;
	}
	else
	{
		mEnt = teamList->activeMapEntityData.next;
	}

	for ( ; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{
		if (clientNum == -1)
		{
			if (mEnt->singleClient < 0)
			{
				continue;
			}
		}
		else if (mEnt->singleClient >= 0 && clientNum != mEnt->singleClient)
		{
			continue;
		}
		if (entNum == mEnt->entNum)
		{
			return(mEnt);
		}
	}

	// not found
	return NULL;
}

////////////////////////////////////////////////////////////////////

/**
 * @struct plane_s
 * @brief Some culling bits
 */
typedef struct plane_s
{
	vec3_t normal;
	float dist;
} plane_t;

static plane_t frustum[4];

/**
 * @brief G_SetupFrustum
 * @param[in] ent
 */
void G_SetupFrustum(gentity_t *ent)
{
	float  xs, xc;
	float  ang;
	vec3_t axis[3];
	vec3_t vieworg;

	ang = (float)(DEG2RAD(106.27f) * 0.5); // 106.27 = 2 * atan(0.75 * tan(90.0/2) * 16/9)
	SinCos(ang, xs, xc);

	AnglesToAxis(ent->client->ps.viewangles, axis);

	VectorScale(axis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, axis[1], frustum[0].normal);

	VectorScale(axis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, axis[1], frustum[1].normal);

	VectorScale(axis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, axis[2], frustum[2].normal);

	VectorScale(axis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, axis[2], frustum[3].normal);

	VectorCopy(ent->client->ps.origin, vieworg);
	vieworg[2] += ent->client->ps.viewheight;

	frustum[0].dist = DotProduct(vieworg, frustum[0].normal);
	frustum[1].dist = DotProduct(vieworg, frustum[1].normal);
	frustum[2].dist = DotProduct(vieworg, frustum[2].normal);
	frustum[3].dist = DotProduct(vieworg, frustum[3].normal);
}

// Give bots a larger view angle through binoculars than players get - this should help the
//		landmine detection...
#define BINOCULAR_ANGLE 13.3f     // 13.3 = 2 * atan(0.75 * tan(10.0/2) * 16/9)
#define BOT_BINOCULAR_ANGLE 60.0f

/**
 * @brief G_SetupFrustum_ForBinoculars
 * @param[in] ent
 */
void G_SetupFrustum_ForBinoculars(gentity_t *ent)
{
	float  xs, xc;
	float  ang;
	vec3_t axis[3];
	vec3_t vieworg;
	float  baseAngle = (ent->r.svFlags & SVF_BOT) ? BOT_BINOCULAR_ANGLE : BINOCULAR_ANGLE;

	ang = (float)(DEG2RAD(baseAngle) * 0.5);
	SinCos(ang, xs, xc);

	AnglesToAxis(ent->client->ps.viewangles, axis);

	VectorScale(axis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, axis[1], frustum[0].normal);

	VectorScale(axis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, axis[1], frustum[1].normal);

	VectorScale(axis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, axis[2], frustum[2].normal);

	VectorScale(axis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, axis[2], frustum[3].normal);

	VectorCopy(ent->client->ps.origin, vieworg);
	vieworg[2] += ent->client->ps.viewheight;

	frustum[0].dist = DotProduct(vieworg, frustum[0].normal);
	frustum[1].dist = DotProduct(vieworg, frustum[1].normal);
	frustum[2].dist = DotProduct(vieworg, frustum[2].normal);
	frustum[3].dist = DotProduct(vieworg, frustum[3].normal);
}

/**
 * @brief G_CullPointAndRadius
 * @param[in] pt
 * @param[in] radius
 * @return true if not culled
 */
static qboolean G_CullPointAndRadius(vec3_t pt, float radius)
{
	float dist = DotProduct(pt, frustum[0].normal) - frustum[0].dist;

	if (dist < -radius || dist <= radius)
	{
		return qfalse;
	}

	dist = DotProduct(pt, frustum[1].normal) - frustum[1].dist;
	if (dist < -radius || dist <= radius)
	{
		return qfalse;
	}

	dist = DotProduct(pt, frustum[2].normal) - frustum[2].dist;
	if (dist < -radius || dist <= radius)
	{
		return qfalse;
	}

	dist = DotProduct(pt, frustum[3].normal) - frustum[3].dist;
	if (dist < -radius || dist <= radius)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief G_VisibleFromBinoculars
 * @param[in] viewer
 * @param[in] ent
 * @param[in,out] origin
 * @return
 */
qboolean G_VisibleFromBinoculars(gentity_t *viewer, gentity_t *ent, vec3_t origin)
{
	vec3_t  vieworg;
	trace_t trace;

	VectorCopy(viewer->client->ps.origin, vieworg);
	vieworg[2] += viewer->client->ps.viewheight;

	if (!G_CullPointAndRadius(origin, 0))
	{
		return qfalse;
	}

	if (!trap_InPVS(vieworg, origin))
	{
		return qfalse;
	}

	trap_Trace(&trace, vieworg, NULL, NULL, origin, viewer->s.number, MASK_SHOT);

	if (trace.fraction != 1.f)
	{
		if (trace.entityNum != ent->s.number)
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}

	return qtrue;
}

/**
 * @brief G_ResetTeamMapData
 */
void G_ResetTeamMapData()
{
	G_InitMapEntityData(&mapEntityData[0]);
	G_InitMapEntityData(&mapEntityData[1]);
}

/**
 * @brief G_UpdateTeamMapData_Construct
 * @param[in] ent
 */
void G_UpdateTeamMapData_Construct(gentity_t *ent)
{
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	switch (ent->s.teamNum)
	{
	case TEAM_SPECTATOR: // both teams - do twice
	{
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
		mEnt->type      = ME_CONSTRUCT;
		mEnt->startTime = level.time;
		mEnt->yaw       = 0;

		teamList = &mapEntityData[1];
		break;
	}
	case TEAM_AXIS:
	{
		teamList = &mapEntityData[0];
		break;
	}
	case TEAM_ALLIES:
	{
		teamList = &mapEntityData[1];

		break;
	}
	default:
		return;
	}

	mEnt = G_FindMapEntityData(teamList, num);
	if (!mEnt)
	{
		mEnt         = G_AllocMapEntityData(teamList);
		mEnt->entNum = num;
	}
	VectorCopy(ent->s.pos.trBase, mEnt->org);
	mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
	mEnt->type      = ME_CONSTRUCT;
	mEnt->startTime = level.time;
	mEnt->yaw       = 0;
}

/**
 * @brief G_UpdateTeamMapData_Tank
 * @param[in] ent
 */
void G_UpdateTeamMapData_Tank(gentity_t *ent)
{
	int                  num       = ent - g_entities;
	mapEntityData_Team_t *teamList = &mapEntityData[0];
	mapEntityData_t      *mEnt;

	mEnt = G_FindMapEntityData(teamList, num);

	if (!mEnt)
	{
		mEnt         = G_AllocMapEntityData(teamList);
		mEnt->entNum = num;
	}
	VectorCopy(ent->s.pos.trBase, mEnt->org);
	mEnt->data      = ent->s.modelindex2;
	mEnt->startTime = level.time;
	if (ent->s.eType == ET_TANK_INDICATOR_DEAD)
	{
		mEnt->type = ME_TANK_DEAD;
	}
	else
	{
		mEnt->type = ME_TANK;
	}
	mEnt->yaw = 0;

	teamList = &mapEntityData[1];
	mEnt     = G_FindMapEntityData(teamList, num);
	if (!mEnt)
	{
		mEnt         = G_AllocMapEntityData(teamList);
		mEnt->entNum = num;
	}
	VectorCopy(ent->s.pos.trBase, mEnt->org);
	mEnt->data      = ent->s.modelindex2;
	mEnt->startTime = level.time;
	if (ent->s.eType == ET_TANK_INDICATOR_DEAD)
	{
		mEnt->type = ME_TANK_DEAD;
	}
	else
	{
		mEnt->type = ME_TANK;
	}
	mEnt->yaw = 0;
}

/**
 * @brief G_UpdateTeamMapData_Destruct
 * @param[in] ent
 */
void G_UpdateTeamMapData_Destruct(gentity_t *ent)
{
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (ent->s.teamNum == TEAM_AXIS)
	{
		teamList = &mapEntityData[1];   // inverted
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
		mEnt->startTime = level.time;
		mEnt->type      = ME_DESTRUCT;
		mEnt->yaw       = 0;
	}
	else
	{
		if (ent->parent->target_ent && (ent->parent->target_ent->s.eType == ET_CONSTRUCTIBLE))
		{
			if (ent->parent->spawnflags & ((1 << 6) | (1 << 4)))
			{
				teamList = &mapEntityData[1];   // inverted
				mEnt     = G_FindMapEntityData(teamList, num);
				if (!mEnt)
				{
					mEnt         = G_AllocMapEntityData(teamList);
					mEnt->entNum = num;
				}
				VectorCopy(ent->s.pos.trBase, mEnt->org);
				mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
				mEnt->startTime = level.time;
				mEnt->type      = ME_DESTRUCT_2;
				mEnt->yaw       = 0;
			}
		}
		else if (ent->parent->target_ent && ent->parent->target_ent->s.eType == ET_EXPLOSIVE)
		{
			// do we have any spawn vars to check?
			teamList = &mapEntityData[1];       // inverted
			mEnt     = G_FindMapEntityData(teamList, num);
			if (!mEnt)
			{
				mEnt         = G_AllocMapEntityData(teamList);
				mEnt->entNum = num;
			}
			VectorCopy(ent->s.pos.trBase, mEnt->org);
			mEnt->data      = mEnt->entNum;     //ent->s.modelindex2;
			mEnt->startTime = level.time;
			mEnt->type      = ME_DESTRUCT;     // or ME_DESTRUCT_2?
			mEnt->yaw       = 0;
		}
	}

	if (ent->s.teamNum == TEAM_ALLIES)
	{
		teamList = &mapEntityData[0];   // inverted
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
		mEnt->startTime = level.time;
		mEnt->type      = ME_DESTRUCT;
		mEnt->yaw       = 0;
	}
	else
	{
		if (ent->parent->target_ent && (ent->parent->target_ent->s.eType == ET_CONSTRUCTIBLE))
		{
			if (ent->parent->spawnflags & ((1 << 6) | (1 << 4)))
			{
				teamList = &mapEntityData[0];   // inverted
				mEnt     = G_FindMapEntityData(teamList, num);
				if (!mEnt)
				{
					mEnt         = G_AllocMapEntityData(teamList);
					mEnt->entNum = num;
				}
				VectorCopy(ent->s.pos.trBase, mEnt->org);
				mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
				mEnt->startTime = level.time;
				mEnt->type      = ME_DESTRUCT_2;
				mEnt->yaw       = 0;
			}
		}
		else if (ent->parent->target_ent && ent->parent->target_ent->s.eType == ET_EXPLOSIVE)
		{
			// do we have any spawn vars to check?
			teamList = &mapEntityData[0];   // inverted
			mEnt     = G_FindMapEntityData(teamList, num);
			if (!mEnt)
			{
				mEnt         = G_AllocMapEntityData(teamList);
				mEnt->entNum = num;
			}
			VectorCopy(ent->s.pos.trBase, mEnt->org);
			mEnt->data      = mEnt->entNum; //ent->s.modelindex2;
			mEnt->startTime = level.time;
			mEnt->type      = ME_DESTRUCT; // or ME_DESTRUCT_2?
			mEnt->yaw       = 0;
		}
	}
}

/**
 * @brief G_UpdateTeamMapData_Player
 * @param[in] ent
 * @param[in] forceAllied
 * @param[in] forceAxis
 */
void G_UpdateTeamMapData_Player(gentity_t *ent, qboolean forceAllied, qboolean forceAxis)
{
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (!ent->client)
	{
		return;
	}

	if (ent->client->ps.pm_flags & PMF_LIMBO)
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_AXIS)
	{
		forceAxis = qtrue;
	}
	else if (ent->client->sess.sessionTeam == TEAM_ALLIES)
	{
		forceAllied = qtrue;
	}

	if (forceAxis)
	{
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->client->ps.origin, mEnt->org);
		mEnt->yaw       = ent->client->ps.viewangles[YAW];
		mEnt->data      = num;
		mEnt->startTime = level.time;

		if (ent->health <= 0)
		{
			mEnt->type = ME_PLAYER_REVIVE;
		}
		else if (ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG])
		{
			mEnt->type = ME_PLAYER_OBJECTIVE;
		}
		else
		{
			mEnt->type = ME_PLAYER;
		}
	}

	if (forceAllied)
	{
		teamList = &mapEntityData[1];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}

		VectorCopy(ent->client->ps.origin, mEnt->org);
		mEnt->yaw       = ent->client->ps.viewangles[YAW];
		mEnt->data      = num;
		mEnt->startTime = level.time;
		if (ent->health <= 0)
		{
			mEnt->type = ME_PLAYER_REVIVE;
		}
		else if (ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG])
		{
			mEnt->type = ME_PLAYER_OBJECTIVE;
		}
		else
		{
			mEnt->type = ME_PLAYER;
		}
	}
}

/**
 * @brief G_UpdateTeamMapData_DisguisedPlayer
 * @param[in] spotter
 * @param[in] ent
 * @param[in] forceAllied
 * @param[in] forceAxis
 */
static void G_UpdateTeamMapData_DisguisedPlayer(gentity_t *spotter, gentity_t *ent, qboolean forceAllied, qboolean forceAxis)
{
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (!ent->client)
	{
		return;
	}

	if (ent->client->ps.pm_flags & PMF_LIMBO)
	{
		return;
	}

	switch (ent->client->sess.sessionTeam)
	{
	case TEAM_AXIS:
		forceAxis = qtrue;
		break;
	case TEAM_ALLIES:
		forceAllied = qtrue;
		break;
	default:
		break;
	}

	if (forceAxis)
	{
		teamList = &mapEntityData[0];

		mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, num, spotter->s.clientNum);
		if (!mEnt)
		{
			mEnt               = G_AllocMapEntityData(teamList);
			mEnt->entNum       = num;
			mEnt->singleClient = spotter->s.clientNum;
		}
		VectorCopy(ent->client->ps.origin, mEnt->org);
		mEnt->yaw       = ent->client->ps.viewangles[YAW];
		mEnt->data      = num;
		mEnt->startTime = level.time;
		mEnt->type      = ME_PLAYER_DISGUISED;
	}

	if (forceAllied)
	{
		teamList = &mapEntityData[1];

		mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, num, spotter->s.clientNum);
		if (!mEnt)
		{
			mEnt               = G_AllocMapEntityData(teamList);
			mEnt->entNum       = num;
			mEnt->singleClient = spotter->s.clientNum;
		}
		VectorCopy(ent->client->ps.origin, mEnt->org);
		mEnt->yaw       = ent->client->ps.viewangles[YAW];
		mEnt->data      = num;
		mEnt->startTime = level.time;
		mEnt->type      = ME_PLAYER_DISGUISED;
	}
}

/**
 * @brief G_UpdateTeamMapData_LandMine
 * @param[in] ent
 */
void G_UpdateTeamMapData_LandMine(gentity_t *ent)
{
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	// must be armed..
	if (!ent->s.effect1Time)
	{
		return;
	}

	// inversed teamlists, we want to see the enemy mines
	if (ent->s.modelindex2)     // must be spotted..
	{
		teamList = &mapEntityData[(ent->s.teamNum == TEAM_AXIS) ? 1 : 0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->r.currentOrigin, mEnt->org);
		mEnt->data      = ent->s.teamNum;
		mEnt->startTime = level.time;
		mEnt->type      = ME_LANDMINE;
	}

	// team mines..
	teamList = &mapEntityData[(ent->s.teamNum == TEAM_AXIS) ? 0 : 1];
	mEnt     = G_FindMapEntityData(teamList, num);
	if (!mEnt)
	{
		mEnt         = G_AllocMapEntityData(teamList);
		mEnt->entNum = num;
	}
	VectorCopy(ent->r.currentOrigin, mEnt->org);
	mEnt->data      = ent->s.teamNum;
	mEnt->startTime = level.time;
	mEnt->type      = ME_LANDMINE;
}

/**
 * @brief G_UpdateTeamMapData_CommandmapMarker
 * @param[in] ent
 */
void G_UpdateTeamMapData_CommandmapMarker(gentity_t *ent)
{
	if (!ent->parent)
	{
		return;
	}

	if (ent->entstate != STATE_DEFAULT)
	{
		return;
	}

	if (ent->parent->spawnflags & (ALLIED_OBJECTIVE | AXIS_OBJECTIVE))
	{
		int                  num = ent - g_entities;
		mapEntityData_Team_t *teamList;
		mapEntityData_t      *mEnt;

		// alies
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.origin, mEnt->org);
		mEnt->data      = ent->parent ? ent->parent->s.teamNum : -1;
		mEnt->startTime = level.time;
		mEnt->type      = ME_COMMANDMAP_MARKER;
		mEnt->yaw       = 0;


		// axis
		teamList = &mapEntityData[1];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt)
		{
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.origin, mEnt->org);
		mEnt->data      = ent->parent ? ent->parent->s.teamNum : -1;
		mEnt->startTime = level.time;
		mEnt->type      = ME_COMMANDMAP_MARKER;
		mEnt->yaw       = 0;
	}
}

/**
 * @brief G_SendSpectatorMapEntityInfo
 * @param[in] e
 */
void G_SendSpectatorMapEntityInfo(gentity_t *e)
{
	// special version, sends different set of ents - only the objectives, but also team info (string is split in two basically)
	mapEntityData_t      *mEnt;
	mapEntityData_Team_t *teamList = &mapEntityData[0]; // Axis data init
	char                 buffer[2048];
	int                  al_cnt = 0, ax_cnt = 0;

	buffer[0] = '\0';

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{
		if (!e->client->sess.shoutcaster)
		{
			if (mEnt->type != ME_CONSTRUCT && mEnt->type != ME_DESTRUCT && mEnt->type != ME_DESTRUCT_2 && mEnt->type != ME_TANK && mEnt->type != ME_TANK_DEAD && mEnt->type != ME_COMMANDMAP_MARKER)
			{
				continue;
			}

			if (mEnt->singleClient >= 0 && e->s.clientNum != mEnt->singleClient)
			{
				continue;
			}
		}
		else
		{
			if (level.time - mEnt->startTime > 1000)
			{
				// we can free this player from the list now
				if (mEnt->type == ME_PLAYER || mEnt->type == ME_PLAYER_REVIVE || mEnt->type == ME_PLAYER_OBJECTIVE)
				{
					mEnt = G_FreeMapEntityData(teamList, mEnt);
					continue;
				}
				else if (mEnt->type == ME_PLAYER_DISGUISED)
				{
					if (mEnt->singleClient == e->s.clientNum)
					{
						mEnt = G_FreeMapEntityData(teamList, mEnt);
						continue;
					}
				}
			}
		}

		ax_cnt++;
	}

	// Allied data init
	teamList = &mapEntityData[1];

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{
		if (!e->client->sess.shoutcaster)
		{
			if (mEnt->type != ME_CONSTRUCT && mEnt->type != ME_DESTRUCT && mEnt->type != ME_DESTRUCT_2 &&
			    mEnt->type != ME_TANK && mEnt->type != ME_TANK_DEAD && mEnt->type != ME_COMMANDMAP_MARKER)
			{
				continue;
			}

			if (mEnt->singleClient >= 0 && e->s.clientNum != mEnt->singleClient)
			{
				continue;
			}
		}
		else
		{
			if (level.time - mEnt->startTime > 1000)
			{
				// we can free this player from the list now
				if (mEnt->type == ME_PLAYER || mEnt->type == ME_PLAYER_REVIVE || mEnt->type == ME_PLAYER_OBJECTIVE)
				{
					mEnt = G_FreeMapEntityData(teamList, mEnt);
					continue;
				}
				else if (mEnt->type == ME_PLAYER_DISGUISED)
				{
					if (mEnt->singleClient == e->s.clientNum)
					{
						mEnt = G_FreeMapEntityData(teamList, mEnt);
						continue;
					}
				}
			}
		}

		al_cnt++;
	}

	// Data setup
	// FIXME: Find out why objective counts are reset to zero when a new player connects
	if (ax_cnt > 0 || al_cnt > 0)
	{
		Com_sprintf(buffer, sizeof(buffer), "entnfo %i %i", ax_cnt, al_cnt);
	}

	// Axis data
	teamList = &mapEntityData[0];

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{
		if (mEnt->type != ME_CONSTRUCT && mEnt->type != ME_DESTRUCT && mEnt->type != ME_DESTRUCT_2 &&
		    mEnt->type != ME_TANK && mEnt->type != ME_TANK_DEAD && mEnt->type != ME_COMMANDMAP_MARKER &&
		    !e->client->sess.shoutcaster)
		{
			continue;
		}

		if (mEnt->singleClient >= 0 && e->s.clientNum != mEnt->singleClient)
		{
			continue;
		}

		G_PushMapEntityToBuffer(buffer, sizeof(buffer), mEnt);
	}

	// Allied data
	teamList = &mapEntityData[1];

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{
		if (mEnt->type != ME_CONSTRUCT && mEnt->type != ME_DESTRUCT && mEnt->type != ME_DESTRUCT_2 &&
		    mEnt->type != ME_TANK && mEnt->type != ME_TANK_DEAD && mEnt->type != ME_COMMANDMAP_MARKER &&
		    !e->client->sess.shoutcaster)
		{
			continue;
		}

		if (mEnt->singleClient >= 0 && e->s.clientNum != mEnt->singleClient)
		{
			continue;
		}

		G_PushMapEntityToBuffer(buffer, sizeof(buffer), mEnt);
	}

	if (buffer[0] != '\0') // don't send an emtpy buffer
	{
		trap_SendServerCommand(e - g_entities, buffer);
	}
}

/**
 * @brief G_SendMapEntityInfo
 * @param[in] e
 */
void G_SendMapEntityInfo(gentity_t *e)
{
	mapEntityData_t      *mEnt;
	mapEntityData_Team_t *teamList;
	char                 buffer[2048];
	int                  cnt = 0;

	buffer[0] = '\0';

	if (e->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		G_SendSpectatorMapEntityInfo(e);
		return;
	}

	// something really went wrong if this evaluates to true - TEAM_FREE
	if (e->client->sess.sessionTeam != TEAM_AXIS && e->client->sess.sessionTeam != TEAM_ALLIES)
	{
		return;
	}

	teamList = e->client->sess.sessionTeam == TEAM_AXIS ? &mapEntityData[0] : &mapEntityData[1];

	mEnt = teamList->activeMapEntityData.next;
	while (mEnt && mEnt != &teamList->activeMapEntityData)
	{
		if (level.time - mEnt->startTime > 1000)
		{
			// we can free this player from the list now
			if (mEnt->type == ME_PLAYER || mEnt->type == ME_PLAYER_REVIVE || mEnt->type == ME_PLAYER_OBJECTIVE)
			{
				mEnt = G_FreeMapEntityData(teamList, mEnt);
				continue;
			}
			else if (mEnt->type == ME_PLAYER_DISGUISED)
			{
				if (mEnt->singleClient == e->s.clientNum)
				{
					mEnt = G_FreeMapEntityData(teamList, mEnt);
					continue;
				}
			}
		}
		cnt++;

		mEnt = mEnt->next;
	}

	if (e->client->sess.sessionTeam == TEAM_AXIS)
	{
		if (cnt > 0)
		{
			Com_sprintf(buffer, sizeof(buffer), "entnfo %i 0", cnt);
		}
	}
	else
	{
		if (cnt > 0)
		{
			Com_sprintf(buffer, sizeof(buffer), "entnfo 0 %i", cnt);
		}
	}

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next)
	{

		if (mEnt->singleClient >= 0 && e->s.clientNum != mEnt->singleClient)
		{
			continue;
		}

		G_PushMapEntityToBuffer(buffer, sizeof(buffer), mEnt);
	}

	if (buffer[0] != '\0') // don't send an emtpy buffer
	{
		trap_SendServerCommand(e - g_entities, buffer);
	}
}

/**
 * @brief G_PopupMessageForMines
 * @param[in] player
 */
void G_PopupMessageForMines(gentity_t *player) // int sound
{
	gentity_t *pm;

	pm = G_PopupMessage(PM_MINES);

	VectorCopy(player->client->landmineSpotted->r.currentOrigin, pm->s.origin);
	pm->s.effect2Time = (player->client->sess.sessionTeam == TEAM_AXIS) ? TEAM_ALLIES : TEAM_AXIS;
	pm->s.effect3Time = player - g_entities;
	//pm->s.loopSound   = sound;
}

/**
 * @brief G_CheckSpottedLandMines
 */
void G_CheckSpottedLandMines(void)
{
	int       i, j;
	gentity_t *ent, *ent2;

	if (level.time - level.lastMapSpottedMinesUpdate < 1000)
	{
		return;
	}
	level.lastMapSpottedMinesUpdate = level.time;

	for (i = 0; i < level.numConnectedClients; i++)
	{
		ent = &g_entities[level.sortedClients[i]];

		if (!ent->inuse || !ent->client)
		{
			continue;
		}

		if (ent->health <= 0)
		{
			continue;
		}

		// must be in a valid team
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}

		if (ent->client->ps.pm_flags & PMF_LIMBO)
		{
			continue;
		}

		if (ent->client->sess.playerType == PC_COVERTOPS && (ent->client->ps.eFlags & EF_ZOOMING))
		{
			G_SetupFrustum_ForBinoculars(ent);

			for (j = 0, ent2 = g_entities; j < level.num_entities; j++, ent2++)
			{
				if (!ent2->inuse || ent2 == ent)
				{
					continue;
				}

				if (ent2->s.eType != ET_MISSILE)
				{
					continue;
				}

				if (ent2->methodOfDeath == MOD_LANDMINE)
				{
					if (ent2->s.effect1Time == 1 && (ent2->s.teamNum != ent->client->sess.sessionTeam))
					{
						// as before, we can only detect a mine if we can see it from our binoculars
						if (G_VisibleFromBinoculars(ent, ent2, ent2->r.currentOrigin))
						{
							G_UpdateTeamMapData_LandMine(ent2);

							switch (ent2->s.teamNum)
							{
							case TEAM_AXIS:
							case TEAM_ALLIES:
								if (!ent2->s.modelindex2)
								{
									ent->client->landmineSpottedTime = level.time;
									ent->client->landmineSpotted     = ent2;
									ent2->s.density                  = ent - g_entities + 1;
									ent2->missionLevel               = level.time;

									ent->client->landmineSpotted->count2 += 50; // @sv_fps
									if (ent->client->landmineSpotted->count2 >= 250)
									{
										ent->client->landmineSpotted->count2 = 250;

										ent->client->landmineSpotted->s.modelindex2 = 1;

										ent->client->landmineSpotted->takedamage = qtrue;

										ent->client->landmineSpotted->r.snapshotCallback = qfalse;

										// for marker
										// Landmine flags shouldn't block our view
										// don't do this if the mine has been triggered.
										if (!G_LandmineTriggered(ent->client->landmineSpotted))
										{
											ent->client->landmineSpotted->s.frame    = rand() % 20;
											ent->client->landmineSpotted->r.contents = CONTENTS_TRANSLUCENT;
											trap_LinkEntity(ent->client->landmineSpotted);
										}

										G_PopupMessageForMines(ent);

										trap_SendServerCommand(ent - g_entities, "cp \"Landmine revealed\"");

										G_AddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f);
										G_DebugAddSkillPoints(ent, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f, "spotting a landmine");
									}
								}
								break;
							default:
								break;
							}
						}
						else
						{
							// if we can't see the mine from our binoculars, make sure we clear out the landmineSpotted ptr,
							// because bots looking for mines are getting confused
							ent->client->landmineSpotted = NULL;
						}
					}
				}
			}
		}
	}
}

/**
 * @brief G_UpdateTeamMapData
 */
void G_UpdateTeamMapData(void)
{
	int             i, j;
	gentity_t       *ent, *ent2;
	mapEntityData_t *mEnt;
	qboolean        f1, f2;

	G_CheckSpottedLandMines();

	if (level.time - level.lastMapEntityUpdate < 1000)
	{
		return;
	}
	level.lastMapEntityUpdate = level.time;

	// all ents - comon update
	for (i = 0; i < level.num_entities; i++)
	{
		ent = &g_entities[i];

		if (!ent->inuse)
		{
			//mapEntityData[0][i].valid = qfalse;
			//mapEntityData[1][i].valid = qfalse;
			continue;
		}

		switch (ent->s.eType)
		{
		case ET_PLAYER:
			G_UpdateTeamMapData_Player(ent, qfalse, qfalse);
			for (j = 0; j < 2; j++)
			{
				mapEntityData_Team_t *teamList = &mapEntityData[j];

				mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

				while (mEnt)
				{
					VectorCopy(ent->client->ps.origin, mEnt->org);
					mEnt->yaw = ent->client->ps.viewangles[YAW];
					mEnt      = G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);
				}
			}
			break;
		case ET_CONSTRUCTIBLE_INDICATOR:
			if (ent->parent && ent->parent->entstate == STATE_DEFAULT)
			{
				G_UpdateTeamMapData_Construct(ent);
			}
			break;
		case ET_EXPLOSIVE_INDICATOR:
			if (ent->parent && ent->parent->entstate == STATE_DEFAULT)
			{
				G_UpdateTeamMapData_Destruct(ent);
			}
			break;
		case ET_TANK_INDICATOR:
		case ET_TANK_INDICATOR_DEAD:
			G_UpdateTeamMapData_Tank(ent);
			break;
		case ET_MISSILE:
			if (ent->methodOfDeath == MOD_LANDMINE)
			{
				G_UpdateTeamMapData_LandMine(ent);
			}
			break;
		case ET_COMMANDMAP_MARKER:
			G_UpdateTeamMapData_CommandmapMarker(ent);
			break;
		default:
			break;
		}
	}

	// clients again - do special stuff for field- and covert ops
	for (i = 0; i < level.numConnectedClients; i++)
	{
		ent = &g_entities[level.sortedClients[i]];

		f1 = ent->client->sess.sessionTeam == TEAM_ALLIES ? qtrue : qfalse;
		f2 = ent->client->sess.sessionTeam == TEAM_AXIS ? qtrue : qfalse;

		if (!ent->inuse)
		{
			continue;
		}

		if (ent->health <= 0)
		{
			continue;
		}

		// must be in a valid team
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->sess.sessionTeam == TEAM_FREE)
		{
			continue;
		}

		if (ent->client->ps.pm_flags & PMF_LIMBO)
		{
			continue;
		}

		if (ent->client->sess.playerType == PC_FIELDOPS && (ent->client->ps.eFlags & EF_ZOOMING) && ent->client->sess.skill[SK_SIGNALS] >= 4)
		{
			vec3_t pos[3];

			G_SetupFrustum_ForBinoculars(ent);

			for (j = 0; j < level.numConnectedClients; j++)
			{
				ent2 = &g_entities[level.sortedClients[j]];

				if (!ent2->inuse || ent2 == ent)
				{
					continue;
				}

				// players are sometimes of type ET_GENERAL
				if (ent2->s.eType != ET_PLAYER)
				{
					continue;
				}

				if (ent2->client->sess.sessionTeam == TEAM_SPECTATOR)
				{
					continue;
				}

				if (ent2->health <= 0 ||
				    ent2->client->sess.sessionTeam == ent->client->sess.sessionTeam ||
				    !ent2->client->ps.powerups[PW_OPS_DISGUISED])
				{
					continue;
				}

				VectorCopy(ent2->client->ps.origin, pos[0]);
				pos[0][2] += ent2->client->ps.mins[2];
				VectorCopy(ent2->client->ps.origin, pos[1]);
				VectorCopy(ent2->client->ps.origin, pos[2]);
				pos[2][2] += ent2->client->ps.maxs[2];

				if (G_VisibleFromBinoculars(ent, ent2, pos[0]) ||
				    G_VisibleFromBinoculars(ent, ent2, pos[1]) ||
				    G_VisibleFromBinoculars(ent, ent2, pos[2]))
				{
					G_UpdateTeamMapData_DisguisedPlayer(ent, ent2, f1, f2);
				}
			}
		}
		else if (ent->client->sess.playerType == PC_COVERTOPS)
		{
			vec3_t pos[3];

			G_SetupFrustum(ent);

			for (j = 0; j < level.numConnectedClients; j++)
			{
				ent2 = &g_entities[level.sortedClients[j]];

				if (!ent2->inuse || ent2 == ent)
				{
					continue;
				}

				// players are sometimes of type ET_GENERAL
				if (ent2->s.eType != ET_PLAYER)
				{
					continue;
				}

				// no spectators thanks
				if (ent2->client->sess.sessionTeam == TEAM_SPECTATOR || ent2->client->sess.sessionTeam == TEAM_FREE)
				{
					continue;
				}

				// do not add 'dead' players
				if (ent2->health <= 0)
				{
					continue;
				}

				// we will only add other team members here
				if (ent2->client->sess.sessionTeam == ent->client->sess.sessionTeam)
				{
					continue;
				}

				VectorCopy(ent2->client->ps.origin, pos[0]);
				pos[0][2] += ent2->client->ps.mins[2];
				VectorCopy(ent2->client->ps.origin, pos[1]);
				VectorCopy(ent2->client->ps.origin, pos[2]);
				pos[2][2] += ent2->client->ps.maxs[2];

				if (G_VisibleFromBinoculars(ent, ent2, pos[0]) ||
				    G_VisibleFromBinoculars(ent, ent2, pos[1]) ||
				    G_VisibleFromBinoculars(ent, ent2, pos[2]))
				{
					G_UpdateTeamMapData_Player(ent2, f1, f2);
				}
			}
		}
	}
}
