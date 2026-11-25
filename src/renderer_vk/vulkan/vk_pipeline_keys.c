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
 * @file vulkan/vk_pipeline_keys.c
 * @brief The built-in pipeline key list: the pipelines the stock maps use,
 * created at startup (VK_CreateListedPipelines) so that a first run doesn't
 * compile them in the middle of a frame.
 */

#include "vk_pipeline_keys.h"

// type, stateBits, cullType, polygonOffset, special, vertexSource
const vkPipelineKey_t vkDefaultPipelineKeys[] =
{
	{ VK_PIPELINE_KEY_UI,       0x00000013, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00000022, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00000023, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00000025, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00000065, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00000100, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00000165, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00010065, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00020065, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x00020100, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_UI,       0x40000100, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000012, 0, 0, 2, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000012, 1, 0, 1, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 0, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 0, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 0, 0, 0, 9  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000013, 2, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000021, 0, 0, 0, 3  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000021, 0, 0, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000021, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000021, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 0, 0, 3  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 0, 1, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 2, 0, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000022, 2, 0, 0, 15 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000023, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000025, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000025, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000025, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000033, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000041, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000041, 0, 1, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000042, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000042, 2, 0, 0, 15 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000045, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000045, 2, 0, 0, 15 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000061, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000061, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000061, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000062, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000062, 0, 0, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000062, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000062, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 0, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 0, 0, 15 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 1, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 1, 0, 3  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 1, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 0, 1, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 2, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 2, 0, 0, 3  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000065, 2, 0, 0, 15 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 2  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 3  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 9  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 0, 0, 11 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 0, 1, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 2, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000100, 2, 0, 0, 11 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000113, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000122, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000165, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000165, 2, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000165, 2, 0, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00000165, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00000165, 2, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00010013, 2, 0, 3, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020013, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020023, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020023, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020023, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020023, 2, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020043, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020062, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 0, 0, 11 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 0, 0, 12 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 1, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 0, 1, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 2, 0, 0, 11 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 2, 0, 0, 12 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 2, 0, 0, 14 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020065, 2, 1, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020100, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x00020113, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x00020113, 2, 0, 0, 9  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000065, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 0, 1, 0, 7  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 0, 1, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 2, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 2, 0, 0, 3  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 2, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 2, 0, 0, 10 },
	{ VK_PIPELINE_KEY_SCENE,    0x40000100, 2, 0, 0, 13 },
	{ VK_PIPELINE_KEY_SCENE,    0x40000165, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 0, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 0, 0, 0, 1  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 0, 0, 0, 5  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 0, 0, 0, 6  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 0, 0, 0, 8  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 0, 1, 0, 6  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 2, 0, 0, 0  },
	{ VK_PIPELINE_KEY_SCENE_LM, 0x00000100, 2, 0, 0, 8  },
};

const int vkNumDefaultPipelineKeys = sizeof(vkDefaultPipelineKeys) / sizeof(vkDefaultPipelineKeys[0]);
