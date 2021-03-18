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
 * @file cvar.c
 * @brief Dynamic variable tracking
 */

#include "q_shared.h"
#include "qcommon.h"

cvar_t *cvar_vars;
cvar_t *cvar_cheats;
int    cvar_modifiedFlags;

#define MAX_CVARS   2048
cvar_t cvar_indexes[MAX_CVARS];
int    cvar_numIndexes;

#define FILE_HASH_SIZE      512
static cvar_t *hashTable[FILE_HASH_SIZE];
#define generateHashValue(fname) Q_GenerateHashValue(fname, FILE_HASH_SIZE, qtrue, qtrue)

/**
 * @brief Cvar_ValidateString
 * @param[in] s
 * @return
 */
static qboolean Cvar_ValidateString(const char *s)
{
	if (!s)
	{
		return qfalse;
	}
	if (strchr(s, '\\'))
	{
		return qfalse;
	}
	if (strchr(s, '\"'))
	{
		return qfalse;
	}
	if (strchr(s, ';'))
	{
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief Cvar_FindVar
 * @param[in] var_name
 * @return
 */
static cvar_t *Cvar_FindVar(const char *var_name)
{
	cvar_t *var;
	long   hash;

	hash = generateHashValue(var_name);

	for (var = hashTable[hash] ; var ; var = var->hashNext)
	{
		if (!Q_stricmp(var_name, var->name))
		{
			return var;
		}
	}

	return NULL;
}

/**
 * @brief Cvar_VariableValue
 * @param[in] var_name
 * @return
 */
float Cvar_VariableValue(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if (!var)
	{
		return 0;
	}
	return var->value;
}

/**
 * @brief Cvar_VariableIntegerValue
 * @param[in] var_name
 * @return
 */
int Cvar_VariableIntegerValue(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if (!var)
	{
		return 0;
	}
	return var->integer;
}

/**
 * @brief Cvar_VariableString
 * @param[in] var_name
 * @return
 */
char *Cvar_VariableString(const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if (!var)
	{
		return "";
	}
	return var->string;
}

/**
 * @brief Cvar_VariableStringBuffer
 * @param[in] var_name
 * @param[out] buffer
 * @param[in] bufsize
 */
void Cvar_VariableStringBuffer(const char *var_name, char *buffer, size_t bufsize)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if (!var)
	{
		*buffer = 0;
	}
	else
	{
		Q_strncpyz(buffer, var->string, bufsize);
	}
}

/**
 * @brief Cvar_LatchedVariableStringBuffer
 * @param[in] var_name
 * @param[out] buffer
 * @param[in] bufsize
 */
void Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer, size_t bufsize)
{
	cvar_t *var;

	var = Cvar_FindVar(var_name);
	if (!var)
	{
		*buffer = 0;
	}
	else
	{
		if (var->latchedString)
		{
			Q_strncpyz(buffer, var->latchedString, bufsize);
		}
		else
		{
			Q_strncpyz(buffer, var->string, bufsize);
		}
	}
}

/**
 * @brief Cvar_Flags
 * @param[in] var_name
 * @return
 */
int Cvar_Flags(const char *var_name)
{
	cvar_t *var;

	if (!(var = Cvar_FindVar(var_name)))
	{
		return CVAR_NONEXISTENT;
	}
	else
	{
		if (var->modified)
		{
			return var->flags | CVAR_MODIFIED;
		}
		else
		{
			return var->flags;
		}
	}
}

/**
 * @brief Cvar_CommandCompletion
 */
void Cvar_CommandCompletion(void (*callback)(const char *s))
{
	cvar_t *cvar;

	for (cvar = cvar_vars ; cvar ; cvar = cvar->next)
	{
		if (cvar->name && (cvar->flags & CVAR_NOTABCOMPLETE) == 0)
		{
			callback(cvar->name);
		}
	}
}

/**
 * @brief Some cvar values need to be safe from foreign characters
 *
 * @param[in] value
 *
 * @return
 */
char *Cvar_ClearForeignCharacters(const char *value)
{
	static char clean[MAX_CVAR_VALUE_STRING];
	int         i, j = 0;

	for (i = 0; value[i] != '\0'; i++)
	{
		if (((byte *)value)[i] != 0xFF && (((byte *)value)[i] <= 127 || ((byte *)value)[i] >= 161))
		{
			clean[j] = value[i];
			j++;
		}
	}
	clean[j] = '\0';

	return clean;
}

/**
 * @brief Cvar_Validate
 * @param[in,out] cv
 * @param[in] value
 * @param[in] warn
 * @return
 */
static const char *Cvar_Validate(cvar_t *cv, const char *value, qboolean warn)
{
	static char s[MAX_CVAR_VALUE_STRING];
	float       valuef  = 0.f;
	qboolean    changed = qfalse;

	if (!cv->validate)
	{
		return value;
	}

	if (!value)
	{
		return value;
	}

	if (Q_isanumber(value))
	{
		valuef = atof(value);

		if (cv->integral)
		{
			if (!Q_isintegral(valuef))
			{
				if (warn)
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' must be integral (%f)", cv->name, valuef);
				}

				valuef  = (int)valuef;
				changed = qtrue;
			}
		}
	}
	else
	{
		if (warn)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' must be numeric (%i)", cv->name, (int)valuef);
		}

		valuef  = atof(cv->resetString);
		changed = qtrue;
	}

	if (valuef < cv->min)
	{
		if (warn)
		{
			if (changed)
			{
				Com_Printf(S_COLOR_YELLOW " and is");
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s'", cv->name);
			}

			if (Q_isintegral(cv->min))
			{
				Com_Printf(S_COLOR_YELLOW " out of range (%i < %i)", (int)valuef, (int)cv->min);
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW " out of range (%f < %f)", valuef, cv->min);
			}
		}

		valuef  = cv->min;
		changed = qtrue;
	}
	else if (valuef > cv->max)
	{
		if (warn)
		{
			if (changed)
			{
				Com_Printf(S_COLOR_YELLOW " and is");
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s'", cv->name);
			}

			if (Q_isintegral(cv->max))
			{
				Com_Printf(S_COLOR_YELLOW " out of range (%i > %i)", (int)valuef, (int)cv->max);
			}
			else
			{
				Com_Printf(S_COLOR_YELLOW " out of range (%f > %f)", valuef, cv->max);
			}
		}

		valuef  = cv->max;
		changed = qtrue;
	}

	if (changed)
	{
		if (Q_isintegral(valuef))
		{
			Com_sprintf(s, sizeof(s), "%d", (int)valuef);

			if (warn)
			{
				Com_Printf(S_COLOR_YELLOW ", setting to %d\n", (int)valuef);
			}
		}
		else
		{
			Com_sprintf(s, sizeof(s), "%f", valuef);

			if (warn)
			{
				Com_Printf(S_COLOR_YELLOW ", setting to %f\n", valuef);
			}
		}

		return s;
	}
	else
	{
		return value;
	}
}

/**
 * @brief If the variable already exists, the value will not be set unless CVAR_ROM
 * The flags will be or'ed in if the variable exists.
 * @param[in] varName
 * @param[in] value
 * @param[in] flags
 * @return
 */
cvar_t *Cvar_Get(const char *varName, const char *value, int flags)
{
	cvar_t *var;
	long   hash;
	int    index;

	if (!varName || !value)
	{
		Com_Error(ERR_FATAL, "Cvar_Get: NULL parameter");
	}

	if (!Cvar_ValidateString(varName))
	{
		Com_Printf("invalid cvar name string: %s\n", varName);
		varName = "BADNAME";
	}

#if 0       // FIXME: values with backslash happen
	if (!Cvar_ValidateString(var_value))
	{
		Com_Printf("invalid cvar value string: %s\n", var_value);
		var_value = "BADVALUE";
	}
#endif

	var = Cvar_FindVar(varName);
	if (var)
	{
		value = Cvar_Validate(var, value, qfalse);

		// if the C code is now specifying a variable that the user already
		// set a value for, take the new value as the reset value
		if (var->flags & CVAR_USER_CREATED)
		{
			var->flags &= ~CVAR_USER_CREATED;
			Z_Free(var->resetString);
			var->resetString = CopyString(value);

			if (flags & CVAR_ROM)
			{
				// this variable was set by the user,
				// so force it to value given by the engine.

				if (var->latchedString)
				{
					Z_Free(var->latchedString);
				}

				var->latchedString = CopyString(value);
			}
		}

		// Make sure the game code cannot mark engine-added variables as gamecode vars
		if (var->flags & CVAR_VM_CREATED)
		{
			if (!(flags & CVAR_VM_CREATED))
			{
				var->flags &= ~CVAR_VM_CREATED;
			}
		}
		else
		{
			if (flags & CVAR_VM_CREATED)
			{
				flags &= ~CVAR_VM_CREATED;
			}
		}

		// Make sure servers cannot mark engine-added variables as SERVER_CREATED
		if (var->flags & CVAR_SERVER_CREATED)
		{
			if (!(flags & CVAR_SERVER_CREATED))
			{
				var->flags &= ~CVAR_SERVER_CREATED;
			}
		}
		else
		{
			if (flags & CVAR_SERVER_CREATED)
			{
				flags &= ~CVAR_SERVER_CREATED;
			}
		}

		var->flags |= flags;

		// only allow one non-empty reset string without a warning
		if (!var->resetString[0])
		{
			// we don't have a reset string yet
			Z_Free(var->resetString);
			var->resetString = CopyString(value);
		}
		else if (value[0] && strcmp(var->resetString, value))
		{
			Com_DPrintf("Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n",
			            varName, var->resetString, value);
		}
		// if we have a latched string, take that value now
		if (var->latchedString)
		{
			char *s;

			s                  = var->latchedString;
			var->latchedString = NULL;  // otherwise cvar_set2 would free it
			Cvar_Set2(varName, s, qtrue);
			Z_Free(s);
		}

		// if CVAR_USERINFO was toggled on for an existing cvar, check wether the value needs to be cleaned from foreigh characters
		// (for instance, seta name "name-with-foreign-chars" in the config file, and toggle to CVAR_USERINFO happens later in CL_Init)
		if (flags & CVAR_USERINFO)
		{
			char *cleaned = Cvar_ClearForeignCharacters(var->string);   // NOTE: it is probably harmless to call Cvar_Set2 in all cases, but I don't want to risk it

			if (strcmp(var->string, cleaned))
			{
				Cvar_Set2(var->name, var->string, qfalse);   // call Cvar_Set2 with the value to be cleaned up for verbosity
			}
		}

		// ZOID--needs to be set so that cvars the game sets as
		// SERVERINFO get sent to clients
		cvar_modifiedFlags |= flags;

		return var;
	}

	//
	// allocate a new cvar
	//

	// find a free cvar
	for (index = 0; index < MAX_CVARS; index++)
	{
		if (!cvar_indexes[index].name)
		{
			break;
		}
	}

	if (index >= MAX_CVARS)
	{
		if (!com_errorEntered)
		{
			Com_Error(ERR_FATAL, "Error: Too many cvars (%d), cannot create a new one!", MAX_CVARS);
		}

		return NULL;
	}

	var = &cvar_indexes[index];

	if (index >= cvar_numIndexes)
	{
		cvar_numIndexes = index + 1;
	}

	var->name              = CopyString(varName);
	var->string            = CopyString(value);
	var->modified          = qtrue;
	var->modificationCount = 1;
	var->value             = Q_atof(var->string);
	var->integer           = Q_atoi(var->string);
	var->resetString       = CopyString(value);
	var->validate          = qfalse;
	var->description       = NULL;

	// link the variable in
	var->next = cvar_vars;
	if (cvar_vars)
	{
		cvar_vars->prev = var;
	}

	var->prev = NULL;
	cvar_vars = var;

	var->flags = flags;
	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	hash           = generateHashValue(varName);
	var->hashIndex = hash;

	var->hashNext = hashTable[hash];
	if (hashTable[hash])
	{
		hashTable[hash]->hashPrev = var;
	}

	var->hashPrev   = NULL;
	hashTable[hash] = var;

	return var;
}

/**
 * @brief If the variable already exists, the value will not be set unless CVAR_ROM
 * The flags will be or'ed in if the variable exists.
 * @param[in] varName cvar's techical name
 * @param[in] value default value, if not set by config or console
 * @param[in] flags
 * @param[in] description The description of this cvar
 * @return new cvar registered cvar instance
 */
cvar_t *Cvar_GetAndDescribe(const char *varName, const char *value, int flags, const char *description)
{
    cvar_t *tmp = Cvar_Get(varName, value, flags);
    Cvar_SetDescription(tmp, description);
    return tmp;
}

#define FOREIGN_MSG "Foreign characters are not allowed in userinfo variables.\n"

/**
 * @brief Cvar_Set2
 * @param[in] var_name
 * @param[in] value
 * @param[in] force
 * @return
 */
cvar_t *Cvar_Set2(const char *var_name, const char *value, qboolean force)
{
	cvar_t *var;

	//Com_DPrintf("Cvar_Set2: %s %s\n", var_name, value);

	if (!Cvar_ValidateString(var_name))
	{
		Com_Printf("invalid cvar name string: %s\n", var_name);
		var_name = "BADNAME";
	}

	var = Cvar_FindVar(var_name);
	if (!var)
	{
		if (!value)
		{
			return NULL;
		}
		// create it
		if (!force)
		{
			return Cvar_Get(var_name, value, CVAR_USER_CREATED);
		}
		else
		{
			return Cvar_Get(var_name, value, 0);
		}
	}

	if (!value)
	{
		value = var->resetString;
	}

	value = Cvar_Validate(var, value, qtrue);

	if (var->flags & CVAR_USERINFO)
	{
		char *cleaned;

		cleaned = Cvar_ClearForeignCharacters(value);
		if (strcmp(value, cleaned))
		{
#ifdef DEDICATED
			Com_Printf(FOREIGN_MSG);
#else
			Com_Printf("%s", FOREIGN_MSG);
#endif
			Com_Printf("Using %s instead of %s\n", cleaned, value);
			return Cvar_Set2(var_name, cleaned, force);
		}
	}

	if ((var->flags & CVAR_LATCH) && var->latchedString)
	{
		if (!strcmp(value, var->string))
		{
			Z_Free(var->latchedString);
			var->latchedString = NULL;
			return var;
		}

		if (!strcmp(value, var->latchedString))
		{
			return var;
		}
	}
	else if (!strcmp(value, var->string))
	{
		return var;
	}

	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	if (!force)
	{
		// don't set unsafe variables when com_crashed is set
		if ((var->flags & CVAR_UNSAFE) && com_crashed != NULL && com_crashed->integer)
		{
			Com_Printf("%s is unsafe. Check com_crashed.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_ROM)
		{
			Com_Printf("%s is read only.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_INIT)
		{
			Com_Printf("%s is write protected.\n", var_name);
			return var;
		}

		if ((var->flags & CVAR_CHEAT) && !cvar_cheats->integer)
		{
			Com_Printf("%s is cheat protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_SHADER)
		{
			Com_Printf("%s will be changed upon recompiling shaders.\n", var_name);
			Cvar_Set("r_recompileShaders", "1");
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latchedString)
			{
				if (strcmp(value, var->latchedString) == 0)
				{
					return var;
				}
				Z_Free(var->latchedString);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
				{
					return var;
				}
			}

			Com_Printf("%s will be changed upon restarting. New value: '%s'\n", var_name, value);
			var->latchedString = CopyString(value);
			var->modified      = qtrue;
			var->modificationCount++;
			return var;
		}
	}
	else
	{
		if (var->latchedString)
		{
			Z_Free(var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp(value, var->string))
	{
		return var;     // not changed

	}
	var->modified = qtrue;
	var->modificationCount++;

	Z_Free(var->string);     // free the old value string

	var->string  = CopyString(value);
	var->value   = Q_atof(var->string);
	var->integer = Q_atoi(var->string);

	return var;
}

/**
 * @brief Cvar_Set
 * @param[in] varName
 * @param[in] value
 */
void Cvar_Set(const char *varName, const char *value)
{
	Cvar_Set2(varName, value, qtrue);
}

/**
 * @brief Cvar_SetSafe
 * @param[in] var_name
 * @param[in] value
 */
void Cvar_SetSafe(const char *var_name, const char *value)
{
	int flags = Cvar_Flags(var_name);

	if ((flags != CVAR_NONEXISTENT) && (flags & CVAR_PROTECTED))
	{
		if (value)
		{
			Com_Error(ERR_DROP, "Restricted source tried to set "
			                    "\"%s\" to \"%s\"", var_name, value);
		}
		else
		{
			Com_Error(ERR_DROP, "Restricted source tried to "
			                    "modify \"%s\"", var_name);
		}
	}
	Cvar_Set(var_name, value);
}

/**
 * @brief Cvar_SetLatched
 * @param[in] var_name
 * @param[in] value
 */
void Cvar_SetLatched(const char *var_name, const char *value)
{
	Cvar_Set2(var_name, value, qfalse);
}

/**
 * @brief Cvar_SetValue
 * @param[in] var_name
 * @param[in] value
 */
void Cvar_SetValue(const char *var_name, float value)
{
	char val[32];

	if (Q_isintegral(value))
	{
		Com_sprintf(val, sizeof(val), "%i", (int)value);
	}
	else
	{
		Com_sprintf(val, sizeof(val), "%f", (double)value);
	}
	Cvar_Set(var_name, val);
}

/**
 * @brief Cvar_SetValueSafe
 * @param[in] var_name
 * @param[in] value
 */
void Cvar_SetValueSafe(const char *var_name, float value)
{
	char val[32];

	if (Q_isintegral(value))
	{
		Com_sprintf(val, sizeof(val), "%i", (int)value);
	}
	else
	{
		Com_sprintf(val, sizeof(val), "%f", (double)value);
	}
	Cvar_SetSafe(var_name, val);
}

/**
 * @brief Cvar_Reset
 * @param[in] var_name
 */
void Cvar_Reset(const char *var_name)
{
	Cvar_Set2(var_name, NULL, qfalse);
}

/**
 * @brief Cvar_ForceReset
 * @param[in] var_name
 */
void Cvar_ForceReset(const char *var_name)
{
	Cvar_Set2(var_name, NULL, qtrue);
}

/**
 * @brief Any testing variables will be reset to the safe values
 */
void Cvar_SetCheatState(void)
{
	cvar_t *var;

	// set all default vars to the safe value
	for (var = cvar_vars; var ; var = var->next)
	{
		if (var->flags & CVAR_CHEAT)
		{
			// the CVAR_LATCHED|CVAR_CHEAT vars might escape the reset here
			// because of a different var->latchedString
			if (var->latchedString)
			{
				Z_Free(var->latchedString);
				var->latchedString = NULL;
			}
			if (strcmp(var->resetString, var->string) != 0)
			{
				Cvar_Set(var->name, var->resetString);
			}
		}
	}
}

/**
 * @brief Prints the value, default, and latched string of the given variable
 *
 * @param[in] v
 */
void Cvar_Print(cvar_t *v)
{
	Com_Printf("\"%s\" is: \"%s" S_COLOR_WHITE "\"", v->name, v->string);

	if (!(v->flags & CVAR_ROM))
	{
		if (!Q_stricmp(v->string, v->resetString))
		{
			Com_Printf(", the default");
		}
		else
		{
			Com_Printf(" default:\"%s" S_COLOR_WHITE "\"",
			           v->resetString);
		}
	}

	Com_Printf("\n");

	if (v->latchedString)
	{
		Com_Printf("latched: \"%s\"\n", v->latchedString);
	}

	if (v->description)
	{
		Com_Printf("%s\n", v->description);
	}
}

/**
 * @brief Handles variable inspection and changing from the console
 * @return
 */
qboolean Cvar_Command(void)
{
	cvar_t *v;

	// check variables
	v = Cvar_FindVar(Cmd_Argv(0));
	if (!v)
	{
		return qfalse;
	}

	// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Cvar_Print(v);
		return qtrue;
	}

	// set the value if forcing isn't required
	Cvar_Set2(v->name, Cmd_Args(), qfalse);
	return qtrue;
}

/**
 * @brief Prints the contents of a cvar
(preferred over Cvar_Command where cvar names and commands conflict)
 */
void Cvar_Print_f(void)
{
	char   *name;
	cvar_t *cv;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: print <variable>\n");
		return;
	}

	name = Cmd_Argv(1);

	cv = Cvar_FindVar(name);

	if (cv)
	{
		Cvar_Print(cv);
	}
	else
	{
		Com_Printf("Cvar %s does not exist.\n", name);
	}
}

/**
 * @brief Toggles a cvar for easy single key binding, optionally through a list of
 * given values
 */
void Cvar_Toggle_f(void)
{
	int  i, c = Cmd_Argc();
	char *curval;

	if (c < 2)
	{
		Com_Printf("usage: toggle <variable> [value1, value2, ...]\n");
		return;
	}

	if (c == 2)
	{
		Cvar_Set2(Cmd_Argv(1), va("%d", (Cvar_VariableValue(Cmd_Argv(1)) == 0.f)),
		          qfalse);
		return;
	}

	if (c == 3)
	{
		Com_Printf("toggle: nothing to toggle to\n");
		return;
	}

	curval = Cvar_VariableString(Cmd_Argv(1));

	// don't bother checking the last arg for a match since the desired
	// behaviour is the same as no match (set to the first argument)
	for (i = 2; i + 1 < c; i++)
	{
		if (strcmp(curval, Cmd_Argv(i)) == 0)
		{
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(i + 1), qfalse);
			return;
		}
	}

	// fallback
	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), qfalse);
}

/**
 * @brief Cycles a cvar for easy single key binding
 */
void Cvar_Cycle_f(void)
{
	int start, end, step, oldvalue, value;

	if (Cmd_Argc() < 4 || Cmd_Argc() > 5)
	{
		Com_Printf("usage: cycle <variable> <start> <end> [step]\n");
		return;
	}

	oldvalue = value = (int)(Cvar_VariableValue(Cmd_Argv(1)));
	start    = Q_atoi(Cmd_Argv(2));
	end      = Q_atoi(Cmd_Argv(3));

	if (Cmd_Argc() == 5)
	{
		step = abs(Q_atoi(Cmd_Argv(4)));
	}
	else
	{
		step = 1;
	}

	if (abs(end - start) < step)
	{
		step = 1;
	}

	if (end < start)
	{
		value -= step;
		if (value < end)
		{
			value = start - (step - (oldvalue - end + 1));
		}
	}
	else
	{
		value += step;
		if (value > end)
		{
			value = start + (step - (end - oldvalue + 1));
		}
	}

	Cvar_Set2(Cmd_Argv(1), va("%i", value), qfalse);
}

/**
 * @brief Allows setting and defining of arbitrary cvars from console, even if they
 * weren't declared in C code.
 */
void Cvar_Set_f(void)
{
	int    c;
	char   *cmd;
	cvar_t *v;

	c   = Cmd_Argc();
	cmd = Cmd_Argv(0);

	if (c < 2)
	{
		Com_Printf("usage: %s <variable> <value> [unsafe]\n", cmd);
		return;
	}
	if (c == 2)
	{
		Cvar_Print_f();
		return;
	}

	// ydnar: handle unsafe vars
	if (c >= 4 && !strcmp(Cmd_Argv(c - 1), "unsafe"))
	{
		c--;
		if (com_crashed != NULL && com_crashed->integer)
		{
			Com_Printf("%s is unsafe. Check com_crashed.\n", Cmd_Argv(1));
			return;
		}
	}

	v = Cvar_Set2(Cmd_Argv(1), Cmd_ArgsFromTo(2, c), qfalse);
	if (!v)
	{
		return;
	}
	switch (cmd[3])
	{
	case 'a':
		if (!(v->flags & CVAR_ARCHIVE))
		{
			v->flags           |= CVAR_ARCHIVE;
			cvar_modifiedFlags |= CVAR_ARCHIVE;
		}
		break;
	case 'u':
		if (!(v->flags & CVAR_USERINFO))
		{
			v->flags           |= CVAR_USERINFO;
			cvar_modifiedFlags |= CVAR_USERINFO;
		}
		break;
	case 's':
		if (!(v->flags & CVAR_SERVERINFO))
		{
			v->flags           |= CVAR_SERVERINFO;
			cvar_modifiedFlags |= CVAR_SERVERINFO;
		}
		break;
	}
}

/**
 * @brief Cvar_Reset_f
 */
void Cvar_Reset_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: reset <variable>\n");
		return;
	}
	Cvar_Reset(Cmd_Argv(1));
}

/**
 * @brief Appends lines containing "set variable value" for all variables
 * with the archive flag set to qtrue.
 * @param[in] f
 */
void Cvar_WriteVariables(fileHandle_t f)
{
	cvar_t *var;
	char   buffer[1024];

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!var->name)
		{
			continue;
		}

		if (var->flags & CVAR_ARCHIVE)
		{
			// write the latched value, even if it hasn't taken effect yet
			if (var->latchedString)
			{
				if (strlen(var->name) + strlen(var->latchedString) + 10 > sizeof(buffer))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: value of variable "
					                          "\"%s\" too long to write to file\n", var->name);
					continue;
				}

				if (var->flags & CVAR_UNSAFE)
				{
					Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\" unsafe\n", var->name, var->latchedString);
				}
				else
				{
					Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
				}
			}
			else
			{
				if (strlen(var->name) + strlen(var->string) + 10 > sizeof(buffer))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: value of variable "
					                          "\"%s\" too long to write to file\n", var->name);
					continue;
				}

				if (var->flags & CVAR_UNSAFE)
				{
					Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\" unsafe\n", var->name, var->string);
				}
				else
				{
					Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
				}
			}

			FS_Write(buffer, strlen(buffer), f);
		}
	}
}

/**
 * @brief Cvar_List_f
 */
void Cvar_List_f(void)
{
	cvar_t   *var;
	int      i = 0;
	int      selectedNum = 0;
	char     *match;
	qboolean raw = qfalse;

	if (Cmd_Argc() > 1)
	{
		match = Cmd_Argv(1);

		if (!Q_stricmp(match, "-raw"))
		{
			raw   = qtrue;
			match = (Cmd_Argc() > 2) ? Cmd_Argv(2) : NULL;
		}
	}
	else
	{
		match = NULL;
	}

	for (var = cvar_vars ; var ; var = var->next, i++)
	{
		if (!var->name || (match && !Com_Filter(match, var->name, qfalse)))
		{
			continue;
		}

		selectedNum++;

		if (var->flags & CVAR_SERVERINFO)
		{
			Com_Printf("S");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_SYSTEMINFO)
		{
			Com_Printf("s");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_USERINFO)
		{
			Com_Printf("U");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_ROM)
		{
			Com_Printf("R");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_INIT)
		{
			Com_Printf("I");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_ARCHIVE)
		{
			Com_Printf("A");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_LATCH)
		{
			Com_Printf("L");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_CHEAT)
		{
			Com_Printf("C");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_PROTECTED)
		{
			Com_Printf("P");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_TEMP)
		{
			Com_Printf("T");
		}
		else
		{
			Com_Printf(" ");
		}
		if (var->flags & CVAR_USER_CREATED)
		{
			Com_Printf("?");
		}
		else
		{
			Com_Printf(" ");
		}

		if (raw)
		{
			char *index;
			char *hat;

			Com_Printf(" %-35s \"", var->name);

			for (index = var->string; ; )
			{
				hat = strchr(index, '^');

				if (!hat)
				{
					break;
				}

				Com_Printf("%.*s", (int)(hat + 1 - index), index);
				index = hat + 1;
			}

			Com_Printf("%s\"\n", index);
		}
		else
		{
			Com_Printf(" %-35s \"%-s\" %s \"%-s\"\n", var->name, var->string, (strcmp(var->string, var->resetString)? "^3!":"-"),var->resetString);
		}
	}

	Com_Printf("\n%i total cvars, %i selected cvars\n", i, selectedNum);
	Com_Printf("%i cvar indexes\n", cvar_numIndexes);
}

/**
 * @brief Unsets a cvar
 * @param[in,out] cv
 * @return
 */
cvar_t *Cvar_Unset(cvar_t *cv)
{
	cvar_t *next = cv->next;

	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= cv->flags;

	if (cv->name)
	{
		Z_Free(cv->name);
	}
	if (cv->string)
	{
		Z_Free(cv->string);
	}
	if (cv->latchedString)
	{
		Z_Free(cv->latchedString);
	}
	if (cv->resetString)
	{
		Z_Free(cv->resetString);
	}
	if (cv->description)
	{
		Z_Free(cv->description);
	}

	if (cv->prev)
	{
		cv->prev->next = cv->next;
	}
	else
	{
		cvar_vars = cv->next;
	}
	if (cv->next)
	{
		cv->next->prev = cv->prev;
	}

	if (cv->hashPrev)
	{
		cv->hashPrev->hashNext = cv->hashNext;
	}
	else
	{
		hashTable[cv->hashIndex] = cv->hashNext;
	}
	if (cv->hashNext)
	{
		cv->hashNext->hashPrev = cv->hashPrev;
	}

	Com_Memset(cv, '\0', sizeof(*cv));

	return next;
}

/**
 * @brief Unsets a userdefined cvar
 */
void Cvar_Unset_f(void)
{
	cvar_t *cv;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: %s <varname>\n", Cmd_Argv(0));
		return;
	}

	cv = Cvar_FindVar(Cmd_Argv(1));

	if (!cv)
	{
		return;
	}

	if (cv->flags & CVAR_USER_CREATED)
	{
		Cvar_Unset(cv);
	}
	else
	{
		Com_Printf("Error: %s: Variable %s is not user created.\n", Cmd_Argv(0), cv->name);
	}
}

/**
 * @brief Resets all cvars to their hardcoded values and removes userdefined variables
 * and variables added via the VMs if requested.
 * @param unsetVM
 */
void Cvar_Restart(qboolean unsetVM)
{
	cvar_t *curvar = cvar_vars;

	while (curvar)
	{
		if ((curvar->flags & CVAR_USER_CREATED) ||
		    (unsetVM && (curvar->flags & CVAR_VM_CREATED)))
		{
			// throw out any variables the user/vm created
			curvar = Cvar_Unset(curvar);
			continue;
		}

		if (!(curvar->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)))
		{
			// Just reset the rest to their default values.
			Cvar_Set2(curvar->name, curvar->resetString, qfalse);
		}

		curvar = curvar->next;
	}
}

/**
 * @brief Resets all cvars to their hardcoded values
 */
void Cvar_Restart_f(void)
{
	Cvar_Restart(qfalse);
	Com_Printf("Cvars have been reset.\n");
}

/**
 * @brief Cvar_InfoString
 * @param[in] bit
 * @return
 */
char *Cvar_InfoString(int bit)
{
	static char info[MAX_INFO_STRING];
	cvar_t      *var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->name && (var->flags & bit))
		{
			Info_SetValueForKey(info, var->name, var->string);
		}
	}
	return info;
}

/**
 * @brief Handles large info strings ( CS_SYSTEMINFO )
 * @param bit
 * @return
 */
char *Cvar_InfoString_Big(int bit)
{
	static char info[BIG_INFO_STRING];
	cvar_t      *var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->name && (var->flags & bit))
		{
			Info_SetValueForKey_Big(info, var->name, var->string);
		}
	}
	return info;
}

/**
 * @brief Cvar_InfoStringBuffer
 * @param[in] bit
 * @param[out] buff
 * @param[in] buffsize
 */
void Cvar_InfoStringBuffer(int bit, char *buff, size_t buffsize)
{
	Q_strncpyz(buff, Cvar_InfoString(bit), buffsize);
}

/**
 * @brief cvar range check
 *
 * @param[in] cv
 * @param[in] minVal
 * @param[in] maxVal
 * @param[in] shouldBeIntegral
 *
 * @note This isn't static so visible to all!
 */
void Cvar_CheckRange(cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral)
{
	cv->validate = qtrue;
	cv->min      = minVal;
	cv->max      = maxVal;
	cv->integral = shouldBeIntegral;

	// Force an initial range check
	Cvar_Set(cv->name, cv->string);

	/*
	if (shouldBeIntegral)
	{
	    if (cv->value != cv->integer)
	    {
	        Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value);
	        Cvar_Set(cv->name, va("%d", cv->integer));
	    }
	}

	if (cv->value < minVal)
	{
	    if (shouldBeIntegral)
	    {
	        Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%i < %i)\n", cv->name, cv->integer, (int) minVal);
	        Cvar_Set(cv->name, va("%i", (int) minVal));
	    }
	    else
	    {
	        Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal);
	        Cvar_Set(cv->name, va("%f", minVal));
	    }
	}
	else if (cv->value > maxVal)
	{
	    if (shouldBeIntegral)
	    {
	        Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%i > %i)\n", cv->name, cv->integer, (int) maxVal);
	        Cvar_Set(cv->name, va("%i", (int) maxVal));
	    }
	    else
	    {
	        Com_Printf(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal);
	        Cvar_Set(cv->name, va("%f", maxVal));
	    }
	}
	*/
}

/**
 * @brief Cvar_SetDescription
 * @param[in,out] cv
 * @param[in] varDescription
 */
void Cvar_SetDescription(cvar_t *cv, const char *varDescription)
{
	if (varDescription && varDescription[0] != '\0')
	{
		if (cv->description != NULL)
		{
			Z_Free(cv->description);
		}
		cv->description = CopyString(varDescription);
	}
}

/**
 * @brief Basically a slightly modified Cvar_Get for the interpreted modules
 *
 * @param[in,out] vmCvar
 * @param[in] varName
 * @param[in] defaultValue
 * @param[in] flags
 */
void Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags)
{
	cvar_t *cv;

	// There is code in Cvar_Get to prevent CVAR_ROM cvars being changed by the
	// user. In other words CVAR_ARCHIVE and CVAR_ROM are mutually exclusive
	// flags. Unfortunately some historical game code (including single player
	// baseq3) sets both flags. We unset CVAR_ROM for such cvars.
	//
	// Update: We no longer do this for ETL
	// All cvars containing both flags are obsolte/unused and deleted in legacy
	// We no longer unset CVAR_ROM - instead we just don't register
	// Side note: ET mods/vanilla don't use affected cvars but they try to register (ui_botsFile, ui_spX ...)
	if ((flags & (CVAR_ARCHIVE | CVAR_ROM)) == (CVAR_ARCHIVE | CVAR_ROM))
	{
		//Com_DPrintf(S_COLOR_YELLOW "WARNING: Unsetting CVAR_ROM cvar '%s', since it is also CVAR_ARCHIVE\n", varName);
		//flags &= ~CVAR_ROM;
		Com_DPrintf(S_COLOR_YELLOW "Cvar_Register WARNING: registering cvar '%s' failed - CVAR_ARCHIVE and CVAR_ROM are exclusive flags!\n", varName);
		return;
	}

	// Don't allow VM to specific a different creator or other internal flags.
	if (flags & CVAR_USER_CREATED)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to set CVAR_USER_CREATED on cvar '%s'\n", varName);
		flags &= ~CVAR_USER_CREATED;
	}
	if (flags & CVAR_SERVER_CREATED)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to set CVAR_SERVER_CREATED on cvar '%s'\n", varName);
		flags &= ~CVAR_SERVER_CREATED;
	}
	if (flags & CVAR_PROTECTED)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to set CVAR_PROTECTED on cvar '%s'\n", varName);
		flags &= ~CVAR_PROTECTED;
	}
	if (flags & CVAR_MODIFIED)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to set CVAR_MODIFIED on cvar '%s'\n", varName);
		flags &= ~CVAR_MODIFIED;
	}
	if (flags & CVAR_NONEXISTENT)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to set CVAR_NONEXISTENT on cvar '%s'\n", varName);
		flags &= ~CVAR_NONEXISTENT;
	}

	cv = Cvar_FindVar(varName);

	// Don't modify cvar if it's protected.
	if (cv && (cv->flags & CVAR_PROTECTED))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to register protected cvar '%s' with value '%s'%s\n",
		varName, defaultValue, (flags & ~cv->flags ) != 0 ? " and new flags" : "" );
	}

	// FIXME/inspect: this causes an endless loop while loading some maps (f.e. fueldump)
 	// CVAR_LATCH | CVAR_ARCHIVE and CVAR_LATCH | CVAR_SERVERINFO cvars from mod game are affected:
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_gametype' with value '4'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'sv_maxclients' with value '20'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_maxGameClients' with value '0'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'dedicated' with value '0'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_maxlives' with value '0'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_maxlivesRespawnPenalty' with value '0'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_countryflags' with value '1'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_corpses' with value '0'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_skillRating' with value '2'
	//WARNING: VM tried to register engine latch cvar to latched value: cvar 'g_multiview' with value '0'
/*
	// Don't set engine latch cvar to latched value.
	else if (cv && (cv->flags & CVAR_LATCH) && !(cv->flags & CVAR_VM_CREATED))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: VM tried to register engine latch cvar to latched value: cvar '%s' with value '%s'%s\n",
		varName, defaultValue, (flags & ~cv->flags ) != 0 ? " and new flags" : "" );

		flags &= ~CVAR_VM_CREATED;
		cv->flags |= flags;
		cvar_modifiedFlags |= flags;
	}
*/
	else
	{
		cv = Cvar_Get(varName, defaultValue, flags | CVAR_VM_CREATED);
	}

	if (!vmCvar)
	{
		return;
	}
	vmCvar->handle            = cv - cvar_indexes;
	vmCvar->modificationCount = -1;
	Cvar_Update(vmCvar);
}

/**
 * @brief Updates an interpreted modules' version of a cvar
 * @param[in,out] vmCvar
 */
void Cvar_Update(vmCvar_t *vmCvar)
{
	cvar_t *cv = NULL;
	etl_assert(vmCvar);

	if ((unsigned)vmCvar->handle >= cvar_numIndexes)
	{
		Com_Error(ERR_DROP, "Cvar_Update: handle out of range");
	}

	cv = cvar_indexes + vmCvar->handle;

	if (cv->modificationCount == vmCvar->modificationCount)
	{
		return;
	}
	if (!cv->string)
	{
		return;     // variable might have been cleared by a cvar_restart
	}
	vmCvar->modificationCount = cv->modificationCount;
	if (strlen(cv->string) + 1 > MAX_CVAR_VALUE_STRING)
	{
		Com_Error(ERR_DROP, "Cvar_Update: src %s length %u exceeds MAX_CVAR_VALUE_STRING",
		          cv->string,
		          (unsigned int) strlen(cv->string));
	}
	Q_strncpyz(vmCvar->string, cv->string, MAX_CVAR_VALUE_STRING);

	vmCvar->value   = cv->value;
	vmCvar->integer = cv->integer;
}

/**
 * @brief Cvar_CompleteCvarName
 * @param[in] args
 * @param[in] argNum
 */
void Cvar_CompleteCvarName(char *args, int argNum)
{
	if (argNum == 2)
	{
		// Skip "<cmd> "
		char *p = Com_SkipTokens(args, 1, " ");

		if (p > args)
		{
			Field_CompleteCommand(p, qfalse, qtrue);
		}
	}
}

/**
 * @brief Reads in all archived cvars
 */
void Cvar_Init(void)
{
	Com_Memset(cvar_indexes, '\0', sizeof(cvar_indexes));
	Com_Memset(hashTable, '\0', sizeof(hashTable));

	cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);

	Cmd_AddCommand("print", Cvar_Print_f, "Prints the contents of a cvar.");
	Cmd_AddCommand("toggle", Cvar_Toggle_f, "Toggles a cvar for easy single key binding, optionally through a list of given values.", Cvar_CompleteCvarName);
	Cmd_AddCommand("cycle", Cvar_Cycle_f, "Cycles a cvar for easy single key binding.", Cvar_CompleteCvarName);
	Cmd_AddCommand("set", Cvar_Set_f, "Allows setting and defining of cvars from console.", Cvar_CompleteCvarName);
	Cmd_AddCommand("sets", Cvar_Set_f, "Allows setting and defining of cvars from console. Serverinfo flag is set.", Cvar_CompleteCvarName);
	Cmd_AddCommand("setu", Cvar_Set_f, "Allows setting and defining of cvars from console. Userinfo flag is set.", Cvar_CompleteCvarName);
	Cmd_AddCommand("seta", Cvar_Set_f, "Allows setting and defining of cvars from console. Archive flag is set.", Cvar_CompleteCvarName);
	Cmd_AddCommand("reset", Cvar_Reset_f, "Resets a specific cvar.", Cvar_CompleteCvarName);
	Cmd_AddCommand("unset", Cvar_Unset_f, "Unsets a userdefined cvar.", Cvar_CompleteCvarName);
	Cmd_AddCommand("cvarlist", Cvar_List_f, "Prints a list of all cvars.");
	Cmd_AddCommand("cvar_restart", Cvar_Restart_f, "Resets all cvars to their hardcoded values.");
}
