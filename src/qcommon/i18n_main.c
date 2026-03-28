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
 * @file i18n_main.c
 * @brief Engine-side localization entry points backed by the shared tinygettext loader.
 */

#ifndef FEATURE_GETTEXT
#error This file should only be compiled if you want i18n support
#endif

#include "q_shared.h"
#include "qcommon.h"
#include "i18n_findlocale.h"
#include "i18n_dictionary.h"

#include <string.h>

static i18n_dictionary_t i18nClientDictionary;
static i18n_dictionary_t i18nModDictionary;
static cvar_t            *cl_lang      = NULL;
static cvar_t            *cl_langDebug = NULL;
static char              cl_lang_last[MAX_CVAR_VALUE_STRING];
static qboolean          i18nInitialized = qfalse;

static void I18N_TranslationMissing(const char *msgid);
static void I18N_CheckLanguage(void);

static int I18N_OpenFile(const char *path, fileHandle_t *fileHandle, fsMode_t mode)
{
	if (mode != FS_READ)
	{
		return -1;
	}

	return FS_FOpenFileRead(path, fileHandle, qfalse);
}

static void I18N_ReadFile(void *buffer, int len, fileHandle_t fileHandle)
{
	FS_Read(buffer, len, fileHandle);
}

static void I18N_CloseFile(fileHandle_t fileHandle)
{
	FS_FCloseFile(fileHandle);
}

static int I18N_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return FS_GetFileList(path, extension, listbuf, bufsize);
}

static void I18N_Print(const char *string)
{
	Com_Printf("%s", string);
}

void I18N_Init(void)
{
	FL_Locale      *locale;
	i18n_imports_t imports;

	if (i18nInitialized)
	{
		I18N_DictionaryShutdown(&i18nClientDictionary);
		I18N_DictionaryShutdown(&i18nModDictionary);
	}

	imports.openFile    = I18N_OpenFile;
	imports.readFile    = I18N_ReadFile;
	imports.closeFile   = I18N_CloseFile;
	imports.getFileList = I18N_GetFileList;
	imports.print       = I18N_Print;

	cl_lang      = Cvar_Get("cl_lang", "en", CVAR_ARCHIVE | CVAR_LATCH);
	cl_langDebug = Cvar_Get("cl_langDebug", "0", CVAR_ARCHIVE);

	I18N_DictionaryInit(&i18nClientDictionary, &imports, "locale/client");
	I18N_DictionaryInit(&i18nModDictionary, &imports, "locale/mod");
	I18N_DictionaryPrintAvailableLanguages(&imports, "locale/client", "Available client translations:");
	I18N_DictionaryPrintAvailableLanguages(&imports, "locale/mod", "Available mod translations:");

	FL_FindLocale(&locale);

	// Do not change the language if it is already set.
	if (!cl_lang->string[0])
	{
		if (locale->lang && locale->lang[0])
		{
			Cvar_Set("cl_lang", va("%s", locale->lang));
		}
		else
		{
			Cvar_Set("cl_lang", "en");
		}
	}

	Com_Memset(cl_lang_last, 0, sizeof(cl_lang_last));
	I18N_SetLanguage(cl_lang->string);
	FL_FreeLocale(&locale);

	i18nInitialized = qtrue;
}

void I18N_SetLanguage(const char *language)
{
	const char *resolvedLanguage;

	resolvedLanguage = (language && language[0]) ? language : "en";

	I18N_DictionarySetLanguage(&i18nClientDictionary, resolvedLanguage);
	I18N_DictionarySetLanguage(&i18nModDictionary, resolvedLanguage);

	Com_Printf("Language set to %s\n", resolvedLanguage);
	Q_strncpyz(cl_lang_last, resolvedLanguage, sizeof(cl_lang_last));
}

static const char *I18N_TranslateInternal(const char *msgid, const i18n_dictionary_t *dictionary)
{
	const char *translated;

	if (!i18nInitialized || !msgid)
	{
		return msgid;
	}

	I18N_CheckLanguage();

	translated = I18N_DictionaryTranslate(dictionary, msgid);
	if (cl_langDebug && cl_langDebug->integer && translated && !Q_stricmp(translated, msgid))
	{
		I18N_TranslationMissing(msgid);
	}

	return translated;
}

const char *I18N_Translate(const char *msgid)
{
	return I18N_TranslateInternal(msgid, &i18nClientDictionary);
}

const char *I18N_TranslateMod(const char *msgid)
{
	return I18N_TranslateInternal(msgid, &i18nModDictionary);
}

static void I18N_CheckLanguage(void)
{
	if (!cl_lang)
	{
		return;
	}

	if (Q_stricmp(cl_lang->string, cl_lang_last))
	{
		I18N_SetLanguage(cl_lang->string);
	}
}

static void I18N_TranslationMissing(const char *msgid)
{
	fileHandle_t fileHandle;

	FS_FOpenFileByMode(va("missing_translations_%s.txt", Cvar_VariableString("cl_lang")), &fileHandle, FS_APPEND);
	FS_Write(va("TRANSLATE(\"%s\");\n", msgid), strlen(msgid) + 15, fileHandle);
	FS_FCloseFile(fileHandle);
}
