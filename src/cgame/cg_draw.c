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
 *
 * @file cg_draw.c
 * @brief draw all of the graphical elements during active (after loading)
 * gameplay
 */

#include "cg_local.h"

#define STATUSBARHEIGHT 452
char *BindingFromName(const char *cvar);
void Controls_GetConfig(void);
void SetHeadOrigin(clientInfo_t *ci, playerInfo_t *pi);
void CG_DrawOverlays(void);
int activeFont;

///////////////////////
////// new hud stuff
///////////////////////

void CG_Text_SetActiveFont(int font)
{
	activeFont = font;
}

int CG_Text_Width_Ext(const char *text, float scale, int limit, fontInfo_t *font)
{
	float out = 0, useScale = scale * font->glyphScale;

	if (text)
	{
		const char  *s = text;
		glyphInfo_t *glyph;
		int         count = 0;
		int         len   = strlen(text);

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
				glyph = &font->glyphs[(unsigned char)*s];
				out  += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

int CG_Text_Width(const char *text, float scale, int limit)
{
	fontInfo_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Width_Ext(text, scale, limit, font);
}

int CG_Text_Height_Ext(const char *text, float scale, int limit, fontInfo_t *font)
{
	float max      = 0;
	float useScale = scale * font->glyphScale;

	if (text)
	{
		const char  *s = text;
		glyphInfo_t *glyph;
		int         count = 0;
		int         len   = strlen(text);

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
				glyph = &font->glyphs[(unsigned char)*s];
				if (max < glyph->height)
				{
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit)
{
	fontInfo_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Height_Ext(text, scale, limit, font);
}

void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader)
{
	w *= scalex;
	h *= scaley;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader)
{
	float w = width * scale;
	float h = height * scale;

	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font)
{
	x -= CG_Text_Width_Ext(text, scalex, limit, font) * 0.5f;

	CG_Text_Paint_Ext(x, y, scalex, scaley, color, text, adjust, limit, style, font);
}

void CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font)
{
	if (text)
	{
		vec4_t      newColor;
		glyphInfo_t *glyph;
		const char  *s = text;
		float       yadj;
		int         len, count = 0, ofs;

		scalex *= font->glyphScale;
		scaley *= font->glyphScale;

		trap_R_SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = strlen(text);
		if (limit > 0 && len > limit)
		{
			len = limit;
		}

		while (s && *s && count < len)
		{
			glyph = &font->glyphs[(unsigned char)*s];
			if (Q_IsColorString(s))
			{
				if (*(s + 1) == COLOR_NULL)
				{
					memcpy(newColor, color, sizeof(newColor));
				}
				else
				{
					memcpy(newColor, g_color_table[ColorIndex(*(s + 1))], sizeof(newColor));
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
				s++;
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style)
{
	fontInfo_t *font = &cgDC.Assets.fonts[activeFont];

	CG_Text_Paint_Ext(x, y, scale, scale, color, text, adjust, limit, style, font);
}

int CG_DrawFieldWidth(int x, int y, int width, int value, int charWidth, int charHeight)
{
	char num[16], *ptr;
	int  l;
	int  totalwidth = 0;

	if (width < 1)
	{
		return 0;
	}

	// draw number string
	if (width > 5)
	{
		width = 5;
	}

	switch (width)
	{
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
	{
		l = width;
	}

	ptr = num;
	while (*ptr && l)
	{
		totalwidth += charWidth;
		ptr++;
		l--;
	}

	return totalwidth;
}

/*
================
CG_DrawTeamBackground
================
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

#define CHATLOC_X 160
#define CHATLOC_Y 478
#define CHATLOC_TEXT_X (CHATLOC_X + 0.25f * TINYCHAR_WIDTH)

/*
=================
CG_DrawTeamInfo
=================
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
		int    x_offset   = 0;
		float  alphapercent;
		float  scale       = 0.2f;
		float  icon_width  = 12.f;
		float  icon_height = 10.f;
		int    chatWidth   = SCREEN_WIDTH - CHATLOC_X - 80 /*(cg_drawHUDHead.integer ? 80 : 0)*/;
		int    chatPosX    = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) ? 20 : CHATLOC_TEXT_X - x_offset;

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

			hcolor[3] = 0.33f * alphapercent;

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
	}
}

const char *CG_PickupItemText(int item)
{
	if (bg_itemlist[item].giType == IT_HEALTH)
	{
		if (bg_itemlist[item].world_model[2])          // this is a multi-stage item
		{   // FIXME: print the correct amount for multi-stage
			return va("a %s", bg_itemlist[item].pickup_name); // FIXME: @translations ?
		}
		else
		{
			return va("%i %s", bg_itemlist[item].quantity, bg_itemlist[item].pickup_name); // FIXME: @translations ?
		}
	}
	else if (bg_itemlist[item].giType == IT_TEAM)
	{
		return CG_TranslateString("an Objective");
	}
	else
	{
		// FIXME: this is just related to english
		if (bg_itemlist[item].pickup_name[0] == 'a' ||  bg_itemlist[item].pickup_name[0] == 'A')
		{
			return va("an %s", bg_itemlist[item].pickup_name); // FIXME: @translations
		}
		else
		{
			return va("a %s", bg_itemlist[item].pickup_name); // FIXME: @translations
		}
	}
}

/*
=================
CG_DrawNotify
=================
*/
#define NOTIFYLOC_Y 42 // bottom end
#define NOTIFYLOC_X 0
#define NOTIFYLOC_Y_SP 128

static void CG_DrawNotify(void)
{
	return; // FIXME: remove? early return ... commented the code

/*
    int    w, h;
    int    i, len;
    vec4_t hcolor;
    int    chatHeight;
    float  alphapercent;
    char   var[MAX_TOKEN_CHARS];
    float  notifytime = 1.0f;
    int    yLoc;

    yLoc = NOTIFYLOC_Y;

    trap_Cvar_VariableStringBuffer("con_notifytime", var, sizeof(var));
    notifytime = atof(var) * 1000;

    if (notifytime <= 100.f)
    {
        notifytime = 100.0f;
    }

    chatHeight = NOTIFY_HEIGHT;

    if (cgs.notifyLastPos != cgs.notifyPos)
    {
        if (cg.time - cgs.notifyMsgTimes[cgs.notifyLastPos % chatHeight] > notifytime)
        {
            cgs.notifyLastPos++;
        }

        h = (cgs.notifyPos - cgs.notifyLastPos) * TINYCHAR_HEIGHT;

        w = 0;

        for (i = cgs.notifyLastPos; i < cgs.notifyPos; i++)
        {
            len = CG_DrawStrlen(cgs.notifyMsgs[i % chatHeight]);
            if (len > w)
            {
                w = len;
            }
        }
        w *= TINYCHAR_WIDTH;
        w += TINYCHAR_WIDTH * 2;

        if (maxCharsBeforeOverlay <= 0)
        {
            maxCharsBeforeOverlay = 80;
        }

        for (i = cgs.notifyPos - 1; i >= cgs.notifyLastPos; i--)
        {
            alphapercent = 1.0f - ((cg.time - cgs.notifyMsgTimes[i % chatHeight]) / notifytime);
            if (alphapercent > 0.5f)
            {
                alphapercent = 1.0f;
            }
            else
            {
                alphapercent *= 2;
            }

            if (alphapercent < 0.f)
            {
                alphapercent = 0.f;
            }

            hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
            hcolor[3] = alphapercent;
            trap_R_SetColor(hcolor);

            CG_DrawStringExt(NOTIFYLOC_X + TINYCHAR_WIDTH,
                             yLoc - (cgs.notifyPos - i) * TINYCHAR_HEIGHT,
                             cgs.notifyMsgs[i % chatHeight], hcolor, qfalse, qfalse,
                             TINYCHAR_WIDTH, TINYCHAR_HEIGHT, maxCharsBeforeOverlay);
        }
    }
*/
}

/*
===============================================================================
LAGOMETER
===============================================================================
*/

#define LAG_SAMPLES     128

typedef struct
{
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void)
{
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = cg.time - cg.latestSnapshotTime;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
	// dropped packet
	if (!snap)
	{
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		lagometer.snapshotCount++;
		return;
	}

	if (cg.demoPlayback)
	{
		snap->ping = (snap->serverTime - snap->ps.commandTime) - 50;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)]   = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect(void)
{
	float      x = Ccg_WideX(SCREEN_WIDTH) - 48; // disconnect icon
	float      y = SCREEN_HEIGHT - 200; // disconnect icon
	int        cmdNum;
	usercmd_t  cmd;
	const char *s;
	int        w;   // bk010215 - FIXME char message[1024];

	// dont draw if a demo and we're running at a different timescale
	if (cg.demoPlayback && cg_timescale.value != 1.0f)
	{
		return;
	}

	// don't draw if the server is respawning
	if (cg.serverRespawning)
	{
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
	    || cmd.serverTime > cg.time)        // special check for map_restart // bk 0102165 - FIXME
	{
		return;
	}

	// also add text in center of screen
	s = CG_TranslateString("Connection Interrupted");
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 100, s, 1.0F);

	// blink the icon
	if ((cg.time >> 9) & 1)
	{
		return;
	}

	CG_DrawPic(x, y, 48, 48, cgs.media.disconnectIcon);
}

#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer(void)
{
	int   a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int   color;
	float vscale;

	if (!cg_lagometer.integer) // || cgs.localServer)
	{
		CG_DrawDisconnect();
		return;
	}

	// draw the graph
	x = Ccg_WideX(SCREEN_WIDTH) - 48;
	y = SCREEN_HEIGHT - 200;

	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid   = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0 ; a < aw ; a++)
	{
		i  = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v  = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0)
		{
			if (color != 1)
			{
				color = 1;
				trap_R_SetColor(colorYellow);
			}
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 2)
			{
				color = 2;
				trap_R_SetColor(colorBlue);
			}
			v = -v;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range  = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0 ; a < aw ; a++)
	{
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if (v > 0)
		{
			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if (color != 5)
				{
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor(colorYellow);
				}
			}
			else
			{
				if (color != 3)
				{
					color = 3;
					trap_R_SetColor(colorGreen);
				}
			}
			v = v * vscale;
			if (v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if (v < 0)
		{
			if (color != 4)
			{
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor(colorRed);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer
#ifdef ALLOW_GSYNC
	    || cg_synchronousClients.integer
#endif // ALLOW_GSYNC
	    )
	{
		CG_DrawBigString(ax, ay, "snc", 1.0);
	}

	CG_DrawDisconnect();
}

/*
===============================================================================
CENTER PRINTING
===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
#define CP_LINEWIDTH (int)(Ccg_WideX(56))

void CG_CenterPrint(const char *str, int y, int charWidth)
{
	char     *s;
	int      i, len;
	qboolean neednewline = qfalse;
	int      priority    = 0;

	// don't draw if this print message is less important
	if (cg.centerPrintTime && priority < cg.centerPrintPriority)
	{
		return;
	}

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));
	cg.centerPrintPriority = priority;

	// turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.centerPrint);
	for (i = 0; i < len; i++)
	{
		// NOTE: subtract a few chars here so long words still get displayed properly
		if (i % (CP_LINEWIDTH - 20) == 0 && i > 0)
		{
			neednewline = qtrue;
		}
		if (cg.centerPrint[i] == ' ' && neednewline)
		{
			cg.centerPrint[i] = '\n';
			neednewline       = qfalse;
		}
	}

	cg.centerPrintTime      = cg.time;
	cg.centerPrintY         = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s                   = cg.centerPrint;
	while (*s)
	{
		if (*s == '\n')
		{
			cg.centerPrintLines++;
		}
		s++;
	}
}

/*
==============
CG_PriorityCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_PriorityCenterPrint(const char *str, int y, int charWidth, int priority)
{
	char     *s;
	int      i, len;
	qboolean neednewline = qfalse;

	// don't draw if this print message is less important
	if (cg.centerPrintTime && priority < cg.centerPrintPriority)
	{
		return;
	}

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));
	cg.centerPrintPriority = priority;

	// turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.centerPrint);
	for (i = 0; i < len; i++)
	{

		// NOTE: subtract a few chars here so long words still get displayed properly
		if (i % (CP_LINEWIDTH - 20) == 0 && i > 0)
		{
			neednewline = qtrue;
		}
		if (cg.centerPrint[i] == ' ' && neednewline)
		{
			cg.centerPrint[i] = '\n';
			neednewline       = qfalse;
		}
	}

	cg.centerPrintTime      = cg.time + 2000;
	cg.centerPrintY         = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s                   = cg.centerPrint;
	while (*s)
	{
		if (*s == '\n')
		{
			cg.centerPrintLines++;
		}
		s++;
	}
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString(void)
{
	char  *start;
	int   l;
	int   x, y, w;
	float *color;

	if (!cg.centerPrintTime)
	{
		return;
	}

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if (!color)
	{
		cg.centerPrintTime     = 0;
		cg.centerPrintPriority = 0;
		return;
	}

	trap_R_SetColor(color);

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while (1)
	{
		char linebuffer[1024];

		for (l = 0; l < CP_LINEWIDTH; l++) // added CP_LINEWIDTH
		{
			if (!start[l] || start[l] == '\n')
			{
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerPrintCharWidth * CG_DrawStrlen(linebuffer);

		x = (Ccg_WideX(SCREEN_WIDTH) - w) / 2;

		CG_DrawStringExt(x, y, linebuffer, color, qfalse, qtrue, cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0);

		y += cg.centerPrintCharWidth * 1.5;

		while (*start && (*start != '\n'))
		{
			start++;
		}
		if (!*start)
		{
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}

/*
================================================================================
CROSSHAIRS
================================================================================
*/

/*
==============
CG_DrawWeapReticle
==============
*/
static void CG_DrawWeapReticle(void)
{
	qboolean fg, garand, k43;

	// So that we will draw reticle
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		garand = (qboolean)(cg.snap->ps.weapon == WP_GARAND_SCOPE);
		k43    = (qboolean)(cg.snap->ps.weapon == WP_K43_SCOPE);
		fg     = (qboolean)(cg.snap->ps.weapon == WP_FG42SCOPE);
	}
	else
	{
		fg     = (qboolean)(cg.weaponSelect == WP_FG42SCOPE);
		garand = (qboolean)(cg.weaponSelect == WP_GARAND_SCOPE);
		k43    = (qboolean)(cg.weaponSelect == WP_K43_SCOPE);
	}

	if (fg)
	{
		// sides
		CG_FillRect(0, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);
		CG_FillRect(560 + cgs.wideXoffset, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);

		// center
		if (cgs.media.reticleShaderSimple)
		{
			CG_DrawPic(80 + cgs.wideXoffset, 0, SCREEN_HEIGHT, SCREEN_HEIGHT, cgs.media.reticleShaderSimple);
		}

		// hairs
		CG_FillRect(84 + cgs.wideXoffset, 239, 150, 3, colorBlack);     // left
		CG_FillRect(234 + cgs.wideXoffset, 240, 173, 1, colorBlack);    // horiz center
		CG_FillRect(407 + cgs.wideXoffset, 239, 150, 3, colorBlack);    // right


		CG_FillRect(319 + cgs.wideXoffset, 2, 3, 151, colorBlack);      // top center top
		CG_FillRect(320 + cgs.wideXoffset, 153, 1, 114, colorBlack);    // top center bot

		CG_FillRect(320 + cgs.wideXoffset, 241, 1, 87, colorBlack);     // bot center top
		CG_FillRect(319 + cgs.wideXoffset, 327, 3, 151, colorBlack);    // bot center bot
	}
	else if (garand)
	{
		// sides
		CG_FillRect(0, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);
		CG_FillRect(560 + cgs.wideXoffset, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);

		// center
		if (cgs.media.reticleShaderSimple)
		{
			CG_DrawPic(80 + cgs.wideXoffset, 0, SCREEN_HEIGHT, SCREEN_HEIGHT, cgs.media.reticleShaderSimple);
		}

		// hairs
		CG_FillRect(84 + cgs.wideXoffset, 239, 177, 2, colorBlack);     // left
		CG_FillRect(320 + cgs.wideXoffset, 242, 1, 58, colorBlack);     // center top
		CG_FillRect(319 + cgs.wideXoffset, 300, 2, 178, colorBlack);    // center bot
		CG_FillRect(380 + cgs.wideXoffset, 239, 177, 2, colorBlack);    // right
	}
	else if (k43)
	{
		// sides
		CG_FillRect(0, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);
		CG_FillRect(560 + cgs.wideXoffset, 0, 80 + cgs.wideXoffset, SCREEN_HEIGHT, colorBlack);

		// center
		if (cgs.media.reticleShaderSimple)
		{
			CG_DrawPic(80 + cgs.wideXoffset, 0, SCREEN_HEIGHT, SCREEN_HEIGHT, cgs.media.reticleShaderSimple);
		}

		// hairs
		CG_FillRect(84 + cgs.wideXoffset, 239, 177, 2, colorBlack);     // left
		CG_FillRect(320 + cgs.wideXoffset, 242, 1, 58, colorBlack);     // center top
		CG_FillRect(319 + cgs.wideXoffset, 300, 2, 178, colorBlack);    // center bot
		CG_FillRect(380 + cgs.wideXoffset, 239, 177, 2, colorBlack);    // right
	}
}

/*
==============
CG_DrawMortarReticle
==============
*/
static void CG_DrawMortarReticle(void)
{
	vec4_t   color             = { 1.f, 1.f, 1.f, .5f };
	vec4_t   color_back        = { 0.f, 0.f, 0.f, .25f };
	vec4_t   color_extends     = { .77f, .73f, .1f, 1.f };
	vec4_t   color_lastfire    = { .77f, .1f, .1f, 1.f };
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
	majorOffset = (int)(floor((int)floor(AngleNormalize360(angle - .5f * 180)) % 15) / 5.f);

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
	if (cg.lastFiredWeapon == WP_MORTAR_SET && cg.mortarImpactTime >= -1)
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
			if (dir[1] == 0 && dir[0] == 0)
			{
				yaw = 0;
			}
			else
			{
				if (dir[0])
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
					CG_DrawPic(136 + 2 + cgs.wideXoffset, 236 + 38 - 10 + 1, 8, 8, cgs.media.ccMortarTargetArrow);
					trap_R_SetColor(NULL);

					hasLeftTarget = qtrue;
				}
			}
			else if (yaw > 90)
			{
				if (!hasRightTarget)
				{
					trap_R_SetColor(color_firerequest);
					CG_DrawPic(350 + 154 - 10 + cgs.wideXoffset, 236 + 38 - 10 + 1, -8, 8, cgs.media.ccMortarTargetArrow);
					trap_R_SetColor(NULL);

					hasRightTarget = qtrue;
				}
			}
			else
			{
				localOffset = ((AngleSubtract(angle, attackRequestAngle)) / 5.f) * 10.f;

				trap_R_SetColor(color_firerequest);
				CG_DrawPic(320 - localOffset - 8 + cgs.wideXoffset, 264 - 8, 16, 16, cgs.media.ccMortarTarget);
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
	majorOffset = (int)(floor((int)((angle + .5f * 50) * 10.f) % 100) / 25.f);

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
	if (cg.lastFiredWeapon == WP_MORTAR_SET && cg.mortarImpactTime >= -1)
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

/*
==============
CG_DrawBinocReticle
==============
*/
static void CG_DrawBinocReticle(void)
{
	// an alternative.  This gives nice sharp lines at the expense of a few extra polys

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

void CG_FinishWeaponChange(int lastweap, int newweap);

/*
=================
CG_DrawCrosshair
=================
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

	switch (weapnum)
	{
	// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
		if (cg.zoomedBinoc)
		{
			CG_DrawBinocReticle();
		}

		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
		{
			return;
		}
		break;

	// special reticle for weapon
	case WP_FG42SCOPE:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		if (!BG_PlayerMounted(cg.snap->ps.eFlags))
		{
			// don't let players run with rifles -- speed 80 == crouch, 128 == walk, 256 == run
			if (VectorLengthSquared(cg.snap->ps.velocity) > Square(127))
			{
				if (cg.snap->ps.weapon == WP_FG42SCOPE)
				{
					CG_FinishWeaponChange(WP_FG42SCOPE, WP_FG42);
				}
				if (cg.snap->ps.weapon == WP_GARAND_SCOPE)
				{
					CG_FinishWeaponChange(WP_GARAND_SCOPE, WP_GARAND);
				}
				if (cg.snap->ps.weapon == WP_K43_SCOPE)
				{
					CG_FinishWeaponChange(WP_K43_SCOPE, WP_K43);
				}
			}

			if (
#if FEATURE_MULTIVIEW
			    cg.mvTotalClients < 1 ||
#endif
			    cg.snap->ps.stats[STAT_HEALTH] > 0)
			{
				CG_DrawWeapReticle();
			}

			return;
		}
		break;
	default:
		break;
	}

	if (cg.predictedPlayerState.eFlags & EF_PRONE_MOVING)
	{
		return;
	}

	// FIXME: spectators/chasing?
	if (cg.predictedPlayerState.weapon == WP_MORTAR_SET && cg.predictedPlayerState.weaponstate != WEAPON_RAISING)
	{
		CG_DrawMortarReticle();
		return;
	}

	if (cg_drawCrosshair.integer < 0)     // moved down so it doesn't keep the scoped weaps from drawing reticles
	{
		return;
	}

	// no crosshair while leaning
	if (cg.snap->ps.leanf)
	{
		return;
	}

	// Don't draw crosshair if have exit hintcursor
	if (cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT)
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
	w *= (1 + f * 2.0);
	h *= (1 + f * 2.0);

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &w, &h);

	hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w), y + 0.5 * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, hShader);

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

		trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w), y + 0.5 * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
	}
}

static void CG_DrawNoShootIcon(void)
{
	float x, y, w, h;

	if (cg.predictedPlayerState.eFlags & EF_PRONE && cg.snap->ps.weapon == WP_PANZERFAUST)
	{
		trap_R_SetColor(colorRed);
	}
	else if (cg.crosshairClientNoShoot
	         // don't shoot friend or civilian
	         || cg.snap->ps.serverCursorHint == HINT_PLYR_NEUTRAL
	         || cg.snap->ps.serverCursorHint == HINT_PLYR_FRIEND)
	{
		float *color = CG_FadeColor(cg.crosshairClientTime, 1000);

		if (!color)
		{
			trap_R_SetColor(NULL);
			return;
		}
		else
		{
			trap_R_SetColor(color);
		}
	}
	else
	{
		return;
	}

	w = h = 48.f;

	x = cg_crosshairX.integer + 1;
	y = cg_crosshairY.integer + 1;
	CG_AdjustFrom640(&x, &y, &w, &h);

	trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w), y + 0.5 * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, cgs.media.friendShader);
}

/*
=================
CG_ScanForCrosshairEntity

  Returns the distance to the entity
=================
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
	VectorMA(start, 8192, cg.refdef.viewaxis[0], end);

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

	if (cent && cent->currentState.powerups & (1 << PW_OPS_DISGUISED))
	{
		if (cgs.clientinfo[cg.crosshairClientNum].team == cgs.clientinfo[cg.clientNum].team)
		{
			cg.crosshairClientNoShoot = qtrue;
		}
	}

	return dist;
}

#define CH_KNIFE_DIST       48  // from g_weapon.c
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96

#define CH_DIST             100 //128       // use the largest value from above

/*
==============
CG_CheckForCursorHints
    concept in progress...
==============
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

	if (trace.fraction == 1)
	{
		return;
	}

	dist = trace.fraction * CH_DIST;

	tracent = &cg_entities[trace.entityNum];

	// invisible entities don't show hints
	if (trace.entityNum >= MAX_CLIENTS &&
	    (tracent->currentState.powerups == STATE_INVISIBLE ||
	     tracent->currentState.powerups == STATE_UNDERCONSTRUCTION))
	{
		return;
	}

	// world
	if (trace.entityNum == ENTITYNUM_WORLD)
	{
		if ((CG_PointContents(trace.endpos, -1) & CONTENTS_WATER) && !(CG_PointContents(cg.refdef.vieworg, -1) & CONTENTS_WATER))       // jaquboss - was only on servercode
		{
			if (dist <= CH_DIST)
			{
				cg.cursorHintIcon  = HINT_WATER;
				cg.cursorHintTime  = cg.time;
				cg.cursorHintFade  = 500;
				cg.cursorHintValue = 0;
			}
		}
		else if ((trace.surfaceFlags & SURF_LADDER) && !(cg.snap->ps.pm_flags & PMF_LADDER))
		{
			if (dist <= CH_DIST)
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
		if (cg.snap->ps.weapon == WP_KNIFE)
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

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames(void)
{
	float      *color;
	char       *name;
	float      w;
	const char *s, *playerClass;
	int        playerHealth = 0;
	vec4_t     c;
	qboolean   drawStuff = qfalse;
	const char *playerRank;
	qboolean   isTank    = qfalse;
	int        maxHealth = 1;
	float      dist; // Distance to the entity under the crosshair
	float      zChange;
	qboolean   hitClient = qfalse;

	if (cg_drawCrosshair.integer < 0)
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	dist = CG_ScanForCrosshairEntity(&zChange, &hitClient);

	if (cg.renderingThirdPerson)
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

	if (cg.crosshairClientNum > MAX_CLIENTS)
	{
		if (!cg_drawCrosshairNames.integer)
		{
			return;
		}

		if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
		{
			if (cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.effect1Time)
			{
				isTank = qtrue;

				playerHealth = cg_entities[cg.crosshairClientNum].currentState.dl_intensity;
				maxHealth    = 255;

				s = Info_ValueForKey(CG_ConfigString(CS_SCRIPT_MOVER_NAMES), va("%i", cg.crosshairClientNum));
				if (!*s)
				{
					return;
				}

				w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;
				CG_DrawSmallStringColor((320 - w / 2) + cgs.wideXoffset, 170, s, color);
			}
			else if (cg_entities[cg.crosshairClientNum].currentState.eType == ET_CONSTRUCTIBLE_MARKER)
			{
				s = Info_ValueForKey(CG_ConfigString(CS_CONSTRUCTION_NAMES), va("%i", cg.crosshairClientNum));
				if (*s)
				{
					w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;
					CG_DrawSmallStringColor((320 - w / 2) + cgs.wideXoffset, 170, s, color);
				}
				return;
			}
		}

		if (!isTank)
		{
			return;
		}
	}
	else if (cgs.clientinfo[cg.crosshairClientNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team)
	{
		if ((cg_entities[cg.crosshairClientNum].currentState.powerups & (1 << PW_OPS_DISGUISED)) && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
		{
			if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR &&
			    cgs.clientinfo[cg.snap->ps.clientNum].skill[SK_SIGNALS] >= 4 && cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_FIELDOPS)
			{
				s = CG_TranslateString("Disguised Enemy!");
				w = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;
				CG_DrawSmallStringColor((320 - w / 2) + cgs.wideXoffset, 170, s, color);
				return;
			}
			else if (dist > 512)
			{
				if (!cg_drawCrosshairNames.integer)
				{
					return;
				}

				drawStuff = qtrue;

				// determine player class
				playerClass = BG_ClassLetterForNumber((cg_entities[cg.crosshairClientNum].currentState.powerups >> PW_OPS_CLASS_1) & 6);

				name = cgs.clientinfo[cg.crosshairClientNum].disguiseName;

				playerRank = cgs.clientinfo[cg.crosshairClientNum].team != TEAM_AXIS ? rankNames_Axis[cgs.clientinfo[cg.crosshairClientNum].disguiseRank] : rankNames_Allies[cgs.clientinfo[cg.crosshairClientNum].disguiseRank];
				s          = va("[%s] %s %s", CG_TranslateString(playerClass), playerRank, name);
				w          = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;

				// draw the name and class
				CG_DrawSmallStringColor((320 - w / 2) + cgs.wideXoffset, 170, s, color);

				// set the health
				// - make sure it's the health for the right entity;
				// if it's not, use the clientinfo health (which is updated by tinfo)
				if (cg.crosshairClientNum == cg.snap->ps.identifyClient)
				{
					playerHealth = cg.snap->ps.identifyClientHealth;
				}
				else
				{
					playerHealth = cgs.clientinfo[cg.crosshairClientNum].health;
				}

				maxHealth = 100;
			}
			else
			{
				// don't show the name after you look away, should this be a disguised covert
				cg.crosshairClientTime = 0;
				return;
			}
		}
		else
		{
			return;
		}
	}

	if (!cg_drawCrosshairNames.integer)
	{
		return;
	}

	// changed this from early-exiting if true, to only executing most stuff if false. We want to
	// show debug info regardless

	// we only want to see players on our team
	if (!isTank && !(cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR && cgs.clientinfo[cg.crosshairClientNum].team != cgs.clientinfo[cg.snap->ps.clientNum].team))
	{
		int i;

		drawStuff = qtrue;

		// determine player class
		playerClass = BG_ClassLetterForNumber(cg_entities[cg.crosshairClientNum].currentState.teamNum);

		name = cgs.clientinfo[cg.crosshairClientNum].name;

		playerRank = cgs.clientinfo[cg.crosshairClientNum].team == TEAM_AXIS ? rankNames_Axis[cgs.clientinfo[cg.crosshairClientNum].rank] : rankNames_Allies[cgs.clientinfo[cg.crosshairClientNum].rank];
		s          = va("[%s] %s %s", CG_TranslateString(playerClass), playerRank, name);
		w          = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;

		// draw the name and class
		CG_DrawSmallStringColor((320 - w / 2) + cgs.wideXoffset, 170, s, color);

		// set the health
		if (cg.crosshairClientNum == cg.snap->ps.identifyClient)
		{
			playerHealth = cg.snap->ps.identifyClientHealth;
		}
		else
		{
			playerHealth = cgs.clientinfo[cg.crosshairClientNum].health;
		}

		maxHealth = 100;
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!cgs.clientinfo[i].infoValid)
			{
				continue;
			}

			if (cgs.clientinfo[i].team != cgs.clientinfo[cg.snap->ps.clientNum].team)
			{
				continue;
			}

			if (cgs.clientinfo[i].cls != PC_MEDIC)
			{
				continue;
			}

			maxHealth += 10;

			if (maxHealth >= 125)
			{
				maxHealth = 125;
				break;
			}
		}

		if (cgs.clientinfo[cg.crosshairClientNum].skill[SK_BATTLE_SENSE] >= 3)
		{
			maxHealth += 15;
		}

		if (cgs.clientinfo[cg.crosshairClientNum].cls == PC_MEDIC)
		{
			maxHealth *= 1.12f;
		}
	}

	// draw the health bar
	//  if ( isTank || (cg.crosshairClientNum == cg.snap->ps.identifyClient && drawStuff && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR ) )
	{
		vec4_t bgcolor;
		float  barFrac = (float)playerHealth / maxHealth;

		if (barFrac > 1.0)
		{
			barFrac = 1.0;
		}
		else if (barFrac < 0)
		{
			barFrac = 0;
		}

		c[0] = 1.0f;
		c[1] = c[2] = barFrac;
		c[3] = (0.25 + barFrac * 0.5) * color[3];

		Vector4Set(bgcolor, 1.f, 1.f, 1.f, .25f * color[3]);

		CG_FilledBar((320 - 110 / 2) + cgs.wideXoffset, 190, 110, 10, c, NULL, bgcolor, barFrac, 16);
	}

	if (isTank)
	{
		return;
	}

	if (drawStuff)
	{
		trap_R_SetColor(NULL);
	}
}

//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void)
{
	CG_DrawBigString((320 - 9 * 8) + cgs.wideXoffset, 440, CG_TranslateString("SPECTATOR"), 1.f);
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void)
{
	char *s;
	char str1[32], str2[32];

	if (cgs.complaintEndTime > cg.time && !cg.demoPlayback && cg_complaintPopUp.integer > 0 && cgs.complaintClient >= 0)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("File complaint against %s for team-killing?"), cgs.clientinfo[cgs.complaintClient].name);
		CG_DrawStringExt(8, 200, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient >= 0)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("Accept %s's application to join your fireteam?"), cgs.clientinfo[cgs.applicationClient].name);
		CG_DrawStringExt(8, 200, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient >= 0)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("Accept %s's proposition to invite %s to join your fireteam?"), cgs.clientinfo[cgs.propositionClient2].name, cgs.clientinfo[cgs.propositionClient].name);
		CG_DrawStringExt(8, 200, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient >= 0)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("Accept %s's invitation to join their fireteam?"), cgs.clientinfo[cgs.invitationClient].name);
		CG_DrawStringExt(8, 200, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.autoFireteamEndTime > cg.time && cgs.autoFireteamNum == -1)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = "Make Fireteam private?";
		CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.autoFireteamCreateEndTime > cg.time && cgs.autoFireteamCreateNum == -1)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = "Create a Fireteam?";
		CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.autoFireteamJoinEndTime > cg.time && cgs.autoFireteamJoinNum == -1)
	{
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = "Join a Fireteam?";
		CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.voteTime)
	{
		int sec;

		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		// play a talk beep whenever it is modified
		if (cgs.voteModified)
		{
			cgs.voteModified = qfalse;
		}

		sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
		if (sec < 0)
		{
			sec = 0;
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
			s = va(CG_TranslateString("VOTE(%i): %s"), sec, cgs.voteString);
			CG_DrawStringExt(8, 200, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

			if (cgs.clientinfo[cg.clientNum].team != TEAM_AXIS && cgs.clientinfo[cg.clientNum].team != TEAM_ALLIES)
			{
				s = CG_TranslateString("Cannot vote as Spectator");
			}
			else
			{
				s = va(CG_TranslateString("YES(%s):%i, NO(%s):%i"), str1, cgs.voteYes, str2, cgs.voteNo);
			}
			CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 60);
			return;
		}
		else
		{
			s = va(CG_TranslateString("YOU VOTED ON: %s"), cgs.voteString);
			CG_DrawStringExt(8, 200, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

			s = va(CG_TranslateString("Y:%i, N:%i"), cgs.voteYes, cgs.voteNo);
			CG_DrawStringExt(8, 214, s, colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 20);
			return;
		}
	}

	if (cgs.complaintEndTime > cg.time && !cg.demoPlayback && cg_complaintPopUp.integer > 0 && cgs.complaintClient < 0)
	{
		if (cgs.complaintClient == -1)
		{
			s = "Your complaint has been filed";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
		if (cgs.complaintClient == -2)
		{
			s = "Complaint dismissed";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
		if (cgs.complaintClient == -3)
		{
			s = "Server Host cannot be complained against";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
		if (cgs.complaintClient == -4)
		{
			s = "You were team-killed by the Server Host";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient < 0)
	{
		if (cgs.applicationClient == -1)
		{
			s = "Your application has been submitted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.applicationClient == -2)
		{
			s = "Your application failed";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.applicationClient == -3)
		{
			s = "Your application has been approved";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.applicationClient == -4)
		{
			s = "Your application reply has been sent";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient < 0)
	{
		if (cgs.propositionClient == -1)
		{
			s = "Your proposition has been submitted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.propositionClient == -2)
		{
			s = "Your proposition was rejected";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.propositionClient == -3)
		{
			s = "Your proposition was accepted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.propositionClient == -4)
		{
			s = "Your proposition reply has been sent";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient < 0)
	{
		if (cgs.invitationClient == -1)
		{
			s = "Your invitation has been submitted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient == -2)
		{
			s = "Your invitation was rejected";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient == -3)
		{
			s = "Your invitation was accepted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient == -4)
		{
			s = "Your invitation reply has been sent";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient < 0)
		{
			return;
		}
	}

	if ((cgs.autoFireteamEndTime > cg.time && cgs.autoFireteamNum == -2) || (cgs.autoFireteamCreateEndTime > cg.time && cgs.autoFireteamCreateNum == -2) || (cgs.autoFireteamJoinEndTime > cg.time && cgs.autoFireteamJoinNum == -2))
	{
		s = "Response Sent";
		CG_DrawStringExt(8, 200, CG_TranslateString(s), colorYellow, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}
}

/*
=================
CG_DrawIntermission
=================
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

/*
=================
CG_DrawSpectatorMessage
=================
*/
static void CG_DrawSpectatorMessage(void)
{
	const char *str, *str2;
	static int lastconfigGet = 0;

	if (!cg_descriptiveText.integer)
	{
		return;
	}

	if (!(cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR))
	{
		return;
	}

	if (cg.time - lastconfigGet > 1000)
	{
		Controls_GetConfig();

		lastconfigGet = cg.time;
	}

	str2 = BindingFromName("openlimbomenu");
	if (!Q_stricmp(str2, "(openlimbomenu)"))
	{
		str2 = "ESCAPE";
	}
	str = va(CG_TranslateString("Press %s to open Limbo Menu"), str2);
	CG_DrawStringExt(8, 154, str, colorWhite, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	str2 = BindingFromName("+attack");
	str  = va(CG_TranslateString("Press %s to follow next player"), str2);
	CG_DrawStringExt(8, 172, str, colorWhite, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

#ifdef FEATURE_MULTIVIEW
	str2 = BindingFromName("mvactivate");
	str  = va(CG_TranslateString("Press %s to %s multiview mode"), str2, ((cg.mvTotalClients > 0) ? "disable" : "activate"));
	CG_DrawStringExt(8, 190, str, colorWhite, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
#endif
}

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

int CG_CalculateReinfTime(qboolean menu)
{
	return((int)CG_CalculateReinfTime_Float(menu));
}

/*
=================
CG_DrawLimboMessage
=================
*/

#define INFOTEXT_STARTX 8

static void CG_DrawLimboMessage(void)
{
	const char    *str;
	playerState_t *ps = &cg.snap->ps;
	int           y   = 118;

	if (ps->stats[STAT_HEALTH] > 0)
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_LIMBO || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg_descriptiveText.integer)
	{
		str = CG_TranslateString("You are wounded and waiting for a medic.");
		CG_DrawSmallStringColor(INFOTEXT_STARTX, y, str, colorWhite);
		y += 18;

		if (cgs.gametype == GT_WOLF_LMS)
		{
			trap_R_SetColor(NULL);
			return;
		}

		str = va(CG_TranslateString("Press %s to go into reinforcement queue."), BindingFromName("+moveup"));

		CG_DrawSmallStringColor(INFOTEXT_STARTX, 134, str, colorWhite);
		y += 18;
	}
	else if (cgs.gametype == GT_WOLF_LMS)
	{
		trap_R_SetColor(NULL);
		return;
	}

	str = (ps->persistant[PERS_RESPAWNS_LEFT] == 0) ? CG_TranslateString("No more reinforcements this round.") : va(CG_TranslateString("Reinforcements deploy in %d seconds."), CG_CalculateReinfTime(qfalse));

	CG_DrawSmallStringColor(INFOTEXT_STARTX, y, str, colorWhite);
	y += 18;

	trap_R_SetColor(NULL);
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void)
{
	char deploytime[128];

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

	// if in limbo, show different follow message
	if (cg.snap->ps.pm_flags & PMF_LIMBO)
	{
		if (cgs.gametype != GT_WOLF_LMS)
		{
			if (cg.snap->ps.persistant[PERS_RESPAWNS_LEFT] == 0)
			{
				if (cg.snap->ps.persistant[PERS_RESPAWNS_PENALTY] >= 0)
				{
					int deployTime = (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_AXIS) ? cg_redlimbotime.integer : cg_bluelimbotime.integer;

					deployTime *= 0.001f;

					sprintf(deploytime, CG_TranslateString("Bonus Life! Deploying in %d seconds"), CG_CalculateReinfTime(qfalse) + cg.snap->ps.persistant[PERS_RESPAWNS_PENALTY] * deployTime);
				}
				else
				{
					sprintf(deploytime, "%s", CG_TranslateString("No more deployments this round"));
				}
			}
			else
			{
				sprintf(deploytime, CG_TranslateString("Deploying in %d seconds"), CG_CalculateReinfTime(qfalse));
			}

			CG_DrawStringExt(INFOTEXT_STARTX, 118, deploytime, colorWhite, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 80);
		}

		// Don't display if you're following yourself
		if (cg.snap->ps.clientNum != cg.clientNum)
		{
			sprintf(deploytime, "(%s %s %s [%s])", CG_TranslateString("Following"),
			        cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES ? rankNames_Allies[cgs.clientinfo[cg.snap->ps.clientNum].rank] : rankNames_Axis[cgs.clientinfo[cg.snap->ps.clientNum].rank],
			        cgs.clientinfo[cg.snap->ps.clientNum].name,
			        BG_ClassnameForNumber(cgs.clientinfo[cg.snap->ps.clientNum].cls));

			CG_DrawStringExt(INFOTEXT_STARTX, 136, deploytime, colorWhite, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 80);
		}
	}
	else
	{
		CG_DrawStringExt(INFOTEXT_STARTX, 118, CG_TranslateString("Following"), colorWhite, qtrue, qtrue, BIGCHAR_WIDTH / 2, BIGCHAR_HEIGHT, 0);

		CG_DrawStringExt(84, 118, va("%s %s [%s]",
		                             cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_ALLIES ? rankNames_Allies[cgs.clientinfo[cg.snap->ps.clientNum].rank] : rankNames_Axis[cgs.clientinfo[cg.snap->ps.clientNum].rank],
		                             cgs.clientinfo[cg.snap->ps.clientNum].name, BG_ClassnameForNumber(cgs.clientinfo[cg.snap->ps.clientNum].cls)),
		                 colorWhite, qtrue, qtrue, BIGCHAR_WIDTH / 2, BIGCHAR_HEIGHT, 0);
	}

	return qtrue;
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup(void)
{
	int             w;
	int             sec = cg.warmup;
	int             cw;
	const char      *s, *s1, *s2;
	static qboolean announced = qfalse;
	int             x;

	if (!sec)
	{
		if ((cgs.gamestate == GS_WARMUP && !cg.warmup) || cgs.gamestate == GS_WAITING_FOR_PLAYERS)
		{
			cw = 9; // 10 - 1;

			if (CG_ConfigString(CS_CONFIGNAME)[0])
			{
				s1 = va("^3Config:^7%s^7", CG_ConfigString(CS_CONFIGNAME));
				w  = CG_DrawStrlen(s1);
				x  = Ccg_WideX(320) - w * 12 / 2;
				CG_DrawStringExt(x, 162, s1, colorWhite, qfalse, qtrue, 12, 16, 0);
			}

			s1 = va(CG_TranslateString("^3WARMUP:^7 Waiting on ^2%i^7 %s"), cgs.minclients, cgs.minclients == 1 ? "player" : "players");
			w  = CG_DrawStrlen(s1);
			x  = Ccg_WideX(320) - w * 12 / 2;
			CG_DrawStringExt(x, 188, s1, colorWhite, qfalse, qtrue, 12, 18, 0);

			if (!cg.demoPlayback && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			    (!(cg.snap->ps.pm_flags & PMF_FOLLOW) || (cg.snap->ps.pm_flags & PMF_LIMBO)))
			{
				char str1[32];

				Q_strncpyz(str1, BindingFromName("ready"), 32);
				if (!Q_stricmp(str1, "(?" "?" "?)"))
				{
					s2 = CG_TranslateString("Type ^3\\ready^* in the console to start");
				}
				else
				{
					s2 = va("Press ^3%s^* to start", str1);
					s2 = CG_TranslateString(s2);
				}
				w = CG_DrawStrlen(s2);
				x = Ccg_WideX(320) - w * cw / 2;
				CG_DrawStringExt(x, 208, s2, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5), 0);
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

	s = va("%s %i", CG_TranslateString("(WARMUP) Match begins in:"), sec + 1);

	w = CG_DrawStrlen(s);
	x = Ccg_WideX(320) - w * 6;
	CG_DrawStringExt(x, 120, s, colorYellow, qfalse, qtrue, 12, 18, 0);

	// pre start actions
	if (sec == 3 && !announced)
	{
		trap_S_StartLocalSound(cgs.media.countPrepare, CHAN_ANNOUNCER);

		CPri(CG_TranslateString("^3PREPARE TO FIGHT!\n"));

		if (!cg.demoPlayback && cg_autoAction.integer & AA_DEMORECORD)
		{
			CG_autoRecord_f();
		}

		announced = qtrue;
	}
	else if (sec != 3)
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
		defender = atoi(Info_ValueForKey(cs, "defender"));

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

		cw = 10 - 1;

		w = CG_DrawStrlen(s);
		x = Ccg_WideX(320) - w * cw / 2;
		CG_DrawStringExt(x, 140, s, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5), 0);

		w = CG_DrawStrlen(s1);
		x = Ccg_WideX(320) - w * cw / 2;
		CG_DrawStringExt(x, 160, s1, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5), 0);

		w = CG_DrawStrlen(s2);
		x = Ccg_WideX(320) - w * cw / 2;
		CG_DrawStringExt(x, 180, s2, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5), 0);
	}
}

//==================================================================================

/*
=================
CG_DrawFlashFade
=================
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
#if FEATURE_MULTIVIEW
		    cg.mvTotalClients < 1 &&
#endif
		    cg.snap->ps.powerups[PW_BLACKOUT] > 0)
		{
			trap_Cvar_Set("ui_blackout", va("%d", cg.snap->ps.powerups[PW_BLACKOUT]));
		}
	}
	else if (cg.snap->ps.powerups[PW_BLACKOUT] == 0
#if FEATURE_MULTIVIEW
	         || cg.mvTotalClients > 0
#endif
	         )
	{
		trap_Cvar_Set("ui_blackout", "0");
	}

	// now draw the fade
	if (cgs.fadeAlphaCurrent > 0.0 || fBlackout)
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
					CG_DrawStringExt(INFOTEXT_STARTX, nOffset, va(CG_TranslateString("The %s team is speclocked!"), teams[i]), color, qtrue, qfalse, 10, 10, 0);
					nOffset += 12;
				}
			}
		}
	}
}

/*
==============
CG_DrawFlashZoomTransition
    hide the snap transition from regular view to/from zoomed

  FIXME: TODO: use cg_fade?
==============
*/
static void CG_DrawFlashZoomTransition(void)
{
	float frac;
	float fadeTime;

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

	fadeTime = 400.f;

	frac = cg.time - cg.zoomTime;

	if (frac < fadeTime)
	{
		vec4_t color;

		frac = frac / fadeTime;
		Vector4Set(color, 0, 0, 0, 1.0f - frac);
		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, color);
	}
}

/*
=================
CG_DrawFlashDamage
=================
*/
static void CG_DrawFlashDamage(void)
{
	if (!cg.snap)
	{
		return;
	}

	if (cg.v_dmg_time > cg.time)
	{
		vec4_t col      = { 0.2, 0, 0, 0 };
		float  redFlash = fabs(cg.v_dmg_pitch * ((cg.v_dmg_time - cg.time) / DAMAGE_TIME));

		// blend the entire screen red
		if (redFlash > 5)
		{
			redFlash = 5;
		}

		col[3] = 0.7 * (redFlash / 5.0) * ((cg_bloodFlash.value > 1.0) ? 1.0 :
		                                   (cg_bloodFlash.value < 0.0) ? 0.0 :
		                                   cg_bloodFlash.value);

		CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, col);
	}
}

/*
=================
CG_DrawFlashFire
=================
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

		if (alpha >= 1.0)
		{
			alpha = 1.0;
		}

		// fade in?
		f = (float)(cg.time - cg.v_noFireTime) / FIRE_FLASH_FADEIN_TIME;
		if (f >= 0.0 && f < 1.0)
		{
			alpha = f;
		}

		max = 0.5 + 0.5 * sin((float)((cg.time / 10) % 1000) / 1000.0);
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

		trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)(255.0 * alpha), 0);
	}
	else
	{
		cg.v_noFireTime = cg.time;
	}
}

/*
==============
CG_DrawFlashBlendBehindHUD
    screen flash stuff drawn first (on top of world, behind HUD)
==============
*/
static void CG_DrawFlashBlendBehindHUD(void)
{
	CG_DrawFlashZoomTransition();
	CG_DrawFlashFade();
}

/*
=================
CG_DrawFlashBlend
    screen flash stuff drawn last (on top of everything)
=================
*/
static void CG_DrawFlashBlend(void)
{
	// no flash blends if in limbo or spectator, and in the limbo menu
	if ((cg.snap->ps.pm_flags & PMF_LIMBO || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) && cg.showGameView)
	{
		return;
	}

	CG_DrawFlashFire();
	CG_DrawFlashDamage();
}

/*
=================
CG_DrawObjectiveInfo
=================
*/
#define OID_LEFT    10
#define OID_TOP     360

void CG_ObjectivePrint(const char *str, int charWidth)
{
	char     *s;
	int      i, len;
	qboolean neednewline = qfalse;

	if (cg.centerPrintTime)
	{
		return;
	}

	s = CG_TranslateString(str);

	Q_strncpyz(cg.oidPrint, s, sizeof(cg.oidPrint));

	// turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.oidPrint);
	for (i = 0; i < len; i++)
	{
		// NOTE: subtract a few chars here so long words still get displayed properly
		if (i % (CP_LINEWIDTH - 20) == 0 && i > 0)
		{
			neednewline = qtrue;
		}
		if (cg.oidPrint[i] == ' ' && neednewline)
		{
			cg.oidPrint[i] = '\n';
			neednewline    = qfalse;
		}
	}

	cg.oidPrintTime      = cg.time;
	cg.oidPrintY         = OID_TOP;
	cg.oidPrintCharWidth = charWidth;

	// count the number of lines for oiding
	cg.oidPrintLines = 1;
	s                = cg.oidPrint;
	while (*s)
	{
		if (*s == '\n')
		{
			cg.oidPrintLines++;
		}
		s++;
	}
}

static void CG_DrawObjectiveInfo(void)
{
	char   *start;
	int    l;
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

	start = cg.oidPrint;

	y = 400 - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	x1 = 319;
	y1 = y - 2;
	x2 = 321;

	// first just find the bounding rect
	while (1)
	{
		char linebuffer[1024];

		for (l = 0; l < CP_LINEWIDTH; l++)
		{
			if (!start[l] || start[l] == '\n')
			{
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen(linebuffer) + 10;

		if (320 - w / 2 < x1)
		{
			x1 = 320 - w / 2;
			x2 = 320 + w / 2;
		}

		x = 320 - w / 2;

		y += cg.oidPrintCharWidth * 1.5;

		while (*start && (*start != '\n'))
		{
			start++;
		}
		if (!*start)
		{
			break;
		}
		start++;
	}

	x2 = x2 + 4;
	y2 = y - cg.oidPrintCharWidth * 1.5 + 4;

	VectorCopy(color, backColor);
	backColor[3] = 0.5 * color[3];
	trap_R_SetColor(backColor);

	CG_DrawPic(x1 + cgs.wideXoffset, y1, x2 - x1, y2 - y1, cgs.media.teamStatusBar);

	VectorSet(backColor, 0, 0, 0);
	CG_DrawRect(x1 + cgs.wideXoffset, y1, x2 - x1, y2 - y1, 1, backColor);

	trap_R_SetColor(color);

	// do the actual drawing
	start = cg.oidPrint;
	y     = 400 - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	while (1)
	{
		char linebuffer[1024];

		for (l = 0; l < CP_LINEWIDTH; l++)
		{
			if (!start[l] || start[l] == '\n')
			{
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen(linebuffer);
		if (x1 + w > x2)
		{
			x2 = x1 + w;
		}

		x = 320 - w / 2;

		CG_DrawStringExt(x + cgs.wideXoffset, y, linebuffer, color, qfalse, qtrue,
		                 cg.oidPrintCharWidth, (int)(cg.oidPrintCharWidth * 1.5), 0);

		y += cg.oidPrintCharWidth * 1.5;

		while (*start && (*start != '\n'))
		{
			start++;
		}
		if (!*start)
		{
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}

//==================================================================================

void CG_DrawTimedMenus(void)
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

/*
=================
CG_Fade
=================
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
}

/*
=================
CG_ScreenFade
=================
*/
static void CG_ScreenFade(void)
{
	int msec;

	if (!cg.fadeRate)
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

		if (!cg.fadeColor1[3])
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

		if (color[3])
		{
			CG_FillRect(0, 0, Ccg_WideX(SCREEN_WIDTH), SCREEN_HEIGHT, color);
		}
	}
}

void CG_DrawDemoRecording(void)
{
	char status[1024];
	char demostatus[128];
	char wavestatus[128];

	if (!cl_demorecording.integer && !cl_waverecording.integer)
	{
		return;
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
		strncpy(demostatus, "", sizeof(demostatus));
	}

	if (cl_waverecording.integer)
	{
		Com_sprintf(wavestatus, sizeof(demostatus), " audio %s: %ik ", cl_wavefilename.string, cl_waveoffset.integer / 1024);
	}
	else
	{
		strncpy(wavestatus, "", sizeof(wavestatus));
	}

	Com_sprintf(status, sizeof(status), "RECORDING%s%s", demostatus, wavestatus);

	CG_Text_Paint_Ext(5, cg_recording_statusline.integer, 0.2f, 0.2f, colorWhite, status, 0, 0, 0, &cgs.media.limboFont2);
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(void)
{
	CG_ScreenFade();
	// no 2d when in esc menu
	// FIXME: do allow for quickchat (bleh)
	// - Removing for now
	//if( trap_Key_GetCatcher() & KEYCATCH_UI ) {
	//  return;
	//}

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

	if (!cg.cameraMode)
	{
		CG_DrawFlashBlendBehindHUD();

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		{
			CG_DrawSpectator();
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
			}

			CG_DrawTeamInfo();

			if (cg_drawStatus.integer)
			{
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
		}

		CG_DrawVote();

		CG_DrawLagometer();
	}

	// don't draw center string if scoreboard is up
	if (!CG_DrawScoreboard())
	{
		CG_SetHud();

		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
		{
			CG_DrawActiveHud();
		}

		if (!cg_paused.integer)
		{
			CG_DrawUpperRight();
		}

		CG_DrawCenterString();
		CG_DrawFollow();
		CG_DrawWarmup();
		CG_DrawNotify();
		CG_DrawGlobalHud();
		CG_DrawObjectiveInfo();
		CG_DrawSpectatorMessage();
		CG_DrawLimboMessage();
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

	// Info overlays
	CG_DrawOverlays();

	// window updates
	CG_windowDraw();

	// draw flash blends now
	CG_DrawFlashBlend();

	CG_DrawDemoRecording();
}

void CG_StartShakeCamera(float p)
{
	cg.cameraShakeScale = p;

	cg.cameraShakeLength = 1000 * (p * p);
	cg.cameraShakeTime   = cg.time + cg.cameraShakeLength;
	cg.cameraShakePhase  = crandom() * M_PI; // start chain in random dir
}

void CG_ShakeCamera(void)
{
	float x, val;

	if (cg.time > cg.cameraShakeTime)
	{
		cg.cameraShakeScale = 0; // all pending explosions resolved, so reset shakescale
		return;
	}

	// starts at 1, approaches 0 over time
	x = (cg.cameraShakeTime - cg.time) / cg.cameraShakeLength;

	// move the camera
	val                   = sin(M_PI * 7 * x + cg.cameraShakePhase) * x * 4.0f * cg.cameraShakeScale;
	cg.refdef.vieworg[2] += val;
	val                   = sin(M_PI * 13 * x + cg.cameraShakePhase) * x * 4.0f * cg.cameraShakeScale;
	cg.refdef.vieworg[1] += val;
	val                   = cos(M_PI * 17 * x + cg.cameraShakePhase) * x * 4.0f * cg.cameraShakeScale;
	cg.refdef.vieworg[0] += val;

	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
}

void CG_DrawMiscGamemodels(void)
{
	int         i, j;
	refEntity_t ent;
	int         drawn = 0;

	memset(&ent, 0, sizeof(ent));

	ent.reType            = RT_MODEL;
	ent.nonNormalizedAxes = qtrue;

	// static gamemodels don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for (i = 0; i < cg.numMiscGameModels; i++)
	{
		if (cgs.miscGameModels[i].radius)
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

		/*      {
		            vec3_t v;
		            vec3_t vu = { 0.f, 0.f, 1.f };
		            vec3_t vl = { 0.f, 1.f, 0.f };
		            vec3_t vf = { 1.f, 0.f, 0.f };

		            VectorCopy( cgs.miscGameModels[i].org, v );
		            VectorMA( v, cgs.miscGameModels[i].radius, vu, v );
		            CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

		            VectorCopy( cgs.miscGameModels[i].org, v );
		            VectorMA( v, cgs.miscGameModels[i].radius, vf, v );
		            CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

		            VectorCopy( cgs.miscGameModels[i].org, v );
		            VectorMA( v, cgs.miscGameModels[i].radius, vl, v );
		            CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

		            VectorCopy( cgs.miscGameModels[i].org, v );
		            VectorMA( v, -cgs.miscGameModels[i].radius, vu, v );
		            CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

		            VectorCopy( cgs.miscGameModels[i].org, v );
		            VectorMA( v, -cgs.miscGameModels[i].radius, vf, v );
		            CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

		            VectorCopy( cgs.miscGameModels[i].org, v );
		            VectorMA( v, -cgs.miscGameModels[i].radius, vl, v );
		            CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );
		        }*/

		for (j = 0; j < 3; j++)
		{
			VectorCopy(cgs.miscGameModels[i].axes[j], ent.axis[j]);
		}
		ent.hModel = cgs.miscGameModels[i].model;

		trap_R_AddRefEntityToScene(&ent);

		drawn++;
	}
}

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
			if (DotProduct(dir, cg.refdef_current->viewaxis[0]) >= -0.6)
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
				if (tr.fraction == 1)
				{
					visible = qtrue;
				}
				trap_R_AddCoronaToScene(cgs.corona[i].org, cgs.corona[i].color[0], cgs.corona[i].color[1], cgs.corona[i].color[2], cgs.corona[i].scale, i, visible);
			}
		}
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(stereoFrame_t stereoView)
{
	float  separation;
	vec3_t baseOrg;

	// optionally draw the info screen instead
	if (!cg.snap)
	{
		CG_DrawInformation(qfalse);
		return;
	}

	switch (stereoView)
	{
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error("CG_DrawActive: Undefined stereoView\n");
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy(cg.refdef_current->vieworg, baseOrg);
	if (separation != 0)
	{
		VectorMA(cg.refdef_current->vieworg, -separation, cg.refdef_current->viewaxis[1], cg.refdef_current->vieworg);
	}

	cg.refdef_current->glfog.registered = 0;    // make sure it doesn't use fog from another scene

	CG_ShakeCamera();
	CG_PB_RenderPolyBuffers();
	CG_DrawMiscGamemodels();
	CG_Coronas();

	if (!(cg.limboEndCinematicTime > cg.time && cg.showGameView))
	{
		trap_R_RenderScene(cg.refdef_current);
	}

	// restore original viewpoint if running stereo
	if (separation != 0)
	{
		VectorCopy(baseOrg, cg.refdef_current->vieworg);
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
