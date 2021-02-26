/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * Portions of this file were taken from the NQ project.
 * Credit goes to core
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
 * @file g_spawn.c
 */

#include "g_local.h"

#ifdef FEATURE_LUA
#include "g_lua.h"
#endif

/**
 * @brief G_SpawnStringExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean G_SpawnStringExt(const char *key, const char *defaultString, char **out, const char *file, int line)
{
	int i;

	if (!level.spawning)
	{
		*out = (char *)defaultString;
		// see InitMover
		G_Error("G_SpawnString() called while not spawning, file %s, line %i\n", file, line);
	}

	for (i = 0 ; i < level.numSpawnVars ; i++)
	{
		if (!strcmp(key, level.spawnVars[i][0]))
		{
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

/**
 * @brief G_SpawnFloatExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean G_SpawnFloatExt(const char *key, const char *defaultString, float *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = G_SpawnStringExt(key, defaultString, &s, file, line);
	*out    = (float)atof(s);
	return present;
}

/**
 * @brief G_SpawnIntExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean G_SpawnIntExt(const char *key, const char *defaultString, int *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = G_SpawnStringExt(key, defaultString, &s, file, line);
	*out    = Q_atoi(s);
	return present;
}

/**
 * @brief G_SpawnVectorExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean G_SpawnVectorExt(const char *key, const char *defaultString, float *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = G_SpawnStringExt(key, defaultString, &s, file, line);
	sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
	return present;
}

/**
 * @brief G_SpawnVector2DExt
 * @param[in] key
 * @param[in] defaultString
 * @param[out] out
 * @param[in] file
 * @param[in] line
 * @return
 */
qboolean G_SpawnVector2DExt(const char *key, const char *defaultString, float *out, const char *file, int line)
{
	char     *s;
	qboolean present;

	present = G_SpawnStringExt(key, defaultString, &s, file, line);
	sscanf(s, "%f %f", &out[0], &out[1]);
	return present;
}

field_t fields[] =
{
	{ "classname",    FOFS(classname),      F_LSTRING,   0 },
	{ "origin",       FOFS(s.origin),       F_VECTOR,    0 },
	{ "model",        FOFS(model),          F_LSTRING,   0 },
	{ "model2",       FOFS(model2),         F_LSTRING,   0 },
	{ "spawnflags",   FOFS(spawnflags),     F_INT,       0 },
	{ "eflags",       FOFS(s.eFlags),       F_INT,       0 },
	{ "svflags",      FOFS(r.svFlags),      F_INT,       0 },
	{ "maxs",         FOFS(r.maxs),         F_VECTOR,    0 },
	{ "mins",         FOFS(r.mins),         F_VECTOR,    0 },
	{ "speed",        FOFS(speed),          F_FLOAT,     0 },
	{ "closespeed",   FOFS(closespeed),     F_FLOAT,     0 },
	{ "target",       FOFS(target),         F_LSTRING,   0 },
	{ "targetname",   FOFS(targetname),     F_LSTRING,   0 },
	{ "message",      FOFS(message),        F_LSTRING,   0 },
	{ "popup",        FOFS(message),        F_LSTRING,   0 }, // mutually exclusive from 'message', but makes the ent more logical for the level designer
	{ "book",         FOFS(message),        F_LSTRING,   0 }, // mutually exclusive from 'message', but makes the ent more logical for the level designer
	{ "team",         FOFS(team),           F_LSTRING,   0 },
	{ "wait",         FOFS(wait),           F_FLOAT,     0 },
	{ "random",       FOFS(random),         F_FLOAT,     0 },
	{ "count",        FOFS(count),          F_INT,       0 },
	{ "health",       FOFS(health),         F_INT,       0 },
	{ "light",        0,                    F_IGNORE,    0 },
	{ "dmg",          FOFS(damage),         F_INT,       0 },
	{ "angles",       FOFS(s.angles),       F_VECTOR,    0 },
	{ "angle",        FOFS(s.angles),       F_ANGLEHACK, 0 },

	{ "duration",     FOFS(duration),       F_FLOAT,     0 },
	{ "rotate",       FOFS(rotate),         F_VECTOR,    0 },

	{ "degrees",      FOFS(angle),          F_FLOAT,     0 },
	{ "time",         FOFS(speed),          F_FLOAT,     0 },

	// additional ai field
	{ "skin",         FOFS(aiSkin),         F_LSTRING,   0 },

	// dlight lightstyles (made all these unique variables for testing)
	{ "_color",       FOFS(dl_color),       F_VECTOR,    0 }, // color of the light	(the underscore is inserted by the color picker in QER)
	{ "color",        FOFS(dl_color),       F_VECTOR,    0 }, // color of the light
	{ "stylestring",  FOFS(dl_stylestring), F_LSTRING,   0 }, // user defined stylestring "fffndlsfaaaaaa" for example

	{ "shader",       FOFS(dl_shader),      F_LSTRING,   0 }, // shader to use for a target_effect or dlight

	// for target_unlock
	{ "key",          FOFS(key),            F_INT,       0 },

	// mg42
	{ "harc",         FOFS(harc),           F_FLOAT,     0 },
	{ "varc",         FOFS(varc),           F_FLOAT,     0 },

	// sniper
	{ "delay",        FOFS(delay),          F_FLOAT,     0 },
	{ "radius",       FOFS(radius),         F_INT,       0 },

	// for reloading savegames at correct mission spot
	{ "missionlevel", FOFS(missionLevel),   F_INT,       0 },

	{ "start_size",   FOFS(start_size),     F_INT,       0 },
	{ "end_size",     FOFS(end_size),       F_INT,       0 },

	{ "shard",        FOFS(count),          F_INT,       0 },

	{ "spawnitem",    FOFS(spawnitem),      F_LSTRING,   0 },

	{ "track",        FOFS(track),          F_LSTRING,   0 },

	{ "scriptName",   FOFS(scriptName),     F_LSTRING,   0 },

	{ "shortname",    FOFS(message),        F_LSTRING,   0 },
	{ "constages",    FOFS(constages),      F_LSTRING,   0 },
	{ "desstages",    FOFS(desstages),      F_LSTRING,   0 },
	{ "partofstage",  FOFS(partofstage),    F_INT,       0 },
	{ "override",     FOFS(spawnitem),      F_LSTRING,   0 },

	{ "damageparent", FOFS(damageparent),   F_LSTRING,   0 },

	{ "numPlayers",   FOFS(numPlayers),     F_INT,       0 }, // number of players needed to trigger this

	{ "contents",     FOFS(r.contents),     F_INT,       0 },
	{ "clipmask",     FOFS(clipmask),       F_INT,       0 },
	{ "count2",       FOFS(count2),         F_INT,       0 },
	// doors need this one
	{ "baseAngle",    FOFS(s.apos.trBase),  F_VECTOR,    0 },
	{ "baseOrigin",   FOFS(s.pos.trBase),   F_VECTOR,    0 },

	{ NULL,           0,                    F_IGNORE,    0 }
};

typedef struct
{
	char *name;
	void (*spawn)(gentity_t * ent);
} spawn_t;

void SP_info_player_start(gentity_t *ent);
void SP_info_player_checkpoint(gentity_t *ent);
void SP_info_player_deathmatch(gentity_t *ent);
void SP_info_player_intermission(gentity_t *ent);

void SP_func_plat(gentity_t *ent);
void SP_func_static(gentity_t *ent);
void SP_func_leaky(gentity_t *ent);
void SP_func_rotating(gentity_t *ent);
void SP_func_bobbing(gentity_t *ent);
void SP_func_pendulum(gentity_t *ent);
void SP_func_button(gentity_t *ent);
void SP_func_explosive(gentity_t *ent);
void SP_func_door(gentity_t *ent);
void SP_func_train(gentity_t *ent);
void SP_func_timer(gentity_t *self);

void SP_func_train_rotating(gentity_t *ent);
void SP_func_secret(gentity_t *ent);

void SP_func_door_rotating(gentity_t *ent);

void SP_func_constructible(gentity_t *ent);
void SP_func_brushmodel(gentity_t *ent);
void SP_misc_constructiblemarker(gentity_t *ent);
void SP_target_explosion(gentity_t *ent);
void SP_misc_landmine(gentity_t *ent);

void SP_trigger_always(gentity_t *ent);
void SP_trigger_multiple(gentity_t *ent);
void SP_trigger_push(gentity_t *ent);
void SP_trigger_teleport(gentity_t *ent);
void SP_trigger_hurt(gentity_t *ent);

void SP_trigger_heal(gentity_t *ent);
void SP_trigger_ammo(gentity_t *ent);

void SP_misc_cabinet_health(gentity_t *self);
void SP_misc_cabinet_supply(gentity_t *self);

// Wolf triggers
void SP_trigger_concussive_dust(gentity_t *ent);
void SP_trigger_once(gentity_t *ent);

void SP_target_remove_powerups(gentity_t *ent);
void SP_target_give(gentity_t *ent);
void SP_target_delay(gentity_t *ent);
void SP_target_speaker(gentity_t *ent);
void SP_target_print(gentity_t *ent);
void SP_target_laser(gentity_t *self);

void SP_target_teleporter(gentity_t *ent);
void SP_target_relay(gentity_t *ent);
void SP_target_kill(gentity_t *ent);
void SP_target_position(gentity_t *ent);
void SP_target_location(gentity_t *ent);
void SP_target_push(gentity_t *ent);
void SP_target_script_trigger(gentity_t *ent);
void SP_misc_beam(gentity_t *self);

// Wolf targets
void SP_target_alarm(gentity_t *ent);
void SP_target_counter(gentity_t *ent);
void SP_target_lock(gentity_t *ent);
void SP_target_effect(gentity_t *ent);
void SP_target_fog(gentity_t *ent);

// entity visibility dummy
void SP_misc_vis_dummy(gentity_t *ent);
void SP_misc_vis_dummy_multiple(gentity_t *ent);

void SP_light(gentity_t *self);
void SP_info_null(gentity_t *self);
void SP_info_notnull(gentity_t *self);
void SP_info_camp(gentity_t *self);
void SP_path_corner(gentity_t *self);
void SP_path_corner_2(gentity_t *self);
void SP_info_limbo_camera(gentity_t *self);
void SP_info_train_spline_main(gentity_t *self);

void SP_misc_teleporter_dest(gentity_t *self); // do not remove!
void SP_misc_model(gentity_t *ent);
void SP_misc_gamemodel(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);
void SP_misc_light_surface(gentity_t *ent);

void SP_misc_commandmap_marker(gentity_t *ent);

void SP_shooter_rocket(gentity_t *ent);
void SP_shooter_grenade(gentity_t *ent);

void SP_team_CTF_redspawn(gentity_t *ent);
void SP_team_CTF_bluespawn(gentity_t *ent);

// for multiplayer spawnpoint selection
void SP_team_WOLF_objective(gentity_t *ent);

void SP_team_WOLF_checkpoint(gentity_t *ent);

void SP_props_box_32(gentity_t *self);
void SP_props_box_48(gentity_t *self);
void SP_props_box_64(gentity_t *self);

// particles
void SP_target_smoke(gentity_t *ent);

// dlights
void SP_dlight(gentity_t *ent);

void SP_corona(gentity_t *ent);

void SP_mg42(gentity_t *ent);
void SP_aagun(gentity_t *ent);

void SP_shooter_mortar(gentity_t *ent);

// alarm
void SP_alarm_box(gentity_t *ent);

void SP_trigger_flagonly(gentity_t *ent);
void SP_trigger_flagonly_multiple(gentity_t *ent);
void SP_trigger_objective_info(gentity_t *ent);

void SP_target_rumble(gentity_t *ent);

void SP_SmokeDust(gentity_t *ent);
void SP_Dust(gentity_t *ent);
void SP_props_sparks(gentity_t *ent);
void SP_props_gunsparks(gentity_t *ent);

// Props
void SP_Props_Bench(gentity_t *ent);
void SP_Props_Radio(gentity_t *ent);
void SP_Props_Chair(gentity_t *ent);
void SP_Props_ChairHiback(gentity_t *ent);
void SP_Props_ChairSide(gentity_t *ent);
void SP_Props_ChairChat(gentity_t *ent);
void SP_Props_ChairChatArm(gentity_t *ent);
void SP_Props_DamageInflictor(gentity_t *ent);
void SP_Props_Locker_Tall(gentity_t *ent);
void SP_Props_Desklamp(gentity_t *ent);
void SP_Props_Flamebarrel(gentity_t *ent);
void SP_crate_64(gentity_t *ent);
void SP_Props_Flipping_Table(gentity_t *ent);
void SP_crate_32(gentity_t *self);
void SP_Props_Crate32x64(gentity_t *ent);
void SP_Props_58x112tablew(gentity_t *ent);
void SP_Props_RadioSEVEN(gentity_t *ent);

void SP_props_flamethrower(gentity_t *ent);

void SP_func_invisible_user(gentity_t *ent);

void SP_lightJunior(gentity_t *self);

void SP_misc_flak(gentity_t *ent);

void SP_props_snowGenerator(gentity_t *ent);

void SP_props_decoration(gentity_t *ent);
void SP_props_decorBRUSH(gentity_t *ent);
void SP_props_statue(gentity_t *ent);
void SP_props_statueBRUSH(gentity_t *ent);
void SP_skyportal(gentity_t *ent);

// scripting
void SP_script_model_med(gentity_t *ent);
void SP_script_mover(gentity_t *ent);
void SP_script_multiplayer(gentity_t *ent);

void SP_props_footlocker(gentity_t *self);
void SP_misc_firetrails(gentity_t *ent);

void SP_misc_spawner(gentity_t *ent);
void SP_props_decor_Scale(gentity_t *ent);

// debris test
void SP_func_debris(gentity_t *ent);
// ===================

// forty - etpro mapscripting - spawn function for fakebrushes
void SP_func_fakebrush(gentity_t *ent);

spawn_t spawns[] =
{
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{ "info_player_start",         SP_info_player_start         },
	{ "info_player_checkpoint",    SP_info_player_checkpoint    },
	{ "info_player_deathmatch",    SP_info_player_deathmatch    },
	{ "info_player_intermission",  SP_info_player_intermission  },
	{ "info_null",                 SP_info_null                 },
	{ "info_notnull",              SP_info_notnull              }, // use target_position instead
	{ "info_notnull_big",          SP_info_notnull              }, // use target_position instead
	{ "info_camp",                 SP_info_camp                 },

	// debris test
	{ "func_debris",               SP_func_debris               },
	// ===================

	{ "func_plat",                 SP_func_plat                 },
	{ "func_button",               SP_func_button               },
	{ "func_explosive",            SP_func_explosive            },
	{ "func_door",                 SP_func_door                 },
	{ "func_static",               SP_func_static               },
	{ "func_leaky",                SP_func_leaky                },
	{ "func_rotating",             SP_func_rotating             },
	{ "func_bobbing",              SP_func_bobbing              },
	{ "func_pendulum",             SP_func_pendulum             },
	{ "func_train",                SP_func_train                },
	{ "func_group",                SP_info_null                 },

	{ "func_train_rotating",       SP_func_train_rotating       },
	{ "func_secret",               SP_func_secret               },

	{ "func_door_rotating",        SP_func_door_rotating        },

	{ "func_timer",                SP_func_timer                }, // rename trigger_timer?

	{ "func_invisible_user",       SP_func_invisible_user       },

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{ "trigger_always",            SP_trigger_always            },
	{ "trigger_multiple",          SP_trigger_multiple          },
	{ "trigger_push",              SP_trigger_push              },
	{ "trigger_teleport",          SP_trigger_teleport          },
	{ "trigger_hurt",              SP_trigger_hurt              },

	// Wolf triggers
	{ "trigger_concussive_dust",   SP_trigger_concussive_dust   },
	{ "trigger_once",              SP_trigger_once              },

	{ "trigger_heal",              SP_trigger_heal              },
	{ "trigger_ammo",              SP_trigger_ammo              },

	// adding the model things to go with the triggers
	{ "misc_cabinet_health",       SP_misc_cabinet_health       },
	{ "misc_cabinet_supply",       SP_misc_cabinet_supply       },

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{ "target_give",               SP_target_give               },
	{ "target_remove_powerups",    SP_target_remove_powerups    },
	{ "target_delay",              SP_target_delay              },
	{ "target_speaker",            SP_target_speaker            },
	{ "target_print",              SP_target_print              },
	{ "target_laser",              SP_target_laser              },
	{ "target_teleporter",         SP_target_teleporter         },
	{ "target_relay",              SP_target_relay              },
	{ "target_kill",               SP_target_kill               },
	{ "target_position",           SP_target_position           },
	{ "target_location",           SP_target_location           },
	{ "target_push",               SP_target_push               },
	{ "target_script_trigger",     SP_target_script_trigger     },

	// Wolf targets
	{ "target_alarm",              SP_target_alarm              },
	{ "target_counter",            SP_target_counter            },
	{ "target_lock",               SP_target_lock               },
	{ "target_effect",             SP_target_effect             },
	{ "target_fog",                SP_target_fog                },

	{ "target_rumble",             SP_target_rumble             },

	{ "light",                     SP_light                     },

	{ "lightJunior",               SP_lightJunior               },

	{ "path_corner",               SP_path_corner               },
	{ "path_corner_2",             SP_path_corner_2             },

	{ "info_train_spline_main",    SP_info_train_spline_main    },
	{ "info_train_spline_control", SP_path_corner_2             },
	{ "info_limbo_camera",         SP_info_limbo_camera         },

	{ "misc_teleporter_dest",      SP_misc_teleporter_dest      },
	{ "misc_model",                SP_misc_model                },
	{ "misc_gamemodel",            SP_misc_gamemodel            },
	{ "misc_portal_surface",       SP_misc_portal_surface       },
	{ "misc_portal_camera",        SP_misc_portal_camera        },

	{ "misc_commandmap_marker",    SP_misc_commandmap_marker    },

	{ "misc_vis_dummy",            SP_misc_vis_dummy            },
	{ "misc_vis_dummy_multiple",   SP_misc_vis_dummy_multiple   },
	{ "misc_light_surface",        SP_misc_light_surface        },

	{ "misc_mg42",                 SP_mg42                      },
	{ "misc_aagun",                SP_aagun                     },

	{ "misc_flak",                 SP_misc_flak                 },
	{ "misc_firetrails",           SP_misc_firetrails           },

	{ "shooter_rocket",            SP_shooter_rocket            },
	{ "shooter_grenade",           SP_shooter_grenade           },

	{ "shooter_mortar",            SP_shooter_mortar            },
	{ "alarm_box",                 SP_alarm_box                 },

	{ "team_CTF_redspawn",         SP_team_CTF_redspawn         },
	{ "team_CTF_bluespawn",        SP_team_CTF_bluespawn        },

	{ "team_WOLF_objective",       SP_team_WOLF_objective       },

	{ "team_WOLF_checkpoint",      SP_team_WOLF_checkpoint      },

	{ "target_smoke",              SP_target_smoke              },

	{ "misc_spawner",              SP_misc_spawner              },

	{ "props_box_32",              SP_props_box_32              },
	{ "props_box_48",              SP_props_box_48              },
	{ "props_box_64",              SP_props_box_64              },

	{ "props_smokedust",           SP_SmokeDust                 },
	{ "props_dust",                SP_Dust                      },
	{ "props_sparks",              SP_props_sparks              },
	{ "props_gunsparks",           SP_props_gunsparks           },

	{ "props_bench",               SP_Props_Bench               },
	{ "props_radio",               SP_Props_Radio               },
	{ "props_chair",               SP_Props_Chair               },
	{ "props_chair_hiback",        SP_Props_ChairHiback         },
	{ "props_chair_side",          SP_Props_ChairSide           },
	{ "props_chair_chat",          SP_Props_ChairChat           },
	{ "props_chair_chatarm",       SP_Props_ChairChatArm        },
	{ "props_damageinflictor",     SP_Props_DamageInflictor     },
	{ "props_locker_tall",         SP_Props_Locker_Tall         },
	{ "props_desklamp",            SP_Props_Desklamp            },
	{ "props_flamebarrel",         SP_Props_Flamebarrel         },
	{ "props_crate_64",            SP_crate_64                  },
	{ "props_flippy_table",        SP_Props_Flipping_Table      },
	{ "props_crate_32",            SP_crate_32                  },
	{ "props_crate_32x64",         SP_Props_Crate32x64          },
	{ "props_58x112tablew",        SP_Props_58x112tablew        },
	{ "props_radioSEVEN",          SP_Props_RadioSEVEN          },
	{ "props_snowGenerator",       SP_props_snowGenerator       },

	{ "props_decoration",          SP_props_decoration          },
	{ "props_decorBRUSH",          SP_props_decorBRUSH          },
	{ "props_statue",              SP_props_statue              },
	{ "props_statueBRUSH",         SP_props_statueBRUSH         },
	{ "props_skyportal",           SP_skyportal                 },
	{ "props_footlocker",          SP_props_footlocker          },
	{ "props_flamethrower",        SP_props_flamethrower        },
	{ "props_decoration_scale",    SP_props_decor_Scale         },

	{ "dlight",                    SP_dlight                    },

	{ "corona",                    SP_corona                    },

	{ "trigger_flagonly",          SP_trigger_flagonly          },
	{ "trigger_flagonly_multiple", SP_trigger_flagonly_multiple },

	{ "trigger_objective_info",    SP_trigger_objective_info    },

	// scripting
	{ "script_model_med",          SP_script_model_med          },
	{ "script_mover",              SP_script_mover              },
	{ "script_multiplayer",        SP_script_multiplayer        },

	{ "func_constructible",        SP_func_constructible        },
	{ "func_brushmodel",           SP_func_brushmodel           },
	{ "misc_beam",                 SP_misc_beam                 },
	{ "misc_constructiblemarker",  SP_misc_constructiblemarker  },
	{ "target_explosion",          SP_target_explosion          },
	{ "misc_landmine",             SP_misc_landmine             },

	{ "func_fakebrush",            SP_func_fakebrush            },

	{ 0,                           0                            }
};

/**
 * @brief Finds the spawn function for the entity and calls it
 * @param ent
 * @return qfalse if spawn not found
 */
qboolean G_CallSpawn(gentity_t *ent)
{
	spawn_t *s;
	gitem_t *item;

	if (!ent->classname)
	{
		G_Printf("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

	// check item spawn functions
	item = BG_FindItemForClassName(ent->classname);

	if (item)
	{
		// found it
		if (g_gametype.integer != GT_WOLF_LMS)     // lets not have items in last man standing for the moment
		{
			G_SpawnItem(ent, item);

			G_Script_ScriptParse(ent);
			G_Script_ScriptEvent(ent, "spawn", "");
			return qtrue;
		}

		return qfalse;
	}

	// check normal spawn functions
	for (s = spawns ; s->name ; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{
			// found it
			s->spawn(ent);

			// entity scripting
			if (/*ent->s.number >= MAX_CLIENTS &&*/ ent->scriptName)
			{
				G_Script_ScriptParse(ent);
				G_Script_ScriptEvent(ent, "spawn", "");
			}

			return qtrue;
		}
	}

	// hack: this avoids spammy prints on start, bsp uses obsolete classnames!
	// bot_sniper_spot (railgun)
	if (Q_stricmp(ent->classname, "bot_sniper_spot"))
	{
		G_Printf("%s doesn't have a spawn function\n", ent->classname);
	}

	return qfalse;
}

/**
 * @brief Builds a copy of the string, translating \\n to real linefeeds
 * so message texts can be multi-line
 * @param string
 * @return
 */
char *G_NewString(const char *string)
{
	char         *newb, *new_p;
	unsigned int i, l;

	l = strlen(string) + 1;

	newb = G_Alloc(l);

	new_p = newb;

	// turn \n into a real linefeed
	for (i = 0 ; i < l ; i++)
	{
		if (i < l - 1 && string[i] == '\\')
		{
			i++;
			if (string[i] == 'n')
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '\\';
			}
		}
		else
		{
			*new_p++ = string[i];
		}
	}

	return newb;
}

/**
 * @brief Takes a key/value pair and sets the binary values
 * in a gentity
 * @param[in] key
 * @param[in] value
 * @param[in] ent
 */
void G_ParseField(const char *key, const char *value, gentity_t *ent)
{
	field_t *f;
	byte    *b;
	float   v;
	vec3_t  vec;

	for (f = fields ; f->name ; f++)
	{
		if (!Q_stricmp(f->name, key))
		{
			// found it
			b = (byte *)ent;

			switch (f->type)
			{
			case F_LSTRING:
				*( char ** )(b + f->ofs) = G_NewString(value);
				break;
			case F_VECTOR:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				(( float * )(b + f->ofs))[0] = vec[0];
				(( float * )(b + f->ofs))[1] = vec[1];
				(( float * )(b + f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*( int * )(b + f->ofs) = Q_atoi(value);
				break;
			case F_FLOAT:
				*( float * )(b + f->ofs) = Q_atof(value);
				break;
			case F_ANGLEHACK:
				v                            = Q_atof(value);
				(( float * )(b + f->ofs))[0] = 0;
				(( float * )(b + f->ofs))[1] = v;
				(( float * )(b + f->ofs))[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}

/**
 * @brief Spawn an entity and fill in all of the level fields from
 * level.spawnVars[], then call the class specfic spawn function
 * @return
 */
gentity_t *G_SpawnGEntityFromSpawnVars(void)
{
	int       i;
	gentity_t *ent;
	char      *str;

	ent = G_Spawn(); // get the next free entity

	for (i = 0 ; i < level.numSpawnVars ; i++)
	{
		G_ParseField(level.spawnVars[i][0], level.spawnVars[i][1], ent);
	}

	// check for "notteam" / "notfree" flags
	G_SpawnInt("notteam", "0", &i);
	if (i)
	{
		G_Printf("G_SpawnGEntityFromSpawnVars Warning: Can't spawn entity in team games - returning NULL\n");

		G_FreeEntity(ent);
		return NULL;
	}

	// allowteams handling
	G_SpawnString("allowteams", "", &str);
	if (str[0])
	{
		str = Q_strlwr(str);
		if (strstr(str, "axis"))
		{
			ent->allowteams |= ALLOW_AXIS_TEAM;
		}
		if (strstr(str, "allies"))
		{
			ent->allowteams |= ALLOW_ALLIED_TEAM;
		}
		if (strstr(str, "cvops"))
		{
			ent->allowteams |= ALLOW_DISGUISED_CVOPS;
		}
	}

	if (ent->targetname && *ent->targetname)
	{
		ent->targetnamehash = BG_StringHashValue(ent->targetname);
	}
	else
	{
		ent->targetnamehash = -1;
	}

	// move editor origin to pos
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	// if we didn't get a classname, don't bother spawning anything
	if (!G_CallSpawn(ent))
	{
		G_FreeEntity(ent);
	}

	return ent;
}

/**
 * @brief G_AddSpawnVarToken
 * @param[in] string
 * @return
 */
char *G_AddSpawnVarToken(const char *string)
{
	size_t l;
	char   *dest;

	l = strlen(string);
	if (level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS)
	{
		G_Error("G_AddSpawnVarToken: MAX_SPAWN_VARS\n");
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	Com_Memcpy(dest, string, l + 1);

	level.numSpawnVarChars += l + 1;

	return dest;
}

/**
 * @brief Parses a brace bounded set of key / value pairs out of the
 * level's entity strings into level.spawnVars[]
 * This does not actually spawn an entity.
 * @return
 */
qboolean G_ParseSpawnVars(void)
{
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars     = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if (!trap_GetEntityToken(com_token, sizeof(com_token)))
	{
		// end of spawn string
		return qfalse;
	}
	if (com_token[0] != '{')
	{
		G_Error("G_ParseSpawnVars: found %s when expecting {\n", com_token);
	}

	// go through all the key / value pairs
	while (1)
	{
		// parse key
		if (!trap_GetEntityToken(keyname, sizeof(keyname)))
		{
			G_Error("G_ParseSpawnVars: EOF without closing brace\n");
		}

		if (keyname[0] == '}')
		{
			break;
		}

		// parse value
		if (!trap_GetEntityToken(com_token, sizeof(com_token)))
		{
			G_Error("G_ParseSpawnVars: EOF without closing brace\n");
		}

		if (com_token[0] == '}')
		{
			G_Error("G_ParseSpawnVars: closing brace without data\n");
		}
		if (level.numSpawnVars == MAX_SPAWN_VARS)
		{
			G_Error("G_ParseSpawnVars: MAX_SPAWN_VARS\n");
		}
		level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken(keyname);
		level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken(com_token);
		level.numSpawnVars++;
	}

	return qtrue;
}

/**
 * @brief Every map should have exactly one worldspawn.
 * @details QUAKED worldspawn (0 0 0) ? NO_GT_WOLF NO_GT_STOPWATCH NO_GT_CHECKPOINT NO_LMS
 *
 * "music"     Music wav file
 * "gravity"   800 is default gravity
 * "message" Text to print during connection process
 * "ambient"  Ambient light value (must use '_color')
 * "_color"    Ambient light color (must be used with 'ambient')
 * "sun"        Shader to use for 'sun' image
 */
void SP_worldspawn(void)
{
	char *s;

	G_SpawnString("classname", "", &s);
	if (Q_stricmp(s, "worldspawn"))
	{
		G_Error("SP_worldspawn: The first entity isn't 'worldspawn'\n");
	}

	// make some data visible to connecting client
	trap_SetConfigstring(CS_GAME_VERSION, GAME_VERSION);

	trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));

	G_SpawnString("music", "", &s);
	trap_SetConfigstring(CS_MUSIC, s);

	G_SpawnString("message", "", &s);
	trap_SetConfigstring(CS_MESSAGE, s);                // map specific message

	G_SpawnString("cclayers", "0", &s);
	if (atoi(s))
	{
		level.ccLayers = qtrue;
	}

	level.mapcoordsValid = qfalse;
	if (G_SpawnVector2D("mapcoordsmins", "-128 128", level.mapcoordsMins) &&     // top left
	    G_SpawnVector2D("mapcoordsmaxs", "128 -128", level.mapcoordsMaxs))       // bottom right
	{
		level.mapcoordsValid = qtrue;
	}

	BG_InitLocations(level.mapcoordsMins, level.mapcoordsMaxs);

	trap_SetConfigstring(CS_MOTD, g_motd.string);       // message of the day

	G_SpawnString("gravity", "800", &s);
	trap_Cvar_Set("g_gravity", s);

	G_SpawnString("spawnflags", "0", &s);
	g_entities[ENTITYNUM_WORLD].spawnflags   = Q_atoi(s);
	g_entities[ENTITYNUM_WORLD].r.worldflags = g_entities[ENTITYNUM_WORLD].spawnflags;

	g_entities[ENTITYNUM_WORLD].s.number   = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname  = "worldspawn";

	g_entities[ENTITYNUM_NONE].s.number   = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_NONE].classname  = "nothing";

	// see if we want a warmup time
	trap_SetConfigstring(CS_WARMUP, "");
	if (g_restarted.integer)
	{
		trap_Cvar_Set("g_restarted", "0");
		level.warmupTime = 0;
	}

	if (g_gamestate.integer == GS_PLAYING)
	{
		G_initMatch();
	}
}

/**
 * @brief forty - etpro mapscripting - spawn function for fake brushes
 * @param ent
 */
void SP_func_fakebrush(gentity_t *ent)
{
	ent->s.eFlags |= EF_FAKEBMODEL;
	G_SetOrigin(ent, ent->s.origin);

	return;
}

/**
 * @brief Parses textual entity definitions out of an entstring and spawns gentities.
 */
void G_SpawnEntitiesFromString(void)
{
	// allow calls to G_Spawn*()
	G_Printf("Enable spawning!\n");
	level.spawning     = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if (!G_ParseSpawnVars())
	{
		G_Error("SpawnEntities: no entities\n");
	}
	SP_worldspawn();

	// parse ents
	while (G_ParseSpawnVars())
	{
		G_SpawnGEntityFromSpawnVars();
	}

#ifdef FEATURE_LUA
	G_LuaHook_SpawnEntitiesFromString();
#endif

	G_Printf("Disable spawning!\n");
	level.spawning = qfalse;            // any future calls to G_Spawn*() will be errors
}

#ifdef FEATURE_LUA
//===============================================================
// Some helper functions for entity property handling..
// these functions are used by Lua.

/**
 * @brief GetFieldIndex
 * @param[in] fieldname
 * @return The index in the fiels[] array of the given fieldname, -1 if not found..
 */
int GetFieldIndex(const char *fieldname)
{
	int i;

	for (i = 0; fields[i].name; i++)
		if (!Q_stricmp(fields[i].name, fieldname))
		{
			return i;
		}
	return -1;
}

/**
 * @brief GetFieldType
 * @param[in] fieldname
 * @return The fieldType of the given fieldname, otherwise F_IGNORE if the field is not found.
 */
fieldtype_t GetFieldType(const char *fieldname)
{
	int index = GetFieldIndex(fieldname);

	if (index == -1)
	{
		return F_IGNORE;
	}
	return fields[index].type;
}
#endif // FEATURE_LUA
