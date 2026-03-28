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
 * @file ui_i18n.c
 * @brief Mod-side localization entry points for ui.
 */

#include "ui_local.h"
#include "ui_i18n.h"
#include "../qcommon/i18n_dictionary.h"

static i18n_dictionary_t ui_i18nState;

static int UI_I18N_OpenFile(const char *path, fileHandle_t *fileHandle, fsMode_t mode)
{
	return trap_FS_FOpenFile(path, fileHandle, mode);
}

static void UI_I18N_ReadFile(void *buffer, int len, fileHandle_t fileHandle)
{
	trap_FS_Read(buffer, len, fileHandle);
}

static void UI_I18N_CloseFile(fileHandle_t fileHandle)
{
	trap_FS_FCloseFile(fileHandle);
}

static int UI_I18N_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return trap_FS_GetFileList(path, extension, listbuf, bufsize);
}

static void UI_I18N_Print(const char *string)
{
	trap_Print(string);
}

void UI_I18N_Init(void)
{
	i18n_imports_t imports;

	imports.openFile    = UI_I18N_OpenFile;
	imports.readFile    = UI_I18N_ReadFile;
	imports.closeFile   = UI_I18N_CloseFile;
	imports.getFileList = UI_I18N_GetFileList;
	imports.print       = UI_I18N_Print;

	I18N_DictionaryInit(&ui_i18nState, &imports, "locale/mod");
}

void UI_I18N_Update(void)
{
	char language[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("cl_lang", language, sizeof(language));
	I18N_DictionarySetLanguage(&ui_i18nState, language);
}

void UI_I18N_Shutdown(void)
{
	I18N_DictionaryShutdown(&ui_i18nState);
}

const char *UI_I18N_Translate(const char *string)
{
	return I18N_DictionaryTranslate(&ui_i18nState, string);
}
