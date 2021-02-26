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
 * @file g_script.c
 * @author Ridah
 * @brief Wolfenstein Entity Scripting
 * @details Scripting that allows the designers to control the behaviour of entities
 * according to each different scenario.
 */

#include "g_local.h"
#include "g_lua.h"

#ifdef FEATURE_OMNIBOT
#include "g_etbot_interface.h"
#endif

//====================================================================

/**
 * @var gScriptActions
 * @brief These are the actions that each event can call
 */
static g_script_stack_action_t gScriptActions[] =
{
	{ "gotomarker",                     G_ScriptAction_GotoMarker,                    GOTOMARKER_HASH                     },
	{ "playsound",                      G_ScriptAction_PlaySound,                     PLAYSOUND_HASH                      },
	{ "playanim",                       G_ScriptAction_PlayAnim,                      PLAYANIM_HASH                       },
	{ "wait",                           G_ScriptAction_Wait,                          WAIT_HASH                           },
	{ "trigger",                        G_ScriptAction_Trigger,                       TRIGGER_HASH                        },
	{ "alertentity",                    G_ScriptAction_AlertEntity,                   ALERTENTITY_HASH                    },
	{ "togglespeaker",                  G_ScriptAction_ToggleSpeaker,                 TOGGLESPEAKER_HASH                  },
	{ "disablespeaker",                 G_ScriptAction_DisableSpeaker,                DISABLESPEAKER_HASH                 },
	{ "enablespeaker",                  G_ScriptAction_EnableSpeaker,                 ENABLESPEAKER_HASH                  },
	{ "accum",                          G_ScriptAction_Accum,                         ACCUM_HASH                          },
	{ "globalaccum",                    G_ScriptAction_GlobalAccum,                   GLOBALACCUM_HASH                    },
	{ "print",                          G_ScriptAction_Print,                         PRINT_HASH                          },
	{ "faceangles",                     G_ScriptAction_FaceAngles,                    FACEANGLES_HASH                     },
	{ "resetscript",                    G_ScriptAction_ResetScript,                   RESETSCRIPT_HASH                    },
	{ "attachtotag",                    G_ScriptAction_TagConnect,                    ATTACHTOTAG_HASH                    },
	{ "halt",                           G_ScriptAction_Halt,                          HALT_HASH                           },
	{ "stopsound",                      G_ScriptAction_StopSound,                     STOPSOUND_HASH                      },
	{ "entityscriptname",               G_ScriptAction_EntityScriptName,              ENTITYSCRIPTNAME_HASH               },
	{ "wm_axis_respawntime",            G_ScriptAction_AxisRespawntime,               WM_AXIS_RESPAWNTIME_HASH            },
	{ "wm_allied_respawntime",          G_ScriptAction_AlliedRespawntime,             WM_ALLIED_RESPAWNTIME_HASH          },
	{ "wm_number_of_objectives",        G_ScriptAction_NumberofObjectives,            WM_NUMBER_OF_OBJECTIVES_HASH        },
	{ "wm_setwinner",                   G_ScriptAction_SetWinner,                     WM_SETWINNER_HASH                   },
	{ "wm_set_defending_team",          G_ScriptAction_SetDefendingTeam,              WM_SET_DEFENDING_TEAM_HASH          },
	{ "wm_announce",                    G_ScriptAction_Announce,                      WM_ANNOUNCE_HASH                    },
	{ "wm_teamvoiceannounce",           G_ScriptAction_TeamVoiceAnnounce,             WM_TEAMVOICEANNOUNCE_HASH           },
	{ "wm_addteamvoiceannounce",        G_ScriptAction_AddTeamVoiceAnnounce,          WM_ADDTEAMVOICEANNOUNCE_HASH        },
	{ "wm_removeteamvoiceannounce",     G_ScriptAction_RemoveTeamVoiceAnnounce,       WM_REMOVETEAMVOICEANNOUNCE_HASH     },
	{ "wm_announce_icon",               G_ScriptAction_Announce_Icon,                 WM_ANNOUNCE_ICON_HASH               },
	{ "wm_endround",                    G_ScriptAction_EndRound,                      WM_ENDROUND_HASH                    },
	{ "wm_set_round_timelimit",         G_ScriptAction_SetRoundTimelimit,             WM_SET_ROUND_TIMELIMIT_HASH         },
	{ "wm_voiceannounce",               G_ScriptAction_VoiceAnnounce,                 WM_VOICEANNOUNCE_HASH               },
	{ "wm_objective_status",            G_ScriptAction_ObjectiveStatus,               WM_OBJECTIVE_STATUS_HASH            },
	{ "wm_set_main_objective",          G_ScriptAction_SetMainObjective,              WM_SET_MAIN_OBJECTIVE_HASH          },
	{ "remove",                         G_ScriptAction_RemoveEntity,                  REMOVE_HASH                         },
	{ "setstate",                       G_ScriptAction_SetState,                      SETSTATE_HASH                       },
	{ "followspline",                   G_ScriptAction_FollowSpline,                  FOLLOWSPLINE_HASH                   },
	{ "followpath",                     G_ScriptAction_FollowPath,                    FOLLOWPATH_HASH                     },
	{ "abortmove",                      G_ScriptAction_AbortMove,                     ABORTMOVE_HASH                      },
	{ "setspeed",                       G_ScriptAction_SetSpeed,                      SETSPEED_HASH                       },
	{ "setrotation",                    G_ScriptAction_SetRotation,                   SETROTATION_HASH                    },
	{ "stoprotation",                   G_ScriptAction_StopRotation,                  STOPROTATION_HASH                   },
	{ "startanimation",                 G_ScriptAction_StartAnimation,                STARTANIMATION_HASH                 },
	{ "attatchtotrain",                 G_ScriptAction_AttatchToTrain,                ATTATCHTOTRAIN_HASH                 },
	{ "freezeanimation",                G_ScriptAction_FreezeAnimation,               FREEZEANIMATION_HASH                },
	{ "unfreezeanimation",              G_ScriptAction_UnFreezeAnimation,             UNFREEZEANIMATION_HASH              },
	{ "remapshader",                    G_ScriptAction_ShaderRemap,                   REMAPSHADER_HASH                    },
	{ "remapshaderflush",               G_ScriptAction_ShaderRemapFlush,              REMAPSHADERFLUSH_HASH               },
	{ "changemodel",                    G_ScriptAction_ChangeModel,                   CHANGEMODEL_HASH                    },
	{ "setchargetimefactor",            G_ScriptAction_SetChargeTimeFactor,           SETCHARGETIMEFACTOR_HASH            },
	{ "setdamagable",                   G_ScriptAction_SetDamagable,                  SETDAMAGABLE_HASH                   },

	{ "repairmg42",                     G_ScriptAction_RepairMG42,                    REPAIRMG42_HASH                     },
	{ "sethqstatus",                    G_ScriptAction_SetHQStatus,                   SETHQSTATUS_HASH                    },

	{ "printaccum",                     G_ScriptAction_PrintAccum,                    PRINTACCUM_HASH                     },
	{ "printglobalaccum",               G_ScriptAction_PrintGlobalAccum,              PRINTGLOBALACCUM_HASH               },
	{ "cvar",                           G_ScriptAction_Cvar,                          CVAR_HASH                           },
	{ "abortifwarmup",                  G_ScriptAction_AbortIfWarmup,                 ABORTIFWARMUP_HASH                  },
	{ "abortifnotsingleplayer",         G_ScriptAction_AbortIfNotSinglePlayer,        ABORTIFNOTSINGLEPLAYER_HASH         },

	{ "mu_start",                       G_ScriptAction_MusicStart,                    MU_START_HASH                       },
	{ "mu_play",                        G_ScriptAction_MusicPlay,                     MU_PLAY_HASH                        },
	{ "mu_stop",                        G_ScriptAction_MusicStop,                     MU_STOP_HASH                        },
	{ "mu_queue",                       G_ScriptAction_MusicQueue,                    MU_QUEUE_HASH                       },
	{ "mu_fade",                        G_ScriptAction_MusicFade,                     MU_FADE_HASH                        },
	{ "setdebuglevel",                  G_ScriptAction_SetDebugLevel,                 SETDEBUGLEVEL_HASH                  },
	{ "setposition",                    G_ScriptAction_SetPosition,                   SETPOSITION_HASH                    },
	{ "setautospawn",                   G_ScriptAction_SetAutoSpawn,                  SETAUTOSPAWN_HASH                   },

	// going for longest silly script command ever here :) (sets a model for a brush to one stolen from a func_brushmodel
	{ "setmodelfrombrushmodel",         G_ScriptAction_SetModelFromBrushmodel,        SETMODELFROMBRUSHMODEL_HASH         },

	// fade all sounds up or down
	{ "fadeallsounds",                  G_ScriptAction_FadeAllSounds,                 FADEALLSOUNDS_HASH                  },

	{ "construct",                      G_ScriptAction_Construct,                     CONSTRUCT_HASH                      },
	{ "spawnrubble",                    G_ScriptAction_SpawnRubble,                   SPAWNRUBBLE_HASH                    },
	{ "setglobalfog",                   G_ScriptAction_SetGlobalFog,                  SETGLOBALFOG_HASH                   },
	{ "allowtankexit",                  G_ScriptAction_AllowTankExit,                 ALLOWTANKEXIT_HASH                  },
	{ "allowtankenter",                 G_ScriptAction_AllowTankEnter,                ALLOWTANKENTER_HASH                 },
	{ "settankammo",                    G_ScriptAction_SetTankAmmo,                   SETTANKAMMO_HASH                    },
	{ "addtankammo",                    G_ScriptAction_AddTankAmmo,                   ADDTANKAMMO_HASH                    },
	{ "kill",                           G_ScriptAction_Kill,                          KILL_HASH                           },
	{ "disablemessage",                 G_ScriptAction_DisableMessage,                DISABLEMESSAGE_HASH                 },

	{ "set",                            etpro_ScriptAction_SetValues,                 SET_HASH                            },
	{ "create",                         G_ScriptAction_Create,                        CREATE_HASH                         },
	{ "delete",                         G_ScriptAction_Delete,                        DELETE_HASH                         },

	{ "constructible_class",            G_ScriptAction_ConstructibleClass,            CONSTRUCTIBLE_CLASS_HASH            },
	{ "constructible_chargebarreq",     G_ScriptAction_ConstructibleChargeBarReq,     CONSTRUCTIBLE_CHARGEBARREQ_HASH     },
	{ "constructible_constructxpbonus", G_ScriptAction_ConstructibleConstructXPBonus, CONSTRUCTIBLE_CONSTRUCTXPBONUS_HASH },
	{ "constructible_destructxpbonus",  G_ScriptAction_ConstructibleDestructXPBonus,  CONSTRUCTIBLE_DESTRUCTXPBONUS_HASH  },
	{ "constructible_health",           G_ScriptAction_ConstructibleHealth,           CONSTRUCTIBLE_HEALTH_HASH           },
	{ "constructible_weaponclass",      G_ScriptAction_ConstructibleWeaponclass,      CONSTRUCTIBLE_WEAPONCLASS_HASH      },
	{ "constructible_duration",         G_ScriptAction_ConstructibleDuration,         CONSTRUCTIBLE_DURATION_HASH         },
	{ NULL,                             NULL,                                         00                                  }
};

qboolean G_Script_EventMatch_StringEqual(g_script_event_t *event, const char *eventParm);
qboolean G_Script_EventMatch_IntInRange(g_script_event_t *event, const char *eventParm);

/**
 * @var gScriptEvents
 * @details The list of events that can start an action sequence
 */
static g_script_event_define_t gScriptEvents[] =
{
	{ "spawn",       NULL,                            SPAWN_HASH       }, ///< called as each character is spawned into the game
	{ "trigger",     G_Script_EventMatch_StringEqual, TRIGGER_HASH     }, ///< something has triggered us (always followed by an identifier)
	{ "pain",        G_Script_EventMatch_IntInRange,  PAIN_HASH        }, ///< we've been hurt
	{ "death",       NULL,                            DEATH_HASH       }, ///< RIP
	{ "activate",    G_Script_EventMatch_StringEqual, ACTIVATE_HASH    }, ///< something has triggered us [activator team]
	{ "stopcam",     NULL,                            STOPCAM_HASH     },
	{ "playerstart", NULL,                            PLAYERSTART_HASH },
	{ "built",       G_Script_EventMatch_StringEqual, BUILT_HASH       },
	{ "buildstart",  G_Script_EventMatch_StringEqual, BUILDSTART_HASH  },
	{ "decayed",     G_Script_EventMatch_StringEqual, DECAYED_HASH     },
	{ "destroyed",   G_Script_EventMatch_StringEqual, DESTROYED_HASH   },
	{ "rebirth",     NULL,                            REBIRTH_HASH     },
	{ "failed",      NULL,                            FAILED_HASH      },
	{ "dynamited",   NULL,                            DYNAMITED_HASH   },
	{ "defused",     NULL,                            DEFUSED_HASH     },
	{ "mg42",        G_Script_EventMatch_StringEqual, MG42_HASH        },
	{ "message",     G_Script_EventMatch_StringEqual, MESSAGE_HASH     }, ///< contains a sequence of VO in a message
	{ "exploded",    NULL,                            EXPLODED_HASH    }, ///< added for omni-bot 0.7

	{ NULL,          NULL,                            00               }
};

/**
 * @brief G_Script_EventMatch_StringEqual
 * @param[in] event
 * @param[in] eventParm
 * @return
 */
qboolean G_Script_EventMatch_StringEqual(g_script_event_t *event, const char *eventParm)
{
	return eventParm && !Q_stricmp(event->params, eventParm);
}

/**
 * @brief G_Script_EventMatch_IntInRange
 * @param[in] event
 * @param[in] eventParm
 * @return
 */
qboolean G_Script_EventMatch_IntInRange(g_script_event_t *event, const char *eventParm)
{
	char *pString, *token;
	int  int1, int2, eInt;

	// get the cast name
	pString = (char *)eventParm;
	token   = COM_ParseExt(&pString, qfalse);
	int1    = Q_atoi(token);
	token   = COM_ParseExt(&pString, qfalse);
	int2    = Q_atoi(token);

	eInt = Q_atoi(event->params);

	return eventParm && eInt > int1 && eInt <= int2;
}

/**
 * @brief G_Script_EventForString
 * @param[in] string
 * @return
 */
int G_Script_EventForString(const char *string)
{
	int i, hash;

	hash = BG_StringHashValue_Lwr(string);

	for (i = 0; gScriptEvents[i].eventStr; i++)
	{
		if (gScriptEvents[i].hash == hash)
		{
			return i;
		}
	}

	// this occures on map start when map script isn't fully init
	return -1;
}

/**
 * @brief G_Script_ActionForString
 * @param[in] string
 * @return
 */
g_script_stack_action_t *G_Script_ActionForString(const char *string)
{
	int i, hash;

	hash = BG_StringHashValue_Lwr(string);

	for (i = 0; gScriptActions[i].actionString; i++)
	{
		if (gScriptActions[i].hash == hash)
		{
			return &gScriptActions[i];
		}
	}

	G_Printf("G_Script_ActionForString warning: unknown action: '%s' - returning NULL\n", string);

	return NULL;
}

/**
 * @brief Loads the script for the current level into the buffer
 */
void G_Script_ScriptLoad(void)
{
	char         filename[MAX_QPATH];
	vmCvar_t     mapname;
	fileHandle_t f     = 0;
	int          len   = 0;
	qboolean     found = qfalse;

	level.scriptEntity = NULL;

	trap_Cvar_VariableStringBuffer("g_scriptName", filename, sizeof(filename));

	if (strlen(filename) > 0)
	{
		trap_Cvar_Register(&mapname, "g_scriptName", "", CVAR_CHEAT);
	}
	else
	{
		trap_Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
	}

	if (g_mapScriptDirectory.string[0])
	{
		Q_strncpyz(filename, g_mapScriptDirectory.string, sizeof(filename));
		Q_strcat(filename, sizeof(filename), "/");
		Q_strcat(filename, sizeof(filename), mapname.string);

		if (g_gametype.integer == GT_WOLF_LMS)
		{
			Q_strcat(filename, sizeof(filename), "_lms");
		}

		Q_strcat(filename, sizeof(filename), ".script");
		len = trap_FS_FOpenFile(filename, &f, FS_READ);

		if (len > 0)
		{
			found = qtrue;
		}
	}

	if (!found)
	{
		Q_strncpyz(filename, "maps/", sizeof(filename));
		Q_strcat(filename, sizeof(filename), mapname.string);

		if (g_gametype.integer == GT_WOLF_LMS)
		{
			Q_strcat(filename, sizeof(filename), "_lms");
		}

		Q_strcat(filename, sizeof(filename), ".script");
		len = trap_FS_FOpenFile(filename, &f, FS_READ);
	}

	// make sure we clear out the temporary scriptname
	trap_Cvar_Set("g_scriptName", "");

	if (len < 0)
	{
		return;
	}

	// make sure we terminate the script with a '\0' to prevent parser from choking
	level.scriptEntity = G_Alloc(len + 1);
	trap_FS_Read(level.scriptEntity, len, f);
	*(level.scriptEntity + len) = '\0';

	trap_FS_FCloseFile(f);
}

/**
 * @brief Parses the script for the given entity
 * @param[in,out] ent
 */
void G_Script_ScriptParse(gentity_t *ent)
{
	char                    *pScript;
	char                    *token;
	qboolean                wantName;
	qboolean                wantScript;
	qboolean                inScript;
	int                     eventNum;
	g_script_event_t        events[G_MAX_SCRIPT_STACK_ITEMS];
	unsigned int            numEventItems;
	g_script_event_t        *curEvent;
	char                    params[MAX_INFO_STRING]; // was MAX_QPATH some of our multiplayer script commands have longer parameters
	g_script_stack_action_t *action;
	int                     i;
	int                     bracketLevel;
	unsigned int            len;
	qboolean                buildScript;

	if (!ent->scriptName)
	{
		return;
	}
	if (!level.scriptEntity)
	{
		return;
	}

	buildScript = (qboolean)(trap_Cvar_VariableIntegerValue("com_buildScript"));

	pScript    = level.scriptEntity;
	wantName   = qtrue;
	wantScript = qfalse;
	inScript   = qfalse;
	COM_BeginParseSession("G_Script_ScriptParse");
	bracketLevel  = 0;
	numEventItems = 0;

	Com_Memset(events, 0, sizeof(events));

	while (1)
	{
		token = COM_Parse(&pScript);

		if (!token[0])
		{
			if (!wantName)
			{
				G_Error("G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine());
			}
			break;
		}

		// end of script
		if (token[0] == '}')
		{
			if (inScript)
			{
				break;
			}
			if (wantName)
			{
				G_Error("G_Script_ScriptParse(), Error (line %d): '}' found, but not expected.\n", COM_GetCurrentParseLine());
			}
			wantName = qtrue;
		}
		else if (token[0] == '{')
		{
			if (wantName)
			{
				G_Error("G_Script_ScriptParse(), Error (line %d): '{' found, NAME expected.\n", COM_GetCurrentParseLine());
			}
			wantScript = qfalse;
		}
		else if (wantName)
		{
			if (!Q_stricmp(token, "entity"))
			{
				// this is an entity, so go back to look for a name
				continue;
			}
			if (!Q_stricmp(ent->scriptName, token))
			{
				inScript      = qtrue;
				numEventItems = 0;
			}
			wantName   = qfalse;
			wantScript = qtrue;
		}
		else if (inScript)
		{
			Q_strlwr(token);
			eventNum = G_Script_EventForString(token);
			if (eventNum < 0)
			{
				G_Error("G_Script_ScriptParse(), Error (line %d): unknown event: %s.\n", COM_GetCurrentParseLine(), token);
			}

			if (numEventItems >= G_MAX_SCRIPT_STACK_ITEMS)
			{
				G_Error("G_Script_ScriptParse(), Error (line %d): G_MAX_SCRIPT_STACK_ITEMS reached (%d)\n", COM_GetCurrentParseLine(), G_MAX_SCRIPT_STACK_ITEMS);
			}

			curEvent           = &events[numEventItems];
			curEvent->eventNum = eventNum;
			Com_Memset(params, 0, sizeof(params));

			// parse any event params before the start of this event's actions
			while ((token = COM_Parse(&pScript)) != NULL && (token[0] != '{'))
			{
				if (!token[0])
				{
					G_Error("G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine());
				}

				if (strlen(params))        // add a space between each param
				{
					Q_strcat(params, sizeof(params), " ");
				}
				Q_strcat(params, sizeof(params), token);
			}

			len = strlen(params);

			if (len)          // copy the params into the event
			{
				curEvent->params = G_Alloc(len + 1);
				Q_strncpyz(curEvent->params, params, len + 1);
			}

			// parse the actions for this event
			while ((token = COM_Parse(&pScript)) != NULL && (token[0] != '}'))
			{
				if (!token[0])
				{
					G_Error("G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine());
				}

				action = G_Script_ActionForString(token);
				if (!action)
				{
					G_Error("G_Script_ScriptParse(), Error (line %d): unknown action: %s.\n", COM_GetCurrentParseLine(), token);
				}

				curEvent->stack.items[curEvent->stack.numItems].action = action;

				Com_Memset(params, 0, sizeof(params));

				// Parse for {}'s if this is a set command
				if (action->hash == SET_HASH || action->hash == DELETE_HASH || action->hash == CREATE_HASH)
				{
					token = COM_Parse(&pScript);
					if (token[0] != '{')
					{
						COM_ParseError("'{' expected, found: %s.\n", token);
					}

					while ((token = COM_Parse(&pScript)) && (token[0] != '}'))
					{
						if (strlen(params))       // add a space between each param
						{
							Q_strcat(params, sizeof(params), " ");
						}

						if (strrchr(token, ' '))      // need to wrap this param in quotes since it has more than one word
						{
							Q_strcat(params, sizeof(params), "\"");
						}

						Q_strcat(params, sizeof(params), token);

						if (strrchr(token, ' '))      // need to wrap this param in quotes since it has mor
						{
							Q_strcat(params, sizeof(params), "\"");
						}
					}
				}
				else
				{
					token = COM_ParseExt(&pScript, qfalse);
					for (i = 0; token[0]; i++)
					{
						if (strlen(params))           // add a space between each param
						{
							Q_strcat(params, sizeof(params), " ");
						}

						if (i == 0)
						{
							// Special case: playsound's need to be cached on startup to prevent in-game pauses
							if (action->hash == PLAYSOUND_HASH)
							{
								G_SoundIndex(token);
							}
							else if (action->hash == CHANGEMODEL_HASH)
							{
								G_ModelIndex(token);
							}
							else if (buildScript && (action->hash == MU_START_HASH || action->hash == MU_PLAY_HASH || action->hash == MU_QUEUE_HASH))
							{
								if (strlen(token))           // we know there's a [0], but don't know if it's '0'
								{
									trap_SendServerCommand(-1, va("addToBuild %s\n", token));
								}
							}
						}

						if (i == 0 || i == 1)
						{
							if (action->hash == REMAPSHADER_HASH)
							{
								G_ShaderIndex(token);
							}
						}

						if (strrchr(token, ' '))          // need to wrap this param in quotes since it has more than one word
						{
							Q_strcat(params, sizeof(params), "\"");
						}

						Q_strcat(params, sizeof(params), token);

						if (strrchr(token, ' '))          // need to wrap this param in quotes since it has more than one word
						{
							Q_strcat(params, sizeof(params), "\"");
						}

						token = COM_ParseExt(&pScript, qfalse);
					}
				}

				if (strlen(params))       // copy the params into the event
				{
					curEvent->stack.items[curEvent->stack.numItems].params = G_Alloc(strlen(params) + 1);
					Q_strncpyz(curEvent->stack.items[curEvent->stack.numItems].params, params, strlen(params) + 1);
				}

				curEvent->stack.numItems++;

				if (curEvent->stack.numItems >= G_MAX_SCRIPT_STACK_ITEMS)
				{
					G_Error("G_Script_ScriptParse(): script exceeded G_MAX_SCRIPT_STACK_ITEMS (%d), line %d\n", G_MAX_SCRIPT_STACK_ITEMS, COM_GetCurrentParseLine());
				}
			}

			numEventItems++;
		}
		else     // skip this character completely
		{
			if (wantScript)
			{
				// note: There are maps available which don't start anymore
				//       Fix the mapscripts!
				G_Error("G_Script_ScriptParse(), Error (line %d): '{' expected, but found '%s'.\n", COM_GetCurrentParseLine(), token);
			}

			while ((token = COM_Parse(&pScript)) != NULL)
			{
				if (!token[0])
				{
					G_Error("G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine());
				}
				else if (token[0] == '{')
				{
					bracketLevel++;
				}
				else if (token[0] == '}')
				{
					if (!--bracketLevel)
					{
						break;
					}
				}
			}
		}
	}

	// alloc and copy the events into the gentity_t for this cast
	if (numEventItems > 0)
	{
		ent->scriptEvents = G_Alloc(sizeof(g_script_event_t) * numEventItems);
		Com_Memcpy(ent->scriptEvents, events, sizeof(g_script_event_t) * numEventItems);
		ent->numScriptEvents = numEventItems;
	}
}

/**
 * @brief G_Script_ScriptChange
 * @param[in,out] ent
 * @param[in] newScriptNum
 */
void G_Script_ScriptChange(gentity_t *ent, int newScriptNum)
{
	g_script_status_t scriptStatusBackup;

	// backup the current scripting
	Com_Memcpy(&scriptStatusBackup, &ent->scriptStatus, sizeof(g_script_status_t));

	// set the new script to this cast, and reset script status
	ent->scriptStatus.scriptEventIndex      = newScriptNum;
	ent->scriptStatus.scriptStackHead       = 0;
	ent->scriptStatus.scriptStackChangeTime = level.time;
	ent->scriptStatus.scriptId              = scriptStatusBackup.scriptId + 1;
	ent->scriptStatus.scriptFlags          |= SCFL_FIRST_CALL;

	// try and run the script, if it doesn't finish, then abort the current script (discard backup)
	if (G_Script_ScriptRun(ent)
	    &&  (ent->scriptStatus.scriptId == scriptStatusBackup.scriptId + 1))          // make sure we didnt change our script via a third party
	{   // completed successfully
		Com_Memcpy(&ent->scriptStatus, &scriptStatusBackup, sizeof(g_script_status_t));
		ent->scriptStatus.scriptFlags &= ~SCFL_FIRST_CALL;
	}
}

/**
 * @brief G_Script_GetEventIndex
 * @param[in] ent
 * @param[in] eventStr
 * @param[in] params
 * @return The event index within the entity for the specified event string extracted from G_Script_ScriptEvent.
 */
int G_Script_GetEventIndex(gentity_t *ent, const char *eventStr, const char *params)
{
	int i, eventNum = -1;
	int hash;

	hash = BG_StringHashValue_Lwr(eventStr);

	// find out which event this is
	for (i = 0; gScriptEvents[i].eventStr; i++)
	{
		if (gScriptEvents[i].hash == hash)       // match found
		{
			eventNum = i;
			break;
		}
	}

	// show debugging info
	if (g_scriptDebug.integer &&
	    (!g_scriptDebugTarget.string[0] || G_MatchString(g_scriptDebugTarget.string, ent->scriptName, qfalse)))
	{
		G_Printf("^7%i : (^5%s^7) ^9GScript Event: ^5%s %s\n", level.time, ent->scriptName ? ent->scriptName : "n/a", eventStr, params ? params : "");
	}

	if (eventNum < 0)
	{
		if (g_scriptDebug.integer)        // dev mode
		{
			G_Printf("^7%i : (^5%s^7) ^3Unknown Event: '%s'\n", level.time, ent->scriptName ? ent->scriptName : "n/a", eventStr);
		}
		return -1;
	}

	// see if this entity has this event
	for (i = 0; i < ent->numScriptEvents; i++)
	{
		if (ent->scriptEvents[i].eventNum == eventNum)
		{
			if ((!ent->scriptEvents[i].params) || (!gScriptEvents[eventNum].eventMatch || gScriptEvents[eventNum].eventMatch(&ent->scriptEvents[i], params)))
			{
				return i;
			}
		}
	}

	// show debugging info - this is missing in vanilla!
	// it'll show mappers explicit events called by ET mods - see goldrush script
	// note for scripters: ignore (n/a) GScript events and pain events for game objects f.e. "(tank) GScript event not found: pain 1199 1200"
	if (g_scriptDebug.integer)
	{
		G_Printf("^7%i : (^5%s^7) ^3GScript Event Not Handled: %s %s\n", level.time, ent->scriptName ? ent->scriptName : "n/a", eventStr, params ? params : "");
	}
	return -1;      // event not found/matched in this ent
}

/**
 * @brief An event has occured, for which a script may exist
 * @param[in] ent
 * @param[in] eventStr
 * @param[in] params
 */
void G_Script_ScriptEvent(gentity_t *ent, const char *eventStr, const char *params)
{
	int i;

	i = G_Script_GetEventIndex(ent, eventStr, params);

	if (i >= 0)
	{
		G_Script_ScriptChange(ent, i);
	}

	// log script trigger stolen & returned actions
	if (!Q_stricmp(eventStr, "trigger"))
	{
		if (!Q_stricmp(params, "stolen"))
		{
			G_LogPrintf("%s popup: ^7%s^7 stole \"%s\"\n", MODNAME,
			            Q_stricmp(ent->classname, "team_CTF_redflag") ? "axis" : "allies", ent->message);
		}
		else if (!Q_stricmp(params, "returned"))
		{
			G_LogPrintf("%s popup: ^7%s^7 returned \"%s\"\n", MODNAME,
			            Q_stricmp(ent->classname, "team_CTF_redflag") ? "allies" : "axis", ent->message);
		}
	}
/*
    // skip these
    if ( //!Q_stricmp(eventStr, "trigger") ||
        !Q_stricmp(eventStr, "activate") ||
        !Q_stricmp(eventStr, "spawn") ||
        !Q_stricmp(eventStr, "death") ||
        !Q_stricmp(eventStr, "pain") ||
        !Q_stricmp(eventStr, "playerstart"))
    {
        return;
    }
*/
	else if (!Q_stricmp(eventStr, "defused"))
	{
#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Defused at %s.", ent->parent ? ent->parent->track : ent->track), eventStr);
#endif
		// log script defused actions
		G_LogPrintf("%s popup: ^7%s^7 defused \"%s\"\n", MODNAME, params, ent->parent ? ent->parent->track : ent->track);
	}
	else if (!Q_stricmp(eventStr, "dynamited"))
	{
#ifdef FEATURE_OMNIBOT
		Bot_Util_SendTrigger(ent, NULL, va("Planted at %s.", ent->parent ? ent->parent->track : ent->track), eventStr);
#endif
		// log script dynamited actions
		G_LogPrintf("%s popup: ^7%s^7 planted \"%s\"\n", MODNAME, params, ent->parent ? ent->parent->track : ent->track);
	}
#ifdef FEATURE_OMNIBOT
	else if (!Q_stricmp(eventStr, "destroyed"))
	{
		Bot_Util_SendTrigger(ent, NULL, va("%s Destroyed.", ent->parent ? ent->parent->track : ent->track), eventStr);
	}
	else if (!Q_stricmp(eventStr, "exploded"))
	{
		Bot_Util_SendTrigger(ent, NULL, va("Explode_%s Exploded.", _GetEntityName(ent)), eventStr);
	}
#endif
}

/**
 * @brief G_Script_ScriptRun
 * @param[in,out] ent
 * @return qtrue if the script completed
 */
qboolean G_Script_ScriptRun(gentity_t *ent)
{
	g_script_stack_t *stack;
	int              oldScriptId;

	if (!ent->scriptEvents)
	{
		ent->scriptStatus.scriptEventIndex = -1;
		return qtrue;
	}

	// if we are still doing a gotomarker, process the movement
	if (ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER)
	{
		G_ScriptAction_GotoMarker(ent, NULL);
	}

	// if we are animating, do the animation
	if (ent->scriptStatus.scriptFlags & SCFL_ANIMATING)
	{
		G_ScriptAction_PlayAnim(ent, ent->scriptStatus.animatingParams);
	}

	if (ent->scriptStatus.scriptEventIndex < 0)
	{
		return qtrue;
	}

	stack = &ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack;

	if (!stack->numItems)
	{
		ent->scriptStatus.scriptEventIndex = -1;
		return qtrue;
	}

	// show debugging info
	if (g_scriptDebug.integer && ent->scriptStatus.scriptStackChangeTime == level.time &&
	    (!g_scriptDebugTarget.string[0] || G_MatchString(g_scriptDebugTarget.string, ent->scriptName, qfalse)))
	{
		if (ent->scriptStatus.scriptStackHead < stack->numItems)
		{
			G_Printf("^7%i : (^5%s^7) ^9GScript Action: ^d%s %s\n", level.time, ent->scriptName, stack->items[ent->scriptStatus.scriptStackHead].action->actionString, (stack->items[ent->scriptStatus.scriptStackHead].params ? stack->items[ent->scriptStatus.scriptStackHead].params : ""));
		}
	}

	while (ent->scriptStatus.scriptStackHead < stack->numItems)
	{
		oldScriptId = ent->scriptStatus.scriptId;
		if (!stack->items[ent->scriptStatus.scriptStackHead].action->actionFunc(ent, stack->items[ent->scriptStatus.scriptStackHead].params))
		{
			ent->scriptStatus.scriptFlags &= ~SCFL_FIRST_CALL;
			return qfalse;
		}
		// if the scriptId changed, a new event was triggered in our scripts which did not finish
		if (oldScriptId != ent->scriptStatus.scriptId)
		{
			return qfalse;
		}
		// move to the next action in the script
		ent->scriptStatus.scriptStackHead++;
		// record the time that this new item became active
		ent->scriptStatus.scriptStackChangeTime = level.time;
		//
		ent->scriptStatus.scriptFlags |= SCFL_FIRST_CALL;
		// show debugging info
		if (g_scriptDebug.integer &&
		    (!g_scriptDebugTarget.string[0] || G_MatchString(g_scriptDebugTarget.string, ent->scriptName, qfalse)))
		{
			if (ent->scriptStatus.scriptStackHead < stack->numItems)
			{
				G_Printf("^7%i : (^5%s^7) ^9GScript Action: ^d%s %s\n", level.time, ent->scriptName, stack->items[ent->scriptStatus.scriptStackHead].action->actionString, (stack->items[ent->scriptStatus.scriptStackHead].params ? stack->items[ent->scriptStatus.scriptStackHead].params : ""));
			}
		}
	}

	ent->scriptStatus.scriptEventIndex = -1;

	return qtrue;
}

//================================================================================
// Script Entities

/**
 * @brief mountedmg42_fire
 * @param[in,out] other
 */
void mountedmg42_fire(gentity_t *other)
{
	vec3_t    forward, right, up;
	vec3_t    muzzle;
	gentity_t *self;

	if (!(self = other->tankLink))
	{
		return;
	}

	AngleVectors(other->client->ps.viewangles, forward, right, up);
	VectorCopy(other->s.pos.trBase, muzzle);
	//VectorMA( muzzle, 42, up, muzzle );
	muzzle[2] += other->client->ps.viewheight;
	VectorMA(muzzle, 58, forward, muzzle);

	SnapVector(muzzle);

	// ent & activator are same for Fire_Lead_Ext
	//self->s.eFlags  |= EF_MG42_ACTIVE;
	other->s.eFlags |= EF_MG42_ACTIVE;

#ifdef FEATURE_LUA
	if (!G_LuaHook_MountedMGFire(other->s.number))
#endif
	{
		if (self->s.density & 8)
		{
			Fire_Lead_Ext(other, other, GetWeaponTableData(WP_DUMMY_MG42)->spread, GetWeaponTableData(WP_DUMMY_MG42)->damage, muzzle, forward, right, up, MOD_BROWNING);
		}
		else
		{
			Fire_Lead_Ext(other, other, GetWeaponTableData(WP_DUMMY_MG42)->spread, GetWeaponTableData(WP_DUMMY_MG42)->damage, muzzle, forward, right, up, MOD_MG42);
		}
	}
}

/**
 * @brief script_linkentity
 * @param[in] ent
 */
void script_linkentity(gentity_t *ent)
{
	// this is required since non-solid brushes need to be linked but not solid
	trap_LinkEntity(ent);
}

/**
 * Script mover flags
 */

#define SMF_TRIGGERSPAWN        1
#define SMF_SOLID               2
#define SMF_EXPLOSIVEDAMAGEONLY 4
#define SMF_RESURRECTABLE       8
#define SMF_COMPASS             16
#define SMF_ALLIES              32
#define SMF_AXIS                64
#define SMF_MOUNTED_GUN         128
#define SMF_DENSITY             256

/**
 * @brief script_mover_die
 * @param[in,out] self
 * @param inflictor - unused
 * @param attacker  - unused
 * @param damage    - unused
 * @param mod       - unused
 */
void script_mover_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod)
{
	G_Script_ScriptEvent(self, "death", "");

	if (!(self->spawnflags & SMF_RESURRECTABLE))
	{
		G_FreeEntity(self);
	}

	if (self->tankLink)
	{
		G_LeaveTank(self->tankLink, qtrue);
	}

	self->die = NULL;
}

/**
 * @brief script_mover_think
 * @param[in,out] ent
 */
void script_mover_think(gentity_t *ent)
{
	if (ent->spawnflags & SMF_MOUNTED_GUN)
	{
		if (!ent->tankLink)
		{
			if (ent->mg42weapHeat)
			{
				ent->mg42weapHeat -= (300.f * 100 * 0.001f);

				if (ent->mg42weapHeat < 0)
				{
					ent->mg42weapHeat = 0;
				}
			}
			if (ent->backupWeaponTime)
			{
				ent->backupWeaponTime -= 100;
				if (ent->backupWeaponTime < 0)
				{
					ent->backupWeaponTime = 0;
				}
			}
		}
	}

	ent->nextthink = level.time + FRAMETIME;
}

/**
 * @brief script_mover_spawn
 * @param[in,out] ent
 */
void script_mover_spawn(gentity_t *ent)
{
	if (ent->spawnflags & SMF_MOUNTED_GUN)
	{
		if (ent->tagBuffer[0] == '\0')
		{
			ent->nextTrain = ent;
		}
		else
		{
			gentity_t *tent;

			tent = G_FindByTargetname(&g_entities[MAX_CLIENTS - 1], ent->tagBuffer);

			if (!tent)
			{
				ent->nextTrain = ent;
			}
			else
			{
				ent->nextTrain = tent;
			}
		}

		ent->s.effect3Time = ent->nextTrain - g_entities;
	}

	if (ent->spawnflags & SMF_SOLID)
	{
		ent->clipmask   = CONTENTS_SOLID;
		ent->r.contents = CONTENTS_SOLID;
	}
	else
	{
		ent->s.eFlags  |= EF_NONSOLID_BMODEL;
		ent->clipmask   = 0;
		ent->r.contents = 0;
	}

	script_linkentity(ent);
	ent->think     = script_mover_think;
	ent->nextthink = level.time + 200;
}

/**
 * @brief script_mover_use
 * @param[in] ent
 * @param other     - unused
 * @param activator - unused
 */
void script_mover_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (ent->spawnflags & SMF_RESURRECTABLE)
	{
		if (ent->count)
		{
			ent->health         = ent->count;
			ent->s.dl_intensity = ent->health;

			G_Script_ScriptEvent(ent, "rebirth", "");

			ent->die = script_mover_die;

			ent->think     = script_mover_think;
			ent->nextthink = level.time + 200;
		}
	}
	else
	{
		script_mover_spawn(ent);
	}
}

/**
 * @brief script_mover_blocked
 * @param[in] ent
 * @param[in] other
 */
void script_mover_blocked(gentity_t *ent, gentity_t *other)
{
	// remove it, we must not stop for anything or it will screw up script timing
	if (!other->client && other->s.eType != ET_CORPSE)
	{
		// except CTF flags!!!!
		if (other->s.eType == ET_ITEM && other->item->giType == IT_TEAM)
		{
			Team_DroppedFlagThink(other);
			return;
		}
		G_FreeEntity(other);
		return;
	}

	// FIXME: we could have certain entities stop us, thereby "pausing" movement
	// until they move out the way. then we can just call the GotoMarker() again,
	// telling it that we are just now calling it for the first time, so it should
	// start us on our way again (theoretically speaking).

	// kill them
	G_Damage(other, ent, ent, NULL, NULL, GIB_DAMAGE(other->health), 0, MOD_CRUSH);
}

/**
 * @brief Scripted brush entity. A simplified means of moving brushes around based on events.
 *
 * @details
 * QUAKED script_mover (0.5 0.25 1.0) ? TRIGGERSPAWN SOLID EXPLOSIVEDAMAGEONLY RESURECTABLE COMPASS ALLIED AXIS MOUNTED_GUN
 *
 * "modelscale" - Scale multiplier (defaults to 1, and scales uniformly)
 * "modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both the "modelscale" is ignored
 * "model2" optional md3 to draw over the solid clip brush
 * "scriptname" name used for scripting purposes (like aiName in AI scripting)
 * "health" optionally make this entity damagable
 * "description" used with health, if the entity is damagable, it draws a healthbar with this description above it.
 *
 * @param[in,out] ent
 */
void SP_script_mover(gentity_t *ent)
{
	float  scale[3] = { 1, 1, 1 };
	vec3_t scalevec;
	char   tagname[MAX_QPATH];
	char   *modelname;

	if (!ent->model)
	{
		G_Error("script_mover entity #%i must have a \"model\"\n", ent->s.number);
	}
	if (!ent->scriptName)
	{
		G_Error("script_mover entity #%i must have a \"scriptname\"\n", ent->s.number);
	}
	ent->blocked = script_mover_blocked;

	// first position at start
	VectorCopy(ent->s.origin, ent->pos1);

	//VectorCopy( ent->r.currentOrigin, ent->pos1 );
	VectorCopy(ent->pos1, ent->pos2);   // don't go anywhere just yet

	trap_SetBrushModel(ent, ent->model);

	InitMover(ent);
	ent->reached        = NULL;
	ent->s.animMovetype = 0;

	ent->s.density = 0;

	if (ent->spawnflags & SMF_DENSITY)
	{
		ent->s.density |= 2;
	}

	if (ent->spawnflags & SMF_RESURRECTABLE)
	{
		ent->use = script_mover_use;
	}

	if (ent->spawnflags & SMF_COMPASS)
	{
		ent->s.time2 = 1;
	}
	else
	{
		ent->s.time2 = 0;
	}

	if (ent->spawnflags & SMF_ALLIES)
	{
		ent->s.teamNum = TEAM_ALLIES;
	}
	else if (ent->spawnflags & SMF_AXIS)
	{
		ent->s.teamNum = TEAM_AXIS;
	}
	else
	{
		ent->s.teamNum = TEAM_FREE;
	}

	if (ent->spawnflags & SMF_TRIGGERSPAWN)
	{
		ent->use = script_mover_use;
		trap_UnlinkEntity(ent);   // make sure it's not visible
		return;
	}

	G_SetAngle(ent, ent->s.angles);

	G_SpawnInt("health", "0", &ent->health);
	if (ent->health)
	{
		char cs[MAX_INFO_STRING];
		char *s;

		ent->takedamage = qtrue;
		ent->count      = ent->health;

		// client needs to know about it as well
		ent->s.effect1Time  = ent->count;
		ent->s.dl_intensity = 255;

		if (G_SpawnString("description", "", &s))
		{
			trap_GetConfigstring(CS_SCRIPT_MOVER_NAMES, cs, sizeof(cs));
			Info_SetValueForKey(cs, va("%i", (int)(ent - g_entities)), s);
			trap_SetConfigstring(CS_SCRIPT_MOVER_NAMES, cs);
		}
	}
	else
	{
		ent->count = 0;
	}

	ent->die = script_mover_die;

	// look for general scaling
	if (G_SpawnFloat("modelscale", "1", &scale[0]))
	{
		scale[2] = scale[1] = scale[0];
	}

	if (G_SpawnString("model2", "", &modelname))
	{
		COM_StripExtension(modelname, tagname, sizeof(tagname));
		Q_strcat(tagname, MAX_QPATH, ".tag");

		ent->tagNumber = trap_LoadTag(tagname);

		//if( !(ent->tagNumber = trap_LoadTag( tagname )) ) {
		//  Com_Error( ERR_DROP, "Failed to load Tag File (%s)", tagname );
		//}
	}

	// look for axis specific scaling
	if (G_SpawnVector("modelscale_vec", "1 1 1", &scalevec[0]))
	{
		VectorCopy(scalevec, scale);
	}

	if (scale[0] != 1.f || scale[1] != 1.f || scale[2] != 1.f)
	{
		ent->s.density |= 1;
		// scale is stored in 'angles2'
		VectorCopy(scale, ent->s.angles2);
	}

	if (ent->spawnflags & SMF_MOUNTED_GUN)
	{
		char *tagent;

		ent->s.density |= 4;
		ent->waterlevel = 0;

		if (G_SpawnString("gun", "", &modelname))
		{
			if (!Q_stricmp(modelname, "browning"))
			{
				ent->s.density |= 8;
			}
		}

		G_SpawnString("tagent", "", &tagent);
		Q_strncpyz(ent->tagBuffer, tagent, 32);
		ent->s.powerups = -1;
	}

	ent->think     = script_mover_spawn;
	ent->nextthink = level.time + FRAMETIME;
}

//..............................................................................

/**
 * Script model med spawn flags
 */
#define SMMSF_TRIGGERSPAWN 1
#define SMMSF_SOLID        2

/**
 * @brief script_model_med_spawn
 * @param[in,out] ent
 */
void script_model_med_spawn(gentity_t *ent)
{
	if (ent->spawnflags & SMMSF_SOLID)
	{
		ent->clipmask   = CONTENTS_SOLID;
		ent->r.contents = CONTENTS_SOLID;
	}
	ent->s.eType = ET_GENERAL;

	ent->s.modelindex = G_ModelIndex(ent->model);
	ent->s.frame      = 0;

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	ent->s.pos.trType = TR_STATIONARY;

	trap_LinkEntity(ent);
}

/**
 * @brief script_model_med_use
 * @param[in] ent
 * @param other     - unused
 * @param activator - unused
 */
void script_model_med_use(gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	script_model_med_spawn(ent);
}

/**
 * @brief SP_script_model_med
 *
 * @details QUAKED script_model_med (0.5 0.25 1.0) (-16 -16 -24) (16 16 64) TriggerSpawn Solid
 * MEDIUM SIZED scripted entity, used for animating a model, moving it around, etc
 * SOLID spawnflag means this entity will clip the player and AI, otherwise they can walk
 * straight through it
 * "model" the full path of the model to use
 * "scriptname" name used for scripting purposes (like aiName in AI scripting)
 *
 * @param[in,out] ent
 */
void SP_script_model_med(gentity_t *ent)
{
	if (!ent->model)
	{
		G_Error("script_model_med entity #%i must have a \"model\"\n", ent->s.number);
	}
	if (!ent->scriptName)
	{
		G_Error("script_model_med entity #%i must have a \"scriptname\"\n", ent->s.number);
	}

	ent->s.eType           = ET_GENERAL;
	ent->s.apos.trType     = TR_STATIONARY;
	ent->s.apos.trTime     = 0;
	ent->s.apos.trDuration = 0;
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorClear(ent->s.apos.trDelta);

	if (ent->spawnflags & SMMSF_TRIGGERSPAWN)
	{
		ent->use = script_model_med_use;
		trap_UnlinkEntity(ent);   // make sure it's not visible
		return;
	}

	script_model_med_spawn(ent);
}

//..............................................................................

/**
 * @brief This is a camera entity.
 *
 * @details QUAKED script_camera (1.0 0.25 1.0) (-8 -8 -8) (8 8 8) TriggerSpawn
 *
 * Used by the scripting to show cinematics, via special
 * camera commands. See scripting documentation.
 *
 * "scriptname" name used for scripting purposes (like aiName in AI scripting)
 *
 * @param[in,out] ent
 *
 * @note Unused
 */
void SP_script_camera(gentity_t *ent)
{
	if (!ent->scriptName)
	{
		G_Error("%s must have a \"scriptname\"\n", ent->classname);
	}

	ent->s.eType           = ET_CAMERA;
	ent->s.apos.trType     = TR_STATIONARY;
	ent->s.apos.trTime     = 0;
	ent->s.apos.trDuration = 0;
	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorClear(ent->s.apos.trDelta);

	ent->s.frame = 0;

	ent->r.svFlags |= SVF_NOCLIENT;     // only broadcast when in use
}

/**
 * @brief This is used to script multiplayer maps. Entity not displayed in game.
 *
 * @details QUAKED script_multiplayer (1.0 0.25 1.0) (-8 -8 -8) (8 8 8)
 *
 * "scriptname" name used for scripting purposes (REQUIRED)
 *
 * @param[in,out] ent
 *
 * @note Also storing some stuff that will change often but needs to be broadcast, so we dont want to use a configstring
 */
void SP_script_multiplayer(gentity_t *ent)
{
	ent->scriptName = "game_manager";

	// broadcasting this to clients now, should be cheaper in bandwidth for sending landmine info
	ent->s.eType   = ET_GAMEMANAGER;
	ent->r.svFlags = SVF_BROADCAST;

	if (level.gameManager)
	{
		G_Error("^1ERROR: multiple script_multiplayers found^7\n");
	}
	level.gameManager                    = ent;
	level.gameManager->s.otherEntityNum  = team_maxLandmines.integer;  // axis landmine count
	level.gameManager->s.otherEntityNum2 = team_maxLandmines.integer;  // allies landmine count
	level.gameManager->s.modelindex      = qfalse; // axis HQ doesn't exist
	level.gameManager->s.modelindex2     = qfalse; // allied HQ doesn't exist

	trap_LinkEntity(ent);
}
