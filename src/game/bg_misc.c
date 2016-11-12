/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2016 ET:Legacy team <mail@etlegacy.com>
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
 * @file bg_misc.c
 * @brief Both games misc functions, all completely stateless
 */

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "../../etmain/ui/menudef.h"

#ifdef GAMEDLL
extern vmCvar_t g_developer;
#endif

const char *skillNames[SK_NUM_SKILLS] =
{
	"Battle Sense",
	"Engineering",
	"First Aid",
	"Signals",
	"Light Weapons",
	"Heavy Weapons",
	"Covert Ops"
};

const char *skillNamesLine1[SK_NUM_SKILLS] =
{
	"Battle",
	"Engineering",
	"First",
	"Signals",
	"Light",
	"Heavy",
	"Covert"
};

const char *skillNamesLine2[SK_NUM_SKILLS] =
{
	"Sense",
	"",
	"Aid",
	"",
	"Weapons",
	"Weapons",
	"Ops"
};

const char *medalNames[SK_NUM_SKILLS] =
{
	"Distinguished Service Medal",
	"Steel Star",
	"Silver Cross",
	"Signals Medal",
	"Infantry Medal",
	"Bombardment Medal",
	"Silver Snake"
};

int skillLevels[SK_NUM_SKILLS][NUM_SKILL_LEVELS] =
{
	{ 0, 20, 50, 90, 140 },
	{ 0, 20, 50, 90, 140 },
	{ 0, 20, 50, 90, 140 },
	{ 0, 20, 50, 90, 140 },
	{ 0, 20, 50, 90, 140 },
	{ 0, 20, 50, 90, 140 },
	{ 0, 20, 50, 90, 140 },
};

vec3_t playerlegsProneMins = { -13.5f, -13.5f, -24.f };
vec3_t playerlegsProneMaxs = { 13.5f, 13.5f, -14.4f };

int          numSplinePaths;
splinePath_t splinePaths[MAX_SPLINE_PATHS];

int          numPathCorners;
pathCorner_t pathCorners[MAX_PATH_CORNERS];

// these defines are matched with the character torso animations
#define DELAY_LOW       100 // machineguns, tesla, spear, flame
#define DELAY_HIGH      100 // mauser, garand
#define DELAY_PISTOL    100 // colt, luger, sp5, cross
#define DELAY_SHOULDER  50  // rl
#define DELAY_THROW     250 // grenades, dynamite
#define DELAY_HW        750

// Using one unified list for which weapons can received ammo
// This is used both by the ammo pack code and by the bot code to determine if reloads are needed
const weapon_t reloadableWeapons[] =
{
	WP_MP40,                WP_THOMPSON,             WP_STEN,            WP_GARAND,       WP_PANZERFAUST, WP_FLAMETHROWER,
	WP_KAR98,               WP_CARBINE,              WP_FG42,            WP_K43,          WP_MOBILE_MG42, WP_COLT,
	WP_LUGER,               WP_MORTAR,               WP_AKIMBO_COLT,     WP_AKIMBO_LUGER, WP_M7,          WP_GPG40,
	WP_AKIMBO_SILENCEDCOLT, WP_AKIMBO_SILENCEDLUGER, WP_MOBILE_BROWNING, WP_MORTAR2,      WP_BAZOOKA,
	WP_NONE
};

// [0]  = maxammo        -   max player ammo carrying capacity.
// [1]  = uses           -   how many 'rounds' it takes/costs to fire one cycle.
// [2]  = maxclip        -   max 'rounds' in a clip.
// [3]  = startammo      -   player ammo when spawning.
// [4]  = startclip      -   player clips when spawning.
// [5]  = reloadTime     -   time from start of reload until ready to fire.
// [6]  = fireDelayTime  -   time from pressing 'fire' until first shot is fired. (used for delaying fire while weapon is 'readied' in animation)
// [7]  = nextShotTime   -   when firing continuously, this is the time between shots
// [8]  = maxHeat        -   max active firing time before weapon 'overheats' (at which point the weapon will fail for a moment)
// [9]  = coolRate       -   how fast the weapon cools down.
// [10] = mod            -   means of death.
ammotable_t ammoTableMP[WP_NUM_WEAPONS] =
{
	//  MAX             USES    MAX     START   START  RELOAD   FIRE            NEXT    HEAT,   COOL,   MOD,    ...
	//  AMMO            AMT.    CLIP    AMMO    CLIP    TIME    DELAY           SHOT
	{ 0,   0, 0,   0,  0,   0,    50,           0,    0,    0,   0                        },  // WP_NONE                  // 0
	{ 999, 0, 999, 0,  0,   0,    50,           200,  0,    0,   MOD_KNIFE                },  // WP_KNIFE                 // 1
	{ 24,  1, 8,   24, 8,   1500, DELAY_PISTOL, 400,  0,    0,   MOD_LUGER                },  // WP_LUGER                 // 2    // NOTE: also 32 round 'snail' magazine
	{ 90,  1, 30,  30, 30,  2400, DELAY_LOW,    150,  0,    0,   MOD_MP40                 },  // WP_MP40                  // 3
	{ 45,  1, 15,  0,  4,   1000, DELAY_THROW,  1600, 0,    0,   MOD_GRENADE_LAUNCHER     },  // WP_GRENADE_LAUNCHER      // 4
	{ 4,   1, 1,   0,  4,   1000, DELAY_HW,     2000, 0,    0,   MOD_PANZERFAUST          },  // WP_PANZERFAUST           // 5    // updated delay so prediction is correct
	{ 200, 1, 200, 0,  200, 1000, DELAY_LOW,    50,   0,    0,   MOD_FLAMETHROWER         },  // WP_FLAMETHROWER          // 6
	{ 24,  1, 8,   24, 8,   1500, DELAY_PISTOL, 400,  0,    0,   MOD_COLT                 },  // WP_COLT                  // 7
	{ 90,  1, 30,  30, 30,  2400, DELAY_LOW,    150,  0,    0,   MOD_THOMPSON             },  // WP_THOMPSON              // 8
	{ 45,  1, 15,  0,  4,   1000, DELAY_THROW,  1600, 0,    0,   MOD_GRENADE_PINEAPPLE    },  // WP_GRENADE_PINEAPPLE     // 9

	{ 96,  1, 32,  32, 32,  3100, DELAY_LOW,    150,  1200, 450, MOD_STEN                 },  // WP_STEN                  // 10
	{ 10,  1, 1,   0,  10,  1500, 50,           1000, 0,    0,   MOD_SYRINGE              },  // WP_MEDIC_SYRINGE         // 11
	{ 1,   0, 1,   0,  0,   3000, 50,           1000, 0,    0,   MOD_AMMO,                },  // WP_AMMO                  // 12
	{ 1,   0, 1,   0,  1,   3000, 50,           1000, 0,    0,   MOD_ARTY,                },  // WP_ARTY                  // 13
	{ 24,  1, 8,   24, 8,   1500, DELAY_PISTOL, 400,  0,    0,   MOD_SILENCER             },  // WP_SILENCER              // 14
	{ 1,   0, 10,  0,  0,   1000, DELAY_THROW,  1600, 0,    0,   MOD_DYNAMITE             },  // WP_DYNAMITE              // 15
	{ 999, 0, 999, 0,  0,   0,    50,           0,    0,    0,   0                        },  // WP_SMOKETRAIL            // 16
	{ 999, 0, 999, 0,  0,   0,    50,           0,    0,    0,   0                        },  // WP_MAPMORTAR             // 17
	{ 999, 0, 999, 0,  0,   0,    50,           0,    0,    0,   0                        },  // VERYBIGEXPLOSION         // 18
	{ 999, 0, 999, 1,  1,   0,    50,           0,    0,    0,   0                        },  // WP_MEDKIT                // 19

	{ 999, 0, 999, 0,  0,   0,    50,           0,    0,    0,   0                        },  // WP_BINOCULARS            // 20
	{ 999, 0, 999, 0,  0,   0,    50,           0,    0,    0,   0                        },  // WP_PLIERS                // 21
	{ 999, 0, 999, 0,  1,   0,    50,           0,    0,    0,   MOD_AIRSTRIKE            },  // WP_SMOKE_MARKER          // 22
	{ 30,  1, 10,  20, 10,  1500, DELAY_LOW,    400,  0,    0,   MOD_KAR98                },  // WP_KAR98                 // 23       K43
	{ 30,  1, 10,  20, 10,  1500, DELAY_LOW,    400,  0,    0,   MOD_CARBINE              },  // WP_CARBINE               // 24       GARAND old max ammo 24 max clip size 8 start ammo 16 start clip 8
	{ 30,  1, 10,  20, 10,  1500, DELAY_LOW,    400,  0,    0,   MOD_GARAND               },  // WP_GARAND                // 25       GARAND old max ammo 24 max clip size 8 start ammo 16 start clip 8
	{ 1,   0, 1,   0,  1,   100,  DELAY_LOW,    100,  0,    0,   MOD_LANDMINE             },  // WP_LANDMINE              // 26
	{ 1,   0, 1,   0,  0,   3000, DELAY_LOW,    2000, 0,    0,   MOD_SATCHEL              },  // WP_SATCHEL               // 27
	{ 1,   0, 1,   0,  0,   3000, 722,          2000, 0,    0,   0,                       },  // WP_SATCHEL_DET           // 28
	{ 1,   0, 10,  0,  1,   1000, DELAY_THROW,  1600, 0,    0,   MOD_SMOKEBOMB            },  // WP_SMOKE_BOMB            // 29

	{ 450, 1, 150, 0,  150, 3000, DELAY_LOW,    66,   1500, 300, MOD_MOBILE_MG42          },  // WP_MOBILE_MG42           // 30
	{ 30,  1, 10,  20, 10,  1500, DELAY_LOW,    400,  0,    0,   MOD_K43                  },  // WP_K43                   // 31       K43
	{ 60,  1, 20,  40, 20,  2000, DELAY_LOW,    100,  0,    0,   MOD_FG42                 },  // WP_FG42                  // 32
	{ 0,   0, 0,   0,  0,   0,    0,            0,    1500, 300, 0                        },  // WP_DUMMY_MG42            // 33
	{ 15,  1, 1,   0,  0,   0,    DELAY_HW,     1600, 0,    0,   MOD_MORTAR               },  // WP_MORTAR                // 34
	{ 48,  1, 8,   48, 8,   2700, DELAY_PISTOL, 200,  0,    0,   MOD_AKIMBO_COLT          },  // WP_AKIMBO_COLT           // 35
	{ 48,  1, 8,   48, 8,   2700, DELAY_PISTOL, 200,  0,    0,   MOD_AKIMBO_LUGER         },  // WP_AKIMBO_LUGER          // 36
	{ 4,   1, 1,   4,  1,   3000, DELAY_LOW,    400,  0,    0,   MOD_GPG40                },  // WP_GPG40                 // 37
	{ 4,   1, 1,   4,  1,   3000, DELAY_LOW,    400,  0,    0,   MOD_M7                   },  // WP_M7                    // 38
	{ 24,  1, 8,   24, 8,   1500, DELAY_PISTOL, 400,  0,    0,   MOD_SILENCED_COLT        },  // WP_SILENCED_COLT         // 39

	{ 30,  1, 10,  20, 10,  1500, 0,            400,  0,    0,   MOD_GARAND_SCOPE         },  // WP_GARAND_SCOPE          // 40       GARAND  old max ammo 24 max clip size 8 start ammo 16 start clip 8
	{ 30,  1, 10,  20, 10,  1500, 0,            400,  0,    0,   MOD_K43_SCOPE            },  // WP_K43_SCOPE             // 41       K43
	{ 60,  1, 20,  40, 20,  2000, DELAY_LOW,    400,  0,    0,   MOD_FG42SCOPE            },  // WP_FG42SCOPE             // 42
	{ 16,  1, 1,   12, 0,   0,    DELAY_HW,     1400, 0,    0,   MOD_MORTAR               },  // WP_MORTAR_SET            // 43
	{ 10,  1, 1,   0,  10,  1500, 50,           1000, 0,    0,   MOD_SYRINGE              },  // WP_MEDIC_ADRENALINE      // 44
	{ 48,  1, 8,   48, 8,   2700, DELAY_PISTOL, 200,  0,    0,   MOD_AKIMBO_SILENCEDCOLT  },  // WP_AKIMBO_SILENCEDCOLT   // 45
	{ 48,  1, 8,   48, 8,   2700, DELAY_PISTOL, 200,  0,    0,   MOD_AKIMBO_SILENCEDLUGER },  // WP_AKIMBO_SILENCEDLUGER  // 46
	{ 450, 1, 150, 0,  150, 3000, DELAY_LOW,    66,   1500, 300, MOD_MOBILE_MG42          },  // WP_MOBILE_MG42_SET       // 47
	{ 999, 0, 999, 0,  0,   0,    50,           200,  0,    0,   MOD_KNIFE_KABAR          },  // WP_KNIFE_KABAR           // 48
	{ 450, 1, 150, 0,  150, 3000, DELAY_LOW,    66,   1500, 300, MOD_MOBILE_BROWNING      },  // WP_MOBILE_BROWNING       // 49

	{ 450, 1, 150, 0,  150, 3000, DELAY_LOW,    66,   1500, 300, MOD_MOBILE_BROWNING      },  // WP_MOBILE_BROWNING_SET   // 50
	{ 15,  1, 1,   0,  0,   0,    DELAY_HW,     1600, 0,    0,   MOD_MORTAR2              },  // WP_MORTAR2				  // 51
	{ 16,  1, 1,   12, 0,   0,    DELAY_HW,     1400, 0,    0,   MOD_MORTAR2              },  // WP_MORTAR2_SET			  // 52
	{ 4,   1, 1,   0,  4,   1000, DELAY_HW,     2000, 0,    0,   MOD_BAZOOKA              },  // WP_BAZOOKA               // 53
};

// WIP: New weapon table (similar to ammoTableMP) to store common weapon properties
// This will save us tons of switches, creates better code and might be populated by custom entries one day
// FIXME: fill me & use!
// ... we might merge it later on with ammoTableMP

// damage - returns 1 for no damage ... FIXME: some weapons are handled differently f.e. VERYBIGEXPLOSION
// spread - bullet weapons only

// [0]  = weapon         -
// [1]  = weapAlts       -
// [2]  = akimboSideram
// [3]  = ammoIndex
// [4]  = clipIndex
// [5]  = isScoped
// [6]  = LightWeaponSupportingFastReload (isLWSF)
// [7]  = damage         -
// [8]  = canGib
// [9]  = isReload       - some weapons don't reload
// [10] = spread         -
// [11] = desc
weaponTable_t weaponTable[WP_NUM_WEAPONS] =
{
	// weapon                  weapAlts          akimboSidearm   ammoIndex             clipIndex           isScoped isLWSF damage canGib isRealod spread desc     isLightWeaponSupportingFastReload
	{ WP_NONE,                 WP_NONE,                WP_NONE,  WP_NONE,              WP_NONE,              qfalse, qfalse, 1,   qfalse, qfalse, 0,    "WP_NONE",             }, // 0
	{ WP_KNIFE,                WP_NONE,                WP_NONE,  WP_KNIFE,             WP_KNIFE,             qfalse, qfalse, 10,  qtrue,  qfalse, 0,    "KNIFE",               }, // 1
	{ WP_LUGER,                WP_SILENCER,            WP_NONE,  WP_LUGER,             WP_LUGER,             qfalse, qtrue,  18,  qfalse, qtrue,  600,  "LUGER",               }, // 2
	{ WP_MP40,                 WP_NONE,                WP_NONE,  WP_MP40,              WP_MP40,              qfalse, qtrue,  18,  qfalse, qtrue,  400,  "MP 40",               }, // 3
	{ WP_GRENADE_LAUNCHER,     WP_NONE,                WP_NONE,  WP_GRENADE_LAUNCHER,  WP_GRENADE_LAUNCHER,  qfalse, qfalse, 250, qtrue,  qfalse, 0,    "",                    }, // 4
	{ WP_PANZERFAUST,          WP_NONE,                WP_NONE,  WP_PANZERFAUST,       WP_PANZERFAUST,       qfalse, qfalse, 400, qtrue,  qfalse, 0,    "PANZERFAUST",         }, // 5
	{ WP_FLAMETHROWER,         WP_NONE,                WP_NONE,  WP_FLAMETHROWER,      WP_FLAMETHROWER,      qfalse, qfalse, 5,   qfalse, qfalse, 0,    "FLAMETHROWER",        }, // 6
	{ WP_COLT,                 WP_SILENCED_COLT,       WP_NONE,  WP_COLT,              WP_COLT,              qfalse, qtrue,  18,  qfalse, qtrue,  600,  "COLT",                }, // 7	// equivalent american weapon to german luger
	{ WP_THOMPSON,             WP_NONE,                WP_NONE,  WP_THOMPSON,          WP_THOMPSON,          qfalse, qtrue,  18,  qfalse, qtrue,  400,  "THOMPSON",            }, // 8	// equivalent american weapon to german mp40
	{ WP_GRENADE_PINEAPPLE,    WP_NONE,                WP_NONE,  WP_GRENADE_PINEAPPLE, WP_GRENADE_PINEAPPLE, qfalse, qfalse, 250, qtrue,  qfalse, 0,    "",                    }, // 9

	{ WP_STEN,                 WP_NONE,                WP_NONE,  WP_STEN,              WP_STEN,              qfalse, qtrue,  14,  qfalse, qtrue,  200,  "STEN",                }, // 10	// silenced sten sub-machinegun
	{ WP_MEDIC_SYRINGE,        WP_NONE,                WP_NONE,  WP_MEDIC_SYRINGE,     WP_MEDIC_SYRINGE,     qfalse, qfalse, 1,   qfalse, qfalse, 0,    "MEDIC",               }, // 11	// broken out from CLASS_SPECIAL per Id request
	{ WP_AMMO,                 WP_NONE,                WP_NONE,  WP_AMMO,              WP_AMMO,              qfalse, qfalse, 1,   qfalse, qfalse, 0,    "AMMO",                }, // 12	// likewise
	{ WP_ARTY,                 WP_NONE,                WP_NONE,  WP_ARTY,              WP_ARTY,              qfalse, qfalse, 1,   qtrue,  qfalse, 0,    "ARTY",                }, // 13
	{ WP_SILENCER,             WP_LUGER,               WP_NONE,  WP_LUGER,             WP_LUGER,             qfalse, qtrue,  18,  qfalse, qtrue,  600,  "SILENCED LUGER",      }, // 14	// used to be sp5
	{ WP_DYNAMITE,             WP_NONE,                WP_NONE,  WP_DYNAMITE,          WP_DYNAMITE,          qfalse, qfalse, 400, qtrue,  qfalse, 0,    "DYNAMITE",            }, // 15
	{ WP_SMOKETRAIL,           WP_NONE,                WP_NONE,  WP_SMOKETRAIL,        WP_SMOKETRAIL,        qfalse, qfalse, 1,   qfalse, qfalse, 0,    "",                    }, // 16
	{ WP_MAPMORTAR,            WP_NONE,                WP_NONE,  WP_MAPMORTAR,         WP_MAPMORTAR,         qfalse, qfalse, 250, qtrue,  qfalse, 0,    "",                    }, // 17
	{ VERYBIGEXPLOSION,        WP_NONE,                WP_NONE,  WP_NONE,              WP_NONE,              qfalse, qfalse, 1,   qtrue,  qfalse, 0,    "",                    }, // 18	// explosion effect for airplanes
	{ WP_MEDKIT,               WP_NONE,                WP_NONE,  WP_MEDKIT,            WP_MEDKIT,            qfalse, qfalse, 1,   qfalse, qfalse, 0,    "",                    }, // 19

	{ WP_BINOCULARS,           WP_NONE,                WP_NONE,  WP_BINOCULARS,        WP_BINOCULARS,        qfalse, qfalse, 1,   qfalse, qfalse, 0,    "",                    }, // 20
	{ WP_PLIERS,               WP_NONE,                WP_NONE,  WP_PLIERS,            WP_PLIERS,            qfalse, qfalse, 1,   qfalse, qfalse, 0,    "PLIERS",              }, // 21
	{ WP_SMOKE_MARKER,         WP_NONE,                WP_NONE,  WP_SMOKE_MARKER,      WP_SMOKE_MARKER,      qfalse, qfalse, 140, qtrue,  qfalse, 0,    "",                    }, // 22	// changed name to cause less confusion
	{ WP_KAR98,                WP_GPG40,               WP_NONE,  WP_KAR98,             WP_KAR98,             qfalse, qfalse, 34,  qfalse, qtrue,  250,  "K43",                 }, // 23	// WolfXP weapons
	{ WP_CARBINE,              WP_M7,                  WP_NONE,  WP_CARBINE,           WP_CARBINE,           qfalse, qfalse, 34,  qfalse, qtrue,  250,  "M1 GARAND",           }, // 24
	{ WP_GARAND,               WP_GARAND_SCOPE,        WP_NONE,  WP_GARAND,            WP_GARAND,            qfalse, qfalse, 34,  qfalse, qtrue,  250,  "SCOPED M1 GARAND",    }, // 25
	{ WP_LANDMINE,             WP_NONE,                WP_NONE,  WP_LANDMINE,          WP_LANDMINE,          qfalse, qfalse, 250, qtrue,  qfalse, 0,    "",                    }, // 26
	{ WP_SATCHEL,              WP_NONE,                WP_NONE,  WP_SATCHEL,           WP_SATCHEL,           qfalse, qfalse, 250, qtrue,  qfalse, 0,    "SATCHEL",             }, // 27
	{ WP_SATCHEL_DET,          WP_NONE,                WP_NONE,  WP_SATCHEL_DET,       WP_SATCHEL_DET,       qfalse, qfalse, 1,   qtrue,  qfalse, 0,    "SATCHEL",             }, // 28
	{ WP_SMOKE_BOMB,           WP_NONE,                WP_NONE,  WP_SMOKE_BOMB,        WP_SMOKE_BOMB,        qfalse, qfalse, 1,   qfalse, qfalse, 0,    "",                    }, // 29

	{ WP_MOBILE_MG42,          WP_MOBILE_MG42_SET,     WP_NONE,  WP_MOBILE_MG42,       WP_MOBILE_MG42,       qfalse, qfalse, 18,  qfalse, qtrue,  2500, "MOBILE MG 42",        }, // 30
	{ WP_K43,                  WP_K43_SCOPE,           WP_NONE,  WP_K43,               WP_K43,               qfalse, qfalse, 34,  qfalse, qtrue,  250,  "SCOPED K43",          }, // 31
	{ WP_FG42,                 WP_FG42SCOPE,           WP_NONE,  WP_FG42,              WP_FG42,              qfalse, qtrue,  16,  qfalse, qtrue,  500,  "FG 42",               }, // 32
	{ WP_DUMMY_MG42,           WP_NONE,                WP_NONE,  WP_DUMMY_MG42,        WP_DUMMY_MG42,        qfalse, qfalse, 1,   qfalse, qfalse, 0,    "",                    }, // 33   // for storing heat on mounted mg42s...
	{ WP_MORTAR,               WP_MORTAR_SET,          WP_NONE,  WP_MORTAR,            WP_MORTAR,            qfalse, qfalse, 1,   qtrue,  qtrue,  0,    "MORTAR",              }, // 34
	{ WP_AKIMBO_COLT,          WP_NONE,                WP_COLT,  WP_COLT,              WP_AKIMBO_COLT,       qfalse, qfalse, 18,  qfalse, qtrue,  600,  "AKIMBO COLTS",        }, // 35
	{ WP_AKIMBO_LUGER,         WP_NONE,                WP_LUGER, WP_LUGER,             WP_AKIMBO_LUGER,      qfalse, qfalse, 18,  qfalse, qtrue,  600,  "AKIMBO LUGERS",       }, // 36

	{ WP_GPG40,                WP_KAR98,               WP_NONE,  WP_GPG40,             WP_GPG40,             qfalse, qfalse, 250, qtrue,  qfalse, 0,    "",                    }, // 37
	{ WP_M7,                   WP_CARBINE,             WP_NONE,  WP_M7,                WP_M7,                qfalse, qfalse, 250, qtrue,  qfalse, 0,    "",                    }, // 38
	{ WP_SILENCED_COLT,        WP_COLT,                WP_NONE,  WP_COLT,              WP_COLT,              qfalse, qfalse, 18,  qfalse, qtrue,  600,  "SILENCED COLT",       }, // 39

	{ WP_GARAND_SCOPE,         WP_GARAND,              WP_NONE,  WP_GARAND,            WP_GARAND,            qtrue,  qfalse, 50,  qfalse, qtrue,  700,  "",                    }, // 40
	{ WP_K43_SCOPE,            WP_K43,                 WP_NONE,  WP_K43,               WP_K43,               qtrue,  qfalse, 50,  qfalse, qtrue,  700,  "",                    }, // 41
	{ WP_FG42SCOPE,            WP_FG42,                WP_NONE,  WP_FG42,              WP_FG42,              qtrue,  qfalse, 30,  qfalse, qtrue,  200,  "FG 42",               }, // 42
	{ WP_MORTAR_SET,           WP_MORTAR,              WP_NONE,  WP_MORTAR,            WP_MORTAR,            qfalse, qfalse, 400, qtrue,  qtrue,  0,    "MORTAR",              }, // 43
	{ WP_MEDIC_ADRENALINE,     WP_NONE,                WP_NONE,  WP_MEDIC_SYRINGE,     WP_MEDIC_SYRINGE,     qfalse, qfalse, 1,   qfalse, qfalse, 0,    "ADRENALINE",          }, // 44
	{ WP_AKIMBO_SILENCEDCOLT,  WP_NONE,                WP_COLT,  WP_COLT,              WP_AKIMBO_COLT,       qfalse, qfalse, 18,  qfalse, qtrue,  600,  "SLNCD AKIMBO COLTS",  }, // 45
	{ WP_AKIMBO_SILENCEDLUGER, WP_NONE,                WP_LUGER, WP_LUGER,             WP_AKIMBO_LUGER,      qfalse, qfalse, 18,  qfalse, qtrue,  600,  "SLNCD AKIMBO LUGERS", }, // 46
	{ WP_MOBILE_MG42_SET,      WP_MOBILE_MG42,         WP_NONE,  WP_MOBILE_MG42,       WP_MOBILE_MG42,       qfalse, qfalse, 18,  qfalse, qtrue,  2500, "MOBILE MG 42",        }, // 47

	// legacy weapons
	{ WP_KNIFE_KABAR,          WP_NONE,                WP_NONE,  WP_KNIFE_KABAR,       WP_KNIFE_KABAR,       qfalse, qfalse, 10,  qtrue,  qfalse, 0,    "KABAR",               }, // 48
	{ WP_MOBILE_BROWNING,      WP_MOBILE_BROWNING_SET, WP_NONE,  WP_MOBILE_BROWNING,   WP_MOBILE_BROWNING,   qfalse, qfalse, 18,  qfalse, qtrue,  2500, "MOBILE BROWNING",     }, // 49
	{ WP_MOBILE_BROWNING_SET,  WP_MOBILE_BROWNING,     WP_NONE,  WP_MOBILE_BROWNING,   WP_MOBILE_BROWNING,   qfalse, qfalse, 18,  qfalse, qtrue,  2500, "MOBILE BROWNING",     }, // 50
	{ WP_MORTAR2,              WP_MORTAR2_SET,         WP_NONE,  WP_MORTAR2,           WP_MORTAR2,           qfalse, qfalse, 1,   qtrue,  qtrue,  0,    "GRANATWERFER",        }, // 51
	{ WP_MORTAR2_SET,          WP_MORTAR2,             WP_NONE,  WP_MORTAR2,           WP_MORTAR2,           qfalse, qfalse, 400, qtrue,  qtrue,  0,    "GRANATWERFER",        }, // 52
	{ WP_BAZOOKA,              WP_NONE,                WP_NONE,  WP_BAZOOKA,           WP_BAZOOKA,           qfalse, qfalse, 400, qtrue,  qfalse, 0,    "BAZOOKA",             }, // 53
};

// WIP: New weapon table for mod properties
// FIXME: add client side properties

// [0]  = mod         -
// [1]  = isHeadshot
// [2]  = isExplosive
modTable_t modTable[MOD_NUM_MODS] =
{
	{ MOD_UNKNOWN,                            qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_MACHINEGUN,                         qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_BROWNING,                           qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_MG42,                               qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_GRENADE,                            qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },

	{ MOD_KNIFE,                              qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_LUGER,                              qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_COLT,                               qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_MP40,                               qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_THOMPSON,                           qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_STEN,                               qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_GARAND,                             qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_SILENCER,                           qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_FG42,                               qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_FG42SCOPE,                          qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_PANZERFAUST,                        qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_GRENADE_LAUNCHER,                   qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_FLAMETHROWER,                       qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_GRENADE_PINEAPPLE,                  qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },

	{ MOD_MAPMORTAR,                          qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_MAPMORTAR_SPLASH,                   qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },

	{ MOD_KICKED,                             qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_DYNAMITE,                           qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_DYNAMITE  },
	{ MOD_AIRSTRIKE,                          qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_SYRINGE,                            qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_AMMO,                               qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_ARTY,                               qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },

	{ MOD_WATER,                              qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_SLIME,                              qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_LAVA,                               qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_CRUSH,                              qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_TELEFRAG,                           qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_FALLING,                            qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_SUICIDE,                            qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_TARGET_LASER,                       qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_TRIGGER_HURT,                       qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_EXPLOSIVE,                          qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },

	{ MOD_CARBINE,                            qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_KAR98,                              qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_GPG40,                              qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_M7,                                 qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_LANDMINE,                           qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_SATCHEL,                            qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_SATCHEL   },

	{ MOD_SMOKEBOMB,                          qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_MOBILE_MG42,                        qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_SILENCED_COLT,                      qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_GARAND_SCOPE,                       qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_CRUSH_CONSTRUCTION,                 qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_CRUSH_CONSTRUCTIONDEATH,            qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER, qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_K43,                                qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_K43_SCOPE,                          qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_MORTAR,                             qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },

	{ MOD_AKIMBO_COLT,                        qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_AKIMBO_LUGER,                       qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_AKIMBO_SILENCEDCOLT,                qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_AKIMBO_SILENCEDLUGER,               qtrue,  qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_SMOKEGRENADE,                       qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_SWAP_PLACES,                        qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_SWITCHTEAM,                         qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_SHOVE,                              qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },

	{ MOD_KNIFE_KABAR,                        qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_MOBILE_BROWNING,                    qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
	{ MOD_MORTAR2,                            qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_BAZOOKA,                            qfalse, qtrue,  WEAPON_CLASS_FOR_MOD_EXPLOSIVE },
	{ MOD_BACKSTAB,                           qfalse, qfalse, WEAPON_CLASS_FOR_MOD_NO        },
};

const char *animStrings[] =
{
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEAD1_WATER",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEAD2_WATER",
	"BOTH_DEATH3",
	"BOTH_DEAD3",
	"BOTH_DEAD3_WATER",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_GRAB_GRENADE",

	"BOTH_ATTACK1",
	"BOTH_ATTACK2",
	"BOTH_ATTACK3",
	"BOTH_ATTACK4",
	"BOTH_ATTACK5",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",
	"BOTH_EXTRA6",
	"BOTH_EXTRA7",
	"BOTH_EXTRA8",
	"BOTH_EXTRA9",
	"BOTH_EXTRA10",
	"BOTH_EXTRA11",
	"BOTH_EXTRA12",
	"BOTH_EXTRA13",
	"BOTH_EXTRA14",
	"BOTH_EXTRA15",
	"BOTH_EXTRA16",
	"BOTH_EXTRA17",
	"BOTH_EXTRA18",
	"BOTH_EXTRA19",
	"BOTH_EXTRA20",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",        // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_STAND_ALT1",
	"TORSO_STAND_ALT2",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",       // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_STAND2_ALT1",
	"TORSO_STAND2_ALT2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",       // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_STAND3_ALT1",
	"TORSO_STAND3_ALT2",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",       // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_STAND4_ALT1",
	"TORSO_STAND4_ALT2",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",       // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_STAND5_ALT1",
	"TORSO_STAND5_ALT2",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1",      // (low)
	"TORSO_RELOAD2",      // (high)
	"TORSO_RELOAD3",      // (pistol)
	"TORSO_RELOAD4",      // (shoulder)

	"TORSO_MG42",         // firing tripod mounted weapon animation

	"TORSO_MOVE",         // torso anim to play while moving and not firing (swinging arms type thing)
	"TORSO_MOVE_ALT",     // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA",
	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",
	"TORSO_EXTRA6",
	"TORSO_EXTRA7",
	"TORSO_EXTRA8",
	"TORSO_EXTRA9",
	"TORSO_EXTRA10",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",
	"LEGS_SWIM_IDLE",

	"LEGS_JUMP",
	"LEGS_JUMPB",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE_ALT",      // "LEGS_IDLE2"
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",          // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
	"LEGS_EXTRA6",
	"LEGS_EXTRA7",
	"LEGS_EXTRA8",
	"LEGS_EXTRA9",
	"LEGS_EXTRA10",
};

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPENDED SPIN PERSISTANT
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
SUSPENDED - will allow items to hang in the air, otherwise they are dropped to the next surface.
SPIN - will allow items to spin in place.
PERSISTANT - some items (ex. clipboards) can be picked up, but don't disappear

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"  override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
"stand" if the item has a stand (ex: mp40_stand.md3) this specifies which stand tag to attach the weapon to ("stand":"4" would mean "tag_stand4" for example)  only weapons support stands currently
*/

// Important notes:
// - whenever you add new items update ITEM_MAX_ITEMS, FIRST_WEAPON_ITEM, ITEM_AMMO_PACK, ITEM_MEGA_AMMO_PACK, ITEM_RED_FLAG, ITEM_BLUE_FLAG
// - bg_itemlist has additional client members
gitem_t bg_itemlist[] =
{
	{
		NULL,                   // classname
		NULL,                   // pickup_sound
		{
			0,                  // world_model[0]
			0,                  // world_model[1]
			0                   // world_model[2]
		},
		NULL,                   // icon
		NULL,                   // ammoicon
		NULL,                   // pickup_name
		0,                      // quantity
		IT_BAD,                 // giType
		0,                      // giTag
	},  // leave index 0 alone
	/*QUAKED item_treasure (1 1 0) (-8 -8 -8) (8 8 8) suspended
	Items the player picks up that are just used to tally a score at end-level
	"model" defaults to 'models/powerups/treasure/goldbar.md3'
	"noise" sound to play on pickup.  defaults to 'sound/pickup/treasure/gold.wav'
	"message" what to call the item when it's picked up.  defaults to "Treasure Item" (SA: temp)
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/treasure/goldbar.md3"
	*/
	/*
	"scriptName"
	*/
	{
		"item_treasure",
		"", // was "sound/pickup/treasure/gold.wav",
		{
			0, // not in path "models/powerups/treasure/goldbar.md3",
			0,
			0
		},
		NULL,   // placeholder
		NULL,                   // ammo icon
		"Treasure Item",     // placeholder
		5,
		IT_TREASURE,
		0,
	},

	// ARMOR/HEALTH/STAMINA

	/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_s.md3"
	*/
	{
		"item_health_small",
		"sound/misc/health_pickup.wav", // was "sound/items/n_health.wav"
		{
			0, // we can't load what's not in - "models/powerups/health/health_s.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Small Health",
		5,
		IT_HEALTH,
		0,
	},
	/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_m.md3"
	*/
	{
		"item_health",
		"sound/misc/health_pickup.wav", //      "sound/multiplayer/health_pickup.wav",
		{
			"models/multiplayer/medpack/medpack_pickup.md3", // was   "models/powerups/health/health_m.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Med Health",
		20,
		IT_HEALTH,
		0,
	},
	/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_m.md3"
	*/
	{
		"item_health_large",
		"sound/misc/health_pickup.wav", //      "sound/multiplayer/health_pickup.wav",
		{
			"models/multiplayer/medpack/medpack_pickup.md3", //  was "models/powerups/health/health_m.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Med Health",
		50,             // increased to 50 from 30 and used it for SP.
		IT_HEALTH,
		0,
	},

	{
		"item_health_cabinet",
		"sound/misc/health_pickup.wav",
		{
			0,
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Health Pack",
		0,
		IT_HEALTH,
		0,
	},
	/*QUAKED item_health_turkey (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	multi-stage health item.
	gives 40 on first use, then gives 20 on "finishing up"

	player will only eat what he needs.  health at 90, turkey fills up and leaves remains (leaving 15).  health at 5 you eat the whole thing.
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_t1.md3"
	*/
	{
		"item_health_turkey",
		"sound/items/hot_pickup.wav",
		{
			"models/powerups/health/health_t3.md3",  // just plate (should now be destructable)
			"models/powerups/health/health_t2.md3",  // half eaten
			"models/powerups/health/health_t1.md3"   // whole turkey
		},
		NULL,
		NULL,   // ammo icon
		"Hot Meal",
		20,                 // amount given in last stage
		IT_HEALTH,
		0,
	},
	/*QUAKED item_health_breadandmeat (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	multi-stage health item.
	gives 30 on first use, then gives 15 on "finishing up"
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_b1.md3"
	*/
	{
		"item_health_breadandmeat",
		"sound/items/cold_pickup.wav",
		{
			0, // we can't load what's not in - "models/powerups/health/health_b3.md3",    // just plate (should now be destructable)
			0, // we can't load what's not in - "models/powerups/health/health_b2.md3",    // half eaten
			0 // we can't load what's not in - "models/powerups/health/health_b1.md3"     // whole turkey
		},
		NULL,
		NULL,   // ammo icon
		"Cold Meal",
		15,                 // amount given in last stage
		IT_HEALTH,
		0,
	},
	/*QUAKED item_health_wall (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	defaults to 50 pts health
	you will probably want to check the 'suspended' box to keep it from falling to the ground
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_w.md3"
	*/
	{
		"item_health_wall",
		"sound/misc/health_pickup.wav", // was "sound/items/n_health.wav"
		{
			0, // we can't load what's not in - "models/powerups/health/health_w.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Health",
		25,
		IT_HEALTH,
		0,
	},

	// STAMINA

	// WEAPONS - wolf weapons

	/*QUAKED weapon_knife (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/knife/knife.md3"
	*/
	{
		"weapon_knife",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/knife/knife.md3",
			"models/multiplayer/knife/v_knife.md3",
			0
		},
		"icons/iconw_knife_1",   // icon
		"icons/ammo2",           // ammo icon
		"Knife",                 // pickup
		50,
		IT_WEAPON,
		WP_KNIFE,
	},

	{
		"weapon_knife_kabar",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/knife_kbar/knife.md3",
			"models/multiplayer/knife_kbar/v_knife.md3",
			0
		},
		"icons/iconw_knife_1",   // icon
		"icons/ammo2",           // ammo icon
		"Ka-Bar",                // pickup
		50,
		IT_WEAPON,
		WP_KNIFE_KABAR,
	},

	/*QUAKED weapon_luger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/luger/luger.md3"
	*/
	{
		"weapon_luger",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/luger/luger.md3",
			"models/weapons2/luger/v_luger.md3",
			0
		},
		"",  // icon
		"icons/ammo2",           // ammo icon
		"Luger",             // pickup
		50,
		IT_WEAPON,
		WP_LUGER,
	},
	/*QUAKED weapon_akimboluger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_luger/luger.md3"
	*/
	{
		"weapon_akimboluger",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/luger/luger.md3",
			"models/weapons2/akimbo_luger/v_akimbo_luger.md3",
			0
		},
		"icons/iconw_colt_1",    // icon                            // FIXME: need new icon
		"icons/ammo2",           // ammo icon
		"Akimbo Luger",          // pickup
		50,
		IT_WEAPON,
		WP_AKIMBO_LUGER,
	},
	/*QUAKED weapon_akimbosilencedluger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_luger/luger.md3"
	*/
	{
		"weapon_akimbosilencedluger",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/luger/luger.md3",
			"models/weapons2/akimbo_luger/v_akimbo_luger.md3",
			0
		},
		"icons/iconw_colt_1",    // icon                            // FIXME: need new icon
		"icons/ammo2",           // ammo icon
		"Silenced Akimbo Luger",         // pickup
		50,
		IT_WEAPON,
		WP_AKIMBO_SILENCEDLUGER,
	},
	/*QUAKED weapon_thompson (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/thompson/thompson.md3"
	*/
	{
		"weapon_thompson",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/thompson/thompson.md3",
			"models/weapons2/thompson/v_thompson.md3",
			0
		},
		"icons/iconw_thompson_1",    // icon
		"icons/ammo2",           // ammo icon
		"Thompson",              // pickup
		30,
		IT_WEAPON,
		WP_THOMPSON,
	},

	{
		"weapon_dummy",
		"",
		{
			0,
			0,
			0
		},
		"",                      // icon
		"",                      // ammo icon
		"BLANK",             // pickup
		0,                      // quantity
		IT_WEAPON,              // item type
		WP_DUMMY_MG42,          // giTag
	},
	/*QUAKED weapon_sten (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/sten/sten.md3"
	*/
	{
		"weapon_sten",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/sten/sten.md3",
			"models/weapons2/sten/v_sten.md3",
			0
		},
		"icons/iconw_sten_1",    // icon
		"icons/ammo2",           // ammo icon
		"Sten gun",              // pickup
		30,
		IT_WEAPON,
		WP_STEN,
	},
	/*QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/colt/colt.md3"
	*/
	{
		"weapon_colt",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/colt/colt.md3",
			"models/weapons2/colt/v_colt.md3",
			0
		},
		"icons/iconw_colt_1",    // icon
		"icons/ammo2",           // ammo icon
		"Colt",                  // pickup
		50,
		IT_WEAPON,
		WP_COLT,
	},
	/*QUAKED weapon_akimbocolt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_colt/colt.md3"
	*/
	{
		"weapon_akimbocolt",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/colt/colt.md3",
			"models/weapons2/akimbo_colt/v_akimbo_colt.md3",
			0
		},
		"icons/iconw_colt_1",    // icon                            // FIXME: need new icon
		"icons/ammo2",           // ammo icon
		"Akimbo Colt",           // pickup
		50,
		IT_WEAPON,
		WP_AKIMBO_COLT,
	},
	/*QUAKED weapon_akimbosilencedcolt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_colt/colt.md3"
	*/
	{
		"weapon_akimbosilencedcolt",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/colt/colt.md3",
			"models/weapons2/akimbo_colt/v_akimbo_colt.md3",
			0
		},
		"icons/iconw_colt_1",    // icon                            // FIXME: need new icon
		"icons/ammo2",           // ammo icon
		"Silenced Akimbo Colt",          // pickup
		50,
		IT_WEAPON,
		WP_AKIMBO_SILENCEDCOLT,
	},
	/*QUAKED weapon_mp40 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	"stand" values:
	    no value:   laying in a default position on it's side (default)
	    2:          upright, barrel pointing up, slightly angled (rack mount)
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models\weapons2\mp40\mp40.md3"
	*/
	{
		"weapon_mp40",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/mp40/mp40.md3",
			"models/weapons2/mp40/v_mp40.md3",
			0
		},
		"icons/iconw_mp40_1",    // icon
		"icons/ammo2",       // ammo icon
		"MP 40",             // pickup
		30,
		IT_WEAPON,
		WP_MP40,
	},
	/*QUAKED weapon_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/panzerfaust/pf.md3"
	*/
	{
		"weapon_panzerfaust",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/panzerfaust/pf.md3",
			"models/weapons2/panzerfaust/v_pf.md3",
			0
		},
		"icons/iconw_panzerfaust_1", // icon
		"icons/ammo6",       // ammo icon
		"Panzerfaust",               // pickup
		1,
		IT_WEAPON,
		WP_PANZERFAUST,
	},
	/*QUAKED weapon_bazooka (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/bazooka/bazooka.md3"
	*/
	{
		"weapon_bazooka",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/bazooka/bazooka_pickup.md3",
			"models/weapons2/bazooka/v_bazooka.md3",
			0
		},
		"icons/iconw_bazooka_1", // icon
		"icons/ammo6",       // ammo icon
		"Bazooka",               // pickup
		1,
		IT_WEAPON,
		WP_BAZOOKA,
	},

	// removed the quaked for this.  we don't actually have a grenade launcher as such.  It's given implicitly
	//         by virtue of getting grenade ammo.  So we don't need to have them in maps

	// weapon_grenadelauncher
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/grenade/grenade.md3",
			"models/weapons2/grenade/v_grenade.md3",
			0
		},
		"icons/iconw_grenade_1", // icon
		"icons/icona_grenade",   // ammo icon
		"Grenade",               // pickup
		6,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
	},
	// weapon_grenadePineapple
	{
		"weapon_grenadepineapple",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/grenade/pineapple.md3",
			"models/weapons2/grenade/v_pineapple.md3",
			0
		},
		"icons/iconw_pineapple_1",   // icon
		"icons/icona_pineapple", // ammo icon
		"Pineapple",             // pickup
		6,
		IT_WEAPON,
		WP_GRENADE_PINEAPPLE,
	},
	// weapon_grenadesmoke
	{
		"weapon_grenadesmoke",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/smokegrenade/smokegrenade.md3",
			"models/multiplayer/smokegrenade/v_smokegrenade.md3",
			0
		},
		"icons/iconw_smokegrenade_1",    // icon
		"icons/ammo2",   // ammo icon
		"smokeGrenade",              // pickup
		50,
		IT_WEAPON,
		WP_SMOKE_MARKER,
	},
	// weapon_smoketrail -- only used as a special effects emitter for smoke trails (artillery spotter etc)
	{
		"weapon_smoketrail",
		"",
		{
			"models/multiplayer/smokegrenade/smokegrenade.md3",
			"models/multiplayer/smokegrenade/v_smokegrenade.md3",
			0
		},
		"icons/iconw_smokegrenade_1",    // icon
		"icons/ammo2",   // ammo icon
		"smokeTrail",                // pickup
		50,
		IT_WEAPON,
		WP_SMOKETRAIL,
	},
	// weapon_medic_heal
	{
		"weapon_medic_heal",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/medpack/medpack.md3",
			"models/multiplayer/medpack/v_medpack.md3",
			0
		},
		"icons/iconw_medheal_1", // icon
		"icons/ammo2",           // ammo icon
		"medicheal",         // pickup
		50,
		IT_WEAPON,
		WP_MEDKIT,
	},
	// weapon_dynamite
	{
		"weapon_dynamite",
		"",
		{
			"models/multiplayer/dynamite/dynamite_3rd.md3",
			"models/weapons2/dynamite/v_dynamite.md3",
			0
		},
		"icons/iconw_dynamite_1",    // icon
		"icons/ammo9",           // ammo icon
		"Dynamite Weapon",       // pickup
		7,
		IT_WEAPON,
		WP_DYNAMITE,
	},
	/*QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/flamethrower/flamethrower.md3"
	*/
	{
		"weapon_flamethrower",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/flamethrower/flamethrower.md3",
			"models/weapons2/flamethrower/v_flamethrower.md3",
			"models/weapons2/flamethrower/pu_flamethrower.md3"
		},
		"icons/iconw_flamethrower_1",    // icon
		"icons/ammo10",              // ammo icon
		"Flamethrower",              // pickup
		200,
		IT_WEAPON,
		WP_FLAMETHROWER,
	},
	// weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_mapmortar",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/grenade/grenade.md3",
			"models/weapons2/grenade/v_grenade.md3",
			0
		},
		"icons/iconw_grenade_1", // icon
		"icons/icona_grenade",   // ammo icon
		"nopickup(WP_MAPMORTAR)",        // pickup
		6,
		IT_WEAPON,
		WP_MAPMORTAR,
	},
	// weapon_class_special (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_class_special",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/pliers/pliers.md3",
			"models/multiplayer/pliers/v_pliers.md3",
			0
		},
		"icons/iconw_pliers_1",  // icon
		"icons/ammo2",           // ammo icon
		"Special",               // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_PLIERS,
	},
	// weapon_arty (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_arty",
		"",
		{
			0,
			0,
			0
		},
		"icons/iconw_syringe_1", // icon
		"icons/ammo2",           // ammo icon
		"Artillery",             // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_ARTY,
	},
	// weapon_medic_syringe (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_medic_syringe",
		"",
		{
			"models/multiplayer/syringe/syringe.md3",
			"models/multiplayer/syringe/v_syringe.md3",
			0
		},
		"icons/iconw_syringe_1", // icon
		"icons/ammo2",           // ammo icon
		"Syringe",               // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_MEDIC_SYRINGE,
	},
	// weapon_medic_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_medic_adrenaline",
		"",
		{
			"models/multiplayer/syringe/syringe.md3",
			"models/multiplayer/syringe/v_syringe.md3",
			0
		},
		"icons/iconw_syringe_1", // icon
		"icons/ammo2",           // ammo icon
		"Adrenaline Syringe",    // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_MEDIC_ADRENALINE,
	},
	// weapon_magicammo (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_magicammo",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/ammopack/ammopack.md3",
			"models/multiplayer/ammopack/v_ammopack.md3",
			"models/multiplayer/ammopack/ammopack_pickup.md3"
		},
		"icons/iconw_ammopack_1", // icon
		"icons/ammo2",           // ammo icon
		"Ammo Pack",             // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_AMMO,
	},
	{
		"weapon_magicammo2",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/binocs/v_binocs.md3",
			"models/multiplayer/binocs/v_binocs.md3",
			"models/multiplayer/binocs/v_binocs.md3"
			//"models/multiplayer/ammopack/ammopack.md3",
			//"models/multiplayer/ammopack/v_ammopack.md3",
			//"models/multiplayer/ammopack/ammopack_pickup_s.md3"
		},
		"icons/iconw_ammopack_1",    // icon
		"icons/ammo2",               // ammo icon
		"Mega Ammo Pack",            // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_AMMO,
	},
	// weapon_binoculars (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_binoculars",
		"sound/misc/w_pkup.wav",
		{
			"",
			"models/multiplayer/binocs/v_binocs.md3",
			0
		},
		"",  // icon
		"",          // ammo icon
		"Binoculars",                // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_BINOCULARS,
	},
	/*QUAKED weapon_k43 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model=""
	*/
	{
		"weapon_kar43",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/kar98/kar98_3rd.md3",
			"models/multiplayer/kar98/v_kar98.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_mauser_1",  // icon
		"icons/ammo3",           // ammo icon
		"Scoped K43 Rifle",      // pickup
		50,
		IT_WEAPON,
		WP_K43,
	},
	/*QUAKED weapon_kar43_scope (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model=""
	*/
	{
		"weapon_kar43_scope",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/kar98/kar98_3rd.md3",
			"models/multiplayer/kar98/v_kar98.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_mauser_1",  // icon
		"icons/ammo3",           // ammo icon
		"K43 Rifle Scope",       // pickup
		50,
		IT_WEAPON,
		WP_K43_SCOPE,
	},
	/*QUAKED weapon_kar98Rifle (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		"weapon_kar98Rifle",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/kar98/kar98_3rd.md3",
			"models/multiplayer/kar98/v_kar98.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_kar98_1",   // icon
		"icons/ammo3",           // ammo icon
		"K43 Rifle",             // pickup
		50,
		IT_WEAPON,
		WP_KAR98,
	},
	/*QUAKED weapon_gpg40 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		"weapon_gpg40",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/kar98/kar98_3rd.md3",
			"models/multiplayer/kar98/v_kar98.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_kar98_1",       // icon
		"icons/ammo10",              // ammo icon
		"GPG40",                     // pickup
		200,
		IT_WEAPON,
		WP_GPG40,
	},
	/*QUAKED weapon_gpg40_allied (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		"weapon_gpg40_allied",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/m1_garand/m1_garand_3rd.md3",
			"models/multiplayer/m1_garand/v_m1_garand.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_m1_garand_1",   // icon
		"icons/ammo10",              // ammo icon
		"M7",                        // pickup
		200,
		IT_WEAPON,
		WP_M7,
	},
	/*QUAKED weapon_M1CarbineRifle (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		"weapon_M1CarbineRifle",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/m1_garand/m1_garand_3rd.md3",
			"models/multiplayer/m1_garand/v_m1_garand.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_m1_garand_1",   // icon
		"icons/ammo3",           // ammo icon
		"M1 Garand",             // pickup
		50,
		IT_WEAPON,
		WP_CARBINE,
	},
	/*
	weapon_garandRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/garand/garand.md3"
	*/
	{
		"weapon_garandRifle",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/m1_garand/m1_garand_3rd.md3",
			"models/multiplayer/m1_garand/v_m1_garand.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_mauser_1",  // icon
		"icons/ammo3",           // ammo icon
		"Scoped M1 Garand",      // pickup
		50,
		IT_WEAPON,
		WP_GARAND,
	},
	/*
	weapon_garandRifleScope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/garand/garand.md3"
	*/
	{
		"weapon_garandRifleScope",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/m1_garand/m1_garand_3rd.md3",
			"models/multiplayer/m1_garand/v_m1_garand.md3",
			"models/multiplayer/mauser/mauser_pickup.md3"
		},
		"icons/iconw_mauser_1",  // icon
		"icons/ammo3",           // ammo icon
		"M1 Garand Scope",       // pickup
		50,
		IT_WEAPON,
		WP_GARAND_SCOPE,
	},
	/*QUAKED weapon_fg42 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/fg42/fg42.md3"
	*/
	{
		"weapon_fg42",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/fg42/fg42.md3",
			"models/weapons2/fg42/v_fg42.md3",
			"models/weapons2/fg42/pu_fg42.md3"
		},
		"icons/iconw_fg42_1",    // icon
		"icons/ammo5",           // ammo icon
		"FG 42 Paratroop Rifle", // pickup
		10,
		IT_WEAPON,
		WP_FG42,
	},
	/*QUAKED weapon_fg42scope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/fg42/fg42.md3"
	*/
	{
		"weapon_fg42scope",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/fg42/fg42.md3",
			"models/weapons2/fg42/v_fg42.md3",
			"models/weapons2/fg42/pu_fg42.md3"
		},
		"icons/iconw_fg42_1",    // icon
		"icons/ammo5",           // ammo icon
		"FG 42 Paratroop Rifle Scope", // pickup
		0,
		IT_WEAPON,
		WP_FG42SCOPE,   // this weap
	},
	/*
	weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/bla?bla?/bla!.md3"
	*/
	{
		"weapon_mortar",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/mortar/mortar_3rd.md3",
			"models/multiplayer/mortar/v_mortar.md3",
			0
		},
		"icons/iconw_mortar_1",  // icon
		"icons/ammo5",           // ammo icon
		"Mortar",                // pickup
		0,
		IT_WEAPON,
		WP_MORTAR,  // this weap
	},
	{
		"weapon_mortar_set",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/mortar/mortar_3rd.md3",
			"models/multiplayer/mortar/v_mortar.md3",
			0
		},
		"icons/iconw_mortar_1",  // icon
		"icons/ammo5",           // ammo icon
		"Mounted Mortar",        // pickup
		0,
		IT_WEAPON,
		WP_MORTAR_SET,  // this weap
	},

	{
		"weapon_mortar2",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/mortar/mortar_3rd.md3",
			"models/multiplayer/mortar/v_mortar.md3",
			0
		},
		"icons/iconw_mortar_1",
		"icons/ammo5",
		"Granatwerfer",
		0,
		IT_WEAPON,
		WP_MORTAR2,
	},
	{
		"weapon_mortar2_set",
		"sound/misc/w_pkup.wav", // FIXME: I've never heard this
		{
			"models/multiplayer/mortar/mortar_3rd.md3",
			"models/multiplayer/mortar/v_mortar.md3",
			0
		},
		"icons/iconw_mortar_1",
		"icons/ammo5",
		"Mounted Granatwerfer",
		0,
		IT_WEAPON,
		WP_MORTAR2_SET,
	},
	// weapon_landmine
	{
		"weapon_landmine",
		"",
		{
			"models/multiplayer/landmine/landmine.md3",
			"models/multiplayer/landmine/v_landmine.md3",
			0
		},
		"icons/iconw_landmine_1",    // icon
		"icons/ammo9",           // ammo icon
		"Landmine",      // pickup
		7,
		IT_WEAPON,
		WP_LANDMINE,
	},
	// weapon_satchel (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		"weapon_satchel",
		"",
		{
			"models/multiplayer/satchel/satchel.md3",
			"models/multiplayer/satchel/v_satchel.md3",
			0
		},
		"icons/iconw_satchel_1", // icon
		"icons/ammo2",           // ammo icon
		"Satchel Charge",        // pickup
		0,
		IT_WEAPON,
		WP_SATCHEL,
	},

	{
		"weapon_satchelDetonator",
		"",
		{
			"models/multiplayer/satchel/radio.md3",
			"models/multiplayer/satchel/v_satchel.md3",
			0
		},
		"icons/iconw_radio_1",       // icon
		"icons/ammo2",               // ammo icon
		"Satchel Charge Detonator",  // pickup
		0,
		IT_WEAPON,
		WP_SATCHEL_DET,
	},

	{
		"weapon_smokebomb",
		"",
		{
			"models/multiplayer/smokebomb/smokebomb.md3",
			"models/multiplayer/smokebomb/v_smokebomb.md3",
			0
		},
		"icons/iconw_dynamite_1",    // icon
		"icons/ammo9",               // ammo icon
		"Smoke Bomb",                // pickup
		0,
		IT_WEAPON,
		WP_SMOKE_BOMB,
	},
	/*QUAKED weapon_mobile_mg42 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended spin - respawn
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/multiplayer/mg42/v_mg42.md3"
	*/
	{
		"weapon_mobile_mg42",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/mg42/mg42_3rd.md3",
			"models/multiplayer/mg42/v_mg42.md3",
			0
		},
		"icons/iconw_mg42_1",    // icon
		"icons/ammo2",           // ammo icon
		"Mobile MG 42",          // pickup
		30,
		IT_WEAPON,
		WP_MOBILE_MG42,
	},

	{
		"weapon_mobile_mg42_set",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/mg42/mg42_3rd.md3",
			"models/multiplayer/mg42/v_mg42.md3",
			0
		},
		"icons/iconw_mg42_1",    // icon
		"icons/ammo2",           // ammo icon
		"Mobile MG 42 Bipod",    // pickup
		30,
		IT_WEAPON,
		WP_MOBILE_MG42_SET,
	},

	{
		"weapon_mobile_browning_set",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/browning/brown30cal_3rd.md3",
			"models/weapons2/browning/v_brown30cal.md3",
			0
		},
		"icons/iconw_browning_1",    // icon
		"icons/ammo2",           // ammo icon
		"Mobile Browning Bipod",     // pickup
		30,
		IT_WEAPON,
		WP_MOBILE_BROWNING_SET,
	},

	{
		"weapon_mobile_browning",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/browning/brown30cal_3rd.md3",
			"models/weapons2/browning/v_brown30cal.md3",
			0
		},
		"icons/iconw_browning_1",    // icon
		"icons/ammo2",           // ammo icon
		"Mobile Browning",           // pickup
		30,
		IT_WEAPON,
		WP_MOBILE_BROWNING,
	},

	{
		"weapon_silencer",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/silencer/silencer.md3",
			"models/weapons2/silencer/v_silencer.md3",
			"models/weapons2/silencer/pu_silencer.md3"
		},
		"icons/iconw_silencer_1",    // icon
		"icons/ammo5",       // ammo icon
		"Silenced Luger",    // pickup
		10,
		IT_WEAPON,
		WP_SILENCER,
	},
	/*QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/colt/colt.md3"
	*/
	{
		"weapon_silencedcolt",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/colt/colt.md3",
			"models/multiplayer/silencedcolt/v_silencedcolt.md3",
			0
		},
		"icons/iconw_colt_1",    // icon
		"icons/ammo2",           // ammo icon
		"Silenced Colt",         // pickup
		50,
		IT_WEAPON,
		WP_SILENCED_COLT,
	},
	// weapon_medic_heal
	{
		"weapon_medic_heal",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/medpack/medpack.md3",
			"models/multiplayer/medpack/v_medpack.md3",
			0
		},
		"icons/iconw_medheal_1", // icon
		"icons/ammo2",           // ammo icon
		"medicheal",             // pickup
		50,
		IT_WEAPON,
		WP_MEDKIT,
	},
	/*QUAKED ammo_syringe (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: medic
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/syringe/syringe.md3
	*/
	{
		"ammo_syringe",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/syringe/syringe.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"syringe",           // pickup
		1,
		IT_AMMO,
		WP_MEDIC_SYRINGE,
	},
	/*QUAKED ammo_smoke_grenade (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: engineer
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/smoke_grenade/smoke_grenade.md3"
	*/
	{
		"ammo_smoke_grenade",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/smoke_grenade/smoke_grenade.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"smoke grenade", // pickup
		1,
		IT_AMMO,
		WP_SMOKE_BOMB,
	},
	/*QUAKED ammo_dynamite (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: engineer
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/dynamite/dynamite.md3"
	*/
	{
		"ammo_dynamite",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/dynamite/dynamite.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"dynamite",  // pickup
		1,
		IT_AMMO,
		WP_DYNAMITE,
	},
	/*QUAKED ammo_disguise (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: covertops
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/disguise/disguise.md3"
	*/
	{
		"ammo_disguise",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/disguise/disguise.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"disguise",  // pickup
		1,
		IT_AMMO,
		-1, // ignored
	},
	/*QUAKED ammo_airstrike (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: LT
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/airstrike/airstrike.md3"
	*/
	{
		"ammo_airstrike",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/disguise/disguise.md3"
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"airstrike canister",    // pickup
		1,
		IT_AMMO,
		WP_SMOKE_MARKER,
	},
	/*QUAKED ammo_landmine (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: LT
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/landmine/landmine.md3"
	*/
	{
		"ammo_landmine",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/landmine/landmine.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"landmine",  // pickup
		1,
		IT_AMMO,
		WP_LANDMINE,
	},
	/*QUAKED ammo_satchel_charge (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: LT
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/satchel/satchel.md3"
	*/
	{
		"ammo_satchel_charge",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/ammo/satchel/satchel.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"satchel charge",    // pickup
		1,
		IT_AMMO,
		WP_SATCHEL,
	},

	// AMMO ITEMS

	/*QUAKED ammo_9mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Luger pistol, MP40 machinegun
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am9mm_s.md3"
	*/
	{
		"ammo_9mm_small",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am9mm_s.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"9mm Rounds",        // pickup
		8,
		IT_AMMO,
		WP_LUGER,
	},
	/*QUAKED ammo_9mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Luger pistol, MP40 machinegun
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am9mm_m.md3"
	*/
	{
		"ammo_9mm",
		"sound/misc/am_pkup.wav",
		{
			0,               // we can't load what's not in - "models/powerups/ammo/am9mm_m.md3",
			0,
			0
		},
		"",                  // icon
		NULL,                // ammo icon
		"9mm",               // pickup
		16,
		IT_AMMO,
		WP_LUGER,
	},
	/*QUAKED ammo_9mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Luger pistol, MP40 machinegun
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am9mm_l.md3"
	*/
	{
		"ammo_9mm_large",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am9mm_l.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		"9mm Box",           // pickup
		24,
		IT_AMMO,
		WP_LUGER,
	},
	/*QUAKED ammo_45cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Thompson, Colt
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am45cal_s.md3"
	*/
	{
		"ammo_45cal_small",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am45cal_s.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		".45cal Rounds", // pickup
		8,
		IT_AMMO,
		WP_COLT,
	},
	/*QUAKED ammo_45cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Thompson, Colt
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am45cal_m.md3"
	*/
	{
		"ammo_45cal",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am45cal_m.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		".45cal",        // pickup
		16,
		IT_AMMO,
		WP_COLT,
	},
	/*QUAKED ammo_45cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Thompson, Colt
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am45cal_l.md3"
	*/
	{
		"ammo_45cal_large",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am45cal_l.md3",
			0,
			0
		},
		"", // icon
		NULL,               // ammo icon
		".45cal Box",        // pickup
		24,
		IT_AMMO,
		WP_COLT,
	},
	/*QUAKED ammo_30cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Garand rifle
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am30cal_s.md3"
	*/
	{
		"ammo_30cal_small",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am30cal_s.md3",
			0,
			0
		},
		"",  // icon
		NULL,                       // ammo icon
		".30cal Rounds",         // pickup
		8,
		IT_AMMO,
		WP_GARAND,
	},
	/*QUAKED ammo_30cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Garand rifle
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am30cal_m.md3"
	*/
	{
		"ammo_30cal",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am30cal_m.md3",
			0,
			0
		},
		"",  // icon
		NULL,                       // ammo icon
		".30cal",                // pickup
		16,
		IT_AMMO,
		WP_GARAND,
	},
	/*QUAKED ammo_30cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Garand rifle
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am30cal_l.md3"
	*/
	{
		"ammo_30cal_large",
		"sound/misc/am_pkup.wav",
		{
			0,                                       // we can't load what's not in - "models/powerups/ammo/am30cal_l.md3",
			0,
			0
		},
		"",  // icon
		NULL,                       // ammo icon
		".30cal Box",                // pickup
		24,
		IT_AMMO,
		WP_GARAND,
	},

	// POWERUP ITEMS

	/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
	Only in CTF games
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/flags/r_flag.md3"
	*/
	{
		"team_CTF_redflag",
		"sound/misc/w_pkup.wav",
		{
			0,
			0,
			0
		},
		"",              // icon
		NULL,            // ammo icon
		"Red Flag",      // pickup
		0,
		IT_TEAM,
		PW_REDFLAG,
	},
	/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
	Only in CTF games
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/flags/b_flag.md3"
	*/
	{
		"team_CTF_blueflag",
		"sound/misc/w_pkup.wav",
		{
			0,
			0,
			0
		},
		"",                 // icon
		NULL,               // ammo icon
		"Blue Flag",        // pickup
		0,
		IT_TEAM,
		PW_BLUEFLAG,
	},

	// end of list marker
	{
		NULL,                   // classname
		NULL,                   // pickup_sound
		{
			0,                  // world_model[0]
			0,                  // world_model[1]
			0                   // world_model[2]
		},
		NULL,                   // icon
		NULL,                   // ammoicon
		NULL,                   // pickup_name
		0,                      // quantity
		IT_BAD,                 // giType
		0,                      // giTag
	}
};

int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;     // keep in sync with ITEM_MAX_ITEMS!

/**
 * @brief BG_FindItemForWeapon
 * @param[in] weapon
 * @return
 */
gitem_t *BG_FindItemForWeapon(weapon_t weapon)
{
	gitem_t *it;

	for (it = bg_itemlist + FIRST_WEAPON_ITEM ; it->classname ; it++)
	{
		if (it->giType == IT_WEAPON && it->giTag == weapon)
		{
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/**
 * @brief BG_FindClipForWeapon
 * @param[in] weapon
 * @return
 */
weapon_t BG_FindClipForWeapon(weapon_t weapon)
{
	// FIXME: check valid weapon?
	return weaponTable[weapon].clipIndex;
}

/**
 * @brief BG_FindAmmoForWeapon
 * @param[in] weapon
 * @return
 */
weapon_t BG_FindAmmoForWeapon(weapon_t weapon)
{
	// FIXME: check valid weapon?
	return weaponTable[weapon].ammoIndex;
}

/**
 * @brief BG_AkimboFireSequence
 * @param[in] weapon
 * @param[in] akimboClip
 * @param[in] mainClip
 * @return 'true' if it's the left hand's turn to fire, 'false' if it's the right hand's turn
 */
qboolean BG_AkimboFireSequence(int weapon, int akimboClip, int mainClip)
{
	if (!IS_AKIMBO_WEAPON(weapon))
	{
		return qfalse;
	}

	if (!akimboClip)
	{
		return qfalse;
	}

	// no ammo in main weapon, must be akimbo turn
	if (!mainClip)
	{
		return qtrue;
	}

	// at this point, both have ammo

	// now check 'cycle'   // (removed old method 11/5/2001)
	if ((akimboClip + mainClip) & 1)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief BG_GetItem
 * @param[in] index
 * @return
 */
gitem_t *BG_GetItem(int index)
{
	return &bg_itemlist[index];
}

/**
 * @brief BG_FindItem
 * @param[in] pickupName
 * @return
 */
gitem_t *BG_FindItem(const char *pickupName)
{
	gitem_t *it;

	for (it = bg_itemlist + 1 ; it->classname ; it++)
	{
		if (!Q_stricmp(it->pickup_name, pickupName))
		{
			return it;
		}
	}

	return NULL;
}

/**
 * @brief BG_FindItemForClassName
 * @param[in] className
 * @return
 */
gitem_t *BG_FindItemForClassName(const char *className)
{
	gitem_t *it;

	for (it = bg_itemlist + 1 ; it->classname ; it++)
	{
		if (!Q_stricmp(it->classname, className))
		{
			return it;
		}
	}

	return NULL;
}
/**
 * @brief Items can be picked up without actually touching their physical bounds to make
 *        grabbing them easier
 * @param[in] ps
 * @param[in] item
 * @param[in] atTime
 * @return
 */
qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime)
{
	vec3_t origin;

	BG_EvaluateTrajectory(&item->pos, atTime, origin, qfalse, item->effect2Time);

	// we are ignoring ducked differences here
	if (ps->origin[0] - origin[0] > 36
	    || ps->origin[0] - origin[0] < -36
	    || ps->origin[1] - origin[1] > 36
	    || ps->origin[1] - origin[1] < -36
	    || ps->origin[2] - origin[2] > 36
	    || ps->origin[2] - origin[2] < -36)
	{
		return qfalse;
	}

	return qtrue;
}

/**
 *  @brief if numOfClips is 0, no ammo is added, it just return whether any ammo CAN be added;
 *         otherwise return whether any ammo was ACTUALLY added.
 *         WARNING: when numOfClips is 0, DO NOT CHANGE ANYTHING under ps.
 * @param[in] cls
 * @param[in] skills
 * @return
 */
int BG_GrenadesForClass(int cls, int *skills)
{
	switch (cls)
	{
	case PC_MEDIC:
		if (skills[SK_FIRST_AID] >= 1)
		{
			return 2;
		}
		return 1;
	case PC_SOLDIER:
		return 4;
	case PC_ENGINEER:
		return 8;
	case PC_FIELDOPS:
		if (skills[SK_SIGNALS] >= 1)
		{
			return 2;
		}
		return 1;
	case PC_COVERTOPS:
		return 2;
	}

	return 0;
}

/**
 * @brief BG_GrenadeTypeForTeam
 * @param[in] team
 * @return
 */
weapon_t BG_GrenadeTypeForTeam(team_t team)
{
	switch (team)
	{
	case TEAM_AXIS:
		return WP_GRENADE_LAUNCHER;
	case TEAM_ALLIES:
		return WP_GRENADE_PINEAPPLE;
	//case TEAM_FREE:
	//case TEAM_SPECTATOR:
	//case TEAM_NUM_TEAMS:
	default:
		return WP_NONE;
	}
}

/**
 * @brief Setting numOfClips = 0 allows you to check if the client needs ammo, but doesnt give any
 * @param[in] ps
 * @param[in] skill
 * @param[in] teamNum
 * @param[in] numOfClips
 * @return
 */
qboolean BG_AddMagicAmmo(playerState_t *ps, int *skill, int teamNum, int numOfClips)
{
	qboolean ammoAdded = qfalse;
	int      maxammo;
	int      weapNumOfClips;
	int      i      = BG_GrenadesForClass(ps->stats[STAT_PLAYER_CLASS], skill); // handle grenades first
	weapon_t weapon = BG_GrenadeTypeForTeam((team_t)teamNum);
	weapon_t clip   = BG_FindClipForWeapon((weapon_t)weapon);

	if (ps->ammoclip[clip] < i)
	{
		// early out
		if (!numOfClips)
		{
			return qtrue;
		}

		ps->ammoclip[clip] += numOfClips;

		ammoAdded = qtrue;

		COM_BitSet(ps->weapons, weapon);

		if (ps->ammoclip[clip] > i)
		{
			ps->ammoclip[clip] = i;
		}
	}

	if (COM_BitCheck(ps->weapons, WP_MEDIC_SYRINGE))
	{
		i = skill[SK_FIRST_AID] >= 2 ? 12 : 10;

		clip = BG_FindClipForWeapon(WP_MEDIC_SYRINGE);

		if (ps->ammoclip[clip] < i)
		{
			if (!numOfClips)
			{
				return qtrue;
			}

			ps->ammoclip[clip] += numOfClips;

			ammoAdded = qtrue;

			if (ps->ammoclip[clip] > i)
			{
				ps->ammoclip[clip] = i;
			}
		}
	}

	// now other weapons
	for (i = 0; reloadableWeapons[i] > 0; i++)
	{
		weapon = reloadableWeapons[i];
		if (COM_BitCheck(ps->weapons, weapon))
		{
			maxammo = BG_MaxAmmoForWeapon(weapon, skill);

			// Handle weapons that just use clip, and not ammo
			if (weapon == WP_FLAMETHROWER)
			{
				clip = BG_FindAmmoForWeapon(weapon);
				if (ps->ammoclip[clip] < maxammo)
				{
					// early out
					if (!numOfClips)
					{
						return qtrue;
					}

					ammoAdded          = qtrue;
					ps->ammoclip[clip] = maxammo;
				}
			}
			else if (weapon == WP_PANZERFAUST || weapon == WP_BAZOOKA)       //%    || weapon == WP_MORTAR ) {
			{
				clip = BG_FindAmmoForWeapon(weapon);
				if (ps->ammoclip[clip] < maxammo)
				{
					// early out
					if (!numOfClips)
					{
						return qtrue;
					}

					ammoAdded           = qtrue;
					ps->ammoclip[clip] += numOfClips;
					if (ps->ammoclip[clip] >= maxammo)
					{
						ps->ammoclip[clip] = maxammo;
					}
				}
			}
			else
			{
				clip = BG_FindAmmoForWeapon(weapon);
				if (ps->ammo[clip] < maxammo)
				{
					// early out
					if (!numOfClips)
					{
						return qtrue;
					}
					ammoAdded = qtrue;

					if (IS_AKIMBO_WEAPON(weapon))
					{
						weapNumOfClips = numOfClips * 2;     // double clips babeh!
					}
					else
					{
						weapNumOfClips = numOfClips;
					}

					// add and limit check
					ps->ammo[clip] += weapNumOfClips * GetAmmoTableData(weapon)->maxclip;
					if (ps->ammo[clip] > maxammo)
					{
						ps->ammo[clip] = maxammo;
					}
				}
			}
		}
	}
	return ammoAdded;
}

/**
 * @brief This needs to be the same for client side prediction and server use.
 * @param[in] ent
 * @param[in] ps
 * @param[in] skill
 * @param[in] teamNum
 * @return false if the item should not be picked up.
 */
qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, int *skill, int teamNum)
{
	gitem_t *item;

	if (ent->modelindex < 1 || ent->modelindex >= bg_numItems)
	{
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

	item = &bg_itemlist[ent->modelindex];

	switch (item->giType)
	{
	case IT_WEAPON:
		if (item->giTag == WP_AMMO)
		{
			// magic ammo for any two-handed weapon
			// - only pick up if ammo is not full, numClips is 0, so ps will
			// NOT be changed (I know, it places the burden on the programmer, rather than the
			// compiler, to ensure that).
			return BG_AddMagicAmmo((playerState_t *)ps, skill, teamNum, 0);      // had to cast const away
		}

		return qtrue;
	case IT_AMMO:
		return qfalse;
	case IT_ARMOR:
		return qfalse;
	case IT_HEALTH:
		// ps->teamNum is really class.... thx whoever decided on that...
		if (ps->teamNum == PC_MEDIC)
		{
			// medics can go up to 12% extra on max health as they have perm. regen
			if (ps->stats[STAT_HEALTH] >= (int)(ps->stats[STAT_MAX_HEALTH] * 1.12))
			{
				return qfalse;
			}
		}
		else
		{
			if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
			{
				return qfalse;
			}
		}
		return qtrue;
	case IT_TEAM:     // team items, such as flags
		// density tracks how many uses left
		if ((ent->density < 1) || (((ps->persistant[PERS_TEAM] == TEAM_AXIS) ? ps->powerups[PW_BLUEFLAG] : ps->powerups[PW_REDFLAG]) != 0))
		{
			return qfalse;
		}

		// otherEntity2 is now used instead of modelindex2
		// ent->modelindex2 is non-zero on items if they are dropped
		// we need to know this because we can pick up our dropped flag (and return it)
		// but we can't pick up our flag at base
		if (ps->persistant[PERS_TEAM] == TEAM_AXIS)
		{
			if (item->giTag == PW_BLUEFLAG ||
			    (item->giTag == PW_REDFLAG && ent->otherEntityNum2 /*ent->modelindex2*/) ||
			    (item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]))
			{
				return qtrue;
			}
		}
		else if (ps->persistant[PERS_TEAM] == TEAM_ALLIES)
		{
			if (item->giTag == PW_REDFLAG ||
			    (item->giTag == PW_BLUEFLAG && ent->otherEntityNum2 /*ent->modelindex2*/) ||
			    (item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]))
			{
				return qtrue;
			}
		}

		return qfalse;

	case IT_HOLDABLE:
		return qtrue;
	case IT_TREASURE:     // treasure always picked up
		return qtrue;
	case IT_KEY:
		return qtrue;     // keys are always picked up
	case IT_BAD:
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
	}
	return qfalse;
}

//======================================================================

/**
 * @brief BG_CalculateSpline_r
 * @param[in] spline
 * @param[out] out1
 * @param[out] out2
 * @param[in] tension
 */
void BG_CalculateSpline_r(splinePath_t *spline, vec3_t out1, vec3_t out2, float tension)
{
	vec3_t points[18];
	int    i;
	int    count = spline->numControls + 2;
	vec3_t dist;

	VectorCopy(spline->point.origin, points[0]);
	for (i = 0; i < spline->numControls; i++)
	{
		VectorCopy(spline->controls[i].origin, points[i + 1]);
	}
	if (!spline->next)
	{
		return;
		//Com_Error( ERR_DROP, "Spline (%s) with no target referenced", spline->point.name );
	}
	VectorCopy(spline->next->point.origin, points[i + 1]);

	while (count > 2)
	{
		for (i = 0; i < count - 1; i++)
		{
			VectorSubtract(points[i + 1], points[i], dist);
			VectorMA(points[i], tension, dist, points[i]);
		}
		count--;
	}

	VectorCopy(points[0], out1);
	VectorCopy(points[1], out2);
}

/**
 * @brief BG_TraverseSpline
 * @param[in] deltaTime
 * @param[in,out] pSpline
 * @return
 */
qboolean BG_TraverseSpline(float *deltaTime, splinePath_t **pSpline)
{
	float dist;

	while ((*deltaTime) > 1)
	{
		(*deltaTime) -= 1;
		dist          = (*pSpline)->length * (*deltaTime);

		if (!(*pSpline)->next || (*pSpline)->next->length == 0.f)
		{
			return qfalse;
			//Com_Error( ERR_DROP, "Spline path end passed (%s)", (*pSpline)->point.name );
		}

		(*pSpline) = (*pSpline)->next;
		*deltaTime = dist / (*pSpline)->length;
	}

	while ((*deltaTime) < 0)
	{
		dist = -((*pSpline)->length * (*deltaTime));

		if (!(*pSpline)->prev || (*pSpline)->prev->length == 0.f)
		{
			return qfalse;
			//Com_Error( ERR_DROP, "Spline path end passed (%s)", (*pSpline)->point.name );
		}

		(*pSpline)   = (*pSpline)->prev;
		(*deltaTime) = 1 - (dist / (*pSpline)->length);
	}

	return qtrue;
}

/**
 * @brief BG_RaySphereIntersection
 * @param[in] radius
 * @param[in] origin
 * @param[in] path
 * @param[out] t0
 * @param[out] t1
 * @return
 */
qboolean BG_RaySphereIntersection(float radius, vec3_t origin, splineSegment_t *path, float *t0, float *t1)
{
	vec3_t v;
	float  b, c, d;

	VectorSubtract(path->start, origin, v);

	b = 2 * DotProduct(v, path->v_norm);
	c = DotProduct(v, v) - (radius * radius);

	d = (b * b) - (4 * c);
	if (d < 0)
	{
		return qfalse;
	}
	d = sqrt(d);

	*t0 = (-b + d) * 0.5f;
	*t1 = (-b - d) * 0.5f;

	return qtrue;
}

/**
 * @brief BG_LinearPathOrigin2
 * @param[in] radius
 * @param[in,out] pSpline
 * @param[in] deltaTime
 * @param[out] result
 * @param[out] backwards - unused
 */
void BG_LinearPathOrigin2(float radius, splinePath_t **pSpline, float *deltaTime, vec3_t result, qboolean backwards)
{
	qboolean first = qtrue;
	float    t     = 0.f;
	int      i     = floor((*deltaTime) * (MAX_SPLINE_SEGMENTS));
	float    frac;

	if (i >= MAX_SPLINE_SEGMENTS)
	{
		i    = MAX_SPLINE_SEGMENTS - 1;
		frac = 1.f;
	}
	else
	{
		frac = (((*deltaTime) * (MAX_SPLINE_SEGMENTS)) - i);
	}

	while (qtrue)
	{
		float t0, t1;

		while (qtrue)
		{
			if (BG_RaySphereIntersection(radius, result, &(*pSpline)->segments[i], &t0, &t1))
			{
				qboolean found = qfalse;

				t0 /= (*pSpline)->segments[i].length;
				t1 /= (*pSpline)->segments[i].length;

				if (first)
				{
					if (radius < 0)
					{
						if (t0 < frac && (t0 >= 0.f && t0 <= 1.f))
						{
							t     = t0;
							found = qtrue;
						}
						else if (t1 < frac)
						{
							t     = t1;
							found = qtrue;
						}
					}
					else
					{
						if (t0 > frac && (t0 >= 0.f && t0 <= 1.f))
						{
							t     = t0;
							found = qtrue;
						}
						else if (t1 > frac)
						{
							t     = t1;
							found = qtrue;
						}
					}
				}
				else
				{
					if (radius < 0)
					{
						if (t0 < t1 && (t0 >= 0.f && t0 <= 1.f))
						{
							t     = t0;
							found = qtrue;
						}
						else
						{
							t     = t1;
							found = qtrue;
						}
					}
					else
					{
						if (t0 > t1 && (t0 >= 0.f && t0 <= 1.f))
						{
							t     = t0;
							found = qtrue;
						}
						else
						{
							t     = t1;
							found = qtrue;
						}
					}
				}

				if (found)
				{
					if (t >= 0.f && t <= 1.f)
					{
						*deltaTime = (i / (float)(MAX_SPLINE_SEGMENTS)) + (t / (float)(MAX_SPLINE_SEGMENTS));
						VectorMA((*pSpline)->segments[i].start, t * (*pSpline)->segments[i].length, (*pSpline)->segments[i].v_norm, result);
						return;
					}
				}
				found = qfalse;
			}

			first = qfalse;
			if (radius < 0)
			{
				i--;
				if (i < 0)
				{
					i = MAX_SPLINE_SEGMENTS - 1;
					break;
				}
			}
			else
			{
				i++;
				if (i >= MAX_SPLINE_SEGMENTS)
				{
					i = 0;
					break;
				}
			}
		}

		if (radius < 0)
		{
			if (!(*pSpline)->prev)
			{
				return;
				//Com_Error( ERR_DROP, "End of spline reached (%s)", start->point.name );
			}
			*pSpline = (*pSpline)->prev;
		}
		else
		{
			if (!(*pSpline)->next)
			{
				return;
				//Com_Error( ERR_DROP, "End of spline reached (%s)", start->point.name );
			}
			*pSpline = (*pSpline)->next;
		}
	}
}

void BG_ComputeSegments(splinePath_t *pSpline)
{
	int    i;
	float  granularity = 1 / ((float)(MAX_SPLINE_SEGMENTS));
	vec3_t vec[4];

	for (i = 0; i < MAX_SPLINE_SEGMENTS; i++)
	{
		BG_CalculateSpline_r(pSpline, vec[0], vec[1], i * granularity);
		VectorSubtract(vec[1], vec[0], pSpline->segments[i].start);
		VectorMA(vec[0], i * granularity, pSpline->segments[i].start, pSpline->segments[i].start);

		BG_CalculateSpline_r(pSpline, vec[2], vec[3], (i + 1) * granularity);
		VectorSubtract(vec[3], vec[2], vec[0]);
		VectorMA(vec[2], (i + 1) * granularity, vec[0], vec[0]);

		VectorSubtract(vec[0], pSpline->segments[i].start, pSpline->segments[i].v_norm);
		pSpline->segments[i].length = VectorLength(pSpline->segments[i].v_norm);
		VectorNormalize(pSpline->segments[i].v_norm);
	}
}

/**
 * @brief BG_EvaluateTrajectory
 * @param[in] tr
 * @param[in] atTime
 * @param[out] result
 * @param[in] isAngle
 * @param[in] splinePath
 */
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splinePath)
{
	float        deltaTime;
	float        phase;
	vec3_t       v;
	splinePath_t *pSpline;
	vec3_t       vec[2];
	qboolean     backwards = qfalse;
	float        deltaTime2;

	switch (tr->trType)
	{
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED:
		VectorCopy(tr->trBase, result);
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
		phase     = sin(deltaTime * M_PI * 2);
		VectorMA(tr->trBase, phase, tr->trDelta, result);
		break;
	case TR_LINEAR_STOP:
		if (atTime > tr->trTime + tr->trDuration)
		{
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		if (deltaTime < 0)
		{
			deltaTime = 0;
		}
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		result[2] -= 0.5f * DEFAULT_GRAVITY * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
	case TR_GRAVITY_LOW:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		result[2] -= 0.5f * (DEFAULT_GRAVITY * 0.3f) * deltaTime * deltaTime;       // FIXME: local gravity...
		break;
	case TR_GRAVITY_FLOAT:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		result[2] -= 0.5f * (DEFAULT_GRAVITY * 0.2f) * deltaTime;
		break;
	// RF, acceleration
	case TR_ACCELERATE:     // trDelta is the ultimate speed
		if (atTime > tr->trTime + tr->trDuration)
		{
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		// phase is the acceleration constant
		phase = VectorLength(tr->trDelta) / (tr->trDuration * 0.001f);
		// trDelta at least gives us the acceleration direction
		VectorNormalize2(tr->trDelta, result);
		// get distance travelled at current time
		VectorMA(tr->trBase, phase * 0.5f * deltaTime * deltaTime, result, result);
		break;
	case TR_DECCELERATE:     // trDelta is the starting speed
		if (atTime > tr->trTime + tr->trDuration)
		{
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		// phase is the breaking constant
		phase = VectorLength(tr->trDelta) / (tr->trDuration * 0.001f);
		// trDelta at least gives us the acceleration direction
		VectorNormalize2(tr->trDelta, result);
		// get distance travelled at current time (without breaking)
		VectorMA(tr->trBase, deltaTime, tr->trDelta, v);
		// subtract breaking force
		VectorMA(v, -phase * 0.5f * deltaTime * deltaTime, result, result);
		break;
	case TR_SPLINE:
		if (!(pSpline = BG_GetSplineData(splinePath, &backwards)))
		{
			return;
		}

		deltaTime = tr->trDuration ? (atTime - tr->trTime) / ((float)tr->trDuration) : 0;

		if (deltaTime < 0.f)
		{
			deltaTime = 0.f;
		}
		else if (deltaTime > 1.f)
		{
			deltaTime = 1.f;
		}

		if (backwards)
		{
			deltaTime = 1 - deltaTime;
		}

		/*      if(pSpline->isStart) {
		            deltaTime = 1 - sin((1 - deltaTime) * M_PI * 0.5f);
		        } else if(pSpline->isEnd) {
		            deltaTime = sin(deltaTime * M_PI * 0.5f);
		        }*/

		deltaTime2 = deltaTime;

		BG_CalculateSpline_r(pSpline, vec[0], vec[1], deltaTime);

		if (isAngle)
		{
			qboolean dampin  = qfalse;
			qboolean dampout = qfalse;
			float    base1;

			if (tr->trBase[0] != 0.f)
			{
				vec3_t       result2;
				splinePath_t *pSp2 = pSpline;

				deltaTime2 += tr->trBase[0] / pSpline->length;

				if (BG_TraverseSpline(&deltaTime2, &pSp2))
				{
					VectorSubtract(vec[1], vec[0], result);
					VectorMA(vec[0], deltaTime, result, result);

					BG_CalculateSpline_r(pSp2, vec[0], vec[1], deltaTime2);

					VectorSubtract(vec[1], vec[0], result2);
					VectorMA(vec[0], deltaTime2, result2, result2);

					if (tr->trBase[0] < 0)
					{
						VectorSubtract(result, result2, result);
					}
					else
					{
						VectorSubtract(result2, result, result);
					}
				}
				else
				{
					VectorSubtract(vec[1], vec[0], result);
				}
			}
			else
			{
				VectorSubtract(vec[1], vec[0], result);
			}

			vectoangles(result, result);

			base1 = tr->trBase[1];
			if (base1 >= 10000 || base1 < -10000)
			{
				dampin = qtrue;
				if (base1 < 0)
				{
					base1 += 10000;
				}
				else
				{
					base1 -= 10000;
				}
			}

			if (base1 >= 1000 || base1 < -1000)
			{
				dampout = qtrue;
				if (base1 < 0)
				{
					base1 += 1000;
				}
				else
				{
					base1 -= 1000;
				}
			}

			if (dampin && dampout)
			{
				result[ROLL] = base1 + ((sin(((deltaTime * 2) - 1) * M_PI * 0.5f) + 1) * 0.5 * tr->trBase[2]);
			}
			else if (dampin)
			{
				result[ROLL] = base1 + (sin(deltaTime * M_PI * 0.5) * tr->trBase[2]);
			}
			else if (dampout)
			{
				result[ROLL] = base1 + ((1 - sin((1 - deltaTime) * M_PI * 0.5)) * tr->trBase[2]);
			}
			else
			{
				result[ROLL] = base1 + (deltaTime * tr->trBase[2]);
			}
		}
		else
		{
			VectorSubtract(vec[1], vec[0], result);
			VectorMA(vec[0], deltaTime, result, result);
		}

		break;
	case TR_LINEAR_PATH:
		if (!(pSpline = BG_GetSplineData(splinePath, &backwards)))
		{
			return;
		}

		deltaTime = tr->trDuration ? (atTime - tr->trTime) / ((float)tr->trDuration) : 0;

		if (deltaTime < 0.f)
		{
			deltaTime = 0.f;
		}
		else if (deltaTime > 1.f)
		{
			deltaTime = 1.f;
		}

		if (backwards)
		{
			deltaTime = 1 - deltaTime;
		}

		if (isAngle)
		{
			int   pos = floor(deltaTime * (MAX_SPLINE_SEGMENTS));
			float frac;

			if (pos >= MAX_SPLINE_SEGMENTS)
			{
				pos  = MAX_SPLINE_SEGMENTS - 1;
				frac = pSpline->segments[pos].length;
			}
			else
			{
				frac = ((deltaTime * (MAX_SPLINE_SEGMENTS)) - pos) * pSpline->segments[pos].length;
			}

			if (tr->trBase[0] != 0.f)
			{
				VectorMA(pSpline->segments[pos].start, frac, pSpline->segments[pos].v_norm, result);
				VectorCopy(result, v);

				BG_LinearPathOrigin2(tr->trBase[0], &pSpline, &deltaTime, v, backwards);
				if (tr->trBase[0] < 0)
				{
					VectorSubtract(v, result, result);
				}
				else
				{
					VectorSubtract(result, v, result);
				}

				vectoangles(result, result);
			}
			else
			{
				vectoangles(pSpline->segments[pos].v_norm, result);
			}
		}
		else
		{
			int   pos = floor(deltaTime * (MAX_SPLINE_SEGMENTS));
			float frac;

			if (pos >= MAX_SPLINE_SEGMENTS)
			{
				pos  = MAX_SPLINE_SEGMENTS - 1;
				frac = pSpline->segments[pos].length;
			}
			else
			{
				frac = ((deltaTime * (MAX_SPLINE_SEGMENTS)) - pos) * pSpline->segments[pos].length;
			}

			VectorMA(pSpline->segments[pos].start, frac, pSpline->segments[pos].v_norm, result);
		}

		break;
	// TODO : case not handle
	//case TR_LINEAR_STOP_BACK:
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime);
	}
}

/**
 * @brief For determining velocity at a given time
 * @param[in] tr
 * @param[in] atTime
 * @param[out] result
 * @param[in] isAngle
 * @param[in] splineData
 */
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splineData)
{
	float deltaTime;
	float phase;

	switch (tr->trType)
	{
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear(result);
		break;
	case TR_LINEAR:
		VectorCopy(tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
		phase     = cos(deltaTime * M_PI * 2);     // derivative of sin = cos
		phase    *= 0.5;
		VectorScale(tr->trDelta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if (atTime > tr->trTime + tr->trDuration)
		{
			VectorClear(result);
			return;
		}
		VectorCopy(tr->trDelta, result);
		break;
	case TR_GRAVITY:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorCopy(tr->trDelta, result);
		result[2] -= DEFAULT_GRAVITY * deltaTime;       // FIXME: local gravity...
		break;
	case TR_GRAVITY_LOW:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorCopy(tr->trDelta, result);
		result[2] -= (DEFAULT_GRAVITY * 0.3f) * deltaTime;         // FIXME: local gravity...
		break;
	case TR_GRAVITY_FLOAT:
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorCopy(tr->trDelta, result);
		result[2] -= (DEFAULT_GRAVITY * 0.2f) * deltaTime;
		break;
	// acceleration
	case TR_ACCELERATE:     // trDelta is eventual speed
		if (atTime > tr->trTime + tr->trDuration)
		{
			VectorClear(result);
			return;
		}
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		phase     = deltaTime / (float)tr->trDuration;
		VectorScale(tr->trDelta, deltaTime * deltaTime, result);
		break;
	case TR_DECCELERATE:     // trDelta is breaking force
		if (atTime > tr->trTime + tr->trDuration)
		{
			VectorClear(result);
			return;
		}
		deltaTime = (atTime - tr->trTime) * 0.001f;      // milliseconds to seconds
		VectorScale(tr->trDelta, deltaTime, result);
		break;
	case TR_SPLINE:
	case TR_LINEAR_PATH:
		VectorClear(result);
		break;
	// TODO : case not handle
	//case TR_LINEAR_STOP_BACK:
	//case TR_GRAVITY_PAUSED:
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime);
	}
}

/**
 * @brief Used to find a good directional vector for a mark projection, which will be more likely
 * to wrap around adjacent surfaces
 * @param[in] dir The direction of the projectile or trace that has resulted in a surface being hit
 * @param[in] normal
 * @param[out] out
 */
void BG_GetMarkDir(const vec3_t dir, const vec3_t normal, vec3_t out)
{
	vec3_t ndir, lnormal;
	float  minDot = 0.3f;
	int    x      = 0;

	if (dir[0] < 0.001f && dir[1] < 0.001f)
	{
		VectorCopy(dir, out);
		return;
	}

	if (VectorLengthSquared(normal) < Square(1.f))            // this is needed to get rid of (0,0,0) normals (happens with entities?)
	{
		VectorSet(lnormal, 0.f, 0.f, 1.f);
	}
	else
	{
		//VectorCopy( normal, lnormal );
		//VectorNormalizeFast( lnormal );
		VectorNormalize2(normal, lnormal);
	}

	VectorNegate(dir, ndir);
	VectorNormalize(ndir);
	if (normal[2] > .8f)
	{
		minDot = .7f;
	}

	// make sure it makrs the impact surface
	while (DotProduct(ndir, lnormal) < minDot && x < 10)
	{
		VectorMA(ndir, .5f, lnormal, ndir);
		VectorNormalize(ndir);

		x++;
	}

#ifdef GAMEDLL
	if (x >= 10)
	{
		if (g_developer.integer)
		{
			Com_Printf("BG_GetMarkDir loops: %i\n", x);
		}
	}
#endif // GAMEDLL

	VectorCopy(ndir, out);
}

// see bg_public.h
const char *eventnames[EV_MAX_EVENTS] =
{
	"EV_NONE",
	"EV_FOOTSTEP",
	"unused event",              // EV_FOOTSTEP_METAL,
	"unused event",              // EV_FOOTSTEP_WOOD,
	"unused event",              // EV_FOOTSTEP_GRASS,
	"unused event",              // EV_FOOTSTEP_GRAVEL,
	"unused event",              // EV_FOOTSTEP_ROOF,
	"unused event",              // EV_FOOTSTEP_SNOW,
	"unused event",              // EV_FOOTSTEP_CARPET",
	"EV_FOOTSPLASH",
	"unused event",              // EV_FOOTWADE,
	"EV_SWIM",
	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",
	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_FALL_NDIE",
	"EV_FALL_DMG_10",
	"EV_FALL_DMG_15",
	"EV_FALL_DMG_25",
	"EV_FALL_DMG_50",
	"EV_WATER_TOUCH",
	"EV_WATER_LEAVE",
	"EV_WATER_UNDER",
	"EV_WATER_CLEAR",
	"EV_ITEM_PICKUP",
	"EV_ITEM_PICKUP_QUIET",
	"EV_GLOBAL_ITEM_PICKUP",
	"EV_NOAMMO",
	"EV_WEAPONSWITCHED",
	"unused event",              // EV_EMPTYCLIP,
	"EV_FILL_CLIP",
	"EV_MG42_FIXED",
	"EV_WEAP_OVERHEAT",
	"EV_CHANGE_WEAPON",
	"EV_CHANGE_WEAPON_2",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPONB",
	"EV_FIRE_WEAPON_LASTSHOT",
	"EV_NOFIRE_UNDERWATER",
	"EV_FIRE_WEAPON_MG42",
	"EV_FIRE_WEAPON_MOUNTEDMG42",
	"unused event",              // EV_ITEM_RESPAWN,
	"unused event",              // EV_ITEM_POP,
	"unused event",              // EV_PLAYER_TELEPORT_IN,
	"unused event",              // EV_PLAYER_TELEPORT_OUT,
	"EV_GRENADE_BOUNCE",
	"EV_GENERAL_SOUND",
	"EV_GENERAL_SOUND_VOLUME",
	"EV_GLOBAL_SOUND",
	"EV_GLOBAL_CLIENT_SOUND",
	"EV_GLOBAL_TEAM_SOUND",
	"EV_FX_SOUND",
	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",
	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_RAILTRAIL",
	"EV_BULLET",
	"EV_LOSE_HAT",
	"EV_PAIN",
	"unused event",              // EV_CROUCH_PAIN,
	"unused event",              // EV_DEATH1,
	"unused event",              // EV_DEATH2,
	"unused event",              // EV_DEATH3,
	"EV_OBITUARY",
	"EV_STOPSTREAMINGSOUND",
	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",
	"EV_GIB_PLAYER",
	"unused event",              // EV_DEBUG_LINE,
	"EV_STOPLOOPINGSOUND",
	"unused event",              // EV_TAUNT,
	"EV_SMOKE",
	"EV_SPARKS",
	"EV_SPARKS_ELECTRIC",
	"EV_EXPLODE",
	"EV_RUBBLE",
	"EV_EFFECT",
	"EV_MORTAREFX",
	"EV_SPINUP",
	"unused event",              // EV_SNOW_ON,
	"unused event",              // EV_SNOW_OFF,
	"EV_MISSILE_MISS_SMALL",
	"EV_MISSILE_MISS_LARGE",
	"EV_MORTAR_IMPACT",
	"EV_MORTAR_MISS",
	"unused event",              // EV_SPIT_HIT,
	"unused event",              // EV_SPIT_MISS,
	"EV_SHARD",
	"EV_JUNK",
	"EV_EMITTER",
	"EV_OILPARTICLES",
	"EV_OILSLICK",
	"EV_OILSLICKREMOVE",
	"unused event",              // EV_MG42EFX,
	"unused event",              // EV_FLAKGUN1,
	"unused event",              // EV_FLAKGUN2,
	"unused event",              // EV_FLAKGUN3,
	"unused event",              // EV_FLAKGUN4,
	"unused event",              // EV_EXERT1,
	"unused event",              // EV_EXERT2,
	"unused event",              // EV_EXERT3,
	"EV_SNOWFLURRY",
	"unused event",              // EV_CONCUSSIVE,
	"EV_DUST",
	"EV_RUMBLE_EFX",
	"EV_GUNSPARKS",
	"EV_FLAMETHROWER_EFFECT",
	"unused event",              // EV_POPUP,
	"unused event",              // EV_POPUPBOOK,
	"unused event",              // EV_GIVEPAGE,
	"EV_MG42BULLET_HIT_FLESH",
	"EV_MG42BULLET_HIT_WALL",
	"EV_SHAKE",
	"EV_DISGUISE_SOUND",
	"EV_BUILDDECAYED_SOUND",
	"EV_FIRE_WEAPON_AAGUN",
	"EV_DEBRIS",
	"EV_ALERT_SPEAKER",
	"EV_POPUPMESSAGE",
	"EV_ARTYMESSAGE",
	"EV_AIRSTRIKEMESSAGE",
	"EV_MEDIC_CALL",
	"EV_SHOVE_SOUND",
	"EV_BODY_DP",
	//"EV_MAX_EVENTS",
};

#ifdef LEGACY_DEBUG
void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
#endif

/**
 * @brief Handles the sequence numbers
 * @param[in] newEvent
 * @param[in] eventParm
 * @param[out] ps
 */
void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps)
{
#ifdef LEGACY_DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if (atof(buf) != 0)
		{
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, newEvent < EV_MAX_EVENTS ? eventnames[newEvent] : "*unknown event*", eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, newEvent < EV_MAX_EVENTS ? eventnames[newEvent] : "*unknown event*", eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_EVENTS - 1)]     = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_EVENTS - 1)] = eventParm;
	ps->eventSequence++;
}

// NOTE: would like to just inline this but would likely break qvm support
#define SETUP_MOUNTEDGUN_STATUS(ps)                           \
	switch (ps->persistant[PERS_HWEAPON_USE]) {                \
	case 1:                                                 \
		ps->eFlags                    |= EF_MG42_ACTIVE;                       \
		ps->eFlags                    &= ~EF_AAGUN_ACTIVE;                     \
		ps->powerups[PW_OPS_DISGUISED] = 0;                 \
		break;                                              \
	case 2:                                                 \
		ps->eFlags                    |= EF_AAGUN_ACTIVE;                      \
		ps->eFlags                    &= ~EF_MG42_ACTIVE;                      \
		ps->powerups[PW_OPS_DISGUISED] = 0;                 \
		break;                                              \
	default:                                                \
		ps->eFlags &= ~EF_MG42_ACTIVE;                      \
		ps->eFlags &= ~EF_AAGUN_ACTIVE;                     \
		break;                                              \
	}

/**
 * @brief This is done after each set of usercmd_t on the server,
 *        and after local prediction on the client
 * @param[in] ps
 * @param[out] s
 * @param[in] time
 * @param[in] snap
 */
void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, int time, qboolean snap)
{
	int i;

	if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_NOCLIP || ps->stats[STAT_HEALTH] <= GIB_HEALTH)     // || ps->pm_flags & PMF_LIMBO ) { // limbo
	{
		s->eType = ET_INVISIBLE;
	}
	else
	{
		s->eType = ET_PLAYER;
	}

	s->number     = ps->clientNum;
	s->pos.trType = TR_INTERPOLATE;
	s->pos.trTime = time;               // help out new synced animations.

	VectorCopy(ps->origin, s->pos.trBase);
	if (snap)
	{
		SnapVector(s->pos.trBase);
	}

	VectorCopy(ps->velocity, s->pos.trDelta);

	if (snap)
	{
		SnapVector(s->pos.trDelta);
	}

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if (snap)
	{
		SnapVector(s->apos.trBase);
	}

	if (ps->movementDir > 128)
	{
		s->angles2[YAW] = (float)ps->movementDir - 256;
	}
	else
	{
		s->angles2[YAW] = ps->movementDir;
	}

	s->angles2[PITCH] = 0;

	s->legsAnim  = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;       // ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	// - let clients know if this person is using a mounted weapon
	// so they don't show any client muzzle flashes

	if (ps->eFlags & EF_MOUNTEDTANK)
	{
		ps->eFlags &= ~EF_MG42_ACTIVE;
		ps->eFlags &= ~EF_AAGUN_ACTIVE;
	}
	else
	{
		SETUP_MOUNTEDGUN_STATUS(ps);
	}

	s->eFlags = ps->eFlags;

	if (ps->stats[STAT_HEALTH] <= 0)
	{
		s->eFlags |= EF_DEAD;
	}
	else
	{
		s->eFlags &= ~EF_DEAD;
	}

	// from MP
	if (ps->externalEvent)
	{
		s->event     = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if (ps->entityEventSequence < ps->eventSequence)
	{
		int seq;

		if (ps->entityEventSequence < ps->eventSequence - MAX_EVENTS)
		{
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq          = ps->entityEventSequence & (MAX_EVENTS - 1);
		s->event     = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	// now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	for (i = ps->oldEventSequence; i != ps->eventSequence; i++)
	{
		s->events[s->eventSequence & (MAX_EVENTS - 1)]     = ps->events[i & (MAX_EVENTS - 1)];
		s->eventParms[s->eventSequence & (MAX_EVENTS - 1)] = ps->eventParms[i & (MAX_EVENTS - 1)];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon          = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for (i = 0 ; i < MAX_POWERUPS ; i++)
	{
		if (ps->powerups[i])
		{
			s->powerups |= 1 << i;
		}
	}

	s->nextWeapon = ps->nextWeapon;
	s->teamNum    = ps->teamNum;
	s->aiState    = ps->aiState;

	if (ps->pm_type != PM_SPECTATOR)
	{
		// abusing entity state constantLight for STAT_PS_FLAGS flags
		s->constantLight = ps->stats[STAT_PS_FLAGS];
	}
}

/**
 * @brief BG_WeaponForMOD
 * @param[in] MOD
 * @return
 */
weapon_t BG_WeaponForMOD(int MOD)
{
	weapon_t i;

	for (i = WP_KNIFE; i < WP_NUM_WEAPONS; i++)
	{
		if (GetAmmoTableData(i)->mod == MOD)
		{
			return i;
		}
	}

	return WP_NONE;
}

const char *rankSoundNames_Allies[NUM_EXPERIENCE_LEVELS] =
{
	"",
	"allies_hq_promo_private",
	"allies_hq_promo_corporal",
	"allies_hq_promo_sergeant",
	"allies_hq_promo_lieutenant",
	"allies_hq_promo_captain",
	"allies_hq_promo_major",
	"allies_hq_promo_colonel",
	"allies_hq_promo_general_brigadier",
	"allies_hq_promo_general_lieutenant",
	"allies_hq_promo_general",
};

const char *rankSoundNames_Axis[NUM_EXPERIENCE_LEVELS] =
{
	"",
	"axis_hq_promo_private",
	"axis_hq_promo_corporal",
	"axis_hq_promo_sergeant",
	"axis_hq_promo_lieutenant",
	"axis_hq_promo_captain",
	"axis_hq_promo_major",
	"axis_hq_promo_colonel",
	"axis_hq_promo_general_major",
	"axis_hq_promo_general_lieutenant",
	"axis_hq_promo_general",
};

const char *rankNames_Axis[NUM_EXPERIENCE_LEVELS] =
{
	"Schutze",
	"Oberschutze",
	"Gefreiter",
	"Feldwebel",
	"Leutnant",
	"Hauptmann",
	"Major",
	"Oberst",
	"Generalmajor",
	"Generalleutnant",
	"General",
};

const char *rankNames_Allies[NUM_EXPERIENCE_LEVELS] =
{
	"Private",
	"Private 1st Class",
	"Corporal",
	"Sergeant",
	"Lieutenant",
	"Captain",
	"Major",
	"Colonel",
	"Brigadier General",
	"Lieutenant General",
	"General",
};

const char *miniRankNames_Axis[NUM_EXPERIENCE_LEVELS] =
{
	"Stz",
	"Otz",
	"Gfr",
	"Fwb",
	"Ltn",
	"Hpt",
	"Mjr",
	"Obs",
	"GMj",
	"GLt",
	"Gen",
};

const char *miniRankNames_Allies[NUM_EXPERIENCE_LEVELS] =
{
	"Pvt",
	"PFC",
	"Cpl",
	"Sgt",
	"Lt",
	"Cpt",
	"Maj",
	"Cnl",
	"BGn",
	"LtG",
	"Gen",
};

/**
 * @brief BG_Find_PathCorner
 * @param[in] match
 * @return
 */
pathCorner_t *BG_Find_PathCorner(const char *match)
{
	int i;

	for (i = 0 ; i < numPathCorners; i++)
	{
		if (!Q_stricmp(pathCorners[i].name, match))
		{
			return &pathCorners[i];
		}
	}

	return NULL;
}

/**
 * @brief BG_AddPathCorner
 * @param[in] name
 * @param[in] origin
 */
void BG_AddPathCorner(const char *name, vec3_t origin)
{
	if (numPathCorners >= MAX_PATH_CORNERS)
	{
		Com_Error(ERR_DROP, "MAX PATH CORNERS (%i) hit", MAX_PATH_CORNERS);
	}

	VectorCopy(origin, pathCorners[numPathCorners].origin);
	Q_strncpyz(pathCorners[numPathCorners].name, name, 64);
	numPathCorners++;
}

/**
 * @brief BG_Find_Spline
 * @param[in] match
 * @return
 */
splinePath_t *BG_Find_Spline(const char *match)
{
	int i;

	for (i = 0 ; i < numSplinePaths; i++)
	{
		if (!Q_stricmp(splinePaths[i].point.name, match))
		{
			return &splinePaths[i];
		}
	}

	return NULL;
}

/**
 * @brief BG_AddSplinePath
 * @param[in] name
 * @param[in] target
 * @param[in] origin
 * @return
 */
splinePath_t *BG_AddSplinePath(const char *name, const char *target, vec3_t origin)
{
	splinePath_t *spline;

	if (numSplinePaths >= MAX_SPLINE_PATHS)
	{
		Com_Error(ERR_DROP, "MAX SPLINES (%i) hit", MAX_SPLINE_PATHS);
	}

	spline = &splinePaths[numSplinePaths];

	memset(spline, 0, sizeof(splinePath_t));

	VectorCopy(origin, spline->point.origin);

	Q_strncpyz(spline->point.name, name, 64);
	Q_strncpyz(spline->strTarget, target ? target : "", 64);

	spline->numControls = 0;

	numSplinePaths++;

	return spline;
}

/**
 * @brief BG_AddSplineControl
 * @param[in,out] spline
 * @param[in] name
 */
void BG_AddSplineControl(splinePath_t *spline, const char *name)
{
	if (spline->numControls >= MAX_SPLINE_CONTROLS)
	{
		Com_Error(ERR_DROP, "MAX SPLINE CONTROLS (%i) hit", MAX_SPLINE_CONTROLS);
	}

	Q_strncpyz(spline->controls[spline->numControls].name, name, 64);

	spline->numControls++;
}

/**
 * @brief BG_SplineLength
 * @param[in] pSpline
 * @return
 */
float BG_SplineLength(splinePath_t *pSpline)
{
	float i;
	float granularity = 0.01f;
	float dist        = 0.f;
	//float tension;
	vec3_t vec[2];
	vec3_t lastPoint = { 0 };
	vec3_t result;

	for (i = 0.f; i <= 1.f; i += granularity)
	{
		/*      if(pSpline->isStart) {
		            tension = 1 - sin((1 - i) * M_PI * 0.5f);
		        } else if(pSpline->isEnd) {
		            tension = sin(i * M_PI * 0.5f);
		        } else {
		            tension = i;
		        }*/

		BG_CalculateSpline_r(pSpline, vec[0], vec[1], i);
		VectorSubtract(vec[1], vec[0], result);
		VectorMA(vec[0], i, result, result);

		if (i != 0.f)
		{
			VectorSubtract(result, lastPoint, vec[0]);
			dist += VectorLength(vec[0]);
		}

		VectorCopy(result, lastPoint);
	}

	return dist;
}

/**
 * @brief BG_BuildSplinePaths
 */
void BG_BuildSplinePaths(void)
{
	int          i, j;
	pathCorner_t *pnt;
	splinePath_t *spline, *st;

	for (i = 0; i < numSplinePaths; i++)
	{
		spline = &splinePaths[i];

		if (*spline->strTarget)
		{
			for (j = 0; j < spline->numControls; j++)
			{
				pnt = BG_Find_PathCorner(spline->controls[j].name);

				if (!pnt)
				{
					Com_Printf("^1Cant find control point (%s) for spline (%s)\n", spline->controls[j].name, spline->point.name);
					// Just changing to a warning for now, easier for region compiles...
					continue;

				}
				else
				{
					VectorCopy(pnt->origin, spline->controls[j].origin);
				}
			}

			st = BG_Find_Spline(spline->strTarget);
			if (!st)
			{
				Com_Printf("^1Cant find target point (%s) for spline (%s)\n", spline->strTarget, spline->point.name);
				// Just changing to a warning for now, easier for region compiles...
				continue;
			}

			spline->next = st;

			spline->length = BG_SplineLength(spline);
			BG_ComputeSegments(spline);
		}
	}

	for (i = 0; i < numSplinePaths; i++)
	{
		spline = &splinePaths[i];

		if (spline->next)
		{
			spline->next->prev = spline;
		}
	}
}

/**
 * @brief BG_GetSplineData
 * @param[in] number
 * @param[out] backwards
 * @return
 */
splinePath_t *BG_GetSplineData(int number, qboolean *backwards)
{
	if (number < 0)
	{
		*backwards = qtrue;
		number     = -number;
	}
	else
	{
		*backwards = qfalse;
	}
	number--;

	if (number < 0 || number >= numSplinePaths)
	{
		return NULL;
	}

	return &splinePaths[number];
}

/**
 * @brief BG_MaxAmmoForWeapon
 * @param[in] weaponNum
 * @param[in] skill
 * @return
 */
int BG_MaxAmmoForWeapon(weapon_t weaponNum, int *skill)
{
	switch (weaponNum)
	{
	//case WP_KNIFE:
	case WP_LUGER:
	case WP_COLT:
	case WP_STEN:
	case WP_SILENCER:
	case WP_CARBINE:
	case WP_KAR98:
	case WP_SILENCED_COLT:
		if (skill[SK_LIGHT_WEAPONS] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + GetAmmoTableData(weaponNum)->maxclip);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	case WP_MP40:
	case WP_THOMPSON:
		if (skill[SK_FIRST_AID] >= 1 || skill[SK_LIGHT_WEAPONS] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + GetAmmoTableData(weaponNum)->maxclip);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	case WP_M7:
	case WP_GPG40:
		if (skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + 4);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
		// FIXME: this is class dependant, not ammo table
		if (skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + 4);
		}
		else if (skill[SK_FIRST_AID] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + 1);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	/*case WP_MOBILE_MG42:
	case WP_MOBILE_BROWNING
	case WP_PANZERFAUST:
	case WP_BAZOOKA:
	case WP_FLAMETHROWER:
	    if( skill[SK_HEAVY_WEAPONS] >= 1 )
	        return( GetAmmoTableData(weaponNum)->maxammo + GetAmmoTableData(weaponNum)->maxclip );
	    else
	        return( GetAmmoTableData(weaponNum)->maxammo );
	    break;
	case WP_MORTAR:
	case WP_MORTAR_SET:
	case WP_MORTAR2:
	case WP_MORTAR2_SET:
	    if( skill[SK_HEAVY_WEAPONS] >= 1 )
	        return( GetAmmoTableData(weaponNum)->maxammo + 2 );
	    else
	        return( GetAmmoTableData(weaponNum)->maxammo );
	    break;*/
	case WP_MEDIC_SYRINGE:
		if (skill[SK_FIRST_AID] >= 2)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + 2);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	case WP_GARAND:
	case WP_K43:
	case WP_FG42:
		if (skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1 || skill[SK_LIGHT_WEAPONS] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + GetAmmoTableData(weaponNum)->maxclip);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_FG42SCOPE:
		if (skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1)
		{
			return(GetAmmoTableData(weaponNum)->maxammo + GetAmmoTableData(weaponNum)->maxclip);
		}
		else
		{
			return(GetAmmoTableData(weaponNum)->maxammo);
		}
		break;
	default:
		return(GetAmmoTableData(weaponNum)->maxammo);
	}
}

/**
 * @brief BG_AdjustAAGunMuzzleForBarrel
 * @param[in,out] origin
 * @param[in] forward
 * @param[in] right
 * @param[in] up
 * @param[in] barrel
 */
void BG_AdjustAAGunMuzzleForBarrel(vec_t *origin, vec_t *forward, vec_t *right, vec_t *up, int barrel)
{
	switch (barrel)
	{
	case 0:
		VectorMA(origin, 64, forward, origin);
		VectorMA(origin, 20, right, origin);
		VectorMA(origin, 40, up, origin);
		break;
	case 1:
		VectorMA(origin, 64, forward, origin);
		VectorMA(origin, 20, right, origin);
		VectorMA(origin, 20, up, origin);
		break;
	case 2:
		VectorMA(origin, 64, forward, origin);
		VectorMA(origin, -20, right, origin);
		VectorMA(origin, 40, up, origin);
		break;
	case 3:
		VectorMA(origin, 64, forward, origin);
		VectorMA(origin, -20, right, origin);
		VectorMA(origin, 20, up, origin);
		break;
	}
}

/**
 * @brief PC_SourceWarning
 * @param[in] handle
 * @param[in] format
 * @note Unused
 */
void PC_SourceWarning(int handle, const char *format, ...)
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

	Com_Printf(S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line, string);
}

/**
 * @brief PC_SourceError
 * @param[in] handle
 * @param[in] format
 */
void PC_SourceError(int handle, const char *format, ...)
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

#ifdef GAMEDLL
	Com_Error(ERR_DROP, S_COLOR_RED "ERROR: %s, line %d: %s", filename, line, string);
#else
	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);
#endif
}

/**
 * @brief PC_Float_Parse
 * @param[in] handle
 * @param[out] f
 * @return
 */
qboolean PC_Float_Parse(int handle, float *f)
{
	pc_token_t token;
	int        negative = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if (token.string[0] == '-')
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			return qfalse;
		}
		negative = qtrue;
	}
	if (token.type != TT_NUMBER)
	{
		PC_SourceError(handle, "expected float but found %s\n", token.string);
		return qfalse;
	}
	if (negative)
	{
		*f = -token.floatvalue;
	}
	else
	{
		*f = token.floatvalue;
	}
	return qtrue;
}

/**
 * @brief PC_Color_Parse
 * @param[in] handle
 * @param[out] c
 * @return
 */
qboolean PC_Color_Parse(int handle, vec4_t *c)
{
	int   i;
	float f;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/**
 * @brief PC_Vec_Parse
 * @param[in] handle
 * @param[out] c
 * @return
 */
qboolean PC_Vec_Parse(int handle, vec3_t *c)
{
	int   i;
	float f;

	for (i = 0; i < 3; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}
		(*c)[i] = f;
	}
	return qtrue;
}

/**
 * @brief PC_Int_Parse
 * @param[in] handle
 * @param[out] i
 * @return
 */
qboolean PC_Int_Parse(int handle, int *i)
{
	pc_token_t token;
	int        negative = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if (token.string[0] == '-')
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			return qfalse;
		}
		negative = qtrue;
	}
	if (token.type != TT_NUMBER)
	{
		PC_SourceError(handle, "expected integer but found %s\n", token.string);
		return qfalse;
	}
	*i = token.intvalue;
	if (negative)
	{
		*i = -*i;
	}
	return qtrue;
}

#ifdef GAMEDLL
/**
 * @brief PC_String_Parse
 * @param[in] handle
 * @return
 */
const char *PC_String_Parse(int handle)
{
	static char buf[MAX_TOKEN_CHARS];
	pc_token_t  token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return NULL;
	}

	Q_strncpyz(buf, token.string, MAX_TOKEN_CHARS);
	return buf;
}

#else

/**
 * @brief PC_String_Parse
 * @param[in] handle
 * @param[out] out
 * @return
 */
qboolean PC_String_Parse(int handle, const char **out)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	*(out) = String_Alloc(token.string);
	return qtrue;
}
#endif

/**
 * @brief Same as PC_String_Parse, but uses a static buff and not the string memory pool
 * @param handle
 * @param out
 * @param size
 * @return
 */
qboolean PC_String_ParseNoAlloc(int handle, char *out, size_t size)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	Q_strncpyz(out, token.string, size);
	return qtrue;
}

const char *bg_fireteamNames[MAX_FIRETEAMS / 2] =
{
	"Alfa",
	"Bravo",
	"Charlie",
	"Delta",
	"Echo",
	"Foxtrot",
};

const voteType_t voteToggles[] =
{
	{ "vote_allow_config",                   CV_SVF_CONFIG                 },
	{ "vote_allow_gametype",                 CV_SVF_GAMETYPE               },
	{ "vote_allow_kick",                     CV_SVF_KICK                   },
	{ "vote_allow_map",                      CV_SVF_MAP                    },
	{ "vote_allow_matchreset",               CV_SVF_MATCHRESET             },
	{ "vote_allow_mutespecs",                CV_SVF_MUTESPECS              },
	{ "vote_allow_nextmap",                  CV_SVF_NEXTMAP                },
	{ "vote_allow_referee",                  CV_SVF_REFEREE                },
	{ "vote_allow_shuffleteamsxp",           CV_SVF_SHUFFLETEAMS           },
	{ "vote_allow_shuffleteamsxp_norestart", CV_SVF_SHUFFLETEAMS_NORESTART },
	{ "vote_allow_swapteams",                CV_SVF_SWAPTEAMS              },
	{ "vote_allow_friendlyfire",             CV_SVF_FRIENDLYFIRE           },
	{ "vote_allow_timelimit",                CV_SVF_TIMELIMIT              },
	{ "vote_allow_warmupdamage",             CV_SVF_WARMUPDAMAGE           },
	{ "vote_allow_antilag",                  CV_SVF_ANTILAG                },
	{ "vote_allow_balancedteams",            CV_SVF_BALANCEDTEAMS          },
	{ "vote_allow_muting",                   CV_SVF_MUTING                 },
	{ "vote_allow_surrender",                CV_SVF_SURRENDER              },
	{ "vote_allow_restartcampaign",          CV_SVF_RESTARTCAMPAIGN        },
	{ "vote_allow_nextcampaign",             CV_SVF_NEXTCAMPAIGN           },
	{ "vote_allow_poll",                     CV_SVF_POLL                   }
};

int numVotesAvailable = sizeof(voteToggles) / sizeof(voteType_t);

// consts to offset random reinforcement seeds
const unsigned int aReinfSeeds[MAX_REINFSEEDS] = { 11, 3, 13, 7, 2, 5, 1, 17 };

// Weapon full names + headshot capability
const weap_ws_t aWeaponInfo[WS_MAX] =
{
	{ qfalse, "KNIF", "Knife"      },  // 0  WS_KNIFE
	{ qfalse, "KNKB", "Ka-Bar"     },  // 1  WS_KNIFE_KBAR
	{ qtrue,  "LUGR", "Luger"      },  // 2  WS_LUGER
	{ qtrue,  "COLT", "Colt"       },  // 3  WS_COLT
	{ qtrue,  "MP40", "MP 40"      },  // 4  WS_MP40
	{ qtrue,  "TMPS", "Thompson"   },  // 5  WS_THOMPSON
	{ qtrue,  "STEN", "Sten"       },  // 6  WS_STEN
	{ qtrue,  "FG42", "FG 42"      },  // 7  WS_FG42
	{ qtrue,  "PNZR", "Panzer"     },  // 8  WS_PANZERFAUST
	{ qtrue,  "BZKA", "Bazooka"    },  // 9  WS_BAZOOKA
	{ qtrue,  "FLAM", "F.Thrower"  },  // 10 WS_FLAMETHROWER
	{ qfalse, "GRND", "Grenade"    },  // 11 WS_GRENADE
	{ qfalse, "MRTR", "Mortar"     },  // 12 WS_MORTAR
	{ qfalse, "GRWF", "Granatwerf" },  // 13 WS_MORTAR2
	{ qfalse, "DYNA", "Dynamite"   },  // 14 WS_DYNAMITE
	{ qfalse, "ARST", "Airstrike"  },  // 15 WS_AIRSTRIKE
	{ qfalse, "ARTY", "Artillery"  },  // 16 WS_ARTILLERY
	{ qfalse, "STCH", "Satchel"    },  // 17 WS_SATCHEL
	{ qfalse, "GRLN", "G.Launchr"  },  // 18 WS_GRENADELAUNCHER
	{ qfalse, "LNMN", "Landmine"   },  // 19 WS_LANDMINE
	{ qtrue,  "MG42", "MG 42 Gun"  },  // 20 WS_MG42
	{ qtrue,  "BRNG", "Browning"   },  // 21 WS_BROWNING
	{ qtrue,  "GARN", "Garand"     },  // 22 WS_CARBINE
	{ qtrue,  "K-43", "K43 Rifle"  },  // 23 WS_KAR98
	{ qtrue,  "SGRN", "Scp.Garand" },  // 24 WS_GARAND
	{ qtrue,  "SK43", "Scp.K43"    }   // 25 WS_K43
};

/**
 * @brief Multiview: Convert weaponstate to simpler format
 * @param[in] ws
 * @return
 */
int BG_simpleWeaponState(int ws)
{
	switch (ws)
	{
	case WEAPON_READY:
	case WEAPON_READYING:
	case WEAPON_RELAXING:
		return WSTATE_IDLE;
	case WEAPON_RAISING:
	case WEAPON_DROPPING:
	case WEAPON_DROPPING_TORELOAD:
		return WSTATE_SWITCH;
	case WEAPON_FIRING:
	case WEAPON_FIRINGALT:
		return WSTATE_FIRE;
	case WEAPON_RELOADING:
		return WSTATE_RELOAD;
	}

	return WSTATE_IDLE;
}

#ifdef FEATURE_MULTIVIEW

/**
 * @brief // Multiview: Reduce hint info to 2 bits.  However, we can really
 * have up to 8 values, as some hints will have a 0 value for
 * cursorHintVal
 * @param hint
 * @param val
 * @return
 */
int BG_simpleHintsCollapse(int hint, int val)
{
	switch (hint)
	{
	case HINT_DISARM:
		if (val > 0)
		{
			return 0;
		}
		break;
	case HINT_BUILD:
		if (val >= 0)
		{
			return 1;
		}
		break;
	case HINT_BREAKABLE:
		if (val == 0)
		{
			return 1;
		}
		break;
	case HINT_DOOR_ROTATING:
	case HINT_BUTTON:
	case HINT_MG42:
		if (val == 0)
		{
			return 2;
		}
		break;
	case HINT_BREAKABLE_DYNAMITE:
		if (val == 0)
		{
			return 3;
		}
		break;
	default:
		break;
	}

	return 0;
}

/**
 * @brief Multiview: Expand the hints. Because we map a couple hints
 * into a single value, we can't replicate the proper hint back
 * in all cases.
 * @param[in] hint
 * @param[in] val
 * @return
 */
int BG_simpleHintsExpand(int hint, int val)
{
	switch (hint)
	{
	case 0:
		return((val >= 0) ? HINT_DISARM : 0);
	case 1:
		return((val >= 0) ? HINT_BUILD : HINT_BREAKABLE);
	case 2:
		return((val >= 0) ? HINT_BUILD : HINT_MG42);
	case 3:
		return((val >= 0) ? HINT_BUILD : HINT_BREAKABLE_DYNAMITE);
	default:
		break;
	}

	return 0;
}
#endif

// Only used locally
typedef struct
{
	const char *colorname;
	vec4_t *color;
} colorTable_t;

// Colors for crosshairs
const colorTable_t OSP_Colortable[] =
{
	{ "white",    &colorWhite    },
	{ "red",      &colorRed      },
	{ "green",    &colorGreen    },
	{ "blue",     &colorBlue     },
	{ "yellow",   &colorYellow   },
	{ "magenta",  &colorMagenta  },
	{ "cyan",     &colorCyan     },
	{ "orange",   &colorOrange   },
	{ "mdred",    &colorMdRed    },
	{ "mdgreen",  &colorMdGreen  },
	{ "dkgreen",  &colorDkGreen  },
	{ "mdcyan",   &colorMdCyan   },
	{ "mdyellow", &colorMdYellow },
	{ "mdorange", &colorMdOrange },
	{ "mdblue",   &colorMdBlue   },
	{ "ltgrey",   &colorLtGrey   },
	{ "mdgrey",   &colorMdGrey   },
	{ "dkgrey",   &colorDkGrey   },
	{ "black",    &colorBlack    },
	{ NULL,       NULL           }
};

extern void trap_Cvar_Set(const char *var_name, const char *value);
/**
 * @brief BG_setCrosshair
 * @param[in] colString
 * @param[in] col
 * @param[in] alpha
 * @param[in] cvarName
 */
void BG_setCrosshair(char *colString, float *col, float alpha, const char *cvarName)
{
	char *s = colString;

	col[0] = 1.0f;
	col[1] = 1.0f;
	col[2] = 1.0f;
	col[3] = (alpha > 1.0f) ? 1.0f : (alpha < 0.0f) ? 0.0f : alpha;

	if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X'))
	{
		s += 2;
		// parse rrggbb
		if (Q_IsHexColorString(s))
		{
			col[0] = ((float)(gethex(*(s)) * 16.f + gethex(*(s + 1)))) / 255.00f;
			col[1] = ((float)(gethex(*(s + 2)) * 16.f + gethex(*(s + 3)))) / 255.00f;
			col[2] = ((float)(gethex(*(s + 4)) * 16.f + gethex(*(s + 5)))) / 255.00f;
			return;
		}
	}
	else
	{
		int i = 0;

		while (OSP_Colortable[i].colorname != NULL)
		{
			if (Q_stricmp(s, OSP_Colortable[i].colorname) == 0)
			{
				col[0] = (*OSP_Colortable[i].color)[0];
				col[1] = (*OSP_Colortable[i].color)[1];
				col[2] = (*OSP_Colortable[i].color)[2];
				return;
			}
			i++;
		}
	}

	trap_Cvar_Set(cvarName, "White");
}

///////////////////////////////////////////////////////////////////////////////

typedef struct locInfo_s
{
	vec2_t gridStartCoord;
	vec2_t gridStep;
} locInfo_t;

static locInfo_t locInfo;

/**
 * @brief BG_InitLocations
 * @param[in] world_mins
 * @param[in] world_maxs
 */
void BG_InitLocations(vec2_t world_mins, vec2_t world_maxs)
{
	// keep this in sync with CG_DrawGrid
	locInfo.gridStep[0] = 1200.f;
	locInfo.gridStep[1] = 1200.f;

	// ensure minimal grid density
	while ((world_maxs[0] - world_mins[0]) / locInfo.gridStep[0] < 7)
	{
		locInfo.gridStep[0] -= 50.f;
	}
	while ((world_mins[1] - world_maxs[1]) / locInfo.gridStep[1] < 7)
	{
		locInfo.gridStep[1] -= 50.f;
	}

	locInfo.gridStartCoord[0] = world_mins[0] + .5f * ((((world_maxs[0] - world_mins[0]) / locInfo.gridStep[0]) - ((int)((world_maxs[0] - world_mins[0]) / locInfo.gridStep[0]))) * locInfo.gridStep[0]);
	locInfo.gridStartCoord[1] = world_mins[1] - .5f * ((((world_mins[1] - world_maxs[1]) / locInfo.gridStep[1]) - ((int)((world_mins[1] - world_maxs[1]) / locInfo.gridStep[1]))) * locInfo.gridStep[1]);
}

static char coord[6];

/**
 * @brief BG_GetLocationString
 * @param[in] xpos
 * @param[in] ypos
 * @return
 */
char *BG_GetLocationString(float xpos, float ypos)
{
	int x = (xpos - locInfo.gridStartCoord[0]) / locInfo.gridStep[0];
	int y = (locInfo.gridStartCoord[1] - ypos) / locInfo.gridStep[1];

	coord[0] = '\0';

	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}

	Com_sprintf(coord, sizeof(coord), "%c,%i", 'A' + x, y);

	return coord;
}

/**
 * @brief BG_BBoxCollision
 * @param[in] min1
 * @param[in] max1
 * @param[in] min2
 * @param[in] max2
 * @return
 */
qboolean BG_BBoxCollision(vec3_t min1, vec3_t max1, vec3_t min2, vec3_t max2)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (min1[i] > max2[i])
		{
			return qfalse;
		}
		if (min2[i] > max1[i])
		{
			return qfalse;
		}
	}

	return qtrue;
}

/////////////////////////

/**
 * @brief BG_FootstepForSurface
 * @param[in] surfaceFlags
 * @return
 */
int BG_FootstepForSurface(int surfaceFlags)
{
	if (surfaceFlags & SURF_NOSTEPS)
	{
		return FOOTSTEP_TOTAL;
	}

	if (surfaceFlags & SURF_METAL)
	{
		return FOOTSTEP_METAL;
	}

	if (surfaceFlags & SURF_WOOD)
	{
		return FOOTSTEP_WOOD;
	}

	if (surfaceFlags & SURF_GRASS)
	{
		return FOOTSTEP_GRASS;
	}

	if (surfaceFlags & SURF_GRAVEL)
	{
		return FOOTSTEP_GRAVEL;
	}

	if (surfaceFlags & SURF_ROOF)
	{
		return FOOTSTEP_ROOF;
	}

	if (surfaceFlags & SURF_SNOW)
	{
		return FOOTSTEP_SNOW;
	}

	if (surfaceFlags & SURF_CARPET)
	{
		return FOOTSTEP_CARPET;
	}

	if (surfaceFlags & SURF_SPLASH)
	{
		return FOOTSTEP_SPLASH;
	}

	return FOOTSTEP_NORMAL;
}
