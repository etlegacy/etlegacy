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
 * @file i18n_dictionary.h
 * @brief Shared tinygettext-backed loader for engine and client-side mod code.
 */

#ifndef INCLUDE_I18N_DICTIONARY_H
#define INCLUDE_I18N_DICTIONARY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "q_shared.h"

typedef int (*i18n_open_file_fn)(const char *path, fileHandle_t *fileHandle, fsMode_t mode);
typedef void (*i18n_read_file_fn)(void *buffer, int len, fileHandle_t fileHandle);
typedef void (*i18n_close_file_fn)(fileHandle_t fileHandle);
typedef int (*i18n_get_file_list_fn)(const char *path, const char *extension, char *listbuf, int bufsize);
typedef void (*i18n_print_fn)(const char *string);

typedef struct i18n_imports_s
{
	i18n_open_file_fn openFile;
	i18n_read_file_fn readFile;
	i18n_close_file_fn closeFile;
	i18n_get_file_list_fn getFileList;
	i18n_print_fn print;
} i18n_imports_t;

typedef struct i18n_dictionary_s
{
	i18n_imports_t imports;
	const char *directory;
	void *manager;
	void *cache;
	char loadedLanguage[MAX_CVAR_VALUE_STRING];
} i18n_dictionary_t;

void I18N_DictionaryInit(i18n_dictionary_t *dictionary, const i18n_imports_t *imports, const char *directory);
void I18N_DictionaryShutdown(i18n_dictionary_t *dictionary);
void I18N_DictionarySetLanguage(i18n_dictionary_t *dictionary, const char *language);
const char *I18N_DictionaryTranslate(const i18n_dictionary_t *dictionary, const char *msgid);
void I18N_DictionaryPrintAvailableLanguages(const i18n_imports_t *imports, const char *directory, const char *label);

#ifdef __cplusplus
}
#endif

#endif
