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
 * @file cg_spawnpoints.c
 */

#include "cg_local.h"

panel_button_text_t spawnpointsTitleFont =
{
	0.19f,                    0.19f,
	{ 0.6f,                   0.6f, 0.6f,  1.f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

panel_button_text_t spawnpointsFont =
{
	0.2f,                    0.2f,
	{ 0.6f,                  0.6f,0.6f,    1.f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

panel_button_t spawnpointsTopBorder =
{
	NULL,
	NULL,
	{ 10,                     129,      204,     136 },
	{ 1,                      255 / 2,  255 / 2, 255 / 2, 255 / 2, 1, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t spawnpointsTopBorderBack =
{
	"white",
	NULL,
	{ 11,                     130,202, 134 },
	{ 1,                      0,  0,   0, (int)(255 * 0.75), 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t spawnpointsTopBorderInner =
{
	"white",
	NULL,
	{ 12,                     131, 200, 12 },
	{ 1,                      41,  51,  43, 204, 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t spawnpointsTopBorderInnerText =
{
	NULL,
	NULL,
	{ 15,                           141,  200, 12 },
	{ 0,                            0,    0,   0, 0, 0, 0, 0},
	&spawnpointsTitleFont,          // font
	NULL,                           // keyDown
	NULL,                           // keyUp
	CG_Spawnpoints_MenuTitleText_Draw,
	NULL,
	0
};

panel_button_t spawnpointsMenuItemText =
{
	NULL,
	NULL,
	{ 16,                      153,  128, 12 },
	{ 0,                       0,    0,   0, 0, 0, 0, 0},
	&spawnpointsFont,          // font
	NULL,                      // keyDown
	NULL,                      // keyUp
	CG_Spawnpoints_MenuText_Draw,
	NULL,
	0
};

panel_button_t *spawnpointsButtons[] =
{
	&spawnpointsTopBorderBack, &spawnpointsTopBorder, &spawnpointsTopBorderInner, &spawnpointsTopBorderInnerText,

	&spawnpointsMenuItemText,

	NULL
};

void CG_Spawnpoints_MenuTitleText_Draw(panel_button_t *button)
{
	CG_Text_Paint_Ext(button->rect.x, button->rect.y + button->data[0], button->font->scalex, button->font->scaley, button->font->colour, CG_TranslateString("CHOOSE SPAWNPOINT"), 0, 0, button->font->style, button->font->font);
}

/**
 * @brief CG_Spawnpoints_MenuText_Draw
 * @param[in] button
 */
void CG_Spawnpoints_MenuText_Draw(panel_button_t *button)
{
	const char *str;
	float      y = button->rect.y;
	int        i;

	// autospawn
	str = va("%i. %s", 0, cg.spawnPoints[0]);
	CG_Text_Paint_Ext(button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0, button->font->style, button->font->font);
	y += button->rect.h;

	for (i = 1; i < cg.spawnCount; i++)
	{
		// hide invalid
		if ((cg.spawnTeams[i] & 0xF) == 0)
		{
			continue;
		}

		// hide inactive
		if (cg.spawnTeams[i] & 256)
		{
			continue;
		}

		// hide enemy spawn points
		if ((cg.spawnTeams[i] & 0xF) != cgs.clientinfo[cg.clientNum].team)
		{
			continue;
		}

		str = va("%i. %s", i % 10, cg.spawnPoints[i]);
		CG_Text_Paint_Ext(button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0, button->font->style, button->font->font);
		y += button->rect.h;
	}
}

/**
 * @brief CG_Spawnpoints_Setup
 */
void CG_Spawnpoints_Setup(void)
{
	BG_PanelButtonsSetup(spawnpointsButtons);
}

/**
 * @brief CG_Spawnpoints_KeyHandling
 * @param[in] key
 * @param[in] down
 */
void CG_Spawnpoints_KeyHandling(int key, qboolean down)
{
	if (down)
	{
		CG_SpawnpointsCheckExecKey(key, qtrue);
	}
}

/**
 * @brief CG_Spawnpoints_Draw
 */
void CG_Spawnpoints_Draw(void)
{
	BG_PanelButtonsRender(spawnpointsButtons);
}

/**
 * @brief CG_SpawnpointsCheckExecKey
 * @param[in] key
 * @param[in] doaction
 * @return
 */
qboolean CG_SpawnpointsCheckExecKey(int key, qboolean doaction)
{
	if (key == K_ESCAPE)
	{
		return qtrue;
	}

	if ((key & K_CHAR_FLAG))
	{
		return qfalse;
	}

	key &= ~K_CHAR_FLAG;

	if (key >= '0' && key <= '9')
	{
		int i = (key - '0') % 10;

		if (i > cg.spawnCount)
		{
			return qfalse;
		}

		if (doaction)
		{
			trap_SendClientCommand(va("setspawnpt %i", i));
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);

			if (i == 0)
			{
				CG_PriorityCenterPrint(CG_TranslateString("Your spawn point will be auto-picked."), 400, cg_fontScaleCP.value, -1);
			}
			else
			{
				CG_PriorityCenterPrint(va(CG_TranslateString("You will spawn at %s."), cg.spawnPoints[i]), 400, cg_fontScaleCP.value, -1);
			}
		}

		return qtrue;
	}

	return qfalse;
}
