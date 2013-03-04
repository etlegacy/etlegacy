/*
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
 * @file tr_shade_calc.c
 */

#ifndef __TR_EXTRATYPES_H__
#define __TR_EXTRATYPES_H__

// tr_extratypes.h, for mods that want to extend tr_types.h without losing compatibility with original VMs

// extra renderfx flags start at 0x0400
#define RF_SUNFLARE         0x0400

// extra refdef flags start at 0x0008
#define RDF_NOFOG       0x0008      // don't apply fog
#define RDF_EXTRA       0x0010      // Makro - refdefex_t to follow after refdef_t
#define RDF_SUNLIGHT    0x0020      // SmileTheory - render sunlight and shadows

typedef struct
{
	float blurFactor;
	float sunDir[3];
	float sunCol[3];
	float sunAmbCol[3];
} refdefex_t;

#endif
