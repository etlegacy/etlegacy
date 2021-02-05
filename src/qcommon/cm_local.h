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
 * @file cm_local.h
 */

#ifndef INCLUDE_CM_LOAD_H
#define INCLUDE_CM_LOAD_H

#include "q_shared.h"
#include "qcommon.h"
#include "cm_polylib.h"

/// originally was 256 and 255 but needs more than that
/// since this includes func_static and func_explosives
#define MAX_SUBMODELS           512
#define BOX_MODEL_HANDLE        511
#define CAPSULE_MODEL_HANDLE    510

/// enable to make the collision detection a bunch faster
#define MRE_OPTIMIZE

/**
 * @struct cNode_s
 */
typedef struct
{
	cplane_t *plane;
	int children[2];                ///< negative numbers are leafs
} cNode_t;

/**
 * @struct cLeaf_s
 */
typedef struct
{
	int cluster;
	int area;

	int firstLeafBrush;
	int numLeafBrushes;

	int firstLeafSurface;
	int numLeafSurfaces;
} cLeaf_t;

/**
 * @struct cmodel_s
 */
typedef struct cmodel_s
{
	vec3_t mins, maxs;
	cLeaf_t leaf;               ///< submodels don't reference the main tree
} cmodel_t;

/**
 * @struct cbrushside_s
 */
typedef struct
{
	cplane_t *plane;
	int surfaceFlags;
	int shaderNum;
} cbrushside_t;

/**
 * @struct cbrush_s
 */
typedef struct
{
	int shaderNum;              ///< the shader that determined the contents
	int contents;
	vec3_t bounds[2];
	int numsides;
	cbrushside_t *sides;
	int checkcount;            ///< to avoid repeated testings
} cbrush_t;

/**
 * @struct cPatch_s
 */
typedef struct
{
	int checkcount;                    ///< to avoid repeated testings
	int surfaceFlags;
	int contents;
	struct patchCollide_s *pc;
} cPatch_t;

/**
 * @struct cArea_s
 */
typedef struct
{
	int floodnum;
	int floodvalid;
} cArea_t;

/**
 * @struct clipMap_s
 */
typedef struct
{
	char name[MAX_QPATH];

	int numShaders;
	dshader_t *shaders;

	int numBrushSides;
	cbrushside_t *brushsides;

	int numPlanes;
	cplane_t *planes;

	int numNodes;
	cNode_t *nodes;

	int numLeafs;
	cLeaf_t *leafs;

	int numLeafBrushes;
	int *leafbrushes;

	int numLeafSurfaces;
	int *leafsurfaces;

	int numSubModels;
	cmodel_t *cmodels;

	int numBrushes;
	cbrush_t *brushes;

	int numClusters;
	int clusterBytes;
	byte *visibility;
	qboolean vised;             ///< if false, visibility is just a single cluster of ffs

	//int numEntityChars;     // NOTE: Unused
	char *entityString;

	int numAreas;
	cArea_t *areas;
	int *areaPortals;           ///< [ numAreas*numAreas ] reference counts

	int numSurfaces;
	cPatch_t **surfaces;            ///< non-patches will be NULL

	int floodvalid;
	int checkcount;                         ///< incremented on each trace
} clipMap_t;


/// keep 1/8 unit away to keep the position valid before network snapping
/// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON    (0.125f)

extern clipMap_t cm;
extern int       c_pointcontents;
extern int       c_traces, c_brush_traces, c_patch_traces;
extern cvar_t    *cm_noAreas;
extern cvar_t    *cm_noCurves;
extern cvar_t    *cm_playerCurveClip;
extern cvar_t    *cm_optimize;
extern cvar_t    *cm_optimizePatchPlanes;

// cm_test.c

/**
 * @struct sphere_s
 * @brief Used for oriented capsule collision detection
 */
typedef struct
{
	qboolean use;
	float radius;
	float halfheight;
	vec3_t offset;
} sphere_t;

/**
 * @struct traceWork_s
 */
typedef struct
{
	vec3_t start;
	vec3_t end;
	vec3_t size[2];         ///< size of the box being swept through the model
	vec3_t offsets[8];      ///< [signbits][x] = either size[0][x] or size[1][x]
	float maxOffset;        ///< longest corner length from origin
	vec3_t extents;         ///< greatest of abs(size[0]) and abs(size[1])
	vec3_t bounds[2];       ///< enclosing box of start and end surrounding by size
	vec3_t modelOrigin;     ///< origin of the model tracing through
	int contents;           ///< ored contents of the model tracing through
	qboolean isPoint;       ///< optimized case
	trace_t trace;          ///< returned from trace call
	sphere_t sphere;        ///< sphere for oriendted capsule collision

	cplane_t tracePlane1;
	cplane_t tracePlane2;
	float traceDist1;
	float traceDist2;
	vec3_t dir;

} traceWork_t;

/**
 * @struct leafList_s
 */
typedef struct leafList_s
{
	int count;
	int maxcount;
	qboolean overflowed;
	int *list;
	vec3_t bounds[2];
	int lastLeaf;           ///< for overflows where each leaf can't be stored individually
	void (*storeLeafs)(struct leafList_s *ll, int nodenum);
} leafList_t;

int CM_BoxBrushes(const vec3_t mins, const vec3_t maxs, cbrush_t **list, int listsize);

void CM_StoreLeafs(leafList_t *ll, int nodenum);
void CM_StoreBrushes(leafList_t *ll, int nodenum);

void CM_BoxLeafnums_r(leafList_t *ll, int nodenum);

cmodel_t *CM_ClipHandleToModel(clipHandle_t handle);

// cm_patch.c
struct patchCollide_s *CM_GeneratePatchCollide(int width, int height, vec3_t *points, qboolean addBevels);
void CM_TraceThroughPatchCollide(traceWork_t *tw, const struct patchCollide_s *pc);
qboolean CM_PositionTestInPatchCollide(traceWork_t *tw, const struct patchCollide_s *pc);
void CM_ClearLevelPatches(void);

#endif // #ifndef INCLUDE_CM_LOAD_H
