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
 * @file cg_atmospheric.c
 * @brief Add atmospheric effects (e.g. rain, snow etc.) to view.
 *
 * Current supported effects are rain and snow.
 */

#include "cg_local.h"

#define MAX_ATMOSPHERIC_HEIGHT          MAX_MAP_SIZE    // maximum world height
//#define MIN_ATMOSPHERIC_HEIGHT          -MAX_MAP_SIZE   // minimum world height

#define MAX_ATMOSPHERIC_PARTICLES       4000    // maximum # of particles
#define MAX_ATMOSPHERIC_DISTANCE        1000    // maximum distance from refdef origin that particles are visible
#define MAX_ATMOSPHERIC_EFFECTSHADERS   4       // maximum different effectshaders for an atmospheric effect

#define ATMOSPHERIC_RAIN_SPEED      (1.1f * DEFAULT_GRAVITY)
#define ATMOSPHERIC_RAIN_HEIGHT     150

#define ATMOSPHERIC_SNOW_SPEED      (0.1f * DEFAULT_GRAVITY)
#define ATMOSPHERIC_SNOW_HEIGHT     3

#define ATMOSPHERIC_PARTICLE_OFFSET 10

typedef enum
{
	ATM_NONE = 0,
	ATM_RAIN,
	ATM_SNOW
	//ATM_SANDSTORM
	//ATM_HAIL
} atmFXType_t;

/**
 * @brief Add polygon to pool
 * @details Helper function to add polygon to pool
 * @param[in] shader
 * @param[in] verts
 */
static void CG_AddPolyToPool(qhandle_t shader, const polyVert_t *verts)
{
	int          firstIndex;
	int          firstVertex;
	int          i;
	polyBuffer_t *pPolyBuffer;

	pPolyBuffer = CG_PB_FindFreePolyBuffer(shader, 3, 3);

	if (!pPolyBuffer)
	{
		return;
	}

	firstIndex  = pPolyBuffer->numIndicies;
	firstVertex = pPolyBuffer->numVerts;

	for (i = 0; i < 3; i++)
	{
		VectorCopy(verts[i].xyz, pPolyBuffer->xyz[firstVertex + i]);

		pPolyBuffer->st[firstVertex + i][0]    = verts[i].st[0];
		pPolyBuffer->st[firstVertex + i][1]    = verts[i].st[1];
		pPolyBuffer->color[firstVertex + i][0] = verts[i].modulate[0];
		pPolyBuffer->color[firstVertex + i][1] = verts[i].modulate[1];
		pPolyBuffer->color[firstVertex + i][2] = verts[i].modulate[2];
		pPolyBuffer->color[firstVertex + i][3] = verts[i].modulate[3];

		pPolyBuffer->indicies[firstIndex + i] = firstVertex + i;
	}

	pPolyBuffer->numIndicies += 3;
	pPolyBuffer->numVerts    += 3;
}

static qboolean kludgeChecked, kludgeResult;
/**
* @brief Activate atmospheric effects for kludge maps
* @details Activate rain for specified kludge maps that don't have it specified for them
*/
qboolean CG_AtmosphericKludge(void)
{
	// Activate rain for specified kludge maps that don't
	// have it specified for them.

	if (kludgeChecked)
	{
		return kludgeResult;
	}
	kludgeChecked = qtrue;
	kludgeResult  = qfalse;

/*
    switch (50)
    {
    case 10: // rain
        // easy rain
        CG_EffectParse("T=RAIN,B=5 10,C=0.5,G=0.5 2,BV=50 50,GV=200 200,W=1 2,D=1000");
        return (kludgeResult = qtrue);
    case 11:
        CG_EffectParse("T=RAIN,B=5 10,C=0.5 2,G=0.5 2,BV=30 100,GV=20 80,W=1 2,D=1000 1000");
        return (kludgeResult = qtrue);
    case 12:
        CG_EffectParse("T=RAIN,B=5 10,C=0.5 2,G=0.5 2,BV=30 100,GV=20 80,W=1 2,D=1000 1000");
        return (kludgeResult = qtrue);
    case 20: // snow

        //CG_EffectParse( "T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=50 50,GV=30 80,W=1 2,D=5000" );

        // snow storm, quite horizontally
        //CG_EffectParse( "T=SNOW,B=20 30,C=0.8,G=0.5 8,BV=100 100,GV=70 150,W=3 5,D=5000" );

        // mild snow storm, quite vertically - likely go for this
        CG_EffectParse("T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=20 30,GV=25 40,W=3 5,D=2000");

        // mild snow storm, quite vertically - likely go for this
       // CG_EffectParse("T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=20 30,GV=25 40,W=3 5,D=5000" );

        // cpu-cheap press event effect
        //CG_EffectParse( "T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=20 30,GV=25 40,W=3 5,D=500" );
        //CG_EffectParse( "T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=20 30,GV=25 40,W=3 5,D=750" );
        return (kludgeResult = qtrue);
    case 26:
        //CG_EffectParse( "T=SNOW,B=5 7,C=0.2,G=0.1 5,BV=15 25,GV=25 40,W=3 5,D=400" );
        CG_EffectParse( "T=SNOW,B=5 7,C=0.2,G=0.1 5,BV=15 25,GV=25 40,W=3 5,H=512,D=2000");
        return (kludgeResult = qtrue);
    case 27:
        CG_EffectParse("T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=20 30,GV=25 40,W=3 5,H=512,D=2000 4000");
        return (kludgeResult = qtrue);
    case 28:
        CG_EffectParse("T=SNOW,B=5 7,C=0.2,G=0.1 5,BV=15 25,GV=25 40,W=3 5,H=512,D=2000");
        return (kludgeResult = qtrue);
    case 29:
        CG_EffectParse("T=SNOW,B=5 10,C=0.5,G=0.3 2,BV=20 30,GV=25 40,W=3 5,H=608,D=2000");
        return (kludgeResult = qtrue);
    case 50:
        CG_EffectParse("T=SNOW,B=20 30,C=0.5,G=0.3 2,BV=20 100,GV=25 40,W=3 5,H=608,D=5000");
        return (kludgeResult = qtrue);
    default:
        break;
    }
*/
	return (kludgeResult = qfalse);
}

typedef enum
{
	ACT_NOT = 0,
	ACT_FALLING
} active_t;

typedef struct cg_atmosphericParticle_s
{
	vec3_t pos, delta, deltaNormalized, color;
	float height, weight;
	active_t active;
	qhandle_t *effectshader;
	atmFXType_t partFX;
} cg_atmosphericParticle_t;

typedef struct cg_atmosphericEffect_s
{
	cg_atmosphericParticle_t particles[MAX_ATMOSPHERIC_PARTICLES];
	qhandle_t effectshaders[MAX_ATMOSPHERIC_EFFECTSHADERS];
	int lastEffectTime, numDrops;
	int gustStartTime, gustEndTime;
	int baseStartTime, baseEndTime;
	int gustMinTime, gustMaxTime;
	int changeMinTime, changeMaxTime;
	int baseMinTime, baseMaxTime;
	float baseWeight, gustWeight;
	int baseDrops, gustDrops;
	int baseHeightOffset;
	vec3_t baseVec, gustVec;

	vec3_t viewDir;

	qboolean (*ParticleCheckVisible)(cg_atmosphericParticle_t *particle);
	qboolean (*ParticleGenerate)(cg_atmosphericParticle_t *particle, vec3_t currvec, float currweight, atmFXType_t atmFX);
	void (*ParticleRender)(cg_atmosphericParticle_t *particle);

	int dropsActive, oldDropsActive;
	int dropsCreated;

	atmFXType_t currentFX;

} cg_atmosphericEffect_t;

static cg_atmosphericEffect_t cg_atmFx;

/**
 * @brief Activate particule
 * @details Activate atmospheric particle
 * @param[out] particle
 * @param[in] active
 * @return
 */
static qboolean CG_SetParticleActive(cg_atmosphericParticle_t *particle, active_t active)
{
	particle->active = active;
	return active ? qtrue : qfalse;
}

/**
 * @brief Generate a particle
 * @details Attempt to 'spot' a drop somewhere below a sky texture.
 * @param[out] particle
 * @param[in] currvec
 * @param[in] currweight
 * @param[in] atmFX
 * @return
 */
static qboolean CG_ParticleGenerate(cg_atmosphericParticle_t *particle, vec3_t currvec, float currweight, atmFXType_t atmFX)
{
	float angle = random() * M_TAU_F;
	float distance = 20 + MAX_ATMOSPHERIC_DISTANCE * random();
	float groundHeight, skyHeight;

	particle->pos[0] = cg.refdef_current->vieworg[0] + sin(angle) * distance;
	particle->pos[1] = cg.refdef_current->vieworg[1] + cos(angle) * distance;

	// choose a spawn point randomly between sky and ground
	skyHeight = BG_GetSkyHeightAtPoint(particle->pos);
	if (skyHeight >= MAX_ATMOSPHERIC_HEIGHT)
	{
		return qfalse;
	}
	groundHeight = BG_GetSkyGroundHeightAtPoint(particle->pos);
	if (groundHeight + particle->height + ATMOSPHERIC_PARTICLE_OFFSET >= skyHeight)
	{
		return qfalse;
	}
	particle->pos[2] = groundHeight + random() * (skyHeight - groundHeight);

	// make sure it doesn't fall from too far cause it then will go over our heads ('lower the ceiling')
	if (cg_atmFx.baseHeightOffset > 0)
	{
		if (particle->pos[2] - cg.refdef_current->vieworg[2] > cg_atmFx.baseHeightOffset)
		{
			particle->pos[2] = cg.refdef_current->vieworg[2] + cg_atmFx.baseHeightOffset;

			if (particle->pos[2] < groundHeight)
			{
				return qfalse;
			}
		}
	}

	// rain goes in bursts - allow max raindrops every 10 seconds
	if (atmFX == ATM_RAIN && (cg_atmFx.oldDropsActive > (0.5f * cg_atmFx.numDrops + 0.001f * cg_atmFx.numDrops * (10000 - (cg.time % 10000)))))
	{
		return qfalse;
	}

	CG_SetParticleActive(particle, ACT_FALLING);

	VectorCopy(currvec, particle->delta);
	if (atmFX == ATM_RAIN)
	{
		particle->delta[2] += crandom() * 100;
	}
	else
	{
		particle->delta[2] += crandom() * 25;
	}

	VectorCopy(particle->delta, particle->deltaNormalized);
	VectorNormalizeFast(particle->deltaNormalized);

	if (atmFX == ATM_RAIN)
	{
		particle->height = ATMOSPHERIC_RAIN_HEIGHT + crandom() * 100;
		particle->weight = currweight;

		// special color
		particle->color[0] = 0.6f + 0.2f * random() * 0xFF;
		particle->color[1] = 0.6f + 0.2f * random() * 0xFF;
		particle->color[2] = 0.6f + 0.2f * random() * 0xFF;
	}
	else
	{
		particle->height = ATMOSPHERIC_SNOW_HEIGHT + random() * 2;
		particle->weight = particle->height * 0.5f;

		particle->color[0] = 255;
		particle->color[1] = 255;
		particle->color[2] = 255;
	}

	//particle->effectshader = &cg_atmFx.effectshaders[ (int) (random() * ( 2 )) + 1 ];
	particle->effectshader = &cg_atmFx.effectshaders[atmFX];

	particle->partFX = atmFX;

	return qtrue;
}

/**
 * @brief Check visibility of particle
 * @details Check the drop is visible and still going, wrapping if necessary.
 * @param[in] particle
 * @return
 */
static qboolean CG_ParticleCheckVisible(cg_atmosphericParticle_t *particle)
{
	float  moved;
	vec2_t distance;

	if (!particle || particle->active == ACT_NOT)
	{
		return qfalse;
	}

	// units moved since last frame
	moved = (cg.time - cg_atmFx.lastEffectTime) * 0.001f;
	VectorMA(particle->pos, moved, particle->delta, particle->pos);

	if ((particle->partFX == ATM_RAIN ? (particle->pos[2] + particle->height) : particle->pos[2]) < BG_GetSkyGroundHeightAtPoint(particle->pos))
	{
		return CG_SetParticleActive(particle, ACT_NOT);
	}

	distance[0] = particle->pos[0] - cg.refdef_current->vieworg[0];
	distance[1] = particle->pos[1] - cg.refdef_current->vieworg[1];

	if ((distance[0] * distance[0] + distance[1] * distance[1]) > Square(MAX_ATMOSPHERIC_DISTANCE))
	{
		// just nuke this particle, let it respawn
		return CG_SetParticleActive(particle, ACT_NOT);
	}

	return qtrue;
}

/*
** Raindrop management functions
*/

/**
 * @brief Draw a particle
 * @details Renders a particle.
 * @param[in] particle
 */
static void CG_ParticleRender(cg_atmosphericParticle_t *particle)
{
	vec3_t     forward, right;
	polyVert_t verts[3];
	vec2_t     line;
	float      len, sinTumbling, cosTumbling, particleWidth, dist = 0.0;
	vec3_t     start, finish;
	float      groundHeight;

	if (particle->active == ACT_NOT)
	{
		return;
	}

	if (CG_CullPoint(particle->pos))
	{
		return;
	}

	VectorCopy(particle->pos, start);

	if (particle->partFX == ATM_SNOW)
	{
		sinTumbling = sin(particle->pos[2] * 0.03125f * (0.5f * particle->weight));
		cosTumbling = cos((particle->pos[2] + particle->pos[1]) * 0.03125f * (0.5f * particle->weight));
		start[0]   += 24 * (1 - particle->deltaNormalized[2]) * sinTumbling;
		start[1]   += 24 * (1 - particle->deltaNormalized[2]) * cosTumbling;
	}
	else // ATM_RAIN
	{
		dist = DistanceSquared(particle->pos, cg.refdef_current->vieworg);
	}
	// make sure it doesn't clip through surfaces
	groundHeight = BG_GetSkyGroundHeightAtPoint(start);
	len          = particle->height;

	if (particle->partFX == ATM_SNOW)
	{
		if (start[2] - len - ATMOSPHERIC_PARTICLE_OFFSET <= groundHeight)
		{
			return;
			// stop snow going through surfaces
			//len = particle->height - groundHeight + start[2];
			//VectorMA(start, len - particle->height, particle->deltaNormalized, start);
		}
	}
	else // ATM_RAIN
	{
		if (start[2] - ATMOSPHERIC_PARTICLE_OFFSET <= groundHeight)
		{
			// stop rain going through surfaces
			len = particle->height - groundHeight + start[2];
			VectorMA(start, len - particle->height, particle->deltaNormalized, start);
		}
	}

	if (len <= 0)
	{
		return;
	}

	if (particle->partFX == ATM_SNOW)
	{
		line[0] = particle->pos[0] - cg.refdef_current->vieworg[0];
		line[1] = particle->pos[1] - cg.refdef_current->vieworg[1];
		dist    = DistanceSquared(particle->pos, cg.refdef_current->vieworg);

		// dist becomes scale
		if (dist > Square(500.f))
		{
			dist = 1.f + ((dist - Square(500.f)) * (10.f / Square(2000.f)));
		}
		else
		{
			dist = 1.f;
		}

		len *= dist;
	}
	else // ATM_RAIN
	{
		// fade nearby rain particles
		if (dist < Square(128.f))
		{
			dist = .25f + .75f * (dist / Square(128.f));
		}
		else
		{
			dist = 1.0f;
		}

	}

	VectorCopy(particle->deltaNormalized, forward);
	VectorMA(start, -len, forward, finish);

	line[0] = DotProduct(forward, cg.refdef_current->viewaxis[1]);
	line[1] = DotProduct(forward, cg.refdef_current->viewaxis[2]);

	VectorScale(cg.refdef_current->viewaxis[1], line[1], right);
	VectorMA(right, -line[0], cg.refdef_current->viewaxis[2], right);
	VectorNormalize(right);

	if (particle->partFX == ATM_SNOW)
	{
		particleWidth = dist * (particle->weight);

		VectorMA(finish, -particleWidth, right, verts[0].xyz);
		verts[0].st[0]       = 0;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = particle->color[0];
		verts[0].modulate[1] = particle->color[1];
		verts[0].modulate[2] = particle->color[2];
		verts[0].modulate[3] = 255;

		VectorMA(start, -particleWidth, right, verts[1].xyz);
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = particle->color[0];
		verts[1].modulate[1] = particle->color[1];
		verts[1].modulate[2] = particle->color[2];
		verts[1].modulate[3] = 255;

		VectorMA(start, particleWidth, right, verts[2].xyz);
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = particle->color[0];
		verts[2].modulate[1] = particle->color[1];
		verts[2].modulate[2] = particle->color[2];
		verts[2].modulate[3] = 255;
	}
	else // ATM_RAIN
	{
		VectorCopy(finish, verts[0].xyz);
		verts[0].st[0]       = 0.5f;
		verts[0].st[1]       = 0;
		verts[0].modulate[0] = particle->color[0];
		verts[0].modulate[1] = particle->color[1];
		verts[0].modulate[2] = particle->color[2];
		verts[0].modulate[3] = 100 * dist;

		VectorMA(start, -particle->weight, right, verts[1].xyz);
		verts[1].st[0]       = 0;
		verts[1].st[1]       = 1;
		verts[1].modulate[0] = particle->color[0];
		verts[1].modulate[1] = particle->color[1];
		verts[1].modulate[2] = particle->color[2];
		verts[1].modulate[3] = 200 * dist;

		VectorMA(start, particle->weight, right, verts[2].xyz);
		verts[2].st[0]       = 1;
		verts[2].st[1]       = 1;
		verts[2].modulate[0] = particle->color[0];
		verts[2].modulate[1] = particle->color[1];
		verts[2].modulate[2] = particle->color[2];
		verts[2].modulate[3] = 200 * dist;
	}
	CG_AddPolyToPool(*particle->effectshader, verts);
}

/*
**  Set up gust parameters.
*/

/**
 * @brief Gust effect
 * @details Generate random values for the next gust.
 */
static void CG_EffectGust(void)
{
	int diff;

	cg_atmFx.baseEndTime   = cg.time                   + cg_atmFx.baseMinTime      + (rand() % (cg_atmFx.baseMaxTime - cg_atmFx.baseMinTime));
	diff                   = cg_atmFx.changeMaxTime    - cg_atmFx.changeMinTime;
	cg_atmFx.gustStartTime = cg_atmFx.baseEndTime      + cg_atmFx.changeMinTime    + (diff ? (rand() % diff) : 0);
	diff                   = cg_atmFx.gustMaxTime      - cg_atmFx.gustMinTime;
	cg_atmFx.gustEndTime   = cg_atmFx.gustStartTime    + cg_atmFx.gustMinTime      + (diff ? (rand() % diff) : 0);
	diff                   = cg_atmFx.changeMaxTime    - cg_atmFx.changeMinTime;
	cg_atmFx.baseStartTime = cg_atmFx.gustEndTime      + cg_atmFx.changeMinTime    + (diff ? (rand() % diff) : 0);
}

/**
 * @brief Gust current effect
 * @details Calculate direction for new drops.
 * @param[out] curr
 * @param[out] weight
 * @param[out] num
 * @return
 */
static qboolean CG_EffectGustCurrent(vec3_t curr, float *weight, int *num)
{
	vec3_t temp;

	if (cg.time < cg_atmFx.baseEndTime)
	{
		VectorCopy(cg_atmFx.baseVec, curr);
		*weight = cg_atmFx.baseWeight;
		*num    = cg_atmFx.baseDrops;
	}
	else
	{
		float frac;

		VectorSubtract(cg_atmFx.gustVec, cg_atmFx.baseVec, temp);

		if (cg.time < cg_atmFx.gustStartTime)
		{
			frac = ((float)(cg.time - cg_atmFx.baseEndTime)) / ((float)(cg_atmFx.gustStartTime - cg_atmFx.baseEndTime));
			VectorMA(cg_atmFx.baseVec, frac, temp, curr);
			*weight = cg_atmFx.baseWeight + (cg_atmFx.gustWeight - cg_atmFx.baseWeight) * frac;
			*num    = cg_atmFx.baseDrops + ((float)(cg_atmFx.gustDrops - cg_atmFx.baseDrops)) * frac;
		}
		else if (cg.time < cg_atmFx.gustEndTime)
		{
			VectorCopy(cg_atmFx.gustVec, curr);
			*weight = cg_atmFx.gustWeight;
			*num    = cg_atmFx.gustDrops;
		}
		else
		{
			frac = 1.0f - ((float)(cg.time - cg_atmFx.gustEndTime)) / ((float)(cg_atmFx.baseStartTime - cg_atmFx.gustEndTime));
			VectorMA(cg_atmFx.baseVec, frac, temp, curr);
			*weight = cg_atmFx.baseWeight + (cg_atmFx.gustWeight - cg_atmFx.baseWeight) * frac;
			*num    = cg_atmFx.baseDrops + ((float)(cg_atmFx.gustDrops - cg_atmFx.baseDrops)) * frac;

			if (cg.time >= cg_atmFx.baseStartTime)
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

/**
 * @brief Parse floats
 * @details Parse the float or floats.
 * @param[in] floatstr
 * @param[out] f1
 * @param[out] f2
 */
static void CG_EP_ParseFloats(char *floatstr, float *f1, float *f2)
{
	char *middleptr;
	char buff[64];

	Q_strncpyz(buff, floatstr, sizeof(buff));

	for (middleptr = buff; *middleptr && *middleptr != ' '; middleptr++)
		;
	if (*middleptr)
	{
		*middleptr++ = 0;
		*f1          = atof(floatstr);
		*f2          = atof(middleptr);
	}
	else
	{
		*f1 = *f2 = atof(floatstr);
	}
}

/**
 * @brief Parse ints
 * @details Parse the int or ints.
 * @param[in] intstr
 * @param[out] i1
 * @param[out] i2
 */
static void CG_EP_ParseInts(char *intstr, int *i1, int *i2)
{
	char *middleptr;
	char buff[64];

	Q_strncpyz(buff, intstr, sizeof(buff));
	for (middleptr = buff; *middleptr && *middleptr != ' '; middleptr++)
		;
	if (*middleptr)
	{
		*middleptr++ = 0;
		*i1          = atof(intstr);
		*i2          = atof(middleptr);
	}
	else
	{
		*i1 = *i2 = atof(intstr);
	}
}

/**
 * @brief Parse effect
 * @details Split the string into it's component parts.
 * @param[in] effectstr
 */
void CG_EffectParse(const char *effectstr)
{
	float       bmin, bmax, cmin, cmax, gmin, gmax, bdrop, gdrop;
	int         bheight;
	char        *startptr, *eqptr, *endptr;
	char        workbuff[128];
	atmFXType_t atmFXType = ATM_NONE;

	if (CG_AtmosphericKludge())
	{
		return;
	}

	// set up some default values
	cg_atmFx.baseVec[0] = cg_atmFx.baseVec[1] = 0;
	cg_atmFx.gustVec[0] = cg_atmFx.gustVec[1] = 100;
	bmin                = 5;
	bmax                = 10;
	cmin                = 1;
	cmax                = 1;
	gmin                = 0;
	gmax                = 2;
	bdrop               = gdrop = 300;
	cg_atmFx.baseWeight = 0.7f;
	cg_atmFx.gustWeight = 1.5f;
	bheight             = 0;

	// parse the parameter string
	Q_strncpyz(workbuff, effectstr, sizeof(workbuff));

	for (startptr = workbuff; *startptr; )
	{
		for (eqptr = startptr; *eqptr && *eqptr != '=' && *eqptr != ','; eqptr++)
			;
		if (!*eqptr)
		{
			break;          // No more string
		}
		if (*eqptr == ',')
		{
			startptr = eqptr + 1;   // Bad argument, continue
			continue;
		}

		*eqptr++ = 0;

		for (endptr = eqptr; *endptr && *endptr != ','; endptr++)
			;
		if (*endptr)
		{
			*endptr++ = 0;
		}

		if (atmFXType == ATM_NONE) // first roll
		{
			if (Q_stricmp(startptr, "T"))
			{
				cg_atmFx.numDrops  = 0;
				cg_atmFx.currentFX = ATM_NONE;
				CG_Printf("Atmospheric effect must start with a type.\n");
				return;
			}

			if (!Q_stricmp(eqptr, "RAIN"))
			{
				atmFXType                     = ATM_RAIN;
				cg_atmFx.currentFX            = ATM_RAIN;
				cg_atmFx.ParticleCheckVisible = &CG_ParticleCheckVisible;
				cg_atmFx.ParticleGenerate     = &CG_ParticleGenerate;
				cg_atmFx.ParticleRender       = &CG_ParticleRender;

				cg_atmFx.baseVec[2] = cg_atmFx.gustVec[2] = -ATMOSPHERIC_RAIN_SPEED;
			}
			else if (!Q_stricmp(eqptr, "SNOW"))
			{
				atmFXType                     = ATM_SNOW;
				cg_atmFx.currentFX            = ATM_SNOW;
				cg_atmFx.ParticleCheckVisible = &CG_ParticleCheckVisible;
				cg_atmFx.ParticleGenerate     = &CG_ParticleGenerate;
				cg_atmFx.ParticleRender       = &CG_ParticleRender;

				cg_atmFx.baseVec[2] = cg_atmFx.gustVec[2] = -ATMOSPHERIC_SNOW_SPEED;
			}
			else
			{
				cg_atmFx.numDrops  = 0;
				cg_atmFx.currentFX = ATM_NONE;
				CG_Printf("Only effect type 'rain' and 'snow' are supported.\n");
				return;
			}
		}
		else
		{
			if (!Q_stricmp(startptr, "B"))
			{
				CG_EP_ParseFloats(eqptr, &bmin, &bmax);
			}
			else if (!Q_stricmp(startptr, "C"))
			{
				CG_EP_ParseFloats(eqptr, &cmin, &cmax);
			}
			else if (!Q_stricmp(startptr, "G"))
			{
				CG_EP_ParseFloats(eqptr, &gmin, &gmax);
			}
			else if (!Q_stricmp(startptr, "BV"))
			{
				CG_EP_ParseFloats(eqptr, &cg_atmFx.baseVec[0], &cg_atmFx.baseVec[1]);
			}
			else if (!Q_stricmp(startptr, "GV"))
			{
				CG_EP_ParseFloats(eqptr, &cg_atmFx.gustVec[0], &cg_atmFx.gustVec[1]);
			}
			else if (!Q_stricmp(startptr, "W"))
			{
				CG_EP_ParseFloats(eqptr, &cg_atmFx.baseWeight, &cg_atmFx.gustWeight);
			}
			else if (!Q_stricmp(startptr, "D"))
			{
				CG_EP_ParseFloats(eqptr, &bdrop, &gdrop);
			}
			else if (!Q_stricmp(startptr, "H"))
			{
				CG_EP_ParseInts(eqptr, &bheight, &bheight);
			}
			else
			{
				CG_Printf("Unknown effect key '%s'.\n", startptr);
			}
		}
		startptr = endptr;
	}

	if (atmFXType == ATM_NONE || !BG_LoadTraceMap(cgs.rawmapname, cg.mapcoordsMins, cg.mapcoordsMaxs))
	{
		// no effects
		cg_atmFx.numDrops  = 0;
		cg_atmFx.currentFX = ATM_NONE;
		return;
	}

	cg_atmFx.baseHeightOffset = bheight;

	if (cg_atmFx.baseHeightOffset < 0)
	{
		cg_atmFx.baseHeightOffset = 0;
	}

	cg_atmFx.baseMinTime   = 1000 * bmin;
	cg_atmFx.baseMaxTime   = 1000 * bmax;
	cg_atmFx.changeMinTime = 1000 * cmin;
	cg_atmFx.changeMaxTime = 1000 * cmax;
	cg_atmFx.gustMinTime   = 1000 * gmin;
	cg_atmFx.gustMaxTime   = 1000 * gmax;
	cg_atmFx.baseDrops     = bdrop;
	cg_atmFx.gustDrops     = gdrop;

	cg_atmFx.numDrops = (cg_atmFx.baseDrops > cg_atmFx.gustDrops) ? cg_atmFx.baseDrops : cg_atmFx.gustDrops;

	if (cg_atmFx.numDrops > MAX_ATMOSPHERIC_PARTICLES)
	{
		cg_atmFx.numDrops = MAX_ATMOSPHERIC_PARTICLES;
	}

	cg_atmFx.effectshaders[ATM_NONE] = 0;
	cg_atmFx.effectshaders[ATM_RAIN] = trap_R_RegisterShader("gfx/misc/raindrop");
	cg_atmFx.effectshaders[ATM_SNOW] = trap_R_RegisterShader("gfx/misc/snow");

	CG_EffectGust();
}

/*
** Main render loop
*/

/**
 * @brief Add atmospheric effects
 * @details Add atmospheric effects (e.g. rain, snow etc.) to view]
 */
void CG_AddAtmosphericEffects()
{
	int                      curr, max, currnum;
	cg_atmosphericParticle_t *particle;
	vec3_t                   currvec;
	float                    currweight;

	if (cg_atmFx.currentFX == ATM_NONE || cg_atmosphericEffects.value <= 0)
	{
		return;
	}

	max = cg_atmosphericEffects.value < 1 ? cg_atmosphericEffects.value * cg_atmFx.numDrops : cg_atmFx.numDrops;

	if (CG_EffectGustCurrent(currvec, &currweight, &currnum))
	{
		// recalculate gust parameters
		CG_EffectGust();
	}

	// allow parametric management of drop count for swelling/waning precip
	cg_atmFx.oldDropsActive = cg_atmFx.dropsActive;
	cg_atmFx.dropsActive    = 0;
	cg_atmFx.dropsCreated   = 0;

	VectorSet(cg_atmFx.viewDir, cg.refdef_current->viewaxis[0][0], cg.refdef_current->viewaxis[0][1], 0.f);

	for (curr = 0; curr < max; curr++)
	{
		particle = &cg_atmFx.particles[curr];

		if (!cg_atmFx.ParticleCheckVisible(particle))
		{
			// effect has terminated or fallen from screen view
			if (!cg_atmFx.ParticleGenerate(particle, currvec, currweight, cg_atmFx.currentFX))
			{
				continue;
			}
			else
			{
				cg_atmFx.dropsCreated++;
			}
		}

		cg_atmFx.ParticleRender(particle);
		cg_atmFx.dropsActive++;
	}

	cg_atmFx.lastEffectTime = cg.time;
}
