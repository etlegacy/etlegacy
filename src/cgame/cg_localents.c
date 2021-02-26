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
 * @file cg_localents.c
 * @brief Every frame, generate renderer commands for locally processed
 *        entities, like smoke puffs, gibs, shells, etc.
 */

#include "cg_local.h"

// increased this
//#define   MAX_LOCAL_ENTITIES  512
#define MAX_LOCAL_ENTITIES  768     // renderer can only handle 1024 entities max, so we should avoid
// overwriting game entities

localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t cg_activeLocalEntities;       // double linked list
localEntity_t *cg_freeLocalEntities;        // single linked list

// debugging
int localEntCount = 0;

/**
 * @brief CG_GetLocation
 * @param[in] client
 * @param[in] origin
 * @return
 */
location_t *CG_GetLocation(int client, vec3_t origin)
{
	location_t *curLoc;
	location_t *bestLoc = NULL;
	float      bestdist = 200000000.f;
	float      len;
	int        i;

	if (ISVALIDCLIENTNUM(client) && cgs.clientLocation[client].lastLocation)
	{
		if ((cgs.clientLocation[client].lastX == origin[0]
		     && cgs.clientLocation[client].lastY == origin[1]
		     && cgs.clientLocation[client].lastZ == origin[2])
		    && &cgs.location[cgs.clientLocation[client].lastLocation])
		{
			return &cgs.location[cgs.clientLocation[client].lastLocation];
		}
	}

	for (i = 0; i < cgs.numLocations; ++i)
	{
		curLoc = &cgs.location[i];
		len    = VectorDistance(origin, curLoc->origin);

		if (len > bestdist || !trap_R_inPVS(origin, curLoc->origin))
		{
			//CG_Printf("^6PVS %i \n", trap_R_inPVS(origin, curLoc->origin));
			//CG_Printf("^6OR %i  %i   --  %i   %i\n", origin[0], origin[1], curLoc->origin[0], curLoc->origin[1]);
			continue;
		}

		bestdist = len;
		bestLoc  = curLoc;
	}

	// store new information
	if (ISVALIDCLIENTNUM(client) && bestLoc != NULL)
	{
		cgs.clientLocation[client].lastX        = origin[0];
		cgs.clientLocation[client].lastY        = origin[1];
		cgs.clientLocation[client].lastZ        = origin[2];
		cgs.clientLocation[client].lastLocation = bestLoc->index;
	}

	return bestLoc;
}

/**
 * @brief CG_GetLocationMsg
 * @param[in] clientNum
 * @param[in] origin
 * @return
 */
char *CG_GetLocationMsg(int clientNum, vec3_t origin)
{
	location_t *bestLoc = CG_GetLocation(clientNum, origin);

	// Fast way out, there are no locations for the map
	if (cgs.numLocations < 1)
	{
		return "Unknown";
	}

	if (bestLoc != NULL && strlen(bestLoc->message) > 1)
	{
		return va("%s", bestLoc->message);
	}

	return "Unknown";
}

/**
 * @brief CG_BuildLocationString
 * @param[in] clientNum
 * @param[in] origin
 * @param[in] flag
 * @return
 */
char *CG_BuildLocationString(int clientNum, vec3_t origin, int flag)
{
	char *locStr = NULL;

	if (cg_locations.integer & flag)
	{
		qboolean locValid = qtrue;

		if (cg_locations.integer & LOC_SHOWDISTANCE)
		{
			if (clientNum == cg.clientNum)
			{
				locStr = va("^3     ");
				//CG_Printf("same client\n");
			}
			else
			{
				//locStr = va("^3%7.2f", VectorDistance(origin, cgs.clientinfo[cg.clientNum].location) / 40.f); // meter
				locStr = va("^3%7.2f", VectorDistance(origin, cgs.clientinfo[cg.clientNum].location));        // game units
			}
		}
		else
		{
			locStr = va("^3%s", CG_GetLocationMsg(clientNum, origin));
			if (!(cg_locations.integer & LOC_KEEPUNKNOWN))
			{
				if (!Q_stricmp(locStr, "Unknown"))
				{
					locStr   = va("^3(%s)", BG_GetLocationString(origin[0], origin[1]));
					locValid = qfalse; // don't draw it twice..
				}
			}
		}

		if ((cg_locations.integer & LOC_SHOWCOORDS) && locValid)
		{
			Q_strcat(locStr, 64, va(" ^3(%s)", BG_GetLocationString(origin[0], origin[1])));   // append a location
		}
	}
	else
	{
		locStr = va("^3%s", BG_GetLocationString(origin[0], origin[1]));
	}

	return locStr;
}

#define MAX_BUFFER 32768
/**
 * @brief Report a location for the player. Uses placed nearby target_location entities
 */
void CG_LoadLocations(void)
{
	fileHandle_t f;                     // handle of file on disk
	int          fLen;                  // length of the file
	char         fBuffer[MAX_BUFFER];   // buffer to read the file into
	char         message[128] = "\0";   // location description
	char         temp[128]    = "\0";   // temporary buffer
	int          x            = 0;      // x-coord of the location
	int          y            = 0;      // y-coord of the location
	int          z            = 0;      // z-coord of the location
	int          p            = 0;      // current location in the file buffer
	int          t            = 0;      // current location in the temp buffer

	fLen = trap_FS_FOpenFile(va("maps/%s_loc_override.dat", cgs.rawmapname), &f, FS_READ);

	if (fLen <= 0)
	{
		// open the location .dat file that matches the map's name
		fLen = trap_FS_FOpenFile(va("maps/%s_loc.dat", cgs.rawmapname), &f, FS_READ);

		if (fLen <= 0)
		{
			CG_Printf(S_COLOR_BLUE "LoadLocations: ^3Warning: ^9No location data found for map ^2%s^9.\n", cgs.rawmapname);
			return;
		}
	}

	if (fLen >= MAX_BUFFER)
	{
		trap_FS_FCloseFile(f);
		CG_Error("Location file is too big, make it smaller (max = %i bytes)\n", MAX_BUFFER);
	}

	trap_FS_Read(&fBuffer, fLen, f);                    // read the file into the buffer
	fBuffer[fLen] = '\0';                               // make sure it's null-terminated
	trap_FS_FCloseFile(f);                              // close the file, we're done with it

	CG_Printf(S_COLOR_BLUE "LoadLocations: ^9location data for map ^2%s ^9loaded\n", cgs.rawmapname);

	// start parsing!
	while (p < fLen)
	{
		// check for the beginning of a comment
		if (fBuffer[p++] == '/')
		{
			//check for single line comment
			if (fBuffer[p] == '/')
			{
				while (p < fLen && (fBuffer[p] != '\n' && fBuffer[p] != '\r'))
				{
					p++;
				}
			}
			// check for multiline comment
			else if (fBuffer[p] == '*')
			{
				p++;
				while (p < fLen && (fBuffer[p] != '*' && fBuffer[p + 1] != '/'))
				{
					p++;
				}
			}
		}

		// parse the next line
		while (p < fLen && (fBuffer[p] != '\n' && fBuffer[p] != '\r'))
		{
			// grab the x-coord
			while (p < fLen && fBuffer[p] != ' ')
			{
				temp[t++] = fBuffer[p++];
			}
			temp[t] = '\0';
			x       = Q_atoi(temp);
			t       = 0;
			Com_Memset(&temp, 0, sizeof(temp));

			if (p > fLen)
			{
				break;
			}

			p++;

			// grab the y-coord
			while (p < fLen && fBuffer[p] != ' ')
			{
				temp[t++] = fBuffer[p++];
			}
			temp[t] = '\0';
			y       = Q_atoi(temp);
			t       = 0;
			Com_Memset(&temp, 0, sizeof(temp));

			if (p > fLen)
			{
				break;
			}

			p++;

			// grab the z-coord
			while (p < fLen && fBuffer[p] != ' ')
			{
				temp[t++] = fBuffer[p++];
			}
			temp[t] = '\0';
			z       = Q_atoi(temp);
			t       = 0;

			Com_Memset(&temp, 0, sizeof(temp));
			if (p > fLen)
			{
				break;
			}

			p++;

			// grab the description
			while (p < fLen && fBuffer[p] != '\n' && fBuffer[p] != '\r')
			{
				// ignore quotation marks
				if (fBuffer[p] != '\"')
				{
					temp[t++] = fBuffer[p++];
				}
				else
				{
					p++;
				}
			}
			temp[t] = '\0';
			t       = 0;

			// if @, then keep the previous location name, otherwise, update message
			if (Q_stricmp(temp, "@"))
			{
				strcpy(message, temp);
			}

			if (p > fLen)
			{
				break;
			}

			if ((x != 0 || y != 0 || z != 0) && strlen(message) > 0)
			{
				location_t *loc = &cgs.location[cgs.numLocations];

				loc->index = cgs.numLocations;
				strcpy(loc->message, message);
				loc->origin[0] = x;
				loc->origin[1] = y;
				loc->origin[2] = z;
				cgs.numLocations++;

				if (cgs.numLocations == MAX_C_LOCATIONS)
				{
					CG_Printf("^9Too many locations specifed.\n");
					break;
				}
			}
		}
	}
	// ok we are succesfull
	CG_Printf("^2%i ^9locations loaded.\n", cgs.numLocations);
	cgs.locationsLoaded = qtrue;
}

/**
 * @brief This is called at startup and for tournement restarts
 */
void CG_InitLocalEntities(void)
{
	int i;

	Com_Memset(cg_localEntities, 0, sizeof(cg_localEntities));
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities        = cg_localEntities;
	for (i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++)
	{
		cg_localEntities[i].next = &cg_localEntities[i + 1];
	}

	// debugging
	localEntCount = 0;
}

/**
 * @brief CG_FreeLocalEntity
 * @param le
 */
void CG_FreeLocalEntity(localEntity_t *le)
{
	if (!le->prev)
	{
		CG_Error("CG_FreeLocalEntity: not active\n");
	}

	// debugging
	localEntCount--;
	//trap_Print(va("FreeLocalEntity: locelEntCount = %d type = %i\n", localEntCount, le->leType));

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next             = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/**
 * @brief CG_FindLocalEntity
 * @details Will allways succeed, even if it requires freeing an old active entity
 * @param[in] index
 * @param[in] sideNum
 * @return
 */
localEntity_t *CG_FindLocalEntity(int index, int sideNum)
{
	int i;

	for (i = 0 ; i < localEntCount ; i++)
	{
		if (cg_localEntities[i].data1 == index)
		{
			if (cg_localEntities[i].data2 == sideNum)
			{
				return &cg_localEntities[i];
			}
		}
	}
	return NULL;
}

/**
 * @brief CG_AllocLocalEntity
 * @details Will allways succeed, even if it requires freeing an old active entity
 * @return
 */
localEntity_t *CG_AllocLocalEntity(void)
{
	localEntity_t *le;

	if (!cg_freeLocalEntities)
	{
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity(cg_activeLocalEntities.prev);
	}

	// debugging
	localEntCount++;
	//trap_Print(va("AllocLocalEntity: locelEntCount = %d\n", localEntCount));

	le                   = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	Com_Memset(le, 0, sizeof(*le));

	// link into the active list
	le->next                          = cg_activeLocalEntities.next;
	le->prev                          = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next       = le;
	return le;
}

/*
====================================================================================
FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.
====================================================================================
*/

// use this to change between particle and trail code
//#define BLOOD_PARTICLE_TRAIL
/**
 * @brief Leave expanding blood puffs behind gibs
 * @param le
 */
void CG_BloodTrail(localEntity_t *le)
{
	int    t;
	int    t2;
	int    step;
	vec3_t newOrigin;
	float  vl;  //bani

#ifndef BLOOD_PARTICLE_TRAIL
	static vec3_t col = { 1, 1, 1 };
#endif

	if (!cg_blood.integer)
	{
		return;
	}

	// step = 150;
#ifdef BLOOD_PARTICLE_TRAIL
	step = 10;
#else
	// time it takes to move 3 units
	vl = VectorLength(le->pos.trDelta);
	// rain - need to check FLT_EPSILON because floating-point math sucks <3
	if (vl < FLT_EPSILON)
	{
		return;
	}
	step = (int)((1000 * 3) / vl);
	// avoid another div by 0
	// check against <= 0 instead of == 0, because it can still wrap; (3000 / (FLT_EPSILON*11.7f)) < 0
	if (step <= 0)
	{
		return;
	}
#endif

	t  = step * ((cg.time - cg.frametime + step) / step);
	t2 = step * (cg.time / step);

	for ( ; t <= t2; t += step)
	{
		BG_EvaluateTrajectory(&le->pos, t, newOrigin, qfalse, -1);

#ifdef BLOOD_PARTICLE_TRAIL
		CG_Particle_Bleed(cgs.media.smokePuffShader, newOrigin, vec3_origin, 0, 500 + rand() % 200);
#else
		// blood trail using trail code (should be faster since we don't have to spawn as many)
		le->headJuncIndex = CG_AddTrailJunc(le->headJuncIndex,
		                                    le,  // rain - zinx's trail fix
		                                    cgs.media.bloodTrailShader,
		                                    t,
		                                    STYPE_STRETCH,
		                                    newOrigin,
		                                    180,
		                                    1.0,  // start alpha
		                                    0.0,  // end alpha
		                                    12.0,
		                                    12.0,
		                                    TJFL_NOCULL,
		                                    col, col,
		                                    0, 0);
#endif
	}
}

/**
 * @brief CG_FragmentBounceMark
 * @param[in,out] le
 * @param[in] trace
 */
void CG_FragmentBounceMark(localEntity_t *le, trace_t *trace)
{
	if (le->leMarkType == LEMT_BLOOD && cg_bloodTime.integer)
	{
		static int lastBloodMark;

		// don't drop too many blood marks
		if (!(lastBloodMark > cg.time || lastBloodMark > cg.time - 100))
		{
			vec4_t projection, color;
			int    radius = 16 + (rand() & 31);
			//CG_ImpactMark( cgs.media.bloodDotShaders[rand()%5], trace->endpos, trace->plane.normal, random()*360,
			//  1,1,1,1, qtrue, radius, qfalse, cg_bloodTime.integer * 1000 );
#if 0
			VectorSubtract(vec3_origin, trace->plane.normal, projection);
			projection[3] = radius * 2.0f;
			VectorMA(trace->endpos, -8.0f, projection, markOrigin);
			CG_ImpactMark(cgs.media.bloodDotShaders[rand() % 5], markOrigin, projection, radius, random() * 360.0f, 1.0f, 1.0f, 1.0f, 1.0f, cg_bloodTime.integer * 1000);
#else
			VectorSet(projection, 0, 0, -1);
			projection[3] = radius;
			Vector4Set(color, 1.0f, 1.0f, 1.0f, 1.0f);
			trap_R_ProjectDecal(cgs.media.bloodDotShaders[rand() % 5], 1, (vec3_t *) trace->endpos, projection, color,
			                    cg_bloodTime.integer * 1000, (cg_bloodTime.integer * 1000) >> 4);
#endif
			lastBloodMark = cg.time;
		}
	}

	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->leMarkType = LEMT_NONE;
}

/**
 * @brief CG_FragmentBounceSound
 * @param[in] le
 * @param[in] trace
 */
void CG_FragmentBounceSound(localEntity_t *le, trace_t *trace)
{
	switch (le->leBounceSoundType)
	{
	// adding machinegun brass bouncy sound for tk
	case LEBS_BRASS:
	{
		int rnd = rand() % 3;

		if (trace->surfaceFlags & SURF_METAL)
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_METAL][rnd][0], 64);
		}
		else if (trace->surfaceFlags & SURF_WOOD)
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_WOOD][rnd][0], 64);
		}
		else if (trace->surfaceFlags & (SURF_GRAVEL | SURF_SNOW | SURF_CARPET | SURF_GRASS))
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_SOFT][rnd][0], 64);
		}
		else
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_STONE][rnd][0], 64);
		}
		break;
	}
	case LEBS_SG_BRASS:
	{
		int rnd = rand() % 3;

		if (trace->surfaceFlags & SURF_METAL)
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_METAL][rnd][1], 96);
		}
		else if (trace->surfaceFlags & SURF_WOOD)
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_WOOD][rnd][1], 96);
		}
		else if (trace->surfaceFlags & (SURF_GRAVEL | SURF_SNOW | SURF_CARPET | SURF_GRASS))
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_SOFT][rnd][1], 96);
		}
		else
		{
			trap_S_StartSoundVControl(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_brassSound[BRASSSOUND_STONE][rnd][1], 96);
		}
	}
	break;
	case LEBS_ROCK:
		trap_S_StartSound(trace->endpos, -1, CHAN_AUTO, cgs.media.sfx_rubbleBounce[(rand() % 3)]);
		break;
	case LEBS_BONE:
		trap_S_StartSound(trace->endpos, -1, CHAN_AUTO, cgs.media.boneBounceSound);
		break;
	default:
		return;
	}

	// don't allow a fragment to make multiple bounce sounds,
	// or it gets too noisy as they settle
	le->leBounceSoundType = LEBS_NONE;
}

/**
 * @brief CG_ReflectVelocity
 * @param[in,out] le
 * @param[in] trace
 */
void CG_ReflectVelocity(localEntity_t *le, trace_t *trace)
{
	vec3_t velocity;
	float  dot;
	int    hitTime = (int)(cg.time - cg.frametime + cg.frametime * trace->fraction);

	// reflect the velocity on the trace plane

	BG_EvaluateTrajectoryDelta(&le->pos, hitTime, velocity, qfalse, -1);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, le->pos.trDelta);

	VectorScale(le->pos.trDelta, le->bounceFactor, le->pos.trDelta);

	VectorCopy(trace->endpos, le->pos.trBase);
	le->pos.trTime = cg.time;

	// check for stop, making sure that even on low FPS systems it doesn't bobble

	if (le->leMarkType == LEMT_BLOOD && trace->startsolid)
	{
		//centity_t *cent;
		//cent = &cg_entities[trace->entityNum];
		//if (cent && cent->currentState.apos.trType != TR_STATIONARY)
		//  le->pos.trType = TR_STATIONARY;
	}
	else if (trace->allsolid || (trace->plane.normal[2] > 0 && (le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2])))
	{
		// if it's a fragment and it's not resting on the world...
		//if(le->leType == LE_DEBRIS && trace->entityNum < (MAX_ENTITIES - 1))
		if (le->leType == LE_FRAGMENT && trace->entityNum < (MAX_ENTITIES - 1))
		{
			le->pos.trType = TR_GRAVITY_PAUSED;
		}
		else
		{
			le->pos.trType = TR_STATIONARY;
		}
	}
}

/**
 * @brief CG_AddEmitter
 * @param[in,out] le
 */
void CG_AddEmitter(localEntity_t *le)
{
	vec3_t dir;

	if (le->breakCount > cg.time)      // using 'breakCount' for 'wait'
	{
		return;
	}

	VectorScale(le->angles.trBase, 30, dir);
	CG_Particle_OilParticle(cgs.media.oilParticle, le->pos.trBase, dir, 15000, le->ownerNum);

	le->breakCount = cg.time + 50;
}

void CG_Explodef(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader);

/**
 * @brief CG_AddFragment
 * @param[in,out] le
 */
void CG_AddFragment(localEntity_t *le)
{
	vec3_t      newOrigin;
	trace_t     trace;
	refEntity_t *re        = &le->refEntity;
	float       flameAlpha = 0.0;
	vec3_t      flameDir;
	qboolean    hasFlame = qfalse;

	if (!re->fadeStartTime || re->fadeEndTime < le->endTime)
	{
		if (le->endTime - cg.time > 5000)
		{
			re->fadeStartTime = le->endTime - 5000;
		}
		else
		{
			re->fadeStartTime = le->endTime - 1000;
		}
		re->fadeEndTime = le->endTime;
	}

	// flaming gibs
	if (le->onFireStart && (le->onFireStart < cg.time && le->onFireEnd > cg.time))
	{
		hasFlame = qtrue;
		// calc the alpha
		flameAlpha = 1.0f - ((float)(cg.time - le->onFireStart) / (float)(le->onFireEnd - le->onFireStart));
		if (flameAlpha < 0.0f)
		{
			flameAlpha = 0.0f;
		}
		if (flameAlpha > 1.0f)
		{
			flameAlpha = 1.0f;
		}
	}

	if (le->leFlags & LEF_SMOKING)
	{
		// create a little less smoke

		//  TODO: FIXME: this is not quite right, because it'll become fps dependant - in a bad way.
		//      the slower the fps, the /more/ smoke there'll be, probably driving the fps lower.
		if (!(rand() % 5))
		{
			refEntity_t flash;
			float       alpha = 1.0f - ((float)(cg.time - le->startTime) / (float)(le->endTime - le->startTime));

			alpha *= 0.25f;
			Com_Memset(&flash, 0, sizeof(flash));
			CG_PositionEntityOnTag(&flash, &le->refEntity, "tag_flash", 0, NULL);
			CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 1000, 8, 20, 20, alpha, 8.f);
		}
	}

	if (le->pos.trType == TR_STATIONARY)
	{
		// add the flame
		if (hasFlame)
		{
			refEntity_t backupEnt = le->refEntity;

			VectorClear(flameDir);
			flameDir[2] = 1;

			le->refEntity.shaderRGBA[3] = ( unsigned char )(255.0f * flameAlpha);
			VectorCopy(flameDir, le->refEntity.fireRiseDir);
			le->refEntity.customShader = cgs.media.onFireShader;
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.customShader = cgs.media.onFireShader2;
			trap_R_AddRefEntityToScene(&le->refEntity);

			le->refEntity = backupEnt;
		}

		trap_R_AddRefEntityToScene(&le->refEntity);

		return;
	}
	else if (le->pos.trType == TR_GRAVITY_PAUSED)
	{
		// add the flame
		if (hasFlame)
		{
			refEntity_t backupEnt = le->refEntity;

			VectorClear(flameDir);
			flameDir[2] = 1;

			le->refEntity.shaderRGBA[3] = ( unsigned char )(255.0f * flameAlpha);
			VectorCopy(flameDir, le->refEntity.fireRiseDir);
			le->refEntity.customShader = cgs.media.onFireShader;
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.customShader = cgs.media.onFireShader2;
			trap_R_AddRefEntityToScene(&le->refEntity);

			le->refEntity = backupEnt;
		}

		trap_R_AddRefEntityToScene(&le->refEntity);
		// trace a line from previous position down, to see if I should start falling again

		VectorCopy(le->refEntity.origin, newOrigin);
		newOrigin[2] -= 5;
		CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_MISSILECLIP);

		if (trace.fraction == 1.0f)     // it's clear, start moving again
		{
			VectorClear(le->pos.trDelta);
			VectorClear(le->angles.trDelta);
			le->pos.trType = TR_GRAVITY;    // nothing below me, start falling again
		}
		else
		{
			return;
		}
	}

	// calculate new position
	BG_EvaluateTrajectory(&le->pos, cg.time, newOrigin, qfalse, -1);

	if (hasFlame)
	{
		// calc the flame dir
		VectorSubtract(le->refEntity.origin, newOrigin, flameDir);
		if (VectorLengthSquared(flameDir) == 0.f)
		{
			flameDir[2] = 1;
			// play a burning sound when not moving
			trap_S_AddLoopingSound(newOrigin, vec3_origin, cgs.media.flameSound, (int)(0.3f * 255.0f * flameAlpha), 0);
		}
		else
		{
			VectorNormalize(flameDir);
			// play a flame blow sound when moving
			trap_S_AddLoopingSound(newOrigin, vec3_origin, cgs.media.flameBlowSound, (int)(0.3f * 255.0f * flameAlpha), 0);
		}
	}

	// trace a line from previous position to new position
	CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID);
	if (trace.fraction == 1.0f)
	{
		int i;

		// still in free fall
		VectorCopy(newOrigin, le->refEntity.origin);

		if ((le->leFlags & LEF_TUMBLE) || le->angles.trType == TR_LINEAR)
		{
			vec3_t angles;

			BG_EvaluateTrajectory(&le->angles, cg.time, angles, qtrue, -1);
			AnglesToAxis(angles, le->refEntity.axis);
			if (le->sizeScale != 0.f && le->sizeScale != 1.0f)
			{
				for (i = 0; i < 3; i++)
				{
					VectorScale(le->refEntity.axis[i], le->sizeScale, le->refEntity.axis[i]);
				}
				le->refEntity.nonNormalizedAxes = qtrue;
			}
		}
		else
		{
			AnglesToAxis(le->angles.trBase, le->refEntity.axis);
			if (le->sizeScale != 0.f && le->sizeScale != 1.0f)
			{
				for (i = 0; i < 3; i++)
				{
					VectorScale(le->refEntity.axis[i], le->sizeScale, le->refEntity.axis[i]);
				}
				le->refEntity.nonNormalizedAxes = qtrue;
			}
		}

		// add the flame
		if (hasFlame)
		{
			refEntity_t backupEnt = le->refEntity;

			le->refEntity.shaderRGBA[3] = ( unsigned char )(255.0f * flameAlpha);
			VectorCopy(flameDir, le->refEntity.fireRiseDir);
			le->refEntity.customShader = cgs.media.onFireShader;
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.customShader = cgs.media.onFireShader2;
			trap_R_AddRefEntityToScene(&le->refEntity);

			le->refEntity = backupEnt;
		}

		trap_R_AddRefEntityToScene(&le->refEntity);

		// add a blood trail
		if (le->leBounceSoundType == LEBS_BLOOD)
		{
			CG_BloodTrail(le);
		}

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if (CG_PointContents(trace.endpos, 0) & CONTENTS_NODROP)
	{
		CG_FreeLocalEntity(le);
		return;
	}

	// do a bouncy sound
	CG_FragmentBounceSound(le, &trace);

	// reflect the velocity on the trace plane
	CG_ReflectVelocity(le, &trace);

	if (le->leFlags & LEF_TUMBLE_SLOW)
	{
		VectorScale(le->angles.trDelta, 0.8f, le->angles.trDelta);
	}

	// break on contact?
	if (le->breakCount)
	{
		if (le->leFlags & LEF_TUMBLE_SLOW)     // HACK HACK x_X
		{
			vec3_t org, dir;
			float  sizeScale = le->sizeScale * 0.8f;

			// make it smaller
			if (sizeScale < 0.7f)
			{
				sizeScale = 0.7f;
			}

			// move us a bit
			VectorNormalize2(le->pos.trDelta, dir);
			VectorMA(trace.endpos, 4.0f * sizeScale, dir, org);

			// randomize vel a bit
			VectorMA(le->pos.trDelta, VectorLength(le->pos.trDelta) * 0.3f, bytedirs[rand() % NUMVERTEXNORMALS], dir);

			CG_Explodef(org, dir, (int)(sizeScale * 50), 0, 0, qfalse, trap_R_GetShaderFromModel(le->refEntity.hModel, 0, 0));

			CG_FreeLocalEntity(le);
			return;
		}
		else
		{
			// re-added gibmodel support
			// FIXME: limit local ent usage and cap this at N gib model ents in total for client? cvar? 0 = off, N = value
			clientInfo_t   *ci;
			int            i, clientNum = le->ownerNum;
			localEntity_t  *nle;
			vec3_t         dir;
			bg_character_t *character;

			if (clientNum < 0 || clientNum >= MAX_CLIENTS)
			{
				CG_Error("Bad clientNum on player entity\n");
			}
			ci        = &cgs.clientinfo[clientNum];
			character = CG_CharacterForClientinfo(ci, NULL);

			// spawn some new fragments
			for (i = 0; i <= le->breakCount; i++)
			{
				nle = CG_AllocLocalEntity();
				Com_Memcpy(&(nle->leType), &(le->leType), sizeof(localEntity_t) - 2 * sizeof(localEntity_t *));
				if (nle->breakCount-- < 2)
				{
					nle->refEntity.hModel = character->gibModels[rand() % 2];
				}
				else
				{
					nle->refEntity.hModel = character->gibModels[rand() % 4];
				}
				// make it smaller
				nle->endTime    = cg.time + 4000 + rand() % 2000;
				nle->sizeScale *= 0.8;
				if (nle->sizeScale < 0.7f)
				{
					nle->sizeScale         = 0.7f;
					nle->leBounceSoundType = 0;
				}
				// move us a bit
				VectorNormalize2(nle->pos.trDelta, dir);
				VectorMA(trace.endpos, 4.0f * le->sizeScale * i, dir, nle->pos.trBase);
				// randomize vel a bit
				VectorMA(nle->pos.trDelta, VectorLength(nle->pos.trDelta) * 0.3f, bytedirs[rand() % NUMVERTEXNORMALS], nle->pos.trDelta);
			}
			// we're done
			CG_FreeLocalEntity(le);
			// end gib model support

			return;
		}
	}

	if (le->pos.trType == TR_STATIONARY && le->leMarkType == LEMT_BLOOD)
	{
		// leave a mark
		if (le->leMarkType)
		{
			CG_FragmentBounceMark(le, &trace);
		}
	}

	// add the flame
	if (hasFlame)
	{
		refEntity_t backupEnt = le->refEntity;

		le->refEntity.shaderRGBA[3] = ( unsigned char )(255.0f * flameAlpha);
		VectorCopy(flameDir, le->refEntity.fireRiseDir);
		le->refEntity.customShader = cgs.media.onFireShader;
		trap_R_AddRefEntityToScene(&le->refEntity);
		le->refEntity.customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene(&le->refEntity);

		le->refEntity = backupEnt;
	}

	trap_R_AddRefEntityToScene(&le->refEntity);
}

/**
 * @brief CG_AddMovingTracer
 * @param[in] le
 */
void CG_AddMovingTracer(localEntity_t *le)
{
	vec3_t start, end, dir;

	BG_EvaluateTrajectory(&le->pos, cg.time, start, qfalse, -1);
	VectorNormalize2(le->pos.trDelta, dir);
	VectorMA(start, cg_tracerLength.value, dir, end);

	CG_DrawTracer(start, end);
}

/**
 * @brief CG_AddSparkElements
 * @param[in,out] le
 */
void CG_AddSparkElements(localEntity_t *le)
{
	vec3_t  newOrigin;
	trace_t trace;
	float   lifeFrac;
	float   time = (float)(cg.time - cg.frametime);

	while (1)
	{
		// calculate new position
		BG_EvaluateTrajectory(&le->pos, cg.time, newOrigin, qfalse, -1);

		//if ((le->endTime - le->startTime) > 500) {

		// trace a line from previous position to new position
		CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, MASK_SHOT);

		// if stuck, kill it
		if (trace.startsolid)
		{
			// HACK, some walls screw up, so just pass through if starting in a solid
			VectorCopy(newOrigin, trace.endpos);
			trace.fraction = 1.0;
		}

		// moved some distance
		VectorCopy(trace.endpos, le->refEntity.origin);

		//} else
		//{   // just move it there
		// VectorCopy( newOrigin, le->refEntity.origin );
		// trace.fraction = 1.0;
		//}

		time += cg.frametime * trace.fraction;

		lifeFrac = (float)(cg.time - le->startTime) / (float)(le->endTime - le->startTime);

		// add a trail
		le->headJuncIndex = CG_AddSparkJunc(le->headJuncIndex,
		                                    le,  // rain - zinx's trail fix
		                                    le->refEntity.customShader,
		                                    le->refEntity.origin,
		                                    200,
		                                    1.0f - lifeFrac,  // start alpha
		                                    0.0,  //1.0 - lifeFrac, // end alpha
		                                    lifeFrac * 2.0f * (((le->endTime - le->startTime) > 400) + 1) * 1.5f,
		                                    lifeFrac * 2.0f * (((le->endTime - le->startTime) > 400) + 1) * 1.5f);

		// if it is in a nodrop zone, remove it
		// this keeps gibs from waiting at the bottom of pits of death
		// and floating levels

		// for some reason SFM1.BSP is one big NODROP zone
		//if ( CG_PointContents( le->refEntity.origin, 0 ) & CONTENTS_NODROP ) {
		// CG_FreeLocalEntity( le );
		//return;
		//}

		if (trace.fraction < 1.0f)
		{
			// just kill it
			CG_FreeLocalEntity(le);
			return;
			/*
			            // reflect the velocity on the trace plane
			            CG_ReflectVelocity( le, &trace );
			            // the intersection is a fraction of the frametime
			            le->pos.trTime = (int)time;
			*/
		}

		if (trace.fraction == 1.0f || time >= (float)cg.time)
		{
			return;
		}
	}
}

/**
 * @brief CG_AddFuseSparkElements
 * @param[in,out] le
 */
void CG_AddFuseSparkElements(localEntity_t *le)
{
	float         FUSE_SPARK_WIDTH = 1.0;
	int           step             = 10;
	float         lifeFrac;
	static vec3_t whiteColor = { 1, 1, 1 };
	int           time       = le->lastTrailTime;

	while (time < cg.time)
	{
		// calculate new position
		BG_EvaluateTrajectory(&le->pos, time, le->refEntity.origin, qfalse, -1);

		lifeFrac = (time - le->startTime) / (float)(le->endTime - le->startTime);

		//if (lifeFrac > 0.2) {
		// add a trail
		// rain - added le for zinx's trail fix
		le->headJuncIndex = CG_AddTrailJunc(le->headJuncIndex, le, cgs.media.sparkParticleShader, time, STYPE_STRETCH, le->refEntity.origin, (int)(lifeFrac * (float)(le->endTime - le->startTime) / 2.0f),
		                                    1.0 /*(1.0 - lifeFrac)*/, 0.0, FUSE_SPARK_WIDTH * (1.0f - lifeFrac), FUSE_SPARK_WIDTH * (1.0f - lifeFrac), TJFL_SPARKHEADFLARE, whiteColor, whiteColor, 0, 0);
		//}

		time += step;

		le->lastTrailTime = time;
	}
}

/**
 * @brief CG_AddBloodElements
 * @param[in,out] le
 */
void CG_AddBloodElements(localEntity_t *le)
{
	vec3_t  newOrigin;
	trace_t trace;
	float   lifeFrac;
	float   time = (float)(cg.time - cg.frametime);
	int     numbounces;

	for (numbounces = 0; numbounces < 5; numbounces++)
	{
		// calculate new position
		BG_EvaluateTrajectory(&le->pos, cg.time, newOrigin, qfalse, -1);

		// trace a line from previous position to new position
		CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, MASK_SHOT);

		// if stuck, kill it
		if (trace.startsolid)
		{
			// HACK, some walls screw up, so just pass through if starting in a solid
			VectorCopy(newOrigin, trace.endpos);
			trace.fraction = 1.0;
		}

		// moved some distance
		VectorCopy(trace.endpos, le->refEntity.origin);
		time += cg.frametime * trace.fraction;

		lifeFrac = (float)(cg.time - le->startTime) / (float)(le->endTime - le->startTime);

		// add a trail
		le->headJuncIndex = CG_AddSparkJunc(le->headJuncIndex,
		                                    le,  // rain - zinx's trail fix
		                                    cgs.media.bloodTrailShader,
		                                    le->refEntity.origin,
		                                    200,
		                                    1.0f - lifeFrac,  // start alpha
		                                    1.0f - lifeFrac,  // end alpha
		                                    3.0,
		                                    5.0);

		if (trace.fraction < 1.0f)
		{
			// reflect the velocity on the trace plane
			CG_ReflectVelocity(le, &trace);

			// TODO: spawn a blood decal here?

			// the intersection is a fraction of the frametime
			le->pos.trTime = (int)time;
		}

		if (trace.fraction == 1.0f || time >= (float)cg.time)
		{
			return;
		}
	}
}

/**
 * @brief CG_AddDebrisElements
 * @param[in,out] le
 */
void CG_AddDebrisElements(localEntity_t *le)
{
	vec3_t  newOrigin;
	trace_t trace;
	float   lifeFrac;
	int     t, step = 50;

	for (t = le->lastTrailTime + step; t < cg.time; t += step)
	{
		// calculate new position
		BG_EvaluateTrajectory(&le->pos, t, newOrigin, qfalse, -1);

		// trace a line from previous position to new position
		CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, MASK_SHOT);

		// if stuck, kill it
		if (trace.startsolid)
		{
			// HACK, some walls screw up, so just pass through if starting in a solid
			VectorCopy(newOrigin, trace.endpos);
			trace.fraction = 1.0f;
		}

		// moved some distance
		VectorCopy(trace.endpos, le->refEntity.origin);

		// add a trail
		lifeFrac = (float)(t - le->startTime) / (float)(le->endTime - le->startTime);

#if 0   // spark line
		if (le->effectWidth > 0)
		{
			le->headJuncIndex = CG_AddSparkJunc(le->headJuncIndex,
			                                    le,  // rain - zinx's trail fix
			                                    cgs.media.sparkParticleShader,
			                                    le->refEntity.origin,
			                                    (int)(600.0 * (0.5 + 0.5 * (0.5 - lifeFrac))),        // trail life
			                                    1.0 - lifeFrac * 2,  // alpha
			                                    0.5 * (1.0 - lifeFrac),    // end alpha
			                                    5.0 * (1.0 - lifeFrac),    // start width
			                                    5.0 * (1.0 - lifeFrac));     // end width
		}
#endif

		// smoke
		if (le->effectFlags & 1)
		{
			le->headJuncIndex2 = CG_AddSmokeJunc(le->headJuncIndex2,
			                                     le,  // rain - zinx's trail fix
			                                     cgs.media.smokeTrailShader,
			                                     le->refEntity.origin,
			                                     (int)(2000.0f * (0.5f + 0.5f * (1.0f - lifeFrac))),        // trail life
			                                     1.0f * (trace.fraction == 1.0f) * (0.5f + 0.5f * (1.0f - lifeFrac)),        // alpha
			                                     1,  // start width
			                                     (int)(60.0f * (0.5f + 0.5f * (1.0f - lifeFrac))));         // end width
		}

		// if it is in a nodrop zone, remove it
		// this keeps gibs from waiting at the bottom of pits of death
		// and floating levels
		//if ( CG_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		//CG_FreeLocalEntity( le );
		//return;
		//}

		if (trace.fraction < 1.0f)
		{
			// reflect the velocity on the trace plane
			CG_ReflectVelocity(le, &trace);
			if (VectorLengthSquared(le->pos.trDelta) < Square(1))
			{
				CG_FreeLocalEntity(le);
				return;
			}
			// the intersection is a fraction of the frametime
			le->pos.trTime = t;
		}

		le->lastTrailTime = t;
	}

	// extended debris elements
	if (!cg_visualEffects.integer)
	{
		return;
	}

	if (le->pos.trType == TR_STATIONARY)
	{
		// sink into the ground if near the removal time
		int t = le->endTime - cg.time;

		if (t < SINK_TIME)
		{
			float oldZ;
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy(le->refEntity.origin, le->refEntity.lightingOrigin);
			le->refEntity.renderfx  |= RF_LIGHTING_ORIGIN;
			oldZ                     = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * (1.0f - (float)t / SINK_TIME);
			trap_R_AddRefEntityToScene(&le->refEntity);
			le->refEntity.origin[2] = oldZ;
		}
		else
		{
			trap_R_AddRefEntityToScene(&le->refEntity);
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory(&le->pos, cg.time, newOrigin, qfalse, -1);

	// trace a line from previous position to new position
	// FIXME: don't bounce at sky?
	CG_Trace(&trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID);
	if (trace.fraction == 1.0f)
	{
		// still in free fall
		VectorCopy(newOrigin, le->refEntity.origin);

		if (le->leFlags & LEF_TUMBLE)
		{
			vec3_t angles;

			BG_EvaluateTrajectory(&le->angles, cg.time, angles, qtrue, -1);
			AnglesToAxis(angles, le->refEntity.axis);
		}

		trap_R_AddRefEntityToScene(&le->refEntity);
		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if (CG_PointContents(trace.endpos, 0) & CONTENTS_NODROP)
	{
		CG_FreeLocalEntity(le);
		return;
	}

	// do a bouncy sound
	CG_FragmentBounceSound(le, &trace);

	// reflect the velocity on the trace plane
	CG_ReflectVelocity(le, &trace);

	trap_R_AddRefEntityToScene(&le->refEntity);
}

/*
=====================================================================
TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/**
 * @brief CG_AddFadeRGB
 * @param[in] le
 */
void CG_AddFadeRGB(localEntity_t *le)
{
	refEntity_t *re = &le->refEntity;
	float       c   = (le->endTime - cg.time) * le->lifeRate;

	c *= 0xff;

	re->shaderRGBA[0] = (byte)(le->color[0] * c);
	re->shaderRGBA[1] = (byte)(le->color[1] * c);
	re->shaderRGBA[2] = (byte)(le->color[2] * c);
	re->shaderRGBA[3] = (byte)(le->color[3] * c);

	trap_R_AddRefEntityToScene(re);
}

/**
 * @brief CG_AddConstRGB
 * @param[in] le
 */
void CG_AddConstRGB(localEntity_t *le)
{
	refEntity_t *re = &le->refEntity;

	re->shaderRGBA[0] = (byte)(le->color[0] * 255);
	re->shaderRGBA[1] = (byte)(le->color[1] * 255);
	re->shaderRGBA[2] = (byte)(le->color[2] * 255);
	re->shaderRGBA[3] = (byte)(le->color[3] * 255);

	trap_R_AddRefEntityToScene(re);
}

/**
 * @brief CG_AddMoveScaleFade
 * @param[in] le
 */
static void CG_AddMoveScaleFade(localEntity_t *le)
{
	refEntity_t *re = &le->refEntity;
	float       c;
	float       len;

	// fade / grow time
	if (le->fadeInTime > le->startTime && cg.time < le->fadeInTime)
	{
		// fade / grow time
		c = 1.0f - (float) (le->fadeInTime - cg.time) / (le->fadeInTime - le->startTime);
	}
	else
	{
		// fade / grow time
		c = (le->endTime - cg.time) * le->lifeRate;
	}

	// spark
	if (!(le->leFlags & LEF_NOFADEALPHA))
	{
		re->shaderRGBA[3] = (byte)(0xff * c * le->color[3]);
	}

	if (!(le->leFlags & LEF_PUFF_DONT_SCALE))
	{
		c          = (le->endTime - cg.time) * le->lifeRate;
		re->radius = le->radius * (1.0f - c) + 8;
	}

	BG_EvaluateTrajectory(&le->pos, cg.time, re->origin, qfalse, -1);

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	len = VectorDistance(re->origin, cg.refdef_current->vieworg);
	if (len < le->radius)
	{
		CG_FreeLocalEntity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/**
 * @brief CG_AddScaleFade
 * @details For rocket smokes that hang in place, fade out, and are
 * removed if the view passes through them.
 * There are often many of these, so it needs to be simple.
 * @param[in] le
 */
static void CG_AddScaleFade(localEntity_t *le)
{
	refEntity_t *re = &le->refEntity;
	float       c   = (le->endTime - cg.time) * le->lifeRate; // fade / grow time
	float       len;

	re->shaderRGBA[3] = (byte)(0xff * c * le->color[3]);
	if (!(le->leFlags & LEF_PUFF_DONT_SCALE))
	{
		re->radius = le->radius * (1.0f - c) + 8;
	}

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	len = VectorDistance(re->origin, cg.refdef_current->vieworg);
	if (len < le->radius)
	{
		CG_FreeLocalEntity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/**
 * @brief CG_AddFallScaleFade
 * @details This is just an optimized CG_AddMoveScaleFade
 * For blood mists that drift down, fade out, and are
 * removed if the view passes through them.
 * There are often 100+ of these, so it needs to be simple.
 * @param[in] le
 */
static void CG_AddFallScaleFade(localEntity_t *le)
{
	refEntity_t *re = &le->refEntity;
	float       c   = (le->endTime - cg.time) * le->lifeRate; // fade time
	float       len;

	re->shaderRGBA[3] = (byte)(0xff * c * le->color[3]);

	re->origin[2] = le->pos.trBase[2] - (1.0f - c) * le->pos.trDelta[2];

	re->radius = le->radius * (1.0f - c) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	len = VectorDistance(re->origin, cg.refdef_current->vieworg);
	if (len < le->radius)
	{
		CG_FreeLocalEntity(le);
		return;
	}

	trap_R_AddRefEntityToScene(re);
}

/**
 * @brief CG_AddExplosion
 * @param[in] ex
 */
static void CG_AddExplosion(localEntity_t *ex)
{
	refEntity_t *ent = &ex->refEntity;

	// add the entity
	// don't add if shader is invalid
	if (ent->customShader >= 0)
	{
		trap_R_AddRefEntityToScene(ent);
	}

	// add the dlight
	if (ex->light != 0.f)
	{
		float light = (float)(cg.time - ex->startTime) / (ex->endTime - ex->startTime);

		if (light < 0.5f)
		{
			light = 1.0f;
		}
		else
		{
			light = 1.0f - (light - 0.5f) * 2;
		}
		light = ex->light * light;

		trap_R_AddLightToScene(ent->origin, 512, light, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2], 0, 0);
	}
}

/**
 * @brief CG_AddSpriteExplosion
 * @param[in] le
 */
static void CG_AddSpriteExplosion(localEntity_t *le)
{
	refEntity_t re = le->refEntity;
	float       c  = (le->endTime - cg.time) / ( float ) (le->endTime - le->startTime);

	if (c > 1)
	{
		c = 1.0;    // can happen during connection problems
	}

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = (byte)(0xff * c * 0.33f);

	re.reType = RT_SPRITE;
	re.radius = 42 * (1.0f - c) + 30;

	// move away from surface
	VectorMA(le->pos.trBase, (1.0f - c), le->pos.trDelta, re.origin);
	// done.

	// don't add if shader is invalid
	if (re.customShader >= 0)
	{
		trap_R_AddRefEntityToScene(&re);
	}

	// add the dlight
	//if (le->light || 1)
	{
		float light = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);

		if (light < 0.5f)
		{
			light = 1.0f;
		}
		else
		{
			light = 1.0f - (light - 0.5f) * 2;
		}
		trap_R_AddLightToScene(re.origin, 320, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], 0, 0);
	}
}

//==============================================================================

/**
 * @brief CG_AddLocalEntities
 */
void CG_AddLocalEntities(void)
{
	localEntity_t *le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;

	for ( ; le != &cg_activeLocalEntities ; le = next)
	{
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if (cg.time >= le->endTime)
		{
			CG_FreeLocalEntity(le);
			continue;
		}
		switch (le->leType)
		{
		default:
			CG_Error("Bad leType: %i\n", le->leType);
			break;
		case LE_MOVING_TRACER:
			CG_AddMovingTracer(le);
			break;
		case LE_SPARK:
			CG_AddSparkElements(le);
			break;
		case LE_FUSE_SPARK:
			CG_AddFuseSparkElements(le);
			break;
		case LE_DEBRIS:
			CG_AddDebrisElements(le);
			break;
		case LE_BLOOD:
			CG_AddBloodElements(le);
			break;
		case LE_MARK:
			break;
		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion(le);
			break;
		case LE_EXPLOSION:
			CG_AddExplosion(le);
			break;
		case LE_FRAGMENT:               // gibs and brass
			CG_AddFragment(le);
			break;
		case LE_MOVE_SCALE_FADE:        // water bubbles
			CG_AddMoveScaleFade(le);
			break;
		case LE_FADE_RGB:               // teleporters, railtrails
			CG_AddFadeRGB(le);
			break;
		case LE_CONST_RGB:
			CG_AddConstRGB(le);         // debug lines
			break;
		case LE_FALL_SCALE_FADE:        // gib blood trails
			CG_AddFallScaleFade(le);
			break;
		case LE_SCALE_FADE:             // rocket trails
			CG_AddScaleFade(le);
			break;
		case LE_EMITTER:
			CG_AddEmitter(le);
			break;
		}
	}
}
