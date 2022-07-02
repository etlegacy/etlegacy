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
 * @file cg_trails.c
 * @brief Draws a trail using multiple junction points
 */

#include "cg_local.h"

typedef struct trailJunc_s
{
	struct trailJunc_s *nextGlobal, *prevGlobal;    // next junction in the global list it is in (free or used)
	struct trailJunc_s *nextJunc;                   // next junction in the trail
	struct trailJunc_s *nextHead, *prevHead;        // next head junc in the world

	void *usedby;        // trail fix
	qboolean inuse, freed;

	qhandle_t shader;

	int sType;
	int flags;
	float sTex;
	vec3_t pos;
	int spawnTime, endTime;
	float alphaStart, alphaEnd;
	vec3_t colorStart, colorEnd;
	float widthStart, widthEnd;

	// current settings
	float alpha;
	float width;
	vec3_t color;

} trailJunc_t;

#define MAX_TRAILJUNCS  4096

trailJunc_t trailJuncs[MAX_TRAILJUNCS];
trailJunc_t *freeTrails, *activeTrails;
trailJunc_t *headTrails;

qboolean initTrails = qfalse;

int numTrailsInuse;

void CG_KillTrail(trailJunc_t *t);

/**
 * @brief CG_FreeTrailJunc
 * @param[in,out] junc
 */
static void CG_FreeTrailJunc(trailJunc_t *junc)
{
	// kill any juncs after us, so they aren't left hanging
	if (junc->nextJunc)
	{
		CG_KillTrail(junc);
	}

	// make it non-active
	junc->inuse = qfalse;
	junc->freed = qtrue;
	if (junc->nextGlobal)
	{
		junc->nextGlobal->prevGlobal = junc->prevGlobal;
	}
	if (junc->prevGlobal)
	{
		junc->prevGlobal->nextGlobal = junc->nextGlobal;
	}
	if (junc == activeTrails)
	{
		activeTrails = junc->nextGlobal;
	}

	// if it's a head, remove it
	if (junc == headTrails)
	{
		headTrails = junc->nextHead;
	}
	if (junc->nextHead)
	{
		junc->nextHead->prevHead = junc->prevHead;
	}
	if (junc->prevHead)
	{
		junc->prevHead->nextHead = junc->nextHead;
	}
	junc->nextHead = NULL;
	junc->prevHead = NULL;

	// stick it in the free list
	junc->prevGlobal = NULL;
	junc->nextGlobal = freeTrails;
	if (freeTrails)
	{
		freeTrails->prevGlobal = junc;
	}
	freeTrails = junc;

	numTrailsInuse--;
}

/**
 * @brief CG_KillTrail
 * @param[in,out] t
 */
void CG_KillTrail(trailJunc_t *t)
{
	trailJunc_t *next;

	if (!t->inuse && t->freed)
	{
		return;
	}
	next = t->nextJunc;
	if (next < &trailJuncs[0] || next >= &trailJuncs[MAX_TRAILJUNCS])
	{
		next = NULL;
	}
	t->nextJunc = NULL;
	if (next && next->nextJunc && next->nextJunc == t)
	{
		next->nextJunc = NULL;
	}
	if (next)
	{
		CG_FreeTrailJunc(next);
	}
}

/**
 * @brief CG_SpawnTrailJunc
 * @param[in,out] headJunc
 * @return
 */
static trailJunc_t *CG_SpawnTrailJunc(trailJunc_t *headJunc)
{
	trailJunc_t *j;

	if (!freeTrails)
	{
		return NULL;
	}

	if (cg_paused.integer)
	{
		return NULL;
	}

	// select the first free trail, and remove it from the list
	j          = freeTrails;
	freeTrails = j->nextGlobal;
	if (freeTrails)
	{
		freeTrails->prevGlobal = NULL;
	}

	j->nextGlobal = activeTrails;
	if (activeTrails)
	{
		activeTrails->prevGlobal = j;
	}
	activeTrails  = j;
	j->prevGlobal = NULL;
	j->inuse      = qtrue;
	j->freed      = qfalse;

	// if this owner has a headJunc, add us to the start
	if (headJunc)
	{
		// remove the headJunc from the list of heads
		if (headJunc == headTrails)
		{
			headTrails = headJunc->nextHead;
			if (headTrails)
			{
				headTrails->prevHead = NULL;
			}
		}
		else
		{
			if (headJunc->nextHead)
			{
				headJunc->nextHead->prevHead = headJunc->prevHead;
			}
			if (headJunc->prevHead)
			{
				headJunc->prevHead->nextHead = headJunc->nextHead;
			}
		}
		headJunc->prevHead = NULL;
		headJunc->nextHead = NULL;
	}
	// make us the headTrail
	if (headTrails)
	{
		headTrails->prevHead = j;
	}
	j->nextHead = headTrails;
	j->prevHead = NULL;
	headTrails  = j;

	j->nextJunc = headJunc; // if headJunc is NULL, then we'll just be the end of the list

	numTrailsInuse++;

	// debugging
	//CG_Printf( "NumTrails: %i\n", numTrailsInuse );

	return j;
}

/**
 * @brief Used for generic trails
 *
 * @param headJuncIndex
 * @param[in,out] usedby
 * @param[in] shader
 * @param[in] spawnTime
 * @param[in] sType
 * @param[in] pos
 * @param[in] trailLife
 * @param[in] alphaStart
 * @param[in] alphaEnd
 * @param[in] startWidth
 * @param[in] endWidth
 * @param[in] flags
 * @param[in] colorStart
 * @param[in] colorEnd
 * @param[in] sRatio
 * @param[in] animSpeed
 *
 * @return The index of the trail junction created
 */
int CG_AddTrailJunc(int headJuncIndex, void *usedby, qhandle_t shader, int spawnTime, int sType, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth, int flags, vec3_t colorStart, vec3_t colorEnd, float sRatio, float animSpeed)
{
	trailJunc_t *j, *headJunc;

	if (headJuncIndex < 0 || headJuncIndex >= MAX_TRAILJUNCS)
	{
		return 0;
	}

	if (headJuncIndex > 0)
	{
		headJunc = &trailJuncs[headJuncIndex - 1];

		// rain - zinx's trail fix
		if (!headJunc->inuse || headJunc->usedby != usedby)
		{
			headJunc = NULL;
		}
	}
	else
	{
		headJunc = NULL;
	}

	j = CG_SpawnTrailJunc(headJunc);
	if (!j)
	{
		// CG_Printf("couldnt spawn trail junc\n");
		return 0;
	}

	// trail fix - mark who's using this trail so that
	// we can handle the someone-else-stole-our-trail case
	j->usedby = usedby;

	if (alphaStart > 1.0f)
	{
		alphaStart = 1.0f;
	}
	if (alphaStart < 0.0f)
	{
		alphaStart = 0.0f;
	}
	if (alphaEnd > 1.0f)
	{
		alphaEnd = 1.0f;
	}
	if (alphaEnd < 0.0f)
	{
		alphaEnd = 0.0f;
	}

	// setup the trail junction
	j->shader = shader;
	j->sType  = sType;
	VectorCopy(pos, j->pos);
	j->flags = flags;

	j->spawnTime = spawnTime;
	j->endTime   = spawnTime + trailLife;

	VectorCopy(colorStart, j->colorStart);
	VectorCopy(colorEnd, j->colorEnd);

	j->alphaStart = alphaStart;
	j->alphaEnd   = alphaEnd;

	j->widthStart = startWidth;
	j->widthEnd   = endWidth;

	if (sType == STYPE_REPEAT)
	{
		if (sRatio == 0) // fix potential div 0
		{
			sRatio = 1;
		}

		if (headJunc)
		{
			j->sTex = headJunc->sTex + ((Distance(headJunc->pos, pos) / sRatio) / j->widthEnd);
		}
		else
		{
			// FIXME: need a way to specify offset timing
			j->sTex = (animSpeed * (1.0f - ((float)(cg.time % 1000) / 1000.0f))) / (sRatio);
			//j->sTex = 0;
		}
	}

	return ((int)(j - trailJuncs) + 1);
}

/**
 * @brief CG_AddSparkJunc
 *
 * @param[in] headJuncIndex
 * @param[in] usedby
 * @param[in] shader
 * @param[in] pos
 * @param[in] trailLife
 * @param[in] alphaStart
 * @param[in] alphaEnd
 * @param[in] startWidth
 * @param[in] endWidth
 *
 * @return The index of the trail junction created
 */
int CG_AddSparkJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth)
{
	trailJunc_t *j, *headJunc;

	if (headJuncIndex < 0 || headJuncIndex >= MAX_TRAILJUNCS)
	{
		return 0;
	}

	if (headJuncIndex > 0)
	{
		headJunc = &trailJuncs[headJuncIndex - 1];

		// trail fix
		if (!headJunc->inuse || headJunc->usedby != usedby)
		{
			headJunc = NULL;
		}
	}
	else
	{
		headJunc = NULL;
	}

	j = CG_SpawnTrailJunc(headJunc);
	if (!j)
	{
		return 0;
	}

	j->usedby = usedby;

	// setup the trail junction
	j->shader = shader;
	j->sType  = STYPE_STRETCH;
	VectorCopy(pos, j->pos);
	j->flags = TJFL_NOCULL;     // don't worry about fading up close

	j->spawnTime = cg.time;
	j->endTime   = cg.time + trailLife;

	VectorSet(j->colorStart, 1.0f, 0.8f + 0.2f * alphaStart, 0.4f + 0.4f * alphaStart);
	VectorSet(j->colorEnd, 1.0f, 0.8f + 0.2f * alphaEnd, 0.4f + 0.4f * alphaEnd);
	//VectorScale( j->colorStart, alphaStart, j->colorStart );
	//VectorScale( j->colorEnd, alphaEnd, j->colorEnd );

	j->alphaStart = alphaStart * 2;
	j->alphaEnd   = alphaEnd * 2;
	//j->alphaStart = 1.0;
	//j->alphaEnd = 1.0;

	j->widthStart = startWidth;
	j->widthEnd   = endWidth;

	return ((int)(j - trailJuncs) + 1);
}

#define ST_RATIO    4.0f     // sprite image: width / height

/**
 * @brief CG_AddSmokeJunc
 *
 * @param[in] headJuncIndex
 * @param[in] usedby
 * @param[in] shader
 * @param[in] pos
 * @param[in] trailLife
 * @param[in] alpha
 * @param[in] startWidth
 * @param[in] endWidth
 *
 * @return The index of the trail junction created
 */
int CG_AddSmokeJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alpha, float startWidth, float endWidth)
{
	trailJunc_t *j, *headJunc;

	if (headJuncIndex < 0 || headJuncIndex >= MAX_TRAILJUNCS)
	{
		return 0;
	}

	if (headJuncIndex > 0)
	{
		headJunc = &trailJuncs[headJuncIndex - 1];

		// trail fix
		if (!headJunc->inuse || headJunc->usedby != usedby)
		{
			headJunc = NULL;
		}
	}
	else
	{
		headJunc = NULL;
	}

	j = CG_SpawnTrailJunc(headJunc);
	if (!j)
	{
		return 0;
	}

	j->usedby = usedby;

	// setup the trail junction
	j->shader = shader;
	j->sType  = STYPE_REPEAT;
	VectorCopy(pos, j->pos);
	j->flags = TJFL_FADEIN;

	j->spawnTime = cg.time;
	j->endTime   = cg.time + trailLife;

	VectorSet(j->colorStart, 0.7f, 0.7f, 0.7f);
	VectorSet(j->colorEnd, 0.0f, 0.0f, 0.0f);

	j->alphaStart = alpha;
	j->alphaEnd   = 0.0f;

	j->widthStart = startWidth;
	j->widthEnd   = endWidth;

	if (headJunc)
	{
		j->sTex = headJunc->sTex + ((Distance(headJunc->pos, pos) / ST_RATIO) / j->widthEnd);
	}
	else
	{
		// first junction, so this will become the "tail" very soon, make it fade out
		j->sTex       = 0;
		j->alphaStart = 0.0f;
		j->alphaEnd   = 0.0f;
	}

	return ((int)(j - trailJuncs) + 1);
}

static vec3_t vforward, vright, vup;
#define MAX_TRAIL_VERTS     2048
static polyVert_t verts[MAX_TRAIL_VERTS];
static polyVert_t outVerts[MAX_TRAIL_VERTS * 3];

// clipping
#define TRAIL_FADE_CLOSE_DIST   64.0f
#define TRAIL_FADE_FAR_SCALE    4.0f

/**
 * @brief CG_AddTrailToScene
 * @param[in] trail
 * @param[in] iteration
 * @param[in] numJuncs
 *
 * @todo TODO: this can do with some major optimization
 */
static void CG_AddTrailToScene(trailJunc_t *trail, int iteration, int numJuncs)
{
	int         k, i, n, l, numOutVerts;
	polyVert_t  mid;
	float       mod[4];
	float       sInc, s;
	trailJunc_t *j, *jNext;
	vec3_t      up, p, v;
	// clipping vars
	vec3_t viewProj;
	float  viewDist, fadeAlpha;

	// add spark shader at head position
	if (trail->flags & TJFL_SPARKHEADFLARE)
	{
		polyBuffer_t *pPolyBuffer = CG_PB_FindFreePolyBuffer(cgs.media.sparkFlareShader, 4, 6);

		if (pPolyBuffer)
		{
			int pos = pPolyBuffer->numVerts;

			j = trail;

			VectorCopy(j->pos, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], -j->width * 2, vup, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], -j->width * 2, vright, pPolyBuffer->xyz[pos]);
			pPolyBuffer->st[pos][0] = 0;
			pPolyBuffer->st[pos][1] = 0;
			pos++;

			VectorCopy(j->pos, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], -j->width * 2, vup, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], j->width * 2, vright, pPolyBuffer->xyz[pos]);
			pPolyBuffer->st[pos][0] = 0;
			pPolyBuffer->st[pos][1] = 1;
			pos++;

			VectorCopy(j->pos, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], j->width * 2, vup, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], j->width * 2, vright, pPolyBuffer->xyz[pos]);
			pPolyBuffer->st[pos][0] = 1;
			pPolyBuffer->st[pos][1] = 1;
			pos++;

			VectorCopy(j->pos, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], j->width * 2, vup, pPolyBuffer->xyz[pos]);
			VectorMA(pPolyBuffer->xyz[pos], -j->width * 2, vright, pPolyBuffer->xyz[pos]);
			pPolyBuffer->st[pos][0] = 1;
			pPolyBuffer->st[pos][1] = 0;
			pos++;

			for (i = 0; i < 4; i++)
			{
				pPolyBuffer->color[pPolyBuffer->numVerts + i][0] = 255;
				pPolyBuffer->color[pPolyBuffer->numVerts + i][1] = 255;
				pPolyBuffer->color[pPolyBuffer->numVerts + i][2] = 255;
				pPolyBuffer->color[pPolyBuffer->numVerts + i][3] = ( unsigned char )(j->alpha * 255.0f);
			}

			pPolyBuffer->indicies[pPolyBuffer->numIndicies + 0] = (unsigned)pPolyBuffer->numVerts + 0;
			pPolyBuffer->indicies[pPolyBuffer->numIndicies + 1] = (unsigned)pPolyBuffer->numVerts + 1;
			pPolyBuffer->indicies[pPolyBuffer->numIndicies + 2] = (unsigned)pPolyBuffer->numVerts + 2;

			pPolyBuffer->indicies[pPolyBuffer->numIndicies + 3] = (unsigned)pPolyBuffer->numVerts + 2;
			pPolyBuffer->indicies[pPolyBuffer->numIndicies + 4] = (unsigned)pPolyBuffer->numVerts + 3;
			pPolyBuffer->indicies[pPolyBuffer->numIndicies + 5] = (unsigned)pPolyBuffer->numVerts + 0;

			pPolyBuffer->numVerts    += 4;
			pPolyBuffer->numIndicies += 6;
		}
	}

	//if (trail->flags & TJFL_CROSSOVER && iteration < 1) {
	//  iteration = 1;
	//}

	sInc = 0;

	if (!numJuncs)
	{
		// first count the number of juncs in the trail
		j        = trail;
		numJuncs = 0;
		sInc     = 0;
		while (j)
		{
			numJuncs++;

			// check for a dead next junc
			if (!j->inuse && j->nextJunc && !j->nextJunc->inuse)
			{
				CG_KillTrail(j);
			}
			else if (j->nextJunc && j->nextJunc->freed)
			{
				// not sure how this can happen, but it does, and causes infinite loops
				j->nextJunc = NULL;
			}

			if (j->nextJunc)
			{
				sInc += VectorDistance(j->nextJunc->pos, j->pos);
			}

			j = j->nextJunc;
		}
	}

	if (numJuncs < 2)
	{
		return;
	}

	s = 0;
	if (trail->sType == STYPE_STRETCH)
	{
		//sInc = ((1.0 - 0.1) / (float)(numJuncs)); // hack, the end of funnel shows a bit of the start (looping)
		s = 0.05f;
	}
	else if (trail->sType == STYPE_REPEAT)
	{
		s = trail->sTex;
	}

	// now traverse the list
	j     = trail;
	jNext = j->nextJunc;
	i     = 0;
	while (jNext)
	{
		// first get the directional vectors to the next junc
		GetPerpendicularViewVector(cg.refdef_current->vieworg, j->pos, jNext->pos, up);

		// if it's a crossover, draw it twice
		if (j->flags & TJFL_CROSSOVER)
		{
			if (iteration > 0)
			{
				ProjectPointOntoVector(cg.refdef_current->vieworg, j->pos, jNext->pos, viewProj);
				VectorSubtract(cg.refdef_current->vieworg, viewProj, v);
				VectorNormalize(v);

				if (iteration == 1)
				{
					VectorMA(up, 0.3f, v, up);
				}
				else
				{
					VectorMA(up, -0.3f, v, up);
				}
				VectorNormalize(up);
			}
		}
		// do fading when moving towards the projection point onto the trail segment vector
		else if (!(j->flags & TJFL_NOCULL) && (j->widthEnd > 4 || jNext->widthEnd > 4))
		{
			ProjectPointOntoVector(cg.refdef_current->vieworg, j->pos, jNext->pos, viewProj);
			viewDist = Distance(viewProj, cg.refdef_current->vieworg);
			if (viewDist < (TRAIL_FADE_CLOSE_DIST * TRAIL_FADE_FAR_SCALE))
			{
				if (viewDist < TRAIL_FADE_CLOSE_DIST)
				{
					fadeAlpha = 0.0;
				}
				else
				{
					fadeAlpha = (viewDist - TRAIL_FADE_CLOSE_DIST) / (TRAIL_FADE_CLOSE_DIST * TRAIL_FADE_FAR_SCALE);
				}
				if (fadeAlpha < j->alpha)
				{
					j->alpha = fadeAlpha;
				}
				if (fadeAlpha < jNext->alpha)
				{
					jNext->alpha = fadeAlpha;
				}
			}
		}

		// now output the QUAD for this segment

		// 1 ----
		VectorMA(j->pos, 0.5f * j->width, up, p);
		VectorCopy(p, verts[i].xyz);
		verts[i].st[0] = s;
		verts[i].st[1] = 1.0;
		for (k = 0; k < 3; k++)
		{
			verts[i].modulate[k] = ( unsigned char )(j->color[k] * 255.0f);
		}
		verts[i].modulate[3] = ( unsigned char )(j->alpha * 255.0f);

		// blend this with the previous junc
		if (j != trail)
		{
			VectorAdd(verts[i].xyz, verts[i - 1].xyz, verts[i].xyz);
			VectorScale(verts[i].xyz, 0.5f, verts[i].xyz);
			VectorCopy(verts[i].xyz, verts[i - 1].xyz);
		}
		else if (j->flags & TJFL_FADEIN)
		{
			verts[i].modulate[3] = 0;   // fade in
		}

		i++;

		// 2 ----
		VectorMA(p, -1 * j->width, up, p);
		VectorCopy(p, verts[i].xyz);
		verts[i].st[0] = s;
		verts[i].st[1] = 0;
		for (k = 0; k < 3; k++)
		{
			verts[i].modulate[k] = ( unsigned char )(j->color[k] * 255.0f);
		}
		verts[i].modulate[3] = ( unsigned char )(j->alpha * 255.0f);

		// blend this with the previous junc
		if (j != trail)
		{
			VectorAdd(verts[i].xyz, verts[i - 3].xyz, verts[i].xyz);
			VectorScale(verts[i].xyz, 0.5f, verts[i].xyz);
			VectorCopy(verts[i].xyz, verts[i - 3].xyz);
		}
		else if (j->flags & TJFL_FADEIN)
		{
			verts[i].modulate[3] = 0;   // fade in
		}

		i++;

		if (trail->sType == STYPE_REPEAT)
		{
			s = jNext->sTex;
		}
		else
		{
			//s += sInc;
			s += VectorDistance(j->pos, jNext->pos) / sInc; // FIXME: div/0!?
			if (s > 1.0f)
			{
				s = 1.0f;
			}
		}

		// 3 ----
		VectorMA(jNext->pos, -0.5f * jNext->width, up, p);
		VectorCopy(p, verts[i].xyz);
		verts[i].st[0] = s;
		verts[i].st[1] = 0.0;
		for (k = 0; k < 3; k++)
		{
			verts[i].modulate[k] = ( unsigned char )(jNext->color[k] * 255.0f);
		}
		verts[i].modulate[3] = ( unsigned char )(jNext->alpha * 255.0f);
		i++;

		// 4 ----
		VectorMA(p, jNext->width, up, p);
		VectorCopy(p, verts[i].xyz);
		verts[i].st[0] = s;
		verts[i].st[1] = 1.0;
		for (k = 0; k < 3; k++)
		{
			verts[i].modulate[k] = ( unsigned char )(jNext->color[k] * 255.0f);
		}
		verts[i].modulate[3] = ( unsigned char )(jNext->alpha * 255.f);
		i++;

		if (i + 4 > MAX_TRAIL_VERTS)
		{
			break;
		}

		j     = jNext;
		jNext = j->nextJunc;
	}

	if (trail->flags & TJFL_FIXDISTORT)
	{
		// build the list of outVerts, by dividing up the QUAD's into 4 Tri's each, so as to allow
		//  any shaped (convex) Quad without bilinear distortion
		for (k = 0, numOutVerts = 0; k < i; k += 4)
		{
			VectorCopy(verts[k].xyz, mid.xyz);
			mid.st[0] = verts[k].st[0];
			mid.st[1] = verts[k].st[1];
			for (l = 0; l < 4; l++)
			{
				mod[l] = (float)verts[k].modulate[l];
			}
			for (n = 1; n < 4; n++)
			{
				VectorAdd(verts[k + n].xyz, mid.xyz, mid.xyz);
				mid.st[0] += verts[k + n].st[0];
				mid.st[1] += verts[k + n].st[1];
				for (l = 0; l < 4; l++)
				{
					mod[l] += (float)verts[k + n].modulate[l];
				}
			}
			VectorScale(mid.xyz, 0.25f, mid.xyz);
			mid.st[0] *= 0.25;
			mid.st[1] *= 0.25;
			for (l = 0; l < 4; l++)
			{
				mid.modulate[l] = ( unsigned char )(mod[l] / 4.0f);
			}

			// now output the tri's
			for (n = 0; n < 4; n++)
			{
				outVerts[numOutVerts++] = verts[k + n];
				outVerts[numOutVerts++] = mid;
				if (n < 3)
				{
					outVerts[numOutVerts++] = verts[k + n + 1];
				}
				else
				{
					outVerts[numOutVerts++] = verts[k];
				}
			}
		}

		if (!(trail->flags & TJFL_NOPOLYMERGE))
		{
			trap_R_AddPolysToScene(trail->shader, 3, &outVerts[0], numOutVerts / 3);
		}
		else
		{
			for (k = 0; k < numOutVerts / 3; k++)
			{
				trap_R_AddPolyToScene(trail->shader, 3, &outVerts[k * 3]);
			}
		}
	}
	else
	{
		// send the polygons
		// FIXME: is it possible to send a GL_STRIP here? We are actually sending 2x the verts we really need to
		if (!(trail->flags & TJFL_NOPOLYMERGE))
		{
			trap_R_AddPolysToScene(trail->shader, 4, &verts[0], i / 4);
		}
		else
		{
			for (k = 0; k < i / 4; k++)
			{
				trap_R_AddPolyToScene(trail->shader, 4, &verts[k * 4]);
			}
		}
	}

	// do we need to make another pass?
	if (trail->flags & TJFL_CROSSOVER)
	{
		if (iteration < 2)
		{
			CG_AddTrailToScene(trail, iteration + 1, numJuncs);
		}
	}
}

/**
 * @brief CG_AddTrails
 */
void CG_AddTrails(void)
{
	float       lifeFrac;
	trailJunc_t *j, *jNext;

	if (!initTrails)
	{
		CG_ClearTrails();
	}

	//AngleVectors( cg.snap->ps.viewangles, vforward, vright, vup );
	VectorCopy(cg.refdef_current->viewaxis[0], vforward);
	VectorCopy(cg.refdef_current->viewaxis[1], vright);
	VectorCopy(cg.refdef_current->viewaxis[2], vup);

	// update the settings for each junc
	j = activeTrails;
	while (j)
	{
		if (cgs.matchPaused)
		{
			j->spawnTime += cg.frametime;
			j->endTime   += cg.frametime;
		}

		lifeFrac = (float)(cg.time - j->spawnTime) / (float)(j->endTime - j->spawnTime);
		if (lifeFrac >= 1.0f)
		{
			j->inuse = qfalse;          // flag it as dead
			j->width = j->widthEnd;
			j->alpha = j->alphaEnd;
			if (j->alpha > 1.0f)
			{
				j->alpha = 1.0f;
			}
			else if (j->alpha < 0.0f)
			{
				j->alpha = 0.0f;
			}
			VectorCopy(j->colorEnd, j->color);
		}
		else
		{
			j->width = j->widthStart + (j->widthEnd - j->widthStart) * lifeFrac;
			j->alpha = j->alphaStart + (j->alphaEnd - j->alphaStart) * lifeFrac;
			if (j->alpha > 1.0f)
			{
				j->alpha = 1.0f;
			}
			else if (j->alpha < 0.0f)
			{
				j->alpha = 0.0;
			}
			VectorSubtract(j->colorEnd, j->colorStart, j->color);
			VectorMA(j->colorStart, lifeFrac, j->color, j->color);
		}

		j = j->nextGlobal;
	}

	// draw the trailHeads
	j = headTrails;
	while (j)
	{
		jNext = j->nextHead;        // in case it gets removed
		if (!j->inuse)
		{
			CG_FreeTrailJunc(j);
		}
		else
		{
			CG_AddTrailToScene(j, 0, 0);
		}
		j = jNext;
	}
}

/**
 * @brief CG_ClearTrails
 */
void CG_ClearTrails(void)
{
	int i;

	Com_Memset(trailJuncs, 0, sizeof(trailJunc_t) * MAX_TRAILJUNCS);

	freeTrails   = trailJuncs;
	activeTrails = NULL;
	headTrails   = NULL;

	for (i = 0 ; i < MAX_TRAILJUNCS ; i++)
	{
		if (i < (MAX_TRAILJUNCS - 1))
		{
			trailJuncs[i].nextGlobal = &trailJuncs[i + 1];
		}

		if (i > 0)
		{
			trailJuncs[i].prevGlobal = &trailJuncs[i - 1];
		}
		else
		{
			trailJuncs[i].prevGlobal = NULL;
		}

		trailJuncs[i].inuse = qfalse;
	}
	trailJuncs[MAX_TRAILJUNCS - 1].nextGlobal = NULL;

	initTrails     = qtrue;
	numTrailsInuse = 0;
}
