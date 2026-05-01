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
 * @file tr_common_vulkan.c
 * @brief Vulkan equivalent of tr_common_opengl.c: bridges the engine's
 * GLimp_* window-system callbacks to the Vulkan subsystem in vulkan/vk.c.
 * No GL context is ever created for this renderer.
 */

#include "tr_common.h"
#include "tr_local_proxy.h"
#include "../renderer_vk/vulkan/vk.h"

/**
 * @brief RE_InitOpenGlSubsystems
 *
 * Called back from GLimp_SetMode() once the (GL-less, Vulkan-flagged)
 * window has been created. This is where the Vulkan instance/surface/
 * device/swapchain are created, since they require a live window handle.
 * @return qtrue on success
 */
int RE_InitOpenGlSubsystems(void)
{
	if (!VK_Init(glConfig.vidWidth, glConfig.vidHeight))
	{
		Ren_Print("RE_InitOpenGlSubsystems: VK_Init failed\n");
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief RE_InitOpenGl
 *
 * Called back from GLimp_Init() after GLimp_StartDriverAndSetMode()
 * succeeds. Vulkan setup is already complete by this point (done in
 * RE_InitOpenGlSubsystems), so this just finalizes the reported driver
 * info.
 */
void RE_InitOpenGl(void)
{
	glConfig.driverType           = GLDRV_ICD;
	glConfig.hardwareType         = GLHW_GENERIC;
	glConfig.extensions_string[0] = '\0';
}

/**
 * @brief R_DoGLimpShutdown
 *
 * Tears down the Vulkan subsystem before handing off to the engine to
 * destroy the window itself.
 */
void R_DoGLimpShutdown(void)
{
	VK_Shutdown();

	ri.GLimp_Shutdown();

	Com_Memset(&glConfig, 0, sizeof(glConfig));
	Com_Memset(&glState, 0, sizeof(glState));
}
