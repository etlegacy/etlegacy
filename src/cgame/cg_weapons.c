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
 * @file cg_weapons.c
 * @brief Events and effects dealing with weapons
 */

#include "cg_local.h"

/**
 * @var weapBanksMultiPlayer
 * @brief The new loadout for WolfXP
 */
weapon_t weapBanksMultiPlayer[MAX_WEAP_BANKS_MP][MAX_WEAPS_IN_BANK_MP] =
{
	{ 0,                   0,                    0,               0,               0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       }, // empty bank '0'
	{ WP_KNIFE,            WP_KNIFE_KABAR,       0,               0,               0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ WP_LUGER,            WP_COLT,              WP_AKIMBO_COLT,  WP_AKIMBO_LUGER, WP_SILENCER,    WP_SILENCED_COLT, WP_AKIMBO_SILENCEDCOLT, WP_AKIMBO_SILENCEDLUGER, 0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ WP_MP40,             WP_THOMPSON,          WP_STEN,         WP_GARAND,       WP_PANZERFAUST, WP_FLAMETHROWER,  WP_KAR98,               WP_CARBINE,              WP_FG42, WP_K43, WP_MOBILE_MG42, WP_MOBILE_BROWNING, WP_MORTAR, WP_MORTAR2, WP_BAZOOKA, WP_GPG40, WP_M7, WP_MP34 },
	{ WP_GRENADE_LAUNCHER, WP_GRENADE_PINEAPPLE, 0,               0,               0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ WP_MEDIC_SYRINGE,    WP_PLIERS,            WP_SMOKE_MARKER, WP_SMOKE_BOMB,   0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ WP_DYNAMITE,         WP_MEDKIT,            WP_AMMO,         WP_SATCHEL,      WP_SATCHEL_DET, 0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ WP_LANDMINE,         WP_MEDIC_ADRENALINE,  0,               0,               0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ WP_BINOCULARS,       0,                    0,               0,               0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
	{ 0,                   0,                    0,               0,               0,              0,                0,                      0,                       0,       0,      0,              0,                  0,         0,          0,          0,        0,     0       },
};

/**
 * @brief CG_StartWeaponAnim
 * @param[in] anim
 */
static void CG_StartWeaponAnim(int anim)
{
	if (cg.predictedPlayerState.pm_type >= PM_DEAD)
	{
		return;
	}

	if (cg.predictedPlayerState.weapon == WP_NONE)
	{
		return;
	}

	cg.predictedPlayerState.weapAnim = ((cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

/**
 * @brief CG_ContinueWeaponAnim
 * @param[in] anim
 */
static void CG_ContinueWeaponAnim(int anim)
{
	if ((cg.predictedPlayerState.weapAnim & ~ANIM_TOGGLEBIT) == anim)
	{
		return;
	}

	CG_StartWeaponAnim(anim);
}

/**
 * @brief Add leaning offset
 * @param[in] viewAngle
 * @param[out] point
 * @param[in] ammount
 */
void AddLean(vec3_t viewAngle, vec3_t point, float ammount)
{
	if (ammount != 0.f)
	{
		vec3_t up, right;

		AngleVectors(viewAngle, up, right, NULL);
		VectorMA(point, ammount, right, point);
		// to match client's view
		point[2] -= Q_fabs(ammount / 3.5f);
	}
}

/**
 * @brief CG_MachineGunEjectBrass
 * @param[in] cent
 */
void CG_MachineGunEjectBrass(centity_t *cent)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity, xvelocity;
	vec3_t        offset = { 0, 0, 0 };
	float         waterScale = 1.0f;
	vec3_t        v[3], end;
	qboolean      isFirstPerson = ((cent->currentState.clientNum == cg.snap->ps.clientNum) && !cg.renderingThirdPerson);

	if (cg_brassTime.integer <= 0)
	{
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = (int)(le->startTime + cg_brassTime.integer + (cg_brassTime.integer / 4) * random());

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);

	AnglesToAxis(cent->lerpAngles, v);

	// new brass handling behavior because the SP stuff just doesn't cut it for MP
	if (BG_PlayerMounted(cent->currentState.eFlags))
	{
		// adjust for the MG tank mounted
		if ((cent->currentState.eFlags & EF_MOUNTEDTANK))
		{
			if (isFirstPerson)
			{
				refEntity_t brass;

				Com_Memset(&brass, 0, sizeof(brass));

				CG_PositionRotatedEntityOnTag(&brass, &cg.predictedPlayerEntity.pe.handRefEnt, "tag_brass");

				VectorMA(brass.origin, 6, brass.axis[0], re->origin);
			}
			else
			{
				offset[0] = -11;
				offset[1] = -4;
				offset[2] = -1;
			}
		}
		else
		{
			offset[0] = 25;
			offset[1] = -4;
			offset[2] = 28;
		}

		velocity[0]          = -20 + 40 * crandom();  // more reasonable brass ballistics for a machinegun
		velocity[1]          = -150 + 40 * crandom();
		velocity[2]          = 100 + 50 * crandom();
		re->hModel           = cgs.media.machinegunBrassModel;
		le->angles.trBase[0] = 90;  //rand()&31; // belt-fed rounds should come out horizontal
	}
	else
	{
		if (GetWeaponTableData(cent->currentState.weapon)->type & (WEAPON_TYPE_MG | WEAPON_TYPE_RIFLE))
		{
			re->hModel = cgs.media.machinegunBrassModel;
		}
		else
		{
			re->hModel = cgs.media.smallgunBrassModel;
		}

		velocity[0] = -50 + 25 * crandom();
		velocity[1] = -100 + 40 * crandom();
		velocity[2] = 200 + 50 * random();

		if (isFirstPerson)
		{
			refEntity_t brass;

			Com_Memset(&brass, 0, sizeof(brass));

			if ((GetWeaponTableData(cent->currentState.weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO) && !cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&brass, &cg.predictedPlayerEntity.pe.handRefEnt, "tag_brass2");
			}
			else
			{
				CG_PositionRotatedEntityOnTag(&brass, &cg.predictedPlayerEntity.pe.handRefEnt, "tag_brass");
			}

			VectorCopy(brass.origin, re->origin);

			le->angles.trBase[0] = (rand() & 31) + 60;    // bullets should come out horizontal not vertical JPW NERVE
		}
		else
		{
			VectorCopy(cg_weapons[cent->currentState.weapon].ejectBrassOffset, offset);
			le->angles.trBase[0] = (rand() & 15) + 82;   // bullets should come out horizontal not vertical JPW NERVE
		}
	}

	if ((cent->currentState.eFlags & EF_MG42_ACTIVE) || (cent->currentState.eFlags & EF_AAGUN_ACTIVE) || !isFirstPerson)
	{
		vec3_t xoffset;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

		if ((cent->currentState.eFlags & EF_MOUNTEDTANK))
		{
			VectorAdd(cg_entities[cg_entities[cent->currentState.clientNum].tagParent].mountedMG42.origin, xoffset, re->origin);
		}
		else
		{
			VectorAdd(cent->lerpOrigin, xoffset, re->origin);
		}
	}

	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & (CONTENTS_WATER | CONTENTS_SLIME)) // modified since slime is no longer deadly
	{
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);

	le->bounceFactor = 0.4f * waterScale;

	le->angles.trType     = TR_LINEAR;
	le->angles.trTime     = cg.time;
	le->angles.trBase[1]  = rand() & 255; // random spin from extractor
	le->angles.trBase[2]  = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	if (CG_PointContents(end, 0) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		le->leBounceSoundType = LEBS_NONE;
	}
	else
	{
		le->leBounceSoundType = LEBS_BRASS;
	}

	le->leMarkType = LEMT_NONE;
}

/**
 * @brief Toss the 'used' panzerfaust casing (unit is one-shot, disposable)
 * @param[in] cent
 */
static void CG_PanzerFaustEjectBrass(centity_t *cent)
{
	localEntity_t *le      = CG_AllocLocalEntity();
	refEntity_t   *re      = &le->refEntity;
	vec3_t        velocity = { 16, -200, 0 };
	vec3_t        offset;
	vec3_t        xoffset, xvelocity;
	float         waterScale = 1.0f;
	vec3_t        v[3];

	VectorCopy(cg_weapons[cent->currentState.weapon].ejectBrassOffset, offset);

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = (int)(le->startTime + (cg_brassTime.integer * 8) + (cg_brassTime.integer * random()));

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand() & 15);
	//le->pos.trTime = cg.time - 2000;

	AnglesToAxis(cent->lerpAngles, v);

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd(cent->lerpOrigin, xoffset, re->origin);

	VectorCopy(re->origin, le->pos.trBase);

	if (CG_PointContents(re->origin, -1) & (CONTENTS_WATER | CONTENTS_SLIME))
	{
		waterScale = 0.1f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale(xvelocity, waterScale, le->pos.trDelta);

	AxisCopy(axisDefault, re->axis);

	// make it bigger
	le->sizeScale = 3.0f;

	re->hModel = cgs.media.panzerfaustBrassModel;

	le->bounceFactor = 0.4f * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;

	le->angles.trBase[0] = 0;
	le->angles.trBase[1] = cent->currentState.apos.trBase[1];   // rotate to match the player
	le->angles.trBase[2] = 0;

	le->angles.trDelta[0] = 0;
	le->angles.trDelta[1] = 0;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE | LEF_SMOKING;   // probably doesn't need to be 'tumble' since it doesn't really rotate much when flying

	le->leBounceSoundType = LEBS_NONE;

	le->leMarkType = LEMT_NONE;
}

/**
 * @brief Compute random wind vector for smoke puff
 * @param[out] dir
 */
void CG_GetWindVector(vec3_t dir)
{
	dir[0] = random() * 0.25f;
	dir[1] = (float)sin(0.00001 * cg.time); // simulate a little wind so it looks natural
	dir[2] = random(); // one direction (so smoke goes side-like)
	VectorNormalize(dir);
}

/**
 * @brief LT/FOPS pyro for marking air strikes
 * @param[in,out] ent
 * @param wi - unused
 */
void CG_PyroSmokeTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int    step;
	vec3_t origin, lastPos, dir;
	//int           contents;
	int           startTime;
	entityState_t *es;
	int           t;
	float         rnd;

	if (ent->currentState.weapon == WP_LANDMINE)
	{
		if (ent->currentState.effect1Time != 2)
		{
			ent->miscTime = 0;
			return;
		}

		if (!ent->miscTime)
		{
			ent->trailTime = cg.time;
			ent->miscTime  = cg.time;

			// play the armed sound - weird place to do it but saves us sending an event
			trap_S_StartSound(NULL, ent->currentState.number, CHAN_WEAPON, cgs.media.minePrimedSound);
		}

		if (cg.time - ent->miscTime > 1000)
		{
			return;
		}
	}

	step      = 50;
	es        = &ent->currentState;
	startTime = ent->trailTime;
	t         = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin, qfalse, es->effect2Time);
	CG_PointContents(origin, -1);

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos, qfalse, es->effect2Time);
	CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	/* smoke pyro works fine in water (well, it's dye in real life, might wanna change this in-game)
	    if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	        return;
	*/

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step)
	{
		BG_EvaluateTrajectory(&es->pos, t, lastPos, qfalse, es->effect2Time);

		if (ent->currentState.density)     // corkscrew effect
		{
			vec3_t right;
			vec3_t angles;

			VectorCopy(ent->currentState.apos.trBase, angles);
			angles[ROLL] += cg.time % 360;
			AngleVectors(angles, NULL, right, NULL);
			VectorMA(lastPos, ent->currentState.density, right, lastPos);
		}

		dir[0] = crandom() * 5; // compute offset from flare base
		dir[1] = crandom() * 5;
		dir[2] = 0;
		VectorAdd(lastPos, dir, origin); // store in origin

		CG_GetWindVector(dir);
		if (ent->currentState.weapon == WP_LANDMINE)
		{
			VectorScale(dir, 45, dir);
		}
		else
		{
			VectorScale(dir, 65, dir);
		}

		rnd = random();

		if (ent->currentState.teamNum == TEAM_ALLIES)     // allied team, generate blue smoke
		{
			CG_SmokePuff(origin, dir,
			             25 + rnd * 110,       // width
			             rnd * 0.5f + 0.5f, rnd * 0.5f + 0.5f, 1, 0.5f,
			             4800 + (rand() % 2800),         // duration was 2800+
			             t,
			             0,
			             0,
			             cgs.media.smokePuffShader);
		}
		else
		{
			CG_SmokePuff(origin, dir,
			             25 + rnd * 110,       // width
			             1.0f, rnd * 0.5f + 0.5f, rnd * 0.5f + 0.5f, 0.5f,
			             4800 + (rand() % 2800),         // duration was 2800+
			             t,
			             0,
			             0,
			             cgs.media.smokePuffShader);
		}
	}
}

/**
 * @brief New trail effects
 * @param[in,out] ent
 * @param wi - unused
 */
void CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int           step = (ent->currentState.eType == ET_FLAMEBARREL) ? 30 : 10;
	vec3_t        origin, lastPos;
	int           contents;
	int           lastContents, startTime = ent->trailTime;
	entityState_t *es = &ent->currentState;
	int           t   = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin, qfalse, es->effect2Time);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ((ent->currentState.eType != ET_RAMJET) && es->pos.trType == TR_STATIONARY)
	{
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos, qfalse, es->effect2Time);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		if (contents & lastContents & CONTENTS_WATER)
		{
			CG_BubbleTrail(lastPos, origin, 3, 8);
		}
		return;
	}

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step)
	{
		float rnd;

		BG_EvaluateTrajectory(&es->pos, t, lastPos, qfalse, es->effect2Time);
		rnd = random();

		if (ent->currentState.eType == ET_FLAMEBARREL)
		{
			if ((rand() % 100) > 50)
			{
				CG_ParticleExplosion("twiltb2", lastPos, vec3_origin, 100 + (int)(rnd * 400), 5, 7 + (int)(rnd * 10), qfalse);       // fire

			}
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 800 + (int)(rnd * 1500), 5, 12 + (int)(rnd * 30), qfalse);          // smoke
		}
		else if (ent->currentState.eType == ET_RAMJET)
		{
			int duration;

			VectorCopy(ent->lerpOrigin, lastPos);
			duration = 100;
			CG_ParticleExplosion("twiltb2", lastPos, vec3_origin, duration + (int)(rnd * 100), 5, 5 + (int)(rnd * 10), qfalse);       // fire
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 400 + (int)(rnd * 750), 12, 24 + (int)(rnd * 30), qfalse);          // smoke
		}
		else
		{
			//CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, 300+(int)(rnd*100), 4, 14+(int)(rnd*8) );   // fire
			CG_ParticleExplosion("blacksmokeanim", lastPos, vec3_origin, 800 + (int)(rnd * 1500), 5, 12 + (int)(rnd * 30), qfalse);          // smoke
		}
	}
}

/**
 * @brief CG_DynamiteTrail
 * @param[in] ent
 * @param wi - unused
 */
static void CG_DynamiteTrail(centity_t *ent, const weaponInfo_t *wi)
{
	vec3_t origin;
	float  mult;

	BG_EvaluateTrajectory(&ent->currentState.pos, cg.time, origin, qfalse, ent->currentState.effect2Time);

	if (ent->currentState.effect1Time)
	{
		mult = 0.004f * (cg.time - ent->currentState.effect1Time) / 30000.0f;
		trap_R_AddLightToScene(origin, 320, (float)fabs(sin((cg.time - ent->currentState.effect1Time) * mult)), 1.0f, 0, 0, 0, REF_FORCE_DLIGHT);
	}
	else
	{
		mult = 1 - ((cg.time - ent->trailTime) / 15500.0f);
		trap_R_AddLightToScene(origin, 320, mult, 1.0f, 1.0f, 0, 0, REF_FORCE_DLIGHT);
	}
}

/**
 * @brief CG_GrenadeTrail
 * @param[in,out] ent
 * @param wi - unused
 */
static void CG_GrenadeTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int           step = 15;  // nice and smooth curves
	vec3_t        origin, lastPos;
	int           contents;
	int           lastContents;
	int           startTime = ent->trailTime;
	entityState_t *es       = &ent->currentState;
	int           t         = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin, qfalse, es->effect2Time);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if (es->pos.trType == TR_STATIONARY)
	{
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos, qfalse, es->effect2Time);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if ((cgs.clientinfo[cg.clientNum].shoutcaster || cgs.sv_cheats) && cg_shoutcastGrenadeTrail.integer)
	{
		vec3_t colorStart = { 1.0f, 0.0f, 0.0f };
		vec3_t colorEnd   = { 1.0f, 0.0f, 0.0f };

		if (ent->currentState.weapon == WP_SMOKE_BOMB)
		{
			VectorSet(colorStart, 0.0f, 0.0f, 1.0f);
			VectorSet(colorEnd, 0.0f, 0.0f, 1.0f);
		}

		for (; t <= ent->trailTime; t += step)
		{
			BG_EvaluateTrajectory(&es->pos, t, origin, qfalse, es->effect2Time);
			ent->headJuncIndex = CG_AddTrailJunc(ent->headJuncIndex,
			                                     ent,
			                                     cgs.media.railCoreShader,
			                                     startTime,
			                                     0,
			                                     origin,
			                                     750,
			                                     0.3f,
			                                     0.0f,
			                                     2,
			                                     20,
			                                     0,
			                                     colorStart,
			                                     colorEnd,
			                                     0,
			                                     0);
			ent->lastTrailTime = cg.time;
		}
	}
	else
	{
		if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		{
			if (contents & lastContents & CONTENTS_WATER)
			{
				CG_BubbleTrail(lastPos, origin, 2, 8);
			}
			return;
		}
		// spawn smoke junctions
		for (; t <= ent->trailTime; t += step)
		{
			BG_EvaluateTrajectory(&es->pos, t, origin, qfalse, es->effect2Time);
			ent->headJuncIndex = CG_AddSmokeJunc(ent->headJuncIndex,
			                                     ent, // trail fix
			                                     cgs.media.smokeTrailShader,
			                                     origin,
			                                     1000, 0.3f, 2, 20);
			ent->lastTrailTime = cg.time;
		}
	}
}

/**
 * @brief Re-inserted this as a debug mechanism for bullets
 * @param[in] color
 * @param[in] start
 * @param[in] end
 * @param[in] index
 * @param[in] sideNum
 */
void CG_RailTrail2(vec3_t color, vec3_t start, vec3_t end, int index, int sideNum)
{
	localEntity_t *le;
	refEntity_t   *re;

	if (index)
	{
		le = CG_FindLocalEntity(index, sideNum);

		if (!le)
		{
			le = CG_AllocLocalEntity();
		}

		le->data1 = index;
		le->data2 = sideNum;
	}
	else
	{
		le = CG_AllocLocalEntity();
	}

	re = &le->refEntity;

	le->leType    = LE_CONST_RGB;
	le->startTime = cg.time;
	le->endTime   = cg.time + cg_railTrailTime.integer;
	le->lifeRate  = 1.0f / (le->endTime - le->startTime);

	re->shaderTime   = cg.time / 1000.0f;
	re->reType       = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);

	le->color[0] = color[0];
	le->color[1] = color[1];
	le->color[2] = color[2];
	le->color[3] = 1.0f;

	AxisClear(re->axis);
}

/**
 * @brief CG_RailTrail
 * @param[in] color
 * @param[in] start
 * @param[in] end
 * @param[in] type
 * @param[in] index
 */
void CG_RailTrail(vec3_t color, vec3_t start, vec3_t end, int type, int index)
{
	vec3_t diff, v1, v2, v3, v4, v5, v6;

	if (!type)     // just a line
	{
		CG_RailTrail2(color, start, end, index, -1);
		return;
	}

	// type '1' (box)

	VectorSubtract(start, end, diff);

	VectorCopy(start, v1);
	VectorCopy(start, v2);
	VectorCopy(start, v3);
	v1[0] -= diff[0];
	v2[1] -= diff[1];
	v3[2] -= diff[2];
	CG_RailTrail2(color, start, v1, index, 1);
	CG_RailTrail2(color, start, v2, index, 2);
	CG_RailTrail2(color, start, v3, index, 3);

	VectorCopy(end, v4);
	VectorCopy(end, v5);
	VectorCopy(end, v6);
	v4[0] += diff[0];
	v5[1] += diff[1];
	v6[2] += diff[2];
	CG_RailTrail2(color, end, v4, index, 4);
	CG_RailTrail2(color, end, v5, index, 5);
	CG_RailTrail2(color, end, v6, index, 6);

	CG_RailTrail2(color, v2, v6, index, 7);
	CG_RailTrail2(color, v6, v1, index, 8);
	CG_RailTrail2(color, v1, v5, index, 9);

	CG_RailTrail2(color, v2, v4, index, 10);
	CG_RailTrail2(color, v4, v3, index, 11);
	CG_RailTrail2(color, v3, v5, index, 12);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Read information for weapon animations (first/length/fps)
 * @param[in] filename
 * @param[out] wi
 * @return
 */
static qboolean CG_ParseWeaponConfig(const char *filename, weaponInfo_t *wi)
{
	char         *text_p, *prev;
	int          len;
	int          i;
	float        fps;
	char         *token;
	qboolean     newfmt = qfalse;
	char         text[20000];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);

	if (len <= 0)
	{
		CG_Printf("CG_ParseWeaponConfig: File not found: %s\n", filename);
		return qfalse;
	}

	if (len >= sizeof(text) - 1)
	{
		CG_Printf("CG_ParseWeaponConfig: File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;

	COM_BeginParseSession("CG_ParseWeaponConfig");

	// read optional parameters
	while (1)
	{
		prev  = text_p; // so we can unget
		token = COM_Parse(&text_p);

		if (!token[0])                         // get the variable
		{
			break;
		}

		//if ( !Q_stricmp( token, "whatever_variable" ) )
		//{
		//  token = COM_Parse( &text_p );   // get the value
		//  if (!token[0])
		//  {
		//    break;
		//  }
		//  continue;
		//}

		if (!Q_stricmp(token, "newfmt"))
		{
			newfmt = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9')
		{
			text_p = prev;  // unget the token
			break;
		}

		Com_Printf("CG_ParseWeaponConfig: Unknown token in weapon cfg '%s' in %s\n", token, filename);
	}

	for (i = 0 ; i < MAX_WP_ANIMATIONS  ; i++)
	{
		token = COM_Parse(&text_p);     // first frame

		if (!token[0])
		{
			break;
		}

		wi->weapAnimations[i].firstFrame = Q_atoi(token);

		token = COM_Parse(&text_p);     // length

		if (!token[0])
		{
			break;
		}

		wi->weapAnimations[i].numFrames = Q_atoi(token);

		token = COM_Parse(&text_p);     // fps

		if (!token[0])
		{
			break;
		}

		fps = (float)atof(token);

		if (fps == 0.f)
		{
			fps = 1;
		}

		wi->weapAnimations[i].frameLerp   = (int)(1000 / fps);
		wi->weapAnimations[i].initialLerp = (int)(1000 / fps);

		token = COM_Parse(&text_p);     // looping frames

		if (!token[0])
		{
			break;
		}

		wi->weapAnimations[i].loopFrames = Q_atoi(token);

		if (wi->weapAnimations[i].loopFrames > wi->weapAnimations[i].numFrames)
		{
			wi->weapAnimations[i].loopFrames = wi->weapAnimations[i].numFrames;
		}
		else if (wi->weapAnimations[i].loopFrames < 0)
		{
			wi->weapAnimations[i].loopFrames = 0;
		}

		// store animation/draw bits in '.moveSpeed'

		wi->weapAnimations[i].moveSpeed = 0;

		if (newfmt)
		{
			token = COM_Parse(&text_p);     // barrel anim bits

			if (!token[0])
			{
				break;
			}

			wi->weapAnimations[i].moveSpeed = Q_atoi(token);

			token = COM_Parse(&text_p);     // animated weapon

			if (!token[0])
			{
				break;
			}

			if (atoi(token))
			{
				wi->weapAnimations[i].moveSpeed |= (1 << W_MAX_PARTS);      // set the bit one higher than can be set by the barrel bits

			}

			token = COM_Parse(&text_p);     // barrel hide bits (so objects can be flagged to not be drawn during all sequences (a reloading hand that comes in from off screen for that one animation for example)

			if (!token[0])
			{
				break;
			}

			wi->weapAnimations[i].moveSpeed |= ((atoi(token)) << 8);       // use 2nd byte for draw bits
		}
	}

	if (i != MAX_WP_ANIMATIONS)
	{
		CG_Printf("CG_ParseWeaponConfig: Error parsing weapon animation file: %s\n", filename);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseError
 * @param[in] handle
 * @param[in] format
 * @return
 */
static qboolean CG_RW_ParseError(int handle, const char *format, ...)
{
	int         line;
	char        filename[MAX_QPATH];
	va_list     argptr;
	static char string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	filename[0] = '\0';
	line        = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

/**
 * @brief CG_RW_ParseWeaponLinkPart
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @param[in] viewType
 * @return
 */
static qboolean CG_RW_ParseWeaponLinkPart(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t  token;
	char        filename[MAX_QPATH];
	int         part;
	partModel_t *partModel;

	if (!PC_Int_Parse(handle, &part))
	{
		return CG_RW_ParseError(handle, "expected part index");
	}

	if (part < 0 || part >= W_MAX_PARTS)
	{
		return CG_RW_ParseError(handle, "part index out of bounds");
	}

	partModel = &weaponInfo->partModels[viewType][part];

	Com_Memset(partModel, 0, sizeof(*partModel));

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "tag"))
		{
			if (!PC_String_ParseNoAlloc(handle, partModel->tagName, sizeof(partModel->tagName)))
			{
				return CG_RW_ParseError(handle, "expected tag name");
			}
		}
		else if (!Q_stricmp(token.string, "model"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected model filename");
			}

			partModel->model = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "skin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			partModel->skin[0] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "axisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			partModel->skin[TEAM_AXIS] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "alliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			partModel->skin[TEAM_ALLIES] = trap_R_RegisterSkin(filename);
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseWeaponLink
 * @param[in] handle
 * @param[in] weaponInfo
 * @param[in] viewType
 * @return
 */
static qboolean CG_RW_ParseWeaponLink(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "part"))
		{
			if (!CG_RW_ParseWeaponLinkPart(handle, weaponInfo, viewType))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseViewType
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @param[in] viewType
 * @return
 */
static qboolean CG_RW_ParseViewType(int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType)
{
	pc_token_t token;
	char       filename[MAX_QPATH];

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "model"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected model filename");
			}

			weaponInfo->weaponModel[viewType].model = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "skin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->weaponModel[viewType].skin[0] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "axisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->weaponModel[viewType].skin[TEAM_AXIS] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "alliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->weaponModel[viewType].skin[TEAM_ALLIES] = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "flashModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected flashModel filename");
			}

			weaponInfo->flashModel[viewType] = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "weaponLink"))
		{
			if (!CG_RW_ParseWeaponLink(handle, weaponInfo, viewType))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseModModel
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseModModel(int handle, weaponInfo_t *weaponInfo)
{
	char filename[MAX_QPATH];
	int  mod;

	if (!PC_Int_Parse(handle, &mod))
	{
		return CG_RW_ParseError(handle, "expected mod index");
	}

	if (mod < 0 || mod >= 6)
	{
		return CG_RW_ParseError(handle, "mod index out of bounds");
	}

	if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
	{
		return CG_RW_ParseError(handle, "expected model filename");
	}

	// maybe it's a shader ...
	// check extensions of file name and register shader OR model
	if (!strstr(filename, ".md3") && !strstr(filename, ".mdc")) // FIXME: add more model formats?
	{   // we assume it's a shader
		weaponInfo->modModels[mod] = trap_R_RegisterShader(filename);
	}
	else
	{
		weaponInfo->modModels[mod] = trap_R_RegisterModel(filename);
	}

	// still no q_handle_t
	if (!weaponInfo->modModels[mod])
	{
		Com_Printf(S_COLOR_RED "ERROR: CG_RW_ParseModModel() no model or shader for %s registered.\n", filename);
		return qfalse; // this isn't vanilla behaviour
	}

	return qtrue;
}

/**
 * @var soundSurfaceTable
 */
static const soundSurfaceTable_t soundSurfaceTable[W_MAX_SND_SURF] =
{
	{ 0,           "default" },
	{ 0,           "far",    },
	{ SURF_METAL,  "metal",  },
	{ SURF_WOOD,   "wood",   },
	{ SURF_GRASS,  "grass",  },
	{ SURF_GRAVEL, "gravel", },
	{ SURF_GLASS,  "glass",  },
	{ SURF_SNOW,   "snow",   },
	{ SURF_ROOF,   "roof",   },
	{ SURF_CARPET, "carpet"  },
	{ 0,           "water",  },
	{ 0,           "flesh"   }
};

/**
 * @brief CG_RW_ParseWeaponSound
 * @param[in] handle
 * @param[in,out] weaponSounds
 * @return
 */
static qboolean CG_RW_ParseWeaponSound(int handle, weaponSounds_t *weaponSounds)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return CG_RW_ParseError(handle, "expected sounds filenames or sounds number");
	}

	// get the number of files sound to register
	if (token.type == TT_NUMBER)
	{
		int  i;
		char filename[MAX_QPATH];

		if (token.intvalue > MAX_WEAPON_SOUNDS)
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: only up to 5 sounds supported per weapon sound\n");
		}

		if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
		{
			return CG_RW_ParseError(handle, "expected soundSurface filename");
		}

		for (i = 0; i < token.intvalue && i < MAX_WEAPON_SOUNDS; i++)
		{
			weaponSounds->sounds[i] = trap_S_RegisterSound(va("%s%i.wav", filename, i + 1), qfalse);
		}

		weaponSounds->count = i;
	}
	else    // assume only one file sound must be register
	{
		weaponSounds->count     = 1;
		weaponSounds->sounds[0] = trap_S_RegisterSound(token.string, qfalse);
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseSoundSurface
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseSoundSurface(int handle, weaponSounds_t *weaponSound)
{
	pc_token_t     token;
	soundSurface_t soundSurface;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		for (soundSurface = 0; soundSurface < W_MAX_SND_SURF; soundSurface++)
		{
			if (!Q_stricmp(token.string, soundSurfaceTable[soundSurface].surfaceName))
			{
				break;
			}
		}

		if (soundSurface == W_MAX_SND_SURF)
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}

		if (!CG_RW_ParseWeaponSound(handle, &weaponSound[soundSurface]))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseImpactMark
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseImpactMark(int handle, weaponInfo_t *weaponInfo)
{
	pc_token_t     token;
	char           filename[MAX_QPATH];
	soundSurface_t impactSurface;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	filename[0] = '\0';

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		for (impactSurface = 0; impactSurface < W_MAX_SND_SURF; impactSurface++)
		{
			if (!Q_stricmp(token.string, soundSurfaceTable[impactSurface].surfaceName))
			{
				if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
				{
					return CG_RW_ParseError(handle, "expected impactMark filename");
				}

				break;
			}
		}

		if (impactSurface == W_MAX_SND_SURF)
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}

		weaponInfo->impactMark[impactSurface] = trap_R_RegisterShaderNoMip(filename);
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseImpactMark
 * @param[in] handle
 * @param[out] impactParticle
 * @return
 */
static qboolean CG_RW_ParseParticleEffect(int handle, impactParticle_t *impactParticle)
{
	int                    index;
	char                   filename[MAX_QPATH];
	char                   surfaceType[8] = { 0 };
	pc_token_t             token;
	soundSurface_t         impactSurface;
	impactParticleEffect_t *weaponParticleEffect = NULL;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "particleEffectType"))
	{
		return CG_RW_ParseError(handle, "expected particleEffectType");
	}

	if (!PC_String_ParseNoAlloc(handle, surfaceType, sizeof(surfaceType)))
	{
		return CG_RW_ParseError(handle, "expected particleEffectType");
	}

	for (impactSurface = 0; impactSurface < W_MAX_SND_SURF; impactSurface++)
	{
		if (!Q_stricmp(surfaceType, soundSurfaceTable[impactSurface].surfaceName))
		{
			break;
		}
	}

	if (impactSurface == W_MAX_SND_SURF)
	{
		return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
	}

	for (index = 0; index < MAX_IMPACT_PARTICLE_EFFECT; index++)
	{
		if (!impactParticle->particleEffect[impactSurface][index].particleEffectUsed)
		{
			impactParticle->particleEffect[impactSurface][index].particleEffectUsed = qtrue;
			weaponParticleEffect                                                    = &impactParticle->particleEffect[impactSurface][index];
			break;
		}
	}

	if (index == MAX_IMPACT_PARTICLE_EFFECT /*|| !weaponParticleEffect*/)
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: only up to %i particle effect per surface\n", MAX_IMPACT_PARTICLE_EFFECT);
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "particleEffectShader"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected particleEffectShader filename");
			}

			weaponParticleEffect->particleEffectShader = trap_R_RegisterShader(filename);
		}
		else if (!Q_stricmp(token.string, "particleEffectSpeed"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectSpeed))
			{
				return CG_RW_ParseError(handle, "expected particleEffectSpeed");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectSpeedRand"))
		{
			if (!PC_Float_Parse(handle, &weaponParticleEffect->particleEffectSpeedRand))
			{
				return CG_RW_ParseError(handle, "expected particleEffectSpeedRand");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectDuration"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectDuration))
			{
				return CG_RW_ParseError(handle, "expected particleEffectDuration");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectCount"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectCount))
			{
				return CG_RW_ParseError(handle, "expected particleEffectCount");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectRandScale"))
		{
			if (!PC_Float_Parse(handle, &weaponParticleEffect->particleEffectRandScale))
			{
				return CG_RW_ParseError(handle, "expected particleEffectRandScale");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectWidth"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectWidth))
			{
				return CG_RW_ParseError(handle, "expected particleEffectWidth");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectHeight"))
		{
			if (!PC_Int_Parse(handle, &weaponParticleEffect->particleEffectHeight))
			{
				return CG_RW_ParseError(handle, "expected particleEffectHeight");
			}
		}
		else if (!Q_stricmp(token.string, "particleEffectAlpha"))
		{
			if (!PC_Float_Parse(handle, &weaponParticleEffect->particleEffectAlpha))
			{
				return CG_RW_ParseError(handle, "expected particleEffectAlpha");
			}
		}
	}

	return qtrue;
}

/**
 * @brief CG_RW_ParseExtraEffect
 * @param[in] handle
 * @param[out] impactParticle
 * @return
 */
static qboolean CG_RW_ParseExtraEffect(int handle, impactParticle_t *impactParticle)
{
	int                 index;
	pc_token_t          token;
	impactExtraEffect_t *impactExtraEffect = NULL;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	for (index = 0; index < MAX_IMPACT_PARTICLE_EFFECT; index++)
	{
		if (!impactParticle->extraEffect[index].extraEffectUsed)
		{
			impactParticle->extraEffect[index].extraEffectUsed = qtrue;
			impactExtraEffect                                  = &impactParticle->extraEffect[index];
			break;
		}
	}

	if (index == MAX_IMPACT_PARTICLE_EFFECT /*|| !impactExtraEffect*/)
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: only up to %i particle effect per surface\n", MAX_IMPACT_PARTICLE_EFFECT);
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "extraEffectCount"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectCount))
			{
				return CG_RW_ParseError(handle, "expected extraEffectCount");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectOriginRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectOriginRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectOriginRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectVelocityRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectVelocityRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectVelocityRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectVelocityScaling"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectVelocityScaling))
			{
				return CG_RW_ParseError(handle, "expected extraEffectVelocityScaling");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectShaderName"))
		{
			if (!PC_String_ParseNoAlloc(handle, impactExtraEffect->extraEffectShaderName, sizeof(impactExtraEffect->extraEffectShaderName)))
			{
				return CG_RW_ParseError(handle, "expected particleEffectRandScale");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectDuration"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectDuration))
			{
				return CG_RW_ParseError(handle, "expected extraEffectDuration");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectDurationRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectDurationRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectDurationRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeStart"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectSizeStart))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeStart");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeStartRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectSizeStartRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeStartRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeEnd"))
		{
			if (!PC_Int_Parse(handle, &impactExtraEffect->extraEffectSizeEnd))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeEnd");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectSizeEndRand"))
		{
			if (!PC_Float_Parse(handle, &impactExtraEffect->extraEffectSizeEndRand))
			{
				return CG_RW_ParseError(handle, "expected extraEffectSizeEndRand");
			}
		}
		else if (!Q_stricmp(token.string, "extraEffectLightAnim"))
		{
			impactExtraEffect->extraEffectLightAnim = qtrue;
		}
	}

	return qtrue;
}

static impactParticleTable_t impactParticleTable[MAX_IMPACT_PARTICLE] = { { { 0 }, { 0 } } };

/**
 * @brief CG_ParseWeaponImpactParticle
 * @param[in] filename
 * @param[in] pImpactParticle
 * @return
 */
static qboolean CG_ParseWeaponImpactParticle(const char *filename, impactParticle_t **pImpactParticle)
{
	pc_token_t       token;
	int              handle;
	impactParticle_t *impactParticle = NULL;
	int              i;

	for (i = 0; i < MAX_IMPACT_PARTICLE; i++)
	{
		if (impactParticleTable[i].impactParticleName[0] == 0)
		{
			Q_strncpyz(impactParticleTable[i].impactParticleName, filename, MAX_QPATH);
			*pImpactParticle = impactParticle = &impactParticleTable[i].impactParticle;
			break;
		}
		else if (!Q_stricmp(filename, &impactParticleTable[i].impactParticleName[0]))
		{
			*pImpactParticle = impactParticle = &impactParticleTable[i].impactParticle;
			return qtrue;
		}
	}

	if (i == MAX_IMPACT_PARTICLE)
	{
		CG_Printf(S_COLOR_RED "WARNING: too much impact particle declared. Max is %i\n", MAX_IMPACT_PARTICLE);
		return qfalse;
	}

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "weaponImpactParticleDef"))
	{
		return CG_RW_ParseError(handle, "expected 'weaponImpactParticleDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "particleDirectionOffset"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->particleDirectionOffset))
			{
				return CG_RW_ParseError(handle, "expected particleDirectionOffset");
			}
		}
		else if (!Q_stricmp(token.string, "particleDirectionScaling"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->particleDirectionScaling))
			{
				return CG_RW_ParseError(handle, "expected particleDirectionScaling");
			}
		}
		else if (!Q_stricmp(token.string, "waterRippleRadius"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterRippleRadius))
			{
				return CG_RW_ParseError(handle, "expected waterRippleRadius");
			}
		}
		else if (!Q_stricmp(token.string, "waterRippleLifeTime"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterRippleLifeTime))
			{
				return CG_RW_ParseError(handle, "expected waterRippleLifeTime");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashDuration"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterSplashDuration))
			{
				return CG_RW_ParseError(handle, "expected waterSplashDuration");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashLight"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->waterSplashLight))
			{
				return CG_RW_ParseError(handle, "expected waterSplashLight");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashLightColor"))
		{
			if (!PC_Vec_Parse(handle, &impactParticle->waterSplashLightColor))
			{
				return CG_RW_ParseError(handle, "expected waterSplashLightColor");
			}
		}
		else if (!Q_stricmp(token.string, "waterSplashIsSprite"))
		{
			impactParticle->waterSplashIsSprite = qtrue;
		}
		else if (!Q_stricmp(token.string, "explosionShaderName"))
		{
			if (!PC_String_ParseNoAlloc(handle, impactParticle->explosionShaderName, sizeof(impactParticle->explosionShaderName)))
			{
				return CG_RW_ParseError(handle, "expected explosionShaderName");
			}
		}
		else if (!Q_stricmp(token.string, "explosionDuration"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->explosionDuration))
			{
				return CG_RW_ParseError(handle, "expected explosionDuration");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeStart"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->explosionSizeStart))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeStart");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeStartRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->explosionSizeStartRand))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeStartRand");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeEnd"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->explosionSizeEnd))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeEnd");
			}
		}
		else if (!Q_stricmp(token.string, "explosionSizeEndRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->explosionSizeEndRand))
			{
				return CG_RW_ParseError(handle, "expected explosionSizeEndRand");
			}
		}
		else if (!Q_stricmp(token.string, "explosionLightAnim"))
		{
			impactParticle->explosionLightAnim = qtrue;
		}
		else if (!Q_stricmp(token.string, "debrisSpeed"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisSpeed))
			{
				return CG_RW_ParseError(handle, "expected debrisSpeed");
			}
		}
		else if (!Q_stricmp(token.string, "debrisSpeedRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->debrisSpeedRand))
			{
				return CG_RW_ParseError(handle, "expected debrisSpeedRand");
			}
		}
		else if (!Q_stricmp(token.string, "debrisDuration"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisDuration))
			{
				return CG_RW_ParseError(handle, "expected debrisDuration");
			}
		}
		else if (!Q_stricmp(token.string, "debrisDurationRand"))
		{
			if (!PC_Float_Parse(handle, &impactParticle->debrisDurationRand))
			{
				return CG_RW_ParseError(handle, "expected debrisDurationRand");
			}
		}
		else if (!Q_stricmp(token.string, "debrisCount"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisCount))
			{
				return CG_RW_ParseError(handle, "expected debrisCount");
			}
		}
		else if (!Q_stricmp(token.string, "debrisCountExtra"))
		{
			if (!PC_Int_Parse(handle, &impactParticle->debrisCountExtra))
			{
				return CG_RW_ParseError(handle, "expected debrisCountExtra");
			}
		}
		else if (!Q_stricmp(token.string, "debrisForBullet"))
		{
			impactParticle->debrisForBullet = qtrue;
		}
		else if (!Q_stricmp(token.string, "particleEffect"))
		{
			if (!CG_RW_ParseParticleEffect(handle, impactParticle))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "extraEffect"))
		{
			if (!CG_RW_ParseExtraEffect(handle, impactParticle))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_RW_ParseClient
 * @param[in] handle
 * @param[in,out] weaponInfo
 * @return
 */
static qboolean CG_RW_ParseClient(int handle, weaponInfo_t *weaponInfo)
{
	pc_token_t token;
	char       filename[MAX_QPATH];

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "standModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected standModel filename");
			}

			weaponInfo->standModel = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "droppedAnglesHack"))
		{
			weaponInfo->droppedAnglesHack = qtrue;
		}
		else if (!Q_stricmp(token.string, "pickupModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected pickupModel filename");
			}

			weaponInfo->weaponModel[W_PU_MODEL].model = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "pickupSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected pickupSound filename");
			}

			//weaponInfo->pickupSound = trap_S_RegisterSound( filename, qfalse );
		}
		else if (!Q_stricmp(token.string, "weaponConfig"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponConfig filename");
			}

			if (!CG_ParseWeaponConfig(filename, weaponInfo))
			{
				CG_Error("Couldn't register weapon (failed to parse %s)", filename);
			}
		}
		else if (!Q_stricmp(token.string, "handsModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected handsModel filename");
			}

			weaponInfo->handsModel = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "flashDlightColor"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->flashDlightColor))
			{
				return CG_RW_ParseError(handle, "expected flashDlightColor as r g b");
			}
		}
		else if (!Q_stricmp(token.string, "flashSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->flashSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "flashEchoSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->flashEchoSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "lastShotSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->lastShotSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "readySound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected readySound filename");
			}

			weaponInfo->readySound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "firingSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected firingSound filename");
			}

			weaponInfo->firingSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "overheatSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected overheatSound filename");
			}

			weaponInfo->overheatSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "reloadSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected reloadSound filename");
			}

			weaponInfo->reloadSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "reloadFastSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected reloadFastSound filename");
			}

			weaponInfo->reloadFastSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "spinupSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected spinupSound filename");
			}

			weaponInfo->spinupSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "spindownSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected spindownSound filename");
			}

			weaponInfo->spindownSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "switchSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected switchSound filename");
			}

			weaponInfo->switchSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "noAmmoSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected noAmmoSound filename");
			}

			weaponInfo->noAmmoSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "weaponIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponIcon filename");
			}

			weaponInfo->weaponIcon[0] = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponIconScale"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->weaponIconScale))
			{
				return CG_RW_ParseError(handle, "expected weaponIconScale filename");
			}
		}
		else if (!Q_stricmp(token.string, "weaponSelectedIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponSelectedIcon filename");
			}

			weaponInfo->weaponIcon[1] = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponSimpleIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponSimpleIcon shadername");
			}

			weaponInfo->weaponSimpleIcon = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponSimpleIconScale"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponSimpleIconScale))
			{
				return CG_RW_ParseError(handle, "expected weaponSimpleIconScale X Y");
			}
		}
		else if (!Q_stricmp(token.string, "weaponCardIcon"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected weaponCardIcon filename");
			}

			weaponInfo->weaponCardIcon = trap_R_RegisterShaderNoMip(filename);
		}
		else if (!Q_stricmp(token.string, "weaponCardScale"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponCardScale))
			{
				return CG_RW_ParseError(handle, "expected weaponCardScale as width height");
			}
		}
		else if (!Q_stricmp(token.string, "weaponCardPointS"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponCardPointS))
			{
				return CG_RW_ParseError(handle, "expected weaponCardScale as S0 S1");
			}
		}
		else if (!Q_stricmp(token.string, "weaponCardPointT"))
		{
			if (!PC_Point_Parse(handle, &weaponInfo->weaponCardPointT))
			{
				return CG_RW_ParseError(handle, "expected weaponCardScale as T0 T1");
			}
		}
		else if (!Q_stricmp(token.string, "missileModel"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileModel filename");
			}

			weaponInfo->missileModel = trap_R_RegisterModel(filename);
		}
		else if (!Q_stricmp(token.string, "missileAlliedSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->missileAlliedSkin = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "missileAxisSkin"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected skin filename");
			}

			weaponInfo->missileAxisSkin = trap_R_RegisterSkin(filename);
		}
		else if (!Q_stricmp(token.string, "missileSound"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileSound filename");
			}

			weaponInfo->missileSound = trap_S_RegisterSound(filename, qfalse);
		}
		else if (!Q_stricmp(token.string, "missileFallSound"))
		{
			if (!CG_RW_ParseWeaponSound(handle, &weaponInfo->missileFallSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "missileBouncingSound"))
		{
			if (!CG_RW_ParseSoundSurface(handle, weaponInfo->missileBouncingSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "missileTrailFunc"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected missileTrailFunc");
			}

			if (!Q_stricmp(filename, "GrenadeTrail"))
			{
				weaponInfo->missileTrailFunc = CG_GrenadeTrail;
			}
			else if (!Q_stricmp(filename, "RocketTrail"))
			{
				weaponInfo->missileTrailFunc = CG_RocketTrail;
			}
			else if (!Q_stricmp(filename, "PyroSmokeTrail"))
			{
				weaponInfo->missileTrailFunc = CG_PyroSmokeTrail;
			}
			else if (!Q_stricmp(filename, "DynamiteTrail"))
			{
				weaponInfo->missileTrailFunc = CG_DynamiteTrail;
			}
		}
		else if (!Q_stricmp(token.string, "missileDlight"))
		{
			if (!PC_Float_Parse(handle, &weaponInfo->missileDlight))
			{
				return CG_RW_ParseError(handle, "expected missileDlight value");
			}
		}
		else if (!Q_stricmp(token.string, "missileDlightColor"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->missileDlightColor))
			{
				return CG_RW_ParseError(handle, "expected missileDlightColor as r g b");
			}
		}
		else if (!Q_stricmp(token.string, "ejectBrassFunc"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected ejectBrassFunc");
			}

			if (!Q_stricmp(filename, "MachineGunEjectBrass"))
			{
				weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
			}
			else if (!Q_stricmp(filename, "PanzerFaustEjectBrass"))
			{
				weaponInfo->ejectBrassFunc = CG_PanzerFaustEjectBrass;
			}
		}
		else if (!Q_stricmp(token.string, "ejectBrassOffset"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->ejectBrassOffset))
			{
				return CG_RW_ParseError(handle, "expected ejectBrassOffset as foward left up");
			}
		}
		else if (!Q_stricmp(token.string, "fireRecoil"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->fireRecoil))
			{
				return CG_RW_ParseError(handle, "expected fireRecoil as pitch yaw roll");
			}
		}
		else if (!Q_stricmp(token.string, "adjustLean"))
		{
			if (!PC_Vec_Parse(handle, &weaponInfo->adjustLean))
			{
				return CG_RW_ParseError(handle, "expected adjustLean as pitch yaw roll");
			}
		}
		else if (!Q_stricmp(token.string, "impactDurationCoeff"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactDurationCoeff))
			{
				return CG_RW_ParseError(handle, "expected impactDurationCoeff value");
			}
		}
		else if (!Q_stricmp(token.string, "impactMarkMaxRange"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactMarkMaxRange))
			{
				return CG_RW_ParseError(handle, "expected impactMarkMaxRange value");
			}
		}
		else if (!Q_stricmp(token.string, "impactSoundRange"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactSoundRange))
			{
				return CG_RW_ParseError(handle, "expected impactSoundRange value");
			}
		}
		else if (!Q_stricmp(token.string, "impactSoundVolume"))
		{
			if (!PC_Int_Parse(handle, &weaponInfo->impactSoundVolume))
			{
				return CG_RW_ParseError(handle, "expected impactSoundVolume value");
			}
		}
		else if (!Q_stricmp(token.string, "impactMarkRadius"))
		{
			if (!PC_Float_Parse(handle, &weaponInfo->impactMarkRadius))
			{
				return CG_RW_ParseError(handle, "expected impactMarkRadius value");
			}
		}
		else if (!Q_stricmp(token.string, "impactParticle"))
		{
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected impactParticle filename");
			}

			if (!CG_ParseWeaponImpactParticle(filename, &weaponInfo->impactParticle))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "impactSound"))
		{
			if (!CG_RW_ParseSoundSurface(handle, weaponInfo->impactSound))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "impactMark"))
		{
			if (!CG_RW_ParseImpactMark(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "modModel"))
		{
			if (!CG_RW_ParseModModel(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "firstPerson"))
		{
			if (!CG_RW_ParseViewType(handle, weaponInfo, W_FP_MODEL))
			{
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "thirdPerson"))
		{
			if (!CG_RW_ParseViewType(handle, weaponInfo, W_TP_MODEL))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	return qtrue;
}

/**
 * @brief CG_RegisterWeaponFromWeaponFile
 * @param[in] filename
 * @param[in] weaponInfo
 * @return
 */
static qboolean CG_RegisterWeaponFromWeaponFile(const char *filename, weaponInfo_t *weaponInfo)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "weaponDef"))
	{
		return CG_RW_ParseError(handle, "expected 'weaponDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_RW_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "client"))
		{
			if (!CG_RW_ParseClient(handle, weaponInfo))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_RW_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_RegisterWeapon
 * @param[in] weaponNum
 * @param[in] force
 */
void CG_RegisterWeapon(int weaponNum, qboolean force)
{
	weaponInfo_t *weaponInfo;

	if (!IS_VALID_WEAPON(weaponNum))
	{
		return;
	}

	weaponInfo = &cg_weapons[weaponNum];

	if (weaponInfo->registered && !force)
	{
		return;
	}

	Com_Memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	/*for( item = bg_itemlist + 1 ; item->classname ; item++ ) {
	    if( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
	        weaponInfo->item = item;
	        break;
	    }
	}

	if( !item->classname ) {
	    CG_Error( "Couldn't find weapon %i", weaponNum );
	}*/

	if (GetWeaponTableData(weaponNum)->weapFile)
	{
		if (!CG_RegisterWeaponFromWeaponFile(va("weapons/%s.weap", GetWeaponTableData(weaponNum)->weapFile), weaponInfo))
		{
			CG_Printf(S_COLOR_RED "WARNING: failed to register media for weapon %i from %s.weap\n", weaponNum, GetWeaponTableData(weaponNum)->weapFile);
		}
	}
	else
	{
		// no weapon file for theses weapons
		if (weaponNum == VERYBIGEXPLOSION || weaponNum == WP_DUMMY_MG42)
		{
			//CG_Printf(S_COLOR_YELLOW "WARNING: skipping weapon %i to register.\n", weaponNum);
			weaponInfo->weaponIconScale = 1;
		}
		else
		{
			CG_Printf(S_COLOR_RED "WARNING: trying to register weapon %i but there is no weapon file entry for it.\n", weaponNum);
		}
	}
}

/**
 * @brief Registers models and icons for items of bg_itemlist (except weapons which are registered from *.weap file data)
 *
 * @param[in] itemNum
 *
 * @note Actually IT_AMMO & IT_TEAM have no visuals
 */
void CG_RegisterItemVisuals(int itemNum)
{
	itemInfo_t *itemInfo = &BG_GetItem(itemNum)->itemInfo;
	gitem_t    *item;
	int        i;

	if (itemInfo->registered)
	{
		return;
	}

	item = BG_GetItem(itemNum);

	if (item->giType == IT_WEAPON)
	{
		return;
	}

	Com_Memset(itemInfo, 0, sizeof(*itemInfo));

	for (i = 0; i < MAX_ITEM_MODELS; i++)
	{
		// some items don't have world models see (bg_itemlist) - we don't have to register
		if (!item->world_model[i] || !item->world_model[i][0])
		{
			CG_DPrintf("CG_RegisterItemVisuals: NULL or empty world_model[%i] for item classname %s\n", i, item->classname);
			itemInfo->models[i] = 0;
		}
		else
		{
			itemInfo->models[i] = trap_R_RegisterModel(item->world_model[i]);
		}
	}

	// some items have no icon shader - we don't have to register
	if (item->icon && item->icon[0])
	{
		itemInfo->icons[0] = trap_R_RegisterShader(item->icon);
	}
	else
	{
		CG_DPrintf("CG_RegisterItemVisuals: NULL or empty item icon shader [%s] for classname %s\n", item->icon, item->classname);
		itemInfo->icons[0] = 0;
	}

	itemInfo->registered = qtrue;   // moved this down after the registerweapon()
}

/*
========================================================================================
VIEW WEAPON
========================================================================================
*/

// weapon animations

/**
 * @brief Get animation info from the parent if necessary
 * @param[in] cent
 * @param[out] part
 * @param[in] parent
 * @param[in] partid
 * @param[in] wi
 * @return
 */
qboolean CG_GetPartFramesFromWeap(centity_t *cent, refEntity_t *part, refEntity_t *parent, int partid, weaponInfo_t *wi)
{
	int         i;
	int         frameoffset = 0;
	animation_t *anim       = cent->pe.weap.animation;

	if (partid == W_MAX_PARTS)
	{
		return qtrue;   // primary weap model drawn for all frames right now
	}

	// check draw bit
	if (anim->moveSpeed & (1 << (partid + 8)))           // hide bits are in high byte
	{
		return qfalse;  // not drawn for current sequence
	}

	// find part's start frame for this animation sequence
	// rain - & out ANIM_TOGGLEBIT or we'll go way out of bounds
	for (i = 0; i < (cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT); i++)
	{
		if (wi->weapAnimations[i].moveSpeed & (1 << partid))         // this part has animation for this sequence
		{
			frameoffset += wi->weapAnimations[i].numFrames;
		}
	}

	// now set the correct frame into the part
	if (anim->moveSpeed & (1 << partid))
	{
		part->backlerp = parent->backlerp;
		part->oldframe = frameoffset + (parent->oldframe - anim->firstFrame);
		part->frame    = frameoffset + (parent->frame - anim->firstFrame);
	}

	return qtrue;
}

/**
 * @brief May include ANIM_TOGGLEBIT
 * @param[in] wi
 * @param[out] lf
 * @param[in] newAnimation
 */
static void CG_SetWeapLerpFrameAnimation(weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation)
{
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_WP_ANIMATIONS)
	{
		CG_Error("Bad animation number (CG_SWLFA): %i\n", newAnimation);
	}

	anim = &wi->weapAnimations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer == 2)
	{
		CG_Printf("Weap Anim: %d\n", newAnimation);
	}
}

/**
 * @brief CG_ClearWeapLerpFrame
 * @param[in] wi
 * @param[in,out] lf
 * @param[in] animationNumber
 */
void CG_ClearWeapLerpFrame(weaponInfo_t *wi, lerpFrame_t *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetWeapLerpFrameAnimation(wi, lf, animationNumber);
	lf->oldFrame      = lf->frame = lf->animation->firstFrame;
	lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
}

/**
 * @brief Sets cg.snap, cg.oldFrame, and cg.backlerp
 * cg.time should be between oldFrameTime and frameTime after exit
 * @param ci - unused
 * @param[in] wi
 * @param[in,out] lf
 * @param[in] newAnimation
 * @param[in] speedScale
 */
static void CG_RunWeapLerpFrame(clientInfo_t *ci, weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation, float speedScale)
{
	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = 0;
		lf->backlerp = 0.f;
		return;
	}

	// see if the animation sequence is switching
	if (!lf->animation)
	{
		CG_ClearWeapLerpFrame(wi, lf, newAnimation);
	}
	else if (newAnimation != lf->animationNumber)
	{
		if ((newAnimation & ~ANIM_TOGGLEBIT) == WEAP_RAISE)
		{
			CG_ClearWeapLerpFrame(wi, lf, newAnimation);     // clear when switching to raise (since it should be out of view anyway)
		}
		else
		{
			CG_SetWeapLerpFrameAnimation(wi, lf, newAnimation);
		}
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		int         f;
		animation_t *anim;

		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;

		// get the next frame based on the animation
		anim = lf->animation;

		if (!anim->frameLerp)
		{
			return;     // shouldn't happen
		}

		if (cg.time < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}

		f  = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc

		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;

			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}

		lf->frame      = anim->firstFrame + f;
		lf->frameModel = anim->mdxFile;

		if (cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer)
			{
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}

	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0f - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/**
 * @brief Modified.  this is now client-side only (server does not dictate weapon animation info)
 * @param[in] ps
 * @param[in] weapon
 * @param[out] weapOld
 * @param[out] weap
 * @param[out] weapBackLerp
 */
static void CG_WeaponAnimation(playerState_t *ps, weaponInfo_t *weapon, int *weapOld, int *weap, float *weapBackLerp)
{
	centity_t    *cent = &cg.predictedPlayerEntity;
	clientInfo_t *ci   = &cgs.clientinfo[ps->clientNum];

	if (cg_noPlayerAnims.integer)
	{
		*weapOld = *weap = 0;
		return;
	}

	CG_RunWeapLerpFrame(ci, weapon, &cent->pe.weap, ps->weapAnim, 1);

	*weapOld      = cent->pe.weap.oldFrame;
	*weap         = cent->pe.weap.frame;
	*weapBackLerp = cent->pe.weap.backlerp;

	if (cg_debugAnim.integer == 3)
	{
		CG_Printf("oldframe: %d   frame: %d   backlerp: %f\n", cent->pe.weap.oldFrame, cent->pe.weap.frame, (double)cent->pe.weap.backlerp);
	}
}

////////////////////////////////////////////////////////////////////////

/**
 * @brief CG_CalculateWeaponPosition
 * @param[in] origin
 * @param[in] angles
 */
static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles)
{
	float scale;
	int   delta;

	VectorCopy(cg.refdef_current->vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	if (cg.predictedPlayerState.eFlags & EF_MOUNTEDTANK)
	{
		angles[PITCH] = cg.refdefViewAngles[PITCH] / 1.2f;
	}

	if (!cg.renderingThirdPerson && (GetWeaponTableData(cg.predictedPlayerState.weapon)->type & WEAPON_TYPE_SET) &&
	    cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		angles[PITCH] = cg.pmext.mountedWeaponAngles[PITCH];
	}

	if (cg.predictedPlayerState.eFlags & EF_PRONE_MOVING)
	{
		int pronemovingtime = cg.time - cg.proneMovingTime;

		if (pronemovingtime > 0)     // div by 0
		{
			float factor = pronemovingtime > 200 ? 1.f : 1.f / (200.f / (float)pronemovingtime);

			VectorMA(origin, factor * -20, cg.refdef_current->viewaxis[0], origin);
			VectorMA(origin, factor * 3, cg.refdef_current->viewaxis[1], origin);
		}
	}
	else
	{
		int pronenomovingtime = cg.time - -cg.proneMovingTime;

		if (pronenomovingtime < 200)
		{
			float factor = pronenomovingtime == 0 ? 1.f : 1.f - (1.f / (200.f / (float)pronenomovingtime));

			VectorMA(origin, factor * -20, cg.refdef_current->viewaxis[0], origin);
			VectorMA(origin, factor * 3, cg.refdef_current->viewaxis[1], origin);
		}
	}

	// adjust 'lean' into weapon
	if (cg.predictedPlayerState.leanf != 0.f)
	{
		vec3_t right, up;

		// reverse the roll on the weapon so it stays relatively level
		angles[ROLL] -= cg.predictedPlayerState.leanf / (cg_weapons[cg.predictedPlayerState.weapon].adjustLean[ROLL] * 2.0f);
		AngleVectors(angles, NULL, right, up);
		VectorMA(origin, angles[ROLL], right, origin);

		// pitch the gun down a bit to show that firing is not allowed when leaning
		angles[PITCH] += (fabsf(cg.predictedPlayerState.leanf) / (cg_weapons[cg.predictedPlayerState.weapon].adjustLean[PITCH] * 2.0f));

		// this gives you some impression that the weapon stays in relatively the same
		// position while you lean, so you appear to 'peek' over the weapon
		AngleVectors(cg.refdefViewAngles, NULL, right, NULL);
		VectorMA(origin, -cg.predictedPlayerState.leanf / 4.0f, right, origin);
	}

	// on odd legs, invert some angles
	if (cg.bobcycle & 1)
	{
		scale = -cg.xyspeed;
	}
	else
	{
		scale = cg.xyspeed;
	}

	// gun angles from bobbing

	angles[ROLL]  += scale        * cg.bobfracsin * 0.005f;
	angles[YAW]   += scale        * cg.bobfracsin * 0.01f;
	angles[PITCH] += cg.xyspeed   * cg.bobfracsin * 0.005f;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME)
	{
		origin[2] += cg.landChange * 0.25f * delta / LAND_DEFLECT_TIME;
	}
	else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
	{
		origin[2] += cg.landChange * 0.25f *
		             (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

	// idle drift
	if ((!(cg.predictedPlayerState.eFlags & EF_MOUNTEDTANK) && !(GetWeaponTableData(cg.predictedPlayerState.weapon)->type & WEAPON_TYPE_SET)))
	{
		float fracsin = (float)sin(cg.time * 0.001);

		// adjustment for MAX KAUFMAN
		//scale = cg.xyspeed + 40;
		scale = 80;

		angles[ROLL]  += scale * fracsin * 0.01f;
		angles[YAW]   += scale * fracsin * 0.01f;
		angles[PITCH] += scale * fracsin * 0.01f;
	}

	// subtract the kickAngles
	VectorMA(angles, -1.0f, cg.kickAngles, angles);
}

/**
 * @brief CG_FlamethrowerFlame
 * @param[in] cent
 * @param[in] origin
 */
static void CG_FlamethrowerFlame(centity_t *cent, vec3_t origin)
{
	if (cent->currentState.weapon != WP_FLAMETHROWER)
	{
		return;
	}

	CG_FireFlameChunks(cent, origin, cent->lerpAngles, 1.0, qtrue);
	return;
}

/**
 * @brief CG_AddWeaponWithPowerups
 * @param[in] gun
 * @param powerups - unused
 * @param ps - unused
 * @param cent - unused
 *
 * @todo cleanup ?
 */
static void CG_AddWeaponWithPowerups(refEntity_t *gun, int powerups, playerState_t *ps, centity_t *cent)
{
	// add powerup effects
	// no powerup effects on weapons
	trap_R_AddRefEntityToScene(gun);
}

// TODO: unused ?
//#define DEBUG_WEAPON

// TODO: move to top ?
#define BARREL_SMOKE_TIME 1000

/**
 * @brief Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
 * The main player will have this called for BOTH cases, so effects like light and
 * sound should only be done on the world model case.
 * @param[in] parent
 * @param[in] ps
 * @param[in,out] cent
 */
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent)
{
	refEntity_t     gun;
	refEntity_t     barrel;
	refEntity_t     flash;
	vec3_t          angles;
	weapon_t        weaponNum = (weapon_t)cent->currentState.weapon;
	weaponInfo_t    *weapon;
	centity_t       *nonPredictedCent;
	qboolean        drawpart;
	qboolean        isPlayer      = cent->currentState.clientNum == cg.snap->ps.clientNum; // might as well have this check consistant throughout the routine
	qboolean        isFirstPerson = isPlayer && !cg.renderingThirdPerson;
	int             clientNum     = ps ? ps->clientNum : cent->currentState.clientNum;
	team_t          team;
	modelViewType_t modelViewType = ps ? W_FP_MODEL : W_TP_MODEL;
	float           x, y;

	// don't draw any weapons when the binocs are up
	if (cent->currentState.eFlags & EF_ZOOMING)
	{
		return;
	}

	// don't draw weapon stuff when looking through a scope
	if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SCOPED)
	{
		if (isFirstPerson)
		{
			return;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_GRENADE)
	{
		if (ps && !ps->ammoclip[weaponNum])
		{
			return;
		}
	}

	// no weapon when on mg_42
	if (cent->currentState.eFlags & EF_MOUNTEDTANK)
	{
		if (isFirstPerson)
		{
			return;
		}

		if (cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			Com_Memset(&flash, 0, sizeof(flash));
			flash.renderfx = RF_LIGHTING_ORIGIN;
			flash.hModel   = cgs.media.mg42muzzleflash;

			VectorCopy(cg_entities[cg_entities[cent->currentState.number].tagParent].mountedMG42Flash.origin, flash.origin);
			AxisCopy(cg_entities[cg_entities[cent->currentState.number].tagParent].mountedMG42Flash.axis, flash.axis);

			trap_R_AddRefEntityToScene(&flash);

			// add dynamic light
			trap_R_AddLightToScene(flash.origin, 320, 1.25f + (rand() & 31) / 128.0f, 1.0f, 0.6f, 0.23f, 0, 0);
		}
		return;
	}

	if ((cent->currentState.eFlags & EF_MG42_ACTIVE) || (cent->currentState.eFlags & EF_AAGUN_ACTIVE))
	{
		// MG42 Muzzle Flash
		if (cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			CG_MG42EFX(cent);
		}
		return;
	}

	if ((!ps || cg.renderingThirdPerson) && (cent->currentState.eFlags & EF_PRONE_MOVING))
	{
		return;
	}

	weapon = &cg_weapons[weaponNum];

	// add the weapon
	Com_Memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx    = parent->renderfx;

	team = ps ? (team_t)ps->persistant[PERS_TEAM] : cgs.clientinfo[cent->currentState.clientNum].team;

	if ((weaponNum != WP_SATCHEL) && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))
	{
		team = team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;
	}

	gun.hModel = weapon->weaponModel[modelViewType].model;

	if ((team == TEAM_AXIS) && weapon->weaponModel[modelViewType].skin[TEAM_AXIS])
	{
		gun.customSkin = weapon->weaponModel[modelViewType].skin[TEAM_AXIS];
	}
	else if ((team == TEAM_ALLIES) && weapon->weaponModel[modelViewType].skin[TEAM_ALLIES])
	{
		gun.customSkin = weapon->weaponModel[modelViewType].skin[TEAM_ALLIES];
	}
	else
	{
		gun.customSkin = weapon->weaponModel[modelViewType].skin[0];   // if not loaded it's 0 so doesn't do any harm
	}

	if (!gun.hModel)
	{
#ifdef DEBUG_WEAPON
		CG_Printf("returning due to: !gun.hModel\n");
#endif
		return;
	}

	if (!ps && (cg.snap->ps.pm_flags & PMF_LADDER) && isPlayer)          // player on ladder
	{
#ifdef DEBUG_WEAPON
		CG_Printf("returning due to: !ps && cg.snap->ps.pm_flags & PMF_LADDER\n");
#endif
		return;
	}

	if (ps && cg.clientNum != cg.snap->ps.clientNum)
	{
		// calculate mounted weapon angles if spectating client
		if (CHECKBITWISE(GetWeaponTableData(cg.snap->ps.weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET) ||
		    CHECKBITWISE(GetWeaponTableData(cg.snap->ps.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
		{
			// update it only once
			if (cg.weaponSetTime == 0)
			{
				x = angle_mod(360.f + cg.snap->ps.viewangles[0]);
				y = angle_mod(360.f + cg.snap->ps.viewangles[1]);
				VectorSet(cg.pmext.mountedWeaponAngles, x, y, 0);
				cg.weaponSetTime = cg.time;
			}
		}
		else
		{
			cg.weaponSetTime = 0;
		}
	}

	if (ps && !cg.renderingThirdPerson && CHECKBITWISE(GetWeaponTableData(cg.predictedPlayerState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET)
	    && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		vec3_t angles;

		angles[YAW]   = angles[ROLL] = 0.f;
		angles[PITCH] = -.4f * AngleNormalize180(cg.pmext.mountedWeaponAngles[PITCH] - ps->viewangles[PITCH]);

		AnglesToAxis(angles, gun.axis);

		CG_PositionRotatedEntityOnTag(&gun, parent, "tag_weapon");
	}
	else if ((!ps || cg.renderingThirdPerson) && (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_MORTAR))
	{
		CG_PositionEntityOnTag(&gun, parent, "tag_weapon2", 0, NULL);
	}
	else
	{
		CG_PositionEntityOnTag(&gun, parent, "tag_weapon", 0, NULL);
	}

	//qboolean    playerScaled = (qboolean)(cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[0] != 0);
	//if(!ps && playerScaled) {   // don't "un-scale" weap up in 1st person
	// for(i=0;i<3;i++) {  // scale weapon back up so it doesn't pick up the adjusted scale of the character models.
	//   // this will affect any parts attached to the gun as well (barrel/bolt/flash/brass/etc.)
	//   VectorScale( gun.axis[i], 1.0/(cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[i]), gun.axis[i]);
	// }
	//}

	if (ps)
	{
		drawpart = CG_GetPartFramesFromWeap(cent, &gun, parent, W_MAX_PARTS, weapon);     // W_MAX_PARTS specifies this as the primary view model
	}
	else
	{
		drawpart = qtrue;
	}

	if (drawpart)
	{
		if (weaponNum == WP_AMMO)
		{
			if (cgs.clientinfo[clientNum].skill[SK_SIGNALS] >= 1)
			{
				gun.customShader = weapon->modModels[0];
			}
		}

		if (!ps)
		{
			if (weaponNum == WP_MEDIC_SYRINGE)
			{
				if (cgs.clientinfo[clientNum].skill[SK_FIRST_AID] >= 3)
				{
					gun.customShader = weapon->modModels[0];
				}
			}
		}

		CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups, ps, cent);
	}

	if ((!ps || cg.renderingThirdPerson) && GetWeaponTableData(weaponNum)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		// add to other hand as well
		CG_PositionEntityOnTag(&gun, parent, "tag_weapon2", 0, NULL);
		CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups, ps, cent);
	}

	Com_Memset(&barrel, 0, sizeof(barrel));
	VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
	barrel.shadowPlane = parent->shadowPlane;
	barrel.renderfx    = parent->renderfx;

	// add barrels
	// attach generic weapon parts to the first person weapon.
	// if a barrel should be attached for third person, add it in the (!ps) section below
	angles[YAW] = angles[PITCH] = 0;

	if (ps)
	{
		int      i;
		qboolean spunpart;

		for (i = W_PART_1; i < W_MAX_PARTS; i++)
		{
			if (CHECKBITWISE(GetWeaponTableData(weaponNum)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && (i == W_PART_4 || i == W_PART_5))
			{
				if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
				{
					continue;
				}
			}

			spunpart      = qfalse;
			barrel.hModel = weapon->partModels[modelViewType][i].model;

			if (CHECKBITWISE(GetWeaponTableData(weaponNum)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
			{
				if (i == W_PART_3)
				{
					if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
					{
						angles[PITCH] = angles[YAW] = 0.f;
						angles[ROLL]  = .8f * AngleNormalize180(cg.pmext.mountedWeaponAngles[YAW] - ps->viewangles[YAW]);
						spunpart      = qtrue;
					}
				}
				else if (i == W_PART_1 || i == W_PART_2)
				{
					if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
					{
						angles[YAW]   = angles[ROLL] = 0.f;
						angles[PITCH] = -.4f * AngleNormalize180(cg.pmext.mountedWeaponAngles[PITCH] - ps->viewangles[PITCH]);
						spunpart      = qtrue;
					}
				}
			}
			//else if (weaponNum == WP_MOBILE_MG42_SET || weaponNum == WP_MOBILE_BROWNING_SET)
			//{
			//}

			if (spunpart)
			{
				AnglesToAxis(angles, barrel.axis);
			}

			if (barrel.hModel)
			{
				if (spunpart)
				{
					CG_PositionRotatedEntityOnTag(&barrel, parent, weapon->partModels[modelViewType][i].tagName);
				}
				else
				{
					CG_PositionEntityOnTag(&barrel, parent, weapon->partModels[modelViewType][i].tagName, 0, NULL);
				}

				drawpart = CG_GetPartFramesFromWeap(cent, &barrel, parent, i, weapon);

				if (CHECKBITWISE(GetWeaponTableData(weaponNum)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && (i == W_PART_1 || i == W_PART_2))
				{
					if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
					{
						VectorMA(barrel.origin, .5f * angles[PITCH], cg.refdef_current->viewaxis[0], barrel.origin);
					}
				}

				if (drawpart)
				{
					if (team == TEAM_AXIS && weapon->partModels[modelViewType][i].skin[TEAM_AXIS])
					{
						barrel.customSkin = weapon->partModels[modelViewType][i].skin[TEAM_AXIS];
					}
					else if (team == TEAM_ALLIES && weapon->partModels[modelViewType][i].skin[TEAM_ALLIES])
					{
						barrel.customSkin = weapon->partModels[modelViewType][i].skin[TEAM_ALLIES];
					}
					else
					{
						barrel.customSkin = weapon->partModels[modelViewType][i].skin[0];  // if not loaded it's 0 so doesn't do any harm
					}

					if (weaponNum == WP_MEDIC_SYRINGE && i == W_PART_1)
					{
						if (cgs.clientinfo[ps->clientNum].skill[SK_FIRST_AID] >= 3)
						{
							barrel.customShader = weapon->modModels[0];
						}
					}

					CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);

					if (weaponNum == WP_SATCHEL_DET && i == W_PART_1)
					{
						float       rangeSquared;
						qboolean    inRange;
						refEntity_t satchelDetPart;

						if (cg.satchelCharge)
						{
							rangeSquared = DistanceSquared(cg.satchelCharge->lerpOrigin, cg.predictedPlayerEntity.lerpOrigin);
						}
						else
						{
							rangeSquared = Square(2001.f);
						}

						if (rangeSquared <= Square(2000))
						{
							inRange = qtrue;
						}
						else
						{
							inRange = qfalse;
						}

						Com_Memset(&satchelDetPart, 0, sizeof(satchelDetPart));
						VectorCopy(parent->lightingOrigin, satchelDetPart.lightingOrigin);
						satchelDetPart.shadowPlane = parent->shadowPlane;
						satchelDetPart.renderfx    = parent->renderfx;

						satchelDetPart.hModel = weapon->modModels[0];
						CG_PositionEntityOnTag(&satchelDetPart, &barrel, "tag_rlight", 0, NULL);
						satchelDetPart.customShader = inRange ? weapon->modModels[2] : weapon->modModels[3];
						CG_AddWeaponWithPowerups(&satchelDetPart, cent->currentState.powerups, ps, cent);

						CG_PositionEntityOnTag(&satchelDetPart, &barrel, "tag_glight", 0, NULL);
						satchelDetPart.customShader = inRange ? weapon->modModels[5] : weapon->modModels[4];
						CG_AddWeaponWithPowerups(&satchelDetPart, cent->currentState.powerups, ps, cent);

						satchelDetPart.hModel = weapon->modModels[1];
						angles[PITCH]         = angles[ROLL] = 0.f;

						if (inRange)
						{
							angles[YAW] = -30.f + (60.f * (rangeSquared / Square(2000)));
						}
						else
						{
							angles[YAW] = 30.f;
						}

						AnglesToAxis(angles, satchelDetPart.axis);
						CG_PositionRotatedEntityOnTag(&satchelDetPart, &barrel, "tag_needle");
						satchelDetPart.customShader = weapon->modModels[2];
						CG_AddWeaponWithPowerups(&satchelDetPart, cent->currentState.powerups, ps, cent);
					}
					else if (CHECKBITWISE(GetWeaponTableData(weaponNum)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && i == W_PART_3)
					{
						if (ps && !cg.renderingThirdPerson && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
						{
							refEntity_t bipodLeg;

							Com_Memset(&bipodLeg, 0, sizeof(bipodLeg));
							VectorCopy(parent->lightingOrigin, bipodLeg.lightingOrigin);
							bipodLeg.shadowPlane = parent->shadowPlane;
							bipodLeg.renderfx    = parent->renderfx;

							bipodLeg.hModel = weapon->partModels[modelViewType][3].model;
							CG_PositionEntityOnTag(&bipodLeg, &barrel, "tag_barrel4", 0, NULL);
							CG_AddWeaponWithPowerups(&bipodLeg, cent->currentState.powerups, ps, cent);

							bipodLeg.hModel = weapon->partModels[modelViewType][4].model;
							CG_PositionEntityOnTag(&bipodLeg, &barrel, "tag_barrel5", 0, NULL);
							CG_AddWeaponWithPowerups(&bipodLeg, cent->currentState.powerups, ps, cent);
						}
					}
				}
			}
		}
	}

	// add the scope model to the rifle if you've got it
	if (isFirstPerson)  // for now just do it on the first person weapons
	{
		if ((GetWeaponTableData(weaponNum)->type & (WEAPON_TYPE_RIFLE | WEAPON_TYPE_RIFLENADE)) && !(GetWeaponTableData(weaponNum)->type & (WEAPON_TYPE_SCOPABLE)))
		{
			if ((cg.snap->ps.ammo[GetWeaponTableData(WP_GPG40)->ammoIndex] || cg.snap->ps.ammo[GetWeaponTableData(WP_M7)->ammoIndex] || cg.snap->ps.ammoclip[GetWeaponTableData(WP_GPG40)->ammoIndex] || cg.snap->ps.ammoclip[GetWeaponTableData(WP_M7)->ammoIndex]))
			{
				int anim = cg.snap->ps.weapAnim & ~ANIM_TOGGLEBIT;

				if (anim == WEAP_ALTSWITCHFROM || anim == WEAP_ALTSWITCHTO || anim == WEAP_IDLE1 || anim == WEAP_IDLE2)
				{
					// prevent the flying nade effect (much visible with M7 when swapping while raising)
					if (weaponNum == cent->currentState.nextWeapon)
					{
						barrel.hModel = weapon->modModels[0];

						if (barrel.hModel)
						{
							CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
							CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
						}
					}
				}
			}
		}
		else if (CHECKBITWISE(GetWeaponTableData(weaponNum)->type, (WEAPON_TYPE_RIFLE | WEAPON_TYPE_SCOPABLE)))
		{
			barrel.hModel = weapon->modModels[0];

			if (barrel.hModel)
			{
				CG_PositionEntityOnTag(&barrel, &gun, (weaponNum == WP_GARAND) ? "tag_scope2" : "tag_scope", 0, NULL);
				CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			}

			barrel.hModel = weapon->modModels[1];
			//if(barrel.hModel) {
			CG_PositionEntityOnTag(&barrel, &gun, "tag_flash", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			//}
		}
	}
	// 3rd person attachements
	else
	{
		if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLENADE)
		{
			// the holder
			barrel.hModel = weapon->modModels[1];
			CG_PositionEntityOnTag(&barrel, &gun, "tag_flash", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);

			// the grenade - have to always enabled it, no means of telling if another person has a grenade loaded or not atm :/
			//if( cg.snap->ps.weaponstate != WEAPON_FIRING && cg.snap->ps.weaponstate != WEAPON_RELOADING ) {
			if (weaponNum == WP_M7 /*|| weaponNum == WP_CARBINE*/)
			{
				barrel.hModel = weapon->missileModel;
				CG_PositionEntityOnTag(&barrel, &barrel, "tag_prj", 0, NULL);
				CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
			}
		}
		else if ((GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLE) && (GetWeaponTableData(weaponNum)->type & (WEAPON_TYPE_SCOPABLE | WEAPON_TYPE_SCOPED)))
		{
			// the holder
			barrel.hModel = weapon->modModels[2];
			CG_PositionEntityOnTag(&barrel, &gun, "tag_scope", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
		}
		else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_MG)
		{
			barrel.hModel = weapon->modModels[0];
			barrel.frame  = GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SETTABLE;
			CG_PositionEntityOnTag(&barrel, &gun, "tag_bipod", 0, NULL);
			CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, ps, cent);
		}
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on the single player podiums), so
	// go ahead and use the cent
	if ((nonPredictedCent - cg_entities) != cent->currentState.clientNum)
	{
		nonPredictedCent = cent;
	}

	// add the flash
	Com_Memset(&flash, 0, sizeof(flash));
	VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx    = parent->renderfx;

	flash.hModel = weapon->flashModel[modelViewType];

	angles[YAW]   = 0;
	angles[PITCH] = 0;
	angles[ROLL]  = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	if (/*isPlayer &&*/ GetWeaponTableData(weaponNum)->attributes & WEAPON_ATTRIBUT_AKIMBO)
	{
		if (!ps || cg.renderingThirdPerson)
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_weapon");
				VectorMA(flash.origin, 10, flash.axis[0], flash.origin);
			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
			}
		}
		else
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash2");

			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash");
			}
		}
	}
	else
	{
		CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
	}

	// store this position for other cgame elements to access
	cent->pe.gunRefEnt      = gun;
	cent->pe.gunRefEntFrame = cg.clientFrame;

	if (weaponNum == WP_FLAMETHROWER && (nonPredictedCent->currentState.eFlags & EF_FIRING))
	{
		// continuous flash
	}
	else
	{
		// continuous smoke after firing
		if (ps || !isFirstPerson)
		{
			if (GetWeaponTableData(weaponNum)->maxHeat)
			{
				// hot smoking gun
				if ((cg.time - cent->overheatTime < 3000) && !(cent->currentState.powerups & (1 << PW_INVULNERABLE)))
				{
					if (!(rand() % 3))
					{
						float alpha = 1.0f - ((float)(cg.time - cent->overheatTime) / 3000.0f);

						alpha *= 0.25f;     // .25 max alpha
						CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 1000, 8, 20, 30, alpha, 8.f);
					}
				}
			}
			else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_PANZER)
			{
				if (cg.time - cent->muzzleFlashTime < BARREL_SMOKE_TIME)
				{
					if (!(rand() % 5))
					{
						float alpha = 1.0f - ((float)(cg.time - cent->muzzleFlashTime) / (float)BARREL_SMOKE_TIME);     // what fraction of BARREL_SMOKE_TIME are we at

						alpha *= 0.25f;     // .25 max alpha
						CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 1000, 8, 20, 30, alpha, 8.f);
					}
				}
			}
		}

		if (CHECKBITWISE(GetWeaponTableData(weaponNum)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
		{
			if (ps && !cg.renderingThirdPerson && cg.time - cent->muzzleFlashTime < 800)
			{
				CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 700, 16, 20, 30, .12f, 4.f);
			}
		}

		// impulse flash
		if (cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME)
		{
			// blue ignition flame if not firing flamer
			if (weaponNum != WP_FLAMETHROWER)
			{
				return;
			}
		}
	}

	// weaps with barrel smoke
	if (ps || !isFirstPerson)
	{
		if (weaponNum == WP_STEN || weaponNum == WP_MP34)
		{
			if (cg.time - cent->muzzleFlashTime < 100)
			{
				CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, flash.origin, 500, 8, 20, 30, 0.25f, 8.f);
			}
		}
	}

	if (weaponNum != WP_FLAMETHROWER)     // hide the flash also for now
	{
		// weapons that don't need to go any further as they have no flash or light
		if (!flash.hModel)
		{
			return;
		}

		// changed this so the muzzle flash stays onscreen for long enough to be seen
		if (cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			//if (firing) { // Ridah
			trap_R_AddRefEntityToScene(&flash);
		}
	}

	if (ps || !isFirstPerson)
	{
		int muzzleContents;

		// get contents to avoid lights in water etc - FIXME: this might be useful for other weapons too (move up?)
		muzzleContents = CG_PointContents(flash.origin, -1);

		if (muzzleContents & MASK_WATER)
		{
			return;
		}

		// no flamethrower flame on prone moving or dead players
		if ((cent->currentState.eFlags & EF_FIRING) && !(cent->currentState.eFlags & (EF_PRONE_MOVING | EF_DEAD)))
		{
			// Flamethrower effect
			CG_FlamethrowerFlame(cent, flash.origin);

			if (weapon->flashDlightColor[0] != 0.f || weapon->flashDlightColor[1] != 0.f || weapon->flashDlightColor[2] != 0.f)
			{
				trap_R_AddLightToScene(flash.origin, 320, 1.25 + (rand() & 31) / 128.0f, weapon->flashDlightColor[0],
				                       weapon->flashDlightColor[1], weapon->flashDlightColor[2], 0, 0);
			}
		}
		else
		{
			if (weaponNum == WP_FLAMETHROWER)
			{
				vec3_t angles;
				AxisToAngles(flash.axis, angles);

				if (ps)
				{
					if (ps->ammoclip[GetWeaponTableData(weaponNum)->ammoIndex])
					{
						CG_FireFlameChunks(cent, flash.origin, angles, 1.0, qfalse);
					}
				}
				else
				{
					CG_FireFlameChunks(cent, flash.origin, angles, 1.0, qfalse);
				}
			}
		}
	}
}

/**
 * @brief Add the weapon, and flash for the player's view
 * @param[in] ps
 */
void CG_AddViewWeapon(playerState_t *ps)
{
	refEntity_t  *hand = &cg.predictedPlayerEntity.pe.handRefEnt;
	float        fovOffset;
	vec3_t       angles;
	vec3_t       gunoff;
	weaponInfo_t *weapon;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (ps->pm_type == PM_INTERMISSION)
	{
		return;
	}

	// no gun if in third person view
	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg.editingSpeakers)
	{
		return;
	}

	// hide all gun models (0)
	// draw all gun models (1)
	// draw only melee weapon, syringe and pliers, and throwables (incl. grenades) (2)
	if (!cg_drawGun.integer || (cg_drawGun.integer == 2
	                            && GetWeaponTableData(ps->weapon)->type
	                            && !(GetWeaponTableData(ps->weapon)->type & WEAPON_TYPE_GRENADE)
	                            && !(GetWeaponTableData(ps->weapon)->type & WEAPON_TYPE_MELEE)
	                            && !(GetWeaponTableData(ps->weapon)->type & WEAPON_TYPE_SYRINGUE)))
	{
		if ((cg.predictedPlayerState.eFlags & EF_FIRING) && !BG_PlayerMounted(cg.predictedPlayerState.eFlags))
		{
			vec3_t origin;

			// special hack for flamethrower...
			VectorCopy(cg.refdef_current->vieworg, origin);

			VectorMA(origin, 18, cg.refdef_current->viewaxis[0], origin);
			VectorMA(origin, -7, cg.refdef_current->viewaxis[1], origin);
			VectorMA(origin, -4, cg.refdef_current->viewaxis[2], origin);

			// Flamethrower effect
			CG_FlamethrowerFlame(&cg.predictedPlayerEntity, origin);
		}

		if (cg.binocZoomTime)
		{
			if (cg.binocZoomTime < 0)
			{
				if (-cg.binocZoomTime + 700 < cg.time)
				{
					cg.binocZoomTime = 0;
				}
			}
			else
			{
				if (cg.binocZoomTime + 500 < cg.time)
				{
					trap_SendConsoleCommand("+zoom\n");
					cg.binocZoomTime = 0;
				}
				else
				{
					// TODO: ?
				}
			}
		}

		return;
	}

	// don't draw if testing a gun model
	if (cg.testGun)
	{
		return;
	}

	if ((ps->eFlags & EF_MG42_ACTIVE) || (ps->eFlags & EF_AAGUN_ACTIVE))
	{
		return;
	}

	// drop gun lower at higher fov
	if (cg_fov.value > 90)
	{
		fovOffset = -0.2f * (cg_fov.value - 90);
	}
	else
	{
		fovOffset = 0;
	}

	// mounted gun drawing
	if (ps->eFlags & EF_MOUNTEDTANK)
	{
		Com_Memset(hand, 0, sizeof(refEntity_t));
		CG_CalculateWeaponPosition(hand->origin, angles);
		AnglesToAxis(angles, hand->axis);
		hand->renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

		if (IS_MOUNTED_TANK_BROWNING(ps->clientNum))
		{
			hand->hModel = cgs.media.hMountedFPBrowning;
		}
		else
		{
			hand->hModel = cgs.media.hMountedFPMG42;
		}

		//gunoff[0] = cg_gun_x.value;
		//gunoff[1] = cg_gun_y.value;
		//gunoff[2] = cg_gun_z.value;

		gunoff[0] = 20;

		if (cg.time - cg.predictedPlayerEntity.muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			gunoff[0] += random() * 2.f;
		}

		VectorMA(hand->origin, gunoff[0], cg.refdef_current->viewaxis[0], hand->origin);
		VectorMA(hand->origin, -10, cg.refdef_current->viewaxis[1], hand->origin);
		VectorMA(hand->origin, (-8 + fovOffset), cg.refdef_current->viewaxis[2], hand->origin);

		CG_AddWeaponWithPowerups(hand, cg.predictedPlayerEntity.currentState.powerups, ps, &cg.predictedPlayerEntity);

		if (cg.time - cg.predictedPlayerEntity.overheatTime < 3000)
		{
			if (!(rand() % 3))
			{
				float alpha = 1.0f - ((float)(cg.time - cg.predictedPlayerEntity.overheatTime) / 3000.0f);

				alpha *= 0.25f;     // .25 max alpha
				CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, cg.tankflashorg, 1000, 8, 20, 30, alpha, 8.f);
			}
		}

		// FIXME: HACK dummy model to just draw _something_
		refEntity_t flash;

		Com_Memset(&flash, 0, sizeof(flash));
		flash.renderfx = (RF_LIGHTING_ORIGIN | RF_DEPTHHACK);
		flash.hModel   = cgs.media.mg42muzzleflash;

		angles[YAW]   = 0;
		angles[PITCH] = 0;
		angles[ROLL]  = crandom() * 10;
		AnglesToAxis(angles, flash.axis);

		CG_PositionRotatedEntityOnTag(&flash, hand, "tag_flash");

		VectorMA(flash.origin, 22, flash.axis[0], flash.origin);

		VectorCopy(flash.origin, cg.tankflashorg);

		if (cg.time - cg.predictedPlayerEntity.muzzleFlashTime < MUZZLE_FLASH_TIME)
		{
			trap_R_AddRefEntityToScene(&flash);
		}

		return;
	}

	if (ps->weapon > WP_NONE)
	{
		weapon = &cg_weapons[ps->weapon];

		Com_Memset(hand, 0, sizeof(refEntity_t));

		// set up gun position
		CG_CalculateWeaponPosition(hand->origin, angles);

		gunoff[0] = cg_gun_x.value;
		gunoff[1] = cg_gun_y.value;
		gunoff[2] = cg_gun_z.value;

		VectorMA(hand->origin, gunoff[0], cg.refdef_current->viewaxis[0], hand->origin);
		VectorMA(hand->origin, gunoff[1], cg.refdef_current->viewaxis[1], hand->origin);
		VectorMA(hand->origin, (gunoff[2] + fovOffset), cg.refdef_current->viewaxis[2], hand->origin);

		AnglesToAxis(angles, hand->axis);

		if (cg_gun_frame.integer)
		{
			hand->frame    = hand->oldframe = cg_gun_frame.integer;
			hand->backlerp = 0;
		}
		else      // get the animation state
		{
			if (cg.binocZoomTime)
			{
				if (cg.binocZoomTime < 0)
				{
					if (-cg.binocZoomTime + 500 + 200 < cg.time)
					{
						cg.binocZoomTime = 0;
					}
					else
					{
						if (-cg.binocZoomTime + 200 < cg.time)
						{
							CG_ContinueWeaponAnim(WEAP_ALTSWITCHFROM);
						}
						else
						{
							CG_ContinueWeaponAnim(WEAP_IDLE2);
						}
					}
				}
				else
				{
					if (cg.binocZoomTime + 500 < cg.time)
					{
						trap_SendConsoleCommand("+zoom\n");
						cg.binocZoomTime = 0;
						CG_ContinueWeaponAnim(WEAP_IDLE2);
					}
					else
					{
						CG_ContinueWeaponAnim(WEAP_ALTSWITCHTO);
					}
				}
			}

			CG_WeaponAnimation(ps, weapon, &hand->oldframe, &hand->frame, &hand->backlerp);
		}

		hand->hModel   = weapon->handsModel;
		hand->renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

		// adjust bazooka so it has bigger distance to our crosshair
		if (ps->weapon == WP_BAZOOKA)
		{
			hand->axis[0][0]       *= .8f;
			hand->axis[0][1]       *= .8f;
			hand->axis[0][2]       *= .8f;
			hand->nonNormalizedAxes = qtrue;
		}

		// add everything onto the hand
		CG_AddPlayerWeapon(hand, ps, &cg.predictedPlayerEntity);
	}
}

/**
 * @brief CG_AddSoundWeapon
 * @param[in] cent
 */
void CG_AddSoundWeapon(centity_t *cent)
{
	weaponInfo_t *weapon = &cg_weapons[cent->currentState.weapon];

	// add weapon ready sound
	cent->pe.lightningFiring = qfalse;

	if ((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound)
	{
		// lightning gun and guantlet make a different sound when fire is held down
		trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, weapon->firingSound, 255, 0);
		cent->pe.lightningFiring = qtrue;
	}
	else if (weapon->readySound)
	{
		trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, weapon->readySound, 255, 0);
	}

	if (cent->currentState.clientNum == cg.snap->ps.clientNum)
	{
		// tick sound to help the player 'count' in their head
		if (cg.predictedPlayerState.grenadeTimeLeft)
		{
			if (((cg.grenLastTime) % 1000) < ((cg.predictedPlayerState.grenadeTimeLeft) % 1000))
			{
				switch (cg.predictedPlayerState.grenadeTimeLeft / 1000)
				{
				case 3:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound[3], CHAN_LOCAL_SOUND);
					break;
				case 2:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound[2], CHAN_LOCAL_SOUND);
					break;
				case 1:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound[1], CHAN_LOCAL_SOUND);
					break;
				case 0:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound[0], CHAN_LOCAL_SOUND);
					break;
				}
			}

			cg.grenLastTime = cg.predictedPlayerState.grenadeTimeLeft;
		}
	}
}

/*
==============================================================================
WEAPON SELECTION
==============================================================================
*/

/**
 * @brief Check weapon for ammo
 * @param weapon - the weapon to check for ammo
 * @return qtrue if the weapon has ammo or don't need ammo, else qfalse if weapon has no ammo left
 */
static qboolean CG_WeaponHasAmmo(weapon_t weapon)
{
	// certain weapons don't have ammo
	if (/*!GetWeaponTableData(weapon)->useAmmo*/ (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MELEE) || weapon == WP_PLIERS)
	{
		return qtrue;
	}

	// check if the weapon still have ammo
	if (!(cg.predictedPlayerState.ammo[GetWeaponTableData(weapon)->ammoIndex]) &&
	    !(cg.predictedPlayerState.ammoclip[GetWeaponTableData(weapon)->clipIndex]))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Check weapon is selectable
 * @param weapon - the weapon to check if selectable
 * @return qtrue if the weapon is selectable, else qfalse if not
 */
qboolean CG_WeaponSelectable(int weapon)
{
	// allow the player to unselect all weapons
	//if(i == WP_NONE)
	//  return qtrue;

	// if the player use a mounted weapon, don't allow weapon selection
	if (BG_PlayerMounted(cg.predictedPlayerState.eFlags))
	{
		return qfalse;
	}

	// check if the selected weapon is available
	if (!(COM_BitCheck(cg.predictedPlayerState.weapons, weapon)))
	{
		return qfalse;
	}

	if (!CG_WeaponHasAmmo((weapon_t)weapon))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_WeaponIndex
 * @param[in] weapnum
 * @param[in,out] bank
 * @param[in,out] cycle
 * @return
 */
int CG_WeaponIndex(int weapnum, int *bank, int *cycle)
{
	static int bnk, cyc;

	if (weapnum <= 0 || weapnum >= WP_NUM_WEAPONS)
	{
		if (bank)
		{
			*bank = 0;
		}

		if (cycle)
		{
			*cycle = 0;
		}
		return 0;
	}

	for (bnk = 0; bnk < MAX_WEAP_BANKS_MP; bnk++)
	{
		for (cyc = 0; cyc < MAX_WEAPS_IN_BANK_MP; cyc++)
		{
			if (!weapBanksMultiPlayer[bnk][cyc])
			{
				break;
			}

			// found the current weapon
			if (weapBanksMultiPlayer[bnk][cyc] == weapnum)
			{
				if (bank)
				{
					*bank = bnk;
				}

				if (cycle)
				{
					*cycle = cyc;
				}
				return 1;
			}
		}
	}

	// failed to find the weapon in the table
	// probably an alternate

	return 0;
}

/**
 * @brief Pass in a bank and cycle and this will return the next valid weapon higher in the cycle.
 * if the weap passed in is above highest in a cycle (MAX_WEAPS_IN_BANK), this will safely loop around
 * @param[in] bank
 * @param[in] cycle
 * @return
 */
static int getNextWeapInBank(int bank, int cycle)
{
	cycle++;

	cycle = cycle % MAX_WEAPS_IN_BANK_MP;

	if (weapBanksMultiPlayer[bank][cycle])          // return next weapon in bank if there is one
	{
		return weapBanksMultiPlayer[bank][cycle];
	}

	// return first in bank
	return weapBanksMultiPlayer[bank][0];
}

/**
 * @brief getNextWeapInBankBynum
 * @param[in] weapnum
 * @return
 */
static int getNextWeapInBankBynum(int weapnum)
{
	int bank, cycle;

	if (!CG_WeaponIndex(weapnum, &bank, &cycle))
	{
		return weapnum;
	}

	return getNextWeapInBank(bank, cycle);
}

/**
 * @brief Pass in a bank and cycle and this will return the next valid weapon lower in the cycle.
 * if the weap passed in is the lowest in a cycle (0), this will loop around to the
 * top (MAX_WEAPS_IN_BANK-1) and start down from there looking for a valid weapon position
 * @param[in] bank
 * @param[in] cycle
 * @return
 */
static int getPrevWeapInBank(int bank, int cycle)
{
	cycle--;

	if (cycle < 0)
	{
		cycle = MAX_WEAPS_IN_BANK_MP - 1;
	}

	while (!weapBanksMultiPlayer[bank][cycle])
	{
		cycle--;

		if (cycle < 0)
		{
			cycle = MAX_WEAPS_IN_BANK_MP - 1;
		}
	}

	return weapBanksMultiPlayer[bank][cycle];
}

/**
 * @brief getPrevWeapInBankBynum
 * @param[in] weapnum
 * @return
 */
static int getPrevWeapInBankBynum(int weapnum)
{
	int bank, cycle;

	if (!CG_WeaponIndex(weapnum, &bank, &cycle))
	{
		return weapnum;
	}

	return getPrevWeapInBank(bank, cycle);
}

/**
 * @brief Pass in a bank and cycle and this will return the next valid weapon in a higher bank.
 * sameBankPosition: if there's a weapon in the next bank at the same cycle,
 * return that (colt returns thompson for example) rather than the lowest weapon
 * @param[in] bank
 * @param[in] cycle
 * @param[in] sameBankPosition
 * @return
 */
static int getNextBankWeap(int bank, int cycle, qboolean sameBankPosition)
{
	bank++;

	bank = bank % MAX_WEAP_BANKS_MP;

	if (sameBankPosition && weapBanksMultiPlayer[bank][cycle])
	{
		return weapBanksMultiPlayer[bank][cycle];
	}

	return weapBanksMultiPlayer[bank][0];
}

/**
 * @brief Pass in a bank and cycle and this will return the next valid weapon in a lower bank.
 * sameBankPosition: if there's a weapon in the prev bank at the same cycle,
 * return that (thompson returns colt for example) rather than the highest weapon
 * @param[in] bank
 * @param[in] cycle
 * @param[in] sameBankPosition
 * @return
 */
static int getPrevBankWeap(int bank, int cycle, qboolean sameBankPosition)
{
	bank--;

	if (bank < 0)          // don't go below 0, cycle up to top
	{
		bank += MAX_WEAP_BANKS_MP;
	}

	bank = bank % MAX_WEAP_BANKS_MP;

	if (sameBankPosition && weapBanksMultiPlayer[bank][cycle])
	{
		return weapBanksMultiPlayer[bank][cycle];
	}
	else    // find highest weap in bank
	{
		int i;

		for (i = MAX_WEAPS_IN_BANK_MP - 1; i >= 0; i--)
		{
			if (weapBanksMultiPlayer[bank][i])
			{
				return weapBanksMultiPlayer[bank][i];
			}
		}

		// if it gets to here, no valid weaps in this bank, go down another bank
		return getPrevBankWeap(bank, cycle, sameBankPosition);
	}
}

/**
 * @brief CG_FinishWeaponChange
 * @param[in] lastweap
 * @param[in] newweap
 */
void CG_FinishWeaponChange(int lastWeapon, int newWeapon)
{
	int newbank;

	if (cg.binocZoomTime)
	{
		return;
	}

	cg.mortarImpactTime = -2;

	if (lastWeapon != GetWeaponTableData(newWeapon)->weapAlts)
	{
		if (((GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_PISTOL) && !(GetWeaponTableData(newWeapon)->attributes & WEAPON_ATTRIBUT_SILENCED) && (cg.pmext.silencedSideArm & 1))
		    || ((GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_PISTOL) && (GetWeaponTableData(newWeapon)->attributes & WEAPON_ATTRIBUT_SILENCED) && !(cg.pmext.silencedSideArm & 1))
		    || ((GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_RIFLE) && (cg.pmext.silencedSideArm & 2))
		    || ((GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_RIFLENADE) && !(cg.pmext.silencedSideArm & 2)))
		{
			newWeapon       = GetWeaponTableData(newWeapon)->weapAlts;
			cg.weaponSelect = newWeapon;
		}
	}

	if (lastWeapon == WP_BINOCULARS && (cg.snap->ps.eFlags & EF_ZOOMING))
	{
		trap_SendConsoleCommand("-zoom\n");
	}

	cg.weaponSelectTime = cg.time;  // flash the weapon icon

	if (cg.newCrosshairIndex)
	{
		trap_Cvar_Set("cg_drawCrossHair", va("%d", cg.newCrosshairIndex - 1));
	}

	cg.newCrosshairIndex = 0;

	// remember which weapon in this bank was last selected so when cycling back
	// to this bank, that weap will be highlighted first
	if (CG_WeaponIndex(newWeapon, &newbank, NULL))
	{
		cg.lastWeapSelInBank[newbank] = newWeapon;
	}

	if (lastWeapon == newWeapon)       // no need to do any more than flash the icon
	{
		return;
	}

	// setup for a user call to CG_LastWeaponUsed_f()
	if (lastWeapon == cg.lastFiredWeapon)
	{
		// don't set switchback for some weaps...
		if (!(GetWeaponTableData(lastWeapon)->type & WEAPON_TYPE_SCOPED))
		{
			cg.switchbackWeapon = lastWeapon;
		}
		else
		{
			cg.switchbackWeapon = GetWeaponTableData(lastWeapon)->weapAlts;
		}
	}
	else
	{
		// if this ended up having the switchback be the same
		// as the new weapon, set the switchback to the prev
		// selected weapon will become the switchback
		if (cg.switchbackWeapon == newWeapon)
		{
			if (!(GetWeaponTableData(lastWeapon)->type & WEAPON_TYPE_SCOPED))
			{
				cg.switchbackWeapon = lastWeapon;
			}
			else
			{
				cg.switchbackWeapon = GetWeaponTableData(lastWeapon)->weapAlts;
			}
		}
		// this fixes cg.switchbackWeapon=0 after very first spawn and switching weapon for the first time
		else if (cg.switchbackWeapon == WP_NONE && CG_WeaponSelectable(lastWeapon)) // ensure last weapon is available
		{
			if (!(GetWeaponTableData(lastWeapon)->type & WEAPON_TYPE_SCOPED))
			{
				cg.switchbackWeapon = lastWeapon;
			}
			else
			{
				cg.switchbackWeapon = GetWeaponTableData(lastWeapon)->weapAlts;
			}
		}
	}

	cg.weaponSelect = newWeapon;
}

extern pmove_t cg_pmove;

/**
 * @brief For example, switching between WP_MAUSER and WP_SNIPERRIFLE
 */
void CG_AltWeapon_f(void)
{
	if (!cg.snap)
	{
		return;
	}

	// overload for spec mode when following
	if (((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
#ifdef FEATURE_MULTIVIEW
	    || cg.mvTotalClients > 0
#endif
	    )
	{
		trap_SendConsoleCommand("followprev");
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_RESPAWNED)
	{
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		return;
	}

	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return;
	}

	if (cg.snap->ps.pm_type == PM_DEAD)
	{
		return;
	}

	// don't allow switch when zooming with binocular not equipped,
	// due to the binocular mask which doesn't disappear when switching
	if (cg.zoomedBinoc && cg.weaponSelect != WP_BINOCULARS)
	{
		return;
	}

	if (cg.weaponSelect == WP_BINOCULARS)
	{
		/*if(cg.snap->ps.eFlags & EF_ZOOMING) {
		    trap_SendConsoleCommand( "-zoom\n" );
		} else {
		    trap_SendConsoleCommand( "+zoom\n" );
		}*/

		// don't allow zooming when prone moving (prevent fast zoom/unzoom)
		if (cg.predictedPlayerState.eFlags & EF_PRONE_MOVING)
		{
			return;
		}

		if (cg.snap->ps.eFlags & EF_ZOOMING)
		{
			trap_SendConsoleCommand("-zoom\n");
			cg.binocZoomTime = -cg.time;
		}
		else
		{
			if (!cg.binocZoomTime)
			{
				cg.binocZoomTime = cg.time;
			}
		}

		return;
	}

	// don't check for weapon with alternative weapon, instead check special action
	if (!GetWeaponTableData(cg.weaponSelect)->weapAlts)
	{
		if (cg_weapaltReloads.integer && GetWeaponTableData(cg.weaponSelect)->useClip)
		{
			//TODO: This is a horrible way of doing it but theres not other way atm.
			trap_SendConsoleCommand("+reload\n");
			trap_SendConsoleCommand("-reload\n");
		}
		// some alt vsays,
		// 0 - disabled
		// 1 - team
		// 2 - fireteam
		else if (cg_quickchat.integer)
		{
			char *cmd;

			if (cg_quickchat.integer == 2)
			{
				cmd = va("vsay_buddy -1 %s", CG_BuildSelectedFirteamString());
			}
			else
			{
				cmd = "vsay_team";
			}

			switch (cg.weaponSelect)
			{
			case WP_DYNAMITE:
				trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "FTExploreArea"));
				break;     //return;
			case WP_SMOKE_BOMB:
				switch ((rand() % 2))
				{
				case 0:
					trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "FTGoUndercover"));
					break;
				case 1:
					trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "FTInfiltrate"));
					break;
				}
				break;     //return;
			case WP_SMOKE_MARKER:
			case WP_GRENADE_LAUNCHER:
			case WP_GRENADE_PINEAPPLE:
				trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "FireInTheHole"));
				break;     //return;
			case WP_PLIERS:
				switch ((rand() % 3))
				{
				case 0:
					trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "CoverMe"));
					break;
				case 1:
					trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "NeedBackup"));
					break;
				case 2:
					trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "ClearPath"));
					break;
				}
				break;     //return;
			case WP_SATCHEL:
				trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "LetsGo"));
				break;     //return;
			case WP_MEDKIT:
			case WP_MEDIC_SYRINGE:
				trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "IamMedic"));
				break;     //return;
			case WP_AMMO:
				trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "IamFieldOps"));
				break;     // return;
			// add others ...
			//case WP_POISON_SYRINGE:
			//trap_SendConsoleCommand(va("cmd %s %s\n", cmd, "EnemyWeak"));
			//return;
			default:
				break;
			}
		}

		return;
	}

	// force pause so holding it down won't go too fast
	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return;
	}

	// don't allow another reloading when we're still swapping to prevent animation breaking
	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return;
	}

	// don't allow another weapon switch when we're still swapping alt weap, to prevent animation breaking
	// there we check the value of the animation to prevent any switch during raising and dropping alt weapon
	// until the animation is ended
	if ((cg.snap->ps.weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHFROM ||
	    (cg.snap->ps.weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHTO)
	{
		return;
	}

	// need ground for this
	if (GetWeaponTableData(cg.weaponSelect)->type & WEAPON_TYPE_SETTABLE)
	{
		if (GetWeaponTableData(cg.weaponSelect)->type & WEAPON_TYPE_MG)
		{
			if (!(cg.predictedPlayerState.eFlags & EF_PRONE))
			{
				return;
			}
		}
		else // mortar
		{
			int    contents;
			vec3_t point;

			if (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_NONE)
			{
				return;
			}

			if (!cg.predictedPlayerState.ammoclip[cg.weaponSelect])
			{
				return;
			}

			if (cg.predictedPlayerState.eFlags & EF_PRONE)
			{
				return;
			}

			if (cg_pmove.waterlevel == 3)
			{
				return;
			}

			// don't allow set if moving
			if (VectorLengthSquared(cg.snap->ps.velocity) != 0.f)
			{
				return;
			}

			// eurgh, need it here too else we play sounds :/
			point[0] = cg.snap->ps.origin[0];
			point[1] = cg.snap->ps.origin[1];
			point[2] = cg.snap->ps.origin[2] + cg.snap->ps.crouchViewHeight;
			contents = CG_PointContents(point, cg.snap->ps.clientNum);

			if (contents & MASK_WATER)
			{
				return;
			}
		}
	}
	else if (GetWeaponTableData(cg.weaponSelect)->type & WEAPON_TYPE_SCOPABLE)
	{
		// don't allow players switching to scoped weapon when prone moving
		if (cg.predictedPlayerState.eFlags & EF_PRONE_MOVING)
		{
			return;
		}
	}

	if ((!(GetWeaponTableData(GetWeaponTableData(cg.weaponSelect)->weapAlts)->type & WEAPON_TYPE_RIFLENADE) // allow alt switch (but for riflenade) even if out-of-ammo
	     && (COM_BitCheck(cg.predictedPlayerState.weapons, GetWeaponTableData(cg.weaponSelect)->weapAlts)))      // and ensure the alt weapon is there
	    || CG_WeaponSelectable(GetWeaponTableData(cg.weaponSelect)->weapAlts))                                       // or check if new weapon is valid (riflenade need ammo for switching to)
	{
		CG_FinishWeaponChange(cg.weaponSelect, GetWeaponTableData(cg.weaponSelect)->weapAlts);
	}
	else if (cg_weapaltReloads.integer)
	{
		//TODO: This is a horrible way of doing it but theres not other way atm.
		trap_SendConsoleCommand("+reload\n");
		trap_SendConsoleCommand("-reload\n");
	}
}

/**
 * @brief CG_NextWeap
 * @param[in] switchBanks - curweap is the last in a bank, 'qtrue' means go to the next available bank, 'qfalse' means loop to the head of the bank
 */
void CG_NextWeap(qboolean switchBanks)
{
	int      bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	int      num      = cg.weaponSelect;
	int      curweap  = cg.weaponSelect;
	qboolean nextbank = qfalse;     // need to switch to the next bank of weapons?
	int      i;

	if (GetWeaponTableData(curweap)->type & (WEAPON_TYPE_PISTOL | WEAPON_TYPE_RIFLE))
	{
		num = GetWeaponTableData(curweap)->weapAlts;
	}

	CG_WeaponIndex(curweap, &bank, &cycle);       // get bank/cycle of current weapon

	if (cg_cycleAllWeaps.integer || !switchBanks)
	{
		for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
		{
			num = getNextWeapInBankBynum(num);

			CG_WeaponIndex(num, NULL, &newcycle);         // get cycle of new weapon.  if it's lower than the original, then it cycled around

			if (switchBanks)
			{
				if (newcycle <= cycle)
				{
					nextbank = qtrue;
					break;
				}
			}
			else        // don't switch banks if you get to the end
			{
				if (num == curweap)        // back to start, just leave it where it is
				{
					return;
				}
			}

			if (CG_WeaponSelectable(num))
			{
				break;
			}

			if (GetWeaponTableData(num)->type & WEAPON_TYPE_RIFLE)
			{
				if (CG_WeaponSelectable(GetWeaponTableData(num)->weapAlts))
				{
					num = GetWeaponTableData(num)->weapAlts;
					break;
				}
			}
		}
	}
	else
	{
		nextbank = qtrue;
	}

	if (nextbank)
	{
		int j;

		for (i = 0; i < MAX_WEAP_BANKS_MP; i++)
		{
			if (cg_cycleAllWeaps.integer)
			{
				num = getNextBankWeap(bank + i, cycle, qfalse);     // cycling all weaps always starts the next bank at the bottom
			}
			else
			{
				if (cg.lastWeapSelInBank[bank + i + 1])
				{
					num = cg.lastWeapSelInBank[bank + i + 1];
				}
				else
				{
					num = getNextBankWeap(bank + i, cycle, qtrue);
				}
			}

			if (num == 0)
			{
				continue;
			}

			//if(num == WP_BINOCULARS) {
			//  continue;
			//}

			if (CG_WeaponSelectable(num))       // first entry in bank was selectable, no need to scan the bank
			{
				break;
			}

			if (GetWeaponTableData(num)->type & WEAPON_TYPE_RIFLE)
			{
				if (CG_WeaponSelectable(GetWeaponTableData(num)->weapAlts))
				{
					num = GetWeaponTableData(num)->weapAlts;
					break;
				}
			}

			CG_WeaponIndex(num, &newbank, &newcycle);     // get the bank of the new weap

			for (j = newcycle; j < MAX_WEAPS_IN_BANK_MP; j++)
			{
				num = getNextWeapInBank(newbank, j);

				//if(num == WP_BINOCULARS) {
				// continue;
				//}

				if (CG_WeaponSelectable(num))       // found selectable weapon
				{
					break;
				}

				if (GetWeaponTableData(num)->type & WEAPON_TYPE_RIFLE)
				{
					if (CG_WeaponSelectable(GetWeaponTableData(num)->weapAlts))
					{
						num = GetWeaponTableData(num)->weapAlts;
						break;
					}
				}

				num = 0;
			}

			if (num)     // a selectable weapon was found in the current bank
			{
				break;
			}
		}
	}

	CG_FinishWeaponChange(curweap, num);
}

/**
 * @brief CG_PrevWeap
 * @param[in] switchBanks - curweap is the last in a bank
             'qtrue'  - go to the next available bank
             'qfalse' - loop to the head of the bank
 */
void CG_PrevWeap(qboolean switchBanks)
{
	int      bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	int      num      = cg.weaponSelect;
	int      curweap  = cg.weaponSelect;
	qboolean prevbank = qfalse;     // need to switch to the next bank of weapons?
	int      i;

	if (((GetWeaponTableData(curweap)->type & WEAPON_TYPE_PISTOL) && (GetWeaponTableData(curweap)->attributes & WEAPON_ATTRIBUT_SILENCED))
	    || GetWeaponTableData(curweap)->type & WEAPON_TYPE_RIFLENADE)
	{
		num = GetWeaponTableData(curweap)->weapAlts;
	}

	CG_WeaponIndex(curweap, &bank, &cycle);       // get bank/cycle of current weapon

	// initially, just try to find a lower weapon in the current bank
	if (cg_cycleAllWeaps.integer || !switchBanks)
	{
		for (i = MAX_WEAPS_IN_BANK_MP; i >= 0; i--)
		{
			num = getPrevWeapInBankBynum(num);

			CG_WeaponIndex(num, NULL, &newcycle);         // get cycle of new weapon.  if it's greater than the original, then it cycled around

			if (switchBanks)
			{
				if (newcycle > (cycle - 1))
				{
					prevbank = qtrue;
					break;
				}
			}
			else        // don't switch banks if you get to the end
			{
				if (num == curweap)        // back to start, just leave it where it is
				{
					return;
				}
			}

			//if(num == WP_BINOCULARS) {
			//  continue;
			//}

			if (CG_WeaponSelectable(num))
			{
				break;
			}

			if (GetWeaponTableData(num)->type & WEAPON_TYPE_RIFLE)
			{
				if (CG_WeaponSelectable(GetWeaponTableData(num)->weapAlts))
				{
					num = GetWeaponTableData(num)->weapAlts;
					break;
				}
			}
		}
	}
	else
	{
		prevbank = qtrue;
	}

	// cycle to previous bank.
	//  if cycleAllWeaps: find highest weapon in bank
	//      else: try to find weap in bank that matches cycle position
	//          else: use base weap in bank

	if (prevbank)
	{
		int j;

		for (i = 0; i < MAX_WEAP_BANKS_MP; i++)
		{
			if (cg_cycleAllWeaps.integer)
			{
				num = getPrevBankWeap(bank - i, cycle, qfalse);     // cycling all weaps always starts the next bank at the bottom
			}
			else
			{
				num = getPrevBankWeap(bank - i, cycle, qtrue);
			}

			if (num == 0)
			{
				continue;
			}

			if (CG_WeaponSelectable(num))       // first entry in bank was selectable, no need to scan the bank
			{
				break;
			}

			if (GetWeaponTableData(num)->type & WEAPON_TYPE_RIFLE)
			{
				if (CG_WeaponSelectable(GetWeaponTableData(num)->weapAlts))
				{
					num = GetWeaponTableData(num)->weapAlts;
					break;
				}
			}

			CG_WeaponIndex(num, &newbank, &newcycle);     // get the bank of the new weap

			for (j = MAX_WEAPS_IN_BANK_MP; j > 0; j--)
			{
				num = getPrevWeapInBank(newbank, j);

				if (CG_WeaponSelectable(num))       // found selectable weapon
				{
					break;
				}

				if (GetWeaponTableData(num)->type & WEAPON_TYPE_RIFLE)
				{
					if (CG_WeaponSelectable(GetWeaponTableData(num)->weapAlts))
					{
						num = GetWeaponTableData(num)->weapAlts;
						break;
					}
				}

				num = 0;
			}

			if (num)     // a selectable weapon was found in the current bank
			{
				break;
			}
		}
	}

	CG_FinishWeaponChange(curweap, num);
}

/**
 * @brief CG_CheckCanSwitch
 * @return
 */
qboolean CG_CheckCanSwitch(void)
{
	if (!cg.snap)
	{
		return qfalse;
	}

	if (cg.snap->ps.pm_flags & PMF_RESPAWNED)
	{
		return qfalse;
	}

	// pause bug
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return qfalse;
	}

	if (cg.snap->ps.pm_type == PM_DEAD)
	{
		return qfalse;
	}

	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return qfalse;
	}

	// don't allow switch when zooming with binocular not equipped,
	// due to the binocular mask which doesn't disappear when switching
	if (cg.zoomedBinoc && cg.weaponSelect != WP_BINOCULARS)
	{
		return qfalse;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		return qfalse;
	}

	// force pause so holding it down won't go too fast
	if (cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer)
	{
		return qfalse;
	}

	if (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_SET)
	{
		return qfalse;
	}

	// don't try to switch when in the middle of reloading
	// cheatinfo:   The server actually would let you switch if this check were not
	//              present, but would discard the reload.  So the when you switched
	//              back you'd have to start the reload over.  This seems bad, however
	//              the delay for the current reload is already in effect, so you'd lose
	//              the reload time twice.  (the first pause for the current weapon reload,
	//              and the pause when you have to reload again 'cause you canceled this one)
	if (cg.snap->ps.weaponstate == WEAPON_RELOADING)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_ZoomRequired
 * @param[in] isNextWeap
 * @return
 */
qboolean CG_ZoomRequired(qboolean isNextWeap)
{
	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if (cg.zoomval != 0.f)
	{
		if (cg_useWeapsForZoom.integer == (isNextWeap ? 1 : 2))
		{
			CG_ZoomIn_f();
			return qtrue;
		}

		if (cg_useWeapsForZoom.integer == (isNextWeap ? 2 : 1))
		{
			CG_ZoomOut_f();
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_LastWeaponUsed_f
 */
void CG_LastWeaponUsed_f(void)
{
	if (!CG_CheckCanSwitch())
	{
		return;
	}

	if (!cg.switchbackWeapon)
	{
		cg.switchbackWeapon = cg.weaponSelect;
		return;
	}

	if (CG_WeaponSelectable(cg.switchbackWeapon))
	{
		CG_FinishWeaponChange(cg.weaponSelect, cg.switchbackWeapon);
	}
	else        // switchback no longer selectable, reset cycle
	{
		cg.switchbackWeapon = WP_NONE;
	}
}

/**
 * @brief CG_NextWeaponInBank_f
 */
void CG_NextWeaponInBank_f(void)
{
	if (CG_ZoomRequired(qtrue))
	{
		return;
	}

	if (!CG_CheckCanSwitch())
	{
		return;
	}

	CG_NextWeap(qfalse);
}

/**
 * @brief CG_PrevWeaponInBank_f
 */
void CG_PrevWeaponInBank_f(void)
{
	if (CG_ZoomRequired(qfalse))
	{
		return;
	}

	if (!CG_CheckCanSwitch())
	{
		return;
	}

	CG_PrevWeap(qfalse);
}

/**
 * @brief CG_NextWeapon_f
 */
void CG_NextWeapon_f(void)
{
	if (!cg.snap)
	{
		return;
	}

#ifdef FEATURE_MULTIVIEW
	// overload for MV clients
	if (cg.mvTotalClients > 0)
	{
		CG_mvToggleView_f();
		return;
	}
#endif

	if (CG_ZoomRequired(qtrue))
	{
		return;
	}

	if (!CG_CheckCanSwitch())
	{
		return;
	}

	CG_NextWeap(qtrue);
}

/**
 * @brief CG_PrevWeapon_f
 */
void CG_PrevWeapon_f(void)
{
	if (!cg.snap)
	{
		return;
	}

#ifdef FEATURE_MULTIVIEW
	// overload for MV clients
	if (cg.mvTotalClients > 0)
	{
		CG_mvSwapViews_f();
		return;
	}
#endif

	if (CG_ZoomRequired(qfalse))
	{
		return;
	}

	if (!CG_CheckCanSwitch())
	{
		return;
	}

	CG_PrevWeap(qtrue);
}

/**
 * @brief Weapon keys are not generally bound directly('bind 1 weapon 1'),
 * rather the key is bound to a given bank ('bind 1 weaponbank 1')
 */
void CG_WeaponBank_f(void)
{
	int newWeapon, i;
	int curbank = 0, curcycle = 0, bank = 0, cycle = 0;

	if (!CG_CheckCanSwitch())
	{
		return;
	}

	bank = Q_atoi(CG_Argv(1));

	if (bank <= 0 || bank >= MAX_WEAP_BANKS_MP)
	{
		return;
	}

	CG_WeaponIndex(cg.weaponSelect, &curbank, &curcycle);         // get bank/cycle of current weapon

	if (!cg.lastWeapSelInBank[bank])
	{
		newWeapon = weapBanksMultiPlayer[bank][0];
		cycle    -= 1; // cycle up to first weap
	}
	else
	{
		newWeapon = cg.lastWeapSelInBank[bank];
		CG_WeaponIndex(newWeapon, &bank, &cycle);

		if (bank != curbank)
		{
			cycle -= 1;
		}
	}

	for (i = 0; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
		newWeapon = getNextWeapInBank(bank, cycle + i);

		if (CG_WeaponSelectable(newWeapon))
		{
			break;
		}

		if (GetWeaponTableData(newWeapon)->type & WEAPON_TYPE_RIFLE)
		{
			if (CG_WeaponSelectable(GetWeaponTableData(newWeapon)->weapAlts))
			{
				newWeapon = GetWeaponTableData(newWeapon)->weapAlts;
				break;
			}
		}
	}

	if (i == MAX_WEAPS_IN_BANK_MP)
	{
		return;
	}

	CG_FinishWeaponChange(cg.weaponSelect, newWeapon);
}

/**
 * @brief CG_Weapon_f
 */
void CG_Weapon_f(void)
{
	int num;

	num = Q_atoi(CG_Argv(1));

	// weapon bind should execute weaponbank instead -- for splitting out class weapons, per Id request
	if (num < MAX_WEAP_BANKS_MP)
	{
		CG_WeaponBank_f();
	}
}

int weapBankSwitchOrder[MAX_WEAP_BANK_SWITCH_ORDER] = { 3, 2, 4, 1 };   // rifle, pistol, grenade, knife

/**
 * @brief The current weapon has just run out of ammo, switch to another one
 * @param[in] allowForceSwitch - enable on EV_NOAMMO and disable on EV_WEAPONSWITCHED
 */
void CG_OutOfAmmoChange(qboolean allowForceSwitch)
{
	int i, j;
	// int bank = 0, cycle = 0;

	// trivial switching
	if (cg.weaponSelect == WP_PLIERS || (cg.weaponSelect == WP_SATCHEL_DET && cg.predictedPlayerState.ammoclip[WP_SATCHEL_DET]))
	{
		return;
	}

	if (allowForceSwitch)
	{
		if ((cg.weaponSelect == WP_LANDMINE || cg.weaponSelect == WP_DYNAMITE) && CG_WeaponSelectable(WP_PLIERS))
		{
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, WP_PLIERS);
			return;
		}

		if (cg.weaponSelect == WP_SATCHEL && CG_WeaponSelectable(WP_SATCHEL_DET))
		{
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, WP_SATCHEL_DET);
			return;
		}

		if (GetWeaponTableData(cg.weaponSelect)->type & (WEAPON_TYPE_SET | WEAPON_TYPE_RIFLENADE))
		{
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, GetWeaponTableData(cg.weaponSelect)->weapAlts);
			return;
		}

		if ((GetWeaponTableData(cg.weaponSelect)->type & WEAPON_TYPE_PANZER) || cg.weaponSelect == WP_SMOKE_BOMB || cg.weaponSelect == WP_MEDIC_ADRENALINE)
		{
			for (i = 0; i < MAX_WEAP_BANK_SWITCH_ORDER; i++)
			{
				for (j = 0; j < MAX_WEAPS_IN_BANK_MP && weapBanksMultiPlayer[weapBankSwitchOrder[i]][j]; j++)
				{
					if (CG_WeaponSelectable(weapBanksMultiPlayer[weapBankSwitchOrder[i]][j]))
					{
						// make sure we don't reselect the panzer or bazooka
						if ((GetWeaponTableData(cg.weaponSelect)->type & WEAPON_TYPE_PANZER) && (GetWeaponTableData(weapBanksMultiPlayer[weapBankSwitchOrder[i]][j])->type & WEAPON_TYPE_PANZER))
						{
							continue;
						}

						CG_FinishWeaponChange(cg.predictedPlayerState.weapon, weapBanksMultiPlayer[weapBankSwitchOrder[i]][j]);
						return;
					}
				}
			}
		}

		// now try the opposite team's equivalent weap
		if (CG_WeaponSelectable(GetWeaponTableData(cg.weaponSelect)->weapEquiv))
		{
			CG_FinishWeaponChange(cg.predictedPlayerState.weapon, GetWeaponTableData(cg.weaponSelect)->weapEquiv);
			return;
		}
	}

	// more useful weapon changes -- check if rifle or pistol is still working, and use that if available
	for (i = 0; i < MAX_WEAP_BANK_SWITCH_ORDER; i++)
	{
		for (j = 0; j < MAX_WEAPS_IN_BANK_MP && weapBanksMultiPlayer[weapBankSwitchOrder[i]][j]; j++)
		{
			if (CG_WeaponSelectable(weapBanksMultiPlayer[weapBankSwitchOrder[i]][j]))
			{
				CG_FinishWeaponChange(cg.predictedPlayerState.weapon, weapBanksMultiPlayer[weapBankSwitchOrder[i]][j]);
				return;
			}
		}
	}

	// NOTE: never reached, as knife is always found
	/*
	// didn't have available alternative or equivalent, try another weap in the bank
	CG_WeaponIndex(cg.weaponSelect, &bank, &cycle);       // get bank/cycle of current weapon

	// otherwise just do something
	for (i = cycle; i < MAX_WEAPS_IN_BANK_MP; i++)
	{
	    equiv = getNextWeapInBank(bank, i);
	    if (CG_WeaponSelectable(equiv))        // found a reasonable replacement
	    {
	        cg.weaponSelect = equiv;
	        CG_FinishWeaponChange(cg.predictedPlayerState.weapon, cg.weaponSelect);
	        return;
	    }
	}
	// still nothing available, just go to the next
	// available weap using the regular selection scheme
	CG_NextWeap(qtrue);
	*/
}


/*
===================================================================================================
WEAPON EVENTS
===================================================================================================
*/

/**
 * @brief CG_MG42EFX
 * @param[in] cent
 */
void CG_MG42EFX(centity_t *cent)
{
	// complete overhaul of this one
	centity_t   *mg42;
	int         num;
	vec3_t      forward, point;
	refEntity_t flash;

	// find the mg42 we're attached to
	for (num = 0 ; num < cg.snap->numEntities ; num++)
	{
		mg42 = &cg_entities[cg.snap->entities[num].number];

		if (mg42->currentState.eType == ET_MG42_BARREL &&
		    mg42->currentState.otherEntityNum == cent->currentState.number)
		{
			// found it, clamp behind gun
			VectorCopy(mg42->currentState.pos.trBase, point);
			//AngleVectors (mg42->s.apos.trBase, forward, NULL, NULL);
			AngleVectors(cent->lerpAngles, forward, NULL, NULL);
			VectorMA(point, 40, forward, point);

			Com_Memset(&flash, 0, sizeof(flash));
			flash.renderfx = RF_LIGHTING_ORIGIN;
			flash.hModel   = cgs.media.mg42muzzleflash;

			VectorCopy(point, flash.origin);
			AnglesToAxis(cent->lerpAngles, flash.axis);

			trap_R_AddRefEntityToScene(&flash);

			// add dynamic light
			trap_R_AddLightToScene(flash.origin, 320, 1.25f + (rand() & 31) / 128.0f, 1.0f, 0.6f, 0.23f, 0, 0);
			return;
		}
	}
}

/**
 * @brief Right now mostly copied directly from Raf's MG42 FX, but with the optional addtion of smoke
 * @param[in] cent
 */
void CG_MortarEFX(centity_t *cent)
{
	if (cent->currentState.density & 1) // map mortar spawn flag
	{
		// smoke
		CG_ParticleImpactSmokePuff(cgs.media.smokePuffShader, cent->currentState.origin);
	}

	if (cent->currentState.density & 2) // map mortar spawn flag
	{
		refEntity_t flash;

		// muzzle flash
		Com_Memset(&flash, 0, sizeof(flash));
		flash.renderfx = RF_LIGHTING_ORIGIN;
		flash.hModel   = cgs.media.mg42muzzleflash;

		VectorCopy(cent->currentState.origin, flash.origin);
		AnglesToAxis(cg.refdefViewAngles, flash.axis);

		trap_R_AddRefEntityToScene(&flash);

		// add dynamic light
		trap_R_AddLightToScene(flash.origin, 320, 1.25f + (rand() & 31) / 128.0f, 1.0f, 1.0f, 1.0f, 0, 0);
	}
}

/**
 * @brief CG_WeaponFireRecoil
 * @param[in] weapon
 */
void CG_WeaponFireRecoil(int weapon)
{
	float  pitchAdd  = cg_weapons[weapon].fireRecoil[PITCH];
	float  yawRandom = cg_weapons[weapon].fireRecoil[YAW];
	vec3_t recoil;

	// FIXME: add recoil for secondary weapons?
	// if (GetWeaponTableData(weapon)->isPistol || GetWeaponTableData(weapon)->isSilencedPistol || GetWeaponTableData(weapon)->isAkimbo)
	// {
	//      pitchAdd = 2 + rand() % 3;
	// }

	if (GetWeaponTableData(weapon)->firingMode & WEAPON_FIRING_MODE_AUTOMATIC)
	{
		pitchAdd *= (1 + rand() % 3);
	}
	else if (CHECKBITWISE(GetWeaponTableData(weapon)->type, (WEAPON_TYPE_RIFLE | WEAPON_TYPE_SCOPED)))
	{
		// scoped weapon avoid yaw recoil (but FG42)
		yawRandom = 0;
	}

	// calc the recoil
	recoil[YAW]   = crandom() * yawRandom;
	recoil[ROLL]  = -recoil[YAW];   // why not
	recoil[PITCH] = -pitchAdd;

	// scale it up a bit (easier to modify this while tweaking)
	VectorScale(recoil, 30, recoil);

	// set the recoil
	VectorCopy(recoil, cg.kickAVel);
}

/**
 * @brief Caused by an EV_FIRE_WEAPON event
 * @param[in,out] cent
 */
void CG_FireWeapon(centity_t *cent)
{
	weaponInfo_t *weap;

	if (BG_PlayerMounted(cent->currentState.eFlags))
	{
		// quick hack for EF_MOUNTEDTANK, need to change this - likely it needs to use viewlocked as well
		if (cent->currentState.eFlags & EF_MOUNTEDTANK)
		{
			if (IS_MOUNTED_TANK_BROWNING(cent->currentState.number))
			{
				trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponSnd_2);
			}
			else
			{
				trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponSnd);
			}
		}
		else if (cent->currentState.eFlags & EF_AAGUN_ACTIVE)
		{
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hflakWeaponSnd);
		}
		else
		{
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponSnd);
		}

		if (cg_brassTime.integer > 0)
		{
			CG_MachineGunEjectBrass(cent);
		}

		if (cg_muzzleFlash.integer)
		{
			cent->muzzleFlashTime = cg.time;
		}
		else
		{
			cent->muzzleFlashTime = 0;
		}

		return;
	}

	if (cent->currentState.weapon == WP_NONE)
	{
		return;
	}

	if (cent->currentState.weapon >= WP_NUM_WEAPONS)
	{
		CG_Error("CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS\n");
	}

	weap = &cg_weapons[cent->currentState.weapon];

	if (cent->currentState.clientNum == cg.snap->ps.clientNum)
	{
		cg.lastFiredWeapon = cent->currentState.weapon;

		// kick angles
		CG_WeaponFireRecoil(cent->currentState.weapon);

		if (CHECKBITWISE(GetWeaponTableData(cent->currentState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET))
		{
			cg.mortarImpactTime        = -1;
			cg.mortarFireAngles[PITCH] = cg.predictedPlayerState.viewangles[PITCH];
			cg.mortarFireAngles[YAW]   = cg.predictedPlayerState.viewangles[YAW];
		}
	}

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	if (cg_muzzleFlash.integer)
	{
		cent->muzzleFlashTime = cg.time;
	}
	else
	{
		cent->muzzleFlashTime = 0;
	}

	// lightning gun only does this on initial press
	if (cent->currentState.weapon == WP_FLAMETHROWER && cent->pe.lightningFiring)
	{
		return;
	}

	// "throwing effort" when grenade is launched far enough
	if ((GetWeaponTableData(cent->currentState.weapon)->type & WEAPON_TYPE_GRENADE) && cent->currentState.apos.trBase[0] > 0)
	{
		return;
	}

	if (!(cent->currentState.eFlags & EF_ZOOMING))       // don't play sounds or eject brass if zoomed in
	{
		int         c             = weap->flashSound.count;
		sfxHandle_t firesound     = 0;
		sfxHandle_t fireEchosound = 0;

		if (c)
		{
			c = rand() % c;

			firesound     = weap->flashSound.sounds[c];
			fireEchosound = weap->flashEchoSound.sounds[c];
		}

		// try to use the lastShotSound, but don't assume it's there.
		// if a weapon without the sound calls it, keep regular fire sound
		if ((cent->currentState.event & ~EV_EVENT_BITS) == EV_FIRE_WEAPON_LASTSHOT)
		{
			c = weap->lastShotSound.count;

			if (c)
			{
				c = rand() % c;

				firesound     = weap->lastShotSound.sounds[c];
				fireEchosound = weap->flashEchoSound.sounds[c];
			}
		}

		if (firesound)
		{
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, firesound);

			if (fireEchosound)  // check for echo
			{
				vec3_t porg, gorg, norm;                    // player/gun origin
				float  gdist;

				VectorCopy(cent->currentState.pos.trBase, gorg);
				VectorCopy(cg.refdef_current->vieworg, porg);
				VectorSubtract(gorg, porg, norm);
				gdist = VectorNormalize(norm);

				if (gdist > 512 && gdist < 4096)                       // temp dist.  TODO: use numbers that are weapon specific
				{                   // use gorg as the new sound origin
					VectorMA(cg.refdef_current->vieworg, 64, norm, gorg);                         // sound-on-a-stick
					trap_S_StartSoundEx(gorg, cent->currentState.number, CHAN_WEAPON, fireEchosound, SND_NOCUT);
				}
			}
		}

		// do brass ejection
		if (weap->ejectBrassFunc && cg_brassTime.integer > 0)
		{
			weap->ejectBrassFunc(cent);
		}
	}
}

/**
 * @brief CG_AddSparks
 * @param[in] origin
 * @param[in] dir
 * @param[in] speed
 * @param[in] duration
 * @param[in] count
 * @param[in] randScale
 */
void CG_AddSparks(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale)
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

		le->leType        = LE_SPARK;
		le->startTime     = cg.time;
		le->endTime       = le->startTime + duration - (int)(0.5f * random() * duration);
		le->lastTrailTime = cg.time;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy(origin, le->pos.trBase);
		VectorMA(le->pos.trBase, 2 + random() * 4, dir, le->pos.trBase);
		VectorCopy(velocity, le->pos.trDelta);
		le->pos.trTime = cg.time;

		le->refEntity.customShader = cgs.media.sparkParticleShader;

		le->bounceFactor = 0.9f;

		//le->leBounceSoundType = LEBS_BLOOD;
		//le->leMarkType = LEMT_BLOOD;
	}
}

/**
 * @brief CG_AddBulletParticles
 * @param[in] origin
 * @param[in] dir
 * @param[in] speed
 * @param duration - unused
 * @param[in] count
 * @param[in] randScale
 */
void CG_AddBulletParticles(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale)
{
	vec3_t velocity, pos;
	int    i;

	// add the falling particles
	for (i = 0; i < count; i++)
	{
		VectorSet(velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale);
		VectorScale(velocity, (float)speed, velocity);

		VectorCopy(origin, pos);
		VectorMA(pos, 2 + random() * 4, dir, pos);

		CG_ParticleBulletDebris(pos, velocity, 300 + rand() % 300);
	}
}

/**
 * @brief CG_AddDirtBulletParticles
 * @param[in] origin
 * @param[in] dir
 * @param[in] speed
 * @param[in] duration
 * @param[in] count
 * @param[in] randScale
 * @param[in] width
 * @param[in] height
 * @param[in] alpha
 * @param[in] shader
 */
void CG_AddDirtBulletParticles(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale, float width, float height, float alpha, qhandle_t shader)
{
	vec3_t velocity, pos;
	int    i;

	// add the big falling particle
	VectorSet(velocity, 0, 0, (float)speed);
	VectorCopy(origin, pos);

	CG_ParticleDirtBulletDebris_Core(pos, velocity, duration, width, height, alpha, shader);  //600 + rand()%300 ); // keep central one

	for (i = 0; i < count; i++)
	{
		VectorSet(velocity, dir[0] * crandom() * speed * randScale, dir[1] * crandom() * speed * randScale, dir[2] * random() * speed);
		CG_ParticleDirtBulletDebris_Core(pos, velocity, duration + (rand() % (duration >> 1)), width, height, alpha, shader);
	}
}

/**
 * @brief CG_RandomDebris
 * @param[out] le
 */
void CG_RandomDebris(localEntity_t *le)
{
	int i = rand() % POSSIBLE_PIECES;

	if (i == 0)
	{
		le->refEntity.hModel  = cgs.media.debFabric[1];
		le->leBounceSoundType = LEBS_BONE;
	}
	else if (i == 1)
	{
		le->refEntity.hModel  = cgs.media.shardMetal1;
		le->leBounceSoundType = LEBS_METAL;
	}
	else if (i == 2)
	{
		le->refEntity.hModel  = cgs.media.shardMetal2;
		le->leBounceSoundType = LEBS_METAL;
	}
	else if (i == 3)
	{
		le->refEntity.hModel  = cgs.media.debRock[1];
		le->leBounceSoundType = LEBS_ROCK;
	}
	else if (i == 4)
	{
		le->refEntity.hModel  = cgs.media.debRock[0];
		le->leBounceSoundType = LEBS_ROCK;
	}
	else
	{
		le->refEntity.hModel  = cgs.media.debRock[2];
		le->leBounceSoundType = LEBS_ROCK;
	}
}

/**
 * @brief CG_AddDebris
 * @param[in] origin
 * @param[in] dir
 * @param[in] speed
 * @param[in] duration
 * @param[in] count
 * @param[in] trace
 */
void CG_AddDebris(vec3_t origin, vec3_t dir, int speed, int duration, int count, trace_t *trace)
{
	localEntity_t *le;
	refEntity_t   *re;
	vec3_t        velocity, unitvel;
	float         timeAdd;
	int           i, j;

	if (!cg_visualEffects.integer)
	{
		return;
	}

	for (i = 0; i < count; i++)
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet(unitvel, dir[0] + crandom() * 0.9f, dir[1] + crandom() * 0.9f, Q_fabs(dir[2]) > 0.5f ? dir[2] * (0.2f + 0.8f * random()) : random() * 0.6f);
		VectorScale(unitvel, (float)speed + (float)speed * 0.5f * crandom(), velocity);

		le->leType    = LE_DEBRIS;
		le->startTime = cg.time;
		// FIXME: this is such a waste - change to (or even drop the multiplicator *2)
		// le->endTime       = le->startTime + 2 * duration;
		// functions calling CG_AddDebris already creating randomnesses - adjust these and do a clean duration param
		le->endTime       = le->startTime + duration + (int)((float)duration * 0.8f * crandom());
		le->lastTrailTime = cg.time;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy(origin, le->pos.trBase);
		VectorCopy(velocity, le->pos.trDelta);
		le->pos.trTime = cg.time;

		timeAdd = 10.0f + random() * 40.0f;
		BG_EvaluateTrajectory(&le->pos, cg.time + (int)timeAdd, le->pos.trBase, qfalse, -1);

		le->bounceFactor = 0.5;
		le->effectWidth  = 5 + random() * 5;
		le->effectFlags |= 1;       // smoke trail

		//le->leMarkType = LEMT_BLOOD;

		// scale
		for (j = 0; j < 3; j++)
		{
			VectorScale(le->refEntity.axis[j], (rand() % 10 + 1) * .1f, le->refEntity.axis[j]);
		}

		// add model & properties of extended debris elements
		// FIXME: find better models and/or extend ...
		// FIXME: make dependant from surface (snow etc and use related models/sounds) and or weapon
		if (trace) // && user enabled
		{
			// airborn or solid with no surface set - just throw projectile fragments
			if (trace->fraction == 1.0f || ((trace->contents & CONTENTS_SOLID) && !trace->surfaceFlags))
			{
				if (rand() % 2)
				{
					le->refEntity.hModel = cgs.media.shardMetal1;  // FIXME: find some other models
				}
				else
				{
					le->refEntity.hModel = cgs.media.shardMetal2;
				}

				le->leBounceSoundType = LEBS_METAL;
				continue;
			}

			if (trace->surfaceFlags & SURF_WOOD)
			{
				le->refEntity.hModel  = cgs.media.debWood[rand() % 6];
				le->leBounceSoundType = LEBS_WOOD;
				continue;
			}

			if (trace->surfaceFlags & SURF_GRAVEL)
			{
				le->refEntity.hModel  = cgs.media.debRock[rand() % 3];
				le->leBounceSoundType = LEBS_ROCK;
				continue;
			}

			if (trace->surfaceFlags & SURF_METAL)
			{
				le->refEntity.hModel  = rand() % 2 ? cgs.media.shardMetal1 : cgs.media.shardMetal2;
				le->leBounceSoundType = LEBS_METAL;
				continue;
			}

			if (trace->surfaceFlags & SURF_CARPET)
			{
				le->refEntity.hModel  = cgs.media.debFabric[rand() % 3];
				le->leBounceSoundType = LEBS_WOOD;
				continue;
			}

			/*
			CG_Printf("--> c:%i sf:%i\n", trace->contents, trace->surfaceFlags);

			if (trace->surfaceFlags & SURF_SNOW)
			{
			    CG_Printf("ON SNOW\n");
			}

			if (trace->surfaceFlags & SURF_GRASS)
			{
			    CG_Printf("ON GRASS\n");
			}

			if (trace->surfaceFlags & SURF_CERAMIC)
			{
			    CG_Printf("ON CERAMIC\n");
			}

			// ---

			if (trace->surfaceFlags & SURF_GLASS)
			{
			    CG_Printf("ON GLASS\n");
			}

			if (trace->surfaceFlags & SURF_ROOF)
			{
			    CG_Printf("ON ROOF\n");
			}

			// --

			if (trace->contents & CONTENTS_WATER)
			{
			    CG_Printf("ON WATER\n");
			}

			if (trace->contents & CONTENTS_BODY)
			{
			    CG_Printf("ON BODY\n");
			}
			if (trace->contents & CONTENTS_CORPSE)
			{
			    CG_Printf("ON CORPSE\n");
			}
			*/

			// FIXME: replace with surface related models
			CG_RandomDebris(le);
		}
		else
		{
			// if there is no trace just throw random debris
			CG_RandomDebris(le);
		}
	}
}

/**
 * @brief CG_WaterRipple
 * @param[in] shader
 * @param[in] loc
 * @param[in] dir - unused
 * @param[in] size
 * @param[in] lifetime
 */
void CG_WaterRipple(qhandle_t shader, vec3_t loc, vec3_t dir, int size, int lifetime)
{
	localEntity_t *le = CG_AllocLocalEntity();
	refEntity_t   *re;

	le->leType  = LE_SCALE_FADE;
	le->leFlags = LEF_PUFF_DONT_SCALE;

	le->startTime = cg.time;
	le->endTime   = cg.time + lifetime;
	le->lifeRate  = 1.0f / (le->endTime - le->startTime);

	re = &le->refEntity;
	VectorCopy(loc, re->origin);
	re->shaderTime    = cg.time / 1000.0f;
	re->reType        = RT_SPLASH;
	re->radius        = size;
	re->customShader  = shader;
	re->shaderRGBA[0] = 0xff;
	re->shaderRGBA[1] = 0xff;
	re->shaderRGBA[2] = 0xff;
	re->shaderRGBA[3] = 0xff;
	le->color[3]      = 1.0f;
}

/**
 * @brief CG_GetSoundSurfaceIndex
 * @param[in] surfFlags
 * @return
 */
soundSurface_t CG_GetSoundSurfaceIndex(int surfFlags)
{
	soundSurface_t soundSurfaceIndex;

	for (soundSurfaceIndex = 0; soundSurfaceIndex < W_MAX_SND_SURF; soundSurfaceIndex++)
	{
		if (surfFlags & soundSurfaceTable[soundSurfaceIndex].surfaceType)
		{
			return soundSurfaceIndex;
		}
	}

	return W_SND_SURF_DEFAULT;
}

/**
 * @brief CG_GetRandomSoundSurface
 * @param[in] weapon
 * @param[in] surf
 * @return
 */
sfxHandle_t CG_GetRandomSoundSurface(weaponSounds_t *weaponSounds, soundSurface_t surf, qboolean forceDefault)
{
	int c = weaponSounds[surf].count;

	// if no sound found for given surface, force using default one if exist
	if (!c && forceDefault)
	{
		surf = W_SND_SURF_DEFAULT;
		c    = weaponSounds[surf].count;
	}

	if (c)
	{
		c = rand() % c;

		return weaponSounds[surf].sounds[c];
	}

	return 0;
}


/**
 * @brief CG_AddImpactParticles
 * @param[in] particleEffect
 * @param[in] missileEffect
 * @param[in] origin
 * @param[in] dir
 * @param[in] surfFlags
 */
static void CG_AddImpactParticles(impactParticle_t *particleEffect, int missileEffect, vec3_t origin, vec3_t dir, soundSurface_t surfFlags)
{
	if (missileEffect == PS_FX_WATER)
	{
		trace_t trace;
		vec3_t  tmpv;
		int     i = 0;

		VectorCopy(origin, tmpv);
		tmpv[2] += MAX_TRACE;

		trap_CM_BoxTrace(&trace, tmpv, origin, NULL, NULL, 0, MASK_WATER);

		// ripple
		CG_WaterRipple(cgs.media.wakeMarkShaderAnim, trace.endpos, dir, particleEffect->waterRippleRadius, particleEffect->waterRippleLifeTime);

		// particle
		for (i = 0; i < MAX_IMPACT_PARTICLE_EFFECT; i++)
		{
			impactParticleEffect_t *effect = &particleEffect->particleEffect[W_SND_SURF_WATER][i];

			if (!effect->particleEffectUsed)
			{
				break;
			}

			CG_AddDirtBulletParticles(trace.endpos, dir,
			                          (int)(effect->particleEffectSpeed + random() * effect->particleEffectSpeedRand),
			                          effect->particleEffectDuration,
			                          effect->particleEffectCount,
			                          effect->particleEffectRandScale,
			                          effect->particleEffectWidth,
			                          effect->particleEffectHeight,
			                          effect->particleEffectAlpha,
			                          effect->particleEffectShader);
		}

		// play a water splash
		if (cg_visualEffects.integer)
		{
			localEntity_t *le;
			le = CG_MakeExplosion(origin, dir, cgs.media.waterSplashModel,
			                      cgs.media.waterSplashShader,
			                      particleEffect->waterSplashDuration,
			                      particleEffect->waterSplashIsSprite);

			le->light = particleEffect->waterSplashLight;
			VectorCopy(particleEffect->waterSplashLightColor, le->lightColor);
		}
	}
	else if (missileEffect == PS_FX_COMMON)
	{
		trace_t trace;
		int     i;
		vec3_t  tmpv, tmpv2, sprOrg, sprVel;

		// explosion sprite animation
		VectorMA(origin, particleEffect->particleDirectionOffset, dir, sprOrg);
		VectorScale(dir, particleEffect->particleDirectionScaling, sprVel);

		VectorCopy(origin, tmpv);
		tmpv[2] += 20;
		VectorCopy(origin, tmpv2);
		tmpv2[2] -= 20;
		trap_CM_BoxTrace(&trace, tmpv, tmpv2, NULL, NULL, 0, MASK_SHOT);

		// particle
		for (i = 0; i < MAX_IMPACT_PARTICLE_EFFECT; i++)
		{
			if (particleEffect->particleEffect[surfFlags][i].particleEffectUsed)
			{
				impactParticleEffect_t *effect = &particleEffect->particleEffect[surfFlags][i];

				CG_AddDirtBulletParticles(trace.endpos, dir,
				                          (int)(effect->particleEffectSpeed + random() * effect->particleEffectSpeedRand),
				                          effect->particleEffectDuration,
				                          effect->particleEffectCount,
				                          effect->particleEffectRandScale,
				                          effect->particleEffectWidth,
				                          effect->particleEffectHeight,
				                          effect->particleEffectAlpha,
				                          effect->particleEffectShader);
			}
			else if (particleEffect->extraEffect[i].extraEffectUsed)
			{
				impactExtraEffect_t *effect = &particleEffect->extraEffect[i];
				int                 j, count;

				for (count = 0; count < effect->extraEffectCount; count++)
				{
					for (j = 0; j < 3; j++)
					{
						sprOrg[j] = origin[j] + effect->extraEffectOriginRand * crandom();
						sprVel[j] = effect->extraEffectVelocityRand * crandom();
					}

					VectorAdd(sprVel, trace.plane.normal, sprVel);
					VectorScale(sprVel, effect->extraEffectVelocityScaling, sprVel);
					CG_ParticleExplosion(effect->extraEffectShaderName,
					                     sprOrg,
					                     sprVel,
					                     (int)(effect->extraEffectDuration + random() * effect->extraEffectDurationRand),
					                     (int)(effect->extraEffectSizeStart + random() * effect->extraEffectSizeStartRand),
					                     (int)(effect->extraEffectSizeEnd + random() * effect->extraEffectSizeEndRand),
					                     effect->extraEffectLightAnim);
				}
			}
		}

		// explosion
		if (particleEffect->explosionShaderName[0] != 0)
		{
			CG_ParticleExplosion(particleEffect->explosionShaderName,
			                     sprOrg,
			                     sprVel,
			                     particleEffect->explosionDuration,
			                     (int)(particleEffect->explosionSizeStart + random() * particleEffect->explosionSizeStartRand),
			                     (int)(particleEffect->explosionSizeEnd + random() * particleEffect->explosionSizeEndRand),
			                     particleEffect->explosionLightAnim);
		}

		// debris
		if (particleEffect->debrisForBullet)
		{
			vec3_t o;
			VectorMA(origin, particleEffect->particleDirectionOffset, dir, o);
			CG_ParticleImpactSmokePuff(cgs.media.smokeParticleShader, o);

			CG_AddBulletParticles(origin, dir,
			                      (int)(particleEffect->debrisSpeed + random() * particleEffect->debrisSpeedRand),
			                      (int)(particleEffect->debrisDuration + random() * particleEffect->debrisDurationRand),
			                      (int)(particleEffect->debrisCount + random() * particleEffect->debrisCountExtra),
			                      1.0f);      // rand scale
		}
		else
		{
			CG_AddDebris(origin, dir,
			             (int)(particleEffect->debrisSpeed + random() * particleEffect->debrisSpeedRand),
			             (int)(particleEffect->debrisDuration + random() * particleEffect->debrisDurationRand),
			             (int)(particleEffect->debrisCount + random() * particleEffect->debrisCountExtra),
			             &trace);
		}
	}
}

/**
 * @brief Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
 * @param[in] weapon
 * @param[in] missileEffect used to define what sort of effect to spawn
 * @param[in] origin
 * @param[in] dir
 * @param[in] surfFlags
 * @param[in] sourceEnt
 *
 * @note modified to send missilehitwall surface parameters
 */
void CG_MissileHitWall(int weapon, int missileEffect, vec3_t origin, vec3_t dir, int surfFlags, int sourceEnt)
{
	soundSurface_t soundSurfaceIndex = W_SND_SURF_DEFAULT;
	qhandle_t      mark;
	sfxHandle_t    sfx, sfx2;
	int            markDuration;
	float          radius;
	qboolean       impactInRange;

	if (surfFlags & SURF_SKY)
	{
		return;
	}

	if (missileEffect == PS_FX_COMMON)
	{
		soundSurfaceIndex = CG_GetSoundSurfaceIndex(surfFlags);
	}
	else if (missileEffect == PS_FX_WATER)
	{
		soundSurfaceIndex = W_SND_SURF_WATER;
	}
	else if (missileEffect == PS_FX_FLESH)
	{
		soundSurfaceIndex = W_SND_SURF_FLESH;
	}

	sfx          = CG_GetRandomSoundSurface(cg_weapons[weapon].impactSound, soundSurfaceIndex, qtrue);
	sfx2         = CG_GetRandomSoundSurface(cg_weapons[weapon].impactSound, W_SND_SURF_FAR, qfalse);
	mark         = cg_weapons[weapon].impactMark[soundSurfaceIndex];
	radius       = cg_weapons[weapon].impactMarkRadius + crandom();
	markDuration = cg_markTime.integer * cg_weapons[weapon].impactDurationCoeff;

	// optimization, only spawn the bullet hole if we are close
	// enough to see it, this way we can leave other marks around a lot
	// longer, since most of the time we can't actually see the bullet holes
	// - small modification.  only do this for non-rifles (so you can see your shots hitting when you're zooming with a rifle scope)
	impactInRange = (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_SCOPED)
	                || cg_weapons[weapon].impactMarkMaxRange < 0 ||
	                (Distance(cg.refdef_current->vieworg, origin) < cg_weapons[weapon].impactMarkMaxRange);

	if (cg_weapons[weapon].impactParticle)
	{
		CG_AddImpactParticles(cg_weapons[weapon].impactParticle, missileEffect, origin, dir, soundSurfaceIndex);
	}

	// no mark found for given surface, force using default one if exist
	if (!mark)
	{
		mark = cg_weapons[weapon].impactMark[W_SND_SURF_DEFAULT];
	}

	if (sfx)
	{
		trap_S_StartSoundVControl(origin, sourceEnt, CHAN_AUTO, sfx, cg_weapons[weapon].impactSoundVolume);
	}

	// distant sounds for weapons with a broadcast fire sound (so you /always/ hear dynamite explosions)
	if (sfx2)
	{
		vec3_t porg, gorg, norm;    // player/gun origin
		float  gdist;

		VectorCopy(origin, gorg);
		VectorCopy(cg.refdef_current->vieworg, porg);
		VectorSubtract(gorg, porg, norm);
		gdist = VectorNormalize(norm);

		if (gdist > 1200 && gdist < 8000)      // 1200 is max cam shakey dist (2*600) use gorg as the new sound origin
		{
			VectorMA(cg.refdef_current->vieworg, cg_weapons[weapon].impactSoundRange, norm, gorg);           // non-distance falloff makes more sense; sfx2range was gdist*0.2
			// sfx2range is variable to give us minimum volume control different explosion sizes (see mortar, panzerfaust, and grenade)
			trap_S_StartSoundEx(gorg, sourceEnt, CHAN_WEAPON, sfx2, SND_NOCUT);
		}
	}

	// markDuration = cg_markTime.integer * x
	if (markDuration && impactInRange)
	{
		vec4_t projection;

		// omnidirectional explosion marks
		if (mark == cgs.media.burnMarkShader)
		{
			VectorSet(projection, 0, 0, -1);
			projection[3] = radius;

			trap_R_ProjectDecal(mark, 1, (vec3_t *) origin, projection, colorWhite, markDuration, (markDuration >> 4));
		}
		else if (mark)
		{
			vec3_t markOrigin;

			VectorSubtract(vec3_origin, dir, projection);
			projection[3] = radius * 32;
			VectorMA(origin, -16.0f, projection, markOrigin);
			// jitter markorigin a bit so they don't end up on an ordered grid
			markOrigin[0] += (random() - 0.5f);
			markOrigin[1] += (random() - 0.5f);
			markOrigin[2] += (random() - 0.5f);
			CG_ImpactMark(mark, markOrigin, projection, radius, random() * 360.0f, 1.0f, 1.0f, 1.0f, 1.0f, markDuration);
		}
	}
}

/**
 * @brief CG_MissileHitWallSmall
 * @param[in] origin
 * @param[in] dir
 */
void CG_MissileHitWallSmall(vec3_t origin, vec3_t dir)
{
	vec3_t        sprOrg, sprVel;
	static vec4_t projection = { 0, 0, -1.0f, 80.0f };     // {x,y,x,radius}

	// explosion sprite animation
	VectorMA(origin, 16, dir, sprOrg);
	VectorScale(dir, 64, sprVel);

	CG_ParticleExplosion("explode1", sprOrg, sprVel, 600, 6, 50, qtrue);

	// throw some debris
	CG_AddDebris(origin, dir, 280, 1400, 7 + rand() % 2, NULL);

	if (cgs.media.sfx_rockexp)
	{
		trap_S_StartSound(origin, -1, CHAN_AUTO, cgs.media.sfx_rockexp);
	}

	// impact mark
	if (cg_markTime.integer)
	{
		trap_R_ProjectDecal(cgs.media.burnMarkShader, 1, (vec3_t *) origin, projection, colorWhite, cg_markTime.integer, (cg_markTime.integer >> 4));
	}
}

/**
 * @brief CG_MissileHitPlayer
 * @param[in] entityNum
 * @param[in] weapon
 * @param[in] origin
 * @param[in] dir
 * @param[in] fleshEntityNum
 */
void CG_MissileHitPlayer(int entityNum, int weapon, vec3_t origin, vec3_t dir, int fleshEntityNum)
{
	CG_Bleed(origin, fleshEntityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood

	if (GetWeaponTableData(weapon)->type & (WEAPON_TYPE_GRENADE | WEAPON_TYPE_PANZER))
	{
		int effect;

		effect = (CG_PointContents(origin, 0) & CONTENTS_WATER) ? PS_FX_WATER : PS_FX_COMMON;

		CG_MissileHitWall(weapon, effect, origin, dir, 0, entityNum);               // like the old one
	}
	else if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_MELEE)
	{
		CG_MissileHitWall(weapon, PS_FX_FLESH, origin, dir, 0, entityNum);        // this one makes the hitting fleshy sound. whee
	}
}

/*
============================================================================
BULLETS
============================================================================
*/

/**
 * @brief CG_SpawnTracer
 * @param[in] sourceEnt
 * @param[in] pstart
 * @param[in] pend
 */
void CG_SpawnTracer(int sourceEnt, vec3_t pstart, vec3_t pend)
{
	localEntity_t *le;
	float         dist;
	vec3_t        dir;
	vec3_t        start, end;

	VectorCopy(pstart, start);
	VectorCopy(pend, end);

	// make MG42 tracers line up
	if (cg_entities[sourceEnt].currentState.eFlags & EF_MG42_ACTIVE) // FIXME: AAGUN
	{
		start[2] -= 42;
	}

	VectorSubtract(end, start, dir);
	dist = VectorNormalize(dir);

	if (dist < 2.0f * cg_tracerLength.value)
	{
		return; // segment isnt long enough, dont bother
	}

	if (sourceEnt < cgs.maxclients)
	{
		// for visual purposes, find the actual tag_weapon for this client
		// and offset the start and end accordingly
		if (!((cg_entities[sourceEnt].currentState.eFlags & EF_MG42_ACTIVE) || (cg_entities[sourceEnt].currentState.eFlags & EF_AAGUN_ACTIVE)))          // not MG42
		{
			orientation_t orientation;

			if (CG_GetWeaponTag(sourceEnt, "tag_flash", &orientation))
			{
				vec3_t ofs;

				VectorSubtract(orientation.origin, start, ofs);

				if (VectorLength(ofs) < 64)
				{
					VectorAdd(start, ofs, start);
				}
			}
		}
	}

	// subtract the length of the tracer from the end point, so we dont go through the end point
	VectorMA(end, -cg_tracerLength.value, dir, end);
	dist = VectorDistance(start, end);

	le            = CG_AllocLocalEntity();
	le->leType    = LE_MOVING_TRACER;
	le->startTime = cg.time - (cg.frametime ? (rand() % cg.frametime) / 2 : 0);
	le->endTime   = (int)(le->startTime + 1000.0f * dist / cg_tracerSpeed.value);

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = le->startTime;
	VectorCopy(start, le->pos.trBase);
	VectorScale(dir, cg_tracerSpeed.value, le->pos.trDelta);
}

/**
 * @brief CG_DrawTracer
 * @param[in] start
 * @param[in] finish
 */
void CG_DrawTracer(const vec3_t start, const vec3_t finish)
{
	vec3_t     forward, right;
	polyVert_t verts[4];
	vec3_t     line;

	VectorSubtract(finish, start, forward);

	line[0] = DotProduct(forward, cg.refdef_current->viewaxis[1]);
	line[1] = DotProduct(forward, cg.refdef_current->viewaxis[2]);

	VectorScale(cg.refdef_current->viewaxis[1], line[1], right);
	VectorMA(right, -line[0], cg.refdef_current->viewaxis[2], right);
	VectorNormalize(right);

	VectorMA(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0]       = 1;
	verts[0].st[1]       = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0]       = 1;
	verts[1].st[1]       = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0]       = 0;
	verts[2].st[1]       = 0;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0]       = 0;
	verts[3].st[1]       = 1;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);
}

/**
 * @brief CG_Tracer
 * @param[in] source
 * @param[in] dest
 * @param[in] sparks
 */
void CG_Tracer(const vec3_t source, const vec3_t dest, int sparks)
{
	float  len, begin, end;
	vec3_t start, finish;
	vec3_t forward;

	// tracer
	VectorSubtract(dest, source, forward);
	len = VectorNormalize(forward);

	// start at least a little ways from the muzzle
	if (len < 100 && !sparks)
	{
		return;
	}

	begin = 50 + random() * (len - 60);
	end   = begin + cg_tracerLength.value;

	if (end > len)
	{
		end = len;
	}

	VectorMA(source, begin, forward, start);
	VectorMA(source, end, forward, finish);

	CG_DrawTracer(start, finish);
}

/**
 * @brief CG_CalcMuzzlePoint
 * @param[in] entityNum
 * @param[in] muzzle
 * @return
 */
qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle)
{
	vec3_t    forward, right, up;
	centity_t *cent;

	if (entityNum == cg.snap->ps.clientNum)
	{
		// see if we're attached to a gun
		if (cg.snap->ps.eFlags & EF_MG42_ACTIVE)
		{
			centity_t *mg42 = &cg_entities[cg.snap->ps.viewlocked_entNum];
			vec3_t    forward;

			AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
			VectorMA(mg42->currentState.pos.trBase, 40, forward, muzzle);       // was -36, made 40 to be in sync with the actual muzzleflash drawing
			muzzle[2] += cg.snap->ps.viewheight;
		}
		else if (cg.snap->ps.eFlags & EF_AAGUN_ACTIVE)
		{
			centity_t *aagun = &cg_entities[cg.snap->ps.viewlocked_entNum];
			vec3_t    forward, right, up;

			AngleVectors(cg.snap->ps.viewangles, forward, right, up);
			VectorCopy(aagun->lerpOrigin, muzzle);                      // modelindex2 will already have been incremented on the server, so work out what it WAS then
			BG_AdjustAAGunMuzzleForBarrel(muzzle, forward, right, up, (aagun->currentState.modelindex2 + 3) % 4);
		}
		else if (cg.snap->ps.eFlags & EF_MOUNTEDTANK)
		{
			if (cg.renderingThirdPerson)
			{
				centity_t *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

				VectorCopy(tank->mountedMG42Flash.origin, muzzle);
				AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
				VectorMA(muzzle, 14, forward, muzzle);
			}
			else
			{
				// fix firstperson tank muzzle origin if drawgun is off
				if (cg_drawGun.integer != 1)
				{
					VectorCopy(cg.snap->ps.origin, muzzle);
					AngleVectors(cg.snap->ps.viewangles, forward, right, up);
					VectorMA(muzzle, 48, forward, muzzle);
					muzzle[2] += cg.snap->ps.viewheight;
					VectorMA(muzzle, 8, right, muzzle);
				}
				else
				{
					VectorCopy(cg.tankflashorg, muzzle);
				}
			}
		}
		else
		{
			VectorCopy(cg.snap->ps.origin, muzzle);
			muzzle[2] += cg.snap->ps.viewheight;
			AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);

			if (CHECKBITWISE(GetWeaponTableData(cg.snap->ps.weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
			{
				VectorMA(muzzle, 36, forward, muzzle);
			}
			else
			{
				VectorMA(muzzle, 14, forward, muzzle);
			}

			AddLean(cg.snap->ps.viewangles, muzzle, cg.snap->ps.leanf);
		}
		return qtrue;
	}

	cent = &cg_entities[entityNum];

	// entity is invalid this frame, don't use the old position value to calc muzzle position
	// otherwise the start position will be wrong
	if (!cent->currentValid)
	{
		return qfalse;
	}

	if (cent->currentState.eFlags & EF_MG42_ACTIVE)
	{
		if (cent->currentState.eType == ET_MG42_BARREL)
		{
			vec3_t forward;

			VectorCopy(cent->currentState.pos.trBase, muzzle);
			AngleVectors(cent->lerpAngles, forward, NULL, NULL);
			VectorMA(muzzle, 40, forward, muzzle);
			muzzle[2] += DEFAULT_VIEWHEIGHT;
		}
	}
	else if (cent->currentState.eFlags & EF_MOUNTEDTANK)
	{
		centity_t *tank = &cg_entities[cent->tagParent];

		VectorCopy(tank->mountedMG42Flash.origin, muzzle);
	}
	else if (cent->currentState.eFlags & EF_AAGUN_ACTIVE)
	{
		centity_t *aagun = NULL;
		int       num;

		// find the mg42 we're attached to
		for (num = 0; num < cg.snap->numEntities; num++)
		{
			aagun = &cg_entities[cg.snap->entities[num].number];

			if (aagun->currentState.eType == ET_AAGUN && aagun->currentState.otherEntityNum == cent->currentState.number)
			{
				// found it
				vec3_t forward, right, up;

				AngleVectors(cg.snap->ps.viewangles, forward, right, up);
				VectorCopy(aagun->lerpOrigin, muzzle);                      // modelindex2 will already have been incremented on the server, so work out what it WAS then
				BG_AdjustAAGunMuzzleForBarrel(muzzle, forward, right, up, (aagun->currentState.modelindex2 + 3) % 4);
			}
		}
	}
	else
	{
		VectorCopy(cent->currentState.pos.trBase, muzzle);

		AngleVectors(cent->currentState.apos.trBase, forward, right, up);

		if (cent->currentState.eFlags & EF_PRONE)
		{
			muzzle[2] += PRONE_VIEWHEIGHT;

			if (CHECKBITWISE(GetWeaponTableData(cent->currentState.weapon)->type, WEAPON_TYPE_MG | WEAPON_TYPE_SET))
			{
				VectorMA(muzzle, 36, forward, muzzle);
			}
			else
			{
				VectorMA(muzzle, 14, forward, muzzle);
			}
		}
		else
		{
			muzzle[2] += DEFAULT_VIEWHEIGHT;
			VectorMA(muzzle, 14, forward, muzzle);
			AddLean(cent->lerpAngles, muzzle, cent->pe.leanDirection);
		}
	}

	return qtrue;
}

/**
 * @brief SnapVectorTowards
 * @param v
 * @param to
 */
void SnapVectorTowards(vec3_t v, vec3_t to)
{
	int i;

	for (i = 0 ; i < 3 ; i++)
	{
		if (to[i] <= v[i])
		{
			v[i] = (float)floor((double)v[i]);
		}
		else
		{
			v[i] = (float)ceil((double)v[i]);
		}
	}
}

/**
 * @brief Renders bullet tracers if  tracer option is valid.
 * @param[in] pstart
 * @param[in] pend
 * @param[in] sourceEntityNum
 * @param[in] otherEntityNum
 */
void CG_DrawBulletTracer(vec3_t pstart, vec3_t pend, int sourceEntityNum, int otherEntityNum)
{
	if (cg_tracers.integer == 2 && sourceEntityNum != cg.clientNum)
	{
		return; // Only own tracers
	}

	if (cg_tracers.integer == 3 && sourceEntityNum == cg.clientNum)
	{
		return; // Only others tracers
	}

	if (sourceEntityNum >= 0 && sourceEntityNum != ENTITYNUM_NONE && cg_tracers.integer <= 3)
	{
		CG_SpawnTracer(sourceEntityNum, pstart, pend);
	}
}

/**
 * @brief Renders bullet effects.
 * @param[in] end
 * @param[in] sourceEntityNum
 * @param[in] flesh
 * @param[in] fleshEntityNum
 * @param[in] otherEntNum2
 * @param[in] waterfraction
 * @param[in] seed
 */
void CG_Bullet(int weapon, vec3_t end, int sourceEntityNum, qboolean flesh, int fleshEntityNum, int otherEntNum2, float waterfraction, int seed)
{
	trace_t    trace, trace2;
	vec3_t     dir;
	vec3_t     start = { 0, 0, 0 };
	static int lastBloodSpat;

	if (sourceEntityNum < 0 || sourceEntityNum >= MAX_GENTITIES)
	{
		return;
	}

	// don't ever shoot if we're binoced in
	if (cg_entities[sourceEntityNum].currentState.eFlags & EF_ZOOMING)
	{
		return;
	}

	// hack for fixed/mounted MG
	if (BG_PlayerMounted(cg_entities[sourceEntityNum].currentState.eFlags))
	{
		if (cg_entities[sourceEntityNum].currentState.eFlags & EF_MOUNTEDTANK)
		{
			if (IS_MOUNTED_TANK_BROWNING(sourceEntityNum))
			{
				weapon = WP_MOBILE_BROWNING;
			}
			else
			{
				weapon = WP_MOBILE_MG42;
			}
		}
		else if (cg_entities[sourceEntityNum].currentState.eFlags & EF_AAGUN_ACTIVE)
		{
			// TODO
		}
		else
		{
			weapon = WP_MOBILE_MG42;
		}
	}

	// snap tracers for MG42 to viewangle of client when antilag is enabled
	if (cgs.antilag && otherEntNum2 == cg.snap->ps.clientNum && (cg_entities[otherEntNum2].currentState.eFlags & EF_MG42_ACTIVE)) // FIXME: AAGUN
	{
		vec3_t  muzzle, forward, right, up;
		float   r, u;
		trace_t tr;

		AngleVectors(cg.predictedPlayerState.viewangles, forward, right, up);
		VectorCopy(cg_entities[cg.snap->ps.viewlocked_entNum].currentState.pos.trBase, muzzle);

		if (cg_entities[cg.snap->ps.viewlocked_entNum].currentState.onFireStart)
		{
			VectorMA(muzzle, 16, up, muzzle);
		}

		r = Q_crandom(&seed) * GetWeaponTableData(WP_DUMMY_MG42)->spread;
		u = Q_crandom(&seed) * GetWeaponTableData(WP_DUMMY_MG42)->spread;

		VectorMA(muzzle, MAX_TRACE, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		cg.bulletTrace = qtrue;
		CG_Trace(&tr, muzzle, NULL, NULL, end, otherEntNum2, MASK_SHOT);
		cg.bulletTrace = qfalse;

		SnapVectorTowards(tr.endpos, muzzle);
		VectorCopy(tr.endpos, end);
	}

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if (cg_tracerChance.value > 0)
	{
		if (CG_CalcMuzzlePoint(sourceEntityNum, start))
		{
			int sourceContentType;
			int destContentType;

			sourceContentType = CG_PointContents(start, 0);
			destContentType   = CG_PointContents(end, 0);

			// do a complete bubble trail if necessary
			if ((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER))
			{
				CG_BubbleTrail(start, end, .5, 8);
			}
			else if ((sourceContentType & CONTENTS_WATER))         // bubble trail from water into air
			{
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, .5, 8);
			}
			else if ((destContentType & CONTENTS_WATER))         // bubble trail from air into water
			{   // only add bubbles if effect is close to viewer
				if (Distance(cg.snap->ps.origin, end) < 1024)
				{
					trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
					CG_BubbleTrail(end, trace.endpos, .5, 8);
				}
			}

			if (cg_tracers.integer)
			{
#if 0
				vec3_t color = { 0, 0, 1 };

				CG_RailTrail(color, start, end, 0, 0);
#endif
				// if not flesh, then do a moving tracer
				if (flesh)
				{
					// draw a tracer
					if (random() < cg_tracerChance.value)
					{
						CG_Tracer(start, end, 0);
					}
				}
				else        // (not flesh)
				{
					CG_DrawBulletTracer(start, end, sourceEntityNum, otherEntNum2);
				}
			}
		}
	}

	// impact splash and mark
	if (flesh)
	{
		vec3_t    origin;
		float     rnd, tmpf;
		vec3_t    smokedir, tmpv, tmpv2;
		int       i, headshot;
		centity_t *cent = &cg_entities[fleshEntityNum];

		if (fleshEntityNum < MAX_CLIENTS)
		{
			CG_Bleed(end, fleshEntityNum);
		}

		// smoke puffs (sometimes with some blood)
		VectorSubtract(end, start, smokedir); // get a nice "through the body" vector
		VectorNormalize(smokedir);
		// all this to come up with a decent center-body displacement of bullet impact point
		VectorSubtract(cent->currentState.pos.trBase, end, tmpv);
		tmpv[2] = 0;
		tmpf    = VectorLength(tmpv);
		VectorScale(smokedir, tmpf, tmpv);
		VectorAdd(end, tmpv, origin);
		// whee, got a bullet impact point projected to center body
		CG_GetOriginForTag(cent, &cent->pe.headRefEnt, "tag_mouth", 0, tmpv, NULL);
		tmpv[2] += 5;
		VectorSubtract(tmpv, origin, tmpv2);
		headshot = (VectorLength(tmpv2) < 10);

		if (headshot && cg_blood.integer)
		{
			for (i = 0; i < 5; i++)
			{
				rnd = random();
				VectorScale(smokedir, 25.0f + random() * 25, tmpv);
				tmpv[0] += crandom() * 25.0f;
				tmpv[1] += crandom() * 25.0f;
				tmpv[2] += crandom() * 25.0f;
				CG_GetWindVector(tmpv2);
				VectorScale(tmpv2, 35, tmpv2); // was 75, before that 55
				tmpv2[2] = 0;
				VectorAdd(tmpv, tmpv2, tmpv);
				CG_SmokePuff(origin, tmpv, 5 + rnd * 10, 1, rnd * 0.8f, rnd * 0.8f, 0.5, 500 + (rand() % 800), cg.time, 0, 0, cgs.media.fleshSmokePuffShader);
			}
		}
		else
		{
			// puff out the front (more dust no blood)
			for (i = 0; i < 10; i++)
			{
				rnd = random();
				VectorScale(smokedir, -35.0f + random() * 25, tmpv);
				tmpv[0] += crandom() * 25.0f;
				tmpv[1] += crandom() * 25.0f;
				tmpv[2] += crandom() * 25.0f;
				CG_GetWindVector(tmpv2);
				VectorScale(tmpv2, 35, tmpv2); // was 75, before that 55
				tmpv2[2] = 0;
				VectorAdd(tmpv, tmpv2, tmpv);
				CG_SmokePuff(origin, tmpv, 5 + rnd * 10, rnd * 0.3f + 0.5f, rnd * 0.3f + 0.5f, rnd * 0.3f + 0.5f, 0.125f, 500 + (rand() % 300), cg.time, 0, 0, cgs.media.smokePuffShader);
			}
		}

		// play the bullet hit flesh sound
		CG_MissileHitWall(weapon, PS_FX_FLESH, cg_entities[fleshEntityNum].currentState.origin, smokedir, 0, fleshEntityNum);

		// if we haven't dropped a blood spat in a while, check if this is a good scenario
		if (cg_blood.integer && cg_bloodTime.integer && (lastBloodSpat > cg.time || lastBloodSpat < cg.time - 500))
		{
			if (CG_CalcMuzzlePoint(sourceEntityNum, start))
			{
				vec3_t trend;
				vec4_t projection;

				VectorSubtract(end, start, dir);
				VectorNormalize(dir);
				VectorMA(end, 128, dir, trend);
				trap_CM_BoxTrace(&trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY);

				if (trace.fraction < 1)
				{
					//CG_ImpactMark( cgs.media.bloodDotShaders[rand()%5], trace.endpos, trace.plane.normal, random()*360,
					//  1,1,1,1, qtrue, 15+random()*20, qfalse, cg_bloodTime.integer * 1000 );
#if 0
					VectorSubtract(vec3_origin, dir, projection);
					projection[3] = 64;
					VectorMA(trace.endpos, -8.0f, projection, markOrigin);
					CG_ImpactMark(cgs.media.bloodDotShaders[rand() % 5], markOrigin, projection, 15.0f + random() * 20.0f, 360.0f * random(),
					              1.0f, 1.0f, 1.0f, 1.0f, cg_bloodTime.integer * 1000);
#else
					VectorSet(projection, 0, 0, -1);
					projection[3] = 15.0f + random() * 20.0f;

					trap_R_ProjectDecal(cgs.media.bloodDotShaders[rand() % 5], 1, (vec3_t *) origin, projection, colorWhite,
					                    cg_bloodTime.integer * 1000, (cg_bloodTime.integer * 1000) >> 4);
#endif
					lastBloodSpat = cg.time;
				}
				else if (lastBloodSpat < cg.time - 1000)
				{
					// drop one on the ground?
					VectorCopy(end, trend);
					trend[2] -= 64;
					trap_CM_BoxTrace(&trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY);

					if (trace.fraction < 1)
					{
						//CG_ImpactMark( cgs.media.bloodDotShaders[rand()%5], trace.endpos, trace.plane.normal, random()*360,
						//  1,1,1,1, qtrue, 15+random()*10, qfalse, cg_bloodTime.integer * 1000 );
#if 0
						VectorSubtract(vec3_origin, dir, projection);
						projection[3] = 64;
						VectorMA(trace.endpos, -8.0f, projection, markOrigin);
						CG_ImpactMark(cgs.media.bloodDotShaders[rand() % 5], markOrigin, projection, 15.0f + random() * 10.0f, 360.0f * random(),
						              1.0f, 1.0f, 1.0f, 1.0f, cg_bloodTime.integer * 1000);
#else
						VectorSet(projection, 0, 0, -1);
						projection[3] = 15.0f + random() * 20.0f;

						trap_R_ProjectDecal(cgs.media.bloodDotShaders[rand() % 5], 1, (vec3_t *) origin, projection, colorWhite,
						                    cg_bloodTime.integer * 1000, (cg_bloodTime.integer * 1000) >> 4);
#endif
						lastBloodSpat = cg.time;
					}
				}
			}
		}
	}
	else        // (not flesh)
	{
		if (CG_CalcMuzzlePoint(sourceEntityNum, start) || cg.snap->ps.persistant[PERS_HWEAPON_USE])
		{
			if (waterfraction != 0.f)
			{
				vec3_t dist;
				vec3_t end2;
				vec3_t dir = { 0, 0, 1 };

				VectorSubtract(end, start, dist);
				VectorMA(start, waterfraction, dist, end2);

				CG_MissileHitWall(weapon, PS_FX_WATER, end2, dir, 0, -1);
				CG_MissileHitWall(weapon, PS_FX_COMMON, end, trace.plane.normal, 0, -1);
			}
			else
			{
				VectorSubtract(end, start, dir);
				VectorNormalizeFast(dir);
				VectorMA(end, 4, dir, end);

				cg.bulletTrace = qtrue;
				CG_Trace(&trace, start, NULL, NULL, end, 0, MASK_SHOT);

				// water check
				CG_Trace(&trace2, start, NULL, NULL, end, 0, MASK_WATER | MASK_SHOT);
				cg.bulletTrace = qfalse;

				if (trace.fraction != trace2.fraction)
				{
					cg.bulletTrace = qtrue;
					CG_Trace(&trace2, start, NULL, NULL, end, -1, MASK_WATER);
					cg.bulletTrace = qfalse;

					CG_MissileHitWall(weapon, PS_FX_WATER, trace2.endpos, trace2.plane.normal, trace2.surfaceFlags, -1);
					return;
				}

				// better bullet marks
				VectorSubtract(vec3_origin, dir, dir);

				CG_MissileHitWall(weapon, PS_FX_COMMON, trace.endpos, dir, trace.surfaceFlags, -1);
			}
		}
	}
}
