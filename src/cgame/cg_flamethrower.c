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
 * @file cg_flamethrower.c
 * @brief Special code for the flamethrower effects
 *
 * The flameChunks behave similarly to the trailJunc's, except they are rendered
 * differently, and also interact with the environment.
 *
 * @note Some AI's are treated different, mostly for aesthetical reasons.
 */

#include "cg_local.h"

// a flameChunk is a ball or section of fuel which goes from fuel->blue ignition->flame ball
// optimization is necessary, since lots of these will be spawned, but as they grow, they can be
// merged so that less overdraw occurs
typedef struct flameChunk_s
{
	struct flameChunk_s *nextGlobal, *prevGlobal;   // next junction in the global list it is in (free or used)
	struct flameChunk_s *nextFlameChunk;            // next junction in the trail
	struct flameChunk_s *nextHead, *prevHead;       // next head junc in the world

	qboolean inuse;
	qboolean dead;          // set when a chunk is effectively inactive, but waiting to be freed
	int ownerCent;              // cent that spawned us

	int timeStart, timeEnd;
	float sizeMax;                  // start small, increase if we slow down
	float sizeRand;
	float sizeRate;                 // rate per ms, variable according to speed (larger if moving slower)
	vec3_t baseOrg;
	int baseOrgTime;
	vec3_t velDir;
	float velSpeed;                 // flame chunks should start with a fast velocity, then slow down if there is nothing behind them pushing them along
	float rollAngle;
	qboolean ignitionOnly;
	int blueLife;
	float gravity;
	vec3_t startVelDir;
	float speedScale;

	// current variables
	vec3_t org;
	float size;
	float lifeFrac;                 // 0.0 (baby) -> 1.0 (aged)

	int lastFriction, lastFrictionTake;
	vec3_t parentFwd;
} flameChunk_t;

// lowered this from 2048.  Still allows 6-9 people flaming.
#define MAX_FLAME_CHUNKS    1024
static flameChunk_t flameChunks[MAX_FLAME_CHUNKS];
static flameChunk_t *freeFlameChunks, *activeFlameChunks, *headFlameChunks;

static int numFlameChunksInuse;

// this structure stores information relevant to each cent in the game, this way we keep
// the flamethrower data seperate to the rest of the code, which helps if we decide against
// using this weapon in the game
typedef struct centFlameInfo_s
{
	int lastClientFrame;            // client frame that we last fired the flamethrower
	vec3_t lastAngles;              // angles at last firing
	vec3_t lastOrigin;              // origin at last firing
	flameChunk_t
	*lastFlameChunk;                // flame chunk we last spawned
	int lastSoundUpdate;

	qboolean lastFiring;
} centFlameInfo_t;

static centFlameInfo_t centFlameInfo[MAX_GENTITIES];

typedef struct
{
	float blowVolume;
	float streamVolume;
} flameSoundStatus_t;

static flameSoundStatus_t centFlameStatus[MAX_GENTITIES];

// procedure defs
flameChunk_t *CG_SpawnFlameChunk(flameChunk_t *headFlameChunk);
void CG_FlameCalcOrg(flameChunk_t *f, int time, vec3_t outOrg);

// these must be globals, since they cannot expand or contract, since that might result in them getting
//  stuck in geometry. therefore when a chunk hits a surface, we should deflect it away from the surface
//  slightly, rather than running along it, so that as the chink grows, the sprites don't sink into the
//  wall too much.
static vec3_t flameChunkMins = { 0, 0, 0 };
static vec3_t flameChunkMaxs = { 0, 0, 0 };

// disable this to stop rotating flames (this is variable so we can change it at run-time)
int rotatingFlames = qtrue;

/**
 * @brief CG_FlameLerpVec
 * @param[in] oldV
 * @param[in] newV
 * @param[in] backLerp
 * @param[in,out] outV
 */
void CG_FlameLerpVec(const vec3_t oldV, const vec3_t newV, float backLerp, vec3_t outV)
{
	VectorScale(newV, (1.0f - backLerp), outV);
	VectorMA(outV, backLerp, oldV, outV);
}

/**
 * @brief CG_FlameAdjustSpeed
 * @param[in,out] f
 * @param[in] change
 */
void CG_FlameAdjustSpeed(flameChunk_t *f, float change)
{
	if (f->velSpeed == 0.f && change == 0.f)
	{
		return;
	}

	f->velSpeed += change;
	if (f->velSpeed < FLAME_MIN_SPEED)
	{
		f->velSpeed = FLAME_MIN_SPEED;
	}
}

/**
 * @brief The given entity is firing a flamethrower
 * @param[in] cent
 * @param[in] origin
 * @param[in] angles
 * @param[in] speedScale
 * @param[in] firing
 */
void CG_FireFlameChunks(centity_t *cent, vec3_t origin, vec3_t angles, float speedScale, qboolean firing)
{
	centFlameInfo_t *centInfo;
	flameChunk_t    *f, *of;
	vec3_t          lastFwd, thisFwd, fwd;
	vec3_t          lastUp, thisUp, up;
	vec3_t          lastRight, thisRight, right;
	vec3_t          thisOrg, org;
	trace_t         trace;
	vec3_t          parentFwd;
	//float frametime, dot;

	centInfo = &centFlameInfo[cent->currentState.number];

	// for any other character or in 3rd person view, use entity angles for friction
	if (cent->currentState.number != cg.snap->ps.clientNum || cg_thirdPerson.integer)
	{
		AngleVectors(cent->currentState.apos.trBase, parentFwd, NULL, NULL);
	}
	else
	{
		AngleVectors(angles, parentFwd, NULL, NULL);
	}

	AngleVectors(angles, thisFwd, thisRight, thisUp);
	VectorCopy(origin, thisOrg);

	// if this entity was firing last frame, interpolate the angles as we spawn the chunks that
	// fired over the last frame
	if ((centInfo->lastClientFrame == cent->currentState.frame) &&
	    (centInfo->lastFlameChunk && centInfo->lastFiring == firing))
	{
		vec3_t lastOrg;
		double fracInc, ft;
		float  timeInc, backLerp;
		int    t;
		int    numFrameChunks = 0; // CHANGE: id

		AngleVectors(centInfo->lastAngles, lastFwd, lastRight, lastUp);
		VectorCopy(centInfo->lastOrigin, lastOrg);
		centInfo->lastFiring = firing;

		of       = centInfo->lastFlameChunk;
		timeInc  = 1000.0f * (firing ? 1.0f : 0.5f) * (FLAME_CHUNK_DIST / (FLAME_START_SPEED * speedScale));
		ft       = (double)of->timeStart + timeInc;
		t        = (int)ft;
		fracInc  = timeInc / (double)(cg.time - of->timeStart);
		backLerp = (float)(1.0 - fracInc);

		while (t <= cg.time)
		{
			// spawn a new chunk
			CG_FlameLerpVec(lastOrg, thisOrg, backLerp, org);

			CG_Trace(&trace, org, flameChunkMins, flameChunkMaxs, org, cent->currentState.number, MASK_SHOT | MASK_WATER);   // JPW NERVE water fixes
			if (trace.startsolid)
			{
				return;     // don't spawn inside a wall

			}
			f = CG_SpawnFlameChunk(of);

			if (!f)
			{
				//CG_Printf( "Out of flame chunks\n" );
				// CHANGE: id
				// to make sure we do not keep trying to add more and more chunks
				centInfo->lastFlameChunk->timeStart = cg.time;
				// end CHANGE: id
				return;
			}

			CG_FlameLerpVec(lastFwd, thisFwd, backLerp, fwd);
			VectorNormalize(fwd);
			CG_FlameLerpVec(lastRight, thisRight, backLerp, right);
			VectorNormalize(right);
			CG_FlameLerpVec(lastUp, thisUp, backLerp, up);
			VectorNormalize(up);

			f->timeStart = t;
			f->timeEnd   = (int)(t + FLAME_LIFETIME * (1.0f / (0.5f + 0.5f * speedScale)));
			f->size      = FLAME_START_SIZE * speedScale;
			f->sizeMax   = speedScale * (FLAME_START_MAX_SIZE + f->sizeRand * (firing ? 1.0f : 0.0f));
			f->sizeRand  = 0;

			if (f->sizeMax > FLAME_MAX_SIZE)
			{
				f->sizeMax = FLAME_MAX_SIZE;
			}

			f->sizeRate = GET_FLAME_BLUE_SIZE_SPEED(f->sizeMax * speedScale * (1.0f + (0.5f * (float)!firing)));
			VectorCopy(org, f->baseOrg);
			f->baseOrgTime = t;
			VectorCopy(fwd, f->velDir);
			VectorCopy(fwd, f->startVelDir);
			f->speedScale = speedScale;

			VectorNormalize(f->velDir);
			f->velSpeed     = FLAME_START_SPEED * (0.5f + 0.5f * speedScale) * (firing ? 1.0f : 4.5f);
			f->ownerCent    = cent->currentState.number;
			f->rollAngle    = crandom() * 179;
			f->ignitionOnly = (qboolean) !firing;

			if (!firing)
			{
				f->gravity  = -150;
				f->blueLife = (int)(FLAME_BLUE_LIFE * 0.1);
			}
			else
			{
				f->gravity  = 0;
				f->blueLife = FLAME_BLUE_LIFE;
			}
			f->lastFriction     = cg.time;
			f->lastFrictionTake = cg.time;
			VectorCopy(parentFwd, f->parentFwd);

			ft += (double)timeInc;
			// always spawn a chunk right on the current time
			if ((int)ft > cg.time && t < cg.time)
			{
				ft       = (double)cg.time;
				backLerp = (float)fracInc; // so it'll get set to zero a few lines down
			}
			t                        = (int)ft;
			backLerp                -= fracInc;
			centInfo->lastFlameChunk = of = f;
			// CHANGE: id
			// don't spawn too many chunks each frame
			if (++numFrameChunks > 50)
			{
				// to make sure we do not keep trying to add more and more chunks
				centInfo->lastFlameChunk->timeStart = cg.time;
				break;
			}
			// end CHANGE: id
		}
	}
	else
	{
		centInfo->lastFiring = firing;

		// just fire a single chunk to get us started
		f = CG_SpawnFlameChunk(NULL);

		if (!f)
		{
			//CG_Printf( "Out of flame chunks\n" );
			return;
		}

		VectorCopy(thisOrg, org);
		VectorCopy(thisFwd, fwd);
		VectorCopy(thisUp, up);
		VectorCopy(thisRight, right);

		f->timeStart = cg.time;
		f->timeEnd   = (int)(cg.time + FLAME_LIFETIME * (1.0f / (0.5f + 0.5f * speedScale)));
		f->size      = FLAME_START_SIZE * speedScale;
		f->sizeMax   = FLAME_START_MAX_SIZE * speedScale;
		if (f->sizeMax > FLAME_MAX_SIZE)
		{
			f->sizeMax = FLAME_MAX_SIZE;
		}

		f->sizeRand = 0;
		f->sizeRate = GET_FLAME_BLUE_SIZE_SPEED(f->sizeMax * speedScale);
		VectorCopy(org, f->baseOrg);
		f->baseOrgTime = cg.time;
		VectorCopy(fwd, f->velDir);
		VectorCopy(fwd, f->startVelDir);
		f->velSpeed     = FLAME_START_SPEED * (0.5f + 0.5f * speedScale);
		f->ownerCent    = cent->currentState.number;
		f->rollAngle    = crandom() * 179;
		f->ignitionOnly = (qboolean) !firing;
		f->speedScale   = speedScale;
		if (!firing)
		{
			f->gravity  = -100;
			f->blueLife = (int)(0.3f * (1.0f / speedScale) * (float)FLAME_BLUE_LIFE);
		}
		else
		{
			f->gravity  = 0;
			f->blueLife = FLAME_BLUE_LIFE;
		}
		f->lastFriction     = cg.time;
		f->lastFrictionTake = cg.time;
		VectorCopy(parentFwd, f->parentFwd);

		centInfo->lastFlameChunk = f;
	}

	// push them along
	/*
	f = centInfo->lastFlameChunk;
	while (f) {

	    if (f->lastFriction < cg.time - 50) {
	        frametime = (float)(cg.time - f->lastFriction) / 1000.0;
	        f->lastFriction = cg.time;
	        dot = DotProduct(parentFwd, f->parentFwd);
	        if (dot >= 0.99) {
	            dot -= 0.99;
	            dot *= (1.0/(1.0-0.99));
	            CG_FlameAdjustSpeed( f, 0.5 * frametime * FLAME_FRICTION_PER_SEC * pow(dot,4) );
	        }
	    }

	    f = f->nextFlameChunk;
	}
	*/

	VectorCopy(angles, centInfo->lastAngles);
	VectorCopy(origin, centInfo->lastOrigin);
	centInfo->lastClientFrame = cent->currentState.frame;
}

/**
 * @brief CG_ClearFlameChunks
 */
void CG_ClearFlameChunks(void)
{
	int i;

	Com_Memset(flameChunks, 0, sizeof(flameChunks));
	Com_Memset(centFlameInfo, 0, sizeof(centFlameInfo));

	freeFlameChunks   = flameChunks;
	activeFlameChunks = NULL;
	headFlameChunks   = NULL;

	flameChunks[0].nextGlobal = &flameChunks[1];

	for (i = 1 ; i < MAX_FLAME_CHUNKS - 1 ; i++)
	{
		flameChunks[i].nextGlobal = &flameChunks[i + 1];
		flameChunks[i].prevGlobal = &flameChunks[i - 1];
		flameChunks[i].inuse      = qfalse;
	}

	flameChunks[i].prevGlobal = &flameChunks[i - 1];

	numFlameChunksInuse = 0;
}

/**
 * @brief CG_SpawnFlameChunk
 * @param[in,out] headFlameChunk
 * @return
 */
flameChunk_t *CG_SpawnFlameChunk(flameChunk_t *headFlameChunk)
{
	flameChunk_t *f;

	if (!freeFlameChunks)
	{
		return NULL;
	}

	if (headFlameChunks && headFlameChunks->dead)
	{
		headFlameChunks = NULL;
	}

	// select the first free trail, and remove it from the list
	f               = freeFlameChunks;
	freeFlameChunks = f->nextGlobal;
	if (freeFlameChunks)
	{
		freeFlameChunks->prevGlobal = NULL;
	}

	f->nextGlobal = activeFlameChunks;
	if (activeFlameChunks)
	{
		activeFlameChunks->prevGlobal = f;
	}
	activeFlameChunks = f;
	f->prevGlobal     = NULL;
	f->inuse          = qtrue;
	f->dead           = qfalse;

	// if this owner has a headJunc, add us to the start
	if (headFlameChunk)
	{
		// remove the headJunc from the list of heads
		if (headFlameChunk == headFlameChunks)
		{
			headFlameChunks = headFlameChunks->nextHead;
			if (headFlameChunks)
			{
				headFlameChunks->prevHead = NULL;
			}
		}
		else
		{
			if (headFlameChunk->nextHead)
			{
				headFlameChunk->nextHead->prevHead = headFlameChunk->prevHead;
			}
			if (headFlameChunk->prevHead)
			{
				headFlameChunk->prevHead->nextHead = headFlameChunk->nextHead;
			}
		}
		headFlameChunk->prevHead = NULL;
		headFlameChunk->nextHead = NULL;
	}
	// make us the headTrail
	if (headFlameChunks)
	{
		headFlameChunks->prevHead = f;
	}
	f->nextHead     = headFlameChunks;
	f->prevHead     = NULL;
	headFlameChunks = f;

	f->nextFlameChunk = headFlameChunk; // if headJunc is NULL, then we'll just be the end of the list

	numFlameChunksInuse++;

	return f;
}

/**
 * @brief CG_FreeFlameChunk
 * @param[in,out] f
 */
void CG_FreeFlameChunk(flameChunk_t *f)
{
	// kill any juncs after us, so they aren't left hanging
	if (f->nextFlameChunk)
	{
		CG_FreeFlameChunk(f->nextFlameChunk);
		f->nextFlameChunk = NULL;
	}

	// make it non-active
	f->inuse = qfalse;
	f->dead  = qfalse;
	if (f->nextGlobal)
	{
		f->nextGlobal->prevGlobal = f->prevGlobal;
	}
	if (f->prevGlobal)
	{
		f->prevGlobal->nextGlobal = f->nextGlobal;
	}
	if (f == activeFlameChunks)
	{
		activeFlameChunks = f->nextGlobal;
	}

	// if it's a head, remove it
	if (f == headFlameChunks)
	{
		headFlameChunks = f->nextHead;
	}
	if (f->nextHead)
	{
		f->nextHead->prevHead = f->prevHead;
	}
	if (f->prevHead)
	{
		f->prevHead->nextHead = f->nextHead;
	}
	f->nextHead = NULL;
	f->prevHead = NULL;

	// stick it in the free list
	f->prevGlobal = NULL;
	f->nextGlobal = freeFlameChunks;
	if (freeFlameChunks)
	{
		freeFlameChunks->prevGlobal = f;
	}
	freeFlameChunks = f;

	numFlameChunksInuse--;
}

/**
 * @brief CG_MergeFlameChunks
 * @details Assumes f1 comes before f2
 * @param[in,out] f1
 * @param[in] f2
 */
void CG_MergeFlameChunks(flameChunk_t *f1, flameChunk_t *f2)
{
	if (f1->nextFlameChunk != f2)
	{
		CG_Error("CG_MergeFlameChunks: f2 doesn't follow f1, cannot merge\n");
	}

	f1->nextFlameChunk = f2->nextFlameChunk;
	f2->nextFlameChunk = NULL;

	VectorCopy(f2->velDir, f1->velDir);

	VectorCopy(f2->baseOrg, f1->baseOrg);
	f1->baseOrgTime = f2->baseOrgTime;

	f1->velSpeed  = f2->velSpeed;
	f1->sizeMax   = f2->sizeMax;
	f1->size      = f2->size;
	f1->timeStart = f2->timeStart;
	f1->timeEnd   = f2->timeEnd;

	CG_FreeFlameChunk(f2);
}

/**
 * @brief CG_FlameCalcOrg
 * @param[in] f
 * @param[in] time
 * @param[out] outOrg
 */
void CG_FlameCalcOrg(flameChunk_t *f, int time, vec3_t outOrg)
{
	VectorMA(f->baseOrg, f->velSpeed * ((float)(time - f->baseOrgTime) / 1000), f->velDir, outOrg);
	//outOrg[2] -= f->gravity * ((float)(time - f->timeStart)/1000.0) * ((float)(time - f->timeStart)/1000.0);
}

/**
 * @brief CG_MoveFlameChunk
 * @param[in,out] f
 */
void CG_MoveFlameChunk(flameChunk_t *f)
{
	vec3_t  newOrigin, sOrg;
	trace_t trace;
	float   dot;

	// subtract friction from speed
	if (f->velSpeed > 1 && f->lastFrictionTake < cg.time - 50)
	{
		CG_FlameAdjustSpeed(f, -((float)(cg.time - f->lastFrictionTake) / 1000.0f) * FLAME_FRICTION_PER_SEC);
		f->lastFrictionTake = cg.time;
	}

	// adjust size
	if (f->size < f->sizeMax)
	{
		if ((cg.time - f->timeStart) < f->blueLife)
		{
			f->sizeRate = GET_FLAME_BLUE_SIZE_SPEED(FLAME_START_MAX_SIZE);    // use a constant so the blue flame doesn't distort
		}
		else
		{
			f->sizeRate = GET_FLAME_SIZE_SPEED(f->sizeMax);
		}

		f->size += f->sizeRate * (float)(cg.time - f->baseOrgTime);
		if (f->size > f->sizeMax)
		{
			f->size = f->sizeMax;
		}
	}

	VectorCopy(f->baseOrg, sOrg);
	while (f->velSpeed > 1 && f->baseOrgTime != cg.time)
	{
		CG_FlameCalcOrg(f, cg.time, newOrigin);

		// trace a line from previous position to new position
		CG_Trace(&trace, sOrg, flameChunkMins, flameChunkMaxs, newOrigin, f->ownerCent, MASK_SHOT | MASK_WATER);   // JPW NERVE water fixes

		if (trace.startsolid)
		{
			f->velSpeed = 0;
			f->dead     = qtrue; // water fixes
			break;
		}

		if (trace.surfaceFlags & SURF_NOIMPACT)
		{
			break;
		}

		// moved some distance
		VectorCopy(trace.endpos, f->baseOrg);
		f->baseOrgTime += (int)((float)(cg.time - f->baseOrgTime) * trace.fraction);

		if (trace.fraction == 1.0f)
		{
			// check for hitting client
			if ((f->ownerCent != cg.snap->ps.clientNum) && !(cg.snap->ps.eFlags & EF_DEAD) && VectorDistance(newOrigin, cg.snap->ps.origin) < 32)
			{
				VectorNegate(f->velDir, trace.plane.normal);
			}
			else
			{
				break;
			}
		}

		// reflect off surface
		dot = DotProduct(f->velDir, trace.plane.normal);
		VectorMA(f->velDir, -2 * dot, trace.plane.normal, f->velDir);
		VectorNormalize(f->velDir);
		// subtract some speed
		f->velSpeed *= 0.5f * (0.25f + 0.75f * ((dot + 1.0f) * 0.5f));
		VectorCopy(f->velDir, f->parentFwd);

		VectorCopy(f->baseOrg, sOrg);
	}

	CG_FlameCalcOrg(f, cg.time, f->org);
	f->baseOrgTime = cg.time;   // incase we skipped the movement
}

static vec3_t vright, vup;
static vec3_t rright, rup;

#ifdef ETLEGACY_DEBUG   // just in case we forget about it, but it should be disabled at all times (only enabled to generate updated shaders)
#ifdef ALLOW_GEN_SHADERS    // secondary security measure

//#define   GEN_FLAME_SHADER

#endif  // ALLOW_GEN_SHADERS
#endif  // ETLEGACY_DEBUG

#define FLAME_BLEND_SRC     "GL_ONE"
#define FLAME_BLEND_DST     "GL_ONE_MINUS_SRC_COLOR"

#define NUM_FLAME_SPRITES       45
#define FLAME_SPRITE_DIR        "twiltb2"

#define NUM_NOZZLE_SPRITES  8

static qhandle_t flameShaders[NUM_FLAME_SPRITES];
static qhandle_t nozzleShaders[NUM_NOZZLE_SPRITES];
//static qboolean  initFlameShaders = qtrue;

//#define MAX_CLIPPED_FLAMES  8       // dont draw more than this many per frame
//static int numClippedFlames;

/**
 * @brief CG_AddFlameSpriteToScene
 * @param[in] f
 * @param[in] lifeFrac
 * @param[in] alpha
 */
void CG_AddFlameSpriteToScene(flameChunk_t *f, float lifeFrac, float alpha)
{
	vec3_t        point, p2, sProj;
	float         radius, sdist;
	int           frameNum;
	vec3_t        vec;
	unsigned char alphaChar;
	vec2_t        rST;
	polyBuffer_t  *pPolyBuffer;

	if (alpha < 0)
	{
		return; // we dont want to see this
	}

	radius = (f->size / 2.0f);
	if (radius < 6)
	{
		radius = 6;
	}

	if (CG_CullPointAndRadius(f->org, radius))
	{
		return;
	}

	rST[0]    = radius * 1.0f;
	rST[1]    = radius * 1.0f / 1.481f;
	alphaChar = (unsigned char)(255.0f * alpha);

	frameNum = (int)floor((double)(lifeFrac * NUM_FLAME_SPRITES));
	if (frameNum < 0)
	{
		frameNum = 0;
	}
	else if (frameNum > NUM_FLAME_SPRITES - 1)
	{
		frameNum = NUM_FLAME_SPRITES - 1;
	}

	pPolyBuffer = CG_PB_FindFreePolyBuffer(flameShaders[frameNum], 4, 6);

	pPolyBuffer->color[pPolyBuffer->numVerts + 0][0] = alphaChar;
	pPolyBuffer->color[pPolyBuffer->numVerts + 0][1] = alphaChar;
	pPolyBuffer->color[pPolyBuffer->numVerts + 0][2] = alphaChar;
	pPolyBuffer->color[pPolyBuffer->numVerts + 0][3] = alphaChar;

	Com_Memcpy(pPolyBuffer->color[pPolyBuffer->numVerts + 1], pPolyBuffer->color[pPolyBuffer->numVerts + 0], sizeof(pPolyBuffer->color[0]));
	Com_Memcpy(pPolyBuffer->color[pPolyBuffer->numVerts + 2], pPolyBuffer->color[pPolyBuffer->numVerts + 0], sizeof(pPolyBuffer->color[0]));
	Com_Memcpy(pPolyBuffer->color[pPolyBuffer->numVerts + 3], pPolyBuffer->color[pPolyBuffer->numVerts + 0], sizeof(pPolyBuffer->color[0]));

	// find the projected distance from the eye to the projection of the flame origin
	// onto the view direction vector
	VectorMA(cg.refdef_current->vieworg, 1024, cg.refdef_current->viewaxis[0], p2);
	ProjectPointOntoVector(f->org, cg.refdef_current->vieworg, p2, sProj);

	// make sure its infront of us
	VectorSubtract(sProj, cg.refdef_current->vieworg, vec);
	sdist = VectorNormalize(vec);
	if (sdist == 0.f || DotProduct(vec, cg.refdef_current->viewaxis[0]) < 0)
	{
		return;
	}

	if (rotatingFlames)           // no rotate for alt flame shaders
	{
		vec3_t rotate_ang;

		vectoangles(cg.refdef_current->viewaxis[0], rotate_ang);
		rotate_ang[ROLL] += f->rollAngle;
		AngleVectors(rotate_ang, NULL, rright, rup);
	}
	else
	{
		VectorCopy(vright, rright);
		VectorCopy(vup, rup);
	}

	VectorMA(f->org, -rST[1], rup, point);
	VectorMA(point, -rST[0], rright, point);
	VectorCopy(point, pPolyBuffer->xyz[pPolyBuffer->numVerts + 0]);
	pPolyBuffer->st[pPolyBuffer->numVerts + 0][0] = 0;
	pPolyBuffer->st[pPolyBuffer->numVerts + 0][1] = 0;

	VectorMA(point, rST[1] * 2, rup, point);
	VectorCopy(point, pPolyBuffer->xyz[pPolyBuffer->numVerts + 1]);
	pPolyBuffer->st[pPolyBuffer->numVerts + 1][0] = 0;
	pPolyBuffer->st[pPolyBuffer->numVerts + 1][1] = 1;

	VectorMA(point, rST[0] * 2, rright, point);
	VectorCopy(point, pPolyBuffer->xyz[pPolyBuffer->numVerts + 2]);
	pPolyBuffer->st[pPolyBuffer->numVerts + 2][0] = 1;
	pPolyBuffer->st[pPolyBuffer->numVerts + 2][1] = 1;

	VectorMA(point, -rST[1] * 2, rup, point);
	VectorCopy(point, pPolyBuffer->xyz[pPolyBuffer->numVerts + 3]);
	pPolyBuffer->st[pPolyBuffer->numVerts + 3][0] = 1;
	pPolyBuffer->st[pPolyBuffer->numVerts + 3][1] = 0;

	pPolyBuffer->indicies[pPolyBuffer->numIndicies + 0] = (unsigned int)pPolyBuffer->numVerts + 0;
	pPolyBuffer->indicies[pPolyBuffer->numIndicies + 1] = (unsigned int)pPolyBuffer->numVerts + 1;
	pPolyBuffer->indicies[pPolyBuffer->numIndicies + 2] = (unsigned int)pPolyBuffer->numVerts + 2;

	pPolyBuffer->indicies[pPolyBuffer->numIndicies + 3] = (unsigned int)pPolyBuffer->numVerts + 2;
	pPolyBuffer->indicies[pPolyBuffer->numIndicies + 4] = (unsigned int)pPolyBuffer->numVerts + 3;
	pPolyBuffer->indicies[pPolyBuffer->numIndicies + 5] = (unsigned int)pPolyBuffer->numVerts + 0;

	pPolyBuffer->numIndicies += 6;
	pPolyBuffer->numVerts    += 4;

}

static int nextFlameLight = 0;
static int lastFlameOwner = -1;


#define FLAME_SOUND_RANGE               1024.0f
#define FLAME_SPRITE_START_BLUE_SCALE   0.2f

#define FLAME_BSCALE                    1.0f // fire stream

/**
 * @brief CG_AddFlameToScene
 * @param[in] fHead
 */
void CG_AddFlameToScene(flameChunk_t *fHead)
{
	flameChunk_t  *f, *fNext;
	int           blueTrailHead = 0, fuelTrailHead = 0;
	static vec3_t whiteColor = { 1, 1, 1 };
	vec3_t        c;
	float         alpha;
	float         lived;
	int           headTimeStart;
	float         vdist, bdot;
	flameChunk_t  *lastBlowChunk = NULL;
	qboolean      isClientFlame;
	int           shader;
	flameChunk_t  *lastBlueChunk = NULL;
	qboolean      skip = qfalse, droppedTrail;
	vec3_t        v;
	vec3_t        lightOrg;         // origin to place light at
	float         lightSize;
	float         lightFlameCount;
	float         lastFuelAlpha;

	isClientFlame = (qboolean)(fHead == centFlameInfo[fHead->ownerCent].lastFlameChunk);

	if ((cg_entities[fHead->ownerCent].currentState.eFlags & EF_FIRING) && (centFlameInfo[fHead->ownerCent].lastFlameChunk == fHead))
	{
		headTimeStart = fHead->timeStart;
	}
	else
	{
		headTimeStart = cg.time;
	}

	VectorClear(lightOrg);
	lightSize       = 0;
	lightFlameCount = 0;

	lastFuelAlpha = 1.0;

	f = fHead;
	while (f)
	{

		if (f->nextFlameChunk && f->nextFlameChunk->dead)
		{
			// kill it
			CG_FreeFlameChunk(f->nextFlameChunk);
			f->nextFlameChunk = NULL;
		}

		// draw this chunk

		fNext = f->nextFlameChunk;
		lived = (float)(headTimeStart - f->timeStart);

		// update the "blow" sound volume (louder as we sway it)
		vdist = Distance(cg.refdef_current->vieworg, f->org);   // NOTE: this needs to be here or the flameSound code further below won't work
		if (lastBlowChunk && (centFlameStatus[f->ownerCent].blowVolume < 1.0f) &&
		    ((bdot = DotProduct(lastBlowChunk->startVelDir, f->startVelDir)) < 1.0f))
		{
			if (vdist < FLAME_SOUND_RANGE)
			{
				centFlameStatus[f->ownerCent].blowVolume += 500.0f * (1.0f - bdot) * (1.0f - (vdist / FLAME_SOUND_RANGE));
				if (centFlameStatus[f->ownerCent].blowVolume > 1.0f)
				{
					centFlameStatus[f->ownerCent].blowVolume = 1.0f;
				}
			}
		}
		lastBlowChunk = f;

		VectorMA(lightOrg, f->size / 20.0f, f->org, lightOrg);
		lightSize       += f->size;
		lightFlameCount += f->size / 20.0f;

		droppedTrail = qfalse;

		// is it a stream chunk? (no special handling)
		if (!f->ignitionOnly && f->velSpeed < 1)
		{
			CG_AddFlameSpriteToScene(f, f->lifeFrac, 1.0);

			// is it in the blue ignition section of the flame?
		}
		else if (isClientFlame && f->blueLife > (lived / 2.0f) && !cgs.matchPaused)
		{

			skip = qfalse;

			// if this is backwards from the last chunk, then skip it
			if (fNext && f != fHead && lastBlueChunk)
			{
				VectorSubtract(f->org, lastBlueChunk->org, v);
				if (VectorNormalize(v) < f->size / 2)
				{
					skip = qtrue;
				}
				else if (DotProduct(v, f->velDir) < 0)
				{
					skip = qtrue;
				}
			}

			// stream sound
			if (!f->ignitionOnly)
			{
				centFlameStatus[f->ownerCent].streamVolume += 0.05f;
				if (centFlameStatus[f->ownerCent].streamVolume > 1.0f)
				{
					centFlameStatus[f->ownerCent].streamVolume = 1.0f;
				}
			}

			if (!skip)
			{
				// just call this for damage checking
				//if (!f->ignitionOnly)
				//CG_AddFlameSpriteToScene( f, f->lifeFrac, -1 );

				lastBlueChunk = f;

				alpha = 1.0;    // new nozzle sprite
				VectorScale(whiteColor, alpha, c);

				if (f->blueLife > lived * 3.0f)
				{

					shader = nozzleShaders[(cg.time / 50 + (cg.time / 50 >> 1)) % NUM_NOZZLE_SPRITES];

					blueTrailHead = CG_AddTrailJunc(blueTrailHead,
					                                NULL,     // rain - zinx's trail fix
					                                shader,
					                                cg.time,
					                                STYPE_STRETCH,
					                                f->org,
					                                1,
					                                alpha, alpha,
					                                f->size * (f->ignitionOnly /*&& (cg.snap->ps.clientNum != f->ownerCent || cg_thirdPerson.integer)*/ ? 2.0f : 1.0f),
					                                FLAME_MAX_SIZE,
					                                TJFL_NOCULL | TJFL_FIXDISTORT,
					                                c, c, 1.0, 5.0);
				}

				// fire stream
				if (!f->ignitionOnly)
				{
					qboolean fskip = qfalse;

					if (!f->nextFlameChunk)
					{
						alpha = 0;
					}
					else if (lived / 1.3f < FLAME_BSCALE * FLAME_BLUE_FADEIN_TIME(f->blueLife))
					{
						alpha = FLAME_BLUE_MAX_ALPHA * ((lived / 1.3f) / (FLAME_BSCALE * FLAME_BLUE_FADEIN_TIME(f->blueLife)));
					}
					else if (lived / 1.3f < (f->blueLife - FLAME_BLUE_FADEOUT_TIME(f->blueLife)))
					{
						alpha = FLAME_BLUE_MAX_ALPHA;
					}
					else
					{
						alpha = FLAME_BLUE_MAX_ALPHA * (1.0f - ((lived / 1.3f - (f->blueLife - FLAME_BLUE_FADEOUT_TIME(f->blueLife))) / (FLAME_BLUE_FADEOUT_TIME(f->blueLife))));
					}
					if (alpha <= 0.0f)
					{
						alpha = 0.0f;
						if (lastFuelAlpha <= 0.0f)
						{
							fskip = qtrue;
						}
					}

					if (!fskip)
					{
						lastFuelAlpha = alpha;

						VectorScale(whiteColor, alpha, c);

						droppedTrail = qtrue;

						fuelTrailHead = CG_AddTrailJunc(fuelTrailHead,
						                                NULL,     // trail fix
						                                cgs.media.flamethrowerFireStream,
						                                cg.time,
						                                (f->ignitionOnly ? STYPE_STRETCH : STYPE_REPEAT),
						                                f->org,
						                                1,
						                                alpha, alpha,
						                                (f->size / 2 < f->sizeMax / 4 ? f->size / 2 : f->sizeMax / 4),
						                                FLAME_MAX_SIZE,
						                                TJFL_NOCULL | TJFL_FIXDISTORT | TJFL_CROSSOVER,
						                                c, c, 0.5, 1.5);
					}
				}
			}
		}

		if (!f->ignitionOnly &&
		    ((float)(FLAME_SPRITE_START_BLUE_SCALE * f->blueLife) < lived))
		{

			float alpha, lifeFrac;

			// should we merge it with the next sprite?
			while (fNext && !droppedTrail)
			{
				if ((Distance(f->org, fNext->org) < ((0.1f + 0.9f * f->lifeFrac) * f->size * 0.35f))
				    &&  (Q_fabs(f->size - fNext->size) < (40.0f))
				    &&  (abs(f->timeStart - fNext->timeStart) < 100)
				    &&  (DotProduct(f->velDir, fNext->velDir) > 0.99f)
				    )
				{
					CG_MergeFlameChunks(f, fNext);
					fNext = f->nextFlameChunk;          // it may have changed
				}
				else
				{
					break;
				}
			}

			lifeFrac = (lived - FLAME_SPRITE_START_BLUE_SCALE * f->blueLife) / ((float)FLAME_LIFETIME - FLAME_SPRITE_START_BLUE_SCALE * f->blueLife);

			alpha = (1.0f - lifeFrac) * 1.4f;
			if (alpha > 1.0f)
			{
				alpha = 1.0f;
			}

			// draw the sprite
			CG_AddFlameSpriteToScene(f, lifeFrac, alpha);

			// update the sizeRate
			f->sizeRate = GET_FLAME_SIZE_SPEED(f->sizeMax);
		}

		f = fNext;
	}

	if (lastFlameOwner == fHead->ownerCent && nextFlameLight == cg.clientFrame)
	{
		return;
	}

	if (!fHead->ignitionOnly)
	{
		nextFlameLight = cg.clientFrame;
		lastFlameOwner = fHead->ownerCent;
	}

	if (lightSize < 80)
	{
		lightSize = 80;
	}

	if (lightSize > 500)
	{
		lightSize = 500;
	}
	lightSize *= 1.0 + 0.2 * (sin(1.0 * cg.time / 50.0) * cos(1.0 * cg.time / 43.0));
	// set the alpha
	alpha = lightSize * 0.005f;
	if (alpha > 2.0f)
	{
		alpha = 2.0f;
	}
	VectorScale(lightOrg, 1.0f / lightFlameCount, lightOrg);
	// if it's only a nozzle, make it blue
	if (fHead->ignitionOnly)
	{
		trap_R_AddLightToScene(lightOrg, 80, alpha, 0.2f, 0.21f, 0.5, 0, 0);
	}
	else if (isClientFlame || (fHead->ownerCent == cg.snap->ps.clientNum))
	{
		trap_R_AddLightToScene(lightOrg, 320, alpha, 1.0f, 0.603922f, 0.207843f, 0, 0);
	}
}

#ifdef GEN_FLAME_SHADER
/**
 * @brief A util to create a bunch of shaders in a unique shader file, which represent an animation
 * @param[in] filename
 * @param[in] shaderName
 * @param[in] dir
 * @param[in] numFrames
 * @param[in] srcBlend
 * @param[in] dstBlend
 * @param[in] extras
 * @param[in] compressedVersionAvailable
 * @param[in] nomipmap
 */
void CG_GenerateShaders(char *filename, char *shaderName, char *dir, int numFrames, char *srcBlend, char *dstBlend, char *extras, qboolean compressedVersionAvailable, qboolean nomipmap)
{
	fileHandle_t f;
	int          b, c, d, lastNumber;
	char         str[512];
	int          i;

	trap_FS_FOpenFile(filename, &f, FS_WRITE);
	for (i = 0; i < numFrames; i++)
	{
		lastNumber  = i;
		b           = lastNumber / 100;
		lastNumber -= b * 100;
		c           = lastNumber / 10;
		lastNumber -= c * 10;
		d           = lastNumber;

		if (compressedVersionAvailable)
		{
			Com_sprintf(str, sizeof(str), "%s%i\n{\n\tnofog%s\n\tallowCompress\n\tcull none\n\t{\n\t\tmapcomp sprites/%s_lg/spr%i%i%i.tga\n\t\tmapnocomp sprites/%s/spr%i%i%i.tga\n\t\tblendFunc %s %s\n%s\t}\n}\n", shaderName, i + 1, nomipmap ? "\n\tnomipmaps" : "", dir, b, c, d, dir, b, c, d, srcBlend, dstBlend, extras);
		}
		else
		{
			Com_sprintf(str, sizeof(str), "%s%i\n{\n\tnofog%s\n\tallowCompress\n\tcull none\n\t{\n\t\tmap sprites/%s/spr%i%i%i.tga\n\t\tblendFunc %s %s\n%s\t}\n}\n", shaderName, i + 1, nomipmap ? "\n\tnomipmap" : "", dir, b, c, d, srcBlend, dstBlend, extras);
		}
		trap_FS_Write(str, strlen(str), f);
	}
	trap_FS_FCloseFile(f);
}
#endif

/**
 * @brief CG_InitFlameChunks
 */
void CG_InitFlameChunks(void)
{
	int  i;
	char filename[MAX_QPATH];

	CG_ClearFlameChunks();

#ifdef GEN_FLAME_SHADER
	CG_GenerateShaders("scripts/flamethrower.shader",
	                   "flamethrowerFire",
	                   FLAME_SPRITE_DIR,
	                   NUM_FLAME_SPRITES,
	                   FLAME_BLEND_SRC,
	                   FLAME_BLEND_DST,
	                   "",
	                   qtrue, qtrue);

	CG_GenerateShaders("scripts/blacksmokeanim.shader",
	                   "blacksmokeanim",
	                   "explode1",
	                   23,
	                   "GL_ZERO",
	                   "GL_ONE_MINUS_SRC_ALPHA",
	                   "\t\talphaGen const 0.2\n",
	                   qfalse, qfalse);

	CG_GenerateShaders("scripts/viewflames.shader",
	                   "viewFlashFire",
	                   "clnfire",
	                   16,
	                   "GL_ONE",
	                   "GL_ONE",
	                   "\t\talphaGen vertex\n\t\trgbGen vertex\n",
	                   qtrue, qtrue);
	/*
	CG_GenerateShaders("scripts/twiltb.shader",
	                   "twiltb",
	                   "twiltb",
	                   42,
	                   "GL_SRC_ALPHA",
	                   "GL_ONE_MINUS_SRC_COLOR",
	                   "",
	                   qtrue, qfalse);
	*/
	CG_GenerateShaders("scripts/twiltb2.shader",
	                   "twiltb2",
	                   "twiltb2",
	                   45,
	                   "GL_ONE",
	                   "GL_ONE_MINUS_SRC_COLOR",
	                   "",
	                   qtrue, qfalse);
	/*
	    CG_GenerateShaders( "scripts/expblue.shader",
	                        "expblue",
	                        "expblue",
	                        25,
	                        "GL_ONE",
	                        "GL_ONE_MINUS_SRC_COLOR",
	                        "",
	                        qfalse, qfalse );

	CG_GenerateShaders("scripts/firest.shader",
	                   "firest",
	                   "firest",
	                   36,
	                   "GL_ONE",
	                   "GL_ONE_MINUS_SRC_COLOR",
	                   "",
	                   qtrue, qfalse);
	*/
	CG_GenerateShaders("scripts/explode1.shader",
	                   "explode1",
	                   "explode1",
	                   23,
	                   "GL_ONE",
	                   "GL_ONE_MINUS_SRC_COLOR",
	                   "",
	                   qtrue, qfalse);
	/*
	CG_GenerateShaders("scripts/funnel.shader",
	                   "funnel",
	                   "funnel",
	                   21,
	                   "GL_ONE",
	                   "GL_ONE_MINUS_SRC_COLOR",
	                   "",
	                   qfalse, qfalse);
	 */
#endif

	for (i = 0; i < NUM_FLAME_SPRITES; i++)
	{
		Com_sprintf(filename, MAX_QPATH, "flamethrowerFire%i", i + 1);
		flameShaders[i] = trap_R_RegisterShader(filename);
	}
	for (i = 0; i < NUM_NOZZLE_SPRITES; i++)
	{
		Com_sprintf(filename, MAX_QPATH, "nozzleFlame%i", i + 1);
		nozzleShaders[i] = trap_R_RegisterShader(filename);
	}
	//initFlameShaders = qfalse;
}

/**
 * @brief CG_AddFlameChunks
 */
void CG_AddFlameChunks(void)
{
	flameChunk_t *f, *fNext;

	//AngleVectors( cg.refdef.viewangles, NULL, vright, vup );
	VectorCopy(cg.refdef_current->viewaxis[1], vright);
	VectorCopy(cg.refdef_current->viewaxis[2], vup);

	// clear out the volumes so we can rebuild them
	Com_Memset(centFlameStatus, 0, sizeof(centFlameStatus));

	//numClippedFlames = 0;

	// age them
	f = activeFlameChunks;
	while (f)
	{
		if (!f->dead)
		{
			if (cgs.matchPaused)
			{
				f->timeStart        += cg.frametime;
				f->timeEnd          += cg.frametime;
				f->baseOrgTime      += cg.frametime;
				f->lastFriction     += cg.frametime;
				f->lastFrictionTake += cg.frametime;
			}

			if (cg.time > f->timeEnd)
			{
				f->dead = qtrue;
			}
			else if (f->ignitionOnly && (f->blueLife < (cg.time - f->timeStart)))
			{
				f->dead = qtrue;
			}
			else
			{
				CG_MoveFlameChunk(f);
				f->lifeFrac = (float)(cg.time - f->timeStart) / (float)(f->timeEnd - f->timeStart);
			}
		}
		f = f->nextGlobal;
	}

	// draw each of the headFlameChunk's
	f = headFlameChunks;
	while (f)
	{
		fNext = f->nextHead;        // in case it gets removed
		if (f->dead)
		{
			if (centFlameInfo[f->ownerCent].lastFlameChunk == f)
			{
				centFlameInfo[f->ownerCent].lastFlameChunk  = NULL;
				centFlameInfo[f->ownerCent].lastClientFrame = 0;
			}
			CG_FreeFlameChunk(f);
		}
		else if (!f->ignitionOnly || (centFlameInfo[f->ownerCent].lastFlameChunk == f)) // don't draw the ignition flame after we start firing
		{
			CG_AddFlameToScene(f);
		}
		f = fNext;
	}
}

#define MIN_BLOW_VOLUME     30

/**
 * @brief CG_UpdateFlamethrowerSounds
 */
void CG_UpdateFlamethrowerSounds(void)
{
	flameChunk_t *f, *trav;

	// draw each of the headFlameChunk's
	f = headFlameChunks;
	while (f)
	{
		// update this entity?
		if (centFlameInfo[f->ownerCent].lastSoundUpdate != cg.time)
		{
			// blow/ignition sound
			if (centFlameStatus[f->ownerCent].blowVolume * 255.0f > MIN_BLOW_VOLUME)
			{
				trap_S_AddLoopingSound(f->org, vec3_origin, cgs.media.flameBlowSound, (int)(255.0f * centFlameStatus[f->ownerCent].blowVolume), 0);
			}
			else
			{
				trap_S_AddLoopingSound(f->org, vec3_origin, cgs.media.flameBlowSound, MIN_BLOW_VOLUME, 0);
			}

			if (centFlameStatus[f->ownerCent].streamVolume != 0.f)
			{
				trap_S_AddLoopingSound(f->org, vec3_origin, cgs.media.flameStreamSound, (int)(255.0f * centFlameStatus[f->ownerCent].streamVolume), 0);
			}

			centFlameInfo[f->ownerCent].lastSoundUpdate = cg.time;
		}

		// traverse the chunks, spawning flame sound sources as we go
		for (trav = f; trav; trav = trav->nextFlameChunk)
		{
			// update the sound volume
			if (trav->blueLife + 100 < (cg.time - trav->timeStart))
			{
				trap_S_AddLoopingSound(trav->org, vec3_origin, cgs.media.flameSound, (int)(255.0f * (0.2f * (trav->size / FLAME_MAX_SIZE))), 0);
			}
		}

		f = f->nextHead;
	}
}
