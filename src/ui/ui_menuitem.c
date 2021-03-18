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
 * @file ui_menuitem.c
 * @brief string allocation/managment
 */

#include "ui_shared.h"
#include "ui_local.h"

/**
 * @brief Item_Tooltip_Initialize
 * @param[in,out] item
 */
void Item_Tooltip_Initialize(itemDef_t *item)
{
	item->text              = NULL;
	item->font              = UI_FONT_COURBD_21;
	item->textalignx        = 3;
	item->textaligny        = 10;
	item->textscale         = .2f;
	item->window.border     = WINDOW_BORDER_FULL;
	item->window.borderSize = 1.f;
	item->window.flags     &= ~WINDOW_VISIBLE;
	item->window.flags     |= (WINDOW_DRAWALWAYSONTOP | WINDOW_AUTOWRAPPED);
	Vector4Set(item->window.backColor, .9f, .9f, .75f, 1.f);
	Vector4Set(item->window.borderColor, 0.f, 0.f, 0.f, 1.f);
	Vector4Set(item->window.foreColor, 0.f, 0.f, 0.f, 1.f);
}

/**
 * @brief Item_SetScreenCoords
 * @param[in,out] item
 * @param[in] x
 * @param[in] y
 */
void Item_SetScreenCoords(itemDef_t *item, float x, float y)
{
	if (item == NULL)
	{
		return;
	}

	item->window.rect.x = x + item->window.rectClient.x;
	item->window.rect.y = y + item->window.rectClient.y;
	item->window.rect.w = item->window.rectClient.w;
	item->window.rect.h = item->window.rectClient.h;

	// FIXME: do the proper thing for the right borders here?
	/*if( item->window.border != 0 ) {
	item->window.rect.x += item->window.borderSize;
	item->window.rect.y += item->window.borderSize;
	item->window.rect.w -= 2 * item->window.borderSize;
	item->window.rect.h -= 2 * item->window.borderSize;
	}*/

	// Don't let tooltips draw off the screen.
	if (item->toolTipData)
	{
		Item_SetScreenCoords(item->toolTipData, x, y);
		{
			float val = (item->toolTipData->window.rect.x + item->toolTipData->window.rect.w) - 635.0f;

			if (val > 0.0f)
			{
				item->toolTipData->window.rectClient.x -= val;
				item->toolTipData->window.rect.x       -= val;
			}
		}
	}

	// force the text rects to recompute
	item->textRect.w = 0;
	item->textRect.h = 0;
}

/**
 * @brief Item_UpdatePosition
 * @param[in] item
 * @todo FIXME: consolidate this with nearby stuff
 */
void Item_UpdatePosition(itemDef_t *item)
{
	float     x, y;
	menuDef_t *menu;

	if (item == NULL || item->parent == NULL)
	{
		return;
	}

	menu = item->parent;

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	/*if (menu->window.border != 0) {
	x += menu->window.borderSize;
	y += menu->window.borderSize;
	}*/

	Item_SetScreenCoords(item, x, y);
}

/**
 * @brief Item_EnableShowViaCvar
 * @param[in] item
 * @param[in] flag
 * @return
 */
qboolean Item_EnableShowViaCvar(itemDef_t *item, int flag)
{
	char script[1024], *p;

	Com_Memset(script, 0, sizeof(script));
	if (item && item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest)
	{
		char       buff[1024];
		const char *val;

		DC->getCVarString(item->cvarTest, buff, sizeof(buff));

		Q_strcat(script, 1024, item->enableCvar);
		p = script;
		while (1)
		{
			val = NULL;
			// expect value then ; or NULL, NULL ends list
			if (!String_Parse(&p, &val))
			{
				return (item->cvarFlags & flag) ? qfalse : qtrue;
			}

			if (val[0] == ';' && val[1] == '\0')
			{
				continue;
			}

			// enable it if any of the values are true
			if (item->cvarFlags & flag)
			{
				if (Q_stricmp(buff, val) == 0)
				{
					return qtrue;
				}
			}
			else
			{
				// disable it if any of the values are true
				if (Q_stricmp(buff, val) == 0)
				{
					return qfalse;
				}
			}
		}

		//return (item->cvarFlags & flag) ? qfalse : qtrue;
	}

	return qtrue;
}

/**
 * @brief Sisplay if we poll on a server toggle setting
 * We want *current* settings, so this is a bit of a perf hit,
 * but this is only during UI display
 * @param[in] item
 * @param[in] fVoteTest
 * @return
 */
qboolean Item_SettingShow(itemDef_t *item, qboolean fVoteTest)
{
	char info[MAX_INFO_STRING];

	if (fVoteTest)
	{
		trap_Cvar_VariableStringBuffer("cg_ui_voteFlags", info, sizeof(info));
		return((atoi(info) & item->voteFlag) != item->voteFlag);
	}

	DC->getConfigString(CS_SERVERTOGGLES, info, sizeof(info));

	if (item->settingFlags & SVS_ENABLED_SHOW)
	{
		return(atoi(info) & item->settingTest);
	}

	if (item->settingFlags & SVS_DISABLED_SHOW)
	{
		return(!(atoi(info) & item->settingTest));
	}

	return qtrue;
}

/**
 * @brief Will optionaly set focus to this item
 * @param[in] item
 * @param[in] x
 * @param[in] y
 * @return
 */
qboolean Item_SetFocus(itemDef_t *item, float x, float y)
{
	int         i;
	itemDef_t   *oldFocus;
	sfxHandle_t *sfx      = &DC->Assets.itemFocusSound;
	qboolean    playSound = qfalse;
	menuDef_t   *parent; // bk001206: = (menuDef_t*)item->parent;

	// sanity check, non-null, not a decoration and does not already have the focus
	if (item == NULL || (item->window.flags & WINDOW_DECORATION) || (item->window.flags & WINDOW_HASFOCUS) || !(item->window.flags & WINDOW_VISIBLE))
	{
		return qfalse;
	}

	// this can be NULL.
	parent = (menuDef_t *)item->parent;

	// items can be enabled and disabled based on cvars
	if ((item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
	{
		return qfalse;
	}

	if ((item->cvarFlags & (CVAR_SHOW | CVAR_HIDE)) && !Item_EnableShowViaCvar(item, CVAR_SHOW))
	{
		return qfalse;
	}

	if ((item->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) && !Item_SettingShow(item, qfalse))
	{
		return qfalse;
	}

	if (item->voteFlag != 0 && !Item_SettingShow(item, qtrue))
	{
		return qfalse;
	}

	oldFocus = Menu_ClearFocus(item->parent);

	if (item->type == ITEM_TYPE_TEXT)
	{
		rectDef_t r = item->textRect;

		r.y -= r.h;
		if (Rect_ContainsPoint(&r, x, y))
		{
			item->window.flags |= WINDOW_HASFOCUS;
			if (item->focusSound)
			{
				sfx = &item->focusSound;
			}

			playSound = qtrue;
		}
		else
		{
			if (oldFocus)
			{
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if (oldFocus->onFocus)
				{
					Item_RunScript(oldFocus, NULL, oldFocus->onFocus);
				}
			}
		}
	}
	else
	{
		item->window.flags |= WINDOW_HASFOCUS;
		if (item->onFocus)
		{
			Item_RunScript(item, NULL, item->onFocus);
		}

		if (item->focusSound)
		{
			sfx = &item->focusSound;
		}

		playSound = qtrue;
	}

	if (playSound && sfx)
	{
		DC->startLocalSound(*sfx, CHAN_LOCAL_SOUND);
	}

	for (i = 0; i < parent->itemCount; i++)
	{
		if (parent->items[i] == item)
		{
			parent->cursorItem = i;
			break;
		}
	}

	return qtrue;
}

/**
 * @brief Item_ListBox_MaxScroll
 * @param[in] item
 * @return
 */
int Item_ListBox_MaxScroll(itemDef_t *item)
{
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	int          count    = DC->feederCount(item->special);
	int          max;

	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		max = count - (int)(item->window.rect.w / listPtr->elementWidth);
	}
	else
	{
		max = count - (int)(item->window.rect.h / listPtr->elementHeight);
	}

	if (max < 0)
	{
		return 0;
	}

	return max;
}

/**
 * @brief Item_ListBox_ThumbPosition
 * @param[in] item
 * @return
 */
int Item_ListBox_ThumbPosition(itemDef_t *item)
{
	float        max, pos, size;
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;

	max = Item_ListBox_MaxScroll(item);
	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		size = item->window.rect.w - (SCROLLBAR_SIZE * 2) - 2;
		if (max > 0)
		{
			pos = (size - SCROLLBAR_SIZE) / max;
		}
		else
		{
			pos = 0;
		}

		pos *= listPtr->startPos;
		return item->window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	}
	else
	{
		size = item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;
		if (max > 0)
		{
			pos = (size - SCROLLBAR_SIZE) / max;
		}
		else
		{
			pos = 0;
		}

		pos *= listPtr->startPos;

		return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

/**
 * @brief Item_ListBox_ThumbDrawPosition
 * @param[in] item
 * @return
 */
int Item_ListBox_ThumbDrawPosition(itemDef_t *item)
{
	if (itemCapture == item)
	{
		int min, max;

		if (item->window.flags & WINDOW_HORIZONTAL)
		{
			min = item->window.rect.x + SCROLLBAR_SIZE + 1;
			max = item->window.rect.x + item->window.rect.w - 2 * SCROLLBAR_SIZE - 1;
			if (DC->cursorx >= min + SCROLLBAR_SIZE / 2 && DC->cursorx <= max + SCROLLBAR_SIZE / 2)
			{
				return DC->cursorx - SCROLLBAR_SIZE / 2;
			}
			else
			{
				return Item_ListBox_ThumbPosition(item);
			}
		}
		else
		{
			min = item->window.rect.y + SCROLLBAR_SIZE + 1;
			max = item->window.rect.y + item->window.rect.h - 2 * SCROLLBAR_SIZE - 1;
			if (DC->cursory >= min + SCROLLBAR_SIZE / 2 && DC->cursory <= max + SCROLLBAR_SIZE / 2)
			{
				return DC->cursory - SCROLLBAR_SIZE / 2;
			}
			else
			{
				return Item_ListBox_ThumbPosition(item);
			}
		}
	}
	else
	{
		return Item_ListBox_ThumbPosition(item);
	}
}

/**
 * @brief Item_Slider_ThumbPosition
 * @param[in] item
 * @return
 */
float Item_Slider_ThumbPosition(itemDef_t *item)
{
	float          value, range, x;
	editFieldDef_t *editDef = item->typeData;

	if (item->text)
	{
		x = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		x = item->window.rect.x;
	}

	if (!editDef || !item->cvar)
	{
		return x;
	}

	value = DC->getCVarValue(item->cvar);

	if (value < editDef->minVal)
	{
		value = editDef->minVal;
	}
	else if (value > editDef->maxVal)
	{
		value = editDef->maxVal;
	}

	range  = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	//value /= (editDef->maxVal - editDef->minVal);
	value *= SLIDER_WIDTH;
	x     += value;
	// vm fuckage
	//x = x + (((float)value / editDef->maxVal) * SLIDER_WIDTH);
	return x;
}

/**
 * @brief Item_Slider_OverSlider
 * @param[in] item
 * @param[in] x
 * @param[in] y
 * @return
 */
int Item_Slider_OverSlider(itemDef_t *item, float x, float y)
{
	rectDef_t r;

	r.x = Item_Slider_ThumbPosition(item) - (SLIDER_THUMB_WIDTH / 2);
	r.y = item->window.rect.y;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if (Rect_ContainsPoint(&r, x, y))
	{
		return WINDOW_LB_THUMB;
	}

	return 0;
}

/**
 * @brief Item_ListBox_OverLB
 * @param[in] item
 * @param[in] x
 * @param[in] y
 * @return
 */
int Item_ListBox_OverLB(itemDef_t *item, float x, float y)
{
	rectDef_t r;
	int       thumbstart;

	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		// check if on left arrow
		r.x = item->window.rect.x;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_LEFTARROW;
		}

		// check if on right arrow
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_RIGHTARROW;
		}

		// check if on thumb
		//thumbstart = Item_ListBox_ThumbPosition(item);
		thumbstart = Item_ListBox_ThumbDrawPosition(item);
		r.x        = thumbstart;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_THUMB;
		}

		r.x = item->window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGUP;
		}

		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGDN;
		}

		// hack hack
		r.x = item->window.rect.x;
		r.w = item->window.rect.w;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_SOMEWHERE;
		}
	}
	else
	{
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		r.y = item->window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_LEFTARROW;
		}

		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_RIGHTARROW;
		}

		//thumbstart = Item_ListBox_ThumbPosition(item);
		thumbstart = Item_ListBox_ThumbDrawPosition(item);
		r.y        = thumbstart;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_THUMB;
		}

		r.y = item->window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGUP;
		}

		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_PGDN;
		}

		// hack hack
		r.y = item->window.rect.y;
		r.h = item->window.rect.h;
		if (Rect_ContainsPoint(&r, x, y))
		{
			return WINDOW_LB_SOMEWHERE;
		}
	}

	return 0;
}

/**
 * @brief Item_ListBox_MouseEnter
 * @param[in] item
 * @param[in] x
 * @param[in] y
 * @param[in] click
 */
void Item_ListBox_MouseEnter(itemDef_t *item, float x, float y, qboolean click)
{
	rectDef_t    r;
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;

	item->window.flags &= ~(WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN | WINDOW_LB_SOMEWHERE);
	item->window.flags |= Item_ListBox_OverLB(item, x, y);

	if (click)
	{
		if (item->window.flags & WINDOW_HORIZONTAL)
		{
			if (!(item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN | WINDOW_LB_SOMEWHERE)))
			{
				// check for selection hit as we have exausted buttons and thumb
				if (listPtr->elementStyle == LISTBOX_IMAGE)
				{
					r.x = item->window.rect.x;
					r.y = item->window.rect.y;
					r.h = item->window.rect.h - SCROLLBAR_SIZE;
					r.w = item->window.rect.w - listPtr->drawPadding;
					if (Rect_ContainsPointN(&r, x, y))
					{
						listPtr->cursorPos = (int)((x - r.x) / listPtr->elementWidth) + listPtr->startPos;

						if (listPtr->cursorPos >= listPtr->endPos)
						{
							listPtr->cursorPos = listPtr->endPos;
						}
					}
				}
				else
				{
					// text hit..
				}
			}
		}
		else if (!(item->window.flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN | WINDOW_LB_SOMEWHERE)))
		{
			r.x = item->window.rect.x;
			r.y = item->window.rect.y;
			r.w = item->window.rect.w - SCROLLBAR_SIZE;
			r.h = item->window.rect.h - listPtr->drawPadding;
			if (Rect_ContainsPointN(&r, x, y))
			{
				listPtr->cursorPos = (int)((y - 2 - r.y) / listPtr->elementHeight) + listPtr->startPos;

				if (listPtr->cursorPos > listPtr->endPos)
				{
					listPtr->cursorPos = listPtr->endPos;
				}
			}
		}
	}
}

/**
 * @brief Item_MouseEnter
 * @param[in] item
 * @param[in] x
 * @param[in] y
 */
void Item_MouseEnter(itemDef_t *item, float x, float y)
{
	if (item)
	{
		rectDef_t r = item->textRect;

		r.y -= r.h;
		// in the text rect?

		// items can be enabled and disabled based on cvars
		if ((item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			return;
		}

		if ((item->cvarFlags & (CVAR_SHOW | CVAR_HIDE)) && !Item_EnableShowViaCvar(item, CVAR_SHOW))
		{
			return;
		}

		// server settings too .. (mostly for callvote)
		if ((item->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) && !Item_SettingShow(item, qfalse))
		{
			return;
		}

		if (item->voteFlag != 0 && !Item_SettingShow(item, qtrue))
		{
			return;
		}

		if (Rect_ContainsPoint(&r, x, y))
		{
			if (!(item->window.flags & WINDOW_MOUSEOVERTEXT))
			{
				Item_RunScript(item, NULL, item->mouseEnterText);
				item->window.flags |= WINDOW_MOUSEOVERTEXT;
			}

			if (!(item->window.flags & WINDOW_MOUSEOVER))
			{
				Item_RunScript(item, NULL, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}
		}
		else
		{
			// not in the text rect
			if (item->window.flags & WINDOW_MOUSEOVERTEXT)
			{
				// if we were
				Item_RunScript(item, NULL, item->mouseExitText);
				item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
			}

			if (!(item->window.flags & WINDOW_MOUSEOVER))
			{
				Item_RunScript(item, NULL, item->mouseEnter);
				item->window.flags |= WINDOW_MOUSEOVER;
			}

			if (item->type == ITEM_TYPE_LISTBOX)
			{
				Item_ListBox_MouseEnter(item, x, y, qfalse);
			}
		}
	}
}

/**
 * @brief Item_MouseLeave
 * @param[in,out] item
 */
void Item_MouseLeave(itemDef_t *item)
{
	if (item)
	{
		if (item->window.flags & WINDOW_MOUSEOVERTEXT)
		{
			Item_RunScript(item, NULL, item->mouseExitText);
			item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
		}

		Item_RunScript(item, NULL, item->mouseExit);
		item->window.flags &= ~(WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW);
	}
}

/**
 * @brief Item_SetMouseOver
 * @param[in,out] item
 * @param[in] focus
 */
void Item_SetMouseOver(itemDef_t *item, qboolean focus)
{
	if (item)
	{
		if (focus)
		{
			item->window.flags |= WINDOW_MOUSEOVER;
		}
		else
		{
			item->window.flags &= ~WINDOW_MOUSEOVER;
		}
	}
}

/**
 * @brief Item_OwnerDraw_HandleKey
 * @param[in,out] item
 * @param[in] key
 * @return
 */
qboolean Item_OwnerDraw_HandleKey(itemDef_t *item, int key)
{
	if (item && DC->ownerDrawHandleKey)
	{
		return DC->ownerDrawHandleKey(item->window.ownerDraw, item->window.ownerDrawFlags, &item->special, key);
	}

	return qfalse;
}

/**
 * @brief Item_ListBox_HandleKey
 * @param[in,out] item
 * @param[in] key
 * @param down - unused
 * @param[in] force
 * @return
 */
qboolean Item_ListBox_HandleKey(itemDef_t *item, int key, qboolean down, qboolean force)
{
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	int          count    = DC->feederCount(item->special);

	if (force || (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && (item->window.flags & WINDOW_HASFOCUS)))
	{
		int max = Item_ListBox_MaxScroll(item);
		int viewmax;

		if (item->window.flags & WINDOW_HORIZONTAL)
		{
			viewmax = (int)(item->window.rect.w / listPtr->elementWidth);

			if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos--;
					if (listPtr->cursorPos < 0)
					{
						listPtr->cursorPos = 0;
					}

					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}

					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}

					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos--;
					if (listPtr->startPos < 0)
					{
						listPtr->startPos = 0;
					}
				}

				return qtrue;
			}
			if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos++;
					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}

					if (listPtr->cursorPos >= count)
					{
						listPtr->cursorPos = count - 1;
					}

					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}

					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos++;
					if (listPtr->startPos >= count)
					{
						listPtr->startPos = count - 1;
					}
				}

				return qtrue;
			}
		}
		else
		{
			viewmax = (int)(item->window.rect.h / listPtr->elementHeight);
			if (key == K_UPARROW || key == K_KP_UPARROW || key == K_MWHEELUP)
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos--;
					if (listPtr->cursorPos < 0)
					{
						listPtr->cursorPos = 0;
					}

					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}

					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}

					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos--;
					if (listPtr->startPos < 0)
					{
						listPtr->startPos = 0;
					}
				}

				return qtrue;
			}
			if (key == K_DOWNARROW || key == K_KP_DOWNARROW || key == K_MWHEELDOWN)
			{
				if (!listPtr->notselectable)
				{
					listPtr->cursorPos++;
					if (listPtr->cursorPos < listPtr->startPos)
					{
						listPtr->startPos = listPtr->cursorPos;
					}

					if (listPtr->cursorPos >= count)
					{
						listPtr->cursorPos = count - 1;
					}

					if (listPtr->cursorPos >= listPtr->startPos + viewmax)
					{
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}

					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}
				else
				{
					listPtr->startPos++;
					if (listPtr->startPos > max)
					{
						listPtr->startPos = max;
					}
				}

				return qtrue;
			}
		}
		// mouse hit
		if (key == K_MOUSE1 || key == K_MOUSE2)
		{
			Item_ListBox_MouseEnter(item, DC->cursorx, DC->cursory, qtrue);

			if (item->window.flags & WINDOW_LB_LEFTARROW)
			{
				listPtr->startPos--;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			else if (item->window.flags & WINDOW_LB_RIGHTARROW)
			{
				// one down
				listPtr->startPos++;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			else if (item->window.flags & WINDOW_LB_PGUP)
			{
				// page up
				listPtr->startPos -= viewmax;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}
			else if (item->window.flags & WINDOW_LB_PGDN)
			{
				// page down
				listPtr->startPos += viewmax;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}
			else if (item->window.flags & WINDOW_LB_THUMB)
			{
				// Display_SetCaptureItem(item);
			}
			else if (item->window.flags & WINDOW_LB_SOMEWHERE)
			{
				// do nowt
			}
			else
			{
				// select an item
				// can't select something that doesn't exist
				if (listPtr->cursorPos >= count)
				{
					listPtr->cursorPos = count - 1;
				}

				if (item->cursorPos == listPtr->cursorPos &&
				    DC->realTime < lastListBoxClickTime && listPtr->doubleClick)
				{
					Item_RunScript(item, NULL, listPtr->doubleClick);
				}

				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;

				if (item->cursorPos != listPtr->cursorPos)
				{
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection(item->special, item->cursorPos);
				}

				if (key == K_MOUSE1)
				{
					DC->feederSelectionClick(item);
				}

				if (key == K_MOUSE2 && listPtr->contextMenu)
				{
					menuDef_t *menu = Menus_FindByName(listPtr->contextMenu);

					if (menu)
					{
						menu->window.rect.x = DC->cursorx;
						menu->window.rect.y = DC->cursory;

						Menu_UpdatePosition(menu);
						Menus_ActivateByName(listPtr->contextMenu, qtrue);
					}
				}
			}

			return qtrue;
		}
		if (key == K_HOME || key == K_KP_HOME)
		{
			// home
			listPtr->startPos = 0;
			return qtrue;
		}

		if (key == K_END || key == K_KP_END)
		{
			// end
			listPtr->startPos = max;
			return qtrue;
		}

		if (key == K_PGUP || key == K_KP_PGUP)
		{
			// page up
			if (!listPtr->notselectable)
			{
				listPtr->cursorPos -= viewmax;
				if (listPtr->cursorPos < 0)
				{
					listPtr->cursorPos = 0;
				}

				if (listPtr->cursorPos < listPtr->startPos)
				{
					listPtr->startPos = listPtr->cursorPos;
				}

				if (listPtr->cursorPos >= listPtr->startPos + viewmax)
				{
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}

				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos);
			}
			else
			{
				listPtr->startPos -= viewmax;
				if (listPtr->startPos < 0)
				{
					listPtr->startPos = 0;
				}
			}

			return qtrue;
		}

		if (key == K_PGDN || key == K_KP_PGDN)
		{
			// page down
			if (!listPtr->notselectable)
			{
				listPtr->cursorPos += viewmax;
				if (listPtr->cursorPos < listPtr->startPos)
				{
					listPtr->startPos = listPtr->cursorPos;
				}

				if (listPtr->cursorPos >= count)
				{
					listPtr->cursorPos = count - 1;
				}

				if (listPtr->cursorPos >= listPtr->startPos + viewmax)
				{
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}

				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection(item->special, item->cursorPos);
			}
			else
			{
				listPtr->startPos += viewmax;
				if (listPtr->startPos > max)
				{
					listPtr->startPos = max;
				}
			}

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Item_CheckBox_HandleKey
 * @param[in] item
 * @param[in] key
 * @return
 */
qboolean Item_CheckBox_HandleKey(itemDef_t *item, int key)
{
	if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && (item->window.flags & WINDOW_HASFOCUS) && item->cvar)
	{
		if (key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
		{
			// added the flag to toggle via action script only
			if (!(item->cvarFlags & CVAR_NOTOGGLE))
			{
				if (item->type == ITEM_TYPE_TRICHECKBOX)
				{
					int curvalue;

					if (key == K_MOUSE2)
					{
						curvalue = (int)(DC->getCVarValue(item->cvar) - 1);
					}
					else
					{
						curvalue = (int)(DC->getCVarValue(item->cvar) + 1);
					}

					if (curvalue > 2)
					{
						curvalue = 0;
					}
					else if (curvalue < 0)
					{
						curvalue = 2;
					}

					DC->setCVar(item->cvar, va("%i", curvalue));
				}
				else
				{
					DC->setCVar(item->cvar, va("%i", DC->getCVarValue(item->cvar) == 0.f));
				}
			}

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Item_YesNo_HandleKey
 * @param[in] item
 * @param[in] key
 * @return
 */
qboolean Item_YesNo_HandleKey(itemDef_t *item, int key)
{
	if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && (item->window.flags & WINDOW_HASFOCUS) && item->cvar)
	{
		if (key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
		{
			// added the flag to toggle via action script only
			if (!(item->cvarFlags & CVAR_NOTOGGLE))
			{
				DC->setCVar(item->cvar, va("%i", !DC->getCVarValue(item->cvar)));
			}

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Item_Multi_CountSettings
 * @param[in] item
 * @return
 */
int Item_Multi_CountSettings(itemDef_t *item)
{
	multiDef_t *multiPtr = (multiDef_t *)item->typeData;

	if (multiPtr == NULL)
	{
		return 0;
	}

	return multiPtr->count;
}

/**
 * @brief Item_Multi_FindCvarByValue
 * @param[in] item
 * @return
 */
int Item_Multi_FindCvarByValue(itemDef_t *item)
{
	char       buff[1024];
	multiDef_t *multiPtr = (multiDef_t *)item->typeData;

	if (multiPtr)
	{
		int   i;
		float value = 0;

		if (multiPtr->strDef)
		{
			DC->getCVarString(item->cvar, buff, sizeof(buff));
		}
		else
		{
			value = DC->getCVarValue(item->cvar);
		}

		for (i = 0; i < multiPtr->count; i++)
		{
			if (multiPtr->strDef)
			{
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0)
				{
					return i;
				}
			}
			else
			{
				if (multiPtr->cvarValue[i] == value)
				{
					return i;
				}
			}
		}
	}

	return 0;
}

/**
 * @brief Item_Multi_Setting
 * @param[in] item
 * @return
 */
const char *Item_Multi_Setting(itemDef_t *item)
{
	multiDef_t *multiPtr = (multiDef_t *)item->typeData;

	if (multiPtr)
	{
		char  buff[1024];
		float value = 0;
		int   i;

		if (multiPtr->strDef)
		{
			DC->getCVarString(item->cvar, buff, sizeof(buff));
		}
		else
		{
			value = DC->getCVarValue(item->cvar);
		}

		for (i = 0; i < multiPtr->count; i++)
		{
			if (multiPtr->strDef)
			{
				if (Q_stricmp(buff, multiPtr->cvarStr[i]) == 0)
				{
					return multiPtr->cvarList[i];
				}
			}
			else
			{
				if (multiPtr->cvarValue[i] == value)
				{
					return multiPtr->cvarList[i];
				}
			}
		}

		if (multiPtr->undefinedStr)
		{
			return multiPtr->undefinedStr;
		}
		else
		{
			return((multiPtr->count == 0) ? DC->translateString(_("None Defined")) : DC->translateString(_("Custom")));
		}
	}
	return "";
}

/**
 * @brief Item_Multi_HandleKey
 * @param[in] item
 * @param[in] key
 * @return
 */
qboolean Item_Multi_HandleKey(itemDef_t *item, int key)
{
	multiDef_t *multiPtr = (multiDef_t *)item->typeData;

	if (multiPtr)
	{
		if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && (item->window.flags & WINDOW_HASFOCUS) && item->cvar)
		{
			if (key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
			{
				int current = Item_Multi_FindCvarByValue(item);
				int max     = Item_Multi_CountSettings(item);

				if (key == K_MOUSE2)
				{
					current--;
				}
				else
				{
					current++;
				}

				if (current < 0)
				{
					current = max - 1;
				}
				else if (current >= max)
				{
					current = 0;
				}

				if (multiPtr->strDef)
				{
					DC->setCVar(item->cvar, multiPtr->cvarStr[current]);
				}
				else
				{
					float value = multiPtr->cvarValue[current];

					if (((float)((int)value)) == value)
					{
						DC->setCVar(item->cvar, va("%i", (int)value));
					}
					else
					{
						DC->setCVar(item->cvar, va("%f", value));
					}
				}

				return qtrue;
			}
		}
	}

	return qfalse;
}

/**
 * @brief Item_ComboSelect
 * @param[in] item
 */
void Item_ComboSelect(itemDef_t *item)
{
	if (item)
	{
		SET_EDITITEM(item);
	}
}

/**
 * @brief Item_ComboDeSelect
 * @param item - unused
 */
void Item_ComboDeSelect(itemDef_t *item)
{
	CLEAR_EDITITEM();
}

/**
 * @brief Item_CalcTextFieldCursor
 * @param[in,out] item
 */
void Item_CalcTextFieldCursor(itemDef_t *item)
{
	if (item->cvar)
	{
		char           buff[1024];
		int            len;
		editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;

		Com_Memset(buff, 0, sizeof(buff));
		DC->getCVarString(item->cvar, buff, sizeof(buff));
		len = Q_UTF8_Strlen(buff);

		if (editPtr && editPtr->maxChars && len > editPtr->maxChars)
		{
			len = editPtr->maxChars;
		}

		item->cursorPos = len;
		DC->setCVar(EDITFIELD_TEMP_CVAR, buff);
	}
}

/**
 * @brief Item_HandleTextFieldSelect
 * @param[in] item
 */
void Item_HandleTextFieldSelect(itemDef_t *item)
{
	if (item)
	{
		Item_CalcTextFieldCursor(item);
		SET_EDITITEM(item);
	}
}

/**
 * @brief Item_HandleTextFieldDeSelect
 * @param[in] item
 */
void Item_HandleTextFieldDeSelect(itemDef_t *item)
{
	if (item && item->cvar)
	{
		char buff[1024];

		DC->getCVarString(EDITFIELD_TEMP_CVAR, buff, sizeof(buff));
		DC->setCVar(item->cvar, buff);
	}

	CLEAR_EDITITEM();
}

/**
 * @brief Item_HandleSaveValue
 */
void Item_HandleSaveValue(void)
{
	if (g_editItem && TEXTFIELD(g_editItem->type))
	{
		itemDef_t *temp = g_editItem;
		Item_HandleTextFieldDeSelect(temp);
		Item_HandleTextFieldSelect(temp);
	}
}

/**
 * @brief Item_TextField_InsertToCursor
 * @param[in,out] len
 * @param[in,out] buff
 * @param[in] key
 * @param[in,out] item
 * @param[in,out] editPtr
 * @return
 */
static qboolean Item_TextField_InsertToCursor(int *len, char *buff, int key, itemDef_t *item, editFieldDef_t *editPtr)
{
	qboolean overStrike = qfalse;

	if (DC->getOverstrikeMode && !DC->getOverstrikeMode())
	{
		if ((*len == MAX_EDITFIELD - 1) || (editPtr->maxChars && *len >= editPtr->maxChars))
		{
			return qtrue;
		}
	}
	else
	{
		if (editPtr->maxChars && item->cursorPos >= editPtr->maxChars)
		{
			return qtrue;
		}
		overStrike = qtrue;
	}

	Q_UTF8_Insert(buff, *len, item->cursorPos, key, overStrike);

	if (item->cursorPos < *len + 1)
	{
		item->cursorPos++;
		if (editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars)
		{
			editPtr->paintOffset++;
		}
	}

	*len += 1;
	return qfalse;
}

/**
 * @brief Item_Combo_HandleKey
 * @param[in] item
 * @param[in] key
 * @return
 */
qboolean Item_Combo_HandleKey(itemDef_t *item, int key)
{
	multiDef_t *multi = NULL;

	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER)
	{
		if (item->cursorPos >= 0)
		{
			multi = (multiDef_t *)item->typeData;
			if (multi->strDef)
			{
				DC->setCVar(item->cvar, multi->cvarStr[item->cursorPos]);
			}
			else
			{
				DC->setCVar(item->cvar, va("%.0f", (double)multi->cvarValue[item->cursorPos]));
			}
			Item_RunScript(item, NULL, item->onAccept);
			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief Item_TextField_HandleKey
 * @param[in] item
 * @param[in] key
 * @return
 */
qboolean Item_TextField_HandleKey(itemDef_t *item, int key)
{
	char           buff[1024];
	int            valueLen;
	itemDef_t      *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;

	if (!item->cvar)
	{
		return qfalse;
	}

	Com_Memset(buff, 0, sizeof(buff));
	DC->getCVarString(EDITFIELD_TEMP_CVAR, buff, sizeof(buff));

	valueLen = Q_UTF8_Strlen(buff);

	if (editPtr->maxChars && valueLen > editPtr->maxChars)
	{
		valueLen = editPtr->maxChars;
		size_t cutOff = Q_UTF8_ByteOffset(buff, editPtr->maxChars);
		Com_Memset(&buff[cutOff], 0, sizeof(buff) - cutOff);
	}

	// make sure our cursor pos doesn't go oob, windows doesn't like negative memory copy operations :)
	if (item->cursorPos < 0 || item->cursorPos > valueLen)
	{
		item->cursorPos = 0;
	}

	if (key & K_CHAR_FLAG)
	{
		key &= ~K_CHAR_FLAG;

		if (key == 'h' - 'a' + 1)          // ctrl-h is backspace
		{
			if (item->cursorPos > 0)
			{
				Q_UTF8_Move(buff, item->cursorPos - 1, item->cursorPos, valueLen + 1 - item->cursorPos);
				item->cursorPos--;
				if (item->cursorPos < editPtr->paintOffset)
				{
					editPtr->paintOffset--;
				}
			}

			DC->setCVar(EDITFIELD_TEMP_CVAR, buff);
			return qtrue;
		}

		// ignore any non printable chars
		if (key < 32 || !item->cvar)
		{
			return qtrue;
		}

		if (item->type == ITEM_TYPE_NUMERICFIELD)
		{
			if ((key < '0' || key > '9') && key != '.')
			{
				return qfalse;
			}
		}

		if (Item_TextField_InsertToCursor(&valueLen, buff, key, item, editPtr))
		{
			return qtrue;
		}

		DC->setCVar(EDITFIELD_TEMP_CVAR, buff);
	}
	else
	{
		if (K_CLIPBOARD(key))  // clipboard paste only on normal textfield
		{
			if (item->type != ITEM_TYPE_NUMERICFIELD)
			{
				char clipbuff[1024];
				uint32_t  clipbuff32[256];

				Com_Memset(clipbuff, 0, sizeof(clipbuff));
				Com_Memset(clipbuff32, 0, sizeof(uint32_t) * 256);

				DC->getClipboardData(clipbuff, sizeof(clipbuff));
				if (strlen(clipbuff))
				{
					int i = 0;
					size_t cliplen = 0;

					Q_UTF8_ToUTF32(clipbuff, clipbuff32, &cliplen);
					for (; i < cliplen; i++)
					{
						if (Item_TextField_InsertToCursor(&valueLen, buff, clipbuff32[i], item, editPtr))
						{
							break;
						}
					}

					DC->setCVar(EDITFIELD_TEMP_CVAR, buff);
				}

				return qtrue;
			}
			else if (item->onPaste)
			{
				// We handle the clipboard action on the script level
				return qfalse;
			}
		}

		if (key == K_DEL || key == K_KP_DEL)
		{
			if (item->cursorPos < valueLen)
			{
				Q_UTF8_Move(buff, item->cursorPos, item->cursorPos + 1, valueLen - item->cursorPos);
				DC->setCVar(EDITFIELD_TEMP_CVAR, buff);
			}

			return qtrue;
		}

		if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW)
		{
			if (editPtr->maxPaintChars && item->cursorPos >= editPtr->paintOffset + editPtr->maxPaintChars && item->cursorPos < valueLen)
			{
				item->cursorPos++;
				editPtr->paintOffset++;
				return qtrue;
			}

			if (item->cursorPos < valueLen)
			{
				item->cursorPos++;
			}

			return qtrue;
		}

		if (key == K_LEFTARROW || key == K_KP_LEFTARROW)
		{
			if (item->cursorPos > 0)
			{
				item->cursorPos--;
			}

			if (item->cursorPos < editPtr->paintOffset)
			{
				editPtr->paintOffset--;
			}

			return qtrue;
		}

		if (key == K_HOME || key == K_KP_HOME)
		{
			item->cursorPos      = 0;
			editPtr->paintOffset = 0;
			return qtrue;
		}

		if (key == K_END || key == K_KP_END)
		{
			item->cursorPos = valueLen;
			if (item->cursorPos > editPtr->maxPaintChars)
			{
				editPtr->paintOffset = valueLen - editPtr->maxPaintChars;
			}

			return qtrue;
		}

		if (key == K_INS || key == K_KP_INS)
		{
			DC->setOverstrikeMode(!DC->getOverstrikeMode());
			return qtrue;
		}
	}

	if ((key == K_TAB && !item->onTab) || key == K_DOWNARROW || key == K_KP_DOWNARROW)
	{
		newItem = Menu_SetNextCursorItem(item->parent);
		if (newItem && TEXTFIELD(newItem->type))
		{
			Item_HandleTextFieldDeSelect(item);
			Item_HandleTextFieldSelect(newItem);
			//g_editItem = newItem;
		}
		else
		{
			Item_HandleSaveValue();
		}
	}
	else if (key == K_TAB && item->onTab)
	{
		Item_RunScript(item, NULL, item->onTab);
		return qtrue;
	}

	if (key == K_UPARROW || key == K_KP_UPARROW)
	{
		newItem = Menu_SetPrevCursorItem(item->parent);
		if (newItem && TEXTFIELD(newItem->type))
		{
			Item_HandleTextFieldDeSelect(item);
			Item_HandleTextFieldSelect(newItem);
			//g_editItem = newItem;
		}
	}

	if (key == K_ENTER || key == K_KP_ENTER || key == K_ESCAPE)
	{
		if ((key == K_ENTER || key == K_KP_ENTER) && item->onAccept)
		{
			Item_RunScript(item, NULL, item->onAccept);
		}
		else if (key == K_ESCAPE && item->onEsc)
		{
			Item_RunScript(item, NULL, item->onEsc);
		}

		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Item_Scroll_ListBox_AutoFunc
 * @param[in,out] p
 */
static void Item_Scroll_ListBox_AutoFunc(void *p)
{
	scrollInfo_t *si = (scrollInfo_t *)p;

	if (DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if (DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/**
 * @brief Item_Scroll_ListBox_ThumbFunc
 * @param[in,out] p
 */
static void Item_Scroll_ListBox_ThumbFunc(void *p)
{
	scrollInfo_t *si = (scrollInfo_t *)p;
	rectDef_t    r;
	int          pos, max;
	listBoxDef_t *listPtr = (listBoxDef_t *)si->item->typeData;

	if (si->item->window.flags & WINDOW_HORIZONTAL)
	{
		if (DC->cursorx == si->xStart)
		{
			return;
		}

		r.x = si->item->window.rect.x + SCROLLBAR_SIZE + 1;
		r.y = si->item->window.rect.y + si->item->window.rect.h - SCROLLBAR_SIZE - 1;
		r.h = SCROLLBAR_SIZE;
		r.w = si->item->window.rect.w - (SCROLLBAR_SIZE * 2) - 2;
		max = Item_ListBox_MaxScroll(si->item);

		pos = (DC->cursorx - r.x - SCROLLBAR_SIZE / 2) * max / (r.w - SCROLLBAR_SIZE);
		if (pos < 0)
		{
			pos = 0;
		}
		else if (pos > max)
		{
			pos = max;
		}

		listPtr->startPos = pos;
		si->xStart        = DC->cursorx;
	}
	else if (DC->cursory != si->yStart)
	{
		r.x = si->item->window.rect.x + si->item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = si->item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = si->item->window.rect.h - (SCROLLBAR_SIZE * 2) - 2;
		r.w = SCROLLBAR_SIZE;
		max = Item_ListBox_MaxScroll(si->item);
		//
		pos = (DC->cursory - r.y - SCROLLBAR_SIZE / 2) * max / (r.h - SCROLLBAR_SIZE);
		if (pos < 0)
		{
			pos = 0;
		}
		else if (pos > max)
		{
			pos = max;
		}

		listPtr->startPos = pos;
		si->yStart        = DC->cursory;
	}

	if (DC->realTime > si->nextScrollTime)
	{
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		// clear doubleclicktime though!
		lastListBoxClickTime = 0;
		Item_ListBox_HandleKey(si->item, si->scrollKey, qtrue, qfalse);
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if (DC->realTime > si->nextAdjustTime)
	{
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if (si->adjustValue > SCROLL_TIME_FLOOR)
		{
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/**
 * @brief Item_Scroll_Slider_ThumbFunc
 * @param[in] p
 */
static void Item_Scroll_Slider_ThumbFunc(void *p)
{
	float          x, value, cursorx;
	scrollInfo_t   *si      = (scrollInfo_t *)p;
	editFieldDef_t *editDef = si->item->typeData;

	if (si->item->text)
	{
		x = si->item->textRect.x + si->item->textRect.w + 8;
	}
	else
	{
		x = si->item->window.rect.x;
	}

	cursorx = DC->cursorx;

	if (cursorx < x)
	{
		cursorx = x;
	}
	else if (cursorx > x + SLIDER_WIDTH)
	{
		cursorx = x + SLIDER_WIDTH;
	}

	value  = cursorx - x;
	value /= SLIDER_WIDTH;
	value *= (editDef->maxVal - editDef->minVal);
	value += editDef->minVal;
	DC->setCVar(si->item->cvar, va("%f", (double)value));
}

/**
 * @brief Item_StartCapture
 * @param[in] item
 * @param[in] key
 */
void Item_StartCapture(itemDef_t *item, int key)
{
	int flags;

	switch (item->type)
	{
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:

	case ITEM_TYPE_LISTBOX:
	{
		flags = Item_ListBox_OverLB(item, DC->cursorx, DC->cursory);
		if (flags & (WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW))
		{
			scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
			scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
			scrollInfo.adjustValue    = SCROLL_TIME_START;
			scrollInfo.scrollKey      = key;
			scrollInfo.scrollDir      = (flags & WINDOW_LB_LEFTARROW) ? qtrue : qfalse;
			scrollInfo.item           = item;
			captureData               = &scrollInfo;
			captureFunc               = &Item_Scroll_ListBox_AutoFunc;
			itemCapture               = item;
		}
		else if (flags & WINDOW_LB_THUMB)
		{
			scrollInfo.scrollKey = key;
			scrollInfo.item      = item;
			scrollInfo.xStart    = DC->cursorx;
			scrollInfo.yStart    = DC->cursory;
			captureData          = &scrollInfo;
			captureFunc          = &Item_Scroll_ListBox_ThumbFunc;
			itemCapture          = item;
		}
		break;
	}
	case ITEM_TYPE_SLIDER:
	{
		flags = Item_Slider_OverSlider(item, DC->cursorx, DC->cursory);
		if (flags & WINDOW_LB_THUMB)
		{
			scrollInfo.scrollKey = key;
			scrollInfo.item      = item;
			scrollInfo.xStart    = DC->cursorx;
			scrollInfo.yStart    = DC->cursory;
			captureData          = &scrollInfo;
			captureFunc          = &Item_Scroll_Slider_ThumbFunc;
			itemCapture          = item;
		}
		break;
	}
	}
}

/**
 * @brief Item_Slider_HandleKey
 * @param[in] item
 * @param[in] key
 * @param down - unused
 * @return
 */
qboolean Item_Slider_HandleKey(itemDef_t *item, int key, qboolean down)
{
	//DC->Print("slider handle key\n");
	if ((item->window.flags & WINDOW_HASFOCUS) && item->cvar && Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
	{
		if (key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3)
		{
			editFieldDef_t *editDef = item->typeData;
			if (editDef)
			{
				rectDef_t testRect;
				float     x;
				float     value;

				if (item->text)
				{
					x = item->textRect.x + item->textRect.w + 8;
				}
				else
				{
					x = item->window.rect.x;
				}

				testRect    = item->window.rect;
				testRect.x  = x;
				value       = (float)SLIDER_THUMB_WIDTH / 2;
				testRect.x -= value;
				//DC->Print("slider x: %f\n", testRect.x);
				testRect.w = (SLIDER_WIDTH + (float)SLIDER_THUMB_WIDTH / 2);
				//DC->Print("slider w: %f\n", testRect.w);
				if (Rect_ContainsPoint(&testRect, DC->cursorx, DC->cursory))
				{
					float work = DC->cursorx - x;
					value  = work / SLIDER_WIDTH;
					value *= (editDef->maxVal - editDef->minVal);
					// vm fuckage
					// value = (((float)(DC->cursorx - x)/ SLIDER_WIDTH) * (editDef->maxVal - editDef->minVal));
					value += editDef->minVal;
					DC->setCVar(item->cvar, va("%f", (double)value));
					return qtrue;
				}
			}
		}
	}

	//DC->Print("slider handle key exit\n");
	return qfalse;
}

/**
 * @brief Item_Action
 * @param[in] item
 */
void Item_Action(itemDef_t *item)
{
	if (item)
	{
		Item_RunScript(item, NULL, item->action);
	}
}

/**
 * @brief Item_CorrectedTextRect
 * @param[in] item
 * @return
 */
rectDef_t *Item_CorrectedTextRect(itemDef_t *item)
{
	static rectDef_t rect;

	Com_Memset(&rect, 0, sizeof(rectDef_t));
	if (item)
	{
		rect = item->textRect;
		if (rect.w != 0.f)
		{
			rect.y -= rect.h;
		}
	}

	return &rect;
}

/**
 * @brief Item_SetTextExtents
 * @param[in,out] item
 * @param[in,out] width
 * @param[in,out] height
 * @param[in] text
 */
void Item_SetTextExtents(itemDef_t *item, int *width, int *height, const char *text)
{
	const char *textPtr = (text) ? text : item->text;

	if (textPtr == NULL)
	{
		return;
	}

	*width  = item->textRect.w;
	*height = item->textRect.h;

	// keeps us from computing the widths and heights more than once
	if (*width == 0 ||
	    (item->type == ITEM_TYPE_OWNERDRAW && item->textalignment == ITEM_ALIGN_CENTER) ||
	    item->textalignment == ITEM_ALIGN_CENTER2 ||
	    item->type == ITEM_TYPE_TIMEOUT_COUNTER)
	{   //int originalWidth = DC->textWidth(item->text, item->textscale, 0);
		int originalWidth = DC->textWidth(textPtr, item->textscale, 0);

		if (item->type == ITEM_TYPE_OWNERDRAW && (item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_RIGHT))
		{
			originalWidth += DC->ownerDrawWidth(item->window.ownerDraw, item->textscale);
		}
		else if (item->type == ITEM_TYPE_EDITFIELD && item->textalignment == ITEM_ALIGN_CENTER && item->cvar)
		{
			char buff[256];

			DC->getCVarString(item->cvar, buff, 256);
			originalWidth += DC->textWidth(buff, item->textscale, 0);
		}
		else if (item->textalignment == ITEM_ALIGN_CENTER2)
		{
			// default centering case
			originalWidth += DC->textWidth(text, item->textscale, 0);
		}

		*width           = DC->textWidth(textPtr, item->textscale, 0);
		*height          = DC->textHeight(textPtr, item->textscale, 0);
		item->textRect.w = *width;
		item->textRect.h = *height;
		item->textRect.x = item->textalignx;
		item->textRect.y = item->textaligny;
		if (item->textalignment == ITEM_ALIGN_RIGHT)
		{
			item->textRect.x = item->textalignx - originalWidth;
		}
		else if (item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_CENTER2)
		{
			// default centering case
			item->textRect.x = item->textalignx - originalWidth / 2;
		}

		ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);
	}
}

/**
 * @brief Item_TextColor
 * @param[in] item
 * @param[in] newColor
 */
void Item_TextColor(itemDef_t *item, vec4_t *newColor)
{
	menuDef_t *parent = (menuDef_t *)item->parent;

	Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount);

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		vec4_t lowLight;

		lowLight[0] = 0.8f * parent->focusColor[0];
		lowLight[1] = 0.8f * parent->focusColor[1];
		lowLight[2] = 0.8f * parent->focusColor[2];
		lowLight[3] = 0.8f * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, *newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else if (item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime / BLINK_DIVISOR) & 1))
	{
		vec4_t lowLight;

		lowLight[0] = 0.8f * item->window.foreColor[0];
		lowLight[1] = 0.8f * item->window.foreColor[1];
		lowLight[2] = 0.8f * item->window.foreColor[2];
		lowLight[3] = 0.8f * item->window.foreColor[3];
		LerpColor(item->window.foreColor, lowLight, *newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Com_Memcpy(newColor, &item->window.foreColor, sizeof(vec4_t));
		// items can be enabled and disabled based on cvars
	}

	if (item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest)
	{
		if ((item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			Com_Memcpy(newColor, &parent->disableColor, sizeof(vec4_t));
		}
	}
}

/**
 * @brief Item_Text_AutoWrapped_Paint
 * @param[in,out] item
 */
void Item_Text_AutoWrapped_Paint(itemDef_t *item)
{
	char       text[1024];
	const char *p, *textPtr, *newLinePtr = NULL;
	char       buff[1024];
	int        width, height, len, textWidth = 0, newLine, newLineWidth, charWidth = 0;
	qboolean   hasWhitespace;
	float      y;
	vec4_t     color;

	if (item->text == NULL)
	{
		if (item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}

	if (*textPtr == '\0')
	{
		return;
	}

	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	y             = item->textaligny;
	len           = 0;
	buff[0]       = '\0';
	newLine       = 0;
	newLineWidth  = 0;
	hasWhitespace = qfalse;
	p             = textPtr;
	while (p)
	{
		charWidth = Q_UTF8_Width(p);
		textWidth = DC->textWidth(buff, item->textscale, 0);
		if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0')
		{
			newLine       = len;
			newLinePtr    = p + 1;
			newLineWidth  = textWidth;
			hasWhitespace = qtrue;
		}
		else if (!hasWhitespace && textWidth > item->window.rect.w)
		{
			newLine      = len;
			newLinePtr   = p;
			newLineWidth = textWidth;
		}

		if ((newLine && textWidth > item->window.rect.w) || *p == '\n' || *p == '\0')
		{
			if (len)
			{
				if (item->textalignment == ITEM_ALIGN_LEFT)
				{
					item->textRect.x = item->textalignx;
				}
				else if (item->textalignment == ITEM_ALIGN_RIGHT)
				{
					item->textRect.x = item->textalignx - newLineWidth;
				}
				else if (item->textalignment == ITEM_ALIGN_CENTER)
				{
					item->textRect.x = item->textalignx - newLineWidth / 2;
				}

				item->textRect.y = y;
				ToWindowCoords(&item->textRect.x, &item->textRect.y, &item->window);

				buff[newLine] = '\0';
				DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, buff, 0, 0, item->textStyle);
			}
			if (*p == '\0')
			{
				break;
			}

			y            += height + 5;
			p             = newLinePtr;
			len           = 0;
			newLine       = 0;
			newLineWidth  = 0;
			hasWhitespace = qfalse;
			continue;
		}

		if (charWidth > 1)
		{
			while (charWidth)
			{
				buff[len++] = *p++;
				charWidth--;
			}
		}
		else
		{
			buff[len++] = *p++;

			if (buff[len - 1] == 13)
			{
				buff[len - 1] = ' ';
			}
		}

		buff[len] = '\0';
	}
}

/**
 * @brief Item_Text_Wrapped_Paint
 * @param[in] item
 */
void Item_Text_Wrapped_Paint(itemDef_t *item)
{
	char       text[1024];
	const char *p, *start, *textPtr;
	char       buff[1024];
	int        width, height;
	float      x, y;
	vec4_t     color;

	// now paint the text and/or any optional images
	// default to left

	if (item->text == NULL)
	{
		if (item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}

	if (*textPtr == '\0')
	{
		return;
	}

	Item_TextColor(item, &color);
	Item_SetTextExtents(item, &width, &height, textPtr);

	x     = item->textRect.x;
	y     = item->textRect.y;
	start = textPtr;
	p     = strchr(textPtr, '\r');
	while (p && *p)
	{
		strncpy(buff, start, p - start + 1);
		buff[p - start] = '\0';
		DC->drawText(x, y, item->textscale, color, buff, 0, 0, item->textStyle);
		y     += height + 5;
		start += p - start + 1;
		p      = strchr(p + 1, '\r');
	}

	DC->drawText(x, y, item->textscale, color, start, 0, 0, item->textStyle);
}

/**
 * @brief Item_HandleKey
 * @param[in] item
 * @param[in] key
 * @param[in] down
 * @return
 */
qboolean Item_HandleKey(itemDef_t *item, int key, qboolean down)
{
	int realKey = key;

	if (realKey & K_CHAR_FLAG)
	{
		realKey &= ~K_CHAR_FLAG;
	}

	if (itemCapture)
	{
		itemCapture = NULL;
		captureFunc = NULL;
		captureData = NULL;
	}
	else
	{
		if (down && (realKey == K_MOUSE1 || realKey == K_MOUSE2 || realKey == K_MOUSE3))
		{
			Item_StartCapture(item, key);
		}
	}

	if (!down)
	{
		return qfalse;
	}

	if (realKey == K_TAB && item->onTab)
	{
		Item_RunScript(item, NULL, item->onTab);
		return qtrue;
	}

	if (realKey == K_ESCAPE && item->onEsc)
	{
		Item_RunScript(item, NULL, item->onEsc);
		return qtrue;
	}

	if (realKey == K_ENTER && item->onEnter)
	{
		Item_RunScript(item, NULL, item->onEnter);
		return qtrue;
	}

	if (K_CLIPBOARD(key) && item->onPaste)
	{
		Item_RunScript(item, NULL, item->onPaste);
		return qtrue;
	}

	switch (item->type)
	{
	case ITEM_TYPE_BUTTON:
		return qfalse;
	case ITEM_TYPE_RADIOBUTTON:
		return qfalse;
	case ITEM_TYPE_CHECKBOX:
	case ITEM_TYPE_TRICHECKBOX:
		return Item_CheckBox_HandleKey(item, key);
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
		//return Item_TextField_HandleKey(item, key);
		return qfalse;
	case ITEM_TYPE_COMBO:
		return qfalse;
	case ITEM_TYPE_LISTBOX:
		return Item_ListBox_HandleKey(item, key, down, qfalse);
	case ITEM_TYPE_YESNO:
		return Item_YesNo_HandleKey(item, key);
	case ITEM_TYPE_MULTI:
		return Item_Multi_HandleKey(item, key);
	case ITEM_TYPE_OWNERDRAW:
		return Item_OwnerDraw_HandleKey(item, key);
	case ITEM_TYPE_BIND:
		return Item_Bind_HandleKey(item, key, down);
	case ITEM_TYPE_SLIDER:
		return Item_Slider_HandleKey(item, key, down);
	default:
		break;
	}
	return qfalse;
}

/**
 * @brief Item_Text_Paint
 * @param[in,out] item
 */
void Item_Text_Paint(itemDef_t *item)
{
	char       text[1024];
	const char *textPtr;
	int        height, width;
	vec4_t     color;
	menuDef_t  *menu = (menuDef_t *)item->parent;

	if (item->window.flags & WINDOW_WRAPPED)
	{
		Item_Text_Wrapped_Paint(item);
		return;
	}

	if (item->window.flags & WINDOW_AUTOWRAPPED)
	{
		Item_Text_AutoWrapped_Paint(item);
		return;
	}

	if (item->text == NULL)
	{
		if (item->cvar == NULL)
		{
			return;
		}
		else
		{
			DC->getCVarString(item->cvar, text, sizeof(text));
			if (item->window.flags & WINDOW_TEXTASINT)
			{
				COM_StripExtension(text, text, sizeof(text));
				item->textRect.w = 0;   // force recalculation
			}
			else if (item->window.flags & WINDOW_TEXTASFLOAT)
			{
				char *s = va("%.2f", atof(text));
				Q_strncpyz(text, s, sizeof(text));
				item->textRect.w = 0;   // force recalculation
			}

			textPtr = text;
		}
	}
	else
	{
		textPtr = item->text;
	}

	// handle counters
	if (item->type == ITEM_TYPE_TIMEOUT_COUNTER && menu != NULL && menu->openTime > 0)
	{
		// calc seconds remaining
		int seconds = (menu->openTime + menu->timeout - DC->realTime + 999) / 1000;

		// build string
		if (seconds <= 2)
		{
			//Com_sprintf( text, 255, "^1%d", seconds );
			Com_sprintf(text, 255, textPtr, va("^1%d^*", seconds));
		}
		else
		{
			//Com_sprintf( text, 255, "%d", seconds );
			Com_sprintf(text, 255, textPtr, va("%d", seconds));
		}

		// set ptr
		textPtr = text;
	}

	// this needs to go here as it sets extents for cvar types as well
	Item_SetTextExtents(item, &width, &height, textPtr);

	if (*textPtr == '\0')
	{
		return;
	}

	Item_TextColor(item, &color);

	//FIXME: this is a fucking mess
	/*
	adjust = 0;
	if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
	adjust = 0.5;
	}

	if (item->textStyle == ITEM_TEXTSTYLE_SHADOWED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
	Fade(&item->window.flags, &DC->Assets.shadowColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, qfalse);
	DC->drawText(item->textRect.x + DC->Assets.shadowX, item->textRect.y + DC->Assets.shadowY, item->textscale, DC->Assets.shadowColor, textPtr, adjust);
	}
	*/

	//  if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
	//      Fade(&item->window.flags, &item->window.outlineColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, qfalse);
	//      /*
	//      Text_Paint(item->textRect.x-1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x+1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x-1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x+1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x-1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
	//      Text_Paint(item->textRect.x+1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
	//      */
	//      DC->drawText(item->textRect.x - 1, item->textRect.y + 1, item->textscale * 1.02, item->window.outlineColor, textPtr, adjust);
	//  }

	DC->drawText(item->textRect.x, item->textRect.y, item->textscale, color, textPtr, 0, 0, item->textStyle);
}

/**
 * @brief Item_TextField_Paint
 * @param[in] item
 */
void Item_TextField_Paint(itemDef_t *item)
{
	char           buff[1024];
	vec4_t         newColor;
	int            offset;
	int            text_len = 0; // screen length of the editfield text that will be printed
	int            field_offset; // character offset in the editfield string
	int            screen_offset; // offset on screen for precise placement
	menuDef_t      *parent  = (menuDef_t *)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;

	Item_Text_Paint(item);

	buff[0] = '\0';

	if (item->cvar)
	{
		if ((item->window.flags & WINDOW_HASFOCUS) && g_editingField)
		{
			DC->getCVarString(EDITFIELD_TEMP_CVAR, buff, sizeof(buff));
		}
		else
		{
			DC->getCVarString(item->cvar, buff, sizeof(buff));
		}
	}

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		vec4_t lowLight;

		lowLight[0] = 0.8f * parent->focusColor[0];
		lowLight[1] = 0.8f * parent->focusColor[1];
		lowLight[2] = 0.8f * parent->focusColor[2];
		lowLight[3] = 0.8f * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Com_Memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	// NOTE: offset from the editfield prefix (like "Say: " in limbo menu)
	offset = (item->text && *item->text) ? 8 : 0;

	// text length control
	// if the edit field goes beyond the available width, drop some characters at the beginning of the string and apply some offseting
	// FIXME: we could cache the text length and offseting, but given the low count of edit fields, I abstained for now
	// FIXME: this won't handle going back into the line of the editfield to the hidden area
	// start of text painting: item->textRect.x + item->textRect.w + offset
	// our window limit: item->window.rect.x + item->window.rect.w
	field_offset = -1;
	do
	{
		field_offset++;
		if (buff[editPtr->paintOffset + field_offset] == '\0')
		{
			break; // keep it safe
		}

		text_len = DC->textWidth(buff + editPtr->paintOffset + field_offset, item->textscale, 0);
	}
	while (text_len + item->textRect.x + item->textRect.w + offset > item->window.rect.x + item->window.rect.w);

	if (field_offset)
	{
		// we had to take out some chars to make it fit in, there is an additional screen offset to compute
		screen_offset = (int)(item->window.rect.x + item->window.rect.w - (text_len + item->textRect.x + item->textRect.w + offset));
	}
	else
	{
		screen_offset = 0;
	}

	if (IS_EDITMODE(item))
	{
		DC->drawTextWithCursor(item->textRect.x + item->textRect.w + offset + screen_offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset + field_offset, item->cursorPos - editPtr->paintOffset - field_offset, (DC->getOverstrikeMode() ? "_" : "|"), editPtr->maxPaintChars, item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x + item->textRect.w + offset + screen_offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset + field_offset, 0, editPtr->maxPaintChars, item->textStyle);
	}
}

/**
 * @brief Item_CheckBox_Paint
 * @param[in] item
 */
void Item_CheckBox_Paint(itemDef_t *item)
{
	vec4_t     newColor;
	float      value        = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;
	menuDef_t  *parent      = (menuDef_t *)item->parent;
	qboolean   hasMultiText = qfalse;
	multiDef_t *multiPtr    = (multiDef_t *)item->typeData;

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		vec4_t lowLight;

		lowLight[0] = 0.8f * parent->focusColor[0];
		lowLight[1] = 0.8f * parent->focusColor[1];
		lowLight[2] = 0.8f * parent->focusColor[2];
		lowLight[3] = 0.8f * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Com_Memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	if (multiPtr && multiPtr->count)
	{
		hasMultiText = qtrue;
	}

	if (item->text)
	{
		Item_Text_Paint(item);
		if (item->type == ITEM_TYPE_TRICHECKBOX && value == 2.f)
		{
			DC->drawHandlePic(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.h, item->window.rect.h, DC->Assets.checkboxCheckNo);
		}
		else if (value != 0.f)
		{
			DC->drawHandlePic(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.h, item->window.rect.h, DC->Assets.checkboxCheck);
		}
		else
		{
			DC->drawHandlePic(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.h, item->window.rect.h, DC->Assets.checkboxCheckNot);
		}

		if (hasMultiText)
		{
			vec4_t colour;

			Item_TextColor(item, &colour);
			DC->drawText(item->textRect.x + item->textRect.w + 8 + item->window.rect.h + 4, item->textRect.y, item->textscale,
			             colour, Item_Multi_Setting(item), 0, 0, item->textStyle);
		}
	}
	else
	{
		if (item->type == ITEM_TYPE_TRICHECKBOX && value == 2.f)
		{
			DC->drawHandlePic(item->window.rect.x, item->window.rect.y, item->window.rect.h, item->window.rect.h, DC->Assets.checkboxCheckNo);
		}
		else if (value != 0.f)
		{
			DC->drawHandlePic(item->window.rect.x, item->window.rect.y, item->window.rect.h, item->window.rect.h, DC->Assets.checkboxCheck);
		}
		else
		{
			DC->drawHandlePic(item->window.rect.x, item->window.rect.y, item->window.rect.h, item->window.rect.h, DC->Assets.checkboxCheckNot);
		}

		if (hasMultiText)
		{
			vec4_t colour;

			Item_TextColor(item, &colour);
			DC->drawText(item->window.rect.x + item->window.rect.h + 4, item->window.rect.y + item->textaligny, item->textscale,
			             colour, Item_Multi_Setting(item), 0, 0, item->textStyle);
		}
	}
}

/**
 * @brief Item_YesNo_Paint
 * @param[in] item
 */
void Item_YesNo_Paint(itemDef_t *item)
{
	vec4_t    newColor;
	menuDef_t *parent = (menuDef_t *)item->parent;
	float     value   = (item->cvar) ? DC->getCVarValue(item->cvar) : 0;

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		vec4_t lowLight;

		lowLight[0] = 0.8f * parent->focusColor[0];
		lowLight[1] = 0.8f * parent->focusColor[1];
		lowLight[2] = 0.8f * parent->focusColor[2];
		lowLight[3] = 0.8f * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Com_Memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	if (item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor,
		             value != 0.f ? DC->translateString(_("Yes")) : DC->translateString(_("No")), 0, 0, item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, value != 0.f ? "Yes" : "No", 0, 0, item->textStyle);
	}
}

/**
 * @brief Item_Multi_Paint
 * @param[in] item
 */
void Item_Multi_Paint(itemDef_t *item)
{
	vec4_t     newColor;
	const char *text   = "";
	menuDef_t  *parent = (menuDef_t *)item->parent;

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		vec4_t lowLight;

		lowLight[0] = 0.8f * parent->focusColor[0];
		lowLight[1] = 0.8f * parent->focusColor[1];
		lowLight[2] = 0.8f * parent->focusColor[2];
		lowLight[3] = 0.8f * parent->focusColor[3];
		LerpColor(parent->focusColor, lowLight, newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Com_Memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
	}

	text = Item_Multi_Setting(item);

	if (item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle);
	}
}

/**
 * @brief Item_Combo_Paint
 * @param[in,out] item
 */
void Item_Combo_Paint(itemDef_t *item)
{
	vec4_t     itemColor, backColor;
	const char *text = Item_Multi_Setting(item);
	int        selectedTextOffset = 0, selectorOffset = 0, temp = 0, widestText = 0, selectorSize = 0;
	//menuDef_t    *parent            = (menuDef_t *)item->parent;
	rectDef_t    rect, selectorRect;
	multiDef_t   *multiPtr;
	char         valueString[MAX_QPATH];
	float        valueFloat = 0;
	int          i;
	static float borderOffset = 4.f;

	Com_Memcpy(&backColor, &item->window.backColor, sizeof(vec4_t));
	Com_Memcpy(&itemColor, &item->window.foreColor, sizeof(vec4_t));

	if (item->text)
	{
		Item_Text_Paint(item);
		selectedTextOffset = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		selectedTextOffset = item->textRect.x;
	}

	multiPtr = (multiDef_t *)item->typeData;
	if (!multiPtr)
	{
		return;
	}

	if (multiPtr->strDef)
	{
		DC->getCVarString(item->cvar, valueString, MAX_QPATH);
	}
	else
	{
		valueFloat = DC->getCVarValue(item->cvar);
	}

	for (i = 0; i < multiPtr->count; i++)
	{
		temp = DC->textWidth(multiPtr->cvarList[i], item->textscale, 0) + borderOffset;
		if (temp > widestText)
		{
			widestText = temp;
		}
	}

	selectorOffset = widestText + selectedTextOffset - 4 + borderOffset;

	selectorSize = DC->textWidth(COMBO_SELECTORCHAR, item->textscale, 0);

	rect.x = selectedTextOffset;
	rect.y = item->textRect.y - item->textRect.h - borderOffset;
	rect.w = widestText + 4 + selectorSize + borderOffset;
	rect.h = item->textRect.h + (borderOffset * 2);

	selectorRect.x = rect.x + (rect.w - selectorSize - 8 - (borderOffset * 2));
	selectorRect.y = rect.y;
	selectorRect.w = selectorSize + 8 + (borderOffset * 2);
	selectorRect.h = rect.h;

	//rect.w = rect.w - selectorRect.w - 1;

	DC->fillRect(rect.x, rect.y, rect.w, rect.h, backColor);
	DC->drawRect(rect.x, rect.y, rect.w, rect.h, item->window.borderSize, item->window.borderColor);

	//DC->fillRect(selectorRect.x, selectorRect.y, selectorRect.w, selectorRect.h, backColor);
	DC->drawRect(selectorRect.x, selectorRect.y, selectorRect.w, selectorRect.h, item->window.borderSize, item->window.borderColor);

	DC->drawText(selectedTextOffset + borderOffset, item->textRect.y, item->textscale, itemColor, text, 0, 0, item->textStyle);
	DC->drawText(selectorOffset, item->textRect.y, item->textscale, itemColor, COMBO_SELECTORCHAR, 0, 0, item->textStyle);

	if (IS_EDITMODE(item))
	{
		float     height = 0;
		vec4_t    lowColor, redishColor;
		vec4_t    *currentColor = NULL;
		rectDef_t textRect      = { selectedTextOffset, 0.f, 0.f, 12.f };

		lowColor[0] = 0.8f * itemColor[0];
		lowColor[1] = 0.8f * itemColor[1];
		lowColor[2] = 0.8f * itemColor[2];
		lowColor[3] = 0.8f * itemColor[3];

		Com_Memcpy(&redishColor, &lowColor, sizeof(vec4_t));
		redishColor[0] = 1.f;

		textRect.w = widestText;

		height = (i * 12.f) + 1.f;

		DC->fillRect(rect.x, item->textRect.y + borderOffset, rect.w, height, backColor);

		item->cursorPos = -1;

		for (i = 0; i < multiPtr->count; i++)
		{
			textRect.y = item->textRect.y + (i * 12.f) + 2.f + borderOffset;

			if (Rect_ContainsPoint(&textRect, DC->cursorx, DC->cursory))
			{
				currentColor    = &itemColor;
				item->cursorPos = i;
			}
			else if ((!multiPtr->strDef && multiPtr->cvarValue[i] == valueFloat) || (multiPtr->strDef && !Q_stricmp(multiPtr->cvarStr[i], valueString)))
			{
				currentColor = &redishColor;
			}
			else
			{
				currentColor = &lowColor;
			}

			DC->drawText(selectedTextOffset + borderOffset, item->textRect.y + (i * 12.f) + 2.f + item->textRect.h + borderOffset, item->textscale, *currentColor, multiPtr->cvarList[i], 0, 0, item->textStyle);
		}

		DC->drawRect(rect.x, item->textRect.y + borderOffset, rect.w, height, item->window.borderSize, item->window.borderColor);
	}
}

/**
 * @brief Item_Slider_Paint
 * @param[in] item
 */
void Item_Slider_Paint(itemDef_t *item)
{
	vec4_t sliderColor;
	float  x, y;
	///menuDef_t *parent = (menuDef_t *)item->parent;

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		vec4_t dimmedColor;

		VectorScale(item->sliderColor, 0.8, dimmedColor);
		LerpColor(item->sliderColor, dimmedColor, sliderColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		Vector4Copy(item->sliderColor, sliderColor);
	}

	y = item->window.rect.y;
	if (item->text)
	{
		Item_Text_Paint(item);
		x = item->textRect.x + item->textRect.w + 8;
	}
	else
	{
		x = item->window.rect.x;
	}

	DC->setColor(sliderColor);
	DC->drawHandlePic(x, y + 1, SLIDER_WIDTH, SLIDER_HEIGHT, DC->Assets.sliderBar);

	x = Item_Slider_ThumbPosition(item);
	DC->drawHandlePic(x - (SLIDER_THUMB_WIDTH / 2), y, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb);
	DC->setColor(NULL);
}

/**
 * @brief Item_Bind_Paint
 * @param[in] item
 */
void Item_Bind_Paint(itemDef_t *item)
{
	vec4_t         newColor, lowLight;
	int            maxChars = 0;
	menuDef_t      *parent  = (menuDef_t *)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;

	if (editPtr)
	{
		maxChars = editPtr->maxPaintChars;
	}

	if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
	{
		if (g_bindItem == item)
		{
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
		}
		else
		{
			lowLight[0] = 0.8f * parent->focusColor[0];
			lowLight[1] = 0.8f * parent->focusColor[1];
			lowLight[2] = 0.8f * parent->focusColor[2];
			lowLight[3] = 0.8f * parent->focusColor[3];
		}

		LerpColor(parent->focusColor, lowLight, newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
	}
	else
	{
		if (g_bindItem == item)
		{
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
			LerpColor(item->window.foreColor, lowLight, newColor, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
		}
		else
		{
			Com_Memcpy(&newColor, &item->window.foreColor, sizeof(vec4_t));
		}
	}

	if (item->text)
	{
		Item_Text_Paint(item);
		DC->drawText(item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, Binding_FromName(item->cvar), 0, maxChars, item->textStyle);
	}
	else
	{
		DC->drawText(item->textRect.x, item->textRect.y, item->textscale, newColor, "FIXME", 0, maxChars, item->textStyle);
	}
}

/**
 * @brief Item_Bind_HandleKey
 * @param[in] item
 * @param[in] key
 * @param[in] down
 * @return
 */
qboolean Item_Bind_HandleKey(itemDef_t *item, int key, qboolean down)
{
	int id;

	if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory) && !g_waitingForKey)
	{
		if (down && (key == K_MOUSE1 || key == K_ENTER))
		{
			g_waitingForKey = qtrue;
			g_bindItem      = item;
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{
		if (!g_waitingForKey || g_bindItem == NULL)
		{
			return qfalse;
		}

		if (key & K_CHAR_FLAG)
		{
			return qtrue;
		}

		switch (key)
		{
		case K_ESCAPE:
			g_waitingForKey = qfalse;
			g_bindItem      = NULL;
			return qtrue;

		case K_BACKSPACE:
			id = Binding_IDFromName(item->cvar);
			Binding_Set(id, -1, -1);
			Controls_SetConfig(qtrue);
			g_waitingForKey = qfalse;
			g_bindItem      = NULL;
			return qtrue;

		case '`':
			return qtrue;
		}
	}

	if (key != -1)
	{
		int i;

		for (i = 0; i < Binding_Count(); i++)
		{
			if (Binding_Check(i, qfalse, key))
			{
				Binding_Set(i, -2, -1);
			}

			if (Binding_Check(i, qtrue, key))
			{
				Binding_Set(i, Binding_Get(i, qfalse), -1);
			}
		}
	}

	id = Binding_IDFromName(item->cvar);

	if (id != -1)
	{
		int binding1;
		int binding2;

		binding1 = Binding_Get(id, qtrue);
		binding2 = Binding_Get(id, qfalse);

		if (key == -1)
		{
			if (binding1 != -1)
			{
				DC->setBinding(binding1, "");
				Binding_Set(id, -1, -2);
			}

			if (binding2 != -1)
			{
				DC->setBinding(binding2, "");
				Binding_Set(id, -2, -1);
			}
		}
		else if (binding1 == -1)
		{
			Binding_Set(id, key, -2);
		}
		else if (binding1 != key && binding2 == -1)
		{
			Binding_Set(id, -2, key);
		}
		else
		{
			DC->setBinding(binding1, "");
			DC->setBinding(binding2, "");
			Binding_Set(id, key, -1);
		}
	}

	Controls_SetConfig(qtrue);
	g_waitingForKey = qfalse;
	g_bindItem      = NULL;

	return qtrue;
}

/**
 * @brief Item_Model_Paint
 * @param[in] item
 */
void Item_Model_Paint(itemDef_t *item)
{
	float       x, y, w, h; //,xx;
	refdef_t    refdef;
	qhandle_t   hModel;
	refEntity_t ent;
	vec3_t      mins, maxs, origin;
	vec3_t      angles;
	modelDef_t  *modelPtr = (modelDef_t *)item->typeData;

	if (modelPtr == NULL)
	{
		return;
	}

	if (!item->asset)
	{
		return;
	}

	hModel = item->asset;

	// setup the refdef
	Com_Memset(&refdef, 0, sizeof(refdef));
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear(refdef.viewaxis);
	x = item->window.rect.x + 1;
	y = item->window.rect.y + 1;
	w = item->window.rect.w - 2;
	h = item->window.rect.h - 2;

	AdjustFrom640(&x, &y, &w, &h);

	refdef.x      = x;
	refdef.y      = y;
	refdef.width  = w;
	refdef.height = h;

	DC->modelBounds(hModel, mins, maxs);

	origin[2] = -0.5f * (mins[2] + maxs[2]);
	origin[1] = 0.5f * (mins[1] + maxs[1]);

	// calculate distance so the model nearly fills the box
	//if (qtrue)
	//{
	float len = 0.5f * (maxs[2] - mins[2]);

	origin[0] = len / 0.268f;        // len / tan( fov/2 )
	//origin[0] = len / tan(w/2);
	//}
	//else
	//{
	//	origin[0] = item->textscale;
	//}

#define NEWWAY
#ifdef NEWWAY
	refdef.fov_x = (modelPtr->fov_x != 0.f) ? modelPtr->fov_x : w;
	refdef.fov_y = (modelPtr->fov_y != 0.f) ? modelPtr->fov_y : h;
#else
	refdef.fov_x  = (int)((float)refdef.width / 640.0f * 90.0f);
	xx            = refdef.width / tan(refdef.fov_x / 360 * M_PI);
	refdef.fov_y  = atan2(refdef.height, xx);
	refdef.fov_y *= (360 / M_PI);
#endif
	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	Com_Memset(&ent, 0, sizeof(ent));

	//adjust = 5.0 * sin( (float)uis.realtime / 500 );
	//adjust = 360 % (int)((float)uis.realtime / 1000);
	//VectorSet( angles, 0, 0, 1 );

	// use item storage to track
	if (modelPtr->rotationSpeed)
	{
		if (DC->realTime > item->window.nextTime)
		{
			item->window.nextTime = DC->realTime + modelPtr->rotationSpeed;
			modelPtr->angle       = (modelPtr->angle + 1) % 360;
		}
	}

	VectorSet(angles, 0, modelPtr->angle, 0);
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;

	if (modelPtr->frameTime)     // don't advance on the first frame
	{
		modelPtr->backlerp += (((DC->realTime - modelPtr->frameTime) / 1000.0f) * (float)modelPtr->fps);
	}

	if (modelPtr->backlerp > 1)
	{
		int backLerpWhole = (int)(floor((double)modelPtr->backlerp));

		modelPtr->frame += (backLerpWhole);
		if ((modelPtr->frame - modelPtr->startframe) > modelPtr->numframes)
		{
			modelPtr->frame = modelPtr->startframe + modelPtr->frame % modelPtr->numframes; // todo: ignoring loopframes
		}

		modelPtr->oldframe += (backLerpWhole);
		if ((modelPtr->oldframe - modelPtr->startframe) > modelPtr->numframes)
		{
			modelPtr->oldframe = modelPtr->startframe + modelPtr->oldframe % modelPtr->numframes;   // todo: ignoring loopframes
		}

		modelPtr->backlerp = modelPtr->backlerp - backLerpWhole;
	}

	modelPtr->frameTime = DC->realTime;

	ent.frame    = modelPtr->frame;
	ent.oldframe = modelPtr->oldframe;
	ent.backlerp = 1.0f - modelPtr->backlerp;

	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.lightingOrigin);
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy(ent.origin, ent.oldorigin);

	DC->addRefEntityToScene(&ent);
	DC->renderScene(&refdef);
}

/**
 * @brief Item_ListBox_Paint
 * @param[in] item
 */
void Item_ListBox_Paint(itemDef_t *item)
{
	int          i;
	float        x, y, size, count, thumb;
	qhandle_t    image;
	qhandle_t    optionalImages[8];
	int          numOptionalImages;
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	rectDef_t    fillRect = item->window.rect;

	/*if( item->window.borderSize ) {
	fillRect.x += item->window.borderSize;
	fillRect.y += item->window.borderSize;
	fillRect.w -= 2 * item->window.borderSize;
	fillRect.h -= 2 * item->window.borderSize;
	}*/

	// the listbox is horizontal or vertical and has a fixed size scroll bar going either direction
	// elements are enumerated from the DC and either text or image handles are acquired from the DC as well
	// textscale is used to size the text, textalignx and textaligny are used to size image elements
	// there is no clipping available so only the last completely visible item is painted
	count = DC->feederCount(item->special);
	// default is vertical if horizontal flag is not here
	if (item->window.flags & WINDOW_HORIZONTAL)
	{
		// draw scrollbar in bottom of the window
		// bar
		x = fillRect.x + 1;
		y = fillRect.y + fillRect.h - SCROLLBAR_SIZE - 1;
		DC->setColor(item->scrollColor);
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowLeft);
		x   += SCROLLBAR_SIZE - 1;
		size = fillRect.w - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, size + 1, SCROLLBAR_SIZE, DC->Assets.scrollBar);
		x += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowRight);
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition(item);   //Item_ListBox_ThumbPosition(item);
		if (thumb > x - SCROLLBAR_SIZE - 1)
		{
			thumb = x - SCROLLBAR_SIZE - 1;
		}

		DC->drawHandlePic(thumb, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);
		DC->setColor(NULL);

		listPtr->endPos = listPtr->startPos;
		size            = fillRect.w - 2;

		// items
		// size contains max available space
		if (listPtr->elementStyle == LISTBOX_IMAGE)
		{
			x = fillRect.x + 1;
			y = fillRect.y + 1;
			for (i = listPtr->startPos; i < count; i++)
			{
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if (image)
				{
					DC->drawHandlePic(x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image);
				}

				if (i == item->cursorPos)
				{
					DC->drawRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize, item->window.borderColor);
				}

				size -= listPtr->elementWidth;
				if (size < listPtr->elementWidth)
				{
					listPtr->drawPadding = size; //listPtr->elementWidth - size;
					break;
				}

				x += listPtr->elementWidth;
				listPtr->endPos++;
			}
		}
	}
	else
	{
		// draw scrollbar to right side of the window
		x = fillRect.x + fillRect.w - SCROLLBAR_SIZE - 1;
		y = fillRect.y + 1;
		DC->setColor(item->scrollColor);
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowUp);
		y += SCROLLBAR_SIZE - 1;

		listPtr->endPos = listPtr->startPos;
		size            = fillRect.h - (SCROLLBAR_SIZE * 2);
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, size + 1, DC->Assets.scrollBar);
		y += size - 1;
		DC->drawHandlePic(x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowDown);
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition(item);
		if (thumb > y - SCROLLBAR_SIZE - 1)
		{
			thumb = y - SCROLLBAR_SIZE - 1;
		}

		DC->drawHandlePic(x, thumb, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb);
		DC->setColor(NULL);

		// adjust size for item painting
		size = fillRect.h /* - 2*/;
		if (listPtr->elementStyle == LISTBOX_IMAGE)
		{
			// fit = 0;
			x = fillRect.x + 1;
			y = fillRect.y + 1;
			for (i = listPtr->startPos; i < count; i++)
			{
				if (i == item->cursorPos)
				{
					DC->fillRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.outlineColor);
				}

				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage(item->special, i);
				if (image)
				{
					DC->drawHandlePic(x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image);
				}

				if (i == item->cursorPos)
				{
					DC->drawRect(x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize, item->window.borderColor);
				}

				listPtr->endPos++;
				size -= listPtr->elementHeight;
				if (size < listPtr->elementHeight)
				{
					listPtr->drawPadding = size; //listPtr->elementHeight - size;
					break;
				}

				y += listPtr->elementHeight;
			}
		}
		// LISTBOX_TEXT
		else
		{
			x = fillRect.x /*+ 1*/;
			y = fillRect.y /*+ 1*/;
			for (i = listPtr->startPos; i < count; i++)
			{
				const char *text;
				// always draw at least one
				// which may overdraw the box if it is too small for the element

				if (listPtr->numColumns > 0)
				{
					int j, k;

					for (j = 0; j < listPtr->numColumns; j++)
					{
						text = DC->feederItemText(item->special, i, j, optionalImages, &numOptionalImages);
						if (numOptionalImages > 0)
						{
							for (k = 0; k < numOptionalImages; k++)
							{
								if (optionalImages[k] >= 0)
								{
									DC->drawHandlePic(x + listPtr->columnInfo[j].pos + k * listPtr->elementHeight + 1,
									                  y + 1, listPtr->elementHeight - 2, listPtr->elementHeight - 2, optionalImages[k]);
								}
							}

							//DC->drawHandlePic( x + 4 + listPtr->columnInfo[j].pos, y - 1 + listPtr->elementHeight / 2, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
						}
						else if (text)
						{
							DC->drawText(x + 4 + listPtr->columnInfo[j].pos + item->textalignx,
							             y + listPtr->elementHeight + item->textaligny, item->textscale, item->window.foreColor, text, 0, listPtr->columnInfo[j].maxChars, item->textStyle);
						}
					}
				}
				else
				{
					text = DC->feederItemText(item->special, i, 0, optionalImages, &numOptionalImages);
					if (numOptionalImages >= 0)
					{
						//DC->drawHandlePic(x + 4 + listPtr->elementHeight, y, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
					}
					else if (text)
					{
						DC->drawText(x + 4 + item->textalignx, y + listPtr->elementHeight + item->textaligny, item->textscale, item->window.foreColor, text, 0, 0, item->textStyle);
					}
				}

				if (i == item->cursorPos)
				{
					DC->fillRect(x, y, fillRect.w - SCROLLBAR_SIZE - 2, listPtr->elementHeight /* - 1*/, item->window.outlineColor);
				}

				size -= listPtr->elementHeight;
				if (size < listPtr->elementHeight)
				{
					listPtr->drawPadding = size; //listPtr->elementHeight - size;
					break;
				}

				listPtr->endPos++;
				y += listPtr->elementHeight;
			}
		}
	}
}

/**
 * @brief Item_OwnerDraw_Paint
 * @param[in] item
 */
void Item_OwnerDraw_Paint(itemDef_t *item)
{
	if (item == NULL)
	{
		return;
	}

	if (DC->ownerDrawItem)
	{
		vec4_t    color, lowLight;
		menuDef_t *parent = (menuDef_t *)item->parent;

		Fade(&item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount);
		Com_Memcpy(&color, &item->window.foreColor, sizeof(color));
		if (item->numColors > 0 && DC->getValue)
		{
			// if the value is within one of the ranges then set color to that, otherwise leave at default
			int   i;
			float f = DC->getValue(item->window.ownerDraw, item->colorRangeType);

			for (i = 0; i < item->numColors; i++)
			{
				if (f >= item->colorRanges[i].low && f <= item->colorRanges[i].high)
				{
					Com_Memcpy(&color, &item->colorRanges[i].color, sizeof(color));
					break;
				}
			}
		}

		if ((item->window.flags & WINDOW_HASFOCUS) && (item->window.flags & WINDOW_FOCUSPULSE))
		{
			lowLight[0] = 0.8f * parent->focusColor[0];
			lowLight[1] = 0.8f * parent->focusColor[1];
			lowLight[2] = 0.8f * parent->focusColor[2];
			lowLight[3] = 0.8f * parent->focusColor[3];
			LerpColor(parent->focusColor, lowLight, color, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
		}
		else if (item->textStyle == ITEM_TEXTSTYLE_BLINK && !((DC->realTime / BLINK_DIVISOR) & 1))
		{
			lowLight[0] = 0.8f * item->window.foreColor[0];
			lowLight[1] = 0.8f * item->window.foreColor[1];
			lowLight[2] = 0.8f * item->window.foreColor[2];
			lowLight[3] = 0.8f * item->window.foreColor[3];
			LerpColor(item->window.foreColor, lowLight, color, 0.5f + 0.5f * (float)(sin(DC->realTime / PULSE_DIVISOR)));
		}

		if ((item->cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) && !Item_EnableShowViaCvar(item, CVAR_ENABLE))
		{
			Com_Memcpy(color, parent->disableColor, sizeof(vec4_t));
		}

		// gah wtf indentation!
		if (item->text)
		{
			Item_Text_Paint(item);
			if (item->text[0])
			{
				// +8 is an offset kludge to properly align owner draw items that have text combined with them
				DC->ownerDrawItem(item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle);
			}
			else
			{
				DC->ownerDrawItem(item->textRect.x + item->textRect.w, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle);
			}
		}
		else
		{
			DC->ownerDrawItem(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, item->textalignx, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle);
		}
	}
}

/**
 * @brief Item_DoTransition
 * @param[in,out] item
 */
void Item_DoTransition(itemDef_t *item)
{
	if (DC->realTime > item->window.nextTime)
	{
		int done = 0;

		item->window.nextTime = DC->realTime + item->window.offsetTime;
		// transition the x,y
		if (item->window.rectClient.x == item->window.rectEffects.x)
		{
			done++;
		}
		else
		{
			if (item->window.rectClient.x < item->window.rectEffects.x)
			{
				item->window.rectClient.x += item->window.rectEffects2.x;
				if (item->window.rectClient.x > item->window.rectEffects.x)
				{
					item->window.rectClient.x = item->window.rectEffects.x;
					done++;
				}
			}
			else
			{
				item->window.rectClient.x -= item->window.rectEffects2.x;
				if (item->window.rectClient.x < item->window.rectEffects.x)
				{
					item->window.rectClient.x = item->window.rectEffects.x;
					done++;
				}
			}
		}

		if (item->window.rectClient.y == item->window.rectEffects.y)
		{
			done++;
		}
		else
		{
			if (item->window.rectClient.y < item->window.rectEffects.y)
			{
				item->window.rectClient.y += item->window.rectEffects2.y;
				if (item->window.rectClient.y > item->window.rectEffects.y)
				{
					item->window.rectClient.y = item->window.rectEffects.y;
					done++;
				}
			}
			else
			{
				item->window.rectClient.y -= item->window.rectEffects2.y;
				if (item->window.rectClient.y < item->window.rectEffects.y)
				{
					item->window.rectClient.y = item->window.rectEffects.y;
					done++;
				}
			}
		}

		if (item->window.rectClient.w == item->window.rectEffects.w)
		{
			done++;
		}
		else
		{
			if (item->window.rectClient.w < item->window.rectEffects.w)
			{
				item->window.rectClient.w += item->window.rectEffects2.w;
				if (item->window.rectClient.w > item->window.rectEffects.w)
				{
					item->window.rectClient.w = item->window.rectEffects.w;
					done++;
				}
			}
			else
			{
				item->window.rectClient.w -= item->window.rectEffects2.w;
				if (item->window.rectClient.w < item->window.rectEffects.w)
				{
					item->window.rectClient.w = item->window.rectEffects.w;
					done++;
				}
			}
		}

		if (item->window.rectClient.h == item->window.rectEffects.h)
		{
			done++;
		}
		else
		{
			if (item->window.rectClient.h < item->window.rectEffects.h)
			{
				item->window.rectClient.h += item->window.rectEffects2.h;
				if (item->window.rectClient.h > item->window.rectEffects.h)
				{
					item->window.rectClient.h = item->window.rectEffects.h;
					done++;
				}
			}
			else
			{
				item->window.rectClient.h -= item->window.rectEffects2.h;
				if (item->window.rectClient.h < item->window.rectEffects.h)
				{
					item->window.rectClient.h = item->window.rectEffects.h;
					done++;
				}
			}
		}

		Item_UpdatePosition(item);

		if (done == 4)
		{
			item->window.flags &= ~WINDOW_INTRANSITION;
		}
	}
}

/**
 * @brief Item_Paint
 * @param item
 */
void Item_Paint(itemDef_t *item)
{
	menuDef_t *parent = NULL;

	if (item == NULL)
	{
		return;
	}

	parent = (menuDef_t *)item->parent;

	if (DC->textFont)
	{
		DC->textFont(item->font);
	}

	if (item->window.flags & WINDOW_ORBITING)
	{
		if (DC->realTime > item->window.nextTime)
		{
			float  rx, ry, c, s, w, h;
			double a;

			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// translate
			w                         = item->window.rectClient.w / 2;
			h                         = item->window.rectClient.h / 2;
			rx                        = item->window.rectClient.x + w - item->window.rectEffects.x;
			ry                        = item->window.rectClient.y + h - item->window.rectEffects.y;
			a                         = 3 * M_PI / 180;
			c                         = (float)cos(a);
			s                         = (float)sin(a);
			item->window.rectClient.x = (rx * c - ry * s) + item->window.rectEffects.x - w;
			item->window.rectClient.y = (rx * s + ry * c) + item->window.rectEffects.y - h;
			Item_UpdatePosition(item);
		}
	}

	if (item->window.flags & WINDOW_INTRANSITION)
	{
		Item_DoTransition(item);
	}

	if (item->window.ownerDrawFlags && DC->ownerDrawVisible)
	{
		if (!DC->ownerDrawVisible(item->window.ownerDrawFlags))
		{
			item->window.flags &= ~(WINDOW_VISIBLE | WINDOW_MOUSEOVER);
		}
		else
		{
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	if (item->cvarFlags & (CVAR_SHOW | CVAR_HIDE))
	{
		if (!Item_EnableShowViaCvar(item, CVAR_SHOW))
		{
			return;
		}
	}

	if ((item->settingFlags & (SVS_ENABLED_SHOW | SVS_DISABLED_SHOW)) && !Item_SettingShow(item, qfalse))
	{
		return;
	}

	if (item->voteFlag != 0 && !Item_SettingShow(item, qtrue))
	{
		return;
	}

	if (item->window.flags & WINDOW_TIMEDVISIBLE)
	{
		//TODO: should we do something here?
	}

	if (!(item->window.flags & WINDOW_VISIBLE))
	{
		return;
	}

	// paint the rect first..
	Window_Paint(&item->window, parent->fadeAmount, parent->fadeClamp, parent->fadeCycle);

	if (debugMode)
	{
		vec4_t    color;
		rectDef_t *r = Item_CorrectedTextRect(item);

		color[1] = color[3] = 1;
		color[0] = color[2] = 0;
		DC->drawRect(r->x, r->y, r->w, r->h, 1, color);
	}

	switch (item->type)
	{
	case ITEM_TYPE_OWNERDRAW:
		Item_OwnerDraw_Paint(item);
		break;
	case ITEM_TYPE_TEXT:
	case ITEM_TYPE_BUTTON:
	case ITEM_TYPE_TIMEOUT_COUNTER:
		Item_Text_Paint(item);
		break;
	case ITEM_TYPE_RADIOBUTTON:
		break;
	case ITEM_TYPE_CHECKBOX:
	case ITEM_TYPE_TRICHECKBOX:
		Item_CheckBox_Paint(item);
		break;
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
		Item_TextField_Paint(item);
		break;
	case ITEM_TYPE_COMBO:
		Item_Combo_Paint(item);
		break;
	case ITEM_TYPE_LISTBOX:
		Item_ListBox_Paint(item);
		break;
	case ITEM_TYPE_MENUMODEL:
		Item_Model_Paint(item);
		break;
	case ITEM_TYPE_MODEL:
		Item_Model_Paint(item);
		break;
	case ITEM_TYPE_YESNO:
		Item_YesNo_Paint(item);
		break;
	case ITEM_TYPE_MULTI:
		Item_Multi_Paint(item);
		break;
	case ITEM_TYPE_BIND:
		Item_Bind_Paint(item);
		break;
	case ITEM_TYPE_SLIDER:
		Item_Slider_Paint(item);
		break;
	default:
		break;
	}
}

/**
 * @brief This gets called on keyboard Enter and KP_Enter
 * @param[in] item
 */
void Item_KeyboardActivate(itemDef_t *item)
{
	if (!item)
	{
		return;
	}

	if (TEXTFIELD(item->type))
	{
		Item_HandleTextFieldSelect(item);
	}
	else
	{
		Item_Action(item);
	}
}

/**
 * @brief This gets called on mouse1-3
 * @param[in] item
 */
void Item_MouseActivate(itemDef_t *item)
{
	if (!item)
	{
		return;
	}

	if (item->type == ITEM_TYPE_TEXT)
	{
		if (Rect_ContainsPoint(Item_CorrectedTextRect(item), DC->cursorx, DC->cursory))
		{
			Item_Action(item);
		}
	}
	else if (TEXTFIELD(item->type))
	{
		if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
		{
			editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;

			// FIXME: make it set the insertion point correctly

			// reset scroll offset so we can see what we're editing
			if (editPtr)
			{
				editPtr->paintOffset = 0;
			}

			Item_HandleTextFieldSelect(item);

			// see elsewhere for venomous comment about this particular piece of "functionality"
			//DC->setOverstrikeMode(qtrue);
		}
	}
	else if (item->type == ITEM_TYPE_COMBO)
	{
		if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
		{
			Item_Action(item);
			Item_ComboSelect(item);
		}
	}
	else
	{
		if (Rect_ContainsPoint(&item->window.rect, DC->cursorx, DC->cursory))
		{
			Item_Action(item);
		}
	}
}

/**
 * @brief Item_Init
 * @param[in,out] item
 */
void Item_Init(itemDef_t *item)
{
	Com_Memset(item, 0, sizeof(itemDef_t));
	item->textscale = 0.55f;

	// default hotkey to -1
	item->hotkey = -1;

	Vector4Set(item->scrollColor, 1, 1, 1, 1);
	Vector4Set(item->sliderColor, 1, 1, 1, 1);

	Window_Init(&item->window);
}
