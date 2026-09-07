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
 * @file renderer_vk/tr_gamma.c
 * @brief screen gamma. renderer1 runs a GLSL program over its
 * main FBO; the Vulkan backend draws the frame into an offscreen image and
 * VK_EndFrame() copies it to the swapchain applying the same
 * pow(rgb, 1 / r_gamma) * (1 << overbrightBits). This file only feeds it.
 */

#include "tr_local.h"
#include "vulkan/vk.h"

/**
 * @brief R_ScreenGamma - per-frame gamma/overbright values for the final
 * pass (called from RB_GammaScreen() before the swap).
 */
void R_ScreenGamma(void)
{
	if (tr.gammaProgramUsed)
	{
		VK_SetGammaParams(r_gamma->value, (float)(1 << tr.overbrightBits));
	}
	else
	{
		VK_SetGammaParams(1.0f, 1.0f);
	}
}

/**
 * @brief R_InitGamma - same gating as renderer1 (r_ignorehwgamma disables
 * the gamma program). Must run before R_InitImages(), since
 * R_SetColorMappings() keeps overbright in windowed mode only when
 * tr.gammaProgramUsed is set.
 */
void R_InitGamma(void)
{
	tr.gammaProgramUsed = qfalse;

	if (ri.Cvar_VariableIntegerValue("r_ignorehwgamma"))
	{
		Ren_Print("INFO: R_InitGamma() skipped - r_ignorehwgamma is set\n");
		return;
	}

	tr.gammaProgramUsed = qtrue;
}

/**
 * @brief R_ShutdownGamma
 */
void R_ShutdownGamma(void)
{
	tr.gammaProgramUsed = qfalse;
}
