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

cvar_t *cl_language;

/**
 * @brief Attempts to detect the system language unless cl_language was already set.
 */
void I18N_Init(void)
{
	FL_Locale *locale;

	cl_language = Cvar_Get("cl_language", "", CVAR_ARCHIVE);

	FL_FindLocale(&locale, FL_MESSAGES);

	// Do not change the language if it is already set
	if (!cl_language->string[0])
	{
		if (locale->lang && locale->lang[0] && locale->country && locale->country[0])
		{
			Cvar_Set("cl_language", va("%s_%s", locale->lang, locale->country));
		}
		else
		{
			// Language detection failed. Fallback to English
			Cvar_Set("cl_language", "en");
		}
	}

	Com_Printf("Language set to %s\n", Cvar_VariableString("cl_language"));

	FL_FreeLocale(&locale);
}
