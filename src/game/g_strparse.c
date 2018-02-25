/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * Portions of this file were taken from the NQ project.
 * Credit goes to core
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
 * @file g_strparse.c
 */

#include "g_strparse.h"
#include "g_local.h"

typedef struct
{
	char *name;
	g_StringToken_t g_index;
} g_strtoken_t;

extern const g_strtoken_t *in_word_set(const char *str, unsigned int len);

/**
 * @brief G_GetTokenForString
 * @param[in] str
 * @return
 */
g_StringToken_t G_GetTokenForString(char const *str)
{
	// Use our minimal perfect hash generated code to give us the
	// result token in optimal time
	const g_strtoken_t *token = in_word_set(str, strlen(str));
	if (token == NULL)
	{
		return TOK_UNKNOWN;
	}
	return (g_StringToken_t)token->g_index;
}
