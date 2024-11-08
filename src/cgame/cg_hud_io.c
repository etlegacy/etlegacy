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
#define HUDS_USER_PATH "profiles/%s/hud.dat"
#define HUDS_USER_BACKUP_PATH "profiles/%s/hud_backup(%s).dat"

typedef struct
{
	qboolean invalid;                                   //< file is just plain invalid
	qboolean calcAnchors;                               //< added in version 2
	qboolean replaceNumberByName;                       //< added in version 3
	char numberToNameTableReminder[MAXHUDS][MAX_QPATH]; //< added in version 3
	qboolean shiftHealthBarDynamicColorStyle;           //< added in version 4
} hudFileUpgrades_t;

static uint32_t CG_CompareHudComponents(hudStucture_t *hud, hudComponent_t *comp, hudStucture_t *parentHud, hudComponent_t *parentComp);

/**
 * @brief CG_HudFilePath
 * @return
 */
static const char *CG_HudFilePath()
{
	static char filePath[MAX_OSPATH] = { 0 };

	if (!filePath[0])
	{
		char tmp[MAX_OSPATH];

		tmp[0] = '\0';
		trap_Cvar_VariableStringBuffer("cl_profile", tmp, MAX_OSPATH);
		Com_sprintf(filePath, MAX_OSPATH, HUDS_USER_PATH, tmp);
	}

	return filePath;
}

/**
 * @brief CG_HudBackupFilePath
 * @param[in] output
 * @param[in] len
 */
static void CG_HudBackupFilePath(char *output, int len)
{
	qtime_t ct;
	char    tmp[MAX_OSPATH];

	tmp[0]    = '\0';
	output[0] = '\0';
	trap_Cvar_VariableStringBuffer("cl_profile", tmp, MAX_OSPATH);
	trap_RealTime(&ct);
	Com_sprintf(output, len, HUDS_USER_BACKUP_PATH, tmp, va("%d-%02d-%02d-%02d%02d%02d", 1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec));
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

	return flags;
}

/**
 * @brief CG_BackupHudFile
 * @param[in] filename
 * @param[in] upgrade - if true, the old file will be kept
 * @return
 */
void CG_BackupHudFile(const char *filename, const qboolean upgrade)
{
	fileHandle_t tmp, backup;
	byte         *buffer;
	int          len;
	qboolean     backupOk = qfalse;

	len = trap_FS_FOpenFile(filename, &tmp, FS_READ);
	if (len > 0)
	{
		char path[MAX_OSPATH];

		CG_HudBackupFilePath(path, MAX_OSPATH);

		buffer = Com_Allocate(len + 1);
		if (!buffer)
		{
			trap_FS_FCloseFile(tmp);
			CG_Error("CG_ReadHudsFromFile: Failed to allocate buffer\n");
			return;
		}

		trap_FS_Read(buffer, len, tmp);
		buffer[len] = 0;

		if (trap_FS_FOpenFile(path, &backup, FS_WRITE) < 0)
		{
			CG_Printf(S_COLOR_RED "ERROR CG_ReadHudsFromFile: failed to save huds backup to '%s'\n", path);
		}
		else
		{
			trap_FS_Write(buffer, len, backup);
			trap_FS_FCloseFile(backup);
			backupOk = qtrue;

			if (upgrade)
			{
				CG_Printf(S_COLOR_CYAN "Upgrading HUD version, backed up old custom hud data to '%s'\n", path);
			}
			else
			{
				CG_Printf(S_COLOR_CYAN "Backed up users custom hud data to '%s'\n", path);
			}
		}

		Com_Dealloc(buffer);
	}

	if (len >= 0)
	{
		trap_FS_FCloseFile(tmp);
	}

	if (!upgrade && backupOk)
	{
		trap_FS_Delete(filename);
		CG_Printf(S_COLOR_RED "Removed users custom hud file due to invalid format '%s'\n", filename);
	}
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

	hudFilePath = CG_HudFilePath();

	// read the old HUD file first, so we can check if we're upgrading the HUD version
	root = Q_FSReadJsonFrom(hudFilePath);

	if (root)
	{
		const int oldVersion = Q_ReadIntValueJson(root, "version");

		// backup old file in case we're upgrading to new version, so downgrading is easy for the user
		if (oldVersion < CURRENT_HUD_JSON_VERSION)
		{
			CG_BackupHudFile(hudFilePath, qtrue);
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

	// setup component pointers to the components list
	for (i = 0, componentOffset = 0; hudComponentFields[i].name; i++)
	{
		if (hudComponentFields[i].isAlias)
		{
			continue;
		}
		hud->components[componentOffset++] = (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);
	}
	// sort the components by their offset
	qsort(hud->components, sizeof(hud->components) / sizeof(hudComponent_t *), sizeof(hudComponent_t *), CG_HudComponentSort);
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
	int           i, componentOffset = 0;
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

	componentOffset = 0;
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

				component->offset    = componentOffset++;
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
	unsigned int   i     = 0;
	char           *name = NULL;
	cJSON          *tmp = NULL, *comps = NULL, *comp = NULL;
	hudComponent_t *component;
	hudStucture_t  *tmpHud, *parentHud = NULL, *oldHud = NULL;
	int            componentOffset = 0;

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
		component = (hudComponent_t *) ((char *) tmpHud + hudComponentFields[i].offset);
		comp      = cJSON_GetObjectItem(comps, hudComponentFields[i].name);

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
		component->offset    = componentOffset++;
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

	hudFilePath = CG_HudFilePath();

	if (!CG_TryReadHudFromFile(HUDS_DEFAULT_PATH, qfalse))
	{
		Com_Printf("^1ERROR while reading hud file\n");
	}

	// This needs to be a .dat file to go around the file extension restrictions of the engine.
	if (!CG_TryReadHudFromFile(hudFilePath, qtrue))
	{
		// If the reading of the custom user hud file fails, then it's possible that the hud file is of "old" format
		// back it up and then remove the file
		CG_BackupHudFile(hudFilePath, qfalse);
	}

	Com_Printf("...hud count: %i\n", hudData.count);
}
