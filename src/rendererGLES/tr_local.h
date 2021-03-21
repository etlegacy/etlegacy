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
 * @file rendererGLES/tr_local.h
 */

#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_public.h"
#include "../renderercommon/tr_common.h"
#include "qgl.h"

#include "GLES/glext.h"
#ifndef GL_RGBA4
#define GL_RGBA4                0x8056
#endif
#ifndef GL_RGB5
#define GL_RGB5                 0x8050
#endif
#define GL_INDEX_TYPE       GL_UNSIGNED_SHORT
typedef unsigned short glIndex_t;

// 14 bits
// can't be increased without changing bit packing for drawsurfs
// see QSORT_SHADERNUM_SHIFT
#define SHADERNUM_BITS          14
#define MAX_SHADERS             (1 << SHADERNUM_BITS)

/**
 * @def ENTITY_LIGHT_STEPS
 * @brief Optimizing diffuse lighting calculation with a table lookup
 */
#define ENTITY_LIGHT_STEPS      64

/**
 * @struct trRefEntity_t
 * @brief A trRefEntity_t has all the information passed in by
 * the client game, as well as some locally derived info
 */
typedef struct
{
	refEntity_t e;

	float axisLength;           ///< compensate for non-normalized axis

	qboolean needDlights;       ///< true for bmodels that touch a dlight
	qboolean lightingCalculated;
	vec3_t lightDir;            ///< normalized direction towards light
	vec3_t ambientLight;        ///< color normalized to 0-255
	int ambientLightInt;        ///< 32 bit rgba packed
	vec3_t directedLight;
	int entityLightInt[ENTITY_LIGHT_STEPS];
	float brightness;
} trRefEntity_t;

/**
 * @struct orientationr_t
 * @brief
 */
typedef struct
{
	vec3_t origin;              ///< in world coordinates
	vec3_t axis[3];             ///< orientation in world
	vec3_t viewOrigin;          ///< viewParms->or.origin in local coordinates
	float modelMatrix[16];
} orientationr_t;

/**
 * @struct image_s
 * @typedef image_t
 * @brief
 */
typedef struct image_s
{
	char imgName[MAX_QPATH];        ///< game path, including extension
	int width, height;              ///< source image
	int uploadWidth, uploadHeight;  ///< after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint texnum;                  ///< gl texture binding

	int frameUsed;                  ///< for texture usage in frame statistics

	int internalFormat;
	int TMU;                        ///< only needed for voodoo2

	qboolean mipmap;
	qboolean allowPicmip;
	int wrapClampMode;              ///< GL_CLAMP or GL_REPEAT

	int hash;                       ///< for fast building of the backupHash

	struct image_s *next;
} image_t;

//===============================================================================

/**
 * @enum shaderSort_t
 * @brief
 */
typedef enum
{
	SS_BAD,
	SS_PORTAL,          ///< mirrors, portals, viewscreens
	SS_ENVIRONMENT,     ///< sky box
	SS_OPAQUE,          ///< opaque

	SS_DECAL,           ///< scorch marks, etc.
	SS_SEE_THROUGH,     ///< ladders, grates, grills that may have small blended edges

	SS_BANNER,          ///< in addition to alpha test

	SS_FOG,

	SS_UNDERWATER,      ///< for items that should be drawn in front of the water plane

	SS_BLEND0,          ///< regular transparency and filters
	SS_BLEND1,          ///< generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,  ///< gun smoke puffs

	SS_NEAREST          ///< blood blobs
} shaderSort_t;

#define MAX_SHADER_STAGES 8

/**
 * @enum genFunc_t
 * @brief
 */
typedef enum
{
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH,
	GF_INVERSE_SAWTOOTH,

	GF_NOISE

} genFunc_t;

/**
 * @enum deform_t
 * @brief
 */
typedef enum
{
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;

/**
 * @enum alphaGen_t
 * @brief
 */
typedef enum
{
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_NORMALZFADE,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
} alphaGen_t;

/**
 * @enum colorGen_t
 * @brief
 */
typedef enum
{
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING, ///< tr.identityLight
	CGEN_IDENTITY,          ///< always (1,1,1,1)
	CGEN_ENTITY,            ///< grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,  ///< grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,      ///< tess.vertexColors
	CGEN_VERTEX,            ///< tess.vertexColors * tr.identityLight
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,          ///< programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_FOG,               ///< standard fog
	CGEN_CONST              ///< fixed color
} colorGen_t;

/**
 * @enum texCoordGen_t
 * @brief
 */
typedef enum
{
	TCGEN_BAD,
	TCGEN_IDENTITY,         ///< clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FIRERISEENV_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR            ///< S and T from world coordinates
} texCoordGen_t;

/**
 * @enum acff_t
 * @brief
 */
typedef enum
{
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

/**
 * @struct waveForm_t
 * @brief
 */
typedef struct
{
	genFunc_t func;

	double base;
	double amplitude;
	double phase;
	double frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

/**
 * @enum texMod_t
 * @brief
 */
typedef enum
{
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE,
	TMOD_SWAP
} texMod_t;

#define MAX_SHADER_DEFORMS  3

/**
 * @struct deformStage_t
 * @brief
 */
typedef struct
{
	deform_t deformation;               ///< vertex coordinate modification type

	vec3_t moveVector;
	waveForm_t deformationWave;
	float deformationSpread;

	float bulgeWidth;
	float bulgeHeight;
	float bulgeSpeed;
} deformStage_t;

/**
 * @struct texModInfo_t
 * @brief
 */
typedef struct
{
	texMod_t type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t wave;

	// used for TMOD_TRANSFORM
	float matrix[2][2];                 ///< s' = s * m[0][0] + t * m[1][0] + trans[0]
	float translate[2];                 ///< t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float scale[2];                     ///< s *= scale[0]
	// t *= scale[1]

	// used for TMOD_SCROLL
	float scroll[2];                    ///< s' = s + scroll[0] * time
	// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float rotateSpeed;

} texModInfo_t;

/**
 * @def MAX_IMAGE_ANIMATIONS
 * @brief Increased this for onfire animation
 */
#define MAX_IMAGE_ANIMATIONS    16

/**
 * @struct textureBundle_t
 * @brief
 *
 * @todo FIXME: change the is* qbooleans to a type index
 */
typedef struct
{
	image_t *image[MAX_IMAGE_ANIMATIONS];
	int numImageAnimations;
	double imageAnimationSpeed;

	texCoordGen_t tcGen;
	vec3_t tcGenVectors[2];

	int numTexMods;
	texModInfo_t *texMods;

	int videoMapHandle;
	qboolean isLightmap;
	qboolean isVideoMap;
} textureBundle_t;

#define NUM_TEXTURE_BUNDLES 2

/**
 * @struct shaderStage_t
 * @brief
 */
typedef struct
{
	qboolean active;

	textureBundle_t bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t rgbWave;
	colorGen_t rgbGen;

	waveForm_t alphaWave;
	alphaGen_t alphaGen;

	byte constantColor[4];                      ///< for CGEN_CONST and AGEN_CONST

	unsigned stateBits;                         ///< GLS_xxxx mask

	acff_t adjustColorsForFog;

	float zFadeBounds[2];

	qboolean isDetail;
	qboolean isFogged;                          ///< used only for shaders that have fog disabled, so we can enable it for individual stages
} shaderStage_t;

struct shaderCommands_s;

#define LIGHTMAP_2D         -4      ///< shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3      ///< pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

/**
 * @enum cullType_t
 * @brief
 */
typedef enum
{
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

/**
 * @enum fogPass_t
 * @brief
 */
typedef enum
{
	FP_NONE,        ///< surface is translucent and will just be adjusted properly
	FP_EQUAL,       ///< surface is opaque but possibly alpha tested
	FP_LE           ///< surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

/**
 * @struct skyParms_t
 * @brief
 */
typedef struct
{
	float cloudHeight;
	image_t *outerbox[6], *innerbox[6];
} skyParms_t;

/**
 * @struct fogParms_t
 * @brief
 */
typedef struct
{
	vec3_t color;
	float depthForOpaque;
	unsigned colorInt;                  ///< in packed byte format
	float tcScale;                      ///< texture coordinate vector scales
} fogParms_t;

/**
 * @struct shader_s
 * @typedef shader_t
 * @brief
 */
typedef struct shader_s
{
	char name[MAX_QPATH];               ///< game path, including extension
	int lightmapIndex;                  ///< for a shader to match, both name and lightmapIndex must match

	int index;                          ///< this shader == tr.shaders[index]
	int sortedIndex;                    ///< this shader == tr.sortedShaders[sortedIndex]

	float sort;                         ///< lower numbered shaders draw before higher numbered

	qboolean defaultShader;             ///< we want to return index 0 if the shader failed to
										///< load for some reason, but R_FindShader should
										///< still keep a name allocated for it, so if
										///< something calls RE_RegisterShader again with
										///< the same name, we don't try looking for it again

	qboolean explicitlyDefined;         ///< found in a .shader file

	int surfaceFlags;                   ///< if explicitlyDefined, this will have SURF_* flags
	int contentFlags;

	qboolean entityMergable;            ///< merge across entites optimizable (smoke, blood)

	qboolean isSky;
	skyParms_t sky;
	fogParms_t fogParms;

	float portalRange;                  ///< distance to fog out at

	vec4_t distanceCull;                ///< opaque alpha range for foliage (inner, outer, alpha threshold, 1/(outer-inner))

	int multitextureEnv;                ///< 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t cullType;                ///< CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean polygonOffset;             ///< set for decals and other items that must be offset
	qboolean noMipMaps;                 ///< for console fonts, 2D elements, etc.
	qboolean noPicMip;                  ///< for images that must always be full resolution

	fogPass_t fogPass;                  ///< draw a blended pass, possibly with depth test equals

	qboolean needsNormal;               ///< not all shaders will need all data to be gathered
	qboolean needsST1;
	qboolean needsST2;
	qboolean needsColor;

	qboolean noFog;

	int numDeforms;
	deformStage_t deforms[MAX_SHADER_DEFORMS];

	int numUnfoggedPasses;
	shaderStage_t *stages[MAX_SHADER_STAGES];

	void (*optimalStageIteratorFunc)(void);

	double clampTime;                                   ///< time this shader is clamped to
	double timeOffset;                                  ///< current time offset for this shader

	struct shader_s *remappedShader;                    ///< current shader this one is remapped too

	struct shader_s *next;
} shader_t;

/**
 * @struct corona_s
 * @typedef corona_t
 * @brief
 */
typedef struct corona_s
{
	vec3_t origin;
	vec3_t color;               ///< range from 0.0 to 1.0, should be color normalized
	vec3_t transformed;         ///< origin in local coordinate system
	float scale;                ///< uses r_flaresize as the baseline (1.0)
	int id;
	qboolean visible;           ///< still send the corona request, even if not visible, for proper fading
} corona_t;

/**
 * @struct dlight_s
 * @typedef dlight_t
 * @brief
 */
typedef struct dlight_s
{
	vec3_t origin;
	vec3_t color;                   ///< range from 0.0 to 1.0, should be color normalized
	float radius;
	float radiusInverseCubed;       ///< attenuation optimization
	float intensity;                ///< 1.0 = fullbright, > 1.0 = overbright
	shader_t *shader;
	int flags;

	vec3_t transformed;             ///< origin in local coordinate system
} dlight_t;

/**
 * @struct decalProjector_s
 * @typedef decalProjector_t
 * @brief Decal projection
 */
typedef struct decalProjector_s
{
	shader_t *shader;
	byte color[4];
	int fadeStartTime, fadeEndTime;
	vec3_t mins, maxs;
	vec3_t center;
	float radius, radius2;
	qboolean omnidirectional;
	int numPlanes;                  ///< either 5 or 6, for quad or triangle projectors
	vec4_t planes[6];
	vec4_t texMat[3][2];
	int projectorNum;               ///< global identifier
}
decalProjector_t;

/**
 * @struct trRefdef_t
 * @brief Holds everything that comes in refdef_t,
 * as well as the locally generated scene information
 */
typedef struct
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];                     ///< transformation matrix

	int time;                               ///< time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;                            ///< RDF_NOWORLDMODEL, etc


	byte areamask[MAX_MAP_AREA_BYTES];      ///< 1 bits will prevent the associated area from rendering at all
	qboolean areamaskModified;              ///< qtrue if areamask changed since last scene

	double floatTime;                       ///< tr.refdef.time / 1000.0

	/// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int num_entities;
	trRefEntity_t *entities;

	int dlightBits;
	int num_dlights;
	dlight_t *dlights;

	int num_coronas;
	corona_t *coronas;

	int numPolys;
	struct srfPoly_s *polys;

	int numPolyBuffers;
	struct srfPolyBuffer_s *polybuffers;

	int decalBits;
	int numDecalProjectors;
	decalProjector_t *decalProjectors;

	int numDecals;
	struct srfDecal_s *decals;

	int numDrawSurfs;
	struct drawSurf_s *drawSurfs;
} trRefdef_t;

//=================================================================================

/**
 * @def MAX_SKIN_SURFACES
 * @brief Max surfaces per-skin
 * @details This is an arbitry limit. Vanilla Q3 only supported 32 surfaces in skins but failed to
 * enforce the maximum limit when reading skin files. It was possile to use more than 32
 * surfaces which accessed out of bounds memory past end of skin->surfaces hunk block.
 */
#define MAX_SKIN_SURFACES   256

/**
 * @struct trRefdef_t
 * @brief Skins allow models to be retextured without modifying the model file
 */
typedef struct
{
	char name[MAX_QPATH];
	int hash;
	shader_t *shader;
} skinSurface_t;

#define MAX_PART_MODELS 7

/**
 * @struct skinModel_t
 * @brief
 */
typedef struct
{
	char type[MAX_QPATH];           ///< md3_lower, md3_lbelt, md3_rbelt, etc.
	char model[MAX_QPATH];          ///< lower.md3, belt1.md3, etc.
	int hash;
} skinModel_t;

/**
 * @struct skin_s
 * @typedef skin_t
 * @brief
 */
typedef struct skin_s
{
	char name[MAX_QPATH];           ///< game path, including extension
	int numSurfaces;
	int numModels;
	skinSurface_t *surfaces;        ///< dynamically allocated array of surfaces
	skinModel_t *models[MAX_PART_MODELS];
} skin_t;

/**
 * @struct skin_s
 * @typedef skin_t
 * @brief
 */
typedef struct
{
	int modelNum;                   ///< bsp model the fog belongs to
	int originalBrushNumber;
	vec3_t bounds[2];

	shader_t *shader;               ///< fog shader to get colorInt and tcScale from
	fogParms_t parms;

	// for clipping distance in fog when outside
	qboolean hasSurface;
	float surface[4];
} fog_t;

/**
 * @struct viewParms_t
 * @brief
 */
typedef struct
{
	orientationr_t orientation;
	orientationr_t world;
	vec3_t pvsOrigin;               ///< may be different than or.origin for portals
	qboolean isPortal;              ///< true if this view is through a portal
	qboolean isMirror;              ///< the portal is a mirror, invert the face culling
	int frameSceneNum;              ///< copied from tr.frameSceneNum
	int frameCount;                 ///< copied from tr.frameCount
	cplane_t portalPlane;           ///< clip anything behind this if mirroring
	int viewportX, viewportY, viewportWidth, viewportHeight;
	float fovX, fovY;
	float projectionMatrix[16];
	cplane_t frustum[5];            ///< added farplane
	vec3_t visBounds[2];
	float zFar;

	glfog_t glFog;                  ///< fog parameters

} viewParms_t;

/*
==============================================================================
SURFACES
==============================================================================
*/

/**
 * @enum surfaceType_t
 * @brief
 *
 * @note Any changes in surfaceType must be mirrored in rb_surfaceTable[]
 */
typedef enum
{
	SF_BAD,
	SF_SKIP,                ///< ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_FOLIAGE,
	SF_POLY,
	SF_MD3,
	SF_MDC,
	SF_MDS,
	SF_MDM,
	SF_FLARE,
	SF_ENTITY,              ///< beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,
	SF_POLYBUFFER,
	SF_DECAL,               ///< decal surfaces

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0xffffffff     ///< ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

/**
 * @struct drawSurf_s
 * @typedef drawSurf_t
 * @brief
 */
typedef struct drawSurf_s
{
	unsigned sort;                      ///< bit combination for fast compares
	surfaceType_t *surface;             ///< any of surface*_t
} drawSurf_t;

#define MAX_FACE_POINTS     1024

#define MAX_PATCH_SIZE      32          ///< max dimensions of a patch mesh in map file
#define MAX_GRID_SIZE       65          ///< max dimensions of a grid mesh in memory

typedef byte color4ub_t[4];

/**
 * @struct srfPoly_s
 * @typedef srfPoly_t
 * @brief When cgame directly specifies a polygon, it becomes a srfPoly_t
 * as soon as it is called
 */
typedef struct srfPoly_s
{
	surfaceType_t surfaceType;
	qhandle_t hShader;
	int fogIndex;
	int numVerts;
	polyVert_t *verts;
} srfPoly_t;

/**
 * @struct srfPolyBuffer_s
 * @typedef srfPolyBuffer_t
 * @brief
 */
typedef struct srfPolyBuffer_s
{
	surfaceType_t surfaceType;
	int fogIndex;
	polyBuffer_t *pPolyBuffer;
} srfPolyBuffer_t;

// decals
#define MAX_DECAL_VERTS         10      ///< worst case is triangle clipped by 6 planes
#define MAX_WORLD_DECALS        1024
#define MAX_ENTITY_DECALS       128

/**
 * @struct srfDecal_s
 * @typedef srfDecal_t
 * @brief
 */
typedef struct srfDecal_s
{
	surfaceType_t surfaceType;
	int numVerts;
	polyVert_t verts[MAX_DECAL_VERTS];
}
srfDecal_t;


/**
 * @struct srfDisplayList_s
 * @typedef srfDisplayList_t
 * @brief
 */
typedef struct srfDisplayList_s
{
	surfaceType_t surfaceType;
	int listNum;
} srfDisplayList_t;

/**
 * @struct srfFlare_s
 * @typedef srfFlare_t
 * @brief
 */
typedef struct srfFlare_s
{
	surfaceType_t surfaceType;
	vec3_t origin;
	vec3_t normal;
	vec3_t color;
} srfFlare_t;

/**
 * @struct srfGeneric_s
 * @typedef srfGeneric_t
 * @brief Normal map drawsurfaces must match this header
 */
typedef struct srfGeneric_s
{
	surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	/// dynamic lighting information
	int dlightBits;
} srfGeneric_t;

/**
 * @struct srfGridMesh_s
 * @typedef srfGridMesh_t
 * @brief
 */
typedef struct srfGridMesh_s
{
	surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	/// dynamic lighting information
	int dlightBits;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t lodOrigin;
	float lodRadius;
	int lodFixed;
	int lodStitched;

	// vertexes
	int width, height;
	float *widthLodError;
	float *heightLodError;
	drawVert_t verts[1];            ///< variable sized
} srfGridMesh_t;

#define VERTEXSIZE  8

/**
 * @struct srfSurfaceFace_s
 * @typedef srfSurfaceFace_t
 * @brief
 */
typedef struct srfSurfaceFace_s
{
	surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	/// dynamic lighting information
	int dlightBits;

	// triangle definitions (no normals at points)
	int numPoints;
	int numIndices;
	int ofsIndices;
	float points[1][VERTEXSIZE];            ///< variable sized
	// there is a variable length list of indices here also
} srfSurfaceFace_t;

/**
 * @struct srfTriangles_s
 * @typedef srfTriangles_t
 * @brief misc_models in maps are turned into direct geometry by q3map2 ;D
 */
typedef struct srfTriangles_s
{
	surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	/// dynamic lighting information
	int dlightBits;

	// triangle definitions
	int numIndexes;
	int *indexes;

	int numVerts;
	drawVert_t *verts;
} srfTriangles_t;

/**
 * @struct srfTriangles2_s
 * @typedef srfTriangles2_t
 * @brief
 */
typedef struct srfTriangles2_s
{
	surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	/// dynamic lighting information
	int dlightBits;

	// triangle definitions
	int numIndexes;
	int *indexes;

	int numVerts;

	vec4_t *xyz;
	vec2_t *st;
	vec2_t *lightmap;
	vec4_t *normal;
	color4ub_t *color;
} srfTriangles2_t;

/// foliage surfaces are autogenerated from models into geometry lists by q3map2
typedef byte fcolor4ub_t[4];

/**
 * @struct foliageInstance_t
 * @brief
 */
typedef struct
{
	vec3_t origin;
	fcolor4ub_t color;
} foliageInstance_t;

/**
 * @struct srfFoliage_t
 * @brief
 */
typedef struct
{
	surfaceType_t surfaceType;

	// culling information
	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	/// dynamic lighting information
	int dlightBits;

	// triangle definitions
	int numIndexes;
	glIndex_t *indexes;

	int numVerts;
	vec4_t *xyz;
	vec4_t *normal;
	vec2_t *texCoords;
	vec2_t *lmTexCoords;

	// origins
	int numInstances;
	foliageInstance_t *instances;
} srfFoliage_t;

extern void(*rb_surfaceTable[SF_NUM_SURFACE_TYPES]) (void *);

/*
==============================================================================
BRUSH MODELS
==============================================================================
*/

// in memory representation

#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2

/**
 * @struct msurface_s
 * @typedef msurface_t
 * @brief
 */
typedef struct msurface_s
{
	int viewCount;                  ///< if == tr.viewCount, already added
	shader_t *shader;
	int fogIndex;

	surfaceType_t *data;            ///< any of srf*_t
} msurface_t;

/**
 * @struct decal_s
 * @typedef decal_t
 * @brief bsp model decal surfaces
 */
typedef struct decal_s
{
	msurface_t *parent;
	shader_t *shader;
	float fadeStartTime, fadeEndTime;
	int fogIndex;
	int numVerts;
	polyVert_t verts[MAX_DECAL_VERTS];
	int projectorNum;
	int frameAdded;                     ///< need to keep decal for at least one frame so we know not to reproject it in later views
} decal_t;

#define CONTENTS_NODE       -1

/**
 * @struct mnode_s
 * @typedef mnode_t
 * @brief
 */
typedef struct mnode_s
{
	// common with leaf and node
	int contents;                   ///< -1 for nodes, to differentiate from leafs
	int visframe;                   ///< node needs to be traversed if current
	vec3_t mins, maxs;              ///< for bounding box culling
	vec3_t surfMins, surfMaxs;      ///< bounding box including surfaces
	struct mnode_s *parent;

	// node specific
	cplane_t *plane;
	struct mnode_s *children[2];

	// leaf specific
	int cluster;
	int area;

	msurface_t **firstmarksurface;
	int nummarksurfaces;
} mnode_t;

/**
 * @struct bmodel_s
 * @typedef bmodel_t
 * @brief
 */
typedef struct bmodel_s
{
	vec3_t bounds[2];                   ///< for culling
	msurface_t *firstSurface;
	int numSurfaces;

	decal_t *decals;

	// for fog volumes
	int firstBrush;
	int numBrushes;
	orientation_t orientation;
	//qboolean visible;
	//int entityNum;
} bmodel_t;

/// optimization
#define WORLD_MAX_SKY_NODES 32

/**
 * @struct world_t
 * @brief
 */
typedef struct
{
	char name[MAX_QPATH];           ///< ie: maps/tim_dm2.bsp
	char baseName[MAX_QPATH];       ///< ie: tim_dm2

	int dataSize;

	int numShaders;
	dshader_t *shaders;

	int numBModels;
	bmodel_t *bmodels;

	int numplanes;
	cplane_t *planes;

	int numnodes;                   ///< includes leafs
	int numDecisionNodes;
	mnode_t *nodes;

	int numSkyNodes;
	mnode_t **skyNodes;             ///< don't walk the entire bsp when rendering sky

	int numsurfaces;
	msurface_t *surfaces;

	int nummarksurfaces;
	msurface_t **marksurfaces;

	int numfogs;
	fog_t *fogs;
	int globalFog;                  ///< index of global fog
	vec4_t globalOriginalFog;       ///< to be able to restore original global fog
	vec4_t globalTransStartFog;     ///< start fog for switch fog transition
	vec4_t globalTransEndFog;       ///< end fog for switch fog transition
	int globalFogTransStartTime;
	int globalFogTransEndTime;

	vec3_t lightGridOrigin;
	vec3_t lightGridSize;
	vec3_t lightGridInverseSize;
	int lightGridBounds[3];
	byte *lightGridData;

	int numClusters;
	int clusterBytes;
	const byte *vis;                ///< may be passed in by CM_LoadMap to save space

	byte *novis;                    ///< clusterBytes of 0xff

	char *entityString;
	char *entityParsePoint;
} world_t;

//======================================================================

/**
 * @enum modtype_t
 * @brief
 */
typedef enum
{
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MDS,
	MOD_MDC,
	MOD_MDM,
	MOD_MDX
} modtype_t;

/**
 * @union model_u
 * @brief
 */
typedef union
{
	bmodel_t *bmodel;               ///< only if type == MOD_BRUSH
	md3Header_t *md3[MD3_MAX_LODS]; ///< only if type == MOD_MESH
	mdsHeader_t *mds;               ///< only if type == MOD_MDS
	mdcHeader_t *mdc[MD3_MAX_LODS]; ///< only if type == MOD_MDC
	mdmHeader_t *mdm;               ///< only if type == MOD_MDM
	mdxHeader_t *mdx;               ///< only if type == MOD_MDX
} model_u;

/**
 * @struct model_s
 * @typedef model_t
 * @brief
 */
typedef struct model_s
{
	char name[MAX_QPATH];
	modtype_t type;
	int index;                      ///< model = tr.models[model->index]

	int dataSize;                   ///< just for listing purposes

	model_u model;

	int numLods;

	qhandle_t shadowShader;
	float shadowParms[6];           ///< x, y, width, height, depth, z offset
} model_t;

#define MAX_MOD_KNOWN   2048

void R_ModelInit(void);
model_t *R_GetModelByHandle(qhandle_t hModel);
int R_LerpTag(orientation_t *tag, const refEntity_t *refent, const char *tagNameIn, int startIndex);
void R_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);

void R_Modellist_f(void);

//====================================================

#define MAX_DRAWIMAGES          2048
#define MAX_LIGHTMAPS           256
#define MAX_SKINS               1024

#define MAX_DRAWSURFS           0x10000
#define DRAWSURF_MASK           (MAX_DRAWSURFS - 1)

/*
the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

modified for Wolf (11 bits of entity num)

old:

22 - 31 : sorted shader index
12 - 21 : entity index
3 - 7   : fog index
2       : used to be clipped flag
0 - 1   : dlightmap index

#define QSORT_SHADERNUM_SHIFT   22
#define QSORT_ENTITYNUM_SHIFT   12
#define QSORT_FOGNUM_SHIFT      3

new:

22 - 31 : sorted shader index
11 - 21 : entity index
2 - 6   : fog index
removed : used to be clipped flag
0 - 1   : dlightmap index

newest: (fixes shader index not having enough bytes)

18 - 31 : sorted shader index
7 - 17  : entity index
2 - 6   : fog index
0 - 1   : dlightmap index
*/
#define QSORT_SHADERNUM_SHIFT   18
#define QSORT_ENTITYNUM_SHIFT   7
#define QSORT_FOGNUM_SHIFT      2
#define QSORT_FRONTFACE_SHIFT   1

extern int gl_filter_min, gl_filter_max;

/**
 * @struct frontEndCounters_t
 * @brief
 * @note performanceCounters_t
 */
typedef struct
{
	int c_sphere_cull_in, c_sphere_cull_out;
	int c_plane_cull_in, c_plane_cull_out;

	int c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int c_leafs;
	int c_dlightSurfaces;
	int c_dlightSurfacesCulled;

	int c_decalProjectors, c_decalTestSurfaces, c_decalClipSurfaces, c_decalSurfaces, c_decalSurfacesCreated;
} frontEndCounters_t;

#define FUNCTABLE_SIZE      4096    ///< % 1024
#define FUNCTABLE_SIZE2     12      ///< % 10
#define FUNCTABLE_MASK      (FUNCTABLE_SIZE - 1)

/**
 * @struct glstate_t
 * @brief The renderer front end should never modify glstate_t
 */
typedef struct
{
	int currenttextures[2];
	int currenttmu;
	qboolean finishCalled;
	int texEnv[2];
	int faceCulling;
	unsigned long glStateBits;
} glstate_t;

/**
 * @struct backEndCounters_t
 * @brief
 */
typedef struct
{
	int c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float c_overDraw;

	int c_dlightVertexes;
	int c_dlightIndexes;

	int c_flareAdds;
	int c_flareTests;
	int c_flareRenders;

	int msec;               ///< total msec for backend run
} backEndCounters_t;

/**
 * @struct backEndState_t
 * @brief all state modified by the back end is seperated
 * from the front end state
 */
typedef struct
{
	trRefdef_t refdef;
	viewParms_t viewParms;
	orientationr_t orientation;
	backEndCounters_t pc;
	qboolean isHyperspace;
	trRefEntity_t *currentEntity;
	qboolean skyRenderedThisView;   ///< flag for drawing sun

	qboolean projection2D;          ///< if qtrue, drawstretchpic doesn't need to change modes
	byte color2D[4];
	qboolean vertexes2D;            ///< shader needs to be finished
	trRefEntity_t entity2D;         ///< currentEntity will point at this when doing 2D rendering
} backEndState_t;

/**
 * @struct trGlobals_t
 * @brief Most renderer globals are defined here.
 * backend functions should never modify any of these fields,
 * but may read fields that aren't dynamically modified
 * by the frontend.
 */
typedef struct
{
	qboolean registered;                        ///< cleared at shutdown, set at beginRegistration

	int visCount;                               ///< incremented every time a new vis cluster is entered
	int frameCount;                             ///< incremented every frame
	int sceneCount;                             ///< incremented every scene
	int viewCount;                              ///< incremented every view (twice a scene if portaled)
												///< and every R_MarkFragments call

	int frameSceneNum;                          ///< zeroed at RE_BeginFrame

	qboolean worldMapLoaded;
	world_t *world;
	char *worldDir;                             ///< for referencing external lightmaps

	const byte *externalVisData;                ///< from RE_SetWorldVisData, shared with CM_Load

	image_t *defaultImage;
	image_t *scratchImage[32];
	image_t *fogImage;
	image_t *dlightImage;                       ///< inverse-square highlight for projective adding
	image_t *flareImage;
	image_t *whiteImage;                        ///< full of 0xff
	image_t *identityLightImage;                ///< full of tr.identityLightByte

	shader_t *defaultShader;
	shader_t *shadowShader;
	shader_t *projectionShadowShader;
	shader_t *dlightShader;

	shader_t *flareShader;
	char *sunShaderName;
	shader_t *sunShader;
	shader_t *sunflareShader[6];                ///< for the camera lens flare effect for sun

	int numLightmaps;
	image_t *lightmaps[MAX_LIGHTMAPS];

	trRefEntity_t *currentEntity;
	trRefEntity_t worldEntity;                  ///< point currentEntity at this when rendering world
	int currentEntityNum;
	int shiftedEntityNum;                       ///< currentEntityNum << QSORT_ENTITYNUM_SHIFT
	model_t *currentModel;
	bmodel_t *currentBModel;                    ///< only valid when rendering brush models

	viewParms_t viewParms;

	float identityLight;                        ///< 1.0 / ( 1 << overbrightBits )
	int identityLightByte;                      ///< identityLight * 255
	int overbrightBits;                         ///< r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t orientation;                 ///< for current entity

	trRefdef_t refdef;

	int viewCluster;

	vec3_t sunLight;                            ///< from the sky shader for this level
	vec3_t sunDirection;

	float lightGridMulAmbient;                  ///< lightgrid multipliers specified in sky shader
	float lightGridMulDirected;

	frontEndCounters_t pc;
	int frontEndMsec;                           ///< not in pc due to clearing issue

	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	model_t *models[MAX_MOD_KNOWN];
	int numModels;

	int numImages;
	image_t *images[MAX_DRAWIMAGES];
	int numCacheImages;                         ///< Unused.

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int numShaders;
	shader_t *shaders[MAX_SHADERS];
	shader_t *sortedShaders[MAX_SHADERS];

	int numSkins;
	skin_t *skins[MAX_SKINS];

	float sinTable[FUNCTABLE_SIZE];
	float squareTable[FUNCTABLE_SIZE];
	float triangleTable[FUNCTABLE_SIZE];
	float sawToothTable[FUNCTABLE_SIZE];
	float inverseSawToothTable[FUNCTABLE_SIZE];

	int allowCompress;                          ///< temp var used while parsing shader only
} trGlobals_t;

extern backEndState_t backEnd;
extern trGlobals_t    tr;
extern glconfig_t     glConfig;     ///< outside of TR since it shouldn't be cleared during ref re-init
extern glstate_t      glState;      ///< outside of TR since it shouldn't be cleared during ref re-init

//====================================================================

void R_SwapBuffers(int);

void R_RenderView(viewParms_t *parms);

void R_AddMD3Surfaces(trRefEntity_t *ent);

void R_TagInfo_f(void);

void R_AddPolygonSurfaces(void);
void R_AddPolygonBufferSurfaces(void);

void R_DecomposeSort(unsigned sort, int *entityNum, shader_t **shader,
					 int *fogNum, int *frontFace, int *dlightMap);

void R_AddDrawSurf(surfaceType_t *surface, shader_t *shader, int fogNum, int frontFace, int dlightMap);

#define CULL_IN     0       ///< completely unclipped
#define CULL_CLIP   1       ///< clipped by one or more planes
#define CULL_OUT    2       ///< completely outside the clipping planes
void R_LocalNormalToWorld(vec3_t local, vec3_t world);
void R_LocalPointToWorld(vec3_t local, vec3_t world);
int R_CullLocalBox(vec3_t bounds[2]);
int R_CullPointAndRadius(vec3_t origin, float radius);
int R_CullLocalPointAndRadius(vec3_t origin, float radius);

void R_RotateForEntity(const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *orientation);

/*
GL wrapper/helper functions
*/
void GL_Bind(image_t *image);
void GL_SetDefaultState(void);
void GL_SelectTexture(int unit);
void GL_TextureMode(const char *string);
void GL_CheckErrors(void);
void GL_State(unsigned long stateBits);
void GL_TexEnv(int env);
void GL_Cull(int cullType);

#define GLS_SRCBLEND_ZERO                       0x00000001
#define GLS_SRCBLEND_ONE                        0x00000002
#define GLS_SRCBLEND_DST_COLOR                  0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR        0x00000004
#define GLS_SRCBLEND_SRC_ALPHA                  0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA        0x00000006
#define GLS_SRCBLEND_DST_ALPHA                  0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA        0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE             0x00000009
#define GLS_SRCBLEND_BITS                       0x0000000f

#define GLS_DSTBLEND_ZERO                       0x00000010
#define GLS_DSTBLEND_ONE                        0x00000020
#define GLS_DSTBLEND_SRC_COLOR                  0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR        0x00000040
#define GLS_DSTBLEND_SRC_ALPHA                  0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA        0x00000060
#define GLS_DSTBLEND_DST_ALPHA                  0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA        0x00000080
#define GLS_DSTBLEND_BITS                       0x000000f0

#define GLS_DEPTHMASK_TRUE                      0x00000100

#define GLS_POLYMODE_LINE                       0x00001000

#define GLS_DEPTHTEST_DISABLE                   0x00010000
#define GLS_DEPTHFUNC_EQUAL                     0x00020000

#define GLS_ATEST_GT_0                          0x10000000
#define GLS_ATEST_LT_80                         0x20000000
#define GLS_ATEST_GE_80                         0x40000000
#define GLS_ATEST_BITS                          0x70000000

#define GLS_DEFAULT         GLS_DEPTHMASK_TRUE

void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void RE_BeginFrame(void);
void RE_BeginRegistration(glconfig_t *glconfigOut);
void RE_LoadWorldMap(const char *name);
void RE_SetWorldVisData(const byte *vis);
qhandle_t RE_RegisterModel(const char *name);
qhandle_t RE_RegisterSkin(const char *name);
void RE_Shutdown(qboolean destroyWindow);

qboolean R_GetEntityToken(char *buffer, size_t size);

float R_ProcessLightmap(byte *pic, int in_padding, int width, int height, byte *pic_out);

qboolean RE_GetSkinModel(qhandle_t skinid, const char *type, char *name);
qhandle_t RE_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap);

model_t *R_AllocModel(void);

void R_Init(void);
image_t *R_FindImageFile(const char *name, qboolean mipmap, qboolean allowPicmip, int glWrapClampMode, qboolean lightmap);

image_t *R_CreateImage(const char *name, const byte *pic, int width, int height, qboolean mipmap, qboolean allowPicmip, int wrapClampMode);

void R_SetColorMappings(void);
void R_GammaCorrect(byte *buffer, int bufSize);

void R_ImageList_f(void);
void R_SkinList_f(void);

const void *RB_TakeScreenshotCmd(const void *data);
void R_ScreenShot_f(void);

void R_InitImages(void);
void R_DeleteTextures(void);
int R_SumOfUsedImages(void);
void R_InitSkins(void);
skin_t *R_GetSkinByHandle(qhandle_t hSkin);

void R_DebugText(const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude);

const void *RB_TakeVideoFrameCmd(const void *data);

// tr_shader.c

qhandle_t RE_RegisterShaderLightMap(const char *name, int lightmapIndex);
qhandle_t RE_RegisterShader(const char *name);
qhandle_t RE_RegisterShaderNoMip(const char *name);
qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage);

shader_t *R_FindShader(const char *name, int lightmapIndex, qboolean mipRawImage);
shader_t *R_GetShaderByHandle(qhandle_t hShader);
shader_t *R_FindShaderByName(const char *name);
void R_InitShaders(void);
void R_ShaderList_f(void);
void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset);

qboolean RE_LoadDynamicShader(const char *shadername, const char *shadertext);

void RE_RenderToTexture(int textureid, int x, int y, int w, int h);

void RE_Finish(void);
int R_GetTextureId(const char *name);

/*
====================================================================
TESSELATOR/SHADER DECLARATIONS
====================================================================
*/

/**
 * @struct stageVars
 * @typedef stageVars_t
 * @brief
 */
typedef struct
{
	color4ub_t colors[SHADER_MAX_VERTEXES];
	vec2_t texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

/**
 * @struct shaderCommands_s
 * @typedef shaderCommands_t
 * @brief
 */
typedef struct shaderCommands_s
{
	glIndex_t indexes[SHADER_MAX_INDEXES];
	vec4_t normal[SHADER_MAX_INDEXES];
	color4ub_t vertexColors[SHADER_MAX_INDEXES];
	vec4_t xyz[SHADER_MAX_INDEXES];
	vec2_t texCoords[SHADER_MAX_INDEXES][2];

	stageVars_t svars;

	color4ub_t constantColor255[SHADER_MAX_VERTEXES];

	shader_t *shader;
	double shaderTime;
	int fogNum;

	int dlightBits;         ///< or together of all vertexDlightBits

	int numIndexes;
	int numVertexes;

	// info extracted from current shader
	int numPasses;
	void (*currentStageIteratorFunc)(void);
	shaderStage_t **xstages;
} shaderCommands_t;

extern shaderCommands_t tess;

void RB_BeginSurface(shader_t *shader, int fogNum);
void RB_EndSurface(void);
void RB_CheckOverflow(int verts, int indexes);
#define RB_CHECKOVERFLOW(v, i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES) { RB_CheckOverflow(v, i); }

void RB_StageIteratorGeneric(void);
void RB_StageIteratorSky(void);
void RB_StageIteratorVertexLitTexture(void);
void RB_StageIteratorLightmappedMultitexture(void);

void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, byte *color);
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2);
void RB_AddQuadStampFadingCornersExt(vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2); // Unused.

void RB_ShowImages(void);

void RB_DrawBounds(vec3_t mins, vec3_t maxs);

/*
============================================================
WORLD MAP
============================================================
*/

void R_AddBrushModelSurfaces(trRefEntity_t *ent);
void R_AddWorldSurfaces(void);

/*
============================================================
FLARES
============================================================
*/

void R_ClearFlares(void);

void RB_AddFlare(void *surface, int fogNum, vec3_t point, vec3_t color, float scale, vec3_t normal, int id, qboolean visible);      //----(SA)  added scale.  added id.  added visible
void RB_AddDlightFlares(void);
void RB_RenderFlares(void);

/*
============================================================
LIGHTS
============================================================
*/

void R_TransformDlights(int count, dlight_t *dl, orientationr_t *orientation);
void R_CullDlights(void);
void R_DlightBmodel(bmodel_t *bmodel);
void R_SetupEntityLighting(const trRefdef_t *refdef, trRefEntity_t *ent);
//int R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);    // Unused

/*
============================================================
SHADOWS
============================================================
*/

void RB_ShadowTessEnd(void);
void RB_ShadowFinish(void);
void RB_ProjectionShadowDeform(void);

/*
============================================================
SKIES
============================================================
*/

void R_BuildCloudData(shaderCommands_t *input);
void R_InitSkyTexCoords(float heightCloud);
void RB_DrawSun(void);
void RB_ClipSkyPolygons(shaderCommands_t *input);

/*
============================================================
CURVE TESSELATION
============================================================
*/

#define PATCH_STITCHING

srfGridMesh_t *R_SubdividePatchToGrid(int width, int height,
									  drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE]);
srfGridMesh_t *R_GridInsertColumn(srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror);
srfGridMesh_t *R_GridInsertRow(srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror);
void R_FreeSurfaceGridMesh(srfGridMesh_t *grid);

/*
============================================================
MARKERS, POLYGON PROJECTION ON WORLD POLYGONS
============================================================
*/

int R_MarkFragments(int numPoints, const vec3_t *points, const vec3_t projection,
					int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);

/*
============================================================
DECALS
============================================================
*/

void RE_ProjectDecal(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime);
void RE_ClearDecals(void);

void R_AddModelShadow(const refEntity_t *ent);

void R_TransformDecalProjector(decalProjector_t * in, vec3_t axis[3], vec3_t origin, decalProjector_t * out);
qboolean R_TestDecalBoundingBox(decalProjector_t *dp, vec3_t mins, vec3_t maxs);
qboolean R_TestDecalBoundingSphere(decalProjector_t *dp, vec3_t center, float radius2);

void R_ProjectDecalOntoSurface(decalProjector_t *dp, msurface_t *surf, bmodel_t *bmodel);

void R_AddDecalSurface(decal_t *decal);
void R_AddDecalSurfaces(bmodel_t *bmodel);
void R_CullDecalProjectors(void);

/*
============================================================
SCENE GENERATION
============================================================
*/

void R_InitNextFrame(void);

void RE_ClearScene(void);
void RE_AddRefEntityToScene(const refEntity_t *ent);
void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);

void RE_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);

void RE_AddPolyBufferToScene(polyBuffer_t *pPolyBuffer);
// modified dlight system to support seperate radius & intensity
// void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw );
void RE_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);

void RE_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);

void RE_RenderScene(const refdef_t *fd);

/*
=============================================================
ANIMATED MODELS
=============================================================
*/

void R_AddAnimSurfaces(trRefEntity_t *ent);
void RB_SurfaceAnim(mdsSurface_t *surface);
int R_GetBoneTag(orientation_t *outTag, mdsHeader_t *mds, int startTagIndex, const refEntity_t *refent, const char *tagName);

// MDM / MDX
void R_MDM_AddAnimSurfaces(trRefEntity_t *ent);
void RB_MDM_SurfaceAnim(mdmSurface_t *surface);
int R_MDM_GetBoneTag(orientation_t *outTag, mdmHeader_t *mdm, int startTagIndex, const refEntity_t *refent, const char *tagName);

/*
=============================================================
=============================================================
*/
void R_TransformModelToClip(const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst);
void R_TransformClipToWindow(const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window);

void RB_DeformTessGeometry(void);

void RB_CalcEnvironmentTexCoords(float *texCoords);
void RB_CalcFireRiseEnvTexCoords(float *st);
void RB_CalcFogTexCoords(float *texCoords);
void RB_CalcScrollTexCoords(const float scrollSpeed[2], float *texCoords);
void RB_CalcRotateTexCoords(float degsPerSecond, float *texCoords);
void RB_CalcScaleTexCoords(const float scale[2], float *texCoords);
void RB_CalcSwapTexCoords(float *texCoords);
void RB_CalcTurbulentTexCoords(const waveForm_t *wf, float *texCoords);
void RB_CalcTransformTexCoords(const texModInfo_t *tmi, float *texCoords);
void RB_CalcModulateColorsByFog(unsigned char *colors);
void RB_CalcModulateAlphasByFog(unsigned char *colors);
void RB_CalcModulateRGBAsByFog(unsigned char *colors);
void RB_CalcWaveAlpha(const waveForm_t *wf, unsigned char *colors);
void RB_CalcWaveColor(const waveForm_t *wf, unsigned char *colors);
void RB_CalcAlphaFromEntity(unsigned char *colors);
void RB_CalcAlphaFromOneMinusEntity(unsigned char *colors);
void RB_CalcStretchTexCoords(const waveForm_t *wf, float *texCoords);
void RB_CalcColorFromEntity(unsigned char *colors);
void RB_CalcColorFromOneMinusEntity(unsigned char *colors);
void RB_CalcSpecularAlpha(unsigned char *alphas);
void RB_CalcDiffuseColor(unsigned char *colors);

/*
=============================================================
RENDERER BACK END FUNCTIONS
=============================================================
*/

/*
=============================================================
RENDERER BACK END COMMAND QUEUE
=============================================================
*/

#define MAX_RENDER_COMMANDS (0x40000 * 2)

/**
 * @struct renderCommandList_t
 * @brief
 */
typedef struct
{
	byte cmds[MAX_RENDER_COMMANDS];
	int used;
} renderCommandList_t;

/**
 * @struct setColorCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	float color[4];
} setColorCommand_t;

/**
 * @struct drawBufferCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	int buffer;
} drawBufferCommand_t;

/**
 * @struct subImageCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	image_t *image;
	int width;
	int height;
	void *data;
} subImageCommand_t;

/**
 * @struct swapBuffersCommand_s
 * @brief
 */
typedef struct
{
	int commandId;
} swapBuffersCommand_t;

/**
 * @struct endFrameCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	int buffer;
} endFrameCommand_t;

/**
 * @struct stretchPicCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	shader_t *shader;
	float x, y;
	float w, h;
	float s1, t1;
	float s2, t2;

	byte gradientColor[4];      ///< color values 0-255
	int gradientType;
	float angle;
} stretchPicCommand_t;

/**
 * @struct poly2dCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	polyVert_t *verts;
	int numverts;
	shader_t *shader;
} poly2dCommand_t;


/**
 * @struct drawSurfsCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	trRefdef_t refdef;
	viewParms_t viewParms;
	drawSurf_t *drawSurfs;
	int numDrawSurfs;
} drawSurfsCommand_t;

/**
 * @enum ssFormat_t
 * @brief
 */
typedef enum
{
	SSF_TGA = 0,
	SSF_JPEG,
	SSF_PNG
} ssFormat_t;

/**
 * @struct screenshotCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	ssFormat_t format;
} screenshotCommand_t;

/**
 * @struct videoFrameCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	int width;
	int height;
	byte *captureBuffer;
	byte *encodeBuffer;
	qboolean motionJpeg;
} videoFrameCommand_t;

/**
 * @struct renderToTextureCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	image_t *image;
	int x;
	int y;
	int w;
	int h;
} renderToTextureCommand_t;

/**
 * @struct renderFinishCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
} renderFinishCommand_t;

/**
 * @enum renderCommand_t
 * @brief
 */
typedef enum
{
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_2DPOLYS,
	RC_ROTATED_PIC,
	RC_STRETCH_PIC_GRADIENT,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT,
	RC_VIDEOFRAME,
	RC_RENDERTOTEXTURE,
	RC_FINISH
} renderCommand_t;

// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
// Heavily increased compared to ioquake
#define MIN_POLYS       4096
#define MIN_POLYVERTS   8192

// modern computers can deal with more than our old MAX (now MIN)
#define MAX_POLYS       16384 ///< was 4096
#define MAX_POLYVERTS   32768 ///< was 8192

// default values for R_Register
#define DEFAULT_POLYS       4096  ///< ET default: 4096
#define DEFAULT_POLYVERTS   12288 ///< ET default: 8192

// max decal projectors per frame, each can generate lots of polys
#define MAX_DECAL_PROJECTORS      128     ///< includes decal projectors that will be culled out, hard limited to 32 active projectors because of bitmasks.
#define MAX_USED_DECAL_PROJECTORS 32
#define MAX_DECALS                1024

#define MAX_POLYBUFFERS 4096

/**
 * @struct backEndData_t
 * @brief All of the information needed by the back end must be
 * contained in a backEndData_t.
 */
typedef struct
{
	drawSurf_t drawSurfs[MAX_DRAWSURFS];
	dlight_t dlights[MAX_DLIGHTS];
	corona_t coronas[MAX_CORONAS];
	trRefEntity_t entities[MAX_REFENTITIES];
	srfPoly_t *polys; // [MAX_POLYS];
	srfPolyBuffer_t polybuffers[MAX_POLYBUFFERS];
	polyVert_t *polyVerts; // [MAX_POLYVERTS];
	decalProjector_t decalProjectors[MAX_DECAL_PROJECTORS];
	srfDecal_t decals[MAX_DECALS];
	renderCommandList_t commands;
} backEndData_t;

extern backEndData_t *backEndData;

void *R_GetCommandBuffer(unsigned int bytes);
void RB_ExecuteRenderCommands(const void *data);

void R_IssuePendingRenderCommands(void);

void R_AddDrawSurfCmd(drawSurf_t *drawSurfs, int numDrawSurfs);

void RE_SetColor(const float *rgba);
void RE_StretchPic(float x, float y, float w, float h,
				   float s1, float t1, float s2, float t2, qhandle_t hShader);
void RE_RotatedPic(float x, float y, float w, float h,
				   float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);
void RE_StretchPicGradient(float x, float y, float w, float h,
						   float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType);
void RE_2DPolyies(polyVert_t *verts, int numverts, qhandle_t hShader);
void RE_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);
void RE_BeginFrame(void);
void RE_EndFrame(int *frontEndMsec, int *backEndMsec);
void RE_SaveJPG(char *filename, int quality, int image_width, int image_height, unsigned char *image_buffer, int padding);
size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality, int image_width, int image_height, byte *image_buffer, int padding);
#ifdef FEATURE_PNG
void RE_SavePNG(char *filename, int image_width, int image_height, unsigned char *image_buffer, int padding);
#endif
void RE_TakeVideoFrame(int width, int height, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg);

// caching system
// NOTE: to disable this for development, set "r_cache 0" in autoexec.cfg

void *R_CacheModelAlloc(int size);
void R_CacheModelFree(void *ptr);
void R_PurgeModels(int count);
void R_BackupModels(void);
qboolean R_FindCachedModel(const char *name, model_t *newmod);
void R_LoadCacheModels(void);

void *R_CacheImageAlloc(int size);
void R_CacheImageFree(void *ptr);
qboolean R_TouchImage(image_t *inImage);
image_t *R_FindCachedImage(const char *name, int hash);
void R_FindFreeTexnum(image_t *image);
void R_LoadCacheImages(void);
void R_PurgeBackupImages(int purgeCount);
void R_BackupImages(void);

void R_CacheShaderFreeExt(const char *name, void *ptr, const char *file, int line);
void *R_CacheShaderAllocExt(const char *name, int size, const char *file, int line);

#define R_CacheShaderAlloc(name, size) R_CacheShaderAllocExt(name, size, __FILE__, __LINE__)
#define R_CacheShaderFree(name, ptr) R_CacheShaderFreeExt(name, ptr, __FILE__, __LINE__)

shader_t *R_FindCachedShader(const char *name, int lightmapIndex, int hash);
void R_BackupShaders(void);
void R_PurgeShaders(int count);
void R_PurgeLightmapShaders(void);
void R_LoadCacheShaders(void);

//------------------------------------------------------------------------------

/**
 * @def NUMMDCVERTEXNORMALS
 * @brief Mesh compression
 */
#define NUMMDCVERTEXNORMALS  256

extern float r_anormals[NUMMDCVERTEXNORMALS][3];

/**
 * @def MDC_MAX_ERROR
 *
 * @brief If any compressed vert is off by more than this from the actual vert, make this a baseframe
 *
 * @note MDC_MAX_ERROR is effectively the compression level. the lower this value, the higher
 * the accuracy, but with lower compression ratios.
 */
#define MDC_MAX_ERROR       0.1

/**
 * @def MDC_MAX_ERROR
 * @brief Lower for more accuracy, but less range
 */
#define MDC_DIST_SCALE      0.05f

/**
 * @def MDC_BITS_PER_AXIS
 * @brief
 * @note We are locked in at 8 or less bits since changing to byte-encoded normals
 */
#define MDC_BITS_PER_AXIS   8
#define MDC_MAX_OFS         127.0f   ///< to be safe

#define MDC_MAX_DIST        (MDC_MAX_OFS * MDC_DIST_SCALE)

// optimized version
#define R_MDC_DecodeXyzCompressed(ofsVec, out, normal) \
	(out)[0] = (((ofsVec) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE; \
	(out)[1] = (((ofsVec >> 8) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE; \
	(out)[2] = (((ofsVec >> 16) & 255) - MDC_MAX_OFS) * MDC_DIST_SCALE; \
	VectorCopy((r_anormals)[(ofsVec >> 24)], normal);

void R_AddMDCSurfaces(trRefEntity_t *ent);

//------------------------------------------------------------------------------

void R_LatLongToNormal(vec3_t outNormal, short latLong);

/*
 * GL FOG
 */

extern glfog_t     glfogsettings[NUM_FOGS];     ///< [0] never used (FOG_NONE)
extern glfogType_t glfogNum;                    ///< fog type to use (from the fog_t enum list)

extern void R_FogOff(void);
extern void R_FogOn(void);

extern void R_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density);

extern int skyboxportal;

// virtual memory
void *R_Hunk_Begin(void);
void R_Hunk_End(void);
void R_FreeImageBuffer(void);

qboolean R_inPVS(const vec3_t p1, const vec3_t p2);

//------------------------------------------------------------------------------

// cvars

extern cvar_t *r_ignoreFastPath;        ///< allows us to ignore our Tess fast paths

extern cvar_t *r_textureBits;           ///< number of desired texture bits
										///< 0 = use framebuffer depth
										///< 16 = use 16-bit textures
										///< 32 = use 32-bit textures
										///< all else = error

extern cvar_t *r_extMaxAnisotropy;      ///< FIXME: not used in GLES ! move it ?
										///< FIXME: "extern int      maxAnisotropy" founded

extern cvar_t *r_lightMap;              ///< render lightmaps only

extern cvar_t *r_showTris;              ///< enables wireframe rendering of the world
extern cvar_t *r_trisColor;             ///< enables modifying of the wireframe colour (in 0xRRGGBB[AA] format, alpha defaults to FF)
extern cvar_t *r_showSky;               ///< forces sky in front of all surfaces
extern cvar_t *r_showNormals;           ///< draws wireframe normals
extern cvar_t *r_normalLength;          ///< length of the normals
//extern cvar_t *r_showmodelbounds;		///< see RB_MDM_SurfaceAnim()

extern cvar_t *r_lodCurveError;

extern cvar_t *r_greyScale;

extern cvar_t *r_directedScale;

extern cvar_t *r_cache;
extern cvar_t *r_cacheShaders;
extern cvar_t *r_cacheModels;

extern cvar_t *r_cacheGathering;

extern cvar_t *r_bonesDebug;

extern cvar_t *r_wolfFog;

extern cvar_t *r_gfxInfo;

#endif //TR_LOCAL_H
