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
 * @file cg_local.h
 */

#include "cg_local.h"

extern displayContextDef_t *DC;

qboolean  bg_loadscreeninited = qfalse;
qhandle_t bg_axispin;
qhandle_t bg_alliedpin;
qhandle_t bg_neutralpin;
qhandle_t bg_pin;

qhandle_t bg_filter_bo;
qhandle_t bg_filter_ff;
qhandle_t bg_filter_hw;
qhandle_t bg_filter_lv;
qhandle_t bg_filter_al;
qhandle_t bg_filter_bt;

qhandle_t bg_mappic;

// panel_button_text_t FONTNAME = { SCALEX, SCALEY, COLOUR, STYLE, FONT };

panel_button_text_t missiondescriptionTxt =
{
	0.2f,                          0.2f,
	{ 0.0f,                        0.0f,0.0f,    1.f },
	0,                             0,
	&cgs.media.bg_loadscreenfont2,
};

panel_button_text_t missiondescriptionHeaderTxt =
{
	0.2f,                          0.2f,
	{ 0.0f,                        0.0f,             0.0f,    0.8f },
	0,                             ITEM_ALIGN_CENTER,
	&cgs.media.bg_loadscreenfont2,
};

panel_button_text_t campaignpheaderTxt =
{
	0.2f,                          0.2f,
	{ 1.0f,                        1.0f,1.0f,    0.6f },
	0,                             0,
	&cgs.media.bg_loadscreenfont2,
};

panel_button_text_t campaignpTxt =
{
	0.22f,                         0.22f,
	{ 1.0f,                        1.0f, 1.0f,  0.6f },
	0,                             0,
	&cgs.media.bg_loadscreenfont2,
};

panel_button_text_t loadScreenMeterBackTxt =
{
	0.22f,                         0.22f,
	{ 0.1f,                        0.1f,             0.1f,  0.8f },
	0,                             ITEM_ALIGN_CENTER,
	&cgs.media.bg_loadscreenfont2,
};

panel_button_t loadScreenMap =
{
	"gfx/loading/camp_map",
	NULL,
	{ 0,                      0,  440, 480 }, // shouldn't this be square?? // no, the map is actually WIDER that tall, which makes it even worse...
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t loadScreenBack =
{
	"gfx/loading/camp_side",
	NULL,
	{ 440,                    0,  200, 480 },
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t loadScreenPins =
{
	NULL,
	NULL,
	{ 0,                            0,  SCREEN_WIDTH, SCREEN_HEIGHT },
	{ 0,                            0,  0,            0, 0, 0, 0, 0 },
	NULL,                           // font
	NULL,                           // keyDown
	NULL,                           // keyUp
	CG_LoadPanel_RenderCampaignPins,
	NULL,
	0
};

panel_button_t missiondescriptionPanelHeaderText =
{
	NULL,
	"***TOP SECRET***",
	{ 440,                     72, 200, 32 },
	{ 0,                       0,  0,   0, 0, 0, 0, 0},
	&missiondescriptionHeaderTxt,// font
	NULL,                      // keyDown
	NULL,                      // keyUp
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

panel_button_t missiondescriptionPanelText =
{
	NULL,
	NULL,
	{ 460,                                    84,   160, 232 },
	{ 0,                                      0,    0,   0, 0, 0, 0, 0},
	&missiondescriptionTxt,                   // font
	NULL,                                     // keyDown
	NULL,                                     // keyUp
	CG_LoadPanel_RenderMissionDescriptionText,
	NULL,
	0
};

panel_button_t campaignheaderPanelText =
{
	NULL,
	NULL,
	{ 456,                              24,   152, 232 },
	{ 0,                                0,    0,   0, 0, 0, 0, 0},
	&campaignpheaderTxt,                // font
	NULL,                               // keyDown
	NULL,                               // keyUp
	CG_LoadPanel_RenderCampaignTypeText,
	NULL,
	0
};

panel_button_t campaignPanelText =
{
	NULL,
	NULL,
	{ 464,                              40,   152, 232 },
	{ 0,                                0,    0,   0, 0, 0, 0, 0},
	&campaignpTxt,                      // font
	NULL,                               // keyDown
	NULL,                               // keyUp
	CG_LoadPanel_RenderCampaignNameText,
	NULL,
	0
};

panel_button_t loadScreenMeterBack =
{
	"gfx/loading/progressbar_back",
	NULL,
	{ 440 + 26,                    480 - 30 + 1,200 - 56, 20 },
	{ 0,                           0,  0,        0, 0, 0, 0, 0},
	NULL,                          // font
	NULL,                          // keyDown
	NULL,                          // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t loadScreenMeterBack2 =
{
	"gfx/loading/progressbar",
	NULL,
	{ 440 + 26,                   480 - 30 + 1,200 - 56, 20 },
	{ 1,                          255,  0,        0, 255, 0, 0, 0},
	NULL,                         // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	CG_LoadPanel_RenderLoadingBar,
	NULL,
	0
};

panel_button_t loadScreenMeterBackText =
{
	NULL,
	"LOADING",
	{ 440 + 28,                 480 - 28 + 12 + 1,   200 - 56 - 2, 16 },
	{ 0,                        0,                   0,            0, 0, 0, 0, 0},
	&loadScreenMeterBackTxt,    // font
	NULL,                       // keyDown
	NULL,                       // keyUp
	CG_LoadPanel_LoadingBarText,
	NULL,
	0
};

panel_button_t *loadpanelButtons[] =
{
	&loadScreenMap,               &loadScreenBack,
	&missiondescriptionPanelText, &missiondescriptionPanelHeaderText,
	&campaignheaderPanelText,     &campaignPanelText,
	&loadScreenMeterBack,         &loadScreenMeterBack2,             &loadScreenMeterBackText,
	&loadScreenPins,
	NULL,
};

/**
 * @brief CG_LoadPanel_GameTypeName
 * @param[in] gt
 * @return
 */
const char *CG_LoadPanel_GameTypeName(gametype_t gt)
{
	switch (gt)
	{
	case GT_SINGLE_PLAYER:
		return CG_TranslateString("Single Player");
	case GT_COOP:
		return CG_TranslateString("Co-op");
	case GT_WOLF:
		return CG_TranslateString("Objective");
	case GT_WOLF_STOPWATCH:
		return CG_TranslateString("Stopwatch");
	case GT_WOLF_CAMPAIGN:
		return CG_TranslateString("Campaign");
	case GT_WOLF_LMS:
		return CG_TranslateString("Last Man Standing");
	case GT_WOLF_MAPVOTE:
		return CG_TranslateString("Map Voting");
	default:
		break;
	}

	return "Invalid";
}

static vec4_t clr3 = { 1.f, 1.f, 1.f, .6f };

/**
 * @brief CG_DrawConnectScreen
 * @param[in] interactive
 * @param[in] forcerefresh
 */
void CG_DrawConnectScreen(qboolean interactive, qboolean forcerefresh)
{
	static qboolean inside = qfalse;
	char            buffer[1024];

	if (!DC)
	{
		return;
	}

	if (inside)
	{
		return;
	}

	inside = qtrue;

	if (!bg_loadscreeninited)
	{
		trap_Cvar_Set("ui_connecting", "0");

		RegisterFont("ariblk", 27, &cgs.media.bg_loadscreenfont1);
		RegisterFont("courbd", 30, &cgs.media.bg_loadscreenfont2);

		bg_axispin    = DC->registerShaderNoMip("gfx/loading/pin_axis");
		bg_alliedpin  = DC->registerShaderNoMip("gfx/loading/pin_allied");
		bg_neutralpin = DC->registerShaderNoMip("gfx/loading/pin_neutral");
		bg_pin        = DC->registerShaderNoMip("gfx/loading/pin_shot");

		bg_filter_bo = DC->registerShaderNoMip("ui/assets/filter_bots");
		bg_filter_ff = DC->registerShaderNoMip("ui/assets/filter_ff");
		bg_filter_hw = DC->registerShaderNoMip("ui/assets/filter_weap");
		bg_filter_lv = DC->registerShaderNoMip("ui/assets/filter_lives");
		bg_filter_al = DC->registerShaderNoMip("ui/assets/filter_antilag");
		bg_filter_bt = DC->registerShaderNoMip("ui/assets/filter_balance");

		bg_mappic = 0;

		BG_PanelButtonsSetup(loadpanelButtons);
		C_PanelButtonsSetup(loadpanelButtons, cgs.wideXoffset);

		bg_loadscreeninited = qtrue;
	}

	BG_PanelButtonsRender(loadpanelButtons);

	if (interactive)
	{
		DC->drawHandlePic(DC->cursorx, DC->cursory, 32, 32, DC->Assets.cursor);
	}

	DC->getConfigString(CS_SERVERINFO, buffer, sizeof(buffer));
	if (*buffer)
	{
		const char *str;
		float      x = 540.0f + cgs.wideXoffset;
		float      y = 322;
		int        i;
		qboolean   enabled = qfalse;

		CG_Text_Paint_Centred_Ext(x, y, 0.22f, 0.22f, clr3, va("^1%s ^0%s", MODNAME, ETLEGACY_VERSION), 0, 0, 0, &cgs.media.bg_loadscreenfont1);

		y   = 340;
		str = Info_ValueForKey(buffer, "sv_hostname");
		CG_Text_Paint_Centred_Ext(x, y, 0.2f, 0.2f, colorWhite, str && *str ? str : "ETHost", 0, 26, 0, &cgs.media.bg_loadscreenfont2);


		y += 14;
		for (i = 0; i < MAX_MOTDLINES; i++)
		{
			str = CG_ConfigString(CS_CUSTMOTD + i);
			if (!str || !*str)
			{
				break;
			}

			CG_Text_Paint_Centred_Ext(x, y, 0.2f, 0.2f, colorWhite, str, 0, 26, 0, &cgs.media.bg_loadscreenfont2);

			y += 10;
		}

		y = 417;

		str = Info_ValueForKey(buffer, "g_friendlyfire");
		if (str && *str && Q_atoi(str))
		{
			x = 461 + cgs.wideXoffset;
			CG_DrawPic(x, y, 16, 16, bg_filter_ff);
		}

		if (Q_atoi(Info_ValueForKey(buffer, "g_gametype")) != GT_WOLF_LMS)
		{
			str = Info_ValueForKey(buffer, "g_maxlives");
			if (str && *str && Q_atoi(str))
			{
				enabled = qtrue;
			}

			if (!enabled)
			{
				str = Info_ValueForKey(buffer, "g_alliedmaxlives");
				if (str && *str && Q_atoi(str))
				{
					enabled = qtrue;
				}
			}

			if (!enabled)
			{
				str = Info_ValueForKey(buffer, "g_axismaxlives");
				if (str && *str && Q_atoi(str))
				{
					enabled = qtrue;
				}
			}
		}

		if (enabled)
		{
			x = 489 + cgs.wideXoffset;
			CG_DrawPic(x, y, 16, 16, bg_filter_lv);
		}

		str = Info_ValueForKey(buffer, "omnibot_playing");
		if (str && *str && Q_atoi(str))
		{
			x = 518 + cgs.wideXoffset;
			CG_DrawPic(x, y, 16, 16, bg_filter_bo);
		}

		str = Info_ValueForKey(buffer, "g_heavyWeaponRestriction");
		if (str && *str && Q_atoi(str) != 100)
		{
			x = 546 + cgs.wideXoffset;
			CG_DrawPic(x, y, 16, 16, bg_filter_hw);
		}

		str = Info_ValueForKey(buffer, "g_antilag");
		if (str && *str && Q_atoi(str))
		{
			x = 575 + cgs.wideXoffset;
			CG_DrawPic(x, y, 16, 16, bg_filter_al);
		}

		str = Info_ValueForKey(buffer, "g_balancedteams");
		if (str && *str && Q_atoi(str))
		{
			x = 604 + cgs.wideXoffset;
			CG_DrawPic(x, y, 16, 16, bg_filter_bt);
		}
	}

	if (*cgs.rawmapname)
	{
		float x = 16 + cgs.wideXoffset + 1;

		if (!bg_mappic)
		{
			bg_mappic = DC->registerShaderNoMip(va("levelshots/%s", cgs.rawmapname));

			if (!bg_mappic)
			{
				bg_mappic = DC->registerShaderNoMip("levelshots/unknownmap");
			}
		}

		trap_R_SetColor(colorBlack);
		CG_DrawPic(x, 2 + 1, 192, 144, bg_mappic);

		trap_R_SetColor(NULL);
		x = 16 + cgs.wideXoffset;
		CG_DrawPic(x, 2, 192, 144, bg_mappic);

		x = 16 + cgs.wideXoffset + 80;
		CG_DrawPic(x, 2 + 6, 20, 20, bg_pin);
	}

	if (forcerefresh)
	{
		DC->updateScreen();
	}

	inside = qfalse;
}

/**
 * @brief CG_LoadPanel_RenderLoadingBar
 * @param[in] button
 */
void CG_LoadPanel_RenderLoadingBar(panel_button_t *button)
{
	int   hunkused, hunkexpected;
	float frac;

	trap_GetHunkData(&hunkused, &hunkexpected);

	if (hunkexpected <= 0)
	{
		return;
	}

	frac = hunkused / (float)hunkexpected;
	if (frac < 0.f)
	{
		frac = 0.f;
	}
	if (frac > 1.f)
	{
		frac = 1.f;
	}

	CG_DrawPicST(button->rect.x, button->rect.y, button->rect.w * frac, button->rect.h, 0, 0, frac, 1, button->hShaderNormal);
}

/**
 * @brief CG_LoadPanel_RenderCampaignTypeText
 * @param[in] button
 */
void CG_LoadPanel_RenderCampaignTypeText(panel_button_t *button)
{
	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, va("%s:", CG_LoadPanel_GameTypeName(cgs.gametype)), 0, 0, button->font->style, button->font->font);
}

/**
 * @brief campaignNameTextScaleFactor
 * @param[in] len
 * @return
 */
float campaignNameTextScaleFactor(int len)
{
	float scaleF = 1.f;

	//CG_Printf("CampaignNameText len: %i\n", len);

	if (len >= 27)
	{
		scaleF *= 0.8f;
		return scaleF;
	}
	// in between scale is 1
	else if (len <= 20 && len > 17)
	{
		scaleF *= 1.25f;
		return scaleF;
	}
	else if (len <= 17 && len > 13)
	{
		scaleF *= 1.5f;
		return scaleF;
	}
	else if (len <= 13)
	{
		scaleF *= 2;
		return scaleF;
	}

	return scaleF;
}

/**
 * @brief CG_LoadPanel_RenderCampaignNameText
 * @param[in] button
 */
void CG_LoadPanel_RenderCampaignNameText(panel_button_t *button)
{
	const char *cs;
	float      w;
	float      scaleF;

	if (cgs.gametype == GT_WOLF_CAMPAIGN)
	{
		cs = DC->nameForCampaign();
		if (!cs)
		{
			return;
		}

		cs = va("%s %iof%i", cs, cgs.currentCampaignMap + 1, cgs.campaignData.mapCount);

		scaleF = campaignNameTextScaleFactor(Q_PrintStrlen(cs));

		w = CG_Text_Width_Ext(cs, button->font->scalex * scaleF, 0, button->font->font);
		CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y, button->font->scalex * scaleF, button->font->scaley * scaleF, button->font->colour, cs, 0, 0, 0, button->font->font);

	}
	else
	{
		if (!cgs.arenaInfoLoaded)
		{
			return;
		}

		scaleF = campaignNameTextScaleFactor(Q_PrintStrlen(cgs.arenaData.longname)); // FIXME: up to 128 chars !

		w = CG_Text_Width_Ext(cgs.arenaData.longname, button->font->scalex * scaleF, 0, button->font->font);
		CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y, button->font->scalex * scaleF, button->font->scaley * scaleF, button->font->colour, cgs.arenaData.longname, 0, 0, 0, button->font->font);
	}
}

/**
 * @brief CG_LoadPanel_RenderMissionDescriptionText
 * @param[in] button
 */
void CG_LoadPanel_RenderMissionDescriptionText(panel_button_t *button)
{
	const char *cs;
	char       *s, *p;
	char       buffer[1024];
	float      y;

	if (cgs.gametype == GT_WOLF_CAMPAIGN)
	{
		cs = DC->descriptionForCampaign();
		if (!cs)
		{
			return;
		}

	}
	else if (cgs.gametype == GT_WOLF_LMS)
	{
		if (!cgs.arenaInfoLoaded)
		{
			return;
		}

		cs = cgs.arenaData.lmsdescription;
	}
	else
	{
		if (!cgs.arenaInfoLoaded)
		{
			return;
		}

		cs = cgs.arenaData.description;
	}

	Q_strncpyz(buffer, cs, sizeof(buffer));
	while ((s = strchr(buffer, '*')))
	{
		*s = '\n';
	}

	BG_FitTextToWidth_Ext(buffer, button->font->scalex, button->rect.w - 16, sizeof(buffer), button->font->font);

	y = button->rect.y + 12;

	s = p = buffer;
	while (*p)
	{
		if (*p == '\n')
		{
			*p++ = '\0';
			DC->drawTextExt(button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0, button->font->font);
			y += 8;
			s  = p;
		}
		else
		{
			p++;
		}
	}
}

/**
 * @brief CG_LoadPanel_KeyHandling
 * @param[in] key
 * @param[in] down
 */
void CG_LoadPanel_KeyHandling(int key, qboolean down)
{
	if (BG_PanelButtonsKeyEvent(key, down, loadpanelButtons))
	{
		return;
	}
}

/**
 * @brief CG_LoadPanel_DrawPin
 * @param[in] text
 * @param[in] px
 * @param[in] py
 * @param[in] sx
 * @param[in] sy
 * @param[in] shader
 * @param[in] pinsize
 * @param[in] backheight
 */
void CG_LoadPanel_DrawPin(const char *text, float px, float py, float sx, float sy, qhandle_t shader, float pinsize, float backheight)
{
	static vec4_t colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };
	float         w                = DC->textWidthExt(text, sx, 0, &cgs.media.bg_loadscreenfont2);
	qboolean      fit              = (px + 20 + w > 440) ? qtrue : qfalse;

	px += cgs.wideXoffset;

	// Pin half width is 16
	// Pin left margin is 4
	// Pin right margin is 0
	// Text margin is 4
	if (fit)
	{
		// x - pinhwidth (16) - pin left margin (4) - w - text margin (4) => x - w - 24
		DC->fillRect(px - w - 24 + 2, py - (backheight / 2.f) + 2, 24 + w, backheight, colourFadedBlack);
		DC->fillRect(px - w - 24, py - (backheight / 2.f), 24 + w, backheight, colorBlack);
	}
	else
	{
		// Width = pinhwidth (16) + pin right margin (0) + w + text margin (4) = 20 + w
		DC->fillRect(px + 2, py - (backheight / 2.f) + 2, 20 + w, backheight, colourFadedBlack);
		DC->fillRect(px, py - (backheight / 2.f), 20 + w, backheight, colorBlack);
	}

	DC->drawHandlePic(px - pinsize, py - pinsize, pinsize * 2.f, pinsize * 2.f, shader);

	if (fit)
	{
		// x - pinhwidth (16) - pin left margin (4) - w => x - w - 20
		DC->drawTextExt(px - w - 20, py + 4, sx, sy, colorWhite, text, 0, 0, 0, &cgs.media.bg_loadscreenfont2);
	}
	else
	{
		// x + pinhwidth (16) + pin right margin (0) => x + 16
		DC->drawTextExt(px + 16, py + 4, sx, sy, colorWhite, text, 0, 0, 0, &cgs.media.bg_loadscreenfont2);
	}
}

/**
 * @brief CG_LoadPanel_RenderCampaignPins
 * @param button - unused
 */
void CG_LoadPanel_RenderCampaignPins(panel_button_t *button)
{
	if (cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS || cgs.gametype == GT_WOLF || cgs.gametype == GT_WOLF_MAPVOTE)
	{
		float px, py;

		if (!cgs.arenaInfoLoaded)
		{
			return;
		}

		px = (cgs.arenaData.mappos[0] / 1024.f) * 440.f;
		py = (cgs.arenaData.mappos[1] / 1024.f) * 480.f;

		CG_LoadPanel_DrawPin(cgs.arenaData.longname, px, py, 0.22f, 0.25f, bg_neutralpin, 16.f, 16.f);
	}
	else
	{
		qhandle_t shader;
		float     px, py;
		int       i;

		if (!cgs.campaignInfoLoaded)
		{
			return;
		}

		for (i = 0; i < cgs.campaignData.mapCount; i++)
		{
			cg.teamWonRounds[1] = Q_atoi(CG_ConfigString(CS_ROUNDSCORES1));
			cg.teamWonRounds[0] = Q_atoi(CG_ConfigString(CS_ROUNDSCORES2));

			if (cg.teamWonRounds[1] & (1 << i))
			{
				shader = bg_axispin;
			}
			else if (cg.teamWonRounds[0] & (1 << i))
			{
				shader = bg_alliedpin;
			}
			else
			{
				shader = bg_neutralpin;
			}

			px = (cgs.campaignData.arenas[i].mappos[0] / 1024.f) * 440.f;
			py = (cgs.campaignData.arenas[i].mappos[1] / 1024.f) * 480.f;

			CG_LoadPanel_DrawPin(cgs.campaignData.arenas[i].longname, px, py, 0.22f, 0.25f, shader, 16.f, 16.f);
		}
	}
}

/**
 * @brief Draws infoScreenText in loading bar
 * @param[in] button
 */
void CG_LoadPanel_LoadingBarText(panel_button_t *button)
{
	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, cg.infoScreenText, 0, 0, 0, button->font->font);
}
