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
 * @file cg_ents.c
 * @brief Present snapshot entities, happens every single frame
 */

#include "cg_local.h"

/**
 * @brief Modifies the entities position and axis by the given
 * tag location
 * @param[in] entity
 * @param[in] parent
 * @param[in] tagName
 * @param[in] startIndex
 * @param[in] offset
 */
void CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, const char *tagName, int startIndex, vec3_t *offset)
{
	int           i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag(&lerped, parent, tagName, startIndex);

	// allow origin offsets along tag
	VectorCopy(parent->origin, entity->origin);

	if (offset)
	{
		VectorAdd(lerped.origin, *offset, lerped.origin);
	}

	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	MatrixMultiply(lerped.axis, parent->axis, entity->axis);
}

/**
 * @brief Modifies the entities position and axis by the given tag location
 * @param[in] entity
 * @param[in] parent
 * @param[in] tagName
 */
void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, const char *tagName)
{
	int           i;
	orientation_t lerped;
	vec3_t        tempAxis[3];

	// lerp the tag
	trap_R_LerpTag(&lerped, parent, tagName, 0);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, parent->axis, entity->axis);
}

/*
==========================================================================
FUNCTIONS CALLED EACH FRAME
==========================================================================
*/

/**
 * @brief Also called by event processing code
 * @param[in] cent
 */
void CG_SetEntitySoundPosition(centity_t *cent)
{
	if (cent->currentState.solid == SOLID_BMODEL)
	{
		vec3_t origin;
		float  *v = cgs.inlineModelMidpoints[cent->currentState.modelindex];

		VectorAdd(cent->lerpOrigin, v, origin);
		trap_S_UpdateEntityPosition(cent->currentState.number, origin);
	}
	else
	{
		trap_S_UpdateEntityPosition(cent->currentState.number, cent->lerpOrigin);
	}
}

#define LS_FRAMETIME 100 // (ms)  cycle through lightstyle characters at 10fps

/**
 * @brief CG_AddLightstyle
 * @param cent
 */
void CG_AddLightstyle(centity_t *cent)
{
	float lightval;
	int   cl;
	float r;
	int   stringlength;
	float offset;
	int   otime = cg.time - cent->dl_time;
	int   lastch, nextch;

	if (cent->dl_stylestring[0] == '\0')
	{
		return;
	}

	stringlength = strlen(cent->dl_stylestring);

	// it's been a long time since you were updated, lets assume a reset
	if (otime > 2 * LS_FRAMETIME)
	{
		otime             = 0;
		cent->dl_frame    = cent->dl_oldframe = 0;
		cent->dl_backlerp = 0;
	}

	cent->dl_time      = cg.time;
	offset             = ((float)otime) / LS_FRAMETIME;
	cent->dl_backlerp += offset;

	if (cent->dl_backlerp > 1)                         // we're moving on to the next frame
	{
		cent->dl_oldframe = cent->dl_oldframe + (int)cent->dl_backlerp;
		cent->dl_frame    = cent->dl_oldframe + 1;
		if (cent->dl_oldframe >= stringlength)
		{
			cent->dl_oldframe = cent->dl_oldframe % stringlength;
			if (cent->dl_oldframe < 3 && cent->dl_sound)     // < 3 so if an alarm comes back into the pvs it will only start a sound if it's going to be closely synced with the light, otherwise wait till the next cycle
			{
				trap_S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, CG_GetGameSound(cent->dl_sound));
			}
		}

		if (cent->dl_frame >= stringlength)
		{
			cent->dl_frame = (cent->dl_frame) % stringlength;
		}

		cent->dl_backlerp = cent->dl_backlerp - (int)cent->dl_backlerp;
	}

	lastch = cent->dl_stylestring[cent->dl_oldframe] - 'a';
	nextch = cent->dl_stylestring[cent->dl_frame] - 'a';

	lightval = (lastch * (1.0f - cent->dl_backlerp)) + (nextch * cent->dl_backlerp);

	// dlight values go from 0-1.5ish
#if 0
	lightval = (lightval * (1000.0f / 24.0f)) - 200.0f;     // they want 'm' as the "middle" value as 300
	lightval = MAX(0.0f, lightval);
	lightval = MIN(1000.0f, lightval);
#else
	lightval *= 0.071429;
	lightval  = MAX(0.0f, lightval);
	lightval  = MIN(20.0f, lightval);
#endif

	cl = cent->currentState.constantLight;
	r  = (cl & 0xFF) / 255.0f;

	// if the dlight has angles, then it is a directional global dlight
	if (cent->currentState.angles[0] != 0.f || cent->currentState.angles[1] != 0.f || cent->currentState.angles[2] != 0.f)
	{
		vec3_t normal;

		AngleVectors(cent->currentState.angles, normal, NULL, NULL);
		trap_R_AddLightToScene(normal, 256, lightval, r, r, r, 0, REF_DIRECTED_DLIGHT);
	}
	// normal global dlight
	else
	{
		trap_R_AddLightToScene(cent->lerpOrigin, 256, lightval, r, ((cl >> 8) & 0xFF) / 255.f, ((cl >> 16) & 0xFF) / 255.f, 0, 0);
	}
}

void CG_GetWindVector(vec3_t dir);

#define DEFAULT_SPEAKER_RANGE  1250
#define DEFAULT_SPEAKER_VOLUME  255

/**
 * @brief Add continuous entity effects, like local entity emission and lighting
 * @param[in,out] cent
 */
static void CG_EntityEffects(centity_t *cent)
{
	static vec3_t dir;

	// update sound origins
	CG_SetEntitySoundPosition(cent);

	// add loop sound
	if (cent->currentState.loopSound)
	{
		// allow looped sounds to start at trigger time
		if (cent->soundTime == 0)
		{
			cent->soundTime = trap_S_GetCurrentSoundTime();
		}

		if (cent->currentState.eType == ET_SPEAKER)
		{
			if (cent->currentState.dmgFlags)      // range is set
			{
				trap_S_AddRealLoopingSound(cent->lerpOrigin, vec3_origin, CG_GetGameSound(cent->currentState.loopSound), cent->currentState.dmgFlags, cent->currentState.onFireStart, cent->soundTime);
			}
			else
			{
				trap_S_AddRealLoopingSound(cent->lerpOrigin, vec3_origin, CG_GetGameSound(cent->currentState.loopSound), DEFAULT_SPEAKER_RANGE, cent->currentState.onFireStart, cent->soundTime);
			}
		}
		else if (cent->currentState.eType == ET_MOVER)
		{
			trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, CG_GetGameSound(cent->currentState.loopSound), cent->currentState.onFireStart, cent->soundTime);
		}
		else if (cent->currentState.solid == SOLID_BMODEL)
		{
			vec3_t origin;
			float  *v = cgs.inlineModelMidpoints[cent->currentState.modelindex];

			VectorAdd(cent->lerpOrigin, v, origin);
			trap_S_AddLoopingSound(origin, vec3_origin, CG_GetGameSound(cent->currentState.loopSound), cent->currentState.onFireStart, cent->soundTime);
		}
		else
		{
			trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, CG_GetGameSound(cent->currentState.loopSound), DEFAULT_SPEAKER_VOLUME, cent->soundTime);
		}
	}
	else if (cent->soundTime)
	{
		cent->soundTime = 0;
	}

	// constant light glow
	if (cent->currentState.constantLight)
	{
		if (cent->dl_stylestring[0] != 0)      // it's probably a dlight
		{
			CG_AddLightstyle(cent);
		}
		else
		{
			int cl = cent->currentState.constantLight;

			trap_R_AddLightToScene(cent->lerpOrigin, ((cl >> 24) & 0xFF) * 4, 1.0f, (cl & 0xFF) / 255.0f, ((cl >> 8) & 0xFF) / 255.0f, ((cl >> 16) & 0xFF) / 255.0f, 0, 0);
		}
	}

	// flaming sounds
	if (CG_EntOnFire(cent))
	{
		// play a flame blow sound when moving
		trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, cgs.media.flameBlowSound, (int)(DEFAULT_SPEAKER_VOLUME * (1.0f - Q_fabs(cent->fireRiseDir[2]))), 0);
		// play a burning sound when not moving
		trap_S_AddLoopingSound(cent->lerpOrigin, vec3_origin, cgs.media.flameSound, (int)(0.3 * DEFAULT_SPEAKER_VOLUME * (pow((double)cent->fireRiseDir[2], 2))), 0);
	}

	// overheating is a special effect
	if ((cent->currentState.eFlags & EF_OVERHEATING) == EF_OVERHEATING)
	{
		if (cent->overheatTime < (cg.time - 3000))
		{
			cent->overheatTime = cg.time;
		}
		if (!(rand() % 3))
		{
			float  alpha;
			vec3_t muzzle;

			if (CG_CalcMuzzlePoint((cent - cg_entities), muzzle))
			{
				muzzle[2] -= DEFAULT_VIEWHEIGHT;
			}
			else
			{
				VectorCopy(cent->lerpOrigin, muzzle);
			}
			alpha  = 1.0f - ((float) (cg.time - cent->overheatTime) / 3000.0f);
			alpha *= 0.25f;
			CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, muzzle, 1000, 8, 20, 30, alpha, 8.f);
		}
	}
	// If EF_SMOKING is set, emit smoke
	else if (cent->currentState.eFlags & EF_SMOKING)
	{
		if (cent->lastTrailTime < cg.time)
		{
			float rnd = random();

			cent->lastTrailTime = cg.time + 100;

			// use wind vector for smoke
			CG_GetWindVector(dir);
			VectorScale(dir, 20, dir); // was 75, before that 55
			if (dir[2] < 10)
			{
				dir[2] += 10;
			}
			//          dir[0] = crandom() * 10;
			//          dir[1] = crandom() * 10;
			//          dir[2] = 10 + rnd * 30;
			CG_SmokePuff(cent->lerpOrigin, dir, 15 + (random() * 10),
			             0.3f + rnd, 0.3f + rnd, 0.3f + rnd, 0.4f, 1500 + (rand() % 500),
			             cg.time, cg.time + 500, 0, cgs.media.smokePuffShader);
		}
	}
	// same thing but for smoking barrels instead of nasty server-side effect from single player
	else if (cent->currentState.eFlags & EF_SMOKINGBLACK)
	{
		if (cent->lastTrailTime < cg.time)
		{
			float rnd = random();

			cent->lastTrailTime = cg.time + 75;

			CG_GetWindVector(dir);
			VectorScale(dir, 50, dir); // was 75, before that 55
			if (dir[2] < 50)
			{
				dir[2] += 50;
			}

			CG_SmokePuff(cent->lerpOrigin, dir, 40 + random() * 70,  //40+(rnd*40),
			             rnd * 0.1f, rnd * 0.1f, rnd * 0.1f, 1.f, 2800 + (rand() % 4000),    //2500+(random()*1500),
			             cg.time, 0, 0, cgs.media.smokePuffShader);
		}
	}
}

/**
 * @brief CG_General
 * @param[in,out] cent
 */
static void CG_General(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex)
	{
		return;
	}

	Com_Memset(&ent, 0, sizeof(ent));

	// set frame
	ent.frame    = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	if (ent.frame)
	{
		ent.oldframe -= 1;
		ent.backlerp  = 1 - cg.frameInterpolation;

		if (cent->currentState.time)
		{
			ent.fadeStartTime = cent->currentState.time;
			ent.fadeEndTime   = cent->currentState.time2;
		}
	}

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum)
	{
		ent.renderfx |= RF_THIRD_PERSON;    // only draw from mirrors
	}

	if (cent->currentState.eType == ET_MG42_BARREL)
	{
		// grab angles from first person user or self if not
		// ATVI Wolfenstein Misc #469 - don't track until viewlocked
		if (cent->currentState.otherEntityNum == cg.snap->ps.clientNum && cg.snap->ps.viewlocked)
		{
			AnglesToAxis(cg.predictedPlayerState.viewangles, ent.axis);
		}
		else
		{
			AnglesToAxis(cent->lerpAngles, ent.axis);
		}
	}
	else if (cent->currentState.eType == ET_AAGUN)
	{
		// grab angles from first person user or self if not
		if (cent->currentState.otherEntityNum == cg.snap->ps.clientNum && cg.snap->ps.viewlocked)
		{
			AnglesToAxis(cg.predictedPlayerState.viewangles, ent.axis);
		}
		else
		{
			//AnglesToAxis( cg_entities[cent->currentState.otherEntityNum].lerpAngles, ent.axis );
			AnglesToAxis(cent->lerpAngles, ent.axis);
		}
	}
	else
	{
		// convert angles to axis
		AnglesToAxis(cent->lerpAngles, ent.axis);
	}

	// scale gamemodels
	if (cent->currentState.eType == ET_GAMEMODEL)
	{
		VectorScale(ent.axis[0], cent->currentState.angles2[0], ent.axis[0]);
		VectorScale(ent.axis[1], cent->currentState.angles2[1], ent.axis[1]);
		VectorScale(ent.axis[2], cent->currentState.angles2[2], ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;

		if (cent->currentState.apos.trType)
		{
			ent.reFlags |= REFLAG_ORIENT_LOD;
		}

		if (s1->torsoAnim)
		{
			if (cg.time >= cent->lerpFrame.frameTime)
			{
				cent->lerpFrame.oldFrameTime = cent->lerpFrame.frameTime;
				cent->lerpFrame.oldFrame     = cent->lerpFrame.frame;

				while (cg.time >= cent->lerpFrame.frameTime &&
				       // if teamNum == 1, then we are supposed to stop
				       // the animation when we reach the end of this loop
				       // clientNum already does this.
				       // - No, it does something a little different.
				       !(s1->teamNum == 1 &&
				         cent->lerpFrame.frame + s1->frame == s1->legsAnim + s1->torsoAnim))
				{
					cent->lerpFrame.frameTime += s1->weapon;
					cent->lerpFrame.frame++;

					if (cent->lerpFrame.frame >= s1->legsAnim + s1->torsoAnim)
					{
						if (s1->clientNum)
						{
							cent->lerpFrame.frame    = s1->legsAnim + s1->torsoAnim - 1;
							cent->lerpFrame.oldFrame = s1->legsAnim + s1->torsoAnim - 1;
						}
						else
						{
							cent->lerpFrame.frame = s1->legsAnim;
						}
					}
				}
			}

			if (cent->lerpFrame.frameTime == cent->lerpFrame.oldFrameTime)
			{
				cent->lerpFrame.backlerp = 0;
			}
			else
			{
				cent->lerpFrame.backlerp = 1.0f - (float)(cg.time - cent->lerpFrame.oldFrameTime) / (cent->lerpFrame.frameTime - cent->lerpFrame.oldFrameTime);
			}

			ent.frame = cent->lerpFrame.frame + s1->frame;  // offset
			if (ent.frame >= s1->legsAnim + s1->torsoAnim)
			{
				ent.frame -= s1->torsoAnim;
			}

			ent.oldframe = cent->lerpFrame.oldFrame + s1->frame;    // offset
			if (ent.oldframe >= s1->legsAnim + s1->torsoAnim)
			{
				ent.oldframe -= s1->torsoAnim;
			}

			ent.backlerp = cent->lerpFrame.backlerp;

			//CG_Printf( "Gamemodel: oldframe: %i frame: %i lerp: %f\n", ent.oldframe, ent.frame, ent.backlerp );
		}

		// only advance/change frame if the game model has not
		// been stopped (teamNum != 1)
		if (cent->trailTime && s1->teamNum != 1)
		{
			cent->lerpFrame.oldFrame = cent->lerpFrame.frame;
			cent->lerpFrame.frame    = s1->legsAnim;

			cent->lerpFrame.oldFrameTime = cent->lerpFrame.frameTime;
			cent->lerpFrame.frameTime    = cg.time;

			ent.oldframe = ent.frame;
			ent.frame    = s1->legsAnim;
			ent.backlerp = 0;
		}

		if (cent->nextState.animMovetype != s1->animMovetype)
		{
			cent->trailTime = 1;
		}
		else
		{
			cent->trailTime = 0;
		}

		if (s1->modelindex2)
		{
			ent.customSkin = cgs.gameModelSkins[s1->modelindex2];
		}
	}

	// special shader if under construction
	if (cent->currentState.powerups == STATE_UNDERCONSTRUCTION)
	{
		ent.customShader = cgs.media.genericConstructionShader;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	Com_Memcpy(&cent->refEnt, &ent, sizeof(refEntity_t));
}

/**
 * @brief Speaker entities can automatically play sounds
 * @param[in,out] cent
 */
static void CG_Speaker(centity_t *cent)
{
	if (!cent->currentState.clientNum)      // FIXME: use something other than clientNum...
	{
		return;     // not auto triggering
	}

	if (cg.time < cent->miscTime)
	{
		return;
	}

	trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, CG_GetGameSound(cent->currentState.eventParm));

	cent->miscTime = (int)(cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom());
}

/**
 * @brief CG_PlayerSeesItem
 * @param[in] ps
 * @param[in] item
 * @param[in] atTime
 * @param itemType - unused
 * @return
 */
qboolean CG_PlayerSeesItem(playerState_t *ps, entityState_t *item, int atTime, int itemType)
{
	vec3_t vorigin, eorigin, viewa, dir;
	float  dot, dist;

	BG_EvaluateTrajectory(&item->pos, atTime, eorigin, qfalse, item->effect2Time);

	VectorCopy(ps->origin, vorigin);
	vorigin[2] += ps->viewheight;           // get the view loc up to the viewheight
	//eorigin[2] += 8;                      // and subtract the item's offset (that is used to place it on the ground)

	VectorSubtract(vorigin, eorigin, dir);

	dist = VectorNormalize(dir);            // dir is now the direction from the item to the player

	if (dist > 255)
	{
		return qfalse;                      // only run the remaining stuff on items that are close enough

	}
	// FIXME: do this without AngleVectors.
	//      It'd be nice if the angle vectors for the player
	//      have already been figured at this point and I can
	//      just pick them up.  (if anybody is storing this somewhere,
	//      for the current frame please let me know so I don't
	//      have to do redundant calcs)
	AngleVectors(ps->viewangles, viewa, 0, 0);
	dot = DotProduct(viewa, dir);

	// give more range based on distance (the hit area is wider when closer)

	//foo = -0.94f - (dist/255.0f) * 0.057f;  // (ranging from -0.94 to -0.997) (it happened to be a pretty good range)
	//foo = -0.94f - (dist * (1.0f / 255.0f)) * 0.057f;       // (ranging from -0.94 to -0.997) (it happened to be a pretty good range)

	//Com_Printf("test: if(%f > %f) return qfalse (dot > foo)\n", dot, foo);
	if (dot > (-0.94f - (dist * (1.0f / 255.0f)) * 0.057f)) // foo
	{
		return qfalse;
	}

	return qtrue;
}

#define SIMPLEITEM_ICON_SIZE 24.f

/**
 * @brief CG_PlayerCanPickupWeapon
 * @param[in] clientNum
 * @param[in] weapon
 */
static int CG_PlayerCanPickupWeapon(int clientNum, weapon_t weapon)
{
	return BG_ClassHasWeapon(GetPlayerClassesData(TEAM_AXIS, cgs.clientinfo[clientNum].cls), weapon) ||
	       BG_ClassHasWeapon(GetPlayerClassesData(TEAM_ALLIES, cgs.clientinfo[clientNum].cls), weapon);
}

/**
 * @brief CG_Item
 * @param[in] cent
 */
static void CG_Item(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *es = &cent->currentState;
	gitem_t       *item;

	// (item index is stored in es->modelindex for item)
	if (es->modelindex >= ITEM_MAX_ITEMS)
	{
		CG_Error("Bad item index %i on entity\n", es->modelindex);
	}

	// if set to invisible, skip
	if (!es->modelindex || (es->eFlags & EF_NODRAW))
	{
		return;
	}

	item = BG_GetItem(es->modelindex);

	if (cg_simpleItems.integer == 1 || (cg_simpleItems.integer > 1 && item->giType != IT_TEAM))
	{
		polyVert_t   temp[4];
		polyVert_t   quad[4];
		qhandle_t    simpleItemShader = 0;
		weaponInfo_t *weaponInfo      = NULL;
		vec3_t       origin;
		vec4_t       accentColor;
		float        simpleItemScaleX = 1.f;
		float        simpleItemScaleY = 1.f;

		VectorCopy(cent->lerpOrigin, origin);
		VectorCopy(colorWhite, accentColor); // default white color

		switch (item->giType)
		{
		case IT_AMMO:
			weaponInfo = &cg_weapons[WP_AMMO];
			Vector4Set(accentColor, 1.f, 1.f, 0.09f, 0.5f);
			break;
		case IT_HEALTH:
			weaponInfo = &cg_weapons[WP_MEDKIT];
			Vector4Set(accentColor, 0.09f, 1.f, 0.09f, 0.5f);
			break;
		case IT_WEAPON:
			weaponInfo = &cg_weapons[item->giWeapon];
			// ammo box
			if (item->giWeapon == WP_AMMO)
			{
				Vector4Set(accentColor, 1.f, 1.f, 0.09f, 0.5f);
			}
			else
			{
				if (COM_BitCheck(cg.snap->ps.weapons, item->giWeapon) ||
				    (cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_SOLDIER && cgs.clientinfo[cg.snap->ps.clientNum].skill[SK_HEAVY_WEAPONS] >= 4 &&
				     (cgs.clientinfo[cg.snap->ps.clientNum].secondaryweapon == item->giWeapon)))
				{
					Vector4Set(accentColor, 1.f, 1.f, 0.09f, 1.f);
				}
				else if (CG_PlayerCanPickupWeapon(cg.snap->ps.clientNum, item->giWeapon))
				{
					Vector4Set(accentColor, 0.75f, 0.75f, 0.75f, 1.f);
				}
				else
				{
					Vector4Set(accentColor, 0.33f, 0.33f, 0.33f, 1.f);
				}
			}
			break;
		case IT_TEAM:
			simpleItemShader = cgs.media.objectiveSimpleIcon;
			origin[2]       += 5 + (float)sin((cg.time + 1000) * 0.005) * 3;
			if (item->giPowerUp == PW_BLUEFLAG)
			{
				Vector4Set(accentColor, 1.f, 0, 0, 1.f);
			}
			else if (item->giPowerUp == PW_REDFLAG)
			{
				Vector4Set(accentColor, 0, 0.5f, 1.f, 1.f);
			}
			break;
		case IT_BAD:
		default:
			return;
		}

		// reset color
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR &&
		    item->giType == IT_WEAPON && item->giWeapon != WP_AMMO)
		{
			Vector4Set(accentColor, 0.73f, 0.78f, 0.79f, 1.f);
		}

		// remove color when item is sinking
		if (item->giType != IT_TEAM && (es->time - 1000) < cg.time)
		{
			vec4_t fadeColor;
			Vector4Set(fadeColor, 0.33f, 0.33f, 0.33f, 1.f);
			VectorCopy(CG_LerpColorWithAttack(accentColor, fadeColor, (es->time - 1000), 1000, 0), accentColor);
		}

		if (weaponInfo)
		{
			// fallback to weapon icon
			simpleItemShader = weaponInfo->weaponSimpleIcon ? weaponInfo->weaponSimpleIcon : weaponInfo->weaponIcon[1];
			// custom simple item scale
			simpleItemScaleX = weaponInfo->weaponSimpleIconScale[0] > 0.f ? weaponInfo->weaponSimpleIconScale[0] : 1.f;
			simpleItemScaleY = weaponInfo->weaponSimpleIconScale[1] > 0.f ? weaponInfo->weaponSimpleIconScale[1] : 1.f;
		}

		if (simpleItemShader)
		{
			origin[2] += SIMPLEITEM_ICON_SIZE * simpleItemScaleY / 2.f;
			// build quad out of verts (inversed)
			VectorSet(temp[0].xyz, 0, 1 * simpleItemScaleX, 1 * simpleItemScaleY);
			VectorSet(temp[1].xyz, 0, -1 * simpleItemScaleX, 1 * simpleItemScaleY);
			VectorSet(temp[2].xyz, 0, -1 * simpleItemScaleX, -1 * simpleItemScaleY);
			VectorSet(temp[3].xyz, 0, 1 * simpleItemScaleX, -1 * simpleItemScaleY);
			// face quad towards viewer
			vec3_rotate(temp[0].xyz, cg.refdef_current->viewaxis, quad[0].xyz);
			vec3_rotate(temp[1].xyz, cg.refdef_current->viewaxis, quad[1].xyz);
			vec3_rotate(temp[2].xyz, cg.refdef_current->viewaxis, quad[2].xyz);
			vec3_rotate(temp[3].xyz, cg.refdef_current->viewaxis, quad[3].xyz);
			// position quad
			VectorMA(origin, SIMPLEITEM_ICON_SIZE / 2, quad[0].xyz, quad[0].xyz);
			VectorMA(origin, SIMPLEITEM_ICON_SIZE / 2, quad[1].xyz, quad[1].xyz);
			VectorMA(origin, SIMPLEITEM_ICON_SIZE / 2, quad[2].xyz, quad[2].xyz);
			VectorMA(origin, SIMPLEITEM_ICON_SIZE / 2, quad[3].xyz, quad[3].xyz);
			// set texture coords
			Vector2Set(quad[0].st, 0.f, 0.f);
			Vector2Set(quad[1].st, 1.f, 0.f);
			Vector2Set(quad[2].st, 1.f, 1.f);
			Vector2Set(quad[3].st, 0.f, 1.f);
			// set color modulation
			Vector4Scale(accentColor, 255.f, accentColor);
			Vector4Copy(accentColor, quad[0].modulate);
			Vector4Copy(accentColor, quad[1].modulate);
			Vector4Copy(accentColor, quad[2].modulate);
			Vector4Copy(accentColor, quad[3].modulate);

			// draw quad
			trap_R_AddPolyToScene(simpleItemShader, 4, quad);
			return;
		}

		// fallback to 3d model, in case no simple icon shader was found
	}

	Com_Memset(&ent, 0, sizeof(ent));

	ent.nonNormalizedAxes = qfalse;

	if (item->giType == IT_WEAPON)
	{
		weaponInfo_t *weaponInfo = &cg_weapons[item->giWeapon];

		if (weaponInfo->standModel)                              // first try to put the weapon on it's 'stand'
		{
			refEntity_t stand;

			Com_Memset(&stand, 0, sizeof(stand));
			stand.hModel = weaponInfo->standModel;

			if (es->eFlags & EF_SPINNING)
			{
				if (es->groundEntityNum == -1 || !es->groundEntityNum)     // spinning with a stand will spin the stand and the attached weap (only when in the air)
				{
					VectorCopy(cg.autoAnglesSlow, cent->lerpAngles);
					VectorCopy(cg.autoAnglesSlow, cent->lastLerpAngles);
				}
				else
				{
					VectorCopy(cent->lastLerpAngles, cent->lerpAngles);     // make a tossed weapon sit on the ground in a position that matches how it was yawed
				}
			}

			AnglesToAxis(cent->lerpAngles, stand.axis);
			VectorCopy(cent->lerpOrigin, stand.origin);

			// scale the stand to match the weapon scale ( the weapon will also be scaled inside CG_PositionEntityOnTag() )
			VectorScale(stand.axis[0], 1.5f, stand.axis[0]);
			VectorScale(stand.axis[1], 1.5f, stand.axis[1]);
			VectorScale(stand.axis[2], 1.5f, stand.axis[2]);

			if (cent->currentState.frame)
			{
				CG_PositionEntityOnTag(&ent, &stand, va("tag_stand%d", cent->currentState.frame), 0, NULL);
			}
			else
			{
				CG_PositionEntityOnTag(&ent, &stand, "tag_stand", 0, NULL);
			}

			VectorCopy(ent.origin, ent.oldorigin);
			ent.nonNormalizedAxes = qtrue;
		}
		else                                    // then default to laying it on it's side
		{
			if (weaponInfo->droppedAnglesHack)
			{
				cent->lerpAngles[2] += 90;
			}

			AnglesToAxis(cent->lerpAngles, ent.axis);

			// increase the size of the weapons when they are presented as items
			VectorScale(ent.axis[0], 1.5f, ent.axis[0]);
			VectorScale(ent.axis[1], 1.5f, ent.axis[1]);
			VectorScale(ent.axis[2], 1.5f, ent.axis[2]);
			ent.nonNormalizedAxes = qtrue;

			VectorCopy(cent->lerpOrigin, ent.origin);
			VectorCopy(cent->lerpOrigin, ent.oldorigin);

			if (es->eFlags & EF_SPINNING)      // spinning will override the angles set by a stand
			{
				if (es->groundEntityNum == -1 || !es->groundEntityNum)     // spinning with a stand will spin the stand and the attached weap (only when in the air)
				{
					VectorCopy(cg.autoAnglesSlow, cent->lerpAngles);
					VectorCopy(cg.autoAnglesSlow, cent->lastLerpAngles);
				}
				else
				{
					VectorCopy(cent->lastLerpAngles, cent->lerpAngles);     // make a tossed weapon sit on the ground in a position that matches how it was yawed
				}
			}
		}
	}
	else
	{
		AnglesToAxis(cent->lerpAngles, ent.axis);
		VectorCopy(cent->lerpOrigin, ent.origin);
		VectorCopy(cent->lerpOrigin, ent.oldorigin);

		if (es->eFlags & EF_SPINNING)      // spinning will override the angles set by a stand
		{
			VectorCopy(cg.autoAnglesSlow, cent->lerpAngles);
			AxisCopy(cg.autoAxisSlow, ent.axis);
		}
	}

	if (es->modelindex2)       // modelindex2 was specified for the ent, meaning it probably has an alternate model (as opposed to the one in the itemlist)
	{   // try to load it first, and if it fails, default to the itemlist model
		ent.hModel = cgs.gameModels[es->modelindex2];
	}
	else
	{
		if (item->giType == IT_WEAPON)
		{
			ent.hModel = cg_weapons[item->giWeapon].weaponModel[W_PU_MODEL].model;

			if (item->giWeapon == WP_AMMO)
			{
				if (cent->currentState.density == 2)
				{
					ent.customShader = cg_weapons[item->giWeapon].modModels[0];
				}
			}
		}
		else
		{
			ent.hModel = BG_GetItem(es->modelindex)->itemInfo.models[0];
		}
	}

	// find midpoint for highlight corona.
	// Can't do it when item is registered since it wouldn't know about replacement model
	if (!(cent->usehighlightOrigin))
	{
		vec3_t mins, maxs, offset;
		int    i;

		trap_R_ModelBounds(ent.hModel, mins, maxs);             // get bounds

		for (i = 0 ; i < 3 ; i++)
		{
			offset[i] = mins[i] + 0.5f * (maxs[i] - mins[i]);    // find object-space center
		}

		VectorCopy(cent->lerpOrigin, cent->highlightOrigin);    // set 'midpoint' to origin

		for (i = 0 ; i < 3 ; i++)                               // adjust midpoint by offset and orientation
		{
			cent->highlightOrigin[i] += offset[0] * ent.axis[0][i] +
			                            offset[1] * ent.axis[1][i] +
			                            offset[2] * ent.axis[2][i];
		}

		cent->usehighlightOrigin = qtrue;
	}

	// items without glow textures need to keep a minimum light value so they are always visible
	ent.renderfx |= RF_MINLIGHT;

	// highlighting items the player looks at
	if (cg_drawCrosshairPickups.integer)
	{
		qboolean highlight = qfalse;

		if (cg_drawCrosshairPickups.integer == 2)      // '2' is 'force highlights'
		{
			highlight = qtrue;
		}

		if (CG_PlayerSeesItem(&cg.predictedPlayerState, es, cg.time, item->giType))
		{
			highlight = qtrue;
		}

		// fixed item highlight fading
		if (highlight)
		{
			if (!cent->highlighted)
			{
				cent->highlighted   = qtrue;
				cent->highlightTime = cg.time;
			}
			ent.hilightIntensity = ((cg.time - cent->highlightTime) / 250.0f) * 1.0f;      // highlightFadeScale = 1.0f; / .25 sec to brighten up
		}
		else
		{
			if (cent->highlighted)
			{
				cent->highlighted   = qfalse;
				cent->highlightTime = cg.time;
			}
			ent.hilightIntensity = 1.0f - ((cg.time - cent->highlightTime) / 1000.0f) * 1.0f;     // highlightFadeScale = 1.0f; / 1 sec to dim down (diff in time causes problems if you quickly flip to/away from looking at the item)
		}

		if (ent.hilightIntensity < 0.25f)       // leave a minlight
		{
			ent.hilightIntensity = 0.25f;
		}
		if (ent.hilightIntensity > 1)
		{
			ent.hilightIntensity = 1.0;
		}
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

//============================================================================

/**
 * @brief CG_Smoker
 * @param[in,out] cent
 */
static void CG_Smoker(centity_t *cent)
{
	// this ent has some special setting up
	// time = speed
	// time2 = duration
	// angles2[0] = start_size
	// angles2[1] = end_size
	// angles2[2] = wait
	// dl_intensity = health
	// constantLight = delay
	// origin2 = normal to emit particles along

	if (cg.time - cent->highlightTime > cent->currentState.constantLight)
	{
		// FIXME: make this framerate independant?
		cent->highlightTime = cg.time;  // fire a particle this frame

		if (cent->currentState.modelindex2)
		{
			CG_ParticleSmoke(cgs.gameShaders[cent->currentState.modelindex2], cent);
		}
		else if (cent->currentState.density == 3)       // cannon
		{
			CG_ParticleSmoke(cgs.media.smokePuffShaderdirty, cent);
		}
		else // if (!(cent->currentState.density)) & others
		{
			CG_ParticleSmoke(cgs.media.smokePuffShader, cent);
		}
	}

	cent->lastTrailTime = cg.time;  // time we were last received at the client
}

static void CG_DrawLandmine(centity_t *cent, refEntity_t *ent)
{
	int color = (int)255 - (255 * fabs(sin(cg.time * 0.002)));

	if (cent->currentState.teamNum == TEAM_AXIS)
	{
		// red landmines
		ent->shaderRGBA[0] = 255;
		ent->shaderRGBA[1] = color;
		ent->shaderRGBA[2] = color;
		ent->shaderRGBA[3] = 255;
	}
	else
	{
		// blue landmines
		ent->shaderRGBA[0] = color;
		ent->shaderRGBA[1] = color;
		ent->shaderRGBA[2] = 255;
		ent->shaderRGBA[3] = 255;
	}

	ent->customShader = cgs.media.shoutcastLandmineShader;
}

/**
 * @brief CG_DrawMineMarkerFlag
 * @param[in] cent
 * @param[out] ent
 * @param[in] weapon
 */
static void CG_DrawMineMarkerFlag(centity_t *cent, refEntity_t *ent, const weaponInfo_t *weapon)
{
	entityState_t *s1 = &cent->currentState;

	ent->hModel = cent->currentState.teamNum == TEAM_AXIS ? weapon->modModels[1] : weapon->modModels[0];

	ent->origin[2]    += 8;
	ent->oldorigin[2] += 8;

	// 20 frames
	if (cg.time >= cent->lerpFrame.frameTime)
	{
		cent->lerpFrame.oldFrameTime = cent->lerpFrame.frameTime;
		cent->lerpFrame.oldFrame     = cent->lerpFrame.frame;

		while (cg.time >= cent->lerpFrame.frameTime)
		{
			cent->lerpFrame.frameTime += 50;    // 1000 / fps (which is 20)
			cent->lerpFrame.frame++;

			if (cent->lerpFrame.frame >= 20)
			{
				cent->lerpFrame.frame = 0;
			}
		}
	}

	if (cent->lerpFrame.frameTime == cent->lerpFrame.oldFrameTime)
	{
		cent->lerpFrame.backlerp = 0;
	}
	else
	{
		cent->lerpFrame.backlerp = 1.0f - (float)(cg.time - cent->lerpFrame.oldFrameTime) / (cent->lerpFrame.frameTime - cent->lerpFrame.oldFrameTime);
	}

	ent->frame = cent->lerpFrame.frame + s1->frame; // offset
	if (ent->frame >= 20)
	{
		ent->frame -= 20;
	}

	ent->oldframe = cent->lerpFrame.oldFrame + s1->frame;   // offset
	if (ent->oldframe >= 20)
	{
		ent->oldframe -= 20;
	}

	ent->backlerp = cent->lerpFrame.backlerp;
}

extern void CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi);
extern void CG_ScanForCrosshairMine(centity_t *cent);
extern void CG_ScanForCrosshairDyna(centity_t *cent);

/**
 * @brief CG_PlayerFloatText
 * @param[in] cent
 * @param[in] text
 * @param[in] height
 */
static void CG_EntityFloatText(centity_t *cent, const char *text, int height)
{
	vec3_t origin;

	VectorCopy(cent->lerpOrigin, origin);

	origin[2] += height;

	CG_AddOnScreenText(text, origin);
}

/**
 * @brief CG_Missile
 * @param[in] cent
 */
static void CG_Missile(centity_t *cent)
{
	refEntity_t        ent;
	entityState_t      *s1 = &cent->currentState;
	const weaponInfo_t *weapon;

	if (s1->weapon > WP_NUM_WEAPONS)
	{
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	if (s1->weapon == WP_SMOKE_BOMB)
	{
		// the smoke effect
		CG_RenderSmokeGrenadeSmoke(cent, weapon);
	}
	else if (s1->weapon == WP_SATCHEL && s1->clientNum == cg.snap->ps.clientNum)
	{
		// use snap client number so that the detonator works right when spectating (#218)
		cg.satchelCharge = cent;
	}
	else if (s1->weapon == WP_ARTY && s1->otherEntityNum2 && s1->teamNum == cgs.clientinfo[cg.clientNum].team)
	{
		VectorCopy(cent->lerpOrigin, cg.artilleryRequestPos[s1->clientNum]);
		cg.artilleryRequestTime[s1->clientNum] = cg.time;
	}

	// add trails
	if (cent->currentState.eType == ET_RAMJET)
	{
		CG_RocketTrail(cent, NULL);
	}
	else if (weapon->missileTrailFunc)
	{
		weapon->missileTrailFunc(cent, weapon);
	}

	// add dynamic light
	if (weapon->missileDlight != 0.f)
	{
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, 1.0,
		                       weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2], 0, 0);
	}

	// whoops, didn't mean to check it in with the missile flare

	// add missile sound
	if (weapon->missileSound)
	{
		if (GetWeaponTableData(cent->currentState.weapon)->type & WEAPON_TYPE_RIFLENADE)
		{
			if (!cent->currentState.effect1Time)
			{
				int flytime = cg.time - cent->currentState.pos.trTime;

				if (flytime > 300)
				{
					// have a quick fade in so we don't have a pop
					vec3_t velocity;
					int    volume = flytime > 375 ? 255 : (int)((75.f / ((float)flytime - 300.f)) * 255);

					BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity, qfalse, -1);
					trap_S_AddLoopingSound(cent->lerpOrigin, velocity, weapon->missileSound, volume, 0);
				}
			}
		}
		else
		{
			vec3_t velocity;

			BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity, qfalse, -1);
			trap_S_AddLoopingSound(cent->lerpOrigin, velocity, weapon->missileSound, 255, 0);
		}
	}

	// Don't tick until armed
	if (cent->currentState.weapon == WP_DYNAMITE)
	{
		if (cent->currentState.effect1Time)
		{
			vec3_t velocity;
			char   *timer;

			BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity, qfalse, -1);
			trap_S_AddLoopingSound(cent->lerpOrigin, velocity, weapon->spindownSound, 255, 0);

			if (cgs.clientinfo[cg.snap->ps.clientNum].team == cent->currentState.teamNum)
			{
				CG_ScanForCrosshairDyna(cent);
			}

			// add dynamite counter to floating string list
			if (cgs.clientinfo[cg.clientNum].shoutcaster)
			{
				timer = va("%i", 30 - (cg.time - cent->currentState.effect1Time) / 1000);
				CG_EntityFloatText(cent, timer, 8);
			}
		}
	}

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;

	if (cent->currentState.eType == ET_FLAMEBARREL)
	{
		ent.hModel = cgs.media.flamebarrel;
	}
	else if (cent->currentState.eType == ET_RAMJET)
	{
		ent.hModel = 0;
		// ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	}
	else
	{
		ent.hModel = weapon->missileModel;

		if (cent->currentState.teamNum == TEAM_ALLIES)
		{
			ent.customSkin = weapon->missileAlliedSkin;
		}
		else if (cent->currentState.teamNum == TEAM_AXIS)
		{
			ent.customSkin = weapon->missileAxisSkin;
		}
	}
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	if (cent->currentState.weapon == WP_LANDMINE)
	{
		// shoutcasters can see armed landmines
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && !cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			return;
		}

		VectorCopy(ent.origin, ent.lightingOrigin);
		ent.renderfx |= RF_LIGHTING_ORIGIN;

		if (cent->currentState.effect1Time == 1)
		{
			ent.origin[2]    -= 8;
			ent.oldorigin[2] -= 8;

			if (cgs.clientinfo[cg.snap->ps.clientNum].team != cent->currentState.teamNum)
			{
				if (cent->currentState.density - 1 == cg.snap->ps.clientNum)
				{
					ent.customShader = cgs.media.genericConstructionShader;
				}
				else if (cgs.clientinfo[cg.clientNum].shoutcaster)
				{
					if (cent->currentState.modelindex2)
					{
						// team or spotted landmine
						CG_DrawMineMarkerFlag(cent, &ent, weapon);
					}
					else
					{
						// unspotted landmine
						CG_DrawLandmine(cent, &ent);
					}
				}
				else if (!cent->currentState.modelindex2)
				{
					// see if we have the skill to see them and are close enough
					if (cgs.clientinfo[cg.snap->ps.clientNum].skill[SK_BATTLE_SENSE] >= 4)
					{
						vec_t distSquared = DistanceSquared(cent->lerpOrigin, cg.predictedPlayerEntity.lerpOrigin);

						if (distSquared > Square(256))
						{
							return;
						}
						else
						{
							ent.customShader = cgs.media.genericConstructionShader;
						}
					}
					else
					{
						return;
					}
				}
				else
				{
					CG_DrawMineMarkerFlag(cent, &ent, weapon);
				}
			}
			else
			{
				if (cgs.clientinfo[cg.clientNum].shoutcaster)
				{
					// shoutcasters can see team landmines
					CG_DrawMineMarkerFlag(cent, &ent, weapon);
				}
				else
				{
					CG_DrawMineMarkerFlag(cent, &ent, weapon);
					CG_ScanForCrosshairMine(cent);
				}
			}
		}

		if (cent->currentState.effect1Time == 2)
		{
			ent.origin[2]    -= 8;
			ent.oldorigin[2] -= 8;
		}
	}

	// convert direction of travel into axis
	// reverted to vanilla behaviour
	// FIXME: check FEATURE_EDV weapon cam (see else condition below)
	if (/*GetWeaponTableData(cent->currentState.weapon)->type & (WEAPON_TYPE_RIFLENADE | WEAPON_TYPE_PANZER | WEAPON_TYPE_GRENADE)
	    &&*/(CHECKBITWISE(GetWeaponTableData(cent->currentState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET)
	     || cent->currentState.weapon == WP_MAPMORTAR /*|| cent->currentState.weapon == WP_SMOKE_MARKER
	        || cent->currentState.weapon == WP_SMOKE_BOMB || cent->currentState.weapon == WP_DYNAMITE*/))
	{
		vec3_t delta;

		if (VectorCompare(cent->rawOrigin, vec3_origin))
		{
			VectorSubtract(cent->lerpOrigin, s1->pos.trBase, delta);
			VectorCopy(cent->lerpOrigin, cent->rawOrigin);
		}
		else
		{
			VectorSubtract(cent->lerpOrigin, cent->rawOrigin, delta);
			if (!VectorCompare(cent->lerpOrigin, cent->rawOrigin))
			{
				VectorCopy(cent->lerpOrigin, cent->rawOrigin);
			}
		}

#ifdef FEATURE_EDV
		// save this so we can use it later (eg in edv)
		{
			vec3_t d2;
			vec3_t temp;

			VectorCopy(delta, d2);
			VectorNormalize(d2);
			vectoangles(d2, temp);
			if (demo_nopitch.integer)
			{
				cent->rawAngles[1] = temp[1];
			}
			else
			{
				VectorCopy(temp, cent->rawAngles);
			}
		}
#endif

		if (VectorNormalize2(delta, ent.axis[0]) == 0.f)
		{
			ent.axis[0][2] = 1;
		}
	}
	else if (cent->lerpAngles[0] == 0.f && cent->lerpAngles[1] == 0.f && cent->lerpAngles[2] == 0.f)
	{
		// FIXME: anything to do/save for edv?
		if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0.f)
		{
			ent.axis[0][2] = 1;
		}
	}

	// spin as it moves
	if (s1->pos.trType != TR_STATIONARY)
	{
		RotateAroundDirection(ent.axis, cg.time / 4.f);
	}
	else
	{
		RotateAroundDirection(ent.axis, s1->time);
	}

	// Added this since it may be a propExlosion
	if (ent.hModel)
	{
		// add to refresh list, possibly with quad glow
		CG_AddRefEntityWithPowerups(&ent, s1->powerups, TEAM_FREE, s1, vec3_origin);
	}

#ifdef FEATURE_EDV
	CG_EDV_WeaponCam(cent, &ent);
#endif
}

// capture and hold flag
static animation_t multi_flagpoleAnims[] =
{
	{ 0, "", 0,   1,   0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (no flags, idle)
	{ 0, "", 0,   15,  0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (axis flag rising)
	{ 0, "", 490, 15,  0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (american flag rising)
	{ 0, "", 20,  211, 211, 1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (axis flag raised)
	{ 0, "", 255, 211, 211, 1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (american flag raised)
	{ 0, "", 235, 15,  0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (axis switching to american)
	{ 0, "", 470, 15,  0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (american switching to axis)
	{ 0, "", 510, 15,  0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 },                  // (axis flag falling)
	{ 0, "", 530, 15,  0,   1000 / 15, 1000 / 15, 0, 0, 0, 0, 0, 0 }                   // (american flag falling)
};

extern void CG_RunLerpFrame(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale);

/**
 * @brief CG_TrapSetAnim
 * @param[in] cent
 * @param[in,out] lf
 * @param newAnim - unused
 */
static void CG_TrapSetAnim(centity_t *cent, lerpFrame_t *lf, int newAnim)
{
	// transition animation
	lf->animationNumber = cent->currentState.frame;

	// teamNum specifies which set of animations to use (only 1 exists right now)
	switch (cent->currentState.teamNum)
	{
	case 1:
		lf->animation = &multi_flagpoleAnims[cent->currentState.frame];
		break;
	default:
		return;
	}

	lf->animationTime = lf->frameTime + lf->animation->initialLerp;
}

/**
 * @brief CG_Trap
 * @param[in] cent
 * @todo Change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
 */
static void CG_Trap(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *cs     = &cent->currentState;
	lerpFrame_t   *traplf = &cent->lerpFrame;

	Com_Memset(&ent, 0, sizeof(ent));

	// initial setup
	if (!traplf->oldFrameTime)
	{
		traplf->frameTime        =
			traplf->oldFrameTime = cg.time;

		CG_TrapSetAnim(cent, traplf, cs->frame);

		traplf->frame        =
			traplf->oldFrame = traplf->animation->firstFrame;
	}

	// transition to new anim if requested
	if ((traplf->animationNumber != cs->frame) || !traplf->animation)
	{
		CG_TrapSetAnim(cent, traplf, cs->frame);
	}

	CG_RunLerpFrame(cent, NULL, traplf, 0, 1);    // use existing lerp code rather than re-writing

	ent.frame    = traplf->frame;
	ent.oldframe = traplf->oldFrame;
	ent.backlerp = traplf->backlerp;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[cs->modelindex];

	AnglesToAxis(cent->lerpAngles, ent.axis);

	trap_R_AddRefEntityToScene(&ent);

	Com_Memcpy(&cent->refEnt, &ent, sizeof(refEntity_t));
}

/**
 * @brief CG_Corona
 * @param[in] cent
 */
static void CG_Corona(centity_t *cent)
{
	float    r, g, b;
	int      dli;
	float    dot, dist;
	vec3_t   dir;
	qboolean behind = qfalse, toofar = qfalse;

	if (cg_coronas.integer == 0)       // if set to '0' no coronas
	{
		return;
	}

	dli = cent->currentState.dl_intensity;
	r   = (dli & 0xFF) / 255.0f;
	g   = ((dli >> 8) & 0xFF) / 255.0f;
	b   = ((dli >> 16) & 0xFF) / 255.0f;

	// only coronas that are in your PVS are being added

	VectorSubtract(cg.refdef_current->vieworg, cent->lerpOrigin, dir);

	dist = VectorNormalize2(dir, dir);
	if (dist > cg_coronafardist.integer)       // performance variable cg_coronafardist will keep down super long traces
	{
		toofar = qtrue;
	}

	dot = DotProduct(dir, cg.refdef_current->viewaxis[0]);
	if (dot >= -0.6f)         // assumes ~90 deg fov - changed value to 0.6 (screen corner at 90 fov)
	{
		behind = qtrue;     // use the dot to at least do trivial removal of those behind you.
	}
	// yeah, I could calc side planes to clip against, but would that be worth it? (much better than dumb dot>= thing?)

	//CG_Printf("dot: %f\n", dot);

	if (cg_coronas.integer == 2)       // if set to '2' trace everything
	{
		behind = qfalse;
		toofar = qfalse;
	}

	if (!behind && !toofar)
	{
		trace_t  tr;
		qboolean visible = qfalse;

		CG_Trace(&tr, cg.refdef_current->vieworg, NULL, NULL, cent->lerpOrigin, -1, MASK_SOLID | CONTENTS_BODY);      // added blockage by players.  not sure how this is going to be since this is their bb, not their model (too much blockage)

		if (tr.fraction == 1.f)
		{
			visible = qtrue;
		}

		trap_R_AddCoronaToScene(cent->lerpOrigin, r, g, b, (float)cent->currentState.density / 255.0f, cent->currentState.number, visible);
	}
}

// func_explosive

/**
 * @brief This is currently almost exactly the same as CG_Mover
 * It's split out so that any changes or experiments are
 * unattached to anything else.
 * @param[in] cent
 */
static void CG_Explosive(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);

	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	// get the model, either as a bmodel or a modelindex
	if (s1->solid == SOLID_BMODEL)
	{
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	}
	else
	{
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add the secondary model
	if (s1->modelindex2)
	{
		ent.skinNum = 0;
		ent.hModel  = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}
	else
	{
		trap_R_AddRefEntityToScene(&ent);
	}
}

/**
 * @brief This is currently almost exactly the same as CG_Mover
 * It's split out so that any changes or experiments are unattached to anything else.
 * @param[in] cent
 */
static void CG_Constructible(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	//VectorCopy( ent.origin, cent->lerpOrigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	//CG_Printf( "Adding constructible: %s\n", CG_ConfigString( CS_OID_NAMES + cent->currentState.otherEntityNum2 ) );

	if (s1->modelindex)
	{
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];

		trap_R_AddRefEntityToScene(&ent);
	}

	// add the secondary model
	if (s1->modelindex2)
	{
		if (cent->currentState.powerups == STATE_UNDERCONSTRUCTION)
		{
			ent.customShader = cgs.media.genericConstructionShader;
		}

		ent.hModel = cgs.inlineDrawModel[s1->modelindex2];

		trap_R_AddRefEntityToScene(&ent);
	}
}

/**
 * @brief CG_Mover
 * @param[in,out] cent
 */
static void CG_Mover(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx = 0;
	ent.skinNum  = 0;

	// get the model, either as a bmodel or a modelindex

	if (s1->solid == SOLID_BMODEL)
	{
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	}
	else
	{
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	//  testing for mike to get movers to scale
	if (cent->currentState.density & 1)
	{
		VectorScale(ent.axis[0], cent->currentState.angles2[0], ent.axis[0]);
		VectorScale(ent.axis[1], cent->currentState.angles2[1], ent.axis[1]);
		VectorScale(ent.axis[2], cent->currentState.angles2[2], ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}

	if (cent->currentState.eType == ET_ALARMBOX)
	{
		ent.renderfx |= RF_MINLIGHT;
	}

	// special shader if under construction
	if (cent->currentState.powerups == STATE_UNDERCONSTRUCTION)
	{
		ent.customShader = cgs.media.genericConstructionShader;
	}

	// add the secondary model
	if (s1->modelindex2 && !(cent->currentState.density & 2))
	{
		ent.hModel = cgs.gameModels[s1->modelindex2];
		ent.frame  = s1->frame;

		if (s1->torsoAnim)
		{
			if (cg.time >= cent->lerpFrame.frameTime)
			{
				cent->lerpFrame.oldFrameTime = cent->lerpFrame.frameTime;
				cent->lerpFrame.oldFrame     = cent->lerpFrame.frame;

				while (cg.time >= cent->lerpFrame.frameTime)
				{
					cent->lerpFrame.frameTime += s1->weapon;
					cent->lerpFrame.frame++;

					if (cent->lerpFrame.frame >= s1->legsAnim + s1->torsoAnim)
					{
						cent->lerpFrame.frame = s1->legsAnim;
					}
				}
			}

			if (cent->lerpFrame.frameTime == cent->lerpFrame.oldFrameTime)
			{
				cent->lerpFrame.backlerp = 0;
			}
			else
			{
				cent->lerpFrame.backlerp = 1.0f - (float)(cg.time - cent->lerpFrame.oldFrameTime) / (cent->lerpFrame.frameTime - cent->lerpFrame.oldFrameTime);
			}

			ent.frame = cent->lerpFrame.frame + s1->frame;  // offset
			if (ent.frame >= s1->legsAnim + s1->torsoAnim)
			{
				ent.frame -= s1->torsoAnim;
			}

			ent.oldframe = cent->lerpFrame.oldFrame + s1->frame;    // offset
			if (ent.oldframe >= s1->legsAnim + s1->torsoAnim)
			{
				ent.oldframe -= s1->torsoAnim;
			}

			ent.backlerp = cent->lerpFrame.backlerp;
		}

		if (cent->trailTime)
		{
			cent->lerpFrame.oldFrame = cent->lerpFrame.frame;
			cent->lerpFrame.frame    = s1->legsAnim;

			cent->lerpFrame.oldFrameTime = cent->lerpFrame.frameTime;
			cent->lerpFrame.frameTime    = cg.time;

			ent.oldframe = ent.frame;
			ent.frame    = s1->legsAnim;
			ent.backlerp = 0;
		}

		if (cent->nextState.animMovetype != s1->animMovetype)
		{
			cent->trailTime = 1;
		}
		else
		{
			cent->trailTime = 0;
		}

		trap_R_AddRefEntityToScene(&ent);
		Com_Memcpy(&cent->refEnt, &ent, sizeof(refEntity_t));
	}
	else
	{
		trap_R_AddRefEntityToScene(&ent);
	}
}

#define NUM_FRAME_PROPELLER 10
#define TIME_FRAME_PROPELLER (1000 / NUM_FRAME_PROPELLER)

/**
 * @brief CG_MovePlane
 * @param[in] cent
 */
void CG_MovePlane(centity_t *cent)
{
	refEntity_t ent;

	// allow the airstrike plane to be completely removed
	if (!cg_visualEffects.integer || cent->currentState.time < 0)
	{
		return;
	}

	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lastLerpOrigin, ent.oldorigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	// set frame
	if (cg.time >= cent->lerpFrame.frameTime)
	{
		cent->lerpFrame.oldFrameTime = cent->lerpFrame.frameTime;
		cent->lerpFrame.oldFrame     = cent->lerpFrame.frame;

		while (cg.time >= cent->lerpFrame.frameTime)
		{
			cent->lerpFrame.frameTime += TIME_FRAME_PROPELLER;
			cent->lerpFrame.frame++;

			if (cent->lerpFrame.frame >= NUM_FRAME_PROPELLER)
			{
				cent->lerpFrame.frame = 0;
			}
		}
	}

	if (cent->lerpFrame.frameTime == cent->lerpFrame.oldFrameTime)
	{
		cent->lerpFrame.backlerp = 0;
	}
	else
	{
		cent->lerpFrame.backlerp = 1.0f - (float)(cg.time - cent->lerpFrame.oldFrameTime) / (cent->lerpFrame.frameTime - cent->lerpFrame.oldFrameTime);
	}

	ent.frame = cent->lerpFrame.frame + cent->currentState.frame; // offset
	if (ent.frame >= NUM_FRAME_PROPELLER)
	{
		ent.frame -= NUM_FRAME_PROPELLER;
	}

	ent.oldframe = cent->lerpFrame.oldFrame + cent->currentState.frame;   // offset
	if (ent.oldframe >= NUM_FRAME_PROPELLER)
	{
		ent.oldframe -= NUM_FRAME_PROPELLER;
	}

	ent.backlerp = cent->lerpFrame.backlerp;

	// fade effect
	if (cent->currentState.time)
	{
		ent.shaderRGBA[3] = (byte)(255.f * (float)(cent->currentState.time2 - cg.time) / (float)(cent->currentState.time2 - cent->currentState.time));
	}
	else
	{
		ent.shaderRGBA[3] = 255;
	}

	if (cent->currentState.teamNum == TEAM_AXIS)
	{
		ent.hModel = cgs.media.airstrikePlane[0];

		if (cg.airstrikePlaneScale[0][0] != 0.f || cg.airstrikePlaneScale[0][1] != 0.f || cg.airstrikePlaneScale[0][2] != 0.f)
		{
			VectorScale(ent.axis[0], cg.airstrikePlaneScale[0][0], ent.axis[0]);
			VectorScale(ent.axis[1], cg.airstrikePlaneScale[0][1], ent.axis[1]);
			VectorScale(ent.axis[2], cg.airstrikePlaneScale[0][2], ent.axis[2]);
			ent.nonNormalizedAxes = qtrue;
		}
	}
	else
	{
		ent.hModel = cgs.media.airstrikePlane[1];

		if (cg.airstrikePlaneScale[1][0] != 0.f || cg.airstrikePlaneScale[1][1] != 0.f || cg.airstrikePlaneScale[1][2] != 0.f)
		{
			VectorScale(ent.axis[0], cg.airstrikePlaneScale[1][0], ent.axis[0]);
			VectorScale(ent.axis[1], cg.airstrikePlaneScale[1][1], ent.axis[1]);
			VectorScale(ent.axis[2], cg.airstrikePlaneScale[1][2], ent.axis[2]);
			ent.nonNormalizedAxes = qtrue;
		}
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/**
 * @brief CG_Mover_PostProcess
 * @param[in] cent
 */
void CG_Mover_PostProcess(centity_t *cent)
{
	refEntity_t mg42base;
	refEntity_t mg42upper;
	refEntity_t mg42gun;
	refEntity_t player;
	refEntity_t flash;
	vec_t       *angles;
	int         i;

	if (!(cent->currentState.density & 4))       // mounted gun
	{
		return;
	}

	if ((cg.snap->ps.eFlags & EF_MOUNTEDTANK) && cg_entities[cg.snap->ps.clientNum].tagParent == cent->currentState.effect3Time)
	{
		i = cg.snap->ps.clientNum;
	}
	else
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			// is this entity mounted on a tank, and attached to _OUR_ turret entity (which could be us)
			if (cg_entities[i].currentValid && (cg_entities[i].currentState.eFlags & EF_MOUNTEDTANK))
			{
				if (cg_entities[i].tagParent == cent->currentState.effect3Time)
				{
					break;
				}
			}
		}
	}

	if (i != MAX_CLIENTS)
	{
		if (i != cg.snap->ps.clientNum)
		{
			angles = cg_entities[i].lerpAngles;
		}
		else
		{
			angles = cg.predictedPlayerState.viewangles;
		}
	}
	else
	{
		angles = vec3_origin;
	}

	cg_entities[cent->currentState.effect3Time].tankparent = cent - cg_entities;
	CG_AttachBitsToTank(&cg_entities[cent->currentState.effect3Time], &mg42base, &mg42upper, &mg42gun, &player, &flash, angles, "tag_player", cent->currentState.density & 8);

	// if we (or someone we're spectating) is on this tank, recalc our view values
	if (cg.snap->ps.eFlags & EF_MOUNTEDTANK)
	{
		centity_t *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

		if (tank == &cg_entities[cent->currentState.effect3Time])
		{
			CG_CalcViewValues();
		}
	}

	VectorCopy(mg42base.origin, mg42base.lightingOrigin);
	VectorCopy(mg42base.origin, mg42base.oldorigin);

	VectorCopy(mg42upper.origin, mg42upper.lightingOrigin);
	VectorCopy(mg42upper.origin, mg42upper.oldorigin);

	VectorCopy(mg42gun.origin, mg42gun.lightingOrigin);
	VectorCopy(mg42gun.origin, mg42gun.oldorigin);

	trap_R_AddRefEntityToScene(&mg42base);

	if (i != cg.snap->ps.clientNum || cg.renderingThirdPerson)
	{
		trap_R_AddRefEntityToScene(&mg42upper);
		trap_R_AddRefEntityToScene(&mg42gun);
	}
}

/**
 * @brief New beam entity, for rope like stuff...
 * @param[in] cent
 */
void CG_Beam_2(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;
	vec3_t        origin, origin2;

	BG_EvaluateTrajectory(&s1->pos, cg.time, origin, qfalse, s1->effect1Time);
	BG_EvaluateTrajectory(&s1->apos, cg.time, origin2, qfalse, s1->effect2Time);

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));

	VectorCopy(origin, ent.origin);
	VectorCopy(origin2, ent.oldorigin);

	//CG_Printf( "O: %i %i %i OO: %i %i %i\n", (int)origin[0], (int)origin[1], (int)origin[2], (int)origin2[0], (int)origin2[1], (int)origin2[2] );
	AxisClear(ent.axis);
	ent.reType       = RT_RAIL_CORE;
	ent.customShader = cgs.gameShaders[s1->modelindex2];
	ent.radius       = 8;
	ent.frame        = 2;

	VectorScale(cent->currentState.angles2, 255, ent.shaderRGBA);
	ent.shaderRGBA[3] = 255;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/**
 * @brief Also called as an event
 * @param[in] cent
 */
void CG_Beam(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(s1->pos.trBase, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);

	AxisClear(ent.axis);
	ent.reType = RT_RAIL_CORE;

	switch (s1->legsAnim)
	{
	case 1:
		ent.customShader = cgs.media.ropeShader;
		break;
	default:
		ent.customShader = cgs.media.railCoreShader;
		break;
	}

	ent.shaderRGBA[0] = (byte)(s1->angles2[0] * 255);
	ent.shaderRGBA[1] = (byte)(s1->angles2[1] * 255);
	ent.shaderRGBA[2] = (byte)(s1->angles2[2] * 255);
	ent.shaderRGBA[3] = 255;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/**
 * @brief CG_Portal
 * @param[in] cent
 */
static void CG_Portal(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	PerpendicularVector(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract(vec3_origin, ent.axis[1], ent.axis[1]);

	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType  = RT_PORTALSURFACE;
	ent.frame   = s1->frame;    // rotation speed
	ent.skinNum = s1->clientNum / 256.0 * 360;    // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/**
 * @brief CG_Prop
 * @param[in] cent
 */
static void CG_Prop(centity_t *cent)
{
	refEntity_t   ent;
	entityState_t *s1 = &cent->currentState;

	// create the render entity
	Com_Memset(&ent, 0, sizeof(ent));

	if (cg.renderingThirdPerson)
	{
		VectorCopy(cent->lerpOrigin, ent.origin);
		VectorCopy(cent->lerpOrigin, ent.oldorigin);

		ent.frame    = s1->frame;
		ent.oldframe = ent.frame;
		ent.backlerp = 0;
	}
	else
	{
		float  scale;
		vec3_t angles;

		VectorCopy(cg.refdef_current->vieworg, ent.origin);
		VectorCopy(cg.refdefViewAngles, angles);

		if (cg.bobcycle & 1)
		{
			scale = -cg.xyspeed;
		}
		else
		{
			scale = cg.xyspeed;
		}

		// modify angles from bobbing
		angles[ROLL]  += scale * cg.bobfracsin * 0.005f;
		angles[YAW]   += scale * cg.bobfracsin * 0.01f;
		angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005f;

		VectorCopy(angles, cent->lerpAngles);

		ent.frame    = s1->frame;
		ent.oldframe = ent.frame;
		ent.backlerp = 0;

		if (cent->currentState.density)
		{
			ent.frame    = s1->frame + cent->currentState.density;
			ent.oldframe = ent.frame - 1;
			ent.backlerp = 1 - cg.frameInterpolation;
			ent.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

			//CG_Printf ("frame %d oldframe %d\n", ent.frame, ent.oldframe);
		}
		else if (ent.frame)
		{
			ent.oldframe -= 1;
			ent.backlerp  = 1 - cg.frameInterpolation;
		}
		else
		{
			ent.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;
		}
	}

	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx |= RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = (cg.time >> 6) & 1;

	// get the model, either as a bmodel or a modelindex
	if (s1->solid == SOLID_BMODEL)
	{
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	}
	else
	{
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// special shader if under construction
	if (cent->currentState.powerups == STATE_UNDERCONSTRUCTION)
	{
		ent.customShader = cgs.media.genericConstructionShader;
	}

	// add the secondary model
	if (s1->modelindex2)
	{
		ent.skinNum = 0;
		ent.hModel  = cgs.gameModels[s1->modelindex2];
		ent.frame   = s1->frame;
		trap_R_AddRefEntityToScene(&ent);
		Com_Memcpy(&cent->refEnt, &ent, sizeof(refEntity_t));
	}
	else
	{
		trap_R_AddRefEntityToScene(&ent);
	}
}

typedef enum cabinetType_e
{
	CT_AMMO,
	CT_HEALTH,
	CT_MAX,
} cabinetType_t;

#define MAX_CABINET_TAGS 6
typedef struct cabinetTag_s
{
	const char *tagsnames[MAX_CABINET_TAGS];

	const char *itemnames[MAX_CABINET_TAGS];
	qhandle_t itemmodels[MAX_CABINET_TAGS];

	const char *modelName;
	qhandle_t model;
} cabinetTag_t;

cabinetTag_t cabinetInfo[CT_MAX] =
{
	{
		{
			"tag_ammo01",
			"tag_ammo02",
			"tag_ammo03",
			"tag_ammo04",
			"tag_ammo05",
			"tag_ammo06",
		},
		{
			"models/multiplayer/supplies/ammobox_wm.md3",
			"models/multiplayer/supplies/ammobox_wm.md3",
			"models/multiplayer/supplies/ammobox_wm.md3",
			"models/multiplayer/supplies/ammobox_wm.md3",
			"models/multiplayer/supplies/ammobox_wm.md3",
			"models/multiplayer/supplies/ammobox_wm.md3",
		},
		{
			0, 0, 0, 0, 0, 0
		},
		"models/mapobjects/supplystands/stand_ammo.md3",
		0,
	},
	{
		{
			"tag_Medikit_01",
			"tag_Medikit_02",
			"tag_Medikit_03",
			"tag_Medikit_04",
			"tag_Medikit_05",
			"tag_Medikit_06",
		},
		{
			"models/multiplayer/supplies/healthbox_wm.md3",
			"models/multiplayer/supplies/healthbox_wm.md3",
			"models/multiplayer/supplies/healthbox_wm.md3",
			"models/multiplayer/supplies/healthbox_wm.md3",
			"models/multiplayer/supplies/healthbox_wm.md3",
			"models/multiplayer/supplies/healthbox_wm.md3",
		},
		{
			0, 0, 0, 0, 0, 0
		},
		"models/mapobjects/supplystands/stand_health.md3",
		0,
	},
};

/**
 * @brief CG_Cabinet
 * @param[in] cent
 * @param[in] type
 */
void CG_Cabinet(centity_t *cent, cabinetType_t type)
{
	refEntity_t cabinet;
	refEntity_t mini_me;
	int         i, cnt;

	if (type >= CT_MAX)
	{
		return;
	}

	Com_Memset(&cabinet, 0, sizeof(cabinet));
	Com_Memset(&mini_me, 0, sizeof(mini_me));

	cabinet.hModel   = cabinetInfo[type].model;
	cabinet.frame    = 0;
	cabinet.oldframe = 0;
	cabinet.backlerp = 0.f;

	VectorCopy(cent->lerpOrigin, cabinet.origin);
	VectorCopy(cabinet.origin, cabinet.oldorigin);
	VectorCopy(cabinet.origin, cabinet.lightingOrigin);
	cabinet.lightingOrigin[2] += 16;
	cabinet.renderfx          |= RF_MINLIGHT;
	AnglesToAxis(cent->lerpAngles, cabinet.axis);

	if (cent->currentState.onFireStart == -9999)
	{
		cnt = MAX_CABINET_TAGS;
	}
	else
	{
		cnt = MAX_CABINET_TAGS * (cent->currentState.onFireStart / (float)cent->currentState.onFireEnd);
		if (cnt == 0 && cent->currentState.onFireStart)
		{
			cnt = 1;
		}
	}

	for (i = 0; i < cnt; i++)
	{
		mini_me.hModel = cabinetInfo[type].itemmodels[i];

		CG_PositionEntityOnTag(&mini_me, &cabinet, cabinetInfo[type].tagsnames[i], 0, NULL);

		VectorCopy(mini_me.origin, mini_me.oldorigin);
		VectorCopy(mini_me.origin, mini_me.lightingOrigin);
		mini_me.renderfx |= RF_MINLIGHT;

		trap_R_AddRefEntityToScene(&mini_me);
	}

	/*  for( k = 0; k < 3; k++ ) {
	        VectorScale( cabinet.axis[k], 2.f, cabinet.axis[k] );
	    }
	    cabinet.nonNormalizedAxes = qtrue;*/

	trap_R_AddRefEntityToScene(&cabinet);
}

/**
 * @brief CG_SetupCabinets
 */
void CG_SetupCabinets(void)
{
	int i, j;

	for (i = 0; i < CT_MAX; i++)
	{
		cabinetInfo[i].model = trap_R_RegisterModel(cabinetInfo[i].modelName);
		for (j = 0; j < MAX_CABINET_TAGS; j++)
		{
			cabinetInfo[i].itemmodels[j] = trap_R_RegisterModel(cabinetInfo[i].itemnames[j]);
		}
	}
}

/**
 * @brief Also called by client movement prediction code
 * @param[in] in
 * @param[in] moverNum
 * @param[in] fromTime
 * @param[in] toTime
 * @param[in] out
 * @param[in] outDeltaAngles
 */
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t outDeltaAngles)
{
	centity_t *cent;
	vec3_t    oldOrigin, origin, deltaOrigin;
	vec3_t    oldAngles, deltaAngles;
	vec3_t    transpose[3];
	vec3_t    matrix[3];
	vec3_t    move, org, org2;

	if (outDeltaAngles)
	{
		VectorClear(outDeltaAngles);
	}

	if (moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL)
	{
		VectorCopy(in, out);
		return;
	}

	cent = &cg_entities[moverNum];

	if (cent->currentState.eType != ET_MOVER)
	{
		VectorCopy(in, out);
		return;
	}

	if (!(cent->currentState.eFlags & EF_PATH_LINK))
	{
		vec3_t angles;

		BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin, qfalse, cent->currentState.effect2Time);
		BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles, qtrue, cent->currentState.effect2Time);

		BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin, qfalse, cent->currentState.effect2Time);
		BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles, qtrue, cent->currentState.effect2Time);

		VectorSubtract(origin, oldOrigin, deltaOrigin);
		VectorSubtract(angles, oldAngles, deltaAngles);
	}
	else
	{
		CG_AddLinkedEntity(cent, qtrue, fromTime);

		VectorCopy(cent->lerpOrigin, oldOrigin);
		VectorCopy(cent->lerpAngles, oldAngles);

		CG_AddLinkedEntity(cent, qtrue, toTime);

		VectorSubtract(cent->lerpOrigin, oldOrigin, deltaOrigin);
		VectorSubtract(cent->lerpAngles, oldAngles, deltaAngles);

		CG_AddLinkedEntity(cent, qtrue, cg.time);
	}

	CreateRotationMatrix(deltaAngles, transpose);
	TransposeMatrix(transpose, matrix);

	VectorSubtract(cg.snap->ps.origin, cent->lerpOrigin, org);

	VectorCopy(org, org2);
	RotatePoint(org2, matrix);
	VectorSubtract(org2, org, move);
	VectorAdd(deltaOrigin, move, deltaOrigin);

	VectorAdd(in, deltaOrigin, out);
	if (outDeltaAngles)
	{
		VectorCopy(deltaAngles, outDeltaAngles);
	}

	// NOTE: origin changes when on a rotating object
}

/**
 * @brief CG_InterpolateEntityPosition
 * @param[in,out] cent
 */
static void CG_InterpolateEntityPosition(centity_t *cent)
{
	vec3_t current, next;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if (cg.nextSnap == NULL)
	{
		// FIXME? There are some cases when in Limbo mode during a map restart
		// that were tripping this error.
		//CG_Error( "CG_InterpolateEntityPosition: cg.nextSnap == NULL" );
		//CG_Printf("CG_InterpolateEntityPosition: cg.nextSnap == NULL");
		return;
	}

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current, qfalse, cent->currentState.effect2Time);
	BG_EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, next, qfalse, cent->currentState.effect2Time);

	cent->lerpOrigin[0] = current[0] + cg.frameInterpolation * (next[0] - current[0]);
	cent->lerpOrigin[1] = current[1] + cg.frameInterpolation * (next[1] - current[1]);
	cent->lerpOrigin[2] = current[2] + cg.frameInterpolation * (next[2] - current[2]);

	BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current, qtrue, cent->currentState.effect2Time);
	BG_EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime, next, qtrue, cent->currentState.effect2Time);

	cent->lerpAngles[0] = LerpAngle(current[0], next[0], cg.frameInterpolation);
	cent->lerpAngles[1] = LerpAngle(current[1], next[1], cg.frameInterpolation);
	cent->lerpAngles[2] = LerpAngle(current[2], next[2], cg.frameInterpolation);
}

/**
 * @brief CG_CalcEntityLerpPositions
 * @param[in] cent
 */
void CG_CalcEntityLerpPositions(centity_t *cent)
{
	if (cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE)
	{
		CG_InterpolateEntityPosition(cent);
		return;
	}

	// fix for jittery clients in multiplayer
	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if (cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP && cent->currentState.number < MAX_CLIENTS)
	{
		CG_InterpolateEntityPosition(cent);
		return;
	}

	// backup
	VectorCopy(cent->lerpAngles, cent->lastLerpAngles);
	VectorCopy(cent->lerpOrigin, cent->lastLerpOrigin);

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin, qfalse, cent->currentState.effect2Time);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles, qtrue, cent->currentState.effect2Time);

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if (cent != &cg.predictedPlayerEntity && !cg.showGameView)
	{
		CG_AdjustPositionForMover(cent->lerpOrigin, cent->currentState.groundEntityNum, cg.snap->serverTime, cg.time, cent->lerpOrigin, NULL);
	}
}

/**
 * @brief CG_ProcessEntity
 * @param[in] cent
 * @note All case aren't handle
 */
static void CG_ProcessEntity(centity_t *cent)
{
	switch (cent->currentState.eType)
	{
	default:
		// test for actual bad entity type
		if (cent->currentState.eType >= ET_EVENTS)
		{
			CG_Error("Bad entity type: %i\n", cent->currentState.eType);
		}
		break;
	case ET_CAMERA:
	case ET_INVISIBLE:
	case ET_TELEPORT_TRIGGER:
	case ET_OID_TRIGGER:
	case ET_EXPLOSIVE_INDICATOR:
	case ET_CONSTRUCTIBLE_INDICATOR:
	case ET_TANK_INDICATOR:
	case ET_TANK_INDICATOR_DEAD:
	case ET_COMMANDMAP_MARKER:  // this one should _never_ reach the client
#ifdef VISIBLE_TRIGGERS
	case ET_TRIGGER_MULTIPLE:
	case ET_TRIGGER_FLAGONLY:
	case ET_TRIGGER_FLAGONLY_MULTIPLE:
#endif // VISIBLE_TRIGGERS
		break;
	case ET_SPEAKER:
		CG_Speaker(cent);
		break;
	case ET_GAMEMODEL:
	case ET_MG42_BARREL:
	case ET_GENERAL:
	case ET_AAGUN:
		CG_General(cent);
		break;
	case ET_CABINET_H:
		CG_Cabinet(cent, CT_HEALTH);
		break;
	case ET_CABINET_A:
		CG_Cabinet(cent, CT_AMMO);
		break;
	case ET_CORPSE:
	case ET_PLAYER:
		if (cg.showGameView && cg.filtercams)
		{
			break;
		}
		CG_Player(cent);
		break;
	case ET_ITEM:
		CG_Item(cent);
		break;
	case ET_MISSILE:
	case ET_FLAMEBARREL:
	case ET_RAMJET:
		CG_Missile(cent);
		break;
	case ET_EXPLOSIVE:
		CG_Explosive(cent);
		break;
	case ET_CONSTRUCTIBLE:
		CG_Constructible(cent);
		break;
	case ET_TRAP:
		CG_Trap(cent);
		break;
	case ET_CONSTRUCTIBLE_MARKER:
	case ET_ALARMBOX:
	case ET_MOVER:
		CG_Mover(cent);
		break;
	case ET_PROP:
		CG_Prop(cent);
		break;
	case ET_BEAM:
		CG_Beam(cent);
		break;
	case ET_PORTAL:
		CG_Portal(cent);
		break;
	case ET_CORONA:
		CG_Corona(cent);
		break;
	case ET_BEAM_2:
		CG_Beam_2(cent);
		break;
	case ET_GAMEMANAGER:
		cgs.gameManager = cent;
		break;
	case ET_SMOKER:
		CG_Smoker(cent);
		break;
	case ET_AIRSTRIKE_PLANE:
		CG_MovePlane(cent);
		break;
	}
}

/**
 * @brief CG_AddCEntity
 * @param[in,out] cent
 */
void CG_AddCEntity(centity_t *cent)
{
	// event-only entities will have been dealt with already
	if (cent->currentState.eType >= ET_EVENTS)
	{
		return;
	}

	cent->processedFrame = cg.clientFrame;

	// calculate the current origin
	CG_CalcEntityLerpPositions(cent);

	// add automatic effects
	CG_EntityEffects(cent);

	// call the appropriate function which will add this entity to the view accordingly
	CG_ProcessEntity(cent);
}

/**
 * @brief CG_AddLinkedEntity
 * @param[in,out] cent
 * @param[in] ignoreframe
 * @param[in] atTime
 * @return
 */
qboolean CG_AddLinkedEntity(centity_t *cent, qboolean ignoreframe, int atTime)
{
	entityState_t *s1;
	centity_t     *centParent;
	entityState_t *sParent;
	vec3_t        v;

	// event-only entities will have been dealt with already
	if (cent->currentState.eType >= ET_EVENTS)
	{
		return qtrue;
	}

	if (!ignoreframe && (cent->processedFrame == cg.clientFrame)
#ifdef FEATURE_MULTIVIEW
	    && cg.mvTotalClients < 2
#endif
	    )
	{
		// already processed this frame
		return qtrue;
	}

	s1         = &cent->currentState;
	centParent = &cg_entities[s1->torsoAnim];
	sParent    = &(centParent->currentState);

	// if parent isn't visible, then don't draw us
	if (!centParent->currentValid)
	{
		return qfalse;
	}

	// make sure all parents are added first
	if ((centParent->processedFrame != cg.clientFrame) || ignoreframe)
	{
		if (sParent->eFlags & EF_PATH_LINK)
		{
			if (!CG_AddLinkedEntity(centParent, ignoreframe, atTime))
			{
				return qfalse;
			}
		}
	}

	if (!ignoreframe)
	{
		cent->processedFrame = cg.clientFrame;
	}

	// removed from here:
	//VectorCopy( cent->lerpAngles, cent->lastLerpAngles );
	//VectorCopy( cent->lerpOrigin, cent->lastLerpOrigin );

	if (!(sParent->eFlags & EF_PATH_LINK))
	{
		if (sParent->pos.trType == TR_LINEAR_PATH)
		{
			int   pos;
			float frac;

			if (!(cent->backspline = BG_GetSplineData(sParent->effect2Time, &cent->back)))
			{
				return qfalse;
			}

			cent->backdelta = sParent->pos.trDuration ? (atTime - sParent->pos.trTime) / (float)sParent->pos.trDuration : 0;

			if (cent->backdelta < 0.f)
			{
				cent->backdelta = 0.f;
			}
			else if (cent->backdelta > 1.f)
			{
				cent->backdelta = 1.f;
			}

			if (cent->back)
			{
				cent->backdelta = 1 - cent->backdelta;
			}

			pos = (int)(floor((double)cent->backdelta * (MAX_SPLINE_SEGMENTS)));
			if (pos >= MAX_SPLINE_SEGMENTS)
			{
				pos  = MAX_SPLINE_SEGMENTS - 1;
				frac = cent->backspline->segments[pos].length;
			}
			else
			{
				frac = ((cent->backdelta * (MAX_SPLINE_SEGMENTS)) - pos) * cent->backspline->segments[pos].length;
			}

			VectorMA(cent->backspline->segments[pos].start, frac, cent->backspline->segments[pos].v_norm, v);
			if (sParent->apos.trBase[0] != 0.f)
			{
				BG_LinearPathOrigin2(sParent->apos.trBase[0], &cent->backspline, &cent->backdelta, v, cent->back);
			}

			VectorCopy(v, cent->lerpOrigin);

			if (s1->angles2[0] != 0.f)
			{
				BG_LinearPathOrigin2(s1->angles2[0], &cent->backspline, &cent->backdelta, v, cent->back);
			}

			VectorCopy(v, cent->origin2);

			if (s1->angles2[0] < 0)
			{
				VectorSubtract(v, cent->lerpOrigin, v);
				vectoangles(v, cent->lerpAngles);
			}
			else if (s1->angles2[0] > 0)
			{
				VectorSubtract(cent->lerpOrigin, v, v);
				vectoangles(v, cent->lerpAngles);
			}
			else
			{
				VectorClear(cent->lerpAngles);
			}

			cent->moving = qtrue;
		}
		else
		{
			cent->moving = qfalse;
			VectorCopy(s1->pos.trBase, cent->lerpOrigin);
			VectorCopy(s1->apos.trBase, cent->lerpAngles);
		}
	}
	else
	{
		if (centParent->moving)
		{
			VectorCopy(centParent->origin2, v);

			cent->back       = centParent->back;
			cent->backdelta  = centParent->backdelta;
			cent->backspline = centParent->backspline;

			VectorCopy(v, cent->lerpOrigin);

			if (s1->angles2[0] != 0.f && cent->backspline)
			{
				BG_LinearPathOrigin2(s1->angles2[0], &cent->backspline, &cent->backdelta, v, cent->back);
			}

			VectorCopy(v, cent->origin2);

			if (s1->angles2[0] < 0)
			{
				VectorSubtract(v, cent->lerpOrigin, v);
				vectoangles(v, cent->lerpAngles);
			}
			else if (s1->angles2[0] > 0)
			{
				VectorSubtract(cent->lerpOrigin, v, v);
				vectoangles(v, cent->lerpAngles);
			}
			else
			{
				VectorClear(cent->lerpAngles);
			}
			cent->moving = qtrue;
		}
		else
		{
			cent->moving = qfalse;
			VectorCopy(s1->pos.trBase, cent->lerpOrigin);
			VectorCopy(s1->apos.trBase, cent->lerpAngles);
		}
	}

	if (!ignoreframe)
	{
		// add automatic effects
		CG_EntityEffects(cent);

		// call the appropriate function which will add this entity to the view accordingly
		CG_ProcessEntity(cent);
	}

	return qtrue;
}

/**
 * @brief CG_AddEntityToTag
 * @param[in,out] cent
 * @return
 */
qboolean CG_AddEntityToTag(centity_t *cent)
{
	centity_t   *centParent;
	refEntity_t ent;

	// event-only entities will have been dealt with already
	if (cent->currentState.eType >= ET_EVENTS)
	{
		return qfalse;
	}

	if (cent->processedFrame == cg.clientFrame
#ifdef FEATURE_MULTIVIEW
	    && cg.mvTotalClients < 2
#endif
	    )
	{
		// already processed this frame
		return qtrue;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions(cent);

	if (cent->tagParent < MAX_CLIENTS)
	{
		return qfalse;
	}

	centParent = &cg_entities[cent->tagParent];

	// if parent isn't visible, then don't draw us
	if (!centParent->currentValid)
	{
		return qfalse;
	}

	// make sure all parents are added first
	if (centParent->processedFrame != cg.clientFrame)
	{
		if (!CG_AddCEntity_Filter(centParent))
		{
			return qfalse;
		}
	}

	cent->processedFrame = cg.clientFrame;

	// start with default axis
	AnglesToAxis(vec3_origin, ent.axis);

	// get the tag position from parent
	CG_PositionEntityOnTag(&ent, &centParent->refEnt, cent->tagName, 0, NULL);

	VectorCopy(ent.origin, cent->lerpOrigin);
	// we need to add the child's angles to the tag angles
	if (cent->currentState.eType != ET_PLAYER)
	{
		if (!cent->currentState.density)      // this entity should rotate with it's parent, but can turn around using it's own angles
		{   // fixed to rotate about the object's axis, not the world
			vec3_t mat[3], mat2[3];

			Com_Memcpy(mat2, ent.axis, sizeof(mat2));
			CreateRotationMatrix(cent->lerpAngles, mat);
			MatrixMultiply(mat, mat2, ent.axis);
			AxisToAngles(ent.axis, cent->lerpAngles);
		}
		else        // face our angles exactly
		{
			BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles, qtrue, cent->currentState.effect2Time);
		}
	}

	// add automatic effects
	CG_EntityEffects(cent);

	// call the appropriate function which will add this entity to the view accordingly
	CG_ProcessEntity(cent);

	return qtrue;
}

/**
 * @brief CG_AddCEntity_Filter
 * @param[in] cent
 * @return
 */
qboolean CG_AddCEntity_Filter(centity_t *cent)
{
	if (cent->processedFrame == cg.clientFrame
#ifdef FEATURE_MULTIVIEW
	    && cg.mvTotalClients < 2
#endif
	    )
	{
		return qtrue;
	}

	if (cent->currentState.eFlags & EF_PATH_LINK)
	{
		return CG_AddLinkedEntity(cent, qfalse, cg.time);
	}

	if (cent->currentState.eFlags & EF_TAGCONNECT)
	{
		return CG_AddEntityToTag(cent);
	}

	CG_AddCEntity(cent);
	return qtrue;
}

/**
 * @brief CG_AddPacketEntities
 */
void CG_AddPacketEntities(void)
{
	int num;

	// set cg.frameInterpolation
	if (cg.nextSnap)
	{
		int delta = (cg.nextSnap->serverTime - cg.snap->serverTime);

		if (delta == 0)
		{
			cg.frameInterpolation = 0;
		}
		else
		{
			cg.frameInterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
		}
	}
	else
	{
		cg.frameInterpolation = 0;  // actually, it should never be used, because
		// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAnglesSlow[0] = 0;
	cg.autoAnglesSlow[1] = (cg.time & 4095) * 360 / 4095.0f;
	cg.autoAnglesSlow[2] = 0;

	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 2047) * 360 / 2048.0f;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis(cg.autoAnglesSlow, cg.autoAxisSlow);
	AnglesToAxis(cg.autoAngles, cg.autoAxis);
	AnglesToAxis(cg.autoAnglesFast, cg.autoAxisFast);

	// generate and add the entity from the playerstate
	BG_PlayerStateToEntityState(&cg.predictedPlayerState, &cg.predictedPlayerEntity.currentState, cg.time, qfalse);
	CG_AddCEntity(&cg.predictedPlayerEntity);

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions(&cg_entities[cg.snap->ps.clientNum]);

	cg.satchelCharge = NULL;

	// changing to a single loop, child will request that their parents are added first anyway
	for (num = 0; num < cg.snap->numEntities ; num++)
	{
		CG_AddCEntity_Filter(&cg_entities[cg.snap->entities[num].number]);
	}

	// Add tank bits as a second loop, to stop recursion problems with tank bits not on base entity
	for (num = 0; num < cg.snap->numEntities ; num++)
	{
		if (cg_entities[cg.snap->entities[num].number].currentState.eType == ET_MOVER)
		{
			CG_Mover_PostProcess(&cg_entities[cg.snap->entities[num].number]);
		}
	}

	// add the flamethrower sounds
	CG_UpdateFlamethrowerSounds();
}

/**
 * @brief CGRefEntityToTag
 * @param[in] ent
 * @param[out] tag
 */
void CGRefEntityToTag(refEntity_t *ent, tag_t *tag)
{
	int i;

	VectorCopy(ent->origin, tag->origin);
	for (i = 0; i < 3; i++)
	{
		VectorCopy(ent->axis[i], tag->axis[i]);
	}
}

/**
 * @brief CGTagToRefEntity
 * @param[in] ent
 * @param[out] tag
 */
void CGTagToRefEntity(refEntity_t *ent, tag_t *tag)
{
	int i;

	VectorCopy(tag->origin, ent->origin);
	for (i = 0; i < 3; i++)
	{
		VectorCopy(tag->axis[i], ent->axis[i]);
	}
}

/**
 * @brief CG_AttachBitsToTank
 * @param[in,out] tank
 * @param[out] mg42base
 * @param[out] mg42upper
 * @param[out] mg42gun
 * @param[out] player
 * @param[out] flash
 * @param[in] playerangles
 * @param[in] tagName
 * @param[in] browning
 */
void CG_AttachBitsToTank(centity_t *tank, refEntity_t *mg42base, refEntity_t *mg42upper, refEntity_t *mg42gun, refEntity_t *player, refEntity_t *flash, vec_t *playerangles, const char *tagName, qboolean browning)
{
	Com_Memset(mg42base, 0, sizeof(refEntity_t));
	Com_Memset(mg42gun, 0, sizeof(refEntity_t));
	Com_Memset(mg42upper, 0, sizeof(refEntity_t));
	Com_Memset(player, 0, sizeof(refEntity_t));
	Com_Memset(flash, 0, sizeof(refEntity_t));

	mg42base->hModel  = cgs.media.hMountedMG42Base;
	mg42upper->hModel = cgs.media.hMountedMG42Nest;

	if (browning)
	{
		mg42gun->hModel = cgs.media.hMountedBrowning;
	}
	else
	{
		mg42gun->hModel = cgs.media.hMountedMG42;
	}

	// entity was not received yet, ignore
	if (tank->currentState.number == 0)
	{
		return;
	}

	if (!CG_AddCEntity_Filter(tank))
	{
		return;
	}

	if (tank->tankframe != cg.clientFrame)
	{
		refEntity_t ent;
		vec3_t      angles;

		tank->tankframe = cg.clientFrame;

		Com_Memset(&ent, 0, sizeof(refEntity_t));

		if (tank->currentState.solid == SOLID_BMODEL)
		{
			ent.hModel = cgs.gameModels[tank->currentState.modelindex2];
		}
		else
		{
			ent.hModel = cgs.gameModels[tank->currentState.modelindex];
		}

		ent.frame    = tank->lerpFrame.frame;
		ent.oldframe = tank->lerpFrame.oldFrame;
		ent.backlerp = tank->lerpFrame.backlerp;

		AnglesToAxis(tank->lerpAngles, ent.axis);
		VectorCopy(tank->lerpOrigin, ent.origin);

		AxisClear(mg42base->axis);
		CG_PositionEntityOnTag(mg42base, &ent, tagName, 0, NULL);

		VectorCopy(playerangles, angles);
		angles[PITCH] = 0;

		// thirdperson tank bugfix
		if ((cg.snap->ps.eFlags & EF_MOUNTEDTANK)
		    && cg_entities[cg.snap->ps.clientNum].tagParent
		    == tank - cg_entities)
		{

			angles[YAW]   -= tank->lerpAngles[YAW];
			angles[PITCH] -= tank->lerpAngles[PITCH];
		}
		else
		{
			int i;

			for (i = 0; i < MAX_CLIENTS; i++)
			{
				// is this entity mounted on a tank, and attached to _OUR_ turret entity (which could be us)
				if (cg_entities[i].currentValid && (cg_entities[i].currentState.eFlags & EF_MOUNTEDTANK) && cg_entities[i].tagParent == tank - cg_entities)
				{
					angles[YAW]   -= tank->lerpAngles[YAW];
					angles[PITCH] -= tank->lerpAngles[PITCH];
					break;
				}
			}
		}

		AnglesToAxis(angles, mg42upper->axis);
		CG_PositionRotatedEntityOnTag(mg42upper, mg42base, "tag_mg42nest");

		VectorCopy(playerangles, angles);
		angles[YAW]  = 0;
		angles[ROLL] = 0;

		AnglesToAxis(angles, mg42gun->axis);
		CG_PositionRotatedEntityOnTag(mg42gun, mg42upper, "tag_mg42");

		CG_PositionEntityOnTag(player, mg42upper, "tag_playerpo", 0, NULL);
		CG_PositionEntityOnTag(flash, mg42gun, "tag_flash", 0, NULL);

		CGRefEntityToTag(mg42base, &tank->mountedMG42Base);
		CGRefEntityToTag(mg42upper, &tank->mountedMG42Nest);
		CGRefEntityToTag(mg42gun, &tank->mountedMG42);
		CGRefEntityToTag(player, &tank->mountedMG42Player);
		CGRefEntityToTag(flash, &tank->mountedMG42Flash);
	}

	CGTagToRefEntity(mg42base, &tank->mountedMG42Base);
	CGTagToRefEntity(mg42upper, &tank->mountedMG42Nest);
	CGTagToRefEntity(mg42gun, &tank->mountedMG42);
	CGTagToRefEntity(player, &tank->mountedMG42Player);
	CGTagToRefEntity(flash, &tank->mountedMG42Flash);
}
