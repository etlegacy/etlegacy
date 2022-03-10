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
 * @file ui_loadpanel.c
 */

#include "ui_local.h"
#include "ui_shared.h"

qboolean bg_loadscreeninited = qfalse;

void UI_LoadPanel_RenderHeaderText(panel_button_t *button);
void UI_LoadPanel_RenderLoadingText(panel_button_t *button);

// panel_button_text_t FONTNAME = { SCALEX, SCALEY, COLOUR, STYLE, FONT };

panel_button_text_t missiondescriptionTxt =
{
	0.2f,                                   0.2f,
	{ 0.0f,                                 0.0f,0.0f,    1.f },
	0,                                      0,
	&uiInfo.uiDC.Assets.bg_loadscreenfont2,
};

panel_button_text_t missiondescriptionHeaderTxt =
{
	0.2f,                                   0.2f,
	{ 0.0f,                                 0.0f,             0.0f,    0.8f },
	0,                                      ITEM_ALIGN_CENTER,
	&uiInfo.uiDC.Assets.bg_loadscreenfont2,
};

panel_button_text_t campaignpheaderTxt =
{
	0.2f,                                   0.2f,
	{ 1.0f,                                 1.0f,1.0f,    0.6f },
	0,                                      0,
	&uiInfo.uiDC.Assets.bg_loadscreenfont2,
};

panel_button_text_t campaignpTxt =
{
	0.35f,                                  0.35f,
	{ 1.0f,                                 1.0f, 1.0f,  0.6f },
	0,                                      0,
	&uiInfo.uiDC.Assets.bg_loadscreenfont2,
};

panel_button_t loadScreenMap =
{
	"gfx/loading/camp_map",
	NULL,
	{ 0,                      0,  440, 480 }, // shouldn't this be square??
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     // font
	NULL,                     // keyDown
	NULL,                     // keyUp
	BG_PanelButtonsRender_Img,
	NULL,
	0,
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
	0,
};

panel_button_t loadingPanelText =
{
	NULL,
	NULL,
	{ 460,                         72,   160, 244 },
	{ 0,                           0,    0,   0, 0, 0, 0, 0},
	&missiondescriptionTxt,        // font
	NULL,                          // keyDown
	NULL,                          // keyUp
	UI_LoadPanel_RenderLoadingText,
	NULL,
	0,
};

panel_button_t campaignPanelText =
{
	NULL,
	NULL,                         //"CONNECTING...",
	{ 470,                        33,   152, 232 },
	{ 0,                          0,    0,   0, 0, 0, 0, 0},
	&campaignpTxt,                // font
	NULL,                         // keyDown
	NULL,                         // keyUp
	UI_LoadPanel_RenderHeaderText,
	NULL,
	0,
};

panel_button_t *loadpanelButtons[] =
{
	&loadScreenMap,
	&loadScreenBack,
	&loadingPanelText,
	/*&loadingPanelHeaderText,*/
	/*&campaignheaderPanelText,*/
	&campaignPanelText,
	NULL,
};

static qboolean connect_ownerdraw;

/**
 * @brief UI_DrawLoadPanel
 * @param[in] ownerdraw
 * @param[in] uihack
 */
void UI_DrawLoadPanel(qboolean ownerdraw, qboolean uihack)
{
	static qboolean inside = qfalse;

	// to avoid a flickering screen on widescreens, we erase it before drawing onto it..
	if (((float)(DC->glconfig.vidWidth) / DC->glconfig.vidHeight) != RATIO43)
	{
		float xoffset = Cui_WideXoffset() * DC->xscale;

		trap_R_DrawStretchPic(0, 0, xoffset, DC->glconfig.vidHeight, 0, 0, 1, 1, DC->registerShaderNoMip("gfx/2d/backtile"));
		trap_R_DrawStretchPic(DC->glconfig.vidWidth - xoffset, 0, xoffset, DC->glconfig.vidHeight, 0, 0, 1, 1, DC->registerShaderNoMip("gfx/2d/backtile"));
	}

	if (inside)
	{
		if (!uihack && trap_Cvar_VariableValue("ui_connecting") != 0.f)
		{
			trap_Cvar_Set("ui_connecting", "0");
		}
		return;
	}

	connect_ownerdraw = ownerdraw;

	inside = qtrue;

	if (!bg_loadscreeninited)
	{
		RegisterFont("ariblk", 27, &uiInfo.uiDC.Assets.bg_loadscreenfont1);
		RegisterFont("courbd", 30, &uiInfo.uiDC.Assets.bg_loadscreenfont2);

		BG_PanelButtonsSetup(loadpanelButtons);
		C_PanelButtonsSetup(loadpanelButtons, Cui_WideXoffset());   // convert to possible widescreen coordinates..

		bg_loadscreeninited = qtrue;
	}

	BG_PanelButtonsRender(loadpanelButtons);

	if (!uihack && trap_Cvar_VariableValue("ui_connecting") != 0.f)
	{
		trap_Cvar_Set("ui_connecting", "0");
	}

	inside = qfalse;
}

/**
 * @brief UI_LoadPanel_RenderHeaderText
 * @param[in,out] button
 */
void UI_LoadPanel_RenderHeaderText(panel_button_t *button)
{
	uiClientState_t cstate;
	char            downloadName[MAX_INFO_VALUE];

	trap_GetClientState(&cstate);

	trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName, sizeof(downloadName));

	if ((cstate.connState == CA_DISCONNECTED || cstate.connState == CA_CONNECTED) && *downloadName)
	{
		button->text = (char*)(__("DOWNLOADING..."));
	}
	else
	{
		button->text = (char*)(__("CONNECTING..."));
	}

	BG_PanelButtonsRender_Text(button);
}

#define ESTIMATES 80

/**
 * @brief UI_DownloadInfo
 * @param[in] downloadName
 * @return
 */
const char *UI_DownloadInfo(const char *downloadName)
{
	static int  tleEstimates[ESTIMATES] = { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
		                                    60,  60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
		                                    60,  60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
		                                    60,  60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 };
	static int  tleIndex = 0;
	char        dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int         downloadSize, downloadCount, downloadTime;
	const char *dlText, *etaText, *xferText;
	const char  *s, *ds;

	dlText = __("Downloading:");
	etaText = __("Estimated time left:");
	xferText = __("Transfer rate:");
	downloadSize  = trap_Cvar_VariableValue("cl_downloadSize");
	downloadCount = trap_Cvar_VariableValue("cl_downloadCount");
	downloadTime  = trap_Cvar_VariableValue("cl_downloadTime");

	if (downloadSize > 0)
	{
		ds = va("%s (%d%%)", downloadName, (int)((float)downloadCount * 100.0f / (float)downloadSize));
	}
	else
	{
		ds = downloadName;
	}

	UI_ReadableSize(dlSizeBuf, sizeof dlSizeBuf, downloadCount);
	UI_ReadableSize(totalSizeBuf, sizeof totalSizeBuf, downloadSize);

	if (downloadCount < 4096 || !downloadTime)
	{
		s = va("%s\n %s\n%s\n\n%s\n %s...\n\n%s\n\n%s %s", dlText, ds, totalSizeBuf,
		       etaText,
		       __("estimating"),
		       xferText,
		       dlSizeBuf,
		       __("copied"));
	}
	else
	{
		int xferRate;

		if ((uiInfo.uiDC.realTime - downloadTime) / 1000)
		{
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		}
		else
		{
			xferRate = 0;
		}
		UI_ReadableSize(xferRateBuf, sizeof xferRateBuf, xferRate);

		// Extrapolate estimated completion time
		if (downloadSize && xferRate)
		{
			int n = downloadSize / xferRate;        // estimated time for entire d/l in secs
			int timeleft = 0, i;

			// We do it in K (/1024) because we'd overflow around 4MB
			tleEstimates[tleIndex] = (n - (((downloadCount / 1024) * n) / (downloadSize / 1024)));
			tleIndex++;
			if (tleIndex >= ESTIMATES)
			{
				tleIndex = 0;
			}

			for (i = 0; i < ESTIMATES; i++)
				timeleft += tleEstimates[i];

			timeleft /= ESTIMATES;

			UI_PrintTime(dlTimeBuf, sizeof dlTimeBuf, timeleft);
		}
		else
		{
			dlTimeBuf[0] = '\0';
		}

		if (xferRate)
		{
			s = va("%s\n %s\n%s\n\n%s\n %s\n\n%s\n %s/%s\n\n%s %s", dlText, ds, totalSizeBuf,
			       etaText, dlTimeBuf,
			       xferText, xferRateBuf,
			       __("sec"), dlSizeBuf,
			       __("copied"));
		}
		else
		{
			if (downloadSize)
			{
				s = va("%s\n %s\n%s\n\n%s\n %s...\n\n%s\n\n%s %s", dlText, ds, totalSizeBuf,
				       etaText,
					   __("estimating"),
				       xferText,
				       dlSizeBuf,
					   __("copied"));
			}
			else
			{
				s = va("%s\n %s\n\n%s\n %s...\n\n%s\n\n%s %s", dlText, ds,
				       etaText,
					   __("estimating"),
				       xferText,
				       dlSizeBuf,
					   __("copied"));
			}
		}
	}

	return s;
}

/**
 * @brief UI_LoadPanel_RenderLoadingText
 * @param[in] button
 */
void UI_LoadPanel_RenderLoadingText(panel_button_t *button)
{
	uiClientState_t cstate;
	char            downloadName[MAX_INFO_VALUE];
	char            buff[2560];
	char            *p1, *p2 = "";
	const char      *s = "";
	float           y;

	trap_GetClientState(&cstate);

	Com_sprintf(buff, sizeof(buff), __("Connecting to:\n %s^*\n\n%s"), cstate.servername, Info_ValueForKey(cstate.updateInfoString, "motd"));

	if (trap_Cvar_VariableValue("com_updateavailable") != 0.f)
	{
		Q_strcat(buff, sizeof(buff), "\n\n");
		Q_strcat(buff, sizeof(buff), __("Your ET: Legacy client is outdated. New update is available for download at www.etlegacy.com"));
	}

	trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName, sizeof(downloadName));

	if (!connect_ownerdraw)
	{
		if (trap_Cvar_VariableValue("ui_connecting") == 0.f)
		{
			switch (cstate.connState)
			{
			case CA_CONNECTING:
				s = va(__("Awaiting connection...%i"), cstate.connectPacketCount);
				break;
			case CA_CHALLENGING:
				s = va(__("Awaiting challenge...%i"), cstate.connectPacketCount);
				break;
			case CA_DISCONNECTED:
			case CA_CONNECTED:
				if (*downloadName || cstate.connState == CA_DISCONNECTED)
				{
					s = UI_DownloadInfo(downloadName);
				}
				else
				{
					s = __("Awaiting gamestate...");
				}
				break;
			case CA_LOADING:
			case CA_PRIMED:
			default:
				break;
			}
		}
		else if (trap_Cvar_VariableValue("ui_dl_running") != 0.f)
		{
			// only toggle during a disconnected download
			s = UI_DownloadInfo(downloadName);
		}

		Q_strcat(buff, sizeof(buff), va("\n\n%s^*", s));

		if (cstate.connState < CA_CONNECTED && *cstate.messageString)
		{
			Q_strcat(buff, sizeof(buff), va("\n\n%s^*", cstate.messageString));
		}
	}

	BG_FitTextToWidth_Ext(buff, button->font->scalex, button->rect.w, sizeof(buff), button->font->font);

	//UI_DrawRect( button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorRed );

	y = button->rect.y + 12;

	p1 = p2 = buff;

	while (*p1)
	{
		if (*p1 == '\n')
		{
			*p1++ = '\0';
			Text_Paint_Ext(button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, p2, 0, 0, 0, button->font->font);
			y += 8;
			p2 = p1;
		}
		else
		{
			p1++;
		}
	}
}
