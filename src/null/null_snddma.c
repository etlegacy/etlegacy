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
 * @file snddma_null.c
 * @brief  all other sound mixing is portable
 */

#include "../client/client.h"

qboolean SNDDMA_Init(void)
{
	return qfalse;
}

int SNDDMA_GetDMAPos(void)
{
	return 0;
}

void SNDDMA_Shutdown(void)
{
}

void SNDDMA_BeginPainting(void)
{
}

void SNDDMA_Submit(void)
{
}

// bk001119 - added boolean flag, match client/snd_public.h
sfxHandle_t S_RegisterSound(const char *name, qboolean compressed)
{
	return 0;
}

void S_StartLocalSound(sfxHandle_t sfxHandle, int channelNum, int volume)
{
}

void S_ClearSoundBuffer(qboolean killStreaming)
{
}

// added for win32 dedicated
void SNDDMA_Activate(void)
{
}

int S_GetSoundLength(sfxHandle_t sfxHandle)
{
	Com_Error(ERR_DROP, "null_snddma.c: S_GetSoundLength");
	return 0;
}

void S_UpdateThread(void)
{
}

void S_AddLoopSounds(void)
{
}
