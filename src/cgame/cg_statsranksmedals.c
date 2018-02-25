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
 * @file cg_statsranksmedals.c
 */

#include "cg_local.h"

rankicon_t rankicons[NUM_EXPERIENCE_LEVELS][2][2] =
{
	{ // rankicons[0]... is never displayed/used
		{
			{ 0, "",                                        128, 128 }, // gfx/hud/ranks/rank1
			{ 0, "",                                        128, 128 }  // models/players/temperate/common/rank1
		},
		{
			{ 0, "",                                        128, 128 }, // gfx/hud/ranks/xrank1
			{ 0, "",                                        128, 128 }  // models/players/temperate/common/xrank1
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank2",                     128, 128 },
			{ 0, "models/players/temperate/common/rank2",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank2",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank2",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank3",                     128, 128 },
			{ 0, "models/players/temperate/common/rank3",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank3",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank3",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank4",                     128, 128 },
			{ 0, "models/players/temperate/common/rank4",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank4",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank4",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank5",                     128, 128 },
			{ 0, "models/players/temperate/common/rank5",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank5",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank5",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank6",                     128, 128 },
			{ 0, "models/players/temperate/common/rank6",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank6",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank6",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank7",                     128, 128 },
			{ 0, "models/players/temperate/common/rank7",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank7",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank7",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank8",                     128, 128 },
			{ 0, "models/players/temperate/common/rank8",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank8",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank8",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank9",                     128, 128 },
			{ 0, "models/players/temperate/common/rank9",   128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank9",                    128, 128 },
			{ 0, "models/players/temperate/common/xrank9",  128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank10",                    128, 128 },
			{ 0, "models/players/temperate/common/rank10",  128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank10",                   128, 128 },
			{ 0, "models/players/temperate/common/xrank10", 128, 128 }
		},
	},
	{
		{
			{ 0, "gfx/hud/ranks/rank11",                    128, 128 },
			{ 0, "models/players/temperate/common/rank11",  128, 128 }
		},
		{
			{ 0, "gfx/hud/ranks/xrank11",                   128, 128 },
			{ 0, "models/players/temperate/common/xrank11", 128, 128 }
		},
	},
};

/**
 * @brief CG_LoadRankIcons
 */
void CG_LoadRankIcons(void)
{
	int i;

	rankicons[0][0][0].shader = 0;
	rankicons[0][1][0].shader = 0;
	rankicons[0][0][1].shader = 0;
	rankicons[0][1][1].shader = 0;

	for (i = 1; i < NUM_EXPERIENCE_LEVELS; i++)
	{
		rankicons[i][0][0].shader = trap_R_RegisterShaderNoMip(rankicons[i][0][0].iconname);
		rankicons[i][1][0].shader = trap_R_RegisterShaderNoMip(rankicons[i][1][0].iconname);
		rankicons[i][0][1].shader = trap_R_RegisterShaderNoMip(rankicons[i][0][1].iconname);
		rankicons[i][1][1].shader = trap_R_RegisterShaderNoMip(rankicons[i][1][1].iconname);
	}
}
