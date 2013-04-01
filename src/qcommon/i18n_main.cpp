/**
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <mail@etlegacy.com>
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
 *
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

#include "../../libs/tinygettext/po_parser.hpp"
#include "../../libs/tinygettext/tinygettext.hpp"

tinygettext::DictionaryManager dictionary;

cvar_t *cl_language;

/**
 * @brief Attempts to detect the system language unless cl_language was already set.
 * Then loads the PO file containing translated strings.
 */
void I18N_Init(void)
{
	FL_Locale                       *locale;
	std::set<tinygettext::Language> languages;

	cl_language = Cvar_Get("cl_language", "", CVAR_ARCHIVE);

	FL_FindLocale(&locale, FL_MESSAGES);

	// Do not change the language if it is already set
	if (cl_language && !cl_language->string[0])
	{
		if (locale->lang && locale->lang[0]) // && locale->country && locale->country[0])
		{
			Cvar_Set("cl_language", va("%s", locale->lang)); //, locale->country));
		}
		else
		{
			// Language detection failed. Fallback to English
			Cvar_Set("cl_language", "en");
		}
	}

	dictionary.add_directory("lang");

	languages = dictionary.get_languages();

	Com_Printf("Available translations:");
	for (std::set<tinygettext::Language>::iterator p = languages.begin(); p != languages.end(); p++)
	{
		Com_Printf(" %s", p->get_name().c_str());
	}

	dictionary.set_language(tinygettext::Language::from_env(std::string(cl_language->string)));
	Com_Printf("\nLanguage set to %s\n", dictionary.get_language().get_name().c_str());

	FL_FreeLocale(&locale);
}

/**
 * @brief Translates a string using the currently selected dictionary
 * @param msgid original string in English
 * @return translated string or English text if dictionary was not found
 */
const char *I18N_Translate(const char *msgid)
{
	// HACK: how to tell tinygettext not to translate if cl_language is English?
	if (Q_stricmp(cl_language->string, "en"))
	{
		return dictionary.get_dictionary().translate(msgid).c_str();
	}
	else
	{
		return msgid;
	}
}
