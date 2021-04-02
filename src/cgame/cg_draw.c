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
 * @file cg_draw.c
 * @brief Draw all of the graphical elements during active (after loading)
 *        gameplay
 */

#include "cg_local.h"

char *Binding_FromName(const char *cvar);
void Controls_GetConfig(void);
void CG_DrawOverlays(void);
int activeFont;

extern vec4_t HUD_Border;

///////////////////////
////// new hud stuff
///////////////////////

/**
 * @brief CG_Text_SetActiveFont
 * @param[in] font
 */
void CG_Text_SetActiveFont(int font)
{
	activeFont = font;
}

/**
 * @brief CG_Text_Width_Ext
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @param[in] font
 * @return
 */
int CG_Text_Width_Ext(const char *text, float scale, int limit, fontHelper_t *font)
{
	float out = 0;

	if (text)
	{
		const char  *s = text;
		glyphInfo_t *glyph;
		int         count = 0;
		int         len   = Q_UTF8_Strlen(text);

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			if (Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = Q_UTF8_GetGlyph(font, s);
				out  += glyph->xSkip;
				s    += Q_UTF8_Width(s);
				count++;
			}
		}
	}

	return out * scale * Q_UTF8_GlyphScale(font);
}

/**
 * @brief CG_Text_Width
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @return
 */
int CG_Text_Width(const char *text, float scale, int limit)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Width_Ext(text, scale, limit, font);
}

/**
 * @brief CG_Text_Height_Ext
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @param[in] font
 * @return
 */
int CG_Text_Height_Ext(const char *text, float scale, int limit, fontHelper_t *font)
{
	float max = 0;

	if (text)
	{
		const char  *s = text;
		glyphInfo_t *glyph;
		int         count = 0;
		int         len   = Q_UTF8_Strlen(text);

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			if (Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = Q_UTF8_GetGlyph(font, s);
				if (max < glyph->height)
				{
					max = glyph->height;
				}
				s += Q_UTF8_Width(s);
				count++;
			}
		}
	}
	return max * scale * Q_UTF8_GlyphScale(font);
}

/**
 * @brief CG_Text_Height
 * @param[in] text
 * @param[in] scale
 * @param[in] limit
 * @return
 */
int CG_Text_Height(const char *text, float scale, int limit)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Height_Ext(text, scale, limit, font);
}

/**
 * @brief CG_Text_PaintChar_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] s
 * @param[in] t
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 */
void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader)
{
	w *= scalex;
	h *= scaley;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

/**
 * @brief CG_Text_PaintChar
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 * @param[in] scale
 * @param[in] s
 * @param[in] t
 * @param[in] s2
 * @param[in] t2
 * @param[in] hShader
 */
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader)
{
	float w = width * scale;
	float h = height * scale;

	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

/**
 * @brief CG_Text_Paint_Centred_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] font
 */
void CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font)
{
	x -= CG_Text_Width_Ext(text, scalex, limit, font) * 0.5f;

	CG_Text_Paint_Ext(x, y, scalex, scaley, color, text, adjust, limit, style, font);
}

/**
 * @brief CG_Text_Paint_RightAligned_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] font
 */
void CG_Text_Paint_RightAligned_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font)
{
	x -= CG_Text_Width_Ext(text, scalex, limit, font);

	CG_Text_Paint_Ext(x, y, scalex, scaley, color, text, adjust, limit, style, font);
}

/**
 * @brief CG_Text_Paint_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scalex
 * @param[in] scaley
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 * @param[in] font
 */
void CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font)
{
	if (text)
	{
		vec4_t      newColor;
		glyphInfo_t *glyph;
		const char  *s = text;
		float       yadj;
		int         len, count = 0, ofs;

		scalex *= Q_UTF8_GlyphScale(font);
		scaley *= Q_UTF8_GlyphScale(font);

		trap_R_SetColor(color);
		Com_Memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = Q_UTF8_Strlen(text);
		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			glyph = Q_UTF8_GetGlyph(font, s);

			if (Q_IsColorString(s))
			{
				if (*(s + 1) == COLOR_NULL)
				{
					Com_Memcpy(newColor, color, sizeof(newColor));
				}
				else
				{
					Com_Memcpy(newColor, g_color_table[ColorIndex(*(s + 1))], sizeof(newColor));
					newColor[3] = color[3];
				}
				trap_R_SetColor(newColor);
				s += 2;
				continue;
			}
			else
			{
				yadj = scaley * glyph->top;

				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
				{
					ofs           = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor(colorBlack);
					CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex) + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor(newColor);
				}
				CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex), y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
				x += (glyph->xSkip * scalex) + adjust;
				s += Q_UTF8_Width(s);
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_Text_Paint
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] adjust
 * @param[in] limit
 * @param[in] style
 */
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	CG_Text_Paint_Ext(x, y, scale, scale, color, text, adjust, limit, style, font);
}

/**
 * @brief CG_Text_PaintWithCursor_Ext
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] cursorPos
 * @param[in] cursor
 * @param[in] limit
 * @param[in] style
 */
void CG_Text_PaintWithCursor_Ext(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style, fontHelper_t *font)
{
	vec4_t      newColor = { 0, 0, 0, 0 };
	glyphInfo_t *glyph, *glyph2;
	float       useScale = scale * Q_UTF8_GlyphScale(font);

	if (text)
	{
		float      yadj;
		int        len   = Q_UTF8_Strlen(text);
		int        count = 0;
		const char *s    = text;

		trap_R_SetColor(color);
		Com_Memcpy(&newColor[0], &color[0], sizeof(vec4_t));

		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		glyph2 = Q_UTF8_GetGlyph(font, cursor);
		while (s && *s && count < len)
		{
			glyph = Q_UTF8_GetGlyph(font, s);
			yadj  = useScale * glyph->top;

			if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
			{
				int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

				colorBlack[3] = newColor[3];
				trap_R_SetColor(colorBlack);
				CG_Text_PaintChar(x + (glyph->pitch * useScale) + ofs, y - yadj + ofs,
				                  glyph->imageWidth,
				                  glyph->imageHeight,
				                  useScale,
				                  glyph->s,
				                  glyph->t,
				                  glyph->s2,
				                  glyph->t2,
				                  glyph->glyph);
				colorBlack[3] = 1.0;
				trap_R_SetColor(newColor);
			}
			CG_Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
			                  glyph->imageWidth,
			                  glyph->imageHeight,
			                  useScale,
			                  glyph->s,
			                  glyph->t,
			                  glyph->s2,
			                  glyph->t2,
			                  glyph->glyph);

			// CG_DrawPic(x, y - yadj, scale * uiDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * uiDC.Assets.textFont.glyphs[text[i]].imageHeight, uiDC.Assets.textFont.glyphs[text[i]].glyph);
			yadj = useScale * glyph2->top;
			if (count == cursorPos && !((cgDC.realTime / BLINK_DIVISOR) & 1))
			{
				CG_Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
				                  glyph2->imageWidth,
				                  glyph2->imageHeight,
				                  useScale,
				                  glyph2->s,
				                  glyph2->t,
				                  glyph2->s2,
				                  glyph2->t2,
				                  glyph2->glyph);
			}

			x += (glyph->xSkip * useScale);
			s += Q_UTF8_Width(s);
			count++;

		}
		// need to paint cursor at end of text
		if (cursorPos == len && !((cgDC.realTime / BLINK_DIVISOR) & 1))
		{
			yadj = useScale * glyph2->top;
			CG_Text_PaintChar(x + (glyph2->pitch * useScale), y - yadj,
			                  glyph2->imageWidth,
			                  glyph2->imageHeight,
			                  useScale,
			                  glyph2->s,
			                  glyph2->t,
			                  glyph2->s2,
			                  glyph2->t2,
			                  glyph2->glyph);
		}

		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_Text_PaintWithCursor
 * @param[in] x
 * @param[in] y
 * @param[in] scale
 * @param[in] color
 * @param[in] text
 * @param[in] cursorPos
 * @param[in] cursor
 * @param[in] limit
 * @param[in] style
 */
void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style)
{
	fontHelper_t *font = &cgDC.Assets.fonts[activeFont];

	CG_Text_PaintWithCursor_Ext(x, y, scale, color, text, cursorPos, cursor, limit, style, font);
}

/**
 * @brief CG_DrawTeamBackground
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @param[in] alpha
 * @param[in] team
 *
 * @note Unused
 */
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team)
{
	vec4_t hcolor;

	if (team == TEAM_AXIS)
	{
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
		hcolor[3] = alpha;
	}
	else if (team == TEAM_ALLIES)
	{
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
		hcolor[3] = alpha;
	}
	else
	{
		return;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);
}

/*
===========================================================================================
  LOWER RIGHT CORNER
===========================================================================================
*/

#define CHATLOC_Y 478

/**
 * @brief CG_DrawTeamInfo
 */
static void CG_DrawTeamInfo(void)
{
	int chatHeight = TEAMCHAT_HEIGHT;

	// no need to adjust chat height for intermission here - CG_DrawTeamInfo is called from CG_Draw2D
	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
	{
		chatHeight = cg_teamChatHeight.integer;
	}

	if (chatHeight <= 0)
	{
		return; // disabled
	}

	if (cgs.teamLastChatPos != cgs.teamChatPos)
	{
		vec4_t hcolor;
		int    i;
		float  lineHeight = 9.f;
		float  alphapercent;
		float  scale       = 0.2f;
		float  icon_width  = 12.f;
		float  icon_height = 8.f;
		int    chatWidth   = 320;
		int    chatPosX    = ((int)Ccg_WideX(SCREEN_WIDTH) - chatWidth) / 2;

		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer)
		{
			cgs.teamLastChatPos++;
		}

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--)
		{
			alphapercent = 1.0f - (cg.time - cgs.teamChatMsgTimes[i % chatHeight]) / (float)(cg_teamChatTime.integer);
			if (alphapercent > 1.0f)
			{
				alphapercent = 1.0f;
			}
			else if (alphapercent < 0)
			{
				alphapercent = 0.f;
			}

			// chatter team instead
			if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_AXIS)
			{
				hcolor[0] = 1;
				hcolor[1] = 0;
				hcolor[2] = 0;
			}
			else if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_ALLIES)
			{
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 1;
			}
			else
			{
				hcolor[0] = 0;
				hcolor[1] = 1;
				hcolor[2] = 0;
			}

			hcolor[3] = 0.66f * alphapercent;

			trap_R_SetColor(hcolor);
			CG_DrawPic(chatPosX, CHATLOC_Y - (cgs.teamChatPos - i) * lineHeight, chatWidth, lineHeight, cgs.media.teamStatusBar);

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor(hcolor);

			// chat icons
			if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_AXIS)
			{
				CG_DrawPic(chatPosX, CHATLOC_Y - (cgs.teamChatPos - i - 0.9f) * lineHeight - 8, icon_width, icon_height, cgs.media.axisFlag);
			}
			else if (cgs.teamChatMsgTeams[i % chatHeight] == TEAM_ALLIES)
			{
				CG_DrawPic(chatPosX, CHATLOC_Y - (cgs.teamChatPos - i - 0.9f) * lineHeight - 8, icon_width, icon_height, cgs.media.alliedFlag);
			}
			CG_Text_Paint_Ext(chatPosX + 16, CHATLOC_Y - (cgs.teamChatPos - i - 1) * lineHeight - 1, scale, scale, hcolor, cgs.teamChatMsgs[i % chatHeight], 0, 0, 0, &cgs.media.limboFont2);
		}
		trap_R_SetColor(NULL);
	}
}

/**
 * @brief CG_PickupItemText
 * @param[in] item
 * @return
 */
const char *CG_PickupItemText(int itemNum)
{
	gitem_t *item;

	item = BG_GetItem(itemNum);

	if (item->giType == IT_HEALTH)
	{
		return va(CG_TranslateString("a %s"), item->pickup_name);
	}

	if (item->giType == IT_TEAM)
	{
		return CG_TranslateString("an Objective");
	}

	// FIXME: this is just related to english
	if (item->pickup_name[0] == 'a' || item->pickup_name[0] == 'A')
	{
		return va(CG_TranslateString("an %s"), item->pickup_name);
	}

	return va(CG_TranslateString("a %s"), item->pickup_name);
}

/*
===============================================================================
CENTER PRINTING
===============================================================================
*/

#define CP_LINEWIDTH (int)(Ccg_WideX(56))

/**
 * @brief Format the message by turning spaces into newlines, if we've run over the linewidth
 * @param[in,out] s The message to format
 * @return The number of line needed to display the messages
 */
static int CG_FormatCenterPrint(char *s)
{
	char     *lastSpace = NULL;
	int      i, len, lastLR = 0;
	int      lineWidth, lineNumber = 1;
	qboolean neednewline = qfalse;

	len       = Q_UTF8_PrintStrlen(s);
	lineWidth = CP_LINEWIDTH;

	for (i = 0; i < len; i++)
	{
		if (Q_IsColorString(s))
		{
			s += 2;
		}

		if ((i - lastLR) >= lineWidth)
		{
			neednewline = qtrue;
		}

		if (*s == ' ')
		{
			lastSpace = s;
		}

		if (neednewline && lastSpace)
		{
			*lastSpace = '\n';
			lastSpace  = NULL;
			lastLR     = i;
			lineNumber++;
			neednewline = qfalse;
		}

		// count the number of lines for centering
		if (*s == '\n')
		{
			lastLR = i;
			lineNumber++;
		}

		s += Q_UTF8_Width(s);
	}

	// we reach the end of the string and it doesn't fit in on line
	if (neednewline && lastSpace)
	{
		*lastSpace = '\n';
		lineNumber++;
	}

	return lineNumber;
}

/**
 * @brief Called for important messages that should stay in the center of the screen
 * for a few moments
 * @param[in] str
 * @param[in] y
 * @param[in] fontScale
 */
void CG_CenterPrint(const char *str, int y, float fontScale)
{
	CG_PriorityCenterPrint(str, y, fontScale, 0);
	cg.centerPrintTime = cg.time;  // overwrite
}

/**
 * @brief Called for important messages that should stay in the center of the screen
 * for a few moments
 * @param[in] str
 * @param[in] y
 * @param[in] fontScale
 * @param[in] priority
 */
void CG_PriorityCenterPrint(const char *str, int y, float fontScale, int priority)
{
	// don't draw if this print message is less important
	if (cg.centerPrintTime && priority < cg.centerPrintPriority)
	{
		return;
	}

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));
	cg.centerPrintLines      = CG_FormatCenterPrint(cg.centerPrint);
	cg.centerPrintPriority   = priority;
	cg.centerPrintLines      = 1;
	cg.centerPrintTime       = cg.time + 2000;
	cg.centerPrintY          = y;
	cg.centerPrintCharHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	cg.centerPrintCharWidth  = CG_Text_Width_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	cg.centerPrintFontScale  = fontScale;
}

/**
 * @brief CG_DrawCenterString
 */
static void CG_DrawCenterString(void)
{
	char  *start, *end;
	int   len;
	char  currentColor[3] = S_COLOR_WHITE;
	char  nextColor[3] = { 0, 0, 0 };
	int   x, y, w;
	float *color;

	if (!cg.centerPrintTime)
	{
		return;
	}

	color = CG_FadeColor(cg.centerPrintTime, (int)(1000 * cg_centertime.value));
	if (!color)
	{
		cg.centerPrintTime     = 0;
		cg.centerPrintPriority = 0;
		return;
	}

	trap_R_SetColor(color);

	y = cg.centerPrintY - cg.centerPrintLines * cg.centerPrintCharHeight / 2;

	for (start = end = cg.centerPrint; *start; end += len)
	{
		if (Q_IsColorString(end))
		{
			// don't store color if the line start with a color
			if (start == end)
			{
				Com_Memset(currentColor, 0, 3);
			}

			// store the used color to reused it in case the line is splitted on multi line
			Q_strncpyz(nextColor, end, 3);
			len = 2;
		}
		else
		{
			len = Q_UTF8_Width(end);
		}

		if (!*end || *end == '\n')
		{
			char linebuffer[1024] = { 0 };

			if (*currentColor)
			{
				Q_strncpyz(linebuffer, currentColor, 3);
				Q_strcat(linebuffer, (end - start) + 3, start);
			}
			else
			{
				Q_strncpyz(linebuffer, start, (end - start) + 1);
			}

			w = CG_Text_Width_Ext(linebuffer, cg.centerPrintFontScale, 0, &cgs.media.limboFont2);
			x = Ccg_WideX(320) - w / 2;

			CG_Text_Paint_Ext(x, y, cg.centerPrintFontScale, cg.centerPrintFontScale, colorWhite, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

			y += cg.centerPrintCharHeight * 1.5f;

			if (nextColor[0])
			{
				Q_strncpyz(currentColor, nextColor, 3);
			}

			start = end + len;
		}
	}

	trap_R_SetColor(NULL);
}

/*
================================================================================
CROSSHAIRS
================================================================================
*/

/**
 * @brief CG_DrawWeapReticle
 */
static void CG_DrawScopedReticle(void)
{
	int weapon;

	// So that we will draw reticle
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		weapon = cg.snap->ps.weapon;
	}
	else
	{
		weapon = cg.weaponSelect;
	}

	if (!(GetWeaponTableData(weapon)->type & WEAPON_TYPE_SCOPED))
	{
		return;
	}

	// sides
	CG_FillRect(0, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);
	CG_FillRect(560 + cgs.wideXoffset, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);

	// center
	if (cgs.media.reticleShaderSimple)
	{
		CG_DrawPic(80 + cgs.wideXoffset, 0, SCREEN_HEIGHT, SCREEN_HEIGHT, cgs.media.reticleShaderSimple);
	}

	if (weapon == WP_FG42_SCOPE)
	{
		// hairs
		CG_FillRect(84 + cgs.wideXoffset, 239, 150, 3, colorBlack);     // left
		CG_FillRect(234 + cgs.wideXoffset, 240, 173, 1, colorBlack);    // horiz center
		CG_FillRect(407 + cgs.wideXoffset, 239, 150, 3, colorBlack);    // right


		CG_FillRect(319 + cgs.wideXoffset, 2, 3, 151, colorBlack);      // top center top
		CG_FillRect(320 + cgs.wideXoffset, 153, 1, 114, colorBlack);    // top center bot

		CG_FillRect(320 + cgs.wideXoffset, 241, 1, 87, colorBlack);     // bot center top
		CG_FillRect(319 + cgs.wideXoffset, 327, 3, 151, colorBlack);    // bot center bot
	}
	else if (weapon == WP_GARAND_SCOPE)
	{
		// hairs
		CG_FillRect(84 + cgs.wideXoffset, 239, 177, 2, colorBlack);     // left
		CG_FillRect(320 + cgs.wideXoffset, 242, 1, 58, colorBlack);     // center top
		CG_FillRect(319 + cgs.wideXoffset, 300, 2, 178, colorBlack);    // center bot
		CG_FillRect(380 + cgs.wideXoffset, 239, 177, 2, colorBlack);    // right
	}
	else if (weapon == WP_K43_SCOPE)
	{
		// hairs
		CG_FillRect(84 + cgs.wideXoffset, 239, 177, 2, colorBlack);     // left
		CG_FillRect(320 + cgs.wideXoffset, 242, 1, 58, colorBlack);     // center top
		CG_FillRect(319 + cgs.wideXoffset, 300, 2, 178, colorBlack);    // center bot
		CG_FillRect(380 + cgs.wideXoffset, 239, 177, 2, colorBlack);    // right
	}
}

/**
 * @brief CG_DrawMortarReticle
 */
static void CG_DrawMortarReticle(void)
{
	vec4_t   color = { 1.f, 1.f, 1.f, .5f };
	vec4_t   color_back = { 0.f, 0.f, 0.f, .25f };
	vec4_t   color_extends = { .77f, .73f, .1f, 1.f };
	vec4_t   color_lastfire = { .77f, .1f, .1f, 1.f };
	vec4_t   color_firerequest = { 1.f, 1.f, 1.f, 1.f };
	float    offset, localOffset;
	int      i, min, majorOffset, val, printval, fadeTime, requestFadeTime;
	char     *s;
	float    angle, angleMin, angleMax;
	qboolean hasRightTarget, hasLeftTarget;

	// Background
	CG_FillRect(136 + cgs.wideXoffset, 236, 154, 38, color_back);
	CG_FillRect(290 + cgs.wideXoffset, 160, 60, 208, color_back);
	CG_FillRect(350 + cgs.wideXoffset, 236, 154, 38, color_back);

	// Horizontal bar

	// bottom
	CG_FillRect(140 + cgs.wideXoffset, 264, 150, 1, color);    // left
	CG_FillRect(350 + cgs.wideXoffset, 264, 150, 1, color);    // right

	// 10 units - 5 degrees
	// total of 360 units
	// nothing displayed between 150 and 210 units
	// 360 / 10 = 36 bits, means 36 * 5 = 180 degrees
	// that means left is cg.predictedPlayerState.viewangles[YAW] - .5f * 180
	angle = 360 - AngleNormalize360(cg.predictedPlayerState.viewangles[YAW] - 90.f);

	offset      = (5.f / 65536) * ((int)(angle * (65536 / 5.f)) & 65535);
	min         = (int)(AngleNormalize360(angle - .5f * 180) / 15.f) * 15;
	majorOffset = (int)(floor((int)floor(AngleNormalize360(angle - .5f * 180)) % 15) / 5);

	for (val = i = 0; i < 36; i++)
	{
		localOffset = i * 10.f + (offset * 2.f);

		if (localOffset >= 150 && localOffset <= 210)
		{
			if (i % 3 == majorOffset)
			{
				val++;
			}
			continue;
		}

		if (i % 3 == majorOffset)
		{
			printval = min - val * 15 + 180;

			// rain - old tertiary abuse was nasty and had undefined result
			if (printval < 0)
			{
				printval += 360;
			}
			else if (printval >= 360)
			{
				printval -= 360;
			}

			s = va("%i", printval);
			CG_Text_Paint_Ext(500 - localOffset  + cgs.wideXoffset - .5f * CG_Text_Width_Ext(s, .15f, 0, &cgs.media.limboFont1), 244, .15f, .15f, color, s, 0, 0, 0, &cgs.media.limboFont1);
			CG_FillRect(500 - localOffset + cgs.wideXoffset, 248, 1, 16, color);
			val++;
		}
		else
		{
			CG_FillRect(500 - localOffset + cgs.wideXoffset, 256, 1, 8, color);
		}
	}

	// the extremes
	// 30 degrees plus a 15 degree border
	angleMin = AngleNormalize360(360 - (cg.pmext.mountedWeaponAngles[YAW] - 90.f) - (30.f + 15.f));
	angleMax = AngleNormalize360(360 - (cg.pmext.mountedWeaponAngles[YAW] - 90.f) + (30.f + 15.f));

	// right
	localOffset = (AngleNormalize360(angle - angleMin) / 5.f) * 10.f;
	CG_FillRect(320 - localOffset + cgs.wideXoffset, 252, 2, 18, color_extends);

	// left
	localOffset = (AngleNormalize360(angleMax - angle) / 5.f) * 10.f;
	CG_FillRect(320 + localOffset + cgs.wideXoffset, 252, 2, 18, color_extends);

	// last fire pos
	fadeTime = 0;
	if (CHECKBITWISE(GetWeaponTableData(cg.lastFiredWeapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && cg.mortarImpactTime >= -1)
	{
		fadeTime = cg.time - (cg.predictedPlayerEntity.muzzleFlashTime + 5000);

		if (fadeTime < 3000)
		{
			float lastfireAngle;

			if (fadeTime > 0)
			{
				color_lastfire[3] = 1.f - (fadeTime / 3000.f);
			}

			lastfireAngle = AngleNormalize360(360 - (cg.mortarFireAngles[YAW] - 90.f));

			localOffset = ((AngleSubtract(angle, lastfireAngle)) / 5.f) * 10.f;
			CG_FillRect(320 - localOffset + cgs.wideXoffset, 252, 2, 18, color_lastfire);
		}
	}

	// mortar attack requests
	hasRightTarget = hasLeftTarget = qfalse;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		requestFadeTime = cg.time - (cg.artilleryRequestTime[i] + 25000);

		if (requestFadeTime < 5000)
		{
			vec3_t dir;
			float  yaw;
			float  attackRequestAngle;

			VectorSubtract(cg.artilleryRequestPos[i], cg.predictedPlayerEntity.lerpOrigin, dir);

			// ripped this out of vectoangles
			if (dir[1] == 0.f && dir[0] == 0.f)
			{
				yaw = 0;
			}
			else
			{
				if (dir[0] != 0.f)
				{
					yaw = (atan2(dir[1], dir[0]) * 180 / M_PI);
				}
				else if (dir[1] > 0)
				{
					yaw = 90;
				}
				else
				{
					yaw = 270;
				}
				if (yaw < 0)
				{
					yaw += 360;
				}
			}

			if (requestFadeTime > 0)
			{
				color_firerequest[3] = 1.f - (requestFadeTime / 5000.f);
			}

			attackRequestAngle = AngleNormalize360(360 - (yaw - 90.f));

			yaw = AngleSubtract(attackRequestAngle, angleMin);

			if (yaw < 0)
			{
				if (!hasLeftTarget)
				{
					trap_R_SetColor(color_firerequest);
					CG_DrawPic(136 + 2 + cgs.wideXoffset, 236 + 38 - 10 + 1, 8, 8, cgs.media.mortarTargetArrow);
					trap_R_SetColor(NULL);

					hasLeftTarget = qtrue;
				}
			}
			else if (yaw > 90)
			{
				if (!hasRightTarget)
				{
					trap_R_SetColor(color_firerequest);
					CG_DrawPic(350 + 154 - 10 + cgs.wideXoffset, 236 + 38 - 10 + 1, -8, 8, cgs.media.mortarTargetArrow);
					trap_R_SetColor(NULL);

					hasRightTarget = qtrue;
				}
			}
			else
			{
				localOffset = ((AngleSubtract(angle, attackRequestAngle)) / 5.f) * 10.f;

				trap_R_SetColor(color_firerequest);
				CG_DrawPic(320 - localOffset - 8 + cgs.wideXoffset, 264 - 8, 16, 16, cgs.media.mortarTarget);
				trap_R_SetColor(NULL);
			}
		}
	}

	// Vertical bar

	// sides
	CG_FillRect(295 + cgs.wideXoffset, 164, 1, 200, color);    // left
	CG_FillRect(345 + cgs.wideXoffset, 164, 1, 200, color);    // right

	// 10 units - 2.5 degrees
	// total of 200 units
	// 200 / 10 = 20 bits, means 20 * 2.5 = 50 degrees
	// that means left is cg.predictedPlayerState.viewangles[PITCH] - .5f * 50
	angle = AngleNormalize180(360 - (cg.predictedPlayerState.viewangles[PITCH] - 60));

	offset      = (2.5f / 65536) * ((int)(angle * (65536 / 2.5f)) & 65535);
	min         = floor((angle + .5f * 50) / 10.f) * 10;
	majorOffset = (int)(floor((int)((angle + .5f * 50) * 10.f) % 100) / 25);

	for (val = i = 0; i < 20; i++)
	{
		localOffset = i * 10.f + (offset * 4.f);

		if (i % 4 == majorOffset)
		{
			printval = min - val * 10;

			// old tertiary abuse was nasty and had undefined result
			if (printval <= -180)
			{
				printval += 360;
			}
			else if (printval >= 180)
			{
				printval -= 180;
			}

			s = va("%i", printval);
			CG_Text_Paint_Ext(320  + cgs.wideXoffset - .5f * CG_Text_Width_Ext(s, .15f, 0, &cgs.media.limboFont1), 164 + localOffset + .5f * CG_Text_Height_Ext(s, .15f, 0, &cgs.media.limboFont1), .15f, .15f, color, s, 0, 0, 0, &cgs.media.limboFont1);
			CG_FillRect(295 + 1 + cgs.wideXoffset, 164 + localOffset, 12, 1, color);
			CG_FillRect(345 - 12 + cgs.wideXoffset, 164 + localOffset, 12, 1, color);
			val++;
		}
		else
		{
			CG_FillRect(295 + 1 + cgs.wideXoffset, 164 + localOffset, 8, 1, color);
			CG_FillRect(345 - 8 + cgs.wideXoffset, 164 + localOffset, 8, 1, color);
		}
	}

	// the extremes
	// 30 degrees up
	// 20 degrees down
	angleMin = AngleNormalize180(360 - (cg.pmext.mountedWeaponAngles[PITCH] - 60)) - 20.f;
	angleMax = AngleNormalize180(360 - (cg.pmext.mountedWeaponAngles[PITCH] - 60)) + 30.f;

	// top
	localOffset = angleMax - angle;
	if (localOffset < 0)
	{
		localOffset = 0;
	}
	localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
	if (localOffset < 100)
	{
		CG_FillRect(295 - 2 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_extends);
		CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_extends);
	}

	// bottom
	localOffset = angle - angleMin;
	if (localOffset < 0)
	{
		localOffset = 0;
	}
	localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
	if (localOffset < 100)
	{
		CG_FillRect(295 - 2 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_extends);
		CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_extends);
	}

	// last fire pos
	if (CHECKBITWISE(GetWeaponTableData(cg.lastFiredWeapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && cg.mortarImpactTime >= -1)
	{
		if (fadeTime < 3000)
		{
			float lastfireAngle = AngleNormalize180(360 - (cg.mortarFireAngles[PITCH] - 60));

			if (lastfireAngle > angle)
			{
				localOffset = lastfireAngle - angle;
				if (localOffset < 0)
				{
					localOffset = 0;
				}
				localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
				if (localOffset < 100)
				{
					CG_FillRect(295 - 2 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_lastfire);
					CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 - localOffset, 6, 2, color_lastfire);
				}
			}
			else
			{
				localOffset = angle - lastfireAngle;
				if (localOffset < 0)
				{
					localOffset = 0;
				}
				localOffset = (AngleNormalize360(localOffset) / 2.5f) * 10.f;
				if (localOffset < 100)
				{
					CG_FillRect(295 - 2 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_lastfire);
					CG_FillRect(345 - 4 + 1 + cgs.wideXoffset, 264 + localOffset, 6, 2, color_lastfire);
				}
			}
		}
	}
}

/**
 * @brief CG_DrawBinocReticle
 */
static void CG_DrawBinocReticle(void)
{
	if (cgs.media.binocShaderSimple)
	{
		CG_DrawPic(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, cgs.media.binocShaderSimple);
	}

	CG_FillRect(146 + cgs.wideXoffset, 239, 348, 1, colorBlack);

	CG_FillRect(188 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // ll
	CG_FillRect(234 + cgs.wideXoffset, 226, 1, 29, colorBlack);     // l
	CG_FillRect(274 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // lr
	CG_FillRect(320 + cgs.wideXoffset, 213, 1, 55, colorBlack);     // center
	CG_FillRect(360 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // rl
	CG_FillRect(406 + cgs.wideXoffset, 226, 1, 29, colorBlack);     // r
	CG_FillRect(452 + cgs.wideXoffset, 234, 1, 13, colorBlack);     // rr
}

/**
 * @brief CG_DrawCrosshair
 */
static void CG_DrawCrosshair(void)
{
	float     w, h;
	qhandle_t hShader;
	float     f;
	float     x, y;
	int       weapnum;

	if (cg.renderingThirdPerson)
	{
		return;
	}

	// using binoculars
	if (cg.zoomedBinoc)
	{
		CG_DrawBinocReticle();
		return;
	}

	// show reticle in limbo and spectator
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		weapnum = cg.snap->ps.weapon;
	}
	else
	{
		weapnum = cg.weaponSelect;
	}

	// no weapon, no crosshair
	if (weapnum == WP_NONE && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		return;
	}

	// special reticle for scoped weapon
	if (cg.zoomed)
	{
		if (!BG_PlayerMounted(cg.snap->ps.eFlags))
		{
			// don't let players prone moving and run with rifles -- speed 80 == crouch, 128 == walk, 256 == run
/*			if (VectorLengthSquared(cg.snap->ps.velocity) > Square(160) || cg.predictedPlayerState.eFlags & EF_PRONE_MOVING)
            {
                CG_FinishWeaponChange(cg.snap->ps.weapon, GetWeaponTableData(cg.snap->ps.weapon)->weapAlts);
            }
            else */if (
#ifdef FEATURE_MULTIVIEW
				cg.mvTotalClients < 1 ||
#endif
				cg.snap->ps.stats[STAT_HEALTH] > 0)
			{
				CG_DrawScopedReticle();
			}

			return;
		}
	}

	if (cg.snap->ps.eFlags & EF_PRONE_MOVING)
	{
		return;
	}

	// FIXME: spectators/chasing?
	if (CHECKBITWISE(GetWeaponTableData(cg.predictedPlayerState.weapon)->type, WEAPON_TYPE_MORTAR | WEAPON_TYPE_SET) && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		CG_DrawMortarReticle();
		return;
	}

	if (cg_drawCrosshair.integer < 0)     // moved down so it doesn't keep the scoped weaps from drawing reticles
	{
		return;
	}

	// no crosshair while leaning
	if (cg.snap->ps.leanf != 0.f)
	{
		return;
	}

	// set color based on health
	if (cg_crosshairHealth.integer)
	{
		vec4_t hcolor;

		CG_ColorForHealth(hcolor);
		trap_R_SetColor(hcolor);
	}
	else
	{
		trap_R_SetColor(cg.xhairColor);
	}

	w = h = cg_crosshairSize.value;

	// crosshair size represents aim spread
	f  = (float)((cg_crosshairPulse.integer == 0) ? 0 : cg.snap->ps.aimSpreadScale / 255.0);
	w *= (1 + f * 2.0f);
	h *= (1 + f * 2.0f);

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &w, &h);

	hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x + 0.5f * (cg.refdef_current->width - w), y + 0.5f * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, hShader);

	if (cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS])
	{
		w = h = cg_crosshairSize.value;
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
		CG_AdjustFrom640(&x, &y, &w, &h);

		if (cg_crosshairHealth.integer == 0)
		{
			trap_R_SetColor(cg.xhairColorAlt);
		}

		trap_R_DrawStretchPic(x + 0.5f * (cg.refdef_current->width - w), y + 0.5f * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
	}
	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawNoShootIcon
 */
static void CG_DrawNoShootIcon(void)
{
	float x, y, w, h;

	if ((cg.predictedPlayerState.eFlags & EF_PRONE) && (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_PANZER))
	{
		trap_R_SetColor(colorRed);
	}
	else if (cg.crosshairClientNoShoot)
	{
		float *color = CG_FadeColor(cg.crosshairClientTime, 1000);

		if (!color)
		{
			trap_R_SetColor(NULL);
			return;
		}

		trap_R_SetColor(color);
	}
	else
	{
		return;
	}

	w = h = 48.f;

	x = cg_crosshairX.integer + 1;
	y = cg_crosshairY.integer + 1;
	CG_AdjustFrom640(&x, &y, &w, &h);

	trap_R_DrawStretchPic(x + 0.5f * (cg.refdef_current->width - w), y + 0.5f * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, cgs.media.friendShader);
	trap_R_SetColor(NULL);
}

/**
 * @brief CG_ScanForCrosshairMine
 * @param[in] cent
 */
void CG_ScanForCrosshairMine(centity_t *cent)
{
	trace_t trace;
	vec3_t  start, end;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 512, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, NULL, NULL, end, -1, MASK_SOLID);

	if (Square(trace.endpos[0] - cent->currentState.pos.trBase[0]) < 256 &&
	    Square(trace.endpos[1] - cent->currentState.pos.trBase[1]) < 256 &&
	    Square(trace.endpos[2] - cent->currentState.pos.trBase[2]) < 256)
	{
		if (cent->currentState.otherEntityNum >= MAX_CLIENTS)
		{
			cg.crosshairMine = -1;
		}
		else
		{
			cg.crosshairMine     = cent->currentState.otherEntityNum;
			cg.crosshairMineTime = cg.time;
		}
	}
}

/**
 * @brief CG_ScanForCrosshairDyna
 * @details Mainly just a copy paste of CG_ScanForCrosshairMine
 * @param cent
 */
void CG_ScanForCrosshairDyna(centity_t *cent)
{
	trace_t trace;
	vec3_t  start, end;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 512, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, NULL, NULL, end, -1, MASK_SOLID);

	// pos.trBase is not the middle of the dyna, but due to different
	// orientations relative to the map axis, this cannot be corrected in a
	// simple way unfortunately
	if (Square(trace.endpos[0] - cent->currentState.pos.trBase[0]) < 256 &&
	    Square(trace.endpos[1] - cent->currentState.pos.trBase[1]) < 256 &&
	    Square(trace.endpos[2] - cent->currentState.pos.trBase[2]) < 256)
	{
		if (cent->currentState.otherEntityNum >= MAX_CLIENTS)
		{
			cg.crosshairDyna = -1;
		}
		else
		{
			cg.crosshairDyna     = cent->currentState.otherEntityNum;
			cg.crosshairDynaTime = cg.time;
		}
	}
}

/**
 * @brief Returns the distance to the entity
 * @param[out] zChange
 * @param[out] hitClient
 * @return Distance to the entity
 */
static float CG_ScanForCrosshairEntity(float *zChange, qboolean *hitClient)
{
	trace_t   trace;
	vec3_t    start, end;
	float     dist;
	centity_t *cent;

	// We haven't hit a client yet
	*hitClient = qfalse;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, MAX_TRACE, cg.refdef.viewaxis[0], end);

	cg.crosshairClientNoShoot = qfalse;

	CG_Trace(&trace, start, NULL, NULL, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM);

	// How far from start to end of trace?
	dist = VectorDistance(start, trace.endpos);

	// How far up or down are we looking?
	*zChange = trace.endpos[2] - start[2];

	if (trace.entityNum >= MAX_CLIENTS)
	{
		if (cg_entities[trace.entityNum].currentState.eFlags & EF_TAGCONNECT)
		{
			trace.entityNum = cg_entities[trace.entityNum].tagParent;
		}

		// is a tank with a healthbar
		// this might have some side-effects, but none right now as the script_mover is the only one that sets effect1Time
		if ((cg_entities[trace.entityNum].currentState.eType == ET_MOVER && cg_entities[trace.entityNum].currentState.effect1Time) ||
		    cg_entities[trace.entityNum].currentState.eType == ET_CONSTRUCTIBLE_MARKER)
		{
			// update the fade timer
			cg.crosshairClientNum    = trace.entityNum;
			cg.crosshairClientTime   = cg.time;
			cg.identifyClientRequest = cg.crosshairClientNum;
		}

		// Default: We're not looking at a client
		cg.crosshairNotLookingAtClient = qtrue;

		return dist;
	}

	// Reset the draw time for the SP crosshair
	cg.crosshairSPClientTime = cg.time;

	// Default: We're not looking at a client
	cg.crosshairNotLookingAtClient = qfalse;

	// We hit a client
	*hitClient = qtrue;

	// update the fade timer
	cg.crosshairClientNum  = trace.entityNum;
	cg.crosshairClientTime = cg.time;
	if (cg.crosshairClientNum != cg.snap->ps.identifyClient && cg.crosshairClientNum != ENTITYNUM_WORLD)
	{
		cg.identifyClientRequest = cg.crosshairClientNum;
	}

	cent = &cg_entities[cg.crosshairClientNum];

	if (cent && (cent->currentState.powerups & (1 << PW_OPS_DISGUISED)))
	{
		if (cgs.clientinfo[cg.crosshairClientNum].team == cgs.clientinfo[cg.clientNum].team)
		{
			cg.crosshairClientNoShoot = qtrue;
		}
	}

	return dist;
}

/**
 * @brief CG_CheckForCursorHints
 * @note Concept in progress...
 */
void CG_CheckForCursorHints(void)
{
	trace_t   trace;
	vec3_t    start, end;
	centity_t *tracent;
	float     dist;

	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg.snap->ps.serverCursorHint)      // server is dictating a cursor hint, use it.
	{
		cg.cursorHintTime  = cg.time;
		cg.cursorHintFade  = 500;   // fade out time
		cg.cursorHintIcon  = cg.snap->ps.serverCursorHint;
		cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
		return;
	}

	// From here on it's client-side cursor hints.  So if the server isn't sending that info (as an option)
	// then it falls into here and you can get basic cursorhint info if you want, but not the detailed info
	// the server sends.

	// the trace
	VectorCopy(cg.refdef_current->vieworg, start);
	VectorMA(start, CH_DIST, cg.refdef_current->viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_PLAYERSOLID);

	dist = trace.fraction * CH_DIST;

	if (trace.fraction == 1.f)
	{
		// might be water
		if ((CG_PointContents(trace.endpos, -1) & CONTENTS_WATER) && !(CG_PointContents(cg.refdef.vieworg, -1) & CONTENTS_WATER))
		{
			if (dist <= CH_WATER_DIST)
			{
				cg.cursorHintIcon  = HINT_WATER;
				cg.cursorHintTime  = cg.time;
				cg.cursorHintFade  = 500;
				cg.cursorHintValue = 0;
			}
		}
		return;
	}

	tracent = &cg_entities[trace.entityNum];

	// invisible entities don't show hints
	if (trace.entityNum >= MAX_CLIENTS &&
	    (tracent->currentState.powerups == STATE_INVISIBLE || tracent->currentState.powerups == STATE_UNDERCONSTRUCTION))
	{
		return;
	}

	// world
	if (trace.entityNum == ENTITYNUM_WORLD)
	{
		if ((trace.surfaceFlags & SURF_LADDER) && !(cg.snap->ps.pm_flags & PMF_LADDER))
		{
			if (dist <= CH_LADDER_DIST)
			{
				cg.cursorHintIcon  = HINT_LADDER;
				cg.cursorHintTime  = cg.time;
				cg.cursorHintFade  = 500;
				cg.cursorHintValue = 0;
			}
		}
	}
	else if (trace.entityNum < MAX_CLIENTS)       // people
	{   // knife
		if (GetWeaponTableData(cg.snap->ps.weapon)->type & WEAPON_TYPE_MELEE)
		{
			if (dist <= CH_KNIFE_DIST)
			{
				vec3_t pforward, eforward;

				AngleVectors(cg.snap->ps.viewangles, pforward, NULL, NULL);
				AngleVectors(tracent->lerpAngles, eforward, NULL, NULL);

				if (DotProduct(eforward, pforward) > 0.6f)           // from behind(-ish)
				{
					cg.cursorHintIcon  = HINT_KNIFE;
					cg.cursorHintTime  = cg.time;
					cg.cursorHintFade  = 100;
					cg.cursorHintValue = 0;
				}
			}
		}
	}
}

/**
 * @brief Draw the crosshaire health bar
 * @param[in] position
 * @param[in] health
 * @param[in] maxHealth
 */
static void CG_DrawCrosshairHealthBar(float position, int health, int maxHealth)
{
	float  *color;
	vec4_t bgcolor, c;
	float  barFrac = (float)health / maxHealth;

	// draw the name of the player being looked at
	color = CG_FadeColor(cg.crosshairClientTime, 1000);

	if (!color)
	{
		trap_R_SetColor(NULL);
		return;
	}

	if (barFrac > 1.0f)
	{
		barFrac = 1.0;
	}
	else if (barFrac < 0)
	{
		barFrac = 0;
	}

	c[0] = 1.0f;
	c[1] = c[2] = barFrac;
	c[3] = (0.25f + barFrac * 0.5f) * color[3];

	Vector4Set(bgcolor, 1.f, 1.f, 1.f, .25f * color[3]);

	// - 110/2
	CG_FilledBar(position - 55, 190, 110, 10, c, NULL, bgcolor, barFrac, 16);
}

/**
 * @brief CG_DrawCrosshairPlayerInfo
 * @param[in] clientNum
 * @param[in] class
 */
static void CG_DrawCrosshairPlayerInfo(int clientNum, int class)
{
	float      *color;
	float      w;
	const char *s;
	int        playerHealth = 0;
	int        maxHealth    = 1;
	qboolean   hasRank      = qfalse;
	float      middle       = 320 + cgs.wideXoffset;
	float      fontScale    = cg_fontScaleCN.value;

	// draw the name of the player being looked at
	color = CG_FadeColor(cg.crosshairClientTime, 1000);

	if (!color)
	{
		trap_R_SetColor(NULL);
		return;
	}

	// draw the name and class
	if (cg_drawCrosshairNames.integer > 0)
	{
		char   colorized[32]         = { 0 };
		size_t colorizedBufferLength = 32;

		if (cg_drawCrosshairNames.integer == 2)
		{
			// Draw them with full colors
			s = va("%s", cgs.clientinfo[clientNum].name);
		}
		else
		{
			// Draw them with a single color (white)
			Q_ColorizeString('7', cgs.clientinfo[clientNum].cleanname, colorized, colorizedBufferLength);
			s = colorized;
		}
		w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);

		CG_Text_Paint_Ext(middle - w / 2, 182, fontScale, fontScale, color, s, 0, 0, 0, &cgs.media.limboFont2);
	}
	if (cg_drawCrosshairInfo.integer & CROSSHAIR_CLASS)
	{
		// - 16 - 110/2
		CG_DrawPic(middle - 71, 187, 16, 16, cgs.media.skillPics[SkillNumForClass(class)]);
	}
	if (cgs.clientinfo[clientNum].rank > 0 && (cg_drawCrosshairInfo.integer & CROSSHAIR_RANK))
	{
		//  + 110/2
		CG_DrawPic(middle + 55, 187, 16, 16, rankicons[cgs.clientinfo[clientNum].rank][cgs.clientinfo[clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
	}
#ifdef FEATURE_PRESTIGE
	if (cgs.prestige && cgs.clientinfo[clientNum].prestige > 0 && (cg_drawCrosshairInfo.integer & CROSSHAIR_PRESTIGE))
	{
		hasRank = (cg_drawCrosshairInfo.integer & CROSSHAIR_RANK) && cgs.clientinfo[clientNum].rank > 0;
		// + 110/2
		CG_DrawPic(middle + 55 + (hasRank ? 18 : 0), 187, 16, 16, cgs.media.prestigePics[0]);
		CG_Text_Paint_Ext(middle + 71 + (hasRank ? 18 : 0), 198, fontScale, fontScale, color, va("%d", cgs.clientinfo[clientNum].prestige), 0, 0, 0, &cgs.media.limboFont2);
	}
#endif

	// set the health
	if (cg.crosshairClientNum == cg.snap->ps.identifyClient)
	{
		playerHealth = cg.snap->ps.identifyClientHealth;

		// identifyClientHealth is sent as unsigned char and negative numbers are not transmitted - see SpectatorThink()
		// this results in player health values behind max health
		// adjust dead player health bogus values for the health bar
		if (playerHealth > 156)
		{
			playerHealth = 0;
		}
	}
	else
	{
		// only team mate health is transmit in clientinfo, this cause a slight refresh delay for disguised ennemy
		// until they are identified through crosshair check on game side, which is connection depend
		playerHealth = cgs.clientinfo[cg.crosshairClientNum].health;
	}

	maxHealth = CG_GetPlayerMaxHealth(clientNum, class, cgs.clientinfo[clientNum].team);

	CG_DrawCrosshairHealthBar(middle, playerHealth, maxHealth);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawCrosshairNames
 */
static void CG_DrawCrosshairNames(void)
{
	float      *color;
	float      w;
	const char *s = NULL;
	float      dist; // Distance to the entity under the crosshair
	float      zChange;
	qboolean   hitClient = qfalse;
	float      middle    = 320 + cgs.wideXoffset;
	float      fontScale = cg_fontScaleCN.value;

	if (cg_drawCrosshair.integer < 0)
	{
		return;
	}

	// dyna > mine
	if (cg.crosshairDyna > -1)
	{
		color = CG_FadeColor(cg.crosshairDynaTime, 1000);

		if (!color)
		{
			trap_R_SetColor(NULL);
			return;
		}

		s = va(CG_TranslateString("%s^7\'s dynamite"), cgs.clientinfo[cg.crosshairDyna].name);
		w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(middle - w / 2, 182, fontScale, fontScale, color, s, 0, 0, 0, &cgs.media.limboFont2);

		cg.crosshairDyna = -1;
		return;
	}

	// mine id's
	if (cg.crosshairMine > -1)
	{
		color = CG_FadeColor(cg.crosshairMineTime, 1000);

		if (!color)
		{
			trap_R_SetColor(NULL);
			return;
		}

		s = va(CG_TranslateString("%s^7\'s mine"), cgs.clientinfo[cg.crosshairMine].name);
		w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(middle - w / 2, 182, fontScale, fontScale, color, s, 0, 0, 0, &cgs.media.limboFont2);

		cg.crosshairMine = -1;
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	dist = CG_ScanForCrosshairEntity(&zChange, &hitClient);

	// world-entity or no-entity
	if (cg.crosshairClientNum < 0)
	{
		return;
	}

	// don't draw crosshair names in shoutcast mode
	// shoutcasters can see tank and truck health
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && cgs.clientinfo[cg.clientNum].shoutcaster &&
	    cg_entities[cg.crosshairClientNum].currentState.eType != ET_MOVER)
	{
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (cg.crosshairClientNum >= MAX_CLIENTS)
	{
		if (!cg_drawCrosshairNames.integer && !cg_drawCrosshairInfo.integer)
		{
			return;
		}

		// draw the name of the player being looked at
		color = CG_FadeColor(cg.crosshairClientTime, 1000);

		if (!color)
		{
			trap_R_SetColor(NULL);
			return;
		}

		if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			if (cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.effect1Time)
			{
				if (cg_drawCrosshairNames.integer > 0)
				{
					s = Info_ValueForKey(CG_ConfigString(CS_SCRIPT_MOVER_NAMES), va("%i", cg.crosshairClientNum));
				}

				CG_DrawCrosshairHealthBar(middle, cg_entities[cg.crosshairClientNum].currentState.dl_intensity, 255);
			}
			else if (cg_entities[cg.crosshairClientNum].currentState.eType == ET_CONSTRUCTIBLE_MARKER)
			{
				if (cg_drawCrosshairNames.integer > 0)
				{
					s = Info_ValueForKey(CG_ConfigString(CS_CONSTRUCTION_NAMES), va("%i", cg.crosshairClientNum));
				}
			}

			if (s && *s)
			{
				w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);
				CG_Text_Paint_Ext(middle - w / 2, 182, fontScale, fontScale, color, s, 0, 0, 0, &cgs.media.limboFont2);
			}
		}
		return;
	}

	// crosshair for disguised enemy
	if (cgs.clientinfo[cg.crosshairClientNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team)
	{
		if (!(cg_entities[cg.crosshairClientNum].currentState.powerups & (1 << PW_OPS_DISGUISED)) ||
		    cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		{
			return;
		}

		if (cgs.clientinfo[cg.snap->ps.clientNum].skill[SK_SIGNALS] >= 4 && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
		{
			// draw the name of the player being looked at
			color = CG_FadeColor(cg.crosshairClientTime, 1000);

			if (!color)
			{
				trap_R_SetColor(NULL);
				return;
			}

			s = CG_TranslateString("Disguised Enemy!");
			w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(middle - w / 2, 182, fontScale, fontScale, color, s, 0, 0, 0, &cgs.media.limboFont2);
			return;
		}

		if (!cg_drawCrosshairNames.integer && !cg_drawCrosshairInfo.integer)
		{
			return;
		}

		CG_DrawCrosshairPlayerInfo(cgs.clientinfo[cg.crosshairClientNum].disguiseClientNum, (cg_entities[cg.crosshairClientNum].currentState.powerups >> PW_OPS_CLASS_1) & 7);
	}
	// we only want to see players on our team
	else if (!(cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR && cgs.clientinfo[cg.crosshairClientNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team))
	{
		if (!cg_drawCrosshairNames.integer && !cg_drawCrosshairInfo.integer)
		{
			return;
		}

		CG_DrawCrosshairPlayerInfo(cg.crosshairClientNum, cgs.clientinfo[cg.crosshairClientNum].cls);
	}
}

//==============================================================================

/**
 * @brief CG_DrawSpectator
 */
static void CG_DrawSpectator(void)
{
	const char *s;
#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingWeaponCam)
	{
		s = CG_TranslateString("WEAPONCAM");
	}
	else if (cgs.demoCamera.renderingFreeCam)
	{
		s = CG_TranslateString("FREECAM");
	}
	else
	{
#endif
	s = CG_TranslateString(va("%s", "SPECTATOR"));
#ifdef FEATURE_EDV
}
#endif
	int w = CG_Text_Width_Ext(va("%s", s), cg_fontScaleTP.value, 0, &cgs.media.limboFont2);

	CG_Text_Paint_Ext(Ccg_WideX(320) - w / 2, 440, cg_fontScaleTP.value, cg_fontScaleTP.value, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawVote
 */
static void CG_DrawVote(void)
{
	const char    *str;
	char          str1[32], str2[32];
	float         x, y, charHeight, fontScale;
	hudStucture_t *activehud;

	activehud = CG_GetActiveHUD();

	fontScale  = activehud->votetext.location.h;
	charHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	y          = activehud->votetext.location.y;
	x          = activehud->votetext.location.x;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		y = 180;
	}

	if (cgs.complaintEndTime > cg.time && !cg.demoPlayback && cg_complaintPopUp.integer > 0 && cgs.complaintClient >= 0)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = va(CG_TranslateString("File complaint against ^7%s^3 for team-killing?"), cgs.clientinfo[cgs.complaintClient].name);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient >= 0)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = va(CG_TranslateString("Accept %s^3's application to join your fireteam?"), cgs.clientinfo[cgs.applicationClient].name);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient >= 0)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = va(CG_TranslateString("Accept %s^3's proposition to invite %s^3 to join your fireteam?"), cgs.clientinfo[cgs.propositionClient2].name, cgs.clientinfo[cgs.propositionClient].name);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient >= 0)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = va(CG_TranslateString("Accept %s^3's invitation to join their fireteam?"), cgs.clientinfo[cgs.invitationClient].name);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.autoFireteamEndTime > cg.time && cgs.autoFireteamNum == -1)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = CG_TranslateString("Make Fireteam private?");
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.autoFireteamCreateEndTime > cg.time && cgs.autoFireteamCreateNum == -1)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = CG_TranslateString("Create a Fireteam?");
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.autoFireteamJoinEndTime > cg.time && cgs.autoFireteamJoinNum == -1)
	{
		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		str = CG_TranslateString("Join a Fireteam?");
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		y += charHeight * 2.0f;

		str = va(CG_TranslateString("Press '%s' for YES, or '%s' for NO"), str1, str2);
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}

	if (cgs.voteTime)
	{
		int sec;

		Q_strncpyz(str1, Binding_FromName("vote yes"), 32);
		Q_strncpyz(str2, Binding_FromName("vote no"), 32);

		// play a talk beep whenever it is modified
		if (cgs.voteModified)
		{
			cgs.voteModified = qfalse;
		}

		sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
		if (sec < 0)
		{
			// expired votetime
			cgs.voteTime                  = 0;
			cgs.complaintEndTime          = 0;
			cgs.applicationEndTime        = 0;
			cgs.propositionEndTime        = 0;
			cgs.invitationEndTime         = 0;
			cgs.autoFireteamEndTime       = 0;
			cgs.autoFireteamCreateEndTime = 0;
			cgs.autoFireteamJoinEndTime   = 0;
			//sec = 0;
			return;
		}

		if (!Q_stricmpn(cgs.voteString, "kick", 4))
		{
			if (strlen(cgs.voteString) > 5)
			{
				int  nameindex;
				char buffer[128];

				Q_strncpyz(buffer, cgs.voteString + 5, sizeof(buffer));
				Q_CleanStr(buffer);

				for (nameindex = 0; nameindex < MAX_CLIENTS; nameindex++)
				{
					if (!cgs.clientinfo[nameindex].infoValid)
					{
						continue;
					}

					if (!Q_stricmp(cgs.clientinfo[nameindex].cleanname, buffer))
					{
						if (cgs.clientinfo[nameindex].team != TEAM_SPECTATOR && cgs.clientinfo[nameindex].team != cgs.clientinfo[cg.clientNum].team)
						{
							return;
						}
					}
				}
			}
		}

		if (!(cg.snap->ps.eFlags & EF_VOTED))
		{
			str = va(CG_TranslateString("VOTE(%i): %s"), sec, cgs.voteString);
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			y += charHeight * 2;

			if (cgs.clientinfo[cg.clientNum].team != TEAM_AXIS && cgs.clientinfo[cg.clientNum].team != TEAM_ALLIES)
			{

				str = va(CG_TranslateString("YES:%i, NO:%i"), cgs.voteYes, cgs.voteNo);
				CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
				y += charHeight * 2;

				str = CG_TranslateString(va("Can't vote as %s", cgs.clientinfo[cg.clientNum].shoutcaster ? "Shoutcaster" : "Spectator"));
			}
			else
			{
				str = va(CG_TranslateString("YES(%s):%i, NO(%s):%i"), str1, cgs.voteYes, str2, cgs.voteNo);
			}
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
		else
		{
			str = va(CG_TranslateString("YOU VOTED ON: %s"), cgs.voteString);
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			y += charHeight * 2;


			str = va(CG_TranslateString("Y:%i, N:%i"), cgs.voteYes, cgs.voteNo);
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.complaintEndTime > cg.time && !cg.demoPlayback && cg_complaintPopUp.integer > 0 && cgs.complaintClient < 0)
	{
		if (cgs.complaintClient == -1)
		{
			str = CG_TranslateString("Your complaint has been filed");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
		if (cgs.complaintClient == -2)
		{
			str = CG_TranslateString("Complaint dismissed");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
		if (cgs.complaintClient == -3)
		{
			str = CG_TranslateString("Server Host cannot be complained against");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
		if (cgs.complaintClient == -4)
		{
			str = CG_TranslateString("You were team-killed by the Server Host");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
		if (cgs.complaintClient == -5)
		{
			str = CG_TranslateString("You were team-killed by a bot.");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient < 0)
	{
		if (cgs.applicationClient == -1)
		{
			str = CG_TranslateString("Your application has been submitted");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.applicationClient == -2)
		{
			str = CG_TranslateString("Your application failed");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.applicationClient == -3)
		{
			str = CG_TranslateString("Your application has been approved");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.applicationClient == -4)
		{
			str = CG_TranslateString("Your application reply has been sent");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient < 0)
	{
		if (cgs.propositionClient == -1)
		{
			str = CG_TranslateString("Your proposition has been submitted");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.propositionClient == -2)
		{
			str = CG_TranslateString("Your proposition was rejected");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.propositionClient == -3)
		{
			str = CG_TranslateString("Your proposition was accepted");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.propositionClient == -4)
		{
			str = CG_TranslateString("Your proposition reply has been sent");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient < 0)
	{
		if (cgs.invitationClient == -1)
		{
			str = CG_TranslateString("Your invitation has been submitted");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.invitationClient == -2)
		{
			str = CG_TranslateString("Your invitation was rejected");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.invitationClient == -3)
		{
			str = CG_TranslateString("Your invitation was accepted");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.invitationClient == -4)
		{
			str = CG_TranslateString("Your invitation reply has been sent");
			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			return;
		}

		if (cgs.invitationClient < 0)
		{
			return;
		}
	}

	if ((cgs.autoFireteamEndTime > cg.time && cgs.autoFireteamNum == -2) || (cgs.autoFireteamCreateEndTime > cg.time && cgs.autoFireteamCreateNum == -2) || (cgs.autoFireteamJoinEndTime > cg.time && cgs.autoFireteamJoinNum == -2))
	{
		str = CG_TranslateString("Response Sent");
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorYellow, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		return;
	}
}

/**
 * @brief CG_DrawIntermission
 */
static void CG_DrawIntermission(void)
{
	// End-of-level autoactions
	if (!cg.demoPlayback)
	{
		static int doScreenshot = 0, doDemostop = 0;

		if (!cg.latchAutoActions)
		{
			cg.latchAutoActions = qtrue;

			if (cg_autoAction.integer & AA_SCREENSHOT)
			{
				doScreenshot = cg.time + 1000;
			}

			if (cg_autoAction.integer & AA_STATSDUMP)
			{
				CG_dumpStats_f();
			}

			if ((cg_autoAction.integer & AA_DEMORECORD) &&
			    ((cgs.gametype == GT_WOLF_STOPWATCH && cgs.currentRound == 0) ||
			     cgs.gametype != GT_WOLF_STOPWATCH))
			{
				doDemostop = cg.time + 5000;    // stats should show up within 5 seconds
			}
		}

		if (doScreenshot > 0 && doScreenshot < cg.time)
		{
			CG_autoScreenShot_f();
			doScreenshot = 0;
		}

		if (doDemostop > 0 && doDemostop < cg.time)
		{
			trap_SendConsoleCommand("stoprecord\n");
			doDemostop = 0;
		}
	}

	// Intermission view
	CG_Debriefing_Draw();
}

/**
 * @brief CG_DrawSpectatorMessage
 */
static void CG_DrawSpectatorMessage(void)
{
	const char    *str, *str2;
	static int    lastconfigGet = 0;
	float         x, y, charHeight, fontScale;
	hudStucture_t *activehud;

	activehud = CG_GetActiveHUD();

	fontScale  = activehud->spectatortext.location.h;
	charHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	y          = activehud->spectatortext.location.y;
	x          = activehud->spectatortext.location.x;

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}
#endif

	if (!cg_descriptiveText.integer)
	{
		return;
	}

	if (!((cg.snap->ps.pm_flags & PMF_LIMBO) || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR))
	{
		return;
	}

	if (cg.time - lastconfigGet > 1000)
	{
		Controls_GetConfig();

		lastconfigGet = cg.time;
	}

	str2 = Binding_FromName("openlimbomenu");
	if (!Q_stricmp(str2, "(openlimbomenu)"))
	{
		str2 = "ESCAPE";
	}
	str = va(CG_TranslateString("Press %s to open Limbo Menu"), str2);
	CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	y += charHeight * 2;

	str2 = Binding_FromName("+attack");

	// we are looking at a client
	if (!cg.crosshairNotLookingAtClient && cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		// draw name with full colors
		str = va(CG_TranslateString("Press %s to follow %s"), str2, cgs.clientinfo[cg.crosshairClientNum].name);
	}
	else
	{
		str = va(CG_TranslateString("Press %s to follow next player"), str2);
	}

	CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	y += charHeight * 2;

	str2 = Binding_FromName("weapalt");
	str  = va(CG_TranslateString("Press %s to follow previous player"), str2);
	CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

#ifdef FEATURE_MULTIVIEW
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && cgs.mvAllowed)
	{
		y += charHeight * 2;

		str2 = Binding_FromName("mvactivate");
		str  = va(CG_TranslateString("Press %s to %s multiview mode"), str2, ((cg.mvTotalClients > 0) ? CG_TranslateString("disable") : CG_TranslateString("activate")));
		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		//y += charHeight * 2;
	}
#endif
}

/**
 * @brief CG_CalculateReinfTime_Float
 * @param[in] menu
 * @return
 */
float CG_CalculateReinfTime_Float(qboolean menu)
{
	team_t team;
	int    dwDeployTime;

	if (menu)
	{
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
		{
			team = cgs.ccSelectedTeam == 0 ? TEAM_AXIS : TEAM_ALLIES;
		}
		else
		{
			team = cgs.clientinfo[cg.clientNum].team;
		}
	}
	else
	{
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}

	dwDeployTime = (team == TEAM_AXIS) ? cg_redlimbotime.integer : cg_bluelimbotime.integer;
	return (1 + (dwDeployTime - ((cgs.aReinfOffset[team] + cg.time - cgs.levelStartTime) % dwDeployTime)) * 0.001f);
}

/**
 * @brief CG_CalculateReinfTime
 * @param menu
 * @return
 */
int CG_CalculateReinfTime(qboolean menu)
{
	return (int)(CG_CalculateReinfTime_Float(menu));
}

/**
 * @brief CG_CalculateShoutcasterReinfTime
 * @param[in] team
 * @return
 */
int CG_CalculateShoutcasterReinfTime(team_t team)
{
	int dwDeployTime;

	dwDeployTime = (team == TEAM_AXIS) ? cg_redlimbotime.integer : cg_bluelimbotime.integer;
	return (int)(1 + (dwDeployTime - ((cgs.aReinfOffset[team] + cg.time - cgs.levelStartTime) % dwDeployTime)) * 0.001f);
}

/**
 * @brief CG_DrawLimboMessage
 */
static void CG_DrawLimboMessage(void)
{
	const char    *str;
	playerState_t *ps = &cg.snap->ps;
	float         x, y, charHeight, fontScale;
	hudStucture_t *activehud;

#ifdef FEATURE_EDV
	if (cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
	{
		return;
	}
#endif

	activehud = CG_GetActiveHUD();

	fontScale  = activehud->limbotext.location.h;
	charHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	y          = activehud->limbotext.location.y;
	x          = activehud->limbotext.location.x;

	if (ps->stats[STAT_HEALTH] > 0)
	{
		return;
	}

	if ((cg.snap->ps.pm_flags & PMF_LIMBO) || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg_descriptiveText.integer)
	{
		str = CG_TranslateString("You are wounded and waiting for a medic.");
		CG_Text_Paint_Ext(x, y + charHeight * 2.0f, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		if (cgs.gametype == GT_WOLF_LMS)
		{
			trap_R_SetColor(NULL);
			return;
		}

		str = va(CG_TranslateString("Press %s to go into reinforcement queue."), Binding_FromName("+moveup"));
		CG_Text_Paint_Ext(x, y + 2 * charHeight * 2.0f, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}
	else if (cgs.gametype == GT_WOLF_LMS)
	{
		trap_R_SetColor(NULL);
		return;
	}

	if (ps->persistant[PERS_RESPAWNS_LEFT] == 0)
	{
		str = CG_TranslateString("No more reinforcements this round.");
	}
	else
	{
		int reinfTime = CG_CalculateReinfTime(qfalse);

		// coloured respawn counter when deployment is close to bring attention to next respawn
		if (reinfTime > 2)
		{
			str = va(CG_TranslateString("Deploying in ^3%d ^7seconds"), reinfTime);
		}
		else if (reinfTime > 1)
		{
			str = va(CG_TranslateString("Deploying in %s%d ^7seconds"), cgs.clientinfo[cg.clientNum].health == 0 ? "^1" : "^3", reinfTime);
		}
		else
		{
			str = va(CG_TranslateString("Deploying in %s%d ^7second"), cgs.clientinfo[cg.clientNum].health == 0 ? "^1" : "^3", reinfTime);
		}
	}

	CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawFollow
 * @return
 */
static qboolean CG_DrawFollow(void)
{
	float         x, y, charHeight, fontScale;
	hudStucture_t *activehud;

	activehud = CG_GetActiveHUD();

	fontScale  = activehud->followtext.location.h;
	charHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	y          = activehud->followtext.location.y;
	x          = activehud->followtext.location.x;

#ifdef FEATURE_MULTIVIEW
	// MV following info for mainview
	if (CG_ViewingDraw())
	{
		return qtrue;
	}
#endif

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return qfalse;
	}

	// Spectators view teamflags
	if (cg.snap->ps.clientNum != cg.clientNum && cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES)
		{
			CG_DrawPic(x + 1, y - charHeight * 2.0f - 12, 18, 12, cgs.media.alliedFlag);
		}
		else
		{
			CG_DrawPic(x + 1, y - charHeight * 2.0f - 12, 18, 12, cgs.media.axisFlag);
		}

		CG_DrawRect_FixedBorder(x, y - charHeight * 2.0f - 13, 20, 14, 1, HUD_Border);
	}

	// if in limbo, show different follow message
	if (cg.snap->ps.pm_flags & PMF_LIMBO)
	{
		char deploytime[128];

		if (cgs.gametype != GT_WOLF_LMS)
		{
			if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0)
			{
				if (cg.snap->ps.persistant[PERS_RESPAWNS_PENALTY] >= 0)
				{
					int deployTime   = ((cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS) ? cg_redlimbotime.integer : cg_bluelimbotime.integer) / 1000;
					int reinfDepTime = CG_CalculateReinfTime(qfalse) + cg.snap->ps.persistant[PERS_RESPAWNS_PENALTY] * deployTime;

					if (reinfDepTime > 1)
					{
						sprintf(deploytime, CG_TranslateString("Bonus Life! Deploying in ^3%d ^7seconds"), reinfDepTime);
					}
					else
					{
						sprintf(deploytime, CG_TranslateString("Bonus Life! Deploying in ^3%d ^7second"), reinfDepTime);
					}
				}
				else
				{
					sprintf(deploytime, "%s", CG_TranslateString("No more deployments this round"));
				}
			}
			else
			{
				int reinfTime = CG_CalculateReinfTime(qfalse);

				// coloured respawn counter when deployment is close to bring attention to next respawn
				if (reinfTime > 1)
				{
					sprintf(deploytime, CG_TranslateString("Deploying in ^3%d ^7seconds"), reinfTime);
				}
				else
				{
					sprintf(deploytime, CG_TranslateString("Deploying in ^3%d ^7second"), reinfTime);
				}
			}

			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, deploytime, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			y += charHeight * 2.0f;
		}

		// Don't display if you're following yourself
		if (cg.snap->ps.clientNum != cg.clientNum)
		{
			const char *follow    = CG_TranslateString("Following");
			char       *w         = cgs.clientinfo[cg.snap->ps.clientNum].name;
			int        charWidth  = CG_Text_Width_Ext("A", fontScale, 0, &cgs.media.limboFont2);
			int        startClass = CG_Text_Width_Ext(va("(%s", follow), fontScale, 0, &cgs.media.limboFont2) + charWidth;
			int        startRank  = CG_Text_Width_Ext(w, fontScale, 0, &cgs.media.limboFont2) + 14 + 2 * charWidth;
			int        endRank;

			CG_DrawPic(x + startClass, y - 10, 14, 14, cgs.media.skillPics[SkillNumForClass(cgs.clientinfo[cg.snap->ps.clientNum].cls)]);

			if (cgs.clientinfo[cg.snap->ps.clientNum].rank > 0)
			{
				CG_DrawPic(x + startClass + startRank, y - 10, 14, 14, rankicons[cgs.clientinfo[cg.snap->ps.clientNum].rank][cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
				endRank = 14;
			}
			else
			{
				endRank = -charWidth;
			}

			CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, va("(%s", follow), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(x + startClass + 14 + charWidth, y, fontScale, fontScale, colorWhite, w, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(x + startClass + startRank + endRank, y, fontScale, fontScale, colorWhite, ")", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		}
	}
	else
	{
		const char *follow    = CG_TranslateString("Following");
		char       *w         = cgs.clientinfo[cg.snap->ps.clientNum].name;
		int        charWidth  = CG_Text_Width_Ext("A", fontScale, 0, &cgs.media.limboFont2);
		int        startClass = CG_Text_Width_Ext(follow, fontScale, 0, &cgs.media.limboFont2) + charWidth;

		CG_DrawPic(x + startClass, y - 10, 14, 14, cgs.media.skillPics[SkillNumForClass(cgs.clientinfo[cg.snap->ps.clientNum].cls)]);

		if (cgs.clientinfo[cg.snap->ps.clientNum].rank > 0)
		{
			int startRank;

			startRank = CG_Text_Width_Ext(w, fontScale, 0, &cgs.media.limboFont2) + 14 + 2 * charWidth;
			CG_DrawPic(x + startClass + startRank, y - 10, 14, 14, rankicons[cgs.clientinfo[cg.snap->ps.clientNum].rank][cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS ? 1 : 0][0].shader);
		}

		CG_Text_Paint_Ext(x, y, fontScale, fontScale, colorWhite, follow, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		CG_Text_Paint_Ext(x + startClass + 14 + charWidth, y, fontScale, fontScale, colorWhite, w, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}

	return qtrue;
}

/**
 * @brief CG_DrawWarmup
 */
static void CG_DrawWarmup(void)
{
	int             w, x;
	int             sec = cg.warmup;
	const char      *s, *s1, *s2;
	static qboolean announced = qfalse;
	float           fontScale = cg_fontScaleTP.value;

	if (!sec)
	{
		if ((cgs.gamestate == GS_WARMUP && !cg.warmup) || cgs.gamestate == GS_WAITING_FOR_PLAYERS)
		{
			if (CG_ConfigString(CS_CONFIGNAME)[0])
			{
				s1 = va(CG_TranslateString("^3Config: ^7%s^7"), CG_ConfigString(CS_CONFIGNAME));
				w  = CG_Text_Width_Ext(s1, cg_fontScaleCP.value, 0, &cgs.media.limboFont2);
				x  = Ccg_WideX(320) - w / 2;
				CG_Text_Paint_Ext(x, 340, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, s1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			}

			if (cgs.minclients > 0)
			{
				s1 = va(CG_TranslateString("^3WARMUP:^7 Waiting on ^2%i^7 %s"), cgs.minclients, cgs.minclients == 1 ? CG_TranslateString("player") : CG_TranslateString("players"));
			}
			else
			{
				s1 = va("%s", CG_TranslateString("^3WARMUP:^7 All players ready!"));
			}

			w = CG_Text_Width_Ext(s1, fontScale, 0, &cgs.media.limboFont2);
			x = Ccg_WideX(320) - w / 2;
			CG_Text_Paint_Ext(x, 104, fontScale, fontScale, colorWhite, s1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

			if (!cg.demoPlayback && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			    (!(cg.snap->ps.pm_flags & PMF_FOLLOW) || (cg.snap->ps.pm_flags & PMF_LIMBO)))
			{
				char str1[32];
				int  y = 360;

				if (cg.snap->ps.eFlags & EF_READY)
				{
					s2 = CG_TranslateString("^2Ready");

					w = CG_Text_Width_Ext(s2, cg_fontScaleCP.value, 0, &cgs.media.limboFont2);
					x = Ccg_WideX(320) - w / 2;
					CG_Text_Paint_Ext(x, y, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, s2, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

					y += 20;

					Q_strncpyz(str1, Binding_FromName("notready"), 32);
					if (!Q_stricmp(str1, "(?" "?" "?)"))
					{
						s2 = CG_TranslateString("Type ^3\\notready^7 in the console to unready");
					}
					else
					{
						s2 = va(CG_TranslateString("^7Press ^3%s^7 to unready"), str1);
						s2 = CG_TranslateString(s2);
					}
				}
				else
				{
					Q_strncpyz(str1, Binding_FromName("ready"), 32);
					if (!Q_stricmp(str1, "(?" "?" "?)"))
					{
						s2 = CG_TranslateString("Type ^3\\ready^7 in the console to start");
					}
					else
					{
						s2 = va(CG_TranslateString("Press ^3%s^7 to start"), str1);
						s2 = CG_TranslateString(s2);
					}
				}

				w = CG_Text_Width_Ext(s2, cg_fontScaleCP.value, 0, &cgs.media.limboFont2);
				x = Ccg_WideX(320) - w / 2;
				CG_Text_Paint_Ext(x, y, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, s2, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			}
			return;
		}
		return;
	}

	sec = (sec - cg.time) / 1000;
	if (sec < 0)
	{
		sec = 0;
	}

	if (sec > 0)
	{
		s = va("%s %s%i", CG_TranslateString("^3WARMUP:^7 Match begins in"), sec  < 4 ? "^1" : "^2", sec);
	}
	else
	{
		s = CG_TranslateString("^3WARMUP:^7 Match begins now!"); // " Timelimit at start: ^3%i min", cgs.timelimit
	}

	w = CG_Text_Width_Ext(s, fontScale, 0, &cgs.media.limboFont2);
	x = Ccg_WideX(320) - w / 2;
	CG_Text_Paint_Ext(x, 208, fontScale, fontScale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

	// pre start actions
	if (sec == 10 && !announced)
	{
		if (cg_announcer.integer)
		{
			trap_S_StartLocalSound(cgs.media.countPrepare, CHAN_ANNOUNCER);
		}

		CPri(CG_TranslateString("^3PREPARE TO FIGHT!\n"));

		if (!cg.demoPlayback && (cg_autoAction.integer & AA_DEMORECORD))
		{
			CG_autoRecord_f();
		}

		announced = qtrue;
	}
	else if (sec != 10)
	{
		announced = qfalse;
	}

	// stopwatch stuff
	s1 = "";
	s2 = "";

	if (cgs.gametype == GT_WOLF_STOPWATCH)
	{
		const char *cs;
		int        defender;

		s = va("%s %i", CG_TranslateString("STOPWATCH ROUND"), cgs.currentRound + 1);

		cs       = CG_ConfigString(CS_MULTI_INFO);
		defender = Q_atoi(Info_ValueForKey(cs, "d")); // defender

		if (!defender)
		{
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Axis team";
					s2 = "Keep the Allies from beating the clock!";
				}
				else
				{
					s1 = "You are on the Axis team";
				}
			}
			else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Allied team";
					s2 = "Try to beat the clock!";
				}
				else
				{
					s1 = "You are on the Allied team";
				}
			}
		}
		else
		{
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Axis team";
					s2 = "Try to beat the clock!";
				}
				else
				{
					s1 = "You are on the Axis team";
				}
			}
			else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES)
			{
				if (cgs.currentRound == 1)
				{
					s1 = "You have been switched to the Allied team";
					s2 = "Keep the Axis from beating the clock!";
				}
				else
				{
					s1 = "You are on the Allied team";
				}
			}
		}

		if (strlen(s1))
		{
			s1 = CG_TranslateString(s1);
		}

		if (strlen(s2))
		{
			s2 = CG_TranslateString(s2);
		}

		w = CG_Text_Width_Ext(s, cg_fontScaleCP.value, 0, &cgs.media.limboFont2);
		x = Ccg_WideX(320) - w / 2;
		CG_Text_Paint_Ext(x, 144, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		w = CG_Text_Width_Ext(s1, cg_fontScaleCP.value, 0, &cgs.media.limboFont2);
		x = Ccg_WideX(320) - w / 2;
		CG_Text_Paint_Ext(x, 164, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, s1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		w = CG_Text_Width_Ext(s2, cg_fontScaleCP.value, 0, &cgs.media.limboFont2);
		x = Ccg_WideX(320) - w / 2;
		CG_Text_Paint_Ext(x, 184, cg_fontScaleCP.value, cg_fontScaleCP.value, colorWhite, s2, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}
}

//==================================================================================

/**
 * @brief CG_DrawFlashFade
 */
static void CG_DrawFlashFade(void)
{
	static int lastTime;
	qboolean   fBlackout = (int_ui_blackout.integer > 0);

	if (cgs.fadeStartTime + cgs.fadeDuration < cg.time)
	{
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
	else if (cgs.fadeAlphaCurrent != cgs.fadeAlpha)
	{
		int time;
		int elapsed = (time = trap_Milliseconds()) - lastTime;    // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading

		lastTime = time;
		if (elapsed < 500 && elapsed > 0)
		{
			if (cgs.fadeAlphaCurrent > cgs.fadeAlpha)
			{
				cgs.fadeAlphaCurrent -= ((float)elapsed / (float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent < cgs.fadeAlpha)
				{
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			}
			else
			{
				cgs.fadeAlphaCurrent += ((float)elapsed / (float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent > cgs.fadeAlpha)
				{
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			}
		}
	}

	// have to inform the ui that we need to remain blacked out (or not)
	if (int_ui_blackout.integer == 0)
	{
		if (
#ifdef FEATURE_MULTIVIEW
			cg.mvTotalClients < 1 &&
#endif
			cg.snap->ps.powerups[PW_BLACKOUT] > 0)
		{
			trap_Cvar_Set("ui_blackout", va("%d", cg.snap->ps.powerups[PW_BLACKOUT]));
		}
	}
	else if (cg.snap->ps.powerups[PW_BLACKOUT] == 0
#ifdef FEATURE_MULTIVIEW
	         || cg.mvTotalClients > 0
#endif
	         )
	{
		trap_Cvar_Set("ui_blackout", "0");
	}

	// now draw the fade
	if (cgs.fadeAlphaCurrent > 0.0f || fBlackout)
	{
		vec4_t col;

		VectorClear(col);
		col[3] = (fBlackout) ? 1.0f : cgs.fadeAlphaCurrent;
		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, col);

		// bail out if we're a speclocked spectator with cg_draw2d = 0
		if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR && !cg_draw2D.integer)
		{
			return;
		}

		// Show who is speclocked
		if (fBlackout)
		{
			int   i, nOffset = 90;
			char  *teams[TEAM_NUM_TEAMS] = { "??", "AXIS", "ALLIES", "???" }; // FIXME: translate team names? team name
			float color[4]               = { 1, 1, 0, 1 };

			for (i = TEAM_AXIS; i <= TEAM_ALLIES; i++)
			{
				if (cg.snap->ps.powerups[PW_BLACKOUT] & i)
				{
					int  w, x;
					char *s1;

					s1 = va(CG_TranslateString("The %s team is speclocked!"), teams[i]);
					w  = CG_Text_Width_Ext(s1, cg_fontScaleSP.value, 0, &cgs.media.limboFont2);
					x  = Ccg_WideX(320) - w / 2;

					CG_Text_Paint_Ext(x, nOffset, cg_fontScaleSP.value, cg_fontScaleSP.value, color, s1, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
					nOffset += 12;
				}
			}
		}
	}
}

/**
 * @brief Hide the snap transition from regular view to/from zoomed
 * @todo FIXME: TODO: use cg_fade?
 */
static void CG_DrawFlashZoomTransition(void)
{
	float frac;
	float fadeTime = 400.f;

	if (!cg.snap)
	{
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags))
	{
		// don't draw when on mg_42
		// keep the timer fresh so when you remove yourself from the mg42, it'll fade
		cg.zoomTime = cg.time;
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	frac = cg.time - cg.zoomTime;

	if (frac < fadeTime)
	{
		vec4_t color;

		frac = frac / fadeTime;
		Vector4Set(color, 0, 0, 0, 1.0f - frac);
		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, color);
	}
}

/**
 * @brief CG_DrawFlashDamage
 */
static void CG_DrawFlashDamage(void)
{
	if (!cg.snap)
	{
		return;
	}

	if (cg_bloodFlash.value <= 0.f || cg_bloodFlashTime.value <= 0.f)
	{
		return;
	}

	if (cg.v_dmg_time > cg.time)
	{
		vec4_t col = { 0.2f, 0.f, 0.f, 0.f };
		float  width;

		width = Ccg_WideX(SCREEN_WIDTH);

		col[3] = (1 - (cg.time - cg.v_dmg_time) / cg_bloodFlashTime.value) * Com_Clamp(0.f, 1.f, cg_bloodFlash.value);

		if (cg.v_dmg_angle == -1.f) // all borders
		{
			// horrible duplicate from differentes borders below
			GradientRound_Paint(width * 0.125f, -SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
			GradientRound_Paint(width - width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
			GradientRound_Paint(width - width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
			GradientRound_Paint(width - width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
			GradientRound_Paint(width * 0.125f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
			GradientRound_Paint(-width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
			GradientRound_Paint(-width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
			GradientRound_Paint(-width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle < 30 || cg.v_dmg_angle >= 330)  // top
		{
			GradientRound_Paint(width * 0.125f, -SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
		}
		else if (/*cg.v_dmg_angle >= 30 &&*/ cg.v_dmg_angle < 60)   // top right corner
		{
			GradientRound_Paint(width - width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle >= 60 && cg.v_dmg_angle < 120)  // right
		{
			GradientRound_Paint(width - width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
		}
		else if (cg.v_dmg_angle >= 120 && cg.v_dmg_angle < 150) // bottom right corner
		{
			GradientRound_Paint(width - width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle >= 150 && cg.v_dmg_angle < 210) // bottom
		{
			GradientRound_Paint(width * 0.125f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.125f, width * 0.75f, SCREEN_HEIGHT * 0.25f, col);
		}
		else if (cg.v_dmg_angle >= 210 && cg.v_dmg_angle < 240) // bottom left corner
		{
			GradientRound_Paint(-width * 0.33f, SCREEN_HEIGHT - SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
		else if (cg.v_dmg_angle >= 240 && cg.v_dmg_angle < 300) // left
		{
			GradientRound_Paint(-width * 0.125f, 0, width * 0.25f, SCREEN_HEIGHT, col);
		}
		else //if (cg.v_dmg_angle >= 300 && cg.v_dmg_angle < 330)   // top left corner
		{
			GradientRound_Paint(-width * 0.33f, -SCREEN_HEIGHT * 0.33f, width * 0.66f, SCREEN_HEIGHT * 0.66f, col);
		}
	}
}

/**
 * @brief CG_DrawFlashFire
 */
static void CG_DrawFlashFire(void)
{
	float alpha;

	if (!cg.snap)
	{
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	if (!cg.snap->ps.onFireStart)
	{
		cg.v_noFireTime = cg.time;
		return;
	}

	alpha = (float)((FIRE_FLASH_TIME - 1000) - (cg.time - cg.snap->ps.onFireStart)) / (FIRE_FLASH_TIME - 1000);
	if (alpha > 0)
	{
		vec4_t col = { 1, 1, 1, 1 };
		float  max, f;

		if (alpha >= 1.0f)
		{
			alpha = 1.0f;
		}

		// fade in?
		f = (float)(cg.time - cg.v_noFireTime) / FIRE_FLASH_FADEIN_TIME;
		if (f >= 0.0f && f < 1.0f)
		{
			alpha = f;
		}

		max = (float)(0.5 + 0.5 * sin((double)((cg.time / 10) % 1000) / 1000.0));
		if (alpha > max)
		{
			alpha = max;
		}
		col[0] = alpha;
		col[1] = alpha;
		col[2] = alpha;
		col[3] = alpha;
		trap_R_SetColor(col);
		CG_DrawPic(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, cgs.media.viewFlashFire[(cg.time / 50) % 16]);
		trap_R_SetColor(NULL);

		trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)(255.0f * alpha), 0);
	}
	else
	{
		cg.v_noFireTime = cg.time;
	}
}

/**
 * @brief Screen flash stuff drawn first (on top of world, behind HUD)
 */
static void CG_DrawFlashBlendBehindHUD(void)
{
	CG_DrawFlashZoomTransition();
	CG_DrawFlashFade();

	// no flash blends if in limbo or spectator, and in the limbo menu
	if (((cg.snap->ps.pm_flags & PMF_LIMBO) || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) && cg.showGameView)
	{
		return;
	}

	CG_DrawFlashFire();
	CG_DrawFlashDamage();
}

//#define OID_LEFT    10
#define OID_TOP     360

/**
 * @brief CG_ObjectivePrint
 * @param[in] str
 * @param[in] fontScale
 */
void CG_ObjectivePrint(const char *str, float fontScale)
{
	const char *s;

	if (cg.centerPrintTime)
	{
		return;
	}

	s = CG_TranslateString(str);

	Q_strncpyz(cg.oidPrint, s, sizeof(cg.oidPrint));
	cg.oidPrintLines      = CG_FormatCenterPrint(cg.oidPrint);
	cg.oidPrintTime       = cg.time;
	cg.oidPrintY          = OID_TOP;
	cg.oidPrintCharHeight = CG_Text_Height_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	cg.oidPrintCharWidth  = CG_Text_Width_Ext("A", fontScale, 0, &cgs.media.limboFont2);
	cg.oidPrintFontScale  = fontScale;
}

/**
 * @brief CG_DrawObjectiveInfo
 */
static void CG_DrawObjectiveInfo(void)
{
	char   *start, *end;
	int    len;
	char   currentColor[3] = S_COLOR_WHITE;
	char   nextColor[3] = { 0, 0, 0 };
	int    x, y, w;
	int    x1, y1, x2, y2;
	float  *color;
	vec4_t backColor = { 0.2f, 0.2f, 0.2f, 1.f };

	if (!cg.oidPrintTime)
	{
		return;
	}

	color = CG_FadeColor(cg.oidPrintTime, 250);
	if (!color)
	{
		cg.oidPrintTime = 0;
		return;
	}

	trap_R_SetColor(color);

	y = 400 - cg.oidPrintLines * cg.oidPrintCharHeight / 2;

	x1 = 319;
	y1 = y - cg.oidPrintCharHeight * 2;
	x2 = 321;

	// first just find the bounding rect
	for (start = end = cg.oidPrint; *start; end += len)
	{
		len = Q_UTF8_Width(end);

		if (!*end || *end == '\n')
		{
			char linebuffer[1024] = { 0 };

			Q_strncpyz(linebuffer, start, (end - start) + 1);

			w = CG_Text_Width_Ext(va("%s  ", linebuffer), cg.oidPrintFontScale, 0, &cgs.media.limboFont2);

			if (320 - w / 2 < x1)
			{
				x1 = 320 - w / 2;
				x2 = 320 + w / 2;
			}

			//x  = 320 - w / 2; // no need to calculate x - it's not used and overwritten in next loop
			y += cg.oidPrintCharHeight * 1.5;

			start = end + len;
		}
	}

	y2 = y - 2;

	VectorCopy(color, backColor);
	backColor[3] = 0.5f * color[3];
	trap_R_SetColor(backColor);

	CG_DrawPic(x1 + cgs.wideXoffset, y1, x2 - x1, y2 - y1, cgs.media.teamStatusBar);

	VectorSet(backColor, 0, 0, 0);
	CG_DrawRect(x1 + cgs.wideXoffset, y1, x2 - x1, y2 - y1, 1, backColor);

	trap_R_SetColor(color);

	// do the actual drawing
	y = 400 - cg.oidPrintLines * cg.oidPrintCharHeight / 2;

	for (start = end = cg.oidPrint; *start; end += len)
	{
		if (Q_IsColorString(end))
		{
			// don't store color if the line start with a color
			if (start == end)
			{
				Com_Memset(currentColor, 0, 3);
			}

			// store the used color to reused it in case the line is splitted on multi line
			Q_strncpyz(nextColor, end, 3);
			len = 2;
		}
		else
		{
			len = Q_UTF8_Width(end);
		}

		if (!*end || *end == '\n')
		{
			char linebuffer[1024] = { 0 };

			if (*currentColor)
			{
				Q_strncpyz(linebuffer, currentColor, 3);
				Q_strcat(linebuffer, (end - start) + 3, start);
			}
			else
			{
				Q_strncpyz(linebuffer, start, (end - start) + 1);
			}

			w = CG_Text_Width_Ext(linebuffer, cg.oidPrintFontScale, 0, &cgs.media.limboFont2);

			if (x1 + w > x2)
			{
				x2 = x1 + w;
			}

			x = 320 - w / 2;

			CG_Text_Paint_Ext(x + cgs.wideXoffset, y, cg.oidPrintFontScale, cg.oidPrintFontScale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

			y += cg.oidPrintCharHeight * 1.5;

			if (nextColor[0])
			{
				Q_strncpyz(currentColor, nextColor, 3);
			}

			start = end + len;
		}
	}

	trap_R_SetColor(NULL);
}

//==================================================================================

/**
 * @brief CG_DrawTimedMenus
 */
static void CG_DrawTimedMenus(void)
{
	if (cg.voiceTime)
	{
		if (cg.time - cg.voiceTime > 2500)
		{
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

/**
 * @brief CG_Fade
 * @param r - unused
 * @param g - unused
 * @param b - unused
 * @param[in] a
 * @param[in] time
 * @param[in] duration
 * @todo cleanup ?
 */
void CG_Fade(int r, int g, int b, int a, int time, int duration)
{
	// incorporate this into the current fade scheme

	cgs.fadeAlpha     = (float)a / 255.0f;
	cgs.fadeStartTime = time;
	cgs.fadeDuration  = duration;

	if (cgs.fadeStartTime + cgs.fadeDuration <= cg.time)
	{
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
	return; // FIXME: early return?

	/*
	if (time <= 0)      // do instantly
	{
	    cg.fadeRate = 1;
	    cg.fadeTime = cg.time - 1;  // set cg.fadeTime behind cg.time so it will start out 'done'
	}
	else
	{
	    cg.fadeRate = 1.0f / time;
	    cg.fadeTime = cg.time + time;
	}

	cg.fadeColor2[0] = ( float )r / 255.0f;
	cg.fadeColor2[1] = ( float )g / 255.0f;
	cg.fadeColor2[2] = ( float )b / 255.0f;
	cg.fadeColor2[3] = ( float )a / 255.0f;
	*/
}

/**
 * @brief CG_ScreenFade
 */
static void CG_ScreenFade(void)
{
	int msec;

	if (cg.fadeRate == 0.f)
	{
		return;
	}

	msec = cg.fadeTime - cg.time;
	if (msec <= 0)
	{
		cg.fadeColor1[0] = cg.fadeColor2[0];
		cg.fadeColor1[1] = cg.fadeColor2[1];
		cg.fadeColor1[2] = cg.fadeColor2[2];
		cg.fadeColor1[3] = cg.fadeColor2[3];

		if (cg.fadeColor1[3] == 0.f)
		{
			cg.fadeRate = 0;
			return;
		}

		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, cg.fadeColor1);

	}
	else
	{
		vec4_t color;
		float  t    = ( float )msec * cg.fadeRate;
		float  invt = 1.0f - t;
		int    i;

		for (i = 0; i < 4; i++)
		{
			color[i] = cg.fadeColor1[i] * t + cg.fadeColor2[i] * invt;
		}

		if (color[3] != 0.f)
		{
			CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, color);
		}
	}
}

static int lastDemoScoreTime = 0;

/**
 * @brief CG_DrawDemoRecording
 */
void CG_DrawDemoRecording(void)
{
	char status[1024];
	char demostatus[128];
	char wavestatus[128];

	if (!cl_demorecording.integer && !cl_waverecording.integer)
	{
		return;
	}

	// poll for score
	if (!lastDemoScoreTime || cg.time > lastDemoScoreTime)
	{
		trap_SendClientCommand("score");
		lastDemoScoreTime = cg.time + 5000; // 5 secs
	}

	if (!cg_recording_statusline.integer)
	{
		return;
	}

	if (cl_demorecording.integer)
	{
		Com_sprintf(demostatus, sizeof(demostatus), " demo %s: %ik ", cl_demofilename.string, cl_demooffset.integer / 1024);
	}
	else
	{
		Q_strncpyz(demostatus, "", sizeof(demostatus));

	}

	if (cl_waverecording.integer)
	{
		Com_sprintf(wavestatus, sizeof(demostatus), " audio %s: %ik ", cl_wavefilename.string, cl_waveoffset.integer / 1024);
	}
	else
	{
		Q_strncpyz(wavestatus, "", sizeof(wavestatus));
	}

	Com_sprintf(status, sizeof(status), "RECORDING%s%s", demostatus, wavestatus);

	CG_Text_Paint_Ext(10, cg_recording_statusline.integer, 0.2f, 0.2f, colorRed, status, 0, 0, 0, &cgs.media.limboFont2);
}

/**
 * @brief CG_DrawOnScreenLabels
 */
void CG_DrawOnScreenLabels(void)
{
	static vec3_t mins  = { -1, -1, -1 };
	static vec3_t maxs  = { 1, 1, 1 };
	vec4_t        white = { 1.0f, 1.0f, 1.0f, 1.0f };
	int           i;
	specLabel_t   *specLabel;
	trace_t       tr;
	int           FadeOut = 0;
	int           FadeIn  = 0;

	for (i = 0; i < cg.specStringCount; ++i)
	{
		specLabel = &cg.specOnScreenLabels[i];

		// Visible checks if information is actually valid
		if (!specLabel || !specLabel->visible)
		{
			continue;
		}

		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, specLabel->origin, -1, CONTENTS_SOLID);

		if (tr.fraction < 1.0f)
		{
			specLabel->lastInvisibleTime = cg.time;
		}
		else
		{
			specLabel->lastVisibleTime = cg.time;
		}

		FadeOut = cg.time - specLabel->lastVisibleTime;
		FadeIn  = cg.time - specLabel->lastInvisibleTime;

		if (FadeIn)
		{
			white[3] = (FadeIn > 500) ? 1.0 : FadeIn / 500.0f;
			if (white[3] < specLabel->alpha)
			{
				white[3] = specLabel->alpha;
			}
		}
		if (FadeOut)
		{
			white[3] = (FadeOut > 500) ? 0.0 : 1.0f - FadeOut / 500.0f;
			if (white[3] > specLabel->alpha)
			{
				white[3] = specLabel->alpha;
			}
		}
		if (white[3] > 1.0f)
		{
			white[3] = 1.0f;
		}

		specLabel->alpha = white[3];
		if (specLabel->alpha <= 0.0f)
		{
			continue;                           // no alpha = nothing to draw..
		}

		CG_Text_Paint_Ext(specLabel->x, specLabel->y, specLabel->scale, specLabel->scale, white, specLabel->text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		// expect update next frame again
		specLabel->visible = qfalse;
	}

	cg.specStringCount = 0;
}

/**
* @brief CG_DrawOnScreenBars
*/
void CG_DrawOnScreenBars(void)
{
	static vec3_t mins  = { -1, -1, -1 };
	static vec3_t maxs  = { 1, 1, 1 };
	vec4_t        white = { 1.0f, 1.0f, 1.0f, 1.0f };
	int           i;
	specBar_t     *specBar;
	trace_t       tr;
	int           FadeOut = 0;
	int           FadeIn  = 0;

	for (i = 0; i < cg.specBarCount; ++i)
	{
		specBar = &cg.specOnScreenBar[i];

		// Visible checks if information is actually valid
		if (!specBar || !specBar->visible)
		{
			continue;
		}

		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, specBar->origin, -1, CONTENTS_SOLID);

		if (tr.fraction < 1.0f)
		{
			specBar->lastInvisibleTime = cg.time;
		}
		else
		{
			specBar->lastVisibleTime = cg.time;
		}

		FadeOut = cg.time - specBar->lastVisibleTime;
		FadeIn  = cg.time - specBar->lastInvisibleTime;

		if (FadeIn)
		{
			white[3] = (FadeIn > 500) ? 1.0 : FadeIn / 500.0f;
			if (white[3] < specBar->alpha)
			{
				white[3] = specBar->alpha;
			}
		}
		if (FadeOut)
		{
			white[3] = (FadeOut > 500) ? 0.0 : 1.0f - FadeOut / 500.0f;
			if (white[3] > specBar->alpha)
			{
				white[3] = specBar->alpha;
			}
		}
		if (white[3] > 1.0f)
		{
			white[3] = 1.0f;
		}

		specBar->alpha = white[3];
		if (specBar->alpha <= 0.0f)
		{
			continue;                           // no alpha = nothing to draw..
		}

		CG_FilledBar(specBar->x, specBar->y, specBar->w, specBar->h, specBar->colorStart, specBar->colorEnd, specBar->colorBack, specBar->fraction, BAR_BG);

		// expect update next frame again
		specBar->visible = qfalse;
	}

	cg.specBarCount = 0;
}

/**
 * @brief CG_DrawBannerPrint
 */
static void CG_DrawBannerPrint(void)
{
	float *color;
	int   lineHeight;

	if ((cg_bannerTime.integer <= 0) || !cg.bannerPrintTime)
	{
		return;
	}

	color = CG_FadeColor(cg.bannerPrintTime, cg_bannerTime.integer);

	if (!color)
	{
		cg.bannerPrintTime = 0;
		return;
	}

	color[3] *= 0.75;

	lineHeight = ((float)CG_Text_Height_Ext("M", 0.23, 0, &cgs.media.limboFont2) * 1.5f);

	CG_DrawMultilineText(Ccg_WideX(320), 20, 0.23, 0.23, color, cg.bannerPrint, lineHeight, 0, 0, ITEM_TEXTSTYLE_SHADOWED, ITEM_ALIGN_CENTER, &cgs.media.limboFont2);
}

#define MAX_DISTANCE 2000.f

/**
 * @brief CG_DrawEnvironmentalAwareness
 */
static void CG_DrawEnvironmentalAwareness()
{
	snapshot_t *snap;
	int        i;

	if (!cg_drawEnvAwareness.integer)
	{
		return;
	}

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport)
	{
		snap = cg.nextSnap;
	}
	else
	{
		snap = cg.snap;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	for (i = 0; i < MAX_CLIENTS /*snap->numEntities*/; ++i)
	{
		vec3_t    dir;
		float     len;
		centity_t *cent = &cg_entities[snap->entities[i].number];
		qhandle_t icon;

		// skip self
		if (cg.clientNum == cent->currentState.clientNum)
		{
			continue;
		}

		VectorSubtract(cent->lerpOrigin, cg.predictedPlayerState.origin, dir);
		len = VectorLength(dir);

		// too far to draw
		//if (len > MAX_DISTANCE)
		//{
		//	continue;
		//}

		icon = CG_GetCompassIcon(cent, qfalse, qfalse);

		if (icon)
		{
			vec4_t  col = { 1.f, 1.f, 1.f, 1 - (len / MAX_DISTANCE) /*0.5f*/ };
			trace_t trace;
			vec3_t  angles;

			VectorNormalize(dir);
			vectoangles(dir, angles);

			if (dir[0] == 0.f && dir[1] == 0.f && dir[2] == 0.f)
			{
				continue;
			}

			AnglesSubtract(cg.predictedPlayerState.viewangles, angles, angles);

			// can we see the player
			if (angles[PITCH] >= -cg.refdef_current->fov_y / 2
			    && angles[PITCH] <= cg.refdef_current->fov_y / 2)
			{
				if (angles[YAW] >= -cg.refdef_current->fov_x / 2
				    && angles[YAW] <= cg.refdef_current->fov_x / 2)
				{
					float  front, left, up;
					float  x, y;
					int    skipNumber = cg.snap->ps.clientNum;
					vec3_t start;
					VectorCopy(cg.refdef.vieworg, start);

					// trace to the target player and ignore other players
					do
					{
						CG_Trace(&trace, start, NULL, NULL, cent->pe.headRefEnt.origin, skipNumber, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM);
						skipNumber = trace.entityNum;
						VectorCopy(trace.endpos, start);
					}
					while (trace.fraction != 1.f && trace.entityNum < MAX_CLIENTS && trace.entityNum != cent->currentState.number);

					// we can see the player head, no need to draw the icon
					if (trace.fraction == 1.f || trace.entityNum == cent->currentState.number)
					{
						continue;
					}

					VectorSubtract(vec3_origin, dir, dir);

					front = DotProduct(dir, cg.refdef.viewaxis[0]);
					left  = DotProduct(dir, cg.refdef.viewaxis[1]);
					up    = DotProduct(dir, cg.refdef.viewaxis[2]);

					dir[0] = front;
					dir[1] = left;
					dir[2] = 0;
					len    = VectorLength(dir);
					if (len < 0.1f)
					{
						len = 0.1f;
					}

					x = -left / front;
					y = up / len;

					trap_R_SetColor(col);
					CG_DrawPic(Ccg_WideX(SCREEN_WIDTH) / 2 + (Ccg_WideX(SCREEN_WIDTH) / 2) * x, SCREEN_HEIGHT / 2 + (SCREEN_HEIGHT / 2) * y, 8, 8, icon);
					trap_R_SetColor(NULL);

					continue;
				}
			}

			trap_R_SetColor(col);
			CG_DrawCompassIcon(Ccg_WideX(SCREEN_WIDTH) * 0.125f, SCREEN_HEIGHT * 0.125f,
			                   Ccg_WideX(SCREEN_WIDTH) * 0.75f, SCREEN_HEIGHT * 0.75f,
			                   cg.predictedPlayerState.origin, cent->lerpOrigin, icon, 0.5f, 8);
			trap_R_SetColor(NULL);
		}
	}
}

/**
 * @brief CG_Draw2D
 */
static void CG_Draw2D(void)
{
	CG_ScreenFade();

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_DrawIntermission();
		return;
	}
	else
	{
		if (cgs.dbShowing)
		{
			CG_Debriefing_Shutdown();
		}
	}

	if (cg.editingSpeakers)
	{
		CG_SpeakerEditorDraw();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].shoutcaster || cg.demoPlayback)
	{
		CG_DrawOnScreenLabels();
		CG_DrawOnScreenBars();
	}

	// no longer cheat protected, we draw crosshair/reticle in non demoplayback
	if (cg_draw2D.integer == 0)
	{
		if (cg.demoPlayback)
		{
			return;
		}
		CG_DrawCrosshair();
		CG_DrawFlashFade();
		return;
	}
#ifdef FEATURE_EDV
	if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#endif
	{
		CG_DrawFlashBlendBehindHUD();
	}

#ifdef FEATURE_EDV
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.demoCamera.renderingFreeCam || cgs.demoCamera.renderingWeaponCam)
#else
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
#endif
	{
		if (!CG_DrawScoreboard() && !cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			CG_DrawSpectator();
		}

		CG_DrawCrosshair();
		CG_DrawCrosshairNames();

		// we need to do this for spectators as well
		CG_DrawTeamInfo();
	}
	else
	{
		// don't draw any status if dead
		if (cg.snap->ps.stats[STAT_HEALTH] > 0 || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawNoShootIcon();
			CG_DrawEnvironmentalAwareness();
		}

		CG_DrawTeamInfo();

		if (cg_drawStatus.integer)
		{
			Menu_PaintAll();
			CG_DrawTimedMenus();
		}
	}

	// don't draw center string if scoreboard is up
	if (!CG_DrawScoreboard())
	{
		CG_DrawBannerPrint();

		CG_SetHud();

		if (cgs.clientinfo[cg.clientNum].shoutcaster && cg_shoutcastDrawPlayers.integer)
		{
			CG_DrawShoutcastPlayerList();
		}

#ifdef FEATURE_EDV
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR && !cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#else
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
#endif
		{
			if (cgs.clientinfo[cg.clientNum].shoutcaster && cg.snap->ps.pm_flags & PMF_FOLLOW)
			{
				CG_DrawShoutcastPlayerStatus();
			}
			else
			{
				CG_DrawActiveHud();
			}
		}

		if (!cg_paused.integer)
		{
			CG_DrawUpperRight();
		}

		CG_DrawCenterString();
#ifdef FEATURE_EDV
		if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
		{
			if (!cgs.clientinfo[cg.clientNum].shoutcaster)
			{
				CG_DrawFollow();
			}
		}
#else
		if (!cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			CG_DrawFollow();
		}
#endif

		CG_DrawWarmup();
		CG_DrawGlobalHud();

		if (!cgs.clientinfo[cg.clientNum].shoutcaster)
		{
			CG_DrawObjectiveInfo();
			CG_DrawSpectatorMessage();
		}

		CG_DrawLimboMessage();

		CG_DrawVote();
	}
	else
	{
		if (cgs.eventHandling != CGAME_EVENT_NONE)
		{
			// draw cursor
			trap_R_SetColor(NULL);
			CG_DrawPic(cgDC.cursorx - 14, cgDC.cursory - 14, 32, 32, cgs.media.cursorIcon);
		}
	}

	if (cg.showFireteamMenu)
	{
		CG_Fireteams_Draw();
	}

	if (cg.showSpawnpointsMenu)
	{
		CG_Spawnpoints_Draw();
	}

	// Info overlays
	CG_DrawOverlays();

#ifdef FEATURE_EDV
	if (!cgs.demoCamera.renderingFreeCam && !cgs.demoCamera.renderingWeaponCam)
#endif
	{
		// window updates
		CG_windowDraw();
	}

	CG_DrawDemoRecording();
}

/**
 * @brief CG_StartShakeCamera
 * @param[in] p
 */
void CG_StartShakeCamera(float p)
{
	cg.cameraShakeScale = p;

	cg.cameraShakeLength = 1000 * (p * p);
	cg.cameraShakeTime   = cg.time + cg.cameraShakeLength;
	cg.cameraShakePhase  = crandom() * M_PI; // start chain in random dir
}

/**
 * @brief CG_ShakeCamera
 */
void CG_ShakeCamera(void)
{
	static vec3_t mins = { -16.0f, -16.0f, -16.0f };
	static vec3_t maxs = { 16.0f, 16.0f, 16.0f };

	if (cg.time > cg.cameraShakeTime)
	{
		cg.cameraShakeScale = 0; // all pending explosions resolved, so reset shakescale
		return;
	}

	{
		double  x    = (cg.cameraShakeTime - cg.time) / cg.cameraShakeLength;
		float   valx = sin(M_PI * 8 * 13.0 + cg.cameraShakePhase) * x * 6 * cg.cameraShakeScale;
		float   valy = sin(M_PI * 17 * x + cg.cameraShakePhase) * x * 6 * cg.cameraShakeScale;
		float   valz = cos(M_PI * 7 * x + cg.cameraShakePhase) * x * 6 * cg.cameraShakeScale;
		vec3_t  vec;
		trace_t tr;

		VectorAdd(cg.refdef.vieworg, tv(valx, valy, valz), vec);
		CG_Trace(&tr, cg.refdef.vieworg, mins, maxs, vec, cg.predictedPlayerState.clientNum, MASK_SOLID);
		if (!(tr.allsolid || tr.startsolid))
		{
			VectorCopy(tr.endpos, cg.refdef.vieworg);
		}
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
	}
}

/**
 * @brief CG_DrawMiscGamemodels
 */
void CG_DrawMiscGamemodels(void)
{
	int         i, j;
	refEntity_t ent;

	Com_Memset(&ent, 0, sizeof(ent));

	ent.reType            = RT_MODEL;
	ent.nonNormalizedAxes = qtrue;

	// static gamemodels don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for (i = 0; i < cg.numMiscGameModels; i++)
	{
		if (cgs.miscGameModels[i].radius != 0.f)
		{
			if (CG_CullPointAndRadius(cgs.miscGameModels[i].org, cgs.miscGameModels[i].radius))
			{
				continue;
			}
		}

		if (!trap_R_inPVS(cg.refdef_current->vieworg, cgs.miscGameModels[i].org))
		{
			continue;
		}

		VectorCopy(cgs.miscGameModels[i].org, ent.origin);
		VectorCopy(cgs.miscGameModels[i].org, ent.oldorigin);
		VectorCopy(cgs.miscGameModels[i].org, ent.lightingOrigin);

		for (j = 0; j < 3; j++)
		{
			VectorCopy(cgs.miscGameModels[i].axes[j], ent.axis[j]);
		}
		ent.hModel = cgs.miscGameModels[i].model;

		trap_R_AddRefEntityToScene(&ent);
	}
}

/**
 * @brief CG_Coronas
 */
void CG_Coronas(void)
{
	if (cg_coronas.integer == 0)
	{
		return;
	}

	{
		int      i;
		trace_t  tr;
		float    dist;
		vec3_t   dir;
		qboolean visible, behind, toofar;

		for (i = 0 ; i < cg.numCoronas ; ++i)
		{
			if (!trap_R_inPVS(cg.refdef_current->vieworg, cgs.corona[i].org))
			{
				continue;
			}

			behind = qfalse; // 'init'
			toofar = qfalse; // 'init'

			VectorSubtract(cg.refdef_current->vieworg, cgs.corona[i].org, dir);
			dist = VectorNormalize2(dir, dir);

			if (dist > cg_coronafardist.integer)
			{
				toofar = qtrue;
			}
			// dot = DotProduct(dir, cg.refdef_current->viewaxis[0]);
			if (DotProduct(dir, cg.refdef_current->viewaxis[0]) >= -0.6f)
			{
				behind = qtrue;
			}

			if (cg_coronas.integer == 2)
			{   // if set to '2' trace everything
				behind = qfalse;
				toofar = qfalse;
			}

			if (!behind && !toofar)
			{
				CG_Trace(&tr, cg.refdef_current->vieworg, NULL, NULL, cgs.corona[i].org, -1, MASK_SOLID | CONTENTS_BODY);

				visible = qfalse; // 'init'
				if (tr.fraction == 1.f)
				{
					visible = qtrue;
				}
				trap_R_AddCoronaToScene(cgs.corona[i].org, cgs.corona[i].color[0], cgs.corona[i].color[1], cgs.corona[i].color[2], cgs.corona[i].scale, i, visible);
			}
		}
	}
}

/**
 * @brief Perform all drawing needed to completely fill the screen
 */
void CG_DrawActive()
{
	// optionally draw the info screen instead
	if (!cg.snap)
	{
		CG_DrawInformation(qfalse);
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	cg.refdef_current->glfog.registered = qfalse;    // make sure it doesn't use fog from another scene

	CG_ShakeCamera();
	CG_PB_RenderPolyBuffers();
	CG_DrawMiscGamemodels();
	CG_Coronas();

	if (!(cg.limboEndCinematicTime > cg.time && cg.showGameView))
	{
		trap_R_RenderScene(cg.refdef_current);
	}

	if (!cg.showGameView)
	{
		// draw status bar and other floating elements
		CG_Draw2D();
	}
	else
	{
		CG_LimboPanel_Draw();
	}
}
