/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 * @file keys.h
 */

#ifndef INCLUDE_KEYS_H
#define INCLUDE_KEYS_H

#include "../ui/keycodes.h"

/**
 * @struct qkey_s
 * @brief
 */
typedef struct
{
	qboolean down;
	int repeats;                ///< if > 1, it is autorepeating
	char *binding;
	int hash;
} qkey_t;

extern qboolean key_overstrikeMode;
extern qkey_t   keys[MAX_KEYS];

// NOTE: the declaration of field_t and Field_Clear is now in qcommon/qcommon.h

void Field_KeyDownEvent(field_t *edit, int key);
void Field_CharEvent(field_t *edit, int ch);
void Field_Draw(field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape);
void Field_BigDraw(field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape);

#define     COMMAND_HISTORY     32
extern field_t historyEditLines[COMMAND_HISTORY];

extern field_t g_consoleField;
extern int     anykeydown;

void Key_WriteBindings(fileHandle_t f);
void Key_SetBinding(int keynum, const char *binding);
void Key_GetBindingByString(const char *binding, int *key1, int *key2);
char *Key_GetBinding(int keynum);
qboolean Key_IsDown(int keynum);
qboolean Key_GetOverstrikeMode(void);
void Key_SetOverstrikeMode(qboolean state);
void Key_ClearStates(void);
int Key_GetKey(const char *binding);

#endif // #ifndef INCLUDE_CLIENT_H
