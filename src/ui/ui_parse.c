/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 * @file ui_parse.c
 * @brief String allocation/managment
 */

#include "ui_shared.h"
#include "ui_local.h"

void Tooltip_ComputePosition(itemDef_t *item)
{
	rectDef_t *itemRect = &item->window.rectClient;
	rectDef_t *tipRect  = &item->toolTipData->window.rectClient;

	DC->textFont(item->toolTipData->font);

	// Set positioning based on item location
	tipRect->x = itemRect->x + (itemRect->w / 3);
	tipRect->y = itemRect->y + itemRect->h + 8;
	tipRect->h = DC->multiLineTextHeight(item->toolTipData->text, item->toolTipData->textscale, 0) + 9.f;
	tipRect->w = DC->multiLineTextWidth(item->toolTipData->text, item->toolTipData->textscale, 0) + 6.f;
	if ((tipRect->w + tipRect->x) > 635.0f)
	{
		tipRect->x -= (tipRect->w + tipRect->x) - 635.0f;
	}

	item->toolTipData->parent        = item->parent;
	item->toolTipData->type          = ITEM_TYPE_TEXT;
	item->toolTipData->window.style  = WINDOW_STYLE_FILLED;
	item->toolTipData->window.flags |= WINDOW_VISIBLE;
}

/*
===============
Item_ValidateTypeData
===============
*/
void Item_ValidateTypeData(itemDef_t *item)
{
	if (item->typeData)
	{
		return;
	}

	if (item->type == ITEM_TYPE_LISTBOX)
	{
		item->typeData = UI_Alloc(sizeof(listBoxDef_t));
		memset(item->typeData, 0, sizeof(listBoxDef_t));
	}
	else if (TEXTFIELD(item->type) || item->type == ITEM_TYPE_YESNO || item->type == ITEM_TYPE_BIND || item->type == ITEM_TYPE_SLIDER || item->type == ITEM_TYPE_TEXT)
	{
		item->typeData = UI_Alloc(sizeof(editFieldDef_t));
		memset(item->typeData, 0, sizeof(editFieldDef_t));
		if (item->type == ITEM_TYPE_EDITFIELD)
		{
			if (!((editFieldDef_t *)item->typeData)->maxPaintChars)
			{
				((editFieldDef_t *)item->typeData)->maxPaintChars = MAX_EDITFIELD;
			}
		}
	}
	else if (item->type == ITEM_TYPE_MULTI || item->type == ITEM_TYPE_CHECKBOX || item->type == ITEM_TYPE_TRICHECKBOX || item->type == ITEM_TYPE_COMBO)
	{
		item->typeData = UI_Alloc(sizeof(multiDef_t));
	}
	else if (item->type == ITEM_TYPE_MODEL)
	{
		item->typeData = UI_Alloc(sizeof(modelDef_t));
	}
	else if (item->type == ITEM_TYPE_MENUMODEL)
	{
		item->typeData = UI_Alloc(sizeof(modelDef_t));
	}
}

/*
========================
Item_ValidateTooltipData
========================
*/
qboolean Item_ValidateTooltipData(itemDef_t *item)
{
	if (item->toolTipData != NULL)
	{
		return qtrue;
	}

	item->toolTipData = UI_Alloc(sizeof(itemDef_t));
	if (item->toolTipData == NULL)
	{
		return qfalse;
	}

	Item_Init(item->toolTipData);
	Item_Tooltip_Initialize(item->toolTipData);

	return qtrue;
}

qboolean Float_Parse(char **p, float *f)
{
	char *token;

	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0)
	{
		*f = atof(token);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean Color_Parse(char **p, vec4_t *c)
{
	int   i;
	float f = 0.0f;

	for (i = 0; i < 4; i++)
	{
		if (!Float_Parse(p, &f))
		{
			return qfalse;
		}

		(*c)[i] = f;
	}

	return qtrue;
}

qboolean Int_Parse(char **p, int *i)
{
	char *token;

	token = COM_ParseExt(p, qfalse);

	if (token && token[0] != 0)
	{
		*i = atoi(token);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean Rect_Parse(char **p, rectDef_t *r)
{
	if (Float_Parse(p, &r->x))
	{
		if (Float_Parse(p, &r->y))
		{
			if (Float_Parse(p, &r->w))
			{
				if (Float_Parse(p, &r->h))
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

qboolean String_Parse(char **p, const char **out)
{
	char *token;

	token = COM_ParseExt(p, qfalse);
	if (token && token[0] != 0)
	{
		*(out) = String_Alloc(token);
		return qtrue;
	}

	return qfalse;
}

/**
* @brief Same as PC_String_Parse except it uses a trap call
* to client's gettext translation function.
*/
qboolean PC_String_ParseTranslate(int handle, const char **out)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	*(out) = String_Alloc(__(token.string));

	return qtrue;
}

qboolean PC_Char_Parse(int handle, char *out)
{
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	*(out) = token.string[0];
	return qtrue;
}

qboolean PC_Script_Parse(int handle, const char **out)
{
	char       script[4096];
	pc_token_t token;

	memset(script, 0, sizeof(script));
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	if (Q_stricmp(token.string, "{") != 0)
	{
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			return qfalse;
		}

		if (Q_stricmp(token.string, "}") == 0)
		{
			*out = String_Alloc(script);
			return qtrue;
		}

		if (token.string[1] != '\0')
		{
			Q_strcat(script, 4096, va("\"%s\"", token.string));
		}
		else
		{
			Q_strcat(script, 4096, token.string);
		}

		Q_strcat(script, 4096, " ");
	}

	return qfalse;
}

/*
===============
Keyword Hash
===============
*/

#define KEYWORDHASH_SIZE    512

typedef struct keywordHash_s
{
	char *keyword;
	qboolean (*func)(itemDef_t *item, int handle);
	struct keywordHash_s *next;
} keywordHash_t;

int KeywordHash_Key(char *keyword)
{
	int register hash = 0, i;

	for (i = 0; keyword[i] != '\0'; i++)
	{
		if (keyword[i] >= 'A' && keyword[i] <= 'Z')
		{
			hash += (keyword[i] + ('a' - 'A')) * (119 + i);
		}
		else
		{
			hash += keyword[i] * (119 + i);
		}
	}

	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (KEYWORDHASH_SIZE - 1);
	return hash;
}

void KeywordHash_Add(keywordHash_t *table[], keywordHash_t *key)
{
	int hash = KeywordHash_Key(key->keyword);
	/*
	if (table[hash]) {
	int collision = qtrue;
	}
	*/
	key->next   = table[hash];
	table[hash] = key;
}

keywordHash_t *KeywordHash_Find(keywordHash_t *table[], char *keyword)
{
	keywordHash_t *key;
	int           hash = KeywordHash_Key(keyword);

	for (key = table[hash]; key; key = key->next)
	{
		if (!Q_stricmp(key->keyword, keyword))
		{
			return key;
		}
	}

	return NULL;
}

/*
===============
Item Keyword Parse functions
===============
*/

// name <string>
qboolean ItemParse_name(itemDef_t *item, int handle)
{
	if (!PC_String_Parse(handle, &item->window.name))
	{
		return qfalse;
	}

	return qtrue;
}

// name <string>
qboolean ItemParse_focusSound(itemDef_t *item, int handle)
{
	const char *temp = NULL;

	if (!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}

	item->focusSound = DC->registerSound(temp, qtrue);
	return qtrue;
}

// text <string>
qboolean ItemParse_text(itemDef_t *item, int handle)
{
	if (!PC_String_ParseTranslate(handle, &item->text))
	{
		return qfalse;
	}

	return qtrue;
}

// textfile <string>
// read an external textfile into item->text
qboolean ItemParse_textfile(itemDef_t *item, int handle)
{
	const char *newtext;
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	newtext    = DC->fileText(token.string);
	item->text = String_Alloc(newtext);

	return qtrue;
}

// group <string>
qboolean ItemParse_group(itemDef_t *item, int handle)
{
	if (!PC_String_Parse(handle, &item->window.group))
	{
		return qfalse;
	}

	return qtrue;
}

// asset_model <string>
qboolean ItemParse_asset_model(itemDef_t *item, int handle)
{
	const char *temp = NULL;
	Item_ValidateTypeData(item);

	if (!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}

	if (!(item->asset))
	{
		item->asset = DC->registerModel(temp);
	}

	return qtrue;
}

// asset_shader <string>
qboolean ItemParse_asset_shader(itemDef_t *item, int handle)
{
	const char *temp = NULL;

	if (!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}

	item->asset = DC->registerShaderNoMip(temp);
	return qtrue;
}

// model_origin <number> <number> <number>
qboolean ItemParse_model_origin(itemDef_t *item, int handle)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *)item->typeData;

	if (PC_Float_Parse(handle, &modelPtr->origin[0]))
	{
		if (PC_Float_Parse(handle, &modelPtr->origin[1]))
		{
			if (PC_Float_Parse(handle, &modelPtr->origin[2]))
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

// model_fovx <number>
qboolean ItemParse_model_fovx(itemDef_t *item, int handle)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *)item->typeData;

	if (!PC_Float_Parse(handle, &modelPtr->fov_x))
	{
		return qfalse;
	}

	return qtrue;
}

// model_fovy <number>
qboolean ItemParse_model_fovy(itemDef_t *item, int handle)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *)item->typeData;

	if (!PC_Float_Parse(handle, &modelPtr->fov_y))
	{
		return qfalse;
	}

	return qtrue;
}

// model_rotation <integer>
qboolean ItemParse_model_rotation(itemDef_t *item, int handle)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *)item->typeData;

	if (!PC_Int_Parse(handle, &modelPtr->rotationSpeed))
	{
		return qfalse;
	}

	return qtrue;
}

// model_angle <integer>
qboolean ItemParse_model_angle(itemDef_t *item, int handle)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *)item->typeData;

	if (!PC_Int_Parse(handle, &modelPtr->angle))
	{
		return qfalse;
	}

	return qtrue;
}

// model_animplay <int(startframe)> <int(numframes)> <int(loopframes)> <int(fps)>
qboolean ItemParse_model_animplay(itemDef_t *item, int handle)
{
	modelDef_t *modelPtr;
	Item_ValidateTypeData(item);
	modelPtr = (modelDef_t *)item->typeData;

	modelPtr->animated = 1;

	if (!PC_Int_Parse(handle, &modelPtr->startframe))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &modelPtr->numframes))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &modelPtr->loopframes))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &modelPtr->fps))
	{
		return qfalse;
	}

	modelPtr->frame     = modelPtr->startframe + 1;
	modelPtr->oldframe  = modelPtr->startframe;
	modelPtr->backlerp  = 0.0f;
	modelPtr->frameTime = DC->realTime;
	return qtrue;
}

// rect <rectangle>
qboolean ItemParse_rect(itemDef_t *item, int handle)
{
	return(PC_Rect_Parse(handle, &item->window.rectClient));
}

// origin <integer, integer>
qboolean ItemParse_origin(itemDef_t *item, int handle)
{
	int x = 0, y = 0;

	if (!PC_Int_Parse(handle, &x))
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &y))
	{
		return qfalse;
	}

	item->window.rectClient.x += x;
	item->window.rectClient.y += y;

	return qtrue;
}

// style <integer>
qboolean ItemParse_style(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->window.style))
	{
		return qfalse;
	}

	return qtrue;
}

// decoration
qboolean ItemParse_decoration(itemDef_t *item, int handle)
{
	item->window.flags |= WINDOW_DECORATION;
	return qtrue;
}

// textasint
qboolean ItemParse_textasint(itemDef_t *item, int handle)
{
	item->window.flags |= WINDOW_TEXTASINT;
	return qtrue;
}

// textasfloat
qboolean ItemParse_textasfloat(itemDef_t *item, int handle)
{
	item->window.flags |= WINDOW_TEXTASFLOAT;
	return qtrue;
}

// notselectable
qboolean ItemParse_notselectable(itemDef_t *item, int handle)
{
	listBoxDef_t *listPtr;
	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t *)item->typeData;
	if (item->type == ITEM_TYPE_LISTBOX && listPtr)
	{
		listPtr->notselectable = qtrue;
	}

	return qtrue;
}

// manually wrapped
qboolean ItemParse_wrapped(itemDef_t *item, int handle)
{
	item->window.flags |= WINDOW_WRAPPED;
	return qtrue;
}

// auto wrapped
qboolean ItemParse_autowrapped(itemDef_t *item, int handle)
{
	item->window.flags |= WINDOW_AUTOWRAPPED;
	return qtrue;
}

// bitflag <integer>
qboolean ItemParse_bitflag(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->bitflag))
	{
		return qfalse;
	}
	return qtrue;
}

// horizontalscroll
qboolean ItemParse_horizontalscroll(itemDef_t *item, int handle)
{
	item->window.flags |= WINDOW_HORIZONTAL;
	return qtrue;
}

// type <integer>
qboolean ItemParse_type(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->type))
	{
		return qfalse;
	}

	Item_ValidateTypeData(item);
	return qtrue;
}

// elementwidth, used for listbox image elements
// uses textalignx for storage
qboolean ItemParse_elementwidth(itemDef_t *item, int handle)
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t *)item->typeData;
	if (!PC_Float_Parse(handle, &listPtr->elementWidth))
	{
		return qfalse;
	}

	return qtrue;
}

// elementheight, used for listbox image elements
// uses textaligny for storage
qboolean ItemParse_elementheight(itemDef_t *item, int handle)
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	listPtr = (listBoxDef_t *)item->typeData;
	if (!PC_Float_Parse(handle, &listPtr->elementHeight))
	{
		return qfalse;
	}

	return qtrue;
}

// feeder <float>
qboolean ItemParse_feeder(itemDef_t *item, int handle)
{
	if (!PC_Float_Parse(handle, &item->special))
	{
		return qfalse;
	}

	return qtrue;
}

// elementtype, used to specify what type of elements a listbox contains
// uses textstyle for storage
qboolean ItemParse_elementtype(itemDef_t *item, int handle)
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t *)item->typeData;
	if (!PC_Int_Parse(handle, &listPtr->elementStyle))
	{
		return qfalse;
	}

	return qtrue;
}

// columns sets a number of columns and an x pos and width per..
qboolean ItemParse_columns(itemDef_t *item, int handle)
{
	int          num = 0;
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t *)item->typeData;
	if (PC_Int_Parse(handle, &num))
	{
		int i;

		if (num > MAX_LB_COLUMNS)
		{
			num = MAX_LB_COLUMNS;
		}

		listPtr->numColumns = num;
		for (i = 0; i < num; i++)
		{
			int pos = 0, width = 0, maxChars = 0;

			if (PC_Int_Parse(handle, &pos) && PC_Int_Parse(handle, &width) && PC_Int_Parse(handle, &maxChars))
			{
				listPtr->columnInfo[i].pos      = pos;
				listPtr->columnInfo[i].width    = width;
				listPtr->columnInfo[i].maxChars = maxChars;
			}
			else
			{
				return qfalse;
			}
		}
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_border(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->window.border))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_bordersize(itemDef_t *item, int handle)
{
	if (!PC_Float_Parse(handle, &item->window.borderSize))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_visible(itemDef_t *item, int handle)
{
	int i = 0;

	if (!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}

	if (i)
	{
		item->window.flags |= WINDOW_VISIBLE;
	}

	return qtrue;
}

qboolean ItemParse_ownerdraw(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->window.ownerDraw))
	{
		return qfalse;
	}

	item->type = ITEM_TYPE_OWNERDRAW;
	return qtrue;
}

qboolean ItemParse_align(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->alignment))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_textalign(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->textalignment))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_textalignx(itemDef_t *item, int handle)
{
	if (!PC_Float_Parse(handle, &item->textalignx))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_textaligny(itemDef_t *item, int handle)
{
	if (!PC_Float_Parse(handle, &item->textaligny))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_textscale(itemDef_t *item, int handle)
{
	if (!PC_Float_Parse(handle, &item->textscale))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_textstyle(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->textStyle))
	{
		return qfalse;
	}

	return qtrue;
}

// added for forcing a font for a given item
qboolean ItemParse_textfont(itemDef_t *item, int handle)
{
	if (!PC_Int_Parse(handle, &item->font))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_backcolor(itemDef_t *item, int handle)
{
	int   i;
	float f = 0.0f;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		item->window.backColor[i] = f;
	}

	return qtrue;
}

qboolean ItemParse_forecolor(itemDef_t *item, int handle)
{
	int   i;
	float f = 0.0f;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		item->window.foreColor[i] = f;
		item->window.flags       |= WINDOW_FORECOLORSET;
	}

	return qtrue;
}

qboolean ItemParse_bordercolor(itemDef_t *item, int handle)
{
	int   i;
	float f = 0.0f;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		item->window.borderColor[i] = f;
	}

	return qtrue;
}

qboolean ItemParse_outlinecolor(itemDef_t *item, int handle)
{
	if (!PC_Color_Parse(handle, &item->window.outlineColor))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_background(itemDef_t *item, int handle)
{
	const char *temp = NULL;

	if (!PC_String_Parse(handle, &temp))
	{
		return qfalse;
	}

	item->window.background = DC->registerShaderNoMip(temp);
	return qtrue;
}

qboolean ItemParse_cinematic(itemDef_t *item, int handle)
{
	if (!PC_String_Parse(handle, &item->window.cinematicName))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_doubleClick(itemDef_t *item, int handle)
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t *)item->typeData;

	if (!PC_Script_Parse(handle, &listPtr->doubleClick))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_onTab(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->onTab))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_onEsc(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->onEsc))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_onEnter(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->onEnter))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_onPaste(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->onPaste))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_contextMenu(itemDef_t *item, int handle)
{
	listBoxDef_t *listPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	listPtr = (listBoxDef_t *)item->typeData;

	if (!PC_String_Parse(handle, &listPtr->contextMenu))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_onFocus(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->onFocus))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_leaveFocus(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->leaveFocus))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_mouseEnter(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->mouseEnter))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_mouseExit(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->mouseExit))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_mouseEnterText(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->mouseEnterText))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_mouseExitText(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->mouseExitText))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_action(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->action))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_accept(itemDef_t *item, int handle)
{
	if (!PC_Script_Parse(handle, &item->onAccept))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_special(itemDef_t *item, int handle)
{
	if (!PC_Float_Parse(handle, &item->special))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_cvarTest(itemDef_t *item, int handle)
{
	if (!PC_String_Parse(handle, &item->cvarTest))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean ItemParse_cvar(itemDef_t *item, int handle)
{
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!PC_String_Parse(handle, &item->cvar))
	{
		return qfalse;
	}

	Q_strlwr((char *)item->cvar);
	if (item->typeData)
	{
		editPtr         = (editFieldDef_t *)item->typeData;
		editPtr->minVal = -1;
		editPtr->maxVal = -1;
		editPtr->defVal = -1;
	}

	return qtrue;
}

qboolean ItemParse_maxChars(itemDef_t *item, int handle)
{
	editFieldDef_t *editPtr;
	int            maxChars = 0;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &maxChars))
	{
		return qfalse;
	}

	editPtr           = (editFieldDef_t *)item->typeData;
	editPtr->maxChars = maxChars;
	return qtrue;
}

qboolean ItemParse_maxPaintChars(itemDef_t *item, int handle)
{
	editFieldDef_t *editPtr;
	int            maxChars = 0;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	if (!PC_Int_Parse(handle, &maxChars))
	{
		return qfalse;
	}

	editPtr                = (editFieldDef_t *)item->typeData;
	editPtr->maxPaintChars = maxChars;
	return qtrue;
}

qboolean ItemParse_cvarFloat(itemDef_t *item, int handle)
{
	editFieldDef_t *editPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	editPtr = (editFieldDef_t *)item->typeData;
	if (PC_String_Parse(handle, &item->cvar) &&
	    PC_Float_Parse(handle, &editPtr->defVal) &&
	    PC_Float_Parse(handle, &editPtr->minVal) &&
	    PC_Float_Parse(handle, &editPtr->maxVal))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean ItemParse_cvarStrList(itemDef_t *item, int handle)
{
	pc_token_t token;
	multiDef_t *multiPtr;
	int        pass;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	multiPtr         = (multiDef_t *)item->typeData;
	multiPtr->count  = 0;
	multiPtr->strDef = qtrue;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	if (*token.string != '{')
	{
		return qfalse;
	}

	pass = 0;
	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if (*token.string == '}')
		{
			return qtrue;
		}

		if (*token.string == ',' || *token.string == ';')
		{
			continue;
		}

		if (pass == 0)
		{
			multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
			pass                                = 1;
		}
		else
		{
			multiPtr->cvarStr[multiPtr->count] = String_Alloc(token.string);
			pass                               = 0;
			multiPtr->count++;
			if (multiPtr->count >= MAX_MULTI_CVARS)
			{
				return qfalse;
			}
		}
	}

	return qfalse;
}

qboolean ItemParse_cvarFloatList(itemDef_t *item, int handle)
{
	pc_token_t token;
	multiDef_t *multiPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	multiPtr         = (multiDef_t *)item->typeData;
	multiPtr->count  = 0;
	multiPtr->strDef = qfalse;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	if (*token.string != '{')
	{
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if (*token.string == '}')
		{
			return qtrue;
		}

		if (*token.string == ',' || *token.string == ';')
		{
			continue;
		}

		multiPtr->cvarList[multiPtr->count] = String_Alloc(token.string);
		if (!PC_Float_Parse(handle, &multiPtr->cvarValue[multiPtr->count]))
		{
			return qfalse;
		}

		multiPtr->count++;
		if (multiPtr->count >= MAX_MULTI_CVARS)
		{
			return qfalse;
		}
	}

	return qfalse;
}

qboolean ItemParse_cvarListUndefined(itemDef_t *item, int handle)
{
	pc_token_t token;
	multiDef_t *multiPtr;

	Item_ValidateTypeData(item);
	if (!item->typeData)
	{
		return qfalse;
	}

	multiPtr = (multiDef_t *)item->typeData;

	multiPtr->undefinedStr = NULL;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	multiPtr->undefinedStr = String_Alloc(token.string);

	return qtrue;
}

qboolean ParseColorRange(itemDef_t *item, int handle, int type)
{
	colorRangeDef_t color;

	if (item->numColors && type != item->colorRangeType)
	{
		PC_SourceError(handle, "both addColorRange and addColorRangeRel - set within same itemdef\n");
		return qfalse;
	}

	memset(&color, 0, sizeof(color));

	item->colorRangeType = type;

	if (PC_Float_Parse(handle, &color.low) &&
	    PC_Float_Parse(handle, &color.high) &&
	    PC_Color_Parse(handle, &color.color))
	{
		if (item->numColors < MAX_COLOR_RANGES)
		{
			memcpy(&item->colorRanges[item->numColors], &color, sizeof(color));
			item->numColors++;
		}

		return qtrue;
	}

	return qfalse;
}

qboolean ItemParse_addColorRangeRel(itemDef_t *item, int handle)
{
	return ParseColorRange(item, handle, RANGETYPE_RELATIVE);
}

qboolean ItemParse_addColorRange(itemDef_t *item, int handle)
{
	return ParseColorRange(item, handle, RANGETYPE_ABSOLUTE);
}

qboolean ItemParse_ownerdrawFlag(itemDef_t *item, int handle)
{
	int i = 0;
	if (!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}

	item->window.ownerDrawFlags |= i;
	return qtrue;
}

qboolean ItemParse_enableCvar(itemDef_t *item, int handle)
{
	if (PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_ENABLE;
		return qtrue;
	}

	return qfalse;
}

qboolean ItemParse_disableCvar(itemDef_t *item, int handle)
{
	if (PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_DISABLE;
		return qtrue;
	}

	return qfalse;
}

qboolean ItemParse_noToggle(itemDef_t *item, int handle)
{
	item->cvarFlags |= CVAR_NOTOGGLE;
	return qtrue;
}

qboolean ItemParse_showCvar(itemDef_t *item, int handle)
{
	if (PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_SHOW;
		return qtrue;
	}

	return qfalse;
}

qboolean ItemParse_hideCvar(itemDef_t *item, int handle)
{
	if (PC_Script_Parse(handle, &item->enableCvar))
	{
		item->cvarFlags = CVAR_HIDE;
		return qtrue;
	}

	return qfalse;
}

qboolean ItemParse_execKey(itemDef_t *item, int handle)
{
	char keyname;

	// read in the hotkey
	if (!PC_Char_Parse(handle, &keyname))
	{
		return qfalse;
	}

	// store it in the hotkey field
	item->hotkey = keyname;

	// read in the command to execute
	if (!PC_Script_Parse(handle, &item->onKey))
	{
		return qfalse;
	}

	return qtrue;
}

// server setting tags
qboolean ItemParse_settingDisabled(itemDef_t *item, int handle)
{
	qboolean fResult = PC_Int_Parse(handle, &item->settingTest);

	if (fResult)
	{
		item->settingFlags = SVS_DISABLED_SHOW;
	}

	return(fResult);
}

qboolean ItemParse_settingEnabled(itemDef_t *item, int handle)
{
	qboolean fResult = PC_Int_Parse(handle, &item->settingTest);

	if (fResult)
	{
		item->settingFlags = SVS_ENABLED_SHOW;
	}

	return(fResult);
}

qboolean ItemParse_tooltip(itemDef_t *item, int handle)
{
	return(Item_ValidateTooltipData(item) && PC_String_ParseTranslate(handle, &item->toolTipData->text));
}

qboolean ItemParse_tooltipalignx(itemDef_t *item, int handle)
{
	return(Item_ValidateTooltipData(item) && PC_Float_Parse(handle, &item->toolTipData->textalignx));
}

qboolean ItemParse_tooltipaligny(itemDef_t *item, int handle)
{
	return(Item_ValidateTooltipData(item) && PC_Float_Parse(handle, &item->toolTipData->textaligny));
}

qboolean ItemParse_voteFlag(itemDef_t *item, int handle)
{
	return(PC_Int_Parse(handle, &item->voteFlag));
}

keywordHash_t itemParseKeywords[] =
{
	{ "accept",            ItemParse_accept,            NULL },
	{ "action",            ItemParse_action,            NULL },
	{ "addColorRange",     ItemParse_addColorRange,     NULL },
	{ "addColorRangeRel",  ItemParse_addColorRangeRel,  NULL },
	{ "align",             ItemParse_align,             NULL },
	{ "asset_model",       ItemParse_asset_model,       NULL },
	{ "asset_shader",      ItemParse_asset_shader,      NULL },
	{ "autowrapped",       ItemParse_autowrapped,       NULL },
	{ "backcolor",         ItemParse_backcolor,         NULL },
	{ "background",        ItemParse_background,        NULL },
	{ "border",            ItemParse_border,            NULL },
	{ "bordercolor",       ItemParse_bordercolor,       NULL },
	{ "bordersize",        ItemParse_bordersize,        NULL },
	{ "cinematic",         ItemParse_cinematic,         NULL },
	{ "columns",           ItemParse_columns,           NULL },
	{ "contextmenu",       ItemParse_contextMenu,       NULL },
	{ "cvar",              ItemParse_cvar,              NULL },
	{ "cvarFloat",         ItemParse_cvarFloat,         NULL },
	{ "cvarFloatList",     ItemParse_cvarFloatList,     NULL },
	{ "cvarStrList",       ItemParse_cvarStrList,       NULL },
	{ "cvarListUndefined", ItemParse_cvarListUndefined, NULL },
	{ "cvarTest",          ItemParse_cvarTest,          NULL },
	{ "decoration",        ItemParse_decoration,        NULL },
	{ "textasint",         ItemParse_textasint,         NULL },
	{ "textasfloat",       ItemParse_textasfloat,       NULL },
	{ "disableCvar",       ItemParse_disableCvar,       NULL },
	{ "doubleclick",       ItemParse_doubleClick,       NULL },
	{ "onTab",             ItemParse_onTab,             NULL },
	{ "onEsc",             ItemParse_onEsc,             NULL },
	{ "onEnter",           ItemParse_onEnter,           NULL },
	{ "onPaste",           ItemParse_onPaste,           NULL },
	{ "elementheight",     ItemParse_elementheight,     NULL },
	{ "elementtype",       ItemParse_elementtype,       NULL },
	{ "elementwidth",      ItemParse_elementwidth,      NULL },
	{ "enableCvar",        ItemParse_enableCvar,        NULL },
	{ "execKey",           ItemParse_execKey,           NULL },
	{ "feeder",            ItemParse_feeder,            NULL },
	{ "focusSound",        ItemParse_focusSound,        NULL },
	{ "forecolor",         ItemParse_forecolor,         NULL },
	{ "group",             ItemParse_group,             NULL },
	{ "hideCvar",          ItemParse_hideCvar,          NULL },
	{ "horizontalscroll",  ItemParse_horizontalscroll,  NULL },
	{ "leaveFocus",        ItemParse_leaveFocus,        NULL },
	{ "maxChars",          ItemParse_maxChars,          NULL },
	{ "maxPaintChars",     ItemParse_maxPaintChars,     NULL },
	{ "model_angle",       ItemParse_model_angle,       NULL },
	{ "model_animplay",    ItemParse_model_animplay,    NULL },
	{ "model_fovx",        ItemParse_model_fovx,        NULL },
	{ "model_fovy",        ItemParse_model_fovy,        NULL },
	{ "model_origin",      ItemParse_model_origin,      NULL },
	{ "model_rotation",    ItemParse_model_rotation,    NULL },
	{ "mouseEnter",        ItemParse_mouseEnter,        NULL },
	{ "mouseEnterText",    ItemParse_mouseEnterText,    NULL },
	{ "mouseExit",         ItemParse_mouseExit,         NULL },
	{ "mouseExitText",     ItemParse_mouseExitText,     NULL },
	{ "name",              ItemParse_name,              NULL },
	{ "noToggle",          ItemParse_noToggle,          NULL }, // use with ITEM_TYPE_YESNO and an action script (see sv_punkbuster)
	{ "notselectable",     ItemParse_notselectable,     NULL },
	{ "onFocus",           ItemParse_onFocus,           NULL },
	{ "origin",            ItemParse_origin,            NULL },
	{ "outlinecolor",      ItemParse_outlinecolor,      NULL },
	{ "ownerdraw",         ItemParse_ownerdraw,         NULL },
	{ "ownerdrawFlag",     ItemParse_ownerdrawFlag,     NULL },
	{ "rect",              ItemParse_rect,              NULL },
	{ "settingDisabled",   ItemParse_settingDisabled,   NULL },
	{ "settingEnabled",    ItemParse_settingEnabled,    NULL },
	{ "showCvar",          ItemParse_showCvar,          NULL },
	{ "special",           ItemParse_special,           NULL },
	{ "style",             ItemParse_style,             NULL },
	{ "text",              ItemParse_text,              NULL },
	{ "textalign",         ItemParse_textalign,         NULL },
	{ "textalignx",        ItemParse_textalignx,        NULL },
	{ "textaligny",        ItemParse_textaligny,        NULL },
	{ "textfile",          ItemParse_textfile,          NULL },
	{ "textfont",          ItemParse_textfont,          NULL },
	{ "textscale",         ItemParse_textscale,         NULL },
	{ "textstyle",         ItemParse_textstyle,         NULL },
	{ "tooltip",           ItemParse_tooltip,           NULL },
	{ "tooltipalignx",     ItemParse_tooltipalignx,     NULL },
	{ "tooltipaligny",     ItemParse_tooltipaligny,     NULL },
	{ "type",              ItemParse_type,              NULL },
	{ "visible",           ItemParse_visible,           NULL },
	{ "voteFlag",          ItemParse_voteFlag,          NULL }, // vote check
	{ "wrapped",           ItemParse_wrapped,           NULL },
	{ "bitflag",           ItemParse_bitflag,           NULL },
	{ NULL,                NULL,                        NULL }
};

keywordHash_t *itemParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Item_SetupKeywordHash
===============
*/
void Item_SetupKeywordHash(void)
{
	int i;

	memset(itemParseKeywordHash, 0, sizeof(keywordHash_t *) * KEYWORDHASH_SIZE);
	for (i = 0; itemParseKeywords[i].keyword; i++)
	{
		KeywordHash_Add(itemParseKeywordHash, &itemParseKeywords[i]);
	}
}

/*
===============
Item_Parse
===============
*/
qboolean Item_Parse(int handle, itemDef_t *item)
{
	pc_token_t    token;
	keywordHash_t *key;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	if (*token.string != '{')
	{
		return qfalse;
	}

	while (1)
	{
		if (!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu item\n");
			return qfalse;
		}

		if (*token.string == '}')
		{
			return qtrue;
		}

		key = KeywordHash_Find(itemParseKeywordHash, token.string);
		if (!key)
		{
			PC_SourceError(handle, "unknown menu item keyword %s", token.string);
			continue;
		}

		if (!key->func(item, handle))
		{
			PC_SourceError(handle, "couldn't parse menu item keyword %s", token.string);
			return qfalse;
		}
	}

	return qfalse;
}

// Item_InitControls
// init's special control types
void Item_InitControls(itemDef_t *item)
{
	if (item == NULL)
	{
		return;
	}

	if (item->type == ITEM_TYPE_LISTBOX)
	{
		listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
		item->cursorPos = 0;
		if (listPtr)
		{
			listPtr->cursorPos = 0;
			listPtr->startPos  = 0;
			listPtr->endPos    = 0;
		}
	}

	if (item->toolTipData != NULL)
	{
		Tooltip_ComputePosition(item);
	}
}

/*
===============
Menu Keyword Parse functions
===============
*/
qboolean MenuParse_name(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_String_Parse(handle, &menu->window.name))
	{
		return qfalse;
	}

	if (Q_stricmp(menu->window.name, "main") == 0)
	{
		// default main as having focus
		//menu->window.flags |= WINDOW_HASFOCUS;
	}

	return qtrue;
}

qboolean MenuParse_fullscreen(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	union
	{
		qboolean b;
		int i;
	} fullScreen;

	if (!PC_Int_Parse(handle, &fullScreen.i))
	{
		return qfalse;
	}

	menu->fullScreen = fullScreen.b;
	return qtrue;
}

qboolean MenuParse_rect(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	return(PC_Rect_Parse(handle, &menu->window.rect));
}

qboolean MenuParse_style(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &menu->window.style))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_visible(itemDef_t *item, int handle)
{
	int       i     = 0;
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}

	if (i)
	{
		menu->window.flags |= WINDOW_VISIBLE;
	}

	return qtrue;
}

qboolean MenuParse_onOpen(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;
	if (!PC_Script_Parse(handle, &menu->onOpen))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_onClose(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Script_Parse(handle, &menu->onClose))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_onESC(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Script_Parse(handle, &menu->onESC))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_onEnter(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Script_Parse(handle, &menu->onEnter))
	{
		return qfalse;
	}

	return qtrue;
}
qboolean MenuParse_onPaste(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Script_Parse(handle, &menu->onPaste))
	{
		return qfalse;
	}

	return qtrue;
}

// menu timeout function
qboolean MenuParse_onTimeout(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &menu->timeout))
	{
		return qfalse;
	}

	if (!PC_Script_Parse(handle, &menu->onTimeout))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_border(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &menu->window.border))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_borderSize(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Float_Parse(handle, &menu->window.borderSize))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_backcolor(itemDef_t *item, int handle)
{
	int       i;
	float     f     = 0.0f;
	menuDef_t *menu = (menuDef_t *)item;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		menu->window.backColor[i] = f;
	}

	return qtrue;
}

qboolean MenuParse_forecolor(itemDef_t *item, int handle)
{
	int       i;
	float     f     = 0.0f;
	menuDef_t *menu = (menuDef_t *)item;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		menu->window.foreColor[i] = f;
		menu->window.flags       |= WINDOW_FORECOLORSET;
	}

	return qtrue;
}

qboolean MenuParse_bordercolor(itemDef_t *item, int handle)
{
	int       i;
	float     f     = 0.0f;
	menuDef_t *menu = (menuDef_t *)item;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		menu->window.borderColor[i] = f;
	}

	return qtrue;
}

qboolean MenuParse_focuscolor(itemDef_t *item, int handle)
{
	int       i;
	float     f     = 0.0f;
	menuDef_t *menu = (menuDef_t *)item;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		menu->focusColor[i] = f;
	}

	item->window.flags |= WINDOW_FOCUSPULSE;
	return qtrue;
}

qboolean MenuParse_disablecolor(itemDef_t *item, int handle)
{
	int       i;
	float     f     = 0.0f;
	menuDef_t *menu = (menuDef_t *)item;

	for (i = 0; i < 4; i++)
	{
		if (!PC_Float_Parse(handle, &f))
		{
			return qfalse;
		}

		menu->disableColor[i] = f;
	}

	return qtrue;
}

qboolean MenuParse_outlinecolor(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Color_Parse(handle, &menu->window.outlineColor))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_background(itemDef_t *item, int handle)
{
	const char *buff = NULL;
	menuDef_t  *menu = (menuDef_t *)item;

	if (!PC_String_Parse(handle, &buff))
	{
		return qfalse;
	}

	menu->window.background = DC->registerShaderNoMip(buff);
	return qtrue;
}

qboolean MenuParse_cinematic(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_String_Parse(handle, &menu->window.cinematicName))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_ownerdrawFlag(itemDef_t *item, int handle)
{
	int       i     = 0;
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &i))
	{
		return qfalse;
	}

	menu->window.ownerDrawFlags |= i;
	return qtrue;
}

qboolean MenuParse_ownerdraw(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &menu->window.ownerDraw))
	{
		return qfalse;
	}

	return qtrue;
}

// decoration
qboolean MenuParse_popup(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;
	menu->window.flags |= WINDOW_POPUP;
	return qtrue;
}

qboolean MenuParse_outOfBounds(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	menu->window.flags |= WINDOW_OOB_CLICK;
	return qtrue;
}

qboolean MenuParse_soundLoop(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_String_Parse(handle, &menu->soundName))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_fadeClamp(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Float_Parse(handle, &menu->fadeClamp))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_fadeAmount(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Float_Parse(handle, &menu->fadeAmount))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_fadeCycle(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (!PC_Int_Parse(handle, &menu->fadeCycle))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_itemDef(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;

	if (menu->itemCount < MAX_MENUITEMS)
	{
		menu->items[menu->itemCount] = UI_Alloc(sizeof(itemDef_t));
		Item_Init(menu->items[menu->itemCount]);
		if (!Item_Parse(handle, menu->items[menu->itemCount]))
		{
			return qfalse;
		}

		menu->items[menu->itemCount]->parent = menu;
		Item_InitControls(menu->items[menu->itemCount++]);

		// If we are storing the hotkeys in the items, we have a little problem, in that
		//      people check with the menu to see if we have a hotkey (see UI_CheckExecKey)
		//      So we sort of need to stuff the hotkey back into the menu for that to work
		//      only do this at all if we're using the item hotkey mode
		//      NOTE:  we couldn't do this earlier because the menu wasn't set, and I don't know
		//      what would happen if we tried to set the menu before the parse had succeeded...
		if (menu->itemHotkeyMode && menu->items[menu->itemCount - 1]->hotkey >= 0)
		{
			menu->onKey[menu->items[menu->itemCount - 1]->hotkey] = String_Alloc(menu->items[menu->itemCount - 1]->onKey);
		}
	}

	return qtrue;
}

qboolean MenuParse_execKey(itemDef_t *item, int handle)
{
	menuDef_t *menu   = (menuDef_t *)item;
	char      keyname = 0;
	short int keyindex;

	if (!PC_Char_Parse(handle, &keyname))
	{
		return qfalse;
	}

	keyindex = keyname;

	if (!PC_Script_Parse(handle, &menu->onKey[keyindex]))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_execKeyInt(itemDef_t *item, int handle)
{
	menuDef_t *menu   = (menuDef_t *)item;
	int       keyname = 0;

	if (!PC_Int_Parse(handle, &keyname))
	{
		return qfalse;
	}

	if (!PC_Script_Parse(handle, &menu->onKey[keyname]))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_drawAlwaysOnTop(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;
	menu->window.flags |= WINDOW_DRAWALWAYSONTOP;
	return qtrue;
}

// parse the command to set if we're looping through all items to find the current hotkey
qboolean MenuParse_itemHotkeyMode(itemDef_t *item, int handle)
{
	// like MenuParse_fullscreen - reading an int
	menuDef_t *menu = (menuDef_t *)item;
	if (!PC_Int_Parse(handle, (int *)&menu->itemHotkeyMode))
	{
		return qfalse;
	}

	return qtrue;
}

qboolean MenuParse_modal(itemDef_t *item, int handle)
{
	menuDef_t *menu = (menuDef_t *)item;
	menu->window.flags |= WINDOW_MODAL;
	return qtrue;
}

keywordHash_t menuParseKeywords[] =
{
	{ "name",             MenuParse_name,            NULL },
	{ "fullscreen",       MenuParse_fullscreen,      NULL },
	{ "rect",             MenuParse_rect,            NULL },
	{ "style",            MenuParse_style,           NULL },
	{ "visible",          MenuParse_visible,         NULL },
	{ "onOpen",           MenuParse_onOpen,          NULL },
	{ "onClose",          MenuParse_onClose,         NULL },
	{ "onTimeout",        MenuParse_onTimeout,       NULL }, // menu timeout function
	{ "onESC",            MenuParse_onESC,           NULL },
	{ "onEnter",          MenuParse_onEnter,         NULL },
	{ "onPaste",          MenuParse_onPaste,         NULL },
	{ "border",           MenuParse_border,          NULL },
	{ "borderSize",       MenuParse_borderSize,      NULL },
	{ "backcolor",        MenuParse_backcolor,       NULL },
	{ "forecolor",        MenuParse_forecolor,       NULL },
	{ "bordercolor",      MenuParse_bordercolor,     NULL },
	{ "focuscolor",       MenuParse_focuscolor,      NULL },
	{ "disablecolor",     MenuParse_disablecolor,    NULL },
	{ "outlinecolor",     MenuParse_outlinecolor,    NULL },
	{ "background",       MenuParse_background,      NULL },
	{ "ownerdraw",        MenuParse_ownerdraw,       NULL },
	{ "ownerdrawFlag",    MenuParse_ownerdrawFlag,   NULL },
	{ "outOfBoundsClick", MenuParse_outOfBounds,     NULL },
	{ "soundLoop",        MenuParse_soundLoop,       NULL },
	{ "itemDef",          MenuParse_itemDef,         NULL },
	{ "cinematic",        MenuParse_cinematic,       NULL },
	{ "popup",            MenuParse_popup,           NULL },
	{ "fadeClamp",        MenuParse_fadeClamp,       NULL },
	{ "fadeCycle",        MenuParse_fadeCycle,       NULL },
	{ "fadeAmount",       MenuParse_fadeAmount,      NULL },
	{ "execKey",          MenuParse_execKey,         NULL },
	{ "execKeyInt",       MenuParse_execKeyInt,      NULL },
	{ "alwaysontop",      MenuParse_drawAlwaysOnTop, NULL },
	{ "modal",            MenuParse_modal,           NULL },

	// parse the command to set if we're looping through all items to find the current hotkey
	{ "itemHotkeyMode",   MenuParse_itemHotkeyMode,  NULL },
	{ NULL,               NULL,                      NULL }
};

keywordHash_t *menuParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Menu_SetupKeywordHash
===============
*/
void Menu_SetupKeywordHash(void)
{
	int i;

	memset(menuParseKeywordHash, 0, sizeof(keywordHash_t *) * KEYWORDHASH_SIZE);
	for (i = 0; menuParseKeywords[i].keyword; i++)
	{
		KeywordHash_Add(menuParseKeywordHash, &menuParseKeywords[i]);
	}
}

/*
===============
Menu_Parse
===============
*/
qboolean Menu_Parse(int handle, menuDef_t *menu)
{
	pc_token_t    token;
	keywordHash_t *key;

	if (!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}

	if (*token.string != '{')
	{
		return qfalse;
	}

	while (1)
	{
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken(handle, &token))
		{
			PC_SourceError(handle, "end of file inside menu\n");
			return qfalse;
		}

		if (*token.string == '}')
		{
			return qtrue;
		}

		key = KeywordHash_Find(menuParseKeywordHash, token.string);
		if (!key)
		{
			PC_SourceError(handle, "unknown menu keyword %s", token.string);
			continue;
		}

		if (!key->func((itemDef_t *)menu, handle))
		{
			PC_SourceError(handle, "couldn't parse menu keyword %s", token.string);
			return qfalse;
		}
	}

	return qfalse;
}
