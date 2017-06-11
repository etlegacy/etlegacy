/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012-2017 ET:Legacy team <mail@etlegacy.com>
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
 * @file renderer2/tr_shader.h
 */

#ifndef TR_SHADER_H
#define TR_SHADER_H

#include "tr_local.h"

#define MAX_GUIDETEXT_HASH      2048
#define MAX_SHADERTABLE_HASH    1024
#define FILE_HASH_SIZE          1024
#define MAX_SHADERTEXT_HASH     2048

#define MAX_SHADER_FILES        4096
#define MAX_GUIDE_FILES         1024

#define MAX_GUIDE_PARAMETERS      16

#define generateHashValue(fname, size) Q_GenerateHashValue(fname, size, qfalse, qtrue)

static shader_t shader;

void R_InitShadersR1(void);
char *FindShaderInShaderTextR1(const char *shaderName);
qboolean ParseShaderR1(char *_text);
shader_t *FinishShaderR1(void);

#endif
