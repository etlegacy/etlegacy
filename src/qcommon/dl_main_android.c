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
 * @file dl_main_android.c
 * @brief Download system implementation that uses the Android's system to do the http interactions
 */

/*
    Java VM Type signatures

    Type Signature                 Java Type
    Z 	                           boolean
    B                              byte
    C                              char
    S                              short
    I 	                           int
    J 	                           long
    F 	                           float
    D                              double
    L fully-qualified-class ;      fully-qualified-class
    [ type 	                       type[]
    ( arg-types ) ret-type         method type
 */

#include "dl_public.h"
#include "q_shared.h"
#include "qcommon.h"
#include "../sdl/sdl_defs.h"
#include <jni.h>

#ifdef DEDICATED
#error "This file is only supported on client code!"
#endif

static struct
{
	JNIEnv *download_env;
	jobject download_singleton;
	qboolean init;
	uint32_t requestId;
	webRequest_t *requests;
} androidSys = { NULL, NULL, qfalse, 0, NULL };

JNIEXPORT void JNICALL Java_org_etlegacy_app_web_ETLDownload_init(JNIEnv *env, jobject this)
{
	Com_Printf("Download system init from Android");
	androidSys.download_env       = env;
	androidSys.download_singleton = this;
	androidSys.init               = qtrue;
}

static void DL_HandleInit()
{
	if (androidSys.init)
	{
		return;
	}
	// If the java code has not done the init, then just call it ourselves
	JNIEnv    *env = SDL_AndroidGetJNIEnv();
	jclass    cls  = (*env)->FindClass(env, "org/etlegacy/app/web/ETLDownload");
	jmethodID id   = (*env)->GetStaticMethodID(env, cls, "instance", "()Lorg/etlegacy/app/web/ETLDownload;");
	(*env)->CallStaticObjectMethod(env, cls, id);
}

static unsigned int DL_GetRequestId()
{
	while (qtrue)
	{
		unsigned int tmp = 1 + (++androidSys.requestId);

		// 0 is an invalid id, and 1 is reserved
		if (tmp == 0 || tmp == FILE_DOWNLOAD_ID)
		{
			continue;
		}

		// wrap around protection
		// very highly unlikely that this ever happens, but you never know
		webRequest_t **lst = &androidSys.requests;

		while (*lst)
		{
			if ((*lst)->id == tmp)
			{
				tmp = 0;
				break;
			}

			lst = &(*lst)->next;
		}

		// so we found the id from the existing request, try again..
		if (!tmp)
		{
			continue;
		}

		return tmp;
	}
}

static webRequest_t *DL_CreateRequest()
{
	webRequest_t *request = Com_Allocate(sizeof(webRequest_t));
	if (!request)
	{
		Com_Error(ERR_FATAL, "Cannot allocate memory for the request\n");
		return NULL;
	}
	Com_Memset(request, 0, sizeof(webRequest_t));
	request->id = DL_GetRequestId();

	request->next       = androidSys.requests;
	androidSys.requests = request;

	return request;
}

static webRequest_t *DL_GetRequestById(unsigned int id)
{
	webRequest_t **lst = &androidSys.requests;

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
	webRequest_t **lst = &androidSys.requests;

	// pop from the list
	while (*lst)
	{
		if (*lst == request)
		{
			*lst = request->next;
			break;
		}
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

	// TODO: will we have some handle to a java download object?
	// if (request->rawHandle)
	// {
	//     curl_easy_cleanup(request->rawHandle);
	//     request->rawHandle = NULL;
	// }

	Com_Dealloc(request);
}

unsigned int DL_BeginDownload(const char *localName, const char *remoteName, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	webRequest_t *request;
	char         referer[MAX_STRING_CHARS + 5 /*"et://"*/];

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

	request     = DL_CreateRequest();
	request->id = FILE_DOWNLOAD_ID; // magical package download id
	Q_strncpyz(request->url, remoteName, ARRAY_LEN(request->url));

	request->data.fileHandle = Sys_FOpen(localName, "wb");
	if (!request->data.fileHandle)
	{
		Com_Printf(S_COLOR_RED  "DL_BeginDownload: Error - unable to open '%s' for writing\n", localName);
		DL_FreeRequest(request);
		return 0;
	}

	DL_HandleInit();

	/* ET://ip:port */
	strcpy(referer, "et://");
	Q_strncpyz(referer + 5, Cvar_VariableString("cl_currentServerIP"), MAX_STRING_CHARS);

	request->complete_clb = complete;
	request->progress_clb = progress;

	JNIEnv    *env   = androidSys.download_env;
	jmethodID method = (*env)->GetMethodID(env, androidSys.download_singleton, "beginDownload", "(Ljava/lang/String;Ljava/lang/String;J)V");

	jstring localString  = (*env)->NewStringUTF(env, localName);
	jstring remoteString = (*env)->NewStringUTF(env, remoteName);
	jlong   requestId    = (jlong)request->id;

	(*env)->CallVoidMethod(env, androidSys.download_singleton, method, localString, remoteString, requestId);

	Cvar_Set("cl_downloadName", remoteName);

	return request->id;
}

unsigned int Web_CreateRequest(const char *url, const char *authToken, webUploadData_t *upload, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
	webRequest_t *request;

	if (!url || !*url)
	{
		Com_Printf(S_COLOR_RED "DL_GetString: Error - empty download URL\n");
		return qfalse;
	}

	if (upload && upload->fileHandle)
	{
		Com_Error(ERR_DROP, S_COLOR_RED "Android does not yet support file upload!\n");
		return qfalse;
	}

	DL_HandleInit();

	request = DL_CreateRequest();

	Q_strncpyz(request->url, url, ARRAY_LEN(request->url));

	request->userData     = userData;
	request->complete_clb = complete;
	request->progress_clb = progress;
	request->uploadData   = upload;

	JNIEnv    *env   = androidSys.download_env;
	jmethodID method = (*env)->GetMethodID(env, androidSys.download_singleton, "createWebRequest", "(Lorg/etlegacy/app/web/Request;)V");

	jclass    uploadDataCls         = (*env)->FindClass(env, "org/etlegacy/app/web/UploadData");
	jmethodID uploadDataConstructor = (*env)->GetMethodID(env, uploadDataCls, "<init>", "()V");
	jobject   uploadData            = (*env)->NewObject(env, uploadDataCls, uploadDataConstructor);

	(*env)->SetObjectField(env, uploadData, (*env)->GetFieldID(env, uploadDataCls, "url", "Ljava/lang/String;"), (*env)->NewStringUTF(env, url));
	(*env)->SetObjectField(env, uploadData, (*env)->GetFieldID(env, uploadDataCls, "authToken", "Ljava/lang/String;"), (*env)->NewStringUTF(env, authToken));
	(*env)->SetIntField(env, uploadData, (*env)->GetFieldID(env, uploadDataCls, "requestType", "I"), upload ? 1 : 0);
	(*env)->SetLongField(env, uploadData, (*env)->GetFieldID(env, uploadDataCls, "nativeIdentifier", "J"), (jlong)request->id);

	if (upload)
	{
		if (upload->contentType[0])
		{
			jmethodID addHeaderMethod = (*env)->GetMethodID(env, uploadDataCls, "addHeader", "(Ljava/lang/String;Ljava/lang/String;)V");
			(*env)->CallVoidMethod(env, uploadData, addHeaderMethod, (*env)->NewStringUTF(env, "Content-Type"), (*env)->NewStringUTF(env, upload->contentType));
		}

		if (upload->bufferSize)
		{
			jbyteArray byteArray = (*env)->NewByteArray(env, upload->bufferSize);
			(*env)->SetByteArrayRegion(env, byteArray, 0, upload->bufferSize, (jbyte *) upload->buffer);
			(*env)->SetObjectField(env, uploadData, (*env)->GetFieldID(env, uploadDataCls, "buffer", "[B"), byteArray);
		}
	}

	(*env)->CallVoidMethod(env, androidSys.download_singleton, method, uploadData);

	return request->id;
}

void DL_DownloadLoop(void)
{
	// FIXME: implement
}

void DL_AbortAll(qboolean block, qboolean allowContinue)
{
	// FIXME: implement
}

void DL_Shutdown(void)
{
	// FIXME: implement
}
