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
 * @file cg_hud_io.c
 * @brief Handle hud file loading and saving
*/

#include "cg_local.h"
#include "json.h"

#define HUDS_DEFAULT_PATH "ui/huds.hud"
#define HUDS_USER_LEGACY_PATH "profiles/%s/hud.dat"
#define HUDS_USER_DIR_PATH "profiles/%s/huds"
#define HUDS_USER_BACKUP_PATH HUDS_USER_DIR_PATH "/hud_v%i_backup(%s).dat"
#define HUDS_FILE_LIST_BUFFER_SIZE 32768

typedef struct
{
	qboolean invalid;                                   //< file is just plain invalid
	qboolean calcAnchors;                               //< added in version 2
	qboolean replaceNumberByName;                       //< added in version 3
	char numberToNameTableReminder[MAXHUDS][MAX_QPATH]; //< added in version 3
	qboolean shiftHealthBarDynamicColorStyle;           //< added in version 4
	qboolean replaceWeaponIconStyle;                    //< added in version 5
	qboolean addNoEchoToPopupmessageFilter;             //< added in version 5
	qboolean shiftHealthBarDynamicColorStyle2;          //< added in version 6
	qboolean moveBarStyleIntoOwnField;                 //< added in version 7
} hudFileUpgrades_t;

static uint32_t CG_CompareHudComponents(hudStucture_t *hud, hudComponent_t *comp, hudStucture_t *parentHud, hudComponent_t *parentComp);

/**
 * @brief Writes the timestamp format used by HUD backup filenames
 * @param[out] output
 * @param[in] len
 */
static void CG_HudTimestamp(char *output, int len)
{
	qtime_t ct;

	output[0] = '\0';
	trap_RealTime(&ct);
	Com_sprintf(output, len, "%d-%02d-%02d-%02d%02d%02d", 1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec);
}

/**
 * @brief Returns the profile-local directory for versioned HUDs
 * @param[out] output
 * @param[in] len
 */
static void CG_HudVersionedDirPath(char *output, int len)
{
	char profile[MAX_OSPATH];

	profile[0] = '\0';
	output[0]  = '\0';
	trap_Cvar_VariableStringBuffer("cl_profile", profile, MAX_OSPATH);
	Com_sprintf(output, len, HUDS_USER_DIR_PATH, profile);
}

/**
 * @brief Returns the canonical path for one JSON HUD version
 * @param[out] output
 * @param[in] len
 * @param[in] version
 */
static void CG_HudFilePathForVersion(char *output, int len, int version)
{
	char hudDir[MAX_OSPATH];

	CG_HudVersionedDirPath(hudDir, sizeof(hudDir));
	Com_sprintf(output, len, "%s/hud_v%i.dat", hudDir, version);
}

/**
 * @brief CG_HudFilePath
 * @return
 */
static const char *CG_HudFilePath()
{
	static char filePath[MAX_OSPATH] = { 0 };

	if (!filePath[0])
	{
		CG_HudFilePathForVersion(filePath, MAX_OSPATH, CURRENT_HUD_JSON_VERSION);
	}

	return filePath;
}

/**
 * @brief Returns the collision-checked migration backup path
 * @param[out] output
 * @param[in] len
 * @param[in] version
 */
static void CG_HudBackupFilePathForVersion(char *output, int len, int version)
{
	char profile[MAX_OSPATH];
	char timestamp[32];

	profile[0] = '\0';
	output[0]  = '\0';
	trap_Cvar_VariableStringBuffer("cl_profile", profile, MAX_OSPATH);
	CG_HudTimestamp(timestamp, sizeof(timestamp));
	Com_sprintf(output, len, HUDS_USER_BACKUP_PATH, profile, version, timestamp);
}

/**
 * @brief Keeps invalid files next to their original name
 * @param[in] filename
 * @param[out] output
 * @param[in] len
 */
static void CG_HudInvalidFilePath(const char *filename, char *output, int len)
{
	const char *scan;
	const char *dot;
	char       timestamp[32];
	int        prefixLen;

	output[0] = '\0';
	dot       = NULL;

	// Only a dot after the last path separator is treated as an extension.
	for (scan = filename; *scan; scan++)
	{
		if (*scan == '/' || *scan == '\\')
		{
			dot = NULL;
		}
		else if (*scan == '.')
		{
			dot = scan;
		}
	}

	CG_HudTimestamp(timestamp, sizeof(timestamp));

	if (!dot)
	{
		Com_sprintf(output, len, "%s_invalid(%s)", filename, timestamp);
		return;
	}

	prefixLen = (int)(dot - filename);
	if (prefixLen + 1 >= len)
	{
		return;
	}

	Q_strncpyz(output, filename, prefixLen + 1);
	Q_strcat(output, len, va("_invalid(%s)%s", timestamp, dot));
}

/**
 * @brief Treats zero-length files as existing migration targets
 * @param[in] filename
 * @return
 */
static qboolean CG_HudFileExists(const char *filename)
{
	fileHandle_t file;
	int          len;

	file = 0;
	len  = trap_FS_FOpenFile(filename, &file, FS_READ);
	if (file)
	{
		trap_FS_FCloseFile(file);
	}

	return len >= 0 ? qtrue : qfalse;
}

/**
 * @brief Reads only enough JSON metadata to drive migration
 * @param[in] filename
 * @param[out] version
 * @return
 */
static qboolean CG_ReadHudFileVersion(const char *filename, int *version)
{
	cJSON *root;
	cJSON *versionItem;

	*version = 0;
	root     = Q_FSReadJsonFrom(filename);
	if (!root)
	{
		return qfalse;
	}

	versionItem = cJSON_GetObjectItem(root, "version");
	if (!versionItem || !cJSON_IsNumber(versionItem))
	{
		cJSON_Delete(root);
		return qfalse;
	}

	*version = (int)cJSON_GetNumberValue(versionItem);
	cJSON_Delete(root);

	return *version > 0 ? qtrue : qfalse;
}

/**
 * @brief CG_MoveHudFile
 * @details HUD migration intentionally avoids filesystem rename semantics because the
 * cgame VM only has portable read, write, close, list, and delete syscalls.
 *
 * A "move" is therefore done as a transactional copy:
 *
 * 1. Refuse to start when the target path already exists. This keeps from
 *    accidentally overwriting existing HUDs, timestamped backups, and
 *    timestamped invalid files.
 * 2. Copy the complete source file into the target path and close both files.
 *    Closing first makes sure a later validation read sees what was written.
 * 3. Re-open the target and validate it. Normal HUD moves validate the JSON
 *    version field against the expected version. Invalid HUD preservation
 *    cannot parse JSON by definition, so it validates that the copied size
 *    matches the original source size.
 * 4. Delete the source only after the target has passed validation. If any
 *    step fails, the source is left in place so migration never destroys the
 *    user's only copy of the HUD.
 *
 * @param[in] source
 * @param[in] target
 * @param[in] expectedVersion
 * @return
 */
static qboolean CG_MoveHudFile(const char *source, const char *target, int expectedVersion)
{
	fileHandle_t sourceFile;
	fileHandle_t targetFile;
	byte         buffer[16384];
	int          sourceLen;
	int          targetLen;
	int          remaining;
	int          chunk;
	int          written;
	int          targetVersion;

	// Do the collision check before opening the source. A pre-existing target
	// means the migration would overwrite user data, so the move must abort.
	if (CG_HudFileExists(target))
	{
		CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: target already exists '%s'\n", target);
		return qfalse;
	}

	// Open the source first so failures never create an empty target file.
	{
		sourceFile = 0;
		sourceLen  = trap_FS_FOpenFile(source, &sourceFile, FS_READ);
		if (sourceLen < 0 || !sourceFile)
		{
			if (sourceFile)
			{
				trap_FS_FCloseFile(sourceFile);
			}
			CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: failed to read source '%s'\n", source);
			return qfalse;
		}
	}

	// Only open the target after the source is known to be readable.
	targetFile = 0;
	if (trap_FS_FOpenFile(target, &targetFile, FS_WRITE) < 0 || !targetFile)
	{
		trap_FS_FCloseFile(sourceFile);
		CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: failed to write target '%s'\n", target);
		return qfalse;
	}

	// Copy in fixed-size chunks to avoid allocating a buffer sized to the HUD.
	remaining = sourceLen;
	while (remaining > 0)
	{
		chunk = remaining > (int)sizeof(buffer) ? (int)sizeof(buffer) : remaining;
		trap_FS_Read(buffer, chunk, sourceFile);
		written = trap_FS_Write(buffer, chunk, targetFile);
		if (written != chunk)
		{
			trap_FS_FCloseFile(sourceFile);
			trap_FS_FCloseFile(targetFile);
			trap_FS_Delete(target);
			CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: short write while moving '%s' to '%s'\n", source, target);
			return qfalse;
		}
		remaining -= chunk;
	}

	// Validation is done through a new read handle, so close the write handle
	// first and make the just-written target visible to the filesystem layer.
	trap_FS_FCloseFile(sourceFile);
	trap_FS_FCloseFile(targetFile);

	// Valid HUD migrations re-read the copied JSON. Invalid HUD preservation
	// can only validate that the copied file exists with the expected size.
	if (expectedVersion > 0)
	{
		if (!CG_ReadHudFileVersion(target, &targetVersion) || targetVersion != expectedVersion)
		{
			trap_FS_Delete(target);
			CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: target validation failed '%s'\n", target);
			return qfalse;
		}
	}
	else
	{
		// Invalid HUD files cannot be parsed, so the only meaningful
		// post-copy validation is that the target has the same byte length.
		targetFile = 0;
		targetLen  = trap_FS_FOpenFile(target, &targetFile, FS_READ);
		if (targetFile)
		{
			trap_FS_FCloseFile(targetFile);
		}

		if (targetLen != sourceLen)
		{
			trap_FS_Delete(target);
			CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: target size validation failed '%s'\n", target);
			return qfalse;
		}
	}

	// Source deletion is the final step. Every earlier failure path returns
	// before this point, leaving the original HUD file untouched.
	if (!trap_FS_Delete(source))
	{
		CG_Printf(S_COLOR_RED "ERROR CG_MoveHudFile: failed to delete source '%s'\n", source);
		return qfalse;
	}

	CG_Printf(S_COLOR_CYAN "Moved HUD file '%s' to '%s'\n", source, target);
	return qtrue;
}

/**
 * @brief Accepts only canonical hud_v<version>.dat files
 * @param[in] filename
 * @param[out] version
 * @return
 */
static qboolean CG_ParseHudVersionFilename(const char *filename, int *version)
{
	const char *scan;
	int        parsedVersion;

	*version = 0;
	if (Q_strncmp(filename, "hud_v", 5))
	{
		return qfalse;
	}

	scan = filename + 5;
	if (!Q_isnumeric(*scan))
	{
		return qfalse;
	}

	parsedVersion = 0;
	while (Q_isnumeric(*scan))
	{
		parsedVersion = parsedVersion * 10 + (*scan - '0');
		scan++;
	}

	if (Q_stricmp(scan, ".dat"))
	{
		return qfalse;
	}

	*version = parsedVersion;
	return qtrue;
}

/**
 * @brief CG_HudListSort
 * @param[in] a
 * @param[in] b
 * @return
 */
static int QDECL CG_HudListSort(const void *a, const void *b)
{
	return ((*(hudStucture_t **) a)->name - (*(hudStucture_t **) b)->name);
}

/**
 * @brief CG_GenerateHudList
 */
static void CG_GenerateHudList()
{
	unsigned int i, x = 0;

	Com_Memset(hudData.list, 0, sizeof(hudStucture_t *) * MAXHUDS);

	for (i = 0; i < MAXHUDS; i++)
	{
		if (!hudData.huds[i].active)
		{
			continue;
		}

		hudData.list[x++] = &hudData.huds[i];
	}

	qsort(hudData.list, x, sizeof(hudStucture_t *), CG_HudListSort);
}

/**
 * @brief CG_GetFreeHud
 * @return
 */
hudStucture_t *CG_GetFreeHud()
{
	unsigned int i;

	for (i = 0; i < MAXHUDS; i++)
	{
		if (!hudData.huds[i].active)
		{
			hudStucture_t *tmpHud = &hudData.huds[i];
			Com_Memset(tmpHud, 0, sizeof(hudStucture_t));

			// save hud index
			tmpHud->hudnumber = i;

			// reset offsets
			for (i = 0; hudComponentFields[i].name; i++)
			{
				hudComponent_t *component = (hudComponent_t *)((char * )tmpHud + hudComponentFields[i].offset);
				component->offset = 999;
			}

			return tmpHud;
		}
	}

	CG_Error("All huds are already in use cannot register a new one!\n");
	return NULL;
}

/**
 * @brief CG_RegisterHud
 * @param[in] hud
 */
void CG_RegisterHud(hudStucture_t *hud)
{
	if (hud->active)
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: trying to register a hud that is already registered!\n");
		return;
	}

	hud->active = qtrue;
	hudData.count++;
	CG_HudComponentsFill(hud);
	CG_GenerateHudList();
}

/**
 * @brief CG_FreeHud
 * @param[in,out] hud
 */
void CG_FreeHud(hudStucture_t *hud)
{
	if (!hud->active)
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: trying to un-register a hud that is already un-registered!\n");
		return;
	}
	hudData.count--;
	hud->active = qfalse;
	CG_GenerateHudList();
}

/**
 * @brief CG_isHudNumberAvailable checks if the hud by said number is available for use, 0 to 2 are forbidden.
 * @param[in] number
 * @return
 */
static qboolean CG_isHudNumberAvailable(int number)
{
	// values 0 is used by the default hud's
	if (number <= 0 || number >= MAXHUDS)
	{
		Com_Printf(S_COLOR_RED "CG_isHudNumberAvailable: invalid HUD number %i, allowed values: 1 - %i\n", number, MAXHUDS);
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief CG_AdjustXFromHudFile
 * @param[in] x
 * @param[in] w
 * @return
 */
float CG_AdjustXFromHudFile(float x, float w)
{
	if (Ccg_Is43Screen())
	{
		return x;
	}

	if ((int)(x + w * .5f) == 320)
	{
		return Ccg_WideX(x) + (Ccg_WideX(w) - w) * .5f;
	}
	else if (x <= 320)
	{
		return Ccg_WideX(x);
	}
	else
	{
		return Ccg_WideX(x + w) - w;
	}
}

/**
 * @brief CG_AdjustXToHudFile
 * @param[in] x
 * @param[in] w
 * @return
 */
float CG_AdjustXToHudFile(float x, float w)
{
	float wideX = Ccg_WideX(320);
	if (Ccg_Is43Screen())
	{
		return x;
	}

	if ((int)(x + w * .5f) == (int)wideX)
	{
		return Ccg_WideXReverse(x - ((Ccg_WideX(w) - w) * .5f));
	}
	else if (x <= wideX)
	{
		return Ccg_WideXReverse(x);
	}
	else
	{
		return Ccg_WideXReverse(x + w) - w;
	}
}

/**
 * @brief CG_FindComponentByName
 * @param[in] hud
 * @param[in] name
 * @return
 */
hudComponent_t *CG_FindComponentByName(hudStucture_t *hud, const char *name)
{
	int i;

	if (!hud || !name || !*name)
	{
		return NULL;
	}

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (Q_stricmp(name, hudComponentFields[i].name))
		{
			continue;
		}

		return (hudComponent_t *)((byte * )hud + hudComponentFields[i].offset);
	}

	return NULL;
}

/**
 * @brief CG_FindComponentName
 * @param[in] hud
 * @param[in] comp
 * @return
 */
const char *CG_FindComponentName(hudStucture_t *hud, hudComponent_t *comp)
{
	int i;

	if (!hud || !comp)
	{
		return NULL;
	}

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (comp == (hudComponent_t *)((byte * )hud + hudComponentFields[i].offset))
		{
			return hudComponentFields[i].name;
		}
	}

	return NULL;
}

/**
 * @brief Safely clone the hud structure and update the pointers to match the new source
 * @param[in,out] target
 * @param[in] source
 */
void CG_CloneHud(hudStucture_t *target, hudStucture_t *source)
{
	unsigned int   i;
	qboolean       hudActive;
	hudComponent_t *tmp, *tmp2;

	etl_assert(target && source);

	hudActive = target->active;

	Com_Memcpy(target, source, sizeof(hudStucture_t));

	for (i = 0; hudComponentFields[i].name; i++)
	{
		tmp  = (hudComponent_t *)((byte * )source + hudComponentFields[i].offset);
		tmp2 = (hudComponent_t *)((byte * )target + hudComponentFields[i].offset);

		if (tmp->parentAnchor.parent)
		{
			const char *parentName = CG_FindComponentName(source, tmp->parentAnchor.parent);
			if (parentName && *parentName)
			{
				tmp2->parentAnchor.parent = CG_FindComponentByName(target, parentName);
			}
			else
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: could not find component name when cloning hud\n");
				tmp2->parentAnchor.parent = NULL;
			}
		}
	}

	// restore the active status to keep the registration in line..
	target->active = hudActive;
	CG_HudComponentsFill(target);
}

/**
 * @brief CG_CloneHudComponent
 * @param[in] hud
 * @param[in] name
 * @param[in] targetHud
 * @param[out] out
 */
static ID_INLINE void CG_CloneHudComponent(hudStucture_t *hud, const char *name, hudStucture_t *targetHud, hudComponent_t *out)
{
	hudComponent_t *comp = CG_FindComponentByName(hud, name);

	if (comp)
	{
		Com_Memcpy(out, comp, sizeof(hudComponent_t));
		Com_Memset(&out->location, 0, sizeof(rectDef_t));
		out->computed = qfalse;

		if (comp->parentAnchor.parent)
		{
			const char *parentName = CG_FindComponentName(hud, comp);
			out->parentAnchor.parent = CG_FindComponentByName(targetHud, parentName);
		}
	}
}

/**
 * @brief CG_ColorToHex
 * @param[in] vec
 * @return
 */
static ID_INLINE char *CG_ColorToHex(const vec_t *vec)
{
	return va("%02x%02x%02x%02x", ((int) (vec[0] * 255.f)) & 0xff, ((int) (vec[1] * 255.f)) & 0xff, ((int) (vec[2] * 255.f)) & 0xff, ((int) (vec[3] * 255.f)) & 0xff);
}

/**
 * @brief CG_CreateHudObject
 * @param[in] hud
 * @return
 */
static cJSON *CG_CreateHudObject(hudStucture_t *hud)
{
	int            j;
	uint32_t       flags;
	hudStucture_t  *parent = NULL;
	hudComponent_t *comp, *parentComp;
	cJSON          *compObj, *tmpObj, *compsObj, *hudObj = cJSON_CreateObject();

	cJSON_AddStringToObject(hudObj, "name", hud->name);

	// no value or null defaults to hud 0
	if (hud->parentNumber > 0)
	{
		cJSON_AddStringToObject(hudObj, "parent", hud->parent);
	}
	// parent -1 means that this hud has no parent so write a implicit false flag
	else if (hud->parentNumber < 0)
	{
		cJSON_AddFalseToObject(hudObj, "parent");
	}

	compsObj = cJSON_AddObjectToObject(hudObj, "components");

	if (hud->parentNumber >= 0)
	{
		parent = CG_GetHudByNumber(hud->parentNumber);

		// check that the parent hud still exists, otherwise mark the hud as "original" and write out all fields.
		if (!parent)
		{
			hud->parentNumber = -1;
		}
	}

	for (j = 0; hudComponentFields[j].name; j++)
	{
		if (hudComponentFields[j].isAlias)
		{
			continue;
		}

		comp = (hudComponent_t *)((char *)hud + hudComponentFields[j].offset);

		if (parent)
		{
			parentComp = CG_FindComponentByName(parent, hudComponentFields[j].name);
			flags      = CG_CompareHudComponents(hud, comp, parent, parentComp);
			if (!flags)
			{
				continue;
			}
		}
		else
		{
			flags = UINT32_MAX;
		}

		compObj = cJSON_AddObjectToObject(compsObj, hudComponentFields[j].name);

		if (flags & BIT(0))
		{
			tmpObj = cJSON_AddObjectToObject(compObj, "rect");
			{
				cJSON_AddNumberToObject(tmpObj, "x", comp->internalLocation.x);
				cJSON_AddNumberToObject(tmpObj, "y", comp->internalLocation.y);
				cJSON_AddNumberToObject(tmpObj, "w", comp->internalLocation.w);
				cJSON_AddNumberToObject(tmpObj, "h", comp->internalLocation.h);
			}
		}

		if (flags & BIT(1))
		{
			cJSON_AddBoolToObject(compObj, "visible", comp->visible);
		}

		if (flags & BIT(2))
		{
			cJSON_AddNumberToObject(compObj, "style", comp->style);
		}

		if (flags & BIT(3))
		{
			cJSON_AddNumberToObject(compObj, "scale", comp->scale);
		}

		if (flags & BIT(4))
		{
			cJSON_AddStringToObject(compObj, "mainColor", CG_ColorToHex(comp->colorMain));
		}

		if (flags & BIT(5))
		{
			cJSON_AddStringToObject(compObj, "secondaryColor", CG_ColorToHex(comp->colorSecondary));
		}

		if (flags & BIT(6))
		{
			cJSON_AddBoolToObject(compObj, "showBackGround", comp->showBackGround);
		}

		if (flags & BIT(7))
		{
			cJSON_AddStringToObject(compObj, "backgroundColor", CG_ColorToHex(comp->colorBackground));
		}

		if (flags & BIT(8))
		{
			cJSON_AddBoolToObject(compObj, "showBorder", comp->showBorder);
		}

		if (flags & BIT(9))
		{
			cJSON_AddStringToObject(compObj, "borderColor", CG_ColorToHex(comp->colorBorder));
		}

		if (flags & BIT(10))
		{
			cJSON_AddNumberToObject(compObj, "textStyle", comp->styleText);
		}

		if (flags & BIT(11))
		{
			cJSON_AddNumberToObject(compObj, "textAlign", comp->alignText);
		}

		if (flags & BIT(12))
		{
			cJSON_AddNumberToObject(compObj, "autoAdjust", comp->autoAdjust);
		}

		if (flags & BIT(13))
		{
			cJSON_AddNumberToObject(compObj, "barStyle", comp->barStyle);
		}

		if (flags & BIT(14))
		{
			cJSON_AddNumberToObject(compObj, "circleDensityPoint", comp->circleDensityPoint);
		}

		if (flags & BIT(15))
		{
			cJSON_AddNumberToObject(compObj, "circleStartAngle", comp->circleStartAngle);
		}

		if (flags & BIT(16))
		{
			cJSON_AddNumberToObject(compObj, "circleEndAngle", comp->circleEndAngle);
		}

		if (flags & BIT(17))
		{
			cJSON_AddNumberToObject(compObj, "circleThickness", comp->circleThickness);
		}

		if (flags & BIT(18))
		{
			cJSON_AddNumberToObject(compObj, "feedTime", comp->feedTime);
		}

		if (flags & BIT(19))
		{
			cJSON_AddNumberToObject(compObj, "feedStayTime", comp->feedStayTime);
		}

		if (flags & BIT(20))
		{
			cJSON_AddNumberToObject(compObj, "feedFadeTime", comp->feedFadeTime);
		}

		if (flags & BIT(0))
		{
			cJSON_AddNumberToObject(compObj, "anchor", comp->anchorPoint);
			tmpObj = cJSON_AddObjectToObject(compObj, "parent");
			{
				cJSON_AddNumberToObject(tmpObj, "anchor", comp->parentAnchor.point);
				if (comp->parentAnchor.parent)
				{
					// FIXME: todo figure out what component are we referring to
					cJSON_AddStringToObject(tmpObj, "component", CG_FindComponentName(hud, comp->parentAnchor.parent));
				}
				else
				{
					cJSON_AddNullToObject(tmpObj, "component");
				}
			}
		}
	}

	return hudObj;
}

#define vec4_cmp(v1, v2) ((v1)[0] == (v2)[0] && (v1)[1] == (v2)[1] && (v1)[2] == (v2)[2] && (v1)[3] == (v2)[3])
#define rect_cmp(r1, r2) ((r1).x == (r2).x && (r1).y == (r2).y && (r1).w == (r2).w && (r1).h == (r2).h)

/**
 * @brief CG_CompareHudComponents
 * @param[in] hud
 * @param[in] comp
 * @param[in] parentHud
 * @param[in] parentComp
 * @return
 */
static uint32_t CG_CompareHudComponents(hudStucture_t *hud, hudComponent_t *comp, hudStucture_t *parentHud, hudComponent_t *parentComp)
{
	uint32_t flags = 0;
	if (!rect_cmp(comp->internalLocation, parentComp->internalLocation))
	{
		flags |= BIT(0);
	}

	if (comp->anchorPoint != parentComp->anchorPoint || comp->parentAnchor.point != parentComp->parentAnchor.point)
	{
		flags |= BIT(0);
	}

	if (Q_stricmp(CG_FindComponentName(hud, comp->parentAnchor.parent), CG_FindComponentName(parentHud, parentComp->parentAnchor.parent)) != 0)
	{
		flags |= BIT(0);
	}

	if (comp->visible != parentComp->visible)
	{
		flags |= BIT(1);
	}

	if (comp->style != parentComp->style)
	{
		flags |= BIT(2);
	}

	if (comp->scale != parentComp->scale)
	{
		flags |= BIT(3);
	}

	if (!vec4_cmp(comp->colorMain, parentComp->colorMain))
	{
		flags |= BIT(4);
	}

	if (!vec4_cmp(comp->colorSecondary, parentComp->colorSecondary))
	{
		flags |= BIT(5);
	}

	if (comp->showBackGround != parentComp->showBackGround)
	{
		flags |= BIT(6);
	}

	if (!vec4_cmp(comp->colorBackground, parentComp->colorBackground))
	{
		flags |= BIT(7);
	}

	if (comp->showBorder != parentComp->showBorder)
	{
		flags |= BIT(8);
	}

	if (!vec4_cmp(comp->colorBorder, parentComp->colorBorder))
	{
		flags |= BIT(9);
	}

	if (comp->styleText != parentComp->styleText)
	{
		flags |= BIT(10);
	}

	if (comp->alignText != parentComp->alignText)
	{
		flags |= BIT(11);
	}

	if (comp->autoAdjust != parentComp->autoAdjust)
	{
		flags |= BIT(12);
	}

	if (comp->barStyle != parentComp->barStyle)
	{
		flags |= BIT(13);
	}

	if (comp->circleDensityPoint != parentComp->circleDensityPoint)
	{
		flags |= BIT(14);
	}

	if (comp->circleStartAngle != parentComp->circleStartAngle)
	{
		flags |= BIT(15);
	}

	if (comp->circleEndAngle != parentComp->circleEndAngle)
	{
		flags |= BIT(16);
	}

	if (comp->circleThickness != parentComp->circleThickness)
	{
		flags |= BIT(17);
	}

	if (comp->feedTime != parentComp->feedTime)
	{
		flags |= BIT(18);
	}

	if (comp->feedStayTime != parentComp->feedStayTime)
	{
		flags |= BIT(19);
	}

	if (comp->feedFadeTime != parentComp->feedFadeTime)
	{
		flags |= BIT(20);
	}

	return flags;
}

/**
 * @brief CG_WriteHudsToFile
 * @return
 */
qboolean CG_WriteHudsToFile()
{
	int           i;
	hudStucture_t *hud;
	cJSON         *root, *huds, *hudObj;
	const char    *hudFilePath;
	char          backupPath[MAX_OSPATH];
	char          invalidPath[MAX_OSPATH];
	int           oldVersion;

	hudFilePath = CG_HudFilePath();

	// Existing lower-version data is moved aside before the current file is
	// written, so a failed save does not destroy the downgrade copy.
	if (CG_HudFileExists(hudFilePath))
	{
		if (CG_ReadHudFileVersion(hudFilePath, &oldVersion))
		{
			if (oldVersion < CURRENT_HUD_JSON_VERSION)
			{
				CG_HudBackupFilePathForVersion(backupPath, sizeof(backupPath), oldVersion);
				if (!CG_MoveHudFile(hudFilePath, backupPath, oldVersion))
				{
					CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to move old HUD to '%s'\n", backupPath);
					return qfalse;
				}
			}
			else if (oldVersion > CURRENT_HUD_JSON_VERSION)
			{
				CG_Printf(S_COLOR_RED "ERROR CG_HudSave: refusing to overwrite future HUD '%s'\n", hudFilePath);
				return qfalse;
			}
		}
		else
		{
			CG_HudInvalidFilePath(hudFilePath, invalidPath, sizeof(invalidPath));
			if (!CG_MoveHudFile(hudFilePath, invalidPath, 0))
			{
				CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to preserve invalid HUD '%s'\n", hudFilePath);
				return qfalse;
			}
		}
	}

	root = cJSON_CreateObject();
	if (!root)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to allocate root object\n");
		return qfalse;
	}

	cJSON_AddNumberToObject(root, "version", CURRENT_HUD_JSON_VERSION);
	huds = cJSON_AddArrayToObject(root, "huds");

	for (i = 1; i < hudData.count; i++)
	{
		hud = hudData.list[i];

		if (hud->isEditable)
		{
			hudObj = CG_CreateHudObject(hud);
			if (hudObj)
			{
				cJSON_AddItemToArray(huds, hudObj);
			}
		}
	}

	if (!Q_FSWriteJSONTo(root, hudFilePath))
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to save hud to '%s'\n", hudFilePath);
		cJSON_Delete(root);

		return qfalse;
	}

	CG_Printf("Saved huds to '%s'\n", hudFilePath);

	return qtrue;
}

/**
 * @brief CG_HudComponentSort
 * @param[in] a
 * @param[in] b
 * @return
 */
static int QDECL CG_HudComponentSort(const void *a, const void *b)
{
	return ((*(hudComponent_t **) a)->offset - (*(hudComponent_t **) b)->offset);
}

/**
 * @brief CG_HudComponentsFill
 * @param[in,out] hud
 */
void CG_HudComponentsFill(hudStucture_t *hud)
{
	int i, componentOffset;

	// Clear stale pointers so short fills remain safe if the component count drifts.
	Com_Memset(hud->components, 0, sizeof(hud->components));

	// setup component pointers to the components list
	for (i = 0, componentOffset = 0; hudComponentFields[i].name; i++)
	{
		if (hudComponentFields[i].isAlias)
		{
			continue;
		}

		// Stop before writing past the component pointer table if the definitions get out of sync.
		if (componentOffset >= ARRAY_LEN(hud->components))
		{
			Com_Printf(S_COLOR_RED "CG_HudComponentsFill: HUD component count overflow (%d)\n", componentOffset);
			break;
		}

		hud->components[componentOffset++] = (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);
	}
	// sort the components by their offset
	qsort(hud->components, componentOffset, sizeof(hud->components[0]), CG_HudComponentSort);

	if (componentOffset != ARRAY_LEN(hud->components))
	{
		Com_Printf(S_COLOR_YELLOW "CG_HudComponentsFill: expected %d HUD components, found %d\n", (int)ARRAY_LEN(hud->components), componentOffset);
	}
}

//  HUD SCRIPT FUNCTIONS BELLOW

/**
 * @brief CG_HUD_ParseError
 * @param[in] handle
 * @param[in] format
 * @return
 */
static qboolean CG_HUD_ParseError(int handle, const char *format, ...)
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

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

/**
 * @brief CG_RectParse
 * @param[in] handle
 * @param[in,out] r
 * @return
 */
static qboolean CG_RectParse(int handle, rectDef_t *r)
{
	pc_token_t peakedToken;

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == '(')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	if (PC_Float_Parse(handle, &r->x))
	{
		if (PC_Float_Parse(handle, &r->y))
		{
			if (PC_Float_Parse(handle, &r->w))
			{
				if (PC_Float_Parse(handle, &r->h))
				{
					return qtrue;
				}
			}
		}
	}

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == ')')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	return qfalse;
}

/**
 * @brief CG_Vec4Parse
 * @param[in] handle
 * @param[out] v
 * @return
 */
static qboolean CG_Vec4Parse(int handle, vec4_t v)
{
	float      r, g, b, a = 0;
	pc_token_t peakedToken;

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == '(')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	if (PC_Float_Parse(handle, &r))
	{
		if (PC_Float_Parse(handle, &g))
		{
			if (PC_Float_Parse(handle, &b))
			{
				if (PC_Float_Parse(handle, &a))
				{
					v[0] = r;
					v[1] = g;
					v[2] = b;
					v[3] = a;
					return qtrue;
				}
			}
		}
	}

	if (!PC_PeakToken(handle, &peakedToken))
	{
		return qfalse;
	}

	if (peakedToken.string[0] == ')')
	{
		if (!trap_PC_ReadToken(handle, &peakedToken))
		{
			return qfalse;
		}
	}

	return qfalse;
}

/**
 * @brief CG_ParseHudComponent
 * @param[in] handle
 * @param[in] comp
 * @return
 */
static qboolean CG_ParseHudComponent(int handle, hudComponent_t *comp)
{
	// old hud files do not have anchor data, so reset them.
	comp->parentAnchor.point  = TOP_LEFT;
	comp->parentAnchor.parent = NULL;
	comp->anchorPoint         = TOP_LEFT;

	//PC_Rect_Parse
	if (!CG_RectParse(handle, &comp->internalLocation))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->style))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->visible))
	{
		return qfalse;
	}

	if (!PC_Float_Parse(handle, &comp->scale))
	{
		return qfalse;
	}

	if (!CG_Vec4Parse(handle, comp->colorMain))
	{
		return qfalse;
	}

	if (!CG_Vec4Parse(handle, comp->colorSecondary))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->showBackGround))
	{
		return qfalse;
	}

	if (!CG_Vec4Parse(handle, comp->colorBackground))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->showBorder))
	{
		return qfalse;
	}

	if (!CG_Vec4Parse(handle, comp->colorBorder))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->styleText))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->alignText))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &comp->autoAdjust))
	{
		return qfalse;
	}

	comp->parsed = qtrue;

	return qtrue;
}

/**
 * @brief CG_ParseHUD
 * @param[in] handle
 * @return
 */
static qboolean CG_ParseHUD(int handle)
{
	int           i;
	pc_token_t    token;
	hudStucture_t *tempHud, *hud, *parentHud = NULL;
	qboolean      loadDefaults = qtrue;

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
	}

	if (!trap_PC_ReadToken(handle, &token))
	{
		return CG_HUD_ParseError(handle, "Error while parsing hud");
	}

	// reset all the components, and set the offset value to 999 for sorting
	tempHud = CG_GetFreeHud();

	// if the first parameter in the hud definition is a "no-defaults" line then no default values are set
	// and the hud is plain (everything is hidden and no positions are set)
	if (!Q_stricmp(token.string, "no-defaults"))
	{
		loadDefaults = qfalse;
	}
	// its either no-defaults, parent or neither
	else if (!Q_stricmp(token.string, "parent"))
	{
		loadDefaults = qfalse;

		if (!PC_Int_Parse(handle, &tempHud->parentNumber))
		{
			return CG_HUD_ParseError(handle, "expected integer value for parent");
		}
	}
	else
	{
		trap_PC_UnReadToken(handle);
	}

	if (!parentHud && tempHud->parentNumber >= 0)
	{
		parentHud = CG_GetHudByNumber(tempHud->parentNumber);
	}

	if (loadDefaults)
	{
		CG_setDefaultHudValues(tempHud);
	}

	while (qtrue)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "hudnumber"))
		{
			if (!PC_Int_Parse(handle, &tempHud->hudnumber))
			{
				return CG_HUD_ParseError(handle, "expected integer value for hudnumber");
			}

			continue;
		}

		for (i = 0; hudComponentFields[i].name; i++)
		{
			if (!Q_stricmp(token.string, hudComponentFields[i].name))
			{
				hudComponent_t *component = (hudComponent_t *)((char * )tempHud + hudComponentFields[i].offset);

				if (parentHud)
				{
					CG_CloneHudComponent(parentHud, hudComponentFields[i].name, tempHud, component);
				}

				component->offset    = i;
				component->hardScale = hudComponentFields[i].scale;
				component->draw      = hudComponentFields[i].draw;
				if (!CG_ParseHudComponent(handle, component))
				{
					return CG_HUD_ParseError(handle, "expected %s", hudComponentFields[i].name);
				}
				break;
			}
		}

		if (!hudComponentFields[i].name)
		{
			return CG_HUD_ParseError(handle, "unexpected token: %s", token.string);
		}
	}

	// check that the hudnumber value was set
	if (!CG_isHudNumberAvailable(tempHud->hudnumber))
	{
		return CG_HUD_ParseError(handle, "Invalid hudnumber value: %i", tempHud->hudnumber);
	}

	hud = CG_GetHudByNumber(tempHud->hudnumber);

	CG_GenerateHudAnchors(tempHud);

	if (!hud)
	{
		CG_RegisterHud(tempHud);
		Com_Printf("...properties for hud %i have been read.\n", tempHud->hudnumber);
	}
	else
	{
		CG_FreeHud(hud);
		CG_RegisterHud(tempHud);
		Com_Printf("...properties for hud %i have been updated.\n", tempHud->hudnumber);
	}

	return qtrue;
}

/**
 * @brief CG_ReadHudFile
 * @param[in] filename
 * @return
 */
static qboolean CG_ReadHudFile(const char *filename)
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource(filename);

	if (!handle)
	{
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "hudDef"))
	{
		return CG_HUD_ParseError(handle, "expected 'hudDef'");
	}

	if (!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return CG_HUD_ParseError(handle, "expected '{'");
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if (token.string[0] == '}')
		{
			break;
		}

		if (!Q_stricmp(token.string, "hud"))
		{
			if (!CG_ParseHUD(handle))
			{
				return qfalse;
			}
		}
		else
		{
			return CG_HUD_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

/**
 * @brief CG_ParseColorFromString
 * @param[out] vec
 * @param[in] s
 * @return
 */
static ID_INLINE qboolean CG_ParseColorFromString(vec_t *vec, char *s)
{
	qboolean res;

	vec[3] = 1.f;
	res    = Q_ParseColor(s, vec);

	if (!res)
	{
		vec[0] = 0.f;
		vec[1] = 0.f;
		vec[2] = 0.f;
		vec[3] = 0.f;
	}

	return res;
}

/**
 * @brief CG_HudParseColorElement
 * @param[in] object
 * @param[in] defaultValue
 * @return
 */
static ID_INLINE float CG_HudParseColorElement(cJSON *object, float defaultValue)
{
	if (!object)
	{
		return defaultValue;
	}

	if (!cJSON_IsNumber(object))
	{
		return defaultValue;
	}

	// check if the number is an integer
	if (ceil(object->valuedouble) == floor(object->valuedouble))
	{
		return ((float) MIN(MAX(object->valuedouble, 0.0), 255.0) / 255.f);
	}
	else if (object->valuedouble <= 1.f && object->valuedouble >= 0.f)
	{
		return (float) object->valuedouble;
	}
	else
	{
		return defaultValue;
	}
}

/**
 * @brief CG_HudParseColorObject
 * @param[in] object
 * @param[in] colorVec
 */
static void CG_HudParseColorObject(cJSON *object, vec_t *colorVec)
{
	if (!object)
	{
		return;
	}

	if (cJSON_IsString(object))
	{
		CG_ParseColorFromString(colorVec, object->valuestring);
	}
	else if (cJSON_IsObject(object))
	{
		colorVec[0] = CG_HudParseColorElement(cJSON_GetObjectItem(object, "r"), 0.f);
		colorVec[1] = CG_HudParseColorElement(cJSON_GetObjectItem(object, "g"), 0.f);
		colorVec[2] = CG_HudParseColorElement(cJSON_GetObjectItem(object, "b"), 0.f);
		colorVec[3] = CG_HudParseColorElement(cJSON_GetObjectItem(object, "a"), 1.f);
	}
	else if (cJSON_IsArray(object))
	{
		int i, len = cJSON_GetArraySize(object);

		for (i = 0; i < len && i < 4; i++)
		{
			colorVec[i] = CG_HudParseColorElement(cJSON_GetArrayItem(object, i), 0.f);
		}

		if (len < 4)
		{
			for (i = len; i < 3; i++)
			{
				colorVec[i] = 0.f;
			}

			colorVec[3] = 1.f;
		}
	}
	else
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudParseColorObject: invalid color data\n");
	}
}

/**
 * @brief CG_ReadHudJsonObject
 * @param[in] hud
 * @param[in] upgr
 * @param[in] isEditable
 * @return
 */
static hudStucture_t *CG_ReadHudJsonObject(cJSON *hud, hudFileUpgrades_t *upgr, qboolean isEditable)
{
	unsigned int  i;
	char          *name;
	cJSON         *tmp;
	cJSON         *comps;
	hudStucture_t *tmpHud;
	hudStucture_t *parentHud = NULL;
	hudStucture_t *oldHud;


	// Sanity check. Only objects should be in the huds array.
	if (!cJSON_IsObject(hud))
	{
		return NULL;
	}

	tmpHud = CG_GetFreeHud();

	name = Q_ReadStringValueJson(hud, "name");
	if (name && *name)
	{
		Q_strncpyz(tmpHud->name, name, MAX_QPATH);

		if (upgr->replaceNumberByName)
		{
			int index = Q_ReadIntValueJson(hud, "number");

			if (index > 0 && index < MAXHUDS)
			{
				Q_strncpyz(upgr->numberToNameTableReminder[index], name, MAX_QPATH);
			}

			// try solve the name conflict from previous file version by using number version instead
			if (CG_GetHudByName(tmpHud->name))
			{
				Q_strncpyz(tmpHud->name, va("%i", Q_ReadIntValueJson(hud, "number")), sizeof(tmpHud->name));
			}
		}
	}
	else
	{
		if (upgr->replaceNumberByName)
		{
			Q_strncpyz(tmpHud->name, va("%i", Q_ReadIntValueJson(hud, "number")), sizeof(tmpHud->name));
		}
		else
		{
			tmpHud->name[0] = '\0';
		}
	}

	// try to retrieved existing HUD from the given HUD name
	oldHud = CG_GetHudByName(tmpHud->name);

	// don't overwrite defaults HUD if we try to read a custom HUD
	// using the same HUD name
	if (oldHud && !oldHud->isEditable)
	{
		Com_Printf("skip %s to not overwrite default HUDs\n", tmpHud->name);
		return oldHud;
	}

	tmp = cJSON_GetObjectItem(hud, "parent");
	if (!tmp || cJSON_IsNull(tmp))
	{
		// default to hud 0 as parent
		Q_strncpyz(tmpHud->parent, DEFAULTHUD, sizeof(tmpHud->parent));
		tmpHud->parentNumber = 0;
	}
	else if (cJSON_IsFalse(tmp))
	{
		// No parent for this hud
		// but fallback to default HUD if component isn't present in file
		// (may happen if the file is edited manually)
		tmpHud->parent[0]    = '\0';
		tmpHud->parentNumber = -1;
		parentHud            = CG_GetHudByNumber(0);
	}
	else
	{
		if (upgr->replaceNumberByName)
		{
			Q_strncpyz(tmpHud->parent, va("%i", tmp->valueint), sizeof(tmpHud->parent));
		}
		else
		{
			Q_strncpyz(tmpHud->parent, tmp->valuestring, sizeof(tmpHud->parent));
		}

		parentHud = CG_GetHudByName(tmpHud->parent);

		if (upgr->replaceNumberByName)
		{
			if (!parentHud)
			{
				int index = Q_atoi(tmpHud->parent);

				if (index > 0 && index < MAXHUDS)
				{
					parentHud = CG_GetHudByName(upgr->numberToNameTableReminder[index]);
				}
			}
		}

		if (parentHud)
		{
			tmpHud->parentNumber = parentHud->hudnumber;
		}
		else
		{
			CG_Printf(S_COLOR_RED "Invalid parent value \"%s\" in hud data\n", tmp->valuestring);
			// return true here since this is not a catastrophic error, the other huds might be valid
			return NULL;
		}
	}

	if (tmpHud->parentNumber >= 0)
	{
		parentHud = CG_GetHudByNumber(tmpHud->parentNumber);
	}

	comps = cJSON_GetObjectItem(hud, "components");
	if (!comps)
	{
		Com_Printf("Missing components object in hud definition: %s\n", tmpHud->name);
		return NULL;
	}

	for (i = 0; hudComponentFields[i].name; i++)
	{
		hudComponent_t *component = (hudComponent_t *) ((char *) tmpHud + hudComponentFields[i].offset);
		cJSON          *comp      = cJSON_GetObjectItem(comps, hudComponentFields[i].name);

		if (parentHud)
		{
			CG_CloneHudComponent(parentHud, hudComponentFields[i].name, tmpHud, component);

			if (upgr->calcAnchors)
			{
				// we need to calculate the parent components location in 4/3 space
				rectDef_t      tmpParentRect;
				hudComponent_t *parentComp = CG_FindComponentByName(parentHud, hudComponentFields[i].name);

				rect_clear(tmpParentRect);

				CG_CalculateComponentLocation(parentComp, 0, &tmpParentRect);

				component->parentAnchor.point  = TOP_LEFT;
				component->parentAnchor.parent = NULL;
				component->anchorPoint         = TOP_LEFT;
				rect_copy(tmpParentRect, component->internalLocation);
			}
		}

		if (!comp)
		{
			continue;
		}

		component->offset    = i;
		component->hardScale = hudComponentFields[i].scale;
		component->draw      = hudComponentFields[i].draw;
		component->parsed    = qtrue;

		tmp = cJSON_GetObjectItem(comp, "rect");
		if (tmp)
		{
			component->internalLocation.x = Q_ReadFloatValueJson(tmp, "x");
			component->internalLocation.y = Q_ReadFloatValueJson(tmp, "y");
			component->internalLocation.w = Q_ReadFloatValueJson(tmp, "w");
			component->internalLocation.h = Q_ReadFloatValueJson(tmp, "h");
		}

		component->style   = Q_ReadIntValueJsonEx(comp, "style", component->style);
		component->visible = Q_ReadBoolValueJsonEx(comp, "visible", component->visible);
		component->scale   = Q_ReadFloatValueJsonEx(comp, "scale", component->scale);

		CG_HudParseColorObject(cJSON_GetObjectItem(comp, "mainColor"), component->colorMain);

		CG_HudParseColorObject(cJSON_GetObjectItem(comp, "secondaryColor"), component->colorSecondary);

		CG_HudParseColorObject(cJSON_GetObjectItem(comp, "backgroundColor"), component->colorBackground);
		// FIXME: mby get rid of the extra booleans
		component->showBackGround = Q_ReadBoolValueJsonEx(comp, "showBackGround", component->showBackGround);

		CG_HudParseColorObject(cJSON_GetObjectItem(comp, "borderColor"), component->colorBorder);
		// FIXME: mby get rid of the extra booleans
		component->showBorder = Q_ReadBoolValueJsonEx(comp, "showBorder", component->showBorder);

		component->styleText  = Q_ReadIntValueJsonEx(comp, "textStyle", component->styleText);
		component->alignText  = Q_ReadIntValueJsonEx(comp, "textAlign", component->alignText);
		component->autoAdjust = Q_ReadIntValueJsonEx(comp, "autoAdjust", component->autoAdjust);

		component->barStyle           = Q_ReadIntValueJsonEx(comp, "barStyle", component->barStyle);
		component->circleDensityPoint = Q_ReadFloatValueJsonEx(comp, "circleDensityPoint", component->circleDensityPoint);
		component->circleStartAngle   = Q_ReadFloatValueJsonEx(comp, "circleStartAngle", component->circleStartAngle);
		component->circleEndAngle     = Q_ReadFloatValueJsonEx(comp, "circleEndAngle", component->circleEndAngle);
		component->circleThickness    = Q_ReadFloatValueJsonEx(comp, "circleThickness", component->circleThickness);

		component->feedTime     = Q_ReadFloatValueJsonEx(comp, "feedTime", component->feedTime);
		component->feedStayTime = Q_ReadFloatValueJsonEx(comp, "feedStayTime", component->feedStayTime);
		component->feedFadeTime = Q_ReadFloatValueJsonEx(comp, "feedFadeTime", component->feedFadeTime);

		component->anchorPoint = Q_ReadIntValueJsonEx(comp, "anchor", component->anchorPoint);
		tmp                    = cJSON_GetObjectItem(comp, "parent");
		if (tmp)
		{
			component->parentAnchor.point = Q_ReadIntValueJsonEx(tmp, "anchor", component->parentAnchor.point);
			if (cJSON_HasObjectItem(tmp, "component"))
			{
				// FIXME: figure out how to setup the component based on a name / identifier
				char *compName = Q_ReadStringValueJson(tmp, "component");
				if (compName && *compName)
				{
					component->parentAnchor.parent = CG_FindComponentByName(tmpHud, compName);
				}
				else
				{
					component->parentAnchor.parent = NULL;
				}
			}
		}
	}

	if (upgr->replaceWeaponIconStyle)
	{
		CLEARBIT(tmpHud->weaponicon.style, 1);
	}

	if (upgr->addNoEchoToPopupmessageFilter)
	{
		int numPopUp;

		// only 3 popupmessages were available
		for (numPopUp = 0; numPopUp < 3; ++numPopUp)
		{
			hudComponent_t *comp = (hudComponent_t *)((byte *)&tmpHud->popupmessages + numPopUp * sizeof(hudComponent_t));
			int            j;

			// don't update style if inherit from parent which already upgrade it
			if (parentHud)
			{
				hudComponent_t *compParent = (hudComponent_t *)((byte *)&parentHud->popupmessages + numPopUp * sizeof(hudComponent_t));

				if (comp->style == compParent->style)
				{
					continue;
				}
			}

			for (j = 10; j > 5; --j)
			{
				if (CHECKBIT(comp->style, j - 1))
				{
					ENABLEBIT(comp->style, j);
				}
				else
				{
					CLEARBIT(comp->style, j);
				}
			}

			CLEARBIT(comp->style, j);
		}
	}

	if (upgr->shiftHealthBarDynamicColorStyle)
	{
		// Ensure dynamic coloration style is applied due to insertion of needle style from bar
		if (tmpHud->healthbar.style & BAR_NEEDLE)
		{
			tmpHud->healthbar.style |= (BAR_NEEDLE << 1);
		}
		else
		{
			tmpHud->healthbar.style |= BAR_NEEDLE;   // by default, needle will be active
		}
	}

	if (upgr->shiftHealthBarDynamicColorStyle2)
	{
		// Ensure dynamic coloration style is applied due to insertion of needle style from bar
		if (tmpHud->crosshairbar.style & BAR_CIRCULAR << 2)
		{
			tmpHud->crosshairbar.style |= (BAR_CIRCULAR << 4);
		}

		tmpHud->crosshairbar.style &= ~(BAR_CIRCULAR << 2);    // by default, circular bar will be desactivate

		// Ensure dynamic coloration style is applied due to insertion of circular style from bar
		if (tmpHud->healthbar.style & BAR_CIRCULAR)
		{
			tmpHud->healthbar.style |= (BAR_CIRCULAR << 1);
		}

		tmpHud->healthbar.style &= ~BAR_CIRCULAR;    // by default, circular bar will be desactivate
	}

	if (upgr->moveBarStyleIntoOwnField)
	{
		int tmp = 0;

		if (!parentHud || (tmpHud->staminabar.style != parentHud->staminabar.style))
		{
			tmpHud->staminabar.barStyle = tmpHud->staminabar.style;
			tmpHud->staminabar.style    = 0;    // clear all
		}

		if (!parentHud || (tmpHud->breathbar.style != parentHud->breathbar.style))
		{
			tmpHud->breathbar.barStyle = tmpHud->breathbar.style;
			tmpHud->breathbar.style    = 0; // clear all
		}

		if (!parentHud || (tmpHud->weaponheatbar.style != parentHud->weaponheatbar.style))
		{
			tmpHud->weaponheatbar.barStyle = tmpHud->weaponheatbar.style;
			tmpHud->weaponheatbar.style    = 0; // clear all
		}

		if (!parentHud || (tmpHud->weaponchargebar.style != parentHud->weaponchargebar.style))
		{
			tmpHud->weaponchargebar.barStyle = tmpHud->weaponchargebar.style;
			tmpHud->weaponchargebar.style    = 0;   // clear all
		}

		if (!parentHud || (tmpHud->cursorhintsbar.style != parentHud->cursorhintsbar.style))
		{
			tmpHud->cursorhintsbar.barStyle = tmpHud->cursorhintsbar.style;
			tmpHud->cursorhintsbar.style    = 0;    // clear all
		}

		if (!parentHud || (tmpHud->healthbar.style != parentHud->healthbar.style))
		{
			tmpHud->healthbar.barStyle = tmpHud->healthbar.style;
			if (tmpHud->healthbar.style & (BAR_CIRCULAR << 1))
			{
				tmpHud->healthbar.style     = 1; // keep dynamic coloration style only
				tmpHud->healthbar.barStyle &= ~(BAR_CIRCULAR << 1);   // remove dynamic coloration style from bar style
			}
		}

		if (!parentHud || (tmpHud->weaponstability.style != parentHud->weaponstability.style))
		{
			tmpHud->weaponstability.barStyle = (tmpHud->weaponstability.style >> 1); // remove "Always" style from bar style
			tmpHud->weaponstability.style    = tmpHud->weaponstability.style & 1; // keep "Always" style only
		}

		if (!parentHud || (tmpHud->crosshairbar.style != parentHud->crosshairbar.style))
		{
			tmpHud->crosshairbar.barStyle  = (tmpHud->crosshairbar.style >> 3);  // remove "Class", "Rank", "Prestige" style from bar style
			tmpHud->crosshairbar.barStyle &= ~(BAR_CIRCULAR << 1);   // remove dynamic coloration style from bar style

			// retrived common style for crosshair bar
			tmp |= (tmpHud->crosshairbar.style & CROSSHAIR_BAR_CLASS);
			tmp |= (tmpHud->crosshairbar.style & CROSSHAIR_BAR_RANK);
			tmp |= (tmpHud->crosshairbar.style & CROSSHAIR_BAR_PRESTIGE);
			if (tmpHud->crosshairbar.style & (BAR_CIRCULAR << 4))
			{
				tmp |= CROSSHAIR_BAR_DYNAMIC_COLOR;
			}
			tmpHud->crosshairbar.style = tmp;
		}
	}

	if (upgr->calcAnchors)
	{
		CG_GenerateHudAnchors(tmpHud);
	}

	tmpHud->isEditable = isEditable;

	if (!oldHud)
	{
		CG_RegisterHud(tmpHud);
		Com_Printf("...properties for hud %s have been read.\n", tmpHud->name);
	}
	else
	{
		// free the old hud since we are basically overwriting it
		CG_FreeHud(oldHud);
		CG_RegisterHud(tmpHud);
		Com_Printf("...properties for hud %s have been updated.\n", tmpHud->name);

		if (hudData.active == oldHud)
		{
			hudData.active = tmpHud;
		}
	}

	return tmpHud;
}

/**
 * @brief CG_CheckJsonFileUpgrades
 * @param[in] root
 * @param[out] ret
 */
static void CG_CheckJsonFileUpgrades(cJSON *root, hudFileUpgrades_t *ret)
{
	uint32_t fileVersion = 0;

	memset(ret, 0, sizeof(hudFileUpgrades_t));

	fileVersion = Q_ReadIntValueJson(root, "version");

	// check version..
	switch (fileVersion)
	{
	case CURRENT_HUD_JSON_VERSION:
		break;
	case 1:         // 2.81 - version before anchors
		ret->calcAnchors = qtrue;
	// fall through
	case 2:         // 2.81.1 - no more number, used unique string name
		ret->replaceNumberByName = qtrue;
	// fall through
	case 3:         // 2.82.1 - needle style has been added for health bar, requiring shifting Dynamic Color style value
		ret->shiftHealthBarDynamicColorStyle = qtrue;
	// fall through
	case 4:         // 2.84 - weapon icon dynamic health style replace by only ticking style due to split with weapon heat bar
		ret->replaceWeaponIconStyle        = qtrue;
		ret->addNoEchoToPopupmessageFilter = qtrue;
	// fall through
	case 5:         // 2.84 - circular style has been added for bar, requiring shifting Dynamic Color style value
		ret->shiftHealthBarDynamicColorStyle2 = qtrue;
	// fall through
	case 6:         // 2.84 - move all bar style into his own field varible to be separated from real style option
		ret->moveBarStyleIntoOwnField = qtrue;
		break;
	default:
		CG_Printf(S_COLOR_RED "ERROR CG_ReadHudJsonFile: invalid version used: %i only %i is supported\n", fileVersion, CURRENT_HUD_JSON_VERSION);
		ret->invalid = qtrue;
		break;
	}
}

/**
 * @brief CG_ReadSingleHudJsonFile
 * @param[in] filename
 * @return
 */
hudStucture_t *CG_ReadSingleHudJsonFile(const char *filename)
{
	cJSON             *root;
	hudStucture_t     *hud = NULL;
	hudFileUpgrades_t upgrades;

	root = Q_FSReadJsonFrom(filename);
	if (!root)
	{
		return NULL;
	}

	CG_CheckJsonFileUpgrades(root, &upgrades);

	if (upgrades.invalid)
	{
		cJSON_Delete(root);
		return NULL;
	}

	if (cJSON_GetObjectItem(root, "components"))
	{
		hud = CG_ReadHudJsonObject(root, &upgrades, qtrue);
	}
	cJSON_Delete(root);
	return hud;
}

/**
 * @brief CG_ReadHudJsonFile
 * @param[in] filename
 * @param[in] isEditable
 * @return
 */
static qboolean CG_ReadHudJsonFile(const char *filename, qboolean isEditable)
{
	cJSON             *root, *huds, *hud;
	hudFileUpgrades_t upgrades;

	root = Q_FSReadJsonFrom(filename);
	if (!root)
	{
		return qfalse;
	}

	CG_CheckJsonFileUpgrades(root, &upgrades);

	if (upgrades.invalid)
	{
		cJSON_Delete(root);
		return qfalse;
	}

	huds = cJSON_GetObjectItem(root, "huds");
	if (!huds || !cJSON_IsArray(huds))
	{
		// if this is a single hud file, then the root object is the hud definition
		if (cJSON_GetObjectItem(root, "components"))
		{
			if (CG_ReadHudJsonObject(root, &upgrades, isEditable))
			{
				cJSON_Delete(root);
				return qtrue;
			}
		}

		Q_JsonError("Missing or huds element is not an array\n");
		cJSON_Delete(root);
		return qfalse;
	}

	cJSON_ArrayForEach(hud, huds)
	{
		// Sanity check. Only objects should be in the huds array.
		if (!cJSON_IsObject(hud))
		{
			Com_Printf("Invalid item in the huds array\n");
			cJSON_Delete(root);
			return qfalse;
		}

		if (!CG_ReadHudJsonObject(hud, &upgrades, isEditable))
		{
			cJSON_Delete(root);
			return qfalse;
		}
	}

	cJSON_Delete(root);

	return qtrue;
}

/**
 * @brief CG_TryReadHudFromFile
 * @param[in] filename
 * @param[in] isEditable
 * @return
 */
qboolean CG_TryReadHudFromFile(const char *filename, qboolean isEditable)
{
	if (!CG_ReadHudJsonFile(filename, isEditable))
	{
		return CG_ReadHudFile(filename);
	}

	return qtrue;
}

/**
 * @brief CG_ReadHudsFromFile
 */
void CG_ReadHudsFromFile(void)
{
	const char *hudFilePath;
	char       profileName[MAX_OSPATH];
	char       legacyPath[MAX_OSPATH];
	char       hudDir[MAX_OSPATH];
	char       fileList[HUDS_FILE_LIST_BUFFER_SIZE];
	char       sourcePath[MAX_OSPATH];
	char       targetPath[MAX_OSPATH];
	char       backupPath[MAX_OSPATH];
	char       invalidPath[MAX_OSPATH];
	char       latestCompatibleHudPath[MAX_OSPATH];
	char       *fileName;
	int        fileCount;
	int        fileIndex;
	int        filenameVersion;
	int        parsedVersion;
	int        currentVersion;
	int        latestCompatibleHudVersion;
	qboolean   userHudsLoaded;
	qboolean   currentHudUsable;

	hudFilePath                = CG_HudFilePath();
	userHudsLoaded             = qfalse;
	currentHudUsable           = qfalse;
	latestCompatibleHudPath[0] = '\0';

	// Always load packaged HUD definitions first. User HUDs are editable
	// overlays and may replace or extend these definitions later.
	if (!CG_TryReadHudFromFile(HUDS_DEFAULT_PATH, qfalse))
	{
		Com_Printf("^1ERROR while reading hud file\n");
	}

	// Migrate the unversioned legacy path into the versioned HUD directory,
	// preserving the JSON version from the file itself.
	{
		profileName[0] = '\0';
		trap_Cvar_VariableStringBuffer("cl_profile", profileName, sizeof(profileName));
		Com_sprintf(legacyPath, sizeof(legacyPath), HUDS_USER_LEGACY_PATH, profileName);

		if (CG_HudFileExists(legacyPath))
		{
			if (CG_ReadHudFileVersion(legacyPath, &parsedVersion))
			{
				CG_HudFilePathForVersion(targetPath, sizeof(targetPath), parsedVersion);
				if (CG_HudFileExists(targetPath))
				{
					// A versioned HUD already exists, so keep both files by moving
					// the legacy file to a timestamped backup for its JSON version.
					CG_HudBackupFilePathForVersion(backupPath, sizeof(backupPath), parsedVersion);
					if (!CG_MoveHudFile(legacyPath, backupPath, parsedVersion))
					{
						CG_Printf(S_COLOR_RED "ERROR CG_ReadHudsFromFile: failed to preserve legacy HUD '%s'\n", legacyPath);
					}
				}
				else
				{
					if (!CG_MoveHudFile(legacyPath, targetPath, parsedVersion))
					{
						// Keep the user's HUD active for this session even if
						// the filesystem refused the canonical migration.
						userHudsLoaded = CG_ReadHudJsonFile(legacyPath, qtrue);
					}
				}
			}
			else
			{
				// Invalid legacy data is not usable for migration, but it may
				// still be user-authored data, so preserve it instead of
				// deleting it.
				CG_HudInvalidFilePath(legacyPath, invalidPath, sizeof(invalidPath));
				if (!CG_MoveHudFile(legacyPath, invalidPath, 0))
				{
					CG_Printf(S_COLOR_RED "ERROR CG_ReadHudsFromFile: failed to preserve invalid legacy HUD '%s'\n", legacyPath);
				}
			}
		}
	}

	// Normalize existing versioned filenames before choosing a HUD to load.
	// e.g. if `hud_v9.dat` contains JSON version 7, try to move it to
	// `hud_v7.dat`
	{
		CG_HudVersionedDirPath(hudDir, sizeof(hudDir));
		fileCount = trap_FS_GetFileList(hudDir, ".dat", fileList, sizeof(fileList));
		fileName  = fileList;
		for (fileIndex = 0; fileIndex < fileCount; fileIndex++)
		{
			if (CG_ParseHudVersionFilename(fileName, &filenameVersion) && filenameVersion <= CURRENT_HUD_JSON_VERSION)
			{
				Com_sprintf(sourcePath, sizeof(sourcePath), "%s/%s", hudDir, fileName);
				if (CG_ReadHudFileVersion(sourcePath, &parsedVersion))
				{
					// Once the JSON version is known, future-version HUDs are left
					// untouched so another ET:L version can keep using them.
					if (parsedVersion <= CURRENT_HUD_JSON_VERSION && parsedVersion != filenameVersion)
					{
						CG_HudFilePathForVersion(targetPath, sizeof(targetPath), parsedVersion);
						if (CG_HudFileExists(targetPath))
						{
							// The corrected path is already occupied, so preserve
							// the mismatched file as a backup of its JSON version.
							CG_HudBackupFilePathForVersion(backupPath, sizeof(backupPath), parsedVersion);
							if (!CG_MoveHudFile(sourcePath, backupPath, parsedVersion))
							{
								continue;
							}
						}
						else
						{
							if (!CG_MoveHudFile(sourcePath, targetPath, parsedVersion))
							{
								continue;
							}
						}
					}
				}
				else
				{
					// Files matching hud_v*.dat are part of the migration set. If
					// they cannot expose a valid version field, move them aside.
					CG_HudInvalidFilePath(sourcePath, invalidPath, sizeof(invalidPath));
					if (!CG_MoveHudFile(sourcePath, invalidPath, 0))
					{
						continue;
					}
				}
			}
			fileName += strlen(fileName) + 1;
		}
	}

	// Check if current hud is usable (i.e. if the parsed version field matches
	// the current version).
	//
	// Older JSON in this filename must be upgraded/moved from a compatible
	// source path instead of being loaded directly.
	if (CG_HudFileExists(hudFilePath))
	{
		if (CG_ReadHudFileVersion(hudFilePath, &currentVersion))
		{
			currentHudUsable = currentVersion == CURRENT_HUD_JSON_VERSION ? qtrue : qfalse;
		}
		else
		{
			// The current path exists but cannot be parsed. Preserve it before
			// looking for another HUD that can regenerate the current file.
			CG_HudInvalidFilePath(hudFilePath, invalidPath, sizeof(invalidPath));
			if (!CG_MoveHudFile(hudFilePath, invalidPath, 0))
			{
				CG_Printf(S_COLOR_RED "ERROR CG_ReadHudsFromFile: failed to preserve invalid HUD '%s'\n", hudFilePath);
			}
		}
	}

	// Otherwise, scan all compatible versioned HUDs and pick the highest JSON
	// version that this build can upgrade.
	if (!currentHudUsable)
	{
		latestCompatibleHudVersion = 0;
		fileCount                  = trap_FS_GetFileList(hudDir, ".dat", fileList, sizeof(fileList));
		fileName                   = fileList;
		for (fileIndex = 0; fileIndex < fileCount; fileIndex++)
		{
			if (CG_ParseHudVersionFilename(fileName, &filenameVersion) && filenameVersion <= CURRENT_HUD_JSON_VERSION)
			{
				Com_sprintf(sourcePath, sizeof(sourcePath), "%s/%s", hudDir, fileName);
				if (CG_ReadHudFileVersion(sourcePath, &parsedVersion))
				{
					// The JSON version is authoritative because filenames can
					// be stale or mismatched after manual user edits.
					if (filenameVersion == parsedVersion && parsedVersion <= CURRENT_HUD_JSON_VERSION && parsedVersion > latestCompatibleHudVersion)
					{
						latestCompatibleHudVersion = parsedVersion;
						Q_strncpyz(latestCompatibleHudPath, sourcePath, sizeof(latestCompatibleHudPath));
					}
				}
				else
				{
					// Invalid candidates cannot participate in upgrade
					// selection. Move them out of the way.
					CG_HudInvalidFilePath(sourcePath, invalidPath, sizeof(invalidPath));
					if (!CG_MoveHudFile(sourcePath, invalidPath, 0))
					{
						continue;
					}
				}
			}
			fileName += strlen(fileName) + 1;
		}

		if (latestCompatibleHudPath[0] && CG_ReadHudJsonFile(latestCompatibleHudPath, qtrue))
		{
			userHudsLoaded = qtrue;
			// Reading an older HUD applies the normal in-memory upgrade path;
			// immediately write that upgraded data to the current path.
			if (!CG_HudFileExists(hudFilePath))
			{
				CG_WriteHudsToFile();
			}
		}
		else if (latestCompatibleHudPath[0])
		{
			// A file with a version field can still fail full HUD parsing.
			// Preserve it so a bad candidate does not get retried forever.
			CG_HudInvalidFilePath(latestCompatibleHudPath, invalidPath, sizeof(invalidPath));
			if (!CG_MoveHudFile(latestCompatibleHudPath, invalidPath, 0))
			{
				CG_Printf(S_COLOR_RED "ERROR CG_ReadHudsFromFile: failed to preserve invalid HUD '%s'\n", latestCompatibleHudPath);
			}
		}
	}

	// This needs to be a .dat file to go around the file extension restrictions of the engine.
	if (!userHudsLoaded && currentHudUsable && !CG_ReadHudJsonFile(hudFilePath, qtrue))
	{
		// If the canonical file passed version probing but failed a full read,
		// preserve it as invalid and continue with the packaged HUDs.
		CG_HudInvalidFilePath(hudFilePath, invalidPath, sizeof(invalidPath));
		if (!CG_MoveHudFile(hudFilePath, invalidPath, 0))
		{
			CG_Printf(S_COLOR_RED "ERROR CG_ReadHudsFromFile: failed to preserve invalid HUD '%s'\n", hudFilePath);
		}
	}

	Com_Printf("...hud count: %i\n", hudData.count);
}
