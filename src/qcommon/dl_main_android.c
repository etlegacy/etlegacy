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

#include "dl_public.h"
#include "q_shared.h"
#include "../sdl/sdl_defs.h"
#include <jni.h>

#ifdef DEDICATED
#error "This file is only supported on client code!"
#endif

static struct {
    JNIEnv *download_env;
    jobject download_singleton;
    qboolean init;
} android_dl = { NULL, NULL, qfalse};

JNIEXPORT void JNICALL Java_org_etlegacy_app_ETLDownload_init(JNIEnv *env, jobject this)
{
    Com_Printf("Download system init from Android");
    android_dl.download_env = env;
    android_dl.download_singleton = this;
    android_dl.init = qtrue;
}

static void DL_HandleInit()
{
    if (android_dl.init)
    {
        return;
    }
    // If the java code has not done the init, then just call it ourselves
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    jclass cls = (*env)->FindClass(env, "org/etlegacy/app/ETLDownload");
    jmethodID id = (*env)->GetStaticMethodID(env, cls, "instance", "()Lorg/etlegacy/app/ETLDownload;");
    (*env)->CallStaticObjectMethod(env, cls, id);
}

unsigned int DL_BeginDownload(const char *localName, const char *remoteName, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
    DL_HandleInit();
    // FIXME: implement
    return 0;
}

unsigned int Web_CreateRequest(const char *url, const char *authToken, webUploadData_t *upload, void *userData, webCallbackFunc_t complete, webProgressCallbackFunc_t progress)
{
    DL_HandleInit();
    // FIXME: implement
    return 0;
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
