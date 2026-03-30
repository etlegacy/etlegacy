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
 * @file i18n.c
 * @brief Shared ANSI C localization support for engine and client VMs.
 */

#ifndef FEATURE_GETTEXT
#error This file should only be compiled if you want i18n support
#endif

#if MODLIB
#if CGAMEDLL
#include "../cgame/cg_local.h"
#elif UIDLL
#include "../ui/ui_local.h"
#else
#error Unsupported MODLIB target for i18n.c
#endif
#else
#include "qcommon.h"
#include "i18n_findlocale.h"
#endif

#define I18N_MAX_CATALOG_BYTES  524288
#define I18N_MAX_ENTRIES        4096
#define I18N_HASH_SIZE          8192

typedef struct
{
	const char *msgid;
	const char *msgstr;
} i18n_entry_t;

typedef struct
{
	qboolean attempted;
	qboolean loaded;
	char language[MAX_CVAR_VALUE_STRING];
	char fileBuffer[I18N_MAX_CATALOG_BYTES];
	char buffer[I18N_MAX_CATALOG_BYTES];
	i18n_entry_t entries[I18N_MAX_ENTRIES];
	unsigned short hashSlots[I18N_HASH_SIZE];
	int entryCount;
} i18n_catalog_t;

#if MODLIB
static i18n_catalog_t i18nModCatalog;
static i18n_catalog_t i18nModEnglishCatalog;
static qboolean       i18nInitialized      = qfalse;
static qboolean       i18nTranslateMod     = qtrue;
static qboolean       i18nTranslateModLast = qtrue;
#else
typedef enum
{
	I18N_DOMAIN_CLIENT,
	I18N_DOMAIN_MOD,
	I18N_DOMAIN_COUNT
} i18n_domain_t;

static i18n_catalog_t i18nCatalogs[I18N_DOMAIN_COUNT];
static i18n_catalog_t i18nEnglishCatalogs[I18N_DOMAIN_COUNT];
static qboolean       i18nInitialized = qfalse;
static cvar_t         *cl_lang        = NULL;
static cvar_t         *cl_langDebug   = NULL;

qboolean doTranslateMod = qtrue;
#endif

static char i18nLanguage[MAX_CVAR_VALUE_STRING];

/**
 * @brief I18N_GetEffectiveLanguage
 * @param[out] language
 * @param[in] languageSize
 */
static void I18N_GetEffectiveLanguage(char *language, size_t languageSize)
{
#if MODLIB
	// Language is latched in the engine, so VMs must read the pending value.
	trap_Cvar_LatchedVariableStringBuffer("cl_lang", language, languageSize);
#else
	// Engine-side lookups must honor the pending profile language as well.
	Cvar_LatchedVariableStringBuffer("cl_lang", language, languageSize);
#endif
}

/**
 * @brief I18N_LanguageIsEnglish
 * @param[in] language
 * @return qtrue if the language resolves to English
 */
static qboolean I18N_LanguageIsEnglish(const char *language)
{
	if (!language || !language[0])
	{
		return qtrue;
	}

	if (language[0] != 'e' || language[1] != 'n')
	{
		return qfalse;
	}

	return language[2] == '\0' || language[2] == '_' || language[2] == '-' || language[2] == '.';
}

/**
 * @brief I18N_ClearCatalog
 * @param[in,out] catalog
 */
static void I18N_ClearCatalog(i18n_catalog_t *catalog)
{
	Com_Memset(catalog, 0, sizeof(*catalog));
}

/**
 * @brief I18N_AppendQuoted
 * @param[in] line
 * @param[in,out] stringStart
 * @param[in,out] storage
 * @param[in] storageEnd
 * @return qtrue if the quoted fragment was appended
 */
static qboolean I18N_AppendQuoted(char *line, char **stringStart, char **storage, char *storageEnd)
{
	char *input;
	char *output;

	while (*line == ' ' || *line == '\t')
	{
		line++;
	}

	if (*line != '\"')
	{
		return qfalse;
	}

	input = line + 1;

	if (*stringStart)
	{
		output = *storage - 1;
	}
	else
	{
		output       = *storage;
		*stringStart = output;
	}

	while (*input && *input != '\"')
	{
		if (output >= storageEnd)
		{
			return qfalse;
		}

		if (*input == '\\')
		{
			input++;

			if (!*input)
			{
				break;
			}

			switch (*input)
			{
			case 'n':
				*output++ = '\n';
				input++;
				break;
			case 'r':
				*output++ = '\r';
				input++;
				break;
			case 't':
				*output++ = '\t';
				input++;
				break;
			case '\\':
				*output++ = '\\';
				input++;
				break;
			case '\"':
				*output++ = '\"';
				input++;
				break;
			default:
				*output++ = *input++;
				break;
			}

			continue;
		}

		*output++ = *input++;
	}

	if (output >= storageEnd)
	{
		return qfalse;
	}

	*output++ = '\0';
	*storage  = output;

	return qtrue;
}

/**
 * @brief I18N_InsertEntry
 * @param[in,out] catalog
 * @param[in] msgid
 * @param[in] msgstr
 * @return qtrue if the entry was stored or intentionally skipped
 */
static qboolean I18N_InsertEntry(i18n_catalog_t *catalog, const char *msgid, const char *msgstr)
{
	int slot;
	int startSlot;
	int entryIndex;

	if (!msgid || !msgstr || !msgid[0] || !msgstr[0])
	{
		return qtrue;
	}

	slot      = Q_GenerateHashValue(msgid, I18N_HASH_SIZE, qfalse, qfalse);
	startSlot = slot;

	while (catalog->hashSlots[slot])
	{
		entryIndex = catalog->hashSlots[slot] - 1;

		if (!strcmp(catalog->entries[entryIndex].msgid, msgid))
		{
			catalog->entries[entryIndex].msgstr = msgstr;
			return qtrue;
		}

		slot = (slot + 1) % I18N_HASH_SIZE;

		if (slot == startSlot)
		{
			return qfalse;
		}
	}

	if (catalog->entryCount >= I18N_MAX_ENTRIES)
	{
		return qfalse;
	}

	entryIndex                          = catalog->entryCount++;
	catalog->entries[entryIndex].msgid  = msgid;
	catalog->entries[entryIndex].msgstr = msgstr;
	catalog->hashSlots[slot]            = (unsigned short)(entryIndex + 1);

	return qtrue;
}

/**
 * @brief I18N_FindTranslation
 * @param[in] catalog
 * @param[in] msgid
 * @return translated string or NULL if no match was found
 */
static const char *I18N_FindTranslation(const i18n_catalog_t *catalog, const char *msgid)
{
	int slot;
	int startSlot;
	int entryIndex;

	if (!catalog->loaded || !msgid || !msgid[0])
	{
		return NULL;
	}

	slot      = Q_GenerateHashValue(msgid, I18N_HASH_SIZE, qfalse, qfalse);
	startSlot = slot;

	while (catalog->hashSlots[slot])
	{
		entryIndex = catalog->hashSlots[slot] - 1;

		if (!strcmp(catalog->entries[entryIndex].msgid, msgid))
		{
			return catalog->entries[entryIndex].msgstr;
		}

		slot = (slot + 1) % I18N_HASH_SIZE;

		if (slot == startSlot)
		{
			break;
		}
	}

	return NULL;
}

/**
 * @brief I18N_RecordMissing
 * @param[in] msgid
 */
static void I18N_RecordMissing(const char *msgid)
{
	char         line[MAX_STRING_CHARS];
	fileHandle_t file;

	if (I18N_LanguageIsEnglish(i18nLanguage))
	{
		return;
	}

#if MODLIB
	{
		char debugValue[MAX_CVAR_VALUE_STRING];

		trap_Cvar_VariableStringBuffer("cl_langDebug", debugValue, sizeof(debugValue));
		if (!Q_atoi(debugValue))
		{
			return;
		}

		if (trap_FS_FOpenFile(va("missing_translations_%s.txt", i18nLanguage), &file, FS_APPEND) < 0)
		{
			return;
		}

		Com_sprintf(line, sizeof(line), "TRANSLATE(\"%s\");\n", msgid);
		trap_FS_Write(line, strlen(line), file);
		trap_FS_FCloseFile(file);
	}
#else
	if (!cl_langDebug || !cl_langDebug->integer)
	{
		return;
	}

	if (FS_FOpenFileByMode(va("missing_translations_%s.txt", i18nLanguage), &file, FS_APPEND) < 0)
	{
		return;
	}

	Com_sprintf(line, sizeof(line), "TRANSLATE(\"%s\");\n", msgid);
	FS_Write(line, strlen(line), file);
	FS_FCloseFile(file);
#endif
}

/**
 * @brief I18N_LoadCatalog
 * @param[in,out] catalog
 * @param[in] basePath
 * @param[in] language
 * @return qtrue if the catalog is ready for lookups
 */
static qboolean I18N_LoadCatalog(i18n_catalog_t *catalog, const char *basePath, const char *language)
{
	fileHandle_t file = 0;
	char         pathname[MAX_QPATH];
	char         fallbackLanguage[MAX_CVAR_VALUE_STRING];
	char         *source;
	char         *storage;
	char         *storageEnd;
	char         *line;
	char         *currentMsgid;
	char         *currentMsgstr;
	int          length;
	int          i;
	qboolean     useFallback;
	qboolean     exactFileFound;

	I18N_ClearCatalog(catalog);
	Q_strncpyz(catalog->language, language, sizeof(catalog->language));
	catalog->attempted = qtrue;

	useFallback    = qfalse;
	exactFileFound = qfalse;
	Q_strncpyz(fallbackLanguage, language, sizeof(fallbackLanguage));

	for (i = 0; fallbackLanguage[i]; i++)
	{
		if (fallbackLanguage[i] == '_' || fallbackLanguage[i] == '-' || fallbackLanguage[i] == '.')
		{
			fallbackLanguage[i] = '\0';
			useFallback         = qtrue;
			break;
		}
	}

	Com_sprintf(pathname, sizeof(pathname), "%s/%s.po", basePath, language);

#if MODLIB
	length = trap_FS_FOpenFile(pathname, &file, FS_READ);
#else
	length = FS_FOpenFileByMode(pathname, &file, FS_READ);
#endif

	if (length > 0)
	{
		exactFileFound = qtrue;
	}
	else if (file)
	{
#if MODLIB
		trap_FS_FCloseFile(file);
#else
		FS_FCloseFile(file);
#endif
		file = 0;
	}

	if (!exactFileFound && useFallback && Q_stricmp(fallbackLanguage, language))
	{
		Com_sprintf(pathname, sizeof(pathname), "%s/%s.po", basePath, fallbackLanguage);

#if MODLIB
		length = trap_FS_FOpenFile(pathname, &file, FS_READ);
#else
		length = FS_FOpenFileByMode(pathname, &file, FS_READ);
#endif

		if (length <= 0)
		{
			if (file)
			{
#if MODLIB
				trap_FS_FCloseFile(file);
#else
				FS_FCloseFile(file);
#endif
			}

			return qfalse;
		}
	}
	else if (!exactFileFound)
	{
		return qfalse;
	}

	if (length >= (int)sizeof(catalog->buffer))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: translation file '%s' is too large (%i bytes)\n", pathname, length);
#if MODLIB
		trap_FS_FCloseFile(file);
#else
		FS_FCloseFile(file);
#endif
		return qfalse;
	}

#if MODLIB
	trap_FS_Read(catalog->fileBuffer, length, file);
	trap_FS_FCloseFile(file);
#else
	FS_Read(catalog->fileBuffer, length, file);
	FS_FCloseFile(file);
#endif

	catalog->fileBuffer[length] = '\0';

	source        = catalog->fileBuffer;
	storage       = catalog->buffer;
	storageEnd    = catalog->buffer + sizeof(catalog->buffer) - 1;
	currentMsgid  = NULL;
	currentMsgstr = NULL;

	while (*source)
	{
		line = source;

		while (*source && *source != '\n' && *source != '\r')
		{
			source++;
		}

		if (*source == '\r')
		{
			*source++ = '\0';
			if (*source == '\n')
			{
				source++;
			}
		}
		else if (*source == '\n')
		{
			*source++ = '\0';
		}

		while (*line == ' ' || *line == '\t')
		{
			line++;
		}

		if (!*line)
		{
			if (!I18N_InsertEntry(catalog, currentMsgid, currentMsgstr))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: translation table overflow while loading '%s'\n", pathname);
				I18N_ClearCatalog(catalog);
				return qfalse;
			}

			currentMsgid  = NULL;
			currentMsgstr = NULL;
			continue;
		}

		if (*line == '#')
		{
			continue;
		}

		if (!Q_strncmp(line, "msgid", 5) && (line[5] == ' ' || line[5] == '\t'))
		{
			if (!I18N_InsertEntry(catalog, currentMsgid, currentMsgstr))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: translation table overflow while loading '%s'\n", pathname);
				I18N_ClearCatalog(catalog);
				return qfalse;
			}

			currentMsgid  = NULL;
			currentMsgstr = NULL;

			if (!I18N_AppendQuoted(line + 5, &currentMsgid, &storage, storageEnd))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: failed to parse msgid in '%s'\n", pathname);
				I18N_ClearCatalog(catalog);
				return qfalse;
			}

			continue;
		}

		if (!Q_strncmp(line, "msgstr", 6) && (line[6] == ' ' || line[6] == '\t'))
		{
			if (!I18N_AppendQuoted(line + 6, &currentMsgstr, &storage, storageEnd))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: failed to parse msgstr in '%s'\n", pathname);
				I18N_ClearCatalog(catalog);
				return qfalse;
			}

			continue;
		}

		if (*line == '\"')
		{
			if (currentMsgstr)
			{
				if (!I18N_AppendQuoted(line, &currentMsgstr, &storage, storageEnd))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: failed to parse continued msgstr in '%s'\n", pathname);
					I18N_ClearCatalog(catalog);
					return qfalse;
				}
			}
			else if (currentMsgid)
			{
				if (!I18N_AppendQuoted(line, &currentMsgid, &storage, storageEnd))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: failed to parse continued msgid in '%s'\n", pathname);
					I18N_ClearCatalog(catalog);
					return qfalse;
				}
			}
		}
	}

	if (!I18N_InsertEntry(catalog, currentMsgid, currentMsgstr))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: translation table overflow while loading '%s'\n", pathname);
		I18N_ClearCatalog(catalog);
		return qfalse;
	}

	catalog->loaded = qtrue;

	return qtrue;
}

/**
 * @brief I18N_TranslateCatalogs
 * @param[in] primaryCatalog
 * @param[in] fallbackCatalog
 * @param[in] msgid
 * @return translated string or the original string when there is no match
 */
static const char *I18N_TranslateCatalogs(const i18n_catalog_t *primaryCatalog, const i18n_catalog_t *fallbackCatalog, const char *msgid)
{
	const char *translation;

	if (!msgid || !msgid[0])
	{
		return msgid;
	}

	translation = I18N_FindTranslation(primaryCatalog, msgid);
	if (translation)
	{
		return translation;
	}

	translation = I18N_FindTranslation(fallbackCatalog, msgid);
	if (translation)
	{
		return translation;
	}

	if ((primaryCatalog && primaryCatalog->loaded) || (fallbackCatalog && fallbackCatalog->loaded))
	{
		I18N_RecordMissing(msgid);
	}

	return msgid;
}

#if MODLIB
/**
 * @brief I18N_RefreshLanguage
 */
static void I18N_RefreshLanguage(void)
{
	char language[MAX_CVAR_VALUE_STRING];
	char gameDir[MAX_CVAR_VALUE_STRING];

	I18N_GetEffectiveLanguage(language, sizeof(language));
	if (!language[0])
	{
		Q_strncpyz(language, "en", sizeof(language));
	}

	trap_Cvar_VariableStringBuffer("fs_game", gameDir, sizeof(gameDir));
	i18nTranslateMod = !gameDir[0] || !Q_stricmp(gameDir, MODNAME);

	if (!Q_stricmp(language, i18nLanguage) && i18nTranslateMod == i18nTranslateModLast)
	{
		return;
	}

	Q_strncpyz(i18nLanguage, language, sizeof(i18nLanguage));
	i18nTranslateModLast = i18nTranslateMod;

	if (!i18nTranslateMod)
	{
		I18N_ClearCatalog(&i18nModCatalog);
		I18N_ClearCatalog(&i18nModEnglishCatalog);
		i18nModCatalog.attempted = qtrue;
		return;
	}

	I18N_LoadCatalog(&i18nModCatalog, "locale/mod", language);

	if (!I18N_LanguageIsEnglish(language))
	{
		I18N_LoadCatalog(&i18nModEnglishCatalog, "locale/mod", "en");
	}
	else
	{
		I18N_ClearCatalog(&i18nModEnglishCatalog);
	}
}
#else
/**
 * @brief I18N_SetLanguage
 * @param[in] language
 */
void I18N_SetLanguage(const char *language)
{
	Q_strncpyz(i18nLanguage, language, sizeof(i18nLanguage));
	I18N_LoadCatalog(&i18nCatalogs[I18N_DOMAIN_CLIENT], "locale/client", language);
	I18N_LoadCatalog(&i18nCatalogs[I18N_DOMAIN_MOD], "locale/mod", language);

	if (!I18N_LanguageIsEnglish(language))
	{
		I18N_LoadCatalog(&i18nEnglishCatalogs[I18N_DOMAIN_CLIENT], "locale/client", "en");
		I18N_LoadCatalog(&i18nEnglishCatalogs[I18N_DOMAIN_MOD], "locale/mod", "en");
	}
	else
	{
		I18N_ClearCatalog(&i18nEnglishCatalogs[I18N_DOMAIN_CLIENT]);
		I18N_ClearCatalog(&i18nEnglishCatalogs[I18N_DOMAIN_MOD]);
	}
}

/**
 * @brief I18N_RefreshLanguage
 */
static void I18N_RefreshLanguage(void)
{
	char language[MAX_CVAR_VALUE_STRING];

	if (!cl_lang)
	{
		return;
	}

	I18N_GetEffectiveLanguage(language, sizeof(language));

	if (Q_stricmp(language, i18nLanguage))
	{
		I18N_SetLanguage(language);
	}
}
#endif

/**
 * @brief I18N_Init
 */
void I18N_Init(void)
{
#if MODLIB
	if (!i18nInitialized)
	{
		i18nInitialized = qtrue;
	}

	I18N_RefreshLanguage();
#else
	if (!i18nInitialized)
	{
		FL_Locale *locale;
		char      language[MAX_CVAR_VALUE_STRING];

		cl_lang      = Cvar_Get("cl_lang", "en", CVAR_ARCHIVE | CVAR_LATCH);
		cl_langDebug = Cvar_Get("cl_langDebug", "0", CVAR_ARCHIVE);

		FL_FindLocale(&locale);

		if (!cl_lang->string[0])
		{
			if (locale->lang && locale->lang[0])
			{
				Cvar_Set("cl_lang", locale->lang);
			}
			else
			{
				Cvar_Set("cl_lang", "en");
			}
		}

		FL_FreeLocale(&locale);
		i18nInitialized = qtrue;

		// Use the effective cvar value so newly selected profile languages apply.
		I18N_GetEffectiveLanguage(language, sizeof(language));
		I18N_SetLanguage(language);
		return;
	}

	I18N_RefreshLanguage();
#endif
}

/**
 * @brief I18N_Translate
 * @param[in] msgid
 * @return
 */
const char *I18N_Translate(const char *msgid)
{
#if MODLIB
	return msgid;
#else
	if (!i18nInitialized)
	{
		I18N_Init();
	}

	I18N_RefreshLanguage();
	return I18N_TranslateCatalogs(&i18nCatalogs[I18N_DOMAIN_CLIENT], &i18nEnglishCatalogs[I18N_DOMAIN_CLIENT], msgid);
#endif
}

/**
 * @brief I18N_TranslateMod
 * @param[in] msgid
 * @return
 */
const char *I18N_TranslateMod(const char *msgid)
{
#if MODLIB
	if (!i18nInitialized)
	{
		I18N_Init();
	}

	I18N_RefreshLanguage();

	// Mod translations are not identity even in English because menu assets use msgids.
	if (!i18nTranslateMod)
	{
		return msgid;
	}

	return I18N_TranslateCatalogs(&i18nModCatalog, &i18nModEnglishCatalog, msgid);
#else
	if (!i18nInitialized)
	{
		I18N_Init();
	}

	I18N_RefreshLanguage();

	// Mod translations are not identity even in English because menu assets use msgids.
	if (!doTranslateMod)
	{
		return msgid;
	}

	return I18N_TranslateCatalogs(&i18nCatalogs[I18N_DOMAIN_MOD], &i18nEnglishCatalogs[I18N_DOMAIN_MOD], msgid);
#endif
}
