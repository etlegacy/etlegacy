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
 * @file bg_classes.c
 */

#include "../qcommon/q_shared.h"
#include "bg_public.h"

bg_playerclass_t bg_playerClasses[2][NUM_PLAYER_CLASSES] =
{
	// Axis player classes
	{
		{
			PC_SOLDIER,
			"characters/temperate/axis/soldier.char",
			"ui/assets/mp_gun_red.tga",
			"ui/assets/mp_arrow_red.tga",
			{ WP_KNIFE,                SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MP40,                 SK_LIGHT_WEAPONS,                            0, 60,  30  }, // soldier start with x2 ammo
				{ WP_MOBILE_MG42,          SK_HEAVY_WEAPONS,                            0, 0,   150 },
				{ WP_FLAMETHROWER,         SK_HEAVY_WEAPONS,                            0, 200, 1   },
				{ WP_PANZERFAUST,          SK_HEAVY_WEAPONS,                            0, 4,   0   },
				{ WP_MORTAR2,              SK_HEAVY_WEAPONS,                            0, 15,  0   },
			},
			{
				{ WP_LUGER,                SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_LUGER,         SK_LIGHT_WEAPONS,                            4, 48,  8   },
				{ WP_MP40,                 SK_HEAVY_WEAPONS,                            4, 60,  30  }, // soldier start with x2 ammo
			},
			{ WP_GRENADE_LAUNCHER,     SK_LIGHT_WEAPONS,                            0, 4,   0   },
			{
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             1, 1,   0   },
			},
			0,
			0,
		},

		{
			PC_MEDIC,
			"characters/temperate/axis/medic.char",
			"ui/assets/mp_health_red.tga",
			"ui/assets/mp_arrow_red.tga",
			{ WP_KNIFE,                SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MP40,                 SK_LIGHT_WEAPONS,                            0, 0,   30  }, // medic start with 1 clip less
			},
			{
				{ WP_LUGER,                SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_LUGER,         SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_LAUNCHER,     SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MEDIC_SYRINGE,        SK_FIRST_AID,                                0, 10,  1   },
				{ WP_MEDIC_ADRENALINE,     SK_FIRST_AID,                                4, 0,   1   }, // share ammo with medic syringe
				{ WP_MEDKIT,               SK_FIRST_AID,                                0, 0,   1   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             1, 1,   0   },
			},
			0,
			0,
		},

		{
			PC_ENGINEER,
			"characters/temperate/axis/engineer.char",
			"ui/assets/mp_wrench_red.tga",
			"ui/assets/mp_arrow_red.tga",
			{ WP_KNIFE,                SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MP40,                 SK_LIGHT_WEAPONS,                            0, 30,  30  },
				{ WP_KAR98,                SK_LIGHT_WEAPONS,                            0, 20,  10  },
			},
			{
				{ WP_LUGER,                SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_LUGER,         SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_LAUNCHER,     SK_LIGHT_WEAPONS,                            0, 4,   0   },
			{
				{ WP_GPG40,                SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 4,   0   }, // riflenade as special weapon
				{ WP_DYNAMITE,             SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 0,   1   },
				{ WP_PLIERS,               SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 0,   1   },
				{ WP_LANDMINE,             SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 0,   1   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             1, 1,   0   },
			},
			0,
			0,
		},

		{
			PC_FIELDOPS,
			"characters/temperate/axis/fieldops.char",
			"ui/assets/mp_ammo_red.tga",
			"ui/assets/mp_arrow_red.tga",
			{ WP_KNIFE,                SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MP40,                 SK_LIGHT_WEAPONS,                            0, 30,  30  },
			},
			{
				{ WP_LUGER,                SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_LUGER,         SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_LAUNCHER,     SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_AMMO,                 SK_SIGNALS,                                  0, 0,   1   },
				{ WP_SMOKE_MARKER,         SK_SIGNALS,                                  0, 0,   1   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             0, 0,   1   },
			},
			0,
			0,
		},

		{
			PC_COVERTOPS,
			"characters/temperate/axis/cvops.char",
			"ui/assets/mp_spy_red.tga",
			"ui/assets/mp_arrow_red.tga",
			{ WP_KNIFE,                SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MP34,                 SK_LIGHT_WEAPONS,                            0, 64,  32  }, // covert-ops start with x2 ammo
				{ WP_FG42,                 SK_LIGHT_WEAPONS,                            0, 40,  20  },
				{ WP_K43,                  SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 20,  10  },
			},
			{
				{ WP_SILENCER,             SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_SILENCEDLUGER, SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_LAUNCHER,     SK_LIGHT_WEAPONS,                            0, 2,   0   },
			{
				{ WP_SMOKE_BOMB,           SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 0,   1   },
				{ WP_SATCHEL,              SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 0,   1   },
				{ WP_SATCHEL_DET,          SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 0,   0   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             0, 0,   1   },
			},
			0,
			0,
		},
	},

	// Allies player classes
	{
		{
			PC_SOLDIER,
			"characters/temperate/allied/soldier.char",
			"ui/assets/mp_gun_blue.tga",
			"ui/assets/mp_arrow_blue.tga",
			{ WP_KNIFE_KABAR,          SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_THOMPSON,             SK_LIGHT_WEAPONS,                            0, 60,  30  }, // soldier start with x2 ammo
				{ WP_MOBILE_BROWNING,      SK_HEAVY_WEAPONS,                            0, 0,   150 },
				{ WP_FLAMETHROWER,         SK_HEAVY_WEAPONS,                            0, 200, 0   },
				{ WP_BAZOOKA,              SK_HEAVY_WEAPONS,                            0, 4,   0   },
				{ WP_MORTAR,               SK_HEAVY_WEAPONS,                            0, 15,  0   },
			},
			{
				{ WP_COLT,                 SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_COLT,          SK_LIGHT_WEAPONS,                            4, 48,  8   },
				{ WP_THOMPSON,             SK_HEAVY_WEAPONS,                            4, 60,  30  }, // soldier start with x2 ammo
			},
			{ WP_GRENADE_PINEAPPLE,    SK_LIGHT_WEAPONS,                            0, 4,   0   },
			{
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             1, 1,   0   },
			},
			0,
			0,
		},

		{
			PC_MEDIC,
			"characters/temperate/allied/medic.char",
			"ui/assets/mp_health_blue.tga",
			"ui/assets/mp_arrow_blue.tga",
			{ WP_KNIFE_KABAR,          SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_THOMPSON,             SK_LIGHT_WEAPONS,                            0, 0,   30  }, // medic start with 1 clip less
			},
			{
				{ WP_COLT,                 SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_COLT,          SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_PINEAPPLE,    SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_MEDIC_SYRINGE,        SK_FIRST_AID,                                0, 10,  1   },
				{ WP_MEDIC_ADRENALINE,     SK_FIRST_AID,                                4, 0,   1   }, // share ammo with medic syringe
				{ WP_MEDKIT,               SK_FIRST_AID,                                0, 0,   1   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             1, 1,   0   },
			},
			0,
			0,
		},

		{
			PC_ENGINEER,
			"characters/temperate/allied/engineer.char",
			"ui/assets/mp_wrench_blue.tga",
			"ui/assets/mp_arrow_blue.tga",
			{ WP_KNIFE_KABAR,          SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_THOMPSON,             SK_LIGHT_WEAPONS,                            0, 30,  30  },
				{ WP_CARBINE,              SK_LIGHT_WEAPONS,                            0, 20,  10  },
			},
			{
				{ WP_COLT,                 SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_COLT,          SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_PINEAPPLE,    SK_LIGHT_WEAPONS,                            0, 4,   0   },
			{
				{ WP_M7,                   SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 4,   0   }, // riflenade as special weapon
				{ WP_DYNAMITE,             SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 0,   1   },
				{ WP_PLIERS,               SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 0,   1   },
				{ WP_LANDMINE,             SK_EXPLOSIVES_AND_CONSTRUCTION,              0, 0,   1   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             1, 1,   0   },
			},
			0,
			0,
		},

		{
			PC_FIELDOPS,
			"characters/temperate/allied/fieldops.char",
			"ui/assets/mp_ammo_blue.tga",
			"ui/assets/mp_arrow_blue.tga",
			{ WP_KNIFE_KABAR,          SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_THOMPSON,             SK_LIGHT_WEAPONS,                            0, 30,  30  },
			},
			{
				{ WP_COLT,                 SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_COLT,          SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_PINEAPPLE,    SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_AMMO,                 SK_SIGNALS,                                  0, 0,   1   },
				{ WP_SMOKE_MARKER,         SK_SIGNALS,                                  0, 0,   1   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             0, 0,   1   },
			},
			0,
			0,
		},

		{
			PC_COVERTOPS,
			"characters/temperate/allied/cvops.char",
			"ui/assets/mp_spy_blue.tga",
			"ui/assets/mp_arrow_blue.tga",
			{ WP_KNIFE_KABAR,          SK_LIGHT_WEAPONS,                            0, 1,   0   },
			{
				{ WP_STEN,                 SK_LIGHT_WEAPONS,                            0, 64,  32  }, // covert-ops start with x2 ammo
				{ WP_FG42,                 SK_LIGHT_WEAPONS,                            0, 40,  20  },
				{ WP_GARAND,               SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 20,  10  },
			},
			{
				{ WP_SILENCED_COLT,        SK_LIGHT_WEAPONS,                            0, 24,  8   },
				{ WP_AKIMBO_SILENCEDCOLT,  SK_LIGHT_WEAPONS,                            4, 48,  8   },
			},
			{ WP_GRENADE_PINEAPPLE,    SK_LIGHT_WEAPONS,                            0, 2,   0   },
			{
				{ WP_SMOKE_BOMB,           SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 0,   1   },
				{ WP_SATCHEL,              SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 0,   1   },
				{ WP_SATCHEL_DET,          SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 0, 0,   0   },
				{ WP_BINOCULARS,           SK_BATTLE_SENSE,                             0, 0,   1   },
			},
			0,
			0,
		},
	},
};

/**
 * @brief BG_GetPlayerClassInfo
 * @param[in] team
 * @param[in] cls
 * @return
 */
bg_playerclass_t *BG_GetPlayerClassInfo(int team, int cls)
{
	if (cls < PC_SOLDIER || cls >= NUM_PLAYER_CLASSES)
	{
		cls = PC_SOLDIER;
	}

	if (team != TEAM_AXIS && team != TEAM_ALLIES)
	{
		team = TEAM_AXIS;
	}

	return GetPlayerClassesData(team, cls);
}

/**
 * @brief BG_PlayerClassForPlayerState
 * @param[in] ps
 * @return
 */
bg_playerclass_t *BG_PlayerClassForPlayerState(playerState_t *ps)
{
	return BG_GetPlayerClassInfo(ps->persistant[PERS_TEAM], ps->stats[STAT_PLAYER_CLASS]);
}

/**
 * @brief BG_ClassHasWeapon
 * @param[in] classInfo
 * @param[in] weap
 * @return
 */
qboolean BG_ClassHasWeapon(bg_playerclass_t *classInfo, weapon_t weap)
{
	int i;

	if (!weap)
	{
		return qfalse;
	}

	for (i = 0; i < MAX_WEAPS_PER_CLASS; i++)
	{
		if (classInfo->classPrimaryWeapons[i].weapon == weap)
		{
			return qtrue;
		}
	}
	return qfalse;
}

/**
 * @brief BG_WeaponIsPrimaryForClassAndTeam
 * @param[in] classnum
 * @param[in] team
 * @param[in] weapon
 * @return
 */
qboolean BG_WeaponIsPrimaryForClassAndTeam(int classnum, team_t team, weapon_t weapon)
{
	if (team == TEAM_AXIS || team == TEAM_ALLIES)
	{
		return BG_ClassHasWeapon(GetPlayerClassesData(team, classnum), weapon);
	}

	return qfalse;
}

/*
 * @brief BG_ShortClassnameForNumber
 * @param[in] classNum
 * @return
 *
 * @note Unused
const char *BG_ShortClassnameForNumber(int classNum)
{
    switch (classNum)
    {
    case PC_SOLDIER:
        return "Soldr";
    case PC_MEDIC:
        return "Medic";
    case PC_ENGINEER:
        return "Engr";
    case PC_FIELDOPS:
        return "FdOps";
    case PC_COVERTOPS:
        return "CvOps";
    default:
        return "^1ERROR";
    }
}
*/

/**
 * @brief BG_ClassnameForNumber
 * @param[in] classNum
 * @return
 */
const char *BG_ClassnameForNumber(int classNum)
{
	switch (classNum)
	{
	case PC_SOLDIER:
		return "Soldier";
	case PC_MEDIC:
		return "Medic";
	case PC_ENGINEER:
		return "Engineer";
	case PC_FIELDOPS:
		return "Field Ops";
	case PC_COVERTOPS:
		return "Covert Ops";
	default:
		return "^1ERROR";
	}
}

/**
 * @brief BG_ClassnameForNumberFilename
 * @param[in] classNum
 * @return
 */
const char *BG_ClassnameForNumberFilename(int classNum)
{
	switch (classNum)
	{
	case PC_SOLDIER:
		return "soldier";
	case PC_MEDIC:
		return "medic";
	case PC_ENGINEER:
		return "engineer";
	case PC_FIELDOPS:
		return "fieldops";
	case PC_COVERTOPS:
		return "covertops";
	default:
		return "^1ERROR";
	}
}

/*
 * @brief BG_ClassLetterForNumber
 * @param[in] classNum
 * @return
 *
 * @note Unused
const char *BG_ClassLetterForNumber(int classNum)
{
    switch (classNum)
    {
    case PC_SOLDIER:
        return "S";
    case PC_MEDIC:
        return "M";
    case PC_ENGINEER:
        return "E";
    case PC_FIELDOPS:
        return "F";
    case PC_COVERTOPS:
        return "C";
    default:
        return "^1E";
    }
}
*/

/*
 * @brief BG_ClassTextToClass
 * @param[in] token
 * @return
 *
 * @note Unused
int BG_ClassTextToClass(const char *token)
{
    if (!Q_stricmp(token, "soldier"))
    {
        return PC_SOLDIER;
    }
    else if (!Q_stricmp(token, "medic"))
    {
        return PC_MEDIC;
    }
    else if (!Q_stricmp(token, "fieldops"))
    {
        return PC_FIELDOPS;
    }
    else if (!Q_stricmp(token, "engineer"))
    {
        return PC_ENGINEER;
    }
    else if (!Q_stricmp(token, "covertops"))
    {
        return PC_COVERTOPS;
    }

    return -1;
}
*/

/**
 * @brief BG_TeamnameForNumber
 * @param[in] teamNum
 * @return
 */
const char *BG_TeamnameForNumber(team_t teamNum)
{
	switch (teamNum)
	{
	case TEAM_FREE:
		return "free";
	case TEAM_AXIS:
		return "axis";
	case TEAM_ALLIES:
		return "allies";
	case TEAM_SPECTATOR:
		return "spectator";
	default:
		return "^1ERROR";
	}
}

skillType_t classskill[NUM_PLAYER_CLASSES] = { SK_HEAVY_WEAPONS, SK_FIRST_AID, SK_EXPLOSIVES_AND_CONSTRUCTION, SK_SIGNALS, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS };

/**
 * @brief BG_ClassSkillForClass
 * @param[in] classnum
 * @return
 */
skillType_t BG_ClassSkillForClass(int classnum)
{
	if (classnum < 0 || classnum >= NUM_PLAYER_CLASSES)
	{
		return SK_BATTLE_SENSE;
	}

	return classskill[classnum];
}
