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
 * @file dl_main_stubs.c
 * @brief Dummy methods for downloading files
 *
 * This file is included only when there is no other way of downloading files.
 */

#include "dl_public.h"
#include "q_shared.h"

/**
 * @brief DL_BeginDownload
 * @param localName - unused
 * @param remoteName - unused
 * @return
 */
unsigned int DL_BeginDownload(const char *localName, const char *remoteName, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	return 0;
}

unsigned int Web_CreateRequest(const char *url, const char *authToken, webUploadData_t *upload, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	return 0;
}

/**
 * @brief DL_DownloadLoop
 * @return
 *
 * @note Maybe this should be CL_DL_DownloadLoop
 */
void DL_DownloadLoop(void)
{
}

void DL_AbortAll(qboolean block, qboolean allowContinue)
{
}

/**
 * @brief DL_Shutdown
 */
void DL_Shutdown(void)
{
}
