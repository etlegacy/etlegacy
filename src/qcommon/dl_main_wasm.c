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
 * @file dl_main_wasm.c
 * @brief In-game HTTP download implementation for Emscripten, using the
 *        Emscripten Fetch API instead of libcurl.
 *
 * This mirrors the request bookkeeping in dl_main_curl.c but drives the
 * actual transfer asynchronously through emscripten_fetch. There is no
 * multi-handle to pump: the browser drives the fetch and invokes our
 * callbacks directly, so DL_DownloadLoop is a no-op.
 */
#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#include <emscripten/emscripten.h>
#include "q_shared.h"
#include "qcommon.h"
#include "dl_public.h"

static struct
{
	qboolean initialized;
	qboolean abort;

	webRequest_t *requests;
	unsigned int requestId;
} webSys = { qfalse, qfalse, NULL, 0 };

static unsigned int DL_GetRequestId(void)
{
	while (qtrue)
	{
		webRequest_t **lst;

		unsigned int tmp = 1 + (++webSys.requestId);

		// 0 is an invalid id, and 1 is reserved
		if (tmp == 0 || tmp == FILE_DOWNLOAD_ID)
		{
			continue;
		}

		lst = &webSys.requests;

		while (*lst)
		{
			if ((*lst)->id == tmp)
			{
				tmp = 0;
				break;
			}

			lst = &(*lst)->next;
		}

		if (!tmp)
		{
			continue;
		}

		return tmp;
	}
}

static webRequest_t *DL_CreateRequest(void)
{
	webRequest_t *request = Com_Allocate(sizeof(webRequest_t));

	if (!request)
	{
		Com_Error(ERR_FATAL, "Cannot allocate memory for the request\n");
		return NULL;
	}
	Com_Memset(request, 0, sizeof(webRequest_t));
	request->id = DL_GetRequestId();

	request->next   = webSys.requests;
	webSys.requests = request;

	return request;
}

static webRequest_t *DL_GetRequestById(unsigned int id)
{
	webRequest_t **lst = &webSys.requests;

	while (*lst)
	{
		if ((*lst)->id == id)
		{
			return *lst;
		}

		lst = &(*lst)->next;
	}

	return NULL;
}

static void DL_FreeRequest(webRequest_t *request)
{
	webRequest_t **lst = &webSys.requests;

	while (*lst)
	{
		if (*lst == request)
		{
			*lst = request->next;
			break;
		}

		lst = &(*lst)->next;
	}

	if (request->data.fileHandle)
	{
		fclose(request->data.fileHandle);
		request->data.fileHandle = NULL;
	}

	if (request->data.buffer)
	{
		Com_Dealloc(request->data.buffer);
		request->data.buffer = NULL;
	}

	// rawHandle here points at the emscripten_fetch_t*, already closed
	// by the time we get here (see DL_cb_Success / DL_cb_Error).
	request->rawHandle = NULL;

	Com_Dealloc(request);
}

static void DL_InitDownload(void)
{
	if (webSys.initialized)
	{
		return;
	}

	Com_Printf("Client download subsystem initialized (Emscripten Fetch)\n");
	webSys.initialized = qtrue;
	webSys.abort        = qfalse;
}

static void DL_cb_Success(emscripten_fetch_t *fetch)
{
	webRequest_t *request = (webRequest_t *)fetch->userData;
	size_t       written;

	if (!request)
	{
		emscripten_fetch_close(fetch);
		return;
	}

	if (request->data.fileHandle)
	{
		written = fwrite(fetch->data, 1, (size_t)fetch->numBytes, request->data.fileHandle);
		if (written != (size_t)fetch->numBytes)
		{
			Com_Printf(S_COLOR_RED "DL_cb_Success: Error - short write (%d/%d bytes) for '%s'\n",
			           (int)written, (int)fetch->numBytes, request->data.name);
		}
		fclose(request->data.fileHandle);
		request->data.fileHandle = NULL;
	}

	request->httpCode = fetch->status;

	emscripten_fetch_close(fetch);
	request->rawHandle = NULL;

	if (request->complete_clb)
	{
		if (webSys.abort || request->abort)
		{
			request->complete_clb(request, REQUEST_ABORT);
		}
		else if (fetch->status >= 200 && fetch->status < 300)
		{
			request->complete_clb(request, REQUEST_OK);
		}
		else
		{
			Com_Printf(S_COLOR_RED "DL_cb_Success: Error - HTTP status %d for '%s'\n", (int)fetch->status, request->url);
			request->complete_clb(request, REQUEST_NOK);
		}
	}

	EM_ASM({
		if (typeof window.syncFiles === 'function')
		{
			window.syncFiles();
		}
	});

	DL_FreeRequest(request);
}

static void DL_cb_Error(emscripten_fetch_t *fetch)
{
	webRequest_t *request = (webRequest_t *)fetch->userData;

	if (!request)
	{
		emscripten_fetch_close(fetch);
		return;
	}

	Com_Printf(S_COLOR_RED "DL_cb_Error: Error - request failed with status %d for '%s'\n", (int)fetch->status, request->url);

	if (request->data.fileHandle)
	{
		fclose(request->data.fileHandle);
		request->data.fileHandle = NULL;
	}

	request->httpCode = fetch->status;

	emscripten_fetch_close(fetch);
	request->rawHandle = NULL;

	if (request->complete_clb)
	{
		request->complete_clb(request, (webSys.abort || request->abort) ? REQUEST_ABORT : REQUEST_NOK);
	}

	DL_FreeRequest(request);
}

static void DL_cb_Progress(emscripten_fetch_t *fetch)
{
	webRequest_t *request = (webRequest_t *)fetch->userData;

	if (!request)
	{
		return;
	}

	if (webSys.abort || request->abort)
	{
		return;
	}

	if (!request->data.requestLength && fetch->totalBytes)
	{
		request->data.requestLength = (size_t)fetch->totalBytes;
	}

	if (request->progress_clb)
	{
		request->progress_clb(request, (double)fetch->dataOffset + (double)fetch->numBytes, (double)fetch->totalBytes);
	}
}

/**
 * @brief DL_BeginDownload
 * @param[in] localName local file path to write the download to
 * @param[in] remoteName full remote URL to fetch
 */
unsigned int DL_BeginDownload(const char *localName, const char *remoteName, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	webRequest_t         *request;
	emscripten_fetch_attr_t attr;

	if (DL_GetRequestById(FILE_DOWNLOAD_ID))
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - called with a download request already active\n");
		return 0;
	}

	if (!localName[0] || !remoteName[0])
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - empty download URL or empty local file name\n");
		return 0;
	}

	if (FS_CreatePath(localName))
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - unable to create directory (%s).\n", localName);
		return 0;
	}

	request           = DL_CreateRequest();
	request->id       = FILE_DOWNLOAD_ID; // magical package download id
	request->userData = userData;
	Q_strncpyz(request->url, remoteName, ARRAY_LEN(request->url));
	Q_strncpyz(request->data.name, localName, ARRAY_LEN(request->data.name));

	request->data.fileHandle = Sys_FOpen(localName, "wb");
	if (!request->data.fileHandle)
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - unable to open '%s' for writing\n", localName);
		DL_FreeRequest(request);
		return 0;
	}

	DL_InitDownload();

	request->complete_clb = complete;
	request->progress_clb = progress;

	emscripten_fetch_attr_init(&attr);
	Q_strncpyz(attr.requestMethod, "GET", sizeof(attr.requestMethod));
	attr.attributes  = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess   = DL_cb_Success;
	attr.onerror     = DL_cb_Error;
	attr.onprogress  = DL_cb_Progress;
	attr.userData    = (void *)request;

	Cvar_Set("cl_downloadName", remoteName);

	request->rawHandle = (void *)emscripten_fetch(&attr, remoteName);

	if (!request->rawHandle)
	{
		Com_Printf(S_COLOR_RED "DL_BeginDownload: Error - emscripten_fetch failed to start.\n");
		if (request->complete_clb)
		{
			request->complete_clb(request, REQUEST_NOK);
		}
		DL_FreeRequest(request);
		return 0;
	}

	return request->id;
}

/**
 * @brief Web_CreateRequest
 * @note Unused in the WASM build.
 */
unsigned int Web_CreateRequest(const char *url, const char *authToken, webUploadData_t *upload, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	Com_Printf(S_COLOR_YELLOW "Web_CreateRequest: not implemented in the WASM build\n");
	return 0;
}

/**
 * @brief DL_DownloadLoop
 * @note No-op: emscripten_fetch is asynchronous and driven by the browser's
 * own event loop, invoking DL_cb_Success/DL_cb_Error/DL_cb_Progress directly.
 */
void DL_DownloadLoop(void)
{
}

void DL_AbortAll(qboolean block, qboolean allowContinue)
{
	webRequest_t *req = webSys.requests;

	webSys.abort = qtrue;

	while (req)
	{
		req->abort = qtrue;
		req = req->next;
	}

	if (!webSys.requests && allowContinue)
	{
		webSys.abort = qfalse;
	}
}

/**
 * @brief DL_Shutdown
 */
void DL_Shutdown(void)
{
	if (!webSys.initialized)
	{
		return;
	}

	DL_AbortAll(qtrue, qfalse);

	webSys.initialized = qfalse;
}
#endif
