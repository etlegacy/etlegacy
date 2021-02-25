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
 * @file cg_info.c
 * @brief Display information while data is being loaded
 */

#include "cg_local.h"

// Color/font info used for all overlays
#define COLOR_BG            { 0.0f, 0.0f, 0.0f, 0.6f }
#define COLOR_BG_TITLE      { 0.16f, 0.2f, 0.17f, 0.8f }
#define COLOR_BG_VIEW       { 0.16f, 0.2f, 0.17f, 0.8f }
#define COLOR_BORDER        { 0.5f, 0.5f, 0.5f, 0.5f }
#define COLOR_BORDER_TITLE  { 0.1f, 0.1f, 0.1f, 0.2f }
#define COLOR_BORDER_VIEW   { 0.2f, 0.2f, 0.2f, 0.4f }
#define COLOR_TEXT          { 0.6f, 0.6f, 0.6f, 1.0f }

#define FONT_HEADER         &cgs.media.limboFont1
#define FONT_SUBHEADER      &cgs.media.limboFont1_lo
#define FONT_TEXT           &cgs.media.limboFont2

const vec4_t color_bg_title = COLOR_BG_TITLE;
const vec4_t color_border1  = COLOR_BORDER;
const vec4_t color_bg       = COLOR_BG_VIEW;
const vec4_t color_border   = COLOR_BORDER_VIEW;

// NOTE: Unused
//#define VD_X    4
//#define VD_Y    78
//#define VD_SCALE_X_HDR  0.25f
//#define VD_SCALE_Y_HDR  0.30f
//#define VD_SCALE_X_NAME 0.30f
//#define VD_SCALE_Y_NAME 0.30f

/**
 * @brief CG_LoadingString
 * @param[in] s
 */
void CG_LoadingString(const char *s)
{
	Q_strncpyz(cg.infoScreenText, s, sizeof(cg.infoScreenText));

	if (s && *s)
	{
		CG_Printf("LOADING... %s\n", s);      // added so you can see from the console what's going on

	}
}

/**
 * @brief Draw all the status / pacifier stuff during level loading
 * @param[in] forcerefresh
 */
void CG_DrawInformation(qboolean forcerefresh)
{
	static int nextcall = 0;
	int        ms;

	if (cg.snap)
	{
		return;     // we are in the world, no need to draw information
	}

	ms = trap_Milliseconds();

	if (ms < nextcall)
	{
		return;
	}

	nextcall = ms + 250;

	// loadpanel: erase the screen now, because otherwise the "awaiting challenge"-UI-screen is still visible behind the client-version of it (the one with the progressbar),..
	// ..and we do not want a flickering screen (on widescreens).
	// debriefing screen: no need to erase the screen..
	if (!cgs.dbShowing)
	{
		if (!cgs.media.backTileShader)
		{
			cgs.media.backTileShader = trap_R_RegisterShaderNoMip("gfx/2d/backtile");
		}
		if (cgs.glconfig.windowAspect != RATIO43)
		{
			float xoffset = Ccg_WideXoffset() * cgs.screenXScale;

			trap_R_DrawStretchPic(0, 0, xoffset, cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.backTileShader);                                     // left side
			trap_R_DrawStretchPic(cgs.glconfig.vidWidth - xoffset, 0, xoffset, cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.backTileShader);       // right side
		}
	}

	CG_DrawConnectScreen(qfalse, forcerefresh);

	// TODO: dynamic game server MOTD window
	/*  if(cg.motdWindow == NULL) {
	        CG_createMOTDWindow();
	    }
	    if(cg.motdWindow != NULL) {
	        CG_windowDraw();
	    }
	*/
}

/**
 * @brief CG_ShowHelp_On
 * @param[in,out] status
 */
void CG_ShowHelp_On(int *status)
{
	int milli = trap_Milliseconds();

	if (*status == SHOW_SHUTDOWN && milli < cg.fadeTime)
	{
		cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
	}
	else if (*status != SHOW_ON)
	{
		cg.fadeTime = milli + STATS_FADE_TIME;
	}

	*status = SHOW_ON;
}

/**
 * @brief CG_ShowHelp_Off
 * @param[in,out] status
 */
void CG_ShowHelp_Off(int *status)
{
	if (*status != SHOW_OFF)
	{
		int milli = trap_Milliseconds();

		if (milli < cg.fadeTime)
		{
			cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
		}
		else
		{
			cg.fadeTime = milli + STATS_FADE_TIME;
		}

		*status = SHOW_SHUTDOWN;
	}
}

/**
 * @brief CG_DemoControlButtonRender
 * @param[in] button
 */
void CG_DemoControlButtonRender(panel_button_t *button)
{
	if (button->data[0])
	{
		CG_FillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, color_bg_title);
		CG_DrawRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1, color_border);
		//BG_PanelButtonsRender_Text(button);

		CG_Text_Paint_Ext(button->rect.x + button->rect.w * 0.4f, button->rect.y + button->rect.h * 0.7f, button->font->scalex, button->font->scaley, button->font->colour, button->text, 0.0f, 0, button->font->style, button->font->font);
	}
	else
	{
		float  demoStatus = ((float)(cg.time - cg.demoinfo->firstTime)) / (cg.demoinfo->lastTime - cg.demoinfo->firstTime);
		vec4_t barColor;

		Vector4Copy(colorGreen, barColor);
		barColor[3] = button->font->colour[3];

		//borderColor
		CG_FilledBar(button->rect.x, button->rect.y, button->rect.w, button->rect.h, barColor, NULL, color_border1, demoStatus, BAR_BG);
	}
}

/**
 * @brief CG_DemoControlButtonDown
 * @param[in] button
 * @param[in] key
 * @return
 */
qboolean CG_DemoControlButtonDown(panel_button_t *button, int key)
{
	if (key != K_MOUSE1 && key != K_MOUSE2)
	{
		return qfalse;
	}

	switch (button->data[0])
	{
	case 0:
	{
		int   result;
		float offset = cgDC.cursorx - button->rect.x;

		offset = offset / button->rect.w;
		result = (int)(cg.demoinfo->firstTime + ((cg.demoinfo->lastTime - cg.demoinfo->firstTime) * offset));
		trap_SendConsoleCommand(va("seekservertime %i", result));
	}
	break;
	case 1:
		trap_SendConsoleCommand("rewind 5");
		break;
	case 2:
		trap_SendConsoleCommand("pausedemo");
		break;
	case 3:
		trap_SendConsoleCommand("fastforward 5");
		break;
	default:
		break;
	}
	return qtrue;
}

/**
 * @brief CG_DemoControlButtonUp
 * @param button - unused
 * @param key - unused
 * @note This function is empty and return always qfalse
 * @return Always qfalse
 */
qboolean CG_DemoControlButtonUp(panel_button_t *button, int key)
{
	return qfalse;
}

panel_button_t demoSliderButton =
{
	NULL,
	NULL,
	{ 0,                       0,  0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	NULL,                      // font
	CG_DemoControlButtonDown,  // keyDown
	CG_DemoControlButtonUp,    // keyUp
	CG_DemoControlButtonRender,
	NULL,
	0
};

panel_button_t demoRewindButton =
{
	NULL,
	"<<",
	{ 0,                       0,  0, 0 },
	{ 1,                       0,  0, 0, 0, 0, 0, 0},
	NULL,                      // font
	CG_DemoControlButtonDown,  // keyDown
	CG_DemoControlButtonUp,    // keyUp
	CG_DemoControlButtonRender,
	NULL,
	0
};

panel_button_t demoPauseButton =
{
	NULL,
	"||",
	{ 0,                       0,  0, 0 },
	{ 2,                       0,  0, 0, 0, 0, 0, 0},
	NULL,                      // font
	CG_DemoControlButtonDown,  // keyDown
	CG_DemoControlButtonUp,    // keyUp
	CG_DemoControlButtonRender,
	NULL,
	0
};

panel_button_t demoFFButton =
{
	NULL,
	">>",
	{ 0,                       0,  0, 0 },
	{ 3,                       0,  0, 0, 0, 0, 0, 0},
	NULL,                      // font
	CG_DemoControlButtonDown,  // keyDown
	CG_DemoControlButtonUp,    // keyUp
	CG_DemoControlButtonRender,
	NULL,
	0
};

static panel_button_t *demoControlButtons[] =
{
	&demoSliderButton,
	&demoRewindButton,
	&demoPauseButton,
	&demoFFButton,
	NULL
};

/**
 * @brief Demo playback key catcher support
 * @param[in] key
 * @param[in] down
 */
void CG_DemoClick(int key, qboolean down)
{
	int milli = trap_Milliseconds();

	// Avoid active console keypress issues
	if (!down && !cgs.fKeyPressed[key])
	{
		return;
	}

	cgs.fKeyPressed[key] = down;

	if (BG_PanelButtonsKeyEvent(key, down, demoControlButtons))
	{
		return;
	}

#ifdef FEATURE_EDV

	cgs.demoCamera.factor = 5;

	if (cg.demohelpWindow == SHOW_ON)
	{
		// pull up extended helpmenu
		if (cgs.currentMenuLevel == ML_MAIN && (key == K_LALT || key == K_RALT))
		{
			cgs.currentMenuLevel = ML_EDV;
			return;
		}
		// return to main helpmenu
		if (cgs.currentMenuLevel == ML_EDV && key == K_BACKSPACE)
		{
			// back to main menu
			if (!down)
			{
				cgs.currentMenuLevel = ML_MAIN;
			}
			return;
		}
	}
#endif

	switch (key)
	{
#ifdef FEATURE_MULTIVIEW
	case K_MOUSE2:
		if (!down)
		{
			CG_mvSwapViews_f();             // Swap the window with the main view
		}
		return;
	//case K_INS: // used by FEATURE_EDV
	case K_KP_PGUP:
		if (!down)
		{
			CG_mvShowView_f();              // Make a window for the client
		}
		return;
	//case K_DEL: // used by FEATURE_EDV
	case K_KP_PGDN:
		if (!down)
		{
			CG_mvHideView_f();              // Delete the window for the client
		}
		return;
	case K_MOUSE3:
		if (!down)
		{
			CG_mvToggleView_f();            // Toggle a window for the client
		}
		return;
#endif // ifdef FEATURE_MULTIVIEW
#ifdef FEATURE_EDV
	case K_KP_ENTER:
		if (!down)
		{
			if (cg.snap->ps.leanf != 0.f && !cgs.demoCamera.renderingFreeCam)
			{
				cgs.demoCamera.startLean = qtrue;
			}

			cgs.demoCamera.renderingFreeCam ^= qtrue;

			if (cgs.demoCamera.renderingFreeCam)
			{
				int viewheight;

				if (cg.snap->ps.eFlags & EF_CROUCHING)
				{
					viewheight = CROUCH_VIEWHEIGHT;
				}
				else if (cg.snap->ps.eFlags & EF_PRONE || cg.snap->ps.eFlags & EF_PRONE_MOVING)
				{
					viewheight = PRONE_VIEWHEIGHT;
				}
				else
				{
					viewheight = DEFAULT_VIEWHEIGHT;
				}
				cgs.demoCamera.camOrigin[2] += viewheight;
			}
		}
		return;
	case K_INS: // mortarcam
		if (!down)
		{
			if (demo_weaponcam.integer & DWC_MORTAR)
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer &= ~DWC_MORTAR));
			}
			else
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer |= DWC_MORTAR));
			}
		}
		return;
	case K_HOME: // panzercam
		if (!down)
		{
			if (demo_weaponcam.integer & DWC_PANZER)
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer &= ~DWC_PANZER));
			}
			else
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer |= DWC_PANZER));
			}
		}
		return;
	case K_DEL:     // grenadecam
		if (!down)
		{
			if (demo_weaponcam.integer & DWC_GRENADE)
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer &= ~DWC_GRENADE));
			}
			else
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer |= DWC_GRENADE));
			}
		}
		return;
	case K_END: // dynamitecam
		if (!down)
		{
			if (demo_weaponcam.integer & DWC_DYNAMITE)
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer &= ~DWC_DYNAMITE));
			}
			else
			{
				trap_Cvar_Set("demo_weaponcam", va("%i", demo_weaponcam.integer |= DWC_DYNAMITE));
			}
		}
		return;
	case K_PGDN:
		if (!down)
		{ // teamonlycam
			trap_Cvar_Set("demo_teamonlymissilecam", ((demo_teamonlymissilecam.integer == 0) ? "1" : "0"));
		}
		return;
	case K_LCTRL:
	case K_RCTRL:
		if (!down)
		{
			trap_Cvar_Set("demo_pvshint", ((demo_pvshint.integer == 0) ? "1" : "0"));
		}
		return;
	case K_F6:
		if (!down)
		{
			if (cg_drawSpectatorNames.integer == DEMO_NAMEOFF)
			{
				trap_Cvar_Set("cg_drawSpectatorNames", va("%i", DEMO_CLEANNAME));
			}
			else if (cg_drawSpectatorNames.integer == DEMO_CLEANNAME)
			{
				trap_Cvar_Set("cg_drawSpectatorNames", va("%i", DEMO_COLOREDNAME));
			}
			else
			{
				trap_Cvar_Set("cg_drawSpectatorNames", va("%i", DEMO_NAMEOFF));
			}
		}
		return;
#endif // ifdef FEATURE_EDV

	// Third-person controls
	case K_ENTER:
		if (!down)
		{
#ifdef FEATURE_EDV
			if (cgs.demoCamera.renderingFreeCam)
			{
				cgs.demoCamera.renderingFreeCam = qfalse;
			}
#endif
			trap_Cvar_Set("cg_thirdperson", ((cg_thirdPerson.integer == 0) ? "1" : "0"));
		}
		return;
	case K_UPARROW:
	case K_DOWNARROW:
#ifdef FEATURE_EDV
		// when not in thirdperson, arrowkeys should be bindable to +freecam_turnleft, ...
		if (cg.renderingThirdPerson && !cgs.demoCamera.renderingFreeCam)
#else
		if (cg.renderingThirdPerson)
#endif
		{
			if (milli > cgs.thirdpersonUpdate)
			{
				float range = cg_thirdPersonRange.value;

				cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
				if (key == K_UPARROW)
				{
					range -= ((range >= 4 * DEMO_RANGEDELTA) ? DEMO_RANGEDELTA : (range - DEMO_RANGEDELTA));
				}
				else
				{
					range += ((range >= 120 * DEMO_RANGEDELTA) ? 0 : DEMO_RANGEDELTA);
				}
				trap_Cvar_Set("cg_thirdPersonRange", va("%f", range));
			}
		}
#ifdef FEATURE_EDV
		else
		{
			CG_RunBinding(key, down);
		}
#endif
		return;
	case K_RIGHTARROW:
	case K_LEFTARROW:
#ifdef FEATURE_EDV
		if (cg.renderingThirdPerson && !cgs.demoCamera.renderingFreeCam)
#else
		if (cg.renderingThirdPerson)
#endif
		{
			if (milli > cgs.thirdpersonUpdate)
			{
				float angle = cg_thirdPersonAngle.value;

				angle += (key == K_LEFTARROW ? DEMO_ANGLEDELTA : -DEMO_ANGLEDELTA);

				cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
				if (key == K_RIGHTARROW)
				{
					if (angle < 0.f)
					{
						angle += 360.0f;
					}
				}
				else
				{
					if (angle >= 360.0f)
					{
						angle -= 360.0f;
					}
				}
				trap_Cvar_Set("cg_thirdPersonAngle", va("%f", angle));
			}
		}
#ifdef FEATURE_EDV
		else
		{
			CG_RunBinding(key, down);
		}
#endif
		return;

	case K_SPACE: // most everyone's favorite jump key, :x
		if (!down)
		{
			trap_SendConsoleCommand("pausedemo");
		}
		return;

	// Timescale controls
	case K_KP_5:
		//case K_KP_INS: // needed for "more options" --> FEATURE_EDV
		if (!down)
		{
			trap_Cvar_Set("timescale", "1");
			cgs.timescaleUpdate = cg.time + 1000;
		}
		return;
	case K_ESCAPE:
		if (!down)
		{
			trap_Cvar_Set("timescale", "1");
			cgs.timescaleUpdate = cg.time + 1000;
		}
		CG_ShowHelp_Off(&cg.demohelpWindow);
		CG_keyOff_f();
		return;
	// Help info
	case K_BACKSPACE:
		if (!down)
		{
			if (cg.demohelpWindow != SHOW_ON)
			{
				CG_ShowHelp_On(&cg.demohelpWindow);
			}
			else
			{
				CG_ShowHelp_Off(&cg.demohelpWindow);
			}
		}
		return;
	case K_TAB:
		if (down)
		{
			CG_ScoresDown_f();
		}
		else
		{
			CG_ScoresUp_f();
		}
		return;
	// Window controls
	case K_LSHIFT:
	case K_RSHIFT:
	//case K_CTRL:
	case K_MOUSE4:
		cgs.fResize = down;
		return;
	case K_MOUSE1:
		cgs.fSelect = down;
		return;
	case K_KP_DOWNARROW:
		if (!down)
		{
			float tscale = cg_timescale.value;

			if (tscale <= 1.1f)
			{
				if (tscale > 0.1f)
				{
					tscale -= 0.1f;
				}
			}
			else
			{
				tscale -= 1.0f;
			}
			trap_Cvar_Set("timescale", va("%f", tscale));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * tscale);
		}
		return;
	case K_MWHEELDOWN:
		if (!(cgs.fKeyPressed[K_RSHIFT] || cgs.fKeyPressed[K_LSHIFT]))
		{
			if (!down)
			{
				CG_ZoomOut_f();
			}
			return;
		}       // Roll over into timescale changes
	case K_KP_LEFTARROW:
		if (!down && cg_timescale.value > 0.1f)
		{
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value - 0.1f));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * cg_timescale.value - 0.1f);
		}
		return;
	case K_KP_UPARROW:
		if (!down)
		{
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value + 1.0f));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * cg_timescale.value + 1.0f);
		}
		return;
	case K_MWHEELUP:
		if (!(cgs.fKeyPressed[K_RSHIFT] || cgs.fKeyPressed[K_LSHIFT]))
		{
			if (!down)
			{
				CG_ZoomIn_f();
			}
			return;
		}       // Roll over into timescale changes
	case K_KP_RIGHTARROW:
		if (!down)
		{
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value + 0.1f));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * cg_timescale.value + 0.1f);
		}
		return;
	// AVI recording controls
	case K_F1:
		if (down)
		{
			cgs.aviDemoRate = demo_avifpsF1.integer;
		}
		else
		{
			trap_Cvar_Set("cl_avidemo", demo_avifpsF1.string);
		}
		return;
	case K_F2:
		if (down)
		{
			cgs.aviDemoRate = demo_avifpsF2.integer;
		}
		else
		{
			trap_Cvar_Set("cl_avidemo", demo_avifpsF2.string);
		}
		return;
	case K_F3:
		if (down)
		{
			cgs.aviDemoRate = demo_avifpsF3.integer;
		}
		else
		{
			trap_Cvar_Set("cl_avidemo", demo_avifpsF3.string);
		}
		return;
	case K_F4:
		if (down)
		{
			cgs.aviDemoRate = demo_avifpsF4.integer;
		}
		else
		{
			trap_Cvar_Set("cl_avidemo", demo_avifpsF4.string);
		}
		return;
	case K_F5:
		if (down)
		{
			cgs.aviDemoRate = demo_avifpsF5.integer;
		}
		else
		{
			trap_Cvar_Set("cl_avidemo", demo_avifpsF5.string);
		}
		return;
	// Screenshot keys
	case K_F11:
		if (!down)
		{
			trap_SendConsoleCommand("screenshot");
		}
		return;
	case K_F12:
		if (!down)
		{
			CG_autoScreenShot_f();
		}
		return;
	default:
#ifdef FEATURE_EDV
		CG_RunBinding(key, down);
#endif // ifdef FEATURE_EDV
		break;
	}
}

#ifdef FEATURE_MULTIVIEW

extern vec4_t HUD_Border;

/**
 * @brief CG_ViewingDraw
 * @return
 */
qboolean CG_ViewingDraw()
{
	if (cg.mvTotalClients < 1)
	{
		return qfalse;

	}
	else
	{
		float      fontScale  = cg_fontScaleSP.value;
		int        y          = 146;
		int        pID        = cg.mvCurrentMainview->mvInfo & MV_PID;
		const char *viewInfo  = CG_TranslateString("Viewing");
		char       *w         = cgs.clientinfo[pID].name;
		int        charWidth  = CG_Text_Width_Ext("A", fontScale, 0, &cgs.media.limboFont2);
		int        charHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
		int        startClass = CG_Text_Width_Ext(viewInfo, fontScale, 0, &cgs.media.limboFont2) + charWidth;

		// teamflags
		if (cgs.clientinfo[pID].team == TEAM_ALLIES)
		{
			CG_DrawPic(9, y - charHeight * 2.0f - 12, 18, 12, cgs.media.alliedFlag);
		}
		else
		{
			CG_DrawPic(9, y - charHeight * 2.0f - 12, 18, 12, cgs.media.axisFlag);
		}

		CG_DrawRect_FixedBorder(8, y - charHeight * 2.0f - 13, 20, 14, 1, HUD_Border);

		CG_DrawPic(8 + startClass, y - 10, 14, 14, cgs.media.skillPics[SkillNumForClass(cgs.clientinfo[pID].cls)]);

		if (cgs.clientinfo[pID].rank > 0)
		{
			int startRank;

			startRank = CG_Text_Width_Ext(w, fontScale, 0, &cgs.media.limboFont2) + 14 + 2 * charWidth;
			CG_DrawPic(8 + startClass + startRank, y - 10, 14, 14, rankicons[cgs.clientinfo[pID].rank][cgs.clientinfo[pID].team == TEAM_AXIS ? 1 : 0][0].shader);
		}

		CG_Text_Paint_Ext(8, y, fontScale, fontScale, colorWhite, viewInfo, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(8 + startClass + 14 + charWidth, y, fontScale, fontScale, colorWhite, w, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		return qtrue;
	}
}
#endif

//#define GS_X    166
#define GS_Y    10
#define GS_W    298

/**
 * @brief CG_GameStatsDraw
 */
void CG_GameStatsDraw(void)
{
	if (cgs.gamestats.show == SHOW_OFF)
	{
		return;
	}
	else
	{
		int         i, x = (Ccg_WideX(SCREEN_WIDTH) / 2) - (GS_W / 2), y = GS_Y, h;
		gameStats_t *gs = &cgs.gamestats;

		vec4_t bgColor     = COLOR_BG;      // window
		vec4_t borderColor = COLOR_BORDER;  // window

		vec4_t bgColorTitle     = COLOR_BG_TITLE;       // titlebar
		vec4_t borderColorTitle = COLOR_BORDER_TITLE;   // titlebar

		// Main header
		int          hStyle  = 0;
		float        hScale  = 0.19f;
		float        hScaleY = 0.19f;
		fontHelper_t *hFont  = FONT_HEADER;

		// Sub header
		int          hStyle2  = 0;
		float        hScale2  = 0.16f;
		float        hScaleY2 = 0.20f;
		fontHelper_t *hFont2  = FONT_SUBHEADER;
		vec4_t       hdrColor = COLOR_TEXT;    // text

		// Text settings
		int          tStyle   = ITEM_TEXTSTYLE_SHADOWED;
		int          tSpacing = 9;      // TODO: Should derive from CG_Text_Height_Ext
		float        tScale   = 0.19f;
		fontHelper_t *tFont   = FONT_TEXT;
		vec4_t       tColor   = COLOR_TEXT; // text

		float diff = cgs.gamestats.fadeTime - cg.time;


		// FIXME: Should compute this beforehand
		h = 2 + tSpacing + 2 +                              // Header
		    2 + 2 + tSpacing + 2 +                          // Stats columns
		    1 +                                             // Stats + extra
		    tSpacing * ((gs->cWeapons > 0) ? gs->cWeapons : 1) +
		    tSpacing * ((gs->fHasStats) ? 7 : 0) +
		    ((cgs.gametype == GT_WOLF_LMS) ? 0 :
			 (
				 4 + 2 * tSpacing +                                 // Rank/XP/Skill Rating
				 1 + tSpacing +
				 4 + 2 * tSpacing +                                 // Skill columns
				 1 +                                                // Skillz
				 tSpacing * ((gs->cSkills > 0) ? gs->cSkills : 1)
		     )
		    ) +
		    5;

		// Fade-in effects
		if (diff > 0.0f)
		{
			float scale = (diff / STATS_FADE_TIME);

			if (cgs.gamestats.show == SHOW_ON)
			{
				scale = 1.0f - scale;
			}

			bgColor[3]          *= scale;
			bgColorTitle[3]     *= scale;
			borderColor[3]      *= scale;
			borderColorTitle[3] *= scale;
			hdrColor[3]         *= scale;
			tColor[3]           *= scale;

			y -= h * (1.0f - scale);

		}
		else if (cgs.gamestats.show == SHOW_SHUTDOWN)
		{
			cgs.gamestats.show = SHOW_OFF;
			return;
		}

		CG_FillRect(x, y, GS_W, h, bgColor);
		CG_DrawRect(x, y, GS_W, h, 1, borderColor);

		y += 1;

		// Header
		CG_FillRect(x + 1, y, GS_W - 2, tSpacing + 4, bgColorTitle);
		CG_DrawRect(x + 1, y, GS_W - 2, tSpacing + 4, 1, borderColorTitle);

		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, hScale, hScaleY, hdrColor, CG_TranslateString("PLAYER STATS"), 0.0f, 0, hStyle, hFont);
		y += 5;

		// Weapon stats
		CG_FillRect(x + 1, y, GS_W - 2, tSpacing + 3, bgColorTitle);
		CG_DrawRect(x + 1, y, GS_W - 2, tSpacing + 3, 1, borderColorTitle);

		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Weapon"), 0.0f, 0, hStyle2, hFont2);
		x += 72;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Accuracy"), 0.0f, 0, hStyle2, hFont2);
		x += 49;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Hits / Shots"), 0.0f, 0, hStyle2, hFont2);
		x += 59;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Kills"), 0.0f, 0, hStyle2, hFont2);
		x += 31;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Deaths"), 0.0f, 0, hStyle2, hFont2);
		x += 37;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Headshots"), 0.0f, 0, hStyle2, hFont2);

		x  = (Ccg_WideX(SCREEN_WIDTH) / 2) - (GS_W / 2);
		y += 2;

		if (gs->cWeapons == 0)
		{
			y += tSpacing;
			CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, CG_TranslateString("No weapon info available."), 0.0f, 0, tStyle, tFont);
		}
		else
		{
			for (i = 0; i < gs->cWeapons; i++)
			{
				y += tSpacing;
				CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, gs->strWS[i], 0.0f, 0, tStyle, tFont);
			}

			if (gs->fHasStats)
			{
				y += tSpacing;
				for (i = 0; i < 6; i++)
				{
					y += tSpacing;
					CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, gs->strExtra[i], 0.0f, 0, tStyle, tFont);
				}
			}
		}

		// No rank/xp/skill info for LMS
		if (cgs.gametype == GT_WOLF_LMS)
		{
			return;
		}

		// Rank/XP info
		y += 2 + tSpacing;
		CG_FillRect(x + 1, y, GS_W - 2, tSpacing + 3, bgColorTitle);
		CG_DrawRect(x + 1, y, GS_W - 2, tSpacing + 3, 1, borderColorTitle);

		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Rank"), 0.0f, 0, hStyle2, hFont2);
		x += 120;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, "XP", 0.0f, 0, hStyle2, hFont2);
#ifdef FEATURE_RATING
		if (cgs.skillRating && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
		{
			x += 52;
			CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, "Skill Rating", 0.0f, 0, hStyle2, hFont2);
		}
#endif
#ifdef FEATURE_PRESTIGE
		if (cgs.prestige && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS && cgs.gametype != GT_WOLF_CAMPAIGN)
		{
			x += 85;
			CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, "Prestige", 0.0f, 0, hStyle2, hFont2);
		}
#endif

		x  = (Ccg_WideX(SCREEN_WIDTH) / 2) - (GS_W / 2);
		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, gs->strRank, 0.0f, 0, tStyle, tFont);

		// Skill info
		y += 2 + tSpacing;
		CG_FillRect(x + 1, y, GS_W - 2, tSpacing + 3, bgColorTitle);
		CG_DrawRect(x + 1, y, GS_W - 2, tSpacing + 3, 1, borderColorTitle);

		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Skills"), 0.0f, 0, hStyle2, hFont2);
		x += 86;
		CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Level"), 0.0f, 0, hStyle2, hFont2);
		x += 74;
#ifdef FEATURE_PRESTIGE
		if (cgs.prestige && cgs.gametype != GT_WOLF_CAMPAIGN && cgs.gametype != GT_WOLF_STOPWATCH && cgs.gametype != GT_WOLF_LMS)
		{
			CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("XP (Total / Next Level)"), 0.0f, 0, hStyle2, hFont2);
		}
		else
#endif
		{
			CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("XP / Next Level"), 0.0f, 0, hStyle2, hFont2);
		}
		if (cgs.gametype == GT_WOLF_CAMPAIGN)
		{
			x += 102;
			CG_Text_Paint_Ext(x + 4, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Medals"), 0.0f, 0, hStyle2, hFont2);
		}

		x  = (Ccg_WideX(SCREEN_WIDTH) / 2) - (GS_W / 2);
		y += 1;

		if (gs->cSkills == 0)
		{
			y += tSpacing;
			CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, CG_TranslateString("No skills acquired!"), 0.0f, 0, tStyle, tFont);
		}
		else
		{
			for (i = 0; i < gs->cSkills; i++)
			{
				y += tSpacing;
				CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, gs->strSkillz[i], 0.0f, 0, tStyle, tFont);
			}
		}
	}
}

#define TS_X    -80     // spacing from right
#define TS_Y    -60     // spacing from bottom
#define TS_W    396     // was 308

/**
 * @brief CG_TopShotsDraw
 */
void CG_TopShotsDraw(void)
{
	if (cgs.topshots.show == SHOW_OFF)
	{
		return;
	}
	else
	{
		int            x = Ccg_WideX(SCREEN_WIDTH) + TS_X - TS_W, y = SCREEN_HEIGHT, h;
		topshotStats_t *ts              = &cgs.topshots;
		vec4_t         bgColor          = COLOR_BG; // window
		vec4_t         borderColor      = COLOR_BORDER; // window
		vec4_t         bgColorTitle     = COLOR_BG_TITLE; // titlebar
		vec4_t         borderColorTitle = COLOR_BORDER_TITLE; // titlebar

		// Main header
		int          hStyle  = 0;
		float        hScale  = 0.19f;
		float        hScaleY = 0.19f;
		fontHelper_t *hFont  = FONT_HEADER;

		// Sub header
		int          hStyle2  = 0;
		float        hScale2  = 0.16f;
		float        hScaleY2 = 0.20f;
		fontHelper_t *hFont2  = FONT_SUBHEADER;
		vec4_t       hdrColor = COLOR_TEXT;       // text

		// Text settings
		int          tStyle   = ITEM_TEXTSTYLE_SHADOWED;
		int          tSpacing = 9;      // Should derive from CG_Text_Height_Ext
		float        tScale   = 0.19f;
		fontHelper_t *tFont   = FONT_TEXT;
		vec4_t       tColor   = COLOR_TEXT; // text

		float diff = cgs.topshots.fadeTime - cg.time;


		// FIXME: Should compute this beforehand
		h = 2 + tSpacing + 2 +                                  // Header
		    2 + 2 + tSpacing + 2 +                          // Stats columns
		    1 +                                             // Stats + extra
		    tSpacing * ((ts->cWeapons > 0) ? ts->cWeapons : 1) +
		    5;


		// Fade-in effects
		if (diff > 0.0f)
		{
			float scale = (diff / STATS_FADE_TIME);

			if (cgs.topshots.show == SHOW_ON)
			{
				scale = 1.0f - scale;
			}

			bgColor[3]          *= scale;
			bgColorTitle[3]     *= scale;
			borderColor[3]      *= scale;
			borderColorTitle[3] *= scale;
			hdrColor[3]         *= scale;
			tColor[3]           *= scale;

			y += (TS_Y - h) * scale;

		}
		else if (cgs.topshots.show == SHOW_SHUTDOWN)
		{
			cgs.topshots.show = SHOW_OFF;
			return;
		}
		else
		{
			y += TS_Y - h;
		}

		CG_FillRect(x, y, TS_W, h, bgColor);
		CG_DrawRect(x, y, TS_W, h, 1, borderColor);

		y += 1;

		// Header
		CG_FillRect(x + 1, y, TS_W - 2, tSpacing + 4, bgColorTitle);
		CG_DrawRect(x + 1, y, TS_W - 2, tSpacing + 4, 1, borderColorTitle);

		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, hScale, hScaleY, hdrColor, CG_TranslateString("\"TOPSHOT\" ACCURACIES"), 0.0f, 0, hStyle, hFont);

		// Weapon stats
		y += 5;
		CG_FillRect(x + 1, y, TS_W - 2, tSpacing + 3, bgColorTitle);
		CG_DrawRect(x + 1, y, TS_W - 2, tSpacing + 3, 1, borderColorTitle);

		x += 4;
		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Weapon"), 0.0f, 0, hStyle2, hFont2);
		x += 66;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Accuracy"), 0.0f, 0, hStyle2, hFont2);
		x += 47;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Hits / Shots"), 0.0f, 0, hStyle2, hFont2);
		x += 62;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Kills"), 0.0f, 0, hStyle2, hFont2);
		x += 31;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Deaths"), 0.0f, 0, hStyle2, hFont2);
		x += 37;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("HeadShots"), 0.0f, 0, hStyle2, hFont2);
		x += 52;
		CG_Text_Paint_Ext(x, y, hScale2, hScaleY2, hdrColor, CG_TranslateString("Player"), 0.0f, 0, hStyle2, hFont2);

		x  = Ccg_WideX(SCREEN_WIDTH) + TS_X - TS_W + 4;
		y += 2;

		if (ts->cWeapons == 0)
		{
			y += tSpacing;
			CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, CG_TranslateString("No qualifying weapon info available."), 0.0f, 0, tStyle, tFont);
		}
		else
		{
			int i;

			for (i = 0; i < ts->cWeapons; i++)
			{
				y += tSpacing;
				CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, ts->strWS[i], 0.0f, 0, tStyle, tFont);
			}
		}
	}
}

#define OBJ_X   -80     // spacing from right
//#define OBJ_Y   -60     // spacing from bottom
#define OBJ_W   308

/**
 * @brief CG_ObjectivesDraw
 */
void CG_ObjectivesDraw()
{
	const char *cs;
	char       color[3];

	if (cgs.objectives.show == SHOW_OFF)
	{
		return;
	}
	else
	{
		int    i, status, x = Ccg_WideX(SCREEN_WIDTH) + OBJ_X - OBJ_W, y = SCREEN_HEIGHT, h;
		int    lines = 0, count = 0;
		char   temp[1024], *s, *p;
		vec4_t bgColor          = COLOR_BG;     // window
		vec4_t borderColor      = COLOR_BORDER; // window
		vec4_t bgColorTitle     = COLOR_BG_TITLE;   // titlebar
		vec4_t borderColorTitle = COLOR_BORDER_TITLE;   // titlebar

		// Main header
		int          hStyle   = 0;
		float        hScale   = 0.19f;
		float        hScaleY  = 0.19f;
		fontHelper_t *hFont   = FONT_HEADER;
		vec4_t       hdrColor = COLOR_TEXT;       // text

		// Text settings
		int          tStyle   = ITEM_TEXTSTYLE_SHADOWED;
		int          tSpacing = 9;      // Should derive from CG_Text_Height_Ext
		float        tScale   = 0.19f;
		fontHelper_t *tFont   = FONT_TEXT;
		vec4_t       tColor   = COLOR_TEXT; // text

		float diff = cgs.objectives.fadeTime - cg.time;

		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		{
			Q_strncpyz(temp, cg.objMapDescription_Neutral, sizeof(temp));
			while ((s = strchr(temp, '*')))
			{
				*s = '\n';
			}
			CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 8, sizeof(temp), FONT_TEXT);
			p = temp;
			while (*p)
			{
				if (*p == '\n')
				{
					*p++ = '\0';
					lines++;
				}
				else
				{
					p++;
				}
			}
			if (temp[0])
			{
				count++;
			}

			Q_strncpyz(temp, cg.objMapDescription_Allied, sizeof(temp));
			while ((s = strchr(temp, '*')))
			{
				*s = '\n';
			}
			CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 28, sizeof(temp), FONT_TEXT);
			p = temp;
			while (*p)
			{
				if (*p == '\n')
				{
					*p++ = '\0';
					lines++;
				}
				else
				{
					p++;
				}
			}
			if (temp[0])
			{
				lines += 1; // Allied
				count++;
			}

			Q_strncpyz(temp, cg.objMapDescription_Axis, sizeof(temp));
			while ((s = strchr(temp, '*')))
			{
				*s = '\n';
			}
			CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 28, sizeof(temp), FONT_TEXT);
			p = temp;
			while (*p)
			{
				if (*p == '\n')
				{
					*p++ = '\0';
					lines++;
				}
				else
				{
					p++;
				}
			}
			if (temp[0])
			{
				lines += 1; // Axis
				count++;
			}
		}
		else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES)
		{
			for (i = 0; i < MAX_OBJECTIVES && cg.objDescription_Allied[i][0] ; i++)
			{
				Q_strncpyz(temp, cg.objDescription_Allied[i], sizeof(temp));
				// no double newlines as they make the pop up look really bad
				while ((s = strstr(temp, "**")))
				{
					*s = ' ';
				}
				while ((s = strchr(temp, '*')))
				{
					*s = '\n';
				}
				CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 28, sizeof(temp), FONT_TEXT);
				p = temp;
				while (*p)
				{
					if (*p == '\n')
					{
						*p++ = '\0';
						lines++;
					}
					else
					{
						p++;
					}
				}
				if (temp[0])
				{
					count++;
				}
			}
		}
		else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
		{
			for (i = 0; i < MAX_OBJECTIVES && cg.objDescription_Axis[i][0] ; i++)
			{
				Q_strncpyz(temp, cg.objDescription_Axis[i], sizeof(temp));
				while ((s = strstr(temp, "**")))
				{
					*s = ' ';
				}
				while ((s = strchr(temp, '*')))
				{
					*s = '\n';
				}
				CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 28, sizeof(temp), FONT_TEXT);
				p = temp;
				while (*p)
				{
					if (*p == '\n')
					{
						*p++ = '\0';
						lines++;
					}
					else
					{
						p++;
					}
				}
				if (temp[0])
				{
					count++;
				}
			}
		}

		// FIXME: Should compute this beforehand
		h = 2 + tSpacing + 2 +                                  // Header
		    1 +
		    tSpacing * (((lines + count - 1) > 0) ? (lines + count - 1) : 1) +
		    2 + tSpacing;

		// Fade-in effects
		if (diff > 0.0f)
		{
			float scale = (diff / STATS_FADE_TIME);

			if (cgs.objectives.show == SHOW_ON)
			{
				scale = 1.0f - scale;
			}

			bgColor[3]          *= scale;
			bgColorTitle[3]     *= scale;
			borderColor[3]      *= scale;
			borderColorTitle[3] *= scale;
			hdrColor[3]         *= scale;
			tColor[3]           *= scale;

			y += (TS_Y - h) * scale;

		}
		else if (cgs.objectives.show == SHOW_SHUTDOWN)
		{
			cgs.objectives.show = SHOW_OFF;
			return;
		}
		else
		{
			y += TS_Y - h;
		}

		CG_FillRect(x, y, OBJ_W, h, bgColor);
		CG_DrawRect(x, y, OBJ_W, h, 1, borderColor);

		y += 1;

		// Header
		CG_FillRect(x + 1, y, OBJ_W - 2, tSpacing + 4, bgColorTitle);
		CG_DrawRect(x + 1, y, OBJ_W - 2, tSpacing + 4, 1, borderColorTitle);

		y += 1 + tSpacing;
		CG_Text_Paint_Ext(x + 4, y, hScale, hScaleY, hdrColor, CG_TranslateString("OBJECTIVES"), 0.0f, 0, hStyle, hFont);

		y += 5;

		if (!count)
		{
			y += tSpacing;
			CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, "Unable to load objectives", 0.0f, 0, tStyle, tFont);
			return;
		}

		cs = CG_ConfigString(CS_MULTI_OBJECTIVE);

		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		{
			Q_strncpyz(temp, cg.objMapDescription_Neutral, sizeof(temp));
			while ((s = strchr(temp, '*')))
			{
				*s = '\n';
			}
			CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 8, sizeof(temp), FONT_TEXT);
			s = p = temp;
			while (*p)
			{
				if (*p == '\n')
				{
					*p++ = '\0';
					y   += tSpacing;
					CG_Text_Paint_Ext(x + 4, y, tScale, tScale, tColor, s, 0.0f, 0, tStyle, tFont);
					s = p;
				}
				else
				{
					p++;
				}
			}
			if (temp[0] && count > 0)
			{
				count--;
				y += tSpacing;
			}

			Q_strncpyz(temp, cg.objMapDescription_Allied, sizeof(temp));
			while ((s = strchr(temp, '*')))
			{
				*s = '\n';
			}
			CG_DrawPic(x + 4, y + 2, 18, 12, cgs.media.alliedFlag);
			CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 26, sizeof(temp), FONT_TEXT);
			y += tSpacing;
			CG_Text_Paint_Ext(x + 26, y, tScale, tScale, tColor, "^$Allies", 0.0f, 0, tStyle, tFont);
			s = p = temp;
			while (*p)
			{
				if (*p == '\n')
				{
					*p++ = '\0';
					y   += tSpacing;
					CG_Text_Paint_Ext(x + 26, y, tScale, tScale, tColor, s, 0.0f, 0, tStyle, tFont);
					s = p;
				}
				else
				{
					p++;
				}
			}
			if (temp[0] && count > 0)
			{
				count--;
				y += tSpacing;
			}

			Q_strncpyz(temp, cg.objMapDescription_Axis, sizeof(temp));
			while ((s = strchr(temp, '*')))
			{
				*s = '\n';
			}
			CG_DrawPic(x + 4, y + 2, 18, 12, cgs.media.axisFlag);
			CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 26, sizeof(temp), FONT_TEXT);
			y += tSpacing;
			CG_Text_Paint_Ext(x + 26, y, tScale, tScale, tColor, "^1Axis", 0.0f, 0, tStyle, tFont);
			s = p = temp;
			while (*p)
			{
				if (*p == '\n')
				{
					*p++ = '\0';
					y   += tSpacing;
					CG_Text_Paint_Ext(x + 26, y, tScale, tScale, tColor, s, 0.0f, 0, tStyle, tFont);
					s = p;
				}
				else
				{
					p++;
				}
			}
			if (temp[0] && count > 0)
			{
				count--;
				//y += tSpacing;
			}
		}
		else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES)
		{
			for (i = 0; i < MAX_OBJECTIVES && cg.objDescription_Allied[i][0] ; i++)
			{
				Q_strncpyz(temp, cg.objDescription_Allied[i], sizeof(temp));
				while ((s = strstr(temp, "**")))
				{
					*s = ' ';
				}
				while ((s = strchr(temp, '*')))
				{
					*s = '\n';
				}
				CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 26, sizeof(temp), FONT_TEXT);

				color[0] = '\0';
				status   = Q_atoi(Info_ValueForKey(cs, va("a%i", i + 1)));
				if (status == 1)
				{
					CG_DrawPic(x + 4, y + 3, 18, 12, cgs.media.alliedFlag);
					Q_strncpyz(color, "^2", sizeof(color));
				}
				else if (status == 2)
				{
					CG_DrawPic(x + 4, y + 3, 18, 12, cgs.media.axisFlag);
					Q_strncpyz(color, "^1", sizeof(color));
				}

				s = p = temp;
				while (*p)
				{
					if (*p == '\n')
					{
						*p++ = '\0';
						y   += tSpacing;
						CG_Text_Paint_Ext(x + 26, y, tScale, tScale, tColor, va("%s%s", color[0] ? color : "", s), 0.0f, 0, tStyle, tFont);
						s = p;
					}
					else
					{
						p++;
					}
				}
				if (temp[0] && count > 0)
				{
					count--;
					y += tSpacing;
				}
			}
		}
		else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS)
		{
			for (i = 0; i < MAX_OBJECTIVES && cg.objDescription_Axis[i][0] ; i++)
			{
				Q_strncpyz(temp, cg.objDescription_Axis[i], sizeof(temp));
				while ((s = strstr(temp, "**")))
				{
					*s = ' ';
				}
				while ((s = strchr(temp, '*')))
				{
					*s = '\n';
				}
				CG_FitTextToWidth_Ext(temp, tScale, OBJ_W - 26, sizeof(temp), FONT_TEXT);

				color[0] = '\0';
				status   = Q_atoi(Info_ValueForKey(cs, va("x%i", i + 1)));
				if (status == 1)
				{
					CG_DrawPic(x + 4, y + 3, 18, 12, cgs.media.axisFlag);
					Q_strncpyz(color, "^2", sizeof(color));
				}
				else if (status == 2)
				{
					CG_DrawPic(x + 4, y + 3, 18, 12, cgs.media.alliedFlag);
					Q_strncpyz(color, "^1", sizeof(color));
				}

				s = p = temp;
				while (*p)
				{
					if (*p == '\n')
					{
						*p++ = '\0';
						y   += tSpacing;
						CG_Text_Paint_Ext(x + 26, y, tScale, tScale, tColor, va("%s%s", color[0] ? color : "", s), 0.0f, 0, tStyle, tFont);
						s = p;
					}
					else
					{
						p++;
					}
				}
				if (temp[0] && count > 0)
				{
					count--;
					y += tSpacing;
				}
			}
		}
	}
}

#define DH_X    -22     // spacing from right
#define DH_Y    -60     // spacing from bottom
#define DH_W    148

/**
 * @brief CG_DrawDemoControls
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] borderColor
 * @param[in] bgColor
 * @param[in] tSpacing
 * @param[in] bgColorTitle
 * @param[in] borderColorTitle
 * @param[in] hScale
 * @param[in] hScaleY
 * @param[in] hdrColor
 * @param[in] hStyle
 * @param[in] hFont
 */
void CG_DrawDemoControls(int x, int y, int w, vec4_t borderColor, vec4_t bgColor, int tSpacing, vec4_t bgColorTitle, vec4_t borderColorTitle, float hScale, float hScaleY, vec4_t hdrColor, int hStyle, fontHelper_t *hFont)
{
	static panel_button_text_t demoControlTxt;
	int                        i;

	demoControlTxt.scalex = hScale;
	demoControlTxt.scaley = hScaleY;
	Vector4Copy(hdrColor, demoControlTxt.colour);
	demoControlTxt.style = ITEM_ALIGN_CENTER;
	demoControlTxt.align = 0;
	demoControlTxt.font  = hFont;

	CG_FillRect(x, y, w, 50, bgColor);
	CG_DrawRect(x, y, w, 50, 1, borderColor);

	y += 1;

	// Header
	CG_FillRect(x + 1, y, w - 2, tSpacing + 4, bgColorTitle);
	CG_DrawRect(x + 1, y, w - 2, tSpacing + 4, 1, borderColorTitle);
	CG_Text_Paint_Ext(x + 4, y + tSpacing, hScale, hScaleY, hdrColor, CG_TranslateString("DEMO STATUS"), 0.0f, 0, hStyle, hFont);

	for (i = 0; i < 4; i++)
	{
		if (i)
		{
			RectangleSet(demoControlButtons[i]->rect, (x + (i * (w / 4)) - 15), y + 30, 30, 15);
		}
		else
		{
			RectangleSet(demoControlButtons[i]->rect, x + 2, y + 15, w - 4, 12);
		}

		demoControlButtons[i]->font = &demoControlTxt;
	}
	BG_PanelButtonsRender(demoControlButtons);

	if (cg.time < cgs.cursorUpdate)
	{
		// render cursor
		trap_R_SetColor(NULL);
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
	}
}

/**
 * @brief CG_DemoHelpDraw
 */
void CG_DemoHelpDraw(void)
{
#ifdef FEATURE_EDV
#define ONOFF(x) ((x) ? ("ON") : ("OFF"))
	const char *freecam     = ONOFF(cgs.demoCamera.renderingFreeCam);
	const char *panzercam   = ONOFF(demo_weaponcam.integer & DWC_PANZER);
	const char *mortarcam   = ONOFF(demo_weaponcam.integer & DWC_MORTAR);
	const char *grenadecam  = ONOFF(demo_weaponcam.integer & DWC_GRENADE);
	const char *dynamitecam = ONOFF(demo_weaponcam.integer & DWC_DYNAMITE);
	const char *teamonly    = ONOFF(demo_teamonlymissilecam.integer);
	const char *pvshint     = ONOFF(demo_pvshint.integer);
	const char *playerNames;

	if (cg_drawSpectatorNames.integer == DEMO_CLEANNAME)
	{
		playerNames = "   Clean";
	}
	else if (cg_drawSpectatorNames.integer == DEMO_COLOREDNAME)
	{
		playerNames = "Coloured";
	}
	else
	{
		playerNames = "     OFF";
	}

#endif

	if (cg.demohelpWindow == SHOW_OFF)
	{
		return;
	}
	else
	{
		const char *help[] =
		{
			"^7TAB       ^3scores",
			"^7F1-F5     ^3avidemo record",
			"^7F11-F12   ^3screenshot",
			NULL,
			"^7KP_DOWN   ^3slow down (--)",
			"^7KP_LEFT   ^3slow down (-)",
			"^7KP_UP     ^3speed up (++)",
			"^7KP_RIGHT  ^3speed up (+)",
			"^7KP_5      ^3normal speed",
			"^7SPACE     ^3pause demo",
			NULL,
			"^7ENTER     ^3External view",
			"^7LFT/RGHT  ^3Change angle",
#ifdef FEATURE_EDV
			"^nUP/DOWN   ^mMove in/out",
			NULL,
			//"^nKP_ENTER  ^mToggle freecam"
			"^nALT       ^mmore options"
#else
			"^nUP/DOWN   ^mMove in/out"
#endif

		};

#ifdef FEATURE_EDV
		const char *edvhelp[] =
		{
			va("^nKP_ENTER  ^mFreecam    ^m%s", freecam),
			va("^nCTRL      ^mPvshint    ^m%s", pvshint),
			NULL,
			va("^nDEL       ^mGrenadecam ^m%s", grenadecam),
			va("^nHOME      ^mPanzercam  ^m%s", panzercam),
			va("^nEND       ^mDynacam    ^m%s", dynamitecam),
			va("^nINS       ^mMortarcam  ^m%s", mortarcam),
			va("^nPGDOWN    ^mTeamonly   ^m%s", teamonly),
			NULL,
			va("^nF6        ^mNames ^m%s",      playerNames),
		};
#endif

#ifdef FEATURE_MULTIVIEW
		const char *mvhelp[] =
		{
			NULL,
			"^7MOUSE1    ^3Select/move view",
			"^7MOUSE2    ^3Swap w/main view",
			"^7MOUSE3    ^3Toggle on/off",
			"^7SHIFT     ^3Hold to resize",
			"^7KP_PGUP   ^3Enable a view",
			"^7KP_PGDN   ^3Close a view"
		};
#endif
		unsigned int i;
		int          x, y = SCREEN_HEIGHT, w, h;

		vec4_t bgColor     = COLOR_BG;              // window
		vec4_t borderColor = COLOR_BORDER;          // window

		vec4_t bgColorTitle     = COLOR_BG_TITLE;       // titlebar
		vec4_t borderColorTitle = COLOR_BORDER_TITLE;   // titlebar

		// Main header
		int          hStyle   = 0;
		float        hScale   = 0.19f;
		float        hScaleY  = 0.19f;
		fontHelper_t *hFont   = FONT_HEADER;
		vec4_t       hdrColor = COLOR_TEXT;  // text

		// Text settings
		int          tStyle   = ITEM_TEXTSTYLE_SHADOWED;
		int          tSpacing = 9;      // Should derive from CG_Text_Height_Ext
		float        tScale   = 0.19f;
		fontHelper_t *tFont   = FONT_TEXT;
		vec4_t       tColor   = COLOR_TEXT; // text

		float diff = cg.fadeTime - trap_Milliseconds();

#ifdef FEATURE_EDV
		mlType_t menuLevel = cgs.currentMenuLevel;
#endif

// the ifdef's are very painfull here
#ifdef FEATURE_EDV
		// FIXME: Should compute this beforehand
		w = DH_W + (
#ifdef FEATURE_MULTIVIEW
			(cg.mvTotalClients > 1) ? 12 :
#endif
			0);
		x = Ccg_WideX(SCREEN_WIDTH) + 3 * DH_X - w;

		if (menuLevel == ML_MAIN)
		{
			h = tSpacing + 9 +
			    tSpacing * (2 +
#ifdef FEATURE_MULTIVIEW
			                ((cg.mvTotalClients > 1) ? ARRAY_LEN(mvhelp) : ARRAY_LEN(help))
#else
			                ARRAY_LEN(help)
#endif
			                );
		}
		else //if(menuLevel == ML_EDV)
		{
			h = tSpacing + 9 +
			    tSpacing * (2 +
#ifdef FEATURE_MULTIVIEW
			                ((cg.mvTotalClients > 1) ? ARRAY_LEN(mvhelp) : ARRAY_LEN(edvhelp))
#else
			                ARRAY_LEN(edvhelp)
#endif
			                );
		}

#else
		// FIXME: Should compute this beforehand
		w = DH_W + (
#ifdef FEATURE_MULTIVIEW
			(cg.mvTotalClients > 1) ? 12 :
#endif
			0);
		x = Ccg_WideX(SCREEN_WIDTH) + 3 * DH_X - w;
		h = tSpacing + 9 +
		    tSpacing * (2 +
#ifdef FEATURE_MULTIVIEW
		                ((cg.mvTotalClients > 1) ? ARRAY_LEN(mvhelp) : ARRAY_LEN(help))
#else
		                ARRAY_LEN(help)
#endif
		                );

#endif

		// Fade-in effects
		if (diff > 0.0f)
		{
			float scale = (diff / STATS_FADE_TIME);

			if (cg.demohelpWindow == SHOW_ON)
			{
				scale = 1.0f - scale;
			}

			bgColor[3]          *= scale;
			bgColorTitle[3]     *= scale;
			borderColor[3]      *= scale;
			borderColorTitle[3] *= scale;
			hdrColor[3]         *= scale;
			tColor[3]           *= scale;

			y += (DH_Y - h) * scale;

		}
		else if (cg.demohelpWindow == SHOW_SHUTDOWN)
		{
			cg.demohelpWindow = SHOW_OFF;
			return;
		}
		else
		{
			y += DH_Y - h;
		}

		if (cg.etLegacyClient && cg.demoinfo)
		{
			CG_DrawDemoControls(x, y - 62, w, borderColor, bgColor, tSpacing, bgColorTitle, borderColorTitle, hScale, hScaleY, hdrColor, hStyle, hFont);
			y += 10;
		}

		CG_FillRect(x, y, w, h, bgColor);
		CG_DrawRect(x, y, w, h, 1, borderColor);

		y += 1;

		// Header
		CG_FillRect(x + 1, y, w - 2, tSpacing + 4, bgColorTitle);
		CG_DrawRect(x + 1, y, w - 2, tSpacing + 4, 1, borderColorTitle);

		x += 4;
		y += tSpacing;

		CG_Text_Paint_Ext(x, y, hScale, hScaleY, hdrColor, CG_TranslateString("DEMO CONTROLS"), 0.0f, 0, hStyle, hFont);

		y += 3;

#ifdef FEATURE_EDV
		if (menuLevel == ML_MAIN)
		{
#endif

		// Control info
		for (i = 0; i < ARRAY_LEN(help); i++)
		{
			y += tSpacing;
			if (help[i] != NULL)
			{
				CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, help[i], 0.0f, 0, tStyle, tFont);
			}
		}
#ifdef FEATURE_EDV
	}
	else if (menuLevel == ML_EDV)
	{
		// Control info
		for (i = 0; i < ARRAY_LEN(edvhelp); i++)
		{
			y += tSpacing;
			if (help[i] != NULL)
			{
				CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, edvhelp[i], 0.0f, 0, tStyle, tFont);
			}
		}
	}
#endif

#ifdef FEATURE_MULTIVIEW
		if (cg.mvTotalClients > 1)
		{
			for (i = 0; i < ARRAY_LEN(mvhelp); i++)
			{
				y += tSpacing;
				if (mvhelp[i] != NULL)
				{
					CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, mvhelp[i], 0.0f, 0, tStyle, tFont);
				}
			}
		}
#endif

		y += tSpacing * 2;
#ifdef FEATURE_EDV
		if (menuLevel == ML_MAIN)
		{
#endif
		CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, CG_TranslateString("^7BACKSPACE ^3help on/off"), 0.0f, 0, tStyle, tFont);
#ifdef FEATURE_EDV
	}
	else if (menuLevel == ML_EDV)
	{
		CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, CG_TranslateString("^7BACKSPACE ^mgo  back"), 0.0f, 0, tStyle, tFont);
	}
#endif

	}
}

/**
 * @brief CG_getBindKeyName
 * @param[in] cmd
 * @param[out] buf
 * @param[in] len
 * @return
 */
char *CG_getBindKeyName(const char *cmd, char *buf, size_t len)
{
	int j;

	for (j = 0; j < 256; j++)
	{
		trap_Key_GetBindingBuf(j, buf, len);
		if (*buf == 0)
		{
			continue;
		}

		if (!Q_stricmp(buf, cmd))
		{
			trap_Key_KeynumToStringBuf(j, buf, MAX_STRING_TOKENS);
			Q_strupr(buf);
			return(buf);
		}
	}

	Q_strncpyz(buf, va("(%s)", cmd), len);
	return(buf);
}

#ifdef FEATURE_MULTIVIEW
typedef struct
{
	const char *cmd;
	const char *info;
} helpType_t;

#define SH_X    8       ///< spacing from left
#define SH_Y    155     ///< spacing from top

/**
 * @brief CG_SpecHelpDraw
 */
void CG_SpecHelpDraw(void)
{
	if (cg.spechelpWindow == SHOW_OFF)
	{
		return;
	}
	else
	{
		const helpType_t help[] =
		{
			{ "+zoom",    "hold for pointer"   },
			{ "+attack",  "window move/resize" },
			{ "+sprint",  "hold to resize"     },
			{ "weapnext", "window on/off"      },
			{ "weapprev", "swap w/main view"   },
			{ NULL,       NULL                 },
			{ "weapalt",  "swingcam toggle"    },
			{ "spechelp", "help on/off"        }
		};

		unsigned int i;
		int x, y = SCREEN_HEIGHT, w, h;
		int len, maxlen = 0;
		char format[MAX_STRING_TOKENS], buf[MAX_STRING_TOKENS];
		char *lines[16];

		vec4_t bgColor     = COLOR_BG;              // window
		vec4_t borderColor = COLOR_BORDER;          // window

		vec4_t bgColorTitle     = COLOR_BG_TITLE;       // titlebar
		vec4_t borderColorTitle = COLOR_BORDER_TITLE;   // titlebar

		// Main header
		int hStyle          = 0;
		float hScale        = 0.19f;
		float hScaleY       = 0.19f;
		fontHelper_t *hFont = FONT_HEADER;
		vec4_t hdrColor     = COLOR_TEXT;    // text

		// Text settings
		int tStyle          = ITEM_TEXTSTYLE_SHADOWED;
		int tSpacing        = 9;        // Should derive from CG_Text_Height_Ext
		float tScale        = 0.19f;
		fontHelper_t *tFont = FONT_TEXT;
		vec4_t tColor       = COLOR_TEXT;   // text

		float diff = cg.fadeTime - trap_Milliseconds();

		// FIXME: Should compute all this stuff beforehand
		// Compute required width
		for (i = 0; i < sizeof(help) / sizeof(helpType_t); i++)
		{
			if (help[i].cmd != NULL)
			{
				len = (int)strlen(CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)));
				if (len > maxlen)
				{
					maxlen = len;
				}
			}
		}

		Q_strncpyz(format, va("^7%%%ds ^3%%s", maxlen), sizeof(format));
		for (i = 0, maxlen = 0; i < sizeof(help) / sizeof(helpType_t); i++)
		{
			if (help[i].cmd != NULL)
			{
				lines[i] = va(format, CG_getBindKeyName(help[i].cmd, buf, sizeof(buf)), help[i].info);
				len      = CG_Text_Width_Ext(lines[i], tScale, 0, FONT_TEXT);
				if (len > maxlen)
				{
					maxlen = len;
				}
			}
			else
			{
				lines[i] = NULL;
			}
		}

		w = maxlen + 8;
		x = SH_X;
		y = SH_Y;
		h = 2 + tSpacing + 2 +                                  // Header
		    2 + 1 +
		    tSpacing * (sizeof(help) / sizeof(helpType_t)) +
		    2;

		// Fade-in effects
		if (diff > 0.0f)
		{
			float scale = (diff / STATS_FADE_TIME);

			if (cg.spechelpWindow == SHOW_ON)
			{
				scale = 1.0f - scale;
			}

			bgColor[3]          *= scale;
			bgColorTitle[3]     *= scale;
			borderColor[3]      *= scale;
			borderColorTitle[3] *= scale;
			hdrColor[3]         *= scale;
			tColor[3]           *= scale;

			x -= w * (1.0f - scale);
		}
		else if (cg.spechelpWindow == SHOW_SHUTDOWN)
		{
			cg.spechelpWindow = SHOW_OFF;
			return;
		}

		CG_FillRect(x, y, w, h, bgColor);
		CG_DrawRect(x, y, w, h, 1, borderColor);

		y += 1;

		// Header
		CG_FillRect(x + 1, y, w - 2, tSpacing + 4, bgColorTitle);
		CG_DrawRect(x + 1, y, w - 2, tSpacing + 4, 1, borderColorTitle);

		x += 4;
		y += tSpacing;

		CG_Text_Paint_Ext(x, y, hScale, hScaleY, hdrColor, CG_TranslateString("SPECTATOR CONTROLS"), 0.0f, 0, hStyle, hFont);

		y += 3;

		// Control info
		for (i = 0; i < sizeof(help) / sizeof(helpType_t); i++)
		{
			y += tSpacing;
			if (lines[i] != NULL)
			{
				CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, lines[i], 0.0f, 0, tStyle, tFont);
			}
		}
	}
}
#endif

/**
 * @brief CG_DrawOverlays
 */
void CG_DrawOverlays(void)
{
	CG_GameStatsDraw();
	CG_TopShotsDraw();
	CG_ObjectivesDraw();
#ifdef FEATURE_MULTIVIEW
	CG_SpecHelpDraw();
#endif
#ifdef FEATURE_EDV
	if (cg.demoPlayback && cg_predefineddemokeys.integer)
#else
	if (cg.demoPlayback)
#endif
	{
		CG_DemoHelpDraw();
	}
}
