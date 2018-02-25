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
 * @file cg_window.c
 * @brief cgame window handling
 */

#include "cg_local.h"

#ifdef FEATURE_MULTIVIEW
extern pmove_t cg_pmove;        // cg_predict.c
#endif

vec4_t colorGreen2 = { 0.305f, 0.475f, 0.305f, 0.48f }; // Slightly off from default fill

/**
 * @brief CG_createStatsWindow
 */
void CG_createStatsWindow(void)
{
	cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING | WFX_FADEIN | /*WFX_SCROLLUP|*/ WFX_TRUETYPE, 110);

	cg.statsWindow = sw;
	if (sw == NULL)
	{
		return;
	}

	// Window specific
	sw->id         = WID_STATS;
	sw->fontScaleX = cf_wstats.value * 0.2f;
	sw->fontScaleY = cf_wstats.value * 0.2f;
	sw->x          = (cg.snap->ps.pm_type == PM_INTERMISSION) ?  10 : 4;
	sw->y          = (cg.snap->ps.pm_type == PM_INTERMISSION) ? -20 : -100; // Align from bottom minus offset and height
}

/**
 * @brief CG_createTopShotsWindow
 * @note Unused
 */
void CG_createTopShotsWindow(void)
{
	cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING | WFX_FLASH | WFX_FADEIN | WFX_SCROLLUP | WFX_TRUETYPE, 190);

	cg.topshotsWindow = sw;
	if (sw == NULL)
	{
		return;
	}

	// Window specific
	sw->id            = WID_TOPSHOTS;
	sw->fontScaleX    = cf_wtopshots.value * 0.2f;
	sw->fontScaleY    = cf_wtopshots.value * 0.2f;
	sw->x             = (cg.snap->ps.pm_type == PM_INTERMISSION) ? -10 : -20;
	sw->y             = (cg.snap->ps.pm_type == PM_INTERMISSION) ? -20 : -60; // Align from bottom minus offset and height
	sw->flashMidpoint = sw->flashPeriod * 0.8f;
	Com_Memcpy(&sw->colorBackground2, &colorGreen2, sizeof(vec4_t));
}

/**
 * @brief CG_createMOTDWindow
 *
 * @todo TODO: dynamic game server MOTD window, see CG_DrawInformation()
 */
void CG_createMOTDWindow(void)
{
	const char *str = CG_ConfigString(CS_CUSTMOTD + 0);

	if (str != NULL && *str != 0)
	{
		int         i;
		cg_window_t *sw = CG_windowAlloc(WFX_TEXTSIZING | WFX_FADEIN, 500);

		cg.motdWindow = sw;
		if (sw == NULL)
		{
			return;
		}

		// Window specific
		sw->id            = WID_MOTD;
		sw->fontScaleX    = 1.0f;
		sw->fontScaleY    = 1.0f;
		sw->x             = 10;
		sw->y             = -36;
		sw->flashMidpoint = sw->flashPeriod * 0.8f;
		Com_Memcpy(&sw->colorBackground2, &colorGreen2, sizeof(vec4_t));

		// Copy all MOTD info into the window
		cg.windowCurrent = sw;
		for (i = 0; i < MAX_MOTDLINES; i++)
		{
			str = CG_ConfigString(CS_CUSTMOTD + i);
			if (str != NULL && *str != 0)
			{
				CG_printWindow(str);
			}
			else
			{
				break;
			}
		}
	}
}

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//      WINDOW HANDLING AND PRIMITIVES
//
//////////////////////////////////////////////
//////////////////////////////////////////////


/**
 * @brief Windowing system setup
 */
void CG_windowInit(void)
{
	int i;

	cg.winHandler.numActiveWindows = 0;
	for (i = 0; i < MAX_WINDOW_COUNT; i++)
	{
		cg.winHandler.window[i].inuse = qfalse;
	}

	cg.msgWstatsWindow    = NULL;
	cg.msgWtopshotsWindow = NULL;
	cg.statsWindow        = NULL;
	cg.topshotsWindow     = NULL;
}

/**
 * @brief Window stuct "constructor" with some common defaults
 * @param[in] w
 * @param[out] fx
 * @param[out] startupLength
 */
void CG_windowReset(cg_window_t *w, int fx, int startupLength)
{
	vec4_t colorGeneralBorder = { 0.5f, 0.35f, 0.25f, 0.5f };
	vec4_t colorGeneralFill   = { 0.3f, 0.45f, 0.3f, 0.5f };

	w->effects       = fx;
	w->fontScaleX    = 0.25;
	w->fontScaleY    = 0.25;
	w->flashPeriod   = 1000;
	w->flashMidpoint = w->flashPeriod / 2;
	w->id            = WID_NONE;
	w->inuse         = qtrue;
	w->lineCount     = 0;
	w->state         = (fx >= WFX_FADEIN) ? WSTATE_START : WSTATE_COMPLETE;
	w->targetTime    = (startupLength > 0) ? startupLength : 0;
	w->time          = trap_Milliseconds();
	w->x             = 0;
	w->y             = 0;

	Com_Memcpy(&w->colorBorder, &colorGeneralBorder, sizeof(vec4_t));
	Com_Memcpy(&w->colorBackground, &colorGeneralFill, sizeof(vec4_t));
}

/**
 * @brief Reserve a window
 * @param[in] fx
 * @param[in] startupLength
 * @return
 */
cg_window_t *CG_windowAlloc(int fx, int startupLength)
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	if (wh->numActiveWindows >= MAX_WINDOW_COUNT)
	{
		return NULL;
	}

	for (i = 0; i < MAX_WINDOW_COUNT; i++)
	{
		w = &wh->window[i];
		if (w->inuse == qfalse)
		{
			CG_windowReset(w, fx, startupLength);
			wh->activeWindows[wh->numActiveWindows++] = i;
			return(w);
		}
	}

	// Fail if we're a full airplane
	return NULL;
}

/**
 * @brief Free up a window reservation
 * @param[in,out] w
 */
void CG_windowFree(cg_window_t *w)
{
	int                i, j;
	cg_windowHandler_t *wh = &cg.winHandler;

	if (w == NULL)
	{
		return;
	}

	if (w->effects >= WFX_FADEIN && w->state != WSTATE_OFF && w->inuse == qtrue)
	{
		w->state = WSTATE_SHUTDOWN;
		w->time  = trap_Milliseconds();
		return;
	}

	for (i = 0; i < wh->numActiveWindows; i++)
	{
		if (w == &wh->window[wh->activeWindows[i]])
		{
			for (j = i; j < wh->numActiveWindows; j++)
			{
				if (j + 1 < wh->numActiveWindows)
				{
					wh->activeWindows[j] = wh->activeWindows[j + 1];
				}
			}

			w->id    = WID_NONE;
			w->inuse = qfalse;
			w->state = WSTATE_OFF;

			CG_removeStrings(w);

			wh->numActiveWindows--;

			break;
		}
	}
}

/**
 * @brief CG_windowCleanup
 */
void CG_windowCleanup(void)
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	for (i = 0; i < wh->numActiveWindows; i++)
	{
		w = &wh->window[wh->activeWindows[i]];
		if (!w->inuse || w->state == WSTATE_OFF)
		{
			CG_windowFree(w);
			i--;
		}
	}
}

/**
 * @brief CG_demoAviFPSDraw
 */
void CG_demoAviFPSDraw(void)
{
	qboolean fKeyDown = (qboolean)(cgs.fKeyPressed[K_F1] | cgs.fKeyPressed[K_F2] | cgs.fKeyPressed[K_F3] | cgs.fKeyPressed[K_F4] | cgs.fKeyPressed[K_F5]);

	if (cg.demoPlayback && fKeyDown && cgs.aviDemoRate >= 0)
	{
		CG_Text_Paint_Ext(42, 425, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, ((cgs.aviDemoRate > 0) ? va("^3Record AVI @ ^7%d^2fps", cgs.aviDemoRate) : "^1Stop AVI Recording"), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}
}

/**
 * @brief CG_demoTimescaleDraw
 */
void CG_demoTimescaleDraw(void)
{
	if (cg.demoPlayback && cgs.timescaleUpdate > cg.time && demo_drawTimeScale.integer != 0)
	{
		vec4_t bgColor = { 0.0f, 0.0f, 0.0f, 0.6f };
		vec4_t bdColor = { 0.5f, 0.5f, 0.5f, 0.5f };

		char *s = va("^7Time Scale: ^3%.1fx", (double)cg_timescale.value);
		int  h  = CG_Text_Height_Ext("A", cg_fontScaleSP.value, 0, &cgs.media.limboFont2);
		int  w  = CG_Text_Width_Ext(s, cg_fontScaleSP.value, 0, &cgs.media.limboFont2);
		int  x  = (int)(Ccg_WideX(SCREEN_WIDTH) - w - 108);

		CG_FillRect(x, SCREEN_HEIGHT - 21, w + 7, h * 2.5f, bgColor);
		CG_DrawRect(x, SCREEN_HEIGHT - 21, w + 7, h * 2.5f, 1, bdColor);
		CG_Text_Paint_Ext(x + 3, SCREEN_HEIGHT - 10, cg_fontScaleSP.value, cg_fontScaleSP.value, colorWhite, s, 0, 0, 0, &cgs.media.limboFont2);
	}
}

/**
 * @brief Main window-drawing handler
 */
void CG_windowDraw(void)
{
	int         h, x, y, i, j, milli, t_offset, tmp;
	cg_window_t *w;
	qboolean    fCleanup = qfalse;
#ifdef FEATURE_MULTIVIEW
	qboolean fAllowMV = (qboolean)(cg.snap != NULL && cg.snap->ps.pm_type != PM_INTERMISSION && cgs.mvAllowed);
#endif
	vec4_t *bg;
	vec4_t textColor, borderColor, bgColor;

	if (cg.winHandler.numActiveWindows == 0)
	{
		// Draw these for demoplayback no matter what
		CG_demoAviFPSDraw();
		CG_demoTimescaleDraw();
		return;
	}

	milli = trap_Milliseconds();
	Com_Memcpy(textColor, colorWhite, sizeof(vec4_t));

#ifdef FEATURE_MULTIVIEW
	// Mouse cursor position for MV highlighting (offset for cursor pointer position)
	// Also allow for swingcam toggling
	if (cg.mvTotalClients > 0 && fAllowMV)
	{
		CG_cursorUpdate();
	}
#endif

	for (i = 0; i < cg.winHandler.numActiveWindows; i++)
	{
		w = &cg.winHandler.window[cg.winHandler.activeWindows[i]];

		if (!w->inuse || w->state == WSTATE_OFF)
		{
			fCleanup = qtrue;
			continue;
		}

#ifdef FEATURE_MULTIVIEW
		// Multiview rendering has its own handling
		if (w->effects & WFX_MULTIVIEW)
		{
			if (w != cg.mvCurrentMainview && fAllowMV)
			{
				CG_mvDraw(w);
			}
			continue;
		}
#endif

		if (w->effects & WFX_TEXTSIZING)
		{
			CG_windowNormalizeOnText(w);
			w->effects &= ~WFX_TEXTSIZING;
		}

		bg = ((w->effects & WFX_FLASH) && (milli % w->flashPeriod) > w->flashMidpoint) ? &w->colorBackground2 : &w->colorBackground;

		h            = (int)w->h;
		x            = (int)w->x;
		y            = (int)w->y;
		t_offset     = milli - w->time;
		textColor[3] = 1.0f;
		Com_Memcpy(&borderColor, w->colorBorder, sizeof(vec4_t));
		Com_Memcpy(&bgColor, bg, sizeof(vec4_t));

		// TODO: Add in support for ALL scrolling effects
		if (w->state == WSTATE_START)
		{
			tmp = w->targetTime - t_offset;
			if (w->effects & WFX_SCROLLUP)
			{
				if (tmp > 0)
				{
					y += (SCREEN_HEIGHT - y) * tmp / w->targetTime;   //(100 * tmp / w->targetTime) / 100;
				}
				else
				{
					w->state = WSTATE_COMPLETE;
				}

				w->curY = y;
			}
			if (w->effects & WFX_FADEIN)
			{
				if (tmp > 0)
				{
					textColor[3] = ((float)t_offset / (float)w->targetTime);
				}
				else
				{
					w->state = WSTATE_COMPLETE;
				}
			}
		}
		else if (w->state == WSTATE_SHUTDOWN)
		{
			tmp = w->targetTime - t_offset;
			if (w->effects & WFX_SCROLLUP)
			{
				if (tmp > 0)
				{
					y = (int)(w->curY + (SCREEN_HEIGHT - w->y) * t_offset / w->targetTime);        //(100 * t_offset / w->targetTime) / 100;
				}
				if (tmp < 0 || y >= SCREEN_HEIGHT)
				{
					w->state = WSTATE_OFF;
					fCleanup = qtrue;
					continue;
				}
			}
			if (w->effects & WFX_FADEIN)
			{
				if (tmp > 0)
				{
					textColor[3] -= ((float)t_offset / (float)w->targetTime);
				}
				else
				{
					textColor[3] = 0.0f;
					w->state     = WSTATE_OFF;
				}
			}
		}

		borderColor[3] *= textColor[3];
		bgColor[3]     *= textColor[3];

		CG_FillRect(x, y, w->w, h, bgColor);
		CG_DrawRect(x, y, w->w, h, 1, borderColor);

		x += 5;
		y -= (w->effects & WFX_TRUETYPE) ? 3 : 0;

		for (j = w->lineCount - 1; j >= 0; j--)
		{
			if (w->effects & WFX_TRUETYPE)
			{
				CG_Text_Paint_Ext(x, y + h, w->fontScaleX, w->fontScaleY, textColor, w->lineText[j], 0.0f, 0, 0, &cgs.media.limboFont2);
			}

			h -= (w->lineHeight[j] + 3);

			if (!(w->effects & WFX_TRUETYPE))
			{
				CG_Text_Paint_Ext(x, y + h, cg_fontScaleSP.value, cg_fontScaleSP.value, textColor, w->lineText[j], 0, 0, 0, &cgs.media.limboFont2);
			}
		}
	}

#ifdef FEATURE_MULTIVIEW
	// Wedge in MV info overlay
	if (cg.mvTotalClients > 0 && fAllowMV)
	{
		CG_mvOverlayDisplay();
	}
#endif

	// Extra rate info
	CG_demoAviFPSDraw();
	CG_demoTimescaleDraw();

#ifdef FEATURE_MULTIVIEW
	// Mouse cursor lays on top of everything
	if (cg.mvTotalClients > 0 && cg.time < cgs.cursorUpdate && fAllowMV)
	{
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
	}
#endif

	if (fCleanup)
	{
		CG_windowCleanup();
	}
}

/**
 * @brief Set the window width and height based on the windows text/font parameters
 * @param[in,out] w
 */
void CG_windowNormalizeOnText(cg_window_t *w)
{
	int i, tmp;

	if (w == NULL)
	{
		return;
	}

	w->w = 0;
	w->h = 0;

	if (!(w->effects & WFX_TRUETYPE))
	{
		w->fontWidth  = (int)(w->fontScaleX * WINDOW_FONTWIDTH);
		w->fontHeight = (int)(w->fontScaleY * WINDOW_FONTHEIGHT);
	}

	for (i = 0; i < w->lineCount; i++)
	{
		if (w->effects & WFX_TRUETYPE)
		{
			tmp = CG_Text_Width_Ext(w->lineText[i], w->fontScaleX, 0, &cgs.media.limboFont2);
		}
		else
		{
			tmp = CG_Text_Width_Ext(w->lineText[i], cg_fontScaleSP.value, 0, &cgs.media.limboFont2);
		}

		if (tmp > w->w)
		{
			w->w = tmp;
		}
	}

	for (i = 0; i < w->lineCount; i++)
	{
		if (w->effects & WFX_TRUETYPE)
		{
			w->lineHeight[i] = CG_Text_Height_Ext(w->lineText[i], w->fontScaleY, 0, &cgs.media.limboFont2);
		}
		else
		{
			w->lineHeight[i] = w->fontHeight;
		}

		w->h += w->lineHeight[i] + 3;
	}

	// Border + margins
	w->w += 10;
	w->h += 3;

	// Set up bottom alignment
	if (w->x < 0)
	{
		w->x += Ccg_WideX(SCREEN_WIDTH) - w->w;
	}
	if (w->y < 0)
	{
		w->y += SCREEN_HEIGHT - w->h;
	}
}

/**
 * @brief CG_printWindow
 * @param[in] str
 */
void CG_printWindow(const char *str)
{
	int         pos = 0, pos2 = 0;
	char        buf[MAX_STRING_CHARS];
	cg_window_t *w = cg.windowCurrent;

	if (w == NULL)
	{
		return;
	}

	// Silly logic for a strict format
	Q_strncpyz(buf, str, MAX_STRING_CHARS);
	while (buf[pos] > 0 && w->lineCount < MAX_WINDOW_LINES)
	{
		if (buf[pos] == '\n')
		{
			if (pos2 == pos)
			{
				if (!CG_addString(w, " "))
				{
					return;
				}
			}
			else
			{
				buf[pos] = 0;
				if (!CG_addString(w, buf + pos2))
				{
					return;
				}
			}
			pos2 = ++pos;
			continue;
		}
		pos++;
	}

	if (pos2 < pos)
	{
		CG_addString(w, buf + pos2);
	}
}

/**
 * @brief String buffer handling
 */
void CG_initStrings(void)
{
	int i;

	for (i = 0; i < MAX_STRINGS; i++)
	{
		cg.aStringPool[i].fActive = qfalse;
		cg.aStringPool[i].str[0]  = 0;
	}
}

/**
 * @brief CG_addString
 * @param[in,out] w
 * @param[in] buf
 * @return
 */
qboolean CG_addString(cg_window_t *w, const char *buf)
{
	int i;

	// Check if we're reusing the current buf
	if (w->lineText[w->lineCount] != NULL)
	{
		for (i = 0; i < MAX_STRINGS; i++)
		{
			if (!cg.aStringPool[i].fActive)
			{
				continue;
			}

			if (w->lineText[w->lineCount] == (char *)&cg.aStringPool[i].str)
			{
				w->lineCount++;
				cg.aStringPool[i].fActive = qtrue;
				Q_strncpyz(cg.aStringPool[i].str, buf, sizeof(cg.aStringPool[0].str));

				return qtrue;
			}
		}
	}

	for (i = 0; i < MAX_STRINGS; i++)
	{
		if (!cg.aStringPool[i].fActive)
		{
			cg.aStringPool[i].fActive = qtrue;
			Q_strncpyz(cg.aStringPool[i].str, buf, sizeof(cg.aStringPool[0].str));
			w->lineText[w->lineCount++] = (char *)&cg.aStringPool[i].str;

			return qtrue;
		}
	}

	return qfalse;
}

/**
 * @brief CG_removeStrings
 * @param[in,out] w
 */
void CG_removeStrings(cg_window_t *w)
{
	int i, j;

	for (i = 0; i < w->lineCount; i++)
	{
		char *ref = w->lineText[i];

		for (j = 0; j < MAX_STRINGS; j++)
		{
			if (!cg.aStringPool[j].fActive)
			{
				continue;
			}

			if (ref == (char *)&cg.aStringPool[j].str)
			{
				w->lineText[i]            = NULL;
				cg.aStringPool[j].fActive = qfalse;
				cg.aStringPool[j].str[0]  = 0;

				break;
			}
		}
	}
}

// cgame cursor handling

#ifdef FEATURE_MULTIVIEW
/**
 * @brief Mouse overlay for controlling multiview windows
 */
void CG_cursorUpdate(void)
{
	int                i, x;
	int                nSelectedWindow = -1;
	float              nx, ny;
	float              fontScale  = cg_fontScaleSP.value;
	int                charHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	int                charWidth  = CG_Text_Width_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	cg_window_t        *w;
	cg_windowHandler_t *wh    = &cg.winHandler;
	qboolean           fFound = qfalse, fUpdateOverlay = qfalse;
	qboolean           fSelect, fResize;

	// Get cursor current position (when connected to a server)
	// Already updated in the keycatcher
	nx      = cgs.cursorX;
	ny      = cgs.cursorY;
	fSelect = cgs.fSelect;
	fResize = cgs.fResize;

	// For mm4
	cg.mvCurrentActive = cg.mvCurrentMainview;

	// For overlay highlights
	for (i = 0; i < cg.mvTotalClients; i++)
	{
		cg.mvOverlay[i].fActive = qfalse;
	}

	for (i = wh->numActiveWindows - 1; i >= 0; i--)
	{
		w = &wh->window[wh->activeWindows[i]];
		if ((w->effects & WFX_MULTIVIEW) && w != cg.mvCurrentMainview)
		{
			// Mouse/window detection
			// If the current window is selected, and the button is down, then allow the update
			// to occur, as quick mouse movements can move it past the window borders
			if (!fFound &&
			    (
			        ((w->mvInfo & MV_SELECTED) && fSelect) ||
			        (!fSelect && nx >= w->x && nx < w->x + w->w && ny >= w->y && ny < w->y + w->h)
			    ))
			{
				if (!(w->mvInfo & MV_SELECTED))
				{
					w->mvInfo      |= MV_SELECTED;
					nSelectedWindow = i;
				}

				// If not dragging/resizing, prime for later update
				if (!fSelect)
				{
					w->m_x = -1.0f;
					w->m_y = -1.0f;
				}
				else
				{
					if (w->m_x > 0 && w->m_y > 0)
					{
						if (fResize)
						{
							w->w += nx - w->m_x;
							if (w->x + w->w > Ccg_WideX(SCREEN_WIDTH) - 2)
							{
								w->w = Ccg_WideX(SCREEN_WIDTH) - 2 - w->x;
							}
							if (w->w < 64)
							{
								w->w = 64;
							}

							w->h += ny - w->m_y;
							if (w->y + w->h > SCREEN_HEIGHT - 2)
							{
								w->h = SCREEN_HEIGHT - 2 - w->y;
							}
							if (w->h < 48)
							{
								w->h = 48;
							}
						}
						else
						{
							w->x += nx - w->m_x;
							if (w->x + w->w > Ccg_WideX(SCREEN_WIDTH) - 2)
							{
								w->x = Ccg_WideX(SCREEN_WIDTH) - 2 - w->w;
							}
							if (w->x < 2)
							{
								w->x = 2;
							}

							w->y += ny - w->m_y;
							if (w->y + w->h > SCREEN_HEIGHT - 2)
							{
								w->y = SCREEN_HEIGHT - 2 - w->h;
							}
							if (w->y < 2)
							{
								w->y = 2;
							}
						}
					}

					w->m_x = nx;
					w->m_y = ny;
				}

				fFound             = qtrue;
				cg.mvCurrentActive = w;

				// Reset mouse info for window if it loses focuse
			}
			else if (w->mvInfo & MV_SELECTED)
			{
				fUpdateOverlay = qtrue;
				w->m_x         = -1.0f;
				w->m_y         = -1.0f;
				w->mvInfo     &= ~MV_SELECTED;

				if (fFound)
				{
					break;              // Small optimization: we've found a new window, and cleared the old focus
				}
			}
		}
	}

	nx = (float)(MVINFO_RIGHT - charWidth * 12.0f);
	ny = (float)(MVINFO_TOP + charHeight * 2.0f);

	// Highlight corresponding active window's overlay element
	if (fFound)
	{
		for (i = 0; i < cg.mvTotalClients; i++)
		{
			if (cg.mvOverlay[i].pID == (cg.mvCurrentActive->mvInfo & MV_PID))
			{
				cg.mvOverlay[i].fActive = qtrue;
				break;
			}
		}
	}
	// Check MV overlay detection here for better perf with more text elements
	// General boundary check
	else
	{
		// display on two columns
		int hOffset = 32;
		int xOffset = (int)(MVINFO_RIGHT - hOffset);

		// Ugh, have to loop through BOTH team lists
		for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
		{
			if (cg.mvTotalTeam[i] == 0)
			{
				continue;
			}

			if (cgs.cursorX >= nx && cgs.cursorY >= ny && cgs.cursorX < xOffset &&
			    cgs.cursorY < ny + (cg.mvTotalTeam[i] * charHeight * 2.0f))
			{
				int pos = (int)((cgs.cursorY - ny) / (charHeight * 2.0f));

				if (pos < cg.mvTotalTeam[i])
				{
					int x = xOffset - cg.mvOverlay[(cg.mvTeamList[i][pos])].width;
					int y = (int)(MVINFO_TOP + ((pos + 1) * charHeight * 2.0f));

					// See if we're really over something
					if (cgs.cursorX >= x && cgs.cursorY >= y &&
					    cgs.cursorX <= xOffset &&
					    cgs.cursorY <= y + charHeight * 2.0f)
					{
						// Perform any other window handling here for MV
						// views based on element selection
						cg.mvOverlay[(cg.mvTeamList[i][pos])].fActive = qtrue;

						w = CG_mvClientLocate(cg.mvOverlay[(cg.mvTeamList[i][pos])].pID);
						if (w != NULL)
						{
							cg.mvCurrentActive = w;
						}

						if (fSelect)
						{
							if (w != NULL)
							{
								// Swap window-view with mainview
								if (w != cg.mvCurrentMainview)
								{
									CG_mvMainviewSwap(w);
								}
							}
							else
							{
								// Swap non-view with mainview
								cg.mvCurrentMainview->mvInfo = (cg.mvCurrentMainview->mvInfo & ~MV_PID) |
								                               (cg.mvOverlay[cg.mvTeamList[i][pos]].pID & MV_PID);
								fUpdateOverlay = qtrue;
							}
						}
					}
				}
			}
			xOffset += hOffset;
			nx      += hOffset;
		}
	}

	// If we have a new highlight, reorder so our highlight is always
	// drawn last (on top of all other windows)
	if (nSelectedWindow >= 0)
	{
		int j;
		fUpdateOverlay = qtrue;
		x              = wh->activeWindows[nSelectedWindow];

		for (j = nSelectedWindow; j < wh->numActiveWindows - 1; j++)
		{
			wh->activeWindows[j] = wh->activeWindows[j + 1];
		}

		wh->activeWindows[wh->numActiveWindows - 1] = x;
	}

	// Finally, sync the overlay, if needed
	if (fUpdateOverlay)
	{
		CG_mvOverlayUpdate();
	}
}
#endif
