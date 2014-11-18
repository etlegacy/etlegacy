/**
 * @file puff.h
 *
 * @copyright
 * Copyright (C) 2002-2004 Mark Adler <madler@alumni.caltech.edu>
 * Copyright (C) 2006 Joerg Dietrich <dietrich_joerg@gmx.de>
 * For conditions of distribution and use, see copyright notice in puff.h
 * version 1.8, 9 Jan 2004
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the author be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef INCLUDE_PUFF_H
#define INCLUDE_PUFF_H

#include "q_shared.h"            /* for definitions of the <stdint.h> types */

/*
 * See puff.c for purpose and usage.
 */
int32_t puff(uint8_t *dest,         /* pointer to destination pointer */
             uint32_t *destlen,     /* amount of output space */
             uint8_t *source,       /* pointer to source data pointer */
             uint32_t *sourcelen);  /* amount of input available */

#endif // #ifndef INCLUDE_PUFF_H
