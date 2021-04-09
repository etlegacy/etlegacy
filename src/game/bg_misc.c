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
 * @file bg_misc.c
 * @brief Both games misc functions, all completely stateless
 */

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "../../etmain/ui/menudef.h"

#ifdef GAMEDLL
extern vmCvar_t g_developer;
extern vmCvar_t team_riflegrenades;
#endif

// *INDENT-OFF*
/**
 * @var skillTable
 * @brief New skill table
 * [0]  = skill           -
 * [1]  = skillNames      -
 * [2]  = skillNamesLine1 - NOTE: Unused
 * [3]  = skillNamesLine2 - NOTE: Unused
 * [4]  = medalNames      - NOTE: Unused
 * [5]  = skillLevels     -
 */
skilltable_t skillTable[SK_NUM_SKILLS] =
{
	// skill                                       skillNames       skillNamesLine1 skillNamesLine2 medalNames                     skillLevels
	{ SK_BATTLE_SENSE,                             "Battle Sense",  "Battle",       "Sense",        "Distinguished Service Medal", { 0, 20, 50, 90, 140 },},  // 0 SK_BATTLE_SENSE
	{ SK_EXPLOSIVES_AND_CONSTRUCTION,              "Engineering",   "Engineering",  "",             "Steel Star",                  { 0, 20, 50, 90, 140 },},  // 1 SK_EXPLOSIVES_AND_CONSTRUCTION
	{ SK_FIRST_AID,                                "First Aid",     "First",        "Aid",          "Silver Cross",                { 0, 20, 50, 90, 140 },},  // 2 SK_FIRST_AID
	{ SK_SIGNALS,                                  "Signals",       "Signals",      "",             "Signals Medal",               { 0, 20, 50, 90, 140 },},  // 3 SK_SIGNALS
	{ SK_LIGHT_WEAPONS,                            "Light Weapons", "Light",        "Weapons",      "Infantry Medal",              { 0, 20, 50, 90, 140 },},  // 4 SK_LIGHT_WEAPONS
	{ SK_HEAVY_WEAPONS,                            "Heavy Weapons", "Heavy",        "Weapons",      "Bombardment Medal",           { 0, 20, 50, 90, 140 },},  // 5 SK_HEAVY_WEAPONS
	{ SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, "Covert Ops",    "Covert",       "Ops",          "Silver Snake",                { 0, 20, 50, 90, 140 },},  // 6 SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS
};
// *INDENT-ON*

sysMessage_t HQMessages[SM_NUM_SYS_MSGS] =
{
	{ "SYS_NeedMedic",         "_hq_need_medic",          "^7(HQ): ^3We need a Medic!"            },
	{ "SYS_NeedEngineer",      "_hq_need_engineer",       "^7(HQ): ^3We need an Engineer!"        },
	{ "SYS_NeedFieldops",      "_hq_need_fieldops",       "^7(HQ): ^3We need an Field Ops!"       },
	{ "SYS_NeedCovertOps",     "_hq_need_covertops",      "^7(HQ): ^3We need an Covert Ops!"      },
	{ "SYS_MenDown",           "_hq_men_down",            "^7(HQ): ^3We've lost most of our men!" },
	{ "SYS_ObjCaptured",       "_hq_objective_captured",  "^7(HQ): ^3Objective captured!"         },
	{ "SYS_ObjLost",           "_hq_objective_lost",      "^7(HQ): ^3Objective lost!"             },
	{ "SYS_ObjDestroyed",      "_hq_objective_destroyed", "^7(HQ): ^3Objective destroyed!"        },
	{ "SYS_ConstructComplete", "_hq_const_completed",     "^7(HQ): ^3Construction complete!"      },
	{ "SYS_ConstructFailed",   "_hq_const_failed",        "^7(HQ): ^3Construction failed!"        },
	{ "SYS_Destroyed",         "_hq_const_destroyed",     "^7(HQ): ^3Construction destroyed!"     },
};

vec3_t playerlegsProneMins = { -13.5f, -13.5f, -24.f };
vec3_t playerlegsProneMaxs = { 13.5f, 13.5f, -14.4f };

vec3_t playerHeadProneMins = { -6.f, -6.f, -24.f };
vec3_t playerHeadProneMaxs = { 6.f, 6.f, 0.f };

int          numSplinePaths;
splinePath_t splinePaths[MAX_SPLINE_PATHS];

int          numPathCorners;
pathCorner_t pathCorners[MAX_PATH_CORNERS];

// these defines are matched with the character torso animations
#define DELAY_LOW       100 ///< machineguns, tesla, spear, flame
#define DELAY_HIGH      100 ///< mauser, garand
#define DELAY_PISTOL    100 ///< colt, luger, sp5, cross
#define DELAY_SHOULDER  50  ///< rl
#define DELAY_THROW     250 ///< grenades, dynamite
#define DELAY_HW        750 ///< heavy weapon

// *INDENT-OFF*
/**
 * @var weaponTable
 * @brief New weapon table to store common weapon properties:
 * [0]   = weapon               - reference
 * [1]   = item                 -
 * [2]   = team                 -
 * [3]   = skillBased           -
 * [4]   = weapAlts             -
 * [5]   = weapEquiv            - the id of the opposite team's weapon (but not for WP_GPG40 <-> WP_M7 - see CG_OutOfAmmoChange).
 * [6]   = akimboSideArm        -
 * [7]   = ammoIndex            - type of weapon ammo this uses
 * [8]   = clipIndex            - which clip this weapon uses. This allows the sniper rifle to use the same clip as the garand, etc.
 * [9]   = damage               - returns 1 for no damage, return 0 for no explode on contact ... FIXME: some weapons are handled differently f.e. VERYBIGEXPLOSION
 * [10]  = spread               -
 * [11]  = spreadScale          -
 * [12]  = splashDamage         -
 * [13]  = splashRadius         -
 * [14]  = type                 -
 * [15]  = fireMode             -
 * [16]  = attributes           -
 * [17]  = zoomOut              -
 * [18]  = zoomIn               -
 * [19]  = zoomedScope          -
 * [20]  = desc                 -
 * [21]  = indexWeaponStat      -
 * [22]  = useAmmo              -
 * [23]  = useClip              -
 * [24]  = maxAmmo              - max player ammo carrying capacity.
 * [25]  = uses                 - how many 'rounds' it takes/costs to fire one cycle.
 * [26]  = maxClip              - max 'rounds' in a clip.
 * [27]  = reloadTime           - time from start of reload until ready to fire.
 * [28]  = fireDelayTime        - time from pressing 'fire' until first shot is fired. (used for delaying fire while weapon is 'readied' in animation)
 * [29]  = nextShotTime         - when firing continuously, this is the time between shots
 * [30]  = grenadeTime          -
 * [31]  = aimSpreadScaleAdd    -
 * [32]  = maxHeat              - max active firing time before weapon 'overheats' (at which point the weapon will fail for a moment)
 * [33]  = coolRate             - how fast the weapon cools down.
 * [34]  = heatRecoveryTime     - time from overheats until weapon can fire again
 * [35]  = switchTimeBegin      -
 * [36]  = switchTimeFinish     -
 * [37]  = altSwitchTimeBegin   -
 * [38]  = altSwitchTimeFinish  -
 * [39]  = knockback            -
 * [40]  = weapRecoilDuration   -
 * [41]  = weapRecoilPitch      -
 * [42]  = weapRecoilYaw        -
 * [43]  = className            -
 * [44]  = weapFile             -
 * [45]  = chargeTimeCoeff      -
 * [46]  = mod                  - means of death.
 * [47]  = splashMod            - splash means of death.
 */
weaponTable_t weaponTable[WP_NUM_WEAPONS] =
{
	// weapon                  item                               team         skillBased                                   weapAlts                weapEquiv          akimboSidearm   ammoIndex             clipIndex             damage spread spreadScale splashDamage splashRadius type                                       firingMode                                                   attributes                                                                                                                                                   zoomOut zoomIn desc                   indexWeaponStat     useAmmo useClip maxAmmo uses maxClip reloadTime firstDelayTime nextShotTime grenadeTime aimSpreadScaleAdd maxHeat coolRate heatRecoveryTime switchTimeBegin switchTimeFinish altSwitchTimeFrom altswitchTimeTo knockback muzzlePointOffset weapRecoilDuration weapRecoilPitch weapRecoilYaw  className          weapFile                 chargeTimeCoeff                      mod                       splashMod
	{ WP_NONE,                 ITEM_NONE,                         TEAM_FREE,   SK_NUM_SKILLS,                               WP_NONE,                WP_NONE,                 WP_NONE,  WP_NONE,              WP_NONE,              1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE,                                                                                         0,      0,     "WP_NONE",             WS_MAX,             qfalse, qfalse, 0,      0,   0,      0,         50,            0,           0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 0, 0, 0 },      0,                 { 0, 0 },       { 0, 0 },      "",                NULL,                    {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_NONE                  // 0
	{ WP_KNIFE,                ITEM_WEAPON_KNIFE,                 TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_NONE,                WP_KNIFE_KABAR,          WP_NONE,  WP_KNIFE,             WP_KNIFE,             10,    0,     0,          0,           0,           WEAPON_TYPE_MELEE,                         WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE,                                                       0,      0,     "KNIFE",               WS_KNIFE,           qfalse, qfalse, 999,    0,   999,    0,         50,            200,         0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "knife",                 {1, 1, 1, 1, 1},                     MOD_KNIFE,                MOD_KNIFE                },  // WP_KNIFE                 // 1
	{ WP_LUGER,                ITEM_WEAPON_LUGER,                 TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_SILENCER,            WP_COLT,                 WP_NONE,  WP_LUGER,             WP_LUGER,             18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                      0,      0,     "LUGER",               WS_LUGER,           qtrue,  qtrue,  24,     1,   8,      1500,      DELAY_PISTOL,  400,         0,          20,               0,      0,       0,               250,            250,             1000,             1190,           0,        { 14, 6, -4 },    100,               { .45f, .15f }, { 0, 0 },      "",                "luger",                 {1, 1, 1, 1, 1},                     MOD_LUGER,                MOD_LUGER                },  // WP_LUGER                 // 2    // NOTE: also 32 round 'snail' magazine
	{ WP_MP40,                 ITEM_WEAPON_MP40,                  TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_NONE,                WP_THOMPSON,             WP_NONE,  WP_MP40,              WP_MP40,              18,    400,   0.6f,       0,           0,           WEAPON_TYPE_SMG,                           WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                      0,      0,     "MP 40",               WS_MP40,            qtrue,  qtrue,  90,     1,   30,     2400,      DELAY_LOW,     150,         0,          15,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "mp40",                  {1, 1, 1, 1, 1},                     MOD_MP40,                 MOD_MP40                 },  // WP_MP40                  // 3
	{ WP_GRENADE_LAUNCHER,     ITEM_WEAPON_GRENADE_LAUNCHER,      TEAM_AXIS,   SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_NONE,                WP_GRENADE_PINEAPPLE,    WP_NONE,  WP_GRENADE_LAUNCHER,  WP_GRENADE_LAUNCHER,  0,     0,     0,          250,         250,         WEAPON_TYPE_GRENADE,                       WEAPON_FIRING_MODE_THROWABLE,                                WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_SHAKE,                                                                     0,      0,     "",                    WS_GRENADE,         qtrue,  qfalse, 4,      1,   1,      0,         DELAY_THROW,   1600,        4000,       0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 20, 0 },    0,                 { 0, 0 },       { 0, 0 },      "grenade",         "grenade",               {1, 1, 1, 1, 1},                     MOD_GRENADE_LAUNCHER,     MOD_GRENADE_LAUNCHER     },  // WP_GRENADE_LAUNCHER      // 4
	{ WP_PANZERFAUST,          ITEM_WEAPON_PANZERFAUST,           TEAM_AXIS,   SK_HEAVY_WEAPONS,                            WP_NONE,                WP_BAZOOKA,              WP_NONE,  WP_PANZERFAUST,       WP_PANZERFAUST,       400,   0,     0,          400,         300,         WEAPON_TYPE_PANZER,                        WEAPON_FIRING_MODE_ONE_SHOT,                                 WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "PANZERFAUST",         WS_PANZERFAUST,     qtrue,  qfalse, 4,      1,   1,      0,         DELAY_HW,      2000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              32000.f,  { 14, 10, 0 },    0,                 { 0, 0 },       { 0, 0 },      "rocket",          "panzerfaust",           {1, .66f, .66f, .66f, .66f},         MOD_PANZERFAUST,          MOD_PANZERFAUST          },  // WP_PANZERFAUST           // 5    // updated delay so prediction is correct
	{ WP_FLAMETHROWER,         ITEM_WEAPON_FLAMETHROWER,          TEAM_FREE,   SK_HEAVY_WEAPONS,                            WP_NONE,                WP_NONE,                 WP_NONE,  WP_FLAMETHROWER,      WP_FLAMETHROWER,      5,     0,     0,          5,           5,           WEAPON_TYPE_BEAM,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "FLAMETHROWER",        WS_FLAMETHROWER,    qtrue,  qfalse, 200,    1,   200,    0,         DELAY_LOW,     50,          0,          0,                0,      0,       0,               250,            250,             0,                0,              2000.f,   { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "flamechunk",      "flamethrower",          {1, 1, 1, 1, 1},                     MOD_FLAMETHROWER,         MOD_FLAMETHROWER         },  // WP_FLAMETHROWER          // 6
	{ WP_COLT,                 ITEM_WEAPON_COLT,                  TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_SILENCED_COLT,       WP_LUGER,                WP_NONE,  WP_COLT,              WP_COLT,              18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                      0,      0,     "COLT",                WS_COLT,            qtrue,  qtrue,  24,     1,   8,      1500,      DELAY_PISTOL,  400,         0,          20,               0,      0,       0,               250,            250,             1000,             1190,           0,        { 14, 6, -4 },    100,               { .45f, .15f }, { 0, 0 },      "",                "colt",                  {1, 1, 1, 1, 1},                     MOD_COLT,                 MOD_COLT                 },  // WP_COLT                  // 7    // equivalent american weapon to german luger
	{ WP_THOMPSON,             ITEM_WEAPON_THOMPSON,              TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_NONE,                WP_MP40,                 WP_NONE,  WP_THOMPSON,          WP_THOMPSON,          18,    400,   0.6f,       0,           0,           WEAPON_TYPE_SMG,                           WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                      0,      0,     "THOMPSON",            WS_THOMPSON,        qtrue,  qtrue,  90,     1,   30,     2400,      DELAY_LOW,     150,         0,          15,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "thompson",              {1, 1, 1, 1, 1},                     MOD_THOMPSON,             MOD_THOMPSON             },  // WP_THOMPSON              // 8    // equivalent american weapon to german mp40
	{ WP_GRENADE_PINEAPPLE,    ITEM_WEAPON_GRENADE_PINEAPPLE,     TEAM_ALLIES, SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_NONE,                WP_GRENADE_LAUNCHER,     WP_NONE,  WP_GRENADE_PINEAPPLE, WP_GRENADE_PINEAPPLE, 0,     0,     0,          250,         250,         WEAPON_TYPE_GRENADE,                       WEAPON_FIRING_MODE_THROWABLE,                                WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_SHAKE,                                                                     0,      0,     "",                    WS_GRENADE,         qtrue,  qfalse, 4,      1,   1,      0,         DELAY_THROW,   1600,        4000,       0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 20, 0 },    0,                 { 0, 0 },       { 0, 0 },      "grenade",         "pineapple",             {1, 1, 1, 1, 1},                     MOD_GRENADE_PINEAPPLE,    MOD_GRENADE_PINEAPPLE    },  // WP_GRENADE_PINEAPPLE     // 9
	//
	{ WP_STEN,                 ITEM_WEAPON_STEN,                  TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_NONE,                WP_MP34,                 WP_NONE,  WP_STEN,              WP_STEN,              14,    200,   0.6f,       0,           0,           WEAPON_TYPE_SMG,                           WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_FALL_OFF,                                           0,      0,     "STEN",                WS_STEN,            qtrue,  qtrue,  96,     1,   32,     3100,      DELAY_LOW,     150,         0,          15,               1200,   450,     2000,            250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "sten",                  {1, 1, 1, 1, 1},                     MOD_STEN,                 MOD_STEN                 },  // WP_STEN                  // 10   // silenced sten sub-machinegun
	{ WP_MEDIC_SYRINGE,        ITEM_WEAPON_MEDIC_SYRINGE,         TEAM_FREE,   SK_FIRST_AID,                                WP_NONE,                WP_NONE,                 WP_NONE,  WP_MEDIC_SYRINGE,     WP_MEDIC_SYRINGE,     1,     0,     0,          0,           0,           WEAPON_TYPE_SYRINGUE,                      WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_FIRE_UNDERWATER,                                                                                                                             0,      0,     "MEDIC",               WS_SYRINGE,         qtrue,  qfalse, 10,     1,   1,      0,         50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "syringe",               {1, 1, 1, 1, 1},                     MOD_SYRINGE,              MOD_SYRINGE              },  // WP_MEDIC_SYRINGE         // 11   // broken out from CLASS_SPECIAL per Id request
	{ WP_AMMO,                 ITEM_WEAPON_MAGICAMMO,             TEAM_FREE,   SK_SIGNALS,                                  WP_NONE,                WP_NONE,                 WP_NONE,  WP_AMMO,              WP_AMMO,              1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE,                                WEAPON_ATTRIBUT_CHARGE_TIME,                                                                                                                                 0,      0,     "AMMO",                WS_MAX,             qfalse, qfalse, 1,      0,   1,      3000,      50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "ammopack",              {0.25f, 0.15f, 0.15f, 0.15f, 0.15f}, MOD_AMMO,                 MOD_AMMO                 },  // WP_AMMO                  // 12   // likewise
	{ WP_ARTY,                 ITEM_WEAPON_ARTY,                  TEAM_FREE,   SK_SIGNALS,                                  WP_NONE,                WP_NONE,                 WP_NONE,  WP_ARTY,              WP_ARTY,              400,   0,     0,          400,         400,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "ARTY",                WS_ARTILLERY,       qfalse, qfalse, 1,      0,   1,      3000,      50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "air strike",      "arty",                  {1, 1, .66f, .66f, .66f},            MOD_ARTY,                 MOD_ARTY                 },  // WP_ARTY                  // 13
	{ WP_SILENCER,             ITEM_WEAPON_SILENCER,              TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_LUGER,               WP_SILENCED_COLT,        WP_NONE,  WP_LUGER,             WP_LUGER,             18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_FALL_OFF,                                                                           0,      0,     "SILENCED LUGER",      WS_LUGER,           qtrue,  qtrue,  24,     1,   8,      1500,      DELAY_PISTOL,  400,         0,          20,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    100,               { .45f, .15f }, { 0, 0 },      "",                "silenced_luger",        {1, 1, 1, 1, 1},                     MOD_SILENCER,             MOD_SILENCER             },  // WP_SILENCER              // 14   // used to be sp5
	{ WP_DYNAMITE,             ITEM_WEAPON_DYNAMITE,              TEAM_FREE,   SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_NONE,                WP_NONE,                 WP_NONE,  WP_DYNAMITE,          WP_DYNAMITE,          0,     0,     0,          400,         400,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE | WEAPON_FIRING_MODE_ONE_SHOT,  WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_SHAKE,                                                                       0,      0,     "DYNAMITE",            WS_DYNAMITE,        qfalse, qfalse, 1,      0,   1,      1000,      DELAY_THROW,   1600,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 20, 0 },    0,                 { 0, 0 },       { 0, 0 },      "dynamite",        "dynamite",              {1, 1, 1, .66f, .66f},               MOD_DYNAMITE,             MOD_DYNAMITE             },  // WP_DYNAMITE              // 15
	{ WP_SMOKETRAIL,           ITEM_WEAPON_SMOKETRAIL,            TEAM_FREE,   SK_SIGNALS,                                  WP_NONE,                WP_NONE,                 WP_NONE,  WP_SMOKETRAIL,        WP_SMOKETRAIL,        0,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "",                    WS_ARTILLERY,       qfalse, qfalse, 999,    0,   999,    0,         50,            0,           0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "WP",              "smoketrail",            {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_SMOKETRAIL            // 16   // WP == White Phosphorous, so we can check for bounce noise in grenade bounce routine
	{ WP_MAPMORTAR,            ITEM_WEAPON_MAPMORTAR,             TEAM_FREE,   SK_HEAVY_WEAPONS,                            WP_NONE,                WP_NONE,                 WP_NONE,  WP_MAPMORTAR,         WP_MAPMORTAR,         250,   0,     0,          250,         120,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_SHAKE,                                                                                                                                       0,      0,     "",                    WS_MORTAR,          qtrue,  qfalse, 999,    0,   999,    0,         50,            0,           0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "mortar",          "mapmortar",             {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_MAPMORTAR             // 17
	{ VERYBIGEXPLOSION,        ITEM_NONE,                         TEAM_FREE,   SK_NUM_SKILLS,                               WP_NONE,                WP_NONE,                 WP_NONE,  WP_NONE,              WP_NONE,              1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "",                    WS_MAX,             qfalse, qfalse, 999,    0,   999,    0,         50,            0,           0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                NULL,                    {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // VERYBIGEXPLOSION         // 18   // explosion effect for airplanes
	{ WP_MEDKIT,               ITEM_WEAPON_MEDIC_HEAL,            TEAM_FREE,   SK_FIRST_AID,                                WP_NONE,                WP_NONE,                 WP_NONE,  WP_MEDKIT,            WP_MEDKIT,            1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE,                                WEAPON_ATTRIBUT_CHARGE_TIME,                                                                                                                                 0,      0,     "",                    WS_MAX,             qfalse, qfalse, 999,    0,   999,    0,         50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "medpack",               {0.25f, 0.25f, 0.15f, 0.15f, 0.15f}, MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_MEDKIT                // 19
	//
	{ WP_BINOCULARS,           ITEM_WEAPON_BINOCULARS,            TEAM_FREE,   SK_NUM_SKILLS,                               WP_NONE,                WP_NONE,                 WP_NONE,  WP_BINOCULARS,        WP_BINOCULARS,        1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE | WEAPON_ATTRIBUT_FIRE_UNDERWATER,                                                       20,     4,     "",                    WS_MAX,             qfalse, qfalse, 999,    0,   999,    0,         50,            0,           0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "binocs",                {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_BINOCULARS            // 20   // per atvi request all use same vals to match menu {36 8}
	{ WP_PLIERS,               ITEM_WEAPON_PLIERS,                TEAM_FREE,   SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_NONE,                WP_NONE,                 WP_NONE,  WP_PLIERS,            WP_PLIERS,            1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_FIRE_UNDERWATER,                                                                                                                             0,      0,     "PLIERS",              WS_MAX,             qfalse, qfalse, 999,    0,   999,    0,         50,            50,          0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "pliers",                {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_PLIERS                // 21
	{ WP_SMOKE_MARKER,         ITEM_WEAPON_SMOKE_MARKER,          TEAM_FREE,   SK_SIGNALS,                                  WP_NONE,                WP_NONE,                 WP_NONE,  WP_SMOKE_MARKER,      WP_SMOKE_MARKER,      0,     0,     0,          140,         140,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE | WEAPON_FIRING_MODE_ONE_SHOT,  WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "",                    WS_AIRSTRIKE,       qfalse, qfalse, 999,    0,   999,    0,         50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "grenade",         "smokemarker",           {1, 1, .66f, .66f, .66f},            MOD_SMOKEGRENADE,         MOD_SMOKEGRENADE         },  // WP_SMOKE_MARKER          // 22
	{ WP_KAR98,                ITEM_WEAPON_KAR98,                 TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_GPG40,               WP_CARBINE,              WP_NONE,  WP_KAR98,             WP_KAR98,             34,    250,   0.5f,       0,           0,           WEAPON_TYPE_RIFLE,                         WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "K43",                 WS_KAR98,           qtrue,  qtrue,  30,     1,   10,     1500,      DELAY_LOW,     400,         0,          50,               0,      0,       0,               250,            250,             2350,             1347,           0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "kar98",                 {1, 1, 1, 1, 1},                     MOD_KAR98,                MOD_KAR98                },  // WP_KAR98                 // 23   // WolfXP weapons    K43
	{ WP_CARBINE,              ITEM_WEAPON_CARBINE,               TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_M7,                  WP_KAR98,                WP_NONE,  WP_CARBINE,           WP_CARBINE,           34,    250,   0.5f,       0,           0,           WEAPON_TYPE_RIFLE,                         WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "M1 GARAND",           WS_CARBINE,         qtrue,  qtrue,  30,     1,   10,     1500,      DELAY_LOW,     400,         0,          50,               0,      0,       0,               250,            250,             2350,             1347,           0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "m1_garand",             {1, 1, 1, 1, 1},                     MOD_CARBINE,              MOD_CARBINE              },  // WP_CARBINE               // 24   // GARAND old max ammo 24 max clip size 8 start ammo 16 start clip 8
	{ WP_GARAND,               ITEM_WEAPON_GARAND,                TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_GARAND_SCOPE,        WP_K43,                  WP_NONE,  WP_GARAND,            WP_GARAND,            34,    250,   0.5f,       0,           0,           WEAPON_TYPE_RIFLE | WEAPON_TYPE_SCOPABLE,  WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE,                                                                                                    0,      0,     "SCOPED M1 GARAND",    WS_GARAND,          qtrue,  qtrue,  30,     1,   10,     1500,      DELAY_LOW,     400,         0,          50,               0,      0,       0,               250,            250,             50,               50,             0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "m1_garand_s",           {1, 1, 1, 1, 1},                     MOD_GARAND,               MOD_GARAND               },  // WP_GARAND                // 25   // GARAND old max ammo 24 max clip size 8 start ammo 16 start clip 8
	{ WP_LANDMINE,             ITEM_WEAPON_LANDMINE,              TEAM_FREE,   SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_NONE,                WP_NONE,                 WP_NONE,  WP_LANDMINE,          WP_LANDMINE,          0,     0,     0,          250,         225,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE | WEAPON_FIRING_MODE_ONE_SHOT,  WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE | WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_SHAKE, 0,      0,     "",                    WS_LANDMINE,        qtrue,  qfalse, 1,      0,   1,      100,       DELAY_LOW,     100,         0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "landmine",        "landmine",              {.5f, .5f, .5f, .33f, .33f},         MOD_LANDMINE,             MOD_LANDMINE             },  // WP_LANDMINE              // 26   // splashRadius was: 400 // for charTime use 33%, not 66%, when upgraded. do not penalize the happy fun engineer.
	{ WP_SATCHEL,              ITEM_WEAPON_SATCHEL,               TEAM_FREE,   SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, WP_NONE,                WP_NONE,                 WP_NONE,  WP_SATCHEL,           WP_SATCHEL,           0,     0,     0,          250,         250,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE | WEAPON_FIRING_MODE_ONE_SHOT,  WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE,                                                           0,      0,     "SATCHEL",             WS_MAX,             qfalse, qfalse, 1,      0,   1,      3000,      DELAY_LOW,     0/*2000*/,   0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 20, 0 },    0,                 { 0, 0 },       { 0, 0 },      "satchel_charge",  "satchel",               {1, 1, .66f, .66f, .66f},            MOD_SATCHEL,              MOD_SATCHEL              },  // WP_SATCHEL               // 27
	{ WP_SATCHEL_DET,          ITEM_WEAPON_SATCHELDET,            TEAM_FREE,   SK_NUM_SKILLS,                               WP_NONE,                WP_NONE,                 WP_NONE,  WP_SATCHEL_DET,       WP_SATCHEL_DET,       1,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_ONE_SHOT,                                 WEAPON_ATTRIBUT_KEEP_DESGUISE,                                                                                                                               0,      0,     "SATCHEL",             WS_SATCHEL,         qfalse, qfalse, 1,      0,   1,      3000,      722,           0/*2000*/,   0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "satchel_det",           {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_SATCHEL_DET           // 28
	{ WP_SMOKE_BOMB,           ITEM_WEAPON_SMOKE_BOMB,            TEAM_FREE,   SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, WP_NONE,                WP_NONE,                 WP_NONE,  WP_SMOKE_BOMB,        WP_SMOKE_BOMB,        0,     0,     0,          0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_THROWABLE | WEAPON_FIRING_MODE_ONE_SHOT,  WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE | WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_SHAKE, 0,      0,     "",                    WS_MAX,             qfalse, qfalse, 1,      0,   10,     1000,      DELAY_THROW,   1600,        4000,       0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 20, 0 },    0,                 { 0, 0 },       { 0, 0 },      "smoke_bomb",      "smokegrenade",          {1, 1, .66f, .66f, .66f},            MOD_SMOKEBOMB,            MOD_SMOKEBOMB            },  // WP_SMOKE_BOMB            // 29
	//
	{ WP_MOBILE_MG42,          ITEM_WEAPON_MOBILE_MG42,           TEAM_AXIS,   SK_HEAVY_WEAPONS,                            WP_MOBILE_MG42_SET,     WP_MOBILE_BROWNING,      WP_NONE,  WP_MOBILE_MG42,       WP_MOBILE_MG42,       18,    2500,  0.9f,       0,           0,           WEAPON_TYPE_MG | WEAPON_TYPE_SETTABLE,     WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "MOBILE MG 42",        WS_MG42,            qtrue,  qtrue,  450,    1,   150,    3000,      DELAY_LOW,     66,          0,          20,               1500,   300,     2000,            250,            250,             1250,             1722,           4000.f,   { 14, 6, -4 },    200,               { .75f, .2f },  { 1.f, .25f }, "",                "mg42",                  {1, 1, 1, 1, 1},                     MOD_MOBILE_MG42,          MOD_MOBILE_MG42          },  // WP_MOBILE_MG42           // 30
	{ WP_K43,                  ITEM_WEAPON_K43,                   TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_K43_SCOPE,           WP_GARAND,               WP_NONE,  WP_K43,               WP_K43,               34,    250,   0.5f,       0,           0,           WEAPON_TYPE_RIFLE | WEAPON_TYPE_SCOPABLE,  WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE,                                                                                                    0,      0,     "SCOPED K43",          WS_K43,             qtrue,  qtrue,  30,     1,   10,     1500,      DELAY_LOW,     400,         0,          50,               0,      0,       0,               250,            250,             50,               50,             0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "k43",                   {1, 1, 1, 1, 1},                     MOD_K43,                  MOD_K43                  },  // WP_K43                   // 31    // K43
	{ WP_FG42,                 ITEM_WEAPON_FG42,                  TEAM_FREE,   SK_LIGHT_WEAPONS,                            WP_FG42_SCOPE,          WP_NONE,                 WP_NONE,  WP_FG42,              WP_FG42,              16,    500,   0.6f,       0,           0,           WEAPON_TYPE_SMG | WEAPON_TYPE_SCOPABLE,    WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                      0,      0,     "FG 42",               WS_FG42,            qtrue,  qtrue,  60,     1,   20,     2000,      DELAY_LOW,     100,         0,          100,              0,      0,       0,               250,            250,             50,               50,             0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "fg42",                  {1, 1, 1, 1, 1},                     MOD_FG42,                 MOD_FG42                 },  // WP_FG42                  // 32
	{ WP_DUMMY_MG42,           ITEM_WEAPON_DUMMY_MG42,            TEAM_FREE,   SK_NUM_SKILLS,                               WP_NONE,                WP_NONE,                 WP_NONE,  WP_DUMMY_MG42,        WP_DUMMY_MG42,        20,    100,   0.9f,       0,           0,           WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "",                    WS_MG42,            qfalse, qfalse, 0,      0,   0,      0,         0,             66,          0,          0,                1500,   300,     2000,            250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                NULL,                    {1, 1, 1, 1, 1},                     MOD_UNKNOWN,              MOD_UNKNOWN              },  // WP_DUMMY_MG42            // 33    // for storing heat on mounted mg42s...
	{ WP_MORTAR,               ITEM_WEAPON_MORTAR,                TEAM_ALLIES, SK_HEAVY_WEAPONS,                            WP_MORTAR_SET,          WP_MORTAR2,              WP_NONE,  WP_MORTAR,            WP_MORTAR,            1,     0,     0,          0,           0,           WEAPON_TYPE_MORTAR | WEAPON_TYPE_SETTABLE, WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "MORTAR",              WS_MORTAR,          qtrue,  qfalse, 15,     1,   1,      0,         DELAY_HW,      1600,        0,          0,                0,      0,       0,               250,            250,             1667,             1000,           0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "mortar",                {1, 1, 1, 1, 1},                     MOD_MORTAR,               MOD_MORTAR               },  // WP_MORTAR                // 34
	{ WP_AKIMBO_COLT,          ITEM_WEAPON_AKIMBO_COLT,           TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_NONE,                WP_AKIMBO_LUGER,         WP_COLT,  WP_COLT,              WP_AKIMBO_COLT,       18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_AKIMBO | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                           0,      0,     "AKIMBO COLTS",        WS_COLT,            qtrue,  qtrue,  48,     1,   8,      2700,      DELAY_PISTOL,  200,         0,          20,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, -6, -4 },   100,               { .45f, .15f }, { 0, 0 },      "",                "akimbo_colt",           {1, 1, 1, 1, 1},                     MOD_AKIMBO_COLT ,         MOD_AKIMBO_COLT          },  // WP_AKIMBO_COLT           // 35
	{ WP_AKIMBO_LUGER,         ITEM_WEAPON_AKIMBO_LUGER,          TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_NONE,                WP_AKIMBO_COLT,          WP_LUGER, WP_LUGER,             WP_AKIMBO_LUGER,      18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_AKIMBO | WEAPON_ATTRIBUT_FALL_OFF,                                                                                                           0,      0,     "AKIMBO LUGERS",       WS_LUGER,           qtrue,  qtrue,  48,     1,   8,      2700,      DELAY_PISTOL,  200,         0,          20,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, -6, -4 },   100,               { .45f, .15f }, { 0, 0 },      "",                "akimbo_luger",          {1, 1, 1, 1, 1},                     MOD_AKIMBO_LUGER,         MOD_AKIMBO_LUGER         },  // WP_AKIMBO_LUGER          // 36
	//
	{ WP_GPG40,                ITEM_WEAPON_GPG40,                 TEAM_AXIS,   SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_KAR98,               WP_NONE,                 WP_NONE,  WP_GPG40,             WP_GPG40,             0,     0,     0,          250,         250,         WEAPON_TYPE_RIFLENADE,                     WEAPON_FIRING_MODE_ONE_SHOT,                                 WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "",                    WS_GRENADELAUNCHER, qtrue,  qfalse, 4,      1,   1,      0,         DELAY_LOW,     400,         0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "gpg40_grenade",   "gpg40",                 {.5f, .5f, .5f, .5f, .5f},           MOD_GPG40,                MOD_GPG40                },  // WP_GPG40                 // 37
	{ WP_M7,                   ITEM_WEAPON_M7,                    TEAM_ALLIES, SK_EXPLOSIVES_AND_CONSTRUCTION,              WP_CARBINE,             WP_NONE,                 WP_NONE,  WP_M7,                WP_M7,                0,     0,     0,          250,         250,         WEAPON_TYPE_RIFLENADE,                     WEAPON_FIRING_MODE_ONE_SHOT,                                 WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "",                    WS_GRENADELAUNCHER, qtrue,  qfalse, 4,      1,   1,      0,         DELAY_LOW,     400,         0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "m7_grenade",      "m7",                    {.5f, .5f, .5f, .5f, .5f},           MOD_M7,                   MOD_M7                   },  // WP_M7                    // 38
	{ WP_SILENCED_COLT,        ITEM_WEAPON_SILENCED_COLT,         TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_COLT,                WP_SILENCER,             WP_NONE,  WP_COLT,              WP_COLT,              18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_FALL_OFF,                                           0,      0,     "SILENCED COLT",       WS_COLT,            qtrue,  qtrue,  24,     1,   8,      1500,      DELAY_PISTOL,  400,         0,          20,               0,      0,       0,               250,            250,             1000,             1190,           0,        { 14, 6, -4 },    100,               { .45f, .15f }, { 0, 0 },      "",                "silenced_colt",         {1, 1, 1, 1, 1},                     MOD_SILENCED_COLT,        MOD_SILENCED_COLT        },  // WP_SILENCED_COLT         // 39
	//
	{ WP_GARAND_SCOPE,         ITEM_WEAPON_GARAND_SCOPE,          TEAM_ALLIES, SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, WP_GARAND,              WP_K43_SCOPE,            WP_NONE,  WP_GARAND,            WP_GARAND,            50,    700,   10.f,       0,           0,           WEAPON_TYPE_RIFLE | WEAPON_TYPE_SCOPED,    WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE,                                                                                                    20,     4,     "",                    WS_GARAND,          qtrue,  qtrue,  30,     1,   10,     1500,      0,             400,         0,          200,              0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    300,               { 0, 0 },       { 1.f, .5f },  "",                "m1_garand_s",           {1, 1, 1, 1, 1},                     MOD_GARAND_SCOPE,         MOD_GARAND_SCOPE         },  // WP_GARAND_SCOPE          // 40    // GARAND  old max ammo 24 max clip size 8 start ammo 16 start clip 8
	{ WP_K43_SCOPE,            ITEM_WEAPON_K43_SCOPE,             TEAM_AXIS,   SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, WP_K43,                 WP_GARAND_SCOPE,         WP_NONE,  WP_K43,               WP_K43,               50,    700,   10.f,       0,           0,           WEAPON_TYPE_RIFLE | WEAPON_TYPE_SCOPED,    WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE,                                                                                                    20,     4,     "",                    WS_K43,             qtrue,  qtrue,  30,     1,   10,     1500,      0,             400,         0,          200,              0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    300,               { 0, 0 },       { 1.f, .5f },  "",                "k43",                   {1, 1, 1, 1, 1},                     MOD_K43_SCOPE,            MOD_K43_SCOPE            },  // WP_K43_SCOPE             // 41    // K43
	{ WP_FG42_SCOPE,           ITEM_WEAPON_FG42_SCOPE,            TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_FG42,                WP_NONE,                 WP_NONE,  WP_FG42,              WP_FG42,              30,    200,   10.f,       0,           0,           WEAPON_TYPE_SMG | WEAPON_TYPE_SCOPED,      WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_NONE,                                                                                                                                        55,     55,    "FG 42",               WS_FG42,            qtrue,  qtrue,  60,     1,   20,     2000,      DELAY_LOW,     400,         0,          100,              0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    100,               { .45f, .15f }, { 0, 0 },      "",                "fg42",                  {1, 1, 1, 1, 1},                     MOD_FG42SCOPE,            MOD_FG42SCOPE            },  // WP_FG42_SCOPE            // 42
	{ WP_MORTAR_SET,           ITEM_WEAPON_MORTAR_SET,            TEAM_ALLIES, SK_HEAVY_WEAPONS,                            WP_MORTAR,              WP_MORTAR2_SET,          WP_NONE,  WP_MORTAR,            WP_MORTAR,            250,   0,     0,          400,         400,         WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET,      WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "MORTAR",              WS_MORTAR,          qtrue,  qfalse, 16,     1,   1,      0,         DELAY_HW,      1400,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "mortar_grenade",  "mortar_set",            {.5f, .33f, .33f, .33f, .33f},       MOD_MORTAR,               MOD_MORTAR               },  // WP_MORTAR_SET            // 43
	{ WP_MEDIC_ADRENALINE,     ITEM_WEAPON_MEDIC_ADRENALINE,      TEAM_FREE,   SK_FIRST_AID,                                WP_NONE,                WP_NONE,                 WP_NONE,  WP_MEDIC_SYRINGE,     WP_MEDIC_SYRINGE,     1,     0,     0,          0,           0,           WEAPON_TYPE_SYRINGUE,                      WEAPON_FIRING_MODE_MANUAL | WEAPON_FIRING_MODE_ONE_SHOT,     WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_KEEP_DESGUISE,                                                               0,      0,     "ADRENALINE",          WS_MAX,             qtrue,  qfalse, 10,     1,   1,      0,         50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "adrenaline",            {1, 1, 1, 1, 1},                     MOD_SYRINGE,              MOD_SYRINGE              },  // WP_MEDIC_ADRENALINE      // 44
	{ WP_AKIMBO_SILENCEDCOLT,  ITEM_WEAPON_AKIMBO_SILENCED_COLT,  TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_NONE,                WP_AKIMBO_SILENCEDLUGER, WP_COLT,  WP_COLT,              WP_AKIMBO_COLT,       18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_AKIMBO | WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_FALL_OFF,                                                0,      0,     "SLNCD AKIMBO COLTS",  WS_COLT,            qtrue,  qtrue,  48,     1,   8,      2700,      DELAY_PISTOL,  200,         0,          20,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, -6, -4 },   100,               { .45f, .15f }, { 0, 0 },      "",                "akimbo_silenced_colt",  {1, 1, 1, 1, 1},                     MOD_AKIMBO_SILENCEDCOLT,  MOD_AKIMBO_SILENCEDCOLT  },  // WP_AKIMBO_SILENCEDCOLT   // 45
	{ WP_AKIMBO_SILENCEDLUGER, ITEM_WEAPON_AKIMBO_SILENCED_LUGER, TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_NONE,                WP_AKIMBO_SILENCEDCOLT,  WP_LUGER, WP_LUGER,             WP_AKIMBO_LUGER,      18,    600,   0.4f,       0,           0,           WEAPON_TYPE_PISTOL,                        WEAPON_FIRING_MODE_SEMI_AUTOMATIC,                           WEAPON_ATTRIBUT_AKIMBO | WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_FALL_OFF,                                                0,      0,     "SLNCD AKIMBO LUGERS", WS_LUGER,           qtrue,  qtrue,  48,     1,   8,      2700,      DELAY_PISTOL,  200,         0,          20,               0,      0,       0,               250,            250,             0,                0,              0,        { 14, -6, -4 },   100,               { .45f, .15f }, { 0, 0 },      "",                "akimbo_silenced_luger", {1, 1, 1, 1, 1},                     MOD_AKIMBO_SILENCEDLUGER, MOD_AKIMBO_SILENCEDLUGER },  // WP_AKIMBO_SILENCEDLUGER  // 46
	{ WP_MOBILE_MG42_SET,      ITEM_WEAPON_MOBILE_MG42_SET,       TEAM_AXIS,   SK_HEAVY_WEAPONS,                            WP_MOBILE_MG42,         WP_MOBILE_BROWNING_SET,  WP_NONE,  WP_MOBILE_MG42,       WP_MOBILE_MG42,       18,    2500,  0.9f,       0,           0,           WEAPON_TYPE_MG | WEAPON_TYPE_SET,          WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_NONE,                                                                                                                                        55,     55,    "MOBILE MG 42",        WS_MG42,            qtrue,  qtrue,  450,    1,   150,    3000,      DELAY_LOW,     66,          0,          20,               1500,   300,     2000,            250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "mg42",                  {1, 1, 1, 1, 1},                     MOD_MOBILE_MG42,          MOD_MOBILE_MG42          },  // WP_MOBILE_MG42_SET       // 47
	// league weapons
	{ WP_KNIFE_KABAR,          ITEM_WEAPON_KNIFE_KABAR,           TEAM_ALLIES, SK_LIGHT_WEAPONS,                            WP_NONE,                WP_KNIFE,                WP_NONE,  WP_KNIFE_KABAR,       WP_KNIFE_KABAR,       10,    0,     0,          0,           0,           WEAPON_TYPE_MELEE,                         WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_FIRE_UNDERWATER | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_NEVER_LOST_DESGUISE,                                                       0,      0,     "KABAR",               WS_KNIFE_KBAR,      qfalse, qfalse, 999,    0,   999,    0,         50,            200,         0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "knife_kbar",            {1, 1, 1, 1, 1},                     MOD_KNIFE_KABAR,          MOD_KNIFE_KABAR          },  // WP_KNIFE_KABAR           // 48
	{ WP_MOBILE_BROWNING,      ITEM_WEAPON_MOBILE_BROWNING,       TEAM_ALLIES, SK_HEAVY_WEAPONS,                            WP_MOBILE_BROWNING_SET, WP_MOBILE_MG42,          WP_NONE,  WP_MOBILE_BROWNING,   WP_MOBILE_BROWNING,   18,    2500,  0.9f,       0,           0,           WEAPON_TYPE_MG | WEAPON_TYPE_SETTABLE,     WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "MOBILE BROWNING",     WS_BROWNING,        qtrue,  qtrue,  450,    1,   150,    3000,      DELAY_LOW,     66,          0,          20,               1500,   300,     2000,            250,            250,             1250,             1722,           4000.f,   { 14, 6, -4 },    200,               { .75f, .2f },  { 1.f, .25f }, "",                "browning",              {1, 1, 1, 1, 1},                     MOD_MOBILE_BROWNING,      MOD_MOBILE_BROWNING      },  // WP_MOBILE_BROWNING       // 49
	{ WP_MOBILE_BROWNING_SET,  ITEM_WEAPON_MOBILE_BROWNING_SET,   TEAM_ALLIES, SK_HEAVY_WEAPONS,                            WP_MOBILE_BROWNING,     WP_MOBILE_MG42_SET,      WP_NONE,  WP_MOBILE_BROWNING,   WP_MOBILE_BROWNING,   18,    2500,  0.9f,       0,           0,           WEAPON_TYPE_MG | WEAPON_TYPE_SET,          WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_NONE,                                                                                                                                        55,     55,    "MOBILE BROWNING",     WS_BROWNING,        qtrue,  qtrue,  450,    1,   150,    3000,      DELAY_LOW,     66,          0,          20,               1500,   300,     2000,            250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "browning",              {1, 1, 1, 1, 1},                     MOD_MOBILE_BROWNING,      MOD_MOBILE_BROWNING      },  // WP_MOBILE_BROWNING_SET   // 50
	{ WP_MORTAR2,              ITEM_WEAPON_MORTAR2,               TEAM_AXIS,   SK_HEAVY_WEAPONS,                            WP_MORTAR2_SET,         WP_MORTAR,               WP_NONE,  WP_MORTAR2,           WP_MORTAR2,           1,     0,     0,          0,           0,           WEAPON_TYPE_MORTAR | WEAPON_TYPE_SETTABLE, WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_NONE,                                                                                                                                        0,      0,     "GRANATWERFER",        WS_MORTAR2,         qtrue,  qfalse, 15,     1,   1,      0,         DELAY_HW,      1600,        0,          0,                0,      0,       0,               250,            250,             1667,             1000,           0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "axis_mortar",           {1, 1, 1, 1, 1},                     MOD_MORTAR2,              MOD_MORTAR2              },  // WP_MORTAR2		   // 51
	{ WP_MORTAR2_SET,          ITEM_WEAPON_MORTAR2_SET,           TEAM_AXIS,   SK_HEAVY_WEAPONS,                            WP_MORTAR2,             WP_MORTAR_SET,           WP_NONE,  WP_MORTAR2,           WP_MORTAR2,           250,   0,     0,          400,         400,         WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET,      WEAPON_FIRING_MODE_MANUAL,                                   WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "GRANATWERFER",        WS_MORTAR2,         qtrue,  qfalse, 16,     1,   1,      0,         DELAY_HW,      1400,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "mortar_grenade",  "axis_mortar_set",       {.5f, .33f, .33f, .33f, .33f},       MOD_MORTAR2,              MOD_MORTAR2              },  // WP_MORTAR2_SET	   // 52
	{ WP_BAZOOKA,              ITEM_WEAPON_BAZOOKA,               TEAM_ALLIES, SK_HEAVY_WEAPONS,                            WP_NONE,                WP_PANZERFAUST,          WP_NONE,  WP_BAZOOKA,           WP_BAZOOKA,           400,   0,     0,          400,         300,         WEAPON_TYPE_PANZER,                        WEAPON_FIRING_MODE_ONE_SHOT,                                 WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "BAZOOKA",             WS_BAZOOKA,         qtrue,  qfalse, 4,      1,   1,      0,         DELAY_HW,      2000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              32000.f,  { 14, 10, 0 },    0,                 { 0, 0 },       { 0, 0 },      "rocket",          "bazooka",               {1, .66f, .66f, .66f, .66f},         MOD_BAZOOKA,              MOD_BAZOOKA              },  // WP_BAZOOKA               // 53
	{ WP_MP34,                 ITEM_WEAPON_MP34,                  TEAM_AXIS,   SK_LIGHT_WEAPONS,                            WP_NONE,                WP_STEN,                 WP_NONE,  WP_MP34,              WP_MP34,              14,    200,   0.6f,       0,           0,           WEAPON_TYPE_SMG,                           WEAPON_FIRING_MODE_AUTOMATIC,                                WEAPON_ATTRIBUT_FAST_RELOAD | WEAPON_ATTRIBUT_SILENCED | WEAPON_ATTRIBUT_KEEP_DESGUISE | WEAPON_ATTRIBUT_FALL_OFF,                                           0,      0,     "MP 34",               WS_MP34,            qtrue,  qtrue,  96,     1,   32,     3100,      DELAY_LOW,     150,         0,          15,               1200,   450,     2000,            250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "",                "mp34",                  {1, 1, 1, 1, 1},                     MOD_MP34,                 MOD_MP34                 },  // WP_MP34                  // 54   // Solothurn S1-100 SMG
	{ WP_AIRSTRIKE,            ITEM_WEAPON_AIRSTRIKE,             TEAM_FREE,   SK_SIGNALS,                                  WP_NONE,                WP_NONE,                 WP_NONE,  WP_AIRSTRIKE,         WP_AIRSTRIKE,         400,   0,     0,          400,         400,         WEAPON_TYPE_NONE,                          WEAPON_FIRING_MODE_NONE,                                     WEAPON_ATTRIBUT_CHARGE_TIME | WEAPON_ATTRIBUT_SHAKE,                                                                                                         0,      0,     "",                    WS_AIRSTRIKE,       qfalse, qfalse, 1,      0,   1,      0,         50,            1000,        0,          0,                0,      0,       0,               250,            250,             0,                0,              0,        { 14, 6, -4 },    0,                 { 0, 0 },       { 0, 0 },      "air strike",      "airstrike",             {1, 1, .66f, .66f, .66f},            MOD_AIRSTRIKE,            MOD_AIRSTRIKE            },  // WP_AIRSTRIKE             // 55   // airstrike shell bomb
};

/**
 * @var modTable
 * @brief New weapon table for mod properties:
 * [0]  = mod                       - mean of death
 * [1]  = weaponIcon                -
 * [2]  = isHeadshot                -
 * [3]  = isExplosive               -
 * [4]  = weaponClassForMOD         -
 * [5]  = noYellMedic               - don't yell medic
 * [6]  = obituaryKillMessage1      -
 * [7]  = obituaryKillMessage2      -
 * [8]  = obituarySelfKillMessage   -
 * [9]  = obituaryNoAttackerMessage -
 * [10] = modName                   - these are just for logging, the client prints its own messages
 * [11] = defaultKillPoints         -
 * [12] = splashKillPoints          -
 * [13] = localizedKillPoints       -
 * [14] = isLocalisedDamage         -
 * [15] = indexWeaponStat           -
 *
 * @note WIP
 * @todo FIXME: add client side properties
 */
modTable_t modTable[MOD_NUM_MODS] =
{
	// mod                                    weaponIcon               isHeadshot isExplosive weaponClassForMOD               noYellMedic  obituaryKillMessage1         obituaryKillMessage2              obituarySelfKillMessage                  obituaryNoAttackerMessage                 modName                                   skillType                                    defaultKillPoint splashKillPoints hitRegionKillPoints   hasHitRegion debugReasonMsg         indexWeaponStat
	{ MOD_UNKNOWN,                            WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was killed by",             "",                               "killed himself",                        NULL,                                     "MOD_UNKNOWN",                            SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_MACHINEGUN,                         WP_MOBILE_MG42,          qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was perforated by",         "'s crew-served MG",              "killed himself",                        NULL,                                     "MOD_MACHINEGUN",                         SK_HEAVY_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "emplaced machinegun", WS_MG42            },
	{ MOD_BROWNING,                           WP_MOBILE_BROWNING,      qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was perforated by",         "'s tank-mounted browning 30cal", "killed himself",                        NULL,                                     "MOD_BROWNING",                           SK_HEAVY_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "emplaced machinegun", WS_BROWNING        },
	{ MOD_MG42,                               WP_MOBILE_MG42,          qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was perforated by",         "'s tank-mounted MG 42",          "killed himself",                        NULL,                                     "MOD_MG42",                               SK_HEAVY_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "emplaced machinegun", WS_MG42            },
	{ MOD_GRENADE,                            WP_NONE,                 qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was blasted by",            "'s explosion",                   "knew how to initiate an explosion",     "has been blasted away by an explosion",  "MOD_GRENADE",                            SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_GRENADE         },

	{ MOD_KNIFE,                              WP_KNIFE,                qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was stabbed by",            "'s knife",                       "killed himself",                        NULL,                                     "MOD_KNIFE",                              SK_LIGHT_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "knife",               WS_KNIFE           },
	{ MOD_LUGER,                              WP_LUGER,                qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Luger 9mm",                   "killed himself",                        NULL,                                     "MOD_LUGER",                              SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_LUGER           },
	{ MOD_COLT,                               WP_COLT,                 qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s .45ACP 1911",                 "killed himself",                        NULL,                                     "MOD_COLT",                               SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_COLT            },
	{ MOD_MP40,                               WP_MP40,                 qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s MP 40",                       "killed himself",                        NULL,                                     "MOD_MP40",                               SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_MP40            },
	{ MOD_THOMPSON,                           WP_THOMPSON,             qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Thompson",                    "killed himself",                        NULL,                                     "MOD_THOMPSON",                           SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_THOMPSON        },
	{ MOD_STEN,                               WP_STEN,                 qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Sten",                        "killed himself",                        NULL,                                     "MOD_STEN",                               SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_STEN            },
	{ MOD_GARAND,                             WP_GARAND,               qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Garand",                      "killed himself",                        NULL,                                     "MOD_GARAND",                             SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_GARAND          },

	{ MOD_SILENCER,                           WP_SILENCER,             qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Luger 9mm",                   "killed himself",                        NULL,                                     "MOD_SILENCER",                           SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_LUGER           },
	{ MOD_FG42,                               WP_FG42,                 qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s FG 42",                       "killed himself",                        NULL,                                     "MOD_FG42",                               SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_FG42            },
	{ MOD_FG42SCOPE,                          WP_FG42_SCOPE,           qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was sniped by",             "'s FG 42",                       "killed himself",                        NULL,                                     "MOD_FG42SCOPE",                          SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_FG42            },
	{ MOD_PANZERFAUST,                        WP_PANZERFAUST,          qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was blasted by",            "'s Panzerfaust",                 "vaporized himself",                     NULL,                                     "MOD_PANZERFAUST",                        SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "rocket launcher",     WS_PANZERFAUST     },
	{ MOD_GRENADE_LAUNCHER,                   WP_GRENADE_LAUNCHER,     qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was exploded by",           "'s grenade",                     "dove on his own grenade",               NULL,                                     "MOD_GRENADE_LAUNCHER",                   SK_LIGHT_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "hand grenade",        WS_GRENADE         },
	{ MOD_FLAMETHROWER,                       WP_FLAMETHROWER,         qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was cooked by",             "'s flamethrower",                "played with fire",                      NULL,                                     "MOD_FLAMETHROWER",                       SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "flamethrower",        WS_FLAMETHROWER    },
	{ MOD_GRENADE_PINEAPPLE,                  WP_GRENADE_PINEAPPLE,    qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was exploded by",           "'s grenade",                     "dove on his own grenade",               NULL,                                     "MOD_GRENADE_PINEAPPLE",                  SK_LIGHT_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "hand grenade",        WS_GRENADE         },

	{ MOD_MAPMORTAR,                          WP_NONE,                 qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was killed by",             "",                               "killed himself",                        "had an appointment with the map mortar", "MOD_MAPMORTAR",                          SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "mortar",              WS_MORTAR          },
	{ MOD_MAPMORTAR_SPLASH,                   WP_NONE,                 qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was killed by",             "",                               "killed himself",                        "took a map mortar shell shower",         "MOD_MAPMORTAR_SPLASH",                   SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "mortar",              WS_MORTAR          },

	{ MOD_KICKED,                             WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        NULL,                                     "MOD_KICKED",                             SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },

	{ MOD_DYNAMITE,                           WP_DYNAMITE,             qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_DYNAMITE,  qfalse,      "was blasted by",            "'s dynamite",                    "dynamited himself to pieces",           NULL,                                     "MOD_DYNAMITE",                           SK_EXPLOSIVES_AND_CONSTRUCTION,              3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "dynamite",            WS_DYNAMITE        },
	{ MOD_AIRSTRIKE,                          WP_SMOKE_MARKER,         qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was blasted by",            "'s support fire",                "obliterated himself",                   NULL,                                     "MOD_AIRSTRIKE",                          SK_SIGNALS,                                  3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "airstrike",           WS_AIRSTRIKE       },
	{ MOD_SYRINGE,                            WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        NULL,                                     "MOD_SYRINGE",                            SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_SYRINGE         },
	{ MOD_AMMO,                               WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        NULL,                                     "MOD_AMMO",                               SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_ARTY,                               WP_BINOCULARS,           qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was shelled by",            "'s artillery support",           "fired-for-effect on himself",           NULL,                                     "MOD_ARTY",                               SK_SIGNALS,                                  4.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "artillery",           WS_ARTILLERY       },

	{ MOD_WATER,                              WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        "drowned",                                "MOD_WATER",                              SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_SLIME,                              WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        "slagged",                                "MOD_SLIME",                              SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_LAVA,                               WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        "was incinerated",                        "MOD_LAVA",                               SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_CRUSH,                              WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was killed by",             "",                               "killed himself",                        "was crushed",                            "MOD_CRUSH",                              SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_TELEFRAG,                           WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was killed by",             "",                               "killed himself",                        "was killed",                             "MOD_TELEFRAG",                           SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_FALLING,                            WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        "rediscovered gravity",                   "MOD_FALLING",                            SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_SUICIDE,                            WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was killed by",             "",                               "committed suicide",                     NULL,                                     "MOD_SUICIDE",                            SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_TARGET_LASER,                       WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        "was killed",                             "MOD_TARGET_LASER",                       SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_TRIGGER_HURT,                       WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        "was mortally wounded",                   "MOD_TRIGGER_HURT",                       SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_EXPLOSIVE,                          WP_NONE,                 qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was killed by",             "",                               "died in his own explosion",             "was pulverized by an explosion",         "MOD_EXPLOSIVE",                          SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },

	{ MOD_CARBINE,                            WP_CARBINE,              qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Garand",                      "killed himself",                        NULL,                                     "MOD_CARBINE",                            SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_CARBINE         },
	{ MOD_KAR98,                              WP_KAR98,                qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s K43",                         "killed himself",                        NULL,                                     "MOD_KAR98",                              SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_KAR98           },
	{ MOD_GPG40,                              WP_GPG40,                qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was killed by",             "'s rifle grenade",               "ate his own rifle grenade",             NULL,                                     "MOD_GPG40",                              SK_EXPLOSIVES_AND_CONSTRUCTION,              3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "rifle grenade",       WS_GRENADELAUNCHER },
	{ MOD_M7,                                 WP_M7,                   qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was killed by",             "'s rifle grenade",               "ate his own rifle grenade",             NULL,                                     "MOD_M7",                                 SK_EXPLOSIVES_AND_CONSTRUCTION,              3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "rifle grenade",       WS_GRENADELAUNCHER },
	{ MOD_LANDMINE,                           WP_LANDMINE,             qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "failed to spot",            "'s Landmine",                    "failed to spot his own landmine",       NULL,                                     "MOD_LANDMINE",                           SK_EXPLOSIVES_AND_CONSTRUCTION,              4.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "landmine",            WS_LANDMINE        },
	{ MOD_SATCHEL,                            WP_SATCHEL,              qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_SATCHEL,   qfalse,      "was blasted by",            "'s Satchel Charge",              "embraced his own satchel explosion",    NULL,                                     "MOD_SATCHEL",                            SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 4.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "satchel charge",      WS_SATCHEL         },

	{ MOD_SMOKEBOMB,                          WP_SMOKE_BOMB,           qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "",                               "killed himself",                        NULL,                                     "MOD_SMOKEBOMB",                          SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_MOBILE_MG42,                        WP_MOBILE_MG42,          qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was mown down by",          "'s Mobile MG 42",                "killed himself",                        NULL,                                     "MOD_MOBILE_MG42",                        SK_HEAVY_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "mobile machinegun",   WS_MG42            },
	{ MOD_SILENCED_COLT,                      WP_SILENCED_COLT,        qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s .45ACP 1911",                 "killed himself",                        NULL,                                     "MOD_SILENCED_COLT",                      SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_COLT            },
	{ MOD_GARAND_SCOPE,                       WP_GARAND_SCOPE,         qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was silenced by",           "'s Garand",                      "killed himself",                        NULL,                                     "MOD_GARAND_SCOPE",                       SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_GARAND          },

	{ MOD_CRUSH_CONSTRUCTION,                 WP_PLIERS,               qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "got caught in",             "'s construction madness",        "engineered himself into oblivion",      NULL,                                     "MOD_CRUSH_CONSTRUCTION",                 SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_CRUSH_CONSTRUCTIONDEATH,            WP_PLIERS,               qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "got burried under",         "'s rubble",                      "buried himself alive",                  NULL,                                     "MOD_CRUSH_CONSTRUCTIONDEATH",            SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },
	{ MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER, WP_PLIERS,               qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was killed by",             "",                               "killed himself",                        "got buried under a pile of rubble",      "MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER", SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },

	{ MOD_K43,                                WP_K43,                  qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s K43",                         "killed himself",                        NULL,                                     "MOD_K43",                                SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_K43             },
	{ MOD_K43_SCOPE,                          WP_K43_SCOPE,            qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was silenced by",           "'s K43",                         "killed himself",                        NULL,                                     "MOD_K43_SCOPE",                          SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS, 3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_K43             },

	{ MOD_MORTAR,                             WP_MORTAR,               qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "never saw",                 "'s mortar round coming",         "never saw his own mortar round coming", NULL,                                     "MOD_MORTAR",                             SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "mortar",              WS_MORTAR          },

	{ MOD_AKIMBO_COLT,                        WP_AKIMBO_COLT,          qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Akimbo .45ACP 1911s",         "killed himself",                        NULL,                                     "MOD_AKIMBO_COLT",                        SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_COLT            },
	{ MOD_AKIMBO_LUGER,                       WP_AKIMBO_LUGER,         qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Akimbo Luger 9mms",           "killed himself",                        NULL,                                     "MOD_AKIMBO_LUGER",                       SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_LUGER           },
	{ MOD_AKIMBO_SILENCEDCOLT,                WP_AKIMBO_SILENCEDCOLT,  qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Akimbo .45ACP 1911s",         "killed himself",                        NULL,                                     "MOD_AKIMBO_SILENCEDCOLT",                SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_COLT            },
	{ MOD_AKIMBO_SILENCEDLUGER,               WP_AKIMBO_SILENCEDLUGER, qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s Akimbo Luger 9mms",           "killed himself",                        NULL,                                     "MOD_AKIMBO_SILENCEDLUGER",               SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_LUGER           },

	{ MOD_SMOKEGRENADE,                       WP_SMOKE_MARKER,         qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "stood on",                  "'s airstrike marker",            "danced on his airstrike marker",        NULL,                                     "MOD_SMOKEGRENADE",                       SK_SIGNALS,                                  3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "hand grenade",        WS_AIRSTRIKE       },

	{ MOD_SWAP_PLACES,                        WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "^2swapped places with^7",   "",                               "killed himself",                        NULL,                                     "MOD_SWAP_PLACES",                        SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },

	{ MOD_SWITCHTEAM,                         WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was killed by",             "",                               "killed himself",                        NULL,                                     "MOD_SWITCHTEAM",                         SK_NUM_SKILLS,                               0.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "",                    WS_MAX             },

	{ MOD_SHOVE,                              WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was thrown to his doom by", "",                               "killed himself",                        NULL,                                     "MOD_SHOVE",                              SK_BATTLE_SENSE,                             5.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "shove",               WS_MAX             },

	{ MOD_KNIFE_KABAR,                        WP_KNIFE_KABAR,          qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was stabbed by",            "",                               "killed himself",                        NULL,                                     "MOD_KNIFE_KABAR",                        SK_LIGHT_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "knife",               WS_KNIFE_KBAR      },
	{ MOD_MOBILE_BROWNING,                    WP_MOBILE_BROWNING,      qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was mown down by",          "'s Mobile Browning",             "killed himself",                        NULL,                                     "MOD_MOBILE_BROWNING",                    SK_HEAVY_WEAPONS,                            3.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "mobile machinegun",   WS_BROWNING        },
	{ MOD_MORTAR2,                            WP_MORTAR2,              qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "never saw",                 "'s mortar round coming",         "never saw his own mortar round coming", NULL,                                     "MOD_MORTAR2",                            SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "mortar",              WS_MORTAR2         },
	{ MOD_BAZOOKA,                            WP_BAZOOKA,              qfalse,    qtrue,      WEAPON_CLASS_FOR_MOD_EXPLOSIVE, qfalse,      "was blasted by",            "'s Bazooka",                     "vaporized himself",                     NULL,                                     "MOD_BAZOOKA",                            SK_HEAVY_WEAPONS,                            3.f,             3.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "rocket launcher",     WS_BAZOOKA         },
	{ MOD_BACKSTAB,                           WP_NONE,                 qfalse,    qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qtrue,       "was backstabbed by",        "'s knife arts",                  "killed himself",                        NULL,                                     "MOD_BACKSTAB",                           SK_LIGHT_WEAPONS,                            5.f,             0.f,             {0.f, 0.f, 0.f, 0.f}, qfalse,      "backstab",            WS_KNIFE           },
	{ MOD_MP34,                               WP_MP34,                 qtrue,     qfalse,     WEAPON_CLASS_FOR_MOD_NO,        qfalse,      "was killed by",             "'s MP 34",                       "killed himself",                        NULL,                                     "MOD_MP34",                               SK_LIGHT_WEAPONS,                            3.f,             0.f,             {5.f, 3.f, 3.f, 3.f}, qtrue,       "",                    WS_MP34            },
};

// *INDENT-ON*

/*
 * @var animStrings
 * @brief text representation for scripting
 * @note unused
 *
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
*/

/**
 * @var bg_itemlist
 * @brief QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPENDED SPIN PERSISTANT
 * DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
 * SUSPENDED - will allow items to hang in the air, otherwise they are dropped to the next surface.
 * SPIN - will allow items to spin in place.
 * PERSISTANT - some items (ex. clipboards) can be picked up, but don't disappear
 *
 * If an item is the target of another entity, it will not spawn in until fired.
 *
 * An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.
 *
 * "notfree" if set to 1, don't spawn in free for all games
 * "notteam" if set to 1, don't spawn in team games
 * "notsingle" if set to 1, don't spawn in single player games
 * "wait"  override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
 * "random" random number of plus or minus seconds varied from the respawn time
 * "count" override quantity or duration on most items.
 * "stand" if the item has a stand (ex: mp40_stand.md3) this specifies which stand tag to attach the weapon to ("stand":"4" would mean "tag_stand4" for example)  only weapons support stands currently
 *
 * @note Important notes:
 * - bg_itemlist has additional client members
 */
gitem_t bg_itemlist[] =
{
	{
		ITEM_NONE,
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
		WP_NONE,                // giWeapon
		PW_NONE,                // giPowerUp
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},  ///< leave index 0 alone

	/** QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_s.md3"
	*/
	{
		ITEM_HEALTH_SMALL,
		"item_health_small",
		"sound/misc/health_pickup.wav", // was "sound/items/n_health.wav"
		{
			0, // we can't load what's not in - "models/powerups/health/health_s.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Small Health Pack",
		5,
		IT_HEALTH,
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_m.md3"
	*/
	{
		ITEM_HEALTH,
		"item_health",
		"sound/misc/health_pickup.wav", //      "sound/multiplayer/health_pickup.wav",
		{
			"models/multiplayer/medpack/medpack_pickup.md3", // was   "models/powerups/health/health_m.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Health Pack",
		20,
		IT_HEALTH,
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_m.md3"
	*/
	{
		ITEM_HEALTH_LARGE,
		"item_health_large",
		"sound/misc/health_pickup.wav", //      "sound/multiplayer/health_pickup.wav",
		{
			"models/multiplayer/medpack/medpack_pickup.md3", //  was "models/powerups/health/health_m.md3",
			0,
			0
		},
		NULL,
		NULL,   // ammo icon
		"Mega Health Pack",
		50,             // increased to 50 from 30 and used it for SP.
		IT_HEALTH,
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},

	{
		ITEM_HEALTH_CABINET,
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
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED item_health_turkey (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	multi-stage health item.
	gives 40 on first use, then gives 20 on "finishing up"

	player will only eat what he needs.  health at 90, turkey fills up and leaves remains (leaving 15).  health at 5 you eat the whole thing.
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_t1.md3"
	*/
	{
		ITEM_HEALTH_TURKEY,
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
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED item_health_breadandmeat (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	multi-stage health item.
	gives 30 on first use, then gives 15 on "finishing up"
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_b1.md3"
	*/
	{
		ITEM_HEALTH_BREADANDMEAT,
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
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED item_health_wall (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	defaults to 50 pts health
	you will probably want to check the 'suspended' box to keep it from falling to the ground
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/health/health_w.md3"
	*/
	{
		ITEM_HEALTH_WALL,
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
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// STAMINA

	// WEAPONS - wolf weapons

	/** QUAKED weapon_knife (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/knife/knife.md3"
	*/
	{
		ITEM_WEAPON_KNIFE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_KNIFE_KABAR,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_luger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/luger/luger.md3"
	*/
	{
		ITEM_WEAPON_LUGER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_akimboluger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_luger/luger.md3"
	*/
	{
		ITEM_WEAPON_AKIMBO_LUGER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_akimbosilencedluger (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_luger/luger.md3"
	*/
	{
		ITEM_WEAPON_AKIMBO_SILENCED_LUGER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_thompson (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/thompson/thompson.md3"
	*/
	{
		ITEM_WEAPON_THOMPSON,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_DUMMY_MG42,
		"weapon_dummy",
		"",
		{
			0,
			0,
			0
		},
		"",                     // icon
		"",                     // ammo icon
		"BLANK",                // pickup
		0,                      // quantity
		IT_WEAPON,              // item type
		WP_DUMMY_MG42,          // giTag
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_sten (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/sten/sten.md3"
	*/
	{
		ITEM_WEAPON_STEN,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},

	/**
	weapon_mp34
	*/
	{
		ITEM_WEAPON_MP34,
		"weapon_mp34",
		"sound/misc/w_pkup.wav",
		{
			"models/weapons2/mp34/mp34_3rd.md3",
			"models/weapons2/mp34/v_mp34.md3",
			0
		},
		"icons/iconw_sten_1",    // icon // FIXME?
		"icons/ammo2",           // ammo icon
		"MP34",                 // pickup
		30,
		IT_WEAPON,
		WP_MP34,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},

	/** QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/colt/colt.md3"
	*/
	{
		ITEM_WEAPON_COLT,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_akimbocolt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_colt/colt.md3"
	*/
	{
		ITEM_WEAPON_AKIMBO_COLT,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_akimbosilencedcolt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/akimbo_colt/colt.md3"
	*/
	{
		ITEM_WEAPON_AKIMBO_SILENCED_COLT,
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
		IT_WEAPON, WP_AKIMBO_SILENCEDCOLT,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_mp40 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	"stand" values:
	    no value:   laying in a default position on it's side (default)
	    2:          upright, barrel pointing up, slightly angled (rack mount)
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models\weapons2\mp40\mp40.md3"
	*/
	{
		ITEM_WEAPON_MP40,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/panzerfaust/pf.md3"
	*/
	{
		ITEM_WEAPON_PANZERFAUST,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_bazooka (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/bazooka/bazooka.md3"
	*/
	{
		ITEM_WEAPON_BAZOOKA,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// removed the quaked for this.  we don't actually have a grenade launcher as such.  It's given implicitly
	//         by virtue of getting grenade ammo.  So we don't need to have them in maps

	// weapon_grenadelauncher
	{
		ITEM_WEAPON_GRENADE_LAUNCHER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_grenadePineapple
	{
		ITEM_WEAPON_GRENADE_PINEAPPLE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_grenadesmoke
	{
		ITEM_WEAPON_SMOKE_MARKER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_smoketrail -- only used as a special effects emitter for smoke trails (artillery spotter etc)
	{
		ITEM_WEAPON_SMOKETRAIL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_medic_heal
	{
		ITEM_WEAPON_MEDIC_HEAL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_dynamite
	{
		ITEM_WEAPON_DYNAMITE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/flamethrower/flamethrower.md3"
	*/
	{
		ITEM_WEAPON_FLAMETHROWER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_MAPMORTAR,
		"weapon_mapmortar",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_class_special (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_PLIERS,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_arty (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_ARTY,
		"weapon_arty",
		"",
		{
			0,
			0,
			0
		},
		"",                      // icon
		"",                      // ammo icon
		"Artillery",             // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_ARTY,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_AIRSTRIKE,
		"weapon_airstrike",
		"",
		{
			0,
			0,
			0
		},
		"",                     // icon
		"",                     // ammo icon
		"Airstrike",            // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_AIRSTRIKE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_medic_syringe (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_MEDIC_SYRINGE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_medic_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_MEDIC_ADRENALINE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_magicammo (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_MAGICAMMO,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MAGICAMMO2,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_binoculars (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_BINOCULARS,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_k43 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model=""
	*/
	{
		ITEM_WEAPON_K43,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_kar43_scope (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model=""
	*/
	{
		ITEM_WEAPON_K43_SCOPE,
		"weapon_kar43_scope",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_kar98Rifle (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		ITEM_WEAPON_KAR98,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_gpg40 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		ITEM_WEAPON_GPG40,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_gpg40_allied (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		ITEM_WEAPON_M7,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_M1CarbineRifle (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/mauser/mauser.md3"
	*/
	{
		ITEM_WEAPON_CARBINE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_garandRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/garand/garand.md3"
	*/
	{
		ITEM_WEAPON_GARAND,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_garandRifleScope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/garand/garand.md3"
	*/
	{
		ITEM_WEAPON_GARAND_SCOPE,
		"weapon_garandRifleScope",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_fg42 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/fg42/fg42.md3"
	*/
	{
		ITEM_WEAPON_FG42,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_fg42scope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/fg42/fg42.md3"
	*/
	{
		ITEM_WEAPON_FG42_SCOPE,
		"weapon_fg42scope",
		"",
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
		WP_FG42_SCOPE,   // this weap
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/*
	weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/bla?bla?/bla!.md3"
	*/
	{
		ITEM_WEAPON_MORTAR,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MORTAR_SET,
		"weapon_mortar_set",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MORTAR2,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MORTAR2_SET,
		"weapon_mortar2_set",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_landmine
	{
		ITEM_WEAPON_LANDMINE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// weapon_satchel (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	{
		ITEM_WEAPON_SATCHEL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_SATCHELDET,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_SMOKE_BOMB,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_mobile_mg42 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended spin - respawn
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/multiplayer/mg42/v_mg42.md3"
	*/
	{
		ITEM_WEAPON_MOBILE_MG42,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MOBILE_MG42_SET,
		"weapon_mobile_mg42_set",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MOBILE_BROWNING_SET,
		"weapon_mobile_browning_set",
		"",
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_MOBILE_BROWNING,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	{
		ITEM_WEAPON_SILENCER,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/weapons2/colt/colt.md3"
	*/
	{
		ITEM_WEAPON_SILENCED_COLT,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_syringe (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: medic
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/syringe/syringe.md3
	*/
	{
		ITEM_AMMO_SYRINGE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_smoke_grenade (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: engineer
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/smoke_grenade/smoke_grenade.md3"
	*/
	{
		ITEM_AMMO_SMOKE_GRENADE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_dynamite (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: engineer
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/dynamite/dynamite.md3"
	*/
	{
		ITEM_AMMO_DYNAMITE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_disguise (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: covertops
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/disguise/disguise.md3"
	*/
	{
		ITEM_AMMO_DISGUISE,
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
		WP_NONE,
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_airstrike (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: LT
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/airstrike/airstrike.md3"
	*/
	{
		ITEM_AMMO_AIRSTRIKE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_landmine (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: LT
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/landmine/landmine.md3"
	*/
	{
		ITEM_AMMO_LANDMINE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_satchel_charge (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: LT
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/ammo/satchel/satchel.md3"
	*/
	{
		ITEM_AMMO_SATCHEL_CHARGE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// AMMO ITEMS

	/** QUAKED ammo_9mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Luger pistol, MP40 machinegun
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am9mm_s.md3"
	*/
	{
		ITEM_AMMO_9MM_SMALL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_9mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Luger pistol, MP40 machinegun
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am9mm_m.md3"
	*/
	{
		ITEM_AMMO_9MM,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_9mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Luger pistol, MP40 machinegun
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am9mm_l.md3"
	*/
	{
		ITEM_AMMO_9MM_LARGE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_45cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Thompson, Colt
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am45cal_s.md3"
	*/
	{
		ITEM_AMMO_45CAL_SMALL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_45cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Thompson, Colt
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am45cal_m.md3"
	*/
	{
		ITEM_AMMO_45CAL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_45cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Thompson, Colt
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am45cal_l.md3"
	*/
	{
		ITEM_AMMO_45CAL_LARGE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_30cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Garand rifle
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am30cal_s.md3"
	*/
	{
		ITEM_AMMO_30CAL_SMALL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_30cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Garand rifle
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am30cal_m.md3"
	*/
	{
		ITEM_AMMO_30CAL,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED ammo_30cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
	used by: Garand rifle
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/powerups/ammo/am30cal_l.md3"
	*/
	{
		ITEM_AMMO_30CAL_LARGE,
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
		PW_NONE,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	// POWERUP ITEMS

	/** QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
	Only in CTF games
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/flags/r_flag.md3"
	*/
	{
		ITEM_RED_FLAG,
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
		WP_NONE,
		PW_REDFLAG,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/** QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
	Only in CTF games
	-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
	model="models/flags/b_flag.md3"
	*/
	{
		ITEM_BLUE_FLAG,
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
		WP_NONE,
		PW_BLUEFLAG,
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	},
	/// end of list marker
	{
		ITEM_MAX_ITEMS,
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
		WP_NONE,                // giWeapon
		PW_NONE,                // goPowerUp
#ifdef CGAMEDLL
		{ 0, { 0 }, { 0 } },
#endif
	}
};

/**
 * @brief BG_AkimboFireSequence
 * @param[in] weapon
 * @param[in] akimboClip
 * @param[in] mainClip
 * @return 'true' if it's the left hand's turn to fire, 'false' if it's the right hand's turn
 */
qboolean BG_AkimboFireSequence(int weapon, int akimboClip, int mainClip)
{
	if (!(GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO))
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
 * @brief Setting numOfClips = 0 allows you to check if the client needs ammo, but doesnt give any
 * @details if numOfClips is 0, no ammo is added, it just return whether any ammo CAN be added
 * otherwise return whether any ammo was ACTUALLY added.
 * WARNING: when numOfClips is 0, DO NOT CHANGE ANYTHING under ps.
 * @param[in,out] ps
 * @param[in] skill
 * @param[in] teamNum
 * @param[in] numOfClips
 * @return
 */
qboolean BG_AddMagicAmmo(playerState_t *ps, int *skill, team_t teamNum, int numOfClips)
{
	qboolean ammoAdded = qfalse;
	weapon_t weapon;

	for (weapon = WP_NONE; weapon < WP_NUM_WEAPONS; weapon++)
	{
		if (GetWeaponTableData(weapon)->useAmmo)
		{
			int      maxAmmo;
			weapon_t clip;

#ifdef GAMEDLL
			if (team_riflegrenades.integer == 0)
			{
				switch (weapon)
				{
				case WP_GPG40:
				case WP_M7:
					continue;
				default:
					break;
				}
			}
#endif

			// special case for grenades, they must be linked to the correct team
			// and they may don't appear in weapons bit-wise (see PM_SwitchIfEmpty and Add_Ammo)
			if (GetWeaponTableData(weapon)->type & WEAPON_TYPE_GRENADE)
			{
				if (weapon != GetPlayerClassesData(teamNum, ps->stats[STAT_PLAYER_CLASS])->classGrenadeWeapon.weapon)
				{
					continue;
				}

				// add grenade only if we ask for
				if (numOfClips)
				{
					COM_BitSet(ps->weapons, weapon);
				}
			}
			else if (!COM_BitCheck(ps->weapons, weapon))
			{
				continue;
			}

			maxAmmo = BG_MaxAmmoForWeapon(weapon, skill, ps->stats[STAT_PLAYER_CLASS]);
			clip    = GetWeaponTableData(weapon)->ammoIndex;;

			// count less the current ammo used in weapon for non-clip weapon
			if (!GetWeaponTableData(weapon)->useClip)
			{
				maxAmmo -= ps->ammoclip[clip];
			}

			if (ps->ammo[clip] < maxAmmo)
			{
				int weapNumOfClips;

				// early out
				if (!numOfClips)
				{
					return qtrue;
				}
				ammoAdded = qtrue;

				if (GetWeaponTableData(weapon)->attributes & WEAPON_ATTRIBUT_AKIMBO)
				{
					weapNumOfClips = numOfClips * 2;         // double clips babeh!
				}
				else
				{
					weapNumOfClips = numOfClips;
				}

				// add and limit check
				ps->ammo[clip] += weapNumOfClips * GetWeaponTableData(weapon)->maxClip;
				if (ps->ammo[clip] > maxAmmo)
				{
					ps->ammo[clip] = maxAmmo;
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
qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, int *skill, team_t teamNum)
{
	gitem_t *item;

	if (ent->modelindex < 1 || ent->modelindex >= ITEM_MAX_ITEMS)
	{
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

	item = BG_GetItem(ent->modelindex);

	switch (item->giType)
	{
	case IT_WEAPON:
		if (item->giWeapon == WP_AMMO)
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
	case IT_HEALTH:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return qfalse;
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
			if (item->giPowerUp == PW_BLUEFLAG ||
			    (item->giPowerUp == PW_REDFLAG && ent->otherEntityNum2 /*ent->modelindex2*/) ||
			    (item->giPowerUp == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]))
			{
				return qtrue;
			}
		}
		else if (ps->persistant[PERS_TEAM] == TEAM_ALLIES)
		{
			if (item->giPowerUp == PW_REDFLAG ||
			    (item->giPowerUp == PW_BLUEFLAG && ent->otherEntityNum2 /*ent->modelindex2*/) ||
			    (item->giPowerUp == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]))
			{
				return qtrue;
			}
		}

		return qfalse;

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
		phase     = sin(deltaTime * M_TAU_F);
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
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime);
	}
}

/**
 * @brief For determining velocity at a given time
 * @param[in] tr
 * @param[in] atTime
 * @param[out] result
 * @param[in] isAngle - unused
 * @param[in] splineData - unused
 */
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splineData)
{
	float deltaTime;
	float phase;

	switch (tr->trType)
	{
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED:
		VectorClear(result);
		break;
	case TR_LINEAR:
		VectorCopy(tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
		phase     = cos(deltaTime *  M_TAU_F);     // derivative of sin = cos
		phase    *= M_TAU_F * 2 * 1000 / (float)tr->trDuration;
		VectorScale(tr->trDelta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if (atTime > tr->trTime + tr->trDuration || atTime < tr->trTime)
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
		// phase     = deltaTime / (float)tr->trDuration;   // TODO: phase is never read
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
	"unused event",              // EV_POWERUP_QUAD
	"unused event",              // EV_POWERUP_BATTLESUIT
	"unused event",              // EV_POWERUP_REGEN
	"EV_GIB_PLAYER",
	"unused event",              // EV_DEBUG_LINE,
	"unused event",              // EV_STOPLOOPINGSOUND
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
	"EV_FLAG_INDICATOR",
	"EV_MISSILE_FALLING",
	"EV_PLAYER_HIT"
	//"EV_MAX_EVENTS",
};

#ifdef ETLEGACY_DEBUG
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
#ifdef ETLEGACY_DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if (atof(buf) != 0.0)
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
	else if (ps->eventSequence == 0)
	{
		s->eventSequence = 0;
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

// *INDENT-OFF*
/**
 * @var rankTable
 * @brief New rank table
 * [0]  = names      - rank name
 * [1]  = miniNames  - mini rank name
 * [2]  = soundNames - sound to play on rank promotion
 */
ranktable_t rankTable[2][NUM_EXPERIENCE_LEVELS] =
{
	// Axis
	{
		// names            miniNames soundNames
		{"Schutze",         "Stz",    "",                                 },
		{"Oberschutze",     "Otz",    "axis_hq_promo_private",            },
		{"Gefreiter",       "Gfr",    "axis_hq_promo_corporal",           },
		{"Feldwebel",       "Fwb",    "axis_hq_promo_sergeant",           },
		{"Leutnant",        "Ltn",    "axis_hq_promo_lieutenant",         },
		{"Hauptmann",       "Hpt",    "axis_hq_promo_captain",            },
		{"Major",           "Mjr",    "axis_hq_promo_major",              },
		{"Oberst",          "Obs",    "axis_hq_promo_colonel",            },
		{"Generalmajor",    "GMj",    "axis_hq_promo_general_major",      },
		{"Generalleutnant", "GLt",    "axis_hq_promo_general_lieutenant", },
		{"General",         "Gen",    "axis_hq_promo_general",            },
	},
	// Allies
	{
		// names               miniNames soundNames
		{"Private",            "Pvt",    "",                                   },
		{"Private 1st Class",  "PFC",    "allies_hq_promo_private",            },
		{"Corporal",           "Cpl",    "allies_hq_promo_corporal",           },
		{"Sergeant",           "Sgt",    "allies_hq_promo_sergeant",           },
		{"Lieutenant",         "Lt",     "allies_hq_promo_lieutenant",         },
		{"Captain",            "Cpt",    "allies_hq_promo_captain",            },
		{"Major",              "Maj",    "allies_hq_promo_major",              },
		{"Colonel",            "Cnl",    "allies_hq_promo_colonel",            },
		{"Brigadier General",  "BGn",    "allies_hq_promo_general_brigadier",  },
		{"Lieutenant General", "LtG",    "allies_hq_promo_general_lieutenant", },
		{"General",            "Gen",    "allies_hq_promo_general",            },
	}
};
// *INDENT-ON*

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
	Q_strncpyz(pathCorners[numPathCorners].name, name, MAX_QPATH);
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

	Com_Memset(spline, 0, sizeof(splinePath_t));

	VectorCopy(origin, spline->point.origin);

	Q_strncpyz(spline->point.name, name, MAX_QPATH);
	Q_strncpyz(spline->strTarget, target ? target : "", MAX_QPATH);

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

	Q_strncpyz(spline->controls[spline->numControls].name, name, MAX_QPATH);

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
 * @param[in] class
 * @return
 */
int BG_MaxAmmoForWeapon(weapon_t weaponNum, const int *skill, int cls)
{
	int maxAmmo = GetWeaponTableData(weaponNum)->maxAmmo;

	if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_PISTOL)
	{
		if (skill[SK_LIGHT_WEAPONS] >= 1)
		{
			maxAmmo += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_SMG)
	{
		if (skill[SK_LIGHT_WEAPONS] >= 1 || (cls == PC_MEDIC && skill[SK_FIRST_AID] >= 1))
		{
			maxAmmo += GetWeaponTableData(weaponNum)->maxClip;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLENADE)
	{
		if (skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
		{
			maxAmmo += 4;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_GRENADE)
	{
		// FIXME: this is class dependant, not ammo table
		maxAmmo = BG_GetPlayerClassInfo(GetWeaponTableData(weaponNum)->team, cls)->classGrenadeWeapon.startingAmmo;

		if (cls == PC_ENGINEER && skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
		{
			maxAmmo += 4;
		}
		else if (cls == PC_MEDIC && skill[SK_FIRST_AID] >= 1)
		{
			maxAmmo += 1;
		}
		else if (cls == PC_FIELDOPS && skill[SK_SIGNALS] >= 1)
		{
			maxAmmo += 1;
		}
	}
	// else if (GetWeaponTableData(weaponNum)->isPanzer || GetWeaponTableData(weaponNum)->isMG || GetWeaponTableData(weaponNum)->isMGSet || weaponNum == WP_FLAMETHROWER)
	// {
	//     if( skill[SK_HEAVY_WEAPONS] >= 1 )
	//     {
	//         maxAmmo += GetWeaponTableData(weaponNum)->maxclip;
	//     }
	// }
	// else if (GetWeaponTableData(weaponNum)->isMortar || GetWeaponTableData(weaponNum)->isMortarSet)
	// {
	//     if( skill[SK_HEAVY_WEAPONS] >= 1 )
	//     {
	//         maxAmmo += 2;
	//     }
	// }
	else if (weaponNum == WP_MEDIC_SYRINGE /*|| weaponNum == WP_MEDIC_ADRENALINE*/) // adrenaline share the same ammo count as syringe
	{
		if (skill[SK_FIRST_AID] >= 2)
		{
			maxAmmo += 2;
		}
	}
	else if (GetWeaponTableData(weaponNum)->type & WEAPON_TYPE_RIFLE)  // also received ammo when weapon is scoped
	{
		if (skill[SK_LIGHT_WEAPONS] >= 1 || (skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1 && (GetWeaponTableData(weaponNum)->type & (WEAPON_TYPE_SCOPED | WEAPON_TYPE_SCOPABLE))))
		{
			maxAmmo += GetWeaponTableData(weaponNum)->maxClip;
		}
	}

	return maxAmmo;
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

/*
 * @brief PC_SourceWarning
 * @param[in] handle
 * @param[in] format
 * @note Unused
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
*/

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
 * @brief PC_Point_Parse
 * @param[in] handle
 * @param[out] c
 * @return
 */
qboolean PC_Point_Parse(int handle, vec2_t *c)
{
	int   i;
	float f;

	for (i = 0; i < 2; i++)
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

// US/UK Combined Communication Board 1943
const char *bg_fireteamNamesAllies[MAX_FIRETEAMS / 2] =
{
	"Able",
	"Baker",
	"Charlie",
	"Dog",
	"Easy",
	"Fox",
};

// Deutsches Reich 1934
const char *bg_fireteamNamesAxis[MAX_FIRETEAMS / 2] =
{
	"Anton",
	"Bruno",
	"Casar",
	"Dora",
	"Emil",
	"Fritz",
};

const voteType_t voteToggles[] =
{
	{ "vote_allow_config",                 CV_SVF_CONFIG                 },
	{ "vote_allow_gametype",               CV_SVF_GAMETYPE               },
	{ "vote_allow_kick",                   CV_SVF_KICK                   },
	{ "vote_allow_map",                    CV_SVF_MAP                    },
	{ "vote_allow_matchreset",             CV_SVF_MATCHRESET             },
	{ "vote_allow_mutespecs",              CV_SVF_MUTESPECS              },
	{ "vote_allow_nextmap",                CV_SVF_NEXTMAP                },
	{ "vote_allow_referee",                CV_SVF_REFEREE                },
	{ "vote_allow_shuffleteams",           CV_SVF_SHUFFLETEAMS           },
	{ "vote_allow_shuffleteams_norestart", CV_SVF_SHUFFLETEAMS_NORESTART },
	{ "vote_allow_swapteams",              CV_SVF_SWAPTEAMS              },
	{ "vote_allow_friendlyfire",           CV_SVF_FRIENDLYFIRE           },
	{ "vote_allow_timelimit",              CV_SVF_TIMELIMIT              },
	{ "vote_allow_warmupdamage",           CV_SVF_WARMUPDAMAGE           },
	{ "vote_allow_antilag",                CV_SVF_ANTILAG                },
	{ "vote_allow_balancedteams",          CV_SVF_BALANCEDTEAMS          },
	{ "vote_allow_muting",                 CV_SVF_MUTING                 },
	{ "vote_allow_surrender",              CV_SVF_SURRENDER              },
	{ "vote_allow_restartcampaign",        CV_SVF_RESTARTCAMPAIGN        },
	{ "vote_allow_nextcampaign",           CV_SVF_NEXTCAMPAIGN           },
	{ "vote_allow_poll",                   CV_SVF_POLL                   },
	{ "vote_allow_maprestart",             CV_SVF_MAPRESTART             },
	{ "vote_allow_cointoss",               CV_SVF_COINTOSS               }
};

int numVotesAvailable = sizeof(voteToggles) / sizeof(voteType_t);

// consts to offset random reinforcement seeds
const int aReinfSeeds[MAX_REINFSEEDS] = { 11, 3, 13, 7, 2, 5, 1, 17 };

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
	{ qfalse, "PNZR", "Panzer"     },  // 8  WS_PANZERFAUST
	{ qfalse, "BZKA", "Bazooka"    },  // 9  WS_BAZOOKA
	{ qfalse, "FLAM", "F.Thrower"  },  // 10 WS_FLAMETHROWER
	{ qfalse, "GRND", "Grenade"    },  // 11 WS_GRENADE
	{ qfalse, "MRTR", "Mortar"     },  // 12 WS_MORTAR
	{ qfalse, "GRWF", "Granatwerf" },  // 13 WS_MORTAR2
	{ qfalse, "DYNA", "Dynamite"   },  // 14 WS_DYNAMITE
	{ qfalse, "ARST", "Airstrike"  },  // 15 WS_AIRSTRIKE
	{ qfalse, "ARTY", "Artillery"  },  // 16 WS_ARTILLERY
	{ qfalse, "STCH", "Satchel"    },  // 17 WS_SATCHEL
	{ qfalse, "GRLN", "G.Launchr"  },  // 18 WS_GRENADELAUNCHER
	{ qfalse, "LNMN", "Landmine"   },  // 19 WS_LANDMINE
	{ qfalse, "MG42", "MG 42 Gun"  },  // 20 WS_MG42
	{ qfalse, "BRNG", "Browning"   },  // 21 WS_BROWNING
	{ qtrue,  "GARN", "Garand"     },  // 22 WS_CARBINE
	{ qtrue,  "K-43", "K43 Rifle"  },  // 23 WS_KAR98
	{ qtrue,  "SGRN", "Scp.Garand" },  // 24 WS_GARAND
	{ qtrue,  "SK43", "Scp.K43"    },  // 25 WS_K43
	{ qtrue,  "MP34", "MP 34"      },  // 26 WS_MP34
	{ qfalse, "SRNG", "Syringe"    }   // 27 WS_SYRINGE
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
		//case WEAPON_READYING:
		//case WEAPON_RELAXING:
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

/**
 * @brief BG_SurfaceForFootstep
 * @param[in] surfaceFlags
 * @return
 */
int BG_SurfaceForFootstep(int surfaceFlags)
{
	if (surfaceFlags == FOOTSTEP_TOTAL)
	{
		return SURF_NOSTEPS;
	}

	if (surfaceFlags == FOOTSTEP_METAL)
	{
		return SURF_METAL;
	}

	if (surfaceFlags == FOOTSTEP_WOOD)
	{
		return SURF_WOOD;
	}

	if (surfaceFlags == FOOTSTEP_GRASS)
	{
		return SURF_GRASS;
	}

	if (surfaceFlags == FOOTSTEP_GRAVEL)
	{
		return SURF_GRAVEL;
	}

	if (surfaceFlags == FOOTSTEP_ROOF)
	{
		return SURF_ROOF;
	}

	if (surfaceFlags == FOOTSTEP_SNOW)
	{
		return SURF_SNOW;
	}

	if (surfaceFlags == FOOTSTEP_CARPET)
	{
		return SURF_CARPET;
	}

	if (surfaceFlags == FOOTSTEP_SPLASH)
	{
		return SURF_SPLASH;
	}

	return 0;
}

/**
 * @brief BG_HeadCollisionBoxOffset
 * @param[in] ps
 * @param[out] headOffset
 */
void BG_HeadCollisionBoxOffset(vec3_t viewangles, int eFlags, vec3_t headOffset)
{
	vec3_t flatforward;
	float  angle;

	angle          = DEG2RAD(viewangles[YAW]);
	flatforward[0] = cos(angle);
	flatforward[1] = sin(angle);
	flatforward[2] = 0;

	if (eFlags & EF_DEAD)
	{
		VectorScale(flatforward, -24, headOffset);
	}
	else            // EF_PRONE
	{
		VectorScale(flatforward, 24, headOffset);
	}
}

/**
 * @brief BG_LegsCollisionBoxOffset
 * @param[in] ps
 * @param[out] legOffset
 */
void BG_LegsCollisionBoxOffset(vec3_t viewangles, int eFlags, vec3_t legsOffset)
{
	vec3_t flatforward;
	float  angle;

	angle          = DEG2RAD(viewangles[YAW]);
	flatforward[0] = cos(angle);
	flatforward[1] = sin(angle);
	flatforward[2] = 0;

	if (eFlags & EF_DEAD)
	{
		VectorScale(flatforward, 32, legsOffset);
	}
	else            // EF_PRONE
	{
		VectorScale(flatforward, -32, legsOffset);
	}
}
