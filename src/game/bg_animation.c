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
 * @file bg_animation.c
 * @brief Incorporates several elements related to the new flexible animation
 *        system.
 *
 * This includes scripting elements, support routines for new animation set
 * reference system and other bits and pieces.
 */

#include "../qcommon/q_shared.h"
#include "bg_public.h"

// added because I need to check single/multiplayer instances and branch accordingly
#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_gametype;
#endif

// debug defines, to prevent doing costly string cvar lookups
//#define   DBGANIMS
//#define   DBGANIMEVENTS

// this is used globally within this file to reduce redundant params
static animScriptData_t *globalScriptData = NULL;

#define MAX_ANIM_DEFINES    16

static const char *globalFilename;    // to prevent redundant params

// these are used globally during script parsing
static int              numDefines[NUM_ANIM_CONDITIONS];
static char             defineStrings[10000]; // stores the actual strings
static unsigned int     defineStringsOffset;
static animStringItem_t defineStr[NUM_ANIM_CONDITIONS][MAX_ANIM_DEFINES];
static int              defineBits[NUM_ANIM_CONDITIONS][MAX_ANIM_DEFINES][2];

static scriptAnimMoveTypes_t parseMovetype;
static int                   parseEvent;

static animStringItem_t weaponStrings[WP_NUM_WEAPONS];

animStringItem_t animStateStr[] =
{
	{ "RELAXED", -1 },
	{ "QUERY",   -1 },
	{ "ALERT",   -1 },
	{ "COMBAT",  -1 },

	{ NULL,      -1 },
};

static animStringItem_t animMoveTypesStr[] =
{
	{ "** UNUSED **", -1 }, // 0
	{ "IDLE",         -1 },
	{ "IDLECR",       -1 },
	{ "WALK",         -1 },
	{ "WALKBK",       -1 },
	{ "WALKCR",       -1 },
	{ "WALKCRBK",     -1 },
	{ "RUN",          -1 },
	{ "RUNBK",        -1 },
	{ "SWIM",         -1 },
	{ "SWIMBK",       -1 }, // 10
	{ "STRAFERIGHT",  -1 },
	{ "STRAFELEFT",   -1 },
	{ "TURNRIGHT",    -1 },
	{ "TURNLEFT",     -1 },
	{ "CLIMBUP",      -1 },
	{ "CLIMBDOWN",    -1 },
	{ "FALLEN",       -1 }, // dead, before limbo
	{ "PRONE",        -1 },
	{ "PRONEBK",      -1 },
	{ "IDLEPRONE",    -1 }, // 20
	{ "FLAILING",     -1 },

	{ "RADIO",        -1 },
	{ "RADIOCR",      -1 },
	{ "RADIOPRONE",   -1 },

	{ "DEAD",         -1 },

	{ NULL,           -1 },
};

animStringItem_t animEventTypesStr[] =
{
	{ "PAIN",                       -1 },
	{ "DEATH",                      -1 },
	{ "FIREWEAPON",                 -1 },
	{ "FIREWEAPON2",                -1 },
	{ "JUMP",                       -1 },
	{ "JUMPBK",                     -1 },
	{ "LAND",                       -1 }, // used, but not defined in script
	{ "DROPWEAPON",                 -1 }, // used, but not defined in script
	{ "RAISEWEAPON",                -1 },
	{ "CLIMBMOUNT",                 -1 },
	{ "CLIMBDISMOUNT",              -1 },
	{ "RELOAD",                     -1 },
	{ "REVIVE",                     -1 },
	{ "DO_ALT_WEAPON_MODE",         -1 },
	{ "UNDO_ALT_WEAPON_MODE",       -1 },
	{ "DO_ALT_WEAPON_MODE_PRONE",   -1 },
	{ "UNDO_ALT_WEAPON_MODE_PRONE", -1 },
	{ "FIREWEAPONPRONE",            -1 },
	{ "FIREWEAPON2PRONE",           -1 },
	{ "RAISEWEAPONPRONE",           -1 },
	{ "RELOADPRONE",                -1 },
	{ "NOPOWER",                    -1 },

	{ NULL,                         -1 },
};

animStringItem_t animBodyPartsStr[] =
{
	{ "** UNUSED **", -1 },
	{ "LEGS",         -1 },
	{ "TORSO",        -1 },
	{ "BOTH",         -1 },

	{ NULL,           -1 },
};

//------------------------------------------------------------
// conditions

static animStringItem_t animConditionPositionsStr[] =
{
	{ "** UNUSED **", -1 },
	{ "BEHIND",       -1 },
	{ "INFRONT",      -1 },
	{ "RIGHT",        -1 },
	{ "LEFT",         -1 },

	{ NULL,           -1 },
};

static animStringItem_t animConditionMountedStr[] =
{
	{ "** UNUSED **", -1 },
	{ "MG42",         -1 },

	{ NULL,           -1 },
};

static animStringItem_t animConditionLeaningStr[] =
{
	{ "** UNUSED **", -1 },
	{ "RIGHT",        -1 },
	{ "LEFT",         -1 },

	{ NULL,           -1 },
};

static animStringItem_t animConditionImpactPointsStr[] =
{
	{ "** UNUSED **",   -1 },
	{ "HEAD",           -1 },
	{ "CHEST",          -1 },
	{ "GUT",            -1 },
	{ "GROIN",          -1 },
	{ "SHOULDER_RIGHT", -1 },
	{ "SHOULDER_LEFT",  -1 },
	{ "KNEE_RIGHT",     -1 },
	{ "KNEE_LEFT",      -1 },

	{ NULL,             -1 },
};

static animStringItem_t animEnemyTeamsStr[] =
{
	{ "NAZI",    -1 },
	{ "ALLIES",  -1 },
	{ "MONSTER", -1 },
	{ "SPARE1",  -1 },
	{ "SPARE2",  -1 },
	{ "SPARE3",  -1 },
	{ "SPARE4",  -1 },
	{ "NEUTRAL", -1 }
};

static animStringItem_t animHealthLevelStr[] =
{
	{ "1", -1 },
	{ "2", -1 },
	{ "3", -1 },
};

static animStringItem_t animFlailTypeStr[] =
{
	{ "** UNUSED **", -1 },
	{ "INAIR",        -1 },
	{ "VCRASH",       -1 },
	{ "HCRASH",       -1 },

	{ NULL,           -1 },
};

static animStringItem_t animGenBitFlagStr[] =
{
	{ "ZOOMING", -1 },   // zooming with binoculars
	{ "HOLDING", -1 },
};

// sorts of duplicate aistateEnum_t.
static animStringItem_t animAIStateStr[] =
{
	{ "RELAXED", -1 },
	{ "QUERY",   -1 },
	{ "ALERT",   -1 },
	{ "COMBAT",  -1 }
};

typedef enum
{
	ANIM_CONDTYPE_BITFLAGS = 0,
	ANIM_CONDTYPE_VALUE,

	NUM_ANIM_CONDTYPES
} animScriptConditionTypes_t;

typedef struct
{
	animScriptConditionTypes_t type;
	animStringItem_t *values;
} animConditionTable_t;

static animStringItem_t animConditionsStr[NUM_ANIM_CONDITIONS + 1] =
{
	{ "WEAPONS",        -1 },
	{ "ENEMY_POSITION", -1 },
	{ "ENEMY_WEAPON",   -1 },
	{ "UNDERWATER",     -1 },
	{ "MOUNTED",        -1 },
	{ "MOVETYPE",       -1 },
	{ "UNDERHAND",      -1 },
	{ "LEANING",        -1 },
	{ "IMPACT_POINT",   -1 },
	{ "CROUCHING",      -1 },
	{ "STUNNED",        -1 },
	{ "FIRING",         -1 },
	{ "SHORT_REACTION", -1 },
	{ "ENEMY_TEAM",     -1 },
	{ "PARACHUTE",      -1 },
	{ "CHARGING",       -1 },
	{ "SECONDLIFE",     -1 },
	{ "HEALTH_LEVEL",   -1 },
	{ "FLAILING_TYPE",  -1 },
	{ "GEN_BITFLAG",    -1 },
	{ "AISTATE",        -1 },
	{ "SUICIDE",        -1 },

	{ NULL,             -1 },
};

static animConditionTable_t animConditionsTable[NUM_ANIM_CONDITIONS] =
{
	{ ANIM_CONDTYPE_BITFLAGS, weaponStrings                },
	{ ANIM_CONDTYPE_BITFLAGS, animConditionPositionsStr    },
	{ ANIM_CONDTYPE_BITFLAGS, weaponStrings                },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    animConditionMountedStr      },
	{ ANIM_CONDTYPE_BITFLAGS, animMoveTypesStr             },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    animConditionLeaningStr      },
	{ ANIM_CONDTYPE_VALUE,    animConditionImpactPointsStr },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    animEnemyTeamsStr            },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
	{ ANIM_CONDTYPE_VALUE,    animHealthLevelStr           },
	{ ANIM_CONDTYPE_VALUE,    animFlailTypeStr             },
	{ ANIM_CONDTYPE_BITFLAGS, animGenBitFlagStr            },
	{ ANIM_CONDTYPE_VALUE,    animAIStateStr               },
	{ ANIM_CONDTYPE_VALUE,    NULL                         },
};

//------------------------------------------------------------

/**
 * @brief BG_StringHashValue
 * @param[in] fname
 * @return A hash value for the given string
 */
long BG_StringHashValue(const char *fname)
{
	int  i;
	long hash;

	if (!fname)
	{
		return -1;
	}

	hash = 0;
	i    = 0;
	while (fname[i] != '\0')
	{
		if (Q_isupper(fname[i]))
		{
			hash += (long)(fname[i] + ('a' - 'A')) * (i + 119);
		}
		else
		{
			hash += (long)(fname[i]) * (i + 119);
		}

		i++;
	}
	if (hash == -1)
	{
		hash = 0;   // never return -1
		Com_Printf("BG_StringHash WARNING: fname with empty string returning 0");
	}
	return hash;
}

/**
 * @brief BG_StringHashValue_Lwr
 * @param[in] fname
 * @return A hash value for the given string (make sure the strings and lowered first)
 */
long BG_StringHashValue_Lwr(const char *fname)
{
	int  i    = 0;
	long hash = 0;

	while (fname[i] != '\0')
	{
		hash += (long)(fname[i]) * (i + 119);
		i++;
	}
	if (hash == -1)
	{
		hash = 0;   // never return -1
	}
	return hash;
}

/**
 * @brief BG_AnimParseError
 * @param[in] msg
 */
void _attribute((noreturn)) QDECL BG_AnimParseError(const char *msg, ...)
{
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	if (globalFilename)
	{
		Com_Error(ERR_DROP, "%s: (%s, line %i)", text, globalFilename, COM_GetCurrentParseLine() + 1);
	}
	else
	{
		Com_Error(ERR_DROP, "%s", text);
	}
}

/**
 * @brief BG_AnimationIndexForString
 * @param[in] string
 * @param[in] animModelInfo
 * @return
 */
static int BG_AnimationIndexForString(char *string, animModelInfo_t *animModelInfo)
{
	int         i, hash = BG_StringHashValue(string);
	animation_t *anim;

	for (i = 0; i < animModelInfo->numAnimations; i++)
	{
		anim = animModelInfo->animations[i];
		if ((hash == anim->nameHash) && !Q_stricmp(string, anim->name))
		{
			// found a match
			return i;
		}
	}
	// no match found
	BG_AnimParseError("BG_AnimationIndexForString: unknown index '%s' for animation group '%s'", string, animModelInfo->animationGroup);
	return -1;  // shutup compiler
}

/*
 * @brief BG_AnimationForString
 * @param[in] string
 * @param[in] animModelInfo
 * @return
 *
 * @note Unused
animation_t *BG_AnimationForString(char *string, animModelInfo_t *animModelInfo)
{
    int         i, hash = BG_StringHashValue(string);
    animation_t *anim;

    for (i = 0; i < animModelInfo->numAnimations; i++)
    {
        anim = animModelInfo->animations[i];
        if ((hash == anim->nameHash) && !Q_stricmp(string, anim->name))
        {
            // found a match
            return anim;
        }
    }
    // no match found
    Com_Error(ERR_DROP, "BG_AnimationForString: unknown animation '%s' for animation group '%s'", string, animModelInfo->animationGroup);
    return NULL;
}
*/

/**
 * @brief errors out if no match found
 * @param[in] token
 * @param[in] strings
 * @param[in] allowFail
 * @return
 */
int BG_IndexForString(char *token, animStringItem_t *strings, qboolean allowFail)
{
	int              i, hash = BG_StringHashValue(token);
	animStringItem_t *strav;

	for (i = 0, strav = strings; strav->string; strav++, i++)
	{
		if (strav->hash == -1)
		{
			strav->hash = BG_StringHashValue(strav->string);
		}
		if ((hash == strav->hash) && !Q_stricmp(token, strav->string))
		{
			// found a match
			return i;
		}
	}
	// no match found
	if (!allowFail)
	{
		BG_AnimParseError("BG_IndexForString: unknown token '%s'", token);
	}

	return -1;
}

/**
 * @brief BG_CopyStringIntoBuffer
 * @param[in] string
 * @param[in] buffer
 * @param[in] bufSize
 * @param[in,out] offset
 * @return
 */
char *BG_CopyStringIntoBuffer(char *string, char *buffer, unsigned int bufSize, unsigned int *offset)
{
	char *pch;

	// check for overloaded buffer
	if (*offset + strlen(string) + 1 >= bufSize)
	{
		BG_AnimParseError("BG_CopyStringIntoBuffer: out of buffer space");
	}

	pch = &buffer[*offset];

	// safe to do a strcpy since we've already checked for overrun
	strcpy(pch, string);

	// move the offset along
	*offset += strlen(string) + 1;

	return pch;
}

/**
 * @brief Builds the list of weapon names from the item list. This is done here rather
 * than hardcoded to ease the process of modifying the weapons.
 */
void BG_InitWeaponStrings(void)
{
	weapon_t i;
	gitem_t  *item;

	Com_Memset(weaponStrings, 0, sizeof(weaponStrings));

	for (i = WP_NONE; i < WP_NUM_WEAPONS; i++)
	{
		// find this weapon in the itemslist, and extract the name
		item = BG_GetItem(GetWeaponTableData(i)->item);

		if (item && item->classname && item->giType == IT_WEAPON && item->giWeapon == i)
		{
			weaponStrings[i].string = item->pickup_name;
		}
		else
		{
			weaponStrings[i].string = "(unknown)";
		}

		weaponStrings[i].hash = BG_StringHashValue(weaponStrings[i].string);
	}
}

#define RESULT_SIZE 2
/**
 * @brief Convert the string into a single int containing bit flags, stopping at a ',' or end of line
 * @param[in,out] text_pp
 * @param[in] stringTable
 * @param[in] condIndex
 * @param[out] result
 */
void BG_ParseConditionBits(char **text_pp, animStringItem_t *stringTable, int condIndex, int result[RESULT_SIZE])
{
	qboolean endFlag = qfalse;
	int      indexFound;
	int      tempBits[2];
	char     currentString[MAX_QPATH];
	qboolean minus = qfalse;
	char     *token;

	currentString[0] = '\0';
	Com_Memset(result, 0, sizeof(result[0]) * RESULT_SIZE);
	Com_Memset(tempBits, 0, sizeof(tempBits));

	while (!endFlag)
	{
		token = COM_ParseExt(text_pp, qfalse);
		if (!token[0])
		{
			COM_RestoreParseSession(text_pp);   // go back to the previous token
			endFlag = qtrue;    // done parsing indexes
			if (!strlen(currentString))
			{
				break;
			}
		}

		if (!Q_stricmp(token, ","))
		{
			endFlag = qtrue;    // end of indexes
		}

		if (!Q_stricmp(token, "none"))       // first bit is always the "unused" bit
		{
			COM_BitSet(result, 0);
			continue;
		}

		if (!Q_stricmp(token, "none,"))          // first bit is always the "unused" bit
		{
			COM_BitSet(result, 0);
			endFlag = qtrue;    // end of indexes
			continue;
		}

		if (!Q_stricmp(token, "NOT"))
		{
			token = "MINUS"; // NOT is equivalent to MINUS
		}

		if (!endFlag && Q_stricmp(token, "AND") && Q_stricmp(token, "MINUS"))         // must be a index
		{   // check for a comma (end of indexes)
			if (token[strlen(token) - 1] == ',')
			{
				endFlag                  = qtrue;
				token[strlen(token) - 1] = '\0';
			}
			// append this to the currentString
			if (strlen(currentString))
			{
				Q_strcat(currentString, sizeof(currentString), " ");
			}
			Q_strcat(currentString, sizeof(currentString), token);
		}

		if (!Q_stricmp(token, "AND") || !Q_stricmp(token, "MINUS") || endFlag)
		{
			// process the currentString
			if (!strlen(currentString))
			{
				if (endFlag)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected end of condition");
				}
				else
				{
					// check for minus indexes to follow
					if (!Q_stricmp(token, "MINUS"))
					{
						minus = qtrue;
						continue;
					}
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}
			}
			if (!Q_stricmp(currentString, "all"))
			{
				tempBits[0] = ~0x0;
				tempBits[1] = ~0x0;
			}
			else
			{
				// first check this string with our defines
				indexFound = BG_IndexForString(currentString, defineStr[condIndex], qtrue);
				if (indexFound >= 0)
				{
					// we have precalculated the bitflags for the defines
					tempBits[0] = defineBits[condIndex][indexFound][0];
					tempBits[1] = defineBits[condIndex][indexFound][1];
				}
				else
				{
					// convert the string into an index
					indexFound = BG_IndexForString(currentString, stringTable, qfalse);
					// convert the index into a bitflag
					COM_BitSet(tempBits, indexFound);
				}
			}
			// perform operation
			if (minus)        // subtract
			{
				result[0] &= ~tempBits[0];
				result[1] &= ~tempBits[1];
			}
			else            // add
			{
				result[0] |= tempBits[0];
				result[1] |= tempBits[1];
			}
			// clear the currentString
			currentString[0] = '\0';
			// check for minus indexes to follow
			if (!Q_stricmp(token, "MINUS"))
			{
				minus = qtrue;
			}
		}
	}
}

/**
 * @brief BG_ParseConditions
 * @param[in] text_pp
 * @param[out] scriptItem
 * @return qtrue if everything went ok, error drops otherwise
 */
qboolean BG_ParseConditions(char **text_pp, animScriptItem_t *scriptItem)
{
	int      conditionIndex, conditionValue[2];
	char     *token;
	qboolean minus;

	conditionValue[0] = 0;
	conditionValue[1] = 0;

	while (1)
	{
		token = COM_ParseExt(text_pp, qfalse);
		if (!token[0])
		{
			break;
		}

		// special case, "default" has no conditions
		if (!Q_stricmp(token, "default"))
		{
			return qtrue;
		}

		if (!Q_stricmp(token, "NOT") || !Q_stricmp(token, "MINUS"))
		{
			minus = qtrue;

			token = COM_ParseExt(text_pp, qfalse);
			//Com_Printf("NOT: %s \n", token );
			if (!token[0])
			{
				break;
			}
		}
		else
		{
			minus = qfalse;
		}


		conditionIndex = BG_IndexForString(token, animConditionsStr, qfalse);

		switch (animConditionsTable[conditionIndex].type)
		{
		case ANIM_CONDTYPE_BITFLAGS:
			BG_ParseConditionBits(text_pp, animConditionsTable[conditionIndex].values, conditionIndex, conditionValue);
			break;
		case ANIM_CONDTYPE_VALUE:
			if (animConditionsTable[conditionIndex].values)
			{
				token = COM_ParseExt(text_pp, qfalse);
				if (!token[0])
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected condition value, found end of line");
				}
				// check for a comma (condition divider)
				if (token[strlen(token) - 1] == ',')
				{
					token[strlen(token) - 1] = '\0';
				}
				conditionValue[0] = BG_IndexForString(token, animConditionsTable[conditionIndex].values, qfalse);
			}
			else
			{
				conditionValue[0] = 1;      // not used, just check for a positive condition
			}
			break;
		default:
			BG_AnimParseError("BG_AnimParseAnimScript: unknown condition type");
			break;
		}

		// now append this condition to the item
		scriptItem->conditions[scriptItem->numConditions].index    = conditionIndex;
		scriptItem->conditions[scriptItem->numConditions].value[0] = conditionValue[0];
		scriptItem->conditions[scriptItem->numConditions].value[1] = conditionValue[1];
		scriptItem->conditions[scriptItem->numConditions].negative = minus;
		scriptItem->numConditions++;
	}

	if (scriptItem->numConditions == 0)
	{
		BG_AnimParseError("BG_ParseConditions: no conditions found");
	}

	return qtrue;
}

/**
 * @brief BG_ParseCommands
 * @param[in] input
 * @param[in] scriptItem
 * @param[in] animModelInfo
 * @param[in] scriptData - unused
 */
static void BG_ParseCommands(char **input, animScriptItem_t *scriptItem, animModelInfo_t *animModelInfo, animScriptData_t *scriptData)
{
	char                *token;
	animScriptCommand_t *command  = NULL;
	int                 partIndex = 0;

	while (1)
	{
		// parse the body part
		token = COM_ParseExt(input, (qboolean)(partIndex < 1));
		if (!token[0])
		{
			break;
		}
		if (!Q_stricmp(token, "}"))
		{
			// unget the bracket and get out of here
			*input -= strlen(token);
			break;
		}

		// new command?
		if (partIndex == 0)
		{
			// have we exceeded the maximum number of commands?
			if (scriptItem->numCommands >= MAX_ANIMSCRIPT_ANIMCOMMANDS)
			{
				BG_AnimParseError("BG_ParseCommands: exceeded maximum number of animations (%i)", MAX_ANIMSCRIPT_ANIMCOMMANDS);
			}
			command = &scriptItem->commands[scriptItem->numCommands++];
			Com_Memset(command, 0, sizeof(*command));
		}

		command->bodyPart[partIndex] = BG_IndexForString(token, animBodyPartsStr, qtrue);
		if (command->bodyPart[partIndex] > 0)
		{
			// parse the animation
			token = COM_ParseExt(input, qfalse);
			if (!token[0])
			{
				BG_AnimParseError("BG_ParseCommands: expected animation");
			}
			command->animIndex[partIndex]    = BG_AnimationIndexForString(token, animModelInfo);
			command->animDuration[partIndex] = animModelInfo->animations[command->animIndex[partIndex]]->duration;
			// if this is a locomotion, set the movetype of the animation so we can reverse engineer the movetype from the animation, on the client
			if (parseMovetype != ANIM_MT_UNUSED && command->bodyPart[partIndex] != ANIM_BP_TORSO)
			{
				animModelInfo->animations[command->animIndex[partIndex]]->movetype |= (1 << parseMovetype);
			}
			// if this is a fireweapon event, then this is a firing animation
			if (parseEvent == ANIM_ET_FIREWEAPON || parseEvent == ANIM_ET_FIREWEAPONPRONE)
			{
				animModelInfo->animations[command->animIndex[partIndex]]->flags      |= ANIMFL_FIRINGANIM;
				animModelInfo->animations[command->animIndex[partIndex]]->initialLerp = 40;
			}
			// check for a duration for this animation instance
			token = COM_ParseExt(input, qfalse);
			if (token && token[0])
			{
				if (!Q_stricmp(token, "duration"))
				{
					// read the duration
					token = COM_ParseExt(input, qfalse);
					if (!token[0])
					{
						BG_AnimParseError("BG_ParseCommands: expected duration value");
					}
					command->animDuration[partIndex] = Q_atoi(token);
				}
				else        // unget the token
				{
					COM_RestoreParseSession(input);
				}
			}
			else
			{
				COM_RestoreParseSession(input);
			}

			if (command->bodyPart[partIndex] != ANIM_BP_BOTH && partIndex++ < 1)
			{
				continue;   // allow parsing of another bodypart
			}
		}
		else
		{
			// unget the token
			*input -= strlen(token);
		}

		// parse optional parameters (sounds, etc)
		while (1)
		{
			token = COM_ParseExt(input, qfalse);
			if (!token[0])
			{
				break;
			}

			if (!Q_stricmp(token, "sound"))
			{
				token = COM_ParseExt(input, qfalse);
				if (!token[0])
				{
					BG_AnimParseError("BG_ParseCommands: expected sound");
				}
				// NOTE: only sound script are supported at this stage
				if (strstr(token, ".wav"))
				{
					BG_AnimParseError("BG_ParseCommands: wav files not supported, only sound scripts");
				}
				// this was crashing because soundIndex wasn't initialized
				// FIXME: find the reason
				//  hmmm, soundindex is setup on both client and server :/
				//  cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
				//  level.animScriptData.soundIndex = G_SoundIndex;
				command->soundIndex = globalScriptData->soundIndex != NULL ? globalScriptData->soundIndex(token) : 0;
			}
			else
			{
				// unknown??
				BG_AnimParseError("BG_ParseCommands: unknown parameter '%s'", token);
			}
		}

		partIndex = 0;
	}
}

typedef enum
{
	PARSEMODE_DEFINES = 0,
	PARSEMODE_ANIMATION,
	PARSEMODE_CANNED_ANIMATIONS,
	PARSEMODE_STATECHANGES,
	PARSEMODE_EVENTS
} animScriptParseMode_t;

static animStringItem_t animParseModesStr[] =
{
	{ "defines",           -1 },
	{ "animations",        -1 },
	{ "canned_animations", -1 },
	{ "statechanges",      -1 },
	{ "events",            -1 },

	{ NULL,                -1 },
};

#define MAX_INDENT_LEVELS   3

/**
 * @brief Parse the animation script for this model, converting it into run-time structures
 * @param[in] animModelInfo
 * @param[in,out] scriptData
 * @param[in] filename
 * @param[in] input
 */
void BG_AnimParseAnimScript(animModelInfo_t *animModelInfo, animScriptData_t *scriptData, const char *filename, char *input)
{
	char                  *text_p = input, *token;
	animScriptParseMode_t parseMode      = PARSEMODE_DEFINES; // start at the defines
	animScript_t          *currentScript = NULL;
	animScriptItem_t      tempScriptItem;
	animScriptItem_t      *currentScriptItem = NULL;
	int                   indexes[MAX_INDENT_LEVELS], indentLevel = 0, newParseMode, i, defineType;

	// the scriptData passed into here must be the one this binary is using
	globalScriptData = scriptData;

	// init the global defines
	globalFilename = filename;
	Com_Memset(defineStr, 0, sizeof(defineStr));
	Com_Memset(defineStrings, 0, sizeof(defineStrings));
	Com_Memset(numDefines, 0, sizeof(numDefines));
	defineStringsOffset = 0;

	for (i = 0; i < MAX_INDENT_LEVELS; i++)
	{
		indexes[i] = -1;
	}

	COM_BeginParseSession("BG_AnimParseAnimScript");

	// read in the weapon defines
	while (1)
	{
		token = COM_Parse(&text_p);
		if (!token[0])
		{
			if (indentLevel)
			{
				BG_AnimParseError("BG_AnimParseAnimScript: unexpected end of file");
			}
			break;
		}

		// check for a new section
		newParseMode = BG_IndexForString(token, animParseModesStr, qtrue);
		if (newParseMode >= 0)
		{
			if (indentLevel)
			{
				BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
			}

			parseMode     = (animScriptParseMode_t)newParseMode;
			parseMovetype = ANIM_MT_UNUSED;
			parseEvent    = -1;
			continue;
		}

		switch (parseMode)
		{
		case PARSEMODE_DEFINES:
			if (!Q_stricmp(token, "set"))
			{
				// read in the define type
				token = COM_ParseExt(&text_p, qfalse);
				if (!token[0])
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected condition type string");
				}
				defineType = BG_IndexForString(token, animConditionsStr, qfalse);

				// read in the define
				token = COM_ParseExt(&text_p, qfalse);
				if (!token[0])
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected condition define string");
				}

				// copy the define to the strings list
				defineStr[defineType][numDefines[defineType]].string = BG_CopyStringIntoBuffer(token, defineStrings, sizeof(defineStrings), &defineStringsOffset);
				defineStr[defineType][numDefines[defineType]].hash   = BG_StringHashValue(defineStr[defineType][numDefines[defineType]].string);
				// expecting an =
				token = COM_ParseExt(&text_p, qfalse);
				if (!token[0])
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected '=', found end of line");
				}
				if (Q_stricmp(token, "="))
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected '=', found '%s'", token);
				}

				// parse the bits
				BG_ParseConditionBits(&text_p, animConditionsTable[defineType].values, defineType, defineBits[defineType][numDefines[defineType]]);
				numDefines[defineType]++;

				// copy the weapon defines over to the enemy_weapon defines
				Com_Memcpy(defineStr[ANIM_COND_ENEMY_WEAPON], defineStr[ANIM_COND_WEAPON], sizeof(animStringItem_t) * MAX_ANIM_DEFINES);
				Com_Memcpy(defineBits[ANIM_COND_ENEMY_WEAPON], defineBits[ANIM_COND_WEAPON], sizeof(int[2]) * MAX_ANIM_DEFINES);

				numDefines[ANIM_COND_ENEMY_WEAPON] = numDefines[ANIM_COND_WEAPON];
			}
			break;
		case PARSEMODE_ANIMATION:
		case PARSEMODE_CANNED_ANIMATIONS:
			if (!Q_stricmp(token, "{"))
			{
				// about to increment indent level, check that we have enough information to do this
				if (indentLevel >= MAX_INDENT_LEVELS)     // too many indentations
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}
				if (indexes[indentLevel] < 0)         // we havent found out what this new group is yet
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}

				indentLevel++;
			}
			else if (!Q_stricmp(token, "}"))
			{
				// reduce the indentLevel
				indentLevel--;
				if (indentLevel < 0)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}
				if (indentLevel == 1)
				{
					currentScript = NULL;
				}
				// make sure we read a new index before next indent
				indexes[indentLevel] = -1;
			}
			else if (indentLevel == 0 && (indexes[indentLevel] < 0))
			{
				if (Q_stricmp(token, "state"))
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected 'state'");
				}

				// read in the state type
				token = COM_ParseExt(&text_p, qfalse);
				if (!token[0])
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected state type");
				}
				indexes[indentLevel] = BG_IndexForString(token, animStateStr, qfalse);

				// check for the open bracket
				token = COM_ParseExt(&text_p, qtrue);
				if (!token[0] || Q_stricmp(token, "{"))
				{
					BG_AnimParseError("BG_AnimParseAnimScript: expected '{'");
				}
				indentLevel++;
			}
			else if ((indentLevel == 1) && (indexes[indentLevel] < 0))
			{
				// we are expecting a movement type
				indexes[indentLevel] = BG_IndexForString(token, animMoveTypesStr, qfalse);
				if (parseMode == PARSEMODE_ANIMATION)
				{
					currentScript = &animModelInfo->scriptAnims[indexes[0]][indexes[1]];
					parseMovetype = (scriptAnimMoveTypes_t)indexes[1];
				}
				else if (parseMode == PARSEMODE_CANNED_ANIMATIONS)
				{
					currentScript = &animModelInfo->scriptCannedAnims[indexes[1]];
				}
				Com_Memset(currentScript, 0, sizeof(*currentScript));
			}
			else if ((indentLevel == 2) && (indexes[indentLevel] < 0))
			{
				// we are expecting a condition specifier
				// move the text_p backwards so we can read in the last token again
				text_p -= strlen(token);
				// sanity check that
				if (Q_strncmp(text_p, token, strlen(token)))
				{
					// this should never happen, just here to check that this operation is correct before code goes live
					BG_AnimParseError("BG_AnimParseAnimScript: internal error");
				}

				Com_Memset(&tempScriptItem, 0, sizeof(tempScriptItem));
				indexes[indentLevel] = BG_ParseConditions(&text_p, &tempScriptItem);
				// do we have enough room in this script for another item?
				if (currentScript->numItems >= MAX_ANIMSCRIPT_ITEMS)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum items per script (%i)", MAX_ANIMSCRIPT_ITEMS);
				}
				// are there enough items left in the global list?
				if (animModelInfo->numScriptItems >= MAX_ANIMSCRIPT_ITEMS_PER_MODEL)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum global items (%i)", MAX_ANIMSCRIPT_ITEMS_PER_MODEL);
				}
				// it was parsed ok, so grab an item from the global list to use
				currentScript->items[currentScript->numItems] = &animModelInfo->scriptItems[animModelInfo->numScriptItems++];
				currentScriptItem                             = currentScript->items[currentScript->numItems];
				currentScript->numItems++;
				// copy the data across from the temp script item
				*currentScriptItem = tempScriptItem;
			}
			else if (indentLevel == 3)
			{
				// we are reading the commands, so parse this line as if it were a command
				// move the text_p backwards so we can read in the last token again
				text_p -= strlen(token);
				// sanity check that
				if (Q_strncmp(text_p, token, strlen(token)))
				{
					// this should never happen, just here to check that this operation is correct before code goes live
					BG_AnimParseError("BG_AnimParseAnimScript: internal error");
				}

				BG_ParseCommands(&text_p, currentScriptItem, animModelInfo, scriptData);
			}
			else
			{
				// huh ??
				BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
			}
			break;
		case PARSEMODE_EVENTS:
			if (!Q_stricmp(token, "{"))
			{
				// about to increment indent level, check that we have enough information to do this
				if (indentLevel >= MAX_INDENT_LEVELS)     // too many indentations
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}
				if (indexes[indentLevel] < 0)         // we havent found out what this new group is yet
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}

				indentLevel++;
			}
			else if (!Q_stricmp(token, "}"))
			{
				// reduce the indentLevel
				indentLevel--;
				if (indentLevel < 0)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}
				if (indentLevel == 0)
				{
					currentScript = NULL;
				}
				// make sure we read a new index before next indent
				indexes[indentLevel] = -1;
			}
			else if (indentLevel == 0 && (indexes[indentLevel] < 0))
			{
				// read in the event type
				indexes[indentLevel] = BG_IndexForString(token, animEventTypesStr, qfalse);
				currentScript        = &animModelInfo->scriptEvents[indexes[0]];

				parseEvent = indexes[indentLevel];

				Com_Memset(currentScript, 0, sizeof(*currentScript));
			}
			else if ((indentLevel == 1) && (indexes[indentLevel] < 0))
			{
				// we are expecting a condition specifier
				// move the text_p backwards so we can read in the last token again
				text_p -= strlen(token);
				// sanity check that
				if (Q_strncmp(text_p, token, strlen(token)))
				{
					// this should never happen, just here to check that this operation is correct before code goes live
					BG_AnimParseError("BG_AnimParseAnimScript: internal error");
				}

				Com_Memset(&tempScriptItem, 0, sizeof(tempScriptItem));
				indexes[indentLevel] = BG_ParseConditions(&text_p, &tempScriptItem);
				// do we have enough room in this script for another item?
				if (currentScript->numItems >= MAX_ANIMSCRIPT_ITEMS)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum items per script (%i)", MAX_ANIMSCRIPT_ITEMS);
				}
				// are there enough items left in the global list?
				if (animModelInfo->numScriptItems >= MAX_ANIMSCRIPT_ITEMS_PER_MODEL)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum global items (%i)", MAX_ANIMSCRIPT_ITEMS_PER_MODEL);
				}
				// it was parsed ok, so grab an item from the global list to use
				currentScript->items[currentScript->numItems] = &animModelInfo->scriptItems[animModelInfo->numScriptItems++];
				currentScriptItem                             = currentScript->items[currentScript->numItems];
				currentScript->numItems++;
				// copy the data across from the temp script item
				*currentScriptItem = tempScriptItem;
			}
			else if (indentLevel == 2)
			{
				// we are reading the commands, so parse this line as if it were a command
				// move the text_p backwards so we can read in the last token again
				text_p -= strlen(token);
				// sanity check that
				if (Q_strncmp(text_p, token, strlen(token)))
				{
					// this should never happen, just here to check that this operation is correct before code goes live
					BG_AnimParseError("BG_AnimParseAnimScript: internal error");
				}

				BG_ParseCommands(&text_p, currentScriptItem, animModelInfo, scriptData);
			}
			else
			{
				// huh ??
				BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
			}
			break;
		default:
			break;
		}
	}

	globalFilename = NULL;
}

//------------------------------------------------------------------------
// run-time gameplay functions, these are called during gameplay, so they must be
// cpu efficient.

/**
 * @brief BG_EvaluateConditions
 * @param[in] client
 * @param[in] scriptItem
 * @return qfalse if the set of conditions fails, qtrue otherwise
 */
qboolean BG_EvaluateConditions(int client, animScriptItem_t *scriptItem)
{
	int                   i;
	animScriptCondition_t *cond;
	qboolean              passed;

	for (i = 0, cond = scriptItem->conditions; i < scriptItem->numConditions; i++, cond++)
	{
		passed = qtrue;

		switch (animConditionsTable[cond->index].type)
		{
		case ANIM_CONDTYPE_BITFLAGS:
			if (!(globalScriptData->clientConditions[client][cond->index][0] & cond->value[0]) &&
			    !(globalScriptData->clientConditions[client][cond->index][1] & cond->value[1]))
			{
				passed = qfalse;
			}
			break;
		case ANIM_CONDTYPE_VALUE:
			if (!(globalScriptData->clientConditions[client][cond->index][0] == cond->value[0]))
			{
				passed = qfalse;
			}
			break;
		default:
			Com_Printf("BG_EvaluateConditions: unknown condition type\n");
			return qfalse;
		}

		if (cond->negative)
		{
			if (passed)
			{
				return qfalse;
			}
		}
		else if (!passed)
		{
			return qfalse;
		}
	}

	// all conditions must have passed
	return qtrue;
}

/**
 * @brief scroll through the script items, returning the first script found to pass all conditions
 * @param[in] client
 * @param[in] script
 * @return NULL if no match found
 */
animScriptItem_t *BG_FirstValidItem(int client, animScript_t *script)
{
	animScriptItem_t **ppScriptItem;
	int              i;

	for (i = 0, ppScriptItem = script->items; i < script->numItems; i++, ppScriptItem++)
	{
		if (BG_EvaluateConditions(client, *ppScriptItem))
		{
			return *ppScriptItem;
		}
	}

	return NULL;
}

/*
 * @brief Clears the animation timer. this is useful when we want to break a animscriptevent
 * @param[out] ps
 * @param[in] bodyPart
 *
 * @note Unused
void BG_ClearAnimTimer(playerState_t *ps, animBodyPart_t bodyPart)
{
    switch (bodyPart)
    {
    case ANIM_BP_LEGS:
        ps->legsTimer = 0;
        break;
    case ANIM_BP_TORSO:
        ps->torsoTimer = 0;
        break;
    case ANIM_BP_BOTH:
    default:
        ps->legsTimer  = 0;
        ps->torsoTimer = 0;
        break;
    }
}
*/

/**
 * @brief BG_PlayAnim
 * @param[in,out] ps
 * @param[in] animModelInfo
 * @param[in] animNum
 * @param[in] bodyPart
 * @param[in] forceDuration
 * @param[in] setTimer
 * @param[in] isContinue
 * @param[in] force
 * @return
 */
int BG_PlayAnim(playerState_t *ps, animModelInfo_t *animModelInfo, int animNum, animBodyPart_t bodyPart, int forceDuration, qboolean setTimer, qboolean isContinue, qboolean force)
{
	int      duration;
	qboolean wasSet = qfalse;

	if (forceDuration)
	{
		duration = forceDuration;
	}
	else
	{
		duration = animModelInfo->animations[animNum]->duration + 50;   // account for lerping between anims
	}

	switch (bodyPart)
	{
	case ANIM_BP_BOTH:
	case ANIM_BP_LEGS:
		if ((ps->legsTimer < 50) || force)
		{
			if (!isContinue || !((ps->legsAnim & ~ANIM_TOGGLEBIT) == animNum))
			{
				wasSet       = qtrue;
				ps->legsAnim = ((ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animNum;
				if (setTimer)
				{
					ps->legsTimer = duration;
				}
			}
			else if (setTimer && animModelInfo->animations[animNum]->loopFrames)
			{
				ps->legsTimer = duration;
			}
		}

		if (bodyPart == ANIM_BP_LEGS)
		{
			break;
		}
	// fall through for ANIM_BP_BOTH
	case ANIM_BP_TORSO:
		if ((ps->torsoTimer < 50) || force)
		{
			if (!isContinue || !((ps->torsoAnim & ~ANIM_TOGGLEBIT) == animNum))
			{
				ps->torsoAnim = ((ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animNum;
				if (setTimer)
				{
					ps->torsoTimer = duration;
				}
			}
			else if (setTimer && animModelInfo->animations[animNum]->loopFrames)
			{
				ps->torsoTimer = duration;
			}
		}

		break;
	default:     // default ANIM_BP_UNUSED NUM_ANIM_BODYPARTS not handled
		break;
	}

	if (!wasSet)
	{
		return -1;
	}

	return duration;
}

/*
 * @brief BG_PlayAnimName
 * @param[in,out] ps
 * @param[in] animModelInfo
 * @param[in] animName
 * @param[in] bodyPart
 * @param[in] setTimer
 * @param[in] isContinue
 * @param[in] force
 * @return
 *
 * @note Unused
 *
int BG_PlayAnimName(playerState_t *ps, animModelInfo_t *animModelInfo, char *animName, animBodyPart_t bodyPart, qboolean setTimer, qboolean isContinue, qboolean force)
{
    return BG_PlayAnim(ps, animModelInfo, BG_AnimationIndexForString(animName, animModelInfo), bodyPart, 0, setTimer, isContinue, force);
}
*/

/**
 * @brief BG_ExecuteCommand
 * @param[in,out] ps
 * @param[in] animModelInfo
 * @param[in] scriptCommand
 * @param[in] setTimer
 * @param[in] isContinue
 * @param[in] force
 * @return The duration of the animation, -1 if no anim was set
 */
int BG_ExecuteCommand(playerState_t *ps, animModelInfo_t *animModelInfo, animScriptCommand_t *scriptCommand, qboolean setTimer, qboolean isContinue, qboolean force)
{
	int      duration       = -1;
	qboolean playedLegsAnim = qfalse;

	if (scriptCommand->bodyPart[0])
	{
		duration = scriptCommand->animDuration[0] + 50;
		// FIXME: how to sync torso/legs anims accounting for transition blends, etc
		if (scriptCommand->bodyPart[0] == ANIM_BP_BOTH || scriptCommand->bodyPart[0] == ANIM_BP_LEGS)
		{
			playedLegsAnim = (qboolean)(BG_PlayAnim(ps, animModelInfo, scriptCommand->animIndex[0], (animBodyPart_t)scriptCommand->bodyPart[0], duration, setTimer, isContinue, force) > -1);
		}
		else
		{
			BG_PlayAnim(ps, animModelInfo, scriptCommand->animIndex[0], (animBodyPart_t)scriptCommand->bodyPart[0], duration, setTimer, isContinue, force);
		}
	}
	if (scriptCommand->bodyPart[1])
	{
		duration = scriptCommand->animDuration[0] + 50;
		// FIXME: how to sync torso/legs anims accounting for transition blends, etc
		// just play the animation for the torso
		if (scriptCommand->bodyPart[1] == ANIM_BP_BOTH || scriptCommand->bodyPart[1] == ANIM_BP_LEGS)
		{
			playedLegsAnim = (qboolean)(BG_PlayAnim(ps, animModelInfo, scriptCommand->animIndex[1], (animBodyPart_t)scriptCommand->bodyPart[1], duration, setTimer, isContinue, force) > -1);
		}
		else
		{
			BG_PlayAnim(ps, animModelInfo, scriptCommand->animIndex[1], (animBodyPart_t)scriptCommand->bodyPart[1], duration, setTimer, isContinue, force);
		}
	}

	if (scriptCommand->soundIndex)
	{
		globalScriptData->playSound(scriptCommand->soundIndex, ps->origin, ps->clientNum);
	}

	if (!playedLegsAnim)
	{
		return -1;
	}

	return duration;
}

/**
 * @brief Runs the normal locomotive animations
 * @param[in,out] ps
 * @param[in,out] animModelInfo
 * @param[in] movetype
 * @param[in] isContinue
 * @return 1 if an animation was set, -1 if no animation was found, 0 otherwise
 */
int BG_AnimScriptAnimation(playerState_t *ps, animModelInfo_t *animModelInfo, scriptAnimMoveTypes_t movetype, qboolean isContinue)
{
	animScript_t        *script        = NULL;
	animScriptItem_t    *scriptItem    = NULL;
	animScriptCommand_t *scriptCommand = NULL;
	int                 state          = ps->aiState;

	// Allow fallen movetype while dead
	if ((ps->eFlags & EF_DEAD) && movetype != ANIM_MT_FALLEN && movetype != ANIM_MT_FLAILING && movetype != ANIM_MT_DEAD)
	{
		return -1;
	}

#ifdef DBGANIMS
	Com_Printf("script anim: cl %i, mt %s, ", ps->clientNum, animMoveTypesStr[movetype]);
#endif

	// try finding a match in all states ABOVE the given state
	while (!scriptItem && state < MAX_AISTATES)
	{
		script = &animModelInfo->scriptAnims[state][movetype];
		if (!script->numItems)
		{
			state++;
			continue;
		}
		// find the first script item, that passes all the conditions for this event
		scriptItem = BG_FirstValidItem(ps->clientNum, script);
		if (!scriptItem)
		{
			state++;
			continue;
		}
	}

	if (!scriptItem)
	{
#ifdef DBGANIMS
		Com_Printf("no valid conditions\n");
#endif
		return -1;
	}
	// save this as our current movetype
	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOVETYPE, movetype, qtrue);
	// pick the correct animation for this character (animations must be constant for each character, otherwise they'll constantly change)
	scriptCommand = &scriptItem->commands[ps->clientNum % scriptItem->numCommands];

#ifdef DBGANIMS
	if (scriptCommand->bodyPart[0])
	{
		Com_Printf("anim0 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[0]].string, animModelInfo->animations[scriptCommand->animIndex[0]]->name);
	}
	if (scriptCommand->bodyPart[1])
	{
		Com_Printf("anim1 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[1]].string, animModelInfo->animations[scriptCommand->animIndex[1]]->name);
	}
	Com_Printf("\n");
#endif

	// run it
	return (BG_ExecuteCommand(ps, animModelInfo, scriptCommand, qfalse, isContinue, qfalse) != -1);
}

/*
 * @brief Uses the current movetype for this client to play a canned animation
 * @param[in,out] ps
 * @param[in] animModelInfo
 * @return The duration in milliseconds that this model should be paused. -1 if no anim found
 *
 * @note Unused
int BG_AnimScriptCannedAnimation(playerState_t *ps, animModelInfo_t *animModelInfo)
{
    animScript_t          *script;
    animScriptItem_t      *scriptItem;
    animScriptCommand_t   *scriptCommand;
    scriptAnimMoveTypes_t movetype;

    if (ps->eFlags & EF_DEAD)
    {
        return -1;
    }

    movetype = (scriptAnimMoveTypes_t)globalScriptData->clientConditions[ps->clientNum][ANIM_COND_MOVETYPE][0];
    if (!movetype)        // no valid movetype yet for this client
    {
        return -1;
    }

    script = &animModelInfo->scriptCannedAnims[movetype];
    if (!script->numItems)
    {
        return -1;
    }
    // find the first script item, that passes all the conditions for this event
    scriptItem = BG_FirstValidItem(ps->clientNum, script);
    if (!scriptItem)
    {
        return -1;
    }
    // pick a random command
    scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];
    // run it
    return BG_ExecuteCommand(ps, animModelInfo, scriptCommand, qtrue, qfalse, qfalse);
}
*/

/**
 * @brief BG_AnimScriptEvent
 * @param[in,out] ps
 * @param[in] animModelInfo
 * @param[in] event
 * @param[in] isContinue
 * @param[in] force
 * @return The duration in milliseconds that this model should be paused. -1 if no event found
 */
int BG_AnimScriptEvent(playerState_t *ps, animModelInfo_t *animModelInfo, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force)
{
	animScript_t        *script;
	animScriptItem_t    *scriptItem;
	animScriptCommand_t *scriptCommand;

	if (event != ANIM_ET_DEATH && (ps->eFlags & EF_DEAD))
	{
		return -1;
	}

	if (event < ANIM_ET_PAIN || event >= NUM_ANIM_EVENTTYPES)
	{
		Com_Printf("BG_AnimScriptEvent: unknown script event -1\n");
		return -1;
	}

#ifdef DBGANIMEVENTS
	Com_Printf("script event: cl %i, ev %s, ", ps->clientNum, animEventTypesStr[event]);
#endif

	script = &animModelInfo->scriptEvents[event];
	if (!script->numItems)
	{
#ifdef DBGANIMEVENTS
		Com_Printf("no entry\n");
#endif
		return -1;
	}
	// find the first script item, that passes all the conditions for this event
	scriptItem = BG_FirstValidItem(ps->clientNum, script);
	if (!scriptItem)
	{
#ifdef DBGANIMEVENTS
		Com_Printf("no valid conditions\n");
#endif
		return -1;
	}
	// pick a random command
	scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];

#ifdef DBGANIMEVENTS
	if (scriptCommand->bodyPart[0])
	{
		Com_Printf("anim0 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[0]].string, animModelInfo->animations[scriptCommand->animIndex[0]]->name);
	}
	if (scriptCommand->bodyPart[1])
	{
		Com_Printf("anim1 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[1]].string, animModelInfo->animations[scriptCommand->animIndex[1]]->name);
	}
	Com_Printf("\n");
#endif

	// run it
	return BG_ExecuteCommand(ps, animModelInfo, scriptCommand, qtrue, isContinue, force);
}

/**
 * @brief BG_GetAnimString
 * @param[in] animModelInfo
 * @param[in] anim
 * @return
 */
char *BG_GetAnimString(animModelInfo_t *animModelInfo, int anim)
{
	if (anim >= animModelInfo->numAnimations)
	{
		BG_AnimParseError("BG_GetAnimString: anim index is out of range");
	}
	return animModelInfo->animations[anim]->name;
}

/**
 * @brief BG_UpdateConditionValue
 * @param[in,out] client
 * @param[in] condition
 * @param[in] value
 * @param[in] checkConversion
 */
void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion)
{
	// fixed checkConversion brained-damagedness, which would try
	// to BitSet an insane value if checkConversion was false but this
	// anim was ANIM_CONDTYPE_BITFLAGS
	if (checkConversion == qtrue)
	{
		if (animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS)
		{
			// we may need to convert to bitflags
			// We want to set the ScriptData to the explicit value passed in.
			//              COM_BitSet will OR values on top of each other, so clear it first.
			globalScriptData->clientConditions[client][condition][0] = 0;
			globalScriptData->clientConditions[client][condition][1] = 0;

			COM_BitSet(globalScriptData->clientConditions[client][condition], value);
			return;
		}
		// we must fall through here because a bunch of non-bitflag
		// conditions are set with checkConversion == qtrue
	}
	globalScriptData->clientConditions[client][condition][0] = value;
}

/**
 * @brief BG_GetConditionValue
 * @param[in] client
 * @param[in] condition
 * @param[in] checkConversion
 * @return
 */
int BG_GetConditionValue(int client, int condition, qboolean checkConversion)
{
	if (animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS)
	{
		if (checkConversion)
		{
			int i;

			// we may need to convert to a value
			//if (!value)
			//  return 0;
			for (i = 0; i < 8 * sizeof(globalScriptData->clientConditions[0][0]); i++)
			{
				if (COM_BitCheck(globalScriptData->clientConditions[client][condition], i))
				{
					return i;
				}
			}
			// nothing found
			return 0;
		}
		else
		{
			// must use COM_BitCheck on the result.
			return (intptr_t)globalScriptData->clientConditions[client][condition];
		}
		//BG_AnimParseError( "BG_GetConditionValue: internal error" );
	}
	else
	{
		return globalScriptData->clientConditions[client][condition][0];
	}
}

/**
 * @brief BG_GetConditionBitFlag
 * @param[in] client
 * @param[in] condition
 * @param[in] bitNumber
 * @return Whether the specified bit flag is set
 */
qboolean BG_GetConditionBitFlag(int client, int condition, int bitNumber)
{
	if (animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS)
	{
		return (COM_BitCheck(globalScriptData->clientConditions[client][condition], bitNumber));
	}
	else
	{
		Com_Error(ERR_DROP, "BG_GetConditionBitFlag: animation condition %i is not a bitflag condition", animConditionsTable[condition].type);
	}
	return qfalse;
}

/**
 * @brief BG_SetConditionBitFlag
 * @param[in] client
 * @param[in] condition
 * @param[in] bitNumber
 */
void BG_SetConditionBitFlag(int client, int condition, int bitNumber)
{
	COM_BitSet(globalScriptData->clientConditions[client][condition], bitNumber);
}

/**
 * @brief BG_ClearConditionBitFlag
 * @param[in] client
 * @param[in] condition
 * @param[in] bitNumber
 */
void BG_ClearConditionBitFlag(int client, int condition, int bitNumber)
{
	COM_BitClear(globalScriptData->clientConditions[client][condition], bitNumber);
}

/**
 * @brief BG_GetAnimScriptAnimation
 * @param[in] client
 * @param[in] animModelInfo
 * @param[in] aistate
 * @param[in] movetype
 * @return The locomotion animation index, -1 if no animation was found, 0 otherwise
 */
int BG_GetAnimScriptAnimation(int client, animModelInfo_t *animModelInfo, aistateEnum_t aistate, scriptAnimMoveTypes_t movetype)
{
	animScript_t        *script;
	animScriptItem_t    *scriptItem = NULL;
	animScriptCommand_t *scriptCommand;
	int                 state = aistate;

	// adapted from the original SP source code
	// try finding a match in all states ABOVE the given state
	while (!scriptItem && state < MAX_AISTATES)
	{
		script = &animModelInfo->scriptAnims[state][movetype];
		if (!script->numItems)
		{
			state++;
			continue;
		}
		// find the first script item, that passes all the conditions for this event
		scriptItem = BG_FirstValidItem(client, script);
		if (!scriptItem)
		{
			state++;
			continue;
		}
	}

	if (!scriptItem)
	{
		return -1;
	}
	// pick the correct animation for this character (animations must be constant for each character, otherwise they'll constantly change)
	scriptCommand = &scriptItem->commands[client % scriptItem->numCommands];
	if (!scriptCommand->bodyPart[0])
	{
		return -1;
	}
	// return the animation
	return scriptCommand->animIndex[0];
}

/*
 * @brief BG_GetAnimScriptEvent
 * @param[in] ps
 * @param[in] event
 * @return The animation index for this event
 *
 * @note Unused
int BG_GetAnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event)
{
    animModelInfo_t     *animModelInfo;
    animScript_t        *script;
    animScriptItem_t    *scriptItem;
    animScriptCommand_t *scriptCommand;

    if (event != ANIM_ET_DEATH && (ps->eFlags & EF_DEAD))
    {
        return -1;
    }

    animModelInfo = BG_GetCharacterForPlayerstate(ps)->animModelInfo;
    script        = &animModelInfo->scriptEvents[event];
    if (!script->numItems)
    {
        return -1;
    }
    // find the first script item, that passes all the conditions for this event
    scriptItem = BG_FirstValidItem(ps->clientNum, script);
    if (!scriptItem)
    {
        return -1;
    }
    // pick a random command
    scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];

    // return the animation
    return scriptCommand->animIndex[0];
}
*/

/**
 * @brief BG_GetAnimationForIndex
 * @param[in] animModelInfo
 * @param[in] index
 * @return The animation_t for the given index
 */
animation_t *BG_GetAnimationForIndex(animModelInfo_t *animModelInfo, int index)
{
	if (index < 0 || index >= animModelInfo->numAnimations)
	{
		Com_Error(ERR_DROP, "BG_GetAnimationForIndex: index out of bounds");
		return NULL;
	}

	return animModelInfo->animations[index];
}

/**
 * @brief BG_AnimUpdatePlayerStateConditions
 * @param[in] pmove
 */
void BG_AnimUpdatePlayerStateConditions(pmove_t *pmove)
{
	playerState_t *ps = pmove->ps;

	// WEAPON
	if (ps->eFlags & EF_ZOOMING)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON, WP_BINOCULARS, qtrue);
		BG_SetConditionBitFlag(ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_ZOOMING);
	}
	else
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON, ps->weapon, qtrue);
		BG_ClearConditionBitFlag(ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_ZOOMING);
	}

	// MOUNTED
	if ((ps->eFlags & EF_MG42_ACTIVE) || (ps->eFlags & EF_MOUNTEDTANK))
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue);
	}
	else if (ps->eFlags & EF_AAGUN_ACTIVE)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_AAGUN, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue);
	}

	// UNDERHAND
	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_UNDERHAND, ps->viewangles[0] > 0, qtrue);

	if (ps->viewheight == ps->crouchViewHeight)
	{
		ps->eFlags |= EF_CROUCHING;
	}
	else
	{
		ps->eFlags &= ~EF_CROUCHING;
	}

	if (pmove->cmd.buttons & BUTTON_ATTACK)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FIRING, qtrue, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FIRING, qfalse, qtrue);
	}

	if (ps->pm_flags & PMF_FLAILING)
	{
		if (ps->groundEntityNum == ENTITYNUM_NONE)
		{
			BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FLAILING_TYPE, FLAILING_INAIR, qtrue);
			ps->pm_time = 750;
		}
		else
		{
			if (globalScriptData->clientConditions[ps->clientNum][ANIM_COND_FLAILING_TYPE][0] != FLAILING_VCRASH)
			{
				BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FLAILING_TYPE, FLAILING_VCRASH, qtrue);
				ps->pm_time = 750;
			}
		}
	}

	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_IMPACT_POINT, IMPACTPOINT_UNUSED, qtrue);
	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_STUNNED, 0, qtrue);
	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_SUICIDE, 0, qtrue);
}
