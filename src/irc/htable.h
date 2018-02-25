/*
 * OpenWolf GPL Source Code
 * Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>
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
 * @file htable.h
 * @brief
 *
 * @note This is FEATURE_IRC_CLIENT and FEATURE_IRC_SERVER only
 */

// Hash table interface
#ifndef INCLUDE_HTABLE_H
#define INCLUDE_HTABLE_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

/*=============================================*
 * Hash table types                            *
 *=============================================*/

// Hash table (opaque type)
struct hashtable_s;
typedef struct hashtable_s *hashtable_t;

/// Function pointer for HT_Apply
typedef qboolean ( *ht_apply_funct )(void *item, void *extra);

/*=============================================*
 * Hash table flags                            *
 *=============================================*/

/// Items are stored inside the table
#define HT_FLAG_INTABLE     (1 << 0)
/// Free items on table destruction
#define HT_FLAG_FREE        (1 << 1)
/// Keys are case-sensitive
#define HT_FLAG_CASE        (1 << 2)
/// Iteration is sorted by key
#define HT_FLAG_SORTED      (1 << 3)

/*=============================================*
 * Hash table functions                        *
 *=============================================*/

hashtable_t HT_Create(
    size_t size,
    unsigned int flags,
    size_t item_size,
    size_t key_offset,
    size_t key_length
    );

/**
 * @def HT_OffsetOfField
 * @brief Macro that determines the offset of a field in a structure
 */
#define HT_OffsetOfField(TYPE, FIELD) \
	((char *)(&(((TYPE *) NULL)->FIELD)) - (char *) NULL)

void HT_Destroy(
    hashtable_t table
    );

void *HT_GetItem(
    hashtable_t table,
    const char *key,
    qboolean *created
    );

void *HT_PutItem(
    hashtable_t table,
    void *item,
    qboolean allow_replacement
    );

qboolean HT_DeleteItem(
    hashtable_t table,
    const char *key,
    void **found
    );

void HT_Apply(
    hashtable_t table,
    ht_apply_funct function,
    void *data
    );

#endif // #ifndef INCLUDE_HTABLE_H
