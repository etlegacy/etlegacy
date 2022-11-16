/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2022 ET:Legacy team <mail@etlegacy.com>
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
#define HUDS_USER_PATH "hud.dat"

static qboolean CG_CompareHudComponents(hudComponent_t *c1, hudComponent_t *c2);

/**
 * @brief CG_addHudToList
 * @param[in] hud
 */
hudStucture_t *CG_addHudToList(hudStucture_t *hud)
{
	hudStucture_t *out = NULL;

	hudlist[hudCount] = *hud;
	out               = &hudlist[hudCount];
	hudCount++;

	CG_HudComponentsFill(out);

	return out;
}

/**
 * @brief Ccg_WideXAdjustFromWide
 * @param x
 * @param w
 * @return
 */
float CG_AdjustXFromHudFile(float x, float w)
{
	if (Ccg_Is43Screen())
	{
		return x;
	}
	else if ((int)(x + w * .5f) == 320)
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
 * @brief Ccg_WideXAdjustFromWide
 * @param x
 * @param w
 * @return
 */
static ID_INLINE float CG_AdjustXToHudFile(float x, float w)
{
	if (Ccg_Is43Screen())
	{
		return x;
	}
	else if ((int)(x + w * .5f) >= (int)(320 * cgs.adr43) - 1 &&
	         (int)(x + w * .5f) <= (int)(320 * cgs.adr43) + 1)
	{
		return (x - (Ccg_WideX(w) - w) * .5f) / cgs.adr43;
	}
	else if (x <= (int)(320 * cgs.adr43))
	{
		return x / cgs.adr43;
	}
	else
	{
		return (x + w - Ccg_WideX(w)) / cgs.adr43;
	}
}

static hudComponent_t *CG_FindComponentByName(hudStucture_t *hud, const char *name)
{
	int i;

	for (i = 0; hudComponentFields[i].name; i++)
	{
		if (Q_stricmp(name, hudComponentFields[i].name))
		{
			continue;
		}

		return (hudComponent_t *)((char * )hud + hudComponentFields[i].offset);
	}

	return NULL;
}

static ID_INLINE void CG_CloneHudComponent(hudStucture_t *hud, const char *name, hudComponent_t *out)
{
	hudComponent_t *comp = CG_FindComponentByName(hud, name);

	if (comp)
	{
		Com_Memcpy(out, comp, sizeof(*out));
	}
}

static ID_INLINE char *CG_ColorToHex(vec_t *vec)
{
	return va("%02x%02x%02x%02x", ((int) (vec[0] * 255.f)) & 0xff, ((int) (vec[1] * 255.f)) & 0xff, ((int) (vec[2] * 255.f)) & 0xff, ((int) (vec[3] * 255.f)) & 0xff);
}

static cJSON *CG_CreateHudObject(hudStucture_t *hud)
{
	int            j;
	hudStucture_t  *parent = NULL;
	hudComponent_t *comp, *parentComp;
	cJSON          *compObj, *rectObj, *compsObj, *hudObj = cJSON_CreateObject();

	cJSON_AddStringToObject(hudObj, "name", hud->name);
	cJSON_AddNumberToObject(hudObj, "number", hud->hudnumber);

	// no value or null defaults to hud 0
	if (hud->parent > 0)
	{
		cJSON_AddNumberToObject(hudObj, "parent", hud->parent);
	}
	// parent -1 means that this hud has no parent so write a implicit false flag
	else if (hud->parent < 0)
	{
		cJSON_AddFalseToObject(hudObj, "parent");
	}

	compsObj = cJSON_AddObjectToObject(hudObj, "components");

	if (hud->parent >= 0)
	{
		parent = CG_getHudByNumber(hud->parent);
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
			if (CG_CompareHudComponents(comp, parentComp))
			{
				continue;
			}
		}

		compObj = cJSON_AddObjectToObject(compsObj, hudComponentFields[j].name);

		rectObj = cJSON_AddObjectToObject(compObj, "rect");
		{
			cJSON_AddNumberToObject(rectObj, "x", CG_AdjustXToHudFile(comp->location.x, comp->location.w));
			cJSON_AddNumberToObject(rectObj, "y", comp->location.y);
			cJSON_AddNumberToObject(rectObj, "w", comp->location.w);
			cJSON_AddNumberToObject(rectObj, "h", comp->location.h);
		}

		cJSON_AddNumberToObject(compObj, "style", comp->style);
		cJSON_AddBoolToObject(compObj, "visible", comp->visible);
		cJSON_AddNumberToObject(compObj, "scale", comp->scale);

		cJSON_AddStringToObject(compObj, "mainColor", CG_ColorToHex(comp->colorMain));
		cJSON_AddStringToObject(compObj, "secondaryColor", CG_ColorToHex(comp->colorSecondary));

		cJSON_AddStringToObject(compObj, "backgroundColor", CG_ColorToHex(comp->colorBackground));
		cJSON_AddBoolToObject(compObj, "showBackGround", comp->showBackGround);

		cJSON_AddStringToObject(compObj, "borderColor", CG_ColorToHex(comp->colorBorder));
		cJSON_AddBoolToObject(compObj, "showBorder", comp->showBorder);

		cJSON_AddNumberToObject(compObj, "textStyle", comp->styleText);
		cJSON_AddNumberToObject(compObj, "textAlign", comp->alignText);
		cJSON_AddNumberToObject(compObj, "autoAdjust", comp->autoAdjust);
		cJSON_AddNumberToObject(compObj, "offset", comp->offset);
	}

	return hudObj;
}

#define vec4_cmp(v1, v2) ((v1)[0] == (v2)[0] && (v1)[1] == (v2)[1] && (v1)[2] == (v2)[2] && (v1)[3] == (v2)[3])
#define rect_cmp(r1, r2) ((r1).x == (r2).x && (r1).y == (r2).y && (r1).w == (r2).w && (r1).h == (r2).h)

static qboolean CG_CompareHudComponents(hudComponent_t *c1, hudComponent_t *c2)
{
	if (!rect_cmp(c1->location, c2->location))
	{
		return qfalse;
	}

	if (c1->visible != c2->visible)
	{
		return qfalse;
	}

	if (c1->style != c2->style)
	{
		return qfalse;
	}

	if (c1->scale != c2->scale)
	{
		return qfalse;
	}

	if (!vec4_cmp(c1->colorMain, c2->colorMain))
	{
		return qfalse;
	}

	if (!vec4_cmp(c1->colorSecondary, c2->colorSecondary))
	{
		return qfalse;
	}

	if (c1->showBackGround != c2->showBackGround)
	{
		return qfalse;
	}

	if (!vec4_cmp(c1->colorBackground, c2->colorBackground))
	{
		return qfalse;
	}

	if (c1->showBorder != c2->showBorder)
	{
		return qfalse;
	}

	if (!vec4_cmp(c1->colorBorder, c2->colorBorder))
	{
		return qfalse;
	}

	if (c1->styleText != c2->styleText)
	{
		return qfalse;
	}

	if (c1->alignText != c2->alignText)
	{
		return qfalse;
	}

	if (c1->autoAdjust != c2->autoAdjust)
	{
		return qfalse;
	}

	return qtrue;
}

qboolean CG_WriteHudsToFile()
{
	int           i;
	hudStucture_t *hud;
	cJSON         *root, *huds, *hudObj;

	root = cJSON_CreateObject();
	if (!root)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to allocate root object\n");
		return qfalse;
	}

	cJSON_AddNumberToObject(root, "version", CURRENT_HUD_JSON_VERSION);
	huds = cJSON_AddArrayToObject(root, "huds");

	for (i = 1; i < hudCount; i++)
	{
		hud = &hudlist[i];

		hudObj = CG_CreateHudObject(hud);
		if (hudObj)
		{
			cJSON_AddItemToArray(huds, hudObj);
		}
	}

	if (!Q_FSWriteJSONTo(root, HUDS_USER_PATH))
	{
		CG_Printf(S_COLOR_RED "ERROR CG_HudSave: failed to save hud to '" HUDS_USER_PATH "'\n");
		cJSON_Delete(root);

		return qfalse;
	}

	CG_Printf("Saved huds to '" HUDS_USER_PATH "'\n");

	return qtrue;
}

static int QDECL CG_HudComponentSort(const void *a, const void *b)
{
	return ((*(hudComponent_t **) a)->offset - (*(hudComponent_t **) b)->offset);
}

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

static int CG_getAvailableNumber()
{
	int           i, number = 1;
	hudStucture_t *hud;

	for (i = 0; i < hudCount; i++)
	{
		hud = &hudlist[i];

		if (hud->hudnumber && hud->hudnumber >= number)
		{
			number = hud->hudnumber + 1;
		}
	}

	return number;
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
					r->x = CG_AdjustXFromHudFile(r->x, r->w);
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
	//PC_Rect_Parse
	if (!CG_RectParse(handle, &comp->location))
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
	hudStucture_t tempHud;
	hudStucture_t *hud, *parentHud = NULL;
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
	Com_Memset(&tempHud, 0, sizeof(hudStucture_t));

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

		if (!PC_Int_Parse(handle, &tempHud.parent))
		{
			return CG_HUD_ParseError(handle, "expected integer value for parent");
		}
	}
	else
	{
		trap_PC_UnReadToken(handle);
	}

	if (!parentHud && tempHud.parent >= 0)
	{
		parentHud = CG_getHudByNumber(tempHud.parent);
	}

	if (loadDefaults)
	{
		CG_setDefaultHudValues(&tempHud);
	}
	else
	{
		for (i = 0; hudComponentFields[i].name; i++)
		{
			hudComponent_t *component = (hudComponent_t *)((char * )&tempHud + hudComponentFields[i].offset);
			component->offset = 999;
		}
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
			if (!PC_Int_Parse(handle, &tempHud.hudnumber))
			{
				return CG_HUD_ParseError(handle, "expected integer value for hudnumber");
			}

			continue;
		}

		for (i = 0; hudComponentFields[i].name; i++)
		{
			if (!Q_stricmp(token.string, hudComponentFields[i].name))
			{
				hudComponent_t *component = (hudComponent_t *)((char * )&tempHud + hudComponentFields[i].offset);

				if (parentHud)
				{
					CG_CloneHudComponent(parentHud, hudComponentFields[i].name, component);
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
	if (!CG_isHudNumberAvailable(tempHud.hudnumber))
	{
		return CG_HUD_ParseError(handle, "Invalid hudnumber value: %i", tempHud.hudnumber);
	}

	hud = CG_getHudByNumber(tempHud.hudnumber);

	if (!hud)
	{
		CG_addHudToList(&tempHud);
		Com_Printf("...properties for hud %i have been read.\n", tempHud.hudnumber);
	}
	else
	{
		Com_Memcpy(hud, &tempHud, sizeof(tempHud));
		CG_HudComponentsFill(hud);
		Com_Printf("...properties for hud %i have been updated.\n", tempHud.hudnumber);
	}

	return qtrue;
}

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

static ID_INLINE qboolean CG_ParseHexColor(vec_t *vec, const char *s)
{
	if (Q_IsHexColorString(s))
	{
		vec[0] = ((float)(gethex(*(s)) * 16 + gethex(*(s + 1)))) / 255.00f;
		vec[1] = ((float)(gethex(*(s + 2)) * 16 + gethex(*(s + 3)))) / 255.00f;
		vec[2] = ((float)(gethex(*(s + 4)) * 16 + gethex(*(s + 5)))) / 255.00f;

		if (Q_HexColorStringHasAlpha(s))
		{
			vec[3] = ((float)(gethex(*(s + 6)) * 16 + gethex(*(s + 7)))) / 255.00f;
		}
		else
		{
			vec[3] = 1.f;
		}

		return qtrue;
	}

	vec[0] = 0.f;
	vec[1] = 0.f;
	vec[2] = 0.f;
	vec[3] = 0.f;

	return qfalse;
}

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

static void CG_HudParseColorObject(cJSON *object, vec_t *colorVec)
{
	if (!object)
	{
		colorVec[0] = 0.f;
		colorVec[1] = 0.f;
		colorVec[2] = 0.f;
		colorVec[3] = 1.f;

		return;
	}

	if (cJSON_IsString(object))
	{
		CG_ParseHexColor(colorVec, object->valuestring);
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

static qboolean CG_ReadHudJsonFile(const char *filename)
{
	cJSON          *root, *huds, *hud, *comps, *comp, *tmp;
	uint32_t       fileVersion = 0;
	hudStucture_t  tmpHud;
	hudStucture_t  *outHud, *parentHud;
	hudComponent_t *component;
	char           *name;
	int            i, componentOffset;

	root = Q_FSReadJsonFrom(filename);
	if (!root)
	{
		return qfalse;
	}

	fileVersion = Q_ReadIntValueJson(root, "version");

	// check version..
	if (fileVersion != CURRENT_HUD_JSON_VERSION)
	{
		CG_Printf(S_COLOR_RED "ERROR CG_ReadHudJsonFile: invalid version used: %i only %i is supported\n", fileVersion, CURRENT_HUD_JSON_VERSION);
		cJSON_Delete(root);
		return qfalse;
	}

	huds = cJSON_GetObjectItem(root, "huds");
	if (!huds || !cJSON_IsArray(huds))
	{
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

		memset(&tmpHud, 0, sizeof(hudStucture_t));
		for (i = 0; hudComponentFields[i].name; i++)
		{
			component         = (hudComponent_t *)((char * )&tmpHud + hudComponentFields[i].offset);
			component->offset = 999;
		}

		componentOffset = 0;

		tmpHud.hudnumber = Q_ReadIntValueJson(hud, "number");
		if (!tmpHud.hudnumber)
		{
			tmpHud.hudnumber = CG_getAvailableNumber();
		}

		tmp = cJSON_GetObjectItem(hud, "parent");
		if (!tmp || cJSON_IsNull(tmp))
		{
			// default to hud 0 as parent
			tmpHud.parent = 0;
		}
		else if (cJSON_IsFalse(tmp))
		{
			// No parent for this hud. Will not load defaults.
			tmpHud.parent = -1;
		}
		else if (cJSON_IsNumber(tmp))
		{
			tmpHud.parent = tmp->valueint;
		}
		else
		{
			CG_Printf(S_COLOR_RED "Invalid parent value in hud data\n");
			continue;
		}

		if (tmpHud.parent >= 0)
		{
			parentHud = CG_getHudByNumber(tmpHud.parent);
		}

		tmpHud.hudnumber = Q_ReadIntValueJson(hud, "number");
		if (!tmpHud.hudnumber)
		{
			tmpHud.hudnumber = CG_getAvailableNumber();
		}

		// check that the hud number value was set
		if (!CG_isHudNumberAvailable(tmpHud.hudnumber))
		{
			Com_Printf("Invalid hudnumber value: %i\n", tmpHud.hudnumber);
			cJSON_Delete(root);
			return qfalse;
		}

		name = Q_ReadStringValueJson(hud, "name");
		if (name)
		{
			Q_strncpyz(tmpHud.name, name, MAX_QPATH);
		}

		comps = cJSON_GetObjectItem(hud, "components");
		if (!comps)
		{
			Com_Printf("Missing components object in hud definition: %i\n", tmpHud.hudnumber);
			cJSON_Delete(root);
			return qfalse;
		}

		for (i = 0; hudComponentFields[i].name; i++)
		{
			component = (hudComponent_t *)((char * )&tmpHud + hudComponentFields[i].offset);
			comp      = cJSON_GetObjectItem(comps, hudComponentFields[i].name);

			if (!comp)
			{
				if (parentHud)
				{
					CG_CloneHudComponent(parentHud, hudComponentFields[i].name, component);
				}

				continue;
			}
			component->offset    = componentOffset++;
			component->hardScale = hudComponentFields[i].scale;
			component->draw      = hudComponentFields[i].draw;
			component->parsed    = qtrue;

			tmp = cJSON_GetObjectItem(comp, "rect");
			if (tmp)
			{
				component->location.x = Q_ReadFloatValueJson(tmp, "x");
				component->location.y = Q_ReadFloatValueJson(tmp, "y");
				component->location.w = Q_ReadFloatValueJson(tmp, "w");
				component->location.h = Q_ReadFloatValueJson(tmp, "h");

				component->location.x = CG_AdjustXFromHudFile(component->location.x, component->location.w);
			}

			component->style   = Q_ReadIntValueJson(comp, "style");
			component->visible = Q_ReadBoolValueJson(comp, "visible");
			component->scale   = Q_ReadFloatValueJson(comp, "scale");

			CG_HudParseColorObject(cJSON_GetObjectItem(comp, "mainColor"), component->colorMain);

			CG_HudParseColorObject(cJSON_GetObjectItem(comp, "secondaryColor"), component->colorSecondary);

			CG_HudParseColorObject(cJSON_GetObjectItem(comp, "backgroundColor"), component->colorBackground);
			// FIXME: mby get rid of the extra booleans
			component->showBackGround = Q_ReadBoolValueJson(comp, "showBackGround");

			CG_HudParseColorObject(cJSON_GetObjectItem(comp, "borderColor"), component->colorBorder);
			// FIXME: mby get rid of the extra booleans
			component->showBorder = Q_ReadBoolValueJson(comp, "showBorder");

			component->styleText  = Q_ReadIntValueJson(comp, "textStyle");
			component->alignText  = Q_ReadIntValueJson(comp, "textAlign");
			component->autoAdjust = Q_ReadIntValueJson(comp, "autoAdjust");
			component->offset     = Q_ReadIntValueJson(comp, "offset");
		}

		outHud = CG_getHudByNumber(tmpHud.hudnumber);

		if (!outHud)
		{
			CG_addHudToList(&tmpHud);
			Com_Printf("...properties for hud %i have been read.\n", tmpHud.hudnumber);
		}
		else
		{
			Com_Memcpy(outHud, &tmpHud, sizeof(tmpHud));
			CG_HudComponentsFill(outHud);
			Com_Printf("...properties for hud %i have been updated.\n", tmpHud.hudnumber);
		}
	}

	cJSON_Delete(root);

	return qtrue;
}

static qboolean CG_TryReadHudFromFile(const char *filename)
{
	if (!CG_ReadHudJsonFile(filename))
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
	if (!CG_TryReadHudFromFile(HUDS_DEFAULT_PATH))
	{
		Com_Printf("^1ERROR while reading hud file\n");
	}

	// This needs to be a .dat file to go around the file extension restrictions of the engine.
	CG_TryReadHudFromFile(HUDS_USER_PATH);

	Com_Printf("...hud count: %i\n", hudCount);
}
