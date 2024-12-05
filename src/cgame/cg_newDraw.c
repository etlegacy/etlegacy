/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file cg_newDraw.c
 */

#include "cg_local.h"

/**
 * @brief CG_TrimLeftPixels
 * @param[in,out] instr
 * @param[in] scale
 * @param[in] w
 * @param[in] size
 * @return
 *
 * @note Unused
 */
int CG_TrimLeftPixels(char *instr, float scale, float w, int size)
{
	char buffer[1024];
	char *p, *s;
	int  tw;
	int  i;

	Q_strncpyz(buffer, instr, 1024);
	Com_Memset(instr, 0, size);

	for (i = 0, p = buffer; *p; p++, i++)
	{
		instr[i] = *p;
		tw       = CG_Text_Width(instr, scale, 0);
		if (tw >= w)
		{
			Com_Memset(instr, 0, size);
			for (s = instr, p = &buffer[i + 1]; *p && ((s - instr) < size); p++, s++)
			{
				*s = *p;
			}
			return tw - w;
		}
	}

	return -1;
}

/**
 * @brief CG_FitTextToWidth_Ext
 * @param[in,out] instr
 * @param[in] scale
 * @param[in] w
 * @param[in] size
 * @param[in] font
 */
void CG_FitTextToWidth_Ext(char *instr, float scale, float w, int size, fontHelper_t *font)
{
	char buffer[1024];
	char *s, *p, *c, *ls = NULL;

	Q_strncpyz(buffer, instr, 1024);
	Com_Memset(instr, 0, size);

	c = s = instr;
	p = buffer;

	while (*p)
	{
		*c = *p++;

		if (*c == ' ')
		{
			ls = c;
		} // store last space, to try not to break mid word

		c++;

		if (*p == '\n')
		{
			s = c + 1;
		}
		else if (CG_Text_Width_Ext(s, scale, 0, font) > w)
		{
			if (ls)
			{
				*ls = '\n';
				s   = ls + 1;
			}
			else
			{
				*c       = *(c - 1);
				*(c - 1) = '\n';
				s        = c++;
			}

			ls = NULL;
		}
	}

	if (c != buffer && (*(c - 1) != '\n'))
	{
		*c++ = '\n';
	}

	*c = '\0';
}

/**
 * @brief CG_FitTextToWidth2
 * @param[in,out] instr
 * @param[in] scale
 * @param[in] w
 * @param[in] size
 *
 * @note Unused
 */
void CG_FitTextToWidth2(char *instr, float scale, float w, int size)
{
	char buffer[1024];
	char *s, *p, *c, *ls = NULL;

	Q_strncpyz(buffer, instr, 1024);
	Com_Memset(instr, 0, size);

	c = s = instr;
	p = buffer;

	while (*p)
	{
		*c = *p++;

		if (*c == ' ')
		{
			ls = c;
		} // store last space, to try not to break mid word

		c++;

		if (*p == '\n')
		{
			s = c + 1;
		}
		else if (CG_Text_Width(s, scale, 0) > w)
		{
			if (ls)
			{
				*ls = '\n';
				s   = ls + 1;
			}
			else
			{
				*c       = *(c - 1);
				*(c - 1) = '\n';
				s        = c++;
			}

			ls = NULL;
		}
	}

	if (c != buffer && (*(c - 1) != '\n'))
	{
		*c++ = '\n';
	}

	*c = '\0';
}

/**
 * @brief CG_FitTextToWidth_SingleLine
 * @param[in,out] instr
 * @param[in] scale
 * @param[in] w
 * @param[in] size
 *
 * @note Unused
 */
void CG_FitTextToWidth_SingleLine(char *instr, float scale, float w, int size)
{
	char *s, *p;
	char buffer[1024];

	Q_strncpyz(buffer, instr, 1024);
	Com_Memset(instr, 0, size);
	p = instr;

	for (s = buffer; *s; s++, p++)
	{
		*p = *s;
		if (CG_Text_Width(instr, scale, 0) > w)
		{
			*p = '\0';
			return;
		}
	}
}

/**
 * @brief CG_DrawPlayerWeaponIcon
 * @param[in] rect
 * @param drawHighlighted - unused
 * @param[in] align
 * @param[in] refcolor
 */
void CG_DrawPlayerWeaponIcon(rectDef_t *rect, int align, vec4_t *refcolor)
{
	int realweap;

	if (cg.predictedPlayerEntity.currentState.eFlags & EF_MOUNTEDTANK)
	{
		realweap = IS_MOUNTED_TANK_BROWNING(cg.snap->ps.clientNum) ? WP_MOBILE_BROWNING : WP_MOBILE_MG42;
	}
	else if ((cg.predictedPlayerEntity.currentState.eFlags & EF_MG42_ACTIVE) || (cg.predictedPlayerEntity.currentState.eFlags & EF_AAGUN_ACTIVE))
	{
		realweap = WP_MOBILE_MG42;
	}
	else
	{
		realweap = cg.predictedPlayerState.weapon;
	}

	if (cg_weapons[realweap].weaponIcon[1])
	{
		float size = MIN(rect->w, rect->h);
		float x    = rect->x;
		float y    = rect->y;
		float w    = MIN(size * cg_weapons[realweap].weaponIconScale, rect->w);
		float h    = size;

		switch (align)
		{
		case ITEM_ALIGN_CENTER:
		case ITEM_ALIGN_CENTER2:
			x += (rect->w - w) * 0.5f;
			break;
		case ITEM_ALIGN_RIGHT:
			x += (rect->w - w);
			break;
		case ITEM_ALIGN_LEFT:
		default:
			break;
		}

		// pulsing grenade icon to help the player 'count' in their head
		if (cg.predictedPlayerState.grenadeTimeLeft)
		{
			float scale = (float)((cg.predictedPlayerState.grenadeTimeLeft) % 1000) / 100.0f;

			x -= scale * 0.5f;
			y -= scale * 0.5f;
			w += scale;
			h += scale;
		}

		trap_R_SetColor(*refcolor);
		CG_DrawPic(x, y, w, h, cg_weapons[realweap].weaponIcon[1]);
		trap_R_SetColor(NULL);
	}
}

#define CURSORHINT_SCALE    10

/**
 * @brief CG_DrawCursorhint
 * @param[in] rect
 * @note
 * cg_cursorHints.integer ==
 *   0:  no hints
 *   1:  sin size pulse
 *   2:  one way size pulse
 *   3:  alpha pulse
 *   4+: static image
 */
void CG_DrawCursorhint(hudComponent_t *comp)
{
	float     *color;
	qhandle_t icon;
	float     scale = 0, halfscale = 0;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	switch (cg.cursorHintIcon)
	{
	case HINT_NONE:
	case HINT_FORCENONE:
	case HINT_COMPLETED:
		icon = 0;
		break;
	case HINT_DOOR:
		icon = cgs.media.doorHintShader;
		break;
	case HINT_DOOR_ROTATING:
		icon = cgs.media.doorRotateHintShader;
		break;
	case HINT_DOOR_LOCKED:
	case HINT_DOOR_ROTATING_LOCKED:
		icon = cgs.media.doorLockHintShader;
		break;
	case HINT_MG42:
		icon = cgs.media.mg42HintShader;
		break;
	case HINT_BREAKABLE:
		icon = cgs.media.breakableHintShader;
		break;
	case HINT_BREAKABLE_DYNAMITE:
		icon = cgs.media.dynamiteHintShader;
		break;
	case HINT_TANK:
		icon = cgs.media.tankHintShader;
		break;
	case HINT_SATCHELCHARGE:
		icon = cgs.media.satchelchargeHintShader;
		break;
	case HINT_CONSTRUCTIBLE:
		icon = cgs.media.buildHintShader;
		break;
	case HINT_UNIFORM:
		icon = cgs.media.uniformHintShader;
		break;
	case HINT_LANDMINE:
		icon = cgs.media.landmineHintShader;
		break;
	case HINT_CHAIR:
		icon = cgs.media.notUsableHintShader;
		break;
	case HINT_HEALTH:
		icon = cgs.media.healthHintShader;
		break;
	case HINT_KNIFE:
		icon = cgs.media.knifeHintShader;
		break;
	case HINT_LADDER:
		icon = cgs.media.ladderHintShader;
		break;
	case HINT_BUTTON:
		icon = cgs.media.buttonHintShader;
		break;
	case HINT_WATER:
		icon = cgs.media.waterHintShader;
		break;
	case HINT_WEAPON:
		icon = cgs.media.weaponHintShader;
		break;
	case HINT_AMMO:
		icon = cgs.media.ammoHintShader;
		break;
	case HINT_POWERUP:
		icon = cgs.media.powerupHintShader;
		break;
	case HINT_BUILD:
		icon = cgs.media.buildHintShader;
		break;
	case HINT_DISARM:
		icon = cgs.media.disarmHintShader;
		break;
	case HINT_REVIVE:
		icon = cgs.media.reviveHintShader;
		break;
	case HINT_DYNAMITE:
		icon = cgs.media.dynamiteHintShader;
		break;
	case HINT_RESTRICTED:
		icon = cgs.media.friendShader;
		break;
	case HINT_ACTIVATE:
	case HINT_BAD_USER:
	default:
		icon = cgs.media.usableHintShader;
		break;
	}

	if (!icon)
	{
		return;
	}

	// color
	color = CG_FadeColor(cg.cursorHintTime, cg.cursorHintFade);
	if (!color)
	{
		trap_R_SetColor(NULL);
		return;
	}

	// color
	if (comp->style & 4)
	{
		color[3] *= 0.5 + 0.5 * sin(cg.time / 150.0);
	}

	// strobe
	if (comp->style & 2)
	{
		scale     = (cg.cursorHintTime % 1000) / 100.0f; // one way size pulse
		halfscale = scale * 0.5f;
	}
	// size
	else if (comp->style & 1)
	{
		scale     = (float)(CURSORHINT_SCALE * (0.5 + 0.5 * sin(cg.time / 150.0))); // sin pulse
		halfscale = scale * 0.5f;
	}

	if (comp->showBackGround)
	{
		CG_FillRect(comp->location.x, comp->location.y, comp->location.w, comp->location.h, comp->colorBackground);
	}

	if (comp->showBorder)
	{
		CG_DrawRect_FixedBorder(comp->location.x, comp->location.y, comp->location.w, comp->location.h, 1, comp->colorBorder);
	}

	// set color and draw the hint
	trap_R_SetColor(color);
	CG_DrawPic(comp->location.x - halfscale, comp->location.y - halfscale, comp->location.w + scale, comp->location.h + scale, icon);

	trap_R_SetColor(NULL);
}

/**
 * @brief CG_DrawCursorHintBar
 * @param[in] comp
 */
void CG_DrawCursorHintBar(hudComponent_t *comp)
{
	float  *color;
	vec4_t textColor;
	float  curValue;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!cg.cursorHintValue)
	{
		return;
	}

	// color
	Vector4Copy(comp->colorMain, textColor);
	color = CG_FadeColor_Ext(cg.cursorHintTime, cg.cursorHintFade, textColor[3]);
	if (!color)
	{
		trap_R_SetColor(NULL);
		return;
	}

	curValue = (float)cg.cursorHintValue / 255.0f;

	if (curValue > 0.01f)
	{
		CG_FilledBar(comp->location.x, comp->location.y + comp->location.h, comp->location.w, comp->location.h, colorRed, colorGreen,
		             comp->colorBackground, comp->colorBorder, curValue, 0.f, comp->style, -1);
	}
}

/**
 * @brief CG_DrawCursorHintText
 * @param[in] comp
 */
void CG_DrawCursorHintText(hudComponent_t *comp)
{
	float      *color;
	vec4_t     textColor;
	const char *str;

	if (cgs.clientinfo[cg.clientNum].shoutcaster)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	if (!cg.cursorHintValue)
	{
		return;
	}

	// color
	Vector4Copy(comp->colorMain, textColor);
	color = CG_FadeColor_Ext(cg.cursorHintTime, cg.cursorHintFade, textColor[3]);
	if (!color)
	{
		trap_R_SetColor(NULL);
		return;
	}

	str = va("%.0f%s", MIN((cg.cursorHintValue / 255.f) * 100, 100), (comp->style & 1) ? " %" : "");

	textColor[3] = color[3];
	CG_DrawCompText(comp, str, textColor, comp->styleText, &cgs.media.limboFont1);
}

/**
 * @brief CG_GetValue
 * @param ownerDraw - unused
 * @param type - unused
 * @todo FIXME: what's this ??
 * @return
 */
float CG_GetValue(int ownerDraw, int type)
{
	return -1;
}

/**
 * @brief CG_OwnerDrawVisible
 * @param flags - unused
 * @return
 * @note THINKABOUTME: should these be exclusive or inclusive..
 */
qboolean CG_OwnerDrawVisible(int flags)
{
	return qfalse;
}

/**
 * @brief Draw a bar showing current stability level (0-255), max at current weapon/ability, and 'perfect' reference mark
 * probably only drawn for scoped weapons
 * @param[in] rect
 */
void CG_DrawWeapStability(hudComponent_t *comp)
{
	static vec4_t goodColor = { 0, 1, 0, 0.5f }, badColor = { 1, 0, 0, 0.5f };

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if (!(comp->style & 1) && !cg.zoomed && !cg.generatingNoiseHud)
	{
		// style '0' means only draw for scoped weapons, '1' means draw all the time
		return;
	}

	// don't draw while switching
	if ((cg.snap->ps.weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHFROM ||
	    (cg.snap->ps.weapAnim & ~ANIM_TOGGLEBIT) == WEAP_ALTSWITCHTO)
	{
		return;
	}

	if (!(cg.snap->ps.aimSpreadScale))
	{
		return;
	}

	if (cg.renderingThirdPerson)
	{
		return;
	}

	CG_FilledBar(comp->location.x, comp->location.y, comp->location.w, comp->location.h, goodColor, badColor,
	             comp->colorBackground, comp->colorBorder, (float)cg.snap->ps.aimSpreadScale / 255.0f, 0.f, comp->style >> 1, -1);
}

/**
 * @brief CG_DrawWeapHeat
 * @param[in] rect
 * @param[in] align
 * @param[in] dynamicColor
 */
void CG_DrawWeapHeat(rectDef_t *rect, int align, qboolean dynamicColor)
{
	static vec4_t color = { 1, 0, 0, 0.2f }, color2 = { 1, 0, 0, 0.5f };
	static vec4_t dynColor = { 1, 1, 0, 0.3f }, dynColor2 = { 1, 0, 0, 0.7f };
	int           flags = 0;

	if (!(cg.snap->ps.curWeapHeat))
	{
		return;
	}

	if (align != HUD_HORIZONTAL)
	{
		flags |= BAR_VERT;   // BAR_VERT
	}

	flags |= BAR_LEFT;             // this is hardcoded now, but will be decided by the menu script
	flags |= BAR_BG;               // draw the filled contrast box
	//flags|=BAR_BGSPACING_X0Y5;   // different style

	flags |= BAR_LERP_COLOR;

	if (dynamicColor)
	{
		CG_FilledBar(rect->x, rect->y, rect->w, rect->h, dynColor, dynColor2, NULL, NULL, (float)cg.snap->ps.curWeapHeat / 255.0f, 0.f, flags, -1);
	}
	else
	{
		CG_FilledBar(rect->x, rect->y, rect->w, rect->h, color, color2, NULL, NULL, (float)cg.snap->ps.curWeapHeat / 255.0f, 0.f, flags, -1);
	}
}

#ifdef FEATURE_EDV
int old_mouse_x_pos = 0, old_mouse_y_pos = 0;
#endif

/**
 * @brief CG_MouseEvent
 * @param[in] x
 * @param[in] y
 */
void CG_MouseEvent(int x, int y)
{
	switch (cgs.eventHandling)
	{
	case CGAME_EVENT_DEMO:
	case CGAME_EVENT_MULTIVIEW:
		if (x != 0 || y != 0)
		{
			cgs.cursorUpdate = cg.time + 5000;
		} // fall through
	case CGAME_EVENT_SPEAKEREDITOR:
	case CGAME_EVENT_CAMERAEDITOR:
	case CGAME_EVENT_GAMEVIEW:
	case CGAME_EVENT_CAMPAIGNBREIFING:
	case CGAME_EVENT_FIRETEAMMSG:
	case CGAME_EVENT_SHOUTCAST:
	case CGAME_EVENT_SPAWNPOINTMSG:
	case CGAME_EVENT_HUDEDITOR:

#ifdef FEATURE_EDV
		if (!cgs.demoCamera.renderingFreeCam)
		{
#endif
		int hudEditorSafeX = SCREEN_WIDTH_SAFE * HUD_EDITOR_SIZE_COEFF;
		int hudEditorSafeY = SCREEN_HEIGHT_SAFE * HUD_EDITOR_SIZE_COEFF;

		cgs.cursorX += x;
		if (cg.editingHud && !cg.fullScreenHudEditor)
		{
			cgs.cursorX = Com_Clamp(0, hudEditorSafeX, cgs.cursorX);
		}
		else
		{
			cgs.cursorX = Com_Clamp(0, SCREEN_WIDTH_SAFE, cgs.cursorX);
		}

		cgs.cursorY += y;
		if (cg.editingHud && !cg.fullScreenHudEditor)
		{
			cgs.cursorY = Com_Clamp(0, hudEditorSafeY, cgs.cursorY);
		}
		else
		{
			cgs.cursorY = Com_Clamp(0, SCREEN_HEIGHT_SAFE, cgs.cursorY);
		}

		if (cgs.eventHandling == CGAME_EVENT_SPEAKEREDITOR)
		{
			CG_SpeakerEditorMouseMove_Handling(x, y);
		}

		if (cgs.eventHandling == CGAME_EVENT_CAMERAEDITOR)
		{
			CG_CameraEditorMouseMove_Handling(x, y);
		}

		if (cgs.eventHandling == CGAME_EVENT_HUDEDITOR)
		{
			CG_HudEditorMouseMove_Handling(cgs.cursorX, cgs.cursorY);
		}
#ifdef FEATURE_EDV
	}
	else
	{
		// mousemovement *should* feel the same as ingame
		char buffer[64];
		int  mx = 0, my = 0;
		int  mouse_x_pos = 0, mouse_y_pos = 0;

		float sensitivity, m_pitch, m_yaw;
		int   m_filter = 0;

		if (demo_lookat.integer != -1)
		{
			return;
		}

		mx += x;
		my += y;

		trap_Cvar_VariableStringBuffer("m_filter", buffer, sizeof(buffer));
		m_filter = Q_atoi(buffer);

		trap_Cvar_VariableStringBuffer("sensitivity", buffer, sizeof(buffer));
		sensitivity = Q_atof(buffer);

		trap_Cvar_VariableStringBuffer("m_pitch", buffer, sizeof(buffer));
		m_pitch = Q_atof(buffer);

		trap_Cvar_VariableStringBuffer("m_yaw", buffer, sizeof(buffer));
		m_yaw = Q_atof(buffer);

		if (m_filter)
		{
			mouse_x_pos = (mx + old_mouse_x_pos) / 2;
			mouse_y_pos = (my + old_mouse_y_pos) / 2;
		}
		else
		{
			mouse_x_pos = mx;
			mouse_y_pos = my;
		}

		old_mouse_x_pos = mx;
		old_mouse_y_pos = my;

		mouse_x_pos *= sensitivity;
		mouse_y_pos *= sensitivity;

		cg.refdefViewAngles[YAW]   -= m_yaw * mouse_x_pos;
		cg.refdefViewAngles[PITCH] += m_pitch * mouse_y_pos;

		if (cg.refdefViewAngles[PITCH] < -90)
		{
			cg.refdefViewAngles[PITCH] = -90;
		}

		if (cg.refdefViewAngles[PITCH] > 90)
		{
			cg.refdefViewAngles[PITCH] = 90;
		}
	}
#endif
		break;
	default:
		if (cg.snap->ps.pm_type == PM_INTERMISSION)
		{
			CG_Debriefing_MouseEvent(x, y);
			return;
		}

		// default handling
		if ((cg.predictedPlayerState.pm_type == PM_NORMAL ||
		     cg.predictedPlayerState.pm_type == PM_SPECTATOR) &&
		    cg.showScores == qfalse)
		{
			trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);
			return;
		}
		break;
	}
}

/**
 * @brief Clean up sample HUD elements spawned by HUD editor
 */
void CG_HudEditor_Cleanup(void)
{
	int i;

	CG_InitPM();
	cg.bannerPrintTime  = 0;
	cg.centerPrintTime  = 0;
	cgs.voteTime        = 0;
	cg.cursorHintTime   = 0;
	cg.crosshairEntTime = 0;
	cg.oidPrintTime     = 0;

	for (i = 0; i < cg_teamChatHeight.integer; i++)
	{
		cgs.teamChatMsgTimes[i] = 0;
	}
}

/**
 * @brief CG_EventHandling
 * @param[in] type
 * @param[in] fForced
 */
void CG_EventHandling(int type, qboolean fForced)
{
	if (cg.demoPlayback && type == CGAME_EVENT_NONE && !fForced)
	{
		type = CGAME_EVENT_DEMO;
	}

	if (type != CGAME_EVENT_NONE)
	{
		trap_Cvar_Set("cl_bypassMouseInput", "0");
	}

	// assume we want to draw cursor
	cgDC.cursorVisible = qtrue;

	switch (type)
	{
	// Demo support
	case CGAME_EVENT_DEMO:
		cgs.fResize         = qfalse;
		cgs.fSelect         = qfalse;
		cgs.cursorUpdate    = cg.time + 10000;
		cgs.timescaleUpdate = cg.time + 4000;
		CG_ScoresUp_f();
		CG_HudEditorReset();
		break;

	case CGAME_EVENT_SPEAKEREDITOR:
	case CGAME_EVENT_GAMEVIEW:
	case CGAME_EVENT_NONE:
	case CGAME_EVENT_CAMPAIGNBREIFING:
	case CGAME_EVENT_FIRETEAMMSG:
	case CGAME_EVENT_SHOUTCAST:
	case CGAME_EVENT_SPAWNPOINTMSG:
	case CGAME_EVENT_MULTIVIEW:
	case CGAME_EVENT_HUDEDITOR:
	default:
		// default handling (cleanup mostly)
		if (cgs.eventHandling == CGAME_EVENT_GAMEVIEW)
		{
			cg.showGameView = qfalse;
			trap_S_FadeBackgroundTrack(0.0f, 500, 0);

			trap_S_StopStreamingSound(-1);
			cg.limboEndCinematicTime = 0;

			if (fForced)
			{
				if (cgs.limboLoadoutModified)
				{
					trap_SendClientCommand("rs");

					cgs.limboLoadoutSelected = qfalse;
				}
			}
		}
		else if (cgs.eventHandling == CGAME_EVENT_MULTIVIEW)
		{
			if (type == -CGAME_EVENT_MULTIVIEW)
			{
				type = CGAME_EVENT_NONE;
			}
			else
			{
				trap_Key_SetCatcher(KEYCATCH_CGAME);
				return;
			}
		}
		else if (cgs.eventHandling == CGAME_EVENT_SPEAKEREDITOR)
		{
			if (type == -CGAME_EVENT_SPEAKEREDITOR)
			{
				type = CGAME_EVENT_NONE;
			}
			else
			{
				trap_Key_SetCatcher(KEYCATCH_CGAME);
				return;
			}
		}
		else if (cgs.eventHandling == CGAME_EVENT_CAMERAEDITOR)
		{
			if (type == -CGAME_EVENT_CAMERAEDITOR)
			{
				type = CGAME_EVENT_NONE;
			}
			else
			{
				trap_Key_SetCatcher(KEYCATCH_CGAME);
				return;
			}
		}
		else if (cgs.eventHandling == CGAME_EVENT_HUDEDITOR)
		{
			CG_HudEditorReset();
		}
		else if (cgs.eventHandling == CGAME_EVENT_CAMPAIGNBREIFING)
		{
			type = CGAME_EVENT_GAMEVIEW;
		}
		else if (cgs.eventHandling == CGAME_EVENT_FIRETEAMMSG)
		{
			cg.showFireteamMenu = qfalse;
			trap_Cvar_Set("cl_bypassmouseinput", "0");
		}
		else if (cgs.eventHandling == CGAME_EVENT_SHOUTCAST)
		{
			if (fForced)
			{
				trap_UI_Popup(UIMENU_INGAME);
			}

			trap_Cvar_Set("cl_bypassmouseinput", "0");
		}
		else if (cgs.eventHandling == CGAME_EVENT_SPAWNPOINTMSG)
		{
			cg.showSpawnpointsMenu = qfalse;
			trap_Cvar_Set("cl_bypassmouseinput", "0");
		}
		else if (cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION && fForced)
		{
			trap_UI_Popup(UIMENU_INGAME);
		}

		break;
	}

	cgs.eventHandling = type;

	if (type == CGAME_EVENT_NONE)
	{
		trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);
		ccInitial = qfalse;
		if (cg.demoPlayback && cg.demohelpWindow != SHOW_OFF)
		{
			CG_ShowHelp_Off(&cg.demohelpWindow);
		}
	}
	else if (type == CGAME_EVENT_GAMEVIEW)
	{
		cg.showGameView = qtrue;
		CG_LimboPanel_Setup();
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
	else if (type == CGAME_EVENT_MULTIVIEW)
	{
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
	else if (type == CGAME_EVENT_FIRETEAMMSG)
	{
		cgs.ftMenuPos       = -1;
		cgs.ftMenuMode      = 0;
		cg.showFireteamMenu = qtrue;
		cgDC.cursorVisible  = qfalse;
		trap_Cvar_Set("cl_bypassmouseinput", "1");
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
	else if (type == CGAME_EVENT_SHOUTCAST)
	{
		cgDC.cursorVisible = qfalse;
		trap_Cvar_Set("cl_bypassmouseinput", "1");
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
	else if (type == CGAME_EVENT_SPAWNPOINTMSG)
	{
		cg.showSpawnpointsMenu = qtrue;
		cgDC.cursorVisible     = qfalse;
		trap_Cvar_Set("cl_bypassmouseinput", "1");
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
	else if (type == CGAME_EVENT_HUDEDITOR)
	{
		CG_HudEditorSetup();
		cg.editingHud          = qtrue;
		cg.generatingNoiseHud  = qfalse;
		cg.fullScreenHudEditor = qfalse;
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
	else
	{
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
}

void CG_KeyEvent(int key, qboolean down)
{
	switch (cgs.eventHandling)
	{
	// Demos get their own keys
	case CGAME_EVENT_DEMO:
#ifdef FEATURE_EDV
		if (cg_predefineddemokeys.integer)
		{
			CG_DemoClick(key, down);
		}
		else
		{
			CG_RunBinding(key, down);
		}
		return;
#else
		CG_DemoClick(key, down);
		return;
#endif
	case CGAME_EVENT_CAMPAIGNBREIFING:
		CG_LoadPanel_KeyHandling(key, down);
		break;
	case CGAME_EVENT_FIRETEAMMSG:
		CG_Fireteams_KeyHandling(key, down);
		break;
	case CGAME_EVENT_SHOUTCAST:
		CG_Shoutcast_KeyHandling(key, down);
		break;
	case CGAME_EVENT_SPAWNPOINTMSG:
		CG_Spawnpoints_KeyHandling(key, down);
		break;
	case CGAME_EVENT_GAMEVIEW:
		CG_LimboPanel_KeyHandling(key, down);
		break;
	case CGAME_EVENT_SPEAKEREDITOR:
		CG_SpeakerEditor_KeyHandling(key, down);
		break;
	case CGAME_EVENT_CAMERAEDITOR:
		CG_CameraEditor_KeyHandling(key, down);
		break;
	case CGAME_EVENT_HUDEDITOR:
		CG_HudEditor_KeyHandling(key, down);
		break;
#ifdef FEATURE_MULTIVIEW
	case  CGAME_EVENT_MULTIVIEW:
#ifdef FEATURE_EDV
		if (cg_predefineddemokeys.integer)
		{
			CG_mv_KeyHandling(key, down);
		}
		else
		{
			CG_RunBinding(key, down);
		}
#else
		CG_mv_KeyHandling(key, down);
#endif
		break;
#endif
	default:
		if (cg.snap->ps.pm_type == PM_INTERMISSION)
		{
			CG_Debriefing_KeyEvent(key, down);
			return;
		}

		// default handling
		if (!down)
		{
			return;
		}

		if ((cg.predictedPlayerState.pm_type == PM_NORMAL ||
		     (cg.predictedPlayerState.pm_type == PM_SPECTATOR && cg.showScores == qfalse)))
		{

			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
			return;
		}
		break;
	}
}

/**
 * @brief CG_GetTeamColor
 * @param[out] color
 */
void CG_GetTeamColor(vec4_t *color)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS)
	{
		(*color)[0] = 1;
		(*color)[3] = .25f;
		(*color)[1] = (*color)[2] = 0;
	}
	else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES)
	{
		(*color)[0] = (*color)[1] = 0;
		(*color)[2] = 1;
		(*color)[3] = .25f;
	}
	else
	{
		(*color)[0] = (*color)[2] = 0;
		(*color)[1] = .17f;
		(*color)[3] = .25f;
	}
}

/**
 * @brief CG_RunMenuScript
 * @param args - unused
 * @todo Unused function ?
 */
void CG_RunMenuScript(char **args)
{
}
