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
 * @file renderer2/tr_model_skel.h
 */

#ifndef TR_MODEL_SKEL_H
#define TR_MODEL_SKEL_H

#include "tr_local.h"

qboolean AddTriangleToVBOTriangleList(growList_t * vboTriangles, skelTriangle_t * tri, int *numBoneReferences, int boneReferences[MAX_BONES]);

void AddSurfaceToVBOSurfacesList(growList_t * vboSurfaces, growList_t * vboTriangles, md5Model_t * md5, md5Surface_t * surf, int skinIndex, int numBoneReferences, int boneReferences[MAX_BONES]);
void AddSurfaceToVBOSurfacesList2(growList_t * vboSurfaces, growList_t * vboTriangles, growList_t * vboVertexes, md5Model_t * md5, int skinIndex, const char *materialName, int numBoneReferences, int boneReferences[MAX_BONES]);


#endif                          // TR_MODEL_SKEL_H
