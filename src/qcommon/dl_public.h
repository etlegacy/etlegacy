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
 * @file dl_public.h
 */

#ifndef INCLUDE_DL_PUBLIC_H
#define INCLUDE_DL_PUBLIC_H

#include "q_shared.h"

typedef enum
{
	REQUEST_NOK   = 0,
	REQUEST_OK    = BIT(0),
	REQUEST_ABORT = BIT(1)
} webRequestResult;

struct webRequest_s;

typedef void (*webCallbackFunc_t)(struct webRequest_s *request, webRequestResult requestResult);
typedef int (*webProgressCallbackFunc_t)(struct webRequest_s *request, double current, double total);

typedef struct webUploadData_s
{
	FILE *fileHandle;
	byte *buffer;
	size_t bufferSize;
	size_t bufferPos;
	char contentType[MAX_QPATH];
} webUploadData_t;

typedef struct
{
	char name[MAX_STRING_CHARS];
	size_t requestLength;

	FILE *fileHandle;

	size_t bufferSize;
	size_t bufferPos;
	byte *buffer;
} webRequestData_t;

typedef struct webRequest_s
{
	unsigned int id;
	char url[MAX_STRING_CHARS];

	qboolean upload;
	qboolean abort;

	webCallbackFunc_t complete_clb;
	webProgressCallbackFunc_t progress_clb;

	webRequestData_t data;
	void *userData;
	webUploadData_t *uploadData;

	long httpCode;

	void *rawHandle;
	void *cList;
	struct webRequest_s *next;
} webRequest_t;

#define FILE_DOWNLOAD_ID 1

unsigned int DL_BeginDownload(const char *localName, const char *remoteName, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress);
unsigned int Web_CreateRequest(const char *url, const char *authToken, webUploadData_t *upload, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress);
void DL_DownloadLoop(void);
void DL_AbortAll(qboolean block, qboolean allowContinue);
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
