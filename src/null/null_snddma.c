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
 * @file null_snddma.c
 * @brief All other sound mixing is portable
 */

#include "../client/client.h"

/**
 * @brief SNDDMA_Init
 * @return
 */
qboolean SNDDMA_Init(void)
{
	return qfalse;
}

/**
 * @brief SNDDMA_GetDMAPos
 * @return
 */
int SNDDMA_GetDMAPos(void)
{
	return 0;
}

/**
 * @brief SNDDMA_Shutdown
 */
void SNDDMA_Shutdown(void)
{
}

/**
 * @brief SNDDMA_BeginPainting
 */
void SNDDMA_BeginPainting(void)
{
}

/**
 * @brief SNDDMA_Submit
 */
void SNDDMA_Submit(void)
{
}

/**
 * @brief S_RegisterSound
 * @param name
 * @param compressed
 * @return
 *
 * @note bk001119 - added boolean flag, match client/snd_public.h
 */
sfxHandle_t S_RegisterSound(const char *name, qboolean compressed)
{
	return 0;
}

/**
 * @brief S_StartLocalSound
 * @param sfxHandle
 * @param channelNum
 * @param volume
 */
void S_StartLocalSound(sfxHandle_t sfxHandle, int channelNum, int volume)
{
}

/**
 * @brief S_ClearSoundBuffer
 * @param killStreaming
 */
void S_ClearSoundBuffer(qboolean killStreaming)
{
}

/**
 * @brief S_GetSoundLength
 * @param sfxHandle
 * @return
 */
int S_GetSoundLength(sfxHandle_t sfxHandle)
{
	Com_Error(ERR_DROP, "null_snddma.c: S_GetSoundLength");
	
	return 0;
}

/**
 * @brief S_UpdateThread
 *
 * @note Unused
 */
void S_UpdateThread(void)
{
}

/**
 * @brief S_AddLoopSounds
 *
 * @todo TODO: empty
 */
void S_AddLoopSounds(void)
{
}
