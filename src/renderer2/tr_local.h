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
 * @file renderer2/tr_local.h
 */

#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../renderercommon/tr_common.h"
#include "../renderercommon/tr_public.h"
#include "tr_extra.h"

#if 1
#define GL_INDEX_TYPE       GL_UNSIGNED_INT
typedef unsigned int glIndex_t;
#else
#define GL_INDEX_TYPE       GL_UNSIGNED_SHORT
typedef unsigned short glIndex_t;
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define MAX_SHADERS             (1 << 12)
#define SHADERS_MASK            (MAX_SHADERS - 1)

#define MAX_SHADER_TABLES       1024
#define MAX_SHADER_STAGES       16

#define MAX_OCCLUSION_QUERIES   4096

#define MAX_FBOS                64

#define MAX_VISCOUNTS           5
#define MAX_VIEWS               10

#define MAX_SHADOWMAPS          5

//#define VOLUMETRIC_LIGHTING 1

#define CALC_REDUNDANT_SHADOWVERTS 0

//#define USE_BSP_CLUSTERSURFACE_MERGING 1

/**
 * @enum deferredShading_t
 * @brief
 */
typedef enum
{
	DS_DISABLED = 0,                ///< traditional Doom 3 style rendering
	DS_STANDARD,                ///< deferred rendering like in Stalker
} deferredShading_t;

#define HDR_ENABLED() ((r_hdrRendering->integer && glConfig2.textureFloatAvailable && glConfig2.framebufferObjectAvailable && glConfig2.framebufferBlitAvailable))

// The cubeProbes used for reflections:
// The width/height (in pixels) of a cubemap texture
#define REF_CUBEMAP_SIZE        32
// The width/height (in pixels) of a 'cm' image (1 cm image contains multiple cubemaps)
#define REF_CUBEMAP_STORE_SIZE  1024
#define REF_CUBEMAP_STORE_SIDE  (REF_CUBEMAP_STORE_SIZE / REF_CUBEMAP_SIZE)
#define REF_CUBEMAPS_PER_FILE   (REF_CUBEMAP_STORE_SIDE * REF_CUBEMAP_STORE_SIDE)

/**
 * @enum cullResult_t
 * @brief
 */
typedef enum
{
	CULL_IN = 0,                ///< completely unclipped
	CULL_CLIP,                  ///< clipped by one or more planes
	CULL_OUT,                   ///< completely outside the clipping planes
} cullResult_t;

/**
 * @struct screenRect_s
 * @typedef screenRect_t
 * @brief
 */
typedef struct screenRect_s
{
	int coords[4];
	struct screenRect_s *next;
} screenRect_t;

/**
 * @enum frustumBits_t
 * @brief
 */
typedef enum
{
	FRUSTUM_LEFT = 0,
	FRUSTUM_RIGHT,
	FRUSTUM_BOTTOM,
	FRUSTUM_TOP,
	FRUSTUM_NEAR,
	FRUSTUM_FAR,
	FRUSTUM_PLANES  = 6,//cplane_t frustum_t[6];
	FRUSTUM_CLIPALL = 0 | 1 | 2 | 4 | 8 | 16    //| 32
} frustumBits_t;

typedef cplane_t frustum_t[6];

/**
 * @enum cubesides
 * @brief -- this is used for culling
 */
enum
{
	CUBESIDE_PX      = (1 << 0),
	CUBESIDE_PY      = (1 << 1),
	CUBESIDE_PZ      = (1 << 2),
	CUBESIDE_NX      = (1 << 3),
	CUBESIDE_NY      = (1 << 4),
	CUBESIDE_NZ      = (1 << 5),
	CUBESIDE_CLIPALL = 1 | 2 | 4 | 8 | 16 | 32
};

/**
 * @struct link_s
 * @typedef link_t
 * @brief
 */
typedef struct link_s
{
	void *data;
	int numElements;                ///< only used by sentinels
	struct link_s *prev, *next;
} link_t;

/**
 * @brief InitLink
 * @param[in,out] l
 * @param[in] data
 */
static ID_INLINE void InitLink(link_t *l, void *data)
{
	l->data = data;
	l->prev = l->next = l;
}

/**
 * @brief ClearLink
 * @param[in,out] l
 */
static ID_INLINE void ClearLink(link_t *l)
{
	l->data = NULL;
	l->prev = l->next = l;
}

/**
 * @brief RemoveLink
 * @param[in,out] l
 */
static ID_INLINE void RemoveLink(link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;

	l->prev = l->next = NULL;
}

/*
 * @brief InsertLinkBefore
 * @param[in,out] l
 * @param[in] sentinel
 *
 * @note Unused
static ID_INLINE void InsertLinkBefore(link_t *l, link_t *sentinel)
{
	l->next = sentinel;
	l->prev = sentinel->prev;

	l->prev->next = l;
	l->next->prev = l;
}
*/

/**
 * @brief InsertLink
 * @param[in,out] l
 * @param[in] sentinel
 */
static ID_INLINE void InsertLink(link_t *l, link_t *sentinel)
{
	l->next = sentinel->next;
	l->prev = sentinel;

	l->next->prev = l;
	l->prev->next = l;
}

/**
 * @brief StackEmpty
 * @param[in] l
 * @return
 */
static ID_INLINE qboolean StackEmpty(link_t *l)
{
	// GCC shit: cannot convert 'bool' to 'qboolean' in return
	return l->next == l ? qtrue : qfalse;
}

/**
 * @brief StackTop
 * @param[in] l
 * @return
 */
static ID_INLINE link_t *StackTop(link_t *l)
{
	return l->next;
}

/**
 * @brief StackPush
 * @param[in] sentinel
 * @param[in] data
 */
static ID_INLINE void StackPush(link_t *sentinel, void *data)
{
	link_t *l;

	l = (link_t *) Com_Allocate(sizeof(*l));
	InitLink(l, data);

	InsertLink(l, sentinel);
}

/**
 * @brief StackPop
 * @param[in] l
 * @return
 */
static ID_INLINE void *StackPop(link_t *l)
{
	link_t *top;
	void   *data;

	if (l->next == l)
	{
		return NULL;
	}

	top = l->next;

#if 1
	RemoveLink(top);
#else
	top->next->prev = top->prev;
	top->prev->next = top->next;

	top->prev = top->next = NULL;
#endif

	data = top->data;
	Com_Dealloc(top);

	return data;
}

/**
 * @brief QueueInit
 * @param[in,out] l
 */
static ID_INLINE void QueueInit(link_t *l)
{
	l->data        = NULL;
	l->numElements = 0;
	l->prev        = l->next = l;
}

/**
 * @brief QueueSize
 * @param[in] l
 * @return
 */
static ID_INLINE int QueueSize(link_t *l)
{
	return l->numElements;
}

/**
 * @brief QueueEmpty
 * @param[in] l
 * @return
 */
static ID_INLINE qboolean QueueEmpty(link_t *l)
{
	return l->prev == l ? qtrue : qfalse;
}

/**
 * @brief EnQueue
 * @param[in,out] sentinel
 * @param[in] data
 */
static ID_INLINE void EnQueue(link_t *sentinel, void *data)
{
	link_t *l;

	l = (link_t *) Com_Allocate(sizeof(*l));
	InitLink(l, data);

	InsertLink(l, sentinel);

	sentinel->numElements++;
}

/*
 * @brief EnQueue2
 * @param[in,out] sentinel
 * @param[in] data
 *
 * @note Unused
static ID_INLINE void EnQueue2(link_t *sentinel, void *data, void *(*mallocFunc)(size_t __size))
{
	link_t *l;

	l = mallocFunc(sizeof(*l));
	InitLink(l, data);

	InsertLink(l, sentinel);

	sentinel->numElements++;
}
*/

/**
 * @brief DeQueue
 * @param[in,out] l
 * @return
 */
static ID_INLINE void *DeQueue(link_t *l)
{
	link_t *tail;
	void   *data;

	tail = l->prev;

#if 1
	RemoveLink(tail);
#else
	tail->next->prev = tail->prev;
	tail->prev->next = tail->next;

	tail->prev = tail->next = NULL;
#endif

	data = tail->data;
	Com_Dealloc(tail);

	l->numElements--;

	return data;
}

/**
 * @brief QueueFront
 * @param[in] l
 * @return
 */
static ID_INLINE link_t *QueueFront(link_t *l)
{
	return l->prev;
}

/**
 * @struct trRefLight_s
 * @typedef trRefLight_t
 * @brief A trRefLight_t has all the information passed in by
 * the client game, as well as some locally derived info
 */
typedef struct trRefLight_s
{
	/// public from client game
	refLight_t l;

	// local
	qboolean isStatic;                  ///< loaded from the BSP entities lump
	qboolean noRadiosity;               ///< this is a pure realtime light that was not considered by XMap2
	qboolean additive;                  ///< texture detail is lost tho when the lightmap is dark
	vec3_t origin;                      ///< l.origin + rotated l.center
	vec3_t transformed;                 ///< origin in local coordinate system
	vec3_t direction;                   ///< for directional lights (sun)

	mat4_t transformMatrix;             ///< light to world
	mat4_t viewMatrix;                  ///< object to light
	mat4_t projectionMatrix;            ///< light frustum

	float falloffLength;

	mat4_t shadowMatrices[MAX_SHADOWMAPS];
	mat4_t shadowMatricesBiased[MAX_SHADOWMAPS];
	mat4_t attenuationMatrix;           ///< attenuation * (light view * entity transform)
	mat4_t attenuationMatrix2;          ///< attenuation * tcMod matrices

	cullResult_t cull;
	vec3_t localBounds[2];
	vec3_t worldBounds[2];
	float sphereRadius;                 ///< calculated from localBounds

	int8_t shadowLOD;                   ///< Level of Detail for shadow mapping

	// GL_EXT_depth_bounds_test
	float depthNear;
	float depthFar;
	qboolean noDepthBoundsTest;

	qboolean clipsNearPlane;

	qboolean noOcclusionQueries;
	uint32_t occlusionQueryObject;
	uint32_t occlusionQuerySamples;
	link_t multiQuery;                  ///< CHC++: list of all nodes that are used by the same occlusion query

	frustum_t frustum;
	vec4_t localFrustum[6];
	struct VBO_s *frustumVBO;
	struct IBO_s *frustumIBO;
	uint16_t frustumIndexes;
	uint16_t frustumVerts;

	screenRect_t scissor;

	struct shader_s *shader;

	struct interactionCache_s *firstInteractionCache;   ///< only used by static lights
	struct interactionCache_s *lastInteractionCache;    ///< only used by static lights

	struct interactionVBO_s *firstInteractionVBO;       ///< only used by static lights
	struct interactionVBO_s *lastInteractionVBO;        ///< only used by static lights

	struct interaction_s *firstInteraction;
	struct interaction_s *lastInteraction;

	uint16_t numInteractions;                           ///< total interactions
	uint16_t numShadowOnlyInteractions;
	uint16_t numLightOnlyInteractions;
	qboolean noSort;                                    ///< don't sort interactions by material

	link_t leafs;

	int visCounts[MAX_VISCOUNTS];                       ///< node needs to be traversed if current
	//struct bspNode_s **leafs;
	//int             numLeafs;
} trRefLight_t;

/**
 * @struct trRefEntity_s
 * @brief a trRefEntity_t has all the information passed in by
 * the client game, as well as some locally derived info
 */
typedef struct
{
	/// public from client game
	refEntity_t e;

	// local
	float axisLength;                   ///< compensate for non-normalized axis
	qboolean lightingCalculated;
	vec3_t lightDir;                    ///< normalized direction towards light
	vec3_t ambientLight;                ///< color normalized to 0-1
	vec3_t directedLight;

	cullResult_t cull;
	vec3_t localBounds[2];
	vec3_t worldBounds[2];

	// GPU occlusion culling
	qboolean noOcclusionQueries;
	uint32_t occlusionQueryObject;
	uint32_t occlusionQuerySamples;
	link_t multiQuery;                  ///< CHC++: list of all nodes that are used by the same occlusion query
} trRefEntity_t;

/**
 * @struct orientationr_t
 * @brief
 */
typedef struct
{
	vec3_t origin;          ///< in world coordinates
	vec3_t axis[3];         ///< orientation in world
	vec3_t viewOrigin;      ///< viewParms->or.origin in local coordinates
	mat4_t transformMatrix; ///< transforms object to world: either used by camera, model or light
	mat4_t viewMatrix;      ///< affine inverse of transform matrix to transform other objects into this space
	mat4_t viewMatrix2;     ///< without quake2opengl conversion
	mat4_t modelViewMatrix; ///< only used by models, camera viewMatrix * transformMatrix
} orientationr_t;

/**
 * @struct vertexHash_s
 * @typedef vertexHash_t
 * @brief Useful helper struct
 */
typedef struct vertexHash_s
{
	vec3_t xyz;
	void *data;

	struct vertexHash_s *next;
} vertexHash_t;

enum
{
	IF_NONE,
	IF_NOPICMIP                = BIT(0),
	IF_NOCOMPRESSION           = BIT(1),
	IF_ALPHA                   = BIT(2),
	IF_NORMALMAP               = BIT(3),
	IF_RGBA16F                 = BIT(4),
	IF_RGBA32F                 = BIT(5),
	IF_LA16F                   = BIT(6),
	IF_LA32F                   = BIT(7),
	IF_ALPHA16F                = BIT(8),
	IF_ALPHA32F                = BIT(9),
	IF_DEPTH16                 = BIT(10),
	IF_DEPTH24                 = BIT(11),
	IF_DEPTH32                 = BIT(12),
	IF_PACKED_DEPTH24_STENCIL8 = BIT(13),
	IF_LIGHTMAP                = BIT(14),
	IF_RGBA16                  = BIT(15),
	IF_RGBE                    = BIT(16),
	IF_ALPHATEST               = BIT(17),
	IF_DISPLACEMAP             = BIT(18)
};

/**
 * @struct filterType_t
 * @brief
 */
typedef enum
{
	FT_DEFAULT = 0,
	FT_LINEAR,
	FT_NEAREST
} filterType_t;

/**
 * @struct wrapType_t
 * @brief
 */
typedef enum
{
	WT_REPEAT = 0,
	WT_CLAMP,                   ///< don't repeat the texture for texture coords outside [0, 1]
	WT_EDGE_CLAMP,
	WT_ZERO_CLAMP,              ///< guarantee 0,0,0,255 edge for projected textures
	WT_ALPHA_ZERO_CLAMP         ///< guarante 0 alpha edge for projected textures
} wrapType_t;

/**
 * @struct image_s
 * @typedef image_t
 * @brief
 */
typedef struct image_s
{
	char name[1024];                        ///< formerly MAX_QPATH, game path, including extension
											///< can contain stuff like this now:
											///< addnormals ( textures/base_floor/stetile4_local.tga ,
											///< heightmap ( textures/base_floor/stetile4_bmp.tga , 4 ) )
	GLenum type;
	GLuint texnum;                          ///< gl texture binding

	uint16_t width, height;                 ///< source image
	uint16_t uploadWidth, uploadHeight;     ///< after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE

	int frameUsed;                          ///< for texture usage in frame statistics

	uint32_t internalFormat;

	uint32_t bits;
	filterType_t filterType;
	wrapType_t wrapType;                    ///< GL_CLAMP or GL_REPEAT

	struct image_s *next;
} image_t;

/**
 * @struct BufferImage_s
 * @typedef BufferImage_t
 * @brief
 *
 * @todo TODO: add this to the framebuffer structure
 */
typedef struct BufferImage_s
{
	uint32_t buffer;
	int format;
	image_t *texture;
} BufferImage_t;

/**
 * @enum BuffetType_t
 * @brief
 */
typedef enum
{
	DEFAULT = 0,
	DIFFUSE,
	NORMAL,
	SPECULAR
} BuffetType_t;

/**
 * @struct FBO_s
 * @typedef FBO_t
 * @brief
 */
typedef struct FBO_s
{
	char name[MAX_QPATH];

	int index;

	uint32_t frameBuffer;

	BufferImage_t colorBuffers[16];
	BufferImage_t depthBuffer;
	BufferImage_t stencilBuffer;
	BufferImage_t packedDepthStencilBuffer;

	int width;
	int height;
} FBO_t;

/**
 * @enum vboUsage_t
 * @brief
 */
typedef enum
{
	VBO_USAGE_STATIC = 0,
	VBO_USAGE_DYNAMIC
} vboUsage_t;

/**
 * @struct VBO_s
 * @typedef VBO_t
 * @brief
 */
typedef struct VBO_s
{
	char name[MAX_QPATH];

	uint32_t vertexesVBO;
	uint32_t vertexesSize;          ///< amount of memory data allocated for all vertices in bytes
	uint32_t vertexesNum;
	uint32_t ofsXYZ;
	uint32_t ofsTexCoords;
	uint32_t ofsLightCoords;
	uint32_t ofsTangents;
	uint32_t ofsBinormals;
	uint32_t ofsNormals;
	uint32_t ofsColors;
	//uint32_t ofsPaintColors;        ///< for advanced terrain blending
	//uint32_t ofsLightDirections;
	uint32_t ofsBoneIndexes;
	uint32_t ofsBoneWeights;

	uint32_t sizeXYZ;
	uint32_t sizeTangents;
	uint32_t sizeBinormals;
	uint32_t sizeNormals;

	uint32_t attribs;
} VBO_t;

/**
 * @struct IBO_s
 * @typedef IBO_t
 * @brief
 */
typedef struct IBO_s
{
	char name[MAX_QPATH];

	uint32_t indexesVBO;
	uint32_t indexesSize;           ///< amount of memory data allocated for all triangles in bytes
	uint32_t indexesNum;

//  uint32_t        ofsIndexes;
} IBO_t;

//===============================================================================

/**
 * @enum shaderSort_t
 * @brief
 */
typedef enum
{
	SS_BAD = 0,
	SS_PORTAL,                  ///< mirrors, portals, viewscreens

	SS_ENVIRONMENT_FOG,         ///< sky

	SS_OPAQUE,                  ///< opaque

	SS_ENVIRONMENT_NOFOG,       ///< moved skybox here so we can fog post process all SS_OPAQUE materials

	SS_DECAL,                   ///< scorch marks, etc.
	SS_SEE_THROUGH,             ///< ladders, grates, grills that may have small blended edges
								///< in addition to alpha test
	SS_BANNER,

	SS_FOG,

	SS_UNDERWATER,              ///< for items that should be drawn in front of the water plane
	SS_WATER,

	SS_FAR,
	SS_MEDIUM,
	SS_CLOSE,

	SS_BLEND0,                  ///< regular transparency and filters
	SS_BLEND1,                  ///< generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,

	SS_ALMOST_NEAREST,          ///< gun smoke puffs

	SS_NEAREST,                 ///< blood blobs
	SS_POST_PROCESS
} shaderSort_t;

/**
 * @struct shaderTable_s
 * @typedef shaderTable_t
 * @brief
 */
typedef struct shaderTable_s
{
	char name[MAX_QPATH];

	uint16_t index;

	qboolean clamp;
	qboolean snap;

	float *values;
	uint16_t numValues;

	struct shaderTable_s *next;
} shaderTable_t;

/**
 * @enum genFunc_t
 * @brief
 */
typedef enum
{
	GF_NONE = 0,

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
	DEFORM_NONE = 0,
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
	DEFORM_TEXT7,
	DEFORM_SPRITE,
	DEFORM_FLARE
} deform_t;

/**
 * @enum deformGen_t
 * @brief deformVertexes types that can be handled by the GPU
 */
typedef enum
{
	// do not edit: same as genFunc_t

	DGEN_NONE = 0,
	DGEN_WAVE_SIN,
	DGEN_WAVE_SQUARE,
	DGEN_WAVE_TRIANGLE,
	DGEN_WAVE_SAWTOOTH,
	DGEN_WAVE_INVERSE_SAWTOOTH,
	DGEN_WAVE_NOISE,

	// do not edit until this line

	DGEN_BULGE,
	DGEN_MOVE
} deformGen_t;

/**
 * @enum deformType_t
 * @brief
 */
typedef enum
{
	DEFORM_TYPE_NONE = 0,
	DEFORM_TYPE_CPU,
	DEFORM_TYPE_GPU,
} deformType_t;

/**
 * @enum alphaGen_t
 * @brief
 */
typedef enum
{
	AGEN_IDENTITY = 0,
	//AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_NORMALZFADE,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	//AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_CONST,
	AGEN_CUSTOM
} alphaGen_t;

/**
 * @enum colorGen_t
 * @brief
 */
typedef enum
{
	CGEN_BAD = 0,
	CGEN_IDENTITY_LIGHTING,     ///< tr.identityLight
	CGEN_IDENTITY,              ///< always (1,1,1,1)
	CGEN_ENTITY,                ///< grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,      ///< grabbed from 1 - entity.modulate
	//CGEN_EXACT_VERTEX,        ///< tess.vertexColors
	CGEN_VERTEX,                ///< tess.vertexColors * tr.identityLight
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,              ///< programmatically generated
	//CGEN_LIGHTING_DIFFUSE,      /// and tr.identitylight
	CGEN_FOG,                   ///< standard fog
	CGEN_CONST,                 ///< fixed color
	CGEN_CUSTOM_RGB,            ///< like fixed color but generated dynamically, single arithmetic expression
	CGEN_CUSTOM_RGBs,           ///< multiple expressions
} colorGen_t;

/**
 * @enum colorGen_t
 * @brief
 *
 * @note Not used in r2
 */
typedef enum
{
	TCGEN_BAD = 0,
	TCGEN_IDENTITY,             ///< clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR                ///< S and T from world coordinates
} texCoordGen_t;

/**
 * @enum acff_t
 * @brief
 */
typedef enum
{
	ACFF_NONE = 0,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

/**
 * @enum alphaTest_t
 * @brief
 */
typedef enum
{
	ATEST_NONE = 0,
	ATEST_GT_0,
	ATEST_LT_128,
	ATEST_GE_128
} alphaTest_t;

/**
 * @enum opcode_t
 * @brief
 */
typedef enum
{
	OP_BAD = 0,
	// logic operators
	OP_LAND,
	OP_LOR,
	OP_GE,
	OP_LE,
	OP_LEQ,
	OP_LNE,
	// arithmetic operators
	OP_ADD,
	OP_SUB,
	OP_DIV,
	OP_MOD,
	OP_MUL,
	OP_NEG,
	// logic operators
	OP_LT,
	OP_GT,
	// embracements
	OP_LPAREN,
	OP_RPAREN,
	OP_LBRACKET,
	OP_RBRACKET,
	// constants or variables
	OP_NUM,
	OP_TIME,
	OP_PARM0,
	OP_PARM1,
	OP_PARM2,
	OP_PARM3,
	OP_PARM4,
	OP_PARM5,
	OP_PARM6,
	OP_PARM7,
	OP_PARM8,
	OP_PARM9,
	OP_PARM10,
	OP_PARM11,
	OP_GLOBAL0,
	OP_GLOBAL1,
	OP_GLOBAL2,
	OP_GLOBAL3,
	OP_GLOBAL4,
	OP_GLOBAL5,
	OP_GLOBAL6,
	OP_GLOBAL7,
	OP_FRAGMENTSHADERS,
	OP_FRAMEBUFFEROBJECTS,
	OP_SOUND,
	OP_DISTANCE,
	// table access
	OP_TABLE
} opcode_t;

/**
 * @struct opstring_t
 * @brief
 */
typedef struct
{
	const char *s;
	opcode_t type;
} opstring_t;

/**
 * @struct expOperation_t
 * @brief
 */
typedef struct
{
	opcode_t type;
	float value;
} expOperation_t;

#define MAX_EXPRESSION_OPS  32

/**
 * @struct expression_t
 * @brief
 */
typedef struct
{
	expOperation_t ops[MAX_EXPRESSION_OPS];
	uint8_t numOps;

	qboolean active;            ///< no parsing problems
} expression_t;

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
	TMOD_NONE = 0,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE,

	TMOD_SCROLL2,
	TMOD_SCALE2,
	TMOD_CENTERSCALE,
	TMOD_SHEAR,
	TMOD_ROTATE2
} texMod_t;

#define MAX_SHADER_DEFORMS  3
#define MAX_SHADER_DEFORM_PARMS (1 + MAX_SHADER_DEFORMS + MAX_SHADER_DEFORMS * 8)

/**
 * @struct deformStage_t
 * @brief
 */
typedef struct
{
	deform_t deformation;           ///< vertex coordinate modification type

	vec3_t moveVector;
	waveForm_t deformationWave;
	float deformationSpread;

	float bulgeWidth;
	float bulgeHeight;
	float bulgeSpeed;

	float flareSize;
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
	mat4_t matrix;  // s' = s * m[0][0] + t * m[1][0] + trans[0]
					// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float scale[2]; // s *= scale[0]
					// t *= scale[1]

	// used for TMOD_SCROLL
	float scroll[2]; // s' = s + scroll[0] * time
					 // t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float rotateSpeed;

	// used by everything else
	expression_t sExp;
	expression_t tExp;
	expression_t rExp;

} texModInfo_t;

#define MAX_IMAGE_ANIMATIONS    16

enum
{
	TB_COLORMAP   = 0,
	TB_DIFFUSEMAP = 0,
	TB_NORMALMAP,
	TB_SPECULARMAP,
	TB_REFLECTIONMAP,
	MAX_TEXTURE_BUNDLES // = 4    If you don't explicitely assign a value here,
						//        the enum will get the correct value automatically.
						//        And if you add a new TB_ type, you wouldn't forget to change this value manually.
};

/**
 * @struct textureBundle_t
 * @brief
 */
typedef struct
{
	uint8_t numImages; // the number of animation images
	double imageAnimationSpeed;
	image_t *image[MAX_IMAGE_ANIMATIONS];

	qboolean isTcGen;
	vec3_t tcGenVectors[2];

	uint8_t numTexMods;
	texModInfo_t *texMods;

	int videoMapHandle;
	qboolean isVideoMap;
} textureBundle_t;

/**
 * @enum stageType_t
 * @brief
 */
typedef enum
{
	// material shader stage types
	ST_COLORMAP = 0,            ///< vanilla Q3A style shader treatening
	ST_DIFFUSEMAP,
	ST_NORMALMAP,
	ST_SPECULARMAP,
	ST_REFLECTIONMAP,           ///< cubeMap based reflection
	ST_REFRACTIONMAP,
	ST_DISPERSIONMAP,
	ST_SKYBOXMAP,
	ST_SCREENMAP,               ///< 2d offscreen or portal rendering
	ST_PORTALMAP,
	ST_HEATHAZEMAP,             ///< heatHaze post process effect
	ST_LIQUIDMAP,

	ST_LIGHTMAP,

	ST_COLLAPSE_lighting_DB,    ///< diffusemap + bumpmap
	ST_COLLAPSE_lighting_DBS,   ///< diffusemap + bumpmap + specularmap
	ST_COLLAPSE_reflection_CB,  ///< color cubemap + bumpmap

	// light shader stage types
	ST_ATTENUATIONMAP_XY,
	ST_ATTENUATIONMAP_Z,
	ST_TCGEN					   ///< tcGen environment reflection
} stageType_t;

/**
 * @enum collapseType_t
 * @brief
 */
typedef enum
{
	COLLAPSE_none = 0,
	COLLAPSE_genericMulti,
	COLLAPSE_lighting_DB,
	COLLAPSE_lighting_DBS,
	COLLAPSE_reflection_CB,
	COLLAPSE_color_lightmap
} collapseType_t;

/**
 * @enum stencilFunc_t
 * @brief StencilFuncs
 */
typedef enum
{
	STF_ALWAYS  = 0x00,
	STF_NEVER   = 0x01,
	STF_LESS    = 0x02,
	STF_LEQUAL  = 0x03,
	STF_GREATER = 0x04,
	STF_GEQUAL  = 0x05,
	STF_EQUAL   = 0x06,
	STF_NEQUAL  = 0x07,
	STF_MASK    = 0x07
} stencilFunc_t;

/**
 * @enum stencilOp_t
 * @brief StencilOps
 */
typedef enum
{
	STO_KEEP    = 0x00,
	STO_ZERO    = 0x01,
	STO_REPLACE = 0x02,
	STO_INVERT  = 0x03,
	STO_INCR    = 0x04,
	STO_DECR    = 0x05,
	STO_MASK    = 0x07
} stencilOp_t;

/**
 * @enum stencilShift_t
 * @brief Shifts
 */
typedef enum
{
	STS_SFAIL = 4,
	STS_ZFAIL = 8,
	STS_ZPASS = 12
} stencilShift_t;

/**
 * @struct stencil_s
 * @typedef stencil_t
 * @brief Shifts
 */
typedef struct stencil_s
{
	short flags;
	byte ref;
	byte mask;
	byte writeMask;
} stencil_t;

/**
 * @struct shaderStage_t
 * @brief Shifts
 */
typedef struct
{
	stageType_t type;

	qboolean active;

	textureBundle_t bundle[MAX_TEXTURE_BUNDLES];

	expression_t ifExp;

	waveForm_t rgbWave;
	colorGen_t rgbGen;
	expression_t rgbExp;
	expression_t redExp;
	expression_t greenExp;
	expression_t blueExp;

	waveForm_t alphaWave;
	alphaGen_t alphaGen;
	expression_t alphaExp;

	expression_t alphaTestExp;

	qboolean tcGen_Environment;
	qboolean tcGen_Lightmap;

	byte constantColor[4];                  ///< for CGEN_CONST and AGEN_CONST

	uint32_t stateBits;                     ///< GLS_xxxx mask

	acff_t adjustColorsForFog;

	stencil_t frontStencil, backStencil;

	qboolean overrideNoPicMip;              ///< for images that must always be full resolution
	qboolean overrideFilterType;            ///< for console fonts, 2D elements, etc.
	filterType_t filterType;
	qboolean overrideWrapType;
	wrapType_t wrapType;

	qboolean uncompressed;
	qboolean highQuality;
	qboolean forceHighQuality;

	qboolean privatePolygonOffset;          ///< set for decals and other items that must be offset
	float privatePolygonOffsetValue;

	expression_t refractionIndexExp;

	expression_t fresnelPowerExp;
	expression_t fresnelScaleExp;
	expression_t fresnelBiasExp;

	expression_t normalScaleExp;

	expression_t etaExp;
	expression_t etaDeltaExp;

	expression_t fogDensityExp;

	expression_t depthScaleExp;

	expression_t deformMagnitudeExp;

	expression_t blurMagnitudeExp;

	expression_t wrapAroundLightingExp;

	float zFadeBounds[2];

	qboolean isDetail;
	qboolean noFog;                         ///< used only for shaders that have fog disabled, so we can enable it for individual stages
	qboolean isFogged;
} shaderStage_t;

struct shaderCommands_s;

#define LIGHTMAP_2D         -4      // shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3      // pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

/**
 * @enum cullType_t
 * @brief
 */
typedef enum
{
	CT_FRONT_SIDED = 0,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

/**
 * @enum fogPass_t
 * @brief
 */
typedef enum
{
	FP_NONE = 0,                ///< surface is translucent and will just be adjusted properly
	FP_EQUAL,                   ///< surface is opaque but possibly alpha tested
	FP_LE                       ///< surface is translucent, but still needs a fog pass (fog surface)
} fogPass_t;

/**
 * @struct skyParms_t
 * @brief
 */
typedef struct
{
	float cloudHeight;
	image_t *outerbox, *innerbox;
} skyParms_t;

/**
 * @struct fogParms_t
 * @brief
 */
typedef struct
{
	vec3_t color;
	float depthForOpaque;
	unsigned colorInt;      ///< in packed byte format
	float density;
	float tcScale;                      ///< texture coordinate vector scales
} fogParms_t;

/**
 * @enum shaderType_t
 * @brief
 */
typedef enum
{
	SHADER_2D = 0 ,             ///< surface material: shader is for 2D rendering
	SHADER_3D_DYNAMIC,          ///< surface material: shader is for cGen diffuseLighting lighting
	SHADER_3D_STATIC,           ///< surface material: pre-lit triangle models
	SHADER_LIGHT                ///< light material: attenuation
} shaderType_t;

/**
 * @struct shader_s
 * @typedef shader_t
 * @brief
 */
typedef struct shader_s
{
	char name[MAX_QPATH];               ///< game path, including extension
	shaderType_t type;

	int index;                          ///< this shader == tr.shaders[index]
	int sortedIndex;                    ///< this shader == tr.sortedShaders[sortedIndex]

	float sort;                         ///< lower numbered shaders draw before higher numbered

	qboolean defaultShader;             ///< we want to return index 0 if the shader failed to
										///< load for some reason, but R_FindShader should
										///< still keep a name allocated for it, so if
										///< something calls RE_RegisterShader again with
										///< the same name, we don't try looking for it again

	qboolean explicitlyDefined;         ///< found in a .shader file
	qboolean createdByGuide;            ///< created using a shader .guide template

	int surfaceFlags;                   ///< if explicitlyDefined, this will have SURF_* flags
	int contentFlags;

	qboolean entityMergable;            ///< merge across entites optimizable (smoke, blood)
	qboolean alphaTest;                 ///< helps merging shadowmap generating surfaces

	qboolean fogVolume;                 ///< surface encapsulates a fog volume
	fogParms_t fogParms;
	fogPass_t fogPass;                  ///< draw a blended pass, possibly with depth test equals
	qboolean noFog;

	qboolean parallax;                  ///< material has normalmaps suited for parallax mapping

	qboolean noShadows;
	qboolean fogLight;
	qboolean blendLight;
	qboolean ambientLight;
	qboolean volumetricLight;
	qboolean translucent;
	qboolean forceOpaque;
	qboolean isSky;
	skyParms_t sky;

	vec4_t distanceCull;                ///< opaque alpha range for foliage (inner, outer, alpha threshold, 1/(outer-inner))

	float portalRange;                  ///< distance to fog out at
	qboolean isPortal;

	collapseType_t collapseType;
	int collapseTextureEnv;             ///< 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t cullType;                ///< CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean polygonOffset;             ///< set for decals and other items that must be offset

	qboolean uncompressed;
	qboolean noPicMip;                  ///< for images that must always be full resolution
	filterType_t filterType;            ///< for console fonts, 2D elements, etc.
	wrapType_t wrapType;

	///< spectrums are used for "invisible writing" that can only be illuminated by a light of matching spectrum
	qboolean spectrum;
	int spectrumValue;

	qboolean interactLight;             ///< this shader can interact with light shaders

	uint8_t numDeforms;
	deformStage_t deforms[MAX_SHADER_DEFORMS];

	uint8_t numStages;
	shaderStage_t *stages[MAX_SHADER_STAGES];

	double clampTime;                                   ///< time this shader is clamped to // FIXME: double for time?
	int timeOffset;                                     ///< current time offset for this shader

	qboolean has_lightmapStage;

	struct shader_s *remappedShader;    ///< current shader this one is remapped too

	struct shader_s *next;
} shader_t;

/**
 * @struct growList_t
 * @brief
 */
typedef struct
{
	qboolean frameMemory;
	int currentElements;
	int maxElements;        ///< will reallocate and move when exceeded
	void **elements;
} growList_t;


enum
{
	ATTR_INDEX_POSITION     = 0,
	ATTR_INDEX_TEXCOORD0    = 1,
	ATTR_INDEX_TEXCOORD1    = 2,
	ATTR_INDEX_TANGENT      = 3,
	ATTR_INDEX_BINORMAL     = 4,
	ATTR_INDEX_NORMAL       = 5,
	ATTR_INDEX_COLOR        = 6,
	// GPU vertex skinning
	ATTR_INDEX_BONE_INDEXES = 7,
	ATTR_INDEX_BONE_WEIGHTS = 8,
	// GPU vertex animations
	ATTR_INDEX_POSITION2    = 9,
	ATTR_INDEX_TANGENT2     = 10,
	ATTR_INDEX_BINORMAL2    = 11,
	ATTR_INDEX_NORMAL2      = 12,

	//ATTR_INDEX_PAINTCOLOR,      ///< for advanced terrain blending
	//ATTR_INDEX_LIGHTDIRECTION,

	ATTR_INDEX_COUNT        = 13
};

// *INDENT-OFF*
enum
{
	GLS_SRCBLEND_ZERO                = (1 << 0),
	GLS_SRCBLEND_ONE                 = (1 << 1),
	GLS_SRCBLEND_DST_COLOR           = (1 << 2),
	GLS_SRCBLEND_ONE_MINUS_DST_COLOR = (1 << 3),
	GLS_SRCBLEND_SRC_ALPHA           = (1 << 4),
	GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA = (1 << 5),
	GLS_SRCBLEND_DST_ALPHA           = (1 << 6),
	GLS_SRCBLEND_ONE_MINUS_DST_ALPHA = (1 << 7),
	GLS_SRCBLEND_ALPHA_SATURATE      = (1 << 8),

	GLS_SRCBLEND_BITS = GLS_SRCBLEND_ZERO
						| GLS_SRCBLEND_ONE
						| GLS_SRCBLEND_DST_COLOR
						| GLS_SRCBLEND_ONE_MINUS_DST_COLOR
						| GLS_SRCBLEND_SRC_ALPHA
						| GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA
						| GLS_SRCBLEND_DST_ALPHA
						| GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
						| GLS_SRCBLEND_ALPHA_SATURATE,

	GLS_DSTBLEND_ZERO                = (1 << 9),
	GLS_DSTBLEND_ONE                 = (1 << 10),
	GLS_DSTBLEND_SRC_COLOR           = (1 << 11),
	GLS_DSTBLEND_ONE_MINUS_SRC_COLOR = (1 << 12),
	GLS_DSTBLEND_SRC_ALPHA           = (1 << 13),
	GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA = (1 << 14),
	GLS_DSTBLEND_DST_ALPHA           = (1 << 15),
	GLS_DSTBLEND_ONE_MINUS_DST_ALPHA = (1 << 16),

	GLS_DSTBLEND_BITS = GLS_DSTBLEND_ZERO
						| GLS_DSTBLEND_ONE
						| GLS_DSTBLEND_SRC_COLOR
						| GLS_DSTBLEND_ONE_MINUS_SRC_COLOR
						| GLS_DSTBLEND_SRC_ALPHA
						| GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
						| GLS_DSTBLEND_DST_ALPHA
						| GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,

	GLS_DEPTHMASK_TRUE = (1 << 17),

	GLS_POLYMODE_LINE = (1 << 18),

	GLS_DEPTHTEST_DISABLE = (1 << 19),

	GLS_DEPTHFUNC_LESS  = (1 << 20),
	GLS_DEPTHFUNC_EQUAL = (1 << 21),

	GLS_DEPTHFUNC_BITS = GLS_DEPTHFUNC_LESS
						 | GLS_DEPTHFUNC_EQUAL,

	GLS_ATEST_GT_0   = (1 << 22),
	GLS_ATEST_LT_128 = (1 << 23),
	GLS_ATEST_GE_128 = (1 << 24),
//	GLS_ATEST_GE_CUSTOM					= (1 << 25),

	GLS_ATEST_BITS = GLS_ATEST_GT_0
					 | GLS_ATEST_LT_128
					 | GLS_ATEST_GE_128,
//											| GLS_ATEST_GT_CUSTOM,

	GLS_REDMASK_FALSE   = (1 << 26),
	GLS_GREENMASK_FALSE = (1 << 27),
	GLS_BLUEMASK_FALSE  = (1 << 28),
	GLS_ALPHAMASK_FALSE = (1 << 29),

	GLS_COLORMASK_BITS = GLS_REDMASK_FALSE
						 | GLS_GREENMASK_FALSE
						 | GLS_BLUEMASK_FALSE
						 | GLS_ALPHAMASK_FALSE,

	GLS_STENCILTEST_ENABLE = (1 << 30),

	GLS_DEFAULT = GLS_DEPTHMASK_TRUE
};
// *INDENT-ON*

/**
 * @enum GLAttrDef
 * @brief
 */
enum GLAttrDef
{
#define ATTR_DECL
#include "tr_glsldef.h"
#undef ATTR_DECL
};

/*
enum
{
	GENERICDEF_USE_DEFORM_VERTEXES  = 0x0001,
	GENERICDEF_USE_TCGEN_AND_TCMOD  = 0x0002,
	GENERICDEF_USE_VERTEX_ANIMATION = 0x0004,
	GENERICDEF_USE_FOG              = 0x0008,
	GENERICDEF_USE_RGBAGEN          = 0x0010,
	GENERICDEF_USE_LIGHTMAP         = 0x0020,
	GENERICDEF_ALL                  = 0x003F,
	GENERICDEF_COUNT                = 0x0040,
};

enum
{
	FOGDEF_USE_DEFORM_VERTEXES  = 0x0001,
	FOGDEF_USE_VERTEX_ANIMATION = 0x0002,
	FOGDEF_ALL                  = 0x0003,
	FOGDEF_COUNT                = 0x0004,
};

enum
{
	DLIGHTDEF_USE_DEFORM_VERTEXES = 0x0001,
	DLIGHTDEF_ALL                 = 0x0001,
	DLIGHTDEF_COUNT               = 0x0002,
};

enum
{
	LIGHTDEF_USE_LIGHTMAP        = 0x0001,
	LIGHTDEF_USE_LIGHT_VECTOR    = 0x0002,
	LIGHTDEF_USE_LIGHT_VERTEX    = 0x0003,
	LIGHTDEF_LIGHTTYPE_MASK      = 0x0003,
	LIGHTDEF_ENTITY              = 0x0004,
	LIGHTDEF_USE_TCGEN_AND_TCMOD = 0x0008,
	LIGHTDEF_USE_NORMALMAP       = 0x0010,
	LIGHTDEF_USE_SPECULARMAP     = 0x0020,
	LIGHTDEF_USE_DELUXEMAP       = 0x0040,
	LIGHTDEF_USE_PARALLAXMAP     = 0x0080,
	LIGHTDEF_USE_SHADOWMAP       = 0x0100,
	LIGHTDEF_ALL                 = 0x01FF,
	LIGHTDEF_COUNT               = 0x0200
};
*/

enum
{
	GLSL_BOOL,
	GLSL_INT,
	GLSL_FLOAT,
	GLSL_FLOAT5,
	GLSL_DOUBLE,
	GLSL_VEC2,
	GLSL_VEC3,
	GLSL_VEC4,
	GLSL_MAT16,
	GLSL_FLOATARR,
	GLSL_VEC4ARR,
	GLSL_MAT16ARR
};

/**
 * @enum EGLCompileMacro
 * @brief
 */
enum EGLCompileMacro
{
#define MACRO_ENUM
#include "tr_glsldef.h"
#undef MACRO_ENUM
	MAX_MACROS
};

/**
 * @enum uniform_t
 * @brief
 */
typedef enum
{
#define UNIFORM_ENUM
#include "tr_glsldef.h"
#undef UNIFORM_ENUM
	UNIFORM_COUNT
} uniform_t;

#define MAX_UNIFORM_VALUES 64

/**
 * @enum texture_def_t
 * @brief
 */
typedef enum
{
#define TEX_DECL
#include "tr_glsldef.h"
#undef TEX_DECL
	TEX_COUNT
} texture_def_t;

/**
 * @struct shaderProgram_s
 * @typedef shaderProgram_t
 * @brief shaderProgram_t represents a pair of one
 * GLSL vertex and one GLSL fragment shader
 */
typedef struct shaderProgram_s
{
	char name[MAX_QPATH];

	GLuint program;
	//uint32_t attribs;                         ///< vertex array attributes

	GLhandleARB vertexShader;
	GLhandleARB fragmentShader;

	// uniform parameters
	GLint uniforms[UNIFORM_COUNT];
	short uniformBufferOffsets[UNIFORM_COUNT];  ///< max 32767/64=511 uniforms
	char *uniformBuffer;

	/// texture binds
	int textureBinds[TEX_COUNT];

	qboolean compiled;
} shaderProgram_t;

/**
 * @struct shaderProgramList_s
 * @typedef shaderProgramList_t
 * @brief
 */
typedef struct shaderProgramList_s
{
	shaderProgram_t *programs;
	shaderProgram_t *current;
	size_t permutations;
	int currentPermutation;
	int currentMacros;
	int macromap[MAX_MACROS];
	int mappedMacros;
} shaderProgramList_t;

/**
 * @struct uniformInfo_s
 * @typedef uniformInfo_t
 * @brief
 */
typedef struct uniformInfo_s
{
	char *name;
	int type;
}uniformInfo_t;

/**
 * @struct uniformValue_s
 * @typedef uniformValue_t
 * @brief
 */
typedef struct uniformValue_s
{
	uniformInfo_t type;
	void *value;
} uniformValue_t;

/**
 * @struct programInfo_s
 * @typedef programInfo_t
 * @brief
 */
typedef struct programInfo_s
{
	char *name;
	char *filename;
	char *fragFilename;
	int macros[MAX_MACROS];
	int numMacros;
	char *extraMacros;
	unsigned int attributes;
	char *vertexLibraries;
	char *fragmentLibraries;
	char *vertexShaderText;
	char *fragmentShaderText;
	uniformValue_t uniformValues[MAX_UNIFORM_VALUES];
	int numUniformValues;
	qboolean compiled;
	unsigned int checkSum;
	shaderProgramList_t *list;
	struct programInfo_s *next;
}programInfo_t;

#define SHADER_PROGRAM_T_OFS(x) ((size_t)&(((shaderProgram_t *)0)->x))

//=================================================================================

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
} decalProjector_t;

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
 * @struct trRefdef_s
 * @brief trRefdef_t holds everything that comes in refdef_t,
 * as well as the locally generated scene information
 */
typedef struct
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];                 ///< transformation matrix

	int time;                           ///< time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;                        ///< RDF_NOWORLDMODEL, etc

	/// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];
	qboolean areamaskModified;          ///< qtrue if areamask changed since last scene

	double floatTime;                    ///< tr.refdef.time / 1000.0

	/// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int numEntities;
	trRefEntity_t *entities;

	int numLights;
	trRefLight_t *lights;

	int num_coronas;
	corona_t *coronas;

	int numPolys;
	struct srfPoly_s *polys;

	int numPolybuffers;
	struct srfPolyBuffer_s *polybuffers;

	int decalBits;                      ///< optimization
	int numDecalProjectors;
	struct decalProjector_s *decalProjectors;

	int numDecals;
	struct srfDecal_s *decals;

	int numDrawSurfs;
	struct drawSurf_s *drawSurfs;

	int numInteractions;
	struct interaction_s *interactions;

	byte *pixelTarget;                  ///< set this to Non Null to copy to a buffer after scene rendering
	int pixelTargetWidth;
	int pixelTargetHeight;

	glfog_t glFog;                      ///< added (needed to pass fog infos into the portal sky scene)
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
 * @struct skinSurface_t
 * @brief skins allow models to be retextured without modifying the model file
 */
typedef struct
{
	char name[MAX_QPATH];
	shader_t *shader;
} skinSurface_t;

#define MAX_PART_MODELS 7

/**
 * @struct skinModel_t
 * @brief
 */
typedef struct
{
	char type[MAX_QPATH];               ///< md3_lower, md3_lbelt, md3_rbelt, etc.
	char model[MAX_QPATH];              ///< lower.md3, belt1.md3, etc.
	int hash;
} skinModel_t;

/**
 * @struct skin_s
 * @typedef skin_t
 * @brief
 */
typedef struct skin_s
{
	char name[MAX_QPATH];               ///< game path, including extension
	int numSurfaces;
	int numModels;
	skinSurface_t *surfaces;            ///< dynamically allocated array of surfaces
	skinModel_t *models[MAX_PART_MODELS];
} skin_t;

//=================================================================================

/**
 * @struct fog_t
 * @brief
 */
typedef struct
{
	int modelNum;                   ///< bsp model the fog belongs to
	int originalBrushNumber;
	vec3_t bounds[2];
	shader_t *shader;               ///< fog shader to get colorInt and tcScale from
	vec4_t color;               ///< in packed byte format
	float tcScale;              ///< texture coordinate vector scales
	fogParms_t fogParms;

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

	vec3_t pvsOrigin;                                   ///< may be different than or.origin for portals

	qboolean isPortal;                                  ///< true if this view is through a portal
	qboolean isMirror;                                  ///< the portal is a mirror, invert the face culling

	int frameSceneNum;                                  ///< copied from tr.frameSceneNum
	int frameCount;                                     ///< copied from tr.frameCount
	int viewCount;                                      ///< copied from tr.viewCount

	cplane_t portalPlane;                               ///< clip anything behind this if mirroring
	int viewportX, viewportY, viewportWidth, viewportHeight;
	vec4_t viewportVerts[4];                            ///< for immediate 2D quad rendering

	float fovX, fovY;
	mat4_t projectionMatrix;
	mat4_t unprojectionMatrix;                          ///< transform pixel window space -> world space

	float parallelSplitDistances[MAX_SHADOWMAPS + 1];   ///< distances in camera space

	frustum_t frustums[MAX_SHADOWMAPS + 1];             ///< first frustum is the default one with complete zNear - zFar range
	// and the other ones are for PSSM

	vec3_t visBounds[2];
	float zNear;
	float zFar;
	glfog_t glFog; //for tr_fog
	int numDrawSurfs;
	struct drawSurf_s *drawSurfs;
	int numInteractions;
	struct interaction_s *interactions;
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
 * @warning Any changes in surfaceType must be mirrored in rb_surfaceTable[]
 */
typedef enum
{
	SF_BAD = 0,
	SF_SKIP,                    ///< ignore

	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_FOLIAGE,

	SF_POLY,
	SF_POLYBUFFER,
	SF_DECAL,                   ///< decal surfaces

	SF_MDV,
	SF_MDM,
	SF_MD5,

	SF_FLARE,
	SF_ENTITY,                  ///< beams, rails, lightning, etc that can be determined by entity

	SF_VBO_MESH,
	SF_VBO_MD5MESH,
	SF_VBO_MDMMESH,
	SF_VBO_MDVMESH,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff         ///< ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

/**
 * @struct drawSurf_s
 * @typedef drawSurf_t
 * @brief
 */
typedef struct drawSurf_s
{
	trRefEntity_t *entity;
	shader_t *shader;
	int16_t lightmapNum;
	int16_t fogNum;

	surfaceType_t *surface;     ///< any of surface*_t
} drawSurf_t;

/**
 * @enum interactionType_t
 * @brief
 */
typedef enum
{
	IA_DEFAULT = 0,                 ///< lighting and shadowing
	IA_SHADOWONLY,
	IA_LIGHTONLY
} interactionType_t;

/**
 * @struct interactionCache_s
 * @typedef interactionCache_t
 * @brief An interactionCache is a node between a light and a precached world surface
 */
typedef struct interactionCache_s
{
	interactionType_t type;

	struct bspSurface_s *surface;

	byte cubeSideBits;
	qboolean redundant;
	qboolean mergedIntoVBO;

	struct interactionCache_s *next;
} interactionCache_t;

/**
 * @struct interactionVBO_s
 * @typedef interactionVBO_t
 * @brief
 */
typedef struct interactionVBO_s
{
	interactionType_t type;

	byte cubeSideBits;

	struct shader_s *shader;
	struct srfVBOMesh_s *vboLightMesh;
	struct srfVBOMesh_s *vboShadowMesh;

	struct interactionVBO_s *next;
} interactionVBO_t;

/**
 * @struct interaction_s
 * @typedef interaction_t
 * @brief An interaction is a node between a light and any surface
 */
typedef struct interaction_s
{
	interactionType_t type;

	trRefLight_t *light;

	trRefEntity_t *entity;
	surfaceType_t *surface;             ///< any of surface*_t
	shader_t *surfaceShader;

	byte cubeSideBits;

	int16_t scissorX, scissorY, scissorWidth, scissorHeight;

	float depthNear;                    ///< for GL_EXT_depth_bounds_test
	float depthFar;
	qboolean noDepthBoundsTest;

	uint32_t occlusionQuerySamples;     ///< visible fragment count
	qboolean noOcclusionQueries;

	struct interaction_s *next;
} interaction_t;

/**
 * @struct shadowState_t
 * @brief
 */
typedef struct
{
	int numDegenerated;                 ///< number of bad triangles
	qboolean degenerated[SHADER_MAX_TRIANGLES];

	qboolean facing[SHADER_MAX_TRIANGLES];
	int numFacing;                      ///< number of triangles facing the light origin

	int numIndexes;
	int indexes[SHADER_MAX_INDEXES];
} shadowState_t;

extern shadowState_t shadowState;

#define MAX_FACE_POINTS     64

#define MAX_PATCH_SIZE      64  ///< max dimensions of a patch mesh in map file
#define MAX_GRID_SIZE       65  ///< max dimensions of a grid mesh in memory

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
	int16_t numVerts;
	int16_t fogIndex;
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
	int16_t fogIndex;
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
 * @struct srfVert_t
 * @brief
 */
typedef struct
{
	vec3_t xyz;
	vec2_t st;
	vec2_t lightmap;
	vec3_t tangent;
	vec3_t binormal;
	vec3_t normal;
	vec4_t paintColor;
	vec4_t lightColor;
	vec3_t lightDirection;
} srfVert_t;

/**
 * @struct srfTriangle_t
 * @brief
 */
typedef struct
{
	int indexes[3];
	vec4_t planeNormal; // unused
	qboolean facingLight;
	qboolean degenerated;
} srfTriangle_t;

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

	// dynamic lighting information
	//int             dlightBits;
}
srfGeneric_t;

/**
 * @struct srfGridMesh_s
 * @typedef srfGridMesh_t
 * @brief
 */
typedef struct srfGridMesh_s
{
	// srfGeneric_t BEGIN
	surfaceType_t surfaceType;

	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	// srfGeneric_t END

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t lodOrigin;
	float lodRadius;
	int lodFixed;
	int lodStitched;

	// triangle definitions
	int width, height;
	float *widthLodError;
	float *heightLodError;

	int numTriangles;
	srfTriangle_t *triangles;

	int numVerts;
	srfVert_t *verts;

	// BSP VBO offsets
	int firstTriangle;

	// static render data
	VBO_t *vbo;                 ///< points to bsp model VBO
	IBO_t *ibo;
} srfGridMesh_t;

/**
 * @struct srfSurfaceFace_t
 * @brief
 */
typedef struct
{
	// srfGeneric_t BEGIN
	surfaceType_t surfaceType;

	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	// srfGeneric_t END

	// triangle definitions
	int numTriangles;
	srfTriangle_t *triangles;

	int numVerts;
	srfVert_t *verts;

	// BSP VBO offsets
	//int firstVert;
	int firstTriangle;

	// static render data
	VBO_t *vbo;                 // points to bsp model VBO
	IBO_t *ibo;
} srfSurfaceFace_t;

/**
 * @struct srfTriangles_t
 * @brief misc_models in maps are turned into direct geometry by xmap
 */
typedef struct
{
	// srfGeneric_t BEGIN
	surfaceType_t surfaceType;

	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	// srfGeneric_t END

	// triangle definitions
	int numTriangles;
	srfTriangle_t *triangles;

	int numVerts;
	srfVert_t *verts;

	// BSP VBO offsets
	//int firstVert;
	int firstTriangle;

	// static render data
	VBO_t *vbo;                 ///< points to bsp model VBO
	IBO_t *ibo;
} srfTriangles_t;

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
	// srfGeneric_t BEGIN

	surfaceType_t surfaceType;

	vec3_t bounds[2];
	vec3_t origin;
	float radius;
	cplane_t plane;

	// srfGeneric_t END

	// dynamic lighting information
	//int dlightBits;

	// triangle definitions
	int numTriangles;
	srfTriangle_t *triangles;

	int numVerts;
	srfVert_t *verts;

	// origins
	int numInstances;
	foliageInstance_t *instances;
 
	// BSP VBO offsets
	int firstTriangle;

	// static render data
	VBO_t *vbo;
	IBO_t *ibo;
} srfFoliage_t;

/**
 * @struct srfVBOMesh_s
 * @typedef srfVBOMesh_t
 * @brief
 */
typedef struct srfVBOMesh_s
{
	surfaceType_t surfaceType;

	struct shader_s *shader;    ///< @todo FIXME move this to somewhere else
	int lightmapNum;            ///< @todo FIXME get rid of this by merging all lightmaps at level load

	// culling information
	vec3_t bounds[2];

	// backEnd stats
	int numIndexes;
	int numVerts;

	// static render data
	VBO_t *vbo;
	IBO_t *ibo;
} srfVBOMesh_t;

/**
 * @struct srfVBOMD5Mesh_s
 * @typedef srfVBOMD5Mesh_t
 * @brief
 */
typedef struct srfVBOMD5Mesh_s
{
	surfaceType_t surfaceType;

	struct md5Model_s *md5Model;
	struct shader_s *shader;        ///< @todo FIXME move this to somewhere else

	int skinIndex;

	int numBoneRemap;
	int boneRemap[MAX_BONES];
	int boneRemapInverse[MAX_BONES];

	// backEnd stats
	int numIndexes;
	int numVerts;

	// static render data
	VBO_t *vbo;
	IBO_t *ibo;
} srfVBOMD5Mesh_t;

/**
 * @struct srfVBOMDMMesh_s
 * @typedef srfVBOMDMMesh_t
 * @brief
 */
typedef struct srfVBOMDMMesh_s
{
	surfaceType_t surfaceType;

	struct mdmModel_s *mdmModel;
	struct mdmSurfaceIntern_s *mdmSurface;
	struct shader_s *shader;    ///< @todo FIXME move this to somewhere else

	int skinIndex;

	int numBoneRemap;
	int boneRemap[MAX_BONES];
	int boneRemapInverse[MAX_BONES];

	// backEnd stats
	int numIndexes;
	int numVerts;

	// static render data
	VBO_t *vbo;
	IBO_t *ibo[MD3_MAX_LODS];
} srfVBOMDMMesh_t;

/**
 * @struct srfVBOMDVMesh_s
 * @typedef srfVBOMDVMesh_t
 * @brief
 */
typedef struct srfVBOMDVMesh_s
{
	surfaceType_t surfaceType;

	struct mdvModel_s *mdvModel;
	struct mdvSurface_s *mdvSurface;

	// backEnd stats
	int numIndexes;
	int numVerts;

	// static render data
	VBO_t *vbo;
	IBO_t *ibo;
} srfVBOMDVMesh_t;

extern void(*rb_surfaceTable[SF_NUM_SURFACE_TYPES]) (void *);

/*
==============================================================================
BRUSH MODELS - in memory representation
==============================================================================
*/

/**
 * @struct bspSurface_s
 * @typedef bspSurface_t
 * @brief
 */
typedef struct bspSurface_s
{
	int viewCount;              ///< if == tr.viewCount, already added
	int lightCount;
	struct shader_s *shader;
	int16_t lightmapNum;        ///< -1 = no lightmap
	int16_t fogIndex;

	surfaceType_t *data;        ///< any of srf*_t
} bspSurface_t;

/**
 * @struct decal_s
 * @typedef decal_t
 * @brief bsp model decal surfaces
 */
typedef struct decal_s
{
	bspSurface_t *parent;
	shader_t *shader;
	float fadeStartTime, fadeEndTime;
	int16_t fogIndex;
	int numVerts;
	polyVert_t verts[MAX_DECAL_VERTS];
	int projectorNum;
	int frameAdded;         ///< need to keep decal for at least one frame so we know not to reproject it in later views
}
decal_t;

#define CONTENTS_NODE       -1

/**
 * @struct bspNode_s
 * @typedef bspNode_t
 * @brief
 */
typedef struct bspNode_s
{
	// common with leaf and node
	int contents;                   ///< -1 for nodes, to differentiate from leafs
	int visCounts[MAX_VISCOUNTS];   ///< node needs to be traversed if current
	int lightCount;
	vec3_t mins, maxs;              ///< for bounding box culling
	vec3_t surfMins, surfMaxs;      ///< bounding box including surfaces
	vec3_t origin;                  ///< center of the bounding box
	struct bspNode_s *parent;

	qboolean visible[MAX_VIEWS];
	int lastVisited[MAX_VIEWS];
	int lastQueried[MAX_VIEWS];
	qboolean issueOcclusionQuery[MAX_VIEWS];

	link_t visChain;                    ///< updated every visit
	link_t occlusionQuery;              ///< updated every visit
	link_t occlusionQuery2;             ///< updated every visit
	link_t multiQuery;                  ///< CHC++: list of all nodes that are used by the same occlusion query

	VBO_t *volumeVBO;
	IBO_t *volumeIBO;
	int volumeVerts;
	int volumeIndexes;

	uint32_t occlusionQueryObjects[MAX_VIEWS];
	int occlusionQuerySamples[MAX_VIEWS];       ///< visible fragment count
	int occlusionQueryNumbers[MAX_VIEWS];       ///< for debugging

	// node specific
	cplane_t *plane;
	struct bspNode_s *children[2];

	// leaf specific
	int cluster;
	int area;

	int numMarkSurfaces;
	bspSurface_t **markSurfaces;
} bspNode_t;

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
/**
 * @struct bspCluster_s
 * @brief
 */
typedef struct
{
	int numMarkSurfaces;
	bspSurface_t **markSurfaces;

	vec3_t origin;              // used for cubemaps
} bspCluster_t;
#endif

/*
 * @struct bspArea_t
 * @brief
 *
 * @note Unused
typedef struct
{
	int             numMarkSurfaces;
	bspSurface_t  **markSurfaces;

	int             numVBOSurfaces;
	srfVBOMesh_t  **vboSurfaces;
} bspArea_t;
*/

/*
 * @struct bspAreaPortal_t
 * @brief
 *
 * @note Unused
typedef struct
{
	int             areas[2];

	vec3_t          points[4];
} bspAreaPortal_t;
*/

/**
 * @struct bspModel_t
 * @brief
 */
typedef struct
{
	vec3_t bounds[2];           ///< for culling

	uint32_t numSurfaces;
	bspSurface_t *firstSurface;

	uint32_t numVBOSurfaces;
	srfVBOMesh_t **vboSurfaces;

	//decals
	decal_t *decals;

	// for fog volumes
	int firstBrush;
	int numBrushes;
	orientation_t orientation;

} bspModel_t;

/**
 * @struct bspGridPoint_t
 * @brief
 */
typedef struct
{
	vec3_t origin;
	vec4_t ambientColor;
	vec4_t directedColor;
	vec3_t direction;
} bspGridPoint_t;

// optimization
#define WORLD_MAX_SKY_NODES 32

/**
 * @struct world_t
 * @brief
 */
typedef struct
{
	char name[MAX_QPATH];                   ///< ie: maps/tim_dm2.bsp
	char baseName[MAX_QPATH];               ///< ie: tim_dm2

	uint32_t dataSize;

	int numShaders;
	dshader_t *shaders;
	
	int numModels;
	bspModel_t *models;

	int numplanes;
	cplane_t *planes;

	int numnodes;                           ///< includes leafs
	int numDecisionNodes;
	bspNode_t *nodes;

	int numSkyNodes;
	bspNode_t **skyNodes;                   ///< don't walk the entire bsp when rendering sky

	int numVerts;
	srfVert_t *verts;
	int redundantVertsCalculationNeeded;
	int *redundantLightVerts;               ///< util to optimize IBOs
	int *redundantShadowVerts;
	int *redundantShadowAlphaTestVerts;
	VBO_t *vbo;
	IBO_t *ibo;

	int numTriangles;
	srfTriangle_t *triangles;

	//int             numAreas;
	//bspArea_t      *areas;

	//int             numAreaPortals;
	//bspAreaPortal_t *areaPortals;

	int numWorldSurfaces;

	int numSurfaces;
	bspSurface_t *surfaces;

	int numMarkSurfaces;
	bspSurface_t **markSurfaces;

	int numFogs;
	fog_t *fogs;

	int globalFog;                          ///< index of global fog
	vec4_t globalOriginalFog;               ///< to be able to restore original global fog
	vec4_t globalTransStartFog;             ///< start fog for switch fog transition
	vec4_t globalTransEndFog;               ///< end fog for switch fog transition
	int globalFogTransStartTime;
	int globalFogTransEndTime;

	vec3_t lightGridOrigin;
	vec3_t lightGridSize;
	vec3_t lightGridInverseSize;
	int lightGridBounds[3];
	bspGridPoint_t *lightGridData;
	int numLightGridPoints;

	int numLights;
	trRefLight_t *lights;

	int numInteractions;
	interactionCache_t **interactions;

	int numClusters;
#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	bspCluster_t *clusters;
#endif
	int clusterBytes;
	const byte *vis;                        ///< may be passed in by CM_LoadMap to save space
	byte *novis;                            ///< clusterBytes of 0xff

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	int numClusterVBOSurfaces[MAX_VISCOUNTS];
	growList_t clusterVBOSurfaces[MAX_VISCOUNTS];       ///< updated every time when changing the view cluster
#endif

	char *entityString;
	char *entityParsePoint;

	qboolean hasSkyboxPortal;
} world_t;

typedef enum
{
	FLAGS_SMOOTH_TRISURF = BIT(0),
	FLAGS_SMOOTH_MESH    = BIT(1),
	FLAGS_SMOOTH_MD3     = BIT(2),
	FLAGS_SMOOTH_MD5     = BIT(3),
	FLAGS_SMOOTH_MDC     = BIT(4),
	FLAGS_SMOOTH_MDM     = BIT(5),
	FLAGS_SMOOTH_PSK     = BIT(6)
} smooth_flags;

/*
==============================================================================
MDV MODELS - meta format for vertex animation models like .md2, .md3, .mdc
==============================================================================
*/

/**
 * @struct mdvFrame_t
 * @brief
 */
typedef struct
{
	float bounds[2][3];
	float localOrigin[3];
	float radius;
} mdvFrame_t;

/**
 * @struct mdvTag_t
 * @brief
 */
typedef struct
{
	float origin[3];
	float axis[3][3];
} mdvTag_t;

/**
 * @struct mdvTagName_t
 * @brief
 */
typedef struct
{
	char name[MAX_QPATH];   ///< tag name
} mdvTagName_t;

/**
 * @struct mdvXyz_t
 * @brief
 */
typedef struct
{
	vec3_t xyz;
} mdvXyz_t;

/**
 * @struct mdvNormTanBi_t
 * @brief
 */
typedef struct
{
	vec3_t normal;
	vec3_t tangent;
	vec3_t binormal;
} mdvNormTanBi_t;

/**
 * @struct mdvSt_t
 * @brief
 */
typedef struct
{
	float st[2];
} mdvSt_t;

/**
 * @struct mdvSurface_s
 * @typedef mdvSurface_t
 * @brief
 */
typedef struct mdvSurface_s
{
	surfaceType_t surfaceType;

	char name[MAX_QPATH];       ///< polyset name

	shader_t *shader;

	int numVerts;
	mdvXyz_t *verts;
	mdvSt_t *st;

	int numTriangles;
	srfTriangle_t *triangles;

	struct mdvModel_s *model;
} mdvSurface_t;

/**
 * @struct mdvModel_s
 * @typedef mdvModel_t
 * @brief
 */
typedef struct mdvModel_s
{
	int numFrames;
	mdvFrame_t *frames;

	int numTags;
	mdvTag_t *tags;
	mdvTagName_t *tagNames;

	int numSurfaces;
	mdvSurface_t *surfaces;

	int numVBOSurfaces;
	srfVBOMDVMesh_t **vboSurfaces;

	int numSkins;
} mdvModel_t;

/*
==============================================================================
MD5 MODELS - in memory representation
==============================================================================
*/
#define MD5_IDENTSTRING     "MD5Version"
#define MD5_VERSION         10

/**
 * @struct md5Weight_t
 * @brief
 */
typedef struct
{
	uint8_t boneIndex;          ///< these are indexes into the boneReferences,
	float boneWeight;           ///< not the global per-frame bone list
	vec3_t offset;
} md5Weight_t;

/**
 * @struct md5Vertex_t
 * @brief
 */
typedef struct
{
	vec3_t position;
	vec2_t texCoords;
	vec3_t tangent;
	vec3_t binormal;
	vec3_t normal;

	uint32_t firstWeight;
	uint16_t numWeights;
	md5Weight_t **weights;
} md5Vertex_t;

/*
 * @struct md5Triangle_t
 * @brief
 *
 * @note Unused
typedef struct
{
	int             indexes[3];
	int             neighbors[3];
} md5Triangle_t;
*/

/**
 * @struct md5Surface_t
 * @brief
 */
typedef struct
{
	surfaceType_t surfaceType;

	//char            name[MAX_QPATH];  ///< polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                    ///< for in-game use

	uint32_t numVerts;
	md5Vertex_t *verts;

	uint32_t numTriangles;
	srfTriangle_t *triangles;

	uint32_t numWeights;
	md5Weight_t *weights;

	struct md5Model_s *model;
} md5Surface_t;

/**
 * @struct md5Bone_t
 * @brief
 */
typedef struct
{
	char name[MAX_QPATH];
	int8_t parentIndex;         ///< parent index (-1 if root)
	vec3_t origin;
	quat_t rotation;
	mat4_t inverseTransform;    ///< full inverse for tangent space transformation
} md5Bone_t;

/**
 * @struct md5Model_s
 * @typedef md5Model_t
 * @brief
 */
typedef struct md5Model_s
{
	uint16_t numBones;
	md5Bone_t *bones;

	uint16_t numSurfaces;
	md5Surface_t *surfaces;

	uint16_t numVBOSurfaces;
	srfVBOMD5Mesh_t **vboSurfaces;

	vec3_t bounds[2];
} md5Model_t;

/**
 * @struct mdmTagIntern_t
 * @brief
 */
typedef struct
{
	char name[64];              ///< name of tag
	vec3_t axis[3];

	int boneIndex;
	vec3_t offset;

	int numBoneReferences;
	int *boneReferences;
} mdmTagIntern_t;

/**
 * @struct mdmSurfaceIntern_s
 * @typedef mdmSurfaceIntern_t
 * @brief
 */
typedef struct mdmSurfaceIntern_s
{
	surfaceType_t surfaceType;

	char name[MAX_QPATH];           ///< polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                ///< for in-game use

	int minLod;

	uint32_t numVerts;
	md5Vertex_t *verts;

	uint32_t numTriangles;
	srfTriangle_t *triangles;

	//uint32_t        numWeights;
	//md5Weight_t    *weights;

	int numBoneReferences;
	int *boneReferences;

	int32_t *collapseMap;           ///< numVerts many

	struct mdmModel_s *model;
} mdmSurfaceIntern_t;

/**
 * @struct mdmModel_s
 * @typedef mdmModel_t
 * @brief
 */
typedef struct mdmModel_s
{
	//uint16_t        numBones;
	//md5Bone_t      *bones;

	float lodScale;
	float lodBias;

	uint16_t numTags;
	mdmTagIntern_t *tags;

	uint16_t numSurfaces;
	mdmSurfaceIntern_t *surfaces;

	uint16_t numVBOSurfaces;
	srfVBOMDMMesh_t **vboSurfaces;

	int numBoneReferences;
	int32_t *boneReferences;

	vec3_t bounds[2];
} mdmModel_t;

extern const float mdmLODResolutions[MD3_MAX_LODS];

/**
 * @enum animType_t
 * @brief
 */
typedef enum
{
	AT_BAD = 0,
	AT_MD5,
	AT_PSA
} animType_t;

enum
{
	COMPONENT_BIT_TX = 1 << 0,
	COMPONENT_BIT_TY = 1 << 1,
	COMPONENT_BIT_TZ = 1 << 2,
	COMPONENT_BIT_QX = 1 << 3,
	COMPONENT_BIT_QY = 1 << 4,
	COMPONENT_BIT_QZ = 1 << 5
};

/**
 * @struct md5Channel_t
 * @brief
 */
typedef struct
{
	char name[MAX_QPATH];
	int8_t parentIndex;

	uint8_t componentsBits;         ///< e.g. (COMPONENT_BIT_TX | COMPONENT_BIT_TY | COMPONENT_BIT_TZ)
	uint16_t componentsOffset;

	vec3_t baseOrigin;
	quat_t baseQuat;
} md5Channel_t;

/**
 * @struct md5Frame_t
 * @brief
 */
typedef struct
{
	vec3_t bounds[2];           ///< bounds of all surfaces of all LOD's for this frame
	float *components;          ///< numAnimatedComponents many
} md5Frame_t;

/**
 * @struct md5Animation_s
 * @typedef md5Animation_t
 * @brief
 */
typedef struct md5Animation_s
{
	uint16_t numFrames;
	md5Frame_t *frames;

	uint8_t numChannels;            ///< same as numBones in model
	md5Channel_t *channels;

	int16_t frameRate;

	uint32_t numAnimatedComponents;
} md5Animation_t;

/**
 * @struct psaAnimation_t
 * @brief
 */
typedef struct
{
	axAnimationInfo_t info;

	int numBones;
	axReferenceBone_t *bones;

	int numKeys;
	axAnimationKey_t *keys;
} psaAnimation_t;

/**
 * @struct skelAnimation_t
 * @brief
 */
typedef struct
{
	char name[MAX_QPATH];       ///< game path, including extension
	animType_t type;
	int index;                  ///< anim = tr.animations[anim->index]

	md5Animation_t *md5;
	psaAnimation_t *psa;

} skelAnimation_t;

/**
 * @struct skelTriangle_t
 * @brief
 */
typedef struct
{
	int indexes[3];
	md5Vertex_t *vertexes[3];
	qboolean referenced;
} skelTriangle_t;

//======================================================================

/**
 * @enum modtype_t
 * @brief
 */
typedef enum
{
	MOD_BAD = 0,
	MOD_BSP,
	MOD_MESH,
	MOD_MDM,
	MOD_MDX,
	MOD_MD5
} modtype_t;

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
	bspModel_t *bsp;                ///< only if type == MOD_BSP
	mdvModel_t *mdv[MD3_MAX_LODS];  ///< only if type == MOD_MESH
	mdmModel_t *mdm;                ///< only if type == MOD_MDM
	mdxHeader_t *mdx;               ///< only if type == MOD_MDX
	md5Model_t *md5;                ///< only if type == MOD_MD5

	int numLods;
} model_t;

void R_ModelInit(void);
model_t *R_GetModelByHandle(qhandle_t hModel);

int RE_LerpTagQ3A(orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagNameIn);
int RE_LerpTagET(orientation_t *tag, const refEntity_t *refent, const char *tagNameIn, int startIndex);

int RE_BoneIndex(qhandle_t hModel, const char *boneName);

void R_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);

void R_Modellist_f(void);

//====================================================
extern refimport_t ri;

#define MAX_MOD_KNOWN           2048
#define MAX_ANIMATIONFILES      4096

#define MAX_LIGHTMAPS           256
#define MAX_SKINS               1024

#define MAX_DRAWSURFS           0x10000
#define DRAWSURF_MASK           (MAX_DRAWSURFS - 1)

#define MAX_INTERACTIONS        MAX_DRAWSURFS * 8
#define INTERACTION_MASK        (MAX_INTERACTIONS - 1)

extern int gl_filter_min, gl_filter_max;

/**
 * @struct frontEndCounters_t
 * @brief
 */
typedef struct
{
	int c_sphere_cull_in, c_sphere_cull_out;
	int c_plane_cull_in, c_plane_cull_out;

	int c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int c_sphere_cull_mdx_in, c_sphere_cull_mdx_clip, c_sphere_cull_mdx_out;
	int c_box_cull_mdx_in, c_box_cull_mdx_clip, c_box_cull_mdx_out;
	int c_box_cull_md5_in, c_box_cull_md5_clip, c_box_cull_md5_out;
	int c_box_cull_light_in, c_box_cull_light_clip, c_box_cull_light_out;
	int c_pvs_cull_light_out;

	int c_pyramidTests;
	int c_pyramid_cull_ent_in, c_pyramid_cull_ent_clip, c_pyramid_cull_ent_out;

	int c_nodes;
	int c_leafs;

	int c_slights;
	int c_slightSurfaces;
	int c_slightInteractions;

	int c_dlights;
	int c_dlightSurfaces;
	int c_dlightSurfacesCulled;
	int c_dlightInteractions;

	int c_depthBoundsTests, c_depthBoundsTestsRejected;

	int c_occlusionQueries;
	int c_occlusionQueriesMulti;
	int c_occlusionQueriesSaved;
	int c_CHCTime;

	// debugging FIXME: debug only
	int c_decalProjectors, c_decalTestSurfaces, c_decalClipSurfaces, c_decalSurfaces, c_decalSurfacesCreated;
} frontEndCounters_t;

#define FOG_TABLE_SIZE      256

#define DEFAULT_FOG_EXP_DENSITY         0.5

#define FUNCTABLE_SIZE      4096    ///< % 1024
#define FUNCTABLE_SIZE2     12      ///< % 10
#define FUNCTABLE_MASK      (FUNCTABLE_SIZE - 1)

#define MAX_GLSTACK         5

/**
 * @struct glstate_t
 * @brief
 *
 * @warning The renderer front end should never modify glState_t
 */
typedef struct
{
	int blendSrc, blendDst;
	float clearColorRed, clearColorGreen, clearColorBlue, clearColorAlpha;
	double clearDepth;
	int clearStencil;
	int colorMaskRed, colorMaskGreen, colorMaskBlue, colorMaskAlpha;
	int cullFace;
	int depthFunc;
	int depthMask;
	int drawBuffer;
	int frontFace;
	int polygonFace, polygonMode;
	int scissorX, scissorY, scissorWidth, scissorHeight;
	int viewportX, viewportY, viewportWidth, viewportHeight;
	float polygonOffsetFactor, polygonOffsetUnits;

	int currenttextures[32];
	int currenttmu;
	//mat4_t        textureMatrix[32];

	int stackIndex;
	//mat4_t        modelMatrix[MAX_GLSTACK];
	//mat4_t        viewMatrix[MAX_GLSTACK];
	mat4_t modelViewMatrix[MAX_GLSTACK];
	mat4_t projectionMatrix[MAX_GLSTACK];
	mat4_t modelViewProjectionMatrix[MAX_GLSTACK];

	qboolean finishCalled;
	int faceCulling;                            ///< @todo FIXME redundant cullFace
	uint32_t glStateBits;
	uint32_t vertexAttribsState;
	uint32_t vertexAttribPointersSet;
	float vertexAttribsInterpolation;           ///< 0 = no interpolation, 1 = final position
	uint32_t vertexAttribsNewFrame;             ///< offset for VBO vertex animations
	uint32_t vertexAttribsOldFrame;             ///< offset for VBO vertex animations
	shaderProgram_t *currentProgram;
	FBO_t *currentFBO;
	VBO_t *currentVBO;
	IBO_t *currentIBO;
} glstate_t;

/**
 * @struct backEndCounters_t
 * @brief
 */
typedef struct
{
	int c_views;
	int c_portals;
	int c_batches;
	int c_surfaces;
	int c_vertexes;
	int c_indexes;
	int c_drawElements;
	float c_overDraw;
	int c_vboVertexBuffers;
	int c_vboIndexBuffers;
	int c_vboVertexes;
	int c_vboIndexes;

	int c_fogSurfaces;
	int c_fogBatches;

	int c_flareAdds;
	int c_flareTests;
	int c_flareRenders;

	int c_occlusionQueries;
	int c_occlusionQueriesMulti;
	int c_occlusionQueriesSaved;
	int c_occlusionQueriesAvailable;
	int c_occlusionQueriesLightsCulled;
	int c_occlusionQueriesEntitiesCulled;
	int c_occlusionQueriesLeafsCulled;
	int c_occlusionQueriesInteractionsCulled;
	int c_occlusionQueriesResponseTime;
	int c_occlusionQueriesFetchTime;

	int c_forwardAmbientTime;
	int c_forwardLightingTime;
	int c_forwardTranslucentTime;

	int c_multiDrawElements;
	int c_multiDrawPrimitives;
	int c_multiVboIndexes;

	int c_glslShaderBinds;

	int msec;                   ///< total msec for backend run
} backEndCounters_t;

/**
 * @struct backEndState_t
 * @brief All state modified by the back end is seperated
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
	trRefLight_t *currentLight;     ///< only used when lighting interactions
	qboolean skyRenderedThisView;   ///< flag for drawing sun

	float hdrAverageLuminance;
	float hdrMaxLuminance;
	float hdrTime;
	float hdrKey;

	qboolean projection2D;          ///< if qtrue, drawstretchpic doesn't need to change modes
	vec4_t color2D;
	qboolean vertexes2D;            ///< shader needs to be finished
	trRefEntity_t entity2D;         ///< currentEntity will point at this when doing 2D rendering
} backEndState_t;

/**
 * @struct cubemapProbe_t
 * @brief
 */
typedef struct
{
	vec3_t origin;
	image_t *cubemap;
} cubemapProbe_t;

/**
 * @struct trGlobals_t
 * @brief Most renderer globals are defined here.
 * backend functions should never modify any of these fields,
 * but may read fields that aren't dynamically modified
 * by the frontend.
 */
typedef struct
{
	qboolean registered;                    ///< cleared at shutdown, set at beginRegistration
	qboolean setfog;
	int visIndex;
	int visClusters[MAX_VISCOUNTS];
	int visCounts[MAX_VISCOUNTS];           ///< incremented every time a new vis cluster is entered

	link_t traversalStack;
	link_t occlusionQueryQueue;
	link_t occlusionQueryList;

	int frameCount;                         ///< incremented every frame
	int sceneCount;                         ///< incremented every scene
	int viewCount;                          ///< incremented every view (twice a scene if portaled)
	int viewCountNoReset;
	int lightCount;                         ///< incremented every time a dlight traverses the world
											///< and every R_MarkFragments call

	int frameSceneNum;                      ///< zeroed at RE_BeginFrame

	qboolean worldMapLoaded;
	//qboolean worldDeluxeMapping;
	qboolean worldHDR_RGBE;
	world_t *world;

	const byte *externalVisData;            ///< from RE_SetWorldVisData, shared with CM_Load

	image_t *defaultImage;
	image_t *scratchImage[32];
	image_t *fogImage;
	image_t *quadraticImage;
	image_t *whiteImage;                    ///< full of 0xff
	image_t *blackImage;                    ///< full of 0x0
	image_t *redImage;
	image_t *greenImage;
	image_t *blueImage;
	image_t *flatImage;                     ///< use this as default normalmap
	//image_t *randomNormalsImage;
	image_t *noFalloffImage;
	image_t *attenuationXYImage;
	image_t *blackCubeImage;
	image_t *whiteCubeImage;
	image_t *autoCubeImage;                 ///< special pointer to the nearest cubemap probe

	image_t *contrastRenderFBOImage;
	image_t *bloomRenderFBOImage[2];
	image_t *currentRenderImage;
	image_t *depthRenderImage;
	image_t *portalRenderImage;

	image_t *deferredDiffuseFBOImage;
	image_t *deferredNormalFBOImage;
	image_t *deferredSpecularFBOImage;
	image_t *deferredRenderFBOImage;
	image_t *lightRenderFBOImage;
	image_t *occlusionRenderFBOImage;
	image_t *depthToColorBackFacesFBOImage;
	image_t *depthToColorFrontFacesFBOImage;
	image_t *downScaleFBOImage_quarter;
	image_t *downScaleFBOImage_64x64;
	//image_t        *downScaleFBOImage_16x16;
	//image_t        *downScaleFBOImage_4x4;
	//image_t        *downScaleFBOImage_1x1;
	image_t *shadowMapFBOImage[MAX_SHADOWMAPS];
	image_t *shadowCubeFBOImage[MAX_SHADOWMAPS];
	image_t *sunShadowMapFBOImage[MAX_SHADOWMAPS];

	// external images
	image_t *charsetImage;
	image_t *grainImage;
	image_t *vignetteImage;

	// framebuffer objects
	FBO_t *geometricRenderFBO;              ///< is the G-Buffer for deferred shading
	FBO_t *lightRenderFBO;                  ///< is the light buffer which contains all light properties of the light pre pass
	FBO_t *deferredRenderFBO;               ///< is used by HDR rendering and deferred shading
	FBO_t *portalRenderFBO;                 ///< holds a copy of the last currentRender that was rendered into a FBO
	FBO_t *occlusionRenderFBO;              ///< used for overlapping visibility determination
	FBO_t *downScaleFBO_quarter;
	FBO_t *downScaleFBO_64x64;
	//FBO_t          *downScaleFBO_16x16;
	//FBO_t          *downScaleFBO_4x4;
	//FBO_t          *downScaleFBO_1x1;
	FBO_t *contrastRenderFBO;
	FBO_t *bloomRenderFBO[2];
	FBO_t *shadowMapFBO[MAX_SHADOWMAPS];
	FBO_t *sunShadowMapFBO[MAX_SHADOWMAPS];

	// vertex buffer objects
	VBO_t *unitCubeVBO;
	IBO_t *unitCubeIBO;

	// internal shaders
	shader_t *defaultShader;
	shader_t *defaultPointLightShader;
	shader_t *defaultProjectedLightShader;
	shader_t *defaultDynamicLightShader;

	// external shaders
	shader_t *flareShader;
	shader_t *sunShader;
	char *sunShaderName;

	int numLightmaps;
	growList_t lightmaps;
	//growList_t deluxemaps;

	image_t *fatLightmap;
	int fatLightmapSize;
	int fatLightmapStep;

	// render entities
	trRefEntity_t *currentEntity;
	trRefEntity_t worldEntity;              ///< point currentEntity at this when rendering world
	model_t *currentModel;

	// render lights
	trRefLight_t *currentLight;

	viewParms_t viewParms;

	float identityLight;                    ///< 1.0 / ( 1 << overbrightBits )
	int overbrightBits;                     ///< r_overbrightBits->integer, but set to 0 if no hw gamma
	int mapOverBrightBits;                  ///< r_mapOverbrightBits->integer, but can be overriden by mapper using the worldspawn

	orientationr_t orientation;             ///< for current entity

	trRefdef_t refdef;

	vec3_t sunLight;                        ///< from the sky shader for this level
	vec3_t sunDirection;

	float lightGridMulAmbient;              ///< lightgrid multipliers specified in sky shader
	float lightGridMulDirected;             ///<

	vec3_t fogColor;
	float fogDensity;

	glfog_t glfogsettings[NUM_FOGS];
	glfogType_t glfogNum;

	frontEndCounters_t pc;
	int frontEndMsec;                       ///< not in pc due to clearing issue

	vec4_t clipRegion;                      ///< 2D clipping region

	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors

	model_t *models[MAX_MOD_KNOWN];
	int numModels;

	int numAnimations;
	skelAnimation_t *animations[MAX_ANIMATIONFILES];

	int numImages;
	growList_t images;

	int numFBOs;
	FBO_t *fbos[MAX_FBOS];

	GLuint vao;

	growList_t vbos;
	growList_t ibos;

	byte *cubeTemp[6];                      ///< 6 textures for cubemap storage
	growList_t cubeProbes;                  ///< all cubemaps in a linear growing list
	vertexHash_t **cubeHashTable;           ///< hash table for faster access

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int numShaders;
	shader_t *shaders[MAX_SHADERS];
	shader_t *sortedShaders[MAX_SHADERS];

	int numSkins;
	skin_t *skins[MAX_SKINS];

	int numTables;
	shaderTable_t *shaderTables[MAX_SHADER_TABLES];

	float sinTable[FUNCTABLE_SIZE];
	float squareTable[FUNCTABLE_SIZE];
	float triangleTable[FUNCTABLE_SIZE];
	float sawToothTable[FUNCTABLE_SIZE];
	float inverseSawToothTable[FUNCTABLE_SIZE];
	float noiseTable[FUNCTABLE_SIZE];
	float fogTable[FOG_TABLE_SIZE];

	uint32_t occlusionQueryObjects[MAX_OCCLUSION_QUERIES];
	int numUsedOcclusionQueryObjects;
} trGlobals_t;


/**
 * @struct trPrograms_s
 * @typedef trPrograms_t
 * @brief
 */
typedef struct trPrograms_s
{
	programInfo_t *gl_genericShader;
	programInfo_t *gl_lightMappingShader;
	programInfo_t *gl_vertexLightingShader_DBS_entity;
	programInfo_t *gl_vertexLightingShader_DBS_world;
	programInfo_t *gl_forwardLightingShader_omniXYZ;
	programInfo_t *gl_forwardLightingShader_projXYZ;
	programInfo_t *gl_forwardLightingShader_directionalSun;
	programInfo_t *gl_shadowFillShader;
	programInfo_t *gl_reflectionShader;
	programInfo_t *gl_skyboxShader;
	programInfo_t *gl_fogQuake3Shader;
	programInfo_t *gl_fogGlobalShader;
	programInfo_t *gl_heatHazeShader;
	programInfo_t *gl_screenShader;
	programInfo_t *gl_portalShader;
	programInfo_t *gl_toneMappingShader;
	programInfo_t *gl_contrastShader;
	programInfo_t *gl_cameraEffectsShader;
	programInfo_t *gl_blurXShader;
	programInfo_t *gl_blurYShader;
	programInfo_t *gl_debugShadowMapShader;

	programInfo_t *gl_liquidShader;
	programInfo_t *gl_rotoscopeShader;
	programInfo_t *gl_bloomShader;
	programInfo_t *gl_refractionShader;
	programInfo_t *gl_depthToColorShader;
	programInfo_t *gl_volumetricFogShader;
	programInfo_t *gl_volumetricLightingShader;
	programInfo_t *gl_dispersionShader;

	programInfo_t *gl_depthOfField;
	programInfo_t *gl_ssao;

	programInfo_t *gl_colorCorrection;

	/// This is set with the GLSL_SelectPermutation
	shaderProgram_t *selectedProgram;
} trPrograms_t;

extern trPrograms_t trProg;

extern const mat4_t quakeToOpenGLMatrix;
extern const mat4_t openGLToQuakeMatrix;
extern const mat4_t quakeToD3DMatrix;
extern const mat4_t flipZMatrix;
extern const GLenum geometricRenderTargets[];
extern int          shadowMapResolutions[5];
extern int          sunShadowMapResolutions[5];

extern backEndState_t backEnd;
extern trGlobals_t    tr;
extern glconfig_t     glConfig;     ///< outside of TR since it shouldn't be cleared during ref re-init
extern glconfig2_t    glConfig2;

extern glstate_t glState;           ///< outside of TR since it shouldn't be cleared during ref re-init

// cvars

extern cvar_t *r_railCoreWidth;

extern cvar_t *r_lodTest;

extern cvar_t *r_wolfFog;



extern cvar_t *r_ambientScale;
extern cvar_t *r_lightScale;
extern cvar_t *r_debugLight;

extern cvar_t *r_staticLight;               ///< static lights enabled/disabled
extern cvar_t *r_dynamicLightShadows;
extern cvar_t *r_precomputedLighting;
extern cvar_t *r_vertexLighting;
extern cvar_t *r_compressDiffuseMaps;
extern cvar_t *r_compressSpecularMaps;
extern cvar_t *r_compressNormalMaps;
extern cvar_t *r_heatHazeFix;
extern cvar_t *r_noMarksOnTrisurfs;

extern cvar_t *r_drawpolies;                ///< disable/enable world rendering

extern cvar_t *r_noLightScissors;
extern cvar_t *r_noLightVisCull;
extern cvar_t *r_noInteractionSort;

extern cvar_t *r_extOcclusionQuery;
extern cvar_t *r_extTextureNonPowerOfTwo;
extern cvar_t *r_extDrawBuffers;
extern cvar_t *r_extVertexArrayObject;
extern cvar_t *r_extHalfFloatPixel;
extern cvar_t *r_extTextureFloat;
extern cvar_t *r_extStencilWrap;
extern cvar_t *r_extStencilTwoSide;
extern cvar_t *r_extDepthBoundsTest;
extern cvar_t *r_extFramebufferObject;
extern cvar_t *r_extPackedDepthStencil;
extern cvar_t *r_extFramebufferBlit;
extern cvar_t *r_extGenerateMipmap;

extern cvar_t *r_collapseStages;


extern cvar_t *r_specularExponent;
extern cvar_t *r_specularExponent2;
extern cvar_t *r_specularScale;
extern cvar_t *r_normalScale;
extern cvar_t *r_normalMapping;
extern cvar_t *r_wrapAroundLighting;
extern cvar_t *r_diffuseLighting;
extern cvar_t *r_rimLighting;
extern cvar_t *r_rimExponent;

// 3 = stencil shadow volumes
// 4 = shadow mapping
extern cvar_t *r_softShadows;
extern cvar_t *r_shadowBlur;

extern cvar_t *r_shadowMapQuality;
extern cvar_t *r_shadowMapSizeUltra;
extern cvar_t *r_shadowMapSizeVeryHigh;
extern cvar_t *r_shadowMapSizeHigh;
extern cvar_t *r_shadowMapSizeMedium;
extern cvar_t *r_shadowMapSizeLow;

extern cvar_t *r_shadowMapSizeSunUltra;
extern cvar_t *r_shadowMapSizeSunVeryHigh;
extern cvar_t *r_shadowMapSizeSunHigh;
extern cvar_t *r_shadowMapSizeSunMedium;
extern cvar_t *r_shadowMapSizeSunLow;

extern cvar_t *r_shadowOffsetFactor;
extern cvar_t *r_shadowOffsetUnits;
extern cvar_t *r_shadowLodBias;
extern cvar_t *r_shadowLodScale;
extern cvar_t *r_noShadowPyramids;
extern cvar_t *r_cullShadowPyramidFaces;
extern cvar_t *r_cullShadowPyramidCurves;
extern cvar_t *r_cullShadowPyramidTriangles;
extern cvar_t *r_debugShadowMaps;
extern cvar_t *r_noShadowFrustums;
extern cvar_t *r_noLightFrustums;
extern cvar_t *r_shadowMapLuminanceAlpha;
extern cvar_t *r_shadowMapLinearFilter;
//extern cvar_t *r_lightBleedReduction;
//extern cvar_t *r_overDarkeningFactor; // exponential shadow mapping
extern cvar_t *r_shadowMapDepthScale;
extern cvar_t *r_parallelShadowSplits;
extern cvar_t *r_parallelShadowSplitWeight;

extern cvar_t *r_stitchCurves;

extern cvar_t *r_skipLightBuffer;

extern cvar_t *r_showShadowVolumes;
extern cvar_t *r_showShadowLod;
extern cvar_t *r_showShadowMaps;
extern cvar_t *r_showSkeleton;
extern cvar_t *r_showEntityTransforms;
extern cvar_t *r_showLightTransforms;
extern cvar_t *r_showLightInteractions;
extern cvar_t *r_showLightScissors;
extern cvar_t *r_showLightBatches;
extern cvar_t *r_showLightGrid;
extern cvar_t *r_showOcclusionQueries;
extern cvar_t *r_showBatches;
extern cvar_t *r_showLightMaps;                 ///< render lightmaps only
//extern cvar_t *r_showDeluxeMaps;
extern cvar_t *r_showCubeProbes;
extern cvar_t *r_showBspNodes;
extern cvar_t *r_showParallelShadowSplits;
extern cvar_t *r_showDecalProjectors;

extern cvar_t *r_vboFaces;
extern cvar_t *r_vboCurves;
extern cvar_t *r_vboTriangles;
extern cvar_t *r_vboShadows;
extern cvar_t *r_vboLighting;
extern cvar_t *r_vboModels;
extern cvar_t *r_vboVertexSkinning;
extern cvar_t *r_vboSmoothNormals;
extern cvar_t *r_vboFoliage;
extern cvar_t *r_worldInlineModels;
#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
extern cvar_t *r_mergeClusterSurfaces;
extern cvar_t *r_mergeClusterFaces;
extern cvar_t *r_mergeClusterCurves;
extern cvar_t *r_mergeClusterTriangles;
#endif

extern cvar_t *r_parallaxMapping;
extern cvar_t *r_parallaxDepthScale;

extern cvar_t *r_dynamicBspOcclusionCulling;
extern cvar_t *r_dynamicEntityOcclusionCulling;
extern cvar_t *r_dynamicLightOcclusionCulling;
extern cvar_t *r_chcMaxPrevInvisNodesBatchSize;
extern cvar_t *r_chcMaxVisibleFrames;
extern cvar_t *r_chcVisibilityThreshold;
extern cvar_t *r_chcIgnoreLeaves;

extern cvar_t *r_hdrRendering;
extern cvar_t *r_hdrMinLuminance;
extern cvar_t *r_hdrMaxLuminance;
extern cvar_t *r_hdrKey;
extern cvar_t *r_hdrContrastThreshold;
extern cvar_t *r_hdrContrastOffset;
extern cvar_t *r_hdrLightmap;
extern cvar_t *r_hdrLightmapExposure;
extern cvar_t *r_hdrLightmapGamma;
extern cvar_t *r_hdrLightmapCompensate;
extern cvar_t *r_hdrToneMappingOperator;
extern cvar_t *r_hdrGamma;
extern cvar_t *r_hdrDebug;

extern cvar_t *r_screenSpaceAmbientOcclusion;
extern cvar_t *r_depthOfField;

extern cvar_t *r_reflectionMapping;
extern cvar_t *r_highQualityNormalMapping;

extern cvar_t *r_bloom;
extern cvar_t *r_bloomBlur;
extern cvar_t *r_bloomPasses;
extern cvar_t *r_rotoscope;
extern cvar_t *r_cameraPostFX;
extern cvar_t *r_cameraVignette;
extern cvar_t *r_cameraFilmGrainScale;

extern cvar_t *r_evsmPostProcess;

extern cvar_t *r_recompileShaders;
extern cvar_t *r_rotoscopeBlur;

extern cvar_t *r_materialScan;

extern cvar_t *r_smoothNormals;

//====================================================================

void R_SwapBuffers(int);

void R_RenderView(viewParms_t *parms);

void R_AddMDVSurfaces(trRefEntity_t *ent);
void R_AddMDVInteractions(trRefEntity_t *ent, trRefLight_t *light);
void R_AddNullModelSurfaces(trRefEntity_t *ent);
void R_AddBeamSurfaces(trRefEntity_t *ent);
void R_AddRailSurfaces(trRefEntity_t *ent, qboolean isUnderwater);
void R_AddLightningBoltSurfaces(trRefEntity_t *ent);

int BSPSurfaceCompare(const void *a, const void *b);

void R_AddPolygonSurfaces(void);
void R_AddPolygonBufferSurfaces(void);

void R_AddDrawSurf(surfaceType_t *surface, shader_t *shader, int lightmapNum, int fogNum);

void R_LocalNormalToWorld(const vec3_t local, vec3_t world);
void R_LocalPointToWorld(const vec3_t local, vec3_t world);

cullResult_t R_CullBox(vec3_t worldBounds[2]);
cullResult_t R_CullLocalBox(vec3_t bounds[2]);
int R_CullLocalPointAndRadius(vec3_t origin, float radius);
int R_CullPointAndRadius(vec3_t origin, float radius);

int R_FogLocalPointAndRadius(const vec3_t pt, float radius);
int R_FogPointAndRadius(const vec3_t pt, float radius);
int R_FogWorldBox(vec3_t bounds[2]);

void R_SetupEntityWorldBounds(trRefEntity_t *ent);

void R_RotateEntityForViewParms(const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *orientation);
void R_RotateEntityForLight(const trRefEntity_t *ent, const trRefLight_t *light, orientationr_t *orientation);
void R_RotateLightForViewParms(const trRefLight_t *ent, const viewParms_t *viewParms, orientationr_t *orientation);

void R_SetupFrustum2(frustum_t frustum, const mat4_t mvp);
void R_SetupFrustum(void);
qboolean R_CompareVert(srfVert_t *v1, srfVert_t *v2, qboolean checkST);
void R_CalcNormalForTriangle(vec3_t normal, const vec3_t v0, const vec3_t v1, const vec3_t v2);

void R_CalcTangentsForTriangle(vec3_t tangent, vec3_t binormal,
							   const vec3_t v0, const vec3_t v1, const vec3_t v2,
							   const vec2_t t0, const vec2_t t1, const vec2_t t2);

void R_CalcTangentsForTriangle2(vec3_t tangent, vec3_t binormal,
								const vec3_t v0, const vec3_t v1, const vec3_t v2,
								const vec2_t t0, const vec2_t t1, const vec2_t t2);

void R_CalcTangentSpace(vec3_t tangent, vec3_t binormal, vec3_t normal,
						const vec3_t v0, const vec3_t v1, const vec3_t v2,
						const vec2_t t0, const vec2_t t1, const vec2_t t2);

void R_CalcTangentSpaceFast(vec3_t tangent, vec3_t binormal, vec3_t normal,
							const vec3_t v0, const vec3_t v1, const vec3_t v2,
							const vec2_t t0, const vec2_t t1, const vec2_t t2);

void R_CalcTBN(vec3_t tangent, vec3_t bitangent, vec3_t normal,
			   const vec3_t v1, const vec3_t v2, const vec3_t v3,
			   const vec2_t w1, const vec2_t w2, const vec2_t w3);

qboolean R_CalcTangentVectors(srfVert_t * dv[3]);

void R_CalcSurfaceTrianglePlanes(int numTriangles, srfTriangle_t *triangles, srfVert_t *verts);

float R_CalcFov(float fovX, float width, float height);

// visualisation tools to help debugging the renderer frontend
void R_DebugAxis(const vec3_t origin, const mat4_t transformMatrix);
void R_DebugBoundingBox(const vec3_t origin, const vec3_t mins, const vec3_t maxs, vec4_t color);
void R_DebugPolygon(int color, int numPoints, float *points);
void R_DebugText(const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude);

/*
====================================================================
OpenGL WRAPPERS, tr_gl.c
====================================================================
*/
#define GLCOLOR_WHITE 1.0f, 1.0f, 1.0f, 1.0f
#define GLCOLOR_BLACK 0.0f, 0.0f, 0.0f, 1.0f
#define GLCOLOR_RED 1.0f, 0.0f, 0.0f, 1.0f
#define GLCOLOR_GREEN 0.0f, 1.0f, 0.0f, 1.0f
#define GLCOLOR_BLUE 0.0f, 0.0f, 1.0f, 1.0f

#define GLCOLOR_NONE 0.0f, 0.0f, 0.0f, 0.0f

void GL_Bind(image_t *image);
void GL_BindNearestCubeMap(const vec3_t xyz);
void GL_Unbind(void);
void BindAnimatedImage(textureBundle_t *bundle);
//void GL_TextureFilter(image_t *image, filterType_t filterType);
void GL_SetDefaultState(void);
void GL_SelectTexture(int unit);
void GL_TextureMode(const char *string);

void GL_BlendFunc(GLenum sfactor, GLenum dfactor);
void GL_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void GL_ClearDepth(GLclampd depth);
void GL_ClearStencil(GLint s);
void GL_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void GL_CullFace(GLenum mode);
void GL_DepthFunc(GLenum func);
void GL_DepthMask(GLboolean flag);
void GL_DrawBuffer(GLenum mode);
void GL_FrontFace(GLenum mode);
void GL_LoadModelViewMatrix(const mat4_t m);
void GL_LoadProjectionMatrix(const mat4_t m);
void GL_PushMatrix(void);
void GL_PopMatrix(void);
void GL_PolygonMode(GLenum face, GLenum mode);
void GL_Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
void GL_Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
void GL_PolygonOffset(float factor, float units);
void GL_Clear(unsigned int bits);

void GL_State(uint32_t stateBits);
void GL_Cull(int cullType);

void GL_CheckErrors_(const char *fileName, int line);

#define GL_CheckErrors() GL_CheckErrors_(__FILE__, __LINE__)

//ModelViewProjectionMatrix
#define GLSTACK_MVPM glState.modelViewProjectionMatrix[glState.stackIndex]
//ProjectionMatrix
#define GLSTACK_PM glState.projectionMatrix[glState.stackIndex]
//ModelViewMatrix
#define GLSTACK_MVM glState.modelViewMatrix[glState.stackIndex]

#define MODEL_MATRIX backEnd.orientation.transformMatrix

void RB_SetViewMVPM(void);

/*
====================================================================

====================================================================
*/

void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void RE_BeginFrame(void);
void RE_EndFrame(int *frontEndMsec, int *backEndMsec);
void RE_BeginRegistration(glconfig_t *glconfigOut);
void RE_LoadWorldMap(const char *name);
void RE_SetWorldVisData(const byte *vis);
qhandle_t RE_RegisterModel(const char *name);
qhandle_t RE_RegisterSkin(const char *name);
void RE_Shutdown(qboolean destroyWindow);

qboolean RE_GetSkinModel(qhandle_t skinid, const char *type, char *name);
qhandle_t RE_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap);

qboolean R_GetEntityToken(char *buffer, size_t size);
float R_ProcessLightmap(byte *pic, int in_padding, int width, int height, byte *pic_out);

model_t *R_AllocModel(void);

void R_Init(void);

void R_SetColorMappings(void);
void R_GammaCorrect(byte *buffer, int bufSize);

void R_ImageList_f(void);
void R_SkinList_f(void);

void R_SubImageCpy(byte *dest, size_t destx, size_t desty, size_t destw, size_t desth, byte *src, size_t srcw, size_t srch, size_t bytes, qboolean in);

// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
const void *RB_TakeScreenshotCmd(const void *data);


void R_InitSkins(void);
skin_t *R_GetSkinByHandle(qhandle_t hSkin);

void R_DeleteSurfaceVBOs(void);

/*
====================================================================
IMAGES, tr_image.c
====================================================================
*/
void R_InitImages(void);
void R_ShutdownImages(void);
int R_SumOfUsedImages(void);
void R_ImageCopyBack(image_t *image, int x, int y, int width, int height);
#define ImageCopyBackBuffer(image) R_ImageCopyBack(image, 0, 0, image->uploadWidth, image->uploadHeight)

image_t *R_FindImageFile(const char *imageName, int bits, filterType_t filterType, wrapType_t wrapType, const char *materialName);
image_t *R_FindCubeImage(const char *imageName, int bits, filterType_t filterType, wrapType_t wrapType, const char *materialName);

image_t *R_CreateImage(const char *name, const byte *pic, int width, int height, int bits, filterType_t filterType,
					   wrapType_t wrapType);

image_t *R_CreateCubeImage(const char *name,
						   const byte *pic[6],
						   int width, int height, int bits, filterType_t filterType, wrapType_t wrapType);

image_t *R_AllocImage(const char *name, qboolean linkIntoHashTable);
void R_UploadImage(const byte **dataArray, int numData, image_t *image);

int RE_GetTextureId(const char *name);

void R_InitFogTable(void);
float R_FogFactor(float s, float t);

/*
====================================================================
SHADERS, tr_shader.c
====================================================================
*/
qhandle_t RE_RegisterShader(const char *name);
qhandle_t RE_RegisterShaderNoMip(const char *name);
qhandle_t RE_RegisterShaderLightAttenuation(const char *name);
qhandle_t RE_RegisterShaderFromImage(const char *name, image_t *image, qboolean mipRawImage);
qboolean RE_LoadDynamicShader(const char *shadername, const char *shadertext);

shader_t *R_FindShader(const char *name, shaderType_t type, qboolean mipRawImage);
shader_t *R_GetShaderByHandle(qhandle_t hShader);
shader_t *R_GetShaderByState(int index, long *cycleTime);
shader_t *R_FindShaderByName(const char *name);
void R_InitShaders(void);
void R_ShaderList_f(void);
void R_ShaderExp_f(void);
void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset);

/*
====================================================================
tr_shade.c
====================================================================
*/
void clipPortalPlane(void);

/*
====================================================================
TESSELATOR/SHADER DECLARATIONS
====================================================================
*/

typedef byte color4ub_t[4];

/**
 * @struct stageVars
 * @typedef stageVars_t
 * @brief
 */
typedef struct stageVars
{
	vec4_t color;
	qboolean texMatricesChanged[MAX_TEXTURE_BUNDLES];
	mat4_t texMatrices[MAX_TEXTURE_BUNDLES];
} stageVars_t;

#define MAX_MULTIDRAW_PRIMITIVES    1000

/**
 * @struct shaderCommands_s
 * @typedef shaderCommands_t
 * @brief
 */
typedef struct shaderCommands_s
{
	vec4_t xyz[SHADER_MAX_VERTEXES];
	vec4_t texCoords[SHADER_MAX_VERTEXES];
	vec4_t lightCoords[SHADER_MAX_VERTEXES];
	vec4_t tangents[SHADER_MAX_VERTEXES];
	vec4_t binormals[SHADER_MAX_VERTEXES];
	vec4_t normals[SHADER_MAX_VERTEXES];
	vec4_t colors[SHADER_MAX_VERTEXES];

	//vec4_t paintColors[SHADER_MAX_VERTEXES];                ///< for advanced terrain blending
	//vec4_t lightDirections[SHADER_MAX_VERTEXES];

	glIndex_t indexes[SHADER_MAX_INDEXES];

	VBO_t *vbo;
	IBO_t *ibo;

	stageVars_t svars;

	shader_t *surfaceShader;
	shader_t *lightShader;

	double shaderTime;

	qboolean skipTangentSpaces;
	qboolean skipVBO;
	int16_t lightmapNum;
	int16_t fogNum;

	uint32_t numIndexes;
	uint32_t numVertexes;
	uint32_t attribsSet;

	int multiDrawPrimitives;
	glIndex_t *multiDrawIndexes[MAX_MULTIDRAW_PRIMITIVES];
	int multiDrawCounts[MAX_MULTIDRAW_PRIMITIVES];

	qboolean vboVertexSkinning;
	mat4_t boneMatrices[MAX_BONES];

	// info extracted from current shader or backend mode
	void (*stageIteratorFunc)(void);
	void (*stageIteratorFunc2)(void);

	int numSurfaceStages;
	shaderStage_t **surfaceStages;
} shaderCommands_t;

extern shaderCommands_t tess;

void GLSL_InitGPUShaders(void);
void GLSL_CompileGPUShaders(void);
void GLSL_ShutdownGPUShaders(void);
void GLSL_BindProgram(shaderProgram_t *program);
void GLSL_BindNullProgram(void);

// *INDENT-OFF*
void Tess_Begin(void (*stageIteratorFunc)(void),
				void (*stageIteratorFunc2)(void),
				shader_t *surfaceShader, shader_t *lightShader,
				qboolean skipTangentSpaces,
				qboolean skipVBO,
				int lightmapNum,
				int fogNum);
// *INDENT-ON*
void Tess_End(void);
void Tess_EndBegin(void);
void Tess_DrawElements(void);
void Tess_CheckOverflow(int verts, int indexes);

void Tess_ComputeColor(shaderStage_t *pStage);

void Tess_StageIteratorDebug(void);
void Tess_StageIteratorGeneric(void);
//void Tess_StageIteratorDepthFill();
void Tess_StageIteratorShadowFill(void);
void Tess_StageIteratorLighting(void);
void Tess_StageIteratorSky(void);

void Tess_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, const vec4_t color);
void Tess_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, const vec4_t color, float s1, float t1, float s2, float t2);
void Tess_AddQuadStampExt2(vec4_t quadVerts[4], const vec4_t color, float s1, float t1, float s2, float t2, qboolean calcNormals);
void Tess_AddQuadStamp2(vec4_t quadVerts[4], const vec4_t color);
void Tess_AddQuadStamp2WithNormals(vec4_t quadVerts[4], const vec4_t color);

/*
Add a polyhedron that is composed of four triangular faces

@param tretraVerts[0..2] are the ground vertices, tretaVerts[3] is the pyramid offset
*/
void Tess_AddTetrahedron(vec4_t tetraVerts[4], vec4_t const color);

void Tess_AddCube(const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const vec4_t color);
void Tess_AddCubeWithNormals(const vec3_t position, const vec3_t minSize, const vec3_t maxSize, const vec4_t color);

void Tess_InstantQuad(vec4_t quadVerts[4]);
void Tess_UpdateVBOs(uint32_t attribBits);

void RB_ShowImages(void);

/*
============================================================
WORLD MAP, tr_world.c
============================================================
*/

void R_AddBSPModelSurfaces(trRefEntity_t *ent);
void R_AddWorldSurfaces(void);
qboolean R_inPVS(const vec3_t p1, const vec3_t p2);

void R_AddWorldInteractions(trRefLight_t *light);
void R_AddPrecachedWorldInteractions(trRefLight_t *light);
void R_ShutdownVBOs(void);

/*
============================================================
FLARES, tr_flares.c
============================================================
*/

void R_ClearFlares(void);

void RB_AddFlare(void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal, qboolean visible, qboolean isCorona);
void RB_AddLightFlares(void);
void RB_RenderFlares(void);

/*
============================================================
LIGHTS, tr_light.c
============================================================
*/

void R_AddBrushModelInteractions(trRefEntity_t *ent, trRefLight_t *light);
void R_SetupEntityLighting(const trRefdef_t *refdef, trRefEntity_t *ent, vec3_t forcedOrigin);

void R_SetupLightOrigin(trRefLight_t *light);
void R_SetupLightLocalBounds(trRefLight_t *light);
void R_SetupLightWorldBounds(trRefLight_t *light);

void R_SetupLightView(trRefLight_t *light);
void R_SetupLightFrustum(trRefLight_t *light);
void R_SetupLightProjection(trRefLight_t *light);

qboolean R_AddLightInteraction(trRefLight_t *light, surfaceType_t *surface, shader_t *surfaceShader, byte cubeSideBits,
							   interactionType_t iaType);

void R_SortInteractions(trRefLight_t *light);

void R_SetupLightScissor(trRefLight_t *light);
//void R_SetupLightDepthBounds(trRefLight_t *light);
void R_SetupLightLOD(trRefLight_t *light);

void R_SetupLightShader(trRefLight_t *light);

byte R_CalcLightCubeSideBits(trRefLight_t * light, vec3_t worldBounds[2]);

int R_CullLightPoint(trRefLight_t *light, const vec3_t p);
int R_CullLightTriangle(trRefLight_t * light, vec3_t verts[3]);
int R_CullLightWorldBounds(trRefLight_t * light, vec3_t worldBounds[2]);

void R_ComputeFinalAttenuation(shaderStage_t *pStage, trRefLight_t *light);

/*
============================================================
FOG, tr_fog.c
============================================================
*/

void R_SetFrameFog(void);
void RB_Fog(glfog_t *curfog);
void RB_FogOff(void);
void RB_FogOn(void);
void RE_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density);
void RE_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);

/*
============================================================
SHADOWS, tr_shadows.c
============================================================
*/

void RB_ProjectionShadowDeform(void);

/*
============================================================
SKIES
============================================================
*/

void R_InitSkyTexCoords(float heightCloud);
void RB_DrawSun(void);

/*
============================================================
CURVE TESSELATION, tr_curve.c
============================================================
*/

srfGridMesh_t *R_SubdividePatchToGrid(int width, int height, srfVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE]);
srfGridMesh_t *R_GridInsertColumn(srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror);
srfGridMesh_t *R_GridInsertRow(srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror);
void R_FreeSurfaceGridMesh(srfGridMesh_t *grid);

/*
============================================================
MARKERS, POLYGON PROJECTION ON WORLD POLYGONS, tr_marks.c
============================================================
*/

int R_MarkFragments(int numPoints, const vec3_t *points, const vec3_t projection,
					int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer);

/*
============================================================
FRAME BUFFER OBJECTS, tr_fbo.c
============================================================
*/
qboolean R_CheckFBO(const FBO_t *fbo);

FBO_t *R_CreateFBO(const char *name, int width, int height);

void R_CreateFBOColorBuffer(FBO_t *fbo, int format, int index);
void R_CreateFBODepthBuffer(FBO_t *fbo, int format);
void R_CreateFBOStencilBuffer(FBO_t *fbo, int format);

void R_AttachFBOTexture1D(int texId, int index);
void R_AttachFBOTexture2D(int target, int texId, int index);
void R_AttachFBOTexture3D(int texId, int index, int zOffset);
void R_AttachFBOTextureDepth(int texId);

void R_CopyToFBO(FBO_t *from, FBO_t *to, GLuint mask, GLuint filter);

void R_BindFBO(FBO_t *fbo);
void R_BindNullFBO(void);

void R_SetDefaultFBO(void);
void R_InitFBOs(void);
void R_ShutdownFBOs(void);
void R_FBOList_f(void);

/*
============================================================
VERTEX BUFFER OBJECTS, tr_vbo.c
============================================================
*/
VBO_t *R_CreateVBO(const char *name, byte *vertexes, int vertexesSize, vboUsage_t usage);
VBO_t *R_CreateVBO2(const char *name, int numVertexes, srfVert_t *verts, uint32_t stateBits, vboUsage_t usage);

IBO_t *R_CreateIBO(const char *name, byte *indexes, int indexesSize, vboUsage_t usage);
IBO_t *R_CreateIBO2(const char *name, int numTriangles, srfTriangle_t *triangles, vboUsage_t usage);

void R_BindVBO(VBO_t *vbo);
void R_BindNullVBO(void);

void R_BindIBO(IBO_t *ibo);
void R_BindNullIBO(void);

void R_InitVBOs(void);
void R_ShutdownVBOs(void);
void R_VBOList_f(void);

/*
============================================================
DECALS - tr_decals.c
============================================================
*/

void RE_ProjectDecal(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime,
					 int fadeTime);
void RE_ClearDecals(void);

//void R_AddModelShadow(refEntity_t *ent);

void R_TransformDecalProjector(decalProjector_t * in, vec3_t axis[3], vec3_t origin, decalProjector_t * out);
qboolean R_TestDecalBoundingBox(decalProjector_t *dp, vec3_t mins, vec3_t maxs);
qboolean R_TestDecalBoundingSphere(decalProjector_t *dp, vec3_t center, float radius2);

void R_ProjectDecalOntoSurface(decalProjector_t *dp, bspSurface_t *surf, bspModel_t *bmodel);

void R_AddDecalSurface(decal_t *decal);
void R_AddDecalSurfaces(bspModel_t *bmodel);
void R_CullDecalProjectors(void);

/*
============================================================
SCENE GENERATION, tr_scene.c
============================================================
*/

void R_InitNextFrame(void);

void RE_ClearScene(void);
void RE_AddRefEntityToScene(const refEntity_t *ent);

#if defined(USE_REFLIGHT)
void RE_AddRefLightToScene(const refLight_t *light);
#endif

void RE_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void RE_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);

void RE_AddPolyBufferToScene(polyBuffer_t *pPolyBuffer);

void RE_AddDynamicLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);

void RE_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
void RE_RenderScene(const refdef_t *fd);

/*
=============================================================
ANIMATED MODELS
=============================================================
*/

void R_InitAnimations(void);

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
qhandle_t RE_RegisterAnimation(const char *name);
#endif

skelAnimation_t *R_GetAnimationByHandle(qhandle_t hAnim);
void R_AnimationList_f(void);

void R_AddMD5Surfaces(trRefEntity_t *ent);
void R_AddMD5Interactions(trRefEntity_t *ent, trRefLight_t *light);

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
int RE_CheckSkeleton(refSkeleton_t *skel, qhandle_t hModel, qhandle_t hAnim);
int RE_BuildSkeleton(refSkeleton_t *skel, qhandle_t hAnim, int startFrame, int endFrame, float frac,
					 qboolean clearOrigin);
int RE_BlendSkeleton(refSkeleton_t *skel, const refSkeleton_t *blend, float frac);
int RE_AnimNumFrames(qhandle_t hAnim);
int RE_AnimFrameRate(qhandle_t hAnim);
#endif

/*
=============================================================
ANIMATED MODELS WOLFENSTEIN
=============================================================
*/

/*
void R_MakeAnimModel(model_t * model);
void R_AddAnimSurfaces(trRefEntity_t * ent);
void RB_SurfaceAnim(mdsSurface_t * surfType);
int R_GetBoneTag(orientation_t * outTag, mdsHeader_t * mds, int startTagIndex, const refEntity_t * refent,
							 const char *tagName);
*/

/*
=============================================================
ANIMATED MODELS WOLF:ET  MDM/MDX
=============================================================
*/

void R_MDM_AddAnimSurfaces(trRefEntity_t *ent);
void R_AddMDMInteractions(trRefEntity_t *ent, trRefLight_t *light);

int R_MDM_GetBoneTag(orientation_t *outTag, mdmModel_t *mdm, int startTagIndex, const refEntity_t *refent,
					 const char *tagName);

void Tess_MDM_SurfaceAnim(mdmSurfaceIntern_t *surface);
void Tess_SurfaceVBOMDMMesh(srfVBOMDMMesh_t *surface);

/*
=============================================================

=============================================================
*/

void R_TransformWorldToClip(const vec3_t src, const float *cameraViewMatrix,
							const float *projectionMatrix, vec4_t eye, vec4_t dst);
void R_TransformModelToClip(const vec3_t src, const float *modelMatrix,
							const float *projectionMatrix, vec4_t eye, vec4_t dst);
void R_TransformClipToWindow(const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window);
float R_ProjectRadius(float r, vec3_t location);


qboolean ShaderRequiresCPUDeforms(const shader_t *shader);
void Tess_DeformGeometry(void);

float RB_EvalWaveForm(const waveForm_t *wf);
float RB_EvalWaveFormClamped(const waveForm_t *wf);
float RB_EvalExpression(const expression_t *exp, float defaultValue);

void RB_CalcTexMatrix(const textureBundle_t *bundle, mat4_t matrix);

/*
=============================================================
RENDERER IMAGE FUNCTIONS
=============================================================
*/
size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality, int image_width, int image_height, byte *image_buffer, int padding);
void RE_SaveJPG(const char *filename, int quality, int image_width, int image_height, byte *image_buffer, int padding);
#ifdef FEATURE_PNG
void RE_SavePNG(char *filename, int image_width, int image_height, unsigned char *image_buffer, int padding);
#endif

/*
=============================================================
RENDERER BACK END FUNCTIONS
=============================================================
*/

void RB_ExecuteRenderCommands(const void *data);

/*
=============================================================
RENDERER BACK END COMMAND QUEUE
=============================================================
*/

#define MAX_RENDER_COMMANDS (0x40000 * 8) ///< was 0x40000

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
 * @struct swapBuffersCommand_t
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

	byte gradientColor[4];              ///< color values 0-255
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
 * @struct drawViewCommand_t
 * @brief
 */
typedef struct
{
	int commandId;
	trRefdef_t refdef;
	viewParms_t viewParms;
} drawViewCommand_t;

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
	RC_END_OF_LIST = 0,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_2DPOLYS,
	RC_ROTATED_PIC,
	RC_STRETCH_PIC_GRADIENT,
	RC_DRAW_VIEW,
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

// max decal projectors per frame, each can generate lots of polys
#define MAX_DECAL_PROJECTORS      128 ///< includes decal projectors that will be culled out, hard limited to 32 active projectors because of bitmasks.
#define MAX_USED_DECAL_PROJECTORS 32
#define MAX_DECALS                1024
#define DECAL_MASK              (MAX_DECALS - 1)

#define MAX_POLYBUFFERS 4096

/**
 * @struct backEndData_t
 * @brief All of the information needed by the back end must be
 * contained in a backEndData_t.
 */
typedef struct
{
	drawSurf_t drawSurfs[MAX_DRAWSURFS];
	interaction_t interactions[MAX_INTERACTIONS];

	corona_t coronas[MAX_CORONAS];

	trRefLight_t lights[MAX_REF_LIGHTS];
	trRefEntity_t entities[MAX_REFENTITIES];

	srfPoly_t *polys;           //[MAX_POLYS];
	polyVert_t *polyVerts;      //[MAX_POLYVERTS];
	srfPolyBuffer_t *polybuffers; //[MAX_POLYS];

	decalProjector_t decalProjectors[MAX_DECAL_PROJECTORS];
	srfDecal_t decals[MAX_DECALS];

	renderCommandList_t commands;
} backEndData_t;

extern backEndData_t *backEndData;  ///< the second one may not be allocated

void *R_GetCommandBuffer(unsigned int bytes);
void RB_ExecuteRenderCommands(const void *data);

void R_IssuePendingRenderCommands(void);

void R_AddDrawViewCmd(void);

void RE_SetColor(const float *rgba);
void RE_SetClipRegion(const float *region);
void RE_StretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
void RE_RotatedPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);
void RE_StretchPicGradient(float x, float y, float w, float h,
						   float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor,
						   int gradientType);
void RE_2DPolyies(polyVert_t *verts, int numverts, qhandle_t hShader);

// video stuff
const void *RB_TakeVideoFrameCmd(const void *data);
void RE_TakeVideoFrame(int width, int height, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg);

// cubemap reflections stuff
void R_BuildCubeMaps(void);
void R_FindTwoNearestCubeMaps(const vec3_t position, cubemapProbe_t **cubeProbe1, cubemapProbe_t **cubeProbe2, float *distance1, float *distance2);

void FreeVertexHashTable(vertexHash_t **hashTable);

void RE_RenderToTexture(int textureid, int x, int y, int w, int h);
void RE_Finish(void);

void LoadRGBEToFloats(const char *name, float **pic, int *width, int *height, qboolean doGamma, qboolean toneMap, qboolean compensate);
void LoadRGBEToHalfs(const char *name, unsigned short **halfImage, int *width, int *height);

//tr_growlist.c
// you don't need to init the growlist if you don't mind it growing and moving the list as it expands
void Com_InitGrowList(growList_t *list, int maxElements);
void Com_DestroyGrowList(growList_t *list);
int  Com_AddToGrowList(growList_t *list, void *data);
void *Com_GrowListElement(const growList_t *list, int index);
int  Com_IndexForGrowListElement(const growList_t *list, const void *element);

//tr_glsl.c
void GLSL_VertexAttribsState(uint32_t stateBits);
void GLSL_VertexAttribPointers(uint32_t attribBits);
void GLSL_SelectTexture(shaderProgram_t *program, texture_def_t tex);
void GLSL_SetUniformBoolean(shaderProgram_t *program, int uniformNum, GLboolean value);
void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value);
void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value);
void GLSL_SetUniformDouble(shaderProgram_t *program, int uniformNum, GLdouble value);
void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v);
void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v);
void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v);
void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v);
void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const mat4_t matrix);
void GLSL_SetUniformFloatARR(shaderProgram_t *program, int uniformNum, float *floatarray, int arraysize);
void GLSL_SetUniformVec4ARR(shaderProgram_t *program, int uniformNum, vec4_t *vectorarray, int arraysize);
void GLSL_SetUniformMatrix16ARR(shaderProgram_t *program, int uniformNum, mat4_t *matrixarray, int arraysize);
void GLSL_SetMacroState(programInfo_t *programlist, int macro, int enabled);
void GLSL_SetMacroStates(programInfo_t *programlist, int numMacros, ...);
void GLSL_SelectPermutation(programInfo_t *programlist);
void GLSL_SetRequiredVertexPointers(programInfo_t *programlist);
void GLSL_SetUniform_DeformParms(deformStage_t deforms[], int numDeforms);
void GLSL_SetUniform_ColorModulate(programInfo_t *prog, int colorGen, int alphaGen);
void GLSL_SetUniform_AlphaTest(uint32_t stateBits);

#define SelectTexture(tex) GLSL_SelectTexture(trProg.selectedProgram, tex)
#define SetUniformBoolean(uniformNum, value) GLSL_SetUniformBoolean(trProg.selectedProgram, uniformNum, value)
#define SetUniformInt(uniformNum, value) GLSL_SetUniformInt(trProg.selectedProgram, uniformNum, value)
#define SetUniformFloat(uniformNum, value) GLSL_SetUniformFloat(trProg.selectedProgram, uniformNum, value)
#define SetUniformFloat5(uniformNum, value) GLSL_SetUniformFloat5(trProg.selectedProgram, uniformNum, value)
#define SetUniformVec2(uniformNum, value) GLSL_SetUniformVec2(trProg.selectedProgram, uniformNum, value)
#define SetUniformVec3(uniformNum, value) GLSL_SetUniformVec3(trProg.selectedProgram, uniformNum, value)
#define SetUniformVec4(uniformNum, value) GLSL_SetUniformVec4(trProg.selectedProgram, uniformNum, value)
#define SetUniformMatrix16(uniformNum, value) GLSL_SetUniformMatrix16(trProg.selectedProgram, uniformNum, value)
#define SetUniformFloatARR(uniformNum, value, size) GLSL_SetUniformFloatARR(trProg.selectedProgram, uniformNum, value, size)
#define SetUniformVec4ARR(uniformNum, value, size) GLSL_SetUniformVec4ARR(trProg.selectedProgram, uniformNum, value, size)
#define SetUniformMatrix16ARR(uniformNum, value, size) GLSL_SetUniformMatrix16ARR(trProg.selectedProgram, uniformNum, value, size)

#define SelectProgram(program) Ren_LogComment("SelectProgram called: (%s:%d)\n", __FILE__, __LINE__); GLSL_SelectPermutation(program)
#define SetMacrosAndSelectProgram(program, ...) GLSL_SetMacroStates(program, NUMARGS(__VA_ARGS__), ## __VA_ARGS__); SelectProgram(program)

#if 0
#define GL_JOIN() glFinish()
#define R2_TIMING(rule) if (r_speeds->integer == rule) { GL_JOIN(); } if (r_speeds->integer == rule)
#define R2_TIMING_SIMPLE() if (r_speeds->integer) { GL_JOIN(); } if (r_speeds->integer)
#else
#define GL_JOIN()
#define R2_TIMING(rule) if (r_speeds->integer == rule)
#define R2_TIMING_SIMPLE() if (r_speeds->integer)
#endif

#endif // TR_LOCAL_H
