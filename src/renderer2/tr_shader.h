/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
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
 * @file renderer2/tr_shader.h
 */

#ifndef TR_SHADER_H
#define TR_SHADER_H

#include "tr_local.h"

#define MAX_SHADERTABLE_HASH    1024

#define MAX_GUIDETEXT_HASH      2048
#define MAX_SHADERTEXT_HASH     2048

#define FILE_HASH_SIZE          1024

#define MAX_SHADER_FILES        4096
#define MAX_GUIDE_FILES         1024

#define MAX_GUIDE_PARAMETERS      16

#define generateHashValue(fname, size) Q_GenerateHashValue(fname, size, qfalse, qtrue)

// dynamic shader list
typedef struct dynamicShader dynamicShader_t;
struct dynamicShader
{
	char *shadertext;
	dynamicShader_t *next;
};

// scan and load shader files behaviour
#define R_SCAN_SCRIPTS_FOLDER   0x0001      ///< 1  - scan material in scripts folder
#define R_SCAN_MATERIAL_FOLDER  0x0002      ///< 2  - scan material in material folder

// tr_shader_r1.c
int ScanAndLoadShaderFilesR1();
char *FindShaderInShaderTextR1(const char *shaderName);
qboolean ParseShaderR1(char *_text);

// tr_shader.c
void GeneratePermanentShaderTable(float *values, int numValues);
void ParseStencil(char **text, stencil_t *stencil);
void ParseWaveForm(char **text, waveForm_t *wave);
qboolean ParseTexMod(char **text, shaderStage_t *stage);
qboolean LoadMap(shaderStage_t *stage, char *buffer);
qboolean ParseStage(shaderStage_t *stage, char **text);
void ParseDeform(char **text);
void ParseSkyParms(char **text);
void ParseSort(char **text);
qboolean SurfaceParm(const char *token);
void ParseSurfaceParm(char **text);
void ParseDiffuseMap(shaderStage_t *stage, char **text);
void ParseNormalMap(shaderStage_t *stage, char **text);
void ParseSpecularMap(shaderStage_t *stage, char **text);
void ParseGlowMap(shaderStage_t *stage, char **text);
void ParseReflectionMap(shaderStage_t *stage, char **text);
void ParseReflectionMapBlended(shaderStage_t *stage, char **text);
void ParseLightFalloffImage(shaderStage_t *stage, char **text);

#endif
