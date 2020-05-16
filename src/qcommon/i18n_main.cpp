/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * Daemon GPL Source Code
 * Copyright (C) 2012 Unvanquished Developers
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
 * @file i18n_main.cpp
 * @brief Glue for findlocale and tinygettext
 */

#ifndef FEATURE_GETTEXT
#error This file should only be compiled if you want i18n support
#endif

extern "C"
{
#include "q_shared.h"
#include "qcommon.h"
#include "i18n_findlocale.h"
}

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <map>

#include "../tinygettext/tinygettext/po_parser.hpp"
#include "../tinygettext/tinygettext/tinygettext.hpp"
#include "../tinygettext/tinygettext/log.hpp"
#include "../tinygettext/tinygettext/file_system.hpp"

tinygettext::DictionaryManager dictionary;
tinygettext::DictionaryManager dictionary_mod;

cvar_t      *cl_lang;
cvar_t      *cl_langDebug;
static char cl_lang_last[3];

qboolean doTranslate    = qfalse; // we don't translate english in general
qboolean doTranslateMod = qtrue; // only translate default mod only

std::map <std::string, std::string> strings; // original text / translated text

static void TranslationMissing(const char *msgid);
static void Tinygettext_Error(const std::string& str);
static void Tinygettext_Warning(const std::string& str);
static void Tinygettext_Info(const std::string& str);

/**
 * @brief std::streambuf based class that uses the engine's File I/O functions for input
 */
class QInputbuf : public std::streambuf
{
private:
	static const size_t BUFFER_SIZE = 8192;
	fileHandle_t        fileHandle;
	char                buffer[BUFFER_SIZE];
	size_t              putBack;
public:
	QInputbuf(const std::string& filename) : putBack(1)
	{
		char *end = buffer + BUFFER_SIZE - putBack;

		setg(end, end, end);

		if (FS_FOpenFileRead(filename.c_str(), &fileHandle, qfalse) <= 0)
		{
			Com_Printf("Warning: can't open or read file '%s' \n", filename.c_str());
		}
	}

	~QInputbuf()
	{
		if (fileHandle)
		{
			FS_FCloseFile(fileHandle);
		}
	}

	int underflow()
	{
		if (gptr() < egptr())  // buffer not exhausted
		{
			return traits_type::to_int_type(*gptr());
		}

		if (!fileHandle)
		{
			return traits_type::eof();
		}

		char *base  = buffer;
		char *start = base;

		if (eback() == base)
		{
			// Make arrangements for putback characters
			memmove(base, egptr() - putBack, putBack);
			start += putBack;
		}

		size_t n = FS_Read(start, BUFFER_SIZE - (start - base), fileHandle);

		if (n == 0)
		{
			return traits_type::eof();
		}

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
 * Uses the engine's File I/O functions for this purpose
 */
class QFileSystem : public tinygettext::FileSystem
{
public:
	QFileSystem()
	{
	}

	std::vector<std::string> open_directory(const std::string& pathname)
	{
		int                      numFiles;
		char                     **files;
		std::vector<std::string> ret;

		files = FS_ListFiles(pathname.c_str(), NULL, &numFiles);

		for (int i = 0; i < numFiles; i++)
		{
			ret.push_back(std::string(files[i]));
		}

		FS_FreeFileList(files);
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
 * @brief Attempts to detect the system language unless cl_lang was already set.
 * Then loads the PO file containing translated strings.
 */
void I18N_Init(void)
{
	FL_Locale                       *locale;
	std::set<tinygettext::Language> languages;
	std::set<tinygettext::Language> languages_mod;

	cl_lang      = Cvar_Get("cl_lang", "en", CVAR_ARCHIVE | CVAR_LATCH);
	cl_langDebug = Cvar_Get("cl_langDebug", "0", CVAR_ARCHIVE);

	tinygettext::Log::set_log_error_callback(&Tinygettext_Error);
	tinygettext::Log::set_log_info_callback(&Tinygettext_Info);
	tinygettext::Log::set_log_warning_callback(&Tinygettext_Warning);

	FL_FindLocale(&locale);

	// Do not change the language if it is already set
	if (!cl_lang->string[0])
	{
		// locale->country is also supported for 'en_US' format
		if (locale->lang && locale->lang[0])
		{
			Cvar_Set("cl_lang", va("%s", locale->lang));
		}
		else
		{
			// Language detection failed. Fallback to English
			Cvar_Set("cl_lang", "en");
		}
	}

#if __cplusplus >= 201103L // C++11
	dictionary.set_filesystem(std::unique_ptr<tinygettext::FileSystem>(new QFileSystem));
	dictionary_mod.set_filesystem(std::unique_ptr<tinygettext::FileSystem>(new QFileSystem));
#else
	dictionary.set_filesystem(std::auto_ptr<tinygettext::FileSystem>(new QFileSystem));
	dictionary_mod.set_filesystem(std::auto_ptr<tinygettext::FileSystem>(new QFileSystem));
#endif
	dictionary.add_directory("locale/client");
	dictionary_mod.add_directory("locale/mod");

	languages = dictionary.get_languages();
	Com_Printf("Available client translations:");
	for (std::set<tinygettext::Language>::iterator p = languages.begin(); p != languages.end(); p++)
	{
		Com_Printf(" %s", p->get_name().c_str());
	}
	Com_Printf("\n");

	languages_mod = dictionary_mod.get_languages();
	Com_Printf("Available mod translations:");
	for (std::set<tinygettext::Language>::iterator p = languages_mod.begin(); p != languages_mod.end(); p++)
	{
		Com_Printf(" %s", p->get_name().c_str());
	}
	Com_Printf("\n");

	I18N_SetLanguage(cl_lang->string);
	FL_FreeLocale(&locale);
}

/**
 * @brief Loads a localization file
 * @param[in] language
 */
void I18N_SetLanguage(const char *language)
{
	// TODO: check if there is a localization file available for the selected language
	dictionary.set_language(tinygettext::Language::from_env(std::string(language)));
	dictionary_mod.set_language(tinygettext::Language::from_env(std::string(language)));

	Com_Printf("Language set to %s\n", dictionary.get_language().get_name().c_str());
	Com_sprintf(cl_lang_last, sizeof(cl_lang_last), "%s", language);

	if (!Q_stricmp(cl_lang->string, "en"))
	{
		doTranslate = qfalse;
	}
	else
	{
		doTranslate = qtrue;
	}

	strings.clear();
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
 *
 * @return translated string or English text if dictionary was not found
 */
static const char *_I18N_Translate(const char *msgid, tinygettext::DictionaryManager &dict)
{
	if (Q_stricmp(cl_lang->string, cl_lang_last))
	{
		I18N_SetLanguage(cl_lang->string);
	}

	if (!doTranslate)
	{
		return msgid;
	}

	// Store translated string if it is not there yet
	if (strings.find(msgid) == strings.end())
	{
		strings.insert(std::make_pair(msgid, dict.get_dictionary().translate(msgid)));
	}

	if (cl_langDebug->integer)
	{
		if (!Q_stricmp(strings.find(msgid)->second.c_str(), msgid))
		{
			TranslationMissing(msgid);
		}
	}

	return strings.find(msgid)->second.c_str();
}

/**
 * @brief I18N_Translate
 * @param[in] msgid
 * @return
 */
const char *I18N_Translate(const char *msgid)
{
	return _I18N_Translate(msgid, dictionary);
}

/**
 * @brief I18N_TranslateMod
 * @param[in] msgid
 * @return
 */
const char *I18N_TranslateMod(const char *msgid)
{
	if (doTranslateMod)
	{
		return _I18N_Translate(msgid, dictionary_mod);
	}
	else
	{
		// we don't check for language change in this case - see *_I18N_Translate()
		// let *I18N_Translate() do the job
		return msgid;
	}
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

	FS_FOpenFileByMode(va("missing_translations_%s.txt", Cvar_VariableString("cl_lang")), &file, FS_APPEND);
	FS_Write(va("TRANSLATE(\"%s\");\n", msgid), strlen(msgid) + 15, file);

	FS_FCloseFile(file);
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
	if (cl_langDebug->integer)
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
	if (cl_langDebug->integer)
	{
		Com_Printf("%s", str.c_str());
	}
}
