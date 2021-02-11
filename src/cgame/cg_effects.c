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
 * @file cg_effects.c
 * @brief These functions generate localentities, usually as a result of event
 *        processing
 */

#include "cg_local.h"

/**
 * @brief Bullets shot underwater
 * @param[in] start
 * @param[in] end
 * @param[in] size
 * @param[in] spacing
 */
void CG_BubbleTrail(vec3_t start, vec3_t end, float size, float spacing)
{
	vec3_t        move;
	vec3_t        vec;
	float         len;
	int           i = rand() % (int)spacing;
	localEntity_t *le;
	refEntity_t   *re;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	// advance a random amount first
	VectorMA(move, i, vec, move);

	VectorScale(vec, spacing, vec);

	for ( ; i < len; i += spacing)
	{
		le            = CG_AllocLocalEntity();
		le->leFlags   = LEF_PUFF_DONT_SCALE;
		le->leType    = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime   = cg.time + 1000 + random() * 250;
		le->lifeRate  = 1.0f / (le->endTime - le->startTime);

		re             = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType        = RT_SPRITE;
		re->rotation      = 0;
		re->radius        = size;
		re->customShader  = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy(move, le->pos.trBase);
		le->pos.trDelta[0] = crandom() * 3;
		le->pos.trDelta[1] = crandom() * 3;
		le->pos.trDelta[2] = crandom() * 5 + 20;

		VectorAdd(move, vec, move);
	}
}

/**
 * @brief Adds a smoke puff or blood trail localEntity.
 * @param[in] p
 * @param[in] vel
 * @param[in] radius
 * @param[in] r
 * @param[in] g
 * @param[in] b
 * @param[in] a
 * @param[in] duration
 * @param[in] startTime
 * @param[in] fadeInTime
 * @param[in] leFlags
 * @param[in] hShader
 * @return
 * @todo It would be nice to have an acceleration vector for this as well.
 * Big velocity vector with a negative acceleration for deceleration, etc.
 * (breath could then come out of a guys mouth at the rate he's walking/running and it
 * would slow down once it's created)
 */
localEntity_t *CG_SmokePuff(const vec3_t p, const vec3_t vel,
                            float radius,
                            float r, float g, float b, float a,
                            float duration,
                            int startTime,
                            int fadeInTime,
                            int leFlags,
                            qhandle_t hShader)
{
	static int    seed = 0x92;
	localEntity_t *le;
	refEntity_t   *re;

	le          = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius  = radius;

	re             = &le->refEntity;
	re->rotation   = Q_random(&seed) * 360;
	re->radius     = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType     = LE_MOVE_SCALE_FADE;
	le->startTime  = startTime;
	le->endTime    = startTime + (int)duration;
	le->fadeInTime = fadeInTime;
	if (fadeInTime > startTime)
	{
		le->lifeRate = 1.0f / (le->endTime - le->fadeInTime);
	}
	else
	{
		le->lifeRate = 1.0f / (le->endTime - le->startTime);
	}

	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy(vel, le->pos.trDelta);
	VectorCopy(p, le->pos.trBase);

	VectorCopy(p, re->origin);
	re->customShader = hShader;

	re->shaderRGBA[0] = (byte)(le->color[0] * 0xff);
	re->shaderRGBA[1] = (byte)(le->color[1] * 0xff);
	re->shaderRGBA[2] = (byte)(le->color[2] * 0xff);
	re->shaderRGBA[3] = 0xff;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

/**
 * @brief CG_MakeExplosion
 * @param[in] origin
 * @param[in] dir
 * @param[in] hModel
 * @param[in] shader
 * @param[in] msec
 * @param[in] isSprite
 * @return
 */
localEntity_t *CG_MakeExplosion(vec3_t origin, vec3_t dir,
                                qhandle_t hModel, qhandle_t shader,
                                int msec, qboolean isSprite)
{
	localEntity_t *ex;
	vec3_t        newOrigin;

	if (msec <= 0)
	{
		CG_Error("CG_MakeExplosion: msec = %i\n", msec);
	}

	ex = CG_AllocLocalEntity();
	if (isSprite)
	{
		vec3_t tmpVec;

		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale(dir, 16, tmpVec);
		VectorAdd(tmpVec, origin, newOrigin);
	}
	else
	{
		ex->leType = LE_EXPLOSION;
		VectorCopy(origin, newOrigin);

		// set axis with random rotate
		if (!dir)
		{
			AxisClear(ex->refEntity.axis);
		}
		else
		{
			VectorCopy(dir, ex->refEntity.axis[0]);
			RotateAroundDirection(ex->refEntity.axis, (rand() % 360));
		}
	}

	ex->startTime = cg.time - (rand() & 63); // skew the time a bit so they aren't all in sync
	ex->endTime   = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel       = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy(newOrigin, ex->refEntity.origin);
	VectorCopy(newOrigin, ex->refEntity.oldorigin);

	// move away from the wall as the sprite expands
	ex->pos.trType = TR_LINEAR;
	ex->pos.trTime = cg.time;
	VectorCopy(newOrigin, ex->pos.trBase);

	if (dir)
	{
		VectorScale(dir, 48, ex->pos.trDelta);
	}

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0f;

	return ex;
}

/**
 * @brief CG_AddBloodTrails
 * @param[in] origin
 * @param[in] dir
 * @param[in] speed
 * @param[in] duration
 * @param[in] count
 * @param[in] randScale
 */
void CG_AddBloodTrails(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity;
	int           i;

	for (i = 0; i < count; i++)
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet(velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale);
		VectorScale(velocity, (float)speed, velocity);

		le->leType        = LE_BLOOD;
		le->startTime     = cg.time;
		le->endTime       = le->startTime + duration; // (removed) - (int)(0.5 * random() * duration);
		le->lastTrailTime = cg.time;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy(origin, le->pos.trBase);
		VectorMA(le->pos.trBase, 2 + random() * 4, dir, le->pos.trBase);
		VectorCopy(velocity, le->pos.trDelta);
		le->pos.trTime = cg.time;

		le->bounceFactor = 0.9f;
	}
}

#define BLOOD_SPURT_COUNT   4
/**
 * @brief This is the spurt of blood when a character gets hit
 * @param[in] origin
 * @param[in] entityNum
 */
void CG_Bleed(vec3_t origin, int entityNum)
{
	if (!cg_blood.integer)
	{
		return;
	}

	// blood spurts
	if (entityNum != cg.snap->ps.clientNum)
	{
		vec3_t vhead, vbody, bOrigin, dir, vec, pvec, ndir;
		int    i, j;

		CG_GetBleedOrigin(vhead, vbody, entityNum);

		// project the impact point onto the vector defined by torso -> head
		ProjectPointOntoVector(origin, vbody, vhead, bOrigin);

		// if it's below the waste, or above the head, clamp
		VectorSubtract(vhead, vbody, vec);
		VectorSubtract(bOrigin, vbody, pvec);
		if (DotProduct(pvec, vec) < 0)
		{
			VectorCopy(vbody, bOrigin);
		}
		else
		{
			VectorSubtract(bOrigin, vhead, pvec);
			if (DotProduct(pvec, vec) > 0)
			{
				VectorCopy(vhead, bOrigin);
			}
		}

		// spawn some blood trails, heading out towards the impact point
		VectorSubtract(origin, bOrigin, dir);
		VectorNormalize(dir);

		{
			float  len;
			vec3_t vec;

			VectorSubtract(bOrigin, vhead, vec);
			len = VectorLength(vec);

			if (len > 8)
			{
				VectorMA(bOrigin, 8, dir, bOrigin);
			}
		}

		for (i = 0; i < BLOOD_SPURT_COUNT; i++)
		{
			VectorCopy(dir, ndir);
			for (j = 0; j < 3; j++)
			{
				ndir[j] += crandom() * 0.3f;
			}
			VectorNormalize(ndir);
			CG_AddBloodTrails(bOrigin, ndir,
			                  100,  // speed
			                  450 + (int)(crandom() * 50),       // duration
			                  2 + rand() % 2,     // count
			                  0.1f);     // rand scale
		}
	}
}

/**
 * @brief CG_LaunchGib
 * @param[in] cent
 * @param[in] origin
 * @param[in] angles
 * @param[in] velocity
 * @param[in] hModel
 * @param[in] sizeScale
 * @param[in] breakCount
 */
void CG_LaunchGib(centity_t *cent, vec3_t origin, vec3_t angles, vec3_t velocity, qhandle_t hModel, float sizeScale, int breakCount)
{
	localEntity_t *le;
	refEntity_t   *re;

	if (!cg_blood.integer || !cg_gibs.integer)
	{
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType     = LE_FRAGMENT;
	le->startTime  = cg.time;
	le->endTime    = le->startTime + 20000 + (int)(crandom() * 5000);
	le->breakCount = breakCount;
	le->sizeScale  = sizeScale;

	VectorCopy(angles, le->angles.trBase);
	VectorCopy(origin, re->origin);
	AnglesToAxis(angles, re->axis);
	if (sizeScale != 1.f)
	{
		int i;

		for (i = 0; i < 3; i++)
		{
			VectorScale(re->axis[i], sizeScale, re->axis[i]);
		}
	}
	re->hModel = hModel;

	re->fadeStartTime = le->endTime - 1000;
	re->fadeEndTime   = le->endTime;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType        = LEMT_BLOOD;
	le->pos.trType        = TR_GRAVITY;

	le->angles.trDelta[0] = (10 + (rand() & 50)) - 30;
	//le->angles.trDelta[0] = (100 + (rand()&500)) - 300;	// pitch
	le->angles.trDelta[1] = (100 + (rand() & 500)) - 300;   // this is the safe one right now (yaw)  turn the others up when I have tumbling things landing properly
	le->angles.trDelta[2] = (10 + (rand() & 50)) - 30;
	//le->angles.trDelta[2] = (100 + (rand()&500)) - 300;	// roll

	le->bounceFactor = 0.3f;

	VectorCopy(origin, le->pos.trBase);
	VectorCopy(velocity, le->pos.trDelta);
	le->pos.trTime = cg.time;


	le->angles.trType = TR_LINEAR;

	le->angles.trTime = cg.time;

	le->ownerNum = cent->currentState.number;

	// if the player is on fire, then spawn some flaming gibs
	if (CG_EntOnFire(cent))
	{
		le->onFireStart = cent->currentState.onFireStart;
		le->onFireEnd   = re->fadeEndTime + 1000;
	}
}

#define GIB_VELOCITY    75
#define GIB_JUMP        250

/**
 * @brief CG_LoseHat
 * @param[in] cent
 * @param[in] dir
 */
void CG_LoseHat(centity_t *cent, vec3_t dir)
{
	clientInfo_t   *ci;
	int            clientNum = cent->currentState.clientNum;
	vec3_t         origin = { 0 }, velocity = { 0 };
	bg_character_t *character;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		CG_Error("Bad clientNum on player entity\n");
	}
	ci        = &cgs.clientinfo[clientNum];
	character = CG_CharacterForClientinfo(ci, cent);

	// don't launch anything if they don't have one
	if (!character->accModels[ACC_HAT])
	{
		return;
	}

	CG_GetOriginForTag(cent, &cent->pe.headRefEnt, "tag_mouth", 0, origin, NULL);

	velocity[0] = dir[0] * (0.75f + random()) * GIB_VELOCITY;
	velocity[1] = dir[1] * (0.75f + random()) * GIB_VELOCITY;
	velocity[2] = GIB_JUMP - 50 + dir[2] * (0.5f + random()) * GIB_VELOCITY;

	{
		localEntity_t *le;
		refEntity_t   *re;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType    = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime   = (int)(le->startTime + 20000 + (crandom() * 5000));

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);
		re->hModel     = character->accModels[ACC_HAT];
		re->customSkin = character->accSkins[ACC_HAT];

		re->fadeStartTime = le->endTime - 1000;
		re->fadeEndTime   = le->endTime;

		// FIXME: origin of hat md3 is offset from center.  need to center the origin when you toss it
		le->pos.trType = TR_GRAVITY;
		VectorCopy(origin, le->pos.trBase);
		VectorCopy(velocity, le->pos.trDelta);
		le->pos.trTime = cg.time;

		// spin it a bit
		le->angles.trType = TR_LINEAR;
		VectorCopy(tv(0, 0, 0), le->angles.trBase);
		le->angles.trDelta[0] = 0;
		le->angles.trDelta[1] = (100 + (rand() & 500)) - 300;
		le->angles.trDelta[2] = 400;    // this is set with a very particular value to try to get it
		                                // to flip exactly once before landing (based on player alive
		                                // (standing) and on level ground) and will be unnecessary when
		                                // I have things landing properly on their own

		le->angles.trTime = cg.time;

		le->bounceFactor = 0.2f;

		// if the player is on fire, then make the hat on fire
		if (CG_EntOnFire(cent))
		{
			le->onFireStart = cent->currentState.onFireStart;
			le->onFireEnd   = cent->currentState.onFireEnd + 4000;
		}
	}
}

/**
 * @brief Places the position of the tag into "org"
 * @param cent - unused
 * @param[in] parent
 * @param[in] tagName
 * @param[in] startIndex
 * @param[in,out] org
 * @param[in] axis
 * @return The index of the tag it used, so we can cycle through tag's with the same name
 */
int CG_GetOriginForTag(centity_t *cent, refEntity_t *parent, const char *tagName, int startIndex, vec3_t org, vec3_t axis[3])
{
	int           i;
	orientation_t lerped;
	int           retval;

	// lerp the tag
	retval = trap_R_LerpTag(&lerped, parent, tagName, startIndex);

	if (retval < 0)
	{
		return retval;
	}

	VectorCopy(parent->origin, org);

	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(org, lerped.origin[i], parent->axis[i], org);
	}

	if (axis)
	{
		MatrixMultiply(lerped.axis, parent->axis, axis);
	}

	return retval;
}

#define MAXJUNCTIONS 8
#define GIB_BLOOD_DOTS  3

/**
 * @brief Generated a bunch of gibs launching out from the bodies location
 * @param[in,out] cent
 * @param[in] playerOrigin
 * @param[in] gdir
 */
void CG_GibPlayer(centity_t *cent, vec3_t playerOrigin, vec3_t gdir)
{
	if (cg_blood.integer && cg_bloodTime.integer)
	{
		vec3_t         origin;
		trace_t        trace;
		clientInfo_t   *ci;
		bg_character_t *character;
		vec4_t         projection;
		// BloodCloud
		qboolean newjunction[MAXJUNCTIONS]    = { 0 };
		vec3_t   junctionOrigin[MAXJUNCTIONS] = { 0 };
		vec3_t   axis[3];

		vec4_t      color;
		vec3_t      velocity, dir, angles;
		refEntity_t *re = &cent->pe.bodyRefEnt;
		int         i, j, count = 0;
		int         tagIndex, gibIndex, junction;
		int         clientNum = cent->currentState.clientNum;

		static const char *JunctiongibTags[] =
		{
			// leg tag
			"tag_footright",
			"tag_footleft",
			"tag_legright",
			"tag_legleft",

			// torsotags
			"tag_armright",
			"tag_armleft",

			"tag_torso",
			"tag_chest"
		};

		static const char *ConnectTags[] =
		{
			// legs tags
			"tag_legright",
			"tag_legleft",
			"tag_torso",
			"tag_torso",

			// torso tags
			"tag_chest",
			"tag_chest",

			"tag_chest",
			"tag_torso",
		};

		static const char *gibTags[] =
		{
			// tags in the legs
			"tag_footright",
			"tag_footleft",
			"tag_legright",
			"tag_legleft",
			"tag_torso",

			// tags in the torso
			"tag_chest",
			"tag_armright",
			"tag_armleft",
			"tag_head",
			NULL
		};

		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		{
			CG_Error("Bad clientNum on player entity\n");
		}

		ci        = &cgs.clientinfo[clientNum];
		character = CG_CharacterForClientinfo(ci, cent);

		// fetch the various positions of the tag_gib*'s
		// and spawn the gibs from the correct places (especially the head)
		for (gibIndex = 0, count = 0; gibIndex < MAX_GIB_MODELS && gibTags[gibIndex]; gibIndex++)
		{
			if (!character->gibModels[gibIndex])
			{
				break;
			}

			for (tagIndex = 0; (tagIndex = CG_GetOriginForTag(cent, re, gibTags[gibIndex], tagIndex, origin, axis)) >= 0; count++, tagIndex++)
			{
				VectorSubtract(origin, re->origin, dir);
				VectorNormalize(dir);

				// spawn a gib
				velocity[0] = dir[0] * (0.5f + random()) * GIB_VELOCITY * 0.3f;
				velocity[1] = dir[1] * (0.5f + random()) * GIB_VELOCITY * 0.3f;
				velocity[2] = GIB_JUMP + dir[2] * (0.5f + random()) * GIB_VELOCITY * 0.5f;

				VectorMA(velocity, GIB_VELOCITY, gdir, velocity);
				AxisToAngles(axis, angles);

				CG_LaunchGib(cent, origin, angles, velocity, character->gibModels[gibIndex], 1.0, 0);

				for (junction = 0; junction < MAXJUNCTIONS; junction++)
				{
					if (!Q_stricmp(gibTags[gibIndex], JunctiongibTags[junction]))
					{
						VectorCopy(origin, junctionOrigin[junction]);
						newjunction[junction] = qtrue;
					}
				}
			}
		}

		for (i = 0; i < MAXJUNCTIONS; i++)
		{
			if (newjunction[i] == qtrue)
			{
				for (j = 0; j < MAXJUNCTIONS; j++)
				{
					if (!Q_stricmp(JunctiongibTags[j], ConnectTags[i]))
					{
						if (newjunction[j] == qtrue)
						{
							// spawn a blood cloud somewhere on the vec from
							VectorSubtract(junctionOrigin[i], junctionOrigin[j], dir);
							CG_ParticleBloodCloud(cent, junctionOrigin[i], dir);
						}
					}
				}
			}
		}

		// spawn a bunch of blood dots around the place
		for (i = 0, count = 0; i < GIB_BLOOD_DOTS * 2; i++)
		{
			if (i > 0)
			{
				velocity[0] = ((i % 2) * 2 - 1) * (40 + 40 * random());
				velocity[1] = (((i / 2) % 2) * 2 - 1) * (40 + 40 * random());
				velocity[2] = (((i < GIB_BLOOD_DOTS) * 2) - 1) * 40;
			}
			else
			{
				VectorClear(velocity);
				velocity[2] = -64;
			}

			VectorAdd(playerOrigin, velocity, origin);

			CG_Trace(&trace, playerOrigin, NULL, NULL, origin, -1, CONTENTS_SOLID);
			if (trace.fraction < 1.0f)
			{
				//BG_GetMarkDir( velocity, trace.plane.normal, velocity );
				//CG_ImpactMark( cgs.media.bloodDotShaders[rand()%5], trace.endpos, velocity, random()*360,
				//	1,1,1,1, qtrue, 30, qfalse, cg_bloodTime.integer * 1000 );
				#if 0
				BG_GetMarkDir(velocity, trace.plane.normal, projection);
				VectorSubtract(vec3_origin, projection, projection);
				projection[3] = 64;
				VectorMA(trace.endpos, -8.0f, projection, markOrigin);
				CG_ImpactMark(cgs.media.bloodDotShaders[rand() % 5], markOrigin, projection, 30.0f, random() * 360.0f, 1.0f, 1.0f, 1.0f, 1.0f, cg_bloodTime.integer * 1000);
				#else
				VectorSet(projection, 0, 0, -1);
				projection[3] = 30.0f;
				Vector4Set(color, 1.0f, 1.0f, 1.0f, 1.0f);
				trap_R_ProjectDecal(cgs.media.bloodDotShaders[rand() % 5], 1, (vec3_t *) trace.endpos, projection, color,
				                    cg_bloodTime.integer * 1000, (cg_bloodTime.integer * 1000) >> 4);
				#endif

				if (count++ > GIB_BLOOD_DOTS)
				{
					break;
				}
			}
		}
	}

	if (!(cent->currentState.eFlags & EF_HEADSHOT))       // already lost hat while living
	{
		CG_LoseHat(cent, tv(0, 0, 1));
	}
}

/**
 * @brief CG_SparklerSparks
 * @param[in] origin
 * @param[in] count
 *
 * @note Unused
 */
void CG_SparklerSparks(vec3_t origin, int count)
{
	// these effect the look of the, umm, effect
	int FUSE_SPARK_LIFE   = 100;
	int FUSE_SPARK_LENGTH = 30;
	// these are calculated from the above
	int           FUSE_SPARK_SPEED = (FUSE_SPARK_LENGTH * 1000 / FUSE_SPARK_LIFE);
	int           i;
	localEntity_t *le;
	refEntity_t   *re;

	for (i = 0; i < count; i++)
	{
		// spawn the spark
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType        = LE_FUSE_SPARK;
		le->startTime     = cg.time;
		le->endTime       = cg.time + FUSE_SPARK_LIFE;
		le->lastTrailTime = cg.time;

		VectorCopy(origin, re->origin);

		le->pos.trType = TR_GRAVITY;
		VectorCopy(origin, le->pos.trBase);
		VectorSet(le->pos.trDelta, crandom(), crandom(), crandom());
		VectorNormalize(le->pos.trDelta);
		VectorScale(le->pos.trDelta, FUSE_SPARK_SPEED, le->pos.trDelta);
		le->pos.trTime = cg.time;
	}
}

/**
 * @brief CG_RumbleEfx
 * @param[in] pitch
 * @param[in] yaw
 */
void CG_RumbleEfx(float pitch, float yaw)
{
	float  pitchRecoilAdd = 0, pitchAdd = 0;
	float  yawRandom = 0;
	vec3_t recoil;

	if (pitch < 1)
	{
		pitch = 1;
	}

	pitchRecoilAdd = pow(random(), 8) * (10 + VectorLength(cg.snap->ps.velocity) / 5);
	pitchAdd       = (rand() % (int)pitch) - (pitch * 0.5f); //5
	yawRandom      = yaw; //2

	pitchRecoilAdd *= 0.5;
	pitchAdd       *= 0.5;
	yawRandom      *= 0.5;

	// calc the recoil

	// the following used to be "recoil[YAW] = crandom()*yawRandom()"
	// but that seems to skew the effect either to the left or to the right for long streches
	// of time. The idea here is to keep it skewed for short period of time and then switches
	// to the other direction - switch the sign of recoil[YAW] when random() < 0.05 and keep the
	// sign otherwise. This seems better at balancing out the effect.
	if (cg.kickAVel[YAW] > 0)
	{
		if (random() < 0.05f)
		{
			recoil[YAW] = -random() * yawRandom;
		}
		else
		{
			recoil[YAW] = random() * yawRandom;
		}
	}
	else if (cg.kickAVel[YAW] < 0)
	{
		if (random() < 0.05f)
		{
			recoil[YAW] = random() * yawRandom;
		}
		else
		{
			recoil[YAW] = -random() * yawRandom;
		}
	}
	else
	{
		if (random() < 0.5f)
		{
			recoil[YAW] = random() * yawRandom;
		}
		else
		{
			recoil[YAW] = -random() * yawRandom;
		}
	}

	recoil[ROLL]  = -recoil[YAW];   // why not
	recoil[PITCH] = -pitchAdd;
	// scale it up a bit (easier to modify this while tweaking)
	VectorScale(recoil, 30, recoil);
	// set the recoil
	VectorCopy(recoil, cg.kickAVel);
	// set the recoil
	cg.recoilPitch -= pitchRecoilAdd;
}

#define MAX_SMOKESPRITES 512
//#define SMOKEBOMB_DISTANCEBETWEENSPRITES 16.f
#define SMOKEBOMB_SPAWNRATE 10
#define SMOKEBOMB_SMOKEVELOCITY ((640.f - 16.f) / 8) / 1000.f       // units per msec
#define SMOKEBOMB_STARTRADIUS 16
#define SMOKEBOMB_FINALRADIUS 640

typedef struct smokesprite_s
{
	struct smokesprite_s *next;
	struct smokesprite_s *prev;             // this one is only valid for alloced smokesprites

	vec3_t pos;
	vec4_t colour;

	vec3_t dir;
	float dist;
	float size;

	centity_t *smokebomb;
} smokesprite_t;

static smokesprite_t SmokeSprites[MAX_SMOKESPRITES];
static int           SmokeSpriteCount = 0;
static smokesprite_t *firstfreesmokesprite;         // pointer to the first free smokepuff in the SmokeSprites pool
static smokesprite_t *lastusedsmokesprite;          // pointer to the last used smokepuff

/**
 * @brief InitSmokeSprites
 */
void InitSmokeSprites(void)
{
	int i;

	Com_Memset(&SmokeSprites, 0, sizeof(SmokeSprites));
	for (i = 0; i < MAX_SMOKESPRITES - 1; i++)
	{
		SmokeSprites[i].next = &SmokeSprites[i + 1];
	}

	firstfreesmokesprite = &SmokeSprites[0];
	lastusedsmokesprite  = NULL;
	SmokeSpriteCount     = 0;
}

/**
 * @brief AllocSmokeSprite
 * @return
 */
static smokesprite_t *AllocSmokeSprite(void)
{
	smokesprite_t *alloc;

	if (SmokeSpriteCount >= MAX_SMOKESPRITES)
	{
		return NULL;
	}

	alloc = firstfreesmokesprite;

	firstfreesmokesprite = alloc->next;

	if (lastusedsmokesprite)
	{
		lastusedsmokesprite->next = alloc;
	}

	alloc->next         = NULL;
	alloc->prev         = lastusedsmokesprite;
	lastusedsmokesprite = alloc;

	SmokeSpriteCount++;
	return(alloc);
}

/**
 * @brief DeAllocSmokeSprite
 * @param[in,out] dealloc
 * @return Previous alloced smokesprite in list (or NULL when there are no more alloced smokesprites left)
 */
static smokesprite_t *DeAllocSmokeSprite(smokesprite_t *dealloc)
{
	smokesprite_t *ret_smokesprite;

	if (dealloc->prev)
	{
		dealloc->prev->next = dealloc->next;
	}

	if (dealloc->next)
	{
		dealloc->next->prev = dealloc->prev;
	}
	else     // no next particle, so this particle was 'lastusedsmokesprite'
	{
		lastusedsmokesprite = dealloc->prev;
		if (lastusedsmokesprite)     // incase that there was no previous particle (happens when there is only one particle and this one gets dealloced)
		{
			lastusedsmokesprite->next = NULL;
		}
	}

	ret_smokesprite = dealloc->prev;

	Com_Memset(dealloc, 0, sizeof(smokesprite_t));
	dealloc->next        = firstfreesmokesprite;
	firstfreesmokesprite = dealloc;

	SmokeSpriteCount--;
	return(ret_smokesprite);
}

/**
 * @brief CG_SmokeSpritePhysics
 * @param[in,out] smokesprite
 * @param[in] dist
 * @return
 */
static qboolean CG_SmokeSpritePhysics(smokesprite_t *smokesprite, const float dist)
{
	trace_t tr;
	vec3_t  oldpos;
	//vec3_t mins, maxs;

	VectorCopy(smokesprite->pos, oldpos);
	VectorMA(oldpos, dist, smokesprite->dir, smokesprite->pos);

	smokesprite->dist += dist;

	smokesprite->size += 1.25f * dist;

	// see if we hit a solid
	// FIXME: use mins and max with smoke sprite  minimum radius and then expand to max possible distance or real current sprite size?
	// would definately look nice I think
	//VectorSet( maxs, .3f * smokesprite->size, .3f * smokesprite->size, .3f * smokesprite->size );
	//VectorNegate( maxs, mins );
	//CG_Trace( &tr, oldpos, mins, maxs, smokesprite->pos, -1, CONTENTS_SOLID );
	CG_Trace(&tr, oldpos, NULL, NULL, smokesprite->pos, -1, CONTENTS_SOLID);

	if (tr.fraction != 1.f)
	{
		//float dot;

		if (smokesprite->dist < 24)
		{
			return qfalse;
		}
		VectorCopy(tr.endpos, smokesprite->pos);

		// bounce off
		//dot = DotProduct( smokesprite->dir, tr.plane.normal );
		//VectorMA( smokesprite->dir, -2*dot, tr.plane.normal, smokesprite->dir );
		//VectorScale( smokesprite->dir, .25f, smokesprite->dir );
	} // else {
	  //	smokesprite->size += 1.25f * dist;
	  //}

	return qtrue;
}

/**
 * @brief CG_SpawnSmokeSprite
 * @param[in,out] cent
 * @param[in] dist
 * @return
 */
qboolean CG_SpawnSmokeSprite(centity_t *cent, float dist)
{
	smokesprite_t *smokesprite = AllocSmokeSprite();

	if (smokesprite)
	{
		smokesprite->smokebomb = cent;
		//VectorCopy( cent->lerpOrigin, smokesprite->pos );
		//smokesprite->pos[2] += 32;
		VectorCopy(cent->origin2, smokesprite->pos);
		VectorCopy(bytedirs[rand() % NUMVERTEXNORMALS], smokesprite->dir);
		smokesprite->dir[2]   *= .5f;
		smokesprite->size      = 16.f;
		smokesprite->colour[0] = .35f; // + crandom() * .1f;
		smokesprite->colour[1] = smokesprite->colour[0];
		smokesprite->colour[2] = smokesprite->colour[0];
		smokesprite->colour[3] = .8f;

		// Advance sprite
		if (!CG_SmokeSpritePhysics(smokesprite, dist))
		{
			DeAllocSmokeSprite(smokesprite);
			return qfalse;
		}
		else
		{
			cent->miscTime++;
		}
	}

	return qtrue;
}

/**
 * @brief CG_RenderSmokeGrenadeSmoke
 * @param[in,out] cent
 * @param[in] weapon
 */
void CG_RenderSmokeGrenadeSmoke(centity_t *cent, const weaponInfo_t *weapon)
{
	//int numSpritesForRadius, numNewSpritesNeeded = 0;
	float spawnrate = (1.f / SMOKEBOMB_SPAWNRATE) * 1000.f;

	if (!cent->currentState.effect1Time)
	{
		cent->miscTime          = 0;
		cent->lastFuseSparkTime = 0;    // last spawn time
		cent->muzzleFlashTime   = 0;    // delta time
		cent->dl_atten          = 0;
		return;
	}

	if (cent->currentState.effect1Time > SMOKEBOMB_STARTRADIUS)
	{
		int volume        = SMOKEBOMB_STARTRADIUS + ((cent->currentState.effect1Time / SMOKEBOMB_FINALRADIUS) * (100 - SMOKEBOMB_STARTRADIUS));
		int spritesNeeded = 0;

		if (!cent->dl_atten ||
		    cent->currentState.pos.trType != TR_STATIONARY ||
		    (cent->currentState.groundEntityNum != ENTITYNUM_WORLD && !VectorCompare(cent->lastLerpOrigin, cent->lerpOrigin)))
		{
			trace_t tr;

			VectorCopy(cent->lerpOrigin, cent->origin2);
			cent->origin2[2] += 32;
			CG_Trace(&tr, cent->currentState.pos.trBase, NULL, NULL, cent->origin2, -1, CONTENTS_SOLID);

			if (tr.startsolid)
			{
				cent->dl_atten = 2;
			}
			else
			{
				VectorCopy(tr.endpos, cent->origin2);
				cent->dl_atten = 1;
			}
		}

		trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, weapon->overheatSound, volume, 0);

		// emitter is stuck in solid
		if (cent->dl_atten == 2)
		{
			return;
		}

		// Number of sprites for radius calculation:
		// lifetime of a sprite : (.5f * radius) / velocity
		// number of sprites in a row: radius / SMOKEBOMB_DISTANCEBETWEENSPRITES
		//numSpritesForRadius = cent->currentState.effect1Time / SMOKEBOMB_DISTANCEBETWEENSPRITES;

		//numSpritesForRadius = cent->currentState.effect1Time / ((((640.f - 16.f)/16)/1000.f) * cg.frametime);
		//numNewSpritesNeeded = numSpritesForRadius - cent->miscTime;

		//CG_Printf( "numSpritesForRadius: %i / numNewSpritesNeeded: %i / cent->miscTime: %i\n", numSpritesForRadius, numNewSpritesNeeded, cent->miscTime );

		if (cg.oldTime && cent->lastFuseSparkTime != cg.time)
		{
			cent->muzzleFlashTime  += cg.frametime;
			spritesNeeded           = cent->muzzleFlashTime / (int)spawnrate;
			cent->muzzleFlashTime  -= (spawnrate * spritesNeeded);
			cent->lastFuseSparkTime = cg.time;
		}

		//if( spritesNeeded + cent->miscTime < 40 )
		//	spritesNeeded = 40 - cent->miscTime;

		if (!spritesNeeded)
		{
			return;
		}
		else if (spritesNeeded == 1)
		{
			// this is theoretically fine, till the smokegrenade ends up in a solid
			//while( !CG_SpawnSmokeSprite( cent, 0.f ) );

			// this is better
			if (!CG_SpawnSmokeSprite(cent, 0.f))
			{
				// try again, just in case, so we don't get lots of gaps and remain quite constant
				CG_SpawnSmokeSprite(cent, 0.f);
			}
		}
		else
		{
			//float lerpfrac = 1.0f / (float)spritesNeeded;
			float lerp = 1.0f;
			float dtime;

			for (dtime = spritesNeeded * spawnrate; dtime > 0; dtime -= spawnrate)
			{
				// this is theoretically fine, till the smokegrenade ends up in a solid
				//while( !CG_SpawnSmokeSprite( cent, lerp * cg.frametime * SMOKEBOMB_SMOKEVELOCITY ) );

				// this is better
				if (!CG_SpawnSmokeSprite(cent, lerp * cg.frametime * SMOKEBOMB_SMOKEVELOCITY))
				{
					// try again, just in case, so we don't get lots of gaps and remain quite constant
					CG_SpawnSmokeSprite(cent, lerp * cg.frametime * SMOKEBOMB_SMOKEVELOCITY);
				}
			}
		}
	}
	else if (cent->currentState.effect1Time == -1)
	{
		// unlink smokesprites from smokebomb
		if (cent->miscTime > 0)
		{
			smokesprite_t *smokesprite = lastusedsmokesprite;

			while (smokesprite)
			{
				if (smokesprite->smokebomb == cent)
				{
					smokesprite->smokebomb = NULL;
					cent->miscTime--;
				}

				smokesprite = smokesprite->prev;
			}
		}
	}
}

/**
 * @brief CG_AddSmokeSprites
 */
void CG_AddSmokeSprites(void)
{
	smokesprite_t *smokesprite = lastusedsmokesprite;
	byte          color[4];
	polyVert_t    verts[4];
	vec3_t        top, bottom;
	vec3_t        right, up, tmp;
	float         radius;
	float         halfSmokeSpriteWidth, halfSmokeSpriteHeight;
	float         dist = SMOKEBOMB_SMOKEVELOCITY * cg.frametime;

	while (smokesprite)
	{
		if (smokesprite->smokebomb && !smokesprite->smokebomb->currentValid)
		{
			smokesprite = smokesprite->prev;
			continue;
		}

		// Do physics
		if (!CG_SmokeSpritePhysics(smokesprite, dist))
		{
			if (smokesprite->smokebomb)
			{
				smokesprite->smokebomb->miscTime--;
			}
			smokesprite = DeAllocSmokeSprite(smokesprite);
			continue;
		}

		if (smokesprite->smokebomb)
		{
			radius = smokesprite->smokebomb->currentState.effect1Time;
		}
		else
		{
			radius = -1;
		}

		if (radius < 0)
		{
			radius = 640; // max radius

		}
		// Expire sprites
		if (smokesprite->dist > radius * .5f)
		{
			if (smokesprite->smokebomb)
			{
				smokesprite->smokebomb->miscTime--;
			}

			smokesprite = DeAllocSmokeSprite(smokesprite);
			continue;
		}

		// Now render it
		halfSmokeSpriteWidth  = 0.5f * smokesprite->size;
		halfSmokeSpriteHeight = 0.5f * smokesprite->size;

		VectorCopy(cg.refdef_current->viewaxis[1], tmp);
		RotatePointAroundVector(right, cg.refdef_current->viewaxis[0], tmp, 0);
		CrossProduct(cg.refdef_current->viewaxis[0], right, up);

		VectorMA(smokesprite->pos, halfSmokeSpriteHeight, up, top);
		VectorMA(smokesprite->pos, -halfSmokeSpriteHeight, up, bottom);

		color[0] = (byte)(smokesprite->colour[0] * 0xff);
		color[1] = (byte)(smokesprite->colour[1] * 0xff);
		color[2] = (byte)(smokesprite->colour[2] * 0xff);
		color[3] = (byte)(smokesprite->colour[3] * 0xff);

		// fadeout
		if (smokesprite->dist > (radius * .5f * .8f))
		{
			color[3] = (byte)(smokesprite->colour[3] - smokesprite->colour[3] * ((smokesprite->dist - (radius * .5f * .8f)) / ((radius * .5f) - (radius * .5f * .8f)))) * 0xff;
		}
		else
		{
			color[3] = (byte)(smokesprite->colour[3] * 0xff);
		}

		VectorMA(top, halfSmokeSpriteWidth, right, verts[0].xyz);
		verts[0].st[0] = 1;
		verts[0].st[1] = 0;
		Com_Memcpy(verts[0].modulate, color, 4);

		VectorMA(top, -halfSmokeSpriteWidth, right, verts[1].xyz);
		verts[1].st[0] = 0;
		verts[1].st[1] = 0;
		Com_Memcpy(verts[1].modulate, color, 4);

		VectorMA(bottom, -halfSmokeSpriteWidth, right, verts[2].xyz);
		verts[2].st[0] = 0;
		verts[2].st[1] = 1;
		Com_Memcpy(verts[2].modulate, color, 4);

		VectorMA(bottom, halfSmokeSpriteWidth, right, verts[3].xyz);
		verts[3].st[0] = 1;
		verts[3].st[1] = 1;
		Com_Memcpy(verts[3].modulate, color, 4);

		trap_R_AddPolyToScene(cgs.media.smokePuffShader, 4, verts);

		smokesprite = smokesprite->prev;
	}
}
