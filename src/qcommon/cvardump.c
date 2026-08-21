/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file cvardump.c
 * @brief Cvar dump command implementation.
 */

#include "q_shared.h"
#include "qcommon.h"
#include "json.h"

/**
 * Cvar list head is owned by cvar.c.
 */
extern cvar_t *cvar_vars;

/**
 * @brief Appends a symbolic cvar flag name to a string buffer.
 * @param[in,out] buffer
 * @param[in] bufferSize
 * @param[in,out] first
 * @param[in] flagName
 */
static void Cvar_DumpAppendFlagName(char *buffer, size_t bufferSize, qboolean *first, const char *flagName)
{
	if (!*first)
	{
		Q_strcat(buffer, bufferSize, "|");
	}

	Q_strcat(buffer, bufferSize, flagName);
	*first = qfalse;
}

/**
 * @brief Builds a compact machine-friendly flag list for one cvar.
 * @param[in] flags
 * @param[out] buffer
 * @param[in] bufferSize
 */
static void Cvar_DumpBuildFlagNames(cvarFlags_t flags, char *buffer, size_t bufferSize)
{
	qboolean first = qtrue;

	*buffer = '\0';

#define CVAR_DUMP_FLAG_NAME(bit) \
		if (flags & bit) \
		{ \
			Cvar_DumpAppendFlagName(buffer, bufferSize, &first, #bit); \
		}

	CVAR_DUMP_FLAG_NAME(CVAR_ARCHIVE);
	CVAR_DUMP_FLAG_NAME(CVAR_USERINFO);
	CVAR_DUMP_FLAG_NAME(CVAR_SERVERINFO);
	CVAR_DUMP_FLAG_NAME(CVAR_SYSTEMINFO);
	CVAR_DUMP_FLAG_NAME(CVAR_INIT);
	CVAR_DUMP_FLAG_NAME(CVAR_LATCH);
	CVAR_DUMP_FLAG_NAME(CVAR_ROM);
	CVAR_DUMP_FLAG_NAME(CVAR_USER_CREATED);
	CVAR_DUMP_FLAG_NAME(CVAR_TEMP);
	CVAR_DUMP_FLAG_NAME(CVAR_CHEAT);
	CVAR_DUMP_FLAG_NAME(CVAR_NORESTART);
	CVAR_DUMP_FLAG_NAME(CVAR_WOLFINFO);
	CVAR_DUMP_FLAG_NAME(CVAR_UNSAFE);
	CVAR_DUMP_FLAG_NAME(CVAR_SERVERINFO_NOUPDATE);
	CVAR_DUMP_FLAG_NAME(CVAR_SERVER_CREATED);
	CVAR_DUMP_FLAG_NAME(CVAR_VM_CREATED);
	CVAR_DUMP_FLAG_NAME(CVAR_PROTECTED);
	CVAR_DUMP_FLAG_NAME(CVAR_SHADER);
	CVAR_DUMP_FLAG_NAME(CVAR_NOTABCOMPLETE);
	CVAR_DUMP_FLAG_NAME(CVAR_NODEFAULT);
#undef CVAR_DUMP_FLAG_NAME
}

/**
 * @brief Finds the next cvar by ascending name.
 *
 * @details Uses repeated scans to avoid temporary allocations.
 *
 * @param[in] lastName Last emitted cvar name, or NULL for first item.
 * @return Next cvar pointer or NULL when no more cvars exist.
 */
static cvar_t *Cvar_DumpFindNextByName(const char *lastName)
{
	cvar_t *var;
	cvar_t *best = NULL;

	for (var = cvar_vars; var; var = var->next)
	{
		if (!var->name)
		{
			continue;
		}

		if (lastName && Q_stricmp(var->name, lastName) <= 0)
		{
			continue;
		}

		if (!best || Q_stricmp(var->name, best->name) < 0)
		{
			best = var;
		}
	}

	return best;
}

/**
 * @brief Dumps all cvars and selected metadata to a file in JSONL.
 */
void Cvar_Dump_f(void)
{
	char       dumpPath[MAX_OSPATH];
	char       flagNames[512];
	const char *lastName = NULL;
	cvar_t     *var;
	FILE       *f;
	int        dumpedCount = 0;

	Q_strncpyz(dumpPath, FS_BuildOSPath(Cvar_VariableString("fs_homepath"), "", "cvardump.jsonl"), sizeof(dumpPath));

	f = Sys_FOpen(dumpPath, "wb");
	if (!f)
	{
		Com_Printf(S_COLOR_YELLOW "dumpcvars: could not open '%s' for writing\n", dumpPath);
		return;
	}

	while ((var = Cvar_DumpFindNextByName(lastName)) != NULL)
	{
		cJSON *json;
		char  *line;

		Cvar_DumpBuildFlagNames(var->flags, flagNames, sizeof(flagNames));

		json = cJSON_CreateObject();
		if (!json)
		{
			Com_Printf(S_COLOR_YELLOW "dumpcvars: could not allocate json object\n");
			fclose(f);
			return;
		}

		cJSON_AddStringToObject(json, "name", var->name);
		cJSON_AddStringToObject(json, "default", var->resetString ? var->resetString : "");
		cJSON_AddStringToObject(json, "latched", var->latchedString ? var->latchedString : "");
		cJSON_AddStringToObject(json, "flags", flagNames);
		cJSON_AddBoolToObject(json, "validate", var->validate);
		cJSON_AddBoolToObject(json, "integral", var->integral);

		if (var->validate)
		{
			cJSON_AddNumberToObject(json, "min", var->min);
			cJSON_AddNumberToObject(json, "max", var->max);
		}
		else
		{
			cJSON_AddNullToObject(json, "min");
			cJSON_AddNullToObject(json, "max");
		}

		cJSON_AddStringToObject(json, "description", var->description ? var->description : "");

		line = cJSON_PrintUnformatted(json);
		cJSON_Delete(json);

		if (!line)
		{
			Com_Printf(S_COLOR_YELLOW "dumpcvars: could not serialise cvar '%s'\n", var->name);
			fclose(f);
			return;
		}

		fprintf(f, "%s\n", line);
		cJSON_free(line);

		lastName = var->name;
		dumpedCount++;
	}

	fclose(f);
	Com_Printf("dumpcvars: wrote %d cvars to %s\n", dumpedCount, dumpPath);
}
