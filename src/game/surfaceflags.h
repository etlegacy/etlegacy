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
 * @file surfaceflags.h
 * @brief This file must be identical in the quake and utils directories
 */

#ifndef INCLUDE_SURFACEFLAGS_H
#define INCLUDE_SURFACEFLAGS_H

/*
 * contents flags are seperate bits
 * a given brush can contribute multiple content bits
 *
 * these definitions also need to be in q_shared.h!
 */
#define CONTENTS_NONE               0x00000000
#define CONTENTS_SOLID              0x00000001
#define CONTENTS_LIGHTGRID          0x00000004
#define CONTENTS_LAVA               0x00000008
#define CONTENTS_SLIME              0x00000010  ///< slag uses the CONTENTS_SLIME flag, the shader reference is changed to 'slag'
#define CONTENTS_WATER              0x00000020
#define CONTENTS_FOG                0x00000040
#define CONTENTS_MISSILECLIP        0x00000080
#define CONTENTS_ITEM               0x00000100
#define CONTENTS_MOVER              0x00004000
#define CONTENTS_AREAPORTAL         0x00008000
#define CONTENTS_PLAYERCLIP         0x00010000
#define CONTENTS_MONSTERCLIP        0x00020000
#define CONTENTS_TELEPORTER         0x00040000
#define CONTENTS_JUMPPAD            0x00080000
#define CONTENTS_CLUSTERPORTAL      0x00100000
#define CONTENTS_DONOTENTER         0x00200000  ///< unused in ETL
#define CONTENTS_DONOTENTER_LARGE   0x00400000  ///< unused in ETL
#define CONTENTS_ORIGIN             0x01000000  ///< removed before bsping an entity
#define CONTENTS_BODY               0x02000000  ///< should never be on a brush, only in game
#define CONTENTS_CORPSE             0x04000000
#define CONTENTS_DETAIL             0x08000000  ///< brushes not used for the bsp

#define CONTENTS_STRUCTURAL     0x10000000      ///< brushes used for the bsp
#define CONTENTS_TRANSLUCENT    0x20000000      ///< don't consume surface fragments inside
#define CONTENTS_TRIGGER        0x40000000
#define CONTENTS_NODROP         0x80000000      ///< don't leave bodies or items (death fog, lava)

#define SURF_NODAMAGE           0x00000001      ///< never give falling damage
#define SURF_SLICK              0x00000002      ///< effects game physics
#define SURF_SKY                0x00000004      ///< lighting from environment map
#define SURF_LADDER             0x00000008
#define SURF_NOIMPACT           0x00000010      ///< don't make missile explosions
#define SURF_NOMARKS            0x00000020      ///< don't leave missile marks
#define SURF_SPLASH             0x00000040      ///< out of surf's, so replacing unused 'SURF_FLESH' - and as SURF_CERAMIC wasn't used, it's now SURF_SPLASH
#define SURF_NODRAW             0x00000080      ///< don't generate a drawsurface at all
#define SURF_HINT               0x00000100      ///< make a primary bsp splitter
#define SURF_SKIP               0x00000200      ///< completely ignore, allowing non-closed brushes
#define SURF_NOLIGHTMAP         0x00000400      ///< surface doesn't need a lightmap
#define SURF_POINTLIGHT         0x00000800      ///< generate lighting info at vertexes
#define SURF_METAL              0x00001000      ///< clanking footsteps
#define SURF_NOSTEPS            0x00002000      ///< no footstep sounds
#define SURF_NONSOLID           0x00004000      ///< don't collide against curves with this set
#define SURF_LIGHTFILTER        0x00008000      ///< act as a light filter during q3map -light
#define SURF_ALPHASHADOW        0x00010000      ///< do per-pixel light shadow casting in q3map
#define SURF_NODLIGHT           0x00020000      ///< don't dlight even if solid (solid lava, skies)
#define SURF_WOOD               0x00040000
#define SURF_GRASS              0x00080000
#define SURF_CERAMIC            0x40            ///< out of surf's, so replacing unused 'SURF_FLESH'
#define SURF_GRAVEL             0x00100000
#define SURF_GLASS              0x00200000      ///< out of surf's, so replacing unused 'SURF_SMGROUP'
#define SURF_SNOW               0x00400000
#define SURF_ROOF               0x00800000
#define SURF_RUBBLE             0x01000000
#define SURF_CARPET             0x02000000
#define SURF_MONSTERSLICK       0x04000000      ///< slick surf that only affects ai's
#define SURF_MONSLICK_W         0x08000000
#define SURF_MONSLICK_N         0x10000000
#define SURF_MONSLICK_E         0x20000000
#define SURF_MONSLICK_S         0x40000000

#define SURF_LANDMINE           0x80000000      ///< ok to place landmines on this surface

#endif // #ifndef INCLUDE_SURFACEFLAGS_H
