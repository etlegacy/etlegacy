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
 * @file i18n_dictionary.cpp
 * @brief Shared tinygettext-backed loader for engine and client-side mod code.
 */

extern "C"
{
#include "i18n_dictionary.h"
}

#ifndef FEATURE_GETTEXT

void I18N_DictionaryInit(i18n_dictionary_t *dictionary, const i18n_imports_t *imports, const char *directory)
{
	if (!dictionary)
	{
		return;
	}

	Com_Memset(dictionary, 0, sizeof(*dictionary));

	if (imports)
	{
		dictionary->imports = *imports;
	}

	dictionary->directory = directory;
}

void I18N_DictionaryShutdown(i18n_dictionary_t *dictionary)
{
	if (!dictionary)
	{
		return;
	}

	dictionary->manager           = NULL;
	dictionary->cache             = NULL;
	dictionary->loadedLanguage[0] = '\0';
}

void I18N_DictionarySetLanguage(i18n_dictionary_t *dictionary, const char *language)
{
	if (!dictionary)
	{
		return;
	}

	Q_strncpyz(dictionary->loadedLanguage, (language && language[0]) ? language : "en", sizeof(dictionary->loadedLanguage));
}

const char *I18N_DictionaryTranslate(const i18n_dictionary_t *dictionary, const char *msgid)
{
	(void)dictionary;
	return msgid;
}

void I18N_DictionaryPrintAvailableLanguages(const i18n_imports_t *imports, const char *directory, const char *label)
{
	(void)imports;
	(void)directory;
	(void)label;
}

#else

#include "../../vendor/tinygettext/tinygettext/dictionary.hpp"
#include "../../vendor/tinygettext/tinygettext/dictionary_manager.hpp"
#include "../../vendor/tinygettext/tinygettext/file_system.hpp"
#include "../../vendor/tinygettext/tinygettext/language.hpp"
#include "../../vendor/tinygettext/tinygettext/log.hpp"

#include <ctype.h>
#include <exception>
#include <ios>
#include <istream>
#include <map>
#include <memory>
#include <set>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

namespace
{

/**
 * Translation catalogs are small, so a fixed listing buffer keeps the loader
 * simple without introducing another allocator on the hot path.
 */
static const int I18N_FILE_LIST_BUFFER_SIZE = 65536;

/**
 * The parser consumes the stream sequentially, so a small read buffer keeps
 * filesystem calls bounded while still avoiding whole-file copies.
 */
static const int I18N_STREAM_BUFFER_SIZE = 4096;

typedef std::map<std::string, std::string> i18n_translation_cache_t;

/**
 * tinygettext uses process-global log callbacks. Each module only drives the
 * loader from one side at a time, so a single active import table is enough.
 */
static const i18n_imports_t *i18nLogImports = NULL;

/**
 * @brief Suppresses tinygettext info logs.
 *
 * Missing translations are expected during development, and logging each miss
 * would spam the console during normal gameplay and menu rendering.
 */
static void I18N_TinygettextLogInfo(const std::string &message)
{
	(void)message;
}

/**
 * @brief Routes tinygettext warnings and errors through the caller's print hook.
 */
static void I18N_TinygettextLog(const std::string &message)
{
	std::string text;

	if (!i18nLogImports || !i18nLogImports->print || message.empty())
	{
		return;
	}

	text  = "tinygettext: ";
	text += message;
	i18nLogImports->print(text.c_str());
}

/**
 * @brief Installs the tinygettext log hooks for the current caller context.
 */
static void I18N_DictionaryConfigureLogging(const i18n_imports_t *imports)
{
	i18nLogImports = imports;
	tinygettext::Log::set_log_info_callback(I18N_TinygettextLogInfo);
	tinygettext::Log::set_log_warning_callback(I18N_TinygettextLog);
	tinygettext::Log::set_log_error_callback(I18N_TinygettextLog);
}

/**
 * @brief Prints a loader error through the caller-supplied print callback.
 */
static void I18N_DictionaryPrintError(const i18n_imports_t *imports, const char *context, const char *detail)
{
	std::string text;

	if (!imports || !imports->print || !context)
	{
		return;
	}

	text  = "tinygettext: ";
	text += context;

	if (detail && detail[0])
	{
		text += ": ";
		text += detail;
	}

	text += "\n";
	imports->print(text.c_str());
}

/**
 * @brief Normalizes user language strings into the format tinygettext expects.
 *
 * The project stores locale filenames as lower-case language codes with an
 * upper-case country suffix, so the loader mirrors that canonical form before
 * asking tinygettext to resolve matches and fallbacks.
 */
static std::string I18N_DictionaryNormalizeLanguage(const char *language)
{
	enum
	{
		I18N_LANGUAGE_PART,
		I18N_COUNTRY_PART,
		I18N_CODESET_PART,
		I18N_MODIFIER_PART
	} state;

	char   normalized[MAX_CVAR_VALUE_STRING];
	size_t i;

	Q_strncpyz(normalized, (language && language[0]) ? language : "en", sizeof(normalized));
	state = I18N_LANGUAGE_PART;

	for (i = 0; normalized[i]; i++)
	{
		unsigned char value;

		if (normalized[i] == '-')
		{
			normalized[i] = '_';
		}

		switch (normalized[i])
		{
		case '_':
			if (state == I18N_LANGUAGE_PART)
			{
				state = I18N_COUNTRY_PART;
			}
			continue;
		case '.':
			state = I18N_CODESET_PART;
			continue;
		case '@':
			state = I18N_MODIFIER_PART;
			continue;
		default:
			break;
		}

		value = (unsigned char)normalized[i];

		if (state == I18N_LANGUAGE_PART || state == I18N_MODIFIER_PART)
		{
			normalized[i] = (char)tolower(value);
		}
		else if (state == I18N_COUNTRY_PART)
		{
			normalized[i] = (char)toupper(value);
		}
	}

	return std::string(normalized);
}

/**
 * @brief Shared stream buffer that reads localization files through ET's FS API.
 */
class QFileStreambuf : public std::streambuf
{
public:
	QFileStreambuf(const i18n_imports_t *imports, const std::string &filename)
		: m_imports(imports)
		, m_fileHandle(0)
		, m_remaining(0)
		, m_isOpen(qfalse)
	{
		setg(m_buffer, m_buffer, m_buffer);

		if (!m_imports || !m_imports->openFile || !m_imports->readFile || !m_imports->closeFile)
		{
			return;
		}

		m_remaining = m_imports->openFile(filename.c_str(), &m_fileHandle, FS_READ);
		if (m_remaining < 0)
		{
			return;
		}

		m_isOpen = qtrue;
	}

	virtual ~QFileStreambuf()
	{
		if (m_isOpen)
		{
			m_imports->closeFile(m_fileHandle);
		}
	}

	qboolean is_open() const
	{
		return m_isOpen;
	}

protected:
	virtual int_type underflow()
	{
		int bytesToRead;

		if (!m_isOpen)
		{
			return traits_type::eof();
		}

		if (gptr() < egptr())
		{
			return traits_type::to_int_type(*gptr());
		}

		if (m_remaining <= 0)
		{
			return traits_type::eof();
		}

		bytesToRead = I18N_STREAM_BUFFER_SIZE;
		if (bytesToRead > m_remaining)
		{
			bytesToRead = m_remaining;
		}

		m_imports->readFile(m_buffer, bytesToRead, m_fileHandle);
		m_remaining -= bytesToRead;

		setg(m_buffer, m_buffer, m_buffer + bytesToRead);
		return traits_type::to_int_type(*gptr());
	}

private:
	const i18n_imports_t *m_imports;
	fileHandle_t m_fileHandle;
	int m_remaining;
	qboolean m_isOpen;
	char m_buffer[I18N_STREAM_BUFFER_SIZE];
};

/**
 * @brief Thin istream wrapper that lets tinygettext parse ET virtual files.
 */
class QFileIstream : public std::istream
{
public:
	QFileIstream(const i18n_imports_t *imports, const std::string &filename)
		: std::istream(NULL)
		, m_streambuf(imports, filename)
	{
		init(&m_streambuf);

		if (!m_streambuf.is_open())
		{
			setstate(std::ios::badbit);
		}
	}

	qboolean is_open() const
	{
		return m_streambuf.is_open();
	}

private:
	QFileStreambuf m_streambuf;
};

/**
 * @brief tinygettext filesystem adapter backed by engine and VM syscalls.
 */
class QFileSystem : public tinygettext::FileSystem
{
public:
	explicit QFileSystem(const i18n_imports_t *imports)
		: m_imports(imports)
	{
	}

	virtual std::vector<std::string> open_directory(const std::string &pathname)
	{
		char                     listBuffer[I18N_FILE_LIST_BUFFER_SIZE];
		const char               *cursor;
		const char               *listEnd;
		int                      numFiles;
		int                      i;
		std::vector<std::string> entries;

		if (!m_imports || !m_imports->getFileList)
		{
			return entries;
		}

		numFiles = m_imports->getFileList(pathname.c_str(), ".po", listBuffer, sizeof(listBuffer));
		if (numFiles <= 0)
		{
			return entries;
		}

		cursor  = listBuffer;
		listEnd = listBuffer + sizeof(listBuffer);

		for (i = 0; i < numFiles && cursor < listEnd && *cursor; i++)
		{
			size_t entryLength;

			entryLength = strlen(cursor);
			entries.push_back(std::string(cursor));
			cursor += entryLength + 1;
		}

		return entries;
	}

	virtual std::unique_ptr<std::istream> open_file(const std::string &filename)
	{
		QFileIstream *stream;

		stream = new QFileIstream(m_imports, filename);
		if (!stream->is_open())
		{
			delete stream;
			return std::unique_ptr<std::istream>();
		}

		return std::unique_ptr<std::istream>(stream);
	}

private:
	const i18n_imports_t *m_imports;
};

/**
 * @brief Creates the tinygettext manager that backs a shared dictionary state.
 */
static tinygettext::DictionaryManager *I18N_DictionaryCreateManager(const i18n_imports_t *imports, const char *directory)
{
	std::unique_ptr<tinygettext::DictionaryManager> manager;
	std::unique_ptr<tinygettext::FileSystem>        filesystem;

	manager.reset(new tinygettext::DictionaryManager("UTF-8"));

	I18N_DictionaryConfigureLogging(imports);

	filesystem.reset(new QFileSystem(imports));
	manager->set_filesystem(std::move(filesystem));

	if (directory && directory[0])
	{
		manager->add_directory(directory);
	}

	return manager.release();
}

/**
 * @brief Cast helper for the opaque dictionary manager pointer.
 */
static tinygettext::DictionaryManager *I18N_DictionaryManagerPtr(i18n_dictionary_t *dictionary)
{
	return static_cast<tinygettext::DictionaryManager *>(dictionary->manager);
}

/**
 * @brief Const cast helper for the opaque dictionary manager pointer.
 */
static const tinygettext::DictionaryManager *I18N_DictionaryManagerPtr(const i18n_dictionary_t *dictionary)
{
	return static_cast<const tinygettext::DictionaryManager *>(dictionary->manager);
}

/**
 * @brief Cast helper for the per-dictionary translation cache.
 */
static i18n_translation_cache_t *I18N_DictionaryCachePtr(i18n_dictionary_t *dictionary)
{
	return static_cast<i18n_translation_cache_t *>(dictionary->cache);
}

/**
 * @brief Const cast helper for the per-dictionary translation cache.
 */
static const i18n_translation_cache_t *I18N_DictionaryCachePtr(const i18n_dictionary_t *dictionary)
{
	return static_cast<const i18n_translation_cache_t *>(dictionary->cache);
}

/**
 * @brief Checks whether a filename ends with the given suffix.
 */
static qboolean I18N_DictionaryHasSuffix(const std::string &value, const char *suffix)
{
	size_t suffixLength;

	if (!suffix)
	{
		return qfalse;
	}

	suffixLength = strlen(suffix);
	if (value.length() < suffixLength)
	{
		return qfalse;
	}

	return value.compare(value.length() - suffixLength, suffixLength, suffix) == 0 ? qtrue : qfalse;
}

} // namespace

void I18N_DictionaryInit(i18n_dictionary_t *dictionary, const i18n_imports_t *imports, const char *directory)
{
	std::unique_ptr<tinygettext::DictionaryManager> manager;
	std::unique_ptr<i18n_translation_cache_t>       cache;

	if (!dictionary)
	{
		return;
	}

	Com_Memset(dictionary, 0, sizeof(*dictionary));

	if (imports)
	{
		dictionary->imports = *imports;
	}

	dictionary->directory = directory;

	try
	{
		manager.reset(I18N_DictionaryCreateManager(&dictionary->imports, directory));
		cache.reset(new i18n_translation_cache_t);
	}
	catch (const std::exception &exception)
	{
		I18N_DictionaryPrintError(&dictionary->imports, "failed to initialize shared loader", exception.what());
		return;
	}
	catch (...)
	{
		I18N_DictionaryPrintError(&dictionary->imports, "failed to initialize shared loader", "unknown error");
		return;
	}

	dictionary->manager = manager.release();
	dictionary->cache   = cache.release();
}

void I18N_DictionaryShutdown(i18n_dictionary_t *dictionary)
{
	if (!dictionary)
	{
		return;
	}

	delete I18N_DictionaryManagerPtr(dictionary);
	delete I18N_DictionaryCachePtr(dictionary);

	dictionary->manager           = NULL;
	dictionary->cache             = NULL;
	dictionary->loadedLanguage[0] = '\0';
}

void I18N_DictionarySetLanguage(i18n_dictionary_t *dictionary, const char *language)
{
	i18n_translation_cache_t       *cache;
	tinygettext::DictionaryManager *manager;
	std::string                    normalizedLanguage;

	if (!dictionary)
	{
		return;
	}

	manager = I18N_DictionaryManagerPtr(dictionary);
	cache   = I18N_DictionaryCachePtr(dictionary);

	if (!manager || !cache)
	{
		return;
	}

	normalizedLanguage = I18N_DictionaryNormalizeLanguage(language);
	if (!Q_stricmp(dictionary->loadedLanguage, normalizedLanguage.c_str()))
	{
		return;
	}

	I18N_DictionaryConfigureLogging(&dictionary->imports);
	manager->set_language(tinygettext::Language::from_env(normalizedLanguage));

	cache->clear();
	Q_strncpyz(dictionary->loadedLanguage, normalizedLanguage.c_str(), sizeof(dictionary->loadedLanguage));
}

const char *I18N_DictionaryTranslate(const i18n_dictionary_t *dictionary, const char *msgid)
{
	i18n_translation_cache_t           *cache;
	tinygettext::DictionaryManager     *manager;
	i18n_translation_cache_t::iterator it;
	std::string                        translated;

	if (!dictionary || !msgid)
	{
		return msgid;
	}

	manager = const_cast<tinygettext::DictionaryManager *>(I18N_DictionaryManagerPtr(dictionary));
	cache   = const_cast<i18n_translation_cache_t *>(I18N_DictionaryCachePtr(dictionary));

	if (!manager || !cache)
	{
		return msgid;
	}

	it = cache->find(msgid);
	if (it != cache->end())
	{
		return it->second.c_str();
	}

	try
	{
		translated = manager->get_dictionary().translate(msgid);
	}
	catch (const std::exception &exception)
	{
		I18N_DictionaryPrintError(&dictionary->imports, "failed to translate string", exception.what());
		return msgid;
	}
	catch (...)
	{
		I18N_DictionaryPrintError(&dictionary->imports, "failed to translate string", "unknown error");
		return msgid;
	}

	it = cache->insert(std::make_pair(std::string(msgid), translated)).first;
	return it->second.c_str();
}

void I18N_DictionaryPrintAvailableLanguages(const i18n_imports_t *imports, const char *directory, const char *label)
{
	QFileSystem                               filesystem(imports);
	tinygettext::DictionaryManager            manager("UTF-8");
	std::set<tinygettext::Language>           languages;
	std::set<tinygettext::Language>::iterator language;
	std::vector<std::string>                  files;
	std::string                               line;
	std::vector<std::string>::iterator        file;

	if (!imports || !imports->print || !directory || !label)
	{
		return;
	}

	I18N_DictionaryConfigureLogging(imports);

	files = filesystem.open_directory(directory);
	for (file = files.begin(); file != files.end(); ++file)
	{
		tinygettext::Language resolvedLanguage;

		if (!I18N_DictionaryHasSuffix(*file, ".po"))
		{
			continue;
		}

		resolvedLanguage = tinygettext::Language::from_env(manager.convertFilename2Language(*file));
		if (resolvedLanguage)
		{
			languages.insert(resolvedLanguage);
		}
	}

	line = label;
	for (language = languages.begin(); language != languages.end(); ++language)
	{
		line += " ";
		line += language->get_name();
	}
	line += "\n";

	imports->print(line.c_str());
}

#endif
