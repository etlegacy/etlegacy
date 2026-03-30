/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * Daemon GPL Source Code
 * Copyright (C) 2012 Unvanquished Developers
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
 * @file i18n_main.cpp
 * @brief Glue for findlocale and tinygettext
 */

#ifndef FEATURE_GETTEXT
#error This file should only be compiled if you want i18n support
#endif

#ifdef _MSC_VER
/* Keep MSVC's math constants in sync before q_math.h provides its fallback values. */
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#endif

extern "C"
{
#ifndef MODLIB
#include "qcommon.h"
#else
#if defined(CGAMEDLL)
#include "q_shared.h"
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void trap_Cvar_Update(vmCvar_t *vmCvar);
void trap_Cvar_Set(const char *varName, const char *value);
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, int len, fileHandle_t f);
int trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
#elif defined(UIDLL)
#include "q_shared.h"
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void trap_Cvar_Update(vmCvar_t *vmCvar);
void trap_Cvar_Set(const char *varName, const char *value);
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, int len, fileHandle_t f);
void trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
#else
#error Unsupported mod localization target
#endif
#endif
#include "i18n_findlocale.h"
}

#include <map>
#include <memory>
#include <set>
#include <stdlib.h>
#include <string>
#include <string.h>

#include "../../vendor/tinygettext/tinygettext/file_system.hpp"
#include "../../vendor/tinygettext/tinygettext/log.hpp"
#include "../../vendor/tinygettext/tinygettext/po_parser.hpp"
#include "../../vendor/tinygettext/tinygettext/tinygettext.hpp"

tinygettext::DictionaryManager dictionary;
tinygettext::DictionaryManager dictionary_mod;

#ifndef MODLIB
static cvar_t *cl_lang      = NULL;
static cvar_t *cl_langDebug = NULL;
#else
static vmCvar_t cl_lang;
static vmCvar_t cl_langDebug;
#endif

static char cl_lang_last[MAX_CVAR_VALUE_STRING];

static qboolean doTranslate         = qfalse; // we don't translate english in general
static qboolean i18nCvarsRegistered = qfalse;

static std::map<std::string, std::string> clientStrings;
static std::map<std::string, std::string> modStrings;

static void I18N_RegisterCvars(void);
extern "C" void I18N_SetLanguage(const char *language);
static const char *I18N_CurrentLanguage(void);
static int I18N_DebugEnabled(void);
static void I18N_SetLanguageCvar(const char *language);
static int I18N_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
static int I18N_FOpenFile(const char *qpath, fileHandle_t *file, fsMode_t mode);
static void I18N_ReadFile(void *buffer, int len, fileHandle_t file);
static void I18N_WriteFile(const void *buffer, int len, fileHandle_t file);
static void I18N_CloseFile(fileHandle_t file);
static void I18N_ResetDirectory(tinygettext::DictionaryManager &dict, const char *path);
static void I18N_PrintAvailableLanguages(const char *label, tinygettext::DictionaryManager &dict);
static void TranslationMissing(const char *msgid);
static const char *_I18N_Translate(const char *msgid, tinygettext::DictionaryManager &dict, std::map<std::string, std::string> &cache);
static void Tinygettext_Error(const std::string& str);
static void Tinygettext_Warning(const std::string& str);
static void Tinygettext_Info(const std::string& str);

#ifndef MODLIB
static void I18N_RegisterCvars(void)
{
	cl_lang             = Cvar_Get("cl_lang", "en", CVAR_ARCHIVE | CVAR_LATCH);
	cl_langDebug        = Cvar_Get("cl_langDebug", "0", CVAR_ARCHIVE);
	i18nCvarsRegistered = qtrue;
}

static const char *I18N_CurrentLanguage(void)
{
	return cl_lang ? cl_lang->string : "";
}

static int I18N_DebugEnabled(void)
{
	return cl_langDebug ? cl_langDebug->integer : 0;
}

static void I18N_SetLanguageCvar(const char *language)
{
	Cvar_Set("cl_lang", language);
}

static int I18N_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return FS_GetFileList(path, extension, listbuf, bufsize);
}

static int I18N_FOpenFile(const char *qpath, fileHandle_t *file, fsMode_t mode)
{
	return FS_FOpenFileByMode(qpath, file, mode);
}

static void I18N_ReadFile(void *buffer, int len, fileHandle_t file)
{
	FS_Read(buffer, len, file);
}

static void I18N_WriteFile(const void *buffer, int len, fileHandle_t file)
{
	FS_Write(buffer, len, file);
}

static void I18N_CloseFile(fileHandle_t file)
{
	FS_FCloseFile(file);
}
#else
/**
 * @brief Client-side mods only rely on base cvar/fs traps so localization stays 2.60b-safe.
 */
static void I18N_RegisterCvars(void)
{
	trap_Cvar_Register(&cl_lang, "cl_lang", "en", CVAR_ARCHIVE | CVAR_LATCH);
	trap_Cvar_Register(&cl_langDebug, "cl_langDebug", "0", CVAR_ARCHIVE);
	i18nCvarsRegistered = qtrue;
}

static const char *I18N_CurrentLanguage(void)
{
	trap_Cvar_Update(&cl_lang);
	return cl_lang.string;
}

static int I18N_DebugEnabled(void)
{
	trap_Cvar_Update(&cl_langDebug);
	return cl_langDebug.integer;
}

static void I18N_SetLanguageCvar(const char *language)
{
	trap_Cvar_Set("cl_lang", language);
	trap_Cvar_Update(&cl_lang);
}

static int I18N_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
	return trap_FS_GetFileList(path, extension, listbuf, bufsize);
}

static int I18N_FOpenFile(const char *qpath, fileHandle_t *file, fsMode_t mode)
{
	return trap_FS_FOpenFile(qpath, file, mode);
}

static void I18N_ReadFile(void *buffer, int len, fileHandle_t file)
{
	trap_FS_Read(buffer, len, file);
}

static void I18N_WriteFile(const void *buffer, int len, fileHandle_t file)
{
	trap_FS_Write(buffer, len, file);
}

static void I18N_CloseFile(fileHandle_t file)
{
	trap_FS_FCloseFile(file);
}
#endif

/**
 * @brief std::streambuf based class that uses the active engine or mod file API for input.
 */
class QInputbuf : public std::streambuf
{
private:
	static const size_t BUFFER_SIZE = 8192;
	fileHandle_t fileHandle;
	int remainingBytes;
	char buffer[BUFFER_SIZE];
	size_t putBack;
public:
	QInputbuf(const std::string& filename) : fileHandle(0), remainingBytes(0), putBack(1)
	{
		char *end = buffer + BUFFER_SIZE - putBack;

		setg(end, end, end);

		remainingBytes = I18N_FOpenFile(filename.c_str(), &fileHandle, FS_READ);
		if (remainingBytes <= 0)
		{
			fileHandle     = 0;
			remainingBytes = 0;
			Com_Printf("Warning: can't open or read file '%s' \n", filename.c_str());
		}
	}

	~QInputbuf()
	{
		if (fileHandle)
		{
			I18N_CloseFile(fileHandle);
		}
	}

	int underflow()
	{
		int  n;
		char *base;
		char *start;

		if (gptr() < egptr())  // buffer not exhausted
		{
			return traits_type::to_int_type(*gptr());
		}

		if (!fileHandle)
		{
			return traits_type::eof();
		}

		base  = buffer;
		start = base;

		if (eback() == base)
		{
			// Make arrangements for putback characters
			memmove(base, egptr() - putBack, putBack);
			start += putBack;
		}

		n = (int)(BUFFER_SIZE - (start - base));
		if (remainingBytes < n)
		{
			n = remainingBytes;
		}

		if (n <= 0)
		{
			return traits_type::eof();
		}

		I18N_ReadFile(start, n, fileHandle);
		remainingBytes -= n;

		// Set buffer pointers
		setg(base, start, start + n);

		return traits_type::to_int_type(*gptr());
	}
};

/**
 * @brief Simple istream based class that takes ownership of the streambuf
 */
class QIstream : public std::istream
{
public:
	QIstream(const std::string& filename) : std::istream(new QInputbuf(filename))
	{
	}
	~QIstream()
	{
		delete rdbuf();
	}
};

/**
 * @brief Class used by tinygettext to read files and directories.
 * Uses the shared qcommon file API wrappers for this purpose.
 */
class QFileSystem : public tinygettext::FileSystem
{
public:
	std::vector<std::string> open_directory(const std::string& pathname)
	{
		static const int         FILE_LIST_SIZE = 32768;
		char                     fileList[FILE_LIST_SIZE];
		char                     *filePtr;
		int                      numFiles;
		std::vector<std::string> ret;

		numFiles = I18N_GetFileList(pathname.c_str(), ".po", fileList, sizeof(fileList));
		filePtr  = fileList;

		for (int i = 0; i < numFiles && filePtr[0]; ++i)
		{
			ret.push_back(std::string(filePtr));
			filePtr += strlen(filePtr) + 1;
		}

		return ret;
	}

#if __cplusplus >= 201103L // C++11
	std::unique_ptr<std::istream> open_file(const std::string& filename)
	{
		return std::unique_ptr<std::istream>(new QIstream(filename));
	}
#else
	std::auto_ptr<std::istream> open_file(const std::string& filename)
	{
		return std::auto_ptr<std::istream>(new QIstream(filename));
	}
#endif
};

/**
 * @brief Re-adds a search path to avoid duplicated directory entries on subsystem restarts.
 */
static void I18N_ResetDirectory(tinygettext::DictionaryManager &dict, const char *path)
{
	dict.remove_directory(path);
	dict.add_directory(path);
}

/**
 * @brief Prints the languages available for a specific translation domain.
 */
static void I18N_PrintAvailableLanguages(const char *label, tinygettext::DictionaryManager &dict)
{
	std::set<tinygettext::Language> languages = dict.get_languages();

	Com_Printf("%s:", label);
	for (std::set<tinygettext::Language>::iterator language = languages.begin(); language != languages.end(); ++language)
	{
		Com_Printf(" %s", language->get_name().c_str());
	}
	Com_Printf("\n");
}

/**
 * @brief Attempts to detect the system language unless cl_lang was already set.
 * Then loads the PO file containing translated strings.
 */
extern "C" void I18N_Init(void)
{
	FL_Locale  *locale;
	const char *language;

	I18N_RegisterCvars();

	tinygettext::Log::set_log_error_callback(&Tinygettext_Error);
	tinygettext::Log::set_log_info_callback(&Tinygettext_Info);
	tinygettext::Log::set_log_warning_callback(&Tinygettext_Warning);

	language = I18N_CurrentLanguage();

	// Do not change the language if it is already set
	if (!language[0])
	{
		locale = NULL;
		FL_FindLocale(&locale);

		// locale->country is also supported for 'en_US' format
		if (locale && locale->lang && locale->lang[0])
		{
			I18N_SetLanguageCvar(locale->lang);
		}
		else
		{
			// Language detection failed. Fallback to English
			I18N_SetLanguageCvar("en");
		}

		FL_FreeLocale(&locale);
		language = I18N_CurrentLanguage();
	}

#if __cplusplus >= 201103L // C++11
	dictionary.set_filesystem(std::unique_ptr<tinygettext::FileSystem>(new QFileSystem));
	dictionary_mod.set_filesystem(std::unique_ptr<tinygettext::FileSystem>(new QFileSystem));
#else
	dictionary.set_filesystem(std::auto_ptr<tinygettext::FileSystem>(new QFileSystem));
	dictionary_mod.set_filesystem(std::auto_ptr<tinygettext::FileSystem>(new QFileSystem));
#endif
	I18N_ResetDirectory(dictionary, "locale/client");
	I18N_ResetDirectory(dictionary_mod, "locale/mod");

	I18N_PrintAvailableLanguages("Available client translations", dictionary);
	I18N_PrintAvailableLanguages("Available mod translations", dictionary_mod);

	I18N_SetLanguage(language);
}

/**
 * @brief Loads a localization file
 * @param[in] language
 */
extern "C" void I18N_SetLanguage(const char *language)
{
	const char *resolvedLanguage = (language && language[0]) ? language : "en";

	// TODO: check if there is a localization file available for the selected language
	dictionary.set_language(tinygettext::Language::from_env(std::string(resolvedLanguage)));
	dictionary_mod.set_language(tinygettext::Language::from_env(std::string(resolvedLanguage)));

	Com_Printf("Language set to %s\n", dictionary.get_language().get_name().c_str());
	Com_sprintf(cl_lang_last, sizeof(cl_lang_last), "%s", resolvedLanguage);

	doTranslate = qtrue;

	clientStrings.clear();
	modStrings.clear();
}

/**
 * @brief Translates a string using the specified dictionary
 *
 * Localized strings are stored in a map container as tinygettext would
 * attempt to read them from the po file at each call and would endlessly
 * spam the console with warnings if the requested translation did not exist.
 *
 * @param[in] msgid original string in English
 * @param[in] dict dictionary to use (client / mod)
 * @param[in,out] cache cached translations for the selected domain
 *
 * @return translated string or English text if dictionary was not found
 */
static const char *_I18N_Translate(const char *msgid, tinygettext::DictionaryManager &dict, std::map<std::string, std::string> &cache)
{
	const char                                   *language;
	std::map<std::string, std::string>::iterator translated;

	if (!i18nCvarsRegistered)
	{
		Com_Printf("Calling translation before I18N is initialized\n");
		return msgid;
	}

	language = I18N_CurrentLanguage();
	if (Q_stricmp(language, cl_lang_last))
	{
		I18N_SetLanguage(language);
	}

	if (!doTranslate)
	{
		return msgid;
	}

	// Store translated string if it is not there yet
	translated = cache.find(msgid);
	if (translated == cache.end())
	{
		cache.insert(std::make_pair(msgid, dict.get_dictionary().translate(msgid)));
		translated = cache.find(msgid);
	}

	if (I18N_DebugEnabled() && !Q_stricmp(translated->second.c_str(), msgid))
	{
		TranslationMissing(msgid);
	}

	return translated->second.c_str();
}

/**
 * @brief I18N_Translate
 * @param[in] msgid
 * @return
 */
extern "C" const char *I18N_Translate(const char *msgid)
{
	return _I18N_Translate(msgid, dictionary, clientStrings);
}

/**
 * @brief I18N_TranslateMod
 * @param[in] msgid
 * @return
 */
extern "C" const char *I18N_TranslateMod(const char *msgid)
{
	return _I18N_Translate(msgid, dictionary_mod, modStrings);
}

/**
 * @brief A dumb function which saves missing strings for the current language and mod
 * passed to it.
 *
 * @param[in] msgid original text
 */
static void TranslationMissing(const char *msgid)
{
	fileHandle_t file;
	const char   *language;
	const char   *line;

	language = I18N_CurrentLanguage();
	line     = va("TRANSLATE(\"%s\");\n", msgid);

	if (I18N_FOpenFile(va("missing_translations_%s.txt", language[0] ? language : "en"), &file, FS_APPEND) >= 0 && file)
	{
		I18N_WriteFile(line, (int)strlen(line), file);
		I18N_CloseFile(file);
	}
}

/**
 * Logging functions which override the default ones from Tinygettext
 */

/**
 * @brief Tinygettext_Error
 * @param[in] str
 */
static void Tinygettext_Error(const std::string& str)
{
	Com_Printf("^1%s^7", str.c_str());
}

/**
 * @brief Tinygettext_Warning
 * @param[in] str
 */
static void Tinygettext_Warning(const std::string& str)
{
	if (I18N_DebugEnabled())
	{
		Com_Printf("^3%s^7", str.c_str());
	}
}

/**
 * @brief Tinygettext_Info
 * @param[in] str
 */
static void Tinygettext_Info(const std::string& str)
{
	if (I18N_DebugEnabled())
	{
		Com_Printf("%s", str.c_str());
	}
}
