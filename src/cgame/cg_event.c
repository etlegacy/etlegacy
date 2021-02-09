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
 * @file cg_event.c
 * @brief Handle entity events at snapshot or playerstate transitions
 */

#include "cg_local.h"
#include "../game/bg_local.h"

extern void CG_StartShakeCamera(float param);
extern void CG_Tracer(vec3_t source, vec3_t dest, int sparks);
//==========================================================================

/**
 * @brief CG_GetObituaryIcon
 * @param[in] mod
 * @param[in] weapon
 * @param[out] weaponShader
 * @param[out] scaleShader
 */
void CG_GetObituaryIcon(meansOfDeath_t mod, weapon_t weapon, qhandle_t *weaponShader, int *scaleShader)
{
	// Get the related weapon from kill
	weapon_t weap = IS_VALID_WEAPON(GetMODTableData(mod)->weaponIcon) ? GetMODTableData(mod)->weaponIcon : weapon;

	// if weapon is still valid
	if (IS_VALID_WEAPON(weap))
	{
		if (cg_drawSmallPopupIcons.integer && cg_weapons[weap].weaponIcon[0])
		{
			*weaponShader = cg_weapons[weap].weaponIcon[0];
			*scaleShader  = cg_weapons[weap].weaponIconScale;
		}
		else if (cg_weapons[weap].weaponIcon[1])
		{
			*weaponShader = cg_weapons[weap].weaponIcon[1];
			*scaleShader  = cg_weapons[weap].weaponIconScale;
		}
		else
		{
			*weaponShader = cgs.media.pmImages[PM_DEATH];
			*scaleShader  = 1;
		}
	}
	else
	{
		*scaleShader = 1;

		// FIXME:
		//case MOD_UNKNOWN:
		//case MOD_FALLING:
		//case MOD_TRIGGER_HURT:
		//case MOD_TELEFRAG:
		//case MOD_TARGET_LASER:
		//case MOD_AIRSTRIKE:
		//case MOD_MAPMORTAR:
		//case MOD_MAPMORTAR_SPLASH:
		//case MOD_EXPLOSIVE:
		//case MOD_GRENADE:
		switch (mod) // deal with icon specials
		{
		case MOD_WATER:
			*weaponShader = cgs.media.waterHintShader;
			break;
		case MOD_SLIME:
			*weaponShader = cgs.media.pmImageSlime;
			break;
		case MOD_LAVA:
			*weaponShader = cgs.media.pmImageLava;
			break;
		case MOD_CRUSH:
			*weaponShader = cgs.media.pmImageCrush;
			break;
		case MOD_SHOVE:
			*weaponShader = cgs.media.pmImageShove;
			break;
		default:
			*weaponShader = cgs.media.pmImages[PM_DEATH];
			break;
		}
	}
}

/**
 * @brief CG_Obituary
 * @param[in] ent
 * @todo FIXME: ... some MODs are not caught - check all!
 *      - MOD_CRUSH_X is selfkill only
 */
static void CG_Obituary(entityState_t *ent)
{
	qhandle_t      shader    = cgs.media.pmImages[PM_DEATH];
	meansOfDeath_t mod       = (meansOfDeath_t)ent->eventParm;
	int            target    = ent->otherEntityNum;
	int            attacker  = ent->otherEntityNum2;
	weapon_t       weapon    = (weapon_t)ent->weapon;
	const char     *message  = NULL;
	const char     *message2 = NULL;
	char           targetName[MAX_NAME_LENGTH];
	char           attackerName[MAX_NAME_LENGTH];
	clientInfo_t   *ci, *ca;  // ca = attacker

	if (target < 0 || target >= MAX_CLIENTS)
	{
		CG_Error("CG_Obituary: target out of range\n");
	}
	ci = &cgs.clientinfo[target];

	if (attacker < 0 || attacker >= MAX_CLIENTS)
	{
		attacker = ENTITYNUM_WORLD;
		ca       = NULL;
	}
	else
	{
		ca = &cgs.clientinfo[attacker];
	}

	Q_strncpyz(targetName, ci->name, sizeof(targetName) - 2);
	strcat(targetName, S_COLOR_WHITE);

	// check for single client messages
	if (!ca)
	{
		message = GetMODTableData(mod)->obituaryNoAttackerMessage;
	}
	// check for self kill messages
	else if (attacker == target)
	{
		// no obituary message if changing teams
		if (mod == MOD_SWITCHTEAM)
		{
			return;
		}

		message = GetMODTableData(mod)->obituarySelfKillMessage;
	}

	if (message)
	{
		if (cg_graphicObituaries.integer)
		{
			qhandle_t weaponShader;
			int       scaleShader;

			CG_GetObituaryIcon(mod, weapon, &weaponShader, &scaleShader);

			CG_AddPMItem(PM_DEATH, targetName, " ", 0, weaponShader, scaleShader, colorYellow);
		}
		else
		{
			CG_AddPMItem(PM_DEATH, va("%s %s.", targetName, CG_TranslateString(message)), " ", shader, 0, 0, colorYellow);
		}
		trap_Print(va("^7%s^7 %s\n", targetName, CG_TranslateString(message)));

		return;
	}

	// check for double client messages
	if (!ca)
	{
		strcpy(attackerName, "noname");
	}
	else
	{
		Q_strncpyz(attackerName, ca->name, sizeof(attackerName) - 2);
		strcat(attackerName, S_COLOR_WHITE);
	}

	// check for kill messages from the current clientNum
#ifdef FEATURE_EDV
	if (attacker == cg.clientNum && !cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#else
	if (attacker == cg.clientNum)
#endif
	{
		char *s;

		if (ca && ci->team == ca->team)
		{
			if (mod == MOD_SWAP_PLACES)
			{
				s = va("%s %s", CG_TranslateString("You swapped places with"), targetName);
			}
			else
			{
				s = va("%s %s", CG_TranslateString("You killed ^1TEAMMATE^7"), targetName);
			}
		}
		else
		{
			s = va("%s %s", CG_TranslateString("You killed"), targetName);
		}

		CG_PriorityCenterPrint(s, 400, cg_fontScaleCP.value, 1);
	}
	else if (attacker == cg.snap->ps.clientNum)
	{
		char *s;

		if (ca && ci->team == ca->team)
		{
			if (mod == MOD_SWAP_PLACES)
			{
				s = va("%s %s %s", attackerName, CG_TranslateString("^7swapped places with"), targetName);
			}
			else
			{
				s = va("%s %s %s", attackerName, CG_TranslateString("^7killed ^1TEAMMATE^7"), targetName);
			}
		}
		else
		{
			s = va("%s %s %s", attackerName, CG_TranslateString("^7killed"), targetName);
		}

		CG_PriorityCenterPrint(s, 400, cg_fontScaleCP.value, 1);
	}

	// check for double client messages
	if (ca)
	{
		message  = GetMODTableData(mod)->obituaryKillMessage1;
		message2 = GetMODTableData(mod)->obituaryKillMessage2;

		if (mod == MOD_BACKSTAB)    // NOTE: we might add a sound for MOD_KNIFE and MOD_KABAR
		{
			// goat luvin
			if (attacker == cg.snap->ps.clientNum || target == cg.snap->ps.clientNum)
			{
				if (ci->team != ca->team)
				{
					trap_S_StartSound(cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.goatAxis);
				}
			}
		}

		// vanilla style
		//if (ci->team == ca->team)
		//{
		//  message  = "^1WAS KILLED BY TEAMMATE^7";
		//  message2 = "";
		//}

		if (message)
		{
			if (cg_graphicObituaries.integer)
			{
				qhandle_t weaponShader;
				int       scaleShader;

				CG_GetObituaryIcon(mod, weapon, &weaponShader, &scaleShader);

				if (cg_graphicObituaries.integer == 1)
				{
					CG_AddPMItem(PM_DEATH, targetName, attackerName, 0, weaponShader, scaleShader, (ci->team == ca->team ? colorRed : colorWhite));
				}
				else
				{
					CG_AddPMItem(PM_DEATH, attackerName, targetName, 0, weaponShader, scaleShader, (ci->team == ca->team ? colorRed : colorWhite));
				}
			}
			else
			{
				if (ci->team == ca->team)
				{
					CG_AddPMItem(PM_DEATH, va("%s^1 %s^7 ", targetName, CG_TranslateString(message)), va("%s^1%s", attackerName, CG_TranslateString(message2)), shader, 0, 0, colorRed);
				}
				else
				{
					CG_AddPMItem(PM_DEATH, va("%s %s ", targetName, CG_TranslateString(message)), va("%s%s", attackerName, CG_TranslateString(message2)), shader, 0, 0, colorWhite);
				}
			}
			trap_Print(va((ci->team == ca->team ? "^7%s^1 %s ^7%s^1%s\n" : "^7%s^7 %s ^7%s^7%s\n"), targetName, CG_TranslateString(message), attackerName, CG_TranslateString(message2)));
			return;
		}
	}

	// we don't know what it was
	CG_AddPMItem(PM_DEATH, va("%s %s.", targetName, CG_TranslateString("died")), " ", shader, 0, 0, colorWhite);
	trap_Print(va("^7%s^7 died\n", targetName));
}

//==========================================================================

// from cg_weapons.c
extern int CG_WeaponIndex(int weapnum, int *bank, int *cycle);

/**
 * @brief A new item was picked up this frame
 * @param[in] itemNum
 */
static void CG_ItemPickup(int itemNum)
{
	gitem_t            *item = BG_GetItem(itemNum);
	int                itemid = item->giWeapon;
	int                wpbank_cur, wpbank_pickup;
	popupMessageType_t giType;

	switch (item->giType)
	{
	case IT_AMMO:
		giType = PM_AMMOPICKUP;
		break;
	case IT_WEAPON:
		if (itemid == WP_AMMO)
		{
			giType = PM_AMMOPICKUP;
		}
		else
		{
			giType = PM_WEAPONPICKUP;
		}
		break;
	case IT_HEALTH:
		giType = PM_HEALTHPICKUP;
		break;
	case IT_TEAM:
		giType = PM_OBJECTIVE;
		break;
	case IT_BAD:
	default:
		giType = PM_MESSAGE;
		break;
	}

#ifdef FEATURE_EDV
	if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
	{
		CG_AddPMItem(giType, va(CG_TranslateString("Picked up %s"), CG_TranslateString(CG_PickupItemText(itemNum))), " ", cgs.media.pmImages[giType], 0, 0, colorWhite);
	}
#else
	CG_AddPMItem(giType, va(CG_TranslateString("Picked up %s"), CG_TranslateString(CG_PickupItemText(itemNum))), " ", cgs.media.pmImages[giType], 0, 0, colorWhite);
#endif

	// see if it should be the grabbed weapon
	if (item->giType == IT_WEAPON)
	{
		// we just drop current weapon
		if (!COM_BitCheck(cg.snap->ps.weapons, cg.weaponSelect))
		{
			cg.weaponSelect = WP_NONE;
		}

		if (cg_autoswitch.integer && cg.predictedPlayerState.weaponstate != WEAPON_RELOADING)
		{
			//  0 - "Off"
			//  1 - "Always Switch"
			//  2 - "If New"
			//  3 - "If Better"
			//  4 - "New or Better"

			// don't ever autoswitch to secondary fire weapons
			// Leave autoswitch to secondary kar/carbine as they use alt ammo and arent zoomed: Note, not that it would do this anyway as it isnt in a bank....
			if (!(GetWeaponTableData(itemid)->type & WEAPON_TYPE_SCOPED) && itemid != WP_AMMO)
			{
				// no weap currently selected, always just select the new one
				if (!cg.weaponSelect)
				{
					cg.weaponSelectTime = cg.time;
					cg.weaponSelect     = itemid;
				}
				// 1 - always switch to new weap
				else if (cg_autoswitch.integer == 1)
				{
					cg.weaponSelectTime = cg.time;
					cg.weaponSelect     = itemid;
				}
				else
				{
					// 2 - switch to weap if it's not already in the player's inventory (Wolf default)
					// 4 - both 2 and 3

					// FIXME:   this works fine for predicted pickups (when you walk over the weapon), but not for
					//          manual pickups (activate item)
					if (cg_autoswitch.integer == 2 || cg_autoswitch.integer == 4)
					{
						if (!COM_BitCheck(cg.snap->ps.weapons, itemid))
						{
							cg.weaponSelectTime = cg.time;
							cg.weaponSelect     = itemid;
						}
					}

					// 3 - switch to weap if it's in a bank greater than the current weap
					// 4 - both 2 and 3
					if (cg_autoswitch.integer == 3 || cg_autoswitch.integer == 4)
					{
						// switch away only if a primary weapon is selected (read: don't switch away if current weap is a secondary mode)
						if (CG_WeaponIndex(cg.weaponSelect, &wpbank_cur, NULL))
						{
							if (CG_WeaponIndex(itemid, &wpbank_pickup, NULL))
							{
								if (wpbank_pickup > wpbank_cur)
								{
									cg.weaponSelectTime = cg.time;
									cg.weaponSelect     = itemid;
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 * @brief Also called by playerstate transition
 * @param[in] cent
 * @param[in] health - unused
 * @param[in] crouching - unused
 */
void CG_PainEvent(centity_t *cent, int health, qboolean crouching)
{
	// don't do more than two pain sounds a second
	// - there are no pain sounds in ET and sound code for this is removed
	// - painTime isn't related to sounds only
	if (cg.time - cent->pe.painTime < 500)
	{
		return;
	}

	// save pain time for programitic twitch animation
	cent->pe.painTime       = cg.time;
	cent->pe.painDirection ^= 1;
}

typedef struct fxSound_s
{
	int max;
	qhandle_t sound[3];
	const char *soundfile[3];
} fxSound_t;

static fxSound_t fxSounds[FXTYPE_MAX] =
{
	// wood
	{ 1, { -1, -1, -1 }, { "sound/world/boardbreak.wav",  NULL,                          NULL                          } },
	// glass
	{ 3, { -1, -1, -1 }, { "sound/world/glassbreak1.wav", "sound/world/glassbreak2.wav", "sound/world/glassbreak3.wav" } },
	// metal
	{ 1, { -1, -1, -1 }, { "sound/world/metalbreak.wav",  NULL,                          NULL                          } },
	// gibs
	{ 1, { -1, -1, -1 }, { "sound/player/gib.wav",        NULL,                          NULL                          } }, // "sound/world/gibsplit1.wav"
	// brick
	{ 1, { -1, -1, -1 }, { "sound/world/debris1.wav",     NULL,                          NULL                          } },
	// stone
	{ 1, { -1, -1, -1 }, { "sound/world/stonefall.wav",   NULL,                          NULL                          } },
	// fabric
	{ 1, { -1, -1, -1 }, { "sound/world/fabricbreak.wav", NULL,                          NULL                          } }
};

/**
 * @brief CG_PrecacheFXSounds
 */
void CG_PrecacheFXSounds(void)
{
	int i, j;

	for (i = FXTYPE_WOOD; i < FXTYPE_MAX; i++)
	{
		for (j = 0; j < fxSounds[i].max; j++)
		{
			fxSounds[i].sound[j] = trap_S_RegisterSound(fxSounds[i].soundfile[j], qfalse);
		}
	}
}

void CG_Explodef(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader);
void CG_RubbleFx(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader, float speedscale, float sizescale);

/**
 * @brief CG_GetSfxSound
 * @param[in] cent
 * @return
 */
static sfxHandle_t CG_GetSoundFx(centity_t *cent)
{
	if (!cent->currentState.dl_intensity)
	{
		sfxHandle_t sound;
		int         index = cent->currentState.frame;

		if (index < FXTYPE_WOOD || index >= FXTYPE_MAX)
		{
			index = FXTYPE_WOOD;
		}

		sound = random() * fxSounds[index].max;

		if (fxSounds[index].sound[sound] == -1)
		{
			fxSounds[index].sound[sound] = trap_S_RegisterSound(fxSounds[index].soundfile[sound], qfalse);
		}

		return fxSounds[index].sound[sound];
	}
	else if (cent->currentState.dl_intensity == -1)
	{
		return 0;
	}

	return CG_GetGameSound(cent->currentState.dl_intensity);
}

/**
 * @brief The old cent-based explode calls will still work with this pass-through
 *      if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 * @param[in] shader
 */
void CG_Explode(centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader)
{
	// inherit shader
	// FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if (cent->currentState.eFlags & EF_INHERITSHADER)
	{
		if (!shader)
		{
			//inheritmodel = cent->currentState.modelindex;
			qhandle_t inheritmodel = cgs.inlineDrawModel[cent->currentState.modelindex];     // okay, this should be better.

			if (inheritmodel)
			{
				shader = trap_R_GetShaderFromModel(inheritmodel, 0, 0);
			}
		}
	}

	CG_Explodef(origin, dir, cent->currentState.density, cent->currentState.frame,
	            CG_GetSoundFx(cent), cent->currentState.weapon, shader);
}

/**
 * @brief CG_Rubble
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 * @param[in] shader
 */
void CG_Rubble(centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader)
{
	// inherit shader
	// FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if (cent->currentState.eFlags & EF_INHERITSHADER)
	{
		if (!shader)
		{
			//inheritmodel = cent->currentState.modelindex;
			qhandle_t inheritmodel = cgs.inlineDrawModel[cent->currentState.modelindex];     // okay, this should be better.

			if (inheritmodel)
			{
				shader = trap_R_GetShaderFromModel(inheritmodel, 0, 0);
			}
		}
	}

	CG_RubbleFx(origin,
	            dir,
	            cent->currentState.density,             // mass
	            cent->currentState.frame,               // type
	            CG_GetSoundFx(cent),                    // sound
	            cent->currentState.weapon,              // forceLowGrav
	            shader,
	            cent->currentState.angles2[0],
	            cent->currentState.angles2[1]
	            );
}

/**
 * @brief CG_RubbleFx
 * @param[in] origin
 * @param[in] dir
 * @param[in] mass
 * @param[in] type
 * @param[in] sound
 * @param[in] forceLowGrav
 * @param[in] shader
 * @param[in] speedscale
 * @param[in] sizescale
 */
void CG_RubbleFx(vec3_t origin, vec3_t dir, int mass, int type, sfxHandle_t sound, int forceLowGrav, qhandle_t shader, float speedscale, float sizescale)
{
	int                 i;
	localEntity_t       *le;
	refEntity_t         *re;
	int                 howmany, total, totalsounds = 0;
	int                 pieces[6];     // how many of each piece
	qhandle_t           modelshader = 0;
	float               materialmul = 1.0f;     // multiplier for different types
	leBounceSoundType_t snd;
	int                 hmodel;
	float               scale;
	int                 endtime;

	Com_Memset(&pieces, 0, sizeof(pieces));

	pieces[5] = (int)(mass / 250.0f);
	pieces[4] = (int)(mass / 76.0f);
	pieces[3] = (int)(mass / 37.0f);      // so 2 per 75
	pieces[2] = (int)(mass / 15.0f);
	pieces[1] = (int)(mass / 10.0f);
	pieces[0] = (int)(mass / 5.0f);

	if (pieces[0] > 20)
	{
		pieces[0] = 20;                 // cap some of the smaller bits so they don't get out of control
	}
	if (pieces[1] > 15)
	{
		pieces[1] = 15;
	}
	if (pieces[2] > 10)
	{
		pieces[2] = 10;
	}

	if (type == 0)        // cap wood even more since it's often grouped, and the small splinters can add up
	{
		if (pieces[0] > 10)
		{
			pieces[0] = 10;
		}
		if (pieces[1] > 10)
		{
			pieces[1] = 10;
		}
		if (pieces[2] > 10)
		{
			pieces[2] = 10;
		}
	}

	total = pieces[5] + pieces[4] + pieces[3] + pieces[2] + pieces[1] + pieces[0];

	if (sound)
	{
		trap_S_StartSound(origin, -1, CHAN_AUTO, sound);
	}

	if (shader)     // shader passed in to use
	{
		modelshader = shader;
	}

	for (i = 0; i < POSSIBLE_PIECES; i++)
	{
		snd    = LEBS_NONE;
		hmodel = 0;

		for (howmany = 0; howmany < pieces[i]; howmany++)
		{
			scale   = 1.0f;
			endtime = 0;     // set endtime offset for faster/slower fadeouts

			switch (type)
			{
			case FXTYPE_WOOD:     // "wood"
				snd    = LEBS_WOOD;
				hmodel = cgs.media.debWood[i];

				if (i == 0)
				{
					scale = 0.5f;
				}
				else if (i == 1)
				{
					scale = 0.6f;
				}
				else if (i == 2)
				{
					scale = 0.7f;
				}
				else if (i == 3)
				{
					scale = 0.5f;
				}
				//                  else goto pass;

				if (i < 3)
				{
					endtime = -3000;     // small bits live 3 sec shorter than normal
				}
				break;

			case FXTYPE_GLASS:     // "glass"
				snd = LEBS_NONE;
				if (i == 5)
				{
					hmodel = cgs.media.shardGlass1;
				}
				else if (i == 4)
				{
					hmodel = cgs.media.shardGlass2;
				}
				else if (i == 2)
				{
					hmodel = cgs.media.shardGlass2;
				}
				else if (i == 1)
				{
					hmodel = cgs.media.shardGlass2;
					scale  = 0.5f;
				}
				else
				{
					goto pass;
				}
				break;

			case FXTYPE_METAL:     // metal
				snd = LEBS_METAL;
				if (i == 5)
				{
					hmodel = cgs.media.shardMetal1;
				}
				else if (i == 4)
				{
					hmodel = cgs.media.shardMetal2;
				}
				else if (i == 2)
				{
					hmodel = cgs.media.shardMetal2;
				}
				else if (i == 1)
				{
					hmodel = cgs.media.shardMetal2;
					scale  = 0.5f;
				}
				else
				{
					goto pass;
				}
				break;

			case FXTYPE_GIBS:     // gibs
				snd = LEBS_BLOOD;
				if (i == 5)
				{
					hmodel = cgs.media.gibIntestine;
				}
				else if (i == 4)
				{
					hmodel = cgs.media.gibLeg;
				}
				else if (i == 2)
				{
					hmodel = cgs.media.gibChest;
				}
				else
				{
					goto pass;
				}
				break;

			case FXTYPE_BRICK:     // brick
				snd    = LEBS_ROCK;
				hmodel = cgs.media.debBlock[i];
				break;

			case FXTYPE_STONE:     // rock
				snd = LEBS_ROCK;
				if (i == 5)
				{
					hmodel = cgs.media.debRock[2];            // temporarily use the next smallest rock piece
				}
				else if (i == 4)
				{
					hmodel = cgs.media.debRock[2];
				}
				else if (i == 3)
				{
					hmodel = cgs.media.debRock[1];
				}
				else if (i == 2)
				{
					hmodel = cgs.media.debRock[0];
				}
				else if (i == 1) // note: there is debBlock 0-5
				{
					hmodel = cgs.media.debBlock[1];            // temporarily use the small block pieces
				}
				else
				{
					hmodel = cgs.media.debBlock[0];            // temporarily use the small block pieces
				}
				if (i <= 2)
				{
					endtime = -2000;     // small bits live 2 sec shorter than normal
				}
				break;

			case FXTYPE_FABRIC:     // fabric
				if (i == 5)
				{
					hmodel = cgs.media.debFabric[0];
				}
				else if (i == 4)
				{
					hmodel = cgs.media.debFabric[1];
				}
				else if (i == 2)
				{
					hmodel = cgs.media.debFabric[2];
				}
				else if (i == 1)
				{
					hmodel = cgs.media.debFabric[2];
					scale  = 0.5;
				}
				else
				{
					goto pass;     // (only do 5, 4, 2 and 1)
				}
				break;
			default:
				break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType    = LE_FRAGMENT;
			le->startTime = cg.time;

			le->endTime = (le->startTime + 5000 + random() * 5000) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//          AxisClear(re->axis);
			//          re->axis[0][0] =
			//          re->axis[1][1] =
			//          re->axis[2][2] = scale;
			//
			//          if(scale != 1.0)
			//              re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale * sizescale;

			if (type == FXTYPE_GLASS)     // glass
			{     // added this because glass looks funky when it fades out
				  // FIXME: need to look into this so that they fade out correctly
				re->fadeStartTime = le->endTime;
				re->fadeEndTime   = le->endTime;
			}
			else
			{
				re->fadeStartTime = le->endTime - 4000;
				re->fadeEndTime   = le->endTime;
			}

			if (total > 5)
			{
				if (totalsounds > 5 || (howmany % 8) != 0)
				{
					snd = LEBS_NONE;
				}
				else
				{
					totalsounds++;
				}
			}

			le->lifeRate   = 1.0f / (le->endTime - le->startTime);
			le->leFlags    = LEF_TUMBLE;
			le->leMarkType = LEMT_NONE;

			VectorCopy(origin, re->origin);
			AxisCopy(axisDefault, re->axis);

			le->leBounceSoundType = snd;
			re->hModel            = hmodel;

			// inherit shader
			if (modelshader)
			{
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if (type == FXTYPE_FABRIC)     // fabric
			{
				le->pos.trType   = TR_GRAVITY_FLOAT;   // the fabric stuff will change to use something that looks better
				le->bounceFactor = 0.0f;
				materialmul      = 0.3f;     // rotation speed
			}
			else
			{
				if (!forceLowGrav && (rand() & 1))        // if low gravity is not forced and die roll goes our way use regular grav
				{
					le->pos.trType = TR_GRAVITY;
				}
				else
				{
					le->pos.trType = TR_GRAVITY_LOW;
				}

				le->bounceFactor = 0.4f;
			}

			// rotation
			le->angles.trType     = TR_LINEAR;
			le->angles.trTime     = cg.time;
			le->angles.trBase[0]  = rand() & 31;
			le->angles.trBase[1]  = rand() & 31;
			le->angles.trBase[2]  = rand() & 31;
			le->angles.trDelta[0] = ((100 + (rand() & 500)) - 300) * materialmul;
			le->angles.trDelta[1] = ((100 + (rand() & 500)) - 300) * materialmul;
			le->angles.trDelta[2] = ((100 + (rand() & 500)) - 300) * materialmul;

			VectorCopy(origin, le->pos.trBase);
			VectorNormalize(dir);
			le->pos.trTime = cg.time;

			// hoping that was just intended to represent randomness
			// if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if (le->angles.trBase[0] == 1.0f || le->angles.trBase[1] == 1.0f || le->angles.trBase[2] == 1.0f)
			{
				le->pos.trType = TR_GRAVITY;
				VectorScale(dir, 10 * 8, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 400) - 200) * speedscale;
				le->pos.trDelta[1] += ((random() * 400) - 200) * speedscale;
				le->pos.trDelta[2]  = ((random() * 400) + 400) * speedscale;
			}
			else
			{
				// location
				VectorScale(dir, 200 + mass, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 200) - 100);
				le->pos.trDelta[1] += ((random() * 200) - 100);

				if (dir[2] != 0.f)
				{
					le->pos.trDelta[2] = random() * 200 * materialmul;     // randomize sort of a lot so they don't all land together
				}
				else
				{
					le->pos.trDelta[2] = random() * 20;
				}
			}
		}
pass:
		continue;
	}
}

/**
 * @brief Made this more generic for spawning hits and breaks without needing a *cent
 * @param[in] origin
 * @param[in] dir
 * @param[in] mass
 * @param[in] type
 * @param[in] sound
 * @param[in] forceLowGrav
 * @param[in] shader
 */
void CG_Explodef(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader)
{
	int                 i;
	localEntity_t       *le;
	refEntity_t         *re;
	int                 howmany, total, totalsounds = 0;
	int                 pieces[POSSIBLE_PIECES];  // how many of each piece
	qhandle_t           modelshader = 0;
	float               materialmul = 1;     // multiplier for different types
	leBounceSoundType_t snd;
	int                 hmodel;
	float               scale;
	int                 endtime;

	Com_Memset(&pieces, 0, sizeof(pieces));

	pieces[5] = (int)(mass / 250.0f);
	pieces[4] = (int)(mass / 76.0f);
	pieces[3] = (int)(mass / 37.0f);      // so 2 per 75
	pieces[2] = (int)(mass / 15.0f);
	pieces[1] = (int)(mass / 10.0f);
	pieces[0] = (int)(mass / 5.0f);

	if (pieces[0] > 20)
	{
		pieces[0] = 20;                 // cap some of the smaller bits so they don't get out of control
	}
	if (pieces[1] > 15)
	{
		pieces[1] = 15;
	}
	if (pieces[2] > 10)
	{
		pieces[2] = 10;
	}

	if (type == FXTYPE_WOOD)        // cap wood even more since it's often grouped, and the small splinters can add up
	{
		if (pieces[0] > 10)
		{
			pieces[0] = 10;
		}
		if (pieces[1] > 10)
		{
			pieces[1] = 10;
		}
		if (pieces[2] > 10)
		{
			pieces[2] = 10;
		}
	}

	total = pieces[5] + pieces[4] + pieces[3] + pieces[2] + pieces[1] + pieces[0];

	if (sound)
	{
		trap_S_StartSound(origin, -1, CHAN_AUTO, sound);
	}

	if (shader)     // shader passed in to use
	{
		modelshader = shader;
	}

	for (i = 0; i < POSSIBLE_PIECES; i++)
	{
		snd    = LEBS_NONE;
		hmodel = 0;

		for (howmany = 0; howmany < pieces[i]; howmany++)
		{
			scale   = 1.0f;
			endtime = 0;     // set endtime offset for faster/slower fadeouts

			switch (type)
			{
			case FXTYPE_WOOD:     // wood
				snd    = LEBS_WOOD;
				hmodel = cgs.media.debWood[i];

				if (i == 0)
				{
					scale = 0.5f;
				}
				else if (i == 1)
				{
					scale = 0.6f;
				}
				else if (i == 2)
				{
					scale = 0.7f;
				}
				else if (i == 3)
				{
					scale = 0.5f;
				}
				//                  else goto pass;

				if (i < 3)
				{
					endtime = -3000;     // small bits live 3 sec shorter than normal
				}
				break;

			case FXTYPE_GLASS:     // glass
				snd = LEBS_NONE;
				if (i == 5)
				{
					hmodel = cgs.media.shardGlass1;
				}
				else if (i == 4)
				{
					hmodel = cgs.media.shardGlass2;
				}
				else if (i == 2)
				{
					hmodel = cgs.media.shardGlass2;
				}
				else if (i == 1)
				{
					hmodel = cgs.media.shardGlass2;
					scale  = 0.5f;
				}
				else
				{
					goto pass;
				}
				break;

			case FXTYPE_METAL:     // metal
				snd = LEBS_BRASS;
				if (i == 5)
				{
					hmodel = cgs.media.shardMetal1;
				}
				else if (i == 4)
				{
					hmodel = cgs.media.shardMetal2;
				}
				else if (i == 2)
				{
					hmodel = cgs.media.shardMetal2;
				}
				else if (i == 1)
				{
					hmodel = cgs.media.shardMetal2;
					scale  = 0.5f;
				}
				else
				{
					goto pass;
				}
				break;

			case FXTYPE_GIBS:     // gibs
				snd = LEBS_BLOOD;
				if (i == 5)
				{
					hmodel = cgs.media.gibIntestine;
				}
				else if (i == 4)
				{
					hmodel = cgs.media.gibLeg;
				}
				else if (i == 2)
				{
					hmodel = cgs.media.gibChest;
				}
				else
				{
					goto pass;
				}
				break;

			case FXTYPE_BRICK:     // brick
				snd    = LEBS_ROCK;
				hmodel = cgs.media.debBlock[i];
				break;

			case FXTYPE_STONE:     // rock
				snd = LEBS_ROCK;
				if (i == 5)
				{
					hmodel = cgs.media.debRock[2];             // temporarily use the next smallest rock piece
				}
				else if (i == 4)
				{
					hmodel = cgs.media.debRock[2];
				}
				else if (i == 3)
				{
					hmodel = cgs.media.debRock[1];
				}
				else if (i == 2)
				{
					hmodel = cgs.media.debRock[0];
				}
				else if (i == 1) // note: there is debBlock 0-5
				{
					hmodel = cgs.media.debBlock[1];            // temporarily use the small block pieces
				}
				else
				{
					hmodel = cgs.media.debBlock[0];            // temporarily use the small block pieces
				}
				if (i <= 2)
				{
					endtime = -2000;     // small bits live 2 sec shorter than normal
				}
				break;

			case FXTYPE_FABRIC:     // fabric
				if (i == 5)
				{
					hmodel = cgs.media.debFabric[0];
				}
				else if (i == 4)
				{
					hmodel = cgs.media.debFabric[1];
				}
				else if (i == 2)
				{
					hmodel = cgs.media.debFabric[2];
				}
				else if (i == 1)
				{
					hmodel = cgs.media.debFabric[2];
					scale  = 0.5;
				}
				else
				{
					goto pass;     // (only do 5, 4, 2 and 1)
				}
				break;
			default:
				break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType    = LE_FRAGMENT;
			le->startTime = cg.time;

			le->endTime = (le->startTime + 5000 + random() * 5000) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//          AxisClear(re->axis);
			//          re->axis[0][0] =
			//          re->axis[1][1] =
			//          re->axis[2][2] = scale;
			//
			//          if(scale != 1.0)
			//              re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale;

			if (type == FXTYPE_GLASS)     // glass
			{     // added this because glass looks funky when it fades out
				  // - need to look into this so that they fade out correctly
				re->fadeStartTime = le->endTime;
				re->fadeEndTime   = le->endTime;
			}
			else
			{
				re->fadeStartTime = le->endTime - 4000;
				re->fadeEndTime   = le->endTime;
			}

			if (total > 5)
			{
				if (totalsounds > 5 || (howmany % 8) != 0)
				{
					snd = LEBS_NONE;
				}
				else
				{
					totalsounds++;
				}
			}

			le->lifeRate   = 1.0f / (le->endTime - le->startTime);
			le->leFlags    = LEF_TUMBLE;
			le->leMarkType = LEMT_NONE;

			VectorCopy(origin, re->origin);
			AxisCopy(axisDefault, re->axis);

			le->leBounceSoundType = snd;
			re->hModel            = hmodel;

			// inherit shader
			if (modelshader)
			{
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if (type == FXTYPE_FABRIC)     // "fabric"
			{
				le->pos.trType   = TR_GRAVITY_FLOAT;   // the fabric stuff will change to use something that looks better
				le->bounceFactor = 0.0f;
				materialmul      = 0.3f;     // rotation speed
			}
			else
			{
				if (!forceLowGrav && (rand() & 1))     // if low gravity is not forced and die roll goes our way use regular grav
				{
					le->pos.trType = TR_GRAVITY;
				}
				else
				{
					le->pos.trType = TR_GRAVITY_LOW;
				}

				le->bounceFactor = 0.4f;
			}

			// rotation
			le->angles.trType     = TR_LINEAR;
			le->angles.trTime     = cg.time;
			le->angles.trBase[0]  = rand() & 31;
			le->angles.trBase[1]  = rand() & 31;
			le->angles.trBase[2]  = rand() & 31;
			le->angles.trDelta[0] = ((100 + (rand() & 500)) - 300) * materialmul;
			le->angles.trDelta[1] = ((100 + (rand() & 500)) - 300) * materialmul;
			le->angles.trDelta[2] = ((100 + (rand() & 500)) - 300) * materialmul;

			VectorCopy(origin, le->pos.trBase);
			VectorNormalize(dir);
			le->pos.trTime = cg.time;

			// hoping that was just intended to represent randomness
			// if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if (le->angles.trBase[0] == 1.0f || le->angles.trBase[1] == 1.0f || le->angles.trBase[2] == 1.0f)
			{
				le->pos.trType = TR_GRAVITY;
				VectorScale(dir, 10 * 8, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 100) - 50);
				le->pos.trDelta[1] += ((random() * 100) - 50);
				le->pos.trDelta[2]  = (random() * 200) + 200;
			}
			else
			{
				// location
				VectorScale(dir, 200 + mass, le->pos.trDelta);
				le->pos.trDelta[0] += ((random() * 100) - 50);
				le->pos.trDelta[1] += ((random() * 100) - 50);

				if (dir[2] != 0.f)
				{
					le->pos.trDelta[2] = random() * 200 * materialmul;     // randomize sort of a lot so they don't all land together
				}
				else
				{
					le->pos.trDelta[2] = random() * 20;
				}
			}
		}
pass:
		continue;
	}
}

/**
 * @brief Quake ed -> target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) fire explode smoke debris gore lowgrav
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 */
void CG_Effect(centity_t *cent, vec3_t origin, vec3_t dir)
{
	localEntity_t *le;
	refEntity_t   *re;
	//int             large, small;

	VectorSet(dir, 0, 0, 1);      // straight up.

	// 1 large per 100, 1 small per 24
	// large   = (int)(mass / 100);
	// small   = (int)(mass / 24) + 1;

	if (cent->currentState.eventParm & 1)      // fire
	{
		int effect;

		effect = (CG_PointContents(origin, 0) & CONTENTS_WATER) ? PS_FX_WATER : PS_FX_COMMON;

		CG_MissileHitWall(WP_DYNAMITE, effect, origin, dir, 0, -1);
		return;
	}

	// right now force smoke on any explosions
	// if(cent->currentState.eventParm & 4)    // smoke
	if (cent->currentState.eventParm & 7)
	{
		int    i, j;
		vec3_t sprVel, sprOrg;

		// explosion sprite animation
		VectorScale(dir, 16, sprVel);
		for (i = 0; i < 5; i++)
		{
			for (j = 0; j < 3; j++)
			{
				sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
			}
			sprVel[2] += rand() % 50;
			CG_ParticleExplosion("blacksmokeanim", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60, qfalse);     // was smokeanimb
		}
	}

	if (cent->currentState.eventParm & 2)      // explode
	{
		vec3_t sprVel, sprOrg;

		trap_S_StartSound(origin, -1, CHAN_AUTO, cgs.media.sfx_rockexp);

		// new explode  (from rl)
		VectorMA(origin, 16, dir, sprOrg);
		VectorScale(dir, 100, sprVel);
		CG_ParticleExplosion("explode1", sprOrg, sprVel, 500, 20, 160, qtrue);

		if (cg_markTime.integer)
		{
			vec4_t color, projection;

			VectorSet(projection, 0, 0, -1);
			projection[3] = 64.0f;
			Vector4Set(color, 1.0f, 1.0f, 1.0f, 1.0f);
			trap_R_ProjectDecal(cgs.media.burnMarkShader, 1, (vec3_t *) origin, projection, color, cg_markTime.integer, (cg_markTime.integer >> 4));
		}
	}

	if (cent->currentState.eventParm & 8)     // rubble
	{     // share the cg_explode code with func_explosives
		const char *s;
		qhandle_t  sh     = 0;     // shader handle
		vec3_t     newdir = { 0, 0, 0 };

		if (cent->currentState.angles2[0] != 0.0f || cent->currentState.angles2[1] != 0.0f || cent->currentState.angles2[2] != 0.0f)
		{
			VectorCopy(cent->currentState.angles2, newdir);
		}

		s = CG_ConfigString(CS_TARGETEFFECT);     // see if ent has a shader specified
		if (s && strlen(s) > 0)
		{
			sh = trap_R_RegisterShader(va("textures/%s", s));        // FIXME: don't do this here.  only for testing

		}
		cent->currentState.eFlags      &= ~EF_INHERITSHADER;     // don't try to inherit shader
		cent->currentState.dl_intensity = 0;        // no sound
		CG_Explode(cent, origin, newdir, sh);
	}

	if (cent->currentState.eventParm & 16)     // gore
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType    = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime   = le->startTime + 5000 + random() * 3000;
		// fading out
		re->fadeStartTime = le->endTime - 4000;
		re->fadeEndTime   = le->endTime;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);
		//re->hModel = hModel;
		re->hModel     = cgs.media.gibIntestine;
		le->pos.trType = TR_GRAVITY;
		VectorCopy(origin, le->pos.trBase);

		//VectorCopy( velocity, le->pos.trDelta );
		VectorNormalize(dir);
		VectorMA(dir, 200, dir, le->pos.trDelta);

		le->pos.trTime = cg.time;

		le->bounceFactor = 0.3f;

		le->leBounceSoundType = LEBS_BLOOD;
		le->leMarkType        = LEMT_BLOOD;
	}

	if (cent->currentState.eventParm & 64)     // debris trails (the black strip that Ryan did)
	{
		CG_AddDebris(origin, dir, 280, 1400, 7 + rand() % 2, NULL); // FIXME: add eventParm & 64 and throw extended debris
	}
}

/**
 * @brief CG_Shard
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 * @note  We should keep this separate since there will be considerable differences
 *   in the physical properties of shard vrs debris. not to mention the fact
 *   there is no way we can quantify what type of effects the designers will
 *   potentially desire. If it is still possible to merge the functionality of
 *   cg_shard into cg_explode at a latter time I would have no problem with that
 *   but for now I want to keep it separate
 */
void CG_Shard(centity_t *cent, vec3_t origin, vec3_t dir)
{
	localEntity_t *le;
	refEntity_t   *re;
	int           type    = cent->currentState.density;
	int           howmany = cent->currentState.frame;
	int           i;
	int           rval;
	qboolean      isflyingdebris = qfalse;

	for (i = 0; i < howmany; i++)
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType    = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime   = le->startTime + 5000 + random() * 5000;

		// fading out
		re->fadeStartTime = le->endTime - 1000;
		re->fadeEndTime   = le->endTime;

		if (type == 999)
		{
			le->startTime     = cg.time;
			le->endTime       = le->startTime + 100;
			re->fadeStartTime = le->endTime - 100;
			re->fadeEndTime   = le->endTime;
			type              = 1;

			isflyingdebris = qtrue;
		}

		le->lifeRate     = 1.0f / (le->endTime - le->startTime);
		le->leFlags      = LEF_TUMBLE;
		le->bounceFactor = 0.4f;
		// le->leBounceSoundType    = LEBS_WOOD;
		le->leMarkType = LEMT_NONE;

		VectorCopy(origin, re->origin);
		AxisCopy(axisDefault, re->axis);


		switch (type)
		{
		case FXTYPE_WOOD:
			rval = rand() % 2;

			if (rval)
			{
				re->hModel = cgs.media.shardWood1;
			}
			else
			{
				re->hModel = cgs.media.shardWood2;
			}
			break;
		case FXTYPE_GLASS:
			rval = rand() % 2;

			if (rval)
			{
				re->hModel = cgs.media.shardGlass1;
			}
			else
			{
				re->hModel = cgs.media.shardGlass2;
			}
			break;
		case FXTYPE_METAL:
			rval = rand() % 2;

			if (rval)
			{
				re->hModel = cgs.media.shardMetal1;
			}
			else
			{
				re->hModel = cgs.media.shardMetal2;
			}
			break;
		case FXTYPE_BRICK:      // rubble
		case FXTYPE_STONE:
			rval = rand() % 3;     // note: there is debBlock 0-5

			if (rval == 1)
			{
				//re->hModel = cgs.media.shardRubble1;
				re->hModel = cgs.media.debBlock[0];
			}
			else if (rval == 2)
			{
				//re->hModel = cgs.media.shardRubble2;
				re->hModel = cgs.media.debBlock[1];
			}
			else
			{
				//re->hModel = cgs.media.shardRubble3;
				re->hModel = cgs.media.debBlock[2];
			}
			break;
		//case ceramic:
		//rval = rand()%2;
		//if (rval)
		//  re->hModel = cgs.media.shardCeramic1;
		//else
		//  re->hModel = cgs.media.shardCeramic2;
		default:     // FXTYPE_GIBS, FXTYPE_FABRIC
			CG_Printf("CG_Debris has an unknown type\n");
			break;
		}

		// location
		if (isflyingdebris)
		{
			le->pos.trType = TR_GRAVITY_LOW;
		}
		else
		{
			le->pos.trType = TR_GRAVITY;
		}

		VectorCopy(origin, le->pos.trBase);
		VectorNormalize(dir);
		VectorScale(dir, 10 * howmany, le->pos.trDelta);
		le->pos.trTime      = cg.time;
		le->pos.trDelta[0] += ((random() * 100) - 50);
		le->pos.trDelta[1] += ((random() * 100) - 50);
		if (type)
		{
			le->pos.trDelta[2] = (random() * 200) + 100;      // randomize sort of a lot so they don't all land together
		}
		else     // glass
		{
			le->pos.trDelta[2] = (random() * 100) + 50;     // randomize sort of a lot so they don't all land together

		}
		// rotation
		le->angles.trType     = TR_LINEAR;
		le->angles.trTime     = cg.time;
		le->angles.trBase[0]  = rand() & 31;
		le->angles.trBase[1]  = rand() & 31;
		le->angles.trBase[2]  = rand() & 31;
		le->angles.trDelta[0] = (100 + (rand() & 500)) - 300;
		le->angles.trDelta[1] = (100 + (rand() & 500)) - 300;
		le->angles.trDelta[2] = (100 + (rand() & 500)) - 300;
	}
}

/**
 * @brief CG_ShardJunk
 * @param[in] origin
 * @param[in] dir
 */
void CG_ShardJunk(vec3_t origin, vec3_t dir)
{
	localEntity_t *le = CG_AllocLocalEntity();
	refEntity_t   *re = &le->refEntity;

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = le->startTime + 5000 + random() * 5000;

	re->fadeStartTime = le->endTime - 1000;
	re->fadeEndTime   = le->endTime;

	le->lifeRate     = 1.0f / (le->endTime - le->startTime);
	le->leFlags      = LEF_TUMBLE;
	le->bounceFactor = 0.4f;
	le->leMarkType   = LEMT_NONE;

	VectorCopy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);

	re->hModel = cgs.media.shardJunk[rand() % MAX_LOCKER_DEBRIS];

	le->pos.trType = TR_GRAVITY;

	VectorCopy(origin, le->pos.trBase);
	VectorNormalize(dir);
	VectorScale(dir, 10 * 8, le->pos.trDelta);
	le->pos.trTime      = cg.time;
	le->pos.trDelta[0] += ((random() * 100) - 50);
	le->pos.trDelta[1] += ((random() * 100) - 50);

	le->pos.trDelta[2] = (random() * 100) + 50;     // randomize sort of a lot so they don't all land together

	// rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	//le->angles.trBase[0] = rand()&31;
	//le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand() & 31;

	//le->angles.trDelta[0] = (100 + (rand()&500)) - 300;
	//le->angles.trDelta[1] = (100 + (rand()&500)) - 300;
	le->angles.trDelta[2] = (100 + (rand() & 500)) - 300;
}

/**
 * @brief Debris test
 * @param[in] cent
 * @param[in] origin
 * @param[in] dir
 */
void CG_Debris(centity_t *cent, vec3_t origin, vec3_t dir)
{
	localEntity_t *le = CG_AllocLocalEntity();
	refEntity_t   *re = &le->refEntity;

	le->leType    = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime   = le->startTime + 5000 + random() * 5000;

	re->fadeStartTime = le->endTime - 1000;
	re->fadeEndTime   = le->endTime;

	le->lifeRate     = 1.0f / (le->endTime - le->startTime);
	le->leFlags      = LEF_TUMBLE | LEF_TUMBLE_SLOW;
	le->bounceFactor = 0.4f;
	le->leMarkType   = LEMT_NONE;
	le->breakCount   = 1;
	le->sizeScale    = 0.5f;

	VectorCopy(origin, re->origin);
	AxisCopy(axisDefault, re->axis);

	re->hModel = cgs.inlineDrawModel[cent->currentState.modelindex];

	le->pos.trType = TR_GRAVITY;

	VectorCopy(origin, le->pos.trBase);
	VectorCopy(dir, le->pos.trDelta);
	le->pos.trTime = cg.time;

	// rotation
	le->angles.trType    = TR_LINEAR;
	le->angles.trTime    = cg.time;
	le->angles.trBase[2] = rand() & 31;

	//le->angles.trDelta[0] = (100 + (rand() & 500)) - 300;
	//le->angles.trDelta[1] = (50 + (rand() & 400)) - 100;
	le->angles.trDelta[2] = (50 + (rand() & 400)) - 100;
}

/**
 * @brief CG_MortarImpact
 * @param[in] cent
 * @param[in] origin
 */
void CG_MortarImpact(centity_t *cent, vec3_t origin)
{
	if (cent->currentState.clientNum == cg.snap->ps.clientNum && cg.mortarImpactTime != -2)
	{
		VectorCopy(origin, cg.mortarImpactPos);
		cg.mortarImpactTime     = cg.time;
		cg.mortarImpactOutOfMap = qfalse;
	}
}

/**
 * @brief CG_MortarMiss
 * @param[in] cent
 * @param[in] origin
 */
void CG_MortarMiss(centity_t *cent, vec3_t origin)
{
	if (cent->currentState.clientNum == cg.snap->ps.clientNum && cg.mortarImpactTime != -2)
	{
		VectorCopy(origin, cg.mortarImpactPos);
		cg.mortarImpactTime = cg.time;
		if (cent->currentState.density)
		{
			cg.mortarImpactOutOfMap = qtrue;
		}
		else
		{
			cg.mortarImpactOutOfMap = qfalse;
		}
	}
}

/**
 * @brief CG_PlayGlobalSound
 * @param[in] cent
 * @param[in] index
 */
void CG_PlayGlobalSound(centity_t *cent, int index)
{
	sfxHandle_t sound = CG_GetGameSound(index);

	if (sound)
	{
		// no origin!
#ifdef FEATURE_EDV
		if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
		{
			trap_S_StartLocalSound(sound, CHAN_AUTO);
		}
		else
		{
			// no origin!
			trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound);

		}
#else
		// no origin!
		trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound);
#endif
	}
	else
	{
		if (index >= GAMESOUND_MAX)
		{
			const char *s;

			s = CG_ConfigString(CS_SOUNDS + (index - GAMESOUND_MAX));

			if (!strstr(s, ".wav") && !strstr(s, ".ogg"))         // sound script names haven't got file extensions
			{
				// origin is NULL!
				if (CG_SoundPlaySoundScript(s, NULL, -1, qtrue))
				{
					return;
				}
			}

			sound = CG_CustomSound(cent->currentState.number, s);
			if (sound)
			{
#ifdef FEATURE_EDV
				if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
				{
					trap_S_StartLocalSound(sound, CHAN_AUTO);
				}
				else
				{
					// origin is NULL!
					trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound);
				}
#else
				// origin is NULL!
				trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_AUTO, sound);
#endif

			}
			else
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: CG_EntityEvent() cannot play EV_GLOBAL_SOUND sound '%s'\n", s);
			}
		}
		else
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: CG_EntityEvent() es->eventParm < GAMESOUND_MAX\n");
		}
	}
}

extern void CG_AddBulletParticles(vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale);

void CG_PlayHitSound(const int clientNum, const int hitSound)
{
	// Do we have hitsounds even enabled
	if (!(cg_hitSounds.integer & HITSOUNDS_ON))
	{
		return;
	}

	// Are we spectating someone?
	if (cg.snap->ps.clientNum != cg.clientNum && cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && !(cg.snap->ps.pm_flags & PMF_LIMBO))
	{
		return;
	}

	// Is the event for the current client (might be the player or a player being spectated)
	if (clientNum != cg.snap->ps.clientNum)
	{
		return;
	}

	switch (hitSound)
	{
		case HIT_TEAMSHOT:
			if (!(cg_hitSounds.integer & HITSOUNDS_NOTEAMSHOT))
			{
				trap_S_StartLocalSound(cgs.media.teamShot, CHAN_LOCAL_SOUND);
			}
			break;
		case HIT_HEADSHOT:
			if (!(cg_hitSounds.integer & HITSOUNDS_NOHEADSHOT))
			{
				trap_S_StartLocalSound(cgs.media.headShot, CHAN_LOCAL_SOUND);
			}
			else if (!(cg_hitSounds.integer & HITSOUNDS_NOBODYSHOT))
			{
				trap_S_StartLocalSound(cgs.media.bodyShot, CHAN_LOCAL_SOUND);
			}
			break;
		case HIT_BODYSHOT:
			if (!(cg_hitSounds.integer & HITSOUNDS_NOBODYSHOT))
			{
				trap_S_StartLocalSound(cgs.media.bodyShot, CHAN_LOCAL_SOUND);
			}
			break;
		default:
			CG_DPrintf("Unkown hitsound: %i\n", hitSound);
			break;
	}
}

/**
 * @brief An entity has an event value also called by CG_CheckPlayerstateEvents
 * @param[in] cent
 * @param[in] position
 */
void CG_EntityEvent(centity_t *cent, vec3_t position)
{
	entityState_t *es   = &cent->currentState;
	int           event = es->event & ~EV_EVENT_BITS;
	int           clientNum;

	static int footstepcnt       = 0;
	static int splashfootstepcnt = 0;

	if (cg_debugEvents.integer)
	{
		CG_Printf("time:%i ent:%3i  event:%3i ", cg.time, es->number, event);

		if (event < EV_NONE || event >= EV_MAX_EVENTS)
		{
			CG_Printf("UNKNOWN\n");
		}
		else
		{
			CG_Printf("%s\n", eventnames[event]);
		}
	}

	if (!event)
	{
		return;
	}

	clientNum = es->clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = 0;
	}

	switch (event)
	{
	// movement generated events
	case EV_FOOTSTEP:
		if (es->eventParm != FOOTSTEP_TOTAL)
		{
			if (es->eventParm)
			{
				trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[es->eventParm][footstepcnt]);
			}
			else
			{
				bg_character_t *character;

				character = CG_CharacterForClientinfo(&cgs.clientinfo[clientNum], cent);
				trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[character->animModelInfo->footsteps][footstepcnt]);
			}
		}
		break;
	case EV_FOOTSPLASH:
		trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_SPLASH][splashfootstepcnt]);
		break;
	case EV_SWIM:
		trap_S_StartSound(NULL, es->number, CHAN_BODY, cgs.media.footsteps[FOOTSTEP_SPLASH][footstepcnt]);
		break;
	case EV_FALL_SHORT:
		if (es->eventParm != FOOTSTEP_TOTAL)
		{
			if (es->eventParm)
			{
				trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound[es->eventParm]);
			}
			else
			{
				bg_character_t *character;

				character = CG_CharacterForClientinfo(&cgs.clientinfo[clientNum], cent);
				trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound[character->animModelInfo->footsteps]);
			}
		}
		if (clientNum == cg.predictedPlayerState.clientNum)
		{
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime   = cg.time;
		}
		break;
	case EV_BODY_DP:
		if (&cg_entities[es->otherEntityNum2])
		{
			Com_Memset(&cg_entities[es->otherEntityNum2].pe, 0, sizeof(playerEntity_t));
		}
		break;
	case EV_FALL_NDIE:
	case EV_FALL_DMG_10:
	case EV_FALL_DMG_15:
	case EV_FALL_DMG_25:
	case EV_FALL_DMG_50:
		if (es->eventParm != FOOTSTEP_TOTAL)
		{
			if (es->eventParm)
			{
				trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound[es->eventParm]);
			}
			else
			{
				bg_character_t *character;
				character = CG_CharacterForClientinfo(&cgs.clientinfo[clientNum], cent);

				trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landSound[character->animModelInfo->footsteps]);
			}
		}
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.landHurt);
		cent->pe.painTime = cg.time;     // don't play a pain sound right after this
		if (event != EV_FALL_NDIE && clientNum == cg.predictedPlayerState.clientNum)
		{
			// smooth landing z changes
			if (event < EV_FALL_DMG_25)
			{
				cg.landChange = -16;
			}
			else
			{
				cg.landChange = -24;
			}
			cg.landTime = cg.time;
		}
		break;
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:        // smooth out step up transitions
	{
		float oldStep;
		int   delta;
		int   step;

		if (clientNum != cg.predictedPlayerState.clientNum)
		{
			break;
		}
		// if we are interpolating, we don't need to smooth steps
		if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
		    cg_nopredict.integer
#ifdef ALLOW_GSYNC
		    || cg_synchronousClients.integer
#endif // ALLOW_GSYNC
		    )
		{
			break;
		}
		// check for stepping up before a previous step is completed
		delta = cg.time - cg.stepTime;
		if (delta < STEP_TIME)
		{
			oldStep = cg.stepChange * (STEP_TIME - delta) / STEP_TIME;
		}
		else
		{
			oldStep = 0;
		}

		// add this amount
		step          = 4 * (event - EV_STEP_4 + 1);
		cg.stepChange = oldStep + step;
		if (cg.stepChange > MAX_STEP_CHANGE)
		{
			cg.stepChange = MAX_STEP_CHANGE;
		}
		cg.stepTime = cg.time;
		break;
	}
	case EV_WATER_TOUCH:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrInSound);
		break;
	case EV_WATER_LEAVE:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound);
		break;
	case EV_WATER_UNDER:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound);
		if (cg.clientNum == es->number)
		{
			cg.waterundertime = cg.time + HOLDBREATHTIME;
		}

		// this fog stuff for underwater is really just a test for feasibility of creating the under-water effect that way.
		// the related issues of death underwater, etc. are not handled at all.
		// the actual problem, of course, is doing underwater stuff when the water is very turbulant and you can't simply
		// do things based on the players head being above/below the water brushes top surface. (since the waves can potentially be /way/ above/below that)

		// causes problems in multiplayer...
		break;
	case EV_WATER_CLEAR:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound);
		if (es->eventParm)
		{
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.watrGaspSound);
		}
		break;
	case EV_ITEM_PICKUP:
	case EV_ITEM_PICKUP_QUIET:
	{
		int index = es->eventParm;              // player predicted

		if (index < 1 || index >= ITEM_MAX_ITEMS)
		{
			break;
		}

		if (event == EV_ITEM_PICKUP)         // not quiet
		{
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.itemPickUpSounds[index]);
		}

		// show icon and name on status bar
		if (es->number == cg.snap->ps.clientNum)
		{
			CG_ItemPickup(index);
		}
	}
	break;

	case EV_GLOBAL_ITEM_PICKUP:
	{
		gitem_t *item;
		int     index = es->eventParm;          // player predicted

		if (index < 1 || index >= ITEM_MAX_ITEMS)
		{
			break;
		}
		item = BG_GetItem(index);
		if (*item->pickup_sound)
		{
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.itemPickUpSounds[index]);
		}

		// show icon and name on status bar
		if (es->number == cg.snap->ps.clientNum)
		{
			CG_ItemPickup(index);
		}
	}
	break;

	// weapon events
	case EV_WEAP_OVERHEAT:
		// start weapon idle animation
		if (es->number == cg.snap->ps.clientNum)
		{
			cg.predictedPlayerState.weapAnim = ((cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | PM_IdleAnimForWeapon(es->weapon);
			cent->overheatTime               = cg.time;     // used to make the barrels smoke when overheated
		}

		if (BG_PlayerMounted(es->eFlags))
		{
			if ((es->eFlags & EF_MOUNTEDTANK) && IS_MOUNTED_TANK_BROWNING(es->number))
			{
				trap_S_StartSoundVControl(NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd_2, 255);
			}
			else
			{
				trap_S_StartSoundVControl(NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd, 255);
			}
		}
		else if (cg_weapons[es->weapon].overheatSound)
		{
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].overheatSound);
		}
		break;
	case EV_SPINUP:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].spinupSound);
		break;
	case EV_FILL_CLIP:
		// IS_VALID_WEAPON(es->weapon) ?
		if (cgs.clientinfo[es->clientNum].skill[SK_LIGHT_WEAPONS] >= 2 && (GetWeaponTableData(es->weapon)->attributes & WEAPON_ATTRIBUT_FAST_RELOAD) && cg_weapons[es->weapon].reloadFastSound)
		{
			trap_S_StartSound(NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadFastSound);
		}
		else if (cg_weapons[es->weapon].reloadSound)
		{
			trap_S_StartSound(NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadSound);     // following sherman's SP fix, should allow killing reload sound when player dies
		}
		break;
	case EV_MG42_FIXED:
		trap_S_StartSound(NULL, es->number, CHAN_WEAPON, cgs.cachedSounds[GAMESOUND_WORLD_MG_CONSTRUCTED]);
		break;
	case EV_NOAMMO:
	case EV_WEAPONSWITCHED:
		if (cg_weapons[es->weapon].noAmmoSound)
		{
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].noAmmoSound);     // FIXME: CHAN_LOCAL_SOUND ?
		}

		if (es->number == cg.snap->ps.clientNum)
		{
			if ((cg_noAmmoAutoSwitch.integer > 0 && !CG_WeaponSelectable(cg.weaponSelect))
			    || (GetWeaponTableData(es->weapon)->firingMode & (WEAPON_FIRING_MODE_ONE_SHOT | WEAPON_FIRING_MODE_THROWABLE)))
			{
				CG_OutOfAmmoChange(event == EV_NOAMMO);
			}
		}
		break;
	case EV_CHANGE_WEAPON:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.selectSound);
		break;
	case EV_CHANGE_WEAPON_2:
		trap_S_StartSound(NULL, es->number, CHAN_AUTO, cgs.media.selectSound);

		// special switching sound for alt weapon
		if (cg_weapons[GetWeaponTableData(es->weapon)->weapAlts].switchSound)
		{
			trap_S_StartSound(NULL, es->number, CHAN_WEAPON, cg_weapons[GetWeaponTableData(es->weapon)->weapAlts].switchSound);
		}

		if (es->number == cg.snap->ps.clientNum)
		{
			// client will get this message if reloading while using an alternate weapon
			// client should voluntarily switch back to primary at that point
			if (GetWeaponTableData(es->weapon)->type & WEAPON_TYPE_SCOPED)
			{
				CG_FinishWeaponChange(es->weapon, GetWeaponTableData(es->weapon)->weapAlts);
			}
		}
		break;
	case EV_FIRE_WEAPON_MOUNTEDMG42:
	case EV_FIRE_WEAPON_MG42:
	{
		vec3_t porg, gorg, norm;         // player/gun origin
		float  gdist;

		VectorCopy(cent->currentState.pos.trBase, gorg);
		VectorCopy(cg.refdef_current->vieworg, porg);
		VectorSubtract(gorg, porg, norm);
		gdist = VectorNormalize(norm);
		if (gdist > 512 && gdist < 4096)
		{
			VectorMA(cg.refdef_current->vieworg, 64, norm, gorg);
			if (IS_MOUNTED_TANK_BROWNING(cent->currentState.number))           // should we use a browning?
			{
				trap_S_StartSoundEx(gorg, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponEchoSnd_2, SND_NOCUT);
			}
			else
			{
				trap_S_StartSoundEx(gorg, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponEchoSnd, SND_NOCUT);
			}
		}

		CG_FireWeapon(cent);
	}
	break;
	case EV_FIRE_WEAPON_AAGUN:
		CG_FireWeapon(cent);
		break;
	case EV_FIRE_WEAPON:
	case EV_FIRE_WEAPONB:
		if (cent->currentState.clientNum == cg.snap->ps.clientNum && (cg.snap->ps.eFlags & EF_ZOOMING))     // to stop airstrike sfx
		{
			break;
		}

		cent->akimboFire = (event == EV_FIRE_WEAPONB);         // akimbo firing

		CG_FireWeapon(cent);
		break;
	case EV_FIRE_WEAPON_LASTSHOT:
		CG_FireWeapon(cent);
		break;
	case EV_NOFIRE_UNDERWATER:
		if (cgs.media.noFireUnderwater)
		{
			trap_S_StartSound(NULL, es->number, CHAN_WEAPON, cgs.media.noFireUnderwater);
		}
		break;
	case EV_GRENADE_BOUNCE:
	{
		sfxHandle_t    sfx;
		soundSurface_t soundSurfaceIndex;

		soundSurfaceIndex = CG_GetSoundSurfaceIndex(BG_SurfaceForFootstep(es->eventParm));

		sfx = CG_GetRandomSoundSurface(cg_weapons[es->weapon].missileBouncingSound, soundSurfaceIndex, qtrue);

		if (sfx)
		{
			trap_S_StartSound(NULL, es->number, CHAN_AUTO, sfx);
		}
	}
	break;
	case EV_RAILTRAIL:
	{
		vec3_t color = { es->angles[0] / 255.f, es->angles[1] / 255.f, es->angles[2] / 255.f };

		// red is default if there is no color set
		if (color[0] == 0.0f && color[1] == 0.0f && color[2] == 0.0f)
		{
			color[0] = 1;
			color[1] = 0;
			color[2] = 0;
		}

		CG_RailTrail(color, es->origin2, es->pos.trBase, es->dmgFlags, es->effect1Time);         // added 'type' field
	}
	break;

	// missile impacts
	case EV_MISSILE_HIT:
	{
		vec3_t dir;

		ByteToDir(es->eventParm, dir);
		CG_MissileHitPlayer(es->number, es->weapon, position, dir, es->otherEntityNum);
	}
	break;
	case EV_MISSILE_MISS_SMALL:
	{
		vec3_t dir;

		ByteToDir(es->eventParm, dir);

		CG_MissileHitWallSmall(position, dir);
	}
	break;
	case EV_MISSILE_MISS:
	{
		vec3_t dir;
		int    effect;

		effect = (CG_PointContents(position, 0) & CONTENTS_WATER) ? PS_FX_WATER : PS_FX_COMMON;

		ByteToDir(es->eventParm, dir);
		CG_MissileHitWall(es->weapon, effect, position, dir, 0, es->number);
	}
	break;
	case EV_MISSILE_MISS_LARGE:
	{
		vec3_t dir;
		int    effect;

		effect = (CG_PointContents(position, 0) & CONTENTS_WATER) ? PS_FX_WATER : PS_FX_COMMON;
		ByteToDir(es->eventParm, dir);
		if (es->weapon == WP_ARTY || es->weapon == WP_AIRSTRIKE || es->weapon == WP_SMOKE_MARKER)
		{
			CG_MissileHitWall(es->weapon, effect, position, dir, 0, es->number);
		}
		else
		{
			CG_MissileHitWall(VERYBIGEXPLOSION, effect, position, dir, 0, es->number);
		}
	}
	break;
	case EV_MORTAR_IMPACT:
	{
		CG_MortarImpact(cent, es->origin2);
		break;
	}
	case EV_MORTAR_MISS:
		CG_MortarMiss(cent, position);
		break;
	case EV_SHOVE_SOUND:
		//if ( cg_shoveSounds.integer ) {
		// origin is NULL!
		trap_S_StartSoundVControl(NULL, es->number, CHAN_AUTO, cgs.media.shoveSound, 255);
		//}
		break;
	case EV_MG42BULLET_HIT_WALL:
		CG_Bullet(es->weapon, es->pos.trBase, es->otherEntityNum, qfalse, ENTITYNUM_WORLD, es->otherEntityNum2, es->origin2[0], es->effect1Time);
		break;
	case EV_MG42BULLET_HIT_FLESH:
		CG_Bullet(es->weapon, es->pos.trBase, es->otherEntityNum, qtrue, es->eventParm, es->otherEntityNum2, 0, es->effect1Time);
		break;
	case EV_BULLET_HIT_WALL:
		CG_Bullet(es->weapon, es->pos.trBase, es->otherEntityNum, qfalse, ENTITYNUM_WORLD, es->otherEntityNum2, es->origin2[0], 0);
		break;
	case EV_BULLET_HIT_FLESH:
		CG_Bullet(es->weapon, es->pos.trBase, es->otherEntityNum, qtrue, es->eventParm, es->otherEntityNum2, 0, 0);
		break;
	case EV_GENERAL_SOUND:
	{
		sfxHandle_t sound = CG_GetGameSound(es->eventParm);

		if (sound)
		{
			// origin is NULL!
			trap_S_StartSoundVControl(NULL, es->number, CHAN_VOICE, sound, 255);
		}
		else
		{
			if (es->eventParm >= GAMESOUND_MAX)
			{
				const char *s;

				s = CG_ConfigString(CS_SOUNDS + (es->eventParm - GAMESOUND_MAX));

				if (CG_SoundPlaySoundScript(s, NULL, es->number, qfalse))
				{
					break;
				}
				sound = CG_CustomSound(es->number, s);
				if (sound)
				{
					trap_S_StartSoundVControl(NULL, es->number, CHAN_VOICE, sound, 255);
				}
			}
		}
	}
	break;
	case EV_FX_SOUND:
	{
		sfxHandle_t sound;
		int         index = es->eventParm;

		if (index < FXTYPE_WOOD || index >= FXTYPE_MAX)
		{
			index = FXTYPE_WOOD ;
		}

		sound = (random() * fxSounds[index].max);
		if (fxSounds[index].sound[sound] == -1)
		{
			fxSounds[index].sound[sound] = trap_S_RegisterSound(fxSounds[index].soundfile[sound], qfalse);
		}

		sound = fxSounds[index].sound[sound];

		trap_S_StartSoundVControl(NULL, es->number, CHAN_VOICE, sound, 255);
	}
	break;
	case EV_GENERAL_SOUND_VOLUME:
	{
		sfxHandle_t sound = CG_GetGameSound(es->eventParm);

		if (sound)
		{
			trap_S_StartSoundVControl(NULL, es->number, CHAN_VOICE, sound, es->onFireStart);
		}
		else
		{
			if (es->eventParm >= GAMESOUND_MAX)
			{
				const char *s;

				s = CG_ConfigString(CS_SOUNDS + (es->eventParm - GAMESOUND_MAX));

				if (CG_SoundPlaySoundScript(s, NULL, es->number, qfalse))
				{
					break;
				}
				sound = CG_CustomSound(es->number, s);
				if (sound)
				{
					trap_S_StartSoundVControl(NULL, es->number, CHAN_VOICE, sound, es->onFireStart);
				}
			}
		}
	}
	break;
	case EV_GLOBAL_TEAM_SOUND:
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == es->teamNum)
		{
			CG_PlayGlobalSound(cent, es->eventParm);
		}
		break;
	case EV_GLOBAL_SOUND:     // play from the player's head so it never diminishes
		CG_PlayGlobalSound(cent, es->eventParm);
		break;
	case EV_GLOBAL_CLIENT_SOUND:
		if (cg.snap->ps.clientNum == es->teamNum)
		{
			CG_PlayGlobalSound(cent, es->eventParm);
		}
		break;
	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		if (cent->currentState.number != cg.snap->ps.clientNum)
		{
			CG_PainEvent(cent, es->eventParm, qfalse);
		}
		break;
	case EV_OBITUARY:
		CG_Obituary(es);
		break;
	case EV_STOPSTREAMINGSOUND:
		//trap_S_StopStreamingSound( es->number );
		trap_S_StartSoundEx(NULL, es->number, CHAN_WEAPON, 0, SND_CUTOFF_ALL);      // kill weapon sound (could be reloading)
		break;
	case EV_LOSE_HAT:
	{
		vec3_t dir;

		ByteToDir(es->eventParm, dir);
		CG_LoseHat(cent, dir);
	}
	break;
	case EV_GIB_PLAYER:
	{
		vec3_t dir;

		trap_S_StartSound(es->pos.trBase, -1, CHAN_AUTO, cgs.media.gibSound);
		ByteToDir(es->eventParm, dir);
		CG_GibPlayer(cent, cent->lerpOrigin, dir);
	}
	break;
	// particles
	case EV_SMOKE:
		if (cent->currentState.density == 3)
		{
			CG_ParticleSmoke(cgs.media.smokePuffShaderdirty, cent);
		}
		else // if (!(cent->currentState.density)) & others
		{
			CG_ParticleSmoke(cgs.media.smokePuffShader, cent);
		}
		break;
	case EV_FLAMETHROWER_EFFECT:
		CG_FireFlameChunks(cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.6f, qtrue);
		break;
	case EV_DUST:
		CG_ParticleDust(cent, cent->currentState.origin, cent->currentState.angles);
		break;
	case EV_RUMBLE_EFX:
	{
		CG_RumbleEfx(cent->currentState.angles[0], cent->currentState.angles[1]);
	}
	break;
	case EV_EMITTER:
	{
		localEntity_t *le = CG_AllocLocalEntity();

		le->leType     = LE_EMITTER;
		le->startTime  = cg.time;
		le->endTime    = le->startTime + 20000;
		le->pos.trType = TR_STATIONARY;
		VectorCopy(cent->currentState.origin, le->pos.trBase);
		VectorCopy(cent->currentState.origin2, le->angles.trBase);
		le->ownerNum = 0;
	}
	break;
	case EV_OILPARTICLES:
		CG_Particle_OilParticle(cgs.media.oilParticle, cent->currentState.origin, cent->currentState.origin2, cent->currentState.time, cent->currentState.density);
		break;
	case EV_OILSLICK:
		CG_Particle_OilSlick(cgs.media.oilSlick, cent);
		break;
	case EV_OILSLICKREMOVE:
		CG_OilSlickRemove(cent);
		break;
	case EV_SPARKS_ELECTRIC:
	case EV_SPARKS:
	{
		int    numsparks;
		int    i;
		vec3_t source, dest;

		if (!(cent->currentState.density))
		{
			cent->currentState.density = 1;
		}
		numsparks = rand() % cent->currentState.density;

		if (!numsparks)
		{
			numsparks = 1;
		}

		for (i = 0; i < numsparks; i++)
		{
			if (event == EV_SPARKS_ELECTRIC)
			{
				VectorCopy(cent->currentState.origin, source);

				VectorCopy(source, dest);
				dest[0] += ((rand() & 31) - 16);
				dest[1] += ((rand() & 31) - 16);
				dest[2] += ((rand() & 31) - 16);

				CG_Tracer(source, dest, 1);
			}
			else
			{
				CG_ParticleSparks(cent->currentState.origin, cent->currentState.angles, cent->currentState.frame, cent->currentState.angles2[0], cent->currentState.angles2[1], cent->currentState.angles2[2]);
			}
		}
	}
	break;
	case EV_GUNSPARKS:
	{
		CG_AddBulletParticles(cent->currentState.origin, cent->currentState.angles, cent->currentState.angles2[2], 800, cent->currentState.density, 1.0f);
	}
	break;
	case EV_SNOWFLURRY:
		CG_ParticleSnowFlurry(cgs.media.snowShader, cent);
		break;

	// for func_exploding
	case EV_EXPLODE:
	{
		vec3_t dir;

		ByteToDir(es->eventParm, dir);
		CG_Explode(cent, position, dir, 0);
	}
	break;
	case EV_RUBBLE:
	{
		vec3_t dir;

		ByteToDir(es->eventParm, dir);
		CG_Rubble(cent, position, dir, 0);
	}
	break;

	// for target_effect
	case EV_EFFECT:
	{
		vec3_t dir;
		ByteToDir(es->eventParm, dir);
		CG_Effect(cent, position, dir);
	}
	break;
	case EV_MORTAREFX:     // mortar firing
		CG_MortarEFX(cent);
		break;
	case EV_SHARD:
	{
		vec3_t dir;

		ByteToDir(es->eventParm, dir);
		CG_Shard(cent, position, dir);
	}
	break;
	case EV_JUNK:
	{
		vec3_t dir;
		int    i;
		int    rval = rand() % 3 + 3;

		ByteToDir(es->eventParm, dir);

		for (i = 0; i < rval; i++)
		{
			CG_ShardJunk(position, dir);
		}
	}
	break;
	case EV_DISGUISE_SOUND:
		trap_S_StartSound(NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.uniformPickup);
		break;
	case EV_BUILDDECAYED_SOUND:
		trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.media.buildDecayedSound);
		break;

	// debris test
	case EV_DEBRIS:
		CG_Debris(cent, position, cent->currentState.origin2);
		break;

	case EV_SHAKE:
	{
		float len;

#ifdef FEATURE_EDV
		if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
		{
			break;
		}
#endif

		len = VectorDistance(cg.snap->ps.origin, cent->lerpOrigin);

		if (len > cent->currentState.onFireStart)
		{
			break;
		}

		len = 1.0f - (len / (float)cent->currentState.onFireStart);
		len = MIN(1.f, len);

		CG_StartShakeCamera(len);
	}
	break;

	case EV_ALERT_SPEAKER:
		switch (cent->currentState.otherEntityNum2)
		{
		case 1:
			CG_UnsetActiveOnScriptSpeaker(cent->currentState.otherEntityNum);
			break;
		case 2:
			CG_SetActiveOnScriptSpeaker(cent->currentState.otherEntityNum);
			break;
		case 0:
		default:
			CG_ToggleActiveOnScriptSpeaker(cent->currentState.otherEntityNum);
			break;
		}
		break;

	case EV_POPUPMESSAGE:
	{
		const char *str   = CG_GetPMItemText(cent);
		qhandle_t  shader = CG_GetPMItemIcon(cent);

		if (str)
		{
			CG_AddPMItem((popupMessageType_t)cent->currentState.effect1Time, str, " ", shader, 0, 0, colorWhite);
		}
		CG_PlayPMItemSound(cent);
	}
	break;
	case EV_AIRSTRIKEMESSAGE:
	{
		const char *wav = NULL;

#ifdef FEATURE_EDV
		if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
		{
			break;
		}
#endif
		switch (cent->currentState.density)
		{
		case 0:         // too many called
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
			{
				wav = "axis_hq_airstrike_denied";
			}
			else
			{
				wav = "allies_hq_airstrike_denied";
			}
			break;
		case 1:         // aborting can't see target
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
			{
				wav = "axis_hq_airstrike_abort";
			}
			else
			{
				wav = "allies_hq_airstrike_abort";
			}
			break;
		case 2:         // firing for effect
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
			{
				wav = "axis_hq_airstrike";
			}
			else
			{
				wav = "allies_hq_airstrike";
			}
			break;
		default:
			break;
		}

		if (wav)
		{
			CG_SoundPlaySoundScript(wav, NULL, -1, (es->effect1Time ? qfalse : qtrue));
		}
	}
	break;
	case EV_ARTYMESSAGE:
	{
		const char *wav = NULL;

#ifdef FEATURE_EDV
		if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
		{
			break;
		}
#endif
		switch (cent->currentState.density)
		{
		case 0:         // too many called
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
			{
				wav = "axis_hq_ffe_denied";
			}
			else
			{
				wav = "allies_hq_ffe_denied";
			}
			break;
		case 1:         // aborting can't see target
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
			{
				wav = "axis_hq_ffe_abort";
			}
			else
			{
				wav = "allies_hq_ffe_abort";
			}
			break;
		case 2:         // firing for effect
			if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
			{
				wav = "axis_hq_ffe";
			}
			else
			{
				wav = "allies_hq_ffe";
			}
			break;
		default:
			break;
		}

		if (wav)
		{
			CG_SoundPlaySoundScript(wav, NULL, -1, (es->effect1Time ? qfalse : qtrue));
		}
	}
	break;
	case EV_MEDIC_CALL:
		switch (cgs.clientinfo[cent->currentState.number].team)
		{
		// TODO: handle theses
//        case TEAM_FREE:
//        case TEAM_SPECTATOR:
//        case TEAM_NUM_TEAMS:
		case TEAM_AXIS:
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sndMedicCall[0]);
			break;
		case TEAM_ALLIES:
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sndMedicCall[1]);
			break;
		default:     // shouldn't happen
			break;
		}
		break;
	case EV_FLAG_INDICATOR:
		cg.flagIndicator   = es->eventParm;
		cg.redFlagCounter  = es->otherEntityNum;
		cg.blueFlagCounter = es->otherEntityNum2;
		break;
    case EV_MISSILE_FALLING:
        // Sound effect for spotter round, had to do this as half-second bomb warning
        if (cg_weapons[es->weapon].missileFallSound.count)
        {
            int i = cg_weapons[es->weapon].missileFallSound.count;

            i = rand() % i;

            trap_S_StartSoundExVControl(NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].missileFallSound.sounds[i], SND_OKTOCUT, 255);
        }
        break;
	case EV_PLAYER_HIT:
		CG_PlayHitSound(es->clientNum, es->eventParm);
		break;
	default:
		if (cg.demoPlayback)
		{
			CG_DPrintf("Unknown event: %i\n", event);
		}
		else
		{
			CG_Error("Unknown event: %i\n", event);
		}
		break;
	}

	{
		int rval = rand() & 3;

		if (splashfootstepcnt != rval)
		{
			splashfootstepcnt = rval;
		}
		else
		{
			splashfootstepcnt++;
		}

		if (splashfootstepcnt > 3)
		{
			splashfootstepcnt = 0;
		}


		if (footstepcnt != rval)
		{
			footstepcnt = rval;
		}
		else
		{
			footstepcnt++;
		}

		if (footstepcnt > 3)
		{
			footstepcnt = 0;
		}
	}
}

/**
 * @brief CG_CheckEvents
 * @param[in] cent
 */
void CG_CheckEvents(centity_t *cent)
{
	int i, event;

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin, qfalse, cent->currentState.effect2Time);
	CG_SetEntitySoundPosition(cent);

	// check for event-only entities
	if (cent->currentState.eType >= ET_EVENTS)
	{
		if (cent->previousEvent)
		{
			return;     // already fired
		}

		cent->previousEvent      = 1;
		cent->currentState.event = cent->currentState.eType - ET_EVENTS;

		CG_EntityEvent(cent, cent->lerpOrigin);
	}
	else
	{
		// Entities that make it here are Not TempEntities.
		//      As far as we could tell, for all non-TempEntities, the
		//      circular 'events' list contains the valid events.  So we
		//      skip processing the single 'event' field and go straight
		//      to the circular list.

		// check the sequencial list
		// if we've added more events than can fit into the list, make sure we only add them once
		if (cent->currentState.eventSequence < cent->previousEventSequence)
		{
			cent->previousEventSequence -= (1 << 8);      // eventSequence is sent as an 8-bit through network stream
		}

		if (cent->currentState.eventSequence - cent->previousEventSequence > MAX_EVENTS)
		{
			cent->previousEventSequence = cent->currentState.eventSequence - MAX_EVENTS;
		}

		for (i = cent->previousEventSequence ; i != cent->currentState.eventSequence; i++)
		{
			event = cent->currentState.events[i & (MAX_EVENTS - 1)];

			cent->currentState.event     = event;
			cent->currentState.eventParm = cent->currentState.eventParms[i & (MAX_EVENTS - 1)];

			CG_EntityEvent(cent, cent->lerpOrigin);
		}

		cent->previousEventSequence = cent->currentState.eventSequence;

		// set the event back so we don't think it's changed next frame (unless it really has)
		cent->currentState.event = cent->previousEvent;
	}
}
