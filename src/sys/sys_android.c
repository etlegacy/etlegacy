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
 * @file sys_android.c
 * @brief
 */

#include "sys_android.h"

void Com_Android_HTTPS(const char *localName, const char *remoteName)
{
	clientJavainterface_t cjv;
	static char           *str;

	cjv.env = (JNIEnv *) SDL_AndroidGetJNIEnv();

	if (cjv.env == NULL)
	{
		return;
	}

	cjv.activity = (jobject)SDL_AndroidGetActivity();

	if (cjv.activity == NULL)
	{
		return;
	}

	cjv.clazz = (*cjv.env)->GetObjectClass(cjv.env, cjv.activity);

	if (cjv.clazz == NULL)
	{
		return;
	}

	// NOTES: Is fullpath etl_url_local == etl_url_remote ?
	cjv.f_id = (*cjv.env)->GetFieldID(cjv.env, cjv.clazz, "etl_url_local", "Ljava/lang/String;");
	if (cjv.f_id == NULL)
	{
		return;
	}

	cjv.jstr = (*cjv.env)->GetObjectField(cjv.env, cjv.activity, cjv.f_id);
	str      = (*cjv.env)->GetStringUTFChars(cjv.env, cjv.jstr, 0);
	(*cjv.env)->ReleaseStringUTFChars(cjv.env, cjv.jstr, str);

	cjv.jstr = (*cjv.env)->NewStringUTF(cjv.env, localName);
	(*cjv.env)->SetObjectField(cjv.env, cjv.activity, cjv.f_id, cjv.jstr);

	cjv.f_id = (*cjv.env)->GetFieldID(cjv.env, cjv.clazz, "etl_url_remote", "Ljava/lang/String;");
	if (cjv.f_id == NULL)
	{
		return;
	}

	cjv.jstr = (*cjv.env)->GetObjectField(cjv.env, cjv.activity, cjv.f_id);
	str      = (*cjv.env)->GetStringUTFChars(cjv.env, cjv.jstr, 0);
	(*cjv.env)->ReleaseStringUTFChars(cjv.env, cjv.jstr, str);

	cjv.jstr = (*cjv.env)->NewStringUTF(cjv.env, remoteName);
	(*cjv.env)->SetObjectField(cjv.env, cjv.activity, cjv.f_id, cjv.jstr);

	(*cjv.env)->DeleteLocalRef(cjv.env, cjv.clazz);
	(*cjv.env)->DeleteLocalRef(cjv.env, cjv.activity);
}

int Com_Get_Android_Bandwidh()
{
	return 0;
}

int Com_Get_Android_Bytes()
{
	return 0;
}
