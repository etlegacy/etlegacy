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
 * @file dl_public.h
 */

#ifndef INCLUDE_DL_PUBLIC_H
#define INCLUDE_DL_PUBLIC_H

typedef enum
{
	DL_CONTINUE,
	DL_DONE,
	DL_FAILED
} dlStatus_t;

int DL_BeginDownload(const char *localName, const char *remoteName);
char *DL_GetString(const char *url);
dlStatus_t DL_DownloadLoop(void);

void DL_Shutdown(void);

/**
 * @enum dlFlags_t
 * @brief bitmask
 */
typedef enum
{
	DL_FLAG_DISCON = 0,
	DL_FLAG_URL
} dlFlags_t;

#endif // #ifndef INCLUDE_DL_PUBLIC_H
